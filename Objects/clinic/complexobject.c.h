/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(complex_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.");

#define COMPLEX_CONJUGATE_METHODDEF    \
    {"conjugate", _PyCFunction_CAST(complex_conjugate), METH_VALUE, complex_conjugate__doc__},

static PyValue
complex_conjugate_impl(PyComplexObject *self);

static PyValue
complex_conjugate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyComplex_Type, "conjugate", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = complex_conjugate_impl((PyComplexObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(complex___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define COMPLEX___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", _PyCFunction_CAST(complex___getnewargs__), METH_VALUE, complex___getnewargs____doc__},

static PyValue
complex___getnewargs___impl(PyComplexObject *self);

static PyValue
complex___getnewargs__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyComplex_Type, "__getnewargs__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = complex___getnewargs___impl((PyComplexObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(complex___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Convert to a string according to format_spec.");

#define COMPLEX___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(complex___format__), METH_VALUE, complex___format____doc__},

static PyValue
complex___format___impl(PyComplexObject *self, PyObject *format_spec);

static PyValue
complex___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;

    if (!_PyArg_CheckArityValueMethod(&PyComplex_Type, "__format__", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format_spec = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = complex___format___impl((PyComplexObject *)PyValue_AsKnownObjectBorrow(self), format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(complex___complex____doc__,
"__complex__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type complex.");

#define COMPLEX___COMPLEX___METHODDEF    \
    {"__complex__", _PyCFunction_CAST(complex___complex__), METH_VALUE, complex___complex____doc__},

static PyValue
complex___complex___impl(PyComplexObject *self);

static PyValue
complex___complex__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyComplex_Type, "__complex__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = complex___complex___impl((PyComplexObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(complex_new__doc__,
"complex(real=0, imag=0)\n"
"--\n"
"\n"
"Create a complex number from a string or numbers.\n"
"\n"
"If a string is given, parse it as a complex number.\n"
"If a single number is given, convert it to a complex number.\n"
"If the \'real\' or \'imag\' arguments are given, create a complex number\n"
"with the specified real and imaginary components.");

static PyValue
complex_new_impl(PyTypeObject *type, PyValueBorrow r, PyValueBorrow i);

static PyValue
complex_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(real)), _PyValue_STATIC_IMMORTAL(&_Py_ID(imag)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"real", "imag", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "complex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow r = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow i = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        r = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        i = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = complex_new_impl(type, r, i);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(complex_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = complex_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(complex_from_number__doc__,
"from_number($type, number, /)\n"
"--\n"
"\n"
"Convert number to a complex floating-point number.");

#define COMPLEX_FROM_NUMBER_METHODDEF    \
    {"from_number", _PyCFunction_CAST(complex_from_number), METH_VALUE|METH_CLASS, complex_from_number__doc__},

static PyValue
complex_from_number_impl(PyTypeObject *type, PyValueBorrow number);

static PyValue
complex_from_number(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow number;

    if (!_PyArg_CheckArityValueMethod(&PyComplex_Type, "from_number", nargs, kwnames, 1)) {
        goto exit;
    }
    number = PyValue_Borrow(args[0]);
    return_value = complex_from_number_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), number);

exit:
    return return_value;
}
/*[clinic end generated code: output=af82d863503c1b14 input=a9049054013a1b77]*/
