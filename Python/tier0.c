/* Tier 0: compile IR blocks to x86-64 machine code
 * (boil/design/05-tier0-jit.md).
 *
 * The code of a block is a function with the tail-call handler signature,
 * which the interpreter enters at any instruction (Python/ceval_tier0.c.h).
 * An instruction's code starts with r12 the frame, r13 the stack pointer
 * and r15 the instruction's first word, as the handler signature has them.
 *
 * Loads and stores of locals and constants, jumps, branches on bools, and
 * int and float arithmetic and compares on inline values run inline.  Their
 * guards branch to a slow path out of line, which calls the handler.  Every
 * other instruction is such a call, with the word's fields as immediates,
 * followed by a test of the result:
 *
 *     word_w:  mov r14, tstate; mov edi, A; mov esi, B; mov edx, C
 *              movzx eax, word [r15]           the handler, as it is now
 *              call [call_table + rax * 8]
 *              cmp rax, frame;  jne leave      another frame, or NULL
 *              mov r12, rax;  mov r15, next_instr;  mov r13, stack_pointer
 *              cmp r15, &words[w + n];  jne redispatch
 *     word_w+n: ...
 *
 * The handler index is read from the word on each execution, so
 * specialisation needs no recompilation: an inline instruction guards the
 * values it handles, and its slow path calls the handler the word has now.
 * "redispatch" finds the code of another word of the block through the
 * jump table.  "leave" jumps to the machine code of the frame returned, if
 * it has code for its next word, and otherwise passes the result to
 * _PyTier0_Continue(), which goes on in the interpreter.  The code is
 * entered and left by tail calls, and the code of every block shares its C
 * frame, so a Python call or return through it does not grow the C stack.
 *
 * Inline code computes stack slots from the fields of the word, so it
 * agrees with the handlers of register-style formats, and leaves
 * frame->instr_ptr alone except before a call that may run Python code.
 * Instrumentation invalidates the code by overwriting the first PATCH_SIZE
 * bytes of every instruction's code with a jump to the interpreter
 * (_PyTier0_Invalidate()), so frames running the code leave at their next
 * instruction.  No instruction's code is shorter than that, and nothing
 * jumps or returns into its first PATCH_SIZE bytes.
 */

#include "Python.h"

#ifdef Py_TIER0

#include "pycore_ceval.h"           // _PY_EVAL_EVENTS_MASK
#include "pycore_code.h"            // COMPARISON_LESS_THAN
#include "pycore_interpframe.h"     // _PyInterpreterFrame
#include "pycore_ir.h"
#include "pycore_long.h"            // _PyLong_CompactValue()
#include "pycore_object.h"          // _Py_INCREF_MORTAL()
#include "pycore_range.h"           // _PyRangeIterObject
#include "pycore_stackref.h"
#include "pycore_tier0.h"
#include "pycore_tstate.h"          // _PyThreadStateImpl

#include "jit_x86.h"

#include <sys/mman.h>               // mmap()
#include <unistd.h>                 // sysconf()

#if !defined(Py_STACKREF_INLINE_VALUES) || defined(Py_STACKREF_DEBUG)
#  error "tier 0 emits the stackref layout of builds with inline values"
#endif

/* Registers at the start of the code of each instruction */
#define R_FRAME  X86_R12
#define R_SP     X86_R13
#define R_TSTATE X86_R14
#define R_NEXT   X86_R15

/* The stack frame of the code, below rbp */
#define SLOT_TSTATE (-8)
#define SLOT_FRAME (-16)
#define SLOT_TABLE (-24)

/* Where a handler in call mode leaves next_instr and stack_pointer */
#define OFF_NEXT_INSTR ((int32_t)offsetof(_PyThreadStateImpl, tier0.next_instr))
#define OFF_STACK_POINTER ((int32_t)offsetof(_PyThreadStateImpl, tier0.stack_pointer))

#define OFF_INSTR_PTR ((int32_t)offsetof(_PyInterpreterFrame, instr_ptr))
#define OFF_LOCALS ((int32_t)offsetof(_PyInterpreterFrame, localsplus))
#define OFF_REFCNT ((int32_t)offsetof(PyObject, ob_refcnt))
#define WORD ((int32_t)sizeof(_PyIRWord))
#define REF ((int32_t)sizeof(_PyStackRef))

/* Blocks larger than this stay interpreted. */
#define MAX_WORDS (1 << 16)

/* The bytes of an instruction's code that _PyTier0_Invalidate() overwrites:
 * jmp rel32. */
#define PATCH_SIZE 5

typedef struct {
    int label;
    uint64_t value;
} jit_const;

/* Code out of line, after the code of the instructions */
typedef enum {
    COLD_CALL,          // call the handler of word w, then go on at the next
    COLD_RELEASE,       // release the object in reg, then go on at resume
    COLD_JUMP,          // move r15 from word w to target and go on there
} cold_kind;

typedef struct {
    cold_kind kind;
    int label;
    uint32_t w;
    uint32_t target;
    jit_reg reg;
    int resume;
} cold_path;

