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
#include "pycore_long_value.h"    // _PyValue_AsNativeBytes()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

static int
pysqlite_cursor_init_impl(pysqlite_Cursor *self,
                          pysqlite_Connection *connection);

static int
pysqlite_cursor_init_value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = clinic_state()->CursorType;
    pysqlite_Connection *connection;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("Cursor", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Cursor", nargs, 1, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->ConnectionType)) {
        _PyArg_BadArgumentValue("Cursor", "argument 1", (clinic_state()->ConnectionType)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    connection = (pysqlite_Connection *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = pysqlite_cursor_init_impl((pysqlite_Cursor *)self, connection);

exit:
    return return_value;
}

static int
pysqlite_cursor_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return pysqlite_cursor_init_value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = pysqlite_cursor_init_value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(pysqlite_cursor_execute__doc__,
"execute($self, sql, parameters=(), /)\n"
"--\n"
"\n"
"Executes an SQL statement.");

#define PYSQLITE_CURSOR_EXECUTE_METHODDEF    \
    {"execute", _PyCFunction_CAST(pysqlite_cursor_execute), METH_VALUE, pysqlite_cursor_execute__doc__},

static PyValue
pysqlite_cursor_execute_impl(pysqlite_Cursor *self, PyObject *sql,
                             PyValueBorrow parameters);

static PyValue
pysqlite_cursor_execute(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sql;
    PyValueBorrow parameters = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("execute", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("execute", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("execute", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    sql = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    parameters = PyValue_Borrow(args[1]);
skip_optional:
    return_value = pysqlite_cursor_execute_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self), sql, parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_executemany__doc__,
"executemany($self, sql, seq_of_parameters, /)\n"
"--\n"
"\n"
"Repeatedly executes an SQL statement.");

#define PYSQLITE_CURSOR_EXECUTEMANY_METHODDEF    \
    {"executemany", _PyCFunction_CAST(pysqlite_cursor_executemany), METH_VALUE, pysqlite_cursor_executemany__doc__},

static PyValue
pysqlite_cursor_executemany_impl(pysqlite_Cursor *self, PyObject *sql,
                                 PyValueBorrow seq_of_parameters);

static PyValue
pysqlite_cursor_executemany(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sql;
    PyValueBorrow seq_of_parameters;

    if (!_PyArg_CheckPositional("executemany", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("executemany", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("executemany", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    sql = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    seq_of_parameters = PyValue_Borrow(args[1]);
    return_value = pysqlite_cursor_executemany_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self), sql, seq_of_parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_executescript__doc__,
"executescript($self, sql_script, /)\n"
"--\n"
"\n"
"Executes multiple SQL statements at once.");

#define PYSQLITE_CURSOR_EXECUTESCRIPT_METHODDEF    \
    {"executescript", _PyCFunction_CAST(pysqlite_cursor_executescript), METH_VALUE|METH_O, pysqlite_cursor_executescript__doc__},

static PyValue
pysqlite_cursor_executescript_impl(pysqlite_Cursor *self,
                                   const char *sql_script);

static PyValue
pysqlite_cursor_executescript(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *sql_script;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("executescript", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("executescript", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    sql_script = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (sql_script == NULL) {
        goto exit;
    }
    return_value = pysqlite_cursor_executescript_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self), sql_script);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_fetchone__doc__,
"fetchone($self, /)\n"
"--\n"
"\n"
"Fetches one row from the resultset.");

#define PYSQLITE_CURSOR_FETCHONE_METHODDEF    \
    {"fetchone", _PyCFunction_CAST(pysqlite_cursor_fetchone), METH_VALUE|METH_NOARGS, pysqlite_cursor_fetchone__doc__},

static PyValue
pysqlite_cursor_fetchone_impl(pysqlite_Cursor *self);

static PyValue
pysqlite_cursor_fetchone(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_cursor_fetchone_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_fetchmany__doc__,
"fetchmany($self, /, size=1)\n"
"--\n"
"\n"
"Fetches several rows from the resultset.\n"
"\n"
"  size\n"
"    The default value is set by the Cursor.arraysize attribute.");

#define PYSQLITE_CURSOR_FETCHMANY_METHODDEF    \
    {"fetchmany", _PyCFunction_CAST(pysqlite_cursor_fetchmany), METH_VALUE|METH_KEYWORDS, pysqlite_cursor_fetchmany__doc__},

static PyValue
pysqlite_cursor_fetchmany_impl(pysqlite_Cursor *self, uint32_t maxrows);

static PyValue
pysqlite_cursor_fetchmany(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(size)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fetchmany",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    uint32_t maxrows = ((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self))->arraysize;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[0]]), &maxrows, sizeof(uint32_t),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(uint32_t)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C uint32_t");
                goto exit;
            }
        }
    }
    return_value = pysqlite_cursor_fetchmany_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self), maxrows);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_fetchall__doc__,
"fetchall($self, /)\n"
"--\n"
"\n"
"Fetches all rows from the resultset.");

#define PYSQLITE_CURSOR_FETCHALL_METHODDEF    \
    {"fetchall", _PyCFunction_CAST(pysqlite_cursor_fetchall), METH_VALUE|METH_NOARGS, pysqlite_cursor_fetchall__doc__},

static PyValue
pysqlite_cursor_fetchall_impl(pysqlite_Cursor *self);

static PyValue
pysqlite_cursor_fetchall(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_cursor_fetchall_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_setinputsizes__doc__,
"setinputsizes($self, sizes, /)\n"
"--\n"
"\n"
"Required by DB-API. Does nothing in sqlite3.");

#define PYSQLITE_CURSOR_SETINPUTSIZES_METHODDEF    \
    {"setinputsizes", _PyCFunction_CAST(pysqlite_cursor_setinputsizes), METH_VALUE|METH_O, pysqlite_cursor_setinputsizes__doc__},

static PyValue
pysqlite_cursor_setinputsizes_impl(pysqlite_Cursor *self,
                                   PyValueBorrow sizes);

static PyValue
pysqlite_cursor_setinputsizes(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sizes;

    sizes = PyValue_Borrow(args[0]);
    return_value = pysqlite_cursor_setinputsizes_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self), sizes);

    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_setoutputsize__doc__,
