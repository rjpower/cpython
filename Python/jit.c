/* The baseline JIT: units, warm-up and frame switches
 * (Include/internal/pycore_jit.h).
 *
 * A unit is a copy of each head word's stencil, the interpreter's own arm
 * compiled once more with the word's fields and the arm's exits as holes
 * (Python/jit_stencils.c), in word order, with the holes patched
 * (bind_hole()).  The copies run in the evaluator's tail-call convention on
 * the driver's frame; code memory is Python/jit_memory.c's. */

#include "Python.h"
#include "pycore_ceval.h"           // _PyEvalActivation
#include "pycore_initconfig.h"      // _Py_GetEnv()
#include "pycore_interpframe.h"     // _PyFrame_Exec()
#include "pycore_ir.h"
#include "pycore_jit.h"
#include "pycore_jit_unwind.h"
#include "pycore_pystate.h"         // _PyInterpreterState_GET()
#include "pycore_rop_metadata.h"    // ROP_HANDLER_Resume_Warm

#include <ctype.h>                  // isdigit()
#include <errno.h>
#include <wchar.h>

/* The mode named by value, or -1.  Upstream's PYTHON_JIT=0 and 1 are off
 * and on. */
static int
mode_named(const char *value)
{
    if (strcmp(value, "off") == 0 || strcmp(value, "0") == 0) {
        return _PyJIT_OFF;
    }
    if (strcmp(value, "on") == 0 || strcmp(value, "1") == 0) {
        return _PyJIT_ON;
    }
    if (strcmp(value, "always") == 0) {
        return _PyJIT_ALWAYS;
    }
    return -1;
}

PyStatus
_PyJit_InitInterpreter(PyInterpreterState *interp)
{
    const PyConfig *config = &interp->config;
    const wchar_t *option = _Py_get_xoption(&config->xoptions, L"jit");
    int mode = _PyJIT_OFF;
    if (option != NULL) {
        const wchar_t *sep = wcschr(option, L'=');
        char value[8] = "";
        if (sep != NULL && wcslen(sep + 1) < sizeof(value)) {
            for (size_t i = 0; sep[1 + i] != L'\0'; i++) {
                value[i] = sep[1 + i] < 128 ? (char)sep[1 + i] : '?';
            }
        }
        mode = mode_named(value);
        if (mode < 0) {
            return _PyStatus_ERR("-X jit: expected off, on or always");
        }
    }
    else {
        const char *env = _Py_GetEnv(config->use_environment, "PYTHON_JIT");
        if (env != NULL) {
            mode = mode_named(env);
            if (mode < 0) {
                return _PyStatus_ERR("PYTHON_JIT: expected off, on, always, 0 or 1");
            }
        }
    }
    /* -X ir_forms, a test of the function forms, runs no unit: the JIT is
     * off under it */
    if (mode != _PyJIT_OFF &&
        (interp->ceval.ir_forms || !_PyJit_IsAvailable())) {
        mode = _PyJIT_OFF;
    }
    interp->ceval.jit = mode;
    interp->ceval.jit_warmup = mode == _PyJIT_ALWAYS ? 1 : _PyJIT_WARMUP;
    /* PYTHON_JIT_WARMUP=N: the warm-up events before a code compiles under
     * on, from startup (always compiles at the first event) */
    const char *warmup = _Py_GetEnv(config->use_environment, "PYTHON_JIT_WARMUP");
    if (warmup != NULL) {
        char *end = NULL;
        errno = 0;
        unsigned long n = isdigit((unsigned char)warmup[0])
                          ? strtoul(warmup, &end, 10) : 0;
        if (end == NULL || *end != '\0' || errno != 0 || n == 0 || n > UINT32_MAX) {
            return _PyStatus_ERR("PYTHON_JIT_WARMUP: expected an integer in 1..2**32-1");
        }
        if (mode == _PyJIT_ON) {
            interp->ceval.jit_warmup = (uint32_t)n;
        }
    }
    return _PyStatus_OK();
}

#ifndef _PY_JIT_SUPPORTED

int
_PyJit_IsAvailable(void)
{
    return 0;
}

#else

int
_PyJit_IsAvailable(void)
{
    return _PyJitMemory_Init() == 0;
}

/* The baseline JIT's word copies: the interpreter's arms, compiled once more
 * as stencils (Python/jit_stencils.c, Tools/jit/stencils.py) */
#include "ceval_tail.h"             // _PyEval_JitDispatch() and the other exits
#include "generated_jit_stencils.h" // _PyJit_Stencils, the support image

/* The static target of the word at i, a head word, or -1 */
static int
static_target(const _PyIRWord *words, uint32_t i)
{
    _PyIRWord w = words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(w));
    if (fmt->flags & _PyIR_NAMES_TARGET) {
        return -1;
    }
    if (fmt->flags & _PyIR_JUMPS) {
        return (int)IR_CD(w);
    }
    if (fmt->flags & _PyIR_FORWARD) {
        return (int)i + 1 + _PyIR_FormatDistance(fmt, w);
    }
    return -1;
}

