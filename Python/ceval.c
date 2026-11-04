/* Execute compiled code */

#include "ceval.h"
#include "pycore_jit.h"
#include "pycore_long.h"
#include "pycore_native_scope.h"

#ifdef HAVE_PTHREAD_NP_H
#  include <pthread_np.h>         // pthread_attr_get_np()
#endif


int
Py_GetRecursionLimit(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return interp->ceval.recursion_limit;
}

void
Py_SetRecursionLimit(int new_limit)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    interp->ceval.recursion_limit = new_limit;
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        int depth = p->py_recursion_limit - p->py_recursion_remaining;
        p->py_recursion_limit = new_limit;
        p->py_recursion_remaining = new_limit - depth;
    }
    _Py_FOR_EACH_TSTATE_END(interp);
    _PyEval_StartTheWorld(interp);
}

int
_Py_ReachedRecursionLimitWithMargin(PyThreadState *tstate, int margin_count)
{
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
#if _Py_STACK_GROWS_DOWN
    if (here_addr > _tstate->c_stack_soft_limit + margin_count * _PyOS_STACK_MARGIN_BYTES) {
#else
    if (here_addr <= _tstate->c_stack_soft_limit - margin_count * _PyOS_STACK_MARGIN_BYTES) {
#endif
        return 0;
    }
    if (_tstate->c_stack_hard_limit == 0) {
        _Py_InitializeRecursionLimits(tstate);
    }
#if _Py_STACK_GROWS_DOWN
    return here_addr <= _tstate->c_stack_soft_limit + margin_count * _PyOS_STACK_MARGIN_BYTES &&
        here_addr >= _tstate->c_stack_soft_limit - 2 * _PyOS_STACK_MARGIN_BYTES;
#else
    return here_addr > _tstate->c_stack_soft_limit - margin_count * _PyOS_STACK_MARGIN_BYTES &&
        here_addr <= _tstate->c_stack_soft_limit + 2 * _PyOS_STACK_MARGIN_BYTES;
#endif
}

#if defined(_Py_LINKER_THREAD_STACK_SIZE)
#  define Py_C_STACK_SIZE _Py_LINKER_THREAD_STACK_SIZE
#elif defined(__s390x__)
#  define Py_C_STACK_SIZE 320000
#elif defined(_WIN32)
   // Don't define Py_C_STACK_SIZE, ask the O/S
#elif defined(__ANDROID__)
#  define Py_C_STACK_SIZE 1200000
#elif defined(__sparc__)
#  define Py_C_STACK_SIZE 1600000
#elif defined(__hppa__) || defined(__powerpc64__)
#  define Py_C_STACK_SIZE 2000000
#else
#  define Py_C_STACK_SIZE 4000000
#endif

#if defined(__EMSCRIPTEN__)

// Temporary workaround to make `pthread_getattr_np` work on Emscripten.
// Emscripten 4.0.6 will contain a fix:
// https://github.com/emscripten-core/emscripten/pull/23887

#include "emscripten/stack.h"

#define pthread_attr_t workaround_pthread_attr_t
#define pthread_attr_init workaround_pthread_attr_init
#define pthread_getattr_np workaround_pthread_getattr_np
#define pthread_attr_getguardsize workaround_pthread_attr_getguardsize
#define pthread_attr_getstack workaround_pthread_attr_getstack
#define pthread_attr_destroy workaround_pthread_attr_destroy

typedef struct {
    void *_a_stackaddr;
    size_t _a_stacksize, _a_guardsize;
} pthread_attr_t;

extern __attribute__((__visibility__("hidden"))) unsigned __default_guardsize;

int pthread_attr_init(pthread_attr_t *a)
{
	return 0;
}

// Modified version of pthread_getattr_np from the upstream PR.

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr) {
  attr->_a_stackaddr = (void*)emscripten_stack_get_base();
  attr->_a_stacksize = emscripten_stack_get_base() - emscripten_stack_get_end();
  attr->_a_guardsize = __default_guardsize;
  return 0;
}

// These three functions copied without any changes from Emscripten libc.

int pthread_attr_getguardsize(const pthread_attr_t *restrict a, size_t *restrict size)
{
	*size = a->_a_guardsize;
	return 0;
}

int pthread_attr_getstack(const pthread_attr_t *restrict a, void **restrict addr, size_t *restrict size)
{
/// XXX musl is not standard-conforming? It should not report EINVAL if _a_stackaddr is zero, and it should
///     report EINVAL if a is null: http://pubs.opengroup.org/onlinepubs/009695399/functions/pthread_attr_getstack.html
	if (!a) return EINVAL;
//	if (!a->_a_stackaddr)
//		return EINVAL;

	*size = a->_a_stacksize;
	*addr = (void *)(a->_a_stackaddr - *size);
	return 0;
}

int pthread_attr_destroy(pthread_attr_t *a)
{
	return 0;
}

#endif

static void
hardware_stack_limits(uintptr_t *base, uintptr_t *top, uintptr_t sp)
{
#ifdef WIN32
    ULONG_PTR low, high;
    GetCurrentThreadStackLimits(&low, &high);
    *top = (uintptr_t)high;
    ULONG guarantee = 0;
#ifdef MS_WINDOWS_DESKTOP
    SetThreadStackGuarantee(&guarantee);
#endif
    *base = (uintptr_t)low + guarantee;
#elif defined(__APPLE__)
    pthread_t this_thread = pthread_self();
    void *stack_addr = pthread_get_stackaddr_np(this_thread); // top of the stack
    size_t stack_size = pthread_get_stacksize_np(this_thread);
    *top = (uintptr_t)stack_addr;
    *base = ((uintptr_t)stack_addr) - stack_size;
#else
    // musl supports HAVE_PTHREAD_GETATTR_NP, but the resulting stack size
    // (on alpine at least) is much smaller than expected and imposes undue
    // limits compared to the old stack size estimation. musl doesn't give
    // us a way to detect it, so we have to assume any non-glibc,
    // non-Android Linux is musl and mustn't rely on pthread_getattr_np.
#  if (defined(HAVE_PTHREAD_GETATTR_NP) || defined(_Py_HAVE_PTHREAD_ATTR_GET_NP)) \
        && !defined(_AIX) && !defined(__NetBSD__) \
        && (defined(__GLIBC__) || defined(__ANDROID__) || !defined(__linux__))
    size_t stack_size, guard_size;
    void *stack_addr;
    pthread_attr_t attr;

    int err = pthread_attr_init(&attr);
#ifdef _Py_HAVE_PTHREAD_ATTR_GET_NP
    err |= pthread_attr_get_np(pthread_self(), &attr);
#else
    err |= pthread_getattr_np(pthread_self(), &attr);
#endif
    if (err == 0) {
        err = pthread_attr_getguardsize(&attr, &guard_size);
        err |= pthread_attr_getstack(&attr, &stack_addr, &stack_size);
        err |= pthread_attr_destroy(&attr);
    }
    if (err == 0) {
        *base = ((uintptr_t)stack_addr) + guard_size;
        *top = (uintptr_t)stack_addr + stack_size;
        return;
    }
#  endif

    // Add some space for caller function then round to minimum page size
    // This is a guess at the top of the stack, but should be a reasonably
    // good guess if called from _PyThreadState_Attach when creating a thread.
    // If the thread is attached deep in a call stack, then the guess will be poor.
#if _Py_STACK_GROWS_DOWN
    uintptr_t top_addr = _Py_SIZE_ROUND_UP(sp + 8*sizeof(void*), SYSTEM_PAGE_SIZE);
    *top = top_addr;
    *base = top_addr - Py_C_STACK_SIZE;
#  else
    uintptr_t base_addr = _Py_SIZE_ROUND_DOWN(sp - 8*sizeof(void*), SYSTEM_PAGE_SIZE);
    *base = base_addr;
    *top = base_addr + Py_C_STACK_SIZE;
#endif
#endif
}

static void
tstate_set_stack(PyThreadState *tstate,
                 uintptr_t base, uintptr_t top)
{
    assert(base < top);
    assert((top - base) >= _PyOS_MIN_STACK_SIZE);

#ifdef _Py_THREAD_SANITIZER
    // Thread sanitizer crashes if we use more than half the stack.
    uintptr_t stacksize = top - base;
#  if _Py_STACK_GROWS_DOWN
    base += stacksize/2;
#  else
    top -= stacksize/2;
#  endif
#endif
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
#if _Py_STACK_GROWS_DOWN
    _tstate->c_stack_top = top;
    _tstate->c_stack_hard_limit = base + _PyOS_STACK_MARGIN_BYTES;
    _tstate->c_stack_soft_limit = base + _PyOS_STACK_MARGIN_BYTES * 2;
#  ifndef NDEBUG
    // Sanity checks
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    assert(ts->c_stack_hard_limit <= ts->c_stack_soft_limit);
    assert(ts->c_stack_soft_limit < ts->c_stack_top);
#  endif
#else
    _tstate->c_stack_top = base;
    _tstate->c_stack_hard_limit = top - _PyOS_STACK_MARGIN_BYTES;
    _tstate->c_stack_soft_limit = top - _PyOS_STACK_MARGIN_BYTES * 2;
#  ifndef NDEBUG
    // Sanity checks
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    assert(ts->c_stack_hard_limit >= ts->c_stack_soft_limit);
    assert(ts->c_stack_soft_limit > ts->c_stack_top);
#  endif
#endif
}


void
_Py_InitializeRecursionLimits(PyThreadState *tstate)
{
    uintptr_t base, top;
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    hardware_stack_limits(&base, &top, here_addr);
    assert(top != 0);

    tstate_set_stack(tstate, base, top);
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    ts->c_stack_init_base = base;
    ts->c_stack_init_top = top;
}


int
PyUnstable_ThreadState_SetStackProtection(PyThreadState *tstate,
                                void *stack_start_addr, size_t stack_size)
{
    if (stack_size < _PyOS_MIN_STACK_SIZE) {
        PyErr_Format(PyExc_ValueError,
                     "stack_size must be at least %zu bytes",
                     _PyOS_MIN_STACK_SIZE);
        return -1;
    }

    uintptr_t base = (uintptr_t)stack_start_addr;
    uintptr_t top = base + stack_size;
    tstate_set_stack(tstate, base, top);
    return 0;
}


void
PyUnstable_ThreadState_ResetStackProtection(PyThreadState *tstate)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    if (ts->c_stack_init_top != 0) {
        tstate_set_stack(tstate,
                         ts->c_stack_init_base,
                         ts->c_stack_init_top);
        return;
    }

    _Py_InitializeRecursionLimits(tstate);
}


/* The function _Py_EnterRecursiveCallTstate() only calls _Py_CheckRecursiveCall()
   if the stack pointer is beyond c_stack_soft_limit. */
int
_Py_CheckRecursiveCall(PyThreadState *tstate, const char *where)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    assert(_tstate->c_stack_soft_limit != 0);
    assert(_tstate->c_stack_hard_limit != 0);
#if _Py_STACK_GROWS_DOWN
    if (here_addr < _tstate->c_stack_hard_limit) {
        if (here_addr < _tstate->c_stack_hard_limit - _PyOS_STACK_MARGIN_BYTES) {
            // Far out of bounds -- Assume stack switching has occurred
            return 0;
        }
        int kbytes_used = (int)(_tstate->c_stack_top - here_addr)/1024;
#else
    if (here_addr > _tstate->c_stack_hard_limit) {
        if (here_addr > _tstate->c_stack_hard_limit + _PyOS_STACK_MARGIN_BYTES) {
            // Far out of bounds -- Assume stack switching has occurred
            return 0;
        }
        int kbytes_used = (int)(here_addr - _tstate->c_stack_top)/1024;
#endif
        /* Too much stack used to safely raise an exception. Give up. */
        char buffer[80];
        snprintf(buffer, 80, "Unrecoverable stack overflow (used %d kB)%s", kbytes_used, where);
        Py_FatalError(buffer);
    }
    if (tstate->recursion_headroom) {
        return 0;
    }
    else {
#if _Py_STACK_GROWS_DOWN
        int kbytes_used = (int)(_tstate->c_stack_top - here_addr)/1024;
#else
        int kbytes_used = (int)(here_addr - _tstate->c_stack_top)/1024;
#endif
        tstate->recursion_headroom++;
        _PyErr_Format(tstate, PyExc_RecursionError,
                    "Stack overflow (used %d kB)%s",
                    kbytes_used,
                    where);
        tstate->recursion_headroom--;
        return -1;
    }
}


const binaryfunc _PyEval_BinaryOps[] = {
    [NB_ADD] = PyNumber_Add,
    [NB_AND] = PyNumber_And,
    [NB_FLOOR_DIVIDE] = PyNumber_FloorDivide,
    [NB_LSHIFT] = PyNumber_Lshift,
    [NB_MATRIX_MULTIPLY] = PyNumber_MatrixMultiply,
    [NB_MULTIPLY] = PyNumber_Multiply,
    [NB_REMAINDER] = PyNumber_Remainder,
    [NB_OR] = PyNumber_Or,
    [NB_POWER] = _PyNumber_PowerNoMod,
    [NB_RSHIFT] = PyNumber_Rshift,
    [NB_SUBTRACT] = PyNumber_Subtract,
    [NB_TRUE_DIVIDE] = PyNumber_TrueDivide,
    [NB_XOR] = PyNumber_Xor,
    [NB_INPLACE_ADD] = PyNumber_InPlaceAdd,
    [NB_INPLACE_AND] = PyNumber_InPlaceAnd,
    [NB_INPLACE_FLOOR_DIVIDE] = PyNumber_InPlaceFloorDivide,
    [NB_INPLACE_LSHIFT] = PyNumber_InPlaceLshift,
    [NB_INPLACE_MATRIX_MULTIPLY] = PyNumber_InPlaceMatrixMultiply,
    [NB_INPLACE_MULTIPLY] = PyNumber_InPlaceMultiply,
    [NB_INPLACE_REMAINDER] = PyNumber_InPlaceRemainder,
    [NB_INPLACE_OR] = PyNumber_InPlaceOr,
    [NB_INPLACE_POWER] = _PyNumber_InPlacePowerNoMod,
    [NB_INPLACE_RSHIFT] = PyNumber_InPlaceRshift,
    [NB_INPLACE_SUBTRACT] = PyNumber_InPlaceSubtract,
    [NB_INPLACE_TRUE_DIVIDE] = PyNumber_InPlaceTrueDivide,
    [NB_INPLACE_XOR] = PyNumber_InPlaceXor,
    [NB_SUBSCR] = PyObject_GetItem,
};

const conversion_func _PyEval_ConversionFuncs[4] = {
    [FVC_STR] = PyObject_Str,
    [FVC_REPR] = PyObject_Repr,
    [FVC_ASCII] = PyObject_ASCII
};

const _Py_SpecialMethod _Py_SpecialMethods[] = {
    [SPECIAL___ENTER__] = {
        .name = &_Py_ID(__enter__),
        .error = (
            "'%T' object does not support the context manager protocol "
            "(missed __enter__ method)"
        ),
        .error_suggestion = (
            "'%T' object does not support the context manager protocol "
            "(missed __enter__ method) but it supports the asynchronous "
            "context manager protocol. Did you mean to use 'async with'?"
        )
    },
    [SPECIAL___EXIT__] = {
        .name = &_Py_ID(__exit__),
        .error = (
            "'%T' object does not support the context manager protocol "
            "(missed __exit__ method)"
        ),
        .error_suggestion = (
            "'%T' object does not support the context manager protocol "
            "(missed __exit__ method) but it supports the asynchronous "
            "context manager protocol. Did you mean to use 'async with'?"
        )
    },
    [SPECIAL___AENTER__] = {
        .name = &_Py_ID(__aenter__),
        .error = (
            "'%T' object does not support the asynchronous "
            "context manager protocol (missed __aenter__ method)"
        ),
        .error_suggestion = (
            "'%T' object does not support the asynchronous context manager "
            "protocol (missed __aenter__ method) but it supports the context "
            "manager protocol. Did you mean to use 'with'?"
        )
    },
    [SPECIAL___AEXIT__] = {
        .name = &_Py_ID(__aexit__),
        .error = (
            "'%T' object does not support the asynchronous "
            "context manager protocol (missed __aexit__ method)"
        ),
        .error_suggestion = (
            "'%T' object does not support the asynchronous context manager "
            "protocol (missed __aexit__ method) but it supports the context "
            "manager protocol. Did you mean to use 'with'?"
        )
    }
};

const size_t _Py_FunctionAttributeOffsets[] = {
    [MAKE_FUNCTION_CLOSURE] = offsetof(PyFunctionObject, func_closure),
    [MAKE_FUNCTION_ANNOTATIONS] = offsetof(PyFunctionObject, func_annotations),
    [MAKE_FUNCTION_KWDEFAULTS] = offsetof(PyFunctionObject, func_kwdefaults),
    [MAKE_FUNCTION_DEFAULTS] = offsetof(PyFunctionObject, func_defaults),
    [MAKE_FUNCTION_ANNOTATE] = offsetof(PyFunctionObject, func_annotate),
};

// PEP 634: Structural Pattern Matching


// Return a tuple of values corresponding to keys, with error checks for
// duplicate/missing keys.
PyObject *
_PyEval_MatchKeys(PyThreadState *tstate, PyObject *map, PyObject *keys)
{
    assert(PyTuple_CheckExact(keys));
    Py_ssize_t nkeys = PyTuple_GET_SIZE(keys);
    if (!nkeys) {
        // No keys means no items.
        return PyTuple_New(0);
    }
    PyObject *seen = NULL;
    PyObject *dummy = NULL;
    PyObject *values = NULL;
    PyObject *key = NULL;
    // We use the two argument form of map.get(key, default) for two reasons:
    // - Atomically check for a key and get its value without error handling.
    // - Don't cause key creation or resizing in dict subclasses like
    //   collections.defaultdict that define __missing__ (or similar).
    PyObject *method_boxes[4];
    _PyValueTemps method_temps = {method_boxes, 0, 4, NULL};
    _PyCValue self, method;
    _PyThreadState_PushCValue(tstate, &self);
    _PyThreadState_PushCValue(tstate, &method);
    self.ref = PyValue_FromPyObjectBorrow(map);
    int res = _PyObject_GetMethodValue(tstate, &self.ref, &_Py_ID(get), &method.ref, map, &method_temps);
    if (res < 0) {
        goto fail;
    }
    PyObject *get = _PyValueTemps_Object(&method_temps, method.ref);
    if (get == NULL || PyErr_Occurred()) {
        goto fail;
    }
    seen = PySet_New(NULL);
    if (seen == NULL) {
        goto fail;
    }
    // dummy = object()
    dummy = _PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (dummy == NULL) {
        goto fail;
    }
    values = PyTuple_New(nkeys);
    if (values == NULL) {
        goto fail;
    }
    for (Py_ssize_t i = 0; i < nkeys; i++) {
        key = PyTuple_GetItemRef(keys, i);
        if (key == NULL) {
            goto fail;
        }
        if (PySet_Contains(seen, key) || PySet_Add(seen, key)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                _PyErr_Format(tstate, PyExc_ValueError,
                              "mapping pattern checks duplicate key (%R)", key);
            }
            goto fail;
        }
        PyObject *self_obj = PyValue_AsPyObjectBorrow(self.ref);
        PyObject *args[] = { self_obj, key, dummy };
        PyObject *value = NULL;
        if (!PyValue_IsNull(self.ref)) {
            value = PyObject_Vectorcall(get, args, 3, NULL);
        }
        else {
            value = PyObject_Vectorcall(get, &args[1], 2, NULL);
        }
        if (value == NULL) {
            goto fail;
        }
        if (value == dummy) {
            // key not in map!
            Py_DECREF(value);
            Py_DECREF(values);
            // Return None:
            values = Py_NewRef(Py_None);
            goto done;
        }
        PyTuple_SET_ITEM(values, i, value);
        Py_CLEAR(key);
        if (PyErr_Occurred()) {
            goto fail;
        }
    }
    // Success:
done:
    _PyThreadState_PopCValue(tstate, &method);
    _PyThreadState_PopCValue(tstate, &self);
    Py_DECREF(seen);
    Py_DECREF(dummy);
    Py_XDECREF(key);
    _PyValueTemps_Close(&method_temps);
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(values);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return values;
fail:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    _PyThreadState_PopCValue(tstate, &method);
    _PyThreadState_PopCValue(tstate, &self);
    Py_XDECREF(seen);
    Py_XDECREF(dummy);
    Py_XDECREF(values);
    Py_XDECREF(key);
    _PyValue_RestoreException(saved);
    _PyValueTemps_Close(&method_temps);
    return NULL;
}

// Extract a named attribute from the subject, with additional bookkeeping to
// raise TypeErrors for repeated lookups. On failure, return NULL (with no
// error set). Use _PyErr_Occurred(tstate) to disambiguate.
static PyObject *
match_class_attr(PyThreadState *tstate, PyObject *subject, PyObject *type,
                 PyObject *name, PyObject *seen)
{
    assert(PyUnicode_CheckExact(name));
    // Only check for duplicates if seen is not NULL.
    if (seen != NULL) {
        assert(PySet_CheckExact(seen));
        if (PySet_Contains(seen, name) || PySet_Add(seen, name)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%s() got multiple sub-patterns for attribute %R",
                            ((PyTypeObject*)type)->tp_name, name);
            }
            return NULL;
        }
    }
    PyObject *attr;
    (void)PyObject_GetOptionalAttr(subject, name, &attr);
    return attr;
}

