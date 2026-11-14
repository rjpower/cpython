/* Descriptors -- a new, flexible way to describe attributes */

#include "Python.h"
#include "pycore_borrow.h"        // _PyBorrowScope_Enter()
#include "pycore_call.h"          // _PyStack_AsDict()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_dict.h"          // PyDict_ContainsValue()
#include "pycore_wasm_trampoline.h" // descr_set_trampoline_call(), descr_get_trampoline_call()
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type, _PyValueGetSet
#include "pycore_modsupport.h"    // _PyArg_UnpackStack()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_object_deferred.h" // _PyObject_SetDeferredRefcount()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS(), _PyTuple_FromValueArray()
#include "pycore_value.h"         // _PyObject_IsSame()
#include "pycore_value_call.h"    // _PyValue_VectorcallTstate()
#include "pycore_value_ops.h"     // _PyValue_GetOptionalAttr(), _PyValue_Repr()
#include "pycore_value_slots.h"   // _PyType_ValueWrapper()


/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class mappingproxy "mappingproxyobject *" "&PyDictProxy_Type"
class property "propertyobject *" "&PyProperty_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=556352653fd4c02e]*/

#include "clinic/descrobject.c.h"

static void
descr_dealloc(PyObject *self)
{
    PyDescrObject *descr = (PyDescrObject *)self;
    _PyObject_GC_UNTRACK(descr);
    Py_XDECREF(descr->d_type);
    Py_XDECREF(descr->d_name);
    Py_XDECREF(descr->d_qualname);
    PyObject_GC_Del(descr);
}

static PyObject *
descr_name(PyDescrObject *descr)
{
    if (descr->d_name != NULL && PyUnicode_Check(descr->d_name))
        return descr->d_name;
    return NULL;
}

static PyObject *
descr_repr(PyDescrObject *descr, const char *kind)
{
    PyObject *name = NULL;
    if (descr->d_name != NULL && PyUnicode_Check(descr->d_name))
        name = descr->d_name;

    if (descr->d_type == &PyBaseObject_Type) {
        return PyUnicode_FromFormat("<%s '%V'>", kind, name, "?");
    }
    return PyUnicode_FromFormat("<%s '%V' of '%s' objects>",
                                kind, name, "?", descr->d_type->tp_name);
}

static PyObject *
method_repr(PyObject *descr)
{
    return descr_repr((PyDescrObject *)descr, "method");
}

static PyObject *
member_repr(PyObject *descr)
{
    return descr_repr((PyDescrObject *)descr, "member");
}

static PyObject *
getset_repr(PyObject *descr)
{
    return descr_repr((PyDescrObject *)descr, "attribute");
}

static PyObject *
wrapperdescr_repr(PyObject *descr)
{
    return descr_repr((PyDescrObject *)descr, "slot wrapper");
}

static int
descr_check_value(PyDescrObject *descr, PyValueBorrow obj)
{
    PyTypeObject *type = PyValue_TYPE(obj);
    if (type != descr->d_type && !PyType_IsSubtype(type, descr->d_type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for '%.100s' objects "
                     "doesn't apply to a '%.100s' object",
                     descr_name((PyDescrObject *)descr), "?",
                     descr->d_type->tp_name,
                     type->tp_name);
        return -1;
    }
    return 0;
}

static int
descr_check(PyDescrObject *descr, PyObject *obj)
{
    return descr_check_value(descr, _Py_VALUE_ARG(obj));
}

static PyObject *
classmethod_get(PyObject *self, PyObject *obj, PyObject *type)
{
    PyMethodDescrObject *descr = (PyMethodDescrObject *)self;
    /* Ensure a valid type.  Class methods ignore obj. */
    if (type == NULL) {
        if (obj != NULL)
            type = (PyObject *)Py_TYPE(obj);
        else {
            /* Wot - no type?! */
            PyErr_Format(PyExc_TypeError,
                         "descriptor '%V' for type '%.100s' "
                         "needs either an object or a type",
                         descr_name((PyDescrObject *)descr), "?",
                         PyDescr_TYPE(descr)->tp_name);
            return NULL;
        }
    }
    if (!PyType_Check(type)) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for type '%.100s' "
                     "needs a type, not a '%.100s' as arg 2",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     Py_TYPE(type)->tp_name);
        return NULL;
    }
    if (!PyType_IsSubtype((PyTypeObject *)type, PyDescr_TYPE(descr))) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' requires a subtype of '%.100s' "
                     "but received '%.100s'",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     ((PyTypeObject *)type)->tp_name);
        return NULL;
    }
    PyTypeObject *cls = NULL;
    if (descr->d_method->ml_flags & METH_METHOD) {
        cls = descr->d_common.d_type;
    }
    return _PyCFunction_NewValue(descr->d_method,
                                 PyValue_FromObjectBorrow(type), NULL, cls);
}

static PyValue
method_get_value(PyObject *self, PyValueBorrow obj, PyObject *type)
{
    PyMethodDescrObject *descr = (PyMethodDescrObject *)self;
    if (PyValue_IsNull(obj)) {
        return PyValue_DUP(PyValue_FromObjectBorrow(self));
    }
    if (descr_check_value((PyDescrObject *)descr, obj) < 0) {
        return PyValue_NULL;
    }
    if (descr->d_method->ml_flags & METH_METHOD) {
        if (type == NULL || PyType_Check(type)) {
            return _PyValue_FromObjectResult(
                _PyCFunction_NewValue(descr->d_method, obj, NULL,
                                      descr->d_common.d_type));
        } else {
            PyErr_Format(PyExc_TypeError,
                        "descriptor '%V' needs a type, not '%s', as arg 2",
                        descr_name((PyDescrObject *)descr), "?",
                        Py_TYPE(type)->tp_name);
            return PyValue_NULL;
        }
    } else {
        return _PyValue_FromObjectResult(
            _PyCFunction_NewValue(descr->d_method, obj, NULL, NULL));
    }
}

_Py_VALUE_THUNK_DESCRGET(method_get, method_get_value)

static PyValue
member_get_value(PyObject *self, PyValueBorrow obj, PyObject *type)
{
    PyMemberDescrObject *descr = (PyMemberDescrObject *)self;
    if (PyValue_IsNull(obj)) {
        return PyValue_DUP(PyValue_FromObjectBorrow(self));
    }
    if (descr_check_value((PyDescrObject *)descr, obj) < 0) {
        return PyValue_NULL;
    }
    // No member descriptor applies to an exact int or float.
    assert(!PyValue_IsInline(obj));
    PyObject *object = PyValue_AsKnownObjectBorrow(obj);

    if (descr->d_member->flags & Py_AUDIT_READ) {
        if (PySys_Audit("object.__getattr__", "Os",
            object, descr->d_member->name) < 0) {
            return PyValue_NULL;
        }
    }

    return _PyMember_GetOneValue((char *)object, descr->d_member);
}

_Py_VALUE_THUNK_DESCRGET(member_get, member_get_value)

static PyValue
getset_get_value(PyObject *self, PyValueBorrow obj, PyObject *type)
{
    PyGetSetDescrObject *descr = (PyGetSetDescrObject *)self;
    if (PyValue_IsNull(obj)) {
        return PyValue_DUP(PyValue_FromObjectBorrow(self));
    }
    if (descr_check_value((PyDescrObject *)descr, obj) < 0) {
        return PyValue_NULL;
    }
    if (descr->d_getset->get != NULL) {
        if (descr->d_value != NULL) {
            const _PyValueGetSet *record = descr->d_value;
            return record->get(obj, record->closure);
        }
        PyObject *box = _PyValue_AdapterBox(obj, BOX_SITE_ATTRIBUTE, descr->d_getset->get);
        if (box == NULL) {
            return PyValue_NULL;
        }
        PyObject *result = descr_get_trampoline_call(
            descr->d_getset->get, box, descr->d_getset->closure);
        _PyValue_AdapterRelease(obj, box);
        return _PyValue_FromObjectResult(result);
    }
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%V' of '%.100s' objects is not readable",
                 descr_name((PyDescrObject *)descr), "?",
                 PyDescr_TYPE(descr)->tp_name);
    return PyValue_NULL;
}

_Py_VALUE_THUNK_DESCRGET(getset_get, getset_get_value)

PyObject *
_PyGetSet_ValueGet(PyObject *self, void *record)
{
    const _PyValueGetSet *r = record;
    return _PyValue_THUNK_EXIT(r->get(_Py_VALUE_ARG(self), r->closure), BOX_SITE_ATTRIBUTE);
}

int
_PyGetSet_ValueSet(PyObject *self, PyObject *value, void *record)
{
    const _PyValueGetSet *r = record;
    if (r->set == NULL) {
        PyErr_SetString(PyExc_AttributeError, "attribute is not writable");
        return -1;
    }
    return r->set(_Py_VALUE_ARG(self), _Py_VALUE_ARG(value), r->closure);
}

static PyObject *
wrapperdescr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    PyWrapperDescrObject *descr = (PyWrapperDescrObject *)self;
    if (obj == NULL) {
        return Py_NewRef(descr);
    }
    if (descr_check((PyDescrObject *)descr, obj) < 0) {
        return NULL;
    }
    return _PyWrapper_NewValue((PyObject *)descr, PyValue_FromObjectBorrow(obj));
}

static int
member_set_value(PyObject *self, PyValueBorrow obj, PyValueBorrow value)
{
    PyMemberDescrObject *descr = (PyMemberDescrObject *)self;
    assert(!PyValue_IsNull(obj));
    if (descr_check_value((PyDescrObject *)descr, obj) < 0) {
        return -1;
    }
    assert(!PyValue_IsInline(obj));
    return _PyMember_SetOneValue((char *)PyValue_AsKnownObjectBorrow(obj),
                                 descr->d_member, value);
}

_Py_VALUE_THUNK_DESCRSET(member_set, member_set_value)