int
_PyJit_GenericOf(int handler)
{
    int generic = _PyJit_Stencils[handler].generic;
    /* A word that the family's generic arm names runs that arm */
    return generic >= 0 ? generic : handler;
}

/* The call or jump rel32 whose field is at p: e8, e9 or 0f 8x before it */
static int
is_branch_at(const uint8_t *p)
{
    return p[-1] == 0xE8 || p[-1] == 0xE9 || (p[-2] == 0x0F && (p[-1] & 0xF0) == 0x80);
}

/* Write hole h of a part copied to base, bound to S.  Returns -1 if a rel32
 * does not reach or an immediate does not fit. */
static int
patch(uint8_t *base, const _PyJitHole *h, uintptr_t S)
{
    uint8_t *at = base + h->offset;
    int64_t value;
    switch (h->patch) {
        case _PyJit_PATCH_REL32:
            value = (int64_t)(S - (uintptr_t)at) + h->addend;
            if (value != (int32_t)value) {
                return -1;
            }
            break;
        case _PyJit_PATCH_VALUE32:
            value = (int32_t)(uint32_t)(S + (uintptr_t)(intptr_t)h->addend - (uintptr_t)at);
            break;
        case _PyJit_PATCH_IMM32:
            value = (int64_t)S;
            if (value != (int32_t)value) {
                return -1;
            }
            break;
        case _PyJit_PATCH_ABS64: {
            uint64_t v = (uint64_t)S + (uint64_t)(int64_t)h->addend;
            memcpy(at, &v, sizeof(v));
            return 0;
        }
        default:
            Py_UNREACHABLE();
    }
    int32_t v32 = (int32_t)value;
    memcpy(at, &v32, sizeof(v32));
    return 0;
}

/* The support image of the stencils, copied once per process into the JIT's
 * region (support_init()), so that every copy reaches it by rel32, and the
 * thread-local offsets of the TPOFF holes */
static struct {
    PyMutex lock;
    int state;                      // 0 not tried, 1 ready, -1 failed
    _PyJitCode code;
    _PyJitUnwindEntry unwind;
    intptr_t tpoff[_PyJit_STENCIL_TLS_COUNT + 1];
} support;

/* The thread pointer, which the TPOFF holes count from */
static uintptr_t
thread_pointer(void)
{
    uintptr_t tp;
    __asm__("mov %%fs:0, %0" : "=r"(tp));
    return tp;
}

/* The address of PLT entry of symbol, or 0 if the image has none */
static uintptr_t
plt_entry(uint16_t symbol)
{
    int32_t at = _PyJit_StencilPlt[symbol];
    return at < 0 ? 0 : (uintptr_t)support.code.start + (uintptr_t)at;
}

/* Bind an EXTERN hole of a part at base: the symbol, or for a call or a jump
 * that does not reach it, the symbol's PLT entry.  Returns -1 if neither
 * reaches; *via_plt counts the PLT's uses. */
static int
patch_extern(uint8_t *base, const _PyJitHole *h, uint64_t *via_plt)
{
    uintptr_t S = (uintptr_t)_PyJit_StencilSymbols[h->symbol];
    if (patch(base, h, S) == 0) {
        return 0;
    }
    uintptr_t plt = plt_entry(h->symbol);
    if (h->patch != _PyJit_PATCH_REL32 || plt == 0 || !is_branch_at(base + h->offset)) {
        return -1;
    }
    (*via_plt)++;
    return patch(base, h, plt);
}

/* One part's rows of call frame information, from the stencil's own, at
 * offset base of the code, up to limit bytes of it (a trimmed copy).  Each
 * part's rows start from the CIE's rule; the registers that the parts
 * before left saved are restored first.  saved is the set of those, by
 * DWARF number. */
