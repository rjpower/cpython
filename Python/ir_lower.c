/* Lowering of stack bytecode to the register IR, and the accessors of IR
 * blocks.  See boil/design/06-architecture-v2.md, section 2, and
 * Include/internal/pycore_ir.h.
 *
 * A code object starts at _PyIR_LowerStub.  The stub's handler, IR_LOWER,
 * calls _PyIR_LowerFrame(), which lowers co_code_adaptive into a block the
 * first time a frame of the code runs, publishes it in co->_co_ir and moves
 * the frame there.  The lowering emits one word per bytecode instruction,
 * except that
 *
 * - EXTENDED_ARG units vanish into the fields of their instruction;
 * - inline caches vanish: each instruction that has a feedback entry names
 *   it by its index in the feedback vector, which follows the words;
 * - a LOAD_FAST_BORROW (or LOAD_FAST_BORROW_LOAD_FAST_BORROW) that pushes
 *   the top inputs of the next instruction, which consumes them as objects
 *   on the same line and in the same basic block, emits nothing.  The
 *   consumer reads the local through its input field, which then differs
 *   from the input's canonical slot.  Code objects lowered while
 *   instrumentation is active absorb nothing, so that every instruction
 *   keeps its INSTRUCTION events;
 * - a FOR_ITER or SEND whose exit is more than IR_MAX_FORWARD words away is
 *   relaxed: it exits to words after its own that jump to the far targets,
 *   and its next word jumps over them to the body (relax()).
 *
 * Temporary d lives in slot co_nlocalsplus + d, so every handler keeps its
 * stack-interpreter body, and a frame has at most IR_MAX_SLOTS slots.
 * Depths come from the stack effects of the bytecode, as in
 * calculate_stackdepth().  A consumer of absorbed loads, whose canonical
 * slots the loads leave as they were, finds stack_pointer through its dst
 * field (IR_STACK_POINTER in Python/ceval_macros.h).
 *
 * Feedback entries are FB_STRIDE bytes apart.  Entry 0 is shared by the
 * sites past the IR_FIELD_MAX-th, never specialises, and holds no static
 * operand: those sites read theirs from the bytecode (_PyIR_BytecodeOparg).
 *
 * The pcmap gives each word two bytecode offsets: start, the first code unit
 * of its instruction (an EXTENDED_ARG if it has one, or the first absorbed
 * load), and anchor, the instruction itself.  The anchor is f_lasti and keys
 * instrumentation, line events included; the start is where jumps and
 * exception handlers land.  The words that relax() adds share the entry of
 * their loop instruction.
 *
 * Instrumentation keeps rewriting co_code_adaptive, which the IR shadows:
 * _PyIR_SyncInstrumentation() copies each rewrite to the word of the
 * instruction.  The handler that the lowering chose, which instrumentation
 * displaces, is kept in block->real once the code is instrumented.
 */

#define NEED_IR_METADATA            // defines _PyIR_Formats, _PyIR_OnlyNames
#include "Python.h"

#include "pycore_backoff.h"         // adaptive_counter_warmup()
#include "pycore_code.h"            // _Py_GetBaseCodeUnit()
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_interpframe.h"     // _PyFrame_GetCode()
#include "pycore_ir.h"
#include "pycore_long.h"            // _PyLong_CheckExactAndCompact()
#include "pycore_opcode_metadata.h" // _PyOpcode_num_popped()
#include "pycore_opcode_utils.h"    // IS_SCOPE_EXIT_OPCODE()
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pystate.h"         // _PyInterpreterState_GET()
#include "pycore_stackref.h"        // Py_STACKREF_INLINE_VALUES

static_assert(sizeof(_PyIRBlock) % sizeof(_PyIRWord) == 0,
              "words must follow the block header aligned");
static_assert(sizeof(_PyFbBlock) % sizeof(_PyIRWord) == 0,
              "entries must follow the feedback header aligned");
static_assert(FB_STRIDE % FB_ALIGN == 0, "entries must stay aligned");
static_assert(sizeof(_PyFbUnary) <= FB_STRIDE && sizeof(_PyFbBinary) <= FB_STRIDE
              && sizeof(_PyFbGlobal) <= FB_STRIDE && sizeof(_PyFbCall) <= FB_STRIDE,
              "FB_STRIDE must hold every entry");


/* Static blocks */

static _PyIRWord lower_stub_words[2] = {
    IR_WORD(IR_LOWER, 0, 0, 0, 0),
    IR_WORD(IR_LOWER, 0, 0, 0, 0),
};

static _PyIRPc16 lower_stub_pcmap[2] = {{0, 0}, {0, 0}};

_PyIRBlock _PyIR_LowerStub = {
    .words = lower_stub_words,
    .nwords = 2,
    .pcmap = lower_stub_pcmap,
};

/* _Py_InitCleanup runs with self in slot 0 and the result of __init__ in
 * slot 1: EXIT_INIT_CHECK checks slot 1, and RETURN_VALUE returns slot 0. */
static _PyIRWord init_cleanup_words[2] = {
    IR_WORD(EXIT_INIT_CHECK, 1, 0, 0, 0),
    IR_WORD(RETURN_VALUE, 0, 0, 0, 0),
};

static _PyIRPc16 init_cleanup_pcmap[2] = {{0, 0}, {1, 1}};

_PyIRBlock _PyIR_InitCleanupBlock = {
    .words = init_cleanup_words,
    .nwords = 2,
    .pcmap = init_cleanup_pcmap,
};


/* The pcmap */

static inline int
pc_start(const _PyIRBlock *block, Py_ssize_t w)
{
    if (block->flags & _PyIR_PCMAP_WIDE) {
        return (int)((const _PyIRPc32 *)block->pcmap)[w].start;
    }
    return ((const _PyIRPc16 *)block->pcmap)[w].start;
}

static inline int
pc_anchor(const _PyIRBlock *block, Py_ssize_t w)
{
    if (block->flags & _PyIR_PCMAP_WIDE) {
        return (int)((const _PyIRPc32 *)block->pcmap)[w].anchor;
    }
    return ((const _PyIRPc16 *)block->pcmap)[w].anchor;
}

static inline Py_ssize_t
word_index(const _PyIRBlock *block, const _PyIRWord *instr)
{
    Py_ssize_t w = instr - block->words;
    assert(w >= 0 && w < (Py_ssize_t)block->nwords);
    return w;
}

int
_PyIR_Anchor(const _PyIRBlock *block, const _PyIRWord *instr)
{
    return pc_anchor(block, word_index(block, instr));
}

int
_PyIR_Start(const _PyIRBlock *block, const _PyIRWord *instr)
{
    return pc_start(block, word_index(block, instr));
}

