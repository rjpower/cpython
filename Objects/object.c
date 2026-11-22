
/* Generic object operations; and implementation of None */

/* This file holds the rich comparison, truth and string dispatchers, so it
   is an adapter unit: Tools/build/check_value_api.py does not list its
   slot-field reads. */
#define Py_VALUE_ADAPTER 1

#include "Python.h"
#include "pycore_brc.h"           // _Py_brc_queue_object()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_context.h"       // _PyContextTokenMissing_Type
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // _PyObject_MaterializeManagedDict()
#include "pycore_floatobject.h"   // _PyFloat_DebugMallocStats()
#include "pycore_function.h"      // _PyClassMethod_GetFunc()
#include "pycore_freelist.h"      // _PyObject_ClearFreeLists()
#include "pycore_genobject.h"     // _PyAsyncGenAThrow_Type
#include "pycore_hamt.h"          // _PyHamtItems_Type
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_instruction_sequence.h" // _PyInstructionSequence_Type
#include "pycore_interpframe.h"   // _PyFrame_Stackbase()
#include "pycore_interpolation.h" // _PyInterpolation_Type
#include "pycore_lazyimportobject.h" // PyLazyImport_Type
#include "pycore_list.h"          // _PyList_DebugMallocStats()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_long_value.h"    // _PyLong_WordOperands()
#include "pycore_memoryobject.h"  // _PyManagedBuffer_Type
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_object.h"        // export _Py_SwappedOp
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_symtable.h"      // PySTEntry_Type
#include "pycore_template.h"      // _PyTemplate_Type _PyTemplateIter_Type
#include "pycore_tuple.h"         // _PyTuple_DebugMallocStats()
#include "pycore_typecache.h"     // _PyTypeCache_Lookup()
#include "pycore_typeobject.h"    // _PyBufferWrapper_Type
#include "pycore_typevarobject.h" // _PyTypeAlias_Type
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()
#include "pycore_value.h"         // PyValue_FromObjectSteal
#include "pycore_unionobject.h"   // _PyUnion_Type
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_value_ops.h"     // _PyValue_RichCompare()
#include "pycore_value_slots.h"   // _PyValue_AdapterBox()


#ifdef Py_LIMITED_API
   // Prevent recursive call _Py_IncRef() <=> Py_INCREF()
#  error "Py_LIMITED_API macro must not be defined"
#endif

/* Defined in tracemalloc.c */
extern void _PyMem_DumpTraceback(int fd, const void *ptr);


int
_PyObject_CheckConsistency(PyObject *op, int check_content)
{
#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    CHECK(!_PyObject_IsFreed(op));
    CHECK(Py_REFCNT(op) >= 1);

    _PyType_CheckConsistency(Py_TYPE(op));

    if (PyUnicode_Check(op)) {
        _PyUnicode_CheckConsistency(op, check_content);
    }
    else if (PyAnyDict_Check(op)) {
        _PyDict_CheckConsistency(op, check_content);
    }
    return 1;

#undef CHECK
}


#ifdef Py_REF_DEBUG
/* We keep the legacy symbol around for backward compatibility. */
Py_ssize_t _Py_RefTotal;

static inline Py_ssize_t
get_legacy_reftotal(void)
{
    return _Py_RefTotal;
}
#endif

#ifdef Py_REF_DEBUG

#  define REFTOTAL(interp) \
    interp->object_state.reftotal

static inline void
reftotal_add(PyThreadState *tstate, Py_ssize_t n)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    // relaxed store to avoid data race with read in get_reftotal()
    Py_ssize_t reftotal = tstate_impl->reftotal + n;
    _Py_atomic_store_ssize_relaxed(&tstate_impl->reftotal, reftotal);
#else
    REFTOTAL(tstate->interp) += n;
#endif
}

static inline Py_ssize_t get_global_reftotal(_PyRuntimeState *);

/* We preserve the number of refs leaked during runtime finalization,
   so they can be reported if the runtime is initialized again. */
// XXX We don't lose any information by dropping this,
// so we should consider doing so.
static Py_ssize_t last_final_reftotal = 0;

void
_Py_FinalizeRefTotal(_PyRuntimeState *runtime)
{
    last_final_reftotal = get_global_reftotal(runtime);
    runtime->object_state.interpreter_leaks = 0;
}

void
_PyInterpreterState_FinalizeRefTotal(PyInterpreterState *interp)
{
    interp->runtime->object_state.interpreter_leaks += REFTOTAL(interp);
    REFTOTAL(interp) = 0;
}

static inline Py_ssize_t
get_reftotal(PyInterpreterState *interp)
{
    /* For a single interpreter, we ignore the legacy _Py_RefTotal,
       since we can't determine which interpreter updated it. */
    Py_ssize_t total = REFTOTAL(interp);
#ifdef Py_GIL_DISABLED
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
        /* This may race with other threads modifications to their reftotal */
        _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)p;
        total += _Py_atomic_load_ssize_relaxed(&tstate_impl->reftotal);
    }
#endif
    return total;
}

static inline Py_ssize_t
get_global_reftotal(_PyRuntimeState *runtime)
{
    Py_ssize_t total = 0;

    /* Add up the total from each interpreter. */
    HEAD_LOCK(&_PyRuntime);
    PyInterpreterState *interp = PyInterpreterState_Head();
    for (; interp != NULL; interp = PyInterpreterState_Next(interp)) {
        total += get_reftotal(interp);
    }
    HEAD_UNLOCK(&_PyRuntime);

    /* Add in the updated value from the legacy _Py_RefTotal. */
    total += get_legacy_reftotal();
    total += last_final_reftotal;
    total += runtime->object_state.interpreter_leaks;

    return total;
}

#undef REFTOTAL

void
_PyDebug_PrintTotalRefs(void) {
    _PyRuntimeState *runtime = &_PyRuntime;
    fprintf(stderr,
            "[%zd refs, %zd blocks]\n",
            get_global_reftotal(runtime), _Py_GetGlobalAllocatedBlocks());
    /* It may be helpful to also print the "legacy" reftotal separately.
       Likewise for the total for each interpreter. */
}
#endif /* Py_REF_DEBUG */

/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros.
   These are used by the individual routines for object creation.
   Do not call them otherwise, they do not initialize the object! */

#ifdef Py_TRACE_REFS

#define REFCHAIN(interp) interp->object_state.refchain
#define REFCHAIN_VALUE ((void*)(uintptr_t)1)

static inline int
has_own_refchain(PyInterpreterState *interp)
{
    if (interp->feature_flags & Py_RTFLAGS_USE_MAIN_OBMALLOC) {
        return (_Py_IsMainInterpreter(interp)
            || _PyInterpreterState_Main() == NULL);
    }
    return 1;
}

static int
refchain_init(PyInterpreterState *interp)
{
    if (!has_own_refchain(interp)) {
        // Legacy subinterpreters share a refchain with the main interpreter.
        REFCHAIN(interp) = REFCHAIN(_PyInterpreterState_Main());
        return 0;
    }
    _Py_hashtable_allocator_t alloc = {
        // Use directly malloc() and free() of the C library. Using
        // PyMem_RawMalloc() and PyMem_RawFree() prevents testing
        // _testcapi.set_nomemory().
        .malloc = malloc,
        .free = free,
    };
    REFCHAIN(interp) = _Py_hashtable_new_full(
        _Py_hashtable_hash_ptr, _Py_hashtable_compare_direct,
        NULL, NULL, &alloc);
    if (REFCHAIN(interp) == NULL) {
        return -1;
    }
    return 0;
}

static void
refchain_fini(PyInterpreterState *interp)
{
    if (has_own_refchain(interp) && REFCHAIN(interp) != NULL) {
        _Py_hashtable_destroy(REFCHAIN(interp));
    }
    REFCHAIN(interp) = NULL;
}

bool
_PyRefchain_IsTraced(PyInterpreterState *interp, PyObject *obj)
{
    return (_Py_hashtable_get(REFCHAIN(interp), obj) == REFCHAIN_VALUE);
}


static void
_PyRefchain_Trace(PyInterpreterState *interp, PyObject *obj)
{
    if (_Py_hashtable_set(REFCHAIN(interp), obj, REFCHAIN_VALUE) < 0) {
        // Use a fatal error because _Py_NewReference() cannot report
        // the error to the caller.
        Py_FatalError("_Py_hashtable_set() memory allocation failed");
    }
}


static void
_PyRefchain_Remove(PyInterpreterState *interp, PyObject *obj)
{
    void *value = _Py_hashtable_steal(REFCHAIN(interp), obj);
#ifndef NDEBUG
    assert(value == REFCHAIN_VALUE);
#else
    (void)value;
#endif
}


/* Add an object to the refchain hash table.
 *
 * Note that objects are normally added to the list by PyObject_Init()
 * indirectly.  Not all objects are initialized that way, though; exceptions
 * include statically allocated type objects, and statically allocated
 * singletons (like Py_True and Py_None). */
void
_Py_AddToAllObjects(PyObject *op)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!_PyRefchain_IsTraced(interp, op)) {
        _PyRefchain_Trace(interp, op);
    }
}
#endif  /* Py_TRACE_REFS */

#ifdef Py_REF_DEBUG
/* Log a fatal error; doesn't return. */
void
_Py_NegativeRefcount(const char *filename, int lineno, PyObject *op)
{
    _PyObject_AssertFailed(op, NULL, "object has negative ref count",
                           filename, lineno, __func__);
}

/* This is used strictly by Py_INCREF(). */
void
_Py_INCREF_IncRefTotal(void)
{
    reftotal_add(_PyThreadState_GET(), 1);
}

/* This is used strictly by Py_DECREF(). */
void
_Py_DECREF_DecRefTotal(void)
{
    reftotal_add(_PyThreadState_GET(), -1);
}

void
_Py_IncRefTotal(PyThreadState *tstate)
{
    reftotal_add(tstate, 1);
}

void
_Py_DecRefTotal(PyThreadState *tstate)
{
    reftotal_add(tstate, -1);
}

void
_Py_AddRefTotal(PyThreadState *tstate, Py_ssize_t n)
{
    reftotal_add(tstate, n);
}

/* This includes the legacy total
   and any carried over from the last runtime init/fini cycle. */
Py_ssize_t
_Py_GetGlobalRefTotal(void)
{
    return get_global_reftotal(&_PyRuntime);
}

Py_ssize_t
_Py_GetLegacyRefTotal(void)
{
    return get_legacy_reftotal();
}

Py_ssize_t
_PyInterpreterState_GetRefTotal(PyInterpreterState *interp)
{
    HEAD_LOCK(&_PyRuntime);
    Py_ssize_t total = get_reftotal(interp);
    HEAD_UNLOCK(&_PyRuntime);
    return total;
}

#endif /* Py_REF_DEBUG */

void
Py_IncRef(PyObject *o)
{
    Py_XINCREF(o);
}

void
Py_DecRef(PyObject *o)
{
    Py_XDECREF(o);
}

void
_Py_IncRef(PyObject *o)
{
    Py_INCREF(o);
}

void
_Py_DecRef(PyObject *o)
{
    Py_DECREF(o);
}

#ifdef Py_GIL_DISABLED
# ifdef Py_REF_DEBUG
static int
is_dead(PyObject *o)
{
#  if SIZEOF_SIZE_T == 8
    return (uintptr_t)o->ob_type == 0xDDDDDDDDDDDDDDDD;
#  else
    return (uintptr_t)o->ob_type == 0xDDDDDDDD;
#  endif
}
# endif

// Decrement the shared reference count of an object. Return 1 if the object
// is dead and should be deallocated, 0 otherwise.
static int
_Py_DecRefSharedIsDead(PyObject *o, const char *filename, int lineno)
{
    // Should we queue the object for the owning thread to merge?
    int should_queue;

    Py_ssize_t new_shared;
    Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&o->ob_ref_shared);
    do {
        should_queue = (shared == 0 || shared == _Py_REF_MAYBE_WEAKREF);

        if (should_queue) {
            // If the object had refcount zero, not queued, and not merged,
            // then we enqueue the object to be merged by the owning thread.
            // In this case, we don't subtract one from the reference count
            // because the queue holds a reference.
            new_shared = _Py_REF_QUEUED;
        }
        else {
            // Otherwise, subtract one from the reference count. This might
            // be negative!
            new_shared = shared - (1 << _Py_REF_SHARED_SHIFT);
        }

#ifdef Py_REF_DEBUG
        if ((new_shared < 0 && _Py_REF_IS_MERGED(new_shared)) ||
            (should_queue && is_dead(o)))
        {
            _Py_NegativeRefcount(filename, lineno, o);
        }
#endif
    } while (!_Py_atomic_compare_exchange_ssize(&o->ob_ref_shared,
                                                &shared, new_shared));

    if (should_queue) {
#ifdef Py_REF_DEBUG
        _Py_IncRefTotal(_PyThreadState_GET());
#endif
        _Py_brc_queue_object(o);
    }
    else if (new_shared == _Py_REF_MERGED) {
        // refcount is zero AND merged
        return 1;
    }
    return 0;
}

void
_Py_DecRefSharedDebug(PyObject *o, const char *filename, int lineno)
{
    if (_Py_DecRefSharedIsDead(o, filename, lineno)) {
        _Py_Dealloc(o);
    }
}

void
_Py_DecRefShared(PyObject *o)
{
    _Py_DecRefSharedDebug(o, NULL, 0);
}

void
_Py_MergeZeroLocalRefcount(PyObject *op)
{
    assert(op->ob_ref_local == 0);

    Py_ssize_t shared = _Py_atomic_load_ssize_acquire(&op->ob_ref_shared);
    if (shared == 0) {
        // Fast-path: shared refcount is zero (including flags)
        _Py_Dealloc(op);
        return;
    }

    // gh-121794: This must be before the store to `ob_ref_shared` (gh-119999),
    // but should outside the fast-path to maintain the invariant that
    // a zero `ob_tid` implies a merged refcount.
    _Py_atomic_store_uintptr_relaxed(&op->ob_tid, 0);

    // Slow-path: atomically set the flags (low two bits) to _Py_REF_MERGED.
    Py_ssize_t new_shared;
    do {
        new_shared = (shared & ~_Py_REF_SHARED_FLAG_MASK) | _Py_REF_MERGED;
    } while (!_Py_atomic_compare_exchange_ssize(&op->ob_ref_shared,
                                                &shared, new_shared));

    if (new_shared == _Py_REF_MERGED) {
        // i.e., the shared refcount is zero (only the flags are set) so we
        // deallocate the object.
        _Py_Dealloc(op);
    }
}

Py_ssize_t
_Py_ExplicitMergeRefcount(PyObject *op, Py_ssize_t extra)
{
    assert(!_Py_IsImmortal(op));

#ifdef Py_REF_DEBUG
    _Py_AddRefTotal(_PyThreadState_GET(), extra);
#endif

    // gh-119999: Write to ob_ref_local and ob_tid before merging the refcount.
    Py_ssize_t local = (Py_ssize_t)op->ob_ref_local;
    _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, 0);
    _Py_atomic_store_uintptr_relaxed(&op->ob_tid, 0);

    Py_ssize_t refcnt;
    Py_ssize_t new_shared;
    Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&op->ob_ref_shared);
    do {
        refcnt = Py_ARITHMETIC_RIGHT_SHIFT(Py_ssize_t, shared, _Py_REF_SHARED_SHIFT);
        refcnt += local;
        refcnt += extra;

        new_shared = _Py_REF_SHARED(refcnt, _Py_REF_MERGED);
    } while (!_Py_atomic_compare_exchange_ssize(&op->ob_ref_shared,
                                                &shared, new_shared));
    return refcnt;
}