static void
part_rows(_PyJitUnwind *u, size_t *at, uint32_t *saved, size_t base,
          const _PyJitPart *part, uint32_t limit)
{
    if (part->ncfa == 0) {
        return;
    }
    assert(base >= *at);
    _PyJitUnwind_Advance(u, (uint32_t)(base - *at));
    *at = base;
    for (unsigned reg = 0; reg < 32; reg++) {
        /* The return address's rule is the CIE's, which the part restates */
        if (reg != DWRF_REG_RA && (*saved & (1u << reg))) {
            _PyJitUnwind_Restore(u, reg);
        }
    }
    *saved = 0;
    for (uint16_t k = 0; k < part->ncfa; k++) {
        const _PyJitCfaOp *op = &part->cfa[k];
        if (op->pc >= limit) {
            break;
        }
        _PyJitUnwind_Advance(u, (uint32_t)(base + op->pc - *at));
        *at = base + op->pc;
        switch (op->op) {
            case DWRF_CFA_def_cfa:
                _PyJitUnwind_DefCfa(u, op->reg, (uint32_t)op->value);
                break;
            case DWRF_CFA_def_cfa_register:
                _PyJitUnwind_DefCfaRegister(u, op->reg);
                break;
            case DWRF_CFA_def_cfa_offset:
                _PyJitUnwind_DefCfaOffset(u, (uint32_t)op->value);
                break;
            case DWRF_CFA_offset:
                _PyJitUnwind_Offset(u, op->reg, (uint32_t)(-op->value / 8));
                *saved |= 1u << op->reg;
                break;
            case DWRF_CFA_restore:
                _PyJitUnwind_Restore(u, op->reg);
                *saved &= ~(1u << op->reg);
                break;
            case 0x0A:
                _PyJitUnwind_RememberState(u);
                break;
            case 0x0B:
                _PyJitUnwind_RestoreState(u);
                break;
            default:
                Py_UNREACHABLE();
        }
    }
}

/* The bytes of a frame for parts with ncfa ops in all */
static size_t
frame_bound(size_t nparts, size_t ncfa)
{
    return 128 + nparts * 24 + ncfa * 16;
}

static int
support_init_locked(void)
{
    const _PyJitPart *part = &_PyJit_SupportPart;
    size_t frame_offset = _Py_SIZE_ROUND_UP(part->size, 8);
    size_t bound = frame_bound(1, part->ncfa);
    uint8_t *frame = PyMem_RawMalloc(bound);
    if (frame == NULL) {
        return -1;
    }
    _PyJitUnwind u;
    size_t at = 0;
    uint32_t saved = 0;
    _PyJitUnwind_Begin(&u, frame, part->size);
    part_rows(&u, &at, &saved, 0, part, part->size);
    _PyJitUnwind_End(&u);
    size_t frame_size = _PyJitUnwind_Terminate(&u);
    assert(frame_size <= bound);
    if (_PyJitMemory_Alloc(frame_offset + frame_size, &support.code) < 0) {
        PyMem_RawFree(frame);
        return -1;
    }
    uint8_t *base = support.code.start;
    memcpy(base, part->bytes, part->size);
    memcpy(base + frame_offset, frame, frame_size);
    PyMem_RawFree(frame);
    uintptr_t tp = thread_pointer();
    for (int i = 0; i < _PyJit_STENCIL_TLS_COUNT; i++) {
        support.tpoff[i] = (intptr_t)((uintptr_t)_PyJit_StencilTls(i) - tp);
    }
    uint64_t via_plt = 0;
    for (int k = 0; k < _PyJit_SUPPORT_HOLES; k++) {
        const _PyJitHole *h = &_PyJit_SupportHoles[k];
        int rc;
        if (h->kind == _PyJit_HOLE_EXTERN) {
            rc = patch_extern(base, h, &via_plt);
        }
        else {
            assert(h->kind == _PyJit_HOLE_TPOFF);
            rc = patch(base, h, (uintptr_t)support.tpoff[h->symbol]);
        }
        if (rc < 0) {
            _PyJitMemory_Free(&support.code);
            return -1;
        }
    }
    if (_PyJitMemory_Seal(&support.code) < 0) {
        _PyJitMemory_Free(&support.code);
        return -1;
    }
    _PyJitUnwind_Register(&support.unwind, &support.code, frame_offset,
                          "py::jit:support");
    return 0;
}

/* Copy the support image, once per process; returns 0 when it is there */
static int
support_init(void)
{
    PyMutex_Lock(&support.lock);
    if (support.state == 0) {
        support.state = support_init_locked() == 0 ? 1 : -1;
    }
    int ok = support.state;
    PyMutex_Unlock(&support.lock);
    return ok > 0 ? 0 : -1;
}

/* Where each head word's copy goes in the unit, by word index */
typedef struct {
    const _PyJitStencil *stencil;   // NULL: the word has no copy
    uint32_t hot;                   // offsets in the block
    uint32_t cold;
    uint32_t data;
    uint32_t hot_size;              // the hot part as copied (trimmed)
} word_copy;

/* The hot part of stencil ends in jmp _JIT_CONTINUE: a copy that the next
 * word's copy follows leaves the jump out */
static int
ends_in_continue(const _PyJitStencil *st)
{
    const _PyJitPart *hot = &st->parts[_PyJit_PART_HOT];
    for (uint16_t k = 0; k < st->nholes; k++) {
        const _PyJitHole *h = &st->holes[k];
        if (h->part == _PyJit_PART_HOT && h->kind == _PyJit_HOLE_CONTINUE &&
            h->offset + 4 == hot->size && hot->bytes[h->offset - 1] == 0xE9)
        {
            return 1;
        }
    }
    return 0;
}

