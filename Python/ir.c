/* The executing copies of code objects, and the decoding of IR words.  See
 * boil/design/06-architecture-v2.md, section 2, and
 * Include/internal/pycore_ir.h.
 *
 * The compiler emits the words (Python/assemble.c).  A code object keeps
 * them, with its feedback table, as it was created; the first call copies
 * them into the executing copy (_PyCode_GetExec()), which specialisation
 * and instrumentation rewrite.  Everything that needs an instruction's
 * oparg, target or generic handler decodes the code object's words, so
 * nothing about an instruction lives only in the executing copy except its
 * current handler and its feedback.
 */

#define NEED_IR_METADATA            // defines _PyIR_Formats, _PyIR_OnlyNames
#include "Python.h"

#include "pycore_backoff.h"         // adaptive_counter_warmup()
#include "pycore_code.h"            // _PyCode_WORDS()
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_interpframe.h"     // _PyFrame_GetCode()
#include "pycore_ir.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Deopt
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pystate.h"         // _PyInterpreterState_GET()
#include "pycore_value.h"

static_assert(sizeof(_PyCodeExec) % sizeof(_PyIRWord) == 0,
              "words must follow the header aligned");
static_assert(sizeof(_PyFbBlock) % sizeof(_PyIRWord) == 0,
              "entries must follow the feedback header aligned");
static_assert(sizeof(_PyFbHeader) == IR_FB_FIRST_UNIT << IR_FB_UNIT_SHIFT,
              "entry 0 is the header");
static_assert(sizeof(_PyFbUnary) % 8 == 0 && sizeof(_PyFbBinary) % 8 == 0
              && sizeof(_PyFbAttr) % 8 == 0 && sizeof(_PyFbGlobal) % 8 == 0
              && sizeof(_PyFbCall) % 8 == 0,
              "entries are whole units");
static_assert(FB_COUNTER < 16 && FB_CALL < 16, "a kind has 4 bits in co_fbinit");


/* _Py_InitCleanup (Python/specialize.c) runs with self in slot 0 and the
 * result of __init__ in slot 1: EXIT_INIT_CHECK checks slot 1, and
 * RETURN_VALUE returns slot 0. The explicit continuation names the first
 * real word. Both slots are temporaries, and the header precedes the words,
 * as in every executing copy, for IR_IS_LOCAL(). */
_PyIRStaticExec _PyIR_InitCleanup = {
    .exec = {
        .words = _PyIR_InitCleanup.words,
        .nwords = 2,
        .nlocalsplus = 0,
        .leaf = NULL,
    },
    .words = {
        IR_WORD(EXIT_INIT_CHECK, 1, 0, 0, 0),
        IR_WORD(RETURN_VALUE, 0, 0, 0, 0),
    },
};


/* Decoding */

int
_PyIR_EventOpcode(int handler)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->flags & _PyIR_FUSED) {
        return fmt->twin == handler ? INSTRUMENTED_POP_JUMP_IF_FALSE
                                    : POP_JUMP_IF_FALSE;
    }
    return _PyIR_BaseOpcode(handler);
}

int
_PyIR_BaseOpcode(int handler)
{
    if (handler == IR_JUMP) {
        return JUMP_FORWARD;
    }
    if (handler >= IR_FIRST_ONLY_HANDLER) {
        handler = _PyIR_Formats[handler].shadow;
        assert(handler < IR_FIRST_ONLY_HANDLER);
    }
    return _PyOpcode_Deopt[handler];
}

/* The value of the field at loc of word */
static inline uint32_t
read_field(_PyIRWord word, int loc)
{
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

static inline int
is_binary_op_handler(int handler)
{
    return handler >= _PyIR_BINARY_OP_FIRST
           && handler <= _PyIR_BINARY_OP_FIRST + NB_OPARG_LAST;
}

int
_PyIR_FieldOparg(_PyIRWord word)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->imm == _PyIR_LOC_NONE) {
        return 0;
    }
    assert(fmt->imm != _PyIR_LOC_FBARG);
    if (fmt->flags & _PyIR_FORWARD) {
        return IR_DS(word);
    }
    return (int)read_field(word, fmt->imm);
}