typedef struct {
    jit_buf b;
    PyCodeObject *co;
    _PyIRBlock *block;
    int *word_label;            // per word, the label of its code, or -1
    jit_const *consts;          // 64-bit constants, read RIP-relative
    int nconsts;
    int consts_capacity;
    cold_path *cold;
    int ncold;
    int cold_capacity;
    int l_base;                 // the entry, at offset 0
    int l_interpret;            // the interpreter runs the word at r15
    int l_leave_rax;            // go on with the result in rax
    int l_continue;             // go on with the result in r12
    int l_redispatch;           // go on in this frame at r15
    int l_jumptable;
    int k_table;
    int k_resume;
    int k_words;
    int k_owned;                // Py_OWNED_MASK
    int k_int_base;
    int k_double_offset;
    int k_float_span;           // inline floats less Py_DOUBLE_OFFSET are below
    int k_true;
    int k_false;
    int k_release;              // tier0_release()
#ifdef Py_REF_DEBUG
    int k_dup;                  // tier0_dup()
#endif
} compiler;

/* Release op, an owned reference that the frame's stack no longer holds, sp
 * being the stack pointer that its destructor may see.  The machine code
 * counts down itself, and calls this at zero, except in builds that track
 * reference counts. */
static void
tier0_release(_PyInterpreterFrame *frame, _PyStackRef *sp, PyObject *op)
{
    _PyFrame_SetStackPointer(frame, sp);
    _PyFrame_StackPointerValidate(frame);
#ifdef Py_REF_DEBUG
    Py_DECREF_MORTAL(op);
#else
    _Py_Dealloc(op);
#endif
    _PyFrame_StackPointerInvalidate(frame);
}

#ifdef Py_REF_DEBUG
static PyObject *
tier0_dup(PyObject *op)
{
    _Py_INCREF_MORTAL(op);
    return op;
}
#endif

static int
add_const(compiler *c, uint64_t value)
{
    if (jit_grow((void **)&c->consts, &c->consts_capacity, c->nconsts + 1,
                 sizeof(jit_const)) < 0)
    {
        c->b.failed = 1;
        return 0;
    }
    int label = jit_new_label(&c->b);
    c->consts[c->nconsts++] = (jit_const){label, value};
    return label;
}

/* The label of a new path out of line */
static int
add_cold(compiler *c, cold_path path)
{
    if (jit_grow((void **)&c->cold, &c->cold_capacity, c->ncold + 1,
                 sizeof(cold_path)) < 0)
    {
        c->b.failed = 1;
        return 0;
    }
    path.label = jit_new_label(&c->b);
    c->cold[c->ncold++] = path;
    return path.label;
}

/* The slow path of the inline code of word w: a call of its handler */
static int
slow_path(compiler *c, uint32_t w)
{
    return add_cold(c, (cold_path){.kind = COLD_CALL, .w = w});
}

static int
instr_words(const _PyIRBlock *block, uint32_t w)
{
    return _PyIR_FormatOf(IR_HANDLER(block->words[w]))->nwords;
}

/* The label of the instruction at word w, or -1 if none starts there */
static int
instr_label(compiler *c, int64_t w)
{
    if (w < 0 || w >= c->block->nwords) {
        return -1;
    }
    return c->word_label[w];
}

/* The label of the instruction at word w, or the exit to the interpreter */
static int
next_label(compiler *c, uint32_t w)
{
    int label = instr_label(c, w);
    return label < 0 ? c->l_interpret : label;
}

/* Displacements from the frame of a slot field and of a local */
static int32_t
slot_disp(uint32_t field)
{
    return OFF_LOCALS + (int32_t)IR_SLOT_INDEX(field) * REF;
}

static int32_t
local_disp(uint32_t local)
{
    return OFF_LOCALS + (int32_t)local * REF;
}

/* Tail-called with r12 the frame, r13 its stack pointer, r14 the thread
 * state, r15 next_instr and edi the offset of the word's code. */
static void
emit_entry(compiler *c)
{
    jit_buf *b = &c->b;
    jit_bind(b, c->l_base);
    jit_push(b, X86_RBP);
    jit_mov(b, X86_RBP, X86_RSP);
    jit_push(b, R_TSTATE);
    jit_push(b, R_FRAME);
    jit_load_rip(b, X86_RAX, c->k_table);
    jit_push(b, X86_RAX);
    // Calls need rsp aligned to 16 bytes
    jit_sub_imm(b, X86_RSP, 8);
    jit_mov32(b, X86_RDI, X86_RDI);
    jit_lea_rip(b, X86_RAX, c->l_base);
    jit_add(b, X86_RAX, X86_RDI);
    jit_jmp_reg(b, X86_RAX);
}

/* Call the handler of the instruction at word w, of nwords words. */
static void
emit_call(compiler *c, uint32_t w, int nwords)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    // Every handler of the word's family has its format.
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    const jit_reg fields[3] = {X86_RDI, X86_RSI, X86_RDX};
    const uint32_t values[3] = {IR_A(word), IR_B(word), IR_C(word)};
    jit_load(b, R_TSTATE, X86_RBP, SLOT_TSTATE);
    for (int i = 0; i < 3; i++) {
        if (fmt->roles[i] != _PyIR_ROLE_NONE) {
            jit_mov_imm32(b, fields[i], values[i]);
        }
    }
    jit_load16(b, X86_RAX, R_NEXT, 0);
    jit_load(b, X86_RCX, X86_RBP, SLOT_TABLE);
    jit_call_table(b, X86_RCX, X86_RAX);
    jit_cmp_mem(b, X86_RAX, X86_RBP, SLOT_FRAME);
    jit_jcc(b, X86_NE, c->l_leave_rax);
    jit_mov(b, R_FRAME, X86_RAX);
    jit_load(b, X86_RCX, X86_RBP, SLOT_TSTATE);
    jit_load(b, R_NEXT, X86_RCX, OFF_NEXT_INSTR);
    jit_load(b, R_SP, X86_RCX, OFF_STACK_POINTER);
    int k_next = add_const(c, (uintptr_t)(c->block->words + w + nwords));
    jit_cmp_rip(b, R_NEXT, k_next);
    jit_jcc(b, X86_NE, c->l_redispatch);
}

