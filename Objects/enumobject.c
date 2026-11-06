/* enumerate object */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_long.h"          // _PyLong_GetOne()
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString
#include "pycore_tuple.h"         // _PyTuple_Recycle()
#include "pycore_value.h"         // PyValue_XCLOSE()
#include "pycore_value_call.h"     // _PyValue_VectorcallThunk()
#include "pycore_value_ops.h"     // _PyValue_IterStep()
#include "pycore_value_slots.h"   // _Py_VALUE_THUNK_ITERNEXT()

#include "clinic/enumobject.c.h"

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class enumerate "enumobject *" "&PyEnum_Type"
class reversed "reversedobject *" "&PyReversed_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d2dfdf1a88c88975]*/

typedef struct {
    PyObject_HEAD
    Py_ssize_t en_index;           /* current index of enumeration */
    PyObject* en_sit;              /* secondary iterator of enumeration */
    PyObject* en_result;           /* result tuple  */
    PyObject* en_longindex;        /* index for sequences >= PY_SSIZE_T_MAX */
    PyObject* one;                 /* borrowed reference */
} enumobject;

#define _enumobject_CAST(op)    ((enumobject *)(op))

/*[clinic input]
@vectorcall
@classmethod
enumerate.__new__ as enum_new

    iterable: object
        an object supporting iteration
    start: object = 0

Return an enumerate object.

The enumerate object yields pairs containing a count (from start, which
defaults to zero) and a value yielded by the iterable argument.

enumerate is useful for obtaining an indexed list:
    (0, seq[0]), (1, seq[1]), (2, seq[2]), ...
[clinic start generated code]*/

static PyValue
enum_new_impl(PyTypeObject *type, PyValueBorrow iterable,
              PyValueBorrow start)
/*[clinic end generated code: output=602355ab76a1c8e3 input=a139e88889360e8f]*/
{
    enumobject *en;

    en = (enumobject *)type->tp_alloc(type, 0);
    if (en == NULL)
        return PyValue_NULL;
    if (!PyValue_IsNull(start)) {
        PyValue index = _PyValue_IndexExact(start);
        if (PyValue_IsNull(index)) {
            Py_DECREF(en);
            return PyValue_NULL;
        }
        en->en_index = _PyValue_AsSsize_t(index, PyExc_OverflowError);
        if (en->en_index == -1 && PyErr_Occurred()) {
            PyErr_Clear();
            en->en_index = PY_SSIZE_T_MAX;
            /* Only a heap int overflows Py_ssize_t, so this does not box. */
            en->en_longindex = _PyValue_AsObjectAt(index, BOX_SITE_PARSER);
            PyValue_CLOSE(index);
            if (en->en_longindex == NULL) {
                Py_DECREF(en);
                return PyValue_NULL;
            }
        } else {
            en->en_longindex = NULL;
            PyValue_CLOSE(index);
        }
    } else {
        en->en_index = 0;
        en->en_longindex = NULL;
    }
    en->en_sit = _PyValue_GetIter(iterable);
    if (en->en_sit == NULL) {
        Py_DECREF(en);
        return PyValue_NULL;
    }
    en->en_result = _PyTuple_FromPairTakeValue(PyValue_PrepareOwned(Py_None), PyValue_PrepareOwned(Py_None));
    if (en->en_result == NULL) {
        Py_DECREF(en);
        return PyValue_NULL;
    }
    en->one = _PyLong_GetOne();    /* borrowed reference */
    return _PyValue_FromObjectResult((PyObject *)en);
}

static void
enum_dealloc(PyObject *op)
{
    enumobject *en = _enumobject_CAST(op);
    PyObject_GC_UnTrack(en);
    Py_XDECREF(en->en_sit);
    Py_XDECREF(en->en_result);
    Py_XDECREF(en->en_longindex);
    Py_TYPE(en)->tp_free(en);
}

static int
enum_traverse(PyObject *op, visitproc visit, void *arg)
{
    enumobject *en = _enumobject_CAST(op);
    Py_VISIT(en->en_sit);
    Py_VISIT(en->en_result);
    Py_VISIT(en->en_longindex);
    return 0;
}

