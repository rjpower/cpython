/* Class object implementation (dead now except for methods) */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_VectorcallTstate()
#include "pycore_classobject.h"    // _PyMethod_NewValue()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_freelist.h"
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_symtable.h"      // _Py_Mangle()
#include "pycore_value.h"         // _PyValue_IdentityHash()
#include "pycore_value_slots.h"  // _Py_VALUE_NOTIMPL_RICHCMP()
#include "pycore_value_api.h"
#include "pycore_value_call.h"    // _PyValue_VectorcallPrepend()
#include "pycore_value_ops.h"     // _PyValue_GetAttr()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()


#include "clinic/classobject.c.h"

#define _PyMethodObject_CAST(op) _Py_CAST(PyMethodObject*, (op))
#define TP_DESCR_GET(t) ((t)->tp_descr_get)

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class method "PyMethodObject *" "&PyMethod_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b16e47edf6107c23]*/


PyObject *
PyMethod_Function(PyObject *im)
{
    if (!PyMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyMethodObject *)im)->im_func;
}

Py_VALUE_ENTRY PyObject *
PyMethod_GetSelfRef(PyObject *im)
{
    if (!PyMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return _PyValue_THUNK_NEW(((PyMethodObject *)im)->im_self, BOX_SITE_FIELD);
}

PyAPI_FUNC(PyObject *)
(PyMethod_GET_FUNCTION)(PyObject *meth)
{
    return _PyMethod_GET_FUNCTION(meth);
}


static PyValue
method_value_vectorcall(PyThreadState *tstate, PyObject *method,
                        const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    assert(Py_IS_TYPE(method, &PyMethod_Type));

    PyMethodObject *im = (PyMethodObject *)method;
    return _PyValue_VectorcallPrepend(
        tstate, PyValue_FromObjectBorrow(im->im_func),
        PyValue_Borrow(im->im_self), args, nargsf, kwnames);
}


/* Method objects are used for bound instance methods returned by
   instancename.methodname. ClassName.methodname returns an ordinary
   function.
*/

PyObject *
(_PyMethod_NewValue)(PyObject *func, PyValueBorrow self)
{
    if (PyValue_IsNull(self)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyMethodObject *im = _Py_FREELIST_POP(PyMethodObject, pymethodobjects);
    if (im == NULL) {
        im = PyObject_GC_New(PyMethodObject, &PyMethod_Type);
        if (im == NULL) {
            return NULL;
        }
    }
    im->im_weakreflist = NULL;
    im->im_func = Py_NewRef(func);
    im->im_self = PyValue_DUP(self);
    im->value_vectorcall = method_value_vectorcall;
    im->vectorcall = _PyValue_VectorcallThunk;
    _PyObject_GC_TRACK(im);
    return (PyObject *)im;
}

PyObject *
PyMethod_New(PyObject *func, PyObject *self)
{
    if (self == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return _PyMethod_NewValue(func, PyValue_FromObjectBorrow(self));
}

/*[clinic input]
method.__reduce__
[clinic start generated code]*/

static PyValue
method___reduce___impl(PyMethodObject *self)
/*[clinic end generated code: output=c40c47424ce3ee62 input=143a0bf5e96de6e8]*/
{
    PyObject *funcself = _PyValue_AsObjectAt(self->im_self, BOX_SITE_FIELD);
    if (funcself == NULL) {
        return PyValue_NULL;
    }
    PyObject *func = PyMethod_GET_FUNCTION(self);
    PyValue funcname_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(func),
                                          &_Py_ID(__name__));
    PyObject *funcname = PyValue_IsNull(funcname_v) ? NULL
        : _PyValue_AsObjectStealAt(funcname_v, BOX_SITE_ATTRIBUTE);
    if (funcname == NULL) {
        Py_DECREF(funcself);
        return PyValue_NULL;
    }
    if (_Py_IsPrivateName(funcname)) {
        PyObject *classname = PyType_Check(funcself)
            ? PyType_GetName((PyTypeObject *)funcself)
            : PyType_GetName(Py_TYPE(funcself));
        if (classname == NULL) {
            Py_DECREF(funcname);
            Py_DECREF(funcself);
            return PyValue_NULL;
        }
        Py_SETREF(funcname, _Py_Mangle(classname, funcname));
        Py_DECREF(classname);
        if (funcname == NULL) {
            Py_DECREF(funcself);
            return PyValue_NULL;
        }
    }
    PyObject *res = Py_BuildValue(
            "N(ON)", _PyEval_GetBuiltin(&_Py_ID(getattr)), funcself, funcname);
    Py_DECREF(funcself);
    return _PyValue_FromObjectResult(res);
}

static PyMethodDef method_methods[] = {
    METHOD___REDUCE___METHODDEF
    {NULL, NULL}
};

/* Descriptors for PyMethod attributes */

/* im_func and im_self are stored in the PyMethod object */

#define MO_OFF(x) offsetof(PyMethodObject, x)

static PyMemberDef method_memberlist[] = {
    {"__func__", _Py_T_OBJECT, MO_OFF(im_func), Py_READONLY,
     "the function (or other callable) implementing a method"},
    {"__self__", _Py_T_VALUE, MO_OFF(im_self), Py_READONLY,
     "the instance to which a method is bound"},
    {NULL}      /* Sentinel */
};

/* Christian Tismer argued convincingly that method attributes should
   (nearly) always override function attributes.
   The one exception is __doc__; there's a default __doc__ which
   should only be used for the class, not for instances */

static PyObject *
method_get_doc(PyObject *self, void *context)
{
    PyMethodObject *im = _PyMethodObject_CAST(self);
    PyValue doc = _PyValue_GetAttr(PyValue_FromObjectBorrow(im->im_func),
                                   &_Py_ID(__doc__));
    return PyValue_IsNull(doc) ? NULL
        : _PyValue_AsObjectStealAt(doc, BOX_SITE_ATTRIBUTE);
}

static PyGetSetDef method_getset[] = {
    {"__doc__", method_get_doc, NULL, NULL},
    {0}
};

static PyObject *
method_getattro(PyObject *obj, PyObject *name)
{
    PyMethodObject *im = (PyMethodObject *)obj;
    PyTypeObject *tp = Py_TYPE(obj);
    PyObject *descr = NULL;

    {
        if (!_PyType_IsReady(tp)) {
            if (PyType_Ready(tp) < 0)
                return NULL;
        }
        descr = _PyType_LookupRef(tp, name);
    }

    if (descr != NULL) {
        descrgetfunc f = TP_DESCR_GET(Py_TYPE(descr));
        if (f != NULL) {
            PyObject *res = f(descr, obj, (PyObject *)Py_TYPE(obj));
            Py_DECREF(descr);
            return res;
        }
        else {
            return descr;
        }
    }

    PyValue attr = _PyValue_GetAttr(PyValue_FromObjectBorrow(im->im_func),
                                    name);
    return PyValue_IsNull(attr) ? NULL
        : _PyValue_AsObjectStealAt(attr, BOX_SITE_ATTRIBUTE);
}

/*[clinic input]
@classmethod
method.__new__ as method_new
    function: object
    instance: object
    /

Create a bound instance method object.
[clinic start generated code]*/

static PyValue
method_new_impl(PyTypeObject *type, PyValueBorrow function,
                PyValueBorrow instance)
/*[clinic end generated code: output=d91b7fb41709da60 input=4e32facc3c3108ae]*/
{
    /* An inline int or float is never callable. */
    if (!(PyValue_IsObject(function) && PyCallable_Check(PyValue_AsKnownObjectBorrow(function)))) {
        PyErr_SetString(PyExc_TypeError,
                        "first argument must be callable");
        return PyValue_NULL;
    }
    if (PyValue_IsNull(instance) || PyValue_IsNone(instance)) {
        PyErr_SetString(PyExc_TypeError,
            "instance must not be None");
        return PyValue_NULL;
    }

    return _PyValue_FromObjectResult(
        _PyMethod_NewValue(PyValue_AsKnownObjectBorrow(function), instance));
}

static void
method_dealloc(PyObject *self)
{
    PyMethodObject *im = _PyMethodObject_CAST(self);
    _PyObject_GC_UNTRACK(im);
    FT_CLEAR_WEAKREFS(self, im->im_weakreflist);
    Py_DECREF(im->im_func);
    PyValue_CLEAR(im->im_self);
    assert(Py_IS_TYPE(self, &PyMethod_Type));
    _Py_FREELIST_FREE(pymethodobjects, (PyObject *)im, PyObject_GC_Del);
}

static PyObject *
method_richcompare(PyObject *self, PyObject *other, int op)
{
    PyMethodObject *a, *b;
    PyObject *res;
    int eq;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyMethod_Check(self) ||
        !PyMethod_Check(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    a = (PyMethodObject *)self;
    b = (PyMethodObject *)other;
    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(a->im_func), PyValue_FromObjectBorrow(b->im_func), Py_EQ);
    if (eq == 1) {
        eq = PyValue_Is(a->im_self, b->im_self);
    }
    else if (eq < 0)
        return NULL;
    if (op == Py_EQ)
        res = eq ? Py_True : Py_False;
    else
        res = eq ? Py_False : Py_True;
    return Py_NewRef(res);
}

static PyObject *
method_repr(PyObject *op)
{
    PyMethodObject *a = _PyMethodObject_CAST(op);
    PyObject *func = a->im_func;
    PyObject *funcname, *result;
    const char *defname = "?";

    PyObject *self = _PyValue_AsObjectAt(a->im_self, BOX_SITE_REPR);
    if (self == NULL) {
        return NULL;
    }

    if (PyObject_GetOptionalAttr(func, &_Py_ID(__qualname__), &funcname) < 0 ||
        (funcname == NULL &&
         PyObject_GetOptionalAttr(func, &_Py_ID(__name__), &funcname) < 0))
    {
        Py_DECREF(self);
        return NULL;
    }

    if (funcname != NULL && !PyUnicode_Check(funcname)) {
        Py_SETREF(funcname, NULL);
    }

    /* XXX Shouldn't use repr()/%R here! */
    result = PyUnicode_FromFormat("<bound method %V of %R>",
                                  funcname, defname, self);

    Py_XDECREF(funcname);
    Py_DECREF(self);
    return result;
}

static Py_hash_t
method_hash(PyObject *self)
{
    PyMethodObject *a = _PyMethodObject_CAST(self);
    Py_hash_t x = _PyValue_IdentityHash(a->im_self);
    Py_hash_t y = _PyValue_Hash(PyValue_FromObjectBorrow(a->im_func));
    if (y == -1) {
        return -1;
    }

    x = x ^ y;
    if (x == -1) {
        x = -2;
    }
    return x;
}

static int
method_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyMethodObject *im = _PyMethodObject_CAST(self);
    Py_VISIT(im->im_func);
    _Py_VISIT_VALUE(im->im_self);
    return 0;
}

static PyObject *
method_descr_get(PyObject *meth, PyObject *obj, PyObject *cls)
{
    Py_INCREF(meth);
    return meth;
}

_Py_VALUE_NOTIMPL_RICHCMP(method_richcompare_value, method_richcompare)

PyTypeObject PyMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "method",
    .tp_basicsize = sizeof(PyMethodObject),
    .tp_dealloc = method_dealloc,
    .tp_vectorcall_offset = offsetof(PyMethodObject, vectorcall),
    .tp_repr = method_repr,
    .tp_hash = method_hash,
    .tp_call = PyVectorcall_Call,
    .tp_getattro = method_getattro,
    .tp_setattro = PyObject_GenericSetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
                Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_doc = method_new__doc__,
    .tp_traverse = method_traverse,
    .tp_richcompare = method_richcompare,
    .tp_weaklistoffset = offsetof(PyMethodObject, im_weakreflist),
    .tp_methods = method_methods,
    .tp_members = method_memberlist,
    .tp_getset = method_getset,
    .tp_descr_get = method_descr_get,
    .tp_new = method_new,
    .tp_value_vectorcall_offset = offsetof(PyMethodObject, value_vectorcall),
    .tp_value = {
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, method_richcompare_value),
    },
};

