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
#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(EncodingMap_size__doc__,
"size($self, /)\n"
"--\n"
"\n"
"Return the size (in bytes) of this object.");

#define ENCODINGMAP_SIZE_METHODDEF    \
    {"size", _PyCFunction_CAST(EncodingMap_size), METH_VALUE, EncodingMap_size__doc__},

static PyValue
EncodingMap_size_impl(struct encoding_map *self);

static PyValue
EncodingMap_size(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&EncodingMapType, "size", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = EncodingMap_size_impl((struct encoding_map *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_title__doc__,
"title($self, /)\n"
"--\n"
"\n"
"Return a version of the string where each word is titlecased.\n"
"\n"
"More specifically, words start with uppercased characters and all\n"
"remaining cased characters have lower case.");

#define UNICODE_TITLE_METHODDEF    \
    {"title", _PyCFunction_CAST(unicode_title), METH_VALUE, unicode_title__doc__},

static PyValue
unicode_title_impl(PyObject *self);

static PyValue
unicode_title(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "title", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_title_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_capitalize__doc__,
"capitalize($self, /)\n"
"--\n"
"\n"
"Return a capitalized version of the string.\n"
"\n"
"More specifically, make the first character have upper case and the\n"
"rest lower case.");

#define UNICODE_CAPITALIZE_METHODDEF    \
    {"capitalize", _PyCFunction_CAST(unicode_capitalize), METH_VALUE, unicode_capitalize__doc__},

static PyValue
unicode_capitalize_impl(PyObject *self);

static PyValue
unicode_capitalize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "capitalize", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_capitalize_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_casefold__doc__,
"casefold($self, /)\n"
"--\n"
"\n"
"Return a version of the string suitable for caseless comparisons.");

#define UNICODE_CASEFOLD_METHODDEF    \
    {"casefold", _PyCFunction_CAST(unicode_casefold), METH_VALUE, unicode_casefold__doc__},

static PyValue
unicode_casefold_impl(PyObject *self);

static PyValue
unicode_casefold(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "casefold", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_casefold_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_center__doc__,
"center($self, width, fillchar=\' \', /)\n"
"--\n"
"\n"
"Return a centered string of length width.\n"
"\n"
"Padding is done using the specified fill character (default is\n"
"a space).");

#define UNICODE_CENTER_METHODDEF    \
    {"center", _PyCFunction_CAST(unicode_center), METH_VALUE, unicode_center__doc__},

static PyValue
unicode_center_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar);