int
_PyCode_InstrFoldedInputs(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->borrow_mask == 0) {
        return 0;
    }
    int n = 0;
    for (int r = 0; r < 4; r++) {
        int k = fmt->roles[r] - _PyIR_ROLE_IN0;
        if (k >= 0 && (fmt->borrow_mask & (1 << k))
            && (int)((word >> (16 + 12 * r)) & IR_FIELD_MAX) < co->co_nlocalsplus)
        {
            n++;
        }
    }
    return n;
}

int
_PyCode_InstrStoredLocal(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (!(fmt->flags & _PyIR_STORE_DST)) {
        return -1;
    }
    for (int r = 0; r < 4; r++) {
        if (fmt->roles[r] == _PyIR_ROLE_DST) {
            int dst = (int)((word >> (16 + 12 * r)) & IR_FIELD_MAX);
            return dst < co->co_nlocalsplus ? dst : -1;
        }
    }
    return -1;
}

int
_PyCode_FbOrdinal(PyCodeObject *co, uint32_t unit)
{
    return _PyIR_FbFind(_PyCode_FBINIT(co), co->co_nfeedback, unit);
}

uint32_t
_PyCode_FbUnits(PyCodeObject *co)
{
    return co->co_nfeedback ? _PyIR_FBINIT_ARG(_PyCode_FBINIT(co)[0]) : 0;
}

int
_PyCode_InstrDepthBefore(PyCodeObject *co, int i, int *floor)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    int nl = co->co_nlocalsplus;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int depth = -1;
    int dst = -1;
    int agree = 1;
    int nstack = 0;
    int lowest = INT_MAX;
    int highest = -1;
    *floor = 0;
    for (int r = 0; r < 4; r++) {
        int role = fmt->roles[r];
        int slot = (int)((word >> (16 + 12 * r)) & IR_FIELD_MAX);
        if (role == _PyIR_ROLE_BASE && (fmt->flags & _PyIR_WINDOW)) {
            if (slot >= nl) {
                int extent = _PyIR_Extent(fmt->reads,
                                          _PyCode_InstrOparg(co, i));
                depth = slot - nl + extent;
                *floor = slot - nl;
            }
        }
        else if (role >= _PyIR_ROLE_IN0 && slot >= nl && slot != IR_NO_DST
                 && !(fmt->flags & _PyIR_WINDOW)) {
            // (A window's sources are its top entries: the extent that its
            // base gives counts them.)
            // The k-th of n inputs lies n-k-1 slots below the top
            int d = slot - nl + fmt->ninputs - (role - _PyIR_ROLE_IN0);
            if (depth >= 0 && d != depth) {
                agree = 0;
            }
            depth = Py_MAX(depth, d);
            nstack++;
            lowest = Py_MIN(lowest, slot - nl);
            highest = Py_MAX(highest, slot - nl);
        }
        else if (role == _PyIR_ROLE_DST && slot >= nl && slot != IR_NO_DST) {
            dst = slot - nl;
        }
    }
    if (!agree) {
        // The inputs are not in stack order: a renamed SWAP leaves the
        // operands of a store the other way round.  With every input in a
        // slot they still fill the top slots, in some order.  Otherwise the
        // depth is not known.
        if (nstack != fmt->ninputs || highest - lowest + 1 != nstack) {
            return _PyCode_DEPTH_UNKNOWN;
        }
        depth = highest + 1;
    }
    if (depth < 0 && dst >= 0 && !(fmt->flags & _PyIR_WINDOW)) {
        // Only folded loads feed the word: its result takes the slot of the
        // first input
        depth = dst + fmt->ninputs;
    }
    if (depth >= 0 && !(fmt->flags & _PyIR_WINDOW)) {
        *floor = depth - fmt->ninputs;
    }
    return depth;
}