static int
getset_set_value(PyObject *self, PyValueBorrow obj, PyValueBorrow value)
{
    PyGetSetDescrObject *descr = (PyGetSetDescrObject *)self;
    assert(!PyValue_IsNull(obj));
    if (descr_check_value((PyDescrObject *)descr, obj) < 0) {
        return -1;
    }
    if (descr->d_getset->set != NULL) {
        const _PyValueGetSet *record = descr->d_value;
        if (record != NULL && record->set != NULL) {
            return record->set(obj, value, record->closure);
        }
        if (record == NULL) {
            PyObject *box = _PyValue_AdapterBox(obj, BOX_SITE_ATTRIBUTE, descr->d_getset->set);
            if (box == NULL) {
                return -1;
            }
            PyObject *vbox = NULL;
            if (!PyValue_IsNull(value)) {
                vbox = _PyValue_AdapterBox(value, BOX_SITE_ATTRIBUTE, descr->d_getset->set);
                if (vbox == NULL) {
                    _PyValue_AdapterRelease(obj, box);
                    return -1;
                }
            }
            int res = descr_set_trampoline_call(
                descr->d_getset->set, box, vbox,
                descr->d_getset->closure);
            if (vbox != NULL) {
                _PyValue_AdapterRelease(value, vbox);
            }
            _PyValue_AdapterRelease(obj, box);
            return res;
        }
    }
    PyErr_Format(PyExc_AttributeError,
                 "attribute '%V' of '%.100s' objects is not writable",
                 descr_name((PyDescrObject *)descr), "?",
                 PyDescr_TYPE(descr)->tp_name);
    return -1;
}

_Py_VALUE_THUNK_DESCRSET(getset_set, getset_set_value)


/* Vectorcall functions for each of the PyMethodDescr calling conventions.
 *
 * First, common helpers
 */
static inline int
method_check_args(PyObject *func, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    assert(!PyErr_Occurred());
    if (nargs < 1) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method %U needs an argument", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    PyObject *self = args[0];
    if (descr_check((PyDescrObject *)func, self) < 0) {
        return -1;
    }
    if (kwnames && PyTuple_GET_SIZE(kwnames)) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U takes no keyword arguments", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return 0;
}

typedef void (*funcptr)(void);

static inline funcptr
method_enter_call(PyThreadState *tstate, PyObject *func)
{
    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
        return NULL;
    }
    return (funcptr)((PyMethodDescrObject *)func)->d_method->ml_meth;
}

/* Now the actual vectorcall functions */
static PyObject *
method_vectorcall_VARARGS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    PyObject *argstuple = PyTuple_FromArray(args+1, nargs-1);
    if (argstuple == NULL) {
        return NULL;
    }
    PyCFunction meth = (PyCFunction)method_enter_call(tstate, func);
    if (meth == NULL) {
        Py_DECREF(argstuple);
        return NULL;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = _PyCFunction_TrampolineCall(
        meth, args[0], argstuple);
    _PyBorrowScope_Exit(tstate, mark);
    Py_DECREF(argstuple);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_VARARGS_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, NULL)) {
        return NULL;
    }
    PyObject *argstuple = PyTuple_FromArray(args+1, nargs-1);
    if (argstuple == NULL) {
        return NULL;
    }
    PyObject *result = NULL;
    /* Create a temporary dict for keyword arguments */
    PyObject *kwdict = NULL;
    if (kwnames != NULL && PyTuple_GET_SIZE(kwnames) > 0) {
        kwdict = _PyStack_AsDict(args + nargs, kwnames);
        if (kwdict == NULL) {
            goto exit;
        }
    }
    PyCFunctionWithKeywords meth = (PyCFunctionWithKeywords)
                                   method_enter_call(tstate, func);
    if (meth == NULL) {
        goto exit;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    result = _PyCFunctionWithKeywords_TrampolineCall(
        meth, args[0], argstuple, kwdict);
    _PyBorrowScope_Exit(tstate, mark);
    _Py_LeaveRecursiveCallTstate(tstate);
exit:
    Py_DECREF(argstuple);
    Py_XDECREF(kwdict);
    return result;
}

static PyObject *
method_vectorcall_FASTCALL_KEYWORDS_METHOD(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, NULL)) {
        return NULL;
    }
    PyCMethod meth = (PyCMethod) method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = meth(args[0],
                            ((PyMethodDescrObject *)func)->d_common.d_type,
                            args+1, nargs-1, kwnames);
    _PyBorrowScope_Exit(tstate, mark);
    _Py_LeaveRecursiveCall();
    return result;
}

static PyObject *
method_vectorcall_FASTCALL(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    PyCFunctionFast meth = (PyCFunctionFast)
                            method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = meth(args[0], args+1, nargs-1);
    _PyBorrowScope_Exit(tstate, mark);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_FASTCALL_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, NULL)) {
        return NULL;
    }
    PyCFunctionFastWithKeywords meth = (PyCFunctionFastWithKeywords)
                                        method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = meth(args[0], args+1, nargs-1, kwnames);
    _PyBorrowScope_Exit(tstate, mark);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_NOARGS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    if (nargs != 1) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                "%U takes no arguments (%zd given)", funcstr, nargs-1);
            Py_DECREF(funcstr);
        }
        return NULL;
    }
    PyCFunction meth = (PyCFunction)method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = _PyCFunction_TrampolineCall(meth, args[0], NULL);
    _PyBorrowScope_Exit(tstate, mark);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
method_vectorcall_O(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_args(func, args, nargs, kwnames)) {
        return NULL;
    }
    if (nargs != 2) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                "%U takes exactly one argument (%zd given)",
                funcstr, nargs-1);
            Py_DECREF(funcstr);
        }
        return NULL;
    }
    PyCFunction meth = (PyCFunction)method_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = _PyCFunction_TrampolineCall(meth, args[0], args[1]);
    _PyBorrowScope_Exit(tstate, mark);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static inline int
method_check_value_args(PyObject *func, const PyValue *args, Py_ssize_t nargs)
{
    if (nargs < 1) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "unbound method %U needs an argument", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return descr_check_value((PyDescrObject *)func, PyValue_Borrow(args[0]));
}

/* METH_VALUE takes positional arguments only, as METH_FASTCALL does: the
   vectorcall rejects keywords with the message of method_vectorcall_FASTCALL
   and calls ml_meth with no kwnames.  METH_VALUE | METH_KEYWORDS passes kwnames
   to ml_meth, which checks its own arity.  The fixed-arity forms are below. */
static int
method_check_value_kwnames(PyObject *func, PyObject *kwnames)
{
    if (kwnames && PyTuple_GET_SIZE(kwnames)) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "%U takes no keyword arguments", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return 0;
}

/* The call of a checked method: args[0] is a valid self. */
static inline PyValue
method_value_invoke(PyThreadState *tstate, PyObject *func,
                    const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    _PyCFunctionValue meth = (_PyCFunctionValue)method_enter_call(tstate, func);
    if (meth == NULL) {
        return PyValue_NULL;
    }
    PyValue result = meth(PyValue_Borrow(args[0]), args + 1, nargs - 1,
                          kwnames);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyValue
method_value_vectorcall_descr(PyThreadState *tstate, PyObject *func,
                              const PyValue *args, size_t nargsf,
                              PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_value_args(func, args, nargs) < 0) {
        return PyValue_NULL;
    }
    if (method_check_value_kwnames(func, kwnames) < 0) {
        return PyValue_NULL;
    }
    return method_value_invoke(tstate, func, args, nargs, NULL);
}

static PyValue
method_value_vectorcall_descr_KEYWORDS(PyThreadState *tstate, PyObject *func,
                                       const PyValue *args, size_t nargsf,
                                       PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_value_args(func, args, nargs) < 0) {
        return PyValue_NULL;
    }
    return method_value_invoke(tstate, func, args, nargs, kwnames);
}

static PyValue
method_value_vectorcall_descr_METHOD(PyThreadState *tstate, PyObject *func,
                                     const PyValue *args, size_t nargsf,
                                     PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_value_args(func, args, nargs) < 0) {
        return PyValue_NULL;
    }
    _PyCMethodValue meth = (_PyCMethodValue)method_enter_call(tstate, func);
    if (meth == NULL) {
        return PyValue_NULL;
    }
    PyValue result = meth(PyValue_Borrow(args[0]),
                          ((PyMethodDescrObject *)func)->d_common.d_type,
                          args + 1, nargs - 1, kwnames);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}


/* METH_VALUE | METH_NOARGS and METH_VALUE | METH_O: the trampolines of
   method_vectorcall_NOARGS and _O, which check the self, the keywords and
   the number of arguments before the call, with the same messages. */
static int
method_check_value_nargs(PyObject *func, Py_ssize_t nargs, Py_ssize_t expected)
{
    if (nargs == expected + 1) {
        return 0;
    }
    PyObject *funcstr = _PyObject_FunctionStr(func);
    if (funcstr != NULL) {
        PyErr_Format(PyExc_TypeError,
                     expected == 0
                     ? "%U takes no arguments (%zd given)"
                     : "%U takes exactly one argument (%zd given)",
                     funcstr, nargs - 1);
        Py_DECREF(funcstr);
    }
    return -1;
}

static PyValue
method_value_vectorcall_descr_checked(PyThreadState *tstate, PyObject *func,
                                      const PyValue *args, size_t nargsf,
                                      PyObject *kwnames, Py_ssize_t expected)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (method_check_value_args(func, args, nargs) < 0) {
        return PyValue_NULL;
    }
    if (method_check_value_kwnames(func, kwnames) < 0) {
        return PyValue_NULL;
    }
    if (method_check_value_nargs(func, nargs, expected) < 0) {
        return PyValue_NULL;
    }
    return method_value_invoke(tstate, func, args, nargs, kwnames);
}

static PyValue
method_value_vectorcall_descr_NOARGS(PyThreadState *tstate, PyObject *func,
                                     const PyValue *args, size_t nargsf,
                                     PyObject *kwnames)
{
    return method_value_vectorcall_descr_checked(tstate, func, args, nargsf,
                                                 kwnames, 0);
}

