#include "Python.h"
#include "pycore_borrow.h"        // _PyBorrowScope_Enter()
#include "pycore_call.h"          // _PyObject_CallNoArgsTstate()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_dict.h"          // _PyDict_NewPresized()
#include "pycore_function.h"      // _PyFunction_Vectorcall() definition
#include "pycore_modsupport.h"    // _Py_VaBuildStack()
#include "pycore_object.h"        // _PyCFunctionWithKeywords_TrampolineCall()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_GET_OBJECT()
#include "pycore_value.h"         // Owning object bridges and cleanup
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()


static void
call_free_buffer(void *buffer)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyMem_Free(buffer);
    _PyValue_RestoreException(saved);
}

static void
call_pop_value(PyThreadState *tstate, _PyCValue *value)
{
    PyObject *saved = PyErr_GetRaisedException();
    _PyThreadState_PopCValue(tstate, value);
    _PyValue_RestoreException(saved);
}

static PyObject *
call_finish_result(PyObject *result)
{
    if (PyErr_Occurred()) {
        Py_CLEAR(result);
    }
    return result;
}

static PyObject *
null_error(PyThreadState *tstate)
{
    if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                         "null argument to internal routine");
    }
    return NULL;
}


PyObject*
_Py_CheckFunctionResult(PyThreadState *tstate, PyObject *callable,
                        PyObject *result, const char *where)
{
    assert((callable != NULL) ^ (where != NULL));

    if (result == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            if (callable)
                _PyErr_Format(tstate, PyExc_SystemError,
                              "%R returned NULL without setting an exception",
                              callable);
            else
                _PyErr_Format(tstate, PyExc_SystemError,
                              "%s returned NULL without setting an exception",
                              where);
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode.
               Py_FatalError() logs the SystemError exception raised above. */
            Py_FatalError("a function returned NULL without setting an exception");
#endif
            return NULL;
        }
    }
    else {
        if (_PyErr_Occurred(tstate)) {
            Py_DECREF(result);

            if (callable) {
                _PyErr_FormatFromCauseTstate(
                    tstate, PyExc_SystemError,
                    "%R returned a result with an exception set", callable);
            }
            else {
                _PyErr_FormatFromCauseTstate(
                    tstate, PyExc_SystemError,
                    "%s returned a result with an exception set", where);
            }
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode.
               Py_FatalError() logs the SystemError exception raised above. */
            Py_FatalError("a function returned a result with an exception set");
#endif
            return NULL;
        }
    }
    return result;
}


int
_Py_CheckSlotResult(PyObject *obj, const char *slot_name, int success)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!success) {
        if (!_PyErr_Occurred(tstate)) {
            _Py_FatalErrorFormat(__func__,
                                 "Slot %s of type %s failed "
                                 "without setting an exception",
                                 slot_name, Py_TYPE(obj)->tp_name);
        }
    }
    else {
        if (_PyErr_Occurred(tstate)) {
            _Py_FatalErrorFormat(__func__,
                                 "Slot %s of type %s succeeded "
                                 "with an exception set",
                                 slot_name, Py_TYPE(obj)->tp_name);
        }
    }
    return 1;
}


/* --- Core PyObject call functions ------------------------------- */

/* Call a callable Python object without any arguments */
PyObject *
PyObject_CallNoArgs(PyObject *func)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, func, NULL, 0, NULL);
}


PyObject *
_PyObject_VectorcallDictTstate(PyThreadState *tstate, PyObject *callable,
                               PyObject *const *args, size_t nargsf,
                               PyObject *kwargs)
{
    assert(callable != NULL && !PyErr_Occurred());
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0 && (nargs == 0 || args != NULL));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    PyObject *kwnames = NULL;
    PyObject *const *owned_stack = NULL;
    PyObject *result = NULL;
    if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
        owned_stack = _PyStack_UnpackDict(tstate, args, nargs, kwargs, &kwnames);
        if (owned_stack == NULL) {
            goto done;
        }
        args = owned_stack;
        nargsf = nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
    }
    /* Preparation can call out. Select the current ordinary protocol now. */
    vectorcallfunc func = PyVectorcall_Function(callable);
    if (func == NULL) {
        result = _PyObject_MakeTpCall(tstate, callable, args, nargs,
                                     owned_stack != NULL ? kwnames : kwargs);
    }
    else {
        result = func(callable, args, nargsf, kwnames);
    }
