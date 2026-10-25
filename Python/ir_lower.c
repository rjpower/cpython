/* Lowering of stack bytecode to the register IR, and the accessors of IR
 * blocks.  See boil/design/01-internal-ir.md and Include/internal/pycore_ir.h.
 *
 * A code object starts at _PyIR_LowerStub.  The stub's handler, IR_LOWER,
 * calls _PyIR_LowerFrame(), which lowers co_code_adaptive into a block the
 * first time a frame of the code runs, publishes it in co->_co_ir and moves
 * the frame there.  The lowering emits one IR instruction per bytecode
 * instruction, except that
 *
 * - EXTENDED_ARG units vanish into the fields of their instruction.  An
 *   IR_EXT word precedes an instruction only when a field needs more than
 *   16 bits;
 * - inline caches vanish: feedback entries follow the words;
 * - a LOAD_FAST_BORROW (or LOAD_FAST_BORROW_LOAD_FAST_BORROW) whose values
 *   the next instruction that is not a load consumes as objects, on the same
 *   line and in the same basic block, becomes IR_ABSORBED_*, which pushes
 *   NULL.  The consumer reads the local through a field marked IR_BORROW.
 *   The word stays, so offsets, depths and instrumentation keep one IR
 *   instruction per bytecode instruction.
 *
 * Temporary d lives in slot co_nlocalsplus + d, so stack_pointer stays valid
 * and every handler keeps its stack-interpreter body.  Depths come from the
 * stack effects of the bytecode, as in calculate_stackdepth().
 *
 * The pcmap gives each word two bytecode offsets: start, the first code unit
 * of its instruction (an EXTENDED_ARG if it has one), and anchor, the
 * instruction itself.  The anchor is f_lasti and keys instrumentation, line
 * events included; the start is where jumps and exception handlers land.
 *
 * Instrumentation keeps rewriting co_code_adaptive, which the IR shadows:
 * _PyIR_SyncInstrumentation() copies each rewrite to the main word of the
 * instruction.  The handler that the lowering chose, which instrumentation
 * displaces, is kept in block->real once the code is instrumented.
 */

#define NEED_IR_METADATA            // defines _PyIR_Formats, _PyIR_OnlyNames
#include "Python.h"

#ifdef Py_IR_INTERP

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
static_assert(FB_ALIGN == sizeof(_PyIRWord),
              "a feedback distance counts words");

/* Words of the feedback header that precedes the entries */
#define FB_HEADER_WORDS ((int)(sizeof(_PyFbBlock) / sizeof(_PyIRWord)))


/* Static blocks */

static _PyIRWord lower_stub_words[2] = {
    IR_WORD(IR_LOWER, 0, 0, 0),
    IR_WORD(IR_LOWER, 0, 0, 0),
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
    IR_WORD(EXIT_INIT_CHECK, IR_SLOT_FIELD(1), 0, 0),
    IR_WORD(RETURN_VALUE, IR_SLOT_FIELD(0), 0, 0),
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
    // Back to the first word of that instruction
    int start = pc_start(block, lo);
    while (lo > 0 && pc_start(block, lo - 1) == start) {
        lo--;
    }
    return block->words + lo;
}

_PyIRWord *
_PyIR_MainWordAt(const _PyIRBlock *block, int offset)
{
    _PyIRWord *instr = _PyIR_InstrAt(block, offset);
    if (IR_HANDLER(*instr) == IR_EXT) {
        instr++;
    }
    return instr;
}

/* The IR_EXT word before the main word instr, or NULL.  Words of one
 * instruction share their pcmap entry, and IR_EXT comes first. */
static inline const _PyIRWord *
ext_word(const _PyIRBlock *block, const _PyIRWord *instr)
{
    Py_ssize_t w = word_index(block, instr);
    if (w > 0 && pc_anchor(block, w - 1) == pc_anchor(block, w)
        && pc_start(block, w - 1) == pc_start(block, w))
    {
        assert(IR_HANDLER(instr[-1]) == IR_EXT);
        return instr - 1;
    }
    return NULL;
}

