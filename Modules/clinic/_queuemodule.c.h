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
#include "pycore_modsupport.h"    // _PyArg_NoPositionalStack()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(simplequeue_new__doc__,
"SimpleQueue()\n"
"--\n"
"\n"
"Simple, unbounded, reentrant FIFO queue.");

static PyValue
simplequeue_new_impl(PyTypeObject *type);

static PyValue
simplequeue_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = simplequeue_get_state_by_type(type)->SimpleQueueType;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("SimpleQueue", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("SimpleQueue", kwnames)) {
        goto exit;
    }
    return_value = simplequeue_new_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
simplequeue_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(simplequeue_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = simplequeue_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_queue_SimpleQueue_put__doc__,
"put($self, /, item, block=True, timeout=None)\n"
"--\n"
"\n"
"Put the item on the queue.\n"
"\n"
"The optional \'block\' and \'timeout\' arguments are ignored, as this\n"
"method never blocks.  They are provided for compatibility with the\n"
"Queue class.");

#define _QUEUE_SIMPLEQUEUE_PUT_METHODDEF    \
    {"put", _PyCFunction_CAST(_queue_SimpleQueue_put), METH_VALUE|METH_KEYWORDS, _queue_SimpleQueue_put__doc__},

static PyValue
_queue_SimpleQueue_put_impl(simplequeueobject *self, PyValueBorrow item,
                            int block, PyValueBorrow timeout);

static PyValue
_queue_SimpleQueue_put(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(item)), _PyValue_STATIC_IMMORTAL(&_Py_ID(block)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"item", "block", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "put",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow item;
    int block = 1;
    PyValueBorrow timeout = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    item = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        block = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (block < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        timeout = PyValue_Borrow(args[_idx[2]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _queue_SimpleQueue_put_impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self), item, block, timeout);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_queue_SimpleQueue_put_nowait__doc__,
"put_nowait($self, /, item)\n"
"--\n"
"\n"
"Put an item into the queue without blocking.\n"
"\n"
"This is exactly equivalent to `put(item)` and is only provided\n"
"for compatibility with the Queue class.");

#define _QUEUE_SIMPLEQUEUE_PUT_NOWAIT_METHODDEF    \
    {"put_nowait", _PyCFunction_CAST(_queue_SimpleQueue_put_nowait), METH_VALUE|METH_KEYWORDS, _queue_SimpleQueue_put_nowait__doc__},

static PyValue
_queue_SimpleQueue_put_nowait_impl(simplequeueobject *self,
                                   PyValueBorrow item);

static PyValue
_queue_SimpleQueue_put_nowait(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(item)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"item", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "put_nowait",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow item;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    item = PyValue_Borrow(args[_idx[0]]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _queue_SimpleQueue_put_nowait_impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self), item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_queue_SimpleQueue_get__doc__,
"get($self, /, block=True, timeout=None)\n"
"--\n"
"\n"
"Remove and return an item from the queue.\n"
"\n"
"If optional args \'block\' is true and \'timeout\' is None (the\n"
"default), block if necessary until an item is available.  If\n"
"\'timeout\' is a non-negative number, it blocks at most \'timeout\'\n"
"seconds and raises the Empty exception if no item was available\n"
"within that time.\n"
"Otherwise (\'block\' is false), return an item if one is immediately\n"
"available, else raise the Empty exception (\'timeout\' is ignored\n"
"in that case).");

#define _QUEUE_SIMPLEQUEUE_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_queue_SimpleQueue_get), METH_VALUE|METH_METHOD|METH_KEYWORDS, _queue_SimpleQueue_get__doc__},

static PyValue
_queue_SimpleQueue_get_impl(simplequeueobject *self, PyTypeObject *cls,
                            int block, PyValueBorrow timeout_obj);

static PyValue
_queue_SimpleQueue_get(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(block)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"block", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int block = 1;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        block = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (block < 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        timeout_obj = PyValue_Borrow(args[_idx[1]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _queue_SimpleQueue_get_impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self), cls, block, timeout_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_queue_SimpleQueue_get_nowait__doc__,
"get_nowait($self, /)\n"
"--\n"
"\n"
"Remove and return an item from the queue without blocking.\n"
"\n"
"Only get an item if one is immediately available. Otherwise\n"
"raise the Empty exception.");

#define _QUEUE_SIMPLEQUEUE_GET_NOWAIT_METHODDEF    \
    {"get_nowait", _PyCFunction_CAST(_queue_SimpleQueue_get_nowait), METH_VALUE|METH_METHOD|METH_KEYWORDS, _queue_SimpleQueue_get_nowait__doc__},

static PyValue
_queue_SimpleQueue_get_nowait_impl(simplequeueobject *self,
                                   PyTypeObject *cls);

static PyValue
_queue_SimpleQueue_get_nowait(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "get_nowait() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _queue_SimpleQueue_get_nowait_impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_queue_SimpleQueue_empty__doc__,
"empty($self, /)\n"
"--\n"
"\n"
"Return True if the queue is empty, False otherwise (not reliable!).");

#define _QUEUE_SIMPLEQUEUE_EMPTY_METHODDEF    \
    {"empty", _PyCFunction_CAST(_queue_SimpleQueue_empty), METH_VALUE|METH_NOARGS, _queue_SimpleQueue_empty__doc__},

static int
_queue_SimpleQueue_empty_impl(simplequeueobject *self);

static PyValue
_queue_SimpleQueue_empty(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    _return_value = _queue_SimpleQueue_empty_impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_queue_SimpleQueue_qsize__doc__,
"qsize($self, /)\n"
"--\n"
"\n"
"Return the approximate size of the queue (not reliable!).");

#define _QUEUE_SIMPLEQUEUE_QSIZE_METHODDEF    \
    {"qsize", _PyCFunction_CAST(_queue_SimpleQueue_qsize), METH_VALUE|METH_NOARGS, _queue_SimpleQueue_qsize__doc__},

static Py_ssize_t
_queue_SimpleQueue_qsize_impl(simplequeueobject *self);

static PyValue
_queue_SimpleQueue_qsize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    _return_value = _queue_SimpleQueue_qsize_impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_queue_SimpleQueue___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define _QUEUE_SIMPLEQUEUE___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_queue_SimpleQueue___sizeof__), METH_VALUE|METH_NOARGS, _queue_SimpleQueue___sizeof____doc__},

static Py_ssize_t
_queue_SimpleQueue___sizeof___impl(simplequeueobject *self);

static PyValue
_queue_SimpleQueue___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    _return_value = _queue_SimpleQueue___sizeof___impl((simplequeueobject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=5c285913e151852c input=a9049054013a1b77]*/