"setoutputsize($self, size, column=None, /)\n"
"--\n"
"\n"
"Required by DB-API. Does nothing in sqlite3.");

#define PYSQLITE_CURSOR_SETOUTPUTSIZE_METHODDEF    \
    {"setoutputsize", _PyCFunction_CAST(pysqlite_cursor_setoutputsize), METH_VALUE, pysqlite_cursor_setoutputsize__doc__},

static PyValue
pysqlite_cursor_setoutputsize_impl(pysqlite_Cursor *self, PyValueBorrow size,
                                   PyValueBorrow column);

static PyValue
pysqlite_cursor_setoutputsize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow size;
    PyValueBorrow column = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("setoutputsize", nargs, 1, 2)) {
        goto exit;
    }
    size = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    column = PyValue_Borrow(args[1]);
skip_optional:
    return_value = pysqlite_cursor_setoutputsize_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self), size, column);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_cursor_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Closes the cursor.");

#define PYSQLITE_CURSOR_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(pysqlite_cursor_close), METH_VALUE|METH_NOARGS, pysqlite_cursor_close__doc__},

static PyValue
pysqlite_cursor_close_impl(pysqlite_Cursor *self);

static PyValue
pysqlite_cursor_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_cursor_close_impl((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

static PyObject *
_sqlite3_Cursor_arraysize_get_impl(pysqlite_Cursor *self);

static PyObject *
_sqlite3_Cursor_arraysize_get(PyObject *self, void *Py_UNUSED(context))
{
    return _sqlite3_Cursor_arraysize_get_impl((pysqlite_Cursor *)self);
}

static int
_sqlite3_Cursor_arraysize_set_impl(pysqlite_Cursor *self, uint32_t value);

static int
_sqlite3_Cursor_arraysize_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    uint32_t value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'arraysize' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(arg), &value, sizeof(uint32_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(uint32_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C uint32_t");
            goto exit;
        }
    }
    return_value = _sqlite3_Cursor_arraysize_set_impl((pysqlite_Cursor *)self, value);

exit:
    return return_value;
}
#define _SQLITE3_CURSOR_ARRAYSIZE_GETSETDEF {"arraysize", (getter)_sqlite3_Cursor_arraysize_get, (setter)_sqlite3_Cursor_arraysize_set, NULL},

/*[clinic end generated code: output=a030f120e82c42e1 input=a9049054013a1b77]*/