// The more complicated "slow" path for undoing the resurrection of an object.
int
_PyObject_ResurrectEndSlow(PyObject *op)
{
    if (_Py_IsImmortal(op)) {
        return 1;
    }
    if (_Py_IsOwnedByCurrentThread(op)) {
        // If the object is owned by the current thread, give up ownership and
        // merge the refcount. This isn't necessary in all cases, but it
        // simplifies the implementation.
        Py_ssize_t refcount = _Py_ExplicitMergeRefcount(op, -1);
        if (refcount == 0) {
#ifdef Py_TRACE_REFS
            _Py_ForgetReference(op);
#endif
            return 0;
        }
        return 1;
    }
    int is_dead = _Py_DecRefSharedIsDead(op, NULL, 0);
    if (is_dead) {
#ifdef Py_TRACE_REFS
        _Py_ForgetReference(op);
#endif
        return 0;
    }
    return 1;
}


#endif  /* Py_GIL_DISABLED */


/**************************************/

PyObject *
PyObject_Init(PyObject *op, PyTypeObject *tp)
{
    if (op == NULL) {
        return PyErr_NoMemory();
    }

    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
PyObject_InitVar(PyVarObject *op, PyTypeObject *tp, Py_ssize_t size)
{
    if (op == NULL) {
        return (PyVarObject *) PyErr_NoMemory();
    }

    _PyObject_InitVar(op, tp, size);
    return op;
}

PyObject *
_PyObject_New(PyTypeObject *tp)
{
    PyObject *op = (PyObject *) PyObject_Malloc(_PyObject_SIZE(tp));
    if (op == NULL) {
        return PyErr_NoMemory();
    }
    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
_PyObject_NewVar(PyTypeObject *tp, Py_ssize_t nitems)
{
    PyVarObject *op;
    const size_t size = _PyObject_VAR_SIZE(tp, nitems);
    op = (PyVarObject *) PyObject_Malloc(size);
    if (op == NULL) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    _PyObject_InitVar(op, tp, nitems);
    return op;
}

void
PyObject_CallFinalizer(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    if (tp->tp_finalize == NULL)
        return;
    /* tp_finalize should only be called once. */
    if (_PyType_IS_GC(tp) && _PyGC_FINALIZED(self))
        return;

    tp->tp_finalize(self);
    if (_PyType_IS_GC(tp)) {
        _PyGC_SET_FINALIZED(self);
    }
}

int
PyObject_CallFinalizerFromDealloc(PyObject *self)
{
    if (Py_REFCNT(self) != 0) {
        _PyObject_ASSERT_FAILED_MSG(self,
                                    "PyObject_CallFinalizerFromDealloc called "
                                    "on object with a non-zero refcount");
    }

    /* Temporarily resurrect the object. */
    _PyObject_ResurrectStart(self);

    PyObject_CallFinalizer(self);

    _PyObject_ASSERT_WITH_MSG(self,
                              Py_REFCNT(self) > 0,
                              "refcount is too small");

    _PyObject_ASSERT(self,
                    (!_PyType_IS_GC(Py_TYPE(self))
                    || _PyObject_GC_IS_TRACKED(self)));

    /* Undo the temporary resurrection; can't use DECREF here, it would
     * cause a recursive call. */
    if (_PyObject_ResurrectEnd(self)) {
        /* tp_finalize resurrected it!
           gh-130202: Note that the object may still be dead in the free
           threaded build in some circumstances, so it's not safe to access
           `self` after this point. For example, the last reference to the
           resurrected `self` may be held by another thread, which can
           concurrently deallocate it. */
        return -1;
    }

    /* this is the normal path out, the caller continues with deallocation. */
    return 0;
}

int
PyObject_Print(PyObject *op, FILE *fp, int flags)
{
    int ret = 0;
    int write_error = 0;
    if (PyErr_CheckSignals())
        return -1;
    if (_Py_EnterRecursiveCall(" printing an object")) {
        return -1;
    }
    clearerr(fp); /* Clear any previous error condition */
    if (op == NULL) {
        Py_BEGIN_ALLOW_THREADS
        fprintf(fp, "<nil>");
        Py_END_ALLOW_THREADS
    }
    else {
        if (Py_REFCNT(op) <= 0) {
            Py_BEGIN_ALLOW_THREADS
            fprintf(fp, "<refcnt %zd at %p>", Py_REFCNT(op), (void *)op);
            Py_END_ALLOW_THREADS
        }
        else {
            PyObject *s;
            if (flags & Py_PRINT_RAW)
                s = _PyValue_Str(PyValue_FromObjectBorrow(op));
            else
                s = _PyValue_Repr(PyValue_FromObjectBorrow(op));
            if (s == NULL) {
                ret = -1;
            }
            else {
                assert(PyUnicode_Check(s));
                const char *t;
                Py_ssize_t len;
                t = PyUnicode_AsUTF8AndSize(s, &len);
                if (t == NULL) {
                    ret = -1;
                }
                else {
                    /* Versions of Android and OpenBSD from before 2023 fail to
                       set the `ferror` indicator when writing to a read-only
                       stream, so we need to check the return value.
                       (https://github.com/openbsd/src/commit/fc99cf9338942ecd9adc94ea08bf6188f0428c15) */
                    if (fwrite(t, 1, len, fp) != (size_t)len) {
                        write_error = 1;
                    }
                }
                Py_DECREF(s);
            }
        }
    }
    if (ret == 0) {
        if (write_error || ferror(fp)) {
            PyErr_SetFromErrno(PyExc_OSError);
            clearerr(fp);
            ret = -1;
        }
    }

    _Py_LeaveRecursiveCall();
    return ret;
}

/* For debugging convenience.  Set a breakpoint here and call it from your DLL */
void
_Py_BreakPoint(void)
{
}


/* Heuristic checking if the object memory is uninitialized or deallocated.
   Rely on the debug hooks on Python memory allocators:
   see _PyMem_IsPtrFreed().

   The function can be used to prevent segmentation fault on dereferencing
   pointers like 0xDDDDDDDDDDDDDDDD. */
int
_PyObject_IsFreed(PyObject *op)
{
    if (_PyMem_IsPtrFreed(op) || _PyMem_IsPtrFreed(Py_TYPE(op))) {
        return 1;
    }
    return 0;
}


/* For debugging convenience. */
void
PyObject_Dump(PyObject* op)
{
    if (_PyObject_IsFreed(op)) {
        /* It seems like the object memory has been freed:
           don't access it to prevent a segmentation fault. */
        fprintf(stderr, "<object at %p is freed>\n", op);
        fflush(stderr);
        return;
    }

    /* first, write fields which are the least likely to crash */
    fprintf(stderr, "object address  : %p\n", (void *)op);
    fprintf(stderr, "object refcount : %zd\n", Py_REFCNT(op));
    fflush(stderr);

    PyTypeObject *type = Py_TYPE(op);
    fprintf(stderr, "object type     : %p\n", type);
    fprintf(stderr, "object type name: %s\n",
            type==NULL ? "NULL" : type->tp_name);

    /* the most dangerous part */
    fprintf(stderr, "object repr     : ");
    fflush(stderr);

    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *exc = PyErr_GetRaisedException();

    (void)PyObject_Print(op, stderr, 0);
    fflush(stderr);

    PyErr_SetRaisedException(exc);
    PyGILState_Release(gil);

    fprintf(stderr, "\n");
    fflush(stderr);
}

/* The string protocol on values (pyjit/design/proposals/value-api-v2a0.md,
   section 4.6). As in Objects/abstract.c, a dispatcher calls the value slot
   of a row if it is set, else the object slot through the adapter, and the
   object function is its thunk. */

/* The adapters of this file (D9): the cold path of a dispatcher whose value
   field is 0. Each takes the object slot that the dispatcher loaded, boxes
   an inline operand and calls that slot; none reads a type again. */

Py_NO_INLINE static PyObject *
adapt_repr(reprfunc slot, PyValueBorrow v)
{
    PyObject *a = _PyValue_AdapterBox(v, BOX_SITE_REPR, slot);
    if (a == NULL) {
        return NULL;
    }
    PyObject *res = slot(a);
    _PyValue_AdapterRelease(v, a);
    return res;
}

PyObject *
_PyValue_CallRepr(PyTypeObject *t, reprfunc slot, int row, PyValueBorrow v)
{
    _Py_reprvaluefunc f = (_Py_reprvaluefunc)_PyType_VALUE_FIELD_AT(t, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, row);
        return f(v);
    }
    return adapt_repr(slot, v);
}

PyObject *
(_PyValue_Repr)(PyValueBorrow v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
    if (PyValue_IsNull(v))
        return PyUnicode_FromString("<NULL>");
    PyTypeObject *tp = PyValue_TYPE(v);
    if (tp->tp_repr == NULL) {
        // An inline value's type has a repr, so v holds an object.
        return PyUnicode_FromFormat("<%s object at %p>",
                                    tp->tp_name,
                                    PyValue_AsKnownObjectBorrow(v));
    }

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyObject_Repr() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* It is possible for a type to have a tp_repr representation that loops
       infinitely. */
    if (_Py_EnterRecursiveCallTstate(tstate,
                                     " while getting the repr of an object")) {
        return NULL;
    }
    res = _PyValue_CallRepr(tp, tp->tp_repr, _Py_VALUE_SLOT_INDEX_repr, v);
    _Py_LeaveRecursiveCallTstate(tstate);

    if (res == NULL) {
        return NULL;
    }
    if (!PyUnicode_Check(res)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%N.__repr__() must return a str, not %T", tp, res);
        Py_DECREF(res);
        return NULL;
    }
    return res;
}

PyObject *
PyObject_Repr(PyObject *v)
{
    return (_PyValue_Repr)(_Py_VALUE_ARG(v));
}

PyObject *
(_PyValue_Str)(PyValueBorrow v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
    if (PyValue_IsNull(v))
        return PyUnicode_FromString("<NULL>");
    PyTypeObject *tp = PyValue_TYPE(v);
    if (tp == &PyUnicode_Type) {
        return Py_NewRef(PyValue_AsKnownObjectBorrow(v));
    }
    if (tp->tp_str == NULL)
        return (_PyValue_Repr)(v);

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyObject_Str() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* It is possible for a type to have a tp_str representation that loops
       infinitely. */
    if (_Py_EnterRecursiveCallTstate(tstate, " while getting the str of an object")) {
        return NULL;
    }
    res = _PyValue_CallRepr(tp, tp->tp_str, _Py_VALUE_SLOT_INDEX_str, v);
    _Py_LeaveRecursiveCallTstate(tstate);

    if (res == NULL) {
        return NULL;
    }
    if (!PyUnicode_Check(res)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%N.__str__() must return a str, not %T", tp, res);
        Py_DECREF(res);
        return NULL;
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}

PyObject *
PyObject_Str(PyObject *v)
{
    return (_PyValue_Str)(_Py_VALUE_ARG(v));
}

PyObject *
(_PyValue_ASCII)(PyValueBorrow v)
{
    PyObject *repr, *ascii, *res;

    repr = (_PyValue_Repr)(v);
    if (repr == NULL)
        return NULL;

    if (PyUnicode_IS_ASCII(repr))
        return repr;

    /* repr is guaranteed to be a PyUnicode object by _PyValue_Repr */
    ascii = _PyUnicode_AsASCIIString(repr, "backslashreplace");
    Py_DECREF(repr);
    if (ascii == NULL)
        return NULL;

    res = PyUnicode_DecodeASCII(
        PyBytes_AS_STRING(ascii),
        PyBytes_GET_SIZE(ascii),
        NULL);

    Py_DECREF(ascii);
    return res;
}

PyObject *
PyObject_ASCII(PyObject *v)
{
    return (_PyValue_ASCII)(_Py_VALUE_ARG(v));
}

PyObject *
PyObject_Bytes(PyObject *v)
{
    PyObject *result, *func;

    if (v == NULL)
        return PyBytes_FromString("<NULL>");

    if (PyBytes_CheckExact(v)) {
        return Py_NewRef(v);
    }

    func = _PyObject_LookupSpecial(v, &_Py_ID(__bytes__));
    if (func != NULL) {
        result = _PyObject_CallNoArgs(func);
        Py_DECREF(func);
        if (result == NULL)
            return NULL;
        if (!PyBytes_Check(result)) {
            PyErr_Format(PyExc_TypeError,
                         "%T.__bytes__() must return a bytes, not %T",
                         v, result);
            Py_DECREF(result);
            return NULL;
        }
        return result;
    }
    else if (PyErr_Occurred())
        return NULL;
    return PyBytes_FromObject(v);
}

static void
clear_freelist(struct _Py_freelist *freelist, int is_finalization,
               freefunc dofree)
{
    void *ptr;
    while ((ptr = _PyFreeList_PopNoStats(freelist)) != NULL) {
        dofree(ptr);
    }
    assert(freelist->size == 0 || freelist->size == -1);
    assert(freelist->freelist == NULL);
    if (is_finalization) {
        freelist->size = -1;
    }
}

static void
free_object(void *obj)
{
    PyObject *op = (PyObject *)obj;
    PyTypeObject *tp = Py_TYPE(op);
    tp->tp_free(op);
    Py_DECREF(tp);
}

void
_PyObject_ClearFreeLists(struct _Py_freelists *freelists, int is_finalization)
{
    // In the free-threaded build, freelists are per-PyThreadState and cleared in PyThreadState_Clear()
    // In the default build, freelists are per-interpreter and cleared in finalize_interp_types()
    clear_freelist(&freelists->floats, is_finalization, free_object);
    clear_freelist(&freelists->complexes, is_finalization, free_object);
    for (Py_ssize_t i = 0; i < PyTuple_MAXSAVESIZE; i++) {
        clear_freelist(&freelists->tuples[i], is_finalization, free_object);
    }
    clear_freelist(&freelists->lists, is_finalization, free_object);
    clear_freelist(&freelists->list_iters, is_finalization, free_object);
    clear_freelist(&freelists->tuple_iters, is_finalization, free_object);
    clear_freelist(&freelists->dicts, is_finalization, free_object);
    clear_freelist(&freelists->dictkeys, is_finalization, PyMem_Free);
    clear_freelist(&freelists->slices, is_finalization, free_object);
    clear_freelist(&freelists->ranges, is_finalization, free_object);
    clear_freelist(&freelists->range_iters, is_finalization, free_object);
    clear_freelist(&freelists->contexts, is_finalization, free_object);
    clear_freelist(&freelists->async_gens, is_finalization, free_object);
    clear_freelist(&freelists->async_gen_asends, is_finalization, free_object);
    clear_freelist(&freelists->futureiters, is_finalization, free_object);
    if (is_finalization) {
        // Only clear object stack chunks during finalization. We use object
        // stacks during GC, so emptying the free-list is counterproductive.
        clear_freelist(&freelists->object_stack_chunks, 1, PyMem_RawFree);
    }
    clear_freelist(&freelists->unicode_writers, is_finalization, PyMem_Free);
    clear_freelist(&freelists->bytes_writers, is_finalization, PyMem_Free);
    clear_freelist(&freelists->ints, is_finalization, free_object);
    clear_freelist(&freelists->pycfunctionobject, is_finalization, PyObject_GC_Del);
    clear_freelist(&freelists->pycmethodobject, is_finalization, PyObject_GC_Del);
    clear_freelist(&freelists->pymethodobjects, is_finalization, free_object);
}

/*
def _PyObject_FunctionStr(x):
    try:
        qualname = x.__qualname__
    except AttributeError:
        return str(x)
    try:
        mod = x.__module__
        if mod is not None and mod != 'builtins':
            return f"{x.__module__}.{qualname}()"
    except AttributeError:
        pass
    return qualname
*/
PyObject *
_PyObject_FunctionStr(PyObject *x)
{
    assert(!PyErr_Occurred());
    PyObject *qualname;
    int ret = PyObject_GetOptionalAttr(x, &_Py_ID(__qualname__), &qualname);
    if (qualname == NULL) {
        if (ret < 0) {
            return NULL;
        }
        return _PyValue_Str(PyValue_FromObjectBorrow(x));
    }
    PyObject *module;
    PyObject *result = NULL;
    ret = PyObject_GetOptionalAttr(x, &_Py_ID(__module__), &module);
    if (module != NULL && module != Py_None) {
        ret = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(module), _PyValue_FromObjectKnown(&_Py_ID(builtins)), Py_NE);
        if (ret < 0) {
            // error
            goto done;
        }
        if (ret > 0) {
            result = PyUnicode_FromFormat("%S.%S()", module, qualname);
            goto done;
        }
    }
    else if (ret < 0) {
        goto done;
    }
    result = PyUnicode_FromFormat("%S()", qualname);
done:
    Py_DECREF(qualname);
    Py_XDECREF(module);
    return result;
}

/* For Python 3.0.1 and later, the old three-way comparison has been
   completely removed in favour of rich comparisons.  PyObject_Compare() and
   PyObject_Cmp() are gone, and the builtin cmp function no longer exists.
   The old tp_compare slot has been renamed to tp_as_async, and should no
   longer be used.  Use tp_richcompare instead.

   See (*) below for practical amendments.

   tp_richcompare gets called with a first argument of the appropriate type
   and a second object of an arbitrary type.  We never do any kind of
   coercion.

   The tp_richcompare slot should return an object, as follows:

    NULL if an exception occurred
    NotImplemented if the requested comparison is not implemented
    any other false value if the requested comparison is false
    any other true value if the requested comparison is true

  The PyObject_RichCompare[Bool]() wrappers raise TypeError when they get
  NotImplemented.

  (*) Practical amendments:

  - If rich comparison returns NotImplemented, == and != are decided by
    comparing the object pointer (i.e. falling back to the base object
    implementation).

*/

/* Map rich comparison operators to their swapped version, e.g. LT <--> GT */
int _Py_SwappedOp[] = {Py_GT, Py_GE, Py_EQ, Py_NE, Py_LT, Py_LE};

static const char * const opstrings[] = {"<", "<=", "==", "!=", ">", ">="};

/* The rich comparison on values, with the order and defaults of
   do_richcompare(); value_ops.c's _PyValue_RichCompareBool() returns its
   truth. */

Py_NO_INLINE static PyValue
adapt_richcompare(richcmpfunc slot, PyValueBorrow v, PyValueBorrow w, int op)
{
    PyObject *a = _PyValue_AdapterBox(v, BOX_SITE_COMPARE, slot);
    if (a == NULL) {
        return PyValue_NULL;
    }
    PyObject *b = _PyValue_AdapterBox(w, BOX_SITE_COMPARE, slot);
    if (b == NULL) {
        _PyValue_AdapterRelease(v, a);
        return PyValue_NULL;
    }
    PyObject *res = slot(a, b, op);
    _PyValue_AdapterRelease(w, b);
    _PyValue_AdapterRelease(v, a);
    return _PyValue_FromObjectResult(res);
}

PyValue
_PyValue_CallRichCompare(PyTypeObject *t, richcmpfunc slot,
                         PyValueBorrow v, PyValueBorrow w, int op)
{
    int row = _Py_VALUE_SLOT_INDEX_richcompare;
    _Py_richcmpvaluefunc f =
        (_Py_richcmpvaluefunc)_PyType_VALUE_FIELD_AT(t, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, row);
        return f(v, w, op);
    }
    return adapt_richcompare(slot, v, w, op);
}

