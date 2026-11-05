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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

static int
pysqlite_connection_init_impl(pysqlite_Connection *self,
                              PyValueBorrow database, double timeout,
                              int detect_types, const char *isolation_level,
                              int check_same_thread, PyValueBorrow factory,
                              int cache_size, int uri,
                              enum autocommit_mode autocommit);

static int
pysqlite_connection_init_value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(database)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), _PyValue_STATIC_IMMORTAL(&_Py_ID(detect_types)), _PyValue_STATIC_IMMORTAL(&_Py_ID(isolation_level)), _PyValue_STATIC_IMMORTAL(&_Py_ID(check_same_thread)), _PyValue_STATIC_IMMORTAL(&_Py_ID(factory)), _PyValue_STATIC_IMMORTAL(&_Py_ID(cached_statements)), _PyValue_STATIC_IMMORTAL(&_Py_ID(uri)), _PyValue_STATIC_IMMORTAL(&_Py_ID(autocommit)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"database", "timeout", "detect_types", "isolation_level", "check_same_thread", "factory", "cached_statements", "uri", "autocommit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Connection",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[9];
    PyValueBorrow database;
    double timeout = 5.0;
    int detect_types = 0;
    const char *isolation_level = "";
    int check_same_thread = 1;
    PyValueBorrow factory = PyValue_FromObjectBorrow((PyObject*)clinic_state()->ConnectionType);
    int cache_size = 128;
    int uri = 0;
    enum autocommit_mode autocommit = LEGACY_TRANSACTION_CONTROL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 9)) {
        goto exit;
    }
    database = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        timeout = _PyValue_AsDouble(PyValue_Borrow(args[_idx[1]]));
        if (timeout == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        detect_types = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (detect_types == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        if (!isolation_level_converter(PyValue_Borrow(args[_idx[3]]), &isolation_level)) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        check_same_thread = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (check_same_thread < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        factory = PyValue_Borrow(args[_idx[5]]);
    }
    if (_idx[6] >= 0) {
        cache_size = _PyValue_AsInt(PyValue_Borrow(args[_idx[6]]));
        if (cache_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        uri = _PyValue_IsTrue(PyValue_Borrow(args[_idx[7]]));
        if (uri < 0) {
            goto exit;
        }
    }
    if (_idx[8] >= 0) {
        if (!autocommit_converter(PyValue_Borrow(args[_idx[8]]), &autocommit)) {
            goto exit;
        }
    }
    return_value = pysqlite_connection_init_impl((pysqlite_Connection *)self, database, timeout, detect_types, isolation_level, check_same_thread, factory, cache_size, uri, autocommit);

exit:
    return return_value;
}

static int
pysqlite_connection_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return pysqlite_connection_init_value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = pysqlite_connection_init_value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(pysqlite_connection_cursor__doc__,
"cursor($self, /, factory=<unrepresentable>)\n"
"--\n"
"\n"
"Return a cursor for the connection.");

#define PYSQLITE_CONNECTION_CURSOR_METHODDEF    \
    {"cursor", _PyCFunction_CAST(pysqlite_connection_cursor), METH_VALUE|METH_KEYWORDS, pysqlite_connection_cursor__doc__},

static PyValue
pysqlite_connection_cursor_impl(pysqlite_Connection *self,
                                PyValueBorrow factory);

static PyValue
pysqlite_connection_cursor(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(factory)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"factory", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cursor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow factory = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        factory = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = pysqlite_connection_cursor_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), factory);

exit:
    return return_value;
}

PyDoc_STRVAR(blobopen__doc__,
"blobopen($self, table, column, rowid, /, *, readonly=False, name=\'main\')\n"
"--\n"
"\n"
"Open and return a BLOB object.\n"
"\n"
"  table\n"
"    Table name.\n"
"  column\n"
"    Column name.\n"
"  rowid\n"
"    Row id.\n"
"  readonly\n"
"    Open the BLOB without write permissions.\n"
"  name\n"
"    Database name.");

#define BLOBOPEN_METHODDEF    \
    {"blobopen", _PyCFunction_CAST(blobopen), METH_VALUE|METH_KEYWORDS, blobopen__doc__},

static PyValue
blobopen_impl(pysqlite_Connection *self, const char *table, const char *col,
              sqlite3_int64 row, int readonly, const char *name);

