#include "parts.h"

#include "pycore_native_scope.h"
#include "pycore_tuple.h"
#include "pycore_interpframe.h"
#include "pycore_ir.h"
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
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(factor);
    _PyValue_RestoreException(saved);
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

static int
close_destination(PyValue *destination)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyValue value = *destination;
    *destination = PyValue_NULL;
    PyValue_XCLOSE(value);
    _PyValue_RestoreException(saved);
    return PyErr_Occurred() ? -1 : 0;
}

/* This exercises the engine with an ordinary C owner and a publication flag.
 * It is deliberately not evidence of VM frame/window publication. */
static PyObject *
scope_mechanism(PyObject *argument, int abort_result, int cleanup_error,
                int primary_error)
{
    _PyNativeScope scope;
    PyObject *primary = NULL;
    PyValue input = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(argument));
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
        destination = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(argument));
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
        PyObject *retired;
        destination = PyValue_PrepareOwned(capsule, &retired);
        assert(retired == NULL);
        if (_PyNativeScope_StageResult(&scope, Py_NewRef(objects[0]),
                                       &scope.result) < 0)
        {
            goto abort;
        }
        _PyNativeResult_Publish(&scope.result, &destination,
                                publish_state, &state);
    }
    int status = _PyNativeScope_End(&scope);
    if (primary != NULL) {
        PyObject *current = PyErr_GetRaisedException();
        assert(current == primary);
        PyErr_SetRaisedException(current);
        Py_DECREF(primary);
        primary = NULL;
    }
    assert(state.destroyed == 1);
    assert(scope.result.state == _Py_NATIVE_RESULT_DRAINED);
    if (status < 0) {
        close_destination(&destination);
        return NULL;
    }
    PyObject *result = PyValue_AsObject(destination);
    if (close_destination(&destination) < 0) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;

abort:
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    close_destination(&destination);
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
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(marker);
        _PyValue_RestoreException(saved);
    }
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(ledger);
    _PyValue_RestoreException(saved);
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
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(ledger);
        _PyValue_RestoreException(saved);
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
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(marker);
        _PyValue_RestoreException(saved);
        if (status < 0 || PyErr_Occurred()) {
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
        PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(argument));
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
    int status = _PyNativeScope_End(&scope);
    PyObject *saved = PyErr_GetRaisedException();
    Py_CLEAR(callable);
    _PyValue_RestoreException(saved);
    if (status < 0 || PyErr_Occurred()) {
        close_destination(&destination);
        return NULL;
    }
    PyObject *result = PyValue_AsObject(destination);
    if (close_destination(&destination) < 0) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;

abort:
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    saved = PyErr_GetRaisedException();
    Py_XDECREF(callable);
    _PyValue_RestoreException(saved);
    close_destination(&destination);
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
    PyValue input = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(argument));
    int begun = _PyNativeScope_Begin(&scope, PyThreadState_Get(), &input,
                                    1, 1, PyValue_NULL, NULL);
    PyValue_XCLOSE(input);
    PyValue destination = PyValue_NULL;
    Publication state = {.destination = &destination};
    if (begun < 0) {
        goto abort;
    }
    BoxFailure failure = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    PyMemAllocatorEx failing = {
        .ctx = &failure,
        .malloc = fail_box_malloc,
        .calloc = fail_box_calloc,
        .realloc = fail_box_realloc,
        .free = fail_box_free,
    };
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failing);
    PyObject *const *objects;
    int status = _PyNativeScope_Objects(&scope, &objects);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    if (status == 0 || !failure.consumed) {
        PyErr_SetString(PyExc_AssertionError,
                        "argument did not allocate a scalar box");
    }
    else {
        assert(PyErr_ExceptionMatches(PyExc_MemoryError));
    }
abort:
    _PyNativeResult_Abort(&scope.result, publish_state, &state);
    _PyNativeScope_End(&scope);
    return NULL;
}

typedef struct {
    PyMemAllocatorEx saved;
    PyMemAllocatorDomain domain;
    int consumed;
} VectorFailure;

static void *
vector_fail_malloc(void *opaque, size_t size)
{
    VectorFailure *failure = opaque;
    failure->consumed = 1;
    PyMem_SetAllocator(failure->domain, &failure->saved);
    return NULL;
}

static void *
vector_fail_calloc(void *opaque, size_t count, size_t size)
{
    return vector_fail_malloc(opaque, 0);
}

static void *
vector_fail_realloc(void *opaque, void *pointer, size_t size)
{
    return vector_fail_malloc(opaque, 0);
}

static void
vector_fail_free(void *opaque, void *pointer)
{
    VectorFailure *failure = opaque;
    failure->saved.free(failure->saved.ctx, pointer);
}

static void
vector_arm_failure(VectorFailure *failure, PyMemAllocatorDomain domain)
{
    failure->domain = domain;
    PyMem_GetAllocator(domain, &failure->saved);
    PyMemAllocatorEx allocator = {
        .ctx = failure,
        .malloc = vector_fail_malloc,
        .calloc = vector_fail_calloc,
        .realloc = vector_fail_realloc,
        .free = vector_fail_free,
    };
    PyMem_SetAllocator(domain, &allocator);
}