static PyValue
method_value_vectorcall_descr_O(PyThreadState *tstate, PyObject *func,
                                const PyValue *args, size_t nargsf,
                                PyObject *kwnames)
{
    return method_value_vectorcall_descr_checked(tstate, func, args, nargsf,
                                                 kwnames, 1);
}

PyValue
_PyMethodDescr_CallFast(PyThreadState *tstate, PyMethodDescrObject *method,
                        const PyValue *args, Py_ssize_t nargs)
{
    assert(Py_IS_TYPE(method, &PyMethodDescr_Type));
    assert(nargs >= 1);
    assert(PyValue_TYPE(args[0]) == method->d_common.d_type);
    int flags = method->d_method->ml_flags & _Py_METH_CALL_FLAGS;
    if (flags == METH_FASTCALL) {
        /* Object methods retain the adapter's boxes and borrow scope. */
        return _PyValue_CallAdapter(tstate, (PyObject *)method,
                                   method->vectorcall, args, nargs, NULL);
    }
    assert((flags & ~(METH_NOARGS | METH_O)) == METH_VALUE);
    if (flags & (METH_NOARGS | METH_O)) {
        Py_ssize_t expected = (flags & METH_O) ? 1 : 0;
        if (method_check_value_nargs((PyObject *)method, nargs, expected) < 0) {
            return PyValue_NULL;
        }
    }
    /* The opcode already checked self and excluded keywords. Recursion and
       the method's own argument validation still belong to this call. */
    PyValue result = method_value_invoke(tstate, (PyObject *)method,
                                        args, nargs, NULL);
    _PyValue_ASSERT_RESULT(tstate, result);
    return result;
}


/* Instances of classmethod_descriptor are unlikely to be called directly.
   For one, the analogous class "classmethod" (for Python classes) is not
   callable. Second, users are not likely to access a classmethod_descriptor
   directly, since it means pulling it from the class __dict__.

   This is just an excuse to say that this doesn't need to be optimized:
   we implement this simply by calling __get__ and then calling the result.
*/
static PyValue
classmethoddescr_value_call(PyThreadState *tstate, PyObject *callable,
                            const PyValue *args, size_t nargsf,
                            PyObject *kwnames)
{
    PyMethodDescrObject *descr = (PyMethodDescrObject *)callable;
    Py_ssize_t argc = PyVectorcall_NARGS(nargsf);
    if (argc < 1) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' of '%.100s' "
                     "object needs an argument",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name);
        return PyValue_NULL;
    }
    PyValueBorrow first = PyValue_Borrow(args[0]);
    if (PyValue_IsInline(first)) {
        /* Not a type: the error classmethod_get gives, without a box. */
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' for type '%.100s' "
                     "needs a type, not a '%.100s' as arg 2",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     PyValue_TYPE(first)->tp_name);
        return PyValue_NULL;
    }
    PyObject *bound = classmethod_get((PyObject *)descr, NULL,
                                      PyValue_AsKnownObjectBorrow(first));
    if (bound == NULL) {
        return PyValue_NULL;
    }
    PyValue res = _PyValue_VectorcallTstate(
        tstate, PyValue_FromObjectBorrow(bound), args + 1, argc - 1, kwnames);
    Py_DECREF(bound);
    return res;
}

/* Calls the slot wrapper of descr on self (section 1.11). */
static PyValue
wrapperdescr_value_raw_call(PyWrapperDescrObject *descr, PyValueBorrow self,
                            const PyValue *args, Py_ssize_t nargs,
                            PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    int has_kw = kwnames != NULL && PyTuple_GET_SIZE(kwnames) != 0;
    int keywords = (descr->d_base->flags & PyWrapperFlag_KEYWORDS) != 0;

    if (!keywords && has_kw) {
        PyErr_Format(PyExc_TypeError,
                     "wrapper %s() takes no keyword arguments",
                     descr->d_base->name);
        return PyValue_NULL;
    }
    if (descr->d_wrapped_value != NULL) {
        _Py_wrappervaluefunc vw = _PyType_ValueWrapper(descr->d_base);
        if (vw != NULL) {
            return vw(self, args, nargs, kwnames, descr->d_wrapped_value);
        }
    }

    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *res = NULL;
    PyObject *obj = _PyValue_AdapterBox(self, BOX_SITE_CALL,
                                       descr->d_base->wrapper);
    if (obj == NULL) {
        goto exit_scope;
    }
    PyObject *argstuple = _PyTuple_FromValueArray(args, nargs);
    if (argstuple == NULL) {
        goto release;
    }
    if (keywords) {
        PyObject *kwdict = NULL;
        if (has_kw) {
            kwdict = _PyValue_StackAsDict(args + nargs, kwnames);
            if (kwdict == NULL) {
                Py_DECREF(argstuple);
                goto release;
            }
        }
        wrapperfunc_kwds wk = _Py_FUNC_CAST(wrapperfunc_kwds,
                                            descr->d_base->wrapper);
        res = (*wk)(obj, argstuple, descr->d_wrapped, kwdict);
        Py_XDECREF(kwdict);
    }
    else {
        res = (*descr->d_base->wrapper)(obj, argstuple, descr->d_wrapped);
    }
    Py_DECREF(argstuple);
release:
    _PyValue_AdapterRelease(self, obj);
exit_scope:
    _PyBorrowScope_Exit(tstate, mark);
    res = _Py_CheckFunctionResult(tstate, (PyObject *)descr, res, NULL);
    return _PyValue_FromObjectResult(res);
}

static PyValue
wrapperdescr_value_call(PyThreadState *tstate, PyObject *callable,
                        const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    PyWrapperDescrObject *descr = (PyWrapperDescrObject *)callable;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);

    /* Make sure that the first argument is acceptable as 'self' */
    if (nargs < 1) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' of '%.100s' "
                     "object needs an argument",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name);
        return PyValue_NULL;
    }
    PyValueBorrow self = PyValue_Borrow(args[0]);
    PyTypeObject *tp = PyValue_TYPE(self);
    if (!PyType_IsSubtype(tp, PyDescr_TYPE(descr))) {
        PyErr_Format(PyExc_TypeError,
                     "descriptor '%V' "
                     "requires a '%.100s' object "
                     "but received a '%.100s'",
                     descr_name((PyDescrObject *)descr), "?",
                     PyDescr_TYPE(descr)->tp_name,
                     tp->tp_name);
        return PyValue_NULL;
    }
    return wrapperdescr_value_raw_call(descr, self, args + 1, nargs - 1,
                                       kwnames);
}


static PyObject *
method_get_doc(PyObject *_descr, void *closure)
{
    PyMethodDescrObject *descr = (PyMethodDescrObject *)_descr;
    return _PyType_GetDocFromInternalDoc(descr->d_method->ml_name, descr->d_method->ml_doc);
}

static PyObject *
method_get_text_signature(PyObject *_descr, void *closure)
{
    PyMethodDescrObject *descr = (PyMethodDescrObject *)_descr;
    return _PyType_GetTextSignatureFromInternalDoc(descr->d_method->ml_name,
                                                   descr->d_method->ml_doc,
                                                   descr->d_method->ml_flags);
}

static PyObject *
calculate_qualname(PyDescrObject *descr)
{
    PyObject *type_qualname, *res;

    if (descr->d_name == NULL || !PyUnicode_Check(descr->d_name)) {
        PyErr_SetString(PyExc_TypeError,
                        "<descriptor>.__name__ is not a unicode object");
        return NULL;
    }

    type_qualname = _PyValue_AsObjectResultAt(
            _PyValue_GetAttr(_PyValue_FromObjectKnown((PyObject *)descr->d_type),
                             &_Py_ID(__qualname__)),
            BOX_SITE_ATTRIBUTE);
    if (type_qualname == NULL)
        return NULL;

    if (!PyUnicode_Check(type_qualname)) {
        PyErr_SetString(PyExc_TypeError, "<descriptor>.__objclass__."
                        "__qualname__ is not a unicode object");
        Py_XDECREF(type_qualname);
        return NULL;
    }

    res = PyUnicode_FromFormat("%S.%S", type_qualname, descr->d_name);
    Py_DECREF(type_qualname);
    return res;
}

static PyObject *
descr_get_qualname(PyObject *self, void *Py_UNUSED(ignored))
{
    PyDescrObject *descr = (PyDescrObject *)self;
    PyObject *qualname;
    Py_BEGIN_CRITICAL_SECTION(self);
    if (descr->d_qualname == NULL) {
        PyObject *new_qualname = calculate_qualname(descr);
        if (new_qualname != NULL) {
            Py_XSETREF(descr->d_qualname, new_qualname);
        }
    }
    qualname = Py_XNewRef(descr->d_qualname);
    Py_END_CRITICAL_SECTION();
    return qualname;
}

static PyObject *
descr_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyDescrObject *descr = (PyDescrObject *)self;
    return Py_BuildValue("N(OO)", _PyEval_GetBuiltin(&_Py_ID(getattr)),
                         PyDescr_TYPE(descr), PyDescr_NAME(descr));
}