static PyValue
unicode_center(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "center", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("center", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!convert_uc(PyValue_Borrow(args[1]), &fillchar)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_center_impl(PyValue_AsKnownObjectBorrow(self), width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_count__doc__,
"count($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the number of non-overlapping occurrences of substring sub in string S[start:end].\n"
"\n"
"Optional arguments start and end are interpreted as in slice\n"
"notation.");

#define UNICODE_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(unicode_count), METH_VALUE, unicode_count__doc__},

static Py_ssize_t
unicode_count_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end);

static PyValue
unicode_count(PyValueBorrow str, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *substr;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "count", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("count", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("count", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("count", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    substr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    _return_value = unicode_count_impl(PyValue_AsKnownObjectBorrow(str), substr, start, end);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_encode__doc__,
"encode($self, /, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Encode the string using the codec registered for encoding.\n"
"\n"
"  encoding\n"
"    The encoding in which to encode the string.\n"
"  errors\n"
"    The error handling scheme to use for encoding errors.\n"
"    The default is \'strict\' meaning that encoding errors raise a\n"
"    UnicodeEncodeError.  Other possible values are \'ignore\', \'replace\'\n"
"    and \'xmlcharrefreplace\' as well as any other name registered with\n"
"    codecs.register_error that can handle UnicodeEncodeErrors.");

#define UNICODE_ENCODE_METHODDEF    \
    {"encode", _PyCFunction_CAST(unicode_encode), METH_VALUE, unicode_encode__doc__},

static PyValue
unicode_encode_impl(PyObject *self, const char *encoding, const char *errors);

static PyValue
unicode_encode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "encode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("encode", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("encode", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("encode", "argument 'errors'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("encode", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = unicode_encode_impl(PyValue_AsKnownObjectBorrow(self), encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_expandtabs__doc__,
"expandtabs($self, /, tabsize=8)\n"
"--\n"
"\n"
"Return a copy where all tab characters are expanded using spaces.\n"
"\n"
"If tabsize is not given, a tab size of 8 characters is assumed.");

#define UNICODE_EXPANDTABS_METHODDEF    \
    {"expandtabs", _PyCFunction_CAST(unicode_expandtabs), METH_VALUE, unicode_expandtabs__doc__},

static PyValue
unicode_expandtabs_impl(PyObject *self, int tabsize);

static PyValue
unicode_expandtabs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(tabsize)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tabsize", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "expandtabs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int tabsize = 8;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        tabsize = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (tabsize == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = unicode_expandtabs_impl(PyValue_AsKnownObjectBorrow(self), tabsize);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_find__doc__,
"find($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in S where substring sub is found, such that sub is contained within S[start:end].\n"
"\n"
"Optional arguments start and end are interpreted as in slice\n"
"notation.  Return -1 on failure.");

#define UNICODE_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(unicode_find), METH_VALUE, unicode_find__doc__},

static Py_ssize_t
unicode_find_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                  Py_ssize_t end);

static PyValue
unicode_find(PyValueBorrow str, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *substr;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "find", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("find", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("find", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    substr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    _return_value = unicode_find_impl(PyValue_AsKnownObjectBorrow(str), substr, start, end);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_index__doc__,
"index($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in S where substring sub is found, such that sub is contained within S[start:end].\n"
"\n"
"Optional arguments start and end are interpreted as in slice\n"
"notation.  Raises ValueError when the substring is not found.");

#define UNICODE_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(unicode_index), METH_VALUE, unicode_index__doc__},

static Py_ssize_t
unicode_index_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end);

static PyValue
unicode_index(PyValueBorrow str, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *substr;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "index", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("index", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("index", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    substr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    _return_value = unicode_index_impl(PyValue_AsKnownObjectBorrow(str), substr, start, end);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isascii__doc__,
"isascii($self, /)\n"
"--\n"
"\n"
"Return True if all characters in the string are ASCII, False otherwise.\n"
"\n"
"ASCII characters have code points in the range U+0000-U+007F.\n"
"Empty string is ASCII too.");

#define UNICODE_ISASCII_METHODDEF    \
    {"isascii", _PyCFunction_CAST(unicode_isascii), METH_VALUE, unicode_isascii__doc__},

static PyValue
unicode_isascii_impl(PyObject *self);

static PyValue
unicode_isascii(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isascii", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isascii_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_islower__doc__,
"islower($self, /)\n"
"--\n"
"\n"
"Return True if the string is a lowercase string, False otherwise.\n"
"\n"
"A string is lowercase if all cased characters in the string are\n"
"lowercase and there is at least one cased character in the string.");

#define UNICODE_ISLOWER_METHODDEF    \
    {"islower", _PyCFunction_CAST(unicode_islower), METH_VALUE, unicode_islower__doc__},

static PyValue
unicode_islower_impl(PyObject *self);

static PyValue
unicode_islower(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "islower", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_islower_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isupper__doc__,
"isupper($self, /)\n"
"--\n"
"\n"
"Return True if the string is an uppercase string, False otherwise.\n"
"\n"
"A string is uppercase if all cased characters in the string are\n"
"uppercase and there is at least one cased character in the string.");

#define UNICODE_ISUPPER_METHODDEF    \
    {"isupper", _PyCFunction_CAST(unicode_isupper), METH_VALUE, unicode_isupper__doc__},

static PyValue
unicode_isupper_impl(PyObject *self);

static PyValue
unicode_isupper(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isupper", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isupper_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_istitle__doc__,
"istitle($self, /)\n"
"--\n"
"\n"
"Return True if the string is a title-cased string, False otherwise.\n"
"\n"
"In a title-cased string, upper- and title-case characters may only\n"
"follow uncased characters and lowercase characters only cased ones.");

#define UNICODE_ISTITLE_METHODDEF    \
    {"istitle", _PyCFunction_CAST(unicode_istitle), METH_VALUE, unicode_istitle__doc__},

static PyValue
unicode_istitle_impl(PyObject *self);

static PyValue
unicode_istitle(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "istitle", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_istitle_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isspace__doc__,
"isspace($self, /)\n"
"--\n"
"\n"
"Return True if the string is a whitespace string, False otherwise.\n"
"\n"
"A string is whitespace if all characters in the string are\n"
"whitespace and there is at least one character in the string.");

#define UNICODE_ISSPACE_METHODDEF    \
    {"isspace", _PyCFunction_CAST(unicode_isspace), METH_VALUE, unicode_isspace__doc__},

static PyValue
unicode_isspace_impl(PyObject *self);

static PyValue
unicode_isspace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isspace", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isspace_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isalpha__doc__,
"isalpha($self, /)\n"
"--\n"
"\n"
"Return True if the string is an alphabetic string, False otherwise.\n"
"\n"
"A string is alphabetic if all characters in the string are\n"
"alphabetic and there is at least one character in the string.");

#define UNICODE_ISALPHA_METHODDEF    \
    {"isalpha", _PyCFunction_CAST(unicode_isalpha), METH_VALUE, unicode_isalpha__doc__},

static PyValue
unicode_isalpha_impl(PyObject *self);

static PyValue
unicode_isalpha(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isalpha", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isalpha_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isalnum__doc__,
"isalnum($self, /)\n"
"--\n"
"\n"
"Return True if the string is an alpha-numeric string, False otherwise.\n"
"\n"
"A string is alpha-numeric if all characters in the string are\n"
"alpha-numeric and there is at least one character in the string.");

#define UNICODE_ISALNUM_METHODDEF    \
    {"isalnum", _PyCFunction_CAST(unicode_isalnum), METH_VALUE, unicode_isalnum__doc__},

static PyValue
unicode_isalnum_impl(PyObject *self);

static PyValue
unicode_isalnum(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isalnum", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isalnum_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isdecimal__doc__,
"isdecimal($self, /)\n"
"--\n"
"\n"
"Return True if the string is a decimal string, False otherwise.\n"
"\n"
"A string is a decimal string if all characters in the string are\n"
"decimal and there is at least one character in the string.");

#define UNICODE_ISDECIMAL_METHODDEF    \
    {"isdecimal", _PyCFunction_CAST(unicode_isdecimal), METH_VALUE, unicode_isdecimal__doc__},

static PyValue
unicode_isdecimal_impl(PyObject *self);

static PyValue
unicode_isdecimal(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isdecimal", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isdecimal_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isdigit__doc__,
"isdigit($self, /)\n"
"--\n"
"\n"
"Return True if the string is a digit string, False otherwise.\n"
"\n"
"A string is a digit string if all characters in the string are\n"
"digits and there is at least one character in the string.");

#define UNICODE_ISDIGIT_METHODDEF    \
    {"isdigit", _PyCFunction_CAST(unicode_isdigit), METH_VALUE, unicode_isdigit__doc__},

static PyValue
unicode_isdigit_impl(PyObject *self);

static PyValue
unicode_isdigit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isdigit", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isdigit_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isnumeric__doc__,
"isnumeric($self, /)\n"
"--\n"
"\n"
"Return True if the string is a numeric string, False otherwise.\n"
"\n"
"A string is numeric if all characters in the string are numeric and\n"
"there is at least one character in the string.");

#define UNICODE_ISNUMERIC_METHODDEF    \
    {"isnumeric", _PyCFunction_CAST(unicode_isnumeric), METH_VALUE, unicode_isnumeric__doc__},

static PyValue
unicode_isnumeric_impl(PyObject *self);

static PyValue
unicode_isnumeric(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isnumeric", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isnumeric_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isidentifier__doc__,
"isidentifier($self, /)\n"
"--\n"
"\n"
"Return True if the string is a valid Python identifier, False otherwise.\n"
"\n"
"Call keyword.iskeyword(s) to test whether string s is a reserved\n"
"identifier, such as \"def\" or \"class\".");

#define UNICODE_ISIDENTIFIER_METHODDEF    \
    {"isidentifier", _PyCFunction_CAST(unicode_isidentifier), METH_VALUE, unicode_isidentifier__doc__},

static PyValue
unicode_isidentifier_impl(PyObject *self);

static PyValue
unicode_isidentifier(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isidentifier", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isidentifier_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_isprintable__doc__,
"isprintable($self, /)\n"
"--\n"
"\n"
"Return True if all characters in the string are printable, False otherwise.\n"
"\n"
"A character is printable if repr() may use it in its output.");

#define UNICODE_ISPRINTABLE_METHODDEF    \
    {"isprintable", _PyCFunction_CAST(unicode_isprintable), METH_VALUE, unicode_isprintable__doc__},

static PyValue
unicode_isprintable_impl(PyObject *self);

static PyValue
unicode_isprintable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "isprintable", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_isprintable_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_join__doc__,
"join($self, iterable, /)\n"
"--\n"
"\n"
"Concatenate any number of strings.\n"
"\n"
"The string whose method is called is inserted in between each given\n"
"string.  The result is returned as a new string.\n"
"\n"
"Example: \'.\'.join([\'ab\', \'pq\', \'rs\']) -> \'ab.pq.rs\'");

#define UNICODE_JOIN_METHODDEF    \
    {"join", _PyCFunction_CAST(unicode_join), METH_VALUE, unicode_join__doc__},

static PyValue
unicode_join_impl(PyObject *self, PyValueBorrow iterable);

static PyValue
unicode_join(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "join", nargs, kwnames, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = unicode_join_impl(PyValue_AsKnownObjectBorrow(self), iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_ljust__doc__,
"ljust($self, width, fillchar=\' \', /)\n"
"--\n"
"\n"
"Return a left-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character (default is\n"
"a space).");

#define UNICODE_LJUST_METHODDEF    \
    {"ljust", _PyCFunction_CAST(unicode_ljust), METH_VALUE, unicode_ljust__doc__},

static PyValue
unicode_ljust_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar);

static PyValue
unicode_ljust(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "ljust", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ljust", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!convert_uc(PyValue_Borrow(args[1]), &fillchar)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_ljust_impl(PyValue_AsKnownObjectBorrow(self), width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_lower__doc__,
"lower($self, /)\n"
"--\n"
"\n"
"Return a copy of the string converted to lowercase.");

#define UNICODE_LOWER_METHODDEF    \
    {"lower", _PyCFunction_CAST(unicode_lower), METH_VALUE, unicode_lower__doc__},

static PyValue
unicode_lower_impl(PyObject *self);

static PyValue
unicode_lower(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "lower", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_lower_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_strip__doc__,
"strip($self, chars=None, /)\n"
"--\n"
"\n"
"Return a copy of the string with leading and trailing whitespace removed.\n"
"\n"
"If chars is given and not None, remove characters in chars instead.");

#define UNICODE_STRIP_METHODDEF    \
    {"strip", _PyCFunction_CAST(unicode_strip), METH_VALUE, unicode_strip__doc__},

static PyValue
unicode_strip_impl(PyObject *self, PyValueBorrow chars);

static PyValue
unicode_strip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow chars = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "strip", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("strip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    chars = PyValue_Borrow(args[0]);
skip_optional:
    return_value = unicode_strip_impl(PyValue_AsKnownObjectBorrow(self), chars);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_lstrip__doc__,
"lstrip($self, chars=None, /)\n"
"--\n"
"\n"
"Return a copy of the string with leading whitespace removed.\n"
"\n"
"If chars is given and not None, remove characters in chars instead.");

#define UNICODE_LSTRIP_METHODDEF    \
    {"lstrip", _PyCFunction_CAST(unicode_lstrip), METH_VALUE, unicode_lstrip__doc__},

static PyValue
unicode_lstrip_impl(PyObject *self, PyValueBorrow chars);

static PyValue
unicode_lstrip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow chars = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "lstrip", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("lstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    chars = PyValue_Borrow(args[0]);
skip_optional:
    return_value = unicode_lstrip_impl(PyValue_AsKnownObjectBorrow(self), chars);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rstrip__doc__,
"rstrip($self, chars=None, /)\n"
"--\n"
"\n"
"Return a copy of the string with trailing whitespace removed.\n"
"\n"
"If chars is given and not None, remove characters in chars instead.");

#define UNICODE_RSTRIP_METHODDEF    \
    {"rstrip", _PyCFunction_CAST(unicode_rstrip), METH_VALUE, unicode_rstrip__doc__},

static PyValue
unicode_rstrip_impl(PyObject *self, PyValueBorrow chars);

static PyValue
unicode_rstrip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow chars = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "rstrip", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    chars = PyValue_Borrow(args[0]);
skip_optional:
    return_value = unicode_rstrip_impl(PyValue_AsKnownObjectBorrow(self), chars);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_replace__doc__,
"replace($self, old, new, /, count=-1)\n"
"--\n"
"\n"
"Return a copy with all occurrences of substring old replaced by new.\n"
"\n"
"  count\n"
"    Maximum number of occurrences to replace.\n"
"    -1 (the default value) means replace all occurrences.\n"
"\n"
"If count is given, only the first count occurrences are replaced.\n"
"If count is not specified or -1, then all occurrences are replaced.");

#define UNICODE_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(unicode_replace), METH_VALUE, unicode_replace__doc__},

static PyValue
unicode_replace_impl(PyObject *self, PyObject *old, PyObject *new,
                     Py_ssize_t count);

static PyValue
unicode_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(count)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "count", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyObject *old;
    PyObject *new;
    Py_ssize_t count = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("replace", "argument 1", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("replace", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    old = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        _PyArg_BadArgumentValue("replace", "argument 2", "str", PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
        _PyArg_BadArgument("replace", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        goto exit;
    }
    new = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
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
    return_value = unicode_replace_impl(PyValue_AsKnownObjectBorrow(self), old, new, count);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_removeprefix__doc__,
"removeprefix($self, prefix, /)\n"
"--\n"
"\n"
"Return a str with the given prefix string removed if present.\n"
"\n"
"If the string starts with the prefix string, return\n"
"string[len(prefix):].  Otherwise, return a copy of the original\n"
"string.");

#define UNICODE_REMOVEPREFIX_METHODDEF    \
    {"removeprefix", _PyCFunction_CAST(unicode_removeprefix), METH_VALUE, unicode_removeprefix__doc__},

static PyValue
unicode_removeprefix_impl(PyObject *self, PyObject *prefix);

static PyValue
unicode_removeprefix(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *prefix;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "removeprefix", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("removeprefix", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("removeprefix", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    prefix = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = unicode_removeprefix_impl(PyValue_AsKnownObjectBorrow(self), prefix);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_removesuffix__doc__,
"removesuffix($self, suffix, /)\n"
"--\n"
"\n"
"Return a str with the given suffix string removed if present.\n"
"\n"
"If the string ends with the suffix string and that suffix is not\n"
"empty, return string[:-len(suffix)].  Otherwise, return a copy of\n"
"the original string.");

#define UNICODE_REMOVESUFFIX_METHODDEF    \
    {"removesuffix", _PyCFunction_CAST(unicode_removesuffix), METH_VALUE, unicode_removesuffix__doc__},

static PyValue
unicode_removesuffix_impl(PyObject *self, PyObject *suffix);

static PyValue
unicode_removesuffix(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *suffix;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "removesuffix", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("removesuffix", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("removesuffix", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    suffix = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = unicode_removesuffix_impl(PyValue_AsKnownObjectBorrow(self), suffix);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rfind__doc__,
"rfind($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in S where substring sub is found, such that sub is contained within S[start:end].\n"
"\n"
"Optional arguments start and end are interpreted as in slice\n"
"notation.  Return -1 on failure.");

#define UNICODE_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(unicode_rfind), METH_VALUE, unicode_rfind__doc__},

static Py_ssize_t
unicode_rfind_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                   Py_ssize_t end);

static PyValue
unicode_rfind(PyValueBorrow str, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *substr;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "rfind", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("rfind", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("rfind", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    substr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    _return_value = unicode_rfind_impl(PyValue_AsKnownObjectBorrow(str), substr, start, end);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rindex__doc__,
"rindex($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in S where substring sub is found, such that sub is contained within S[start:end].\n"
"\n"
"Optional arguments start and end are interpreted as in slice\n"
"notation.  Raises ValueError when the substring is not found.");

#define UNICODE_RINDEX_METHODDEF    \
    {"rindex", _PyCFunction_CAST(unicode_rindex), METH_VALUE, unicode_rindex__doc__},

static Py_ssize_t
unicode_rindex_impl(PyObject *str, PyObject *substr, Py_ssize_t start,
                    Py_ssize_t end);

static PyValue
unicode_rindex(PyValueBorrow str, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *substr;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    Py_ssize_t _return_value;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "rindex", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rindex", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("rindex", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("rindex", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    substr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    _return_value = unicode_rindex_impl(PyValue_AsKnownObjectBorrow(str), substr, start, end);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rjust__doc__,
"rjust($self, width, fillchar=\' \', /)\n"
"--\n"
"\n"
"Return a right-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character (default is\n"
"a space).");

#define UNICODE_RJUST_METHODDEF    \
    {"rjust", _PyCFunction_CAST(unicode_rjust), METH_VALUE, unicode_rjust__doc__},

static PyValue
unicode_rjust_impl(PyObject *self, Py_ssize_t width, Py_UCS4 fillchar);

static PyValue
unicode_rjust(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    Py_UCS4 fillchar = ' ';

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "rjust", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rjust", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!convert_uc(PyValue_Borrow(args[1]), &fillchar)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_rjust_impl(PyValue_AsKnownObjectBorrow(self), width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_split__doc__,
"split($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the substrings in the string, using sep as the separator string.\n"
"\n"
"  sep\n"
"    The separator used to split the string.\n"
"\n"
"    When set to None (the default value), will split on any\n"
"    whitespace character (including \\n \\r \\t \\f and spaces) and\n"
"    will discard empty strings from the result.\n"
"  maxsplit\n"
"    Maximum number of splits.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splitting starts at the front of the string and works to the end.\n"
"\n"
"Note, str.split() is mainly useful for data that has been\n"
"intentionally delimited.  With natural text that includes\n"
"punctuation, consider using the regular expression module.");

#define UNICODE_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(unicode_split), METH_VALUE, unicode_split__doc__},

static PyValue
unicode_split_impl(PyObject *self, PyValueBorrow sep, Py_ssize_t maxsplit);

static PyValue
unicode_split(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxsplit)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "split",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow sep = PyValue_Borrow(PyValue_None);
    Py_ssize_t maxsplit = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        sep = PyValue_Borrow(args[_idx[0]]);
    }
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
    return_value = unicode_split_impl(PyValue_AsKnownObjectBorrow(self), sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_partition__doc__,
"partition($self, sep, /)\n"
"--\n"
"\n"
"Partition the string into three parts using the given separator.\n"
"\n"
"This will search for the separator in the string.  If the separator\n"
"is found, returns a 3-tuple containing the part before the\n"
"separator, the separator itself, and the part after it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing\n"
"the original string and two empty strings.");

#define UNICODE_PARTITION_METHODDEF    \
    {"partition", _PyCFunction_CAST(unicode_partition), METH_VALUE, unicode_partition__doc__},

static PyValue
unicode_partition_impl(PyObject *self, PyValueBorrow sep);

static PyValue
unicode_partition(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sep;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "partition", nargs, kwnames, 1)) {
        goto exit;
    }
    sep = PyValue_Borrow(args[0]);
    return_value = unicode_partition_impl(PyValue_AsKnownObjectBorrow(self), sep);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the string into three parts using the given separator.\n"
"\n"
"This will search for the separator in the string, starting at the\n"
"end.  If the separator is found, returns a 3-tuple containing the\n"
"part before the separator, the separator itself, and the part after\n"
"it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing two\n"
"empty strings and the original string.");

#define UNICODE_RPARTITION_METHODDEF    \
    {"rpartition", _PyCFunction_CAST(unicode_rpartition), METH_VALUE, unicode_rpartition__doc__},

static PyValue
unicode_rpartition_impl(PyObject *self, PyValueBorrow sep);

static PyValue
unicode_rpartition(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sep;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "rpartition", nargs, kwnames, 1)) {
        goto exit;
    }
    sep = PyValue_Borrow(args[0]);
    return_value = unicode_rpartition_impl(PyValue_AsKnownObjectBorrow(self), sep);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_rsplit__doc__,
"rsplit($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the substrings in the string, using sep as the separator string.\n"
"\n"
"  sep\n"
"    The separator used to split the string.\n"
"\n"
"    When set to None (the default value), will split on any\n"
"    whitespace character (including \\n \\r \\t \\f and spaces) and\n"
"    will discard empty strings from the result.\n"
"  maxsplit\n"
"    Maximum number of splits.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splitting starts at the end of the string and works to the front.");

#define UNICODE_RSPLIT_METHODDEF    \
    {"rsplit", _PyCFunction_CAST(unicode_rsplit), METH_VALUE, unicode_rsplit__doc__},

static PyValue
unicode_rsplit_impl(PyObject *self, PyValueBorrow sep, Py_ssize_t maxsplit);

static PyValue
unicode_rsplit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxsplit)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "maxsplit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "rsplit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow sep = PyValue_Borrow(PyValue_None);
    Py_ssize_t maxsplit = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        sep = PyValue_Borrow(args[_idx[0]]);
    }
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
    return_value = unicode_rsplit_impl(PyValue_AsKnownObjectBorrow(self), sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the string, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends\n"
"is given and true.");

#define UNICODE_SPLITLINES_METHODDEF    \
    {"splitlines", _PyCFunction_CAST(unicode_splitlines), METH_VALUE, unicode_splitlines__doc__},

static PyValue
unicode_splitlines_impl(PyObject *self, int keepends);

static PyValue
unicode_splitlines(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(keepends)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"keepends", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "splitlines",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int keepends = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        keepends = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (keepends < 0) {
            goto exit;
        }
    }
    return_value = unicode_splitlines_impl(PyValue_AsKnownObjectBorrow(self), keepends);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_swapcase__doc__,
"swapcase($self, /)\n"
"--\n"
"\n"
"Convert uppercase characters to lowercase and lowercase characters to uppercase.");

#define UNICODE_SWAPCASE_METHODDEF    \
    {"swapcase", _PyCFunction_CAST(unicode_swapcase), METH_VALUE, unicode_swapcase__doc__},

static PyValue
unicode_swapcase_impl(PyObject *self);

static PyValue
unicode_swapcase(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "swapcase", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_swapcase_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_maketrans__doc__,
"maketrans(x, y=<unrepresentable>, z=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return a translation table usable for str.translate().\n"
"\n"
"If there is only one argument, it must be a dictionary mapping\n"
"Unicode ordinals (integers) or characters to Unicode ordinals,\n"
"strings or None.  Character keys will be then converted to ordinals.\n"
"If there are two arguments, they must be strings of equal length,\n"
"and in the resulting dictionary, each character in x will be mapped\n"
"to the character at the same position in y.  If there is a third\n"
"argument, it must be a string, whose characters will be mapped to\n"
"None in the result.");

#define UNICODE_MAKETRANS_METHODDEF    \
    {"maketrans", _PyCFunction_CAST(unicode_maketrans), METH_VALUE|METH_STATIC, unicode_maketrans__doc__},

static PyValue
unicode_maketrans_impl(PyValueBorrow x, PyObject *y, PyObject *z);

static PyValue
unicode_maketrans(PyValueBorrow null, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow x;
    PyObject *y = NULL;
    PyObject *z = NULL;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "maketrans", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("maketrans", nargs, 1, 3)) {
        goto exit;
    }
    x = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("maketrans", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("maketrans", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    y = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        _PyArg_BadArgumentValue("maketrans", "argument 3", "str", PyValue_Borrow(args[2]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])))) {
        _PyArg_BadArgument("maketrans", "argument 3", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        goto exit;
    }
    z = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2]));
