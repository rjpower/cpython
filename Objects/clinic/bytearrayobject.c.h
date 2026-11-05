/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_runtime.h"       // _Py_SINGLETON()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

static PyObject *
bytearray_iconcat_impl(PyByteArrayObject *self, PyObject *other);

static PyObject *
bytearray_iconcat(PyObject *self, PyObject *other)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_iconcat_impl((PyByteArrayObject *)self, other);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
bytearray_repeat_impl(PyByteArrayObject *self, Py_ssize_t count);

static PyObject *
bytearray_repeat(PyObject *self, Py_ssize_t count)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_repeat_impl((PyByteArrayObject *)self, count);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
bytearray_irepeat_impl(PyByteArrayObject *self, Py_ssize_t count);

static PyObject *
bytearray_irepeat(PyObject *self, Py_ssize_t count)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_irepeat_impl((PyByteArrayObject *)self, count);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
bytearray_getitem_impl(PyByteArrayObject *self, Py_ssize_t i);

static PyObject *
bytearray_getitem(PyObject *self, Py_ssize_t i)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_getitem_impl((PyByteArrayObject *)self, i);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
bytearray_setitem_impl(PyByteArrayObject *self, Py_ssize_t i,
                       PyObject *value);

static int
bytearray_setitem(PyObject *self, Py_ssize_t i, PyObject *value)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_setitem_impl((PyByteArrayObject *)self, i, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
bytearray___init___impl(PyByteArrayObject *self, PyValueBorrow arg,
                        const char *encoding, const char *errors);

static int
bytearray___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .fname = "bytearray",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow arg = PyValue_Borrow(PyValue_NULL);
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        arg = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("bytearray", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("bytearray", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("bytearray", "argument 'errors'", "str", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("bytearray", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = bytearray___init___impl((PyByteArrayObject *)self, arg, encoding, errors);

exit:
    return return_value;
}

static int
bytearray___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return bytearray___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = bytearray___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

static PyValue
bytearray_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_init == bytearray___init__);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        return PyValue_NULL;
    }
    if (bytearray___init___value(self, args, nargs, kwnames) != 0) {
        Py_DECREF(self);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(self);
}

static PyObject *
bytearray_repr_impl(PyObject *op);

static PyObject *
bytearray_repr(PyObject *op)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(op);
    return_value = bytearray_repr_impl(op);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_find__doc__,
"find($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTEARRAY_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(bytearray_find), METH_VALUE, bytearray_find__doc__},

static PyValue
bytearray_find_impl(PyByteArrayObject *self, PyValueBorrow sub,
                    Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_find(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "find", kwnames)) {
        goto exit;
    }
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_find_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_count__doc__,
"count($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the number of non-overlapping occurrences of subsection \'sub\' in bytes B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.");

#define BYTEARRAY_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(bytearray_count), METH_VALUE, bytearray_count__doc__},

static PyValue
bytearray_count_impl(PyByteArrayObject *self, PyValueBorrow sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "count", kwnames)) {
        goto exit;
    }
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_count_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the bytearray.");

#define BYTEARRAY_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(bytearray_clear), METH_VALUE|METH_NOARGS, bytearray_clear__doc__},

static PyValue
bytearray_clear_impl(PyByteArrayObject *self);

static PyValue
bytearray_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = bytearray_clear_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(bytearray_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of B.");

#define BYTEARRAY_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(bytearray_copy), METH_VALUE|METH_NOARGS, bytearray_copy__doc__},

static PyValue
bytearray_copy_impl(PyByteArrayObject *self);

static PyValue
bytearray_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_copy_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_index__doc__,
"index($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the lowest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTEARRAY_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(bytearray_index), METH_VALUE, bytearray_index__doc__},

static PyValue
bytearray_index_impl(PyByteArrayObject *self, PyValueBorrow sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_index(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "index", kwnames)) {
        goto exit;
    }
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_index_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_rfind__doc__,
"rfind($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Return -1 on failure.");

#define BYTEARRAY_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(bytearray_rfind), METH_VALUE, bytearray_rfind__doc__},