static PyValue
blobopen(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(readonly)), _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "readonly", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "blobopen",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    const char *table;
    const char *col;
    sqlite3_int64 row;
    int readonly = 0;
    const char *name = "main";

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("blobopen", "argument 1", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("blobopen", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    table = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (table == NULL) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        _PyArg_BadArgumentValue("blobopen", "argument 2", "str", PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
        _PyArg_BadArgument("blobopen", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        goto exit;
    }
    col = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
    if (col == NULL) {
        goto exit;
    }
    if (!sqlite3_int64_converter(PyValue_Borrow(args[_idx[2]]), &row)) {
        goto exit;
    }
    if (_idx[3] >= 0) {
        readonly = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (readonly < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[4]]))) {
            _PyArg_BadArgumentValue("blobopen", "argument 'name'", "str", PyValue_Borrow(args[_idx[4]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])))) {
            _PyArg_BadArgument("blobopen", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])));
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])));
        if (name == NULL) {
            goto exit;
        }
    }
    return_value = blobopen_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), table, col, row, readonly, name);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the database connection.\n"
"\n"
"Any pending transaction is not committed implicitly.");

#define PYSQLITE_CONNECTION_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(pysqlite_connection_close), METH_VALUE|METH_NOARGS, pysqlite_connection_close__doc__},

static PyValue
pysqlite_connection_close_impl(pysqlite_Connection *self);

static PyValue
pysqlite_connection_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_connection_close_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_commit__doc__,
"commit($self, /)\n"
"--\n"
"\n"
"Commit any pending transaction to the database.\n"
"\n"
"If there is no open transaction, this method is a no-op.");

#define PYSQLITE_CONNECTION_COMMIT_METHODDEF    \
    {"commit", _PyCFunction_CAST(pysqlite_connection_commit), METH_VALUE|METH_NOARGS, pysqlite_connection_commit__doc__},

static PyValue
pysqlite_connection_commit_impl(pysqlite_Connection *self);

static PyValue
pysqlite_connection_commit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_connection_commit_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_rollback__doc__,
"rollback($self, /)\n"
"--\n"
"\n"
"Roll back to the start of any pending transaction.\n"
"\n"
"If there is no open transaction, this method is a no-op.");

#define PYSQLITE_CONNECTION_ROLLBACK_METHODDEF    \
    {"rollback", _PyCFunction_CAST(pysqlite_connection_rollback), METH_VALUE|METH_NOARGS, pysqlite_connection_rollback__doc__},

static PyValue
pysqlite_connection_rollback_impl(pysqlite_Connection *self);

static PyValue
pysqlite_connection_rollback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_connection_rollback_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_create_function__doc__,
"create_function($self, name, narg, func, /, *, deterministic=False)\n"
"--\n"
"\n"
"Creates a new function.");

#define PYSQLITE_CONNECTION_CREATE_FUNCTION_METHODDEF    \
    {"create_function", _PyCFunction_CAST(pysqlite_connection_create_function), METH_VALUE|METH_METHOD|METH_KEYWORDS, pysqlite_connection_create_function__doc__},

static PyValue
pysqlite_connection_create_function_impl(pysqlite_Connection *self,
                                         PyTypeObject *cls, const char *name,
                                         int narg, PyValueBorrow func,
                                         int deterministic);

