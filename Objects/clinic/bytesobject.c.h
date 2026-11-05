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
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(bytes___bytes____doc__,
"__bytes__($self, /)\n"
"--\n"
"\n"
"Convert this value to exact type bytes.");

#define BYTES___BYTES___METHODDEF    \
    {"__bytes__", _PyCFunction_CAST(bytes___bytes__), METH_VALUE|METH_NOARGS, bytes___bytes____doc__},

static PyValue
bytes___bytes___impl(PyBytesObject *self);

static PyValue
bytes___bytes__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = bytes___bytes___impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(bytes_split__doc__,
"split($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytes, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytes.\n"
"    None (the default value) means split on ASCII whitespace\n"
"    characters (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.");

#define BYTES_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(bytes_split), METH_VALUE|METH_KEYWORDS, bytes_split__doc__},

static PyValue
bytes_split_impl(PyBytesObject *self, PyValueBorrow sep, Py_ssize_t maxsplit);

static PyValue
bytes_split(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = bytes_split_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_partition__doc__,
"partition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytes into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytes.  If the\n"
"separator is found, returns a 3-tuple containing the part before the\n"
"separator, the separator itself, and the part after it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing the\n"
"original bytes object and two empty bytes objects.");

#define BYTES_PARTITION_METHODDEF    \
    {"partition", _PyCFunction_CAST(bytes_partition), METH_VALUE|METH_O, bytes_partition__doc__},

static PyValue
bytes_partition_impl(PyBytesObject *self, Py_buffer *sep);

static PyValue
bytes_partition(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer sep = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &sep, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = bytes_partition_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), &sep);

exit:
    /* Cleanup for sep */
    if (sep.obj) {
       PyBuffer_Release(&sep);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytes into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytes, starting at the\n"
"end.  If the separator is found, returns a 3-tuple containing the\n"
"part before the separator, the separator itself, and the part after\n"
"it.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing two\n"
"empty bytes objects and the original bytes object.");

#define BYTES_RPARTITION_METHODDEF    \
    {"rpartition", _PyCFunction_CAST(bytes_rpartition), METH_VALUE|METH_O, bytes_rpartition__doc__},

static PyValue
bytes_rpartition_impl(PyBytesObject *self, Py_buffer *sep);

static PyValue
bytes_rpartition(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer sep = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &sep, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = bytes_rpartition_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), &sep);

exit:
    /* Cleanup for sep */
    if (sep.obj) {
       PyBuffer_Release(&sep);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_rsplit__doc__,
"rsplit($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytes, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytes.\n"
"    None (the default value) means split on ASCII whitespace\n"
"    characters (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splitting is done starting at the end of the bytes and working to\n"
"the front.");

#define BYTES_RSPLIT_METHODDEF    \
    {"rsplit", _PyCFunction_CAST(bytes_rsplit), METH_VALUE|METH_KEYWORDS, bytes_rsplit__doc__},

static PyValue
bytes_rsplit_impl(PyBytesObject *self, PyValueBorrow sep,
                  Py_ssize_t maxsplit);

static PyValue
bytes_rsplit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = bytes_rsplit_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sep, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_join__doc__,
"join($self, iterable_of_bytes, /)\n"
"--\n"
"\n"
"Concatenate any number of bytes objects.\n"
"\n"
"The bytes whose method is called is inserted in between each pair.\n"
"\n"
"The result is returned as a new bytes object.\n"
"\n"
"Example: b\'.\'.join([b\'ab\', b\'pq\', b\'rs\']) -> b\'ab.pq.rs\'.");

#define BYTES_JOIN_METHODDEF    \
    {"join", _PyCFunction_CAST(bytes_join), METH_VALUE|METH_O, bytes_join__doc__},

static PyValue
bytes_join_impl(PyBytesObject *self, PyValueBorrow iterable_of_bytes);

static PyValue
bytes_join(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable_of_bytes;

    iterable_of_bytes = PyValue_Borrow(args[0]);
    return_value = bytes_join_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), iterable_of_bytes);

    return return_value;
}