skip_optional:
    return_value = unicode_maketrans_impl(x, y, z);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_translate__doc__,
"translate($self, table, /)\n"
"--\n"
"\n"
"Replace each character in the string using the given translation table.\n"
"\n"
"  table\n"
"    Translation table, which must be a mapping of Unicode ordinals\n"
"    to Unicode ordinals, strings, or None.\n"
"\n"
"The table must implement lookup/indexing via __getitem__, for\n"
"instance a dictionary or list.  If this operation raises\n"
"LookupError, the character is left untouched.  Characters mapped to\n"
"None are deleted.");

#define UNICODE_TRANSLATE_METHODDEF    \
    {"translate", _PyCFunction_CAST(unicode_translate), METH_VALUE, unicode_translate__doc__},

static PyValue
unicode_translate_impl(PyObject *self, PyValueBorrow table);

static PyValue
unicode_translate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow table;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "translate", nargs, kwnames, 1)) {
        goto exit;
    }
    table = PyValue_Borrow(args[0]);
    return_value = unicode_translate_impl(PyValue_AsKnownObjectBorrow(self), table);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_upper__doc__,
"upper($self, /)\n"
"--\n"
"\n"
"Return a copy of the string converted to uppercase.");

#define UNICODE_UPPER_METHODDEF    \
    {"upper", _PyCFunction_CAST(unicode_upper), METH_VALUE, unicode_upper__doc__},