static PyValue
bytearray_rfind_impl(PyByteArrayObject *self, PyValueBorrow sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_rfind(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "rfind", kwnames)) {
        goto exit;
    }
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_rfind_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_rindex__doc__,
"rindex($self, sub[, start[, end]], /)\n"
"--\n"
"\n"
"Return the highest index in B where subsection \'sub\' is found, such that \'sub\' is contained within B[start:end].\n"
"\n"
"  start\n"
"    Optional start position. Default: start of the bytes.\n"
"  end\n"
"    Optional stop position. Default: end of the bytes.\n"
"\n"
"Raise ValueError if the subsection is not found.");

#define BYTEARRAY_RINDEX_METHODDEF    \
    {"rindex", _PyCFunction_CAST(bytearray_rindex), METH_VALUE, bytearray_rindex__doc__},

static PyValue
bytearray_rindex_impl(PyByteArrayObject *self, PyValueBorrow sub,
                      Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_rindex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "rindex", kwnames)) {
        goto exit;
    }
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_rindex_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sub, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_startswith__doc__,
"startswith($self, prefix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytearray starts with the specified prefix, False otherwise.\n"
"\n"
"  prefix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytearray.\n"
"  end\n"
"    Optional stop position. Default: end of the bytearray.");

#define BYTEARRAY_STARTSWITH_METHODDEF    \
    {"startswith", _PyCFunction_CAST(bytearray_startswith), METH_VALUE, bytearray_startswith__doc__},

static PyValue
bytearray_startswith_impl(PyByteArrayObject *self, PyValueBorrow subobj,
                          Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_startswith(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "startswith", kwnames)) {
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_startswith_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), subobj, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_endswith__doc__,
"endswith($self, suffix[, start[, end]], /)\n"
"--\n"
"\n"
"Return True if the bytearray ends with the specified suffix, False otherwise.\n"
"\n"
"  suffix\n"
"    A bytes or a tuple of bytes to try.\n"
"  start\n"
"    Optional start position. Default: start of the bytearray.\n"
"  end\n"
"    Optional stop position. Default: end of the bytearray.");

#define BYTEARRAY_ENDSWITH_METHODDEF    \
    {"endswith", _PyCFunction_CAST(bytearray_endswith), METH_VALUE, bytearray_endswith__doc__},

static PyValue
bytearray_endswith_impl(PyByteArrayObject *self, PyValueBorrow subobj,
                        Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_endswith(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "endswith", kwnames)) {
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_endswith_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), subobj, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_removeprefix__doc__,
"removeprefix($self, prefix, /)\n"
"--\n"
"\n"
"Return a bytearray with the given prefix string removed if present.\n"
"\n"
"If the bytearray starts with the prefix string, return\n"
"bytearray[len(prefix):].  Otherwise, return a copy of the original\n"
"bytearray.");

#define BYTEARRAY_REMOVEPREFIX_METHODDEF    \
    {"removeprefix", _PyCFunction_CAST(bytearray_removeprefix), METH_VALUE|METH_O, bytearray_removeprefix__doc__},

static PyValue
bytearray_removeprefix_impl(PyByteArrayObject *self, Py_buffer *prefix);

static PyValue
bytearray_removeprefix(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_removeprefix_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), &prefix);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for prefix */
    if (prefix.obj) {
       PyBuffer_Release(&prefix);
    }

    return return_value;
}

PyDoc_STRVAR(bytearray_removesuffix__doc__,
"removesuffix($self, suffix, /)\n"
"--\n"
"\n"
"Return a bytearray with the given suffix string removed if present.\n"
"\n"
"If the bytearray ends with the suffix string and that suffix is not\n"
"empty, return bytearray[:-len(suffix)].  Otherwise, return a copy of\n"
"the original bytearray.");

#define BYTEARRAY_REMOVESUFFIX_METHODDEF    \
    {"removesuffix", _PyCFunction_CAST(bytearray_removesuffix), METH_VALUE|METH_O, bytearray_removesuffix__doc__},

static PyValue
bytearray_removesuffix_impl(PyByteArrayObject *self, Py_buffer *suffix);

