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
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(pysqlite_complete_statement__doc__,
"complete_statement($module, /, statement)\n"
"--\n"
"\n"
"Checks if a string contains a complete SQL statement.");

#define PYSQLITE_COMPLETE_STATEMENT_METHODDEF    \
    {"complete_statement", _PyCFunction_CAST(pysqlite_complete_statement), METH_VALUE|METH_KEYWORDS, pysqlite_complete_statement__doc__},

static PyValue
pysqlite_complete_statement_impl(PyObject *module, const char *statement);

static PyValue
pysqlite_complete_statement(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(statement)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"statement", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "complete_statement",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *statement;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("complete_statement", "argument 'statement'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("complete_statement", "argument 'statement'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    statement = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (statement == NULL) {
        goto exit;
    }
    return_value = pysqlite_complete_statement_impl(PyValue_AsKnownObjectBorrow(module), statement);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_register_adapter__doc__,
"register_adapter($module, type, adapter, /)\n"
"--\n"
"\n"
"Register a function to adapt Python objects to SQLite values.");

#define PYSQLITE_REGISTER_ADAPTER_METHODDEF    \
    {"register_adapter", _PyCFunction_CAST(pysqlite_register_adapter), METH_VALUE, pysqlite_register_adapter__doc__},

static PyValue
pysqlite_register_adapter_impl(PyObject *module, PyTypeObject *type,
                               PyValueBorrow caster);

static PyValue
pysqlite_register_adapter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *type;
    PyValueBorrow caster;

    if (!_PyArg_CheckPositional("register_adapter", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyType_Type)) {
        _PyArg_BadArgumentValue("register_adapter", "argument 1", (&PyType_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    type = (PyTypeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    caster = PyValue_Borrow(args[1]);
    return_value = pysqlite_register_adapter_impl(PyValue_AsKnownObjectBorrow(module), type, caster);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_register_converter__doc__,
"register_converter($module, typename, converter, /)\n"
"--\n"
"\n"
"Register a function to convert SQLite values to Python objects.");

#define PYSQLITE_REGISTER_CONVERTER_METHODDEF    \
    {"register_converter", _PyCFunction_CAST(pysqlite_register_converter), METH_VALUE, pysqlite_register_converter__doc__},

static PyValue
pysqlite_register_converter_impl(PyObject *module, PyObject *orig_name,
                                 PyValueBorrow callable);

static PyValue
pysqlite_register_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *orig_name;
    PyValueBorrow callable;

    if (!_PyArg_CheckPositional("register_converter", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("register_converter", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("register_converter", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    orig_name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    callable = PyValue_Borrow(args[1]);
    return_value = pysqlite_register_converter_impl(PyValue_AsKnownObjectBorrow(module), orig_name, callable);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_enable_callback_trace__doc__,
"enable_callback_tracebacks($module, enable, /)\n"
"--\n"
"\n"
"Enable or disable callback functions throwing errors to stderr.");

#define PYSQLITE_ENABLE_CALLBACK_TRACE_METHODDEF    \
    {"enable_callback_tracebacks", _PyCFunction_CAST(pysqlite_enable_callback_trace), METH_VALUE|METH_O, pysqlite_enable_callback_trace__doc__},

static PyValue
pysqlite_enable_callback_trace_impl(PyObject *module, int enable);

static PyValue
pysqlite_enable_callback_trace(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int enable;

    enable = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (enable == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pysqlite_enable_callback_trace_impl(PyValue_AsKnownObjectBorrow(module), enable);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_adapt__doc__,
"adapt($module, obj, proto=PrepareProtocolType, alt=<unrepresentable>, /)\n"
"--\n"
"\n"
"Adapt given object to given protocol.");

#define PYSQLITE_ADAPT_METHODDEF    \
    {"adapt", _PyCFunction_CAST(pysqlite_adapt), METH_VALUE, pysqlite_adapt__doc__},

static PyValue
pysqlite_adapt_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow proto,
                    PyValueBorrow alt);

static PyValue
pysqlite_adapt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    PyValueBorrow proto = PyValue_FromObjectBorrow((PyObject *)clinic_state()->PrepareProtocolType);
    PyValueBorrow alt = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("adapt", nargs, 1, 3)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    proto = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    alt = PyValue_Borrow(args[2]);
skip_optional:
    return_value = pysqlite_adapt_impl(PyValue_AsKnownObjectBorrow(module), obj, proto, alt);

exit:
    return return_value;
}
/*[clinic end generated code: output=300906a07bb026a8 input=a9049054013a1b77]*/
