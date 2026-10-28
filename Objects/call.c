#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgsTstate()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_dict.h"          // _PyDict_FromItems()
#include "pycore_function.h"      // _PyFunction_Vectorcall() definition
#include "pycore_gc_walk.h"       // Py_GC_PRECISE
#include "pycore_modsupport.h"    // _Py_VaBuildStack()
#include "pycore_object.h"        // _PyCFunctionWithKeywords_TrampolineCall()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()


static PyObject *
null_error(PyThreadState *tstate)
{
    if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                         "null argument to internal routine");
    }
    return NULL;
}


/* Does a call's result agree with the error indicator?  It reads only,
   so a precise caller holds nothing across it (10a-2.md, rule 1). */
static inline int
result_agrees(PyThreadState *tstate, int failed)
{
    return failed == (_PyErr_Occurred(tstate) != NULL);
}

/* Report a call's result that disagrees with the error indicator: a
   SystemError replaces the result, and the return is -1.  Out of line and
   opaque: it is the error path, and its frame is scanned. */
static Py_NO_INLINE int
check_function_result(PyThreadState *tstate, PyObject *callable,
                      int failed, const char *where)
{
    assert((callable != NULL) ^ (where != NULL));

    if (failed) {
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
            /* Ensure that the bug is caught in debug mode.  The fatal error
               logs the SystemError exception raised above, under the name
               of the public check, which both forms share. */
            _Py_FatalErrorFunc("_Py_CheckFunctionResult",
                               "a function returned NULL without setting an exception");
#endif
            return -1;
        }
    }
    else {
        if (_PyErr_Occurred(tstate)) {
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
            /* Ensure that the bug is caught in debug mode.  The fatal error
               logs the SystemError exception raised above, under the name
               of the public check, which both forms share. */
            _Py_FatalErrorFunc("_Py_CheckFunctionResult",
                               "a function returned a result with an exception set");
#endif
            return -1;
        }
    }
    return 0;
}

Py_GC_PRECISE PyObject*
_Py_CheckFunctionResult(PyThreadState *tstate, PyObject *callable,
                        PyObject *result, const char *where)
{
    if (!result_agrees(tstate, result == NULL)) {
        (void)check_function_result(tstate, callable, result == NULL, where);
        return NULL;
    }
    return result;
}

Py_GC_PRECISE PyValue
_Py_CheckFunctionResultValue(PyThreadState *tstate, PyObject *callable,
                             PyValue result, const char *where)
{
    if (!result_agrees(tstate, PyValue_IsNull(result))) {
        (void)check_function_result(tstate, callable, PyValue_IsNull(result),
                                    where);
        return PyValue_NULL;
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
Py_GC_PRECISE PyObject *
PyObject_CallNoArgs(PyObject *func)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_CallNoArgsTstate(tstate, func);
}


/* The canonical values of args[0:n], in small if n + 1 values fit and
   otherwise in an array allocated with PyMem_Malloc(), with a free slot
   before them for PY_VECTORCALL_ARGUMENTS_OFFSET.  NULL, with no
   exception set, if the heap array cannot be allocated: it does not
   collect.  The caller holds args, which keep the objects among the values
   alive (10a-2.md, rule 3).  Free with free_values(). */
Py_GC_PRECISE static PyValue *
values_of(PyObject *const *args, Py_ssize_t n,
          PyValue *small, Py_ssize_t small_len)
{
    PyValue *buf = small;
    if (n + 1 > small_len) {
        buf = PyMem_Malloc((n + 1) * sizeof(PyValue));
        if (buf == NULL) {
            return NULL;
        }
    }
    buf[0] = PyValue_NULL;
    for (Py_ssize_t i = 0; i < n; i++) {
        buf[i + 1] = PyValue_FromObject(args[i]);
    }
    return buf + 1;
}

Py_GC_PRECISE static void
free_values(PyValue *values, PyValue *small)
{
    if (values - 1 != small) {
        PyMem_Free(values - 1);
    }
}

Py_GC_PRECISE PyObject *
_PyObject_VectorcallTstate(PyThreadState *tstate, PyObject *callable,
                           PyObject *const *args, size_t nargsf,
                           PyObject *kwnames)
{
    assert(kwnames == NULL || PyTuple_Check(kwnames));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(args != NULL || nargs == 0);
    Py_ssize_t total = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0);
    PyValue small[_PY_FASTCALL_SMALL_STACK + 1];
    PyValue *values = values_of(args, total, small, Py_ARRAY_LENGTH(small));
    if (values == NULL) {
        return PyErr_NoMemory();
    }
    PyValue res = _PyObject_VectorcallValueTstate(
        tstate, PyValue_FromObject(callable), values,
        nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    free_values(values, small);
    return PyValue_Object(res);
}


PyValue
_PyObject_VectorcallDictValueTstate(PyThreadState *tstate, PyValue callable,
                                    const PyValue *args, size_t nargsf,
                                    PyObject *kwargs)
{
    /* PyObject_VectorcallDict() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(kwargs == NULL || PyDict_Check(kwargs));

    vectorcallfunc func = NULL;
    if (!PyValue_IsInline(callable)) {
        func = PyVectorcall_Function(PyValue_ObjectBorrow(callable));
    }
    if (func == NULL) {
        /* Use tp_call instead */
        return _PyObject_MakeTpCall(tstate, callable, args, nargs, kwargs);
    }

    PyValue res;
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        res = func(callable, args, nargsf, NULL);
    }
    else {
        PyObject *kwnames;
        const PyValue *newargs = _PyStack_UnpackDict(tstate, args, nargs,
                                                     kwargs, &kwnames);
        if (newargs == NULL) {
            return PyValue_NULL;
        }
        res = func(callable, newargs,
                   nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
        _PyStack_UnpackDict_Free(newargs, nargs, kwnames);
    }
    return _Py_CheckFunctionResultValue(
        tstate, PyValue_ObjectBorrow(callable), res, NULL);
}