// increment en_longindex with lock held, return the next index to be used
// or NULL on error
static inline PyObject *
increment_longindex_lock_held(enumobject *en)
{
    if (en->en_longindex == NULL) {
        en->en_longindex = PyLong_FromSsize_t(PY_SSIZE_T_MAX);
        if (en->en_longindex == NULL) {
            return NULL;
        }
    }
    assert(en->en_longindex != NULL);
    // We hold one reference to "next_index" (a.k.a. the old value of
    // en->en_longindex); we'll either return it or keep it in en->en_longindex
    PyObject *next_index = en->en_longindex;
    PyValue stepped_up_v = _PyValue_Add(PyValue_FromObjectBorrow(next_index),
                                        PyValue_FromObjectBorrow(en->one));
    PyObject *stepped_up = PyValue_IsNull(stepped_up_v) ? NULL
        : _PyValue_AsObjectStealAt(stepped_up_v, BOX_SITE_FIELD);
    if (stepped_up == NULL) {
        return NULL;
    }
    en->en_longindex = stepped_up;
    return next_index;
}

// The (index, item) pair of a step past PY_SSIZE_T_MAX, taking next_item.
static _PyStatusValue
enum_next_long(enumobject *en, PyValue next_item)
{
    PyObject *iterator_owner = Py_NewRef(en);
    PyObject *result = en->en_result;
    PyObject *next_index = NULL;
    PyObject *output = NULL;

    Py_BEGIN_CRITICAL_SECTION(en);
    next_index = increment_longindex_lock_held(en);
    Py_END_CRITICAL_SECTION();
    if (next_index == NULL) {
        goto done;
    }

    if (_PyObject_IsUniquelyReferenced(result)) {
        output = Py_NewRef(result);
        PyValue replacement_index = PyValue_PrepareOwned(next_index);
        next_index = NULL;
        PyValue replacement_item = next_item;
        next_item = PyValue_NULL;
        PyValue old_index = _PyTuple_CAST(result)->ob_item[0];
        // a move: replaced below
        PyValue old_item = _PyTuple_CAST(result)->ob_item[1];
        _PyTuple_CAST(result)->ob_item[0] = replacement_index;
        _PyTuple_CAST(result)->ob_item[1] = replacement_item;
        // Reset the hash and GC tracking before retiring displaced owners.
        _PyTuple_Recycle(result);
        PyValue_XCLOSE(old_index);
        PyValue_XCLOSE(old_item);
    }
    else {
        PyValue pair[2] = {PyValue_PrepareOwned(next_index), next_item};
        next_index = NULL;
        next_item = PyValue_NULL;
        output = _PyTuple_FromValueStealOnSuccess(pair, 2);
        if (output == NULL) {
            PyValue_CLOSE(pair[0]);
            PyValue_CLOSE(pair[1]);
        }
    }

done:
    Py_CLEAR(next_index);
    PyValue_XCLOSE(next_item);
    Py_CLEAR(iterator_owner);
    if (output == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(output)};
}

/* The value iternext slot of enumerate (R16): the inner iterator's end, with
   its return value, or its error is enumerate's. The pair holds the index
   and the item as words. */
static _PyStatusValue
enum_next_value(PyObject *op)
{
    enumobject *en = _enumobject_CAST(op);
    PyObject *iterator_owner = Py_NewRef(en);
    PyObject *result = en->en_result;
    PyValue next_index = PyValue_NULL;
    PyValue next_item = PyValue_NULL;
    _PyStatusValue output = {PYGEN_ERROR, PyValue_NULL};

    PyObject *it = en->en_sit;
    _PyStatusValue step = _PyValue_IterStep(it);
    if (step.status != PYGEN_NEXT) {
        output = step;
        goto done;
    }
    next_item = step.value;

    Py_ssize_t en_index = FT_ATOMIC_LOAD_SSIZE_RELAXED(en->en_index);
    if (en_index == PY_SSIZE_T_MAX) {
        PyValue transferred = next_item;
        next_item = PyValue_NULL;
        output = enum_next_long(en, transferred);
        goto done;
    }

    next_index = PyValue_FromInt64(en_index);
    if (PyValue_IsNull(next_index)) {
        goto done;
    }
    FT_ATOMIC_STORE_SSIZE_RELAXED(en->en_index, en_index + 1);

    if (_PyObject_IsUniquelyReferenced(result)) {
        PyObject *pair = Py_NewRef(result);
        PyValue replacement_index = next_index;
        next_index = PyValue_NULL;
        PyValue replacement_item = next_item;
        next_item = PyValue_NULL;
        PyValue old_index = _PyTuple_CAST(result)->ob_item[0];
        // a move: replaced below
        PyValue old_item = _PyTuple_CAST(result)->ob_item[1];
        _PyTuple_CAST(result)->ob_item[0] = replacement_index;
        _PyTuple_CAST(result)->ob_item[1] = replacement_item;
        // Reset the hash and GC tracking before retiring displaced owners.
        _PyTuple_Recycle(result);
        PyValue_XCLOSE(old_index);
        PyValue_XCLOSE(old_item);
        output = (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(pair)};
    }
    else {
        PyValue items[2] = {next_index, next_item};
        PyObject *pair = _PyTuple_FromValueStealOnSuccess(items, 2);
        if (pair == NULL) {
            goto done;
        }
        next_index = PyValue_NULL;
        next_item = PyValue_NULL;
        output = (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(pair)};
    }

done:
    PyValue_XCLOSE(next_index);
    PyValue_XCLOSE(next_item);
    Py_CLEAR(iterator_owner);
    return output;
}