done:
    if (owned_stack != NULL) {
        _PyStack_UnpackDict_Free(owned_stack, nargs, kwnames);
    }
    return _Py_CheckFunctionResult(tstate, callable, call_finish_result(result), NULL);
}


PyObject *
PyObject_VectorcallDict(PyObject *callable, PyObject *const *args,
                       size_t nargsf, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallDictTstate(tstate, callable, args, nargsf, kwargs);
}

static void
object_is_not_callable(PyThreadState *tstate, PyObject *callable)
{
    if (Py_IS_TYPE(callable, &PyModule_Type)) {
        // >>> import pprint
        // >>> pprint(thing)
        // Traceback (most recent call last):
        //   File "<stdin>", line 1, in <module>
        // TypeError: 'module' object is not callable. Did you mean: 'pprint.pprint(...)'?
        PyObject *name = PyModule_GetNameObject(callable);
        if (name == NULL) {
            _PyErr_Clear(tstate);
            goto basic_type_error;
        }
        PyObject *attr;
        int res = PyObject_GetOptionalAttr(callable, name, &attr);
        if (res < 0) {
            _PyErr_Clear(tstate);
        }
        else if (res > 0 && PyCallable_Check(attr)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'%.200s' object is not callable. "
                          "Did you mean: '%U.%U(...)'?",
                          Py_TYPE(callable)->tp_name, name, name);
            Py_DECREF(attr);
            Py_DECREF(name);
            return;
        }
        Py_XDECREF(attr);
        Py_DECREF(name);
    }
basic_type_error:
    _PyErr_Format(tstate, PyExc_TypeError, "'%.200s' object is not callable",
                  Py_TYPE(callable)->tp_name);
}


PyObject *
_PyObject_MakeTpCall(PyThreadState *tstate, PyObject *callable,
                     PyObject *const *args, Py_ssize_t nargs,
                     PyObject *keywords)
{
    assert(nargs >= 0 && (nargs == 0 || args != NULL));
    assert(keywords == NULL || PyTuple_Check(keywords) || PyDict_Check(keywords));
    PyObject *argstuple = NULL;
    PyObject *kwdict = NULL;
    PyObject *result = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    /* Preserve the cheap noncallable rejection before preparation. */
    if (Py_TYPE(callable)->tp_call == NULL) {
        object_is_not_callable(tstate, callable);
        goto done;
    }
    argstuple = PyTuple_FromArray(args, nargs);
    if (argstuple == NULL) {
        goto done;
    }
    if (keywords == NULL || PyDict_Check(keywords)) {
        kwdict = Py_XNewRef(keywords);
    }
    else if (PyTuple_GET_SIZE(keywords) != 0) {
        assert(args != NULL);
        kwdict = _PyStack_AsDict(args + nargs, keywords);
        if (kwdict == NULL) {
            goto done;
        }
    }
    ternaryfunc call = Py_TYPE(callable)->tp_call;
    if (call == NULL) {
        object_is_not_callable(tstate, callable);
        goto done;
    }
    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object") == 0) {
        _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
        result = _PyCFunctionWithKeywords_TrampolineCall(
            (PyCFunctionWithKeywords)call, callable, argstuple, kwdict);
        _PyBorrowScope_Exit(tstate, mark);
        _Py_LeaveRecursiveCallTstate(tstate);
    }
done:
    Py_CLEAR(kwdict);
    Py_CLEAR(argstuple);
    return _Py_CheckFunctionResult(tstate, callable, call_finish_result(result), NULL);
}


vectorcallfunc
PyVectorcall_Function(PyObject *callable)
{
    return _PyVectorcall_FunctionInline(callable);
}


