#include "Python.h"
#include "pycore_typeobject.h"    // _PyType_IsGenericCallPair()
#include "pycore_borrow.h"        // _PyBorrowScope_Enter()
#include "pycore_call.h"          // _PyObject_CallNoArgsTstate()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_dict.h"          // _PyDict_NewPresized()
#include "pycore_function.h"      // _PyFunction_Vectorcall() definition
#include "pycore_list.h"          // PyList_GET_VALUE()
#include "pycore_interpframe.h"   // _PyEvalFramePushAndInit()
#include "pycore_modsupport.h"    // _Py_VaBuildStack()
#include "pycore_object.h"        // _PyCFunctionWithKeywords_TrampolineCall()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_GET_OBJECT()
#include "pycore_value.h"         // Owning object bridges and cleanup
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_value_call.h"    // _PyValue_VectorcallTstate()


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
    return _Py_CheckFunctionResult(tstate, callable, result, NULL);
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
    ternaryfunc call = Py_TYPE(callable)->tp_call;
    if (call == NULL) {
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
    return _Py_CheckFunctionResult(tstate, callable, result, NULL);
}


vectorcallfunc
PyVectorcall_Function(PyObject *callable)
{
    return _PyVectorcall_FunctionInline(callable);
}


Py_VALUE_ENTRY PyObject *
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

    /* The tuple's words are the window (D11): no box on entry. */
    Py_ssize_t nargs = PyTuple_GET_SIZE(tuple);
    PyValue res;
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        res = _PyValue_InstanceVectorcall(tstate, callable,
                                          _PyTuple_ITEMS(tuple), nargs, NULL);
    }
    else {
        PyObject *kwnames;
        PyValue *window = _PyValue_StackUnpackDict(
            tstate, _PyTuple_ITEMS(tuple), nargs, kwargs, &kwnames);
        if (window == NULL) {
            return NULL;
        }
        res = _PyValue_InstanceVectorcall(
            tstate, callable, window, nargs | PY_VECTORCALL_ARGUMENTS_OFFSET,
            kwnames);
        _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    }
    return _PyValue_THUNK_EXIT(res, BOX_SITE_CALL);
}


PyObject *
PyObject_Vectorcall(PyObject *callable, PyObject *const *args,
                     size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, callable,
                                      args, nargsf, kwnames);
}


