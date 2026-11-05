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
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(memoryview__doc__,
"memoryview(object)\n"
"--\n"
"\n"
"Create a new memoryview object which references the given object.");

static PyValue
memoryview_impl(PyTypeObject *type, PyValueBorrow object);

static PyValue
memoryview_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(object)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"object", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "memoryview",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow object;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    object = PyValue_Borrow(args[_idx[0]]);
    return_value = memoryview_impl(type, object);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
memoryview(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(memoryview_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = memoryview_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(memoryview__from_flags__doc__,
"_from_flags($type, /, object, flags)\n"
"--\n"
"\n"
"Create a new memoryview object which references the given object.");

#define MEMORYVIEW__FROM_FLAGS_METHODDEF    \
    {"_from_flags", _PyCFunction_CAST(memoryview__from_flags), METH_VALUE|METH_KEYWORDS|METH_CLASS, memoryview__from_flags__doc__},

static PyValue
memoryview__from_flags_impl(PyTypeObject *type, PyValueBorrow object,
                            int flags);

static PyValue
memoryview__from_flags(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(object)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"object", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_from_flags",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow object;
    int flags;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    object = PyValue_Borrow(args[_idx[0]]);
    flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = memoryview__from_flags_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), object, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the underlying buffer exposed by the memoryview object.");

#define MEMORYVIEW_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(memoryview_release), METH_VALUE|METH_NOARGS, memoryview_release__doc__},

static PyValue
memoryview_release_impl(PyMemoryViewObject *self);

static PyValue
memoryview_release(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = memoryview_release_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(memoryview___exit____doc__,
"__exit__($self, /, *exc_info)\n"
"--\n"
"\n"
"Release the underlying buffer exposed by the memoryview object.");

#define MEMORYVIEW___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(memoryview___exit__), METH_VALUE, memoryview___exit____doc__},

static PyValue
memoryview___exit___impl(PyMemoryViewObject *self, const PyValue *exc_info,
                         Py_ssize_t exc_info_length);

static PyValue
memoryview___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *exc_info;
    Py_ssize_t exc_info_length;

    exc_info = args;
    exc_info_length = nargs;
    return_value = memoryview___exit___impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self), exc_info, exc_info_length);

    return return_value;
}

PyDoc_STRVAR(memoryview_cast__doc__,
"cast($self, /, format, shape=<unrepresentable>, *, order=\'C\')\n"
"--\n"
"\n"
"Cast a memoryview to a new format or shape.\n"
"\n"
"With a multidimensional *shape*, *order* selects the result\n"
"layout: \'C\' for C-contiguous (row-major, the default) or \'F\'\n"
"for Fortran-contiguous (column-major).");

#define MEMORYVIEW_CAST_METHODDEF    \
    {"cast", _PyCFunction_CAST(memoryview_cast), METH_VALUE|METH_KEYWORDS, memoryview_cast__doc__},

static PyValue
memoryview_cast_impl(PyMemoryViewObject *self, PyObject *format,
                     PyValueBorrow shape, int order);

static PyValue
memoryview_cast(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), _PyValue_STATIC_IMMORTAL(&_Py_ID(shape)), _PyValue_STATIC_IMMORTAL(&_Py_ID(order)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", "shape", "order", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "cast",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyObject *format;
    PyValueBorrow shape = PyValue_Borrow(PyValue_NULL);
    int order = 'C';

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("cast", "argument 'format'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("cast", "argument 'format'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        shape = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("cast", "argument 'order'", "a unicode character", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("cast", "argument 'order'", "a unicode character", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]))) != 1) {
            PyErr_Format(PyExc_TypeError,
                "cast(): argument 'order' must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]))));
            goto exit;
        }
        order = PyUnicode_READ_CHAR(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])), 0);
    }
    return_value = memoryview_cast_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self), format, shape, order);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_toreadonly__doc__,
"toreadonly($self, /)\n"
"--\n"
"\n"
"Return a readonly version of the memoryview.");

#define MEMORYVIEW_TOREADONLY_METHODDEF    \
    {"toreadonly", _PyCFunction_CAST(memoryview_toreadonly), METH_VALUE|METH_NOARGS, memoryview_toreadonly__doc__},