PyObject *
_PyObject_VectorcallDictTstate(PyThreadState *tstate, PyObject *callable,
                               PyObject *const *args, size_t nargsf,
                               PyObject *kwargs)
{
    assert(callable != NULL);
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyValue small[_PY_FASTCALL_SMALL_STACK + 1];
    PyValue *values = values_of(args, nargs, small, Py_ARRAY_LENGTH(small));
    if (values == NULL) {
        return PyErr_NoMemory();
    }
    PyValue res = _PyObject_VectorcallDictValueTstate(
        tstate, PyValue_FromObject(callable), values,
        nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwargs);
    free_values(values, small);
    return PyValue_Object(res);
}


PyObject *
PyObject_VectorcallDict(PyObject *callable, PyObject *const *args,
                       size_t nargsf, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallDictTstate(tstate, callable, args, nargsf, kwargs);
}

/* Out of line and opaque: it is the error path, and its frame is scanned.
   PyObject_GetOptionalAttr() returns through an object pointer, which a
   precise frame could not hold (10a-2.md, rule 6). */
static Py_NO_INLINE void
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


Py_GC_PRECISE PyValue
_PyObject_MakeTpCall(PyThreadState *tstate, PyValue callable,
                     const PyValue *args, Py_ssize_t nargs,
                     PyObject *keywords)
{
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(keywords == NULL || PyTuple_Check(keywords) || PyDict_Check(keywords));

    /* This frame's span anchors keywords, the callable's object, the
       argument tuple and the keyword dict, each from before the first
       collecting call that it outlives (10a-2.md, rule 2). */
    PyValue slots[4];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, keywords);
    PyValue res = PyValue_NULL;
    PyObject *argstuple;
    PyObject *kwdict;
    PyObject *result = NULL;

    // A box of an inline callable, which is never callable, for the error.
    PyObject *callable_o = PyValue_Object(callable);
    if (callable_o == NULL) {
        goto done;
    }
    _PyValueSpan_AppendObject(&span, callable_o);
    /* Slow path: build a temporary tuple for positional arguments and a
     * temporary dictionary for keyword arguments (if any) */
    ternaryfunc call = Py_TYPE(callable_o)->tp_call;
    if (call == NULL) {
        object_is_not_callable(tstate, callable_o);
        goto done;
    }

    argstuple = _PyTuple_FromValues(args, nargs);
    if (argstuple == NULL) {
        goto done;
    }
    _PyValueSpan_AppendObject(&span, argstuple);

    if (keywords == NULL || PyDict_Check(keywords)) {
        kwdict = keywords;
    }
    else if (PyTuple_GET_SIZE(keywords)) {
        assert(args != NULL);
        kwdict = _PyStack_AsDict(args + nargs, keywords);
        if (kwdict == NULL) {
            goto done;
        }
        _PyValueSpan_AppendObject(&span, kwdict);
    }
    else {
        kwdict = NULL;
    }

    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object") == 0)
    {
        result = _PyCFunctionWithKeywords_TrampolineCall(
            (PyCFunctionWithKeywords)call, callable_o, argstuple, kwdict);
        _Py_LeaveRecursiveCallTstate(tstate);
    }

    result = _Py_CheckFunctionResult(tstate, callable_o, result, NULL);
    if (result != NULL) {
        res = PyValue_FromObject(result);
    }
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}


