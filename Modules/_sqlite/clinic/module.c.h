/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(pysqlite_complete_statement__doc__,
"complete_statement($module, /, statement)\n"
"--\n"
"\n"
"Checks if a string contains a complete SQL statement.");

#define PYSQLITE_COMPLETE_STATEMENT_METHODDEF    \
    {"complete_statement", _PyCFunction_CAST(pysqlite_complete_statement), METH_FASTCALL|METH_KEYWORDS, pysqlite_complete_statement__doc__},

static PyObject *
pysqlite_complete_statement_impl(PyObject *module, const char *statement);

static PyValue
pysqlite_complete_statement(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(statement), },
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
    PyValue argsbuf[1];
    const char *statement;
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
            _PyArg_BadArgument("complete_statement", "argument 'statement'", "str", _arg);
            goto exit;
        }
        statement = _PyUnicode_AsUTF8NoNUL(_arg);
        if (statement == NULL) {
            goto exit;
        }
    }
    _return_value = pysqlite_complete_statement_impl(module, statement);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_register_adapter__doc__,
"register_adapter($module, type, adapter, /)\n"
"--\n"
"\n"
"Register a function to adapt Python objects to SQLite values.");

#define PYSQLITE_REGISTER_ADAPTER_METHODDEF    \
    {"register_adapter", _PyCFunction_CAST(pysqlite_register_adapter), METH_FASTCALL, pysqlite_register_adapter__doc__},

static PyObject *
pysqlite_register_adapter_impl(PyObject *module, PyTypeObject *type,
                               PyObject *caster);

static PyValue
pysqlite_register_adapter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *type;
    PyObject *caster;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("register_adapter", nargs, 2, 2)) {
        goto exit;
    }
    type = (PyTypeObject *)PyValue_Object(args[0]);
    if (type == NULL) {
        goto exit;
    }
    caster = PyValue_Object(args[1]);
    if (caster == NULL) {
        goto exit;
    }
    _return_value = pysqlite_register_adapter_impl(module, type, caster);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_register_converter__doc__,
"register_converter($module, typename, converter, /)\n"
"--\n"
"\n"
"Register a function to convert SQLite values to Python objects.");

#define PYSQLITE_REGISTER_CONVERTER_METHODDEF    \
    {"register_converter", _PyCFunction_CAST(pysqlite_register_converter), METH_FASTCALL, pysqlite_register_converter__doc__},

static PyObject *
pysqlite_register_converter_impl(PyObject *module, PyObject *orig_name,
                                 PyObject *callable);

static PyValue
pysqlite_register_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *orig_name;
    PyObject *callable;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("register_converter", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("register_converter", "argument 1", "str", _arg);
            goto exit;
        }
        orig_name = _arg;
    }
    callable = PyValue_Object(args[1]);
    if (callable == NULL) {
        goto exit;
    }
    _return_value = pysqlite_register_converter_impl(module, orig_name, callable);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_enable_callback_trace__doc__,
"enable_callback_tracebacks($module, enable, /)\n"
"--\n"
"\n"
"Enable or disable callback functions throwing errors to stderr.");

#define PYSQLITE_ENABLE_CALLBACK_TRACE_METHODDEF    \
    {"enable_callback_tracebacks", _PyCFunction_CAST(pysqlite_enable_callback_trace), METH_O, pysqlite_enable_callback_trace__doc__},

static PyObject *
pysqlite_enable_callback_trace_impl(PyObject *module, int enable);

static PyValue
pysqlite_enable_callback_trace(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int enable;
    PyObject *_return_value;

    enable = PyLong_AsIntValue(arg);
    if (enable == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = pysqlite_enable_callback_trace_impl(module, enable);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(pysqlite_adapt__doc__,
"adapt($module, obj, proto=PrepareProtocolType, alt=<unrepresentable>, /)\n"
"--\n"
"\n"
"Adapt given object to given protocol.");

#define PYSQLITE_ADAPT_METHODDEF    \
    {"adapt", _PyCFunction_CAST(pysqlite_adapt), METH_FASTCALL, pysqlite_adapt__doc__},

static PyObject *
pysqlite_adapt_impl(PyObject *module, PyObject *obj, PyObject *proto,
                    PyObject *alt);

static PyValue
pysqlite_adapt(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *proto = (PyObject *)clinic_state()->PrepareProtocolType;
    PyObject *alt = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("adapt", nargs, 1, 3)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    proto = PyValue_Object(args[1]);
    if (proto == NULL) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    alt = PyValue_Object(args[2]);
    if (alt == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = pysqlite_adapt_impl(module, obj, proto, alt);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=d1484b447c4c7d68 input=a9049054013a1b77]*/
