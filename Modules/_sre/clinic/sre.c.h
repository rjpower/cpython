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
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_sre_getcodesize__doc__,
"getcodesize($module, /)\n"
"--\n"
"\n");

#define _SRE_GETCODESIZE_METHODDEF    \
    {"getcodesize", _PyCFunction_CAST(_sre_getcodesize), METH_VALUE|METH_NOARGS, _sre_getcodesize__doc__},

static int
_sre_getcodesize_impl(PyObject *module);

static PyValue
_sre_getcodesize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = _sre_getcodesize_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_ascii_iscased__doc__,
"ascii_iscased($module, character, /)\n"
"--\n"
"\n");

#define _SRE_ASCII_ISCASED_METHODDEF    \
    {"ascii_iscased", _PyCFunction_CAST(_sre_ascii_iscased), METH_VALUE|METH_O, _sre_ascii_iscased__doc__},

static int
_sre_ascii_iscased_impl(PyObject *module, int character);

static PyValue
_sre_ascii_iscased(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int character;
    int _return_value;

    character = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_ascii_iscased_impl(PyValue_AsKnownObjectBorrow(module), character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_unicode_iscased__doc__,
"unicode_iscased($module, character, /)\n"
"--\n"
"\n");

#define _SRE_UNICODE_ISCASED_METHODDEF    \
    {"unicode_iscased", _PyCFunction_CAST(_sre_unicode_iscased), METH_VALUE|METH_O, _sre_unicode_iscased__doc__},

static int
_sre_unicode_iscased_impl(PyObject *module, int character);

static PyValue
_sre_unicode_iscased(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int character;
    int _return_value;

    character = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_unicode_iscased_impl(PyValue_AsKnownObjectBorrow(module), character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_ascii_tolower__doc__,
"ascii_tolower($module, character, /)\n"
"--\n"
"\n");

#define _SRE_ASCII_TOLOWER_METHODDEF    \
    {"ascii_tolower", _PyCFunction_CAST(_sre_ascii_tolower), METH_VALUE|METH_O, _sre_ascii_tolower__doc__},

static int
_sre_ascii_tolower_impl(PyObject *module, int character);

static PyValue
_sre_ascii_tolower(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int character;
    int _return_value;

    character = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_ascii_tolower_impl(PyValue_AsKnownObjectBorrow(module), character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_unicode_tolower__doc__,
"unicode_tolower($module, character, /)\n"
"--\n"
"\n");

#define _SRE_UNICODE_TOLOWER_METHODDEF    \
    {"unicode_tolower", _PyCFunction_CAST(_sre_unicode_tolower), METH_VALUE|METH_O, _sre_unicode_tolower__doc__},

static int
_sre_unicode_tolower_impl(PyObject *module, int character);

static PyValue
_sre_unicode_tolower(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int character;
    int _return_value;

    character = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_unicode_tolower_impl(PyValue_AsKnownObjectBorrow(module), character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_prefixmatch__doc__,
"prefixmatch($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Matches zero or more characters at the beginning of the string.");

#define _SRE_SRE_PATTERN_PREFIXMATCH_METHODDEF    \
    {"prefixmatch", _PyCFunction_CAST(_sre_SRE_Pattern_prefixmatch), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_prefixmatch__doc__},

static PyValue
_sre_SRE_Pattern_prefixmatch_impl(PatternObject *self, PyTypeObject *cls,
                                  PyValueBorrow string, Py_ssize_t pos,
                                  Py_ssize_t endpos);

static PyValue
_sre_SRE_Pattern_prefixmatch(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pos)), _PyValue_STATIC_IMMORTAL(&_Py_ID(endpos)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "prefixmatch",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
    }
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
            endpos = ival;
        }
    }
    return_value = _sre_SRE_Pattern_prefixmatch_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_fullmatch__doc__,
"fullmatch($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Matches against all of the string.");

#define _SRE_SRE_PATTERN_FULLMATCH_METHODDEF    \
    {"fullmatch", _PyCFunction_CAST(_sre_SRE_Pattern_fullmatch), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_fullmatch__doc__},

static PyValue
_sre_SRE_Pattern_fullmatch_impl(PatternObject *self, PyTypeObject *cls,
                                PyValueBorrow string, Py_ssize_t pos,
                                Py_ssize_t endpos);

static PyValue
_sre_SRE_Pattern_fullmatch(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pos)), _PyValue_STATIC_IMMORTAL(&_Py_ID(endpos)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fullmatch",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
    }
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
            endpos = ival;
        }
    }
    return_value = _sre_SRE_Pattern_fullmatch_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_search__doc__,