int
_PyCode_InstrHoles(PyCodeObject *co, int i, int holes[4])
{
    int floor;
    int named = _PyCode_InstrDepthBefore(co, i, &floor);
    if (named < 0) {
        return 0;
    }
    _PyIRWord word = _PyCode_WORDS(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int n = 0;
    for (int r = 0; r < 4; r++) {
        int k = fmt->roles[r] - _PyIR_ROLE_IN0;
        if (k >= 0 && (fmt->borrow_mask & (1 << k))
            && (int)((word >> (16 + 12 * r)) & IR_FIELD_MAX) < co->co_nlocalsplus)
        {
            holes[n++] = named - fmt->ninputs + k;
        }
    }
    return n;
}

int
_PyCode_InstrDiscards(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (!(fmt->flags & _PyIR_DISCARDS)) {
        return 0;
    }
    for (int r = 0; r < 4; r++) {
        if (fmt->roles[r] == _PyIR_ROLE_DST) {
            return (int)((word >> (16 + 12 * r)) & IR_FIELD_MAX) == IR_NO_DST;
        }
    }
    return 0;
}

int
_PyCode_InstrUnpackedLocals(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (!(fmt->flags & _PyIR_UNPACKS)) {
        return 0;
    }
    for (int r = 0; r < 4; r++) {
        if (fmt->roles[r] == _PyIR_ROLE_DST) {
            int dst = (int)((word >> (16 + 12 * r)) & IR_FIELD_MAX);
            if (dst >= co->co_nlocalsplus) {
                return 0;
            }
            return _PyIR_Extent(fmt->writes, _PyCode_InstrOparg(co, i));
        }
    }
    return 0;
}

int
_PyCode_InstrFbIndex(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->fb == _PyIR_LOC_NONE) {
        return -1;
    }
    return (int)read_field(word, fmt->fb);
}

int
_PyCode_InstrOparg(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < Py_SIZE(co));
    _PyIRWord word = _PyCode_WORDS(co)[i];
    int handler = IR_HANDLER(word);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    switch (fmt->imm) {
        case _PyIR_LOC_NONE:
            return 0;
        case _PyIR_LOC_FBARG: {
            if (is_binary_op_handler(handler)) {
                return handler - _PyIR_BINARY_OP_FIRST;
            }
            // The sites that need the operation have an entry of their own
            int i = _PyCode_FbOrdinal(co, read_field(word, fmt->fb));
            assert(i > 0);
            return (int)_PyIR_FBINIT_ARG(_PyCode_FBINIT(co)[i]);
        }
        default:
            if (fmt->flags & _PyIR_FORWARD) {
                return IR_DS(word);
            }
            return (int)read_field(word, fmt->imm);
    }
}

int
_PyCode_InstrTarget(PyCodeObject *co, int i)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(_PyCode_InstrHandler(co, i));
    if (fmt->flags & _PyIR_JUMPS) {
        return _PyCode_InstrOparg(co, i);
    }
    if (fmt->flags & _PyIR_FORWARD) {
        return i + 1 + _PyCode_InstrOparg(co, i);
    }
    return -1;
}

_PyFbHeader *
_PyIR_InstrFeedback(PyCodeObject *co, const _PyCodeExec *exec,
                    const _PyIRWord *instr)
{
    int fb = _PyCode_InstrFbIndex(co, (int)(instr - exec->words));
    if (fb < 0) {
        return NULL;
    }
    return _PyIR_FbEntry(exec->entries, (uint32_t)fb);
}


int
_PyCode_GenericHandler(PyCodeObject *co, int i)
{
    return _PyCode_InstrHandler(co, i);
}

int
_PyCode_HandlerFor(PyCodeObject *co, int i, int opcode)
{
    int handler = _PyCode_InstrHandler(co, i);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->flags & _PyIR_FUSED) {
        // Instrumentation sees a fused word as a conditional jump
        // (_PyIR_EventOpcode()): it goes back to the word's own handler
        // and goes on to the twin.
        if (opcode == POP_JUMP_IF_FALSE) {
            return handler;
        }
        if (opcode == INSTRUMENTED_POP_JUMP_IF_FALSE) {
            return fmt->twin;
        }
        return opcode;
    }
    if (_PyIR_BaseOpcode(handler) == opcode) {
        return handler;
    }
    return opcode;
}