/* This publishes a C destination, not a VM call window. The paired VM
 * consumer tests must independently establish the real generic route. */
static PyObject *
vector_mechanism(PyObject *callable, const PyValue *values,
                 Py_ssize_t positional_count, Py_ssize_t total_count,
                 PyObject *kwnames, int fault)
{
    _PyNativeScope scope;
    PyValue destination = PyValue_NULL;
    Publication publication = {.destination = &destination};
    VectorFailure failure = {0};
    if (fault == 1) {
        vector_arm_failure(&failure, PYMEM_DOMAIN_MEM);
    }
    int status = _PyNativeScope_BeginVector(
        &scope, PyThreadState_Get(), callable, values,
        positional_count, total_count, kwnames);
    if (fault == 1) {
        PyMem_SetAllocator(failure.domain, &failure.saved);
    }
    if (status < 0) {
        goto abort;
    }
    if (fault == 2) {
        vector_arm_failure(&failure, PYMEM_DOMAIN_OBJ);
    }
    status = _PyCFunction_CallScoped(&scope, callable, &scope.result);
    if (fault == 2) {
        PyMem_SetAllocator(failure.domain, &failure.saved);
        if (status < 0 && failure.consumed &&
            (scope.objects[0] == NULL || scope.objects[1] != NULL))
        {
            PyErr_SetString(PyExc_AssertionError,
                            "boxing fault did not leave a partial view array");
        }
    }
    if (fault != 0 && (status >= 0 || !failure.consumed)) {
        PyErr_SetString(PyExc_AssertionError,
                        "native vector allocation fault was not consumed");
        goto abort;
    }
    if (status != 0) {
        if (status > 0) {
            PyErr_SetString(PyExc_RuntimeError,
                            "vector mechanism target is not eligible");
        }
        goto abort;
    }
    _PyNativeResult_Publish(&scope.result, &destination,
                            publish_state, &publication);
    PyObject *retired = _PyNativeScope_TakeRetiredResult(&scope);
    status = _PyNativeScope_End(&scope);
    PyObject *saved = PyErr_GetRaisedException();
    Py_XDECREF(retired);
    _PyValue_RestoreException(saved);
    if (status < 0 || PyErr_Occurred()) {
        close_destination(&destination);
        return NULL;
    }
    PyObject *result = PyValue_AsObject(destination);
    if (close_destination(&destination) < 0) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;

abort:
    _PyNativeResult_Abort(&scope.result, publish_state, &publication);
    (void)_PyNativeScope_End(&scope);
    close_destination(&destination);
    return NULL;
}