"search($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Scan through string looking for a match, and return a corresponding match object instance.\n"
"\n"
"Return None if no position in the string matches.");

#define _SRE_SRE_PATTERN_SEARCH_METHODDEF    \
    {"search", _PyCFunction_CAST(_sre_SRE_Pattern_search), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_search__doc__},

static PyValue
_sre_SRE_Pattern_search_impl(PatternObject *self, PyTypeObject *cls,
                             PyValueBorrow string, Py_ssize_t pos,
                             Py_ssize_t endpos);

static PyValue
_sre_SRE_Pattern_search(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pos)), _PyValue_STATIC_IMMORTAL(&_Py_ID(endpos)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "search",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
    }
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
            endpos = ival;
        }
    }
    return_value = _sre_SRE_Pattern_search_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_findall__doc__,
"findall($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Return a list of all non-overlapping matches of pattern in string.");

#define _SRE_SRE_PATTERN_FINDALL_METHODDEF    \
    {"findall", _PyCFunction_CAST(_sre_SRE_Pattern_findall), METH_VALUE|METH_KEYWORDS, _sre_SRE_Pattern_findall__doc__},

static PyValue
_sre_SRE_Pattern_findall_impl(PatternObject *self, PyValueBorrow string,
                              Py_ssize_t pos, Py_ssize_t endpos);

static PyValue
_sre_SRE_Pattern_findall(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pos)), _PyValue_STATIC_IMMORTAL(&_Py_ID(endpos)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "findall",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
    }
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
            endpos = ival;
        }
    }
    return_value = _sre_SRE_Pattern_findall_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_finditer__doc__,
"finditer($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Return an iterator over all non-overlapping matches for the RE pattern in string.\n"
"\n"
"For each match, the iterator returns a match object.");

#define _SRE_SRE_PATTERN_FINDITER_METHODDEF    \
    {"finditer", _PyCFunction_CAST(_sre_SRE_Pattern_finditer), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_finditer__doc__},

static PyValue
_sre_SRE_Pattern_finditer_impl(PatternObject *self, PyTypeObject *cls,
                               PyValueBorrow string, Py_ssize_t pos,
                               Py_ssize_t endpos);

static PyValue
_sre_SRE_Pattern_finditer(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pos)), _PyValue_STATIC_IMMORTAL(&_Py_ID(endpos)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "finditer",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
    }
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
            endpos = ival;
        }
    }
    return_value = _sre_SRE_Pattern_finditer_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_scanner__doc__,
"scanner($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN_SCANNER_METHODDEF    \
    {"scanner", _PyCFunction_CAST(_sre_SRE_Pattern_scanner), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_scanner__doc__},

static PyValue
_sre_SRE_Pattern_scanner_impl(PatternObject *self, PyTypeObject *cls,
                              PyValueBorrow string, Py_ssize_t pos,
                              Py_ssize_t endpos);

static PyValue
_sre_SRE_Pattern_scanner(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pos)), _PyValue_STATIC_IMMORTAL(&_Py_ID(endpos)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "scanner",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
    }
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
            endpos = ival;
        }
    }
    return_value = _sre_SRE_Pattern_scanner_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_split__doc__,
"split($self, /, string, maxsplit=0)\n"
"--\n"
"\n"
"Split string by the occurrences of pattern.");

#define _SRE_SRE_PATTERN_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(_sre_SRE_Pattern_split), METH_VALUE|METH_KEYWORDS, _sre_SRE_Pattern_split__doc__},

static PyValue
_sre_SRE_Pattern_split_impl(PatternObject *self, PyValueBorrow string,
                            Py_ssize_t maxsplit);

static PyValue
_sre_SRE_Pattern_split(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxsplit)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "maxsplit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "split",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow string;
    Py_ssize_t maxsplit = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    string = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            maxsplit = ival;
        }
    }
    return_value = _sre_SRE_Pattern_split_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), string, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_sub__doc__,
"sub($self, /, repl, string, count=0)\n"
"--\n"
"\n"
"Return the string obtained by replacing the leftmost non-overlapping occurrences of pattern in string by the replacement repl.");

#define _SRE_SRE_PATTERN_SUB_METHODDEF    \
    {"sub", _PyCFunction_CAST(_sre_SRE_Pattern_sub), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_sub__doc__},

static PyValue
_sre_SRE_Pattern_sub_impl(PatternObject *self, PyTypeObject *cls,
                          PyValueBorrow repl, PyValueBorrow string,
                          Py_ssize_t count);