/* The type of an operand: one test of the pointer space, then the cold
   path for an inline int or float. do_richcompare() reads it at each step. */
static inline PyTypeObject *
richcompare_type(PyValueBorrow v)
{
    if (PyValue_IsPointerSpace(v)) {
        return Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    }
    return _PyValue_TypeNotPointer(v);
}

/* Perform a rich comparison, raising TypeError when the requested comparison
   operator is not supported. As upstream, each step reads the types of the
   operands again: a slot that an earlier step called can run Python code
   that assigns __class__ or replaces a slot. NotImplemented is immortal, so
   the results that are dropped own nothing. */
static PyValue
do_richcompare(PyValueBorrow v, PyValueBorrow w, int op)
{
    int checked_reverse_op = 0;

    /* The steps: the reflected comparison of a subtype, v's, then w's. One
       call site serves the three. */
    for (int step = 0; step < 3; step++) {
        PyTypeObject *tv = richcompare_type(v);
        PyTypeObject *tw = richcompare_type(w);
        PyTypeObject *t;
        PyValueBorrow a, b;
        int aop;
        if (step == 0) {
            if (tv == tw || !PyType_IsSubtype(tw, tv)) {
                continue;
            }
            t = tw; a = w; b = v; aop = _Py_SwappedOp[op];
        }
        else if (step == 1) {
            t = tv; a = v; b = w; aop = op;
        }
        else {
            if (checked_reverse_op) {
                continue;
            }
            t = tw; a = w; b = v; aop = _Py_SwappedOp[op];
        }
        richcmpfunc f = t->tp_richcompare;
        if (f == NULL) {
            continue;
        }
        if (step == 0) {
            checked_reverse_op = 1;
        }
        PyValue res = _PyValue_CallRichCompare(t, f, a, b, aop);
        if (!PyValue_Is(res, PyValue_NotImplemented)) {
            return res;
        }
        assert(!_PyValue_OwnsCountNotNull(res));
    }
    /* If neither object implements it, provide a sensible default
       for == and !=, but raise an exception for ordering. */
    switch (op) {
    case Py_EQ:
        return PyValue_Is(v, w) ? PyValue_True : PyValue_False;
    case Py_NE:
        return PyValue_Is(v, w) ? PyValue_False : PyValue_True;
    default:
        PyErr_Format(PyExc_TypeError,
                     "'%s' not supported between instances of '%.100s' and '%.100s'",
                     opstrings[op],
                     richcompare_type(v)->tp_name,
                     richcompare_type(w)->tp_name);
        return PyValue_NULL;
    }
}

/* The exits of a rich comparison: the pairs whose comparison runs no Python
   code and answers with a bool, taken before the protocol of do_richcompare()
   and so without its recursion guard or result value. Returns 0 or 1, or -1
   when the operands need the protocol.

   The exits are keyed on the operands' comparison slots, not their types:
   two operands with one slot get that slot's answer from either operand's
   call, and a subclass that overrides nothing (re's _NamedIntConstant, an
   enum member) keeps its base's slot. An operand with another slot, a
   subclass that overrides a comparison, and a pair of which exactly one is
   an inline value, take the protocol. */
static inline int
richcompare_exit(PyValueBorrow left, PyValueBorrow right, int op)
{
    if (PyValue_IsInline(left) && PyValue_IsInline(right)) {
        /* R-inline: two inline values are exact ints or non-NaN floats.
           int's and float's compares, in the full dispatch order (int's
           gives NotImplemented for a float, then float's reflected one
           decides), come to comparing the words, or the doubles: an inline
           int has at most 51 bits, so it is exact as a double. */
        if (PyValue_BothInlineInt(left, right)) {
            return _PyLong_CompareOp(
                _PyLong_WordCompare(PyValue_InlineInt64(left),
                                    PyValue_InlineInt64(right)), op);
        }
        /* Set by the call, which holds for the operands tested above; the
           initialisers quiet -Wmaybe-uninitialized in release builds. */
        double a = 0.0, b = 0.0;
        bool floats = PyValue_FloatOperandValues(left, right, &a, &b);
        assert(floats);
        (void)floats;
        return _PyFloat_CompareOp(a, b, op);
    }
    if (!PyValue_IsPointerSpace(left) || !PyValue_IsPointerSpace(right)) {
        return -1;
    }
    PyObject *a = BITS_TO_PTR_MASKED(left);
    PyObject *b = BITS_TO_PTR_MASKED(right);
    richcmpfunc f = Py_TYPE(a)->tp_richcompare;
    if (f != Py_TYPE(b)->tp_richcompare) {
        return -1;
    }
    if (f == PyUnicode_RichCompare && (op == Py_EQ || op == Py_NE)) {
        return _PyUnicode_Equal(a, b) == (op == Py_EQ);
    }
    if (f == PyBytes_Type.tp_richcompare) {
        /* Two bytes objects: bytes_richcompare() answers with Py_True or
           Py_False for every operator, and runs no Python code. */
        return f(a, b, op) == Py_True;
    }
    if (f == PyLong_Type.tp_richcompare) {
        /* An int, or a subclass that keeps int's compare, of one digit; the
           protocol takes the rest. */
        if (!_PyLong_BothAreCompact((PyLongObject *)a, (PyLongObject *)b)) {
            return -1;
        }
        return _PyLong_CompareOp(
            _PyLong_WordCompare(_PyLong_CompactValue((PyLongObject *)a),
                                _PyLong_CompactValue((PyLongObject *)b)), op);
    }
    if (f == PyBaseObject_Type.tp_richcompare && (op == Py_EQ || op == Py_NE)) {
        /* object's == is identity, and its != inverts it. */
        return (a == b) == (op == Py_EQ);
    }
    return -1;
}

/* The protocol, with the recursion guard. */
static PyValue
richcompare_protocol(PyValueBorrow v, PyValueBorrow w, int op)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_Py_EnterRecursiveCallTstate(tstate, " in comparison")) {
        return PyValue_NULL;
    }
    PyValue res = do_richcompare(v, w, op);
    _Py_LeaveRecursiveCallTstate(tstate);
    return res;
}

/* Perform a rich comparison with value result: the exits, else the protocol
   (with a check for NULL arguments). */

PyValue
(_PyValue_RichCompare)(PyValueBorrow v, PyValueBorrow w, int op)
{
    assert(Py_LT <= op && op <= Py_GE);
    if (PyValue_IsNull(v) || PyValue_IsNull(w)) {
        if (!_PyErr_Occurred(_PyThreadState_GET())) {
            PyErr_BadInternalCall();
        }
        return PyValue_NULL;
    }
    int res = richcompare_exit(v, w, op);
    if (res >= 0) {
        return res ? PyValue_True : PyValue_False;
    }
    return richcompare_protocol(v, w, op);
}

PyObject *
PyObject_RichCompare(PyObject *v, PyObject *w, int op)
{
    return _PyValue_THUNK_EXIT(
        (_PyValue_RichCompare)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w), op),
        BOX_SITE_COMPARE);
}

/* The truth of the protocol's result, out of line so that the exits of
   _PyValue_RichCompareBool() need no frame. */
static Py_NO_INLINE int
richcompare_bool_protocol(PyValueBorrow left, PyValueBorrow right, int op)
{
    PyValue res = richcompare_protocol(left, right, op);
    if (PyValue_IsNull(res)) {
        return -1;
    }
    int ok = (_PyValue_IsTrue)(PyValue_Borrow(res));
    PyValue_CLOSE(res);
    return ok;
}

int
(_PyValue_RichCompareBool)(PyValueBorrow left, PyValueBorrow right, int op)
{
    /* Python's containers consider an object equal to itself, including NaN. */
    if ((op == Py_EQ || op == Py_NE) && PyValue_Is(left, right)) {
        return op == Py_EQ;
    }
    int res = richcompare_exit(left, right, op);
    if (res >= 0) {
        return res;
    }
    return richcompare_bool_protocol(left, right, op);
}

/* Perform a rich comparison with integer result, returning -1 for error,
   0 for false, 1 for true: _PyValue_RichCompareBool(), which also guarantees
   that identity implies equality. */
int
PyObject_RichCompareBool(PyObject *v, PyObject *w, int op)
{
    return (_PyValue_RichCompareBool)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w), op);
}

Py_hash_t
PyObject_HashNotImplemented(PyObject *v)
{
    PyErr_Format(PyExc_TypeError, "unhashable type: '%.200s'",
                 Py_TYPE(v)->tp_name);
    return -1;
}

Py_hash_t
PyObject_Hash(PyObject *v)
{
    return (_PyValue_Hash)(_Py_VALUE_ARG(v));
}

PyObject *
PyObject_GetAttrString(PyObject *v, const char *name)
{
    PyObject *w, *res;

    if (Py_TYPE(v)->tp_getattr != NULL)
        return (*Py_TYPE(v)->tp_getattr)(v, (char*)name);
    w = PyUnicode_FromString(name);
    if (w == NULL)
        return NULL;
    res = _PyValue_AsObjectResultAt(_PyValue_GetAttr(PyValue_FromObjectBorrow(v), w), BOX_SITE_ATTRIBUTE);
    Py_DECREF(w);
    return res;
}

int
PyObject_HasAttrStringWithError(PyObject *obj, const char *name)
{
    PyObject *res;
    int rc = PyObject_GetOptionalAttrString(obj, name, &res);
    Py_XDECREF(res);
    return rc;
}


int
PyObject_HasAttrString(PyObject *obj, const char *name)
{
    int rc = PyObject_HasAttrStringWithError(obj, name);
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyObject_HasAttrString(); consider using "
            "PyObject_HasAttrStringWithError(), "
            "PyObject_GetOptionalAttrString() or PyObject_GetAttrString()");
        return 0;
    }
    return rc;
}

int
PyObject_SetAttrString(PyObject *v, const char *name, PyObject *w)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (w == NULL && _PyErr_Occurred(tstate)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyErr_SetString(tstate, PyExc_SystemError,
            "PyObject_SetAttrString() must not be called with NULL value "
            "and an exception set");
        _PyErr_ChainExceptions1Tstate(tstate, exc);
        return -1;
    }

    if (Py_TYPE(v)->tp_setattr != NULL) {
        return (*Py_TYPE(v)->tp_setattr)(v, (char*)name, w);
    }

    PyObject *s = PyUnicode_InternFromString(name);
    if (s == NULL) {
        return -1;
    }

    int res = _PyValue_SetAttr(PyValue_FromObjectBorrow(v), s, PyValue_FromObjectBorrow(w));
    Py_DECREF(s);
    return res;
}

int
PyObject_DelAttrString(PyObject *v, const char *name)
{
    return PyObject_SetAttrString(v, name, NULL);
}

int
_PyObject_IsAbstract(PyObject *obj)
{
    int res;
    PyObject* isabstract;

    if (obj == NULL)
        return 0;

    res = PyObject_GetOptionalAttr(obj, &_Py_ID(__isabstractmethod__), &isabstract);
    if (res > 0) {
        res = _PyValue_IsTrue(PyValue_FromObjectBorrow(isabstract));
        Py_DECREF(isabstract);
    }
    return res;
}

PyObject *
_PyObject_GetAttrId(PyObject *v, _Py_Identifier *name)
{
    PyObject *result;
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
_Py_COMP_DIAG_POP
    if (!oname)
        return NULL;
    result = _PyValue_AsObjectResultAt(_PyValue_GetAttr(PyValue_FromObjectBorrow(v), oname), BOX_SITE_ATTRIBUTE);
    return result;
}

int
_PyObject_SetAttributeErrorContext(PyObject* v, PyObject* name)
{
    assert(PyErr_Occurred());
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)){
        return 0;
    }
    // Intercept AttributeError exceptions and augment them to offer suggestions later.
    PyObject *exc = PyErr_GetRaisedException();
    if (!PyErr_GivenExceptionMatches(exc, PyExc_AttributeError)) {
        goto restore;
    }
    PyAttributeErrorObject* the_exc = (PyAttributeErrorObject*) exc;
    // Check if this exception was already augmented
    if (the_exc->name || the_exc->obj) {
        goto restore;
    }
    // Augment the exception with the name and object
    if (_PyValue_SetAttr(PyValue_FromObjectBorrow(exc), &_Py_ID(name), PyValue_FromObjectBorrow(name)) ||
        _PyValue_SetAttr(PyValue_FromObjectBorrow(exc), &_Py_ID(obj), PyValue_FromObjectBorrow(v))) {
        Py_DECREF(exc);
        return 1;
    }