/* ------------------------------------------------------------------------
 * instance method
 */

/*[clinic input]
class instancemethod "PyInstanceMethodObject *" "&PyInstanceMethod_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=28c9762a9016f4d2]*/

PyObject *
PyInstanceMethod_New(PyObject *func) {
    PyInstanceMethodObject *method;
    method = PyObject_GC_New(PyInstanceMethodObject,
                             &PyInstanceMethod_Type);
    if (method == NULL) return NULL;
    method->func = Py_NewRef(func);
    _PyObject_GC_TRACK(method);
    return (PyObject *)method;
}

PyObject *
PyInstanceMethod_Function(PyObject *im)
{
    if (!PyInstanceMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyInstanceMethod_GET_FUNCTION(im);
}

#define IMO_OFF(x) offsetof(PyInstanceMethodObject, x)

static PyMemberDef instancemethod_memberlist[] = {
    {"__func__", _Py_T_OBJECT, IMO_OFF(func), Py_READONLY,
     "the function (or other callable) implementing a method"},
    {NULL}      /* Sentinel */
};

static PyObject *
instancemethod_get_doc(PyObject *self, void *context)
{
    PyValue doc = _PyValue_GetAttr(
        PyValue_FromObjectBorrow(PyInstanceMethod_GET_FUNCTION(self)),
        &_Py_ID(__doc__));
    return PyValue_IsNull(doc) ? NULL
        : _PyValue_AsObjectStealAt(doc, BOX_SITE_ATTRIBUTE);
}

static PyGetSetDef instancemethod_getset[] = {
    {"__doc__", instancemethod_get_doc, NULL, NULL},
    {0}
};

static PyObject *
instancemethod_getattro(PyObject *self, PyObject *name)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *descr = NULL;

    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0)
            return NULL;
    }
    descr = _PyType_LookupRef(tp, name);

    if (descr != NULL) {
        descrgetfunc f = TP_DESCR_GET(Py_TYPE(descr));
        if (f != NULL) {
            PyObject *res = f(descr, self, (PyObject *)Py_TYPE(self));
            Py_DECREF(descr);
            return res;
        }
        else {
            return descr;
        }
    }

    PyValue attr = _PyValue_GetAttr(
        PyValue_FromObjectBorrow(PyInstanceMethod_GET_FUNCTION(self)), name);
    return PyValue_IsNull(attr) ? NULL
        : _PyValue_AsObjectStealAt(attr, BOX_SITE_ATTRIBUTE);
}

