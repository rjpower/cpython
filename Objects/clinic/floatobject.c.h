/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(float_is_integer__doc__,
"is_integer($self, /)\n"
"--\n"
"\n"
"Return True if the float is an integer.");

#define FLOAT_IS_INTEGER_METHODDEF    \
    {"is_integer", (PyCFunction)float_is_integer, METH_NOARGS, float_is_integer__doc__},

static PyObject *
float_is_integer_impl(PyObject *self);

static PyObject *
float_is_integer(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_is_integer_impl(self);
}

PyDoc_STRVAR(float___trunc____doc__,
"__trunc__($self, /)\n"
"--\n"
"\n"
"Return the Integral closest to x between 0 and x.");

#define FLOAT___TRUNC___METHODDEF    \
    {"__trunc__", (PyCFunction)float___trunc__, METH_NOARGS, float___trunc____doc__},

static PyObject *
float___trunc___impl(PyObject *self);

static PyObject *
float___trunc__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float___trunc___impl(self);
}

PyDoc_STRVAR(float___floor____doc__,
"__floor__($self, /)\n"
"--\n"
"\n"
"Return the floor as an Integral.");

#define FLOAT___FLOOR___METHODDEF    \
    {"__floor__", (PyCFunction)float___floor__, METH_NOARGS, float___floor____doc__},

static PyObject *
float___floor___impl(PyObject *self);

static PyObject *
float___floor__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float___floor___impl(self);
}

PyDoc_STRVAR(float___ceil____doc__,
"__ceil__($self, /)\n"
"--\n"
"\n"
"Return the ceiling as an Integral.");

#define FLOAT___CEIL___METHODDEF    \
    {"__ceil__", (PyCFunction)float___ceil__, METH_NOARGS, float___ceil____doc__},

static PyObject *
float___ceil___impl(PyObject *self);

static PyObject *
float___ceil__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float___ceil___impl(self);
}

PyDoc_STRVAR(float___round____doc__,
"__round__($self, ndigits=None, /)\n"
"--\n"
"\n"
"Return the Integral closest to x, rounding half toward even.\n"
"\n"
"When an argument is passed, work like built-in round(x, ndigits).");

#define FLOAT___ROUND___METHODDEF    \
    {"__round__", _PyCFunction_CAST(float___round__), METH_FASTCALL, float___round____doc__},

static PyValue
float___round___impl(PyObject *self, PyValue o_ndigits);