static PyValue
pysqlite_connection_create_function(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(deterministic)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "deterministic", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create_function",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    const char *name;
    int narg;
    PyValueBorrow func;
    int deterministic = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("create_function", "argument 1", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("create_function", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (name == NULL) {
        goto exit;
    }
    narg = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (narg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Borrow(args[_idx[2]]);
    if (_idx[3] >= 0) {
        deterministic = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (deterministic < 0) {
            goto exit;
        }
    }
    return_value = pysqlite_connection_create_function_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, name, narg, func, deterministic);

exit:
    return return_value;
}

#if defined(HAVE_WINDOW_FUNCTIONS)

PyDoc_STRVAR(create_window_function__doc__,
"create_window_function($self, name, num_params, aggregate_class, /)\n"
"--\n"
"\n"
"Creates or redefines an aggregate window function. Non-standard.\n"
"\n"
"  name\n"
"    The name of the SQL aggregate window function to be created or\n"
"    redefined.\n"
"  num_params\n"
"    The number of arguments the step and inverse methods takes.\n"
"  aggregate_class\n"
"    A class with step(), finalize(), value(), and inverse() methods.\n"
"    Set to None to clear the window function.");

#define CREATE_WINDOW_FUNCTION_METHODDEF    \
    {"create_window_function", _PyCFunction_CAST(create_window_function), METH_VALUE|METH_METHOD|METH_KEYWORDS, create_window_function__doc__},

static PyValue
create_window_function_impl(pysqlite_Connection *self, PyTypeObject *cls,
                            const char *name, int num_params,
                            PyValueBorrow aggregate_class);

static PyValue
create_window_function(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    int num_params;
    PyValueBorrow aggregate_class;

    if (!_PyArg_NoKwnames("create_window_function", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("create_window_function", nargs, 3, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("create_window_function", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("create_window_function", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    num_params = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (num_params == -1 && PyErr_Occurred()) {
        goto exit;
    }
    aggregate_class = PyValue_Borrow(args[2]);
    return_value = create_window_function_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, name, num_params, aggregate_class);

exit:
    return return_value;
}

#endif /* defined(HAVE_WINDOW_FUNCTIONS) */

PyDoc_STRVAR(pysqlite_connection_create_aggregate__doc__,
"create_aggregate($self, name, n_arg, aggregate_class, /)\n"
"--\n"
"\n"
"Creates a new aggregate.");

#define PYSQLITE_CONNECTION_CREATE_AGGREGATE_METHODDEF    \
    {"create_aggregate", _PyCFunction_CAST(pysqlite_connection_create_aggregate), METH_VALUE|METH_METHOD|METH_KEYWORDS, pysqlite_connection_create_aggregate__doc__},

static PyValue
pysqlite_connection_create_aggregate_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name, int n_arg,
                                          PyValueBorrow aggregate_class);

static PyValue
pysqlite_connection_create_aggregate(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    int n_arg;
    PyValueBorrow aggregate_class;

    if (!_PyArg_NoKwnames("create_aggregate", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("create_aggregate", nargs, 3, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("create_aggregate", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("create_aggregate", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    n_arg = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (n_arg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    aggregate_class = PyValue_Borrow(args[2]);
    return_value = pysqlite_connection_create_aggregate_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, name, n_arg, aggregate_class);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_authorizer__doc__,
"set_authorizer($self, authorizer_callback, /)\n"
"--\n"
"\n"
"Set authorizer callback.");

#define PYSQLITE_CONNECTION_SET_AUTHORIZER_METHODDEF    \
    {"set_authorizer", _PyCFunction_CAST(pysqlite_connection_set_authorizer), METH_VALUE|METH_METHOD|METH_KEYWORDS, pysqlite_connection_set_authorizer__doc__},

static PyValue
pysqlite_connection_set_authorizer_impl(pysqlite_Connection *self,
                                        PyTypeObject *cls,
                                        PyValueBorrow callable);

static PyValue
pysqlite_connection_set_authorizer(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow callable;

    if (!_PyArg_NoKwnames("set_authorizer", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_authorizer", nargs, 1, 1)) {
        goto exit;
    }
    callable = PyValue_Borrow(args[0]);
    return_value = pysqlite_connection_set_authorizer_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, callable);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_progress_handler__doc__,
"set_progress_handler($self, progress_handler, /, n)\n"
"--\n"
"\n"
"Set progress handler callback.\n"
"\n"
"  progress_handler\n"
"    A callable that takes no arguments.\n"
"    If the callable returns non-zero, the current query is\n"
"    terminated, and an exception is raised.\n"
"  n\n"
"    The number of SQLite virtual machine instructions that are\n"
"    executed between invocations of \'progress_handler\'.\n"
"\n"
"If \'progress_handler\' is None or \'n\' is 0, the progress handler is\n"
"disabled.");

#define PYSQLITE_CONNECTION_SET_PROGRESS_HANDLER_METHODDEF    \
    {"set_progress_handler", _PyCFunction_CAST(pysqlite_connection_set_progress_handler), METH_VALUE|METH_METHOD|METH_KEYWORDS, pysqlite_connection_set_progress_handler__doc__},

static PyValue
pysqlite_connection_set_progress_handler_impl(pysqlite_Connection *self,
                                              PyTypeObject *cls,
                                              PyValueBorrow callable, int n);

static PyValue
pysqlite_connection_set_progress_handler(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('n')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "n", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_progress_handler",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow callable;
    int n;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    callable = PyValue_Borrow(args[_idx[0]]);
    n = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pysqlite_connection_set_progress_handler_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, callable, n);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_trace_callback__doc__,
"set_trace_callback($self, trace_callback, /)\n"
"--\n"
"\n"
"Set a trace callback called for each SQL statement (passed as unicode).");

#define PYSQLITE_CONNECTION_SET_TRACE_CALLBACK_METHODDEF    \
    {"set_trace_callback", _PyCFunction_CAST(pysqlite_connection_set_trace_callback), METH_VALUE|METH_METHOD|METH_KEYWORDS, pysqlite_connection_set_trace_callback__doc__},

static PyValue
pysqlite_connection_set_trace_callback_impl(pysqlite_Connection *self,
                                            PyTypeObject *cls,
                                            PyValueBorrow callable);

static PyValue
pysqlite_connection_set_trace_callback(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow callable;

    if (!_PyArg_NoKwnames("set_trace_callback", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_trace_callback", nargs, 1, 1)) {
        goto exit;
    }
    callable = PyValue_Borrow(args[0]);
    return_value = pysqlite_connection_set_trace_callback_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, callable);

exit:
    return return_value;
}

#if defined(PY_SQLITE_ENABLE_LOAD_EXTENSION)

PyDoc_STRVAR(pysqlite_connection_enable_load_extension__doc__,
"enable_load_extension($self, enable, /)\n"
"--\n"
"\n"
"Enable dynamic loading of SQLite extension modules.");

#define PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF    \
    {"enable_load_extension", _PyCFunction_CAST(pysqlite_connection_enable_load_extension), METH_VALUE|METH_O, pysqlite_connection_enable_load_extension__doc__},

static PyValue
pysqlite_connection_enable_load_extension_impl(pysqlite_Connection *self,
                                               int onoff);

static PyValue
pysqlite_connection_enable_load_extension(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int onoff;

    onoff = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (onoff < 0) {
        goto exit;
    }
    return_value = pysqlite_connection_enable_load_extension_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), onoff);

exit:
    return return_value;
}

#endif /* defined(PY_SQLITE_ENABLE_LOAD_EXTENSION) */

#if defined(PY_SQLITE_ENABLE_LOAD_EXTENSION)

PyDoc_STRVAR(pysqlite_connection_load_extension__doc__,
"load_extension($self, name, /, *, entrypoint=None)\n"
"--\n"
"\n"
"Load SQLite extension module.");

#define PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF    \
    {"load_extension", _PyCFunction_CAST(pysqlite_connection_load_extension), METH_VALUE|METH_KEYWORDS, pysqlite_connection_load_extension__doc__},

static PyValue
pysqlite_connection_load_extension_impl(pysqlite_Connection *self,
                                        const char *extension_name,
                                        const char *entrypoint);

static PyValue
pysqlite_connection_load_extension(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(entrypoint)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "entrypoint", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load_extension",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    const char *extension_name;
    const char *entrypoint = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("load_extension", "argument 1", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("load_extension", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    extension_name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (extension_name == NULL) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("load_extension", "argument 'entrypoint'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            entrypoint = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            entrypoint = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (entrypoint == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("load_extension", "argument 'entrypoint'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    return_value = pysqlite_connection_load_extension_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), extension_name, entrypoint);

exit:
    return return_value;
}

#endif /* defined(PY_SQLITE_ENABLE_LOAD_EXTENSION) */

PyDoc_STRVAR(pysqlite_connection_execute__doc__,
"execute($self, sql, parameters=(), /)\n"
"--\n"
"\n"
"Executes an SQL statement.");

#define PYSQLITE_CONNECTION_EXECUTE_METHODDEF    \
    {"execute", _PyCFunction_CAST(pysqlite_connection_execute), METH_VALUE, pysqlite_connection_execute__doc__},

static PyValue
pysqlite_connection_execute_impl(pysqlite_Connection *self, PyObject *sql,
                                 PyValueBorrow parameters);

static PyValue
pysqlite_connection_execute(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = pysqlite_connection_execute_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), sql, parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_executemany__doc__,
"executemany($self, sql, parameters, /)\n"
"--\n"
"\n"
"Repeatedly executes an SQL statement.");

#define PYSQLITE_CONNECTION_EXECUTEMANY_METHODDEF    \
    {"executemany", _PyCFunction_CAST(pysqlite_connection_executemany), METH_VALUE, pysqlite_connection_executemany__doc__},

static PyValue
pysqlite_connection_executemany_impl(pysqlite_Connection *self,
                                     PyObject *sql, PyValueBorrow parameters);

static PyValue
pysqlite_connection_executemany(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sql;
    PyValueBorrow parameters;

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
    parameters = PyValue_Borrow(args[1]);
    return_value = pysqlite_connection_executemany_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), sql, parameters);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_executescript__doc__,
"executescript($self, sql_script, /)\n"
"--\n"
"\n"
"Executes multiple SQL statements at once.");

#define PYSQLITE_CONNECTION_EXECUTESCRIPT_METHODDEF    \
    {"executescript", _PyCFunction_CAST(pysqlite_connection_executescript), METH_VALUE|METH_O, pysqlite_connection_executescript__doc__},

static PyValue
pysqlite_connection_executescript_impl(pysqlite_Connection *self,
                                       PyValueBorrow script_obj);

static PyValue
pysqlite_connection_executescript(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow script_obj;

    script_obj = PyValue_Borrow(args[0]);
    return_value = pysqlite_connection_executescript_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), script_obj);

    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_interrupt__doc__,
"interrupt($self, /)\n"
"--\n"
"\n"
"Abort any pending database operation.");

#define PYSQLITE_CONNECTION_INTERRUPT_METHODDEF    \
    {"interrupt", _PyCFunction_CAST(pysqlite_connection_interrupt), METH_VALUE|METH_NOARGS, pysqlite_connection_interrupt__doc__},

static PyValue
pysqlite_connection_interrupt_impl(pysqlite_Connection *self);

static PyValue
pysqlite_connection_interrupt(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_connection_interrupt_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_iterdump__doc__,
"iterdump($self, /, *, filter=None)\n"
"--\n"
"\n"
"Returns iterator to the dump of the database in an SQL text format.\n"
"\n"
"  filter\n"
"    An optional LIKE pattern for database objects to dump");

#define PYSQLITE_CONNECTION_ITERDUMP_METHODDEF    \
    {"iterdump", _PyCFunction_CAST(pysqlite_connection_iterdump), METH_VALUE|METH_KEYWORDS, pysqlite_connection_iterdump__doc__},

static PyValue
pysqlite_connection_iterdump_impl(pysqlite_Connection *self,
                                  PyValueBorrow filter);

static PyValue
pysqlite_connection_iterdump(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(filter)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"filter", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "iterdump",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow filter = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        filter = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = pysqlite_connection_iterdump_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), filter);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_backup__doc__,
"backup($self, /, target, *, pages=-1, progress=None, name=\'main\',\n"
"       sleep=0.25)\n"
"--\n"
"\n"
"Makes a backup of the database.");

#define PYSQLITE_CONNECTION_BACKUP_METHODDEF    \
    {"backup", _PyCFunction_CAST(pysqlite_connection_backup), METH_VALUE|METH_KEYWORDS, pysqlite_connection_backup__doc__},

static PyValue
pysqlite_connection_backup_impl(pysqlite_Connection *self,
                                pysqlite_Connection *target, int pages,
                                PyValueBorrow progress, const char *name,
                                double sleep);

static PyValue
pysqlite_connection_backup(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(target)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pages)), _PyValue_STATIC_IMMORTAL(&_Py_ID(progress)), _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(sleep)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"target", "pages", "progress", "name", "sleep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "backup",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    pysqlite_Connection *target;
    int pages = -1;
    PyValueBorrow progress = PyValue_Borrow(PyValue_None);
    const char *name = "main";
    double sleep = 0.25;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), clinic_state()->ConnectionType)) {
        _PyArg_BadArgumentValue("backup", "argument 'target'", (clinic_state()->ConnectionType)->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    target = (pysqlite_Connection *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        pages = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (pages == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        progress = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            _PyArg_BadArgumentValue("backup", "argument 'name'", "str", PyValue_Borrow(args[_idx[3]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])))) {
            _PyArg_BadArgument("backup", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
        if (name == NULL) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        sleep = _PyValue_AsDouble(PyValue_Borrow(args[_idx[4]]));
        if (sleep == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = pysqlite_connection_backup_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), target, pages, progress, name, sleep);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_create_collation__doc__,
"create_collation($self, name, callback, /)\n"
"--\n"
"\n"
"Creates a collation function.");

#define PYSQLITE_CONNECTION_CREATE_COLLATION_METHODDEF    \
    {"create_collation", _PyCFunction_CAST(pysqlite_connection_create_collation), METH_VALUE|METH_METHOD|METH_KEYWORDS, pysqlite_connection_create_collation__doc__},

static PyValue
pysqlite_connection_create_collation_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name,
                                          PyValueBorrow callable);

static PyValue
pysqlite_connection_create_collation(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    PyValueBorrow callable;

    if (!_PyArg_NoKwnames("create_collation", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("create_collation", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("create_collation", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("create_collation", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    callable = PyValue_Borrow(args[1]);
    return_value = pysqlite_connection_create_collation_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), cls, name, callable);

exit:
    return return_value;
}

#if defined(PY_SQLITE_HAVE_SERIALIZE)

PyDoc_STRVAR(serialize__doc__,
"serialize($self, /, *, name=\'main\')\n"
"--\n"
"\n"
"Serialize a database into a byte string.\n"
"\n"
"  name\n"
"    Which database to serialize.\n"
"\n"
"For an ordinary on-disk database file, the serialization is just\n"
"a copy of the disk file.  For an in-memory database or a \"temp\"\n"
"database, the serialization is the same sequence of bytes which\n"
"would be written to disk if that database were backed up to disk.");

#define SERIALIZE_METHODDEF    \
    {"serialize", _PyCFunction_CAST(serialize), METH_VALUE|METH_KEYWORDS, serialize__doc__},

static PyValue
serialize_impl(pysqlite_Connection *self, const char *name);

static PyValue
serialize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "serialize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *name = "main";

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("serialize", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("serialize", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        if (name == NULL) {
            goto exit;
        }
    }
    return_value = serialize_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), name);

exit:
    return return_value;
}

#endif /* defined(PY_SQLITE_HAVE_SERIALIZE) */

#if defined(PY_SQLITE_HAVE_SERIALIZE)

PyDoc_STRVAR(deserialize__doc__,
"deserialize($self, data, /, *, name=\'main\')\n"
"--\n"
"\n"
"Load a serialized database.\n"
"\n"
"  data\n"
"    The serialized database content.\n"
"  name\n"
"    Which database to reopen with the deserialization.\n"
"\n"
"The deserialize interface causes the database connection to\n"
"disconnect from the target database, and then reopen it as\n"
"an in-memory database based on the given serialized data.\n"
"\n"
"The deserialize interface will fail with SQLITE_BUSY if the database\n"
"is currently in a read transaction or is involved in a backup\n"
"operation.");

#define DESERIALIZE_METHODDEF    \
    {"deserialize", _PyCFunction_CAST(deserialize), METH_VALUE|METH_KEYWORDS, deserialize__doc__},

static PyValue
deserialize_impl(pysqlite_Connection *self, Py_buffer *data,
                 const char *name);

static PyValue
deserialize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "deserialize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer data = {NULL, NULL};
    const char *name = "main";

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&data, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("deserialize", "argument 'name'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("deserialize", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (name == NULL) {
            goto exit;
        }
    }
    return_value = deserialize_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), &data, name);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(PY_SQLITE_HAVE_SERIALIZE) */

