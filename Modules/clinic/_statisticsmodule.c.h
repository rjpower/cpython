/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_statistics__normal_dist_inv_cdf__doc__,
"_normal_dist_inv_cdf($module, p, mu, sigma, /)\n"
"--\n"
"\n");

#define _STATISTICS__NORMAL_DIST_INV_CDF_METHODDEF    \
    {"_normal_dist_inv_cdf", _PyCFunction_CAST(_statistics__normal_dist_inv_cdf), METH_FASTCALL, _statistics__normal_dist_inv_cdf__doc__},

static double
_statistics__normal_dist_inv_cdf_impl(PyObject *module, double p, double mu,
                                      double sigma);

static PyValue
_statistics__normal_dist_inv_cdf(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    double p;
    double mu;
    double sigma;
    double _return_value;

    if (!_PyArg_CheckPositional("_normal_dist_inv_cdf", nargs, 3, 3)) {
        goto exit;
    }
    p = PyFloat_AsDoubleValue(args[0]);
    if (p == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    mu = PyFloat_AsDoubleValue(args[1]);
    if (mu == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    sigma = PyFloat_AsDoubleValue(args[2]);
    if (sigma == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _statistics__normal_dist_inv_cdf_impl(module, p, mu, sigma);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=1d8cc0151cf54419 input=a9049054013a1b77]*/