static PyValue
_sre_SRE_Pattern_sub(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(repl)), _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(count)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"repl", "string", "count", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sub",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow repl;
    PyValueBorrow string;
    Py_ssize_t count = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    repl = PyValue_Borrow(args[_idx[0]]);
    string = PyValue_Borrow(args[_idx[1]]);
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
            count = ival;
        }
    }
    return_value = _sre_SRE_Pattern_sub_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, repl, string, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_subn__doc__,
"subn($self, /, repl, string, count=0)\n"
"--\n"
"\n"
"Return the tuple (new_string, number_of_subs_made) found by replacing the leftmost non-overlapping occurrences of pattern with the replacement repl.");

#define _SRE_SRE_PATTERN_SUBN_METHODDEF    \
    {"subn", _PyCFunction_CAST(_sre_SRE_Pattern_subn), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Pattern_subn__doc__},

static PyValue
_sre_SRE_Pattern_subn_impl(PatternObject *self, PyTypeObject *cls,
                           PyValueBorrow repl, PyValueBorrow string,
                           Py_ssize_t count);

static PyValue
_sre_SRE_Pattern_subn(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(repl)), _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), _PyValue_STATIC_IMMORTAL(&_Py_ID(count)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"repl", "string", "count", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "subn",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow repl;
    PyValueBorrow string;
    Py_ssize_t count = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    repl = PyValue_Borrow(args[_idx[0]]);
    string = PyValue_Borrow(args[_idx[1]]);
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
            count = ival;
        }
    }
    return_value = _sre_SRE_Pattern_subn_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), cls, repl, string, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(_sre_SRE_Pattern___copy__), METH_VALUE|METH_NOARGS, _sre_SRE_Pattern___copy____doc__},

static PyValue
_sre_SRE_Pattern___copy___impl(PatternObject *self);

static PyValue
_sre_SRE_Pattern___copy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _sre_SRE_Pattern___copy___impl((PatternObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(_sre_SRE_Pattern___deepcopy__), METH_VALUE|METH_O, _sre_SRE_Pattern___deepcopy____doc__},

static PyValue
_sre_SRE_Pattern___deepcopy___impl(PatternObject *self, PyValueBorrow memo);

static PyValue
_sre_SRE_Pattern___deepcopy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow memo;

    memo = PyValue_Borrow(args[0]);
    return_value = _sre_SRE_Pattern___deepcopy___impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), memo);

    return return_value;
}

#if defined(Py_DEBUG)

PyDoc_STRVAR(_sre_SRE_Pattern__fail_after__doc__,
"_fail_after($self, count, exception, /)\n"
"--\n"
"\n"
"For debugging.");

#define _SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF    \
    {"_fail_after", _PyCFunction_CAST(_sre_SRE_Pattern__fail_after), METH_VALUE, _sre_SRE_Pattern__fail_after__doc__},

static PyValue
_sre_SRE_Pattern__fail_after_impl(PatternObject *self, int count,
                                  PyValueBorrow exception);

static PyValue
_sre_SRE_Pattern__fail_after(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int count;
    PyValueBorrow exception;

    if (!_PyArg_CheckPositional("_fail_after", nargs, 2, 2)) {
        goto exit;
    }
    count = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (count == -1 && PyErr_Occurred()) {
        goto exit;
    }
    exception = PyValue_Borrow(args[1]);
    return_value = _sre_SRE_Pattern__fail_after_impl((PatternObject *)PyValue_AsKnownObjectBorrow(self), count, exception);

exit:
    return return_value;
}

#endif /* defined(Py_DEBUG) */

PyDoc_STRVAR(_sre_compile__doc__,
"compile($module, /, pattern, flags, code, groups, groupindex,\n"
"        indexgroup)\n"
"--\n"
"\n");

#define _SRE_COMPILE_METHODDEF    \
    {"compile", _PyCFunction_CAST(_sre_compile), METH_VALUE|METH_KEYWORDS, _sre_compile__doc__},

static PyValue
_sre_compile_impl(PyObject *module, PyValueBorrow pattern, int flags,
                  PyObject *code, Py_ssize_t groups, PyObject *groupindex,
                  PyObject *indexgroup);

