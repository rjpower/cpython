/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

static int
pysqlite_connection_init_impl(pysqlite_Connection *self, PyObject *database,
                              double timeout, int detect_types,
                              const char *isolation_level,
                              int check_same_thread, PyObject *factory,
                              int cache_size, int uri,
                              enum autocommit_mode autocommit);

static int
pysqlite_connection_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(database), &_Py_ID(timeout), &_Py_ID(detect_types), &_Py_ID(isolation_level), &_Py_ID(check_same_thread), &_Py_ID(factory), &_Py_ID(cached_statements), &_Py_ID(uri), &_Py_ID(autocommit), },
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
    PyValue argsbuf[9];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *database;
    double timeout = 5.0;
    int detect_types = 0;
    const char *isolation_level = "";
    int check_same_thread = 1;
    PyObject *factory = (PyObject*)clinic_state()->ConnectionType;
    int cache_size = 128;
    int uri = 0;
    enum autocommit_mode autocommit = LEGACY_TRANSACTION_CONTROL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    database = PyValue_Object(fastargs[0]);
    if (database == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[1])) {
        timeout = PyFloat_AsDoubleValue(fastargs[1]);
        if (timeout == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[2])) {
        detect_types = PyLong_AsIntValue(fastargs[2]);
        if (detect_types == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[3])) {
        {
            PyObject *_arg = PyValue_Object(fastargs[3]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!isolation_level_converter(_arg, &isolation_level)) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[4])) {
        check_same_thread = PyObject_IsTrueValue(fastargs[4]);
        if (check_same_thread < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[5])) {
        factory = PyValue_Object(fastargs[5]);
        if (factory == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[6])) {
        cache_size = PyLong_AsIntValue(fastargs[6]);
        if (cache_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[7])) {
        uri = PyObject_IsTrueValue(fastargs[7]);
        if (uri < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    {
        PyObject *_arg = PyValue_Object(fastargs[8]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!autocommit_converter(_arg, &autocommit)) {
            goto exit;
        }
    }
skip_optional_kwonly:
    return_value = pysqlite_connection_init_impl((pysqlite_Connection *)self, database, timeout, detect_types, isolation_level, check_same_thread, factory, cache_size, uri, autocommit);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_cursor__doc__,
"cursor($self, /, factory=<unrepresentable>)\n"
"--\n"
"\n"
"Return a cursor for the connection.");

#define PYSQLITE_CONNECTION_CURSOR_METHODDEF    \
    {"cursor", _PyCFunction_CAST(pysqlite_connection_cursor), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_cursor__doc__},

static PyObject *
pysqlite_connection_cursor_impl(pysqlite_Connection *self, PyObject *factory);

static PyValue
pysqlite_connection_cursor(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(factory), },
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
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *factory = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    factory = PyValue_Object(args[0]);
    if (factory == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = pysqlite_connection_cursor_impl((pysqlite_Connection *)self, factory);
    return_value = PyValue_FromResult(_return_value);

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
    {"blobopen", _PyCFunction_CAST(blobopen), METH_FASTCALL|METH_KEYWORDS, blobopen__doc__},

static PyObject *
blobopen_impl(pysqlite_Connection *self, const char *table, const char *col,
              sqlite3_int64 row, int readonly, const char *name);

static PyValue
blobopen(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(readonly), &_Py_ID(name), },
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
    PyValue argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    const char *table;
    const char *col;
    sqlite3_int64 row;
    int readonly = 0;
    const char *name = "main";
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("blobopen", "argument 1", "str", _arg);
            goto exit;
        }
        table = _PyUnicode_AsUTF8NoNUL(_arg);
        if (table == NULL) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("blobopen", "argument 2", "str", _arg);
            goto exit;
        }
        col = _PyUnicode_AsUTF8NoNUL(_arg);
        if (col == NULL) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!sqlite3_int64_converter(_arg, &row)) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[3])) {
        readonly = PyObject_IsTrueValue(args[3]);
        if (readonly < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[4]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("blobopen", "argument 'name'", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
skip_optional_kwonly:
    _return_value = blobopen_impl((pysqlite_Connection *)self, table, col, row, readonly, name);
    return_value = PyValue_FromResult(_return_value);

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
    {"close", (PyCFunction)pysqlite_connection_close, METH_NOARGS, pysqlite_connection_close__doc__},

static PyObject *
pysqlite_connection_close_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_close_impl((pysqlite_Connection *)self);
}

PyDoc_STRVAR(pysqlite_connection_commit__doc__,
"commit($self, /)\n"
"--\n"
"\n"
"Commit any pending transaction to the database.\n"
"\n"
"If there is no open transaction, this method is a no-op.");

#define PYSQLITE_CONNECTION_COMMIT_METHODDEF    \
    {"commit", (PyCFunction)pysqlite_connection_commit, METH_NOARGS, pysqlite_connection_commit__doc__},

static PyObject *
pysqlite_connection_commit_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_commit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_commit_impl((pysqlite_Connection *)self);
}

PyDoc_STRVAR(pysqlite_connection_rollback__doc__,
"rollback($self, /)\n"
"--\n"
"\n"
"Roll back to the start of any pending transaction.\n"
"\n"
"If there is no open transaction, this method is a no-op.");

#define PYSQLITE_CONNECTION_ROLLBACK_METHODDEF    \
    {"rollback", (PyCFunction)pysqlite_connection_rollback, METH_NOARGS, pysqlite_connection_rollback__doc__},

static PyObject *
pysqlite_connection_rollback_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_rollback(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_rollback_impl((pysqlite_Connection *)self);
}

PyDoc_STRVAR(pysqlite_connection_create_function__doc__,
"create_function($self, name, narg, func, /, *, deterministic=False)\n"
"--\n"
"\n"
"Creates a new function.");

#define PYSQLITE_CONNECTION_CREATE_FUNCTION_METHODDEF    \
    {"create_function", _PyCFunction_CAST(pysqlite_connection_create_function), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_function__doc__},

static PyObject *
pysqlite_connection_create_function_impl(pysqlite_Connection *self,
                                         PyTypeObject *cls, const char *name,
                                         int narg, PyObject *func,
                                         int deterministic);

static PyValue
pysqlite_connection_create_function(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(deterministic), },
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
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    const char *name;
    int narg;
    PyObject *func;
    int deterministic = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("create_function", "argument 1", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    narg = PyLong_AsIntValue(args[1]);
    if (narg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    func = PyValue_Object(args[2]);
    if (func == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    deterministic = PyObject_IsTrueValue(args[3]);
    if (deterministic < 0) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = pysqlite_connection_create_function_impl((pysqlite_Connection *)self, cls, name, narg, func, deterministic);
    return_value = PyValue_FromResult(_return_value);

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
    {"create_window_function", _PyCFunction_CAST(create_window_function), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, create_window_function__doc__},

static PyObject *
create_window_function_impl(pysqlite_Connection *self, PyTypeObject *cls,
                            const char *name, int num_params,
                            PyObject *aggregate_class);

static PyValue
create_window_function(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    int num_params;
    PyObject *aggregate_class;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("create_window_function", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("create_window_function", nargs, 3, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("create_window_function", "argument 1", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    num_params = PyLong_AsIntValue(args[1]);
    if (num_params == -1 && PyErr_Occurred()) {
        goto exit;
    }
    aggregate_class = PyValue_Object(args[2]);
    if (aggregate_class == NULL) {
        goto exit;
    }
    _return_value = create_window_function_impl((pysqlite_Connection *)self, cls, name, num_params, aggregate_class);
    return_value = PyValue_FromResult(_return_value);

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
    {"create_aggregate", _PyCFunction_CAST(pysqlite_connection_create_aggregate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_aggregate__doc__},

static PyObject *
pysqlite_connection_create_aggregate_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name, int n_arg,
                                          PyObject *aggregate_class);

static PyValue
pysqlite_connection_create_aggregate(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    int n_arg;
    PyObject *aggregate_class;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("create_aggregate", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("create_aggregate", nargs, 3, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("create_aggregate", "argument 1", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    n_arg = PyLong_AsIntValue(args[1]);
    if (n_arg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    aggregate_class = PyValue_Object(args[2]);
    if (aggregate_class == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_create_aggregate_impl((pysqlite_Connection *)self, cls, name, n_arg, aggregate_class);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_authorizer__doc__,
"set_authorizer($self, authorizer_callback, /)\n"
"--\n"
"\n"
"Set authorizer callback.");

#define PYSQLITE_CONNECTION_SET_AUTHORIZER_METHODDEF    \
    {"set_authorizer", _PyCFunction_CAST(pysqlite_connection_set_authorizer), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_authorizer__doc__},

static PyObject *
pysqlite_connection_set_authorizer_impl(pysqlite_Connection *self,
                                        PyTypeObject *cls,
                                        PyObject *callable);

static PyValue
pysqlite_connection_set_authorizer(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *callable;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("set_authorizer", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_authorizer", nargs, 1, 1)) {
        goto exit;
    }
    callable = PyValue_Object(args[0]);
    if (callable == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_set_authorizer_impl((pysqlite_Connection *)self, cls, callable);
    return_value = PyValue_FromResult(_return_value);

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
    {"set_progress_handler", _PyCFunction_CAST(pysqlite_connection_set_progress_handler), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_progress_handler__doc__},

static PyObject *
pysqlite_connection_set_progress_handler_impl(pysqlite_Connection *self,
                                              PyTypeObject *cls,
                                              PyObject *callable, int n);

static PyValue
pysqlite_connection_set_progress_handler(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('n'), },
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
    PyValue argsbuf[2];
    PyObject *callable;
    int n;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    callable = PyValue_Object(args[0]);
    if (callable == NULL) {
        goto exit;
    }
    n = PyLong_AsIntValue(args[1]);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = pysqlite_connection_set_progress_handler_impl((pysqlite_Connection *)self, cls, callable, n);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_set_trace_callback__doc__,
"set_trace_callback($self, trace_callback, /)\n"
"--\n"
"\n"
"Set a trace callback called for each SQL statement (passed as unicode).");

#define PYSQLITE_CONNECTION_SET_TRACE_CALLBACK_METHODDEF    \
    {"set_trace_callback", _PyCFunction_CAST(pysqlite_connection_set_trace_callback), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_set_trace_callback__doc__},

static PyObject *
pysqlite_connection_set_trace_callback_impl(pysqlite_Connection *self,
                                            PyTypeObject *cls,
                                            PyObject *callable);

static PyValue
pysqlite_connection_set_trace_callback(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *callable;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("set_trace_callback", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_trace_callback", nargs, 1, 1)) {
        goto exit;
    }
    callable = PyValue_Object(args[0]);
    if (callable == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_set_trace_callback_impl((pysqlite_Connection *)self, cls, callable);
    return_value = PyValue_FromResult(_return_value);

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
    {"enable_load_extension", _PyCFunction_CAST(pysqlite_connection_enable_load_extension), METH_O, pysqlite_connection_enable_load_extension__doc__},

static PyObject *
pysqlite_connection_enable_load_extension_impl(pysqlite_Connection *self,
                                               int onoff);

static PyValue
pysqlite_connection_enable_load_extension(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int onoff;
    PyObject *_return_value;

    onoff = PyObject_IsTrueValue(arg);
    if (onoff < 0) {
        goto exit;
    }
    _return_value = pysqlite_connection_enable_load_extension_impl((pysqlite_Connection *)self, onoff);
    return_value = PyValue_FromResult(_return_value);

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
    {"load_extension", _PyCFunction_CAST(pysqlite_connection_load_extension), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_load_extension__doc__},

static PyObject *
pysqlite_connection_load_extension_impl(pysqlite_Connection *self,
                                        const char *extension_name,
                                        const char *entrypoint);

static PyValue
pysqlite_connection_load_extension(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(entrypoint), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    const char *extension_name;
    const char *entrypoint = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("load_extension", "argument 1", "str", _arg);
            goto exit;
        }
        extension_name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (extension_name == NULL) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            entrypoint = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            entrypoint = _PyUnicode_AsUTF8NoNUL(_arg);
            if (entrypoint == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("load_extension", "argument 'entrypoint'", "str or None", _arg);
            goto exit;
        }
    }
skip_optional_kwonly:
    _return_value = pysqlite_connection_load_extension_impl((pysqlite_Connection *)self, extension_name, entrypoint);
    return_value = PyValue_FromResult(_return_value);

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
    {"execute", _PyCFunction_CAST(pysqlite_connection_execute), METH_FASTCALL, pysqlite_connection_execute__doc__},

static PyObject *
pysqlite_connection_execute_impl(pysqlite_Connection *self, PyObject *sql,
                                 PyObject *parameters);

static PyValue
pysqlite_connection_execute(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sql;
    PyObject *parameters = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("execute", nargs, 1, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("execute", "argument 1", "str", _arg);
            goto exit;
        }
        sql = _arg;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    parameters = PyValue_Object(args[1]);
    if (parameters == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = pysqlite_connection_execute_impl((pysqlite_Connection *)self, sql, parameters);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_executemany__doc__,
"executemany($self, sql, parameters, /)\n"
"--\n"
"\n"
"Repeatedly executes an SQL statement.");

#define PYSQLITE_CONNECTION_EXECUTEMANY_METHODDEF    \
    {"executemany", _PyCFunction_CAST(pysqlite_connection_executemany), METH_FASTCALL, pysqlite_connection_executemany__doc__},

static PyObject *
pysqlite_connection_executemany_impl(pysqlite_Connection *self,
                                     PyObject *sql, PyObject *parameters);

static PyValue
pysqlite_connection_executemany(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sql;
    PyObject *parameters;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("executemany", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("executemany", "argument 1", "str", _arg);
            goto exit;
        }
        sql = _arg;
    }
    parameters = PyValue_Object(args[1]);
    if (parameters == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_executemany_impl((pysqlite_Connection *)self, sql, parameters);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_executescript__doc__,
"executescript($self, sql_script, /)\n"
"--\n"
"\n"
"Executes multiple SQL statements at once.");

#define PYSQLITE_CONNECTION_EXECUTESCRIPT_METHODDEF    \
    {"executescript", _PyCFunction_CAST(pysqlite_connection_executescript), METH_O, pysqlite_connection_executescript__doc__},

static PyObject *
pysqlite_connection_executescript_impl(pysqlite_Connection *self,
                                       PyObject *script_obj);

static PyValue
pysqlite_connection_executescript(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *script_obj;
    PyObject *_return_value;

    script_obj = PyValue_Object(arg);
    if (script_obj == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_executescript_impl((pysqlite_Connection *)self, script_obj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_interrupt__doc__,
"interrupt($self, /)\n"
"--\n"
"\n"
"Abort any pending database operation.");

#define PYSQLITE_CONNECTION_INTERRUPT_METHODDEF    \
    {"interrupt", (PyCFunction)pysqlite_connection_interrupt, METH_NOARGS, pysqlite_connection_interrupt__doc__},

static PyObject *
pysqlite_connection_interrupt_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_interrupt(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_interrupt_impl((pysqlite_Connection *)self);
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
    {"iterdump", _PyCFunction_CAST(pysqlite_connection_iterdump), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_iterdump__doc__},

static PyObject *
pysqlite_connection_iterdump_impl(pysqlite_Connection *self,
                                  PyObject *filter);

static PyValue
pysqlite_connection_iterdump(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(filter), },
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
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *filter = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    filter = PyValue_Object(args[0]);
    if (filter == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = pysqlite_connection_iterdump_impl((pysqlite_Connection *)self, filter);
    return_value = PyValue_FromResult(_return_value);

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
    {"backup", _PyCFunction_CAST(pysqlite_connection_backup), METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_backup__doc__},

static PyObject *
pysqlite_connection_backup_impl(pysqlite_Connection *self,
                                pysqlite_Connection *target, int pages,
                                PyObject *progress, const char *name,
                                double sleep);

static PyValue
pysqlite_connection_backup(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(target), &_Py_ID(pages), &_Py_ID(progress), &_Py_ID(name), &_Py_ID(sleep), },
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
    PyValue argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    pysqlite_Connection *target;
    int pages = -1;
    PyObject *progress = Py_None;
    const char *name = "main";
    double sleep = 0.25;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyObject_TypeCheck(_arg, clinic_state()->ConnectionType)) {
            _PyArg_BadArgument("backup", "argument 'target'", (clinic_state()->ConnectionType)->tp_name, _arg);
            goto exit;
        }
        target = (pysqlite_Connection *)_arg;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[1])) {
        pages = PyLong_AsIntValue(args[1]);
        if (pages == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[2])) {
        progress = PyValue_Object(args[2]);
        if (progress == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[3])) {
        {
            PyObject *_arg = PyValue_Object(args[3]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("backup", "argument 'name'", "str", _arg);
                goto exit;
            }
            name = _PyUnicode_AsUTF8NoNUL(_arg);
            if (name == NULL) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    sleep = PyFloat_AsDoubleValue(args[4]);
    if (sleep == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = pysqlite_connection_backup_impl((pysqlite_Connection *)self, target, pages, progress, name, sleep);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_connection_create_collation__doc__,
"create_collation($self, name, callback, /)\n"
"--\n"
"\n"
"Creates a collation function.");

#define PYSQLITE_CONNECTION_CREATE_COLLATION_METHODDEF    \
    {"create_collation", _PyCFunction_CAST(pysqlite_connection_create_collation), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, pysqlite_connection_create_collation__doc__},

static PyObject *
pysqlite_connection_create_collation_impl(pysqlite_Connection *self,
                                          PyTypeObject *cls,
                                          const char *name,
                                          PyObject *callable);

static PyValue
pysqlite_connection_create_collation(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    PyObject *callable;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("create_collation", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("create_collation", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("create_collation", "argument 1", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    callable = PyValue_Object(args[1]);
    if (callable == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_create_collation_impl((pysqlite_Connection *)self, cls, name, callable);
    return_value = PyValue_FromResult(_return_value);

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
    {"serialize", _PyCFunction_CAST(serialize), METH_FASTCALL|METH_KEYWORDS, serialize__doc__},

static PyObject *
serialize_impl(pysqlite_Connection *self, const char *name);

static PyValue
serialize(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), },
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
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *name = "main";
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("serialize", "argument 'name'", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
skip_optional_kwonly:
    _return_value = serialize_impl((pysqlite_Connection *)self, name);
    return_value = PyValue_FromResult(_return_value);

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
    {"deserialize", _PyCFunction_CAST(deserialize), METH_FASTCALL|METH_KEYWORDS, deserialize__doc__},

static PyObject *
deserialize_impl(pysqlite_Connection *self, Py_buffer *data,
                 const char *name);

static PyValue
deserialize(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    const char *name = "main";
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyUnicode_Check(_arg)) {
            Py_ssize_t len;
            const char *ptr = PyUnicode_AsUTF8AndSize(_arg, &len);
            if (ptr == NULL) {
                goto exit;
            }
            if (PyBuffer_FillInfo(&data, _arg, (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
                goto exit;
            }
        }
        else { /* any bytes-like object */
            if (PyObject_GetBuffer(_arg, &data, PyBUF_SIMPLE) != 0) {
                goto exit;
            }
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("deserialize", "argument 'name'", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
skip_optional_kwonly:
    _return_value = deserialize_impl((pysqlite_Connection *)self, &data, name);
    return_value = PyValue_FromResult(_return_value);

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
    {"__enter__", (PyCFunction)pysqlite_connection_enter, METH_NOARGS, pysqlite_connection_enter__doc__},

static PyObject *
pysqlite_connection_enter_impl(pysqlite_Connection *self);

static PyObject *
pysqlite_connection_enter(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return pysqlite_connection_enter_impl((pysqlite_Connection *)self);
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
    {"__exit__", _PyCFunction_CAST(pysqlite_connection_exit), METH_FASTCALL, pysqlite_connection_exit__doc__},

static PyObject *
pysqlite_connection_exit_impl(pysqlite_Connection *self, PyObject *exc_type,
                              PyObject *exc_value, PyObject *exc_tb);

static PyValue
pysqlite_connection_exit(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_tb;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = PyValue_Object(args[0]);
    if (exc_type == NULL) {
        goto exit;
    }
    exc_value = PyValue_Object(args[1]);
    if (exc_value == NULL) {
        goto exit;
    }
    exc_tb = PyValue_Object(args[2]);
    if (exc_tb == NULL) {
        goto exit;
    }
    _return_value = pysqlite_connection_exit_impl((pysqlite_Connection *)self, exc_type, exc_value, exc_tb);
    return_value = PyValue_FromResult(_return_value);

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
    {"setlimit", _PyCFunction_CAST(setlimit), METH_FASTCALL, setlimit__doc__},

static PyObject *
setlimit_impl(pysqlite_Connection *self, int category, int limit);

static PyValue
setlimit(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int category;
    int limit;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("setlimit", nargs, 2, 2)) {
        goto exit;
    }
    category = PyLong_AsIntValue(args[0]);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    limit = PyLong_AsIntValue(args[1]);
    if (limit == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = setlimit_impl((pysqlite_Connection *)self, category, limit);
    return_value = PyValue_FromResult(_return_value);

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
    {"getlimit", _PyCFunction_CAST(getlimit), METH_O, getlimit__doc__},

static PyObject *
getlimit_impl(pysqlite_Connection *self, int category);

static PyValue
getlimit(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int category;
    PyObject *_return_value;

    category = PyLong_AsIntValue(arg);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = getlimit_impl((pysqlite_Connection *)self, category);
    return_value = PyValue_FromResult(_return_value);

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
    {"setconfig", _PyCFunction_CAST(setconfig), METH_FASTCALL, setconfig__doc__},

static PyObject *
setconfig_impl(pysqlite_Connection *self, int op, int enable);

static PyValue
setconfig(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int op;
    int enable = 1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("setconfig", nargs, 1, 2)) {
        goto exit;
    }
    op = PyLong_AsIntValue(args[0]);
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    enable = PyObject_IsTrueValue(args[1]);
    if (enable < 0) {
        goto exit;
    }
skip_optional:
    _return_value = setconfig_impl((pysqlite_Connection *)self, op, enable);
    return_value = PyValue_FromResult(_return_value);

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
    {"getconfig", _PyCFunction_CAST(getconfig), METH_O, getconfig__doc__},

static int
getconfig_impl(pysqlite_Connection *self, int op);

static PyValue
getconfig(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int op;
    int _return_value;

    op = PyLong_AsIntValue(arg);
    if (op == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = getconfig_impl((pysqlite_Connection *)self, op);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromResult(PyBool_FromLong((long)_return_value));

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
/*[clinic end generated code: output=c7a6524e9fe6efff input=a9049054013a1b77]*/