static PyValue
bytearray_removesuffix(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_removesuffix_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), &suffix);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for suffix */
    if (suffix.obj) {
       PyBuffer_Release(&suffix);
    }

    return return_value;
}

PyDoc_STRVAR(bytearray_resize__doc__,
"resize($self, size, /)\n"
"--\n"
"\n"
"Resize the internal buffer of bytearray to len.\n"
"\n"
"  size\n"
"    New size to resize to.");

#define BYTEARRAY_RESIZE_METHODDEF    \
    {"resize", _PyCFunction_CAST(bytearray_resize), METH_VALUE|METH_O, bytearray_resize__doc__},

static PyValue
bytearray_resize_impl(PyByteArrayObject *self, Py_ssize_t size);

static PyValue
bytearray_resize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size;

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
        size = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_resize_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_take_bytes__doc__,
"take_bytes($self, n=None, /)\n"
"--\n"
"\n"
"Take *n* bytes from the bytearray and return them as a bytes object.\n"
"\n"
"  n\n"
"    Bytes to take, negative indexes from end. None indicates all bytes.");

#define BYTEARRAY_TAKE_BYTES_METHODDEF    \
    {"take_bytes", _PyCFunction_CAST(bytearray_take_bytes), METH_VALUE, bytearray_take_bytes__doc__},

static PyValue
bytearray_take_bytes_impl(PyByteArrayObject *self, PyValueBorrow n);

static PyValue
bytearray_take_bytes(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow n = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "take_bytes", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("take_bytes", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    n = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_take_bytes_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_translate__doc__,
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

#define BYTEARRAY_TRANSLATE_METHODDEF    \
    {"translate", _PyCFunction_CAST(bytearray_translate), METH_VALUE, bytearray_translate__doc__},

static PyValue
bytearray_translate_impl(PyByteArrayObject *self, PyValueBorrow table,
                         PyValueBorrow deletechars);

static PyValue
bytearray_translate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_translate_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), table, deletechars);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_maketrans__doc__,
"maketrans(frm, to, /)\n"
"--\n"
"\n"
"Return a translation table usable for the bytes or bytearray translate method.\n"
"\n"
"The returned table will be one where each byte in frm is mapped to\n"
"the byte at the same position in to.\n"
"\n"
"The bytes objects frm and to must be of the same length.");

#define BYTEARRAY_MAKETRANS_METHODDEF    \
    {"maketrans", _PyCFunction_CAST(bytearray_maketrans), METH_VALUE|METH_STATIC, bytearray_maketrans__doc__},

static PyValue
bytearray_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyValue
bytearray_maketrans(PyValueBorrow null, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer frm = {NULL, NULL};
    Py_buffer to = {NULL, NULL};

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "maketrans", kwnames)) {
        goto exit;
    }
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
    return_value = bytearray_maketrans_impl(&frm, &to);

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

PyDoc_STRVAR(bytearray_replace__doc__,
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

#define BYTEARRAY_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(bytearray_replace), METH_VALUE, bytearray_replace__doc__},

static PyValue
bytearray_replace_impl(PyByteArrayObject *self, Py_buffer *old,
                       Py_buffer *new, Py_ssize_t count);

static PyValue
bytearray_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_replace_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), &old, &new, count);
    Py_END_CRITICAL_SECTION();

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

PyDoc_STRVAR(bytearray_split__doc__,
"split($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytearray, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytearray.\n"
"    None (the default value) means split on ASCII whitespace\n"
"    characters (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.");

#define BYTEARRAY_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(bytearray_split), METH_VALUE, bytearray_split__doc__},

static PyValue
bytearray_split_impl(PyByteArrayObject *self, PyValueBorrow sep,
                     Py_ssize_t maxsplit);

static PyValue
bytearray_split(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_split_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sep, maxsplit);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_partition__doc__,
"partition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytearray into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytearray.  If the\n"
"separator is found, returns a 3-tuple containing the part before the\n"
"separator, the separator itself, and the part after it as new\n"
"bytearray objects.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing the copy\n"
"of the original bytearray object and two empty bytearray objects.");

#define BYTEARRAY_PARTITION_METHODDEF    \
    {"partition", _PyCFunction_CAST(bytearray_partition), METH_VALUE|METH_O, bytearray_partition__doc__},

