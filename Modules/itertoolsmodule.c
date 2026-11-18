#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_call.h"              // _PyObject_CallNoArgs()
#include "pycore_ceval.h"             // _PyEval_GetBuiltin()
#include "pycore_critical_section.h"  // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_dict.h"              // PyDict_GetValue()
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_long.h"              // _PyLong_GetZero()
#include "pycore_long_value.h"        // _PyValue_AsLong()
#include "pycore_moduleobject.h"      // _PyModule_GetState()
#include "pycore_value.h"              // PyValue_CLOSE()
#include "pycore_value_api.h"          // _PyValue_AsObjectAt()
#include "pycore_value_call.h"         // _PyValue_CallOneArg()
#include "pycore_value_ops.h"          // _PyValue_RichCompareBool()
#include "pycore_value_slots.h"        // _Py_VALUE_THUNK_ITERNEXT()
#include "pycore_slots.h"              // _Py_value_iternext
#include "pycore_typeobject.h"        // _PyType_GetModuleState()
#include "pycore_object.h"            // _PyObject_GC_TRACK()
#include "pycore_tuple.h"             // _PyTuple_ITEMS()

#include <stddef.h>                   // offsetof()

/* Itertools module written and maintained
   by Raymond D. Hettinger <python@rcn.com>
*/

typedef struct {
    PyTypeObject *accumulate_type;
    PyTypeObject *batched_type;
    PyTypeObject *chain_type;
    PyTypeObject *combinations_type;
    PyTypeObject *compress_type;
    PyTypeObject *count_type;
    PyTypeObject *cwr_type;
    PyTypeObject *cycle_type;
    PyTypeObject *dropwhile_type;
    PyTypeObject *filterfalse_type;
    PyTypeObject *groupby_type;
    PyTypeObject *_grouper_type;
    PyTypeObject *islice_type;
    PyTypeObject *pairwise_type;
    PyTypeObject *permutations_type;
    PyTypeObject *product_type;
    PyTypeObject *repeat_type;
    PyTypeObject *starmap_type;
    PyTypeObject *takewhile_type;
    PyTypeObject *tee_type;
    PyTypeObject *teedataobject_type;
    PyTypeObject *ziplongest_type;
} itertools_state;

static inline itertools_state *
get_module_state(PyObject *mod)
{
    void *state = _PyModule_GetState(mod);
    assert(state != NULL);
    return (itertools_state *)state;
}

static inline itertools_state *
get_module_state_by_cls(PyTypeObject *cls)
{
    void *state = _PyType_GetModuleState(cls);
    assert(state != NULL);
    return (itertools_state *)state;
}

static struct PyModuleDef itertoolsmodule;

static inline itertools_state *
find_state_by_type(PyTypeObject *tp)
{
    PyObject *mod = PyType_GetModuleByDef(tp, &itertoolsmodule);
    assert(mod != NULL);
    return get_module_state(mod);
}

// The generated code (clinic/itertoolsmodule.c.h) needs the object types
// of the type slots implemented with Argument Clinic.
typedef struct chainobject chainobject;
typedef struct productobject productobject;
typedef struct combinationsobject combinationsobject;
typedef struct cwrobject cwrobject;
typedef struct permutationsobject permutationsobject;
typedef struct accumulateobject accumulateobject;
typedef struct ziplongestobject ziplongestobject;

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module itertools
class itertools.groupby "groupbyobject *" "clinic_state()->groupby_type"
class itertools._grouper "_grouperobject *" "clinic_state()->_grouper_type"
class itertools.teedataobject "teedataobject *" "clinic_state()->teedataobject_type"
class itertools._tee "teeobject *" "clinic_state()->tee_type"
class itertools.batched "batchedobject *" "clinic_state()->batched_type"
class itertools.cycle "cycleobject *" "clinic_state()->cycle_type"
class itertools.dropwhile "dropwhileobject *" "clinic_state()->dropwhile_type"
class itertools.takewhile "takewhileobject *" "clinic_state()->takewhile_type"
class itertools.starmap "starmapobject *" "clinic_state()->starmap_type"
class itertools.chain "chainobject *" "clinic_state()->chain_type"
class itertools.product "productobject *" "clinic_state()->product_type"
class itertools.combinations "combinationsobject *" "clinic_state()->combinations_type"
class itertools.combinations_with_replacement "cwrobject *" "clinic_state()->cwr_type"
class itertools.permutations "permutationsobject *" "clinic_state()->permutations_type"
class itertools.accumulate "accumulateobject *" "clinic_state()->accumulate_type"
class itertools.zip_longest "ziplongestobject *" "clinic_state()->ziplongest_type"
class itertools.compress "compressobject *" "clinic_state()->compress_type"
class itertools.filterfalse "filterfalseobject *" "clinic_state()->filterfalse_type"
class itertools.count "countobject *" "clinic_state()->count_type"
class itertools.pairwise "pairwiseobject *" "clinic_state()->pairwise_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=1086fb2d308d75d2]*/

#define clinic_state() (find_state_by_type(type))
#define clinic_state_by_cls() (get_module_state_by_cls(base_tp))
#include "clinic/itertoolsmodule.c.h"
#undef clinic_state_by_cls
#undef clinic_state


/* batched object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *it;
    Py_ssize_t batch_size;
    bool strict;
} batchedobject;

#define batchedobject_CAST(op)  ((batchedobject *)(op))

/*[clinic input]
@permit_long_summary
@classmethod
itertools.batched.__new__ as batched_new
    iterable: object
    n: Py_ssize_t
    *
    strict: bool = False

Batch data into tuples of length n. The last batch may be shorter than n.

Loops over the input iterable and accumulates data into tuples
up to size n.  The input is consumed lazily, just enough to
fill a batch.  The result is yielded as soon as a batch is full
or when the input iterable is exhausted.

    >>> for batch in batched('ABCDEFG', 3):
    ...     print(batch)
    ...
    ('A', 'B', 'C')
    ('D', 'E', 'F')
    ('G',)

If "strict" is True, raises a ValueError if the final batch is shorter
than n.

[clinic start generated code]*/

static PyValue
batched_new_impl(PyTypeObject *type, PyValueBorrow iterable, Py_ssize_t n,
                 int strict)
/*[clinic end generated code: output=e9434456a581eb9e input=b31d8be8e8577a34]*/
{
    PyObject *it;
    batchedobject *bo;

    if (n < 1) {
        /* We could define the n==0 case to return an empty iterator
           but that is at odds with the idea that batching should
           never throw-away input data.
        */
        PyErr_SetString(PyExc_ValueError, "n must be at least one");
        return PyValue_NULL;
    }
    it = _PyValue_GetIter(iterable);
    if (it == NULL) {
        return PyValue_NULL;
    }

    /* create batchedobject structure */
    bo = (batchedobject *)type->tp_alloc(type, 0);
    if (bo == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }
    bo->batch_size = n;
    bo->it = it;
    bo->strict = (bool) strict;
    return _PyValue_FromObjectResult((PyObject *)bo);
}

static void
batched_dealloc(PyObject *op)
{
    batchedobject *bo = batchedobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(bo);
    PyObject_GC_UnTrack(bo);
    Py_XDECREF(bo->it);
    tp->tp_free(bo);
    Py_DECREF(tp);
}

static int
batched_traverse(PyObject *op, visitproc visit, void *arg)
{
    batchedobject *bo = batchedobject_CAST(op);
    Py_VISIT(Py_TYPE(bo));
    Py_VISIT(bo->it);
    return 0;
}