vectorcallfunc
PyVectorcall_Function(PyObject *callable)
{
    return _PyVectorcall_FunctionInline(callable);
}


/* The keyword form of _PyVectorcall_Call().  Opaque until
   _PyStack_UnpackDict() returns its names in a span rather than through
   an object pointer (10a-2.md, "Waits for 1b"). */
static Py_NO_INLINE PyValue
vectorcall_call_dict(PyThreadState *tstate, vectorcallfunc func,
                     PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(tuple);
    const PyValue *items = _PyTuple_ITEMS(tuple);
    PyValue callable_v = PyValue_FromObject(callable);
    PyObject *kwnames;
    const PyValue *args = _PyStack_UnpackDict(tstate, items, nargs,
                                              kwargs, &kwnames);
    if (args == NULL) {
        return PyValue_NULL;
    }
    PyValue result = func(callable_v, args,
                          nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    _PyStack_UnpackDict_Free(args, nargs, kwnames);

    return _Py_CheckFunctionResultValue(tstate, callable, result, NULL);
}

Py_GC_PRECISE static PyValue
_PyVectorcall_Call(PyThreadState *tstate, vectorcallfunc func,
                   PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    assert(func != NULL);

    if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
        return vectorcall_call_dict(tstate, func, callable, tuple, kwargs);
    }
    /* The tuple owns the argument array, so this frame's span anchors it
       for the call (10a-2.md, rule 3). */
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, tuple);
    PyValue res = func(PyValue_FromObject(callable), _PyTuple_ITEMS(tuple),
                       PyTuple_GET_SIZE(tuple), NULL);
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}


Py_GC_PRECISE PyObject *
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

    return PyValue_Object(
        _PyVectorcall_Call(tstate, func, callable, tuple, kwargs));
}


Py_GC_PRECISE PyObject *
PyObject_Vectorcall(PyObject *callable, PyObject *const *args,
                     size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, callable,
                                      args, nargsf, kwnames);
}


Py_GC_PRECISE PyValue
PyObject_VectorcallValue(PyValue callable, const PyValue *args,
                         size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallValueTstate(tstate, callable,
                                           args, nargsf, kwnames);
}


Py_GC_PRECISE PyObject *
_PyObject_Call(PyThreadState *tstate, PyObject *callable,
               PyObject *args, PyObject *kwargs)
{
    ternaryfunc call;
    PyObject *result;
    PyValue slots[3];
    _PyValueRoot span;

    /* PyObject_Call() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
    assert(PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    vectorcallfunc vector_func = PyVectorcall_Function(callable);
    if (vector_func != NULL) {
        return PyValue_Object(
            _PyVectorcall_Call(tstate, vector_func, callable, args, kwargs));
    }
    else {
        call = Py_TYPE(callable)->tp_call;
        if (call == NULL) {
            object_is_not_callable(tstate, callable);
            return NULL;
        }

        /* This frame's span anchors the parameters across the recursion
           check, and callable across the call for the result check
           (10a-2.md, rule 1). */
        _PyThreadState_PushValueSpan(tstate, &span, slots);
        _PyValueSpan_AppendObject(&span, callable);
        _PyValueSpan_AppendObject(&span, args);
        _PyValueSpan_AppendObject(&span, kwargs);
        if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
            _PyThreadState_PopValueRoot(tstate, &span);
            return NULL;
        }

        result = (*call)(callable, args, kwargs);

        _Py_LeaveRecursiveCallTstate(tstate);

        result = _Py_CheckFunctionResult(tstate, callable, result, NULL);
        _PyThreadState_PopValueRoot(tstate, &span);
        return result;
    }
}

Py_GC_PRECISE PyObject *
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


