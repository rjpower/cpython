/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value.h"         // _PyValue_RestoreException()

static PyObject *
pysqlite_row_new_impl(PyTypeObject *type, pysqlite_Cursor *cursor,
                      PyObject *data);

static PyObject *
pysqlite_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->RowType;
    _PyArg_OwnerScope _args_scope = {0};
    PyObject *const *fastargs;
    pysqlite_Cursor *cursor;
    PyObject *data;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("Row", kwargs)) {
        goto exit;
    }
    if (_PyArg_OwnerScope_BeginTuple(&_args_scope, args, 0) < 0) {
        goto exit;
    }
    fastargs = _PyArg_UnpackPositionalOwned(&_args_scope, "Row",
                                          2, 2);
    if (fastargs == NULL) {
        goto exit;
    }
    if (!PyObject_TypeCheck(fastargs[0], clinic_state()->CursorType)) {
        _PyArg_BadArgument("Row", "argument 1", (clinic_state()->CursorType)->tp_name, fastargs[0]);
        goto exit;
    }
    cursor = (pysqlite_Cursor *)fastargs[0];
    if (!PyTuple_Check(fastargs[1])) {
        _PyArg_BadArgument("Row", "argument 2", "tuple", fastargs[1]);
        goto exit;
    }
    data = fastargs[1];
    return_value = pysqlite_row_new_impl(type, cursor, data);

exit:
    if (_PyArg_OwnerScope_End(&_args_scope) < 0) {
        PyObject *_saved = PyErr_GetRaisedException();
        Py_CLEAR(return_value);
        _PyValue_RestoreException(_saved);
    }
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
/*[clinic end generated code: output=782547a4c52986fa input=a9049054013a1b77]*/