// On success (match), return a tuple of extracted attributes. On failure (no
// match), return NULL. Use _PyErr_Occurred(tstate) to disambiguate.
PyObject*
_PyEval_MatchClass(PyThreadState *tstate, PyObject *subject, PyObject *type,
                   Py_ssize_t nargs, PyObject *kwargs)
{
    if (!PyType_Check(type)) {
        const char *e = "class pattern must refer to a class";
        _PyErr_Format(tstate, PyExc_TypeError, e);
        return NULL;
    }
    assert(PyTuple_CheckExact(kwargs));
    // First, an isinstance check:
    if (PyObject_IsInstance(subject, type) <= 0) {
        return NULL;
    }
    // Short circuit if there aren't any arguments:
    Py_ssize_t nkwargs = PyTuple_GET_SIZE(kwargs);
    Py_ssize_t nattrs = nargs + nkwargs;
    if (!nattrs) {
        return PyTuple_New(0);
    }
    // So far so good:
    PyObject *seen = NULL;
    // Only check for duplicates if there is at least one positional attribute
    // and two or more attributes in total. Duplicate keyword attributes are
    // detected during the compile stage and raise a SyntaxError.
    if (nargs > 0 && nattrs > 1) {
        seen = PySet_New(NULL);
        if (seen == NULL) {
            return NULL;
        }
    }
    PyObject *attrs = PyTuple_New(nattrs);
    if (attrs == NULL) {
        Py_XDECREF(seen);
        return NULL;
    }
    // NOTE: From this point on, goto fail on failure:
    PyObject *match_args = NULL;
    // First, the positional subpatterns:
    if (nargs) {
        int match_self = 0;
        if (PyObject_GetOptionalAttr(type, &_Py_ID(__match_args__), &match_args) < 0) {
            goto fail;
        }
        if (match_args) {
            if (!PyTuple_CheckExact(match_args)) {
                const char *e = "%s.__match_args__ must be a tuple (got %s)";
                _PyErr_Format(tstate, PyExc_TypeError, e,
                              ((PyTypeObject *)type)->tp_name,
                              Py_TYPE(match_args)->tp_name);
                goto fail;
            }
        }
        else {
            // _Py_TPFLAGS_MATCH_SELF is only acknowledged if the type does not
            // define __match_args__. This is natural behavior for subclasses:
            // it's as if __match_args__ is some "magic" value that is lost as
            // soon as they redefine it.
            match_args = PyTuple_New(0);
            match_self = PyType_HasFeature((PyTypeObject*)type,
                                            _Py_TPFLAGS_MATCH_SELF);
        }
        assert(PyTuple_CheckExact(match_args));
        Py_ssize_t allowed = match_self ? 1 : PyTuple_GET_SIZE(match_args);
        if (allowed < nargs) {
            const char *plural = (allowed == 1) ? "" : "s";
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%s() accepts %zd positional sub-pattern%s (%zd given)",
                          ((PyTypeObject*)type)->tp_name,
                          allowed, plural, nargs);
            goto fail;
        }
        if (match_self) {
            // Easy. Copy the subject itself, and move on to kwargs.
            assert(PyValue_IsNull(PyTuple_GET_VALUE(attrs, 0)));
            PyTuple_SET_ITEM(attrs, 0, Py_NewRef(subject));
        }
        else {
            for (Py_ssize_t i = 0; i < nargs; i++) {
                PyValue name_value = PyTuple_GET_VALUE(match_args, i);
                if (!PyValue_IsObject(name_value) ||
                    !PyUnicode_CheckExact(PyValue_AsPyObjectBorrow(name_value))) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "__match_args__ elements must be strings "
                                  "(got %s)", PyValue_TYPE(name_value)->tp_name);
                    goto fail;
                }
                PyObject *name = PyValue_AsPyObjectBorrow(name_value);
                PyObject *attr = match_class_attr(tstate, subject, type, name,
                                                  seen);
                if (attr == NULL) {
                    goto fail;
                }
                assert(PyValue_IsNull(PyTuple_GET_VALUE(attrs, i)));
                PyTuple_SET_ITEM(attrs, i, attr);
            }
        }
        Py_CLEAR(match_args);
    }
    // Finally, the keyword subpatterns:
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *name = _PyTuple_GET_OBJECT(kwargs, i);
        PyObject *attr = match_class_attr(tstate, subject, type, name, seen);
        if (attr == NULL) {
            goto fail;
        }
        assert(PyValue_IsNull(PyTuple_GET_VALUE(attrs, nargs + i)));
        PyTuple_SET_ITEM(attrs, nargs + i, attr);
    }
    Py_XDECREF(seen);
    return attrs;
fail:
    // We really don't care whether an error was raised or not... that's our
    // caller's problem. All we know is that the match failed.
    Py_XDECREF(match_args);
    Py_XDECREF(seen);
    Py_DECREF(attrs);
    return NULL;
}


static int do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause);

PyObject *
PyEval_EvalCode(PyObject *co, PyObject *globals, PyObject *locals)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (locals == NULL) {
        locals = globals;
    }
    PyObject *builtins = _PyDict_LoadBuiltinsFromGlobals(globals);
    if (builtins == NULL) {
        return NULL;
    }
    PyFrameConstructor desc = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = ((PyCodeObject *)co)->co_name,
        .fc_qualname = ((PyCodeObject *)co)->co_name,
        .fc_code = co,
        .fc_defaults = NULL,
        .fc_kwdefaults = NULL,
        .fc_closure = NULL
    };
    PyFunctionObject *func = _PyFunction_FromConstructor(&desc);
    _Py_DECREF_BUILTINS(builtins);
    if (func == NULL) {
        return NULL;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_LEGACY);
    PyObject *res = _PyEval_Vector(tstate, func, locals, NULL, 0, NULL);
    Py_DECREF(func);
    return res;
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f)
{
    /* Function kept for backward compatibility */
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f->f_frame, 0);
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_EvalFrame(tstate, f->f_frame, throwflag);
}

static inline PyObject *
check_value_bridge_result(PyObject *result)
{
    if (result != NULL && PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static void
clear_value_arguments(PyValue *arguments, int count)
{
    PyObject *saved = PyErr_GetRaisedException();
    for (int i = count - 1; i >= 0; i--) {
        PyValue value = arguments[i];
        arguments[i] = PyValue_NULL;
        PyValue_CLOSE(value);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    _PyValue_RestoreException(saved);
}

typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    PyCodeObject *code;
    Py_ssize_t current_word;
    Py_ssize_t next_word;
    int input_slots[2];
    int result_slot;
} _PyValueOperation;

typedef enum {
    CALL_TRANSACTION_VECTOR,
    CALL_TRANSACTION_KEYWORDS,
    CALL_TRANSACTION_EX,
} _PyCallTransactionKind;

/* Numeric homes survive instrumentation changes. This record owns no
 * preparation resource: each preparation helper drains its locals before
 * returning, including before an inlined Python frame can escape. */
typedef struct {
    _PyValueOperation operation;
    int base;
    int count;
    int destination;
    int argument_count;
    _PyCallTransactionKind kind;
    int instrumented;
    int effects;
    int monitored;
    int periodic;
    int prepared;
} _PyCallTransaction;

static inline _PyValueOperation
value_operation_begin(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *next)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyCodeExec *exec = code->_co_exec;
    assert(exec != NULL && frame->ir_words == exec->words);
    assert(frame->instr_ptr >= exec->words &&
           frame->instr_ptr < exec->words + exec->nwords);
    assert(next >= exec->words && next <= exec->words + exec->nwords);
    return (_PyValueOperation){tstate, frame, code,
        frame->instr_ptr - exec->words, next - exec->words, {-1, -1}, -1};
}

static int
value_operation_restore2(_PyValueOperation *operation,
                         PyValue *first, PyValue *second)
{
    _PyInterpreterFrame *frame = operation->frame;
    PyObject *saved = PyErr_GetRaisedException();
    if (operation->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != operation->code) {
        PyErr_SetString(PyExc_SystemError, "dictionary store changed the active frame");
        _PyValue_RestoreException(saved);
        return -1;
    }
    int nlocals = operation->code->co_nlocalsplus;
    int nslots = _PyFrame_NumSlots(frame);
    PyValue *slots = _PyFrame_Slots(frame);
    for (int i = 0; i < 2; i++) {
        int slot = operation->input_slots[i];
        if (slot < 0 || slot >= nslots ||
            (slot >= nlocals && !PyValue_IsNull(slots[slot]))) {
            PyErr_SetString(PyExc_SystemError, "dictionary store changed an input home");
            _PyValue_RestoreException(saved);
            return -1;
        }
    }
    PyValue *owners[2] = {first, second};
    for (int i = 0; i < 2; i++) {
        int slot = operation->input_slots[i];
        if (slot >= nlocals) {
            slots[slot] = *owners[i];
            *owners[i] = PyValue_NULL;
        }
    }
    /* Callback-current locals remain intact. Their extra evaluated
     * snapshots are still C-owned; only temporary owners moved back. */
    _PyValue_RestoreException(saved);
    return 0;
}

/* Fixed store operands have already moved out of their frame homes. Keep
 * their original release order, with no decoded slot access between closes. */
static void
value_operation_close3(PyValue *first, PyValue *second, PyValue *third)
{
    PyValue owners[3] = {*first, second == NULL ? PyValue_NULL : *second,
                        third == NULL ? PyValue_NULL : *third};
    *first = PyValue_NULL;
    if (second != NULL) {
        *second = PyValue_NULL;
    }
    if (third != NULL) {
        *third = PyValue_NULL;
    }
    PyObject *saved = PyErr_GetRaisedException();
    for (int i = 0; i < 3; i++) {
        PyValue_XCLOSE(owners[i]);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    _PyValue_RestoreException(saved);
}

static int
value_store_subscript(PyValue container, PyValue key, PyValue value,
                      _PyValueTemps *temps)
{
    if (PyValue_IsObject(container)) {
        PyObject *obj = PyValue_AsPyObjectBorrow(container);
        PyMappingMethods *mapping = Py_TYPE(obj)->tp_as_mapping;
        if (mapping != NULL && mapping->mp_ass_subscript == _PyDict_StoreSubscript) {
            return PyDict_SetValue(obj, key, value);
        }
    }
    PyObject *obj = _PyValueTemps_Object(temps, container);
    if (obj == NULL || PyErr_Occurred()) {
        return -1;
    }
    PyObject *key_obj = _PyValueTemps_Object(temps, key);
    if (key_obj == NULL || PyErr_Occurred()) {
        return -1;
    }
    PyObject *value_obj = _PyValueTemps_Object(temps, value);
    if (value_obj == NULL || PyErr_Occurred()) {
        return -1;
    }
    return PyObject_SetItem(obj, key_obj, value_obj);
}

static int
value_store_attribute(PyValue owner, PyValue value, PyObject *name,
                      _PyValueTemps *temps, int *invoked)
{
    *invoked = 0;
    PyObject *object = _PyValueTemps_Object(temps, owner);
    if (object == NULL || PyErr_Occurred()) {
        return -1;
    }
    PyObject *object_value = PyValue_IsNull(value) ? NULL :
        _PyValueTemps_Object(temps, value);
    if ((!PyValue_IsNull(value) && object_value == NULL) || PyErr_Occurred()) {
        return -1;
    }
    *invoked = 1;
    return PyObject_SetAttr(object, name, object_value);
}

static int
value_operation_restore1(_PyValueOperation *operation, PyValue *owner)
{
    _PyInterpreterFrame *frame = operation->frame;
    PyObject *saved = PyErr_GetRaisedException();
    int slot = operation->input_slots[0];
    if (operation->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != operation->code || slot < 0 ||
        slot >= _PyFrame_NumSlots(frame) ||
        (slot >= operation->code->co_nlocalsplus &&
         !PyValue_IsNull(_PyFrame_Slots(frame)[slot]))) {
        PyErr_SetString(PyExc_SystemError, "attribute operation changed its input home");
        _PyValue_RestoreException(saved);
        return -1;
    }
    if (slot >= operation->code->co_nlocalsplus) {
        _PyFrame_Slots(frame)[slot] = *owner;
        *owner = PyValue_NULL;
    }
    _PyValue_RestoreException(saved);
    return 0;
}

static int
value_load_attribute(PyThreadState *tstate, PyValue owner, PyObject *name,
                     int method, _PyValueTemps *temps,
                     PyValue *result, PyValue *self, int *invoked)
{
    *result = *self = PyValue_NULL;
    *invoked = 0;
    PyObject *object = _PyValueTemps_Object(temps, owner);
    if (object == NULL || PyErr_Occurred()) {
        return -1;
    }
    *invoked = 1;
    if (method) {
        *self = PyValue_MakeHeapSafe(PyValue_Borrow(owner));
        return _PyObject_GetMethodValue(tstate, self, name, result, object, temps);
    }
    *result = _PyObject_GetAttrValue(object, name);
    return PyValue_IsNull(*result) || PyErr_Occurred() ? -1 : 0;
}

static PyValue
value_binary_operation(int operation, PyValue left, PyValue right,
                       _PyValueTemps *temps)
{
    /* Inputs are independent owning snapshots before either conversion can
     * allocate. The dictionary's native entry needs no scalar object view. */
    if (operation == NB_SUBSCR && PyValue_IsObject(left)) {
        PyObject *container = PyValue_AsPyObjectBorrow(left);
        if (PyAnyDict_CheckExact(container)) {
            PyValue result = PyValue_NULL;
            int found = PyDict_GetValue(container, right, &result);
            if (found != 0) {
                return result;
            }
            /* Exact dictionaries have no __missing__. Do not repeat a hash
             * or equality callback just to construct the missing-key error. */
            PyObject *key = _PyValueTemps_Object(temps, right);
            if (key == NULL || PyErr_Occurred()) {
                return PyValue_NULL;
            }
            _PyErr_SetKeyError(key);
            return PyValue_NULL;
        }
    }
    PyObject *left_obj = _PyValueTemps_Object(temps, left);
    if (left_obj == NULL || PyErr_Occurred()) {
        return PyValue_NULL;
    }
    PyObject *right_obj = _PyValueTemps_Object(temps, right);
    if (right_obj == NULL || PyErr_Occurred()) {
        return PyValue_NULL;
    }
    assert(_PyEval_BinaryOps[operation] != NULL);
    PyObject *result = _PyEval_BinaryOps[operation](left_obj, right_obj);
    return result == NULL ? PyValue_NULL : PyValue_FromPyObjectSteal(result);
}

#include "ceval_macros.h"


/* Helper functions to keep the size of the largest uops down */

PyObject *
_Py_VectorCall_ValueSteal(
    PyValue callable,
    PyValue *arguments,
    int total_args,
    PyValue kwnames)
{
    PyObject *res;
    callable = PyValue_MakeHeapSafe(callable);
    kwnames = PyValue_MakeHeapSafe(kwnames);
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        res = NULL;
        goto cleanup;
    }
    PyObject *callable_o = PyValue_AsPyObjectBorrow(callable);
    PyObject *kwnames_o = PyValue_AsPyObjectBorrow(kwnames);
    int positional_args = total_args;
    if (kwnames_o != NULL) {
        positional_args -= (int)PyTuple_GET_SIZE(kwnames_o);
    }
    res = PyObject_Vectorcall(
        callable_o, args_o,
        positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames_o);
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
cleanup:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(kwnames);
    _PyValue_RestoreException(saved);
    clear_value_arguments(arguments, total_args);
    saved = PyErr_GetRaisedException();
    PyValue_CLOSE(callable);
    _PyValue_RestoreException(saved);
    return check_value_bridge_result(res);
}

PyObject*
_Py_VectorCallInstrumentation_ValueSteal(
    PyValue callable,
    PyValue* arguments,
    int total_args,
    PyValue kwnames,
    bool call_instrumentation,
    _PyInterpreterFrame* frame,
    _PyInstr* this_instr,
    PyThreadState* tstate)
{
    PyObject* res;
    callable = PyValue_MakeHeapSafe(callable);
    kwnames = PyValue_MakeHeapSafe(kwnames);
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        res = NULL;
        goto cleanup;
    }
    PyObject* callable_o = PyValue_AsPyObjectBorrow(callable);
    PyObject* kwnames_o = PyValue_AsPyObjectBorrow(kwnames);
    int positional_args = total_args;
    if (kwnames_o != NULL) {
        positional_args -= (int)PyTuple_GET_SIZE(kwnames_o);
    }
    res = PyObject_Vectorcall(
        callable_o, args_o,
        positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames_o);
    if (call_instrumentation) {
        PyObject* arg = total_args == 0 ?
            &_PyInstrumentation_MISSING : args_o[0];
        if (res == NULL) {
            _Py_call_instrumentation_exc2(
                tstate, PY_MONITORING_EVENT_C_RAISE,
                frame, this_instr, callable_o, arg);
        }
        else {
            int err = _Py_call_instrumentation_2args(
                tstate, PY_MONITORING_EVENT_C_RETURN,
                frame, this_instr, callable_o, arg);
            if (err < 0) {
                Py_CLEAR(res);
            }
        }
    }
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
cleanup:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(kwnames);
    _PyValue_RestoreException(saved);
    clear_value_arguments(arguments, total_args);
    saved = PyErr_GetRaisedException();
    PyValue_CLOSE(callable);
    _PyValue_RestoreException(saved);
    return check_value_bridge_result(res);
}

PyObject *
_Py_BuiltinCallFast_Value(
    PyValue callable,
    PyValue *arguments,
    int total_args)
{
    PyObject *res;
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        return NULL;
    }
    PyObject *callable_o = PyValue_AsPyObjectBorrow(callable);
    PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable_o);
    res = _PyCFunctionFast_CAST(cfunc)(
        PyCFunction_GET_SELF(callable_o),
        args_o,
        total_args
    );
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    return res;
}

PyObject *
_Py_BuiltinCallFastWithKeywords_Value(
    PyValue callable,
    PyValue *arguments,
    int total_args)
{
    PyObject *res;
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        return NULL;
    }
    PyObject *callable_o = PyValue_AsPyObjectBorrow(callable);
    PyCFunctionFastWithKeywords cfunc =
        _PyCFunctionFastWithKeywords_CAST(PyCFunction_GET_FUNCTION(callable_o));
    res = cfunc(PyCFunction_GET_SELF(callable_o), args_o, total_args, NULL);
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    return res;
}

PyObject *
_PyCallMethodDescriptorFast_Value(
    PyValue callable,
    PyCFunctionFast cfunc,
    PyObject *self,
    PyValue *arguments,
    int total_args)
{
    PyObject *res;
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        return NULL;
    }
    assert(PyValue_Is(PyValue_FromObjectBorrow(self), arguments[0]));

    res = cfunc(self, (args_o + 1), total_args - 1);
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    return res;
}

PyObject *
_PyCallMethodDescriptorFastWithKeywords_Value(
    PyValue callable,
    PyCFunctionFastWithKeywords cfunc,
    PyObject *self,
    PyValue *arguments,
    int total_args)
{
    PyObject *res;
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        return NULL;
    }
    assert(PyValue_Is(PyValue_FromObjectBorrow(self), arguments[0]));

    res = cfunc(self, (args_o + 1), total_args-1, NULL);
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    return res;
}

PyObject *
_Py_CallBuiltinClass_Value(
    PyValue callable,
    PyValue *arguments,
    int total_args)
{
    PyObject *res;
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        return NULL;
    }
    PyTypeObject *tp = (PyTypeObject *)PyValue_AsPyObjectBorrow(callable);
    res = tp->tp_vectorcall((PyObject *)tp, args_o, total_args | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    return res;
}

PyObject *
_Py_BuildString_ValueSteal(
    PyValue *arguments,
    int total_args)
{
    PyObject *res;
    VALUES_TO_PYOBJECTS(arguments, total_args, args_o);
    if (CONVERSION_FAILED(args_o)) {
        res = NULL;
        goto cleanup;
    }
    res = _PyUnicode_JoinArray(&_Py_STR(empty), args_o, total_args);
    VALUES_TO_PYOBJECTS_CLEANUP(args_o, res);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
cleanup:
    clear_value_arguments(arguments, total_args);
    return check_value_bridge_result(res);
}

PyObject *
_Py_BuildMap_ValueSteal(
    PyValue *arguments,
    int half_args)
{
    /* RegCFG verifies that this evaluated window contains consecutive
     * temporaries, not locals exposed to callbacks through f_locals. */
    _PyValue_OwnArray(arguments, half_args * 2);
    PyObject *res = _PyDict_FromValueItems(arguments, 2, arguments + 1, 2,
                                          half_args);
    clear_value_arguments(arguments, half_args * 2);
    return check_value_bridge_result(res);
}



int _Py_CheckRecursiveCallPy(
    PyThreadState *tstate)
{
    if (tstate->recursion_headroom) {
        if (tstate->py_recursion_remaining < -50) {
            /* Overflowing while handling an overflow. Give up. */
            Py_FatalError("Cannot recover from Python stack overflow.");
        }
    }
    else {
        if (tstate->py_recursion_remaining <= 0) {
            tstate->recursion_headroom++;
            _PyErr_Format(tstate, PyExc_RecursionError,
                        "maximum recursion depth exceeded");
            tstate->recursion_headroom--;
            return -1;
        }
    }
    return 0;
}

/* The value that a frame returns or yields to the entry frame lands in
 * entry.stack[0], slot 1 of the entry frame. */
static_assert(offsetof(_PyEntryFrame, stack) ==
              offsetof(_PyInterpreterFrame, owned_slots) + sizeof(PyValue),
              "the entry frame's stack must follow owned_slots[0]");

/* The entry frame's words.  Its slot 1 is a temporary, which
 * INTERPRETER_EXIT takes; the header precedes the words, as in every
 * executing copy, for IR_IS_LOCAL(). */
