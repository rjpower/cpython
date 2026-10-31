/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_testconsole_write_input__doc__,
"write_input($module, /, file, s)\n"
"--\n"
"\n"
"Writes UTF-16-LE encoded bytes to the console as if typed by a user.");

#define _TESTCONSOLE_WRITE_INPUT_METHODDEF    \
    {"write_input", _PyCFunction_CAST(_testconsole_write_input), METH_FASTCALL|METH_KEYWORDS, _testconsole_write_input__doc__},

static PyObject *
_testconsole_write_input_impl(PyObject *module, PyObject *file, Py_buffer *s);

static PyValue
_testconsole_write_input(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(file), _Py_LATIN1_CHR('s'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "s", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "write_input",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    PyObject *file;
    Py_buffer s = {NULL, NULL};
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file = PyValue_Object(args[0]);
    if (file == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &s, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    _return_value = _testconsole_write_input_impl(module, file, &s);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for s */
    if (s.obj) {
       PyBuffer_Release(&s);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_testconsole_read_output__doc__,
"read_output($module, /, file)\n"
"--\n"
"\n"
"Reads a str from the console as written to stdout.");

#define _TESTCONSOLE_READ_OUTPUT_METHODDEF    \
    {"read_output", _PyCFunction_CAST(_testconsole_read_output), METH_FASTCALL|METH_KEYWORDS, _testconsole_read_output__doc__},

static PyObject *
_testconsole_read_output_impl(PyObject *module, PyObject *file);

static PyValue
_testconsole_read_output(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(file), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "read_output",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    PyObject *file;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file = PyValue_Object(args[0]);
    if (file == NULL) {
        goto exit;
    }
    _return_value = _testconsole_read_output_impl(module, file);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#ifndef _TESTCONSOLE_WRITE_INPUT_METHODDEF
    #define _TESTCONSOLE_WRITE_INPUT_METHODDEF
#endif /* !defined(_TESTCONSOLE_WRITE_INPUT_METHODDEF) */

#ifndef _TESTCONSOLE_READ_OUTPUT_METHODDEF
    #define _TESTCONSOLE_READ_OUTPUT_METHODDEF
#endif /* !defined(_TESTCONSOLE_READ_OUTPUT_METHODDEF) */
/*[clinic end generated code: output=ba0ac7e2e3ae6abf input=a9049054013a1b77]*/