PyDoc_STRVAR(bytes_find__doc__,
"find($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTES_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(bytes_find), METH_VALUE, bytes_find__doc__},

static PyValue
bytes_find_impl(PyBytesObject *self, PyValueBorrow sub, Py_ssize_t start,
                Py_ssize_t end);

static PyValue
bytes_find(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Borrow(args[0]);
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
    return_value = bytes_find_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_index__doc__,
"index($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTES_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(bytes_index), METH_VALUE, bytes_index__doc__},

static PyValue
bytes_index_impl(PyBytesObject *self, PyValueBorrow sub, Py_ssize_t start,
                 Py_ssize_t end);

static PyValue
bytes_index(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Borrow(args[0]);
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
    return_value = bytes_index_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_rfind__doc__,
"rfind($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTES_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(bytes_rfind), METH_VALUE, bytes_rfind__doc__},

static PyValue
bytes_rfind_impl(PyBytesObject *self, PyValueBorrow sub, Py_ssize_t start,
                 Py_ssize_t end);

static PyValue
bytes_rfind(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Borrow(args[0]);
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
    return_value = bytes_rfind_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_rindex__doc__,
"rindex($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start,end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTES_RINDEX_METHODDEF    \
    {"rindex", _PyCFunction_CAST(bytes_rindex), METH_VALUE, bytes_rindex__doc__},

static PyValue
bytes_rindex_impl(PyBytesObject *self, PyValueBorrow sub, Py_ssize_t start,
                  Py_ssize_t end);

static PyValue
bytes_rindex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("rindex", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Borrow(args[0]);
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
    return_value = bytes_rindex_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_strip__doc__,
"strip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading and trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading and trailing ASCII\n"
"whitespace.");

#define BYTES_STRIP_METHODDEF    \
    {"strip", _PyCFunction_CAST(bytes_strip), METH_VALUE, bytes_strip__doc__},

static PyValue
bytes_strip_impl(PyBytesObject *self, PyValueBorrow bytes);

static PyValue
bytes_strip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("strip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Borrow(args[0]);
skip_optional:
    return_value = bytes_strip_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_lstrip__doc__,
"lstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading  ASCII whitespace.");

#define BYTES_LSTRIP_METHODDEF    \
    {"lstrip", _PyCFunction_CAST(bytes_lstrip), METH_VALUE, bytes_lstrip__doc__},

static PyValue
bytes_lstrip_impl(PyBytesObject *self, PyValueBorrow bytes);

static PyValue
bytes_lstrip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("lstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Borrow(args[0]);
skip_optional:
    return_value = bytes_lstrip_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_rstrip__doc__,
"rstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip trailing ASCII whitespace.");

#define BYTES_RSTRIP_METHODDEF    \
    {"rstrip", _PyCFunction_CAST(bytes_rstrip), METH_VALUE, bytes_rstrip__doc__},

static PyValue
bytes_rstrip_impl(PyBytesObject *self, PyValueBorrow bytes);

static PyValue
bytes_rstrip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("rstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Borrow(args[0]);
skip_optional:
    return_value = bytes_rstrip_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), bytes);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_count__doc__,
"count($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the number of non-overlapping occurrences of subsection \'sub\' in bytes B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTES_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(bytes_count), METH_VALUE, bytes_count__doc__},

static PyValue
bytes_count_impl(PyBytesObject *self, PyValueBorrow sub, Py_ssize_t start,
                 Py_ssize_t end);

static PyValue
bytes_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("count", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Borrow(args[0]);
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
    return_value = bytes_count_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_translate__doc__,
"translate($self, table, /, delete=b\'\')\n"
"--\n"
"\n"
"Return a copy with each character mapped by the given translation table.\n"
"\n"
"  table\n"
"    Translation table, which must be a bytes object of length 256.\n"
"\n"
"All characters occurring in the optional argument delete are\n"
"removed.  The remaining characters are mapped through the given\n"
"translation table.");

#define BYTES_TRANSLATE_METHODDEF    \
    {"translate", _PyCFunction_CAST(bytes_translate), METH_VALUE|METH_KEYWORDS, bytes_translate__doc__},

static PyValue
bytes_translate_impl(PyBytesObject *self, PyValueBorrow table,
                     PyValueBorrow deletechars);

static PyValue
bytes_translate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(delete)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "delete", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "translate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow table;
    PyValueBorrow deletechars = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    table = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        deletechars = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = bytes_translate_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), table, deletechars);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_maketrans__doc__,
"maketrans(frm, to, /)\n"
"--\n"
"\n"
"Return a translation table usable for the bytes or bytearray translate method.\n"
"\n"
"The returned table will be one where each byte in frm is mapped to\n"
"the byte at the same position in to.\n"
"\n"
"The bytes objects frm and to must be of the same length.");

