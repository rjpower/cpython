/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_operator_truth__doc__,
"truth($module, a, /)\n"
"--\n"
"\n"
"Return True if a is true, False otherwise.");

#define _OPERATOR_TRUTH_METHODDEF    \
    {"truth", _PyCFunction_CAST(_operator_truth), METH_VALUE|METH_O, _operator_truth__doc__},

static int
_operator_truth_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_truth(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    int _return_value;

    a = PyValue_Borrow(args[0]);
    _return_value = _operator_truth_impl(PyValue_AsKnownObjectBorrow(module), a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_add__doc__,
"add($module, a, b, /)\n"
"--\n"
"\n"
"Same as a + b.");

#define _OPERATOR_ADD_METHODDEF    \
    {"add", _PyCFunction_CAST(_operator_add), METH_VALUE, _operator_add__doc__},

static PyValue
_operator_add_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_add(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.add()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("add", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_add_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_sub__doc__,
"sub($module, a, b, /)\n"
"--\n"
"\n"
"Same as a - b.");

#define _OPERATOR_SUB_METHODDEF    \
    {"sub", _PyCFunction_CAST(_operator_sub), METH_VALUE, _operator_sub__doc__},

static PyValue
_operator_sub_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_sub(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.sub()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("sub", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_sub_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_mul__doc__,
"mul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a * b.");

#define _OPERATOR_MUL_METHODDEF    \
    {"mul", _PyCFunction_CAST(_operator_mul), METH_VALUE, _operator_mul__doc__},

static PyValue
_operator_mul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_mul(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.mul()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("mul", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_mul_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_matmul__doc__,
"matmul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a @ b.");

#define _OPERATOR_MATMUL_METHODDEF    \
    {"matmul", _PyCFunction_CAST(_operator_matmul), METH_VALUE, _operator_matmul__doc__},

static PyValue
_operator_matmul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_matmul(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.matmul()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("matmul", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_matmul_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_floordiv__doc__,
"floordiv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a // b.");

#define _OPERATOR_FLOORDIV_METHODDEF    \
    {"floordiv", _PyCFunction_CAST(_operator_floordiv), METH_VALUE, _operator_floordiv__doc__},

static PyValue
_operator_floordiv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_floordiv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.floordiv()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("floordiv", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_floordiv_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_truediv__doc__,
"truediv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a / b.");

#define _OPERATOR_TRUEDIV_METHODDEF    \
    {"truediv", _PyCFunction_CAST(_operator_truediv), METH_VALUE, _operator_truediv__doc__},

static PyValue
_operator_truediv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_truediv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.truediv()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("truediv", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_truediv_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_mod__doc__,
"mod($module, a, b, /)\n"
"--\n"
"\n"
"Same as a % b.");

#define _OPERATOR_MOD_METHODDEF    \
    {"mod", _PyCFunction_CAST(_operator_mod), METH_VALUE, _operator_mod__doc__},

static PyValue
_operator_mod_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_mod(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.mod()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("mod", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_mod_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_neg__doc__,
"neg($module, a, /)\n"
"--\n"
"\n"
"Same as -a.");

#define _OPERATOR_NEG_METHODDEF    \
    {"neg", _PyCFunction_CAST(_operator_neg), METH_VALUE|METH_O, _operator_neg__doc__},

static PyValue
_operator_neg_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_neg(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_neg_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_pos__doc__,
"pos($module, a, /)\n"
"--\n"
"\n"
"Same as +a.");

#define _OPERATOR_POS_METHODDEF    \
    {"pos", _PyCFunction_CAST(_operator_pos), METH_VALUE|METH_O, _operator_pos__doc__},

static PyValue
_operator_pos_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_pos(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_pos_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_abs__doc__,
"abs($module, a, /)\n"
"--\n"
"\n"
"Same as abs(a).");

#define _OPERATOR_ABS_METHODDEF    \
    {"abs", _PyCFunction_CAST(_operator_abs), METH_VALUE|METH_O, _operator_abs__doc__},

static PyValue
_operator_abs_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_abs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_abs_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_inv__doc__,
"inv($module, a, /)\n"
"--\n"
"\n"
"Same as ~a.");

#define _OPERATOR_INV_METHODDEF    \
    {"inv", _PyCFunction_CAST(_operator_inv), METH_VALUE|METH_O, _operator_inv__doc__},

static PyValue
_operator_inv_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_inv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_inv_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_invert__doc__,
"invert($module, a, /)\n"
"--\n"
"\n"
"Same as ~a.");

#define _OPERATOR_INVERT_METHODDEF    \
    {"invert", _PyCFunction_CAST(_operator_invert), METH_VALUE|METH_O, _operator_invert__doc__},

static PyValue
_operator_invert_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_invert(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_invert_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_lshift__doc__,
"lshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a << b.");

#define _OPERATOR_LSHIFT_METHODDEF    \
    {"lshift", _PyCFunction_CAST(_operator_lshift), METH_VALUE, _operator_lshift__doc__},

static PyValue
_operator_lshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_lshift(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.lshift()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("lshift", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_lshift_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_rshift__doc__,
"rshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a >> b.");

#define _OPERATOR_RSHIFT_METHODDEF    \
    {"rshift", _PyCFunction_CAST(_operator_rshift), METH_VALUE, _operator_rshift__doc__},

static PyValue
_operator_rshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_rshift(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.rshift()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rshift", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_rshift_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_not___doc__,
"not_($module, a, /)\n"
"--\n"
"\n"
"Same as not a.");

#define _OPERATOR_NOT__METHODDEF    \
    {"not_", _PyCFunction_CAST(_operator_not_), METH_VALUE|METH_O, _operator_not___doc__},

static int
_operator_not__impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_not_(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    int _return_value;

    a = PyValue_Borrow(args[0]);
    _return_value = _operator_not__impl(PyValue_AsKnownObjectBorrow(module), a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_and___doc__,
"and_($module, a, b, /)\n"
"--\n"
"\n"
"Same as a & b.");

#define _OPERATOR_AND__METHODDEF    \
    {"and_", _PyCFunction_CAST(_operator_and_), METH_VALUE, _operator_and___doc__},

static PyValue
_operator_and__impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_and_(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.and_()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("and_", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_and__impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_xor__doc__,
"xor($module, a, b, /)\n"
"--\n"
"\n"
"Same as a ^ b.");

#define _OPERATOR_XOR_METHODDEF    \
    {"xor", _PyCFunction_CAST(_operator_xor), METH_VALUE, _operator_xor__doc__},

static PyValue
_operator_xor_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_xor(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.xor()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("xor", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_xor_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_or___doc__,
"or_($module, a, b, /)\n"
"--\n"
"\n"
"Same as a | b.");

#define _OPERATOR_OR__METHODDEF    \
    {"or_", _PyCFunction_CAST(_operator_or_), METH_VALUE, _operator_or___doc__},

static PyValue
_operator_or__impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_or_(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.or_()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("or_", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_or__impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_iadd__doc__,
"iadd($module, a, b, /)\n"
"--\n"
"\n"
"Same as a += b.");

#define _OPERATOR_IADD_METHODDEF    \
    {"iadd", _PyCFunction_CAST(_operator_iadd), METH_VALUE, _operator_iadd__doc__},

static PyValue
_operator_iadd_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_iadd(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.iadd()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("iadd", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_iadd_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_isub__doc__,
"isub($module, a, b, /)\n"
"--\n"
"\n"
"Same as a -= b.");

#define _OPERATOR_ISUB_METHODDEF    \
    {"isub", _PyCFunction_CAST(_operator_isub), METH_VALUE, _operator_isub__doc__},

static PyValue
_operator_isub_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_isub(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.isub()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("isub", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_isub_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_imul__doc__,
"imul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a *= b.");

#define _OPERATOR_IMUL_METHODDEF    \
    {"imul", _PyCFunction_CAST(_operator_imul), METH_VALUE, _operator_imul__doc__},

static PyValue
_operator_imul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_imul(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.imul()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("imul", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_imul_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_imatmul__doc__,
"imatmul($module, a, b, /)\n"
"--\n"
"\n"
"Same as a @= b.");

#define _OPERATOR_IMATMUL_METHODDEF    \
    {"imatmul", _PyCFunction_CAST(_operator_imatmul), METH_VALUE, _operator_imatmul__doc__},

static PyValue
_operator_imatmul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_imatmul(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.imatmul()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("imatmul", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_imatmul_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ifloordiv__doc__,
"ifloordiv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a //= b.");

#define _OPERATOR_IFLOORDIV_METHODDEF    \
    {"ifloordiv", _PyCFunction_CAST(_operator_ifloordiv), METH_VALUE, _operator_ifloordiv__doc__},

static PyValue
_operator_ifloordiv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ifloordiv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ifloordiv()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ifloordiv", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ifloordiv_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_itruediv__doc__,
"itruediv($module, a, b, /)\n"
"--\n"
"\n"
"Same as a /= b.");

#define _OPERATOR_ITRUEDIV_METHODDEF    \
    {"itruediv", _PyCFunction_CAST(_operator_itruediv), METH_VALUE, _operator_itruediv__doc__},

static PyValue
_operator_itruediv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_itruediv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.itruediv()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("itruediv", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_itruediv_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_imod__doc__,
"imod($module, a, b, /)\n"
"--\n"
"\n"
"Same as a %= b.");

#define _OPERATOR_IMOD_METHODDEF    \
    {"imod", _PyCFunction_CAST(_operator_imod), METH_VALUE, _operator_imod__doc__},

static PyValue
_operator_imod_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_imod(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.imod()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("imod", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_imod_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ilshift__doc__,
"ilshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a <<= b.");

#define _OPERATOR_ILSHIFT_METHODDEF    \
    {"ilshift", _PyCFunction_CAST(_operator_ilshift), METH_VALUE, _operator_ilshift__doc__},

static PyValue
_operator_ilshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ilshift(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ilshift()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ilshift", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ilshift_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_irshift__doc__,
"irshift($module, a, b, /)\n"
"--\n"
"\n"
"Same as a >>= b.");

#define _OPERATOR_IRSHIFT_METHODDEF    \
    {"irshift", _PyCFunction_CAST(_operator_irshift), METH_VALUE, _operator_irshift__doc__},

static PyValue
_operator_irshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_irshift(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.irshift()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("irshift", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_irshift_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_iand__doc__,
"iand($module, a, b, /)\n"
"--\n"
"\n"
"Same as a &= b.");

#define _OPERATOR_IAND_METHODDEF    \
    {"iand", _PyCFunction_CAST(_operator_iand), METH_VALUE, _operator_iand__doc__},

static PyValue
_operator_iand_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_iand(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.iand()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("iand", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_iand_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ixor__doc__,
"ixor($module, a, b, /)\n"
"--\n"
"\n"
"Same as a ^= b.");

#define _OPERATOR_IXOR_METHODDEF    \
    {"ixor", _PyCFunction_CAST(_operator_ixor), METH_VALUE, _operator_ixor__doc__},

static PyValue
_operator_ixor_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ixor(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ixor()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ixor", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ixor_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ior__doc__,
"ior($module, a, b, /)\n"
"--\n"
"\n"
"Same as a |= b.");

#define _OPERATOR_IOR_METHODDEF    \
    {"ior", _PyCFunction_CAST(_operator_ior), METH_VALUE, _operator_ior__doc__},

static PyValue
_operator_ior_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ior(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ior()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ior", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ior_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_concat__doc__,
"concat($module, a, b, /)\n"
"--\n"
"\n"
"Same as a + b, for a and b sequences.");

#define _OPERATOR_CONCAT_METHODDEF    \
    {"concat", _PyCFunction_CAST(_operator_concat), METH_VALUE, _operator_concat__doc__},

static PyValue
_operator_concat_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_concat(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.concat()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("concat", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_concat_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_iconcat__doc__,
"iconcat($module, a, b, /)\n"
"--\n"
"\n"
"Same as a += b, for a and b sequences.");

#define _OPERATOR_ICONCAT_METHODDEF    \
    {"iconcat", _PyCFunction_CAST(_operator_iconcat), METH_VALUE, _operator_iconcat__doc__},

static PyValue
_operator_iconcat_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_iconcat(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.iconcat()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("iconcat", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_iconcat_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_contains__doc__,
"contains($module, a, b, /)\n"
"--\n"
"\n"
"Same as b in a (note reversed operands).");

#define _OPERATOR_CONTAINS_METHODDEF    \
    {"contains", _PyCFunction_CAST(_operator_contains), METH_VALUE, _operator_contains__doc__},

static int
_operator_contains_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_contains(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;
    int _return_value;

    if (!_PyArg_NoKwnamesValue("_operator.contains()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("contains", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    _return_value = _operator_contains_impl(PyValue_AsKnownObjectBorrow(module), a, b);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_indexOf__doc__,
"indexOf($module, a, b, /)\n"
"--\n"
"\n"
"Return the first index of b in a.");

#define _OPERATOR_INDEXOF_METHODDEF    \
    {"indexOf", _PyCFunction_CAST(_operator_indexOf), METH_VALUE, _operator_indexOf__doc__},

static Py_ssize_t
_operator_indexOf_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_indexOf(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValue("_operator.indexOf()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("indexOf", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    _return_value = _operator_indexOf_impl(PyValue_AsKnownObjectBorrow(module), a, b);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_countOf__doc__,
"countOf($module, a, b, /)\n"
"--\n"
"\n"
"Return the number of items in a which are, or which equal, b.");

#define _OPERATOR_COUNTOF_METHODDEF    \
    {"countOf", _PyCFunction_CAST(_operator_countOf), METH_VALUE, _operator_countOf__doc__},

static Py_ssize_t
_operator_countOf_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_countOf(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValue("_operator.countOf()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("countOf", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    _return_value = _operator_countOf_impl(PyValue_AsKnownObjectBorrow(module), a, b);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_getitem__doc__,
"getitem($module, a, b, /)\n"
"--\n"
"\n"
"Same as a[b].");

#define _OPERATOR_GETITEM_METHODDEF    \
    {"getitem", _PyCFunction_CAST(_operator_getitem), METH_VALUE, _operator_getitem__doc__},

static PyValue
_operator_getitem_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_getitem(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.getitem()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("getitem", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_getitem_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_setitem__doc__,
"setitem($module, a, b, c, /)\n"
"--\n"
"\n"
"Same as a[b] = c.");

#define _OPERATOR_SETITEM_METHODDEF    \
    {"setitem", _PyCFunction_CAST(_operator_setitem), METH_VALUE, _operator_setitem__doc__},

static PyValue
_operator_setitem_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
                       PyValueBorrow c);

static PyValue
_operator_setitem(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;
    PyValueBorrow c;

    if (!_PyArg_NoKwnamesValue("_operator.setitem()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("setitem", nargs, 3, 3)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    c = PyValue_Borrow(args[2]);
    return_value = _operator_setitem_impl(PyValue_AsKnownObjectBorrow(module), a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_delitem__doc__,
"delitem($module, a, b, /)\n"
"--\n"
"\n"
"Same as del a[b].");

#define _OPERATOR_DELITEM_METHODDEF    \
    {"delitem", _PyCFunction_CAST(_operator_delitem), METH_VALUE, _operator_delitem__doc__},

static PyValue
_operator_delitem_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_delitem(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.delitem()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("delitem", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_delitem_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_eq__doc__,
"eq($module, a, b, /)\n"
"--\n"
"\n"
"Same as a == b.");

#define _OPERATOR_EQ_METHODDEF    \
    {"eq", _PyCFunction_CAST(_operator_eq), METH_VALUE, _operator_eq__doc__},

static PyValue
_operator_eq_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_eq(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.eq()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("eq", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_eq_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ne__doc__,
"ne($module, a, b, /)\n"
"--\n"
"\n"
"Same as a != b.");

#define _OPERATOR_NE_METHODDEF    \
    {"ne", _PyCFunction_CAST(_operator_ne), METH_VALUE, _operator_ne__doc__},

static PyValue
_operator_ne_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ne(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ne()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ne", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ne_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_lt__doc__,
"lt($module, a, b, /)\n"
"--\n"
"\n"
"Same as a < b.");

#define _OPERATOR_LT_METHODDEF    \
    {"lt", _PyCFunction_CAST(_operator_lt), METH_VALUE, _operator_lt__doc__},

static PyValue
_operator_lt_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_lt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.lt()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("lt", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_lt_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_le__doc__,
"le($module, a, b, /)\n"
"--\n"
"\n"
"Same as a <= b.");

#define _OPERATOR_LE_METHODDEF    \
    {"le", _PyCFunction_CAST(_operator_le), METH_VALUE, _operator_le__doc__},

static PyValue
_operator_le_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_le(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.le()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("le", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_le_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_gt__doc__,
"gt($module, a, b, /)\n"
"--\n"
"\n"
"Same as a > b.");

#define _OPERATOR_GT_METHODDEF    \
    {"gt", _PyCFunction_CAST(_operator_gt), METH_VALUE, _operator_gt__doc__},

static PyValue
_operator_gt_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_gt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.gt()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("gt", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_gt_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ge__doc__,
"ge($module, a, b, /)\n"
"--\n"
"\n"
"Same as a >= b.");

#define _OPERATOR_GE_METHODDEF    \
    {"ge", _PyCFunction_CAST(_operator_ge), METH_VALUE, _operator_ge__doc__},

static PyValue
_operator_ge_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ge(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ge()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ge", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ge_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_pow__doc__,
"pow($module, a, b, /)\n"
"--\n"
"\n"
"Same as a ** b.");

#define _OPERATOR_POW_METHODDEF    \
    {"pow", _PyCFunction_CAST(_operator_pow), METH_VALUE, _operator_pow__doc__},

static PyValue
_operator_pow_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_pow(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.pow()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pow", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_pow_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_ipow__doc__,
"ipow($module, a, b, /)\n"
"--\n"
"\n"
"Same as a **= b.");

#define _OPERATOR_IPOW_METHODDEF    \
    {"ipow", _PyCFunction_CAST(_operator_ipow), METH_VALUE, _operator_ipow__doc__},

static PyValue
_operator_ipow_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_ipow(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.ipow()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ipow", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_ipow_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_index__doc__,
"index($module, a, /)\n"
"--\n"
"\n"
"Same as a.__index__()");

#define _OPERATOR_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(_operator_index), METH_VALUE|METH_O, _operator_index__doc__},

static PyValue
_operator_index_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_index(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_index_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_is___doc__,
"is_($module, a, b, /)\n"
"--\n"
"\n"
"Same as a is b.");

#define _OPERATOR_IS__METHODDEF    \
    {"is_", _PyCFunction_CAST(_operator_is_), METH_VALUE, _operator_is___doc__},

static PyValue
_operator_is__impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_is_(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.is_()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("is_", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_is__impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_is_not__doc__,
"is_not($module, a, b, /)\n"
"--\n"
"\n"
"Same as a is not b.");

#define _OPERATOR_IS_NOT_METHODDEF    \
    {"is_not", _PyCFunction_CAST(_operator_is_not), METH_VALUE, _operator_is_not__doc__},

static PyValue
_operator_is_not_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
_operator_is_not(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator.is_not()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("is_not", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator_is_not_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator_is_none__doc__,
"is_none($module, a, /)\n"
"--\n"
"\n"
"Same as a is None.");

#define _OPERATOR_IS_NONE_METHODDEF    \
    {"is_none", _PyCFunction_CAST(_operator_is_none), METH_VALUE|METH_O, _operator_is_none__doc__},

static PyValue
_operator_is_none_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_is_none(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_is_none_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_is_not_none__doc__,
"is_not_none($module, a, /)\n"
"--\n"
"\n"
"Same as a is not None.");

#define _OPERATOR_IS_NOT_NONE_METHODDEF    \
    {"is_not_none", _PyCFunction_CAST(_operator_is_not_none), METH_VALUE|METH_O, _operator_is_not_none__doc__},

static PyValue
_operator_is_not_none_impl(PyObject *module, PyValueBorrow a);

static PyValue
_operator_is_not_none(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    a = PyValue_Borrow(args[0]);
    return_value = _operator_is_not_none_impl(PyValue_AsKnownObjectBorrow(module), a);

    return return_value;
}

PyDoc_STRVAR(_operator_length_hint__doc__,
"length_hint($module, obj, default=0, /)\n"
"--\n"
"\n"
"Return an estimate of the number of items in obj.\n"
"\n"
"This is useful for presizing containers when building from an iterable.\n"
"\n"
"If the object supports len(), the result will be exact.\n"
"Otherwise, it may over- or under-estimate by an arbitrary amount.\n"
"The result will be an integer >= 0.");

#define _OPERATOR_LENGTH_HINT_METHODDEF    \
    {"length_hint", _PyCFunction_CAST(_operator_length_hint), METH_VALUE, _operator_length_hint__doc__},

static Py_ssize_t
_operator_length_hint_impl(PyObject *module, PyValueBorrow obj,
                           Py_ssize_t default_value);

static PyValue
_operator_length_hint(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    Py_ssize_t default_value = 0;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValue("_operator.length_hint()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("length_hint", nargs, 1, 2)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        default_value = ival;
    }
skip_optional:
    _return_value = _operator_length_hint_impl(PyValue_AsKnownObjectBorrow(module), obj, default_value);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_operator__compare_digest__doc__,
"_compare_digest($module, a, b, /)\n"
"--\n"
"\n"
"Return \'a == b\'.\n"
"\n"
"This function uses an approach designed to prevent\n"
"timing analysis, making it appropriate for cryptography.\n"
"\n"
"a and b must both be of the same type: either str (ASCII only),\n"
"or any bytes-like object.\n"
"\n"
"Note: If a and b are of different lengths, or if an error occurs,\n"
"a timing attack could theoretically reveal information about the\n"
"types and lengths of a and b--but not their values.");

#define _OPERATOR__COMPARE_DIGEST_METHODDEF    \
    {"_compare_digest", _PyCFunction_CAST(_operator__compare_digest), METH_VALUE, _operator__compare_digest__doc__},

static PyValue
_operator__compare_digest_impl(PyObject *module, PyValueBorrow a,
                               PyValueBorrow b);

static PyValue
_operator__compare_digest(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_NoKwnamesValue("_operator._compare_digest()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_compare_digest", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _operator__compare_digest_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}
/*[clinic end generated code: output=d0bbec4ef51bf25f input=a9049054013a1b77]*/