static PyValue
bytearray_partition_impl(PyByteArrayObject *self, PyValueBorrow sep);

static PyValue
bytearray_partition(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sep;

    sep = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_partition_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sep);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_rpartition__doc__,
"rpartition($self, sep, /)\n"
"--\n"
"\n"
"Partition the bytearray into three parts using the given separator.\n"
"\n"
"This will search for the separator sep in the bytearray, starting at\n"
"the end.  If the separator is found, returns a 3-tuple containing\n"
"the part before the separator, the separator itself, and the part\n"
"after it as new bytearray objects.\n"
"\n"
"If the separator is not found, returns a 3-tuple containing two\n"
"empty bytearray objects and the copy of the original bytearray\n"
"object.");

#define BYTEARRAY_RPARTITION_METHODDEF    \
    {"rpartition", _PyCFunction_CAST(bytearray_rpartition), METH_VALUE|METH_O, bytearray_rpartition__doc__},

static PyValue
bytearray_rpartition_impl(PyByteArrayObject *self, PyValueBorrow sep);

static PyValue
bytearray_rpartition(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sep;

    sep = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_rpartition_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sep);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_rsplit__doc__,
"rsplit($self, /, sep=None, maxsplit=-1)\n"
"--\n"
"\n"
"Return a list of the sections in the bytearray, using sep as the delimiter.\n"
"\n"
"  sep\n"
"    The delimiter according which to split the bytearray.\n"
"    None (the default value) means split on ASCII whitespace\n"
"    characters (space, tab, return, newline, formfeed, vertical tab).\n"
"  maxsplit\n"
"    Maximum number of splits to do.\n"
"    -1 (the default value) means no limit.\n"
"\n"
"Splitting is done starting at the end of the bytearray and working\n"
"to the front.");

#define BYTEARRAY_RSPLIT_METHODDEF    \
    {"rsplit", _PyCFunction_CAST(bytearray_rsplit), METH_VALUE, bytearray_rsplit__doc__},

static PyValue
bytearray_rsplit_impl(PyByteArrayObject *self, PyValueBorrow sep,
                      Py_ssize_t maxsplit);

static PyValue
bytearray_rsplit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_rsplit_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sep, maxsplit);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse the order of the values in B in place.");

#define BYTEARRAY_REVERSE_METHODDEF    \
    {"reverse", _PyCFunction_CAST(bytearray_reverse), METH_VALUE|METH_NOARGS, bytearray_reverse__doc__},

static PyValue
bytearray_reverse_impl(PyByteArrayObject *self);

static PyValue
bytearray_reverse(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_reverse_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_insert__doc__,
"insert($self, index, item, /)\n"
"--\n"
"\n"
"Insert a single item into the bytearray before the given index.\n"
"\n"
"  index\n"
"    The index where the value is to be inserted.\n"
"  item\n"
"    The item to be inserted.");

#define BYTEARRAY_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(bytearray_insert), METH_VALUE, bytearray_insert__doc__},

static PyValue
bytearray_insert_impl(PyByteArrayObject *self, Py_ssize_t index, int item);

static PyValue
bytearray_insert(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    int item;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "insert", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
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
        index = ival;
    }
    if (!_getbytevalue(PyValue_Borrow(args[1]), &item)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_insert_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), index, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_append__doc__,
"append($self, item, /)\n"
"--\n"
"\n"
"Append a single item to the end of the bytearray.\n"
"\n"
"  item\n"
"    The item to be appended.");

#define BYTEARRAY_APPEND_METHODDEF    \
    {"append", _PyCFunction_CAST(bytearray_append), METH_VALUE|METH_O, bytearray_append__doc__},

static PyValue
bytearray_append_impl(PyByteArrayObject *self, int item);