/* The stack pointer after an instruction that leaves n values from the
 * slot of field. */
static void
emit_set_sp(compiler *c, uint32_t field, int n)
{
    jit_lea(&c->b, R_SP, R_FRAME, slot_disp(field) + n * REF);
}

static void
emit_advance(compiler *c, int nwords)
{
    jit_lea(&c->b, R_NEXT, R_NEXT, nwords * WORD);
}

/* PyStackRef_DUP() of the reference in reg, which stays there */
static void
emit_dup(compiler *c, jit_reg reg)
{
    jit_buf *b = &c->b;
    int done = jit_new_label(b);
    jit_test_rip(b, reg, c->k_owned);
    jit_jcc(b, X86_NE, done);
#ifdef Py_REF_DEBUG
    jit_mov(b, X86_RDI, reg);
    jit_call_rip(b, c->k_dup);
    if (reg != X86_RAX) {
        jit_mov(b, reg, X86_RAX);
    }
#else
    jit_arith_mem_imm8(b, 0, 0, reg, OFF_REFCNT, 1);
#endif
    jit_bind(b, done);
}

/* PyStackRef_XCLOSE() of the reference in reg, which the stack no longer
 * holds, with r13 already the stack pointer after the instruction.  reg is
 * one that C calls preserve if code after this needs other registers. */
static void
emit_release(compiler *c, jit_reg reg)
{
    jit_buf *b = &c->b;
    int done = jit_new_label(b);
    int slow = add_cold(c, (cold_path){.kind = COLD_RELEASE, .reg = reg,
                                       .resume = done});
    jit_test_rip(b, reg, c->k_owned);
#ifdef Py_REF_DEBUG
    jit_jcc(b, X86_E, slow);
#else
    jit_jcc(b, X86_NE, done);
    jit_arith_mem_imm8(b, 0, 5, reg, OFF_REFCNT, 1);
    jit_jcc(b, X86_E, slow);
#endif
    jit_bind(b, done);
}

/* Copy local into the slot at dst: a new reference if dup, else a borrowed
 * one, as PyStackRef_BorrowAt() makes. */
static void
emit_load_local(compiler *c, int32_t dst, uint32_t local, int dup)
{
    jit_buf *b = &c->b;
    int32_t src = local_disp(local);
    jit_load(b, X86_RAX, R_FRAME, src);
    if (dup) {
        emit_dup(c, X86_RAX);
    }
    else {
        int done = jit_new_label(b);
        jit_cmp16_mem_imm(b, R_FRAME, src + 6, 0);
        jit_jcc(b, X86_NE, done);
        jit_or_imm(b, X86_RAX, Py_TAG_REFCNT);
        jit_bind(b, done);
    }
    jit_store(b, R_FRAME, dst, X86_RAX);
}

/* Move the value in the slot of field in to local, leaving the value it
 * replaces in old. */
static void
emit_store_local(compiler *c, uint32_t in, uint32_t local, jit_reg old)
{
    jit_buf *b = &c->b;
    jit_load(b, X86_RAX, R_FRAME, slot_disp(in));
    jit_load(b, old, R_FRAME, local_disp(local));
    jit_store(b, R_FRAME, local_disp(local), X86_RAX);
}

/* The value that a constant load pushes, as its handler computes it */
static uintptr_t
constant_bits(PyCodeObject *co, int handler, uint32_t oparg)
{
    if (handler == LOAD_SMALL_INT_INLINE) {
        return PyStackRef_FromInt32((int32_t)oparg).bits;
    }
    if (handler == LOAD_SMALL_INT) {
        PyObject *obj = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + oparg];
        return PyStackRef_FromPyObjectBorrow(obj).bits;
    }
    PyObject *obj = PyTuple_GET_ITEM(co->co_consts, oparg);
    if (handler == LOAD_CONST_INLINE_INT) {
        int32_t value = (int32_t)_PyLong_CompactValue((PyLongObject *)obj);
        return PyStackRef_FromInt32(value).bits;
    }
    if (handler == LOAD_CONST_INLINE_FLOAT) {
        return PyStackRef_FromDoubleNotNaN(PyFloat_AS_DOUBLE(obj)).bits;
    }
    return PyStackRef_FromPyObjectBorrow(obj).bits;
}

/* The bool for flags as cond has them, in rax */
static void
emit_bool(compiler *c, jit_cond cond)
{
    jit_load_rip(&c->b, X86_RAX, c->k_false);
    jit_cmov_rip(&c->b, cond, X86_RAX, c->k_true);
}

/* The condition of a compare's result from oparg's mask of
 * COMPARISON_LESS_THAN and friends; for floats, as ucomisd sets flags.
 * Inline floats are never NaN, so the compare is ordered and the
 * unordered bit has no effect.  Returns -1 for a constant result. */
static int
compare_cond(uint32_t oparg, int is_float)
{
    switch (oparg & (COMPARISON_LESS_THAN | COMPARISON_GREATER_THAN
                     | COMPARISON_EQUALS))
    {
        case COMPARISON_LESS_THAN:
            return is_float ? X86_B : X86_L;
        case COMPARISON_GREATER_THAN:
            return is_float ? X86_A : X86_G;
        case COMPARISON_EQUALS:
            return X86_E;
        case COMPARISON_LESS_THAN | COMPARISON_GREATER_THAN:
            return X86_NE;
        case COMPARISON_LESS_THAN | COMPARISON_EQUALS:
            return is_float ? X86_BE : X86_LE;
        case COMPARISON_GREATER_THAN | COMPARISON_EQUALS:
            return is_float ? X86_AE : X86_GE;
    }
    return -1;
}

