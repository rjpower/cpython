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
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(OrderedDict_fromkeys__doc__,
"fromkeys($type, /, iterable, value=None)\n"
"--\n"
"\n"
"Create a new ordered dictionary with keys from iterable and values set to value.");

#define ORDEREDDICT_FROMKEYS_METHODDEF    \
    {"fromkeys", _PyCFunction_CAST(OrderedDict_fromkeys), METH_VALUE|METH_CLASS, OrderedDict_fromkeys__doc__},

static PyValue
OrderedDict_fromkeys_impl(PyTypeObject *type, PyValueBorrow seq,
                          PyValueBorrow value);

static PyValue
OrderedDict_fromkeys(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(value)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "value", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromkeys",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow seq;
    PyValueBorrow value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    seq = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        value = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = OrderedDict_fromkeys_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), seq, value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define ORDEREDDICT___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(OrderedDict___sizeof__), METH_VALUE|METH_NOARGS, OrderedDict___sizeof____doc__},

static Py_ssize_t
OrderedDict___sizeof___impl(PyODictObject *self);

static PyValue
OrderedDict___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    _return_value = OrderedDict___sizeof___impl((PyODictObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling");

#define ORDEREDDICT___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(OrderedDict___reduce__), METH_VALUE|METH_NOARGS, OrderedDict___reduce____doc__},

static PyValue
OrderedDict___reduce___impl(PyODictObject *od);

static PyValue
OrderedDict___reduce__(PyValueBorrow od, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = OrderedDict___reduce___impl((PyODictObject *)PyValue_AsKnownObjectBorrow(od));

    return return_value;
}

PyDoc_STRVAR(OrderedDict_setdefault__doc__,
"setdefault($self, /, key, default=None)\n"
"--\n"
"\n"
"Insert key with a value of default if key is not in the dictionary.\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define ORDEREDDICT_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(OrderedDict_setdefault), METH_VALUE, OrderedDict_setdefault__doc__},

static PyValue
OrderedDict_setdefault_impl(PyODictObject *self, PyValueBorrow key,
                            PyValueBorrow default_value);

static PyValue
OrderedDict_setdefault(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "setdefault",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        default_value = PyValue_Borrow(args[_idx[1]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = OrderedDict_setdefault_impl((PyODictObject *)PyValue_AsKnownObjectBorrow(self), key, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_pop__doc__,
"pop($self, /, key, default=<unrepresentable>)\n"
"--\n"
"\n"
"od.pop(key[,default]) -> v, remove specified key and return the corresponding value.\n"
"\n"
"If the key is not found, return the default if given; otherwise,\n"
"raise a KeyError.");

#define ORDEREDDICT_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(OrderedDict_pop), METH_VALUE, OrderedDict_pop__doc__},

static PyValue
OrderedDict_pop_impl(PyODictObject *self, PyValueBorrow key,
                     PyValueBorrow default_value);

static PyValue
OrderedDict_pop(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "pop",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        default_value = PyValue_Borrow(args[_idx[1]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = OrderedDict_pop_impl((PyODictObject *)PyValue_AsKnownObjectBorrow(self), key, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_popitem__doc__,
"popitem($self, /, last=True)\n"
"--\n"
"\n"
"Remove and return a (key, value) pair from the dictionary.\n"
"\n"
"Pairs are returned in LIFO order if last is true or FIFO order if\n"
"false.");

#define ORDEREDDICT_POPITEM_METHODDEF    \
    {"popitem", _PyCFunction_CAST(OrderedDict_popitem), METH_VALUE, OrderedDict_popitem__doc__},

static PyValue
OrderedDict_popitem_impl(PyODictObject *self, int last);

static PyValue
OrderedDict_popitem(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(last)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"last", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "popitem",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int last = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        last = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (last < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = OrderedDict_popitem_impl((PyODictObject *)PyValue_AsKnownObjectBorrow(self), last);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(OrderedDict_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from ordered dict.");

#define ORDEREDDICT_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(OrderedDict_clear), METH_VALUE|METH_NOARGS, OrderedDict_clear__doc__},

static PyValue
OrderedDict_clear_impl(PyODictObject *self);

static PyValue
OrderedDict_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = OrderedDict_clear_impl((PyODictObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(OrderedDict_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"A shallow copy of ordered dict.");

#define ORDEREDDICT_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(OrderedDict_copy), METH_VALUE|METH_NOARGS, OrderedDict_copy__doc__},

static PyValue
OrderedDict_copy_impl(PyObject *od);

static PyValue
OrderedDict_copy(PyValueBorrow od, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(od));
    return_value = OrderedDict_copy_impl(PyValue_AsKnownObjectBorrow(od));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(OrderedDict_move_to_end__doc__,
"move_to_end($self, /, key, last=True)\n"
"--\n"
"\n"
"Move an existing element to the end (or beginning if last is false).\n"
"\n"
"Raise KeyError if the element does not exist.");

#define ORDEREDDICT_MOVE_TO_END_METHODDEF    \
    {"move_to_end", _PyCFunction_CAST(OrderedDict_move_to_end), METH_VALUE, OrderedDict_move_to_end__doc__},

static PyValue
OrderedDict_move_to_end_impl(PyODictObject *self, PyValueBorrow key,
                             int last);

static PyValue
OrderedDict_move_to_end(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(last)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "last", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "move_to_end",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow key;
    int last = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        last = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (last < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = OrderedDict_move_to_end_impl((PyODictObject *)PyValue_AsKnownObjectBorrow(self), key, last);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
odictiter_iternext_impl(PyObject *self);

static PyObject *
odictiter_iternext(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = odictiter_iternext_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=031bf732d100b7e6 input=a9049054013a1b77]*/