static PyObject *
_PyVectorcall_Call(PyThreadState *tstate, PyObject *callable,
                   PyObject *tuple, PyObject *kwargs, bool respect_flags)
{
    /* The tuple's items are lent to the callee as a borrow-stack run. */
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *const *stack = NULL;
    PyObject *kwnames = NULL;
    PyObject *result = NULL;
    Py_ssize_t nargs = PyTuple_GET_SIZE(tuple);
    PyObject *const *args = _PyBorrow_TupleItems(tuple, nargs);
    if (args == NULL) {
        goto done;
    }
    size_t nargsf = nargs;
    if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
        stack = _PyStack_UnpackDict(tstate, args, nargs, kwargs, &kwnames);
        if (stack == NULL) {
            goto done;
        }
        args = stack;
        nargsf |= PY_VECTORCALL_ARGUMENTS_OFFSET;
    }
    vectorcallfunc func = NULL;
    if (respect_flags) {
        func = PyVectorcall_Function(callable);
        if (func == NULL) {
            /* Ordinary Call retains its original tuple/dict input contract. */
            ternaryfunc call = Py_TYPE(callable)->tp_call;
            if (call == NULL) {
                object_is_not_callable(tstate, callable);
                goto done;
            }
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object") == 0) {
                result = call(callable, tuple, kwargs);
                _Py_LeaveRecursiveCallTstate(tstate);
            }
            goto done;
        }
    }
    else {
        /* Public PyVectorcall_Call intentionally ignores HAVE_VECTORCALL. */
        Py_ssize_t offset = Py_TYPE(callable)->tp_vectorcall_offset;
        if (offset > 0) {
            memcpy(&func, (char *)callable + offset, sizeof(func));
        }
        if (func == NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'%.200s' object does not support vectorcall",
                          Py_TYPE(callable)->tp_name);
            goto done;
        }
    }
    result = func(callable, args, nargsf, kwnames);
done:
    if (stack != NULL) {
        _PyStack_UnpackDict_Free(stack, nargs, kwnames);
    }
    _PyBorrowScope_Exit(tstate, mark);
    return _Py_CheckFunctionResult(tstate, callable, call_finish_result(result), NULL);
}


PyObject *
PyVectorcall_Call(PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();

    /* get vectorcallfunc as in _PyVectorcall_Function, but without
     * the Py_TPFLAGS_HAVE_VECTORCALL check */
    Py_ssize_t offset = Py_TYPE(callable)->tp_vectorcall_offset;
    if (offset <= 0) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }
    assert(PyCallable_Check(callable));

    vectorcallfunc func;
    memcpy(&func, (char *) callable + offset, sizeof(func));
    if (func == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }

    return _PyVectorcall_Call(tstate, callable, tuple, kwargs, false);
}


PyObject *
PyObject_Vectorcall(PyObject *callable, PyObject *const *args,
                     size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, callable,
                                      args, nargsf, kwnames);
}


PyObject *
_PyObject_Call(PyThreadState *tstate, PyObject *callable,
               PyObject *args, PyObject *kwargs)
{
    ternaryfunc call;
    PyObject *result;

    /* PyObject_Call() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
    assert(PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    vectorcallfunc vector_func = PyVectorcall_Function(callable);
    if (vector_func != NULL) {
        return _PyVectorcall_Call(tstate, callable, args, kwargs, true);
    }
    else {
        call = Py_TYPE(callable)->tp_call;
        if (call == NULL) {
            object_is_not_callable(tstate, callable);
            return NULL;
        }

        if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
            return NULL;
        }

        _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
        result = (*call)(callable, args, kwargs);
        _PyBorrowScope_Exit(tstate, mark);

        _Py_LeaveRecursiveCallTstate(tstate);

        return _Py_CheckFunctionResult(tstate, callable, call_finish_result(result), NULL);
    }
}

PyObject *
PyObject_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_Call(tstate, callable, args, kwargs);
}


/* Function removed in the Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject *)
PyCFunction_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    return PyObject_Call(callable, args, kwargs);
}


PyObject *
PyObject_CallOneArg(PyObject *func, PyObject *arg)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    assert(arg != NULL);
    PyObject *_args[2];
    PyObject **args = _args + 1;  // For PY_VECTORCALL_ARGUMENTS_OFFSET
    args[0] = arg;
    PyThreadState *tstate = _PyThreadState_GET();
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return _PyObject_VectorcallTstate(tstate, func, args, nargsf, NULL);
}


/* --- PyFunction call functions ---------------------------------- */

PyObject *
_PyFunction_Vectorcall(PyObject *func, PyObject* const* stack,
                       size_t nargsf, PyObject *kwnames)
{
    assert(PyFunction_Check(func));
    PyFunctionObject *f = (PyFunctionObject *)func;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    PyThreadState *tstate = _PyThreadState_GET();
    assert(nargs == 0 || stack != NULL);
    EVAL_CALL_STAT_INC(EVAL_CALL_FUNCTION_VECTORCALL);
    if (((PyCodeObject *)f->func_code)->co_flags & CO_OPTIMIZED) {
        return _PyEval_Vector(tstate, f, NULL, stack, nargs, kwnames);
    }
    else {
        return _PyEval_Vector(tstate, f, f->func_globals, stack, nargs, kwnames);
    }
}

/* --- More complex call functions -------------------------------- */

/* External interface to call any callable object.
   The args must be a tuple or NULL.  The kwargs must be a dict or NULL.
   Function removed in Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject*)
PyEval_CallObjectWithKeywords(PyObject *callable,
                              PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyEval_CallObjectWithKeywords() must not be called with an exception
       set. It raises a new exception if parameters are invalid or if
       PyTuple_New() fails, and so the original exception is lost. */
    assert(!_PyErr_Occurred(tstate));
#endif

    if (args != NULL && !PyTuple_Check(args)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "argument list must be a tuple");
        return NULL;
    }

    if (kwargs != NULL && !PyDict_Check(kwargs)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "keyword list must be a dictionary");
        return NULL;
    }

    if (args == NULL) {
        return _PyObject_VectorcallDictTstate(tstate, callable,
                                              NULL, 0, kwargs);
    }
    else {
        return _PyObject_Call(tstate, callable, args, kwargs);
    }
}