_PyIRWord *
_PyIR_InstrAt(const _PyIRBlock *block, int offset)
{
    // The last word whose instruction starts at or before offset
    Py_ssize_t lo = 0, hi = block->nwords;
    while (hi - lo > 1) {
        Py_ssize_t mid = lo + (hi - lo) / 2;
        if (pc_start(block, mid) <= offset) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    // Back to the word of that instruction, before any that relax() added
    int start = pc_start(block, lo);
    while (lo > 0 && pc_start(block, lo - 1) == start) {
        lo--;
    }
    return block->words + lo;
}

static inline int
is_trampoline(const _PyIRBlock *block, Py_ssize_t w)
{
    return w > 0 && IR_HANDLER(block->words[w]) == JUMP_FORWARD
        && pc_start(block, w - 1) == pc_start(block, w)
        && pc_anchor(block, w - 1) == pc_anchor(block, w);
}

int
_PyIR_IsTrampoline(const _PyIRBlock *block, const _PyIRWord *instr)
{
    return is_trampoline(block, word_index(block, instr));
}

/* The value of the field at loc of the word at instr */
static inline uint32_t
read_field(const _PyIRWord *instr, int loc)
{
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*instr);
    switch (loc) {
        case _PyIR_LOC_A:
            return IR_A(word);
        case _PyIR_LOC_B:
            return IR_B(word);
        case _PyIR_LOC_C:
            return IR_C(word);
        case _PyIR_LOC_D:
            return IR_D(word);
        case _PyIR_LOC_CD:
            return IR_CD(word);
        default:
            Py_UNREACHABLE();
    }
}

/* The handler whose format decodes the word at instr */
static inline int
decoding_handler(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    if (_PyIR_FormatOf(handler)->flags & _PyIR_TRANSPARENT) {
        assert(block->real != NULL);
        handler = block->real[word_index(block, instr)];
    }
    return handler;
}

/* The feedback index of the word at instr, which handler decodes, or -1 */
static inline int
fb_index(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    if (block == &_PyIR_LowerStub) {
        return -1;
    }
    handler = decoding_handler(block, instr, handler);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->fb == _PyIR_LOC_NONE) {
        return -1;
    }
    return (int)read_field(instr, fmt->fb);
}

_PyFbHeader *
_PyIR_InstrFeedback(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    int i = fb_index(block, instr, handler);
    if (i < 0) {
        return NULL;
    }
    return _PyIR_FbEntry(block->entries, (uint32_t)i);
}

/* The oparg in the bytecode of the instruction whose word is at instr:
 * decode from its start, which may be an absorbed load, to its anchor. */
static int
bytecode_oparg(PyCodeObject *co, const _PyIRBlock *block, const _PyIRWord *instr)
{
    Py_ssize_t w = word_index(block, instr);
    int anchor = pc_anchor(block, w);
    int oparg = 0;
    for (int i = pc_start(block, w); i <= anchor; ) {
        _Py_CODEUNIT unit = _Py_GetBaseCodeUnit(co, i);
        if (i == anchor) {
            return (oparg << 8) | unit.op.arg;
        }
        if (unit.op.code == EXTENDED_ARG) {
            oparg = (oparg << 8) | unit.op.arg;
            i++;
        }
        else {
            oparg = 0;
            i += 1 + _PyOpcode_Caches[unit.op.code];
        }
    }
    Py_UNREACHABLE();
}

int
_PyIR_BytecodeOparg(_PyInterpreterFrame *frame, const _PyIRWord *instr)
{
    return bytecode_oparg(_PyFrame_GetCode(frame), _PyFrame_IRBlock(frame),
                          instr);
}

int
_PyIR_InstrOparg(PyCodeObject *co, const _PyIRBlock *block,
                 const _PyIRWord *instr, int handler)
{
    if (block == &_PyIR_LowerStub) {
        return 0;
    }
    handler = decoding_handler(block, instr, handler);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    switch (fmt->imm) {
        case _PyIR_LOC_NONE:
            return 0;
        case _PyIR_LOC_FBARG: {
            int i = fb_index(block, instr, handler);
            if (i == 0) {
                return bytecode_oparg(co, block, instr);
            }
            return (int)_PyIR_FbEntry(block->entries, (uint32_t)i)->arg;
        }
        default:
            return (int)read_field(instr, fmt->imm);
    }
}

int
_PyIR_RealHandler(_PyInterpreterFrame *frame, const _PyIRWord *instr)
{
    return decoding_handler(_PyFrame_IRBlock(frame), instr, IR_HANDLER(*instr));
}

/* The opcode of co_code_adaptive that the lowered handler stands for */
static inline int
lowered_opcode(int handler)
{
    if (handler >= IR_FIRST_ONLY_HANDLER) {
        return _PyIR_Formats[handler].shadow;
    }
    return _PyOpcode_Deopt[handler];
}

int
_PyIR_GenericHandler(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    handler = decoding_handler(block, instr, handler);
    if (handler >= _PyIR_BINARY_OP_FIRST
        && handler <= _PyIR_BINARY_OP_FIRST + NB_OPARG_LAST)
    {
        return handler;
    }
    int opcode = _PyOpcode_Deopt[lowered_opcode(handler)];
    if (opcode == BINARY_OP) {
        int i = fb_index(block, instr, handler);
        assert(i > 0);
        return _PyIR_BINARY_OP_FIRST
            + (int)_PyIR_FbEntry(block->entries, (uint32_t)i)->arg;
    }
    return opcode;
}

int
_PyIR_OriginalHandler(_PyInterpreterFrame *frame, const _PyIRWord *instr,
                      int opcode)
{
    const _PyIRBlock *block = _PyFrame_IRBlock(frame);
    if (block->real == NULL) {
        return opcode;
    }
    int handler = block->real[word_index(block, instr)];
    return lowered_opcode(handler) == opcode ? handler : opcode;
}


/* Lowering */

typedef struct {
    int start;          // offset of the first code unit, EXTENDED_ARG included
    int anchor;         // offset of the instruction
    int opcode;         // its base opcode: unspecialised, uninstrumented
    int top;            // the opcode at anchor in co_code_adaptive
    int oparg;
    int popped;         // its stack effect
    int pushed;
    int depth;          // stack depth before, or -1 if unreachable
    int target;         // the instruction a jump goes to, or -1
    int handler;        // the handler that the lowering chooses
    int word;           // index of its word
    int fb;             // index of its feedback entry, or -1
    int pc_start;       // the start of its words in the pcmap
    int borrow[3];      // per input: 1 + the local of an absorbed load, or 0
    uint8_t block_start;// an event or a jump may come in before it
    uint8_t absorbed;   // a load that its consumer reads through a field
    uint8_t nwords;     // 1, or with the words that relax() adds more
} lower_insn;

typedef struct {
    PyCodeObject *co;
    lower_insn *insns;
    int n;
    int *index;         // per code unit, the instruction that covers it
    int nwords;
    int nfb;            // feedback entries, entry 0 included
    int lines_marked;   // mark_line_starts() has run
    int nabsorbed;      // loads absorbed into their consumers
    // Per code unit not decoded yet, what jumps and exception handlers
    // bring to the instruction that starts there:
    // (depth + 1) << 1 | block_start, or 0.
    uint32_t *pending;
    int rescan;         // a backward jump reaches code that scan() did not
    int resolved;       // jump targets are instruction indices
} lowering;

