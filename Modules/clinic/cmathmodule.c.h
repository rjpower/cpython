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
#include "pycore_long_value.h"    // _PyValue_AsDouble()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(cmath_acos__doc__,
"acos($module, z, /)\n"
"--\n"
"\n"
"Return the arc cosine of z.");

#define CMATH_ACOS_METHODDEF    \
    {"acos", _PyCFunction_CAST(cmath_acos), METH_VALUE|METH_O, cmath_acos__doc__},

static Py_complex
cmath_acos_impl(PyObject *module, Py_complex z);

static PyValue
cmath_acos(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_acos_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_acosh__doc__,
"acosh($module, z, /)\n"
"--\n"
"\n"
"Return the inverse hyperbolic cosine of z.");

#define CMATH_ACOSH_METHODDEF    \
    {"acosh", _PyCFunction_CAST(cmath_acosh), METH_VALUE|METH_O, cmath_acosh__doc__},

static Py_complex
cmath_acosh_impl(PyObject *module, Py_complex z);

static PyValue
cmath_acosh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_acosh_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_asin__doc__,
"asin($module, z, /)\n"
"--\n"
"\n"
"Return the arc sine of z.");

#define CMATH_ASIN_METHODDEF    \
    {"asin", _PyCFunction_CAST(cmath_asin), METH_VALUE|METH_O, cmath_asin__doc__},

static Py_complex
cmath_asin_impl(PyObject *module, Py_complex z);

static PyValue
cmath_asin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_asin_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_asinh__doc__,
"asinh($module, z, /)\n"
"--\n"
"\n"
"Return the inverse hyperbolic sine of z.");

#define CMATH_ASINH_METHODDEF    \
    {"asinh", _PyCFunction_CAST(cmath_asinh), METH_VALUE|METH_O, cmath_asinh__doc__},

static Py_complex
cmath_asinh_impl(PyObject *module, Py_complex z);

static PyValue
cmath_asinh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_asinh_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_atan__doc__,
"atan($module, z, /)\n"
"--\n"
"\n"
"Return the arc tangent of z.");

#define CMATH_ATAN_METHODDEF    \
    {"atan", _PyCFunction_CAST(cmath_atan), METH_VALUE|METH_O, cmath_atan__doc__},

static Py_complex
cmath_atan_impl(PyObject *module, Py_complex z);

static PyValue
cmath_atan(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_atan_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_atanh__doc__,
"atanh($module, z, /)\n"
"--\n"
"\n"
"Return the inverse hyperbolic tangent of z.");

#define CMATH_ATANH_METHODDEF    \
    {"atanh", _PyCFunction_CAST(cmath_atanh), METH_VALUE|METH_O, cmath_atanh__doc__},

static Py_complex
cmath_atanh_impl(PyObject *module, Py_complex z);

static PyValue
cmath_atanh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_atanh_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_cos__doc__,
"cos($module, z, /)\n"
"--\n"
"\n"
"Return the cosine of z.");

#define CMATH_COS_METHODDEF    \
    {"cos", _PyCFunction_CAST(cmath_cos), METH_VALUE|METH_O, cmath_cos__doc__},

static Py_complex
cmath_cos_impl(PyObject *module, Py_complex z);

static PyValue
cmath_cos(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_cos_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_cosh__doc__,
"cosh($module, z, /)\n"
"--\n"
"\n"
"Return the hyperbolic cosine of z.");

#define CMATH_COSH_METHODDEF    \
    {"cosh", _PyCFunction_CAST(cmath_cosh), METH_VALUE|METH_O, cmath_cosh__doc__},

static Py_complex
cmath_cosh_impl(PyObject *module, Py_complex z);

static PyValue
cmath_cosh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_cosh_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_exp__doc__,
"exp($module, z, /)\n"
"--\n"
"\n"
"Return the exponential value e**z.");

#define CMATH_EXP_METHODDEF    \
    {"exp", _PyCFunction_CAST(cmath_exp), METH_VALUE|METH_O, cmath_exp__doc__},

static Py_complex
cmath_exp_impl(PyObject *module, Py_complex z);

static PyValue
cmath_exp(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_exp_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_log10__doc__,
"log10($module, z, /)\n"
"--\n"
"\n"
"Return the base-10 logarithm of z.");

#define CMATH_LOG10_METHODDEF    \
    {"log10", _PyCFunction_CAST(cmath_log10), METH_VALUE|METH_O, cmath_log10__doc__},

static Py_complex
cmath_log10_impl(PyObject *module, Py_complex z);

static PyValue
cmath_log10(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_log10_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_sin__doc__,
"sin($module, z, /)\n"
"--\n"
"\n"
"Return the sine of z.");