Py_VALUE_ENTRY PyObject *
_PyObject_Call(PyThreadState *tstate, PyObject *callable,
               PyObject *args, PyObject *kwargs)
{
    /* PyObject_Call() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
    assert(PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    return _PyValue_THUNK_EXIT(
        _PyValue_Call(tstate, PyValue_FromObjectBorrow(callable), args, kwargs),
        BOX_SITE_CALL);
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

PyValue
_PyFunction_ValueVectorcall(PyThreadState *tstate, PyObject *func,
                            const PyValue *args, size_t nargsf,
                            PyObject *kwnames)
{
    assert(PyFunction_Check(func));
    return _PyEval_VectorValue(tstate, (PyFunctionObject *)func, args,
                               nargsf, kwnames);
}

/* The object vectorcall of a core callable with a value vectorcall (D8):
   the object vector becomes a call-scratch window with a reserved slot
   before it (an exact int or float inline, every other object its borrowed
   word: no box), the instance's value vectorcall runs with the offset
   flag, and an inline result is boxed on the way out (kind THUNK). Core
   code never calls a thunk; the object call API and extensions do. */
/* The thunk boxes for the caller of the exported wrapper, so the wrapper is
   the entry and the body is inlined into it. */
#ifdef Py_STATS
#  define _PyValue_THUNK_INLINE static inline Py_ALWAYS_INLINE
#else
#  define _PyValue_THUNK_INLINE static inline
#endif

#ifdef Py_DEBUG
void
_PyValue_CheckInstanceInvariant(PyObject *callable)
{
    if (!PyFunction_Check(callable)) {
        return;
    }
    PyFunctionObject *func = (PyFunctionObject *)callable;
    assert((func->vectorcall == _PyFunction_Vectorcall)
           == (func->func_value_vectorcall != NULL));
}
#endif

_PyValue_THUNK_INLINE PyObject *
value_vectorcall_thunk(PyObject *callable, PyObject *const *args,
                       size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyTypeObject *tp = Py_TYPE(callable);
    _PyValue_ASSERT_INSTANCE(callable);
    _Py_vectorcallvaluefunc f = NULL;
    if (tp->tp_value_vectorcall_offset != 0) {
        memcpy(&f, (char *)callable + tp->tp_value_vectorcall_offset,
               sizeof(f));
    }
    if (f == NULL) {
        /* An extension cached the thunk before PyFunction_SetVectorcall()
           replaced the value form (R1): call the object field. */
        vectorcallfunc of = NULL;
        if (tp->tp_vectorcall_offset > 0) {
            memcpy(&of, (char *)callable + tp->tp_vectorcall_offset,
                   sizeof(of));
        }
        if (of == NULL || of == _PyFunction_Vectorcall
            || of == _PyValue_VectorcallThunk)
        {
            _PyErr_Format(tstate, PyExc_SystemError,
                          "'%.200s' object has no value vectorcall",
                          tp->tp_name);
            return NULL;
        }
        return of(callable, args, nargsf, kwnames);
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    Py_ssize_t total = nargs + (kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames));
    PyValue small[_PY_FASTCALL_SMALL_STACK + 1];
    PyValue *buf = small;
    if (total + 1 > (Py_ssize_t)Py_ARRAY_LENGTH(small)) {
        if ((size_t)total > PY_SSIZE_T_MAX / sizeof(PyValue) - 1) {
            _PyErr_NoMemory(tstate);
            return NULL;
        }
        buf = PyMem_Malloc(((size_t)total + 1) * sizeof(PyValue));
        if (buf == NULL) {
            _PyErr_NoMemory(tstate);
            return NULL;
        }
    }
    buf[0] = PyValue_NULL;
    _PyValue_WindowFromObjects(buf + 1, args, total);
    PyValue res = f(tstate, callable, buf + 1,
                    nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    _PyValue_ASSERT_RESULT(tstate, res);
    if (buf != small) {
        PyMem_Free(buf);
    }
    return _PyValue_THUNK_EXIT(res, BOX_SITE_CALL);
}

#define _Py_VALUE_VECTORCALL_THUNK(NAME) \
    Py_VALUE_ENTRY PyObject * \
    NAME(PyObject *callable, PyObject *const *args, size_t nargsf, \
         PyObject *kwnames) \
    { \
        return value_vectorcall_thunk(callable, args, nargsf, kwnames); \
    }

/* The exported name stays: the interpreter's guard and
   PyFunction_SetVectorcall() compare against it. */
_Py_VALUE_VECTORCALL_THUNK(_PyFunction_Vectorcall)
_Py_VALUE_VECTORCALL_THUNK(_PyValue_VectorcallThunk)

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
Py_VALUE_ENTRY PyObject *
_PyObject_Call_Prepend(PyThreadState *tstate, PyObject *callable,
                       PyObject *obj, PyObject *args, PyObject *kwargs)
{
    assert(PyTuple_Check(args));
    /* A call-scratch window of obj and the tuple's words, with a reserved
       slot before it (D11: no box on entry). */
    Py_ssize_t count = PyTuple_GET_SIZE(args);
    PyValue small[_PY_FASTCALL_SMALL_STACK + 1];
    PyValue *buf = small;
    if ((size_t)count > PY_SSIZE_T_MAX / sizeof(PyValue) - 2) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }
    if (count + 2 > (Py_ssize_t)Py_ARRAY_LENGTH(small)) {
        buf = PyMem_Malloc(((size_t)count + 2) * sizeof(PyValue));
        if (buf == NULL) {
            _PyErr_NoMemory(tstate);
            return NULL;
        }
    }
    buf[0] = PyValue_NULL;
    PyValue *window = buf + 1;
    _PyValue_WindowPut(&window[0], PyValue_FromObjectBorrow(obj));
    _PyValue_WindowCopy(window + 1, _PyTuple_ITEMS(args), count);
    PyValueBorrow c = PyValue_FromObjectBorrow(callable);
    PyValue res;
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        res = _PyValue_VectorcallTstate(
            tstate, c, window, (count + 1) | PY_VECTORCALL_ARGUMENTS_OFFSET,
            NULL);
    }
    else {
        PyObject *kwnames;
        PyValue *kwwindow = _PyValue_StackUnpackDict(tstate, window, count + 1,
                                                     kwargs, &kwnames);
        if (kwwindow == NULL) {
            res = PyValue_NULL;
        }
        else {
            res = _PyValue_VectorcallTstate(
                tstate, c, kwwindow,
                (count + 1) | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
            _PyValue_StackUnpackDict_Free(kwwindow, count + 1, kwnames);
        }
    }
    if (buf != small) {
        PyMem_Free(buf);
    }
    return _PyValue_THUNK_EXIT(res, BOX_SITE_CALL);
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
    if (stack == NULL) {
        goto done;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    if (nargs == 1 && PyTuple_Check(stack[0])) {
        Py_ssize_t count = PyTuple_GET_SIZE(stack[0]);
        PyObject *const *args = _PyBorrow_TupleItemsFor(
            stack[0], count, _PyBox_CALL_ADAPTER(callable));
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
            PyMem_Free(stack);
        }
    }
    return result;
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
    _PyStatusValue found = _PyValue_GetMethod(PyValue_FromObjectBorrow(obj), name);
    if (found.status < 0) {
        goto done;
    }
    method.ref = _PyValue_Publish(&found.value);
    if (found.status == 1) {
        self.ref = PyValue_DUP(PyValue_FromObjectBorrow(obj));
    }
    PyObject *callable = _PyValueTemps_ObjectAt(&temps, method.ref, BOX_SITE_CALL);
    if (callable == NULL) {
        goto done;
    }
    PyObject *receiver = NULL;
    if (!PyValue_IsNull(self.ref)) {
        receiver = _PyValueTemps_ObjectAt(&temps, self.ref, BOX_SITE_CALL);
        if (receiver == NULL) {
            goto done;
        }
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
        args = _PyBorrow_TupleItemsFor(built[0], count,
                                       _PyBox_CALL_ADAPTER(callable));
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
            PyMem_Free(built);
        }
    }
    _PyThreadState_PopCValue(tstate, &method);
    _PyThreadState_PopCValue(tstate, &self);
    _PyValueTemps_Close(&temps);
    return result;
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
    _PyStatusValue found = _PyValue_GetMethod(PyValue_FromObjectBorrow(args[0]), name);
    if (found.status < 0) {
        goto done;
    }
    method.ref = _PyValue_Publish(&found.value);
    if (found.status == 1) {
        self.ref = PyValue_DUP(PyValue_FromObjectBorrow(args[0]));
    }
    PyObject *callable = _PyValueTemps_ObjectAt(&temps, method.ref, BOX_SITE_CALL);
    if (callable == NULL) {
        goto done;
    }
    PyObject *receiver = NULL;
    if (!PyValue_IsNull(self.ref)) {
        receiver = _PyValueTemps_ObjectAt(&temps, self.ref, BOX_SITE_CALL);
        if (receiver == NULL) {
            goto done;
        }
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
    _PyThreadState_PopCValue(tstate, &method);
    _PyThreadState_PopCValue(tstate, &self);
    _PyValueTemps_Close(&temps);
    return result;
}


/* The value form of PyObject_VectorcallMethod(): args is a window of words
   that the caller keeps, args[0] the receiver and the rest the arguments, so
   an inline operand reaches the method without a box. With the offset flag,
   args[-1] may be used for a bound callee. The result is owned. */
PyValue
_PyValue_VectorcallMethod(PyObject *name, const PyValue *args, size_t nargsf,
                          PyObject *kwnames)
{
    assert(name != NULL && args != NULL && PyVectorcall_NARGS(nargsf) >= 1);
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue result = PyValue_NULL;
    _PyCValue method;
    _PyThreadState_PushCValue(tstate, &method);
    _PyStatusValue found = _PyValue_GetMethod(PyValue_Borrow(args[0]), name);
    if (found.status < 0) {
        goto done;
    }
    method.ref = _PyValue_Publish(&found.value);
    if (PyValue_IsObject(method.ref)) {
        EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_METHOD,
                                       PyValue_AsKnownObjectBorrow(method.ref));
    }
    if (found.status == 1) {
        // An unbound method: the receiver is its first argument.
        result = _PyValue_VectorcallTstate(
            tstate, method.ref, args,
            nargsf & ~PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    }
    else {
        result = _PyValue_VectorcallTstate(tstate, method.ref, args + 1,
                                           nargsf - 1, kwnames);
    }
done:
    _PyThreadState_PopCValue(tstate, &method);
    return result;
}

PyValue
(_PyValue_CallMethodNoArgs)(PyValueBorrow self, PyObject *name)
{
    PyValue stack[1];
    _PyValue_WindowPut(&stack[0], self);
    return _PyValue_VectorcallMethod(name, stack, 1, NULL);
}

PyValue
(_PyValue_CallMethodOneArg)(PyValueBorrow self, PyObject *name,
                            PyValueBorrow arg)
{
    PyValue stack[2];
    _PyValue_WindowPut(&stack[0], self);
    _PyValue_WindowPut(&stack[1], arg);
    return _PyValue_VectorcallMethod(name, stack, 2, NULL);
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
    _PyStatusValue found = _PyValue_GetMethod(PyValue_FromObjectBorrow(obj), name);
    if (found.status < 0) {
        goto done;
    }
    method.ref = _PyValue_Publish(&found.value);
    if (found.status == 1) {
        self.ref = PyValue_DUP(PyValue_FromObjectBorrow(obj));
    }
    PyObject *callable = _PyValueTemps_ObjectAt(&temps, method.ref, BOX_SITE_CALL);
    if (callable == NULL) {
        goto done;
    }
    PyObject *receiver = NULL;
    if (!PyValue_IsNull(self.ref)) {
        receiver = _PyValueTemps_ObjectAt(&temps, self.ref, BOX_SITE_CALL);
        if (receiver == NULL) {
            goto done;
        }
    }
    va_list vargs;
    va_start(vargs, name);
    result = object_vacall(tstate, receiver, callable, vargs);
    va_end(vargs);
done:
    _PyThreadState_PopCValue(tstate, &method);
    _PyThreadState_PopCValue(tstate, &self);
    _PyValueTemps_Close(&temps);
    return result;
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
        if (PyDict_SetValue(dict, PyValue_FromObjectBorrow(key), PyValue_FromObjectBorrow(values[i])) < 0) {
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
    items = PyDict_Items(kwargs);
    if (items == NULL) {
        goto error;
    }
    nkwargs = PyList_GET_SIZE(items);
    /* Snapshot shape is checked before allocating or boxing keyword values. */
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *pair = PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(items, i));
        PyValueBorrow key = PyTuple_GET_VALUE(pair, 0);
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
    if (stack == NULL) {
        _PyErr_NoMemory(tstate);
        goto error;
    }
    stack++;
    for (Py_ssize_t i = 0; i < nargs; i++) {
        stack[i] = args[i];
    }
    kwnames = PyTuple_New(nkwargs);
    if (kwnames == NULL) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *pair = PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(items, i));
        PyObject *key = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(pair, 0));
        PyTuple_SET_VALUE(kwnames, i, PyValue_DUP(PyValue_FromObjectBorrow(key)));
        // The callee is not known here: the function names itself.
        stack[nargs + i] = _PyValue_AsObjectFor(
            PyTuple_GET_VALUE(pair, 1),
            _PyBox_AT(BOX_SITE_CALL_ADAPTER, BOX_KIND_CALL_ADAPTER,
                      _Py_FUNC_ADDR(_PyStack_UnpackDict)));
        if (stack[nargs + i] == NULL) {
            goto error;
        }
    }
    Py_CLEAR(items);
    *p_kwnames = kwnames;
    return stack;
