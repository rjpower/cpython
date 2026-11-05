/* Test helpers for the value vectorcall (M3a,
 * pyjit/design/proposals/value-api-m3.md, sections 1.1-1.6 and 3).
 * The tests are in Lib/test/test_value_call.py.
 *
 * METH_VALUE functions that report what they were given, a METH_VALUE
 * method of int, accessors for the vectorcall fields of an instance, C
 * entry points into the value call API, and a spec type whose instances
 * hold a value vectorcall. */

#include "parts.h"

#include "pycore_function.h"      // _PyFunction_Vectorcall()
#include "pycore_methodobject.h"  // _PyCFunction_NewValue()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_value.h"         // PyValue
#include "pycore_value_call.h"    // METH_VALUE

#include <stddef.h>               // offsetof()
#include <string.h>               // memcpy(), strlen()


/* METH_VALUE functions */

/* (nargs, positional values, keyword dict or None) */
static PyValue
vc_args(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
        PyObject *kwnames)
{
    PyObject *positional = _PyTuple_FromValueArray(args, nargs);
    if (positional == NULL) {
        return PyValue_NULL;
    }
    PyObject *kw;
    if (kwnames == NULL) {
        kw = Py_NewRef(Py_None);
    }
    else {
        kw = _PyValue_StackAsDict(args + nargs, kwnames);
        if (kw == NULL) {
            Py_DECREF(positional);
            return PyValue_NULL;
        }
    }
    PyObject *result = Py_BuildValue("(nNN)", nargs, positional, kw);
    return PyValue_FromObjectSteal(result);
}

static PyValue
vc_noargs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
          PyObject *kwnames)
{
    if (kwnames != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "vc_noargs() takes no keyword arguments");
        return PyValue_NULL;
    }
    if (nargs != 0) {
        PyErr_Format(PyExc_TypeError,
                     "%s() takes no arguments (%zd given)",
                     "vc_noargs", nargs);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(Py_NewRef(Py_None));
}

static PyValue
vc_one(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
       PyObject *kwnames)
{
    if (kwnames != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "vc_one() takes no keyword arguments");
        return PyValue_NULL;
    }
    if (nargs != 1) {
        PyErr_Format(PyExc_TypeError,
                     "%s() takes exactly one argument (%zd given)",
                     "vc_one", nargs);
        return PyValue_NULL;
    }
    return PyValue_DUP(PyValue_Borrow(args[0]));
}

static PyValue
vc_self(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
        PyObject *kwnames)
{
    return PyValue_DUP(self);
}

/* vc_call_first(callback, **kwargs): calls callback(), then returns the
 * keyword values, read from the window after the call */
static PyValue
vc_call_first(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
              PyObject *kwnames)
{
    if (nargs != 1) {
        PyErr_SetString(PyExc_TypeError,
                        "vc_call_first() takes one positional argument");
        return PyValue_NULL;
    }
    PyObject *empty = PyTuple_New(0);
    if (empty == NULL) {
        return PyValue_NULL;
    }
    PyValue r = _PyValue_Call(_PyThreadState_GET(), PyValue_Borrow(args[0]),
                              empty, NULL);
    Py_DECREF(empty);
    if (PyValue_IsNull(r)) {
        return PyValue_NULL;
    }
    PyValue_CLOSE(r);
    Py_ssize_t nkw = kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames);
    return PyValue_FromObjectSteal(
        _PyTuple_FromValueArray(args + nargs, nkw));
}

static PyValue
vc_method(PyValueBorrow self, PyTypeObject *defining_class,
          const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    return PyValue_FromObjectSteal(Py_NewRef((PyObject *)defining_class));
}

/* ((self, *args), self_was_inline), as the method of int */
static PyValue
vc_int_method(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
              PyObject *kwnames)
{
    if (kwnames != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "int method takes no keyword arguments");
        return PyValue_NULL;
    }
    int inline_self = PyValue_IsInline(self);
    PyObject *items = PyTuple_New(nargs + 1);
    if (items == NULL) {
        return PyValue_NULL;
    }
    PyObject *first = PyValue_AsObject(self);
    if (first == NULL) {
        Py_DECREF(items);
        return PyValue_NULL;
    }
    PyTuple_SET_ITEM(items, 0, first);
    for (Py_ssize_t i = 0; i < nargs; i++) {
        PyObject *item = PyValue_AsObject(PyValue_Borrow(args[i]));
        if (item == NULL) {
            Py_DECREF(items);
            return PyValue_NULL;
        }
        PyTuple_SET_ITEM(items, i + 1, item);
    }
    PyObject *result = Py_BuildValue("(NO)", items,
                                     inline_self ? Py_True : Py_False);
    return PyValue_FromObjectSteal(result);
}

#define VALUE_CAST(f) ((PyCFunction)(void (*)(void))(f))