#define CMATH_SIN_METHODDEF    \
    {"sin", _PyCFunction_CAST(cmath_sin), METH_VALUE|METH_O, cmath_sin__doc__},

static Py_complex
cmath_sin_impl(PyObject *module, Py_complex z);

static PyValue
cmath_sin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_sin_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_sinh__doc__,
"sinh($module, z, /)\n"
"--\n"
"\n"
"Return the hyperbolic sine of z.");

#define CMATH_SINH_METHODDEF    \
    {"sinh", _PyCFunction_CAST(cmath_sinh), METH_VALUE|METH_O, cmath_sinh__doc__},

static Py_complex
cmath_sinh_impl(PyObject *module, Py_complex z);

static PyValue
cmath_sinh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_sinh_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_sqrt__doc__,
"sqrt($module, z, /)\n"
"--\n"
"\n"
"Return the square root of z.");

#define CMATH_SQRT_METHODDEF    \
    {"sqrt", _PyCFunction_CAST(cmath_sqrt), METH_VALUE|METH_O, cmath_sqrt__doc__},

static Py_complex
cmath_sqrt_impl(PyObject *module, Py_complex z);

static PyValue
cmath_sqrt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_sqrt_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_tan__doc__,
"tan($module, z, /)\n"
"--\n"
"\n"
"Return the tangent of z.");

#define CMATH_TAN_METHODDEF    \
    {"tan", _PyCFunction_CAST(cmath_tan), METH_VALUE|METH_O, cmath_tan__doc__},

static Py_complex
cmath_tan_impl(PyObject *module, Py_complex z);

static PyValue
cmath_tan(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_tan_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_tanh__doc__,
"tanh($module, z, /)\n"
"--\n"
"\n"
"Return the hyperbolic tangent of z.");

#define CMATH_TANH_METHODDEF    \
    {"tanh", _PyCFunction_CAST(cmath_tanh), METH_VALUE|METH_O, cmath_tanh__doc__},

static Py_complex
cmath_tanh_impl(PyObject *module, Py_complex z);

static PyValue
cmath_tanh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;
    Py_complex _return_value;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    /* modifications for z */
    errno = 0;
    _return_value = cmath_tanh_impl(PyValue_AsKnownObjectBorrow(module), z);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_log__doc__,
"log($module, z, base=<unrepresentable>, /)\n"
"--\n"
"\n"
"log(z[, base]) -> the logarithm of z to the given base.\n"
"\n"
"If the base is not specified, returns the natural logarithm (base e)\n"
"of z.");

#define CMATH_LOG_METHODDEF    \
    {"log", _PyCFunction_CAST(cmath_log), METH_VALUE, cmath_log__doc__},

static PyValue
cmath_log_impl(PyObject *module, Py_complex x, PyValueBorrow y_obj);