static PyValue
bytearray_append(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int item;

    if (!_getbytevalue(PyValue_Borrow(args[0]), &item)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_append_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_extend__doc__,
"extend($self, iterable_of_ints, /)\n"
"--\n"
"\n"
"Append all the items from the iterator or sequence to the end of the bytearray.\n"
"\n"
"  iterable_of_ints\n"
"    The iterable of items to append.");

#define BYTEARRAY_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(bytearray_extend), METH_VALUE|METH_O, bytearray_extend__doc__},

static PyValue
bytearray_extend_impl(PyByteArrayObject *self,
                      PyValueBorrow iterable_of_ints);

static PyValue
bytearray_extend(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable_of_ints;

    iterable_of_ints = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_extend_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), iterable_of_ints);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_pop__doc__,
"pop($self, index=-1, /)\n"
"--\n"
"\n"
"Remove and return a single item from B.\n"
"\n"
"  index\n"
"    The index from where to remove the item.\n"
"    -1 (the default value) means remove the last item.\n"
"\n"
"If no index argument is given, will pop the last item.");

#define BYTEARRAY_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(bytearray_pop), METH_VALUE, bytearray_pop__doc__},

static PyValue
bytearray_pop_impl(PyByteArrayObject *self, Py_ssize_t index);

static PyValue
bytearray_pop(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index = -1;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "pop", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
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
        index = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_pop_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), index);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_remove__doc__,
"remove($self, value, /)\n"
"--\n"
"\n"
"Remove the first occurrence of a value in the bytearray.\n"
"\n"
"  value\n"
"    The value to remove.");

#define BYTEARRAY_REMOVE_METHODDEF    \
    {"remove", _PyCFunction_CAST(bytearray_remove), METH_VALUE|METH_O, bytearray_remove__doc__},

static PyValue
bytearray_remove_impl(PyByteArrayObject *self, int value);

static PyValue
bytearray_remove(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int value;

    if (!_getbytevalue(PyValue_Borrow(args[0]), &value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_remove_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_strip__doc__,
"strip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading and trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading and trailing ASCII\n"
"whitespace.");

#define BYTEARRAY_STRIP_METHODDEF    \
    {"strip", _PyCFunction_CAST(bytearray_strip), METH_VALUE, bytearray_strip__doc__},

static PyValue
bytearray_strip_impl(PyByteArrayObject *self, PyValueBorrow bytes);

static PyValue
bytearray_strip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "strip", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("strip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_strip_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), bytes);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_lstrip__doc__,
"lstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip leading bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip leading ASCII whitespace.");

#define BYTEARRAY_LSTRIP_METHODDEF    \
    {"lstrip", _PyCFunction_CAST(bytearray_lstrip), METH_VALUE, bytearray_lstrip__doc__},

static PyValue
bytearray_lstrip_impl(PyByteArrayObject *self, PyValueBorrow bytes);

static PyValue
bytearray_lstrip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "lstrip", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("lstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_lstrip_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), bytes);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_rstrip__doc__,
"rstrip($self, bytes=None, /)\n"
"--\n"
"\n"
"Strip trailing bytes contained in the argument.\n"
"\n"
"If the argument is omitted or None, strip trailing ASCII whitespace.");

#define BYTEARRAY_RSTRIP_METHODDEF    \
    {"rstrip", _PyCFunction_CAST(bytearray_rstrip), METH_VALUE, bytearray_rstrip__doc__},

static PyValue
bytearray_rstrip_impl(PyByteArrayObject *self, PyValueBorrow bytes);

static PyValue
bytearray_rstrip(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "rstrip", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("rstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_rstrip_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), bytes);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_decode__doc__,
"decode($self, /, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decode the bytearray using the codec registered for encoding.\n"
"\n"
"  encoding\n"
"    The encoding with which to decode the bytearray.\n"
"  errors\n"
"    The error handling scheme to use for the handling of decoding\n"
"    errors.  The default is \'strict\' meaning that decoding errors\n"
"    raise a UnicodeDecodeError.  Other possible values are \'ignore\'\n"
"    and \'replace\' as well as any other name registered with\n"
"    codecs.register_error that can handle UnicodeDecodeErrors.");

#define BYTEARRAY_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(bytearray_decode), METH_VALUE, bytearray_decode__doc__},

static PyValue
bytearray_decode_impl(PyByteArrayObject *self, const char *encoding,
                      const char *errors);

static PyValue
bytearray_decode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_decode_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), encoding, errors);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_join__doc__,
"join($self, iterable_of_bytes, /)\n"
"--\n"
"\n"
"Concatenate any number of bytes/bytearray objects.\n"
"\n"
"The bytearray whose method is called is inserted in between each\n"
"pair.\n"
"\n"
"The result is returned as a new bytearray object.");