restore:
    PyErr_SetRaisedException(exc);
    return 0;
}

/* The receiver of an attribute error raised for v: as
   _PyObject_SetAttributeErrorContext(), with an inline v boxed for the
   exception's obj field (a box on the error path, value-api-m1b-m2.md Q4). */
static void
value_attribute_error_context(PyValueBorrow v, PyObject *name)
{
    if (!PyValue_IsInline(v)) {
        _PyObject_SetAttributeErrorContext(PyValue_AsKnownObjectBorrow(v),
                                           name);
        return;
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return;
    }
    PyObject *exc = PyErr_GetRaisedException();
    PyObject *obj = _PyValue_AsObjectAt(v, BOX_SITE_ATTRIBUTE);
    if (obj == NULL) {
        // Keep the AttributeError; the context is a best effort.
        PyErr_Clear();
        PyErr_SetRaisedException(exc);
        return;
    }
    PyErr_SetRaisedException(exc);
    _PyObject_SetAttributeErrorContext(obj, name);
    Py_DECREF(obj);
}

int
_PyObject_CheckAttributeName(PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }
    return 0;
}

/* The adapter of getattro: the object slot or, for a type that has only the
   legacy tp_getattr (which takes a C string), that. Neither: AttributeError.
   A slot is passed as the dispatcher read it. */
Py_NO_INLINE static PyValue
adapt_getattro(getattrofunc slot, getattrfunc legacy, PyValueBorrow v,
               PyObject *name)
{
    if (slot == NULL && legacy == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "'%.100s' object has no attribute '%U'",
                     PyValue_TYPE(v)->tp_name, name);
        return PyValue_NULL;
    }
    PyObject *self = slot != NULL
        ? _PyValue_AdapterBox(v, BOX_SITE_ATTRIBUTE, slot)
        : _PyValue_AdapterBox(v, BOX_SITE_ATTRIBUTE, legacy);
    if (self == NULL) {
        return PyValue_NULL;
    }
    PyObject *result;
    if (slot != NULL) {
        result = slot(self, name);
    }
    else {
        const char *name_str = PyUnicode_AsUTF8(name);
        result = name_str == NULL ? NULL : legacy(self, (char *)name_str);
    }
    _PyValue_AdapterRelease(v, self);
    return _PyValue_FromObjectResult(result);
}

/* The getattro of tp, the type of v: its value slot, or its object slot or
   tp_getattr through the adapter. */
static PyValue
call_getattro(PyTypeObject *tp, PyValueBorrow v, PyObject *name)
{
    _Py_getattrovaluefunc f = _PyType_VALUE_SLOT(tp, getattro);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(tp, _Py_VALUE_SLOT_INDEX_getattro);
        return f(v, name);
    }
    return adapt_getattro(tp->tp_getattro, tp->tp_getattr, v, name);
}

PyValue
(_PyValue_GetAttr)(PyValueBorrow v, PyObject *name)
{
    if (_PyObject_CheckAttributeName(name) < 0) {
        return PyValue_NULL;
    }
    PyValue result = call_getattro(PyValue_TYPE(v), v, name);
    if (PyValue_IsNull(result)) {
        value_attribute_error_context(v, name);
    }
    return result;
}

Py_VALUE_ENTRY PyObject *
PyObject_GetAttr(PyObject *v, PyObject *name)
{
    return _PyValue_THUNK_EXIT((_PyValue_GetAttr)(_Py_VALUE_ARG(v), name), BOX_SITE_ATTRIBUTE);
}

_PyStatusValue
(_PyValue_GetOptionalAttr)(PyValueBorrow v, PyObject *name)
{
    if (_PyObject_CheckAttributeName(name) < 0) {
        return (_PyStatusValue){-1, PyValue_NULL};
    }
    PyTypeObject *tp = PyValue_TYPE(v);
    /* The fast paths suppress AttributeError without raising it. A type
       and a module are never inline. */
    _Py_getattrovaluefunc f = _PyType_VALUE_SLOT(tp, getattro);
    if (f == _PyObject_GenericGetAttrValue) {
        return (_PyObject_GenericLookupAttrValue)(v, name, NULL);
    }
    PyValue result;
    if (f == _Py_type_getattro_value) {
        int suppressed = 0;
        result = _Py_type_getattro_impl(
            (PyTypeObject *)PyValue_AsKnownObjectBorrow(v), name, &suppressed);
        if (suppressed) {
            return (_PyStatusValue){0, PyValue_NULL};
        }
    }
    else if (f == _Py_module_getattro_value) {
        result = _Py_module_getattro_impl(
            (PyModuleObject *)PyValue_AsKnownObjectBorrow(v), name, 1);
        if (!PyValue_IsNull(result)) {
            return (_PyStatusValue){1, result};
        }
        return (_PyStatusValue){PyErr_Occurred() ? -1 : 0, PyValue_NULL};
    }
    else if (f == NULL && tp->tp_getattro == NULL && tp->tp_getattr == NULL) {
        return (_PyStatusValue){0, PyValue_NULL};
    }
    else {
        result = call_getattro(tp, v, name);
    }
    if (!PyValue_IsNull(result)) {
        return (_PyStatusValue){1, result};
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return (_PyStatusValue){-1, PyValue_NULL};
    }
    PyErr_Clear();
    return (_PyStatusValue){0, PyValue_NULL};
}

Py_VALUE_ENTRY int
PyObject_GetOptionalAttr(PyObject *v, PyObject *name, PyObject **result)
{
    return _PyValue_THUNK_EXIT_STATUS(
        (_PyValue_GetOptionalAttr)(_Py_VALUE_ARG(v), name), result, BOX_SITE_ATTRIBUTE);
}

int
PyObject_GetOptionalAttrString(PyObject *obj, const char *name, PyObject **result)
{
    if (Py_TYPE(obj)->tp_getattr == NULL) {
        PyObject *oname = PyUnicode_FromString(name);
        if (oname == NULL) {
            *result = NULL;
            return -1;
        }
        int rc = PyObject_GetOptionalAttr(obj, oname, result);
        Py_DECREF(oname);
        return rc;
    }

    *result = (*Py_TYPE(obj)->tp_getattr)(obj, (char*)name);
    if (*result != NULL) {
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return -1;
    }
    PyErr_Clear();
    return 0;
}

int
PyObject_HasAttrWithError(PyObject *obj, PyObject *name)
{
    _PyStatusValue r = (_PyValue_GetOptionalAttr)(_Py_VALUE_ARG(obj), name);
    PyValue_XCLOSE(r.value);
    return r.status;
}

int
PyObject_HasAttr(PyObject *obj, PyObject *name)
{
    int rc = PyObject_HasAttrWithError(obj, name);
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyObject_HasAttr(); consider using "
            "PyObject_HasAttrWithError(), "
            "PyObject_GetOptionalAttr() or PyObject_GetAttr()");
        return 0;
    }
    return rc;
}

/* The adapter of setattro: the object slot or, for a type that has only the
   legacy tp_setattr (which takes a C string), that. Neither: TypeError. The
   receiver and an inline value are boxed. value is PyValue_NULL to delete. */
Py_NO_INLINE static int
adapt_setattro(setattrofunc slot, setattrfunc legacy, PyValueBorrow v,
               PyObject *name, PyValueBorrow value)
{
    if (slot == NULL && legacy == NULL) {
        PyTypeObject *tp = PyValue_TYPE(v);
        PyErr_Format(PyExc_TypeError,
                     tp->tp_getattr == NULL && tp->tp_getattro == NULL
                     ? "'%.100s' object has no attributes (%s .%U)"
                     : "'%.100s' object has only read-only attributes "
                       "(%s .%U)",
                     tp->tp_name,
                     PyValue_IsNull(value) ? "del" : "assign to",
                     name);
        return -1;
    }
    PyObject *self = slot != NULL
        ? _PyValue_AdapterBox(v, BOX_SITE_ATTRIBUTE, slot)
        : _PyValue_AdapterBox(v, BOX_SITE_ATTRIBUTE, legacy);
    if (self == NULL) {
        return -1;
    }
    PyObject *w = NULL;
    if (!PyValue_IsNull(value)) {
        w = slot != NULL
            ? _PyValue_AdapterBox(value, BOX_SITE_ATTRIBUTE, slot)
            : _PyValue_AdapterBox(value, BOX_SITE_ATTRIBUTE, legacy);
        if (w == NULL) {
            _PyValue_AdapterRelease(v, self);
            return -1;
        }
    }
    int err = -1;
    if (slot != NULL) {
        err = slot(self, name, w);
    }
    else {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str != NULL) {
            err = legacy(self, (char *)name_str, w);
        }
    }
    if (w != NULL) {
        _PyValue_AdapterRelease(value, w);
    }
    _PyValue_AdapterRelease(v, self);
    return err;
}

int
(_PyValue_SetAttr)(PyValueBorrow v, PyObject *name, PyValueBorrow value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (PyValue_IsNull(value) && _PyErr_Occurred(tstate)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyErr_SetString(tstate, PyExc_SystemError,
            "PyObject_SetAttr() must not be called with NULL value "
            "and an exception set");
        _PyErr_ChainExceptions1Tstate(tstate, exc);
        return -1;
    }

    PyTypeObject *tp = PyValue_TYPE(v);
    int err;

    if (_PyObject_CheckAttributeName(name) < 0) {
        return -1;
    }
    Py_INCREF(name);

    _PyUnicode_InternMortal(tstate->interp, &name);
    _Py_setattrovaluefunc f = _PyType_VALUE_SLOT(tp, setattro);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(tp, _Py_VALUE_SLOT_INDEX_setattro);
        err = f(v, name, value);
        Py_DECREF(name);
        return err;
    }
    err = adapt_setattro(tp->tp_setattro, tp->tp_setattr, v, name, value);
    Py_DECREF(name);
    return err;
}

int
_PyValue_CallSetAttrBase(PyTypeObject *base, setattrofunc slot,
                         PyValueBorrow self, PyObject *name,
                         PyValueBorrow value)
{
    assert(slot != NULL);
    int row = _Py_VALUE_SLOT_INDEX_setattro;
    _Py_setattrovaluefunc f =
        (_Py_setattrovaluefunc)_PyType_VALUE_FIELD_AT(base, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(base, row);
        return f(self, name, value);
    }
    return adapt_setattro(slot, NULL, self, name, value);
}

int
PyObject_SetAttr(PyObject *v, PyObject *name, PyObject *value)
{
    return (_PyValue_SetAttr)(_Py_VALUE_ARG(v), name, _Py_VALUE_ARG(value));
}

int
PyObject_DelAttr(PyObject *v, PyObject *name)
{
    return _PyValue_SetAttr(PyValue_FromObjectBorrow(v), name, PyValue_FromObjectBorrow(NULL));
}

PyObject **
_PyObject_ComputedDictPointer(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    assert((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);

    Py_ssize_t dictoffset = tp->tp_dictoffset;
    if (dictoffset == 0) {
        return NULL;
    }

    if (dictoffset < 0) {
        assert(dictoffset != -1);

        Py_ssize_t tsize = Py_SIZE(obj);
        if (tsize < 0) {
            tsize = -tsize;
        }
        size_t size = _PyObject_VAR_SIZE(tp, tsize);
        assert(size <= (size_t)PY_SSIZE_T_MAX);
        dictoffset += (Py_ssize_t)size;

        _PyObject_ASSERT(obj, dictoffset > 0);
        _PyObject_ASSERT(obj, dictoffset % SIZEOF_VOID_P == 0);
    }
    return (PyObject **) ((char *)obj + dictoffset);
}

/* Helper to get a pointer to an object's __dict__ slot, if any.
 * Creates the dict from inline attributes if necessary.
 * Does not set an exception.
 *
 * Note that the tp_dictoffset docs used to recommend this function,
 * so it should be treated as part of the public API.
 */
PyObject **
_PyObject_GetDictPtr(PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return _PyObject_ComputedDictPointer(obj);
    }
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL && Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        dict = _PyObject_MaterializeManagedDict(obj);
        if (dict == NULL) {
            PyErr_Clear();
            return NULL;
        }
    }
    return (PyObject **)&_PyObject_ManagedDictPointer(obj)->dict;
}

PyObject *
PyObject_SelfIter(PyObject *obj)
{
    return Py_NewRef(obj);
}

/* Helper used when the __next__ method is removed from a type:
   tp_iternext is never NULL and can be safely called without checking
   on every iteration.
 */

// The object side of the pair table's _PyValue_NextNotImplemented().
Py_VALUE_ENTRY PyObject *
_PyObject_NextNotImplemented(PyObject *self)
{
    return _PyValue_IterNextThunkExit(_PyValue_NextNotImplemented(self));
}


/* Lifetime of a class attribute found for a lookup.

   A type-cache hit borrows the attribute from the cache entry, which borrows
   it from a type dict on the MRO. That borrow is valid until Python code can
   run: Python can mutate a type, and a mutation may drop the dict's reference
   (the entry's version tag is then 0 and its table is retired, so the borrow
   is never read again, but the object may be freed). Everything between the
   lookup and a pin_class_attr() runs no Python: it reads types, flags and the instance
   dict of an inline-value or managed-dict object.

   Before a step that can run Python, pin_class_attr() takes an owner of the attribute
   and of the type of the receiver. The receiver keeps its type alive only
   while Python has not run: it may reassign __class__ and release the type.
   A type-cache miss pins at once: the MRO walk compares keys of a general
   dict with arbitrary __eq__. The caller owns the receiver and the name for
   the whole call, as for any borrowed argument.

   A descriptor type whose descr_get leaves the descriptor unused once it
   calls out needs no pin even when the callout runs Python (a getset getter,
   or the audit hook of a member): it copies the getter, the closure and the
   member offset out of the descriptor before the callout, and the
   PyGetSetDef or PyMemberDef they point into belongs to the extension and is
   not freed with the descriptor. Function and method descriptors create a
   bound object, taking their own references, and run no Python. */
static inline int
descr_get_leaves_descr_unused(PyObject *descr)
{
    PyTypeObject *dt = Py_TYPE(descr);
    if (dt == &PyFunction_Type || dt == &PyGetSetDescr_Type ||
        dt == &PyMethodDescr_Type) {
        return 1;
    }
    return dt == &PyMemberDescr_Type;
}

/* Make the lookup's result owned, along with the type of the receiver. */
static inline void
pin_class_attr(PyValueBorrow descr_value, PyValue *owner, PyTypeObject *tp,
               int *pinned)
{
    if (!*pinned) {
        *owner = PyValue_XDUP(descr_value);
        Py_INCREF(tp);
        *pinned = 1;
    }
}

/* The cold parts of the generic attribute read and of _PyValue_GetMethod,
   out of line so that a read the type cache answers is one contiguous path. */

/* The lookup of name on tp that the type cache did not answer: the type is
   readied if it is not, and the MRO is walked. The caller holds a count on
   tp. Returns -1 with an exception set when the type cannot be readied. */
static _Py_COLD_FUNCTION Py_NO_INLINE int
class_attr_uncached(PyTypeObject *tp, PyObject *name, PyValue *owner,
                    PyValueBorrow *descr_value)
{
    if (!_PyType_IsReady(tp) && PyType_Ready(tp) < 0) {
        return -1;
    }
    (void)_PyType_LookupValueAndVersion(tp, name, owner);
    *descr_value = PyValue_Borrow(*owner);
    return 0;
}

/* Is the member descriptor's field of obj unset, so that its read would
   raise AttributeError (gh-157840)? A member of another type or an audited
   member is read the ordinary way. */
