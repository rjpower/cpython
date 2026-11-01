#include "Python.h"
#include "pycore_code.h"
#include "pycore_jit.h"

/* A bounded whole-method descriptor graph, not an arithmetic-region recognizer.
 * No Python semantics are copied here; operations use generated helpers. */
_PyJitCodeUnit *
_PyJit_BuildBaseline(PyCodeObject *code, _PyCodeExec *exec,
                     uint64_t generation, uintptr_t epoch, uint64_t source_stamp)
{
    Py_ssize_t count = Py_SIZE(code);
    if (count < 1 || count > 4096 ||
        code->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        return NULL;
    }
    _PyJitCodeUnit *unit = PyMem_Calloc(1, sizeof(*unit));
    if (unit == NULL) {
        return NULL;
    }
    unit->refs = 1;
    if (PyErr_Occurred()) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    unit->generation = generation;
    unit->epoch = epoch;
    unit->source_stamp = source_stamp;
    unit->nops = (uint32_t)count;
    unit->ops = PyMem_Calloc((size_t)count, sizeof(*unit->ops));
    if (unit->ops == NULL || PyErr_Occurred()) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    unit->offsets = PyMem_Malloc((size_t)count * sizeof(*unit->offsets));
    if (unit->ops == NULL || unit->offsets == NULL || PyErr_Occurred() || code->_co_exec != exec) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    for (uint32_t i = 0; i < unit->nops; i++) {
        _PyIRWord word = _PyCode_WORDS(code)[i];
        int handler = _PyCode_GenericHandler(code, (int)i);
        const _PyJitHelperInfo *info = _PyJit_GetHelper((unsigned int)handler);
        const _PyIRFormat *format = _PyIR_FormatOf(handler);
        _PyJitLirOp *op = &unit->ops[i];
        *op = (_PyJitLirOp){
            .decoded = {.instruction = exec->words + i,
                .fields = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)},
                .wide = IR_CD(word), .handler = (uint16_t)handler},
            .helper = info ? info->helper : NULL, .word = i,
            .fallthrough = i + 1, .target = UINT32_MAX,
            .kind = info && info->helper ? _PyJIT_LIR_HELPER : _PyJIT_LIR_EXIT,
        };
        if (format->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) {
            int target = _PyCode_InstrTarget(code, (int)i);
            if (target < 0 || (uint32_t)target >= unit->nops) {
                _PyJit_FreeUnit(unit);
                return NULL;
            }
            op->target = (uint32_t)target;
            if (op->helper != NULL) {
                op->kind = _PyJIT_LIR_BRANCH;
            }
        }
        if ((handler == POP_JUMP_IF_FALSE || handler == POP_JUMP_IF_TRUE ||
             handler == POP_JUMP_IF_NONE || handler == POP_JUMP_IF_NOT_NONE) &&
            i + 1 < unit->nops && _PyCode_InstrHandler(code, (int)i + 1) == NOT_TAKEN) {
            /* The shared conditional helper skips this instrumentation marker
             * on its non-taken path. No Python operation is copied here. */
            op->fallthrough = i + 2;
        }
        if (i + 1 == unit->nops && op->kind == _PyJIT_LIR_HELPER && handler != RETURN_VALUE) {
            _PyJit_FreeUnit(unit);
            return NULL;
        }
    }
    if (_PyJit_EmitBaseline(unit) < 0) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    return unit;
}