static const uint8_t fb_kind[256] = {
#define FB_KIND(OP, TYPE, KIND) [OP] = KIND,
    _PyFb_FAMILIES(FB_KIND)
    _PyFb_BRANCHES(FB_KIND)
#undef FB_KIND
};

static int
malformed(PyCodeObject *co, int offset, const char *what)
{
    PyErr_Format(PyExc_SystemError,
                 "cannot run code object %R: %s at offset %d",
                 co, what, offset * (int)sizeof(_Py_CODEUNIT));
    return -1;
}

static inline _Py_CODEUNIT
base_unit(PyCodeObject *co, int i)
{
    _Py_CODEUNIT unit = _PyCode_CODE(co)[i];
    if (unit.op.code >= MIN_INSTRUMENTED_OPCODE) {
        unit = _Py_GetBaseCodeUnit(co, i);
    }
    return unit;
}

/* The offset that the instruction at index i jumps to, which may be out of
 * range, or INT_MIN if it does not jump */
static int
jump_offset(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    int next = in->anchor + 1 + _PyOpcode_Caches[in->opcode];
    switch (in->opcode) {
        case JUMP_FORWARD:
        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_TRUE:
        case POP_JUMP_IF_NONE:
        case POP_JUMP_IF_NOT_NONE:
        case FOR_ITER:
        case SEND:
            return next + in->oparg;
        case JUMP_BACKWARD:
        case JUMP_BACKWARD_NO_INTERRUPT:
        case END_ASYNC_FOR:
            return next - in->oparg;
        default:
            assert(!OPCODE_HAS_JUMP(in->opcode));
            return INT_MIN;
    }
}

/* Record that an edge with the given depth, or -1 for none, comes in at
 * offset, which scan() has not reached yet. */
static int
pend(lowering *L, int offset, int depth)
{
    uint32_t p = L->pending[offset] | 1;
    if (depth >= 0) {
        if (depth > L->co->co_stacksize) {
            return malformed(L->co, offset, "stack overflow");
        }
        uint32_t d = (uint32_t)(depth + 1) << 1;
        if ((p >> 1) != 0 && (p & ~1u) != d) {
            return malformed(L->co, offset, "inconsistent stack depth");
        }
        p |= d;
    }
    L->pending[offset] = p;
    return 0;
}

/* The depths at which the exception handlers start, before scan() */
static int
pend_handlers(lowering *L)
{
    PyObject *table = L->co->co_exceptiontable;
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(table);
    unsigned char *end = p + PyBytes_GET_SIZE(table);
    int size = (int)Py_SIZE(L->co);
    while (p < end) {
        int start, length, handler, depth_lasti;
        p = parse_varint(p, &start);
        p = parse_varint(p, &length);
        p = parse_varint(p, &handler);
        p = parse_varint(p, &depth_lasti);
        if (handler < 0 || handler >= size) {
            return malformed(L->co, handler,
                             "exception handler inside an instruction");
        }
        int depth = (depth_lasti >> 1) + (depth_lasti & 1) + 1;
        if (pend(L, handler, depth) < 0) {
            return -1;
        }
    }
    return 0;
}

/* After scan(): the handlers start instructions, and no load is absorbed
 * across the edge of a range either, so that the unwinding finds the range
 * of an absorbed load at its consumer. */
static int
check_handlers(lowering *L)
{
    PyObject *table = L->co->co_exceptiontable;
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(table);
    unsigned char *end = p + PyBytes_GET_SIZE(table);
    int size = (int)Py_SIZE(L->co);
    while (p < end) {
        int start, length, handler, depth_lasti;
        p = parse_varint(p, &start);
        p = parse_varint(p, &length);
        p = parse_varint(p, &handler);
        p = parse_varint(p, &depth_lasti);
        if (L->insns[L->index[handler]].start != handler) {
            return malformed(L->co, handler,
                             "exception handler inside an instruction");
        }
        if (start >= 0 && start < size) {
            L->insns[L->index[start]].block_start = 1;
        }
        if (start >= 0 && length > 0 && length < size - start) {
            L->insns[L->index[start + length]].block_start = 1;
        }
    }
    return 0;
}

static int choose_handler(PyCodeObject *co, lower_insn *in, int inline_values);
/* Decode the instructions, in one forward pass that also follows the stack
 * depth, marks where jumps land, chooses the handlers and places the
 * feedback entries.  Edges that come in ahead wait in
 * L->pending.  A backward jump into code that the pass found unreachable
 * sets L->rescan, and compute_depths() starts over. */
static int
scan(lowering *L)
{
    PyCodeObject *co = L->co;
    int size = (int)Py_SIZE(co);
#ifdef Py_STACKREF_INLINE_VALUES
    int inline_values = _PyInterpreterState_GET()->opt_config.tagged_values_enabled;
#else
    int inline_values = 0;
#endif
    int n = 0;
    int depth = 0;      // the depth that falls through, or -1
    L->nfb = 1;         // entry 0 is shared
    for (int i = 0; i < size; n++) {
        // L->insns starts zeroed
        lower_insn *in = &L->insns[n];
        in->start = i;
        in->target = -1;
        in->fb = -1;
        uint32_t pending = L->pending[i];
        int oparg = 0;
        _Py_CODEUNIT unit = base_unit(co, i);
        while (unit.op.code == EXTENDED_ARG && i + 1 < size) {
            oparg = (oparg << 8) | unit.op.arg;
            L->index[i++] = n;
            unit = base_unit(co, i);
        }
        oparg = (oparg << 8) | unit.op.arg;
        int opcode = unit.op.code;
        if (!IS_VALID_OPCODE(opcode)) {
            PyErr_Format(PyExc_SystemError,
                         "cannot run code object %R: unknown opcode %d "
                         "at offset %d", co, opcode, i * (int)sizeof(_Py_CODEUNIT));
            return -1;
        }
        int popped = _PyIR_StackEffects[opcode][0];
        int pushed = _PyIR_StackEffects[opcode][1];
        if (popped < 0) {
            popped = _PyOpcode_num_popped(opcode, oparg);
            pushed = _PyOpcode_num_pushed(opcode, oparg);
        }
        assert(popped == _PyOpcode_num_popped(opcode, oparg));
        assert(pushed == _PyOpcode_num_pushed(opcode, oparg));
        if (_PyOpcode_Deopt[opcode] != opcode || popped < 0 || pushed < 0) {
            return malformed(co, i, "invalid instruction");
        }
        in->popped = popped;
        in->pushed = pushed;
        in->anchor = i;
        in->opcode = opcode;
        in->oparg = oparg;
        in->top = _PyCode_CODE(co)[i].op.code;
        int length = 1 + _PyOpcode_Caches[opcode];
        if (i + length > size) {
            return malformed(co, i, "truncated instruction");
        }
        for (int k = 0; k < length; k++) {
            L->index[i + k] = n;
        }
        i += length;

        in->block_start = pending & 1;
        if (pending >> 1) {
            int d = (int)(pending >> 1) - 1;
            if (depth >= 0 && depth != d) {
                return malformed(co, in->anchor, "inconsistent stack depth");
            }
            depth = d;
        }
        in->depth = depth;
        if (depth >= 0) {
            if (depth > co->co_stacksize) {
                return malformed(co, in->anchor, "stack overflow");
            }
            if (depth < popped) {
                return malformed(co, in->anchor, "stack underflow");
            }
            depth += pushed - popped;
        }

        in->handler = choose_handler(co, in, inline_values);
        if (in->handler < 0) {
            return malformed(co, in->anchor, "invalid oparg");
        }
        const _PyIRFormat *fmt = _PyIR_FormatOf(in->handler);
        in->nwords = 1;
        if (fmt->fb != _PyIR_LOC_NONE) {
            assert(fb_kind[opcode] != FB_COUNTER);
            // Sites past the last index share entry 0
            in->fb = L->nfb <= IR_FIELD_MAX ? L->nfb++ : 0;
        }

        int offset = OPCODE_HAS_JUMP(opcode) ? jump_offset(L, n) : INT_MIN;
        if (offset != INT_MIN) {
            if (offset < 0 || offset >= size) {
                return malformed(co, in->anchor, "jump into an instruction");
            }
            // An offset until resolve_target() runs
            in->target = offset;
            // END_ASYNC_FOR names END_SEND but does not jump there
            if (opcode != END_ASYNC_FOR && offset >= i) {
                if (pend(L, offset, depth) < 0) {
                    return -1;
                }
            }
            else if (opcode != END_ASYNC_FOR) {
                lower_insn *target = &L->insns[L->index[offset]];
                if (target->start != offset) {
                    return malformed(co, in->anchor, "jump into an instruction");
                }
                target->block_start = 1;
                if (depth >= 0 && target->depth < 0) {
                    L->rescan = 1;
                }
                else if (depth >= 0 && target->depth != depth) {
                    return malformed(co, target->anchor,
                                     "inconsistent stack depth");
                }
            }
        }
        if (IS_SCOPE_EXIT_OPCODE(opcode) || IS_UNCONDITIONAL_JUMP_OPCODE(opcode)) {
            depth = -1;
        }
    }
    L->n = n;
    return 0;
}

