/* Type object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PySequence_IterSearch()
#include "pycore_borrow.h"        // _PyBorrowScope_Enter()
#include "pycore_call.h"          // _PyObject_VectorcallTstate()
#include "pycore_code.h"          // CO_FAST_FREE
#include "pycore_descrobject.h"   // _PyMember_GetOffset()
#include "pycore_dict.h"          // _PyDict_KeysSize()
#include "pycore_function.h"      // _PyFunction_GetVersionForCurrentState()
#include "pycore_gc_trace.h"      // _PyGC_TraceRegisterStaticType()
#include "pycore_interpframe.h"   // _PyInterpreterFrame
#include "pycore_long.h"          // _PyLong_IsNegative(), _PyLong_GetOne()
#include "pycore_memoryobject.h"  // _PyMemoryView_FromBufferProc()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_moduleobject.h"  // _PyModule_GetDef()
#include "pycore_object.h"        // _PyType_HasFeature()
#include "pycore_object_alloc.h"  // _PyObject_MallocWithType()
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_slots.h"         // _PySlotIterator_Init
#include "pycore_symtable.h"      // _Py_Mangle()
#include "pycore_tuple.h"         // _PyTuple_FromPair
#include "pycore_typecache.h"     // _PyTypeCache_Lookup()
#include "pycore_typeobject.h"    // _PyTypes_InitTypes()
#include "pycore_unicodeobject.h" // _PyUnicode_Copy
#include "pycore_unionobject.h"   // _Py_union_type_or
#include "pycore_value.h"         // _PyValue_RestoreException()
#include "pycore_weakref.h"       // _PyWeakref_GET_REF()
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_stats.h"
#include "opcode.h"               // MAKE_CELL

#include <stddef.h>               // ptrdiff_t

/*[clinic input]
class type "PyTypeObject *" "&PyType_Type"
class object "PyObject *" "&PyBaseObject_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4b94608d231c434b]*/

#include "clinic/typeobject.c.h"

/* Support type attribute lookup cache */

#define MCACHE_CACHEABLE_NAME(name) (PyUnicode_CheckExact(name) && PyUnicode_CHECK_INTERNED(name))

#define NEXT_VERSION_TAG(interp) \
    (interp)->types.next_version_tag

// Storage for the mutexes saved by type_lock_prevent_release().  Defined for
// both builds so the call sites don't need to be conditionally compiled.
typedef struct {
    PyMutex *mutex1;
    PyMutex *mutex2;
} pinned_mutexes_t;

#ifdef Py_GIL_DISABLED

// There's a global lock for types that ensures that tp_version_tag and
// _spec_cache are correctly updated if the type is modified.  It also protects
// tp_mro, tp_bases, and tp_base.  This avoids having to take additional locks
// while doing various subclass processing which may result in odd behaviors
// w.r.t. running with the GIL as the outer type lock could be released and
// reacquired during a subclass update if there's contention on the subclass
// lock.
//
// Note that this lock does not protect updates of other type slots or the
// tp_flags member.  Instead, we either ensure those updates are done before
// the type has been revealed to other threads or we only do those updates
// while the stop-the-world mechanism is active.  The slots and flags are read
// in many places without holding a lock and without atomics.
#define TYPE_LOCK &_PyInterpreterState_GET()->types.mutex
#define BEGIN_TYPE_LOCK() Py_BEGIN_CRITICAL_SECTION_MUTEX(TYPE_LOCK)
#define END_TYPE_LOCK() Py_END_CRITICAL_SECTION()

#define BEGIN_TYPE_DICT_LOCK(d) \
    Py_BEGIN_CRITICAL_SECTION2_MUTEX(TYPE_LOCK, &_PyObject_CAST(d)->ob_mutex)

#define END_TYPE_DICT_LOCK() Py_END_CRITICAL_SECTION2()

#if !defined(NDEBUG) || defined(Py_DEBUG)
// Return true if the world is currently stopped.
static bool
types_world_is_stopped(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return interp->stoptheworld.world_stopped;
}
#endif

// Checks that the type has not yet been revealed (exposed) to other
// threads.  The _Py_TYPE_REVEALED_FLAG flag is set by type_new() and
// PyType_FromMetaclass() to indicate that a newly initialized type might be
// revealed.  We only have ob_flags on 64-bit platforms.
#if SIZEOF_VOID_P > 4
#define TYPE_IS_REVEALED(tp) ((((PyObject *)(tp))->ob_flags & _Py_TYPE_REVEALED_FLAG) != 0)
#else
#define TYPE_IS_REVEALED(tp) 0
#endif

#ifdef Py_DEBUG
#define ASSERT_TYPE_LOCK_HELD() \
    if (!types_world_is_stopped()) { _Py_CRITICAL_SECTION_ASSERT_MUTEX_LOCKED(TYPE_LOCK); }

// Checks if we can safely update type slots or tp_flags.
#define ASSERT_WORLD_STOPPED_OR_NEW_TYPE(tp) \
    assert(!TYPE_IS_REVEALED(tp) || types_world_is_stopped())

#define ASSERT_NEW_TYPE_OR_LOCKED(tp) \
    if (TYPE_IS_REVEALED(tp)) { ASSERT_TYPE_LOCK_HELD(); }
#else
#define ASSERT_TYPE_LOCK_HELD()
#define ASSERT_WORLD_STOPPED_OR_NEW_TYPE(tp)
#define ASSERT_NEW_TYPE_OR_LOCKED(tp)
#endif

static void
types_stop_world(void)
{
    assert(!types_world_is_stopped());
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    assert(types_world_is_stopped());
}

static void
types_start_world(void)
{
    assert(types_world_is_stopped());
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StartTheWorld(interp);
    assert(!types_world_is_stopped());
}

// Temporarily prevent the mutexes held by the topmost critical section from
// being released when the current thread blocks (blocking detaches the thread,
// which suspends its critical sections and releases the mutexes they hold).
//
// All of the mutexes held by the critical section are pinned, not just
// TYPE_LOCK.  If only TYPE_LOCK was pinned then _PyCriticalSection_Resume()
// would have to re-acquire the other mutex while TYPE_LOCK is held.  That
// deadlocks against a thread that holds that mutex and is waiting for
// TYPE_LOCK, which is exactly what BEGIN_TYPE_DICT_LOCK() does: the type dict
// mutex is on the heap and TYPE_LOCK is in _PyRuntime, so the address ordering
// used by two-mutex critical sections usually acquires the dict mutex first.
// By pinning both mutexes there is nothing to re-acquire on resume.
//
// Holding the mutexes while blocked does not prevent the world from being
// stopped: a thread waiting on either of them parks with _PY_LOCK_DETACH and
// so is detached while it waits.
static void
type_lock_prevent_release(pinned_mutexes_t *pinned)
{
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t tag = tstate->critical_section;
    PyCriticalSection *c = (PyCriticalSection *)(tag & ~_Py_CRITICAL_SECTION_MASK);
    pinned->mutex1 = c->_cs_mutex;
    pinned->mutex2 = NULL;
    c->_cs_mutex = NULL;
    if ((tag & _Py_CRITICAL_SECTION_TWO_MUTEXES) != 0) {
        PyCriticalSection2 *c2 = (PyCriticalSection2 *)c;
        pinned->mutex2 = c2->_cs_mutex2;
        c2->_cs_mutex2 = NULL;
    }
    assert(pinned->mutex1 == TYPE_LOCK || pinned->mutex2 == TYPE_LOCK);
}

static void
type_lock_allow_release(pinned_mutexes_t *pinned)
{
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t tag = tstate->critical_section;
    PyCriticalSection *c = (PyCriticalSection *)(tag & ~_Py_CRITICAL_SECTION_MASK);
    assert(c->_cs_mutex == NULL);
    c->_cs_mutex = pinned->mutex1;
    if ((tag & _Py_CRITICAL_SECTION_TWO_MUTEXES) != 0) {
        PyCriticalSection2 *c2 = (PyCriticalSection2 *)c;
        assert(c2->_cs_mutex2 == NULL);
        c2->_cs_mutex2 = pinned->mutex2;
    }
    else {
        assert(pinned->mutex2 == NULL);
    }
}

#else

#define BEGIN_TYPE_LOCK()
#define END_TYPE_LOCK()
#define BEGIN_TYPE_DICT_LOCK(d)
#define END_TYPE_DICT_LOCK()
#define ASSERT_TYPE_LOCK_HELD()
#define TYPE_IS_REVEALED(tp) 0
#define ASSERT_WORLD_STOPPED_OR_NEW_TYPE(tp)
#define ASSERT_NEW_TYPE_OR_LOCKED(tp)
#define types_world_is_stopped() 1
#define types_stop_world()
#define types_start_world()
#define type_lock_prevent_release(pinned) ((void)(pinned))
#define type_lock_allow_release(pinned) ((void)(pinned))

#endif

#define PyTypeObject_CAST(op)   ((PyTypeObject *)(op))

static void
slot_bf_releasebuffer(PyObject *self, Py_buffer *buffer);

static void
releasebuffer_call_python(PyObject *self, Py_buffer *buffer);

static PyObject *
slot_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static int
slot_tp_setattro(PyObject *self, PyObject *name, PyObject *value);

static PyObject *
slot_tp_call(PyObject *self, PyObject *args, PyObject *kwds);

static inline PyTypeObject *
type_from_ref(PyObject *ref)
{
    PyObject *obj = _PyWeakref_GET_REF(ref);
    if (obj == NULL) {
        return NULL;
    }
    return _PyType_CAST(obj);
}


/* helpers for static builtin types */

#ifndef NDEBUG
static inline int
managed_static_type_index_is_set(PyTypeObject *self)
{
    return self->tp_subclasses != NULL;
}
#endif

static inline size_t
managed_static_type_index_get(PyTypeObject *self)
{
    assert(managed_static_type_index_is_set(self));
    /* We store a 1-based index so 0 can mean "not initialized". */
    return (size_t)self->tp_subclasses - 1;
}

static inline void
managed_static_type_index_set(PyTypeObject *self, size_t index)
{
    assert(index < _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES);
    /* We store a 1-based index so 0 can mean "not initialized". */
    self->tp_subclasses = (PyObject *)(index + 1);
}

static inline void
managed_static_type_index_clear(PyTypeObject *self)
{
    self->tp_subclasses = NULL;
}

static PyTypeObject *
static_ext_type_lookup(PyInterpreterState *interp, size_t index,
                       int64_t *p_interp_count)
{
    assert(interp->runtime == &_PyRuntime);
    assert(index < _Py_MAX_MANAGED_STATIC_EXT_TYPES);

    size_t full_index = index + _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES;
    int64_t interp_count = _Py_atomic_load_int64(
            &_PyRuntime.types.managed_static.types[full_index].interp_count);
    assert((interp_count == 0) ==
            (_PyRuntime.types.managed_static.types[full_index].type == NULL));
    *p_interp_count = interp_count;

    PyTypeObject *type = interp->types.for_extensions.initialized[index].type;
    if (type == NULL) {
        return NULL;
    }
    assert(!interp->types.for_extensions.initialized[index].isbuiltin);
    assert(type == _PyRuntime.types.managed_static.types[full_index].type);
    assert(managed_static_type_index_is_set(type));
    return type;
}

static managed_static_type_state *
managed_static_type_state_get(PyInterpreterState *interp, PyTypeObject *self)
{
    // It's probably a builtin type.
    size_t index = managed_static_type_index_get(self);
    managed_static_type_state *state =
            &(interp->types.builtins.initialized[index]);
    if (state->type == self) {
        return state;
    }
    if (index > _Py_MAX_MANAGED_STATIC_EXT_TYPES) {
        return state;
    }
    return &(interp->types.for_extensions.initialized[index]);
}

/* For static types we store some state in an array on each interpreter. */
managed_static_type_state *
_PyStaticType_GetState(PyInterpreterState *interp, PyTypeObject *self)
{
    assert(self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN);
    return managed_static_type_state_get(interp, self);
}

/* Set the type's per-interpreter state. */
static void
managed_static_type_state_init(PyInterpreterState *interp, PyTypeObject *self,
                               int isbuiltin, int initial)
{
    assert(interp->runtime == &_PyRuntime);

    size_t index;
    if (initial) {
        assert(!managed_static_type_index_is_set(self));
        if (isbuiltin) {
            index = interp->types.builtins.num_initialized;
            assert(index < _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES);
        }
        else {
            PyMutex_Lock(&interp->types.mutex);
            index = interp->types.for_extensions.next_index;
            interp->types.for_extensions.next_index++;
            PyMutex_Unlock(&interp->types.mutex);
            assert(index < _Py_MAX_MANAGED_STATIC_EXT_TYPES);
        }
        managed_static_type_index_set(self, index);
    }
    else {
        index = managed_static_type_index_get(self);
        if (isbuiltin) {
            assert(index == interp->types.builtins.num_initialized);
            assert(index < _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES);
        }
        else {
            assert(index < _Py_MAX_MANAGED_STATIC_EXT_TYPES);
        }
    }
    size_t full_index = isbuiltin
        ? index
        : index + _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES;

    assert((initial == 1) ==
            (_Py_atomic_load_int64(&_PyRuntime.types.managed_static.types[full_index].interp_count) == 0));
    (void)_Py_atomic_add_int64(
            &_PyRuntime.types.managed_static.types[full_index].interp_count, 1);

    if (initial) {
        assert(_PyRuntime.types.managed_static.types[full_index].type == NULL);
        _PyRuntime.types.managed_static.types[full_index].type = self;
    }
    else {
        assert(_PyRuntime.types.managed_static.types[full_index].type == self);
    }

    managed_static_type_state *state = isbuiltin
        ? &(interp->types.builtins.initialized[index])
        : &(interp->types.for_extensions.initialized[index]);

    /* It should only be called once for each builtin type per interpreter. */
    assert(state->type == NULL);
    state->type = self;
    state->isbuiltin = isbuiltin;

    /* state->tp_subclasses is left NULL until init_subclasses() sets it. */
    /* state->tp_weaklist is left NULL until insert_head() or insert_after()
       (in weakrefobject.c) sets it. */

    if (isbuiltin) {
        interp->types.builtins.num_initialized++;
    }
    else {
        interp->types.for_extensions.num_initialized++;
    }
}

/* Reset the type's per-interpreter state.
   This basically undoes what managed_static_type_state_init() did. */
static void
managed_static_type_state_clear(PyInterpreterState *interp, PyTypeObject *self,
                                int isbuiltin, int final)
{
    size_t index = managed_static_type_index_get(self);
    size_t full_index = isbuiltin
        ? index
        : index + _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES;

    managed_static_type_state *state = isbuiltin
        ? &(interp->types.builtins.initialized[index])
        : &(interp->types.for_extensions.initialized[index]);
    assert(state != NULL);

    assert(_Py_atomic_load_int64(&_PyRuntime.types.managed_static.types[full_index].interp_count) > 0);
    assert(_PyRuntime.types.managed_static.types[full_index].type == state->type);

    assert(state->type != NULL);
    state->type = NULL;
    state->ready = 0;
    assert(state->tp_weaklist == NULL);  // It was already cleared out.

    (void)_Py_atomic_add_int64(
            &_PyRuntime.types.managed_static.types[full_index].interp_count, -1);
    if (final) {
        assert(!_Py_atomic_load_int64(&_PyRuntime.types.managed_static.types[full_index].interp_count));
        _PyRuntime.types.managed_static.types[full_index].type = NULL;

        managed_static_type_index_clear(self);
    }

    if (isbuiltin) {
        assert(interp->types.builtins.num_initialized > 0);
        interp->types.builtins.num_initialized--;
    }
    else {
        PyMutex_Lock(&interp->types.mutex);
        assert(interp->types.for_extensions.num_initialized > 0);
        interp->types.for_extensions.num_initialized--;
        if (interp->types.for_extensions.num_initialized == 0) {
            interp->types.for_extensions.next_index = 0;
        }
        PyMutex_Unlock(&interp->types.mutex);
    }
}



PyObject *
_PyStaticType_GetBuiltins(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    Py_ssize_t count = (Py_ssize_t)interp->types.builtins.num_initialized;
    assert(count <= _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES);

    PyObject *results = PyList_New(count);
    if (results == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < count; i++) {
        PyTypeObject *cls = interp->types.builtins.initialized[i].type;
        assert(cls != NULL);
        assert(interp->types.builtins.initialized[i].isbuiltin);
        PyList_SET_ITEM(results, i, Py_NewRef((PyObject *)cls));
    }

    return results;
}


// Also see _PyStaticType_InitBuiltin() and _PyStaticType_FiniBuiltin().

/* end static builtin helpers */

static void
type_set_flags(PyTypeObject *tp, unsigned long flags)
{
    ASSERT_WORLD_STOPPED_OR_NEW_TYPE(tp);
    tp->tp_flags = flags;
}

static void
type_set_flags_with_mask(PyTypeObject *tp, unsigned long mask, unsigned long flags)
{
    ASSERT_WORLD_STOPPED_OR_NEW_TYPE(tp);
    unsigned long new_flags = (tp->tp_flags & ~mask) | flags;
    type_set_flags(tp, new_flags);
}

static void
type_add_flags(PyTypeObject *tp, unsigned long flag)
{
    type_set_flags(tp, tp->tp_flags | flag);
}

static void
type_clear_flags(PyTypeObject *tp, unsigned long flag)
{
    type_set_flags(tp, tp->tp_flags & ~flag);
}

static inline void
start_readying(PyTypeObject *type)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = managed_static_type_state_get(interp, type);
        assert(state != NULL);
        assert(!state->readying);
        state->readying = 1;
        return;
    }
    assert((type->tp_flags & Py_TPFLAGS_READYING) == 0);
    type_add_flags(type, Py_TPFLAGS_READYING);
}

static inline void
stop_readying(PyTypeObject *type)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = managed_static_type_state_get(interp, type);
        assert(state != NULL);
        assert(state->readying);
        state->readying = 0;
        return;
    }
    assert(type->tp_flags & Py_TPFLAGS_READYING);
    type_clear_flags(type, Py_TPFLAGS_READYING);
}

static inline int
is_readying(PyTypeObject *type)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = managed_static_type_state_get(interp, type);
        assert(state != NULL);
        return state->readying;
    }
    return (type->tp_flags & Py_TPFLAGS_READYING) != 0;
}


static int
type_cache_admitted(PyInterpreterState *interp, PyTypeObject *type)
{
    if (interp->types.lookup_closed) {
        _PyType_AbortClosedLookup();
    }
    if (interp->types.mutation_depth != 0 ||
        !(type->tp_flags & Py_TPFLAGS_READY)) {
        return 0;
    }
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        managed_static_type_state *state = _PyStaticType_GetState(interp, type);
        return state != NULL && state->type == type &&
            state->ready && !state->readying;
    }
    return !(type->tp_flags & Py_TPFLAGS_READYING);
}

unsigned int
_PyType_GetVersionForCurrentState(PyInterpreterState *interp, PyTypeObject *type)
{
    if (!type_cache_admitted(interp, type)) {
        return 0;
    }
    return FT_ATOMIC_LOAD_UINT_RELAXED(type->tp_version_tag);
}

int
_PyType_MatchesVersion(PyInterpreterState *interp, PyTypeObject *type,
                       unsigned int expected)
{
    unsigned int current = _PyType_GetVersionForCurrentState(interp, type);
    return expected != 0 && current == expected;
}


/* accessors for objects stored on PyTypeObject */

static inline PyObject *
lookup_tp_dict(PyTypeObject *self)
{
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, self);
        assert(state != NULL);
        return state->tp_dict;
    }
    return self->tp_dict;
}

PyObject *
_PyType_GetDict(PyTypeObject *self)
{
    /* It returns a borrowed reference. */
    return lookup_tp_dict(self);
}

PyObject *
PyType_GetDict(PyTypeObject *self)
{
    PyObject *dict = lookup_tp_dict(self);
    return _Py_XNewRef(dict);
}

static inline void
set_tp_dict(PyTypeObject *self, PyObject *dict)
{
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, self);
        assert(state != NULL);
        state->tp_dict = dict;
        return;
    }
    self->tp_dict = dict;
}

static inline void
clear_tp_dict(PyTypeObject *self)
{
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, self);
        assert(state != NULL);
        Py_CLEAR(state->tp_dict);
        return;
    }
    Py_CLEAR(self->tp_dict);
}


static inline PyObject *
lookup_tp_bases(PyTypeObject *self)
{
    return self->tp_bases;
}

PyObject *
_PyType_GetBases(PyTypeObject *self)
{
    PyObject *res;

    BEGIN_TYPE_LOCK();
    res = lookup_tp_bases(self);
    Py_INCREF(res);
    END_TYPE_LOCK();

    return res;
}

static inline void
mro_epoch_advance(PyTypeObject *type)
{
    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        PyHeapTypeObject *heap = (PyHeapTypeObject *)type;
        if (heap->ht_mro_epoch != UINT64_MAX) {
            heap->ht_mro_epoch++;
        }
    }
}

static inline void
set_tp_bases(PyTypeObject *self, PyObject *bases, int initial)
{
    assert(PyTuple_Check(bases));
    ASSERT_NEW_TYPE_OR_LOCKED(self);
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        // XXX tp_bases can probably be statically allocated for each
        // static builtin type.
        assert(PyTuple_CheckExact(bases));
        assert(initial);
        assert(self->tp_bases == NULL);
        if (PyTuple_GET_SIZE(bases) == 0) {
            assert(self->tp_base == NULL);
        }
        else {
            assert(PyTuple_GET_SIZE(bases) == 1);
            assert(PyValue_IsObject(PyTuple_GET_VALUE(bases, 0)) &&
                   PyValue_AsPyObjectBorrow(PyTuple_GET_VALUE(bases, 0)) ==
                       (PyObject *)self->tp_base);
            assert(self->tp_base->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN);
            assert(_Py_IsImmortal(self->tp_base));
        }
        _Py_SetImmortal(bases);
    }
    if (self->tp_bases != bases) {
        mro_epoch_advance(self);
    }
    self->tp_bases = bases;
}

static inline void
clear_tp_bases(PyTypeObject *self, int final)
{
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        if (final) {
            if (self->tp_bases != NULL) {
                if (PyTuple_GET_SIZE(self->tp_bases) == 0) {
                    Py_CLEAR(self->tp_bases);
                }
                else {
                    assert(_Py_IsImmortal(self->tp_bases));
                    _Py_ClearImmortal(self->tp_bases);
                }
            }
        }
        return;
    }
    if (self->tp_bases != NULL) {
        mro_epoch_advance(self);
    }
    Py_CLEAR(self->tp_bases);
}


static inline PyObject *
lookup_tp_mro(PyTypeObject *self)
{
    return self->tp_mro;
}

static inline void
set_tp_mro(PyTypeObject *self, PyObject *mro, int initial)
{
    if (mro != NULL) {
        assert(PyTuple_CheckExact(mro));
        if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
            // XXX tp_mro can probably be statically allocated for each
            // static builtin type.
            assert(initial);
            assert(self->tp_mro == NULL);
            /* Other checks are done via set_tp_bases. */
            _Py_SetImmortal(mro);
        }
        else {
            PyUnstable_Object_EnableDeferredRefcount(mro);
        }
    }
    pinned_mutexes_t pinned;
    if (!initial) {
        type_lock_prevent_release(&pinned);
        types_stop_world();
    }
    if (self->tp_mro != mro) {
        mro_epoch_advance(self);
    }
    self->tp_mro = mro;
    if (!initial) {
        types_start_world();
        type_lock_allow_release(&pinned);
    }
}

static inline void
clear_tp_mro(PyTypeObject *self, int final)
{
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        if (final) {
            if (self->tp_mro != NULL) {
                if (PyTuple_GET_SIZE(self->tp_mro) == 0) {
                    Py_CLEAR(self->tp_mro);
                }
                else {
                    assert(_Py_IsImmortal(self->tp_mro));
                    _Py_ClearImmortal(self->tp_mro);
                }
            }
        }
        return;
    }
    if (self->tp_mro != NULL) {
        mro_epoch_advance(self);
    }
    Py_CLEAR(self->tp_mro);
}


static PyObject *
init_tp_subclasses(PyTypeObject *self)
{
    assert(Py_REFCNT(self) > 0);
    Py_INCREF(self);
    PyObject *result = NULL;
    for (;;) {
        PyObject *candidate = NULL;
        if (!PyErr_Occurred()) {
            candidate = PyDict_New();
        }
        if (candidate != NULL && !PyErr_Occurred()) {
            if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
                managed_static_type_state *state = _PyStaticType_GetState(
                    _PyInterpreterState_GET(), self);
                if (state->tp_subclasses == NULL) {
                    state->tp_subclasses = candidate;
                    candidate = NULL;
                }
            }
            else if (self->tp_subclasses == NULL) {
                self->tp_subclasses = candidate;
                candidate = NULL;
            }
        }
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(candidate);
        _PyValue_RestoreException(saved);
        if (PyErr_Occurred()) {
            break;
        }
        /* A losing candidate's retirement can replace the installed map. */
        if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
            managed_static_type_state *state = _PyStaticType_GetState(
                _PyInterpreterState_GET(), self);
            result = state->tp_subclasses;
        }
        else {
            result = (PyObject *)self->tp_subclasses;
        }
        if (result != NULL) {
            break;
        }
    }
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(self);
    _PyValue_RestoreException(saved);
    return PyErr_Occurred() ? NULL : result;
}

static void
clear_tp_subclasses(PyTypeObject *self)
{
    /* Delete the dictionary to save memory. _PyStaticType_Dealloc()
       callers also test if tp_subclasses is NULL to check if a static type
       has no subclass. */
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, self);
        Py_CLEAR(state->tp_subclasses);
        return;
    }
    Py_CLEAR(self->tp_subclasses);
}

static inline PyObject *
lookup_tp_subclasses(PyTypeObject *self)
{
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, self);
        assert(state != NULL);
        return state->tp_subclasses;
    }
    return (PyObject *)self->tp_subclasses;
}

PyObject *
_PyType_LookupSubclasses(PyTypeObject *self)
{
    return lookup_tp_subclasses(self);
}

PyObject *
_PyType_InitSubclasses(PyTypeObject *self)
{
    PyObject *existing = lookup_tp_subclasses(self);
    if (existing != NULL) {
        return existing;
    }
    return init_tp_subclasses(self);
}

int
_PyType_HasSubclasses(PyTypeObject *self)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN
        // XXX _PyStaticType_GetState() should never return NULL.
        && _PyStaticType_GetState(interp, self) == NULL)
    {
        return 0;
    }
    if (lookup_tp_subclasses(self) == NULL) {
        return 0;
    }
    return 1;
}

static int
get_subclasses_unlocked(PyTypeObject *self, PyObject *list)
{
    ASSERT_TYPE_LOCK_HELD();
    assert(PyList_GET_SIZE(list) == 0);
    if (PyErr_Occurred()) {
        return -1;
    }
    PyObject *subclasses = lookup_tp_subclasses(self);
    if (subclasses == NULL) {
        return 0;
    }
    assert(PyDict_CheckExact(subclasses));
    if (PyDict_GET_SIZE(subclasses) > ((PyListObject *)list)->allocated) {
        return 1;
    }

    /* Capture is pure: neither the map cursor nor its borrows cross a
     * callout. The map keeps each temporary weakref close nonfinal. */
    Py_ssize_t pos = 0;
    PyValue ref;
    int found;
    while ((found = _PyDict_NextValue(subclasses, &pos, NULL, &ref, NULL)) > 0) {
        assert(PyValue_IsObject(ref));
        PyTypeObject *child = type_from_ref(PyValue_AsPyObjectBorrow(ref));
        PyValue_CLOSE(ref);
        if (child != NULL) {
            Py_ssize_t size = PyList_GET_SIZE(list);
            PyList_SET_VALUE(list, size,
                             PyValue_FromPyObjectSteal((PyObject *)child));
            Py_SET_SIZE(list, size + 1);
        }
    }
    return found < 0 || PyErr_Occurred() ? -1 : 0;
}

PyObject*
_PyType_GetSubclasses(PyTypeObject *self)
{
    assert(Py_REFCNT(self) > 0);
    Py_INCREF(self);
    PyObject *list = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    for (;;) {
        Py_ssize_t capacity;
        BEGIN_TYPE_LOCK();
        PyObject *map = lookup_tp_subclasses(self);
        capacity = map == NULL ? 0 : PyDict_GET_SIZE(map);
        END_TYPE_LOCK();
        list = PyList_New(capacity);
        if (list != NULL) {
            /* Only initialized child owners are logically visible during
             * any later allocation, retry or cleanup. */
            Py_SET_SIZE(list, 0);
        }
        if (list == NULL || PyErr_Occurred()) {
            break;
        }
        int status;
        BEGIN_TYPE_LOCK();
        status = get_subclasses_unlocked(self, list);
        END_TYPE_LOCK();
        if (status <= 0 || PyErr_Occurred()) {
            break;
        }
        PyObject *saved = PyErr_GetRaisedException();
        Py_CLEAR(list);
        _PyValue_RestoreException(saved);
        if (PyErr_Occurred()) {
            break;
        }
    }

done:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(self);
    _PyValue_RestoreException(saved);
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_CLEAR(list);
        _PyValue_RestoreException(saved);
    }
    return list;
}

/* end accessors for objects stored on PyTypeObject */


/*
 * finds the beginning of the docstring's introspection signature.
 * if present, returns a pointer pointing to the first '('.
 * otherwise returns NULL.
 *
 * doesn't guarantee that the signature is valid, only that it
 * has a valid prefix.  (the signature must also pass skip_signature.)
 */
static const char *
find_signature(const char *name, const char *doc)
{
    const char *dot;
    size_t length;

    if (!doc)
        return NULL;

    assert(name != NULL);

    /* for dotted names like classes, only use the last component */
    dot = strrchr(name, '.');
    if (dot)
        name = dot + 1;

    length = strlen(name);
    if (strncmp(doc, name, length))
        return NULL;
    doc += length;
    if (*doc != '(')
        return NULL;
    return doc;
}

#define SIGNATURE_END_MARKER         ")\n--\n\n"
#define SIGNATURE_END_MARKER_LENGTH  6
/*
 * skips past the end of the docstring's introspection signature.
 * (assumes doc starts with a valid signature prefix.)
 */
static const char *
skip_signature(const char *doc)
{
    while (*doc) {
        if ((*doc == *SIGNATURE_END_MARKER) &&
            !strncmp(doc, SIGNATURE_END_MARKER, SIGNATURE_END_MARKER_LENGTH))
            return doc + SIGNATURE_END_MARKER_LENGTH;
        if ((*doc == '\n') && (doc[1] == '\n'))
            return NULL;
        doc++;
    }
    return NULL;
}

int
_PyType_CheckConsistency(PyTypeObject *type)
{
#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG((PyObject *)type, Py_STRINGIFY(expr)); } } while (0)

    CHECK(!_PyObject_IsFreed((PyObject *)type));

    if (!(type->tp_flags & Py_TPFLAGS_READY)) {
        /* don't check static types before PyType_Ready() */
        return 1;
    }

    CHECK(Py_REFCNT(type) >= 1);
    CHECK(PyType_Check(type));

    CHECK(!is_readying(type));
    CHECK(lookup_tp_dict(type) != NULL);

    if (type->tp_flags & Py_TPFLAGS_HAVE_GC) {
        // bpo-44263: tp_traverse is required if Py_TPFLAGS_HAVE_GC is set.
        // Note: tp_clear is optional.
        CHECK(type->tp_traverse != NULL);
    }

    if (type->tp_flags & Py_TPFLAGS_DISALLOW_INSTANTIATION) {
        CHECK(type->tp_new == NULL);
        CHECK(PyDict_Contains(lookup_tp_dict(type), &_Py_ID(__new__)) == 0);
    }

    return 1;
#undef CHECK
}

static const char *
_PyType_DocWithoutSignature(const char *name, const char *internal_doc)
{
    const char *doc = find_signature(name, internal_doc);

    if (doc) {
        doc = skip_signature(doc);
        if (doc)
            return doc;
        }
    return internal_doc;
}

PyObject *
_PyType_GetDocFromInternalDoc(const char *name, const char *internal_doc)
{
    const char *doc = _PyType_DocWithoutSignature(name, internal_doc);

    if (!doc || *doc == '\0') {
        Py_RETURN_NONE;
    }

    return PyUnicode_FromString(doc);
}

static const char *
signature_from_flags(int flags)
{
    switch (flags & ~METH_COEXIST) {
        case METH_NOARGS:
            return "($self, /)";
        case METH_NOARGS|METH_CLASS:
            return "($type, /)";
        case METH_NOARGS|METH_STATIC:
            return "()";
        case METH_O:
            return "($self, object, /)";
        case METH_O|METH_CLASS:
            return "($type, object, /)";
        case METH_O|METH_STATIC:
            return "(object, /)";
        default:
            return NULL;
    }
}

PyObject *
_PyType_GetTextSignatureFromInternalDoc(const char *name, const char *internal_doc, int flags)
{
    const char *start = find_signature(name, internal_doc);
    const char *end;

    if (start)
        end = skip_signature(start);
    else
        end = NULL;
    if (!end) {
        start = signature_from_flags(flags);
        if (start) {
            return PyUnicode_FromString(start);
        }
        Py_RETURN_NONE;
    }

    /* back "end" up until it points just past the final ')' */
    end -= SIGNATURE_END_MARKER_LENGTH - 1;
    assert((end - start) >= 2); /* should be "()" at least */
    assert(end[-1] == ')');
    assert(end[0] == '\n');
    return PyUnicode_FromStringAndSize(start, end - start);
}


unsigned int
PyType_ClearCache(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return NEXT_VERSION_TAG(interp) - 1;
}


void
_PyTypes_Fini(PyInterpreterState *interp)
{
    // All the managed static types should have been finalized already.
    assert(interp->types.for_extensions.num_initialized == 0);
    for (size_t i = 0; i < _Py_MAX_MANAGED_STATIC_EXT_TYPES; i++) {
        assert(interp->types.for_extensions.initialized[i].type == NULL);
    }
    assert(interp->types.builtins.num_initialized == 0);
    for (size_t i = 0; i < _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES; i++) {
        assert(interp->types.builtins.initialized[i].type == NULL);
    }
}


int
PyType_AddWatcher(PyType_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    // start at 1, 0 is reserved for cpython optimizer
    for (int i = 1; i < TYPE_MAX_WATCHERS; i++) {
        if (!interp->type_watchers[i]) {
            interp->type_watchers[i] = callback;
            return i;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "no more type watcher IDs available");
    return -1;
}

static inline int
validate_watcher_id(PyInterpreterState *interp, int watcher_id)
{
    if (watcher_id < 0 || watcher_id >= TYPE_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "Invalid type watcher ID %d", watcher_id);
        return -1;
    }
    if (!interp->type_watchers[watcher_id]) {
        PyErr_Format(PyExc_ValueError, "No type watcher set for ID %d", watcher_id);
        return -1;
    }
    return 0;
}

int
PyType_ClearWatcher(int watcher_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id) < 0) {
        return -1;
    }
    interp->type_watchers[watcher_id] = NULL;
    return 0;
}

static int assign_version_tag(PyInterpreterState *interp, PyTypeObject *type);

int
PyType_Watch(int watcher_id, PyObject* obj)
{
    if (!PyType_Check(obj)) {
        PyErr_SetString(PyExc_ValueError, "Cannot watch non-type");
        return -1;
    }
    PyTypeObject *type = (PyTypeObject *)obj;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id) < 0) {
        return -1;
    }
    // ensure we will get a callback on the next modification
    BEGIN_TYPE_LOCK();
    assign_version_tag(interp, type);
    type->tp_watched |= (1 << watcher_id);
    END_TYPE_LOCK();
    return 0;
}

int
PyType_Unwatch(int watcher_id, PyObject* obj)
{
    if (!PyType_Check(obj)) {
        PyErr_SetString(PyExc_ValueError, "Cannot watch non-type");
        return -1;
    }
    PyTypeObject *type = (PyTypeObject *)obj;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id)) {
        return -1;
    }
    type->tp_watched &= ~(1 << watcher_id);
    return 0;
}

static void
set_version_unlocked(PyTypeObject *tp, unsigned int version)
{
    assert(version == 0 || (tp->tp_versions_used != _Py_ATTR_CACHE_UNUSED));
#ifndef Py_GIL_DISABLED
    PyInterpreterState *interp = _PyInterpreterState_GET();
    // lookup the old version and set to null
    if (tp->tp_version_tag != 0) {
        PyTypeObject **slot =
            interp->types.type_version_cache
            + (tp->tp_version_tag % TYPE_VERSION_CACHE_SIZE);
        *slot = NULL;
    }
    if (version) {
        tp->tp_versions_used++;
    }
#else
    if (version) {
        _Py_atomic_add_uint16(&tp->tp_versions_used, 1);
    }
#endif
    FT_ATOMIC_STORE_UINT_RELAXED(tp->tp_version_tag, version);
#ifndef Py_GIL_DISABLED
    if (version != 0) {
        PyTypeObject **slot =
            interp->types.type_version_cache
            + (version % TYPE_VERSION_CACHE_SIZE);
        *slot = tp;
    }
#endif
}


static void
type_mutation_close(PyObject **slot)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyObject *owner = *slot;
    *slot = NULL;
    Py_XDECREF(owner);
    _PyValue_RestoreException(saved);
}


static PyObject *
type_mutation_get_dict(PyTypeObject *type)
{
    PyObject *dict = PyType_GetDict(type);
    if (dict == NULL && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_SystemError, "type has no dictionary");
    }
    return dict;
}

int
_PyTypeMutation_Begin(_PyTypeMutation *scope, PyTypeObject *type)
{
    assert(!scope->active);
    if (PyErr_Occurred()) {
        return -1;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->types.mutation_depth == UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "type mutation nesting overflow");
        return -1;
    }
    assert(Py_REFCNT(type) > 0);
    Py_INCREF(type);
    interp->types.mutation_depth++;
    scope->interp = interp;
    scope->type = type;
    scope->active = 1;
    return 0;
}

int
_PyTypeMutation_End(_PyTypeMutation *scope)
{
    if (!scope->active) {
        return PyErr_Occurred() ? -1 : 0;
    }
    PyInterpreterState *interp = scope->interp;
    PyTypeObject *type = scope->type;
    scope->type = NULL;
    scope->active = 0;
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(type);
    _PyValue_RestoreException(saved);
    // No type access or callback follows reopening this scope's admission.
    assert(interp->types.mutation_depth > 0);
    interp->types.mutation_depth--;
    return PyErr_Occurred() ? -1 : 0;
}

// The cursor exists only in this allocation-free capture. Weakref and child
// temporary decrements are nonfinal because their existing owners remain live.
static PyTypeObject *
next_nonzero_subclass(PyTypeObject *type)
{
    assert(!PyErr_Occurred());
    PyObject *subclasses = lookup_tp_subclasses(type);
    if (subclasses == NULL) {
        return NULL;
    }
    assert(PyDict_CheckExact(subclasses));
    Py_ssize_t pos = 0;
    PyValue ref;
    int found;
    while ((found = _PyDict_NextValue(subclasses, &pos, NULL, &ref, NULL)) > 0) {
        assert(PyValue_IsObject(ref));
        PyTypeObject *child = type_from_ref(PyValue_AsPyObjectBorrow(ref));
        PyValue_CLOSE(ref);
        if (child != NULL) {
            if (child->tp_version_tag != 0) {
                return child;
            }
            Py_DECREF(child);
        }
    }
    assert(found == 0 && !PyErr_Occurred());
    return NULL;
}

static void
type_modified_recursive(PyTypeObject *type)
{
    ASSERT_NEW_TYPE_OR_LOCKED(type);
    assert(_PyInterpreterState_GET()->types.mutation_depth != 0);
    if (type->tp_version_tag == 0) {
        return;
    }
    assert(!(type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));
    PyObject *saved = PyErr_GetRaisedException();
    for (;;) {
        PyTypeObject *child = next_nonzero_subclass(type);
        if (child == NULL) {
            break;
        }
        type_modified_recursive(child);
        PyObject *error = PyErr_GetRaisedException();
        Py_DECREF(child);
        _PyValue_RestoreException(error);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
        // Reacquire the current graph from its beginning after every callout.
    }
    if (type->tp_version_tag == 0) {
        // A child's callback may have completed this parent's invalidation.
        // Do not emit a second notification without an intervening admission.
        _PyValue_RestoreException(saved);
        return;
    }
    set_version_unlocked(type, 0);
    if (PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        PyHeapTypeObject *heap = (PyHeapTypeObject *)type;
        FT_ATOMIC_STORE_PTR_RELAXED(heap->_spec_cache.getitem, NULL);
        FT_ATOMIC_STORE_PTR_RELAXED(heap->_spec_cache.init, NULL);
        FT_ATOMIC_STORE_UINT32_RELAXED(heap->_spec_cache.getitem_version, 0);
    }
    struct type_cache *retired = _PyTypeCache_Detach(type);
    // Watcher failures retain their documented unraisable policy. Preserve a
    // prior error independently of reporting, which can itself reenter.
    int bits = type->tp_watched;
    for (int i = 0; bits != 0; i++, bits >>= 1) {
        assert(i < TYPE_MAX_WATCHERS);
        if (bits & 1) {
            PyType_WatchCallback cb = _PyInterpreterState_GET()->type_watchers[i];
            if (cb != NULL && cb(type) < 0) {
                PyErr_FormatUnraisable(
                    "Exception ignored in type watcher callback #%d for %R",
                    i, type);
            }
            _PyValue_RestoreException(saved);
            saved = PyErr_GetRaisedException();
        }
    }
    _PyTypeCache_Retire(retired);
    _PyValue_RestoreException(saved);
}

void
_PyType_Modified_Unlocked(PyTypeObject *type)
{
    // Invalidation is allowed while unwinding an existing error. Individual
    // writers still stop on that error instead of publishing another write.
    PyObject *saved = PyErr_GetRaisedException();
    _PyTypeMutation scope = {0};
    if (_PyTypeMutation_Begin(&scope, type) == 0) {
        type_modified_recursive(type);
    }
    _PyTypeMutation_End(&scope);
    _PyValue_RestoreException(saved);
}

void
PyType_Modified(PyTypeObject *type)
{
    BEGIN_TYPE_LOCK();
    _PyType_Modified_Unlocked(type);
    END_TYPE_LOCK();
}

static int
is_subtype_with_mro(PyObject *a_mro, PyTypeObject *a, PyTypeObject *b);

// Check if the `mro` method on `type` is overridden, i.e.,
// `type(tp).mro is not type.mro`.
static int
has_custom_mro(PyTypeObject *tp)
{
    _PyCValue c_ref1, c_ref2;
    PyThreadState *tstate = _PyThreadState_GET();
    _PyThreadState_PushCValue(tstate, &c_ref1);
    _PyThreadState_PushCValue(tstate, &c_ref2);

    _PyType_LookupValueAndVersion(Py_TYPE(tp), &_Py_ID(mro), &c_ref1.ref);
    _PyType_LookupValueAndVersion(&PyType_Type, &_Py_ID(mro), &c_ref2.ref);

    int custom = !PyValue_Is(c_ref1.ref, c_ref2.ref);

    _PyThreadState_PopCValue(tstate, &c_ref2);
    _PyThreadState_PopCValue(tstate, &c_ref1);
    return custom;
}

static void
type_mro_modified(PyTypeObject *type, PyObject *bases)
{
    /*
       Check that all base classes or elements of the MRO of type are
       able to be cached.  This function is called after the base
       classes or mro of the type are altered.

       Unset HAVE_VERSION_TAG and VALID_VERSION_TAG if the type
       has a custom MRO that includes a type which is not officially
       super type, or if the type implements its own mro() method.

       Called from mro_internal, which will subsequently be called on
       each subclass when their mro is recursively updated.
     */
    PyObject *receiver_owner = Py_NewRef((PyObject *)type);
    PyObject *bases_owner = Py_NewRef(bases);
    PyTypeObject *receiver = (PyTypeObject *)receiver_owner;
    Py_ssize_t i, n;

    ASSERT_TYPE_LOCK_HELD();
    if (!Py_IS_TYPE(receiver, &PyType_Type) && has_custom_mro(receiver)) {
        goto clear;
    }
    n = PyTuple_GET_SIZE(bases_owner);
    for (i = 0; i < n; i++) {
        PyValue base_value = PyTuple_GET_VALUE(bases_owner, i);
        assert(PyValue_IsObject(base_value));
        PyObject *base_object = PyValue_AsPyObjectBorrow(base_value);
        assert(PyType_Check(base_object));
        PyTypeObject *cls = (PyTypeObject *)base_object;

        if (cls->tp_versions_used >= _Py_ATTR_CACHE_UNUSED) {
            goto clear;
        }

        if (!is_subtype_with_mro(lookup_tp_mro(receiver), receiver, cls)) {
            goto clear;
        }
    }
    goto done;

 clear:
    assert(!(receiver->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));
    set_version_unlocked(receiver, 0);  /* 0 is not a valid version tag */
    _PyTypeCache_Invalidate(receiver);
    receiver->tp_versions_used = _Py_ATTR_CACHE_UNUSED;
    if (PyType_HasFeature(receiver, Py_TPFLAGS_HEAPTYPE)) {
        // This field *must* be invalidated if the type is modified (see the
        // comment on struct _specialization_cache):
        FT_ATOMIC_STORE_PTR_RELAXED(
            ((PyHeapTypeObject *)receiver)->_spec_cache.getitem, NULL);
    }
 done:
    type_mutation_close(&bases_owner);
    type_mutation_close(&receiver_owner);
}

/*
An optimizer that reads a type version from an inline cache needs the type object itself,
for example to install watchers to understand when it changes.

So we add a global cache from type version to borrowed references of type objects.

This is similar to func_version_cache.
*/

void
_PyType_SetVersion(PyTypeObject *tp, unsigned int version)
{
    BEGIN_TYPE_LOCK();
    set_version_unlocked(tp, version);
    END_TYPE_LOCK();
}

PyTypeObject *
_PyType_LookupByVersion(unsigned int version)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->types.lookup_closed) {
        _PyType_AbortClosedLookup();
    }
    if (interp->types.mutation_depth != 0 || version == 0) {
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    return NULL;
#else
    PyTypeObject *type;
    switch (version) {
        case _Py_TYPE_VERSION_INT: type = &PyLong_Type; break;
        case _Py_TYPE_VERSION_FLOAT: type = &PyFloat_Type; break;
        case _Py_TYPE_VERSION_LIST: type = &PyList_Type; break;
        case _Py_TYPE_VERSION_TUPLE: type = &PyTuple_Type; break;
        case _Py_TYPE_VERSION_STR: type = &PyUnicode_Type; break;
        case _Py_TYPE_VERSION_SET: type = &PySet_Type; break;
        case _Py_TYPE_VERSION_FROZEN_SET: type = &PyFrozenSet_Type; break;
        case _Py_TYPE_VERSION_DICT: type = &PyDict_Type; break;
        case _Py_TYPE_VERSION_BYTEARRAY: type = &PyByteArray_Type; break;
        case _Py_TYPE_VERSION_BYTES: type = &PyBytes_Type; break;
        case _Py_TYPE_VERSION_COMPLEX: type = &PyComplex_Type; break;
        case _Py_TYPE_VERSION_FROZENDICT: type = &PyFrozenDict_Type; break;
        default:
            type = interp->types.type_version_cache[version % TYPE_VERSION_CACHE_SIZE];
            break;
    }
    return type != NULL && _PyType_MatchesVersion(interp, type, version)
        ? type : NULL;
#endif
}

#define MAX_VERSIONS_PER_CLASS 1000
#if _Py_ATTR_CACHE_UNUSED < MAX_VERSIONS_PER_CLASS
#error "_Py_ATTR_CACHE_UNUSED must be bigger than max"
#endif

static inline unsigned int
next_global_version_tag(void)
{
    unsigned int old;
    do {
        old = _Py_atomic_load_uint_relaxed(&_PyRuntime.types.next_version_tag);
        if (old >= _Py_MAX_GLOBAL_TYPE_VERSION_TAG) {
            return 0;
        }
    } while (!_Py_atomic_compare_exchange_uint(&_PyRuntime.types.next_version_tag, &old, old + 1));
    return old + 1;
}

static int
assign_version_tag(PyInterpreterState *interp, PyTypeObject *type)
{
    ASSERT_TYPE_LOCK_HELD();

    /* Ensure that the tp_version_tag is valid.
     * To respect the invariant, this must first be done on all super classes.
     * Return 0 if this cannot be done, 1 if tp_version_tag is set.
    */
    if (!type_cache_admitted(interp, type)) {
        return 0;
    }
    if (type->tp_version_tag != 0) {
        return 1;
    }
    if (!_PyType_HasFeature(type, Py_TPFLAGS_READY)) {
        return 0;
    }
    if (type->tp_versions_used >= MAX_VERSIONS_PER_CLASS) {
        /* (this includes `tp_versions_used == _Py_ATTR_CACHE_UNUSED`) */
        return 0;
    }

    PyObject *bases = lookup_tp_bases(type);
    Py_ssize_t n = PyTuple_GET_SIZE(bases);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *b = _PyTuple_GET_OBJECT(bases, i);
        if (!assign_version_tag(interp, _PyType_CAST(b))) {
            return 0;
        }
    }
    if (type->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) {
        /* static types */
        unsigned int next_version_tag = next_global_version_tag();
        if (next_version_tag == 0) {
            /* We have run out of version numbers */
            return 0;
        }
        set_version_unlocked(type, next_version_tag);
        assert (type->tp_version_tag <= _Py_MAX_GLOBAL_TYPE_VERSION_TAG);
    }
    else {
        /* heap types */
        if (NEXT_VERSION_TAG(interp) == 0) {
            /* We have run out of version numbers */
            return 0;
        }
        set_version_unlocked(type, NEXT_VERSION_TAG(interp)++);
        assert (type->tp_version_tag != 0);
    }
    return 1;
}

int PyUnstable_Type_AssignVersionTag(PyTypeObject *type)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    int assigned;
    BEGIN_TYPE_LOCK();
    assigned = assign_version_tag(interp, type);
    END_TYPE_LOCK();
    return assigned;
}


static PyMemberDef type_members[] = {
    {"__basicsize__", Py_T_PYSSIZET, offsetof(PyTypeObject,tp_basicsize),Py_READONLY},
    {"__itemsize__", Py_T_PYSSIZET, offsetof(PyTypeObject, tp_itemsize), Py_READONLY},
    {"__flags__", Py_T_ULONG, offsetof(PyTypeObject, tp_flags), Py_READONLY},
    /* Note that this value is misleading for static builtin types,
       since the memory at this offset will always be NULL. */
    {"__weakrefoffset__", Py_T_PYSSIZET,
     offsetof(PyTypeObject, tp_weaklistoffset), Py_READONLY},
    {"__base__", _Py_T_OBJECT, offsetof(PyTypeObject, tp_base), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET,
     offsetof(PyTypeObject, tp_dictoffset), Py_READONLY},
    {0}
};

static int
check_set_special_type_attr(PyTypeObject *type, PyObject *value, const char *name)
{
    if (!value) {
        PyErr_Format(PyExc_TypeError,
                     "cannot delete '%s' attribute of type '%s'",
                     name, type->tp_name);
        return 0;
    }
    if (_PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE)) {
        PyErr_Format(PyExc_TypeError,
                     "cannot set '%s' attribute of immutable type '%s'",
                     name, type->tp_name);
        return 0;
    }

    if (PySys_Audit("object.__setattr__", "OsO",
                    type, name, value) < 0) {
        return 0;
    }

    return 1;
}

const char *
_PyType_Name(PyTypeObject *type)
{
    assert(type->tp_name != NULL);
    const char *s = strrchr(type->tp_name, '.');
    if (s == NULL) {
        s = type->tp_name;
    }
    else {
        s++;
    }
    return s;
}

static PyObject *
type_name(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        PyHeapTypeObject* et = (PyHeapTypeObject*)type;
        return Py_NewRef(et->ht_name);
    }
    else {
        return PyUnicode_FromString(_PyType_Name(type));
    }
}

static PyObject *
type_qualname(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        PyHeapTypeObject* et = (PyHeapTypeObject*)type;
        return Py_NewRef(et->ht_qualname);
    }
    else {
        return PyUnicode_FromString(_PyType_Name(type));
    }
}

static int
type_set_name(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    const char *tp_name;
    Py_ssize_t name_size;

    if (!check_set_special_type_attr(type, value, "__name__"))
        return -1;
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "can only assign string to %s.__name__, not '%s'",
                     type->tp_name, Py_TYPE(value)->tp_name);
        return -1;
    }

    tp_name = PyUnicode_AsUTF8AndSize(value, &name_size);
    if (tp_name == NULL)
        return -1;
    if (strlen(tp_name) != (size_t)name_size) {
        PyErr_SetString(PyExc_ValueError,
                        "type name must not contain null characters");
        return -1;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    type->tp_name = tp_name;
    PyObject *old_name = ((PyHeapTypeObject*)type)->ht_name;
    ((PyHeapTypeObject*)type)->ht_name = Py_NewRef(value);
    _PyEval_StartTheWorld(interp);
    Py_DECREF(old_name);
    return 0;
}

static int
type_set_qualname(PyObject *tp, PyObject *value, void *context)
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyHeapTypeObject* et;

    if (!check_set_special_type_attr(type, value, "__qualname__"))
        return -1;
    if (!PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "can only assign string to %s.__qualname__, not '%s'",
                     type->tp_name, Py_TYPE(value)->tp_name);
        return -1;
    }

    et = (PyHeapTypeObject*)type;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    PyObject *old_qualname = et->ht_qualname;
    et->ht_qualname = Py_NewRef(value);
    _PyEval_StartTheWorld(interp);
    Py_DECREF(old_qualname);
    return 0;
}

static PyObject *
type_module(PyTypeObject *type)
{
    PyObject *mod;
    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        PyObject *dict = lookup_tp_dict(type);
        if (PyDict_GetItemRef(dict, &_Py_ID(__module__), &mod) == 0) {
            PyErr_Format(PyExc_AttributeError, "__module__");
        }
    }
    else {
        const char *s = strrchr(type->tp_name, '.');
        if (s != NULL) {
            mod = PyUnicode_FromStringAndSize(
                type->tp_name, (Py_ssize_t)(s - type->tp_name));
            if (mod != NULL) {
                PyInterpreterState *interp = _PyInterpreterState_GET();
                _PyUnicode_InternMortal(interp, &mod);
            }
        }
        else {
            mod = &_Py_ID(builtins);
        }
    }
    return mod;
}

static inline PyObject *
type_get_module(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    return type_module(type);
}

static int
type_set_module(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    int result = -1;
    Py_XINCREF(value);
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (!check_set_special_type_attr(type, value, "__module__") || PyErr_Occurred()) {
        goto done;
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (PyDict_Pop(dict, &_Py_ID(__firstlineno__), NULL) < 0 || PyErr_Occurred()) {
        goto done;
    }
    result = PyDict_SetItem(dict, &_Py_ID(__module__), value);
done:
    type_mutation_close(&dict);
    type_mutation_close(&value);
    if (_PyTypeMutation_End(&scope) < 0) {
        return -1;
    }
    return result;
}


PyObject *
_PyType_GetFullyQualifiedName(PyTypeObject *type, char sep)
{
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        return PyUnicode_FromString(type->tp_name);
    }

    PyObject *qualname = type_qualname((PyObject *)type, NULL);
    if (qualname == NULL) {
        return NULL;
    }

    PyObject *module = type_module(type);
    if (module == NULL) {
        Py_DECREF(qualname);
        return NULL;
    }

    PyObject *result;
    if (PyUnicode_Check(module)
        && !_PyUnicode_Equal(module, &_Py_ID(builtins))
        && !_PyUnicode_Equal(module, &_Py_ID(__main__)))
    {
        result = PyUnicode_FromFormat("%U%c%U", module, sep, qualname);
    }
    else {
        result = Py_NewRef(qualname);
    }
    Py_DECREF(module);
    Py_DECREF(qualname);
    return result;
}

PyObject *
PyType_GetFullyQualifiedName(PyTypeObject *type)
{
    return _PyType_GetFullyQualifiedName(type, '.');
}

static PyObject *
type_abstractmethods(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyObject *res = NULL;
    /* type itself has an __abstractmethods__ descriptor (this). Don't return
       that. */
    if (type == &PyType_Type) {
        PyErr_SetObject(PyExc_AttributeError, &_Py_ID(__abstractmethods__));
    }
    else {
        PyObject *dict = lookup_tp_dict(type);
        if (PyDict_GetItemRef(dict, &_Py_ID(__abstractmethods__), &res) == 0) {
            PyErr_SetObject(PyExc_AttributeError, &_Py_ID(__abstractmethods__));
        }
    }
    return res;
}

static int
type_set_abstractmethods(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    int result = -1;
    int abstract = 0;
    Py_XINCREF(value);
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (_PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE)) {
        PyErr_Format(PyExc_TypeError,
                     "cannot set '__abstractmethods__' attribute of immutable type '%s'",
                     type->tp_name);
        goto done;
    }
    if (value != NULL) {
        abstract = PyObject_IsTrue(value);
        if (abstract < 0 || PyErr_Occurred()) {
            goto done;
        }
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (value != NULL) {
        result = PyDict_SetItem(dict, &_Py_ID(__abstractmethods__), value);
    }
    else {
        result = PyDict_Pop(dict, &_Py_ID(__abstractmethods__), NULL);
        if (result == 0 && !PyErr_Occurred()) {
            PyErr_SetObject(PyExc_AttributeError, &_Py_ID(__abstractmethods__));
        }
    }
    if (result < 0 || PyErr_Occurred()) {
        goto done;
    }
    pinned_mutexes_t pinned;
    BEGIN_TYPE_LOCK();
    type_lock_prevent_release(&pinned);
    types_stop_world();
    if (abstract) {
        type_add_flags(type, Py_TPFLAGS_IS_ABSTRACT);
    }
    else {
        type_clear_flags(type, Py_TPFLAGS_IS_ABSTRACT);
    }
    types_start_world();
    type_lock_allow_release(&pinned);
    END_TYPE_LOCK();
    result = 0;
done:
    type_mutation_close(&dict);
    type_mutation_close(&value);
    return _PyTypeMutation_End(&scope) < 0 ? -1 : result;
}

static PyObject *
type_get_bases(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyObject *bases = _PyType_GetBases(type);
    if (bases == NULL) {
        Py_RETURN_NONE;
    }
    return bases;
}

static PyObject *
type_get_mro(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyObject *mro = lookup_tp_mro(type);
    if (mro == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(mro);
}

static PyTypeObject *find_best_base(PyObject *);
static int mro_internal(PyTypeObject *, int, PyObject **);
static int type_is_subtype_base_chain(PyTypeObject *, PyTypeObject *);
static int compatible_for_assignment(PyTypeObject *, PyTypeObject *, const char *, int);
static int add_subclass(PyTypeObject*, PyTypeObject*);
static int add_all_subclasses(PyTypeObject *type, PyObject *bases);
static void remove_subclass(PyTypeObject *, PyTypeObject *);
static void remove_all_subclasses(PyTypeObject *type, PyObject *bases);
static int update_all_slots(PyTypeObject *);

typedef int (*update_callback)(PyTypeObject *, void *);
static int update_subclasses(PyTypeObject *type, PyObject *attr_name,
                             update_callback callback, void *data);
static int recurse_down_subclasses(PyTypeObject *type, PyObject *name,
                                   update_callback callback, void *data);

// Compute tp_mro for this type and all of its subclasses.  This
// is called after __bases__ is assigned to an existing type.
static int
mro_hierarchy_for_complete_type(PyTypeObject *type, PyObject *temp)
{
    ASSERT_TYPE_LOCK_HELD();

    PyObject *old_mro;
    int res = mro_internal(type, 0, &old_mro);
    if (res <= 0) {
        /* error / reentrance */
        return res;
    }
    PyObject *new_mro = lookup_tp_mro(type);
    assert(new_mro != NULL);

    PyObject *tuple;
    if (old_mro != NULL) {
        tuple = PyTuple_Pack(3, type, new_mro, old_mro);
    }
    else {
        tuple = _PyTuple_FromPair((PyObject *)type, new_mro);
    }

    if (tuple != NULL) {
        res = PyList_Append(temp, tuple);
    }
    else {
        res = -1;
    }
    Py_XDECREF(tuple);

    if (res < 0) {
        set_tp_mro(type, old_mro, 0);
        Py_DECREF(new_mro);
        return -1;
    }
    Py_XDECREF(old_mro);

    // Avoid creating an empty list if there is no subclass
    if (_PyType_HasSubclasses(type)) {
        /* Obtain a copy of subclasses list to iterate over.

           Otherwise type->tp_subclasses might be altered
           in the middle of the loop, for example, through a custom mro(),
           by invoking type_set_bases on some subclass of the type
           which in turn calls remove_subclass/add_subclass on this type.

           Finally, this makes things simple avoiding the need to deal
           with dictionary iterators and weak references.
        */
        PyObject *subclasses = _PyType_GetSubclasses(type);
        if (subclasses == NULL) {
            return -1;
        }

        Py_ssize_t n = PyList_GET_SIZE(subclasses);
        for (Py_ssize_t i = 0; i < n; i++) {
            PyValue child = PyList_GetValue(subclasses, i);
            PyTypeObject *subclass = NULL;
            if (PyValue_IsNull(child) || PyErr_Occurred()) {
                res = -1;
            }
            else if (!PyValue_IsObject(child)) {
                PyErr_SetString(PyExc_SystemError,
                                "subclass list contains a non-object value");
                res = -1;
            }
            else {
                PyObject *child_object = PyValue_AsPyObjectBorrow(child);
                if (!PyType_Check(child_object)) {
                    PyErr_SetString(PyExc_SystemError,
                                    "subclass list contains a non-type object");
                    res = -1;
                }
                else {
                    subclass = (PyTypeObject *)child_object;
                    res = mro_hierarchy_for_complete_type(subclass, temp);
                }
            }
            PyObject *saved = PyErr_GetRaisedException();
            PyValue retired_child = child;
            child = PyValue_NULL;
            PyValue_XCLOSE(retired_child);
            _PyValue_RestoreException(saved);
            if (PyErr_Occurred()) {
                res = -1;
            }
            if (res < 0) {
                break;
            }
        }
        type_mutation_close(&subclasses);
        if (PyErr_Occurred()) {
            res = -1;
        }
    }

    return res;
}

static int
type_check_new_bases(PyTypeObject *type, PyObject *new_bases, PyTypeObject **best_base)
{
    // Check arguments, this is re-entrant due to the PySys_Audit() call
    if (!check_set_special_type_attr(type, new_bases, "__bases__")) {
        return -1;
    }
    assert(new_bases != NULL);

    if (!PyTuple_Check(new_bases)) {
        PyErr_Format(PyExc_TypeError,
             "can only assign tuple to %s.__bases__, not %s",
                 type->tp_name, Py_TYPE(new_bases)->tp_name);
        return -1;
    }
    if (PyTuple_GET_SIZE(new_bases) == 0) {
        PyErr_Format(PyExc_TypeError,
             "can only assign non-empty tuple to %s.__bases__, not ()",
                 type->tp_name);
        return -1;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(new_bases);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyTuple_GET_VALUE(new_bases, i);
        if (!PyValue_IsObject(item) ||
            !PyType_Check(PyValue_AsPyObjectBorrow(item))) {
            PyObject *receiver_owner = Py_NewRef((PyObject *)type);
            PyObject *receiver_name_owner = NULL;
            if (PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
                receiver_name_owner = Py_NewRef(((PyHeapTypeObject *)type)->ht_name);
            }
            PyTypeObject *actual_type = PyValue_TYPE(item);
            PyObject *actual_type_owner = Py_NewRef((PyObject *)actual_type);
            PyObject *actual_name_owner = NULL;
            if (PyType_HasFeature(actual_type, Py_TPFLAGS_HEAPTYPE)) {
                actual_name_owner = Py_NewRef(((PyHeapTypeObject *)actual_type)->ht_name);
            }
            assert(receiver_owner != NULL);
            assert(actual_type_owner != NULL);
            assert(!PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE) ||
                   receiver_name_owner != NULL);
            assert(!PyType_HasFeature(actual_type, Py_TPFLAGS_HEAPTYPE) ||
                   actual_name_owner != NULL);
            const char *receiver_name = type->tp_name;
            const char *actual_name = actual_type->tp_name;
            PyErr_Format(PyExc_TypeError,
                         "%s.__bases__ must be tuple of classes, not '%s'",
                         receiver_name, actual_name);
            type_mutation_close(&actual_name_owner);
            type_mutation_close(&actual_type_owner);
            type_mutation_close(&receiver_name_owner);
            type_mutation_close(&receiver_owner);
            return -1;
        }
        PyObject *ob = PyValue_AsPyObjectBorrow(item);
        PyTypeObject *base = (PyTypeObject *)ob;

        if (is_subtype_with_mro(lookup_tp_mro(base), base, type) ||
            /* In case of reentering here again through a custom mro()
               the above check is not enough since it relies on
               base->tp_mro which would gonna be updated inside
               mro_internal only upon returning from the mro().

               However, base->tp_base has already been assigned (see
               below), which in turn may cause an inheritance cycle
               through tp_base chain.  And this is definitely
               not what you want to ever happen.  */
            (lookup_tp_mro(base) != NULL
             && type_is_subtype_base_chain(base, type)))
        {
            PyErr_SetString(PyExc_TypeError,
                            "a __bases__ item causes an inheritance cycle");
            return -1;
        }
    }

    // Compute the new MRO and the new base class
    *best_base = find_best_base(new_bases);
    if (*best_base == NULL)
        return -1;

    if (!compatible_for_assignment(type, *best_base, "__bases__", 0)) {
        return -1;
    }

    return 0;
}

static int
type_set_bases_unlocked(PyTypeObject *type, PyObject *new_bases, PyTypeObject *best_base)
{
    ASSERT_TYPE_LOCK_HELD();

    Py_ssize_t n;
    PyObject *old_bases = lookup_tp_bases(type);
    assert(old_bases != NULL);
    PyTypeObject *old_base = type->tp_base;
    pinned_mutexes_t pinned;

    type_lock_prevent_release(&pinned);
    types_stop_world();
    set_tp_bases(type, Py_NewRef(new_bases), 0);
    if (type->tp_base != best_base) {
        mro_epoch_advance(type);
    }
    type->tp_base = (PyTypeObject *)Py_NewRef(best_base);
    types_start_world();
    type_lock_allow_release(&pinned);

    PyObject *temp = PyList_New(0);
    if (temp == NULL) {
        goto bail;
    }
    if (mro_hierarchy_for_complete_type(type, temp) < 0) {
        goto undo;
    }
    Py_DECREF(temp);

    /* Take no action in case if type->tp_bases has been replaced
       through reentrance.  */
    int res;
    if (lookup_tp_bases(type) == new_bases) {
        /* any base that was in __bases__ but now isn't, we
           need to remove |type| from its tp_subclasses.
           conversely, any class now in __bases__ that wasn't
           needs to have |type| added to its subclasses. */

        /* for now, sod that: just remove from all old_bases,
           add to all new_bases */
        remove_all_subclasses(type, old_bases);
        res = add_all_subclasses(type, new_bases);
        if (update_all_slots(type) < 0) {
            goto bail;
        }
        /* Clear the VALID_VERSION flag of 'type' and all its subclasses. */
        _PyType_Modified_Unlocked(type);
    }
    else {
        res = 0;
    }

    RARE_EVENT_INC(set_bases);
    Py_DECREF(old_bases);
    Py_DECREF(old_base);

    assert(_PyType_CheckConsistency(type));
    return res;

  undo:
    assert(PyList_CheckExact(temp));
    n = PyList_GET_SIZE(temp);
    for (Py_ssize_t i = n - 1; i >= 0; i--) {
        PyValue record_value = PyList_GET_VALUE(temp, i);
        assert(PyValue_IsObject(record_value));
        PyObject *record_owner = Py_NewRef(
            PyValue_AsPyObjectBorrow(record_value));
        assert(PyTuple_CheckExact(record_owner));
        Py_ssize_t record_size = PyTuple_GET_SIZE(record_owner);
        assert(record_size == 2 || record_size == 3);
        PyValue cls_value = PyTuple_GET_VALUE(record_owner, 0);
        PyValue new_mro_value = PyTuple_GET_VALUE(record_owner, 1);
        assert(PyValue_IsObject(cls_value));
        assert(PyValue_IsObject(new_mro_value));
        PyObject *cls_object = PyValue_AsPyObjectBorrow(cls_value);
        PyObject *new_mro = PyValue_AsPyObjectBorrow(new_mro_value);
        assert(PyType_Check(cls_object));
        assert(PyTuple_CheckExact(new_mro));
        PyTypeObject *cls = (PyTypeObject *)cls_object;
        PyObject *old_mro = NULL;
        if (record_size == 3) {
            PyValue old_mro_value = PyTuple_GET_VALUE(record_owner, 2);
            assert(PyValue_IsObject(old_mro_value));
            old_mro = PyValue_AsPyObjectBorrow(old_mro_value);
            assert(PyTuple_CheckExact(old_mro));
        }
        /* Do not rollback if cls has a newer version of MRO.  */
        if (lookup_tp_mro(cls) == new_mro) {
            set_tp_mro(cls, Py_XNewRef(old_mro), 0);
            Py_DECREF(new_mro);
        }
        type_mutation_close(&record_owner);
    }
    Py_DECREF(temp);

  bail:
    if (lookup_tp_bases(type) == new_bases) {
        assert(type->tp_base == best_base);

        type_lock_prevent_release(&pinned);
        types_stop_world();
        set_tp_bases(type, old_bases, 0);
        if (type->tp_base != old_base) {
            mro_epoch_advance(type);
        }
        type->tp_base = old_base;
        types_start_world();
        type_lock_allow_release(&pinned);

        Py_DECREF(new_bases);
        Py_DECREF(best_base);
    }
    else {
        Py_DECREF(old_bases);
        Py_DECREF(old_base);
    }

    assert(_PyType_CheckConsistency(type));
    return -1;
}

static int
type_set_bases(PyObject *tp, PyObject *new_bases, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyTypeObject *best_base;
    int res;
    BEGIN_TYPE_LOCK();
    res = type_check_new_bases(type, new_bases, &best_base);
    if (res == 0) {
        res = type_set_bases_unlocked(type, new_bases, best_base);
    }
    END_TYPE_LOCK();
    return res;
}

static PyObject *
type_dict(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyObject *dict = lookup_tp_dict(type);
    if (dict == NULL) {
        Py_RETURN_NONE;
    }
    return PyDictProxy_New(dict);
}

static PyObject *
type_get_doc(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    PyObject *result;
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE) && type->tp_doc != NULL) {
        return _PyType_GetDocFromInternalDoc(type->tp_name, type->tp_doc);
    }
    PyObject *dict = lookup_tp_dict(type);
    if (PyDict_GetItemRef(dict, &_Py_ID(__doc__), &result) == 0) {
        result = Py_NewRef(Py_None);
    }
    else if (result) {
        descrgetfunc descr_get = Py_TYPE(result)->tp_descr_get;
        if (descr_get) {
            Py_SETREF(result, descr_get(result, NULL, (PyObject *)type));
        }
    }
    return result;
}

static PyObject *
type_get_text_signature(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    return _PyType_GetTextSignatureFromInternalDoc(type->tp_name, type->tp_doc, 0);
}

static int
type_set_doc(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    int result = -1;
    Py_XINCREF(value);
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (!check_set_special_type_attr(type, value, "__doc__") || PyErr_Occurred()) {
        goto done;
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = PyDict_SetItem(dict, &_Py_ID(__doc__), value);
done:
    type_mutation_close(&dict);
    type_mutation_close(&value);
    if (_PyTypeMutation_End(&scope) < 0) {
        return -1;
    }
    return result;
}

static PyObject *
type_get_annotate(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    PyObject *annotate = NULL;
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        PyErr_Format(PyExc_AttributeError, "type object '%s' has no attribute '__annotate__'", type->tp_name);
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (PyDict_GetItemRef(dict, &_Py_ID(__annotate__), &annotate) < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (annotate == NULL &&
        (PyDict_GetItemRef(dict, &_Py_ID(__annotate_func__), &annotate) < 0 || PyErr_Occurred())) {
        goto done;
    }
    if (annotate != NULL) {
        descrgetfunc get = Py_TYPE(annotate)->tp_descr_get;
        if (get != NULL) {
            PyObject *selected = annotate;
            annotate = get(selected, NULL, tp);
            type_mutation_close(&selected);
        }
    }
    else {
        annotate = Py_NewRef(Py_None);
        PyType_Modified(type);
        if (PyErr_Occurred()) {
            goto done;
        }
        type_mutation_close(&dict);
        if (PyErr_Occurred()) {
            goto done;
        }
        dict = type_mutation_get_dict(type);
        if (dict == NULL || PyErr_Occurred() ||
            PyDict_SetItem(dict, &_Py_ID(__annotate_func__), annotate) < 0) {
            goto done;
        }
    }
done:
    type_mutation_close(&dict);
    if (PyErr_Occurred()) {
        type_mutation_close(&annotate);
    }
    _PyTypeMutation_End(&scope);
    if (PyErr_Occurred()) {
        type_mutation_close(&annotate);
    }
    return annotate;
}

static int
type_set_annotate(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    int result = -1;
    Py_XINCREF(value);
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete __annotate__ attribute");
        goto done;
    }
    if (_PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE)) {
        PyErr_Format(PyExc_TypeError,
                     "cannot set '__annotate__' attribute of immutable type '%s'",
                     type->tp_name);
        goto done;
    }
    if (!Py_IsNone(value) && !PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "__annotate__ must be callable or None");
        goto done;
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred() ||
        PyDict_SetItem(dict, &_Py_ID(__annotate_func__), value) < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (!Py_IsNone(value) &&
        (PyDict_Pop(dict, &_Py_ID(__annotations_cache__), NULL) < 0 || PyErr_Occurred())) {
        goto done;
    }
    result = 0;
done:
    type_mutation_close(&dict);
    type_mutation_close(&value);
    return _PyTypeMutation_End(&scope) < 0 ? -1 : result;
}

static PyObject *
type_get_annotations(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    PyObject *annotations = NULL;
    PyObject *annotate = NULL;
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        PyErr_Format(PyExc_AttributeError, "type object '%s' has no attribute '__annotations__'", type->tp_name);
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred() ||
        PyDict_GetItemRef(dict, &_Py_ID(__annotations__), &annotations) < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (annotations == NULL &&
        (PyDict_GetItemRef(dict, &_Py_ID(__annotations_cache__), &annotations) < 0 || PyErr_Occurred())) {
        goto done;
    }
    if (annotations != NULL) {
        descrgetfunc get = Py_TYPE(annotations)->tp_descr_get;
        if (get != NULL) {
            PyObject *selected = annotations;
            annotations = get(selected, NULL, tp);
            type_mutation_close(&selected);
        }
    }
    else {
        annotate = PyObject_GetAttrString(tp, "__annotate__");
        if (annotate == NULL || PyErr_Occurred()) {
            goto done;
        }
        if (PyCallable_Check(annotate)) {
            annotations = _PyObject_CallOneArg(annotate, _PyLong_GetOne());
            if (annotations == NULL || PyErr_Occurred()) {
                goto done;
            }
            if (!PyDict_Check(annotations)) {
                PyErr_Format(PyExc_TypeError,
                             "__annotate__() must return a dict, not %T", annotations);
                goto done;
            }
        }
        else {
            annotations = PyDict_New();
            if (annotations == NULL || PyErr_Occurred()) {
                goto done;
            }
        }
        // The annotate callback ran once. Its old dict view is not a write
        // target after reentry; invalidate, retire it and acquire current state.
        PyType_Modified(type);
        if (PyErr_Occurred()) {
            goto done;
        }
        type_mutation_close(&dict);
        if (PyErr_Occurred()) {
            goto done;
        }
        dict = type_mutation_get_dict(type);
        if (dict == NULL || PyErr_Occurred() ||
            PyDict_SetItem(dict, &_Py_ID(__annotations_cache__), annotations) < 0) {
            goto done;
        }
    }
done:
    type_mutation_close(&annotate);
    type_mutation_close(&dict);
    if (PyErr_Occurred()) {
        type_mutation_close(&annotations);
    }
    _PyTypeMutation_End(&scope);
    if (PyErr_Occurred()) {
        type_mutation_close(&annotations);
    }
    return annotations;
}

static int
type_set_annotations(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    int result = -1;
    Py_XINCREF(value);
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (_PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE)) {
        PyErr_Format(PyExc_TypeError,
                     "cannot set '__annotations__' attribute of immutable type '%s'",
                     type->tp_name);
        goto done;
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    int explicit = PyDict_Contains(dict, &_Py_ID(__annotations__));
    if (explicit < 0 || PyErr_Occurred()) {
        goto done;
    }
    PyObject *key = explicit ? &_Py_ID(__annotations__) : &_Py_ID(__annotations_cache__);
    if (value != NULL) {
        result = PyDict_SetItem(dict, key, value);
    }
    else {
        result = PyDict_Pop(dict, key, NULL);
        if (result == 0 && !PyErr_Occurred()) {
            PyErr_SetString(PyExc_AttributeError, "__annotations__");
        }
    }
    if (result < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (explicit &&
        (PyDict_Pop(dict, &_Py_ID(__annotations_cache__), NULL) < 0 || PyErr_Occurred())) {
        goto done;
    }
    if (PyDict_Pop(dict, &_Py_ID(__annotate_func__), NULL) < 0 || PyErr_Occurred() ||
        PyDict_Pop(dict, &_Py_ID(__annotate__), NULL) < 0 || PyErr_Occurred()) {
        goto done;
    }
    result = 0;
done:
    type_mutation_close(&dict);
    type_mutation_close(&value);
    return _PyTypeMutation_End(&scope) < 0 ? -1 : result;
}

static PyObject *
type_get_type_params(PyObject *tp, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    if (type == &PyType_Type) {
        return PyTuple_New(0);
    }

    PyObject *params;
    if (PyDict_GetItemRef(lookup_tp_dict(type), &_Py_ID(__type_params__), &params) == 0) {
        return PyTuple_New(0);
    }
    return params;
}

static int
type_set_type_params(PyObject *tp, PyObject *value, void *Py_UNUSED(closure))
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    _PyTypeMutation scope = {0};
    PyObject *dict = NULL;
    int result = -1;
    Py_XINCREF(value);
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    if (!check_set_special_type_attr(type, value, "__type_params__") || PyErr_Occurred()) {
        goto done;
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = PyDict_SetItem(dict, &_Py_ID(__type_params__), value);
done:
    type_mutation_close(&dict);
    type_mutation_close(&value);
    if (_PyTypeMutation_End(&scope) < 0) {
        return -1;
    }
    return result;
}


/*[clinic input]
type.__instancecheck__ -> bool

    instance: object
    /

Check if an object is an instance.
[clinic start generated code]*/

static int
type___instancecheck___impl(PyTypeObject *self, PyObject *instance)
/*[clinic end generated code: output=08b6bf5f591c3618 input=cdbfeaee82c01a0f]*/
{
    return _PyObject_RealIsInstance(instance, (PyObject *)self);
}

/*[clinic input]
type.__subclasscheck__ -> bool

    subclass: object
    /

Check if a class is a subclass.
[clinic start generated code]*/

static int
type___subclasscheck___impl(PyTypeObject *self, PyObject *subclass)
/*[clinic end generated code: output=97a4e51694500941 input=071b2ca9e03355f4]*/
{
    return _PyObject_RealIsSubclass(subclass, (PyObject *)self);
}


static PyGetSetDef type_getsets[] = {
    {"__name__", type_name, type_set_name, NULL},
    {"__qualname__", type_qualname, type_set_qualname, NULL},
    {"__bases__", type_get_bases, type_set_bases, NULL},
    {"__mro__", type_get_mro, NULL, NULL},
    {"__module__", type_get_module, type_set_module, NULL},
    {"__abstractmethods__", type_abstractmethods,
     type_set_abstractmethods, NULL},
    {"__dict__",  type_dict,  NULL, NULL},
    {"__doc__", type_get_doc, type_set_doc, NULL},
    {"__text_signature__", type_get_text_signature, NULL, NULL},
    {"__annotations__", type_get_annotations, type_set_annotations, NULL},
    {"__annotate__", type_get_annotate, type_set_annotate, NULL},
    {"__type_params__", type_get_type_params, type_set_type_params, NULL},
    {0}
};

static PyObject *
type_repr(PyObject *self)
{
    PyTypeObject *type = PyTypeObject_CAST(self);
    if (type->tp_name == NULL) {
        // type_repr() called before the type is fully initialized
        // by PyType_Ready().
        return PyUnicode_FromFormat("<class at %p>", type);
    }

    PyObject *mod = type_module(type);
    if (mod == NULL) {
        PyErr_Clear();
    }
    else if (!PyUnicode_Check(mod)) {
        Py_CLEAR(mod);
    }

    PyObject *name = type_qualname(self, NULL);
    if (name == NULL) {
        Py_XDECREF(mod);
        return NULL;
    }

    PyObject *result;
    if (mod != NULL && !_PyUnicode_Equal(mod, &_Py_ID(builtins))) {
        result = PyUnicode_FromFormat("<class '%U.%U'>", mod, name);
    }
    else {
        result = PyUnicode_FromFormat("<class '%s'>", type->tp_name);
    }
    Py_XDECREF(mod);
    Py_DECREF(name);

    return result;
}

static PyObject *
type_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyTypeObject *type = PyTypeObject_CAST(self);
    PyObject *obj;
    PyThreadState *tstate = _PyThreadState_GET();

#ifdef Py_DEBUG
    /* type_call() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* Special case: type(x) should return Py_TYPE(x) */
    /* We only want type itself to accept the one-argument form (#27157) */
    if (type == &PyType_Type) {
        assert(args != NULL && PyTuple_Check(args));
        assert(kwds == NULL || PyDict_Check(kwds));
        Py_ssize_t nargs = PyTuple_GET_SIZE(args);

        if (nargs == 1 && (kwds == NULL || !PyDict_GET_SIZE(kwds))) {
            PyValue argument = PyTuple_GET_VALUE(args, 0);
            assert(!PyValue_IsNull(argument));
            return Py_NewRef((PyObject *)PyValue_TYPE(argument));
        }

        /* SF bug 475327 -- if that didn't trigger, we need 3
           arguments. But PyArg_ParseTuple in type_new may give
           a msg saying type() needs exactly 3. */
        if (nargs != 3) {
            PyErr_SetString(PyExc_TypeError,
                            "type() takes 1 or 3 arguments");
            return NULL;
        }
    }

    if (type->tp_new == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "cannot create '%s' instances", type->tp_name);
        return NULL;
    }

    /* One borrow scope covers tp_new and tp_init: both parse args. */
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    obj = type->tp_new(type, args, kwds);
    obj = _Py_CheckFunctionResult(tstate, (PyObject*)type, obj, NULL);
    if (obj == NULL) {
        goto done;
    }

    /* If the returned object is not an instance of type,
       it won't be initialized. */
    if (!PyObject_TypeCheck(obj, type)) {
        goto done;
    }

    type = Py_TYPE(obj);
    if (type->tp_init != NULL) {
        int res = type->tp_init(obj, args, kwds);
        if (res < 0) {
            assert(_PyErr_Occurred(tstate));
            Py_SETREF(obj, NULL);
        }
        else {
            assert(!_PyErr_Occurred(tstate));
        }
    }
done:
    _PyBorrowScope_Exit(tstate, mark);
    return obj;
}

PyObject *
_PyType_NewManagedObject(PyTypeObject *type)
{
    assert(type->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    assert(_PyType_IS_GC(type));
    assert(type->tp_new == PyBaseObject_Type.tp_new);
    assert(type->tp_alloc == PyType_GenericAlloc);
    assert(type->tp_itemsize == 0);
    PyObject *obj = PyType_GenericAlloc(type, 0);
    if (obj == NULL) {
        return PyErr_NoMemory();
    }
    return obj;
}

static void
init_value_slots(PyObject *obj, PyTypeObject *type, Py_ssize_t nitems)
{
    /* Zero-filled memory is not the absent-value encoding. Initialize
       declared value fields before the instance can become observable. */
    for (PyTypeObject *base = type; base != NULL; base = base->tp_base) {
        for (PyMemberDef *mp = base->tp_members;
             mp != NULL && mp->name != NULL; mp++) {
            if (mp->type == _Py_T_VALUE || mp->type == _Py_T_VALUE_EX) {
                assert(!(mp->flags & Py_RELATIVE_OFFSET));
                char *addr = (char *)obj + mp->offset;
                if (mp->flags & _Py_AFTER_ITEMS) {
                    addr += _Py_SIZE_ROUND_UP(nitems * type->tp_itemsize,
                                             SIZEOF_VOID_P);
                }
                *(PyValue *)addr = PyValue_NULL;
            }
        }
    }
}

PyObject *
_PyType_AllocNoTrack(PyTypeObject *type, Py_ssize_t nitems)
{
    PyObject *obj;
    /* The +1 on nitems is needed for most types but not all. We could save a
     * bit of space by allocating one less item in certain cases, depending on
     * the type. However, given the extra complexity (e.g. an additional type
     * flag to indicate when that is safe) it does not seem worth the memory
     * savings. An example type that doesn't need the +1 is a subclass of
     * tuple. See GH-100659 and GH-81381. */
    size_t size = _PyObject_VAR_SIZE(type, nitems+1);

    const size_t presize = _PyType_PreHeaderSize(type);
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        assert(type->tp_itemsize == 0);
        size += _PyInlineValuesSize(type);
    }
    char *alloc = _PyObject_MallocWithType(type, size + presize);
    if (alloc  == NULL) {
        return PyErr_NoMemory();
    }
    obj = (PyObject *)(alloc + presize);
    if (presize) {
        ((PyObject **)alloc)[0] = NULL;
        ((PyObject **)alloc)[1] = NULL;
    }
    if (PyType_IS_GC(type)) {
        _PyObject_GC_Link(obj);
    }
    // Zero out the object after the PyObject header. The header fields are
    // initialized by _PyObject_Init[Var]().
    memset((char *)obj + sizeof(PyObject), 0, size - sizeof(PyObject));

    /* Both managed inline accessors and CREATE observers need the type.
       _PyObject_Init still performs the sole type-reference increment. */
    Py_SET_TYPE(obj, type);
    if (type->tp_itemsize != 0) {
        /* The CREATE tracer runs inside _PyObject_InitVar, before its final
           size write. Its after-items member reads need the actual size. */
        Py_SET_SIZE((PyVarObject *)obj, nitems);
    }
    /* _PyObject_Init can notify the reference tracer. Use the explicit
       allocation size here, before the variable object header is initialized. */
    init_value_slots(obj, type, nitems);
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        _PyObject_InitInlineValues(obj, type);
    }
    if (type->tp_itemsize == 0) {
        _PyObject_Init(obj, type);
    }
    else {
        _PyObject_InitVar((PyVarObject *)obj, type, nitems);
    }
    return obj;
}

PyObject *
PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)
{
    PyObject *obj = _PyType_AllocNoTrack(type, nitems);
    if (obj == NULL) {
        return NULL;
    }

    if (_PyType_IS_GC(type)) {
        _PyObject_GC_TRACK(obj);
    }
    return obj;
}

PyObject *
PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return type->tp_alloc(type, 0);
}

/* Helpers for subtyping */

static inline PyMemberDef *
_PyHeapType_GET_MEMBERS(PyHeapTypeObject* type)
{
    return PyObject_GetItemData((PyObject *)type);
}

static int
traverse_slots(PyTypeObject *type, PyObject *self, visitproc visit, void *arg)
{
    Py_ssize_t i, n;
    PyMemberDef *mp;

    n = Py_SIZE(type);
    mp = _PyHeapType_GET_MEMBERS((PyHeapTypeObject *)type);
    for (i = 0; i < n; i++, mp++) {
        if (mp->type == _Py_T_VALUE || mp->type == _Py_T_VALUE_EX) {
            PyValue *slot = (PyValue *)_PyMember_GetOffset(self, mp);
            assert(PyValue_IsHeapSafe(*slot));
            _Py_VISIT_VALUE(*slot);
        }
        else if (mp->type == Py_T_OBJECT_EX) {
            void *addr = _PyMember_GetOffset(self, mp);
            PyObject *obj = *(PyObject **)addr;
            if (obj != NULL) {
                int err = visit(obj, arg);
                if (err)
                    return err;
            }
        }
    }
    return 0;
}

static int
subtype_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyTypeObject *type, *base;
    traverseproc basetraverse;

    /* Find the nearest base with a different tp_traverse,
       and traverse slots while we're at it */
    type = Py_TYPE(self);
    base = type;
    while ((basetraverse = base->tp_traverse) == subtype_traverse) {
        if (Py_SIZE(base)) {
            int err = traverse_slots(base, self, visit, arg);
            if (err)
                return err;
        }
        base = base->tp_base;
        assert(base);
    }

    if (type->tp_dictoffset != base->tp_dictoffset) {
        assert(base->tp_dictoffset == 0);
        if (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
            assert(type->tp_dictoffset == -1);
            int err = PyObject_VisitManagedDict(self, visit, arg);
            if (err) {
                return err;
            }
        }
        else {
            PyObject **dictptr = _PyObject_ComputedDictPointer(self);
            if (dictptr && *dictptr) {
                Py_VISIT(*dictptr);
            }
        }
    }

    if (type->tp_flags & Py_TPFLAGS_HEAPTYPE
        && (!basetraverse || !(base->tp_flags & Py_TPFLAGS_HEAPTYPE))) {
        /* For a heaptype, the instances count as references
           to the type.          Traverse the type so the collector
           can find cycles involving this link.
           Skip this visit if basetraverse belongs to a heap type: in that
           case, basetraverse will visit the type when we call it later.
           */
        Py_VISIT(type);
    }

    if (basetraverse)
        return basetraverse(self, visit, arg);
    return 0;
}

static int
collect_slot_error(PyObject **saved)
{
    int failed = PyErr_Occurred() != NULL;
    _PyValue_RestoreException(*saved);
    *saved = PyErr_GetRaisedException();
    return failed;
}

static int
clear_slots(PyTypeObject *type, PyObject *self)
{
    Py_ssize_t i, n;
    PyMemberDef *mp;

    PyObject *saved = PyErr_GetRaisedException();
    int failed = 0;
    n = Py_SIZE(type);
    mp = _PyHeapType_GET_MEMBERS((PyHeapTypeObject *)type);
    for (i = 0; i < n; i++, mp++) {
        if (mp->flags & Py_READONLY) {
            continue;
        }
        if (mp->type == _Py_T_VALUE || mp->type == _Py_T_VALUE_EX) {
            PyValue *addr = _PyMember_GetOffset(self, mp);
            PyValue previous = *addr;
            *addr = PyValue_NULL;
            PyValue_XCLOSE(previous);
            failed |= collect_slot_error(&saved);
        }
        else if (mp->type == Py_T_OBJECT_EX) {
            void *addr = _PyMember_GetOffset(self, mp);
            PyObject *obj = *(PyObject **)addr;
            if (obj != NULL) {
                *(PyObject **)addr = NULL;
                Py_DECREF(obj);
                failed |= collect_slot_error(&saved);
            }
        }
    }
    PyErr_SetRaisedException(saved);
    return failed ? -1 : 0;
}

static int
subtype_clear(PyObject *self)
{
    PyTypeObject *type, *base;
    inquiry baseclear;
    PyObject *saved = PyErr_GetRaisedException();
    int failed = 0;

    /* Find the nearest base with a different tp_clear
       and clear slots while we're at it */
    type = Py_TYPE(self);
    Py_INCREF(type);
    base = type;
    while ((baseclear = base->tp_clear) == subtype_clear) {
        if (Py_SIZE(base)) {
            failed |= clear_slots(base, self) < 0;
            failed |= collect_slot_error(&saved);
        }
        base = base->tp_base;
        assert(base);
    }

    /* Clear the instance dict (if any), to break cycles involving only
       __dict__ slots (as in the case 'self.__dict__ is self'). */
    if (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        if ((base->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
            PyObject_ClearManagedDict(self);
            failed |= collect_slot_error(&saved);
        }
        else {
            assert((base->tp_flags & Py_TPFLAGS_INLINE_VALUES) ==
                   (type->tp_flags & Py_TPFLAGS_INLINE_VALUES));
        }
    }
    else if (type->tp_dictoffset != base->tp_dictoffset) {
        PyObject **dictptr = _PyObject_ComputedDictPointer(self);
        if (dictptr && *dictptr) {
            Py_CLEAR(*dictptr);
            failed |= collect_slot_error(&saved);
        }
    }

    if (baseclear) {
        failed |= baseclear(self) < 0;
        failed |= collect_slot_error(&saved);
    }
    Py_DECREF(type);
    failed |= collect_slot_error(&saved);
    PyErr_SetRaisedException(saved);
    return failed ? -1 : 0;
}

static void
subtype_dealloc(PyObject *self)
{
    PyTypeObject *type, *base;
    destructor basedealloc;
    int has_finalizer;

    /* Extract the type; we expect it to be a heap type */
    type = Py_TYPE(self);
    _PyObject_ASSERT((PyObject *)type, type->tp_flags & Py_TPFLAGS_HEAPTYPE);

    /* Test whether the type has GC exactly once */

    if (!_PyType_IS_GC(type)) {
        /* A non GC dynamic type allows certain simplifications:
           there's no need to call clear_slots(), or DECREF the dict,
           or clear weakrefs. */

        /* Maybe call finalizer; exit early if resurrected */
        if (type->tp_finalize) {
            if (PyObject_CallFinalizerFromDealloc(self) < 0)
                return;
        }
        if (type->tp_del) {
            type->tp_del(self);
            if (Py_REFCNT(self) > 0) {
                return;
            }
        }

        /* Find the nearest base with a different tp_dealloc */
        base = type;
        while ((basedealloc = base->tp_dealloc) == subtype_dealloc) {
            base = base->tp_base;
            assert(base);
        }

        /* Extract the type again; tp_del may have changed it */
        type = Py_TYPE(self);

        // Don't read type memory after calling basedealloc() since basedealloc()
        // can deallocate the type and free its memory.
        int type_needs_decref = (type->tp_flags & Py_TPFLAGS_HEAPTYPE
                                 && !(base->tp_flags & Py_TPFLAGS_HEAPTYPE));

        assert((type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);

        /* Call the base tp_dealloc() */
        assert(basedealloc);
        basedealloc(self);

        /* Can't reference self beyond this point. It's possible tp_del switched
           our type from a HEAPTYPE to a non-HEAPTYPE, so be careful about
           reference counting. Only decref if the base type is not already a heap
           allocated type. Otherwise, basedealloc should have decref'd it already */
        if (type_needs_decref) {
            _Py_DECREF_TYPE(type);
        }

        /* Done */
        return;
    }

    /* We get here only if the type has GC */

    /* UnTrack and re-Track around the trashcan macro, alas */
    /* See explanation at end of function for full disclosure */
    PyObject_GC_UnTrack(self);

    /* Find the nearest base with a different tp_dealloc */
    base = type;
    while ((/*basedealloc =*/ base->tp_dealloc) == subtype_dealloc) {
        base = base->tp_base;
        assert(base);
    }

    has_finalizer = type->tp_finalize || type->tp_del;

    if (type->tp_finalize) {
        _PyObject_GC_TRACK(self);
        if (PyObject_CallFinalizerFromDealloc(self) < 0) {
            /* Resurrected */
            return;
        }
        _PyObject_GC_UNTRACK(self);
    }
    /*
      If we added a weaklist, we clear it. Do this *before* calling tp_del,
      clearing slots, or clearing the instance dict.

      GC tracking must be off at this point. weakref callbacks (if any, and
      whether directly here or indirectly in something we call) may trigger GC,
      and if self is tracked at that point, it will look like trash to GC and GC
      will try to delete self again.
    */
    if (type->tp_weaklistoffset && !base->tp_weaklistoffset) {
        PyObject_ClearWeakRefs(self);
    }

    if (type->tp_del) {
        _PyObject_GC_TRACK(self);
        type->tp_del(self);
        if (Py_REFCNT(self) > 0) {
            /* Resurrected */
            return;
        }
        _PyObject_GC_UNTRACK(self);
    }
    if (has_finalizer) {
        /* New weakrefs could be created during the finalizer call.
           If this occurs, clear them out without calling their
           finalizers since they might rely on part of the object
           being finalized that has already been destroyed. */
        if (type->tp_weaklistoffset && !base->tp_weaklistoffset) {
            _PyWeakref_ClearWeakRefsNoCallbacks(self);
        }
    }

    /*  Clear slots up to the nearest base with a different tp_dealloc */
    PyObject *slot_error = PyErr_GetRaisedException();
    PyTypeObject *slot_type = Py_TYPE(self);
    Py_INCREF(slot_type);
    type = slot_type;
    base = type;
    while ((basedealloc = base->tp_dealloc) == subtype_dealloc) {
        if (Py_SIZE(base)) {
            (void)clear_slots(base, self);
            (void)collect_slot_error(&slot_error);
        }
        base = base->tp_base;
        assert(base);
    }

    /* If we added a dict, DECREF it, or free inline values. */
    if (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        PyObject_ClearManagedDict(self);
        (void)collect_slot_error(&slot_error);
    }
    else if (type->tp_dictoffset && !base->tp_dictoffset) {
        PyObject **dictptr = _PyObject_ComputedDictPointer(self);
        if (dictptr != NULL) {
            PyObject *dict = *dictptr;
            if (dict != NULL) {
                *dictptr = NULL;
                Py_DECREF(dict);
                (void)collect_slot_error(&slot_error);
            }
        }
    }

    /* Extract the type again; tp_del may have changed it */
    type = Py_TYPE(self);

    /* Call the base tp_dealloc(); first retrack self if
     * basedealloc knows about gc.
     */
    if (_PyType_IS_GC(base)) {
        _PyObject_GC_TRACK(self);
    }

    // Don't read type memory after calling basedealloc() since basedealloc()
    // can deallocate the type and free its memory.
    int type_needs_decref = (type->tp_flags & Py_TPFLAGS_HEAPTYPE
                             && !(base->tp_flags & Py_TPFLAGS_HEAPTYPE));

    assert(basedealloc);
    basedealloc(self);
    (void)collect_slot_error(&slot_error);

    /* Can't reference self beyond this point. It's possible tp_del switched
       our type from a HEAPTYPE to a non-HEAPTYPE, so be careful about
       reference counting. Only decref if the base type is not already a heap
       allocated type. Otherwise, basedealloc should have decref'd it already */
    if (type_needs_decref) {
        _Py_DECREF_TYPE(type);
        (void)collect_slot_error(&slot_error);
    }
    Py_DECREF(slot_type);
    (void)collect_slot_error(&slot_error);
    PyErr_SetRaisedException(slot_error);
}

static PyTypeObject *solid_base(PyTypeObject *type);

/* type test with subclassing support */

static int
type_is_subtype_base_chain(PyTypeObject *a, PyTypeObject *b)
{
    do {
        if (a == b)
            return 1;
        a = a->tp_base;
    } while (a != NULL);

    return (b == &PyBaseObject_Type);
}

static int
is_subtype_with_mro(PyObject *a_mro, PyTypeObject *a, PyTypeObject *b)
{
    if (a_mro == NULL) {
        return type_is_subtype_base_chain(a, b);
    }
    assert(PyTuple_Check(a_mro));
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(a_mro); i++) {
        PyValue item = PyTuple_GET_VALUE(a_mro, i);
        if (PyValue_IsObject(item) &&
                PyValue_AsPyObjectBorrow(item) == (PyObject *)b) {
            return 1;
        }
    }
    return 0;
}

int
PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)
{
    return is_subtype_with_mro(a->tp_mro, a, b);
}

/* Routines to do a method lookup in the type without looking in the
   instance dictionary (so we can't use PyObject_GetAttr) but still
   binding it to the instance.

   Variants:

   - _PyObject_LookupSpecial() returns NULL without raising an exception
     when the _PyType_LookupRef() call fails;

   - lookup_maybe_method() and lookup_method() are internal routines similar
     to _PyObject_LookupSpecial(), but can return unbound PyFunction
     to avoid temporary method object. Pass self as first argument when
     unbound == 1.
*/

PyObject *
_PyObject_LookupSpecial(PyObject *self, PyObject *attr)
{
    PyObject *res;

    res = _PyType_LookupRef(Py_TYPE(self), attr);
    if (res != NULL) {
        descrgetfunc f;
        if ((f = Py_TYPE(res)->tp_descr_get) != NULL) {
            Py_SETREF(res, f(res, self, (PyObject *)(Py_TYPE(self))));
        }
    }
    return res;
}

// Lookup the method name `attr` on `*self`. On entry, `*method` is null.
// On exit, `*method` is the method object and `*self` is cleared if the
// method is bound.
// Return 1 on success, -1 on error, and 0 if the method is missing.
//
// Publish canonical owners in the caller's slots before invoking a
// descriptor; scoped object views keep its C protocol inputs alive.
int
_PyObject_LookupSpecialMethod(PyObject *attr, PyValue *method,
                              PyValue *self)
{
    assert(PyValue_IsNull(*method));
    assert(!PyValue_IsNull(*self));
    PyObject *attr_owner = NULL, *self_view = NULL, *method_view = NULL;
    PyObject *bound = NULL, *binding_type = NULL;
    PyValue found = PyValue_NULL;
    int rc = -1;
    if (PyErr_Occurred()) {
        return -1;
    }
    attr_owner = Py_NewRef(attr);
    self_view = PyValue_AsObject(*self);
    if (self_view == NULL || PyErr_Occurred()) {
        goto done;
    }
    (void)_PyType_LookupValueAndVersion(Py_TYPE(self_view), attr_owner, &found);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (PyValue_IsNull(found)) {
        rc = 0;
        goto done;
    }
    method_view = PyValue_AsObject(found);
    if (method_view == NULL || PyErr_Occurred()) {
        goto done;
    }
    /* Successful method/self outputs retain owned object views for native
       consumers, including WITH_EXCEPT_START; do not canonicalize them here. */
    *method = PyValue_FromPyObjectSteal(method_view);
    method_view = NULL;
    {
        PyValue retired = found;
        found = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        goto done;
    }
    PyObject *method_object = PyValue_AsPyObjectBorrow(*method);
    if (_PyType_HasFeature(Py_TYPE(method_object), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
        PyValue retired = *self;
        *self = PyValue_FromPyObjectSteal(self_view);
        self_view = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
        if (!PyErr_Occurred()) {
            rc = 1;
        }
        goto done;
    }
    descrgetfunc f = Py_TYPE(method_object)->tp_descr_get;
    if (f != NULL) {
        binding_type = Py_NewRef((PyObject *)Py_TYPE(self_view));
        bound = f(method_object, self_view, binding_type);
        if (bound != NULL) {
            PyValue retired = *method;
            *method = PyValue_FromPyObjectSteal(bound);
            bound = NULL;
            PyObject *saved = PyErr_GetRaisedException();
            PyValue_XCLOSE(retired);
            _PyValue_RestoreException(saved);
        }
        else {
            goto done;
        }
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    {
        PyValue retired = *self;
        *self = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    if (!PyErr_Occurred()) {
        rc = 1;
    }
done:
    {
        PyValue retired = found;
        found = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&bound);
    type_mutation_close(&method_view);
    type_mutation_close(&binding_type);
    type_mutation_close(&attr_owner);
    type_mutation_close(&self_view);
    if (rc < 0 || PyErr_Occurred()) {
        PyValue retired = *method;
        *method = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
        return -1;
    }
    return rc;
}

static int
lookup_method_ex(PyObject *self, PyObject *attr, PyValue *out,
                 int raise_attribute_error)
{
    assert(PyValue_IsNull(*out));
    PyObject *self_owner = NULL, *attr_owner = NULL;
    PyObject *view = NULL, *bound = NULL, *binding_type = NULL;
    PyObject *suppressed_error = NULL;
    int rc = -1;
    if (PyErr_Occurred()) {
        return -1;
    }
    self_owner = Py_NewRef(self);
    attr_owner = Py_NewRef(attr);
    (void)_PyType_LookupValueAndVersion(Py_TYPE(self_owner), attr_owner, out);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (PyValue_IsNull(*out)) {
        if (raise_attribute_error) {
            PyErr_SetObject(PyExc_AttributeError, attr_owner);
        }
        goto done;
    }
    view = PyValue_AsObject(*out);
    if (view == NULL || PyErr_Occurred()) {
        goto done;
    }
    {
        PyValue retired = *out;
        *out = PyValue_FromPyObjectSteal(view);
        view = NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        goto done;
    }
    PyObject *value = PyValue_AsPyObjectBorrow(*out);
    if (_PyType_HasFeature(Py_TYPE(value), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
        rc = 1;
        goto done;
    }
    descrgetfunc f = Py_TYPE(value)->tp_descr_get;
    if (f == NULL) {
        rc = 0;
        goto done;
    }
    binding_type = Py_NewRef((PyObject *)Py_TYPE(self_owner));
    bound = f(value, self_owner, binding_type);
    /* Suppress only the descriptor's actual NULL/AttributeError result.
       New cleanup errors still propagate after this deliberate suppression. */
    if (bound == NULL && !raise_attribute_error &&
        PyErr_ExceptionMatches(PyExc_AttributeError)) {
        suppressed_error = PyErr_GetRaisedException();
    }
    int had_bound = bound != NULL;
    {
        PyValue retired = *out;
        *out = PyValue_NULL;
        if (bound != NULL) {
            *out = PyValue_FromPyObjectSteal(bound);
            bound = NULL;
        }
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    if (had_bound && !PyErr_Occurred()) {
        rc = 0;
    }
done:
    type_mutation_close(&view);
    type_mutation_close(&bound);
    type_mutation_close(&suppressed_error);
    type_mutation_close(&binding_type);
    type_mutation_close(&attr_owner);
    type_mutation_close(&self_owner);
    if (rc < 0 || PyErr_Occurred()) {
        PyValue retired = *out;
        *out = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
        return -1;
    }
    return rc;
}

static int
lookup_maybe_method(PyObject *self, PyObject *attr, PyValue *out)
{
    return lookup_method_ex(self, attr, out, 0);
}

static int
lookup_method(PyObject *self, PyObject *attr, PyValue *out)
{
    return lookup_method_ex(self, attr, out, 1);
}


static inline PyObject*
vectorcall_unbound(PyThreadState *tstate, int unbound, PyObject *func,
                   PyObject *const *args, Py_ssize_t nargs)
{
    size_t nargsf = nargs;
    if (!unbound) {
        /* Skip self argument, freeing up args[0] to use for
         * PY_VECTORCALL_ARGUMENTS_OFFSET */
        args++;
        nargsf = nargsf - 1 + PY_VECTORCALL_ARGUMENTS_OFFSET;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_SLOT, func);
    return _PyObject_VectorcallTstate(tstate, func, args, nargsf, NULL);
}

static PyObject*
call_unbound_noarg(int unbound, PyObject *func, PyObject *self)
{
    if (unbound) {
        return PyObject_CallOneArg(func, self);
    }
    else {
        return _PyObject_CallNoArgs(func);
    }
}

// Call the method with the name `attr` on `self`. Returns NULL with an
// exception set if the method is missing or an error occurs.
static PyObject *
call_method_noarg(PyObject *self, PyObject *attr)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self_owner = NULL;
    PyObject *result = NULL;
    _PyCValue cref;
    int unbound = -1;
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(self);
    _PyThreadState_PushCValue(tstate, &cref);
    unbound = lookup_method(self_owner, attr, &cref.ref);
    if (PyErr_Occurred() || unbound < 0) {
        goto done;
    }
    assert(!PyValue_IsNull(cref.ref));
    assert(PyValue_IsPointerSpace(cref.ref));
    PyObject *func = PyValue_AsPyObjectBorrow(cref.ref);
    result = call_unbound_noarg(unbound, func, self_owner);
done:
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

static PyObject *
call_method(PyObject *self, PyObject *attr, PyObject *args, PyObject *kwds)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self_owner = NULL, *args_owner = NULL, *kwds_owner = NULL;
    PyObject *result = NULL;
    _PyCValue cref;
    int unbound = -1;
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(self);
    args_owner = Py_NewRef(args);
    kwds_owner = Py_XNewRef(kwds);
    _PyThreadState_PushCValue(tstate, &cref);
    unbound = lookup_method(self_owner, attr, &cref.ref);
    if (PyErr_Occurred() || unbound < 0) {
        goto done;
    }
    assert(!PyValue_IsNull(cref.ref));
    assert(PyValue_IsPointerSpace(cref.ref));
    PyObject *meth = PyValue_AsPyObjectBorrow(cref.ref);
    if (unbound) {
        result = _PyObject_Call_Prepend(tstate, meth, self_owner, args_owner,
                                        kwds_owner);
    }
    else {
        result = _PyObject_Call(tstate, meth, args_owner, kwds_owner);
    }
done:
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&kwds_owner);
    type_mutation_close(&args_owner);
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

/* A variation of PyObject_CallMethod* that uses lookup_method()
   instead of PyObject_GetAttrString().

   args is an argument vector of length nargs. The first element in this
   vector is the special object "self" which is used for the method lookup */
static PyObject *
vectorcall_method(PyObject *name, PyObject *const *args, Py_ssize_t nargs)
{
    assert(nargs >= 1);
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self_owner = NULL, *result = NULL;
    _PyCValue cref;
    int unbound = -1;
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(args[0]);
    _PyThreadState_PushCValue(tstate, &cref);
    unbound = lookup_method(self_owner, name, &cref.ref);
    if (PyErr_Occurred() || unbound < 0) {
        goto done;
    }
    assert(!PyValue_IsNull(cref.ref));
    assert(PyValue_IsPointerSpace(cref.ref));
    PyObject *func = PyValue_AsPyObjectBorrow(cref.ref);
    result = vectorcall_unbound(tstate, unbound, func, args, nargs);
done:
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

/* Clone of vectorcall_method() that returns NotImplemented
 * when the lookup fails. */
static PyObject *
vectorcall_maybe(PyThreadState *tstate, PyObject *name,
                 PyObject *const *args, Py_ssize_t nargs)
{
    assert(nargs >= 1);

    PyObject *self_owner = NULL;
    PyObject *result = NULL;
    _PyCValue cref = { .ref = PyValue_NULL };
    int unbound = -1;

    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(args[0]);
    _PyThreadState_PushCValue(tstate, &cref);
    unbound = lookup_maybe_method(self_owner, name, &cref.ref);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (unbound < 0) {
        result = Py_NewRef(Py_NotImplemented);
        goto done;
    }
    assert(!PyValue_IsNull(cref.ref));
    assert(PyValue_IsPointerSpace(cref.ref));
    PyObject *func = PyValue_AsPyObjectBorrow(cref.ref);
    result = vectorcall_unbound(tstate, unbound, func, args, nargs);

done:
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

/* Call the method with the name `attr` on `self`. Returns NULL if the
   method is missing or an error occurs.  No exception is set if
   the method is missing. If attr_is_none is not NULL, it is set to 1 if
   the attribute was found and was None, or 0 if it was not found. */
static PyObject *
maybe_call_special_no_args(PyObject *self, PyObject *attr, int *attr_is_none)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self_owner = NULL;
    PyObject *result = NULL;
    _PyCValue cref = { .ref = PyValue_NULL };
    int unbound = -1;

    if (attr_is_none != NULL) {
        *attr_is_none = 0;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(self);
    _PyThreadState_PushCValue(tstate, &cref);
    unbound = lookup_maybe_method(self_owner, attr, &cref.ref);
    if (PyErr_Occurred() || unbound < 0) {
        goto done;
    }
    assert(!PyValue_IsNull(cref.ref));
    assert(PyValue_IsPointerSpace(cref.ref));
    PyObject *func = PyValue_AsPyObjectBorrow(cref.ref);
    if (attr_is_none != NULL) {
        *attr_is_none = (func == Py_None);
    }
    if (func != Py_None || attr_is_none == NULL) {
        result = call_unbound_noarg(unbound, func, self_owner);
    }

done:
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        if (attr_is_none != NULL) {
            *attr_is_none = 0;
        }
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

static PyObject *
maybe_call_special_one_arg(PyObject *self, PyObject *attr, PyObject *arg,
                           int *attr_is_none)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self_owner = NULL;
    PyObject *arg_owner = NULL;
    PyObject *result = NULL;
    _PyCValue cref = { .ref = PyValue_NULL };
    int unbound = -1;

    if (attr_is_none != NULL) {
        *attr_is_none = 0;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(self);
    arg_owner = Py_NewRef(arg);
    _PyThreadState_PushCValue(tstate, &cref);
    unbound = lookup_maybe_method(self_owner, attr, &cref.ref);
    if (PyErr_Occurred() || unbound < 0) {
        goto done;
    }
    assert(!PyValue_IsNull(cref.ref));
    assert(PyValue_IsPointerSpace(cref.ref));
    PyObject *func = PyValue_AsPyObjectBorrow(cref.ref);
    if (attr_is_none != NULL) {
        *attr_is_none = (func == Py_None);
    }
    if (func != Py_None || attr_is_none == NULL) {
        PyObject *args[] = { self_owner, arg_owner };
        result = vectorcall_unbound(tstate, unbound, func, args, 2);
    }

done:
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&arg_owner);
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        if (attr_is_none != NULL) {
            *attr_is_none = 0;
        }
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

PyObject *
_PyObject_MaybeCallSpecialNoArgs(PyObject *self, PyObject *attr)
{
    return maybe_call_special_no_args(self, attr, NULL);
}

PyObject *
_PyObject_MaybeCallSpecialOneArg(PyObject *self, PyObject *attr, PyObject *arg)
{
    return maybe_call_special_one_arg(self, attr, arg, NULL);
}

/*
    Method resolution order algorithm C3 described in
    "A Monotonic Superclass Linearization for Dylan",
    by Kim Barrett, Bob Cassel, Paul Haahr,
    David A. Moon, Keith Playford, and P. Tucker Withington.
    (OOPSLA 1996)

    Some notes about the rules implied by C3:

    No duplicate bases.
    It isn't legal to repeat a class in a list of base classes.

    The next three properties are the 3 constraints in "C3".

    Local precedence order.
    If A precedes B in C's MRO, then A will precede B in the MRO of all
    subclasses of C.

    Monotonicity.
    The MRO of a class must be an extension without reordering of the
    MRO of each of its superclasses.

    Extended Precedence Graph (EPG).
    Linearization is consistent if there is a path in the EPG from
    each class to all its successors in the linearization.  See
    the paper for definition of EPG.
 */

static int
tail_contains(PyObject *tuple, Py_ssize_t whence, PyObject *o)
{
    for (Py_ssize_t j = whence + 1; j < PyTuple_GET_SIZE(tuple); j++) {
        PyValue item = PyTuple_GET_VALUE(tuple, j);
        if (PyValue_IsObject(item) && PyValue_AsPyObjectBorrow(item) == o) {
            return 1;
        }
    }
    return 0;
}

static PyObject *
class_name(PyObject *cls)
{
    PyObject *name;
    // Note that this is potentially re-entrant.
    if (PyObject_GetOptionalAttr(cls, &_Py_ID(__name__), &name) == 0) {
        name = PyObject_Repr(cls);
    }
    return name;
}

static int
check_duplicates(PyObject *tuple)
{
    PyObject *owner = Py_NewRef(tuple);
    PyObject *name = NULL;
    PyObject *error_type = NULL;
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(owner);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyTuple_GET_VALUE(owner, i);
        if (!PyValue_IsObject(item) ||
                !PyType_Check(PyValue_AsPyObjectBorrow(item))) {
            error_type = Py_NewRef(PyValue_TYPE(item));
            PyErr_Format(PyExc_TypeError, "bases must be types; got '%N'",
                         error_type);
            goto done;
        }
        for (Py_ssize_t j = i + 1; j < n; j++) {
            if (PyValue_Is(item, PyTuple_GET_VALUE(owner, j))) {
                name = class_name(PyValue_AsPyObjectBorrow(item));
                if (name == NULL || PyErr_Occurred()) {
                    goto done;
                }
                if (PyUnicode_Check(name)) {
                    PyErr_Format(PyExc_TypeError, "duplicate base class %U", name);
                }
                else {
                    PyErr_SetString(PyExc_TypeError, "duplicate base class");
                }
                goto done;
            }
        }
    }
    result = 0;
done:
    type_mutation_close(&name);
    type_mutation_close(&error_type);
    type_mutation_close(&owner);
    return PyErr_Occurred() ? -1 : result;
}

/* Raise a TypeError for an MRO order disagreement.

   It's hard to produce a good error message.  In the absence of better
   insight into error reporting, report the classes that were candidates
   to be put next into the MRO.  There is some conflict between the
   order in which they should be put in the MRO, but it's hard to
   diagnose what constraint can't be satisfied.
*/

static void
set_mro_error(PyObject **to_merge, Py_ssize_t to_merge_size, Py_ssize_t *remain)
{
    char buf[1000];
    int formatted = 0;
    PyObject *set = NULL;
    PyObject *keys = NULL;
    PyObject *name = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    set = PyDict_New();
    if (set == NULL || PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < to_merge_size; i++) {
        PyObject *tuple = to_merge[i];
        if (remain[i] < PyTuple_GET_SIZE(tuple)) {
            PyValue candidate = PyTuple_GET_VALUE(tuple, remain[i]);
            if (PyDict_SetValue(set, candidate, PyValue_FromObjectBorrow(Py_None)) < 0 ||
                    PyErr_Occurred()) {
                goto done;
            }
        }
    }
    keys = PyDict_Keys(set);
    if (keys == NULL || PyErr_Occurred()) {
        goto done;
    }
    Py_ssize_t n = PyList_GET_SIZE(keys);
    Py_ssize_t off = PyOS_snprintf(buf, sizeof(buf), "Cannot create a "
        "consistent method resolution order (MRO) for bases");
    for (Py_ssize_t i = 0; i < n && (size_t)off < sizeof(buf); i++) {
        PyValue candidate = PyList_GET_VALUE(keys, i);
        assert(PyValue_IsObject(candidate));
        name = class_name(PyValue_AsPyObjectBorrow(candidate));
        if (name == NULL || PyErr_Occurred()) {
            goto done;
        }
        const char *text = PyUnicode_Check(name) ? PyUnicode_AsUTF8(name) : "?";
        if (text == NULL || PyErr_Occurred()) {
            goto done;
        }
        off += PyOS_snprintf(buf + off, sizeof(buf) - off, " %s", text);
        type_mutation_close(&name);
        if (PyErr_Occurred()) {
            goto done;
        }
        if (i + 1 < n && (size_t)(off + 1) < sizeof(buf)) {
            buf[off++] = ',';
            buf[off] = '\0';
        }
    }
    formatted = 1;
done:
    type_mutation_close(&name);
    type_mutation_close(&keys);
    type_mutation_close(&set);
    if (formatted && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError, buf);
    }
}

static int
pmerge(PyObject *acc, PyObject **to_merge, Py_ssize_t to_merge_size)
{
    int result = -1;
    Py_ssize_t *remain = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    if ((size_t)to_merge_size > SIZE_MAX / sizeof(Py_ssize_t)) {
        PyErr_NoMemory();
        goto done;
    }
    remain = PyMem_New(Py_ssize_t, to_merge_size);
    if (remain == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_NoMemory();
        }
        goto done;
    }
    if (PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < to_merge_size; i++) {
        remain[i] = 0;
    }

again:
    ;
    Py_ssize_t empty_count = 0;
    for (Py_ssize_t i = 0; i < to_merge_size; i++) {
        PyObject *tuple = to_merge[i];
        if (remain[i] >= PyTuple_GET_SIZE(tuple)) {
            empty_count++;
            continue;
        }
        PyValue value = PyTuple_GET_VALUE(tuple, remain[i]);
        assert(PyValue_IsObject(value));
        PyObject *candidate = PyValue_AsPyObjectBorrow(value);
        for (Py_ssize_t j = 0; j < to_merge_size; j++) {
            if (tail_contains(to_merge[j], remain[j], candidate)) {
                goto skip;
            }
        }
        if (PyList_Append(acc, candidate) < 0 || PyErr_Occurred()) {
            goto done;
        }
        for (Py_ssize_t j = 0; j < to_merge_size; j++) {
            PyObject *other = to_merge[j];
            if (remain[j] < PyTuple_GET_SIZE(other) &&
                    PyValue_Is(value, PyTuple_GET_VALUE(other, remain[j]))) {
                remain[j]++;
            }
        }
        goto again;
skip:
        ;
    }
    if (empty_count != to_merge_size) {
        set_mro_error(to_merge, to_merge_size, remain);
        goto done;
    }
    result = 0;
done:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    if (remain != NULL) {
        PyMem_Free(remain);
    }
    _PyValue_RestoreException(saved);
    return PyErr_Occurred() ? -1 : result;
}

static PyObject *
mro_implementation_unlocked(PyTypeObject *type)
{
    ASSERT_TYPE_LOCK_HELD();
    PyObject *receiver = Py_NewRef(type);
    PyObject *bases = NULL;
    PyObject *base_mro = NULL;
    PyObject *error_type = NULL;
    PyObject *result = NULL;
    PyObject **to_merge = NULL;
    Py_ssize_t captured = 0;
    if (PyErr_Occurred()) {
        goto done;
    }
    if (!_PyType_IsReady(type) &&
            (PyType_Ready(type) < 0 || PyErr_Occurred())) {
        goto done;
    }
    bases = Py_XNewRef(lookup_tp_bases(type));
    if (bases == NULL) {
        PyErr_SetString(PyExc_SystemError, "type has no bases tuple");
        goto done;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(bases);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyTuple_GET_VALUE(bases, i);
        if (!PyValue_IsObject(item) || !PyType_Check(PyValue_AsPyObjectBorrow(item))) {
            error_type = Py_NewRef(PyValue_TYPE(item));
            PyErr_Format(PyExc_TypeError, "bases must be types; got '%N'", error_type);
            goto done;
        }
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        if (lookup_tp_mro(base) == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "Cannot extend an incomplete type '%.100s'", base->tp_name);
            goto done;
        }
    }
    if (n == 1) {
        PyValue item = PyTuple_GET_VALUE(bases, 0);
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        base_mro = Py_NewRef(lookup_tp_mro(base));
        Py_ssize_t k = PyTuple_GET_SIZE(base_mro);
        if (k == PY_SSIZE_T_MAX) {
            PyErr_NoMemory();
            goto done;
        }
        result = PyTuple_New(k + 1);
        if (result == NULL || PyErr_Occurred()) {
            goto done;
        }
        PyTuple_SET_VALUE(result, 0, PyValue_FromPyObjectNew((PyObject *)type));
        for (Py_ssize_t i = 0; i < k; i++) {
            PyTuple_SET_VALUE(result, i + 1, PyValue_MakeHeapSafe(
                PyTuple_GET_VALUE(base_mro, i)));
        }
        goto done;
    }
    if (check_duplicates(bases) < 0 || PyErr_Occurred()) {
        goto done;
    }
    if ((size_t)n >= SIZE_MAX / sizeof(PyObject *)) {
        PyErr_NoMemory();
        goto done;
    }
    to_merge = PyMem_Calloc((size_t)n + 1, sizeof(PyObject *));
    if (to_merge == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_NoMemory();
        }
        goto done;
    }
    if (PyErr_Occurred()) {
        goto done;
    }
    /* Reacquire every MRO after array allocation, then own the complete
     * input snapshot before result allocation or C3's append callbacks. */
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyTuple_GET_VALUE(bases, i);
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        PyObject *mro = lookup_tp_mro(base);
        if (mro == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "Cannot extend an incomplete type '%.100s'", base->tp_name);
            goto done;
        }
        to_merge[captured++] = Py_NewRef(mro);
    }
    to_merge[captured++] = Py_NewRef(bases);
    result = PyList_New(1);
    if (result != NULL) {
        // Keep private logical slots initialized even on success plus error.
        Py_SET_SIZE(result, 0);
    }
    if (result == NULL || PyErr_Occurred()) {
        goto done;
    }
    PyList_SET_VALUE(result, 0, PyValue_FromPyObjectNew((PyObject *)type));
    Py_SET_SIZE(result, 1);
    if (pmerge(result, to_merge, n + 1) < 0 || PyErr_Occurred()) {
        type_mutation_close(&result);
    }

done:
    for (Py_ssize_t i = 0; i < captured; i++) {
        type_mutation_close(&to_merge[i]);
    }
    PyObject *saved = PyErr_GetRaisedException();
    if (to_merge != NULL) {
        PyMem_Free(to_merge);
    }
    _PyValue_RestoreException(saved);
    type_mutation_close(&error_type);
    type_mutation_close(&base_mro);
    type_mutation_close(&bases);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
mro_implementation(PyTypeObject *type)
{
    PyObject *mro;
    BEGIN_TYPE_LOCK();
    mro = mro_implementation_unlocked(type);
    END_TYPE_LOCK();
    return mro;
}

/*[clinic input]
type.mro

Return a type's method resolution order.
[clinic start generated code]*/

static PyObject *
type_mro_impl(PyTypeObject *self)
/*[clinic end generated code: output=bffc4a39b5b57027 input=28414f4e156db28d]*/
{
    PyObject *original = NULL;
    PyObject *result = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    original = mro_implementation(self);
    if (original == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (PyList_Check(original)) {
        result = original;
        original = NULL;
    }
    else {
        result = PySequence_List(original);
        if (result == NULL || PyErr_Occurred()) {
            goto done;
        }
    }

done:
    type_mutation_close(&original);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static int
mro_check(PyTypeObject *type, PyObject *mro)
{
    PyObject *owner = Py_NewRef(mro);
    PyObject *solid = Py_NewRef(solid_base(type));
    PyObject *error_type = NULL;
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(owner); i++) {
        PyValue item = PyTuple_GET_VALUE(owner, i);
        if (!PyValue_IsObject(item) || !PyType_Check(PyValue_AsPyObjectBorrow(item))) {
            error_type = Py_NewRef(PyValue_TYPE(item));
            PyErr_Format(PyExc_TypeError,
                         "%N.mro() returned a non-class ('%N')", type, error_type);
            goto done;
        }
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        PyTypeObject *solid_type = (PyTypeObject *)solid;
        if (!is_subtype_with_mro(lookup_tp_mro(solid_type), solid_type, solid_base(base))) {
            PyErr_Format(PyExc_TypeError,
                         "%N.mro() returned base with unsuitable layout ('%N')",
                         type, base);
            goto done;
        }
    }
    result = 0;
done:
    type_mutation_close(&error_type);
    type_mutation_close(&solid);
    type_mutation_close(&owner);
    return PyErr_Occurred() ? -1 : result;
}

/* Lookups an mcls.mro method, invokes it and checks the result (if needed,
   in case of a custom mro() implementation).

   Keep in mind that during execution of this function type->tp_mro
   can be replaced due to possible reentrance (for example,
   through type_set_bases):

      - when looking up the mcls.mro attribute (it could be
        a user-provided descriptor);

      - from inside a custom mro() itself;

      - through a finalizer of the return value of mro().
*/
static PyObject *
mro_invoke(PyTypeObject *type)
{
    ASSERT_TYPE_LOCK_HELD();
    PyObject *mro_result = NULL;
    PyObject *new_mro = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    const int custom = !Py_IS_TYPE(type, &PyType_Type);
    if (custom) {
        mro_result = call_method_noarg((PyObject *)type, &_Py_ID(mro));
    }
    else {
        mro_result = mro_implementation_unlocked(type);
    }
    if (mro_result == NULL || PyErr_Occurred()) {
        goto done;
    }
    new_mro = PySequence_Tuple(mro_result);
    type_mutation_close(&mro_result);
    if (new_mro == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (PyTuple_GET_SIZE(new_mro) == 0) {
        PyErr_SetString(PyExc_TypeError, "type MRO must not be empty");
        goto done;
    }
    if (custom && (mro_check(type, new_mro) < 0 || PyErr_Occurred())) {
        goto done;
    }

done:
    type_mutation_close(&mro_result);
    if (PyErr_Occurred()) {
        type_mutation_close(&new_mro);
    }
    return new_mro;
}

/* Calculates and assigns a new MRO to type->tp_mro.
   Return values and invariants:

     - Returns 1 if a new MRO value has been set to type->tp_mro due to
       this call of mro_internal (no tricky reentrancy and no errors).

       In case if p_old_mro argument is not NULL, a previous value
       of type->tp_mro is put there, and the ownership of this
       reference is transferred to a caller.
       Otherwise, the previous value (if any) is decref'ed.

     - Returns 0 in case when type->tp_mro gets changed because of
       reentering here through a custom mro() (see a comment to mro_invoke).

       In this case, a refcount of an old type->tp_mro is adjusted
       somewhere deeper in the call stack (by the innermost mro_internal
       or its caller) and may become zero upon returning from here.
       This also implies that the whole hierarchy of subclasses of the type
       has seen the new value and updated their MRO accordingly.

     - Returns -1 in case of an error.
*/
static int
mro_internal(PyTypeObject *type, int initial, PyObject **p_old_mro)
{
    ASSERT_TYPE_LOCK_HELD();

    PyObject *new_mro, *old_mro;
    int reent;

    /* Keep a reference to be able to do a reentrancy check below.
       Don't let old_mro be GC'ed and its address be reused for
       another object, like (suddenly!) a new tp_mro.  */
    old_mro = Py_XNewRef(lookup_tp_mro(type));
    new_mro = mro_invoke(type);  /* might cause reentrance */
    reent = (lookup_tp_mro(type) != old_mro);
    Py_XDECREF(old_mro);
    if (new_mro == NULL) {
        return -1;
    }

    if (reent) {
        Py_DECREF(new_mro);
        return 0;
    }

    set_tp_mro(type, new_mro, initial);

    type_mro_modified(type, new_mro);
    /* corner case: the super class might have been hidden
       from the custom MRO */
    type_mro_modified(type, lookup_tp_bases(type));

    // XXX Expand this to Py_TPFLAGS_IMMUTABLETYPE?
    if (!(type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN)) {
        _PyType_Modified_Unlocked(type);
    }
    else {
        /* For static builtin types, this is only called during init
           before the method cache has been populated. */
        assert(type->tp_version_tag);
    }

    if (p_old_mro != NULL)
        *p_old_mro = old_mro;  /* transfer the ownership */
    else
        Py_XDECREF(old_mro);

    return 1;
}

/* Calculate the best base amongst multiple base classes.
   This is the first one that's on the path to the "solid base". */

static PyTypeObject *
find_best_base(PyObject *bases)
{
    assert(PyTuple_Check(bases) && PyTuple_GET_SIZE(bases) > 0);
    PyObject *owner = Py_NewRef(bases);
    PyObject *selected = NULL;
    PyObject *winner_owner = NULL;
    PyObject *error_type = NULL;
    PyTypeObject *result = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(owner);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyTuple_GET_VALUE(owner, i);
        if (!PyValue_IsObject(item) || !PyType_Check(PyValue_AsPyObjectBorrow(item))) {
            error_type = Py_NewRef(PyValue_TYPE(item));
            PyErr_Format(PyExc_TypeError, "bases must be types; got '%N'", error_type);
            goto done;
        }
        PyTypeObject *base_i = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        /* Preserve per-base rejection/readiness order. The prior solid
         * winner stays owned if readiness reparents an earlier base. */
        if (!_PyType_IsReady(base_i) &&
                (PyType_Ready(base_i) < 0 || PyErr_Occurred())) {
            goto done;
        }
        if (!_PyType_HasFeature(base_i, Py_TPFLAGS_BASETYPE)) {
            PyErr_Format(PyExc_TypeError, "type '%.100s' is not an acceptable base type",
                         base_i->tp_name);
            goto done;
        }
        type_mutation_close(&winner_owner);
        type_mutation_close(&selected);
        if (PyErr_Occurred()) {
            goto done;
        }
        /* Old-pin retirement can also reenter. Recompute only now, without
         * replaying readiness or carrying an old ancestor pointer. */
        PyTypeObject *winner = NULL;
        PyTypeObject *base = NULL;
        for (Py_ssize_t j = 0; j <= i; j++) {
            PyValue current = PyTuple_GET_VALUE(owner, j);
            PyTypeObject *base_j = _PyType_CAST(PyValue_AsPyObjectBorrow(current));
            if (!_PyType_IsReady(base_j)) {
                PyErr_Format(PyExc_TypeError,
                             "Cannot extend an incomplete type '%.100s'", base_j->tp_name);
                goto done;
            }
            if (!_PyType_HasFeature(base_j, Py_TPFLAGS_BASETYPE)) {
                PyErr_Format(PyExc_TypeError,
                             "type '%.100s' is not an acceptable base type", base_j->tp_name);
                goto done;
            }
            PyTypeObject *candidate = solid_base(base_j);
            if (winner == NULL || PyType_IsSubtype(candidate, winner)) {
                /* Equal solid bases retain the earlier dominant base. */
                if (winner == NULL || !PyType_IsSubtype(winner, candidate)) {
                    winner = candidate;
                    base = base_j;
                }
            }
            else if (!PyType_IsSubtype(winner, candidate)) {
                PyErr_SetString(PyExc_TypeError,
                                "multiple bases have instance lay-out conflict");
                goto done;
            }
        }
        assert(winner != NULL && base != NULL);
        winner_owner = Py_NewRef(winner);
        selected = Py_NewRef(base);
    }
    result = (PyTypeObject *)selected;
done:
    type_mutation_close(&error_type);
    type_mutation_close(&winner_owner);
    type_mutation_close(&selected);
    type_mutation_close(&owner);
    return PyErr_Occurred() ? NULL : result;
}

static int
shape_differs(PyTypeObject *t1, PyTypeObject *t2)
{
    return (
        t1->tp_basicsize != t2->tp_basicsize ||
        t1->tp_itemsize != t2->tp_itemsize
    );
}

static PyTypeObject *
solid_base(PyTypeObject *type)
{
    PyTypeObject *base;

    if (type->tp_base) {
        base = solid_base(type->tp_base);
    }
    else {
        base = &PyBaseObject_Type;
    }
    if (shape_differs(type, base)) {
        return type;
    }
    else {
        return base;
    }
}

#ifdef Py_GIL_DISABLED

// The structures and functions below are used in the free-threaded build
// to safely make updates to type slots, on type_setattro() for a slot
// or when __bases__ is re-assigned.  Since the slots are read without atomic
// operations and without locking, we can only safely update them while the
// world is stopped.  However, with the world stopped, we are very limited on
// which APIs can be safely used.  For example, calling _PyObject_HashDictKey()
// or _PyDict_GetItemRef_KnownHash() are not safe and can potentially cause
// deadlocks.  Hashing can be re-entrant and _PyDict_GetItemRef_KnownHash can
// acquire a lock if the dictionary is not owned by the current thread, to
// mark it shared on reading.
//
// We do the slot updates in two steps.  First, with TYPE_LOCK held, we lookup
// the descriptor for each slot, for each subclass. We build a queue of
// updates to perform but don't actually update the type structures.  After we
// are finished the lookups, we stop-the-world and apply all of the updates.
// The apply_slot_updates() code is simple and easy to confirm that it is
// safe.

typedef struct {
    PyTypeObject *type;
    void **slot_ptr;
    void *slot_value;
} slot_update_item_t;

// The number of slot updates performed is based on the number of changed
// slots and the number of subclasses.  It's possible there are many updates
// required if there are many subclasses (potentially an unbounded amount).
// Usually the number of slot updates is small, most often zero or one.  When
// running the unit tests, we don't exceed 20.  The chunk size is set to
// handle the common case with a single chunk and to not require too many
// chunk allocations if there are many subclasses.
#define SLOT_UPDATE_CHUNK_SIZE 30

typedef struct _slot_update {
    struct _slot_update *prev;
    Py_ssize_t n;
    slot_update_item_t updates[SLOT_UPDATE_CHUNK_SIZE];
} slot_update_chunk_t;

// a queue of updates to be performed
typedef struct {
    slot_update_chunk_t *head;
} slot_update_t;

static slot_update_chunk_t *
slot_update_new_chunk(void)
{
    slot_update_chunk_t *chunk = PyMem_Malloc(sizeof(slot_update_chunk_t));
    if (chunk == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    chunk->prev = NULL;
    chunk->n = 0;
    return chunk;
}

static void
slot_update_free_chunks(slot_update_t *updates)
{
    slot_update_chunk_t *chunk = updates->head;
    while (chunk != NULL) {
        slot_update_chunk_t *prev = chunk->prev;
        PyMem_Free(chunk);
        chunk = prev;
    }
}

static int
queue_slot_update(slot_update_t *updates, PyTypeObject *type,
                  void **slot_ptr, void *slot_value)
{
    if (*slot_ptr == slot_value) {
        return 0; // slot pointer not actually changed, don't queue update
    }
    if (updates->head == NULL || updates->head->n == SLOT_UPDATE_CHUNK_SIZE) {
        slot_update_chunk_t *chunk = slot_update_new_chunk();
        if (chunk == NULL) {
            return -1; // out-of-memory
        }
        chunk->prev = updates->head;
        updates->head = chunk;
    }
    slot_update_item_t *item = &updates->head->updates[updates->head->n];
    item->type = type;
    item->slot_ptr = slot_ptr;
    item->slot_value = slot_value;
    updates->head->n++;
    assert(updates->head->n <= SLOT_UPDATE_CHUNK_SIZE);
    return 0;
}

static void
apply_slot_updates(slot_update_t *updates)
{
    assert(types_world_is_stopped());
    slot_update_chunk_t *chunk = updates->head;
    while (chunk != NULL) {
        for (Py_ssize_t i = 0; i < chunk->n; i++) {
            slot_update_item_t *item = &chunk->updates[i];
            *(item->slot_ptr) = item->slot_value;
            if (item->slot_value == slot_tp_call) {
                /* A generic __call__ is incompatible with vectorcall */
                type_clear_flags(item->type, Py_TPFLAGS_HAVE_VECTORCALL);
            }
        }
        chunk = chunk->prev;
    }
}

static void
apply_type_slot_updates(slot_update_t *updates)
{
    // This must be done carefully to avoid data races and deadlocks.  We
    // have just updated the type __dict__, while holding TYPE_LOCK.  We have
    // collected all of the required type slot updates into the 'updates'
    // queue.  Note that those updates can apply to multiple types since
    // subclasses might also be affected by the dict change.
    //
    // We need to prevent other threads from writing to the dict before we can
    // finish updating the slots. The actual stores to the slots are done
    // with the world stopped.  If we block on the stop-the-world mutex then
    // we could release TYPE_LOCK mutex and potentially allow other threads
    // to update the dict.  That's because TYPE_LOCK was acquired using a
    // critical section.
    //
    // The type_lock_prevent_release() call prevents the mutexes held by the
    // critical section (TYPE_LOCK and the type dict mutex) from being released
    // even if we block on the STW mutex.  We need to take care that we do not
    // deadlock because of that.  It is safe because a thread waiting for either
    // of those mutexes detaches while it waits and so does not hold up the
    // stop-the-world.  Pinning both mutexes rather than only TYPE_LOCK is what
    // makes this safe: otherwise the dict mutex would be released when we
    // block and _PyCriticalSection_Resume() would have to re-acquire it while
    // holding TYPE_LOCK, deadlocking with a thread that holds the dict mutex
    // and is waiting for TYPE_LOCK.
    pinned_mutexes_t pinned;
    type_lock_prevent_release(&pinned);
    types_stop_world();
    apply_slot_updates(updates);
    types_start_world();
    type_lock_allow_release(&pinned);
}

#else

// dummy definition, this parameter is only NULL in the default build
typedef void slot_update_t;

#endif

/// data passed to update_slots_callback()
typedef struct {
    slot_update_t *queued_updates;
    pytype_slotdef **defs;
} update_callback_data_t;

static void object_dealloc(PyObject *);
static PyObject *object_new(PyTypeObject *, PyObject *, PyObject *);
static int object_init(PyObject *, PyObject *, PyObject *);
static int update_slot(PyTypeObject *, PyObject *, slot_update_t *update);
static void fixup_slot_dispatchers(PyTypeObject *);
static int type_ready(PyTypeObject *, int, int);
static int type_ready_publish(PyTypeObject *, int);
static int type_new_set_names(PyTypeObject *);
static int type_new_init_subclass(PyTypeObject *, PyObject *);
static bool has_slotdef(PyObject *);

/*
 * Helpers for  __dict__ descriptor.  We don't want to expose the dicts
 * inherited from various builtin types.  The builtin base usually provides
 * its own __dict__ descriptor, so we use that when we can.
 */
static PyTypeObject *
get_builtin_base_with_dict(PyTypeObject *type)
{
    while (type->tp_base != NULL) {
        if (type->tp_dictoffset != 0 &&
            !(type->tp_flags & Py_TPFLAGS_HEAPTYPE))
            return type;
        type = type->tp_base;
    }
    return NULL;
}

/* Return a new reference to the __dict__ data descriptor, or NULL. */
static PyObject *
get_dict_descriptor(PyTypeObject *type)
{
    PyObject *descr = _PyType_LookupRef(type, &_Py_ID(__dict__));
    if (descr != NULL && !PyDescr_IsData(descr)) {
        Py_CLEAR(descr);
    }
    return descr;
}

static void
raise_dict_descr_error(PyObject *obj)
{
    PyErr_Format(PyExc_TypeError,
                 "this __dict__ descriptor does not support "
                 "'%.200s' objects", Py_TYPE(obj)->tp_name);
}

static PyObject *
subtype_dict(PyObject *obj, void *context)
{
    PyTypeObject *base;

    base = get_builtin_base_with_dict(Py_TYPE(obj));
    if (base != NULL) {
        descrgetfunc func;
        PyObject *descr = get_dict_descriptor(base);
        if (descr == NULL) {
            raise_dict_descr_error(obj);
            return NULL;
        }
        func = Py_TYPE(descr)->tp_descr_get;
        if (func == NULL) {
            Py_DECREF(descr);
            raise_dict_descr_error(obj);
            return NULL;
        }
        PyObject *dict = func(descr, obj, (PyObject *)(Py_TYPE(obj)));
        Py_DECREF(descr);
        return dict;
    }
    return PyObject_GenericGetDict(obj, context);
}

int
_PyObject_SetDict(PyObject *obj, PyObject *value)
{
    if (value != NULL && !PyDict_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "__dict__ must be set to a dictionary, "
                     "not a '%.200s'", Py_TYPE(value)->tp_name);
        return -1;
    }
    if (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        return _PyObject_SetManagedDict(obj, value);
    }
    PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
    if (dictptr == NULL) {
        PyErr_SetString(PyExc_AttributeError,
                        "This object has no __dict__");
        return -1;
    }
    Py_BEGIN_CRITICAL_SECTION(obj);
    // gh-133980: To prevent use-after-free from other threads that reference
    // the __dict__
    _PyObject_XSetRefDelayed(dictptr, Py_NewRef(value));
    Py_END_CRITICAL_SECTION();
    return 0;
}

static int
subtype_setdict(PyObject *obj, PyObject *value, void *context)
{
    PyTypeObject *base;

    base = get_builtin_base_with_dict(Py_TYPE(obj));
    if (base != NULL) {
        descrsetfunc func;
        PyObject *descr = get_dict_descriptor(base);
        if (descr == NULL) {
            raise_dict_descr_error(obj);
            return -1;
        }
        func = Py_TYPE(descr)->tp_descr_set;
        if (func == NULL) {
            Py_DECREF(descr);
            raise_dict_descr_error(obj);
            return -1;
        }
        int res = func(descr, obj, value);
        Py_DECREF(descr);
        return res;
    }
    return _PyObject_SetDict(obj, value);
}

static PyObject *
subtype_getweakref(PyObject *obj, void *context)
{
    PyObject **weaklistptr;
    PyObject *result;
    PyTypeObject *type = Py_TYPE(obj);

    if (type->tp_weaklistoffset == 0) {
        PyErr_SetString(PyExc_AttributeError,
                        "This object has no __weakref__");
        return NULL;
    }
    _PyObject_ASSERT((PyObject *)type,
                     type->tp_weaklistoffset > 0 ||
                     type->tp_weaklistoffset == MANAGED_WEAKREF_OFFSET);
    _PyObject_ASSERT((PyObject *)type,
                     ((type->tp_weaklistoffset + (Py_ssize_t)sizeof(PyObject *))
                      <= type->tp_basicsize));

    /* Synchronize with weakref creation/destruction so the list head cannot
     * be removed and freed between loading it and taking a reference. */
    LOCK_WEAKREFS(obj);
    weaklistptr = (PyObject **)((char *)obj + type->tp_weaklistoffset);
    if (*weaklistptr == NULL) {
        result = Py_NewRef(Py_None);
    }
    else if (_Py_TryIncref(*weaklistptr)) {
        result = *weaklistptr;
    }
    else {
        result = Py_NewRef(Py_None);
    }

    UNLOCK_WEAKREFS(obj);

    return result;
}

/* getset definitions for common descriptors */
static PyGetSetDef subtype_getset_dict = {
    "__dict__", subtype_dict, subtype_setdict,
    PyDoc_STR("dictionary for instance variables"),
};

static PyGetSetDef subtype_getset_weakref = {
    "__weakref__", subtype_getweakref, NULL,
    PyDoc_STR("list of weak references to the object"),
};

static int
valid_identifier(PyObject *s)
{
    if (!PyUnicode_Check(s)) {
        PyErr_Format(PyExc_TypeError,
                     "__slots__ items must be strings, not '%.200s'",
                     Py_TYPE(s)->tp_name);
        return 0;
    }
    if (!PyUnicode_IsIdentifier(s)) {
        PyErr_SetString(PyExc_TypeError,
                        "__slots__ must be identifiers");
        return 0;
    }
    return 1;
}

static int
type_init(PyObject *cls, PyObject *args, PyObject *kwds)
{
    assert(args != NULL && PyTuple_Check(args));
    assert(kwds == NULL || PyDict_Check(kwds));

    if (kwds != NULL && PyTuple_GET_SIZE(args) == 1 &&
        PyDict_GET_SIZE(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError,
                        "type.__init__() takes no keyword arguments");
        return -1;
    }

    if ((PyTuple_GET_SIZE(args) != 1 && PyTuple_GET_SIZE(args) != 3)) {
        PyErr_SetString(PyExc_TypeError,
                        "type.__init__() takes 1 or 3 arguments");
        return -1;
    }

    return 0;
}


unsigned long
PyType_GetFlags(PyTypeObject *type)
{
    return type->tp_flags;
}


int
PyType_SUPPORTS_WEAKREFS(PyTypeObject *type)
{
    return _PyType_SUPPORTS_WEAKREFS(type);
}


/* Determine the most derived metatype. */
PyTypeObject *
_PyType_CalculateMetaclass(PyTypeObject *metatype, PyObject *bases)
{
    PyObject *owner = Py_NewRef(bases);
    PyObject *winner = Py_NewRef(metatype);
    PyTypeObject *result = NULL;
    if (PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(owner); i++) {
        PyTypeObject *candidate = PyValue_TYPE(PyTuple_GET_VALUE(owner, i));
        if (PyType_IsSubtype((PyTypeObject *)winner, candidate)) {
            continue;
        }
        if (PyType_IsSubtype(candidate, (PyTypeObject *)winner)) {
            PyObject *replacement = Py_NewRef(candidate);
            type_mutation_close(&winner);
            winner = replacement;
            if (PyErr_Occurred()) {
                goto done;
            }
            continue;
        }
        PyErr_SetString(PyExc_TypeError,
                        "metaclass conflict: "
                        "the metaclass of a derived class "
                        "must be a (non-strict) subclass "
                        "of the metaclasses of all its bases");
        goto done;
    }
    result = (PyTypeObject *)winner;
done:
    type_mutation_close(&winner);
    type_mutation_close(&owner);
    return PyErr_Occurred() ? NULL : result;
}


// Forward declaration
static PyObject *
type_new(PyTypeObject *metatype, PyObject *args, PyObject *kwds);

typedef struct {
    PyTypeObject *metatype;
    PyObject *args;
    PyObject *kwds;
    PyObject *orig_dict;
    PyObject *name;
    PyObject *bases;
    PyTypeObject *base;
    PyObject *slots;
    Py_ssize_t nslot;
    int add_dict;
    int add_weak;
    int may_add_dict;
    int may_add_weak;
} type_new_ctx;


/* Check for valid slot names and two special cases */
static int
type_new_visit_slots(type_new_ctx *ctx)
{
    PyObject *slots_owner = NULL;
    PyObject *error_type_owner = NULL;
    PyValue selected = PyValue_NULL;
    int result = -1;

    if (PyErr_Occurred()) {
        goto done;
    }
    slots_owner = Py_NewRef(ctx->slots);
    if (slots_owner == NULL || PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < ctx->nslot; i++) {
        selected = PyTuple_GET_VALUE(slots_owner, i);
        if (!PyValue_IsObject(selected) ||
                !PyUnicode_Check(PyValue_AsPyObjectBorrow(selected))) {
            error_type_owner = (PyObject *)Py_NewRef(PyValue_TYPE(selected));
            if (error_type_owner == NULL || PyErr_Occurred()) {
                goto done;
            }
            char message[256];
            PyOS_snprintf(message, sizeof(message),
                          "__slots__ items must be strings, not '%.200s'",
                          ((PyTypeObject *)error_type_owner)->tp_name);
            PyErr_SetString(PyExc_TypeError, message);
            goto done;
        }
        PyObject *name = PyValue_AsPyObjectBorrow(selected);
        int valid = valid_identifier(name);
        if (valid < 0 || PyErr_Occurred()) {
            goto done;
        }
        if (!valid) {
            goto done;
        }
        assert(PyUnicode_Check(name));
        if (_PyUnicode_Equal(name, &_Py_ID(__dict__))) {
            if (!ctx->may_add_dict || ctx->add_dict != 0) {
                PyErr_SetString(PyExc_TypeError,
                    "__dict__ slot disallowed: "
                    "we already got one");
                goto done;
            }
            ctx->add_dict++;
        }
        if (_PyUnicode_Equal(name, &_Py_ID(__weakref__))) {
            if (!ctx->may_add_weak || ctx->add_weak != 0) {
                PyErr_SetString(PyExc_TypeError,
                    "__weakref__ slot disallowed: "
                    "we already got one");
                goto done;
            }
            ctx->add_weak++;
        }
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    result = 0;

done:
    type_mutation_close(&error_type_owner);
    type_mutation_close(&slots_owner);
    return PyErr_Occurred() ? -1 : result;
}


/* Copy slots into a list, mangle names and sort them.
   Sorted names are needed for __class__ assignment.
   Convert them back to tuple at the end.
*/
static PyObject*
type_new_copy_slots(type_new_ctx *ctx, PyObject *dict)
{
    PyObject *slots_owner = NULL;
    PyObject *name_owner = NULL;
    PyObject *dict_owner = NULL;
    PyObject *new_slots = NULL;
    PyObject *mangled = NULL;
    PyObject *tuple = NULL;
    PyObject *result = NULL;
    int ok = 0;
    Py_ssize_t nslot = ctx->nslot;
    Py_ssize_t new_nslot = nslot - ctx->add_dict - ctx->add_weak;

    if (PyErr_Occurred()) {
        goto done;
    }
    slots_owner = Py_NewRef(ctx->slots);
    if (slots_owner == NULL || PyErr_Occurred()) goto done;
    name_owner = Py_NewRef(ctx->name);
    if (name_owner == NULL || PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    new_slots = PyList_New(new_nslot);
    if (new_slots == NULL || PyErr_Occurred()) goto done;

    Py_ssize_t j = 0;
    for (Py_ssize_t i = 0; i < nslot; i++) {
        PyValue value = PyTuple_GET_VALUE(slots_owner, i);
        assert(PyValue_IsObject(value));
        PyObject *slot = PyValue_AsPyObjectBorrow(value);
        if ((ctx->add_dict && _PyUnicode_Equal(slot, &_Py_ID(__dict__))) ||
            (ctx->add_weak && _PyUnicode_Equal(slot, &_Py_ID(__weakref__)))) {
            if (PyErr_Occurred()) goto done;
            continue;
        }
        if (PyErr_Occurred()) goto done;
        mangled = _Py_Mangle(name_owner, slot);
        if (mangled == NULL || PyErr_Occurred()) goto done;
        int set_status = PyList_SetItem(new_slots, j, Py_NewRef(mangled));
        if (set_status < 0 || PyErr_Occurred()) goto done;
        int contains = PyDict_Contains(dict_owner, mangled);
        if (contains < 0 || PyErr_Occurred()) goto done;
        if (contains > 0) {
            if (!_PyUnicode_Equal(mangled, &_Py_ID(__qualname__)) &&
                !_PyUnicode_Equal(mangled, &_Py_ID(__classcell__)) &&
                !_PyUnicode_Equal(mangled, &_Py_ID(__classdictcell__))) {
                PyErr_Format(PyExc_ValueError,
                             "%R in __slots__ conflicts with class variable",
                             mangled);
                goto done;
            }
            if (PyErr_Occurred()) goto done;
        }
        type_mutation_close(&mangled);
        if (PyErr_Occurred()) goto done;
        j++;
    }
    assert(j == new_nslot);
    int sort_status = PyList_Sort(new_slots);
    if (sort_status < 0 || PyErr_Occurred()) goto done;
    tuple = PyList_AsTuple(new_slots);
    if (tuple == NULL || PyErr_Occurred()) goto done;
    if (PyTuple_GET_SIZE(tuple) != new_nslot) {
        PyErr_SetString(PyExc_RuntimeError, "stale __slots__ construction");
        goto done;
    }
    for (Py_ssize_t i = 0; i < new_nslot; i++) {
        PyValue value = PyTuple_GET_VALUE(tuple, i);
        if (!PyValue_IsObject(value) ||
                !PyUnicode_Check(PyValue_AsPyObjectBorrow(value))) {
            PyErr_SetString(PyExc_RuntimeError, "stale __slots__ construction");
            goto done;
        }
    }
    if (PyErr_Occurred()) goto done;
    ok = 1;

done:
    type_mutation_close(&mangled);
    type_mutation_close(&new_slots);
    type_mutation_close(&dict_owner);
    type_mutation_close(&name_owner);
    type_mutation_close(&slots_owner);
    if (PyErr_Occurred()) ok = 0;
    if (ok) {
        result = tuple;
        tuple = NULL;
    }
    type_mutation_close(&tuple);
    return result;
}


static void
type_new_slots_bases(type_new_ctx *ctx)
{
    Py_ssize_t nbases = PyTuple_GET_SIZE(ctx->bases);
    if (nbases > 1 &&
        ((ctx->may_add_dict && ctx->add_dict == 0) ||
         (ctx->may_add_weak && ctx->add_weak == 0))) {
        for (Py_ssize_t i = 0; i < nbases; i++) {
            PyValue value = PyTuple_GET_VALUE(ctx->bases, i);
            assert(PyValue_IsObject(value));
            PyObject *obj = PyValue_AsPyObjectBorrow(value);
            assert(PyType_Check(obj));
            if (obj == (PyObject *)ctx->base) continue;
            PyTypeObject *base = _PyType_CAST(obj);
            if (ctx->may_add_dict && ctx->add_dict == 0 &&
                    base->tp_dictoffset != 0) {
                ctx->add_dict++;
            }
            if (ctx->may_add_weak && ctx->add_weak == 0 &&
                    base->tp_weaklistoffset != 0) {
                ctx->add_weak++;
            }
            if (ctx->may_add_dict && ctx->add_dict == 0) continue;
            if (ctx->may_add_weak && ctx->add_weak == 0) continue;
            break;
        }
    }
}


static int
type_new_slots_impl(type_new_ctx *ctx, PyObject *dict)
{
    if (type_new_visit_slots(ctx) < 0) {
        return -1;
    }

    PyObject *new_slots = type_new_copy_slots(ctx, dict);
    if (new_slots == NULL) {
        return -1;
    }
    assert(PyTuple_CheckExact(new_slots));

    Py_XSETREF(ctx->slots, new_slots);
    ctx->nslot = PyTuple_GET_SIZE(new_slots);

    /* Secondary bases may provide weakrefs or dict */
    type_new_slots_bases(ctx);
    return 0;
}


static Py_ssize_t
type_new_slots(type_new_ctx *ctx, PyObject *dict)
{
    // Check for a __slots__ sequence variable in dict, and count it
    ctx->add_dict = 0;
    ctx->add_weak = 0;
    ctx->may_add_dict = (ctx->base->tp_dictoffset == 0);
    ctx->may_add_weak = (ctx->base->tp_weaklistoffset == 0);

    if (ctx->slots == NULL) {
        if (ctx->may_add_dict) {
            ctx->add_dict++;
        }
        if (ctx->may_add_weak && ctx->base->tp_itemsize == 0) {
            ctx->add_weak++;
        }
    }
    else {
        /* Have slots */
        if (type_new_slots_impl(ctx, dict) < 0) {
            return -1;
        }
    }
    return 0;
}


static PyTypeObject*
type_new_alloc(type_new_ctx *ctx)
{
    PyTypeObject *metatype = ctx->metatype;
    PyTypeObject *type;

    // Allocate the type object
    type = (PyTypeObject *)metatype->tp_alloc(metatype, ctx->nslot);
    if (type == NULL) {
        return NULL;
    }
    PyHeapTypeObject *et = (PyHeapTypeObject *)type;

    // Initialize tp_flags.
    // All heap types need GC, since we can create a reference cycle by storing
    // an instance on one of its parents.
    type_set_flags(type, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE |
                   Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC);
    et->ht_mro_epoch = 0;

    // Initialize essential fields
    type->tp_as_async = &et->as_async;
    type->tp_as_number = &et->as_number;
    type->tp_as_sequence = &et->as_sequence;
    type->tp_as_mapping = &et->as_mapping;
    type->tp_as_buffer = &et->as_buffer;

    set_tp_bases(type, Py_NewRef(ctx->bases), 1);
    if (type->tp_base != ctx->base) {
        mro_epoch_advance(type);
    }
    type->tp_base = (PyTypeObject *)Py_NewRef(ctx->base);

    type->tp_dealloc = subtype_dealloc;
    /* Always override allocation strategy to use regular heap */
    type->tp_alloc = PyType_GenericAlloc;
    type->tp_free = PyObject_GC_Del;

    type->tp_traverse = subtype_traverse;
    type->tp_clear = subtype_clear;

    et->ht_name = Py_NewRef(ctx->name);
    et->ht_module = NULL;
    et->_ht_tpname = NULL;
    et->ht_token = NULL;

#ifdef Py_GIL_DISABLED
    et->unique_id = _PyObject_AssignUniqueId((PyObject *)et);
#endif

    return type;
}


static int
type_new_set_name(const type_new_ctx *ctx, PyTypeObject *type)
{
    Py_ssize_t name_size;
    type->tp_name = PyUnicode_AsUTF8AndSize(ctx->name, &name_size);
    if (!type->tp_name) {
        return -1;
    }
    if (strlen(type->tp_name) != (size_t)name_size) {
        PyErr_SetString(PyExc_ValueError,
                        "type name must not contain null characters");
        return -1;
    }
    return 0;
}


/* Set __module__ in the dict */
static int
type_new_set_module(PyObject *dict)
{
    int r = PyDict_Contains(dict, &_Py_ID(__module__));
    if (r < 0) {
        return -1;
    }
    if (r > 0) {
        return 0;
    }

    PyObject *globals = PyEval_GetGlobals();
    if (globals == NULL) {
        return 0;
    }

    PyObject *module;
    r = PyDict_GetItemRef(globals, &_Py_ID(__name__), &module);
    if (module) {
        r = PyDict_SetItem(dict, &_Py_ID(__module__), module);
        Py_DECREF(module);
    }
    return r;
}


/* Set ht_qualname to dict['__qualname__'] if available, else to
   __name__.  The __qualname__ accessor will look for ht_qualname. */
static int
type_new_set_ht_name(PyTypeObject *type, PyObject *dict)
{
    PyHeapTypeObject *et = (PyHeapTypeObject *)type;
    PyObject *qualname;
    if (PyDict_GetItemRef(dict, &_Py_ID(__qualname__), &qualname) < 0) {
        return -1;
    }
    if (qualname != NULL) {
        if (!PyUnicode_Check(qualname)) {
            PyErr_Format(PyExc_TypeError,
                    "type __qualname__ must be a str, not %s",
                    Py_TYPE(qualname)->tp_name);
            Py_DECREF(qualname);
            return -1;
        }
        et->ht_qualname = qualname;
        if (PyDict_DelItem(dict, &_Py_ID(__qualname__)) < 0) {
            return -1;
        }
    }
    else {
        et->ht_qualname = Py_NewRef(et->ht_name);
    }
    return 0;
}


/* Set tp_doc to a copy of dict['__doc__'], if the latter is there
   and is a string.  The __doc__ accessor will first look for tp_doc;
   if that fails, it will still look into __dict__. */
static int
type_new_set_doc(PyTypeObject *type, PyObject* dict)
{
    PyObject *type_owner = NULL;
    PyObject *dict_owner = NULL;
    PyValue doc = PyValue_NULL;
    char *tp_doc = NULL;
    int result = -1;
    if (PyErr_Occurred()) goto done;
    type_owner = Py_NewRef((PyObject *)type);
    if (type_owner == NULL || PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    int found = PyDict_GetValue(dict_owner, PyValue_FromObjectBorrow(&_Py_ID(__doc__)), &doc);
    if (found < 0 || PyErr_Occurred()) goto done;
    if (found == 0 || !PyValue_IsObject(doc) ||
            !PyUnicode_Check(PyValue_AsPyObjectBorrow(doc))) {
        result = 0;
        goto done;
    }
    const char *doc_str = PyUnicode_AsUTF8(PyValue_AsPyObjectBorrow(doc));
    if (doc_str == NULL || PyErr_Occurred()) goto done;
    Py_ssize_t size = strlen(doc_str) + 1;
    tp_doc = PyMem_Malloc(size);
    if (tp_doc == NULL || PyErr_Occurred()) {
        if (tp_doc == NULL && !PyErr_Occurred()) PyErr_NoMemory();
        goto done;
    }
    memcpy(tp_doc, doc_str, size);
    type->tp_doc = tp_doc;
    tp_doc = NULL;
    result = 0;
done:
    if (tp_doc != NULL) {
        PyObject *saved = PyErr_GetRaisedException();
        char *retired = tp_doc;
        tp_doc = NULL;
        PyMem_Free(retired);
        _PyValue_RestoreException(saved);
    }
    {
        PyObject *saved = PyErr_GetRaisedException();
        PyValue retired = doc;
        doc = PyValue_NULL;
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&dict_owner);
    type_mutation_close(&type_owner);
    return PyErr_Occurred() ? -1 : result;
}


static int
type_new_staticmethod(PyObject *dict, PyObject *attr)
{
    PyObject *dict_owner = NULL;
    PyObject *key_owner = NULL;
    PyObject *wrapper = NULL;
    PyValue selected = PyValue_NULL;
    int result = -1;
    if (PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    key_owner = Py_NewRef(attr);
    if (key_owner == NULL || PyErr_Occurred()) goto done;
    int found = PyDict_GetValue(dict_owner, PyValue_FromObjectBorrow(key_owner), &selected);
    if (found < 0 || PyErr_Occurred()) goto done;
    if (found == 0 || !PyValue_IsObject(selected) ||
            !PyFunction_Check(PyValue_AsPyObjectBorrow(selected))) {
        result = 0;
        goto done;
    }
    if (PyErr_Occurred()) goto done;
    wrapper = PyStaticMethod_New(PyValue_AsPyObjectBorrow(selected));
    if (wrapper == NULL || PyErr_Occurred()) goto done;
    result = PyDict_SetItem(dict_owner, key_owner, wrapper);
done:
    type_mutation_close(&wrapper);
    {
        PyObject *saved = PyErr_GetRaisedException();
        PyValue retired = selected;
        selected = PyValue_NULL;
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&key_owner);
    type_mutation_close(&dict_owner);
    return PyErr_Occurred() ? -1 : result;
}


static int
type_new_classmethod(PyObject *dict, PyObject *attr)
{
    PyObject *dict_owner = NULL;
    PyObject *key_owner = NULL;
    PyObject *wrapper = NULL;
    PyValue selected = PyValue_NULL;
    int result = -1;
    if (PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    key_owner = Py_NewRef(attr);
    if (key_owner == NULL || PyErr_Occurred()) goto done;
    int found = PyDict_GetValue(dict_owner, PyValue_FromObjectBorrow(key_owner), &selected);
    if (found < 0 || PyErr_Occurred()) goto done;
    if (found == 0 || !PyValue_IsObject(selected) ||
            !PyFunction_Check(PyValue_AsPyObjectBorrow(selected))) {
        result = 0;
        goto done;
    }
    if (PyErr_Occurred()) goto done;
    wrapper = PyClassMethod_New(PyValue_AsPyObjectBorrow(selected));
    if (wrapper == NULL || PyErr_Occurred()) goto done;
    result = PyDict_SetItem(dict_owner, key_owner, wrapper);
done:
    type_mutation_close(&wrapper);
    {
        PyObject *saved = PyErr_GetRaisedException();
        PyValue retired = selected;
        selected = PyValue_NULL;
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&key_owner);
    type_mutation_close(&dict_owner);
    return PyErr_Occurred() ? -1 : result;
}

/* Add __dict__ or __weakref__ descriptor */
static int
type_add_common_descriptor(PyInterpreterState *interp,
                           PyObject **cache,
                           PyGetSetDef *getset_def,
                           PyObject *dict)
{
#ifdef Py_GIL_DISABLED
    PyMutex_Lock(&interp->cached_objects.descriptor_mutex);
#endif
    PyObject *descr = *cache;
    if (!descr) {
        descr = PyDescr_NewGetSet(&PyBaseObject_Type, getset_def);
        *cache = descr;
    }
#ifdef Py_GIL_DISABLED
    PyMutex_Unlock(&interp->cached_objects.descriptor_mutex);
#endif
    if (!descr) {
        return -1;
    }
    if (PyDict_SetDefaultRef(dict, PyDescr_NAME(descr), descr, NULL) < 0) {
        return -1;
    }
    return 0;
}

/* Add descriptors for custom slots from __slots__, or for __dict__ */
static int
type_new_descriptors(const type_new_ctx *ctx, PyTypeObject *type, PyObject *dict)
{
    PyHeapTypeObject *et = (PyHeapTypeObject *)type;
    Py_ssize_t slotoffset = ctx->base->tp_basicsize;
    if (et->ht_slots != NULL) {
        PyMemberDef *mp = _PyHeapType_GET_MEMBERS(et);
        Py_ssize_t nslot = PyTuple_GET_SIZE(et->ht_slots);
        int after_items = (ctx->base->tp_itemsize != 0 &&
                           !(ctx->base->tp_flags & Py_TPFLAGS_ITEMS_AT_END));
        if (ctx->base->tp_itemsize != 0 &&
            !(ctx->base->tp_flags & Py_TPFLAGS_TUPLE_SUBCLASS))
        {
            PyErr_Format(PyExc_TypeError,
                         "arbitrary __slots__ not supported for subtype of '%s'",
                         ctx->base->tp_name);
            return -1;
        }
        for (Py_ssize_t i = 0; i < nslot; i++, mp++) {
            mp->name = PyUnicode_AsUTF8(
                _PyTuple_GET_OBJECT(et->ht_slots, i));
            if (mp->name == NULL) {
                return -1;
            }
            mp->type = _Py_T_VALUE_EX;
            mp->offset = slotoffset;
            if (after_items) {
                mp->flags |= _Py_AFTER_ITEMS;
            }

            /* __dict__ and __weakref__ are already filtered out */
            assert(strcmp(mp->name, "__dict__") != 0);
            assert(strcmp(mp->name, "__weakref__") != 0);

            slotoffset += sizeof(PyValue);
        }
    }

    if (ctx->add_weak) {
        assert((type->tp_flags & Py_TPFLAGS_MANAGED_WEAKREF) == 0);
        type_add_flags(type, Py_TPFLAGS_MANAGED_WEAKREF);
        type->tp_weaklistoffset = MANAGED_WEAKREF_OFFSET;
    }
    if (ctx->add_dict) {
        assert((type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);
        type_add_flags(type, Py_TPFLAGS_MANAGED_DICT);
        type->tp_dictoffset = -1;
    }

    type->tp_basicsize = slotoffset;
    type->tp_itemsize = ctx->base->tp_itemsize;
    type->tp_members = _PyHeapType_GET_MEMBERS(et);

    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (type->tp_dictoffset) {
        if (type_add_common_descriptor(
            interp,
            &interp->cached_objects.dict_descriptor,
            &subtype_getset_dict,
            dict) < 0)
        {
            return -1;
        }
    }
    if (type->tp_weaklistoffset) {
        if (type_add_common_descriptor(
            interp,
            &interp->cached_objects.weakref_descriptor,
            &subtype_getset_weakref,
            dict) < 0)
        {
            return -1;
        }
    }

    return 0;
}


static void
type_new_set_slots(const type_new_ctx *ctx, PyTypeObject *type)
{
    type->tp_getset = NULL;

    /* Special case some slots */
    if (type->tp_dictoffset != 0 || ctx->nslot > 0) {
        PyTypeObject *base = ctx->base;
        if (base->tp_getattr == NULL && base->tp_getattro == NULL) {
            type->tp_getattro = PyObject_GenericGetAttr;
        }
        if (base->tp_setattr == NULL && base->tp_setattro == NULL) {
            type->tp_setattro = PyObject_GenericSetAttr;
        }
    }
}


/* store type in class' cell if one is supplied */
static int
type_new_set_classcell(PyTypeObject *type, PyObject *dict)
{
    PyObject *type_owner = NULL;
    PyObject *dict_owner = NULL;
    PyObject *selected_type_owner = NULL;
    PyValue cell = PyValue_NULL;
    int result = -1;
    if (PyErr_Occurred()) goto done;
    type_owner = Py_NewRef((PyObject *)type);
    if (type_owner == NULL || PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    int found = PyDict_GetValue(
        dict_owner, PyValue_FromObjectBorrow(&_Py_ID(__classcell__)), &cell);
    if (found < 0 || PyErr_Occurred()) goto done;
    if (found == 0) { result = 0; goto done; }
    if (!PyValue_IsObject(cell) || !PyCell_Check(PyValue_AsPyObjectBorrow(cell))) {
        selected_type_owner = (PyObject *)Py_NewRef(PyValue_TYPE(cell));
        if (selected_type_owner == NULL || PyErr_Occurred()) goto done;
        PyErr_Format(PyExc_TypeError,
                     "__classcell__ must be a nonlocal cell, not %.200R",
                     selected_type_owner);
        goto done;
    }
    int cell_status = PyCell_Set(PyValue_AsPyObjectBorrow(cell), type_owner);
    if (cell_status < 0 || PyErr_Occurred()) goto done;
    int del_status = PyDict_DelItem(dict_owner, &_Py_ID(__classcell__));
    if (del_status < 0 || PyErr_Occurred()) goto done;
    result = 0;
done:
    type_mutation_close(&selected_type_owner);
    {
        PyObject *saved = PyErr_GetRaisedException();
        PyValue retired = cell;
        cell = PyValue_NULL;
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&dict_owner);
    type_mutation_close(&type_owner);
    return PyErr_Occurred() ? -1 : result;
}

static int
type_new_set_classdictcell(PyObject *dict)
{
    PyObject *dict_owner = NULL;
    PyObject *selected_type_owner = NULL;
    PyValue cell = PyValue_NULL;
    int result = -1;
    if (PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    int found = PyDict_GetValue(
        dict_owner, PyValue_FromObjectBorrow(&_Py_ID(__classdictcell__)), &cell);
    if (found < 0 || PyErr_Occurred()) goto done;
    if (found == 0) { result = 0; goto done; }
    if (!PyValue_IsObject(cell) || !PyCell_Check(PyValue_AsPyObjectBorrow(cell))) {
        selected_type_owner = (PyObject *)Py_NewRef(PyValue_TYPE(cell));
        if (selected_type_owner == NULL || PyErr_Occurred()) goto done;
        PyErr_Format(PyExc_TypeError,
                     "__classdictcell__ must be a nonlocal cell, not %.200R",
                     selected_type_owner);
        goto done;
    }
    int cell_status = PyCell_Set(PyValue_AsPyObjectBorrow(cell), dict_owner);
    if (cell_status < 0 || PyErr_Occurred()) goto done;
    int del_status = PyDict_DelItem(dict_owner, &_Py_ID(__classdictcell__));
    if (del_status < 0 || PyErr_Occurred()) goto done;
    result = 0;
done:
    type_mutation_close(&selected_type_owner);
    {
        PyObject *saved = PyErr_GetRaisedException();
        PyValue retired = cell;
        cell = PyValue_NULL;
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&dict_owner);
    return PyErr_Occurred() ? -1 : result;
}

static int
type_new_set_attrs(const type_new_ctx *ctx, PyTypeObject *type)
{
    if (type_new_set_name(ctx, type) < 0) {
        return -1;
    }

    PyObject *dict = lookup_tp_dict(type);
    assert(dict);

    if (type_new_set_module(dict) < 0) {
        return -1;
    }

    if (type_new_set_ht_name(type, dict) < 0) {
        return -1;
    }

    if (type_new_set_doc(type, dict) < 0) {
        return -1;
    }

    /* Special-case __new__: if it's a plain function,
       make it a static function */
    if (type_new_staticmethod(dict, &_Py_ID(__new__)) < 0) {
        return -1;
    }

    /* Special-case __init_subclass__ and __class_getitem__:
       if they are plain functions, make them classmethods */
    if (type_new_classmethod(dict, &_Py_ID(__init_subclass__)) < 0) {
        return -1;
    }
    if (type_new_classmethod(dict, &_Py_ID(__class_getitem__)) < 0) {
        return -1;
    }

    if (type_new_descriptors(ctx, type, dict) < 0) {
        return -1;
    }

    type_new_set_slots(ctx, type);

    if (type_new_set_classcell(type, dict) < 0) {
        return -1;
    }
    if (type_new_set_classdictcell(dict) < 0) {
        return -1;
    }

#ifdef Py_GIL_DISABLED
    // enable deferred reference counting on functions and descriptors
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (PyFunction_Check(value) || Py_TYPE(value)->tp_descr_get != NULL) {
            PyUnstable_Object_EnableDeferredRefcount(value);
        }
    }
#endif

    return 0;
}


static int
type_new_get_slots(type_new_ctx *ctx, PyObject *dict)
{
    PyObject *dict_owner = NULL;
    PyObject *iterable_owner = NULL;
    PyObject *new_slots = NULL;
    PyValue selected = PyValue_NULL;
    int result = -1;
    int missing = 0;
    if (PyErr_Occurred()) goto done;
    dict_owner = Py_NewRef(dict);
    if (dict_owner == NULL || PyErr_Occurred()) goto done;
    int found = PyDict_GetValue(
        dict_owner, PyValue_FromObjectBorrow(&_Py_ID(__slots__)), &selected);
    if (found < 0 || PyErr_Occurred()) goto done;
    if (found == 0) {
        missing = 1;
        result = 0;
        goto done;
    }
    iterable_owner = PyValue_AsObject(selected);
    if (iterable_owner == NULL || PyErr_Occurred()) goto done;
    if (PyUnicode_Check(iterable_owner)) {
        new_slots = PyTuple_Pack(1, iterable_owner);
    }
    else {
        new_slots = PySequence_Tuple(iterable_owner);
    }
    if (new_slots == NULL || PyErr_Occurred()) goto done;
    if (!PyTuple_CheckExact(new_slots)) {
        PyErr_SetString(PyExc_SystemError,
                        "type.__new__ slots conversion did not return an exact tuple");
        goto done;
    }
    if (PyErr_Occurred()) goto done;
    result = 0;
done:
    type_mutation_close(&iterable_owner);
    {
        PyObject *saved = PyErr_GetRaisedException();
        PyValue retired = selected;
        selected = PyValue_NULL;
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&dict_owner);
    if (PyErr_Occurred()) {
        result = -1;
    }
    if (result == 0) {
        if (missing) {
            ctx->slots = NULL;
            ctx->nslot = 0;
        }
        else {
            ctx->slots = new_slots;
            ctx->nslot = PyTuple_GET_SIZE(new_slots);
            new_slots = NULL;
        }
    }
    type_mutation_close(&new_slots);
    return result;
}


static PyTypeObject*
type_new_init(type_new_ctx *ctx)
{
    PyObject *dict = _PyDict_CopyAsDict(ctx->orig_dict);
    if (dict == NULL) {
        goto error;
    }

    if (type_new_get_slots(ctx, dict) < 0) {
        goto error;
    }
    assert(!PyErr_Occurred());

    if (type_new_slots(ctx, dict) < 0) {
        goto error;
    }

    PyTypeObject *type = type_new_alloc(ctx);
    if (type == NULL) {
        goto error;
    }

    set_tp_dict(type, dict);

    PyHeapTypeObject *et = (PyHeapTypeObject*)type;
    if (ctx->slots && PyTuple_GET_SIZE(ctx->slots)) {
        et->ht_slots = ctx->slots;
        ctx->slots = NULL;
    }
    else {
        et->ht_slots = NULL;
        Py_CLEAR(ctx->slots);
    }

    return type;

error:
    Py_CLEAR(ctx->slots);
    Py_XDECREF(dict);
    return NULL;
}


static PyObject*
type_new_impl(type_new_ctx *ctx)
{
    PyTypeObject *type = type_new_init(ctx);
    if (type == NULL) {
        return NULL;
    }

    if (type_new_set_attrs(ctx, type) < 0) {
        goto error;
    }

    /* Initialize the rest */
    if (type_ready_publish(type, 1) < 0) {
        goto error;
    }

    if (!_PyDict_HasOnlyStringKeys(type->tp_dict)) {
        if (PyErr_WarnFormat(
                PyExc_RuntimeWarning,
                1,
                "non-string key in the __dict__ of class %.200s",
                type->tp_name) == -1)
        {
            goto error;
        }
    }

    if (type_new_set_names(type) < 0) {
        goto error;
    }

    if (type_new_init_subclass(type, ctx->kwds) < 0) {
        goto error;
    }

    assert(_PyType_CheckConsistency(type));

    return (PyObject *)type;

error:
    Py_DECREF(type);
    return NULL;
}


static int
type_new_get_bases(type_new_ctx *ctx, PyObject **type,
                   PyObject **selected_metatype_owner,
                   PyObject **selected_base_owner)
{
    PyObject *bases_owner = NULL;
    PyObject *initial_metatype_owner = NULL;
    PyObject *args_owner = NULL;
    PyObject *kwds_owner = NULL;
    PyObject *base_view = NULL;
    PyObject *winner_owner = NULL;
    PyObject *chosen_bases_owner = NULL;
    PyObject *chosen_metatype_owner = NULL;
    PyObject *chosen_base_owner = NULL;
    PyObject *delegated = NULL;
    int result = -1;

    *selected_metatype_owner = NULL;
    *selected_base_owner = NULL;
    *type = NULL;
    if (PyErr_Occurred()) goto done;
    bases_owner = Py_NewRef(ctx->bases);
    if (bases_owner == NULL || PyErr_Occurred()) goto done;
    initial_metatype_owner = Py_NewRef((PyObject *)ctx->metatype);
    if (initial_metatype_owner == NULL || PyErr_Occurred()) goto done;
    args_owner = Py_NewRef(ctx->args);
    if (args_owner == NULL || PyErr_Occurred()) goto done;
    kwds_owner = Py_XNewRef(ctx->kwds);
    if (ctx->kwds != NULL && kwds_owner == NULL) goto done;
    if (PyErr_Occurred()) goto done;

    Py_ssize_t nbases = PyTuple_GET_SIZE(bases_owner);
    if (nbases == 0) {
        chosen_base_owner = (PyObject *)Py_NewRef(&PyBaseObject_Type);
        if (chosen_base_owner == NULL || PyErr_Occurred()) goto done;
        chosen_bases_owner = PyTuple_Pack(1, chosen_base_owner);
        if (chosen_bases_owner == NULL || PyErr_Occurred()) goto done;
        chosen_metatype_owner = Py_NewRef(initial_metatype_owner);
        if (chosen_metatype_owner == NULL || PyErr_Occurred()) goto done;
        result = 0;
        goto done;
    }

    for (Py_ssize_t i = 0; i < nbases; i++) {
        PyValue value = PyTuple_GET_VALUE(bases_owner, i);
        if (PyValue_IsObject(value) && PyType_Check(PyValue_AsPyObjectBorrow(value))) {
            continue;
        }
        base_view = PyValue_AsObject(value);
        if (base_view == NULL || PyErr_Occurred()) goto done;
        int has_mro_entries = PyObject_HasAttrWithError(base_view, &_Py_ID(__mro_entries__));
        type_mutation_close(&base_view);
        if (has_mro_entries < 0 || PyErr_Occurred()) goto done;
        if (has_mro_entries) {
            PyErr_SetString(PyExc_TypeError,
                            "type() doesn't support MRO entry resolution; "
                            "use types.new_class()");
            goto done;
        }
        if (PyErr_Occurred()) goto done;
    }

    PyTypeObject *winner = _PyType_CalculateMetaclass(ctx->metatype, bases_owner);
    if (winner == NULL || PyErr_Occurred()) goto done;
    winner_owner = Py_NewRef((PyObject *)winner);
    if (winner_owner == NULL || PyErr_Occurred()) goto done;

    if (winner != ctx->metatype && winner->tp_new != type_new) {
        if (winner->tp_new == NULL) {
            char message[512];
            PyOS_snprintf(message, sizeof(message),
                          "cannot create '%.400s' instances", winner->tp_name);
            PyErr_SetString(PyExc_TypeError, message);
            goto done;
        }
        delegated = winner->tp_new(winner, args_owner, kwds_owner);
        if (delegated == NULL || PyErr_Occurred()) goto done;
        result = 1;
        goto done;
    }

    PyTypeObject *base = find_best_base(bases_owner);
    if (base == NULL || PyErr_Occurred()) goto done;
    chosen_base_owner = Py_NewRef((PyObject *)base);
    if (chosen_base_owner == NULL || PyErr_Occurred()) goto done;
    chosen_metatype_owner = Py_NewRef(winner_owner);
    if (chosen_metatype_owner == NULL || PyErr_Occurred()) goto done;
    chosen_bases_owner = Py_NewRef(bases_owner);
    if (chosen_bases_owner == NULL || PyErr_Occurred()) goto done;
    result = 0;

done:
    type_mutation_close(&base_view);
    type_mutation_close(&winner_owner);
    type_mutation_close(&initial_metatype_owner);
    type_mutation_close(&args_owner);
    type_mutation_close(&kwds_owner);
    type_mutation_close(&bases_owner);
    if (PyErr_Occurred()) result = -1;
    if (result == 0) {
        ctx->base = _PyType_CAST(chosen_base_owner);
        ctx->metatype = _PyType_CAST(chosen_metatype_owner);
        ctx->bases = chosen_bases_owner;
        chosen_base_owner = NULL;
        chosen_metatype_owner = NULL;
        chosen_bases_owner = NULL;
        *selected_metatype_owner = (PyObject *)ctx->metatype;
        *selected_base_owner = (PyObject *)ctx->base;
    }
    else if (result == 1) {
        *type = delegated;
        delegated = NULL;
    }
    type_mutation_close(&chosen_bases_owner);
    type_mutation_close(&chosen_base_owner);
    type_mutation_close(&chosen_metatype_owner);
    type_mutation_close(&delegated);
    if (PyErr_Occurred()) {
        if (*selected_base_owner != NULL) {
            type_mutation_close(selected_base_owner);
            ctx->base = NULL;
        }
        if (*selected_metatype_owner != NULL) {
            type_mutation_close(selected_metatype_owner);
            ctx->metatype = NULL;
        }
        type_mutation_close(type);
        return -1;
    }
    return result;
}


static PyObject *
type_new(PyTypeObject *metatype, PyObject *args, PyObject *kwds)
{
    assert(args != NULL && PyTuple_Check(args));
    assert(kwds == NULL || PyDict_Check(kwds));

    PyObject *keywords = NULL;
    PyObject *owned_bases = NULL;
    PyObject *selected_metatype_owner = NULL;
    PyObject *selected_base_owner = NULL;
    PyObject *type = NULL;
    PyTypeObject *error_type = NULL;
    keywords = Py_XNewRef(kwds);
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError,
                     "type.__new__() takes exactly 3 arguments (%zd given)",
                     nargs);
        goto done;
    }

    /* The arguments must be a str, a tuple and a dict, which are never
     * inline, so the args tuple roots them and nothing is boxed.  Invalid
     * inline arguments need a type diagnostic, not a scalar box. */
    PyValue name_value = PyTuple_GET_VALUE(args, 0);
    PyValue bases_value = PyTuple_GET_VALUE(args, 1);
    PyValue dict_value = PyTuple_GET_VALUE(args, 2);
    PyValue invalid = PyValue_NULL;
    int invalid_arg = 0;
    if (!PyValue_IsObject(name_value) ||
            !PyUnicode_Check(PyValue_AsPyObjectBorrow(name_value))) {
        invalid = name_value;
        invalid_arg = 1;
    }
    else if (!PyValue_IsObject(bases_value) ||
            !PyTuple_Check(PyValue_AsPyObjectBorrow(bases_value))) {
        invalid = bases_value;
        invalid_arg = 2;
    }
    else if (!PyValue_IsObject(dict_value) ||
            !PyAnyDict_Check(PyValue_AsPyObjectBorrow(dict_value))) {
        invalid = dict_value;
        invalid_arg = 3;
    }
    if (invalid_arg != 0) {
        error_type = (PyTypeObject *)Py_NewRef(PyValue_TYPE(invalid));
        if (invalid_arg == 3) {
            PyErr_Format(PyExc_TypeError,
                         "type.__new__() argument 3 must be dict or frozendict, not %N",
                         (PyObject *)error_type);
        }
        else {
            const char *actual = PyValue_IsObject(invalid) &&
                PyValue_AsPyObjectBorrow(invalid) == Py_None
                ? "None" : error_type->tp_name;
            char message[256];
            PyOS_snprintf(message, sizeof(message),
                          "type.__new__() argument %d must be %s, not %.50s",
                          invalid_arg, invalid_arg == 1 ? "str" : "tuple",
                          actual);
            PyErr_SetString(PyExc_TypeError, message);
        }
        goto done;
    }

    type_new_ctx ctx = {
        .metatype = metatype,
        .args = args,
        .kwds = keywords,
        .orig_dict = PyValue_AsPyObjectBorrow(dict_value),
        .name = PyValue_AsPyObjectBorrow(name_value),
        .bases = PyValue_AsPyObjectBorrow(bases_value),
        .base = NULL,
        .slots = NULL,
        .nslot = 0,
        .add_dict = 0,
        .add_weak = 0,
        .may_add_dict = 0,
        .may_add_weak = 0};
    int res = type_new_get_bases(&ctx, &type,
                                 &selected_metatype_owner,
                                 &selected_base_owner);
    /* Only status 0 transfers a bases owner; failure and delegation leave
     * the initial borrowed tuple in ctx.bases. */
    if (res == 0) {
        owned_bases = ctx.bases;
    }
    if (res < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (res == 1) {
        assert(type != NULL);
        goto done;
    }
    assert(ctx.base != NULL);
    assert(ctx.bases != NULL);
    type = type_new_impl(&ctx);

done:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    Py_CLEAR(error_type);
    _PyValue_RestoreException(saved);
    type_mutation_close(&selected_base_owner);
    type_mutation_close(&selected_metatype_owner);
    saved = PyErr_GetRaisedException();
    Py_CLEAR(owned_bases);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_CLEAR(keywords);
    _PyValue_RestoreException(saved);
    return type;
}


static PyObject *
type_vectorcall(PyObject *metatype, PyObject *const *args,
                 size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs == 1 && metatype == (PyObject *)&PyType_Type){
        if (!_PyArg_NoKwnames("type", kwnames)) {
            return NULL;
        }
        return Py_NewRef(Py_TYPE(args[0]));
    }
    /* In other (much less common) cases, fall back to
       more flexible calling conventions. */
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_MakeTpCall(tstate, metatype, args, nargs, kwnames);
}

/* Align up to the nearest multiple of alignof(max_align_t)
 * (like _Py_ALIGN_UP, but for a size rather than pointer)
 */
static Py_ssize_t
_align_up(Py_ssize_t size)
{
    return (size + ALIGNOF_MAX_ALIGN_T - 1) & ~(ALIGNOF_MAX_ALIGN_T - 1);
}

static inline int
check_basicsize_includes_size_and_offsets(PyTypeObject* type)
{
    if (type->tp_alloc != PyType_GenericAlloc) {
        // Custom allocators can ignore tp_basicsize
        return 1;
    }
    Py_ssize_t max = (Py_ssize_t)type->tp_basicsize;

    if (type->tp_base && type->tp_base->tp_basicsize > type->tp_basicsize) {
        PyErr_Format(PyExc_TypeError,
                     "tp_basicsize for type '%s' (%zd) is too small for base '%s' (%zd)",
                     type->tp_name, type->tp_basicsize,
                     type->tp_base->tp_name, type->tp_base->tp_basicsize);
        return 0;
    }
    if (type->tp_weaklistoffset + (Py_ssize_t)sizeof(PyObject*) > max) {
        PyErr_Format(PyExc_TypeError,
                     "weaklist offset %zd is out of bounds for type '%s' (tp_basicsize = %zd)",
                     type->tp_weaklistoffset,
                     type->tp_name, type->tp_basicsize);
        return 0;
    }
    if (type->tp_dictoffset + (Py_ssize_t)sizeof(PyObject*) > max) {
        PyErr_Format(PyExc_TypeError,
                     "dict offset %zd is out of bounds for type '%s' (tp_basicsize = %zd)",
                     type->tp_dictoffset,
                     type->tp_name, type->tp_basicsize);
        return 0;
    }
    if (type->tp_vectorcall_offset + (Py_ssize_t)sizeof(vectorcallfunc*) > max) {
        PyErr_Format(PyExc_TypeError,
                     "vectorcall offset %zd is out of bounds for type '%s' (tp_basicsize = %zd)",
                     type->tp_vectorcall_offset,
                     type->tp_name, type->tp_basicsize);
        return 0;
    }
    return 1;
}

static int
check_immutable_bases(const char *type_name, PyObject *bases, int skip_first)
{
    PyObject *owner = Py_NewRef(bases);
    PyObject *error_type = NULL;
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t i = skip_first ? 1 : 0; i < PyTuple_GET_SIZE(owner); i++) {
        PyValue item = PyTuple_GET_VALUE(owner, i);
        if (!PyValue_IsObject(item) || !PyType_Check(PyValue_AsPyObjectBorrow(item))) {
            error_type = Py_NewRef(PyValue_TYPE(item));
            PyErr_Format(PyExc_TypeError, "bases must be types; got '%N'", error_type);
            goto done;
        }
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        if (!_PyType_HasFeature(base, Py_TPFLAGS_IMMUTABLETYPE)) {
            PyErr_Format(PyExc_TypeError,
                         "Creating immutable type %s from mutable base %N", type_name, base);
            goto done;
        }
    }
    result = 0;
done:
    type_mutation_close(&error_type);
    type_mutation_close(&owner);
    return PyErr_Occurred() ? -1 : result;
}


/* Set *dest to the offset specified by a special "__*offset__" member.
 * Return 0 on success, -1 on failure.
 */
static inline int
special_offset_from_member(
            const PyMemberDef *memb /* may be NULL */,
            Py_ssize_t type_data_offset,
            Py_ssize_t *dest /* not NULL */)
{
    if (memb == NULL) {
        *dest = 0;
        return 0;
    }
    if (memb->type != Py_T_PYSSIZET) {
        PyErr_Format(
            PyExc_SystemError,
            "type of %s must be Py_T_PYSSIZET",
            memb->name);
        return -1;
    }
    if (memb->flags == Py_READONLY) {
        *dest = memb->offset;
        return 0;
    }
    else if (memb->flags == (Py_READONLY | Py_RELATIVE_OFFSET)) {
        *dest = memb->offset + type_data_offset;
        return 0;
    }
    PyErr_Format(
        PyExc_SystemError,
        "flags for %s must be Py_READONLY or (Py_READONLY | Py_RELATIVE_OFFSET)",
        memb->name);
    return -1;
}


static PyObject *
type_from_slots_or_spec(
    PySlot *slots, PyType_Spec *spec,
    PyTypeObject *metaclass, PyObject *module, PyObject *bases_in)
{
    /* Invariant: A non-NULL value in one of these means this function holds
     * a strong reference or owns allocated memory.
     * These get decrefed/freed/returned at the end, on both success and error.
     */
    PyHeapTypeObject *res = NULL;
    PyTypeObject *type;
    PyObject *bases = NULL;
    char *tp_doc = NULL;
    PyObject *ht_name = NULL;
    char *_ht_tpname = NULL;

    int r;

    /* First pass of slots */

    Py_ssize_t nmembers = 0;
    const PyMemberDef *weaklistoffset_member = NULL;
    const PyMemberDef *dictoffset_member = NULL;
    const PyMemberDef *vectorcalloffset_member = NULL;
    Py_ssize_t basicsize = 0;
    Py_ssize_t extra_basicsize = 0;
    Py_ssize_t itemsize = 0;
    int flags = 0;
    void *token = NULL;

    bool have_relative_members = false;
    Py_ssize_t max_relative_offset = 0;

    PyObject *bases_slot = NULL; /* borrowed from the slots */

    _PySlotIterator it;

    if (spec) {
        assert(!slots);
        if (spec->basicsize > 0) {
            basicsize = spec->basicsize;
        }
        if (spec->basicsize < 0) {
            extra_basicsize = -spec->basicsize;
        }
        itemsize = spec->itemsize;
        flags = spec->flags;
        _PySlotIterator_InitLegacy(&it, spec->slots, _PySlot_KIND_TYPE);
        it.name = spec->name;
    }
    else {
        assert(!spec);
        assert(!metaclass);
        assert(!module);
        assert(!bases_in);
        _PySlotIterator_Init(&it, slots, _PySlot_KIND_TYPE);
    }

    #define NO_SPEC                                         \
        if (spec) {                                         \
            PyErr_Format(                                   \
                PyExc_SystemError,                          \
                "%s must not be used with PyType_Spec",     \
                _PySlot_GetName(it.current.sl_id));         \
            goto finally;                                   \
        }                                                   \
        /////////////////////////////////////////////////////

    while (_PySlotIterator_Next(&it)) {
        switch (it.current.sl_id) {
        case Py_slot_invalid:
            goto finally;
        case Py_tp_name:
            NO_SPEC;
            it.name = it.current.sl_ptr;
            break;
        case Py_tp_metaclass:
            NO_SPEC;
            metaclass = it.current.sl_ptr;
            break;
        case Py_tp_module:
            NO_SPEC;
            module = it.current.sl_ptr;
            break;
        case Py_tp_bases:
            bases_slot = it.current.sl_ptr;
            break;
        case Py_tp_base:
            if (!_PySlotIterator_SawSlot(&it, Py_tp_bases)) {
                bases_slot = it.current.sl_ptr;
            }
            break;
        case Py_tp_basicsize:
            NO_SPEC;
            basicsize = it.current.sl_size;
            if (basicsize <= 0) {
                PyErr_SetString(
                    PyExc_SystemError,
                    "Py_tp_basicsize must be positive");
                goto finally;
            }
            break;
        case Py_tp_extra_basicsize:
            NO_SPEC;
            extra_basicsize = it.current.sl_size;
            if (extra_basicsize <= 0) {
                PyErr_SetString(
                    PyExc_SystemError,
                    "Py_tp_extra_basicsize must be positive");
                goto finally;
            }
            break;
        case Py_tp_itemsize:
            NO_SPEC;
            itemsize = it.current.sl_size;
            if (itemsize <= 0) {
                PyErr_SetString(
                    PyExc_SystemError,
                    "Py_tp_itemsize must be positive");
                goto finally;
            }
            break;
        case Py_tp_flags:
            NO_SPEC;
            flags = (int)it.current.sl_uint64;
            break;
        case Py_tp_members:
            for (const PyMemberDef *memb = it.current.sl_ptr;
                 memb->name != NULL;
                 memb++)
            {
                nmembers++;
                if (memb->flags & Py_RELATIVE_OFFSET) {
                    if (memb->offset < 0) {
                        PyErr_SetString(
                            PyExc_SystemError,
                            "Member offset must not be negative");
                        goto finally;
                    }
                    have_relative_members = true;
                    max_relative_offset = Py_MAX(max_relative_offset,
                                                 memb->offset);
                }
                if (strcmp(memb->name, "__weaklistoffset__") == 0) {
                    weaklistoffset_member = memb;
                }
                else if (strcmp(memb->name, "__dictoffset__") == 0) {
                    dictoffset_member = memb;
                }
                else if (strcmp(memb->name, "__vectorcalloffset__") == 0) {
                    vectorcalloffset_member = memb;
                }
            }
            break;
        case Py_tp_token:
            token = it.current.sl_ptr;
            if (token == Py_TP_USE_SPEC) {
                if (!spec) {
                    PyErr_SetString(
                        PyExc_SystemError,
                        "Py_tp_token: Py_TP_USE_SPEC (NULL) can only be "
                        "used with PyType_Spec");
                    goto finally;
                }
                token = spec;
            }
            break;
        case Py_tp_doc:
            /* For the docstring slot, which usually points to a static string
               literal, we need to make a copy */
            if (it.current.sl_ptr == NULL) {
                PyMem_Free(tp_doc);
                tp_doc = NULL;
            }
            else {
                size_t len = strlen(it.current.sl_ptr)+1;
                tp_doc = PyMem_Malloc(len);
                if (tp_doc == NULL) {
                    PyErr_NoMemory();
                    goto finally;
                }
                memcpy(tp_doc, it.current.sl_ptr, len);
            }
            break;
        }
    }
    #undef NO_SPEC

    /* Required slots & bad combinations */

    if (it.name == NULL) {
        if (spec) {
            PyErr_SetString(PyExc_SystemError,
                            "Type spec does not define the name field.");
        }
        else {
            PyErr_SetString(PyExc_SystemError,
                            "Py_tp_name slot is required.");
        }
        goto finally;
    }

    if (_PySlotIterator_SawSlot(&it, Py_tp_basicsize)
        && _PySlotIterator_SawSlot(&it, Py_tp_extra_basicsize))
    {
        PyErr_Format(
            PyExc_SystemError,
            "type %s: Py_tp_basicsize and Py_tp_extra_basicsize are "
            "mutually exclusive",
            it.name);
        goto finally;
    }

    if (have_relative_members) {
        if (!extra_basicsize) {
            PyErr_SetString(
                PyExc_SystemError,
                "With Py_RELATIVE_OFFSET, basicsize must be extended");
            goto finally;
        }
        if (max_relative_offset >= extra_basicsize) {
            PyErr_SetString(
                PyExc_SystemError,
                "Member offset out of range (0..extra_basicsize)");
            goto finally;
        }
    }

    /* Prepare the type name and qualname */

    assert(it.name);
    const char *s = strrchr(it.name, '.');
    if (s == NULL) {
        s = it.name;
    }
    else {
        s++;
    }

    ht_name = PyUnicode_FromString(s);
    if (!ht_name) {
        goto finally;
    }

    /* Copy the name to a buffer we own.
    *
    * Unfortunately, we can't use tp_name directly (with some
    * flag saying that it should be deallocated with the type),
    * because tp_name is public API and may be set independently
    * of any such flag.
    * So, we use a separate buffer, _ht_tpname, that's always
    * deallocated with the type (if it's non-NULL).
    */
    Py_ssize_t name_buf_len = strlen(it.name) + 1;
    _ht_tpname = PyMem_Malloc(name_buf_len);
    if (_ht_tpname == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    memcpy(_ht_tpname, it.name, name_buf_len);

    /* Get a tuple of bases.
     * bases is a strong reference (unlike bases_in).
     * (This is convoluted for backwards compatibility -- preserving priority
     * of the various ways to specify bases)
     */
    if (!bases_in) {
        bases_in = bases_slot;
    }
    if (bases_in) {
        if (PyTuple_Check(bases_in)) {
            bases = Py_NewRef(bases_in);
        }
        else {
            bases = PyTuple_Pack(1, bases_in);
        }
    }
    else {
        bases = PyTuple_Pack(1, &PyBaseObject_Type);
    }
    if (!bases) {
        goto finally;
    }

    /* If this is an immutable type, check if all bases are also immutable.
     * (This isn't necessary for static types: those can't have heap bases,
     * and only heap types can be mutable.)
     */
    if (flags & Py_TPFLAGS_IMMUTABLETYPE) {
        if (check_immutable_bases(it.name, bases, 0) < 0) {
            goto finally;
        }
    }

    /* Calculate best base, and check that all bases are type objects */
    PyTypeObject *base = find_best_base(bases);  // borrowed ref
    if (base == NULL) {
        goto finally;
    }

    /* Calculate the metaclass */

    if (!metaclass) {
        metaclass = &PyType_Type;
    }
    metaclass = _PyType_CalculateMetaclass(metaclass, bases);
    if (metaclass == NULL) {
        goto finally;
    }
    if (!PyType_Check(metaclass)) {
        PyErr_Format(PyExc_TypeError,
                     "Metaclass '%R' is not a subclass of 'type'.",
                     metaclass);
        goto finally;
    }
    if (metaclass->tp_new && metaclass->tp_new != PyType_Type.tp_new) {
        PyErr_SetString(
            PyExc_TypeError,
            "Metaclasses with custom tp_new are not supported.");
        goto finally;
    }

    // find_best_base() should check Py_TPFLAGS_BASETYPE & raise a proper
    // exception, here we just check its work
    assert(_PyType_HasFeature(base, Py_TPFLAGS_BASETYPE));

    /* Calculate sizes */

    Py_ssize_t type_data_offset = basicsize;
    if (extra_basicsize) {
        /* Extend */
        assert(basicsize == 0);
        type_data_offset = _align_up(base->tp_basicsize);
        basicsize = type_data_offset + _align_up(extra_basicsize);

        /* Inheriting variable-sized types is limited */
        if (base->tp_itemsize
            && !((base->tp_flags | flags) & Py_TPFLAGS_ITEMS_AT_END))
        {
            PyErr_SetString(
                PyExc_SystemError,
                "Cannot extend variable-size class without Py_TPFLAGS_ITEMS_AT_END.");
            goto finally;
        }
    }
    if (basicsize == 0) {
        /* Inherit */
        basicsize = base->tp_basicsize;
    }

    /* Compute special offsets */

    Py_ssize_t weaklistoffset = 0;
    if (special_offset_from_member(weaklistoffset_member, type_data_offset,
                                  &weaklistoffset) < 0) {
        goto finally;
    }
    Py_ssize_t dictoffset = 0;
    if (special_offset_from_member(dictoffset_member, type_data_offset,
                                  &dictoffset) < 0) {
        goto finally;
    }
    Py_ssize_t vectorcalloffset = 0;
    if (special_offset_from_member(vectorcalloffset_member, type_data_offset,
                                  &vectorcalloffset) < 0) {
        goto finally;
    }

    /* Allocate the new type
     *
     * Between here and PyType_Ready, we should limit:
     * - calls to Python code
     * - raising exceptions
     * - memory allocations
     */

    res = (PyHeapTypeObject*)metaclass->tp_alloc(metaclass, nmembers);
    if (res == NULL) {
        goto finally;
    }

    type = &res->ht_type;
    /* The flags must be initialized early, before the GC traverses us */
    type_set_flags(type, flags | Py_TPFLAGS_HEAPTYPE);
    res->ht_mro_epoch = 0;

    res->ht_module = Py_XNewRef(module);

    /* Initialize essential fields */

    type->tp_as_async = &res->as_async;
    type->tp_as_number = &res->as_number;
    type->tp_as_sequence = &res->as_sequence;
    type->tp_as_mapping = &res->as_mapping;
    type->tp_as_buffer = &res->as_buffer;

    /* Set slots we have prepared */

    if (type->tp_base != base) {
        mro_epoch_advance(type);
    }
    type->tp_base = (PyTypeObject *)Py_NewRef(base);
    set_tp_bases(type, bases, 1);
    bases = NULL;  // We give our reference to bases to the type

    type->tp_doc = tp_doc;
    tp_doc = NULL;  // Give ownership of the allocated memory to the type

    res->ht_qualname = Py_NewRef(ht_name);
    res->ht_name = ht_name;
    ht_name = NULL;  // Give our reference to the type

    type->tp_name = _ht_tpname;
    res->_ht_tpname = _ht_tpname;
    _ht_tpname = NULL;  // Give ownership to the type

    res->ht_token = token;

    /* Copy the sizes */

    type->tp_basicsize = basicsize;
    type->tp_itemsize = itemsize;

    /* Second pass of slots: copy most of them into the type */

    _PySlotIterator_Rewind(&it, spec ? (void*)spec->slots : (void*)slots);
    while (_PySlotIterator_Next(&it)) {
        switch (it.current.sl_id) {
        case Py_slot_invalid:
            goto finally;
        case Py_tp_base:
        case Py_tp_bases:
        case Py_tp_doc:
            /* Processed above */
            break;
        case Py_tp_members:
            {
                /* Move the slots to the heap type itself */
                size_t len = Py_TYPE(type)->tp_itemsize * nmembers;
                memcpy(_PyHeapType_GET_MEMBERS(res), it.current.sl_ptr, len);
                type->tp_members = _PyHeapType_GET_MEMBERS(res);
                PyMemberDef *memb;
                Py_ssize_t i;
                for (memb = _PyHeapType_GET_MEMBERS(res), i = nmembers;
                     i > 0; ++memb, --i)
                {
                    if (memb->flags & Py_RELATIVE_OFFSET) {
                        memb->flags &= ~Py_RELATIVE_OFFSET;
                        memb->offset += type_data_offset;
                    }
                }
            }
            break;
        default:
            _PySlot_heaptype_apply_field_slot(res, it.current);
            break;
        }
    }
    if (type->tp_dealloc == NULL) {
        /* It's a heap type, so needs the heap types' dealloc.
           subtype_dealloc will call the base type's tp_dealloc, if
           necessary. */
        type->tp_dealloc = subtype_dealloc;
    }

    /* Set up offsets */

    type->tp_vectorcall_offset = vectorcalloffset;
    type->tp_weaklistoffset = weaklistoffset;
    type->tp_dictoffset = dictoffset;

#ifdef Py_GIL_DISABLED
    // Assign a unique id to enable per-thread refcounting
    res->unique_id = _PyObject_AssignUniqueId((PyObject *)res);
#endif

    /* Ready the type (which includes inheritance).
     *
     * After this call we should generally only touch up what's
     * accessible to Python code, like __dict__.
     */
    if (type_ready_publish(type, 0) < 0) {
        goto finally;
    }

    if (!check_basicsize_includes_size_and_offsets(type)) {
        goto finally;
    }

    PyObject *dict = lookup_tp_dict(type);
    if (type->tp_doc) {
        PyObject *__doc__ = PyUnicode_FromString(_PyType_DocWithoutSignature(type->tp_name, type->tp_doc));
        if (!__doc__) {
            goto finally;
        }
        r = PyDict_SetItem(dict, &_Py_ID(__doc__), __doc__);
        Py_DECREF(__doc__);
        if (r < 0) {
            goto finally;
        }
    }

    if (weaklistoffset) {
        if (PyDict_DelItem(dict, &_Py_ID(__weaklistoffset__)) < 0) {
            goto finally;
        }
    }
    if (dictoffset) {
        if (PyDict_DelItem(dict, &_Py_ID(__dictoffset__)) < 0) {
            goto finally;
        }
    }

    /* Set type.__module__ */
    r = PyDict_Contains(dict, &_Py_ID(__module__));
    if (r < 0) {
        goto finally;
    }
    if (r == 0) {
        s = strrchr(it.name, '.');
        if (s != NULL) {
            PyObject *modname = PyUnicode_FromStringAndSize(
                    it.name, (Py_ssize_t)(s - it.name));
            if (modname == NULL) {
                goto finally;
            }
            r = PyDict_SetItem(dict, &_Py_ID(__module__), modname);
            Py_DECREF(modname);
            if (r != 0) {
                goto finally;
            }
        }
        else {
            if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                    "builtin type %.200s has no __module__ attribute",
                    it.name))
                goto finally;
        }
    }

    assert(_PyType_CheckConsistency(type));

finally:
    if (PyErr_Occurred()) {
        Py_CLEAR(res);
    }
    Py_XDECREF(bases);
    PyMem_Free(tp_doc);
    Py_XDECREF(ht_name);
    PyMem_Free(_ht_tpname);
    return (PyObject*)res;
}

PyObject *
PyType_FromSlots(PySlot *slots)
{
    return type_from_slots_or_spec(slots, NULL, NULL, NULL, NULL);
}

PyObject *
PyType_FromMetaclass(
    PyTypeObject *metaclass, PyObject *module,
    PyType_Spec *spec, PyObject *bases)
{
    return type_from_slots_or_spec(NULL, spec, metaclass, module, bases);
}

PyObject *
PyType_FromModuleAndSpec(PyObject *module, PyType_Spec *spec, PyObject *bases)
{
    return type_from_slots_or_spec(NULL, spec, NULL, module, bases);
}

PyObject *
PyType_FromSpecWithBases(PyType_Spec *spec, PyObject *bases)
{
    return type_from_slots_or_spec(NULL, spec, NULL, NULL, bases);
}

PyObject *
PyType_FromSpec(PyType_Spec *spec)
{
    return type_from_slots_or_spec(NULL, spec, NULL, NULL, NULL);
}

PyObject *
PyType_GetName(PyTypeObject *type)
{
    return type_name((PyObject *)type, NULL);
}

PyObject *
PyType_GetQualName(PyTypeObject *type)
{
    return type_qualname((PyObject *)type, NULL);
}

PyObject *
PyType_GetModuleName(PyTypeObject *type)
{
    return type_module(type);
}

void *
PyType_GetSlot(PyTypeObject *type, int slot_in)
{
    uint16_t slot = _PySlot_resolve_type_slot(slot_in);
    return _PySlot_type_getslot(type, slot);
}

PyObject *
PyType_GetModule_DuringGC(PyTypeObject *type)
{
    assert(PyType_Check(type));
    if (!_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        return NULL;
    }
    PyHeapTypeObject* et = (PyHeapTypeObject*)type;
    return et->ht_module;
}

PyObject *
PyType_GetModule(PyTypeObject *type)
{
    assert(PyType_Check(type));
    if (!_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        PyErr_Format(
            PyExc_TypeError,
            "PyType_GetModule: Type '%s' is not a heap type",
            type->tp_name);
        return NULL;
    }

    PyHeapTypeObject* et = (PyHeapTypeObject*)type;
    if (!et->ht_module) {
        PyErr_Format(
            PyExc_TypeError,
            "PyType_GetModule: Type '%s' has no associated module",
            type->tp_name);
        return NULL;
    }
    return et->ht_module;
}

void *
PyType_GetModuleState_DuringGC(PyTypeObject *type)
{
    PyObject *m = PyType_GetModule_DuringGC(type);
    if (m == NULL) {
        return NULL;
    }
    return _PyModule_GetState(m);
}

void *
PyType_GetModuleState(PyTypeObject *type)
{
    PyObject *m = PyType_GetModule(type);
    if (m == NULL) {
        return NULL;
    }
    return _PyModule_GetState(m);
}

/* Return borrowed ref to the module of the first superclass where the module
 * has the given token.
 */
PyObject *
PyType_GetModuleByToken_DuringGC(PyTypeObject *type, const void *token)
{
    assert(PyType_Check(type));

    if (!_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        // type_ready_mro() ensures that no heap type is
        // contained in a static type MRO.
        return NULL;
    }
    else {
        PyHeapTypeObject *ht = (PyHeapTypeObject*)type;
        PyObject *module = ht->ht_module;
        if (module && _PyModule_GetToken(module) == token) {
            return module;
        }
    }

    PyObject *res = NULL;
    BEGIN_TYPE_LOCK();

    PyObject *mro = lookup_tp_mro(type);
    // The type must be ready
    assert(mro != NULL);
    assert(PyTuple_Check(mro));
    // mro_invoke() ensures that the type MRO cannot be empty.
    assert(PyTuple_GET_SIZE(mro) >= 1);
    // Also, the first item in the MRO is the type itself, which
    // we already checked above. We skip it in the loop.
    PyValue first = PyTuple_GET_VALUE(mro, 0);
    assert(PyValue_IsObject(first));
    assert(PyValue_AsPyObjectBorrow(first) == (PyObject *)type);

    Py_ssize_t n = PyTuple_GET_SIZE(mro);
    for (Py_ssize_t i = 1; i < n; i++) {
        PyValue super_value = PyTuple_GET_VALUE(mro, i);
        assert(PyValue_IsObject(super_value));
        PyObject *super_obj = PyValue_AsPyObjectBorrow(super_value);
        assert(PyType_Check(super_obj));
        PyTypeObject *super = (PyTypeObject *)super_obj;
        if (!_PyType_HasFeature(super, Py_TPFLAGS_HEAPTYPE)) {
            // Static types in the MRO need to be skipped
            continue;
        }

        PyHeapTypeObject *ht = (PyHeapTypeObject*)super;
        PyObject *module = ht->ht_module;
        if (module && _PyModule_GetToken(module) == token) {
            res = module;
            break;
        }
    }
    END_TYPE_LOCK();

    return res;
}

PyObject *
PyType_GetModuleByToken(PyTypeObject *type, const void *token)
{
    return Py_XNewRef(PyType_GetModuleByDef(type, (PyModuleDef *)token));
}

PyObject *
PyType_GetModuleByDef(PyTypeObject *type, PyModuleDef *def)
{
    PyObject *mod = PyType_GetModuleByToken_DuringGC(type, def);
    if (!mod) {
        PyErr_Format(
            PyExc_TypeError,
            "PyType_GetModuleByDef: No superclass of '%s' has the given module",
            type->tp_name);
        return NULL;
    }
    return mod;
}


static PyTypeObject *
get_base_by_token_recursive(PyObject *bases, void *token)
{
    assert(bases != NULL);
    PyTypeObject *res = NULL;
    Py_ssize_t n = PyTuple_GET_SIZE(bases);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue value = PyTuple_GET_VALUE(bases, i);
        assert(PyValue_IsObject(value));
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(value));
        if (!_PyType_HasFeature(base, Py_TPFLAGS_HEAPTYPE)) {
            continue;
        }
        if (((PyHeapTypeObject*)base)->ht_token == token) {
            res = base;
            break;
        }
        base = get_base_by_token_recursive(lookup_tp_bases(base), token);
        if (base != NULL) {
            res = base;
            break;
        }
    }
    return res;  // Prefer to return recursively from one place
}

int
PyType_GetBaseByToken_DuringGC(PyTypeObject *type, void *token, PyTypeObject **result)
{
    if (result != NULL) {
        *result = NULL;
    }
    if (token == NULL) {
        return -1;
    }
    if (!PyType_Check(type)) {
        return -1;
    }

    if (!_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
        // No static type has a heaptype superclass,
        // which is ensured by type_ready_mro().
        return 0;
    }
    if (((PyHeapTypeObject*)type)->ht_token == token) {
found:
        if (result != NULL) {
            *result = type;
        }
        return 1;
    }

    PyObject *mro = type->tp_mro;  // No lookup, following PyType_IsSubtype()
    if (mro == NULL) {
        PyTypeObject *base;
        base = get_base_by_token_recursive(lookup_tp_bases(type), token);
        if (base != NULL) {
            // Copying the given type can cause a slowdown,
            // unlike the overwrite below.
            type = base;
            goto found;
        }
        return 0;
    }
    // mro_invoke() ensures that the type MRO cannot be empty.
    assert(PyTuple_GET_SIZE(mro) >= 1);
    // Also, the first item in the MRO is the type itself, which
    // we already checked above. We skip it in the loop.
    PyValue first = PyTuple_GET_VALUE(mro, 0);
    assert(PyValue_IsObject(first));
    assert(PyValue_AsPyObjectBorrow(first) == (PyObject *)type);
    Py_ssize_t n = PyTuple_GET_SIZE(mro);
    for (Py_ssize_t i = 1; i < n; i++) {
        PyValue value = PyTuple_GET_VALUE(mro, i);
        assert(PyValue_IsObject(value));
        PyTypeObject *base = (PyTypeObject *)PyValue_AsPyObjectBorrow(value);
        if (_PyType_HasFeature(base, Py_TPFLAGS_HEAPTYPE)
            && ((PyHeapTypeObject*)base)->ht_token == token) {
            type = base;
            goto found;
        }
    }
    return 0;
}

int
PyType_GetBaseByToken(PyTypeObject *type, void *token, PyTypeObject **result)
{
    if (result != NULL) {
         *result = NULL;
    }
    if (token == NULL) {
        PyErr_Format(PyExc_SystemError,
                     "PyType_GetBaseByToken called with token=NULL");
        return -1;
    }
    if (!PyType_Check(type)) {
        PyErr_Format(PyExc_TypeError,
                     "expected a type, got a '%T' object", type);
        return -1;
    }

    int res = PyType_GetBaseByToken_DuringGC(type, token, result);
    if (res > 0 && result) {
        Py_INCREF(*result);
    }
    return res;
}


void *
PyObject_GetTypeData_DuringGC(PyObject *obj, PyTypeObject *cls)
{
    assert(PyObject_TypeCheck(obj, cls));
    return (char *)obj + _align_up(cls->tp_base->tp_basicsize);
}

void *
PyObject_GetTypeData(PyObject *obj, PyTypeObject *cls)
{
    return PyObject_GetTypeData_DuringGC(obj, cls);
}

Py_ssize_t
PyType_GetTypeDataSize(PyTypeObject *cls)
{
    ptrdiff_t result = cls->tp_basicsize - _align_up(cls->tp_base->tp_basicsize);
    if (result < 0) {
        return 0;
    }
    return result;
}

static inline void *
getitemdata(PyObject *obj, bool raise)
{
    if (!_PyType_HasFeature(Py_TYPE(obj), Py_TPFLAGS_ITEMS_AT_END)) {
        if (raise) {
            PyErr_Format(PyExc_TypeError,
                         "type '%T' does not have Py_TPFLAGS_ITEMS_AT_END",
                         obj);
        }
        return NULL;
    }
    return (char *)obj + Py_TYPE(obj)->tp_basicsize;
}

void *
PyObject_GetItemData_DuringGC(PyObject *obj)
{
    return getitemdata(obj, false);
}

void *
PyObject_GetItemData(PyObject *obj)
{
    return getitemdata(obj, true);
}

/* Internal API to look for a name through the MRO, bypassing the method cache.
   The result is stored as a PyValue in `out`. It never set an exception.
   Returns -1 if there was an error, 0 if the name was not found, and 1 if
   the name was found. */
static int
find_name_in_mro(PyTypeObject *type, PyObject *name, PyValue *out)
{
    Py_hash_t hash = _PyObject_HashDictKey(name);
    if (hash == -1) {
        PyErr_Clear();
        return -1;
    }

    /* Look in tp_dict of types in MRO */
    PyObject *mro = lookup_tp_mro(type);
    if (mro == NULL) {
        if (!is_readying(type)) {
            if (PyType_Ready(type) < 0) {
                PyErr_Clear();
                return -1;
            }
            mro = lookup_tp_mro(type);
        }
        if (mro == NULL) {
            return -1;
        }
    }

    int res = 0;
    PyThreadState *tstate = _PyThreadState_GET();
    /* Keep a strong reference to mro because type->tp_mro can be replaced
       during dict lookup, e.g. when comparing to non-string keys. */
    _PyCValue mro_ref;
    _PyThreadState_PushCValue(tstate, &mro_ref);
    mro_ref.ref = PyValue_FromPyObjectNew(mro);
    Py_ssize_t n = PyTuple_GET_SIZE(mro);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *base = _PyTuple_GET_OBJECT(mro, i);
        PyObject *dict = lookup_tp_dict(_PyType_CAST(base));
        assert(dict && PyDict_Check(dict));
        Py_ssize_t ix = _Py_dict_lookup_threadsafe_value(
            (PyDictObject *)dict, name, hash, out);
        if (ix == DKIX_ERROR) {
            PyErr_Clear();
            res = -1;
            goto done;
        }
        if (!PyValue_IsNull(*out)) {
            res = 1;
            break;
        }
    }
done:
    _PyThreadState_PopCValue(tstate, &mro_ref);
    return res;
}

int
_PyType_TryLookupValueNoAlloc(PyTypeObject *type, PyObject *name,
                             PyValue *out, unsigned int *version)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    (void)_PyType_GetVersionForCurrentState(interp, type);
    *out = PyValue_NULL;
    *version = 0;
#ifdef Py_GIL_DISABLED
    return 0;
#else
    if (PyErr_Occurred()) {
        return -1;
    }
    if (!PyUnicode_CheckExact(name) ||
        PyUnstable_Unicode_GET_CACHED_HASH(name) == -1 ||
        !(type->tp_flags & Py_TPFLAGS_READY)) {
        return 0;
    }
    PyObject *mro = lookup_tp_mro(type);
    if (mro == NULL || !PyTuple_CheckExact(mro)) {
        return 0;
    }
    /* Preflight every base before the first lookup: a general-key table
     * can call arbitrary equality code even with an exact Unicode query. */
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(mro); i++) {
        PyValue base = PyTuple_GET_VALUE(mro, i);
        if (!PyValue_IsObject(base) || !PyType_Check(PyValue_AsPyObjectBorrow(base))) {
            return 0;
        }
        PyObject *dict = lookup_tp_dict((PyTypeObject *)PyValue_AsPyObjectBorrow(base));
        if (dict == NULL || !PyDict_CheckExact(dict) ||
            ((PyDictObject *)dict)->ma_keys->dk_kind == DICT_KEYS_GENERAL) {
            return 0;
        }
    }
    if (!PyUnstable_Type_AssignVersionTag(type)) {
        return 0;
    }
    unsigned int expected = _PyType_GetVersionForCurrentState(interp, type);
    int found = find_name_in_mro(type, name, out);
    assert(found >= 0 && !PyErr_Occurred());
    *version = _PyType_MatchesVersion(interp, type, expected) ? expected : 0;
    return found < 0 ? 0 : 1;
#endif
}

/* Check if the "readied" PyUnicode name
   is a double-underscore special name. */
static int
is_dunder_name(PyObject *name)
{
    Py_ssize_t length = PyUnicode_GET_LENGTH(name);
    int kind = PyUnicode_KIND(name);
    /* Special names contain at least "__x__" and are always ASCII. */
    if (length > 4 && kind == PyUnicode_1BYTE_KIND) {
        const Py_UCS1 *characters = PyUnicode_1BYTE_DATA(name);
        return (
            ((characters[length-2] == '_') && (characters[length-1] == '_')) &&
            ((characters[0] == '_') && (characters[1] == '_'))
        );
    }
    return 0;
}

/* Owning object bridge for MRO lookup. NULL means absence or an exception
   from lookup, scalar boxing or cleanup. A nonzero output version is still
   admitted after those callouts, not merely the raw tag observed on entry. */
PyObject *
_PyType_LookupRefAndVersion(PyTypeObject *type, PyObject *name, unsigned int *version)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    (void)_PyType_GetVersionForCurrentState(interp, type);
    Py_INCREF(type);
    Py_INCREF(name);
    PyValue out;
    unsigned int expected = _PyType_LookupValueAndVersion(type, name, &out);
    PyObject *result = NULL;
    if (!PyValue_IsNull(out)) {
        result = PyValue_AsObject(out);
    }
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(out);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_DECREF(name);
    _PyValue_RestoreException(saved);
    unsigned int current = 0;
    if (!PyErr_Occurred() && Py_REFCNT(type) != 1) {
        current = _PyType_MatchesVersion(interp, type, expected) ? expected : 0;
    }
    saved = PyErr_GetRaisedException();
    Py_DECREF(type);
    _PyValue_RestoreException(saved);
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        Py_XDECREF(result);
        _PyValue_RestoreException(saved);
        result = NULL;
        current = 0;
    }
    if (version != NULL) {
        *version = current;
    }
    return result;
}

static int
should_assign_version_tag(PyTypeObject *type, PyObject *name, unsigned int version_tag)
{
    return (version_tag == 0
        && FT_ATOMIC_LOAD_UINT16_RELAXED(type->tp_versions_used) < MAX_VERSIONS_PER_CLASS
        && MCACHE_CACHEABLE_NAME(name));
}

unsigned int
_PyType_LookupValueAndVersion(PyTypeObject *type, PyObject *name, PyValue *out)
{
    *out = PyValue_NULL;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    (void)_PyType_GetVersionForCurrentState(interp, type);
    if (PyErr_Occurred()) {
        return 0;
    }
    int cacheable = MCACHE_CACHEABLE_NAME(name);
    if (cacheable) {
        struct _PyTypeCacheLookupResult r = _PyTypeCache_Lookup(type, name);
        if (r.cache_hit) {
            OBJECT_STAT_INC_COND(type_cache_hits, !is_dunder_name(name));
            OBJECT_STAT_INC_COND(type_cache_dunder_hits, is_dunder_name(name));
            *out = r.value;
            return r.version_tag;
        }
    }
    OBJECT_STAT_INC_COND(type_cache_misses, !is_dunder_name(name));
    OBJECT_STAT_INC_COND(type_cache_dunder_misses, is_dunder_name(name));

    // Keep the semantic source and receiver alive through allocator/free hooks.
    Py_INCREF(type);
    Py_INCREF(name);
    PyValue result = PyValue_NULL;
    unsigned int expected = 0;
    int res;
    BEGIN_TYPE_LOCK();
    if (cacheable) {
        unsigned int current = _PyType_GetVersionForCurrentState(interp, type);
        if (current != 0 || should_assign_version_tag(type, name, current)) {
            assign_version_tag(interp, type);
            expected = _PyType_GetVersionForCurrentState(interp, type);
        }
    }
    res = find_name_in_mro(type, name, &result);
    if (res >= 0 && !PyErr_Occurred() &&
        _PyType_MatchesVersion(interp, type, expected)) {
        _PyTypeCache_Insert(type, name, result, expected);
    }
    END_TYPE_LOCK();

    // Name/error cleanup may invalidate the selected version. Retain type and
    // the owned result until those callbacks have finished.
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(name);
    _PyValue_RestoreException(saved);
    if (res < 0 || PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(result);
        result = PyValue_NULL;
        _PyValue_RestoreException(saved);
        expected = 0;
    }
    unsigned int version = 0;
    if (Py_REFCNT(type) != 1) {
        version = _PyType_MatchesVersion(interp, type, expected) ? expected : 0;
    }
    // A final close cannot be followed by any type access or version lookup.
    saved = PyErr_GetRaisedException();
    Py_DECREF(type);
    _PyValue_RestoreException(saved);
    if (PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(result);
        _PyValue_RestoreException(saved);
        return 0;
    }
    *out = result;
    return version;
}

/* Owning object lookup. Scalar boxing and owner cleanup may raise;
   callers must distinguish absence from NULL with a pending exception. */
PyObject *
_PyType_LookupRef(PyTypeObject *type, PyObject *name)
{
    return _PyType_LookupRefAndVersion(type, name, NULL);
}

int
_PyType_CacheInitForSpecialization(PyHeapTypeObject *type, PyObject *init,
                                   unsigned int tp_version)
{
    (void)_PyType_GetVersionForCurrentState(_PyInterpreterState_GET(),
                                          (PyTypeObject *)type);
    if (PyErr_Occurred() || !init || !tp_version) {
        return 0;
    }
    int can_cache;
    BEGIN_TYPE_LOCK();
    can_cache = _PyType_MatchesVersion(_PyInterpreterState_GET(),
                                        (PyTypeObject *)type, tp_version);
    #ifdef Py_GIL_DISABLED
    can_cache = can_cache && _PyObject_HasDeferredRefcount(init);
    #endif
    if (can_cache) {
        FT_ATOMIC_STORE_PTR_RELEASE(type->_spec_cache.init, init);
    }
    END_TYPE_LOCK();
    return can_cache;
}

int
_PyType_CacheGetItemForSpecialization(PyHeapTypeObject *ht, PyObject *descriptor, uint32_t tp_version)
{
    (void)_PyType_GetVersionForCurrentState(_PyInterpreterState_GET(),
                                          (PyTypeObject *)ht);
    if (PyErr_Occurred() || !descriptor || !tp_version) {
        return 0;
    }
    int can_cache;
    BEGIN_TYPE_LOCK();
    can_cache = _PyType_MatchesVersion(_PyInterpreterState_GET(),
                                        (PyTypeObject *)ht, tp_version);
    // This pointer is invalidated by PyType_Modified (see the comment on
    // struct _specialization_cache):
    PyFunctionObject *func = (PyFunctionObject *)descriptor;
    uint32_t version = _PyFunction_GetVersionForCurrentState(func);
    can_cache = can_cache && _PyFunction_IsVersionValid(version);
#ifdef Py_GIL_DISABLED
    can_cache = can_cache && _PyObject_HasDeferredRefcount(descriptor);
#endif
    if (can_cache) {
        FT_ATOMIC_STORE_PTR_RELEASE(ht->_spec_cache.getitem, descriptor);
        FT_ATOMIC_STORE_UINT32_RELAXED(ht->_spec_cache.getitem_version, version);
    }
    END_TYPE_LOCK();
    return can_cache;
}

void
_PyType_SetFlags(PyTypeObject *self, unsigned long mask, unsigned long flags)
{
    unsigned long new_flags = (self->tp_flags & ~mask) | flags;
    if (new_flags != self->tp_flags) {
        types_stop_world();
        // can't use new_flags here since they could be out-of-date
        self->tp_flags = (self->tp_flags & ~mask) | flags;
        types_start_world();
    }
}

int
_PyType_Validate(PyTypeObject *ty, _py_validate_type validate, unsigned int *tp_version)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    *tp_version = 0;
    if (!type_cache_admitted(interp, ty) || PyErr_Occurred()) {
        return -1;
    }
    Py_INCREF(ty);
    int err;
    BEGIN_TYPE_LOCK();
    err = validate(ty);
    if (err == 0 && !PyErr_Occurred() && type_cache_admitted(interp, ty) &&
        assign_version_tag(interp, ty)) {
        *tp_version = _PyType_GetVersionForCurrentState(interp, ty);
    }
    else {
        err = -1;
    }
    END_TYPE_LOCK();
    if (Py_REFCNT(ty) == 1) {
        *tp_version = 0;
        err = -1;
    }
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(ty);
    _PyValue_RestoreException(saved);
    if (PyErr_Occurred()) {
        *tp_version = 0;
        return -1;
    }
    return err;
}

static void
set_flags_recursive(PyTypeObject *self, unsigned long mask, unsigned long flags)
{
    if (PyType_HasFeature(self, Py_TPFLAGS_IMMUTABLETYPE) ||
        (self->tp_flags & mask) == flags)
    {
        return;
    }

    type_set_flags_with_mask(self, mask, flags);

    PyObject *children = _PyType_GetSubclasses(self);
    if (children == NULL) {
        return;
    }

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(children); i++) {
        if (PyErr_Occurred()) {
            break;
        }
        PyValue child_value = PyList_GetValue(children, i);
        if (PyValue_IsNull(child_value)) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_SystemError,
                                "subclass list contains a NULL value");
            }
            break;
        }
        if (!PyErr_Occurred()) {
            if (!PyValue_IsObject(child_value)) {
                PyErr_SetString(PyExc_SystemError,
                                "subclass list contains a non-object value");
            }
            else {
                PyObject *child = PyValue_AsPyObjectBorrow(child_value);
                if (!PyType_Check(child)) {
                    PyErr_SetString(PyExc_SystemError,
                                    "subclass list contains a non-type");
                }
                else {
                    set_flags_recursive((PyTypeObject *)child, mask, flags);
                }
            }
        }

        PyValue child_owner = child_value;
        child_value = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_CLOSE(child_owner);
        _PyValue_RestoreException(saved);
        if (PyErr_Occurred()) {
            break;
        }
    }
    type_mutation_close(&children);
}

void
_PyType_SetFlagsRecursive(PyTypeObject *self, unsigned long mask, unsigned long flags)
{
    BEGIN_TYPE_LOCK();
    /* Ideally, changing flags and invalidating the old version tag would
       happen in one step. But _PyType_Modified_Unlocked() is re-entrant and
       cannot run with the world stopped, so we must invalidate first.
       Immutable/static-builtin types are skipped because
       set_flags_recursive() does not modify them. */
    if (!PyType_HasFeature(self, Py_TPFLAGS_IMMUTABLETYPE) &&
        (self->tp_flags & mask) != flags)
    {
        _PyType_Modified_Unlocked(self);
    }
    /* Keep TYPE_LOCK held while waiting for stop-the-world so no thread
       can reassign a version tag before the flag update. */
    pinned_mutexes_t pinned;
    type_lock_prevent_release(&pinned);
    types_stop_world();
    set_flags_recursive(self, mask, flags);
    types_start_world();
    type_lock_allow_release(&pinned);
    END_TYPE_LOCK();
}

/* This is similar to PyObject_GenericGetAttr(),
   but uses _PyType_LookupRef() instead of just looking in type->tp_dict.

   The argument suppress_missing_attribute is used to provide a
   fast path for hasattr. The possible values are:

   * NULL: do not suppress the exception
   * Non-zero pointer: suppress the PyExc_AttributeError and
     set *suppress_missing_attribute to 1 to signal we are returning NULL while
     having suppressed the exception (other exceptions are not suppressed)

   */
PyObject *
_Py_type_getattro_impl(PyTypeObject *type, PyObject *name, int *suppress_missing_attribute)
{
    PyObject *type_owner = NULL, *name_owner = NULL, *result = NULL;
    PyValue ref = PyValue_NULL;
    if (PyErr_Occurred()) {
        return NULL;
    }
    type_owner = Py_NewRef((PyObject *)type);
    name_owner = Py_NewRef(name);
    ref = _Py_type_getattro_value((PyTypeObject *)type_owner,
                                 name_owner, suppress_missing_attribute);
    if (!PyValue_IsNull(ref) && !PyErr_Occurred()) {
        result = PyValue_AsObject(ref);
    }
    {
        PyValue retired = ref;
        ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&name_owner);
    type_mutation_close(&type_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

/* This is similar to PyObject_GenericGetAttr(),
   but uses _PyType_LookupRef() instead of just looking in type->tp_dict. */
PyObject *
_Py_type_getattro(PyObject *tp, PyObject *name)
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    return _Py_type_getattro_impl(type, name, NULL);
}

/* Like _Py_type_getattro but returns a PyValue.
   This can return a deferred reference in the free-threaded build
   when the attribute is found without going through a descriptor.

   suppress_missing_attribute (optional):
   * NULL: do not suppress the exception
   * Non-zero pointer: suppress the PyExc_AttributeError and
     set *suppress_missing_attribute to 1 to signal we are returning NULL while
     having suppressed the exception (other exceptions are not suppressed)
*/
PyValue
_Py_type_getattro_value(PyTypeObject *type, PyObject *name,
                           int *suppress_missing_attribute)
{
    PyObject *type_owner = NULL, *name_owner = NULL, *metatype_owner = NULL;
    PyObject *diagnostic_type = NULL, *diagnostic_name = NULL;
    PyValue result = PyValue_NULL;
    _PyCValue result_ref, meta_attribute_ref, attribute_ref;
    PyThreadState *tstate = _PyThreadState_GET();
    int active = 0, missing = 0;
    descrgetfunc meta_get = NULL;
    if (PyErr_Occurred()) {
        return PyValue_NULL;
    }
    type_owner = Py_NewRef((PyObject *)type);
    name_owner = Py_NewRef(name);
    metatype_owner = Py_NewRef((PyObject *)Py_TYPE(type));
    if (!PyUnicode_Check(name_owner)) {
        diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(name_owner));
        PyTypeObject *selected = (PyTypeObject *)diagnostic_type;
        if (selected->tp_flags & Py_TPFLAGS_HEAPTYPE) {
            diagnostic_name = Py_NewRef(((PyHeapTypeObject *)selected)->ht_name);
        }
        const char *selected_name = selected->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'", selected_name);
        goto done;
    }
    if (!_PyType_IsReady((PyTypeObject *)type_owner)) {
        if (PyType_Ready((PyTypeObject *)type_owner) < 0 || PyErr_Occurred()) {
            goto done;
        }
    }
    _PyThreadState_PushCValue(tstate, &result_ref);
    _PyThreadState_PushCValue(tstate, &meta_attribute_ref);
    _PyThreadState_PushCValue(tstate, &attribute_ref);
    active = 1;
    (void)_PyType_LookupValueAndVersion((PyTypeObject *)metatype_owner, name_owner,
                                        &meta_attribute_ref.ref);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (PyValue_IsObject(meta_attribute_ref.ref)) {
        PyObject *meta_obj = PyValue_AsPyObjectBorrow(meta_attribute_ref.ref);
        meta_get = Py_TYPE(meta_obj)->tp_descr_get;
        if (meta_get != NULL && PyDescr_IsData(meta_obj)) {
            PyObject *returned = meta_get(meta_obj, type_owner, metatype_owner);
            if (returned != NULL) {
                result_ref.ref = PyValue_FromPyObjectSteal(returned);
            }
            goto done;
        }
    }
    (void)_PyType_LookupValueAndVersion((PyTypeObject *)type_owner, name_owner,
                                        &attribute_ref.ref);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (!PyValue_IsNull(attribute_ref.ref)) {
        PyObject *attr_obj = PyValue_IsObject(attribute_ref.ref)
            ? PyValue_AsPyObjectBorrow(attribute_ref.ref) : NULL;
        descrgetfunc local_get = attr_obj == NULL ? NULL : Py_TYPE(attr_obj)->tp_descr_get;
        {
            PyValue retired = meta_attribute_ref.ref;
            meta_attribute_ref.ref = PyValue_NULL;
            PyObject *saved = PyErr_GetRaisedException();
            PyValue_XCLOSE(retired);
            _PyValue_RestoreException(saved);
        }
        if (PyErr_Occurred()) {
            goto done;
        }
        if (local_get != NULL) {
            if (Py_TYPE(attr_obj) == &PyStaticMethod_Type) {
                PyObject *callable = _PyStaticMethod_GetFunc(attr_obj);
                if (callable != NULL) {
                    result_ref.ref = PyValue_FromPyObjectNew(callable);
                    goto done;
                }
            }
            PyObject *returned = local_get(attr_obj, NULL, type_owner);
            if (returned != NULL) {
                result_ref.ref = PyValue_FromPyObjectSteal(returned);
            }
            goto done;
        }
        result_ref.ref = attribute_ref.ref;
        attribute_ref.ref = PyValue_NULL;
        goto done;
    }
    if (meta_get != NULL) {
        PyObject *meta_obj = PyValue_AsPyObjectBorrow(meta_attribute_ref.ref);
        PyObject *returned = meta_get(meta_obj, type_owner, metatype_owner);
        if (returned != NULL) {
            result_ref.ref = PyValue_FromPyObjectSteal(returned);
        }
        goto done;
    }
    if (!PyValue_IsNull(meta_attribute_ref.ref)) {
        result_ref.ref = meta_attribute_ref.ref;
        meta_attribute_ref.ref = PyValue_NULL;
        goto done;
    }
    if (suppress_missing_attribute == NULL) {
        diagnostic_type = Py_NewRef(type_owner);
        PyTypeObject *selected = (PyTypeObject *)diagnostic_type;
        if (selected->tp_flags & Py_TPFLAGS_HEAPTYPE) {
            diagnostic_name = Py_NewRef(((PyHeapTypeObject *)selected)->ht_name);
        }
        const char *selected_name = selected->tp_name;
        PyErr_Format(PyExc_AttributeError,
                     "type object '%.100s' has no attribute '%U'",
                     selected_name, name_owner);
    }
    else {
        missing = 1;
    }
done:
    if (active) {
        {
            PyValue retired = _PyThreadState_PopCValueSteal(tstate, &attribute_ref);
            attribute_ref.ref = PyValue_NULL;
            PyObject *saved = PyErr_GetRaisedException();
            PyValue_XCLOSE(retired);
            _PyValue_RestoreException(saved);
        }
        {
            PyValue retired = _PyThreadState_PopCValueSteal(tstate, &meta_attribute_ref);
            meta_attribute_ref.ref = PyValue_NULL;
            PyObject *saved = PyErr_GetRaisedException();
            PyValue_XCLOSE(retired);
            _PyValue_RestoreException(saved);
        }
    }
    type_mutation_close(&diagnostic_name);
    type_mutation_close(&diagnostic_type);
    type_mutation_close(&metatype_owner);
    type_mutation_close(&name_owner);
    type_mutation_close(&type_owner);
    if (active) {
        result = _PyThreadState_PopCValueSteal(tstate, &result_ref);
        result_ref.ref = PyValue_NULL;
    }
    if (PyErr_Occurred()) {
        PyValue retired = result;
        result = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
        return PyValue_NULL;
    }
    if (missing) {
        *suppress_missing_attribute = 1;
    }
    return result;
}

// Called by type_setattro().  Updates both the type dict and
// the type versions.
static int
type_update_dict(PyTypeObject *type, PyDictObject *dict, PyObject *name,
                 PyObject *value, PyObject **old_value)
{
    assert(_PyInterpreterState_GET()->types.mutation_depth != 0);
    if (PyErr_Occurred()) {
        return -1;
    }
    int found = _PyDict_GetItemRef_Unicode_LockHeld(dict, name, old_value);
    if (found < 0 || PyErr_Occurred()) {
        return -1;
    }
    // The caller invalidated before selecting this owned dictionary. An old
    // value's object bridge may call out; never write through a replaced view.
    if (lookup_tp_dict(type) != (PyObject *)dict) {
        PyErr_SetString(PyExc_RuntimeError, "type dictionary changed during mutation");
        return -1;
    }
    if (value == NULL && found == 0) {
        PyErr_Format(PyExc_AttributeError,
                     "type object '%.50s' has no attribute '%U'", type->tp_name, name);
        _PyObject_SetAttributeErrorContext((PyObject *)type, name);
        return -1;
    }
    int result = _PyDict_SetItem_LockHeld(dict, name, value);
    if (result < 0 || PyErr_Occurred()) {
        // Preserve provider/retirement errors, including a cleanup KeyError.
        return -1;
    }
    if (lookup_tp_dict(type) != (PyObject *)dict) {
        PyErr_SetString(PyExc_RuntimeError, "type dictionary changed during mutation");
        return -1;
    }
    return 0;
}

static int
update_slot_after_setattr(PyTypeObject *type, PyObject *name)
{
#ifdef Py_GIL_DISABLED
    assert(SLOT_UPDATE_CHUNK_SIZE >= MAX_EQUIV);
    slot_update_chunk_t chunk = {0};
    // Stack allocate the first chunk.  It is usually the only one needed but
    // updates are queued for subclasses as well, so more chunks are needed if
    // the type has more than SLOT_UPDATE_CHUNK_SIZE subclasses.
    slot_update_t queued_updates = {&chunk};

    int res = update_slot(type, name, &queued_updates);
    if (res == 0 && queued_updates.head->n > 0) {
        apply_type_slot_updates(&queued_updates);
        ASSERT_TYPE_LOCK_HELD();
    }
    slot_update_chunk_t *cur = queued_updates.head;
    while (cur != &chunk) {
        slot_update_chunk_t *prev = cur->prev;
        PyMem_Free(cur);
        cur = prev;
    }
    return res;
#else
    update_slot(type, name, NULL);
    return 0;
#endif
}

static int
type_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    if (PyErr_Occurred()) {
        return -1;
    }
    PyTypeObject *type = PyTypeObject_CAST(self);
    Py_INCREF(type);
    Py_XINCREF(value);
    PyObject *original_name = Py_NewRef(name);
    name = NULL;
    PyObject *dict = NULL;
    PyObject *candidate = NULL;
    PyObject *descr = NULL;
    PyObject *old_value = NULL;
    _PyTypeMutation scope = {0};
    int res = -1;
    if (type->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) {
        PyErr_Format(PyExc_TypeError, "cannot set %R attribute of immutable type '%s'",
                     original_name, type->tp_name);
        goto done;
    }
    if (!PyUnicode_Check(original_name)) {
        PyErr_Format(PyExc_TypeError, "attribute name must be string, not '%.200s'",
                     Py_TYPE(original_name)->tp_name);
        goto done;
    }
    name = PyUnicode_CheckExact(original_name)
        ? Py_NewRef(original_name) : _PyUnicode_Copy(original_name);
    if (name == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (!PyUnicode_CHECK_INTERNED(name)) {
        _PyUnicode_InternMortal(_PyInterpreterState_GET(), &name);
        if (PyErr_Occurred()) {
            goto done;
        }
        if (!PyUnicode_CHECK_INTERNED(name)) {
            PyErr_SetString(PyExc_MemoryError, "Out of memory interning an attribute name");
            goto done;
        }
    }
    PyTypeObject *metatype = Py_TYPE(type);
    assert(!_PyType_HasFeature(metatype, Py_TPFLAGS_INLINE_VALUES));
    assert(!_PyType_HasFeature(metatype, Py_TPFLAGS_MANAGED_DICT));

#ifdef Py_GIL_DISABLED
    // gh-139103: Enable deferred refcounting for functions and descriptors
    // assigned to type objects.  This is important for `dataclass.__init__`,
    // which is generated dynamically, and for descriptor scaling on
    // free-threaded builds.
    if (value != NULL && (PyFunction_Check(value) || Py_TYPE(value)->tp_descr_get != NULL))
    {
        PyUnstable_Object_EnableDeferredRefcount(value);
    }
#endif

    descr = _PyType_LookupRef(metatype, name);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (descr != NULL) {
        descrsetfunc f = Py_TYPE(descr)->tp_descr_set;
        if (f != NULL) {
            // A direct descriptor's actual setter owns its mutation scope.
            res = f(descr, self, value);
            goto done;
        }
    }
    if (_PyTypeMutation_Begin(&scope, type) < 0) {
        goto done;
    }
    PyType_Modified(type);
    if (PyErr_Occurred()) {
        goto done;
    }
    if (lookup_tp_dict(type) == NULL) {
        candidate = PyDict_New();
        if (candidate == NULL || PyErr_Occurred()) {
            goto done;
        }
        BEGIN_TYPE_LOCK();
        if (lookup_tp_dict(type) == NULL) {
            set_tp_dict(type, candidate);
            candidate = NULL;
        }
        END_TYPE_LOCK();
        type_mutation_close(&candidate);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    dict = type_mutation_get_dict(type);
    if (dict == NULL || PyErr_Occurred()) {
        goto done;
    }
    BEGIN_TYPE_DICT_LOCK(dict);
    res = type_update_dict(type, (PyDictObject *)dict, name, value, &old_value);
    if (!PyErr_Occurred()) {
        assert(_PyType_CheckConsistency(type));
    }
    if (res == 0 && !PyErr_Occurred() && is_dunder_name(name) && has_slotdef(name)) {
        res = update_slot_after_setattr(type, name);
    }
    END_TYPE_DICT_LOCK();

done:
    type_mutation_close(&candidate);
    type_mutation_close(&dict);
    type_mutation_close(&descr);
    // Keep admission closed through delayed retirement of the displaced value.
    PyObject *saved = PyErr_GetRaisedException();
    _PyObject_XDecRefDelayed(old_value);
    _PyValue_RestoreException(saved);
    type_mutation_close(&name);
    type_mutation_close(&original_name);
    type_mutation_close(&value);
    saved = PyErr_GetRaisedException();
    Py_DECREF(type);
    _PyValue_RestoreException(saved);
    return _PyTypeMutation_End(&scope) < 0 ? -1 : res;
}


static void
type_dealloc_common(PyTypeObject *type)
{
    PyObject *bases = lookup_tp_bases(type);
    if (bases != NULL) {
        PyObject *exc = PyErr_GetRaisedException();
        BEGIN_TYPE_LOCK();
        remove_all_subclasses(type, bases);
        END_TYPE_LOCK();
        _PyValue_RestoreException(exc);
    }
}


static void
clear_static_tp_subclasses(PyTypeObject *type, int isbuiltin)
{
    PyObject *subclasses = Py_XNewRef(lookup_tp_subclasses(type));
    if (subclasses == NULL) {
        return;
    }

    /* Normally it would be a problem to finalize the type if its
       tp_subclasses wasn't cleared first.  However, this is only
       ever called at the end of runtime finalization, so we can be
       more liberal in cleaning up.  If the given type still has
       subtypes at this point then some extension module did not
       correctly finalize its objects.

       We can safely obliterate such subtypes since the extension
       module and its objects won't be used again, except maybe if
       the runtime were re-initialized.  In that case the sticky
       situation would only happen if the module were re-imported
       then and only if the subtype were stored in a global and only
       if that global were not overwritten during import.  We'd be
       fine since the extension is otherwise unsafe and unsupported
       in that situation, and likely problematic already.

       In any case, this situation means at least some memory is
       going to leak.  This mostly only affects embedding scenarios.
     */

#ifndef NDEBUG
    // For now we just do a sanity check and then clear tp_subclasses.
    PyObject *primary = PyErr_GetRaisedException();
    Py_ssize_t pos = 0;
    int found;
    while (!PyErr_Occurred()) {
        PyValue ref = PyValue_NULL;
        found = _PyDict_NextValue(subclasses, &pos, NULL, &ref, NULL);
        if (found <= 0) {
            break;
        }
        assert(PyValue_IsObject(ref));
        PyObject *weakref = PyValue_AsPyObjectBorrow(ref);
        assert(PyWeakref_Check(weakref));
        PyTypeObject *subclass = type_from_ref(weakref);
        PyValue_CLOSE(ref);
        if (PyErr_Occurred()) {
            PyObject *error = PyErr_GetRaisedException();
            if (subclass != NULL) {
                PyErr_SetRaisedException(error);
                PyObject *owner = (PyObject *)subclass;
                type_mutation_close(&owner);
                error = NULL;
            }
            if (primary == NULL) {
                primary = error;
            }
            else if (error != NULL) {
                PyErr_SetRaisedException(primary);
                primary = NULL;
                type_mutation_close(&error);
                primary = PyErr_GetRaisedException();
            }
            break;
        }
        if (subclass != NULL) {
            // All static builtin subtypes should have been finalized already.
            assert(!isbuiltin || !(subclass->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));
            PyObject *owner = (PyObject *)subclass;
            type_mutation_close(&owner);
        }
    }
    PyObject *diagnostic_error = PyErr_GetRaisedException();
    if (primary == NULL) {
        primary = diagnostic_error;
    }
    else if (diagnostic_error != NULL) {
        PyErr_SetRaisedException(primary);
        primary = NULL;
        type_mutation_close(&diagnostic_error);
        primary = PyErr_GetRaisedException();
    }
#else
    (void)isbuiltin;
    PyObject *primary = PyErr_GetRaisedException();
#endif

    clear_tp_subclasses(type);
    PyObject *clear_error = PyErr_GetRaisedException();
    if (primary == NULL) {
        primary = clear_error;
    }
    else if (clear_error != NULL) {
        PyErr_SetRaisedException(primary);
        primary = NULL;
        type_mutation_close(&clear_error);
        primary = PyErr_GetRaisedException();
    }
    PyErr_SetRaisedException(primary);
    type_mutation_close(&subclasses);
}

static void
clear_static_type_objects(PyInterpreterState *interp, PyTypeObject *type,
                          int isbuiltin, int final)
{
    if (final) {
        Py_CLEAR(type->tp_cache);
    }
    _PyTypeCache_Invalidate(type);
    clear_tp_dict(type);
    clear_tp_bases(type, final);
    clear_tp_mro(type, final);
    clear_static_tp_subclasses(type, isbuiltin);
}


static void
fini_static_type(PyInterpreterState *interp, PyTypeObject *type,
                 int isbuiltin, int final)
{
    assert(type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN);
    assert(_Py_IsImmortal((PyObject *)type));

    type_dealloc_common(type);

    clear_static_type_objects(interp, type, isbuiltin, final);

    if (final) {
        BEGIN_TYPE_LOCK();
        type_clear_flags(type, Py_TPFLAGS_READY);
        set_version_unlocked(type, 0);
        END_TYPE_LOCK();
    }

    _PyStaticType_ClearWeakRefs(interp, type);
    managed_static_type_state_clear(interp, type, isbuiltin, final);
    /* We leave _Py_TPFLAGS_STATIC_BUILTIN set on tp_flags. */
}

void
_PyTypes_FiniExtTypes(PyInterpreterState *interp)
{
    for (size_t i = _Py_MAX_MANAGED_STATIC_EXT_TYPES; i > 0; i--) {
        if (interp->types.for_extensions.num_initialized == 0) {
            break;
        }
        int64_t count = 0;
        PyTypeObject *type = static_ext_type_lookup(interp, i-1, &count);
        if (type == NULL) {
            continue;
        }
        int final = (count == 1);
        fini_static_type(interp, type, 0, final);
    }
}

void
_PyStaticType_FiniBuiltin(PyInterpreterState *interp, PyTypeObject *type)
{
    fini_static_type(interp, type, 1, _Py_IsMainInterpreter(interp));
}


void
_PyTypes_FiniCachedDescriptors(PyInterpreterState *interp)
{
    Py_CLEAR(interp->cached_objects.dict_descriptor);
    Py_CLEAR(interp->cached_objects.weakref_descriptor);
}


static void
notify_type_destroy(PyTypeObject *type)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    int bits = type->tp_watched;
    int i = 0;
    while (bits) {
        assert(i < TYPE_MAX_WATCHERS);
        if (bits & 1) {
            PyType_WatchCallback cb = interp->type_watchers[i];
            if (cb && (cb(type) < 0)) {
                PyErr_FormatUnraisable(
                    "Exception ignored in type watcher callback #%d "
                    "for %R",
                    i, type);
            }
        }
        i++;
        bits >>= 1;
    }
}


static void
type_dealloc(PyObject *self)
{
    PyTypeObject *type = PyTypeObject_CAST(self);

    // Assert this is a heap-allocated type object
    _PyObject_ASSERT((PyObject *)type, type->tp_flags & Py_TPFLAGS_HEAPTYPE);

    // Notify type watchers before teardown.  The type object is still fully
    // intact at this point (dict, bases, mro, name are all valid), so
    // callbacks can safely inspect it.
    if (type->tp_watched) {
        _PyObject_ResurrectStart(self);
        notify_type_destroy(type);
        if (_PyObject_ResurrectEnd(self)) {
            return;     // callback resurrected the object
        }
    }

    _PyObject_GC_UNTRACK(type);
    type_dealloc_common(type);

    // PyObject_ClearWeakRefs() raises an exception if Py_REFCNT() != 0
    assert(Py_REFCNT(type) == 0);
    PyObject_ClearWeakRefs((PyObject *)type);

    Py_XDECREF(type->tp_base);
    Py_XDECREF(type->tp_dict);
    Py_XDECREF(type->tp_bases);
    Py_XDECREF(type->tp_mro);
    Py_XDECREF(type->tp_cache);
    clear_tp_subclasses(type);
    _PyTypeCache_Invalidate(type);

    /* A type's tp_doc is heap allocated, unlike the tp_doc slots
     * of most other objects.  It's okay to cast it to char *.
     */
    PyMem_Free((char *)type->tp_doc);

    PyHeapTypeObject *et = (PyHeapTypeObject *)type;
    Py_XDECREF(et->ht_name);
    Py_XDECREF(et->ht_qualname);
    Py_XDECREF(et->ht_slots);
    if (et->ht_cached_keys) {
        _PyDict_RemoveKeysForClass(et);
    }
    Py_XDECREF(et->ht_module);
    PyMem_Free(et->_ht_tpname);
#ifdef Py_GIL_DISABLED
    assert(et->unique_id == _Py_INVALID_UNIQUE_ID);
#endif
    et->ht_token = NULL;
    Py_TYPE(type)->tp_free((PyObject *)type);
}


/*[clinic input]
type.__subclasses__

Return a list of immediate subclasses.
[clinic start generated code]*/

static PyObject *
type___subclasses___impl(PyTypeObject *self)
/*[clinic end generated code: output=eb5eb54485942819 input=5af66132436f9a7b]*/
{
    return _PyType_GetSubclasses(self);
}

static PyObject *
type_prepare(PyObject *self, PyObject *const *args, Py_ssize_t nargs,
             PyObject *kwnames)
{
    return PyDict_New();
}


/*
   Merge the __dict__ of aclass into dict, and recursively also all
   the __dict__s of aclass's base classes.  The order of merging isn't
   defined, as it's expected that only the final set of dict keys is
   interesting.
   Return 0 on success, -1 on error.
*/

static int
merge_class_dict(PyObject *dict, PyObject *aclass)
{
    PyObject *classdict;
    PyObject *bases;

    assert(PyDict_Check(dict));
    assert(aclass);

    /* Merge in the type's dict (if any). */
    if (PyObject_GetOptionalAttr(aclass, &_Py_ID(__dict__), &classdict) < 0) {
        return -1;
    }
    if (classdict != NULL) {
        int status = PyDict_Update(dict, classdict);
        Py_DECREF(classdict);
        if (status < 0)
            return -1;
    }

    /* Recursively merge in the base types' (if any) dicts. */
    if (PyObject_GetOptionalAttr(aclass, &_Py_ID(__bases__), &bases) < 0) {
        return -1;
    }
    if (bases != NULL) {
        /* We have no guarantee that bases is a real tuple */
        Py_ssize_t i, n;
        n = PySequence_Size(bases); /* This better be right */
        if (n < 0) {
            Py_DECREF(bases);
            return -1;
        }
        else {
            for (i = 0; i < n; i++) {
                int status;
                PyObject *base = PySequence_GetItem(bases, i);
                if (base == NULL) {
                    Py_DECREF(bases);
                    return -1;
                }
                status = merge_class_dict(dict, base);
                Py_DECREF(base);
                if (status < 0) {
                    Py_DECREF(bases);
                    return -1;
                }
            }
        }
        Py_DECREF(bases);
    }
    return 0;
}

/* __dir__ for type objects: returns __dict__ and __bases__.
   We deliberately don't suck up its __class__, as methods belonging to the
   metaclass would probably be more confusing than helpful.
*/
/*[clinic input]
type.__dir__

Specialized __dir__ implementation for types.
[clinic start generated code]*/

static PyObject *
type___dir___impl(PyTypeObject *self)
/*[clinic end generated code: output=69d02fe92c0f15fa input=7733befbec645968]*/
{
    PyObject *result = NULL;
    PyObject *dict = PyDict_New();

    if (dict != NULL && merge_class_dict(dict, (PyObject *)self) == 0)
        result = PyDict_Keys(dict);

    Py_XDECREF(dict);
    return result;
}

/*[clinic input]
type.__sizeof__

Return memory consumption of the type object.
[clinic start generated code]*/

static PyObject *
type___sizeof___impl(PyTypeObject *self)
/*[clinic end generated code: output=766f4f16cd3b1854 input=99398f24b9cf45d6]*/
{
    size_t size;
    if (self->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        PyHeapTypeObject* et = (PyHeapTypeObject*)self;
        size = sizeof(PyHeapTypeObject);
        if (et->ht_cached_keys)
            size += _PyDict_KeysSize(et->ht_cached_keys);
    }
    else {
        size = sizeof(PyTypeObject);
    }
    return PyLong_FromSize_t(size);
}

static PyMethodDef type_methods[] = {
    TYPE_MRO_METHODDEF
    TYPE___SUBCLASSES___METHODDEF
    {"__prepare__", _PyCFunction_CAST(type_prepare),
     METH_FASTCALL | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("__prepare__($cls, name, bases, /, **kwds)\n"
               "--\n"
               "\n"
               "Create the namespace for the class statement")},
    TYPE___INSTANCECHECK___METHODDEF
    TYPE___SUBCLASSCHECK___METHODDEF
    TYPE___DIR___METHODDEF
    TYPE___SIZEOF___METHODDEF
    {0}
};

PyDoc_STRVAR(type_doc,
"type(object) -> the object's type\n"
"type(name, bases, dict, **kwds) -> a new type");

static int
type_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyTypeObject *type = PyTypeObject_CAST(self);

    /* Because of type_is_gc(), the collector only calls this
       for heaptypes. */
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        char msg[200];
        sprintf(msg, "type_traverse() called on non-heap type '%.100s'",
                type->tp_name);
        _PyObject_ASSERT_FAILED_MSG((PyObject *)type, msg);
    }

    Py_VISIT(type->tp_dict);
    Py_VISIT(type->tp_cache);
    Py_VISIT(type->tp_mro);
    Py_VISIT(type->tp_bases);
    Py_VISIT(type->tp_base);
    PyHeapTypeObject *ht = (PyHeapTypeObject *)type;
    Py_VISIT(ht->ht_module);

    /* The rest cannot be involved in cycles, but the tracing collector
       must see every reference the type owns: tp_subclasses is a dict of
       weak references, ht_slots is a tuple of strings, ht_*name are
       strings, and the shared keys hold strings. */
    Py_VISIT(type->tp_subclasses);
    _Py_VISIT_ATOMIC(ht->ht_name);
    _Py_VISIT_ATOMIC(ht->ht_qualname);
    Py_VISIT(ht->ht_slots);
    return _PyDict_TraverseKeysForClass(ht, visit, arg);
}

/* Visit what a static type refers to, for the tracing verifier.  A static
   builtin type keeps its dict and subclasses per interpreter. */
int
_PyType_TraverseStatic(PyTypeObject *type, visitproc visit, void *arg)
{
    assert(!(type->tp_flags & Py_TPFLAGS_HEAPTYPE));
    if (!(type->tp_flags & Py_TPFLAGS_READY)) {
        return 0;
    }
    Py_VISIT(lookup_tp_dict(type));
    Py_VISIT(lookup_tp_subclasses(type));
    Py_VISIT(type->tp_cache);
    Py_VISIT(lookup_tp_mro(type));
    Py_VISIT(lookup_tp_bases(type));
    Py_VISIT(type->tp_base);
    return 0;
}

static int
type_clear(PyObject *self)
{
    PyTypeObject *type = PyTypeObject_CAST(self);

    /* Because of type_is_gc(), the collector only calls this
       for heaptypes. */
    _PyObject_ASSERT((PyObject *)type, type->tp_flags & Py_TPFLAGS_HEAPTYPE);

    /* We need to invalidate the method cache carefully before clearing
       the dict, so that other objects caught in a reference cycle
       don't start calling destroyed methods.

       Otherwise, we need to clear tp_mro, which is
       part of a hard cycle (its first element is the class itself) that
       won't be broken otherwise (it's a tuple and tuples don't have a
       tp_clear handler).
       We also need to clear ht_module, if present: the module usually holds a
       reference to its class. None of the other fields need to be

       cleared, and here's why:

       tp_cache:
           Not used; if it were, it would be a dict.

       tp_bases, tp_base:
           If these are involved in a cycle, there must be at least
           one other, mutable object in the cycle, e.g. a base
           class's dict; the cycle will be broken that way.

       tp_subclasses:
           A dict of weak references can't be part of a cycle; and
           dicts have their own tp_clear.

       slots (in PyHeapTypeObject):
           A tuple of strings can't be part of a cycle.
    */

    PyType_Modified(type);
    PyObject *dict = lookup_tp_dict(type);
    if (dict) {
        PyDict_Clear(dict);
    }
    Py_CLEAR(((PyHeapTypeObject *)type)->ht_module);

    if (type->tp_mro != NULL) {
        mro_epoch_advance(type);
    }
    Py_CLEAR(type->tp_mro);

    return 0;
}

static int
type_is_gc(PyObject *tp)
{
    PyTypeObject *type = PyTypeObject_CAST(tp);
    return type->tp_flags & Py_TPFLAGS_HEAPTYPE;
}


static PyNumberMethods type_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
};

PyTypeObject PyType_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "type",                                     /* tp_name */
    sizeof(PyHeapTypeObject),                   /* tp_basicsize */
    sizeof(PyMemberDef),                        /* tp_itemsize */
    type_dealloc,                               /* tp_dealloc */
    offsetof(PyTypeObject, tp_vectorcall),      /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    type_repr,                                  /* tp_repr */
    &type_as_number,                            /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    type_call,                                  /* tp_call */
    0,                                          /* tp_str */
    _Py_type_getattro,                          /* tp_getattro */
    type_setattro,                              /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TYPE_SUBCLASS |
    Py_TPFLAGS_HAVE_VECTORCALL |
    Py_TPFLAGS_ITEMS_AT_END,                    /* tp_flags */
    type_doc,                                   /* tp_doc */
    type_traverse,                              /* tp_traverse */
    type_clear,                                 /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyTypeObject, tp_weaklist),        /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    type_methods,                               /* tp_methods */
    type_members,                               /* tp_members */
    type_getsets,                               /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(PyTypeObject, tp_dict),            /* tp_dictoffset */
    type_init,                                  /* tp_init */
    0,                                          /* tp_alloc */
    type_new,                                   /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    type_is_gc,                                 /* tp_is_gc */
    .tp_vectorcall = type_vectorcall,
};


/* The base type of all types (eventually)... except itself. */

/* You may wonder why object.__new__() only complains about arguments
   when object.__init__() is not overridden, and vice versa.

   Consider the use cases:

   1. When neither is overridden, we want to hear complaints about
      excess (i.e., any) arguments, since their presence could
      indicate there's a bug.

   2. When defining an Immutable type, we are likely to override only
      __new__(), since __init__() is called too late to initialize an
      Immutable object.  Since __new__() defines the signature for the
      type, it would be a pain to have to override __init__() just to
      stop it from complaining about excess arguments.

   3. When defining a Mutable type, we are likely to override only
      __init__().  So here the converse reasoning applies: we don't
      want to have to override __new__() just to stop it from
      complaining.

   4. When __init__() is overridden, and the subclass __init__() calls
      object.__init__(), the latter should complain about excess
      arguments; ditto for __new__().

   Use cases 2 and 3 make it unattractive to unconditionally check for
   excess arguments.  The best solution that addresses all four use
   cases is as follows: __init__() complains about excess arguments
   unless __new__() is overridden and __init__() is not overridden
   (IOW, if __init__() is overridden or __new__() is not overridden);
   symmetrically, __new__() complains about excess arguments unless
   __init__() is overridden and __new__() is not overridden
   (IOW, if __new__() is overridden or __init__() is not overridden).
*/

/* Forward */
static PyObject *
object_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static int
excess_args(PyObject *args, PyObject *kwds)
{
    return PyTuple_GET_SIZE(args) ||
        (kwds && PyDict_Check(kwds) && PyDict_GET_SIZE(kwds));
}

static int
object_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyTypeObject *type = Py_TYPE(self);
    if (excess_args(args, kwds)) {
        if (type->tp_init != object_init) {
            PyErr_SetString(PyExc_TypeError,
                            "object.__init__() takes exactly one argument (the instance to initialize)");
            return -1;
        }
        if (type->tp_new == object_new) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s.__init__() takes exactly one argument (the instance to initialize)",
                         type->tp_name);
            return -1;
        }
    }
    return 0;
}

static PyObject *
object_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (excess_args(args, kwds)) {
        if (type->tp_new != object_new) {
            PyErr_SetString(PyExc_TypeError,
                            "object.__new__() takes exactly one argument (the type to instantiate)");
            return NULL;
        }
        if (type->tp_init == object_init) {
            PyErr_Format(PyExc_TypeError, "%.200s() takes no arguments",
                         type->tp_name);
            return NULL;
        }
    }

    if (type->tp_flags & Py_TPFLAGS_IS_ABSTRACT) {
        PyObject *abstract_methods;
        PyObject *sorted_methods;
        PyObject *joined;
        PyObject* comma_w_quotes_sep;
        Py_ssize_t method_count;

        /* Compute "', '".join(sorted(type.__abstractmethods__))
           into joined. */
        abstract_methods = type_abstractmethods((PyObject *)type, NULL);
        if (abstract_methods == NULL)
            return NULL;
        sorted_methods = PySequence_List(abstract_methods);
        Py_DECREF(abstract_methods);
        if (sorted_methods == NULL)
            return NULL;
        if (PyList_Sort(sorted_methods)) {
            Py_DECREF(sorted_methods);
            return NULL;
        }
        comma_w_quotes_sep = PyUnicode_FromString("', '");
        if (!comma_w_quotes_sep) {
            Py_DECREF(sorted_methods);
            return NULL;
        }
        joined = PyUnicode_Join(comma_w_quotes_sep, sorted_methods);
        Py_DECREF(comma_w_quotes_sep);
        if (joined == NULL)  {
            Py_DECREF(sorted_methods);
            return NULL;
        }
        method_count = PyObject_Length(sorted_methods);
        Py_DECREF(sorted_methods);
        if (method_count == -1) {
            Py_DECREF(joined);
            return NULL;
        }

        PyErr_Format(PyExc_TypeError,
                     "Can't instantiate abstract class %s "
                     "without an implementation for abstract method%s '%U'",
                     type->tp_name,
                     method_count > 1 ? "s" : "",
                     joined);
        Py_DECREF(joined);
        return NULL;
    }
    PyObject *obj = type->tp_alloc(type, 0);
    if (obj == NULL) {
        return NULL;
    }
    return obj;
}

static void
object_dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
object_repr(PyObject *self)
{
    PyTypeObject *type;
    PyObject *mod, *name, *rtn;

    type = Py_TYPE(self);
    mod = type_module(type);
    if (mod == NULL)
        PyErr_Clear();
    else if (!PyUnicode_Check(mod)) {
        Py_SETREF(mod, NULL);
    }
    name = type_qualname((PyObject *)type, NULL);
    if (name == NULL) {
        Py_XDECREF(mod);
        return NULL;
    }
    if (mod != NULL && !_PyUnicode_Equal(mod, &_Py_ID(builtins)))
        rtn = PyUnicode_FromFormat("<%U.%U object at %p>", mod, name, self);
    else
        rtn = PyUnicode_FromFormat("<%s object at %p>",
                                  type->tp_name, self);
    Py_XDECREF(mod);
    Py_DECREF(name);
    return rtn;
}

static PyObject *
object_str(PyObject *self)
{
    unaryfunc f;

    f = Py_TYPE(self)->tp_repr;
    if (f == NULL)
        f = object_repr;
    return f(self);
}

static PyObject *
object_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *res;

    switch (op) {

    case Py_EQ:
        /* Return NotImplemented instead of False, so if two
           objects are compared, both get a chance at the
           comparison.  See issue #1393. */
        res = Py_NewRef((self == other) ? Py_True : Py_NotImplemented);
        break;

    case Py_NE:
        /* By default, __ne__() delegates to __eq__() and inverts the result,
           unless the latter returns NotImplemented. */
        if (Py_TYPE(self)->tp_richcompare == NULL) {
            res = Py_NewRef(Py_NotImplemented);
            break;
        }
        res = (*Py_TYPE(self)->tp_richcompare)(self, other, Py_EQ);
        if (res != NULL && res != Py_NotImplemented) {
            int ok = PyObject_IsTrue(res);
            Py_DECREF(res);
            if (ok < 0)
                res = NULL;
            else {
                if (ok)
                    res = Py_NewRef(Py_False);
                else
                    res = Py_NewRef(Py_True);
            }
        }
        break;

    default:
        res = Py_NewRef(Py_NotImplemented);
        break;
    }

    return res;
}

PyObject*
_Py_BaseObject_RichCompare(PyObject* self, PyObject* other, int op)
{
    return object_richcompare(self, other, op);
}

static PyObject *
object_get_class(PyObject *self, void *closure)
{
    PyTypeObject *type = Py_TYPE(self);
    _Py_INCREF_TYPE(type);
    return (PyObject *)type;
}

static int
compatible_with_tp_base(PyTypeObject *child)
{
    PyTypeObject *parent = child->tp_base;
    return (parent != NULL &&
            child->tp_basicsize == parent->tp_basicsize &&
            child->tp_itemsize == parent->tp_itemsize &&
            (child->tp_dealloc == subtype_dealloc ||
             child->tp_dealloc == parent->tp_dealloc));
}

static int
same_slots_added(PyTypeObject *a, PyTypeObject *b)
{
    PyTypeObject *base = a->tp_base;
    Py_ssize_t size;
    PyObject *slots_a, *slots_b;

    assert(base == b->tp_base);
    size = base->tp_basicsize;
    if (a->tp_dictoffset == size && b->tp_dictoffset == size)
        size += sizeof(PyObject *);
    if (a->tp_weaklistoffset == size && b->tp_weaklistoffset == size)
        size += sizeof(PyObject *);

    /* Check slots compliance */
    if (!(a->tp_flags & Py_TPFLAGS_HEAPTYPE) ||
        !(b->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        return 0;
    }
    slots_a = ((PyHeapTypeObject *)a)->ht_slots;
    slots_b = ((PyHeapTypeObject *)b)->ht_slots;
    if (slots_a && slots_b) {
        if (PyObject_RichCompareBool(slots_a, slots_b, Py_EQ) != 1)
            return 0;
        size += sizeof(PyObject *) * PyTuple_GET_SIZE(slots_a);
    }
    return size == a->tp_basicsize && size == b->tp_basicsize;
}

static int
compatible_flags(int setclass, PyTypeObject *origto, PyTypeObject *newto, unsigned long flags)
{
    /* For __class__ assignment, the flags should be the same.
       For __bases__ assignment, the new base flags can only be set
       if the original class flags are set.
     */
    return setclass ? (origto->tp_flags & flags) == (newto->tp_flags & flags)
                    : !(~(origto->tp_flags & flags) & (newto->tp_flags & flags));
}

static int
compatible_for_assignment(PyTypeObject *origto, PyTypeObject *newto,
                          const char *attr, int setclass)
{
    PyTypeObject *newbase, *oldbase;
    PyTypeObject *oldto = setclass ? origto : origto->tp_base;

    if (setclass && newto->tp_free != oldto->tp_free) {
        PyErr_Format(PyExc_TypeError,
                     "%s assignment: "
                     "'%s' deallocator differs from '%s'",
                     attr,
                     newto->tp_name,
                     oldto->tp_name);
        return 0;
    }
    if (!compatible_flags(setclass, origto, newto,
                          Py_TPFLAGS_HAVE_GC |
                          Py_TPFLAGS_INLINE_VALUES |
                          Py_TPFLAGS_PREHEADER))
    {
        goto differs;
    }
    /* For __class__ assignment, tp_dictoffset and tp_weaklistoffset should
       be the same for old and new types.
       For __bases__ assignment, they can only be set in the new base
       if they are set in the original class with the same value.
     */
    if ((setclass || newto->tp_dictoffset)
        && origto->tp_dictoffset != newto->tp_dictoffset)
    {
        goto differs;
    }
    if ((setclass || newto->tp_weaklistoffset)
        && origto->tp_weaklistoffset != newto->tp_weaklistoffset)
    {
        goto differs;
    }
    /*
     It's tricky to tell if two arbitrary types are sufficiently compatible as
     to be interchangeable; e.g., even if they have the same tp_basicsize, they
     might have totally different struct fields. It's much easier to tell if a
     type and its supertype are compatible; e.g., if they have the same
     tp_basicsize, then that means they have identical fields. So to check
     whether two arbitrary types are compatible, we first find the highest
     supertype that each is compatible with, and then if those supertypes are
     compatible then the original types must also be compatible.
    */
    newbase = newto;
    oldbase = oldto;
    while (compatible_with_tp_base(newbase))
        newbase = newbase->tp_base;
    while (compatible_with_tp_base(oldbase))
        oldbase = oldbase->tp_base;
    if (newbase != oldbase &&
        (newbase->tp_base != oldbase->tp_base ||
         !same_slots_added(newbase, oldbase))) {
        goto differs;
    }
    return 1;
differs:
    PyErr_Format(PyExc_TypeError,
                    "%s assignment: "
                    "'%s' object layout differs from '%s'",
                    attr,
                    newto->tp_name,
                    oldto->tp_name);
    return 0;
}



static int
object_set_class_world_stopped(PyObject *self, PyTypeObject *newto)
{
    PyTypeObject *oldto = Py_TYPE(self);

    /* In versions of CPython prior to 3.5, the code in
       compatible_for_assignment was not set up to correctly check for memory
       layout / slot / etc. compatibility for non-HEAPTYPE classes, so we just
       disallowed __class__ assignment in any case that wasn't HEAPTYPE ->
       HEAPTYPE.

       During the 3.5 development cycle, we fixed the code in
       compatible_for_assignment to correctly check compatibility between
       arbitrary types, and started allowing __class__ assignment in all cases
       where the old and new types did in fact have compatible slots and
       memory layout (regardless of whether they were implemented as HEAPTYPEs
       or not).

       Just before 3.5 was released, though, we discovered that this led to
       problems with immutable types like int, where the interpreter assumes
       they are immutable and interns some values. Formerly this wasn't a
       problem, because they really were immutable -- in particular, all the
       types where the interpreter applied this interning trick happened to
       also be statically allocated, so the old HEAPTYPE rules were
       "accidentally" stopping them from allowing __class__ assignment. But
       with the changes to __class__ assignment, we started allowing code like

         class MyInt(int):
             ...
         # Modifies the type of *all* instances of 1 in the whole program,
         # including future instances (!), because the 1 object is interned.
         (1).__class__ = MyInt

       (see https://bugs.python.org/issue24912).

       In theory the proper fix would be to identify which classes rely on
       this invariant and somehow disallow __class__ assignment only for them,
       perhaps via some mechanism like a new Py_TPFLAGS_IMMUTABLE flag (a
       "denylisting" approach). But in practice, since this problem wasn't
       noticed late in the 3.5 RC cycle, we're taking the conservative
       approach and reinstating the same HEAPTYPE->HEAPTYPE check that we used
       to have, plus an "allowlist". For now, the allowlist consists only of
       ModuleType subtypes, since those are the cases that motivated the patch
       in the first place -- see https://bugs.python.org/issue22986 -- and
       since module objects are mutable we can be sure that they are
       definitely not being interned. So now we allow HEAPTYPE->HEAPTYPE *or*
       ModuleType subtype -> ModuleType subtype.

       So far as we know, all the code beyond the following 'if' statement
       will correctly handle non-HEAPTYPE classes, and the HEAPTYPE check is
       needed only to protect that subset of non-HEAPTYPE classes for which
       the interpreter has baked in the assumption that all instances are
       truly immutable.
    */
    if (!(PyType_IsSubtype(newto, &PyModule_Type) &&
          PyType_IsSubtype(oldto, &PyModule_Type)) &&
        (_PyType_HasFeature(newto, Py_TPFLAGS_IMMUTABLETYPE) ||
         _PyType_HasFeature(oldto, Py_TPFLAGS_IMMUTABLETYPE))) {
        PyErr_Format(PyExc_TypeError,
                     "__class__ assignment only supported for mutable types "
                     "or ModuleType subclasses");
        return -1;
    }

    if (compatible_for_assignment(oldto, newto, "__class__", 1)) {
        /* Changing the class will change the implicit dict keys,
         * so we must materialize the dictionary first. */
        if (oldto->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
            PyDictObject *dict = _PyObject_GetManagedDict(self);
            if (dict == NULL) {
                dict = _PyObject_MaterializeManagedDict_LockHeld(self);
                if (dict == NULL) {
                    return -1;
                }
            }

            assert(_PyObject_GetManagedDict(self) == dict);

            int err;
            Py_BEGIN_CRITICAL_SECTION(dict);
            err = _PyDict_DetachFromObject(dict, self);
            Py_END_CRITICAL_SECTION();
            if (err < 0) {
                return -1;
            }

        }
        if (newto->tp_flags & Py_TPFLAGS_HEAPTYPE) {
            Py_INCREF(newto);
        }

        Py_SET_TYPE(self, newto);

        return 0;
    }
    else {
        return -1;
    }
}

static int
object_set_class(PyObject *self, PyObject *value, void *closure)
{

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete __class__ attribute");
        return -1;
    }
    if (!PyType_Check(value)) {
        PyErr_Format(PyExc_TypeError,
          "__class__ must be set to a class, not '%s' object",
          Py_TYPE(value)->tp_name);
        return -1;
    }
    PyTypeObject *newto = (PyTypeObject *)value;

    if (PySys_Audit("object.__setattr__", "OsO",
                    self, "__class__", value) < 0) {
        return -1;
    }

    int unique = _PyObject_IsUniquelyReferenced(self);
    if (!unique) {
        types_stop_world();
    }
    PyTypeObject *oldto = Py_TYPE(self);
    int res = object_set_class_world_stopped(self, newto);
    if (!unique) {
        types_start_world();
    }
    if (res == 0) {
        if (oldto->tp_flags & Py_TPFLAGS_HEAPTYPE) {
            Py_DECREF(oldto);
        }

        RARE_EVENT_INC(set_class);
        return 0;
    }
    return res;
}

static PyGetSetDef object_getsets[] = {
    {"__class__", object_get_class, object_set_class,
     PyDoc_STR("the object's class")},
    {0}
};


/* Stuff to implement __reduce_ex__ for pickle protocols >= 2.
   We fall back to helpers in copyreg for:
   - pickle protocols < 2
   - calculating the list of slot names (done only once per class)
   - the __newobj__ function (which is used as a token but never called)
*/

static PyObject *
import_copyreg(void)
{
    /* Try to fetch cached copy of copyreg from sys.modules first in an
       attempt to avoid the import overhead. Previously this was implemented
       by storing a reference to the cached module in a static variable, but
       this broke when multiple embedded interpreters were in use (see issue
       #17408 and #19088). */
    PyObject *copyreg_module = PyImport_GetModule(&_Py_ID(copyreg));
    if (copyreg_module != NULL) {
        return copyreg_module;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    return PyImport_Import(&_Py_ID(copyreg));
}

static PyObject *
_PyType_GetSlotNames(PyTypeObject *cls)
{
    PyObject *copyreg;
    PyObject *slotnames;

    assert(PyType_Check(cls));

    /* Get the slot names from the cache in the class if possible. */
    PyObject *dict = lookup_tp_dict(cls);
    if (PyDict_GetItemRef(dict, &_Py_ID(__slotnames__), &slotnames) < 0) {
        return NULL;
    }
    if (slotnames != NULL) {
        if (slotnames != Py_None && !PyList_Check(slotnames)) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s.__slotnames__ should be a list or None, "
                         "not %.200s",
                         cls->tp_name, Py_TYPE(slotnames)->tp_name);
            Py_DECREF(slotnames);
            return NULL;
        }
        return slotnames;
    }

    /* The class does not have the slot names cached yet. */
    copyreg = import_copyreg();
    if (copyreg == NULL)
        return NULL;

    /* Use _slotnames function from the copyreg module to find the slots
       by this class and its bases. This function will cache the result
       in __slotnames__. */
    slotnames = PyObject_CallMethodOneArg(
            copyreg, &_Py_ID(_slotnames), (PyObject *)cls);
    Py_DECREF(copyreg);
    if (slotnames == NULL)
        return NULL;

    if (slotnames != Py_None && !PyList_Check(slotnames)) {
        PyErr_SetString(PyExc_TypeError,
                        "copyreg._slotnames didn't return a list or None");
        Py_DECREF(slotnames);
        return NULL;
    }

    return slotnames;
}

static PyObject *
object_getstate_default(PyObject *obj, int required)
{
    PyObject *obj_owner = NULL;
    PyObject *state = NULL;
    PyObject *state2 = NULL;
    PyObject *slotnames = NULL;
    PyObject *slots = NULL;
    PyObject *name = NULL;
    PyObject *value = NULL;
    PyObject *selected_type = NULL;
    PyObject *diagnostic_type = NULL;
    PyObject *diagnostic_name = NULL;
    PyValue name_value = PyValue_NULL;
    int rc = -1;

    if (PyErr_Occurred()) {
        goto done;
    }
    obj_owner = Py_NewRef(obj);

    if (required && Py_TYPE(obj)->tp_itemsize) {
        diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(obj));
        if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                              Py_TPFLAGS_HEAPTYPE)) {
            diagnostic_name = Py_NewRef(((PyHeapTypeObject *)diagnostic_type)->ht_name);
        }
        const char *type_name = ((PyTypeObject *)diagnostic_type)->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "cannot pickle %.200s objects",
                     type_name);
        type_mutation_close(&diagnostic_name);
        type_mutation_close(&diagnostic_type);
        goto done;
    }

    if (_PyObject_IsInstanceDictEmpty(obj)) {
        state = Py_NewRef(Py_None);
    }
    else {
        state = PyObject_GenericGetDict(obj, NULL);
    }
    if (state == NULL || PyErr_Occurred()) {
        goto done;
    }

    selected_type = Py_NewRef((PyObject *)Py_TYPE(obj));
    slotnames = _PyType_GetSlotNames((PyTypeObject *)selected_type);
    type_mutation_close(&selected_type);
    if (slotnames == NULL || PyErr_Occurred()) {
        goto done;
    }

    assert(slotnames == Py_None || PyList_Check(slotnames));
    if (required) {
        Py_ssize_t basicsize = PyBaseObject_Type.tp_basicsize;
        if (Py_TYPE(obj)->tp_dictoffset &&
            (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0)
        {
            basicsize += sizeof(PyObject *);
        }
        if (Py_TYPE(obj)->tp_weaklistoffset > 0) {
            basicsize += sizeof(PyObject *);
        }
        if (slotnames != Py_None) {
            basicsize += sizeof(PyObject *) * PyList_GET_SIZE(slotnames);
        }
        if (Py_TYPE(obj)->tp_basicsize > basicsize) {
            type_mutation_close(&slotnames);
            type_mutation_close(&state);
            if (PyErr_Occurred()) {
                goto done;
            }
            diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(obj));
            if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                                  Py_TPFLAGS_HEAPTYPE)) {
                diagnostic_name = Py_NewRef(((PyHeapTypeObject *)diagnostic_type)->ht_name);
            }
            const char *type_name = ((PyTypeObject *)diagnostic_type)->tp_name;
            PyErr_Format(PyExc_TypeError,
                         "cannot pickle '%.200s' object",
                         type_name);
            type_mutation_close(&diagnostic_name);
            type_mutation_close(&diagnostic_type);
            goto done;
        }
    }

    if (slotnames == Py_None || PyList_GET_SIZE(slotnames) == 0) {
        rc = 0;
        goto done;
    }

    slots = PyDict_New();
    if (slots == NULL || PyErr_Occurred()) {
        goto done;
    }

    Py_ssize_t slotnames_size = PyList_GET_SIZE(slotnames);
    for (Py_ssize_t i = 0; i < slotnames_size; i++) {
        name_value = PyList_GetValue(slotnames, i);
        if (PyValue_IsNull(name_value) || PyErr_Occurred()) {
            goto done;
        }
        if (!PyValue_IsObject(name_value) ||
                !PyUnicode_Check(PyValue_AsPyObjectBorrow(name_value))) {
            diagnostic_type = Py_NewRef((PyObject *)PyValue_TYPE(name_value));
            if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                                  Py_TPFLAGS_HEAPTYPE)) {
                diagnostic_name = Py_NewRef(((PyHeapTypeObject *)diagnostic_type)->ht_name);
            }
            const char *type_name = ((PyTypeObject *)diagnostic_type)->tp_name;
            PyErr_Format(PyExc_TypeError,
                         "attribute name must be string, not '%.200s'",
                         type_name);
            type_mutation_close(&diagnostic_name);
            type_mutation_close(&diagnostic_type);
            goto done;
        }
        name = PyValue_AsPyObjectSteal(name_value);
        name_value = PyValue_NULL;

        int status = PyObject_GetOptionalAttr(obj, name, &value);
        if (status < 0 || PyErr_Occurred()) {
            type_mutation_close(&name);
            type_mutation_close(&value);
            goto done;
        }
        if (status == 0) {
            type_mutation_close(&name);
        }
        else {
            int err = PyDict_SetItem(slots, name, value);
            type_mutation_close(&name);
            type_mutation_close(&value);
            if (err < 0 || PyErr_Occurred()) {
                goto done;
            }
        }
        if (PyErr_Occurred()) {
            goto done;
        }
        if (slotnames_size != PyList_GET_SIZE(slotnames)) {
            PyErr_Format(PyExc_RuntimeError,
                         "__slotnames__ changed size during iteration");
            goto done;
        }
    }

    if (PyDict_GET_SIZE(slots) > 0) {
        state2 = _PyTuple_FromPair(state, slots);
        type_mutation_close(&state);
        if (state2 == NULL || PyErr_Occurred()) {
            goto done;
        }
        state = state2;
        state2 = NULL;
    }

    type_mutation_close(&slots);
    if (PyErr_Occurred()) {
        goto done;
    }
    type_mutation_close(&slotnames);
    if (PyErr_Occurred()) {
        goto done;
    }
    rc = 0;
    goto done;

done:
    {
        PyValue retired = name_value;
        name_value = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&name);
    type_mutation_close(&value);
    type_mutation_close(&selected_type);
    type_mutation_close(&diagnostic_name);
    type_mutation_close(&diagnostic_type);
    type_mutation_close(&slotnames);
    type_mutation_close(&slots);
    type_mutation_close(&state2);
    type_mutation_close(&obj_owner);
    if (rc < 0 || PyErr_Occurred()) {
        type_mutation_close(&state);
        return NULL;
    }
    return state;
}

static PyObject *
object_getstate(PyObject *obj, int required)
{
    PyObject *getstate, *state;

    getstate = PyObject_GetAttr(obj, &_Py_ID(__getstate__));
    if (getstate == NULL) {
        return NULL;
    }
    if (PyCFunction_Check(getstate) &&
        PyCFunction_GET_SELF(getstate) == obj &&
        PyCFunction_GET_FUNCTION(getstate) == object___getstate__)
    {
        /* If __getstate__ is not overridden pass the required argument. */
        state = object_getstate_default(obj, required);
    }
    else {
        state = _PyObject_CallNoArgs(getstate);
    }
    Py_DECREF(getstate);
    return state;
}

PyObject *
_PyObject_GetState(PyObject *obj)
{
    return object_getstate(obj, 0);
}

/*[clinic input]
object.__getstate__

Helper for pickle.
[clinic start generated code]*/

static PyObject *
object___getstate___impl(PyObject *self)
/*[clinic end generated code: output=5a2500dcb6217e9e input=692314d8fbe194ee]*/
{
    return object_getstate_default(self, 0);
}

static int
_PyObject_GetNewArguments(PyObject *obj, PyObject **args, PyObject **kwargs)
{
    PyObject *obj_owner = NULL;
    PyObject *method = NULL;
    PyObject *outer = NULL;
    PyObject *local_args = NULL;
    PyObject *local_kwargs = NULL;
    PyObject *diagnostic_type = NULL;
    PyObject *diagnostic_name = NULL;
    PyValue args_value = PyValue_NULL;
    PyValue kwargs_value = PyValue_NULL;
    int rc = -1;

    if (args == NULL || kwargs == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    *args = NULL;
    *kwargs = NULL;
    if (PyErr_Occurred()) {
        return -1;
    }
    obj_owner = Py_NewRef(obj);

    /* We first attempt to fetch the arguments for __new__ by calling
       __getnewargs_ex__ on the object. */
    method = _PyObject_LookupSpecial(obj, &_Py_ID(__getnewargs_ex__));
    if (method == NULL) {
        if (PyErr_Occurred()) {
            goto done;
        }
        goto fallback;
    }
    if (PyErr_Occurred()) {
        goto done;
    }

    outer = _PyObject_CallNoArgs(method);
    type_mutation_close(&method);
    if (outer == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (!PyTuple_Check(outer)) {
        diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(outer));
        if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                              Py_TPFLAGS_HEAPTYPE)) {
            diagnostic_name = Py_NewRef(
                ((PyHeapTypeObject *)diagnostic_type)->ht_name);
        }
        const char *tp_name = ((PyTypeObject *)diagnostic_type)->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "__getnewargs_ex__ should return a tuple, "
                     "not '%.200s'", tp_name);
        type_mutation_close(&diagnostic_name);
        type_mutation_close(&diagnostic_type);
        goto done;
    }
    if (PyTuple_GET_SIZE(outer) != 2) {
        PyErr_Format(PyExc_ValueError,
                     "__getnewargs_ex__ should return a tuple of "
                     "length 2, not %zd", PyTuple_GET_SIZE(outer));
        goto done;
    }
    args_value = PyTuple_GetValue(outer, 0);
    if (PyValue_IsNull(args_value) || PyErr_Occurred()) {
        goto done;
    }
    kwargs_value = PyTuple_GetValue(outer, 1);
    if (PyValue_IsNull(kwargs_value) || PyErr_Occurred()) {
        goto done;
    }
    type_mutation_close(&outer);
    if (PyErr_Occurred()) {
        goto done;
    }

    /* XXX We should perhaps allow None to be passed here. */
    if (!PyValue_IsObject(args_value) ||
            !PyTuple_Check(PyValue_AsPyObjectBorrow(args_value))) {
        diagnostic_type = Py_NewRef((PyObject *)PyValue_TYPE(args_value));
        if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                              Py_TPFLAGS_HEAPTYPE)) {
            diagnostic_name = Py_NewRef(
                ((PyHeapTypeObject *)diagnostic_type)->ht_name);
        }
        const char *tp_name = ((PyTypeObject *)diagnostic_type)->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "first item of the tuple returned by "
                     "__getnewargs_ex__ must be a tuple, not '%.200s'",
                     tp_name);
        type_mutation_close(&diagnostic_name);
        type_mutation_close(&diagnostic_type);
        goto done;
    }
    if (!PyValue_IsObject(kwargs_value) ||
            !PyDict_Check(PyValue_AsPyObjectBorrow(kwargs_value))) {
        diagnostic_type = Py_NewRef((PyObject *)PyValue_TYPE(kwargs_value));
        if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                              Py_TPFLAGS_HEAPTYPE)) {
            diagnostic_name = Py_NewRef(
                ((PyHeapTypeObject *)diagnostic_type)->ht_name);
        }
        const char *tp_name = ((PyTypeObject *)diagnostic_type)->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "second item of the tuple returned by "
                     "__getnewargs_ex__ must be a dict, not '%.200s'",
                     tp_name);
        type_mutation_close(&diagnostic_name);
        type_mutation_close(&diagnostic_type);
        goto done;
    }
    local_args = PyValue_AsPyObjectSteal(args_value);
    args_value = PyValue_NULL;
    local_kwargs = PyValue_AsPyObjectSteal(kwargs_value);
    kwargs_value = PyValue_NULL;
    rc = 0;
    goto done;

fallback:
    method = _PyObject_LookupSpecial(obj, &_Py_ID(__getnewargs__));
    if (method == NULL) {
        if (PyErr_Occurred()) {
            goto done;
        }
        rc = 0;
        goto done;
    }
    if (PyErr_Occurred()) {
        goto done;
    }
    local_args = _PyObject_CallNoArgs(method);
    type_mutation_close(&method);
    if (local_args == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (!PyTuple_Check(local_args)) {
        diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(local_args));
        if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                              Py_TPFLAGS_HEAPTYPE)) {
            diagnostic_name = Py_NewRef(
                ((PyHeapTypeObject *)diagnostic_type)->ht_name);
        }
        const char *tp_name = ((PyTypeObject *)diagnostic_type)->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "__getnewargs__ should return a tuple, "
                     "not '%.200s'", tp_name);
        type_mutation_close(&diagnostic_name);
        type_mutation_close(&diagnostic_type);
        goto done;
    }
    rc = 0;

done:
    type_mutation_close(&method);
    type_mutation_close(&outer);
    {
        PyValue retired = args_value;
        args_value = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    {
        PyValue retired = kwargs_value;
        kwargs_value = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&diagnostic_name);
    type_mutation_close(&diagnostic_type);
    type_mutation_close(&obj_owner);

    if (rc < 0 || PyErr_Occurred()) {
        type_mutation_close(&local_args);
        type_mutation_close(&local_kwargs);
        return -1;
    }
    *args = local_args;
    local_args = NULL;
    *kwargs = local_kwargs;
    local_kwargs = NULL;
    return 0;
}

static int
_PyObject_GetItemsIter(PyObject *obj, PyObject **listitems,
                       PyObject **dictitems)
{
    if (listitems == NULL || dictitems == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }

    if (!PyList_Check(obj)) {
        *listitems = Py_NewRef(Py_None);
    }
    else {
        *listitems = PyObject_GetIter(obj);
        if (*listitems == NULL)
            return -1;
    }

    if (!PyDict_Check(obj)) {
        *dictitems = Py_NewRef(Py_None);
    }
    else {
        PyObject *items = PyObject_CallMethodNoArgs(obj, &_Py_ID(items));
        if (items == NULL) {
            Py_CLEAR(*listitems);
            return -1;
        }
        *dictitems = PyObject_GetIter(items);
        Py_DECREF(items);
        if (*dictitems == NULL) {
            Py_CLEAR(*listitems);
            return -1;
        }
    }

    assert(*listitems != NULL && *dictitems != NULL);

    return 0;
}

static PyObject *
reduce_newobj(PyObject *obj)
{
    PyObject *obj_owner = NULL;
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *copyreg = NULL;
    PyObject *newobj = NULL;
    PyObject *newargs = NULL;
    PyObject *state = NULL;
    PyObject *listitems = NULL;
    PyObject *dictitems = NULL;
    PyObject *result = NULL;
    PyObject *class_owner = NULL;
    PyObject *diagnostic_type = NULL;
    PyObject *diagnostic_name = NULL;
    PyValue item_value = PyValue_NULL;
    int hasargs = 0;
    int rc = -1;

    if (PyErr_Occurred()) {
        goto done;
    }
    obj_owner = Py_NewRef(obj);
    if (Py_TYPE(obj)->tp_new == NULL) {
        diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(obj));
        if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                              Py_TPFLAGS_HEAPTYPE)) {
            diagnostic_name = Py_NewRef(
                ((PyHeapTypeObject *)diagnostic_type)->ht_name);
        }
        const char *tp_name = ((PyTypeObject *)diagnostic_type)->tp_name;
        PyErr_Format(PyExc_TypeError,
                     "cannot pickle '%.200s' object",
                     tp_name);
        type_mutation_close(&diagnostic_name);
        type_mutation_close(&diagnostic_type);
        goto done;
    }
    if (_PyObject_GetNewArguments(obj, &args, &kwargs) < 0 ||
        PyErr_Occurred()) {
        goto done;
    }

    copyreg = import_copyreg();
    if (copyreg == NULL || PyErr_Occurred()) {
        goto done;
    }
    hasargs = (args != NULL);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        Py_ssize_t i, n;

        type_mutation_close(&kwargs);
        if (PyErr_Occurred()) {
            goto done;
        }
        newobj = PyObject_GetAttr(copyreg, &_Py_ID(__newobj__));
        type_mutation_close(&copyreg);
        if (newobj == NULL || PyErr_Occurred()) {
            goto done;
        }
        n = args ? PyTuple_GET_SIZE(args) : 0;
        newargs = PyTuple_New(n + 1);
        if (newargs == NULL || PyErr_Occurred()) {
            goto done;
        }
        PyTuple_SET_VALUE(newargs, 0,
                          PyValue_FromPyObjectNew((PyObject *)Py_TYPE(obj)));
        for (i = 0; i < n; i++) {
            item_value = PyTuple_GetValue(args, i);
            if (PyValue_IsNull(item_value) || PyErr_Occurred()) {
                goto done;
            }
            PyTuple_SET_VALUE(newargs, i + 1, item_value);
            item_value = PyValue_NULL;
        }
        type_mutation_close(&args);
        if (PyErr_Occurred()) {
            goto done;
        }
        goto state_phase;
    }
    else if (args != NULL) {
        newobj = PyObject_GetAttr(copyreg, &_Py_ID(__newobj_ex__));
        type_mutation_close(&copyreg);
        if (newobj == NULL || PyErr_Occurred()) {
            goto done;
        }
        class_owner = Py_NewRef((PyObject *)Py_TYPE(obj));
        newargs = PyTuple_Pack(3, class_owner, args, kwargs);
        type_mutation_close(&args);
        type_mutation_close(&kwargs);
        type_mutation_close(&class_owner);
        if (newargs == NULL || PyErr_Occurred()) {
            goto done;
        }
        goto state_phase;
    }
    else {
        type_mutation_close(&copyreg);
        type_mutation_close(&kwargs);
        if (PyErr_Occurred()) {
            goto done;
        }
        PyErr_BadInternalCall();
        goto done;
    }

state_phase:
    state = object_getstate(obj,
                            !(hasargs || PyList_Check(obj) || PyDict_Check(obj)));
    if (state == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (_PyObject_GetItemsIter(obj, &listitems, &dictitems) < 0 ||
        PyErr_Occurred()) {
        goto done;
    }
    result = PyTuple_Pack(5, newobj, newargs, state, listitems, dictitems);
    if (result != NULL && !PyErr_Occurred()) {
        rc = 0;
    }
    goto done;

done:
    {
        PyValue retired = item_value;
        item_value = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&args);
    type_mutation_close(&kwargs);
    type_mutation_close(&copyreg);
    type_mutation_close(&class_owner);
    type_mutation_close(&diagnostic_name);
    type_mutation_close(&diagnostic_type);
    type_mutation_close(&newobj);
    type_mutation_close(&newargs);
    type_mutation_close(&state);
    type_mutation_close(&listitems);
    type_mutation_close(&dictitems);
    type_mutation_close(&obj_owner);
    if (rc < 0 || PyErr_Occurred()) {
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

/*
 * There were two problems when object.__reduce__ and object.__reduce_ex__
 * were implemented in the same function:
 *  - trying to pickle an object with a custom __reduce__ method that
 *    fell back to object.__reduce__ in certain circumstances led to
 *    infinite recursion at Python level and eventual RecursionError.
 *  - Pickling objects that lied about their type by overwriting the
 *    __class__ descriptor could lead to infinite recursion at C level
 *    and eventual segfault.
 *
 * Because of backwards compatibility, the two methods still have to
 * behave in the same way, even if this is not required by the pickle
 * protocol. This common functionality was moved to the _common_reduce
 * function.
 */
static PyObject *
_common_reduce(PyObject *self, int proto)
{
    PyObject *copyreg, *res;

    if (proto >= 2)
        return reduce_newobj(self);

    copyreg = import_copyreg();
    if (!copyreg)
        return NULL;

    res = PyObject_CallMethod(copyreg, "_reduce_ex", "Oi", self, proto);
    Py_DECREF(copyreg);

    return res;
}

/*[clinic input]
object.__reduce__

Helper for pickle.
[clinic start generated code]*/

static PyObject *
object___reduce___impl(PyObject *self)
/*[clinic end generated code: output=d4ca691f891c6e2f input=11562e663947e18b]*/
{
    return _common_reduce(self, 0);
}

/*[clinic input]
object.__reduce_ex__

  protocol: int
  /

Helper for pickle.
[clinic start generated code]*/

static PyObject *
object___reduce_ex___impl(PyObject *self, int protocol)
/*[clinic end generated code: output=2e157766f6b50094 input=f326b43fb8a4c5ff]*/
{
    PyObject *reduce;
    if (PyObject_GetOptionalAttr(self, &_Py_ID(__reduce__), &reduce) < 0) {
        return NULL;
    }
    if (reduce != NULL) {
        PyObject *cls, *clsreduce;
        int override;

        cls = (PyObject *) Py_TYPE(self);
        clsreduce = PyObject_GetAttr(cls, &_Py_ID(__reduce__));
        if (clsreduce == NULL) {
            Py_DECREF(reduce);
            return NULL;
        }

        PyInterpreterState *interp = _PyInterpreterState_GET();
        override = (clsreduce != _Py_INTERP_CACHED_OBJECT(interp, objreduce));
        Py_DECREF(clsreduce);
        if (override) {
            PyObject *res = _PyObject_CallNoArgs(reduce);
            Py_DECREF(reduce);
            return res;
        }
        else
            Py_DECREF(reduce);
    }

    return _common_reduce(self, protocol);
}

static PyObject *
object_subclasshook(PyObject *cls, PyObject *args)
{
    Py_RETURN_NOTIMPLEMENTED;
}

PyDoc_STRVAR(object_subclasshook_doc,
"Abstract classes can override this to customize issubclass().\n"
"\n"
"This is invoked early on by abc.ABCMeta.__subclasscheck__().\n"
"It should return True, False or NotImplemented.  If it returns\n"
"NotImplemented, the normal algorithm is used.  Otherwise, it\n"
"overrides the normal algorithm (and the outcome is cached).\n");

static PyObject *
object_init_subclass(PyObject *cls, PyObject *arg)
{
    Py_RETURN_NONE;
}

PyDoc_STRVAR(object_init_subclass_doc,
"This method is called when a class is subclassed.\n"
"\n"
"The default implementation does nothing. It may be\n"
"overridden to extend subclasses.\n");

/*[clinic input]
object.__format__

  format_spec: unicode
  /

Default object formatter.

Return str(self) if format_spec is empty. Raise TypeError otherwise.
[clinic start generated code]*/

static PyObject *
object___format___impl(PyObject *self, PyObject *format_spec)
/*[clinic end generated code: output=34897efb543a974b input=b94d8feb006689ea]*/
{
    /* Issue 7994: If we're converting to a string, we
       should reject format specifications */
    if (PyUnicode_GET_LENGTH(format_spec) > 0) {
        PyErr_Format(PyExc_TypeError,
                     "unsupported format string passed to %.200s.__format__",
                     Py_TYPE(self)->tp_name);
        return NULL;
    }
    return PyObject_Str(self);
}

/*[clinic input]
object.__sizeof__

Size of object in memory, in bytes.
[clinic start generated code]*/

static PyObject *
object___sizeof___impl(PyObject *self)
/*[clinic end generated code: output=73edab332f97d550 input=1200ff3dfe485306]*/
{
    Py_ssize_t res, isize;

    res = 0;
    isize = Py_TYPE(self)->tp_itemsize;
    if (isize > 0) {
        /* This assumes that ob_size is valid if tp_itemsize is not 0,
         which isn't true for PyLongObject. */
        res = _PyVarObject_CAST(self)->ob_size * isize;
    }
    res += Py_TYPE(self)->tp_basicsize;

    return PyLong_FromSsize_t(res);
}

/* __dir__ for generic objects: returns __dict__, __class__,
   and recursively up the __class__.__bases__ chain.
*/
/*[clinic input]
object.__dir__

Default dir() implementation.
[clinic start generated code]*/

static PyObject *
object___dir___impl(PyObject *self)
/*[clinic end generated code: output=66dd48ea62f26c90 input=0a89305bec669b10]*/
{
    PyObject *result = NULL;
    PyObject *dict = NULL;
    PyObject *itsclass = NULL;

    /* Get __dict__ (which may or may not be a real dict...) */
    if (PyObject_GetOptionalAttr(self, &_Py_ID(__dict__), &dict) < 0) {
        return NULL;
    }
    if (dict == NULL) {
        dict = PyDict_New();
    }
    else if (!PyDict_Check(dict)) {
        Py_DECREF(dict);
        dict = PyDict_New();
    }
    else {
        /* Copy __dict__ to avoid mutating it. */
        PyObject *temp = PyDict_Copy(dict);
        Py_SETREF(dict, temp);
    }

    if (dict == NULL)
        goto error;

    /* Merge in attrs reachable from its class. */
    if (PyObject_GetOptionalAttr(self, &_Py_ID(__class__), &itsclass) < 0) {
        goto error;
    }
    /* XXX(tomer): Perhaps fall back to Py_TYPE(obj) if no
                   __class__ exists? */
    if (itsclass != NULL && merge_class_dict(dict, itsclass) < 0)
        goto error;

    result = PyDict_Keys(dict);
    /* fall through */
error:
    Py_XDECREF(itsclass);
    Py_XDECREF(dict);
    return result;
}

static PyMethodDef object_methods[] = {
    OBJECT___REDUCE_EX___METHODDEF
    OBJECT___REDUCE___METHODDEF
    OBJECT___GETSTATE___METHODDEF
    {"__subclasshook__", object_subclasshook, METH_CLASS | METH_O,
     object_subclasshook_doc},
    {"__init_subclass__", object_init_subclass, METH_CLASS | METH_NOARGS,
     object_init_subclass_doc},
    OBJECT___FORMAT___METHODDEF
    OBJECT___SIZEOF___METHODDEF
    OBJECT___DIR___METHODDEF
    {0}
};

PyDoc_STRVAR(object_doc,
"object()\n--\n\n"
"The base class of the class hierarchy.\n\n"
"When called, it accepts no arguments and returns a new featureless\n"
"instance that has no instance attributes and cannot be given any.\n");

PyTypeObject PyBaseObject_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "object",                                   /* tp_name */
    sizeof(PyObject),                           /* tp_basicsize */
    0,                                          /* tp_itemsize */
    object_dealloc,                             /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    object_repr,                                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    PyObject_GenericHash,                       /* tp_hash */
    0,                                          /* tp_call */
    object_str,                                 /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    object_doc,                                 /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    object_richcompare,                         /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    object_methods,                             /* tp_methods */
    0,                                          /* tp_members */
    object_getsets,                             /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    object_init,                                /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    object_new,                                 /* tp_new */
    PyObject_Free,                              /* tp_free */
};


static int
type_add_method(PyTypeObject *type, PyMethodDef *meth)
{
    PyObject *descr;
    int isdescr = 1;
    if (meth->ml_flags & METH_CLASS) {
        if (meth->ml_flags & METH_STATIC) {
            PyErr_SetString(PyExc_ValueError,
                    "method cannot be both class and static");
            return -1;
        }
        descr = PyDescr_NewClassMethod(type, meth);
    }
    else if (meth->ml_flags & METH_STATIC) {
        PyObject *mod = type_module(type);
        if (mod == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                return -1;
            }
            PyErr_Clear();
        }
        PyObject *cfunc = PyCFunction_NewEx(meth, (PyObject*)type, mod);
        Py_XDECREF(mod);
        if (cfunc == NULL) {
            return -1;
        }
        descr = PyStaticMethod_New(cfunc);
        isdescr = 0;  // PyStaticMethod is not PyDescrObject
        Py_DECREF(cfunc);
    }
    else {
        descr = PyDescr_NewMethod(type, meth);
    }
    if (descr == NULL) {
        return -1;
    }

    PyObject *name;
    if (isdescr) {
        name = PyDescr_NAME(descr);
    }
    else {
        name = PyUnicode_FromString(meth->ml_name);
        if (name == NULL) {
            Py_DECREF(descr);
            return -1;
        }
    }

    int err;
    PyObject *dict = lookup_tp_dict(type);
    if (!(meth->ml_flags & METH_COEXIST)) {
        err = PyDict_SetDefaultRef(dict, name, descr, NULL) < 0;
    }
    else {
        err = PyDict_SetItem(dict, name, descr) < 0;
    }
    if (!isdescr) {
        Py_DECREF(name);
    }
    Py_DECREF(descr);
    if (err) {
        return -1;
    }
    return 0;
}

int
_PyType_AddMethod(PyTypeObject *type, PyMethodDef *meth)
{
    return type_add_method(type, meth);
}


/* Add the methods from tp_methods to the __dict__ in a type object */
static int
type_add_methods(PyTypeObject *type)
{
    PyMethodDef *meth = type->tp_methods;
    if (meth == NULL) {
        return 0;
    }

    for (; meth->ml_name != NULL; meth++) {
        if (type_add_method(type, meth) < 0) {
            return -1;
        }
    }
    return 0;
}


static int
type_add_members(PyTypeObject *type)
{
    PyMemberDef *memb = type->tp_members;
    if (memb == NULL) {
        return 0;
    }

    PyObject *dict = lookup_tp_dict(type);
    for (; memb->name != NULL; memb++) {
        PyObject *descr = PyDescr_NewMember(type, memb);
        if (descr == NULL)
            return -1;

        if (PyDict_SetDefaultRef(dict, PyDescr_NAME(descr), descr, NULL) < 0) {
            Py_DECREF(descr);
            return -1;
        }
        Py_DECREF(descr);
    }
    return 0;
}


static int
type_add_getset(PyTypeObject *type)
{
    PyGetSetDef *gsp = type->tp_getset;
    if (gsp == NULL) {
        return 0;
    }

    PyObject *dict = lookup_tp_dict(type);
    for (; gsp->name != NULL; gsp++) {
        PyObject *descr = PyDescr_NewGetSet(type, gsp);
        if (descr == NULL) {
            return -1;
        }

        if (PyDict_SetDefaultRef(dict, PyDescr_NAME(descr), descr, NULL) < 0) {
            Py_DECREF(descr);
            return -1;
        }
        Py_DECREF(descr);
    }
    return 0;
}


static void
inherit_special(PyTypeObject *type, PyTypeObject *base)
{
    /* Copying tp_traverse and tp_clear is connected to the GC flags */
    if (!(type->tp_flags & Py_TPFLAGS_HAVE_GC) &&
        (base->tp_flags & Py_TPFLAGS_HAVE_GC) &&
        (!type->tp_traverse && !type->tp_clear)) {
        type_add_flags(type, Py_TPFLAGS_HAVE_GC);
        if (type->tp_traverse == NULL)
            type->tp_traverse = base->tp_traverse;
        if (type->tp_clear == NULL)
            type->tp_clear = base->tp_clear;
    }
    type_add_flags(type, base->tp_flags & (Py_TPFLAGS_PREHEADER
                                           | Py_TPFLAGS_DEALLOC_READS_REFERENTS));

    if (type->tp_basicsize == 0)
        type->tp_basicsize = base->tp_basicsize;

    /* Copy other non-function slots */

#define COPYVAL(SLOT) \
    if (type->SLOT == 0) { type->SLOT = base->SLOT; }

    COPYVAL(tp_itemsize);
    COPYVAL(tp_weaklistoffset);
    COPYVAL(tp_dictoffset);

#undef COPYVAL

    /* Setup fast subclass flags */
    PyObject *mro = lookup_tp_mro(base);
    unsigned long flags = 0;
    if (is_subtype_with_mro(mro, base, (PyTypeObject*)PyExc_BaseException)) {
        flags |= Py_TPFLAGS_BASE_EXC_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyType_Type)) {
        flags |= Py_TPFLAGS_TYPE_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyLong_Type)) {
        flags |= Py_TPFLAGS_LONG_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyBytes_Type)) {
        flags |= Py_TPFLAGS_BYTES_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyUnicode_Type)) {
        flags |= Py_TPFLAGS_UNICODE_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyTuple_Type)) {
        flags |= Py_TPFLAGS_TUPLE_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyList_Type)) {
        flags |= Py_TPFLAGS_LIST_SUBCLASS;
    }
    else if (is_subtype_with_mro(mro, base, &PyDict_Type)) {
        flags |= Py_TPFLAGS_DICT_SUBCLASS;
    }

    /* Setup some inheritable flags */
    if (PyType_HasFeature(base, _Py_TPFLAGS_MATCH_SELF)) {
        flags |= _Py_TPFLAGS_MATCH_SELF;
    }
    if (PyType_HasFeature(base, Py_TPFLAGS_ITEMS_AT_END)) {
        flags |= Py_TPFLAGS_ITEMS_AT_END;
    }
    type_add_flags(type, flags);
}

static int
overrides_hash(PyTypeObject *type)
{
    PyObject *dict = lookup_tp_dict(type);

    assert(dict != NULL);
    int r = PyDict_Contains(dict, &_Py_ID(__eq__));
    if (r == 0) {
        r = PyDict_Contains(dict, &_Py_ID(__hash__));
    }
    return r;
}

static int
inherit_slots(PyTypeObject *type, PyTypeObject *base)
{
    PyTypeObject *basebase;

#undef SLOTDEFINED
#undef COPYSLOT
#undef COPYNUM
#undef COPYSEQ
#undef COPYMAP
#undef COPYBUF

#define SLOTDEFINED(SLOT) \
    (base->SLOT != 0 && \
     (basebase == NULL || base->SLOT != basebase->SLOT))

#define COPYSLOT(SLOT) \
    if (!type->SLOT && SLOTDEFINED(SLOT)) type->SLOT = base->SLOT

#define COPYASYNC(SLOT) COPYSLOT(tp_as_async->SLOT)
#define COPYNUM(SLOT) COPYSLOT(tp_as_number->SLOT)
#define COPYSEQ(SLOT) COPYSLOT(tp_as_sequence->SLOT)
#define COPYMAP(SLOT) COPYSLOT(tp_as_mapping->SLOT)
#define COPYBUF(SLOT) COPYSLOT(tp_as_buffer->SLOT)

    /* This won't inherit indirect slots (from tp_as_number etc.)
       if type doesn't provide the space. */

    if (type->tp_as_number != NULL && base->tp_as_number != NULL) {
        basebase = base->tp_base;
        if (basebase->tp_as_number == NULL)
            basebase = NULL;
        COPYNUM(nb_add);
        COPYNUM(nb_subtract);
        COPYNUM(nb_multiply);
        COPYNUM(nb_remainder);
        COPYNUM(nb_divmod);
        COPYNUM(nb_power);
        COPYNUM(nb_negative);
        COPYNUM(nb_positive);
        COPYNUM(nb_absolute);
        COPYNUM(nb_bool);
        COPYNUM(nb_invert);
        COPYNUM(nb_lshift);
        COPYNUM(nb_rshift);
        COPYNUM(nb_and);
        COPYNUM(nb_xor);
        COPYNUM(nb_or);
        COPYNUM(nb_int);
        COPYNUM(nb_float);
        COPYNUM(nb_inplace_add);
        COPYNUM(nb_inplace_subtract);
        COPYNUM(nb_inplace_multiply);
        COPYNUM(nb_inplace_remainder);
        COPYNUM(nb_inplace_power);
        COPYNUM(nb_inplace_lshift);
        COPYNUM(nb_inplace_rshift);
        COPYNUM(nb_inplace_and);
        COPYNUM(nb_inplace_xor);
        COPYNUM(nb_inplace_or);
        COPYNUM(nb_true_divide);
        COPYNUM(nb_floor_divide);
        COPYNUM(nb_inplace_true_divide);
        COPYNUM(nb_inplace_floor_divide);
        COPYNUM(nb_index);
        COPYNUM(nb_matrix_multiply);
        COPYNUM(nb_inplace_matrix_multiply);
    }

    if (type->tp_as_async != NULL && base->tp_as_async != NULL) {
        basebase = base->tp_base;
        if (basebase->tp_as_async == NULL)
            basebase = NULL;
        COPYASYNC(am_await);
        COPYASYNC(am_aiter);
        COPYASYNC(am_anext);
    }

    if (type->tp_as_sequence != NULL && base->tp_as_sequence != NULL) {
        basebase = base->tp_base;
        if (basebase->tp_as_sequence == NULL)
            basebase = NULL;
        COPYSEQ(sq_length);
        COPYSEQ(sq_concat);
        COPYSEQ(sq_repeat);
        COPYSEQ(sq_item);
        COPYSEQ(sq_ass_item);
        COPYSEQ(sq_contains);
        COPYSEQ(sq_inplace_concat);
        COPYSEQ(sq_inplace_repeat);
    }

    if (type->tp_as_mapping != NULL && base->tp_as_mapping != NULL) {
        basebase = base->tp_base;
        if (basebase->tp_as_mapping == NULL)
            basebase = NULL;
        COPYMAP(mp_length);
        COPYMAP(mp_subscript);
        COPYMAP(mp_ass_subscript);
    }

    if (type->tp_as_buffer != NULL && base->tp_as_buffer != NULL) {
        basebase = base->tp_base;
        if (basebase->tp_as_buffer == NULL)
            basebase = NULL;
        COPYBUF(bf_getbuffer);
        COPYBUF(bf_releasebuffer);
    }

    basebase = base->tp_base;

    COPYSLOT(tp_dealloc);
    if (type->tp_getattr == NULL && type->tp_getattro == NULL) {
        type->tp_getattr = base->tp_getattr;
        type->tp_getattro = base->tp_getattro;
    }
    if (type->tp_setattr == NULL && type->tp_setattro == NULL) {
        type->tp_setattr = base->tp_setattr;
        type->tp_setattro = base->tp_setattro;
    }
    COPYSLOT(tp_repr);
    /* tp_hash see tp_richcompare */
    {
        /* Always inherit tp_vectorcall_offset to support PyVectorcall_Call().
         * If Py_TPFLAGS_HAVE_VECTORCALL is not inherited, then vectorcall
         * won't be used automatically. */
        COPYSLOT(tp_vectorcall_offset);

        /* Inherit Py_TPFLAGS_HAVE_VECTORCALL if tp_call is not overridden */
        if (!type->tp_call &&
            _PyType_HasFeature(base, Py_TPFLAGS_HAVE_VECTORCALL))
        {
            type_add_flags(type, Py_TPFLAGS_HAVE_VECTORCALL);
        }
        COPYSLOT(tp_call);
    }
    COPYSLOT(tp_str);
    {
        /* Copy comparison-related slots only when
           not overriding them anywhere */
        if (type->tp_richcompare == NULL &&
            type->tp_hash == NULL)
        {
            int r = overrides_hash(type);
            if (r < 0) {
                return -1;
            }
            if (!r) {
                type->tp_richcompare = base->tp_richcompare;
                type->tp_hash = base->tp_hash;
            }
        }
    }
    {
        COPYSLOT(tp_iter);
        COPYSLOT(tp_iternext);
    }
    {
        COPYSLOT(tp_descr_get);
        /* Inherit Py_TPFLAGS_METHOD_DESCRIPTOR if tp_descr_get was inherited,
         * but only for extension types */
        if (base->tp_descr_get &&
            type->tp_descr_get == base->tp_descr_get &&
            _PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE) &&
            _PyType_HasFeature(base, Py_TPFLAGS_METHOD_DESCRIPTOR))
        {
            type_add_flags(type, Py_TPFLAGS_METHOD_DESCRIPTOR);
        }
        COPYSLOT(tp_descr_set);
        COPYSLOT(tp_dictoffset);
        COPYSLOT(tp_init);
        COPYSLOT(tp_alloc);
        COPYSLOT(tp_is_gc);
        COPYSLOT(tp_finalize);
        if ((type->tp_flags & Py_TPFLAGS_HAVE_GC) ==
            (base->tp_flags & Py_TPFLAGS_HAVE_GC)) {
            /* They agree about gc. */
            COPYSLOT(tp_free);
        }
        else if ((type->tp_flags & Py_TPFLAGS_HAVE_GC) &&
                 type->tp_free == NULL &&
                 base->tp_free == PyObject_Free) {
            /* A bit of magic to plug in the correct default
             * tp_free function when a derived class adds gc,
             * didn't define tp_free, and the base uses the
             * default non-gc tp_free.
             */
            type->tp_free = PyObject_GC_Del;
        }
        /* else they didn't agree about gc, and there isn't something
         * obvious to be done -- the type is on its own.
         */
    }
    return 0;
}

static int add_operators(PyTypeObject *type);
static int add_tp_new_wrapper(PyTypeObject *type);

#define COLLECTION_FLAGS (Py_TPFLAGS_SEQUENCE | Py_TPFLAGS_MAPPING)

static int
type_ready_pre_checks(PyTypeObject *type)
{
    /* Consistency checks for PEP 590:
     * - Py_TPFLAGS_METHOD_DESCRIPTOR requires tp_descr_get
     * - Py_TPFLAGS_HAVE_VECTORCALL requires tp_call and
     *   tp_vectorcall_offset > 0
     * To avoid mistakes, we require this before inheriting.
     */
    if (type->tp_flags & Py_TPFLAGS_METHOD_DESCRIPTOR) {
        _PyObject_ASSERT((PyObject *)type, type->tp_descr_get != NULL);
    }
    if (type->tp_flags & Py_TPFLAGS_HAVE_VECTORCALL) {
        _PyObject_ASSERT((PyObject *)type, type->tp_vectorcall_offset > 0);
        _PyObject_ASSERT((PyObject *)type, type->tp_call != NULL);
    }

    /* Consistency checks for pattern matching
     * Py_TPFLAGS_SEQUENCE and Py_TPFLAGS_MAPPING are mutually exclusive */
    _PyObject_ASSERT((PyObject *)type, (type->tp_flags & COLLECTION_FLAGS) != COLLECTION_FLAGS);

    if (type->tp_name == NULL) {
        PyErr_Format(PyExc_SystemError,
                     "Type does not define the tp_name field.");
        return -1;
    }
    return 0;
}


static int
type_ready_set_base(PyTypeObject *type)
{
    /* Initialize tp_base (defaults to BaseObject unless that's us) */
    PyTypeObject *base = type->tp_base;
    if (base == NULL && type != &PyBaseObject_Type) {
        base = &PyBaseObject_Type;
        if (type->tp_flags & Py_TPFLAGS_HEAPTYPE) {
            if (type->tp_base != base) {
                mro_epoch_advance(type);
            }
            type->tp_base = (PyTypeObject*)Py_NewRef((PyObject*)base);
        }
        else {
            type->tp_base = base;
        }
    }
    assert(type->tp_base != NULL || type == &PyBaseObject_Type);

    /* Now the only way base can still be NULL is if type is
     * &PyBaseObject_Type. */

    /* Initialize the base class */
    if (base != NULL && !_PyType_IsReady(base)) {
        if (PyType_Ready(base) < 0) {
            return -1;
        }
    }

    return 0;
}

static int
type_ready_set_type(PyTypeObject *type)
{
    /* Initialize ob_type if NULL.      This means extensions that want to be
       compilable separately on Windows can call PyType_Ready() instead of
       initializing the ob_type field of their type objects. */
    /* The test for base != NULL is really unnecessary, since base is only
       NULL when type is &PyBaseObject_Type, and we know its ob_type is
       not NULL (it's initialized to &PyType_Type).      But coverity doesn't
       know that. */
    PyTypeObject *base = type->tp_base;
    if (Py_IS_TYPE(type, NULL) && base != NULL) {
        Py_SET_TYPE(type, Py_TYPE(base));
    }

    return 0;
}

static int
type_ready_set_bases(PyTypeObject *type, int initial)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        if (!initial) {
            assert(lookup_tp_bases(type) != NULL);
            return 0;
        }
        assert(lookup_tp_bases(type) == NULL);
    }

    PyObject *bases = lookup_tp_bases(type);
    if (bases == NULL) {
        PyTypeObject *base = type->tp_base;
        if (base == NULL) {
            bases = PyTuple_New(0);
        }
        else {
            bases = PyTuple_Pack(1, base);
        }
        if (bases == NULL) {
            return -1;
        }
        set_tp_bases(type, bases, 1);
    }
    return 0;
}


static int
type_ready_set_dict(PyTypeObject *type)
{
    if (lookup_tp_dict(type) != NULL) {
        return 0;
    }

    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return -1;
    }
    set_tp_dict(type, dict);
    return 0;
}


/* If the type dictionary doesn't contain a __doc__, set it from
   the tp_doc slot. */
static int
type_dict_set_doc(PyTypeObject *type)
{
    PyObject *dict = lookup_tp_dict(type);
    int r = PyDict_Contains(dict, &_Py_ID(__doc__));
    if (r < 0) {
        return -1;
    }
    if (r > 0) {
        return 0;
    }

    if (type->tp_doc != NULL) {
        const char *doc_str;
        doc_str = _PyType_DocWithoutSignature(type->tp_name, type->tp_doc);
        PyObject *doc = PyUnicode_FromString(doc_str);
        if (doc == NULL) {
            return -1;
        }

        if (PyDict_SetItem(dict, &_Py_ID(__doc__), doc) < 0) {
            Py_DECREF(doc);
            return -1;
        }
        Py_DECREF(doc);
    }
    else {
        if (PyDict_SetItem(dict, &_Py_ID(__doc__), Py_None) < 0) {
            return -1;
        }
    }
    return 0;
}


static int
type_ready_fill_dict(PyTypeObject *type)
{
    /* Add type-specific descriptors to tp_dict */
    if (add_operators(type) < 0) {
        return -1;
    }
    if (type_add_methods(type) < 0) {
        return -1;
    }
    if (type_add_members(type) < 0) {
        return -1;
    }
    if (type_add_getset(type) < 0) {
        return -1;
    }
    if (type_dict_set_doc(type) < 0) {
        return -1;
    }
    return 0;
}

static int
type_ready_preheader(PyTypeObject *type)
{
    if (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        if (type->tp_dictoffset > 0 || type->tp_dictoffset < -1) {
            PyErr_Format(PyExc_TypeError,
                        "type %s has the Py_TPFLAGS_MANAGED_DICT flag "
                        "but tp_dictoffset is set",
                        type->tp_name);
            return -1;
        }
        if (!(type->tp_flags & Py_TPFLAGS_HAVE_GC)) {
            PyErr_Format(PyExc_SystemError,
                        "type %s has the Py_TPFLAGS_MANAGED_DICT flag "
                        "but not Py_TPFLAGS_HAVE_GC flag",
                        type->tp_name);
            return -1;
        }
        type->tp_dictoffset = -1;
    }
    if (type->tp_flags & Py_TPFLAGS_MANAGED_WEAKREF) {
        if (type->tp_weaklistoffset != 0 &&
            type->tp_weaklistoffset != MANAGED_WEAKREF_OFFSET)
        {
            PyErr_Format(PyExc_TypeError,
                        "type %s has the Py_TPFLAGS_MANAGED_WEAKREF flag "
                        "but tp_weaklistoffset is set",
                        type->tp_name);
            return -1;
        }
        if (!(type->tp_flags & Py_TPFLAGS_HAVE_GC)) {
            PyErr_Format(PyExc_SystemError,
                        "type %s has the Py_TPFLAGS_MANAGED_WEAKREF flag "
                        "but not Py_TPFLAGS_HAVE_GC flag",
                        type->tp_name);
            return -1;
        }
        type->tp_weaklistoffset = MANAGED_WEAKREF_OFFSET;
    }
    return 0;
}

static int
type_ready_mro(PyTypeObject *type, int initial)
{
    ASSERT_TYPE_LOCK_HELD();

    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        if (!initial) {
            assert(lookup_tp_mro(type) != NULL);
            return 0;
        }
        assert(lookup_tp_mro(type) == NULL);
    }

    /* Calculate method resolution order */
    if (mro_internal(type, initial, NULL) < 0) {
        return -1;
    }
    PyObject *mro = lookup_tp_mro(type);
    assert(mro != NULL);
    assert(PyTuple_Check(mro));

    /* All bases of statically allocated type should be statically allocated,
       and static builtin types must have static builtin bases. */
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        assert(type->tp_flags & Py_TPFLAGS_IMMUTABLETYPE);
        Py_ssize_t n = PyTuple_GET_SIZE(mro);
        for (Py_ssize_t i = 0; i < n; i++) {
            PyValue value = PyTuple_GET_VALUE(mro, i);
            assert(PyValue_IsObject(value));
            PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(value));
            if (base->tp_flags & Py_TPFLAGS_HEAPTYPE) {
                PyObject *base_owner = Py_NewRef((PyObject *)base);
                PyObject *name_owner = Py_NewRef(((PyHeapTypeObject *)base)->ht_name);
                const char *base_name = base->tp_name;
                PyErr_Format(PyExc_TypeError,
                             "type '%.100s' is not dynamically allocated but "
                             "its base type '%.100s' is dynamically allocated",
                             type->tp_name, base_name);
                type_mutation_close(&name_owner);
                type_mutation_close(&base_owner);
                return -1;
            }
            assert(!(type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) ||
                   (base->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));
        }
    }
    return 0;
}


// For static types, inherit tp_as_xxx structures from the base class
// if it's NULL.
//
// For heap types, tp_as_xxx structures are not NULL: they are set to the
// PyHeapTypeObject.as_xxx fields by type_new_alloc().
static void
type_ready_inherit_as_structs(PyTypeObject *type, PyTypeObject *base)
{
    if (type->tp_as_async == NULL) {
        type->tp_as_async = base->tp_as_async;
    }
    if (type->tp_as_number == NULL) {
        type->tp_as_number = base->tp_as_number;
    }
    if (type->tp_as_sequence == NULL) {
        type->tp_as_sequence = base->tp_as_sequence;
    }
    if (type->tp_as_mapping == NULL) {
        type->tp_as_mapping = base->tp_as_mapping;
    }
    if (type->tp_as_buffer == NULL) {
        type->tp_as_buffer = base->tp_as_buffer;
    }
}

static void
inherit_patma_flags(PyTypeObject *type, PyTypeObject *base) {
    if ((type->tp_flags & COLLECTION_FLAGS) == 0) {
        type_add_flags(type, base->tp_flags & COLLECTION_FLAGS);
    }
}

static int
type_ready_inherit(PyTypeObject *type)
{
    ASSERT_TYPE_LOCK_HELD();
    PyObject *base_owner = Py_XNewRef(type->tp_base);
    PyObject *mro = Py_XNewRef(lookup_tp_mro(type));
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    if (mro == NULL) {
        PyErr_SetString(PyExc_SystemError, "type has no MRO tuple");
        goto done;
    }
    /* A custom MRO can omit tp_base: the dominant base needs its own pin. */
    PyTypeObject *base = (PyTypeObject *)base_owner;
    if (base != NULL) {
        inherit_special(type, base);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    for (Py_ssize_t i = 1; i < PyTuple_GET_SIZE(mro); i++) {
        PyValue item = PyTuple_GET_VALUE(mro, i);
        if (PyValue_IsObject(item)) {
            PyObject *parent = PyValue_AsPyObjectBorrow(item);
            if (PyType_Check(parent)) {
                if (inherit_slots(type, (PyTypeObject *)parent) < 0 || PyErr_Occurred()) {
                    goto done;
                }
                inherit_patma_flags(type, (PyTypeObject *)parent);
                if (PyErr_Occurred()) {
                    goto done;
                }
            }
        }
    }
    if (base != NULL) {
        type_ready_inherit_as_structs(type, base);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    if (_PyType_IS_GC(type) && (type->tp_flags & Py_TPFLAGS_BASETYPE) &&
            (type->tp_free == NULL || type->tp_free == PyObject_Free)) {
        PyErr_Format(PyExc_TypeError, "type '%.100s' participates in "
                     "gc and is a base type but has inappropriate tp_free slot", type->tp_name);
        goto done;
    }
    result = 0;
done:
    type_mutation_close(&mro);
    type_mutation_close(&base_owner);
    return PyErr_Occurred() ? -1 : result;
}


/* Hack for tp_hash and __hash__.
   If after all that, tp_hash is still NULL, and __hash__ is not in
   tp_dict, set tp_hash to PyObject_HashNotImplemented and
   tp_dict['__hash__'] equal to None.
   This signals that __hash__ is not inherited. */
static int
type_ready_set_hash(PyTypeObject *type)
{
    if (type->tp_hash != NULL) {
        return 0;
    }

    PyObject *dict = lookup_tp_dict(type);
    int r = PyDict_Contains(dict, &_Py_ID(__hash__));
    if (r < 0) {
        return -1;
    }
    if (r > 0) {
        return 0;
    }

    if (PyDict_SetItem(dict, &_Py_ID(__hash__), Py_None) < 0) {
        return -1;
    }
    type->tp_hash = PyObject_HashNotImplemented;
    return 0;
}


/* Link into each base class's list of subclasses */
static int
type_ready_add_subclasses(PyTypeObject *type)
{
    PyObject *bases = Py_XNewRef(lookup_tp_bases(type));
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    if (bases == NULL) {
        PyErr_SetString(PyExc_SystemError, "type has no bases tuple");
        goto done;
    }
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(bases); i++) {
        PyValue item = PyTuple_GET_VALUE(bases, i);
        if (PyValue_IsObject(item)) {
            PyObject *base = PyValue_AsPyObjectBorrow(item);
            if (PyType_Check(base) &&
                    (add_subclass((PyTypeObject *)base, type) < 0 || PyErr_Occurred())) {
                goto done;
            }
        }
    }
    result = 0;
done:
    type_mutation_close(&bases);
    return PyErr_Occurred() ? -1 : result;
}


// Set tp_new and the "__new__" key in the type dictionary.
// Use the Py_TPFLAGS_DISALLOW_INSTANTIATION flag.
static int
type_ready_set_new(PyTypeObject *type, int initial)
{
    PyTypeObject *base = type->tp_base;
    /* The condition below could use some explanation.

       It appears that tp_new is not inherited for static types whose base
       class is 'object'; this seems to be a precaution so that old extension
       types don't suddenly become callable (object.__new__ wouldn't insure the
       invariants that the extension type's own factory function ensures).

       Heap types, of course, are under our control, so they do inherit tp_new;
       static extension types that specify some other built-in type as the
       default also inherit object.__new__. */
    if (type->tp_new == NULL
        && base == &PyBaseObject_Type
        && !(type->tp_flags & Py_TPFLAGS_HEAPTYPE))
    {
        if (initial) {
            type_add_flags(type, Py_TPFLAGS_DISALLOW_INSTANTIATION);
        } else {
            assert(type->tp_flags & Py_TPFLAGS_DISALLOW_INSTANTIATION);
        }
    }

    if (!(type->tp_flags & Py_TPFLAGS_DISALLOW_INSTANTIATION)) {
        if (type->tp_new != NULL) {
            if (initial || base == NULL || type->tp_new != base->tp_new) {
                // If "__new__" key does not exists in the type dictionary,
                // set it to tp_new_wrapper().
                if (add_tp_new_wrapper(type) < 0) {
                    return -1;
                }
            }
        }
        else {
            if (initial) {
                // tp_new is NULL: inherit tp_new from base
                type->tp_new = base->tp_new;
            }
        }
    }
    else {
        // Py_TPFLAGS_DISALLOW_INSTANTIATION sets tp_new to NULL
        if (initial) {
            type->tp_new = NULL;
        }
    }
    return 0;
}

static int
type_ready_managed_dict(PyTypeObject *type)
{
    if (!(type->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
        return 0;
    }
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        PyErr_Format(PyExc_SystemError,
                     "type %s has the Py_TPFLAGS_MANAGED_DICT flag "
                     "but not Py_TPFLAGS_HEAPTYPE flag",
                     type->tp_name);
        return -1;
    }
    PyHeapTypeObject* et = (PyHeapTypeObject*)type;
    if (et->ht_cached_keys == NULL) {
        et->ht_cached_keys = _PyDict_NewKeysForClass(et);
        if (et->ht_cached_keys == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    if (type->tp_itemsize == 0) {
        type_add_flags(type, Py_TPFLAGS_INLINE_VALUES);
    }
    return 0;
}


static int
type_ready_post_checks(PyTypeObject *type)
{
    // bpo-44263: tp_traverse is required if Py_TPFLAGS_HAVE_GC is set.
    // Note: tp_clear is optional.
    if (type->tp_flags & Py_TPFLAGS_HAVE_GC
        && type->tp_traverse == NULL)
    {
        PyErr_Format(PyExc_SystemError,
                     "type %s has the Py_TPFLAGS_HAVE_GC flag "
                     "but has no traverse function",
                     type->tp_name);
        return -1;
    }
    if (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        if (type->tp_dictoffset != -1) {
            PyErr_Format(PyExc_SystemError,
                        "type %s has the Py_TPFLAGS_MANAGED_DICT flag "
                        "but tp_dictoffset is set to incompatible value",
                        type->tp_name);
            return -1;
        }
    }
    else if (type->tp_dictoffset < (Py_ssize_t)sizeof(PyObject)) {
        if (type->tp_dictoffset + type->tp_basicsize <= 0) {
            PyErr_Format(PyExc_SystemError,
                         "type %s has a tp_dictoffset that is too small",
                         type->tp_name);
            return -1;
        }
    }
    return 0;
}


static int
type_ready(PyTypeObject *type, int initial, int add_subclasses)
{
    ASSERT_TYPE_LOCK_HELD();

    _PyObject_ASSERT((PyObject *)type, !is_readying(type));
    start_readying(type);

    if (type_ready_pre_checks(type) < 0) {
        goto error;
    }

    _PyTypeCache_InitType(type);

    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        // A static type is a root for the tracing collector: its dict and
        // the rest are reachable from nothing else on the heap.
        _PyGC_TraceRegisterStaticType(_PyInterpreterState_GET(), type);
    }

#ifdef Py_TRACE_REFS
    /* PyType_Ready is the closest thing we have to a choke point
     * for type objects, so is the best place I can think of to try
     * to get type objects into the doubly-linked list of all objects.
     * Still, not all type objects go through PyType_Ready.
     */
    _Py_AddToAllObjects((PyObject *)type);
#endif

    /* Initialize tp_dict: _PyType_IsReady() tests if tp_dict != NULL */
    if (type_ready_set_dict(type) < 0) {
        goto error;
    }
    if (type_ready_set_base(type) < 0) {
        goto error;
    }
    if (type_ready_set_type(type) < 0) {
        goto error;
    }
    if (type_ready_set_bases(type, initial) < 0) {
        goto error;
    }
    if (type_ready_mro(type, initial) < 0) {
        goto error;
    }
    if (type_ready_set_new(type, initial) < 0) {
        goto error;
    }
    if (type_ready_fill_dict(type) < 0) {
        goto error;
    }
    if (initial) {
        if (type_ready_inherit(type) < 0) {
            goto error;
        }
        if (type_ready_preheader(type) < 0) {
            goto error;
        }
    }
    if (type_ready_set_hash(type) < 0) {
        goto error;
    }
    if (add_subclasses) {
        if (type_ready_add_subclasses(type) < 0) {
            goto error;
        }
    }
    if (initial) {
        if (type_ready_managed_dict(type) < 0) {
            goto error;
        }
        if (type_ready_post_checks(type) < 0) {
            goto error;
        }
    }

    if (add_subclasses) {
        /* All done -- set the ready flag */
        if (initial) {
            type_add_flags(type, Py_TPFLAGS_READY);
        } else {
            assert(type->tp_flags & Py_TPFLAGS_READY);
        }
    }

    stop_readying(type);

    assert(_PyType_CheckConsistency(type));
    return 0;

error:
    stop_readying(type);
    return -1;
}

static int
type_ready_publish(PyTypeObject *type, int fix_slots)
{
    int res;
    BEGIN_TYPE_LOCK();
    res = type_ready(type, 1, 0);
    if (res == 0) {
        assert(!(type->tp_flags & Py_TPFLAGS_READY));
        assert(!is_readying(type));

        if (fix_slots) {
            // Put the proper slots in place and only then publish the type as
            // a subclass of its bases.  Since the type is not reachable by
            // other threads before it is published, the slots can be updated
            // without stopping the world.  This step is skipped for
            // type_from_slots_or_spec().
            fixup_slot_dispatchers(type);
        }

        // Set the ready flag before revealing the type since type_add_flags()
        // may only be used on types that are not yet revealed.
        type_add_flags(type, Py_TPFLAGS_READY);

#if defined(Py_GIL_DISABLED) && defined(Py_DEBUG) && SIZEOF_VOID_P > 4
        // Mark the type as revealed while still holding the type lock.
        // Threads can only find the type through the subclasses of its bases,
        // which is done below with the lock held.  So, they cannot see the
        // type before the flag is set.
        ((PyObject*)type)->ob_flags |= _Py_TYPE_REVEALED_FLAG;
#endif

        res = type_ready_add_subclasses(type);
        if (res == 0) {
            assert(_PyType_CheckConsistency(type));
        }
    }
    END_TYPE_LOCK();
    return res;
}

int
PyType_Ready(PyTypeObject *type)
{
    if (type->tp_flags & Py_TPFLAGS_READY) {
        assert(_PyType_CheckConsistency(type));
        return 0;
    }
    assert(!(type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN));

    /* Historically, all static types were immutable. See bpo-43908 */
    if (!(type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        type_add_flags(type, Py_TPFLAGS_IMMUTABLETYPE);
        /* Static types must be immortal */
        _Py_SetImmortalUntracked((PyObject *)type);
    }

    int res;
    BEGIN_TYPE_LOCK();
    if (!(type->tp_flags & Py_TPFLAGS_READY)) {
        res = type_ready(type, 1, 1);
    } else {
        res = 0;
        assert(_PyType_CheckConsistency(type));
    }
    END_TYPE_LOCK();
    return res;
}


static int
init_static_type(PyInterpreterState *interp, PyTypeObject *self,
                 int isbuiltin, int initial)
{
    assert(_Py_IsImmortal((PyObject *)self));
    assert(!(self->tp_flags & Py_TPFLAGS_HEAPTYPE));
    assert(!(self->tp_flags & Py_TPFLAGS_MANAGED_DICT));
    assert(!(self->tp_flags & Py_TPFLAGS_MANAGED_WEAKREF));

    if (initial) {
        assert((self->tp_flags & Py_TPFLAGS_READY) == 0);

        type_add_flags(self, _Py_TPFLAGS_STATIC_BUILTIN);
        type_add_flags(self, Py_TPFLAGS_IMMUTABLETYPE);

        if (self->tp_version_tag == 0) {
            unsigned int next_version_tag = next_global_version_tag();
            assert(next_version_tag != 0);
            _PyType_SetVersion(self, next_version_tag);
        }
    }
    else {
        assert(!initial);
        assert(self->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN);
        assert(self->tp_version_tag != 0);
    }

    managed_static_type_state_init(interp, self, isbuiltin, initial);

    int res;
    BEGIN_TYPE_LOCK();
    res = type_ready(self, initial, 1);
    END_TYPE_LOCK();
    if (res < 0) {
        _PyStaticType_ClearWeakRefs(interp, self);
        managed_static_type_state_clear(interp, self, isbuiltin, initial);
    }
    else {
        /* Admits the type to this interpreter's lookup cache. */
        managed_static_type_state_get(interp, self)->ready = 1;
    }
    return res;
}

int
_PyStaticType_InitForExtension(PyInterpreterState *interp, PyTypeObject *self)
{
    return init_static_type(interp, self, 0, ((self->tp_flags & Py_TPFLAGS_READY) == 0));
}

int
_PyStaticType_InitBuiltin(PyInterpreterState *interp, PyTypeObject *self)
{
    return init_static_type(interp, self, 1, _Py_IsMainInterpreter(interp));
}


static int
add_subclass(PyTypeObject *base, PyTypeObject *type)
{
    assert(Py_REFCNT(base) > 0 && Py_REFCNT(type) > 0);
    Py_INCREF(base);
    Py_INCREF(type);
    PyObject *key = NULL;
    PyObject *ref = NULL;
    PyObject *subclasses = NULL;
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    key = PyLong_FromVoidPtr((void *)type);
    if (key == NULL || PyErr_Occurred()) {
        goto done;
    }
    ref = PyWeakref_NewRef((PyObject *)type, NULL);
    if (ref == NULL || PyErr_Occurred()) {
        goto done;
    }
    PyObject *current = lookup_tp_subclasses(base);
    if (current == NULL) {
        current = init_tp_subclasses(base);
    }
    if (current == NULL || PyErr_Occurred()) {
        goto done;
    }
    subclasses = Py_NewRef(current);
    result = PyDict_SetItem(subclasses, key, ref);

done:
    type_mutation_close(&ref);
    type_mutation_close(&key);
    if (result == 0 && !PyErr_Occurred() &&
            lookup_tp_subclasses(base) != subclasses) {
        PyErr_SetString(PyExc_RuntimeError,
                        "subclass dictionary changed during publication");
    }
    /* On successful revalidation the installed slot keeps this close
     * nonfinal. An error path may retire a detached map instead. */
    type_mutation_close(&subclasses);
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(type);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_DECREF(base);
    _PyValue_RestoreException(saved);
    return PyErr_Occurred() ? -1 : result;
}

static int
add_all_subclasses(PyTypeObject *type, PyObject *bases)
{
    PyObject *owner = Py_NewRef(bases);
    int result = 0;
    if (PyErr_Occurred()) {
        result = -1;
        goto done;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(owner);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyTuple_GET_VALUE(owner, i);
        assert(PyValue_IsObject(item));
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        if (add_subclass(base, type) < 0 || PyErr_Occurred()) {
            result = -1;
            break;
        }
    }
done:
    type_mutation_close(&owner);
    return PyErr_Occurred() ? -1 : result;
}

static int
get_subclasses_key(PyTypeObject *type, PyTypeObject *base, PyValue *result)
{
    *result = PyValue_NULL;
    if (PyErr_Occurred()) {
        return -1;
    }
    PyObject *key = PyLong_FromVoidPtr((void *)type);
    if (key != NULL) {
        if (!PyErr_Occurred()) {
            PyObject *retired = NULL;
            *result = PyValue_PrepareOwned(key, &retired);
            key = NULL;
            type_mutation_close(&retired);
        }
        type_mutation_close(&key);
        if (!PyErr_Occurred()) {
            return 1;
        }
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(*result);
        *result = PyValue_NULL;
        _PyValue_RestoreException(saved);
        return -1;
    }
    if (!PyErr_ExceptionMatches(PyExc_MemoryError)) {
        return -1;
    }
    PyErr_Clear();
    if (PyErr_Occurred()) {
        return -1;
    }

    /* The target can already be dying. Compare only its address: weakref
     * promotion would reject it, and INCREF would resurrect a zero count. */
    PyObject *subclasses = lookup_tp_subclasses(base);
    if (subclasses == NULL) {
        return 0;
    }
    Py_ssize_t pos = 0;
    PyValue candidate, ref;
    int found;
    while ((found = _PyDict_NextValue(subclasses, &pos, &candidate, &ref, NULL)) > 0) {
        assert(PyValue_IsObject(ref));
        PyWeakReference *weakref = (PyWeakReference *)PyValue_AsPyObjectBorrow(ref);
        assert(PyWeakref_Check((PyObject *)weakref));
        int match = weakref->wr_object == (PyObject *)type;
        PyValue_CLOSE(ref);
        if (match) {
            *result = candidate;
            return 1;
        }
        /* The map still owns this exact integer key: this is nonfinal. */
        PyValue_CLOSE(candidate);
    }
    return found < 0 || PyErr_Occurred() ? -1 : 0;
}

static void
remove_subclass(PyTypeObject *base, PyTypeObject *type)
{
    assert(Py_REFCNT(base) > 0);
    Py_INCREF(base);
    PyObject *saved = PyErr_GetRaisedException();
    PyObject *primary = saved;
    PyObject *subclasses = NULL;
    PyValue key = PyValue_NULL;
    PyValue ref = PyValue_NULL;
    int found = get_subclasses_key(type, base, &key);
    if (found <= 0 || PyErr_Occurred()) {
        goto done;
    }
    subclasses = Py_XNewRef(lookup_tp_subclasses(base));
    if (subclasses == NULL) {
        goto done;
    }
    assert(PyDict_CheckExact(subclasses));
    found = PyDict_GetValue(subclasses, key, &ref);
    if (found > 0 && !PyErr_Occurred()) {
        /* Keep ref alive across this immediate delete; internal identity
         * integer lookup/hash does not call out between the operations. */
        if (PyDict_DelValue(subclasses, key) < 0 || PyErr_Occurred()) {
            goto done;
        }
    }
    if (!PyErr_Occurred() && PyDict_GET_SIZE(subclasses) == 0 &&
            lookup_tp_subclasses(base) == subclasses) {
        /* The local owner makes detaching the slot nonfinal. */
        clear_tp_subclasses(base);
    }

done:
    saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(ref);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(key);
    _PyValue_RestoreException(saved);
    type_mutation_close(&subclasses);
    saved = PyErr_GetRaisedException();
    Py_DECREF(base);
    _PyValue_RestoreException(saved);
    _PyValue_RestoreException(primary);
}

static void
remove_all_subclasses(PyTypeObject *type, PyObject *bases)
{
    assert(bases != NULL);
    PyObject *owner = Py_NewRef(bases);
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(owner); i++) {
        PyValue item = PyTuple_GET_VALUE(owner, i);
        if (PyValue_IsObject(item)) {
            PyObject *base = PyValue_AsPyObjectBorrow(item);
            if (PyType_Check(base)) {
                remove_subclass((PyTypeObject *)base, type);
            }
        }
    }
    type_mutation_close(&owner);
}

static int
check_num_args(PyObject *ob, int n)
{
    if (!PyTuple_CheckExact(ob)) {
        PyErr_SetString(PyExc_SystemError,
            "PyArg_UnpackTuple() argument list is not a tuple");
        return 0;
    }
    if (n == PyTuple_GET_SIZE(ob))
        return 1;
    PyErr_Format(
        PyExc_TypeError,
        "expected %d argument%s, got %zd", n, n == 1 ? "" : "s", PyTuple_GET_SIZE(ob));
    return 0;
}

static Py_ssize_t
check_pow_args(PyObject *ob)
{
    // Returns the argument count on success or `-1` on error.
    int min = 1;
    int max = 2;
    if (!PyTuple_CheckExact(ob)) {
        PyErr_SetString(PyExc_SystemError,
            "PyArg_UnpackTuple() argument list is not a tuple");
        return -1;
    }
    Py_ssize_t size = PyTuple_GET_SIZE(ob);
    if (size >= min && size <= max) {
        return size;
    }
    PyErr_Format(
        PyExc_TypeError,
        "expected %d or %d arguments, got %zd", min, max, PyTuple_GET_SIZE(ob));
    return -1;
}

/* Generic wrappers for overloadable 'operators' such as __getitem__ */

/* There's a wrapper *function* for each distinct function typedef used
   for type object slots (e.g. binaryfunc, ternaryfunc, etc.).  There's a
   wrapper *table* for each distinct operation (e.g. __len__, __add__).
   Most tables have only one entry; the tables for binary operators have two
   entries, one regular and one with reversed arguments. */

static PyObject *
wrap_lenfunc(PyObject *self, PyObject *args, void *wrapped)
{
    lenfunc func = (lenfunc)wrapped;
    Py_ssize_t res;

    if (!check_num_args(args, 0))
        return NULL;
    res = (*func)(self);
    if (res == -1 && PyErr_Occurred())
        return NULL;
    return PyLong_FromSsize_t(res);
}

static PyObject *
wrap_inquirypred(PyObject *self, PyObject *args, void *wrapped)
{
    inquiry func = (inquiry)wrapped;
    int res;

    if (!check_num_args(args, 0))
        return NULL;
    res = (*func)(self);
    if (res == -1 && PyErr_Occurred())
        return NULL;
    return PyBool_FromLong((long)res);
}

static PyObject *
wrap_binaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
    binaryfunc func = (binaryfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *other = NULL;
    PyObject *result = NULL;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    other = PyTuple_GetItemRef(tuple, 0);
    if (other == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = (*func)(receiver, other);

done:
    type_mutation_close(&other);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_binaryfunc_l(PyObject *self, PyObject *args, void *wrapped)
{
    binaryfunc func = (binaryfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *other = NULL;
    PyObject *result = NULL;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    other = PyTuple_GetItemRef(tuple, 0);
    if (other == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = (*func)(receiver, other);

done:
    type_mutation_close(&other);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_binaryfunc_r(PyObject *self, PyObject *args, void *wrapped)
{
    binaryfunc func = (binaryfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *other = NULL;
    PyObject *result = NULL;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    other = PyTuple_GetItemRef(tuple, 0);
    if (other == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = (*func)(other, receiver);

done:
    type_mutation_close(&other);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_ternaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
    ternaryfunc func = (ternaryfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *other = NULL;
    PyObject *third_owner = NULL;
    PyObject *third = Py_None;
    PyObject *result = NULL;
    Py_ssize_t size;

    /* Note: This wrapper only works for __pow__() */

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    size = check_pow_args(tuple);
    if (size == -1 || PyErr_Occurred()) {
        goto done;
    }
    other = PyTuple_GetItemRef(tuple, 0);
    if (other == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (size == 2) {
        third_owner = PyTuple_GetItemRef(tuple, 1);
        if (third_owner == NULL || PyErr_Occurred()) {
            goto done;
        }
        third = third_owner;
    }
    result = (*func)(receiver, other, third);

done:
    type_mutation_close(&third_owner);
    type_mutation_close(&other);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_ternaryfunc_r(PyObject *self, PyObject *args, void *wrapped)
{
    ternaryfunc func = (ternaryfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *other = NULL;
    PyObject *third_owner = NULL;
    PyObject *third = Py_None;
    PyObject *result = NULL;
    Py_ssize_t size;

    /* Note: This wrapper only works for __rpow__() */

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    size = check_pow_args(tuple);
    if (size == -1 || PyErr_Occurred()) {
        goto done;
    }
    other = PyTuple_GetItemRef(tuple, 0);
    if (other == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (size == 2) {
        third_owner = PyTuple_GetItemRef(tuple, 1);
        if (third_owner == NULL || PyErr_Occurred()) {
            goto done;
        }
        third = third_owner;
    }
    result = (*func)(other, receiver, third);

done:
    type_mutation_close(&third_owner);
    type_mutation_close(&other);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_unaryfunc(PyObject *self, PyObject *args, void *wrapped)
{
    unaryfunc func = (unaryfunc)wrapped;

    if (!check_num_args(args, 0))
        return NULL;
    return (*func)(self);
}

static PyObject *
wrap_indexargfunc(PyObject *self, PyObject *args, void *wrapped)
{
    ssizeargfunc func = (ssizeargfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *operand = NULL;
    PyObject *result = NULL;
    Py_ssize_t i;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    operand = PyTuple_GetItemRef(tuple, 0);
    if (operand == NULL || PyErr_Occurred()) {
        goto done;
    }
    i = PyNumber_AsSsize_t(operand, PyExc_OverflowError);
    if (PyErr_Occurred()) {
        goto done;
    }
    result = (*func)(receiver, i);

done:
    type_mutation_close(&operand);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static Py_ssize_t
getindex(PyObject *self, PyObject *arg)
{
    Py_ssize_t i;

    i = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
    if (i == -1 && PyErr_Occurred())
        return -1;
    if (i < 0) {
        PySequenceMethods *sq = Py_TYPE(self)->tp_as_sequence;
        if (sq && sq->sq_length) {
            Py_ssize_t n = (*sq->sq_length)(self);
            if (n < 0) {
                assert(PyErr_Occurred());
                return -1;
            }
            i += n;
        }
    }
    return i;
}

static PyObject *
wrap_sq_item(PyObject *self, PyObject *args, void *wrapped)
{
    ssizeargfunc func = (ssizeargfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *operand = NULL;
    PyObject *result = NULL;
    Py_ssize_t i;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (PyTuple_GET_SIZE(tuple) == 1) {
        operand = PyTuple_GetItemRef(tuple, 0);
        if (operand == NULL || PyErr_Occurred()) {
            goto done;
        }
        i = getindex(receiver, operand);
        if (PyErr_Occurred()) {
            goto done;
        }
        result = (*func)(receiver, i);
        goto done;
    }
    check_num_args(tuple, 1);
    assert(PyErr_Occurred());

done:
    type_mutation_close(&operand);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_sq_setitem(PyObject *self, PyObject *args, void *wrapped)
{
    ssizeobjargproc func = (ssizeobjargproc)wrapped;
    Py_ssize_t i;
    int res;
    PyObject *arg, *value;

    if (!PyArg_UnpackTuple(args, "__setitem__", 2, 2, &arg, &value))
        return NULL;
    i = getindex(self, arg);
    if (i == -1 && PyErr_Occurred())
        return NULL;
    res = (*func)(self, i, value);
    if (res == -1 && PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
wrap_sq_delitem(PyObject *self, PyObject *args, void *wrapped)
{
    ssizeobjargproc func = (ssizeobjargproc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *operand = NULL;
    PyObject *result = NULL;
    Py_ssize_t i;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    operand = PyTuple_GetItemRef(tuple, 0);
    if (operand == NULL || PyErr_Occurred()) {
        goto done;
    }
    i = getindex(receiver, operand);
    if (PyErr_Occurred()) {
        goto done;
    }
    (void)(*func)(receiver, i, NULL);
    if (PyErr_Occurred()) {
        goto done;
    }
    result = Py_NewRef(Py_None);

done:
    type_mutation_close(&operand);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

/* XXX objobjproc is a misnomer; should be objargpred */
static PyObject *
wrap_objobjproc(PyObject *self, PyObject *args, void *wrapped)
{
    objobjproc func = (objobjproc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *operand = NULL;
    PyObject *result = NULL;
    int res;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    operand = PyTuple_GetItemRef(tuple, 0);
    if (operand == NULL || PyErr_Occurred()) {
        goto done;
    }
    res = (*func)(receiver, operand);
    if (PyErr_Occurred()) {
        goto done;
    }
    result = PyBool_FromLong((long)res);

done:
    type_mutation_close(&operand);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_objobjargproc(PyObject *self, PyObject *args, void *wrapped)
{
    objobjargproc func = (objobjargproc)wrapped;
    int res;
    PyObject *key, *value;

    if (!PyArg_UnpackTuple(args, "__setitem__", 2, 2, &key, &value))
        return NULL;
    res = (*func)(self, key, value);
    if (res == -1 && PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
wrap_delitem(PyObject *self, PyObject *args, void *wrapped)
{
    objobjargproc func = (objobjargproc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *operand = NULL;
    PyObject *result = NULL;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    operand = PyTuple_GetItemRef(tuple, 0);
    if (operand == NULL || PyErr_Occurred()) {
        goto done;
    }
    (void)(*func)(receiver, operand, NULL);
    if (PyErr_Occurred()) {
        goto done;
    }
    result = Py_NewRef(Py_None);

done:
    type_mutation_close(&operand);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

/* Helper to check for object.__setattr__ or __delattr__ applied to a type.
   This is called the Carlo Verre hack after its discoverer.  See
   https://mail.python.org/pipermail/python-dev/2003-April/034535.html
   */
static int
hackcheck_unlocked(PyObject *self, setattrofunc func, const char *what)
{
    PyTypeObject *type = Py_TYPE(self);

    ASSERT_TYPE_LOCK_HELD();

    PyObject *mro = lookup_tp_mro(type);
    if (!mro) {
        /* Probably ok not to check the call in this case. */
        return 1;
    }
    assert(PyTuple_Check(mro));

    /* Find the (base) type that defined the type's slot function. */
    PyTypeObject *defining_type = type;
    Py_ssize_t i;
    for (i = PyTuple_GET_SIZE(mro) - 1; i >= 0; i--) {
        PyValue value = PyTuple_GET_VALUE(mro, i);
        assert(PyValue_IsObject(value));
        PyTypeObject *base = _PyType_CAST(PyValue_AsPyObjectBorrow(value));
        if (base->tp_setattro == slot_tp_setattro) {
            /* Ignore Python classes:
               they never define their own C-level setattro. */
        }
        else if (base->tp_setattro == type->tp_setattro) {
            defining_type = base;
            break;
        }
    }

    /* Reject calls that jump over intermediate C-level overrides. */
    for (PyTypeObject *base = defining_type; base; base = base->tp_base) {
        if (base->tp_setattro == func) {
            /* 'func' is the right slot function to call. */
            break;
        }
        else if (base->tp_setattro != slot_tp_setattro) {
            /* 'base' is not a Python class and overrides 'func'.
               Its tp_setattro should be called instead. */
            PyObject *type_owner = Py_NewRef((PyObject *)type);
            PyObject *name_owner = NULL;
            if (_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
                name_owner = Py_NewRef(((PyHeapTypeObject *)type)->ht_name);
            }
            const char *type_name = type->tp_name;
            PyErr_Format(PyExc_TypeError,
                         "can't apply this %s to %s object",
                         what, type_name);
            type_mutation_close(&name_owner);
            type_mutation_close(&type_owner);
            return 0;
        }
    }
    return 1;
}

static int
hackcheck(PyObject *self, setattrofunc func, const char *what)
{
    if (!PyType_Check(self)) {
        return 1;
    }

    int res;
    BEGIN_TYPE_LOCK();
    res = hackcheck_unlocked(self, func, what);
    END_TYPE_LOCK();
    return res;
}

static PyObject *
wrap_setattr(PyObject *self, PyObject *args, void *wrapped)
{
    setattrofunc func = (setattrofunc)wrapped;
    int res;
    PyObject *name, *value;

    if (!PyArg_UnpackTuple(args, "__setattr__", 2, 2, &name, &value))
        return NULL;
    if (!hackcheck(self, func, "__setattr__"))
        return NULL;
    res = (*func)(self, name, value);
    if (res < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
wrap_delattr(PyObject *self, PyObject *args, void *wrapped)
{
    setattrofunc func = (setattrofunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *name = NULL;
    PyObject *result = NULL;
    int res;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    name = PyTuple_GetItemRef(tuple, 0);
    if (name == NULL || PyErr_Occurred()) {
        goto done;
    }
    res = hackcheck(receiver, func, "__delattr__");
    if (!res || PyErr_Occurred()) {
        goto done;
    }
    res = (*func)(receiver, name, NULL);
    if (res < 0 || PyErr_Occurred()) {
        goto done;
    }
    result = Py_NewRef(Py_None);

done:
    type_mutation_close(&name);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_hashfunc(PyObject *self, PyObject *args, void *wrapped)
{
    hashfunc func = (hashfunc)wrapped;
    Py_hash_t res;

    if (!check_num_args(args, 0))
        return NULL;
    res = (*func)(self);
    if (res == -1 && PyErr_Occurred())
        return NULL;
    return PyLong_FromSsize_t(res);
}

static PyObject *
wrap_call(PyObject *self, PyObject *args, void *wrapped, PyObject *kwds)
{
    ternaryfunc func = (ternaryfunc)wrapped;

    return (*func)(self, args, kwds);
}

static PyObject *
wrap_del(PyObject *self, PyObject *args, void *wrapped)
{
    destructor func = (destructor)wrapped;

    if (!check_num_args(args, 0))
        return NULL;

    (*func)(self);
    Py_RETURN_NONE;
}

static PyObject *
wrap_richcmpfunc(PyObject *self, PyObject *args, void *wrapped, int op)
{
    richcmpfunc func = (richcmpfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *other = NULL;
    PyObject *result = NULL;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    other = PyTuple_GetItemRef(tuple, 0);
    if (other == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = (*func)(receiver, other, op);

done:
    type_mutation_close(&other);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

#undef RICHCMP_WRAPPER
#define RICHCMP_WRAPPER(NAME, OP) \
static PyObject * \
richcmp_##NAME(PyObject *self, PyObject *args, void *wrapped) \
{ \
    return wrap_richcmpfunc(self, args, wrapped, OP); \
}

RICHCMP_WRAPPER(lt, Py_LT)
RICHCMP_WRAPPER(le, Py_LE)
RICHCMP_WRAPPER(eq, Py_EQ)
RICHCMP_WRAPPER(ne, Py_NE)
RICHCMP_WRAPPER(gt, Py_GT)
RICHCMP_WRAPPER(ge, Py_GE)

static PyObject *
wrap_next(PyObject *self, PyObject *args, void *wrapped)
{
    unaryfunc func = (unaryfunc)wrapped;
    PyObject *res;

    if (!check_num_args(args, 0))
        return NULL;
    res = (*func)(self);
    if (res == NULL && !PyErr_Occurred())
        PyErr_SetNone(PyExc_StopIteration);
    return res;
}

static PyObject *
wrap_descr_get(PyObject *self, PyObject *args, void *wrapped)
{
    descrgetfunc func = (descrgetfunc)wrapped;
    PyObject *obj;
    PyObject *type = NULL;

    if (!PyArg_UnpackTuple(args, "__get__", 1, 2, &obj, &type))
        return NULL;
    if (obj == Py_None)
        obj = NULL;
    if (type == Py_None)
        type = NULL;
    if (type == NULL && obj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "__get__(None, None) is invalid");
        return NULL;
    }
    return (*func)(self, obj, type);
}

static PyObject *
wrap_descr_set(PyObject *self, PyObject *args, void *wrapped)
{
    descrsetfunc func = (descrsetfunc)wrapped;
    PyObject *obj, *value;
    int ret;

    if (!PyArg_UnpackTuple(args, "__set__", 2, 2, &obj, &value))
        return NULL;
    ret = (*func)(self, obj, value);
    if (ret < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
wrap_descr_delete(PyObject *self, PyObject *args, void *wrapped)
{
    descrsetfunc func = (descrsetfunc)wrapped;
    PyObject *receiver = NULL;
    PyObject *tuple = NULL;
    PyObject *operand = NULL;
    PyObject *result = NULL;
    int ret;

    if (PyErr_Occurred()) {
        goto done;
    }
    receiver = Py_NewRef(self);
    tuple = Py_NewRef(args);
    if (!check_num_args(tuple, 1) || PyErr_Occurred()) {
        goto done;
    }
    operand = PyTuple_GetItemRef(tuple, 0);
    if (operand == NULL || PyErr_Occurred()) {
        goto done;
    }
    ret = (*func)(receiver, operand, NULL);
    if (ret < 0 || PyErr_Occurred()) {
        goto done;
    }
    result = Py_NewRef(Py_None);

done:
    type_mutation_close(&operand);
    type_mutation_close(&tuple);
    type_mutation_close(&receiver);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static PyObject *
wrap_buffer(PyObject *self, PyObject *args, void *wrapped)
{
    PyObject *arg = NULL;

    if (!PyArg_UnpackTuple(args, "__buffer__", 1, 1, &arg)) {
        return NULL;
    }
    Py_ssize_t flags = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
    if (flags == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (flags > INT_MAX || flags < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError,
                        "buffer flags out of range");
        return NULL;
    }

    return _PyMemoryView_FromBufferProc(self, (int)flags,
                                        (getbufferproc)wrapped);
}

static PyObject *
wrap_releasebuffer(PyObject *self, PyObject *args, void *wrapped)
{
    PyObject *arg = NULL;
    if (!PyArg_UnpackTuple(args, "__release_buffer__", 1, 1, &arg)) {
        return NULL;
    }
    if (!PyMemoryView_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected a memoryview object");
        return NULL;
    }
    PyMemoryViewObject *mview = (PyMemoryViewObject *)arg;
    if (mview->view.obj == NULL) {
        // Already released, ignore
        Py_RETURN_NONE;
    }
    if (mview->view.obj != self) {
        PyErr_SetString(PyExc_ValueError,
                        "memoryview's buffer is not this object");
        return NULL;
    }
    if (mview->flags & _Py_MEMORYVIEW_RELEASED) {
        PyErr_SetString(PyExc_ValueError,
                        "memoryview's buffer has already been released");
        return NULL;
    }
    PyObject *res = PyObject_CallMethodNoArgs((PyObject *)mview, &_Py_ID(release));
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    Py_RETURN_NONE;
}

static PyObject *
wrap_init(PyObject *self, PyObject *args, void *wrapped, PyObject *kwds)
{
    initproc func = (initproc)wrapped;

    if (func(self, args, kwds) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
tp_new_wrapper(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyTypeObject *staticbase;
    PyObject *arg0, *res;

    if (self == NULL || !PyType_Check(self)) {
        PyErr_Format(PyExc_SystemError,
                     "__new__() called with non-type 'self'");
        return NULL;
    }
    PyTypeObject *type = (PyTypeObject *)self;

    if (nargs < 1) {
        PyErr_Format(PyExc_TypeError,
                     "%s.__new__(): not enough arguments",
                     type->tp_name);
        return NULL;
    }
    arg0 = args[0];
    if (!PyType_Check(arg0)) {
        PyErr_Format(PyExc_TypeError,
                     "%s.__new__(X): X is not a type object (%s)",
                     type->tp_name,
                     Py_TYPE(arg0)->tp_name);
        return NULL;
    }
    PyTypeObject *subtype = (PyTypeObject *)arg0;

    if (!PyType_IsSubtype(subtype, type)) {
        PyErr_Format(PyExc_TypeError,
                     "%s.__new__(%s): %s is not a subtype of %s",
                     type->tp_name,
                     subtype->tp_name,
                     subtype->tp_name,
                     type->tp_name);
        return NULL;
    }

    /* Check that the use doesn't do something silly and unsafe like
       object.__new__(dict).  To do this, we check that the
       most derived base that's not a heap type is this type. */
    staticbase = subtype;
    while (staticbase && (staticbase->tp_new == slot_tp_new))
        staticbase = staticbase->tp_base;
    /* If staticbase is NULL now, it is a really weird type.
       In the spirit of backwards compatibility (?), just shut up. */
    if (staticbase && staticbase->tp_new != type->tp_new) {
        if (staticbase->tp_new == NULL) {
            PyErr_Format(PyExc_TypeError,
                         "cannot create '%s' instances", subtype->tp_name);
            return NULL;
        }
        PyErr_Format(PyExc_TypeError,
                     "%s.__new__(%s) is not safe, use %s.__new__()",
                     type->tp_name,
                     subtype->tp_name,
                     staticbase->tp_name);
        return NULL;
    }

    PyObject *args_tuple = PyTuple_FromArray(args + 1, nargs - 1);
    if (args_tuple == NULL) {
        return NULL;
    }
    PyObject *kwds = NULL;
    if (kwnames != NULL) {
        kwds = _PyStack_AsDict(args + nargs, kwnames);
        if (kwds == NULL) {
            Py_DECREF(args_tuple);
            return NULL;
        }
    }
    res = type->tp_new(subtype, args_tuple, kwds);
    Py_DECREF(args_tuple);
    Py_XDECREF(kwds);
    return res;
}

static struct PyMethodDef tp_new_methoddef[] = {
    {"__new__", _PyCFunction_CAST(tp_new_wrapper), METH_FASTCALL|METH_KEYWORDS,
     PyDoc_STR("__new__($type, *args, **kwargs)\n--\n\n"
               "Create and return a new object.  "
               "See help(type) for accurate signature.")},
    {0}
};

static int
add_tp_new_wrapper(PyTypeObject *type)
{
    PyObject *dict = lookup_tp_dict(type);
    int r = PyDict_Contains(dict, &_Py_ID(__new__));
    if (r > 0) {
        return 0;
    }
    if (r < 0) {
        return -1;
    }

    PyObject *func = PyCFunction_NewEx(tp_new_methoddef, (PyObject *)type, NULL);
    if (func == NULL) {
        return -1;
    }
    _PyObject_SetDeferredRefcount(func);
    r = PyDict_SetItem(dict, &_Py_ID(__new__), func);
    Py_DECREF(func);
    return r;
}

/* Slot wrappers that call the corresponding __foo__ slot.  See comments
   below at override_slots() for more explanation. */

#define SLOT0(FUNCNAME, DUNDER) \
static PyObject * \
FUNCNAME(PyObject *self) \
{ \
    PyObject* stack[1] = {self}; \
    return vectorcall_method(&_Py_ID(DUNDER), stack, 1); \
}

#define SLOT1(FUNCNAME, DUNDER, ARG1TYPE) \
static PyObject * \
FUNCNAME(PyObject *self, ARG1TYPE arg1) \
{ \
    PyObject* stack[2] = {self, arg1}; \
    return vectorcall_method(&_Py_ID(DUNDER), stack, 2); \
}

/* Boolean helper for SLOT1BINFULL().
   right.__class__ is a nontrivial subclass of left.__class__. */
static int
method_is_overloaded(PyObject *left, PyObject *right, PyObject *name)
{
    PyObject *a, *b;
    int ok;

    if (PyObject_GetOptionalAttr((PyObject *)(Py_TYPE(right)), name, &b) < 0) {
        return -1;
    }
    if (b == NULL) {
        /* If right doesn't have it, it's not overloaded */
        return 0;
    }

    if (PyObject_GetOptionalAttr((PyObject *)(Py_TYPE(left)), name, &a) < 0) {
        Py_DECREF(b);
        return -1;
    }
    if (a == NULL) {
        Py_DECREF(b);
        /* If right has it but left doesn't, it's overloaded */
        return 1;
    }

    ok = PyObject_RichCompareBool(a, b, Py_NE);
    Py_DECREF(a);
    Py_DECREF(b);
    return ok;
}


#define SLOT1BINFULL(FUNCNAME, TESTFUNC, SLOTNAME, DUNDER, RDUNDER) \
static PyObject * \
FUNCNAME(PyObject *self, PyObject *other) \
{ \
    PyObject* stack[2]; \
    PyThreadState *tstate = _PyThreadState_GET(); \
    int do_other = !Py_IS_TYPE(self, Py_TYPE(other)) && \
        Py_TYPE(other)->tp_as_number != NULL && \
        Py_TYPE(other)->tp_as_number->SLOTNAME == TESTFUNC; \
    if (Py_TYPE(self)->tp_as_number != NULL && \
        Py_TYPE(self)->tp_as_number->SLOTNAME == TESTFUNC) { \
        PyObject *r; \
        if (do_other && PyType_IsSubtype(Py_TYPE(other), Py_TYPE(self))) { \
            int ok = method_is_overloaded(self, other, &_Py_ID(RDUNDER)); \
            if (ok < 0) { \
                return NULL; \
            } \
            if (ok) { \
                stack[0] = other; \
                stack[1] = self; \
                r = vectorcall_maybe(tstate, &_Py_ID(RDUNDER), stack, 2); \
                if (r != Py_NotImplemented) \
                    return r; \
                Py_DECREF(r); \
                do_other = 0; \
            } \
        } \
        stack[0] = self; \
        stack[1] = other; \
        r = vectorcall_maybe(tstate, &_Py_ID(DUNDER), stack, 2); \
        if (r != Py_NotImplemented || \
            Py_IS_TYPE(other, Py_TYPE(self))) \
            return r; \
        Py_DECREF(r); \
    } \
    if (do_other) { \
        stack[0] = other; \
        stack[1] = self; \
        return vectorcall_maybe(tstate, &_Py_ID(RDUNDER), stack, 2); \
    } \
    Py_RETURN_NOTIMPLEMENTED; \
}

#define SLOT1BIN(FUNCNAME, SLOTNAME, DUNDER, RDUNDER) \
    SLOT1BINFULL(FUNCNAME, FUNCNAME, SLOTNAME, DUNDER, RDUNDER)

static Py_ssize_t
slot_sq_length(PyObject *self)
{
    PyObject* stack[1] = {self};
    PyObject *res = vectorcall_method(&_Py_ID(__len__), stack, 1);
    Py_ssize_t len;

    if (res == NULL)
        return -1;

    Py_SETREF(res, _PyNumber_Index(res));
    if (res == NULL)
        return -1;

    assert(PyLong_Check(res));
    if (_PyLong_IsNegative((PyLongObject *)res)) {
        Py_DECREF(res);
        PyErr_SetString(PyExc_ValueError,
                        "__len__() should return >= 0");
        return -1;
    }

    len = PyNumber_AsSsize_t(res, PyExc_OverflowError);
    assert(len >= 0 || PyErr_ExceptionMatches(PyExc_OverflowError));
    Py_DECREF(res);
    return len;
}

static PyObject *
slot_sq_item(PyObject *self, Py_ssize_t i)
{
    PyObject *ival = PyLong_FromSsize_t(i);
    if (ival == NULL) {
        return NULL;
    }
    PyObject *stack[2] = {self, ival};
    PyObject *retval = vectorcall_method(&_Py_ID(__getitem__), stack, 2);
    Py_DECREF(ival);
    return retval;
}

static int
slot_sq_ass_item(PyObject *self, Py_ssize_t index, PyObject *value)
{
    PyObject *stack[3];
    PyObject *res;
    PyObject *index_obj;

    index_obj = PyLong_FromSsize_t(index);
    if (index_obj == NULL) {
        return -1;
    }

    stack[0] = self;
    stack[1] = index_obj;
    if (value == NULL) {
        res = vectorcall_method(&_Py_ID(__delitem__), stack, 2);
    }
    else {
        stack[2] = value;
        res = vectorcall_method(&_Py_ID(__setitem__), stack, 3);
    }
    Py_DECREF(index_obj);

    if (res == NULL) {
        return -1;
    }
    Py_DECREF(res);
    return 0;
}

static int
slot_sq_contains(PyObject *self, PyObject *value)
{
    int attr_is_none = 0;
    PyObject *res = maybe_call_special_one_arg(self, &_Py_ID(__contains__), value,
                                               &attr_is_none);
    if (attr_is_none) {
        PyErr_Format(PyExc_TypeError,
            "'%.200s' object is not a container",
            Py_TYPE(self)->tp_name);
        return -1;
    }
    else if (res == NULL && PyErr_Occurred()) {
        return -1;
    }
    else if (res == NULL) {
        return (int)_PySequence_IterSearch(self, value, PY_ITERSEARCH_CONTAINS);
    }
    int result = PyObject_IsTrue(res);
    Py_DECREF(res);
    return result;
}

#define slot_mp_length slot_sq_length

SLOT1(slot_mp_subscript, __getitem__, PyObject *)

static int
slot_mp_ass_subscript(PyObject *self, PyObject *key, PyObject *value)
{
    PyObject *stack[3];
    PyObject *res;

    stack[0] = self;
    stack[1] = key;
    if (value == NULL) {
        res = vectorcall_method(&_Py_ID(__delitem__), stack, 2);
    }
    else {
        stack[2] = value;
        res = vectorcall_method(&_Py_ID(__setitem__), stack, 3);
    }

    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

SLOT1BIN(slot_nb_add, nb_add, __add__, __radd__)
SLOT1BIN(slot_nb_subtract, nb_subtract, __sub__, __rsub__)
SLOT1BIN(slot_nb_multiply, nb_multiply, __mul__, __rmul__)
SLOT1BIN(slot_nb_matrix_multiply, nb_matrix_multiply, __matmul__, __rmatmul__)
SLOT1BIN(slot_nb_remainder, nb_remainder, __mod__, __rmod__)
SLOT1BIN(slot_nb_divmod, nb_divmod, __divmod__, __rdivmod__)

static PyObject *slot_nb_power(PyObject *, PyObject *, PyObject *);

SLOT1BINFULL(slot_nb_power_binary, slot_nb_power, nb_power, __pow__, __rpow__)

static PyObject *
slot_nb_power(PyObject *self, PyObject *other, PyObject *modulus)
{
    if (modulus == Py_None)
        return slot_nb_power_binary(self, other);

    /* The following code is a copy of SLOT1BINFULL, but for three arguments. */
    PyObject* stack[3];
    PyThreadState *tstate = _PyThreadState_GET();
    int do_other = !Py_IS_TYPE(self, Py_TYPE(other)) &&
        Py_TYPE(other)->tp_as_number != NULL &&
        Py_TYPE(other)->tp_as_number->nb_power == slot_nb_power;
    if (Py_TYPE(self)->tp_as_number != NULL &&
        Py_TYPE(self)->tp_as_number->nb_power == slot_nb_power) {
        PyObject *r;
        if (do_other && PyType_IsSubtype(Py_TYPE(other), Py_TYPE(self))) {
            int ok = method_is_overloaded(self, other, &_Py_ID(__rpow__));
            if (ok < 0) {
                return NULL;
            }
            if (ok) {
                stack[0] = other;
                stack[1] = self;
                stack[2] = modulus;
                r = vectorcall_maybe(tstate, &_Py_ID(__rpow__), stack, 3);
                if (r != Py_NotImplemented)
                    return r;
                Py_DECREF(r);
                do_other = 0;
            }
        }
        stack[0] = self;
        stack[1] = other;
        stack[2] = modulus;
        r = vectorcall_maybe(tstate, &_Py_ID(__pow__), stack, 3);
        if (r != Py_NotImplemented ||
            Py_IS_TYPE(other, Py_TYPE(self)))
            return r;
        Py_DECREF(r);
    }
    if (do_other) {
        stack[0] = other;
        stack[1] = self;
        stack[2] = modulus;
        return vectorcall_maybe(tstate, &_Py_ID(__rpow__), stack, 3);
    }
    Py_RETURN_NOTIMPLEMENTED;
}

SLOT0(slot_nb_negative, __neg__)
SLOT0(slot_nb_positive, __pos__)
SLOT0(slot_nb_absolute, __abs__)

static int
slot_nb_bool(PyObject *self)
{
    int using_len = 0;
    int attr_is_none = 0;
    PyObject *value = maybe_call_special_no_args(self, &_Py_ID(__bool__),
                                                 &attr_is_none);
    if (attr_is_none) {
        PyErr_Format(PyExc_TypeError,
                     "'%.200s' cannot be interpreted as a boolean",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    else if (value == NULL && !PyErr_Occurred()) {
        value = _PyObject_MaybeCallSpecialNoArgs(self, &_Py_ID(__len__));
        if (value == NULL && !PyErr_Occurred()) {
            return 1;
        }
        using_len = 1;
    }

    if (value == NULL) {
        return -1;
    }

    int result;
    if (using_len) {
        /* bool type enforced by slot_nb_len */
        result = PyObject_IsTrue(value);
    }
    else if (PyBool_Check(value)) {
        result = PyObject_IsTrue(value);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "%T.__bool__() must return a bool, not %T",
                     self, value);
        result = -1;
    }
    Py_DECREF(value);
    return result;
}


static PyObject *
slot_nb_index(PyObject *self)
{
    PyObject *stack[1] = {self};
    return vectorcall_method(&_Py_ID(__index__), stack, 1);
}


SLOT0(slot_nb_invert, __invert__)
SLOT1BIN(slot_nb_lshift, nb_lshift, __lshift__, __rlshift__)
SLOT1BIN(slot_nb_rshift, nb_rshift, __rshift__, __rrshift__)
SLOT1BIN(slot_nb_and, nb_and, __and__, __rand__)
SLOT1BIN(slot_nb_xor, nb_xor, __xor__, __rxor__)
SLOT1BIN(slot_nb_or, nb_or, __or__, __ror__)

SLOT0(slot_nb_int, __int__)
SLOT0(slot_nb_float, __float__)
SLOT1(slot_nb_inplace_add, __iadd__, PyObject *)
SLOT1(slot_nb_inplace_subtract, __isub__, PyObject *)
SLOT1(slot_nb_inplace_multiply, __imul__, PyObject *)
SLOT1(slot_nb_inplace_matrix_multiply, __imatmul__, PyObject *)
SLOT1(slot_nb_inplace_remainder, __imod__, PyObject *)
/* Can't use SLOT1 here, because nb_inplace_power is ternary */
static PyObject *
slot_nb_inplace_power(PyObject *self, PyObject * arg1, PyObject *arg2)
{
    PyObject *stack[2] = {self, arg1};
    return vectorcall_method(&_Py_ID(__ipow__), stack, 2);
}
SLOT1(slot_nb_inplace_lshift, __ilshift__, PyObject *)
SLOT1(slot_nb_inplace_rshift, __irshift__, PyObject *)
SLOT1(slot_nb_inplace_and, __iand__, PyObject *)
SLOT1(slot_nb_inplace_xor, __ixor__, PyObject *)
SLOT1(slot_nb_inplace_or, __ior__, PyObject *)
SLOT1BIN(slot_nb_floor_divide, nb_floor_divide,
         __floordiv__, __rfloordiv__)
SLOT1BIN(slot_nb_true_divide, nb_true_divide, __truediv__, __rtruediv__)
SLOT1(slot_nb_inplace_floor_divide, __ifloordiv__, PyObject *)
SLOT1(slot_nb_inplace_true_divide, __itruediv__, PyObject *)

static PyObject *
slot_tp_repr(PyObject *self)
{
    PyObject *res = _PyObject_MaybeCallSpecialNoArgs(self, &_Py_ID(__repr__));
    if (res != NULL) {
        return res;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    return PyUnicode_FromFormat("<%s object at %p>",
                                Py_TYPE(self)->tp_name, self);
}

SLOT0(slot_tp_str, __str__)

static Py_hash_t
slot_tp_hash(PyObject *self)
{
    PyObject *res;
    int attr_is_none = 0;
    res  = maybe_call_special_no_args(self, &_Py_ID(__hash__), &attr_is_none);
    if (attr_is_none || res == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return PyObject_HashNotImplemented(self);
    }
    if (!PyLong_Check(res)) {
        Py_DECREF(res);
        PyErr_SetString(PyExc_TypeError,
                        "__hash__ method should return an integer");
        return -1;
    }
    /* Transform the PyLong `res` to a Py_hash_t `h`.  For an existing
       hashable Python object x, hash(x) will always lie within the range of
       Py_hash_t.  Therefore our transformation must preserve values that
       already lie within this range, to ensure that if x.__hash__() returns
       hash(y) then hash(x) == hash(y). */
    Py_ssize_t h = PyLong_AsSsize_t(res);
    if (h == -1 && PyErr_Occurred()) {
        /* res was not within the range of a Py_hash_t, so we're free to
           use any sufficiently bit-mixing transformation;
           long.__hash__ will do nicely. */
        PyErr_Clear();
        h = PyLong_Type.tp_hash(res);
    }
    /* -1 is reserved for errors. */
    if (h == -1)
        h = -2;
    Py_DECREF(res);
    return h;
}

static PyObject *
slot_tp_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    return call_method(self, &_Py_ID(__call__), args, kwds);
}

/* There are two slot dispatch functions for tp_getattro.

   - _Py_slot_tp_getattro() is used when __getattribute__ is overridden
     but no __getattr__ hook is present;

   - _Py_slot_tp_getattr_hook() is used when a __getattr__ hook is present.

   The code in update_one_slot() always installs _Py_slot_tp_getattr_hook();
   this detects the absence of __getattr__ and then installs the simpler
   slot if necessary. */

PyObject *
_Py_slot_tp_getattro(PyObject *self, PyObject *name)
{
    PyObject *stack[2] = {self, name};
    return vectorcall_method(&_Py_ID(__getattribute__), stack, 2);
}

static inline PyObject *
call_attribute(PyObject *self, PyObject *attr, PyObject *name)
{
    PyObject *res, *descr = NULL;

    if (_PyType_HasFeature(Py_TYPE(attr), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
        PyObject *args[] = { self, name };
        res = PyObject_Vectorcall(attr, args, 2, NULL);
        return res;
    }

    descrgetfunc f = Py_TYPE(attr)->tp_descr_get;

    if (f != NULL) {
        descr = f(attr, self, (PyObject *)(Py_TYPE(self)));
        if (descr == NULL)
            return NULL;
        else
            attr = descr;
    }
    res = PyObject_CallOneArg(attr, name);
    Py_XDECREF(descr);
    return res;
}

PyObject *
_Py_slot_tp_getattr_hook(PyObject *self, PyObject *name)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject *getattr, *getattribute, *res;

    /* speed hack: we could use lookup_maybe, but that would resolve the
       method fully for each attribute lookup for classes with
       __getattr__, even when the attribute is present. So we use
       _PyType_LookupRef and create the method only when needed, with
       call_attribute. */
    getattr = _PyType_LookupRef(tp, &_Py_ID(__getattr__));
    if (getattr == NULL) {
        /* No __getattr__ hook: use a simpler dispatcher */
#ifndef Py_GIL_DISABLED
        // Replacing the slot is only thread-safe if there is a GIL.
        tp->tp_getattro = _Py_slot_tp_getattro;
#endif
        return _Py_slot_tp_getattro(self, name);
    }
    /* speed hack: we could use lookup_maybe, but that would resolve the
       method fully for each attribute lookup for classes with
       __getattr__, even when self has the default __getattribute__
       method. So we use _PyType_LookupRef and create the method only when
       needed, with call_attribute. */
    getattribute = _PyType_LookupRef(tp, &_Py_ID(__getattribute__));
    if (getattribute == NULL ||
        (Py_IS_TYPE(getattribute, &PyWrapperDescr_Type) &&
         ((PyWrapperDescrObject *)getattribute)->d_wrapped ==
             (void *)PyObject_GenericGetAttr)) {
        Py_XDECREF(getattribute);
        res = _PyObject_GenericGetAttrWithDict(self, name, NULL, 1);
        /* if res == NULL with no exception set, then it must be an
           AttributeError suppressed by us. */
        if (res == NULL && !PyErr_Occurred()) {
            res = call_attribute(self, getattr, name);
        }
    }
    else {
        res = call_attribute(self, getattribute, name);
        Py_DECREF(getattribute);
        if (res == NULL && PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            res = call_attribute(self, getattr, name);
        }
    }

    Py_DECREF(getattr);
    return res;
}

static int
slot_tp_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PyObject *stack[3];
    PyObject *res;

    stack[0] = self;
    stack[1] = name;
    if (value == NULL) {
        res = vectorcall_method(&_Py_ID(__delattr__), stack, 2);
    }
    else {
        stack[2] = value;
        res = vectorcall_method(&_Py_ID(__setattr__), stack, 3);
    }
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

static PyObject *name_op[] = {
    &_Py_ID(__lt__),
    &_Py_ID(__le__),
    &_Py_ID(__eq__),
    &_Py_ID(__ne__),
    &_Py_ID(__gt__),
    &_Py_ID(__ge__),
};

static PyObject *
slot_tp_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *res = _PyObject_MaybeCallSpecialOneArg(self, name_op[op], other);
    if (res == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        Py_RETURN_NOTIMPLEMENTED;
    }
    return res;
}

static int
has_dunder_getitem(PyObject *self)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *self_owner = NULL;
    _PyCValue cref;
    int status = -1;
    int result = -1;
    if (PyErr_Occurred()) {
        return -1;
    }
    self_owner = Py_NewRef(self);
    _PyThreadState_PushCValue(tstate, &cref);
    status = lookup_maybe_method(self_owner, &_Py_ID(__getitem__), &cref.ref);
    if (PyErr_Occurred()) {
        result = -1;
    }
    else {
        result = status >= 0 ? 1 : 0;
    }
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        return -1;
    }
    return result;
}

static PyObject *
slot_tp_iter(PyObject *self)
{
    PyObject *self_owner = NULL;
    PyObject *result = NULL;
    PyObject *diagnostic_type = NULL;
    PyObject *diagnostic_name = NULL;
    int attr_is_none = 0;
    int present;
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(self);
    result = maybe_call_special_no_args(self_owner, &_Py_ID(__iter__),
                                        &attr_is_none);
    if (PyErr_Occurred() || result != NULL) {
        goto done;
    }
    if (attr_is_none) {
        goto not_iterable;
    }
    present = has_dunder_getitem(self_owner);
    if (PyErr_Occurred() || present < 0) {
        goto done;
    }
    if (present) {
        result = PySeqIter_New(self_owner);
        goto done;
    }

not_iterable:
    diagnostic_type = Py_NewRef((PyObject *)Py_TYPE(self_owner));
    if (PyType_HasFeature((PyTypeObject *)diagnostic_type,
                          Py_TPFLAGS_HEAPTYPE)) {
        diagnostic_name = Py_NewRef(
            ((PyHeapTypeObject *)diagnostic_type)->ht_name);
    }
    const char *type_name = ((PyTypeObject *)diagnostic_type)->tp_name;
    PyErr_Format(PyExc_TypeError,
        "'%.200s' object is not iterable",
        type_name);

done:
    type_mutation_close(&diagnostic_name);
    type_mutation_close(&diagnostic_type);
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
        return NULL;
    }
    return result;
}

static PyObject *
slot_tp_iternext(PyObject *self)
{
    PyObject *stack[1] = {self};
    return vectorcall_method(&_Py_ID(__next__), stack, 1);
}

int
_PyType_HasSlotTpIternext(PyTypeObject *type)
{
    return type->tp_iternext == slot_tp_iternext;
}

static PyObject *
slot_tp_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    PyObject *self_owner = NULL, *obj_owner = NULL, *type_owner = NULL;
    PyObject *receiver_type_owner = NULL, *get = NULL, *result = NULL;
    if (PyErr_Occurred()) {
        return NULL;
    }
    self_owner = Py_NewRef(self);
    obj_owner = Py_XNewRef(obj);
    type_owner = Py_XNewRef(type);
    receiver_type_owner = Py_NewRef((PyObject *)Py_TYPE(self_owner));
    PyTypeObject *tp = (PyTypeObject *)receiver_type_owner;
    get = _PyType_LookupRef(tp, &_Py_ID(__get__));
    if (PyErr_Occurred()) {
        goto done;
    }
    if (get == NULL) {
#ifndef Py_GIL_DISABLED
        if (tp->tp_descr_get == slot_tp_descr_get) {
            tp->tp_descr_get = NULL;
        }
#endif
        result = Py_NewRef(self_owner);
        goto done;
    }
    PyObject *stack[3] = {self_owner,
        obj_owner != NULL ? obj_owner : Py_None,
        type_owner != NULL ? type_owner : Py_None};
    result = PyObject_Vectorcall(get, stack, 3, NULL);
done:
    type_mutation_close(&get);
    type_mutation_close(&receiver_type_owner);
    type_mutation_close(&type_owner);
    type_mutation_close(&obj_owner);
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static int
slot_tp_descr_set(PyObject *self, PyObject *target, PyObject *value)
{
    PyObject* stack[3];
    PyObject *res;

    stack[0] = self;
    stack[1] = target;
    if (value == NULL) {
        res = vectorcall_method(&_Py_ID(__delete__), stack, 2);
    }
    else {
        stack[2] = value;
        res = vectorcall_method(&_Py_ID(__set__), stack, 3);
    }
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

static int
slot_tp_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *res = call_method(self, &_Py_ID(__init__), args, kwds);
    if (res == NULL)
        return -1;
    if (res != Py_None) {
        PyErr_Format(PyExc_TypeError,
                     "__init__() should return None, not '%.200s'",
                     Py_TYPE(res)->tp_name);
        Py_DECREF(res);
        return -1;
    }
    Py_DECREF(res);
    return 0;
}

static PyObject *
slot_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *type_owner = NULL, *args_owner = NULL, *kwds_owner = NULL;
    PyObject *func = NULL, *result = NULL;
    if (PyErr_Occurred()) {
        return NULL;
    }
    type_owner = Py_NewRef((PyObject *)type);
    args_owner = Py_NewRef(args);
    kwds_owner = Py_XNewRef(kwds);
    func = PyObject_GetAttr(type_owner, &_Py_ID(__new__));
    if (func == NULL || PyErr_Occurred()) {
        goto done;
    }
    result = _PyObject_Call_Prepend(_PyThreadState_GET(), func,
                                  type_owner, args_owner, kwds_owner);
done:
    type_mutation_close(&func);
    type_mutation_close(&kwds_owner);
    type_mutation_close(&args_owner);
    type_mutation_close(&type_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&result);
    }
    return result;
}

static void
slot_tp_finalize(PyObject *self)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    PyObject *self_owner = Py_NewRef(self);
    PyObject *result = NULL;
    PyObject *report_owner = NULL;
    _PyCValue cref;
    _PyThreadState_PushCValue(tstate, &cref);
    int unbound = lookup_maybe_method(self_owner, &_Py_ID(__del__), &cref.ref);
    if (unbound >= 0 && !PyErr_Occurred()) {
        assert(!PyValue_IsNull(cref.ref));
        PyObject *del = PyValue_AsPyObjectBorrow(cref.ref);
        report_owner = Py_NewRef(del);
        result = call_unbound_noarg(unbound, del, self_owner);
    }
    else {
        report_owner = Py_NewRef(self_owner);
    }
    type_mutation_close(&result);
    {
        PyValue retired = _PyThreadState_PopCValueSteal(tstate, &cref);
        cref.ref = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_XCLOSE(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyErr_FormatUnraisable("Exception ignored while "
                               "calling deallocator %R", report_owner);
    }
    type_mutation_close(&report_owner);
    type_mutation_close(&self_owner);
    if (PyErr_Occurred()) {
        /* Original caller/live or temporary-resurrection owner still roots self. */
        PyErr_FormatUnraisable("Exception ignored while "
                               "calling deallocator %R", self);
    }
    /* format_unraisable_v clears its error even when the hook fails. */
    _PyErr_SetRaisedException(tstate, exc);
    exc = NULL;
}

typedef struct _PyBufferWrapper {
    PyObject_HEAD
    PyObject *mv;
    PyObject *obj;
} PyBufferWrapper;

#define PyBufferWrapper_CAST(op)    ((PyBufferWrapper *)(op))

static int
bufferwrapper_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyBufferWrapper *self = PyBufferWrapper_CAST(op);
    Py_VISIT(self->mv);
    Py_VISIT(self->obj);
    return 0;
}

static void
bufferwrapper_dealloc(PyObject *self)
{
    PyBufferWrapper *bw = PyBufferWrapper_CAST(self);

    _PyObject_GC_UNTRACK(self);
    Py_XDECREF(bw->mv);
    Py_XDECREF(bw->obj);
    Py_TYPE(self)->tp_free(self);
}

static void
bufferwrapper_releasebuf(PyObject *self, Py_buffer *view)
{
    PyBufferWrapper *bw = PyBufferWrapper_CAST(self);

    if (bw->mv == NULL || bw->obj == NULL) {
        // Already released
        return;
    }

    PyObject *mv = bw->mv;
    PyObject *obj = bw->obj;

    assert(PyMemoryView_Check(mv));
    Py_TYPE(mv)->tp_as_buffer->bf_releasebuffer(mv, view);
    // We only need to call bf_releasebuffer if it's a Python function. If it's a C
    // bf_releasebuf, it will be called when the memoryview is released.
    if (((PyMemoryViewObject *)mv)->view.obj != obj
            && Py_TYPE(obj)->tp_as_buffer != NULL
            && Py_TYPE(obj)->tp_as_buffer->bf_releasebuffer == slot_bf_releasebuffer) {
        releasebuffer_call_python(obj, view);
    }

    Py_CLEAR(bw->mv);
    Py_CLEAR(bw->obj);
}


static PyBufferProcs bufferwrapper_as_buffer = {
    .bf_releasebuffer = bufferwrapper_releasebuf,
};


PyTypeObject _PyBufferWrapper_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "_buffer_wrapper",
    .tp_basicsize = sizeof(PyBufferWrapper),
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_traverse = bufferwrapper_traverse,
    .tp_dealloc = bufferwrapper_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_as_buffer = &bufferwrapper_as_buffer,
};

static int
slot_bf_getbuffer(PyObject *self, Py_buffer *buffer, int flags)
{
    PyObject *flags_obj = PyLong_FromLong(flags);
    if (flags_obj == NULL) {
        return -1;
    }
    PyBufferWrapper *wrapper = NULL;
    PyObject *stack[2] = {self, flags_obj};
    PyObject *ret = vectorcall_method(&_Py_ID(__buffer__), stack, 2);
    if (ret == NULL) {
        goto fail;
    }
    if (!PyMemoryView_Check(ret)) {
        PyErr_Format(PyExc_TypeError,
                     "%T.__buffer__() must return a memoryview, not %T",
                     self, ret);
        goto fail;
    }

    if (PyObject_GetBuffer(ret, buffer, flags) < 0) {
        goto fail;
    }
    assert(buffer->obj == ret);

    wrapper = PyObject_GC_New(PyBufferWrapper, &_PyBufferWrapper_Type);
    if (wrapper == NULL) {
        PyBuffer_Release(buffer);
        goto fail;
    }
    wrapper->mv = ret;
    wrapper->obj = Py_NewRef(self);
    _PyObject_GC_TRACK(wrapper);

    buffer->obj = (PyObject *)wrapper;
    Py_DECREF(ret);
    Py_DECREF(flags_obj);
    return 0;

fail:
    Py_XDECREF(wrapper);
    Py_XDECREF(ret);
    Py_DECREF(flags_obj);
    return -1;
}

static releasebufferproc
releasebuffer_maybe_call_super_unlocked(PyObject *self, Py_buffer *buffer)
{
    PyTypeObject *self_type = Py_TYPE(self);
    PyObject *mro = lookup_tp_mro(self_type);
    if (mro == NULL) {
        return NULL;
    }

    assert(PyTuple_Check(mro));
    Py_ssize_t n = PyTuple_GET_SIZE(mro);
    Py_ssize_t i;

    /* No need to check the last one: it's gonna be skipped anyway.  */
    for (i = 0;  i < n -1; i++) {
        PyValue value = PyTuple_GET_VALUE(mro, i);
        assert(PyValue_IsObject(value));
        assert(PyValue_AsPyObjectBorrow(value) == (PyObject *)self_type ||
               PyType_Check(PyValue_AsPyObjectBorrow(value)));
        if ((PyObject *)(self_type) == PyValue_AsPyObjectBorrow(value))
            break;
    }
    i++;  /* skip self_type */
    if (i >= n)
        return NULL;

    for (; i < n; i++) {
        PyValue value = PyTuple_GET_VALUE(mro, i);
        assert(PyValue_IsObject(value));
        PyObject *obj = PyValue_AsPyObjectBorrow(value);
        assert(PyType_Check(obj));
        PyTypeObject *base_type = (PyTypeObject *)obj;
        if (base_type->tp_as_buffer != NULL
            && base_type->tp_as_buffer->bf_releasebuffer != NULL
            && base_type->tp_as_buffer->bf_releasebuffer != slot_bf_releasebuffer) {
            return base_type->tp_as_buffer->bf_releasebuffer;
        }
    }

    return NULL;
}

static void
releasebuffer_maybe_call_super(PyObject *self, Py_buffer *buffer)
{
    releasebufferproc base_releasebuffer;

    BEGIN_TYPE_LOCK();
    base_releasebuffer = releasebuffer_maybe_call_super_unlocked(self, buffer);
    END_TYPE_LOCK();

    if (base_releasebuffer != NULL) {
        base_releasebuffer(self, buffer);
    }
}

static void
releasebuffer_call_python(PyObject *self, Py_buffer *buffer)
{
    // bf_releasebuffer may be called while an exception is already active.
    // We have no way to report additional errors up the stack, because
    // this slot returns void, so we simply stash away the active exception
    // and restore it after the call to Python returns.
    PyObject *exc = PyErr_GetRaisedException();

    PyObject *mv;
    bool is_buffer_wrapper = Py_TYPE(buffer->obj) == &_PyBufferWrapper_Type;
    if (is_buffer_wrapper) {
        // Make sure we pass the same memoryview to
        // __release_buffer__() that __buffer__() returned.
        PyBufferWrapper *bw = (PyBufferWrapper *)buffer->obj;
        if (bw->mv == NULL) {
            goto end;
        }
        mv = Py_NewRef(bw->mv);
    }
    else {
        // This means we are not dealing with a memoryview returned
        // from a Python __buffer__ function.
        mv = PyMemoryView_FromBuffer(buffer);
        if (mv == NULL) {
            PyErr_FormatUnraisable("Exception ignored in bf_releasebuffer of %s", Py_TYPE(self)->tp_name);
            goto end;
        }
        // Set the memoryview to restricted mode, which forbids
        // users from saving any reference to the underlying buffer
        // (e.g., by doing .cast()). This is necessary to ensure
        // no Python code retains a reference to the to-be-released
        // buffer.
        ((PyMemoryViewObject *)mv)->flags |= _Py_MEMORYVIEW_RESTRICTED;
    }
    PyObject *stack[2] = {self, mv};
    PyObject *ret = vectorcall_method(&_Py_ID(__release_buffer__), stack, 2);
    if (ret == NULL) {
        PyErr_FormatUnraisable("Exception ignored in __release_buffer__ of %s", Py_TYPE(self)->tp_name);
    }
    else {
        Py_DECREF(ret);
    }
    if (!is_buffer_wrapper) {
        PyObject *res = PyObject_CallMethodNoArgs(mv, &_Py_ID(release));
        if (res == NULL) {
            PyErr_FormatUnraisable("Exception ignored in bf_releasebuffer of %s", Py_TYPE(self)->tp_name);
        }
        else {
            Py_DECREF(res);
        }
    }
    Py_DECREF(mv);
end:
    assert(!PyErr_Occurred());

    PyErr_SetRaisedException(exc);
}

/*
 * bf_releasebuffer is very delicate, because we need to ensure that
 * C bf_releasebuffer slots are called correctly (or we'll leak memory),
 * but we cannot trust any __release_buffer__ implemented in Python to
 * do so correctly. Therefore, if a base class has a C bf_releasebuffer
 * slot, we call it directly here. That is safe because this function
 * only gets called from C callers of the bf_releasebuffer slot. Python
 * code that calls __release_buffer__ directly instead goes through
 * wrap_releasebuffer(), which doesn't call the bf_releasebuffer slot
 * directly but instead simply releases the associated memoryview.
 */
static void
slot_bf_releasebuffer(PyObject *self, Py_buffer *buffer)
{
    releasebuffer_call_python(self, buffer);
    releasebuffer_maybe_call_super(self, buffer);
}

static PyObject *
slot_am_generic(PyObject *self, PyObject *name)
{
    PyObject *res = _PyObject_MaybeCallSpecialNoArgs(self, name);
    if (res == NULL && !PyErr_Occurred()) {
        PyErr_Format(PyExc_AttributeError,
            "object %.50s does not have %U method",
            Py_TYPE(self)->tp_name, name);
    }
    return res;
}

static PyObject *
slot_am_await(PyObject *self)
{
    return slot_am_generic(self, &_Py_ID(__await__));
}

static PyObject *
slot_am_aiter(PyObject *self)
{
    return slot_am_generic(self, &_Py_ID(__aiter__));
}

static PyObject *
slot_am_anext(PyObject *self)
{
    return slot_am_generic(self, &_Py_ID(__anext__));
}

/*
Table mapping __foo__ names to tp_foo offsets and slot_tp_foo wrapper functions.

The table is ordered by offsets relative to the 'PyHeapTypeObject' structure,
which incorporates the additional structures used for numbers, sequences and
mappings.  Note that multiple names may map to the same slot (e.g. __eq__,
__ne__ etc. all map to tp_richcompare) and one name may map to multiple slots
(e.g. __str__ affects tp_str as well as tp_repr). The table is terminated with
an all-zero entry.
*/

#undef TPSLOT
#undef FLSLOT
#undef BUFSLOT
#undef AMSLOT
#undef ETSLOT
#undef SQSLOT
#undef MPSLOT
#undef NBSLOT
#undef UNSLOT
#undef IBSLOT
#undef BINSLOT
#undef RBINSLOT

#define TPSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    {#NAME, offsetof(PyTypeObject, SLOT), (void *)(FUNCTION), WRAPPER, \
     PyDoc_STR(DOC), .name_strobj = &_Py_ID(NAME)}
#define FLSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC, FLAGS) \
    {#NAME, offsetof(PyTypeObject, SLOT), (void *)(FUNCTION), WRAPPER, \
     PyDoc_STR(DOC), FLAGS, .name_strobj = &_Py_ID(NAME) }
#define ETSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    {#NAME, offsetof(PyHeapTypeObject, SLOT), (void *)(FUNCTION), WRAPPER, \
     PyDoc_STR(DOC), .name_strobj = &_Py_ID(NAME) }
#define BUFSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_buffer.SLOT, FUNCTION, WRAPPER, DOC)
#define AMSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_async.SLOT, FUNCTION, WRAPPER, DOC)
#define SQSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_sequence.SLOT, FUNCTION, WRAPPER, DOC)
#define MPSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_mapping.SLOT, FUNCTION, WRAPPER, DOC)
#define NBSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, WRAPPER, DOC)
#define UNSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, WRAPPER, \
           #NAME "($self, /)\n--\n\n" DOC)
#define IBSLOT(NAME, SLOT, FUNCTION, WRAPPER, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, WRAPPER, \
           #NAME "($self, value, /)\n--\n\nReturn self" DOC "value.")
#define BINSLOT(NAME, SLOT, FUNCTION, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, wrap_binaryfunc_l, \
           #NAME "($self, value, /)\n--\n\nReturn self" DOC "value.")
#define RBINSLOT(NAME, SLOT, FUNCTION, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, wrap_binaryfunc_r, \
           #NAME "($self, value, /)\n--\n\nReturn value" DOC "self.")
#define BINSLOTNOTINFIX(NAME, SLOT, FUNCTION, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, wrap_binaryfunc_l, \
           #NAME "($self, value, /)\n--\n\n" DOC)
#define RBINSLOTNOTINFIX(NAME, SLOT, FUNCTION, DOC) \
    ETSLOT(NAME, as_number.SLOT, FUNCTION, wrap_binaryfunc_r, \
           #NAME "($self, value, /)\n--\n\n" DOC)

static pytype_slotdef slotdefs[] = {
    TPSLOT(__getattribute__, tp_getattr, NULL, NULL, ""),
    TPSLOT(__getattr__, tp_getattr, NULL, NULL, ""),
    TPSLOT(__setattr__, tp_setattr, NULL, NULL, ""),
    TPSLOT(__delattr__, tp_setattr, NULL, NULL, ""),
    TPSLOT(__repr__, tp_repr, slot_tp_repr, wrap_unaryfunc,
           "__repr__($self, /)\n--\n\nReturn repr(self)."),
    TPSLOT(__hash__, tp_hash, slot_tp_hash, wrap_hashfunc,
           "__hash__($self, /)\n--\n\nReturn hash(self)."),
    FLSLOT(__call__, tp_call, slot_tp_call,
           _Py_FUNC_CAST(wrapperfunc, wrap_call),
           "__call__($self, /, *args, **kwargs)\n--\n\nCall self as a function.",
           PyWrapperFlag_KEYWORDS),
    TPSLOT(__str__, tp_str, slot_tp_str, wrap_unaryfunc,
           "__str__($self, /)\n--\n\nReturn str(self)."),
    TPSLOT(__getattribute__, tp_getattro, _Py_slot_tp_getattr_hook,
           wrap_binaryfunc,
           "__getattribute__($self, name, /)\n--\n\nReturn getattr(self, name)."),
    TPSLOT(__getattr__, tp_getattro, _Py_slot_tp_getattr_hook, NULL,
           "__getattr__($self, name, /)\n--\n\nImplement getattr(self, name)."),
    TPSLOT(__setattr__, tp_setattro, slot_tp_setattro, wrap_setattr,
           "__setattr__($self, name, value, /)\n--\n\nImplement setattr(self, name, value)."),
    TPSLOT(__delattr__, tp_setattro, slot_tp_setattro, wrap_delattr,
           "__delattr__($self, name, /)\n--\n\nImplement delattr(self, name)."),
    TPSLOT(__lt__, tp_richcompare, slot_tp_richcompare, richcmp_lt,
           "__lt__($self, value, /)\n--\n\nReturn self<value."),
    TPSLOT(__le__, tp_richcompare, slot_tp_richcompare, richcmp_le,
           "__le__($self, value, /)\n--\n\nReturn self<=value."),
    TPSLOT(__eq__, tp_richcompare, slot_tp_richcompare, richcmp_eq,
           "__eq__($self, value, /)\n--\n\nReturn self==value."),
    TPSLOT(__ne__, tp_richcompare, slot_tp_richcompare, richcmp_ne,
           "__ne__($self, value, /)\n--\n\nReturn self!=value."),
    TPSLOT(__gt__, tp_richcompare, slot_tp_richcompare, richcmp_gt,
           "__gt__($self, value, /)\n--\n\nReturn self>value."),
    TPSLOT(__ge__, tp_richcompare, slot_tp_richcompare, richcmp_ge,
           "__ge__($self, value, /)\n--\n\nReturn self>=value."),
    TPSLOT(__iter__, tp_iter, slot_tp_iter, wrap_unaryfunc,
           "__iter__($self, /)\n--\n\nImplement iter(self)."),
    TPSLOT(__next__, tp_iternext, slot_tp_iternext, wrap_next,
           "__next__($self, /)\n--\n\nImplement next(self)."),
    TPSLOT(__get__, tp_descr_get, slot_tp_descr_get, wrap_descr_get,
           "__get__($self, instance, owner=None, /)\n--\n\nReturn an attribute of instance, which is of type owner."),
    TPSLOT(__set__, tp_descr_set, slot_tp_descr_set, wrap_descr_set,
           "__set__($self, instance, value, /)\n--\n\nSet an attribute of instance to value."),
    TPSLOT(__delete__, tp_descr_set, slot_tp_descr_set,
           wrap_descr_delete,
           "__delete__($self, instance, /)\n--\n\nDelete an attribute of instance."),
    FLSLOT(__init__, tp_init, slot_tp_init,
           _Py_FUNC_CAST(wrapperfunc, wrap_init),
           "__init__($self, /, *args, **kwargs)\n--\n\n"
           "Initialize self.  See help(type(self)) for accurate signature.",
           PyWrapperFlag_KEYWORDS),
    TPSLOT(__new__, tp_new, slot_tp_new, NULL,
           "__new__(type, /, *args, **kwargs)\n--\n\n"
           "Create and return new object.  See help(type) for accurate signature."),
    TPSLOT(__del__, tp_finalize, slot_tp_finalize, (wrapperfunc)wrap_del,
           "__del__($self, /)\n--\n\n"
           "Called when the instance is about to be destroyed."),

    BUFSLOT(__buffer__, bf_getbuffer, slot_bf_getbuffer, wrap_buffer,
            "__buffer__($self, flags, /)\n--\n\n"
            "Return a buffer object that exposes the underlying memory of the object."),
    BUFSLOT(__release_buffer__, bf_releasebuffer, slot_bf_releasebuffer, wrap_releasebuffer,
            "__release_buffer__($self, buffer, /)\n--\n\n"
            "Release the buffer object that exposes the underlying memory of the object."),

    AMSLOT(__await__, am_await, slot_am_await, wrap_unaryfunc,
           "__await__($self, /)\n--\n\nReturn an iterator to be used in await expression."),
    AMSLOT(__aiter__, am_aiter, slot_am_aiter, wrap_unaryfunc,
           "__aiter__($self, /)\n--\n\nReturn an awaitable, that resolves in asynchronous iterator."),
    AMSLOT(__anext__, am_anext, slot_am_anext, wrap_unaryfunc,
           "__anext__($self, /)\n--\n\nReturn a value or raise StopAsyncIteration."),

    BINSLOT(__add__, nb_add, slot_nb_add,
           "+"),
    RBINSLOT(__radd__, nb_add, slot_nb_add,
           "+"),
    BINSLOT(__sub__, nb_subtract, slot_nb_subtract,
           "-"),
    RBINSLOT(__rsub__, nb_subtract, slot_nb_subtract,
           "-"),
    BINSLOT(__mul__, nb_multiply, slot_nb_multiply,
           "*"),
    RBINSLOT(__rmul__, nb_multiply, slot_nb_multiply,
           "*"),
    BINSLOT(__mod__, nb_remainder, slot_nb_remainder,
           "%"),
    RBINSLOT(__rmod__, nb_remainder, slot_nb_remainder,
           "%"),
    BINSLOTNOTINFIX(__divmod__, nb_divmod, slot_nb_divmod,
           "Return divmod(self, value)."),
    RBINSLOTNOTINFIX(__rdivmod__, nb_divmod, slot_nb_divmod,
           "Return divmod(value, self)."),
    NBSLOT(__pow__, nb_power, slot_nb_power, wrap_ternaryfunc,
           "__pow__($self, value, mod=None, /)\n--\n\nReturn pow(self, value, mod)."),
    NBSLOT(__rpow__, nb_power, slot_nb_power, wrap_ternaryfunc_r,
           "__rpow__($self, value, mod=None, /)\n--\n\nReturn pow(value, self, mod)."),
    UNSLOT(__neg__, nb_negative, slot_nb_negative, wrap_unaryfunc, "-self"),
    UNSLOT(__pos__, nb_positive, slot_nb_positive, wrap_unaryfunc, "+self"),
    UNSLOT(__abs__, nb_absolute, slot_nb_absolute, wrap_unaryfunc,
           "abs(self)"),
    UNSLOT(__bool__, nb_bool, slot_nb_bool, wrap_inquirypred,
           "True if self else False"),
    UNSLOT(__invert__, nb_invert, slot_nb_invert, wrap_unaryfunc, "~self"),
    BINSLOT(__lshift__, nb_lshift, slot_nb_lshift, "<<"),
    RBINSLOT(__rlshift__, nb_lshift, slot_nb_lshift, "<<"),
    BINSLOT(__rshift__, nb_rshift, slot_nb_rshift, ">>"),
    RBINSLOT(__rrshift__, nb_rshift, slot_nb_rshift, ">>"),
    BINSLOT(__and__, nb_and, slot_nb_and, "&"),
    RBINSLOT(__rand__, nb_and, slot_nb_and, "&"),
    BINSLOT(__xor__, nb_xor, slot_nb_xor, "^"),
    RBINSLOT(__rxor__, nb_xor, slot_nb_xor, "^"),
    BINSLOT(__or__, nb_or, slot_nb_or, "|"),
    RBINSLOT(__ror__, nb_or, slot_nb_or, "|"),
    UNSLOT(__int__, nb_int, slot_nb_int, wrap_unaryfunc,
           "int(self)"),
    UNSLOT(__float__, nb_float, slot_nb_float, wrap_unaryfunc,
           "float(self)"),
    IBSLOT(__iadd__, nb_inplace_add, slot_nb_inplace_add,
           wrap_binaryfunc, "+="),
    IBSLOT(__isub__, nb_inplace_subtract, slot_nb_inplace_subtract,
           wrap_binaryfunc, "-="),
    IBSLOT(__imul__, nb_inplace_multiply, slot_nb_inplace_multiply,
           wrap_binaryfunc, "*="),
    IBSLOT(__imod__, nb_inplace_remainder, slot_nb_inplace_remainder,
           wrap_binaryfunc, "%="),
    IBSLOT(__ipow__, nb_inplace_power, slot_nb_inplace_power,
           wrap_ternaryfunc, "**="),
    IBSLOT(__ilshift__, nb_inplace_lshift, slot_nb_inplace_lshift,
           wrap_binaryfunc, "<<="),
    IBSLOT(__irshift__, nb_inplace_rshift, slot_nb_inplace_rshift,
           wrap_binaryfunc, ">>="),
    IBSLOT(__iand__, nb_inplace_and, slot_nb_inplace_and,
           wrap_binaryfunc, "&="),
    IBSLOT(__ixor__, nb_inplace_xor, slot_nb_inplace_xor,
           wrap_binaryfunc, "^="),
    IBSLOT(__ior__, nb_inplace_or, slot_nb_inplace_or,
           wrap_binaryfunc, "|="),
    BINSLOT(__floordiv__, nb_floor_divide, slot_nb_floor_divide, "//"),
    RBINSLOT(__rfloordiv__, nb_floor_divide, slot_nb_floor_divide, "//"),
    BINSLOT(__truediv__, nb_true_divide, slot_nb_true_divide, "/"),
    RBINSLOT(__rtruediv__, nb_true_divide, slot_nb_true_divide, "/"),
    IBSLOT(__ifloordiv__, nb_inplace_floor_divide,
           slot_nb_inplace_floor_divide, wrap_binaryfunc, "//="),
    IBSLOT(__itruediv__, nb_inplace_true_divide,
           slot_nb_inplace_true_divide, wrap_binaryfunc, "/="),
    NBSLOT(__index__, nb_index, slot_nb_index, wrap_unaryfunc,
           "__index__($self, /)\n--\n\n"
           "Return self converted to an integer, if self is suitable "
           "for use as an index into a list."),
    BINSLOT(__matmul__, nb_matrix_multiply, slot_nb_matrix_multiply,
            "@"),
    RBINSLOT(__rmatmul__, nb_matrix_multiply, slot_nb_matrix_multiply,
             "@"),
    IBSLOT(__imatmul__, nb_inplace_matrix_multiply, slot_nb_inplace_matrix_multiply,
           wrap_binaryfunc, "@="),
    MPSLOT(__len__, mp_length, slot_mp_length, wrap_lenfunc,
           "__len__($self, /)\n--\n\nReturn len(self)."),
    MPSLOT(__getitem__, mp_subscript, slot_mp_subscript,
           wrap_binaryfunc,
           "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    MPSLOT(__setitem__, mp_ass_subscript, slot_mp_ass_subscript,
           wrap_objobjargproc,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    MPSLOT(__delitem__, mp_ass_subscript, slot_mp_ass_subscript,
           wrap_delitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),

    SQSLOT(__len__, sq_length, slot_sq_length, wrap_lenfunc,
           "__len__($self, /)\n--\n\nReturn len(self)."),
    /* Heap types defining __add__/__mul__ have sq_concat/sq_repeat == NULL.
       The logic in abstract.c always falls back to nb_add/nb_multiply in
       this case.  Defining both the nb_* and the sq_* slots to call the
       user-defined methods has unexpected side-effects, as shown by
       test_descr.notimplemented() */
    SQSLOT(__add__, sq_concat, NULL, wrap_binaryfunc,
           "__add__($self, value, /)\n--\n\nReturn self+value."),
    SQSLOT(__mul__, sq_repeat, NULL, wrap_indexargfunc,
           "__mul__($self, value, /)\n--\n\nReturn self*value."),
    SQSLOT(__rmul__, sq_repeat, NULL, wrap_indexargfunc,
           "__rmul__($self, value, /)\n--\n\nReturn value*self."),
    SQSLOT(__getitem__, sq_item, slot_sq_item, wrap_sq_item,
           "__getitem__($self, key, /)\n--\n\nReturn self[key]."),
    SQSLOT(__setitem__, sq_ass_item, slot_sq_ass_item, wrap_sq_setitem,
           "__setitem__($self, key, value, /)\n--\n\nSet self[key] to value."),
    SQSLOT(__delitem__, sq_ass_item, slot_sq_ass_item, wrap_sq_delitem,
           "__delitem__($self, key, /)\n--\n\nDelete self[key]."),
    SQSLOT(__contains__, sq_contains, slot_sq_contains, wrap_objobjproc,
           "__contains__($self, key, /)\n--\n\nReturn bool(key in self)."),
    SQSLOT(__iadd__, sq_inplace_concat, NULL,
           wrap_binaryfunc,
           "__iadd__($self, value, /)\n--\n\nImplement self+=value."),
    SQSLOT(__imul__, sq_inplace_repeat, NULL,
           wrap_indexargfunc,
           "__imul__($self, value, /)\n--\n\nImplement self*=value."),

    {NULL}
};

/* Stores the number of times where slotdefs has elements with same name.
   This counter precalculated by _PyType_InitSlotDefs() when the main
   interpreter starts. */
static uint8_t slotdefs_dups[Py_ARRAY_LENGTH(slotdefs)][1 + MAX_EQUIV];

/* Given a type pointer and an offset gotten from a slotdef entry, return a
   pointer to the actual slot.  This is not quite the same as simply adding
   the offset to the type pointer, since it takes care to indirect through the
   proper indirection pointer (as_buffer, etc.); it returns NULL if the
   indirection pointer is NULL. */
static void **
slotptr(PyTypeObject *type, int ioffset)
{
    char *ptr;
    long offset = ioffset;

    /* Note: this depends on the order of the members of PyHeapTypeObject! */
    assert(offset >= 0);
    assert((size_t)offset < offsetof(PyHeapTypeObject, ht_name));
    if ((size_t)offset >= offsetof(PyHeapTypeObject, as_buffer)) {
        ptr = (char *)type->tp_as_buffer;
        offset -= offsetof(PyHeapTypeObject, as_buffer);
    }
    else if ((size_t)offset >= offsetof(PyHeapTypeObject, as_sequence)) {
        ptr = (char *)type->tp_as_sequence;
        offset -= offsetof(PyHeapTypeObject, as_sequence);
    }
    else if ((size_t)offset >= offsetof(PyHeapTypeObject, as_mapping)) {
        ptr = (char *)type->tp_as_mapping;
        offset -= offsetof(PyHeapTypeObject, as_mapping);
    }
    else if ((size_t)offset >= offsetof(PyHeapTypeObject, as_number)) {
        ptr = (char *)type->tp_as_number;
        offset -= offsetof(PyHeapTypeObject, as_number);
    }
    else if ((size_t)offset >= offsetof(PyHeapTypeObject, as_async)) {
        ptr = (char *)type->tp_as_async;
        offset -= offsetof(PyHeapTypeObject, as_async);
    }
    else {
        ptr = (char *)type;
    }
    if (ptr != NULL)
        ptr += offset;
    return (void **)ptr;
}

// Return true if "name" corresponds to at least one slot definition.  This is
// a more accurate but more expensive test compared to is_dunder_name().
static bool
has_slotdef(PyObject *name)
{
    for (pytype_slotdef *p = slotdefs; p->name_strobj; p++) {
        if (p->name_strobj == name) {
            return true;
        }
    }
    return false;
}

/* Common code for update_slots_callback() and fixup_slot_dispatchers().
 *
 * This is meant to set a "slot" like type->tp_repr or
 * type->tp_as_sequence->sq_concat by looking up special methods like
 * __repr__ or __add__. The opposite (adding special methods from slots) is
 * done by add_operators(), called from PyType_Ready(). Since update_one_slot()
 * calls PyType_Ready() if needed, the special methods are already in place.
 *
 * The special methods corresponding to each slot are defined in the "slotdef"
 * array. Note that one slot may correspond to multiple special methods and vice
 * versa. For example, tp_richcompare uses 6 methods __lt__, ..., __ge__ and
 * tp_as_number->nb_add uses __add__ and __radd__. In the other direction,
 * __add__ is used by the number and sequence protocols and __getitem__ by the
 * sequence and mapping protocols. This causes a lot of complications.
 *
 * In detail, update_one_slot() does the following:
 *
 * First of all, if the slot in question does not exist, return immediately.
 * This can happen for example if it's tp_as_number->nb_add but tp_as_number
 * is NULL.
 *
 * For the given slot, we loop over all the special methods with a name
 * corresponding to that slot (for example, for tp_descr_set, this would be
 * __set__ and __delete__) and we look up these names in the MRO of the type.
 * If we don't find any special method, the slot is set to NULL (regardless of
 * what was in the slot before).
 *
 * Suppose that we find exactly one special method. If it's a wrapper_descriptor
 * (i.e. a special method calling a slot, for example str.__repr__ which calls
 * the tp_repr for the 'str' class) with the correct name ("__repr__" for
 * tp_repr), for the right class, calling the right wrapper C function (like
 * wrap_unaryfunc for tp_repr), then the slot is set to the slot that the
 * wrapper_descriptor originally wrapped. For example, a class inheriting
 * from 'str' and not redefining __repr__ will have tp_repr set to the tp_repr
 * of 'str'.
 * In all other cases where the special method exists, the slot is set to a
 * wrapper calling the special method. There is one exception: if the special
 * method is a wrapper_descriptor with the correct name but the type has
 * precisely one slot set for that name and that slot is not the one that we
 * are updating, then NULL is put in the slot (this exception is the only place
 * in update_one_slot() where the *existing* slots matter).
 *
 * When there are multiple special methods for the same slot, the above is
 * applied for each special method. As long as the results agree, the common
 * resulting slot is applied. If the results disagree, then a wrapper for
 * the special methods is installed. This is always safe, but less efficient
 * because it uses method lookup instead of direct C calls.
 *
 * There are some further special cases for specific slots, like supporting
 * __hash__ = None for tp_hash and special code for tp_new.
 *
 * When done, next_p is set to the next slotdef with a different offset,
 * because that's convenient for fixup_slot_dispatchers().
 *
 * If the queued_updates pointer is provided, the actual updates to the slot
 * pointers are queued, rather than being immediately performed.  That argument
 * is only used for the free-threaded build since those updates need to be
 * done while the world is stopped.
 *
 * This function will only return an error if the queued_updates argument is
 * provided and allocating memory for the queue fails.  Other exceptions that
 * occur internally are ignored, such as when looking up descriptors. */
static int
update_one_slot(PyTypeObject *type, pytype_slotdef *p, pytype_slotdef **next_p,
                slot_update_t *queued_updates)
{
    ASSERT_NEW_TYPE_OR_LOCKED(type);

    PyObject *descr;
    PyWrapperDescrObject *d;

    // The correct specialized C function, like "tp_repr of str" in the
    // example above
    void *specific = NULL;

    // A generic wrapper that uses method lookup (safe but slow)
    void *generic = NULL;

    // Set to 1 if the generic wrapper is necessary
    int use_generic = 0;

    int offset = p->offset;
    void **ptr = slotptr(type, offset);

    if (ptr == NULL) {
        do {
            ++p;
        } while (p->offset == offset);
        if (next_p != NULL) {
            *next_p = p;
        }
        return 0;
    }
    /* We may end up clearing live exceptions below, so make sure it's ours. */
    assert(!PyErr_Occurred());
    do {
        /* Use faster uncached lookup as we won't get any cache hits during type setup. */
        PyValue descr_ref;
        int res = find_name_in_mro(type, p->name_strobj, &descr_ref);
        if (res <= 0) {
            if (ptr == (void**)&type->tp_iternext) {
                specific = (void *)_PyObject_NextNotImplemented;
            }
            continue;
        }
        descr = PyValue_IsObject(descr_ref)
            ? PyValue_AsPyObjectBorrow(descr_ref) : NULL;
        if (descr != NULL && Py_IS_TYPE(descr, &PyWrapperDescr_Type) &&
            ((PyWrapperDescrObject *)descr)->d_base->name_strobj == p->name_strobj) {
            void **tptr;
            size_t index = (p - slotdefs);
            if (slotdefs_dups[index][0] > 1) {
                tptr = NULL;
                for (size_t i = 1; i <= slotdefs_dups[index][0]; i++) {
                    pytype_slotdef *q = &slotdefs[slotdefs_dups[index][i]];
                    void **qptr = slotptr(type, q->offset);
                    if (qptr == NULL || *qptr == NULL)
                        continue;
                    if (tptr != NULL) {
                        tptr = NULL;
                        break;
                    }
                    tptr = qptr;
                }
            }
            else {
                tptr = slotptr(type, offset);
            }

            if (tptr == NULL || tptr == ptr)
                generic = p->function;
            d = (PyWrapperDescrObject *)descr;
            if ((specific == NULL || specific == d->d_wrapped) &&
                d->d_base->wrapper == p->wrapper &&
                is_subtype_with_mro(lookup_tp_mro(type), type, PyDescr_TYPE(d)))
            {
                specific = d->d_wrapped;
            }
            else {
                /* We cannot use the specific slot function because either
                   - it is not unique: there are multiple methods for this
                     slot and they conflict
                   - the signature is wrong (as checked by the ->wrapper
                     comparison above)
                   - it's wrapping the wrong class
                 */
                use_generic = 1;
            }
        }
        else if (descr != NULL && Py_IS_TYPE(descr, &PyCFunction_Type) &&
                 PyCFunction_GET_FUNCTION(descr) ==
                 _PyCFunction_CAST(tp_new_wrapper) &&
                 ptr == (void**)&type->tp_new)
        {
            /* The __new__ wrapper is not a wrapper descriptor,
               so must be special-cased differently.
               If we don't do this, creating an instance will
               always use slot_tp_new which will look up
               __new__ in the MRO which will call tp_new_wrapper
               which will look through the base classes looking
               for a static base and call its tp_new (usually
               PyType_GenericNew), after performing various
               sanity checks and constructing a new argument
               list.  Cut all that nonsense short -- this speeds
               up instance creation tremendously. */
            specific = (void *)type->tp_new;
            /* XXX I'm not 100% sure that there isn't a hole
               in this reasoning that requires additional
               sanity checks.  I'll buy the first person to
               point out a bug in this reasoning a beer. */
        }
        else if (descr == Py_None &&
                 ptr == (void**)&type->tp_hash) {
            /* We specifically allow __hash__ to be set to None
               to prevent inheritance of the default
               implementation from object.__hash__ */
            specific = (void *)PyObject_HashNotImplemented;
        }
        else {
            use_generic = 1;
            if (generic == NULL && descr != NULL &&
                Py_IS_TYPE(descr, &PyMethodDescr_Type) &&
                *ptr == ((PyMethodDescrObject *)descr)->d_method->ml_meth)
            {
                generic = *ptr;
            }
            else {
                generic = p->function;
            }
            if (p->function == slot_tp_call) {
                /* A generic __call__ is incompatible with vectorcall */
                if (queued_updates == NULL) {
                    type_clear_flags(type, Py_TPFLAGS_HAVE_VECTORCALL);
                }
            }
        }
        PyValue_CLOSE(descr_ref);
    } while ((++p)->offset == offset);

    void *slot_value;
    if (specific && !use_generic) {
        slot_value = specific;
    } else {
        slot_value = generic;
    }

#ifdef Py_GIL_DISABLED
    if (queued_updates != NULL) {
        // queue the update to perform later, while world is stopped
        if (queue_slot_update(queued_updates, type, ptr, slot_value) < 0) {
            return -1;
        }
    } else {
        // do the update to the type structure now
        *ptr = slot_value;
    }
#else
    // always do the update immediately
    assert(queued_updates == NULL);
    *ptr = slot_value;
#endif

    if (next_p != NULL) {
        *next_p = p;
    }
    return 0;
}

/* In the type, update the slots whose slotdefs are gathered in the pp array.
   This is a callback for update_subclasses(). */
static int
update_slots_callback(PyTypeObject *type, void *data)
{
    ASSERT_NEW_TYPE_OR_LOCKED(type);

    update_callback_data_t *update_data = (update_callback_data_t *)data;
    pytype_slotdef **pp = update_data->defs;
    for (; *pp; pp++) {
        if (update_one_slot(type, *pp, NULL, update_data->queued_updates) < 0) {
            return -1;
        }
    }
    return 0;
}

/* Update the slots after assignment to a class (type) attribute. */
static int
update_slot(PyTypeObject *type, PyObject *name, slot_update_t *queued_updates)
{
    pytype_slotdef *ptrs[MAX_EQUIV];
    pytype_slotdef *p;
    pytype_slotdef **pp;
    int offset;

    ASSERT_TYPE_LOCK_HELD();
    assert(PyUnicode_CheckExact(name));
    assert(PyUnicode_CHECK_INTERNED(name));

    pp = ptrs;
    for (p = slotdefs; p->name; p++) {
        assert(PyUnicode_CheckExact(p->name_strobj));
        assert(PyUnicode_CHECK_INTERNED(p->name_strobj));
        assert(PyUnicode_CheckExact(name));
        /* bpo-40521: Using interned strings. */
        if (p->name_strobj == name) {
            *pp++ = p;
        }
    }
    *pp = NULL;
    for (pp = ptrs; *pp; pp++) {
        p = *pp;
        offset = p->offset;
        while (p > slotdefs && (p-1)->offset == offset)
            --p;
        *pp = p;
    }
    if (ptrs[0] == NULL)
        return 0; /* Not an attribute that affects any slots */

    update_callback_data_t callback_data;
    callback_data.defs = ptrs;
    callback_data.queued_updates = queued_updates;
    return update_subclasses(type, name,
                             update_slots_callback, (void *)&callback_data);
}

/* Store the proper functions in the slot dispatches at class (type)
   definition time, based upon which operations the class overrides in its
   dict.  The type must not be revealed to other threads yet, so that the
   slots can be updated directly rather than with the world stopped. */
static void
fixup_slot_dispatchers(PyTypeObject *type)
{
    ASSERT_TYPE_LOCK_HELD();
    ASSERT_WORLD_STOPPED_OR_NEW_TYPE(type);
    assert(!PyErr_Occurred());
    for (pytype_slotdef *p = slotdefs; p->name; ) {
        int rv = update_one_slot(type, p, &p, NULL);
        // always returns 0 if queued_updates == NULL
        assert (rv == 0);
        (void)rv;
    }
}

#ifdef Py_GIL_DISABLED

// Called when __bases__ is re-assigned.
static int
update_all_slots(PyTypeObject* type)
{
    // Note that update_slot() can fail due to out-of-memory when allocating
    // the queue chunks to hold the updates.  That's unlikely since the number
    // of updates is normally small but we handle that case.  update_slot()
    // can fail internally for other reasons (a lookup fails) but those
    // errors are suppressed.
    slot_update_t queued_updates = {0};
    for (pytype_slotdef *p = slotdefs; p->name; p++) {
        if (update_slot(type, p->name_strobj, &queued_updates) < 0) {
            if (queued_updates.head) {
                slot_update_free_chunks(&queued_updates);
            }
            return -1;
        }
    }
    if (queued_updates.head != NULL) {
        apply_type_slot_updates(&queued_updates);
        ASSERT_TYPE_LOCK_HELD();
        slot_update_free_chunks(&queued_updates);
    }
    return 0;
}

#else

// Called when __bases__ is re-assigned.
static int
update_all_slots(PyTypeObject* type)
{
    pytype_slotdef *p;
    for (p = slotdefs; p->name; p++) {
        /* update_slot returns int but can't actually fail in this case*/
        update_slot(type, p->name_strobj, NULL);
    }
    return 0;
}

#endif

int
_PyType_InitSlotDefs(PyInterpreterState *interp)
{
    if (!_Py_IsMainInterpreter(interp)) {
        return 0;
    }
    if (PyErr_Occurred()) {
        return -1;
    }
    int result = -1;
    PyObject *bytearray = NULL;
    PyObject *cache = NULL;
    PyValue value = PyValue_NULL;
    cache = PyDict_New();
    if (cache == NULL || PyErr_Occurred()) {
        goto done;
    }

    pytype_slotdef *p;
    Py_ssize_t idx = 0;
    for (p = slotdefs; p->name_strobj; p++, idx++) {
        assert(idx < 255);
        int found = PyDict_GetItemRef(cache, p->name_strobj, &bytearray);
        if (PyErr_Occurred() || found < 0) {
            goto done;
        }
        if (!bytearray) {
            Py_ssize_t size = sizeof(uint8_t) * (1 + MAX_EQUIV);
            bytearray = PyByteArray_FromStringAndSize(NULL, size);
            if (bytearray == NULL || PyErr_Occurred()) {
                goto done;
            }
            uint8_t *data = (uint8_t *)PyByteArray_AS_STRING(bytearray);
            data[0] = 0;
            int status = PyDict_SetItem(cache, p->name_strobj, bytearray);
            if (PyErr_Occurred() || status < 0) {
                goto done;
            }
        }
        assert(PyByteArray_CheckExact(bytearray));
        uint8_t *data = (uint8_t *)PyByteArray_AS_STRING(bytearray);
        data[0] += 1;
        assert(data[0] < MAX_EQUIV);
        data[data[0]] = (uint8_t)idx;
        type_mutation_close(&bytearray);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    memset(slotdefs_dups, -1, sizeof(slotdefs_dups));

    Py_ssize_t pos = 0;
    int found;
    while ((found = _PyDict_NextValue(cache, &pos, NULL, &value, NULL)) > 0) {
        if (PyErr_Occurred()) {
            goto done;
        }
        assert(PyValue_IsObject(value));
        PyObject *object = PyValue_AsPyObjectBorrow(value);
        assert(PyByteArray_CheckExact(object));
        uint8_t *data = (uint8_t *)PyByteArray_AS_STRING(object);
        uint8_t n = data[0];
        for (uint8_t i = 0; i < n; i++) {
            uint8_t slot = data[i + 1];
            memcpy(&slotdefs_dups[slot], data, sizeof(uint8_t) * (n + 1));
        }
        PyValue saved = value;
        value = PyValue_NULL;
        PyObject *error = PyErr_GetRaisedException();
        PyValue_XCLOSE(saved);
        _PyValue_RestoreException(error);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    if (found < 0 || PyErr_Occurred()) {
        goto done;
    }
    result = 0;

done:
    if (!PyValue_IsNull(value)) {
        PyValue saved = value;
        value = PyValue_NULL;
        PyObject *error = PyErr_GetRaisedException();
        PyValue_XCLOSE(saved);
        _PyValue_RestoreException(error);
    }
    type_mutation_close(&bytearray);
    if (PyErr_Occurred()) {
        result = -1;
    }
    type_mutation_close(&cache);
    if (PyErr_Occurred()) {
        result = -1;
    }
    return result;
}


PyObject *
_PyType_GetSlotWrapperNames(void)
{
    size_t len = Py_ARRAY_LENGTH(slotdefs) - 1;
    PyObject *names = PyList_New(len);
    if (names == NULL) {
        return NULL;
    }
    assert(slotdefs[len].name == NULL);
    for (size_t i = 0; i < len; i++) {
        pytype_slotdef *slotdef = &slotdefs[i];
        assert(slotdef->name != NULL);
        PyList_SET_ITEM(names, i, Py_NewRef(slotdef->name_strobj));
    }
    return names;
}


/* Call __set_name__ on all attributes (including descriptors)
  in a newly generated type */
static int
type_new_set_names(PyTypeObject *type)
{
    if (PyErr_Occurred()) {
        return -1;
    }
    int result = -1;
    PyObject *type_owner = Py_NewRef((PyObject *)type);
    PyObject *source = Py_XNewRef(lookup_tp_dict(type));
    PyObject *names_to_set = NULL;
    PyObject *key = NULL;
    PyObject *value = NULL;
    PyObject *set_name = NULL;
    PyObject *res = NULL;
    PyObject *value_type_owner = NULL;
    PyObject *value_name_owner = NULL;
    PyObject *type_name_owner = NULL;
    if (source == NULL || PyErr_Occurred()) {
        goto done;
    }
    names_to_set = PyDict_Copy(source);
    if (names_to_set == NULL || PyErr_Occurred()) {
        goto done;
    }

    Py_ssize_t i = 0;
    int found;
    while ((found = PyDict_NextEntryRef(names_to_set, &i, &key, &value)) > 0) {
        if (PyErr_Occurred()) {
            goto done;
        }
        set_name = _PyObject_LookupSpecial(value, &_Py_ID(__set_name__));
        if (set_name == NULL) {
            if (PyErr_Occurred()) {
                goto done;
            }
            type_mutation_close(&value);
            type_mutation_close(&key);
            if (PyErr_Occurred()) {
                goto done;
            }
            continue;
        }
        if (PyErr_Occurred()) {
            goto done;
        }
        res = PyObject_CallFunctionObjArgs(set_name, type, key, NULL);
        int call_failed = res == NULL || PyErr_Occurred();
        type_mutation_close(&set_name);
        if (call_failed) {
            type_mutation_close(&res);
            value_type_owner = Py_NewRef((PyObject *)Py_TYPE(value));
            if (_PyType_HasFeature(Py_TYPE(value), Py_TPFLAGS_HEAPTYPE)) {
                value_name_owner = Py_NewRef(((PyHeapTypeObject *)Py_TYPE(value))->ht_name);
            }
            if (_PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE)) {
                type_name_owner = Py_NewRef(((PyHeapTypeObject *)type)->ht_name);
            }
            const char *value_type_name = Py_TYPE(value)->tp_name;
            const char *receiver_type_name = type->tp_name;
            _PyErr_FormatNote(
                "Error calling __set_name__ on '%.100s' instance %R "
                "in '%.100s'",
                value_type_name, key, receiver_type_name);
            type_mutation_close(&type_name_owner);
            type_mutation_close(&value_name_owner);
            type_mutation_close(&value_type_owner);
            goto done;
        }
        if (PyErr_Occurred()) {
            type_mutation_close(&res);
            goto done;
        }
        type_mutation_close(&res);
        if (PyErr_Occurred()) {
            goto done;
        }
        type_mutation_close(&value);
        type_mutation_close(&key);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    if (found < 0 || PyErr_Occurred()) {
        goto done;
    }
    result = 0;

done:
    type_mutation_close(&res);
    type_mutation_close(&set_name);
    type_mutation_close(&value);
    type_mutation_close(&key);
    type_mutation_close(&value_type_owner);
    type_mutation_close(&value_name_owner);
    type_mutation_close(&type_name_owner);
    type_mutation_close(&names_to_set);
    type_mutation_close(&source);
    type_mutation_close(&type_owner);
    if (PyErr_Occurred()) {
        result = -1;
    }
    return result;
}


/* Call __init_subclass__ on the parent of a newly generated type */
static int
type_new_init_subclass(PyTypeObject *type, PyObject *kwds)
{
    PyObject *args[2] = {(PyObject *)type, (PyObject *)type};
    PyObject *super = PyObject_Vectorcall((PyObject *)&PySuper_Type,
                                          args, 2, NULL);
    if (super == NULL) {
        return -1;
    }

    PyObject *func = PyObject_GetAttr(super, &_Py_ID(__init_subclass__));
    Py_DECREF(super);
    if (func == NULL) {
        return -1;
    }

    PyObject *result = PyObject_VectorcallDict(func, NULL, 0, kwds);
    Py_DECREF(func);
    if (result == NULL) {
        return -1;
    }

    Py_DECREF(result);
    return 0;
}


/* recurse_down_subclasses() and update_subclasses() are mutually
   recursive functions to call a callback for all subclasses,
   but refraining from recursing into subclasses that define 'attr_name'. */

static int
update_subclasses(PyTypeObject *type, PyObject *attr_name,
                  update_callback callback, void *data)
{
    if (PyErr_Occurred() || callback(type, data) < 0 || PyErr_Occurred()) {
        return -1;
    }
    return recurse_down_subclasses(type, attr_name, callback, data);
}

static int
recurse_down_subclasses(PyTypeObject *type, PyObject *attr_name,
                        update_callback callback, void *data)
{
    PyObject *children = NULL;
    PyObject *dict = NULL;
    int result = -1;
    if (PyErr_Occurred()) {
        goto done;
    }
    children = _PyType_GetSubclasses(type);
    if (children == NULL || PyErr_Occurred()) {
        goto done;
    }
    Py_ssize_t n = PyList_GET_SIZE(children);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyList_GET_VALUE(children, i);
        assert(PyValue_IsObject(item));
        PyTypeObject *child = _PyType_CAST(PyValue_AsPyObjectBorrow(item));
        dict = Py_XNewRef(lookup_tp_dict(child));
        int contains = 0;
        if (dict != NULL && PyDict_Check(dict)) {
            contains = PyDict_Contains(dict, attr_name);
        }
        type_mutation_close(&dict);
        if (contains < 0 || PyErr_Occurred()) {
            goto done;
        }
        if (!contains &&
                (update_subclasses(child, attr_name, callback, data) < 0 ||
                 PyErr_Occurred())) {
            goto done;
        }
    }
    result = 0;
done:
    type_mutation_close(&dict);
    type_mutation_close(&children);
    return PyErr_Occurred() ? -1 : result;
}

static int
slot_inherited(PyTypeObject *type, pytype_slotdef *slotdef, void **slot)
{
    void **slot_base = slotptr(type->tp_base, slotdef->offset);
    if (slot_base == NULL || *slot != *slot_base) {
        return 0;
    }

    /* Some slots are inherited in pairs. */
    if (slot == (void *)&type->tp_hash) {
        return (type->tp_richcompare == type->tp_base->tp_richcompare);
    }
    else if (slot == (void *)&type->tp_richcompare) {
        return (type->tp_hash == type->tp_base->tp_hash);
    }

    /* It must be inherited (see type_ready_inherit()). */
    return 1;
}

/* This function is called by PyType_Ready() to populate the type's
   dictionary with method descriptors for function slots.  For each
   function slot (like tp_repr) that's defined in the type, one or more
   corresponding descriptors are added in the type's tp_dict dictionary
   under the appropriate name (like __repr__).  Some function slots
   cause more than one descriptor to be added (for example, the nb_add
   slot adds both __add__ and __radd__ descriptors) and some function
   slots compete for the same descriptor (for example both sq_item and
   mp_subscript generate a __getitem__ descriptor).

   In the latter case, the first slotdef entry encountered wins.  Since
   slotdef entries are sorted by the offset of the slot in the
   PyHeapTypeObject, this gives us some control over disambiguating
   between competing slots: the members of PyHeapTypeObject are listed
   from most general to least general, so the most general slot is
   preferred.  In particular, because as_mapping comes before as_sequence,
   for a type that defines both mp_subscript and sq_item, mp_subscript
   wins.

   This only adds new descriptors and doesn't overwrite entries in
   tp_dict that were previously defined.  The descriptors contain a
   reference to the C function they must call, so that it's safe if they
   are copied into a subtype's __dict__ and the subtype has a different
   C function in its slot -- calling the method defined by the
   descriptor will call the C function that was used to create it,
   rather than the C function present in the slot when it is called.
   (This is important because a subtype may have a C function in the
   slot that calls the method from the dictionary, and we want to avoid
   infinite recursion here.) */

static int
add_operators(PyTypeObject *type)
{
    PyObject *dict = lookup_tp_dict(type);
    pytype_slotdef *p;
    PyObject *descr;
    void **ptr;

    for (p = slotdefs; p->name; p++) {
        if (p->wrapper == NULL)
            continue;
        ptr = slotptr(type, p->offset);
        if (!ptr || !*ptr)
            continue;
        /* Also ignore when the type slot has been inherited. */
        if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN
            && type->tp_base != NULL
            && slot_inherited(type, p, ptr))
        {
            continue;
        }
        int r = PyDict_Contains(dict, p->name_strobj);
        if (r > 0)
            continue;
        if (r < 0) {
            return -1;
        }
        if (*ptr == (void *)PyObject_HashNotImplemented) {
            /* Classes may prevent the inheritance of the tp_hash
               slot by storing PyObject_HashNotImplemented in it. Make it
               visible as a None value for the __hash__ attribute. */
            if (PyDict_SetItem(dict, p->name_strobj, Py_None) < 0)
                return -1;
        }
        else {
            descr = PyDescr_NewWrapper(type, p, *ptr);
            if (descr == NULL)
                return -1;
            if (PyDict_SetItem(dict, p->name_strobj, descr) < 0) {
                Py_DECREF(descr);
                return -1;
            }
            Py_DECREF(descr);
        }
    }
    return 0;
}


int
PyType_Freeze(PyTypeObject *type)
{
    // gh-121654: Check the __mro__ instead of __bases__
    PyObject *mro = type_get_mro((PyObject *)type, NULL);
    if (!PyTuple_Check(mro)) {
        Py_DECREF(mro);
        PyErr_SetString(PyExc_TypeError, "unable to get the type MRO");
        return -1;
    }

    int check = check_immutable_bases(type->tp_name, mro, 1);
    Py_DECREF(mro);
    if (check < 0) {
        return -1;
    }

    BEGIN_TYPE_LOCK();
    types_stop_world();
    type_add_flags(type, Py_TPFLAGS_IMMUTABLETYPE);
    types_start_world();
    ASSERT_TYPE_LOCK_HELD();
    _PyType_Modified_Unlocked(type);
    END_TYPE_LOCK();

    return 0;
}


/* Cooperative 'super' */

typedef struct {
    PyObject_HEAD
    PyTypeObject *type;
    PyObject *obj;
    PyTypeObject *obj_type;
} superobject;

#define superobject_CAST(op)    ((superobject *)(op))

static PyMemberDef super_members[] = {
    {"__thisclass__", _Py_T_OBJECT, offsetof(superobject, type), Py_READONLY,
     "the class invoking super()"},
    {"__self__",  _Py_T_OBJECT, offsetof(superobject, obj), Py_READONLY,
     "the instance invoking super(); may be None"},
    {"__self_class__", _Py_T_OBJECT, offsetof(superobject, obj_type), Py_READONLY,
     "the type of the instance invoking super(); may be None"},
    {0}
};

static void
super_dealloc(PyObject *self)
{
    superobject *su = superobject_CAST(self);

    _PyObject_GC_UNTRACK(self);
    Py_XDECREF(su->obj);
    Py_XDECREF(su->type);
    Py_XDECREF(su->obj_type);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
super_repr(PyObject *self)
{
    superobject *su = superobject_CAST(self);

    if (su->obj_type)
        return PyUnicode_FromFormat(
            "<super: <class '%s'>, <%s object>>",
            su->type ? su->type->tp_name : "NULL",
            su->obj_type->tp_name);
    else
        return PyUnicode_FromFormat(
            "<super: <class '%s'>, NULL>",
            su->type ? su->type->tp_name : "NULL");
}

/* Do a super lookup without executing descriptors or falling back to getattr
on the super object itself.

May return NULL with or without an exception set, like PyDict_GetItemWithError. */
PyObject *
_PySuper_LookupDescr(PyTypeObject *su_type, PyTypeObject *su_obj_type, PyObject *name)
{
    if (PyErr_Occurred()) {
        return NULL;
    }
    PyObject *su_type_owner = Py_NewRef((PyObject *)su_type);
    PyObject *su_obj_type_owner = Py_NewRef((PyObject *)su_obj_type);
    PyObject *name_owner = Py_NewRef(name);
    PyObject *mro = lookup_tp_mro(su_obj_type);
    PyObject *mro_owner = Py_XNewRef(mro);
    PyObject *res = NULL;
    int result = 0;

    if (mro_owner != NULL) {
        assert(PyTuple_Check(mro_owner));
        Py_ssize_t n = PyTuple_GET_SIZE(mro_owner);
        Py_ssize_t i;
        for (i = 0; i + 1 < n; i++) {
            PyValue value = PyTuple_GET_VALUE(mro_owner, i);
            assert(PyValue_IsObject(value));
            PyObject *obj = PyValue_AsPyObjectBorrow(value);
            assert(PyType_Check(obj));
            if (obj == su_type_owner) {
                break;
            }
        }
        i++;  /* skip su->type (if any) */
        if (i < n) {
            for (; i < n; i++) {
                PyValue value = PyTuple_GET_VALUE(mro_owner, i);
                assert(PyValue_IsObject(value));
                PyObject *obj = PyValue_AsPyObjectBorrow(value);
                assert(PyType_Check(obj));
                PyObject *dict_owner = Py_XNewRef(lookup_tp_dict((PyTypeObject *)obj));
                assert(dict_owner != NULL && PyDict_Check(dict_owner));
                if (PyErr_Occurred()) {
                    type_mutation_close(&dict_owner);
                    break;
                }
                result = PyDict_GetItemRef(dict_owner, name_owner, &res);
                int failed = PyErr_Occurred() || result < 0;
                type_mutation_close(&dict_owner);
                if (failed || PyErr_Occurred()) {
                    break;
                }
                if (result > 0) {
                    break;
                }
            }
        }
    }

    type_mutation_close(&name_owner);
    type_mutation_close(&su_obj_type_owner);
    type_mutation_close(&su_type_owner);
    type_mutation_close(&mro_owner);
    if (PyErr_Occurred()) {
        type_mutation_close(&res);
        return NULL;
    }
    return result > 0 ? res : NULL;
}

// if `method` is non-NULL, we are looking for a method descriptor,
// and setting `*method = 1` means we found one.
static PyObject *
do_super_lookup(superobject *su, PyTypeObject *su_type, PyObject *su_obj,
                PyTypeObject *su_obj_type, PyObject *name, int *method)
{
    PyObject *res;
    int temp_su = 0;

    if (su_obj_type == NULL) {
        goto skip;
    }

    res = _PySuper_LookupDescr(su_type, su_obj_type, name);
    if (res != NULL) {
        if (method && _PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
            *method = 1;
        }
        else {
            descrgetfunc f = Py_TYPE(res)->tp_descr_get;
            if (f != NULL) {
                PyObject *res2;
                res2 = f(res,
                    /* Only pass 'obj' param if this is instance-mode super
                    (See SF ID #743627)  */
                    (su_obj == (PyObject *)su_obj_type) ? NULL : su_obj,
                    (PyObject *)su_obj_type);
                Py_SETREF(res, res2);
            }
        }

        return res;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }

  skip:
    if (su == NULL) {
        PyObject *args[] = {(PyObject *)su_type, su_obj};
        su = (superobject *)PyObject_Vectorcall((PyObject *)&PySuper_Type, args, 2, NULL);
        if (su == NULL) {
            return NULL;
        }
        temp_su = 1;
    }
    res = PyObject_GenericGetAttr((PyObject *)su, name);
    if (temp_su) {
        Py_DECREF(su);
    }
    return res;
}

static PyObject *
super_getattro(PyObject *self, PyObject *name)
{
    superobject *su = superobject_CAST(self);

    /* We want __class__ to return the class of the super object
       (i.e. super, or a subclass), not the class of su->obj. */
    if (PyUnicode_Check(name) &&
        PyUnicode_GET_LENGTH(name) == 9 &&
        _PyUnicode_Equal(name, &_Py_ID(__class__)))
        return PyObject_GenericGetAttr(self, name);

    return do_super_lookup(su, su->type, su->obj, su->obj_type, name, NULL);
}

static PyTypeObject *
supercheck(PyTypeObject *type, PyObject *obj)
{
    /* Check that a super() call makes sense.  Return a type object.

       obj can be a class, or an instance of one:

       - If it is a class, it must be a subclass of 'type'.      This case is
         used for class methods; the return value is obj.

       - If it is an instance, it must be an instance of 'type'.  This is
         the normal case; the return value is obj.__class__.

       But... when obj is an instance, we want to allow for the case where
       Py_TYPE(obj) is not a subclass of type, but obj.__class__ is!
       This will allow using super() with a proxy for obj.
    */

    /* Check for first bullet above (special case) */
    if (PyType_Check(obj) && PyType_IsSubtype((PyTypeObject *)obj, type)) {
        return (PyTypeObject *)Py_NewRef(obj);
    }

    /* Normal case */
    if (PyType_IsSubtype(Py_TYPE(obj), type)) {
        return (PyTypeObject*)Py_NewRef(Py_TYPE(obj));
    }
    else {
        /* Try the slow way */
        PyObject *class_attr;

        if (PyObject_GetOptionalAttr(obj, &_Py_ID(__class__), &class_attr) < 0) {
            return NULL;
        }
        if (class_attr != NULL &&
            PyType_Check(class_attr) &&
            (PyTypeObject *)class_attr != Py_TYPE(obj))
        {
            int ok = PyType_IsSubtype(
                (PyTypeObject *)class_attr, type);
            if (ok) {
                return (PyTypeObject *)class_attr;
            }
        }
        Py_XDECREF(class_attr);
    }

    const char *type_or_instance, *obj_str;

    if (PyType_Check(obj)) {
        type_or_instance = "type";
        obj_str = ((PyTypeObject*)obj)->tp_name;
    }
    else {
        type_or_instance = "instance of";
        obj_str = Py_TYPE(obj)->tp_name;
    }

    PyErr_Format(PyExc_TypeError,
                "super(type, obj): obj (%s %.200s) is not "
                "an instance or subtype of type (%.200s).",
                type_or_instance, obj_str, type->tp_name);

    return NULL;
}

PyObject *
_PySuper_Lookup(PyTypeObject *su_type, PyObject *su_obj, PyObject *name, int *method)
{
    PyTypeObject *su_obj_type = supercheck(su_type, su_obj);
    if (su_obj_type == NULL) {
        return NULL;
    }
    PyObject *res = do_super_lookup(NULL, su_type, su_obj, su_obj_type, name, method);
    Py_DECREF(su_obj_type);
    return res;
}

static PyObject *
super_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    superobject *su = superobject_CAST(self);
    superobject *newobj;

    if (obj == NULL || obj == Py_None || su->obj != NULL) {
        /* Not binding to an object, or already bound */
        return Py_NewRef(self);
    }
    if (!Py_IS_TYPE(su, &PySuper_Type))
        /* If su is an instance of a (strict) subclass of super,
           call its type */
        return PyObject_CallFunctionObjArgs((PyObject *)Py_TYPE(su),
                                            su->type, obj, NULL);
    else {
        /* Inline the common case */
        PyTypeObject *obj_type = supercheck(su->type, obj);
        if (obj_type == NULL)
            return NULL;
        newobj = (superobject *)PySuper_Type.tp_new(&PySuper_Type,
                                                 NULL, NULL);
        if (newobj == NULL) {
            Py_DECREF(obj_type);
            return NULL;
        }
        newobj->type = (PyTypeObject*)Py_NewRef(su->type);
        newobj->obj = Py_NewRef(obj);
        newobj->obj_type = obj_type;
        return (PyObject *)newobj;
    }
}

static int
super_init_without_args(_PyInterpreterFrame *cframe, PyTypeObject **type_p,
                        PyObject **obj_p)
{
    *type_p = NULL;
    *obj_p = NULL;
    if (PyErr_Occurred()) {
        return -1;
    }
    PyCodeObject *co = _PyFrame_GetCode(cframe);
    PyObject *co_owner = Py_NewRef((PyObject *)co);
    PyObject *firstarg = NULL;
    PyObject *type_owner = NULL;
    PyObject *error_type_owner = NULL;
    PyObject *error_name_owner = NULL;
    int result = -1;

    if (co->co_argcount == 0) {
        PyErr_SetString(PyExc_RuntimeError, "super(): no arguments");
        goto cleanup;
    }

    assert(_PyFrame_GetCode(cframe)->co_nlocalsplus > 0);
    PyValue firstref = _PyFrame_GetLocalsArray(cframe)[0];
    if (PyValue_IsNull(firstref)) {
        PyErr_SetString(PyExc_RuntimeError, "super(): arg[0] deleted");
        goto cleanup;
    }
    // The first argument might be a cell.
    // "firstarg" is a cell here unless (very unlikely) super()
    // was called from the C-API before the first MAKE_CELL op.
    if ((_PyLocals_GetKind(co->co_localspluskinds, 0) & CO_FAST_CELL) &&
            (_PyInterpreterFrame_LASTI(cframe) >= 0)) {
        // MAKE_CELL and COPY_FREE_VARS have no quickened forms, so no need
        // to use _PyOpcode_Deopt here:
        assert(_PyCode_InstrHandler(co, 0) == MAKE_CELL ||
               _PyCode_InstrHandler(co, 0) == COPY_FREE_VARS);
        assert(PyValue_IsObject(firstref));
        PyObject *cell = PyValue_AsPyObjectBorrow(firstref);
        assert(PyCell_Check(cell));
        firstarg = PyCell_Get(cell);
        if (firstarg == NULL || PyErr_Occurred()) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_RuntimeError, "super(): arg[0] deleted");
            }
            goto cleanup;
        }
    }
    else {
        // The argument may be an inline value, which this boxes.
        firstarg = PyValue_AsPyObjectNewBoxed(firstref);
        if (firstarg == NULL || PyErr_Occurred()) {
            goto cleanup;
        }
    }

    // Look for __class__ in the free vars.
    int i = PyUnstable_Code_GetFirstFree(co);
    for (; i < co->co_nlocalsplus; i++) {
        assert((_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_FREE) != 0);
        PyValue name_value = PyTuple_GET_VALUE(co->co_localsplusnames, i);
        assert(PyValue_IsObject(name_value));
        PyObject *name = PyValue_AsPyObjectBorrow(name_value);
        assert(PyUnicode_CheckExact(name));
        if (_PyUnicode_Equal(name, &_Py_ID(__class__))) {
            PyValue cell_value = _PyFrame_GetLocalsArray(cframe)[i];
            if (!PyValue_IsObject(cell_value) || PyValue_IsNull(cell_value) ||
                    !PyCell_Check(PyValue_AsPyObjectBorrow(cell_value))) {
                PyErr_SetString(PyExc_RuntimeError,
                                "super(): bad __class__ cell");
                goto cleanup;
            }
            PyObject *cell = PyValue_AsPyObjectBorrow(cell_value);
            type_owner = PyCell_Get(cell);
            if (type_owner == NULL || PyErr_Occurred()) {
                if (!PyErr_Occurred()) {
                    PyErr_SetString(PyExc_RuntimeError,
                                    "super(): empty __class__ cell");
                }
                goto cleanup;
            }
            if (!PyType_Check(type_owner)) {
                PyTypeObject *actual_type = Py_TYPE(type_owner);
                error_type_owner = Py_NewRef((PyObject *)actual_type);
                if (_PyType_HasFeature(actual_type, Py_TPFLAGS_HEAPTYPE)) {
                    error_name_owner = Py_NewRef(((PyHeapTypeObject *)actual_type)->ht_name);
                }
                const char *type_name = actual_type->tp_name;
                PyErr_Format(PyExc_RuntimeError,
                             "super(): __class__ is not a type (%s)", type_name);
                goto cleanup;
            }
            break;
        }
    }
    if (type_owner == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "super(): __class__ cell not found");
        goto cleanup;
    }

    result = 0;

cleanup:
    type_mutation_close(&error_name_owner);
    type_mutation_close(&error_type_owner);
    type_mutation_close(&co_owner);
    if (result < 0 || PyErr_Occurred()) {
        type_mutation_close(&type_owner);
        type_mutation_close(&firstarg);
        return -1;
    }
    *type_p = (PyTypeObject *)type_owner;
    *obj_p = firstarg;
    type_owner = NULL;
    firstarg = NULL;
    return 0;
}

static int super_init_impl(PyObject *self, PyTypeObject *type, PyObject *obj);

static int
super_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyTypeObject *type = NULL;
    PyObject *obj = NULL;

    if (!_PyArg_NoKeywords("super", kwds))
        return -1;
    if (!PyArg_ParseTuple(args, "|O!O:super", &PyType_Type, &type, &obj))
        return -1;
    if (super_init_impl(self, type, obj) < 0) {
        return -1;
    }
    return 0;
}

static inline int
super_init_impl(PyObject *self, PyTypeObject *type, PyObject *obj) {
    superobject *su = superobject_CAST(self);
    PyTypeObject *obj_type = NULL;
    if (type == NULL) {
        /* Call super(), without args -- fill in from __class__
           and first local variable on the stack. */
        PyThreadState *tstate = _PyThreadState_GET();
        _PyInterpreterFrame *frame = _PyThreadState_GetFrame(tstate);
        if (frame == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "super(): no current frame");
            return -1;
        }
        int res = super_init_without_args(frame, &type, &obj);

        if (res < 0) {
            return -1;
        }
    }
    else {
        Py_INCREF(type);
        Py_XINCREF(obj);
    }

    if (obj == Py_None) {
        Py_DECREF(obj);
        obj = NULL;
    }
    if (obj != NULL) {
        obj_type = supercheck(type, obj);
        if (obj_type == NULL) {
            Py_DECREF(type);
            Py_DECREF(obj);
            return -1;
        }
    }
    Py_XSETREF(su->type, (PyTypeObject*)type);
    Py_XSETREF(su->obj, obj);
    Py_XSETREF(su->obj_type, obj_type);
    return 0;
}

PyDoc_STRVAR(super_doc,
"super() -> same as super(__class__, <first argument>)\n"
"super(type) -> unbound super object\n"
"super(type, obj) -> bound super object; requires isinstance(obj, type)\n"
"super(type, type2) -> bound super object; requires\n"
"    issubclass(type2, type)\n"
"Typical use to call a cooperative superclass method:\n"
"class C(B):\n"
"    def meth(self, arg):\n"
"        super().meth(arg)\n"
"This works for class methods too:\n"
"class C(B):\n"
"    @classmethod\n"
"    def cmeth(cls, arg):\n"
"        super().cmeth(arg)\n");

static int
super_traverse(PyObject *self, visitproc visit, void *arg)
{
    superobject *su = superobject_CAST(self);

    Py_VISIT(su->obj);
    Py_VISIT(su->type);
    Py_VISIT(su->obj_type);

    return 0;
}

static PyObject *
super_vectorcall(PyObject *self, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(PyType_Check(self));
    if (!_PyArg_NoKwnames("super", kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("super()", nargs, 0, 2)) {
        return NULL;
    }
    PyTypeObject *type = NULL;
    PyObject *obj = NULL;
    PyTypeObject *self_type = (PyTypeObject *)self;
    PyObject *su = self_type->tp_alloc(self_type, 0);
    if (su == NULL) {
        return NULL;
    }
    // 1 or 2 argument form super().
    if (nargs != 0) {
        PyObject *arg0 = args[0];
        if (!PyType_Check(arg0)) {
            PyErr_Format(PyExc_TypeError,
                "super() argument 1 must be a type, not %.200s", Py_TYPE(arg0)->tp_name);
            goto fail;
        }
        type = (PyTypeObject *)arg0;
    }
    if (nargs == 2) {
        obj = args[1];
    }
    if (super_init_impl(su, type, obj) < 0) {
        goto fail;
    }
    return su;
fail:
    Py_DECREF(su);
    return NULL;
}

PyTypeObject PySuper_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "super",                                    /* tp_name */
    sizeof(superobject),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    super_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    super_repr,                                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    super_getattro,                             /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    super_doc,                                  /* tp_doc */
    super_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    super_members,                              /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    super_descr_get,                            /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    super_init,                                 /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    .tp_vectorcall = super_vectorcall,
};
