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
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(batched_new__doc__,
"batched(iterable, n, *, strict=False)\n"
"--\n"
"\n"
"Batch data into tuples of length n. The last batch may be shorter than n.\n"
"\n"
"Loops over the input iterable and accumulates data into tuples\n"
"up to size n.  The input is consumed lazily, just enough to\n"
"fill a batch.  The result is yielded as soon as a batch is full\n"
"or when the input iterable is exhausted.\n"
"\n"
"    >>> for batch in batched(\'ABCDEFG\', 3):\n"
"    ...     print(batch)\n"
"    ...\n"
"    (\'A\', \'B\', \'C\')\n"
"    (\'D\', \'E\', \'F\')\n"
"    (\'G\',)\n"
"\n"
"If \"strict\" is True, raises a ValueError if the final batch is shorter\n"
"than n.");

static PyValue
batched_new_impl(PyTypeObject *type, PyValueBorrow iterable, Py_ssize_t n,
                 int strict);

static PyValue
batched_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('n')), _PyValue_STATIC_IMMORTAL(&_Py_ID(strict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "n", "strict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "batched",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow iterable;
    Py_ssize_t n;
    int strict = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[_idx[0]]);
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
        n = ival;
    }
    if (_idx[2] >= 0) {
        strict = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (strict < 0) {
            goto exit;
        }
    }
    return_value = batched_new_impl(type, iterable, n, strict);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
