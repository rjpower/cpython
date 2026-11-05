/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

static PyValue
pysqlite_row_new_impl(PyTypeObject *type, pysqlite_Cursor *cursor,
                      PyObject *data);

static PyValue
pysqlite_row_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->RowType;
    pysqlite_Cursor *cursor;
    PyObject *data;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("Row", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Row", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->CursorType)) {
        _PyArg_BadArgumentValue("Row", "argument 1", (clinic_state()->CursorType)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    cursor = (pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("Row", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    data = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = pysqlite_row_new_impl(type, cursor, data);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
pysqlite_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(pysqlite_row_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = pysqlite_row_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(pysqlite_row_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Returns the keys of the row.");

#define PYSQLITE_ROW_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(pysqlite_row_keys), METH_VALUE|METH_NOARGS, pysqlite_row_keys__doc__},

static PyValue
pysqlite_row_keys_impl(pysqlite_Row *self);

static PyValue
pysqlite_row_keys(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_row_keys_impl((pysqlite_Row *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=3b3ba7b5d20c5f61 input=a9049054013a1b77]*/