int
_PyIR_RealHandler(_PyInterpreterFrame *frame, const _PyIRWord *instr)
{
    int handler = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr));
    if (_PyIR_FormatOf(handler)->flags & _PyIR_TRANSPARENT) {
        handler = _PyCode_InstrHandler(_PyFrame_GetCode(frame),
                                       _PyFrame_InstrOffset(frame, instr));
    }
    return handler;
}

int
_PyIR_OriginalHandler(_PyInterpreterFrame *frame, const _PyIRWord *instr,
                      int opcode)
{
    return _PyCode_HandlerFor(_PyFrame_GetCode(frame),
                              _PyFrame_InstrOffset(frame, instr), opcode);
}


/* The executing copy */

typedef struct {
    _PyIRLeafPath path;
    uint32_t dependencies;
    int slot;
} LeafDefinition;

typedef struct {
    LeafDefinition definitions[_Py_IR_LEAF_WORDS];
    int count;
    uint32_t executed;
} LeafBindings;

/* Fail closed if generated operand/effect contracts grow beyond this consumer.
 * Callback-capable read/compare/entry/return effects remain obligations for
 * the guarded runtime path, never declarations of generic-opcode purity. */
static int
leaf_format(int handler)
{
    if (handler < 0 || handler >= IR_NUM_HANDLERS) {
        return 0;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    uint8_t roles[4] = {_PyIR_ROLE_DST, _PyIR_ROLE_NONE,
                        _PyIR_ROLE_NONE, _PyIR_ROLE_NONE};
    int inputs = 0, writes = 1, writes_form = _PyIR_EXT_FIXED;
    int imm = _PyIR_LOC_NONE, fb = _PyIR_LOC_NONE;
    int flags = _PyIR_VALID | _PyIR_STORE_DST | _PyIR_DISCARDS;
    int effects = 0;
    switch (handler) {
        case LOAD_FAST: case LOAD_FAST_BORROW:
            roles[1] = _PyIR_ROLE_LOCAL_IN;
            imm = _PyIR_LOC_B;
            effects = _PyIR_PURE;
            break;
        case LOAD_CONST: case LOAD_CONST_INLINE_INT: case LOAD_CONST_INLINE_FLOAT:
            roles[2] = roles[3] = _PyIR_ROLE_IMM;
            imm = _PyIR_LOC_CD;
            effects = handler == LOAD_CONST ? _PyIR_PURE : 0;
            break;
        case LOAD_SMALL_INT: case LOAD_SMALL_INT_INLINE:
            roles[1] = _PyIR_ROLE_IMM;
            imm = _PyIR_LOC_B;
            /* The range check below proves the immediate constructor cannot
             * fail; its generic metadata conservatively includes allocation. */
            effects = _PyIR_ALLOCATES |
                (handler == LOAD_SMALL_INT ? _PyIR_PURE : _PyIR_RAISES);
            break;
        case LOAD_GLOBAL:
            roles[1] = _PyIR_ROLE_FB;
            roles[2] = roles[3] = _PyIR_ROLE_IMM;
            imm = _PyIR_LOC_CD;
            fb = _PyIR_LOC_B;
            writes_form = _PyIR_EXT_BIT0;
            flags = _PyIR_VALID | _PyIR_RUNS_CODE;
            break;
        case LOAD_ATTR:
            roles[1] = _PyIR_ROLE_IN0;
            roles[2] = _PyIR_ROLE_FB;
            roles[3] = _PyIR_ROLE_IMM;
            inputs = 1;
            imm = _PyIR_LOC_D;
            fb = _PyIR_LOC_C;
            writes_form = _PyIR_EXT_BIT0;
            flags |= _PyIR_NO_FOLD | _PyIR_RUNS_CODE;
            break;
        case COMPARE_OP_JUMP:
            roles[0] = _PyIR_ROLE_IN0;
            roles[1] = _PyIR_ROLE_IN0 + 1;
            roles[2] = _PyIR_ROLE_FB;
            roles[3] = _PyIR_ROLE_IMM;
            inputs = 2;
            writes = 0;
            imm = _PyIR_LOC_D;
            fb = _PyIR_LOC_C;
            flags = _PyIR_VALID | _PyIR_RUNS_CODE | _PyIR_FORWARD | _PyIR_FUSED;
            break;
        case RETURN_VALUE:
            roles[0] = _PyIR_ROLE_IN0;
            inputs = 1;
            writes = 0;
            flags = _PyIR_VALID | _PyIR_RUNS_CODE;
            break;
        case RESUME:
            roles[0] = _PyIR_ROLE_IMM;
            writes = 0;
            imm = _PyIR_LOC_A;
            flags = _PyIR_VALID | _PyIR_RUNS_CODE;
            break;
        default:
            return 0;
    }
    if (handler == LOAD_GLOBAL || handler == LOAD_ATTR ||
        handler == COMPARE_OP_JUMP || handler == RETURN_VALUE || handler == RESUME) {
        effects = _PyIR_RAISES | _PyIR_ESCAPES | _PyIR_RUNS_PYTHON | _PyIR_ALLOCATES;
        if (handler == LOAD_GLOBAL || handler == LOAD_ATTR || handler == COMPARE_OP_JUMP) {
            effects |= _PyIR_SPECIALIZES;
        }
        if (handler == RESUME) {
            effects |= _PyIR_POLLS;
        }
    }
    return memcmp(fmt->roles, roles, sizeof(roles)) == 0 &&
        fmt->flags == flags && fmt->effects == effects &&
        fmt->imm == imm && fmt->fb == fb && fmt->ninputs == inputs &&
        fmt->borrow_mask == (1 << inputs) - 1 &&
        fmt->reads.form == _PyIR_EXT_FIXED && fmt->reads.k == inputs &&
        fmt->writes.form == writes_form && fmt->writes.k == writes &&
        fmt->through.form == _PyIR_EXT_FIXED && fmt->through.k == 0;
}

/* Only construction walks IR. Execution follows four bounded read paths. */
static int
leaf_source(PyCodeObject *co, LeafBindings *bindings, int slot,
            LeafDefinition *out)
{
    if (slot < 0 || slot >= _PyFrame_NumSlotsForCodeObject(co)) {
        return 0;
    }
    for (int i = bindings->count; i-- > 0;) {
        if (bindings->definitions[i].slot == slot) {
            *out = bindings->definitions[i];
            return out->path.kind != 0;
        }
    }
    *out = (LeafDefinition){.slot = slot};
    if (slot < co->co_argcount) {
        out->path.kind = _PY_IR_LEAF_ARGUMENT;
        out->path.root = (uint32_t)slot;
        return 1;
    }
    const uint32_t *constants = _PyCode_CONSTSLOTS(co);
    for (int i = 0; i < co->co_nconstslots; i++) {
        if (constants[2 * i] == (uint32_t)slot &&
            constants[2 * i + 1] < (uint32_t)PyTuple_GET_SIZE(co->co_consts)) {
            out->path.kind = _PY_IR_LEAF_CONSTANT;
            out->path.root = constants[2 * i + 1];
            return 1;
        }
    }
    return 0;
}

static int
leaf_consume(PyCodeObject *co, LeafBindings *bindings, int slot)
{
    if (slot < co->co_nlocalsplus) {
        return 1;
    }
    for (int i = bindings->count; i-- > 0;) {
        if (bindings->definitions[i].slot == slot) {
            int live = bindings->definitions[i].path.kind != 0;
            bindings->definitions[i].path.kind = 0;
            return live;
        }
    }
    return 0;
}

static int
leaf_feedback(PyCodeObject *co, int pc, int kind)
{
    int unit = _PyCode_InstrFbIndex(co, pc);
    if (unit < IR_FB_FIRST_UNIT) {
        return 0;
    }
    int ordinal = _PyCode_FbOrdinal(co, (uint32_t)unit);
    const _PyIRFormat *fmt = _PyIR_FormatOf(_PyCode_InstrHandler(co, pc));
    if (ordinal <= 0 || _PyIR_FBINIT_KIND(_PyCode_FBINIT(co)[ordinal]) != kind ||
        fmt->fb_kind != kind || fmt->fb_size == 0) {
        return 0;
    }
    uint32_t end = ordinal + 1 < co->co_nfeedback
        ? _PyIR_FBINIT_UNIT(_PyCode_FBINIT(co)[ordinal + 1])
        : _PyCode_FbUnits(co);
    return end >= (uint32_t)unit + (fmt->fb_size >> IR_FB_UNIT_SHIFT);
}

static int
leaf_body(PyCodeObject *co, int pc, LeafBindings bindings,
          _PyIRLeafRegion *region, int arm, uint32_t predicate_dependencies,
          uint32_t *visited)
{
    while (pc > 0 && pc < Py_SIZE(co)) {
        uint32_t bit = UINT32_C(1) << pc;
        if (*visited & bit) {
            return 0;
        }
        *visited |= bit;
        _PyIRWord word = co->co_words[pc];
        int handler = IR_HANDLER(word);
        if (!leaf_format(handler)) {
            return 0;
        }
        LeafDefinition value;
        if (handler == RETURN_VALUE) {
            if (!leaf_source(co, &bindings, IR_A(word), &value) ||
                ((value.dependencies | predicate_dependencies) & bindings.executed)
                    != bindings.executed) {
                return 0;
            }
            region->paths[arm] = value.path;
            return 1;
        }
        if (handler == COMPARE_OP_JUMP) {
            LeafDefinition right;
            if (region->conditional || !leaf_feedback(co, pc, FB_BINARY) ||
                !leaf_source(co, &bindings, IR_A(word), &value) ||
                !leaf_source(co, &bindings, IR_B(word), &right)) {
                return 0;
            }
            int target = pc + 1 + IR_DS(word);
            if (target <= pc + 1 || target >= Py_SIZE(co)) {
                return 0;
            }
            int ordinal = _PyCode_FbOrdinal(co, IR_C(word));
            uint32_t comparison = _PyIR_FBINIT_ARG(_PyCode_FBINIT(co)[ordinal]);
            if (comparison & ~(IR_FUSED_OP_MASK | _PyIR_FUSED_IF_FALSE)) {
                return 0;
            }
            if (((comparison & IR_FUSED_OP_MASK) >> 5) > Py_GE) {
                return 0;
            }
            region->conditional = 1;
            region->comparison = comparison;
            region->paths[0] = value.path;
            region->paths[1] = right.path;
            uint32_t dependencies = value.dependencies | right.dependencies;
            if (!leaf_consume(co, &bindings, IR_A(word)) ||
                !leaf_consume(co, &bindings, IR_B(word))) {
                return 0;
            }
            return leaf_body(co, pc + 1, bindings, region, 2,
                             dependencies, visited) &&
                   leaf_body(co, target, bindings, region, 3,
                             dependencies, visited);
        }
        if (handler == LOAD_FAST || handler == LOAD_FAST_BORROW) {
            if (IR_B(word) >= (uint32_t)co->co_nlocalsplus ||
                !leaf_source(co, &bindings, IR_B(word), &value)) {
                return 0;
            }
        }
        else if (handler == LOAD_CONST || handler == LOAD_CONST_INLINE_INT ||
                 handler == LOAD_CONST_INLINE_FLOAT) {
            uint32_t constant = IR_CD(word);
            if (constant >= (uint32_t)PyTuple_GET_SIZE(co->co_consts)) {
                return 0;
            }
            value = (LeafDefinition){.path = {
                .kind = _PY_IR_LEAF_CONSTANT, .root = constant}};
        }
        else if (handler == LOAD_SMALL_INT || handler == LOAD_SMALL_INT_INLINE) {
            if (IR_B(word) >= _PY_NSMALLPOSINTS) {
                return 0;
            }
            value = (LeafDefinition){.path = {
                .kind = _PY_IR_LEAF_SMALL_INT, .root = IR_B(word)}};
        }
        else if (handler == LOAD_GLOBAL) {
            if ((IR_CD(word) & 1) ||
                (IR_CD(word) >> 1) >= (uint32_t)PyTuple_GET_SIZE(co->co_names) ||
                !leaf_feedback(co, pc, FB_GLOBAL)) {
                return 0;
            }
            value = (LeafDefinition){.path = {
                .kind = _PY_IR_LEAF_GLOBAL, .root = (uint32_t)pc}};
        }
        else if (handler == LOAD_ATTR) {
            if ((IR_D(word) & 1) ||
                (IR_D(word) >> 1) >= (uint32_t)PyTuple_GET_SIZE(co->co_names) ||
                !leaf_feedback(co, pc, FB_ATTR) ||
                !leaf_source(co, &bindings, IR_B(word), &value) ||
                value.path.length == _Py_IR_LEAF_STEPS ||
                !leaf_consume(co, &bindings, IR_B(word))) {
                return 0;
            }
            value.path.attributes[value.path.length++] = (uint16_t)pc;
        }
        else {
            return 0;
        }
        int destination = (int)IR_A(word);
        if (destination < co->co_nlocalsplus ||
            destination >= _PyFrame_NumSlotsForCodeObject(co) ||
            bindings.count == _Py_IR_LEAF_WORDS) {
            return 0;
        }
        value.slot = destination;
        value.dependencies |= bit;
        bindings.definitions[bindings.count++] = value;
        bindings.executed |= bit;
        pc++;
    }
    return 0;
}

static _PyIRLeafRegion *
make_leaf_region(PyCodeObject *co)
{
    if (Py_SIZE(co) < 2 || Py_SIZE(co) > _Py_IR_LEAF_WORDS ||
        co->co_argcount > _Py_IR_LEAF_WINDOW - 2 || co->co_kwonlyargcount ||
        co->co_ncellvars || co->co_nfreevars ||
        (co->co_flags & (CO_OPTIMIZED | CO_NEWLOCALS)) !=
            (CO_OPTIMIZED | CO_NEWLOCALS) ||
        (co->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR |
                         CO_VARARGS | CO_VARKEYWORDS)) ||
        PyBytes_GET_SIZE(co->co_exceptiontable) ||
        _PyCode_InstrHandler(co, 0) != RESUME || !leaf_format(RESUME) ||
        IR_A(co->co_words[0]) != 0) {
        return NULL;
    }
    /* Validate implicit roots independently of their eventual use. */
    const uint32_t *constants = _PyCode_CONSTSLOTS(co);
    for (int i = 0; i < co->co_nconstslots; i++) {
        uint32_t slot = constants[2 * i];
        if (slot < (uint32_t)co->co_argcount || slot >= (uint32_t)co->co_nlocalsplus ||
            constants[2 * i + 1] >= (uint32_t)PyTuple_GET_SIZE(co->co_consts) ||
            !(_PyLocals_GetKind(co->co_localspluskinds, slot) & CO_FAST_CONST)) {
            return NULL;
        }
        for (int j = 0; j < i; j++) {
            if (constants[2 * j] == slot) {
                return NULL;
            }
        }
    }
    _PyIRLeafRegion region = {0};
    LeafBindings bindings = {0};
    uint32_t visited = 1;
    if (!leaf_body(co, 1, bindings, &region, 2, 0, &visited) ||
        visited != (UINT32_MAX >> (32 - Py_SIZE(co)))) {
        return NULL;
    }
    _PyIRLeafRegion *result = _PyMem_NoScanMalloc(sizeof(*result));
    if (result != NULL) {
        *result = region;
    }
    return result;
}