static PyMethodDef vc_method_def = {
    "vc_method", VALUE_CAST(vc_method), METH_VALUE | METH_METHOD, NULL,
};

static PyMethodDef int_method_def = {
    "vc_int_method", VALUE_CAST(vc_int_method), METH_VALUE, NULL,
};


/* make_int_method(name): the method descriptor of int */

static PyObject *
make_int_method(PyObject *Py_UNUSED(module), PyObject *name)
{
    const char *text = PyUnicode_AsUTF8(name);
    if (text == NULL) {
        return NULL;
    }
    /* The definition must outlive the descriptor: leaked, in a test. */
    PyMethodDef *def = PyMem_RawMalloc(sizeof(*def));
    if (def == NULL) {
        return PyErr_NoMemory();
    }
    size_t size = strlen(text) + 1;
    char *copy = PyMem_RawMalloc(size);
    if (copy == NULL) {
        PyMem_RawFree(def);
        return PyErr_NoMemory();
    }
    memcpy(copy, text, size);
    *def = int_method_def;
    def->ml_name = copy;
    return PyDescr_NewMethod(&PyLong_Type, def);
}


/* bad_flags(flags): the exceptions of PyCFunction_NewEx() and
 * PyDescr_NewMethod() for METH_VALUE | flags */

static PyObject *
bad_flags(PyObject *Py_UNUSED(module), PyObject *arg)
{
    int flags = PyLong_AsInt(arg);
    if (flags == -1 && PyErr_Occurred()) {
        return NULL;
    }
    static PyMethodDef def;
    def = (PyMethodDef){"bad_flags_function", VALUE_CAST(vc_one),
                        METH_VALUE | flags, NULL};
    PyObject *results[2] = {NULL, NULL};

    PyObject *func = PyCFunction_NewEx(&def, NULL, NULL);
    if (func != NULL) {
        results[0] = Py_NewRef(Py_None);
        Py_DECREF(func);
    }
    else {
        results[0] = PyErr_GetRaisedException();
    }
    PyObject *descr = PyDescr_NewMethod(&PyLong_Type, &def);
    if (descr != NULL) {
        results[1] = Py_NewRef(Py_None);
        Py_DECREF(descr);
    }
    else {
        results[1] = PyErr_GetRaisedException();
    }
    return Py_BuildValue("(NN)", results[0], results[1]);
}


/* Accessors */

static PyObject *
value_vectorcall_is_set(PyObject *Py_UNUSED(module), PyObject *obj)
{
    Py_ssize_t off = Py_TYPE(obj)->tp_value_vectorcall_offset;
    if (off <= 0) {
        Py_RETURN_FALSE;
    }
    _Py_vectorcallvaluefunc f;
    memcpy(&f, (char *)obj + off, sizeof(f));
    return PyBool_FromLong(f != NULL);
}

static PyObject *
object_vectorcall_is_thunk(PyObject *Py_UNUSED(module), PyObject *obj)
{
    Py_ssize_t off = Py_TYPE(obj)->tp_vectorcall_offset;
    if (off <= 0) {
        Py_RETURN_FALSE;
    }
    vectorcallfunc f;
    memcpy(&f, (char *)obj + off, sizeof(f));
    return PyBool_FromLong(f == _PyValue_VectorcallThunk
                           || f == _PyFunction_Vectorcall);
}

/* The object form of a value result */
static PyObject *
value_result(PyValue result)
{
    if (PyValue_IsNull(result)) {
        return NULL;
    }
    PyObject *obj = PyValue_AsObject(PyValue_Borrow(result));
    PyValue_CLOSE(result);
    return obj;
}

static PyObject *
call_value(PyObject *Py_UNUSED(module), PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "call_value() needs a callable");
        return NULL;
    }
    PyValueBorrow callable = PyTuple_GET_VALUE(args, 0);
    PyObject *rest = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (rest == NULL) {
        return NULL;
    }
    PyValue result = _PyValue_Call(_PyThreadState_GET(), callable, rest,
                                   kwargs);
    Py_DECREF(rest);
    return value_result(result);
}

/* call_dict(callable, args, kwargs): _PyValue_Call() with the caller's
 * tuple and dict, which the callee can reach */
static PyObject *
call_dict(PyObject *Py_UNUSED(module), PyObject *const *args,
          Py_ssize_t nargs)
{
    if (nargs != 3 || !PyTuple_Check(args[1]) || !PyDict_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "call_dict() takes a callable, a tuple and a dict");
        return NULL;
    }
    PyValue result = _PyValue_Call(_PyThreadState_GET(),
                                   PyValue_FromObjectBorrow(args[0]),
                                   args[1], args[2]);
    return value_result(result);
}