/* Turn the target offset of instruction i into an instruction index */
static int
resolve_target(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    int target = L->index[in->target];
    if (L->insns[target].start != in->target) {
        return malformed(L->co, in->anchor, "jump into an instruction");
    }
    in->target = target;
    return 0;
}

static int
push_depth(lowering *L, int *stack, int *sp, int i, int depth)
{
    lower_insn *in = &L->insns[i];
    if (depth > L->co->co_stacksize) {
        return malformed(L->co, in->anchor, "stack overflow");
    }
    if (in->depth < 0) {
        in->depth = depth;
        stack[(*sp)++] = i;
    }
    else if (in->depth != depth) {
        return malformed(L->co, in->anchor, "inconsistent stack depth");
    }
    return 0;
}

/* The depths by a worklist, for code that scan() cannot follow in one pass:
 * the depth of every instruction is unknown on entry, and the jump targets
 * are indices. */
static int
compute_depths(lowering *L)
{
    PyCodeObject *co = L->co;
    int *stack = PyMem_Malloc(L->n * sizeof(int));
    if (stack == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    int sp = 0;
    int err = 0;
    if (L->n > 0) {
        err = push_depth(L, stack, &sp, 0, 0);
    }
    PyObject *table = co->co_exceptiontable;
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(table);
    unsigned char *end = p + PyBytes_GET_SIZE(table);
    while (err == 0 && p < end) {
        int start, length, handler, depth_lasti;
        p = parse_varint(p, &start);
        p = parse_varint(p, &length);
        p = parse_varint(p, &handler);
        p = parse_varint(p, &depth_lasti);
        int depth = (depth_lasti >> 1) + (depth_lasti & 1) + 1;
        err = push_depth(L, stack, &sp, L->index[handler], depth);
    }
    while (err == 0 && sp > 0) {
        int i = stack[--sp];
        lower_insn *in = &L->insns[i];
        if (in->depth < in->popped) {
            err = malformed(co, in->anchor, "stack underflow");
            break;
        }
        int depth = in->depth - in->popped + in->pushed;
        if (in->target >= 0 && in->opcode != END_ASYNC_FOR) {
            err = push_depth(L, stack, &sp, in->target, depth);
            if (err) {
                break;
            }
        }
        if (!IS_SCOPE_EXIT_OPCODE(in->opcode)
            && !IS_UNCONDITIONAL_JUMP_OPCODE(in->opcode)
            && i + 1 < L->n)
        {
            err = push_depth(L, stack, &sp, i + 1, depth);
        }
    }
    PyMem_Free(stack);
    return err;
}

/* Mark the first instruction of each line, before which a LINE event may
 * fire.  This reads the line table as _PyLineTable_NextAddressRange() does,
 * but in one loop, since it runs for every code object that absorbs. */
static void
mark_line_starts(lowering *L)
{
    L->lines_marked = 1;
    PyCodeObject *co = L->co;
    const uint8_t *p = (const uint8_t *)PyBytes_AS_STRING(co->co_linetable);
    const uint8_t *limit = p + PyBytes_GET_SIZE(co->co_linetable);
    int size = (int)Py_SIZE(co);
    int line = co->co_firstlineno;
    int current = -1;   // the line of the previous entry, or -1 for none
    int offset = 0;
    while (p < limit && offset < size) {
        uint8_t first = *p++;
        assert(first & 128);
        int code = (first >> 3) & 15;
        int start = offset;
        offset += (first & 7) + 1;
        if (code == PY_CODE_LOCATION_INFO_NONE) {
            current = -1;
        }
        else {
            if (code == PY_CODE_LOCATION_INFO_NO_COLUMNS
                || code == PY_CODE_LOCATION_INFO_LONG)
            {
                // A signed varint in 6-bit chunks
                unsigned int read = *p;
                unsigned int value = read & 63;
                unsigned int shift = 0;
                while (read & 64) {
                    read = *++p;
                    shift += 6;
                    value |= (read & 63) << shift;
                }
                line += (value & 1) ? -(int)(value >> 1) : (int)(value >> 1);
            }
            else if (code >= PY_CODE_LOCATION_INFO_ONE_LINE0) {
                line += code - PY_CODE_LOCATION_INFO_ONE_LINE0;
            }
            if (line != current && line >= 0) {
                int i = L->index[start];
                L->insns[i].block_start = 1;
                if (L->insns[i].start != start && i + 1 < L->n) {
                    // A line that starts inside an instruction starts with
                    // the next one
                    L->insns[i + 1].block_start = 1;
                }
            }
            current = line;
        }
        while (p < limit && !(*p & 128)) {
            p++;
        }
    }
}

/* The handler for an instruction before instrumentation and absorption:
 * its opcode, a form that pushes its constant inline, or for BINARY_OP the
 * handler of its operation.  -1 for an oparg out of range. */
static int
choose_handler(PyCodeObject *co, lower_insn *in, int inline_values)
{
    if (in->opcode == BINARY_OP) {
        if (in->oparg > NB_OPARG_LAST) {
            return -1;
        }
        return _PyIR_BINARY_OP_FIRST + in->oparg;
    }
#ifdef Py_STACKREF_INLINE_VALUES
    if (inline_values) {
        if (in->opcode == LOAD_SMALL_INT) {
            return LOAD_SMALL_INT_INLINE;
        }
        if (in->opcode == LOAD_CONST) {
            PyObject *value = PyTuple_GET_ITEM(co->co_consts, in->oparg);
            if (_PyLong_CheckExactAndCompact(value)) {
                return LOAD_CONST_INLINE_INT;
            }
            if (PyFloat_CheckExact(value) && !isnan(PyFloat_AS_DOUBLE(value))) {
                return LOAD_CONST_INLINE_FLOAT;
            }
        }
    }
#else
    (void)co;
    (void)inline_values;
#endif
    return in->opcode;
}

/* Absorb the borrowed loads that push the top inputs of instruction i,
 * those that its format lets name a local.  Only NOPs and other absorbed
 * loads may lie between, so the instructions that run in place of the loads
 * do not use stack_pointer, and no line or block may start after the load,
 * so nothing can write the local before i reads it.  Returns the index of
 * the first load absorbed, or -1. */
static int
absorb_loads(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    if (i == 0 || (L->insns[i - 1].opcode != LOAD_FAST_BORROW
                   && L->insns[i - 1].opcode != LOAD_FAST_BORROW_LOAD_FAST_BORROW
                   && L->insns[i - 1].opcode != NOP))
    {
        return -1;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(in->handler);
    if (in->depth < 0 || (fmt->flags & _PyIR_WINDOW) || fmt->borrow_mask == 0) {
        return -1;
    }
    int base = in->depth - fmt->ninputs;
    if (!L->lines_marked) {
        // Only now, since most code objects that run once, such as module
        // bodies, absorb nothing.
        mark_line_starts(L);
    }
    int first = -1;
    for (int j = i - 1; j >= 0; j--) {
        lower_insn *load = &L->insns[j];
        if (L->insns[j + 1].block_start || load->depth < base) {
            return first;
        }
        if (load->opcode == NOP) {
            continue;
        }
        int locals[2];
        if (load->opcode == LOAD_FAST_BORROW) {
            locals[0] = load->oparg;
        }
        else if (load->opcode == LOAD_FAST_BORROW_LOAD_FAST_BORROW) {
            locals[0] = load->oparg >> 4;
            locals[1] = load->oparg & 15;
        }
        else {
            return first;
        }
        int pushed = load->pushed;
        for (int p = 0; p < pushed; p++) {
            int k = load->depth + p - base;
            if (k >= fmt->ninputs || !((fmt->borrow_mask >> k) & 1)
                || locals[p] >= L->co->co_nlocalsplus)
            {
                return first;
            }
        }
        for (int p = 0; p < pushed; p++) {
            int k = load->depth + p - base;
            in->borrow[k] = locals[p] + 1;
        }
        load->absorbed = 1;
        L->nabsorbed++;
        first = j;
    }
    return first;
}

/* Whether instrumentation is active for co, so that the lowering keeps a
 * word per instruction for its INSTRUCTION events. */
static int
instrumentation_active(PyCodeObject *co)
{
    if (co->_co_monitoring != NULL) {
        return 1;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        if (interp->monitors.tools[e]) {
            return 1;
        }
    }
    return 0;
}

static inline int
insn_nwords(const lower_insn *in)
{
    return in->absorbed ? 0 : in->nwords;
}

/* The words of a relaxed FOR_ITER and of a relaxed SEND, its own included.
 * A relaxed FOR_ITER at word w exits by distance 1, so that
 *
 *     w + 1   JUMP_FORWARD to w + 4, the body
 *     w + 2   JUMP_FORWARD to END_FOR, where a generator returns
 *     w + 3   JUMP_FORWARD past END_FOR, where an exhausted iterator goes
 *
 * and a relaxed SEND at w has w + 1, to w + 3, and w + 2, to END_SEND. */
#define RELAXED_FOR_ITER_WORDS 4
#define RELAXED_SEND_WORDS 3

/* The farthest exit of FOR_ITER and SEND that the lowering does not relax.
 * Tests lower it to relax every loop. */
int _PyIR_MaxForward = IR_MAX_FORWARD;

/* The oparg that the handlers of instruction i, of format fmt, read: the
 * word index of the target of a jump, the distance from the next word to
 * the exit of FOR_ITER and SEND, otherwise the oparg of the bytecode. */
static inline uint32_t
handler_oparg(lowering *L, int i, const _PyIRFormat *fmt)
{
    lower_insn *in = &L->insns[i];
    if (fmt->flags & _PyIR_JUMPS) {
        return (uint32_t)L->insns[in->target].word;
    }
    if (fmt->flags & _PyIR_FORWARD) {
        if (in->nwords > 1) {
            return 1;
        }
        return (uint32_t)(L->insns[in->target].word - (in->word + 1));
    }
    if (in->opcode == GET_ITER && in->oparg) {
        // The compiler cannot know whether types.coroutine() will be called
        return (L->co->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))
            ? GET_ITER_YIELD_FROM_NO_CHECK : GET_ITER_YIELD_FROM_CORO_CHECK;
    }
    return (uint32_t)in->oparg;
}

/* Place the words, relaxing each FOR_ITER and SEND whose exit is too far,
 * until none is: relaxing only moves exits further away. */
static void
layout(lowering *L)
{
    for (;;) {
        int word = 0;
        for (int i = 0; i < L->n; i++) {
            L->insns[i].word = word;
            word += insn_nwords(&L->insns[i]);
        }
        L->nwords = word;
        if (word <= _PyIR_MaxForward) {
            return;
        }
        int changed = 0;
        for (int i = 0; i < L->n; i++) {
            lower_insn *in = &L->insns[i];
            if ((in->opcode != FOR_ITER && in->opcode != SEND)
                || in->nwords > 1 || in->absorbed)
            {
                continue;
            }
            int distance = L->insns[in->target].word - (in->word + 1);
            if (distance > _PyIR_MaxForward) {
                in->nwords = in->opcode == FOR_ITER
                    ? RELAXED_FOR_ITER_WORDS : RELAXED_SEND_WORDS;
                changed = 1;
            }
        }
        if (!changed) {
            return;
        }
    }
}

static int
too_large(PyCodeObject *co, int offset, const char *what, uint32_t value)
{
    PyErr_Format(PyExc_SystemError,
                 "cannot run code object %R: %s %u at offset %d does not "
                 "fit in the register IR", co, what, value,
                 offset * (int)sizeof(_Py_CODEUNIT));
    return -1;
}

/* The word of instruction i with the given handler.  Fields that do not
 * fit fail for reachable code; unreachable code never runs, and its
 * fields become 0. */
static int
encode(lowering *L, int i, int handler, _PyIRWord *word)
{
    lower_insn *in = &L->insns[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(in->handler);
    int reachable = in->depth >= 0;
    int depth = reachable ? in->depth : Py_MAX(in->popped, fmt->ninputs);
    int top = L->co->co_nlocalsplus + depth;
    int base = top - fmt->ninputs;
    uint32_t oparg = handler_oparg(L, i, fmt);
    // Every role's value, then a pick per field.  DST is meaningless when
    // fmt->dst is _PyIR_NO_DST.  Only formats with a dst borrow, and they
    // have at most three inputs; others, such as STORE_SLICE, have four.
    uint32_t values[_PyIR_ROLE_IN0 + 4] = {
        [_PyIR_ROLE_NONE] = 0,
        [_PyIR_ROLE_DST] = (uint32_t)(fmt->flags & _PyIR_WINDOW
                                      ? top - in->popped + fmt->dst
                                      : top + fmt->dst),
        [_PyIR_ROLE_BASE] = (uint32_t)(top - in->popped),
        [_PyIR_ROLE_FB] = in->fb > 0 ? (uint32_t)in->fb : 0,
        [_PyIR_ROLE_IMM] = oparg,
        [_PyIR_ROLE_IN0] = in->borrow[0] ? (uint32_t)in->borrow[0] - 1 : (uint32_t)base,
        [_PyIR_ROLE_IN0 + 1] = in->borrow[1] ? (uint32_t)in->borrow[1] - 1 : (uint32_t)base + 1,
        [_PyIR_ROLE_IN0 + 2] = in->borrow[2] ? (uint32_t)in->borrow[2] - 1 : (uint32_t)base + 2,
        [_PyIR_ROLE_IN0 + 3] = (uint32_t)base + 3,
    };
    uint32_t fields[4];
    for (int r = 0; r < 4; r++) {
        int role = fmt->roles[r];
        assert(role < _PyIR_ROLE_IN0 + 4);
        assert(role < _PyIR_ROLE_IN0 || role - _PyIR_ROLE_IN0 < fmt->ninputs);
        fields[r] = values[role];
        if (role == _PyIR_ROLE_IMM) {
            continue;
        }
        if (fields[r] > IR_FIELD_MAX) {
            // Only unreachable code names slots past the frame, since
            // lower() checks the slot count.
            assert(!reachable);
            fields[r] = 0;
        }
    }
    if (fmt->imm == _PyIR_LOC_CD) {
        if (oparg > IR_CD_MAX) {
            if (reachable) {
                return too_large(L->co, in->anchor, "oparg", oparg);
            }
            oparg = 0;
        }
        fields[2] = oparg & IR_FIELD_MAX;
        fields[3] = oparg >> 12;
    }
    else if (fmt->imm >= _PyIR_LOC_A && fmt->imm <= _PyIR_LOC_D) {
        int r = fmt->imm - _PyIR_LOC_A;
        if (oparg > IR_FIELD_MAX) {
            if (reachable) {
                return too_large(L->co, in->anchor, "oparg", oparg);
            }
            fields[r] = 0;
        }
    }
    *word = IR_WORD(handler, fields[0], fields[1], fields[2], fields[3]);
    return 0;
}

static inline void
set_pc(_PyIRBlock *block, int w, int start, int anchor)
{
    if (block->flags & _PyIR_PCMAP_WIDE) {
        ((_PyIRPc32 *)block->pcmap)[w] = (_PyIRPc32){(uint32_t)start, (uint32_t)anchor};
    }
    else {
        ((_PyIRPc16 *)block->pcmap)[w] = (_PyIRPc16){(uint16_t)start, (uint16_t)anchor};
    }
}

static inline _PyIRWord
jump_word(int target)
{
    return IR_WORD(JUMP_FORWARD, 0, 0, target & IR_FIELD_MAX, target >> 12);
}

static _PyIRBlock *
emit(lowering *L)
{
    PyCodeObject *co = L->co;
    int wide = Py_SIZE(co) > UINT16_MAX;
    size_t words_size = (size_t)L->nwords * sizeof(_PyIRWord);
    // Entry 0 exists only with others
    size_t fb_size = L->nfb > 1
        ? sizeof(_PyFbBlock) + (size_t)L->nfb * FB_STRIDE : 0;
    size_t pc_size = (size_t)L->nwords * (wide ? sizeof(_PyIRPc32) : sizeof(_PyIRPc16));
    _PyIRBlock *block = PyMem_Malloc(sizeof(_PyIRBlock) + words_size
                                     + fb_size + pc_size);
    if (block == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyIRWord *words = (_PyIRWord *)(block + 1);
    *block = (_PyIRBlock){
        .words = words,
        .nwords = L->nwords,
        .flags = (wide ? _PyIR_PCMAP_WIDE : 0) | (L->nabsorbed ? _PyIR_FUSED : 0),
        .pcmap = (char *)words + words_size + fb_size,
    };
    if (fb_size) {
        block->feedback = (_PyFbBlock *)(words + L->nwords);
        // Entries that this pass does not set start zeroed
        memset(block->feedback, 0, fb_size);
        block->feedback->size = (uint32_t)fb_size;
        block->entries = (char *)(block->feedback + 1);
        // The shared entry never specialises: its counter never triggers
        _PyFbHeader *shared = _PyIR_FbEntry(block->entries, 0);
        shared->kind = FB_COUNTER;
        shared->state = FB_OFF;
        shared->counter = initial_unreachable_backoff_counter();
    }
    int instrumented = co->_co_monitoring != NULL;
    if (instrumented) {
        block->real = PyMem_Calloc(L->nwords, sizeof(uint16_t));
        if (block->real == NULL) {
            PyMem_Free(block);
            PyErr_NoMemory();
            return NULL;
        }
    }
#ifdef Py_GIL_DISABLED
    int enable_counters = 0;
#else
    int enable_counters = ENABLE_SPECIALIZATION
        && _PyInterpreterState_GET()->opt_config.specialization_enabled;
#endif
    _Py_BackoffCounter counter = enable_counters
        ? adaptive_counter_warmup() : initial_unreachable_backoff_counter();
    // The words after absorbed loads cover their offsets
    int pc_start = -1;
    for (int i = 0; i < L->n; i++) {
        lower_insn *in = &L->insns[i];
        if (pc_start < 0) {
            pc_start = in->start;
        }
        if (in->absorbed) {
            continue;
        }
        in->pc_start = pc_start;
        pc_start = -1;
        int w = in->word;
        // co_code_adaptive holds opcodes that instrumentation put there,
        // in place of the lowering's choice, which real[] keeps.
        int handler = in->top == in->opcode ? in->handler : in->top;
        if (encode(L, i, handler, &words[w]) < 0) {
            PyMem_Free(block->real);
            PyMem_Free(block);
            return NULL;
        }
        if (instrumented) {
            block->real[w] = (uint16_t)in->handler;
        }
        if (in->fb > 0) {
            _PyFbHeader *h = _PyIR_FbEntry(block->entries, (uint32_t)in->fb);
            h->kind = fb_kind[in->opcode];
            if (h->kind == FB_BRANCH) {
                h->history = 0x5555;    // alternating taken and not taken
            }
            else {
                h->counter = counter;
                h->arg = handler_oparg(L, i, _PyIR_FormatOf(in->handler));
            }
        }
        set_pc(block, w, in->pc_start, in->anchor);
        if (in->nwords > 1) {
            int exit = L->insns[in->target].word;
            if (in->opcode == FOR_ITER) {
                words[w + 1] = jump_word(w + RELAXED_FOR_ITER_WORDS);
                words[w + 2] = jump_word(exit);
                words[w + 3] = jump_word(exit + 1);
            }
            else {
                assert(in->opcode == SEND);
                words[w + 1] = jump_word(w + RELAXED_SEND_WORDS);
                words[w + 2] = jump_word(exit);
            }
            for (int k = 1; k < in->nwords; k++) {
                set_pc(block, w + k, in->pc_start, in->anchor);
                if (instrumented) {
                    block->real[w + k] = JUMP_FORWARD;
                }
            }
        }
    }
    return block;
}

#ifdef Py_DEBUG
/* The pcmap invariants that the accessors rely on */
static void
check_block(lowering *L, _PyIRBlock *block)
{
    for (int i = 0; i < L->n; i++) {
        lower_insn *in = &L->insns[i];
        _PyIRWord *instr = block->words + in->word;
        assert(_PyIR_InstrAt(block, in->start) == instr);
        assert(_PyIR_InstrAt(block, in->anchor) == instr);
        if (in->absorbed) {
            // Its consumer, or a NOP before it, covers its offsets
            continue;
        }
        assert(_PyIR_Anchor(block, instr) == in->anchor);
        assert(_PyIR_Start(block, instr) == in->pc_start);
        assert(!_PyIR_IsTrampoline(block, instr));
        for (int k = 1; k < in->nwords; k++) {
            assert(_PyIR_IsTrampoline(block, instr + k));
        }
        int handler = in->handler;
        const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
        if (fmt->imm != _PyIR_LOC_NONE && in->depth >= 0) {
            int expected = (fmt->imm == _PyIR_LOC_FBARG && in->fb == 0)
                ? in->oparg : (int)handler_oparg(L, i, fmt);
            assert(_PyIR_InstrOparg(L->co, block, instr, handler) == expected);
        }
        _PyFbHeader *fb = _PyIR_InstrFeedback(block, instr, handler);
        assert((fb != NULL) == (in->fb >= 0));
        assert(fb == NULL || in->fb == 0 || fb->kind == fb_kind[in->opcode]);
    }
}
#endif

/* Lower co, absorbing loads unless one_to_one is set */
static _PyIRBlock *
lower(PyCodeObject *co, int one_to_one)
{
    if (co->co_nlocalsplus + co->co_stacksize > IR_MAX_SLOTS) {
        PyErr_Format(PyExc_SystemError,
                     "cannot run code object %R: it needs %d slots for its "
                     "locals, cells, free variables and stack, and the "
                     "register IR has at most %d",
                     co, co->co_nlocalsplus + co->co_stacksize, IR_MAX_SLOTS);
        return NULL;
    }
    lowering L = {.co = co};
    _PyIRBlock *block = NULL;
    Py_ssize_t size = Py_MAX(Py_SIZE(co), 1);
    L.insns = PyMem_Calloc(size, sizeof(lower_insn));
    L.index = PyMem_Calloc(size, sizeof(int) + sizeof(uint32_t));
    if (L.insns == NULL || L.index == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    L.pending = (uint32_t *)(L.index + size);
    if (pend_handlers(&L) < 0 || scan(&L) < 0 || check_handlers(&L) < 0) {
        goto done;
    }
    if (L.rescan) {
        for (int i = 0; i < L.n; i++) {
            L.insns[i].depth = -1;
            if (L.insns[i].target >= 0 && resolve_target(&L, i) < 0) {
                goto done;
            }
        }
        L.resolved = 1;
        if (compute_depths(&L) < 0) {
            goto done;
        }
    }
    int absorb = !one_to_one && !instrumentation_active(co);
    for (int i = 0; i < L.n; i++) {
        if (!L.resolved && L.insns[i].target >= 0 && resolve_target(&L, i) < 0) {
            goto done;
        }
        if (absorb) {
            absorb_loads(&L, i);
        }
    }
    layout(&L);
    if (L.nwords > IR_CD_MAX) {
        // Jump targets would not fit
        too_large(co, 0, "word count", (uint32_t)L.nwords);
        goto done;
    }
    block = emit(&L);
#ifdef Py_DEBUG
    if (block != NULL) {
        check_block(&L, block);
    }
#endif
done:
    PyMem_Free(L.insns);
    PyMem_Free(L.index);
    return block;
}

_PyIRBlock *
_PyIR_Lower(PyCodeObject *co)
{
    _PyIRBlock *block = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_ir);
    if (block != &_PyIR_LowerStub) {
        return block;
    }
    Py_BEGIN_CRITICAL_SECTION(co);
    block = co->_co_ir;
    if (block == &_PyIR_LowerStub) {
        block = lower(co, 0);
        if (block != NULL) {
            FT_ATOMIC_STORE_PTR_RELEASE(co->_co_ir, block);
        }
    }
    Py_END_CRITICAL_SECTION();
    return block;
}

int
_PyIR_LowerFrame(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    (void)tstate;
    assert(_PyFrame_IsAtStub(frame));
    PyCodeObject *co = _PyFrame_GetCode(frame);
    int resumed = frame->instr_ptr == _PyIR_STUB_RESUMED;
    _PyIRBlock *block = _PyIR_Lower(co);
    if (block == NULL) {
        return -1;
    }
    _PyIRWord *instr = block->words;
    if (resumed && co->_co_firsttraceable < Py_SIZE(co)) {
        // Past the first RESUME, which has one word
        instr = _PyIR_InstrAt(block, co->_co_firsttraceable) + 1;
    }
    frame->instr_ptr = instr;
    _PyFrame_SetIRBlock(frame, block);
    return 0;
}

void
_PyIR_Clear(PyCodeObject *co)
{
    _PyIRBlock *block = co->_co_ir;
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock) {
        return;
    }
    if (block->feedback != NULL) {
        _PyFb_Clear(block->feedback);
    }
    if (block->fused != NULL) {
        _PyIRBlock *fused = block->fused;
        if (fused->feedback != NULL) {
            _PyFb_Clear(fused->feedback);
        }
        PyMem_Free(fused->real);
        PyMem_Free(fused);
    }
    PyMem_Free(block->real);
    PyMem_Free(block);
    co->_co_ir = &_PyIR_LowerStub;
}

static size_t
block_size(const _PyIRBlock *block)
{
    size_t pc_entry = (block->flags & _PyIR_PCMAP_WIDE)
        ? sizeof(_PyIRPc32) : sizeof(_PyIRPc16);
    size_t res = sizeof(_PyIRBlock)
        + block->nwords * (sizeof(_PyIRWord) + pc_entry);
    if (block->real != NULL) {
        res += block->nwords * sizeof(uint16_t);
    }
    _PyFbBlock *feedback = block->feedback;
    if (feedback != NULL) {
        res += feedback->size;
        if (feedback->npoly) {
            // capacity: npoly rounded up to a power of 2
            size_t capacity = 1;
            while (capacity < feedback->npoly) {
                capacity *= 2;
            }
            res += capacity * sizeof(_PyFbPoly);
        }
    }
    if (block->fused != NULL) {
        res += block_size(block->fused);
    }
    return res;
}

size_t
_PyIR_SizeOf(PyCodeObject *co)
{
    _PyIRBlock *block = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_ir);
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock) {
        return 0;
    }
    return block_size(block);
}