static _PyCodeExec *
make_exec(PyCodeObject *co)
{
    Py_ssize_t nwords = Py_SIZE(co);
    int nfb = co->co_nfeedback;
    size_t words_size = (size_t)nwords * sizeof(_PyIRWord);
    size_t fb_size = nfb
        ? sizeof(_PyFbBlock) + ((size_t)_PyCode_FbUnits(co) << IR_FB_UNIT_SHIFT)
        : 0;
    // Feedback entries hold borrowed pointers only.
    _PyCodeExec *exec = _PyMem_NoScanMalloc(sizeof(_PyCodeExec) + words_size + fb_size);
    if (exec == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyIRWord *words = (_PyIRWord *)(exec + 1);
    *exec = (_PyCodeExec){
        .words = words,
        .nwords = (uint32_t)nwords,
        .nlocalsplus = (uint32_t)co->co_nlocalsplus,
        .leaf = make_leaf_region(co),
    };
    memcpy(words, _PyCode_WORDS(co), words_size);
    if (fb_size == 0) {
        return exec;
    }
    exec->feedback = (_PyFbBlock *)(words + nwords);
    memset(exec->feedback, 0, fb_size);
    exec->feedback->size = (uint32_t)fb_size;
    exec->entries = (char *)(exec->feedback + 1);
    // The shared entry is a header only and never specialises: its counter
    // never triggers
    _PyFbHeader *shared = _PyIR_FbEntry(exec->entries, 0);
    shared->kind = FB_COUNTER;
    shared->state = FB_OFF;
    shared->counter = initial_unreachable_backoff_counter();
#ifdef Py_GIL_DISABLED
    int enable_counters = 0;
#else
    int enable_counters = ENABLE_SPECIALIZATION
        && _PyInterpreterState_GET()->opt_config.specialization_enabled;
#endif
    _Py_BackoffCounter counter = enable_counters
        ? adaptive_counter_warmup() : initial_unreachable_backoff_counter();
    const uint32_t *fbinit = _PyCode_FBINIT(co);
    for (int i = 1; i < nfb; i++) {
        _PyFbHeader *h = _PyIR_FbEntry(exec->entries,
                                       _PyIR_FBINIT_UNIT(fbinit[i]));
        h->kind = (uint8_t)_PyIR_FBINIT_KIND(fbinit[i]);
        h->counter = counter;
        if (h->kind == FB_BRANCH) {
            h->history = 0x5555;    // alternating taken and not taken
        }
        else {
            h->arg = _PyIR_FBINIT_ARG(fbinit[i]);
        }
    }
    // The entry of a fused word keeps the taken history of its jump, seeded
    // as for a BRANCH entry
    for (Py_ssize_t i = 0; i < nwords; i++) {
        const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(words[i]));
        if (fmt->flags & _PyIR_FUSED) {
            int fb = (int)read_field(words[i], fmt->fb);
            assert(fb >= IR_FB_FIRST_UNIT);
            _PyIR_FbEntry(exec->entries, (uint32_t)fb)->taken = 0x5555;
        }
    }
    return exec;
}