/* (tp_vectorcall_offset, tp_value_vectorcall_offset) of a type */
static PyObject *
vectorcall_offsets(PyObject *Py_UNUSED(module), PyObject *type)
{
    if (!PyType_Check(type)) {
        PyErr_SetString(PyExc_TypeError, "expected a type");
        return NULL;
    }
    PyTypeObject *tp = (PyTypeObject *)type;
    return Py_BuildValue("(nn)", tp->tp_vectorcall_offset,
                         tp->tp_value_vectorcall_offset);
}

static PyObject *
call_one_arg(PyObject *Py_UNUSED(module), PyObject *const *args,
             Py_ssize_t nargs)
{
    if (nargs != 2) {
        PyErr_SetString(PyExc_TypeError,
                        "call_one_arg() takes a callable and an argument");
        return NULL;
    }
    PyValue result = _PyValue_CallOneArg(_PyThreadState_GET(),
                                         PyValue_FromObjectBorrow(args[0]),
                                         PyValue_FromObjectBorrow(args[1]));
    return value_result(result);
}


/* The spec types: instances hold a value vectorcall that returns a marker,
 * and an object vectorcall that is the thunk. */

typedef struct {
    PyObject_HEAD
    _Py_vectorcallvaluefunc value_vectorcall;
    vectorcallfunc vectorcall;
} VCallObject;

static PyValue
vcall_value_vectorcall(PyThreadState *tstate, PyObject *callable,
                       const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *result = Py_BuildValue("(sn)", "value-marker", nargs);
    return PyValue_FromObjectSteal(result);
}

static PyObject *
vcall_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    VCallObject *self = (VCallObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->value_vectorcall = vcall_value_vectorcall;
    self->vectorcall = _PyValue_VectorcallThunk;
    return (PyObject *)self;
}

static void
vcall_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyMemberDef vcall_members[] = {
    {"__vectorcalloffset__", Py_T_PYSSIZET,
     offsetof(VCallObject, vectorcall), Py_READONLY},
    {"__valuevectorcalloffset__", Py_T_PYSSIZET,
     offsetof(VCallObject, value_vectorcall), Py_READONLY},
    {NULL},
};

static PyType_Slot vcall_slots[] = {
    {Py_tp_new, vcall_new},
    {Py_tp_dealloc, vcall_dealloc},
    {Py_tp_call, PyVectorcall_Call},
    {Py_tp_members, vcall_members},
    {0, NULL},
};

static PyType_Spec vcall_spec = {
    .name = "_testinternalcapi.VCall",
    .basicsize = sizeof(VCallObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL
              | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = vcall_slots,
};

static PyType_Spec vcall_base_spec = {
    .name = "_testinternalcapi.VCallBase",
    .basicsize = sizeof(VCallObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_VECTORCALL
              | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_BASETYPE),
    .slots = vcall_slots,
};


static PyMethodDef methods[] = {
    {"vc_args", VALUE_CAST(vc_args), METH_VALUE, NULL},
    {"vc_noargs", VALUE_CAST(vc_noargs), METH_VALUE, NULL},
    {"vc_one", VALUE_CAST(vc_one), METH_VALUE, NULL},
    {"vc_self", VALUE_CAST(vc_self), METH_VALUE, NULL},
    {"vc_call_first", VALUE_CAST(vc_call_first), METH_VALUE, NULL},
    {"make_int_method", make_int_method, METH_O},
    {"bad_flags", bad_flags, METH_O},
    {"value_vectorcall_is_set", value_vectorcall_is_set, METH_O},
    {"object_vectorcall_is_thunk", object_vectorcall_is_thunk, METH_O},
    {"call_value", _PyCFunction_CAST(call_value),
     METH_VARARGS | METH_KEYWORDS},
    {"call_one_arg", _PyCFunction_CAST(call_one_arg), METH_FASTCALL},
    {"call_dict", _PyCFunction_CAST(call_dict), METH_FASTCALL},
    {"vectorcall_offsets", vectorcall_offsets, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_ValueCall(PyObject *module)
{
    if (PyModule_AddFunctions(module, methods) < 0) {
        return -1;
    }
    PyObject *vcall = PyType_FromSpec(&vcall_spec);
    if (vcall == NULL) {
        return -1;
    }
    if (PyModule_Add(module, "VCall", vcall) < 0) {
        return -1;
    }
    PyObject *base = PyType_FromSpec(&vcall_base_spec);
    if (base == NULL) {
        return -1;
    }
    if (PyModule_Add(module, "VCallBase", Py_NewRef(base)) < 0) {
        Py_DECREF(base);
        return -1;
    }
    /* vc_method receives the defining class VCallBase. */
    PyObject *func = _PyCFunction_NewValue(
        &vc_method_def, PyValue_FromObjectBorrow(module), NULL,
        (PyTypeObject *)base);
    Py_DECREF(base);
    return PyModule_Add(module, "vc_method", func);
}
