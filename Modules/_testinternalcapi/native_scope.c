#include "parts.h"

#include "pycore_borrow.h"        // _PyBorrow_Depth()
#include "pycore_native_scope.h"
#include "pycore_tuple.h"
#include "pycore_interpframe.h"
#include "pycore_ir.h"
#include "pycore_methodobject.h"  // PyCFunctionObject
#include "pycore_modsupport.h"

#include "clinic/native_scope.c.h"

/*[clinic input]
module _testinternalcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7bb583d8c9eb9a78]*/

/*[clinic input]
_testinternalcapi.native_scope_flat

    value: object
    scale: int = 1
    *
    callback: object = None

Exercise a flat keyword parser with caller-owned argument views.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_native_scope_flat_impl(PyObject *module, PyObject *value,
                                         int scale, PyObject *callback)
/*[clinic end generated code: output=b49f104151c166ad input=cace89e89799028f]*/
{
    if (callback != Py_None) {
        return PyObject_CallOneArg(callback, value);
    }
    PyObject *factor = PyLong_FromLong(scale);
    if (factor == NULL) {
        return NULL;
    }
    PyObject *result = PyNumber_Multiply(value, factor);
    Py_DECREF(factor);
    return result;
}

/*[clinic input]
_testinternalcapi.native_scope_flat_noargs

Return a multi-digit scalar through a genuine NOARGS wrapper.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_native_scope_flat_noargs_impl(PyObject *module)
/*[clinic end generated code: output=016d3b6df8172ef8 input=cad52f11e0cbae5b]*/
{
    return PyLong_FromLong(INT32_MAX);
}

/*[clinic input]
_testinternalcapi.native_scope_flat_fast

    left: object
    right: object
    /

Add two objects through a genuine FASTCALL wrapper.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_native_scope_flat_fast_impl(PyObject *module,
                                              PyObject *left,
                                              PyObject *right)
/*[clinic end generated code: output=23abc60fb9e5edfb input=d222590933b73d2c]*/
{
    return PyNumber_Add(left, right);
}


typedef struct {
    PyValue *destination;
    int published;
    int destroyed;
    int cleanup_error;
} Publication;

static void
publish_state(void *opaque)
{
    Publication *state = opaque;
    assert(PyValue_IsNull(*state->destination) ||
           PyValue_IsLanguageValue(*state->destination));
    state->published = 1;
}

static void
publication_destructor(PyObject *capsule)
{
    PyObject *entry_error = PyErr_GetRaisedException();
    Publication *state = PyCapsule_GetPointer(capsule,
                                             "native_scope.publication");
    assert(state != NULL);
    assert(state->published);
    state->destroyed++;
    if (state->cleanup_error) {
        PyErr_SetString(PyExc_RuntimeError, "native scope cleanup failure");
        PyErr_FormatUnraisable("Exception ignored by native scope publication fixture");
    }
    PyErr_SetRaisedException(entry_error);
}

/* This exercises the engine with an ordinary C owner and a publication flag.
 * It is deliberately not evidence of VM frame/window publication. */
