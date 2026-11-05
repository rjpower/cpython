#ifndef Py_INTERNAL_VALUE_CALL_H
#define Py_INTERNAL_VALUE_CALL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_value.h"         // PyValue_Borrow()
#include "pycore_value_slots.h"   // _Py_vectorcallvaluefunc

/* The value vectorcall (pyjit/design/proposals/value-api-m3.md, step M3a).
 *
 * A value vectorcall (_Py_vectorcallvaluefunc) takes a window of words that
 * the caller owns for the call: in the interpreter the frame's call window,
 * in C code a C array of owners or a call-scratch array written by the
 * window primitives of pycore_value.h. The callee borrows each element
 * (PyValue_Borrow(args[i])) and takes a count (PyValue_DUP) for anything it
 * keeps. nargsf carries PY_VECTORCALL_ARGUMENTS_OFFSET and kwnames is a
 * tuple of str or NULL, as in PEP 590. The result is owned, or PyValue_NULL
 * with an exception set.
 *
 * Core code calls values through _PyValue_VectorcallTstate() and the
 * fixed-arity forms below, never through an object vectorcall: each core
 * callable keeps its object vectorcall field only as a thunk for the object
 * call API. */

/* METH_VALUE (section 1.5): a call convention of its own, disjoint from
 * METH_FASTCALL, METH_O and METH_NOARGS. Valid alone or with METH_METHOD,
 * and with METH_CLASS, METH_STATIC, METH_COEXIST. ml_meth is a
 * _PyCFunctionValue, or a _PyCMethodValue with METH_METHOD, cast through
 * PyCFunction. The function checks its own arity. Include/methodobject.h
 * reserves the bit. */
#define METH_VALUE 0x0400

typedef PyValue (*_PyCFunctionValue)(
    PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
    PyObject *kwnames);
typedef PyValue (*_PyCMethodValue)(
    PyValueBorrow self, PyTypeObject *defining_class,
    const PyValue *args, Py_ssize_t nargs, PyObject *kwnames);

/* Value callees are core code, so their result is only asserted, as the
 * interpreter's specialised call handlers assert it: PyValue_NULL if and
 * only if an exception is set. The adapters, where object code returns,
 * make upstream's release check (_Py_CheckFunctionResult). */
#define _PyValue_ASSERT_RESULT(TSTATE, RES) \
    assert(PyValue_IsNull(RES) == (_PyErr_Occurred(TSTATE) != NULL))

/* The instance invariant (value-api-m3.md, section 1.2): a Python function's
 * object vectorcall is the value thunk if and only if its value field is
 * non-NULL (PyFunction_SetVectorcall() keeps the pair together). Checked in
 * the dispatcher and the thunk of a debug build. */
#ifdef Py_DEBUG
PyAPI_FUNC(void) _PyValue_CheckInstanceInvariant(PyObject *callable);
#  define _PyValue_ASSERT_INSTANCE(C) _PyValue_CheckInstanceInvariant(C)
#else
#  define _PyValue_ASSERT_INSTANCE(C) ((void)0)
#endif

/* upstream's "'%.200s' object is not callable" for the type of v. Returns
 * PyValue_NULL. */
extern PyValue _PyValue_NotCallable(PyThreadState *tstate, PyValueBorrow v);
#define _PyValue_NotCallable(T, V) (_PyValue_NotCallable)(T, PyValue_Borrow(V))

/* The object vectorcall of an object-form native (value field NULL): boxes
 * the inline words of args (nargsf's offset slot included) on the borrow
 * stack (kind CALL_ADAPTER) inside a borrow scope, calls of, checks the
 * result as _Py_CheckFunctionResult and converts it with
 * PyValue_FromObjectSteal. */
extern PyValue _PyValue_CallAdapter(
    PyThreadState *tstate, PyObject *callable, vectorcallfunc of,
    const PyValue *args, size_t nargsf, PyObject *kwnames);