_Py_VALUE_THUNK_ITERNEXT(enum_next, enum_next_value)

static PyObject *
enum_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    enumobject *en = _enumobject_CAST(op);
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(en);
    if (en->en_longindex != NULL) {
        result = Py_BuildValue("O(OO)", Py_TYPE(en), en->en_sit, en->en_longindex);
    }
    else {
        Py_ssize_t en_index = FT_ATOMIC_LOAD_SSIZE_RELAXED(en->en_index);
        result = Py_BuildValue("O(On)", Py_TYPE(en), en->en_sit, en_index);
    }
    Py_END_CRITICAL_SECTION();
    return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef enum_methods[] = {
    {"__reduce__", enum_reduce, METH_NOARGS, reduce_doc},
    {"__class_getitem__",    Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("'enumerate' objects are generic over the type of their values")},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyEnum_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "enumerate",                    /* tp_name */
    sizeof(enumobject),             /* tp_basicsize */
    0,                              /* tp_itemsize */
    /* methods */
    enum_dealloc,                   /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,        /* tp_flags */
    enum_new__doc__,                /* tp_doc */
    enum_traverse,                  /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    PyObject_SelfIter,              /* tp_iter */
    enum_next,                      /* tp_iternext */
    enum_methods,                   /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    PyType_GenericAlloc,            /* tp_alloc */
    enum_new,                       /* tp_new */
    PyObject_GC_Del,                /* tp_free */
    .tp_vectorcall = _PyValue_VectorcallThunk,
    .tp_value_vectorcall = (_Py_valuefunc)enum_vectorcall,
    .tp_value = { .iternext = _Py_VALUE_SLOT_INIT(iternext, enum_next_value) },
};

/* Reversed Object ***************************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t      index;
    PyObject* seq;
} reversedobject;

#define _reversedobject_CAST(op)    ((reversedobject *)(op))

/*[clinic input]
@vectorcall
@classmethod
reversed.__new__ as reversed_new

    object as seq: object
    /

Return a reverse iterator over the values of the given sequence.
[clinic start generated code]*/

static PyValue
reversed_new_impl(PyTypeObject *type, PyValueBorrow seq)
/*[clinic end generated code: output=50e7515996fd4a70 input=7db568182ab28c59]*/
{
    Py_ssize_t n;
    reversedobject *ro;

    _PyStatusValue lookup = _PyValue_LookupSpecial(seq, &_Py_ID(__reversed__));
    if (lookup.status < 0) {
        return PyValue_NULL;
    }
    if (lookup.status > 0) {
        if (PyValue_IsNone(lookup.value)) {
            PyValue_CLOSE(lookup.value);
            PyErr_Format(PyExc_TypeError,
                         "'%.200s' object is not reversible",
                         PyValue_TYPE(seq)->tp_name);
            return PyValue_NULL;
        }
        PyValue res = _PyValue_CallNoArgs(_PyThreadState_GET(), lookup.value);
        PyValue_CLOSE(lookup.value);
        return res;
    }

    if (!(PyValue_IsObject(seq) && _PyValue_SequenceCheck(seq))) {
        PyErr_Format(PyExc_TypeError,
                     "'%.200s' object is not reversible",
                     PyValue_TYPE(seq)->tp_name);
        return PyValue_NULL;
    }

    n = _PyValue_Size(seq);
    if (n == -1)
        return PyValue_NULL;

    ro = (reversedobject *)type->tp_alloc(type, 0);
    if (ro == NULL)
        return PyValue_NULL;

    ro->index = n-1;
    ro->seq = Py_NewRef(PyValue_AsKnownObjectBorrow(seq));  /* a sequence is an object */
    return _PyValue_FromObjectResult((PyObject *)ro);
}