static const struct {
    _PyCodeExec exec;
    _PyIRWord words[3];
} entry_trampoline = {
    .exec = { .nwords = 3, .nlocalsplus = 1 },
    .words = {
        /* Put a NOP at the start, so that the IP points into
        * the code, rather than before it */
        IR_WORD(NOP, 0, 0, 0, 0),
        IR_WORD(INTERPRETER_EXIT, 1, 0, 0, 0),  /* reached on return and yield */
        IR_WORD(INTERPRETER_EXIT, 1, 0, 0, 0),
    },
};
#define _Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS (entry_trampoline.words)

const _PyInstr *_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS_PTR = (_PyInstr*)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;


PyObject **
_PyObjectArray_FromValueArray(PyValue *input, Py_ssize_t nargs, PyObject **scratch)
{
    if (nargs < 0 || (size_t)nargs >= SIZE_MAX / sizeof(PyObject *) - 1) {
        PyErr_NoMemory();
        return NULL;
    }
    /* Window slots are private evaluated operands. Give every borrowed
     * pointer an independent owner before the first allocation can reenter
     * and replace the local/container from which it was evaluated. */
    for (Py_ssize_t i = 0; i < nargs; i++) {
        if (!PyValue_IsHeapSafe(input[i])) {
            input[i] = PyValue_MakeHeapSafe(input[i]);
        }
    }
    PyObject **result;
    if (nargs > MAX_VALUE_SCRATCH) {
        // +1 in case PY_VECTORCALL_ARGUMENTS_OFFSET is set.
        result = PyMem_Malloc((nargs + 1) * sizeof(PyObject *));
        if (result == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        if (PyErr_Occurred()) {
            PyObject *saved = PyErr_GetRaisedException();
            PyMem_Free(result);
            _PyValue_RestoreException(saved);
            return NULL;
        }
    }
    else {
        result = scratch;
    }
    result++;
    result[0] = NULL; /* Keep GCC happy */
    for (Py_ssize_t i = 0; i < nargs; i++) {
        result[i] = PyValue_AsObject(input[i]);
        if (result[i] == NULL || PyErr_Occurred()) {
            _PyObjectArray_Free(result - 1, scratch, i + (result[i] != NULL));
            return NULL;
        }
    }
    return result;
}

void
_PyObjectArray_Free(PyObject **array, PyObject **scratch, Py_ssize_t nargs)
{
    _PyValueTemps owners = {array + 1, nargs, nargs, NULL};
    _PyValueTemps_Close(&owners);
    PyObject *saved = PyErr_GetRaisedException();
    if (array != scratch) {
        PyMem_Free(array);
    }
    _PyValue_RestoreException(saved);
}

/* _PyEval_EvalFrameDefault is too large to optimize for speed with PGO on MSVC.
 */
#if (defined(_MSC_VER) && \
     (_MSC_VER < 1943) && \
     defined(_Py_USING_PGO))
#define DO_NOT_OPTIMIZE_INTERP_LOOP
#endif

#ifdef DO_NOT_OPTIMIZE_INTERP_LOOP
#  pragma optimize("t", off)
/* This setting is reversed below following _PyEval_EvalFrameDefault */
#endif

#if _Py_TAIL_CALL_INTERP
#include "ir_targets.h"
#include "generated_ir_cases.c.h"
#endif


PyValue
_PyEval_GetIter(PyValue iterable, PyValue *index_or_null, int yield_from)
{
    PyTypeObject *tp = PyValue_TYPE(iterable);
    if (tp->_tp_iteritem != NULL) {
        /* Leave iterable on stack and push tagged index 0 */
        *index_or_null = PyValue_TagIndex(0);
        return iterable;
    }
    *index_or_null = PyValue_NULL;
    if (tp->tp_iter == PyObject_SelfIter) {
        return iterable;
    }
    if (yield_from && tp == &PyCoro_Type) {
        if (yield_from == GET_ITER_YIELD_FROM) {
            // The compiler cannot know whether types.coroutine() will be
            // called, which sets the flag on a copy of the code object
            int flags = _PyFrame_GetCode(_PyThreadState_GET()->current_frame)->co_flags;
            yield_from = (flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))
                ? GET_ITER_YIELD_FROM_NO_CHECK : GET_ITER_YIELD_FROM_CORO_CHECK;
        }
        if (yield_from == GET_ITER_YIELD_FROM_CORO_CHECK) {
            /* `iterable` is a coroutine and it is used in a 'yield from'
            * expression of a regular generator. */
            PyErr_SetString(PyExc_TypeError,
                            "cannot 'yield from' a coroutine object "
                            "in a non-coroutine generator");
            PyValue_CLOSE(iterable);
            return PyValue_ERROR;
        }
        return iterable;
    }
    /* Pop iterable, and push iterator then NULL. An int or float is not
       iterable; PyObject_GetIter() raises the error. */
    iterable = PyValue_Box(iterable);
    if (PyValue_IsInline(iterable)) {
        return PyValue_ERROR;
    }
    PyObject *iter_o = PyObject_GetIter(PyValue_AsPyObjectBorrow(iterable));
    PyValue_CLOSE(iterable);
    if (iter_o == NULL) {
        return PyValue_ERROR;
    }
    return PyValue_FromPyObjectSteal(iter_o);
}

int _PyEval_StoreName(PyThreadState *tstate, PyValue v, PyObject *name, PyObject* ns)
{
    int deletion = PyValue_IsNull(v);

    if (ns == NULL) {
        const char *msg = deletion
            ? "no locals found when deleting %R"
            : "no locals found when storing %R";
        _PyErr_Format(tstate, PyExc_SystemError, msg, name);
        return 1;
    }

    if (deletion) {
        int error = PyObject_DelItem(ns, name);
        if (error) {
            _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                    NAME_ERROR_MSG,
                                    name);
        }
        return error;
    }

    PyObject *v_o = PyValue_AsPyObjectNewBoxed(v);
    if (v_o == NULL) {
        return -1;
    }
    int err;
    if (PyDict_CheckExact(ns)) {
        err = PyDict_SetItem(ns, name, v_o);
    }
    else {
        err = PyObject_SetItem(ns, name, v_o);
    }
    Py_DECREF(v_o);
    return err;
}

#if (defined(__GNUC__) && __GNUC__ >= 10 && !defined(__clang__)) && defined(__x86_64__)
/*
 * gh-129987: The SLP autovectorizer can cause poor code generation for
 * opcode dispatch in some GCC versions (observed in GCCs 12 through 15,
 * probably caused by https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115777),
 * negating any benefit we get from vectorization elsewhere in the
 * interpreter loop. Disabling it significantly affected older GCC versions
 * (prior to GCC 9, 40% performance drop), so we have to selectively disable
 * it.
 */
#define DONT_SLP_VECTORIZE __attribute__((optimize ("no-tree-slp-vectorize", "no-omit-frame-pointer")))
#else
#define DONT_SLP_VECTORIZE
#endif

#ifdef WITH_DTRACE
static void
dtrace_function_entry(_PyInterpreterFrame *frame)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = _PyFrame_GetCode(frame);
    PyObject *exc = PyErr_GetRaisedException();
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    PyErr_SetRaisedException(exc);
    if (filename == NULL) {
        filename = "<unknown>";
    }
    if (funcname == NULL) {
        funcname = "<unknown>";
    }
    lineno = PyUnstable_InterpreterFrame_GetLine(frame);

    PyDTrace_FUNCTION_ENTRY(filename, funcname, lineno);
}

static void
dtrace_function_return(_PyInterpreterFrame *frame)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = _PyFrame_GetCode(frame);
    PyObject *exc = PyErr_GetRaisedException();
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    PyErr_SetRaisedException(exc);
    if (filename == NULL) {
        filename = "<unknown>";
    }
    if (funcname == NULL) {
        funcname = "<unknown>";
    }
    lineno = PyUnstable_InterpreterFrame_GetLine(frame);

    PyDTrace_FUNCTION_RETURN(filename, funcname, lineno);
}
#endif

#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
static int
jit_helpers_have_monitoring(PyCodeObject *code)
{
    _PyCoMonitoringData *data = code->_co_monitoring;
    if (data == NULL) {
        return 0;
    }
    /* A wholly empty header may only record synchronization with a global
     * epoch. Retained observer payload still requires interpreter execution. */
    if (data->tools || data->lines || data->line_tools ||
        data->per_instruction_opcodes || data->per_instruction_tools) {
        return 1;
    }
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        if (data->local_monitors.tools[i] || data->active_monitors.tools[i]) {
            return 1;
        }
    }
    return 0;
}

#endif

int
_PyJit_ObserversAllow(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
    if (IS_PEP523_HOOKED(tstate) || tstate->tracing || tstate->c_tracefunc ||
        tstate->c_profilefunc || jit_helpers_have_monitoring(_PyFrame_GetCode(frame))) {
        return 0;
    }
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        if (tstate->interp->monitors.tools[i]) {
            return 0;
        }
    }
#ifdef Py_DEBUG
    if (frame->lltrace) {
        return 0;
    }
#endif
    return !(PyDTrace_FUNCTION_ENTRY_ENABLED() || PyDTrace_FUNCTION_RETURN_ENABLED() ||
             PyDTrace_LINE_ENABLED());
#else
    return 0;
#endif
}

/* Only object bridges enter this cold path. Inline operations and handlers
 * with no private boxes need neither a call nor an observer/epoch lookup. */
static void
drop_failed_value(PyValue *result)
{
    if (result != NULL && !PyValue_IsNull(*result)) {
        PyObject *primary = PyErr_GetRaisedException();
        PyValue owner = *result;
        *result = PyValue_NULL;
        PyValue_XCLOSE(owner);
        _PyValue_RestoreException(primary);
    }
}

static int
value_operation_refresh(_PyValueOperation *operation, _PyInstr **next,
                        PyValue *failed_result)
{
    PyThreadState *tstate = operation->tstate;
    _PyInterpreterFrame *frame = operation->frame;
    PyCodeObject *code = operation->code;
    Py_ssize_t word = operation->next_word;
    Py_ssize_t current = operation->current_word;
refresh_after_cleanup:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    /* Free hooks can change instrumentation or replace the executing copy.
     * Do not decode the next handler until these callbacks have finished. */
    if (tstate->current_frame != frame || _PyFrame_GetCode(frame) != code) {
        PyErr_SetString(PyExc_SystemError, "value bridge changed the active frame");
        _PyValue_RestoreException(saved);
        drop_failed_value(failed_result);
        return -1;
    }
    uintptr_t epoch = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) &
                      ~_PY_EVAL_EVENTS_MASK;
    if (code->_co_instrumentation_version != epoch) {
        (void)_Py_Instrument(code, tstate->interp);
    }
    _PyCodeExec *exec = code->_co_exec;
    if (tstate->current_frame != frame || _PyFrame_GetCode(frame) != code ||
        exec == NULL || word < 0 || word > exec->nwords ||
        current < 0 || current >= exec->nwords) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "value bridge changed executing words");
        }
        _PyValue_RestoreException(saved);
        drop_failed_value(failed_result);
        return -1;
    }
    frame->ir_words = exec->words;
    frame->ir_feedback = exec->entries;
    frame->instr_ptr = exec->words + current;
    *next = exec->words + word;
    _PyValue_RestoreException(saved);
    if (PyErr_Occurred() && failed_result != NULL &&
        !PyValue_IsNull(*failed_result)) {
        /* Instrumentation itself can fail after receiver cleanup. Retire
         * the C-only result, then refresh after its possible callbacks too. */
        drop_failed_value(failed_result);
        goto refresh_after_cleanup;
    }
    return PyErr_Occurred() ? -1 : 0;
}

static int
finish_value_bridges_ex(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr **next, _PyValueTemps *temps,
                       PyValue *failed_result)
{
    if (temps == NULL || temps->count == 0) {
        return 0;
    }
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return _PyValueTemps_Close(temps);
    }
    _PyValueOperation operation = value_operation_begin(tstate, frame, *next);
    _PyValueTemps_Close(temps);
    if (PyErr_Occurred() && failed_result != NULL) {
        drop_failed_value(failed_result);
    }
    return value_operation_refresh(&operation, next, failed_result);
}

static int
value_operation_finish(_PyValueOperation *operation, _PyInstr **next,
                       _PyValueTemps *temps)
{
    /* The operation may have reentered before creating any private boxes.
     * Refresh from pre-call scalar PCs even when the owner record is empty. */
    _PyValueTemps_Close(temps);
    return value_operation_refresh(operation, next, NULL);
}

static _PyCallTransaction
call_transaction_begin(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *next, int base, int count, int destination,
                       int argument_count, _PyCallTransactionKind kind,
                       int instrumented, int periodic)
{
    _PyValueOperation operation = value_operation_begin(tstate, frame, next);
    assert(base >= operation.code->co_nlocalsplus);
    assert(count >= 0 && base <= _PyFrame_NumSlots(frame) - count);
    assert(destination >= 0 && destination < _PyFrame_NumSlots(frame));
    /* Every call input is an evaluated temporary, including snapshots of
     * locals. Callback-current LOCAL homes are never consumed by this record. */
    _PyValue_OwnArray(_PyFrame_Slots(frame) + base, count);
    return (_PyCallTransaction){
        .operation = operation, .base = base, .count = count,
        .destination = destination, .argument_count = argument_count,
        .kind = kind, .instrumented = instrumented, .periodic = periodic,
    };
}

static int
call_transaction_refresh(_PyCallTransaction *call, _PyInstr **next)
{
    _PyValueOperation *op = &call->operation;
    int status;
    do {
        status = value_operation_refresh(op, next, NULL);
        if (op->tstate->current_frame != op->frame ||
            _PyFrame_GetCode(op->frame) != op->code) {
            PyObject *saved = PyErr_GetRaisedException();
            PyErr_SetString(PyExc_SystemError,
                            "call preparation changed the active frame");
            _PyValue_RestoreException(saved);
            return -1;
        }
        /* Restoring a pending error can retire another exception. Its
         * callbacks also precede the final decode, so validate after restore. */
    } while (op->code->_co_exec != NULL &&
             (op->frame->ir_words != op->code->_co_exec->words ||
              (status == 0 && op->code->_co_instrumentation_version !=
              (_Py_atomic_load_uintptr_relaxed(&op->tstate->eval_breaker) &
               ~_PY_EVAL_EVENTS_MASK))));
    return status;
}

static void
call_transaction_close_value(PyValue *slot)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyValue value = *slot;
    *slot = PyValue_NULL;
    PyValue_XCLOSE(value);
    _PyValue_RestoreException(saved);
}

static void
call_transaction_close_object(PyObject **slot)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyObject *value = *slot;
    *slot = NULL;
    Py_XDECREF(value);
    _PyValue_RestoreException(saved);
}

static int
call_transaction_expand_method(_PyCallTransaction *call, _PyInstr **next)
{
    call->prepared = 1;
    PyValue *homes = _PyFrame_Slots(call->operation.frame) + call->base;
    if (PyValue_TYPE(homes[0]) != &PyMethod_Type ||
        !PyValue_IsNull(homes[1])) {
        return 0;
    }
    PyMethodObject *method = (PyMethodObject *)PyValue_AsPyObjectBorrow(homes[0]);
    PyValue self = PyValue_FromPyObjectNew(method->im_self);
    PyValue function = PyValue_FromPyObjectNew(method->im_func);
    PyValue obsolete = homes[0];
    /* Publish both final input homes before the obsolete wrapper can run
     * a destructor. No pointer into the old window crosses that release. */
    homes[0] = function;
    homes[1] = self;
    call->effects = 1;
    call_transaction_close_value(&obsolete);
    return call_transaction_refresh(call, next);
}

static int
call_transaction_prepare_tuple(_PyCallTransaction *call, _PyInstr **next)
{
    assert(call->kind == CALL_TRANSACTION_EX);
    call->prepared = 1;
    PyValue *homes = _PyFrame_Slots(call->operation.frame) + call->base;
    PyValue args = PyValue_MakeHeapSafe(PyValue_Borrow(homes[2]));
    PyValue func = PyValue_MakeHeapSafe(PyValue_Borrow(homes[0]));
    PyValue obsolete = PyValue_NULL;
    PyObject *args_object = NULL, *func_object = NULL, *tuple = NULL;
    call->effects = 1;
    args_object = PyValue_AsObject(args);
    if (args_object == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (PyTuple_CheckExact(args_object)) {
        goto done;
    }
    func_object = PyValue_AsObject(func);
    if (func_object == NULL || PyErr_Occurred()) {
        goto done;
    }
    if (_Py_Check_ArgsIterable(call->operation.tstate, func_object,
                              args_object) < 0) {
        goto done;
    }
    tuple = PySequence_Tuple(args_object);
    if (tuple == NULL || call_transaction_refresh(call, next) < 0) {
        goto done;
    }
    homes = _PyFrame_Slots(call->operation.frame) + call->base;
    obsolete = homes[2];
    /* A tuple is a heap object, so this pure owner transfer cannot retire a
     * scalar box. Only the final tuple home is visible during local cleanup. */
    homes[2] = PyValue_FromPyObjectSteal(tuple);
    tuple = NULL;
done:
    call_transaction_close_object(&tuple);
    call_transaction_close_object(&func_object);
    call_transaction_close_object(&args_object);
    call_transaction_close_value(&func);
    call_transaction_close_value(&args);
    call_transaction_close_value(&obsolete);
    return call_transaction_refresh(call, next);
}

static int
call_transaction_monitor(_PyCallTransaction *call, _PyInstr **next)
{
    assert(!call->monitored);
    PyValue *homes = _PyFrame_Slots(call->operation.frame) + call->base;
    PyValue function = PyValue_MakeHeapSafe(PyValue_Borrow(homes[0]));
    PyValue first = PyValue_NULL;
    if (call->kind == CALL_TRANSACTION_EX) {
        PyObject *tuple = PyValue_AsPyObjectBorrow(homes[2]);
        if (PyTuple_GET_SIZE(tuple)) {
            first = PyValue_MakeHeapSafe(PyTuple_GET_VALUE(tuple, 0));
        }
    }
    else if (!PyValue_IsNull(homes[1])) {
        first = PyValue_MakeHeapSafe(PyValue_Borrow(homes[1]));
    }
    else if (call->argument_count) {
        first = PyValue_MakeHeapSafe(PyValue_Borrow(homes[2]));
    }
    PyObject *function_object = NULL, *argument = NULL;
    call->effects = 1;
    call->monitored = 1;
    function_object = PyValue_AsObject(function);
    if (function_object == NULL || PyErr_Occurred()) {
        goto done;
    }
    argument = PyValue_IsNull(first) ? Py_NewRef(&_PyInstrumentation_MISSING)
                                   : PyValue_AsObject(first);
    if (argument == NULL || call_transaction_refresh(call, next) < 0) {
        goto done;
    }
    (void)_Py_call_instrumentation_2args(
        call->operation.tstate, PY_MONITORING_EVENT_CALL, call->operation.frame,
        call->operation.frame->instr_ptr, function_object, argument);
done:
    call_transaction_close_object(&argument);
    call_transaction_close_object(&function_object);
    call_transaction_close_value(&first);
    call_transaction_close_value(&function);
    return call_transaction_refresh(call, next);
}

static void
call_transaction_publish_pc(void *opaque)
{
    _PyCallTransaction *call = opaque;
    _PyValueOperation *op = &call->operation;
    assert(op->tstate->current_frame == op->frame);
    assert(_PyFrame_GetCode(op->frame) == op->code);
    _PyCodeExec *exec = op->code->_co_exec;
    assert(exec != NULL && op->current_word >= 0 &&
           op->current_word < exec->nwords);
    op->frame->ir_words = exec->words;
    op->frame->ir_feedback = exec->entries;
    op->frame->instr_ptr = exec->words + op->current_word;
}

static void
call_transaction_detach(_PyCallTransaction *call, PyValue *retired)
{
    PyValue *homes = _PyFrame_Slots(call->operation.frame) + call->base;
    for (int i = 0; i < call->count; i++) {
        retired[i] = homes[i];
        homes[i] = PyValue_NULL;
    }
}

static void
call_transaction_retire(PyValue *retired, int count, PyValue *small)
{
    for (int i = count; i-- > 0;) {
        call_transaction_close_value(&retired[i]);
    }
    if (retired != small && retired != NULL) {
        PyObject *saved = PyErr_GetRaisedException();
        PyMem_Free(retired);
        _PyValue_RestoreException(saved);
    }
}

/* Result publication targets the captured destination itself, including a
 * folded LOCAL or an input overlap. The producer must not move window[0]
 * afterward, even on an error edge after cleanup. */
static int
call_transaction_native(_PyCallTransaction *call, _PyInstr **next)
{
    PyValue small[8];
    PyValue *retired = small;
    int retired_count = 0, scope_started = 0;
    PyObject *callable = NULL, *monitor_argument = NULL;
    _PyNativeScope scope;
    call->effects = 1;
    if (call->instrumented && !call->monitored &&
        call_transaction_monitor(call, next) < 0) {
        return -1;
    }
    if (call->count > (int)Py_ARRAY_LENGTH(small)) {
        retired = PyMem_Malloc((size_t)call->count * sizeof(PyValue));
        if (retired == NULL && !PyErr_Occurred()) {
            PyErr_NoMemory();
        }
        if (retired == NULL || call_transaction_refresh(call, next) < 0) {
            goto done;
        }
    }
    PyValue *homes = _PyFrame_Slots(call->operation.frame) + call->base;
    callable = PyValue_AsObject(homes[0]);
    if (callable == NULL || call_transaction_refresh(call, next) < 0) {
        goto done;
    }
    homes = _PyFrame_Slots(call->operation.frame) + call->base;
    scope_started = 1;
    int status;
    if (call->kind == CALL_TRANSACTION_EX) {
        PyObject *tuple = PyValue_AsPyObjectBorrow(homes[2]);
        PyObject *kwargs = PyValue_AsPyObjectBorrow(homes[3]);
        status = _PyNativeScope_BeginTupleCall(
            &scope, call->operation.tstate, callable, tuple, kwargs);
    }
    else {
        int offset = PyValue_IsNull(homes[1]) ? 2 : 1;
        int total = call->argument_count + 2 - offset;
        PyObject *names = call->kind == CALL_TRANSACTION_KEYWORDS
            ? PyValue_AsPyObjectBorrow(homes[call->count - 1]) : NULL;
        Py_ssize_t positional = total - (names == NULL ? 0 : PyTuple_GET_SIZE(names));
        if (_PyCFunction_CanCallScoped(callable)) {
            status = _PyNativeScope_BeginVector(
                &scope, call->operation.tstate, callable, homes + offset,
                positional, total, names);
        }
        else {
            status = _PyNativeScope_BeginObjectVector(
                &scope, call->operation.tstate, callable, homes + offset,
                positional, total, names);
        }
    }
    if (status < 0 || call_transaction_refresh(call, next) < 0) {
        goto abort;
    }
    if (call->kind != CALL_TRANSACTION_EX) {
        PyObject *const *objects;
        if (_PyNativeScope_Objects(&scope, &objects) < 0) {
            goto abort;
        }
        if (call->instrumented) {
            monitor_argument = Py_NewRef(scope.nargs ? objects[0]
                                                    : &_PyInstrumentation_MISSING);
        }
    }
    else if (call->instrumented) {
        PyValue first = PyTuple_GET_SIZE(scope.callargs)
            ? PyTuple_GET_VALUE(scope.callargs, 0) : PyValue_NULL;
        monitor_argument = PyValue_IsNull(first)
            ? Py_NewRef(&_PyInstrumentation_MISSING) : PyValue_AsObject(first);
        if (monitor_argument == NULL || PyErr_Occurred()) {
            goto abort;
        }
    }
    if (call_transaction_refresh(call, next) < 0) {
        goto abort;
    }
    if (call->kind == CALL_TRANSACTION_EX) {
        status = _PyNativeScope_InvokeTupleCall(&scope);
    }
    else {
        status = scope.vector == 1
            ? _PyCFunction_CallScoped(&scope, scope.callable, &scope.result) : 1;
        if (status == 1) {
            /* NOT_INVOKED may change adapters, never replay preparation. */
            status = _PyNativeScope_InvokeObjectVector(&scope);
        }
    }
    (void)call_transaction_refresh(call, next);
    if (call->instrumented &&
        !(call->kind == CALL_TRANSACTION_EX &&
          (PyFunction_Check(callable) || PyMethod_Check(callable)))) {
        if (status < 0 || PyErr_Occurred()) {
            _Py_call_instrumentation_exc2(
                call->operation.tstate, PY_MONITORING_EVENT_C_RAISE,
                call->operation.frame, call->operation.frame->instr_ptr,
                callable, monitor_argument);
        }
        else {
            (void)_Py_call_instrumentation_2args(
                call->operation.tstate, PY_MONITORING_EVENT_C_RETURN,
                call->operation.frame, call->operation.frame->instr_ptr,
                callable, monitor_argument);
        }
    }
    if (status < 0 || call_transaction_refresh(call, next) < 0) {
        goto abort;
    }
    call_transaction_detach(call, retired);
    retired_count = call->count;
    _PyNativeResult_Publish(&scope.result,
        _PyFrame_Slots(call->operation.frame) + call->destination,
        call_transaction_publish_pc, call);
    goto end_scope;
abort:
    _PyNativeResult_Abort(&scope.result, call_transaction_publish_pc, call);
end_scope:
    (void)_PyNativeScope_End(&scope);
done:
    assert(scope_started || retired_count == 0);
    call_transaction_close_object(&monitor_argument);
    call_transaction_close_object(&callable);
    call_transaction_retire(retired, retired_count, small);
    int failed = call_transaction_refresh(call, next);
    if (failed == 0 && call->periodic) {
        (void)check_periodics_at_end(call->operation.tstate, call->operation.frame);
        failed = call_transaction_refresh(call, next);
    }
    return failed;
}

static int call_transaction_invoke(_PyCallTransaction *, _PyInstr **,
                                    _PyInterpreterFrame **);

static int
value_reader_complete(_PyValueOperation *operation, _PyInstr **next,
                      _PyValueTemps *temps, PyValue *result,
                      PyValue *first, PyValue *second)
{
    _PyInterpreterFrame *frame = operation->frame;
    int slot = operation->result_slot;
    /* All indices were captured before the provider. A folded destination
     * stays untouched until the original operand owners have retired. */
    if (operation->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != operation->code) {
        PyObject *saved = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_SystemError, "value reader changed the active frame");
        _PyValue_RestoreException(saved);
        drop_failed_value(result);
        value_operation_close3(first, second, NULL);
        _PyValueTemps_Close(temps);
        return -1;
    }
    int local = slot < operation->code->co_nlocalsplus;
    if (slot < 0 ||
        slot >= _PyFrame_NumSlots(frame) ||
        (!local && !PyValue_IsNull(_PyFrame_Slots(frame)[slot]))) {
        PyObject *saved = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_SystemError, "value reader changed its result home");
        _PyValue_RestoreException(saved);
        drop_failed_value(result);
        value_operation_close3(first, second, NULL);
        _PyValueTemps_Close(temps);
        return value_operation_refresh(operation, next, NULL);
    }
    *result = _PyValueTemps_Publish(temps, result);
    if (!local) {
        _PyFrame_Slots(frame)[slot] = *result;
    }
    value_operation_close3(first, second, NULL);
    _PyValueTemps_Close(temps);
    if (PyErr_Occurred() && local) {
        drop_failed_value(result);
    }
    return value_operation_refresh(operation, next, local ? result : NULL);
}

