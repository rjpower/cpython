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
static_assert(sizeof(_PyCodeExec) - offsetof(_PyCodeExec, nlocalsplus) ==
              sizeof(void *) + sizeof(uint32_t),
              "keep the local-count offset relative to executing words");
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
 * real word. Both slots are temporaries, so the words carry no slot-class
 * flag, and the header precedes the words, as in every executing copy, for
 * IR_IS_LOCAL(). */
_PyIRStaticExec _PyIR_InitCleanup = {
    .exec = {
        .words = _PyIR_InitCleanup.words,
        .nwords = 2,
        .nlocalsplus = 0,
    },
    .words = {
        IR_WORD(EXIT_INIT_CHECK, 1, 0, 0, 0),
        IR_WORD(RETURN_VALUE, 0, 0, 0, 0),
    },
};


/* Decoding */

const _PyIRGeneratorBoundary *
_PyIR_GeneratorBoundaryOf(int handler)
{
    if (handler < 0 || handler >= IR_NUM_HANDLERS ||
        !(_PyIR_HandlerFlags[handler] & _PyIR_H_DISPATCH))
    {
        return NULL;
    }
    const _PyIRGeneratorBoundary *boundary =
        &_PyIR_GeneratorBoundaries[handler];
    assert(boundary->handler == handler);
    return boundary;
}

int
_PyIR_GeneratorBoundaryHandler(int ordinary, int yielded_consumer,
                              int completed_consumer, int resume_consumer)
{
    const _PyIRGeneratorBoundary *source = _PyIR_GeneratorBoundaryOf(ordinary);
    if (source == NULL || source->ordinary != ordinary ||
        (source->flags & _PyIR_GEN_BOUNDARY_PLACED) ||
        yielded_consumer < _PyIR_GEN_CONSUMER_NONE ||
        yielded_consumer > _PyIR_GEN_CONSUMER_POP_TOP ||
        completed_consumer < _PyIR_GEN_CONSUMER_NONE ||
        completed_consumer > _PyIR_GEN_CONSUMER_POP_TOP ||
        resume_consumer < _PyIR_GEN_CONSUMER_NONE ||
        resume_consumer > _PyIR_GEN_CONSUMER_POP_TOP ||
        !(yielded_consumer || completed_consumer || resume_consumer))
    {
        return -1;
    }
    for (int handler = 0; handler < IR_NUM_HANDLERS; handler++) {
        const _PyIRGeneratorBoundary *boundary =
            _PyIR_GeneratorBoundaryOf(handler);
        if (boundary != NULL && boundary->ordinary == ordinary &&
            (boundary->flags & (_PyIR_GEN_BOUNDARY_PLACED |
                                _PyIR_GEN_BOUNDARY_CLOSED)) ==
                (_PyIR_GEN_BOUNDARY_PLACED | _PyIR_GEN_BOUNDARY_CLOSED) &&
            boundary->yielded_consumer == yielded_consumer &&
            boundary->completed_consumer == completed_consumer &&
            boundary->resume_consumer == resume_consumer)
        {
            return handler;
        }
    }
    return -1;
}

static int
generator_ordinary(_PyIRWord word)
{
    const _PyIRGeneratorBoundary *boundary =
        _PyIR_GeneratorBoundaryOf(IR_HANDLER(word));
    return boundary == NULL ? -1 : boundary->ordinary;
}

static int
generator_is(_PyIRWord word, int ordinary)
{
    int handler = generator_ordinary(word);
    return handler == ordinary ||
        (_PyIR_FormatOf(ordinary)->twin != 0 &&
         handler == _PyIR_FormatOf(ordinary)->twin);
}

/* This is the existing successful-edge retirement receipt, not an opcode
 * classification. It grants no physical NULL or owner fact. */
