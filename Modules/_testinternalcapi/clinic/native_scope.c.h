/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_testinternalcapi_native_scope_flat__doc__,
"native_scope_flat($module, /, value, scale=1, *, callback=None)\n"
"--\n"
"\n"
"Exercise a flat keyword parser with caller-owned argument views.");

#define _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_METHODDEF    \
    {"native_scope_flat", _PyCFunction_CAST(_testinternalcapi_native_scope_flat), METH_FASTCALL|METH_KEYWORDS, _testinternalcapi_native_scope_flat__doc__},

static PyObject *
_testinternalcapi_native_scope_flat_impl(PyObject *module, PyObject *value,
                                         int scale, PyObject *callback);

static PyObject *
_testinternalcapi_native_scope_flat(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(value)), _PyValue_STATIC_IMMORTAL(&_Py_ID(scale)), _PyValue_STATIC_IMMORTAL(&_Py_ID(callback)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"value", "scale", "callback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "native_scope_flat",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *value;
    int scale = 1;
    PyObject *callback = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        scale = PyLong_AsInt(args[1]);
        if (scale == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    callback = args[2];
skip_optional_kwonly:
    return_value = _testinternalcapi_native_scope_flat_impl(module, value, scale, callback);

exit:
    return return_value;
}

PyDoc_STRVAR(_testinternalcapi_native_scope_flat_noargs__doc__,
"native_scope_flat_noargs($module, /)\n"
"--\n"
"\n"
"Return a multi-digit scalar through a genuine NOARGS wrapper.");

#define _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_NOARGS_METHODDEF    \
    {"native_scope_flat_noargs", (PyCFunction)_testinternalcapi_native_scope_flat_noargs, METH_NOARGS, _testinternalcapi_native_scope_flat_noargs__doc__},

static PyObject *
_testinternalcapi_native_scope_flat_noargs_impl(PyObject *module);

static PyObject *
_testinternalcapi_native_scope_flat_noargs(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _testinternalcapi_native_scope_flat_noargs_impl(module);
}

PyDoc_STRVAR(_testinternalcapi_native_scope_flat_fast__doc__,
"native_scope_flat_fast($module, left, right, /)\n"
"--\n"
"\n"
"Add two objects through a genuine FASTCALL wrapper.");

#define _TESTINTERNALCAPI_NATIVE_SCOPE_FLAT_FAST_METHODDEF    \
    {"native_scope_flat_fast", _PyCFunction_CAST(_testinternalcapi_native_scope_flat_fast), METH_FASTCALL, _testinternalcapi_native_scope_flat_fast__doc__},

static PyObject *
_testinternalcapi_native_scope_flat_fast_impl(PyObject *module,
                                              PyObject *left,
                                              PyObject *right);

static PyObject *
_testinternalcapi_native_scope_flat_fast(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *left;
    PyObject *right;

    if (!_PyArg_CheckPositional("native_scope_flat_fast", nargs, 2, 2)) {
        goto exit;
    }
    left = args[0];
    right = args[1];
    return_value = _testinternalcapi_native_scope_flat_fast_impl(module, left, right);

exit:
    return return_value;
}
/*[clinic end generated code: output=70db6e8450c337ec input=a9049054013a1b77]*/
