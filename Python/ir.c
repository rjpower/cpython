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
#include "pycore_code.h"            // PyCodeObject
#include "pycore_flowgraph.h"        // _PyCfg_LowerCode()
#include "pycore_regcfg.h"           // _PyIR_Assemble()
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_interpframe.h"     // _PyFrame_GetCode()
#include "pycore_ir.h"
#include "pycore_jit.h"              // _PyJit_FreeUnits()
#include "pycore_opcode_metadata.h" // _PyOpcode_Deopt
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pystate.h"         // _PyInterpreterState_GET()
#include "pycore_rop_metadata.h"    // _PyRop_SelectReturn(), ROP_HANDLER_*
#include "pycore_value.h"
#include "pycore_rop_metadata.h"     // ROP_HANDLER_LocalWrite_Store

static_assert(sizeof(_PyCodeExec) % sizeof(_PyIRWord) == 0,
              "words must follow the header aligned");
static_assert(sizeof(_PyCodeExec) - offsetof(_PyCodeExec, nfixed) ==
              sizeof(uint32_t),
              "keep the fixed-slot-count offset relative to executing words");
static_assert(sizeof(_PyFbBlock) % sizeof(_PyIRWord) == 0,
              "entries must follow the feedback header aligned");
static_assert(sizeof(_PyFbHeader) == IR_FB_FIRST_UNIT << IR_FB_UNIT_SHIFT,
              "entry 0 is the header");
static_assert(sizeof(_PyFbUnary) % 8 == 0 && sizeof(_PyFbBinary) % 8 == 0
              && sizeof(_PyFbAttr) % 8 == 0 && sizeof(_PyFbGlobal) % 8 == 0
              && sizeof(_PyFbCall) % 8 == 0
              && sizeof(_PyFbCompare) % 8 == 0 && sizeof(_PyFbContains) % 8 == 0,
              "entries are whole units");
static_assert(FB_COUNTER < 16 && FB_CONTAINS < 16, "a kind has 4 bits in co_fbinit");


/* _Py_InitCleanup (Python/specialize.c) runs with self in slot 0 and the
 * result of __init__ in slot 1: InitResult.Check checks slot 1, and the
 * Return arm of a thread frame returns slot 0. The explicit continuation names the first
 * real word. Both slots are temporaries, so the words carry no slot-class
 * flag, and the header precedes the words, as in every executing copy, for
 * IR_IS_LOCAL(). */
_PyIRStaticExec _PyIR_InitCleanup = {
    .exec = {
        .words = _PyIR_InitCleanup.words,
        .nwords = 2,
        .first_traceable = 2,
        .nfixed = 0,
    },
    .words = {
        IR_WORD(ROP_HANDLER_InitResult_Check, 1, 0, 0, 0),
        IR_WORD(_PyRop_SelectReturn(ROP_SELECT_THREAD_FRAME, ROP_SELECT_LIVE_SCRATCH),
                0, 0, 0, 0),
    },
};

/* The IR that the code object of the shim holds: no feedback, constant or
 * exception, and each word stands for the instruction of its index. */
static uint32_t init_cleanup_pcmap[2] = {0, 1};
_PyCodeIR _PyIR_InitCleanupIR = {
    .nwords = 2,
    .nslots = 2,
    .first_traceable = 2,
    .words = _PyIR_InitCleanup.words,
    .pcmap = init_cleanup_pcmap,
};


/* Decoding */

const char *
_PyIR_HandlerName(int handler)
{
    assert(handler >= 0 && handler < IR_NUM_HANDLERS);
    return _PyIR_IsOnlyHandler(handler)
        ? _PyIR_OnlyNames[handler] : _PyOpcode_OpName[_PyIR_HandlerOpcode[handler]];
}

int
_PyIR_OriginOpcode(PyCodeObject *co, int word)
{
    assert(word >= 0 && word < (int)_PyCode_IR(co)->nwords);
    int offset = (int)_PyCode_IR(co)->pcmap[word];
    return _PyOpcode_Deopt[_PyCode_CODE(co)[offset].op.code];
}