Py_NO_INLINE static int
member_field_missing(PyObject *obj, PyObject *descr)
{
    PyMemberDef *member = ((PyMemberDescrObject *)descr)->d_member;
    if ((member->type == Py_T_OBJECT_EX || member->type == _Py_T_VALUE_EX)
        && !(member->flags & Py_AUDIT_READ)
        && PyObject_TypeCheck(obj, PyDescr_TYPE(descr))) {
        void *addr = _PyMember_GetOffset(obj, member);
        return member->type == _Py_T_VALUE_EX
            ? PyValue_IsNull(*(PyValue *)addr)
            : FT_ATOMIC_LOAD_PTR(*(PyObject **)addr) == NULL;
    }
    return 0;
}

static _Py_COLD_FUNCTION Py_NO_INLINE void
attribute_missing(PyTypeObject *tp, PyValueBorrow v, PyObject *name)
{
    PyErr_Format(PyExc_AttributeError,
                 "'%.100s' object has no attribute '%U'",
                 tp->tp_name, name);
    value_attribute_error_context(v, name);
}

/* The port of _PyObject_GetMethod(): a method descriptor found on the type
   gives status 1 with the descriptor, to call with obj as self, so that no
   bound method is made; anything else is the attribute, status 0. A
   classmethod therefore gives its bound method (status 0): the status
   cannot carry a self other than obj. */
_PyStatusValue
(_PyValue_GetMethod)(PyValueBorrow v, PyObject *name)
{
    _PyStatusValue r = {-1, PyValue_NULL};
    /* One test of the receiver; an inline receiver has no instance
       attributes. */
    PyObject *obj = PyValue_ObjectOrNull(v);
    PyTypeObject *tp = obj != NULL ? Py_TYPE(obj) : _PyValue_TypeNotPointer(v);
    /* See "Lifetime of a class attribute": descr_value is borrowed from the
       type cache until pinned, then owned by owner, as is tp. */
    PyValueBorrow descr_value;
    PyValue owner = PyValue_NULL;
    int pinned = 0;
    if (_PyType_VALUE_SLOT(tp, getattro) != _PyObject_GenericGetAttrValue
        || _PyTypeCache_Lookup(tp, name, &descr_value) == 0)
    {
        // Another getattro, an uncacheable name or a miss. A type that is not
        // ready has no table, so only here is it readied.
        Py_INCREF(tp);
        pinned = 1;
        if (!_PyType_IsReady(tp) && PyType_Ready(tp) < 0) {
            goto done;
        }
        if (_PyType_VALUE_SLOT(tp, getattro) != _PyObject_GenericGetAttrValue
            || !PyUnicode_CheckExact(name))
        {
            r.value = (_PyValue_GetAttr)(v, name);
            r.status = PyValue_IsNull(r.value) ? -1 : 0;
            goto done;
        }
        (void)_PyType_LookupValueAndVersion(tp, name, &owner);
        descr_value = PyValue_Borrow(owner);
    }
    PyObject *descr = PyValue_ObjectOrNull(descr_value);
    int has_get = descr != NULL && Py_TYPE(descr)->tp_descr_get != NULL;
    bool method_descriptor = descr != NULL &&
        _PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR);
    if (has_get && PyDescr_IsData(descr)) {
        if (!descr_get_leaves_descr_unused(descr)) {
            pin_class_attr(descr_value, &owner, tp, &pinned);
        }
        r.value = (_PyValue_DescrGet)(descr, v, tp);
        r.status = PyValue_IsNull(r.value) ? -1 : 0;
        goto done;
    }
    PyValue attr = PyValue_NULL;
    int handled = (obj != NULL && (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) ?
        _PyObject_TryGetInstanceValue(obj, name, &attr) : 0;
    if (handled < 0) {
        goto done;
    }
    if (!PyValue_IsNull(attr)) {
        r = (_PyStatusValue){0, attr};
        goto done;
    }
    PyObject *dict = NULL;
    if (obj != NULL && !handled) {
        if (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
            dict = (PyObject *)_PyObject_GetManagedDict(obj);
        }
        else {
            PyObject **ptr = _PyObject_ComputedDictPointer(obj);
            dict = ptr == NULL ? NULL : FT_ATOMIC_LOAD_PTR_ACQUIRE(*ptr);
        }
    }
    if (dict != NULL) {
        // A dict with general keys runs their __eq__.
        pin_class_attr(descr_value, &owner, tp, &pinned);
        Py_INCREF(dict);
        int found = _PyDict_GetValue((PyDictObject *)dict, PyValue_FromObjectBorrow(name), &attr);
        Py_DECREF(dict);
        if (found < 0) {
            PyValue_XCLOSE(attr);
            goto done;
        }
        if (found) {
            r = (_PyStatusValue){0, attr};
            goto done;
        }
    }
    if (method_descriptor) {
        // The result owns the descriptor.
        r = (_PyStatusValue){1, pinned ? owner : PyValue_XDUP(descr_value)};
        owner = PyValue_NULL;
    }
    else if (has_get) {
        if (Py_IS_TYPE(descr, &PyStaticMethod_Type)) {
            r = (_PyStatusValue){
                0, PyValue_DUP(PyValue_FromObjectBorrow(_PyStaticMethod_GetFunc(descr)))};
        }
        else {
            if (!descr_get_leaves_descr_unused(descr)) {
                pin_class_attr(descr_value, &owner, tp, &pinned);
            }
            r.value = (_PyValue_DescrGet)(descr, v, tp);
            r.status = PyValue_IsNull(r.value) ? -1 : 0;
        }
    }
    else if (!PyValue_IsNull(descr_value)) {
        r = (_PyStatusValue){0, pinned ? owner : PyValue_XDUP(descr_value)};
        owner = PyValue_NULL;
    }
    else {
        attribute_missing(tp, v, name);
    }
 done:
    if (pinned) {
        PyValue_XCLOSE(owner);
        Py_DECREF(tp);
    }
    return r;
}

Py_NO_INLINE static PyValue
adapt_descr_get(descrgetfunc get, PyObject *descr, PyValueBorrow obj,
                PyTypeObject *type)
{
    PyObject *o = NULL;
    if (!PyValue_IsNull(obj)) {
        o = _PyValue_AdapterBox(obj, BOX_SITE_ATTRIBUTE, get);
        if (o == NULL) {
            return PyValue_NULL;
        }
    }
    PyObject *result = get(descr, o, (PyObject *)type);
    if (o != NULL) {
        _PyValue_AdapterRelease(obj, o);
    }
    return _PyValue_FromObjectResult(result);
}

PyValue
(_PyValue_DescrGet)(PyObject *descr, PyValueBorrow obj, PyTypeObject *type)
{
    PyTypeObject *dt = Py_TYPE(descr);
    _Py_descrgetvaluefunc f = _PyType_VALUE_SLOT(dt, descr_get);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(dt, _Py_VALUE_SLOT_INDEX_descr_get);
        return f(descr, obj, (PyObject *)type);
    }
    descrgetfunc get = dt->tp_descr_get;
    assert(get != NULL);
    return adapt_descr_get(get, descr, obj, type);
}

Py_NO_INLINE static int
adapt_descr_set(descrsetfunc set, PyObject *descr, PyValueBorrow obj,
                PyValueBorrow value)
{
    PyObject *o = _PyValue_AdapterBox(obj, BOX_SITE_ATTRIBUTE, set);
    if (o == NULL) {
        return -1;
    }
    PyObject *w = NULL;
    if (!PyValue_IsNull(value)) {
        w = _PyValue_AdapterBox(value, BOX_SITE_ATTRIBUTE, set);
        if (w == NULL) {
            _PyValue_AdapterRelease(obj, o);
            return -1;
        }
    }
    int res = set(descr, o, w);
    if (w != NULL) {
        _PyValue_AdapterRelease(value, w);
    }
    _PyValue_AdapterRelease(obj, o);
    return res;
}

int
(_PyValue_DescrSet)(PyObject *descr, PyValueBorrow obj, PyValueBorrow value)
{
    PyTypeObject *dt = Py_TYPE(descr);
    _Py_descrsetvaluefunc f = _PyType_VALUE_SLOT(dt, descr_set);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(dt, _Py_VALUE_SLOT_INDEX_descr_set);
        return f(descr, obj, value);
    }
    descrsetfunc set = dt->tp_descr_set;
    assert(set != NULL);
    return adapt_descr_set(set, descr, obj, value);
}


/* Generic GetAttr functions - put these in your tp_[gs]etattro slot. */

/* The generic attribute read of v (_PyObject_GenericGetAttrWithDict()).
   When suppress is 1, an AttributeError is suppressed: PyValue_NULL with no
   exception set means the attribute is missing. An inline v has no
   instance attributes, and its type's descriptors see it inline. */
static PyValue
generic_getattr_value(PyValueBorrow v, PyObject *name, PyObject *dict,
                      int suppress)
{
    /* Make sure the logic of _PyValue_GetMethod is in sync with
       this method.
    */

    /* One test of the receiver; an inline receiver has no instance
       attributes. */
    PyObject *obj = PyValue_ObjectOrNull(v);
    PyTypeObject *tp = obj != NULL ? Py_TYPE(obj) : _PyValue_TypeNotPointer(v);
    PyObject *descr = NULL;
    PyValue res = PyValue_NULL;
    int has_get = 0;

    /* The public entries (PyObject_GenericGetAttr() and
       _PyObject_GenericGetAttrWithDict()) test the name; the value slot is
       reached from _PyValue_GetAttr(), which has. */
    assert(PyUnicode_Check(name));

    /* See "Lifetime of a class attribute": descr_value is borrowed from the
       type cache until pinned, then owned by owner, as is tp. */
    PyValueBorrow descr_value;
    PyValue owner = PyValue_NULL;
    int pinned = 0;
    if (_PyTypeCache_Lookup(tp, name, &descr_value) == 0) {
        // A type that is not ready has no table, so only a miss is here.
        Py_INCREF(tp);
        pinned = 1;
        if (class_attr_uncached(tp, name, &owner, &descr_value) < 0) {
            goto done;
        }
    }
    descr = PyValue_ObjectOrNull(descr_value);

    if (descr != NULL) {
        has_get = Py_TYPE(descr)->tp_descr_get != NULL;
        if (has_get && PyDescr_IsData(descr)) {
            // gh-157840: We special-case member descriptors here to avoid
            // allocating an extra AttributeError
            if (suppress && obj != NULL && Py_IS_TYPE(descr, &PyMemberDescr_Type)
                && member_field_missing(obj, descr)) {
                goto done;
            }
            if (!descr_get_leaves_descr_unused(descr)) {
                pin_class_attr(descr_value, &owner, tp, &pinned);
            }
            res = (_PyValue_DescrGet)(descr, v, tp);
            if (PyValue_IsNull(res) && suppress &&
                    PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
            }
            goto done;
        }
    }
    if (dict == NULL && obj != NULL) {
        if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
            PyValue attr = PyValue_NULL;
            int handled = PyUnicode_CheckExact(name) ?
                _PyObject_TryGetInstanceValue(obj, name, &attr) : 0;
            if (handled < 0) {
                goto done;
            }
            if (handled) {
                if (!PyValue_IsNull(attr)) {
                    res = attr;
                    goto done;
                }
            }
            else {
                dict = (PyObject *)_PyObject_MaterializeManagedDict(obj);
                if (dict == NULL) {
                    goto done;
                }
            }
        }
        else if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
            dict = (PyObject *)_PyObject_GetManagedDict(obj);
        }
        else {
            PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
            if (dictptr) {
#ifdef Py_GIL_DISABLED
                dict = _Py_atomic_load_ptr_acquire(dictptr);
#else
                dict = *dictptr;
#endif
            }
        }
    }
    if (dict != NULL) {
        // A dict with general keys runs their __eq__.
        pin_class_attr(descr_value, &owner, tp, &pinned);
        Py_INCREF(dict);
        int rc = _PyDict_GetValue((PyDictObject *)dict, PyValue_FromObjectBorrow(name), &res);
        /* Suppress the lookup's AttributeError before retirement, so a
         * later cleanup failure remains an error. */
        if (rc < 0 && PyValue_IsNull(res) && suppress &&
            PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            rc = 0;
        }
        Py_CLEAR(dict);
        if (!PyValue_IsNull(res) || rc < 0) {
            goto done;
        }
    }

    if (has_get) {
        if (!descr_get_leaves_descr_unused(descr)) {
            pin_class_attr(descr_value, &owner, tp, &pinned);
        }
        res = (_PyValue_DescrGet)(descr, v, tp);
        if (PyValue_IsNull(res) && suppress &&
                PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
        }
        goto done;
    }

    if (!PyValue_IsNull(descr_value)) {
        // A class attribute, perhaps inline: the result owns the lookup's.
        if (pinned) {
            res = owner;
            owner = PyValue_NULL;
        }
        else {
            res = PyValue_XDUP(descr_value);
        }
        goto done;
    }

    if (!suppress) {
        attribute_missing(tp, v, name);
    }
  done:
    if (pinned) {
        PyValue_XCLOSE(owner);
        Py_DECREF(tp);
    }
    return res;
}

PyValue
(_PyObject_GenericGetAttrValue)(PyValueBorrow v, PyObject *name)
{
    return generic_getattr_value(v, name, NULL, 0);
}

_PyStatusValue
(_PyObject_GenericLookupAttrValue)(PyValueBorrow v, PyObject *name,
                                   PyObject *dict)
{
    PyValue res = generic_getattr_value(v, name, dict, 1);
    if (!PyValue_IsNull(res)) {
        return (_PyStatusValue){1, res};
    }
    return (_PyStatusValue){PyErr_Occurred() ? -1 : 0, PyValue_NULL};
}

Py_VALUE_ENTRY PyObject *
_PyObject_GenericGetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *dict, int suppress)
{
    if (_PyObject_CheckAttributeName(name) < 0) {
        return NULL;
    }
    return _PyValue_THUNK_EXIT(
        generic_getattr_value(_Py_VALUE_ARG(obj), name, dict, suppress), BOX_SITE_ATTRIBUTE);
}

PyObject *
PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
{
    return _PyObject_GenericGetAttrWithDict(obj, name, NULL, 0);
}

int
(_PyObject_GenericSetAttrWithDictValue)(PyValueBorrow v, PyObject *name,
                                        PyValueBorrow value, PyObject *dict)
{
    PyTypeObject *tp = PyValue_TYPE(v);
    // An inline value has no instance dict.
    PyObject *obj = PyValue_ObjectOrNull(v);
    PyObject *owned_dict = NULL;
    PyObject *descr;
    int res = -1;

    assert(!PyType_IsSubtype(tp, &PyType_Type));
    /* The public entries test the name; the value slot is reached from
       _PyValue_SetAttr(), which has. */
    assert(PyUnicode_Check(name));

    _Py_INCREF_TYPE(tp);
    if (!_PyType_IsReady(tp) && PyType_Ready(tp) < 0) {
        _Py_DECREF_TYPE(tp);
        return -1;
    }

    Py_INCREF(name);

    PyThreadState *tstate = _PyThreadState_GET();
    _PyCValue cref;
    _PyThreadState_PushCValue(tstate, &cref);

    (void)_PyType_LookupValueAndVersion(tp, name, &cref.ref);
    descr = PyValue_ObjectOrNull(cref.ref);

    if (descr != NULL && Py_TYPE(descr)->tp_descr_set != NULL) {
        res = (_PyValue_DescrSet)(descr, v, value);
        goto done;
    }

    if (dict == NULL) {
        PyObject **dictptr;

        if (obj == NULL) {
            dictptr = NULL;
        }
        else if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
            res = _PyObject_StoreInstanceValue(obj, name, value);
            goto error_check;
        }
        else if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
            PyManagedDictPointer *managed_dict = _PyObject_ManagedDictPointer(obj);
            dictptr = (PyObject **)&managed_dict->dict;
        }
        else {
            dictptr = _PyObject_ComputedDictPointer(obj);
        }
        if (dictptr == NULL) {
            if (PyValue_IsNull(cref.ref)) {
                if (_PyType_VALUE_SLOT(tp, setattro)
                    == _PyObject_GenericSetAttrValue)
                {
                    PyErr_Format(PyExc_AttributeError,
                                "'%.100s' object has no attribute '%U' and no "
                                "__dict__ for setting new attributes",
                                tp->tp_name, name);
                }
                else {
                    PyErr_Format(PyExc_AttributeError,
                                "'%.100s' object has no attribute '%U'",
                                tp->tp_name, name);
                }
                value_attribute_error_context(v, name);
            }
            else {
                PyErr_Format(PyExc_AttributeError,
                            "'%.100s' object attribute '%U' is read-only",
                            tp->tp_name, name);
            }
            goto done;
        }
        else {
            res = _PyObjectDict_SetValue(tp, obj, dictptr, name, value);
        }
    }
    else {
        owned_dict = Py_NewRef(dict);
        if (PyValue_IsNull(value))
            res = _PyDict_DelValue((PyDictObject *)dict, PyValue_FromObjectBorrow(name));
        else
            res = _PyDict_SetValue((PyDictObject *)dict, PyValue_FromObjectBorrow(name), value);
    }
  error_check:
    if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Format(PyExc_AttributeError,
                        "'%.100s' object has no attribute '%U'",
                        tp->tp_name, name);
        value_attribute_error_context(v, name);
    }
  done:
    _PyThreadState_PopCValue(tstate, &cref);
    Py_CLEAR(owned_dict);
    Py_CLEAR(name);
    _Py_DECREF_TYPE(tp);
    return res;
}

