#ifndef Py_INTERNAL_WEAKREF_H
#define Py_INTERNAL_WEAKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_lock.h"             // PyMutex_LockFlags()
#include "pycore_object.h"           // _Py_REF_IS_MERGED()
#include "pycore_pyatomic_ft_wrappers.h"

// Called from deallocators.  The reference count is constant.
#  define _PyWeakref_ASSERT_DEAD(obj) ((void)0)


#define LOCK_WEAKREFS(obj)
#define UNLOCK_WEAKREFS(obj)

#define LOCK_WEAKREFS_FOR_WR(wr)
#define UNLOCK_WEAKREFS_FOR_WR(wr)

// A deallocator clears the weak references to the object it destroys.  The
// header flag says whether the weak reference table holds a list for it.
#define FT_CLEAR_WEAKREFS(obj)                                      \
    do {                                                            \
        _PyWeakref_ASSERT_DEAD(obj);                                \
        if (_PyObject_CAST(obj)->ob_flags & _Py_HAS_WEAKREFS_FLAG) { \
            PyObject_ClearWeakRefs(obj);                            \
        }                                                           \
    } while (0)


static inline PyObject* _PyWeakref_GET_REF(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);

    PyObject *obj = FT_ATOMIC_LOAD_PTR(ref->wr_object);
    if (obj == Py_None) {
        // clear_weakref() was called
        return NULL;
    }

    LOCK_WEAKREFS(obj);
    if (_Py_TryIncref(obj)) {
        UNLOCK_WEAKREFS(obj);
        return obj;
    }
    UNLOCK_WEAKREFS(obj);
    return NULL;
}

static inline int _PyWeakref_IS_DEAD(PyObject *ref_obj)
{
    assert(PyWeakref_Check(ref_obj));
    PyWeakReference *ref = _Py_CAST(PyWeakReference*, ref_obj);
    // The collector clears weak references to an object before it is
    // swept, so a reference is dead exactly when it has been cleared.
    return FT_ATOMIC_LOAD_PTR(ref->wr_object) == Py_None;
}

extern Py_ssize_t _PyWeakref_GetWeakrefCount(PyObject *obj);

// The address of the head of obj's list of weak references, which stays
// valid while obj lives: an entry in the weak reference table
// (Objects/weakrefobject.c), or a static builtin type's per-interpreter
// list.  If obj has no entry, NULL, or with create set a new entry, or NULL
// with MemoryError.
extern PyWeakReference **_PyWeakref_ListPtr(PyObject *obj, int create);

// Remove the entry of obj, which has _Py_HAS_WEAKREFS_FLAG, from the weak
// reference table: its block is being freed.  Any reference still in its
// list is cleared without its callback.
extern void _PyWeakref_DropList(PyObject *obj);

// Clear all the weak references to obj but leave their callbacks uncalled and
// intact.
extern void _PyWeakref_ClearWeakRefsNoCallbacks(PyObject *obj);

PyAPI_FUNC(int) _PyWeakref_IsDead(PyObject *weakref);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_WEAKREF_H */