static PyValue
float___round__(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyValue o_ndigits = PyValue_FromObject(Py_None);
    PyValue _return_value;

    if (!_PyArg_CheckPositional("__round__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    o_ndigits = args[0];
skip_optional:
    _return_value = float___round___impl(self, o_ndigits);
    return_value = _return_value;

exit:
    return return_value;
}

PyDoc_STRVAR(float_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return self, the complex conjugate of any float.");

#define FLOAT_CONJUGATE_METHODDEF    \
    {"conjugate", (PyCFunction)float_conjugate, METH_NOARGS, float_conjugate__doc__},

static PyObject *
float_conjugate_impl(PyObject *self);

static PyObject *
float_conjugate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_conjugate_impl(self);
}

PyDoc_STRVAR(float_hex__doc__,
"hex($self, /)\n"
"--\n"
"\n"
"Return a hexadecimal representation of a floating-point number.\n"
"\n"
">>> (-0.1).hex()\n"
"\'-0x1.999999999999ap-4\'\n"
">>> 3.14159.hex()\n"
"\'0x1.921f9f01b866ep+1\'");

#define FLOAT_HEX_METHODDEF    \
    {"hex", (PyCFunction)float_hex, METH_NOARGS, float_hex__doc__},

static PyObject *
float_hex_impl(PyObject *self);

static PyObject *
float_hex(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_hex_impl(self);
}

PyDoc_STRVAR(float_fromhex__doc__,
"fromhex($type, string, /)\n"
"--\n"
"\n"
"Create a floating-point number from a hexadecimal string.\n"
"\n"
">>> float.fromhex(\'0x1.ffffp10\')\n"
"2047.984375\n"
">>> float.fromhex(\'-0x1p-1074\')\n"
"-5e-324");

#define FLOAT_FROMHEX_METHODDEF    \
    {"fromhex", _PyCFunction_CAST(float_fromhex), METH_O|METH_CLASS, float_fromhex__doc__},

static PyObject *
float_fromhex_impl(PyTypeObject *type, PyObject *string);

static PyValue
float_fromhex(PyObject *type, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;
    PyObject *_return_value;

    string = PyValue_Object(arg);
    if (string == NULL) {
        goto exit;
    }
    _return_value = float_fromhex_impl((PyTypeObject *)type, string);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(float_as_integer_ratio__doc__,
"as_integer_ratio($self, /)\n"
"--\n"
"\n"
"Return a pair of integers, whose ratio is exactly equal to the original float.\n"
"\n"
"The ratio is in lowest terms and has a positive denominator.  Raise\n"
"OverflowError on infinities and a ValueError on NaNs.\n"
"\n"
">>> (10.0).as_integer_ratio()\n"
"(10, 1)\n"
">>> (0.0).as_integer_ratio()\n"
"(0, 1)\n"
">>> (-.25).as_integer_ratio()\n"
"(-1, 4)");

#define FLOAT_AS_INTEGER_RATIO_METHODDEF    \
    {"as_integer_ratio", (PyCFunction)float_as_integer_ratio, METH_NOARGS, float_as_integer_ratio__doc__},

static PyObject *
float_as_integer_ratio_impl(PyObject *self);

static PyObject *
float_as_integer_ratio(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float_as_integer_ratio_impl(self);
}

PyDoc_STRVAR(float_new__doc__,
"float(x=0, /)\n"
"--\n"
"\n"
"Convert a string or number to a floating-point number, if possible.");

static PyValue
float_new_impl(PyTypeObject *type, PyValue x);

static PyObject *
float_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyFloat_Type;
    PyValue x = PyValue_NULL;
    PyValue _return_value;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("float", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("float", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    x = PyTuple_GET_VALUE(args, 0);
skip_optional:
    _return_value = float_new_impl(type, x);
    return_value = PyValue_Object(_return_value);

exit:
    return return_value;
}

static PyValue
float_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyValue x = PyValue_NULL;
    PyValue _return_value;

    assert(_PyType_CAST(type)->tp_new == float_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("float", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("float", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    x = args[0];
skip_optional:
    _return_value = float_new_impl(_PyType_CAST(type), x);
    return_value = PyValue_Object(_return_value);

exit:
    return PyValue_FromResult(return_value);
}

PyDoc_STRVAR(float_from_number__doc__,
"from_number($type, number, /)\n"
"--\n"
"\n"
"Convert real number to a floating-point number.");

#define FLOAT_FROM_NUMBER_METHODDEF    \
    {"from_number", _PyCFunction_CAST(float_from_number), METH_O|METH_CLASS, float_from_number__doc__},

static PyObject *
float_from_number_impl(PyTypeObject *type, PyObject *number);

static PyValue
float_from_number(PyObject *type, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *number;
    PyObject *_return_value;

    number = PyValue_Object(arg);
    if (number == NULL) {
        goto exit;
    }
    _return_value = float_from_number_impl((PyTypeObject *)type, number);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(float___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define FLOAT___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)float___getnewargs__, METH_NOARGS, float___getnewargs____doc__},

static PyObject *
float___getnewargs___impl(PyObject *self);

static PyObject *
float___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return float___getnewargs___impl(self);
}

PyDoc_STRVAR(float___getformat____doc__,
"__getformat__($type, typestr, /)\n"
"--\n"
"\n"
"You probably don\'t want to use this function.\n"
"\n"
"  typestr\n"
"    Must be \'double\' or \'float\'.\n"
"\n"
"It exists mainly to be used in Python\'s test suite.\n"
"\n"
"This function returns whichever of \'IEEE, big-endian\' or \'IEEE,\n"
"little-endian\' best describes the format of floating-point numbers\n"
"used by the C type named by typestr.");

#define FLOAT___GETFORMAT___METHODDEF    \
    {"__getformat__", _PyCFunction_CAST(float___getformat__), METH_O|METH_CLASS, float___getformat____doc__},

static PyObject *
float___getformat___impl(PyTypeObject *type, const char *typestr);

static PyValue
float___getformat__(PyObject *type, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *typestr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("__getformat__", "argument", "str", _arg);
            goto exit;
        }
        typestr = _PyUnicode_AsUTF8NoNUL(_arg);
        if (typestr == NULL) {
            goto exit;
        }
    }
    _return_value = float___getformat___impl((PyTypeObject *)type, typestr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(float___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Formats the float according to format_spec.");

#define FLOAT___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(float___format__), METH_O, float___format____doc__},

static PyObject *
float___format___impl(PyObject *self, PyObject *format_spec);

static PyValue
float___format__(PyObject *self, PyValue arg)
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
    _return_value = float___format___impl(self, format_spec);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=f4b421f992da63ff input=a9049054013a1b77]*/
