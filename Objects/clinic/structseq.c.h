/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

static PyObject *
structseq_new_impl(PyTypeObject *type, PyObject *arg, PyObject *dict);

static PyObject *
structseq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sequence), &_Py_ID(dict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sequence", "dict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "structseq",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *arg;
    PyObject *dict = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    arg = PyValue_Object(fastargs[0]);
    if (arg == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    dict = PyValue_Object(fastargs[1]);
    if (dict == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = structseq_new_impl(type, arg, dict);

exit:
    return return_value;
}

PyDoc_STRVAR(structseq___replace____doc__,
"__replace__($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy with new values for the specified fields.");

#define STRUCTSEQ___REPLACE___METHODDEF    \
    {"__replace__", _PyCFunction_CAST(structseq___replace__), METH_VARARGS|METH_KEYWORDS, structseq___replace____doc__},

static PyObject *
structseq___replace___impl(PyStructSequence *self, PyObject *changes);

static PyObject *
structseq___replace__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *changes = NULL;

    if (!_PyArg_NoPositional("__replace__", args)) {
        goto exit;
    }
    if (kwargs == NULL) {
        changes = PyDict_New();
        if (changes == NULL) {
            goto exit;
        }
    }
    else {
        changes = Py_NewRef(kwargs);
    }
    return_value = structseq___replace___impl((PyStructSequence *)self, changes);

exit:
    /* Cleanup for changes */
    Py_XDECREF(changes);

    return return_value;
}
/*[clinic end generated code: output=a5cf0c57c2e2c55e input=a9049054013a1b77]*/