/* Load the inputs of fields B and C into rax and rcx and go to slow unless
 * both are inline ints. */
static void
emit_int_inputs(compiler *c, uint32_t w, int slow)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    jit_load(b, X86_RAX, R_FRAME, slot_disp(IR_B(word)));
    jit_load(b, X86_RCX, R_FRAME, slot_disp(IR_C(word)));
    // PyStackRef_BothInlineInt()
    jit_mov(b, X86_RDX, X86_RAX);
    jit_and(b, X86_RDX, X86_RCX);
    jit_cmp_rip(b, X86_RDX, c->k_int_base);
    jit_jcc(b, X86_B, slow);
}

/* Load the inputs of fields B and C as doubles into xmm0 and xmm1 and go to
 * slow unless both are inline floats; leaves Py_DOUBLE_OFFSET in rdx. */
static void
emit_float_inputs(compiler *c, uint32_t w, int slow)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    jit_load(b, X86_RAX, R_FRAME, slot_disp(IR_B(word)));
    jit_load(b, X86_RCX, R_FRAME, slot_disp(IR_C(word)));
    jit_load_rip(b, X86_RDX, c->k_double_offset);
    jit_sub(b, X86_RAX, X86_RDX);
    jit_cmp_rip(b, X86_RAX, c->k_float_span);
    jit_jcc(b, X86_AE, slow);
    jit_sub(b, X86_RCX, X86_RDX);
    jit_cmp_rip(b, X86_RCX, c->k_float_span);
    jit_jcc(b, X86_AE, slow);
    jit_movq_to_xmm(b, 0, X86_RAX);
    jit_movq_to_xmm(b, 1, X86_RCX);
}

/* BINARY_OP and COMPARE_OP on two inline ints or two inline floats, with
 * the result, an inline value or a bool, in the slot of field A. */
static int
emit_binary(compiler *c, uint32_t w)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    int handler = IR_HANDLER(word);
    int is_float = 0;
    int cond = -1;
    switch (handler) {
        case BINARY_OP_ADD_INT:
        case BINARY_OP_SUBTRACT_INT:
        case BINARY_OP_MULTIPLY_INT:
            break;
        case BINARY_OP_ADD_FLOAT:
        case BINARY_OP_SUBTRACT_FLOAT:
        case BINARY_OP_MULTIPLY_FLOAT:
            is_float = 1;
            break;
        case COMPARE_OP_INT:
        case COMPARE_OP_FLOAT:
            is_float = handler == COMPARE_OP_FLOAT;
            cond = compare_cond(IR_D(c->block->words[w + 1]), is_float);
            if (cond < 0) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    int slow = slow_path(c, w);
    if (is_float) {
        emit_float_inputs(c, w, slow);
    }
    else {
        emit_int_inputs(c, w, slow);
    }
    switch (handler) {
        case BINARY_OP_ADD_INT:
            jit_add32(b, X86_RAX, X86_RCX);
            break;
        case BINARY_OP_SUBTRACT_INT:
            jit_sub32(b, X86_RAX, X86_RCX);
            break;
        case BINARY_OP_MULTIPLY_INT:
            jit_imul32(b, X86_RAX, X86_RCX);
            break;
        case BINARY_OP_ADD_FLOAT:
            jit_addsd(b, 0, 1);
            break;
        case BINARY_OP_SUBTRACT_FLOAT:
            jit_subsd(b, 0, 1);
            break;
        case BINARY_OP_MULTIPLY_FLOAT:
            jit_mulsd(b, 0, 1);
            break;
        case COMPARE_OP_INT:
            jit_cmp32(b, X86_RAX, X86_RCX);
            break;
        case COMPARE_OP_FLOAT:
            jit_ucomisd(b, 0, 1);
            break;
    }
    if (cond >= 0) {
        emit_bool(c, (jit_cond)cond);
    }
    else if (is_float) {
        // The handler boxes a NaN.
        jit_ucomisd(b, 0, 0);
        jit_jcc(b, X86_P, slow);
        jit_movq_from_xmm(b, X86_RAX, 0);
        jit_add(b, X86_RAX, X86_RDX);
    }
    else {
        // The handler boxes a result that does not fit in int32.
        jit_jcc(b, X86_O, slow);
        jit_or_rip(b, X86_RAX, c->k_int_base);
    }
    jit_store(b, R_FRAME, slot_disp(IR_A(word)), X86_RAX);
    emit_set_sp(c, IR_A(word), 1);
    return 1;
}