static PyObject *
native_scope_vector(PyObject *Py_UNUSED(module), PyObject *const *args,
                    Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs < 1) {
        PyErr_SetString(PyExc_TypeError, "a C callable is required");
        return NULL;
    }
    PyObject *callable = args[0];
    Py_ssize_t nkw = kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t npos = nargs - 1;
    if (npos > PY_SSIZE_T_MAX - nkw ||
        (size_t)(npos + nkw) > (size_t)PY_SSIZE_T_MAX / sizeof(PyValue))
    {
        return PyErr_NoMemory();
    }
    Py_ssize_t count = npos + nkw;
    if (!_PyCFunction_CanCallScoped(callable)) {
        /* Eligibility is decided before invoking any body. The unchanged
         * object array is still owned by this mechanism's native caller. */
        return PyObject_Vectorcall(callable, args + 1, npos, kwnames);
    }
    PyValue *values = count == 0 ? NULL :
        PyMem_Malloc((size_t)count * sizeof(PyValue));
    if (count != 0 && values == NULL) {
        return PyErr_NoMemory();
    }
    for (Py_ssize_t i = 0; i < count; i++) {
        values[i] = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(args[i + 1]));
    }
    PyObject *result = vector_mechanism(callable, values, npos, count,
                                        kwnames, 0);
    PyObject *saved = PyErr_GetRaisedException();
    for (Py_ssize_t i = count; i-- > 0;) {
        PyValue value = values[i];
        values[i] = PyValue_NULL;
        PyValue_CLOSE(value);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    PyMem_Free(values);
    _PyValue_RestoreException(saved);
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
vector_fault_mechanism(PyObject *callable, int fault)
{
    PyValue values[2] = {PyValue_FromInt32(1), PyValue_NULL};
    values[1] = PyValue_FromInt32(fault == 1 ? 2 : INT32_MAX);
    PyObject *result = vector_mechanism(callable, values, 2, 2, NULL, fault);
    PyObject *saved = PyErr_GetRaisedException();
    for (int i = 1; i >= 0; i--) {
        PyValue owner = values[i];
        values[i] = PyValue_NULL;
        PyValue_XCLOSE(owner);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    PyErr_SetRaisedException(saved);
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
native_scope_vector_alloc_oom(PyObject *Py_UNUSED(module), PyObject *callable)
{
    return vector_fault_mechanism(callable, 1);
}

static PyObject *
native_scope_vector_box_oom(PyObject *Py_UNUSED(module), PyObject *callable)
{
    return vector_fault_mechanism(callable, 2);
}

static PyObject *
native_scope_vector_can_call(PyObject *Py_UNUSED(module), PyObject *callable)
{
    return PyBool_FromLong(_PyCFunction_CanCallScoped(callable));
}

static PyObject *
vector_override(PyObject *callable, PyObject *const *args, size_t nargsf,
                 PyObject *kwnames)
{
    return PyUnicode_FromString("custom vectorcall");
}

static PyObject *
native_scope_vector_override(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    static PyMethodDef method = {"scoped_override", native_scope_echo, METH_O};
    PyObject *callable = PyCFunction_New(&method, NULL);
    if (callable != NULL) {
        ((PyCFunctionObject *)callable)->vectorcall = vector_override;
    }
    return callable;
}

static PyObject *
native_scope_vector_prior_error(PyObject *Py_UNUSED(module), PyObject *callable)
{
    PyObject *primary = PyObject_CallFunction(PyExc_ValueError, "s",
                                             "native vector entry error");
    if (primary == NULL) {
        return NULL;
    }
    PyErr_SetRaisedException(Py_NewRef(primary));
    _PyNativeScope scope;
    int status = _PyNativeScope_BeginVector(
        &scope, PyThreadState_Get(), callable, NULL, 0, 0, NULL);
    PyValue destination = PyValue_NULL;
    Publication publication = {.destination = &destination};
    _PyNativeResult_Abort(&scope.result, publish_state, &publication);
    (void)_PyNativeScope_End(&scope);
    PyObject *raised = PyErr_GetRaisedException();
    int correct = status == -1 && raised == primary;
    Py_XDECREF(raised);
    if (!correct) {
        Py_DECREF(primary);
        PyErr_SetString(PyExc_AssertionError, "native vector entry error changed");
        return NULL;
    }
    PyErr_SetRaisedException(primary);
    return NULL;
}

static PyObject *
native_scope_vector_counts(PyObject *Py_UNUSED(module), PyObject *callable)
{
    Py_ssize_t counts[][2] = {
        {-1, 0}, {1, 0}, {0, 1}, {PY_SSIZE_T_MAX, PY_SSIZE_T_MAX},
    };
    PyValue input = PyValue_FromInt32(1);
    for (size_t i = 0; i < Py_ARRAY_LENGTH(counts); i++) {
        _PyNativeScope scope;
        int status = _PyNativeScope_BeginVector(
            &scope, PyThreadState_Get(), callable, &input,
            counts[i][0], counts[i][1], NULL);
        int correct = status == -1 && PyErr_Occurred();
        PyValue destination = PyValue_NULL;
        Publication publication = {.destination = &destination};
        _PyNativeResult_Abort(&scope.result, publish_state, &publication);
        (void)_PyNativeScope_End(&scope);
        PyErr_Clear();
        if (!correct) {
            PyValue_CLOSE(input);
            PyErr_SetString(PyExc_AssertionError,
                            "native vector invalid count was accepted");
            return NULL;
        }
    }
    PyValue_CLOSE(input);
    Py_RETURN_NONE;
}

typedef struct {
    PyMemAllocatorEx saved;
    PyMemAllocatorDomain domain;
    PyCFunctionObject *callable;
    int consumed;
} VectorTargetChange;

static void
vector_change_target(VectorTargetChange *change)
{
    change->consumed = 1;
    PyMem_SetAllocator(change->domain, &change->saved);
    change->callable->vectorcall = vector_override;
}

static void *
vector_change_malloc(void *opaque, size_t size)
{
    VectorTargetChange *change = opaque;
    vector_change_target(change);
    return change->saved.malloc(change->saved.ctx, size);
}

static void *
vector_change_calloc(void *opaque, size_t count, size_t size)
{
    VectorTargetChange *change = opaque;
    vector_change_target(change);
    return change->saved.calloc(change->saved.ctx, count, size);
}

static void *
vector_change_realloc(void *opaque, void *pointer, size_t size)
{
    VectorTargetChange *change = opaque;
    vector_change_target(change);
    return change->saved.realloc(change->saved.ctx, pointer, size);
}

static void
vector_change_free(void *opaque, void *pointer)
{
    VectorTargetChange *change = opaque;
    change->saved.free(change->saved.ctx, pointer);
}

static PyObject *
native_scope_vector_target_change(PyObject *Py_UNUSED(module), PyObject *callable)
{
    if (!_PyCFunction_CanCallScoped(callable)) {
        PyErr_SetString(PyExc_TypeError, "a standard C callable is required");
        return NULL;
    }
    PyCFunctionObject *function = (PyCFunctionObject *)callable;
    vectorcallfunc original = function->vectorcall;
    for (int phase = 0; phase < 2; phase++) {
        PyValue values[2] = {PyValue_FromInt32(INT32_MAX), PyValue_FromInt32(1)};
        VectorTargetChange change = {
            .domain = phase == 0 ? PYMEM_DOMAIN_MEM : PYMEM_DOMAIN_OBJ,
            .callable = function,
        };
        PyMem_GetAllocator(change.domain, &change.saved);
        PyMemAllocatorEx allocator = {
            .ctx = &change,
            .malloc = vector_change_malloc,
            .calloc = vector_change_calloc,
            .realloc = vector_change_realloc,
            .free = vector_change_free,
        };
        if (phase == 0) {
            PyMem_SetAllocator(change.domain, &allocator);
        }
        _PyNativeScope scope;
        int status = _PyNativeScope_BeginVector(
            &scope, PyThreadState_Get(), callable, values, 2, 2, NULL);
        PyMem_SetAllocator(change.domain, &change.saved);
        if (status == 0) {
            if (phase == 1) {
                PyMem_SetAllocator(change.domain, &allocator);
            }
            status = _PyCFunction_CallScoped(&scope, callable, &scope.result);
            PyMem_SetAllocator(change.domain, &change.saved);
        }
        int correct = change.consumed && status == 1 && !PyErr_Occurred() &&
                      scope.result.state == _Py_NATIVE_RESULT_EMPTY;
        function->vectorcall = original;
        PyValue destination = PyValue_NULL;
        Publication publication = {.destination = &destination};
        _PyNativeResult_Abort(&scope.result, publish_state, &publication);
        (void)_PyNativeScope_End(&scope);
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_CLOSE(values[1]);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
        PyValue_CLOSE(values[0]);
        _PyValue_RestoreException(saved);
        if (!correct || PyErr_Occurred()) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_AssertionError,
                                "changed vector target was invoked");
            }
            return NULL;
        }
    }
    Py_RETURN_NONE;
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

#ifdef Py_VALUE_INLINE_VALUES
typedef struct {
    PyMemAllocatorEx saved;
    void *target;
    /* Borrowed from the immutable bound-self tuple, which also owns this
     * record's capsule. There is no hidden owning ledger edge. */
    PyObject *ledger;
    int armed;
} EndFailure;

static void *
end_malloc(void *opaque, size_t size)
{
    EndFailure *failure = opaque;
    return failure->saved.malloc(failure->saved.ctx, size);
}

static void *
end_calloc(void *opaque, size_t count, size_t size)
{
    EndFailure *failure = opaque;
    return failure->saved.calloc(failure->saved.ctx, count, size);
}

static void *
end_realloc(void *opaque, void *pointer, size_t size)
{
    EndFailure *failure = opaque;
    return failure->saved.realloc(failure->saved.ctx, pointer, size);
}

static void
end_free(void *opaque, void *pointer)
{
    EndFailure *failure = opaque;
    if (!failure->armed || pointer != failure->target) {
        failure->saved.free(failure->saved.ctx, pointer);
        return;
    }
    /* The receiver/callable pins the heap record across body return and End.
     * Restore the allocator before forwarding or doing anything reentrant. */
    PyMemAllocatorEx saved = failure->saved;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &saved);
    failure->armed = 0;
    failure->target = NULL;
    PyObject *entry_error = PyErr_GetRaisedException();
    saved.free(saved.ctx, pointer);
    PyObject *marker = PyErr_Occurred() ? NULL :
        PyUnicode_FromString("end cleanup");
    if (marker != NULL) {
        PyList_Append(failure->ledger, marker);
        PyObject *error = PyErr_GetRaisedException();
        Py_DECREF(marker);
        _PyValue_RestoreException(error);
    }
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError, "native scope End cleanup");
    }
    PyErr_FormatUnraisable("Exception ignored by native scope retired-box fixture");
    PyErr_SetRaisedException(entry_error);
}

