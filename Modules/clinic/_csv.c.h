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
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_csv_Dialect___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Raise an exception to avoid pickling.");

#define _CSV_DIALECT___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(_csv_Dialect___reduce__), METH_VALUE|METH_NOARGS, _csv_Dialect___reduce____doc__},

static PyValue
_csv_Dialect___reduce___impl(DialectObj *self);

static PyValue
_csv_Dialect___reduce__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _csv_Dialect___reduce___impl((DialectObj *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_csv_Dialect___reduce_ex____doc__,
"__reduce_ex__($self, protocol, /)\n"
"--\n"
"\n"
"Raise an exception to avoid pickling.");

#define _CSV_DIALECT___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(_csv_Dialect___reduce_ex__), METH_VALUE|METH_O, _csv_Dialect___reduce_ex____doc__},

static PyValue
_csv_Dialect___reduce_ex___impl(DialectObj *self, PyValueBorrow protocol);

static PyValue
_csv_Dialect___reduce_ex__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow protocol;

    protocol = PyValue_Borrow(args[0]);
    return_value = _csv_Dialect___reduce_ex___impl((DialectObj *)PyValue_AsKnownObjectBorrow(self), protocol);

    return return_value;
}

PyDoc_STRVAR(_csv_Dialect___replace____doc__,
"__replace__($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy of the dialect with the specified options replaced.");

#define _CSV_DIALECT___REPLACE___METHODDEF    \
    {"__replace__", _PyCFunction_CAST(_csv_Dialect___replace__), METH_VALUE|METH_KEYWORDS, _csv_Dialect___replace____doc__},

static PyValue
_csv_Dialect___replace___impl(DialectObj *self, PyObject *changes);

static PyValue
_csv_Dialect___replace__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *changes = NULL;

    if (!_PyArg_CheckPositional("__replace__", nargs, 0, 0)) {
        goto exit;
    }
    changes = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (changes == NULL) {
        goto exit;
    }
    return_value = _csv_Dialect___replace___impl((DialectObj *)PyValue_AsKnownObjectBorrow(self), changes);

exit:
    /* Cleanup for changes */
    Py_XDECREF(changes);

    return return_value;
}

static PyObject *
Reader_iternext_impl(ReaderObj *self);

