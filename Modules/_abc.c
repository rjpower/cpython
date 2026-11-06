/* ABCMeta implementation */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_dict.h"           // PyDict_PopValue()
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyType_GetSubclasses()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_setobject.h"     // _PySet_NextEntryRef()
#include "pycore_value.h"         // PyValue_CLOSE()
#include "pycore_value_ops.h"     // _PyValue_GetIter()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()
#include "pycore_weakref.h"       // _PyWeakref_GET_REF()
#include "clinic/_abc.c.h"

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _abc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=964f5328e1aefcda]*/

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");

typedef struct {
    PyTypeObject *_abc_data_type;
    uint64_t abc_invalidation_counter;
} _abcmodule_state;

static inline _abcmodule_state*
get_abc_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (_abcmodule_state *)state;
}

static inline uint64_t
get_invalidation_counter(_abcmodule_state *state)
{
#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_uint64(&state->abc_invalidation_counter);
#else
    return state->abc_invalidation_counter;
#endif
}

static inline void
increment_invalidation_counter(_abcmodule_state *state)
{
#ifdef Py_GIL_DISABLED
    _Py_atomic_add_uint64(&state->abc_invalidation_counter, 1);
#else
    state->abc_invalidation_counter++;
#endif
}

/* This object stores internal state for ABCs.
   Note that we can use normal sets for caches,
   since they are never iterated over. */
typedef struct {
    PyObject_HEAD
    /* These sets of weak references are lazily created. Once created, they
       will point to the same sets until the ABCMeta object is destroyed or
       cleared, both of which will only happen while the object is visible to a
       single thread. */
    PyObject *_abc_registry;
    PyObject *_abc_cache;
    PyObject *_abc_negative_cache;
    uint64_t _abc_negative_cache_version;
} _abc_data;

#define _abc_data_CAST(op)  ((_abc_data *)(op))

static inline uint64_t
get_cache_version(_abc_data *impl)
{
#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_uint64(&impl->_abc_negative_cache_version);
#else
    return impl->_abc_negative_cache_version;
#endif
}

static inline void
set_cache_version(_abc_data *impl, uint64_t version)
{
#ifdef Py_GIL_DISABLED
    _Py_atomic_store_uint64(&impl->_abc_negative_cache_version, version);
#else
    impl->_abc_negative_cache_version = version;
#endif
}

static int
abc_data_traverse(PyObject *op, visitproc visit, void *arg)
{
    _abc_data *self = _abc_data_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->_abc_registry);
    Py_VISIT(self->_abc_cache);
    Py_VISIT(self->_abc_negative_cache);
    return 0;
}

static int
abc_data_clear(PyObject *op)
{
    _abc_data *self = _abc_data_CAST(op);
    Py_CLEAR(self->_abc_registry);
    Py_CLEAR(self->_abc_cache);
    Py_CLEAR(self->_abc_negative_cache);
    return 0;
}

static void
abc_data_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    PyTypeObject *tp = Py_TYPE(self);
    (void)abc_data_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
abc_data_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _abc_data *self = (_abc_data *) type->tp_alloc(type, 0);
    _abcmodule_state *state = NULL;
    if (self == NULL) {
        return NULL;
    }

    state = _PyType_GetModuleState(type);
    if (state == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    self->_abc_registry = NULL;
    self->_abc_cache = NULL;
    self->_abc_negative_cache = NULL;
    self->_abc_negative_cache_version = get_invalidation_counter(state);
    return (PyObject *) self;
}

PyDoc_STRVAR(abc_data_doc,
"Internal state held by ABC machinery.");

static PyType_Slot _abc_data_type_spec_slots[] = {
    {Py_tp_doc, (void *)abc_data_doc},
    {Py_tp_new, abc_data_new},
    {Py_tp_dealloc, abc_data_dealloc},
    {Py_tp_traverse, abc_data_traverse},
    {Py_tp_clear, abc_data_clear},
    {0, 0}
};

static PyType_Spec _abc_data_type_spec = {
    .name = "_abc._abc_data",
    .basicsize = sizeof(_abc_data),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = _abc_data_type_spec_slots,
};

static _abc_data *
_get_impl(PyObject *module, PyObject *self)
{
    _abcmodule_state *state = get_abc_state(module);
    PyValue impl_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(self), &_Py_ID(_abc_impl));
    PyObject *impl = PyValue_IsNull(impl_v) ? NULL : _PyValue_AsObjectStealAt(impl_v, BOX_SITE_FIELD);
    if (impl == NULL) {
        return NULL;
    }
    if (!Py_IS_TYPE(impl, state->_abc_data_type)) {
        PyErr_SetString(PyExc_TypeError, "_abc_impl is set to a wrong type");
        Py_DECREF(impl);
        return NULL;
    }
    return (_abc_data *)impl;
}