static PyMethodDef descr_methods[] = {
    {"__reduce__", descr_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyMemberDef descr_members[] = {
    {"__objclass__", _Py_T_OBJECT, offsetof(PyDescrObject, d_type), Py_READONLY},
    {"__name__", _Py_T_OBJECT, offsetof(PyDescrObject, d_name), Py_READONLY},
    {0}
};

static PyGetSetDef method_getset[] = {
    {"__doc__", method_get_doc},
    {"__qualname__", descr_get_qualname},
    {"__text_signature__", method_get_text_signature},
    {0}
};

static PyObject *
member_get_doc(PyObject *_descr, void *closure)
{
    PyMemberDescrObject *descr = (PyMemberDescrObject *)_descr;
    if (descr->d_member->doc == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(descr->d_member->doc);
}

static PyGetSetDef member_getset[] = {
    {"__doc__", member_get_doc},
    {"__qualname__", descr_get_qualname},
    {0}
};

static PyObject *
getset_get_doc(PyObject *self, void *closure)
{
    PyGetSetDescrObject *descr = (PyGetSetDescrObject *)self;
    if (descr->d_getset->doc == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(descr->d_getset->doc);
}

static PyGetSetDef getset_getset[] = {
    {"__doc__", getset_get_doc},
    {"__qualname__", descr_get_qualname},
    {0}
};

static PyObject *
wrapperdescr_get_doc(PyObject *self, void *closure)
{
    PyWrapperDescrObject *descr = (PyWrapperDescrObject *)self;
    return _PyType_GetDocFromInternalDoc(descr->d_base->name, descr->d_base->doc);
}

static PyObject *
wrapperdescr_get_text_signature(PyObject *self, void *closure)
{
    PyWrapperDescrObject *descr = (PyWrapperDescrObject *)self;
    return _PyType_GetTextSignatureFromInternalDoc(descr->d_base->name,
                                                   descr->d_base->doc, 0);
}

static PyGetSetDef wrapperdescr_getset[] = {
    {"__doc__", wrapperdescr_get_doc},
    {"__qualname__", descr_get_qualname},
    {"__text_signature__", wrapperdescr_get_text_signature},
    {0}
};

static int
descr_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyDescrObject *descr = (PyDescrObject *)self;
    Py_VISIT(descr->d_type);
    _Py_VISIT_ATOMIC(descr->d_name);
    _Py_VISIT_ATOMIC(descr->d_qualname);
    return 0;
}

PyTypeObject PyMethodDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "method_descriptor",
    sizeof(PyMethodDescrObject),
    0,
    descr_dealloc,                              /* tp_dealloc */
    offsetof(PyMethodDescrObject, vectorcall),  /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    method_repr,                                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    PyVectorcall_Call,                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_HAVE_VECTORCALL |
    Py_TPFLAGS_METHOD_DESCRIPTOR,               /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    descr_methods,                              /* tp_methods */
    descr_members,                              /* tp_members */
    method_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    method_get,                                 /* tp_descr_get */
    0,                                          /* tp_descr_set */
    .tp_value_vectorcall_offset = offsetof(PyMethodDescrObject,
                                           d_value_vectorcall),
    .tp_value.descr_get = _Py_VALUE_SLOT_INIT(descr_get, method_get_value),
};

/* This is for METH_CLASS in C, not for "f = classmethod(f)" in Python! */
PyTypeObject PyClassMethodDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "classmethod_descriptor",
    sizeof(PyMethodDescrObject),
    0,
    descr_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    method_repr,                                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    _PyValue_TpCallThunk,                       /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    descr_members,                              /* tp_members */
    method_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    classmethod_get,                            /* tp_descr_get */
    0,                                          /* tp_descr_set */
    .tp_value.call = _Py_VALUE_SLOT_INIT(call, classmethoddescr_value_call),
};

PyTypeObject PyMemberDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "member_descriptor",
    sizeof(PyMemberDescrObject),
    0,
    descr_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    member_repr,                                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    descr_methods,                              /* tp_methods */
    descr_members,                              /* tp_members */
    member_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    member_get,                                 /* tp_descr_get */
    member_set,                                 /* tp_descr_set */
    .tp_value = {
        .descr_get = _Py_VALUE_SLOT_INIT(descr_get, member_get_value),
        .descr_set = _Py_VALUE_SLOT_INIT(descr_set, member_set_value),
    },
};

PyTypeObject PyGetSetDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "getset_descriptor",
    sizeof(PyGetSetDescrObject),
    0,
    descr_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    getset_repr,                                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    descr_members,                              /* tp_members */
    getset_getset,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    getset_get,                                 /* tp_descr_get */
    getset_set,                                 /* tp_descr_set */
    .tp_value = {
        .descr_get = _Py_VALUE_SLOT_INIT(descr_get, getset_get_value),
        .descr_set = _Py_VALUE_SLOT_INIT(descr_set, getset_set_value),
    },
};

PyTypeObject PyWrapperDescr_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "wrapper_descriptor",
    sizeof(PyWrapperDescrObject),
    0,
    descr_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    wrapperdescr_repr,                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    _PyValue_TpCallThunk,                       /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_METHOD_DESCRIPTOR,               /* tp_flags */
    0,                                          /* tp_doc */
    descr_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    descr_methods,                              /* tp_methods */
    descr_members,                              /* tp_members */
    wrapperdescr_getset,                        /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    wrapperdescr_get,                           /* tp_descr_get */
    0,                                          /* tp_descr_set */
    .tp_value.call = _Py_VALUE_SLOT_INIT(call, wrapperdescr_value_call),
};

static PyDescrObject *
descr_new(PyTypeObject *descrtype, PyTypeObject *type, const char *name)
{
    PyDescrObject *descr;

    descr = (PyDescrObject *)PyType_GenericAlloc(descrtype, 0);
    if (descr != NULL) {
        _PyObject_SetDeferredRefcount((PyObject *)descr);
        descr->d_type = (PyTypeObject*)Py_XNewRef(type);
        descr->d_name = PyUnicode_InternFromString(name);
        if (descr->d_name == NULL) {
            Py_SETREF(descr, NULL);
        }
        else {
            descr->d_qualname = NULL;
        }
    }
    return descr;
}

PyObject *
PyDescr_NewMethod(PyTypeObject *type, PyMethodDef *method)
{
    /* Figure out correct vectorcall function to use */
    vectorcallfunc vectorcall;
    _Py_vectorcallvaluefunc value_vectorcall = NULL;
    switch (method->ml_flags & _Py_METH_CALL_FLAGS)
    {
        case METH_VARARGS:
            vectorcall = method_vectorcall_VARARGS;
            break;
        case METH_VARARGS | METH_KEYWORDS:
            vectorcall = method_vectorcall_VARARGS_KEYWORDS;
            break;
        case METH_FASTCALL:
            vectorcall = method_vectorcall_FASTCALL;
            break;
        case METH_FASTCALL | METH_KEYWORDS:
            vectorcall = method_vectorcall_FASTCALL_KEYWORDS;
            break;
        case METH_NOARGS:
            vectorcall = method_vectorcall_NOARGS;
            break;
        case METH_O:
            vectorcall = method_vectorcall_O;
            break;
        case METH_METHOD | METH_FASTCALL | METH_KEYWORDS:
            vectorcall = method_vectorcall_FASTCALL_KEYWORDS_METHOD;
            break;
        case METH_VALUE:
            value_vectorcall = method_value_vectorcall_descr;
            vectorcall = _PyValue_VectorcallThunk;
            break;
        case METH_VALUE | METH_KEYWORDS:
            value_vectorcall = method_value_vectorcall_descr_KEYWORDS;
            vectorcall = _PyValue_VectorcallThunk;
            break;
        case METH_VALUE | METH_METHOD | METH_KEYWORDS:
            value_vectorcall = method_value_vectorcall_descr_METHOD;
            vectorcall = _PyValue_VectorcallThunk;
            break;
        case METH_VALUE | METH_NOARGS:
            value_vectorcall = method_value_vectorcall_descr_NOARGS;
            vectorcall = _PyValue_VectorcallThunk;
            break;
        case METH_VALUE | METH_O:
            value_vectorcall = method_value_vectorcall_descr_O;
            vectorcall = _PyValue_VectorcallThunk;
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "%s() method: bad call flags", method->ml_name);
            return NULL;
    }

    PyMethodDescrObject *descr;

    descr = (PyMethodDescrObject *)descr_new(&PyMethodDescr_Type,
                                             type, method->ml_name);
    if (descr != NULL) {
        descr->d_method = method;
        descr->vectorcall = vectorcall;
        descr->d_value_vectorcall = (_Py_valuefunc)value_vectorcall;
    }
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)
{
    PyMethodDescrObject *descr;

    descr = (PyMethodDescrObject *)descr_new(&PyClassMethodDescr_Type,
                                             type, method->ml_name);
    if (descr != NULL)
        descr->d_method = method;
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewMember(PyTypeObject *type, PyMemberDef *member)
{
    PyMemberDescrObject *descr;

    if (member->flags & Py_RELATIVE_OFFSET) {
        PyErr_SetString(
            PyExc_SystemError,
            "PyDescr_NewMember used with Py_RELATIVE_OFFSET");
        return NULL;
    }
    descr = (PyMemberDescrObject *)descr_new(&PyMemberDescr_Type,
                                             type, member->name);
    if (descr != NULL)
        descr->d_member = member;
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewGetSet(PyTypeObject *type, PyGetSetDef *getset)
{
    PyGetSetDescrObject *descr;

    descr = (PyGetSetDescrObject *)descr_new(&PyGetSetDescr_Type,
                                             type, getset->name);
    if (descr != NULL) {
        descr->d_getset = getset;
        descr->d_value = getset->get == _PyGetSet_ValueGet ? getset->closure : NULL;
    }
    return (PyObject *)descr;
}

PyObject *
_PyDescr_NewWrapperPair(PyTypeObject *type, struct wrapperbase *base,
                        void *wrapped, void *wrapped_value)
{
    PyWrapperDescrObject *descr;

    descr = (PyWrapperDescrObject *)descr_new(&PyWrapperDescr_Type,
                                             type, base->name);
    if (descr != NULL) {
        descr->d_base = base;
        descr->d_wrapped = wrapped;
        descr->d_wrapped_value = wrapped_value;
    }
    return (PyObject *)descr;
}

PyObject *
PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *base, void *wrapped)
{
    return _PyDescr_NewWrapperPair(type, base, wrapped, NULL);
}

int
PyDescr_IsData(PyObject *ob)
{
    return Py_TYPE(ob)->tp_descr_set != NULL;
}

/* --- mappingproxy: read-only proxy for mappings --- */

/* This has no reason to be in this file except that adding new files is a
   bit of a pain */

typedef struct {
    PyObject_HEAD
    PyObject *mapping;
} mappingproxyobject;

static Py_ssize_t
mappingproxy_len(PyObject *self)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return _PyValue_Size(PyValue_FromObjectBorrow(pp->mapping));
}