static PyValue
unicode_upper_impl(PyObject *self);

static PyValue
unicode_upper(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "upper", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_upper_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_zfill__doc__,
"zfill($self, width, /)\n"
"--\n"
"\n"
"Pad a numeric string with zeros on the left, to fill a field of the given width.\n"
"\n"
"The string is never truncated.");

#define UNICODE_ZFILL_METHODDEF    \
    {"zfill", _PyCFunction_CAST(unicode_zfill), METH_VALUE, unicode_zfill__doc__},

static PyValue
unicode_zfill_impl(PyObject *self, Py_ssize_t width);

static PyValue
unicode_zfill(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "zfill", nargs, kwnames, 1)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    return_value = unicode_zfill_impl(PyValue_AsKnownObjectBorrow(self), width);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_startswith__doc__,
"startswith($self, prefix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the string starts with the specified prefix, False otherwise.\n"
"\n"
"  prefix\n"
"    A string or a tuple of strings to try.\n"
"  start\n"
"    Optional start position. Default: start of the string.\n"
"  end\n"
"    Optional stop position. Default: end of the string.");

#define UNICODE_STARTSWITH_METHODDEF    \
    {"startswith", _PyCFunction_CAST(unicode_startswith), METH_VALUE, unicode_startswith__doc__},

static PyValue
unicode_startswith_impl(PyObject *self, PyValueBorrow subobj,
                        Py_ssize_t start, Py_ssize_t end);