error:
    Py_CLEAR(items);
    if (stack != NULL) {
        for (Py_ssize_t i = 0; i < nkwargs; i++) {
            Py_CLEAR(stack[nargs + i]);
        }
        PyMem_Free(stack - 1);
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
    PyMem_Free((PyObject **)stack - 1);
    Py_CLEAR(kwnames);
}

// Export for the stable ABI
#undef PyVectorcall_NARGS
Py_ssize_t
PyVectorcall_NARGS(size_t n)
{
    return _PyVectorcall_NARGS(n);
}


/* --- The value vectorcall (value-api-m3.md, step M3a) ------------ */

PyValue
(_PyValue_NotCallable)(PyThreadState *tstate, PyValueBorrow v)
{
    _PyErr_Format(tstate, PyExc_TypeError, "'%.200s' object is not callable",
                  PyValue_TYPE(v)->tp_name);
    return PyValue_NULL;
}

PyValue
_PyValue_CallAdapter(PyThreadState *tstate, PyObject *callable,
                     vectorcallfunc of, const PyValue *args, size_t nargsf,
                     PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    Py_ssize_t total = nargs + (kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames));
    /* The vector lends each object word and boxes each inline one on the
       borrow stack (kind CALL_ADAPTER); it has its own offset slot. */
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *res = NULL;
    PyObject **vector = _PyBorrow_ValueVectorFor(args, total,
                                                  _PyBox_CALL_ADAPTER(callable));
    if (vector != NULL) {
        res = of(callable, vector, nargs | PY_VECTORCALL_ARGUMENTS_OFFSET,
                 kwnames);
    }
    _PyBorrowScope_Exit(tstate, mark);
    res = _Py_CheckFunctionResult(tstate, callable, res, NULL);
    return _PyValue_FromObjectResult(res);
}

