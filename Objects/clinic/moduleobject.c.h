/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_value.h"         // _PyValue_RestoreException()

PyDoc_STRVAR(module___init____doc__,
"module(name, doc=None)\n"
"--\n"
"\n"
"Create a module object.\n"
"\n"
"The name must be a string; the optional doc argument can have any type.");

static int
module___init___impl(PyModuleObject *self, PyObject *name, PyObject *doc);

static int
module___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { &_Py_ID(name), &_Py_ID(doc), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "doc", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "module",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t noptargs;
    _PyArg_OwnerScope _args_scope = {0};
    PyObject *name;
    PyObject *doc = Py_None;

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
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("module", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    name = fastargs[0];
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    doc = fastargs[1];
    if (PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = module___init___impl((PyModuleObject *)self, name, doc);

exit:
    if (_PyArg_OwnerScope_End(&_args_scope) < 0) {
        return_value = -1;
    }
    return return_value;
}
/*[clinic end generated code: output=cb132ffa6836f316 input=a9049054013a1b77]*/
