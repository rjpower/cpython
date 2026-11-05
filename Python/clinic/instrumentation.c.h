/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(monitoring_use_tool_id__doc__,
"use_tool_id($module, tool_id, name, /)\n"
"--\n"
"\n");

#define MONITORING_USE_TOOL_ID_METHODDEF    \
    {"use_tool_id", _PyCFunction_CAST(monitoring_use_tool_id), METH_VALUE, monitoring_use_tool_id__doc__},

static PyValue
monitoring_use_tool_id_impl(PyObject *module, int tool_id,
                            PyValueBorrow name);

static PyValue
monitoring_use_tool_id(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;
    PyValueBorrow name;

    if (!_PyArg_NoKwnamesValue("monitoring.use_tool_id()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("use_tool_id", nargs, 2, 2)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    name = PyValue_Borrow(args[1]);
    return_value = monitoring_use_tool_id_impl(PyValue_AsKnownObjectBorrow(module), tool_id, name);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_clear_tool_id__doc__,
"clear_tool_id($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_CLEAR_TOOL_ID_METHODDEF    \
    {"clear_tool_id", _PyCFunction_CAST(monitoring_clear_tool_id), METH_VALUE, monitoring_clear_tool_id__doc__},

static PyValue
monitoring_clear_tool_id_impl(PyObject *module, int tool_id);

static PyValue
monitoring_clear_tool_id(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;

    if (!_PyArg_CheckArityValue("monitoring.clear_tool_id()", nargs, kwnames, 1)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_clear_tool_id_impl(PyValue_AsKnownObjectBorrow(module), tool_id);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_free_tool_id__doc__,
"free_tool_id($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_FREE_TOOL_ID_METHODDEF    \
    {"free_tool_id", _PyCFunction_CAST(monitoring_free_tool_id), METH_VALUE, monitoring_free_tool_id__doc__},

static PyValue
monitoring_free_tool_id_impl(PyObject *module, int tool_id);

static PyValue
monitoring_free_tool_id(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;

    if (!_PyArg_CheckArityValue("monitoring.free_tool_id()", nargs, kwnames, 1)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_free_tool_id_impl(PyValue_AsKnownObjectBorrow(module), tool_id);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_get_tool__doc__,
"get_tool($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_GET_TOOL_METHODDEF    \
    {"get_tool", _PyCFunction_CAST(monitoring_get_tool), METH_VALUE, monitoring_get_tool__doc__},

static PyValue
monitoring_get_tool_impl(PyObject *module, int tool_id);

static PyValue
monitoring_get_tool(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;

    if (!_PyArg_CheckArityValue("monitoring.get_tool()", nargs, kwnames, 1)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_get_tool_impl(PyValue_AsKnownObjectBorrow(module), tool_id);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_register_callback__doc__,
"register_callback($module, tool_id, event, func, /)\n"
"--\n"
"\n");

#define MONITORING_REGISTER_CALLBACK_METHODDEF    \
    {"register_callback", _PyCFunction_CAST(monitoring_register_callback), METH_VALUE, monitoring_register_callback__doc__},

static PyValue
monitoring_register_callback_impl(PyObject *module, int tool_id, int event,
                                  PyValueBorrow func);

static PyValue
monitoring_register_callback(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;
    int event;
    PyValueBorrow func;

    if (!_PyArg_NoKwnamesValue("monitoring.register_callback()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("register_callback", nargs, 3, 3)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    event = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (event == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Borrow(args[2]);
    return_value = monitoring_register_callback_impl(PyValue_AsKnownObjectBorrow(module), tool_id, event, func);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_get_events__doc__,
"get_events($module, tool_id, /)\n"
"--\n"
"\n");

#define MONITORING_GET_EVENTS_METHODDEF    \
    {"get_events", _PyCFunction_CAST(monitoring_get_events), METH_VALUE, monitoring_get_events__doc__},

static int
monitoring_get_events_impl(PyObject *module, int tool_id);

static PyValue
monitoring_get_events(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;
    int _return_value;

    if (!_PyArg_CheckArityValue("monitoring.get_events()", nargs, kwnames, 1)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = monitoring_get_events_impl(PyValue_AsKnownObjectBorrow(module), tool_id);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_set_events__doc__,
"set_events($module, tool_id, event_set, /)\n"
"--\n"
"\n");

#define MONITORING_SET_EVENTS_METHODDEF    \
    {"set_events", _PyCFunction_CAST(monitoring_set_events), METH_VALUE, monitoring_set_events__doc__},

static PyValue
monitoring_set_events_impl(PyObject *module, int tool_id, int event_set);

static PyValue
monitoring_set_events(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;
    int event_set;

    if (!_PyArg_NoKwnamesValue("monitoring.set_events()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_events", nargs, 2, 2)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    event_set = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (event_set == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_set_events_impl(PyValue_AsKnownObjectBorrow(module), tool_id, event_set);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_get_local_events__doc__,
"get_local_events($module, tool_id, code, /)\n"
"--\n"
"\n");

#define MONITORING_GET_LOCAL_EVENTS_METHODDEF    \
    {"get_local_events", _PyCFunction_CAST(monitoring_get_local_events), METH_VALUE, monitoring_get_local_events__doc__},

static int
monitoring_get_local_events_impl(PyObject *module, int tool_id,
                                 PyValueBorrow code);

static PyValue
monitoring_get_local_events(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;
    PyValueBorrow code;
    int _return_value;

    if (!_PyArg_NoKwnamesValue("monitoring.get_local_events()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("get_local_events", nargs, 2, 2)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    code = PyValue_Borrow(args[1]);
    _return_value = monitoring_get_local_events_impl(PyValue_AsKnownObjectBorrow(module), tool_id, code);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_set_local_events__doc__,
"set_local_events($module, tool_id, code, event_set, /)\n"
"--\n"
"\n");

#define MONITORING_SET_LOCAL_EVENTS_METHODDEF    \
    {"set_local_events", _PyCFunction_CAST(monitoring_set_local_events), METH_VALUE, monitoring_set_local_events__doc__},

static PyValue
monitoring_set_local_events_impl(PyObject *module, int tool_id,
                                 PyValueBorrow code, int event_set);

static PyValue
monitoring_set_local_events(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int tool_id;
    PyValueBorrow code;
    int event_set;

    if (!_PyArg_NoKwnamesValue("monitoring.set_local_events()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_local_events", nargs, 3, 3)) {
        goto exit;
    }
    tool_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (tool_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    code = PyValue_Borrow(args[1]);
    event_set = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (event_set == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = monitoring_set_local_events_impl(PyValue_AsKnownObjectBorrow(module), tool_id, code, event_set);

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring_restart_events__doc__,
"restart_events($module, /)\n"
"--\n"
"\n");

#define MONITORING_RESTART_EVENTS_METHODDEF    \
    {"restart_events", _PyCFunction_CAST(monitoring_restart_events), METH_VALUE, monitoring_restart_events__doc__},

static PyValue
monitoring_restart_events_impl(PyObject *module);

static PyValue
monitoring_restart_events(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("monitoring.restart_events()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = monitoring_restart_events_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(monitoring__all_events__doc__,
"_all_events($module, /)\n"
"--\n"
"\n");

#define MONITORING__ALL_EVENTS_METHODDEF    \
    {"_all_events", _PyCFunction_CAST(monitoring__all_events), METH_VALUE, monitoring__all_events__doc__},

static PyValue
monitoring__all_events_impl(PyObject *module);

static PyValue
monitoring__all_events(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("monitoring._all_events()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = monitoring__all_events_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}
/*[clinic end generated code: output=f56d42624617d0bc input=a9049054013a1b77]*/