/* tp_call of callable with an argument tuple and a dict (or NULL), in a
   borrow scope, with upstream's release result check. The caller holds
   the recursion guard. */
static PyValue
value_tp_call(PyThreadState *tstate, PyObject *callable, ternaryfunc call,
              PyObject *argstuple, PyObject *kwdict)
{
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *res = _PyCFunctionWithKeywords_TrampolineCall(
        (PyCFunctionWithKeywords)call, callable, argstuple, kwdict);
    _PyBorrowScope_Exit(tstate, mark);
    res = _Py_CheckFunctionResult(tstate, callable, res, NULL);
    return _PyValue_FromObjectResult(res);
}

PyValue
_PyValue_TpCallAdapter(PyThreadState *tstate, PyObject *callable,
                       const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    ternaryfunc call = Py_TYPE(callable)->tp_call;
    if (call == NULL) {
        object_is_not_callable(tstate, callable);
        return PyValue_NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    /* Tuples store words: no box. */
    PyObject *argstuple = _PyTuple_FromValueArray(args, nargs);
    if (argstuple == NULL) {
        return PyValue_NULL;
    }
    PyObject *kwdict = NULL;
    if (kwnames != NULL && PyTuple_GET_SIZE(kwnames) != 0) {
        kwdict = _PyValue_StackAsDict(args + nargs, kwnames);
        if (kwdict == NULL) {
            Py_DECREF(argstuple);
            return PyValue_NULL;
        }
    }
    PyValue res = value_tp_call(tstate, callable, call, argstuple, kwdict);
    Py_XDECREF(kwdict);
    Py_DECREF(argstuple);
    return res;
}

PyValue
(_PyValue_VectorcallPrepend)(PyThreadState *tstate, PyValueBorrow callable,
                             PyValueBorrow self, const PyValue *args,
                             size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyValue res;
    if (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET) {
        /* args[-1] may be a frame slot, which the GC rechecks as a home:
           it receives an owner (D5) and gets its word back after. */
        PyValue *slot = (PyValue *)args - 1;
        PyValue saved = *slot;
        *slot = PyValue_DUP(self);
        res = _PyValue_VectorcallTstate(tstate, callable, slot, nargs + 1,
                                        kwnames);
        PyValue_CLOSE(*slot);
        *slot = saved;
        return res;
    }
    /* A call-scratch copy that no traversal reads, with a reserved slot
       before it so that a bound callee can prepend in place. */
    Py_ssize_t total = nargs + (kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames));
    PyValue small[_PY_FASTCALL_SMALL_STACK + 2];
    PyValue *buf = small;
    if (total + 2 > (Py_ssize_t)Py_ARRAY_LENGTH(small)) {
        if ((size_t)total > PY_SSIZE_T_MAX / sizeof(PyValue) - 2) {
            _PyErr_NoMemory(tstate);
            return PyValue_NULL;
        }
        buf = PyMem_Malloc(((size_t)total + 2) * sizeof(PyValue));
        if (buf == NULL) {
            _PyErr_NoMemory(tstate);
            return PyValue_NULL;
        }
    }
    buf[0] = PyValue_NULL;
    PyValue *window = buf + 1;
    _PyValue_WindowPut(&window[0], self);
    if (total != 0) {
        _PyValue_WindowCopy(window + 1, args, total);
    }
    res = _PyValue_VectorcallTstate(
        tstate, callable, window, (nargs + 1) | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames);
    if (buf != small) {
        PyMem_Free(buf);
    }
    return res;
}