#define BYTEARRAY_JOIN_METHODDEF    \
    {"join", _PyCFunction_CAST(bytearray_join), METH_VALUE|METH_O, bytearray_join__doc__},

static PyValue
bytearray_join_impl(PyByteArrayObject *self, PyValueBorrow iterable_of_bytes);

static PyValue
bytearray_join(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable_of_bytes;

    iterable_of_bytes = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_join_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), iterable_of_bytes);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_splitlines__doc__,
"splitlines($self, /, keepends=False)\n"
"--\n"
"\n"
"Return a list of the lines in the bytearray, breaking at line boundaries.\n"
"\n"
"Line breaks are not included in the resulting list unless keepends\n"
"is given and true.");

#define BYTEARRAY_SPLITLINES_METHODDEF    \
    {"splitlines", _PyCFunction_CAST(bytearray_splitlines), METH_VALUE, bytearray_splitlines__doc__},

static PyValue
bytearray_splitlines_impl(PyByteArrayObject *self, int keepends);

static PyValue
bytearray_splitlines(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_splitlines_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), keepends);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_fromhex__doc__,
"fromhex($type, string, /)\n"
"--\n"
"\n"
"Create a bytearray object from a string of hexadecimal numbers.\n"
"\n"
"Spaces between two numbers are accepted.\n"
"Example:\n"
"    bytearray.fromhex(\'B9 01EF\') -> bytearray(b\'\\\\xb9\\\\x01\\\\xef\')");

#define BYTEARRAY_FROMHEX_METHODDEF    \
    {"fromhex", _PyCFunction_CAST(bytearray_fromhex), METH_VALUE|METH_O|METH_CLASS, bytearray_fromhex__doc__},

static PyValue
bytearray_fromhex_impl(PyTypeObject *type, PyValueBorrow string);

static PyValue
bytearray_fromhex(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    return_value = bytearray_fromhex_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string);

    return return_value;
}

PyDoc_STRVAR(bytearray_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Create a string of hexadecimal numbers from a bytearray object.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from\n"
"    the right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = bytearray([0xb9, 0x01, 0xef])\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define BYTEARRAY_HEX_METHODDEF    \
    {"hex", _PyCFunction_CAST(bytearray_hex), METH_VALUE, bytearray_hex__doc__},

static PyValue
bytearray_hex_impl(PyByteArrayObject *self, PyValueBorrow sep,
                   Py_ssize_t bytes_per_sep);

static PyValue
bytearray_hex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_hex_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), sep, bytes_per_sep);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(bytearray_reduce), METH_VALUE|METH_NOARGS, bytearray_reduce__doc__},

static PyValue
bytearray_reduce_impl(PyByteArrayObject *self);

static PyValue
bytearray_reduce(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_reduce_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_reduce_ex__doc__,
"__reduce_ex__($self, proto=0, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_EX_METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(bytearray_reduce_ex), METH_VALUE, bytearray_reduce_ex__doc__},

static PyValue
bytearray_reduce_ex_impl(PyByteArrayObject *self, int proto);

static PyValue
bytearray_reduce_ex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int proto = 0;

    if (!_PyArg_NoKwnamesValueMethod(&PyByteArray_Type, "__reduce_ex__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__reduce_ex__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    proto = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = bytearray_reduce_ex_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self), proto);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns the size of the bytearray object in memory, in bytes.");

#define BYTEARRAY_SIZEOF_METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(bytearray_sizeof), METH_VALUE|METH_NOARGS, bytearray_sizeof__doc__},

static PyValue
bytearray_sizeof_impl(PyByteArrayObject *self);

static PyValue
bytearray_sizeof(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = bytearray_sizeof_impl((PyByteArrayObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=e276b7012398ddd2 input=a9049054013a1b77]*/
