/* Tier 0: compile IR blocks to x86-64 machine code that calls the handlers
 * of the IR (boil/design/05-tier0-jit.md).
 *
 * The code of a block is a function with the tail-call handler signature,
 * which the interpreter enters at any instruction (Python/ceval_tier0.c.h).
 * Each instruction becomes a call of its handler in call mode, with the
 * word's fields as immediates, followed by a test of the result:
 *
 *     word_w:  mov r14, tstate; mov edi, A; mov esi, B; mov edx, C
 *              movzx eax, word [r15]           the handler, as it is now
 *              call [call_table + rax * 8]
 *              cmp rax, frame;  jne leave      another frame, or NULL
 *              mov r12, rax;  mov r15, next_instr;  mov r13, stack_pointer
 *              cmp r15, &words[w + n];  jne redispatch
 *     word_w+n: ...
 *
 * so that a word starts with r12 the frame, r13 the stack pointer and r15
 * the word's address, as the handler signature has them.  The handler
 * index is read from the word on each execution, so specialisation needs
 * no recompilation.  "redispatch" finds the code of another word of the
 * block through the jump table; "leave" passes the result to
 * _PyTier0_Continue(), which goes on in the new frame's machine code or in
 * the interpreter.  The code is entered and left by tail calls, so it
 * never holds C stack between instructions of different frames.
 */

#include "Python.h"

#ifdef Py_TIER0

#include "pycore_interpframe.h"     // _PyInterpreterFrame
#include "pycore_ir.h"
#include "pycore_tier0.h"
#include "pycore_tstate.h"          // _PyThreadStateImpl

#include "jit_x86.h"

#include <sys/mman.h>               // mmap()
#include <unistd.h>                 // sysconf()

/* Registers at the start of the code of each word */
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

/* Blocks larger than this stay interpreted. */
#define MAX_WORDS (1 << 16)

typedef struct {
    int label;
    uint64_t value;
} jit_const;

typedef struct {
    jit_buf b;
    _PyIRBlock *block;
    int *word_label;            // per word, the label of its code, or -1
    jit_const *consts;          // 64-bit constants, read RIP-relative
    int nconsts;
    int consts_capacity;
    int l_base;                 // the entry, at offset 0
    int l_interpret;            // the interpreter runs the word at r15
    int l_leave_rax;            // go on with the result in rax
    int l_continue;             // go on with the result in r12
    int l_redispatch;           // go on in this frame at r15
    int l_jumptable;
    int k_table;
    int k_resume;
    int k_words;
} compiler;

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
    jit_load(b, R_TSTATE, X86_RBP, SLOT_TSTATE);
    jit_mov_imm32(b, X86_RDI, IR_A(word));
    jit_mov_imm32(b, X86_RSI, IR_B(word));
    jit_mov_imm32(b, X86_RDX, IR_C(word));
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
    c->k_table = add_const(c, (uintptr_t)_PyTier0_Runtime.call_table);
    c->k_resume = add_const(c, (uintptr_t)_PyTier0_Runtime.resume);
    c->k_words = add_const(c, (uintptr_t)block->words);
    emit_entry(c);
    for (uint32_t w = 0; w < block->nwords; ) {
        int handler = IR_HANDLER(block->words[w]);
        int nwords = _PyIR_FormatOf(handler)->nwords;
        c->word_label[w] = jit_new_label(b);
        jit_bind(b, c->word_label[w]);
        emit_call(c, w, nwords);
        w += nwords;
    }
    // No instruction falls off the end of a block.
    jit_jmp(b, c->l_interpret);
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
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock
        || block->nwords > MAX_WORDS
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
install(const jit_buf *b, int table)
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
        .valid = 1,
    };
    return code;
}

_PyTier0Code *
_PyTier0_Compile(_PyIRBlock *block)
{
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
    compiler c = {.block = block};
    c.word_label = PyMem_Malloc(block->nwords * sizeof(int));
    if (c.word_label != NULL) {
        for (uint32_t w = 0; w < block->nwords; w++) {
            c.word_label[w] = -1;
        }
        int table = emit_block(&c);
        if (table >= 0) {
            code = install(&c.b, table);
        }
    }
    PyMem_Free(c.word_label);
    PyMem_Free(c.consts);
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
    if (code != NULL) {
        code->valid = 0;
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