/* _get_impl() of a value: an inline int or float is boxed first. */
static _abc_data *
_get_impl_value(PyObject *module, PyValueBorrow self)
{
    PyObject *self_obj = _PyValue_AsObjectAt(self, BOX_SITE_PARSER);
    if (self_obj == NULL) {
        return NULL;
    }
    _abc_data *impl = _get_impl(module, self_obj);
    Py_DECREF(self_obj);
    return impl;
}

static int
_in_weak_set(_abc_data *impl, PyObject **pset, PyObject *obj)
{
    PyObject *set;
    Py_BEGIN_CRITICAL_SECTION(impl);
    set = *pset;
    Py_END_CRITICAL_SECTION();
    if (set == NULL || PySet_GET_SIZE(set) == 0) {
        return 0;
    }
    PyObject *ref = PyWeakref_NewRef(obj, NULL);
    if (ref == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Clear();
            return 0;
        }
        return -1;
    }
    int res = PySet_ContainsValue(set, PyValue_FromObjectBorrow(ref));
    Py_DECREF(ref);
    return res;
}

static PyObject *
_destroy(PyObject *setweakref, PyObject *objweakref)
{
    PyObject *set = _PyWeakref_GET_REF(setweakref);
    if (set == NULL) {
        Py_RETURN_NONE;
    }
    if (PySet_DiscardValue(set, PyValue_FromObjectBorrow(objweakref)) < 0) {
        Py_DECREF(set);
        return NULL;
    }
    Py_DECREF(set);
    Py_RETURN_NONE;
}

static PyMethodDef _destroy_def = {
    "_destroy", _destroy, METH_O
};

static int
_add_to_weak_set(_abc_data *impl, PyObject **pset, PyObject *obj)
{
    PyObject *set;
    Py_BEGIN_CRITICAL_SECTION(impl);
    set = *pset;
    if (set == NULL) {
        set = *pset = PySet_New(NULL);
    }
    Py_END_CRITICAL_SECTION();
    if (set == NULL) {
        return -1;
    }

    PyObject *ref, *wr;
    PyObject *destroy_cb;
    wr = PyWeakref_NewRef(set, NULL);
    if (wr == NULL) {
        return -1;
    }
    destroy_cb = PyCFunction_NewEx(&_destroy_def, wr, NULL);
    if (destroy_cb == NULL) {
        Py_DECREF(wr);
        return -1;
    }
    ref = PyWeakref_NewRef(obj, destroy_cb);
    Py_DECREF(destroy_cb);
    if (ref == NULL) {
        Py_DECREF(wr);
        return -1;
    }
    int ret = PySet_AddValue(set, PyValue_FromObjectBorrow(ref));
    Py_DECREF(wr);
    Py_DECREF(ref);
    return ret;
}

/*[clinic input]
_abc._reset_registry

    self: object
    /

Internal ABC helper to reset registry of a given class.

Should be only used by refleak.py
[clinic start generated code]*/

static PyValue
_abc__reset_registry_impl(PyObject *module, PyValueBorrow self)
/*[clinic end generated code: output=58d9e874d176c159 input=12a0b7eb339ac35c]*/
{
    _abc_data *impl = _get_impl_value(module, self);
    if (impl == NULL) {
        return PyValue_NULL;
    }
    PyObject *registry;
    Py_BEGIN_CRITICAL_SECTION(impl);
    registry = impl->_abc_registry;
    Py_END_CRITICAL_SECTION();
    if (registry != NULL && PySet_Clear(registry) < 0) {
        Py_DECREF(impl);
        return PyValue_NULL;
    }
    Py_DECREF(impl);
    return PyValue_None;
}

/*[clinic input]
_abc._reset_caches

    self: object
    /

Internal ABC helper to reset both caches of a given class.

Should be only used by refleak.py
[clinic start generated code]*/

