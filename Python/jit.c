/* The baseline JIT: units, warm-up and the switches
 * (Include/internal/pycore_jit.h, pyjit/design/proposals/tiers87.md Phase 2).
 *
 * B1 compiles every word as a call of the function form of the handler that
 * the word names at compile time (ir_handler_forms in Python/ceval.c), and
 * continues natively when the form's transfer is the same frame and a word
 * known at compile time.  The machine code goes through the assembler and
 * code memory of Python/jit_asm.c and Python/jit_memory.c. */

#include "Python.h"
#include "pycore_initconfig.h"      // _Py_GetEnv()
#include "pycore_interpframe.h"     // _PyFrame_Exec()
#include "pycore_ir.h"
#include "pycore_jit.h"
#include "pycore_pystate.h"         // _PyInterpreterState_GET()

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
    if (mode != _PyJIT_OFF && !_PyJit_IsAvailable()) {
        mode = _PyJIT_OFF;
    }
    interp->ceval.jit = mode;
    interp->ceval.jit_warmup = mode == _PyJIT_ALWAYS ? 1 : _PyJIT_WARMUP;
    /* The JIT's activation loop is the forms executor */
    if (mode != _PyJIT_OFF) {
        interp->ceval.ir_forms = 1;
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

#include "pycore_jit_asm.h"
#include "pycore_jit_lower.h"       // _PyJit_Templates

int
_PyJit_IsAvailable(void)
{
    return _PyJitMemory_Init() == 0;
}

/* Where an operand word's entry leads (no transfer names one), and in a
 * debug build a frame whose base moved. */
static void
bad_entry(void)
{
    Py_FatalError("baseline JIT: an operand word entered, or a frame's base moved");
}

/* What compiled code tests after a word: RETIRED after a word that may run
 * Python (which may have enabled monitoring), anything at a backward jump,
 * where a stale unit is replaced. */
#define MAY_RUN_PYTHON \
    (_PyIR_ESCAPES | _PyIR_RUNS_PYTHON | _PyIR_DEOPTS | _PyIR_PUSHES_FRAME | \
     _PyIR_POLLS)

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
        return (int)i + 1 + IR_DS(w);
    }
    return -1;
}

#define R(X) _PyJitReg_##X
#define AT(B, D) _PyJitMem_At(R(B), (D))
#define WORD_AT(I) AT(R14, (int32_t)((I) * sizeof(_PyIRWord)))

/* The state of one unit's compilation: the words, a label per head word
 * (-1 for the operand words) and the unit's shared exits.  Every piece
 * below may be emitted anywhere in the code, hot or cold: none falls
 * through to what follows it. */
typedef struct {
    _PyJitAsm *a;
    PyCodeObject *code;
    _PyJitUnit *unit;
    const _PyIRWord *words;
    uint32_t nwords;
    _PyJitLabel *labels;
    _PyJitLabel leave;      // leave the unit with the transfer in rax:rdx
    _PyJitLabel backedge;   // a backward jump of an invalid unit, in its frame
    _PyJitLabel live;       // call the live handler of the word at rdx
    _PyJitLabel trap;       // an operand word's entry, a broken invariant
} _PyJitCompiler;

/* The entry, the shared exits and the stubs.  Registers at a word boundary:
 * r12 tstate, r13 frame, r14 word 0, r15 frame->base, rbx the unit; rsp is
 * 16-byte aligned. */