#define BYTES_MAKETRANS_METHODDEF    \
    {"maketrans", _PyCFunction_CAST(bytes_maketrans), METH_VALUE|METH_STATIC, bytes_maketrans__doc__},

static PyValue
bytes_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyValue
bytes_maketrans(PyValueBorrow null, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer frm = {NULL, NULL};
    Py_buffer to = {NULL, NULL};

    if (!_PyArg_CheckPositional("maketrans", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &frm, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &to, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = bytes_maketrans_impl(&frm, &to);

exit:
    /* Cleanup for frm */
    if (frm.obj) {
       PyBuffer_Release(&frm);
    }
    /* Cleanup for to */
    if (to.obj) {
       PyBuffer_Release(&to);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_replace__doc__,
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

#define BYTES_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(bytes_replace), METH_VALUE|METH_KEYWORDS, bytes_replace__doc__},

static PyValue
bytes_replace_impl(PyBytesObject *self, Py_buffer *old, Py_buffer *new,
                   Py_ssize_t count);

static PyValue
bytes_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_buffer old = {NULL, NULL};
    Py_buffer new = {NULL, NULL};
    Py_ssize_t count = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &old, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &new, PyBUF_SIMPLE) != 0) {
        goto exit;
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
            count = ival;
        }
    }
    return_value = bytes_replace_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), &old, &new, count);

exit:
    /* Cleanup for old */
    if (old.obj) {
       PyBuffer_Release(&old);
    }
    /* Cleanup for new */
    if (new.obj) {
       PyBuffer_Release(&new);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_removeprefix__doc__,
"removeprefix($self, prefix, /)\n"
"--\n"
"\n"
"Return a bytes object with the given prefix string removed if present.\n"
"\n"
"If the bytes starts with the prefix string, return\n"
"bytes[len(prefix):].  Otherwise, return a copy of the original\n"
"bytes.");

#define BYTES_REMOVEPREFIX_METHODDEF    \
    {"removeprefix", _PyCFunction_CAST(bytes_removeprefix), METH_VALUE|METH_O, bytes_removeprefix__doc__},

static PyValue
bytes_removeprefix_impl(PyBytesObject *self, Py_buffer *prefix);

static PyValue
bytes_removeprefix(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer prefix = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &prefix, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = bytes_removeprefix_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), &prefix);

exit:
    /* Cleanup for prefix */
    if (prefix.obj) {
       PyBuffer_Release(&prefix);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_removesuffix__doc__,
"removesuffix($self, suffix, /)\n"
"--\n"
"\n"
"Return a bytes object with the given suffix string removed if present.\n"
"\n"
"If the bytes ends with the suffix string and that suffix is not\n"
"empty, return bytes[:-len(prefix)].  Otherwise, return a copy of the\n"
"original bytes.");

#define BYTES_REMOVESUFFIX_METHODDEF    \
    {"removesuffix", _PyCFunction_CAST(bytes_removesuffix), METH_VALUE|METH_O, bytes_removesuffix__doc__},

static PyValue
bytes_removesuffix_impl(PyBytesObject *self, Py_buffer *suffix);

static PyValue
bytes_removesuffix(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer suffix = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &suffix, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = bytes_removesuffix_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), &suffix);

exit:
    /* Cleanup for suffix */
    if (suffix.obj) {
       PyBuffer_Release(&suffix);
    }

    return return_value;
}