static PyValue
mappingproxy_getitem_value(PyObject *self, PyValueBorrow key)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return _PyValue_GetItem(PyValue_FromObjectBorrow(pp->mapping), key);
}
_Py_VALUE_THUNK_SUBSCRIPT(mappingproxy_getitem, mappingproxy_getitem_value)

static PyMappingMethods mappingproxy_as_mapping = {
    mappingproxy_len,                           /* mp_length */
    mappingproxy_getitem,                       /* mp_subscript */
    0,                                          /* mp_ass_subscript */
};

static PyObject *
mappingproxy_or(PyObject *left, PyObject *right)
{
    if (PyObject_TypeCheck(left, &PyDictProxy_Type)) {
        left = ((mappingproxyobject*)left)->mapping;
    }
    if (PyObject_TypeCheck(right, &PyDictProxy_Type)) {
        right = ((mappingproxyobject*)right)->mapping;
    }
    return _PyValue_AsObjectResultAt(
        _PyValue_Or(PyValue_FromObjectBorrow(left),
                    PyValue_FromObjectBorrow(right)),
        BOX_SITE_NUMBER);
}

static PyObject *
mappingproxy_ior(PyObject *self, PyObject *Py_UNUSED(other))
{
    return PyErr_Format(PyExc_TypeError,
        "'|=' is not supported by %s; use '|' instead", Py_TYPE(self)->tp_name);
}

static PyNumberMethods mappingproxy_as_number = {
    .nb_or = mappingproxy_or,
    .nb_inplace_or = mappingproxy_ior,
};

static int
mappingproxy_contains(PyObject *self, PyObject *key)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    if (PyDict_CheckExact(pp->mapping))
        return PyDict_ContainsValue(pp->mapping, PyValue_FromObjectBorrow(key));
    else
        return _PyValue_Contains(PyValue_FromObjectBorrow(pp->mapping), PyValue_FromObjectBorrow(key));
}

static PySequenceMethods mappingproxy_as_sequence = {
    0,                                          /* sq_length */
    0,                                          /* sq_concat */
    0,                                          /* sq_repeat */
    0,                                          /* sq_item */
    0,                                          /* sq_slice */
    0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    mappingproxy_contains,                      /* sq_contains */
    0,                                          /* sq_inplace_concat */
    0,                                          /* sq_inplace_repeat */
};

static PyObject *
mappingproxy_get(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    /* newargs: mapping, key, default=None */
    PyObject *newargs[3];
    newargs[0] = pp->mapping;
    newargs[2] = Py_None;

    if (!_PyArg_UnpackStack(args, nargs, "get", 1, 2,
                            &newargs[1], &newargs[2]))
    {
        return NULL;
    }
    return PyObject_VectorcallMethod(&_Py_ID(get), newargs,
                                     3 | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                     NULL);
}

static PyObject *
mappingproxy_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(keys));
}

static PyObject *
mappingproxy_values(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(values));
}

static PyObject *
mappingproxy_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(items));
}

static PyObject *
mappingproxy_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(copy));
}

static PyObject *
mappingproxy_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return PyObject_CallMethodNoArgs(pp->mapping, &_Py_ID(__reversed__));
}

/* WARNING: mappingproxy methods must not give access
            to the underlying mapping */

static PyMethodDef mappingproxy_methods[] = {
    {"get",       _PyCFunction_CAST(mappingproxy_get), METH_FASTCALL,
     PyDoc_STR("get($self, key, default=None, /)\n--\n\n"
        "Return the value for key if key is in the mapping, else default.")},
    {"keys",      mappingproxy_keys,       METH_NOARGS,
     PyDoc_STR("D.keys() -> a set-like object providing a view on D's keys")},
    {"values",    mappingproxy_values,     METH_NOARGS,
     PyDoc_STR("D.values() -> an object providing a view on D's values")},
    {"items",     mappingproxy_items,      METH_NOARGS,
     PyDoc_STR("D.items() -> a set-like object providing a view on D's items")},
    {"copy",      mappingproxy_copy,       METH_NOARGS,
     PyDoc_STR("D.copy() -> a shallow copy of D")},
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("mappingproxy objects are generic over two types, signifying (respectively) the types of their keys and values")},
    {"__reversed__", mappingproxy_reversed, METH_NOARGS,
     PyDoc_STR("D.__reversed__() -> reverse iterator")},
    {0}
};

static void
mappingproxy_dealloc(PyObject *self)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    _PyObject_GC_UNTRACK(pp);
    Py_DECREF(pp->mapping);
    PyObject_GC_Del(pp);
}

static PyObject *
mappingproxy_getiter(PyObject *self)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return _PyValue_GetIter(PyValue_FromObjectBorrow(pp->mapping));
}

static Py_hash_t
mappingproxy_hash(PyObject *self)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return _PyValue_Hash(PyValue_FromObjectBorrow(pp->mapping));
}

static PyObject *
mappingproxy_str(PyObject *self)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return _PyValue_Str(PyValue_FromObjectBorrow(pp->mapping));
}

static PyObject *
mappingproxy_repr(PyObject *self)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    return PyUnicode_FromFormat("mappingproxy(%R)", pp->mapping);
}

static int
mappingproxy_traverse(PyObject *self, visitproc visit, void *arg)
{
    mappingproxyobject *pp = (mappingproxyobject *)self;
    Py_VISIT(pp->mapping);
    return 0;
}