static PyValue
_abc__reset_caches_impl(PyObject *module, PyValueBorrow self)
/*[clinic end generated code: output=dca934a1ec714955 input=c0ac616fd8acfb6f]*/
{
    _abc_data *impl = _get_impl_value(module, self);
    if (impl == NULL) {
        return PyValue_NULL;
    }
    PyObject *cache, *negative_cache;
    Py_BEGIN_CRITICAL_SECTION(impl);
    cache = impl->_abc_cache;
    negative_cache = impl->_abc_negative_cache;
    Py_END_CRITICAL_SECTION();
    if (cache != NULL && PySet_Clear(cache) < 0) {
        Py_DECREF(impl);
        return PyValue_NULL;
    }
    /* also the second cache */
    if (negative_cache != NULL && PySet_Clear(negative_cache) < 0) {
        Py_DECREF(impl);
        return PyValue_NULL;
    }
    Py_DECREF(impl);
    return PyValue_None;
}

/*[clinic input]
_abc._get_dump

    self: object
    /

Internal ABC helper for cache and registry debugging.

Return shallow copies of registry, of both caches, and
negative cache version. Don't call this function directly,
instead use ABC._dump_registry() for a nice repr.
[clinic start generated code]*/

static PyValue
_abc__get_dump_impl(PyObject *module, PyValueBorrow self)
/*[clinic end generated code: output=f1123d5801e06e2e input=2c5deb1bfe9e3c79]*/
{
    _abc_data *impl = _get_impl_value(module, self);
    if (impl == NULL) {
        return PyValue_NULL;
    }
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(impl);
    res = Py_BuildValue("NNNK",
                        PySet_New(impl->_abc_registry),
                        PySet_New(impl->_abc_cache),
                        PySet_New(impl->_abc_negative_cache),
                        get_cache_version(impl));
    Py_END_CRITICAL_SECTION();
    Py_DECREF(impl);
    return _PyValue_FromObjectResult(res);
}