PyDoc_STRVAR(bytes_startswith__doc__,
"startswith($self, prefix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytes starts with the specified prefix, False otherwise.\n"
"\n"
"  prefix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTES_STARTSWITH_METHODDEF    \
    {"startswith", _PyCFunction_CAST(bytes_startswith), METH_VALUE, bytes_startswith__doc__},

static PyValue
bytes_startswith_impl(PyBytesObject *self, PyValueBorrow subobj,
                      Py_ssize_t start, Py_ssize_t end);

static PyValue
bytes_startswith(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

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
    return_value = bytes_startswith_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), subobj, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_endswith__doc__,
"endswith($self, suffix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytes ends with the specified suffix, False otherwise.\n"
"\n"
"  suffix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTES_ENDSWITH_METHODDEF    \
    {"endswith", _PyCFunction_CAST(bytes_endswith), METH_VALUE, bytes_endswith__doc__},

static PyValue
bytes_endswith_impl(PyBytesObject *self, PyValueBorrow subobj,
                    Py_ssize_t start, Py_ssize_t end);

static PyValue
bytes_endswith(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

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
    return_value = bytes_endswith_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), subobj, start, end);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_decode__doc__,
"decode($self, /, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decode the bytes using the codec registered for encoding.\n"
"\n"
"  encoding\n"
"    The encoding with which to decode the bytes.\n"
"  errors\n"
"    The error handling scheme to use for the handling of decoding\n"
"    errors.  The default is \'strict\' meaning that decoding errors\n"
"    raise a UnicodeDecodeError.  Other possible values are \'ignore\'\n"
"    and \'replace\' as well as any other name registered with\n"
"    codecs.register_error that can handle UnicodeDecodeErrors.");

#define BYTES_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(bytes_decode), METH_VALUE|METH_KEYWORDS, bytes_decode__doc__},

static PyValue
bytes_decode_impl(PyBytesObject *self, const char *encoding,
                  const char *errors);

static PyValue
bytes_decode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "decode",
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
            _PyArg_BadArgumentValue("decode", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("decode", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("decode", "argument 'errors'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("decode", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = bytes_decode_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the bytes, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends\n"
"is given and true.");

#define BYTES_SPLITLINES_METHODDEF    \
    {"splitlines", _PyCFunction_CAST(bytes_splitlines), METH_VALUE|METH_KEYWORDS, bytes_splitlines__doc__},

static PyValue
bytes_splitlines_impl(PyBytesObject *self, int keepends);

static PyValue
bytes_splitlines(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = bytes_splitlines_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), keepends);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_fromhex__doc__,
"fromhex($type, string, /)\n"
"--\n"
"\n"
"Create a bytes object from a string of hexadecimal numbers.\n"
"\n"
"Spaces between two numbers are accepted.\n"
"Example: bytes.fromhex(\'B9 01EF\') -> b\'\\\\xb9\\\\x01\\\\xef\'.");

#define BYTES_FROMHEX_METHODDEF    \
    {"fromhex", _PyCFunction_CAST(bytes_fromhex), METH_VALUE|METH_O|METH_CLASS, bytes_fromhex__doc__},

static PyValue
bytes_fromhex_impl(PyTypeObject *type, PyValueBorrow string);

static PyValue
bytes_fromhex(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    return_value = bytes_fromhex_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string);

    return return_value;
}

PyDoc_STRVAR(bytes_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Create a string of hexadecimal numbers from a bytes object.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from\n"
"    the right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = b\'\\xb9\\x01\\xef\'\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define BYTES_HEX_METHODDEF    \
    {"hex", _PyCFunction_CAST(bytes_hex), METH_VALUE|METH_KEYWORDS, bytes_hex__doc__},

static PyValue
bytes_hex_impl(PyBytesObject *self, PyValueBorrow sep,
               Py_ssize_t bytes_per_sep);

static PyValue
bytes_hex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bytes_per_sep)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "hex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow sep = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t bytes_per_sep = 1;

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
            bytes_per_sep = ival;
        }
    }
    return_value = bytes_hex_impl((PyBytesObject *)PyValue_AsKnownObjectBorrow(self), sep, bytes_per_sep);

exit:
    return return_value;
}

static PyValue
bytes_new_impl(PyTypeObject *type, PyValueBorrow x, const char *encoding,
               const char *errors);

static PyValue
bytes_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(source)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"source", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bytes",
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
            _PyArg_BadArgumentValue("bytes", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("bytes", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("bytes", "argument 'errors'", "str", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("bytes", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = bytes_new_impl(type, x, encoding, errors);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
bytes_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(bytes_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = bytes_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
bytes_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == bytes_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    return bytes_new_value(_PyType_CAST(type), args, nargs, kwnames);
}
/*[clinic end generated code: output=254aea0e3c75e565 input=a9049054013a1b77]*/