static int
finish_value_bridges(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr **next, _PyValueTemps *temps)
{
    return finish_value_bridges_ex(tstate, frame, next, temps, NULL);
}

static int
value_attribute_complete(_PyValueOperation *operation, _PyInstr **next,
                         _PyValueTemps *temps, PyValue *result,
                         PyValue *self, PyValue *receiver, int method)
{
    if (!method) {
        return value_reader_complete(operation, next, temps, result, receiver, NULL);
    }
    _PyInterpreterFrame *frame = operation->frame;
    int slot = operation->result_slot;
    if (operation->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != operation->code ||
        slot < operation->code->co_nlocalsplus || slot + 1 >= _PyFrame_NumSlots(frame) ||
        !PyValue_IsNull(_PyFrame_Slots(frame)[slot]) ||
        !PyValue_IsNull(_PyFrame_Slots(frame)[slot + 1])) {
        PyObject *saved = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_SystemError, "method lookup changed its output homes");
        _PyValue_RestoreException(saved);
        drop_failed_value(result);
        drop_failed_value(self);
        value_operation_close3(receiver, NULL, NULL);
        _PyValueTemps_Close(temps);
        return value_operation_refresh(operation, next, NULL);
    }
    *result = _PyValueTemps_Publish(temps, result);
    *self = _PyValueTemps_Publish(temps, self);
    _PyFrame_Slots(frame)[slot] = *result;
    _PyFrame_Slots(frame)[slot + 1] = *self;
    value_operation_close3(receiver, NULL, NULL);
    _PyValueTemps_Close(temps);
    return value_operation_refresh(operation, next, NULL);
}

static int
retire_published_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr **next, _PyValueTemps *temps,
                        PyValue *first, PyValue *second, PyValue *failed_result)
{
    PyValue owners[2] = {*first, second == NULL ? PyValue_NULL : *second};
    *first = PyValue_NULL;
    if (second != NULL) {
        *second = PyValue_NULL;
    }
    /* LIFO retires the displaced field before its receiver. Both are
     * detached before any callback, and no raw field pointer crosses it. */
    for (int i = 1; i >= 0; i--) {
        if (PyValue_IsObject(owners[i]) && (owners[i].bits & 1) == 0) {
            assert(temps->count < temps->capacity);
            temps->objects[temps->count++] = (PyObject *)owners[i].bits;
        }
    }
    return finish_value_bridges_ex(tstate, frame, next, temps, failed_result);
}

typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    PyCodeObject *code;
    Py_ssize_t current_word;
    Py_ssize_t next_word;
    int base;
    int inputs;
    PyValue *result;
} _PyNativeCallPublication;

static _PyNativeCallPublication
native_call_publication(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *next, int base, int inputs, PyValue *result)
{
    _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
    assert(exec != NULL && frame->ir_words == exec->words);
    return (_PyNativeCallPublication){
        tstate, frame, _PyFrame_GetCode(frame),
        frame->instr_ptr - exec->words, next - exec->words,
        base, inputs, result,
    };
}

/* These callbacks only publish homes and the already-selected logical PC.
 * The original input owners have moved to caller-local c/s before publication. */
static void
native_call_publish_pc(void *opaque)
{
    _PyNativeCallPublication *state = opaque;
    _PyInterpreterFrame *frame = state->frame;
    assert(state->tstate->current_frame == frame);
    assert(_PyFrame_GetCode(frame) == state->code);
    _PyCodeExec *exec = state->code->_co_exec;
    assert(exec != NULL && state->current_word >= 0 &&
           state->current_word < exec->nwords);
    frame->ir_words = exec->words;
    frame->ir_feedback = exec->entries;
    frame->instr_ptr = exec->words + state->current_word;
}

static void
native_call_publish_result(void *opaque)
{
    _PyNativeCallPublication *state = opaque;
    native_call_publish_pc(state);
    PyValue *window = _PyFrame_Slots(state->frame) + state->base;
    for (int i = 0; i < state->inputs; i++) {
        window[i] = PyValue_NULL;
    }
    window[0] = *state->result;
}

static int
native_call_finish(_PyNativeCallPublication *state, _PyNativeScope *scope,
                   PyValue *argument, PyValue *callable, _PyInstr **next)
{
    (void)_PyNativeScope_End(scope);
    PyObject *saved = PyErr_GetRaisedException();
    if (argument != NULL) {
        PyValue owner = *argument;
        *argument = PyValue_NULL;
        PyValue_XCLOSE(owner);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
        owner = *callable;
        *callable = PyValue_NULL;
        PyValue_XCLOSE(owner);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    _PyInterpreterFrame *frame = state->frame;
    if (state->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != state->code) {
        PyErr_SetString(PyExc_SystemError, "native cleanup changed the active frame");
        _PyValue_RestoreException(saved);
        return -1;
    }
    uintptr_t epoch = _Py_atomic_load_uintptr_relaxed(&state->tstate->eval_breaker) &
                      ~_PY_EVAL_EVENTS_MASK;
    if (state->code->_co_instrumentation_version != epoch) {
        (void)_Py_Instrument(state->code, state->tstate->interp);
    }
    _PyCodeExec *exec = state->code->_co_exec;
    if (state->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != state->code || exec == NULL ||
        state->current_word < 0 || state->current_word >= exec->nwords ||
        state->next_word < 0 || state->next_word > exec->nwords) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "native cleanup changed executing words");
        }
        _PyValue_RestoreException(saved);
        return -1;
    }
    frame->ir_words = exec->words;
    frame->ir_feedback = exec->entries;
    frame->instr_ptr = exec->words + state->current_word;
    *next = exec->words + state->next_word;
    _PyValue_RestoreException(saved);
    return PyErr_Occurred() ? -1 : 0;
}

#include "generated_jit_helpers.c.h"

PyObject* _Py_HOT_FUNCTION DONT_SLP_VECTORIZE
_PyEval_EvalFrameDefault(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    check_invalid_reentrancy();
    CALL_STAT_INC(pyeval_calls);

#if USE_COMPUTED_GOTOS && !_Py_TAIL_CALL_INTERP
/* Import the static jump table */
#include "ir_targets.h"
#endif

#ifdef Py_STATS
    int lastopcode = 0;
#endif
#if !_Py_TAIL_CALL_INTERP
    int opcode;        /* Current handler */
    _PyIRWord ir_word = 0;  /* Its word, from which the handler takes its fields */
#endif
    _PyEntryFrame entry;
    _PyValueTemps *value_temps = NULL;
#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
    _PyJitTestRequest *jit_request = NULL;
    _PyJitContext jit_context = {0};
    _PyJitAction jit_result;
    int jit_from_return = 0;
    _PyJitFlow *jit_flow = NULL;
    void *jit_cold_request = NULL;
#endif

    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
        _PyEval_FrameClearAndPop(tstate, frame);
        return NULL;
    }

    /* Local "register" variables.
     * These are cached values from the frame and code object.  */
    _PyInstr *next_instr;
    PyValue *slots;
    entry.stack[0] = PyValue_NULL;
#if defined(Py_DEBUG)
    /* Set these to invalid but identifiable values for debugging. */
    entry.frame.f_funcobj = (PyValue){.bits = 0xaaa0};
    entry.frame.f_locals = (PyObject*)0xaaa1;
    entry.frame.frame_obj = (PyFrameObject*)0xaaa2;
    entry.frame.f_globals = (PyObject*)0xaaa3;
    entry.frame.f_builtins = (PyObject*)0xaaa4;
#endif
    entry.frame.f_executable = PyValue_None;
    entry.frame.base = entry.frame.owned_slots;
    /* A return or a yield resumes the entry frame at the word after its
     * instr_ptr, the first INTERPRETER_EXIT */
    entry.frame.instr_ptr = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry.frame.ir_words = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry.frame.ir_feedback = NULL;
    entry.frame.owner = FRAME_OWNED_BY_INTERPRETER;
    entry.frame.visited = 0;
    /* What the frame returns or yields lands in entry.stack[0] */
    entry.frame.return_slot = 1;
#ifdef Py_DEBUG
    entry.frame.lltrace = 0;
#endif
    /* Push frame */
    entry.frame.previous = tstate->current_frame;
    frame->previous = &entry.frame;
    tstate->current_frame = frame;
#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
    if (!throwflag) {
        jit_request = _PyJit_TestClaim(tstate, frame, &jit_cold_request);
    }
#endif

    /* support for generator.throw() */
    if (throwflag) {
        if (_Py_EnterRecursivePy(tstate)) {
            goto early_exit;
        }
        /* Because this avoids the RESUME, we need to update instrumentation */
        _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
        next_instr = frame->instr_ptr;
        monitor_throw(tstate, frame, next_instr);
        slots = _PyFrame_Slots(frame);
#if _Py_TAIL_CALL_INTERP
#   if Py_STATS
        return _TAIL_CALL_error(slots, tstate, next_instr, TAIL_CALL_NO_OPERANDS, lastopcode);
#   else
        return _TAIL_CALL_error(slots, tstate, next_instr, TAIL_CALL_NO_OPERANDS);
#   endif
#else
        goto error;
#endif
    }

#if _Py_TAIL_CALL_INTERP
#   if Py_STATS
        return _TAIL_CALL_start_frame(NULL, tstate, NULL, TAIL_CALL_NO_OPERANDS, lastopcode);
#   else
        return _TAIL_CALL_start_frame(NULL, tstate, NULL, TAIL_CALL_NO_OPERANDS);
#   endif
#else
    goto start_frame;
#if !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
    jit_resume:
    {
        /* PREPARED transfer pins are released only after start_frame has
         * completed recursion/events. RETURN arrives in an already-live caller. */
        if (jit_context.unit != NULL && _PyJit_ReleaseActivation(&jit_context) < 0) {
            goto error;
        }
        if (jit_request != NULL) {
            goto jit_step;
        }
        if (!jit_from_return || !_PyJit_PopPending(jit_flow, frame, next_instr)) {
            DISPATCH();
        }
    }
    jit_enter:
    {
        int entered = _PyJit_PrepareActivation(tstate, frame, next_instr,
                                               jit_from_return, &jit_context);
        if (entered < 0) {
            goto error;
        }
        if (entered) {
            jit_context.flow_owner = &jit_flow;
            jit_context.cold_request = jit_cold_request;
            jit_result = jit_context.unit->entry(&jit_context);
            goto jit_consume_action;
        }
        DISPATCH();
    }

    jit_step:
    {
        /* M0 executes real generated helpers, not native machine code. An
         * unavailable adapter ends stepping for this entire activation. */
        if (jit_request == NULL || frame != jit_request->claimed_frame) {
            DISPATCH();
        }
        if (jit_request->remaining-- <= 0 || !_PyJit_ObserversAllow(tstate, frame)) {
            jit_request->claimed_frame = NULL;
            jit_request = NULL;
            DISPATCH();
        }
        PyCodeObject *code = _PyFrame_GetCode(frame);
        Py_ssize_t offset = next_instr - frame->ir_words;
        assert(offset >= 0 && offset < Py_SIZE(code));
        _PyIRWord pristine = _PyCode_WORDS(code)[offset];
        /* Applies the existing per-interpreter boxed-constant policy without
         * importing executing-only quickening into the semantic descriptor. */
        int handler = _PyCode_GenericHandler(code, (int)offset);
        if (handler >= Py_ARRAY_LENGTH(jit_helpers) || jit_helpers[handler].helper == NULL) {
            jit_request->actions[_PyJIT_DEOPT]++;
            jit_request->phases[_PyJIT_DEOPT_BEFORE]++;
            jit_request->claimed_frame = NULL;
            jit_request = NULL;
            DISPATCH();
        }
        _PyJitDecodedOp op = {
            .instruction = next_instr,
            .fields = {IR_A(pristine), IR_B(pristine), IR_C(pristine), IR_D(pristine)},
            .wide = IR_CD(pristine), .handler = (uint16_t)handler,
        };
        _PyJitContext ctx = {.tstate = tstate, .frame = frame, .next_instr = next_instr, .cold_request = jit_cold_request};
        /* Retire the diagnostic frame identity before RETURN can clear it
         * and invoke finalizers which might allocate another frame there. */
        if (handler == RETURN_VALUE && frame == jit_request->claimed_frame) {
            jit_request->claimed_frame = NULL;
        }
        _PyJitAction action = jit_helpers[handler].helper(&ctx, &op);
        jit_request->actions[action]++;
        jit_request->phases[ctx.phase]++;
        jit_context = ctx;
        jit_result = action;
        goto jit_consume_action;
    }

    jit_consume_action:
    {
        /* This is the sole owning context. A local struct copy would leave
         * stale owned pins in jit_context after a terminal Action. */
        frame = jit_context.frame;
        next_instr = jit_context.next_instr;
        slots = _PyFrame_Slots(frame);
        switch (jit_result) {
            case _PyJIT_CONTINUE:
            case _PyJIT_BRANCH:
                assert(jit_context.unit == NULL);  /* M0 only: native stays native */
                goto jit_step;
            case _PyJIT_RETURN:
                assert(jit_context.phase == _PyJIT_RETURN_COMPLETED);
                jit_from_return = 1;
                goto jit_resume;  /* release terminal pin, then fresh caller lookup */
            case _PyJIT_ENTER_FRAME:
                _PyJit_PushPending(&jit_context);
                if (jit_context.phase == _PyJIT_ENTER_PREPARED) {
                    tstate->current_frame = frame;
                    CALL_STAT_INC(inlined_py_calls);
                    goto start_frame;  /* releases old pin after start completes */
                }
                assert(jit_context.phase == _PyJIT_ENTER_STARTED);
                jit_from_return = 0;
                goto jit_resume;
            case _PyJIT_RAISE:
                if (jit_request != NULL) {
                    jit_request->claimed_frame = NULL;
                    jit_request = NULL;
                }
                next_instr = jit_context.raising_instr + 1;
                _PyJit_ReleaseActivation(&jit_context);
                if (jit_context.phase == _PyJIT_UNWIND_PENDING) {
                    goto exception_unwind;
                }
                assert(jit_context.phase == _PyJIT_ERROR_LOG_PENDING);
                goto error;
            case _PyJIT_DEOPT:
                assert(jit_context.phase == _PyJIT_DEOPT_BEFORE || jit_context.phase == _PyJIT_DEOPT_AFTER);
                if (jit_request != NULL) {
                    jit_request->claimed_frame = NULL;
                    jit_request = NULL;
                }
                if (_PyJit_ReleaseActivation(&jit_context) < 0) {
                    if (jit_context.phase == _PyJIT_DEOPT_BEFORE) {
                        /* error attributes next_instr - 1; normal fallback
                         * must retain the pending instruction unchanged. */
                        next_instr++;
                    }
                    goto error;
                }
                DISPATCH();  /* exact pending/committed next_instr, no replay */
            default:
                Py_UNREACHABLE();
        }
    }
#endif
#       include "generated_ir_cases.c.h"
#endif


early_exit:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame = dying->previous;
    JIT_RETIRE_COLD(dying);
    _PyEval_FrameClearAndPop(tstate, dying);
    assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
    /* Restore previous frame and exit */
    tstate->current_frame = frame->previous;
#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
    if (jit_flow != NULL) _PyJit_FreeFlow(&jit_flow);
#endif
    return NULL;
}


#ifdef DO_NOT_OPTIMIZE_INTERP_LOOP
#  pragma optimize("", on)
#endif

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER) /* MS_WINDOWS */
#  pragma warning(pop)
#endif

