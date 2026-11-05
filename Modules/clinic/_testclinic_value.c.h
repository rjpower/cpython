/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_runtime.h"       // _Py_SINGLETON()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(object_converter__doc__,
"object_converter($module, a, /)\n"
"--\n"
"\n");

#define OBJECT_CONVERTER_METHODDEF    \
    {"object_converter", _PyCFunction_CAST(object_converter), METH_VALUE, object_converter__doc__},

static PyValue
object_converter_impl(PyObject *module, PyValueBorrow a);

static PyValue
object_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("_testclinic_value.object_converter()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = object_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(objects_converter__doc__,
"objects_converter($module, a, b=None, /)\n"
"--\n"
"\n");

#define OBJECTS_CONVERTER_METHODDEF    \
    {"objects_converter", _PyCFunction_CAST(objects_converter), METH_VALUE, objects_converter__doc__},

static PyValue
objects_converter_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
objects_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("_testclinic_value.objects_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("objects_converter", nargs, 1, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyValue_Borrow(args[1]);
skip_optional:
    return_value = objects_converter_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(int_converter__doc__,
"int_converter($module, a=12, b=34, /)\n"
"--\n"
"\n");

#define INT_CONVERTER_METHODDEF    \
    {"int_converter", _PyCFunction_CAST(int_converter), METH_VALUE, int_converter__doc__},

static PyValue
int_converter_impl(PyObject *module, int a, int b);

static PyValue
int_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a = 12;
    int b = 34;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.int_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("int_converter", nargs, 0, 2)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = int_converter_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(long_converter__doc__,
"long_converter($module, a=12, /)\n"
"--\n"
"\n");

#define LONG_CONVERTER_METHODDEF    \
    {"long_converter", _PyCFunction_CAST(long_converter), METH_VALUE, long_converter__doc__},

static PyValue
long_converter_impl(PyObject *module, long a);

static PyValue
long_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long a = 12;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.long_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("long_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = _PyValue_AsLong(PyValue_Borrow(args[0]));
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = long_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(py_ssize_t_converter__doc__,
"py_ssize_t_converter($module, a=12, b=56, c=78, d=90, /)\n"
"--\n"
"\n");

#define PY_SSIZE_T_CONVERTER_METHODDEF    \
    {"py_ssize_t_converter", _PyCFunction_CAST(py_ssize_t_converter), METH_VALUE, py_ssize_t_converter__doc__},

static PyValue
py_ssize_t_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                          Py_ssize_t c, Py_ssize_t d);

static PyValue
py_ssize_t_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a = 12;
    Py_ssize_t b = 56;
    Py_ssize_t c = 78;
    Py_ssize_t d = 90;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.py_ssize_t_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("py_ssize_t_converter", nargs, 0, 4)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        a = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[1]), &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[2]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        c = ival;
        if (c < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "c cannot be negative");
            goto exit;
        }
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_non_negative_ssize_t_value(PyValue_Borrow(args[3]), &d)) {
        goto exit;
    }
skip_optional:
    return_value = py_ssize_t_converter_impl(PyValue_AsKnownObjectBorrow(module), a, b, c, d);

exit:
    return return_value;
}

PyDoc_STRVAR(slice_index_converter__doc__,
"slice_index_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define SLICE_INDEX_CONVERTER_METHODDEF    \
    {"slice_index_converter", _PyCFunction_CAST(slice_index_converter), METH_VALUE, slice_index_converter__doc__},

static PyValue
slice_index_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                           Py_ssize_t c);

static PyValue
slice_index_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a = 12;
    Py_ssize_t b = 34;
    Py_ssize_t c = 56;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.slice_index_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("slice_index_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[0]), &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexNotNoneValue(PyValue_Borrow(args[1]), &b)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &c)) {
        goto exit;
    }