PyObject *
PyObject_CallObject(PyObject *callable, PyObject *args)
{
    PyThreadState *tstate = _PyThreadState_GET();
    assert(!_PyErr_Occurred(tstate));
    if (args == NULL) {
        return _PyObject_CallNoArgsTstate(tstate, callable);
    }
    if (!PyTuple_Check(args)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "argument list must be a tuple");
        return NULL;
    }
    return _PyObject_Call(tstate, callable, args, NULL);
}


/* Call callable(obj, *args, **kwargs). */
PyObject *
_PyObject_Call_Prepend(PyThreadState *tstate, PyObject *callable,
                       PyObject *obj, PyObject *args, PyObject *kwargs)
{
    assert(PyTuple_Check(args));
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack = small_stack;
    PyObject *result = NULL;
    Py_ssize_t count = PyTuple_GET_SIZE(args);
    PyObject *const *views = _PyBorrow_TupleItems(args, count);
    if (views == NULL) {
        goto done;
    }
    if ((size_t)count > SIZE_MAX / sizeof(*stack) - 1) {
        PyErr_NoMemory();
        goto done;
    }
    if (count + 1 > (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = PyMem_Malloc(((size_t)count + 1) * sizeof(*stack));
        if (stack == NULL) {
            if (!PyErr_Occurred()) {
                PyErr_NoMemory();
            }
            goto done;
        }
    }
    stack[0] = obj;
    memcpy(stack + 1, views, (size_t)count * sizeof(*stack));
    result = _PyObject_VectorcallDictTstate(tstate, callable, stack, count + 1, kwargs);
done:
    if (stack != small_stack) {
        call_free_buffer(stack);
    }
    _PyBorrowScope_Exit(tstate, mark);
    return call_finish_result(result);
}


/* --- Call with a format string ---------------------------------- */

static PyObject *
_PyObject_CallFunctionVa(PyThreadState *tstate, PyObject *callable,
                         const char *format, va_list va)
{
    if (callable == NULL) {
        return null_error(tstate);
    }
    if (format == NULL || *format == '\0') {
        return _PyObject_CallNoArgsTstate(tstate, callable);
    }
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    Py_ssize_t nargs = 0;
    PyObject **stack = _Py_VaBuildStack(small_stack, Py_ARRAY_LENGTH(small_stack),
                                       format, va, &nargs);
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = NULL;
    if (stack == NULL || PyErr_Occurred()) {
        goto done;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    if (nargs == 1 && PyTuple_Check(stack[0])) {
        Py_ssize_t count = PyTuple_GET_SIZE(stack[0]);
        PyObject *const *args = _PyBorrow_TupleItems(stack[0], count);
        if (args == NULL) {
            goto done;
        }
        result = _PyObject_VectorcallTstate(tstate, callable, args, count, NULL);
    }
    else {
        result = _PyObject_VectorcallTstate(tstate, callable, stack, nargs, NULL);
    }
done:
    _PyBorrowScope_Exit(tstate, mark);
    if (stack != NULL) {
        for (Py_ssize_t i = 0; i < nargs; i++) {
            Py_CLEAR(stack[i]);
        }
        if (stack != small_stack) {
            call_free_buffer(stack);
        }
    }
    return call_finish_result(result);
}


PyObject *
PyObject_CallFunction(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *result;
    PyThreadState *tstate = _PyThreadState_GET();

    va_start(va, format);
    result = _PyObject_CallFunctionVa(tstate, callable, format, va);
    va_end(va);

    return result;
}


/* PyEval_CallFunction is exact copy of PyObject_CallFunction.
   Function removed in Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject*)
PyEval_CallFunction(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *result;
    PyThreadState *tstate = _PyThreadState_GET();

    va_start(va, format);
    result = _PyObject_CallFunctionVa(tstate, callable, format, va);
    va_end(va);

    return result;
}


/* _PyObject_CallFunction_SizeT is exact copy of PyObject_CallFunction.
 * This function must be kept because it is part of the stable ABI.
 */
PyAPI_FUNC(PyObject *)  /* abi_only */
_PyObject_CallFunction_SizeT(PyObject *callable, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();

    va_list va;
    va_start(va, format);
    PyObject *result = _PyObject_CallFunctionVa(tstate, callable, format, va);
    va_end(va);

    return result;
}


static PyObject *
callmethod(PyThreadState *tstate, PyObject *obj, PyObject *name,
           const char *format, va_list va)
{
    PyObject *boxes[4];
    _PyValueTemps temps = {boxes, 0, 4};
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **built = NULL;
    Py_ssize_t nargs = 0;
    PyObject *result = NULL;
    _PyCValue self, method;
    _PyThreadState_PushCValue(tstate, &self);
    _PyThreadState_PushCValue(tstate, &method);
    self.ref = PyValue_FromPyObjectBorrow(obj);
    if (_PyObject_GetMethodValue(tstate, &self.ref, name, &method.ref, obj) < 0 ||
            PyErr_Occurred()) {
        goto done;
    }
    PyObject *callable = _PyValueTemps_ObjectAt(&temps, method.ref, BOX_SITE_CALL);
    if (callable == NULL) {
        goto done;
    }
    PyObject *receiver = PyValue_IsNull(self.ref) ? NULL : _PyValueTemps_ObjectAt(&temps, self.ref, BOX_SITE_CALL);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (!PyCallable_Check(callable)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "attribute of type '%.200s' is not callable", Py_TYPE(callable)->tp_name);
        goto done;
    }
    if (format != NULL && *format != '\0') {
        built = _Py_VaBuildStack(small_stack, Py_ARRAY_LENGTH(small_stack), format, va, &nargs);
        if (built == NULL) {
            goto done;
        }
    }
    PyObject *const *args = built;
    Py_ssize_t count = nargs;
    if (nargs == 1 && PyTuple_Check(built[0])) {
        count = PyTuple_GET_SIZE(built[0]);
        args = _PyBorrow_TupleItems(built[0], count);
        if (args == NULL) {
            goto done;
        }
    }
    if (receiver != NULL) {
        result = _PyObject_VectorcallPrepend(tstate, callable, receiver, args, count, NULL);
    }
    else {
        result = _PyObject_VectorcallTstate(tstate, callable, args, count, NULL);
    }
done:
    _PyBorrowScope_Exit(tstate, mark);
    if (built != NULL) {
        for (Py_ssize_t i = 0; i < nargs; i++) {
            Py_CLEAR(built[i]);
        }
        if (built != small_stack) {
            call_free_buffer(built);
        }
    }
    call_pop_value(tstate, &method);
    call_pop_value(tstate, &self);
    _PyValueTemps_Close(&temps);
    return call_finish_result(result);
}

PyObject *
PyObject_CallMethod(PyObject *obj, const char *name, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, obj, name_obj, format, va);
    va_end(va);

    Py_DECREF(name_obj);
    return retval;
}


