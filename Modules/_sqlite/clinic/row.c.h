/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_borrow.h"        // _PyBorrow_TupleItems()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static PyObject *
pysqlite_row_new_impl(PyTypeObject *type, pysqlite_Cursor *cursor,
                      PyObject *data);

static PyObject *
pysqlite_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->RowType;
    pysqlite_Cursor *cursor;
    PyObject *data;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("Row", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Row", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    PyObject *const *_args_items = _PyBorrow_TupleItems(args, PyTuple_GET_SIZE(args));
    if (_args_items == NULL) {
        goto exit;
    }
    if (!PyObject_TypeCheck(_args_items[0], clinic_state()->CursorType)) {
        _PyArg_BadArgument("Row", "argument 1", (clinic_state()->CursorType)->tp_name, _args_items[0]);
        goto exit;
    }
    cursor = (pysqlite_Cursor *)_args_items[0];
    if (!PyTuple_Check(_args_items[1])) {
        _PyArg_BadArgument("Row", "argument 2", "tuple", _args_items[1]);
        goto exit;
    }
    data = _args_items[1];
    return_value = pysqlite_row_new_impl(type, cursor, data);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_row_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Returns the keys of the row.");

#define PYSQLITE_ROW_KEYS_METHODDEF    \
    {"keys", (PyCFunction)pysqlite_row_keys, METH_NOARGS, pysqlite_row_keys__doc__},

static PyObject *
pysqlite_row_keys_impl(pysqlite_Row *self);

static PyObject *
pysqlite_row_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_row_keys_impl((pysqlite_Row *)self);
}
/*[clinic end generated code: output=0a25b36bb24c447f input=a9049054013a1b77]*/
