/* Execute compiled code */

#include "ceval.h"
#include "pycore_dict.h"          // PyDict_GetValue()
#include "pycore_function.h"      // FUNC_VERSION_FIRST_VALID
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_long.h"
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_native_scope.h"
#include "pycore_setobject.h"     // PySet_AddValue()
#include "pycore_transition_roots.h" // _PyTransitionRoots
#include "pycore_tuple.h"         // _PyTuple_PackValues()
#include "pycore_value_call.h"    // _PyValue_VectorcallTstate()
#include "pycore_value_ops.h"     // _PyValue_AsSsize_t()

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
    PyValue get = PyValue_NULL;
    PyValue map_v = PyValue_DUP(PyValue_FromObjectBorrow(map));
    _PyStatusValue found = _PyValue_GetMethod(map_v, &_Py_ID(get));
    if (found.status < 0) {
        goto fail;
    }
    get = found.value;
    int has_self = found.status > 0;
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
        // The key stays a value, so an inline key is not boxed
        PyValueBorrow key_v = PyTuple_GET_VALUE(keys, i);
        if (PySet_ContainsValue(seen, key_v) || PySet_AddValue(seen, key_v)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                PyValue key_owner = PyTuple_GetValue(keys, i);
                key = PyValue_IsNull(key_owner) ? NULL
                    : _PyValue_AsObjectStealAt(key_owner, BOX_SITE_TUPLE);
                if (key != NULL) {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "mapping pattern checks duplicate key (%R)", key);
                }
            }
            goto fail;
        }
        PyValue value;
        if (PyDict_CheckExact(map)) {
            // dict.get(key, dummy) without the call
            int r = PyDict_GetValue(map, key_v, &value);
            if (r < 0) {
                goto fail;
            }
            if (r == 0) {
                // key not in map!
                Py_DECREF(values);
                values = Py_NewRef(Py_None);
                goto done;
            }
        }
        else {
            PyValue k = PyValue_DUP(key_v);
            PyValue dummy_v = PyValue_DUP(PyValue_FromObjectBorrow(dummy));
            PyValue args[] = { map_v, k, dummy_v };
            value = has_self
                ? _PyValue_Vectorcall(get, args, 3, NULL)
                : _PyValue_Vectorcall(get, &args[1], 2, NULL);
            PyValue_CLOSE(k);
            PyValue_CLOSE(dummy_v);
            if (PyValue_IsNull(value)) {
                goto fail;
            }
            if (PyValue_IsObject(value) &&
                PyValue_AsKnownObjectBorrow(value) == dummy) {
                // key not in map!
                PyValue_CLOSE(value);
                Py_DECREF(values);
                values = Py_NewRef(Py_None);
                goto done;
            }
        }
        PyTuple_SET_VALUE(values, i, value);
    }
    // Success:
done:
    PyValue_CLOSE(get);
    PyValue_CLOSE(map_v);
    Py_DECREF(seen);
    Py_DECREF(dummy);
    Py_XDECREF(key);
    return values;
fail:
    PyValue_XCLOSE(get);
    PyValue_XCLOSE(map_v);
    Py_XDECREF(seen);
    Py_XDECREF(dummy);
    Py_XDECREF(values);
    Py_XDECREF(key);
    return NULL;
}

// Extract a named attribute from the subject, with additional bookkeeping to
// raise TypeErrors for repeated lookups. On failure, return NULL (with no
// error set). Use _PyErr_Occurred(tstate) to disambiguate.
static PyValue
match_class_attr(PyThreadState *tstate, PyValueBorrow subject, PyObject *type,
                 PyObject *name, PyObject *seen)
{
    assert(PyUnicode_CheckExact(name));
    // Only check for duplicates if seen is not NULL.
    if (seen != NULL) {
        assert(PySet_CheckExact(seen));
        if (PySet_ContainsValue(seen, PyValue_FromObjectBorrow(name)) || PySet_AddValue(seen, PyValue_FromObjectBorrow(name))) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                _PyErr_Format(tstate, PyExc_TypeError,
                            "%s() got multiple sub-patterns for attribute %R",
                            ((PyTypeObject*)type)->tp_name, name);
            }
            return PyValue_NULL;
        }
    }
    // A missing attribute is a failed match, with no exception
    return _PyValue_GetOptionalAttr(subject, name).value;
}

// On success (match), return a tuple of extracted attributes. On failure (no
// match), return NULL. Use _PyErr_Occurred(tstate) to disambiguate.
PyObject*
(_PyEval_MatchClass)(PyThreadState *tstate, PyValueBorrow subject, PyObject *type,
                     Py_ssize_t nargs, PyObject *kwargs)
{
    if (!PyType_Check(type)) {
        const char *e = "class pattern must refer to a class";
        _PyErr_Format(tstate, PyExc_TypeError, e);
        return NULL;
    }
    assert(PyTuple_CheckExact(kwargs));
    // First, an isinstance check:
    PyValue type_value = PyValue_DUP(PyValue_FromObjectBorrow(type));
    int isinstance = _PyEval_IsInstanceValue(subject, type_value);
    PyValue_CLOSE(type_value);
    if (isinstance <= 0) {
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
            PyTuple_SET_VALUE(attrs, 0, PyValue_DUP(subject));
        }
        else {
            for (Py_ssize_t i = 0; i < nargs; i++) {
                PyValueBorrow name_value = PyTuple_GET_VALUE(match_args, i);
                if (!PyValue_IsObject(name_value) ||
                    !PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(name_value))) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "__match_args__ elements must be strings "
                                  "(got %s)", PyValue_TYPE(name_value)->tp_name);
                    goto fail;
                }
                PyObject *name = PyValue_AsKnownObjectBorrow(name_value);
                PyValue attr = match_class_attr(tstate, subject, type, name,
                                                seen);
                if (PyValue_IsNull(attr)) {
                    goto fail;
                }
                assert(PyValue_IsNull(PyTuple_GET_VALUE(attrs, i)));
                PyTuple_SET_VALUE(attrs, i, attr);
            }
        }
        Py_CLEAR(match_args);
    }
    // Finally, the keyword subpatterns:
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *name = _PyTuple_GET_OBJECT(kwargs, i);
        PyValue attr = match_class_attr(tstate, subject, type, name, seen);
        if (PyValue_IsNull(attr)) {
            goto fail;
        }
        assert(PyValue_IsNull(PyTuple_GET_VALUE(attrs, nargs + i)));
        PyTuple_SET_VALUE(attrs, nargs + i, attr);
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

static void
clear_value_arguments(_PyInterpreterFrame *frame, PyValue *arguments, int count)
{
    for (int i = count - 1; i >= 0; i--) {
        // a move: cleared below
        PyValue value = arguments[i];
        _PyFrame_WriteHome(frame, arguments + i, PyValue_NULL);
        PyValue_CLOSE(value);
    }
}

/* Two-word results of the helpers that handlers call: returned in rax:rdx
 * under the x86-64 SysV ABI, so a handler keeps no addressable local for
 * them (pyjit/design/proposals/register-ir-v2-a3.md 3.5, R-a).
 * _PyStatusValue is in pycore_value.h. */
typedef struct {
    PyValue first;
    PyValue second;
} _PyValuePair;

typedef struct {
    int status;
    PyObject *object;
} _PyStatusObject;

typedef struct {
    int status;
    _PyInterpreterFrame *child;
} _PyCallResult;

typedef struct {
    PyObject *match;
    PyObject *rest;
} _PyObjectPair;

typedef enum {
    CALL_TRANSACTION_VECTOR,
    CALL_TRANSACTION_KEYWORDS,
    CALL_TRANSACTION_EX,
} _PyCallTransactionKind;

/* The flags of call_transaction_run_*(): what the handler did before the
 * call (CALL_EFFECTS, CALL_PREPARED, CALL_MONITORED, in its register
 * local call_state) and two constants of the handler. */
#define CALL_EFFECTS 1
#define CALL_PREPARED 2
#define CALL_MONITORED 4
#define CALL_INSTRUMENTED 8
#define CALL_PERIODIC 16
#define CALL_SELF_PRESENT 32
#define CALL_KWARGS_PRESENT 64
#define CALL_FAILED 128

typedef enum {
    CALL_SELF_ABSENT,
    CALL_SELF_SUPPLIED,
} _PyCallSelfRole;

/* The state of one call, which lives only in the frame of the out-of-line
 * call_transaction_run_*(), across the helpers it calls (R-c).  Numeric
 * homes survive instrumentation changes.  This record owns no preparation
 * resource: each preparation helper drains its locals before returning,
 * including before an inlined Python frame can escape. */
typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    _PyInstr *current;
    int base;
    int count;
    int destination;
    int argument_count;
    _PyCallTransactionKind kind;
    int instrumented;
    int monitored;
    int periodic;
    int state;
} _PyCallTransaction;

/* Raise the SystemError of a failed revalidation. An operation error can be
 * pending; it becomes the SystemError's __cause__ (as in
 * _Py_CheckFunctionResult) instead of being discarded. */
static void
value_revalidation_error(const char *message)
{
    if (PyErr_Occurred()) {
        _PyErr_FormatFromCause(PyExc_SystemError, "%s", message);
    }
    else {
        PyErr_SetString(PyExc_SystemError, message);
    }
}

/* Whether frame is still the thread's current frame and still runs the
 * executing copy of its code.  An operation can run Python; the frame and
 * its code are values the handler keeps in registers, so the check needs
 * no record of them (R-b). */
static inline int
value_frame_is_current(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    if (tstate->current_frame != frame) {
        return 0;
    }
    _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
    return exec != NULL && frame->ir_words == exec->words;
}

/* Move each owner whose slot is a temporary back into it, and return the
 * owners it did not move (PyValue_NULL for a moved one). */
static _PyValuePair
value_operation_restore2(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         PyValue first, PyValue second,
                         int first_slot, int second_slot, uint16_t phase)
{
    frame->phase_state = 0;
    _PyValuePair rest = {first, second};
    if (!value_frame_is_current(tstate, frame)) {
        value_revalidation_error("dictionary store changed the active frame");
        return rest;
    }
    int nlocals = _PyFrame_GetCode(frame)->co_nlocalsplus;
    int nslots = _PyFrame_NumSlots(frame);
    PyValue *slots = _PyFrame_Slots(frame);
    int input_slots[2] = {first_slot, second_slot};
    for (int i = 0; i < 2; i++) {
        int slot = input_slots[i];
        if (slot < 0 || slot >= nslots ||
            (slot >= nlocals && !PyValue_IsNull(slots[slot]))) {
            value_revalidation_error("dictionary store changed an input home");
            return rest;
        }
    }
    if (first_slot >= nlocals) {
        _PyFrame_WriteHome(frame, slots + first_slot, first);
        _PyValue_CheckTemp(slots[first_slot]);
        rest.first = PyValue_NULL;
    }
    if (second_slot >= nlocals) {
        _PyFrame_WriteHome(frame, slots + second_slot, second);
        _PyValue_CheckTemp(slots[second_slot]);
        rest.second = PyValue_NULL;
    }
    frame->phase_state = phase;
    /* Callback-current locals remain intact. Their extra evaluated
     * snapshots are still C-owned; only temporary owners moved back. */
    return rest;
}

/* Fixed store operands have already moved out of their frame homes. Keep
 * their original release order, with no decoded slot access between closes. */
static inline void
value_operation_close3(PyValue first, PyValue second, PyValue third)
{
    PyValue_XCLOSE(first);
    PyValue_XCLOSE(second);
    PyValue_XCLOSE(third);
}

/* IMPORT_NAME's level as a C int, PyLong_AsInt() of it: -1 with an exception
 * set on error.  The compiler's level is an inline int. */
static inline int
value_import_level(PyValue level)
{
    if (PyValue_IsInlineInt(level)) {
        int64_t i = PyValue_InlineInt64(level);
        if (INT_MIN <= i && i <= INT_MAX) {
            return (int)i;
        }
    }
    PyObject *level_o = PyValue_AsObject(level);
    if (level_o == NULL) {
        return -1;
    }
    int ilevel = _PyValue_AsInt(PyValue_FromObjectBorrow(level_o));
    Py_DECREF(level_o);
    return ilevel;
}

/* container[key] for BINARY_OP's NB_SUBSCR: an exact dict, and an exact list
 * or tuple indexed by an inline int in range, as list_subscript() and
 * tuple_subscript() do, read the item as a value; anything else is
 * _PyValue_GetItem(). A dict miss raises KeyError with the key as it is.
 * The caller holds an owner of both. */
static PyValue
value_subscript(PyValue container, PyValue key)
{
    if (PyValue_IsObject(container)) {
        PyObject *o = PyValue_AsKnownObjectBorrow(container);
        if (PyAnyDict_CheckExact(o)) {
            PyValue result = PyValue_NULL;
            int found = PyDict_GetValue(o, key, &result);
            if (found != 0) {
                return result;
            }
            /* Exact dictionaries have no __missing__. Do not repeat a hash
             * or equality callback just to construct the missing-key error. */
            _PyErr_SetKeyErrorValue(key);
            return PyValue_NULL;
        }
        if (PyValue_IsInlineInt(key)) {
            int is_list = PyList_CheckExact(o);
            if (is_list || PyTuple_CheckExact(o)) {
                Py_ssize_t size = Py_SIZE(o);
                Py_ssize_t index = PyValue_InlineInt64(key);
                if (index < 0) {
                    index += size;
                }
                if (0 <= index && index < size) {
                    return is_list
                        ? _PyList_GetValue((PyListObject *)o, index)
                        : PyTuple_GetValue(o, index);
                }
            }
        }
    }
    return _PyValue_GetItem(container, key);
}

/* The wrappers below keep a runtime-API out-parameter in their own C frame,
 * so that the handler that calls them takes the address of none of its
 * locals (spec 3.5, R-c).  Py_NO_INLINE keeps the address from returning to
 * the handler. */

/* PyUnicode_Append takes the address of the string it replaces.  Consumes
 * `left` and returns the result, or NULL with an exception set; `left` is
 * released on error, as PyUnicode_Append does. */
static Py_NO_INLINE PyObject *
value_unicode_append(PyObject *left, PyObject *right)
{
    PyUnicode_Append(&left, right);
    return left;
}

/* _PyDict_MergeUniq reports the duplicate key through an out-parameter.
 * Returns the status and, on a negative status, the new reference to the
 * duplicate key or NULL. */
static Py_NO_INLINE _PyStatusObject
value_dict_merge_uniq(PyObject *dict, PyObject *update)
{
    PyObject *dupkey = NULL;
    _PyStatusObject result;
    result.status = _PyDict_MergeUniq(dict, update, &dupkey);
    result.object = dupkey;
    return result;
}

/* _PyEval_ExceptionGroupMatch returns its two results through
 * out-parameters.  Returns {match, rest} as new references; match is NULL
 * when the match raised (an exception is set). */
static Py_NO_INLINE _PyObjectPair
value_exception_group_match(_PyInterpreterFrame *frame, PyObject *exc_value,
                            PyObject *match_type)
{
    PyObject *match = NULL;
    PyObject *rest = NULL;
    _PyObjectPair result = {NULL, NULL};
    if (_PyEval_ExceptionGroupMatch(frame, exc_value, match_type,
                                    &match, &rest) < 0) {
        return result;
    }
    assert((match == NULL) == (rest == NULL));
    result.match = match;
    result.rest = rest;
    return result;
}

#include "ceval_macros.h"


/* Helper functions to keep the size of the largest uops down */

PyObject *
_Py_BuildString_ValueSteal(
    _PyInterpreterFrame *frame,
    PyValue *arguments,
    int total_args)
{
    /* The pieces are str objects (constants and the results of FORMAT_*),
     * never inline words: the join reads the window's objects in place on
     * the borrow stack. */
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    _PyValue_RecheckWindow(arguments, total_args);
    PyObject **objects = _PyBorrow_ValueVector(arguments, total_args);
    PyObject *res = NULL;
    if (objects != NULL) {
        res = _PyUnicode_JoinArray(&_Py_STR(empty), objects, total_args);
    }
    _PyBorrowScope_Exit(tstate, mark);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    clear_value_arguments(frame, arguments, total_args);
    return res;
}

PyObject *
_Py_BuildMap_ValueSteal(
    _PyInterpreterFrame *frame,
    PyValue *arguments,
    int half_args)
{
    /* RegCFG verifies that this evaluated window contains consecutive
     * temporaries, not locals exposed to callbacks through f_locals, and
     * temporaries own their values. */
    _PyValue_RecheckWindow(arguments, half_args * 2);
    PyObject *res = _PyDict_FromValueItems(arguments, 2, arguments + 1, 2,
                                          half_args);
    clear_value_arguments(frame, arguments, half_args * 2);
    return res;
}

/* Out of line: the vectorcall's argument array is a local, which would be
 * the handler's (spec 3.5, R-c).  super(class, self), or super() when nargs
 * is 0. */
static Py_NO_INLINE PyValue
super_call(PyObject *global_super, PyObject *class, PyValue self, int nargs)
{
    PyValue stack[] = {PyValue_DUP(PyValue_FromObjectBorrow(class)), self};
    PyValue super = _PyValue_Vectorcall(PyValue_FromObjectBorrow(global_super),
                                        stack, nargs, NULL);
    PyValue_CLOSE(stack[0]);
    return super;
}

/* Out of line, as super_call: exit_func(exit_self, exc, val, tb), or
 * exit_func(exc, val, tb) when has_self is 0.  The array keeps a free slot
 * before the arguments for PY_VECTORCALL_ARGUMENTS_OFFSET. */