static PyValue
_sre_compile(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pattern)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), _PyValue_STATIC_IMMORTAL(&_Py_ID(code)), _PyValue_STATIC_IMMORTAL(&_Py_ID(groups)), _PyValue_STATIC_IMMORTAL(&_Py_ID(groupindex)), _PyValue_STATIC_IMMORTAL(&_Py_ID(indexgroup)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pattern", "flags", "code", "groups", "groupindex", "indexgroup", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyValueBorrow pattern;
    int flags;
    PyObject *code;
    Py_ssize_t groups;
    PyObject *groupindex;
    PyObject *indexgroup;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 6, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    pattern = PyValue_Borrow(args[_idx[0]]);
    flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[2]])), &PyList_Type)) {
        _PyArg_BadArgumentValue("compile", "argument 'code'", "list", PyValue_Borrow(args[_idx[2]]));
        goto exit;
    }
    code = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[3]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        groups = ival;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[4]])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("compile", "argument 'groupindex'", "dict", PyValue_Borrow(args[_idx[4]]));
        goto exit;
    }
    groupindex = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[5]])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("compile", "argument 'indexgroup'", "tuple", PyValue_Borrow(args[_idx[5]]));
        goto exit;
    }
    indexgroup = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[5]]));
    return_value = _sre_compile_impl(PyValue_AsKnownObjectBorrow(module), pattern, flags, code, groups, groupindex, indexgroup);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_template__doc__,
"template($module, pattern, template, /)\n"
"--\n"
"\n"
"\n"
"\n"
"  template\n"
"    A list containing interleaved literal strings (str or bytes) and group\n"
"    indices (int), as returned by re._parser.parse_template():\n"
"        [literal1, group1, ..., literalN, groupN]");

#define _SRE_TEMPLATE_METHODDEF    \
    {"template", _PyCFunction_CAST(_sre_template), METH_VALUE, _sre_template__doc__},

static PyValue
_sre_template_impl(PyObject *module, PyValueBorrow pattern,
                   PyObject *template);

static PyValue
_sre_template(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow pattern;
    PyObject *template;

    if (!_PyArg_CheckPositional("template", nargs, 2, 2)) {
        goto exit;
    }
    pattern = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyList_Type)) {
        _PyArg_BadArgumentValue("template", "argument 2", "list", PyValue_Borrow(args[1]));
        goto exit;
    }
    template = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _sre_template_impl(PyValue_AsKnownObjectBorrow(module), pattern, template);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_expand__doc__,
"expand($self, /, template)\n"
"--\n"
"\n"
"Return the string obtained by doing backslash substitution on the string template, as done by the sub() method.");

#define _SRE_SRE_MATCH_EXPAND_METHODDEF    \
    {"expand", _PyCFunction_CAST(_sre_SRE_Match_expand), METH_VALUE|METH_KEYWORDS, _sre_SRE_Match_expand__doc__},

static PyValue
_sre_SRE_Match_expand_impl(MatchObject *self, PyValueBorrow template);

static PyValue
_sre_SRE_Match_expand(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(template)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"template", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "expand",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow template;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    template = PyValue_Borrow(args[_idx[0]]);
    return_value = _sre_SRE_Match_expand_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), template);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_group__doc__,
"group($self, /, *args)\n"
"--\n"
"\n"
"Return subgroup(s) of the match by indices or names.\n"
"\n"
"For 0 returns the entire match.");

#define _SRE_SRE_MATCH_GROUP_METHODDEF    \
    {"group", _PyCFunction_CAST(_sre_SRE_Match_group), METH_VALUE, _sre_SRE_Match_group__doc__},

static PyValue
_sre_SRE_Match_group_impl(MatchObject *self, const PyValue *args,
                          Py_ssize_t args_length);

static PyValue
_sre_SRE_Match_group(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    __clinic_args = args;
    args_length = nargs;
    return_value = _sre_SRE_Match_group_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), __clinic_args, args_length);

    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_groups__doc__,
"groups($self, /, default=None)\n"
"--\n"
"\n"
"Return a tuple containing all the subgroups of the match, from 1.\n"
"\n"
"  default\n"
"    Is used for groups that did not participate in the match.");

#define _SRE_SRE_MATCH_GROUPS_METHODDEF    \
    {"groups", _PyCFunction_CAST(_sre_SRE_Match_groups), METH_VALUE|METH_KEYWORDS, _sre_SRE_Match_groups__doc__},

static PyValue
_sre_SRE_Match_groups_impl(MatchObject *self, PyValueBorrow default_value);

static PyValue
_sre_SRE_Match_groups(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "groups",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        default_value = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = _sre_SRE_Match_groups_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_groupdict__doc__,
"groupdict($self, /, default=None)\n"
"--\n"
"\n"
"Return a dictionary containing all the named subgroups of the match, keyed by the subgroup name.\n"
"\n"
"  default\n"
"    Is used for groups that did not participate in the match.");