static void
instancemethod_dealloc(PyObject *self) {
    _PyObject_GC_UNTRACK(self);
    Py_DECREF(PyInstanceMethod_GET_FUNCTION(self));
    PyObject_GC_Del(self);
}

static int
instancemethod_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(PyInstanceMethod_GET_FUNCTION(self));
    return 0;
}

static PyObject *
instancemethod_call(PyObject *self, PyObject *arg, PyObject *kw)
{
    return PyObject_Call(PyInstanceMethod_GET_FUNCTION(self), arg, kw);
}

static PyObject *
instancemethod_descr_get(PyObject *descr, PyObject *obj, PyObject *type) {
    PyObject *func = PyInstanceMethod_GET_FUNCTION(descr);
    if (obj == NULL) {
        return Py_NewRef(func);
    }
    else
        return PyMethod_New(func, obj);
}

static PyObject *
instancemethod_richcompare(PyObject *self, PyObject *other, int op)
{
    PyInstanceMethodObject *a, *b;
    PyObject *res;
    int eq;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyInstanceMethod_Check(self) ||
        !PyInstanceMethod_Check(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    a = (PyInstanceMethodObject *)self;
    b = (PyInstanceMethodObject *)other;
    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(a->func), PyValue_FromObjectBorrow(b->func), Py_EQ);
    if (eq < 0)
        return NULL;
    if (op == Py_EQ)
        res = eq ? Py_True : Py_False;
    else
        res = eq ? Py_False : Py_True;
    return Py_NewRef(res);
}