static const _PyIROwnershipEvent *
generator_completion(_PyIRWord word)
{
    int ordinary = generator_ordinary(word);
    if (ordinary < 0) {
        return NULL;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(ordinary);
    if (fmt->owner_events != 1 || fmt->roles[0] != _PyIR_ROLE_DST ||
        fmt->roles[1] != _PyIR_ROLE_BASE)
    {
        return NULL;
    }
    const _PyIROwnershipEvent *event = _PyIR_OwnerPrograms + fmt->owner_program;
    if (event->kind != _PyIR_OWNER_NORMAL_COMPLETION || event->dst != 1 ||
        !(event->flags & _PyIR_COMPLETION_HAS_RESULT) ||
        event->src != _PyIR_Extent(fmt->reads, 0))
    {
        return NULL;
    }
    return event;
}

static int
generator_exit(_PyIRWord word, int pc)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(generator_ordinary(word));
    return fmt->flags & _PyIR_FORWARD ? pc + 1 + IR_DS(word) : (int)IR_CD(word);
}

/* Resume STORE_LOCAL is an actual STORE_FAST consumer. Completion
 * STORE_LOCAL instead means an already folded local A, and skips no word. */
static int
generator_resume_shape(const _PyIRWord *words, int nwords, int nl, int pc,
                       int *consumer)
{
    if (pc + 2 >= nwords || !generator_is(words[pc + 1], RESUME)) {
        return _PyIR_GEN_CONSUMER_NONE;
    }
    int home = (int)IR_A(words[pc]);
    _PyIRWord next = words[pc + 2];
    if (generator_is(next, STORE_FAST) && (int)IR_A(next) == home &&
        (int)IR_B(next) < nl && home >= nl && home != IR_NO_DST)
    {
        *consumer = pc + 2;
        return _PyIR_GEN_CONSUMER_STORE_LOCAL;
    }
    if (!generator_is(next, JUMP_BACKWARD_NO_INTERRUPT)) {
        return _PyIR_GEN_CONSUMER_NONE;
    }
    int target = (int)IR_CD(next);
    if (target < 0 || target >= nwords) {
        return _PyIR_GEN_CONSUMER_NONE;
    }
    const _PyIROwnershipEvent *event = generator_completion(words[target]);
    if (event == NULL || !(event->flags & _PyIR_COMPLETION_SEND_YIELDED) ||
        event->src != 3 || event->extent != 2 ||
        (int)IR_B(words[target]) + event->extent != home)
    {
        return _PyIR_GEN_CONSUMER_NONE;
    }
    *consumer = target;
    return _PyIR_GEN_CONSUMER_SEND;
}

static int
generator_completed_shape(const _PyIRWord *words, int nwords, int nl, int pc)
{
    int target = generator_exit(words[pc], pc);
    if (target < 0 || target >= nwords) {
        return _PyIR_GEN_CONSUMER_NONE;
    }
    int home = (int)IR_A(words[pc]);
    if (home == IR_NO_DST) {
        return _PyIR_GEN_CONSUMER_DISCARD;
    }
    if (home < nl) {
        return _PyIR_GEN_CONSUMER_STORE_LOCAL;
    }
    if ((generator_is(words[target], RETURN_VALUE) ||
         generator_is(words[target], RETURN_VALUE_EMPTY)) &&
        (int)IR_A(words[target]) == home)
    {
        return _PyIR_GEN_CONSUMER_RETURN;
    }
    if (generator_is(words[target], STORE_FAST) &&
        (int)IR_A(words[target]) == home && (int)IR_B(words[target]) < nl)
    {
        return _PyIR_GEN_CONSUMER_STORE_FAST;
    }
    if (generator_is(words[target], POP_TOP) &&
        (int)IR_A(words[target]) == home)
    {
        return _PyIR_GEN_CONSUMER_POP_TOP;
    }
    /* A real SSA value may survive another call/await or feed an ordinary
     * arithmetic provider. Its necessary scratch publication is retained.
     * Common normal-transfer admission proves that A is absent or belongs
     * to the actual retired window; this shape query grants no such fact. */
    return _PyIR_GEN_CONSUMER_HOME;
}