int
_PyIR_BaseOpcode(int handler)
{
    assert(handler >= 0 && handler < IR_NUM_HANDLERS);
    if (handler == IR_OPERANDS) {
        // Part of the instruction before it, as a CACHE is upstream
        return CACHE;
    }
    // The shadow of a handler is the stack opcode that it stands for
    int shadow = _PyIR_FormatOf(handler)->shadow;
    assert(shadow < (int)Py_ARRAY_LENGTH(_PyOpcode_Deopt));
    return _PyOpcode_Deopt[shadow];
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
        case _PyIR_LOC_AB:
            return IR_AB(word);
        case _PyIR_LOC_BC:
            return IR_BC(word);
        default:
            Py_UNREACHABLE();
    }
}

int
_PyIR_FieldOparg(_PyIRWord word)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->imm == _PyIR_LOC_NONE) {
        return 0;
    }
    assert(fmt->imm != _PyIR_LOC_FBARG);
    if (_PyIR_CALL_SHAPED(fmt->operands)) {
        // The oparg of the stack instruction: the number of arguments
        return fmt->operands == _PyIR_OPERANDS_EX
            ? 0 : _PyIR_CallSlotOperands(word) - _PyIR_CallHasSelf(word);
    }
    if ((fmt->flags & _PyIR_FORWARD) && fmt->target == _PyIR_LOC_NONE) {
        return IR_DS(word);
    }
    return (int)read_field(word, fmt->imm);
}

int
_PyCode_FbOrdinal(PyCodeObject *co, uint32_t unit)
{
    return _PyIR_FbFind(_PyCode_IR(co)->fbinit, (int)_PyCode_IR(co)->nfeedback, unit);
}

uint32_t
_PyCode_FbUnits(PyCodeObject *co)
{
    return (int)_PyCode_IR(co)->nfeedback ? _PyIR_FBINIT_ARG(_PyCode_IR(co)->fbinit[0]) : 0;
}

int
_PyCode_InstrFbIndex(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < _PyCode_IRSize(co));
    _PyIRWord word = _PyCode_IRWords(co)[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->fb == _PyIR_LOC_NONE) {
        if (fmt->fixed_words == 0) {
            return -1;
        }
        // An arm's entry is P0 of its first operand word
        return (int)(_PyCode_IRWords(co)[i + 1] & 0xFFFF);
    }
    return (int)read_field(word, fmt->fb);
}