static PyObject *
scope_mechanism(PyObject *argument, int abort_result, int cleanup_error,
                int primary_error)
{
    _PyNativeScope scope;
    PyObject *primary = NULL;
    PyValue input = PyValue_DUP(PyValue_FromObjectBorrow(argument));
    int begun = _PyNativeScope_Begin(&scope, PyThreadState_Get(), &input,
                                    1, 1, PyValue_NULL, NULL);
    PyValue_CLOSE(input);
    PyValue destination = PyValue_NULL;
    Publication state = {.destination = &destination,
                         .cleanup_error = cleanup_error};
    if (begun < 0) {
        goto abort;
    }
    PyObject *const *objects;
    if (_PyNativeScope_Objects(&scope, &objects) < 0) {
        goto abort;
    }
    PyObject *capsule = PyCapsule_New(&state, "native_scope.publication",
                                     publication_destructor);
    if (capsule == NULL) {
        goto abort;
    }
    if (abort_result) {
        destination = PyValue_DUP(PyValue_FromObjectBorrow(argument));
        if (_PyNativeScope_StageResult(&scope, capsule, &scope.result) < 0) {
            goto abort;
        }
        if (primary_error) {
            PyErr_SetString(PyExc_ValueError, "native scope primary failure");
            primary = PyErr_GetRaisedException();
            PyErr_SetRaisedException(Py_NewRef(primary));
        }
        _PyNativeResult_Abort(&scope.result, publish_state, &state);
    }
    else {
        destination = PyValue_PrepareOwned(capsule);
        if (_PyNativeScope_StageResult(&scope, Py_NewRef(objects[0]),
                                       &scope.result) < 0)
        {
            goto abort;
        }
        _PyNativeResult_Publish(&scope.result, &destination,
                                publish_state, &state);
    }
    _PyNativeScope_End(&scope);
    int failed = primary != NULL;
    if (primary != NULL) {
        PyObject *current = PyErr_GetRaisedException();
        assert(current == primary);
        PyErr_SetRaisedException(current);
        Py_DECREF(primary);
        primary = NULL;
    }
    assert(state.destroyed == 1);
    assert(scope.result.state == _Py_NATIVE_RESULT_DRAINED);
    if (failed) {
        PyValue_CLEAR(destination);
        return NULL;
    }
    PyObject *result = _PyValue_AsObjectAt(destination, BOX_SITE_TEST);
    PyValue_CLEAR(destination);
    return result;

abort:
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    PyValue_CLEAR(destination);
    return NULL;
}

static PyObject *
native_scope_publish(PyObject *Py_UNUSED(module), PyObject *argument)
{
    return scope_mechanism(argument, 0, 0, 0);
}

static PyObject *
native_scope_abort(PyObject *Py_UNUSED(module), PyObject *argument)
{
    return scope_mechanism(argument, 1, 0, 0);
}

static PyObject *
native_scope_cleanup_error(PyObject *Py_UNUSED(module), PyObject *argument)
{
    return scope_mechanism(argument, 0, 1, 0);
}

static PyObject *
native_scope_primary_error(PyObject *Py_UNUSED(module), PyObject *argument)
{
    return scope_mechanism(argument, 1, 1, 1);
}

/* End a scope with an exception pending, when the staged result is the only
 * owner of factory()'s result. Its release can run a raising __del__; End
 * must leave the pending exception in place. */
static PyObject *
native_scope_end_pending(PyObject *Py_UNUSED(module), PyObject *factory)
{
    _PyNativeScope scope;
    PyValue input = PyValue_NULL;
    PyValue destination = PyValue_NULL;
    Publication state = {.destination = &destination};
    if (_PyNativeScope_Begin(&scope, PyThreadState_Get(), &input, 0, 0,
                             PyValue_NULL, NULL) < 0 ||
        _PyNativeScope_StageResult(&scope, PyObject_CallNoArgs(factory),
                                   &scope.result) < 0)
    {
        _PyNativeResult_Abort(&scope.result, publish_state, &state);
        _PyNativeScope_End(&scope);
        return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "native scope primary failure");
    PyObject *primary = PyErr_GetRaisedException();
    PyErr_SetRaisedException(Py_NewRef(primary));
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    PyObject *raised = PyErr_GetRaisedException();
    int valid = raised == primary;
    Py_XDECREF(raised);
    if (!valid) {
        Py_DECREF(primary);
        PyErr_SetString(PyExc_AssertionError,
                        "native scope End changed the pending exception");
        return NULL;
    }
    PyErr_SetRaisedException(primary);
    return NULL;
}

static PyObject *
native_scope_echo(PyObject *Py_UNUSED(module), PyObject *argument)
{
    return Py_NewRef(argument);
}

static PyObject *
native_scope_noargs(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    return PyFloat_FromDouble(2.5);
}