static Py_NO_INLINE PyObject *
with_exit_call(PyObject *exit_func, PyObject *exit_self, int has_self,
               PyObject *exc, PyObject *val, PyObject *tb)
{
    PyObject *stack[5] = {NULL, exit_self, exc, val, tb};
    PyObject *res = PyObject_Vectorcall(exit_func, stack + 2 - has_self,
            (3 + has_self) | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    _PyGC_CLEAR_ARRAY(stack);
    return res;
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
    _PyIRWord words[1];
} entry_trampoline = {
    .exec = { .nwords = 1, .nlocalsplus = 1 },
    .words = {
        IR_WORD(INTERPRETER_EXIT, 1, 0, 0, 0),  /* reached on return and yield */
    },
};
#define _Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS (entry_trampoline.words)

const _PyInstr *_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS_PTR = (_PyInstr*)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;



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


/* The iterator of iterable and its index: {iterable, tagged index 0} when
   the interpreter indexes it, {iterator, PyValue_NULL} otherwise, and
   {PyValue_NULL, PyValue_NULL} with an exception set.  Consumes iterable.
   Both come back in registers (spec 3.5, R-a); out of line, as when it was
   exported. */
static Py_NO_INLINE _PyValuePair
_PyEval_GetIter(PyValue iterable, int yield_from)
{
    _PyValuePair result = {PyValue_NULL, PyValue_NULL};
    PyTypeObject *tp = PyValue_TYPE(iterable);
    if (tp->_tp_iteritem != NULL) {
        /* Leave iterable on stack and push tagged index 0 */
        result.first = iterable;
        result.second = PyValue_TagIndex(0);
        return result;
    }
    if (tp->tp_iter == PyObject_SelfIter) {
        result.first = iterable;
        return result;
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
            return result;
        }
        result.first = iterable;
        return result;
    }
    /* Pop iterable, and push iterator then NULL. An int or float is not
       iterable; PyObject_GetIter() raises the error. */
    PyObject *iter_o = _PyValue_GetIter(iterable);
    PyValue_CLOSE(iterable);
    if (iter_o == NULL) {
        return result;
    }
    result.first = PyValue_FromObjectSteal(iter_o);
    return result;
}

int (_PyEval_StoreName)(PyThreadState *tstate, PyValueBorrow v, PyObject *name, PyObject* ns)
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
        int error = _PyValue_DelItem(PyValue_FromObjectBorrow(ns), PyValue_FromObjectBorrow(name));
        if (error) {
            _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                    NAME_ERROR_MSG,
                                    name);
        }
        return error;
    }

    if (PyDict_CheckExact(ns)) {
        // PyDict_SetItem() with the value as it is: an inline value needs no box.
        return PyDict_SetValue(ns, PyValue_FromObjectBorrow(name), v);
    }
    return _PyValue_SetItem(PyValue_FromObjectBorrow(ns),
                            PyValue_FromObjectBorrow(name), v);
}

/* The wrappers below keep the address of a C local out of the interpreter
   handlers (spec 3.5, R-c): each runtime API with an out-parameter runs in
   its own C frame and returns its outputs by value. */

/* namespace[name] for LOAD_FROM_DICT_OR_GLOBALS and _DEREF, as
 * _PyValue_GetOptionalItem(), with a lazy import in the result reified. */
static Py_NO_INLINE _PyStatusValue
value_load_from_dict(PyThreadState *tstate, PyObject *namespace,
                     PyObject *name)
{
    _PyStatusValue found = _PyValue_GetOptionalItem(
        PyValue_FromObjectBorrow(namespace), PyValue_FromObjectBorrow(name));
    if (found.status > 0 && PyValue_IsObject(found.value) &&
        PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(found.value)))
    {
        PyObject *reified = _PyLazyImport_Reify(
            tstate, PyValue_AsKnownObjectBorrow(found.value), name, namespace);
        PyValue_CLOSE(found.value);
        found.value = reified == NULL ? PyValue_NULL
                                      : PyValue_FromObjectSteal(reified);
        if (reified == NULL) {
            found.status = -1;
        }
    }
    return found;
}

/* Out of line: _PyEval_LoadGlobalValue writes the value through a pointer.
   Returns PyValue_NULL, with an exception set, on error. */
static Py_NO_INLINE PyValue
value_load_global(PyObject *globals, PyObject *builtins, PyObject *name)
{
    PyValue v = PyValue_NULL;
    _PyEval_LoadGlobalValue(globals, builtins, name, &v);
    return v;
}

/* The next item of iter: {1, item} (owned), {0, PyValue_NULL} at the end
   with no exception set, {-1, PyValue_NULL} with an exception set.  The
   index lives in the caller's frame slot, *index_ptr, which it updates
   (spec 3.5, R-a and R-d). */
static _PyStatusValue
_PyForIter_VirtualIteratorNext(PyThreadState *tstate, _PyInterpreterFrame *frame,
                               PyValue iter, PyValue *index_ptr);

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

#ifndef NDEBUG
/* Whether the instrumentation of code, which a running frame of tstate
 * executes, is as the eval loop keeps it.  Setting events re-instruments
 * every code object with a frame on a thread's stack
 * (instrument_all_executing_code_objects), and a frame's RESUME
 * re-instruments its code when it starts, except while tracing in the
 * default build (_MAYBE_INSTRUMENT).  A walk that fails stops with the error
 * set; once Python code has handled the error, the frames the walk did not
 * reach run code older than the version that failed, until a later walk
 * completes. */
static int
instrumentation_is_current(PyThreadState *tstate, PyCodeObject *code)
{
    uintptr_t epoch = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) &
                      ~_PY_EVAL_EVENTS_MASK;
    uintptr_t version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(code->_co_instrumentation_version);
    uintptr_t failed = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(
        tstate->interp->ceval.instrumentation_failed_version);
    if (version == epoch || version < failed || PyErr_Occurred()) {
        return 1;
    }
#ifdef Py_GIL_DISABLED
    return 0;
#else
    return tstate->tracing != 0;
#endif
}
#endif

/* After the operation, check that the frame is still its own and restore
 * the frame's instr_ptr to current, the handler's instruction (its
 * this_instr), which the handler passes by value (R-b).  The operation can
 * run Python, and so can the release of its operands, of its results and of
 * a displaced local (__del__, weakref callbacks).  Python can change the
 * instrumentation, which rewrites handler fields in place and which dispatch
 * reads, but the executing copy is fixed for the code object's life
 * (pyjit/design/register-ir.md 1.10): frame->ir_words and the handler's
 * current and next instructions stay valid, and the checks here only assert
 * it.  On failure it closes failed_result. */
static int
value_operation_refresh(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *current, PyValue failed_result)
{
    if (tstate->current_frame != frame) {
        value_revalidation_error("value bridge changed the active frame");
        PyValue_XCLOSE(failed_result);
        return -1;
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    assert(instrumentation_is_current(tstate, code));
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || frame->ir_words != exec->words ||
        current < exec->words || current >= exec->words + exec->nwords) {
        /* Keep a pending operation error. */
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "value bridge changed executing words");
        }
        PyValue_XCLOSE(failed_result);
        return -1;
    }
    assert(frame->ir_feedback == exec->entries);
    frame->instr_ptr = current;
    return 0;
}

/* Release owners[0] to owners[count - 1] in order, then the handler's input
 * boxes, then refresh: releasing an owner can run Python. */
static int
finish_value_bridges_ex(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *next, _PyValueTemps *temps,
                       PyValue *owners, int count, PyValue failed_result)
{
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        for (int i = 0; i < count; i++) {
            PyValue_XCLOSE(owners[i]);
        }
        _PyValueTemps_Close(temps);
        return 0;
    }
    _PyInstr *current = frame->instr_ptr;
#ifndef NDEBUG
    _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
    assert(exec != NULL && frame->ir_words == exec->words);
    assert(current >= exec->words && current < exec->words + exec->nwords);
    assert(next >= exec->words && next <= exec->words + exec->nwords);
#endif
    (void)next;
    for (int i = 0; i < count; i++) {
        PyValue_XCLOSE(owners[i]);
    }
    _PyValueTemps_Close(temps);
    return value_operation_refresh(tstate, frame, current, failed_result);
}

static int
value_operation_finish(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *current, _PyValueTemps *temps)
{
    /* The operation can run Python even when it created no private boxes:
     * check the frame even when the owner record is empty. */
    _PyValueTemps_Close(temps);
    return value_operation_refresh(tstate, frame, current, PyValue_NULL);
}

/* The checks of a call's window, before the call has any effect; the
 * handler keeps base and destination in registers. */
static inline int
call_transaction_begin(_PyInterpreterFrame *frame, int base, int count,
                       int destination, _PyCallTransactionKind kind,
                       unsigned int virtual_holes)
{
    assert(base >= _PyFrame_GetCode(frame)->co_nlocalsplus);
    assert(count >= 0 && base <= _PyFrame_NumSlots(frame) - count);
    assert((destination >= 0 && destination < _PyFrame_NumSlots(frame))
           || destination == IR_NO_DST);
    (void)destination;
    /* Every call input is an evaluated temporary, including snapshots of
     * locals, and owns its value. Callback-current LOCAL homes are never
     * consumed by this record. */
    PyValue *homes = _PyFrame_Slots(frame) + base;
    for (int i = 0; i < count; i++) {
        if ((i == 1 && (virtual_holes & _PyIR_CALL_HOLE_SELF)) ||
            (i == 3 && (virtual_holes & _PyIR_CALL_HOLE_KWARGS))) {
            continue;
        }
        _PyValue_RecheckHome(homes[i]);
    }
    int roles = 0;
    if (!(virtual_holes & _PyIR_CALL_HOLE_SELF) && !PyValue_IsNull(homes[1])) {
        roles |= CALL_SELF_PRESENT;
    }
    if (kind == CALL_TRANSACTION_EX &&
        !(virtual_holes & _PyIR_CALL_HOLE_KWARGS) && !PyValue_IsNull(homes[3])) {
        roles |= CALL_KWARGS_PRESENT;
    }
    return roles;
}

/* Window-relative geometry includes reserved virtual cells. Only actual
 * input owners are vacated; virtual cells supply no readable value. */
static inline void
call_clear_owned_slots(_PyInterpreterFrame *frame, PyValue *window, int first, int count,
                       _PyCallTransactionKind kind, int state)
{
    for (int i = first; i < first + count; i++) {
        if ((i == 1 && !(state & CALL_SELF_PRESENT)) ||
            (i == 3 && kind == CALL_TRANSACTION_EX &&
             !(state & CALL_KWARGS_PRESENT))) {
            continue;
        }
        _PyFrame_WriteHome(frame, window + i, PyValue_NULL);
    }
}

static int
call_transaction_refresh(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         _PyInstr *current)
{
    int status = value_operation_refresh(tstate, frame, current, PyValue_NULL);
    if (status == 0) {
        return 0;
    }
    if (!value_frame_is_current(tstate, frame)) {
        value_revalidation_error(
            "call preparation changed the active frame");
        return -1;
    }
    return status;
}

/* Return the prepared state even on failure after effects. Promotion supplies
 * a real self before any obsolete-wrapper callback; no absent home is read. */
static int
call_transaction_expand_method(PyThreadState *tstate, _PyInterpreterFrame *frame,
                               _PyInstr *current, int base, int state)
{
    PyValue *homes = _PyFrame_Slots(frame) + base;
    if (PyValue_TYPE(homes[0]) != &PyMethod_Type ||
        (state & CALL_SELF_PRESENT)) {
        return state | CALL_PREPARED;
    }
    PyMethodObject *method = (PyMethodObject *)PyValue_AsKnownObjectBorrow(homes[0]);
    PyValue self = PyValue_DUP(method->im_self);
    PyValue function = PyValue_DUP(PyValue_FromObjectBorrow(method->im_func));
    PyValue obsolete = homes[0];
    /* Publish both final input homes before the obsolete wrapper can run
     * a destructor. No pointer into the old window crosses that release. */
    _PyFrame_WriteHome(frame, homes, function);
    _PyFrame_WriteHome(frame, homes + 1, self);
    state |= CALL_SELF_PRESENT | CALL_PREPARED | CALL_EFFECTS;
    _PyValue_CheckHome(homes[0]);
    _PyValue_CheckHome(homes[1]);
    PyValue_CLOSE(obsolete);
    int status = call_transaction_refresh(tstate, frame, current);
    return state | (status < 0 ? CALL_FAILED : 0);
}

/* Replace CALL_FUNCTION_EX's positional arguments by a tuple.  It is an
 * effect even when they are one already. */
static int
call_transaction_prepare_tuple(PyThreadState *tstate, _PyInterpreterFrame *frame,
                               _PyInstr *current, int base)
{
    PyValue *homes = _PyFrame_Slots(frame) + base;
    PyValue args = PyValue_DUP(homes[2]);
    PyValue func = PyValue_DUP(homes[0]);
    PyValue obsolete = PyValue_NULL;
    PyObject *args_object = NULL, *func_object = NULL, *tuple = NULL;
    int status = -1;
    args_object = PyValue_AsObject(args);
    if (args_object == NULL) {
        goto done;
    }
    if (PyTuple_CheckExact(args_object)) {
        status = 0;
        goto done;
    }
    func_object = PyValue_AsObject(func);
    if (func_object == NULL) {
        goto done;
    }
    if (_Py_Check_ArgsIterable(tstate, func_object, args_object) < 0) {
        goto done;
    }
    tuple = _PyValue_SequenceTuple(PyValue_FromObjectBorrow(args_object));
    if (tuple == NULL || call_transaction_refresh(tstate, frame, current) < 0) {
        goto done;
    }
    homes = _PyFrame_Slots(frame) + base;
    obsolete = homes[2];
    /* A tuple is a heap object, so this pure owner transfer cannot retire a
     * scalar box. Only the final tuple home is visible during local cleanup. */
    _PyFrame_WriteHome(frame, homes + 2, PyValue_FromObjectSteal(tuple));
    tuple = NULL;
    status = 0;
done:
    Py_CLEAR(tuple);
    Py_CLEAR(func_object);
    Py_CLEAR(args_object);
    PyValue_CLEAR(func);
    PyValue_CLEAR(args);
    PyValue_CLEAR(obsolete);
    if (call_transaction_refresh(tstate, frame, current) < 0) {
        return -1;
    }
    return status;
}

/* The word of an immortal or caller-pinned object for a monitoring argument:
 * uncounted, never closed. */
static inline PyValue
borrowed_word(PyObject *obj)
{
    PyValue word;
    _PyValue_WindowFromObjects(&word, &obj, 1);
    return word;
}

/* The CALL event of an instrumented call. The function and the first
 * argument are read as words: an inline first argument is not boxed. */
static int
call_transaction_monitor(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         _PyInstr *current, int base, int argument_count,
                         _PyCallTransactionKind kind, int roles)
{
    PyValue *homes = _PyFrame_Slots(frame) + base;
    PyValue function = PyValue_DUP(homes[0]);
    PyValue first = PyValue_NULL;
    if (kind == CALL_TRANSACTION_EX) {
        PyObject *tuple = PyValue_AsKnownObjectBorrow(homes[2]);
        if (PyTuple_GET_SIZE(tuple)) {
            first = PyValue_DUP(PyTuple_GET_VALUE(tuple, 0));
        }
    }
    else if (roles & CALL_SELF_PRESENT) {
        first = PyValue_DUP(homes[1]);
    }
    else if (argument_count) {
        first = PyValue_DUP(homes[2]);
    }
    int status = -1;
    if (call_transaction_refresh(tstate, frame, current) >= 0) {
        status = _Py_call_instrumentation_2args(
            tstate, PY_MONITORING_EVENT_CALL, frame,
            frame->instr_ptr, function,
            PyValue_IsNull(first)
                ? borrowed_word(&_PyInstrumentation_MISSING) : first);
    }
    PyValue_XCLOSE(first);
    PyValue_CLEAR(function);
    if (call_transaction_refresh(tstate, frame, current) < 0) {
        return -1;
    }
    return status < 0 ? -1 : 0;
}

/* The transaction-record forms of the call preparations. */
static int
call_record_refresh(_PyCallTransaction *call)
{
    return call_transaction_refresh(call->tstate, call->frame, call->current);
}

static int
call_record_expand_method(_PyCallTransaction *call)
{
    call->state |= CALL_PREPARED;
    call->state = call_transaction_expand_method(
        call->tstate, call->frame, call->current, call->base, call->state);
    return call->state & CALL_FAILED ? -1 : 0;
}

static int
call_record_prepare_tuple(_PyCallTransaction *call)
{
    assert(call->kind == CALL_TRANSACTION_EX);
    call->state |= CALL_PREPARED;
    return call_transaction_prepare_tuple(call->tstate, call->frame,
                                          call->current, call->base);
}

static int
call_record_monitor(_PyCallTransaction *call)
{
    assert(!call->monitored);
    call->monitored = 1;
    return call_transaction_monitor(call->tstate, call->frame, call->current,
                                    call->base, call->argument_count,
                                    call->kind, call->state);
}

static void
call_transaction_publish_pc(void *opaque)
{
    _PyCallTransaction *call = opaque;
    assert(call->tstate->current_frame == call->frame);
#ifndef NDEBUG
    _PyCodeExec *exec = _PyFrame_GetCode(call->frame)->_co_exec;
    assert(exec != NULL && call->current >= exec->words &&
           call->current < exec->words + exec->nwords);
    assert(call->frame->ir_words == exec->words);
    assert(call->frame->ir_feedback == exec->entries);
#endif
    call->frame->instr_ptr = call->current;
}

static void
call_transaction_detach(_PyCallTransaction *call, PyValue *retired)
{
    PyValue *homes = _PyFrame_Slots(call->frame) + call->base;
    for (int i = 0; i < call->count; i++) {
        if ((i == 1 && !(call->state & CALL_SELF_PRESENT)) ||
            (i == 3 && call->kind == CALL_TRANSACTION_EX &&
             !(call->state & CALL_KWARGS_PRESENT))) {
            retired[i] = PyValue_NULL;
            continue;
        }
        retired[i] = homes[i];
        _PyFrame_WriteHome(call->frame, homes + i, PyValue_NULL);
    }
}