/* PyEval_CallMethod is exact copy of PyObject_CallMethod.
   Function removed in Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject*)
PyEval_CallMethod(PyObject *obj, const char *name, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, obj, name_obj, format, va);
    va_end(va);

    Py_DECREF(name_obj);
    return retval;
}


PyObject *
_PyObject_CallMethod(PyObject *obj, PyObject *name,
                     const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, obj, name, format, va);
    va_end(va);

    return retval;
}


PyObject *
_PyObject_CallMethodId(PyObject *obj, _Py_Identifier *name,
                       const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PyObject *name_obj = _PyUnicode_FromId(name); /* borrowed */
_Py_COMP_DIAG_POP
    if (name_obj == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, obj, name_obj, format, va);
    va_end(va);

    return retval;
}


// _PyObject_CallMethod_SizeT is exact copy of PyObject_CallMethod.
// This function must be kept because it is part of the stable ABI.
PyAPI_FUNC(PyObject *)  /* abi_only */
_PyObject_CallMethod_SizeT(PyObject *obj, const char *name,
                           const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, obj, name_obj, format, va);
    va_end(va);

    Py_DECREF(name_obj);
    return retval;
}


/* --- Call with "..." arguments ---------------------------------- */

static PyObject *
object_vacall(PyThreadState *tstate, PyObject *base,
              PyObject *callable, va_list vargs)
{
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack;
    Py_ssize_t nargs;
    PyObject *result;
    Py_ssize_t i;
    va_list countva;

    if (callable == NULL) {
        return null_error(tstate);
    }

    /* Count the number of arguments */
    va_copy(countva, vargs);
    nargs = base ? 1 : 0;
    while (1) {
        PyObject *arg = va_arg(countva, PyObject *);
        if (arg == NULL) {
            break;
        }
        nargs++;
    }
    va_end(countva);

    /* Copy arguments */
    if (nargs <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(nargs * sizeof(stack[0]));
        if (stack == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    i = 0;
    if (base) {
        stack[i++] = base;
    }

    for (; i < nargs; ++i) {
        stack[i] = va_arg(vargs, PyObject *);
    }

#ifdef Py_STATS
    if (PyFunction_Check(callable)) {
        EVAL_CALL_STAT_INC(EVAL_CALL_API);
    }
#endif
    /* Call the function */
    result = _PyObject_VectorcallTstate(tstate, callable, stack, nargs, NULL);

    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}

PyObject *
_PyObject_VectorcallPrepend(PyThreadState *tstate, PyObject *callable,
                            PyObject *arg, PyObject *const *args,
                            size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs == 0 || args[nargs-1]);

    PyObject *result;
    if (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET) {
        /* PY_VECTORCALL_ARGUMENTS_OFFSET is set, so we are allowed to mutate the vector */
        PyObject **newargs = (PyObject**)args - 1;
        nargs += 1;
        PyObject *tmp = newargs[0];
        newargs[0] = arg;
        assert(newargs[nargs-1]);
        result = _PyObject_VectorcallTstate(tstate, callable, newargs,
                                            nargs, kwnames);
        newargs[0] = tmp;
    }
    else {
        Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
        Py_ssize_t totalargs = nargs + nkwargs;
        if (totalargs == 0) {
            return _PyObject_VectorcallTstate(tstate, callable, &arg, 1, NULL);
        }

        PyObject *newargs_stack[_PY_FASTCALL_SMALL_STACK];
        PyObject **newargs;
        if (totalargs <= (Py_ssize_t)Py_ARRAY_LENGTH(newargs_stack) - 1) {
            newargs = newargs_stack;
        }
        else {
            newargs = PyMem_Malloc((totalargs+1) * sizeof(PyObject *));
            if (newargs == NULL) {
                _PyErr_NoMemory(tstate);
                return NULL;
            }
        }
        /* use borrowed references */
        newargs[0] = arg;
        /* bpo-37138: since totalargs > 0, it's impossible that args is NULL.
         * We need this, since calling memcpy() with a NULL pointer is
         * undefined behaviour. */
        assert(args != NULL);
        memcpy(newargs + 1, args, totalargs * sizeof(PyObject *));
        result = _PyObject_VectorcallTstate(tstate, callable,
                                            newargs, nargs+1, kwnames);
        if (newargs != newargs_stack) {
            PyMem_Free(newargs);
        }
    }
    return result;
}

PyObject *
PyObject_VectorcallMethod(PyObject *name, PyObject *const *args,
                           size_t nargsf, PyObject *kwnames)
{
    assert(name != NULL && args != NULL && PyVectorcall_NARGS(nargsf) >= 1);
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *boxes[4];
    _PyValueTemps temps = {boxes, 0, 4};
    PyObject *result = NULL;
    _PyCValue self, method;
    _PyThreadState_PushCValue(tstate, &self);
    _PyThreadState_PushCValue(tstate, &method);
    self.ref = PyValue_FromPyObjectBorrow(args[0]);
    if (_PyObject_GetMethodValue(tstate, &self.ref, name, &method.ref, args[0]) < 0 ||
            PyErr_Occurred()) {
        goto done;
    }
    PyObject *callable = _PyValueTemps_ObjectAt(&temps, method.ref, BOX_SITE_CALL);
    if (callable == NULL) {
        goto done;
    }
    PyObject *receiver = PyValue_IsNull(self.ref) ? NULL : _PyValueTemps_ObjectAt(&temps, self.ref, BOX_SITE_CALL);
    if (PyErr_Occurred()) {
        goto done;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_METHOD, callable);
    if (receiver == NULL) {
        result = _PyObject_VectorcallTstate(tstate, callable, args + 1, nargsf - 1, kwnames);
    }
    else if (receiver == args[0]) {
        result = _PyObject_VectorcallTstate(tstate, callable, args,
                                           nargsf & ~PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    }
    else {
        result = _PyObject_VectorcallPrepend(tstate, callable, receiver, args + 1,
                                            nargsf - 1, kwnames);
    }
done:
    call_pop_value(tstate, &method);
    call_pop_value(tstate, &self);
    _PyValueTemps_Close(&temps);
    return call_finish_result(result);
}


PyObject *
PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }
    PyObject *boxes[4];
    _PyValueTemps temps = {boxes, 0, 4};
    PyObject *result = NULL;
    _PyCValue self, method;
    _PyThreadState_PushCValue(tstate, &self);
    _PyThreadState_PushCValue(tstate, &method);
    self.ref = PyValue_FromPyObjectBorrow(obj);
    if (_PyObject_GetMethodValue(tstate, &self.ref, name, &method.ref, obj) < 0 ||
            PyErr_Occurred()) {
        goto done;
    }
    PyObject *callable = _PyValueTemps_ObjectAt(&temps, method.ref, BOX_SITE_CALL);
    if (callable == NULL) {
        goto done;
    }
    PyObject *receiver = PyValue_IsNull(self.ref) ? NULL : _PyValueTemps_ObjectAt(&temps, self.ref, BOX_SITE_CALL);
    if (PyErr_Occurred()) {
        goto done;
    }
    va_list vargs;
    va_start(vargs, name);
    result = object_vacall(tstate, receiver, callable, vargs);
    va_end(vargs);
done:
    call_pop_value(tstate, &method);
    call_pop_value(tstate, &self);
    _PyValueTemps_Close(&temps);
    return call_finish_result(result);
}