/* The state of one unit's compilation */
typedef struct {
    const _PyIRWord *words;
    uint32_t nwords;
    word_copy *copies;
    uint8_t *base;                  // the block
    uint64_t via_plt;
} _PyJitCompiler;

/* The address of word j's copy, or 0 if it has none */
static uintptr_t
copy_of(const _PyJitCompiler *c, int64_t j)
{
    if (j < 0 || j >= c->nwords || c->copies[j].stencil == NULL) {
        return 0;
    }
    return (uintptr_t)(c->base + c->copies[j].hot);
}

/* What hole h of word i's copy is bound to (the plan's section 2): the
 * word's fields, the copies of its successors, the support image and
 * libpython, and the exits into the normal evaluator
 * (Python/ceval_evaluator.h), each of which closes the departure window.
 * Returns 0 for a hole that patch_extern() binds. */
static uintptr_t
bind_hole(const _PyJitCompiler *c, uint32_t i, const _PyJitHole *h)
{
    _PyIRWord w = c->words[i];
    const word_copy *wc = &c->copies[i];
    uintptr_t dispatch = (uintptr_t)_PyEval_JitDispatch;
    switch ((_PyJitHoleKind)h->kind) {
        case _PyJit_HOLE_FIELD_A: return IR_A(w);
        case _PyJit_HOLE_FIELD_B: return IR_B(w);
        case _PyJit_HOLE_FIELD_C: return IR_C(w);
        case _PyJit_HOLE_FIELD_D: return IR_D(w);
        case _PyJit_HOLE_CONTINUE: {
            uintptr_t next = copy_of(c, (int64_t)i + _PyIR_WordLength(w));
            return next ? next : dispatch;
        }
        case _PyJit_HOLE_TARGET: {
            uintptr_t target = copy_of(c, static_target(c->words, i));
            return target ? target : dispatch;
        }
        case _PyJit_HOLE_SWITCH: return dispatch;
        case _PyJit_HOLE_REDIRECT:
        case _PyJit_HOLE_AGAIN: return (uintptr_t)_PyEval_JitInterpret;
        case _PyJit_HOLE_MISS: return (uintptr_t)_PyEval_JitMiss;
        case _PyJit_HOLE_PROCESS_ERROR: return (uintptr_t)_PyEval_JitProcessError;
        case _PyJit_HOLE_CLOSE_DISPATCH: return (uintptr_t)_PyEval_JitCloseDispatch;
        case _PyJit_HOLE_GEN_RETURN: return (uintptr_t)_PyEval_JitGenReturn;
        case _PyJit_HOLE_EXTERN: return 0;
        case _PyJit_HOLE_TPOFF: return (uintptr_t)support.tpoff[h->symbol];
        case _PyJit_HOLE_PART: {
            uint32_t at = h->target == _PyJit_PART_HOT ? wc->hot
                        : h->target == _PyJit_PART_COLD ? wc->cold : wc->data;
            return (uintptr_t)(c->base + at);
        }
        case _PyJit_HOLE_SUPPORT: return (uintptr_t)support.code.start;
    }
    Py_UNREACHABLE();
}

/* Copy word i's parts into the block and patch their holes; -1 if a hole
 * does not reach */
static int
copy_word(_PyJitCompiler *c, uint32_t i)
{
    const word_copy *wc = &c->copies[i];
    const _PyJitStencil *st = wc->stencil;
    uint8_t *at[3] = {c->base + wc->hot, c->base + wc->cold, c->base + wc->data};
    memcpy(at[_PyJit_PART_HOT], st->parts[_PyJit_PART_HOT].bytes, wc->hot_size);
    for (int p = _PyJit_PART_COLD; p <= _PyJit_PART_DATA; p++) {
        if (st->parts[p].bytes != NULL) {
            memcpy(at[p], st->parts[p].bytes, st->parts[p].size);
        }
    }
    for (uint16_t k = 0; k < st->nholes; k++) {
        const _PyJitHole *h = &st->holes[k];
        if (h->part == _PyJit_PART_HOT && h->offset >= wc->hot_size) {
            continue;       // the left-out jump to the next copy
        }
        int rc = h->kind == _PyJit_HOLE_EXTERN
            ? patch_extern(at[h->part], h, &c->via_plt)
            : patch(at[h->part], h, bind_hole(c, i, h));
        if (rc < 0) {
            return -1;
        }
    }
    return 0;
}

/* The executing copy whose words unit compiled: its header lies before
 * them (_PyFrame_Exec()) */
static _PyCodeExec *
exec_of(const _PyJitUnit *unit)
{
    return (_PyCodeExec *)unit->words - 1;
}

/* Rewrite every head of exec whose handler is from to the handler to, which
 * has the same format; returns how many.  Resume.Warm and Resume.Poll are
 * the only pair: a code lowered for the JIT has Resume.Warm heads (the frame
 * start and each yield-from loop's resume), and no specialisation rewrites
 * them. */