/* FOR_ITER_RANGE on a range iterator whose next value fits in int32 */
static int
emit_for_iter_range(compiler *c, uint32_t w)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    uint32_t oparg = IR_D(c->block->words[w + 1]);
    // When exhausted, the handler jumps over the END_FOR at the target.
    uint32_t target = w + 2 + oparg + 1;
    if ((IR_B(word) & IR_BORROW) || instr_label(c, target) < 0) {
        return 0;
    }
    int slow = slow_path(c, w);
    int exhausted = add_cold(c, (cold_path){.kind = COLD_JUMP, .w = w,
                                            .target = target});
    jit_load(b, X86_RAX, R_FRAME, slot_disp(IR_B(word)));
    jit_and_imm(b, X86_RAX, ~Py_TAG_REFCNT);
    jit_mov_imm64(b, X86_RCX, (uintptr_t)&PyRangeIter_Type);
    jit_cmp_mem(b, X86_RCX, X86_RAX, (int32_t)offsetof(PyObject, ob_type));
    jit_jcc(b, X86_NE, slow);
    // _PyObject_IsUniquelyReferenced()
    jit_arith_mem_imm8(b, 0, 7, X86_RAX, OFF_REFCNT, 1);
    jit_jcc(b, X86_NE, slow);
    int32_t off_start = (int32_t)offsetof(_PyRangeIterObject, start);
    int32_t off_step = (int32_t)offsetof(_PyRangeIterObject, step);
    int32_t off_len = (int32_t)offsetof(_PyRangeIterObject, len);
    jit_load(b, X86_RCX, X86_RAX, off_len);
    jit_cmp_imm(b, X86_RCX, 0);
    jit_jcc(b, X86_LE, exhausted);
    jit_load(b, X86_RDX, X86_RAX, off_start);
    // The handler boxes a value that does not fit in int32.
    jit_movsxd(b, X86_RSI, X86_RDX);
    jit_cmp(b, X86_RSI, X86_RDX);
    jit_jcc(b, X86_NE, slow);
    jit_add_mem(b, X86_RSI, X86_RAX, off_step);
    jit_store(b, X86_RAX, off_start, X86_RSI);
    jit_sub_imm(b, X86_RCX, 1);
    jit_store(b, X86_RAX, off_len, X86_RCX);
    jit_mov32(b, X86_RAX, X86_RDX);
    jit_or_rip(b, X86_RAX, c->k_int_base);
    jit_store(b, R_FRAME, slot_disp(IR_A(word)), X86_RAX);
    emit_set_sp(c, IR_A(word), 1);
    return 1;
}

/* POP_JUMP_IF_FALSE and POP_JUMP_IF_TRUE on a bool, recording the branch
 * in the feedback entry's history as RECORD_BRANCH_TAKEN() does. */
static int
emit_pop_jump_if(compiler *c, uint32_t w)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    uint32_t cond = IR_A(word);
    int taken = instr_label(c, (int64_t)w + 1 + IR_C(word));
    if ((cond & IR_BORROW) || taken < 0) {
        return 0;
    }
    // Falling through skips a NOT_TAKEN, as the handler does.
    int fall = (w + 1 < c->block->nwords
                && IR_HANDLER(c->block->words[w + 1]) == NOT_TAKEN) ? 2 : 1;
    _PyFbHeader *fb = (_PyFbHeader *)(c->block->words + w + IR_B(word));
    jit_load(b, X86_RAX, R_FRAME, slot_disp(cond));
    emit_set_sp(c, cond, 0);
    jit_mov_imm64(b, X86_RDX, (uintptr_t)&fb->history);
    jit_load16(b, X86_RCX, X86_RDX, 0);
    jit_add32(b, X86_RCX, X86_RCX);
    int is_true = IR_HANDLER(word) == POP_JUMP_IF_TRUE;
    jit_cmp_rip(b, X86_RAX, is_true ? c->k_true : c->k_false);
    int not_taken = jit_new_label(b);
    jit_jcc(b, X86_NE, not_taken);
    jit_or_imm(b, X86_RCX, 1);
    jit_store16(b, X86_RDX, 0, X86_RCX);
    jit_lea(b, R_NEXT, R_NEXT, (int32_t)(1 + IR_C(word)) * WORD);
    jit_jmp(b, taken);
    jit_bind(b, not_taken);
    jit_store16(b, X86_RDX, 0, X86_RCX);
    emit_advance(c, fall);
    if (fall == 2) {
        jit_jmp(b, next_label(c, w + 2));
    }
    return 1;
}

/* JUMP_BACKWARD, which checks the eval breaker, and the other jumps */
static int
emit_jump(compiler *c, uint32_t w)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    int handler = IR_HANDLER(word);
    int64_t distance = handler == JUMP_FORWARD ? IR_A(word) : -(int64_t)IR_A(word);
    int64_t target = (int64_t)w + 1 + distance;
    int label = instr_label(c, target);
    if (label < 0) {
        return 0;
    }
    if (handler == JUMP_BACKWARD) {
        // check_periodics(); the handler handles what is pending.
        jit_load(b, X86_RAX, X86_RBP, SLOT_TSTATE);
        jit_test_mem_imm32(b, X86_RAX, (int32_t)offsetof(PyThreadState, eval_breaker),
                           _PY_EVAL_EVENTS_MASK);
        jit_jcc(b, X86_NE, slow_path(c, w));
    }
    jit_lea(b, R_NEXT, R_NEXT, (int32_t)(target - w) * WORD);
    jit_jmp(b, label);
    return 1;
}

/* Emit the instruction at word w inline, if it is one that runs inline, and
 * return whether it was. */