int
(_PyObject_GenericSetAttrValue)(PyValueBorrow v, PyObject *name,
                                PyValueBorrow value)
{
    return (_PyObject_GenericSetAttrWithDictValue)(v, name, value, NULL);
}

int
_PyObject_GenericSetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *value, PyObject *dict)
{
    if (_PyObject_CheckAttributeName(name) < 0) {
        return -1;
    }
    return (_PyObject_GenericSetAttrWithDictValue)(
        _Py_VALUE_ARG(obj), name, _Py_VALUE_ARG(value), dict);
}

int
PyObject_GenericSetAttr(PyObject *obj, PyObject *name, PyObject *value)
{
    if (_PyObject_CheckAttributeName(name) < 0) {
        return -1;
    }
    return (_PyObject_GenericSetAttrValue)(_Py_VALUE_ARG(obj), name,
                                           _Py_VALUE_ARG(value));
}

int
PyObject_GenericSetDict(PyObject *obj, PyObject *value, void *context)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete __dict__");
        return -1;
    }
    return _PyObject_SetDict(obj, value);
}


/* Test a value used as condition, e.g., in a while or if statement.
   Return -1 if an error occurred */

Py_NO_INLINE static Py_ssize_t
adapt_bool(inquiry slot, PyValueBorrow v)
{
    PyObject *a = _PyValue_AdapterBox(v, BOX_SITE_NUMBER, slot);
    if (a == NULL) {
        return -1;
    }
    int res = slot(a);
    _PyValue_AdapterRelease(v, a);
    return res;
}

static Py_ssize_t
call_bool(PyTypeObject *t, inquiry slot, PyValueBorrow v)
{
    int row = _Py_VALUE_SLOT_INDEX_nb_bool;
    _Py_inquiryvaluefunc f =
        (_Py_inquiryvaluefunc)_PyType_VALUE_FIELD_AT(t, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, row);
        return f(v);
    }
    return adapt_bool(slot, v);
}

int
(_PyValue_IsTrueSlow)(PyValueBorrow v)
{
    Py_ssize_t res;
    /* The callers have answered for a bool, None, an int and a float: v
       holds another object, and the one test of its tag is this. */
    PyObject *obj = PyValue_AsKnownObjectBorrow(v);
    assert(!PyValue_IsTrue(v) && !PyValue_IsFalse(v) && !PyValue_IsNone(v));
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_as_number != NULL &&
             tp->tp_as_number->nb_bool != NULL)
        res = call_bool(tp, tp->tp_as_number->nb_bool, v);
    else if (tp->tp_as_mapping != NULL &&
             tp->tp_as_mapping->mp_length != NULL)
        res = (*tp->tp_as_mapping->mp_length)(obj);
    else if (tp->tp_as_sequence != NULL &&
             tp->tp_as_sequence->sq_length != NULL)
        res = (*tp->tp_as_sequence->sq_length)(obj);
    else
        return 1;
    /* if it is negative, it should be either -1 or -2 */
    return (res > 0) ? 1 : Py_SAFE_DOWNCAST(res, Py_ssize_t, int);
}

int
PyObject_IsTrue(PyObject *v)
{
    return (_PyValue_IsTrue)(_Py_VALUE_ARG(v));
}

/* equivalent of 'not v'
   Return -1 if an error occurred */

int
(_PyValue_Not)(PyValueBorrow v)
{
    int res;
    res = (_PyValue_IsTrue)(v);
    if (res < 0)
        return res;
    return res == 0;
}

int
PyObject_Not(PyObject *v)
{
    return (_PyValue_Not)(_Py_VALUE_ARG(v));
}

/* Test whether an object can be called */

int
PyCallable_Check(PyObject *x)
{
    if (x == NULL)
        return 0;
    return Py_TYPE(x)->tp_call != NULL;
}


/* Helper for PyObject_Dir without arguments: returns the local scope. */
static PyObject *
_dir_locals(void)
{
    PyObject *names;
    PyObject *locals;

    if (_PyEval_GetFrame() != NULL) {
        locals = _PyEval_GetFrameLocals();
    }
    else {
        PyThreadState *tstate = _PyThreadState_GET();
        locals = _PyEval_GetGlobalsFromRunningMain(tstate);
        if (locals == NULL) {
            if (!_PyErr_Occurred(tstate)) {
                locals = _PyEval_GetFrameLocals();
                assert(_PyErr_Occurred(tstate));
            }
        }
        else {
            Py_INCREF(locals);
        }
    }
    if (locals == NULL) {
        return NULL;
    }

    names = _PyValue_MappingKeys(PyValue_FromObjectBorrow(locals));
    Py_DECREF(locals);
    if (!names) {
        return NULL;
    }
    if (!PyList_Check(names)) {
        PyErr_Format(PyExc_TypeError,
            "dir(): expected keys() of locals to be a list, "
            "not '%.200s'", Py_TYPE(names)->tp_name);
        Py_DECREF(names);
        return NULL;
    }
    if (PyList_Sort(names)) {
        Py_DECREF(names);
        return NULL;
    }
    return names;
}

/* Helper for PyObject_Dir: object introspection. */
static PyObject *
_dir_object(PyObject *obj)
{
    PyObject *result, *sorted;
    PyObject *dirfunc = _PyObject_LookupSpecial(obj, &_Py_ID(__dir__));

    assert(obj != NULL);
    if (dirfunc == NULL) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_TypeError, "object does not provide __dir__");
        return NULL;
    }
    /* use __dir__ */
    result = _PyObject_CallNoArgs(dirfunc);
    Py_DECREF(dirfunc);
    if (result == NULL)
        return NULL;
    /* return sorted(result) */
    sorted = _PyValue_SequenceList(PyValue_FromObjectBorrow(result));
    Py_DECREF(result);
    if (sorted == NULL)
        return NULL;
    if (PyList_Sort(sorted)) {
        Py_DECREF(sorted);
        return NULL;
    }
    return sorted;
}

/* Implementation of dir() -- if obj is NULL, returns the names in the current
   (local) scope.  Otherwise, performs introspection of the object: returns a
   sorted list of attribute names (supposedly) accessible from the object
*/
PyObject *
PyObject_Dir(PyObject *obj)
{
    return (obj == NULL) ? _dir_locals() : _dir_object(obj);
}

/*
None is a non-NULL undefined value.
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

/* ARGSUSED */
static PyObject *
none_repr(PyObject *op)
{
    return PyUnicode_FromString("None");
}

static void
none_dealloc(PyObject* none)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref None out of existence. Instead,
     * since None is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(none);
}

static PyObject *
none_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NoneType takes no arguments");
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
none_bool(PyObject *v)
{
    return 0;
}

static Py_hash_t none_hash(PyObject *v)
{
    return 0xFCA86420;
}

static PyNumberMethods none_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    none_bool,                  /* nb_bool */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    0,                          /* nb_and */
    0,                          /* nb_xor */
    0,                          /* nb_or */
    0,                          /* nb_int */
    0,                          /* nb_reserved */
    0,                          /* nb_float */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    0,                          /* nb_index */
};

PyDoc_STRVAR(none_doc,
"NoneType()\n"
"--\n\n"
"The type of the None singleton.");

PyTypeObject _PyNone_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NoneType",
    0,
    0,
    none_dealloc,       /*tp_dealloc*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    none_repr,          /*tp_repr*/
    &none_as_number,    /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    none_hash,          /*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
    none_doc,           /*tp_doc */
    0,                  /*tp_traverse */
    0,                  /*tp_clear */
    _Py_BaseObject_RichCompare, /*tp_richcompare */
    0,                  /*tp_weaklistoffset */
    0,                  /*tp_iter */
    0,                  /*tp_iternext */
    0,                  /*tp_methods */
    0,                  /*tp_members */
    0,                  /*tp_getset */
    0,                  /*tp_base */
    0,                  /*tp_dict */
    0,                  /*tp_descr_get */
    0,                  /*tp_descr_set */
    0,                  /*tp_dictoffset */
    0,                  /*tp_init */
    0,                  /*tp_alloc */
    none_new,           /*tp_new */
};

PyObject _Py_NoneStruct = _PyObject_HEAD_INIT(&_PyNone_Type);

/* NotImplemented is an object that can be used to signal that an
   operation is not implemented for the given type combination. */

static PyObject *
NotImplemented_repr(PyObject *op)
{
    return PyUnicode_FromString("NotImplemented");
}

static PyObject *
NotImplemented_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("NotImplemented");
}

static PyMethodDef notimplemented_methods[] = {
    {"__reduce__", NotImplemented_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyObject *
notimplemented_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NotImplementedType takes no arguments");
        return NULL;
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static void
notimplemented_dealloc(PyObject *notimplemented)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref NotImplemented out of existence. Instead,
     * since Notimplemented is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(notimplemented);
}

static int
notimplemented_bool(PyObject *v)
{
    PyErr_SetString(PyExc_TypeError,
                    "NotImplemented should not be used in a boolean context");
    return -1;
}

static PyNumberMethods notimplemented_as_number = {
    .nb_bool = notimplemented_bool,
};

PyDoc_STRVAR(notimplemented_doc,
"NotImplementedType()\n"
"--\n\n"
"The type of the NotImplemented singleton.");

PyTypeObject _PyNotImplemented_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NotImplementedType",
    0,
    0,
    notimplemented_dealloc,       /*tp_dealloc*/ /*never called*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    NotImplemented_repr,        /*tp_repr*/
    &notimplemented_as_number,  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
    notimplemented_doc, /*tp_doc */
    0,                  /*tp_traverse */
    0,                  /*tp_clear */
    0,                  /*tp_richcompare */
    0,                  /*tp_weaklistoffset */
    0,                  /*tp_iter */
    0,                  /*tp_iternext */
    notimplemented_methods, /*tp_methods */
    0,                  /*tp_members */
    0,                  /*tp_getset */
    0,                  /*tp_base */
    0,                  /*tp_dict */
    0,                  /*tp_descr_get */
    0,                  /*tp_descr_set */
    0,                  /*tp_dictoffset */
    0,                  /*tp_init */
    0,                  /*tp_alloc */
    notimplemented_new, /*tp_new */
};

PyObject _Py_NotImplementedStruct = _PyObject_HEAD_INIT(&_PyNotImplemented_Type);


PyStatus
_PyObject_InitState(PyInterpreterState *interp)
{
#ifdef Py_TRACE_REFS
    if (refchain_init(interp) < 0) {
        return _PyStatus_NO_MEMORY();
    }
#endif
    return _PyStatus_OK();
}

void
_PyObject_FiniState(PyInterpreterState *interp)
{
#ifdef Py_TRACE_REFS
    refchain_fini(interp);
#endif
}


extern PyTypeObject _PyLegacyEventHandler_Type;
extern PyTypeObject _PyLineIterator;
extern PyTypeObject _PyMemoryIter_Type;
extern PyTypeObject _PyPositionsIterator;
extern PyTypeObject _Py_GenericAliasIterType;

static PyTypeObject* static_types[_Py_NUM_MANAGED_PREINITIALIZED_TYPES] = {
    // The two most important base types: must be initialized first and
    // deallocated last.
    &PyBaseObject_Type,
    &PyType_Type,

    // PyStaticMethod_Type and PyCFunction_Type are used by PyType_Ready()
    // on other types and so must be initialized first.
    &PyStaticMethod_Type,
    &PyCFunction_Type,

    // Static types with base=&PyBaseObject_Type
    &PyAsyncGen_Type,
    &PyByteArrayIter_Type,
    &PyByteArray_Type,
    &PyBytesIter_Type,
    &PyBytes_Type,
    &PyCallIter_Type,
    &PyCapsule_Type,
    &PyCell_Type,
    &PyClassMethodDescr_Type,
    &PyClassMethod_Type,
    &PyCode_Type,
    &PyComplex_Type,
    &PyContextToken_Type,
    &PyContextVar_Type,
    &PyContext_Type,
    &PyCoro_Type,
    &PyDictItems_Type,
    &PyDictIterItem_Type,
    &PyDictIterKey_Type,
    &PyDictIterValue_Type,
    &PyDictKeys_Type,
    &PyDictProxy_Type,
    &PyDictRevIterItem_Type,
    &PyDictRevIterKey_Type,
    &PyDictRevIterValue_Type,
    &PyDictValues_Type,
    &PyDict_Type,
    &PyEllipsis_Type,
    &PyEnum_Type,
    &PyFilter_Type,
    &PyFloat_Type,
    &PyFrameLocalsProxy_Type,
    &PyFrame_Type,
    &PyFrozenDict_Type,
    &PyFrozenSet_Type,
    &PyFunction_Type,
    &PyGen_Type,
    &PyGetSetDescr_Type,
    &PyInstanceMethod_Type,
    &PyLazyImport_Type,
    &PyListIter_Type,
    &PyListRevIter_Type,
    &PyList_Type,
    &PyLongRangeIter_Type,
    &PyLong_Type,
    &PyMap_Type,
    &PyMemberDescr_Type,
    &PyMemoryView_Type,
    &PyMethodDescr_Type,
    &PyMethod_Type,
    &PyModuleDef_Type,
    &PyModule_Type,
    &PyODictIter_Type,
    &PyPickleBuffer_Type,
    &PyProperty_Type,
    &PyRangeIter_Type,
    &PyRange_Type,
    &PyReversed_Type,
    &PySTEntry_Type,
    &PySentinel_Type,
    &PySeqIter_Type,
    &PySetIter_Type,
    &PySet_Type,
    &PySlice_Type,
    &PyStdPrinter_Type,
    &PySuper_Type,
    &PyTraceBack_Type,
    &PyTupleIter_Type,
    &PyTuple_Type,
    &PyUnicodeIter_Type,
    &PyUnicode_Type,
    &PyWrapperDescr_Type,
    &PyZip_Type,
    &Py_GenericAliasType,
    &_PyAsyncGenASend_Type,
    &_PyAsyncGenAThrow_Type,
    &_PyAsyncGenWrappedValue_Type,
    &_PyBufferWrapper_Type,
    &_PyContextTokenMissing_Type,
    &_PyCoroWrapper_Type,
    &_Py_GenericAliasIterType,
    &_PyHamtItems_Type,
    &_PyHamtKeys_Type,
    &_PyHamtValues_Type,
    &_PyHamt_ArrayNode_Type,
    &_PyHamt_BitmapNode_Type,
    &_PyHamt_CollisionNode_Type,
    &_PyHamt_Type,
    &_PyInstructionSequence_Type,
    &_PyInterpolation_Type,
    &_PyLegacyEventHandler_Type,
    &_PyLineIterator,
    &_PyManagedBuffer_Type,
    &_PyMemoryIter_Type,
    &_PyMethodWrapper_Type,
    &_PyNamespace_Type,
    &_PyNone_Type,
    &_PyNotImplemented_Type,
    &_PyPositionsIterator,
    &_PyTemplate_Type,
    &_PyTemplateIter_Type,
    &_PyUnicodeASCIIIter_Type,
    &_PyUnion_Type,
    &_PyWeakref_CallableProxyType,
    &_PyWeakref_ProxyType,
    &_PyWeakref_RefType,
    &_PyTypeAlias_Type,
    &_PyNoDefault_Type,

    // subclasses: _PyTypes_FiniTypes() deallocates them before their base
    // class
    &PyBool_Type,         // base=&PyLong_Type
    &PyCMethod_Type,      // base=&PyCFunction_Type
    &PyODictItems_Type,   // base=&PyDictItems_Type
    &PyODictKeys_Type,    // base=&PyDictKeys_Type
    &PyODictValues_Type,  // base=&PyDictValues_Type
    &PyODict_Type,        // base=&PyDict_Type
};