Py_GC_PRECISE PyObject *
PyObject_CallOneArg(PyObject *func, PyObject *arg)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    assert(arg != NULL);
    PyThreadState *tstate = _PyThreadState_GET();
    /* The argument array is this frame's, so its span anchors arg for the
       call (10a-2.md, rule 2): the free slot for
       PY_VECTORCALL_ARGUMENTS_OFFSET, then arg's value. */
    PyValue _args[2];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, _args);
    _PyValueSpan_Append(&span, PyValue_NULL);
    _PyValueSpan_Append(&span, PyValue_FromObject(arg));
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyValue res = _PyObject_VectorcallValueTstate(
        tstate, PyValue_FromObject(func), _args + 1, nargsf, NULL);
    _PyThreadState_PopValueRoot(tstate, &span);
    return PyValue_Object(res);
}


/* --- PyFunction call functions ---------------------------------- */

Py_GC_PRECISE PyValue
_PyFunction_Vectorcall(PyValue func, const PyValue *stack,
                       size_t nargsf, PyObject *kwnames)
{
    assert(PyFunction_Check(PyValue_ObjectBorrow(func)));
    PyFunctionObject *f = (PyFunctionObject *)PyValue_ObjectBorrow(func);
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
Py_GC_PRECISE PyAPI_FUNC(PyObject*)
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
Py_GC_PRECISE PyObject *
_PyObject_Call_Prepend(PyThreadState *tstate, PyObject *callable,
                       PyObject *obj, PyObject *args, PyObject *kwargs)
{
    assert(PyTuple_Check(args));

    PyValue small_stack[_PY_FASTCALL_SMALL_STACK];
    PyValue *stack;

    Py_ssize_t argcount = PyTuple_GET_SIZE(args);
    if (argcount + 1 <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc((argcount + 1) * sizeof(PyValue));
        if (stack == NULL) {
            return PyErr_NoMemory();
        }
    }

    /* This frame creates the argument array, so the array is its span:
       obj, then the tuple's items (10a-2.md, rule 2). */
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, stack);
    _PyValueSpan_Append(&span, PyValue_FromObject(obj));
    const PyValue *items = _PyTuple_ITEMS(args);
    for (Py_ssize_t i = 0; i < argcount; i++) {
        _PyValueSpan_Append(&span, items[i]);
    }

    PyValue result = _PyObject_VectorcallDictValueTstate(
        tstate, PyValue_FromObject(callable), stack, argcount + 1, kwargs);
    _PyThreadState_PopValueRoot(tstate, &span);
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return PyValue_Object(result);
}


/* --- Call with a format string ---------------------------------- */