static PyObject *
mappingproxy_richcompare(PyObject *self, PyObject *w, int op)
{
    if (op == Py_EQ || op == Py_NE) {
        mappingproxyobject *v = (mappingproxyobject *)self;
        // We have to guard the mutable `dict` instances, because it can
        // otherwise mutate the type's `__dict__` entries and cause crashes.
        // But, do not create copies on known types like `OrderedDict`
        // or immutable types like `frozendict`
        // for memory optimization. See gh-152405 for the details.
        if (
            PyDict_CheckExact(v->mapping) &&
            !(PyAnyDict_CheckExact(w) ||
                Py_TYPE(w) == &PyDictProxy_Type ||
                PyODict_CheckExact(w))
        ) {
            // So, instead we send a copy:
            PyObject *copy = PyDict_Copy(v->mapping);
            if (copy == NULL) {
                return NULL;
            }
            PyObject *res = _PyValue_AsObjectResultAt(
                _PyValue_RichCompare(PyValue_FromObjectBorrow(copy),
                                     PyValue_FromObjectBorrow(w), op),
                BOX_SITE_COMPARE);
            Py_DECREF(copy);
            return res;
        }
        // Otherwise we are free to share the mapping directly:
        return _PyValue_AsObjectResultAt(
            _PyValue_RichCompare(PyValue_FromObjectBorrow(v->mapping),
                                 PyValue_FromObjectBorrow(w), op),
            BOX_SITE_COMPARE);
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static int
mappingproxy_check_mapping(PyObject *mapping)
{
    if (!_PyValue_MappingCheck(PyValue_FromObjectBorrow(mapping))
        || PyList_Check(mapping)
        || PyTuple_Check(mapping)) {
        PyErr_Format(PyExc_TypeError,
                    "mappingproxy() argument must be a mapping, not %s",
                    Py_TYPE(mapping)->tp_name);
        return -1;
    }
    return 0;
}

PyObject *
PyDictProxy_New(PyObject *mapping)
{
    mappingproxyobject *pp;

    if (mappingproxy_check_mapping(mapping) == -1)
        return NULL;

    pp = PyObject_GC_New(mappingproxyobject, &PyDictProxy_Type);
    if (pp != NULL) {
        pp->mapping = Py_NewRef(mapping);
        _PyObject_GC_TRACK(pp);
    }
    return (PyObject *)pp;
}

/*[clinic input]
@classmethod
mappingproxy.__new__ as mappingproxy_new

    mapping: object

Read-only proxy of a mapping.
[clinic start generated code]*/

static PyValue
mappingproxy_new_impl(PyTypeObject *type, PyValueBorrow mapping)
/*[clinic end generated code: output=484d11cca2bd373c input=c156df096ef7590c]*/
{
    PyObject *mapping_o = _PyValue_AsObjectAt(mapping, BOX_SITE_PARSER);
    if (mapping_o == NULL) {
        return PyValue_NULL;
    }
    PyObject *proxy = PyDictProxy_New(mapping_o);
    Py_DECREF(mapping_o);
    return _PyValue_FromObjectResult(proxy);
}


/* --- Wrapper object for "slot" methods --- */

/* This has no reason to be in this file except that adding new files is a
   bit of a pain */

typedef struct {
    PyObject_HEAD
    PyWrapperDescrObject *descr;
    PyValue self;
} wrapperobject;

#define Wrapper_Check(v) Py_IS_TYPE(v, &_PyMethodWrapper_Type)

static void
wrapper_dealloc(PyObject *self)
{
    wrapperobject *wp = (wrapperobject *)self;
    PyObject_GC_UnTrack(wp);
    Py_XDECREF(wp->descr);
    PyValue_CLEAR(wp->self);
    PyObject_GC_Del(wp);
}

static PyObject *
wrapper_richcompare(PyObject *a, PyObject *b, int op)
{
    wrapperobject *wa, *wb;
    int eq;

    assert(a != NULL && b != NULL);

    /* both arguments should be wrapperobjects */
    if ((op != Py_EQ && op != Py_NE)
        || !Wrapper_Check(a) || !Wrapper_Check(b))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    wa = (wrapperobject *)a;
    wb = (wrapperobject *)b;
    eq = (wa->descr == wb->descr && PyValue_Is(wa->self, wb->self));
    if (eq == (op == Py_EQ)) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

static Py_hash_t
wrapper_hash(PyObject *self)
{
    wrapperobject *wp = (wrapperobject *)self;
    Py_hash_t x, y;
    x = _PyValue_IdentityHash(wp->self);
    y = Py_HashPointer(wp->descr);
    x = x ^ y;
    if (x == -1)
        x = -2;
    return x;
}

static PyObject *
wrapper_repr(PyObject *self)
{
    wrapperobject *wp = (wrapperobject *)self;
    PyObject *obj = _PyValue_AsObjectAt(wp->self, BOX_SITE_REPR);
    if (obj == NULL) {
        return NULL;
    }
    PyObject *res = PyUnicode_FromFormat(
        "<method-wrapper '%s' of %s object at %p>",
        wp->descr->d_base->name,
        PyValue_TYPE(wp->self)->tp_name,
        obj);
    Py_DECREF(obj);
    return res;
}

static PyObject *
wrapper_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    wrapperobject *wp = (wrapperobject *)self;
    PyObject *obj = _PyValue_AsObjectAt(wp->self, BOX_SITE_FIELD);
    if (obj == NULL) {
        return NULL;
    }
    PyObject *res = Py_BuildValue("N(OO)", _PyEval_GetBuiltin(&_Py_ID(getattr)),
                                  obj, PyDescr_NAME(wp->descr));
    Py_DECREF(obj);
    return res;
}

static PyMethodDef wrapper_methods[] = {
    {"__reduce__", wrapper_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyMemberDef wrapper_members[] = {
    {"__self__", _Py_T_VALUE, offsetof(wrapperobject, self), Py_READONLY},
    {0}
};

static PyObject *
wrapper_objclass(PyObject *wp, void *Py_UNUSED(ignored))
{
    PyObject *c = (PyObject *)PyDescr_TYPE(((wrapperobject *)wp)->descr);

    return Py_NewRef(c);
}

static PyObject *
wrapper_name(PyObject *wp, void *Py_UNUSED(ignored))
{
    const char *s = ((wrapperobject *)wp)->descr->d_base->name;

    return PyUnicode_FromString(s);
}

static PyObject *
wrapper_doc(PyObject *self, void *Py_UNUSED(ignored))
{
    wrapperobject *wp = (wrapperobject *)self;
    return _PyType_GetDocFromInternalDoc(wp->descr->d_base->name, wp->descr->d_base->doc);
}

static PyObject *
wrapper_text_signature(PyObject *self, void *Py_UNUSED(ignored))
{
    wrapperobject *wp = (wrapperobject *)self;
    return _PyType_GetTextSignatureFromInternalDoc(wp->descr->d_base->name,
                                                   wp->descr->d_base->doc, 0);
}

static PyObject *
wrapper_qualname(PyObject *self, void *Py_UNUSED(ignored))
{
    wrapperobject *wp = (wrapperobject *)self;
    return descr_get_qualname((PyObject *)wp->descr, NULL);
}

static PyGetSetDef wrapper_getsets[] = {
    {"__objclass__", wrapper_objclass},
    {"__name__", wrapper_name},
    {"__qualname__", wrapper_qualname},
    {"__doc__", wrapper_doc},
    {"__text_signature__", wrapper_text_signature},
    {0}
};

static PyValue
wrapper_value_call(PyThreadState *tstate, PyObject *callable,
                   const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    wrapperobject *wp = (wrapperobject *)callable;
    return wrapperdescr_value_raw_call(wp->descr, PyValue_Borrow(wp->self), args,
                                       PyVectorcall_NARGS(nargsf), kwnames);
}

static int
wrapper_traverse(PyObject *self, visitproc visit, void *arg)
{
    wrapperobject *wp = (wrapperobject *)self;
    Py_VISIT(wp->descr);
    _Py_VISIT_VALUE(wp->self);
    return 0;
}

_Py_VALUE_NOTIMPL_RICHCMP(wrapper_richcompare_value, wrapper_richcompare)

PyTypeObject _PyMethodWrapper_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "method-wrapper",                           /* tp_name */
    sizeof(wrapperobject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    wrapper_dealloc,                            /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    wrapper_repr,                               /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    wrapper_hash,                               /* tp_hash */
    _PyValue_TpCallThunk,                       /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    0,                                          /* tp_doc */
    wrapper_traverse,                           /* tp_traverse */
    0,                                          /* tp_clear */
    wrapper_richcompare,                        /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    wrapper_methods,                            /* tp_methods */
    wrapper_members,                            /* tp_members */
    wrapper_getsets,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    .tp_value = {
        .call = _Py_VALUE_SLOT_INIT(call, wrapper_value_call),
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, wrapper_richcompare_value),
    },
};

PyObject *
PyWrapper_New(PyObject *d, PyObject *self)
{
    return _PyWrapper_NewValue(d, PyValue_FromObjectBorrow(self));
}

PyObject *
(_PyWrapper_NewValue)(PyObject *d, PyValueBorrow self)
{
    wrapperobject *wp;
    PyWrapperDescrObject *descr;

    assert(PyObject_TypeCheck(d, &PyWrapperDescr_Type));
    descr = (PyWrapperDescrObject *)d;
    assert(PyType_IsSubtype(PyValue_TYPE(self), PyDescr_TYPE(descr)));

    wp = PyObject_GC_New(wrapperobject, &_PyMethodWrapper_Type);
    if (wp != NULL) {
        wp->descr = (PyWrapperDescrObject*)Py_NewRef(descr);
        wp->self = PyValue_DUP(self);
        _PyObject_GC_TRACK(wp);
    }
    return (PyObject *)wp;
}


/* A built-in 'property' type */

#define _propertyobject_CAST(op)    ((propertyobject *)(op))

/*
class property(object):

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        if doc is None and fget is not None and hasattr(fget, "__doc__"):
            doc = fget.__doc__
        self.__get = fget
        self.__set = fset
        self.__del = fdel
        try:
            self.__doc__ = doc
        except AttributeError:  # read-only or dict-less class
            pass
        self.__name = None

    def __set_name__(self, owner, name):
        self.__name = name

    @property
    def __name__(self):
        return self.__name if self.__name is not None else self.fget.__name__

    @__name__.setter
    def __name__(self, value):
        self.__name = value

    def __get__(self, inst, type=None):
        if inst is None:
            return self
        if self.__get is None:
            raise AttributeError("property has no getter")
        return self.__get(inst)

    def __set__(self, inst, value):
        if self.__set is None:
            raise AttributeError("property has no setter")
        return self.__set(inst, value)

    def __delete__(self, inst):
        if self.__del is None:
            raise AttributeError("property has no deleter")
        return self.__del(inst)

*/

static PyValue property_copy(PyObject *, PyValueBorrow, PyValueBorrow,
                             PyValueBorrow);

static PyValue
property_member_get(PyValueBorrow self, void *closure)
{
    propertyobject *prop = (propertyobject *)PyValue_AsKnownObjectBorrow(self);
    PyValue field = *(PyValue *)((char *)prop + (uintptr_t)closure);
    return PyValue_IsNull(field) ? PyValue_None : PyValue_DUP(field);
}

static int
property_member_readonly_set(PyValueBorrow Py_UNUSED(self),
                             PyValueBorrow Py_UNUSED(value),
                             void *Py_UNUSED(closure))
{
    PyErr_SetString(PyExc_AttributeError, "readonly attribute");
    return -1;
}

static int
property_doc_set(PyValueBorrow self, PyValueBorrow value, void *closure)
{
    propertyobject *prop = (propertyobject *)PyValue_AsKnownObjectBorrow(self);
    PyValue old = prop->prop_doc;
    prop->prop_doc = PyValue_XDUP(value);
    PyValue_XCLOSE(old);
    return 0;
}

PyDoc_STRVAR(getter_doc,
             "getter($self, object, /)\n"
             "--\n"
             "\n"
             "Descriptor to obtain a copy of the property with a different getter.");

static PyValue
property_getter(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
                PyObject *kwnames)
{
    assert(nargs == 1 && kwnames == NULL);
    return property_copy(PyValue_AsKnownObjectBorrow(self),
                         PyValue_Borrow(args[0]),
                         PyValue_Borrow(PyValue_NULL),
                         PyValue_Borrow(PyValue_NULL));
}


PyDoc_STRVAR(setter_doc,
             "setter($self, object, /)\n"
             "--\n"
             "\n"
             "Descriptor to obtain a copy of the property with a different setter.");

static PyValue
property_setter(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
                PyObject *kwnames)
{
    assert(nargs == 1 && kwnames == NULL);
    return property_copy(PyValue_AsKnownObjectBorrow(self),
                         PyValue_Borrow(PyValue_NULL),
                         PyValue_Borrow(args[0]),
                         PyValue_Borrow(PyValue_NULL));
}


PyDoc_STRVAR(deleter_doc,
             "deleter($self, object, /)\n"
             "--\n"
             "\n"
             "Descriptor to obtain a copy of the property with a different deleter.");

static PyValue
property_deleter(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
                 PyObject *kwnames)
{
    assert(nargs == 1 && kwnames == NULL);
    return property_copy(PyValue_AsKnownObjectBorrow(self),
                         PyValue_Borrow(PyValue_NULL),
                         PyValue_Borrow(PyValue_NULL),
                         PyValue_Borrow(args[0]));
}


/*[clinic input]
property.__set_name__

    owner: object
    name: object
    /

Method to set name of a property.
[clinic start generated code]*/

static PyValue
property___set_name___impl(propertyobject *self, PyValueBorrow owner,
                           PyValueBorrow name)
/*[clinic end generated code: output=b236024393248986 input=0211b5b7ace099c5]*/
{
    PyValue old = self->prop_name;
    self->prop_name = PyValue_DUP(name);
    PyValue_XCLOSE(old);

    return PyValue_None;
}

static PyMethodDef property_methods[] = {
    {"getter", _PyCFunction_CAST(property_getter), METH_VALUE | METH_O, getter_doc},
    {"setter", _PyCFunction_CAST(property_setter), METH_VALUE | METH_O, setter_doc},
    {"deleter", _PyCFunction_CAST(property_deleter), METH_VALUE | METH_O, deleter_doc},
    PROPERTY___SET_NAME___METHODDEF
    {0}
};


static void
property_dealloc(PyObject *self)
{
    propertyobject *gs = (propertyobject *)self;

    _PyObject_GC_UNTRACK(self);
    PyValue_XCLOSE(gs->prop_get);
    PyValue_XCLOSE(gs->prop_set);
    PyValue_XCLOSE(gs->prop_del);
    PyValue_XCLOSE(gs->prop_doc);
    PyValue_XCLOSE(gs->prop_name);
    Py_TYPE(self)->tp_free(self);
}

static int
property_name(propertyobject *prop, PyValue *name)
{
    if (!PyValue_IsNull(prop->prop_name)) {
        *name = PyValue_DUP(prop->prop_name);
        return 1;
    }
    if (PyValue_IsNull(prop->prop_get)) {
        *name = PyValue_NULL;
        return 0;
    }
    PyValue getter = PyValue_DUP(prop->prop_get);
    _PyStatusValue result = _PyValue_GetOptionalAttr(
        getter, &_Py_ID(__name__));
    PyValue_CLOSE(getter);
    *name = result.value;
    return result.status;
}

static void
property_missing_error(propertyobject *prop, PyValueBorrow obj,
                       const char *action)
{
    PyValue name = PyValue_NULL;
    int name_status = property_name(prop, &name);
    if (name_status < 0) {
        return;
    }
    PyObject *name_repr = name_status ? _PyValue_Repr(name) : NULL;
    PyValue_XCLOSE(name);
    if (name_status && name_repr == NULL) {
        return;
    }
    PyObject *qualname = PyValue_IsNull(obj) ? NULL :
        PyType_GetQualName(PyValue_TYPE(obj));
    if (!PyValue_IsNull(obj) && qualname == NULL) {
        Py_XDECREF(name_repr);
        return;
    }
    if (name_repr != NULL && qualname != NULL) {
        PyErr_Format(PyExc_AttributeError, "property %U of %R object has no %s",
                     name_repr, qualname, action);
    }
    else if (qualname != NULL) {
        PyErr_Format(PyExc_AttributeError, "property of %R object has no %s",
                     qualname, action);
    }
    else {
        PyErr_Format(PyExc_AttributeError, "property has no %s", action);
    }
    Py_XDECREF(name_repr);
    Py_XDECREF(qualname);
}

static PyValue
property_descr_get_value(PyObject *self, PyValueBorrow obj, PyObject *type)
{
    propertyobject *prop = (propertyobject *)self;
    if (PyValue_IsNull(obj) || PyValue_IsNone(obj)) {
        return PyValue_FromObjectSteal(Py_NewRef(self));
    }
    PyValue getter = PyValue_IsNull(prop->prop_get) ? PyValue_NULL :
        PyValue_DUP(prop->prop_get);
    if (PyValue_IsNull(getter)) {
        property_missing_error(prop, obj, "getter");
        return PyValue_NULL;
    }
    PyValue args[1] = {PyValue_DUP(obj)};
    PyValue result = _PyValue_VectorcallTstate(_PyThreadState_GET(), getter,
                                               args, 1, NULL);
    PyValue_CLOSE(args[0]);
    PyValue_CLOSE(getter);
    return result;
}

static int
property_descr_set_value(PyObject *self, PyValueBorrow obj,
                         PyValueBorrow value)
{
    propertyobject *prop = (propertyobject *)self;
    int deleting = PyValue_IsNull(value);
    PyValue stored_func = deleting ? prop->prop_del : prop->prop_set;
    PyValue func = PyValue_IsNull(stored_func) ? PyValue_NULL :
        PyValue_DUP(stored_func);
    if (PyValue_IsNull(func)) {
        property_missing_error(prop, obj, deleting ? "deleter" : "setter");
        return -1;
    }
    PyValue args[2] = {PyValue_IsNull(obj) ? PyValue_NULL : PyValue_DUP(obj),
                       PyValue_NULL};
    Py_ssize_t nargs = 1;
    if (!deleting) {
        args[1] = PyValue_DUP(value);
        nargs = 2;
        if (PyValue_IsObject(func)) {
            EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API,
                                           PyValue_AsKnownObjectBorrow(func));
        }
    }
    PyValue result = _PyValue_VectorcallTstate(_PyThreadState_GET(), func,
                                               args, nargs, NULL);
    PyValue_CLOSE(args[0]);
    PyValue_XCLOSE(args[1]);
    PyValue_CLOSE(func);
    if (PyValue_IsNull(result)) {
        return -1;
    }
    PyValue_CLOSE(result);
    return 0;
}

Py_VALUE_ENTRY static PyObject *
property_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    PyValue value = obj == NULL ? PyValue_NULL :
        PyValue_FromObjectSteal(Py_NewRef(obj));
    PyValue result = property_descr_get_value(self, PyValue_Borrow(value),
                                              type);
    PyValue_XCLOSE(value);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_ATTRIBUTE);
}