PyObject *
PyObject_CallFunctionObjArgs(PyObject *callable, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    va_list vargs;
    PyObject *result;

    va_start(vargs, callable);
    result = object_vacall(tstate, NULL, callable, vargs);
    va_end(vargs);

    return result;
}


/* --- PyStack functions ------------------------------------------ */

PyObject *
_PyStack_AsDict(PyObject *const *values, PyObject *kwnames)
{
    assert(kwnames != NULL);
    Py_ssize_t nkwargs = PyTuple_GET_SIZE(kwnames);
    PyObject *dict = _PyDict_NewPresized(nkwargs);
    if (dict == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        /* Vectorcall kwnames hold only str, which is never inline. */
        PyObject *key = _PyTuple_GET_OBJECT(kwnames, i);
        if (PyDict_SetItem(dict, key, values[i]) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
    }
    return dict;
}


/* Convert (args, nargs, kwargs: dict) into a (stack, nargs, kwnames: tuple).

   Allocate a new argument vector and keyword names tuple. Return the argument
   vector; return NULL with exception set on error. Return the keyword names
   tuple in *p_kwnames.

   This also checks that all keyword names are strings. If not, a TypeError is
   raised.

   The newly allocated argument vector supports PY_VECTORCALL_ARGUMENTS_OFFSET.

   The positional arguments are borrowed references from the input array
   (which must be kept alive by the caller). The keyword argument values
   are new references.

   When done, you must call _PyStack_UnpackDict_Free(stack, nargs, kwnames) */
PyObject *const *
_PyStack_UnpackDict(PyThreadState *tstate,
                    PyObject *const *args, Py_ssize_t nargs,
                    PyObject *kwargs, PyObject **p_kwnames)
{
    assert(nargs >= 0 && kwargs != NULL && PyDict_Check(kwargs));
    *p_kwnames = NULL;
    PyObject *items = NULL;
    PyObject *kwnames = NULL;
    PyObject **stack = NULL;
    Py_ssize_t nkwargs = 0;
    if (PyErr_Occurred()) {
        goto error;
    }
    items = PyDict_Items(kwargs);
    if (items == NULL) {
        goto error;
    }
    nkwargs = PyList_GET_SIZE(items);
    /* Snapshot shape is checked before allocating or boxing keyword values. */
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *pair = PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(items, i));
        PyValue key = PyTuple_GET_VALUE(pair, 0);
        if (!PyValue_IsObject(key) || !PyUnicode_Check(PyValue_AsKnownObjectBorrow(key))) {
            _PyErr_SetString(tstate, PyExc_TypeError, "keywords must be strings");
            goto error;
        }
    }
    Py_ssize_t maxnargs = PY_SSIZE_T_MAX / sizeof(*stack) - 1;
    if (nargs > maxnargs - nkwargs) {
        _PyErr_NoMemory(tstate);
        goto error;
    }
    stack = PyMem_Calloc(1 + nargs + nkwargs, sizeof(*stack));
    if (stack != NULL) {
        stack++;
    }
    if (stack == NULL || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            _PyErr_NoMemory(tstate);
        }
        goto error;
    }
    for (Py_ssize_t i = 0; i < nargs; i++) {
        stack[i] = args[i];
    }
    kwnames = PyTuple_New(nkwargs);
    if (kwnames == NULL || PyErr_Occurred()) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *pair = PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(items, i));
        PyObject *key = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(pair, 0));
        PyTuple_SET_ITEM(kwnames, i, Py_NewRef(key));
        stack[nargs + i] = _PyValue_AsObjectAt(PyTuple_GET_VALUE(pair, 1), BOX_SITE_CALL_ADAPTER);
        if (stack[nargs + i] == NULL) {
            goto error;
        }
    }
    Py_CLEAR(items);
    if (PyErr_Occurred()) {
        goto error;
    }
    *p_kwnames = kwnames;
    return stack;
error:
    Py_CLEAR(items);
    if (stack != NULL) {
        for (Py_ssize_t i = 0; i < nkwargs; i++) {
            Py_CLEAR(stack[nargs + i]);
        }
        call_free_buffer(stack - 1);
    }
    Py_CLEAR(kwnames);
    return NULL;
}

void
_PyStack_UnpackDict_Free(PyObject *const *stack, Py_ssize_t nargs,
                         PyObject *kwnames)
{
    Py_ssize_t count = PyTuple_GET_SIZE(kwnames);
    PyObject **owned = (PyObject **)stack;
    for (Py_ssize_t i = 0; i < count; i++) {
        Py_CLEAR(owned[nargs + i]);
    }
    _PyStack_UnpackDict_FreeNoDecRef(stack, kwnames);
}

void
_PyStack_UnpackDict_FreeNoDecRef(PyObject *const *stack, PyObject *kwnames)
{
    call_free_buffer((PyObject **)stack - 1);
    Py_CLEAR(kwnames);
}

// Export for the stable ABI
#undef PyVectorcall_NARGS
Py_ssize_t
PyVectorcall_NARGS(size_t n)
{
    return _PyVectorcall_NARGS(n);
}
