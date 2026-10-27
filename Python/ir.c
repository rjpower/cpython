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
static_assert(FB_STRIDE % sizeof(void *) == 0, "entries must stay aligned");
static_assert(sizeof(_PyFbUnary) <= FB_STRIDE && sizeof(_PyFbBinary) <= FB_STRIDE
              && sizeof(_PyFbGlobal) <= FB_STRIDE && sizeof(_PyFbCall) <= FB_STRIDE,
              "FB_STRIDE must hold every entry");


/* _Py_InitCleanup (Python/specialize.c) runs with self in slot 0 and the
 * result of __init__ in slot 1: EXIT_INIT_CHECK checks slot 1, and
 * RETURN_VALUE returns slot 0.  Its first word, a NOP, never runs: the shim's
 * instr_ptr stays there, and __init__ returns to the word after it.  Both
 * slots are temporaries, and the header precedes the words, as in every
 * executing copy, for IR_IS_LOCAL(). */
_PyIRStaticExec _PyIR_InitCleanup = {
    .exec = {
        .words = _PyIR_InitCleanup.words,
        .nwords = 3,
        .nlocalsplus = 0,
    },
    .words = {
        IR_WORD(NOP, 0, 0, 0, 0),
        IR_WORD(EXIT_INIT_CHECK, 1, 0, 0, 0),
        IR_WORD(RETURN_VALUE, 0, 0, 0, 0),
    },
};


/* Decoding */

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
            int fb = (int)read_field(word, fmt->fb);
            // Sites that share entry 0 have a NOFB handler instead
            assert(fb > 0 && fb < co->co_nfeedback);
            return (int)_PyIR_FBINIT_ARG(_PyCode_FBINIT(co)[fb]);
        }
        default:
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
_PyCode_HandlerFor(PyCodeObject *co, int i, int opcode)
{
    int handler = _PyCode_InstrHandler(co, i);
    if (_PyIR_BaseOpcode(handler) == opcode) {
        return handler;
    }
    // The instrumented twin of a NOFB form is NOFB too
    if (handler >= IR_FIRST_ONLY_HANDLER && _PyIR_FormatOf(handler)->fb == _PyIR_LOC_NONE
        && _PyIR_FormatOf(opcode)->nofb != 0)
    {
        return _PyIR_FormatOf(opcode)->nofb;
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

static _PyCodeExec *
make_exec(PyCodeObject *co)
{
    Py_ssize_t nwords = Py_SIZE(co);
    int nfb = co->co_nfeedback;
    size_t words_size = (size_t)nwords * sizeof(_PyIRWord);
    size_t fb_size = nfb ? sizeof(_PyFbBlock) + (size_t)nfb * FB_STRIDE : 0;
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
    memcpy(words, _PyCode_WORDS(co), words_size);
    if (fb_size == 0) {
        return exec;
    }
    exec->feedback = (_PyFbBlock *)(words + nwords);
    memset(exec->feedback, 0, fb_size);
    exec->feedback->size = (uint32_t)fb_size;
    exec->entries = (char *)(exec->feedback + 1);
    // The shared entry never specialises: its counter never triggers
    _PyFbHeader *shared = _PyIR_FbEntry(exec->entries, 0);
    shared->kind = FB_COUNTER;
    shared->state = FB_OFF;
    shared->counter = initial_unreachable_backoff_counter();
    int enable_counters = ENABLE_SPECIALIZATION
        && _PyInterpreterState_GET()->opt_config.specialization_enabled;
    _Py_BackoffCounter counter = enable_counters
        ? adaptive_counter_warmup() : initial_unreachable_backoff_counter();
    const uint32_t *fbinit = _PyCode_FBINIT(co);
    for (int i = 1; i < nfb; i++) {
        _PyFbHeader *h = _PyIR_FbEntry(exec->entries, (uint32_t)i);
        h->kind = (uint8_t)_PyIR_FBINIT_KIND(fbinit[i]);
        h->counter = counter;
        if (h->kind == FB_BRANCH) {
            h->history = 0x5555;    // alternating taken and not taken
        }
        else {
            h->arg = _PyIR_FBINIT_ARG(fbinit[i]);
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
    _PyCodeExec *exec = co->_co_exec;
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return;
    }
    if (exec->feedback != NULL) {
        _PyFb_Clear(exec->feedback);
    }
    _PyMem_NoScanFree(exec);
    co->_co_exec = NULL;
}

size_t
_PyCode_ExecSizeOf(PyCodeObject *co)
{
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return 0;
    }
    size_t res = sizeof(_PyCodeExec) + exec->nwords * sizeof(_PyIRWord);
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