/* Whether the dispatcher would select an instance vectorcall entry. */
static inline int
value_call_has_vectorcall(PyObject *callable)
{
    PyTypeObject *tp = Py_TYPE(callable);
    if (tp->tp_flags & Py_TPFLAGS_HAVE_VECTORCALL) {
        _Py_vectorcallvaluefunc f = NULL;
        if (tp->tp_value_vectorcall_offset != 0) {
            memcpy(&f, (char *)callable + tp->tp_value_vectorcall_offset,
                   sizeof(f));
        }
        vectorcallfunc of;
        memcpy(&of, (char *)callable + tp->tp_vectorcall_offset, sizeof(of));
        if (f != NULL || of != NULL) {
            return 1;
        }
    }
    return 0;
}

static inline int
value_call_is_tp_call(PyObject *callable)
{
    return !value_call_has_vectorcall(callable) &&
           _PyType_VALUE_SLOT(Py_TYPE(callable), call) == NULL;
}

static int
kwargs_have_non_unicode_key(PyObject *kwargs)
{
    Py_ssize_t pos = 0;
    PyValueBorrow key, value;
    while (_PyDict_NextBorrowed(kwargs, &pos, &key, &value)) {
        if (!PyValue_IsObject(key) ||
            !PyUnicode_Check(PyValue_AsKnownObjectBorrow(key))) {
            return 1;
        }
    }
    return 0;
}