static PyObject *
instancemethod_repr(PyObject *self)
{
    PyObject *func = PyInstanceMethod_Function(self);
    PyObject *funcname, *result;
    const char *defname = "?";

    if (func == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (PyObject_GetOptionalAttr(func, &_Py_ID(__name__), &funcname) < 0) {
        return NULL;
    }
    if (funcname != NULL && !PyUnicode_Check(funcname)) {
        Py_SETREF(funcname, NULL);
    }

    result = PyUnicode_FromFormat("<instancemethod %V at %p>",
                                  funcname, defname, self);

    Py_XDECREF(funcname);
    return result;
}

/*[clinic input]
@classmethod
instancemethod.__new__ as instancemethod_new
    function: object
    /

Bind a function to a class.
[clinic start generated code]*/

static PyValue
instancemethod_new_impl(PyTypeObject *type, PyValueBorrow function)
/*[clinic end generated code: output=59b513aebac4756e input=cfc54e8b973664a8]*/
{
    /* An inline int or float is never callable. */
    if (!(PyValue_IsObject(function) && PyCallable_Check(PyValue_AsKnownObjectBorrow(function)))) {
        PyErr_SetString(PyExc_TypeError,
                        "first argument must be callable");
        return PyValue_NULL;
    }

    return _PyValue_FromObjectResult(
        PyInstanceMethod_New(PyValue_AsKnownObjectBorrow(function)));
}

PyTypeObject PyInstanceMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "instancemethod",
    .tp_basicsize = sizeof(PyInstanceMethodObject),
    .tp_dealloc = instancemethod_dealloc,
    .tp_repr = instancemethod_repr,
    .tp_call = instancemethod_call,
    .tp_getattro = instancemethod_getattro,
    .tp_setattro = PyObject_GenericSetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_doc = instancemethod_new__doc__,
    .tp_traverse = instancemethod_traverse,
    .tp_richcompare = instancemethod_richcompare,
    .tp_members = instancemethod_memberlist,
    .tp_getset = instancemethod_getset,
    .tp_descr_get = instancemethod_descr_get,
    .tp_new = instancemethod_new,
};