static int
rewrite_heads(_PyCodeExec *exec, int from, int to)
{
    int n = 0;
    for (uint32_t i = 0; i < exec->nwords; i += _PyIR_WordLength(exec->words[i])) {
        if (IR_HANDLER(exec->words[i]) == from) {
            _PyIR_SetHandler(&exec->words[i], to);
            n++;
        }
    }
    return n;
}

/* Every frame switch to the words of exec goes through the entries of its
 * valid newest unit, which a compile or a stale unit's reprieve at the cap
 * publishes.  While they are published, the code's Resume.Warm heads are
 * Resume.Poll: the code has nothing left to warm up, so a frame start and a
 * delegation chain skip them as they skip any Resume.Poll that would do
 * nothing (_PyRop_StartSkips()), and the start goes on through the entries
 * (IR_JIT_SWITCH() in IR_START_FRAME(), or the unit's own switch). */
static void
publish(_PyCodeExec *exec, _PyJitUnit *unit)
{
    assert(exec->jit_unit == unit && unit->invalid == 0);
    if (unit->warm_heads) {
        rewrite_heads(exec, ROP_HANDLER_Resume_Warm, ROP_HANDLER_Resume_Poll);
    }
    exec->jit_entries = unit->entries;
}

/* The newest unit of exec is invalid (retired, stale) or replaced: switches
 * to its words go to the driver, which decides by the warm-up event
 * (_PyJit_Event()), and its heads are Resume.Warm again, so that frame
 * starts count warm-up events and recompile. */
static void
unpublish(_PyCodeExec *exec)
{
    if (exec->jit_entries != NULL && exec->jit_unit->warm_heads) {
        rewrite_heads(exec, ROP_HANDLER_Resume_Poll, ROP_HANDLER_Resume_Warm);
    }
    exec->jit_entries = NULL;
}

/* STALE into the unit's invalid byte, RETIRED kept, and the newest unit's
 * entries unpublished: the next switch to the code recompiles it */
static void
mark_stale(_PyJitUnit *unit)
{
    unit->invalid |= _PyJIT_STALE;
    _PyCodeExec *exec = exec_of(unit);
    if (exec->jit_unit == unit) {
        unpublish(exec);
    }
}

/* The bytes that start every copy: the stencils are functions, and GCC opens
 * each with endbr64 */
static const uint8_t ENDBR64[4] = {0xF3, 0x0F, 0x1E, 0xFA};

/* Send each entry to word i's copy in unit to the evaluator: a jmp to
 * _PyEval_JitInterpret(), which runs the word under the arm that it names,
 * over the copy's first instruction after its endbr64.  Every way into the
 * copy (an entry, the copy before it, a branch) arrives at its start with
 * next_instr at the word.  A thread that is inside the copy, at a call that
 * changed the word, returns to a later instruction: no branch of a stencil
 * goes to the bytes that the jmp takes (test_jit_stencils). */
static void
divert_copy(_PyJitUnit *unit, uint32_t i)
{
    uint8_t *start = (uint8_t *)unit->entries[i];
    assert(start >= unit->code.start && start < unit->code.start + unit->code.size);
    assert(memcmp(start, ENDBR64, sizeof(ENDBR64)) == 0);
    uint8_t *at = start + sizeof(ENDBR64);
    uint8_t jmp[5] = {0xE9};
    int64_t rel = (int64_t)((uintptr_t)_PyEval_JitInterpret - (uintptr_t)(at + 5));
    /* The unit's exits reach the evaluator by rel32 (patch()) */
    assert(rel == (int32_t)rel);
    int32_t rel32 = (int32_t)rel;
    memcpy(jmp + 1, &rel32, sizeof(rel32));
    if (memcmp(at, jmp, sizeof(jmp)) == 0) {
        return;
    }
    if (_PyJitMemory_Patch(at, jmp, sizeof(jmp)) < 0) {
        Py_FatalError("baseline JIT: a changed word's copy cannot be diverted");
    }
}

int
_PyJit_IsDiverted(const _PyJitUnit *unit, uint32_t i)
{
    if (unit->arms[i] == 0) {
        return 0;
    }
    const uint8_t *at = (const uint8_t *)unit->entries[i] + sizeof(ENDBR64);
    int32_t rel32;
    memcpy(&rel32, at + 1, sizeof(rel32));
    return at[0] == 0xE9 &&
           (uintptr_t)(at + 5) + (intptr_t)rel32 == (uintptr_t)_PyEval_JitInterpret;
}

/* Word instr of exec's code changed its arm (set_opcode() in
 * Python/specialize.c).  Every unit of the code compiled the old arm, whose
 * copy reads the feedback as the old arm left it: the copy goes to the
 * evaluator from now on, in the units that frames may still be running too.
 * The newest unit is stale and the next warm-up event recompiles the code.
 * A unit at the recompile cap is never marked: it runs on, its changed words
 * in the evaluator. */
