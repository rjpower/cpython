#ifndef Py_INTERNAL_TRANSITION_ROOTS_H
#define Py_INTERNAL_TRANSITION_ROOTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interpframe_structs.h" // _PyInterpreterFrame
#include "pycore_ir.h"           // _PyIR_RETIRE_*, _PyIR_SHADOW_*
#include "pycore_tstate.h"       // _PyThreadStateImpl
#include "pycore_value.h"        // PyValue

enum {
    _Py_TRANSITION_KIND_EXTERNAL,
    _Py_TRANSITION_KIND_RETIRING,
    _Py_TRANSITION_KIND_STAGED,
    /* Active-frame selection metadata, never an external owner. */
    _Py_TRANSITION_KIND_ERROR,
};

enum {
    _Py_TRANSITION_CODE = 1,
    _Py_TRANSITION_FUNCTION = 2,
    _Py_TRANSITION_LOCALS = 4,
    _Py_TRANSITION_FRAME_OBJECT = 8,
    _Py_TRANSITION_EXCEPTION = 16,
};

/* The adapter installs each value before advancing end, and advances begin
 * before closing or transferring it. Entries outside [begin, end) are unread. */
typedef struct {
    PyValue *base;
    int capacity;
    int begin;
    int end;
} _PyTransitionPrefix;

/* External owners during an outlined, potentially reentrant retirement.
 * Frame presence selects readable values within [0, remaining).
 * A producer advances remaining or removes a header bit before closing that
 * owner. Code stays pinned through the last home/map read. Once code closes,
 * a walker uses only these explicit fields, never frame geometry.
 *
 * Storage for the record, frame and homes must outlive nested callbacks.
 * Pop precedes release of the frame's arena reservation. Heap-frame takeover
 * transfers the prefix and header authority before any subsequent callback.
 * A staged provider scope may outlive its consumed frame only after disarming
 * all authority and setting frame=NULL; the empty node stays linked until pop.
 */
typedef struct _PyTransitionRoots {
    struct _PyTransitionRoots *previous;
    _PyInterpreterFrame *frame;
    int remaining;
    unsigned char header_mask;
    unsigned char result_present:1;
    unsigned char kind:2;
    /* Only the owning constructor scope may consume this shim node. */
    unsigned char caller_staged:1;
    unsigned char home_authority:2;
    /* Producer outcome, retained after code detach; observers never recheck. */
    unsigned char retire_reason;
    unsigned char shadow_status;
    PyValue result;
    union {
        _PyTransitionPrefix *prefix;  /* EXTERNAL only */
        const _PyIRHomeView *homes;   /* RETIRING/STAGED/ERROR */
    };
} _PyTransitionRoots;

#if SIZEOF_VOID_P == 8
static_assert(offsetof(_PyTransitionRoots, retire_reason) == 22,
              "retirement reason must use existing record padding");
static_assert(offsetof(_PyTransitionRoots, shadow_status) == 23,
              "shadow status must use existing record padding");
static_assert(sizeof(_PyTransitionRoots) == 40,
              "retirement observation must not grow the record");
#endif

static inline void
_PyThreadState_PushTransitionRoots(PyThreadState *tstate,
                                  _PyTransitionRoots *roots)
{
    assert(roots->frame != NULL);
    assert(roots->remaining >= 0);
    assert(roots->kind <= _Py_TRANSITION_KIND_ERROR);
    assert(!roots->caller_staged ||
           roots->kind == _Py_TRANSITION_KIND_STAGED);
    assert((roots->header_mask & ~31) == 0);
    assert(roots->retire_reason <= _PyIR_RETIRE_STAGED_FAILURE);
    assert(roots->shadow_status <= _PyIR_SHADOW_MISMATCH);
    assert(roots->retire_reason != _PyIR_RETIRE_NONE ||
           roots->shadow_status == _PyIR_SHADOW_UNCOVERED);
    assert(!(roots->header_mask & _Py_TRANSITION_EXCEPTION) ||
           roots->frame->owner == FRAME_OWNED_BY_GENERATOR);
    assert(roots->kind == _Py_TRANSITION_KIND_ERROR || roots->remaining == 0 ||
           (roots->header_mask & _Py_TRANSITION_CODE));
    if (roots->kind == _Py_TRANSITION_KIND_ERROR) {
        assert(roots->header_mask == 0 && !roots->result_present);
        assert(roots->home_authority == _PyIR_HOME_MUST_READABLE);
        assert(roots->homes != NULL);
    }
    else if (roots->kind != _Py_TRANSITION_KIND_EXTERNAL) {
        assert(roots->home_authority != _PyIR_HOME_SHADOW_CANDIDATE);
        if (roots->home_authority == _PyIR_HOME_NONE) {
            assert(roots->remaining == 0 && roots->homes == NULL);
        }
        else if (roots->home_authority == _PyIR_HOME_FRAME_PRESENCE) {
            assert(roots->homes == NULL);
        }
        else {
            assert(roots->home_authority == _PyIR_HOME_MUST_READABLE);
            assert(roots->homes != NULL);
            assert(roots->header_mask & _Py_TRANSITION_CODE);
        }
    }
    else {
        assert(roots->home_authority == _PyIR_HOME_NONE);
        assert(roots->remaining == 0);
    }
    if (roots->kind == _Py_TRANSITION_KIND_EXTERNAL && roots->prefix != NULL) {
        assert(roots->prefix->base != NULL);
        assert(roots->prefix->begin >= 0);
        assert(roots->prefix->begin <= roots->prefix->end);
        assert(roots->prefix->end <= roots->prefix->capacity);
    }
    roots->previous = ((_PyThreadStateImpl *)tstate)->transition_roots;
    ((_PyThreadStateImpl *)tstate)->transition_roots = roots;
}

static inline void
_PyThreadState_PopTransitionRoots(PyThreadState *tstate,
                                 _PyTransitionRoots *roots)
{
    assert(((_PyThreadStateImpl *)tstate)->transition_roots == roots);
    assert(roots->kind <= _Py_TRANSITION_KIND_ERROR);
    assert(roots->frame != NULL || roots->kind == _Py_TRANSITION_KIND_STAGED ||
           roots->kind == _Py_TRANSITION_KIND_ERROR);
    assert(!roots->caller_staged);
    assert(roots->remaining == 0);
    assert(roots->header_mask == 0);
    assert(!roots->result_present);
    assert(roots->home_authority == _PyIR_HOME_NONE);
    if (roots->kind == _Py_TRANSITION_KIND_EXTERNAL) {
        assert(roots->prefix == NULL ||
               roots->prefix->begin == roots->prefix->end);
    }
    else {
        assert(roots->homes == NULL);
    }
    ((_PyThreadStateImpl *)tstate)->transition_roots = roots->previous;
}

/* Mark external roots only. Never use this visitor for cyclic-GC edge
 * subtraction: moving a heap edge here removes that container edge first. */
PyAPI_FUNC(int) _PyGC_TraceVisitTransitionRoots(PyThreadState *tstate,
                                              visitproc visit, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TRANSITION_ROOTS_H */