batched_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(batched_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = batched_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(pairwise_new__doc__,
"pairwise(iterable, /)\n"
"--\n"
"\n"
"Return an iterator of overlapping pairs taken from the input iterator.\n"
"\n"
"    s -> (s0,s1), (s1,s2), (s2, s3), ...");

static PyValue
pairwise_new_impl(PyTypeObject *type, PyValueBorrow iterable);

static PyValue
pairwise_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->pairwise_type;
    PyValueBorrow iterable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("pairwise", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pairwise", nargs, 1, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = pairwise_new_impl(type, iterable);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
pairwise_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(pairwise_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = pairwise_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_groupby__doc__,
"groupby(iterable, key=None)\n"
"--\n"
"\n"
"make an iterator that returns consecutive keys and groups from the iterable\n"
"\n"
"  iterable\n"
"    Elements to divide into groups according to the key function.\n"
"  key\n"
"    A function for computing the group category for each element.\n"
"    If the key function is not specified or is None, the element itself\n"
"    is used for grouping.");

static PyValue
itertools_groupby_impl(PyTypeObject *type, PyValueBorrow it,
                       PyValueBorrow keyfunc);

static PyValue
itertools_groupby_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "groupby",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow it;
    PyValueBorrow keyfunc = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    it = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        keyfunc = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = itertools_groupby_impl(type, it, keyfunc);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_groupby(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_groupby_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_groupby_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
itertools__grouper_impl(PyTypeObject *type, PyObject *parent,
                        PyValueBorrow tgtkey);

static PyValue
itertools__grouper_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->_grouper_type;
    PyObject *parent;
    PyValueBorrow tgtkey;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("_grouper", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_grouper", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state_by_cls()->groupby_type)) {
        _PyArg_BadArgumentValue("_grouper", "argument 1", (clinic_state_by_cls()->groupby_type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    parent = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    tgtkey = PyValue_Borrow(args[1]);
    return_value = itertools__grouper_impl(type, parent, tgtkey);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools__grouper(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools__grouper_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools__grouper_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_teedataobject__doc__,
"teedataobject(iterable, values, next, /)\n"
"--\n"
"\n"
"Data container common to multiple tee objects.");

static PyValue
itertools_teedataobject_impl(PyTypeObject *type, PyValueBorrow it,
                             PyObject *values, PyValueBorrow next);

static PyValue
itertools_teedataobject_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->teedataobject_type;
    PyValueBorrow it;
    PyObject *values;
    PyValueBorrow next;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("teedataobject", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("teedataobject", nargs, 3, 3)) {
        goto exit;
    }
    it = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyList_Type)) {
        _PyArg_BadArgumentValue("teedataobject", "argument 2", "list", PyValue_Borrow(args[1]));
        goto exit;
    }
    values = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    next = PyValue_Borrow(args[2]);
    return_value = itertools_teedataobject_impl(type, it, values, next);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_teedataobject(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_teedataobject_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_teedataobject_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools__tee__doc__,
"_tee(iterable, /)\n"
"--\n"
"\n"
"Iterator wrapped to make it copyable.");

static PyValue
itertools__tee_impl(PyTypeObject *type, PyValueBorrow iterable);

static PyValue
itertools__tee_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->tee_type;
    PyValueBorrow iterable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("_tee", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_tee", nargs, 1, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = itertools__tee_impl(type, iterable);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools__tee(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools__tee_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools__tee_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_tee__doc__,
"tee($module, iterable, n=2, /)\n"
"--\n"
"\n"
"Returns a tuple of n independent iterators.");

#define ITERTOOLS_TEE_METHODDEF    \
    {"tee", _PyCFunction_CAST(itertools_tee), METH_VALUE, itertools_tee__doc__},

static PyValue
itertools_tee_impl(PyObject *module, PyValueBorrow iterable, Py_ssize_t n);

static PyValue
itertools_tee(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;
    Py_ssize_t n = 2;

    if (!_PyArg_NoKwnamesValue("itertools.tee()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tee", nargs, 1, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
        if (n < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "n cannot be negative");
            goto exit;
        }
    }
skip_optional:
    return_value = itertools_tee_impl(PyValue_AsKnownObjectBorrow(module), iterable, n);

exit:
    return return_value;
}

PyDoc_STRVAR(itertools_cycle__doc__,
"cycle(iterable, /)\n"
"--\n"
"\n"
"Return elements from the iterable until it is exhausted. Then repeat the sequence indefinitely.");

static PyValue
itertools_cycle_impl(PyTypeObject *type, PyValueBorrow iterable);

static PyValue
itertools_cycle_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->cycle_type;
    PyValueBorrow iterable;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("cycle", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("cycle", nargs, 1, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = itertools_cycle_impl(type, iterable);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_cycle(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_cycle_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_cycle_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_dropwhile__doc__,
"dropwhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Drop items from the iterable while predicate(item) is true.\n"
"\n"
"Afterwards, return every element until the iterable is exhausted.");

static PyValue
itertools_dropwhile_impl(PyTypeObject *type, PyValueBorrow func,
                         PyValueBorrow seq);

static PyValue
itertools_dropwhile_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->dropwhile_type;
    PyValueBorrow func;
    PyValueBorrow seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("dropwhile", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("dropwhile", nargs, 2, 2)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    seq = PyValue_Borrow(args[1]);
    return_value = itertools_dropwhile_impl(type, func, seq);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_dropwhile(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_dropwhile_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_dropwhile_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_takewhile__doc__,
"takewhile(predicate, iterable, /)\n"
"--\n"
"\n"
"Return successive entries from an iterable as long as the predicate evaluates to true for each entry.");

static PyValue
itertools_takewhile_impl(PyTypeObject *type, PyValueBorrow func,
                         PyValueBorrow seq);

static PyValue
itertools_takewhile_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->takewhile_type;
    PyValueBorrow func;
    PyValueBorrow seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("takewhile", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("takewhile", nargs, 2, 2)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    seq = PyValue_Borrow(args[1]);
    return_value = itertools_takewhile_impl(type, func, seq);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_takewhile(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_takewhile_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_takewhile_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_starmap__doc__,
"starmap(function, iterable, /)\n"
"--\n"
"\n"
"Return an iterator whose values are returned from the function evaluated with an argument tuple taken from the given sequence.");

static PyValue
itertools_starmap_impl(PyTypeObject *type, PyValueBorrow func,
                       PyValueBorrow seq);

static PyValue
itertools_starmap_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->starmap_type;
    PyValueBorrow func;
    PyValueBorrow seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("starmap", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("starmap", nargs, 2, 2)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    seq = PyValue_Borrow(args[1]);
    return_value = itertools_starmap_impl(type, func, seq);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_starmap(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_starmap_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_starmap_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_chain_from_iterable__doc__,
"from_iterable($type, iterable, /)\n"
"--\n"
"\n"
"Alternative chain() constructor taking a single iterable argument that evaluates lazily.");

#define ITERTOOLS_CHAIN_FROM_ITERABLE_METHODDEF    \
    {"from_iterable", _PyCFunction_CAST(itertools_chain_from_iterable), METH_VALUE|METH_O|METH_CLASS, itertools_chain_from_iterable__doc__},

static PyValue
itertools_chain_from_iterable_impl(PyTypeObject *type, PyValueBorrow arg);

static PyValue
itertools_chain_from_iterable(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = itertools_chain_from_iterable_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), arg);

    return return_value;
}

static PyObject *
chain_next_impl(chainobject *self);

static PyObject *
chain_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = chain_next_impl((chainobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
product_next_impl(productobject *self);

static PyObject *
product_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = product_next_impl((productobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(itertools_combinations__doc__,
"combinations(iterable, r)\n"
"--\n"
"\n"
"Return successive r-length combinations of elements in the iterable.\n"
"\n"
"combinations(range(4), 3) --> (0,1,2), (0,1,3), (0,2,3), (1,2,3)");

static PyValue
itertools_combinations_impl(PyTypeObject *type, PyValueBorrow iterable,
                            Py_ssize_t r);

static PyValue
itertools_combinations_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('r')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "combinations",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow iterable;
    Py_ssize_t r;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[_idx[0]]);
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
        r = ival;
        if (r < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "r cannot be negative");
            goto exit;
        }
    }
    return_value = itertools_combinations_impl(type, iterable, r);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_combinations(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_combinations_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_combinations_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
combinations_next_impl(combinationsobject *self);

static PyObject *
combinations_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = combinations_next_impl((combinationsobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(itertools_combinations_with_replacement__doc__,
"combinations_with_replacement(iterable, r)\n"
"--\n"
"\n"
"Return successive r-length combinations of elements in the iterable allowing individual elements to have successive repeats.\n"
"\n"
"combinations_with_replacement(\'ABC\', 2) --> (\'A\',\'A\'), (\'A\',\'B\'), (\'A\',\'C\'), (\'B\',\'B\'), (\'B\',\'C\'), (\'C\',\'C\')");

static PyValue
itertools_combinations_with_replacement_impl(PyTypeObject *type,
                                             PyValueBorrow iterable,
                                             Py_ssize_t r);

static PyValue
itertools_combinations_with_replacement_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('r')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "combinations_with_replacement",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow iterable;
    Py_ssize_t r;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[_idx[0]]);
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
        r = ival;
        if (r < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "r cannot be negative");
            goto exit;
        }
    }
    return_value = itertools_combinations_with_replacement_impl(type, iterable, r);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_combinations_with_replacement(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_combinations_with_replacement_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_combinations_with_replacement_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
cwr_next_impl(cwrobject *self);

static PyObject *
cwr_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = cwr_next_impl((cwrobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(itertools_permutations__doc__,
"permutations(iterable, r=None)\n"
"--\n"
"\n"
"Return successive r-length permutations of elements in the iterable.\n"
"\n"
"permutations(range(3), 2) --> (0,1), (0,2), (1,0), (1,2), (2,0), (2,1)");

static PyValue
itertools_permutations_impl(PyTypeObject *type, PyValueBorrow iterable,
                            PyValueBorrow robj);

static PyValue
itertools_permutations_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('r')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "r", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "permutations",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow iterable;
    PyValueBorrow robj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        robj = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = itertools_permutations_impl(type, iterable, robj);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_permutations(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_permutations_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_permutations_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
permutations_next_impl(permutationsobject *self);

static PyObject *
permutations_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = permutations_next_impl((permutationsobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(itertools_accumulate__doc__,
"accumulate(iterable, func=None, *, initial=None)\n"
"--\n"
"\n"
"Return series of accumulated sums (or other binary function results).");

static PyValue
itertools_accumulate_impl(PyTypeObject *type, PyValueBorrow iterable,
                          PyValueBorrow binop, PyValueBorrow initial);

static PyValue
itertools_accumulate_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(func)), _PyValue_STATIC_IMMORTAL(&_Py_ID(initial)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "func", "initial", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "accumulate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow iterable;
    PyValueBorrow binop = PyValue_Borrow(PyValue_None);
    PyValueBorrow initial = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        binop = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        initial = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = itertools_accumulate_impl(type, iterable, binop, initial);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_accumulate(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_accumulate_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_accumulate_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
accumulate_next_impl(accumulateobject *self);

static PyObject *
accumulate_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = accumulate_next_impl((accumulateobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(itertools_compress__doc__,
"compress(data, selectors)\n"
"--\n"
"\n"
"Return data elements corresponding to true selector elements.\n"
"\n"
"Forms a shorter iterator from selected data elements using the selectors\n"
"to choose the data elements.");

static PyValue
itertools_compress_impl(PyTypeObject *type, PyValueBorrow seq1,
                        PyValueBorrow seq2);

static PyValue
itertools_compress_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(selectors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "selectors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow seq1;
    PyValueBorrow seq2;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    seq1 = PyValue_Borrow(args[_idx[0]]);
    seq2 = PyValue_Borrow(args[_idx[1]]);
    return_value = itertools_compress_impl(type, seq1, seq2);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_compress(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_compress_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_compress_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_filterfalse__doc__,
"filterfalse(function, iterable, /)\n"
"--\n"
"\n"
"Return those items of iterable for which function(item) is false.\n"
"\n"
"If function is None, return the items that are false.");

static PyValue
itertools_filterfalse_impl(PyTypeObject *type, PyValueBorrow func,
                           PyValueBorrow seq);

static PyValue
itertools_filterfalse_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->filterfalse_type;
    PyValueBorrow func;
    PyValueBorrow seq;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("filterfalse", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("filterfalse", nargs, 2, 2)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    seq = PyValue_Borrow(args[1]);
    return_value = itertools_filterfalse_impl(type, func, seq);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_filterfalse(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_filterfalse_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_filterfalse_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(itertools_count__doc__,
"count(start=0, step=1)\n"
"--\n"
"\n"
"Return a count object whose .__next__() method returns consecutive values.\n"
"\n"
"Equivalent to:\n"
"    def count(firstval=0, step=1):\n"
"        x = firstval\n"
"        while 1:\n"
"            yield x\n"
"            x += step");

static PyValue
itertools_count_impl(PyTypeObject *type, PyValueBorrow long_cnt,
                     PyValueBorrow long_step);

static PyValue
itertools_count_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(start)), _PyValue_STATIC_IMMORTAL(&_Py_ID(step)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"start", "step", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "count",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow long_cnt = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow long_step = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        long_cnt = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        long_step = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = itertools_count_impl(type, long_cnt, long_step);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
itertools_count(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(itertools_count_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = itertools_count_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
zip_longest_next_impl(ziplongestobject *self);

static PyObject *
zip_longest_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = zip_longest_next_impl((ziplongestobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=1c806fb727c32938 input=a9049054013a1b77]*/
