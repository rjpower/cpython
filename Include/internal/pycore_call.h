#ifndef Py_INTERNAL_CALL_H
#define Py_INTERNAL_CALL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // EVAL_CALL_STAT_INC_IF_FUNCTION()
#include "pycore_gc_walk.h"       // Py_GC_PRECISE
#include "pycore_value.h"         // _PyThreadState_PushValueRoot()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_stats.h"

/* Flags that determine the C calling convention. */
#define _Py_METH_CALL_FLAGS \
    (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O | \
     METH_KEYWORDS | METH_METHOD)

/* Suggested size (number of positional arguments) for arrays of PyObject*
   allocated on a C stack to avoid allocating memory on the heap memory. Such
   array is used to pass positional arguments to call functions of the
   PyObject_Vectorcall() family.

   The size is chosen to not abuse the C stack and so limit the risk of stack
   overflow. The size is also chosen to allow using the small stack for most
   function calls of the Python standard library. On 64-bit CPU, it allocates
   40 bytes on the stack. */
#define _PY_FASTCALL_SMALL_STACK 5


// Export for 'math' shared extension, used via _PyObject_VectorcallTstate()
// static inline function.
PyAPI_FUNC(PyObject*) _Py_CheckFunctionResult(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *result,
    const char *where);

// _Py_CheckFunctionResult() for a value result.
PyAPI_FUNC(PyValue) _Py_CheckFunctionResultValue(
    PyThreadState *tstate,
    PyObject *callable,
    PyValue result,
    const char *where);

// Call callable(self, *args, **kwargs) on values.
extern PyValue _PyObject_Call_PrependValue(
    PyThreadState *tstate,
    PyValue callable,
    PyValue self,
    PyObject *args,
    PyObject *kwargs);

extern PyObject* _PyObject_VectorcallDictTstate(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwargs);

// PyObject_CallValue() on a thread state.  Exported for signalmodule.
PyAPI_FUNC(PyValue) _PyObject_CallValue(
    PyThreadState *tstate,
    PyValue callable,
    PyObject *args,
    PyObject *kwargs);

// Export for 'array' shared extension
PyAPI_FUNC(PyObject*) _PyObject_CallMethod(
    PyObject *obj,
    PyObject *name,
    const char *format, ...);


// Call callable(arg, *args); arg is placed in args[-1] if nargsf allows.
extern PyValue _PyObject_VectorcallPrependValue(
    PyThreadState *tstate,
    PyValue callable,
    PyValue arg,
    const PyValue *args,
    size_t nargsf,
    PyObject *kwnames);

extern PyObject *_PyObject_VectorcallPrepend(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *arg,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwnames);

/* === Vectorcall protocol (PEP 590) ============================= */

// Call callable using tp_call. Arguments are like PyObject_Vectorcall(),
// except that nargs is plainly the number of arguments without flags.
//
// Call callable through its type's tp_call.  Exported for the static inline
// _PyObject_VectorcallValueTstate().
PyAPI_FUNC(PyValue) _PyObject_MakeTpCall(
    PyThreadState *tstate,
    PyValue callable,
    const PyValue *args, Py_ssize_t nargs,
    PyObject *keywords);

// Static inline variant of public PyVectorcall_Function().
Py_GC_PRECISE static inline vectorcallfunc
_PyVectorcall_FunctionInline(PyObject *callable)
{
    assert(callable != NULL);

    PyTypeObject *tp = Py_TYPE(callable);
    if (!PyType_HasFeature(tp, Py_TPFLAGS_HAVE_VECTORCALL)) {
        return NULL;
    }
    assert(PyCallable_Check(callable));

    Py_ssize_t offset = tp->tp_vectorcall_offset;
    assert(offset > 0);

    vectorcallfunc ptr;
    memcpy(&ptr, (char *) callable + offset, sizeof(ptr));
    return ptr;
}


/* Call the callable object 'callable' with the "vectorcall" calling
   convention.

   args is a C array for positional arguments.

   nargsf is the number of positional arguments plus optionally the flag
   PY_VECTORCALL_ARGUMENTS_OFFSET which means that the caller is allowed to
   modify args[-1].

   kwnames is a tuple of keyword names. The values of the keyword arguments
   are stored in "args" after the positional arguments (note that the number
   of keyword arguments does not change nargsf). kwnames can also be NULL if
   there are no keyword arguments.

   keywords must only contain strings and all keys must be unique.

   Return the result on success. Raise an exception and return PyValue_NULL
   on error.

   Precise (boil/design/work/10a-2.md): the result check names callable_o
   after the call, so the frame's span holds it across the call. */
Py_GC_PRECISE static inline PyValue
_PyObject_VectorcallValueTstate(PyThreadState *tstate, PyValue callable,
                                const PyValue *args, size_t nargsf,
                                PyObject *kwnames)
{
    assert(kwnames == NULL || PyTuple_Check(kwnames));
    assert(args != NULL || PyVectorcall_NARGS(nargsf) == 0);

    if (!PyValue_IsInline(callable)) {
        PyObject *callable_o = PyValue_ObjectBorrow(callable);
        vectorcallfunc func = _PyVectorcall_FunctionInline(callable_o);
        if (func != NULL) {
            _PyValueRoot root;
            _PyThreadState_PushValueRoot(tstate, &root);
            root.object = callable_o;
            PyValue res = func(callable, args, nargsf, kwnames);
            PyValue checked = _Py_CheckFunctionResultValue(
                tstate, callable_o, res, NULL);
            _PyThreadState_PopValueRoot(tstate, &root);
            return checked;
        }
    }
    return _PyObject_MakeTpCall(tstate, callable, args,
                                PyVectorcall_NARGS(nargsf), kwnames);
}

/* The call on objects (PyObject_Vectorcall()): the canonical values of the
   arguments are passed to _PyObject_VectorcallValueTstate(), and the result
   is boxed. */
PyAPI_FUNC(PyObject *) _PyObject_VectorcallTstate(
    PyThreadState *tstate, PyObject *callable,
    PyObject *const *args, size_t nargsf,
    PyObject *kwnames);

extern PyValue _PyObject_VectorcallDictValueTstate(
    PyThreadState *tstate, PyValue callable,
    const PyValue *args, size_t nargsf,
    PyObject *kwargs);


Py_GC_PRECISE static inline PyObject *
_PyObject_CallNoArgsTstate(PyThreadState *tstate, PyObject *func) {
    return PyValue_Object(_PyObject_VectorcallValueTstate(
        tstate, PyValue_FromObject(func), NULL, 0, NULL));
}


// Private static inline function variant of public PyObject_CallNoArgs()
Py_GC_PRECISE static inline PyObject *
_PyObject_CallNoArgs(PyObject *func) {
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_CallNoArgsTstate(tstate, func);
}


/* Convert (args, nargs, kwargs: dict) into (stack, nargs, kwnames: tuple):
   a new argument array, with a free slot before it for
   PY_VECTORCALL_ARGUMENTS_OFFSET, holding the positional arguments and then
   the keyword values.  NULL with an exception set on error, such as a key
   that is not a str.  Free the array with _PyStack_UnpackDict_Free(). */
PyAPI_FUNC(const PyValue *)
_PyStack_UnpackDict(PyThreadState *tstate,
    const PyValue *args, Py_ssize_t nargs,
    PyObject *kwargs, PyObject **p_kwnames);

// Exported for external JIT support
PyAPI_FUNC(void) _PyStack_UnpackDict_Free(
    const PyValue *stack,
    Py_ssize_t nargs,
    PyObject *kwnames);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CALL_H */