static inline uint32_t
read_field(const _PyIRBlock *block, const _PyIRWord *instr, int loc)
{
    const _PyIRWord *ext;
    switch (loc) {
        case _PyIR_LOC_A:
            ext = ext_word(block, instr);
            return (ext ? IR_A(*ext) << 16 : 0) | IR_A(*instr);
        case _PyIR_LOC_B:
            ext = ext_word(block, instr);
            return (ext ? IR_B(*ext) << 16 : 0) | IR_B(*instr);
        case _PyIR_LOC_C:
            ext = ext_word(block, instr);
            return (ext ? IR_C(*ext) << 16 : 0) | IR_C(*instr);
        case _PyIR_LOC_D:
            return IR_D(instr[1]);
        case _PyIR_LOC_F:
            return IR_F(instr[1]);
        default:
            Py_UNREACHABLE();
    }
}

/* The handler whose format decodes the main word instr */
static inline int
decoding_handler(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    if (_PyIR_FormatOf(handler)->flags & _PyIR_TRANSPARENT) {
        assert(block->real != NULL);
        handler = block->real[word_index(block, instr)];
    }
    return handler;
}

_PyFbHeader *
_PyIR_InstrFeedback(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    if (block == &_PyIR_LowerStub) {
        return NULL;
    }
    handler = decoding_handler(block, instr, handler);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->fb == _PyIR_LOC_NONE) {
        return NULL;
    }
    uint32_t distance = read_field(block, instr, fmt->fb);
    assert(distance != 0);
    return (_PyFbHeader *)(instr + distance);
}