int
_PyIR_GeneratorBoundaryCandidate(const _PyIRWord *words, int nwords, int nl,
                                int pc)
{
    if (pc < 0 || pc >= nwords || nl < 0 || nl > IR_MAX_SLOTS) {
        return -1;
    }
    _PyIRWord word = words[pc];
    int ordinary = generator_ordinary(word);
    if (ordinary < 0) {
        return -1;
    }
    int yielded = _PyIR_GEN_CONSUMER_NONE;
    int completed = _PyIR_GEN_CONSUMER_NONE;
    int resume = _PyIR_GEN_CONSUMER_NONE;
    const _PyIROwnershipEvent *event = generator_completion(word);
    if (event != NULL) {
        completed = generator_completed_shape(words, nwords, nl, pc);
        if (completed == _PyIR_GEN_CONSUMER_NONE) {
            return -1;
        }
        if (event->flags & _PyIR_COMPLETION_SEND_YIELDED) {
            if (pc + 1 >= nwords || event->src != 3 || event->extent != 2 ||
                !generator_is(words[pc + 1], YIELD_DELEGATED) ||
                IR_D(words[pc + 1]) != IR_B(word) ||
                IR_B(words[pc + 1]) != IR_B(word) + event->extent)
            {
                return -1;
            }
            int consumer;
            if (generator_resume_shape(words, nwords, nl, pc + 1, &consumer) !=
                    _PyIR_GEN_CONSUMER_SEND || consumer != pc)
            {
                return -1;
            }
            yielded = _PyIR_GEN_CONSUMER_DELEGATED;
        }
    }
    else if (generator_is(word, YIELD_VALUE) ||
             generator_is(word, YIELD_DELEGATED))
    {
        int consumer;
        resume = generator_resume_shape(words, nwords, nl, pc, &consumer);
        if (resume == _PyIR_GEN_CONSUMER_NONE ||
            (generator_is(word, YIELD_DELEGATED) &&
             (resume != _PyIR_GEN_CONSUMER_SEND || consumer != pc - 1)) ||
            (!generator_is(word, YIELD_DELEGATED) &&
             resume != _PyIR_GEN_CONSUMER_STORE_LOCAL))
        {
            return -1;
        }
    }
    else {
        return -1;
    }
    return _PyIR_GeneratorBoundaryHandler(ordinary, yielded, completed, resume);
}

int
_PyIR_CheckGeneratorBoundary(const _PyIRWord *words, int nwords, int nl,
                            int pc, int handler)
{
    if (_PyIR_GeneratorBoundaryCandidate(words, nwords, nl, pc) != handler) {
        return 0;
    }
    const _PyIRGeneratorBoundary *boundary = _PyIR_GeneratorBoundaryOf(handler);
    if (boundary->yielded_consumer == _PyIR_GEN_CONSUMER_DELEGATED) {
        const _PyIRGeneratorBoundary *next =
            _PyIR_GeneratorBoundaryOf(IR_HANDLER(words[pc + 1]));
        if (next == NULL || !(next->flags & _PyIR_GEN_BOUNDARY_CLOSED) ||
            next->resume_consumer != _PyIR_GEN_CONSUMER_SEND)
        {
            return 0;
        }
    }
    if (boundary->resume_consumer == _PyIR_GEN_CONSUMER_SEND) {
        if (pc == 0) {
            return 0;
        }
        const _PyIRGeneratorBoundary *source =
            _PyIR_GeneratorBoundaryOf(IR_HANDLER(words[pc - 1]));
        if (source == NULL || !(source->flags & _PyIR_GEN_BOUNDARY_CLOSED) ||
            source->yielded_consumer != _PyIR_GEN_CONSUMER_DELEGATED)
        {
            return 0;
        }
    }
    if (boundary->completed_consumer != _PyIR_GEN_CONSUMER_NONE) {
        int target = generator_exit(words[pc], pc);
        int send_found = boundary->yielded_consumer != _PyIR_GEN_CONSUMER_NONE;
        /* END_SEND has both normal SEND and CLEANUP_THROW ingress. Select
         * them together; a compatible window on one edge cannot certify a
         * different edge's value or make a partial public cycle complete. */
        for (int i = 0; i < nwords; i++) {
            const _PyIROwnershipEvent *event = generator_completion(words[i]);
            if (event == NULL || generator_exit(words[i], i) != target) {
                continue;
            }
            const _PyIRGeneratorBoundary *incoming =
                _PyIR_GeneratorBoundaryOf(IR_HANDLER(words[i]));
            if (incoming == NULL ||
                !(incoming->flags & _PyIR_GEN_BOUNDARY_CLOSED) ||
                incoming->completed_consumer != boundary->completed_consumer ||
                IR_A(words[i]) != IR_A(words[pc]) ||
                IR_B(words[i]) != IR_B(words[pc]))
            {
                return 0;
            }
            send_found |= incoming->yielded_consumer != _PyIR_GEN_CONSUMER_NONE;
        }
        return send_found;
    }
    return 1;
}

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