PyDoc_STRVAR(pysqlite_connection_enter__doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Called when the connection is used as a context manager.\n"
"\n"
"Returns itself as a convenience to the caller.");

#define PYSQLITE_CONNECTION_ENTER_METHODDEF    \
    {"__enter__", _PyCFunction_CAST(pysqlite_connection_enter), METH_VALUE|METH_NOARGS, pysqlite_connection_enter__doc__},

static PyValue
pysqlite_connection_enter_impl(pysqlite_Connection *self);

static PyValue
pysqlite_connection_enter(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pysqlite_connection_enter_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_exit__doc__,
"__exit__($self, type, value, traceback, /)\n"
"--\n"
"\n"
"Called when the connection is used as a context manager.\n"
"\n"
"If there was any exception, a rollback takes place; otherwise we\n"
"commit.");

#define PYSQLITE_CONNECTION_EXIT_METHODDEF    \
    {"__exit__", _PyCFunction_CAST(pysqlite_connection_exit), METH_VALUE, pysqlite_connection_exit__doc__},

static PyValue
pysqlite_connection_exit_impl(pysqlite_Connection *self,
                              PyValueBorrow exc_type,
                              PyValueBorrow exc_value, PyValueBorrow exc_tb);

static PyValue
pysqlite_connection_exit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exc_type;
    PyValueBorrow exc_value;
    PyValueBorrow exc_tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = PyValue_Borrow(args[0]);
    exc_value = PyValue_Borrow(args[1]);
    exc_tb = PyValue_Borrow(args[2]);
    return_value = pysqlite_connection_exit_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(setlimit__doc__,
"setlimit($self, category, limit, /)\n"
"--\n"
"\n"
"Set connection run-time limits.\n"
"\n"
"  category\n"
"    The limit category to be set.\n"
"  limit\n"
"    The new limit. If the new limit is a negative number, the limit\n"
"    is unchanged.\n"
"\n"
"Attempts to increase a limit above its hard upper bound are silently\n"
"truncated to the hard upper bound. Regardless of whether or not the\n"
"limit was changed, the prior value of the limit is returned.");