int
_PyCode_InstrOparg(PyCodeObject *co, int i)
{
    assert(i >= 0 && i < _PyCode_IRSize(co));
    _PyIRWord word = _PyCode_IRWords(co)[i];
    int handler = IR_HANDLER(word);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    switch (fmt->imm) {
        case _PyIR_LOC_NONE:
            return 0;
        case _PyIR_LOC_FBARG: {
            // The sites that need the operation have an entry of their own
            int i = _PyCode_FbOrdinal(co, read_field(word, fmt->fb));
            assert(i > 0);
            return (int)_PyIR_FBINIT_ARG(_PyCode_IR(co)->fbinit[i]);
        }
        default:
            if ((fmt->flags & _PyIR_FORWARD) && fmt->target == _PyIR_LOC_NONE) {
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
        return i + 1 + _PyIR_FormatDistance(fmt, _PyCode_IRWords(co)[i]);
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
    return _PyIR_FbEntry(exec->words, (uint32_t)fb);
}


int
_PyCode_GenericHandler(PyCodeObject *co, int i)
{
    return _PyCode_InstrHandler(co, i);
}

int
_PyCode_IsTrailing(PyCodeObject *co, int i)
{
    return (_PyIR_HandlerFlags[_PyCode_InstrHandler(co, i)] & _PyIR_H_TRAILING) != 0;
}

int
_PyCode_HandlerFor(PyCodeObject *co, int i, int observed)
{
    int handler = _PyCode_InstrHandler(co, i);
    if (!observed) {
        return handler;
    }
    // A handler whose family declares events has an observed twin
    int twin = _PyIR_FormatOf(handler)->twin;
    assert(twin != _PyIR_NO_HANDLER && handler >= IR_FIRST_ARM);
    return twin;
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

/* The handler that the word at instr runs once a LINE or INSTRUCTION
 * wrapper has passed it on: the monitoring tables hold what the wrapper
 * displaced, which may be the other wrapper over an observed word. */
int
_PyIR_DispatchedHandler(_PyInterpreterFrame *frame, const _PyIRWord *instr)
{
    int handler = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr));
    if (handler != ROP_HANDLER_Line_Hook && handler != ROP_HANDLER_Instruction_Hook) {
        return _PyIR_RealHandler(frame, instr);
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int index = _PyFrame_InstrOffset(frame, instr);
    // LINE may displace INSTRUCTION, which displaces the word's handler
    if (handler == ROP_HANDLER_Line_Hook) {
        handler = _PyCode_LineDisplaced(code, index);
    }
    if (handler == ROP_HANDLER_Instruction_Hook) {
        handler = _PyCode_InstructionDisplaced(code, index);
    }
    return handler;
}


/* The executing copy */

/* word, with the dst of a discarded result (IR_NO_DST) replaced by the
 * frame's discard slot, which handlers write like a local. */
static _PyIRWord
exec_discard_slot(_PyIRWord word, uint32_t discard)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (!_PyIR_HasDst(fmt)) {
        return word;
    }
    for (int r = 0; r < 4; r++) {
        int shift = 16 + 12 * r;
        if (fmt->roles[r] == _PyIR_ROLE_DST &&
            ((word >> shift) & IR_FIELD_MAX) == IR_NO_DST)
        {
            return (word & ~((_PyIRWord)IR_FIELD_MAX << shift)) |
                   ((_PyIRWord)discard << shift);
        }
    }
    return word;
}

static _PyCodeExec *
make_exec(PyCodeObject *co, const _PyCodeIR *ir)
{
    Py_ssize_t nwords = ir->nwords;
    int nfb = (int)ir->nfeedback;
    size_t words_size = (size_t)nwords * sizeof(_PyIRWord);
    size_t fb_size = nfb
        ? sizeof(_PyFbBlock) + ((size_t)_PyCode_FbUnits(co) << IR_FB_UNIT_SHIFT)
        : 0;
    // The image of the constant slots follows the words
    int nconst = (int)ir->nconst;
    // The image has an even number of entries: the fill copies two at a
    // time, and the entry after an odd count is NULL
    size_t image_size = (size_t)(nconst + (nconst & 1)) * sizeof(PyValue);
    // Feedback entries hold borrowed pointers only.
    char *base = _PyMem_NoScanMalloc(
        fb_size + sizeof(_PyCodeExec) + words_size + image_size);
    if (base == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyCodeExec *exec = (_PyCodeExec *)(base + fb_size);
    _PyIRWord *words = (_PyIRWord *)(exec + 1);
    *exec = (_PyCodeExec){
        .words = words,
        .nwords = (uint32_t)nwords,
        .nfixed = ir->nfixed,
        .first_traceable = ir->first_traceable,
        .nconst = (uint16_t)nconst,
    };
    // Without constants the block is empty and begins at the discard slot
    exec->const_first = nconst > 0
        ? (uint16_t)ir->const_first
        : (uint16_t)(ir->nfixed - 1);
    if (nconst > 0) {
        // The frames borrow these: they are inline or immortal
        PyValueBorrow *values =
            (PyValueBorrow *)((char *)words + words_size);
        for (int j = 0; j < nconst; j++) {
            values[j] = ir->const_values[j];
        }
        if (nconst & 1) {
            values[nconst] = PyValue_Borrow(PyValue_NULL);
        }
        exec->const_image = values;
    }
    /* The slot-class flags are a cache of the fields and nfixed,
     * which handlers test in the word instead of loading nfixed. */
    const _PyIRWord *source = ir->words;
    uint32_t discard = ir->nfixed - 1;
    for (Py_ssize_t i = 0; i < nwords; i++) {
        assert(!(source[i] & IR_F_LOCAL_MASK));
        words[i] = exec_discard_slot(source[i], discard);
        words[i] |= _PyIR_ExecLocalFlags(words[i], exec->nfixed);
        /* An arm's fixed operand words are payload: no handler field,
         * no slot flags. */
        int fixed = _PyIR_WordLength(source[i]) - 1;
        if (!_PyIR_HandlerHasOperands(IR_HANDLER(source[i]))) {
            for (int k = 0; k < fixed; k++) {
                words[i + 1 + k] = source[i + 1 + k];
            }
            i += fixed;
        }
    }
    /* A call is a head and its operand words, whose fields name its operands */
    for (Py_ssize_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        if (_PyIR_HandlerHasOperands(IR_HANDLER(words[i]))) {
            words[i] = (words[i] & ~(IR_F_CALL_OWES_CALLABLE | IR_F_CALL_OWES_OPERANDS))
                       | _PyIR_ExecCallFlags(&words[i], exec->nfixed);
        }
    }
    if (fb_size == 0) {
        return exec;
    }
    memset(base, 0, fb_size);
    _PyIR_FbBlock(words)->size = (uint32_t)fb_size;
    // The shared entry is a header only and never specialises: its counter
    // never triggers
    _PyFbHeader *shared = _PyIR_FbEntry(words, IR_FB_SHARED);
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
    const uint32_t *fbinit = ir->fbinit;
    for (int i = 1; i < nfb; i++) {
        _PyFbHeader *h = _PyIR_FbEntry(words, _PyIR_FBINIT_FIELD(fbinit[i]));
        h->kind = (uint8_t)_PyIR_FBINIT_KIND(fbinit[i]);
        h->counter = counter;
        h->arg = _PyIR_FBINIT_ARG(fbinit[i]);
    }
    return exec;
}

/* Lowering needs no GC allocation and no Python code, so it cannot re-enter
 * itself on the same code object. */
_PyCodeIR *
_PyIR_LowerCode(PyCodeObject *co)
{
    _PyRegCfg *cfg;
    _PyCodeIR *ir = NULL;
    if (_PyCfg_LowerCode(co, &cfg) == 0) {
        if (_PyIR_Assemble(co, cfg, &ir) < 0) {
            ir = NULL;
        }
        _PyRegCfg_Free(cfg);
    }
    if (ir == NULL) {
        _PyErr_FormatNote("while lowering %U (%U:%d) to the register IR",
                          co->co_qualname, co->co_filename, co->co_firstlineno);
        return NULL;
    }
    return ir;
}

void
_PyIR_Free(_PyCodeIR *ir)
{
    PyMem_Free(ir);
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
        _PyCodeIR *ir = co->_co_ir;
        if (ir == NULL) {
            ir = _PyIR_LowerCode(co);
            if (ir != NULL) {
                co->_co_ir = ir;
                co->co_framesize = (int)ir->nslots + FRAME_SPECIALS_SIZE;
            }
        }
        exec = ir == NULL ? NULL : make_exec(co, ir);
        if (exec != NULL) {
            FT_ATOMIC_STORE_PTR_RELEASE(co->_co_exec, exec);
        }
    }
    Py_END_CRITICAL_SECTION();
    return exec;
}

int
_PyCode_OffsetWord(PyCodeObject *co, int offset)
{
    const _PyCodeIR *ir = _PyCode_IR(co);
    uint32_t unit = (uint32_t)offset / sizeof(_Py_CODEUNIT);
    // The first word whose instruction is at or after the unit
    uint32_t lo = 0, hi = ir->nwords;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (ir->pcmap[mid] < unit) {
            lo = mid + 1;
        }
        else {
            hi = mid;
        }
    }
    return (int)lo;
}

