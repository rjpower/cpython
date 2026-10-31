/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_runtime.h"       // _Py_SINGLETON()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

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
bytearray___init___impl(PyByteArrayObject *self, PyObject *arg,
                        const char *encoding, const char *errors);

static int
bytearray___init___helper(PyObject *self, const PyValue *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(source), &_Py_ID(encoding), &_Py_ID(errors), },
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
    PyValue argsbuf[3];
    const PyValue *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 0;
    PyObject *arg = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        arg = PyValue_Object(fastargs[0]);
        if (arg == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[1])) {
        {
            PyObject *_arg = PyValue_Object(fastargs[1]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("bytearray", "argument 'encoding'", "str", _arg);
                goto exit;
            }
            encoding = _PyUnicode_AsUTF8NoNUL(_arg);
            if (encoding == NULL) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        PyObject *_arg = PyValue_Object(fastargs[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("bytearray", "argument 'errors'", "str", _arg);
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(_arg);
        if (errors == NULL) {
            goto exit;
        }
    }
skip_optional_pos:
    return_value = bytearray___init___impl((PyByteArrayObject *)self, arg, encoding, errors);

exit:
    return return_value;
}

static int
bytearray___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return bytearray___init___helper(self, _PyTuple_ITEMS(args),
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyValue
bytearray_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self;
    int _result;
    PyObject *arg = NULL;
    const char *encoding = NULL;
    const char *errors = NULL;

    assert(_PyType_CAST(type)->tp_init == bytearray___init__);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs > 3) {
        self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
            (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
        if (self == NULL) {
            return PyValue_NULL;
        }
        _result = bytearray___init___helper(self, args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
        if (_result != 0) {
            Py_DECREF(self);
            return PyValue_NULL;
        }
        return PyValue_FromObject(self);
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    arg = PyValue_Object(args[0]);
    if (arg == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("bytearray", "argument 'encoding'", "str", _arg);
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(_arg);
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("bytearray", "argument 'errors'", "str", _arg);
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(_arg);
        if (errors == NULL) {
            goto exit;
        }
    }
skip_optional:
    self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        goto exit;
    }
    _result = bytearray___init___impl((PyByteArrayObject *)self, arg, encoding, errors);
    if (_result != 0) {
        Py_DECREF(self);
        goto exit;
    }
    return_value = self;

exit:
    return PyValue_FromResult(return_value);
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
    {"find", _PyCFunction_CAST(bytearray_find), METH_FASTCALL, bytearray_find__doc__},

static PyObject *
bytearray_find_impl(PyByteArrayObject *self, PyObject *sub, Py_ssize_t start,
                    Py_ssize_t end);

static PyValue
bytearray_find(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Object(args[0]);
    if (sub == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_find_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"count", _PyCFunction_CAST(bytearray_count), METH_FASTCALL, bytearray_count__doc__},

static PyObject *
bytearray_count_impl(PyByteArrayObject *self, PyObject *sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_count(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("count", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Object(args[0]);
    if (sub == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_count_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the bytearray.");

#define BYTEARRAY_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)bytearray_clear, METH_NOARGS, bytearray_clear__doc__},

static PyObject *
bytearray_clear_impl(PyByteArrayObject *self);

static PyObject *
bytearray_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_clear_impl((PyByteArrayObject *)self);
}

PyDoc_STRVAR(bytearray_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of B.");

#define BYTEARRAY_COPY_METHODDEF    \
    {"copy", (PyCFunction)bytearray_copy, METH_NOARGS, bytearray_copy__doc__},

static PyObject *
bytearray_copy_impl(PyByteArrayObject *self);

static PyObject *
bytearray_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_copy_impl((PyByteArrayObject *)self);
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
    {"index", _PyCFunction_CAST(bytearray_index), METH_FASTCALL, bytearray_index__doc__},

static PyObject *
bytearray_index_impl(PyByteArrayObject *self, PyObject *sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_index(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Object(args[0]);
    if (sub == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_index_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"rfind", _PyCFunction_CAST(bytearray_rfind), METH_FASTCALL, bytearray_rfind__doc__},

static PyObject *
bytearray_rfind_impl(PyByteArrayObject *self, PyObject *sub,
                     Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_rfind(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Object(args[0]);
    if (sub == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_rfind_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"rindex", _PyCFunction_CAST(bytearray_rindex), METH_FASTCALL, bytearray_rindex__doc__},

static PyObject *
bytearray_rindex_impl(PyByteArrayObject *self, PyObject *sub,
                      Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_rindex(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sub;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("rindex", nargs, 1, 3)) {
        goto exit;
    }
    sub = PyValue_Object(args[0]);
    if (sub == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_rindex_impl((PyByteArrayObject *)self, sub, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"startswith", _PyCFunction_CAST(bytearray_startswith), METH_FASTCALL, bytearray_startswith__doc__},

static PyObject *
bytearray_startswith_impl(PyByteArrayObject *self, PyObject *subobj,
                          Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_startswith(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("startswith", nargs, 1, 3)) {
        goto exit;
    }
    subobj = PyValue_Object(args[0]);
    if (subobj == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_startswith_impl((PyByteArrayObject *)self, subobj, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"endswith", _PyCFunction_CAST(bytearray_endswith), METH_FASTCALL, bytearray_endswith__doc__},

static PyObject *
bytearray_endswith_impl(PyByteArrayObject *self, PyObject *subobj,
                        Py_ssize_t start, Py_ssize_t end);

static PyValue
bytearray_endswith(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *subobj;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("endswith", nargs, 1, 3)) {
        goto exit;
    }
    subobj = PyValue_Object(args[0]);
    if (subobj == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &end)) {
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_endswith_impl((PyByteArrayObject *)self, subobj, start, end);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"removeprefix", _PyCFunction_CAST(bytearray_removeprefix), METH_O, bytearray_removeprefix__doc__},

static PyObject *
bytearray_removeprefix_impl(PyByteArrayObject *self, Py_buffer *prefix);

static PyValue
bytearray_removeprefix(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer prefix = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &prefix, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_removeprefix_impl((PyByteArrayObject *)self, &prefix);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"removesuffix", _PyCFunction_CAST(bytearray_removesuffix), METH_O, bytearray_removesuffix__doc__},

static PyObject *
bytearray_removesuffix_impl(PyByteArrayObject *self, Py_buffer *suffix);

static PyValue
bytearray_removesuffix(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer suffix = {NULL, NULL};
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &suffix, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_removesuffix_impl((PyByteArrayObject *)self, &suffix);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"resize", _PyCFunction_CAST(bytearray_resize), METH_O, bytearray_resize__doc__},

static PyObject *
bytearray_resize_impl(PyByteArrayObject *self, Py_ssize_t size);

static PyValue
bytearray_resize(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size;
    PyObject *_return_value;

    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(arg);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_resize_impl((PyByteArrayObject *)self, size);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"take_bytes", _PyCFunction_CAST(bytearray_take_bytes), METH_FASTCALL, bytearray_take_bytes__doc__},

static PyObject *
bytearray_take_bytes_impl(PyByteArrayObject *self, PyObject *n);

static PyValue
bytearray_take_bytes(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *n = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("take_bytes", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    n = PyValue_Object(args[0]);
    if (n == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_take_bytes_impl((PyByteArrayObject *)self, n);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"translate", _PyCFunction_CAST(bytearray_translate), METH_FASTCALL|METH_KEYWORDS, bytearray_translate__doc__},

static PyObject *
bytearray_translate_impl(PyByteArrayObject *self, PyObject *table,
                         PyObject *deletechars);

static PyValue
bytearray_translate(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(delete), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *table;
    PyObject *deletechars = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    table = PyValue_Object(args[0]);
    if (table == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    deletechars = PyValue_Object(args[1]);
    if (deletechars == NULL) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_translate_impl((PyByteArrayObject *)self, table, deletechars);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"maketrans", _PyCFunction_CAST(bytearray_maketrans), METH_FASTCALL|METH_STATIC, bytearray_maketrans__doc__},

static PyObject *
bytearray_maketrans_impl(Py_buffer *frm, Py_buffer *to);

static PyValue
bytearray_maketrans(PyObject *null, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer frm = {NULL, NULL};
    Py_buffer to = {NULL, NULL};
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("maketrans", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &frm, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &to, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    _return_value = bytearray_maketrans_impl(&frm, &to);
    return_value = PyValue_FromResult(_return_value);

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
    {"replace", _PyCFunction_CAST(bytearray_replace), METH_FASTCALL|METH_KEYWORDS, bytearray_replace__doc__},

static PyObject *
bytearray_replace_impl(PyByteArrayObject *self, Py_buffer *old,
                       Py_buffer *new, Py_ssize_t count);

static PyValue
bytearray_replace(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(count), },
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
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    Py_buffer old = {NULL, NULL};
    Py_buffer new = {NULL, NULL};
    Py_ssize_t count = -1;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &old, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &new, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[2]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        count = ival;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_replace_impl((PyByteArrayObject *)self, &old, &new, count);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"split", _PyCFunction_CAST(bytearray_split), METH_FASTCALL|METH_KEYWORDS, bytearray_split__doc__},

static PyObject *
bytearray_split_impl(PyByteArrayObject *self, PyObject *sep,
                     Py_ssize_t maxsplit);

static PyValue
bytearray_split(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sep), &_Py_ID(maxsplit), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        sep = PyValue_Object(args[0]);
        if (sep == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsplit = ival;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_split_impl((PyByteArrayObject *)self, sep, maxsplit);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"partition", _PyCFunction_CAST(bytearray_partition), METH_O, bytearray_partition__doc__},

static PyObject *
bytearray_partition_impl(PyByteArrayObject *self, PyObject *sep);

static PyValue
bytearray_partition(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sep;
    PyObject *_return_value;

    sep = PyValue_Object(arg);
    if (sep == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_partition_impl((PyByteArrayObject *)self, sep);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"rpartition", _PyCFunction_CAST(bytearray_rpartition), METH_O, bytearray_rpartition__doc__},

static PyObject *
bytearray_rpartition_impl(PyByteArrayObject *self, PyObject *sep);

static PyValue
bytearray_rpartition(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sep;
    PyObject *_return_value;

    sep = PyValue_Object(arg);
    if (sep == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_rpartition_impl((PyByteArrayObject *)self, sep);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"rsplit", _PyCFunction_CAST(bytearray_rsplit), METH_FASTCALL|METH_KEYWORDS, bytearray_rsplit__doc__},

static PyObject *
bytearray_rsplit_impl(PyByteArrayObject *self, PyObject *sep,
                      Py_ssize_t maxsplit);

static PyValue
bytearray_rsplit(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sep), &_Py_ID(maxsplit), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = Py_None;
    Py_ssize_t maxsplit = -1;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        sep = PyValue_Object(args[0]);
        if (sep == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsplit = ival;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_rsplit_impl((PyByteArrayObject *)self, sep, maxsplit);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse the order of the values in B in place.");

#define BYTEARRAY_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)bytearray_reverse, METH_NOARGS, bytearray_reverse__doc__},

static PyObject *
bytearray_reverse_impl(PyByteArrayObject *self);

static PyObject *
bytearray_reverse(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_reverse_impl((PyByteArrayObject *)self);
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
    {"insert", _PyCFunction_CAST(bytearray_insert), METH_FASTCALL, bytearray_insert__doc__},

static PyObject *
bytearray_insert_impl(PyByteArrayObject *self, Py_ssize_t index, int item);

static PyValue
bytearray_insert(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    int item;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    if (!getbytevalue_value(args[1], &item)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_insert_impl((PyByteArrayObject *)self, index, item);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"append", _PyCFunction_CAST(bytearray_append), METH_O, bytearray_append__doc__},

static PyObject *
bytearray_append_impl(PyByteArrayObject *self, int item);

static PyValue
bytearray_append(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int item;
    PyObject *_return_value;

    if (!getbytevalue_value(arg, &item)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_append_impl((PyByteArrayObject *)self, item);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"extend", _PyCFunction_CAST(bytearray_extend), METH_O, bytearray_extend__doc__},

static PyObject *
bytearray_extend_impl(PyByteArrayObject *self, PyObject *iterable_of_ints);

static PyValue
bytearray_extend(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *iterable_of_ints;
    PyObject *_return_value;

    iterable_of_ints = PyValue_Object(arg);
    if (iterable_of_ints == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_extend_impl((PyByteArrayObject *)self, iterable_of_ints);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"pop", _PyCFunction_CAST(bytearray_pop), METH_FASTCALL, bytearray_pop__doc__},

static PyObject *
bytearray_pop_impl(PyByteArrayObject *self, Py_ssize_t index);

static PyValue
bytearray_pop(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index = -1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_pop_impl((PyByteArrayObject *)self, index);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"remove", _PyCFunction_CAST(bytearray_remove), METH_O, bytearray_remove__doc__},

static PyObject *
bytearray_remove_impl(PyByteArrayObject *self, int value);

static PyValue
bytearray_remove(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int value;
    PyObject *_return_value;

    if (!getbytevalue_value(arg, &value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_remove_impl((PyByteArrayObject *)self, value);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"strip", _PyCFunction_CAST(bytearray_strip), METH_FASTCALL, bytearray_strip__doc__},

static PyObject *
bytearray_strip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyValue
bytearray_strip(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *bytes = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("strip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Object(args[0]);
    if (bytes == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_strip_impl((PyByteArrayObject *)self, bytes);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"lstrip", _PyCFunction_CAST(bytearray_lstrip), METH_FASTCALL, bytearray_lstrip__doc__},

static PyObject *
bytearray_lstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyValue
bytearray_lstrip(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *bytes = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("lstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Object(args[0]);
    if (bytes == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_lstrip_impl((PyByteArrayObject *)self, bytes);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"rstrip", _PyCFunction_CAST(bytearray_rstrip), METH_FASTCALL, bytearray_rstrip__doc__},

static PyObject *
bytearray_rstrip_impl(PyByteArrayObject *self, PyObject *bytes);

static PyValue
bytearray_rstrip(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *bytes = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("rstrip", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    bytes = PyValue_Object(args[0]);
    if (bytes == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_rstrip_impl((PyByteArrayObject *)self, bytes);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"decode", _PyCFunction_CAST(bytearray_decode), METH_FASTCALL|METH_KEYWORDS, bytearray_decode__doc__},

static PyObject *
bytearray_decode_impl(PyByteArrayObject *self, const char *encoding,
                      const char *errors);

static PyValue
bytearray_decode(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(encoding), &_Py_ID(errors), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *encoding = NULL;
    const char *errors = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        {
            PyObject *_arg = PyValue_Object(args[0]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("decode", "argument 'encoding'", "str", _arg);
                goto exit;
            }
            encoding = _PyUnicode_AsUTF8NoNUL(_arg);
            if (encoding == NULL) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("decode", "argument 'errors'", "str", _arg);
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(_arg);
        if (errors == NULL) {
            goto exit;
        }
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_decode_impl((PyByteArrayObject *)self, encoding, errors);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"join", _PyCFunction_CAST(bytearray_join), METH_O, bytearray_join__doc__},

static PyObject *
bytearray_join_impl(PyByteArrayObject *self, PyObject *iterable_of_bytes);

static PyValue
bytearray_join(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *iterable_of_bytes;
    PyObject *_return_value;

    iterable_of_bytes = PyValue_Object(arg);
    if (iterable_of_bytes == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_join_impl((PyByteArrayObject *)self, iterable_of_bytes);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"splitlines", _PyCFunction_CAST(bytearray_splitlines), METH_FASTCALL|METH_KEYWORDS, bytearray_splitlines__doc__},

static PyObject *
bytearray_splitlines_impl(PyByteArrayObject *self, int keepends);

static PyValue
bytearray_splitlines(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(keepends), },
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
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int keepends = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    keepends = PyObject_IsTrueValue(args[0]);
    if (keepends < 0) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_splitlines_impl((PyByteArrayObject *)self, keepends);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"fromhex", _PyCFunction_CAST(bytearray_fromhex), METH_O|METH_CLASS, bytearray_fromhex__doc__},

static PyObject *
bytearray_fromhex_impl(PyTypeObject *type, PyObject *string);

static PyValue
bytearray_fromhex(PyObject *type, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;
    PyObject *_return_value;

    string = PyValue_Object(arg);
    if (string == NULL) {
        goto exit;
    }
    _return_value = bytearray_fromhex_impl((PyTypeObject *)type, string);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"hex", _PyCFunction_CAST(bytearray_hex), METH_FASTCALL|METH_KEYWORDS, bytearray_hex__doc__},

static PyObject *
bytearray_hex_impl(PyByteArrayObject *self, PyObject *sep,
                   Py_ssize_t bytes_per_sep);

static PyValue
bytearray_hex(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sep), &_Py_ID(bytes_per_sep), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *sep = NULL;
    Py_ssize_t bytes_per_sep = 1;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        sep = PyValue_Object(args[0]);
        if (sep == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        bytes_per_sep = ival;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_hex_impl((PyByteArrayObject *)self, sep, bytes_per_sep);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_METHODDEF    \
    {"__reduce__", (PyCFunction)bytearray_reduce, METH_NOARGS, bytearray_reduce__doc__},

static PyObject *
bytearray_reduce_impl(PyByteArrayObject *self);

static PyObject *
bytearray_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = bytearray_reduce_impl((PyByteArrayObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(bytearray_reduce_ex__doc__,
"__reduce_ex__($self, proto=0, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define BYTEARRAY_REDUCE_EX_METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(bytearray_reduce_ex), METH_FASTCALL, bytearray_reduce_ex__doc__},

static PyObject *
bytearray_reduce_ex_impl(PyByteArrayObject *self, int proto);

static PyValue
bytearray_reduce_ex(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int proto = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("__reduce_ex__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    proto = PyLong_AsIntValue(args[0]);
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = bytearray_reduce_ex_impl((PyByteArrayObject *)self, proto);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bytearray_sizeof__doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns the size of the bytearray object in memory, in bytes.");

#define BYTEARRAY_SIZEOF_METHODDEF    \
    {"__sizeof__", (PyCFunction)bytearray_sizeof, METH_NOARGS, bytearray_sizeof__doc__},

static PyObject *
bytearray_sizeof_impl(PyByteArrayObject *self);

static PyObject *
bytearray_sizeof(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return bytearray_sizeof_impl((PyByteArrayObject *)self);
}
/*[clinic end generated code: output=ad3c4f0a69e76790 input=a9049054013a1b77]*/
