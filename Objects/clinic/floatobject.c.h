/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(float_is_integer__doc__,
"is_integer($self, /)\n"
"--\n"
"\n"
"Return True if the float is an integer.");

#define FLOAT_IS_INTEGER_METHODDEF    \
    {"is_integer", _PyCFunction_CAST(float_is_integer), METH_VALUE|METH_NOARGS, float_is_integer__doc__},

static PyValue
float_is_integer_impl(PyValueBorrow self);

static PyValue
float_is_integer(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float_is_integer_impl(self);

    return return_value;
}

PyDoc_STRVAR(float___trunc____doc__,
"__trunc__($self, /)\n"
"--\n"
"\n"
"Return the Integral closest to x between 0 and x.");

#define FLOAT___TRUNC___METHODDEF    \
    {"__trunc__", _PyCFunction_CAST(float___trunc__), METH_VALUE|METH_NOARGS, float___trunc____doc__},

static PyValue
float___trunc___impl(PyValueBorrow self);

static PyValue
float___trunc__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float___trunc___impl(self);

    return return_value;
}

PyDoc_STRVAR(float___floor____doc__,
"__floor__($self, /)\n"
"--\n"
"\n"
"Return the floor as an Integral.");

#define FLOAT___FLOOR___METHODDEF    \
    {"__floor__", _PyCFunction_CAST(float___floor__), METH_VALUE|METH_NOARGS, float___floor____doc__},

static PyValue
float___floor___impl(PyValueBorrow self);

static PyValue
float___floor__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float___floor___impl(self);

    return return_value;
}

PyDoc_STRVAR(float___ceil____doc__,
"__ceil__($self, /)\n"
"--\n"
"\n"
"Return the ceiling as an Integral.");

#define FLOAT___CEIL___METHODDEF    \
    {"__ceil__", _PyCFunction_CAST(float___ceil__), METH_VALUE|METH_NOARGS, float___ceil____doc__},

static PyValue
float___ceil___impl(PyValueBorrow self);

static PyValue
float___ceil__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float___ceil___impl(self);

    return return_value;
}

PyDoc_STRVAR(float___round____doc__,
"__round__($self, ndigits=None, /)\n"
"--\n"
"\n"
"Return the Integral closest to x, rounding half toward even.\n"
"\n"
"When an argument is passed, work like built-in round(x, ndigits).");

#define FLOAT___ROUND___METHODDEF    \
    {"__round__", _PyCFunction_CAST(float___round__), METH_VALUE, float___round____doc__},

static PyValue
float___round___impl(PyValueBorrow self, PyValueBorrow o_ndigits);

static PyValue
float___round__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow o_ndigits = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyFloat_Type, "__round__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__round__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    o_ndigits = PyValue_Borrow(args[0]);
skip_optional:
    return_value = float___round___impl(self, o_ndigits);

exit:
    return return_value;
}

PyDoc_STRVAR(float_conjugate__doc__,
"conjugate($self, /)\n"
"--\n"
"\n"
"Return self, the complex conjugate of any float.");

#define FLOAT_CONJUGATE_METHODDEF    \
    {"conjugate", _PyCFunction_CAST(float_conjugate), METH_VALUE|METH_NOARGS, float_conjugate__doc__},

static PyValue
float_conjugate_impl(PyValueBorrow self);

static PyValue
float_conjugate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float_conjugate_impl(self);

    return return_value;
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
    {"hex", _PyCFunction_CAST(float_hex), METH_VALUE|METH_NOARGS, float_hex__doc__},

static PyValue
float_hex_impl(PyValueBorrow self);

static PyValue
float_hex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float_hex_impl(self);

    return return_value;
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
    {"fromhex", _PyCFunction_CAST(float_fromhex), METH_VALUE|METH_O|METH_CLASS, float_fromhex__doc__},

static PyValue
float_fromhex_impl(PyTypeObject *type, PyValueBorrow string);

static PyValue
float_fromhex(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    return_value = float_fromhex_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string);

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
    {"as_integer_ratio", _PyCFunction_CAST(float_as_integer_ratio), METH_VALUE|METH_NOARGS, float_as_integer_ratio__doc__},

static PyValue
float_as_integer_ratio_impl(PyValueBorrow self);

static PyValue
float_as_integer_ratio(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float_as_integer_ratio_impl(self);

    return return_value;
}

PyDoc_STRVAR(float_new__doc__,
"float(x=0, /)\n"
"--\n"
"\n"
"Convert a string or number to a floating-point number, if possible.");

static PyValue
float_new_impl(PyTypeObject *type, PyValueBorrow x);

static PyValue
float_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = &PyFloat_Type;
    PyValueBorrow x = PyValue_Borrow(PyValue_NULL);

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("float", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("float", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    x = PyValue_Borrow(args[0]);
skip_optional:
    return_value = float_new_impl(type, x);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
float_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(float_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = float_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
float_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == float_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    return float_new_value(_PyType_CAST(type), args, nargs, kwnames);
}

PyDoc_STRVAR(float_from_number__doc__,
"from_number($type, number, /)\n"
"--\n"
"\n"
"Convert real number to a floating-point number.");

#define FLOAT_FROM_NUMBER_METHODDEF    \
    {"from_number", _PyCFunction_CAST(float_from_number), METH_VALUE|METH_O|METH_CLASS, float_from_number__doc__},

static PyValue
float_from_number_impl(PyTypeObject *type, PyValueBorrow number);

static PyValue
float_from_number(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow number;

    number = PyValue_Borrow(args[0]);
    return_value = float_from_number_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), number);

    return return_value;
}

PyDoc_STRVAR(float___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define FLOAT___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", _PyCFunction_CAST(float___getnewargs__), METH_VALUE|METH_NOARGS, float___getnewargs____doc__},

static PyValue
float___getnewargs___impl(PyValueBorrow self);

static PyValue
float___getnewargs__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = float___getnewargs___impl(self);

    return return_value;
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
    {"__getformat__", _PyCFunction_CAST(float___getformat__), METH_VALUE|METH_O|METH_CLASS, float___getformat____doc__},

static PyValue
float___getformat___impl(PyTypeObject *type, const char *typestr);

static PyValue
float___getformat__(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *typestr;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__getformat__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__getformat__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    typestr = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (typestr == NULL) {
        goto exit;
    }
    return_value = float___getformat___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), typestr);

exit:
    return return_value;
}

PyDoc_STRVAR(float___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Formats the float according to format_spec.");

#define FLOAT___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(float___format__), METH_VALUE|METH_O, float___format____doc__},

static PyValue
float___format___impl(PyValueBorrow self, PyObject *format_spec);

static PyValue
float___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format_spec = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = float___format___impl(self, format_spec);

exit:
    return return_value;
}
/*[clinic end generated code: output=07225286e2b679ee input=a9049054013a1b77]*/