static int
emit_inline(compiler *c, uint32_t w)
{
    jit_buf *b = &c->b;
    _PyIRWord word = c->block->words[w];
    int handler = IR_HANDLER(word);
    uint32_t A = IR_A(word), B = IR_B(word), C = IR_C(word);
    switch (handler) {
        case NOP:
        case NOT_TAKEN:
            break;
        case LOAD_FAST:
        case LOAD_FAST_BORROW:
            emit_load_local(c, slot_disp(A), B, handler == LOAD_FAST);
            emit_set_sp(c, A, 1);
            break;
        case LOAD_FAST_LOAD_FAST:
        case LOAD_FAST_BORROW_LOAD_FAST_BORROW: {
            int dup = handler == LOAD_FAST_LOAD_FAST;
            emit_load_local(c, slot_disp(A), B >> 4, dup);
            emit_load_local(c, slot_disp(A) + REF, B & 15, dup);
            emit_set_sp(c, A, 2);
            break;
        }
        case STORE_FAST:
            if (A & IR_BORROW) {
                return 0;
            }
            emit_store_local(c, A, B, X86_RCX);
            emit_set_sp(c, A, 0);
            emit_release(c, X86_RCX);
            break;
        case STORE_FAST_STORE_FAST:
            if ((A | B) & IR_BORROW) {
                return 0;
            }
            emit_store_local(c, B, C >> 4, X86_RCX);
            emit_set_sp(c, B, 0);
            emit_release(c, X86_RCX);
            emit_store_local(c, A, C & 15, X86_RCX);
            emit_set_sp(c, A, 0);
            emit_release(c, X86_RCX);
            break;
        case STORE_FAST_LOAD_FAST:
            if (B & IR_BORROW) {
                return 0;
            }
            // rbx survives the call that counts in debug builds.
            emit_store_local(c, B, C >> 4, X86_RBX);
            emit_load_local(c, slot_disp(A), C & 15, 1);
            emit_set_sp(c, A, 1);
            emit_release(c, X86_RBX);
            break;
        case POP_TOP:
            if (A & IR_BORROW) {
                return 0;
            }
            jit_load(b, X86_RCX, R_FRAME, slot_disp(A));
            emit_set_sp(c, A, 0);
            emit_release(c, X86_RCX);
            break;
        case LOAD_CONST:
        case LOAD_CONST_INLINE_INT:
        case LOAD_CONST_INLINE_FLOAT:
        case LOAD_SMALL_INT:
        case LOAD_SMALL_INT_INLINE:
            jit_mov_imm64(b, X86_RAX, constant_bits(c->co, handler, B));
            jit_store(b, R_FRAME, slot_disp(A), X86_RAX);
            emit_set_sp(c, A, 1);
            break;
        case TO_BOOL_BOOL: {
            if ((B & IR_BORROW) || A != B) {
                return 0;
            }
            int done = jit_new_label(b);
            jit_load(b, X86_RAX, R_FRAME, slot_disp(B));
            jit_cmp_rip(b, X86_RAX, c->k_true);
            jit_jcc(b, X86_E, done);
            jit_cmp_rip(b, X86_RAX, c->k_false);
            jit_jcc(b, X86_NE, slow_path(c, w));
            jit_bind(b, done);
            emit_set_sp(c, A, 1);
            break;
        }
        case JUMP_BACKWARD:
        case JUMP_BACKWARD_NO_INTERRUPT:
        case JUMP_FORWARD:
            return emit_jump(c, w);
        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_TRUE:
            return emit_pop_jump_if(c, w);
        case FOR_ITER_RANGE:
            if (!emit_for_iter_range(c, w)) {
                return 0;
            }
            break;
        default:
            if (!emit_binary(c, w)) {
                return 0;
            }
            break;
    }
    emit_advance(c, instr_words(c->block, w));
    return 1;
}

static void
emit_cold(compiler *c)
{
    jit_buf *b = &c->b;
    for (int i = 0; i < c->ncold; i++) {
        cold_path path = c->cold[i];
        jit_bind(b, path.label);
        switch (path.kind) {
            case COLD_CALL: {
                int nwords = instr_words(c->block, path.w);
                emit_call(c, path.w, nwords);
                jit_jmp(b, next_label(c, path.w + nwords));
                break;
            }
            case COLD_RELEASE:
                // A destructor may ask for the line number.
                jit_store(b, R_FRAME, OFF_INSTR_PTR, R_NEXT);
                jit_mov(b, X86_RDI, R_FRAME);
                jit_mov(b, X86_RSI, R_SP);
                jit_mov(b, X86_RDX, path.reg);
                jit_call_rip(b, c->k_release);
                jit_jmp(b, path.resume);
                break;
            case COLD_JUMP:
                jit_lea(b, R_NEXT, R_NEXT, (int32_t)(path.target - path.w) * WORD);
                jit_jmp(b, next_label(c, path.target));
                break;
        }
    }
}

/* Go on with the frame in r12, which a handler returned, in its machine
 * code if it has some for its next word, as _PyTier0_Continue() would, but
 * without leaving the C frame, which the code of every block shares.
 * Falls through if the frame is NULL or tagged, or goes on elsewhere. */