static void
call_transaction_retire(PyValue *retired, int count, PyValue *small)
{
    for (int i = count; i-- > 0;) {
        PyValue_CLEAR(retired[i]);
    }
    if (retired != small && retired != NULL) {
        PyMem_Free(retired);
    }
}

/* Result publication targets the captured destination itself when it is a
 * temporary, including an input overlap, before the window retires. A
 * folded LOCAL destination gets the result after the window retires and
 * the periodic check passes; on failure the result is left in window[0].
 * The producer must not move window[0] afterward, even on an error edge
 * after cleanup.
 *
 * The call is the value vectorcall (or the tuple call) on the window's
 * words: a callee with a value form sees an inline argument as it is, and
 * the call adapter of an object-form callee boxes it (pycore_value_call.h).
 * The window keeps its owners until the result is published. */
static int
call_transaction_native(_PyCallTransaction *call)
{
    PyValue small[8];
    PyValue *retired = small;
    PyValue local_result = PyValue_NULL;
    PyValue result = PyValue_NULL;
    PyValue displaced = PyValue_NULL;
    PyValue callable = PyValue_NULL;
    PyValue monitor_argument = PyValue_NULL;
    int retired_count = 0, call_status = -1, to_local = 0;
    if (call->instrumented && !call->monitored &&
        call_record_monitor(call) < 0) {
        return -1;
    }
    if (call->count > (int)Py_ARRAY_LENGTH(small)) {
        retired = PyMem_Malloc((size_t)call->count * sizeof(PyValue));
        if (retired == NULL) {
            PyErr_NoMemory();
        }
        if (retired == NULL || call_record_refresh(call) < 0) {
            goto done;
        }
    }
    PyValue *homes = _PyFrame_Slots(call->frame) + call->base;
    /* The call pins its callable: the window's owner can be replaced while
     * the callee runs. */
    callable = PyValue_DUP(homes[0]);
    if (call->instrumented) {
        PyValue first = PyValue_NULL;
        if (call->kind == CALL_TRANSACTION_EX) {
            PyObject *tuple = PyValue_AsKnownObjectBorrow(homes[2]);
            if (PyTuple_GET_SIZE(tuple)) {
                first = PyValue_DUP(PyTuple_GET_VALUE(tuple, 0));
            }
        }
        else {
            int offset = call->state & CALL_SELF_PRESENT ? 1 : 2;
            int total = call->argument_count + 2 - offset;
            if (total) {
                first = PyValue_DUP(homes[offset]);
            }
        }
        /* MISSING is immortal: the word needs no count. */
        monitor_argument = PyValue_IsNull(first)
            ? borrowed_word(&_PyInstrumentation_MISSING) : first;
    }
    if (call_record_refresh(call) < 0) {
        goto abort;
    }
    homes = _PyFrame_Slots(call->frame) + call->base;
    if (call->kind == CALL_TRANSACTION_EX) {
        PyObject *tuple = PyValue_AsKnownObjectBorrow(homes[2]);
        PyObject *kwargs = call->state & CALL_KWARGS_PRESENT
            ? PyValue_AsKnownObjectBorrow(homes[3]) : NULL;
        result = _PyValue_Call(call->tstate, callable, tuple, kwargs);
    }
    else {
        int offset = call->state & CALL_SELF_PRESENT ? 1 : 2;
        int total = call->argument_count + 2 - offset;
        PyObject *names = call->kind == CALL_TRANSACTION_KEYWORDS
            ? PyValue_AsKnownObjectBorrow(homes[call->count - 1]) : NULL;
        Py_ssize_t positional = total - (names == NULL ? 0 : PyTuple_GET_SIZE(names));
        /* An arena window grants argument reads, not a writable owner in
         * args[-1]. Native adapters supply their own prepend workspace. */
        size_t nargsf = (size_t)positional;
        result = _PyValue_VectorcallTstate(call->tstate, callable,
                                           homes + offset, nargsf, names);
    }
    /* The call can run Python: refresh before monitoring reads the frame. */
    int refreshed = call_record_refresh(call);
    int failed_call = PyValue_IsNull(result);
    if (call->instrumented &&
        !(call->kind == CALL_TRANSACTION_EX && PyValue_IsObject(callable) &&
          (PyFunction_Check(PyValue_AsKnownObjectBorrow(callable)) ||
           PyMethod_Check(PyValue_AsKnownObjectBorrow(callable))))) {
        if (failed_call || refreshed < 0) {
            _Py_call_instrumentation_exc2(
                call->tstate, PY_MONITORING_EVENT_C_RAISE,
                call->frame, call->frame->instr_ptr,
                callable, monitor_argument);
        }
        else if (_Py_call_instrumentation_2args(
                     call->tstate, PY_MONITORING_EVENT_C_RETURN,
                     call->frame, call->frame->instr_ptr,
                     callable, monitor_argument) < 0) {
            failed_call = 1;
        }
    }
    if (failed_call || refreshed < 0 ||
        call_record_refresh(call) < 0) {
        goto abort;
    }
    call_transaction_detach(call, retired);
    retired_count = call->count;
    // IR_NO_DST: the result waits in local_result until it is released
    to_local = call->destination < _PyFrame_GetCode(call->frame)->co_nlocalsplus
               || call->destination == IR_NO_DST;
    _PyValue_CheckHome(result);
    if (to_local) {
        local_result = result;
    }
    else {
        PyValue *home = _PyFrame_Slots(call->frame) + call->destination;
        _PyValue_RecheckHome(*home);
        displaced = *home;
        _PyFrame_WriteHome(call->frame, home, result);
    }
    result = PyValue_NULL;
    call_transaction_publish_pc(call);
    call_status = 0;
    goto done;
abort:
    PyValue_XCLOSE(result);
    call_transaction_publish_pc(call);
done:
    assert(retired_count == 0 || retired != NULL);
    PyValue_XCLOSE(displaced);
    if (!PyValue_IsObject(monitor_argument) ||
        PyValue_AsKnownObjectBorrow(monitor_argument) != &_PyInstrumentation_MISSING) {
        PyValue_XCLOSE(monitor_argument);
    }
    PyValue_XCLOSE(callable);
    if (retired != NULL) {
        call_transaction_retire(retired, retired_count, small);
    }
    int failed = call_record_refresh(call);
    if (call_status < 0) {
        assert(PyValue_IsNull(local_result));
        return -1;
    }
    if (failed == 0 && call->periodic) {
        int periodic = check_periodics_at_end(call->tstate,
                                              call->frame);
        failed = call_record_refresh(call);
        if (periodic < 0) {
            failed = -1;
        }
    }
    if (to_local) {
        /* Upstream closes a call's inputs, runs its periodic check and only
         * then stores the result with STORE_FAST. So a folded local gets
         * the result after the window retires and the check passes, as
         * IR_STORE_DST does in the specialised handlers, and its old value
         * goes last. If the check raised, the local keeps its old value and
         * the result waits in window[0], where the unwinder releases it after
         * the traceback entry and the RAISE event, as upstream pops it. */
        if (failed < 0) {
            PyValue *window = _PyFrame_Slots(call->frame) + call->base;
            assert(PyValue_IsNull(window[0]));
            _PyFrame_WriteHome(call->frame, window, local_result);
            return -1;
        }
        if (call->destination == IR_NO_DST) {
            PyValue_XCLOSE(local_result);   // may run __del__
            return call_record_refresh(call);
        }
        PyValue *home = _PyFrame_Slots(call->frame) + call->destination;
        // a move: replaced below
        PyValue displaced_local = *home;
        _PyFrame_WriteHome(call->frame, home, local_result);
        _PyValue_CheckHome(*home);
        PyValue_XCLOSE(displaced_local);
        failed = call_record_refresh(call);
    }
    return failed;
}

/* Exact calls from heap-owned frames cannot overlap thread-stack homes.
 * Move their arguments after initialization, preserving any constant homes
 * beyond the actual arguments. No owner is retired and no callback can run. */
static Py_NO_INLINE void
call_transfer_exact_args(_PyInterpreterFrame *dest, _PyInterpreterFrame *source,
                         PyValue *window, int nargs,
                         _PyCallSelfRole self_role)
{
    int has_self = self_role == CALL_SELF_SUPPLIED;
    if (has_self) {
        _PyFrame_MoveHome(source, window + 1, dest, dest->base);
    }
    for (int i = 0; i < nargs; i++) {
        _PyFrame_MoveHome(source, window + 2 + i, dest, dest->base + has_self + i);
    }
}

/* The handlers' calls (call_transaction_run_*), after
 * _PyEvalFramePushAndInit and its kin, which they use. */
static _PyCallResult call_transaction_run_vector(
    PyThreadState *, _PyInterpreterFrame *, int, int, int, int);
static inline _PyCallResult call_bind_known_python(
    PyThreadState *, _PyInterpreterFrame *, int, int, int,
    _PyCallTransactionKind, const _PyFbCall *, int);
static _PyCallResult call_transaction_run_keywords(
    PyThreadState *, _PyInterpreterFrame *, int, int, int, int);
static _PyCallResult call_transaction_run_ex(
    PyThreadState *, _PyInterpreterFrame *, int, int, int);

/* Publish result, the operation's value (to its slot if that is a
 * temporary), then close the operands first and second, then the boxes, and
 * refresh.  Returns the published result, or PyValue_NULL on error, when it
 * has closed result unless the slot already owns it.  All indices come from
 * the handler's word, as read before the provider: a folded destination
 * stays untouched until the original operand owners have retired. */
static PyValue
value_reader_complete(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *current, _PyValueTemps *temps,
                      int result_slot, PyValue result,
                      PyValue first, PyValue second, uint16_t phase)
{
    frame->phase_state = 0;
    if (!value_frame_is_current(tstate, frame)) {
        PyErr_SetString(PyExc_SystemError, "value reader changed the active frame");
        PyValue_XCLOSE(result);
        value_operation_close3(first, second, PyValue_NULL);
        _PyValueTemps_Close(temps);
        return PyValue_NULL;
    }
    int discard = result_slot == IR_NO_DST;
    int local = discard || result_slot < _PyFrame_GetCode(frame)->co_nlocalsplus;
    if (!discard && (result_slot < 0 ||
        result_slot >= _PyFrame_NumSlots(frame) ||
        (!local && !PyValue_IsNull(_PyFrame_Slots(frame)[result_slot])))) {
        PyErr_SetString(PyExc_SystemError, "value reader changed its result home");
        PyValue_XCLOSE(result);
        value_operation_close3(first, second, PyValue_NULL);
        _PyValueTemps_Close(temps);
        (void)value_operation_refresh(tstate, frame, current, PyValue_NULL);
        return PyValue_NULL;
    }
    result = _PyValue_Publish(&result);
    if (!local) {
        _PyFrame_WriteHome(frame, _PyFrame_Slots(frame) + result_slot, result);
        _PyValue_CheckTemp(_PyFrame_Slots(frame)[result_slot]);
    }
    frame->phase_state = phase;
    value_operation_close3(first, second, PyValue_NULL);
    _PyValueTemps_Close(temps);
    frame->phase_state = 0;
    if (value_operation_refresh(tstate, frame, current,
                                local ? result : PyValue_NULL) < 0) {
        return PyValue_NULL;
    }
    return result;
}

static int
finish_value_bridges(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *next, _PyValueTemps *temps)
{
    if (temps == NULL || temps->count == 0) {
        return 0;
    }
    return finish_value_bridges_ex(tstate, frame, next, temps, NULL, 0,
                                   PyValue_NULL);
}

/* value_reader_complete() for LOAD_ATTR, whose method form has two results,
 * the attribute and self, for result_slot and the slot after it.  Returns
 * the published attribute, or PyValue_NULL on error. */
static PyValue
value_attribute_complete(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         _PyInstr *current, _PyValueTemps *temps,
                         int result_slot, PyValue result, PyValue self,
                         PyValue receiver, int method)
{
    if (!method) {
        return value_reader_complete(tstate, frame, current, temps,
                                     result_slot, result, receiver,
                                     PyValue_NULL, 0);
    }
    int slot = result_slot;
    if (!value_frame_is_current(tstate, frame) ||
        slot < _PyFrame_GetCode(frame)->co_nlocalsplus ||
        slot + 1 >= _PyFrame_NumSlots(frame) ||
        !PyValue_IsNull(_PyFrame_Slots(frame)[slot]) ||
        !PyValue_IsNull(_PyFrame_Slots(frame)[slot + 1])) {
        int nlocals = _PyFrame_GetCode(frame)->co_nlocalsplus;
        int nslots = _PyFrame_NumSlots(frame);
        int occupied = slot >= nlocals && slot + 1 < nslots
            ? ((!PyValue_IsNull(_PyFrame_Slots(frame)[slot])) |
               (!PyValue_IsNull(_PyFrame_Slots(frame)[slot + 1]) << 1)) : -1;
        PyErr_Format(PyExc_SystemError,
                     "method lookup changed its output homes "
                     "(current=%d, dst=%d, locals=%d, slots=%d, "
                     "occupied=%d, pc=%zd)",
                     value_frame_is_current(tstate, frame), slot, nlocals,
                     nslots, occupied,
                     current - frame->ir_words);
        PyValue_XCLOSE(result);
        PyValue_XCLOSE(self);
        value_operation_close3(receiver, PyValue_NULL, PyValue_NULL);
        _PyValueTemps_Close(temps);
        (void)value_operation_refresh(tstate, frame, current, PyValue_NULL);
        return PyValue_NULL;
    }
    result = _PyValue_Publish(&result);
    self = _PyValue_Publish(&self);
    _PyFrame_WriteHome(frame, _PyFrame_Slots(frame) + slot, result);
    _PyFrame_WriteHome(frame, _PyFrame_Slots(frame) + slot + 1, self);
    _PyValue_CheckTemp(_PyFrame_Slots(frame)[slot]);
    _PyValue_CheckTemp(_PyFrame_Slots(frame)[slot + 1]);
    value_operation_close3(receiver, PyValue_NULL, PyValue_NULL);
    _PyValueTemps_Close(temps);
    if (value_operation_refresh(tstate, frame, current, PyValue_NULL) < 0) {
        return PyValue_NULL;
    }
    return result;
}

/* Release first, then second, the values that a handler's published result
 * displaced or whose receiver it no longer needs, then the input boxes, and
 * refresh.  Both are detached before any callback, and no raw field pointer
 * crosses it.  On failure, also close failed_result, the handler's result
 * that a local was to receive.  Out of line: the owners array is this
 * function's (R-a, R-c). */
static Py_NO_INLINE int
retire_published_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *next, _PyValueTemps *temps,
                        PyValue first, PyValue second, PyValue failed_result)
{
    PyValue owners[2] = {first, second};
    return finish_value_bridges_ex(tstate, frame, next, temps, owners, 2,
                                   failed_result);
}

/* Release the local that a handler's result displaced (IR_STORE_DST),
 * then its input boxes, and refresh: the release can run Python.  Out of
 * line, so that the address of displaced is taken in this function's frame
 * and not in the handler's, which would then need a frame and a canary on
 * every entry (register-ir-v2-a3.md, 3.4). */
static Py_NO_INLINE int
finish_displaced_local(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *next, _PyValueTemps *temps,
                       PyValue displaced)
{
    return finish_value_bridges_ex(tstate, frame, next, temps, &displaced, 1,
                                   PyValue_NULL);
}

/* The rest of IR_UNPACK_TO_LOCALS(): store the elements from index first on,
 * releasing each local's old value before the next is stored, as the
 * STORE_FAST sequence does, and refreshing after each release, which can run
 * Python.  Out of line for the address of the old value (as
 * finish_displaced_local) and for unpacks of many elements. */
static Py_NO_INLINE int
finish_unpack_to_locals(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *next, _PyValueTemps *temps,
                        PyValue *locals, PyValue *window, int n, int first,
                        int canonical)
{
    frame->phase_state = 0;
    int err = 0;
    for (int i = first; i < n; i++) {
        PyValue *element = &window[n - 1 - i];
        if (err < 0) {
            PyValue rest = *element;
            _PyFrame_WriteHome(frame, element, PyValue_NULL);
            PyValue_XCLOSE(rest);
            continue;
        }
        PyValue old = locals[i];
        assert(!canonical || _PyValue_IsHome(*element));
        _PyFrame_WriteHome(frame, locals + i,
                           canonical ? *element : _PyValue_Publish(element));
        _PyValue_CheckHome(locals[i]);
        _PyFrame_WriteHome(frame, element, PyValue_NULL);
        err = finish_value_bridges_ex(tstate, frame, next, temps, &old, 1,
                                      PyValue_NULL);
    }
    return err;
}

typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    PyCodeObject *code;
    Py_ssize_t current_word;
    Py_ssize_t next_word;
    int base;
    int inputs;
    int roles;
    PyValue *result;
} _PyNativeCallPublication;

