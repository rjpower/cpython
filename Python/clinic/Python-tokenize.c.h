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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

static PyValue
tokenizeriter_new_impl(PyTypeObject *type, PyValueBorrow readline,
                       int extra_tokens, const char *encoding);

static PyValue
tokenizeriter_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(extra_tokens)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "extra_tokens", "encoding", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "tokenizeriter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow readline;
    int extra_tokens;
    const char *encoding = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    readline = PyValue_Borrow(args[_idx[0]]);
    extra_tokens = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
    if (extra_tokens < 0) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("tokenizeriter", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("tokenizeriter", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    return_value = tokenizeriter_new_impl(type, readline, extra_tokens, encoding);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
tokenizeriter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(tokenizeriter_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = tokenizeriter_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
tokenizeriter_next_impl(tokenizeriterobject *self);

static PyObject *
tokenizeriter_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = tokenizeriter_next_impl((tokenizeriterobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=931696e9f64b1f2b input=a9049054013a1b77]*/
