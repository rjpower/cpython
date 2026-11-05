/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(math_integer_gcd__doc__,
"gcd($module, /, *integers)\n"
"--\n"
"\n"
"Greatest Common Divisor.");

#define MATH_INTEGER_GCD_METHODDEF    \
    {"gcd", _PyCFunction_CAST(math_integer_gcd), METH_VALUE, math_integer_gcd__doc__},

static PyValue
math_integer_gcd_impl(PyObject *module, const PyValue *args,
                      Py_ssize_t args_length);

static PyValue
math_integer_gcd(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    if (!_PyArg_NoKwnamesValue("math.math.integer.gcd()", kwnames)) {
        goto exit;
    }
    __clinic_args = args;
    args_length = nargs;
    return_value = math_integer_gcd_impl(PyValue_AsKnownObjectBorrow(module), __clinic_args, args_length);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_lcm__doc__,
"lcm($module, /, *integers)\n"
"--\n"
"\n"
"Least Common Multiple.");

#define MATH_INTEGER_LCM_METHODDEF    \
    {"lcm", _PyCFunction_CAST(math_integer_lcm), METH_VALUE, math_integer_lcm__doc__},

static PyValue
math_integer_lcm_impl(PyObject *module, const PyValue *args,
                      Py_ssize_t args_length);

static PyValue
math_integer_lcm(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    if (!_PyArg_NoKwnamesValue("math.math.integer.lcm()", kwnames)) {
        goto exit;
    }
    __clinic_args = args;
    args_length = nargs;
    return_value = math_integer_lcm_impl(PyValue_AsKnownObjectBorrow(module), __clinic_args, args_length);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_isqrt__doc__,
"isqrt($module, n, /)\n"
"--\n"
"\n"
"Return the integer part of the square root of the input.");

#define MATH_INTEGER_ISQRT_METHODDEF    \
    {"isqrt", _PyCFunction_CAST(math_integer_isqrt), METH_VALUE|METH_O, math_integer_isqrt__doc__},

static PyValue
math_integer_isqrt_impl(PyObject *module, PyValueBorrow n_arg);

static PyValue
math_integer_isqrt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow n_arg;

    n_arg = PyValue_Borrow(args[0]);
    return_value = math_integer_isqrt_impl(PyValue_AsKnownObjectBorrow(module), n_arg);

    return return_value;
}

PyDoc_STRVAR(math_integer_factorial__doc__,
"factorial($module, n, /)\n"
"--\n"
"\n"
"Find n!.");

#define MATH_INTEGER_FACTORIAL_METHODDEF    \
    {"factorial", _PyCFunction_CAST(math_integer_factorial), METH_VALUE|METH_O, math_integer_factorial__doc__},

static PyValue
math_integer_factorial_impl(PyObject *module, PyValueBorrow arg);

static PyValue
math_integer_factorial(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = math_integer_factorial_impl(PyValue_AsKnownObjectBorrow(module), arg);

    return return_value;
}

PyDoc_STRVAR(math_integer_perm__doc__,
"perm($module, n, k=None, /)\n"
"--\n"
"\n"
"Number of ways to choose k items from n items without repetition and with order.\n"
"\n"
"Evaluates to n! / (n - k)! when k <= n and evaluates\n"
"to zero when k > n.\n"
"\n"
"If k is not specified or is None, then k defaults to n\n"
"and the function returns n!.\n"
"\n"
"Raises ValueError if either of the arguments are negative.");

#define MATH_INTEGER_PERM_METHODDEF    \
    {"perm", _PyCFunction_CAST(math_integer_perm), METH_VALUE, math_integer_perm__doc__},

static PyValue
math_integer_perm_impl(PyObject *module, PyValueBorrow n_arg,
                       PyValueBorrow k_arg);

static PyValue
math_integer_perm(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow n_arg;
    PyValueBorrow k_arg = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("math.math.integer.perm()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("perm", nargs, 1, 2)) {
        goto exit;
    }
    n_arg = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    k_arg = PyValue_Borrow(args[1]);
skip_optional:
    return_value = math_integer_perm_impl(PyValue_AsKnownObjectBorrow(module), n_arg, k_arg);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_comb__doc__,
"comb($module, n, k, /)\n"
"--\n"
"\n"
"Number of ways to choose k items from n items without repetition and without order.\n"
"\n"
"Evaluates to n! / (k! * (n - k)!) when k <= n and evaluates\n"
"to zero when k > n.\n"
"\n"
"Also called the binomial coefficient because it is equivalent\n"
"to the coefficient of k-th term in polynomial expansion of the\n"
"expression (1 + x)**n.\n"
"\n"
"Raises ValueError if either of the arguments are negative.");

#define MATH_INTEGER_COMB_METHODDEF    \
    {"comb", _PyCFunction_CAST(math_integer_comb), METH_VALUE, math_integer_comb__doc__},

static PyValue
math_integer_comb_impl(PyObject *module, PyValueBorrow n_arg,
                       PyValueBorrow k_arg);

static PyValue
math_integer_comb(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow n_arg;
    PyValueBorrow k_arg;

    if (!_PyArg_NoKwnamesValue("math.math.integer.comb()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("comb", nargs, 2, 2)) {
        goto exit;
    }
    n_arg = PyValue_Borrow(args[0]);
    k_arg = PyValue_Borrow(args[1]);
    return_value = math_integer_comb_impl(PyValue_AsKnownObjectBorrow(module), n_arg, k_arg);

exit:
    return return_value;
}
/*[clinic end generated code: output=b3497dfe47b01d6b input=a9049054013a1b77]*/
