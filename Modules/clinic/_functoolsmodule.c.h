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
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_functools_cmp_to_key__doc__,
"cmp_to_key($module, /, mycmp)\n"
"--\n"
"\n"
"Convert a cmp= function into a key= function.\n"
"\n"
"  mycmp\n"
"    Function that compares two objects.");

#define _FUNCTOOLS_CMP_TO_KEY_METHODDEF    \
    {"cmp_to_key", _PyCFunction_CAST(_functools_cmp_to_key), METH_VALUE, _functools_cmp_to_key__doc__},

static PyValue
_functools_cmp_to_key_impl(PyObject *module, PyValueBorrow mycmp);

static PyValue
_functools_cmp_to_key(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(mycmp)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mycmp", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cmp_to_key",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow mycmp;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    mycmp = PyValue_Borrow(args[_idx[0]]);
    return_value = _functools_cmp_to_key_impl(PyValue_AsKnownObjectBorrow(module), mycmp);

exit:
    return return_value;
}

PyDoc_STRVAR(_functools_reduce__doc__,
"reduce($module, function, iterable, /,\n"
"       initial=functools._initial_missing)\n"
"--\n"
"\n"
"Apply a function of two arguments cumulatively to the items of an iterable, from left to right.\n"
"\n"
"This effectively reduces the iterable to a single value.  If initial is\n"
"present, it is placed before the items of the iterable in the\n"
"calculation, and serves as a default when the iterable is empty.\n"
"\n"
"For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5])\n"
"calculates ((((1 + 2) + 3) + 4) + 5).");

#define _FUNCTOOLS_REDUCE_METHODDEF    \
    {"reduce", _PyCFunction_CAST(_functools_reduce), METH_VALUE, _functools_reduce__doc__},

static PyValue
_functools_reduce_impl(PyObject *module, PyValueBorrow func,
                       PyValueBorrow seq, PyValueBorrow result);

static PyValue
_functools_reduce(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(initial)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "initial", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "reduce",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow func;
    PyValueBorrow seq;
    PyValueBorrow result = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    func = PyValue_Borrow(args[_idx[0]]);
    seq = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        result = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _functools_reduce_impl(PyValue_AsKnownObjectBorrow(module), func, seq, result);

exit:
    return return_value;
}

PyDoc_STRVAR(_functools__lru_cache_wrapper_cache_info__doc__,
"cache_info($self, /)\n"
"--\n"
"\n"
"Report cache statistics");

#define _FUNCTOOLS__LRU_CACHE_WRAPPER_CACHE_INFO_METHODDEF    \
    {"cache_info", _PyCFunction_CAST(_functools__lru_cache_wrapper_cache_info), METH_VALUE|METH_NOARGS, _functools__lru_cache_wrapper_cache_info__doc__},

static PyValue
_functools__lru_cache_wrapper_cache_info_impl(PyObject *self);

static PyValue
_functools__lru_cache_wrapper_cache_info(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _functools__lru_cache_wrapper_cache_info_impl(PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_functools__lru_cache_wrapper_cache_clear__doc__,
"cache_clear($self, /)\n"
"--\n"
"\n"
"Clear the cache and cache statistics");

#define _FUNCTOOLS__LRU_CACHE_WRAPPER_CACHE_CLEAR_METHODDEF    \
    {"cache_clear", _PyCFunction_CAST(_functools__lru_cache_wrapper_cache_clear), METH_VALUE|METH_NOARGS, _functools__lru_cache_wrapper_cache_clear__doc__},

static PyValue
_functools__lru_cache_wrapper_cache_clear_impl(PyObject *self);

static PyValue
_functools__lru_cache_wrapper_cache_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _functools__lru_cache_wrapper_cache_clear_impl(PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_functools__lru_cache_wrapper___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Return a copy of the wrapper, which is the wrapper itself.");

#define _FUNCTOOLS__LRU_CACHE_WRAPPER___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(_functools__lru_cache_wrapper___copy__), METH_VALUE|METH_NOARGS, _functools__lru_cache_wrapper___copy____doc__},

static PyValue
_functools__lru_cache_wrapper___copy___impl(PyObject *self);

static PyValue
_functools__lru_cache_wrapper___copy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _functools__lru_cache_wrapper___copy___impl(PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_functools__lru_cache_wrapper___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n"
"Return a deep copy of the wrapper, which is the wrapper itself.");

#define _FUNCTOOLS__LRU_CACHE_WRAPPER___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(_functools__lru_cache_wrapper___deepcopy__), METH_VALUE|METH_O, _functools__lru_cache_wrapper___deepcopy____doc__},

static PyValue
_functools__lru_cache_wrapper___deepcopy___impl(PyObject *self,
                                                PyValueBorrow memo);

static PyValue
_functools__lru_cache_wrapper___deepcopy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow memo;

    memo = PyValue_Borrow(args[0]);
    return_value = _functools__lru_cache_wrapper___deepcopy___impl(PyValue_AsKnownObjectBorrow(self), memo);

    return return_value;
}
/*[clinic end generated code: output=9fd8ee0c02276835 input=a9049054013a1b77]*/