Py_VALUE_ENTRY static int
property_descr_set(PyObject *self, PyObject *obj, PyObject *value)
{
    PyValue obj_value = obj == NULL ? PyValue_NULL :
        PyValue_FromObjectSteal(Py_NewRef(obj));
    PyValue field_value = value == NULL ? PyValue_NULL :
        PyValue_FromObjectSteal(Py_NewRef(value));
    int result = property_descr_set_value(self, PyValue_Borrow(obj_value),
                                          PyValue_Borrow(field_value));
    PyValue_XCLOSE(obj_value);
    PyValue_XCLOSE(field_value);
    return result;
}

static PyValue
property_copy(PyObject *old, PyValueBorrow get, PyValueBorrow set,
              PyValueBorrow del)
{
    propertyobject *pold = (propertyobject *)old;
    /* Keep all source fields alive across the subclass constructor callback. */
    PyValue old_get = PyValue_IsNull(pold->prop_get) ? PyValue_NULL :
        PyValue_DUP(pold->prop_get);
    PyValue old_set = PyValue_IsNull(pold->prop_set) ? PyValue_NULL :
        PyValue_DUP(pold->prop_set);
    PyValue old_del = PyValue_IsNull(pold->prop_del) ? PyValue_NULL :
        PyValue_DUP(pold->prop_del);
    PyValue old_doc = PyValue_IsNull(pold->prop_doc) ? PyValue_NULL :
        PyValue_DUP(pold->prop_doc);
    PyValue old_name = PyValue_IsNull(pold->prop_name) ? PyValue_NULL :
        PyValue_DUP(pold->prop_name);
    int getter_doc = pold->getter_doc;
    PyObject *type_obj = PyObject_Type(old);
    if (type_obj == NULL) {
        goto error;
    }
    PyValue type = PyValue_FromObjectSteal(type_obj);
    PyValue args[4];
    PyValue fields[3] = {old_get, old_set, old_del};
    for (int i = 0; i < 3; i++) {
        PyValueBorrow requested = i == 0 ? get : (i == 1 ? set : del);
        if (!PyValue_IsNull(requested) && !PyValue_IsNone(requested)) {
            args[i] = PyValue_DUP(requested);
        }
        else if (!PyValue_IsNull(fields[i])) {
            args[i] = PyValue_DUP(fields[i]);
        }
        else {
            args[i] = PyValue_None;
        }
    }
    if (getter_doc && !PyValue_IsNone(args[0])) {
        args[3] = PyValue_None;
    }
    else if (!PyValue_IsNull(old_doc)) {
        args[3] = PyValue_DUP(old_doc);
    }
    else {
        args[3] = PyValue_None;
    }
    PyValue result = _PyValue_VectorcallTstate(_PyThreadState_GET(), type,
                                               args, 4, NULL);
    for (int i = 0; i < 4; i++) {
        PyValue_XCLOSE(args[i]);
    }
    PyValue_CLOSE(type);
    if (!PyValue_IsNull(result) && PyValue_IsObject(result) &&
        PyObject_TypeCheck(PyValue_AsKnownObjectBorrow(result),
                           &PyProperty_Type))
    {
        propertyobject *new_prop = (propertyobject *)
            PyValue_AsKnownObjectBorrow(result);
        PyValue new_name = PyValue_IsNull(old_name) ? PyValue_NULL :
            PyValue_DUP(old_name);
        PyValue_XCLOSE(new_prop->prop_name);
        new_prop->prop_name = new_name;
    }
    PyValue_XCLOSE(old_get);
    PyValue_XCLOSE(old_set);
    PyValue_XCLOSE(old_del);
    PyValue_XCLOSE(old_doc);
    PyValue_XCLOSE(old_name);
    return result;
error:
    PyValue_XCLOSE(old_get);
    PyValue_XCLOSE(old_set);
    PyValue_XCLOSE(old_del);
    PyValue_XCLOSE(old_doc);
    PyValue_XCLOSE(old_name);
    return PyValue_NULL;
}

/* A property field stores a language value; None and omitted arguments both
   use the null-field sentinel. */
static PyValue
property_field_value(PyValueBorrow arg)
{
    if (PyValue_IsNull(arg) || PyValue_IsNone(arg)) {
        return PyValue_NULL;
    }
    return PyValue_DUP(arg);
}

/*[clinic input]
property.__init__ as property_init

    fget: object(c_default="NULL") = None
        function to be used for getting an attribute value
    fset: object(c_default="NULL") = None
        function to be used for setting an attribute value
    fdel: object(c_default="NULL") = None
        function to be used for del'ing an attribute
    doc: object(c_default="NULL") = None
        docstring

Property attribute.

Typical use is to define a managed attribute x:

class C(object):
    def getx(self): return self._x
    def setx(self, value): self._x = value
    def delx(self): del self._x
    x = property(getx, setx, delx, "I'm the 'x' property.")

Decorators make defining new properties or modifying existing ones easy:

class C(object):
    @property
    def x(self):
        "I am the 'x' property."
        return self._x
    @x.setter
    def x(self, value):
        self._x = value
    @x.deleter
    def x(self):
        del self._x
[clinic start generated code]*/