_PyCodeExec *
_PyCode_GetExec(PyCodeObject *co)
{
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    if (exec != NULL) {
        return exec;
    }
    Py_BEGIN_CRITICAL_SECTION(co);
    exec = co->_co_exec;
    if (exec == NULL) {
        exec = make_exec(co);
        if (exec != NULL) {
            FT_ATOMIC_STORE_PTR_RELEASE(co->_co_exec, exec);
        }
    }
    Py_END_CRITICAL_SECTION();
    return exec;
}

void
_PyCode_ClearExec(PyCodeObject *co)
{
    /* Only code_dealloc frees the executing copy: a running frame holds a
     * reference to its code object, so frame->ir_words never changes while
     * the frame runs (pyjit/design/register-ir.md 1.10). */
    assert(Py_REFCNT(co) == 0);
    _PyCodeExec *exec = co->_co_exec;
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return;
    }
    co->_co_exec = NULL;
    if (exec->feedback != NULL) {
        _PyFb_Clear(exec->feedback);
    }
    _PyMem_NoScanFree(exec->leaf);
    _PyMem_NoScanFree(exec);
}

size_t
_PyCode_ExecSizeOf(PyCodeObject *co)
{
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return 0;
    }
    size_t res = sizeof(_PyCodeExec) + exec->nwords * sizeof(_PyIRWord);
    if (exec->leaf != NULL) {
        res += sizeof(*exec->leaf);
    }
    _PyFbBlock *feedback = exec->feedback;
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