/* Instrumentation */

/* Run BODY with w set to the word of each instruction of block, skipping
 * the words that relax() adds */
#define FOR_EACH_INSTRUCTION(block, w, BODY) \
    for (Py_ssize_t w = 0; w < (Py_ssize_t)(block)->nwords; w++) { \
        if (is_trampoline((block), w)) { \
            continue; \
        } \
        BODY \
    }

/* Replace the fused block of co with one that has a word per instruction.
 * Frames may still be in the fused block, on thread stacks or suspended, so
 * it stays allocated, and its words run IR_REMAP, which moves a frame over
 * at its next dispatch.  A frame dispatches between instructions, where both
 * blocks hold the same values in the same slots: the canonical slot of an
 * absorbed load is only written by the instruction that absorbs it.  The
 * words that relax() added keep jumping within the fused block, to words
 * that run IR_REMAP. */
static int
lower_one_to_one(PyCodeObject *co)
{
    _PyIRBlock *fused = co->_co_ir;
    assert(fused->fused == NULL);
    _PyIRBlock *block = lower(co, 1);
    if (block == NULL) {
        return -1;
    }
    block->fused = fused;
    FOR_EACH_INSTRUCTION(fused, w, {
        _PyIR_SetHandler(&fused->words[w], IR_REMAP);
    })
    FT_ATOMIC_STORE_PTR_RELEASE(co->_co_ir, block);
    return 0;
}