PyValue
(_PyValue_Call)(PyThreadState *tstate, PyValueBorrow callable,
                PyObject *args, PyObject *kwargs)
{
    assert(PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    if (!PyValue_IsObject(callable)) {
        return _PyValue_NotCallable(tstate, callable);
    }
    if (PyValue_IsObject(callable)) {
        PyObject *c = PyValue_AsKnownObjectBorrow(callable);
        if (value_call_is_tp_call(c)) {
            /* Nothing to convert: tp_call gets the original tuple and
               dict, under upstream's guard. */
            ternaryfunc call = Py_TYPE(c)->tp_call;
            if (call == NULL) {
                object_is_not_callable(tstate, c);
                return PyValue_NULL;
            }
            if (_Py_EnterRecursiveCallTstate(tstate,
                                             " while calling a Python object")) {
                return PyValue_NULL;
            }
            PyValue res = value_tp_call(tstate, c, call, args, kwargs);
            _Py_LeaveRecursiveCallTstate(tstate);
            return res;
        }
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0 &&
            !value_call_has_vectorcall(c) &&
            _PyType_IsGenericCallPair(Py_TYPE(c)) &&
            kwargs_have_non_unicode_key(kwargs)) {
            if (_Py_EnterRecursiveCallTstate(tstate,
                                             " while calling a Python object")) {
                return PyValue_NULL;
            }
            PyValue res = value_tp_call(tstate, c, Py_TYPE(c)->tp_call,
                                        args, kwargs);
            _Py_LeaveRecursiveCallTstate(tstate);
            return res;
        }
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            PyValue result = PyValue_NULL;
            if (_PyType_CallWithOriginalDict(tstate, c, args, kwargs,
                                             &result)) {
                return result;
            }
        }
    }
    /* The tuple's words are the window: no box, no copy. */
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_VectorcallTstate(tstate, callable, _PyTuple_ITEMS(args),
                                         nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(tstate, _PyTuple_ITEMS(args),
                                               nargs, kwargs, &kwnames);
    if (window == NULL) {
        return PyValue_NULL;
    }
    PyValue res = _PyValue_VectorcallTstate(
        tstate, callable, window, nargs | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return res;
}

PyValue
(_PyValue_CallNoArgs)(PyThreadState *tstate, PyValueBorrow callable)
{
    PyValue buf[1] = {PyValue_NULL};
    return _PyValue_VectorcallTstate(tstate, callable, buf + 1,
                                     PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
}

PyValue
(_PyValue_CallOneArg)(PyThreadState *tstate, PyValueBorrow callable,
                      PyValueBorrow arg)
{
    PyValue buf[2] = {PyValue_NULL, PyValue_NULL};
    _PyValue_WindowPut(&buf[1], arg);
    return _PyValue_VectorcallTstate(tstate, callable, buf + 1,
                                     1 | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
}

PyValue *
_PyValue_StackUnpackDict(PyThreadState *tstate, const PyValue *args,
                         Py_ssize_t nargs, PyObject *kwargs,
                         PyObject **p_kwnames)
{
    assert(nargs >= 0 && kwargs != NULL && PyDict_Check(kwargs));
    *p_kwnames = NULL;
    Py_ssize_t nkwargs = PyDict_GET_SIZE(kwargs);
    if ((size_t)nargs > PY_SSIZE_T_MAX / sizeof(PyValue) - 1 - (size_t)nkwargs) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }
    PyValue *buf = PyMem_Malloc((1 + (size_t)nargs + (size_t)nkwargs)
                                * sizeof(PyValue));
    if (buf == NULL) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }
    buf[0] = PyValue_NULL;
    PyValue *window = buf + 1;
    if (nargs != 0) {
        _PyValue_WindowCopy(window, args, nargs);
    }
    /* The dict's values are owners: the callee may mutate a dict that
       other code shares. */
    Py_ssize_t n = _PyValue_WindowFromDictValues(window + nargs, kwargs,
                                                 p_kwnames);
    if (n < 0) {
        PyMem_Free(buf);
        return NULL;
    }
    assert(n == nkwargs);
    return window;
}

void
_PyValue_StackUnpackDict_Free(PyValue *window, Py_ssize_t nargs,
                              PyObject *kwnames)
{
    Py_ssize_t nkwargs = PyTuple_GET_SIZE(kwnames);
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyValue_CLOSE(window[nargs + i]);
    }
    PyMem_Free(window - 1);
    Py_DECREF(kwnames);
}