static void
emit_switch(compiler *c)
{
    jit_buf *b = &c->b;
    int slow = c->l_continue;
    jit_test(b, R_FRAME, R_FRAME);
    jit_jcc(b, X86_E, slow);
    jit_test_imm(b, R_FRAME, _PyTier0_INTERPRET);
    jit_jcc(b, X86_NE, slow);
    jit_load(b, X86_RCX, X86_RBP, SLOT_TSTATE);
    jit_load(b, R_NEXT, X86_RCX, OFF_NEXT_INSTR);
    jit_load(b, R_SP, X86_RCX, OFF_STACK_POINTER);
    jit_cmp8_mem_imm(b, R_FRAME, (int32_t)offsetof(_PyInterpreterFrame, owner),
                     FRAME_OWNED_BY_INTERPRETER);
    jit_jcc(b, X86_E, slow);
    // rax: the block of the frame's code, rcx: its machine code
    jit_load(b, X86_RAX, R_FRAME, (int32_t)offsetof(_PyInterpreterFrame, f_executable));
    jit_and_imm(b, X86_RAX, ~Py_TAG_REFCNT);
    jit_load(b, X86_RAX, X86_RAX, (int32_t)offsetof(PyCodeObject, _co_ir));
    jit_load(b, X86_RCX, X86_RAX, (int32_t)offsetof(_PyIRBlock, tier0));
    jit_test(b, X86_RCX, X86_RCX);
    jit_jcc(b, X86_E, slow);
    // Invalid code has a jump to the interpreter at each word, so validity
    // needs no test.  A frame that has not started may be at the stub, and
    // one that IR_REMAP has yet to move in the block that this one
    // replaced, out of the block.
    jit_mov(b, X86_RDX, R_NEXT);
    jit_sub_mem(b, X86_RDX, X86_RAX, (int32_t)offsetof(_PyIRBlock, words));
    jit_load32(b, X86_RSI, X86_RAX, (int32_t)offsetof(_PyIRBlock, nwords));
    jit_shl_imm(b, X86_RSI, 3);
    jit_cmp(b, X86_RDX, X86_RSI);
    jit_jcc(b, X86_AE, slow);
    // Entries of the offsets are 4 bytes, words 8
    jit_shr_imm(b, X86_RDX, 1);
    jit_load(b, X86_RSI, X86_RCX, (int32_t)offsetof(_PyTier0Code, offsets));
    jit_movsxd_index(b, X86_RAX, X86_RSI, X86_RDX);
    jit_cmp_imm(b, X86_RAX, 0);
    jit_jcc(b, X86_E, slow);
    jit_add_mem(b, X86_RAX, X86_RCX, (int32_t)offsetof(_PyTier0Code, entry));
    jit_store(b, X86_RBP, SLOT_FRAME, R_FRAME);
    jit_jmp_reg(b, X86_RAX);
}

static void
emit_exits(compiler *c)
{
    jit_buf *b = &c->b;
    // The state of the word at r15 is in registers; store it where
    // _PyTier0_Continue() finds it.
    jit_bind(b, c->l_interpret);
    jit_load(b, X86_RAX, X86_RBP, SLOT_TSTATE);
    jit_store(b, X86_RAX, OFF_NEXT_INSTR, R_NEXT);
    jit_store(b, X86_RAX, OFF_STACK_POINTER, R_SP);
    jit_lea(b, R_FRAME, R_FRAME, _PyTier0_INTERPRET);
    jit_jmp(b, c->l_continue);

    jit_bind(b, c->l_leave_rax);
    jit_mov(b, R_FRAME, X86_RAX);
    emit_switch(c);
    jit_bind(b, c->l_continue);
    jit_load(b, R_TSTATE, X86_RBP, SLOT_TSTATE);
    jit_leave(b);
    jit_jmp_rip(b, c->k_resume);

    // A jump, an exception handler or a handler that runs its word again
    jit_bind(b, c->l_redispatch);
    jit_mov(b, X86_RAX, R_NEXT);
    jit_load_rip(b, X86_RCX, c->k_words);
    jit_sub(b, X86_RAX, X86_RCX);
    jit_cmp_imm(b, X86_RAX, (int32_t)(c->block->nwords * sizeof(_PyIRWord)));
    jit_jcc(b, X86_AE, c->l_interpret);
    // Entries of the jump table are 4 bytes, words 8
    jit_shr_imm(b, X86_RAX, 1);
    jit_lea_rip(b, X86_RCX, c->l_jumptable);
    jit_movsxd_index(b, X86_RAX, X86_RCX, X86_RAX);
    jit_cmp_imm(b, X86_RAX, 0);
    jit_jcc(b, X86_E, c->l_interpret);
    jit_lea_rip(b, X86_RCX, c->l_base);
    jit_add(b, X86_RAX, X86_RCX);
    jit_jmp_reg(b, X86_RAX);
}

/* The constants, then the jump table; return the table's offset. */
static int
emit_data(compiler *c)
{
    jit_buf *b = &c->b;
    jit_align(b, 8);
    for (int i = 0; i < c->nconsts; i++) {
        jit_bind(b, c->consts[i].label);
        jit_u64(b, c->consts[i].value);
    }
    jit_bind(b, c->l_jumptable);
    int table = b->size;
    for (uint32_t w = 0; w < c->block->nwords; w++) {
        int label = c->word_label[w];
        jit_u32(b, label < 0 || b->failed ? 0 : (uint32_t)b->labels[label]);
    }
    return table;
}

static void
init_constants(compiler *c)
{
    c->k_table = add_const(c, (uintptr_t)_PyTier0_Runtime.call_table);
    c->k_resume = add_const(c, (uintptr_t)_PyTier0_Runtime.resume);
    c->k_words = add_const(c, (uintptr_t)c->block->words);
    c->k_owned = add_const(c, Py_OWNED_MASK);
    c->k_int_base = add_const(c, Py_INT_BASE);
    c->k_double_offset = add_const(c, Py_DOUBLE_OFFSET);
    c->k_float_span = add_const(c, Py_INT_BASE - Py_DOUBLE_OFFSET);
    c->k_true = add_const(c, PyStackRef_True.bits);
    c->k_false = add_const(c, PyStackRef_False.bits);
    c->k_release = add_const(c, (uintptr_t)tier0_release);
#ifdef Py_REF_DEBUG
    c->k_dup = add_const(c, (uintptr_t)tier0_dup);
#endif
}