void
_PyJit_MarkWordChanged(_PyCodeExec *exec, const _PyInstr *instr)
{
    assert(instr >= exec->words && instr < exec->words + exec->nwords);
    uint32_t i = (uint32_t)(instr - exec->words);
    for (_PyJitUnit *u = exec->jit_unit; u != NULL; u = u->older) {
        /* No arm is 0: a word without a copy (a gap) has nothing to divert */
        if (u->arms[i] != 0 && u->arms[i] != IR_HANDLER(*instr)) {
            divert_copy(u, i);
        }
    }
    _PyJitUnit *unit = exec->jit_unit;
    if (unit != NULL && !(unit->invalid & _PyJIT_STALE) &&
        exec->jit_compiles < _PyJIT_MAX_COMPILES)
    {
        mark_stale(unit);
    }
}

/* The plan that word w compiles to.  A unit at the recompile cap is never
 * marked or recompiled, so it plans nothing: each word keeps its handler
 * check and runs the live handler's form after the handler changes, as a
 * planned arm, which is compiled for one handler, cannot. */
const _PyIRStubPlan *
_PyJit_PlanOf(_PyIRWord w, int capped)
{
    static const _PyIRStubPlan none = {_PyIR_PLAN_NONE};
    return capped ? &none : &_PyIR_StubPlans[_PyJit_CompiledHandler(w)];
}


/* The unit's code: each head word's copy, in word order (hot parts, then
 * cold parts, then jump tables), the entry table, then the unwind info.
 * Fills unit->code, entries and the byte counts; returns the frame's offset
 * in the block, or 0 if the unit cannot be placed or a hole does not
 * reach. */
static size_t
emit_unit(_PyJitCompiler *c, _PyJitUnit *unit)
{
    const _PyIRWord *words = c->words;
    uint32_t nwords = c->nwords;
    size_t hot = 0, ncfa = 0, nparts = 0;
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        word_copy *wc = &c->copies[i];
        const _PyJitStencil *st = &_PyJit_Stencils[IR_HANDLER(words[i])];
        if (st->name == NULL) {
            /* A declared gap: holes and entries that name the word go to
             * the evaluator, which runs it */
            continue;
        }
        wc->stencil = st;
        unit->arms[i] = (uint16_t)IR_HANDLER(words[i]);
        wc->hot = (uint32_t)hot;
        wc->hot_size = st->parts[_PyJit_PART_HOT].size;
        uint32_t next = i + (uint32_t)_PyIR_WordLength(words[i]);
        if (next < nwords && _PyJit_Stencils[IR_HANDLER(words[next])].name != NULL &&
            ends_in_continue(st))
        {
            wc->hot_size -= 5;
        }
        hot += wc->hot_size;
        for (int p = _PyJit_PART_HOT; p <= _PyJit_PART_COLD; p++) {
            ncfa += st->parts[p].ncfa;
            nparts += st->parts[p].bytes != NULL;
        }
    }
    size_t cold = _Py_SIZE_ROUND_UP(hot, 16), cold_start = cold;
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        word_copy *wc = &c->copies[i];
        if (wc->stencil != NULL && wc->stencil->parts[_PyJit_PART_COLD].bytes != NULL) {
            wc->cold = (uint32_t)cold;
            cold += wc->stencil->parts[_PyJit_PART_COLD].size;
        }
    }
    size_t data = _Py_SIZE_ROUND_UP(cold, 4);
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        word_copy *wc = &c->copies[i];
        if (wc->stencil != NULL && wc->stencil->parts[_PyJit_PART_DATA].bytes != NULL) {
            data = _Py_SIZE_ROUND_UP(data, 4);
            wc->data = (uint32_t)data;
            data += wc->stencil->parts[_PyJit_PART_DATA].size;
        }
    }
    size_t entries = _Py_SIZE_ROUND_UP(data, 8);
    size_t code_size = entries + nwords * sizeof(void *);
    if (code_size > UINT32_MAX) {
        return 0;
    }

    /* The unwind info: every part's own rows, at its place */
    size_t bound = frame_bound(nparts, ncfa);
    uint8_t *frame = PyMem_RawMalloc(bound);
    if (frame == NULL) {
        return 0;
    }
    _PyJitUnwind u;
    size_t at = 0;
    uint32_t saved = 0;
    _PyJitUnwind_Begin(&u, frame, code_size);
    for (int p = _PyJit_PART_HOT; p <= _PyJit_PART_COLD; p++) {
        for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
            const word_copy *wc = &c->copies[i];
            if (wc->stencil == NULL || wc->stencil->parts[p].bytes == NULL) {
                continue;
            }
            const _PyJitPart *part = &wc->stencil->parts[p];
            if (p == _PyJit_PART_HOT) {
                part_rows(&u, &at, &saved, wc->hot, part, wc->hot_size);
            }
            else {
                part_rows(&u, &at, &saved, wc->cold, part, part->size);
            }
        }
    }
    _PyJitUnwind_End(&u);
    size_t frame_size = _PyJitUnwind_Terminate(&u);
    assert(frame_size <= bound);
    size_t frame_offset = _Py_SIZE_ROUND_UP(code_size, 8);

    if (_PyJitMemory_Alloc(frame_offset + frame_size, &unit->code) < 0) {
        PyMem_RawFree(frame);
        return 0;
    }
    c->base = unit->code.start;
    memset(c->base, 0xCC, frame_offset);
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        if (c->copies[i].stencil != NULL && copy_word(c, i) < 0) {
            PyMem_RawFree(frame);
            _PyJitMemory_Free(&unit->code);
            return 0;
        }
    }
    const void **table = (const void **)(c->base + entries);
    for (uint32_t i = 0; i < nwords; i++) {
        table[i] = (const void *)_PyEval_JitBadEntry;
    }
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        uintptr_t copy = copy_of(c, i);
        table[i] = copy ? (const void *)copy : (const void *)_PyEval_JitInterpret;
    }
    memcpy(c->base + frame_offset, frame, frame_size);
    PyMem_RawFree(frame);
    if (_PyJitMemory_Seal(&unit->code) < 0) {
        _PyJitMemory_Free(&unit->code);
        return 0;
    }
    unit->entries = (const void *const *)table;
    unit->hot_bytes = (uint32_t)hot;
    unit->cold_bytes = (uint32_t)(data - cold_start);
    return frame_offset;
}

