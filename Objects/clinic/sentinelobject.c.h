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
sentinel_new_impl(PyTypeObject *type, PyObject *name, PyObject *repr);

static PyObject *
sentinel_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(repr), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "repr", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sentinel",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t noptargs;
    _PyArg_OwnerScope _args_scope = {0};
    PyObject *name;
    PyObject *repr = Py_None;

    if (_PyArg_OwnerScope_BeginTuple(&_args_scope, args, 2) < 0) {
        goto exit;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    fastargs = _PyArg_UnpackKeywordsOwned(&_args_scope, kwargs, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    noptargs = Py_MIN(_args_scope.nargs, 1)
        + (_args_scope.nobjects - _args_scope.nargs)
        - (1 + 0);
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("sentinel", "argument 1", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    repr = fastargs[1];
    if (PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = sentinel_new_impl(type, name, repr);

exit:
    if (_PyArg_OwnerScope_End(&_args_scope) < 0) {
        PyObject *_saved = PyErr_GetRaisedException();
        Py_CLEAR(return_value);
        _PyValue_RestoreException(_saved);
    }
    return return_value;
}
/*[clinic end generated code: output=754dfaf6d4b2a85e input=a9049054013a1b77]*/