static int
ordinary_handler_for(int handler, int opcode)
{
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
    if (handler == RETURN_VALUE_EMPTY || handler == INSTRUMENTED_RETURN_VALUE_EMPTY) {
        if (opcode == RETURN_VALUE) {
            return RETURN_VALUE_EMPTY;
        }
        if (opcode == INSTRUMENTED_RETURN_VALUE) {
            return INSTRUMENTED_RETURN_VALUE_EMPTY;
        }
    }
    if (handler == YIELD_DELEGATED || handler == INSTRUMENTED_YIELD_DELEGATED) {
        if (opcode == YIELD_VALUE) {
            return YIELD_DELEGATED;
        }
        if (opcode == INSTRUMENTED_YIELD_VALUE) {
            return INSTRUMENTED_YIELD_DELEGATED;
        }
    }
    if (_PyIR_BaseOpcode(handler) == opcode) {
        return handler;
    }
    return opcode;
}

int
_PyCode_HandlerFor(PyCodeObject *co, int i, int opcode)
{
    int handler = _PyCode_InstrHandler(co, i);
    const _PyIRGeneratorBoundary *boundary = _PyIR_GeneratorBoundaryOf(handler);
    int ordinary = boundary == NULL ? handler : boundary->ordinary;
    int target = ordinary_handler_for(ordinary, opcode);
    if (boundary == NULL || !(boundary->flags & _PyIR_GEN_BOUNDARY_PLACED) ||
        (_PyIR_FormatOf(target)->flags & _PyIR_TRANSPARENT))
    {
        /* LINE/INSTRUCTION keep their existing transparent identity. Their
         * callback path must materialize before an ordinary source body. */
        return target;
    }
    int placed = _PyIR_GeneratorBoundaryHandler(
        target, boundary->yielded_consumer, boundary->completed_consumer,
        boundary->resume_consumer);
    if (placed < 0) {
        Py_FatalError("missing closed generator instrumentation target");
    }
    return placed;
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
    };
    if (_PyCode_BuildSemanticPlan(co, &exec->semantic_plan) < 0) {
        _PyMem_NoScanFree(exec);
        return NULL;
    }
    /* The slot-class flags are a cache of the fields and nlocalsplus,
     * which handlers test in the word instead of loading nlocalsplus. */
    const _PyIRWord *source = _PyCode_WORDS(co);
    for (Py_ssize_t i = 0; i < nwords; i++) {
        assert(!(source[i] & IR_F_LOCAL_MASK));
        words[i] = source[i] | _PyIR_ExecLocalFlags(source[i], exec->nlocalsplus);
    }
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
    if (exec->semantic_plan != NULL &&
        (exec->semantic_plan->nsites != 0 || exec->semantic_plan->nphases != 0)) {
        PyMem_Free((void *)exec->semantic_plan);
    }
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
    if (exec->semantic_plan != NULL &&
        (exec->semantic_plan->nsites != 0 || exec->semantic_plan->nphases != 0)) {
        res += _PyIR_PlanSize(exec->semantic_plan);
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

int _PyIR_ShadowRetirementObserved = 0;

uint8_t
_PyIR_ShadowRetirement(_PyInterpreterFrame *frame, uint8_t reason)
{
    assert(reason <= _PyIR_RETIRE_STAGED_FAILURE);
    if (reason != _PyIR_RETIRE_NORMAL_RETURN || frame->f_executable == NULL) {
        return _PyIR_SHADOW_UNCOVERED;
    }
    PyCodeObject *co = frame->f_executable;
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    if (exec == NULL || exec->semantic_plan == NULL ||
        frame->ir_words != exec->words)
    {
        return _PyIR_SHADOW_UNCOVERED;
    }
    /* Integer bounds/alignment checks precede word decoding. This also
     * excludes static executable images attached to an ordinary code pin. */
    uintptr_t pc = (uintptr_t)frame->instr_ptr;
    uintptr_t start = (uintptr_t)exec->words;
    if (pc < start || pc - start >= (size_t)exec->nwords * sizeof(_PyIRWord) ||
        (pc - start) % sizeof(_PyIRWord))
    {
        return _PyIR_SHADOW_UNCOVERED;
    }
    uint32_t site = (uint32_t)((pc - start) / sizeof(_PyIRWord));
    if (_PyIR_FormatOf(IR_HANDLER(co->co_words[site]))->transfer_kind !=
        _PyIR_TRANSFER_EXIT)
    {
        return _PyIR_SHADOW_UNCOVERED;
    }
    const _PyIRCodePlan *plan = exec->semantic_plan;
    if (!(plan->flags & _PyIR_PLAN_ALL_RETURNS)) {
        uint32_t low = 0, high = plan->nsites;
        while (low < high) {
            uint32_t mid = low + (high - low) / 2;
            if (plan->sites[mid] < site) {
                low = mid + 1;
            }
            else {
                high = mid;
            }
        }
        if (low == plan->nsites || plan->sites[low] != site) {
            return _PyIR_SHADOW_UNCOVERED;
        }
    }
    /* Nullable bindings, including hidden constants, keep their dynamic
     * authority. The first exact scratch shape is empty, so an inline value
     * is a mismatch too; testing mortal counts alone would miss it. */
    for (int home = co->co_nlocalsplus;
         home < _PyFrame_NumSlotsForCodeObject(co); home++)
    {
        if (!PyValue_IsNull(frame->base[home])) {
            return _PyIR_SHADOW_MISMATCH;
        }
    }
    return _PyIR_SHADOW_EXACT;
}

/* Validate a pinned frame image before either an active shape or finite
 * suspension-role reader inspects its immutable instruction. */
static _PyCodeExec *
frame_image_site(_PyInterpreterFrame *frame, const _PyInstr *origin, uint32_t *site)
{
    if (frame->f_executable == NULL) {
        return NULL;
    }
    PyCodeObject *co = frame->f_executable;
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    int nl = co->co_nlocalsplus;
    int scratch = Py_MAX(co->co_stacksize, 1);
    if (exec == NULL || frame->base == NULL ||
        frame->ir_words != exec->words || exec->nwords != Py_SIZE(co) ||
        exec->nlocalsplus != nl || nl < 0 || nl > IR_MAX_SLOTS ||
        scratch > IR_MAX_SLOTS - nl || _PyFrame_NumSlotsForCodeObject(co) != nl + scratch)
    {
        return NULL;
    }
    uintptr_t pc = (uintptr_t)origin;
    uintptr_t start = (uintptr_t)exec->words;
    size_t size = (size_t)exec->nwords * sizeof(_PyIRWord);
    if (pc < start || pc - start >= size ||
        (pc - start) % sizeof(_PyIRWord))
    {
        return NULL;
    }
    *site = (uint32_t)((pc - start) / sizeof(_PyIRWord));
    return exec;
}

int
_PyIR_DelegatedReceiverHome(_PyInterpreterFrame *frame)
{
    if (frame->owner != FRAME_OWNED_BY_GENERATOR) {
        return -1;
    }
    uint32_t continuation;
    _PyCodeExec *exec = frame_image_site(frame, frame->instr_ptr, &continuation);
    if (exec == NULL || continuation == 0) {
        return -1;
    }
    PyCodeObject *co = frame->f_executable;
    _PyIRWord word = co->co_words[continuation - 1];
    int handler = IR_HANDLER(word);
    const _PyIRGeneratorBoundary *boundary = _PyIR_GeneratorBoundaryOf(handler);
    if (boundary == NULL) {
        return -1;
    }
    handler = boundary->ordinary;
    if (handler != YIELD_DELEGATED && handler != INSTRUMENTED_YIELD_DELEGATED) {
        return -1;
    }
    int dst = (int)IR_A(word), outgoing = (int)IR_B(word), receiver = (int)IR_D(word);
    if (dst != frame->return_slot ||
        outgoing < co->co_nlocalsplus || outgoing >= _PyFrame_NumSlotsForCodeObject(co) ||
        receiver < co->co_nlocalsplus ||
        receiver >= _PyFrame_NumSlotsForCodeObject(co) ||
        receiver == dst || receiver == outgoing)
    {
        return -1;
    }
    return receiver;
}

static _PyIRHomeAuthority
plan_home_view(_PyInterpreterFrame *frame, _PyCodeExec *exec, uint32_t site,
               uint8_t selector_class, uint8_t phase, _PyIRHomeView *out)
{
    if (exec == NULL || exec->semantic_plan == NULL) {
        return _PyIR_HOME_NONE;
    }
    PyCodeObject *co = frame->f_executable;
    int nl = co->co_nlocalsplus;
    int scratch = Py_MAX(co->co_stacksize, 1);
    const _PyIRCodePlan *plan = exec->semantic_plan;
    const _PyIRPhaseSite *sites = _PyIR_PlanPhases(plan);
    uint32_t low = 0, high = plan->nphases;
    while (low < high) {
        uint32_t mid = low + (high - low) / 2;
        if (sites[mid].site < site ||
            (sites[mid].site == site &&
             (sites[mid].selector_class < selector_class ||
              (sites[mid].selector_class == selector_class && sites[mid].phase < phase)))) {
            low = mid + 1;
        }
        else {
            high = mid;
        }
    }
    if (low == plan->nphases || sites[low].site != site ||
        sites[low].selector_class != selector_class || sites[low].phase != phase) {
        return _PyIR_HOME_NONE;
    }
    const _PyIRPhaseSite *row = sites + low;
    if (row->authority != _PyIR_HOME_SHADOW_CANDIDATE &&
        row->authority != _PyIR_HOME_MUST_READABLE)
    {
        return _PyIR_HOME_NONE;
    }
    /* The immutable builder validates and orders ranges once. Readers borrow
     * this geometry; they do not construct a second decoder or revalidate it. */
    assert(row->first <= plan->nranges && row->count <= plan->nranges - row->first);
    const _PyIRPhaseRange *ranges = _PyIR_PlanRanges(plan) + row->first;
    *out = (_PyIRHomeView){
        .nbindings = (uint16_t)nl,
        .scratch_limit = (uint16_t)scratch,
        .nranges = row->count,
        .ranges = ranges,
    };
    return (_PyIRHomeAuthority)row->authority;
}

_PyIRHomeAuthority
_PyIR_GetErrorHomeView(_PyInterpreterFrame *frame, const _PyInstr *origin,
                       uint8_t checkpoint, _PyIRHomeView *out)
{
    if (checkpoint == 0) {
        return _PyIR_HOME_NONE;
    }
    uint32_t site;
    _PyCodeExec *exec = frame_image_site(frame, origin, &site);
    return exec == NULL ? _PyIR_HOME_NONE : plan_home_view(
        frame, exec, site, _PyIR_SELECTOR_ERROR, checkpoint, out);
}