static _PyNativeCallPublication
native_call_publication(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *next, int base, int inputs, int roles,
                        PyValue *result)
{
    _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
    assert(exec != NULL && frame->ir_words == exec->words);
    return (_PyNativeCallPublication){
        tstate, frame, _PyFrame_GetCode(frame),
        frame->instr_ptr - exec->words, next - exec->words,
        base, inputs, roles, result,
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
    assert(frame->ir_words == exec->words);
    assert(frame->ir_feedback == exec->entries);
    frame->instr_ptr = exec->words + state->current_word;
}

static void
native_call_publish_result(void *opaque)
{
    _PyNativeCallPublication *state = opaque;
    native_call_publish_pc(state);
    PyValue *window = _PyFrame_Slots(state->frame) + state->base;
    call_clear_owned_slots(state->frame, window, 0, state->inputs,
                           CALL_TRANSACTION_VECTOR, state->roles);
    _PyFrame_WriteHome(state->frame, window, *state->result);
}

static int
native_call_finish(_PyNativeCallPublication *state, _PyNativeScope *scope,
                   PyValue *argument, PyValue *callable, _PyInstr *next)
{
    _PyNativeScope_End(scope);
    if (argument != NULL) {
        PyValue_CLEAR(*argument);
        PyValue_CLEAR(*callable);
    }
    _PyInterpreterFrame *frame = state->frame;
    if (state->tstate->current_frame != frame ||
        _PyFrame_GetCode(frame) != state->code) {
        value_revalidation_error("native cleanup changed the active frame");
        return -1;
    }
    /* As in value_operation_refresh, the executing copy is fixed. */
    assert(instrumentation_is_current(state->tstate, state->code));
    _PyCodeExec *exec = state->code->_co_exec;
    if (exec == NULL ||
        state->current_word < 0 || state->current_word >= exec->nwords ||
        state->next_word < 0 || state->next_word > exec->nwords) {
        /* Keep a pending call error. */
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "native cleanup changed executing words");
        }
        return -1;
    }
    assert(frame->ir_words == exec->words);
    assert(frame->ir_feedback == exec->entries);
    assert(next == exec->words + state->next_word);
    (void)next;
    frame->instr_ptr = exec->words + state->current_word;
    return 0;
}

/* CALL_BUILTIN_O's native call, out of line (spec 3.5, R-c): the scope, its
 * result and the publication record are addressed by the runtime API and by
 * the publish callbacks, so they live in this function's own frame, not the
 * handler's.  Returns the published result, or PyValue_NULL with the error
 * set; on failure the inputs stay in the window (the frame owns them). */
static Py_NO_INLINE PyValue
native_call_builtin_o(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *next, int base, int inputs,
                      PyValue callable, PyValue *args, int roles)
{
    PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
    _PyNativeScope scope;
    PyValue pending = PyValue_NULL;
    _PyNativeCallPublication publication = native_call_publication(
        tstate, frame, next, base, inputs, roles, &pending);
    int status = _PyNativeScope_Begin(
        &scope, tstate, args, 1, 1, PyValue_NULL, NULL);
    if (status == 0) {
        status = _PyCFunction_CallScoped(&scope, callable_o, &scope.result);
    }
    if (status < 0) {
        _PyNativeResult_Abort(&scope.result, native_call_publish_pc, &publication);
        (void)native_call_finish(&publication, &scope, NULL, NULL, next);
        return PyValue_NULL;
    }
    PyValue c = callable;
    PyValue s = args[0];  // a move: the consumed input, closed below
    _PyNativeResult_Publish(&scope.result, &pending,
                            native_call_publish_result, &publication);
    if (native_call_finish(&publication, &scope, &s, &c, next) < 0) {
        return PyValue_NULL;
    }
    return pending;
}

/* The handlers, after the helpers that they call, and their table */
#include "ir_targets.h"
#include "generated_ir_cases.c.h"

/* The only non-tail call into the handlers.  GCC 15.2 miscompiles a call
   to a no_callee_saved_registers function that has a value live across it,
   at -Og to -O3: it reloads the value from a callee-saved register that it
   never set (reproducer: build/pyjit-link-wave1/a3/gcc-ncsr-bug.c in the
   pyjit build tree).  This function has the normal ABI and nothing live
   across the call, so it saves the callee-saved registers itself, and the
   caller keeps its values across a normal call.  It is not inlined, and the
   call is not a tail call: a handler does not preserve the registers that
   our own caller expects.  Clang's preserve_none is not known to have the
   bug; both compilers take this path, so there is one form.  The object
   check (a3/step2/objcheck.sh) refuses any other non-tail call to a
   _TAIL_CALL_* function. */
static Py_NO_INLINE PyValue
enter_handlers(py_tail_call_funcptr handler, TAIL_CALL_PARAMS)
{
    return handler(TAIL_CALL_ARGS);
}

void
_PyEval_RecordSourceError(PyThreadState *tstate, _PyInterpreterFrame *frame,
                          _PyExceptionHandler handler)
{
    _PyEvalActivation *activation = ((_PyThreadStateImpl *)tstate)->eval_activation;
    assert(activation != NULL);
    activation->source_error_handler = handler;
    activation->source_error_frame = frame;
}

void
_PyEval_InvalidateSourceErrors(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyEvalActivation *activation = ((_PyThreadStateImpl *)tstate)->eval_activation;
    for (; activation != NULL; activation = activation->previous) {
        if (activation->source_error_frame == frame) {
            activation->source_error_frame = NULL;
            activation->source_error_handler = (_PyExceptionHandler){0};
        }
    }
}

void
_PyEval_EnterObserverContext(PyThreadState *tstate,
                              const _PyIRObserverSpec *context,
                              _PyIRObserverScope *scope)
{
    scope->activation = ((_PyThreadStateImpl *)tstate)->eval_activation;
    scope->previous = NULL;
    if (scope->activation != NULL) {
        scope->previous = scope->activation->observer_context;
        scope->activation->observer_context = context;
    }
}

void
_PyEval_LeaveObserverContext(PyThreadState *tstate, _PyIRObserverScope *scope)
{
    assert(scope->activation == ((_PyThreadStateImpl *)tstate)->eval_activation);
    if (scope->activation != NULL) {
        scope->activation->observer_context = scope->previous;
    }
}

int
_PyEval_GetObserverContext(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            int event, _PyIRObserverSpec *result)
{
    _PyThreadStateImpl *impl = (_PyThreadStateImpl *)tstate;
    for (_PyEvalActivation *activation = impl->eval_activation;
         activation != NULL; activation = activation->previous)
    {
        const _PyIRObserverSpec *context = activation->observer_context;
        if (context == NULL) {
            continue;
        }
        if (context->phase == _PY_IR_OBSERVER_UNSUPPORTED ||
            context->frame != frame || impl->observed_frame != frame ||
            context->event != event || tstate->what_event != event)
        {
            return 0;
        }
        *result = *context;
        return 1;
    }
    return 0;
}

/* Only external evaluator entry and explicit custom-evaluator handoffs use
 * this lookup. Empty provider nodes survive until their outer C scope exits. */
static Py_NO_INLINE bool
handoff_staged_frame(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    for (_PyTransitionRoots *roots =
             ((_PyThreadStateImpl *)tstate)->transition_roots;
         roots != NULL; roots = roots->previous)
    {
        if (roots->kind == _Py_TRANSITION_KIND_STAGED && !roots->caller_staged &&
            roots->frame != NULL && roots->frame == frame)
        {
            assert(!roots->result_present);
            roots->remaining = 0;
            roots->header_mask = 0;
            roots->home_authority = _PyIR_HOME_NONE;
            roots->homes = NULL;
            roots->frame = NULL;
            return true;
        }
    }
    return false;
}

int
_PyEval_ActivateStagedFrame(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    if (tstate->current_frame != frame || !handoff_staged_frame(tstate, frame)) {
        PyErr_SetString(PyExc_SystemError, "no live staged frame to activate");
        return -1;
    }
    return 0;
}

/* The evaluation loop.  _PyEval_EvalFrameDefault() wraps each activation in
   its borrow floor.  The loop sets up the activation and calls the first
   handler, and the handlers run the frames by tail calls until one of them
   returns.  It keeps an external name so that debuggers and profilers
   (Tools/gdb/libpython.py) can find its frames. */
PyValue _PyEval_EvalFrameLoop(PyThreadState *tstate,
                              _PyInterpreterFrame *frame, int throwflag);

Py_NO_INLINE PyValue _Py_HOT_FUNCTION DONT_SLP_VECTORIZE
_PyEval_EvalFrameLoop(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    check_invalid_reentrancy();
    CALL_STAT_INC(pyeval_calls);

#ifdef Py_STATS
    int lastopcode = 0;
#endif

    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        uint8_t reason = _PyIR_RETIRE_ERROR_UNWIND;
        if (((_PyThreadStateImpl *)tstate)->transition_roots != NULL &&
            handoff_staged_frame(tstate, frame))
        {
            reason = _PyIR_RETIRE_STAGED_FAILURE;
        }
        /* Handoff to retirement has no callback or poll before its roots. */
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
        _PyEval_FrameClearAndPop(tstate, frame, reason);
        return PyValue_NULL;
    }

    /* The activation's record, linked from the thread state until it
     * returns.  It lies on this C stack between the stack pointer and the
     * record of the activation that this one runs inside. */
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    _PyEvalActivation activation;
    activation.previous = tstate_impl->eval_activation;
    activation.source_error_frame = NULL;
    activation.source_error_handler = (_PyExceptionHandler){0};
    activation.observer_context = NULL;
    tstate_impl->eval_activation = &activation;
#if _Py_STACK_GROWS_DOWN
    assert(_Py_get_machine_stack_pointer() <= (uintptr_t)&activation);
    assert(activation.previous == NULL ||
           (uintptr_t)&activation < (uintptr_t)activation.previous);
#else
    assert(_Py_get_machine_stack_pointer() >= (uintptr_t)&activation);
    assert(activation.previous == NULL ||
           (uintptr_t)&activation > (uintptr_t)activation.previous);
#endif
    _PyEntryFrame *entry = &activation.entry;

    entry->stack[0] = PyValue_NULL;
#if defined(Py_DEBUG)
    /* Set these to invalid but identifiable values for debugging. */
    entry->frame.f_locals = (PyObject*)0xaaa1;
    entry->frame.frame_obj = (PyFrameObject*)0xaaa2;
    entry->frame.f_globals = (PyObject*)0xaaa3;
    entry->frame.f_builtins = (PyObject*)0xaaa4;
#endif
    entry->frame.f_executable = NULL;
    entry->frame.f_funcobj = NULL;
    entry->frame.base = entry->frame.owned_slots;
    entry->frame.presence_bits = NULL; /* Native entry homes have no arena bits. */
    /* Returns and yields resume the explicit real INTERPRETER_EXIT target. */
    entry->frame.instr_ptr = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry->frame.ir_words = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry->frame.ir_feedback = NULL;
    entry->frame.owner = FRAME_OWNED_BY_INTERPRETER;
    entry->frame.visited = 0;
    /* What the frame returns or yields lands in entry->stack[0] */
    entry->frame.return_slot = 1;
    entry->frame.return_pc = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry->frame.pending_kwargs = IR_NO_DST;
    entry->frame.pending_redirect = 0;
    entry->frame.phase_state = 0;
#ifdef Py_DEBUG
    entry->frame.lltrace = 0;
#endif
    /* Push frame */
    entry->frame.previous = tstate->current_frame;
    frame->previous = &entry->frame;
    tstate->current_frame = frame;
    if (tstate_impl->transition_roots != NULL) {
        (void)handoff_staged_frame(tstate, frame);
    }

    PyValue result;
    if (throwflag) {
        /* support for generator.throw() */
        if (_Py_EnterRecursivePy(tstate)) {
            goto early_exit;
        }
        /* Because this avoids the RESUME, we need to update instrumentation */
        _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
        _PyInstr *next_instr = frame->instr_ptr;
        monitor_throw(tstate, frame, next_instr);
        PyValue *slots = _PyFrame_Slots(frame);
        _PyIRWord ir_word = 0;
        result = enter_handlers(_TAIL_CALL_error, TAIL_CALL_ARGS);
    }
    else {
        /* start_frame takes its word and slots from the frame */
        PyValue *slots = NULL;
        _PyInstr *next_instr = NULL;
        _PyIRWord ir_word = 0;
        result = enter_handlers(_TAIL_CALL_start_frame, TAIL_CALL_ARGS);
    }
    /* The handler that returned, INTERPRETER_EXIT or exit_unwind_notrace,
     * left the entry frame */
    assert(tstate_impl->eval_activation == &activation);
    tstate_impl->eval_activation = activation.previous;
    return result;

early_exit:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame = dying->previous;
    _PyEval_FrameClearAndPop(tstate, dying, _PyIR_RETIRE_ERROR_UNWIND);
    assert(frame == &entry->frame);
    /* Restore previous frame and exit */
    tstate->current_frame = frame->previous;
    tstate_impl->eval_activation = activation.previous;
    return PyValue_NULL;
}


#ifdef DO_NOT_OPTIMIZE_INTERP_LOOP
#  pragma optimize("", on)
#endif

/* Each activation of the evaluation loop is a borrow scope with a floor
 * (pyjit/design/native-boundary.md, section 2).
 *
 * The release invariant: when _Py_HandlePending() releases the entries above
 * eval_floor, no live C frame between the floor and the current instruction
 * holds a mark or a borrow.  Every caller of _Py_HandlePending() is at an
 * instruction boundary of this activation, where all native code it called
 * has returned.  So the floor is set here, around the loop, and not in
 * _PyEval_EvalFrame(): a PEP 523 hook runs C code between the two, and
 * anything it pushed would be released while still live. */
static PyValue
eval_frame_default_value(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    _PyBorrowStack *borrow = &((_PyThreadStateImpl *)tstate)->borrow;
    PyObject **outer_floor = borrow->eval_floor;
    /* Entries below this activation's floor can only have requested a
       release before it started, so the only request of theirs that its
       instruction boundaries can consume is one pending now. */
    int outer_request = _Py_eval_breaker_bit_is_set(tstate,
                                                    _PY_BORROW_RELEASE_BIT);
    PyObject **floor = borrow->top;
    borrow->eval_floor = floor;
    PyValue result = _PyEval_EvalFrameLoop(tstate, frame, throwflag);
    assert(borrow->eval_floor == floor);
    _PyBorrowScope_Exit(tstate, floor);
    borrow->eval_floor = outer_floor;
    if (outer_request && outer_floor != NULL && borrow->top != outer_floor) {
        /* Renew the request that this activation may have consumed.  Entries
           that requested nothing stay unrequested: a live argument vector
           sits above the outer floor during every callback from a builtin,
           and its own scope releases it. */
        _PyBorrow_RequestRelease(tstate);
    }
    return result;
}

/* The evaluation loop for callers that take the value of the frame as it is
 * (the value vectorcall of a Python function, generators).  A PEP 523 hook
 * returns an object, which it steals back. */
PyValue
_PyEval_EvalFrameValue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       int throwflag)
{
    if (tstate->interp->eval_frame == NULL) {
        EVAL_CALL_STAT_INC(EVAL_CALL_TOTAL);
        return eval_frame_default_value(tstate, frame, throwflag);
    }
    return _PyValue_FromObjectResult(_PyEval_EvalFrame(tstate, frame, throwflag));
}

/* The exported evaluation loop and the PEP 523 default: an object result,
 * which boxes an inline value. */