static PyObject *
Reader_iternext(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = Reader_iternext_impl((ReaderObj *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_csv_reader__doc__,
"reader($module, iterable, dialect=\'excel\', /, **fmtparams)\n"
"--\n"
"\n"
"Return a reader object that will process lines from the given iterable.\n"
"\n"
"The \"iterable\" argument can be any object that returns a line\n"
"of input for each iteration, such as a file object or a list.  The\n"
"optional \"dialect\" argument defines a CSV dialect.  The function\n"
"also accepts optional keyword arguments which override settings\n"
"provided by the dialect.\n"
"\n"
"The returned object is an iterator.  Each iteration returns a row\n"
"of the CSV file (which can span multiple input lines).");

#define _CSV_READER_METHODDEF    \
    {"reader", _PyCFunction_CAST(_csv_reader), METH_VALUE|METH_KEYWORDS, _csv_reader__doc__},

static PyValue
_csv_reader_impl(PyObject *module, PyValueBorrow iterable,
                 PyValueBorrow dialect, PyObject *fmtparams);

static PyValue
_csv_reader(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;
    PyValueBorrow dialect = PyValue_Borrow(PyValue_NULL);
    PyObject *fmtparams = NULL;

    if (!_PyArg_CheckPositional("reader", nargs, 1, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    dialect = PyValue_Borrow(args[1]);
skip_optional:
    fmtparams = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (fmtparams == NULL) {
        goto exit;
    }
    return_value = _csv_reader_impl(PyValue_AsKnownObjectBorrow(module), iterable, dialect, fmtparams);

exit:
    /* Cleanup for fmtparams */
    Py_XDECREF(fmtparams);

    return return_value;
}

PyDoc_STRVAR(_csv_writer__doc__,
"writer($module, fileobj, dialect=\'excel\', /, **fmtparams)\n"
"--\n"
"\n"
"Return a writer object writing user data to the given file object.\n"
"\n"
"The \"fileobj\" argument can be any object that supports the file API.\n"
"The optional \"dialect\" argument defines a CSV dialect.  The function\n"
"also accepts optional keyword arguments which override settings\n"
"provided by the dialect.");

#define _CSV_WRITER_METHODDEF    \
    {"writer", _PyCFunction_CAST(_csv_writer), METH_VALUE|METH_KEYWORDS, _csv_writer__doc__},

static PyValue
_csv_writer_impl(PyObject *module, PyValueBorrow output_file,
                 PyValueBorrow dialect, PyObject *fmtparams);

static PyValue
_csv_writer(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow output_file;
    PyValueBorrow dialect = PyValue_Borrow(PyValue_NULL);
    PyObject *fmtparams = NULL;

    if (!_PyArg_CheckPositional("writer", nargs, 1, 2)) {
        goto exit;
    }
    output_file = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    dialect = PyValue_Borrow(args[1]);
skip_optional:
    fmtparams = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (fmtparams == NULL) {
        goto exit;
    }
    return_value = _csv_writer_impl(PyValue_AsKnownObjectBorrow(module), output_file, dialect, fmtparams);

exit:
    /* Cleanup for fmtparams */
    Py_XDECREF(fmtparams);

    return return_value;
}

PyDoc_STRVAR(_csv_list_dialects__doc__,
"list_dialects($module, /)\n"
"--\n"
"\n"
"Return a list of all known dialect names.");

#define _CSV_LIST_DIALECTS_METHODDEF    \
    {"list_dialects", _PyCFunction_CAST(_csv_list_dialects), METH_VALUE|METH_NOARGS, _csv_list_dialects__doc__},

static PyValue
_csv_list_dialects_impl(PyObject *module);

static PyValue
_csv_list_dialects(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _csv_list_dialects_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_csv_register_dialect__doc__,
"register_dialect($module, name, dialect=\'excel\', /, **fmtparams)\n"
"--\n"
"\n"
"Create a mapping from a string name to a CVS dialect.\n"
"\n"
"The optional \"dialect\" argument specifies the base dialect instance\n"
"or the name of the registered dialect.  The function also accepts\n"
"optional keyword arguments which override settings provided by the\n"
"dialect.");

#define _CSV_REGISTER_DIALECT_METHODDEF    \
    {"register_dialect", _PyCFunction_CAST(_csv_register_dialect), METH_VALUE|METH_KEYWORDS, _csv_register_dialect__doc__},

static PyValue
_csv_register_dialect_impl(PyObject *module, PyValueBorrow name_obj,
                           PyValueBorrow dialect_obj, PyObject *fmtparams);

static PyValue
_csv_register_dialect(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow name_obj;
    PyValueBorrow dialect_obj = PyValue_Borrow(PyValue_NULL);
    PyObject *fmtparams = NULL;

    if (!_PyArg_CheckPositional("register_dialect", nargs, 1, 2)) {
        goto exit;
    }
    name_obj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    dialect_obj = PyValue_Borrow(args[1]);
skip_optional:
    fmtparams = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (fmtparams == NULL) {
        goto exit;
    }
    return_value = _csv_register_dialect_impl(PyValue_AsKnownObjectBorrow(module), name_obj, dialect_obj, fmtparams);

exit:
    /* Cleanup for fmtparams */
    Py_XDECREF(fmtparams);

    return return_value;
}

PyDoc_STRVAR(_csv_unregister_dialect__doc__,
"unregister_dialect($module, /, name)\n"
"--\n"
"\n"
"Delete the name/dialect mapping associated with a string name.");

#define _CSV_UNREGISTER_DIALECT_METHODDEF    \
    {"unregister_dialect", _PyCFunction_CAST(_csv_unregister_dialect), METH_VALUE|METH_KEYWORDS, _csv_unregister_dialect__doc__},

static PyValue
_csv_unregister_dialect_impl(PyObject *module, PyValueBorrow name);

static PyValue
_csv_unregister_dialect(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "unregister_dialect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow name;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    name = PyValue_Borrow(args[_idx[0]]);
    return_value = _csv_unregister_dialect_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_csv_get_dialect__doc__,
"get_dialect($module, /, name)\n"
"--\n"
"\n"
"Return the dialect instance associated with name.");

#define _CSV_GET_DIALECT_METHODDEF    \
    {"get_dialect", _PyCFunction_CAST(_csv_get_dialect), METH_VALUE|METH_KEYWORDS, _csv_get_dialect__doc__},

static PyValue
_csv_get_dialect_impl(PyObject *module, PyValueBorrow name);

static PyValue
_csv_get_dialect(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "get_dialect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow name;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    name = PyValue_Borrow(args[_idx[0]]);
    return_value = _csv_get_dialect_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_csv_field_size_limit__doc__,
"field_size_limit($module, /, new_limit=<unrepresentable>)\n"
"--\n"
"\n"
"Sets an upper limit on parsed fields.\n"
"\n"
"Returns old limit. If limit is not given, no new limit is set and\n"
"the old limit is returned");

#define _CSV_FIELD_SIZE_LIMIT_METHODDEF    \
    {"field_size_limit", _PyCFunction_CAST(_csv_field_size_limit), METH_VALUE|METH_KEYWORDS, _csv_field_size_limit__doc__},

static PyValue
_csv_field_size_limit_impl(PyObject *module, PyValueBorrow new_limit);

static PyValue
_csv_field_size_limit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(new_limit)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"new_limit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "field_size_limit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow new_limit = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        new_limit = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = _csv_field_size_limit_impl(PyValue_AsKnownObjectBorrow(module), new_limit);

exit:
    return return_value;
}
/*[clinic end generated code: output=505d51236adb4cb0 input=a9049054013a1b77]*/
