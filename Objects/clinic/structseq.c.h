/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_value.h"         // _PyValue_RestoreException()

static PyObject *
structseq_new_impl(PyTypeObject *type, PyObject *arg, PyObject *dict);

static PyObject *
structseq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
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
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t noptargs;
    _PyArg_OwnerScope _args_scope = {0};
    PyObject *arg;
    PyObject *dict = NULL;

    if (_PyArg_OwnerScope_BeginTuple(&_args_scope, args, 2) < 0) {
        goto exit;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    fastargs = _PyArg_UnpackKeywordsOwned(&_args_scope, kwargs, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    noptargs = Py_MIN(_args_scope.nargs, 2)
        + (_args_scope.nobjects - _args_scope.nargs)
        - (1 + 0);
    arg = fastargs[0];
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    dict = fastargs[1];
    if (PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = structseq_new_impl(type, arg, dict);

exit:
    if (_PyArg_OwnerScope_End(&_args_scope) < 0) {
        PyObject *_saved = PyErr_GetRaisedException();
        Py_CLEAR(return_value);
        _PyValue_RestoreException(_saved);
    }
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
/*[clinic end generated code: output=0ddbdf1fe17c92d3 input=a9049054013a1b77]*/