static PyValue
memoryview_toreadonly_impl(PyMemoryViewObject *self);

static PyValue
memoryview_toreadonly(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = memoryview_toreadonly_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(memoryview_tolist__doc__,
"tolist($self, /)\n"
"--\n"
"\n"
"Return the data in the buffer as a list of elements.");

#define MEMORYVIEW_TOLIST_METHODDEF    \
    {"tolist", _PyCFunction_CAST(memoryview_tolist), METH_VALUE|METH_NOARGS, memoryview_tolist__doc__},

static PyValue
memoryview_tolist_impl(PyMemoryViewObject *self);

static PyValue
memoryview_tolist(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = memoryview_tolist_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(memoryview_tobytes__doc__,
"tobytes($self, /, order=\'C\')\n"
"--\n"
"\n"
"Return the data in the buffer as a byte string.\n"
"\n"
"Order can be {\'C\', \'F\', \'A\'}.  When order is \'C\' or \'F\', the data of\n"
"the original array is converted to C or Fortran order.  For\n"
"contiguous views, \'A\' returns an exact copy of the physical memory.\n"
"In particular, in-memory Fortran order is preserved.  For\n"
"non-contiguous views, the data is converted to C first.  order=None\n"
"is the same as order=\'C\'.");

#define MEMORYVIEW_TOBYTES_METHODDEF    \
    {"tobytes", _PyCFunction_CAST(memoryview_tobytes), METH_VALUE|METH_KEYWORDS, memoryview_tobytes__doc__},

static PyValue
memoryview_tobytes_impl(PyMemoryViewObject *self, const char *order);

static PyValue
memoryview_tobytes(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(order)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"order", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "tobytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *order = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("tobytes", "argument 'order'", "str or None", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])) == Py_None) {
            order = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            order = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            if (order == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("tobytes", "argument 'order'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
    }
    return_value = memoryview_tobytes_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self), order);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_hex__doc__,
"hex($self, /, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Return the data in the buffer as a str of hexadecimal numbers.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from\n"
"    the right, negative values count from the left.\n"
"\n"
"Example:\n"
">>> value = memoryview(b\'\\xb9\\x01\\xef\')\n"
">>> value.hex()\n"
"\'b901ef\'\n"
">>> value.hex(\':\')\n"
"\'b9:01:ef\'\n"
">>> value.hex(\':\', 2)\n"
"\'b9:01ef\'\n"
">>> value.hex(\':\', -2)\n"
"\'b901:ef\'");

#define MEMORYVIEW_HEX_METHODDEF    \
    {"hex", _PyCFunction_CAST(memoryview_hex), METH_VALUE|METH_KEYWORDS, memoryview_hex__doc__},

static PyValue
memoryview_hex_impl(PyMemoryViewObject *self, PyValueBorrow sep,
                    Py_ssize_t bytes_per_sep);

static PyValue
memoryview_hex(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = memoryview_hex_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self), sep, bytes_per_sep);

exit:
    return return_value;
}

PyDoc_STRVAR(memoryview_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Count the number of occurrences of a value.");

#define MEMORYVIEW_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(memoryview_count), METH_VALUE|METH_O, memoryview_count__doc__},

static PyValue
memoryview_count_impl(PyMemoryViewObject *self, PyValueBorrow value);

static PyValue
memoryview_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    value = PyValue_Borrow(args[0]);
    return_value = memoryview_count_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self), value);

    return return_value;
}

PyDoc_STRVAR(memoryview_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"Return the index of the first occurrence of a value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define MEMORYVIEW_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(memoryview_index), METH_VALUE, memoryview_index__doc__},

static PyValue
memoryview_index_impl(PyMemoryViewObject *self, PyValueBorrow value,
                      Py_ssize_t start, Py_ssize_t stop);

static PyValue
memoryview_index(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    value = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexNotNoneValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexNotNoneValue(PyValue_Borrow(args[2]), &stop)) {
        goto exit;
    }
skip_optional:
    return_value = memoryview_index_impl((PyMemoryViewObject *)PyValue_AsKnownObjectBorrow(self), value, start, stop);

exit:
    return return_value;
}
/*[clinic end generated code: output=8b09316a8c454b86 input=a9049054013a1b77]*/