PyObject *
_PyEval_EvalFrameDefault(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    PyValue result = eval_frame_default_value(tstate, frame, throwflag);
    if (PyValue_IsNull(result)) {
        return NULL;
    }
    PyObject *object = PyValue_AsObject(result);
    PyValue_CLOSE(result);
    return object;
}

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
        name_str = PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(names, 0));
        Py_INCREF(name_str);
        break;
    case 2:
        name_str = PyUnicode_FromFormat("%U and %U",
                                        PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(names, len - 2)),
                                        PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(names, len - 1)));
        break;
    default:
        tail = PyUnicode_FromFormat(", %U, and %U",
                                    PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(names, len - 2)),
                                    PyValue_AsKnownObjectBorrow(PyList_GET_VALUE(names, len - 1)));
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
            PyObject *name = _PyValue_Repr(PyValue_FromObjectBorrow(raw));
            if (name == NULL) {
                Py_DECREF(missing_names);
                return;
            }
            PyList_SET_VALUE(missing_names, j++, PyValue_PrepareOwned(name));
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
            PyValueBorrow kwvalue = PyTuple_GET_VALUE(kwnames, k2);
            if (!PyValue_IsObject(kwvalue)) {
                continue;
            }
            PyObject* kwname = PyValue_AsKnownObjectBorrow(kwvalue);
            if (kwname == posonly_name){
                if(PyList_AppendValue(posonly_names, PyValue_FromObjectBorrow(kwname)) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
                continue;
            }

            int cmp = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(posonly_name), PyValue_FromObjectBorrow(kwname), Py_EQ);

            if ( cmp > 0) {
                if(PyList_AppendValue(posonly_names, PyValue_FromObjectBorrow(kwname)) != 0) {
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
initialize_locals(PyThreadState *tstate, _PyInterpreterFrame *frame,
    PyFunctionObject *func, PyCodeObject *co, PyValue const *args,
    Py_ssize_t argcount, PyObject *kwnames)
{
    PyValue *localsplus = frame->base;
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
        _PyFrame_WriteHome(frame, localsplus + i, PyValue_FromObjectSteal(kwdict));
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
        _PyFrame_WriteHome(frame, localsplus + j, args[j]);
        _PyValue_CheckHome(localsplus[j]);
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
        _PyFrame_WriteHome(frame, localsplus + total_args, PyValue_FromObjectSteal(u));
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
            PyValueBorrow keyword_value = PyTuple_GET_VALUE(kwnames, i);
            PyObject *keyword = PyValue_IsObject(keyword_value)
                ? PyValue_AsKnownObjectBorrow(keyword_value) : NULL;
            // a move: closed or stored below
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
                PyObject *varname = PyValue_AsKnownObjectBorrow(co_varnames[j]);
                if (varname == keyword) {
                    goto kw_found;
                }
            }

            /* Slow fallback, just in case */
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = PyValue_AsKnownObjectBorrow(co_varnames[j]);
                int cmp = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(keyword), PyValue_FromObjectBorrow(varname), Py_EQ);
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
                                             PyValue_XDUP(co_varnames[k]));
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

            int err = PyDict_SetValue(kwdict, PyValue_FromObjectBorrow(keyword),
                                      value_value);
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
            _PyFrame_WriteHome(frame, localsplus + j, value_value);
            _PyValue_CheckHome(localsplus[j]);
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
                    _PyFrame_WriteHome(frame, localsplus + m + i, PyValue_XDUP(defs[i]));
                    _PyValue_CheckHome(localsplus[m+i]);
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
                PyValue def;
                int found = PyDict_GetValue(func->func_kwdefaults,
                                            PyValue_FromObjectBorrow(varname),
                                            &def);
                if (found < 0) {
                    goto fail_post_args;
                }
                if (found > 0) {
                    _PyFrame_WriteHome(frame, localsplus + i, def);
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

/* This continuation is reached only when a remaining release may reenter.
 * Its record protects the result and retiring reservation through the final
 * code/exception close, whose referents can themselves invoke Python. */
static Py_NO_INLINE PyValue
clear_thread_frame_slow(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        int stage, PyValue result, uint8_t reason,
                        int initial_extent)
{
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
    PyValue *saved_top = frame->saved_top;
    assert((PyObject **)frame == limit);
    assert(frame->base >= _PyDataStack_Slots(&chunk->data[0]));
    assert(frame->base <= _PyDataStack_Slots(top));
#endif
    _PyTransitionRoots roots;
    _PyFrame_InitClearRoots(frame, stage, initial_extent, result, &roots);
    roots.retire_reason = reason;
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    roots.shadow_status = _PyIR_ShadowRetirement(frame, reason);
    assert(roots.shadow_status != _PyIR_SHADOW_MISMATCH);
    if (stage != _Py_FRAME_CLEAR_DONE) {
        _PyFrame_FinishClearExceptCode(frame, stage, &roots);
    }
    assert(roots.remaining == 0);
    roots.header_mask &= ~_Py_TRANSITION_CODE;
    Py_CLEAR(frame->f_executable);
#ifndef NDEBUG
    /* Nested calls may grow the arena, but must restore this reservation. */
    assert(tstate->datastack_chunk == chunk);
    assert(tstate->datastack_top == top);
    assert(tstate->datastack_limit == limit);
    assert(frame->saved_top == saved_top);
#endif
    result = roots.result;
    roots.result = PyValue_NULL;
    roots.result_present = 0;
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}

static PyValue
clear_thread_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyValue result, uint8_t reason)
{
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    assert((PyObject **)frame == tstate->datastack_limit);
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    int stage = _PyFrame_TryClearExceptCode(frame);
    if (stage != _Py_FRAME_CLEAR_DONE ||
        _PyFrame_ObjectMayReenter((PyObject *)frame->f_executable))
    {
        return clear_thread_frame_slow(tstate, frame, stage, result, reason,
                                      _PyFrame_GetCode(frame)->co_nlocalsplus);
    }
    Py_CLEAR(frame->f_executable);
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}

static PyValue
clear_gen_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                PyValue result, uint8_t reason)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    frame->previous = NULL;
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    result = _PyFrame_ClearGenerator(gen, result, reason);
    // No escaping call may follow this store before evaluator return.
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
    return result;
}

static inline Py_ALWAYS_INLINE _PyIRStepResult
complete_step_impl(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, int base, int destination,
                   _PyInstr *exit, PyValue value, enum _PyIRStepKind kind,
                   bool canonical_input)
{
    if (canonical_input) {
        assert(_PyValue_IsHome(value));
    }
    else {
        value = _PyValue_Publish(&value);
    }
    PyValue *window = frame->base + base;
    PyValue receiver = window[0];
    PyCodeObject *code = _PyFrame_GetCode(frame);
    frame->instr_ptr = site;
    if (code->_co_monitoring != NULL &&
        kind != _PY_IR_SEND_THROW && PyValue_IsObject(receiver)) {
        PyObject *object = PyValue_AsKnownObjectBorrow(receiver);
        if (PyGen_CheckExact(object) || PyCoro_CheckExact(object)) {
            _PyIRStepResult observed = _PyIR_ObserveStep(
                tstate, frame, site, exit, PyValue_Borrow(value), kind, 0);
            if (observed.status < 0) {
                PyObject *error = PyErr_GetRaisedException();
                assert(error != NULL);
                PyValue_XCLOSE(value);
                PyErr_SetRaisedException(error);
                observed.next = frame->instr_ptr + 1;
                return observed;
            }
            if (observed.status == _PY_IR_STEP_REDIRECT) {
                PyValue_XCLOSE(value);
                observed.next = frame->instr_ptr;
                return observed;
            }
        }
    }

    /* A nonredirecting PRE callback cannot move the active frame or mutate
     * its scratch window. Its homes anchor receiver throughout observation;
     * a redirect returned above, before any cached input is reused.
     *
     * Commit the outcome without callbacks. A SEND destination may overlap
     * any input; detach the whole window before publishing it. */
    PyValue cursor = window[1];
    PyValue sent = kind != _PY_IR_FOR_ITER ? window[2] : PyValue_NULL;
    PyValue exception = kind == _PY_IR_SEND_THROW ? window[3] : PyValue_NULL;
    _PyFrame_WriteHome(frame, window, PyValue_NULL);
    _PyFrame_WriteHome(frame, window + 1, PyValue_NULL);
    if (kind != _PY_IR_FOR_ITER) {
        _PyFrame_WriteHome(frame, window + 2, PyValue_NULL);
    }
    if (kind == _PY_IR_SEND_THROW) {
        _PyFrame_WriteHome(frame, window + 3, PyValue_NULL);
    }
    PyValue displaced = PyValue_NULL;
    if (kind != _PY_IR_FOR_ITER && destination != IR_NO_DST) {
        assert(destination < _PyFrame_NumSlots(frame));
        displaced = frame->base[destination];
        assert(destination < _PyFrame_GetCode(frame)->co_nlocalsplus ||
               PyValue_IsNull(displaced));
        _PyFrame_WriteHome(frame, frame->base + destination, value);
        _PyValue_CheckHome(value);
        value = PyValue_NULL;
    }
    frame->return_pc = exit;
    _PyIRStepResult observed = {exit, _PY_IR_STEP_EXIT};
    if (code->_co_monitoring != NULL) {
        observed = _PyIR_ObserveStep(
            tstate, frame, site, exit, PyValue_Borrow(PyValue_None), kind, 1);
    }
    /* POST callbacks run at the semantic operation for exception attribution.
     * Once they succeed, finalizers see its completed continuation. */
    if (observed.status >= 0) {
        frame->instr_ptr = observed.next;
    }
    PyObject *error = observed.status < 0 ? PyErr_GetRaisedException() : NULL;
    assert(observed.status >= 0 || error != NULL);
    PyValue_XCLOSE(exception);
    PyValue_XCLOSE(sent);
    PyValue_XCLOSE(cursor);
    PyValue_XCLOSE(receiver);
    PyValue_XCLOSE(displaced);
    PyValue_XCLOSE(value);
    if (observed.status < 0) {
        PyErr_SetRaisedException(error);
        observed.next = frame->instr_ptr + 1;
        return observed;
    }
    observed.next = frame->instr_ptr;
    return observed;
}

_PyIRStepResult
_PyEval_CompleteStep(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *site, int base, int destination,
                     _PyInstr *exit, PyValue value, enum _PyIRStepKind kind)
{
    /* Native completions still need publication; generator returns arrive
     * from the emitter's checked canonical input or its Publish fallback. */
    return complete_step_impl(tstate, frame, site, base, destination, exit,
                              value, kind, false);
}

_PyIRStepResult
_PyEval_CompleteGeneratorReturn(PyThreadState *tstate,
                                _PyInterpreterFrame *frame, PyValue value)
{
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        _PyFrame_SetReturnValue(frame, value);
        return (_PyIRStepResult){frame->return_pc, _PY_IR_STEP_EXIT};
    }
    _PyInstr *site = frame->instr_ptr;
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int offset = (int)_PyFrame_InstrOffset(frame, site);
    assert(offset >= 0 && offset < Py_SIZE(code));
    /* SEND/FOR completion uses the immutable A/B/signed-D layout, including
     * when the executing site has a transparent instrumentation wrapper. */
    _PyIRWord word = code->co_words[offset];
    int opcode = _PyIR_HandlerBase[IR_HANDLER(word)];
    assert(opcode == SEND || opcode == FOR_ITER);
    _PyInstr *exit = frame->return_pc + IR_DS(word);
    if (opcode == SEND) {
        return complete_step_impl(tstate, frame, site, IR_B(word), IR_A(word),
                                  exit, value, _PY_IR_SEND, true);
    }
    return complete_step_impl(tstate, frame, site, IR_B(word), IR_A(word),
                              exit, value, _PY_IR_FOR_ITER, true);
}

static PyValue
frame_clear_and_pop(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue result, uint8_t reason)
{
    // By this point, tstate->current_frame is already set to the parent frame.
    _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        return clear_thread_frame(tstate, frame, result, reason);
    }
    return clear_gen_frame(tstate, frame, result, reason);
}

PyValue
_PyEval_FrameClearAndPopWithResult(PyThreadState *tstate,
                                  _PyInterpreterFrame *frame, PyValue result)
{
    assert(!PyValue_IsNull(result));
    return frame_clear_and_pop(tstate, frame, result, _PyIR_RETIRE_NORMAL_RETURN);
}

Py_NO_INLINE PyValue
_PyEval_FrameClearAndPopGeneralWithResult(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame,
                                         PyValue result)
{
    assert(!PyValue_IsNull(result));
    if (frame->owner != FRAME_OWNED_BY_THREAD) {
        return frame_clear_and_pop(tstate, frame, result,
                                   _PyIR_RETIRE_NORMAL_RETURN);
    }
    assert((PyObject **)frame == tstate->datastack_limit);
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
    int extent = _PyFrame_NumSlots(frame);
    int stage = _PyFrame_TryClearExceptCodeGeneral(frame);
    if (stage != _Py_FRAME_CLEAR_DONE ||
        _PyFrame_ObjectMayReenter((PyObject *)frame->f_executable))
    {
        return clear_thread_frame_slow(tstate, frame, stage, result,
                                      _PyIR_RETIRE_NORMAL_RETURN, extent);
    }
    Py_CLEAR(frame->f_executable);
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}

void
_PyEval_FrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        uint8_t reason)
{
    (void)frame_clear_and_pop(tstate, frame, PyValue_NULL, reason);
}

int
_PyEval_RetireStagedFrame(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    /* Match the token before reading storage which a prior consumer may have
     * released. Missing-token errors must not inspect the supplied pointer. */
    if (tstate->current_frame == frame || !handoff_staged_frame(tstate, frame)) {
        PyErr_SetString(PyExc_SystemError, "no live staged frame to retire");
        return -1;
    }
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    PyObject *error = _PyErr_GetRaisedException(tstate);
    _PyEval_FrameClearAndPop(tstate, frame, _PyIR_RETIRE_STAGED_FAILURE);
    _PyErr_SetRaisedException(tstate, error);
    return 0;
}

/* A custom evaluator can return a result or error without consuming its input.
 * The outer provider still owns that reservation; the node decides, not the
 * contents of frame storage which a delegating evaluator may already free. */
static void
finish_staged_provider(PyThreadState *tstate, _PyTransitionRoots *staged,
                       _PyInterpreterFrame *previous)
{
    assert(tstate->current_frame == previous);
    if (staged->frame != NULL) {
        int status = _PyEval_RetireStagedFrame(tstate, staged->frame);
        assert(status == 0);
        (void)status;
    }
    _PyThreadState_PopTransitionRoots(tstate, staged);
}

/* Binding owns initialized arena homes before the child is current. The
 * outlined caller keeps this descriptor through all later provider cleanup. */
static void
stage_binding_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    _PyTransitionRoots *staged)
{
    *staged = (_PyTransitionRoots){
        .frame = frame,
        .remaining = _PyFrame_NumSlots(frame),
        .home_authority = _PyIR_HOME_FRAME_PRESENCE,
        .kind = _Py_TRANSITION_KIND_STAGED,
        /* A custom evaluator may install nullable headers before handoff. */
        .header_mask = _Py_TRANSITION_CODE | _Py_TRANSITION_FUNCTION |
            _Py_TRANSITION_LOCALS | _Py_TRANSITION_FRAME_OBJECT,
    };
    _PyThreadState_PushTransitionRoots(tstate, staged);
}

static void
unstage_binding_frame(PyThreadState *tstate, _PyTransitionRoots *staged)
{
    staged->remaining = 0;
    staged->header_mask = 0;
    staged->home_authority = _PyIR_HOME_NONE;
    staged->homes = NULL;
    staged->frame = NULL;
    _PyThreadState_PopTransitionRoots(tstate, staged);
}

/* code is independently pinned through preparation and binding. Consumes
 * func, locals and args, but never reselects the function's mutable code. */
static _PyInterpreterFrame *
eval_frame_push_and_init_code(PyThreadState *tstate, PyValue func,
                        PyObject *locals, PyValue *args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous, PyCodeObject *code,
                        _PyTransitionRoots *staged)
{
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    CALL_STAT_INC(frames_pushed);
    if (_PyCode_GetExec(code) == NULL) {
        goto fail;
    }
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
    if (frame == NULL) {
        PyErr_NoMemory();
        goto fail;
    }
    _PyFrame_Initialize(tstate, frame, func, locals, code, frame->base, previous);
    stage_binding_frame(tstate, frame, staged);
    if (initialize_locals(tstate, frame, func_obj, code, args, argcount, kwnames)) {
        assert(frame->owner == FRAME_OWNED_BY_THREAD);
        _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
        /* Transfer to retirement without a callback or poll. */
        unstage_binding_frame(tstate, staged);
        (void)clear_thread_frame(tstate, frame, PyValue_NULL,
                                _PyIR_RETIRE_STAGED_FAILURE);
        return NULL;
    }
    return frame;
fail:
    /* Consume the references */
    PyValue_CLOSE(func);
    Py_XDECREF(locals);
    for (size_t i = 0; i < argcount; i++) {
        PyValue_CLOSE(args[i]);
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            PyValue_CLOSE(args[i+argcount]);
        }
    }
    return NULL;
}

/* Consumes references to func, locals and all the args. Explicit locals is
 * an input of this public provider; inferred policy belongs to its caller. */
Py_NO_INLINE _PyInterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyValue func,
                      PyObject *locals, PyValue *args, size_t argcount,
                      PyObject *kwnames, _PyInterpreterFrame *previous)
{
    PyFunctionObject *function = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(function->func_code);
    _PyTransitionRoots staged;
    _PyInterpreterFrame *frame = eval_frame_push_and_init_code(
        tstate, func, locals, args, argcount, kwnames, previous, code, &staged);
    Py_DECREF(code);
    if (frame != NULL) {
        /* The caller must activate this child before it can reenter. */
        unstage_binding_frame(tstate, &staged);
    }
    return frame;
}

/* Same as eval_frame_push_and_init_code but takes an args tuple and kwargs dict.
   Steals references to func, callargs and kwargs.
*/
static _PyInterpreterFrame *
eval_frame_push_and_init_ex_code(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs,
    _PyInterpreterFrame *previous, PyCodeObject *code,
    _PyTransitionRoots *staged)
{
    bool has_dict = (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0);
    PyObject *kwnames = NULL;
    PyValue *newargs = NULL;
    PyObject *const *object_array = NULL;
    PyValue stack_array[8] = {0};
    Py_ssize_t total = nargs;
    _PyInterpreterFrame *new_frame = NULL;
    /* The positional objects for _PyStack_UnpackDict() live in this scope. */
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    if (has_dict) {
        PyObject *const *positionals = _PyBorrow_TupleItems(callargs, nargs);
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
    }
    /* Object views stay owned until the complete new frame is published.
     * Never overlay object pointers with value bits in the same array. */
    for (Py_ssize_t i = 0; i < total; i++) {
        PyValueBorrow borrowed = has_dict ? PyValue_FromObjectBorrow(object_array[i])
                                    : PyTuple_GET_VALUE(callargs, i);
        newargs[i] = PyValue_XDUP(borrowed);
    }
    new_frame = eval_frame_push_and_init_code(
        tstate, func, locals,
        newargs, nargs, kwnames, previous, code, staged
    );
    goto done;
fail_func:
    PyValue_CLOSE(func);
    Py_XDECREF(locals);
done:
    if (object_array != NULL) {
        _PyStack_UnpackDict_Free(object_array, nargs, kwnames);
    }
    _PyBorrowScope_Exit(tstate, mark);
    if (newargs != NULL && newargs != stack_array) {
        PyMem_Free(newargs);
    }
    Py_DECREF(callargs);
    Py_XDECREF(kwargs);
    return new_frame;
}

Py_NO_INLINE _PyInterpreterFrame *
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs,
    _PyInterpreterFrame *previous)
{
    PyFunctionObject *function = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(function->func_code);
    _PyTransitionRoots staged;
    _PyInterpreterFrame *frame = eval_frame_push_and_init_ex_code(
        tstate, func, locals, nargs, callargs, kwargs, previous, code, &staged);
    Py_DECREF(code);
    if (frame != NULL) {
        unstage_binding_frame(tstate, &staged);
    }
    return frame;
}

/* The shim's stage remains through self retirement and the final M2 closes.
 * After the arena authority transfers, the outer node only blocks redirects
 * of its caller; PopFrame cannot call out before we discard that metadata. */
