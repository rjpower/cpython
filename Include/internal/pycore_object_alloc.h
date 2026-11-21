#ifndef Py_INTERNAL_OBJECT_ALLOC_H
#define Py_INTERNAL_OBJECT_ALLOC_H

#include "pycore_object.h"      // _PyType_HasFeature()
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include "pycore_tstate.h"      // _PyThreadStateImpl

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef WITH_MIMALLOC
static inline mi_heap_t *
_PyObject_GetAllocationHeap(_PyThreadStateImpl *tstate, PyTypeObject *tp)
{
    struct _mimalloc_thread_state *m = &tstate->mimalloc;
    if (_PyType_HasFeature(tp, Py_TPFLAGS_PREHEADER)) {
        return &m->heaps[_Py_MIMALLOC_HEAP_GC_PRE];
    }
    else if (_PyType_IS_GC(tp)) {
        return &m->heaps[_Py_MIMALLOC_HEAP_GC];
    }
    else {
        return &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
    }
}
#endif

#if defined(WITH_MIMALLOC) && !defined(Py_GIL_DISABLED)
// The object allocator's slow route, for an allocator that is not the stock
// mimalloc one (the debug hooks, tracemalloc, PyMem_SetAllocator()) and for
// a thread that has no heap of its own; see _PyObject_MallocWithType().
extern void *_PyObject_MallocWithTypeHooked(PyTypeObject *tp, size_t size);
extern void *_PyObject_MiMalloc(void *ctx, size_t size);
#endif

// Allocate size bytes for an object of type tp from the mimalloc heap that
// matches it. We use different heaps depending on if the object supports GC
// and if it has a pre-header, so that a collector visiting a heap knows
// where its objects start.
//
// In the default build the stock allocator is called directly with the heap
// that tp selects. An allocator that hooks the object domain takes no heap
// argument, so there the choice is smuggled through the thread state's
// current_object_heap (_PyObject_MallocWithTypeHooked()).
static inline void *
_PyObject_MallocWithType(PyTypeObject *tp, size_t size)
{
#ifdef WITH_MIMALLOC
#  ifndef Py_GIL_DISABLED
    if (_PyRuntime.allocators.standard.obj.malloc == _PyObject_MiMalloc) {
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
        // An unbound thread state has zeroed heaps, which no thread owns.
        if (tstate != NULL) {
            mi_heap_t *heap = _PyObject_GetAllocationHeap(tstate, tp);
            if (heap->thread_id == _mi_prim_thread_id()) {
                return mi_heap_malloc(heap, size);
            }
        }
    }
    return _PyObject_MallocWithTypeHooked(tp, size);
#  else
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    struct _mimalloc_thread_state *m = &tstate->mimalloc;
    m->current_object_heap = _PyObject_GetAllocationHeap(tstate, tp);
    void *mem = PyObject_Malloc(size);
    m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
    return mem;
#  endif
#else
    return PyObject_Malloc(size);
#endif
}

static inline void *
_PyObject_ReallocWithType(PyTypeObject *tp, void *ptr, size_t size)
{
#ifdef WITH_MIMALLOC
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
#  ifndef Py_GIL_DISABLED
    if (tstate == NULL || tstate->mimalloc.current_object_heap == NULL) {
        return PyObject_Realloc(ptr, size);
    }
#  endif
    struct _mimalloc_thread_state *m = &tstate->mimalloc;
    m->current_object_heap = _PyObject_GetAllocationHeap(tstate, tp);
#endif
    void *mem = PyObject_Realloc(ptr, size);
#ifdef WITH_MIMALLOC
    m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
#endif
    return mem;
}

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OBJECT_ALLOC_H