static _PyStatusValue
batched_next_value(PyObject *op)
{
    batchedobject *bo = batchedobject_CAST(op);
    Py_ssize_t i;
    Py_ssize_t n = FT_ATOMIC_LOAD_SSIZE_RELAXED(bo->batch_size);
    PyObject *it = bo->it;
    PyObject *result;

    if (n < 0) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    result = PyTuple_New(n);
    if (result == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    PyValue *items = _PyTuple_ITEMS(result);
    for (i=0 ; i < n ; i++) {
        _PyStatusValue next = _PyValue_IterNext(it);
        if (next.status != PYGEN_NEXT) {
            PyValue_XCLOSE(next.value);
            if (next.status < 0) {
                FT_ATOMIC_STORE_SSIZE_RELAXED(bo->batch_size, -1);
#ifndef Py_GIL_DISABLED
                Py_CLEAR(bo->it);
#endif
                Py_DECREF(result);
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
            if (i == 0) {
                FT_ATOMIC_STORE_SSIZE_RELAXED(bo->batch_size, -1);
#ifndef Py_GIL_DISABLED
                Py_CLEAR(bo->it);
#endif
                Py_DECREF(result);
                return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
            }
            if (bo->strict) {
                FT_ATOMIC_STORE_SSIZE_RELAXED(bo->batch_size, -1);
#ifndef Py_GIL_DISABLED
                Py_CLEAR(bo->it);
#endif
                Py_DECREF(result);
                PyErr_SetString(PyExc_ValueError, "batched(): incomplete batch");
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
            if (_PyTuple_Resize(&result, i) < 0) {
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
            return (_PyStatusValue){PYGEN_NEXT, _PyValue_FromObjectResult(result)};
        }
        items[i] = next.value;
    }
    return (_PyStatusValue){PYGEN_NEXT, _PyValue_FromObjectResult(result)};
}

_Py_VALUE_THUNK_ITERNEXT(batched_next, batched_next_value)

static PyType_Slot batched_slots[] = {
    {Py_tp_dealloc, batched_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)batched_new__doc__},
    {Py_tp_traverse, batched_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, batched_next},
    {_Py_value_iternext, batched_next_value},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, batched_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec batched_spec = {
    .name = "itertools.batched",
    .basicsize = sizeof(batchedobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = batched_slots,
};


/* pairwise object ***********************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *it;
    PyValue old;        /* optional value home */
    PyObject *result;
} pairwiseobject;

#define pairwiseobject_CAST(op) ((pairwiseobject *)(op))

/*[clinic input]
@classmethod
itertools.pairwise.__new__ as pairwise_new
    iterable: object
    /
Return an iterator of overlapping pairs taken from the input iterator.

    s -> (s0,s1), (s1,s2), (s2, s3), ...

[clinic start generated code]*/

static PyValue
pairwise_new_impl(PyTypeObject *type, PyValueBorrow iterable)
/*[clinic end generated code: output=f4397a5342119b45 input=6e7c3cddb431a8d6]*/
{
    PyObject *it;
    pairwiseobject *po;

    it = _PyValue_GetIter(iterable);
    if (it == NULL) {
        return PyValue_NULL;
    }
    po = (pairwiseobject *)type->tp_alloc(type, 0);
    if (po == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }
    po->it = it;
    po->old = PyValue_NULL;
    po->result = _PyTuple_FromPairTakeValue(PyValue_PrepareOwned(Py_None), PyValue_PrepareOwned(Py_None));
    if (po->result == NULL) {
        Py_DECREF(po);
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult((PyObject *)po);
}

static void
pairwise_dealloc(PyObject *op)
{
    pairwiseobject *po = pairwiseobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(po);
    PyObject_GC_UnTrack(po);
    Py_XDECREF(po->it);
    PyValue_XCLOSE(po->old);
    Py_XDECREF(po->result);
    tp->tp_free(po);
    Py_DECREF(tp);
}

static int
pairwise_traverse(PyObject *op, visitproc visit, void *arg)
{
    pairwiseobject *po = pairwiseobject_CAST(op);
    Py_VISIT(Py_TYPE(po));
    Py_VISIT(po->it);
    _Py_VISIT_VALUE(po->old);
    Py_VISIT(po->result);
    return 0;
}

static _PyStatusValue
pairwise_next_value(PyObject *op)
{
    pairwiseobject *po = pairwiseobject_CAST(op);
    PyObject *it = po->it;
    PyObject *result;

    if (it == NULL) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    if (PyValue_IsNull(po->old)) {
        _PyStatusValue first = _PyValue_IterStep(it);
        if (first.status != PYGEN_NEXT) {
            Py_CLEAR(po->it);
            return first;
        }
        PyValue prev = po->old;
        po->old = first.value;
        _PyValue_CheckHome(po->old);
        PyValue_XCLOSE(prev);
        it = po->it;
        if (it == NULL) {
            PyValue_CLEAR(po->old);
            return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
        }
    }
    PyValue old = PyValue_DUP(po->old);
    _PyStatusValue step = _PyValue_IterStep(it);
    if (step.status != PYGEN_NEXT) {
        Py_CLEAR(po->it);
        PyValue_CLEAR(po->old);
        PyValue_CLOSE(old);
        return step;
    }
    PyValue new = step.value;

    result = po->result;
    if (_PyObject_IsUniquelyReferenced(result)) {
        Py_INCREF(result);
        PyValue last_old = _PyTuple_CAST(result)->ob_item[0];
        PyValue last_new = _PyTuple_CAST(result)->ob_item[1];
        _PyTuple_CAST(result)->ob_item[0] = PyValue_DUP(old);
        _PyTuple_CAST(result)->ob_item[1] = PyValue_DUP(new);
        _PyValue_CheckHome(_PyTuple_CAST(result)->ob_item[0]);
        _PyValue_CheckHome(_PyTuple_CAST(result)->ob_item[1]);
        PyValue_CLOSE(last_old);
        PyValue_CLOSE(last_new);
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        _PyTuple_Recycle(result);
    }
    else {
        result = _PyTuple_FromPairTakeValue(PyValue_DUP(old),
                                            PyValue_DUP(new));
    }

    PyValue prev = po->old;
    po->old = new;
    _PyValue_CheckHome(po->old);
    PyValue_XCLOSE(prev);
    PyValue_CLOSE(old);
    if (result == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(result)};
}

_Py_VALUE_THUNK_ITERNEXT(pairwise_next, pairwise_next_value)

static PyType_Slot pairwise_slots[] = {
    {Py_tp_dealloc, pairwise_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)pairwise_new__doc__},
    {Py_tp_traverse, pairwise_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, pairwise_next},
    {_Py_value_iternext, pairwise_next_value},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_new, pairwise_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec pairwise_spec = {
    .name = "itertools.pairwise",
    .basicsize = sizeof(pairwiseobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = pairwise_slots,
};


/* groupby object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *it;
    PyObject *keyfunc;
    PyValue tgtkey;     /* optional value home */
    PyValue currkey;    /* optional value home */
    PyValue currvalue;  /* optional value home */
    const void *currgrouper;  /* borrowed reference */
    itertools_state *state;
} groupbyobject;

#define groupbyobject_CAST(op)  ((groupbyobject *)(op))

static PyObject *_grouper_create(groupbyobject *, PyValueBorrow);

/*[clinic input]
@permit_long_summary
@classmethod
itertools.groupby.__new__

    iterable as it: object
        Elements to divide into groups according to the key function.
    key as keyfunc: object = None
        A function for computing the group category for each element.
        If the key function is not specified or is None, the element itself
        is used for grouping.

make an iterator that returns consecutive keys and groups from the iterable
[clinic start generated code]*/

static PyValue
itertools_groupby_impl(PyTypeObject *type, PyValueBorrow it,
                       PyValueBorrow keyfunc)
/*[clinic end generated code: output=c5896f52daed8928 input=9f89fe625b20ef1a]*/
{
    groupbyobject *gbo;

    gbo = (groupbyobject *)type->tp_alloc(type, 0);
    if (gbo == NULL)
        return PyValue_NULL;
    gbo->tgtkey = PyValue_NULL;
    gbo->currkey = PyValue_NULL;
    gbo->currvalue = PyValue_NULL;
    gbo->keyfunc = _PyValue_AsObjectAt(keyfunc, BOX_SITE_FIELD);
    if (gbo->keyfunc == NULL) {
        Py_DECREF(gbo);
        return PyValue_NULL;
    }
    gbo->it = _PyValue_GetIter(it);
    if (gbo->it == NULL) {
        Py_DECREF(gbo);
        return PyValue_NULL;
    }
    gbo->state = find_state_by_type(type);
    return _PyValue_FromObjectResult((PyObject *)gbo);
}

static void
groupby_dealloc(PyObject *op)
{
    groupbyobject *gbo = groupbyobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(gbo);
    PyObject_GC_UnTrack(gbo);
    Py_XDECREF(gbo->it);
    Py_XDECREF(gbo->keyfunc);
    PyValue_XCLOSE(gbo->tgtkey);
    PyValue_XCLOSE(gbo->currkey);
    PyValue_XCLOSE(gbo->currvalue);
    tp->tp_free(gbo);
    Py_DECREF(tp);
}

static int
groupby_traverse(PyObject *op, visitproc visit, void *arg)
{
    groupbyobject *gbo = groupbyobject_CAST(op);
    Py_VISIT(Py_TYPE(gbo));
    Py_VISIT(gbo->it);
    Py_VISIT(gbo->keyfunc);
    _Py_VISIT_VALUE(gbo->tgtkey);
    _Py_VISIT_VALUE(gbo->currkey);
    _Py_VISIT_VALUE(gbo->currvalue);
    return 0;
}

/* Advance to the next item: PYGEN_NEXT, PYGEN_RETURN when the input is
   exhausted (no exception) or PYGEN_ERROR. */
Py_LOCAL_INLINE(int)
groupby_step(groupbyobject *gbo)
{
    PyValue newkey;
    _PyStatusValue step = _PyValue_IterNext(gbo->it);
    if (step.status != PYGEN_NEXT) {
        return step.status;
    }
    PyValue newvalue = step.value;

    if (gbo->keyfunc == Py_None) {
        newkey = PyValue_DUP(newvalue);
    } else {
        newkey = _PyValue_CallOneArg(_PyThreadState_GET(),
                                     PyValue_FromObjectBorrow(gbo->keyfunc),
                                     newvalue);
        if (PyValue_IsNull(newkey)) {
            PyValue_CLOSE(newvalue);
            return PYGEN_ERROR;
        }
    }

    PyValue oldvalue = gbo->currvalue;
    PyValue oldkey = gbo->currkey;
    gbo->currvalue = newvalue;
    gbo->currkey = newkey;
    _PyValue_CheckHome(gbo->currvalue);
    _PyValue_CheckHome(gbo->currkey);
    PyValue_XCLOSE(oldkey);
    PyValue_XCLOSE(oldvalue);
    return PYGEN_NEXT;
}

static PyObject *
groupby_next(PyObject *op)
{
    PyObject *grouper;
    groupbyobject *gbo = groupbyobject_CAST(op);

    gbo->currgrouper = NULL;
    /* skip to next iteration group */
    for (;;) {
        if (PyValue_IsNull(gbo->currkey))
            /* pass */;
        else if (PyValue_IsNull(gbo->tgtkey))
            break;
        else {
            /* A user-defined __eq__ can re-enter groupby and advance the iterator,
               mutating gbo->tgtkey / gbo->currkey while we are comparing them.
               Take local snapshots and hold strong references so the
               comparison sees the same values even under re-entrancy. */
            PyValue tgtkey = PyValue_DUP(gbo->tgtkey);
            PyValue currkey = PyValue_DUP(gbo->currkey);
            int rcmp = _PyValue_RichCompareBool(tgtkey, currkey, Py_EQ);
            PyValue_CLOSE(tgtkey);
            PyValue_CLOSE(currkey);

            if (rcmp == -1)
                return NULL;
            else if (rcmp == 0)
                break;
        }

        if (groupby_step(gbo) != PYGEN_NEXT)
            return NULL;
    }
    PyValue oldtgt = gbo->tgtkey;
    gbo->tgtkey = PyValue_DUP(gbo->currkey);
    _PyValue_CheckHome(gbo->tgtkey);
    PyValue_XCLOSE(oldtgt);

    grouper = _grouper_create(gbo, PyValue_Borrow(gbo->tgtkey));
    if (grouper == NULL)
        return NULL;

    return _PyTuple_FromPairTakeValue(PyValue_DUP(gbo->currkey),
                                      PyValue_FromObjectSteal(grouper));
}

static PyType_Slot groupby_slots[] = {
    {Py_tp_dealloc, groupby_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_groupby__doc__},
    {Py_tp_traverse, groupby_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, groupby_next},
    {Py_tp_new, itertools_groupby},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec groupby_spec = {
    .name = "itertools.groupby",
    .basicsize= sizeof(groupbyobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = groupby_slots,
};

/* _grouper object (internal) ************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *parent;
    PyValue tgtkey;     /* required value home */
} _grouperobject;

#define _grouperobject_CAST(op) ((_grouperobject *)(op))

/*[clinic input]
@classmethod
itertools._grouper.__new__

    parent: object(subclass_of='clinic_state_by_cls()->groupby_type')
    tgtkey: object
    /
[clinic start generated code]*/

static PyValue
itertools__grouper_impl(PyTypeObject *type, PyObject *parent,
                        PyValueBorrow tgtkey)
/*[clinic end generated code: output=bc293e045a6457b6 input=afe05eb477118f12]*/
{
    return _PyValue_FromObjectResult(
        _grouper_create(groupbyobject_CAST(parent), tgtkey));
}

static PyObject *
_grouper_create(groupbyobject *parent, PyValueBorrow tgtkey)
{
    itertools_state *state = parent->state;
    _grouperobject *igo = PyObject_GC_New(_grouperobject, state->_grouper_type);
    if (igo == NULL)
        return NULL;
    igo->parent = Py_NewRef(parent);
    igo->tgtkey = PyValue_DUP(tgtkey);
    _PyValue_CheckHome(igo->tgtkey);
    parent->currgrouper = igo;  /* borrowed reference */

    PyObject_GC_Track(igo);
    return (PyObject *)igo;
}

static void
_grouper_dealloc(PyObject *op)
{
    _grouperobject *igo = _grouperobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(igo);
    PyObject_GC_UnTrack(igo);
    Py_DECREF(igo->parent);
    PyValue_CLOSE(igo->tgtkey);
    PyObject_GC_Del(igo);
    Py_DECREF(tp);
}

static int
_grouper_traverse(PyObject *op, visitproc visit, void *arg)
{
    _grouperobject *igo = _grouperobject_CAST(op);
    Py_VISIT(Py_TYPE(igo));
    Py_VISIT(igo->parent);
    _Py_VISIT_VALUE(igo->tgtkey);
    return 0;
}

static _PyStatusValue
_grouper_next_value(PyObject *op)
{
    _grouperobject *igo = _grouperobject_CAST(op);
    groupbyobject *gbo = groupbyobject_CAST(igo->parent);
    int rcmp;

    if (gbo->currgrouper != igo)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    if (PyValue_IsNull(gbo->currvalue)) {
        int status = groupby_step(gbo);
        if (status != PYGEN_NEXT)
            return (_PyStatusValue){status, PyValue_NULL};
    }

    assert(!PyValue_IsNull(gbo->currkey));
    /* A user-defined __eq__ can re-enter the grouper and advance the iterator,
       mutating gbo->currkey while we are comparing them.
       Take local snapshots and hold strong references so the comparison
       sees the same values even under re-entrancy. */
    PyValue tgtkey = PyValue_DUP(igo->tgtkey);
    PyValue currkey = PyValue_DUP(gbo->currkey);
    rcmp = _PyValue_RichCompareBool(tgtkey, currkey, Py_EQ);
    PyValue_CLOSE(tgtkey);
    PyValue_CLOSE(currkey);

    if (rcmp <= 0) {
        /* got any error or current group is end */
        return (_PyStatusValue){rcmp < 0 ? PYGEN_ERROR : PYGEN_RETURN,
                                PyValue_NULL};
    }

    /* A re-entrant __eq__ may have taken the value already. */
    PyValue r = gbo->currvalue;
    gbo->currvalue = PyValue_NULL;
    PyValue_CLEAR(gbo->currkey);

    if (PyValue_IsNull(r)) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    return (_PyStatusValue){PYGEN_NEXT, r};
}

_Py_VALUE_THUNK_ITERNEXT(_grouper_next, _grouper_next_value)

static PyType_Slot _grouper_slots[] = {
    {Py_tp_dealloc, _grouper_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, _grouper_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, _grouper_next},
    {_Py_value_iternext, _grouper_next_value},
    {Py_tp_new, itertools__grouper},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec _grouper_spec = {
    .name = "itertools._grouper",
    .basicsize = sizeof(_grouperobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = _grouper_slots,
};


/* tee object and with supporting function and objects ***********************/

/* The teedataobject pre-allocates space for LINKCELLS number of objects.
   To help the object fit neatly inside cache lines (space for 16 to 32
   pointers), the value should be a multiple of 16 minus  space for
   the other structure members including PyHEAD overhead.  The larger the
   value, the less memory overhead per object and the less time spent
   allocating/deallocating new links.  The smaller the number, the less
   wasted space and the more rapid freeing of older data.
*/
#define LINKCELLS 57

typedef struct {
    PyObject_HEAD
    PyObject *it;
    int numread;                /* 0 <= numread <= LINKCELLS */
    int running;
    PyObject *nextlink;
    PyValue values[LINKCELLS];  /* owners below numread, unset above */
} teedataobject;

#define teedataobject_CAST(op)  ((teedataobject *)(op))

typedef struct {
    PyObject_HEAD
    teedataobject *dataobj;
    int index;                  /* 0 <= index <= LINKCELLS */
    PyObject *weakreflist;
    itertools_state *state;
} teeobject;

#define teeobject_CAST(op)  ((teeobject *)(op))

static PyObject *
teedataobject_newinternal(itertools_state *state, PyObject *it)
{
    teedataobject *tdo;

    tdo = PyObject_GC_New(teedataobject, state->teedataobject_type);
    if (tdo == NULL)
        return NULL;

    tdo->running = 0;
    tdo->numread = 0;
    tdo->nextlink = NULL;
    tdo->it = Py_NewRef(it);
    PyObject_GC_Track(tdo);
    return (PyObject *)tdo;
}

static PyObject *
teedataobject_jumplink(itertools_state *state, teedataobject *tdo)
{
    PyObject *link;
    Py_BEGIN_CRITICAL_SECTION(tdo);
    if (tdo->nextlink == NULL)
        tdo->nextlink = teedataobject_newinternal(state, tdo->it);
    link = Py_XNewRef(tdo->nextlink);
    Py_END_CRITICAL_SECTION();
    return link;
}

static _PyStatusValue
teedataobject_getitem_lock_held(teedataobject *tdo, int i)
{
    assert(i < LINKCELLS);
    if (i < tdo->numread) {
        return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(tdo->values[i])};
    }
    /* this is the lead iterator, so fetch more data */
    assert(i == tdo->numread);
    if (tdo->running) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot re-enter the tee iterator");
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    tdo->running = 1;
    _PyStatusValue step = _PyValue_IterNext(tdo->it);
    tdo->running = 0;
    if (step.status != PYGEN_NEXT) {
        return step;
    }
    tdo->numread++;
    tdo->values[i] = step.value;
    _PyValue_CheckHome(tdo->values[i]);
    return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(tdo->values[i])};
}

static _PyStatusValue
teedataobject_getitem(teedataobject *tdo, int i)
{
    _PyStatusValue result;
    Py_BEGIN_CRITICAL_SECTION(tdo);
    result = teedataobject_getitem_lock_held(tdo, i);
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
teedataobject_traverse(PyObject *op, visitproc visit, void * arg)
{
    int i;
    teedataobject *tdo = teedataobject_CAST(op);

    Py_VISIT(Py_TYPE(tdo));
    Py_VISIT(tdo->it);
    for (i = 0; i < tdo->numread; i++)
        _Py_VISIT_VALUE(tdo->values[i]);
    Py_VISIT(tdo->nextlink);
    return 0;
}

static void
teedataobject_safe_decref(PyObject *obj)
{
    while (obj && _PyObject_IsUniquelyReferenced(obj)) {
        teedataobject *tmp = teedataobject_CAST(obj);
        PyObject *nextlink;
        Py_BEGIN_CRITICAL_SECTION(obj);
        nextlink = tmp->nextlink;
        tmp->nextlink = NULL;
        Py_END_CRITICAL_SECTION();
        Py_SETREF(obj, nextlink);
    }
    Py_XDECREF(obj);
}

static int
teedataobject_clear(PyObject *op)
{
    int i;
    PyObject *tmp;
    teedataobject *tdo = teedataobject_CAST(op);

    Py_BEGIN_CRITICAL_SECTION(op);
    Py_CLEAR(tdo->it);
    for (i=0 ; i<tdo->numread ; i++)
        PyValue_CLEAR(tdo->values[i]);
    tmp = tdo->nextlink;
    tdo->nextlink = NULL;
    Py_END_CRITICAL_SECTION();
    teedataobject_safe_decref(tmp);
    return 0;
}

static void
teedataobject_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)teedataobject_clear(op);
    PyObject_GC_Del(op);
    Py_DECREF(tp);
}

/*[clinic input]
@classmethod
itertools.teedataobject.__new__
    iterable as it: object
    values: object(subclass_of='&PyList_Type')
    next: object
    /
Data container common to multiple tee objects.
[clinic start generated code]*/

static PyValue
itertools_teedataobject_impl(PyTypeObject *type, PyValueBorrow it,
                             PyObject *values, PyValueBorrow next)
/*[clinic end generated code: output=fc1e303029fde78a input=be60f2fabd2b72ba]*/
{
    teedataobject *tdo;
    Py_ssize_t i, len;

    itertools_state *state = get_module_state_by_cls(type);
    assert(type == state->teedataobject_type);

    PyObject *it_obj = _PyValue_AsObjectAt(it, BOX_SITE_PARSER);
    if (it_obj == NULL)
        return PyValue_NULL;
    tdo = (teedataobject *)teedataobject_newinternal(state, it_obj);
    Py_DECREF(it_obj);
    if (!tdo)
        return PyValue_NULL;

    len = PyList_GET_SIZE(values);
    if (len > LINKCELLS)
        goto err;
    for (i=0; i<len; i++) {
        PyValue value = PyList_GetValue(values, i);
        if (PyValue_IsNull(value)) {
            Py_DECREF(tdo);
            return PyValue_NULL;
        }
        tdo->values[i] = value;
        _PyValue_CheckHome(tdo->values[i]);
        /* len <= LINKCELLS < INT_MAX */
        tdo->numread = Py_SAFE_DOWNCAST(i + 1, Py_ssize_t, int);
    }

    if (len == LINKCELLS) {
        if (!PyValue_IsNone(next)) {
            if (PyValue_TYPE(next) != state->teedataobject_type)
                goto err;
            assert(tdo->nextlink == NULL);
            tdo->nextlink = Py_NewRef(PyValue_AsKnownObjectBorrow(next));
        }
    } else {
        if (!PyValue_IsNone(next))
            goto err; /* shouldn't have a next if we are not full */
    }
    return _PyValue_FromObjectResult((PyObject*)tdo);

err:
    Py_XDECREF(tdo);
    PyErr_SetString(PyExc_ValueError, "Invalid arguments");
    return PyValue_NULL;
}

static PyType_Slot teedataobject_slots[] = {
    {Py_tp_dealloc, teedataobject_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_teedataobject__doc__},
    {Py_tp_traverse, teedataobject_traverse},
    {Py_tp_clear, teedataobject_clear},
    {Py_tp_new, itertools_teedataobject},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec teedataobject_spec = {
    .name = "itertools._tee_dataobject",
    .basicsize = sizeof(teedataobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = teedataobject_slots,
};


static _PyStatusValue
tee_next_value(PyObject *op)
{
    teeobject *to = teeobject_CAST(op);
    _PyStatusValue value;

#ifndef Py_GIL_DISABLED
    /* The GIL already serializes access, so keep the simple path without the
       snapshot and revalidation that the free-threaded build needs. */
    if (to->index >= LINKCELLS) {
        PyObject *link = teedataobject_jumplink(to->state, to->dataobj);
        if (link == NULL) {
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        }
        Py_SETREF(to->dataobj, (teedataobject *)link);
        to->index = 0;
    }
    value = teedataobject_getitem(to->dataobj, to->index);
    if (value.status != PYGEN_NEXT) {
        return value;
    }
    to->index++;
    return value;
#else
    for (;;) {
        teedataobject *dataobj;
        int index;

        /* Snapshot the branch position (strong ref to the shared data object)
           under the tee lock; the data object is locked separately, not nested,
           then the advance is revalidated. */
        Py_BEGIN_CRITICAL_SECTION(op);
        dataobj = (teedataobject *)Py_NewRef((PyObject *)to->dataobj);
        index = to->index;
        Py_END_CRITICAL_SECTION();

        if (index < LINKCELLS) {
            value = teedataobject_getitem(dataobj, index);
            if (value.status == PYGEN_NEXT) {
                Py_BEGIN_CRITICAL_SECTION(op);
                if (to->dataobj == dataobj && to->index == index) {
                    to->index = index + 1;
                }
                Py_END_CRITICAL_SECTION();
            }
            Py_DECREF(dataobj);
            return value;
        }

        PyObject *link = teedataobject_jumplink(to->state, dataobj);
        if (link == NULL) {
            Py_DECREF(dataobj);
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        }
        Py_BEGIN_CRITICAL_SECTION(op);
        if (to->dataobj == dataobj) {
            Py_SETREF(to->dataobj, (teedataobject *)link);
            to->index = 0;
            link = NULL;
        }
        Py_END_CRITICAL_SECTION();
        Py_XDECREF(link);
        Py_DECREF(dataobj);
    }
#endif
}

_Py_VALUE_THUNK_ITERNEXT(tee_next, tee_next_value)

static int
tee_traverse(PyObject *op, visitproc visit, void *arg)
{
    teeobject *to = teeobject_CAST(op);
    Py_VISIT(Py_TYPE(to));
    Py_VISIT((PyObject *)to->dataobj);
    return 0;
}

static teeobject *
tee_copy_impl(teeobject *to)
{
    teeobject *newto = PyObject_GC_New(teeobject, Py_TYPE(to));
    if (newto == NULL) {
        return NULL;
    }
    Py_BEGIN_CRITICAL_SECTION(to);
    newto->dataobj = (teedataobject *)Py_NewRef(to->dataobj);
    newto->index = to->index;
    Py_END_CRITICAL_SECTION();
    newto->weakreflist = NULL;
    newto->state = to->state;
    PyObject_GC_Track(newto);
    return newto;
}

static inline PyObject *
tee_copy(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    teeobject *to = teeobject_CAST(op);
    return (PyObject *)tee_copy_impl(to);
}

PyDoc_STRVAR(teecopy_doc, "Returns an independent iterator.");

static PyObject *
tee_fromiterable(itertools_state *state, PyValueBorrow iterable)
{
    teeobject *to;
    PyObject *it;

    it = _PyValue_GetIter(iterable);
    if (it == NULL)
        return NULL;
    if (PyObject_TypeCheck(it, state->tee_type)) {
        to = tee_copy_impl((teeobject *)it);  // 'it' can be fast casted
        goto done;
    }

    PyObject *dataobj = teedataobject_newinternal(state, it);
    if (!dataobj) {
        to = NULL;
        goto done;
    }
    to = PyObject_GC_New(teeobject, state->tee_type);
    if (to == NULL) {
        Py_DECREF(dataobj);
        goto done;
    }
    to->dataobj = (teedataobject *)dataobj;
    to->index = 0;
    to->weakreflist = NULL;
    to->state = state;
    PyObject_GC_Track(to);
done:
    Py_DECREF(it);
    return (PyObject *)to;
}

/*[clinic input]
@classmethod
itertools._tee.__new__
    iterable: object
    /
Iterator wrapped to make it copyable.
[clinic start generated code]*/

static PyValue
itertools__tee_impl(PyTypeObject *type, PyValueBorrow iterable)
/*[clinic end generated code: output=1fe98874e71a3759 input=adc0779d2afe37a2]*/
{
    itertools_state *state = get_module_state_by_cls(type);
    return _PyValue_FromObjectResult(tee_fromiterable(state, iterable));
}

static int
tee_clear(PyObject *op)
{
    teeobject *to = teeobject_CAST(op);
    if (to->weakreflist != NULL)
        PyObject_ClearWeakRefs(op);
    Py_CLEAR(to->dataobj);
    return 0;
}

static void
tee_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)tee_clear(op);
    PyObject_GC_Del(op);
    Py_DECREF(tp);
}

static PyMethodDef tee_methods[] = {
    {"__copy__", tee_copy, METH_NOARGS, teecopy_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyMemberDef tee_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(teeobject, weakreflist), Py_READONLY},
    {NULL},
};

static PyType_Slot tee_slots[] = {
    {Py_tp_dealloc, tee_dealloc},
    {Py_tp_doc, (void *)itertools__tee__doc__},
    {Py_tp_traverse, tee_traverse},
    {Py_tp_clear, tee_clear},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, tee_next},
    {_Py_value_iternext, tee_next_value},
    {Py_tp_methods, tee_methods},
    {Py_tp_members, tee_members},
    {Py_tp_new, itertools__tee},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec tee_spec = {
    .name = "itertools._tee",
    .basicsize = sizeof(teeobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = tee_slots,
};

/*[clinic input]
itertools.tee
    iterable: object
    n: Py_ssize_t(allow_negative=False) = 2
    /
Returns a tuple of n independent iterators.
[clinic start generated code]*/

static PyValue
itertools_tee_impl(PyObject *module, PyValueBorrow iterable, Py_ssize_t n)
/*[clinic end generated code: output=925ade1e99f3aafc input=0f72d78e655f45cb]*/
{
    Py_ssize_t i;
    PyObject *it, *to, *result;

    result = PyTuple_New(n);
    if (result == NULL)
        return PyValue_NULL;
    if (n == 0)
        return _PyValue_FromObjectResult(result);
    it = _PyValue_GetIter(iterable);
    if (it == NULL) {
        Py_DECREF(result);
        return PyValue_NULL;
    }

    itertools_state *state = get_module_state(module);
    to = tee_fromiterable(state, PyValue_FromObjectBorrow(it));
    Py_DECREF(it);
    if (to == NULL) {
        Py_DECREF(result);
        return PyValue_NULL;
    }

    PyTuple_SET_VALUE(result, 0, PyValue_PrepareOwned(to));
    for (i = 1; i < n; i++) {
        to = tee_copy(to, NULL);
        if (to == NULL) {
            Py_DECREF(result);
            return PyValue_NULL;
        }
        PyTuple_SET_VALUE(result, i, PyValue_PrepareOwned(to));
    }
    return _PyValue_FromObjectResult(result);
}


/* cycle object **************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *it;
    PyObject *saved;
    Py_ssize_t index;
} cycleobject;

#define cycleobject_CAST(op)    ((cycleobject *)(op))

/*[clinic input]
@permit_long_summary
@classmethod
itertools.cycle.__new__
    iterable: object
    /
Return elements from the iterable until it is exhausted. Then repeat the sequence indefinitely.
[clinic start generated code]*/

static PyValue
itertools_cycle_impl(PyTypeObject *type, PyValueBorrow iterable)
/*[clinic end generated code: output=9f4281f0801e78ac input=ead392f4aac7afd8]*/
{
    PyObject *it;
    PyObject *saved;
    cycleobject *lz;

    /* Get iterator. */
    it = _PyValue_GetIter(iterable);
    if (it == NULL)
        return PyValue_NULL;

    saved = PyList_New(0);
    if (saved == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }

    /* create cycleobject structure */
    lz = (cycleobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        Py_DECREF(saved);
        return PyValue_NULL;
    }
    lz->it = it;
    lz->saved = saved;
    lz->index = -1;

    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
cycle_dealloc(PyObject *op)
{
    cycleobject *lz = cycleobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->it);
    Py_XDECREF(lz->saved);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
cycle_traverse(PyObject *op, visitproc visit, void *arg)
{
    cycleobject *lz = cycleobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->it);
    Py_VISIT(lz->saved);
    return 0;
}

static _PyStatusValue
cycle_next_value(PyObject *op)
{
    cycleobject *lz = cycleobject_CAST(op);
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(lz->index);

    if (index < 0) {
        _PyStatusValue item = _PyValue_IterNext(lz->it);
        if (item.status == PYGEN_NEXT) {
            if (PyList_AppendValue(lz->saved, item.value) < 0) {
                PyValue_CLOSE(item.value);
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
            return item;
        }
        if (item.status < 0)
            return item;
        index = 0;
        FT_ATOMIC_STORE_SSIZE_RELAXED(lz->index, 0);
#ifndef Py_GIL_DISABLED
        Py_CLEAR(lz->it);
#endif
    }
    if (PyList_GET_SIZE(lz->saved) == 0)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    PyValue saved_item = PyList_GetValue(lz->saved, index);
    if (PyValue_IsNull(saved_item))
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    index++;
    if (index >= PyList_GET_SIZE(lz->saved)) {
        index = 0;
    }
    FT_ATOMIC_STORE_SSIZE_RELAXED(lz->index, index);
    return (_PyStatusValue){PYGEN_NEXT, saved_item};
}

_Py_VALUE_THUNK_ITERNEXT(cycle_next, cycle_next_value)

static PyType_Slot cycle_slots[] = {
    {Py_tp_dealloc, cycle_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_cycle__doc__},
    {Py_tp_traverse, cycle_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, cycle_next},
    {_Py_value_iternext, cycle_next_value},
    {Py_tp_new, itertools_cycle},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec cycle_spec = {
    .name = "itertools.cycle",
    .basicsize = sizeof(cycleobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = cycle_slots,
};


/* dropwhile object **********************************************************/

typedef struct {
    PyObject_HEAD
    PyValue func;
    PyObject *it;
    long start;
} dropwhileobject;

#define dropwhileobject_CAST(op)    ((dropwhileobject *)(op))

/*[clinic input]
@classmethod
itertools.dropwhile.__new__
    predicate as func: object
    iterable as seq: object
    /
Drop items from the iterable while predicate(item) is true.

Afterwards, return every element until the iterable is exhausted.
[clinic start generated code]*/

static PyValue
itertools_dropwhile_impl(PyTypeObject *type, PyValueBorrow func,
                         PyValueBorrow seq)
/*[clinic end generated code: output=f62eacfceb14c30a input=d39737147c9f0a26]*/
{
    PyObject *it;
    dropwhileobject *lz;

    /* Get iterator. */
    it = _PyValue_GetIter(seq);
    if (it == NULL)
        return PyValue_NULL;

    /* create dropwhileobject structure */
    lz = (dropwhileobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }
    lz->it = it;
    lz->func = PyValue_DUP(func);
    _PyValue_CheckHome(lz->func);
    lz->start = 0;

    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
dropwhile_dealloc(PyObject *op)
{
    dropwhileobject *lz = dropwhileobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    PyValue_CLOSE(lz->func);
    Py_XDECREF(lz->it);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
dropwhile_traverse(PyObject *op, visitproc visit, void *arg)
{
    dropwhileobject *lz = dropwhileobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->it);
    _Py_VISIT_VALUE(lz->func);
    return 0;
}

static _PyStatusValue
dropwhile_next_value(PyObject *op)
{
    dropwhileobject *lz = dropwhileobject_CAST(op);
    PyObject *it = lz->it;
    for (;;) {
        _PyStatusValue item = _PyValue_IterStep(it);
        if (item.status != PYGEN_NEXT)
            return item;
        if (lz->start == 1)
            return item;

        PyValue good = _PyValue_CallOneArg(_PyThreadState_GET(), lz->func,
                                           item.value);
        if (PyValue_IsNull(good)) {
            PyValue_CLOSE(item.value);
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        }
        int ok = _PyValue_IsTrue(good);
        PyValue_CLOSE(good);
        if (ok == 0) {
            lz->start = 1;
            return item;
        }
        PyValue_CLOSE(item.value);
        if (ok < 0)
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
}

_Py_VALUE_THUNK_ITERNEXT(dropwhile_next, dropwhile_next_value)

static PyType_Slot dropwhile_slots[] = {
    {Py_tp_dealloc, dropwhile_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_dropwhile__doc__},
    {Py_tp_traverse, dropwhile_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, dropwhile_next},
    {_Py_value_iternext, dropwhile_next_value},
    {Py_tp_new, itertools_dropwhile},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec dropwhile_spec = {
    .name = "itertools.dropwhile",
    .basicsize = sizeof(dropwhileobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = dropwhile_slots,
};


/* takewhile object **********************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *func;
    PyObject *it;
    long stop;
} takewhileobject;

#define takewhileobject_CAST(op)    ((takewhileobject *)(op))

/*[clinic input]
@permit_long_summary
@classmethod
itertools.takewhile.__new__
    predicate as func: object
    iterable as seq: object
    /
Return successive entries from an iterable as long as the predicate evaluates to true for each entry.
[clinic start generated code]*/

static PyValue
itertools_takewhile_impl(PyTypeObject *type, PyValueBorrow func,
                         PyValueBorrow seq)
/*[clinic end generated code: output=5ba96188c8bc1dca input=61e42255dd0a7657]*/
{
    PyObject *it;
    takewhileobject *lz;

    /* Get iterator. */
    it = _PyValue_GetIter(seq);
    if (it == NULL)
        return PyValue_NULL;

    /* create takewhileobject structure */
    lz = (takewhileobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }
    lz->it = it;
    lz->func = _PyValue_AsObjectAt(func, BOX_SITE_FIELD);
    if (lz->func == NULL) {
        Py_DECREF(lz);
        return PyValue_NULL;
    }
    lz->stop = 0;

    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
takewhile_dealloc(PyObject *op)
{
    takewhileobject *lz = takewhileobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->func);
    Py_XDECREF(lz->it);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
takewhile_traverse(PyObject *op, visitproc visit, void *arg)
{
    takewhileobject *lz = takewhileobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->it);
    Py_VISIT(lz->func);
    return 0;
}

static _PyStatusValue
takewhile_next_value(PyObject *op)
{
    takewhileobject *lz = takewhileobject_CAST(op);
    if (lz->stop == 1)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};

    _PyStatusValue step = _PyValue_IterStep(lz->it);
    if (step.status != PYGEN_NEXT) {
        return step;
    }

    PyValue good = _PyValue_CallOneArg(_PyThreadState_GET(),
                                      PyValue_FromObjectBorrow(lz->func),
                                      step.value);
    if (PyValue_IsNull(good)) {
        PyValue_CLOSE(step.value);
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    int ok = _PyValue_IsTrue(good);
    PyValue_CLOSE(good);
    if (ok > 0)
        return step;
    PyValue_CLOSE(step.value);
    if (ok == 0)
        lz->stop = 1;
    return (_PyStatusValue){ok < 0 ? PYGEN_ERROR : PYGEN_RETURN,
                            PyValue_NULL};
}

_Py_VALUE_THUNK_ITERNEXT(takewhile_next, takewhile_next_value)

static PyType_Slot takewhile_slots[] = {
    {Py_tp_dealloc, takewhile_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_takewhile__doc__},
    {Py_tp_traverse, takewhile_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, takewhile_next},
    {_Py_value_iternext, takewhile_next_value},
    {Py_tp_new, itertools_takewhile},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec takewhile_spec = {
    .name = "itertools.takewhile",
    .basicsize = sizeof(takewhileobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = takewhile_slots,
};


/* islice object *************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *it;
    Py_ssize_t next;
    Py_ssize_t stop;
    Py_ssize_t step;
    Py_ssize_t cnt;
} isliceobject;

#define isliceobject_CAST(op)   ((isliceobject *)(op))

static PyObject *
islice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *seq;
    Py_ssize_t start=0, stop=-1, step=1;
    PyObject *it, *a1=NULL, *a2=NULL, *a3=NULL;
    Py_ssize_t numargs;
    isliceobject *lz;

    itertools_state *st = find_state_by_type(type);
    PyTypeObject *islice_type = st->islice_type;
    if ((type == islice_type || type->tp_init == islice_type->tp_init) &&
        !_PyArg_NoKeywords("islice", kwds))
        return NULL;

    if (!PyArg_UnpackTuple(args, "islice", 2, 4, &seq, &a1, &a2, &a3))
        return NULL;

    numargs = PyTuple_Size(args);
    if (numargs == 2) {
        if (a1 != Py_None) {
            stop = _PyValue_AsSsize_t(PyValue_FromObjectBorrow(a1), PyExc_OverflowError);
            if (stop == -1) {
                if (PyErr_Occurred())
                    PyErr_Clear();
                PyErr_SetString(PyExc_ValueError,
                   "Stop argument for islice() must be None or "
                   "an integer: 0 <= x <= sys.maxsize.");
                return NULL;
            }
        }
    } else {
        if (a1 != Py_None)
            start = _PyValue_AsSsize_t(PyValue_FromObjectBorrow(a1), PyExc_OverflowError);
        if (start == -1 && PyErr_Occurred())
            PyErr_Clear();
        if (a2 != Py_None) {
            stop = _PyValue_AsSsize_t(PyValue_FromObjectBorrow(a2), PyExc_OverflowError);
            if (stop == -1) {
                if (PyErr_Occurred())
                    PyErr_Clear();
                PyErr_SetString(PyExc_ValueError,
                   "Stop argument for islice() must be None or "
                   "an integer: 0 <= x <= sys.maxsize.");
                return NULL;
            }
        }
    }
    if (start<0 || stop<-1) {
        PyErr_SetString(PyExc_ValueError,
           "Indices for islice() must be None or "
           "an integer: 0 <= x <= sys.maxsize.");
        return NULL;
    }

    if (a3 != NULL) {
        if (a3 != Py_None)
            step = _PyValue_AsSsize_t(PyValue_FromObjectBorrow(a3), PyExc_OverflowError);
        if (step == -1 && PyErr_Occurred())
            PyErr_Clear();
    }
    if (step<1) {
        PyErr_SetString(PyExc_ValueError,
           "Step for islice() must be a positive integer or None.");
        return NULL;
    }

    /* Get iterator. */
    it = _PyValue_GetIter(PyValue_FromObjectBorrow(seq));
    if (it == NULL)
        return NULL;

    /* create isliceobject structure */
    lz = (isliceobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return NULL;
    }
    lz->it = it;
    lz->next = start;
    lz->stop = stop;
    lz->step = step;
    lz->cnt = 0L;

    return (PyObject *)lz;
}

static void
islice_dealloc(PyObject *op)
{
    isliceobject *lz = isliceobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->it);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
islice_traverse(PyObject *op, visitproc visit, void *arg)
{
    isliceobject *lz = isliceobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->it);
    return 0;
}

static _PyStatusValue
islice_next_value(PyObject *op)
{
    isliceobject *lz = isliceobject_CAST(op);
    PyObject *it = lz->it;
    Py_ssize_t stop = lz->stop;
    Py_ssize_t oldnext;

    if (it == NULL)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};

    while (lz->cnt < lz->next) {
        _PyStatusValue skipped = _PyValue_IterStep(it);
        if (skipped.status != PYGEN_NEXT) {
            Py_CLEAR(lz->it);
            return skipped;
        }
        PyValue_CLOSE(skipped.value);
        lz->cnt++;
    }
    if (stop != -1 && lz->cnt >= stop) {
        Py_CLEAR(lz->it);
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    _PyStatusValue item = _PyValue_IterStep(it);
    if (item.status != PYGEN_NEXT) {
        Py_CLEAR(lz->it);
        return item;
    }
    lz->cnt++;
    oldnext = lz->next;
    /* The (size_t) cast below avoids the danger of undefined
       behaviour from signed integer overflow. */
    lz->next += (size_t)lz->step;
    if (lz->next < oldnext || (stop != -1 && lz->next > stop))
        lz->next = stop;
    return item;
}

_Py_VALUE_THUNK_ITERNEXT(islice_next, islice_next_value)

PyDoc_STRVAR(islice_doc,
"islice(iterable, stop) --> islice object\n\
islice(iterable, start, stop[, step]) --> islice object\n\
\n\
Return an iterator whose next() method returns selected values from an\n\
iterable.  If start is specified, will skip all preceding elements;\n\
otherwise, start defaults to zero.  Step defaults to one.  If\n\
specified as another value, step determines how many values are\n\
skipped between successive calls.  Works like a slice() on a list\n\
but returns an iterator.");

static PyType_Slot islice_slots[] = {
    {Py_tp_dealloc, islice_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)islice_doc},
    {Py_tp_traverse, islice_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, islice_next},
    {_Py_value_iternext, islice_next_value},
    {Py_tp_new, islice_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec islice_spec = {
    .name = "itertools.islice",
    .basicsize = sizeof(isliceobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = islice_slots,
};


/* starmap object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *func;
    PyObject *it;
} starmapobject;

#define starmapobject_CAST(op)  ((starmapobject *)(op))

/*[clinic input]
@permit_long_summary
@classmethod
itertools.starmap.__new__
    function as func: object
    iterable as seq: object
    /
Return an iterator whose values are returned from the function evaluated with an argument tuple taken from the given sequence.
[clinic start generated code]*/

static PyValue
itertools_starmap_impl(PyTypeObject *type, PyValueBorrow func,
                       PyValueBorrow seq)
/*[clinic end generated code: output=1276b3f4a949de1e input=8c9068da0692d6d2]*/
{
    PyObject *it;
    starmapobject *lz;

    /* Get iterator. */
    it = _PyValue_GetIter(seq);
    if (it == NULL)
        return PyValue_NULL;

    /* create starmapobject structure */
    lz = (starmapobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }
    lz->it = it;
    lz->func = _PyValue_AsObjectAt(func, BOX_SITE_FIELD);
    if (lz->func == NULL) {
        Py_DECREF(lz);
        return PyValue_NULL;
    }

    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
starmap_dealloc(PyObject *op)
{
    starmapobject *lz = starmapobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->func);
    Py_XDECREF(lz->it);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
starmap_traverse(PyObject *op, visitproc visit, void *arg)
{
    starmapobject *lz = starmapobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->it);
    Py_VISIT(lz->func);
    return 0;
}

static _PyStatusValue
starmap_next_value(PyObject *op)
{
    starmapobject *lz = starmapobject_CAST(op);
    _PyStatusValue step = _PyValue_IterStep(lz->it);
    if (step.status != PYGEN_NEXT) {
        return step;
    }

    PyObject *args = _PyValue_SequenceTuple(step.value);
    PyValue_CLOSE(step.value);
    if (args == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    PyValue result = _PyValue_Call(_PyThreadState_GET(),
                                   PyValue_FromObjectBorrow(lz->func),
                                   args, NULL);
    Py_DECREF(args);
    if (PyValue_IsNull(result)) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    return (_PyStatusValue){PYGEN_NEXT, result};
}

_Py_VALUE_THUNK_ITERNEXT(starmap_next, starmap_next_value)

static PyType_Slot starmap_slots[] = {
    {Py_tp_dealloc, starmap_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_starmap__doc__},
    {Py_tp_traverse, starmap_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, starmap_next},
    {_Py_value_iternext, starmap_next_value},
    {Py_tp_new, itertools_starmap},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec starmap_spec = {
    .name = "itertools.starmap",
    .basicsize = sizeof(starmapobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = starmap_slots,
};


/* chain object **************************************************************/

struct chainobject {
    PyObject_HEAD
    PyObject *source;                   /* Iterator over input iterables */
    PyObject *active;                   /* Currently running input iterator */
};

#define chainobject_CAST(op)    ((chainobject *)(op))

static PyObject *
chain_new_internal(PyTypeObject *type, PyObject *source)
{
    chainobject *lz;

    lz = (chainobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(source);
        return NULL;
    }

    lz->source = source;
    lz->active = NULL;
    return (PyObject *)lz;
}

static PyObject *
chain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *source;

    itertools_state *state = find_state_by_type(type);
    PyTypeObject *chain_type = state->chain_type;
    if ((type == chain_type || type->tp_init == chain_type->tp_init) &&
        !_PyArg_NoKeywords("chain", kwds))
        return NULL;

    source = _PyValue_GetIter(PyValue_FromObjectBorrow(args));
    if (source == NULL)
        return NULL;

    return chain_new_internal(type, source);
}

/*[clinic input]
@permit_long_summary
@classmethod
itertools.chain.from_iterable
    iterable as arg: object
    /
Alternative chain() constructor taking a single iterable argument that evaluates lazily.
[clinic start generated code]*/

static PyValue
itertools_chain_from_iterable_impl(PyTypeObject *type, PyValueBorrow arg)
/*[clinic end generated code: output=ac9818d0cf63ae8e input=a9bf8227221c75b3]*/
{
    PyObject *source;

    source = _PyValue_GetIter(arg);
    if (source == NULL)
        return PyValue_NULL;

    return _PyValue_FromObjectResult(chain_new_internal(type, source));
}

static void
chain_dealloc(PyObject *op)
{
    chainobject *lz = chainobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->active);
    Py_XDECREF(lz->source);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
chain_traverse(PyObject *op, visitproc visit, void *arg)
{
    chainobject *lz = chainobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->source);
    Py_VISIT(lz->active);
    return 0;
}

static _PyStatusValue
chain_next_lock_held(chainobject *lz)
{
    while (lz->source != NULL) {
        if (lz->active == NULL) {
            _PyStatusValue source = _PyValue_IterNext(lz->source);
            if (source.status == PYGEN_ERROR) {
                Py_CLEAR(lz->source);
                return source;
            }
            if (source.status != PYGEN_NEXT) {
                PyValue_XCLOSE(source.value);
                Py_CLEAR(lz->source);
                return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
            }
            lz->active = _PyValue_GetIter(source.value);
            PyValue_CLOSE(source.value);
            if (lz->active == NULL) {
                Py_CLEAR(lz->source);
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
        }

        _PyStatusValue item = _PyValue_IterNext(lz->active);
        if (item.status == PYGEN_NEXT || item.status == PYGEN_ERROR) {
            return item;
        }
        PyValue_XCLOSE(item.value);
        Py_CLEAR(lz->active);
    }
    return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
}

static _PyStatusValue
chain_next_value(PyObject *op)
{
    _PyStatusValue result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = chain_next_lock_held(chainobject_CAST(op));
    Py_END_CRITICAL_SECTION();
    return result;
}
_Py_VALUE_THUNK_ITERNEXT(chain_next, chain_next_value)

PyDoc_STRVAR(chain_doc,
"chain(*iterables)\n\
--\n\
\n\
Return a chain object whose .__next__() method returns elements from the\n\
first iterable until it is exhausted, then elements from the next\n\
iterable, until all of the iterables are exhausted.");

PyDoc_STRVAR(chain_class_getitem_doc,
"chain is generic over the type of its contents.\n\
This is the union of the types of the input iterable contents.");

static PyMethodDef chain_methods[] = {
    ITERTOOLS_CHAIN_FROM_ITERABLE_METHODDEF
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       chain_class_getitem_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot chain_slots[] = {
    {Py_tp_dealloc, chain_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)chain_doc},
    {Py_tp_traverse, chain_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, chain_next},
    {_Py_value_iternext, chain_next_value},
    {Py_tp_methods, chain_methods},
    {Py_tp_new, chain_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec chain_spec = {
    .name = "itertools.chain",
    .basicsize = sizeof(chainobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = chain_slots,
};


/* product object ************************************************************/

struct productobject {
    PyObject_HEAD
    PyObject *pools;        /* tuple of pool tuples */
    Py_ssize_t *indices;    /* one index per pool */
    PyObject *result;       /* most recently returned result tuple */
    int stopped;            /* set to 1 when the iterator is exhausted */
};

#define productobject_CAST(op)  ((productobject *)(op))

static PyObject *
product_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    productobject *lz;
    Py_ssize_t nargs, npools, repeat=1;
    PyObject *pools = NULL;
    Py_ssize_t *indices = NULL;
    Py_ssize_t i;

    if (kwds != NULL) {
        char *kwlist[] = {"repeat", 0};
        PyObject *tmpargs = PyTuple_New(0);
        if (tmpargs == NULL)
            return NULL;
        if (!PyArg_ParseTupleAndKeywords(tmpargs, kwds, "|n:product",
                                         kwlist, &repeat)) {
            Py_DECREF(tmpargs);
            return NULL;
        }
        Py_DECREF(tmpargs);
        if (repeat < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "repeat argument cannot be negative");
            return NULL;
        }
    }

    assert(PyTuple_CheckExact(args));
    if (repeat == 0) {
        nargs = 0;
    } else {
        nargs = PyTuple_GET_SIZE(args);
        if ((size_t)nargs > PY_SSIZE_T_MAX/sizeof(Py_ssize_t)/repeat) {
            PyErr_SetString(PyExc_OverflowError, "repeat argument too large");
            return NULL;
        }
    }
    npools = nargs * repeat;

    indices = PyMem_New(Py_ssize_t, npools);
    if (indices == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    pools = PyTuple_New(npools);
    if (pools == NULL)
        goto error;

    for (i=0; i < nargs ; ++i) {
        PyValue item = PyTuple_GetValue(args, i);
        if (PyValue_IsNull(item))
            goto error;
        PyObject *pool = _PyValue_SequenceTuple(PyValue_Borrow(item));
        PyValue_CLOSE(item);
        if (pool == NULL)
            goto error;
        PyTuple_SET_VALUE(pools, i, PyValue_PrepareOwned(pool));
        indices[i] = 0;
    }
    for ( ; i < npools; ++i) {
        PyObject *pool = _PyTuple_GET_OBJECT(pools, i - nargs);
        Py_INCREF(pool);
        PyTuple_SET_VALUE(pools, i, PyValue_PrepareOwned(pool));
        indices[i] = 0;
    }

    /* create productobject structure */
    lz = (productobject *)type->tp_alloc(type, 0);
    if (lz == NULL)
        goto error;

    lz->pools = pools;
    lz->indices = indices;
    lz->result = NULL;
    lz->stopped = 0;

    return (PyObject *)lz;

error:
    if (indices != NULL)
        PyMem_Free(indices);
    Py_XDECREF(pools);
    return NULL;
}

static void
product_dealloc(PyObject *op)
{
    productobject *lz = productobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->pools);
    Py_XDECREF(lz->result);
    PyMem_Free(lz->indices);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static PyObject *
product_sizeof(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    productobject *lz = productobject_CAST(op);
    size_t res = _PyObject_SIZE(Py_TYPE(lz));
    res += (size_t)PyTuple_GET_SIZE(lz->pools) * sizeof(Py_ssize_t);
    return PyLong_FromSize_t(res);
}

PyDoc_STRVAR(sizeof_doc, "Returns size in memory, in bytes.");

static int
product_traverse(PyObject *op, visitproc visit, void *arg)
{
    productobject *lz = productobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->pools);
    Py_VISIT(lz->result);
    return 0;
}

/*[clinic input]
@critical_section
itertools.product.__next__ as product_next
[clinic start generated code]*/

static PyObject *
product_next_impl(productobject *self)
/*[clinic end generated code: output=d20b8571608c2043 input=54266fe7ed4a857a]*/
{
    productobject *lz = self;
    PyObject *pool;
    PyValue elem;
    PyValue oldelem;
    PyObject *pools = lz->pools;
    PyObject *result = lz->result;
    Py_ssize_t npools = PyTuple_GET_SIZE(pools);
    Py_ssize_t i;

    if (lz->stopped)
        return NULL;

    if (result == NULL) {
        /* On the first pass, return an initial tuple filled with the
           first element from each pool. */
        result = PyTuple_New(npools);
        if (result == NULL)
            goto empty;
        lz->result = result;
        for (i=0; i < npools; i++) {
            pool = _PyTuple_GET_OBJECT(pools, i);
            if (PyTuple_GET_SIZE(pool) == 0)
                goto empty;
            elem = PyValue_DUP(PyTuple_GET_VALUE(pool, 0));
            PyTuple_SET_VALUE(result, i, elem);
        }
    } else {
        Py_ssize_t *indices = lz->indices;

        /* Copy the previous result tuple or re-use it if available */
        if (!_PyObject_IsUniquelyReferenced(result)) {
            PyObject *old_result = result;
            result = _PyTuple_FromValueArray(_PyTuple_ITEMS(old_result), npools);
            if (result == NULL)
                goto empty;
            lz->result = result;
            Py_DECREF(old_result);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        else {
            _PyTuple_Recycle(result);
        }
        /* Now, we've got the only copy so we can update it in-place */
        assert (npools==0 || Py_REFCNT(result) == 1);

        /* Update the pool indices right-to-left.  Only advance to the
           next pool when the previous one rolls-over */
        for (i=npools-1 ; i >= 0 ; i--) {
            pool = _PyTuple_GET_OBJECT(pools, i);
            indices[i]++;
            if (indices[i] == PyTuple_GET_SIZE(pool)) {
                /* Roll-over and advance to next pool */
                indices[i] = 0;
                elem = PyValue_DUP(PyTuple_GET_VALUE(pool, 0));
                oldelem = _PyTuple_CAST(result)->ob_item[i];
                _PyTuple_CAST(result)->ob_item[i] = elem;
                PyValue_CLOSE(oldelem);
            } else {
                /* No rollover. Just increment and stop here. */
                elem = PyValue_DUP(PyTuple_GET_VALUE(pool, indices[i]));
                oldelem = _PyTuple_CAST(result)->ob_item[i];
                _PyTuple_CAST(result)->ob_item[i] = elem;
                PyValue_CLOSE(oldelem);
                break;
            }
        }

        /* If i is negative, then the indices have all rolled-over
           and we're done. */
        if (i < 0)
            goto empty;
    }

    return Py_NewRef(result);

empty:
    lz->stopped = 1;
    return NULL;
}

static PyMethodDef product_methods[] = {
    {"__sizeof__", product_sizeof, METH_NOARGS, sizeof_doc},
    {NULL,              NULL}   /* sentinel */
};

PyDoc_STRVAR(product_doc,
"product(*iterables, repeat=1)\n\
--\n\
\n\
Cartesian product of input iterables.  Equivalent to nested for-loops.\n\n\
For example, product(A, B) returns the same as:  ((x,y) for x in A for y in B).\n\
The leftmost iterators are in the outermost for-loop, so the output tuples\n\
cycle in a manner similar to an odometer (with the rightmost element changing\n\
on every iteration).\n\n\
To compute the product of an iterable with itself, specify the number\n\
of repetitions with the optional repeat keyword argument. For example,\n\
product(A, repeat=4) means the same as product(A, A, A, A).\n\n\
product('ab', range(3)) --> ('a',0) ('a',1) ('a',2) ('b',0) ('b',1) ('b',2)\n\
product((0,1), (0,1), (0,1)) --> (0,0,0) (0,0,1) (0,1,0) (0,1,1) (1,0,0) ...");

static PyType_Slot product_slots[] = {
    {Py_tp_dealloc, product_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)product_doc},
    {Py_tp_traverse, product_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, product_next},
    {Py_tp_methods, product_methods},
    {Py_tp_new, product_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec product_spec = {
    .name = "itertools.product",
    .basicsize = sizeof(productobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = product_slots,
};


/* combinations object *******************************************************/

struct combinationsobject {
    PyObject_HEAD
    PyObject *pool;         /* input converted to a tuple */
    Py_ssize_t *indices;    /* one index per result element */
    PyObject *result;       /* most recently returned result tuple */
    Py_ssize_t r;           /* size of result tuple */
    int stopped;            /* set to 1 when the iterator is exhausted */
};

#define combinationsobject_CAST(op) ((combinationsobject *)(op))

/*[clinic input]
@classmethod
itertools.combinations.__new__
    iterable: object
    r: Py_ssize_t(allow_negative=False)
Return successive r-length combinations of elements in the iterable.

combinations(range(4), 3) --> (0,1,2), (0,1,3), (0,2,3), (1,2,3)
[clinic start generated code]*/

static PyValue
itertools_combinations_impl(PyTypeObject *type, PyValueBorrow iterable,
                            Py_ssize_t r)
/*[clinic end generated code: output=cdbe4eca2dfb4cc9 input=a32f07a15cfa4676]*/
{
    combinationsobject *co;
    Py_ssize_t n;
    PyObject *pool = NULL;
    Py_ssize_t *indices = NULL;
    Py_ssize_t i;

    pool = _PyValue_SequenceTuple(iterable);
    if (pool == NULL)
        goto error;
    n = PyTuple_GET_SIZE(pool);

    indices = PyMem_New(Py_ssize_t, r);
    if (indices == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    for (i=0 ; i<r ; i++)
        indices[i] = i;

    /* create combinationsobject structure */
    co = (combinationsobject *)type->tp_alloc(type, 0);
    if (co == NULL)
        goto error;

    co->pool = pool;
    co->indices = indices;
    co->result = NULL;
    co->r = r;
    co->stopped = r > n ? 1 : 0;

    return _PyValue_FromObjectResult((PyObject *)co);

error:
    if (indices != NULL)
        PyMem_Free(indices);
    Py_XDECREF(pool);
    return PyValue_NULL;
}

static void
combinations_dealloc(PyObject *op)
{
    combinationsobject *co = combinationsobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(co);
    PyObject_GC_UnTrack(co);
    Py_XDECREF(co->pool);
    Py_XDECREF(co->result);
    PyMem_Free(co->indices);
    tp->tp_free(co);
    Py_DECREF(tp);
}

static PyObject *
combinations_sizeof(PyObject *op, PyObject *Py_UNUSED(args))
{
    combinationsobject *co = combinationsobject_CAST(op);
    size_t res = _PyObject_SIZE(Py_TYPE(co));
    res += (size_t)co->r * sizeof(Py_ssize_t);
    return PyLong_FromSize_t(res);
}

static int
combinations_traverse(PyObject *op, visitproc visit, void *arg)
{
    combinationsobject *co = combinationsobject_CAST(op);
    Py_VISIT(Py_TYPE(co));
    Py_VISIT(co->pool);
    Py_VISIT(co->result);
    return 0;
}

/*[clinic input]
@critical_section
itertools.combinations.__next__ as combinations_next
[clinic start generated code]*/

static PyObject *
combinations_next_impl(combinationsobject *self)
/*[clinic end generated code: output=19965c1a1f4b8f2a input=93000f6ec48380d7]*/
{
    combinationsobject *co = self;
    PyValue elem;
    PyValue oldelem;
    PyObject *pool = co->pool;
    Py_ssize_t *indices = co->indices;
    PyObject *result = co->result;
    Py_ssize_t n = PyTuple_GET_SIZE(pool);
    Py_ssize_t r = co->r;
    Py_ssize_t i, j, index;

    if (co->stopped)
        return NULL;

    if (result == NULL) {
        /* On the first pass, initialize result tuple using the indices */
        result = PyTuple_New(r);
        if (result == NULL)
            goto empty;
        co->result = result;
        for (i=0; i<r ; i++) {
            index = indices[i];
            elem = PyValue_DUP(PyTuple_GET_VALUE(pool, index));
            PyTuple_SET_VALUE(result, i, elem);
        }
    } else {
        /* Copy the previous result tuple or re-use it if available */
        if (!_PyObject_IsUniquelyReferenced(result)) {
            PyObject *old_result = result;
            result = _PyTuple_FromValueArray(_PyTuple_ITEMS(old_result), r);
            if (result == NULL)
                goto empty;
            co->result = result;
            Py_DECREF(old_result);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        else {
            _PyTuple_Recycle(result);
        }
        /* Now, we've got the only copy so we can update it in-place
         * CPython's empty tuple is a singleton and cached in
         * PyTuple's freelist.
         */
        assert(r == 0 || Py_REFCNT(result) == 1);

        /* Scan indices right-to-left until finding one that is not
           at its maximum (i + n - r). */
        for (i=r-1 ; i >= 0 && indices[i] == i+n-r ; i--)
            ;

        /* If i is negative, then the indices are all at
           their maximum value and we're done. */
        if (i < 0)
            goto empty;

        /* Increment the current index which we know is not at its
           maximum.  Then move back to the right setting each index
           to its lowest possible value (one higher than the index
           to its left -- this maintains the sort order invariant). */
        indices[i]++;
        for (j=i+1 ; j<r ; j++)
            indices[j] = indices[j-1] + 1;

        /* Update the result tuple for the new indices
           starting with i, the leftmost index that changed */
        for ( ; i<r ; i++) {
            index = indices[i];
            elem = PyValue_DUP(PyTuple_GET_VALUE(pool, index));
            oldelem = _PyTuple_CAST(result)->ob_item[i];
            _PyTuple_CAST(result)->ob_item[i] = elem;
            PyValue_CLOSE(oldelem);
        }
    }

    return Py_NewRef(result);

empty:
    co->stopped = 1;
    return NULL;
}

static PyMethodDef combinations_methods[] = {
    {"__sizeof__", combinations_sizeof, METH_NOARGS, sizeof_doc},
    {NULL,              NULL}   /* sentinel */
};

static PyType_Slot combinations_slots[] = {
    {Py_tp_dealloc, combinations_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_combinations__doc__},
    {Py_tp_traverse, combinations_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, combinations_next},
    {Py_tp_methods, combinations_methods},
    {Py_tp_new, itertools_combinations},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec combinations_spec = {
    .name = "itertools.combinations",
    .basicsize = sizeof(combinationsobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = combinations_slots,
};


/* combinations with replacement object **************************************/

/* Equivalent to:

        def combinations_with_replacement(iterable, r):
            "combinations_with_replacement('ABC', 2) --> AA AB AC BB BC CC"
            # number items returned:  (n+r-1)! / r! / (n-1)!
            pool = tuple(iterable)
            n = len(pool)
            indices = [0] * r
            yield tuple(pool[i] for i in indices)
            while 1:
                for i in reversed(range(r)):
                    if indices[i] != n - 1:
                        break
                else:
                    return
                indices[i:] = [indices[i] + 1] * (r - i)
                yield tuple(pool[i] for i in indices)

        def combinations_with_replacement2(iterable, r):
            'Alternate version that filters from product()'
            pool = tuple(iterable)
            n = len(pool)
            for indices in product(range(n), repeat=r):
                if sorted(indices) == list(indices):
                    yield tuple(pool[i] for i in indices)
*/
struct cwrobject {
    PyObject_HEAD
    PyObject *pool;         /* input converted to a tuple */
    Py_ssize_t *indices;    /* one index per result element */
    PyObject *result;       /* most recently returned result tuple */
    Py_ssize_t r;           /* size of result tuple */
    int stopped;            /* set to 1 when the cwr iterator is exhausted */
};

#define cwrobject_CAST(op)  ((cwrobject *)(op))

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
@classmethod
itertools.combinations_with_replacement.__new__
    iterable: object
    r: Py_ssize_t(allow_negative=False)
Return successive r-length combinations of elements in the iterable allowing individual elements to have successive repeats.

combinations_with_replacement('ABC', 2) --> ('A','A'), ('A','B'), ('A','C'), ('B','B'), ('B','C'), ('C','C')
[clinic start generated code]*/

static PyValue
itertools_combinations_with_replacement_impl(PyTypeObject *type,
                                             PyValueBorrow iterable,
                                             Py_ssize_t r)
/*[clinic end generated code: output=9b90c15cd54286fd input=828696750169e84f]*/
{
    cwrobject *co;
    Py_ssize_t n;
    PyObject *pool = NULL;
    Py_ssize_t *indices = NULL;
    Py_ssize_t i;

    pool = _PyValue_SequenceTuple(iterable);
    if (pool == NULL)
        goto error;
    n = PyTuple_GET_SIZE(pool);

    indices = PyMem_New(Py_ssize_t, r);
    if (indices == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    for (i=0 ; i<r ; i++)
        indices[i] = 0;

    /* create cwrobject structure */
    co = (cwrobject *)type->tp_alloc(type, 0);
    if (co == NULL)
        goto error;

    co->pool = pool;
    co->indices = indices;
    co->result = NULL;
    co->r = r;
    co->stopped = !n && r;

    return _PyValue_FromObjectResult((PyObject *)co);

error:
    if (indices != NULL)
        PyMem_Free(indices);
    Py_XDECREF(pool);
    return PyValue_NULL;
}

static void
cwr_dealloc(PyObject *op)
{
    cwrobject *co = cwrobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(co);
    PyObject_GC_UnTrack(co);
    Py_XDECREF(co->pool);
    Py_XDECREF(co->result);
    PyMem_Free(co->indices);
    tp->tp_free(co);
    Py_DECREF(tp);
}

static PyObject *
cwr_sizeof(PyObject *op, PyObject *Py_UNUSED(args))
{
    cwrobject *co = cwrobject_CAST(op);
    size_t res = _PyObject_SIZE(Py_TYPE(co));
    res += (size_t)co->r * sizeof(Py_ssize_t);
    return PyLong_FromSize_t(res);
}

static int
cwr_traverse(PyObject *op, visitproc visit, void *arg)
{
    cwrobject *co = cwrobject_CAST(op);
    Py_VISIT(Py_TYPE(co));
    Py_VISIT(co->pool);
    Py_VISIT(co->result);
    return 0;
}

/*[clinic input]
@critical_section
itertools.combinations_with_replacement.__next__ as cwr_next
[clinic start generated code]*/

static PyObject *
cwr_next_impl(cwrobject *self)
/*[clinic end generated code: output=760ddcfbf905ee37 input=d9ed46a28a5bb29a]*/
{
    cwrobject *co = self;
    PyValueBorrow elem;
    PyValue oldelem;
    PyObject *pool = co->pool;
    Py_ssize_t *indices = co->indices;
    PyObject *result = co->result;
    Py_ssize_t n = PyTuple_GET_SIZE(pool);
    Py_ssize_t r = co->r;
    Py_ssize_t i, index;

    if (co->stopped)
        return NULL;

    if (result == NULL) {
        /* On the first pass, initialize result tuple with pool[0] */
        result = PyTuple_New(r);
        if (result == NULL)
            goto empty;
        co->result = result;
        if (n > 0) {
            elem = PyTuple_GET_VALUE(pool, 0);
            for (i=0; i<r ; i++) {
                assert(indices[i] == 0);
                PyTuple_SET_VALUE(result, i, PyValue_DUP(elem));
            }
        }
    } else {
        /* Copy the previous result tuple or re-use it if available */
        if (!_PyObject_IsUniquelyReferenced(result)) {
            PyObject *old_result = result;
            result = _PyTuple_FromValueArray(_PyTuple_ITEMS(old_result), r);
            if (result == NULL)
                goto empty;
            co->result = result;
            Py_DECREF(old_result);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        else {
            _PyTuple_Recycle(result);
        }
        /* Now, we've got the only copy so we can update it in-place CPython's
           empty tuple is a singleton and cached in PyTuple's freelist. */
        assert(r == 0 || Py_REFCNT(result) == 1);

       /* Scan indices right-to-left until finding one that is not
        * at its maximum (n-1). */
        for (i=r-1 ; i >= 0 && indices[i] == n-1; i--)
            ;

        /* If i is negative, then the indices are all at
           their maximum value and we're done. */
        if (i < 0)
            goto empty;

        /* Increment the current index which we know is not at its
           maximum.  Then set all to the right to the same value. */
        index = indices[i] + 1;
        assert(index < n);
        elem = PyTuple_GET_VALUE(pool, index);
        for ( ; i<r ; i++) {
            indices[i] = index;
            oldelem = _PyTuple_CAST(result)->ob_item[i];
            _PyTuple_CAST(result)->ob_item[i] = PyValue_DUP(elem);
            PyValue_CLOSE(oldelem);
        }
    }

    return Py_NewRef(result);

empty:
    co->stopped = 1;
    return NULL;
}

static PyMethodDef cwr_methods[] = {
    {"__sizeof__", cwr_sizeof, METH_NOARGS, sizeof_doc},
    {NULL,              NULL}   /* sentinel */
};

static PyType_Slot cwr_slots[] = {
    {Py_tp_dealloc, cwr_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_combinations_with_replacement__doc__},
    {Py_tp_traverse, cwr_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, cwr_next},
    {Py_tp_methods, cwr_methods},
    {Py_tp_new, itertools_combinations_with_replacement},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec cwr_spec = {
    .name = "itertools.combinations_with_replacement",
    .basicsize = sizeof(cwrobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = cwr_slots,
};


/* permutations object ********************************************************

def permutations(iterable, r=None):
    # permutations('ABCD', 2) --> AB AC AD BA BC BD CA CB CD DA DB DC
    # permutations(range(3)) --> 012 021 102 120 201 210
    pool = tuple(iterable)
    n = len(pool)
    r = n if r is None else r
    if r > n:
        return
    indices = list(range(n))
    cycles = list(range(n, n-r, -1))
    yield tuple(pool[i] for i in indices[:r])
    while n:
        for i in reversed(range(r)):
            cycles[i] -= 1
            if cycles[i] == 0:
                indices[i:] = indices[i+1:] + indices[i:i+1]
                cycles[i] = n - i
            else:
                j = cycles[i]
                indices[i], indices[-j] = indices[-j], indices[i]
                yield tuple(pool[i] for i in indices[:r])
                break
        else:
            return
*/

struct permutationsobject {
    PyObject_HEAD
    PyObject *pool;         /* input converted to a tuple */
    Py_ssize_t *indices;    /* one index per element in the pool */
    Py_ssize_t *cycles;     /* one rollover counter per element in the result */
    PyObject *result;       /* most recently returned result tuple */
    Py_ssize_t r;           /* size of result tuple */
    int stopped;            /* set to 1 when the iterator is exhausted */
};

#define permutationsobject_CAST(op) ((permutationsobject *)(op))

/* An inline int or float is an int or float: PyLong_Check() on a value. */
static int
value_is_long(PyValueBorrow v)
{
    return PyType_IsSubtype(PyValue_TYPE(v), &PyLong_Type);
}

/* PyNumber_Check() on a value; an inline int or float is a number. */
static int
value_is_number(PyValueBorrow v)
{
    return !PyValue_IsObject(v) || PyNumber_Check(PyValue_AsKnownObjectBorrow(v));
}

/*[clinic input]
@classmethod
itertools.permutations.__new__
    iterable: object
    r as robj: object = None
Return successive r-length permutations of elements in the iterable.

permutations(range(3), 2) --> (0,1), (0,2), (1,0), (1,2), (2,0), (2,1)
[clinic start generated code]*/

static PyValue
itertools_permutations_impl(PyTypeObject *type, PyValueBorrow iterable,
                            PyValueBorrow robj)
/*[clinic end generated code: output=3304a138ec86c42a input=57d0170a4ac0ec7a]*/
{
    permutationsobject *po;
    Py_ssize_t n;
    Py_ssize_t r;
    PyObject *pool = NULL;
    Py_ssize_t *indices = NULL;
    Py_ssize_t *cycles = NULL;
    Py_ssize_t i;

    pool = _PyValue_SequenceTuple(iterable);
    if (pool == NULL)
        goto error;
    n = PyTuple_GET_SIZE(pool);

    r = n;
    if (!PyValue_IsNone(robj)) {
        if (!value_is_long(robj)) {
            PyErr_SetString(PyExc_TypeError, "Expected int as r");
            goto error;
        }
        r = _PyValue_IntAsSsize_t(robj);
        if (r == -1 && PyErr_Occurred())
            goto error;
    }
    if (r < 0) {
        PyErr_SetString(PyExc_ValueError, "r must be non-negative");
        goto error;
    }

    indices = PyMem_New(Py_ssize_t, n);
    cycles = PyMem_New(Py_ssize_t, r);
    if (indices == NULL || cycles == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    for (i=0 ; i<n ; i++)
        indices[i] = i;
    for (i=0 ; i<r ; i++)
        cycles[i] = n - i;

    /* create permutationsobject structure */
    po = (permutationsobject *)type->tp_alloc(type, 0);
    if (po == NULL)
        goto error;

    po->pool = pool;
    po->indices = indices;
    po->cycles = cycles;
    po->result = NULL;
    po->r = r;
    po->stopped = r > n ? 1 : 0;

    return _PyValue_FromObjectResult((PyObject *)po);

error:
    if (indices != NULL)
        PyMem_Free(indices);
    if (cycles != NULL)
        PyMem_Free(cycles);
    Py_XDECREF(pool);
    return PyValue_NULL;
}

static void
permutations_dealloc(PyObject *op)
{
    permutationsobject *po = permutationsobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(po);
    PyObject_GC_UnTrack(po);
    Py_XDECREF(po->pool);
    Py_XDECREF(po->result);
    PyMem_Free(po->indices);
    PyMem_Free(po->cycles);
    tp->tp_free(po);
    Py_DECREF(tp);
}

static PyObject *
permutations_sizeof(PyObject *op, PyObject *Py_UNUSED(args))
{
    permutationsobject *po = permutationsobject_CAST(op);
    size_t res = _PyObject_SIZE(Py_TYPE(po));
    res += (size_t)PyTuple_GET_SIZE(po->pool) * sizeof(Py_ssize_t);
    res += (size_t)po->r * sizeof(Py_ssize_t);
    return PyLong_FromSize_t(res);
}

static int
permutations_traverse(PyObject *op, visitproc visit, void *arg)
{
    permutationsobject *po = permutationsobject_CAST(op);
    Py_VISIT(Py_TYPE(po));
    Py_VISIT(po->pool);
    Py_VISIT(po->result);
    return 0;
}

/*[clinic input]
@critical_section
itertools.permutations.__next__ as permutations_next
[clinic start generated code]*/

static PyObject *
permutations_next_impl(permutationsobject *self)
/*[clinic end generated code: output=17d8db4465bd3ee5 input=d4c1a1eed76a35e0]*/
{
    permutationsobject *po = self;
    PyValue elem;
    PyValue oldelem;
    PyObject *pool = po->pool;
    Py_ssize_t *indices = po->indices;
    Py_ssize_t *cycles = po->cycles;
    PyObject *result = po->result;
    Py_ssize_t n = PyTuple_GET_SIZE(pool);
    Py_ssize_t r = po->r;
    Py_ssize_t i, j, k, index;

    if (po->stopped)
        return NULL;

    if (result == NULL) {
        /* On the first pass, initialize result tuple using the indices */
        result = PyTuple_New(r);
        if (result == NULL)
            goto empty;
        po->result = result;
        for (i=0; i<r ; i++) {
            index = indices[i];
            elem = PyValue_DUP(PyTuple_GET_VALUE(pool, index));
            PyTuple_SET_VALUE(result, i, elem);
        }
    } else {
        if (n == 0)
            goto empty;

        /* Copy the previous result tuple or re-use it if available */
        if (!_PyObject_IsUniquelyReferenced(result)) {
            PyObject *old_result = result;
            result = _PyTuple_FromValueArray(_PyTuple_ITEMS(old_result), r);
            if (result == NULL)
                goto empty;
            po->result = result;
            Py_DECREF(old_result);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        else {
            _PyTuple_Recycle(result);
        }
        /* Now, we've got the only copy so we can update it in-place */
        assert(r == 0 || Py_REFCNT(result) == 1);

        /* Decrement rightmost cycle, moving leftward upon zero rollover */
        for (i=r-1 ; i>=0 ; i--) {
            cycles[i] -= 1;
            if (cycles[i] == 0) {
                /* rotatation: indices[i:] = indices[i+1:] + indices[i:i+1] */
                index = indices[i];
                for (j=i ; j<n-1 ; j++)
                    indices[j] = indices[j+1];
                indices[n-1] = index;
                cycles[i] = n - i;
            } else {
                j = cycles[i];
                index = indices[i];
                indices[i] = indices[n-j];
                indices[n-j] = index;

                for (k=i; k<r ; k++) {
                    /* start with i, the leftmost element that changed */
                    /* yield tuple(pool[k] for k in indices[:r]) */
                    index = indices[k];
                    elem = PyValue_DUP(PyTuple_GET_VALUE(pool, index));
                    oldelem = _PyTuple_CAST(result)->ob_item[k];
                    _PyTuple_CAST(result)->ob_item[k] = elem;
                    PyValue_CLOSE(oldelem);
                }
                break;
            }
        }
        /* If i is negative, then the cycles have all
           rolled-over and we're done. */
        if (i < 0)
            goto empty;
    }
    return Py_NewRef(result);

empty:
    po->stopped = 1;
    return NULL;
}

static PyMethodDef permuations_methods[] = {
    {"__sizeof__", permutations_sizeof, METH_NOARGS, sizeof_doc},
    {NULL,              NULL}   /* sentinel */
};

static PyType_Slot permutations_slots[] = {
    {Py_tp_dealloc, permutations_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_permutations__doc__},
    {Py_tp_traverse, permutations_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, permutations_next},
    {Py_tp_methods, permuations_methods},
    {Py_tp_new, itertools_permutations},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec permutations_spec = {
    .name = "itertools.permutations",
    .basicsize = sizeof(permutationsobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = permutations_slots,
};


/* accumulate object ********************************************************/

struct accumulateobject {
    PyObject_HEAD
    PyValue total;      /* optional value home */
    PyObject *it;
    PyObject *binop;
    PyValue initial;    /* optional value home; unset when not given */
    itertools_state *state;
};

#define accumulateobject_CAST(op)   ((accumulateobject *)(op))

/*[clinic input]
@classmethod
itertools.accumulate.__new__
    iterable: object
    func as binop: object = None
    *
    initial: object = None
Return series of accumulated sums (or other binary function results).
[clinic start generated code]*/

static PyValue
itertools_accumulate_impl(PyTypeObject *type, PyValueBorrow iterable,
                          PyValueBorrow binop, PyValueBorrow initial)
/*[clinic end generated code: output=00d4c4113673c5bb input=c4ce20ac59bf7ffd]*/
{
    PyObject *it;
    accumulateobject *lz;

    /* Get iterator. */
    it = _PyValue_GetIter(iterable);
    if (it == NULL)
        return PyValue_NULL;

    /* create accumulateobject structure */
    lz = (accumulateobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }

    lz->total = PyValue_NULL;
    lz->it = it;
    if (!PyValue_IsNone(binop)) {
        lz->binop = _PyValue_AsObjectAt(binop, BOX_SITE_FIELD);
        if (lz->binop == NULL) {
            Py_DECREF(lz);
            return PyValue_NULL;
        }
    }
    if (!PyValue_IsNone(initial)) {
        lz->initial = PyValue_DUP(initial);
        _PyValue_CheckHome(lz->initial);
    }
    lz->state = find_state_by_type(type);
    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
accumulate_dealloc(PyObject *op)
{
    accumulateobject *lz = accumulateobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->binop);
    PyValue_XCLOSE(lz->total);
    Py_XDECREF(lz->it);
    PyValue_XCLOSE(lz->initial);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
accumulate_traverse(PyObject *op, visitproc visit, void *arg)
{
    accumulateobject *lz = accumulateobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->binop);
    Py_VISIT(lz->it);
    _Py_VISIT_VALUE(lz->total);
    _Py_VISIT_VALUE(lz->initial);
    return 0;
}

static _PyStatusValue
accumulate_next_lock_held(accumulateobject *lz)
{
    if (!PyValue_IsNull(lz->initial)) {
        PyValue_XCLOSE(lz->total);
        lz->total = lz->initial;
        lz->initial = PyValue_NULL;
        return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(lz->total)};
    }
    _PyStatusValue step = _PyValue_IterStep(lz->it);
    if (step.status != PYGEN_NEXT) {
        return step;
    }
    PyValue val = step.value;

    if (PyValue_IsNull(lz->total)) {
        lz->total = val;
        _PyValue_CheckHome(lz->total);
        return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(lz->total)};
    }

    /* The operation can run Python that reassigns lz->total. */
    PyValue total = PyValue_DUP(lz->total);
    PyValue newtotal;
    if (lz->binop == NULL) {
        newtotal = _PyValue_Add(total, val);
    }
    else {
        PyValue args[2] = {total, val};
        newtotal = _PyValue_Vectorcall(PyValue_FromObjectBorrow(lz->binop),
                                       args, 2, NULL);
    }
    PyValue_CLOSE(total);
    PyValue_CLOSE(val);
    if (PyValue_IsNull(newtotal)) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }

    PyValue oldtotal = lz->total;
    lz->total = PyValue_DUP(newtotal);
    _PyValue_CheckHome(lz->total);
    PyValue_XCLOSE(oldtotal);
    return (_PyStatusValue){PYGEN_NEXT, newtotal};
}

/*[clinic input]
@critical_section
itertools.accumulate.__next__ as accumulate_next
[clinic start generated code]*/

static PyObject *
accumulate_next_impl(accumulateobject *self)
/*[clinic end generated code: output=30d3f9b40248d8a6 input=758a144b7cfb1ef4]*/
{
    _PyStatusValue r = accumulate_next_lock_held(self);
    if (r.status != PYGEN_NEXT) {
        return NULL;
    }
    return _PyValue_AsObjectStealAt(r.value, BOX_SITE_ITERATION);
}

static _PyStatusValue
accumulate_next_value(PyObject *op)
{
    _PyStatusValue r;
    Py_BEGIN_CRITICAL_SECTION(op);
    r = accumulate_next_lock_held(accumulateobject_CAST(op));
    Py_END_CRITICAL_SECTION();
    return r;
}

static PyType_Slot accumulate_slots[] = {
    {Py_tp_dealloc, accumulate_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_accumulate__doc__},
    {Py_tp_traverse, accumulate_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, accumulate_next},
    {_Py_value_iternext, accumulate_next_value},
    {Py_tp_new, itertools_accumulate},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec accumulate_spec = {
    .name = "itertools.accumulate",
    .basicsize = sizeof(accumulateobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = accumulate_slots,
};


/* compress object ************************************************************/

/* Equivalent to:

    def compress(data, selectors):
        "compress('ABCDEF', [1,0,1,0,1,1]) --> A C E F"
        return (d for d, s in zip(data, selectors) if s)
*/

typedef struct {
    PyObject_HEAD
    PyObject *data;
    PyObject *selectors;
} compressobject;

#define compressobject_CAST(op) ((compressobject *)(op))

/*[clinic input]
@classmethod
itertools.compress.__new__
    data as seq1: object
    selectors as seq2: object
Return data elements corresponding to true selector elements.

Forms a shorter iterator from selected data elements using the selectors
to choose the data elements.
[clinic start generated code]*/

static PyValue
itertools_compress_impl(PyTypeObject *type, PyValueBorrow seq1,
                        PyValueBorrow seq2)
/*[clinic end generated code: output=57acfa9c20754d14 input=32ca4347dbc46749]*/
{
    PyObject *data=NULL, *selectors=NULL;
    compressobject *lz;

    data = _PyValue_GetIter(seq1);
    if (data == NULL)
        goto fail;
    selectors = _PyValue_GetIter(seq2);
    if (selectors == NULL)
        goto fail;

    /* create compressobject structure */
    lz = (compressobject *)type->tp_alloc(type, 0);
    if (lz == NULL)
        goto fail;
    lz->data = data;
    lz->selectors = selectors;
    return _PyValue_FromObjectResult((PyObject *)lz);

fail:
    Py_XDECREF(data);
    Py_XDECREF(selectors);
    return PyValue_NULL;
}

static void
compress_dealloc(PyObject *op)
{
    compressobject *lz = compressobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->data);
    Py_XDECREF(lz->selectors);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
compress_traverse(PyObject *op, visitproc visit, void *arg)
{
    compressobject *lz = compressobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->data);
    Py_VISIT(lz->selectors);
    return 0;
}

static _PyStatusValue
compress_next_value(PyObject *op)
{
    compressobject *lz = compressobject_CAST(op);
    PyObject *data = lz->data, *selectors = lz->selectors;

    while (1) {
        /* Steps:  get datum, get selector, evaluate selector.
           Order is important (to match the pure python version
           in terms of which input gets a chance to raise an
           exception first).
        */

        _PyStatusValue datum = _PyValue_IterStep(data);
        if (datum.status != PYGEN_NEXT)
            return datum;

        _PyStatusValue selector = _PyValue_IterStep(selectors);
        if (selector.status != PYGEN_NEXT) {
            PyValue_CLOSE(datum.value);
            return selector;
        }

        int ok = _PyValue_IsTrue(selector.value);
        PyValue_CLOSE(selector.value);
        if (ok > 0)
            return datum;
        PyValue_CLOSE(datum.value);
        if (ok < 0)
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
}

_Py_VALUE_THUNK_ITERNEXT(compress_next, compress_next_value)

static PyType_Slot compress_slots[] = {
    {Py_tp_dealloc, compress_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_compress__doc__},
    {Py_tp_traverse, compress_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, compress_next},
    {_Py_value_iternext, compress_next_value},
    {Py_tp_new, itertools_compress},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec compress_spec = {
    .name = "itertools.compress",
    .basicsize = sizeof(compressobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = compress_slots,
};


/* filterfalse object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyValue func;
    PyObject *it;
} filterfalseobject;

#define filterfalseobject_CAST(op)  ((filterfalseobject *)(op))

/*[clinic input]
@classmethod
itertools.filterfalse.__new__
    function as func: object
    iterable as seq: object
    /
Return those items of iterable for which function(item) is false.

If function is None, return the items that are false.
[clinic start generated code]*/

static PyValue
itertools_filterfalse_impl(PyTypeObject *type, PyValueBorrow func,
                           PyValueBorrow seq)
/*[clinic end generated code: output=76280d19eca73e55 input=2d684a2c66f99cde]*/
{
    PyObject *it;
    filterfalseobject *lz;

    /* Get iterator. */
    it = _PyValue_GetIter(seq);
    if (it == NULL)
        return PyValue_NULL;

    /* create filterfalseobject structure */
    lz = (filterfalseobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return PyValue_NULL;
    }
    lz->it = it;
    lz->func = PyValue_DUP(func);
    _PyValue_CheckHome(lz->func);

    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
filterfalse_dealloc(PyObject *op)
{
    filterfalseobject *lz = filterfalseobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    PyValue_CLOSE(lz->func);
    Py_XDECREF(lz->it);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
filterfalse_traverse(PyObject *op, visitproc visit, void *arg)
{
    filterfalseobject *lz = filterfalseobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->it);
    _Py_VISIT_VALUE(lz->func);
    return 0;
}

static _PyStatusValue
filterfalse_next_value(PyObject *op)
{
    filterfalseobject *lz = filterfalseobject_CAST(op);
    PyObject *it = lz->it;

    for (;;) {
        _PyStatusValue item = _PyValue_IterStep(it);
        if (item.status != PYGEN_NEXT)
            return item;

        int ok;
        if (PyValue_Is(lz->func, PyValue_None) ||
            (PyValue_IsObject(lz->func) &&
             PyValue_AsKnownObjectBorrow(lz->func) == (PyObject *)&PyBool_Type)) {
            ok = _PyValue_IsTrue(item.value);
        } else {
            PyValue good = _PyValue_CallOneArg(_PyThreadState_GET(), lz->func,
                                               item.value);
            if (PyValue_IsNull(good)) {
                PyValue_CLOSE(item.value);
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
            ok = _PyValue_IsTrue(good);
            PyValue_CLOSE(good);
        }
        if (ok == 0)
            return item;
        PyValue_CLOSE(item.value);
        if (ok < 0)
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
}

_Py_VALUE_THUNK_ITERNEXT(filterfalse_next, filterfalse_next_value)

static PyType_Slot filterfalse_slots[] = {
    {Py_tp_dealloc, filterfalse_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_filterfalse__doc__},
    {Py_tp_traverse, filterfalse_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, filterfalse_next},
    {_Py_value_iternext, filterfalse_next_value},
    {Py_tp_new, itertools_filterfalse},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec filterfalse_spec = {
    .name = "itertools.filterfalse",
    .basicsize = sizeof(filterfalseobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = filterfalse_slots,
};


/* count object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyValue cnt;        /* required value home: the next value to return */
    PyValue step;       /* required value home, fixed at construction */
} countobject;

#define countobject_CAST(op)    ((countobject *)(op))

/* Counting logic and invariants:

The counter is one value: an inline int while it fits, a heap int after,
or whatever number the caller gave (float, Fraction, Decimal, an int
subclass).  Advances with:  cnt += step, by _PyValue_Add(), so there is no
overflow or underflow.  Step may be zero -- effectively a slow version of
repeat(cnt).

As upstream's fast mode did, an integer start that fits Py_ssize_t with an
integer step equal to 1 (or no step) is normalized to an exact int, so the
first value returned is not an int subclass such as True.
*/

/*[clinic input]
@permit_long_summary
@classmethod
itertools.count.__new__
    start as long_cnt: object(c_default="NULL") = 0
    step as long_step: object(c_default="NULL") = 1
Return a count object whose .__next__() method returns consecutive values.

Equivalent to:
    def count(firstval=0, step=1):
        x = firstval
        while 1:
            yield x
            x += step
[clinic start generated code]*/

static PyValue
itertools_count_impl(PyTypeObject *type, PyValueBorrow long_cnt,
                     PyValueBorrow long_step)
/*[clinic end generated code: output=5f203ac605ad72fd input=91e4b12c0e88b9f4]*/
{
    countobject *lz;
    int fast_mode;
    Py_ssize_t cnt = 0;
    long step;

    if ((!PyValue_IsNull(long_cnt) && !value_is_number(long_cnt)) ||
        (!PyValue_IsNull(long_step) && !value_is_number(long_step))) {
                    PyErr_SetString(PyExc_TypeError, "a number is required");
                    return PyValue_NULL;
    }

    fast_mode = (PyValue_IsNull(long_cnt) || value_is_long(long_cnt)) &&
                (PyValue_IsNull(long_step) || value_is_long(long_step));

    if (fast_mode && !PyValue_IsNull(long_cnt)) {
        cnt = _PyValue_IntAsSsize_t(long_cnt);
        if (cnt == -1 && PyErr_Occurred()) {
            PyErr_Clear();
            fast_mode = 0;
        }
    }
    if (fast_mode && !PyValue_IsNull(long_step)) {
        step = _PyValue_AsLong(long_step);
        if (step != 1) {
            fast_mode = 0;
            if (step == -1 && PyErr_Occurred())
                PyErr_Clear();
        }
    }

    PyValue cnt_value;
    if (fast_mode) {
        cnt_value = PyValue_FromInt64(cnt);
        if (PyValue_IsNull(cnt_value)) {
            return PyValue_NULL;
        }
    }
    else if (PyValue_IsNull(long_cnt)) {
        /* If not specified, start defaults to 0 */
        cnt_value = PyValue_FromInlineInt(0);
    }
    else {
        cnt_value = PyValue_DUP(long_cnt);
    }

    /* If not specified, step defaults to 1 */
    PyValue step_value = PyValue_IsNull(long_step)
        ? PyValue_FromInlineInt(1)
        : PyValue_DUP(long_step);

    /* create countobject structure */
    lz = (countobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        PyValue_CLOSE(cnt_value);
        PyValue_CLOSE(step_value);
        return PyValue_NULL;
    }
    lz->cnt = cnt_value;
    lz->step = step_value;
    _PyValue_CheckHome(lz->cnt);
    _PyValue_CheckHome(lz->step);

    return _PyValue_FromObjectResult((PyObject *)lz);
}

static void
count_dealloc(PyObject *op)
{
    countobject *lz = countobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    PyValue_CLOSE(lz->cnt);
    PyValue_CLOSE(lz->step);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
count_traverse(PyObject *op, visitproc visit, void *arg)
{
    countobject *lz = countobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    _Py_VISIT_VALUE(lz->cnt);
    _Py_VISIT_VALUE(lz->step);
    return 0;
}

static _PyStatusValue
count_next_lock_held(countobject *lz)
{
    /* The addition can run Python that re-enters this counter: return our
       own reference to the old value and replace the home afterwards. */
    PyValue result = PyValue_DUP(lz->cnt);
    PyValue stepped_up = _PyValue_Add(result, lz->step);
    if (PyValue_IsNull(stepped_up)) {
        PyValue_CLOSE(result);
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    PyValue old = lz->cnt;
    lz->cnt = stepped_up;
    _PyValue_CheckHome(lz->cnt);
    PyValue_CLOSE(old);
    return (_PyStatusValue){PYGEN_NEXT, result};
}

static _PyStatusValue
count_next_value(PyObject *op)
{
    countobject *lz = countobject_CAST(op);
    _PyStatusValue r;
    Py_BEGIN_CRITICAL_SECTION(lz);
    r = count_next_lock_held(lz);
    Py_END_CRITICAL_SECTION();
    return r;
}

_Py_VALUE_THUNK_ITERNEXT(count_next, count_next_value)

/* Is step an integer equal to 1?  The repr omits it then. */
static int
count_step_is_one(PyValueBorrow step)
{
    if (PyValue_IsInlineInt(step)) {
        return PyValue_InlineInt64(step) == 1;
    }
    if (!PyValue_LongCheck(step)) {
        return 0;
    }
    long v = _PyValue_AsLong(step);
    if (v == -1 && PyErr_Occurred()) {
        PyErr_Clear();
    }
    return v == 1;
}

static PyObject *
count_repr(PyObject *op)
{
    countobject *lz = countobject_CAST(op);
    PyValue cnt;
    Py_BEGIN_CRITICAL_SECTION(lz);
    cnt = PyValue_DUP(lz->cnt);
    Py_END_CRITICAL_SECTION();

    PyObject *cnt_repr = _PyValue_Repr(cnt);
    PyValue_CLOSE(cnt);
    if (cnt_repr == NULL) {
        return NULL;
    }

    PyObject *result;
    if (count_step_is_one(PyValue_Borrow(lz->step))) {
        /* Don't display step when it is an integer equal to 1 */
        result = PyUnicode_FromFormat("%s(%U)", _PyType_Name(Py_TYPE(lz)),
                                      cnt_repr);
    }
    else {
        PyObject *step_repr = _PyValue_Repr(lz->step);
        if (step_repr == NULL) {
            Py_DECREF(cnt_repr);
            return NULL;
        }
        result = PyUnicode_FromFormat("%s(%U, %U)", _PyType_Name(Py_TYPE(lz)),
                                      cnt_repr, step_repr);
        Py_DECREF(step_repr);
    }
    Py_DECREF(cnt_repr);
    return result;
}

static PyType_Slot count_slots[] = {
    {Py_tp_dealloc, count_dealloc},
    {Py_tp_repr, count_repr},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)itertools_count__doc__},
    {Py_tp_traverse, count_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, count_next},
    {_Py_value_iternext, count_next_value},
    {Py_tp_new, itertools_count},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec count_spec = {
    .name = "itertools.count",
    .basicsize = sizeof(countobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = count_slots,
};


/* repeat object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyValue element;    /* required value home */
    Py_ssize_t cnt;
} repeatobject;

#define repeatobject_CAST(op)   ((repeatobject *)(op))

static PyObject *
repeat_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    repeatobject *ro;
    PyObject *element;
    Py_ssize_t cnt = -1, n_args;
    static char *kwargs[] = {"object", "times", NULL};

    n_args = PyTuple_GET_SIZE(args);
    if (kwds != NULL)
        n_args += PyDict_GET_SIZE(kwds);
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|n:repeat", kwargs,
                                     &element, &cnt))
        return NULL;
    /* Does user supply times argument? */
    if (n_args == 2 && cnt < 0)
        cnt = 0;

    ro = (repeatobject *)type->tp_alloc(type, 0);
    if (ro == NULL)
        return NULL;
    ro->element = PyValue_DUP(PyValue_FromObjectBorrow(element));
    _PyValue_CheckHome(ro->element);
    ro->cnt = cnt;
    return (PyObject *)ro;
}

static void
repeat_dealloc(PyObject *op)
{
    repeatobject *ro = repeatobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(ro);
    PyObject_GC_UnTrack(ro);
    PyValue_CLOSE(ro->element);
    tp->tp_free(ro);
    Py_DECREF(tp);
}

static int
repeat_traverse(PyObject *op, visitproc visit, void *arg)
{
    repeatobject *ro = repeatobject_CAST(op);
    Py_VISIT(Py_TYPE(ro));
    _Py_VISIT_VALUE(ro->element);
    return 0;
}

static _PyStatusValue
repeat_next_value(PyObject *op)
{
    repeatobject *ro = repeatobject_CAST(op);
    Py_ssize_t cnt = FT_ATOMIC_LOAD_SSIZE_RELAXED(ro->cnt);
    if (cnt == 0) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    if (cnt > 0) {
        cnt--;
        FT_ATOMIC_STORE_SSIZE_RELAXED(ro->cnt, cnt);
    }
    return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(ro->element)};
}

_Py_VALUE_THUNK_ITERNEXT(repeat_next, repeat_next_value)

static PyObject *
repeat_repr(PyObject *op)
{
    repeatobject *ro = repeatobject_CAST(op);
    PyObject *element_repr = _PyValue_Repr(ro->element);
    if (element_repr == NULL) {
        return NULL;
    }
    PyObject *result;
    if (ro->cnt == -1)
        result = PyUnicode_FromFormat("%s(%U)",
                                      _PyType_Name(Py_TYPE(ro)), element_repr);
    else
        result = PyUnicode_FromFormat("%s(%U, %zd)",
                                      _PyType_Name(Py_TYPE(ro)), element_repr,
                                      ro->cnt);
    Py_DECREF(element_repr);
    return result;
}

static PyObject *
repeat_len(PyObject *op, PyObject *Py_UNUSED(args))
{
    repeatobject *ro = repeatobject_CAST(op);
    if (ro->cnt == -1) {
        PyErr_SetString(PyExc_TypeError, "len() of unsized object");
        return NULL;
    }
    return PyLong_FromSize_t(ro->cnt);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef repeat_methods[] = {
    {"__length_hint__", repeat_len, METH_NOARGS, length_hint_doc},
    {NULL,              NULL}           /* sentinel */
};

PyDoc_STRVAR(repeat_doc,
"repeat(object [,times]) -> create an iterator which returns the object\n\
for the specified number of times.  If not specified, returns the object\n\
endlessly.");

static PyType_Slot repeat_slots[] = {
    {Py_tp_dealloc, repeat_dealloc},
    {Py_tp_repr, repeat_repr},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)repeat_doc},
    {Py_tp_traverse, repeat_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, repeat_next},
    {_Py_value_iternext, repeat_next_value},
    {Py_tp_methods, repeat_methods},
    {Py_tp_new, repeat_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec repeat_spec = {
    .name = "itertools.repeat",
    .basicsize = sizeof(repeatobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = repeat_slots,
};


/* ziplongest object *********************************************************/

struct ziplongestobject {
    PyObject_HEAD
    Py_ssize_t tuplesize;
    Py_ssize_t numactive;
    PyObject *ittuple;                  /* tuple of iterators */
    PyObject *result;
    PyValue fillvalue;                  /* required value home */
};

#define ziplongestobject_CAST(op)   ((ziplongestobject *)(op))

/* The iterator at index i of ittuple, borrowed, or NULL once exhausted. */
static inline PyObject *
ittuple_get_iterator(PyObject *ittuple, Py_ssize_t i)
{
    PyValueBorrow it = PyTuple_GET_VALUE(ittuple, i);
    return PyValue_IsNull(it) ? NULL : PyValue_AsKnownObjectBorrow(it);
}

static PyObject *
zip_longest_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ziplongestobject *lz;
    Py_ssize_t i;
    PyObject *ittuple;  /* tuple of iterators */
    PyObject *result;
    Py_ssize_t tuplesize;
    PyValue fill = PyValue_NULL;

    if (kwds != NULL && PyDict_CheckExact(kwds) && PyDict_GET_SIZE(kwds) > 0) {
        if (PyDict_GET_SIZE(kwds) == 1) {
            if (PyDict_GetValue(kwds,
                                PyValue_FromObjectBorrow(&_Py_ID(fillvalue)),
                                &fill) < 0) {
                return NULL;
            }
        }
        if (PyValue_IsNull(fill)) {
            PyErr_SetString(PyExc_TypeError,
                "zip_longest() got an unexpected keyword argument");
            return NULL;
        }
    }
    else {
        fill = PyValue_None;
    }

    /* args must be a tuple */
    assert(PyTuple_Check(args));
    tuplesize = PyTuple_GET_SIZE(args);

    /* obtain iterators */
    ittuple = PyTuple_New(tuplesize);
    if (ittuple == NULL) {
        PyValue_CLOSE(fill);
        return NULL;
    }
    for (i=0; i < tuplesize; i++) {
        PyValue item = PyTuple_GetValue(args, i);
        if (PyValue_IsNull(item)) {
            Py_DECREF(ittuple);
            PyValue_CLOSE(fill);
            return NULL;
        }
        PyObject *it = _PyValue_GetIter(PyValue_Borrow(item));
        PyValue_CLOSE(item);
        if (it == NULL) {
            Py_DECREF(ittuple);
            PyValue_CLOSE(fill);
            return NULL;
        }
        PyTuple_SET_VALUE(ittuple, i, PyValue_PrepareOwned(it));
    }

    /* create a result holder */
    result = PyTuple_New(tuplesize);
    if (result == NULL) {
        Py_DECREF(ittuple);
        PyValue_CLOSE(fill);
        return NULL;
    }
    for (i=0 ; i < tuplesize ; i++) {
        Py_INCREF(Py_None);
        PyTuple_SET_VALUE(result, i, PyValue_PrepareOwned(_PyObject_CAST(Py_None)));
    }

    /* create ziplongestobject structure */
    lz = (ziplongestobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(ittuple);
        Py_DECREF(result);
        PyValue_CLOSE(fill);
        return NULL;
    }
    lz->ittuple = ittuple;
    lz->tuplesize = tuplesize;
    lz->numactive = tuplesize;
    lz->result = result;
    lz->fillvalue = fill;
    _PyValue_CheckHome(lz->fillvalue);
    return (PyObject *)lz;
}

static void
zip_longest_dealloc(PyObject *op)
{
    ziplongestobject *lz = ziplongestobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(lz);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->ittuple);
    Py_XDECREF(lz->result);
    PyValue_XCLOSE(lz->fillvalue);
    tp->tp_free(lz);
    Py_DECREF(tp);
}

static int
zip_longest_traverse(PyObject *op, visitproc visit, void *arg)
{
    ziplongestobject *lz = ziplongestobject_CAST(op);
    Py_VISIT(Py_TYPE(lz));
    Py_VISIT(lz->ittuple);
    Py_VISIT(lz->result);
    _Py_VISIT_VALUE(lz->fillvalue);
    return 0;
}

/* The next item of iterator i, or the fillvalue once it is exhausted. The
   status is PYGEN_RETURN when every iterator is now exhausted, and
   PYGEN_ERROR with an exception set on failure. */
static _PyStatusValue
zip_longest_item(ziplongestobject *lz, Py_ssize_t i)
{
    PyObject *it = ittuple_get_iterator(lz->ittuple, i);
    if (it == NULL) {
        return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(lz->fillvalue)};
    }
    _PyStatusValue step = _PyValue_IterNext(it);
    if (step.status == PYGEN_NEXT) {
        return step;
    }
    lz->numactive -= 1;
    if (lz->numactive == 0 || step.status == PYGEN_ERROR) {
        lz->numactive = 0;
        return step;
    }
    _PyTuple_CAST(lz->ittuple)->ob_item[i] = PyValue_NULL;
    Py_DECREF(it);
    return (_PyStatusValue){PYGEN_NEXT, PyValue_DUP(lz->fillvalue)};
}

static _PyStatusValue
zip_longest_next_lock_held(ziplongestobject *lz)
{
    Py_ssize_t i;
    Py_ssize_t tuplesize = lz->tuplesize;
    PyObject *result = lz->result;
    PyValue olditem;
    _PyStatusValue item;

    if (tuplesize == 0)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    if (lz->numactive == 0)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    if (_PyObject_IsUniquelyReferenced(result)) {
        Py_INCREF(result);
        for (i=0 ; i < tuplesize ; i++) {
            item = zip_longest_item(lz, i);
            if (item.status != PYGEN_NEXT) {
                Py_DECREF(result);
                return item;
            }
            olditem = _PyTuple_CAST(result)->ob_item[i];
            _PyTuple_CAST(result)->ob_item[i] = item.value;
            _PyValue_CheckHome(_PyTuple_CAST(result)->ob_item[i]);
            PyValue_CLOSE(olditem);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        _PyTuple_Recycle(result);
    } else {
        result = PyTuple_New(tuplesize);
        if (result == NULL)
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        for (i=0 ; i < tuplesize ; i++) {
            item = zip_longest_item(lz, i);
            if (item.status != PYGEN_NEXT) {
                Py_DECREF(result);
                return item;
            }
            PyTuple_SET_VALUE(result, i, item.value);
        }
    }
    return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(result)};
}

/*[clinic input]
@critical_section
itertools.zip_longest.__next__ as zip_longest_next
[clinic start generated code]*/

static PyObject *
zip_longest_next_impl(ziplongestobject *self)
/*[clinic end generated code: output=11bc756cf896c576 input=5b1e3b52098852a9]*/
{
    _PyStatusValue r = zip_longest_next_lock_held(self);
    if (r.status != PYGEN_NEXT) {
        return NULL;
    }
    return _PyValue_AsObjectStealAt(r.value, BOX_SITE_ITERATION);
}

static _PyStatusValue
zip_longest_next_value(PyObject *op)
{
    _PyStatusValue r;
    Py_BEGIN_CRITICAL_SECTION(op);
    r = zip_longest_next_lock_held(ziplongestobject_CAST(op));
    Py_END_CRITICAL_SECTION();
    return r;
}

PyDoc_STRVAR(zip_longest_doc,
"zip_longest(*iterables, fillvalue=None)\n\
--\n\
\n\
Return a zip_longest object whose .__next__() method returns a tuple where\n\
the i-th element comes from the i-th iterable argument.  The .__next__()\n\
method continues until the longest iterable in the argument sequence\n\
is exhausted and then it raises StopIteration.  When the shorter iterables\n\
are exhausted, the fillvalue is substituted in their place.  The fillvalue\n\
defaults to None or can be specified by a keyword argument.\n\
");

static PyType_Slot ziplongest_slots[] = {
    {Py_tp_dealloc, zip_longest_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)zip_longest_doc},
    {Py_tp_traverse, zip_longest_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, zip_longest_next},
    {_Py_value_iternext, zip_longest_next_value},
    {Py_tp_new, zip_longest_new},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

static PyType_Spec ziplongest_spec = {
    .name = "itertools.zip_longest",
    .basicsize = sizeof(ziplongestobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = ziplongest_slots,
};


/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"Functional tools for creating and using iterators.\n\
\n\
Infinite iterators:\n\
count(start=0, step=1) --> start, start+step, start+2*step, ...\n\
cycle(p) --> p0, p1, ... plast, p0, p1, ...\n\
repeat(elem [,n]) --> elem, elem, elem, ... endlessly or up to n times\n\
\n\
Iterators terminating on the shortest input sequence:\n\
accumulate(p[, func]) --> p0, p0+p1, p0+p1+p2\n\
batched(p, n) --> [p0, p1, ..., p_n-1], [p_n, p_n+1, ..., p_2n-1], ...\n\
chain(p, q, ...) --> p0, p1, ... plast, q0, q1, ...\n\
chain.from_iterable([p, q, ...]) --> p0, p1, ... plast, q0, q1, ...\n\
compress(data, selectors) --> (d[0] if s[0]), (d[1] if s[1]), ...\n\
dropwhile(predicate, seq) --> seq[n], seq[n+1], starting when predicate fails\n\
groupby(iterable[, keyfunc]) --> sub-iterators grouped by value of keyfunc(v)\n\
filterfalse(predicate, seq) --> elements of seq where predicate(elem) is False\n\
islice(seq, [start,] stop [, step]) --> elements from\n\
       seq[start:stop:step]\n\
pairwise(s) --> (s[0],s[1]), (s[1],s[2]), (s[2], s[3]), ...\n\
starmap(fun, seq) --> fun(*seq[0]), fun(*seq[1]), ...\n\
tee(it, n=2) --> (it1, it2 , ... itn) splits one iterator into n\n\
takewhile(predicate, seq) --> seq[0], seq[1], until predicate fails\n\
zip_longest(p, q, ...) --> (p[0], q[0]), (p[1], q[1]), ...\n\
\n\
Combinatoric generators:\n\
product(p, q, ... [repeat=1]) --> cartesian product\n\
permutations(p[, r])\n\
combinations(p, r)\n\
combinations_with_replacement(p, r)\n\
");

static int
itertoolsmodule_traverse(PyObject *mod, visitproc visit, void *arg)
{
    itertools_state *state = get_module_state(mod);
    Py_VISIT(state->accumulate_type);
    Py_VISIT(state->batched_type);
    Py_VISIT(state->chain_type);
    Py_VISIT(state->combinations_type);
    Py_VISIT(state->compress_type);
    Py_VISIT(state->count_type);
    Py_VISIT(state->cwr_type);
    Py_VISIT(state->cycle_type);
    Py_VISIT(state->dropwhile_type);
    Py_VISIT(state->filterfalse_type);
    Py_VISIT(state->groupby_type);
    Py_VISIT(state->_grouper_type);
    Py_VISIT(state->islice_type);
    Py_VISIT(state->pairwise_type);
    Py_VISIT(state->permutations_type);
    Py_VISIT(state->product_type);
    Py_VISIT(state->repeat_type);
    Py_VISIT(state->starmap_type);
    Py_VISIT(state->takewhile_type);
    Py_VISIT(state->tee_type);
    Py_VISIT(state->teedataobject_type);
    Py_VISIT(state->ziplongest_type);
    return 0;
}

static int
itertoolsmodule_clear(PyObject *mod)
{
    itertools_state *state = get_module_state(mod);
    Py_CLEAR(state->accumulate_type);
    Py_CLEAR(state->batched_type);
    Py_CLEAR(state->chain_type);
    Py_CLEAR(state->combinations_type);
    Py_CLEAR(state->compress_type);
    Py_CLEAR(state->count_type);
    Py_CLEAR(state->cwr_type);
    Py_CLEAR(state->cycle_type);
    Py_CLEAR(state->dropwhile_type);
    Py_CLEAR(state->filterfalse_type);
    Py_CLEAR(state->groupby_type);
    Py_CLEAR(state->_grouper_type);
    Py_CLEAR(state->islice_type);
    Py_CLEAR(state->pairwise_type);
    Py_CLEAR(state->permutations_type);
    Py_CLEAR(state->product_type);
    Py_CLEAR(state->repeat_type);
    Py_CLEAR(state->starmap_type);
    Py_CLEAR(state->takewhile_type);
    Py_CLEAR(state->tee_type);
    Py_CLEAR(state->teedataobject_type);
    Py_CLEAR(state->ziplongest_type);
    return 0;
}

static void
itertoolsmodule_free(void *mod)
{
    (void)itertoolsmodule_clear((PyObject *)mod);
}

#define ADD_TYPE(module, type, spec)                                     \
do {                                                                     \
    type = (PyTypeObject *)PyType_FromModuleAndSpec(module, spec, NULL); \
    if (type == NULL) {                                                  \
        return -1;                                                       \
    }                                                                    \
    if (PyModule_AddType(module, type) < 0) {                            \
        return -1;                                                       \
    }                                                                    \
} while (0)

static int
itertoolsmodule_exec(PyObject *mod)
{
    itertools_state *state = get_module_state(mod);
    ADD_TYPE(mod, state->accumulate_type, &accumulate_spec);
    ADD_TYPE(mod, state->batched_type, &batched_spec);
    ADD_TYPE(mod, state->chain_type, &chain_spec);
    ADD_TYPE(mod, state->combinations_type, &combinations_spec);
    ADD_TYPE(mod, state->compress_type, &compress_spec);
    ADD_TYPE(mod, state->count_type, &count_spec);
    ADD_TYPE(mod, state->cwr_type, &cwr_spec);
    ADD_TYPE(mod, state->cycle_type, &cycle_spec);
    ADD_TYPE(mod, state->dropwhile_type, &dropwhile_spec);
    ADD_TYPE(mod, state->filterfalse_type, &filterfalse_spec);
    ADD_TYPE(mod, state->groupby_type, &groupby_spec);
    ADD_TYPE(mod, state->_grouper_type, &_grouper_spec);
    ADD_TYPE(mod, state->islice_type, &islice_spec);
    ADD_TYPE(mod, state->pairwise_type, &pairwise_spec);
    ADD_TYPE(mod, state->permutations_type, &permutations_spec);
    ADD_TYPE(mod, state->product_type, &product_spec);
    ADD_TYPE(mod, state->repeat_type, &repeat_spec);
    ADD_TYPE(mod, state->starmap_type, &starmap_spec);
    ADD_TYPE(mod, state->takewhile_type, &takewhile_spec);
    ADD_TYPE(mod, state->tee_type, &tee_spec);
    ADD_TYPE(mod, state->teedataobject_type, &teedataobject_spec);
    ADD_TYPE(mod, state->ziplongest_type, &ziplongest_spec);

    Py_SET_TYPE(state->teedataobject_type, &PyType_Type);
    return 0;
}

static struct PyModuleDef_Slot itertoolsmodule_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, itertoolsmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static PyMethodDef module_methods[] = {
    ITERTOOLS_TEE_METHODDEF
    {NULL, NULL} /* sentinel */
};


static struct PyModuleDef itertoolsmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "itertools",
    .m_doc = module_doc,
    .m_size = sizeof(itertools_state),
    .m_methods = module_methods,
    .m_slots = itertoolsmodule_slots,
    .m_traverse = itertoolsmodule_traverse,
    .m_clear = itertoolsmodule_clear,
    .m_free = itertoolsmodule_free,
};

PyMODINIT_FUNC
PyInit_itertools(void)
{
    return PyModuleDef_Init(&itertoolsmodule);
}