// Compute set of abstract method names.
static int
compute_abstract_methods(PyObject *self)
{
    int ret = -1;
    PyObject *abstracts = PyFrozenSet_New(NULL);
    if (abstracts == NULL) {
        return -1;
    }

    PyObject *ns = NULL, *items = NULL, *bases = NULL;  // Py_XDECREF()ed on error.

    /* Stage 1: direct abstract methods. */
    PyValue ns_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(self), &_Py_ID(__dict__));
    ns = PyValue_IsNull(ns_v) ? NULL : _PyValue_AsObjectStealAt(ns_v, BOX_SITE_FIELD);
    if (!ns) {
        goto error;
    }

    // We can't use PyDict_Next(ns) even when ns is dict because
    // _PyObject_IsAbstract() can mutate ns.
    items = _PyValue_MappingItems(PyValue_FromObjectBorrow(ns));
    if (!items) {
        goto error;
    }
    assert(PyList_Check(items));
    for (Py_ssize_t pos = 0; pos < PyList_GET_SIZE(items); pos++) {
        PyValue pair_v = PyList_GetValue(items, pos);
        PyObject *pair = PyValue_IsNull(pair_v) ? NULL : _PyValue_AsObjectStealAt(pair_v, BOX_SITE_FIELD);
        if (!pair) {
            goto error;
        }
        PyObject *it = _PyValue_SequenceFast(PyValue_FromObjectBorrow(pair), "items() returned non-iterable");
        Py_DECREF(pair);
        if (!it) {
            goto error;
        }
        if (PySequence_Fast_GET_SIZE(it) != 2) {
            PyErr_SetString(PyExc_TypeError,
                            "items() returned item which size is not 2");
            Py_DECREF(it);
            goto error;
        }

        // items or it may be cleared while accessing __abstractmethod__
        // So we need to keep strong references for key and value
        PyValue key = PyList_CheckExact(it) ? PyList_GetValue(it, 0)
                                            : PyTuple_GetValue(it, 0);
        if (PyValue_IsNull(key)) {
            Py_DECREF(it);
            goto error;
        }
        PyValue value_v = PyList_CheckExact(it) ? PyList_GetValue(it, 1)
                                  : PyTuple_GetValue(it, 1);
        if (PyValue_IsNull(value_v)) {
            PyValue_CLOSE(key);
            Py_DECREF(it);
            goto error;
        }
        // An int or float has no __isabstractmethod__.
        int is_abstract = PyValue_IsInline(value_v)
            ? 0 : _PyObject_IsAbstract(PyValue_AsKnownObjectBorrow(value_v));
        PyValue_CLOSE(value_v);
        if (is_abstract < 0 ||
                (is_abstract && PySet_AddValue(abstracts, key) < 0)) {
            Py_DECREF(it);
            PyValue_CLOSE(key);
            goto error;
        }
        PyValue_CLOSE(key);
        Py_DECREF(it);
    }

    /* Stage 2: inherited abstract methods. */
    PyValue bases_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(self), &_Py_ID(__bases__));
    bases = PyValue_IsNull(bases_v) ? NULL : _PyValue_AsObjectStealAt(bases_v, BOX_SITE_FIELD);
    if (!bases) {
        goto error;
    }
    if (!PyTuple_Check(bases)) {
        PyErr_SetString(PyExc_TypeError, "__bases__ is not tuple");
        goto error;
    }

    for (Py_ssize_t pos = 0; pos < PyTuple_GET_SIZE(bases); pos++) {
        PyValue item_v = PyTuple_GetValue(bases, pos);
        PyObject *item = PyValue_IsNull(item_v) ? NULL : _PyValue_AsObjectStealAt(item_v, BOX_SITE_FIELD);
        PyObject *base_abstracts, *iter;

        if (item == NULL) {
            goto error;
        }
        int status = PyObject_GetOptionalAttr(item, &_Py_ID(__abstractmethods__),
                                              &base_abstracts);
        Py_DECREF(item);
        if (status < 0) {
            goto error;
        }
        if (base_abstracts == NULL) {
            continue;
        }
        if (!(iter = _PyValue_GetIter(PyValue_FromObjectBorrow(base_abstracts)))) {
            Py_DECREF(base_abstracts);
            goto error;
        }
        Py_DECREF(base_abstracts);
        PyObject *key, *value;
        while ((key = PyIter_Next(iter))) {
            if (PyObject_GetOptionalAttr(self, key, &value) < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            if (value == NULL) {
                Py_DECREF(key);
                continue;
            }

            int is_abstract = _PyObject_IsAbstract(value);
            Py_DECREF(value);
            if (is_abstract < 0 ||
                    (is_abstract && PySet_AddValue(abstracts, PyValue_FromObjectBorrow(key)) < 0))
            {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            Py_DECREF(key);
        }
        Py_DECREF(iter);
        if (PyErr_Occurred()) {
            goto error;
        }
    }

    if (_PyValue_SetAttr(PyValue_FromObjectBorrow(self), &_Py_ID(__abstractmethods__), PyValue_FromObjectBorrow(abstracts)) < 0) {
        goto error;
    }

    ret = 0;
error:
    Py_DECREF(abstracts);
    Py_XDECREF(ns);
    Py_XDECREF(items);
    Py_XDECREF(bases);
    return ret;
}

#define COLLECTION_FLAGS (Py_TPFLAGS_SEQUENCE | Py_TPFLAGS_MAPPING)

static PyValue
abc_abc_init_body(PyObject *module, PyObject *self);

/*[clinic input]
@permit_long_summary
_abc._abc_init

    self: object
    /

Internal ABC helper for class set-up. Should be never used outside abc module.
[clinic start generated code]*/

static PyValue
_abc__abc_init_impl(PyObject *module, PyValueBorrow self)
/*[clinic end generated code: output=e9c20228f1136a63 input=0b3513f947736d39]*/
{
    PyObject *self_obj = _PyValue_AsObjectAt(self, BOX_SITE_PARSER);
    if (self_obj == NULL) {
        return PyValue_NULL;
    }
    PyValue result = abc_abc_init_body(module, self_obj);
    Py_DECREF(self_obj);
    return result;
}

static PyValue
abc_abc_init_body(PyObject *module, PyObject *self)
{
    _abcmodule_state *state = get_abc_state(module);
    PyObject *data;
    if (compute_abstract_methods(self) < 0) {
        return PyValue_NULL;
    }

    /* Set up inheritance registry. */
    data = abc_data_new(state->_abc_data_type, NULL, NULL);
    if (data == NULL) {
        return PyValue_NULL;
    }
    if (_PyValue_SetAttr(PyValue_FromObjectBorrow(self), &_Py_ID(_abc_impl), PyValue_FromObjectBorrow(data)) < 0) {
        Py_DECREF(data);
        return PyValue_NULL;
    }
    Py_DECREF(data);
    /* If __abc_tpflags__ & COLLECTION_FLAGS is set, then set the corresponding bit(s)
     * in the new class.
     * Used by collections.abc.Sequence and collections.abc.Mapping to indicate
     * their special status w.r.t. pattern matching. */
    if (PyType_Check(self)) {
        PyTypeObject *cls = (PyTypeObject *)self;
        PyObject *dict = _PyType_GetDict(cls);
        PyValue flags = PyValue_NULL;
        if (PyDict_PopValue(dict, PyValue_FromObjectBorrow(&_Py_ID(__abc_tpflags__)),
                            &flags) < 0) {
            return PyValue_NULL;
        }
        if (PyValue_IsNull(flags) || PyValue_TYPE(flags) != &PyLong_Type) {
            PyValue_XCLOSE(flags);
            return PyValue_None;
        }

        long val;
        {
            PyObject *flags_obj = _PyValue_AsObjectAt(flags, BOX_SITE_NUMBER);
            if (flags_obj == NULL) {
                PyValue_CLOSE(flags);
                return PyValue_NULL;
            }
            val = _PyValue_AsLong(PyValue_FromObjectBorrow(flags_obj));
            Py_DECREF(flags_obj);
        }
        PyValue_CLOSE(flags);
        if (val == -1 && PyErr_Occurred()) {
            return PyValue_NULL;
        }
        if ((val & COLLECTION_FLAGS) == COLLECTION_FLAGS) {
            PyErr_SetString(PyExc_TypeError, "__abc_tpflags__ cannot be both Py_TPFLAGS_SEQUENCE and Py_TPFLAGS_MAPPING");
            return PyValue_NULL;
        }
        _PyType_SetFlags((PyTypeObject *)self, 0, val & COLLECTION_FLAGS);
    }
    return PyValue_None;
}

static PyValue
abc_abc_register_body(PyObject *module, PyObject *self,
                PyValueBorrow subclass);

/*[clinic input]
@permit_long_summary
_abc._abc_register

    self: object
    subclass: object
    /

Internal ABC helper for subclasss registration. Should be never used outside abc module.
[clinic start generated code]*/

static PyValue
_abc__abc_register_impl(PyObject *module, PyValueBorrow self,
                        PyValueBorrow subclass)
/*[clinic end generated code: output=e8fbf397452cbbaf input=135ab13a581b4414]*/
{
    PyObject *self_obj = _PyValue_AsObjectAt(self, BOX_SITE_PARSER);
    if (self_obj == NULL) {
        return PyValue_NULL;
    }
    PyValue result = abc_abc_register_body(module, self_obj, subclass);
    Py_DECREF(self_obj);
    return result;
}

static PyValue
abc_abc_register_body(PyObject *module, PyObject *self,
                PyValueBorrow subclass)
{
    if (!(PyValue_IsObject(subclass) && PyType_Check(PyValue_AsKnownObjectBorrow(subclass)))) {
        PyErr_SetString(PyExc_TypeError, "Can only register classes");
        return PyValue_NULL;
    }
    PyObject *subclass_obj = PyValue_AsKnownObjectBorrow(subclass);  // checked above
    int result = PyObject_IsSubclass(subclass_obj, self);
    if (result > 0) {
        return PyValue_DUP(subclass);  /* Already a subclass. */
    }
    if (result < 0) {
        return PyValue_NULL;
    }
    /* Subtle: test for cycles *after* testing for "already a subclass";
       this means we allow X.register(X) and interpret it as a no-op. */
    result = PyObject_IsSubclass(self, subclass_obj);
    if (result > 0) {
        /* This would create a cycle, which is bad for the algorithm below. */
        PyErr_SetString(PyExc_RuntimeError, "Refusing to create an inheritance cycle");
        return PyValue_NULL;
    }
    if (result < 0) {
        return PyValue_NULL;
    }
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return PyValue_NULL;
    }
    if (_add_to_weak_set(impl, &impl->_abc_registry, subclass_obj) < 0) {
        Py_DECREF(impl);
        return PyValue_NULL;
    }
    Py_DECREF(impl);

    /* Invalidate negative cache */
    increment_invalidation_counter(get_abc_state(module));

    /* Set Py_TPFLAGS_SEQUENCE or Py_TPFLAGS_MAPPING flag */
    if (PyType_Check(self)) {
        unsigned long collection_flag =
            PyType_GetFlags((PyTypeObject *)self) & COLLECTION_FLAGS;
        if (collection_flag) {
            _PyType_SetFlagsRecursive((PyTypeObject *)subclass_obj,
                                      COLLECTION_FLAGS,
                                      collection_flag);
        }
    }
    return PyValue_DUP(subclass);
}