static PyObject *
native_scope_raise(PyObject *Py_UNUSED(module), PyObject *argument)
{
    PyErr_SetObject(PyExc_ValueError, argument);
    return NULL;
}

static PyObject *
native_scope_reenter(PyObject *Py_UNUSED(module), PyObject *callback)
{
    return PyObject_CallNoArgs(callback);
}

static PyMethodDef bound_reenter = {
    "native_scope_bound_reenter", native_scope_reenter, METH_O,
};

static PyObject *
native_scope_bound(PyObject *Py_UNUSED(module), PyObject *receiver)
{
    return PyCFunction_New(&bound_reenter, receiver);
}

static void
completed_call_cleanup(PyObject *capsule)
{
    PyObject *entry_error = PyErr_GetRaisedException();
    PyObject *ledger = PyCapsule_GetPointer(capsule, "native_scope.cleanup");
    assert(ledger != NULL);
    PyObject *marker = PyUnicode_FromString("cleanup");
    if (marker != NULL) {
        PyList_Append(ledger, marker);
        Py_DECREF(marker);
    }
    Py_DECREF(ledger);
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError, "completed native call cleanup");
    }
    PyErr_FormatUnraisable("Exception ignored by native scope argument fixture");
    PyErr_SetRaisedException(entry_error);
}

static PyObject *
native_scope_cleanup_argument(PyObject *Py_UNUSED(module), PyObject *ledger)
{
    if (!PyList_CheckExact(ledger)) {
        PyErr_SetString(PyExc_TypeError, "expected an exact ledger list");
        return NULL;
    }
    Py_INCREF(ledger);
    PyObject *capsule = PyCapsule_New(ledger, "native_scope.cleanup",
                                     completed_call_cleanup);
    if (capsule == NULL) {
        Py_DECREF(ledger);
    }
    return capsule;
}

static PyObject *
native_scope_discard(PyObject *Py_UNUSED(module), PyObject *argument)
{
    if (PyCapsule_IsValid(argument, "native_scope.cleanup")) {
        PyObject *ledger = PyCapsule_GetPointer(argument,
                                               "native_scope.cleanup");
        if (ledger == NULL) {
            return NULL;
        }
        PyObject *marker = PyUnicode_FromString("body");
        if (marker == NULL) {
            return NULL;
        }
        int status = PyList_Append(ledger, marker);
        Py_DECREF(marker);
        if (status < 0) {
            return NULL;
        }
    }
    return PyLong_FromLong(INT32_MAX);
}

static PyObject *
gateway_mechanism(PyObject *argument)
{
    int noargs = argument == NULL;
    static PyMethodDef echo = {"scoped_echo", native_scope_echo, METH_O};
    static PyMethodDef noarg = {"scoped_noargs", native_scope_noargs,
                               METH_NOARGS};
    _PyNativeScope scope;
    PyValue input = noargs ? PyValue_NULL :
        PyValue_DUP(PyValue_FromObjectBorrow(argument));
    int begun = _PyNativeScope_Begin(&scope, PyThreadState_Get(), &input,
                                    !noargs, !noargs, PyValue_NULL, NULL);
    PyValue_XCLOSE(input);
    PyValue destination = PyValue_NULL;
    Publication state = {.destination = &destination};
    PyObject *callable = NULL;
    if (begun < 0) {
        goto abort;
    }
    callable = PyCFunction_New(noargs ? &noarg : &echo, NULL);
    if (callable == NULL ||
        _PyCFunction_CallScoped(&scope, callable, &scope.result) < 0)
    {
        goto abort;
    }
    _PyNativeResult_Publish(&scope.result, &destination, publish_state, &state);
    _PyNativeScope_End(&scope);
    Py_CLEAR(callable);
    PyObject *result = _PyValue_AsObjectAt(destination, BOX_SITE_TEST);
    PyValue_CLEAR(destination);
    return result;

abort:
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    Py_XDECREF(callable);
    PyValue_CLEAR(destination);
    return NULL;
}