static void
clear_init_shim(PyThreadState *tstate, _PyTransitionRoots *staged)
{
    _PyInterpreterFrame *shim = staged->frame;
    PyObject *error = _PyErr_GetRaisedException(tstate);
    _PyFrame_ClearTemporaries(shim, 0);
    staged->remaining = 0;
    staged->header_mask = 0;
    staged->home_authority = _PyIR_HOME_NONE;
    staged->homes = NULL;
    _PyEval_FrameClearAndPop(tstate, shim, _PyIR_RETIRE_STAGED_FAILURE);
    /* Deallocators preserve the raised-exception state they entered with.
     * Thus restoring the saved error below cannot close a new exception. */
    assert(!_PyErr_Occurred(tstate));
    staged->caller_staged = 0;
    staged->frame = NULL;
    _PyThreadState_PopTransitionRoots(tstate, staged);
    _PyErr_SetRaisedException(tstate, error);
}

static Py_NO_INLINE _PyInterpreterFrame *
create_init_frame_slow(PyThreadState *tstate, _PyInterpreterFrame *caller,
                       PyValue *window, int nargs, int destination,
                       _PyInstr *next)
{
    PyCodeObject *shim_code = (PyCodeObject *)&_Py_InitCleanup;
    CALL_STAT_INC(frames_pushed);
    _PyInterpreterFrame *shim = _PyThreadState_PushFrame(
        tstate, shim_code->co_framesize);
    if (shim == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyFrame_InitializeTrampoline(shim, shim_code, 1, caller);
    _PyFrame_WriteHome(shim, shim->base, PyValue_DUP(window[1]));
    _PyTransitionRoots shim_roots = {
        .frame = shim,
        .remaining = _PyFrame_NumSlots(shim),
        .home_authority = _PyIR_HOME_FRAME_PRESENCE,
        .header_mask = _Py_TRANSITION_CODE,
        .kind = _Py_TRANSITION_KIND_STAGED,
        .caller_staged = 1,
    };
    _PyThreadState_PushTransitionRoots(tstate, &shim_roots);

    PyValue small[8];
    int argcount = nargs + 1;
    PyValue *arguments = small;
    if (argcount > (int)Py_ARRAY_LENGTH(small)) {
        arguments = PyMem_Malloc(sizeof(PyValue) * argcount);
        if (arguments == NULL) {
            PyErr_NoMemory();
            clear_init_shim(tstate, &shim_roots);
            /* No input moved: the caller's error unwind still owns them. */
            return NULL;
        }
    }
    PyValue function = window[0];
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(function);
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(func->func_code);
    _PyFrame_WriteHome(caller, window, PyValue_NULL);
    for (int i = 0; i < argcount; i++) {
        arguments[i] = window[i + 1];
        _PyFrame_WriteHome(caller, window + i + 1, PyValue_NULL);
    }
    _PyTransitionRoots child_roots;
    _PyInterpreterFrame *child = eval_frame_push_and_init_code(
        tstate, function, NULL, arguments, argcount, NULL, shim, code,
        &child_roots);
    /* The binder consumed every argument on both outcomes. */
    if (arguments != small) {
        PyMem_Free(arguments);
    }
    Py_DECREF(code);
    if (child == NULL) {
        clear_init_shim(tstate, &shim_roots);
        return NULL;
    }
    caller->return_slot = (uint16_t)destination;
    caller->return_pc = next;
    tstate->py_recursion_remaining--;
    unstage_binding_frame(tstate, &child_roots);
    shim_roots.remaining = 0;
    shim_roots.header_mask = 0;
    shim_roots.home_authority = _PyIR_HOME_NONE;
    shim_roots.homes = NULL;
    shim_roots.caller_staged = 0;
    shim_roots.frame = NULL;
    _PyThreadState_PopTransitionRoots(tstate, &shim_roots);
    return child;
}

/* Reuse the ordinary typed initializer and exact positional mover. All
 * callback-capable binding stays in the outlined continuation above. */
Py_NO_INLINE _PyInterpreterFrame *
_PyEval_CreateInitFrame(PyThreadState *tstate, _PyInterpreterFrame *caller,
                        PyValue *window, int nargs, int destination,
                        _PyInstr *next)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(window[0]);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    assert(func->vectorcall == _PyFunction_Vectorcall);
    if (!(code->co_flags & CO_OPTIMIZED) ||
        (code->co_flags & (CO_VARARGS | CO_VARKEYWORDS)) ||
        code->co_kwonlyargcount || code->co_argcount != nargs + 1 ||
        code->_co_exec == NULL ||
        !_PyThreadState_HasStackSpace(tstate,
            code->co_framesize + _Py_InitCleanup.co_framesize))
    {
        return create_init_frame_slow(
            tstate, caller, window, nargs, destination, next);
    }
    _PyInterpreterFrame *shim = _PyFrame_PushTrampolineUnchecked(
        tstate, (PyCodeObject *)&_Py_InitCleanup, 1, caller);
    _PyFrame_WriteHome(shim, shim->base, PyValue_DUP(window[1]));
    PyValue function = window[0];
    _PyFrame_WriteHome(caller, window, PyValue_NULL);
    _PyInterpreterFrame *child = _PyFrame_PushUnchecked(tstate, function, shim);
    call_transfer_exact_args(child, caller, window, nargs, CALL_SELF_SUPPLIED);
    caller->return_slot = (uint16_t)destination;
    caller->return_pc = next;
    tstate->py_recursion_remaining--;
    return child;
}

/* The caller owns its whole window until the final non-reentrant commit. */
static int
call_transaction_overlap_python(_PyCallTransaction *call,
                                _PyInterpreterFrame **out)
{
#ifdef Py_GIL_DISABLED
    return 1;
#else
    if (call->kind != CALL_TRANSACTION_VECTOR || call->instrumented ||
        call->frame->owner != FRAME_OWNED_BY_THREAD) {
        return 1;
    }
    PyValue *homes = _PyFrame_Slots(call->frame) + call->base;
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(homes[0]);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int offset = call->state & CALL_SELF_PRESENT ? 1 : 2;
    int nargs = call->argument_count + 2 - offset;
    int flags = code->co_flags;
    PyObject *defaults = func->func_defaults;
    Py_ssize_t ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    if (!(flags & CO_OPTIMIZED) || code->co_kwonlyargcount ||
        nargs > code->co_argcount || nargs < code->co_argcount - ndefaults ||
        code->_co_exec == NULL ||
        !_PyThreadState_HasStackSpace(call->tstate, code->co_framesize)) {
        return 1;
    }

    /* Allocation can run Python. Pin the selected code and revalidate after
     * preparing kwargs; no window owner has been detached yet. */
    PyObject *kwdict = NULL;
    int status = 1;
    Py_INCREF(code);
    if (flags & CO_VARKEYWORDS) {
        kwdict = PyDict_New();
        if (kwdict == NULL || call_record_refresh(call) < 0) {
            status = -1;
            goto cleanup;
        }
    }
    homes = _PyFrame_Slots(call->frame) + call->base;
    if (!PyValue_IsObject(homes[0]) ||
        Py_TYPE(PyValue_AsKnownObjectBorrow(homes[0])) != &PyFunction_Type ||
        (PyObject *)func != PyValue_AsKnownObjectBorrow(homes[0]) ||
        func->func_code != (PyObject *)code ||
        func->vectorcall != _PyFunction_Vectorcall ||
        IS_PEP523_HOOKED(call->tstate) || code->_co_exec == NULL ||
        !_PyThreadState_HasStackSpace(call->tstate, code->co_framesize)) {
        goto cleanup;
    }
    defaults = func->func_defaults;
    ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    if (nargs < code->co_argcount - ndefaults) {
        goto cleanup;
    }
    Py_DECREF(code);  /* The function still owns it. */

    /* No callback, release or failure is allowed until child is current. */
    PyValue function = homes[0];
    _PyFrame_WriteHome(call->frame, homes, PyValue_NULL);
    _PyInterpreterFrame *child = _PyFrame_PushOverlapping(
        call->tstate, function, homes + offset, nargs, call->frame);
    for (int i = nargs; i < code->co_argcount; i++) {
        _PyFrame_WriteHome(child, child->base + i, PyValue_XDUP(
            _PyTuple_ITEMS(defaults)[i - (code->co_argcount - ndefaults)]));
    }
    int extra = code->co_argcount;
    if (flags & CO_VARARGS) {
        PyValue empty = _PyValue_STATIC_IMMORTAL(&_Py_SINGLETON(tuple_empty));
        _PyFrame_WriteHome(child, child->base + extra++, empty);
    }
    if (flags & CO_VARKEYWORDS) {
        _PyFrame_WriteHome(child, child->base + extra, PyValue_FromObjectSteal(kwdict));
    }
    call->frame->return_slot = (uint16_t)call->destination;
    call->frame->return_pc = call->frame->instr_ptr + 1;
    call->tstate->current_frame = child;
    *out = child;
    return 0;

cleanup:
    Py_XDECREF(kwdict);
    Py_DECREF(code);
    if (call_record_refresh(call) < 0) {
        return -1;
    }
    return status;
#endif
}

/* Parent inputs are never offered to a consuming binder. Copies can fail
 * and retire while the parent's actual IN_CALL homes remain owning. */
static Py_NO_INLINE int
call_transaction_push_python_fallback(_PyCallTransaction *call,
                                      _PyInterpreterFrame **out)
{
    PyValue small[16];
    PyValue *storage = small;
    int copied = 0, retired_count = 0;
    /* 1: not invoked (the caller falls back), 0: pushed, -1: failed */
    int status = 1;
    PyValue function = PyValue_NULL;
    PyObject *locals = NULL, *names = NULL, *tuple = NULL, *kwargs = NULL;
    PyCodeObject *code = NULL;
    _PyInterpreterFrame *child = NULL;
    _PyTransitionRoots staged;
    bool child_staged = false;
    if (call->count > (int)Py_ARRAY_LENGTH(small) / 2) {
        storage = PyMem_Malloc((size_t)call->count * 2 * sizeof(PyValue));
        if (storage == NULL) {
            PyErr_NoMemory();
        }
        if (storage == NULL || call_record_refresh(call) < 0) {
            status = -1;
            goto done;
        }
    }
    PyValue *homes = _PyFrame_Slots(call->frame) + call->base;
    PyObject *callable = PyValue_AsKnownObjectBorrow(homes[0]);
    /* Allocation above may have changed vectorcall. Choose a new adapter
     * inside the same record; never replay method/monitor/EX preparation. */
    if (Py_TYPE(callable) != &PyFunction_Type || IS_PEP523_HOOKED(call->tstate) ||
        ((PyFunctionObject *)callable)->vectorcall != _PyFunction_Vectorcall) {
        goto done;
    }
    function = PyValue_XDUP(homes[0]);
    code = (PyCodeObject *)Py_NewRef(PyFunction_GET_CODE(callable));
    locals = code->co_flags & CO_OPTIMIZED ? NULL
        : Py_NewRef(PyFunction_GET_GLOBALS(callable));
    PyValue consumed_function = function;
    function = PyValue_NULL;
    PyObject *consumed_locals = locals;
    locals = NULL;
    if (call->kind == CALL_TRANSACTION_EX) {
        tuple = Py_NewRef(PyValue_AsKnownObjectBorrow(homes[2]));
        kwargs = call->state & CALL_KWARGS_PRESENT
            ? Py_NewRef(PyValue_AsKnownObjectBorrow(homes[3])) : NULL;
        PyObject *consumed_tuple = tuple, *consumed_kwargs = kwargs;
        tuple = kwargs = NULL;
        child = eval_frame_push_and_init_ex_code(
            call->tstate, consumed_function, consumed_locals,
            PyTuple_GET_SIZE(consumed_tuple), consumed_tuple, consumed_kwargs,
            call->frame, code, &staged);
    }
    else {
        int offset = call->state & CALL_SELF_PRESENT ? 1 : 2;
        int total = call->argument_count + 2 - offset;
        if (call->kind == CALL_TRANSACTION_KEYWORDS) {
            names = Py_NewRef(PyValue_AsKnownObjectBorrow(homes[call->count - 1]));
        }
        for (int i = 0; i < total; i++) {
            storage[i] = PyValue_XDUP(homes[offset + i]);
            copied++;
        }
        int positional = total - (names == NULL ? 0 : (int)PyTuple_GET_SIZE(names));
        /* Even on failure all these bits are consumed by the provider. */
        copied = 0;
        child = eval_frame_push_and_init_code(
            call->tstate, consumed_function, consumed_locals, storage,
            positional, names, call->frame, code, &staged);
    }
    child_staged = child != NULL;
    if (call_record_refresh(call) < 0 || child == NULL) {
        status = -1;
        goto done;
    }
    /* The code-specific provider initializes at numeric PC 0. Binding may
     * already have reinstrumented its code: never subtract the returned
     * child's old instruction pointer from a current executable. */
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec->nwords == 0) {
        PyErr_SetString(PyExc_SystemError, "Python call lost its executable");
        status = -1;
        goto done;
    }
    child->ir_words = exec->words;
    child->ir_feedback = exec->entries;
    child->instr_ptr = exec->words;
    PyValue *retired = storage + call->count;
    call_transaction_detach(call, retired);
    retired_count = call->count;
    call->frame->return_slot = (uint16_t)call->destination;
    call->frame->return_pc = call->frame->instr_ptr + 1;
    assert(child->previous == call->frame);
    call->tstate->current_frame = child;
    unstage_binding_frame(call->tstate, &staged);
    child_staged = false;
    status = 0;
done:
    PyValue_CLEAR(function);
    Py_CLEAR(locals);
    Py_CLEAR(names);
    Py_CLEAR(tuple);
    Py_CLEAR(kwargs);
    PyObject *code_owner = (PyObject *)code;
    code = NULL;
    Py_CLEAR(code_owner);
    for (int i = copied; i-- > 0;) {
        PyValue_CLEAR(storage[i]);
    }
    for (int i = retired_count; i-- > 0;) {
        PyValue_CLEAR(storage[call->count + i]);
    }
    if (storage != small && storage != NULL) {
        PyMem_Free(storage);
    }
    /* The releases above ran with the child current.  The child has not
     * started, so it needs no refresh: its words are fixed, and its RESUME
     * brings its instrumentation up to date, as for every frame that starts. */
    if (status == 0 && call->tstate->current_frame != child) {
        value_revalidation_error("value bridge changed the active frame");
        status = -1;
    }
    assert(status != 0 ||
           child->ir_words == _PyFrame_GetCode(child)->_co_exec->words);
    if (child != NULL && status < 0) {
        /* The child has not entered its recursion/monitor protocol. Restore
         * the actual parent before clearing it; do not unwind it as executed. */
        call->tstate->current_frame = call->frame;
        call_transaction_publish_pc(call);
        if (child_staged) {
            unstage_binding_frame(call->tstate, &staged);
        }
        _PyEval_FrameClearAndPop(call->tstate, child, _PyIR_RETIRE_STAGED_FAILURE);
        child = NULL;
    }
    if (child == NULL) {
        /* Either the call was not invoked (1), or it failed (-1) and the
         * failing step above set its own error. */
        assert(status == 1 || (status < 0 && PyErr_Occurred()));
        int failed = call_record_refresh(call);
        if (failed < 0 || status < 0) {
            return -1;
        }
    }
    *out = child;
    return child != NULL ? 0 : 1;
}

static int
call_transaction_push_python(_PyCallTransaction *call,
                             _PyInterpreterFrame **out)
{
    int overlap = call_transaction_overlap_python(call, out);
    if (overlap <= 0) {
        return overlap;
    }
    return call_transaction_push_python_fallback(call, out);
}

static int
call_transaction_invoke(_PyCallTransaction *call, _PyInterpreterFrame **child)
{
    *child = NULL;
    if (!(call->state & CALL_PREPARED)) {
        int prepared = call->kind == CALL_TRANSACTION_EX
            ? call_record_prepare_tuple(call)
            : call_record_expand_method(call);
        if (prepared < 0) {
            return -1;
        }
    }
    if (call->instrumented && !call->monitored &&
        call_record_monitor(call) < 0) {
        return -1;
    }
    PyValueBorrow callable = PyValue_Borrow(
        _PyFrame_Slots(call->frame)[call->base]);
    if (PyValue_IsObject(callable)) {
        PyObject *object = PyValue_AsKnownObjectBorrow(callable);
        if (Py_TYPE(object) == &PyFunction_Type &&
            !IS_PEP523_HOOKED(call->tstate) &&
            ((PyFunctionObject *)object)->vectorcall == _PyFunction_Vectorcall &&
            !(call->kind == CALL_TRANSACTION_EX && call->instrumented)) {
            int status = call_transaction_push_python(call, child);
            if (status <= 0) {
                return status;
            }
        }
    }
    return call_transaction_native(call);
}

static inline _PyCallTransaction
call_transaction_init(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      int base, int count, int destination, int argument_count,
                      _PyCallTransactionKind kind, int flags)
{
    return (_PyCallTransaction) {
        .tstate = tstate, .frame = frame, .current = frame->instr_ptr,
        .base = base, .count = count, .destination = destination,
        .argument_count = argument_count, .kind = kind,
        .instrumented = (flags & CALL_INSTRUMENTED) != 0,
        .monitored = (flags & CALL_MONITORED) != 0,
        .periodic = (flags & CALL_PERIODIC) != 0,
        .state = flags & (CALL_PREPARED | CALL_SELF_PRESENT | CALL_KWARGS_PRESENT),
    };
}

/* Run a call: the rest of its preparation, its CALL event, and either a
 * pushed Python frame or a native call.  The handler keeps the call's state
 * in registers (flags: CALL_EFFECTS, CALL_PREPARED and CALL_MONITORED for
 * what it did, CALL_INSTRUMENTED and CALL_PERIODIC for what it is).  The
 * transaction record, whose helpers take its address, lives only in this
 * function's frame (register-ir-v2-a3.md 3.5, R-c).  Returns {-1, NULL} on
 * error, {0, child} for a pushed frame, {0, NULL} for a finished call. */