PyObject *
_PyValue_StackAsDict(const PyValue *values, PyObject *kwnames)
{
    assert(kwnames != NULL);
    Py_ssize_t nkwargs = PyTuple_GET_SIZE(kwnames);
    PyObject *dict = _PyDict_NewPresized(nkwargs);
    if (dict == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        /* Vectorcall kwnames hold only str, which is never inline. */
        if (_PyDict_SetValue_Take2((PyDictObject *)dict,
                                   PyValue_DUP(PyTuple_GET_VALUE(kwnames, i)),
                                   PyValue_DUP(values[i])) < 0)
        {
            Py_DECREF(dict);
            return NULL;
        }
    }
    return dict;
}

PyValue
_PyValue_InstanceVectorcall(PyThreadState *tstate, PyObject *callable,
                            const PyValue *args, size_t nargsf,
                            PyObject *kwnames)
{
    PyTypeObject *tp = Py_TYPE(callable);
    if (tp->tp_value_vectorcall_offset != 0) {
        _Py_vectorcallvaluefunc f;
        memcpy(&f, (char *)callable + tp->tp_value_vectorcall_offset,
               sizeof(f));
        if (f != NULL) {
            PyValue res = f(tstate, callable, args, nargsf, kwnames);
            _PyValue_ASSERT_RESULT(tstate, res);
            return res;
        }
    }
    vectorcallfunc of = NULL;
    if (tp->tp_vectorcall_offset > 0) {
        memcpy(&of, (char *)callable + tp->tp_vectorcall_offset, sizeof(of));
    }
    if (of == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      tp->tp_name);
        return PyValue_NULL;
    }
    return _PyValue_CallAdapter(tstate, callable, of, args, nargsf, kwnames);
}