PyStatus
_PyTypes_InitTypes(PyInterpreterState *interp)
{
    // All other static types (unless initialized elsewhere)
    for (size_t i=0; i < Py_ARRAY_LENGTH(static_types); i++) {
        PyTypeObject *type = static_types[i];
        if (type == NULL) {
            continue;
        }
        if (_PyStaticType_InitBuiltin(interp, type) < 0) {
            return _PyStatus_ERR("Can't initialize builtin type");
        }
        if (type == &PyType_Type) {
            // Sanitify checks of the two most important types
            assert(PyBaseObject_Type.tp_base == NULL);
            assert(PyType_Type.tp_base == &PyBaseObject_Type);
        }
    }

    // Cache __reduce__ from PyBaseObject_Type object
    PyObject *baseobj_dict = _PyType_GetDict(&PyBaseObject_Type);
    PyValue baseobj_reduce_value;
    int found = _PyDict_GetValue((PyDictObject *)baseobj_dict,
                                _PyValue_FromObjectKnown(&_Py_ID(__reduce__)),
                                &baseobj_reduce_value);
    if (found < 0) {
        return _PyStatus_ERR("Can't get __reduce__ from base object");
    }
    /* object's own namespace holds only descriptors, never an int or float. */
    PyObject *baseobj_reduce = found > 0
        ? PyValue_AsKnownObjectSteal(baseobj_reduce_value) : NULL;
    /* This identity cache borrows the immutable builtin namespace's
     * descriptor. Dropping the extra lookup owner cannot finalize it. */
    assert(baseobj_reduce == NULL ||
           (Py_IS_TYPE(baseobj_reduce, &PyMethodDescr_Type) &&
            Py_REFCNT(baseobj_reduce) > 1));
    _Py_INTERP_CACHED_OBJECT(interp, objreduce) = baseobj_reduce;
    Py_CLEAR(baseobj_reduce);

    // Must be after static types are initialized
    if (_Py_initialize_generic(interp) < 0) {
        return _PyStatus_ERR("Can't initialize generic types");
    }

    return _PyStatus_OK();
}


// Best-effort function clearing static types.
//
// Don't deallocate a type if it still has subclasses. If a Py_Finalize()
// sub-function is interrupted by CTRL+C or fails with MemoryError, some
// subclasses are not cleared properly. Leave the static type unchanged in this
// case.
void
_PyTypes_FiniTypes(PyInterpreterState *interp)
{
    // Deallocate types in the reverse order to deallocate subclasses before
    // their base classes.
    for (Py_ssize_t i=Py_ARRAY_LENGTH(static_types)-1; i>=0; i--) {
        PyTypeObject *type = static_types[i];
        if (type == NULL) {
            continue;
        }
        _PyStaticType_FiniBuiltin(interp, type);
    }
}


static inline void
new_reference(PyObject *op)
{
    // Skip the immortal object check in Py_SET_REFCNT; always set refcnt to 1
#if !defined(Py_GIL_DISABLED)
#if SIZEOF_VOID_P > 4
    op->ob_refcnt_full = 1;
    assert(op->ob_refcnt == 1);
    assert(op->ob_flags == 0);
#else
    op->ob_refcnt = 1;
#endif
#else
    op->ob_flags = 0;
    op->ob_mutex = (PyMutex){ 0 };
#ifdef _Py_THREAD_SANITIZER
    _Py_atomic_store_uintptr_relaxed(&op->ob_tid, _Py_ThreadId());
    _Py_atomic_store_uint8_relaxed(&op->ob_gc_bits, 0);
    _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, 1);
    _Py_atomic_store_ssize_relaxed(&op->ob_ref_shared, 0);
#else
    op->ob_tid = _Py_ThreadId();
    op->ob_gc_bits = 0;
    op->ob_ref_local = 1;
    op->ob_ref_shared = 0;
#endif
#endif
#ifdef Py_TRACE_REFS
    _Py_AddToAllObjects(op);
#endif
    _PyReftracerTrack(op, PyRefTracer_CREATE);
}

void
_Py_NewReference(PyObject *op)
{
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    new_reference(op);
}

void
_Py_NewReferenceNoTotal(PyObject *op)
{
    new_reference(op);
}

void
_Py_SetImmortalUntracked(PyObject *op)
{
    // Check if already immortal to avoid degrading from static immortal to plain immortal
    if (_Py_IsImmortal(op)) {
        return;
    }
#ifdef Py_GIL_DISABLED
    _Py_atomic_store_uintptr_relaxed(&op->ob_tid, _Py_UNOWNED_TID);
    _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, _Py_IMMORTAL_REFCNT_LOCAL);
    _Py_atomic_store_ssize_relaxed(&op->ob_ref_shared, 0);
    _Py_atomic_or_uint8(&op->ob_gc_bits, _PyGC_BITS_DEFERRED);
#elif SIZEOF_VOID_P > 4
    op->ob_flags = _Py_IMMORTAL_FLAGS;
    op->ob_refcnt = _Py_IMMORTAL_INITIAL_REFCNT;
#else
    op->ob_refcnt = _Py_IMMORTAL_INITIAL_REFCNT;
#endif
}

void
_Py_SetImmortal(PyObject *op)
{
    if (PyObject_IS_GC(op) && _PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_UNTRACK(op);
    }
    _Py_SetImmortalUntracked(op);
}

void
_PyObject_SetDeferredRefcount(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    assert(PyType_IS_GC(Py_TYPE(op)));
    assert(_Py_IsOwnedByCurrentThread(op));
    assert(op->ob_ref_shared == 0);
    _PyObject_SET_GC_BITS(op, _PyGC_BITS_DEFERRED);
    op->ob_ref_shared = _Py_REF_SHARED(_Py_REF_DEFERRED, 0);
#endif
}

int
PyUnstable_Object_EnableDeferredRefcount(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    if (!PyType_IS_GC(Py_TYPE(op))) {
        // Deferred reference counting doesn't work
        // on untracked types.
        return 0;
    }

    if (!PyObject_GC_IsTracked(op)) {
        // When deferred refcount is enabled, the object will only be
        // deallocated by the tracing garbage collector. So it must be tracked
        // by the garbage collector.
        return 0;
    }

    uint8_t bits = _Py_atomic_load_uint8(&op->ob_gc_bits);
    if ((bits & _PyGC_BITS_DEFERRED) != 0)
    {
        // Nothing to do.
        return 0;
    }

    if (_Py_atomic_compare_exchange_uint8(&op->ob_gc_bits, &bits, bits | _PyGC_BITS_DEFERRED) == 0)
    {
        // Someone beat us to it!
        return 0;
    }
    _Py_atomic_add_ssize(&op->ob_ref_shared, _Py_REF_SHARED(_Py_REF_DEFERRED, 0));
    return 1;
#else
    return 0;
#endif
}

int
PyUnstable_Object_IsUniqueReferencedTemporary(PyObject *op)
{
    if (!_PyObject_IsUniquelyReferenced(op)) {
        return 0;
    }

    _PyInterpreterFrame *frame = _PyEval_GetFrame();
    if (frame == NULL) {
        return 0;
    }

    /* The temporaries; NULL ones are not objects */
    PyValue *base = _PyFrame_Stackbase(frame);
    PyValue *stackpointer = frame->base + _PyFrame_NumSlots(frame);
    while (stackpointer > base) {
        stackpointer--;
        PyValueBorrow ref = PyValue_Borrow(*stackpointer);
        if (!PyValue_IsObject(ref)) {
            continue;
        }
        if (op == PyValue_AsKnownObjectBorrow(ref)) {
            return PyValue_RefcountOnObject(ref);
        }
    }
    return 0;
}

int
PyUnstable_TryIncRef(PyObject *op)
{
    return _Py_TryIncref(op);
}

void
PyUnstable_EnableTryIncRef(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    _PyObject_SetMaybeWeakref(op);
#endif
}

int
PyUnstable_SetImmortal(PyObject *op)
{
    assert(op != NULL);
    if (!_PyObject_IsUniquelyReferenced(op) || PyUnicode_Check(op)) {
        return 0;
    }
    _Py_SetImmortal(op);
    return 1;
}

void
_Py_ResurrectReference(PyObject *op)
{
#ifdef Py_TRACE_REFS
    _Py_AddToAllObjects(op);
#endif
}

void
_Py_ForgetReference(PyObject *op)
{
#ifdef Py_TRACE_REFS
    if (Py_REFCNT(op) < 0) {
        _PyObject_ASSERT_FAILED_MSG(op, "negative refcnt");
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();

#ifdef SLOW_UNREF_CHECK
    if (!_PyRefchain_Get(interp, op)) {
        /* Not found */
        _PyObject_ASSERT_FAILED_MSG(op,
                                    "object not found in the objects list");
    }
#endif

    _PyRefchain_Remove(interp, op);
#endif
}


#ifdef Py_TRACE_REFS
static int
_Py_PrintReference(_Py_hashtable_t *ht,
                   const void *key, const void *value,
                   void *user_data)
{
    PyObject *op = (PyObject*)key;
    FILE *fp = (FILE *)user_data;
    fprintf(fp, "%p [%zd] ", (void *)op, Py_REFCNT(op));
    if (PyObject_Print(op, fp, 0) != 0) {
        PyErr_Clear();
    }
    putc('\n', fp);
    return 0;
}


/* Print all live objects.  Because PyObject_Print is called, the
 * interpreter must be in a healthy state.
 */
void
_Py_PrintReferences(PyInterpreterState *interp, FILE *fp)
{
    if (interp == NULL) {
        interp = _PyInterpreterState_Main();
    }
    fprintf(fp, "Remaining objects:\n");
    _Py_hashtable_foreach(REFCHAIN(interp), _Py_PrintReference, fp);
}


static int
_Py_PrintReferenceAddress(_Py_hashtable_t *ht,
                          const void *key, const void *value,
                          void *user_data)
{
    PyObject *op = (PyObject*)key;
    FILE *fp = (FILE *)user_data;
    fprintf(fp, "%p [%zd] %s\n",
            (void *)op, Py_REFCNT(op), Py_TYPE(op)->tp_name);
    return 0;
}


/* Print the addresses of all live objects.  Unlike _Py_PrintReferences, this
 * doesn't make any calls to the Python C API, so is always safe to call.
 */
// XXX This function is not safe to use if the interpreter has been
// freed or is in an unhealthy state (e.g. late in finalization).
// The call in Py_FinalizeEx() is okay since the main interpreter
// is statically allocated.
void
_Py_PrintReferenceAddresses(PyInterpreterState *interp, FILE *fp)
{
    fprintf(fp, "Remaining object addresses:\n");
    _Py_hashtable_foreach(REFCHAIN(interp), _Py_PrintReferenceAddress, fp);
}


typedef struct {
    PyObject *self;
    PyObject *args;
    PyObject *list;
    PyObject *type;
    Py_ssize_t limit;
} _Py_GetObjectsData;

enum {
    _PY_GETOBJECTS_IGNORE = 0,
    _PY_GETOBJECTS_ERROR = 1,
    _PY_GETOBJECTS_STOP = 2,
};

static int
_Py_GetObject(_Py_hashtable_t *ht,
              const void *key, const void *value,
              void *user_data)
{
    PyObject *op = (PyObject *)key;
    _Py_GetObjectsData *data = user_data;
    if (data->limit > 0) {
        if (PyList_GET_SIZE(data->list) >= data->limit) {
            return _PY_GETOBJECTS_STOP;
        }
    }

    if (op == data->self) {
        return _PY_GETOBJECTS_IGNORE;
    }
    if (op == data->args) {
        return _PY_GETOBJECTS_IGNORE;
    }
    if (op == data->list) {
        return _PY_GETOBJECTS_IGNORE;
    }
    if (data->type != NULL) {
        if (op == data->type) {
            return _PY_GETOBJECTS_IGNORE;
        }
        if (!Py_IS_TYPE(op, (PyTypeObject *)data->type)) {
            return _PY_GETOBJECTS_IGNORE;
        }
    }

    if (PyList_Append(data->list, op) < 0) {
        return _PY_GETOBJECTS_ERROR;
    }
    return 0;
}


/* The implementation of sys.getobjects(). */
PyObject *
_Py_GetObjects(PyObject *self, PyObject *args)
{
    Py_ssize_t limit;
    PyObject *type = NULL;
    if (!PyArg_ParseTuple(args, "n|O", &limit, &type)) {
        return NULL;
    }

    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }

    _Py_GetObjectsData data = {
        .self = self,
        .args = args,
        .list = list,
        .type = type,
        .limit = limit,
    };
    PyInterpreterState *interp = _PyInterpreterState_GET();
    int res = _Py_hashtable_foreach(REFCHAIN(interp), _Py_GetObject, &data);
    if (res == _PY_GETOBJECTS_ERROR) {
        Py_DECREF(list);
        return NULL;
    }
    return list;
}

#undef REFCHAIN
#undef REFCHAIN_VALUE

#endif  /* Py_TRACE_REFS */


void
_PyObject_DebugTypeStats(FILE *out)
{
    _PyDict_DebugMallocStats(out);
    _PyFloat_DebugMallocStats(out);
    _PyList_DebugMallocStats(out);
    _PyTuple_DebugMallocStats(out);
}

/* These methods are used to control infinite recursion in repr, str, print,
   etc.  Container objects that may recursively contain themselves,
   e.g. builtin dictionaries and lists, should use Py_ReprEnter() and
   Py_ReprLeave() to avoid infinite recursion.

   Py_ReprEnter() returns 0 the first time it is called for a particular
   object and 1 every time thereafter.  It returns -1 if an exception
   occurred.  Py_ReprLeave() has no return value.

   See dictobject.c and listobject.c for examples of use.
*/