static void
format_missing(PyThreadState *tstate, const char *kind,
               PyCodeObject *co, PyObject *names, PyObject *qualname)
{
    int err;
    Py_ssize_t len = PyList_GET_SIZE(names);
    PyObject *name_str, *comma, *tail, *tmp;

    assert(PyList_CheckExact(names));
    assert(len >= 1);
    /* Deal with the joys of natural language. */
    switch (len) {
    case 1:
        name_str = PyValue_AsPyObjectBorrow(PyList_GET_VALUE(names, 0));
        Py_INCREF(name_str);
        break;
    case 2:
        name_str = PyUnicode_FromFormat("%U and %U",
                                        PyValue_AsPyObjectBorrow(PyList_GET_VALUE(names, len - 2)),
                                        PyValue_AsPyObjectBorrow(PyList_GET_VALUE(names, len - 1)));
        break;
    default:
        tail = PyUnicode_FromFormat(", %U, and %U",
                                    PyValue_AsPyObjectBorrow(PyList_GET_VALUE(names, len - 2)),
                                    PyValue_AsPyObjectBorrow(PyList_GET_VALUE(names, len - 1)));
        if (tail == NULL)
            return;
        /* Chop off the last two objects in the list. This shouldn't actually
           fail, but we can't be too careful. */
        err = PyList_SetSlice(names, len - 2, len, NULL);
        if (err == -1) {
            Py_DECREF(tail);
            return;
        }
        /* Stitch everything up into a nice comma-separated list. */
        comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            Py_DECREF(tail);
            return;
        }
        tmp = PyUnicode_Join(comma, names);
        Py_DECREF(comma);
        if (tmp == NULL) {
            Py_DECREF(tail);
            return;
        }
        name_str = PyUnicode_Concat(tmp, tail);
        Py_DECREF(tmp);
        Py_DECREF(tail);
        break;
    }
    if (name_str == NULL)
        return;
    _PyErr_Format(tstate, PyExc_TypeError,
                  "%U() missing %zd required %s argument%s: %U",
                  qualname,
                  len,
                  kind,
                  len == 1 ? "" : "s",
                  name_str);
    Py_DECREF(name_str);
}

static void
missing_arguments(PyThreadState *tstate, PyCodeObject *co,
                  Py_ssize_t missing, Py_ssize_t defcount,
                  PyValue *localsplus, PyObject *qualname)
{
    Py_ssize_t i, j = 0;
    Py_ssize_t start, end;
    int positional = (defcount != -1);
    const char *kind = positional ? "positional" : "keyword-only";
    PyObject *missing_names;

    /* Compute the names of the arguments that are missing. */
    missing_names = PyList_New(missing);
    if (missing_names == NULL)
        return;
    if (positional) {
        start = 0;
        end = co->co_argcount - defcount;
    }
    else {
        start = co->co_argcount;
        end = start + co->co_kwonlyargcount;
    }
    for (i = start; i < end; i++) {
        if (PyValue_IsNull(localsplus[i])) {
            PyObject *raw = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
            PyObject *name = PyObject_Repr(raw);
            if (name == NULL) {
                Py_DECREF(missing_names);
                return;
            }
            PyList_SET_ITEM(missing_names, j++, name);
        }
    }
    assert(j == missing);
    format_missing(tstate, kind, co, missing_names, qualname);
    Py_DECREF(missing_names);
}

static void
too_many_positional(PyThreadState *tstate, PyCodeObject *co,
                    Py_ssize_t given, PyObject *defaults,
                    PyValue *localsplus, PyObject *qualname,
                    int should_suggest_missing_self)
{
    int plural;
    Py_ssize_t kwonly_given = 0;
    Py_ssize_t i;
    PyObject *sig, *kwonly_sig;
    const char *self_hint = "";
    Py_ssize_t co_argcount = co->co_argcount;

    assert((co->co_flags & CO_VARARGS) == 0);
    /* Count missing keyword-only args. */
    for (i = co_argcount; i < co_argcount + co->co_kwonlyargcount; i++) {
        if (!PyValue_IsNull(localsplus[i])) {
            kwonly_given++;
        }
    }
    Py_ssize_t defcount = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    if (defcount) {
        Py_ssize_t atleast = co_argcount - defcount;
        plural = 1;
        sig = PyUnicode_FromFormat("from %zd to %zd", atleast, co_argcount);
    }
    else {
        plural = (co_argcount != 1);
        sig = PyUnicode_FromFormat("%zd", co_argcount);
    }
    if (sig == NULL)
        return;
    if (kwonly_given) {
        const char *format = " positional argument%s (and %zd keyword-only argument%s)";
        kwonly_sig = PyUnicode_FromFormat(format,
                                          given != 1 ? "s" : "",
                                          kwonly_given,
                                          kwonly_given != 1 ? "s" : "");
        if (kwonly_sig == NULL) {
            Py_DECREF(sig);
            return;
        }
    }
    else {
        /* This will not fail. */
        kwonly_sig = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
        assert(kwonly_sig != NULL);
    }
    if (should_suggest_missing_self) {
        self_hint = ". Did you forget the 'self' parameter "
                    "in the function definition?";
    }
    _PyErr_Format(tstate, PyExc_TypeError,
                  "%U() takes %U positional argument%s but %zd%U %s given%s",
                  qualname,
                  sig,
                  plural ? "s" : "",
                  given,
                  kwonly_sig,
                  given == 1 && !kwonly_given ? "was" : "were",
                  self_hint
                );
    Py_DECREF(sig);
    Py_DECREF(kwonly_sig);
}

static int
suggest_missing_self(PyFunctionObject *func, PyCodeObject *co,
                     PyObject *first_argument, Py_ssize_t argcount)
{
    /* Missing self shows up as exactly one extra positional argument. */
    if ((co->co_argcount + 1) != argcount) {
        return 0;
    }

    if (first_argument == NULL || PyType_Check(first_argument)) {
        /* When first arg is NULL, it's not really about self.
           If it's a type object, then it's a classmethod. */
        return 0;
    }

    if (co->co_argcount > 0) {
        PyObject *first_parameter_name = _PyTuple_GET_OBJECT(co->co_localsplusnames, 0);
        /* If the receiver parameter is already declared, another hint would be misleading. */
        if (PyUnicode_CompareWithASCIIString(first_parameter_name, "self") == 0 ||
            PyUnicode_CompareWithASCIIString(first_parameter_name, "cls") == 0)
        {
            return 0;
        }
    }
    /* If the current function matches on the type, it's likely worth adding the hint. */
    PyTypeObject *self_cls = Py_TYPE(first_argument);
    PyObject *possibly_current_function = _PyType_LookupRef(self_cls, co->co_name);
    int res = possibly_current_function == (PyObject *)func;
    Py_XDECREF(possibly_current_function);
    return res;
}

static int
positional_only_passed_as_keyword(PyThreadState *tstate, PyCodeObject *co,
                                  Py_ssize_t kwcount, PyObject* kwnames,
                                  PyFunctionObject *func)
{
    int posonly_conflicts = 0;
    PyObject* posonly_names = PyList_New(0);
    if (posonly_names == NULL) {
        goto fail;
    }
    for(int k=0; k < co->co_posonlyargcount; k++){
        PyObject* posonly_name = _PyTuple_GET_OBJECT(co->co_localsplusnames, k);

        for (int k2=0; k2<kwcount; k2++){
            /* Compare the pointers first and fallback to PyObject_RichCompareBool*/
            PyValue kwvalue = PyTuple_GET_VALUE(kwnames, k2);
            if (!PyValue_IsObject(kwvalue)) {
                continue;
            }
            PyObject* kwname = PyValue_AsPyObjectBorrow(kwvalue);
            if (kwname == posonly_name){
                if(PyList_Append(posonly_names, kwname) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
                continue;
            }

            int cmp = PyObject_RichCompareBool(posonly_name, kwname, Py_EQ);

            if ( cmp > 0) {
                if(PyList_Append(posonly_names, kwname) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
            } else if (cmp < 0) {
                goto fail;
            }

        }
    }
    if (posonly_conflicts) {
        PyObject* comma = PyUnicode_FromString(", ");
        if (comma == NULL) {
            goto fail;
        }
        PyObject* error_names = PyUnicode_Join(comma, posonly_names);
        Py_DECREF(comma);
        if (error_names == NULL) {
            goto fail;
        }
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%U() got some positional-only arguments passed"
                      " as keyword arguments: '%U'",
                      FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname),
                      error_names);
        Py_DECREF(error_names);
        goto fail;
    }

    Py_DECREF(posonly_names);
    return 0;

fail:
    Py_XDECREF(posonly_names);
    return 1;

}

static int
initialize_locals(PyThreadState *tstate, PyFunctionObject *func, PyCodeObject *co,
    PyValue *localsplus, PyValue const *args,
    Py_ssize_t argcount, PyObject *kwnames)
{
    const Py_ssize_t total_args = co->co_argcount + co->co_kwonlyargcount;
    /* Create a dictionary for keyword parameters (**kwags) */
    PyObject *kwdict;
    Py_ssize_t i;
    PyObject *first_argument = NULL;
    if (co->co_flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL) {
            goto fail_pre_positional;
        }
        i = total_args;
        if (co->co_flags & CO_VARARGS) {
            i++;
        }
        assert(PyValue_IsNull(localsplus[i]));
        localsplus[i] = PyValue_FromPyObjectSteal(kwdict);
    }
    else {
        kwdict = NULL;
    }

    /* For the missing self hint, which an int or float never gets: its type
       has no Python methods. */
    if (argcount > co->co_argcount && !(co->co_flags & CO_VARARGS)
        && !PyValue_IsInline(args[0]))
    {
        first_argument = PyValue_AsPyObjectNew(args[0]);
    }

    /* Copy all positional arguments into local variables */
    Py_ssize_t j, n;
    if (argcount > co->co_argcount) {
        n = co->co_argcount;
    }
    else {
        n = argcount;
    }
    for (j = 0; j < n; j++) {
        assert(PyValue_IsNull(localsplus[j]));
        localsplus[j] = args[j];
    }

    /* Pack other positional arguments into the *args argument */
    if (co->co_flags & CO_VARARGS) {
        PyObject *u = NULL;
        if (argcount == n) {
            u = (PyObject *)&_Py_SINGLETON(tuple_empty);
        }
        else {
            u = _PyTuple_FromValueStealOnSuccess(args + n, argcount - n);
            if (u == NULL) {
                for (Py_ssize_t i = n; i < argcount; i++) {
                    PyValue_CLOSE(args[i]);
                }
            }
        }
        if (u == NULL) {
            goto fail_post_positional;
        }
        assert(PyValue_IsNull(localsplus[total_args]));
        localsplus[total_args] = PyValue_FromPyObjectSteal(u);
    }
    else if (argcount > n) {
        /* Too many positional args. Error is reported later */
        for (j = n; j < argcount; j++) {
            PyValue_CLOSE(args[j]);
        }
    }

    /* Handle keyword arguments */
    if (kwnames != NULL) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (i = 0; i < kwcount; i++) {
            PyValue *co_varnames;
            PyValue keyword_value = PyTuple_GET_VALUE(kwnames, i);
            PyObject *keyword = PyValue_IsObject(keyword_value)
                ? PyValue_AsPyObjectBorrow(keyword_value) : NULL;
            PyValue value_value = args[i+argcount];
            Py_ssize_t j;

            if (keyword == NULL || !PyUnicode_Check(keyword)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%U() keywords must be strings",
                              FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname));
                goto kw_fail;
            }

            /* Speed hack: do raw pointer compares. As names are
            normally interned this should almost always hit. */
            co_varnames = ((PyTupleObject *)(co->co_localsplusnames))->ob_item;
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = PyValue_AsPyObjectBorrow(co_varnames[j]);
                if (varname == keyword) {
                    goto kw_found;
                }
            }

            /* Slow fallback, just in case */
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = PyValue_AsPyObjectBorrow(co_varnames[j]);
                int cmp = PyObject_RichCompareBool( keyword, varname, Py_EQ);
                if (cmp > 0) {
                    goto kw_found;
                }
                else if (cmp < 0) {
                    goto kw_fail;
                }
            }

            assert(j >= total_args);
            if (kwdict == NULL) {

                if (co->co_posonlyargcount
                    && positional_only_passed_as_keyword(tstate, co,
                                                        kwcount, kwnames,
                                                        func))
                {
                    goto kw_fail;
                }

                PyObject* suggestion_keyword = NULL;
                if (total_args > co->co_posonlyargcount) {
                    PyObject* possible_keywords = PyList_New(total_args - co->co_posonlyargcount);

                    if (!possible_keywords) {
                        PyErr_Clear();
                    } else {
                        for (Py_ssize_t k = co->co_posonlyargcount; k < total_args; k++) {
                            PyList_SET_VALUE(possible_keywords, k - co->co_posonlyargcount,
                                             PyValue_NewRef(co_varnames[k]));
                        }

                        suggestion_keyword = _Py_CalculateSuggestions(possible_keywords, keyword);
                        Py_DECREF(possible_keywords);
                    }
                }

                PyObject *qualname = FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname);
                if (suggestion_keyword) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                "%U() got an unexpected keyword argument '%S'. Did you mean '%S'?",
                                qualname, keyword, suggestion_keyword);
                    Py_DECREF(suggestion_keyword);
                } else {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                "%U() got an unexpected keyword argument '%S'",
                                qualname, keyword);
                }

                goto kw_fail;
            }

            PyObject *value = PyValue_AsPyObjectNewBoxed(value_value);
            if (value == NULL) {
                goto kw_fail;
            }
            int err = PyDict_SetItem(kwdict, keyword, value);
            Py_DECREF(value);
            if (err == -1) {
                goto kw_fail;
            }
            PyValue_CLOSE(value_value);
            continue;

        kw_fail:
            for (;i < kwcount; i++) {
                PyValue_CLOSE(args[i+argcount]);
            }
            goto fail_post_args;

        kw_found:
            if (!PyValue_IsNull(localsplus[j])) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%U() got multiple values for argument '%S'",
                              FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname),
                              keyword);
                goto kw_fail;
            }
            localsplus[j] = value_value;
        }
    }

    /* Check the number of positional arguments */
    if ((argcount > co->co_argcount) && !(co->co_flags & CO_VARARGS)) {
        int missing_self_hint = suggest_missing_self(func, co, first_argument, argcount);
        too_many_positional(tstate, co, argcount, func->func_defaults, localsplus,
                            FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname),
                            missing_self_hint);
        goto fail_post_args;
    }

    /* Add missing positional arguments (copy default values from defs) */
    if (argcount < co->co_argcount) {
        Py_ssize_t defcount = func->func_defaults == NULL ? 0 : PyTuple_GET_SIZE(func->func_defaults);
        Py_ssize_t m = co->co_argcount - defcount;
        Py_ssize_t missing = 0;
        for (i = argcount; i < m; i++) {
            if (PyValue_IsNull(localsplus[i])) {
                missing++;
            }
        }
        if (missing) {
            missing_arguments(tstate, co, missing, defcount, localsplus,
                              FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname));
            goto fail_post_args;
        }
        if (n > m)
            i = n - m;
        else
            i = 0;
        if (defcount) {
            PyValue *defs = _PyTuple_ITEMS(func->func_defaults);
            for (; i < defcount; i++) {
                if (PyValue_IsNull(localsplus[m+i])) {
                    localsplus[m+i] = PyValue_NewRef(defs[i]);
                }
            }
        }
    }

    /* Add missing keyword arguments (copy default values from kwdefs) */
    if (co->co_kwonlyargcount > 0) {
        Py_ssize_t missing = 0;
        for (i = co->co_argcount; i < total_args; i++) {
            if (!PyValue_IsNull(localsplus[i]))
                continue;
            PyObject *varname = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
            if (func->func_kwdefaults != NULL) {
                PyObject *def;
                if (PyDict_GetItemRef(func->func_kwdefaults, varname, &def) < 0) {
                    goto fail_post_args;
                }
                if (def) {
                    PyObject *retired;
                    localsplus[i] = PyValue_PrepareOwned(def, &retired);
                    Py_XDECREF(retired);
                    if (PyErr_Occurred()) {
                        goto fail_post_args;
                    }
                    continue;
                }
            }
            missing++;
        }
        if (missing) {
            missing_arguments(tstate, co, missing, -1, localsplus,
                              FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname));
            goto fail_post_args;
        }
    }
    Py_XDECREF(first_argument);
    return 0;

fail_pre_positional:
    for (j = 0; j < argcount; j++) {
        PyValue_CLOSE(args[j]);
    }
    /* fall through */
fail_post_positional:
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (j = argcount; j < argcount+kwcount; j++) {
            PyValue_CLOSE(args[j]);
        }
    }
    /* fall through */
fail_post_args:
    Py_XDECREF(first_argument);
    return -1;
}

static void
clear_thread_frame(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    // Make sure that this is, indeed, the top frame. We can't check this in
    // _PyThreadState_PopFrame, since f_code is already cleared at that point:
    assert((PyObject **)frame == tstate->datastack_limit);
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    _PyFrame_ClearExceptCode(frame);
    PyValue_CLEAR(frame->f_executable);
    _PyThreadState_PopFrame(tstate, frame);
}

static void
clear_gen_frame(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    frame->previous = NULL;
    Py_BEGIN_CRITICAL_SECTION(gen);
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    _PyFrame_ClearExceptCode(frame);
    Py_END_CRITICAL_SECTION();
    _PyErr_ClearExcState(&gen->gi_exc_state);
    // gh-143939: There must not be any escaping calls between setting
    // the generator return kind and returning from _PyEval_EvalFrame.
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
}

_PyInstr *
_PyEval_GeneratorReturnTarget(_PyInterpreterFrame *frame)
{
    _PyInstr *next = frame->instr_ptr + 1;
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return next;
    }
    PyCodeObject *co = _PyFrame_GetCode(frame);
    int i = (int)_PyFrame_InstrOffset(frame, frame->instr_ptr);
    assert(_PyIR_BaseOpcode(_PyCode_InstrHandler(co, i)) == SEND ||
           _PyIR_BaseOpcode(_PyCode_InstrHandler(co, i)) == FOR_ITER);
    return next + _PyCode_InstrOparg(co, i);
}

void
_PyEval_FrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    // By this point, tstate->current_frame is already set to the parent frame.
    _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);

    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        clear_thread_frame(tstate, frame);
    }
    else {
        clear_gen_frame(tstate, frame);
    }
}

/* code is independently pinned through preparation and binding. Consumes
 * func, locals and args, but never reselects the function's mutable code. */
static _PyInterpreterFrame *
eval_frame_push_and_init_code(PyThreadState *tstate, PyValue func,
                        PyObject *locals, PyValue *args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous, PyCodeObject *code)
{
    func = PyValue_MakeHeapSafe(func);
    size_t total = argcount + (kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames));
    _PyValue_OwnArray(args, total);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_AsPyObjectBorrow(func);
    CALL_STAT_INC(frames_pushed);
    if (_PyCode_GetExec(code) == NULL) {
        goto fail;
    }
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
    if (frame == NULL) {
        goto fail;
    }
    _PyFrame_Initialize(tstate, frame, func, locals, code, frame->base, 0, previous);
    if (initialize_locals(tstate, func_obj, code, frame->base, args, argcount, kwnames)) {
        assert(frame->owner == FRAME_OWNED_BY_THREAD);
        PyObject *saved = PyErr_GetRaisedException();
        _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
        clear_thread_frame(tstate, frame);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return frame;
fail:
    ;
    if (!PyErr_Occurred()) {
        PyErr_NoMemory();
    }
    PyObject *saved = PyErr_GetRaisedException();
    /* Consume the references */
    PyValue_CLOSE(func);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_XDECREF(locals);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    for (size_t i = 0; i < argcount; i++) {
        PyValue_CLOSE(args[i]);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            PyValue_CLOSE(args[i+argcount]);
            _PyValue_RestoreException(saved);
            saved = PyErr_GetRaisedException();
        }
    }
    _PyValue_RestoreException(saved);
    return NULL;
}

/* Consumes references to func, locals and all the args. Explicit locals is
 * an input of this public provider; inferred policy belongs to its caller. */
_PyInterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyValue func,
                      PyObject *locals, PyValue *args, size_t argcount,
                      PyObject *kwnames, _PyInterpreterFrame *previous)
{
    func = PyValue_MakeHeapSafe(func);
    PyFunctionObject *function = (PyFunctionObject *)PyValue_AsPyObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(function->func_code);
    _PyInterpreterFrame *frame = eval_frame_push_and_init_code(
        tstate, func, locals, args, argcount, kwnames, previous, code);
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(code);
    _PyValue_RestoreException(saved);
    return frame;
}