static PyObject *
_PyObject_CallFunctionVa(PyThreadState *tstate, PyObject *callable,
                         const char *format, va_list va)
{
    PyObject* small_stack[_PY_FASTCALL_SMALL_STACK];
    const Py_ssize_t small_stack_len = Py_ARRAY_LENGTH(small_stack);
    PyObject **stack;
    Py_ssize_t nargs, i;
    PyObject *result;

    if (callable == NULL) {
        return null_error(tstate);
    }

    if (!format || !*format) {
        return _PyObject_CallNoArgsTstate(tstate, callable);
    }

    stack = _Py_VaBuildStack(small_stack, small_stack_len,
                             format, va, &nargs);
    if (stack == NULL) {
        return NULL;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    if (nargs == 1 && PyTuple_Check(stack[0])) {
        /* Special cases for backward compatibility:
           - PyObject_CallFunction(func, "O", tuple) calls func(*tuple)
           - PyObject_CallFunction(func, "(OOO)", arg1, arg2, arg3) calls
             func(*(arg1, arg2, arg3)): func(arg1, arg2, arg3) */
        PyObject *args = stack[0];
        result = PyValue_Object(_PyObject_VectorcallValueTstate(
            tstate, PyValue_FromObject(callable), _PyTuple_ITEMS(args),
            PyTuple_GET_SIZE(args), NULL));
    }
    else {
        result = _PyObject_VectorcallTstate(tstate, callable,
                                            stack, nargs, NULL);
    }

    for (i = 0; i < nargs; ++i) {
        Py_DECREF(stack[i]);
    }
    if (stack != small_stack) {
        PyMem_Free(stack);
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
    PyObject *result = NULL;

    _PyValueRoot self, method;
    _PyThreadState_PushValueRoot(tstate, &self);
    _PyThreadState_PushValueRoot(tstate, &method);
    self.value = PyValue_FromObject(obj);

    int res = _PyObject_GetMethodValue(tstate, &self.value, name, &method.value);
    if (res < 0) {
        goto exit;
    }

    PyObject *callable = _PyValueRoot_Object(&method);
    PyObject *self_obj = _PyValueRoot_Object(&self);

    if (!PyCallable_Check(callable)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "attribute of type '%.200s' is not callable",
                      Py_TYPE(callable)->tp_name);
        goto exit;
    }

    /* Build the positional arguments from the format string. */
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    Py_ssize_t nargs = 0;
    PyObject **built = NULL;
    if (format != NULL && *format != '\0') {
        built = _Py_VaBuildStack(small_stack, _PY_FASTCALL_SMALL_STACK,
                                 format, va, &nargs);
        if (built == NULL) {
            goto exit;
        }
    }

    /* Backward compat: a single tuple from "O" is unpacked. */
    PyObject *const *args = built;
    Py_ssize_t n = nargs;
    int boxed = 1;
    if (nargs == 1 && PyTuple_Check(built[0])) {
        args = _PyTuple_Objects(built[0]);
        n = PyTuple_GET_SIZE(built[0]);
        boxed = args != NULL;
    }

    if (!boxed) {
        result = NULL;
    }
    else if (self_obj != NULL) {
        result = _PyObject_VectorcallPrepend(tstate, callable, self_obj, args, n, NULL);
    }
    else {
        result = _PyObject_VectorcallTstate(tstate, callable, args, n, NULL);
    }

    for (Py_ssize_t i = 0; i < nargs; i++) {
        Py_DECREF(built[i]);
    }
    if (built != small_stack) {
        PyMem_Free(built);
    }

exit:
    _PyThreadState_PopValueRoot(tstate, &method);
    _PyThreadState_PopValueRoot(tstate, &self);
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

Py_GC_PRECISE PyValue
_PyObject_VectorcallPrependValue(PyThreadState *tstate, PyValue callable,
                                 PyValue arg, const PyValue *args,
                                 size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyValue result;
    _PyValueRoot span;

    if (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET) {
        /* PY_VECTORCALL_ARGUMENTS_OFFSET is set, so we are allowed to
           mutate the vector.  The slot is borrowed and may hold a live
           value of its creator, so this frame's span holds the saved
           value, and arg, while the slot holds arg (10a-2.md, rule 4). */
        PyValue *shifted = (PyValue *)args - 1;
        PyValue saved[2];
        _PyThreadState_PushValueSpan(tstate, &span, saved);
        _PyValueSpan_Append(&span, shifted[0]);
        _PyValueSpan_Append(&span, arg);
        shifted[0] = arg;
        result = _PyObject_VectorcallValueTstate(
            tstate, callable, shifted, nargs + 1, kwnames);
        shifted[0] = saved[0];
        _PyThreadState_PopValueRoot(tstate, &span);
        return result;
    }

    Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t totalargs = nargs + nkwargs;
    PyValue newargs_stack[_PY_FASTCALL_SMALL_STACK];
    PyValue *newargs;
    if (totalargs <= (Py_ssize_t)Py_ARRAY_LENGTH(newargs_stack) - 1) {
        newargs = newargs_stack;
    }
    else {
        newargs = PyMem_Malloc((totalargs + 1) * sizeof(PyValue));
        if (newargs == NULL) {
            _PyErr_NoMemory(tstate);
            return PyValue_NULL;
        }
    }
    /* This frame creates the argument array, so the array is its span:
       arg, then the copies of args (10a-2.md, rule 2). */
    _PyThreadState_PushValueSpan(tstate, &span, newargs);
    _PyValueSpan_Append(&span, arg);
    for (Py_ssize_t i = 0; i < totalargs; i++) {
        _PyValueSpan_Append(&span, args[i]);
    }
    result = _PyObject_VectorcallValueTstate(
        tstate, callable, newargs, nargs + 1,
        totalargs == 0 ? NULL : kwnames);
    _PyThreadState_PopValueRoot(tstate, &span);
    if (newargs != newargs_stack) {
        PyMem_Free(newargs);
    }
    return result;
}

Py_GC_PRECISE PyObject *
_PyObject_VectorcallPrepend(PyThreadState *tstate, PyObject *callable,
                            PyObject *arg, PyObject *const *args,
                            size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs == 0 || args[nargs-1]);
    _PyValueRoot span;

    if (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET) {
        /* PY_VECTORCALL_ARGUMENTS_OFFSET is set, so we are allowed to
           mutate the vector.  As in _PyObject_VectorcallPrependValue(), the
           span holds the saved object and arg while the slot holds arg. */
        PyObject **shifted = (PyObject **)args - 1;
        PyValue saved[2];
        _PyThreadState_PushValueSpan(tstate, &span, saved);
        PyObject *tmp = shifted[0];
        _PyValueSpan_AppendObject(&span, tmp);
        _PyValueSpan_AppendObject(&span, arg);
        shifted[0] = arg;
        assert(shifted[nargs]);
        PyObject *result = _PyObject_VectorcallTstate(tstate, callable,
                                                      shifted, nargs + 1,
                                                      kwnames);
        shifted[0] = tmp;
        _PyThreadState_PopValueRoot(tstate, &span);
        return result;
    }

    Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t totalargs = nargs + nkwargs;
    PyValue newargs_stack[_PY_FASTCALL_SMALL_STACK];
    PyValue *newargs;
    if (totalargs <= (Py_ssize_t)Py_ARRAY_LENGTH(newargs_stack) - 1) {
        newargs = newargs_stack;
    }
    else {
        newargs = PyMem_Malloc((totalargs + 1) * sizeof(PyValue));
        if (newargs == NULL) {
            _PyErr_NoMemory(tstate);
            return NULL;
        }
    }
    /* This frame creates the argument array, of canonical values, so the
       array is its span: arg, then args (10a-2.md, rule 2).  bpo-37138:
       args is not NULL when totalargs > 0. */
    _PyThreadState_PushValueSpan(tstate, &span, newargs);
    _PyValueSpan_Append(&span, PyValue_FromObject(arg));
    for (Py_ssize_t i = 0; i < totalargs; i++) {
        _PyValueSpan_Append(&span, PyValue_FromObject(args[i]));
    }
    PyValue res = _PyObject_VectorcallValueTstate(
        tstate, PyValue_FromObject(callable), newargs, nargs + 1,
        totalargs == 0 ? NULL : kwnames);
    _PyThreadState_PopValueRoot(tstate, &span);
    if (newargs != newargs_stack) {
        PyMem_Free(newargs);
    }
    return PyValue_Object(res);
}

PyObject *
PyObject_VectorcallMethod(PyObject *name, PyObject *const *args,
                           size_t nargsf, PyObject *kwnames)
{
    assert(name != NULL);
    assert(args != NULL);
    assert(PyVectorcall_NARGS(nargsf) >= 1);

    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot self, method;
    _PyThreadState_PushValueRoot(tstate, &self);
    _PyThreadState_PushValueRoot(tstate, &method);
    /* Use args[0] as "self" argument */
    self.value = PyValue_FromObject(args[0]);
    int unbound = _PyObject_GetMethodValue(tstate, &self.value, name, &method.value);
    if (unbound < 0) {
        _PyThreadState_PopValueRoot(tstate, &method);
        _PyThreadState_PopValueRoot(tstate, &self);
        return NULL;
    }

    PyObject *callable = _PyValueRoot_Object(&method);
    PyObject *self_obj = _PyValueRoot_Object(&self);
    PyObject *result;

    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_METHOD, callable);
    if (self_obj == NULL) {
        /* Skip "self". We can keep PY_VECTORCALL_ARGUMENTS_OFFSET since
         * args[-1] in the onward call is args[0] here. */
        result = _PyObject_VectorcallTstate(tstate, callable,
                                            args + 1, nargsf - 1, kwnames);
    }
    else if (self_obj == args[0]) {
        /* We must remove PY_VECTORCALL_ARGUMENTS_OFFSET since
         * that would be interpreted as allowing to change args[-1] */
        result = _PyObject_VectorcallTstate(tstate, callable, args,
                                            nargsf & ~PY_VECTORCALL_ARGUMENTS_OFFSET,
                                            kwnames);
    }
    else {
        /* classmethod: self_obj is the type, not args[0]. Replace
         * args[0] with self_obj and call the underlying callable. */
        result = _PyObject_VectorcallPrepend(tstate, callable, self_obj,
                                             args + 1, nargsf - 1, kwnames);
    }
    _PyThreadState_PopValueRoot(tstate, &method);
    _PyThreadState_PopValueRoot(tstate, &self);
    return result;
}