static int
property_init_impl(propertyobject *self, PyValueBorrow fget,
                   PyValueBorrow fset, PyValueBorrow fdel, PyValueBorrow doc)
/*[clinic end generated code: output=602fb7cadd4bb99b input=dfb5dbbffc6932d5]*/
{
    PyValue get = property_field_value(fget);
    PyValue set = property_field_value(fset);
    PyValue del = property_field_value(fdel);
    PyValue getter_for_doc = PyValue_IsNull(get) ? PyValue_NULL :
        PyValue_DUP(get);
    PyValue old;
    old = self->prop_get; self->prop_get = get; PyValue_XCLOSE(old);
    old = self->prop_set; self->prop_set = set; PyValue_XCLOSE(old);
    old = self->prop_del; self->prop_del = del; PyValue_XCLOSE(old);
    old = self->prop_doc; self->prop_doc = PyValue_NULL; PyValue_XCLOSE(old);
    old = self->prop_name; self->prop_name = PyValue_NULL; PyValue_XCLOSE(old);

    self->getter_doc = 0;
    PyValue prop_doc = PyValue_NULL;

    if (!PyValue_IsNull(doc) && !PyValue_IsNone(doc)) {
        prop_doc = PyValue_DUP(doc);
    }
    /* if no docstring given and the getter has one, use that one */
    else if (!PyValue_IsNull(getter_for_doc)) {
        _PyStatusValue attr = _PyValue_GetOptionalAttr(getter_for_doc,
                                                       &_Py_ID(__doc__));
        if (attr.status < 0) {
            PyValue_CLOSE(getter_for_doc);
            return -1;
        }
        prop_doc = attr.value;
        if (PyValue_IsNone(prop_doc)) {
            PyValue_CLOSE(prop_doc);
            prop_doc = PyValue_NULL;
        }
        if (!PyValue_IsNull(prop_doc)) {
            self->getter_doc = 1;
        }
    }
    PyValue_XCLOSE(getter_for_doc);

    if (Py_IS_TYPE(self, &PyProperty_Type)) {
        self->prop_doc = prop_doc;
    } else {
        /* If this is a property subclass, put __doc__ in the dict
           or designated slot of the subclass instance instead, otherwise
           it gets shadowed by __doc__ in the class's dict. */

        if (PyValue_IsNull(prop_doc)) {
            prop_doc = PyValue_None;
        }
        int err = _PyValue_SetAttr(PyValue_FromObjectBorrow((PyObject *)self),
                                   &_Py_ID(__doc__), prop_doc);
        PyValue_XCLOSE(prop_doc);
        if (err < 0) {
            assert(PyErr_Occurred());
            if (!self->getter_doc &&
                PyErr_ExceptionMatches(PyExc_AttributeError))
            {
                PyErr_Clear();
                // https://github.com/python/cpython/issues/98963#issuecomment-1574413319
                // Python silently dropped this doc assignment through 3.11.
                // We preserve that behavior for backwards compatibility.
                //
                // If we ever want to deprecate this behavior, only raise a
                // warning or error when proc_doc is not None so that
                // property without a specific doc= still works.
                return 0;
            } else {
                return -1;
            }
        }
    }

    return 0;
}

static PyValue
property_get__name__(PyValueBorrow op, void *Py_UNUSED(ignored))
{
    propertyobject *prop = _propertyobject_CAST(
        PyValue_AsKnownObjectBorrow(op));
    PyValue name = PyValue_NULL;
    int status = property_name(prop, &name);
    if (status < 0) {
        return PyValue_NULL;
    }
    if (status == 0) {
        PyErr_SetString(PyExc_AttributeError,
                        "'property' object has no attribute '__name__'");
        return PyValue_NULL;
    }
    return name;
}

static int
property_set__name__(PyValueBorrow op, PyValueBorrow value,
                     void *Py_UNUSED(ignored))
{
    propertyobject *prop = _propertyobject_CAST(
        PyValue_AsKnownObjectBorrow(op));
    PyValue old = prop->prop_name;
    prop->prop_name = PyValue_XDUP(value);
    PyValue_XCLOSE(old);
    return 0;
}

static PyValue
property_get___isabstractmethod__(PyValueBorrow op, void *closure)
{
    propertyobject *prop = _propertyobject_CAST(
        PyValue_AsKnownObjectBorrow(op));
    PyValue fields[3] = {
        PyValue_IsNull(prop->prop_get) ? PyValue_NULL : PyValue_DUP(prop->prop_get),
        PyValue_IsNull(prop->prop_set) ? PyValue_NULL : PyValue_DUP(prop->prop_set),
        PyValue_IsNull(prop->prop_del) ? PyValue_NULL : PyValue_DUP(prop->prop_del),
    };
    for (int i = 0; i < 3; i++) {
        if (PyValue_IsNull(fields[i]) || PyValue_IsNone(fields[i])) {
            PyValue_XCLOSE(fields[i]);
            fields[i] = PyValue_NULL;
            continue;
        }
        _PyStatusValue attr = _PyValue_GetOptionalAttr(
            fields[i], &_Py_ID(__isabstractmethod__));
        if (attr.status < 0) {
            goto error;
        }
        if (attr.status == 1) {
            int truth = _PyValue_IsTrue(attr.value);
            PyValue_CLOSE(attr.value);
            if (truth < 0) {
                goto error;
            }
            if (truth) {
                PyValue_CLOSE(fields[i]);
                fields[i] = PyValue_NULL;
                for (int j = i + 1; j < 3; j++) {
                    PyValue_XCLOSE(fields[j]);
                    fields[j] = PyValue_NULL;
                }
                return PyValue_True;
            }
        }
        PyValue_CLOSE(fields[i]);
        fields[i] = PyValue_NULL;
    }
    return PyValue_False;
error:
    for (int i = 0; i < 3; i++) {
        PyValue_XCLOSE(fields[i]);
    }
    return PyValue_NULL;
}

static PyGetSetDef property_getsetlist[] = {
    _Py_VALUE_GETSET("fget", property_member_get,
                     property_member_readonly_set, NULL,
                     (void *)offsetof(propertyobject, prop_get)),
    _Py_VALUE_GETSET("fset", property_member_get,
                     property_member_readonly_set, NULL,
                     (void *)offsetof(propertyobject, prop_set)),
    _Py_VALUE_GETSET("fdel", property_member_get,
                     property_member_readonly_set, NULL,
                     (void *)offsetof(propertyobject, prop_del)),
    _Py_VALUE_GETSET("__doc__", property_member_get, property_doc_set, NULL,
                     (void *)offsetof(propertyobject, prop_doc)),
    _Py_VALUE_GETSET("__name__", property_get__name__,
                     property_set__name__, NULL, NULL),
    _Py_VALUE_GETTER("__isabstractmethod__",
                     property_get___isabstractmethod__, NULL, NULL),
    {NULL} /* Sentinel */
};

static int
property_traverse(PyObject *self, visitproc visit, void *arg)
{
    propertyobject *pp = (propertyobject *)self;
    _Py_VISIT_VALUE(pp->prop_get);
    _Py_VISIT_VALUE(pp->prop_set);
    _Py_VISIT_VALUE(pp->prop_del);
    _Py_VISIT_VALUE(pp->prop_doc);
    _Py_VISIT_VALUE(pp->prop_name);
    return 0;
}

static int
property_clear(PyObject *self)
{
    propertyobject *pp = (propertyobject *)self;
    PyValue_CLEAR(pp->prop_get);
    PyValue_CLEAR(pp->prop_set);
    PyValue_CLEAR(pp->prop_del);
    PyValue_CLEAR(pp->prop_doc);
    PyValue_CLEAR(pp->prop_name);
    return 0;
}


PyTypeObject PyDictProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "mappingproxy",                             /* tp_name */
    sizeof(mappingproxyobject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    mappingproxy_dealloc,                       /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    mappingproxy_repr,                          /* tp_repr */
    &mappingproxy_as_number,                    /* tp_as_number */
    &mappingproxy_as_sequence,                  /* tp_as_sequence */
    &mappingproxy_as_mapping,                   /* tp_as_mapping */
    mappingproxy_hash,                          /* tp_hash */
    0,                                          /* tp_call */
    mappingproxy_str,                           /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_MAPPING,                     /* tp_flags */
    mappingproxy_new__doc__,                    /* tp_doc */
    mappingproxy_traverse,                      /* tp_traverse */
    0,                                          /* tp_clear */
    mappingproxy_richcompare,                   /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    mappingproxy_getiter,                       /* tp_iter */
    0,                                          /* tp_iternext */
    mappingproxy_methods,                       /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    mappingproxy_new,                           /* tp_new */
    .tp_value = {
        .subscript = _Py_VALUE_SLOT_INIT(subscript,
                                         mappingproxy_getitem_value),
    },
};

PyTypeObject PyProperty_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "property",                                 /* tp_name */
    sizeof(propertyobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    property_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    property_init__doc__,                       /* tp_doc */
    property_traverse,                          /* tp_traverse */
    property_clear,                             /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    property_methods,                           /* tp_methods */
    0,                                          /* tp_members */
    property_getsetlist,                        /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    property_descr_get,                         /* tp_descr_get */
    property_descr_set,                         /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    property_init,                              /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    .tp_value = {
        .descr_get = _Py_VALUE_SLOT_INIT(descr_get,
                                         property_descr_get_value),
        .descr_set = _Py_VALUE_SLOT_INIT(descr_set,
                                         property_descr_set_value),
    },
};
