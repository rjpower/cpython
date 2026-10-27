/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(complex_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.");

#define COMPLEX_CONJUGATE_METHODDEF    \
    {"conjugate", (PyCFunction)complex_conjugate, METH_NOARGS, complex_conjugate__doc__},

static PyObject *
complex_conjugate_impl(PyComplexObject *self);

static PyObject *
complex_conjugate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex_conjugate_impl((PyComplexObject *)self);
}

PyDoc_STRVAR(complex___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define COMPLEX___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)complex___getnewargs__, METH_NOARGS, complex___getnewargs____doc__},

static PyObject *
complex___getnewargs___impl(PyComplexObject *self);

static PyObject *
complex___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex___getnewargs___impl((PyComplexObject *)self);
}

PyDoc_STRVAR(complex___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Convert to a string according to format_spec.");

#define COMPLEX___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(complex___format__), METH_O, complex___format____doc__},

static PyObject *
complex___format___impl(PyComplexObject *self, PyObject *format_spec);

static PyValue
complex___format__(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("__format__", "argument", "str", _arg);
            goto exit;
        }
        format_spec = _arg;
    }
    _return_value = complex___format___impl((PyComplexObject *)self, format_spec);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(complex___complex____doc__,
"__complex__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type complex.");

#define COMPLEX___COMPLEX___METHODDEF    \
    {"__complex__", (PyCFunction)complex___complex__, METH_NOARGS, complex___complex____doc__},

static PyObject *
complex___complex___impl(PyComplexObject *self);

static PyObject *
complex___complex__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return complex___complex___impl((PyComplexObject *)self);
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

static PyObject *
complex_new_impl(PyTypeObject *type, PyObject *r, PyObject *i);

static PyObject *
complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(real), &_Py_ID(imag), },
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
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *r = NULL;
    PyObject *i = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        r = PyValue_Object(fastargs[0]);
        if (r == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    i = PyValue_Object(fastargs[1]);
    if (i == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = complex_new_impl(type, r, i);

exit:
    return return_value;
}

PyDoc_STRVAR(complex_from_number__doc__,
"from_number($type, number, /)\n"
"--\n"
"\n"
"Convert number to a complex floating-point number.");

#define COMPLEX_FROM_NUMBER_METHODDEF    \
    {"from_number", _PyCFunction_CAST(complex_from_number), METH_O|METH_CLASS, complex_from_number__doc__},

static PyObject *
complex_from_number_impl(PyTypeObject *type, PyObject *number);

static PyValue
complex_from_number(PyObject *type, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *number;
    PyObject *_return_value;

    number = PyValue_Object(arg);
    if (number == NULL) {
        goto exit;
    }
    _return_value = complex_from_number_impl((PyTypeObject *)type, number);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=0ab233a4d2821103 input=a9049054013a1b77]*/