static int
emit_block(compiler *c)
{
    jit_buf *b = &c->b;
    _PyIRBlock *block = c->block;
    c->l_base = jit_new_label(b);
    c->l_interpret = jit_new_label(b);
    c->l_leave_rax = jit_new_label(b);
    c->l_continue = jit_new_label(b);
    c->l_redispatch = jit_new_label(b);
    c->l_jumptable = jit_new_label(b);
    init_constants(c);
    for (uint32_t w = 0; w < block->nwords; w += instr_words(block, w)) {
        c->word_label[w] = jit_new_label(b);
    }
    emit_entry(c);
    for (uint32_t w = 0; w < block->nwords; w += instr_words(block, w)) {
        int start = b->size;
        jit_bind(b, c->word_label[w]);
        if (!emit_inline(c, w)) {
            emit_call(c, w, instr_words(block, w));
        }
        if (b->size - start < PATCH_SIZE) {
            jit_nop(b, PATCH_SIZE - (b->size - start));
        }
    }
    // No instruction falls off the end of a block.
    jit_jmp(b, c->l_interpret);
    emit_cold(c);
    emit_exits(c);
    int table = emit_data(c);
    if (jit_resolve(b) < 0) {
        return -1;
    }
    return table;
}

/* Whether the code of block can be compiled now (1), not while it is
 * instrumented (0), or never (-1). */
static int
compilable(_PyIRBlock *block)
{
    if (block->nwords > MAX_WORDS
        || block->tier0_compiles >= _PyTier0_MAX_COMPILES)
    {
        return -1;
    }
    for (uint32_t w = 0; w < block->nwords; ) {
        int handler = IR_HANDLER(block->words[w]);
        const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
        if (handler == IR_EXT) {
            // The prefix widens the fields of the next word, which entries
            // at that word would not see.
            return -1;
        }
        if ((handler >= MIN_INSTRUMENTED_OPCODE && handler < IR_FIRST_ONLY_HANDLER)
            || (fmt->flags & _PyIR_TRANSPARENT))
        {
            return 0;
        }
        w += fmt->nwords;
    }
    return 1;
}

static void
free_code(_PyTier0Code *code)
{
    munmap(code->entry, code->size);
    PyMem_RawFree(code);
}

/* Map the code of b, executable and not writable.  The record comes from
 * the raw allocator, so that code compiled while regrtest hunts leaks does
 * not count as a leaked block. */
static _PyTier0Code *
install(const jit_buf *b, int table, int32_t interpret)
{
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    size_t size = ((size_t)b->size + page - 1) / page * page;
    _PyTier0Code *code = PyMem_RawMalloc(sizeof(_PyTier0Code));
    if (code == NULL) {
        return NULL;
    }
    char *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        PyMem_RawFree(code);
        return NULL;
    }
    memcpy(mem, b->code, b->size);
    if (mprotect(mem, size, PROT_READ | PROT_EXEC) < 0) {
        munmap(mem, size);
        PyMem_RawFree(code);
        return NULL;
    }
    *code = (_PyTier0Code){
        .entry = mem,
        .offsets = (const int32_t *)(mem + table),
        .size = size,
        .interpret = interpret,
        .valid = 1,
    };
    return code;
}

_PyTier0Code *
_PyTier0_Compile(PyCodeObject *co)
{
    _PyIRBlock *block = co->_co_ir;
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock) {
        return NULL;
    }
    _PyTier0Code *code = _PyTier0_ValidCode(block);
    if (code != NULL) {
        return code;
    }
    block->tier0_heat = 0;
    int ok = compilable(block);
    if (ok <= 0) {
        if (ok < 0) {
            block->tier0_heat = INT32_MIN;
        }
        return NULL;
    }
    compiler c = {.co = co, .block = block};
    c.word_label = PyMem_Malloc(block->nwords * sizeof(int));
    if (c.word_label != NULL) {
        for (uint32_t w = 0; w < block->nwords; w++) {
            c.word_label[w] = -1;
        }
        int table = emit_block(&c);
        if (table >= 0) {
            code = install(&c.b, table, c.b.labels[c.l_interpret]);
        }
    }
    PyMem_Free(c.word_label);
    PyMem_Free(c.consts);
    PyMem_Free(c.cold);
    jit_free(&c.b);
    if (code == NULL) {
        block->tier0_heat = INT32_MIN;
        return NULL;
    }
    block->tier0_compiles++;
    code->older = block->tier0;
    block->tier0 = code;
    return code;
}

void
_PyTier0_Invalidate(_PyIRBlock *block)
{
    _PyTier0Code *code = _PyTier0_ValidCode(block);
    if (code == NULL) {
        return;
    }
    code->valid = 0;
    uint8_t *mem = code->entry;
    if (mprotect(mem, code->size, PROT_READ | PROT_WRITE) < 0) {
        Py_FatalError("cannot make tier 0 code writable");
    }
    for (uint32_t w = 0; w < block->nwords; w++) {
        int32_t at = code->offsets[w];
        if (at != 0) {
            int32_t disp = code->interpret - (at + PATCH_SIZE);
            mem[at] = 0xE9;     // jmp rel32
            memcpy(mem + at + 1, &disp, sizeof(disp));
        }
    }
    if (mprotect(mem, code->size, PROT_READ | PROT_EXEC) < 0) {
        Py_FatalError("cannot make tier 0 code executable");
    }
}

void
_PyTier0_Clear(_PyIRBlock *block)
{
    _PyTier0Code *code = block->tier0;
    while (code != NULL) {
        _PyTier0Code *older = code->older;
        free_code(code);
        code = older;
    }
    block->tier0 = NULL;
}

#endif /* Py_TIER0 */