PyObject *
PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    _PyValueRoot self, method;
    _PyThreadState_PushValueRoot(tstate, &self);
    _PyThreadState_PushValueRoot(tstate, &method);
    self.value = PyValue_FromObject(obj);
    int res = _PyObject_GetMethodValue(tstate, &self.value, name, &method.value);
    if (res < 0) {
        _PyThreadState_PopValueRoot(tstate, &method);
        _PyThreadState_PopValueRoot(tstate, &self);
        return NULL;
    }
    PyObject *callable = _PyValueRoot_Object(&method);
    PyObject *self_obj = _PyValueRoot_Object(&self);

    va_list vargs;
    va_start(vargs, name);
    PyObject *result = object_vacall(tstate, self_obj, callable, vargs);
    va_end(vargs);

    _PyThreadState_PopValueRoot(tstate, &method);
    _PyThreadState_PopValueRoot(tstate, &self);
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
_PyStack_AsDict(const PyValue *values, PyObject *kwnames)
{
    Py_ssize_t nkwargs;

    assert(kwnames != NULL);
    nkwargs = PyTuple_GET_SIZE(kwnames);
    return _PyDict_FromItems(_PyTuple_HEAP_ITEMS(kwnames), 1,
                             values, 1, nkwargs);
}


/* Convert (args, nargs, kwargs: dict) into a (stack, nargs, kwnames: tuple)
   (Include/internal/pycore_call.h). */