static void
emit_frame(_PyJitCompiler *c)
{
    _PyJitAsm *a = c->a;
    /* enter(tstate, frame, address of the word's block) */
    _PyJitAsm_Push(a, R(RBP));
    _PyJitAsm_MovRR(a, R(RBP), R(RSP));
    _PyJitAsm_Push(a, R(RBX));
    _PyJitAsm_Push(a, R(R12));
    _PyJitAsm_Push(a, R(R13));
    _PyJitAsm_Push(a, R(R14));
    _PyJitAsm_Push(a, R(R15));
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(R12), R(RDI));
    _PyJitAsm_MovRR(a, R(R13), R(RSI));
    _PyJitAsm_MovRM(a, R(R14), AT(R13, offsetof(_PyInterpreterFrame, ir_words)));
    _PyJitAsm_MovRM(a, R(R15), AT(R13, offsetof(_PyInterpreterFrame, base)));
    _PyJitAsm_MovImm(a, R(RBX), (int64_t)(uintptr_t)c->unit);
    _PyJitAsm_JmpR(a, R(RDX));

    _PyJitAsm_Bind(a, c->leave);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_Pop(a, R(R15));
    _PyJitAsm_Pop(a, R(R14));
    _PyJitAsm_Pop(a, R(R13));
    _PyJitAsm_Pop(a, R(R12));
    _PyJitAsm_Pop(a, R(RBX));
    _PyJitAsm_Pop(a, R(RBP));
    _PyJitAsm_Ret(a);

    /* The stubs are cold */
    _PyJitAsm_Section(a, _PyJitSection_COLD);

    /* A warm-up event, which may recompile; then leave with the transfer,
     * whose frame is the unit's. */
    _PyJitAsm_Bind(a, c->backedge);
    _PyJitAsm_Push(a, R(RDX));
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_CallAbs(a, (const void *)_PyJit_Event);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_Pop(a, R(RDX));
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_Jmp(a, c->leave);

    _PyJitAsm_Bind(a, c->trap);
    _PyJitAsm_CallAbs(a, (const void *)bad_entry);

    /* Called with rdx at a word whose handler is no longer the one compiled:
     * runs the word's live handler by its form, as the word's own call
     * would. */
    _PyJitAsm_Bind(a, c->live);
    _PyJitAsm_MovRM(a, R(RCX), AT(RDX, 0));
    _PyJitAsm_MovRR(a, R(RAX), R(RCX));
    _PyJitAsm_AluRI(a, _PyJitAlu_AND, R(RAX), IR_HANDLER_MASK);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_MovImm(a, R(R8), (int64_t)(uintptr_t)_PyEval_HandlerForms());
    _PyJitAsm_JmpM(a, _PyJitMem_Indexed(R(R8), R(RAX), 3, 0));
    _PyJitAsm_Section(a, _PyJitSection_HOT);
}

/* The continuation of word i, with its transfer in rax:rdx: the next word or
 * the word's static target in the same frame by a native jump, anything
 * else by leaving.  The unit's invalid byte is tested after a word that may
 * run Python (RETIRED: monitoring went on) and at a backward jump (either:
 * the backedge stub may recompile). */
static void
emit_continuation(_PyJitCompiler *c, uint32_t i)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(w));
    uint32_t next = i + (uint32_t)_PyIR_WordLength(w);
    /* Another frame: a call, a return, a yield, a resume */
    _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RAX), R(R13));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, c->leave);
    int target = static_target(c->words, i);
    if (target >= 0 && ((uint32_t)target >= c->nwords || c->labels[target] < 0)) {
        target = -1;
    }
    if (target >= 0 && (uint32_t)target <= i) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRR(a, R(RCX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, c->backedge);
    }
    else if (fmt->effects & MAY_RUN_PYTHON) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRI(a, R(RCX), _PyJIT_RETIRED);
        _PyJitAsm_Jcc(a, _PyJitCond_NE, c->leave);
    }
    if (target >= 0) {
        _PyJitAsm_Lea(a, R(RCX), WORD_AT((uint32_t)target));
        _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_E, c->labels[target]);
    }
    if (!(fmt->flags & _PyIR_JUMP_ALWAYS) && next < c->nwords) {
        _PyJitAsm_Lea(a, R(RCX), WORD_AT(next));
        _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_E, c->labels[next]);
    }
    _PyJitAsm_Jmp(a, c->leave);
}

/* Word i as a call of its handler's function form, with the word's address
 * and bits as immediates, then its continuation.  Specialisation rewrites a
 * word's handler and its feedback entry together, and a handler's guards are
 * only sound against the entry that its own specialisation wrote, so on a
 * word that specialisation can rewrite the compiled handler runs only while
 * the word still names it; otherwise the word's live handler runs.  B2's
 * slow path for a word is this code. */
static void
emit_form_word(_PyJitCompiler *c, uint32_t i)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    _PyJitLabel changed = _PyJitAsm_NewLabel(a);
    _PyJitLabel called = _PyJitAsm_NewLabel(a);
#ifdef Py_DEBUG
    /* base is fixed while a frame runs */
    _PyJitLabel base_ok = _PyJitAsm_NewLabel(a);
    _PyJitAsm_AluRM(a, _PyJitAlu_CMP, R(R15),
                    AT(R13, offsetof(_PyInterpreterFrame, base)));
    _PyJitAsm_Jcc(a, _PyJitCond_E, base_ok);
    _PyJitAsm_Jmp(a, c->trap);
    _PyJitAsm_Bind(a, base_ok);
#endif
    int guarded = _PyJit_HandlerMayChange(c->code, (int)i);
    if (guarded) {
        _PyJitAsm_MovzxRM16(a, R(RCX), WORD_AT(i));
        _PyJitAsm_AluRI(a, _PyJitAlu_CMP, R(RCX), (int32_t)(w & 0xFFFF));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, changed);
    }
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(i));
    _PyJitAsm_MovImm(a, R(RCX), (int64_t)w);
    _PyJitAsm_CallAbs(a, _PyEval_HandlerForms()[IR_HANDLER(w)]);
    _PyJitAsm_Bind(a, called);
    emit_continuation(c, i);
    if (!guarded) {
        return;
    }

    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    _PyJitAsm_Bind(a, changed);
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(i));
    _PyJitAsm_LeaLabel(a, R(RAX), c->live);
    _PyJitAsm_CallR(a, R(RAX));
    _PyJitAsm_Jmp(a, called);
    _PyJitAsm_Section(a, section);
}