static PyValue
abc_abc_instancecheck_body(PyObject *module, PyObject *self,
                           PyObject *instance);

/*[clinic input]
@permit_long_summary
_abc._abc_instancecheck

    self: object
    instance: object
    /

Internal ABC helper for instance checks. Should be never used outside abc module.
[clinic start generated code]*/

static PyValue
_abc__abc_instancecheck_impl(PyObject *module, PyValueBorrow self,
                             PyValueBorrow instance)
/*[clinic end generated code: output=583622aac3140613 input=0bbc8da0ea346719]*/
{
    PyObject *self_obj = _PyValue_AsObjectAt(self, BOX_SITE_PARSER);
    if (self_obj == NULL) {
        return PyValue_NULL;
    }
    PyObject *instance_obj = _PyValue_AsObjectAt(instance, BOX_SITE_PARSER);
    if (instance_obj == NULL) {
        Py_DECREF(self_obj);
        return PyValue_NULL;
    }
    PyValue result = abc_abc_instancecheck_body(module, self_obj, instance_obj);
    Py_DECREF(instance_obj);
    Py_DECREF(self_obj);
    return result;
}

static PyValue
abc_abc_instancecheck_body(PyObject *module, PyObject *self,
                           PyObject *instance)
{
    PyObject *subtype, *result = NULL, *subclass = NULL;
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return PyValue_NULL;
    }

    if (PyObject_GetOptionalAttr(instance, &_Py_ID(__class__), &subclass) < 0) {
        Py_DECREF(impl);
        return PyValue_NULL;
    }
    if (subclass == NULL) {
        /* Fall back to the type when the instance has no __class__, matching
           the behaviour of the built-in isinstance() (gh-153772). */
        subclass = Py_NewRef((PyObject *)Py_TYPE(instance));
    }
    /* Inline the cache checking. */
    int incache = _in_weak_set(impl, &impl->_abc_cache, subclass);
    if (incache < 0) {
        goto end;
    }
    if (incache > 0) {
        result = Py_NewRef(Py_True);
        goto end;
    }
    subtype = (PyObject *)Py_TYPE(instance);
    if (subtype == subclass) {
        if (get_cache_version(impl) == get_invalidation_counter(get_abc_state(module))) {
            incache = _in_weak_set(impl, &impl->_abc_negative_cache, subclass);
            if (incache < 0) {
                goto end;
            }
            if (incache > 0) {
                result = Py_NewRef(Py_False);
                goto end;
            }
        }
        /* Fall back to the subclass check. */
        result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                           subclass);
        goto end;
    }
    /* A subclass check can change the instance's actual type. */
    Py_INCREF(subtype);
    result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                       subclass);
    if (result == NULL) {
        Py_DECREF(subtype);
        goto end;
    }

    switch (_PyValue_IsTrue(PyValue_FromObjectBorrow(result))) {
    case -1:
        Py_SETREF(result, NULL);
        break;
    case 0:
        Py_DECREF(result);
        result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                           subtype);
        break;
    case 1:  // Nothing to do.
        break;
    default:
        Py_UNREACHABLE();
    }
    Py_DECREF(subtype);