const PyValue *
_PyStack_UnpackDict(PyThreadState *tstate,
                    const PyValue *args, Py_ssize_t nargs,
                    PyObject *kwargs, PyObject **p_kwnames)
{
    assert(nargs >= 0);
    assert(kwargs != NULL);
    assert(PyDict_Check(kwargs));

    Py_ssize_t nkwargs = PyDict_GET_SIZE(kwargs);
    /* Check for overflow in the PyMem_Malloc() call below. The subtraction
     * in this check cannot overflow: both maxnargs and nkwargs are
     * non-negative signed integers, so their difference fits in the type. */
    Py_ssize_t maxnargs = PY_SSIZE_T_MAX / sizeof(args[0]) - 1;
    if (nargs > maxnargs - nkwargs) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }

    /* Add 1 to support PY_VECTORCALL_ARGUMENTS_OFFSET */
    PyValue *stack = PyMem_Malloc((1 + nargs + nkwargs) * sizeof(args[0]));
    if (stack == NULL) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }

    PyObject *kwnames = PyTuple_New(nkwargs);
    if (kwnames == NULL) {
        PyMem_Free(stack);
        return NULL;
    }

    stack++;  /* For PY_VECTORCALL_ARGUMENTS_OFFSET */

    for (Py_ssize_t i = 0; i < nargs; i++) {
        stack[i] = args[i];
    }

    /* The values stay in kwargs, which the caller holds, while the call
     * runs.  This loop doesn't support lookup function mutating the
     * dictionary to change its size. It's a deliberate choice for speed,
     * this function is called in the performance critical hot code. */
    PyValue *kwstack = stack + nargs;
    Py_ssize_t pos = 0, i = 0;
    PyValue key, value;
    int keys_are_strings = 1;
    while (_PyDict_NextValue(kwargs, &pos, &key, &value, NULL)) {
        keys_are_strings &= PyType_HasFeature(PyValue_Type(key),
                                              Py_TPFLAGS_UNICODE_SUBCLASS);
        PyTuple_SET_VALUE(kwnames, i, key);
        kwstack[i] = value;
        i++;
    }

    /* Checked once at the end rather than inside the loop above, which
     * simplifies the failing case. */
    if (!keys_are_strings) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "keywords must be strings");
        _PyStack_UnpackDict_Free(stack, nargs, kwnames);
        return NULL;
    }

    *p_kwnames = kwnames;
    return stack;
}

void
_PyStack_UnpackDict_Free(const PyValue *stack, Py_ssize_t nargs,
                         PyObject *kwnames)
{
    PyMem_Free((PyValue *)stack - 1);
}

// Export for the stable ABI
#undef PyVectorcall_NARGS
Py_ssize_t
PyVectorcall_NARGS(size_t n)
{
    return _PyVectorcall_NARGS(n);
}