int
Py_ReprEnter(PyObject *obj)
{
    /* The caller owns obj; the thread-state dict lives as long as the thread. */
    PyValueBorrow value = PyValue_FromObjectBorrow(obj);
    PyValue guard = PyValue_NULL;
    PyObject *list = NULL;
    PyObject *candidate = NULL;
    PyObject *dict = PyThreadState_GetDict();
    int result = -1;
    /* Ignore a missing thread-state, so that this function can be called
       early on startup. */
    if (dict == NULL) {
        result = 0;
        goto done;
    }
    PyValueBorrow name = PyValue_FromObjectBorrow(&_Py_ID(Py_Repr));
    Py_hash_t hash = _PyValue_Hash(_PyValue_FromObjectKnown(&_Py_ID(Py_Repr)));
    if (hash == -1) {
        goto done;
    }
    int found = _PyDict_GetValue_KnownHash((PyDictObject *)dict, name, hash, &guard);
    if (found < 0) {
        goto done;
    }
    if (!found) {
        candidate = PyList_New(0);
        if (candidate == NULL) {
            goto done;
        }
        found = _PyDict_SetDefaultValue(dict, name,
                                        PyValue_FromObjectBorrow(candidate), &guard);
        Py_CLEAR(candidate);
        if (found < 0) {
            goto done;
        }
    }
    if (!PyValue_IsObject(guard) ||
        !PyList_Check(PyValue_AsKnownObjectBorrow(guard))) {
        PyErr_SetString(PyExc_SystemError, "invalid thread repr guard");
        goto done;
    }
    list = PyValue_AsKnownObjectSteal(guard);
    guard = PyValue_NULL;
    for (Py_ssize_t i = PyList_GET_SIZE(list); --i >= 0;) {
        if (PyValue_Is(PyList_GET_VALUE(list, i), value)) {
            result = 1;
            goto done;
        }
    }
    result = _PyList_AppendTakeValue((PyListObject *)list, PyValue_DUP(value));
done:
    Py_CLEAR(candidate);
    Py_CLEAR(list);
    PyValue_XCLOSE(guard);
    return result;
}

void
Py_ReprLeave(PyObject *obj)
{
    PyObject *exc = PyErr_GetRaisedException();
    /* The caller owns obj; the thread-state dict lives as long as the thread. */
    PyValueBorrow value = PyValue_FromObjectBorrow(obj);
    PyValue guard = PyValue_NULL;
    PyObject *list = NULL;
    PyObject *dict = PyThreadState_GetDict();
    if (dict == NULL)
        goto finally;
    Py_hash_t hash = _PyValue_Hash(_PyValue_FromObjectKnown(&_Py_ID(Py_Repr)));
    if (hash == -1 || _PyDict_GetValue_KnownHash((PyDictObject *)dict,
            PyValue_FromObjectBorrow(&_Py_ID(Py_Repr)), hash, &guard) <= 0) {
        goto finally;
    }
    if (!PyValue_IsObject(guard) ||
        !PyList_Check(PyValue_AsKnownObjectBorrow(guard))) {
        goto finally;
    }
    list = PyValue_AsKnownObjectSteal(guard);
    guard = PyValue_NULL;
    /* Count backwards because we always expect obj to be list[-1] */
    for (Py_ssize_t i = PyList_GET_SIZE(list); --i >= 0;) {
        if (PyValue_Is(PyList_GET_VALUE(list, i), value)) {
            PyList_SetSlice(list, i, i + 1, NULL);
            break;
        }
    }

finally:
    Py_XDECREF(list);
    PyValue_XCLOSE(guard);
    /* ignore exceptions because there is no way to report them. */
    PyErr_SetRaisedException(exc);
}

/* Trashcan support. */

/* Add op to the tstate->delete_later list.  Called when the current
 * call-stack depth gets large.  op must be a gc'ed object, with refcount 0.
 *  Py_DECREF must already have been called on it.
 */
void
_PyTrash_thread_deposit_object(PyThreadState *tstate, PyObject *op)
{
    _PyObject_ASSERT(op, Py_REFCNT(op) == 0);
    PyTypeObject *tp = Py_TYPE(op);
    assert(tp->tp_flags & Py_TPFLAGS_HAVE_GC);
    int tracked = 0;
    if (tp->tp_is_gc == NULL || tp->tp_is_gc(op)) {
        tracked = _PyObject_GC_IS_TRACKED(op);
        if (tracked) {
            _PyObject_GC_UNTRACK(op);
        }
    }
    uintptr_t tagged_ptr = ((uintptr_t)tstate->delete_later) | tracked;
#ifdef Py_GIL_DISABLED
    op->ob_tid = tagged_ptr;
#else
    _Py_AS_GC(op)->_gc_next = tagged_ptr;
#endif
    tstate->delete_later = op;
}

/* Deallocate all the objects in the tstate->delete_later list.
 * Called when the call-stack unwinds again. */
void
_PyTrash_thread_destroy_chain(PyThreadState *tstate)
{
    while (tstate->delete_later) {
        PyObject *op = tstate->delete_later;
        destructor dealloc = Py_TYPE(op)->tp_dealloc;

#ifdef Py_GIL_DISABLED
        uintptr_t tagged_ptr = op->ob_tid;
        op->ob_tid = 0;
        _Py_atomic_store_ssize_relaxed(&op->ob_ref_shared, _Py_REF_MERGED);
#else
        uintptr_t tagged_ptr = _Py_AS_GC(op)->_gc_next;
        _Py_AS_GC(op)->_gc_next = 0;
#endif
        tstate->delete_later = (PyObject *)(tagged_ptr & ~1);
        if (tagged_ptr & 1) {
            _PyObject_GC_TRACK(op);
        }
        /* Call the deallocator directly.  This used to try to
         * fool Py_DECREF into calling it indirectly, but
         * Py_DECREF was already called on this object, and in
         * assorted non-release builds calling Py_DECREF again ends
         * up distorting allocation statistics.
         */
        _PyObject_ASSERT(op, Py_REFCNT(op) == 0);
#ifdef Py_TRACE_REFS
        _Py_ForgetReference(op);
#endif
        _PyReftracerTrack(op, PyRefTracer_DESTROY);
        (*dealloc)(op);
    }
}

void _Py_NO_RETURN
_PyObject_AssertFailed(PyObject *obj, const char *expr, const char *msg,
                       const char *file, int line, const char *function)
{
    fprintf(stderr, "%s:%d: ", file, line);
    if (function) {
        fprintf(stderr, "%s: ", function);
    }
    fflush(stderr);

    if (expr) {
        fprintf(stderr, "Assertion \"%s\" failed", expr);
    }
    else {
        fprintf(stderr, "Assertion failed");
    }
    fflush(stderr);

    if (msg) {
        fprintf(stderr, ": %s", msg);
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    if (_PyObject_IsFreed(obj)) {
        /* It seems like the object memory has been freed:
           don't access it to prevent a segmentation fault. */
        fprintf(stderr, "<object at %p is freed>\n", obj);
        fflush(stderr);
    }
    else {
        /* Display the traceback where the object has been allocated.
           Do it before dumping repr(obj), since repr() is more likely
           to crash than dumping the traceback. */
        PyTypeObject *type = Py_TYPE(obj);
        const size_t presize = _PyType_PreHeaderSize(type);
        void *ptr = (void *)((char *)obj - presize);
        _PyMem_DumpTraceback(fileno(stderr), ptr);

        /* This might succeed or fail, but we're about to abort, so at least
           try to provide any extra info we can: */
        PyObject_Dump(obj);

        fprintf(stderr, "\n");
        fflush(stderr);
    }

    Py_FatalError("_PyObject_AssertFailed");
}


/*
When deallocating a container object, it's possible to trigger an unbounded
chain of deallocations, as each Py_DECREF in turn drops the refcount on "the
next" object in the chain to 0.  This can easily lead to stack overflows.
To avoid that, if the C stack is nearing its limit, instead of calling
dealloc on the object, it is added to a queue to be freed later when the
stack is shallower */
static Py_NO_INLINE void
py_dealloc(PyObject *op)
{
    PyTypeObject *type = Py_TYPE(op);
    unsigned long gc_flag = type->tp_flags & Py_TPFLAGS_HAVE_GC;
    destructor dealloc = type->tp_dealloc;
    PyThreadState *tstate = NULL;
    intptr_t margin = 0;
    if (gc_flag) {
        tstate = _PyThreadState_GET();
        margin = _Py_RecursionLimit_GetMargin(tstate);
        if (margin < 2) {
            _PyTrash_thread_deposit_object(tstate, (PyObject *)op);
            return;
        }
    }
#ifdef Py_DEBUG
    if (tstate == NULL) {
        tstate = _PyThreadState_GET();
    }
    PyObject *old_exc = tstate != NULL ? tstate->current_exception : NULL;
    // Keep the old exception type alive to prevent undefined behavior
    // on (tstate->curexc_type != old_exc_type) below
    Py_XINCREF(old_exc);
    // Make sure that type->tp_name remains valid
    Py_INCREF(type);
#endif

#ifdef Py_TRACE_REFS
    _Py_ForgetReference(op);
#endif
    _PyReftracerTrack(op, PyRefTracer_DESTROY);
    (*dealloc)(op);

#ifdef Py_DEBUG
    // gh-89373: The tp_dealloc function must leave the current exception
    // unchanged.
    if (tstate != NULL && tstate->current_exception != old_exc) {
        const char *err;
        if (old_exc == NULL) {
            err = "Deallocator of type '%s' raised an exception";
        }
        else if (tstate->current_exception == NULL) {
            err = "Deallocator of type '%s' cleared the current exception";
        }
        else {
            // It can happen if dealloc() normalized the current exception.
            // A deallocator function must not change the current exception,
            // not even normalize it.
            err = "Deallocator of type '%s' overrode the current exception";
        }
        _Py_FatalErrorFormat(__func__, err, type->tp_name);
    }
    Py_XDECREF(old_exc);
    Py_DECREF(type);
#endif
    if (gc_flag && tstate->delete_later && margin >= 4) {
        _PyTrash_thread_destroy_chain(tstate);
    }
}

/*
 * gh-130706: Keep the GC/reftracer path in the non-inlined py_dealloc().
 * Inlining it makes the compiler save callee-saved registers at entry,
 * so the non-GC objects path would have unnecessary register spills.
 */
void
_Py_Dealloc(PyObject *op)
{
#if !defined(Py_DEBUG) && !defined(Py_TRACE_REFS)
    // gh-130706: Avoid unnecessary register spills for non-GC objects.
    PyTypeObject *type = Py_TYPE(op);
    if (_PyRuntime.ref_tracer.tracer_func == NULL
        && !(type->tp_flags & Py_TPFLAGS_HAVE_GC))
    {
        type->tp_dealloc(op);
        return;
    }
#endif
    // GC objects (trashcan), reftracer set, or debug builds.
    py_dealloc(op);
}


PyObject **
PyObject_GET_WEAKREFS_LISTPTR(PyObject *op)
{
    return _PyObject_GET_WEAKREFS_LISTPTR(op);
}


#undef Py_NewRef
#undef Py_XNewRef

// Export Py_NewRef() and Py_XNewRef() as regular functions for the stable ABI.
PyObject*
Py_NewRef(PyObject *obj)
{
    return _Py_NewRef(obj);
}

PyObject*
Py_XNewRef(PyObject *obj)
{
    return _Py_XNewRef(obj);
}

#undef Py_Is
#undef Py_IsNone
#undef Py_IsTrue
#undef Py_IsFalse

// Export Py_Is(), Py_IsNone(), Py_IsTrue(), Py_IsFalse() as regular functions
// for the stable ABI.
int Py_Is(PyObject *x, PyObject *y)
{
    return (x == y);
}

int Py_IsNone(PyObject *x)
{
    return Py_Is(x, Py_None);
}

int Py_IsTrue(PyObject *x)
{
    return Py_Is(x, Py_True);
}

int Py_IsFalse(PyObject *x)
{
    return Py_Is(x, Py_False);
}


// Py_SET_REFCNT() implementation for stable ABI
void
_Py_SetRefcnt(PyObject *ob, Py_ssize_t refcnt)
{
    Py_SET_REFCNT(ob, refcnt);
}

int PyRefTracer_SetTracer(PyRefTracer tracer, void *data) {
    _Py_AssertHoldsTstate();

    _PyEval_StopTheWorldAll(&_PyRuntime);
    if (_PyRuntime.ref_tracer.tracer_func != NULL) {
        _PyReftracerTrack(NULL, PyRefTracer_TRACKER_REMOVED);
        if (PyErr_Occurred()) {
            _PyEval_StartTheWorldAll(&_PyRuntime);
            return -1;
        }
    }
    _PyRuntime.ref_tracer.tracer_func = tracer;
    _PyRuntime.ref_tracer.tracer_data = data;
    _PyEval_StartTheWorldAll(&_PyRuntime);
    return 0;
}

PyRefTracer PyRefTracer_GetTracer(void** data) {
    _Py_AssertHoldsTstate();
    if (data != NULL) {
        *data = _PyRuntime.ref_tracer.tracer_data;
    }
    return _PyRuntime.ref_tracer.tracer_func;
}



static PyObject* constants[] = {
    &_Py_NoneStruct,                   // Py_CONSTANT_NONE
    (PyObject*)(&_Py_FalseStruct),     // Py_CONSTANT_FALSE
    (PyObject*)(&_Py_TrueStruct),      // Py_CONSTANT_TRUE
    &_Py_EllipsisObject,               // Py_CONSTANT_ELLIPSIS
    &_Py_NotImplementedStruct,         // Py_CONSTANT_NOT_IMPLEMENTED
    NULL,  // Py_CONSTANT_ZERO
    NULL,  // Py_CONSTANT_ONE
    NULL,  // Py_CONSTANT_EMPTY_STR
    NULL,  // Py_CONSTANT_EMPTY_BYTES
    NULL,  // Py_CONSTANT_EMPTY_TUPLE
};

void
_Py_GetConstant_Init(void)
{
    constants[Py_CONSTANT_ZERO] = _PyLong_GetZero();
    constants[Py_CONSTANT_ONE] = _PyLong_GetOne();
    constants[Py_CONSTANT_EMPTY_STR] = PyUnicode_New(0, 0);
    constants[Py_CONSTANT_EMPTY_BYTES] = PyBytes_FromStringAndSize("", 0);
    constants[Py_CONSTANT_EMPTY_TUPLE] = PyTuple_New(0);
#ifndef NDEBUG
    for (size_t i=0; i < Py_ARRAY_LENGTH(constants); i++) {
        assert(constants[i] != NULL);
        assert(_Py_IsImmortal(constants[i]));
    }
#endif
}

PyObject*
Py_GetConstant(unsigned int constant_id)
{
    if (constant_id < Py_ARRAY_LENGTH(constants)) {
        return constants[constant_id];
    }
    else {
        PyErr_BadInternalCall();
        return NULL;
    }
}


PyObject*
Py_GetConstantBorrowed(unsigned int constant_id)
{
    // All constants are immortal
    return Py_GetConstant(constant_id);
}

int
PyUnstable_IsImmortal(PyObject *op)
{
    /* Checking a reference count requires a thread state */
    _Py_AssertHoldsTstate();
    assert(op != NULL);
    return _Py_IsImmortal(op);
}

int
PyUnstable_Object_IsUniquelyReferenced(PyObject *op)
{
    _Py_AssertHoldsTstate();
    assert(op != NULL);
    return _PyObject_IsUniquelyReferenced(op);
}

int
_PyObject_VisitType(PyObject *op, visitproc visit, void *arg)
{
    assert(op != NULL);
    PyTypeObject *tp = Py_TYPE(op);
    _PyObject_ASSERT((PyObject *)tp, PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE));
    Py_VISIT(tp);
    return 0;
}

// Implementations for the stable ABI
// Keep these at the end.
#undef Py_TYPE
#undef Py_REFCNT
#undef Py_SIZE
#undef Py_IS_TYPE
#undef Py_SET_SIZE
PyTypeObject* Py_TYPE(PyObject *ob) { return _Py_TYPE_impl(ob); }
Py_ssize_t Py_REFCNT(PyObject *ob) { return _Py_REFCNT(ob); }
Py_ssize_t Py_SIZE(PyObject *o) { return _Py_SIZE_impl(o); }
int Py_IS_TYPE(PyObject *o, PyTypeObject *t) { return _Py_IS_TYPE_impl(o, t); }
void Py_SET_SIZE(PyVarObject *o, Py_ssize_t s) { _Py_SET_SIZE_impl(o, s); }