#define _SRE_SRE_MATCH_GROUPDICT_METHODDEF    \
    {"groupdict", _PyCFunction_CAST(_sre_SRE_Match_groupdict), METH_VALUE|METH_KEYWORDS, _sre_SRE_Match_groupdict__doc__},

static PyValue
_sre_SRE_Match_groupdict_impl(MatchObject *self, PyValueBorrow default_value);

static PyValue
_sre_SRE_Match_groupdict(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "groupdict",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        default_value = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = _sre_SRE_Match_groupdict_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_start__doc__,
"start($self, group=0, /)\n"
"--\n"
"\n"
"Return index of the start of the substring matched by group.");

#define _SRE_SRE_MATCH_START_METHODDEF    \
    {"start", _PyCFunction_CAST(_sre_SRE_Match_start), METH_VALUE, _sre_SRE_Match_start__doc__},

static Py_ssize_t
_sre_SRE_Match_start_impl(MatchObject *self, PyValueBorrow group);

static PyValue
_sre_SRE_Match_start(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow group = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("start", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    group = PyValue_Borrow(args[0]);
skip_optional:
    _return_value = _sre_SRE_Match_start_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), group);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_end__doc__,
"end($self, group=0, /)\n"
"--\n"
"\n"
"Return index of the end of the substring matched by group.");

#define _SRE_SRE_MATCH_END_METHODDEF    \
    {"end", _PyCFunction_CAST(_sre_SRE_Match_end), METH_VALUE, _sre_SRE_Match_end__doc__},

static Py_ssize_t
_sre_SRE_Match_end_impl(MatchObject *self, PyValueBorrow group);

static PyValue
_sre_SRE_Match_end(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow group = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("end", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    group = PyValue_Borrow(args[0]);
skip_optional:
    _return_value = _sre_SRE_Match_end_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), group);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_span__doc__,
"span($self, group=0, /)\n"
"--\n"
"\n"
"For match object m, return the 2-tuple (m.start(group), m.end(group)).");

#define _SRE_SRE_MATCH_SPAN_METHODDEF    \
    {"span", _PyCFunction_CAST(_sre_SRE_Match_span), METH_VALUE, _sre_SRE_Match_span__doc__},

static PyValue
_sre_SRE_Match_span_impl(MatchObject *self, PyValueBorrow group);

static PyValue
_sre_SRE_Match_span(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow group = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("span", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    group = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _sre_SRE_Match_span_impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), group);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_MATCH___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(_sre_SRE_Match___copy__), METH_VALUE|METH_NOARGS, _sre_SRE_Match___copy____doc__},

static PyValue
_sre_SRE_Match___copy___impl(MatchObject *self);

static PyValue
_sre_SRE_Match___copy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _sre_SRE_Match___copy___impl((MatchObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _SRE_SRE_MATCH___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(_sre_SRE_Match___deepcopy__), METH_VALUE|METH_O, _sre_SRE_Match___deepcopy____doc__},

static PyValue
_sre_SRE_Match___deepcopy___impl(MatchObject *self, PyValueBorrow memo);

static PyValue
_sre_SRE_Match___deepcopy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow memo;

    memo = PyValue_Borrow(args[0]);
    return_value = _sre_SRE_Match___deepcopy___impl((MatchObject *)PyValue_AsKnownObjectBorrow(self), memo);

    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Scanner_prefixmatch__doc__,
"prefixmatch($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_SCANNER_PREFIXMATCH_METHODDEF    \
    {"prefixmatch", _PyCFunction_CAST(_sre_SRE_Scanner_prefixmatch), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Scanner_prefixmatch__doc__},

static PyValue
_sre_SRE_Scanner_prefixmatch_impl(ScannerObject *self, PyTypeObject *cls);

static PyValue
_sre_SRE_Scanner_prefixmatch(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "prefixmatch() takes no arguments");
        goto exit;
    }
    return_value = _sre_SRE_Scanner_prefixmatch_impl((ScannerObject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Scanner_search__doc__,
"search($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_SCANNER_SEARCH_METHODDEF    \
    {"search", _PyCFunction_CAST(_sre_SRE_Scanner_search), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sre_SRE_Scanner_search__doc__},

static PyValue
_sre_SRE_Scanner_search_impl(ScannerObject *self, PyTypeObject *cls);

static PyValue
_sre_SRE_Scanner_search(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "search() takes no arguments");
        goto exit;
    }
    return_value = _sre_SRE_Scanner_search_impl((ScannerObject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

#ifndef _SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF
    #define _SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF
#endif /* !defined(_SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF) */
/*[clinic end generated code: output=a9e5c026ab3b1584 input=a9049054013a1b77]*/