_PyIRWord *
_PyIR_Remap(PyCodeObject *co, const _PyIRWord *instr)
{
    _PyIRBlock *block = co->_co_ir;
    assert(block->fused != NULL);
    return _PyIR_InstrAt(block, _PyIR_Start(block->fused, instr));
}

int
_PyIR_PrepareInstrumentation(PyCodeObject *co, int instructions)
{
    _PyIRBlock *block = co->_co_ir;
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock) {
        return 0;
    }
    if (instructions && (block->flags & _PyIR_FUSED)) {
        // The new block has real[], since co->_co_monitoring is set
        assert(co->_co_monitoring != NULL);
        return lower_one_to_one(co);
    }
    if (block->real != NULL) {
        return 0;
    }
    uint16_t *real = PyMem_Calloc(block->nwords, sizeof(uint16_t));
    if (real == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (Py_ssize_t w = 0; w < (Py_ssize_t)block->nwords; w++) {
        // Not instrumented yet, so the handler is the lowering's choice
        // or a specialisation of it.
        int handler = IR_HANDLER(block->words[w]);
        assert(!(_PyIR_FormatOf(handler)->flags & _PyIR_TRANSPARENT));
        if (is_trampoline(block, w)) {
            real[w] = (uint16_t)handler;
            continue;
        }
        real[w] = (uint16_t)_PyIR_GenericHandler(block, block->words + w, handler);
    }
    block->real = real;
    return 0;
}