/* Same as eval_frame_push_and_init_code but takes an args tuple and kwargs dict.
   Steals references to func, callargs and kwargs.
*/
static _PyInterpreterFrame *
eval_frame_push_and_init_ex_code(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs,
    _PyInterpreterFrame *previous, PyCodeObject *code)
{
    bool has_dict = (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0);
    PyObject *kwnames = NULL;
    PyValue *newargs = NULL;
    PyObject *const *object_array = NULL;
    PyObject **positionals = NULL;
    PyObject *scratch[MAX_VALUE_SCRATCH + 1];
    PyValue stack_array[8] = {0};
    Py_ssize_t total = nargs;
    _PyInterpreterFrame *new_frame = NULL;
    if (has_dict) {
        positionals = _PyObjectArray_FromValueArray(_PyTuple_ITEMS(callargs), nargs, scratch);
        if (positionals == NULL) {
            goto fail_func;
        }
        object_array = _PyStack_UnpackDict(tstate, positionals, nargs, kwargs, &kwnames);
        if (object_array == NULL) {
            goto fail_func;
        }
        total += PyTuple_GET_SIZE(kwnames);
    }
    if (total <= (Py_ssize_t)Py_ARRAY_LENGTH(stack_array)) {
        newargs = stack_array;
    }
    else {
        newargs = PyMem_Malloc(sizeof(PyValue) * total);
        if (newargs == NULL) {
            PyErr_NoMemory();
            goto fail_func;
        }
        if (PyErr_Occurred()) {
            goto fail_func;
        }
    }
    /* Object views stay owned until the complete new frame is published.
     * Never overlay object pointers with value bits in the same array. */
    for (Py_ssize_t i = 0; i < total; i++) {
        PyValue borrowed = has_dict ? PyValue_FromObjectBorrow(object_array[i])
                                    : PyTuple_GET_VALUE(callargs, i);
        newargs[i] = PyValue_NewRef(borrowed);
    }
    new_frame = eval_frame_push_and_init_code(
        tstate, func, locals,
        newargs, nargs, kwnames, previous, code
    );
    goto done;
fail_func:
    ;
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_CLOSE(func);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_XDECREF(locals);
    _PyValue_RestoreException(saved);
done:
    saved = PyErr_GetRaisedException();
    if (object_array != NULL) {
        _PyStack_UnpackDict_Free(object_array, nargs, kwnames);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    if (positionals != NULL) {
        _PyObjectArray_Free(positionals - 1, scratch, nargs);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    if (newargs != NULL && newargs != stack_array) {
        PyMem_Free(newargs);
        _PyValue_RestoreException(saved);
        saved = PyErr_GetRaisedException();
    }
    Py_DECREF(callargs);
    _PyValue_RestoreException(saved);
    saved = PyErr_GetRaisedException();
    Py_XDECREF(kwargs);
    _PyValue_RestoreException(saved);
    if (new_frame != NULL && PyErr_Occurred()) {
        saved = PyErr_GetRaisedException();
        _PyEval_FrameClearAndPop(tstate, new_frame);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return new_frame;
}

_PyInterpreterFrame *
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs,
    _PyInterpreterFrame *previous)
{
    func = PyValue_MakeHeapSafe(func);
    PyFunctionObject *function = (PyFunctionObject *)PyValue_AsPyObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(function->func_code);
    _PyInterpreterFrame *frame = eval_frame_push_and_init_ex_code(
        tstate, func, locals, nargs, callargs, kwargs, previous, code);
    PyObject *saved = PyErr_GetRaisedException();
    Py_DECREF(code);
    _PyValue_RestoreException(saved);
    return frame;
}

/* Parent inputs are never offered to a consuming binder. Copies can fail
 * and retire while the parent's actual IN_CALL homes remain owning. */
static int
call_transaction_push_python(_PyCallTransaction *call, _PyInstr **next,
                             _PyInterpreterFrame **out)
{
    PyValue small[16];
    PyValue *storage = small;
    int copied = 0, retired_count = 0;
    int invoked = 0;
    PyValue function = PyValue_NULL;
    PyObject *locals = NULL, *names = NULL, *tuple = NULL, *kwargs = NULL;
    PyCodeObject *code = NULL;
    _PyInterpreterFrame *child = NULL;
    _PyValueOperation *op = &call->operation;
    call->effects = 1;
    if (call->count > (int)Py_ARRAY_LENGTH(small) / 2) {
        storage = PyMem_Malloc((size_t)call->count * 2 * sizeof(PyValue));
        if (storage == NULL && !PyErr_Occurred()) {
            PyErr_NoMemory();
        }
        if (storage == NULL || call_transaction_refresh(call, next) < 0) {
            goto done;
        }
    }
    PyValue *homes = _PyFrame_Slots(op->frame) + call->base;
    PyObject *callable = PyValue_AsPyObjectBorrow(homes[0]);
    /* Allocation above may have changed vectorcall. Choose a new adapter
     * inside the same record; never replay method/monitor/EX preparation. */
    if (Py_TYPE(callable) != &PyFunction_Type || IS_PEP523_HOOKED(op->tstate) ||
        ((PyFunctionObject *)callable)->vectorcall != _PyFunction_Vectorcall) {
        goto done;
    }
    function = PyValue_NewRef(homes[0]);
    code = (PyCodeObject *)Py_NewRef(PyFunction_GET_CODE(callable));
    locals = code->co_flags & CO_OPTIMIZED ? NULL
        : Py_NewRef(PyFunction_GET_GLOBALS(callable));
    PyValue consumed_function = function;
    function = PyValue_NULL;
    PyObject *consumed_locals = locals;
    locals = NULL;
    invoked = 1;
    if (call->kind == CALL_TRANSACTION_EX) {
        tuple = Py_NewRef(PyValue_AsPyObjectBorrow(homes[2]));
        kwargs = Py_XNewRef(PyValue_AsPyObjectBorrow(homes[3]));
        PyObject *consumed_tuple = tuple, *consumed_kwargs = kwargs;
        tuple = kwargs = NULL;
        child = eval_frame_push_and_init_ex_code(
            op->tstate, consumed_function, consumed_locals,
            PyTuple_GET_SIZE(consumed_tuple), consumed_tuple, consumed_kwargs,
            op->frame, code);
    }
    else {
        int offset = PyValue_IsNull(homes[1]) ? 2 : 1;
        int total = call->argument_count + 2 - offset;
        if (call->kind == CALL_TRANSACTION_KEYWORDS) {
            names = Py_NewRef(PyValue_AsPyObjectBorrow(homes[call->count - 1]));
        }
        for (int i = 0; i < total; i++) {
            storage[i] = PyValue_NewRef(homes[offset + i]);
            copied++;
        }
        int positional = total - (names == NULL ? 0 : (int)PyTuple_GET_SIZE(names));
        /* Even on failure all these bits are consumed by the provider. */
        copied = 0;
        child = eval_frame_push_and_init_code(
            op->tstate, consumed_function, consumed_locals, storage,
            positional, names, op->frame, code);
    }
    if (call_transaction_refresh(call, next) < 0 || child == NULL) {
        goto done;
    }
    /* The code-specific provider initializes at numeric PC 0. Binding may
     * already have reinstrumented its code: never subtract the returned
     * child's old instruction pointer from a current executable. */
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec->nwords == 0) {
        PyErr_SetString(PyExc_SystemError, "Python call lost its executable");
        goto done;
    }
    child->ir_words = exec->words;
    child->ir_feedback = exec->entries;
    child->instr_ptr = exec->words;
    PyValue *retired = storage + call->count;
    call_transaction_detach(call, retired);
    retired_count = call->count;
    op->frame->return_slot = (uint16_t)call->destination;
    assert(child->previous == op->frame);
    op->tstate->current_frame = child;
done:
    call_transaction_close_value(&function);
    call_transaction_close_object(&locals);
    call_transaction_close_object(&names);
    call_transaction_close_object(&tuple);
    call_transaction_close_object(&kwargs);
    PyObject *code_owner = (PyObject *)code;
    code = NULL;
    call_transaction_close_object(&code_owner);
    for (int i = copied; i-- > 0;) {
        call_transaction_close_value(&storage[i]);
    }
    for (int i = retired_count; i-- > 0;) {
        call_transaction_close_value(&storage[call->count + i]);
    }
    if (storage != small && storage != NULL) {
        PyObject *saved = PyErr_GetRaisedException();
        PyMem_Free(storage);
        _PyValue_RestoreException(saved);
    }
    if (child != NULL && retired_count != 0 && !PyErr_Occurred()) {
        _PyCallTransaction child_call = {.operation = {
            op->tstate, child, _PyFrame_GetCode(child), 0, 0, {-1, -1}, -1}};
        _PyInstr *entry;
        (void)call_transaction_refresh(&child_call, &entry);
    }
    if (child != NULL && (retired_count == 0 || PyErr_Occurred())) {
        /* The child has not entered its recursion/monitor protocol. Restore
         * the actual parent before clearing it; do not unwind it as executed. */
        PyObject *saved = PyErr_GetRaisedException();
        op->tstate->current_frame = op->frame;
        call_transaction_publish_pc(call);
        _PyEval_FrameClearAndPop(op->tstate, child);
        child = NULL;
        _PyValue_RestoreException(saved);
    }
    if (child == NULL) {
        if (invoked && !PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError,
                            "Python frame initialization failed without an error");
        }
        int failed = call_transaction_refresh(call, next);
        if (failed < 0) {
            return -1;
        }
    }
    *out = child;
    return child != NULL ? 0 : 1;
}

static int
call_transaction_invoke(_PyCallTransaction *call, _PyInstr **next,
                        _PyInterpreterFrame **child)
{
    *child = NULL;
    if (!call->prepared) {
        int prepared = call->kind == CALL_TRANSACTION_EX
            ? call_transaction_prepare_tuple(call, next)
            : call_transaction_expand_method(call, next);
        if (prepared < 0) {
            return -1;
        }
    }
    if (call->instrumented && !call->monitored &&
        call_transaction_monitor(call, next) < 0) {
        return -1;
    }
    PyValue callable = _PyFrame_Slots(call->operation.frame)[call->base];
    if (PyValue_IsObject(callable)) {
        PyObject *object = PyValue_AsPyObjectBorrow(callable);
        if (Py_TYPE(object) == &PyFunction_Type &&
            !IS_PEP523_HOOKED(call->operation.tstate) &&
            ((PyFunctionObject *)object)->vectorcall == _PyFunction_Vectorcall &&
            !(call->kind == CALL_TRANSACTION_EX && call->instrumented)) {
            int status = call_transaction_push_python(call, next, child);
            if (status <= 0) {
                return status;
            }
        }
    }
    return call_transaction_native(call, next);
}

PyObject *
_PyEval_Vector(PyThreadState *tstate, PyFunctionObject *func,
               PyObject *locals,
               PyObject* const* args, size_t argcount,
               PyObject *kwnames)
{
    size_t total_args = argcount;
    if (kwnames) {
        total_args += PyTuple_GET_SIZE(kwnames);
    }
    PyValue stack_array[8] = {0};
    PyValue *arguments;
    if (total_args <= 8) {
        arguments = stack_array;
    }
    else {
        arguments = PyMem_Malloc(sizeof(PyValue) * total_args);
        if (arguments == NULL) {
            return PyErr_NoMemory();
        }
    }
    /* _PyEvalFramePushAndInit consumes the references
     * to func, locals and all its arguments */
    Py_XINCREF(locals);
    for (size_t i = 0; i < argcount; i++) {
        arguments[i] = PyValue_NewRef(PyValue_FromObjectBorrow(args[i]));
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            arguments[i+argcount] = PyValue_NewRef(PyValue_FromObjectBorrow(args[i+argcount]));
        }
    }
    _PyInterpreterFrame *frame = _PyEvalFramePushAndInit(
        tstate, PyValue_FromPyObjectNew(func), locals,
        arguments, argcount, kwnames, NULL);
    if (total_args > 8) {
        PyMem_Free(arguments);
    }
    if (frame == NULL) {
        return NULL;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_VECTOR);
    return _PyEval_EvalFrame(tstate, frame, 0);
}

/* Legacy API */
PyObject *
PyEval_EvalCodeEx(PyObject *_co, PyObject *globals, PyObject *locals,
                  PyObject *const *args, int argcount,
                  PyObject *const *kws, int kwcount,
                  PyObject *const *defs, int defcount,
                  PyObject *kwdefs, PyObject *closure)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *res = NULL;
    PyObject *defaults = PyTuple_FromArray(defs, defcount);
    if (defaults == NULL) {
        return NULL;
    }
    PyObject *builtins = _PyDict_LoadBuiltinsFromGlobals(globals);
    if (builtins == NULL) {
        Py_DECREF(defaults);
        return NULL;
    }
    if (locals == NULL) {
        locals = globals;
    }
    PyObject *kwnames = NULL;
    PyObject *const *allargs;
    PyObject **newargs = NULL;
    PyFunctionObject *func = NULL;
    if (kwcount == 0) {
        allargs = args;
    }
    else {
        kwnames = PyTuple_New(kwcount);
        if (kwnames == NULL) {
            goto fail;
        }
        newargs = PyMem_Malloc(sizeof(PyObject *)*(kwcount+argcount));
        if (newargs == NULL) {
            goto fail;
        }
        for (int i = 0; i < argcount; i++) {
            newargs[i] = args[i];
        }
        for (int i = 0; i < kwcount; i++) {
            PyTuple_SET_ITEM(kwnames, i, Py_NewRef(kws[2*i]));
            newargs[argcount+i] = kws[2*i+1];
        }
        allargs = newargs;
    }
    PyFrameConstructor constr = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = ((PyCodeObject *)_co)->co_name,
        .fc_qualname = ((PyCodeObject *)_co)->co_name,
        .fc_code = _co,
        .fc_defaults = defaults,
        .fc_kwdefaults = kwdefs,
        .fc_closure = closure
    };
    func = _PyFunction_FromConstructor(&constr);
    if (func == NULL) {
        goto fail;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_LEGACY);
    res = _PyEval_Vector(tstate, func, locals,
                         allargs, argcount,
                         kwnames);
fail:
    Py_XDECREF(func);
    Py_XDECREF(kwnames);
    PyMem_Free(newargs);
    _Py_DECREF_BUILTINS(builtins);
    Py_DECREF(defaults);
    return res;
}

/* Logic for matching an exception in an except* clause (too
   complicated for inlining).
*/

int
_PyEval_ExceptionGroupMatch(_PyInterpreterFrame *frame, PyObject* exc_value,
                            PyObject *match_type, PyObject **match, PyObject **rest)
{
    if (Py_IsNone(exc_value)) {
        *match = Py_NewRef(Py_None);
        *rest = Py_NewRef(Py_None);
        return 0;
    }
    assert(PyExceptionInstance_Check(exc_value));

    if (PyErr_GivenExceptionMatches(exc_value, match_type)) {
        /* Full match of exc itself */
        bool is_eg = _PyBaseExceptionGroup_Check(exc_value);
        if (is_eg) {
            *match = Py_NewRef(exc_value);
        }
        else {
            /* naked exception - wrap it */
            PyObject *excs = PyTuple_Pack(1, exc_value);
            if (excs == NULL) {
                return -1;
            }
            PyObject *wrapped = _PyExc_CreateExceptionGroup("", excs);
            Py_DECREF(excs);
            if (wrapped == NULL) {
                return -1;
            }
            PyFrameObject *f = _PyFrame_GetFrameObject(frame);
            if (f == NULL) {
                Py_DECREF(wrapped);
                return -1;
            }

            PyObject *tb = _PyTraceBack_FromFrame(NULL, f);
            if (tb == NULL) {
                Py_DECREF(wrapped);
                return -1;
            }
            PyException_SetTraceback(wrapped, tb);
            Py_DECREF(tb);

            *match = wrapped;
        }
        *rest = Py_NewRef(Py_None);
        return 0;
    }

    /* exc_value does not match match_type.
     * Check for partial match if it's an exception group.
     */
    if (_PyBaseExceptionGroup_Check(exc_value)) {
        PyObject *pair = PyObject_CallMethod(exc_value, "split", "(O)",
                                             match_type);
        if (pair == NULL) {
            return -1;
        }

        if (!PyTuple_CheckExact(pair)) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s.split must return a tuple, not %.200s",
                         Py_TYPE(exc_value)->tp_name, Py_TYPE(pair)->tp_name);
            Py_DECREF(pair);
            return -1;
        }

        // allow tuples of length > 2 for backwards compatibility
        if (PyTuple_GET_SIZE(pair) < 2) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s.split must return a 2-tuple, "
                         "got tuple of size %zd",
                         Py_TYPE(exc_value)->tp_name, PyTuple_GET_SIZE(pair));
            Py_DECREF(pair);
            return -1;
        }

        *match = PyTuple_GetItemRef(pair, 0);
        if (*match == NULL) {
            Py_DECREF(pair);
            return -1;
        }
        *rest = PyTuple_GetItemRef(pair, 1);
        if (*rest == NULL) {
            Py_CLEAR(*match);
            Py_DECREF(pair);
            return -1;
        }
        Py_DECREF(pair);
        return 0;
    }
    /* no match */
    *match = Py_NewRef(Py_None);
    *rest = Py_NewRef(exc_value);
    return 0;
}

/* Iterate v argcnt times and store the results on the stack (via decreasing
   sp).  Return 1 for success, 0 if error.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target.
*/

int
_PyEval_UnpackIterableValue(PyThreadState *tstate, PyObject *v,
                       int argcnt, int argcntafter, PyValue *sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it;  /* iter(v) */
    PyObject *w;
    PyObject *l = NULL; /* variable list */
    assert(v != NULL);

    it = PyObject_GetIter(v);
    if (it == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
            Py_TYPE(v)->tp_iter == NULL && !PySequence_Check(v))
        {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "cannot unpack non-iterable %.200s object",
                          Py_TYPE(v)->tp_name);
        }
        return 0;
    }

    for (; i < argcnt; i++) {
        w = PyIter_Next(it);
        if (w == NULL) {
            /* Iterator done, via error or exhaustion. */
            if (!_PyErr_Occurred(tstate)) {
                if (argcntafter == -1) {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "not enough values to unpack "
                                  "(expected %d, got %d)",
                                  argcnt, i);
                }
                else {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "not enough values to unpack "
                                  "(expected at least %d, got %d)",
                                  argcnt + argcntafter, i);
                }
            }
            goto Error;
        }
        *--sp = PyValue_FromPyObjectSteal(w);
    }

    if (argcntafter == -1) {
        /* We better have exhausted the iterator now. */
        w = PyIter_Next(it);
        if (w == NULL) {
            if (_PyErr_Occurred(tstate))
                goto Error;
            Py_DECREF(it);
            return 1;
        }
        Py_DECREF(w);

        if (PyList_CheckExact(v) || PyTuple_CheckExact(v)
              || PyDict_CheckExact(v)) {
            ll = PyDict_CheckExact(v) ? PyDict_Size(v) : Py_SIZE(v);
            if (ll > argcnt) {
                _PyErr_Format(tstate, PyExc_ValueError,
                              "too many values to unpack (expected %d, got %zd)",
                              argcnt, ll);
                goto Error;
            }
        }
        _PyErr_Format(tstate, PyExc_ValueError,
                      "too many values to unpack (expected %d)",
                      argcnt);
        goto Error;
    }

    l = PySequence_List(it);
    if (l == NULL)
        goto Error;
    *--sp = PyValue_FromPyObjectSteal(l);
    i++;

    ll = PyList_GET_SIZE(l);
    if (ll < argcntafter) {
        _PyErr_Format(tstate, PyExc_ValueError,
            "not enough values to unpack (expected at least %d, got %zd)",
            argcnt + argcntafter, argcnt + ll);
        goto Error;
    }

    /* Pop the "after-variable" args off the list. */
    for (j = argcntafter; j > 0; j--, i++) {
        *--sp = _PyList_ITEMS(l)[ll - j];
        _PyList_ITEMS(l)[ll - j] = PyValue_NULL;
    }
    /* Resize the list. */
    Py_SET_SIZE(l, ll - argcntafter);
    Py_DECREF(it);
    return 1;

Error:
    /* The slots that the values went to are temporaries, which hold NULL
       unless live */
    for (; i > 0; i--, sp++) {
        PyValue ref = *sp;
        *sp = PyValue_NULL;
        PyValue_CLOSE(ref);
    }
    Py_XDECREF(it);
    return 0;
}



void
_PyEval_MonitorRaise(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _PyInstr *instr)
{
    if (no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_RAISE)) {
        return;
    }
    do_monitor_exc(tstate, frame, instr, PY_MONITORING_EVENT_RAISE);
}

bool
_PyEval_NoToolsForUnwind(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    return no_tools_for_local_event(tstate, frame, PY_MONITORING_EVENT_PY_UNWIND);
}


void
PyThreadState_EnterTracing(PyThreadState *tstate)
{
    assert(tstate->tracing >= 0);
    tstate->tracing++;
}

void
PyThreadState_LeaveTracing(PyThreadState *tstate)
{
    assert(tstate->tracing > 0);
    tstate->tracing--;
}


PyObject*
_PyEval_CallTracing(PyObject *func, PyObject *args)
{
    // Save and disable tracing
    PyThreadState *tstate = _PyThreadState_GET();
    int save_tracing = tstate->tracing;
    tstate->tracing = 0;

    // Call the tracing function
    PyObject *result = PyObject_Call(func, args, NULL);

    // Restore tracing
    tstate->tracing = save_tracing;
    return result;
}

void
PyEval_SetProfile(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyEval_SetProfile(tstate, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        PyErr_FormatUnraisable("Exception ignored in PyEval_SetProfile");
    }
}

void
PyEval_SetProfileAllThreads(Py_tracefunc func, PyObject *arg)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_PyEval_SetProfileAllThreads(interp, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        PyErr_FormatUnraisable("Exception ignored in PyEval_SetProfileAllThreads");
    }
}

void
PyEval_SetTrace(Py_tracefunc func, PyObject *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyEval_SetTrace(tstate, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        PyErr_FormatUnraisable("Exception ignored in PyEval_SetTrace");
    }
}

void
PyEval_SetTraceAllThreads(Py_tracefunc func, PyObject *arg)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_PyEval_SetTraceAllThreads(interp, func, arg) < 0) {
        /* Log _PySys_Audit() error */
        PyErr_FormatUnraisable("Exception ignored in PyEval_SetTraceAllThreads");
    }
}

int
_PyEval_SetCoroutineOriginTrackingDepth(int depth)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (depth < 0) {
        _PyErr_SetString(tstate, PyExc_ValueError, "depth must be >= 0");
        return -1;
    }
    tstate->coroutine_origin_tracking_depth = depth;
    return 0;
}


int
_PyEval_GetCoroutineOriginTrackingDepth(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->coroutine_origin_tracking_depth;
}

int
_PyEval_SetAsyncGenFirstiter(PyObject *firstiter)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_PySys_Audit(tstate, "sys.set_asyncgen_hook_firstiter", NULL) < 0) {
        return -1;
    }

    Py_XSETREF(tstate->async_gen_firstiter, Py_XNewRef(firstiter));
    return 0;
}

