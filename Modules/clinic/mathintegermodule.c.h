/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(math_integer_gcd__doc__,
"gcd($module, /, *integers)\n"
"--\n"
"\n"
"Greatest Common Divisor.");

#define MATH_INTEGER_GCD_METHODDEF    \
    {"gcd", _PyCFunction_CAST(math_integer_gcd), METH_FASTCALL, math_integer_gcd__doc__},

static PyValue
math_integer_gcd_impl(PyObject *module, const PyValue *args,
                      Py_ssize_t args_length);

static PyValue
math_integer_gcd(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;
    PyValue _return_value;

    __clinic_args = args;
    args_length = nargs;
    _return_value = math_integer_gcd_impl(module, __clinic_args, args_length);
    return_value = _return_value;

    return return_value;
}

PyDoc_STRVAR(math_integer_lcm__doc__,
"lcm($module, /, *integers)\n"
"--\n"
"\n"
"Least Common Multiple.");

#define MATH_INTEGER_LCM_METHODDEF    \
    {"lcm", _PyCFunction_CAST(math_integer_lcm), METH_FASTCALL, math_integer_lcm__doc__},

static PyValue
math_integer_lcm_impl(PyObject *module, const PyValue *args,
                      Py_ssize_t args_length);

static PyValue
math_integer_lcm(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;
    PyValue _return_value;

    __clinic_args = args;
    args_length = nargs;
    _return_value = math_integer_lcm_impl(module, __clinic_args, args_length);
    return_value = _return_value;

    return return_value;
}

PyDoc_STRVAR(math_integer_isqrt__doc__,
"isqrt($module, n, /)\n"
"--\n"
"\n"
"Return the integer part of the square root of the input.");

#define MATH_INTEGER_ISQRT_METHODDEF    \
    {"isqrt", _PyCFunction_CAST(math_integer_isqrt), METH_O, math_integer_isqrt__doc__},

static PyObject *
math_integer_isqrt_impl(PyObject *module, PyObject *n);

static PyValue
math_integer_isqrt(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *n;
    PyObject *_return_value;

    n = PyValue_Object(arg);
    if (n == NULL) {
        goto exit;
    }
    _return_value = math_integer_isqrt_impl(module, n);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(math_integer_factorial__doc__,
"factorial($module, n, /)\n"
"--\n"
"\n"
"Find n!.");

#define MATH_INTEGER_FACTORIAL_METHODDEF    \
    {"factorial", _PyCFunction_CAST(math_integer_factorial), METH_O, math_integer_factorial__doc__},

static PyObject *
math_integer_factorial_impl(PyObject *module, PyObject *arg);

static PyValue
math_integer_factorial(PyObject *module, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = math_integer_factorial_impl(module, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"perm", _PyCFunction_CAST(math_integer_perm), METH_FASTCALL, math_integer_perm__doc__},

static PyObject *
math_integer_perm_impl(PyObject *module, PyObject *n, PyObject *k);

static PyValue
math_integer_perm(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *n;
    PyObject *k = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("perm", nargs, 1, 2)) {
        goto exit;
    }
    n = PyValue_Object(args[0]);
    if (n == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    k = PyValue_Object(args[1]);
    if (k == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = math_integer_perm_impl(module, n, k);
    return_value = PyValue_FromResult(_return_value);

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
    {"comb", _PyCFunction_CAST(math_integer_comb), METH_FASTCALL, math_integer_comb__doc__},

static PyObject *
math_integer_comb_impl(PyObject *module, PyObject *n, PyObject *k);

static PyValue
math_integer_comb(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *n;
    PyObject *k;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("comb", nargs, 2, 2)) {
        goto exit;
    }
    n = PyValue_Object(args[0]);
    if (n == NULL) {
        goto exit;
    }
    k = PyValue_Object(args[1]);
    if (k == NULL) {
        goto exit;
    }
    _return_value = math_integer_comb_impl(module, n, k);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=983be0cf0d3a8dd6 input=a9049054013a1b77]*/