skip_optional:
    return_value = slice_index_converter_impl(PyValue_AsKnownObjectBorrow(module), a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(bool_converter__doc__,
"bool_converter($module, a=True, b=True, /)\n"
"--\n"
"\n");

#define BOOL_CONVERTER_METHODDEF    \
    {"bool_converter", _PyCFunction_CAST(bool_converter), METH_VALUE, bool_converter__doc__},

static PyValue
bool_converter_impl(PyObject *module, int a, int b);

static PyValue
bool_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a = 1;
    int b = 1;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.bool_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("bool_converter", nargs, 0, 2)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (a < 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = bool_converter_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(double_converter__doc__,
"double_converter($module, a=12.5, /)\n"
"--\n"
"\n");

#define DOUBLE_CONVERTER_METHODDEF    \
    {"double_converter", _PyCFunction_CAST(double_converter), METH_VALUE, double_converter__doc__},

static PyValue
double_converter_impl(PyObject *module, double a);

static PyValue
double_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double a = 12.5;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.double_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("double_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (a == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = double_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_converter__doc__,
"unicode_converter($module, a, /)\n"
"--\n"
"\n");

#define UNICODE_CONVERTER_METHODDEF    \
    {"unicode_converter", _PyCFunction_CAST(unicode_converter), METH_VALUE, unicode_converter__doc__},

static PyValue
unicode_converter_impl(PyObject *module, PyObject *a);

static PyValue
unicode_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;

    if (!_PyArg_CheckArityValue("_testclinic_value.unicode_converter()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("unicode_converter", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("unicode_converter", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    a = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = unicode_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(str_converter__doc__,
"str_converter($module, a=\'a\', /)\n"
"--\n"
"\n");

#define STR_CONVERTER_METHODDEF    \
    {"str_converter", _PyCFunction_CAST(str_converter), METH_VALUE, str_converter__doc__},

static PyValue
str_converter_impl(PyObject *module, const char *a);

static PyValue
str_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *a = "a";

    if (!_PyArg_NoKwnamesValue("_testclinic_value.str_converter()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("str_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("str_converter", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("str_converter", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    a = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (a == NULL) {
        goto exit;
    }
skip_optional:
    return_value = str_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(str_none_converter__doc__,
"str_none_converter($module, a, /)\n"
"--\n"
"\n");

#define STR_NONE_CONVERTER_METHODDEF    \
    {"str_none_converter", _PyCFunction_CAST(str_none_converter), METH_VALUE, str_none_converter__doc__},

static PyValue
str_none_converter_impl(PyObject *module, const char *a);

static PyValue
str_none_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *a;

    if (!_PyArg_CheckArityValue("_testclinic_value.str_none_converter()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("str_none_converter", "argument", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        a = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        a = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (a == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("str_none_converter", "argument", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    return_value = str_none_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(py_buffer_converter__doc__,
"py_buffer_converter($module, a, /)\n"
"--\n"
"\n");

#define PY_BUFFER_CONVERTER_METHODDEF    \
    {"py_buffer_converter", _PyCFunction_CAST(py_buffer_converter), METH_VALUE, py_buffer_converter__doc__},

static PyValue
py_buffer_converter_impl(PyObject *module, Py_buffer *a);

static PyValue
py_buffer_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer a = {NULL, NULL};

    if (!_PyArg_CheckArityValue("_testclinic_value.py_buffer_converter()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &a, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = py_buffer_converter_impl(PyValue_AsKnownObjectBorrow(module), &a);

exit:
    /* Cleanup for a */
    if (a.obj) {
       PyBuffer_Release(&a);
    }

    return return_value;
}

PyDoc_STRVAR(subclass_tuple__doc__,
"subclass_tuple($module, a, /)\n"
"--\n"
"\n");

#define SUBCLASS_TUPLE_METHODDEF    \
    {"subclass_tuple", _PyCFunction_CAST(subclass_tuple), METH_VALUE, subclass_tuple__doc__},

static PyValue
subclass_tuple_impl(PyObject *module, PyObject *a);

static PyValue
subclass_tuple(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;

    if (!_PyArg_CheckArityValue("_testclinic_value.subclass_tuple()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("subclass_tuple", "argument", "tuple", PyValue_Borrow(args[0]));
        goto exit;
    }
    a = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = subclass_tuple_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(subclass_long__doc__,
"subclass_long($module, a, /)\n"
"--\n"
"\n");

#define SUBCLASS_LONG_METHODDEF    \
    {"subclass_long", _PyCFunction_CAST(subclass_long), METH_VALUE, subclass_long__doc__},

static PyValue
subclass_long_impl(PyObject *module, PyValueBorrow a);

static PyValue
subclass_long(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("_testclinic_value.subclass_long()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyLong_Type)) {
        _PyArg_BadArgumentValue("subclass_long", "argument", "int", PyValue_Borrow(args[0]));
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = subclass_long_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

PyDoc_STRVAR(ret_bool__doc__,
"ret_bool($module, a, /)\n"
"--\n"
"\n");

#define RET_BOOL_METHODDEF    \
    {"ret_bool", _PyCFunction_CAST(ret_bool), METH_VALUE, ret_bool__doc__},

static int
ret_bool_impl(PyObject *module, int a);

static PyValue
ret_bool(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a;
    int _return_value;

    if (!_PyArg_CheckArityValue("_testclinic_value.ret_bool()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (a < 0) {
        goto exit;
    }
    _return_value = ret_bool_impl(PyValue_AsKnownObjectBorrow(module), a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(ret_int__doc__,
"ret_int($module, a, /)\n"
"--\n"
"\n");

#define RET_INT_METHODDEF    \
    {"ret_int", _PyCFunction_CAST(ret_int), METH_VALUE, ret_int__doc__},

static int
ret_int_impl(PyObject *module, int a);

static PyValue
ret_int(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a;
    int _return_value;

    if (!_PyArg_CheckArityValue("_testclinic_value.ret_int()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = ret_int_impl(PyValue_AsKnownObjectBorrow(module), a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(ret_ssize_t__doc__,
"ret_ssize_t($module, a, /)\n"
"--\n"
"\n");

#define RET_SSIZE_T_METHODDEF    \
    {"ret_ssize_t", _PyCFunction_CAST(ret_ssize_t), METH_VALUE, ret_ssize_t__doc__},

static Py_ssize_t
ret_ssize_t_impl(PyObject *module, Py_ssize_t a);

static PyValue
ret_ssize_t(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckArityValue("_testclinic_value.ret_ssize_t()", nargs, kwnames, 1)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        a = ival;
    }
    _return_value = ret_ssize_t_impl(PyValue_AsKnownObjectBorrow(module), a);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(ret_double__doc__,
"ret_double($module, a, /)\n"
"--\n"
"\n");

#define RET_DOUBLE_METHODDEF    \
    {"ret_double", _PyCFunction_CAST(ret_double), METH_VALUE, ret_double__doc__},

static double
ret_double_impl(PyObject *module, double a);

static PyValue
ret_double(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double a;
    double _return_value;

    if (!_PyArg_CheckArityValue("_testclinic_value.ret_double()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (a == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = ret_double_impl(PyValue_AsKnownObjectBorrow(module), a);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(test_empty_function__doc__,
"test_empty_function($module, /)\n"
"--\n"
"\n");

#define TEST_EMPTY_FUNCTION_METHODDEF    \
    {"test_empty_function", _PyCFunction_CAST(test_empty_function), METH_VALUE, test_empty_function__doc__},

static PyValue
test_empty_function_impl(PyObject *module);

static PyValue
test_empty_function(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("_testclinic_value.test_empty_function()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = test_empty_function_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(keywords__doc__,
"keywords($module, /, a, b)\n"
"--\n"
"\n");

#define KEYWORDS_METHODDEF    \
    {"keywords", _PyCFunction_CAST(keywords), METH_VALUE, keywords__doc__},

static PyValue
keywords_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
keywords(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    b = PyValue_Borrow(args[_idx[1]]);
    return_value = keywords_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_kwonly__doc__,
"keywords_kwonly($module, /, a, *, b)\n"
"--\n"
"\n");

#define KEYWORDS_KWONLY_METHODDEF    \
    {"keywords_kwonly", _PyCFunction_CAST(keywords_kwonly), METH_VALUE, keywords_kwonly__doc__},

static PyValue
keywords_kwonly_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
keywords_kwonly(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    b = PyValue_Borrow(args[_idx[1]]);
    return_value = keywords_kwonly_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_opt__doc__,
"keywords_opt($module, /, a, b=None, c=None)\n"
"--\n"
"\n");

#define KEYWORDS_OPT_METHODDEF    \
    {"keywords_opt", _PyCFunction_CAST(keywords_opt), METH_VALUE, keywords_opt__doc__},

static PyValue
keywords_opt_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
                  PyValueBorrow c);

static PyValue
keywords_opt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('c')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow a;
    PyValueBorrow b = PyValue_Borrow(PyValue_None);
    PyValueBorrow c = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        c = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = keywords_opt_impl(PyValue_AsKnownObjectBorrow(module), a, b, c);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords__doc__,
"posonly_keywords($module, a, /, b)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_METHODDEF    \
    {"posonly_keywords", _PyCFunction_CAST(posonly_keywords), METH_VALUE, posonly_keywords__doc__},

static PyValue
posonly_keywords_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b);

static PyValue
posonly_keywords(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_keywords",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    b = PyValue_Borrow(args[_idx[1]]);
    return_value = posonly_keywords_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(varpos__doc__,
"varpos($module, /, *args)\n"
"--\n"
"\n");

#define VARPOS_METHODDEF    \
    {"varpos", _PyCFunction_CAST(varpos), METH_VALUE, varpos__doc__},

static PyValue
varpos_impl(PyObject *module, PyObject *args);

static PyValue
varpos(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.varpos()", kwnames)) {
        goto exit;
    }
    __clinic_args = _PyTuple_FromValueArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = varpos_impl(PyValue_AsKnownObjectBorrow(module), __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(varpos_array__doc__,
"varpos_array($module, /, *args)\n"
"--\n"
"\n");

#define VARPOS_ARRAY_METHODDEF    \
    {"varpos_array", _PyCFunction_CAST(varpos_array), METH_VALUE, varpos_array__doc__},

static PyValue
varpos_array_impl(PyObject *module, const PyValue *args,
                  Py_ssize_t args_length);

static PyValue
varpos_array(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    if (!_PyArg_NoKwnamesValue("_testclinic_value.varpos_array()", kwnames)) {
        goto exit;
    }
    __clinic_args = args;
    args_length = nargs;
    return_value = varpos_array_impl(PyValue_AsKnownObjectBorrow(module), __clinic_args, args_length);

exit:
    return return_value;
}

PyDoc_STRVAR(kwds_with_pos_only__doc__,
"kwds_with_pos_only($module, a, b, /, **kwds)\n"
"--\n"
"\n");

#define KWDS_WITH_POS_ONLY_METHODDEF    \
    {"kwds_with_pos_only", _PyCFunction_CAST(kwds_with_pos_only), METH_VALUE, kwds_with_pos_only__doc__},

static PyValue
kwds_with_pos_only_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
                        PyObject *kwds);

static PyValue
kwds_with_pos_only(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;
    PyObject *__clinic_kwds = NULL;

    if (!_PyArg_CheckPositional("kwds_with_pos_only", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    __clinic_kwds = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (__clinic_kwds == NULL) {
        goto exit;
    }
    return_value = kwds_with_pos_only_impl(PyValue_AsKnownObjectBorrow(module), a, b, __clinic_kwds);

exit:
    /* Cleanup for kwds */
    Py_XDECREF(__clinic_kwds);

    return return_value;
}

static PyValue
_testclinic_value_Pair_impl(PyTypeObject *type, PyValueBorrow a,
                            PyValueBorrow b);

static PyValue
_testclinic_value_Pair_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Pair",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    PyValueBorrow b = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _testclinic_value_Pair_impl(type, a, b);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_testclinic_value_Pair(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_testclinic_value_Pair_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _testclinic_value_Pair_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
_testclinic_value_Pair_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == _testclinic_value_Pair);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    return _testclinic_value_Pair_value(_PyType_CAST(type), args, nargs, kwnames);
}

PyDoc_STRVAR(_testclinic_value_Pair_get__doc__,
"get($self, /)\n"
"--\n"
"\n");

#define _TESTCLINIC_VALUE_PAIR_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_testclinic_value_Pair_get), METH_VALUE, _testclinic_value_Pair_get__doc__},

static PyValue
_testclinic_value_Pair_get_impl(PairObject *self);

static PyValue
_testclinic_value_Pair_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&Pair_Type, "get", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = _testclinic_value_Pair_get_impl((PairObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

static int
_testclinic_value_Cell___init___impl(PairObject *self, PyValueBorrow a,
                                     PyValueBorrow b);

static int
_testclinic_value_Cell___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Cell",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    PyValueBorrow b = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _testclinic_value_Cell___init___impl((PairObject *)self, a, b);

exit:
    return return_value;
}

static int
_testclinic_value_Cell___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _testclinic_value_Cell___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _testclinic_value_Cell___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

static PyValue
_testclinic_value_Cell_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_init == _testclinic_value_Cell___init__);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        return PyValue_NULL;
    }
    if (_testclinic_value_Cell___init___value(self, args, nargs, kwnames) != 0) {
        Py_DECREF(self);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(self);
}

PyDoc_STRVAR(_testclinic_value_Probe_where__doc__,
"where($self, /)\n"
"--\n"
"\n");

#define _TESTCLINIC_VALUE_PROBE_WHERE_METHODDEF    \
    {"where", _PyCFunction_CAST(_testclinic_value_Probe_where), METH_VALUE|METH_METHOD, _testclinic_value_Probe_where__doc__},

static PyValue
_testclinic_value_Probe_where_impl(PyObject *self, PyTypeObject *cls);

static PyValue
_testclinic_value_Probe_where(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "where() takes no arguments");
        goto exit;
    }
    return_value = _testclinic_value_Probe_where_impl(PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}
/*[clinic end generated code: output=9bb35f21f2a59f80 input=a9049054013a1b77]*/