static PyObject *
native_scope_gateway_o(PyObject *Py_UNUSED(module), PyObject *argument)
{
    return gateway_mechanism(argument);
}

static PyObject *
native_scope_gateway_noargs(PyObject *Py_UNUSED(module),
                            PyObject *Py_UNUSED(ignored))
{
    return gateway_mechanism(NULL);
}

typedef struct {
    PyMemAllocatorEx saved;
    int consumed;
} BoxFailure;

static void *
fail_box_malloc(void *opaque, size_t size)
{
    BoxFailure *failure = opaque;
    failure->consumed = 1;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure->saved);
    return NULL;
}

static void *
fail_box_calloc(void *opaque, size_t nelem, size_t elsize)
{
    return fail_box_malloc(opaque, 0);
}

static void *
fail_box_realloc(void *opaque, void *pointer, size_t size)
{
    return fail_box_malloc(opaque, 0);
}

static void
fail_box_free(void *opaque, void *pointer)
{
    BoxFailure *failure = opaque;
    failure->saved.free(failure->saved.ctx, pointer);
}

static PyObject *
native_scope_box_oom(PyObject *Py_UNUSED(module), PyObject *argument)
{
    _PyNativeScope scope;
    PyValue input = PyValue_DUP(PyValue_FromObjectBorrow(argument));
    PyValue destination = PyValue_NULL;
    Publication state = {.destination = &destination};
    BoxFailure failure = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    PyMemAllocatorEx failing = {
        .ctx = &failure,
        .malloc = fail_box_malloc,
        .calloc = fail_box_calloc,
        .realloc = fail_box_realloc,
        .free = fail_box_free,
    };
    /* Begin boxes an inline argument at once, so the box's allocation is
       the first, and it fails. */
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failing);
    int begun = _PyNativeScope_Begin(&scope, PyThreadState_Get(), &input,
                                    1, 1, PyValue_NULL, NULL);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    if (begun == 0 || !failure.consumed) {
        PyErr_SetString(PyExc_AssertionError,
                        "argument did not allocate a scalar box");
    }
    else {
        assert(PyErr_ExceptionMatches(PyExc_MemoryError));
    }
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    PyValue_XCLOSE(input);
    return NULL;
}

static PyObject *
vector_method(PyObject *self, PyTypeObject *defining_class,
               PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    assert(self == Py_None);
    return Py_NewRef((PyObject *)defining_class);
}

static PyObject *
native_scope_vector_method(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    static PyMethodDef method = {
        "scoped_method", _PyCFunction_CAST(vector_method),
        METH_METHOD | METH_FASTCALL | METH_KEYWORDS,
    };
    return PyCMethod_New(&method, Py_None, NULL, &PyBaseObject_Type);
}

/* Snapshot actual frame homes before allocating the report. This does not
 * consult a caller-maintained publication flag and never repairs a home. */
