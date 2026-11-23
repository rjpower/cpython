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

int
_PyJit_IsAvailable(void)
{
    return _PyJitMemory_Init() == 0;
}

/* Where an operand word's entry leads: no transfer names one. */
static void
bad_entry(void)
{
    Py_FatalError("baseline JIT: entered an operand word");
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

/* Emit the unit's code for words: the entry, the shared exits, a block per
 * head word and the entry table.  labels has a slot per word, -1 for the
 * operand words.  Returns the label of the table. */
static _PyJitLabel
emit_unit(_PyJitAsm *a, _PyJitUnit *unit, const _PyIRWord *words,
          uint32_t nwords, _PyJitLabel *labels)
{
    _PyJitLabel leave = _PyJitAsm_NewLabel(a);
    _PyJitLabel backedge = _PyJitAsm_NewLabel(a);
    _PyJitLabel trap = _PyJitAsm_NewLabel(a);
    _PyJitLabel table = _PyJitAsm_NewLabel(a);
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        labels[i] = _PyJitAsm_NewLabel(a);
    }

    /* The entry: enter(tstate, frame, address of the word's block) */
    _PyJitAsm_Push(a, R(RBP));
    _PyJitAsm_MovRR(a, R(RBP), R(RSP));
    _PyJitAsm_Push(a, R(RBX));
    _PyJitAsm_Push(a, R(R12));
    _PyJitAsm_Push(a, R(R13));
    _PyJitAsm_Push(a, R(R14));
    _PyJitAsm_MovRR(a, R(R12), R(RDI));
    _PyJitAsm_MovRR(a, R(R13), R(RSI));
    _PyJitAsm_MovRM(a, R(R14),
                    AT(R13, offsetof(_PyInterpreterFrame, ir_words)));
    _PyJitAsm_MovImm(a, R(RBX), (int64_t)(uintptr_t)unit);
    _PyJitAsm_JmpR(a, R(RDX));

    /* The exit, with the transfer in rax:rdx */
    _PyJitAsm_Bind(a, leave);
    _PyJitAsm_Pop(a, R(R14));
    _PyJitAsm_Pop(a, R(R13));
    _PyJitAsm_Pop(a, R(R12));
    _PyJitAsm_Pop(a, R(RBX));
    _PyJitAsm_Pop(a, R(RBP));
    _PyJitAsm_Ret(a);

    /* A backward jump of a stale or retired unit, in the same frame: a
     * warm-up event, which may recompile; then leave with the transfer. */
    _PyJitAsm_Bind(a, backedge);
    _PyJitAsm_Push(a, R(RDX));
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_CallAbs(a, (const void *)_PyJit_Event);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_Pop(a, R(RDX));
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_Jmp(a, leave);

    _PyJitAsm_Bind(a, trap);
    _PyJitAsm_CallAbs(a, (const void *)bad_entry);

    for (uint32_t i = 0; i < nwords; ) {
        _PyIRWord w = words[i];
        int handler = IR_HANDLER(w);
        uint32_t next = i + (uint32_t)_PyIR_WordLength(w);
        const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
        _PyJitAsm_Bind(a, labels[i]);
        _PyJitAsm_MovRR(a, R(RDI), R(R12));
        _PyJitAsm_MovRR(a, R(RSI), R(R13));
        _PyJitAsm_Lea(a, R(RDX), AT(R14, (int32_t)(i * sizeof(_PyIRWord))));
        _PyJitAsm_MovImm(a, R(RCX), (int64_t)w);
        _PyJitAsm_CallAbs(a, _PyEval_HandlerForm(handler));
        /* Another frame: a call, a return, a yield, a resume */
        _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RAX), R(R13));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, leave);
        int target = static_target(words, i);
        if (target >= 0 && ((uint32_t)target >= nwords || labels[target] < 0)) {
            target = -1;
        }
        if (target >= 0 && (uint32_t)target <= i) {
            _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
            _PyJitAsm_TestRR(a, R(RCX), R(RCX));
            _PyJitAsm_Jcc(a, _PyJitCond_NE, backedge);
        }
        else if (fmt->effects & MAY_RUN_PYTHON) {
            _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
            _PyJitAsm_TestRI(a, R(RCX), _PyJIT_RETIRED);
            _PyJitAsm_Jcc(a, _PyJitCond_NE, leave);
        }
        if (target >= 0) {
            _PyJitAsm_Lea(a, R(RCX),
                          AT(R14, (int32_t)((uint32_t)target * sizeof(_PyIRWord))));
            _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
            _PyJitAsm_Jcc(a, _PyJitCond_E, labels[target]);
        }
        if (!(fmt->flags & _PyIR_JUMP_ALWAYS) && next < nwords) {
            /* The next word's block follows */
            _PyJitAsm_Lea(a, R(RCX),
                          AT(R14, (int32_t)(next * sizeof(_PyIRWord))));
            _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
            _PyJitAsm_Jcc(a, _PyJitCond_NE, leave);
        }
        else {
            _PyJitAsm_Jmp(a, leave);
        }
        i = next;
    }

    _PyJitAsm_Align(a, 8);
    _PyJitAsm_Bind(a, table);
    for (uint32_t i = 0; i < nwords; i++) {
        _PyJitAsm_QuadLabel(a, labels[i] >= 0 ? labels[i] : trap);
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
    _PyJitLabel table = emit_unit(&a, unit, words, nwords, labels);
    int placed = _PyJitAsm_Place(&a, &unit->code);
    size_t table_offset = placed < 0 ? 0 : _PyJitAsm_LabelOffset(&a, table);
    _PyJitAsm_Fini(&a);
    PyMem_RawFree(labels);
    exec->jit_compiles++;
    if (placed < 0) {
        /* Out of code memory: do not try this code again */
        exec->jit_compiles = _PyJIT_MAX_COMPILES;
        PyMem_RawFree(unit);
        return NULL;
    }
    unit->entries = (const void *const *)(unit->code.start + table_offset);
    unit->older = exec->jit_unit;
    exec->jit_unit = unit;
    if (_PyInterpreterState_GET()->config.perf_profiling == 1) {
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
        int warmup = tstate->interp->ceval.jit == _PyJIT_ALWAYS ? 1 : _PyJIT_WARMUP;
        if (++exec->jit_heat < (uint32_t)warmup ||
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