static void
end_failure_destroy(PyObject *capsule)
{
    PyObject *entry_error = PyErr_GetRaisedException();
    EndFailure *failure = PyCapsule_GetPointer(capsule,
                                              "native_scope.end_failure");
    assert(failure != NULL);
    /* An unconsumed hook must never retain a pointer to a freed record. */
    if (failure->armed) {
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure->saved);
    }
    PyMem_Free(failure);
    if (PyErr_Occurred()) {
        PyErr_FormatUnraisable("Exception ignored by native scope record fixture");
    }
    PyErr_SetRaisedException(entry_error);
}

static PyObject *
end_failure_body(PyObject *self, PyObject *argument)
{
    if (argument == Py_None) {
        return PyLong_FromLong(INT32_MAX);
    }
    if (argument != Py_True) {
        PyErr_SetString(PyExc_TypeError, "expected True or None");
        return NULL;
    }
    PyObject *capsule = _PyTuple_GET_OBJECT(self, 0);
    EndFailure *failure = PyCapsule_GetPointer(capsule,
                                              "native_scope.end_failure");
    if (failure == NULL) {
        return NULL;
    }
    assert(!failure->armed);
    PyObject *marker = PyUnicode_FromString("body");
    if (marker == NULL) {
        return NULL;
    }
    int status = PyList_Append(failure->ledger, marker);
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(marker);
    _PyValue_RestoreException(saved);
    if (status < 0 || PyErr_Occurred()) {
        return NULL;
    }
    /* Two digits bypass the one-digit free list. The exact public OBJ free
     * pointer is observed without peeking through debug allocator prefixes. */
    PyObject *result = PyLong_FromLong(INT32_MAX);
    if (result == NULL) {
        return NULL;
    }
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_DECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure->saved);
    failure->target = result;
    failure->armed = 1;
    PyMemAllocatorEx wrapper = {
        .ctx = failure,
        .malloc = end_malloc,
        .calloc = end_calloc,
        .realloc = end_realloc,
        .free = end_free,
    };
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &wrapper);
    return result;
}
#endif