static inline _PyCallResult
call_transaction_run(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     int base, int count, int destination, int argument_count,
                     _PyCallTransactionKind kind, int flags)
{
    _PyCallTransaction call = call_transaction_init(
        tstate, frame, base, count, destination, argument_count, kind, flags);
    _PyInterpreterFrame *child;
    int status = call_transaction_invoke(&call, &child);
    return (_PyCallResult){status < 0 ? -1 : 0, status < 0 ? NULL : child};
}

/* One out-of-line run per call kind.  The handler's instr_ptr is its
 * instruction here: every preparation helper the handler called restored
 * it. */
static Py_NO_INLINE _PyCallResult
call_transaction_run_vector(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            int base, int destination, int argument_count,
                            int flags)
{
    return call_transaction_run(tstate, frame, base, argument_count + 2,
                                destination, argument_count,
                                CALL_TRANSACTION_VECTOR, flags);
}

/* The checked prefix proves the function version and standard provider.
 * Return 1 before effects on a miss; no transaction record exists on a hit. */
static inline _PyCallResult
call_bind_known_python(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       int base, int destination, int argument_count,
                       _PyCallTransactionKind kind, const _PyFbCall *binding,
                       int roles)
{
#ifdef Py_GIL_DISABLED
    return (_PyCallResult){1, NULL};
#else
    if (kind == CALL_TRANSACTION_KEYWORDS &&
        (binding->binding_flags & _Py_CALL_BIND_KEYWORDS) &&
        frame->owner == FRAME_OWNED_BY_THREAD)
    {
        int kwcount = (binding->binding_flags & _Py_CALL_BIND_KEYWORD_COUNT_MASK)
            >> _Py_CALL_BIND_KEYWORD_COUNT_SHIFT;
        PyValue *homes = frame->base + base;
        PyFunctionObject *func = (PyFunctionObject *)
            PyValue_AsKnownObjectBorrow(homes[0]);
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        int has_self = (roles & CALL_SELF_PRESENT) != 0;
        int offset = has_self ? 1 : 2;
        int positional = argument_count + has_self - kwcount;
        PyValueBorrow names_value = PyValue_Borrow(homes[argument_count + 2]);
        if (kwcount < 1 || kwcount > _Py_CALL_BIND_KEYWORD_CAPACITY ||
            has_self != !!(binding->binding_flags & _Py_CALL_BIND_SELF) ||
            positional < 0 || positional > code->co_argcount ||
            !PyValue_IsObject(names_value) ||
            !PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(names_value)) ||
            func->vectorcall != _PyFunction_Vectorcall ||
            code->_co_exec == NULL ||
            !_PyThreadState_HasStackSpace(tstate, code->co_framesize))
        {
            return (_PyCallResult){1, NULL};
        }
        PyObject *names = PyValue_AsKnownObjectBorrow(names_value);
        if (PyTuple_GET_SIZE(names) != kwcount) {
            return (_PyCallResult){1, NULL};
        }
        int total = code->co_argcount + code->co_kwonlyargcount;
        for (int i = 0; i < kwcount; i++) {
            int slot = binding->binding.keyword_dest[i];
            PyValueBorrow name = PyTuple_GET_VALUE(names, i);
            if (slot < code->co_posonlyargcount || slot < positional ||
                slot >= total || !PyValue_IsObject(name) ||
                !PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(name)) ||
                PyValue_AsKnownObjectBorrow(name) !=
                    _PyTuple_GET_OBJECT(code->co_localsplusnames, slot))
            {
                return (_PyCallResult){1, NULL};
            }
            for (int j = 0; j < i; j++) {
                if (binding->binding.keyword_dest[j] == slot) {
                    return (_PyCallResult){1, NULL};
                }
            }
        }

        /* Functions created from the same code share a version. Read their
         * current default shape before relying on the cached coverage proof. */
        PyObject *defaults = func->func_defaults;
        Py_ssize_t ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
        Py_ssize_t first_default = code->co_argcount - ndefaults;
        if (ndefaults > UINT16_MAX || first_default - positional > kwcount) {
            return (_PyCallResult){1, NULL};
        }
        for (int slot = positional; slot < first_default; slot++) {
            int i;
            for (i = 0; i < kwcount; i++) {
                if (binding->binding.keyword_dest[i] == slot) {
                    break;
                }
            }
            if (i == kwcount) {
                return (_PyCallResult){1, NULL};
            }
        }

        /* All guards precede this transfer. Detach the keyword suffix and
         * names before initialization can fill overlapping constant slots. */
        PyValue keywords[_Py_CALL_BIND_KEYWORD_CAPACITY];
        for (int i = 0; i < kwcount; i++) {
            keywords[i] = homes[offset + positional + i];
            _PyFrame_WriteHome(frame, homes + offset + positional + i, PyValue_NULL);
        }
        PyValue names_owner = homes[argument_count + 2];
        _PyFrame_WriteHome(frame, homes + argument_count + 2, PyValue_NULL);
        PyValue function = homes[0];
        _PyFrame_WriteHome(frame, homes, PyValue_NULL);
        /* No failure, callback or release until the child is complete. */
        _PyInterpreterFrame *child = _PyFrame_PushOverlapping(
            tstate, function, homes + offset, positional, frame);
        for (int i = 0; i < kwcount; i++) {
            _PyFrame_WriteHome(child, child->base + binding->binding.keyword_dest[i], keywords[i]);
        }
        for (int i = positional; i < code->co_argcount; i++) {
            if (PyValue_IsNull(child->base[i])) {
                assert(i >= first_default);
                _PyFrame_WriteHome(child, child->base + i, PyValue_XDUP(
                    _PyTuple_ITEMS(defaults)[i - first_default]));
            }
        }
        frame->return_slot = (uint16_t)destination;
        frame->return_pc = frame->instr_ptr + 1;
        tstate->current_frame = child;
        CALL_STAT_INC(keyword_plan_hits);
        /* Exact strings in an exact tuple cannot invoke Python finalizers. */
        PyValue_CLOSE(names_owner);
        return (_PyCallResult){0, child};
    }
    if (kind == CALL_TRANSACTION_VECTOR &&
        (binding->binding_flags & _Py_CALL_BIND_DIRECT) &&
        frame->owner == FRAME_OWNED_BY_THREAD)
    {
        PyValue *homes = frame->base + base;
        PyFunctionObject *func = (PyFunctionObject *)
            PyValue_AsKnownObjectBorrow(homes[0]);
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        int offset = roles & CALL_SELF_PRESENT ? 1 : 2;
        int supplied = argument_count + 2 - offset;
        if (supplied == binding->binding.positional.bound_count && code->_co_exec != NULL &&
            _PyThreadState_HasStackSpace(tstate, code->co_framesize))
        {
            if (supplied < code->co_argcount) {
                /* A same-code function can share this version but have a
                 * different defaults tuple. Validate the cached offset. */
                PyObject *defaults = func->func_defaults;
                Py_ssize_t ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
                Py_ssize_t first_default = code->co_argcount - ndefaults;
                if (ndefaults > UINT16_MAX || supplied < first_default ||
                    binding->binding.positional.default_start != supplied - first_default)
                {
                    return (_PyCallResult){1, NULL};
                }
            }
            PyObject *kwargs = NULL;
            if (binding->binding.positional.kwargs_slot != IR_NO_DST &&
                !(binding->binding_flags & _Py_CALL_BIND_VIRTUAL_KWARGS))
            {
                /* Empty-dict allocation schedules GC; it does not run
                 * Python before the complete child is published. */
                kwargs = PyDict_New();
                if (kwargs == NULL) {
                    return (_PyCallResult){-1, NULL};
                }
            }
            PyValue function = homes[0];
            _PyFrame_WriteHome(frame, homes, PyValue_NULL);
            _PyInterpreterFrame *child = _PyFrame_PushOverlapping(
                tstate, function, homes + offset, supplied, frame);
            if (supplied < code->co_argcount) {
                PyValue *defaults = _PyTuple_ITEMS(func->func_defaults);
                for (int i = supplied; i < code->co_argcount; i++) {
                    _PyFrame_WriteHome(child, child->base + i, PyValue_XDUP(
                        defaults[binding->binding.positional.default_start + i - supplied]));
                }
            }
            if (binding->binding_flags & _Py_CALL_BIND_VARARGS) {
                _PyFrame_WriteHome(child, child->base + code->co_argcount,
                    (PyValue)_PyValue_STATIC_IMMORTAL(&_Py_SINGLETON(tuple_empty)));
            }
            if (binding->binding.positional.kwargs_slot != IR_NO_DST) {
                if (binding->binding_flags & _Py_CALL_BIND_VIRTUAL_KWARGS) {
                    child->pending_kwargs = binding->binding.positional.kwargs_slot;
                }
                else {
                    _PyFrame_WriteHome(child,
                        child->base + binding->binding.positional.kwargs_slot,
                        PyValue_FromObjectSteal(kwargs));
                }
            }
            frame->return_slot = (uint16_t)destination;
            frame->return_pc = frame->instr_ptr + 1;
            tstate->current_frame = child;
            return (_PyCallResult){0, child};
        }
    }
    return (_PyCallResult){1, NULL};
#endif
}

static Py_NO_INLINE _PyCallResult
call_transaction_run_keywords(PyThreadState *tstate, _PyInterpreterFrame *frame,
                              int base, int destination, int argument_count,
                              int flags)
{
    return call_transaction_run(tstate, frame, base, argument_count + 3,
                                destination, argument_count,
                                CALL_TRANSACTION_KEYWORDS, flags);
}

static Py_NO_INLINE _PyCallResult
call_transaction_run_ex(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        int base, int destination, int flags)
{
    return call_transaction_run(tstate, frame, base, 4, destination, 0,
                                CALL_TRANSACTION_EX, flags);
}

Py_NO_INLINE PyObject *
_PyEval_Vector(PyThreadState *tstate, PyFunctionObject *func,
               PyObject *locals,
               PyObject* const* args, size_t argcount,
               PyObject *kwnames)
{
    _PyInterpreterFrame *previous = tstate->current_frame;
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
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
    /* The constructor consumes func, locals and every argument owner. */
    Py_XINCREF(locals);
    for (size_t i = 0; i < argcount; i++) {
        arguments[i] = PyValue_XDUP(PyValue_FromObjectBorrow(args[i]));
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
            arguments[i+argcount] = PyValue_XDUP(PyValue_FromObjectBorrow(args[i+argcount]));
        }
    }
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(func->func_code);
    _PyTransitionRoots staged;
    bool linked = false;
    _PyInterpreterFrame *frame = eval_frame_push_and_init_code(
        tstate, PyValue_DUP(PyValue_FromObjectBorrow(func)), locals,
        arguments, argcount, kwnames, NULL, code, &staged);
    linked = frame != NULL;
    Py_DECREF(code);
    if (total_args > 8) {
        PyMem_Free(arguments);
    }
    if (!linked) {
        return NULL;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_VECTOR);
    PyObject *result = _PyEval_EvalFrame(tstate, frame, 0);
    finish_staged_provider(tstate, &staged, previous);
#ifndef NDEBUG
    /* The first root chunk is retained even after its initial frame exits. */
    assert(chunk == NULL || tstate->datastack_chunk == chunk);
    assert(chunk == NULL || tstate->datastack_top == top);
    assert(chunk == NULL || tstate->datastack_limit == limit);
#endif
    return result;
}

Py_NO_INLINE PyValue
_PyEval_VectorValue(PyThreadState *tstate, PyFunctionObject *func,
                     const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    _PyInterpreterFrame *previous = tstate->current_frame;
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    EVAL_CALL_STAT_INC(EVAL_CALL_FUNCTION_VECTORCALL);
    Py_ssize_t total = nargs + (kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames));
    PyValue small[8];
    PyValue *owners = small;
    if (total > (Py_ssize_t)Py_ARRAY_LENGTH(small)) {
        owners = PyMem_Malloc(sizeof(PyValue) * (size_t)total);
        if (owners == NULL) {
            _PyErr_NoMemory(tstate);
            return PyValue_NULL;
        }
    }
    for (Py_ssize_t i = 0; i < total; i++) {
        owners[i] = PyValue_DUP(args[i]);
    }
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(func->func_code);
    PyObject *locals = code->co_flags & CO_OPTIMIZED ? NULL
        : Py_NewRef(func->func_globals);
    _PyTransitionRoots staged;
    bool linked = false;
    _PyInterpreterFrame *frame = eval_frame_push_and_init_code(
        tstate, PyValue_DUP(PyValue_FromObjectBorrow(func)), locals,
        owners, nargs, kwnames, NULL, code, &staged);
    linked = frame != NULL;
    Py_DECREF(code);
    if (owners != small) {
        PyMem_Free(owners);
    }
    if (!linked) {
        return PyValue_NULL;
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_VECTOR);
    PyValue result = _PyEval_EvalFrameValue(tstate, frame, 0);
    finish_staged_provider(tstate, &staged, previous);
#ifndef NDEBUG
    /* The first root chunk is retained even after its initial frame exits. */
    assert(chunk == NULL || tstate->datastack_chunk == chunk);
    assert(chunk == NULL || tstate->datastack_top == top);
    assert(chunk == NULL || tstate->datastack_limit == limit);
#endif
    return result;
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
            PyTuple_SET_VALUE(kwnames, i, PyValue_DUP(PyValue_FromObjectBorrow(_PyObject_CAST(kws[2*i]))));
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
            PyObject *excs = _PyTuple_PackValues(1, (PyValueBorrow[]){PyValue_FromObjectBorrow(exc_value)});
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

        PyValue match_owner = PyTuple_GetValue(pair, 0);
        *match = PyValue_IsNull(match_owner) ? NULL
            : _PyValue_AsObjectStealAt(match_owner, BOX_SITE_TUPLE);
        if (*match == NULL) {
            Py_DECREF(pair);
            return -1;
        }
        PyValue rest_owner = PyTuple_GetValue(pair, 1);
        *rest = PyValue_IsNull(rest_owner) ? NULL
            : _PyValue_AsObjectStealAt(rest_owner, BOX_SITE_TUPLE);
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

/* Consume v, iterate argcnt times and store the results via decreasing sp.
   Return 1 for success, 0 if error.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target.
*/