#define SETLIMIT_METHODDEF    \
    {"setlimit", _PyCFunction_CAST(setlimit), METH_VALUE, setlimit__doc__},

static PyValue
setlimit_impl(pysqlite_Connection *self, int category, int limit);

static PyValue
setlimit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int category;
    int limit;

    if (!_PyArg_CheckPositional("setlimit", nargs, 2, 2)) {
        goto exit;
    }
    category = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    limit = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (limit == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = setlimit_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), category, limit);

exit:
    return return_value;
}

PyDoc_STRVAR(getlimit__doc__,
"getlimit($self, category, /)\n"
"--\n"
"\n"
"Get connection run-time limits.\n"
"\n"
"  category\n"
"    The limit category to be queried.");

#define GETLIMIT_METHODDEF    \
    {"getlimit", _PyCFunction_CAST(getlimit), METH_VALUE|METH_O, getlimit__doc__},

static PyValue
getlimit_impl(pysqlite_Connection *self, int category);

static PyValue
getlimit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int category;

    category = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = getlimit_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), category);

exit:
    return return_value;
}

PyDoc_STRVAR(setconfig__doc__,
"setconfig($self, op, enable=True, /)\n"
"--\n"
"\n"
"Set a boolean connection configuration option.\n"
"\n"
"  op\n"
"    The configuration verb; one of the sqlite3.SQLITE_DBCONFIG codes.");