PyObject *
_PyEval_GetAsyncGenFirstiter(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->async_gen_firstiter;
}

int
_PyEval_SetAsyncGenFinalizer(PyObject *finalizer)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (_PySys_Audit(tstate, "sys.set_asyncgen_hook_finalizer", NULL) < 0) {
        return -1;
    }

    Py_XSETREF(tstate->async_gen_finalizer, Py_XNewRef(finalizer));
    return 0;
}

PyObject *
_PyEval_GetAsyncGenFinalizer(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->async_gen_finalizer;
}

_PyInterpreterFrame *
_PyEval_GetFrame(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyThreadState_GetFrame(tstate);
}

PyFrameObject *
PyEval_GetFrame(void)
{
    _PyInterpreterFrame *frame = _PyEval_GetFrame();
    if (frame == NULL) {
        return NULL;
    }
    PyFrameObject *f = _PyFrame_GetFrameObject(frame);
    if (f == NULL) {
        PyErr_Clear();
    }
    return f;
}

PyObject *
_PyEval_GetBuiltins(PyThreadState *tstate)
{
    _PyInterpreterFrame *frame = _PyThreadState_GetFrame(tstate);
    if (frame != NULL) {
        return frame->f_builtins;
    }
    return tstate->interp->builtins;
}

PyObject *
PyEval_GetBuiltins(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_GetBuiltins(tstate);
}

/* Convenience function to get a builtin from its name */
PyObject *
_PyEval_GetBuiltin(PyObject *name)
{
    PyObject *attr;
    if (PyMapping_GetOptionalItem(PyEval_GetBuiltins(), name, &attr) == 0) {
        PyErr_SetObject(PyExc_AttributeError, name);
    }
    return attr;
}

PyObject *
PyEval_GetLocals(void)
{
    // We need to return a borrowed reference here, so some tricks are needed
    PyThreadState *tstate = _PyThreadState_GET();
     _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    // Be aware that this returns a new reference
    PyObject *locals = _PyFrame_GetLocals(current_frame);

    if (locals == NULL) {
        return NULL;
    }

    if (PyFrameLocalsProxy_Check(locals)) {
        PyFrameObject *f = _PyFrame_GetFrameObject(current_frame);
        if (f == NULL) {
            Py_DECREF(locals);
            return NULL;
        }

        PyObject *ret = f->f_locals_cache;
        if (ret == NULL) {
            ret = PyDict_New();
            if (ret == NULL) {
                Py_DECREF(locals);
                return NULL;
            }
            f->f_locals_cache = ret;
        }
        if (PyDict_Update(ret, locals) < 0) {
            // At this point, if the cache dict is broken, it will stay broken, as
            // trying to clean it up or replace it will just cause other problems
            ret = NULL;
        }
        Py_DECREF(locals);
        return ret;
    }

    assert(PyMapping_Check(locals));
    assert(Py_REFCNT(locals) > 1);
    Py_DECREF(locals);

    return locals;
}

PyObject *
_PyEval_GetFrameLocals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
     _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "frame does not exist");
        return NULL;
    }

    PyObject *locals = _PyFrame_GetLocals(current_frame);
    if (locals == NULL) {
        return NULL;
    }

    if (PyFrameLocalsProxy_Check(locals)) {
        PyObject* ret = PyDict_New();
        if (ret == NULL) {
            Py_DECREF(locals);
            return NULL;
        }
        if (PyDict_Update(ret, locals) < 0) {
            Py_DECREF(ret);
            Py_DECREF(locals);
            return NULL;
        }
        Py_DECREF(locals);
        return ret;
    }

    assert(PyMapping_Check(locals));
    return locals;
}

static PyObject *
_PyEval_GetGlobals(PyThreadState *tstate)
{
    _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        return NULL;
    }
    return current_frame->f_globals;
}

PyObject *
PyEval_GetGlobals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyEval_GetGlobals(tstate);
}

PyObject *
_PyEval_GetGlobalsFromRunningMain(PyThreadState *tstate)
{
    if (!_PyInterpreterState_IsRunningMain(tstate->interp)) {
        return NULL;
    }
    PyObject *mod = _Py_GetMainModule(tstate);
    if (_Py_CheckMainModule(mod) < 0) {
        Py_XDECREF(mod);
        return NULL;
    }
    PyObject *globals = PyModule_GetDict(mod);  // borrowed
    Py_DECREF(mod);
    return globals;
}

static PyObject *
get_globals_builtins(PyObject *globals)
{
    PyObject *builtins = NULL;
    if (PyAnyDict_Check(globals)) {
        if (PyDict_GetItemRef(globals, &_Py_ID(__builtins__), &builtins) < 0) {
            return NULL;
        }
    }
    else {
        if (PyMapping_GetOptionalItem(
                        globals, &_Py_ID(__builtins__), &builtins) < 0)
        {
            return NULL;
        }
    }
    return builtins;
}

static int
set_globals_builtins(PyObject *globals, PyObject *builtins)
{
    if (PyDict_Check(globals)) {
        if (PyDict_SetItem(globals, &_Py_ID(__builtins__), builtins) < 0) {
            return -1;
        }
    }
    else {
        if (PyObject_SetItem(globals, &_Py_ID(__builtins__), builtins) < 0) {
            if (PyFrozenDict_Check(globals)) {
                PyErr_SetString(PyExc_TypeError,
                                "cannot assign __builtins__ to frozendict globals");
            }
            return -1;
        }
    }
    return 0;
}

int
_PyEval_EnsureBuiltins(PyThreadState *tstate, PyObject *globals,
                       PyObject **p_builtins)
{
    PyObject *builtins = get_globals_builtins(globals);
    if (builtins == NULL) {
        if (_PyErr_Occurred(tstate)) {
            return -1;
        }
        builtins = PyEval_GetBuiltins();  // borrowed
        if (builtins == NULL) {
            assert(_PyErr_Occurred(tstate));
            return -1;
        }
        Py_INCREF(builtins);
        if (set_globals_builtins(globals, builtins) < 0) {
            Py_DECREF(builtins);
            return -1;
        }
    }
    if (p_builtins != NULL) {
        *p_builtins = builtins;
    }
    else {
        Py_DECREF(builtins);
    }
    return 0;
}

int
_PyEval_EnsureBuiltinsWithModule(PyThreadState *tstate, PyObject *globals,
                                 PyObject **p_builtins)
{
    PyObject *builtins = get_globals_builtins(globals);
    if (builtins == NULL) {
        if (_PyErr_Occurred(tstate)) {
            return -1;
        }
        builtins = PyImport_ImportModuleLevel("builtins", NULL, NULL, NULL, 0);
        if (builtins == NULL) {
            return -1;
        }
        if (set_globals_builtins(globals, builtins) < 0) {
            Py_DECREF(builtins);
            return -1;
        }
    }
    if (p_builtins != NULL) {
        *p_builtins = builtins;
    }
    else {
        Py_DECREF(builtins);
    }
    return 0;
}

PyObject*
PyEval_GetFrameLocals(void)
{
    return _PyEval_GetFrameLocals();
}

PyObject* PyEval_GetFrameGlobals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *current_frame = _PyThreadState_GetFrame(tstate);
    if (current_frame == NULL) {
        return NULL;
    }
    return Py_XNewRef(current_frame->f_globals);
}

PyObject* PyEval_GetFrameBuiltins(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return Py_XNewRef(_PyEval_GetBuiltins(tstate));
}

int
PyEval_MergeCompilerFlags(PyCompilerFlags *cf)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *current_frame = tstate->current_frame;
    if (current_frame == tstate->base_frame) {
        current_frame = NULL;
    }
    int result = cf->cf_flags != 0;

    if (current_frame != NULL) {
        const int codeflags = _PyFrame_GetCode(current_frame)->co_flags;
        const int compilerflags = codeflags & PyCF_MASK;
        if (compilerflags) {
            result = 1;
            cf->cf_flags |= compilerflags;
        }
    }
    return result;
}

static const char *
functionobject_get_name(PyFunctionObject *f)
{
#ifdef Py_GIL_DISABLED
    for (;;) {
        PyObject *name = Py_NewRef(FT_ATOMIC_LOAD_PTR_ACQUIRE(f->func_name));
        // UTF-8 conversion may detach while acquiring the string's lock.
        // This could result in func_name being freed if func_name was
        // recently replaced using _PyObject_XSetRefDelayed().  The returned
        // char* is only valid until the next quiescent point.
        const char *result = PyUnicode_AsUTF8(name);
        if (result == NULL) {
            Py_DECREF(name);
            return NULL;
        }
        int unchanged = name == FT_ATOMIC_LOAD_PTR_ACQUIRE(f->func_name);
        Py_DECREF(name);
        if (unchanged) {
            return result;
        }
    }
#else
    return PyUnicode_AsUTF8(f->func_name);
#endif
}

const char *
PyEval_GetFuncName(PyObject *func)
{
    if (PyMethod_Check(func))
        return PyEval_GetFuncName(PyMethod_GET_FUNCTION(func));
    else if (PyFunction_Check(func)) {
        return functionobject_get_name((PyFunctionObject *)func);
    }
    else if (PyCFunction_Check(func))
        return ((PyCFunctionObject*)func)->m_ml->ml_name;
    else
        return Py_TYPE(func)->tp_name;
}

const char *
PyEval_GetFuncDesc(PyObject *func)
{
    if (PyMethod_Check(func))
        return "()";
    else if (PyFunction_Check(func))
        return "()";
    else if (PyCFunction_Check(func))
        return "()";
    else
        return " object";
}

/* Extract a slice index from a PyLong or an object with the
   nb_index slot defined, and store in *pi.
   Silently reduce values larger than PY_SSIZE_T_MAX to PY_SSIZE_T_MAX,
   and silently boost values less than PY_SSIZE_T_MIN to PY_SSIZE_T_MIN.
   Return 0 on error, 1 on success.
*/
int
_PyEval_SliceIndex(PyObject *v, Py_ssize_t *pi)
{
    if (Py_IsNone(v)) {
        return 1;
    }
    return _PyEval_SliceIndexNotNone(v, pi);
}

int
_PyEval_SliceIndexNotNone(PyObject *v, Py_ssize_t *pi)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t x;
    if (PyLong_CheckExact(v) && _PyLong_IsCompact((PyLongObject *)v)) {
        *pi = _PyLong_CompactValue((PyLongObject *)v);
        return 1;
    }
    if (_PyIndex_Check(v)) {
        x = PyNumber_AsSsize_t(v, NULL);
        if (x == -1 && _PyErr_Occurred(tstate))
            return 0;
    }
    else {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "slice indices must be integers or "
                         "have an __index__ method");
        return 0;
    }
    *pi = x;
    return 1;
}

int
_PyEval_UnpackIndices(PyObject *start, PyObject *stop,
                      Py_ssize_t len,
                      Py_ssize_t *istart, Py_ssize_t *istop)
{
    if (len < 0) {
        return 0;
    }
    *istart = 0;
    *istop = PY_SSIZE_T_MAX;
    if (!_PyEval_SliceIndex(start, istart)) {
        return 0;
    }
    if (!_PyEval_SliceIndex(stop, istop)) {
        return 0;
    }
    PySlice_AdjustIndices(len, istart, istop, 1);
    return 1;
}

/* Inline values in BINARY_SLICE, STORE_SLICE and CALL_ISINSTANCE

   An int or float operand may be inline (see pycore_value.h). Bounds that
   are inline ints or None slice a list, tuple or str without boxes, and
   isinstance() of an inline value against a type needs no box. Other
   operands are boxed for the duration of the call. */

/* The bounds of a slice with no step, as PySlice_Unpack() gives them, if
   start and stop are each an inline int or None, and one is an inline int. */
static bool
inline_slice_bounds(PyValue start, PyValue stop,
                    Py_ssize_t *istart, Py_ssize_t *istop)
{
    if (!PyValue_IsInlineInt(start) && !PyValue_IsInlineInt(stop)) {
        return false;
    }
    if (PyValue_IsInlineInt(start)) {
        *istart = PyValue_InlineInt(start);
    }
    else if (PyValue_IsNone(start)) {
        *istart = 0;
    }
    else {
        return false;
    }
    if (PyValue_IsInlineInt(stop)) {
        *istop = PyValue_InlineInt(stop);
    }
    else if (PyValue_IsNone(stop)) {
        *istop = PY_SSIZE_T_MAX;
    }
    else {
        return false;
    }
    return true;
}

/* Store in *box a new box if ref holds an inline value, else NULL. Return a
   borrowed reference to the object ref holds or to the box, or NULL with an
   exception set if boxing fails. */
static PyObject *
borrow_or_box(PyValue ref, PyObject **box)
{
    if (PyValue_IsInline(ref)) {
        *box = PyValue_AsPyObjectNewBoxed(ref);
        return *box;
    }
    *box = NULL;
    return PyValue_AsPyObjectBorrow(ref);
}

static PyObject *
binary_slice(PyObject *container, PyObject *start, PyObject *stop)
{
    if (PyList_CheckExact(container)) {
        return _PyList_BinarySlice(container, start, stop);
    }
    if (PyTuple_CheckExact(container)) {
        return _PyTuple_BinarySlice(container, start, stop);
    }
    if (PyUnicode_CheckExact(container)) {
        return _PyUnicode_BinarySlice(container, start, stop);
    }
    PyObject *slice = PySlice_New(start, stop, NULL);
    if (slice == NULL) {
        return NULL;
    }
    PyObject *res = PyObject_GetItem(container, slice);
    Py_DECREF(slice);
    return res;
}

// container[start:stop], for BINARY_SLICE.
PyObject *
_PyEval_BinarySlice(PyValue container_st, PyValue start_st, PyValue stop_st)
{
    Py_ssize_t start, stop;
    if (PyValue_IsPointerSpace(container_st) &&
        inline_slice_bounds(start_st, stop_st, &start, &stop))
    {
        PyObject *container = PyValue_AsPyObjectBorrow(container_st);
        if (PyList_CheckExact(container)) {
            PySlice_AdjustIndices(PyList_GET_SIZE(container), &start, &stop, 1);
            return PyList_GetSlice(container, start, stop);
        }
        if (PyTuple_CheckExact(container)) {
            PySlice_AdjustIndices(PyTuple_GET_SIZE(container), &start, &stop, 1);
            return PyTuple_GetSlice(container, start, stop);
        }
        if (PyUnicode_CheckExact(container)) {
            PySlice_AdjustIndices(PyUnicode_GET_LENGTH(container), &start, &stop, 1);
            return PyUnicode_Substring(container, start, stop);
        }
    }
    PyObject *container_box, *start_box = NULL, *stop_box = NULL, *res = NULL;
    PyObject *container = borrow_or_box(container_st, &container_box);
    if (container == NULL) {
        goto done;
    }
    PyObject *start_o = borrow_or_box(start_st, &start_box);
    if (start_o == NULL) {
        goto done;
    }
    PyObject *stop_o = borrow_or_box(stop_st, &stop_box);
    if (stop_o == NULL) {
        goto done;
    }
    res = binary_slice(container, start_o, stop_o);
done:
    Py_XDECREF(container_box);
    Py_XDECREF(start_box);
    Py_XDECREF(stop_box);
    return res;
}

// container[start:stop] = v, for STORE_SLICE. Return 0, or -1 with an
// exception set.
int
_PyEval_StoreSlice(PyValue v_st, PyValue container_st,
                   PyValue start_st, PyValue stop_st)
{
    Py_ssize_t start, stop;
    if (PyValue_IsPointerSpace(v_st) && PyValue_IsPointerSpace(container_st) &&
        PyList_CheckExact(PyValue_AsPyObjectBorrow(container_st)) &&
        inline_slice_bounds(start_st, stop_st, &start, &stop))
    {
        return _PyList_StoreSlice(PyValue_AsPyObjectBorrow(container_st), start, stop,
                                  PyValue_AsPyObjectBorrow(v_st));
    }
    PyObject *v_box, *container_box = NULL, *start_box = NULL, *stop_box = NULL;
    int err = -1;
    PyObject *v = borrow_or_box(v_st, &v_box);
    if (v == NULL) {
        goto done;
    }
    PyObject *container = borrow_or_box(container_st, &container_box);
    if (container == NULL) {
        goto done;
    }
    PyObject *start_o = borrow_or_box(start_st, &start_box);
    if (start_o == NULL) {
        goto done;
    }
    PyObject *stop_o = borrow_or_box(stop_st, &stop_box);
    if (stop_o == NULL) {
        goto done;
    }
    PyObject *slice = PySlice_New(start_o, stop_o, NULL);
    if (slice == NULL) {
        goto done;
    }
    err = PyObject_SetItem(container, slice, v);
    Py_DECREF(slice);
done:
    Py_XDECREF(v_box);
    Py_XDECREF(container_box);
    Py_XDECREF(start_box);
    Py_XDECREF(stop_box);
    return err;
}

/* isinstance(inst, cls), for CALL_ISINSTANCE when inst or cls may be inline.
   An int or float has no __class__ of its own, so against an exact type, or
   a tuple of them, the answer is that of PyObject_IsInstance(), whose
   object_isinstance() then tests the subtype relation alone. */
int
_PyEval_IsInstanceValue(PyValue inst_st, PyValue cls_st)
{
    if (PyValue_IsInline(inst_st) && PyValue_IsPointerSpace(cls_st)) {
        PyTypeObject *type = PyValue_TYPE(inst_st);
        PyObject *cls = PyValue_AsPyObjectBorrow(cls_st);
        if (PyType_CheckExact(cls)) {
            return PyType_IsSubtype(type, (PyTypeObject *)cls);
        }
        if (PyTuple_CheckExact(cls)) {
            Py_ssize_t n = PyTuple_GET_SIZE(cls);
            Py_ssize_t i = 0;
            for (; i < n; i++) {
                PyValue item = PyTuple_GET_VALUE(cls, i);
                if (!PyValue_IsObject(item) ||
                    !PyType_CheckExact(PyValue_AsPyObjectBorrow(item))) {
                    break;
                }
                if (PyType_IsSubtype(type, (PyTypeObject *)PyValue_AsPyObjectBorrow(item))) {
                    return 1;
                }
            }
            if (i == n) {
                return 0;
            }
        }
    }
    PyObject *inst_box, *cls_box = NULL;
    int res = -1;
    PyObject *inst = borrow_or_box(inst_st, &inst_box);
    if (inst != NULL) {
        PyObject *cls = borrow_or_box(cls_st, &cls_box);
        if (cls != NULL) {
            res = PyObject_IsInstance(inst, cls);
        }
    }
    Py_XDECREF(inst_box);
    Py_XDECREF(cls_box);
    return res;
}

PyObject *
_PyEval_ImportName(PyThreadState *tstate, PyObject *builtins,
            PyObject *globals, PyObject *locals, PyObject *name,
            PyObject *fromlist, PyObject *level)
{
    PyObject *import_func;
    if (PyMapping_GetOptionalItem(builtins, &_Py_ID(__import__),
                                  &import_func) < 0) {
        return NULL;
    }
    if (import_func == NULL) {
        _PyErr_SetString(tstate, PyExc_ImportError, "__import__ not found");
        return NULL;
    }

    PyObject *res = _PyEval_ImportNameWithImport(
        tstate, import_func, globals, locals, name, fromlist, level);
    Py_DECREF(import_func);
    return res;
}

PyObject *
_PyEval_ImportNameWithImport(PyThreadState *tstate, PyObject *import_func,
                             PyObject *globals, PyObject *locals,
                             PyObject *name, PyObject *fromlist, PyObject *level)
{
    if (locals == NULL) {
        locals = Py_None;
    }

    /* Fast path for not overloaded __import__. */
    if (_PyImport_IsDefaultImportFunc(tstate->interp, import_func)) {
        int ilevel = PyLong_AsInt(level);
        if (ilevel == -1 && _PyErr_Occurred(tstate)) {
            return NULL;
        }
        return PyImport_ImportModuleLevelObject(
                        name,
                        globals,
                        locals,
                        fromlist,
                        ilevel);
    }

    PyObject *args[5] = {name, globals, locals, fromlist, level};
    PyObject *res = PyObject_Vectorcall(import_func, args, 5, NULL);
    return res;
}

static int
check_lazy_import_compatibility(PyThreadState *tstate, PyObject *globals,
                               PyObject *name, PyObject *level)
{
     // Check if this module should be imported lazily due to
     // the compatibility mode support via __lazy_modules__.
    PyObject *lazy_modules = NULL;
    PyObject *abs_name = NULL;
    int res = -1;

    if (globals != NULL &&
        PyMapping_GetOptionalItem(globals, &_Py_ID(__lazy_modules__),
                                  &lazy_modules) < 0)
    {
        return -1;
    }
    if (lazy_modules == NULL) {
        assert(!PyErr_Occurred());
        return 0;
    }

    int ilevel = PyLong_AsInt(level);
    if (ilevel == -1 && _PyErr_Occurred(tstate)) {
        goto error;
    }

    abs_name = _PyImport_GetAbsName(tstate, name, globals, ilevel);
    if (abs_name == NULL) {
        goto error;
    }

    res = PySequence_Contains(lazy_modules, abs_name);
error:
    Py_XDECREF(abs_name);
    Py_XDECREF(lazy_modules);
    return res;
}

static int
is_lazy_import_module_level(void)
{
    _PyInterpreterFrame *frame = _PyEval_GetFrame();
    return frame != NULL && frame->f_globals == frame->f_locals;
}