int
_PyIR_WordAnchor(PyCodeObject *co, int offset)
{
    _PyIRBlock *block = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_ir);
    if (block == &_PyIR_LowerStub) {
        return offset;
    }
    return _PyIR_Anchor(block, _PyIR_InstrAt(block, offset));
}

void
_PyIR_SyncInstrumentation(PyCodeObject *co, int offset)
{
    _PyIRBlock *block = co->_co_ir;
    if (block == &_PyIR_LowerStub) {
        // The lowering reads the instrumentation when it runs
        return;
    }
    _PyIRWord *instr = _PyIR_InstrAt(block, offset);
    if (_PyIR_Anchor(block, instr) != offset) {
        // An EXTENDED_ARG prefix or an absorbed load, which has no word of
        // its own
        return;
    }
    assert(block->real != NULL);
    int lowered = block->real[instr - block->words];
    int top = _PyCode_CODE(co)[offset].op.code;
    int handler = lowered_opcode(lowered) == top ? lowered : top;
    if (IR_HANDLER(*instr) == handler) {
        return;
    }
    _PyIR_SetHandler(instr, handler);
    if (_PyIR_FormatOf(handler)->flags & _PyIR_TRANSPARENT) {
        return;
    }
    // Restart the warmup of an instruction whose handler changes between
    // its instrumented and uninstrumented forms; the shared entry never
    // warms up.
    if (fb_index(block, instr, handler) > 0) {
        _PyFbHeader *h = _PyIR_InstrFeedback(block, instr, handler);
        if (h->kind != FB_BRANCH) {
            FT_ATOMIC_STORE_UINT16_RELAXED(h->counter.value_and_backoff,
                                           adaptive_counter_warmup().value_and_backoff);
        }
    }
}