static PyObject *
native_scope_end_failure(PyObject *Py_UNUSED(module), PyObject *ledger)
{
#ifndef Py_VALUE_INLINE_VALUES
    PyErr_SetString(PyExc_NotImplementedError, "requires inline scalar values");
    return NULL;
#else
    if (!PyList_CheckExact(ledger)) {
        PyErr_SetString(PyExc_TypeError, "expected an exact ledger list");
        return NULL;
    }
    EndFailure *failure = PyMem_Calloc(1, sizeof(*failure));
    if (failure == NULL) {
        return PyErr_NoMemory();
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyMem_Free(failure);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    failure->ledger = ledger;
    PyObject *capsule = PyCapsule_New(failure, "native_scope.end_failure",
                                     end_failure_destroy);
    if (capsule == NULL) {
        PyObject *saved = PyErr_GetRaisedException();
        PyMem_Free(failure);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(capsule);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    PyObject *receiver = _PyTuple_FromPair(capsule, ledger);
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(capsule);
    _PyValue_RestoreException(saved);
    if (receiver == NULL || PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(receiver);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    static PyMethodDef method = {"native_scope_end_failure_body",
                                 end_failure_body, METH_O};
    PyObject *function = PyCFunction_New(&method, receiver);
    saved = PyErr_GetRaisedException();
    Py_DECREF(receiver);
    _PyValue_RestoreException(saved);
    if (function != NULL && PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_DECREF(function);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return function;
#endif
}

typedef struct {
    PyMemAllocatorEx saved;
    PyThreadState *tstate;
    unsigned long thread;
    PyCodeObject *driver_code;
    PyCFunctionObject *target;
    PyObject *callback;
    vectorcallfunc original;
    int fired;
    int handler;
    int target_calls;
    int callback_calls;
} NativeVMProbe;

/* GIL-only, stack-scoped test request. Foreign-thread allocations only
 * forward; an override copies its target before any call can release GIL. */
static NativeVMProbe *native_vm_probe;

static PyObject *
native_vm_override(PyObject *callable, PyObject *const *args,
                   size_t nargsf, PyObject *kwnames)
{
    NativeVMProbe *probe = native_vm_probe;
    assert(probe != NULL && callable == (PyObject *)probe->target);
    vectorcallfunc original = probe->original;
    if (PyThread_get_thread_ident() == probe->thread) {
        probe->target_calls++;
    }
    return original(callable, args, nargsf, kwnames);
}

static void *
native_vm_malloc(void *opaque, size_t size)
{
    NativeVMProbe *probe = opaque;
    if (PyThread_get_thread_ident() == probe->thread &&
        size == 2 * (sizeof(PyValue) + sizeof(PyObject *)) &&
        PyThreadState_GetUnchecked() == probe->tstate) {
        _PyInterpreterFrame *frame = probe->tstate->current_frame;
        if (frame != NULL && frame->owner != FRAME_OWNED_BY_INTERPRETER &&
            _PyFrame_GetCode(frame) == probe->driver_code &&
            IR_HANDLER(*frame->instr_ptr) == CALL_KW_NON_PY) {
            /* BeginVector is the first MEM allocation in this selected
             * no-growing handler; the source window is already strong. */
            probe->fired = 1;
            probe->handler = CALL_KW_NON_PY;
            PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &probe->saved);
            probe->target->vectorcall = native_vm_override;
            if (probe->callback != Py_None) {
                probe->callback_calls++;
                PyObject *result = PyObject_CallNoArgs(probe->callback);
                PyObject *saved = PyErr_GetRaisedException();
                Py_XDECREF(result);
                _PyValue_RestoreException(saved);
            }
        }
    }
    return probe->saved.malloc(probe->saved.ctx, size);
}

static void *
native_vm_calloc(void *opaque, size_t count, size_t size)
{
    NativeVMProbe *probe = opaque;
    return probe->saved.calloc(probe->saved.ctx, count, size);
}

static void *
native_vm_realloc(void *opaque, void *pointer, size_t size)
{
    NativeVMProbe *probe = opaque;
    return probe->saved.realloc(probe->saved.ctx, pointer, size);
}

static void
native_vm_free(void *opaque, void *pointer)
{
    NativeVMProbe *probe = opaque;
    probe->saved.free(probe->saved.ctx, pointer);
}

static PyObject *
native_scope_vm_prepare(PyObject *Py_UNUSED(module), PyObject *const *args,
                        Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs != 2 || kwnames == NULL || PyTuple_GET_SIZE(kwnames) != 1) {
        PyErr_SetString(PyExc_TypeError, "expected driver, target, callback= keyword");
        return NULL;
    }
    PyValue name = PyTuple_GET_VALUE(kwnames, 0);
    if (!PyValue_IsObject(name) ||
        !PyUnicode_Check(PyValue_AsPyObjectBorrow(name)) ||
        PyUnicode_CompareWithASCIIString(PyValue_AsPyObjectBorrow(name), "callback") != 0 ||
        !PyFunction_Check(args[0]) ||
        !_PyCFunction_CanCallScoped(args[1]) ||
        (args[2] != Py_None && !PyCallable_Check(args[2]))) {
        PyErr_SetString(PyExc_TypeError, "expected driver, standard C target, callback or None");
        return NULL;
    }
    if (native_vm_probe != NULL) {
        PyErr_SetString(PyExc_RuntimeError, "native VM probe is already active");
        return NULL;
    }
    PyObject *driver = Py_NewRef(args[0]);
    PyObject *target = Py_NewRef(args[1]);
    PyObject *callback = Py_NewRef(args[2]);
    NativeVMProbe probe = {
        .tstate = PyThreadState_Get(),
        .thread = PyThread_get_thread_ident(),
        .driver_code = (PyCodeObject *)PyFunction_GET_CODE(driver),
        .target = (PyCFunctionObject *)target,
        .callback = callback,
        .original = ((PyCFunctionObject *)target)->vectorcall,
    };
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &probe.saved);
    PyMemAllocatorEx allocator = {
        .ctx = &probe, .malloc = native_vm_malloc, .calloc = native_vm_calloc,
        .realloc = native_vm_realloc, .free = native_vm_free,
    };
    native_vm_probe = &probe;
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &allocator);
    PyObject *result = PyObject_CallNoArgs(driver);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &probe.saved);
    probe.target->vectorcall = probe.original;
    native_vm_probe = NULL;
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(callback);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_DECREF(target);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_DECREF(driver);
    _PyValue_RestoreException(saved);
    if (result == NULL || PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return Py_BuildValue("(Niiii)", result, probe.fired, probe.handler,
                         probe.target_calls, probe.callback_calls);
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
    switch (handler) {
        case CALL_FUNCTION_EX: case INSTRUMENTED_CALL_FUNCTION_EX:
        case CALL_EX_PY: case CALL_EX_NON_PY_GENERAL:
            count = 4;
            break;
        case CALL_KW: case INSTRUMENTED_CALL_KW: case CALL_KW_PY:
        case CALL_KW_BOUND_METHOD: case CALL_KW_NON_PY:
            count = argc + 3;
            break;
        case CALL: case CALL_JIT_WARM: case INSTRUMENTED_CALL:
        case CALL_PY_GENERAL: case CALL_BOUND_METHOD_GENERAL:
        case CALL_NON_PY_GENERAL:
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
            if (PyValue_IsHeapSafe(value)) {
                strong |= UINT64_C(1) << i;
            }
        }
        snapshots[i] = PyValue_MakeHeapSafe(PyValue_Borrow(value));
    }
    int nlocals = code->co_nlocalsplus;
    PyObject *values = PyTuple_New(nslots);
    PyObject *report = NULL;
    if (values != NULL && !PyErr_Occurred()) {
        for (int i = 0; i < nslots; i++) {
            PyTuple_SET_VALUE(values, i, PyValue_IsNull(snapshots[i])
                ? PyValue_FromPyObjectNew(Py_None) : PyValue_NewRef(snapshots[i]));
        }
        report = Py_BuildValue("(iniiiiOKK)", handler, word_index, nlocals,
            base, count, destination, values,
            (unsigned long long)present, (unsigned long long)strong);
    }
    PyObject *saved = PyErr_GetRaisedException();
    Py_XDECREF(values);
    _PyValue_RestoreException(saved);
    for (int i = nslots; i-- > 0;) {
        saved = PyErr_GetRaisedException();
        PyValue owner = snapshots[i];
        snapshots[i] = PyValue_NULL;
        PyValue_XCLOSE(owner);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(report);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return report;
}