Py_VALUE_ENTRY PyObject *
_PyValue_TpCallThunk(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_vectorcallvaluefunc f = _PyType_VALUE_SLOT(Py_TYPE(callable), call);
    assert(f != NULL);
    assert(PyTuple_Check(args));
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyValue res;
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        res = f(tstate, callable, _PyTuple_ITEMS(args), nargs, NULL);
    }
    else {
        PyObject *kwnames;
        PyValue *window = _PyValue_StackUnpackDict(
            tstate, _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
        if (window == NULL) {
            return NULL;
        }
        res = f(tstate, callable, window,
                nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
        _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    }
    _PyValue_ASSERT_RESULT(tstate, res);
    return _PyValue_THUNK_EXIT(res, BOX_SITE_CALL);
}

#ifdef Py_STATS
/* The owner of a call adapter's boxes (pyjit/design/proposals/value-api-g.md,
   section 1.2): the C entry of the callee, which receives the objects. A
   Python function's entry is _PyFunction_Vectorcall(), in the core. */
const void *
_PyCall_BoxOwner(PyObject *callable)
{
    PyTypeObject *tp = Py_TYPE(callable);
    if (PyCFunction_Check(callable)) {
        return _Py_FUNC_ADDR(((PyCFunctionObject *)callable)->m_ml->ml_meth);
    }
    if (tp == &PyMethodDescr_Type || tp == &PyClassMethodDescr_Type) {
        PyMethodDef *def = ((PyMethodDescrObject *)callable)->d_method;
        return _Py_FUNC_ADDR(def->ml_meth);
    }
    if (tp == &PyWrapperDescr_Type) {
        return ((PyWrapperDescrObject *)callable)->d_wrapped;
    }
    if (PyMethod_Check(callable)) {
        return _PyCall_BoxOwner(PyMethod_GET_FUNCTION(callable));
    }
    if (PyType_Check(callable)) {
        PyTypeObject *type = (PyTypeObject *)callable;
        if (type->tp_vectorcall != NULL) {
            return _Py_FUNC_ADDR(type->tp_vectorcall);
        }
        if (type->tp_init != NULL
            && type->tp_init != PyBaseObject_Type.tp_init)
        {
            return _Py_FUNC_ADDR(type->tp_init);
        }
        return _Py_FUNC_ADDR(type->tp_new);
    }
    vectorcallfunc func = PyVectorcall_Function(callable);
    if (func != NULL) {
        return _Py_FUNC_ADDR(func);
    }
    return _Py_FUNC_ADDR(tp->tp_call);
}
#endif