int
_PyIR_InstrOparg(const _PyIRBlock *block, const _PyIRWord *instr, int handler)
{
    if (block == &_PyIR_LowerStub) {
        return 0;
    }
    handler = decoding_handler(block, instr, handler);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    switch (fmt->imm) {
        case _PyIR_LOC_NONE:
            return 0;
        case _PyIR_LOC_FBARG:
            return (int)_PyIR_InstrFeedback(block, instr, handler)->arg;
        default:
            return (int)read_field(block, instr, fmt->imm);
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
    int depth;          // stack depth before, or -1 if unreachable
    int target;         // the instruction a jump goes to, or -1
    int handler;        // the handler that the lowering chooses
    int ext;            // 1 if an IR_EXT word precedes the main word
    int word;           // index of the first word
    int fb;             // offset of the feedback entry in words from the
                        // first entry, or -1
    uint32_t borrow[3]; // per input: an absorbed local's slot field, or 0
    uint8_t block_start;// an event or a jump may come in before it
} lower_insn;

typedef struct {
    PyCodeObject *co;
    lower_insn *insns;
    int n;
    int *index;         // per code unit, the instruction that covers it
    int nwords;
    int fb_words;       // words of all entries
} lowering;

static const uint8_t fb_entry_size[256] = {
#define FB_SIZE(OP, TYPE, KIND) [OP] = sizeof(TYPE),
    _PyFb_FAMILIES(FB_SIZE)
    _PyFb_BRANCHES(FB_SIZE)
#undef FB_SIZE
};

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

static int
decode(lowering *L)
{
    PyCodeObject *co = L->co;
    int size = (int)Py_SIZE(co);
    int n = 0;
    for (int i = 0; i < size; n++) {
        lower_insn *in = &L->insns[n];
        memset(in, 0, sizeof(*in));
        in->start = i;
        in->depth = -1;
        in->target = -1;
        in->fb = -1;
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
        if (_PyOpcode_Deopt[opcode] != opcode
            || _PyOpcode_num_popped(opcode, oparg) < 0
            || _PyOpcode_num_pushed(opcode, oparg) < 0)
        {
            return malformed(co, i, "invalid instruction");
        }
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
    }
    L->n = n;
    return 0;
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

static int
resolve_jumps(lowering *L)
{
    int size = (int)Py_SIZE(L->co);
    for (int i = 0; i < L->n; i++) {
        lower_insn *in = &L->insns[i];
        int offset = jump_offset(L, i);
        if (offset == INT_MIN) {
            continue;
        }
        if (offset < 0 || offset >= size
            || L->insns[L->index[offset]].start != offset)
        {
            return malformed(L->co, in->anchor, "jump into an instruction");
        }
        in->target = L->index[offset];
        // END_ASYNC_FOR names END_SEND but does not jump there
        if (in->opcode != END_ASYNC_FOR) {
            L->insns[in->target].block_start = 1;
        }
    }
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
    int size = (int)Py_SIZE(co);
    while (err == 0 && p < end) {
        int start, length, handler, depth_lasti;
        p = parse_varint(p, &start);
        p = parse_varint(p, &length);
        p = parse_varint(p, &handler);
        p = parse_varint(p, &depth_lasti);
        if (handler < 0 || handler >= size
            || L->insns[L->index[handler]].start != handler)
        {
            err = malformed(co, handler, "exception handler inside an instruction");
            break;
        }
        int h = L->index[handler];
        L->insns[h].block_start = 1;
        int depth = (depth_lasti >> 1) + (depth_lasti & 1) + 1;
        err = push_depth(L, stack, &sp, h, depth);
    }
    while (err == 0 && sp > 0) {
        int i = stack[--sp];
        lower_insn *in = &L->insns[i];
        int popped = _PyOpcode_num_popped(in->opcode, in->oparg);
        int pushed = _PyOpcode_num_pushed(in->opcode, in->oparg);
        if (in->depth < popped) {
            err = malformed(co, in->anchor, "stack underflow");
            break;
        }
        int depth = in->depth - popped + pushed;
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
 * fire. */
static void
mark_line_starts(lowering *L)
{
    PyCodeAddressRange range;
    _PyCode_InitAddressRange(L->co, &range);
    int current = -1;
    for (int i = 0; i < L->n; i++) {
        lower_insn *in = &L->insns[i];
        int line = _PyCode_CheckLineNumber(in->start * (int)sizeof(_Py_CODEUNIT),
                                           &range);
        if (line != current && line >= 0) {
            in->block_start = 1;
        }
        current = line;
    }
}

/* The handler for an instruction before instrumentation and absorption:
 * its opcode, or a form that pushes its constant inline. */
static int
choose_handler(PyCodeObject *co, lower_insn *in)
{
#ifdef Py_STACKREF_INLINE_VALUES
    if (_PyInterpreterState_GET()->opt_config.tagged_values_enabled) {
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
#endif
    return in->opcode;
}

/* Loads that push without escaping, raising or writing a local */
static inline int
is_plain_load(int opcode)
{
    switch (opcode) {
        case LOAD_FAST:
        case LOAD_FAST_BORROW:
        case LOAD_FAST_LOAD_FAST:
        case LOAD_FAST_BORROW_LOAD_FAST_BORROW:
        case LOAD_CONST:
        case LOAD_SMALL_INT:
        case NOP:
            return 1;
        default:
            return 0;
    }
}

/* Absorb the borrowed loads that feed the inputs of instruction i which its
 * format lets carry IR_BORROW.  Only plain loads may lie between, and no
 * line or block may start after the load, so nothing can observe the
 * NULL in the canonical slot or write the local before i reads it. */
static void
absorb_loads(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(in->handler);
    if (in->depth < 0 || (fmt->flags & _PyIR_WINDOW) || fmt->borrow_mask == 0) {
        return;
    }
    int base = in->depth - fmt->ninputs;
    for (int j = i - 1; j >= 0; j--) {
        lower_insn *load = &L->insns[j];
        if (L->insns[j + 1].block_start || !is_plain_load(load->opcode)
            || load->depth < 0)
        {
            return;
        }
        int pushed = _PyOpcode_num_pushed(load->opcode, load->oparg);
        if (load->depth < base) {
            return;
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
            continue;
        }
        int ok = 1;
        for (int p = 0; p < pushed; p++) {
            int k = load->depth + p - base;
            ok &= k < fmt->ninputs && (fmt->borrow_mask >> k) & 1
                  && locals[p] < L->co->co_nlocalsplus;
        }
        if (!ok) {
            continue;
        }
        for (int p = 0; p < pushed; p++) {
            int k = load->depth + p - base;
            in->borrow[k] = IR_SLOT_FIELD(locals[p]) | IR_BORROW;
        }
        load->handler = load->opcode == LOAD_FAST_BORROW
            ? IR_ABSORBED_LOAD_FAST_BORROW
            : IR_ABSORBED_LOAD_FAST_BORROW_LOAD_FAST_BORROW;
    }
}

static inline int
insn_nwords(const lower_insn *in)
{
    return in->ext + _PyIR_FormatOf(in->handler)->nwords;
}

/* The jump distance in words of instruction i */
static int
jump_distance(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    lower_insn *target = &L->insns[in->target];
    int next = in->word + insn_nwords(in);
    int dest = target->word;
    if (in->opcode == FOR_ITER || in->opcode == END_ASYNC_FOR) {
        // FOR_ITER skips END_FOR, and END_ASYNC_FOR names END_SEND, by
        // their main words
        dest += target->ext;
    }
    if (in->opcode == JUMP_BACKWARD || in->opcode == JUMP_BACKWARD_NO_INTERRUPT
        || in->opcode == END_ASYNC_FOR)
    {
        return next - dest;
    }
    return dest - next;
}

/* The oparg that the handlers of instruction i read */
static uint32_t
handler_oparg(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    if (_PyIR_FormatOf(in->handler)->flags & _PyIR_JUMPS) {
        return (uint32_t)jump_distance(L, i);
    }
    if (in->opcode == GET_ITER && in->oparg) {
        // The compiler cannot know whether types.coroutine() will be called
        return (L->co->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))
            ? GET_ITER_YIELD_FROM_NO_CHECK : GET_ITER_YIELD_FROM_CORO_CHECK;
    }
    return (uint32_t)in->oparg;
}

static uint32_t
fb_distance(lowering *L, int i)
{
    lower_insn *in = &L->insns[i];
    if (in->fb < 0) {
        return 0;
    }
    int main = in->word + in->ext;
    return (uint32_t)(L->nwords + FB_HEADER_WORDS + in->fb - main);
}

static uint32_t
slot_field(int slot)
{
    assert(slot >= 0);
    return IR_SLOT_FIELD(slot);
}

/* Fields A, B, C and word 1 of instruction i */
static void
encode(lowering *L, int i, uint32_t fields[3], _PyIRWord *word1)
{
    lower_insn *in = &L->insns[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(in->handler);
    int nlocalsplus = L->co->co_nlocalsplus;
    int popped = _PyOpcode_num_popped(in->opcode, in->oparg);
    // Unreachable code never runs; give it fields that stay in the frame.
    int depth = in->depth >= 0 ? in->depth : Py_MAX(popped, fmt->ninputs);
    uint32_t oparg = handler_oparg(L, i);
    uint32_t fb = fb_distance(L, i);
    for (int r = 0; r < 3; r++) {
        int role = fmt->roles[r];
        uint32_t value = 0;
        if (role == _PyIR_ROLE_DST) {
            value = slot_field(nlocalsplus + depth + fmt->dst);
        }
        else if (role == _PyIR_ROLE_BASE) {
            value = slot_field(nlocalsplus + depth - popped);
        }
        else if (role == _PyIR_ROLE_FB) {
            value = fb;
        }
        else if (role == _PyIR_ROLE_IMM) {
            value = oparg;
        }
        else if (role >= _PyIR_ROLE_IN0) {
            int k = role - _PyIR_ROLE_IN0;
            value = in->borrow[k] ? in->borrow[k]
                : slot_field(nlocalsplus + depth - fmt->ninputs + k);
        }
        fields[r] = value;
    }
    *word1 = IR_WORD1(fmt->imm == _PyIR_LOC_D ? oparg : 0,
                      fmt->fb == _PyIR_LOC_F ? fb : 0);
}

/* Place the words, adding IR_EXT words until every field fits */
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
        int changed = 0;
        for (int i = 0; i < L->n; i++) {
            lower_insn *in = &L->insns[i];
            uint32_t fields[3];
            _PyIRWord word1;
            encode(L, i, fields, &word1);
            if (!in->ext && (fields[0] | fields[1] | fields[2]) > 0xFFFF) {
                in->ext = 1;
                changed = 1;
            }
        }
        if (!changed) {
            return;
        }
    }
}

static void
init_entry(lowering *L, lower_insn *in, _PyFbHeader *h, int enable_counters)
{
    h->kind = fb_kind[in->opcode];
    if (h->kind == FB_BRANCH) {
        h->history = 0x5555;    // alternating taken and not taken
        return;
    }
    h->counter = enable_counters ? adaptive_counter_warmup()
                                 : initial_unreachable_backoff_counter();
    h->arg = (uint32_t)handler_oparg(L, (int)(in - L->insns));
}

static _PyIRBlock *
emit(lowering *L)
{
    PyCodeObject *co = L->co;
    int wide = Py_SIZE(co) > UINT16_MAX;
    size_t words_size = (size_t)L->nwords * sizeof(_PyIRWord);
    size_t fb_size = L->fb_words
        ? sizeof(_PyFbBlock) + (size_t)L->fb_words * sizeof(_PyIRWord) : 0;
    size_t pc_size = (size_t)L->nwords * (wide ? sizeof(_PyIRPc32) : sizeof(_PyIRPc16));
    _PyIRBlock *block = PyMem_Calloc(1, sizeof(_PyIRBlock) + words_size
                                        + fb_size + pc_size);
    if (block == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyIRWord *words = (_PyIRWord *)(block + 1);
    block->words = words;
    block->nwords = L->nwords;
    block->flags = wide ? _PyIR_PCMAP_WIDE : 0;
    block->pcmap = (char *)words + words_size + fb_size;
    char *entries = NULL;
    if (fb_size) {
        block->feedback = (_PyFbBlock *)(words + L->nwords);
        block->feedback->size = (uint32_t)fb_size;
        entries = (char *)(block->feedback + 1);
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
    for (int i = 0; i < L->n; i++) {
        lower_insn *in = &L->insns[i];
        uint32_t fields[3];
        _PyIRWord word1;
        encode(L, i, fields, &word1);
        int w = in->word;
        if (in->ext) {
            words[w++] = IR_WORD(IR_EXT, fields[0] >> 16, fields[1] >> 16,
                                 fields[2] >> 16);
        }
        // co_code_adaptive holds opcodes that instrumentation put there,
        // in place of the lowering's choice, which real[] keeps.
        int handler = in->top == in->opcode ? in->handler : in->top;
        words[w] = IR_WORD(handler, fields[0], fields[1], fields[2]);
        if (instrumented) {
            block->real[w] = (uint16_t)in->handler;
        }
        if (_PyIR_FormatOf(in->handler)->nwords == 2) {
            words[w + 1] = word1;
        }
        if (in->fb >= 0) {
            init_entry(L, in, (_PyFbHeader *)(entries + in->fb * sizeof(_PyIRWord)),
                       enable_counters);
        }
        for (int k = in->word; k < in->word + insn_nwords(in); k++) {
            if (wide) {
                ((_PyIRPc32 *)block->pcmap)[k] = (_PyIRPc32){
                    (uint32_t)in->start, (uint32_t)in->anchor};
            }
            else {
                ((_PyIRPc16 *)block->pcmap)[k] = (_PyIRPc16){
                    (uint16_t)in->start, (uint16_t)in->anchor};
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
        _PyIRWord *first = block->words + in->word;
        _PyIRWord *main = first + in->ext;
        assert(_PyIR_InstrAt(block, in->start) == first);
        assert(_PyIR_InstrAt(block, in->anchor) == first);
        assert(_PyIR_MainWordAt(block, in->anchor) == main);
        assert(_PyIR_Anchor(block, main) == in->anchor);
        assert(_PyIR_Start(block, first) == in->start);
        assert(ext_word(block, main) == (in->ext ? first : NULL));
        int handler = in->handler;
        if (_PyIR_FormatOf(handler)->imm != _PyIR_LOC_NONE) {
            assert((uint32_t)_PyIR_InstrOparg(block, main, handler)
                   == handler_oparg(L, i));
        }
        _PyFbHeader *fb = _PyIR_InstrFeedback(block, main, handler);
        assert((fb != NULL) == (in->fb >= 0));
        assert(fb == NULL || fb->kind == fb_kind[in->opcode]);
    }
}
#endif

static _PyIRBlock *
lower(PyCodeObject *co)
{
    lowering L = {.co = co};
    _PyIRBlock *block = NULL;
    Py_ssize_t size = Py_SIZE(co);
    L.insns = PyMem_Malloc(Py_MAX(size, 1) * sizeof(lower_insn));
    L.index = PyMem_Malloc(Py_MAX(size, 1) * sizeof(int));
    if (L.insns == NULL || L.index == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    if (decode(&L) < 0 || resolve_jumps(&L) < 0 || compute_depths(&L) < 0) {
        goto done;
    }
    mark_line_starts(&L);
    for (int i = 0; i < L.n; i++) {
        L.insns[i].handler = choose_handler(co, &L.insns[i]);
    }
    for (int i = 0; i < L.n; i++) {
        absorb_loads(&L, i);
    }
    for (int i = 0; i < L.n; i++) {
        lower_insn *in = &L.insns[i];
        if (_PyIR_FormatOf(in->handler)->fb != _PyIR_LOC_NONE) {
            assert(fb_entry_size[in->opcode] != 0);
            in->fb = L.fb_words;
            L.fb_words += fb_entry_size[in->opcode] / (int)sizeof(_PyIRWord);
        }
    }
    layout(&L);
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
        block = lower(co);
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
        // Past the first RESUME
        instr = _PyIR_MainWordAt(block, co->_co_firsttraceable);
        instr += _PyIR_FormatOf(decoding_handler(block, instr,
                                                 IR_HANDLER(*instr)))->nwords;
    }
    frame->instr_ptr = instr;
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
    PyMem_Free(block->real);
    PyMem_Free(block);
    co->_co_ir = &_PyIR_LowerStub;
}

size_t
_PyIR_SizeOf(PyCodeObject *co)
{
    _PyIRBlock *block = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_ir);
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock) {
        return 0;
    }
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
    return res;
}


/* Instrumentation */

/* Call fn for the first and main word of each instruction of block */
#define FOR_EACH_INSTRUCTION(block, first, main, BODY) \
    for (Py_ssize_t first = 0; first < (Py_ssize_t)(block)->nwords; ) { \
        Py_ssize_t main = first + (IR_HANDLER((block)->words[first]) == IR_EXT); \
        BODY \
        first = main + _PyIR_FormatOf(decoding_handler( \
            (block), (block)->words + main, \
            IR_HANDLER((block)->words[main])))->nwords; \
    }

int
_PyIR_PrepareInstrumentation(PyCodeObject *co)
{
    _PyIRBlock *block = co->_co_ir;
    if (block == &_PyIR_LowerStub || block == &_PyIR_InitCleanupBlock
        || block->real != NULL)
    {
        return 0;
    }
    uint16_t *real = PyMem_Calloc(block->nwords, sizeof(uint16_t));
    if (real == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    FOR_EACH_INSTRUCTION(block, first, main, {
        // Not instrumented yet, so the handler is the lowering's choice
        // or a specialisation of it.
        int handler = IR_HANDLER(block->words[main]);
        assert(!(_PyIR_FormatOf(handler)->flags & _PyIR_TRANSPARENT));
        real[main] = (uint16_t)(handler >= IR_FIRST_ONLY_HANDLER
                                ? handler : _PyOpcode_Deopt[handler]);
    })
    block->real = real;
    return 0;
}

void
_PyIR_SyncInstrumentation(PyCodeObject *co, int offset)
{
    _PyIRBlock *block = co->_co_ir;
    if (block == &_PyIR_LowerStub) {
        // The lowering reads the instrumentation when it runs
        return;
    }
    _PyIRWord *instr = _PyIR_MainWordAt(block, offset);
    if (_PyIR_Anchor(block, instr) != offset) {
        // An EXTENDED_ARG prefix, which has no word of its own
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
    // its instrumented and uninstrumented forms.
    _PyFbHeader *h = _PyIR_InstrFeedback(block, instr, handler);
    if (h != NULL && h->kind != FB_BRANCH) {
        FT_ATOMIC_STORE_UINT16_RELAXED(h->counter.value_and_backoff,
                                       adaptive_counter_warmup().value_and_backoff);
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
    FOR_EACH_INSTRUCTION(block, first, main, {
        _PyIRWord *instr = block->words + main;
        int offset = _PyIR_Anchor(block, instr);
        int handler = IR_HANDLER(*instr);
        _Py_CODEUNIT *unit = &instructions[offset];
        if (handler < IR_FIRST_ONLY_HANDLER) {
            // Specialised or instrumented; absorbed loads keep their opcode
            unit->op.code = (uint8_t)handler;
        }
        int base = _Py_GetBaseCodeUnit(co, offset).op.code;
        int caches = _PyOpcode_Caches[base];
        if (caches) {
            memset(&unit[1], 0, caches * sizeof(_Py_CODEUNIT));
            _PyFbHeader *h = _PyIR_InstrFeedback(block, instr, handler);
            if (h != NULL && h->kind == FB_BRANCH) {
                unit[1].cache = h->history;
            }
            else if (h != NULL) {
                write_fields(unit, h);
            }
        }
    })
    return 0;
}

#endif  /* Py_IR_INTERP */