/* The tp_call of a callable with no vectorcall and no value call: builds
 * the argument tuple from the words (no box: tuples store words) and the
 * keyword dict, calls tp_call in a borrow scope, checks the result. Raises
 * upstream's "not callable" when tp_call is NULL. */
extern PyValue _PyValue_TpCallAdapter(
    PyThreadState *tstate, PyObject *callable,
    const PyValue *args, size_t nargsf, PyObject *kwnames);

/* The dispatcher (section 1.3): the value form of
 * _PyObject_VectorcallTstate(). An inline value is never callable. A
 * callable whose type has Py_TPFLAGS_HAVE_VECTORCALL is called through its
 * value vectorcall field, or through the call adapter on its object field
 * when the value field is NULL; any other through the type's value call
 * slot, or the tp_call adapter, inside upstream's tp_call recursion guard.
 * Never calls a thunk. */
static inline PyValue
(_PyValue_VectorcallTstate)(PyThreadState *tstate, PyValueBorrow callable,
                          const PyValue *args, size_t nargsf,
                          PyObject *kwnames)
{
    assert(kwnames == NULL || PyTuple_Check(kwnames));
    assert(args != NULL || PyVectorcall_NARGS(nargsf) == 0);
    if (!PyValue_IsObject(callable)) {
        return _PyValue_NotCallable(tstate, callable);
    }
    PyObject *c = PyValue_AsKnownObjectBorrow(callable);
    PyTypeObject *tp = Py_TYPE(c);
    if (tp->tp_flags & Py_TPFLAGS_HAVE_VECTORCALL) {
        _PyValue_ASSERT_INSTANCE(c);
        Py_ssize_t off = tp->tp_value_vectorcall_offset;
        _Py_vectorcallvaluefunc f = NULL;
        if (off != 0) {
            memcpy(&f, (char *)c + off, sizeof(f));
        }
        if (f != NULL) {
            PyValue res = f(tstate, c, args, nargsf, kwnames);
            _PyValue_ASSERT_RESULT(tstate, res);
            return res;
        }
        vectorcallfunc of;
        memcpy(&of, (char *)c + tp->tp_vectorcall_offset, sizeof(of));
        if (of != NULL) {
            return _PyValue_CallAdapter(tstate, c, of, args, nargsf, kwnames);
        }
    }
    /* Type-level calls get upstream's tp_call guard; vectorcall callees
       above keep their own, as upstream's do. */
    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
        return PyValue_NULL;
    }
    PyValue res;
    _Py_vectorcallvaluefunc vf = _PyType_VALUE_SLOT(tp, call);
    if (vf != NULL) {
        res = vf(tstate, c, args, nargsf, kwnames);
        _PyValue_ASSERT_RESULT(tstate, res);
    }
    else {
        res = _PyValue_TpCallAdapter(tstate, c, args, nargsf, kwnames);
    }
    _Py_LeaveRecursiveCallTstate(tstate);
    return res;
}

#define _PyValue_VectorcallTstate(T, C, A, N, K) \
    (_PyValue_VectorcallTstate)(T, PyValue_Borrow(C), A, N, K)

static inline PyValue
_PyValue_Vectorcall(PyValueBorrow callable, const PyValue *args,
                    size_t nargsf, PyObject *kwnames)
{
    return _PyValue_VectorcallTstate(_PyThreadState_GET(), callable, args,
                                     nargsf, kwnames);
}
#define _PyValue_Vectorcall(C, A, N, K) \
    (_PyValue_Vectorcall)(PyValue_Borrow(C), A, N, K)

/* callable(self, *args) (section 1.1, D5). With the offset flag, args[-1]
 * receives PyValue_DUP(self) for the call and gets its word back after;
 * without it, the words are copied into a call-scratch array. */
extern PyValue _PyValue_VectorcallPrepend(
    PyThreadState *tstate, PyValueBorrow callable, PyValueBorrow self,
    const PyValue *args, size_t nargsf, PyObject *kwnames);
#define _PyValue_VectorcallPrepend(T, C, S, A, N, K) \
    (_PyValue_VectorcallPrepend)(T, PyValue_Borrow(C), PyValue_Borrow(S), \
                                 A, N, K)