static void
free_unit(_PyJitUnit *unit)
{
    /* The unwinder reads the frame to forget it: deregister before freeing */
    _PyJitUnwind_Deregister(&unit->unwind);
    _PyJitMemory_Free(&unit->code);
    PyMem_RawFree(unit->arms);
    PyMem_RawFree(unit);
}

/* Compile the words of exec, the executing copy of code, into a new unit
 * and make it the exec's.  Returns NULL if the code is observed, which runs
 * in the observing evaluator, or cannot be placed. */
static _PyJitUnit *
compile(PyCodeObject *code, _PyCodeExec *exec)
{
    if (_PyCode_IsObserved(code)) {
        return NULL;
    }
    /* Units are made only here, so draining here bounds the dead list by
     * the compile rate */
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->ceval.jit_dead != NULL) {
        _PyJit_Reclaim(interp);
    }
    if (support_init() < 0) {
        /* No code memory, or the support image does not reach: no unit
         * will compile */
        exec->jit_compiles = _PyJIT_MAX_COMPILES;
        return NULL;
    }
    const _PyIRWord *words = exec->words;
    uint32_t nwords = exec->nwords;
    _PyJitUnit *unit = PyMem_RawCalloc(1, sizeof(_PyJitUnit));
    word_copy *copies = PyMem_RawCalloc(nwords, sizeof(word_copy));
    uint16_t *arms = PyMem_RawCalloc(nwords, sizeof(uint16_t));
    if (unit == NULL || copies == NULL || arms == NULL) {
        PyMem_RawFree(unit);
        PyMem_RawFree(copies);
        PyMem_RawFree(arms);
        return NULL;
    }
    unit->arms = arms;
    unit->nwords = nwords;
    unit->words = words;
    /* The unit compiles the heads as Resume.Poll, as they are while its
     * entries are published; a live unit that this compile replaces gives
     * its heads back first */
    _PyJitUnit *live = exec->jit_entries != NULL ? exec->jit_unit : NULL;
    unpublish(exec);
    unit->warm_heads = rewrite_heads(exec, ROP_HANDLER_Resume_Warm,
                                     ROP_HANDLER_Resume_Poll) > 0;
    _PyJitCompiler c = {.words = words, .nwords = nwords, .copies = copies};
    size_t frame_offset = emit_unit(&c, unit);
    PyMem_RawFree(copies);
    exec->jit_compiles++;
    interp->ceval.jit_compiled_codes += exec->jit_compiles == 1;
    interp->ceval.jit_compiles++;
    interp->ceval.jit_compiled_words += nwords;
    if (frame_offset == 0) {
        /* Out of code memory, or a hole that does not reach: do not try
         * this code again */
        exec->jit_compiles = _PyJIT_MAX_COMPILES;
        if (unit->warm_heads) {
            rewrite_heads(exec, ROP_HANDLER_Resume_Poll, ROP_HANDLER_Resume_Warm);
        }
        if (live != NULL) {
            publish(exec, live);
        }
        PyMem_RawFree(unit->arms);
        PyMem_RawFree(unit);
        return NULL;
    }
    interp->ceval.jit_hot_bytes += unit->hot_bytes;
    interp->ceval.jit_cold_bytes += unit->cold_bytes;
    interp->ceval.jit_plt_holes += c.via_plt;
    unit->older = exec->jit_unit;
    exec->jit_unit = unit;
    publish(exec, unit);
    /* The unit's name for gdb and for the perf map */
    char name[512] = "py::jit";
    PyObject *exc = PyErr_GetRaisedException();
    const char *qualname = PyUnicode_AsUTF8(code->co_qualname);
    const char *filename = PyUnicode_AsUTF8(code->co_filename);
    if (qualname != NULL && filename != NULL) {
        PyOS_snprintf(name, sizeof(name), "py::jit:%s:%s", qualname, filename);
    }
    PyErr_Clear();
    PyErr_SetRaisedException(exc);
    _PyJitUnwind_Register(&unit->unwind, &unit->code, frame_offset, name);
    if (interp->config.perf_profiling == 1) {
        PyUnstable_WritePerfMapEntry(unit->code.start,
                                     (unsigned int)frame_offset, name);
    }
    return unit;
}