end:
    Py_XDECREF(impl);
    Py_XDECREF(subclass);
    return _PyValue_FromObjectResult(result);
}


// Return -1 when exception occurred.
// Return 1 when result is set.
// Return 0 otherwise.
static int subclasscheck_check_registry(_abc_data *impl, PyObject *subclass,
                                        PyObject **result);

static PyValue
abc_abc_subclasscheck_body(PyObject *module, PyObject *self,
                    PyValueBorrow subclass);

/*[clinic input]
@permit_long_summary
_abc._abc_subclasscheck

    self: object
    subclass: object
    /

Internal ABC helper for subclasss checks. Should be never used outside abc module.
[clinic start generated code]*/

static PyValue
_abc__abc_subclasscheck_impl(PyObject *module, PyValueBorrow self,
                             PyValueBorrow subclass)
/*[clinic end generated code: output=09fc059365eb79c3 input=5bf1ef712f5d3610]*/
{
    PyObject *self_obj = _PyValue_AsObjectAt(self, BOX_SITE_PARSER);
    if (self_obj == NULL) {
        return PyValue_NULL;
    }
    PyValue result = abc_abc_subclasscheck_body(module, self_obj, subclass);
    Py_DECREF(self_obj);
    return result;
}

static PyValue
abc_abc_subclasscheck_body(PyObject *module, PyObject *self,
                    PyValueBorrow subclass)
{
    if (!(PyValue_IsObject(subclass) && PyType_Check(PyValue_AsKnownObjectBorrow(subclass)))) {
        PyErr_SetString(PyExc_TypeError, "issubclass() arg 1 must be a class");
        return PyValue_NULL;
    }
    PyObject *subclass_obj = PyValue_AsKnownObjectBorrow(subclass);  // checked above

    PyObject *ok, *subclasses = NULL, *result = NULL;
    _abcmodule_state *state = NULL;
    Py_ssize_t pos;
    int incache;
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return PyValue_NULL;
    }

    /* 1. Check cache. */
    incache = _in_weak_set(impl, &impl->_abc_cache, subclass_obj);
    if (incache < 0) {
        goto end;
    }
    if (incache > 0) {
        result = Py_True;
        goto end;
    }

    state = get_abc_state(module);
    /* 2. Check negative cache; may have to invalidate. */
    uint64_t invalidation_counter = get_invalidation_counter(state);
    if (get_cache_version(impl) < invalidation_counter) {
        /* Invalidate the negative cache. */
        PyObject *negative_cache;
        Py_BEGIN_CRITICAL_SECTION(impl);
        negative_cache = impl->_abc_negative_cache;
        Py_END_CRITICAL_SECTION();
        if (negative_cache != NULL && PySet_Clear(negative_cache) < 0) {
            goto end;
        }
        set_cache_version(impl, invalidation_counter);
    }
    else {
        incache = _in_weak_set(impl, &impl->_abc_negative_cache, subclass_obj);
        if (incache < 0) {
            goto end;
        }
        if (incache > 0) {
            result = Py_False;
            goto end;
        }
    }

    /* 3. Check the subclass hook. */
    ok = PyObject_CallMethodOneArg(
            self, &_Py_ID(__subclasshook__), subclass_obj);
    if (ok == NULL) {
        goto end;
    }
    if (ok == Py_True) {
        Py_DECREF(ok);
        if (_add_to_weak_set(impl, &impl->_abc_cache, subclass_obj) < 0) {
            goto end;
        }
        result = Py_True;
        goto end;
    }
    if (ok == Py_False) {
        Py_DECREF(ok);
        if (_add_to_weak_set(impl, &impl->_abc_negative_cache, subclass_obj) < 0) {
            goto end;
        }
        result = Py_False;
        goto end;
    }
    if (ok != Py_NotImplemented) {
        Py_DECREF(ok);
        PyErr_SetString(PyExc_AssertionError, "__subclasshook__ must return either"
                                              " False, True, or NotImplemented");
        goto end;
    }
    Py_DECREF(ok);

    /* 4. Check if it's a direct subclass. */
    if (PyType_IsSubtype((PyTypeObject *)subclass_obj, (PyTypeObject *)self)) {
        if (_add_to_weak_set(impl, &impl->_abc_cache, subclass_obj) < 0) {
            goto end;
        }
        result = Py_True;
        goto end;
    }

    /* 5. Check if it's a subclass of a registered class (recursive). */
    if (subclasscheck_check_registry(impl, subclass_obj, &result)) {
        // Exception occurred or result is set.
        goto end;
    }

    /* 6. Check if it's a subclass of a subclass (recursive). */
    subclasses = PyObject_CallMethod(self, "__subclasses__", NULL);
    if (subclasses == NULL) {
        goto end;
    }
    if (!PyList_Check(subclasses)) {
        PyErr_SetString(PyExc_TypeError, "__subclasses__() must return a list");
        goto end;
    }
    for (pos = 0; pos < PyList_GET_SIZE(subclasses); pos++) {
        PyValue scls_v = PyList_GetValue(subclasses, pos);
        PyObject *scls = PyValue_IsNull(scls_v) ? NULL : _PyValue_AsObjectStealAt(scls_v, BOX_SITE_FIELD);
        if (scls == NULL) {
            goto end;
        }
        int r = PyObject_IsSubclass(subclass_obj, scls);
        Py_DECREF(scls);
        if (r > 0) {
            if (_add_to_weak_set(impl, &impl->_abc_cache, subclass_obj) < 0) {
                goto end;
            }
            result = Py_True;
            goto end;
        }
        if (r < 0) {
            goto end;
        }
    }

    /* No dice; update negative cache. */
    if (_add_to_weak_set(impl, &impl->_abc_negative_cache, subclass_obj) < 0) {
        goto end;
    }
    result = Py_False;

