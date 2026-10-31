/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(test_empty_function__doc__,
"test_empty_function($module, /)\n"
"--\n"
"\n");

#define TEST_EMPTY_FUNCTION_METHODDEF    \
    {"test_empty_function", (PyCFunction)test_empty_function, METH_NOARGS, test_empty_function__doc__},

static PyObject *
test_empty_function_impl(PyObject *module);

static PyObject *
test_empty_function(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return test_empty_function_impl(module);
}

PyDoc_STRVAR(my_int_func__doc__,
"my_int_func($module, arg, /)\n"
"--\n"
"\n");

#define MY_INT_FUNC_METHODDEF    \
    {"my_int_func", _PyCFunction_CAST(my_int_func), METH_O, my_int_func__doc__},

static int
my_int_func_impl(PyObject *module, int arg);

static PyValue
my_int_func(PyObject *module, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    int arg;
    int _return_value;

    arg = PyLong_AsIntValue(arg_);
    if (arg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_int_func_impl(module, arg);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(my_int_sum__doc__,
"my_int_sum($module, x, y, /)\n"
"--\n"
"\n");

#define MY_INT_SUM_METHODDEF    \
    {"my_int_sum", _PyCFunction_CAST(my_int_sum), METH_FASTCALL, my_int_sum__doc__},

static int
my_int_sum_impl(PyObject *module, int x, int y);

static PyValue
my_int_sum(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int x;
    int y;
    int _return_value;

    if (!_PyArg_CheckPositional("my_int_sum", nargs, 2, 2)) {
        goto exit;
    }
    x = PyLong_AsIntValue(args[0]);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    y = PyLong_AsIntValue(args[1]);
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_int_sum_impl(module, x, y);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(my_float_sum__doc__,
"my_float_sum($module, x, y, /)\n"
"--\n"
"\n");

#define MY_FLOAT_SUM_METHODDEF    \
    {"my_float_sum", _PyCFunction_CAST(my_float_sum), METH_FASTCALL, my_float_sum__doc__},

static float
my_float_sum_impl(PyObject *module, float x, float y);

static PyValue
my_float_sum(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    float x;
    float y;
    float _return_value;

    if (!_PyArg_CheckPositional("my_float_sum", nargs, 2, 2)) {
        goto exit;
    }
    x = (float) PyFloat_AsDoubleValue(args[0]);
    if (x == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    y = (float) PyFloat_AsDoubleValue(args[1]);
    if (y == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_float_sum_impl(module, x, y);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble((double)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(my_double_sum__doc__,
"my_double_sum($module, x, y, /)\n"
"--\n"
"\n");

#define MY_DOUBLE_SUM_METHODDEF    \
    {"my_double_sum", _PyCFunction_CAST(my_double_sum), METH_FASTCALL, my_double_sum__doc__},

static double
my_double_sum_impl(PyObject *module, double x, double y);

static PyValue
my_double_sum(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    double x;
    double y;
    double _return_value;

    if (!_PyArg_CheckPositional("my_double_sum", nargs, 2, 2)) {
        goto exit;
    }
    x = PyFloat_AsDoubleValue(args[0]);
    if (x == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    y = PyFloat_AsDoubleValue(args[1]);
    if (y == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = my_double_sum_impl(module, x, y);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(get_file_descriptor__doc__,
"get_file_descriptor($module, file, /)\n"
"--\n"
"\n"
"Get a file descriptor.");

#define GET_FILE_DESCRIPTOR_METHODDEF    \
    {"get_file_descriptor", _PyCFunction_CAST(get_file_descriptor), METH_O, get_file_descriptor__doc__},

static int
get_file_descriptor_impl(PyObject *module, int fd);

static PyValue
get_file_descriptor(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_arg);
        if (fd < 0) {
            goto exit;
        }
    }
    _return_value = get_file_descriptor_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=5a8d05658bc902db input=a9049054013a1b77]*/