static PyObject *
native_scope_call_state(PyObject *Py_UNUSED(module), PyObject *code_object)
{
    if (!PyCode_Check(code_object)) {
        PyErr_SetString(PyExc_TypeError, "expected a code object");
        return NULL;
    }
    _PyInterpreterFrame *frame = PyThreadState_Get()->current_frame;
    while (frame != NULL &&
           (frame->owner == FRAME_OWNED_BY_INTERPRETER ||
            (PyObject *)_PyFrame_GetCode(frame) != code_object)) {
        frame = frame->previous;
    }
    if (frame == NULL) {
        PyErr_SetString(PyExc_ValueError, "code has no active frame");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)code_object;
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || frame->ir_words != exec->words ||
        frame->instr_ptr < exec->words ||
        frame->instr_ptr >= exec->words + exec->nwords) {
        PyErr_SetString(PyExc_AssertionError, "frame has stale executing words");
        return NULL;
    }
    Py_ssize_t word_index = frame->instr_ptr - exec->words;
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*frame->instr_ptr);
    int handler = IR_HANDLER(word);
    int argc = (int)IR_C(word), count;
    switch (_PyIR_BaseOpcode(handler)) {
        case CALL_FUNCTION_EX:
            count = 4;
            break;
        case CALL_KW:
            count = argc + 3;
            break;
        case CALL:
            count = argc + 2;
            break;
        default:
            PyErr_SetString(PyExc_AssertionError, "frame is outside the selected call routes");
            return NULL;
    }
    int base = (int)IR_B(word), destination = (int)IR_A(word);
    int nslots = _PyFrame_NumSlots(frame);
    PyValue snapshots[64];
    if (nslots > (int)Py_ARRAY_LENGTH(snapshots) ||
        base < code->co_nlocalsplus || base > nslots - count ||
        destination >= nslots) {
        PyErr_SetString(PyExc_AssertionError, "observer frame exceeds bounded call shape");
        return NULL;
    }
    uint64_t present = 0, strong = 0;
    for (int i = 0; i < nslots; i++) {
        PyValue value = _PyFrame_Slots(frame)[i];
        if (!PyValue_IsNull(value)) {
            present |= UINT64_C(1) << i;
            // The slot owns its value: a count, or none needed (H1).
            if (PyValue_RefcountOnObject(value) || !PyValue_IsObject(value)
                || _Py_IsImmortal(PyValue_AsKnownObjectBorrow(value)))
            {
                strong |= UINT64_C(1) << i;
            }
        }
        snapshots[i] = PyValue_XDUP(value);
    }
    int nlocals = code->co_nlocalsplus;
    PyObject *values = PyTuple_New(nslots);
    PyObject *report = NULL;
    if (values != NULL) {
        for (int i = 0; i < nslots; i++) {
            PyTuple_SET_VALUE(values, i, PyValue_IsNull(snapshots[i])
                ? PyValue_DUP(PyValue_FromObjectBorrow(Py_None)) : PyValue_XDUP(snapshots[i]));
        }
        report = Py_BuildValue("(iniiiiOKK)", handler, word_index, nlocals,
            base, count, destination, values,
            (unsigned long long)present, (unsigned long long)strong);
    }
    Py_XDECREF(values);
    for (int i = nslots; i-- > 0;) {
        PyValue_CLEAR(snapshots[i]);
    }
    return report;
}

static PyMethodDef methods[] = {
    {"native_scope_call_state", native_scope_call_state, METH_O},
    _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_METHODDEF
    _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_NOARGS_METHODDEF
    _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_FAST_METHODDEF
    {"native_scope_vector_method", native_scope_vector_method, METH_NOARGS},
    {"native_scope_publish", native_scope_publish, METH_O},
    {"native_scope_abort", native_scope_abort, METH_O},
    {"native_scope_cleanup_error", native_scope_cleanup_error, METH_O},
    {"native_scope_primary_error", native_scope_primary_error, METH_O},
    {"native_scope_end_pending", native_scope_end_pending, METH_O},
    {"native_scope_echo", native_scope_echo, METH_O},
    {"native_scope_noargs", native_scope_noargs, METH_NOARGS},
    {"native_scope_raise", native_scope_raise, METH_O},
    {"native_scope_reenter", native_scope_reenter, METH_O},
    {"native_scope_bound", native_scope_bound, METH_O},
    {"native_scope_cleanup_argument", native_scope_cleanup_argument, METH_O},
    {"native_scope_discard", native_scope_discard, METH_O},
    {"native_scope_gateway_o", native_scope_gateway_o, METH_O},
    {"native_scope_gateway_noargs", native_scope_gateway_noargs, METH_NOARGS},
    {"native_scope_box_oom", native_scope_box_oom, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_NativeScope(PyObject *module)
{
    return PyModule_AddFunctions(module, methods);
}