static PyValue
cmath_log(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex x;
    PyValueBorrow y_obj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("cmath.log()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("log", nargs, 1, 2)) {
        goto exit;
    }
    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        x = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        x.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        x.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    y_obj = PyValue_Borrow(args[1]);
skip_optional:
    return_value = cmath_log_impl(PyValue_AsKnownObjectBorrow(module), x, y_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_phase__doc__,
"phase($module, z, /)\n"
"--\n"
"\n"
"Return argument, also known as the phase angle, of a complex.");

#define CMATH_PHASE_METHODDEF    \
    {"phase", _PyCFunction_CAST(cmath_phase), METH_VALUE|METH_O, cmath_phase__doc__},

static PyValue
cmath_phase_impl(PyObject *module, Py_complex z);

static PyValue
cmath_phase(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_phase_impl(PyValue_AsKnownObjectBorrow(module), z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_polar__doc__,
"polar($module, z, /)\n"
"--\n"
"\n"
"Convert a complex from rectangular coordinates to polar coordinates.\n"
"\n"
"r is the distance from 0 and phi the phase angle.");

#define CMATH_POLAR_METHODDEF    \
    {"polar", _PyCFunction_CAST(cmath_polar), METH_VALUE|METH_O, cmath_polar__doc__},

static PyValue
cmath_polar_impl(PyObject *module, Py_complex z);

static PyValue
cmath_polar(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_polar_impl(PyValue_AsKnownObjectBorrow(module), z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_rect__doc__,
"rect($module, r, phi, /)\n"
"--\n"
"\n"
"Convert from polar coordinates to rectangular coordinates.");

#define CMATH_RECT_METHODDEF    \
    {"rect", _PyCFunction_CAST(cmath_rect), METH_VALUE, cmath_rect__doc__},

static Py_complex
cmath_rect_impl(PyObject *module, double r, double phi);

static PyValue
cmath_rect(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double r;
    double phi;
    Py_complex _return_value;

    if (!_PyArg_NoKwnamesValue("cmath.rect()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rect", nargs, 2, 2)) {
        goto exit;
    }
    r = _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (r == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    phi = _PyValue_AsDouble(PyValue_Borrow(args[1]));
    if (phi == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = cmath_rect_impl(PyValue_AsKnownObjectBorrow(module), r, phi);
    if (errno == EDOM) {
        PyErr_SetString(PyExc_ValueError, "math domain error");
        goto exit;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError, "math range error");
        goto exit;
    }
    else {
        return_value = _PyValue_FromObjectResult(PyComplex_FromCComplex(_return_value));
    }

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isfinite__doc__,
"isfinite($module, z, /)\n"
"--\n"
"\n"
"Return True if both the real and imaginary parts of z are finite, else False.");

#define CMATH_ISFINITE_METHODDEF    \
    {"isfinite", _PyCFunction_CAST(cmath_isfinite), METH_VALUE|METH_O, cmath_isfinite__doc__},

static PyValue
cmath_isfinite_impl(PyObject *module, Py_complex z);

static PyValue
cmath_isfinite(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_isfinite_impl(PyValue_AsKnownObjectBorrow(module), z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isnan__doc__,
"isnan($module, z, /)\n"
"--\n"
"\n"
"Checks if the real or imaginary part of z not a number (NaN).");

#define CMATH_ISNAN_METHODDEF    \
    {"isnan", _PyCFunction_CAST(cmath_isnan), METH_VALUE|METH_O, cmath_isnan__doc__},

static PyValue
cmath_isnan_impl(PyObject *module, Py_complex z);

static PyValue
cmath_isnan(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_isnan_impl(PyValue_AsKnownObjectBorrow(module), z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isinf__doc__,
"isinf($module, z, /)\n"
"--\n"
"\n"
"Checks if the real or imaginary part of z is infinite.");

#define CMATH_ISINF_METHODDEF    \
    {"isinf", _PyCFunction_CAST(cmath_isinf), METH_VALUE|METH_O, cmath_isinf__doc__},

static PyValue
cmath_isinf_impl(PyObject *module, Py_complex z);

static PyValue
cmath_isinf(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_complex z;

    if (PyValue_IsObject(PyValue_Borrow(args[0]))) {
        z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    }
    else {
        z.real = _PyValue_AsDouble(PyValue_Borrow(args[0]));
        z.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = cmath_isinf_impl(PyValue_AsKnownObjectBorrow(module), z);

exit:
    return return_value;
}

PyDoc_STRVAR(cmath_isclose__doc__,
"isclose($module, /, a, b, *, rel_tol=1e-09, abs_tol=0.0)\n"
"--\n"
"\n"
"Determine whether two complex numbers are close in value.\n"
"\n"
"  rel_tol\n"
"    maximum difference for being considered \"close\", relative to the\n"
"    magnitude of the input values\n"
"  abs_tol\n"
"    maximum difference for being considered \"close\", regardless of the\n"
"    magnitude of the input values\n"
"\n"
"Return True if a is close in value to b, and False otherwise.\n"
"\n"
"For the values to be considered close, the difference between them must\n"
"be smaller than at least one of the tolerances.\n"
"\n"
"-inf, inf and NaN behave similarly to the IEEE 754 Standard.  That is,\n"
"NaN is not close to anything, even itself. inf and -inf are only close\n"
"to themselves.");

#define CMATH_ISCLOSE_METHODDEF    \
    {"isclose", _PyCFunction_CAST(cmath_isclose), METH_VALUE, cmath_isclose__doc__},

static int
cmath_isclose_impl(PyObject *module, Py_complex a, Py_complex b,
                   double rel_tol, double abs_tol);

static PyValue
cmath_isclose(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), _PyValue_STATIC_IMMORTAL(&_Py_ID(rel_tol)), _PyValue_STATIC_IMMORTAL(&_Py_ID(abs_tol)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "rel_tol", "abs_tol", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "isclose",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    Py_complex a;
    Py_complex b;
    double rel_tol = 1e-09;
    double abs_tol = 0.0;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        a = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    }
    else {
        a.real = _PyValue_AsDouble(PyValue_Borrow(args[_idx[0]]));
        a.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        b = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
    }
    else {
        b.real = _PyValue_AsDouble(PyValue_Borrow(args[_idx[1]]));
        b.imag = 0.0;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        rel_tol = _PyValue_AsDouble(PyValue_Borrow(args[_idx[2]]));
        if (rel_tol == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        abs_tol = _PyValue_AsDouble(PyValue_Borrow(args[_idx[3]]));
        if (abs_tol == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = cmath_isclose_impl(PyValue_AsKnownObjectBorrow(module), a, b, rel_tol, abs_tol);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}
/*[clinic end generated code: output=53ba5adf965bc917 input=a9049054013a1b77]*/