static PyObject *
flat_format_views(PyObject *Py_UNUSED(module), PyObject *arg)
{
    PyObject *outer = NULL, *args = NULL, *callback = NULL;
    PyObject *callback_result = NULL, *copied_text = NULL, *count_obj = NULL;
    PyObject *result = NULL, *result_value = NULL, *result_text = NULL;
    PyObject *result_count = NULL, *value = NULL;
    const char *text = NULL;
    Py_ssize_t count = -7;
    _PyArg_OwnerScope scope = {0};

    if (PyErr_Occurred()) return NULL;
    if (!PyTuple_CheckExact(arg) || PyTuple_GET_SIZE(arg) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected (parse_args, callback)");
        goto done;
    }
    outer = Py_NewRef(arg);
    if (outer == NULL || PyErr_Occurred()) goto done;
    args = PyTuple_GetItemRef(outer, 0);
    if (args == NULL || PyErr_Occurred()) goto done;
    callback = PyTuple_GetItemRef(outer, 1);
    if (callback == NULL || PyErr_Occurred()) goto done;
    if (!PyTuple_Check(args) || !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "expected a tuple and callable");
        goto done;
    }
    int parsed = _PyArg_ParseTupleOwned(&scope, args, "Os|n:flat_format_views",
                                        &value, &text, &count);
    if (!parsed || PyErr_Occurred()) goto done;
    callback_result = PyObject_CallNoArgs(callback);
    if (callback_result == NULL || PyErr_Occurred()) goto done;
    {
        PyObject *detached = callback_result;
        callback_result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) goto done;
    copied_text = PyUnicode_FromString(text);
    if (copied_text == NULL || PyErr_Occurred()) goto done;
    count_obj = PyLong_FromSsize_t(count);
    if (count_obj == NULL || PyErr_Occurred()) goto done;
    result = PyTuple_New(3);
    if (result == NULL || PyErr_Occurred()) goto done;
    result_value = Py_NewRef(value);
    if (result_value == NULL || PyErr_Occurred()) goto done;
    PyTuple_SET_ITEM(result, 0, result_value);
    result_value = NULL;
    if (PyErr_Occurred()) goto done;
    result_text = Py_NewRef(copied_text);
    if (result_text == NULL || PyErr_Occurred()) goto done;
    PyTuple_SET_ITEM(result, 1, result_text);
    result_text = NULL;
    if (PyErr_Occurred()) goto done;
    result_count = Py_NewRef(count_obj);
    if (result_count == NULL || PyErr_Occurred()) goto done;
    PyTuple_SET_ITEM(result, 2, result_count);
    result_count = NULL;
    if (PyErr_Occurred()) goto done;