/* Word i, a template's, goes on to word dest of its frame by a native jump
 * (dest > i: the next word or a forward target), with the unit's tests that
 * emit_continuation() makes.  The tests are in a stub at stub when the word
 * needs them; the hot jump to dest is the caller's.  A word whose dest has
 * no block leaves with the transfer. */
static void
emit_template_stub(_PyJitCompiler *c, uint32_t i, uint32_t dest, _PyJitLabel stub)
{
    _PyJitAsm *a = c->a;
    _PyJitAsm_Bind(a, stub);
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(dest));
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_Jmp(a, dest <= i ? c->backedge : c->leave);
}

/* The tests on the way from word i to dest: a backward jump tests the
 * invalid byte, the hot path of a word that may run Python tests RETIRED.
 * A set bit goes to stub. */
static void
emit_template_tests(_PyJitCompiler *c, uint32_t i, uint32_t dest, int may_run_python,
                    _PyJitLabel stub)
{
    _PyJitAsm *a = c->a;
    if (dest <= i) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRR(a, R(RCX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, stub);
    }
    else if (may_run_python) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRI(a, R(RCX), _PyJIT_RETIRED);
        _PyJitAsm_Jcc(a, _PyJitCond_NE, stub);
    }
}

static int
has_block(_PyJitCompiler *c, uint32_t dest)
{
    return dest < c->nwords && c->labels[dest] >= 0;
}

/* Word i by its handler's inline template (B2): the handler check of a
 * word that specialisation may rewrite, the template, and the exits.  The
 * template's hot path falls through to the next word, whose block follows,
 * or jumps to its static target; its failed guards go to the word as
 * emit_form_word() runs it, in the cold section.  The hot path does not run
 * the form, so it may run Python only through the effects that the handler
 * has besides its deopts. */
static void
emit_template_word(_PyJitCompiler *c, uint32_t i, _PyJitTemplate template)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(w));
    int may_run_python = (fmt->effects & (MAY_RUN_PYTHON & ~_PyIR_DEOPTS)) != 0;
    uint32_t next = i + (uint32_t)_PyIR_WordLength(w);
    int target = static_target(c->words, i);
    _PyJitWordCx cx = {
        .a = a, .words = c->words, .word = w, .index = i,
        .target = -1, .slow = _PyJitAsm_NewLabel(a), .leave = c->leave,
    };
    _PyJitLabel target_stub = -1;
    if (target >= 0) {
        if ((uint32_t)target > i && !may_run_python && has_block(c, (uint32_t)target)) {
            cx.target = c->labels[target];
        }
        else {
            cx.target = _PyJitAsm_NewLabel(a);
            target_stub = _PyJitAsm_NewLabel(a);
        }
    }
    if (_PyJit_HandlerMayChange(c->code, (int)i)) {
        _PyJitAsm_MovzxRM16(a, R(RCX), WORD_AT(i));
        _PyJitAsm_AluRI(a, _PyJitAlu_CMP, R(RCX), (int32_t)(w & 0xFFFF));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, cx.slow);
    }
    template(&cx);
    assert(cx.cold_depth == 0);
    /* The fall-through to the next word, whose block follows */
    _PyJitLabel next_stub = -1;
    if (may_run_python || !has_block(c, next)) {
        next_stub = _PyJitAsm_NewLabel(a);
        emit_template_tests(c, i, next, may_run_python, next_stub);
        if (!has_block(c, next)) {
            _PyJitAsm_Jmp(a, next_stub);
        }
    }
    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    if (next_stub >= 0) {
        emit_template_stub(c, i, next, next_stub);
    }
    if (target_stub >= 0) {
        /* The jump to the target, with its tests */
        _PyJitAsm_Bind(a, cx.target);
        emit_template_tests(c, i, (uint32_t)target, may_run_python, target_stub);
        if (has_block(c, (uint32_t)target)) {
            _PyJitAsm_Jmp(a, c->labels[target]);
        }
        else {
            _PyJitAsm_Jmp(a, target_stub);
        }
        emit_template_stub(c, i, (uint32_t)target, target_stub);
    }
    _PyJitAsm_Bind(a, cx.slow);
    emit_form_word(c, i);
    _PyJitAsm_Section(a, section);
}

/* The unit's code: the frame, a block per head word and the entry table.
 * Returns the label of the table. */