#define SETCONFIG_METHODDEF    \
    {"setconfig", _PyCFunction_CAST(setconfig), METH_VALUE, setconfig__doc__},

static PyValue
setconfig_impl(pysqlite_Connection *self, int op, int enable);

static PyValue
setconfig(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int op;
    int enable = 1;

    if (!_PyArg_CheckPositional("setconfig", nargs, 1, 2)) {
        goto exit;
    }
    op = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    enable = _PyValue_IsTrue(PyValue_Borrow(args[1]));
    if (enable < 0) {
        goto exit;
    }
skip_optional:
    return_value = setconfig_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), op, enable);

exit:
    return return_value;
}

PyDoc_STRVAR(getconfig__doc__,
"getconfig($self, op, /)\n"
"--\n"
"\n"
"Query a boolean connection configuration option.\n"
"\n"
"  op\n"
"    The configuration verb; one of the sqlite3.SQLITE_DBCONFIG codes.");

#define GETCONFIG_METHODDEF    \
    {"getconfig", _PyCFunction_CAST(getconfig), METH_VALUE|METH_O, getconfig__doc__},

static int
getconfig_impl(pysqlite_Connection *self, int op);

static PyValue
getconfig(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int op;
    int _return_value;

    op = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = getconfig_impl((pysqlite_Connection *)PyValue_AsKnownObjectBorrow(self), op);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

#ifndef CREATE_WINDOW_FUNCTION_METHODDEF
    #define CREATE_WINDOW_FUNCTION_METHODDEF
#endif /* !defined(CREATE_WINDOW_FUNCTION_METHODDEF) */

#ifndef PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
    #define PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF
#endif /* !defined(PYSQLITE_CONNECTION_ENABLE_LOAD_EXTENSION_METHODDEF) */

#ifndef PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
    #define PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF
#endif /* !defined(PYSQLITE_CONNECTION_LOAD_EXTENSION_METHODDEF) */

#ifndef SERIALIZE_METHODDEF
    #define SERIALIZE_METHODDEF
#endif /* !defined(SERIALIZE_METHODDEF) */

#ifndef DESERIALIZE_METHODDEF
    #define DESERIALIZE_METHODDEF
#endif /* !defined(DESERIALIZE_METHODDEF) */
/*[clinic end generated code: output=3515bce7b48927c0 input=a9049054013a1b77]*/