end:
    Py_DECREF(impl);
    Py_XDECREF(subclasses);
    return _PyValue_FromObjectResult(Py_XNewRef(result));
}


static int
subclasscheck_check_registry(_abc_data *impl, PyObject *subclass,
                             PyObject **result)
{
    // Fast path: check subclass is in weakref directly.
    int ret = _in_weak_set(impl, &impl->_abc_registry, subclass);
    if (ret < 0) {
        *result = NULL;
        return -1;
    }
    if (ret > 0) {
        *result = Py_True;
        return 1;
    }

    PyObject *registry_shared;
    Py_BEGIN_CRITICAL_SECTION(impl);
    registry_shared = impl->_abc_registry;
    Py_END_CRITICAL_SECTION();
    if (registry_shared == NULL) {
        return 0;
    }

    // Make a local copy of the registry to protect against concurrent
    // modifications of _abc_registry.
    PyObject *registry = PyFrozenSet_New(registry_shared);
    if (registry == NULL) {
        return -1;
    }
    PyValue keyv;
    Py_ssize_t pos = 0;
    Py_hash_t hash;
    int rc;

    while ((rc = _PySet_NextValue(registry, &pos, &keyv, &hash)) > 0) {
        PyValue key_v = keyv;
        PyObject *key = PyValue_IsNull(key_v) ? NULL : _PyValue_AsObjectStealAt(key_v, BOX_SITE_FIELD);
        if (key == NULL) {
            ret = -1;
            rc = 0;
            break;
        }
        PyObject *rkey;
        int wr = PyWeakref_GetRef(key, &rkey);
        Py_DECREF(key);
        if (wr < 0) {
            // Someone inject non-weakref type in the registry.
            ret = -1;
            break;
        }

        if (rkey == NULL) {
            continue;
        }
        int r = PyObject_IsSubclass(subclass, rkey);
        Py_DECREF(rkey);
        if (r < 0) {
            ret = -1;
            break;
        }
        if (r > 0) {
            if (_add_to_weak_set(impl, &impl->_abc_cache, subclass) < 0) {
                ret = -1;
                break;
            }
            *result = Py_True;
            ret = 1;
            break;
        }
    }
    if (rc < 0) {
        ret = -1;
    }

    Py_DECREF(registry);
    return ret;
}