static _PyJitLabel
emit_unit(_PyJitCompiler *c)
{
    _PyJitAsm *a = c->a;
    c->leave = _PyJitAsm_NewLabel(a);
    c->backedge = _PyJitAsm_NewLabel(a);
    c->live = _PyJitAsm_NewLabel(a);
    c->trap = _PyJitAsm_NewLabel(a);
    for (uint32_t i = 0; i < c->nwords; i += _PyIR_WordLength(c->words[i])) {
        c->labels[i] = _PyJitAsm_NewLabel(a);
    }
    emit_frame(c);
    for (uint32_t i = 0; i < c->nwords; i += _PyIR_WordLength(c->words[i])) {
        _PyJitAsm_Bind(a, c->labels[i]);
        _PyJitTemplate template = _PyJit_Templates[IR_HANDLER(c->words[i])];
        if (template != NULL) {
            emit_template_word(c, i, template);
        }
        else {
            emit_form_word(c, i);
        }
    }
    _PyJitLabel table = _PyJitAsm_NewLabel(a);
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    _PyJitAsm_Align(a, 8);
    _PyJitAsm_Bind(a, table);
    for (uint32_t i = 0; i < c->nwords; i++) {
        _PyJitAsm_QuadLabel(a, c->labels[i] >= 0 ? c->labels[i] : c->trap);
    }
    return table;
}

static void
free_unit(_PyJitUnit *unit)
{
    _PyJitMemory_Free(&unit->code);
    PyMem_RawFree(unit);
}

/* Compile the words of exec, the executing copy of code, into a new unit
 * and make it the exec's.  Returns NULL if the words hold an instrumented
 * handler or the code cannot be placed. */
static _PyJitUnit *
compile(PyCodeObject *code, _PyCodeExec *exec)
{
    const _PyIRWord *words = exec->words;
    uint32_t nwords = exec->nwords;
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        if (_PyIR_HandlerFlags[IR_HANDLER(words[i])] & _PyIR_H_INSTRUMENTED) {
            return NULL;
        }
    }
    _PyJitUnit *unit = PyMem_RawCalloc(1, sizeof(_PyJitUnit));
    _PyJitLabel *labels = PyMem_RawMalloc(nwords * sizeof(_PyJitLabel));
    if (unit == NULL || labels == NULL) {
        PyMem_RawFree(unit);
        PyMem_RawFree(labels);
        return NULL;
    }
    for (uint32_t i = 0; i < nwords; i++) {
        labels[i] = -1;
    }
    unit->nwords = nwords;
    unit->words = words;
    _PyJitAsm a;
    _PyJitAsm_Init(&a);
    _PyJitCompiler c = {.a = &a, .code = code, .unit = unit, .words = words, .nwords = nwords,
                        .labels = labels};
    _PyJitLabel table = emit_unit(&c);
    int placed = _PyJitAsm_Place(&a, &unit->code);
    size_t table_offset = placed < 0 ? 0 : _PyJitAsm_LabelOffset(&a, table);
    _PyJitAsm_Fini(&a);
    PyMem_RawFree(labels);
    exec->jit_compiles++;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    interp->ceval.jit_compiled_codes += exec->jit_compiles == 1;
    interp->ceval.jit_compiles++;
    interp->ceval.jit_compiled_words += nwords;
    if (placed < 0) {
        /* Out of code memory: do not try this code again */
        exec->jit_compiles = _PyJIT_MAX_COMPILES;
        PyMem_RawFree(unit);
        return NULL;
    }
    unit->entries = (const void *const *)(unit->code.start + table_offset);
    unit->older = exec->jit_unit;
    exec->jit_unit = unit;
    if (interp->config.perf_profiling == 1) {
        PyObject *exc = PyErr_GetRaisedException();
        const char *qualname = PyUnicode_AsUTF8(code->co_qualname);
        const char *filename = PyUnicode_AsUTF8(code->co_filename);
        if (qualname != NULL && filename != NULL) {
            char name[512];
            PyOS_snprintf(name, sizeof(name), "py::jit:%s:%s", qualname, filename);
            PyUnstable_WritePerfMapEntry(unit->code.start,
                                         (unsigned int)unit->code.size, name);
        }
        PyErr_Clear();
        PyErr_SetRaisedException(exc);
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
    if (exec == &_PyIR_InitCleanupExec) {
        /* Shared by every interpreter and never freed */
        return NULL;
    }
    if (unit != NULL && unit->invalid == _PyJIT_STALE) {
        if (exec->jit_compiles >= _PyJIT_MAX_COMPILES) {
            unit->invalid = 0;
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
_PyJit_FreeUnits(_PyCodeExec *exec)
{
    _PyJitUnit *unit = exec->jit_unit;
    exec->jit_unit = NULL;
    while (unit != NULL) {
        _PyJitUnit *older = unit->older;
        free_unit(unit);
        unit = older;
    }
}

#endif  /* _PY_JIT_SUPPORTED */
