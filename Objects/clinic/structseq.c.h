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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

static PyValue
structseq_new_impl(PyTypeObject *type, PyValueBorrow arg, PyValueBorrow dict);

static PyValue
structseq_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sequence)), _PyValue_STATIC_IMMORTAL(&_Py_ID(dict)), },
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
    Py_ssize_t _idx[2];
    PyValueBorrow arg;
    PyValueBorrow dict = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    arg = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        dict = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = structseq_new_impl(type, arg, dict);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
structseq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(structseq_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = structseq_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(structseq___replace____doc__,
"__replace__($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy with new values for the specified fields.");

#define STRUCTSEQ___REPLACE___METHODDEF    \
    {"__replace__", _PyCFunction_CAST(structseq___replace__), METH_VALUE, structseq___replace____doc__},

static PyValue
structseq___replace___impl(PyStructSequence *self, PyObject *changes);

static PyValue
structseq___replace__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = structseq___replace___impl((PyStructSequence *)PyValue_AsKnownObjectBorrow(self), changes);

exit:
    /* Cleanup for changes */
    Py_XDECREF(changes);

    return return_value;
}
/*[clinic end generated code: output=927d6664d68d15fd input=a9049054013a1b77]*/