int
_PyIR_WriteAdaptive(PyCodeObject *co, _Py_CODEUNIT *instructions,
                    void (*write_fields)(_Py_CODEUNIT *, const _PyFbHeader *))
{
    _PyIRBlock *block = _PyIR_Lower(co);
    if (block == NULL) {
        return -1;
    }
    FOR_EACH_INSTRUCTION(block, w, {
        _PyIRWord *instr = block->words + w;
        int offset = _PyIR_Anchor(block, instr);
        int handler = IR_HANDLER(*instr);
        _Py_CODEUNIT *unit = &instructions[offset];
        // Specialised or instrumented; an IR-only handler shows as the
        // instruction it stands for, and absorbed loads keep their opcode.
        unit->op.code = (uint8_t)(handler < IR_FIRST_ONLY_HANDLER
                                  ? handler : _PyIR_Formats[handler].shadow);
        int base = _Py_GetBaseCodeUnit(co, offset).op.code;
        int caches = _PyOpcode_Caches[base];
        if (caches) {
            memset(&unit[1], 0, caches * sizeof(_Py_CODEUNIT));
            // The shared entry holds nothing of the instruction
            if (fb_index(block, instr, handler) > 0) {
                _PyFbHeader *h = _PyIR_InstrFeedback(block, instr, handler);
                if (h->kind == FB_BRANCH) {
                    unit[1].cache = h->history;
                }
                else {
                    write_fields(unit, h);
                }
            }
        }
    })
    return 0;
}