static void
reversed_dealloc(PyObject *op)
{
    reversedobject *ro = _reversedobject_CAST(op);
    PyObject_GC_UnTrack(ro);
    Py_XDECREF(ro->seq);
    Py_TYPE(ro)->tp_free(ro);
}

static int
reversed_traverse(PyObject *op, visitproc visit, void *arg)
{
    reversedobject *ro = _reversedobject_CAST(op);
    Py_VISIT(ro->seq);
    return 0;
}

static _PyStatusValue
reversed_next_value(PyObject *op)
{
    reversedobject *ro = _reversedobject_CAST(op);
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(ro->index);

    if (index >= 0) {
        PyValue item = _PyValue_SequenceGetItem(_Py_VALUE_ARG(ro->seq), index);
        if (!PyValue_IsNull(item)) {
            FT_ATOMIC_STORE_SSIZE_RELAXED(ro->index, index - 1);
            return (_PyStatusValue){PYGEN_NEXT, item};
        }
        if (PyErr_ExceptionMatches(PyExc_IndexError) ||
            PyErr_ExceptionMatches(PyExc_StopIteration))
            PyErr_Clear();
    }
    FT_ATOMIC_STORE_SSIZE_RELAXED(ro->index, -1);
#ifndef Py_GIL_DISABLED
    Py_CLEAR(ro->seq);
#endif
    /* Any error other than the end of the sequence propagates. */
    return (_PyStatusValue){PyErr_Occurred() ? PYGEN_ERROR : PYGEN_RETURN,
                            PyValue_NULL};
}

_Py_VALUE_THUNK_ITERNEXT(reversed_next, reversed_next_value)

static PyObject *
reversed_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    reversedobject *ro = _reversedobject_CAST(op);
    Py_ssize_t position, seqsize;
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(ro->index);

    if (index == -1)
        return PyLong_FromLong(0);
    assert(ro->seq != NULL);
    seqsize = _PyValue_SequenceSize(PyValue_FromObjectBorrow(ro->seq));
    if (seqsize == -1)
        return NULL;
    position = index + 1;
    return PyLong_FromSsize_t((seqsize < position)  ?  0  :  position);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
reversed_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    reversedobject *ro = _reversedobject_CAST(op);
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(ro->index);
    if (index != -1) {
        return Py_BuildValue("O(O)n", Py_TYPE(ro), ro->seq, ro->index);
    }
    else {
        return Py_BuildValue("O(())", Py_TYPE(ro));
    }
}

static PyObject *
reversed_setstate(PyObject *op, PyObject *state)
{
    reversedobject *ro = _reversedobject_CAST(op);
    Py_ssize_t index = _PyValue_IntAsSsize_t(PyValue_FromObjectBorrow(state));
    if (index == -1 && PyErr_Occurred())
        return NULL;
    Py_ssize_t ro_index = FT_ATOMIC_LOAD_SSIZE_RELAXED(ro->index);
    // if the iterator is exhausted we do not set the state
    // this is for backwards compatibility reasons. in practice this situation
    // will not occur, see gh-120971
    if (ro_index != -1) {
        Py_ssize_t n = _PyValue_SequenceSize(PyValue_FromObjectBorrow(ro->seq));
        if (n < 0)
            return NULL;
        if (index < -1)
            index = -1;
        else if (index > n-1)
            index = n-1;
        FT_ATOMIC_STORE_SSIZE_RELAXED(ro->index, index);
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef reversediter_methods[] = {
    {"__length_hint__", reversed_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", reversed_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", reversed_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyReversed_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "reversed",                     /* tp_name */
    sizeof(reversedobject),         /* tp_basicsize */
    0,                              /* tp_itemsize */
    /* methods */
    reversed_dealloc,               /* tp_dealloc */
    0,                              /* tp_vectorcall_offset */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_as_async */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,        /* tp_flags */
    reversed_new__doc__,            /* tp_doc */
    reversed_traverse,              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    PyObject_SelfIter,              /* tp_iter */
    reversed_next,                  /* tp_iternext */
    reversediter_methods,           /* tp_methods */
    0,                              /* tp_members */
    0,                              /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    0,                              /* tp_init */
    PyType_GenericAlloc,            /* tp_alloc */
    reversed_new,                   /* tp_new */
    PyObject_GC_Del,                /* tp_free */
    .tp_vectorcall = _PyValue_VectorcallThunk,
    .tp_value_vectorcall = (_Py_valuefunc)reversed_vectorcall,
    .tp_value = { .iternext = _Py_VALUE_SLOT_INIT(iternext, reversed_next_value) },
};