/*[clinic input]
_abc.get_cache_token

Returns the current ABC cache token.

The token is an opaque object (supporting equality testing) identifying
the current version of the ABC cache for virtual subclasses.  The token
changes with every call to register() on any ABC.
[clinic start generated code]*/

static PyValue
_abc_get_cache_token_impl(PyObject *module)
/*[clinic end generated code: output=c50002ea165181ff input=d87acc04492f6bf3]*/
{
    _abcmodule_state *state = get_abc_state(module);
    return _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong(get_invalidation_counter(state)));
}

static struct PyMethodDef _abcmodule_methods[] = {
    _ABC_GET_CACHE_TOKEN_METHODDEF
    _ABC__ABC_INIT_METHODDEF
    _ABC__RESET_REGISTRY_METHODDEF
    _ABC__RESET_CACHES_METHODDEF
    _ABC__GET_DUMP_METHODDEF
    _ABC__ABC_REGISTER_METHODDEF
    _ABC__ABC_INSTANCECHECK_METHODDEF
    _ABC__ABC_SUBCLASSCHECK_METHODDEF
    {NULL,       NULL}          /* sentinel */
};

static int
_abcmodule_exec(PyObject *module)
{
    _abcmodule_state *state = get_abc_state(module);
    state->abc_invalidation_counter = 0;
    state->_abc_data_type = (PyTypeObject *)PyType_FromModuleAndSpec(module, &_abc_data_type_spec, NULL);
    if (state->_abc_data_type == NULL) {
        return -1;
    }

    return 0;
}

static int
_abcmodule_traverse(PyObject *module, visitproc visit, void *arg)
{
    _abcmodule_state *state = get_abc_state(module);
    Py_VISIT(state->_abc_data_type);
    return 0;
}

static int
_abcmodule_clear(PyObject *module)
{
    _abcmodule_state *state = get_abc_state(module);
    Py_CLEAR(state->_abc_data_type);
    return 0;
}

static void
_abcmodule_free(void *module)
{
    (void)_abcmodule_clear((PyObject *)module);
}

static PyModuleDef_Slot _abcmodule_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, _abcmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _abcmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_abc",
    .m_doc = _abc__doc__,
    .m_size = sizeof(_abcmodule_state),
    .m_methods = _abcmodule_methods,
    .m_slots = _abcmodule_slots,
    .m_traverse = _abcmodule_traverse,
    .m_clear = _abcmodule_clear,
    .m_free = _abcmodule_free,
};

PyMODINIT_FUNC
PyInit__abc(void)
{
    return PyModuleDef_Init(&_abcmodule);
}
