#include "Python.h"
#include "pycore_code.h"
#include "pycore_jit.h"
#include "pycore_jit_ir.h"
#include "pycore_pystate.h"

/* A bounded whole-method descriptor graph, not an arithmetic-region recognizer.
 * No Python semantics are copied here; operations use generated helpers. */
static _PyJitCodeUnit *
build_method(PyCodeObject *code, _PyCodeExec *exec,
             uint64_t generation, uintptr_t epoch, uint64_t source_stamp, _PyJitGraph *graph)
{
    Py_ssize_t count = Py_SIZE(code);
    if (count < 1 || count > 4096 ||
        code->co_flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        _PyJit_FreeGraph(graph);
        return NULL;
    }
    if (!_PyJit_ReserveBytes(sizeof(_PyJitCodeUnit))) {
        _PyJit_FreeGraph(graph);
        return NULL;
    }
    _PyJitCodeUnit *unit = PyMem_Calloc(1, sizeof(*unit));
    if (unit == NULL) {
        _PyJit_ReleaseBytes(sizeof(*unit));
        _PyJit_FreeGraph(graph);
        return NULL;
    }
    unit->graph = graph;
    unit->refs = 1;
    unit->charged_bytes = sizeof(*unit);
    if (PyErr_Occurred()) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    unit->generation = generation;
    unit->epoch = epoch;
    unit->source_stamp = source_stamp;
    unit->nops = (uint32_t)count;
    unit->nwords = (uint32_t)count;
    unit->record_header = unit->record_backedge = UINT32_MAX;
    size_t bytes = (size_t)count * sizeof(*unit->ops);
    if (!_PyJit_ReserveBytes(bytes)) { _PyJit_FreeUnit(unit); return NULL; }
    unit->charged_bytes += bytes;
    unit->ops = PyMem_Calloc((size_t)count, sizeof(*unit->ops));
    if (unit->ops == NULL || PyErr_Occurred()) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    bytes = (size_t)count * sizeof(*unit->offsets);
    if (!_PyJit_ReserveBytes(bytes)) { _PyJit_FreeUnit(unit); return NULL; }
    unit->charged_bytes += bytes;
    unit->offsets = PyMem_Malloc(bytes);
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
                .wide = IR_CD(word), .handler = (uint16_t)handler,
                .oparg = _PyCode_InstrOparg(code, (int)i)},
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
            if (handler == FOR_ITER) {
                target++;  /* exhaustion skips the END_FOR marker */
                if ((uint32_t)target >= unit->nops) { _PyJit_FreeUnit(unit); return NULL; }
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
        if (handler == JUMP_BACKWARD && op->target < i && unit->record_header == UINT32_MAX &&
            _PyFrame_NumSlotsForCodeObject(code) <= JIT_GRAPH_HOMES &&
            _PyThreadState_GET()->interp->opt_config.tagged_values_enabled &&
            exec->jit != NULL && !exec->jit->record_attempted) {
            unit->record_header = op->target;
            unit->record_backedge = i;
            unit->recording = 1;
        }
        if (i + 1 == unit->nops && op->kind == _PyJIT_LIR_HELPER &&
            handler != RETURN_VALUE && handler != RAISE_VARARGS) {
            _PyJit_FreeUnit(unit);
            return NULL;
        }
    }
    for (uint32_t i = 0; i < unit->nwords; i++) {
        unit->ops[i].next_op = unit->ops[i].fallthrough;
        unit->ops[i].target_op = unit->ops[i].target;
    }
    if ((graph != NULL && _PyJit_LowerGraph(unit) < 0) || _PyJit_EmitBaseline(unit) < 0) {
        _PyJit_FreeUnit(unit);
        return NULL;
    }
    return unit;
}

_PyJitCodeUnit *
_PyJit_BuildBaseline(PyCodeObject *code, _PyCodeExec *exec,
                     uint64_t generation, uintptr_t epoch, uint64_t source_stamp)
{
    return build_method(code, exec, generation, epoch, source_stamp, NULL);
}

_PyJitCodeUnit *
_PyJit_BuildOptimized(PyCodeObject *code, _PyCodeExec *exec,
                     uint64_t generation, uintptr_t epoch, uint64_t source_stamp, _PyJitGraph *graph)
{
    return build_method(code, exec, generation, epoch, source_stamp, graph);
}
