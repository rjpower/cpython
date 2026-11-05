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

PyDoc_STRVAR(_interpqueues_create__doc__,
"create($module, /, maxsize, unboundop=-1, fallback=-1)\n"
"--\n"
"\n"
"Create a new cross-interpreter queue and return its unique generated ID.\n"
"\n"
"It is a new reference as though bind() had been called on the queue.\n"
"The caller is responsible for calling destroy() for the new queue\n"
"before the runtime is finalized.");

#define _INTERPQUEUES_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_interpqueues_create), METH_VALUE|METH_KEYWORDS, _interpqueues_create__doc__},

static PyValue
_interpqueues_create_impl(PyObject *module, Py_ssize_t maxsize,
                          int unboundarg, int fallbackarg);

static PyValue
_interpqueues_create(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(maxsize)), _PyValue_STATIC_IMMORTAL(&_Py_ID(unboundop)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fallback)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"maxsize", "unboundop", "fallback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_ssize_t maxsize;
    int unboundarg = -1;
    int fallbackarg = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[0]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsize = ival;
    }
    if (_idx[1] >= 0) {
        unboundarg = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (unboundarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        fallbackarg = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (fallbackarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _interpqueues_create_impl(PyValue_AsKnownObjectBorrow(module), maxsize, unboundarg, fallbackarg);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_destroy__doc__,
"destroy($module, /, qid)\n"
"--\n"
"\n"
"Clear and destroy the queue.\n"
"\n"
"Afterward attempts to use the queue will behave as though it never\n"
"existed.");

#define _INTERPQUEUES_DESTROY_METHODDEF    \
    {"destroy", _PyCFunction_CAST(_interpqueues_destroy), METH_VALUE|METH_KEYWORDS, _interpqueues_destroy__doc__},

static PyValue
_interpqueues_destroy_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_destroy(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "destroy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_destroy_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_list_all__doc__,
"list_all($module, /)\n"
"--\n"
"\n"
"Return the list of ID triples for all queues.\n"
"\n"
"Each ID triple consists of (ID, default unbound op, default fallback).");

#define _INTERPQUEUES_LIST_ALL_METHODDEF    \
    {"list_all", _PyCFunction_CAST(_interpqueues_list_all), METH_VALUE|METH_NOARGS, _interpqueues_list_all__doc__},

static PyValue
_interpqueues_list_all_impl(PyObject *module);

static PyValue
_interpqueues_list_all(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _interpqueues_list_all_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_interpqueues_put__doc__,
"put($module, /, qid, obj, unboundop=-1, fallback=-1)\n"
"--\n"
"\n"
"Add the object\'s data to the queue.");

#define _INTERPQUEUES_PUT_METHODDEF    \
    {"put", _PyCFunction_CAST(_interpqueues_put), METH_VALUE|METH_KEYWORDS, _interpqueues_put__doc__},

static PyValue
_interpqueues_put_impl(PyObject *module, int64_t qid, PyValueBorrow obj,
                       int unboundarg, int fallbackarg);

static PyValue
_interpqueues_put(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), _PyValue_STATIC_IMMORTAL(&_Py_ID(obj)), _PyValue_STATIC_IMMORTAL(&_Py_ID(unboundop)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fallback)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", "obj", "unboundop", "fallback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "put",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    int64_t qid;
    PyValueBorrow obj;
    int unboundarg = -1;
    int fallbackarg = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        unboundarg = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (unboundarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        fallbackarg = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (fallbackarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _interpqueues_put_impl(PyValue_AsKnownObjectBorrow(module), qid, obj, unboundarg, fallbackarg);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get__doc__,
"get($module, /, qid)\n"
"--\n"
"\n"
"Return the (object, unbound op) from the front of the queue.\n"
"\n"
"If there is nothing to receive then raise QueueEmpty.");

#define _INTERPQUEUES_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_interpqueues_get), METH_VALUE|METH_KEYWORDS, _interpqueues_get__doc__},

static PyValue
_interpqueues_get_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_get(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_bind__doc__,
"bind($module, /, qid)\n"
"--\n"
"\n"
"Take a reference to the identified queue.\n"
"\n"
"The queue is not destroyed until there are no references left.");

#define _INTERPQUEUES_BIND_METHODDEF    \
    {"bind", _PyCFunction_CAST(_interpqueues_bind), METH_VALUE|METH_KEYWORDS, _interpqueues_bind__doc__},

static PyValue
_interpqueues_bind_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_bind(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "bind",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_bind_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_release__doc__,
"release($module, /, qid)\n"
"--\n"
"\n"
"Release a reference to the queue.\n"
"\n"
"The queue is destroyed once there are no references left.");

#define _INTERPQUEUES_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_interpqueues_release), METH_VALUE|METH_KEYWORDS, _interpqueues_release__doc__},

static PyValue
_interpqueues_release_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_release(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "release",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_release_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get_maxsize__doc__,
"get_maxsize($module, /, qid)\n"
"--\n"
"\n"
"Return the maximum number of items in the queue.");

#define _INTERPQUEUES_GET_MAXSIZE_METHODDEF    \
    {"get_maxsize", _PyCFunction_CAST(_interpqueues_get_maxsize), METH_VALUE|METH_KEYWORDS, _interpqueues_get_maxsize__doc__},

static PyValue
_interpqueues_get_maxsize_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_get_maxsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_maxsize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_maxsize_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get_queue_defaults__doc__,
"get_queue_defaults($module, /, qid)\n"
"--\n"
"\n"
"Return the queue\'s default values, set when it was created.");

#define _INTERPQUEUES_GET_QUEUE_DEFAULTS_METHODDEF    \
    {"get_queue_defaults", _PyCFunction_CAST(_interpqueues_get_queue_defaults), METH_VALUE|METH_KEYWORDS, _interpqueues_get_queue_defaults__doc__},

static PyValue
_interpqueues_get_queue_defaults_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_get_queue_defaults(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_queue_defaults",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_queue_defaults_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_is_full__doc__,
"is_full($module, /, qid)\n"
"--\n"
"\n"
"Return true if the queue has a maxsize and has reached it.");

#define _INTERPQUEUES_IS_FULL_METHODDEF    \
    {"is_full", _PyCFunction_CAST(_interpqueues_is_full), METH_VALUE|METH_KEYWORDS, _interpqueues_is_full__doc__},

static PyValue
_interpqueues_is_full_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_is_full(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_full",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_is_full_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get_count__doc__,
"get_count($module, /, qid)\n"
"--\n"
"\n"
"Return the number of items in the queue.");

#define _INTERPQUEUES_GET_COUNT_METHODDEF    \
    {"get_count", _PyCFunction_CAST(_interpqueues_get_count), METH_VALUE|METH_KEYWORDS, _interpqueues_get_count__doc__},

static PyValue
_interpqueues_get_count_impl(PyObject *module, int64_t qid);

static PyValue
_interpqueues_get_count(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(qid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_count",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int64_t qid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!qidarg_converter(PyValue_Borrow(args[_idx[0]]), &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_count_impl(PyValue_AsKnownObjectBorrow(module), qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues__register_heap_types__doc__,
"_register_heap_types($module, /, queuetype, emptyerror, fullerror)\n"
"--\n"
"\n"
"Return the number of items in the queue.");

#define _INTERPQUEUES__REGISTER_HEAP_TYPES_METHODDEF    \
    {"_register_heap_types", _PyCFunction_CAST(_interpqueues__register_heap_types), METH_VALUE|METH_KEYWORDS, _interpqueues__register_heap_types__doc__},

static PyValue
_interpqueues__register_heap_types_impl(PyObject *module,
                                        PyTypeObject *queuetype,
                                        PyValueBorrow emptyerror,
                                        PyValueBorrow fullerror);

static PyValue
_interpqueues__register_heap_types(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(queuetype)), _PyValue_STATIC_IMMORTAL(&_Py_ID(emptyerror)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fullerror)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"queuetype", "emptyerror", "fullerror", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_register_heap_types",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyTypeObject *queuetype;
    PyValueBorrow emptyerror;
    PyValueBorrow fullerror;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyType_Type)) {
        _PyArg_BadArgumentValue("_register_heap_types", "argument 'queuetype'", (&PyType_Type)->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    queuetype = (PyTypeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    emptyerror = PyValue_Borrow(args[_idx[1]]);
    fullerror = PyValue_Borrow(args[_idx[2]]);
    return_value = _interpqueues__register_heap_types_impl(PyValue_AsKnownObjectBorrow(module), queuetype, emptyerror, fullerror);

exit:
    return return_value;
}
/*[clinic end generated code: output=c0a145b3ff85428d input=a9049054013a1b77]*/