const _PyCodeIR *
_PyCode_GetIR(PyCodeObject *co)
{
    if (_PyCode_GetExec(co) == NULL) {
        return NULL;
    }
    return co->_co_ir;
}

void
_PyCode_ClearExec(PyCodeObject *co)
{
    /* Only code_dealloc frees the executing copy: a running frame holds a
     * reference to its code object, so frame->ir_words never changes while
     * the frame runs (pyjit/design/register-ir.md 1.10). */
    assert(Py_REFCNT(co) == 0);
    _PyCodeExec *exec = co->_co_exec;
    if (exec == &_PyIR_InitCleanupExec) {
        return;
    }
    if (exec != NULL) {
        co->_co_exec = NULL;
        _PyJit_FreeUnits(exec);
        char *base = (char *)exec;
        if (co->_co_ir->nfeedback) {
            _PyFbBlock *block = _PyIR_FbBlock(exec->words);
            _PyFb_Clear(block);
            base = (char *)(block + 1) - block->size;
        }
        _PyMem_NoScanFree(base);
    }
    _PyIR_Free(co->_co_ir);
    co->_co_ir = NULL;
}

size_t
_PyCode_ExecSizeOf(PyCodeObject *co)
{
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return 0;
    }
    size_t res = sizeof(_PyCodeExec) + exec->nwords * sizeof(_PyIRWord);
    res += co->_co_ir->nbytes;
    res += (size_t)(exec->nconst + (exec->nconst & 1)) * sizeof(PyValueBorrow);
    if (co->_co_ir->nfeedback) {
        _PyFbBlock *feedback = _PyIR_FbBlock(exec->words);
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

uint8_t
_PyIR_ShadowRetirement(_PyInterpreterFrame *frame, uint8_t reason,
                       const _PyIRCodePlan *plan)
{
    assert(reason <= _PyIR_RETIRE_STAGED_FAILURE);
    if (reason != _PyIR_RETIRE_NORMAL_RETURN || frame->f_executable == NULL) {
        return _PyIR_SHADOW_UNCOVERED;
    }
    PyCodeObject *co = frame->f_executable;
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->_co_exec);
    if (exec == NULL || plan == NULL || frame->ir_words != exec->words)
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
    if (_PyIR_FormatOf(IR_HANDLER(_PyCode_IRWords(co)[site]))->transfer_kind !=
        _PyIR_TRANSFER_EXIT)
    {
        return _PyIR_SHADOW_UNCOVERED;
    }
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
    for (int home = _PyCode_IRFixed(co);
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
    if (exec == NULL || frame->base == NULL ||
        frame->ir_words != exec->words ||
        exec->nwords != _PyCode_IR(co)->nwords)
    {
        return NULL;
    }
    int nl = _PyCode_IRFixed(co);
    int nslots = (int)_PyCode_IR(co)->nslots;
    if (exec->nfixed != nl || nl < 0 || nslots <= nl || nslots > IR_MAX_SLOTS ||
        _PyFrame_NumSlotsForCodeObject(co) != nslots)
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
    _PyIRWord word = _PyCode_IRWords(co)[continuation - 1];
    int handler = IR_HANDLER(word);
    if (handler != ROP_HANDLER_YieldFrom_Delegated &&
        handler != ROP_HANDLER_YieldFrom_Delegated_Observed &&
        handler != ROP_HANDLER_YieldFrom_Observed)
    {
        return -1;
    }
    // The word's value (A) and the sent value (B) are the value home of the
    // SEND's window, which the receiver precedes by the cursor
    int outgoing = (int)IR_A(word), dst = (int)IR_B(word);
    if (dst == IR_NO_DST) {
        dst = _PyFrame_DiscardSlotForCodeObject(co);
    }
    int receiver = dst - 2;
    if (dst != frame->return_slot || outgoing != dst ||
        receiver < _PyCode_IRFixed(co) ||
        dst >= _PyFrame_NumSlotsForCodeObject(co))
    {
        return -1;
    }
    return receiver;
}
