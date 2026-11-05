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
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_bisect_bisect_right__doc__,
"bisect_right($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Return the index where to insert item x in list a, assuming a is sorted.\n"
"\n"
"The return value i is such that all e in a[:i] have e <= x, and all e in\n"
"a[i:] have e > x.  So if x already appears in the list, a.insert(i, x)\n"
"will insert just after the rightmost x already there.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_BISECT_RIGHT_METHODDEF    \
    {"bisect_right", _PyCFunction_CAST(_bisect_bisect_right), METH_VALUE, _bisect_bisect_right__doc__},

static Py_ssize_t
_bisect_bisect_right_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                          Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key);

static PyValue
_bisect_bisect_right(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('x')), _PyValue_STATIC_IMMORTAL(&_Py_ID(lo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hi)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bisect_right",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow a;
    PyValueBorrow x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyValueBorrow key = PyValue_Borrow(PyValue_None);
    Py_ssize_t _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    x = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
            if (lo < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "lo cannot be negative");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[_idx[3]]), &hi)) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        key = PyValue_Borrow(args[_idx[4]]);
    }
    _return_value = _bisect_bisect_right_impl(PyValue_AsKnownObjectBorrow(module), a, x, lo, hi, key);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_bisect_insort_right__doc__,
"insort_right($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Insert item x in list a, and keep it sorted assuming a is sorted.\n"
"\n"
"If x is already in a, insert it to the right of the rightmost x.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_INSORT_RIGHT_METHODDEF    \
    {"insort_right", _PyCFunction_CAST(_bisect_insort_right), METH_VALUE, _bisect_insort_right__doc__},

static PyValue
_bisect_insort_right_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                          Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key);

static PyValue
_bisect_insort_right(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('x')), _PyValue_STATIC_IMMORTAL(&_Py_ID(lo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hi)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "insort_right",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow a;
    PyValueBorrow x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyValueBorrow key = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    x = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
            if (lo < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "lo cannot be negative");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[_idx[3]]), &hi)) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        key = PyValue_Borrow(args[_idx[4]]);
    }
    return_value = _bisect_insort_right_impl(PyValue_AsKnownObjectBorrow(module), a, x, lo, hi, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_bisect_bisect_left__doc__,
"bisect_left($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Return the index where to insert item x in list a, assuming a is sorted.\n"
"\n"
"The return value i is such that all e in a[:i] have e < x, and all e in\n"
"a[i:] have e >= x.  So if x already appears in the list, a.insert(i, x)\n"
"will insert just before the leftmost x already there.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_BISECT_LEFT_METHODDEF    \
    {"bisect_left", _PyCFunction_CAST(_bisect_bisect_left), METH_VALUE, _bisect_bisect_left__doc__},

static Py_ssize_t
_bisect_bisect_left_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                         Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key);

static PyValue
_bisect_bisect_left(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('x')), _PyValue_STATIC_IMMORTAL(&_Py_ID(lo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hi)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bisect_left",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow a;
    PyValueBorrow x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyValueBorrow key = PyValue_Borrow(PyValue_None);
    Py_ssize_t _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    x = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
            if (lo < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "lo cannot be negative");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[_idx[3]]), &hi)) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        key = PyValue_Borrow(args[_idx[4]]);
    }
    _return_value = _bisect_bisect_left_impl(PyValue_AsKnownObjectBorrow(module), a, x, lo, hi, key);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_bisect_insort_left__doc__,
"insort_left($module, /, a, x, lo=0, hi=None, *, key=None)\n"
"--\n"
"\n"
"Insert item x in list a, and keep it sorted assuming a is sorted.\n"
"\n"
"If x is already in a, insert it to the left of the leftmost x.\n"
"\n"
"Optional args lo (default 0) and hi (default len(a)) bound the\n"
"slice of a to be searched.\n"
"\n"
"A custom key function can be supplied to customize the sort order.");

#define _BISECT_INSORT_LEFT_METHODDEF    \
    {"insort_left", _PyCFunction_CAST(_bisect_insort_left), METH_VALUE, _bisect_insort_left__doc__},

static PyValue
_bisect_insort_left_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                         Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key);

static PyValue
_bisect_insort_left(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('x')), _PyValue_STATIC_IMMORTAL(&_Py_ID(lo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hi)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "x", "lo", "hi", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "insort_left",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow a;
    PyValueBorrow x;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    PyValueBorrow key = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    x = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            lo = ival;
            if (lo < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "lo cannot be negative");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[_idx[3]]), &hi)) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        key = PyValue_Borrow(args[_idx[4]]);
    }
    return_value = _bisect_insort_left_impl(PyValue_AsKnownObjectBorrow(module), a, x, lo, hi, key);

exit:
    return return_value;
}
/*[clinic end generated code: output=4a64c77906099392 input=a9049054013a1b77]*/