PyObject *
_PyEval_LazyImportName(PyThreadState *tstate, PyObject *builtins,
                       PyObject *globals, PyObject *locals, PyObject *name,
                       PyObject *fromlist, PyObject *level, int lazy)
{
    PyObject *res = NULL;
    // Check if global policy overrides the local syntax
    switch (PyImport_GetLazyImportsMode()) {
        case PyImport_LAZY_ALL:
            if (!lazy) {
                lazy = is_lazy_import_module_level();
            }
            break;
        case PyImport_LAZY_NORMAL:
            break;
    }

    if (!lazy) {
        // See if __lazy_modules__ forces this to be lazy.
        // __lazy_modules__ only applies at module level; exec() inside
        // functions or classes should remain eager.
        if (is_lazy_import_module_level()) {
            lazy = check_lazy_import_compatibility(tstate, globals, name, level);
            if (lazy < 0) {
                return NULL;
            }
        }
    }

    if (!lazy) {
        // Not a lazy import or lazy imports are disabled, fallback to the
        // regular import.
        return _PyEval_ImportName(tstate, builtins, globals, locals,
                                  name, fromlist, level);
    }

    PyObject *lazy_import_func;
    if (PyMapping_GetOptionalItem(builtins, &_Py_ID(__lazy_import__),
                                  &lazy_import_func) < 0) {
        goto error;
    }
    if (lazy_import_func == NULL) {
        assert(!PyErr_Occurred());
        _PyErr_SetString(tstate, PyExc_ImportError,
                         "__lazy_import__ not found");
        goto error;
    }

    if (locals == NULL) {
        locals = Py_None;
    }

    if (_PyImport_IsDefaultLazyImportFunc(tstate->interp, lazy_import_func)) {
        int ilevel = PyLong_AsInt(level);
        if (ilevel == -1 && PyErr_Occurred()) {
            goto error;
        }

        res = _PyImport_LazyImportModuleLevelObject(
            tstate, name, builtins, globals, locals, fromlist, ilevel
        );
        goto error;
    }

    PyObject *args[5] = {name, globals, locals, fromlist, level};
    res = PyObject_Vectorcall(lazy_import_func, args, 5, NULL);
error:
    Py_XDECREF(lazy_import_func);
    return res;
}

PyObject *
_PyEval_ImportFrom(PyThreadState *tstate, PyObject *v, PyObject *name)
{
    PyObject *x;
    PyObject *fullmodname, *mod_name, *origin, *mod_name_or_unknown, *errmsg, *spec;

    if (PyObject_GetOptionalAttr(v, name, &x) != 0) {
        return x;
    }
    /* Issue #17636: in case this failed because of a circular relative
       import, try to fallback on reading the module directly from
       sys.modules. */
    if (PyObject_GetOptionalAttr(v, &_Py_ID(__name__), &mod_name) < 0) {
        return NULL;
    }
    if (mod_name == NULL || !PyUnicode_Check(mod_name)) {
        Py_CLEAR(mod_name);
        goto error;
    }
    fullmodname = PyUnicode_FromFormat("%U.%U", mod_name, name);
    if (fullmodname == NULL) {
        Py_DECREF(mod_name);
        return NULL;
    }
    x = PyImport_GetModule(fullmodname);
    Py_DECREF(fullmodname);
    if (x == NULL && !_PyErr_Occurred(tstate)) {
        goto error;
    }
    Py_DECREF(mod_name);
    return x;

 error:
    if (mod_name == NULL) {
        mod_name_or_unknown = PyUnicode_FromString("<unknown module name>");
        if (mod_name_or_unknown == NULL) {
            return NULL;
        }
    } else {
        mod_name_or_unknown = mod_name;
    }
    // mod_name is no longer an owned reference
    assert(mod_name_or_unknown);
    assert(mod_name == NULL || mod_name == mod_name_or_unknown);

    origin = NULL;
    if (PyObject_GetOptionalAttr(v, &_Py_ID(__spec__), &spec) < 0) {
        Py_DECREF(mod_name_or_unknown);
        return NULL;
    }
    if (spec == NULL) {
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R (unknown location)",
            name, mod_name_or_unknown
        );
        goto done_with_errmsg;
    }
    if (_PyModuleSpec_GetFileOrigin(spec, &origin) < 0) {
        goto done;
    }

    int is_possibly_shadowing = _PyModule_IsPossiblyShadowing(origin);
    if (is_possibly_shadowing < 0) {
        goto done;
    }
    int is_possibly_shadowing_stdlib = 0;
    if (is_possibly_shadowing) {
        PyObject *stdlib_modules;
        if (PySys_GetOptionalAttrString("stdlib_module_names", &stdlib_modules) < 0) {
            goto done;
        }
        if (stdlib_modules && PyAnySet_Check(stdlib_modules)) {
            is_possibly_shadowing_stdlib = PySet_Contains(stdlib_modules, mod_name_or_unknown);
            if (is_possibly_shadowing_stdlib < 0) {
                Py_DECREF(stdlib_modules);
                goto done;
            }
        }
        Py_XDECREF(stdlib_modules);
    }

    if (origin == NULL && PyModule_Check(v)) {
        // Fall back to __file__ for diagnostics if we don't have
        // an origin that is a location
        origin = PyModule_GetFilenameObject(v);
        if (origin == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_SystemError)) {
                goto done;
            }
            // PyModule_GetFilenameObject raised "module filename missing"
            _PyErr_Clear(tstate);
        }
        assert(origin == NULL || PyUnicode_Check(origin));
    }

    if (is_possibly_shadowing_stdlib) {
        assert(origin);
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R "
            "(consider renaming %R since it has the same "
            "name as the standard library module named %R "
            "and prevents importing that standard library module)",
            name, mod_name_or_unknown, origin, mod_name_or_unknown
        );
    }
    else {
        int rc = _PyModuleSpec_IsInitializing(spec);
        if (rc < 0) {
            goto done;
        }
        else if (rc > 0) {
            if (is_possibly_shadowing) {
                assert(origin);
                // For non-stdlib modules, only mention the possibility of
                // shadowing if the module is being initialized.
                errmsg = PyUnicode_FromFormat(
                    "cannot import name %R from %R "
                    "(consider renaming %R if it has the same name "
                    "as a library you intended to import)",
                    name, mod_name_or_unknown, origin
                );
            }
            else if (origin) {
                errmsg = PyUnicode_FromFormat(
                    "cannot import name %R from partially initialized module %R "
                    "(most likely due to a circular import) (%S)",
                    name, mod_name_or_unknown, origin
                );
            }
            else {
                errmsg = PyUnicode_FromFormat(
                    "cannot import name %R from partially initialized module %R "
                    "(most likely due to a circular import)",
                    name, mod_name_or_unknown
                );
            }
        }
        else {
            assert(rc == 0);
            if (origin) {
                errmsg = PyUnicode_FromFormat(
                    "cannot import name %R from %R (%S)",
                    name, mod_name_or_unknown, origin
                );
            }
            else {
                errmsg = PyUnicode_FromFormat(
                    "cannot import name %R from %R (unknown location)",
                    name, mod_name_or_unknown
                );
            }
        }
    }

done_with_errmsg:
    if (errmsg != NULL) {
        /* NULL checks for mod_name and origin done by _PyErr_SetImportErrorWithNameFrom */
        _PyErr_SetImportErrorWithNameFrom(errmsg, mod_name, origin, name);
        Py_DECREF(errmsg);
    }

done:
    Py_XDECREF(origin);
    Py_XDECREF(spec);
    Py_DECREF(mod_name_or_unknown);
    return NULL;
}

#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

#define CANNOT_EXCEPT_STAR_EG "catching ExceptionGroup with except* "\
                              "is not allowed. Use except instead."

int
_PyEval_CheckExceptTypeValid(PyThreadState *tstate, PyObject* right)
{
    if (PyTuple_Check(right)) {
        Py_ssize_t i, length;
        length = PyTuple_GET_SIZE(right);
        for (i = 0; i < length; i++) {
            PyValue exc = PyTuple_GET_VALUE(right, i);
            if (!PyValue_IsObject(exc) ||
                !PyExceptionClass_Check(PyValue_AsPyObjectBorrow(exc))) {
                _PyErr_SetString(tstate, PyExc_TypeError,
                    CANNOT_CATCH_MSG);
                return -1;
            }
        }
    }
    else {
        if (!PyExceptionClass_Check(right)) {
            _PyErr_SetString(tstate, PyExc_TypeError,
                CANNOT_CATCH_MSG);
            return -1;
        }
    }
    return 0;
}

int
_PyEval_CheckExceptStarTypeValid(PyThreadState *tstate, PyObject* right)
{
    if (_PyEval_CheckExceptTypeValid(tstate, right) < 0) {
        return -1;
    }

    /* reject except *ExceptionGroup */

    int is_subclass = 0;
    if (PyTuple_Check(right)) {
        Py_ssize_t length = PyTuple_GET_SIZE(right);
        for (Py_ssize_t i = 0; i < length; i++) {
            /* CheckExceptTypeValid has already proved object/class entries. */
            PyObject *exc = _PyTuple_GET_OBJECT(right, i);
            is_subclass = PyObject_IsSubclass(exc, PyExc_BaseExceptionGroup);
            if (is_subclass < 0) {
                return -1;
            }
            if (is_subclass) {
                break;
            }
        }
    }
    else {
        is_subclass = PyObject_IsSubclass(right, PyExc_BaseExceptionGroup);
        if (is_subclass < 0) {
            return -1;
        }
    }
    if (is_subclass) {
        _PyErr_SetString(tstate, PyExc_TypeError,
            CANNOT_EXCEPT_STAR_EG);
            return -1;
    }
    return 0;
}

int
_Py_Check_ArgsIterable(PyThreadState *tstate, PyObject *func, PyObject *args)
{
    if (Py_TYPE(args)->tp_iter == NULL && !PySequence_Check(args)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "Value after * must be an iterable, not %.200s",
                      Py_TYPE(args)->tp_name);
        return -1;
    }
    return 0;
}

void
_PyEval_FormatKwargsError(PyThreadState *tstate, PyObject *func, PyObject *kwargs, PyObject *dupkey)
{
    if (dupkey != NULL) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        _PyErr_Format(
            tstate, PyExc_TypeError,
            "%V got multiple values for keyword argument '%S'",
            funcstr, "function", dupkey);
        Py_XDECREF(funcstr);
        return;
    }
    /* _PyDict_MergeUniq raises attribute
     * error (percolated from an attempt
     * to get 'keys' attribute) instead of
     * a type error if its second argument
     * is not a mapping.
     */
    if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        int has_keys = PyObject_HasAttrWithError(kwargs, &_Py_ID(keys));
        if (has_keys == 0) {
            _PyErr_Format(
                tstate, PyExc_TypeError,
                "Value after ** must be a mapping, not %T",
                kwargs);
            Py_DECREF(exc);
        }
        else {
            _PyErr_ChainExceptions1Tstate(tstate, exc);
        }
    }
}

void
_PyEval_FormatExcCheckArg(PyThreadState *tstate, PyObject *exc,
                          const char *format_str, PyObject *obj)
{
    const char *obj_str;

    if (!obj)
        return;

    obj_str = PyUnicode_AsUTF8(obj);
    if (!obj_str)
        return;

    _PyErr_Format(tstate, exc, format_str, obj_str);

    if (exc == PyExc_NameError) {
        // Include the name in the NameError exceptions to offer suggestions later.
        PyObject *exc = PyErr_GetRaisedException();
        if (PyErr_GivenExceptionMatches(exc, PyExc_NameError)) {
            if (((PyNameErrorObject*)exc)->name == NULL) {
                // We do not care if this fails because we are going to restore the
                // NameError anyway.
                (void)PyObject_SetAttr(exc, &_Py_ID(name), obj);
            }
        }
        PyErr_SetRaisedException(exc);
    }
}

void
_PyEval_FormatExcUnbound(PyThreadState *tstate, PyCodeObject *co, int oparg)
{
    PyObject *name;
    /* Don't stomp existing exception */
    if (_PyErr_Occurred(tstate))
        return;
    name = _PyTuple_GET_OBJECT(co->co_localsplusnames, oparg);
    if (oparg < PyUnstable_Code_GetFirstFree(co)) {
        _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
                                  UNBOUNDLOCAL_ERROR_MSG, name);
    } else {
        _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                  UNBOUNDFREE_ERROR_MSG, name);
    }
}

void
_PyEval_FormatAwaitableError(PyThreadState *tstate, PyTypeObject *type, int oparg)
{
    if (type->tp_as_async == NULL || type->tp_as_async->am_await == NULL) {
        if (oparg == 1) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'async with' received an object from __aenter__ "
                          "that does not implement __await__: %.100s",
                          type->tp_name);
        }
        else if (oparg == 2) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'async with' received an object from __aexit__ "
                          "that does not implement __await__: %.100s",
                          type->tp_name);
        }
    }
}


Py_ssize_t
PyUnstable_Eval_RequestCodeExtraIndex(freefunc free)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    Py_ssize_t new_index;

#ifdef Py_GIL_DISABLED
    struct _py_code_state *state = &interp->code_state;
    FT_MUTEX_LOCK(&state->mutex);
#endif

    if (interp->co_extra_user_count >= MAX_CO_EXTRA_USERS - 1) {
#ifdef Py_GIL_DISABLED
        FT_MUTEX_UNLOCK(&state->mutex);
#endif
        return -1;
    }

    new_index = interp->co_extra_user_count;
    interp->co_extra_freefuncs[new_index] = free;

    // Publish freefuncs[new_index] before making the index visible.
    FT_ATOMIC_STORE_SSIZE_RELEASE(interp->co_extra_user_count, new_index + 1);

#ifdef Py_GIL_DISABLED
    FT_MUTEX_UNLOCK(&state->mutex);
#endif
    return new_index;
}

/* Implement Py_EnterRecursiveCall() and Py_LeaveRecursiveCall() as functions
   for the limited API. */

int Py_EnterRecursiveCall(const char *where)
{
    return _Py_EnterRecursiveCall(where);
}

void Py_LeaveRecursiveCall(void)
{
    _Py_LeaveRecursiveCall();
}

PyObject *
_PyEval_GetANext(PyObject *aiter)
{
    unaryfunc getter = NULL;
    PyObject *next_iter = NULL;
    PyTypeObject *type = Py_TYPE(aiter);
    if (PyAsyncGen_CheckExact(aiter)) {
        return type->tp_as_async->am_anext(aiter);
    }
    if (type->tp_as_async != NULL){
        getter = type->tp_as_async->am_anext;
    }

    if (getter != NULL) {
        next_iter = (*getter)(aiter);
        if (next_iter == NULL) {
            return NULL;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                        "'async for' requires an iterator with "
                        "__anext__ method, got %.100s",
                        type->tp_name);
        return NULL;
    }

    PyObject *awaitable = _PyCoro_GetAwaitableIter(next_iter);
    if (awaitable == NULL) {
        _PyErr_FormatFromCause(
            PyExc_TypeError,
            "'async for' received an invalid object "
            "from __anext__: %.100s",
            Py_TYPE(next_iter)->tp_name);
    }
    Py_DECREF(next_iter);
    return awaitable;
}

void
_PyEval_LoadGlobalValue(PyObject *globals, PyObject *builtins, PyObject *name,
                        PyValue *writeto, _PyValueTemps *temps)
{
    PyObject *namespace = globals;
    if (PyAnyDict_CheckExact(globals) && PyAnyDict_CheckExact(builtins)) {
        namespace = _PyDict_LoadGlobalValue((PyDictObject *)globals,
                                             (PyDictObject *)builtins,
                                             name, writeto);
        if (PyValue_IsNull(*writeto) && !PyErr_Occurred()) {
            // A missing key does not set an exception in the dictionary helper.
            _PyEval_FormatExcCheckArg(PyThreadState_GET(), PyExc_NameError,
                                        NAME_ERROR_MSG, name);
        }
    }
    else {
        /* Slow-path if globals or builtins is not a dict */
        /* namespace 1: globals */
        PyObject *res;
        if (PyMapping_GetOptionalItem(globals, name, &res) < 0) {
            *writeto = PyValue_NULL;
            return;
        }
        if (res == NULL) {
            /* namespace 2: builtins */
            namespace = builtins;
            if (PyMapping_GetOptionalItem(builtins, name, &res) < 0) {
                *writeto = PyValue_NULL;
                return;
            }
            if (res == NULL) {
                _PyEval_FormatExcCheckArg(
                            PyThreadState_GET(), PyExc_NameError,
                            NAME_ERROR_MSG, name);
                *writeto = PyValue_NULL;
                return;
            }
        }
        *writeto = PyValue_FromPyObjectSteal(res);
    }

    PyObject *res_o = PyValue_IsObject(*writeto) ?
        PyValue_AsPyObjectBorrow(*writeto) : NULL;
    if (res_o != NULL && PyLazyImport_CheckExact(res_o)) {
        PyObject *l_v = _PyLazyImport_Reify(
            PyThreadState_GET(), res_o, name, namespace);
        PyValue old = *writeto;
        *writeto = l_v == NULL ? PyValue_NULL : PyValue_FromPyObjectSteal(l_v);
        *writeto = _PyValueTemps_Publish(temps, writeto);
        PyObject *saved = PyErr_GetRaisedException();
        PyValue_CLOSE(old);
        _PyValue_RestoreException(saved);
    }
    if (!PyValue_IsNull(*writeto)) {
        /* Queue private scalar owners in the caller's context. Its actual
         * frame publication must precede their callback-capable release. */
        *writeto = _PyValueTemps_Publish(temps, writeto);
    }
}

PyObject *
_PyEval_GetAwaitable(PyObject *iterable, int oparg)
{
    PyObject *iter = _PyCoro_GetAwaitableIter(iterable);

    if (iter == NULL) {
        _PyEval_FormatAwaitableError(PyThreadState_GET(),
            Py_TYPE(iterable), oparg);
    }
    else if (PyCoro_CheckExact(iter)) {
        PyCoroObject *coro = (PyCoroObject *)iter;
        int8_t frame_state = FT_ATOMIC_LOAD_INT8_RELAXED(coro->cr_frame_state);
        if (frame_state == FRAME_SUSPENDED_YIELD_FROM ||
            frame_state == FRAME_SUSPENDED_YIELD_FROM_LOCKED)
        {
            /* `iter` is a coroutine object that is being awaited. */
            Py_CLEAR(iter);
            _PyErr_SetString(PyThreadState_GET(), PyExc_RuntimeError,
                             "coroutine is being awaited already");
        }
    }
    return iter;
}

PyObject *
_PyEval_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *name)
{
    PyObject *value;
    PyObject *namespace = frame->f_locals;
    if (namespace == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                            "no locals found");
        return NULL;
    }
    if (PyMapping_GetOptionalItem(namespace, name, &value) < 0) {
        return NULL;
    }
    if (value != NULL) {
        goto found;
    }
    namespace = frame->f_globals;
    if (PyDict_GetItemRef(namespace, name, &value) < 0) {
        return NULL;
    }
    if (value != NULL) {
        goto found;
    }
    namespace = frame->f_builtins;
    if (PyMapping_GetOptionalItem(namespace, name, &value) < 0) {
        return NULL;
    }
    if (value == NULL) {
        _PyEval_FormatExcCheckArg(
                    tstate, PyExc_NameError,
                            NAME_ERROR_MSG, name);
        return NULL;
    }
found:
    if (PyLazyImport_CheckExact(value)) {
        Py_SETREF(value, _PyLazyImport_Reify(tstate, value, name, namespace));
    }
    return value;
}

PyValue _PyForIter_VirtualIteratorNext(PyThreadState* tstate, _PyInterpreterFrame* frame, PyValue iter, PyValue* index_ptr)
{
    PyObject *iter_o = PyValue_AsPyObjectBorrow(iter);
    PyValue index = *index_ptr;
    if (PyValue_IsTaggedIndex(index)) {
        intptr_t i = PyValue_UntagIndex(index);
        assert(i >= 0);
        _PyValueIndexPair next_index = Py_TYPE(iter_o)->_tp_iteritem(iter_o, i);
        i = next_index.index;
        if (PyValue_IsNull(next_index.value)) {
            return i < 0 ? PyValue_ERROR : PyValue_NULL;
        }
        *index_ptr = PyValue_TagIndex(i);
        return next_index.value;
    }
    PyObject *next = (*Py_TYPE(iter_o)->tp_iternext)(iter_o);
    if (next == NULL) {
        if (_PyErr_Occurred(tstate)) {
            if (_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                _PyEval_MonitorRaise(tstate, frame, frame->instr_ptr);
                _PyErr_Clear(tstate);
            }
            else {
                return PyValue_ERROR;
            }
        }
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(next);
}

/* Check if a 'cls' provides the given special method. */
static inline int
type_has_special_method(PyTypeObject *cls, PyObject *name)
{
    assert(!PyErr_Occurred());
    PyObject *r = _PyType_LookupRef(cls, name);
    int res = r != NULL && Py_TYPE(r)->tp_descr_get != NULL;
    Py_XDECREF(r);
    return res;
}

int
_PyEval_SpecialMethodCanSuggest(PyObject *self, int oparg)
{
    PyTypeObject *type = Py_TYPE(self);
    switch (oparg) {
        case SPECIAL___ENTER__:
        case SPECIAL___EXIT__: {
            return type_has_special_method(type, &_Py_ID(__aenter__))
                   && type_has_special_method(type, &_Py_ID(__aexit__));
        }
        case SPECIAL___AENTER__:
        case SPECIAL___AEXIT__: {
            return type_has_special_method(type, &_Py_ID(__enter__))
                   && type_has_special_method(type, &_Py_ID(__exit__));
        }
        default:
            Py_FatalError("unsupported special method");
    }
}