Py_NO_INLINE int
_PyEval_UnpackIterableValue(PyThreadState *tstate, PyObject *v,
                       int argcnt, int argcntafter, PyValue *sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it = NULL;  /* iter(v) */
    _PyStatusValue step;
    PyObject *l = NULL; /* variable list */
    assert(v != NULL);
    assert(argcnt >= 0 && argcntafter >= -1);
    int capacity = argcnt + (argcntafter < 0 ? 0 : argcntafter + 1);
    _PyInterpreterFrame *frame = tstate->current_frame;
    assert(sp - frame->base >= capacity);
    assert(sp <= frame->base + _PyFrame_NumSlots(frame));
    _PyTransitionPrefix prefix = {
        .base = sp - capacity,
        .capacity = capacity,
        .begin = capacity,
        .end = capacity,
    };
    _PyTransitionRoots roots = {
        .frame = frame,
        .prefix = &prefix,
    };
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    int result = 0;

    it = _PyValue_GetIter(PyValue_FromObjectBorrow(v));
    if (it == NULL) {
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
            Py_TYPE(v)->tp_iter == NULL && !_PyValue_SequenceCheck(PyValue_FromObjectBorrow(v)))
        {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "cannot unpack non-iterable %.200s object",
                          Py_TYPE(v)->tp_name);
        }
        goto Done;
    }

    for (; i < argcnt; i++) {
        step = _PyValue_IterNext(it);
        if (step.status != PYGEN_NEXT) {
            /* Iterator done, via error or exhaustion. */
            if (step.status == PYGEN_RETURN) {
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
        *--sp = step.value;
        prefix.begin--;
        _PyValue_CheckTemp(*sp);
    }

    if (argcntafter == -1) {
        /* We better have exhausted the iterator now. */
        step = _PyValue_IterNext(it);
        if (step.status == PYGEN_ERROR)
            goto Error;
        if (step.status == PYGEN_RETURN) {
            result = 1;
            goto Done;
        }
        PyValue_CLOSE(step.value);

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

    l = _PyValue_SequenceList(PyValue_FromObjectBorrow(it));
    if (l == NULL)
        goto Error;
    *--sp = PyValue_FromObjectSteal(l);
    prefix.begin--;
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
        prefix.begin--;
    }
    /* Resize the list. */
    Py_SET_SIZE(l, ll - argcntafter);
    result = 1;
    goto Done;

Error:
    /* The slots that the values went to are temporaries, which hold NULL
       unless live */
    for (; i > 0; i--, sp++) {
        // a move: cleared below
        PyValue ref = *sp;
        prefix.begin++;
        _PyFrame_WriteHome(frame, sp, PyValue_NULL);
        PyValue_CLOSE(ref);
    }
Done:
    Py_XDECREF(it);
    Py_DECREF(v);
    /* Installed values are external prefix owners until cleanup completes.
     * Publish their frame authority before disarming, without a callback. */
    for (int home = prefix.begin; home < prefix.end; home++) {
        _PyFrame_SetHomeBit(frame, prefix.base + home,
                           !PyValue_IsNull(prefix.base[home]));
    }
    prefix.begin = prefix.end;
    _PyThreadState_PopTransitionRoots(tstate, &roots);
    return result;
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

    assert(_PyValue_MappingCheck(PyValue_FromObjectBorrow(locals)));
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

    assert(_PyValue_MappingCheck(PyValue_FromObjectBorrow(locals)));
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
        PyValue builtins_v;
        int found = PyDict_GetValue(
            globals, _PyValue_FromObjectKnown(&_Py_ID(__builtins__)),
            &builtins_v);
        if (found < 0) {
            return NULL;
        }
        if (found > 0) {
            builtins = _PyValue_AsObjectStealAt(builtins_v, BOX_SITE_DICT);
            if (builtins == NULL) {
                return NULL;
            }
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
        if (PyDict_SetValue(globals, _PyValue_FromObjectKnown(&_Py_ID(__builtins__)), PyValue_FromObjectBorrow(builtins)) < 0) {
            return -1;
        }
    }
    else {
        if (_PyValue_SetItem(PyValue_FromObjectBorrow(globals), _PyValue_FromObjectKnown(&_Py_ID(__builtins__)), PyValue_FromObjectBorrow(builtins)) < 0) {
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
        x = _PyValue_AsSsize_t(PyValue_FromObjectBorrow(v), NULL);
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
   bounds go into a slice object by value, and the item is read or stored
   through _PyValue_GetItem() or _PyValue_SetItem(): no bound is boxed. */

/* The bounds of a slice with no step, as PySlice_Unpack() gives them, if
   start and stop are each an inline int or None, and one is an inline int. */
static bool
inline_slice_bounds(PyValueBorrow start, PyValueBorrow stop,
                    Py_ssize_t *istart, Py_ssize_t *istop)
{
    if (!PyValue_IsInlineInt(start) && !PyValue_IsInlineInt(stop)) {
        return false;
    }
    if (PyValue_IsInlineInt(start)) {
        *istart = PyValue_InlineInt64(start);
    }
    else if (PyValue_IsNone(start)) {
        *istart = 0;
    }
    else {
        return false;
    }
    if (PyValue_IsInlineInt(stop)) {
        *istop = PyValue_InlineInt64(stop);
    }
    else if (PyValue_IsNone(stop)) {
        *istop = PY_SSIZE_T_MAX;
    }
    else {
        return false;
    }
    return true;
}

/* The slice start:stop as a new object, with the bounds as they are (value
   fields, M4b): no box of an inline bound. NULL with an exception set. */
static PyObject *
slice_of_bounds(PyValueBorrow start, PyValueBorrow stop)
{
    return _PySlice_FromValues(start, stop, PyValue_NULL);
}

// container[start:stop], for BINARY_SLICE: an owner of the item, or
// PyValue_NULL with an exception set.
PyValue
(_PyEval_BinarySlice)(PyValueBorrow container_st, PyValueBorrow start_st, PyValueBorrow stop_st)
{
    if (PyValue_IsPointerSpace(container_st)) {
        PyObject *container = PyValue_AsKnownObjectBorrow(container_st);
        PyObject *res = NULL;
        Py_ssize_t start, stop;
        if (inline_slice_bounds(start_st, stop_st, &start, &stop)) {
            if (PyList_CheckExact(container)) {
                PySlice_AdjustIndices(PyList_GET_SIZE(container), &start, &stop, 1);
                res = PyList_GetSlice(container, start, stop);
                goto done;
            }
            if (PyTuple_CheckExact(container)) {
                PySlice_AdjustIndices(PyTuple_GET_SIZE(container), &start, &stop, 1);
                res = PyTuple_GetSlice(container, start, stop);
                goto done;
            }
            if (PyUnicode_CheckExact(container)) {
                PySlice_AdjustIndices(PyUnicode_GET_LENGTH(container), &start, &stop, 1);
                res = PyUnicode_Substring(container, start, stop);
                goto done;
            }
        }
        else if (PyValue_IsPointerSpace(start_st) &&
                 PyValue_IsPointerSpace(stop_st))
        {
            /* Object bounds (None, or an object with __index__): the type's
               own slicing reads them without a slice object. */
            PyObject *start_o = PyValue_AsKnownObjectBorrow(start_st);
            PyObject *stop_o = PyValue_AsKnownObjectBorrow(stop_st);
            if (PyList_CheckExact(container)) {
                res = _PyList_BinarySlice(container, start_o, stop_o);
                goto done;
            }
            if (PyTuple_CheckExact(container)) {
                res = _PyTuple_BinarySlice(container, start_o, stop_o);
                goto done;
            }
            if (PyUnicode_CheckExact(container)) {
                res = _PyUnicode_BinarySlice(container, start_o, stop_o);
                goto done;
            }
        }
        goto generic;
done:
        return res == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res);
    }
generic:;
    PyObject *slice = slice_of_bounds(start_st, stop_st);
    if (slice == NULL) {
        return PyValue_NULL;
    }
    PyValue res = _PyValue_GetItem(container_st, PyValue_FromObjectBorrow(slice));
    Py_DECREF(slice);
    return res;
}

// container[start:stop] = v, for STORE_SLICE. Return 0, or -1 with an
// exception set.
int
(_PyEval_StoreSlice)(PyValueBorrow v_st, PyValueBorrow container_st,
                   PyValueBorrow start_st, PyValueBorrow stop_st)
{
    Py_ssize_t start, stop;
    if (PyValue_IsPointerSpace(v_st) && PyValue_IsPointerSpace(container_st) &&
        PyList_CheckExact(PyValue_AsKnownObjectBorrow(container_st)) &&
        inline_slice_bounds(start_st, stop_st, &start, &stop))
    {
        return _PyList_StoreSlice(PyValue_AsKnownObjectBorrow(container_st), start, stop,
                                  PyValue_AsKnownObjectBorrow(v_st));
    }
    PyObject *slice = slice_of_bounds(start_st, stop_st);
    if (slice == NULL) {
        return -1;
    }
    int err = _PyValue_SetItem(container_st, PyValue_FromObjectBorrow(slice), v_st);
    Py_DECREF(slice);
    return err;
}

/* isinstance(inst, cls), for CALL_ISINSTANCE when inst or cls may be inline.
   An int or float has no __class__ of its own, so against an exact type, or
   a tuple of them, the answer is that of PyObject_IsInstance(), whose
   object_isinstance() then tests the subtype relation alone. */
int
(_PyEval_IsInstanceValue)(PyValueBorrow inst_st, PyValueBorrow cls_st)
{
    if (PyValue_IsInline(inst_st) && PyValue_IsPointerSpace(cls_st)) {
        PyTypeObject *type = PyValue_TYPE(inst_st);
        PyObject *cls = PyValue_AsKnownObjectBorrow(cls_st);
        if (PyType_CheckExact(cls)) {
            return PyType_IsSubtype(type, (PyTypeObject *)cls);
        }
        if (PyTuple_CheckExact(cls)) {
            Py_ssize_t n = PyTuple_GET_SIZE(cls);
            Py_ssize_t i = 0;
            for (; i < n; i++) {
                PyValueBorrow item = PyTuple_GET_VALUE(cls, i);
                if (!PyValue_IsObject(item) ||
                    !PyType_CheckExact(PyValue_AsKnownObjectBorrow(item))) {
                    break;
                }
                if (PyType_IsSubtype(type, (PyTypeObject *)PyValue_AsKnownObjectBorrow(item))) {
                    return 1;
                }
            }
            if (i == n) {
                return 0;
            }
        }
    }
    return _PyValue_IsInstance(inst_st, cls_st);
}

PyObject *
_PyEval_ImportName(PyThreadState *tstate, PyObject *builtins,
            PyObject *globals, PyObject *locals, PyObject *name,
            PyObject *fromlist, int level)
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
                             PyObject *name, PyObject *fromlist, int level)
{
    if (locals == NULL) {
        locals = Py_None;
    }

    /* Fast path for not overloaded __import__. */
    if (_PyImport_IsDefaultImportFunc(tstate->interp, import_func)) {
        return PyImport_ImportModuleLevelObject(
                        name,
                        globals,
                        locals,
                        fromlist,
                        level);
    }

    PyObject *level_o = PyLong_FromLong(level);
    if (level_o == NULL) {
        return NULL;
    }
    PyObject *args[5] = {name, globals, locals, fromlist, level_o};
    PyObject *res = PyObject_Vectorcall(import_func, args, 5, NULL);
    Py_DECREF(level_o);
    return res;
}

static int
check_lazy_import_compatibility(PyThreadState *tstate, PyObject *globals,
                               PyObject *name, int level)
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

    abs_name = _PyImport_GetAbsName(tstate, name, globals, level);
    if (abs_name == NULL) {
        goto error;
    }

    res = _PyValue_Contains(PyValue_FromObjectBorrow(lazy_modules), PyValue_FromObjectBorrow(abs_name));
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
                       PyObject *fromlist, int level, int lazy)
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
        res = _PyImport_LazyImportModuleLevelObject(
            tstate, name, builtins, globals, locals, fromlist, level
        );
        goto error;
    }

    PyObject *level_o = PyLong_FromLong(level);
    if (level_o == NULL) {
        goto error;
    }
    PyObject *args[5] = {name, globals, locals, fromlist, level_o};
    res = PyObject_Vectorcall(lazy_import_func, args, 5, NULL);
    Py_DECREF(level_o);
error:
    Py_XDECREF(lazy_import_func);
    return res;
}

static PyValue
_eval_import_from_miss(PyThreadState *tstate, PyValueBorrow v, PyObject *name)
{
    PyValue mod_name_value = PyValue_NULL;
    PyValue spec_value = PyValue_NULL;
    PyValue origin_value = PyValue_NULL;
    PyObject *mod_name = NULL;
    PyObject *mod_name_or_unknown = NULL;
    PyObject *unknown_name = NULL;
    PyObject *origin = NULL;
    PyObject *module_origin = NULL;
    PyObject *errmsg = NULL;

    _PyStatusValue name_attr = _PyValue_GetOptionalAttr(
        v, &_Py_ID(__name__));
    if (name_attr.status < 0) {
        return PyValue_NULL;
    }
    if (name_attr.status > 0 && PyValue_IsObject(name_attr.value) &&
        PyUnicode_Check(PyValue_AsKnownObjectBorrow(name_attr.value)))
    {
        mod_name_value = name_attr.value;
        mod_name = PyValue_AsKnownObjectBorrow(mod_name_value);
    }
    else {
        PyValue_XCLOSE(name_attr.value);
        goto error;
    }

    PyObject *fullmodname = PyUnicode_FromFormat("%U.%U", mod_name, name);
    if (fullmodname == NULL) {
        goto done;
    }
    PyObject *x = PyImport_GetModule(fullmodname);
    Py_DECREF(fullmodname);
    if (x == NULL && !_PyErr_Occurred(tstate)) {
        goto error;
    }
    PyValue_CLOSE(mod_name_value);
    if (x == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(x);

error:
    if (mod_name == NULL) {
        unknown_name = PyUnicode_FromString("<unknown module name>");
        if (unknown_name == NULL) {
            goto done;
        }
        mod_name_or_unknown = unknown_name;
    }
    else {
        mod_name_or_unknown = mod_name;
    }

    _PyStatusValue spec_attr = _PyValue_GetOptionalAttr(
        v, &_Py_ID(__spec__));
    if (spec_attr.status < 0) {
        goto done;
    }
    int has_spec = spec_attr.status > 0;
    if (has_spec) {
        spec_value = spec_attr.value;
    }
    if (!has_spec) {
        errmsg = PyUnicode_FromFormat(
            "cannot import name %R from %R (unknown location)",
            name, mod_name_or_unknown
        );
        goto done_with_errmsg;
    }
    int origin_status = _PyModuleSpec_GetFileOriginValue(
        PyValue_Borrow(spec_value), &origin_value);
    if (origin_status < 0) {
        goto done;
    }
    if (origin_status > 0) {
        origin = PyValue_AsKnownObjectBorrow(origin_value);
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
            is_possibly_shadowing_stdlib = PySet_ContainsValue(
                stdlib_modules, PyValue_FromObjectBorrow(mod_name_or_unknown));
            if (is_possibly_shadowing_stdlib < 0) {
                Py_XDECREF(stdlib_modules);
                goto done;
            }
        }
        Py_XDECREF(stdlib_modules);
    }

    if (origin == NULL && PyValue_IsObject(v) &&
        PyModule_Check(PyValue_AsKnownObjectBorrow(v)))
    {
        module_origin = PyModule_GetFilenameObject(
            PyValue_AsKnownObjectBorrow(v));
        origin = module_origin;
        if (module_origin == NULL) {
            if (!PyErr_ExceptionMatches(PyExc_SystemError)) {
                goto done;
            }
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
        int rc = _PyModuleSpec_IsInitializingValue(
            PyValue_Borrow(spec_value));
        if (rc < 0) {
            goto done;
        }
        else if (rc > 0) {
            if (is_possibly_shadowing) {
                assert(origin);
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
        _PyErr_SetImportErrorWithNameFrom(errmsg, mod_name, origin, name);
        Py_DECREF(errmsg);
    }

done:
    Py_XDECREF(module_origin);
    PyValue_XCLOSE(origin_value);
    PyValue_XCLOSE(spec_value);
    PyValue_XCLOSE(mod_name_value);
    Py_XDECREF(unknown_name);
    return PyValue_NULL;
}

PyValue
_PyEval_ImportFromValue(PyThreadState *tstate, PyValueBorrow v, PyObject *name)
{
    _PyStatusValue attr = _PyValue_GetOptionalAttr(v, name);
    if (attr.status > 0) {
        return attr.value;
    }
    if (attr.status < 0) {
        return PyValue_NULL;
    }
    return _eval_import_from_miss(tstate, v, name);
}

PyObject *
_PyEval_ImportFrom(PyThreadState *tstate, PyObject *v, PyObject *name)
{
    PyValue result = _PyEval_ImportFromValue(
        tstate, PyValue_FromObjectBorrow(v), name);
    if (PyValue_IsNull(result)) {
        return NULL;
    }
    return _PyValue_AsObjectStealAt(result, BOX_SITE_ATTRIBUTE);
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
            PyValueBorrow exc = PyTuple_GET_VALUE(right, i);
            if (!PyValue_IsObject(exc) ||
                !PyExceptionClass_Check(PyValue_AsKnownObjectBorrow(exc))) {
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
    if (Py_TYPE(args)->tp_iter == NULL && !_PyValue_SequenceCheck(PyValue_FromObjectBorrow(args))) {
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
                (void)_PyValue_SetAttr(PyValue_FromObjectBorrow(exc), &_Py_ID(name), PyValue_FromObjectBorrow(obj));
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
                        PyValue *writeto)
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
        *writeto = PyValue_FromObjectSteal(res);
    }

    PyObject *res_o = PyValue_IsObject(*writeto) ?
        PyValue_AsKnownObjectBorrow(*writeto) : NULL;
    if (res_o != NULL && PyLazyImport_CheckExact(res_o)) {
        PyObject *l_v = _PyLazyImport_Reify(
            PyThreadState_GET(), res_o, name, namespace);
        // a move: replaced below
        PyValue old = *writeto;
        *writeto = l_v == NULL ? PyValue_NULL : PyValue_FromObjectSteal(l_v);
        *writeto = _PyValue_Publish(writeto);
        PyValue_CLOSE(old);
    }
    if (!PyValue_IsNull(*writeto)) {
        *writeto = _PyValue_Publish(writeto);
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

PyValue
_PyEval_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *name)
{
    PyObject *namespace = frame->f_locals;
    if (namespace == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "no locals found");
        return PyValue_NULL;
    }
    _PyStatusValue found = value_load_from_dict(tstate, namespace, name);
    if (found.status != 0) {
        return found.value;
    }
    namespace = frame->f_globals;
    found.status = PyDict_GetValue(namespace, PyValue_FromObjectBorrow(name),
                                   &found.value);
    if (found.status < 0) {
        return PyValue_NULL;
    }
    if (found.status > 0) {
        if (PyValue_IsObject(found.value) &&
            PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(found.value))) {
            PyObject *reified = _PyLazyImport_Reify(
                tstate, PyValue_AsKnownObjectBorrow(found.value), name, namespace);
            PyValue_CLOSE(found.value);
            return _PyValue_FromObjectResult(reified);
        }
        return found.value;
    }
    namespace = frame->f_builtins;
    found = value_load_from_dict(tstate, namespace, name);
    if (found.status != 0) {
        return found.value;
    }
    _PyEval_FormatExcCheckArg(tstate, PyExc_NameError, NAME_ERROR_MSG, name);
    return PyValue_NULL;
}

/* Out of line, as when it was exported. */
static Py_NO_INLINE _PyStatusValue
_PyForIter_VirtualIteratorNext(PyThreadState *tstate, _PyInterpreterFrame *frame,
                               PyValue iter, PyValue *index_ptr)
{
    _PyStatusValue result = {0, PyValue_NULL};
    PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
    PyValueBorrow index = PyValue_Borrow(*index_ptr);
    if (PyValue_IsTaggedIndex(index)) {
        intptr_t i = PyValue_UntagIndex(index);
        assert(i >= 0);
        _PyValueIndexPair next_index = Py_TYPE(iter_o)->_tp_iteritem(iter_o, i);
        i = next_index.index;
        result.value = next_index.value;
        if (PyValue_IsNull(next_index.value)) {
            result.status = i < 0 ? -1 : 0;
            return result;
        }
        _PyFrame_WriteHome(frame, index_ptr, PyValue_TagIndex(i));
        result.status = 1;
        return result;
    }
    _PyStatusValue step = _PyValue_IterStep(iter_o);
    if (step.status == PYGEN_NEXT) {
        result.value = step.value;
        result.status = 1;
        return result;
    }
    if (step.status == PYGEN_RETURN) {
        /* The semantic completion reports an implicit StopIteration once.
         * Keep the returned owner without fabricating a separate RAISE. */
        result.value = step.value;
        return result;
    }
    if (_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
        _PyEval_MonitorRaise(tstate, frame, frame->instr_ptr);
        _PyErr_Clear(tstate);
    }
    else {
        result.status = -1;
    }
    return result;
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