_PyJitUnit *
_PyJit_Event(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyCodeExec *exec = _PyFrame_Exec(frame);
    _PyJitUnit *unit = exec->jit_unit;
    if (unit != NULL && unit->invalid == 0) {
        return unit;
    }
    if (exec == &_PyIR_InitCleanupExec ||
        tstate->interp->ceval.jit == _PyJIT_OFF) {
        /* The static exec is shared by every interpreter and never freed;
         * a code lowered with the JIT on may run in an interpreter that has
         * it off. */
        return NULL;
    }
    if (unit != NULL && unit->invalid == _PyJIT_STALE) {
        if (exec->jit_compiles >= _PyJIT_MAX_COMPILES) {
            unit->invalid = 0;
            publish(exec, unit);
            return unit;
        }
    }
    else {
        if (++exec->jit_heat < tstate->interp->ceval.jit_warmup ||
            exec->jit_compiles >= _PyJIT_MAX_COMPILES) {
            return NULL;
        }
    }
    exec->jit_heat = 0;
    return compile(_PyFrame_GetCode(frame), exec);
}

_PyJitUnit *
_PyJit_Compile(PyCodeObject *code)
{
    _PyCodeExec *exec = _PyCode_GetExec(code);
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return NULL;
    }
    return compile(code, exec);
}

void
_PyJit_Retire(_PyCodeExec *exec)
{
    if (exec->jit_unit != NULL) {
        exec->jit_unit->invalid |= _PyJIT_RETIRED;
        unpublish(exec);
    }
}

static void
free_chain(_PyJitUnit *unit)
{
    while (unit != NULL) {
        _PyJitUnit *older = unit->older;
        free_unit(unit);
        unit = older;
    }
}

/* Whether a thread of interp is in a departure window: its flag, or the
 * flag that one of its activations saved, is set.  The caller holds the
 * GIL, so every other thread is stopped where its flag is published: a
 * thread sets it before it can release the GIL and clears it only while
 * holding the GIL. */
static bool
in_departure_window(PyInterpreterState *interp)
{
    bool found = false;
    HEAD_LOCK(interp->runtime);
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        _PyThreadStateImpl *impl = (_PyThreadStateImpl *)ts;
        found = impl->jit_departing;
        for (_PyEvalActivation *a = impl->eval_activation;
             a != NULL && !found; a = a->previous)
        {
            found = a->jit_departing;
        }
        if (found) {
            break;
        }
    }
    HEAD_UNLOCK(interp->runtime);
    return found;
}

void
_PyJit_Reclaim(PyInterpreterState *interp)
{
    _PyJitUnit *dead = interp->ceval.jit_dead;
    if (dead == NULL || in_departure_window(interp)) {
        return;
    }
    interp->ceval.jit_dead = NULL;
    free_chain(dead);
}

void
_PyJit_FreeUnits(_PyCodeExec *exec)
{
    _PyJitUnit *unit = exec->jit_unit;
    /* The words go with the exec: no head to give back */
    exec->jit_entries = NULL;
    exec->jit_unit = NULL;
    if (unit == NULL) {
        return;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        /* No thread runs Python */
        free_chain(unit);
        return;
    }
    /* A thread in a departure window may still run one of them: the
     * whole chain waits on the dead list (_PyJit_Reclaim()) */
    _PyJitUnit *last = unit;
    while (last->older != NULL) {
        last = last->older;
    }
    PyInterpreterState *interp = tstate->interp;
    last->older = interp->ceval.jit_dead;
    interp->ceval.jit_dead = unit;
}

void
_PyJit_FiniInterpreter(PyInterpreterState *interp)
{
    free_chain(interp->ceval.jit_dead);
    interp->ceval.jit_dead = NULL;
}

#endif  /* _PY_JIT_SUPPORTED */