done:
    {
        PyObject *detached = callback_result;
        callback_result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = copied_text;
        copied_text = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = count_obj;
        count_obj = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = result_value;
        result_value = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = result_text;
        result_text = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = result_count;
        result_count = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = outer;
        outer = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = args;
        args = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = callback;
        callback = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    if (_PyArg_OwnerScope_End(&scope) < 0 || PyErr_Occurred()) {
        PyObject *detached = result;
        result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
flat_format_rejects_unsupported(PyObject *Py_UNUSED(module),
                                PyObject *Py_UNUSED(unused))
{
    static const char *formats[] = {"(O)", "O&", "s*", "s#", "y", "e",
                                    "O||O", "q", NULL};
    PyObject *empty = NULL, *result = NULL;
    PyObject *exception = NULL, *message = NULL;
    _PyArg_OwnerScope scope = {0};
    int scope_ended = 1;

    if (PyErr_Occurred()) return NULL;
    empty = PyTuple_New(0);
    if (empty == NULL || PyErr_Occurred()) goto error;
    result = PyTuple_New(8);
    if (result == NULL || PyErr_Occurred()) goto error;
    for (Py_ssize_t i = 0; formats[i] != NULL; i++) {
        scope = (_PyArg_OwnerScope){0};
        scope_ended = 0;
        int ok = _PyArg_ParseTupleOwned(&scope, empty, formats[i]);
        if (ok) {
            if (PyErr_Occurred()) goto iteration_error;
            PyErr_SetString(PyExc_AssertionError,
                            "unsupported format was not rejected");
            goto iteration_error;
        }
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError,
                            "unsupported format was not rejected");
            goto iteration_error;
        }
        exception = PyErr_GetRaisedException();
        if (exception == NULL || PyErr_Occurred()) goto iteration_error;
        message = PyObject_Str(exception);
        if (message == NULL || PyErr_Occurred()) goto iteration_error;
        {
            PyObject *detached = exception;
            exception = NULL;
            PyObject *saved = PyErr_GetRaisedException();
            Py_XDECREF(detached);
            _PyValue_RestoreException(saved);
        }
        if (PyErr_Occurred()) goto iteration_error;
        PyTuple_SET_ITEM(result, i, message);
        message = NULL;
        if (PyErr_Occurred()) goto iteration_error;
        if (_PyArg_OwnerScope_End(&scope) < 0 || PyErr_Occurred()) {
            scope_ended = 1;
            goto error;
        }
        scope_ended = 1;
        continue;
iteration_error:
        if (!scope_ended) {
            _PyArg_OwnerScope_End(&scope);
            scope_ended = 1;
        }
        {
            PyObject *detached = message;
            message = NULL;
            PyObject *saved = PyErr_GetRaisedException();
            Py_XDECREF(detached);
            _PyValue_RestoreException(saved);
        }
        {
            PyObject *detached = exception;
            exception = NULL;
            PyObject *saved = PyErr_GetRaisedException();
            Py_XDECREF(detached);
            _PyValue_RestoreException(saved);
        }
        goto error;
    }
    {
        PyObject *detached = empty;
        empty = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) goto error;
    return result;
error:
    {
        PyObject *detached = message;
        message = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = exception;
        exception = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = empty;
        empty = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = result;
        result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    return NULL;
}

static PyObject *
flat_format_typed_nullable(PyObject *Py_UNUSED(module), PyObject *arg)
{
    PyObject *outer = NULL, *args = NULL, *callback = NULL;
    PyObject *callback_result = NULL, *text_copy = NULL, *result = NULL;
    PyObject *result_value = NULL, *result_text = NULL, *value = NULL;
    const char *text = NULL;
    _PyArg_OwnerScope scope = {0};

    if (PyErr_Occurred()) return NULL;
    if (!PyTuple_CheckExact(arg) || PyTuple_GET_SIZE(arg) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected (parse_args, callback)");
        goto done;
    }
    outer = Py_NewRef(arg);
    if (outer == NULL || PyErr_Occurred()) goto done;
    args = PyTuple_GetItemRef(outer, 0);
    if (args == NULL || PyErr_Occurred()) goto done;
    callback = PyTuple_GetItemRef(outer, 1);
    if (callback == NULL || PyErr_Occurred()) goto done;
    if (!PyTuple_Check(args) || !PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "expected a tuple and callable");
        goto done;
    }
    int parsed = _PyArg_ParseTupleOwned(&scope, args,
        "O!z:flat_format_typed_nullable", &PyLong_Type, &value, &text);
    if (!parsed || PyErr_Occurred()) goto done;
    callback_result = PyObject_CallNoArgs(callback);
    if (callback_result == NULL || PyErr_Occurred()) goto done;
    {
        PyObject *detached = callback_result;
        callback_result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) goto done;
    text_copy = text == NULL ? Py_NewRef(Py_None) : PyUnicode_FromString(text);
    if (text_copy == NULL || PyErr_Occurred()) goto done;
    result = PyTuple_New(2);
    if (result == NULL || PyErr_Occurred()) goto done;
    result_value = Py_NewRef(value);
    if (result_value == NULL || PyErr_Occurred()) goto done;
    PyTuple_SET_ITEM(result, 0, result_value);
    result_value = NULL;
    if (PyErr_Occurred()) goto done;
    result_text = Py_NewRef(text_copy);
    if (result_text == NULL || PyErr_Occurred()) goto done;
    PyTuple_SET_ITEM(result, 1, result_text);
    result_text = NULL;
    if (PyErr_Occurred()) goto done;
done:
    {
        PyObject *detached = callback_result;
        callback_result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = text_copy;
        text_copy = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = result_value;
        result_value = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = result_text;
        result_text = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = outer;
        outer = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = args;
        args = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *detached = callback;
        callback = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
    }
    if (_PyArg_OwnerScope_End(&scope) < 0 || PyErr_Occurred()) {
        PyObject *detached = result;
        result = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(detached);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyMethodDef methods[] = {
    {"flat_format_views", flat_format_views, METH_O},
    {"flat_format_rejects_unsupported", flat_format_rejects_unsupported, METH_NOARGS},
    {"flat_format_typed_nullable", flat_format_typed_nullable, METH_O},
    {"native_scope_call_state", native_scope_call_state, METH_O},
    {"native_scope_vm_prepare", _PyCFunction_CAST(native_scope_vm_prepare), METH_FASTCALL | METH_KEYWORDS},
    _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_METHODDEF
    _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_NOARGS_METHODDEF
    _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_FAST_METHODDEF
    {"native_scope_vector", _PyCFunction_CAST(native_scope_vector),
     METH_FASTCALL | METH_KEYWORDS},
    {"native_scope_vector_alloc_oom", native_scope_vector_alloc_oom, METH_O},
    {"native_scope_vector_box_oom", native_scope_vector_box_oom, METH_O},
    {"native_scope_vector_can_call", native_scope_vector_can_call, METH_O},
    {"native_scope_vector_override", native_scope_vector_override, METH_NOARGS},
    {"native_scope_vector_prior_error", native_scope_vector_prior_error, METH_O},
    {"native_scope_vector_counts", native_scope_vector_counts, METH_O},
    {"native_scope_vector_target_change", native_scope_vector_target_change, METH_O},
    {"native_scope_vector_method", native_scope_vector_method, METH_NOARGS},
    {"native_scope_publish", native_scope_publish, METH_O},
    {"native_scope_abort", native_scope_abort, METH_O},
    {"native_scope_cleanup_error", native_scope_cleanup_error, METH_O},
    {"native_scope_primary_error", native_scope_primary_error, METH_O},
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
    {"native_scope_end_failure", native_scope_end_failure, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_NativeScope(PyObject *module)
{
    return PyModule_AddFunctions(module, methods);
}