/* callable(*args, **kwargs), args a tuple and kwargs a dict or NULL: the
 * body of the object call API's tuple entries (D11). The tuple's words are
 * the window; a non-empty dict expands into a call-scratch array, an empty
 * one into NULL kwnames. A tp_call callee reached with nothing to convert
 * receives the original tuple and dict. */
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(PyValue) _PyValue_Call(PyThreadState *tstate,
                                  PyValueBorrow callable,
                                  PyObject *args, PyObject *kwargs);
#define _PyValue_Call(T, C, A, K) (_PyValue_Call)(T, PyValue_Borrow(C), A, K)

/* The fixed-arity forms. Each reserves args[-1] for a bound callee. The
 * method forms (_PyValue_VectorcallMethod, _PyValue_CallMethodOneArg) come
 * with W2's first batch, once M2's value method lookup exists. */
extern PyValue _PyValue_CallNoArgs(PyThreadState *tstate,
                                   PyValueBorrow callable);
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(PyValue) _PyValue_CallOneArg(PyThreadState *tstate,
                                        PyValueBorrow callable,
                                        PyValueBorrow arg);
#define _PyValue_CallNoArgs(T, C) (_PyValue_CallNoArgs)(T, PyValue_Borrow(C))
#define _PyValue_CallOneArg(T, C, A) \
    (_PyValue_CallOneArg)(T, PyValue_Borrow(C), PyValue_Borrow(A))

/* The call-scratch window of args[0..nargs) followed by the values of
 * kwargs, a non-empty dict, with one reserved slot before it; *p_kwnames
 * receives a new tuple of the keys. The positional words are copied (the
 * caller keeps them); the dict values are owners, because the callee may
 * mutate a dict that other code shares. Returns the window (element 0, not
 * the reserved slot) or NULL with an exception set. Free it with
 * _PyValue_StackUnpackDict_Free(). */
// Export for shared extensions with Clinic value emission (M3b).
PyAPI_FUNC(PyValue *) _PyValue_StackUnpackDict(
    PyThreadState *tstate, const PyValue *args, Py_ssize_t nargs,
    PyObject *kwargs, PyObject **p_kwnames);
PyAPI_FUNC(void) _PyValue_StackUnpackDict_Free(
    PyValue *window, Py_ssize_t nargs, PyObject *kwnames);

/* A new dict of kwnames[i] -> values[i] (owners taken, no box): the
 * keyword dict of an object-form callee that takes one. */
// Export for '_testinternalcapi' and shared extensions with Clinic value
// emission (M3b).
PyAPI_FUNC(PyObject *) _PyValue_StackAsDict(const PyValue *values,
                                            PyObject *kwnames);

/* The value side of the tp_call row's pair with PyVectorcall_Call (V2a-0
 * D14): calls the instance's value vectorcall, or the call adapter on its
 * object vectorcall when the value field is NULL. */
extern PyValue _PyValue_InstanceVectorcall(
    PyThreadState *tstate, PyObject *callable,
    const PyValue *args, size_t nargsf, PyObject *kwnames);

/* The object tp_call of a type whose tp_value.call is set (the pair row of
 * section 1.2): the tuple's words are the window, the dict expands as in
 * _PyValue_Call(), and the result leaves through _PyValue_THUNK_EXIT(). */
extern PyObject *_PyValue_TpCallThunk(PyObject *callable, PyObject *args,
                                      PyObject *kwargs);

/* The object vectorcall of every core callable with a value vectorcall
 * field other than a Python function (D8): converts the object vector into
 * a call-scratch window with _PyValue_WindowFromObjects(), calls the
 * instance's value vectorcall with the offset flag and leaves through
 * _PyValue_THUNK_EXIT(). Only the creators of section 1.4 name it. */
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(PyObject *) _PyValue_VectorcallThunk(
    PyObject *callable, PyObject *const *args, size_t nargsf,
    PyObject *kwnames);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_CALL_H */