static PyValue
unicode_startswith(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "startswith", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("startswith", nargs, 1, 3)) {
        goto exit;
    }
    subobj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_startswith_impl(PyValue_AsKnownObjectBorrow(self), subobj, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_endswith__doc__,
"endswith($self, suffix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the string ends with the specified suffix, False otherwise.\n"
"\n"
"  suffix\n"
"    A string or a tuple of strings to try.\n"
"  start\n"
"    Optional start position. Default: start of the string.\n"
"  end\n"
"    Optional stop position. Default: end of the string.");

#define UNICODE_ENDSWITH_METHODDEF    \
    {"endswith", _PyCFunction_CAST(unicode_endswith), METH_VALUE, unicode_endswith__doc__},

static PyValue
unicode_endswith_impl(PyObject *self, PyValueBorrow subobj, Py_ssize_t start,
                      Py_ssize_t end);

static PyValue
unicode_endswith(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyUnicode_Type, "endswith", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("endswith", nargs, 1, 3)) {
        goto exit;
    }
    subobj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[2]), &end)) {
        goto exit;
    }
skip_optional:
    return_value = unicode_endswith_impl(PyValue_AsKnownObjectBorrow(self), subobj, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Return a formatted version of the string as described by format_spec.");

#define UNICODE___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(unicode___format__), METH_VALUE, unicode___format____doc__},

static PyValue
unicode___format___impl(PyObject *self, PyObject *format_spec);

static PyValue
unicode___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "__format__", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format_spec = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = unicode___format___impl(PyValue_AsKnownObjectBorrow(self), format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the string in memory, in bytes.");

#define UNICODE_SIZEOF_METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(unicode_sizeof), METH_VALUE, unicode_sizeof__doc__},

static PyValue
unicode_sizeof_impl(PyObject *self);

static PyValue
unicode_sizeof(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyUnicode_Type, "__sizeof__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = unicode_sizeof_impl(PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

static PyValue
unicode_new_impl(PyTypeObject *type, PyValueBorrow x, const char *encoding,
                 const char *errors);

static PyValue
unicode_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(object)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"object", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "str",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow x = PyValue_Borrow(PyValue_NULL);
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        x = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("str", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("str", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("str", "argument 'errors'", "str", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("str", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = unicode_new_impl(type, x, encoding, errors);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
unicode_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(unicode_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = unicode_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
/*[clinic end generated code: output=2e37876f5aeb6bc7 input=a9049054013a1b77]*/
