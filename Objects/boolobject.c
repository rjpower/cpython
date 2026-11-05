/* Boolean type, a subtype of int */

// bool's value slots call int's value functions, never an int thunk (D12).
#define Py_VALUE_MARK_NUMBERS 1

/* Defer line of the value-API waves (value-api-v2a0.md, section 1.5): the
   callers of the protocol marks in this unit are converted by W2's caller
   batches, which remove this line. */
#define Py_VALUE_DEFER_MARKS 1

#include "Python.h"
#include "pycore_long.h"          // FALSE_TAG TRUE_TAG
#include "pycore_long_value.h"    // _PyLong_ValueAnd()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _Py_FatalRefcountError()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_value_slots.h"   // _Py_VALUE_THUNK_BINARY()

#include <stddef.h>

/* We define bool_repr to return "False" or "True" */

static PyObject *
bool_repr(PyObject *self)
{
    return self == Py_True ? &_Py_ID(True) : &_Py_ID(False);
}

/* Function to return a bool from a C long */

PyObject *PyBool_FromLong(long ok)
{
    return ok ? Py_True : Py_False;
}

/* We define bool_new to always return either Py_True or Py_False */

static PyObject *
bool_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *x = Py_False;
    long ok;

    if (!_PyArg_NoKeywords("bool", kwds))
        return NULL;
    if (!PyArg_UnpackTuple(args, "bool", 0, 1, &x))
        return NULL;
    ok = PyObject_IsTrue(x);
    if (ok < 0)
        return NULL;
    return PyBool_FromLong(ok);
}

static PyObject *
bool_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    long ok = 0;
    if (!_PyArg_NoKwnames("bool", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("bool", nargs, 0, 1)) {
        return NULL;
    }

    assert(PyType_Check(type));
    if (nargs) {
        ok = PyObject_IsTrue(args[0]);
        if (ok < 0) {
            return NULL;
        }
    }
    return PyBool_FromLong(ok);
}

/* Arithmetic operations redefined to return bool if both args are bool. */

static PyValue
_PyBool_ValueInvert(PyValueBorrow v)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "Bitwise inversion '~' on bool is deprecated and will be removed in "
                     "Python 3.16. This returns the bitwise inversion of the underlying int "
                     "object and is usually not what you expect from negating "
                     "a bool. Use the 'not' operator for boolean negation or "
                     "~int(x) if you really want the bitwise inversion of the "
                     "underlying int.",
                     1) < 0) {
        return PyValue_NULL;
    }
    return _PyLong_ValueInvert(v);
}

static PyValue
_PyBool_ValueAnd(PyValueBorrow a, PyValueBorrow b)
{
    if (!PyValue_BoolCheck(a) || !PyValue_BoolCheck(b)) {
        return _PyLong_ValueAnd(a, b);
    }
    return PyValue_FromBool(PyValue_IsTrue(a) && PyValue_IsTrue(b));
}

static PyValue
_PyBool_ValueOr(PyValueBorrow a, PyValueBorrow b)
{
    if (!PyValue_BoolCheck(a) || !PyValue_BoolCheck(b)) {
        return _PyLong_ValueOr(a, b);
    }
    return PyValue_FromBool(PyValue_IsTrue(a) || PyValue_IsTrue(b));
}

static PyValue
_PyBool_ValueXor(PyValueBorrow a, PyValueBorrow b)
{
    if (!PyValue_BoolCheck(a) || !PyValue_BoolCheck(b)) {
        return _PyLong_ValueXor(a, b);
    }
    return PyValue_FromBool(PyValue_IsTrue(a) != PyValue_IsTrue(b));
}

_Py_VALUE_THUNK_UNARY(bool_invert, _PyBool_ValueInvert)
_Py_VALUE_THUNK_BINARY(bool_and, _PyBool_ValueAnd)
_Py_VALUE_THUNK_BINARY(bool_or, _PyBool_ValueOr)
_Py_VALUE_THUNK_BINARY(bool_xor, _PyBool_ValueXor)

/* Doc string */

PyDoc_STRVAR(bool_doc,
"bool(object=False, /)\n\
--\n\
\n\
Returns True when the argument is true, False otherwise.\n\
The builtins True and False are the only two instances of the class bool.\n\
The class bool is a subclass of the class int, and cannot be subclassed.");

/* Arithmetic methods -- only so we can override &, |, ^. */

static PyNumberMethods bool_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    0,                          /* nb_bool */
    bool_invert,                /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    bool_and,                   /* nb_and */
    bool_xor,                   /* nb_xor */
    bool_or,                    /* nb_or */
    0,                          /* nb_int */
    0,                          /* nb_reserved */
    0,                          /* nb_float */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    0,                          /* nb_index */
};

static void
bool_dealloc(PyObject *boolean)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref Booleans out of existence. Instead,
     * since bools are immortal, re-set the reference count.
     */
    _Py_SetImmortal(boolean);
}

/* The type object for bool.  Note that this cannot be subclassed! */

PyTypeObject PyBool_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bool",
    offsetof(struct _longobject, long_value.ob_digit),  /* tp_basicsize */
    sizeof(digit),                              /* tp_itemsize */
    bool_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    bool_repr,                                  /* tp_repr */
    &bool_as_number,                            /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
    bool_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    &PyLong_Type,                               /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    bool_new,                                   /* tp_new */
    .tp_vectorcall = bool_vectorcall,
    .tp_value = {
        .nb_invert = _Py_VALUE_SLOT_INIT(nb_invert, _PyBool_ValueInvert),
        .nb_and = _Py_VALUE_SLOT_INIT(nb_and, _PyBool_ValueAnd),
        .nb_xor = _Py_VALUE_SLOT_INIT(nb_xor, _PyBool_ValueXor),
        .nb_or = _Py_VALUE_SLOT_INIT(nb_or, _PyBool_ValueOr),
    },
};

/* The objects representing bool values False and True */

struct _longobject _Py_FalseStruct = {
    PyObject_HEAD_INIT(&PyBool_Type)
    { .lv_tag = _PyLong_FALSE_TAG,
        { 0 }
    }
};

struct _longobject _Py_TrueStruct = {
    PyObject_HEAD_INIT(&PyBool_Type)
    { .lv_tag = _PyLong_TRUE_TAG,
        { 1 }
    }
};
