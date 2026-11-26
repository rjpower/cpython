/* Execute compiled code */

#include "ceval.h"
#include "pycore_attr_recipe.h"   // _PyAttr_AdmittedInstanceSlot()
#include "pycore_global_recipe.h" // _PyGlobalRecipe
#include "pycore_descrobject.h"   // _PyGetSet_ValueRead()
#include "pycore_dict.h"          // PyDict_GetValue()
#include "pycore_function.h"      // FUNC_VERSION_FIRST_VALID
#include "pycore_jit_lower.h"     // _PyEval_JitRaiseExit
#include "pycore_jit.h"           // _PyJitUnit
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_long.h"
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_native_scope.h"
#include "pycore_rop_metadata.h"  // ROP_HANDLER_*
#include "pycore_rop_rows.h"      // _PyRop_AnchorForCall()
#include "pycore_setobject.h"     // PySet_AddValue()
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
            "'%N' object does not support the context manager protocol "
            "(missed __enter__ method)"
        ),
        .error_suggestion = (
            "'%N' object does not support the context manager protocol "
            "(missed __enter__ method) but it supports the asynchronous "
            "context manager protocol. Did you mean to use 'async with'?"
        )
    },
    [SPECIAL___EXIT__] = {
        .name = &_Py_ID(__exit__),
        .error = (
            "'%N' object does not support the context manager protocol "
            "(missed __exit__ method)"
        ),
        .error_suggestion = (
            "'%N' object does not support the context manager protocol "
            "(missed __exit__ method) but it supports the asynchronous "
            "context manager protocol. Did you mean to use 'async with'?"
        )
    },
    [SPECIAL___AENTER__] = {
        .name = &_Py_ID(__aenter__),
        .error = (
            "'%N' object does not support the asynchronous "
            "context manager protocol (missed __aenter__ method)"
        ),
        .error_suggestion = (
            "'%N' object does not support the asynchronous context manager "
            "protocol (missed __aenter__ method) but it supports the context "
            "manager protocol. Did you mean to use 'with'?"
        )
    },
    [SPECIAL___AEXIT__] = {
        .name = &_Py_ID(__aexit__),
        .error = (
            "'%N' object does not support the asynchronous "
            "context manager protocol (missed __aexit__ method)"
        ),
        .error_suggestion = (
            "'%N' object does not support the asynchronous context manager "
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
    /* The caller owns map and keys; dummy is owned here. */
    PyValueBorrow map_v = PyValue_FromObjectBorrow(map);
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
        if (_PySet_ContainsValue((PySetObject *)seen, key_v) || _PySet_AddValue((PySetObject *)seen, key_v)) {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!
                PyValue key_owner = _PyTuple_GetValue(keys, i);
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
            int r = _PyDict_GetValue((PyDictObject *)map, key_v, &value);
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
            PyValue args[3];
            _PyValue_WindowPut(&args[0], map_v);
            _PyValue_WindowPut(&args[1], key_v);
            _PyValue_WindowPut(&args[2], PyValue_FromObjectBorrow(dummy));
            value = has_self
                ? _PyValue_Vectorcall(get, args, 3, NULL)
                : _PyValue_Vectorcall(get, &args[1], 2, NULL);
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
    Py_DECREF(seen);
    Py_DECREF(dummy);
    Py_XDECREF(key);
    return values;
fail:
    PyValue_XCLOSE(get);
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
        if (_PySet_ContainsValue((PySetObject *)seen, PyValue_FromObjectBorrow(name)) || _PySet_AddValue((PySetObject *)seen, PyValue_FromObjectBorrow(name))) {
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
(_PyEval_MatchClass)(PyThreadState *tstate, PyValueBorrow subject, PyValueBorrow type_value,
                     Py_ssize_t nargs, PyObject *kwargs)
{
    if (!PyValue_IsObject(type_value) ||
        !PyType_Check(PyValue_AsKnownObjectBorrow(type_value))) {
        const char *e = "class pattern must refer to a class";
        _PyErr_Format(tstate, PyExc_TypeError, e);
        return NULL;
    }
    PyObject *type = PyValue_AsKnownObjectBorrow(type_value);
    assert(PyTuple_CheckExact(kwargs));
    // First, an isinstance check:
    int isinstance = _PyValue_IsInstance(subject, PyValue_FromObjectBorrow(type));
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


static int do_raise(PyThreadState *tstate, PyObject *exc, PyObject *cause,
                    bool cause_is_number);

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

/* The flags of a generic call (call_run_*()): what the handler is and what it
 * knows of its word, in its register local call_state.  CALL_EFFECTS marks a
 * call that specialization already acted on, which finishes on its word
 * generically. */
#define CALL_EFFECTS 1
#define CALL_INSTRUMENTED 8
#define CALL_PERIODIC 16
#define CALL_SELF_PRESENT 32
#define CALL_KWARGS_PRESENT 64
/* The callable operand is a bound method whose function and receiver the
 * push takes in its place: the operands stay the method and NULL. */
#define CALL_EXPANDED 256

/* What a call handler reads from its word: whether the call has a self
 * operand (CALL and CALL_KW), whether it has a kwargs operand
 * (CALL_FUNCTION_EX), and the number of words of the instruction.  The
 * operands are read through the words after the head (pycore_ir.h):
 * IR_OPERAND(k) is the slot of operand k, a view that the handler reads once
 * and does not close. */
/* The self operand of a method call that LOAD_ATTR resolved to a plain
 * attribute (a module member, a class attribute) is NULL at run time: the
 * call then has no self. */
#define IR_CALL_SELF_STATE(W) \
    ((((W) & IR_F_NO_SELF) || PyValue_IsNull(IR_OPERAND(0))) \
     ? 0 : CALL_SELF_PRESENT)
#define IR_CALL_KWARGS_STATE(W) (((W) & IR_F_NO_KW) ? 0 : CALL_KWARGS_PRESENT)
#define IR_CALL_LENGTH(W) _PyIR_CallWordLength(W)
#define IR_CALL_HAS_SELF(W) (!((W) & IR_F_NO_SELF))
#define IR_OPERAND(K) IR_SLOT(_PyIR_CallOperand(this_instr, (K)))
/* Argument K of the call as the specialized handlers count them: the self
 * when there is one, else the first argument, which follows the self
 * operand's slot if the word has one that is NULL at run time. */
#define IR_CALL_ARG(K) \
    IR_OPERAND((K) + ((call_state & CALL_SELF_PRESENT) ? 0 : IR_CALL_HAS_SELF(ir_word)))
/* The first operand that a native handler passes to the callee: operand 0,
 * or operand 1 when the word has a self operand that is NULL at run time. */
#define IR_CALL_FIRST() \
    ((call_state & CALL_SELF_PRESENT) ? 0 : IR_CALL_HAS_SELF(ir_word))
/* The tuple of a CALL_KW's keyword names: its last operand is the index in
 * co_consts, which the code keeps alive. */
#define IR_CALL_KWNAMES() \
    _PyIR_AdoptTemp(PyTuple_GET_VALUE(FRAME_CO_CONSTS, \
        _PyIR_CallOperand(this_instr, (int)IR_C(ir_word) - 1)))

typedef enum {
    CALL_SELF_ABSENT,
    CALL_SELF_SUPPLIED,
} _PyCallSelfRole;

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

/* container[key] for GetItem.Generic: an exact dict, and an exact list
 * or tuple indexed by an inline int in range, as list_subscript() and
 * tuple_subscript() do, read the item as a value; anything else is
 * _PyValue_GetItem(). A dict miss raises KeyError with the key as it is.
 * The caller holds an owner of both. */
static PyValue
value_subscript(PyValueBorrow container, PyValueBorrow key)
{
    PyObject *o = PyValue_ObjectOrNull(container);
    if (o != NULL) {
        if (PyAnyDict_CheckExact(o)) {
            PyValue result = PyValue_NULL;
            int found = _PyDict_GetValue((PyDictObject *)o, key, &result);
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
                        : _PyTuple_GetValue(o, index);
                }
            }
        }
    }
    return _PyValue_GetItem(container, key);
}

/* lhs op rhs for BINARY_OP's operation oparg; both operands are borrowed. */
static PyValue
value_binary_op(int oparg, PyValueBorrow lhs, PyValueBorrow rhs)
{
    return _PyValue_BinaryOps[oparg](lhs, rhs);
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

PyObject *
_Py_BuildSet_ValueSteal(
    _PyInterpreterFrame *frame,
    PyValue *arguments,
    int total_args)
{
    /* As _Py_BuildMap_ValueSteal: a window of temporaries that own their
     * values.  Each value is taken from its home and moves into the set,
     * or is closed once an add has failed (or the set was not made); every
     * home is empty on return. */
    _PyValue_RecheckWindow(arguments, total_args);
    PyObject *set = PySet_New(NULL);
    int err = set == NULL;
    for (int i = 0; i < total_args; i++) {
        PyValue value = _PyFrame_TakeHome(frame, &arguments[i]);
        if (err == 0) {
            err = _PySet_AddValueTakeRef((PySetObject *)set, value);
        }
        else {
            PyValue_CLOSE(value);
        }
    }
    if (err) {
        Py_XDECREF(set);
        return NULL;
    }
    return set;
}

/* Out of line, as super_call: exit_func(exit_self, exc, val, tb), or
 * exit_func(exc, val, tb) when exit_self is NULL.  exit_func is any attribute
 * value of the context manager's type, an inline scalar included, which the
 * call rejects with upstream's TypeError.  The words are borrowed. */
static Py_NO_INLINE PyValue
with_exit_call(PyValueBorrow exit_func, PyValueBorrow exit_self,
               PyObject *exc, PyObject *val, PyObject *tb)
{
    PyValue stack[4];
    int has_self = !PyValue_IsNull(exit_self);
    _PyValue_WindowPut(&stack[0], exit_self);
    _PyValue_WindowPut(&stack[1], PyValue_FromObjectBorrow(exc));
    _PyValue_WindowPut(&stack[2], PyValue_FromObjectBorrow(val));
    _PyValue_WindowPut(&stack[3], PyValue_FromObjectBorrow(tb));
    return _PyValue_Vectorcall(exit_func, stack + 1 - has_self, 3 + has_self,
                               NULL);
}

#define with_exit_call(F, S, E, V, T) \
    (with_exit_call)(PyValue_Borrow(F), PyValue_Borrow(S), E, V, T)

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

/* The entry frame's words: the Return arm of the entry frame
 * (Return.External).  Its slot 1 is a temporary, which the arm takes, so the
 * word carries no slot-class flag; the header precedes the words, as in
 * every executing copy, for IR_IS_LOCAL(). */
static const struct {
    _PyCodeExec exec;
    _PyIRWord words[1];
} entry_trampoline = {
    .exec = { .nwords = 1, .nfixed = 1 },
    .words = {
        /* reached on return and yield */
        IR_WORD(_PyRop_SelectReturn(ROP_SELECT_ENTRY_FRAME, ROP_SELECT_VACANT_SCRATCH),
                1, 0, 0, 0),
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
    if (_PyType_IteratesByIndex(tp)) {
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

/* The asynchronous iterator of obj, as GET_AITER pushes it, or the null value
 * with an exception set.  Consumes obj. */
static PyValue
_PyEval_GetAIter(PyThreadState *tstate, PyValue obj)
{
    unaryfunc getter = NULL;
    // A number has no __aiter__
    if (!PyValue_IsObject(obj)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'async for' requires an object with "
                      "__aiter__ method, got %.100s",
                      PyValue_TYPE(obj)->tp_name);
        return PyValue_NULL;
    }
    PyObject *obj_o = PyValue_AsKnownObjectBorrow(obj);
    PyTypeObject *type = Py_TYPE(obj_o);

    if (type->tp_as_async != NULL) {
        getter = type->tp_as_async->am_aiter;
    }

    if (getter == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'async for' requires an object with "
                      "__aiter__ method, got %.100s",
                      type->tp_name);
        PyValue_CLOSE(obj);
        return PyValue_NULL;
    }

    PyObject *iter_o = (*getter)(obj_o);
    PyValue_CLOSE(obj);
    if (iter_o == NULL) {
        return PyValue_NULL;
    }

    if (Py_TYPE(iter_o)->tp_as_async == NULL ||
            Py_TYPE(iter_o)->tp_as_async->am_anext == NULL) {

        _PyErr_Format(tstate, PyExc_TypeError,
                      "'async for' received an object from __aiter__ "
                      "that does not implement __anext__: %.100s",
                      Py_TYPE(iter_o)->tp_name);
        Py_DECREF(iter_o);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(iter_o);
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
        return _PyDict_SetValue((PyDictObject *)ns, PyValue_FromObjectBorrow(name), v);
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
                               PyValueBorrow iter, PyValue *index_ptr);
static inline Py_ALWAYS_INLINE _PyForIterStep
_PyForIter_VirtualIteratorNextInline(PyThreadState *tstate,
                                     _PyInterpreterFrame *frame,
                                     PyValueBorrow iter, PyValue cursor);
static int _PyForIter_StopIsExhaustion(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame);

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
static Py_NO_INLINE int
value_operation_refresh_failed(PyThreadState *tstate,
                               _PyInterpreterFrame *frame, _PyInstr *current,
                               PyValue failed_result)
{
    if (tstate->current_frame != frame) {
        value_revalidation_error("value bridge changed the active frame");
        PyValue_XCLOSE(failed_result);
        return -1;
    }
    /* Keep a pending operation error. */
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_SystemError, "value bridge changed executing words");
    }
    PyValue_XCLOSE(failed_result);
    return -1;
}

static inline Py_ALWAYS_INLINE int
value_operation_refresh(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *current, PyValue failed_result)
{
    if (tstate->current_frame != frame) {
        return value_operation_refresh_failed(tstate, frame, current,
                                              failed_result);
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || frame->ir_words != exec->words ||
        current < exec->words || current >= exec->words + exec->nwords) {
        return value_operation_refresh_failed(tstate, frame, current,
                                              failed_result);
    }
    assert(instrumentation_is_current(tstate, code));
    frame->instr_ptr = current;
    return 0;
}

/* Operand k of the call whose head is head, as an owner: a local is counted
 * and a temporary gives up its home.  The caller reads each operand it takes
 * once, before anything runs Python. */
static inline PyValue
call_operand_take(_PyInterpreterFrame *frame, const _PyIRWord *head, int k)
{
    PyValue *home = &_PyFrame_Slots(frame)[_PyIR_CallOperand(head, k)];
    if (head[1 + k / IR_OPERANDS_PER_WORD] &
        (IR_F_LOCAL_A << (k % IR_OPERANDS_PER_WORD))) {
        return PyValue_DUP(*home);
    }
    PyValue owner = *home;
    *home = PyValue_NULL;
    return owner;
}

/* The word after the call that starts at current: the pc to which its
 * callee returns.  A call is a head and its operand words. */
static inline _PyInstr *
call_return_pc(_PyInstr *current)
{
    return current + _PyIR_CallWordLength(FT_ATOMIC_LOAD_UINT64_RELAXED(*current));
}

/* The release of one temporary home of a call: the owner gives up its home,
 * and a count that is not the last closes at once, which runs nothing.
 * Returns 1 and leaves the owner in its home when it holds the last count,
 * which call_release_rest() closes.  A home may be NULL: a self operand's
 * is, and a callable that a Python callee took from its home. */
static inline Py_ALWAYS_INLINE int
call_release_home(PyValue *slots, uint32_t home)
{
    PyValue owner = slots[home];
    if (PyValue_RefcountOnObject(owner)) {
        if (Py_REFCNT(BITS_TO_PTR(owner)) <= 1) {
            return 1;
        }
        Py_DECREF_MORTAL(BITS_TO_PTR(owner));
    }
    slots[home] = PyValue_NULL;
    return 0;
}

/* The release pass over what a native call of at most IR_OPERANDS_PER_WORD
 * operands owes: its temporary operands, which are all in the first operand
 * word, and its callable, each taken from its home before it closes.  The
 * facts of the word say what is owed (IR_F_CALL_OWES_CALLABLE and
 * IR_F_CALL_OWES_OPERANDS), so a call that names only locals reads none of
 * them.  OPERANDS is the number of operands that name slots: all of the
 * word's, but the names of a CALL_KW.  A local is only borrowed, and so is
 * the home named DST, which the caller has written.
 *
 * An owner that holds the last count is left in its home, which keeps the
 * loop free of deallocations (and so of the registers that a call would cost
 * every handler); the others close at once, which runs nothing.  The return
 * value says whether a pass is still owed, and call_release_rest() makes it. */
static inline Py_ALWAYS_INLINE int
call_release_pass(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                  int operands, uint32_t dst)
{
    int owed = 0;
    if (word & IR_F_CALL_OWES_OPERANDS) {
        _PyIRWord ops = head[1];
        uint64_t homes = ops >> 16;
        for (int k = 0; k < operands; k++, homes >>= 12) {
            uint32_t home = (uint32_t)(homes & IR_FIELD_MAX);
            if (home != dst && !(ops & (IR_F_LOCAL_A << k))) {
                owed |= call_release_home(slots, home);
            }
        }
    }
    if (word & IR_F_CALL_OWES_CALLABLE) {
        owed |= call_release_home(slots, IR_B(word));
    }
    return owed;
}

/* The pass for any count of operands, and the one that call_release_pass()
 * owes.  It reads the fields and the slot-class flags of the words, not the
 * facts of the head, so a call that has not stored its result may name a dst
 * that nothing went to (UINT32_MAX).  It walks the operand words, four
 * operands to a word, and moves the owners that hold the last count to
 * OWNERS, which then close in reverse order, so that all of them have left
 * the frame before the first deallocation can run Python (handlers81
 * rule 5). */
static inline Py_ALWAYS_INLINE void
call_release_owners(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                    int operands, uint32_t dst, PyValue *owners)
{
    int last = 0;
#define CALL_DETACH(HOME) \
    do { \
        uint32_t home_ = (HOME); \
        PyValue owner_ = slots[home_]; \
        slots[home_] = PyValue_NULL; \
        if (PyValue_RefcountOnObject(owner_)) { \
            if (Py_REFCNT(BITS_TO_PTR(owner_)) <= 1) { \
                owners[last++] = owner_; \
            } \
            else { \
                Py_DECREF_MORTAL(BITS_TO_PTR(owner_)); \
            } \
        } \
    } while (0)
    for (int w = 0, left = operands; left > 0;
         w++, left -= IR_OPERANDS_PER_WORD)
    {
        _PyIRWord ops = head[1 + w];
        uint64_t homes = ops >> 16;
        int count = left < IR_OPERANDS_PER_WORD ? left : IR_OPERANDS_PER_WORD;
        for (int k = 0; k < count; k++, homes >>= 12) {
            uint32_t home = (uint32_t)(homes & IR_FIELD_MAX);
            if (home != dst && !(ops & (IR_F_LOCAL_A << k))) {
                CALL_DETACH(home);
            }
        }
    }
    uint32_t callable = IR_B(word);
    if (callable != dst && !(word & IR_F_LOCAL_B)) {
        CALL_DETACH(callable);
    }
#undef CALL_DETACH
    while (last > 0) {
        PyValue_XCLOSE(owners[--last]);
    }
}

/* The pass for a call of at most IR_OPERANDS_PER_WORD operands, whose owners
 * are in a fixed array: no variable-length array, so no stack check. */
static Py_NO_INLINE void
call_release_small(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                   int operands, uint32_t dst)
{
    PyValue owners[IR_OPERANDS_PER_WORD + 1];
    call_release_owners(slots, head, word, operands, dst, owners);
}

/* The pass for a longer call.  The operands and the callable are at most
 * IR_FIELD_MAX + 1. */
static Py_NO_INLINE void
call_release_wide(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                  int operands, uint32_t dst)
{
    PyValue owners[operands + 1];
    call_release_owners(slots, head, word, operands, dst, owners);
}

/* The pass that call_release_pass() owes, and the one for a longer call:
 * the handlers call this one entry. */
static Py_NO_INLINE void
call_release_rest(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                  int operands, uint32_t dst)
{
    if (operands > IR_OPERANDS_PER_WORD) {
        call_release_wide(slots, head, word, operands, dst);
        return;
    }
    call_release_small(slots, head, word, operands, dst);
}

/* Release the temporary operands and the callable of a call: its owners that
 * hold other counts at once, and the rest out of line.  A longer call than
 * one operand word holds is all out of line. */
static inline Py_ALWAYS_INLINE void
call_release_operands(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                      int operands, uint32_t dst)
{
    if (Py_UNLIKELY(((word & IR_F_CALL_OWES_OPERANDS) &&
                     operands > IR_OPERANDS_PER_WORD) ||
                    call_release_pass(slots, head, word, operands, dst))) {
        call_release_rest(slots, head, word, operands, dst);
    }
}

/* The result of a native call, which the handler computed from the operand
 * homes themselves: store it in dst, then release the call's temporaries
 * (call_release_operands).  The result goes first (handlers81 rules 6 and 7),
 * so a dst that is one of the temporary homes, or a local, displaces its old
 * value, which the caller closes last (ir_old). */
static inline Py_ALWAYS_INLINE PyValue
call_publish(_PyInterpreterFrame *frame, const _PyIRWord *head, _PyIRWord word,
             PyValue result, int operands)
{
    PyValue *slots = _PyFrame_Slots(frame);
    uint32_t dst = IR_A(word);
    PyValue displaced = slots[dst];
    _PyValue_CheckHome(result);
    slots[dst] = result;
    call_release_operands(slots, head, word, operands, dst);
    return displaced;
}

/* call_publish out of line, for the error edges of a handler that publishes
 * its result before the periodic check: one copy of the loop instead of one
 * per handler. */
static Py_NO_INLINE PyValue
call_publish_cold(_PyInterpreterFrame *frame, const _PyIRWord *head,
                  _PyIRWord word, PyValue result, int operands)
{
    return call_publish(frame, head, word, result, operands);
}

/* Copy N operands of the call at HEAD, from operand FIRST on, to ARGS.  They
 * are all in the first operand word (FIRST + N is at most
 * IR_OPERANDS_PER_WORD), which is read once. */
#define CALL_NATIVE_GATHER(HEAD, SLOTS, FIRST, N, ARGS) \
    do { \
        uint64_t fields_ = (HEAD)[1] >> (16 + 12 * (FIRST)); \
        for (int i_ = 0; i_ < (N); i_++) { \
            (ARGS)[i_] = (SLOTS)[fields_ & IR_FIELD_MAX]; \
            fields_ >>= 12; \
        } \
    } while (0)

/* The same for any count, across the operand words after head: from operand
 * FIRST on, N operands, four to a word. */
static inline Py_ALWAYS_INLINE void
call_gather_words(PyValue *slots, const _PyIRWord *head, int first, int n,
                  PyValue *args)
{
    int w = first / IR_OPERANDS_PER_WORD;
    int s = first % IR_OPERANDS_PER_WORD;
    for (; n > 0; w++, s = 0) {
        int count = IR_OPERANDS_PER_WORD - s;
        if (count > n) {
            count = n;
        }
        CALL_NATIVE_GATHER(&head[w], slots, s, count, args);
        args += count;
        n -= count;
    }
}

/* The native vectorcall of a call, on its operands as they stand in their
 * homes: they are borrowed for the call, as a temporary cannot change and a
 * local's old value survives a write through its frame's proxy.  The caller
 * publishes the result, which releases the temporaries.  The arguments follow
 * the self slot, which is NULL at run time for a class attribute that is not
 * a method.  receiver, if it is not NULL, goes before them: the self of a
 * bound method that the call expanded.  keywords: the last operand is the
 * index of the keyword names in co_consts. */
static inline Py_ALWAYS_INLINE PyValue
call_vector_impl(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 const _PyIRWord *head, PyValue callable, PyValue receiver,
                 int keywords)
{
    _PyIRWord word = *head;
    PyValue *slots = _PyFrame_Slots(frame);
    int operands = (int)IR_C(word) - keywords;
    int first = (word & IR_F_NO_SELF) == 0 &&
        PyValue_IsNull(slots[_PyIR_CallOperand(head, 0)]);
    int lead = !PyValue_IsNull(receiver);
    int nargs = lead + operands - first;
    PyValue small[16];
    PyValue *args = small;
    if (nargs > (int)Py_ARRAY_LENGTH(small)) {
        args = PyMem_Malloc((size_t)nargs * sizeof(PyValue));
        if (args == NULL) {
            PyErr_NoMemory();
            return PyValue_NULL;
        }
    }
    if (lead) {
        args[0] = receiver;
    }
    if (operands <= IR_OPERANDS_PER_WORD) {
        CALL_NATIVE_GATHER(head, slots, first, operands - first, args + lead);
    }
    else {
        call_gather_words(slots, head, first, operands - first, args + lead);
    }
    PyObject *names = NULL;
    size_t positional = (size_t)nargs;
    if (keywords) {
        names = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(
            _PyFrame_GetCode(frame)->co_consts,
            _PyIR_CallOperand(head, (int)IR_C(word) - 1)));
        positional -= (size_t)PyTuple_GET_SIZE(names);
    }
    PyValue result = _PyValue_VectorcallTstate(tstate, callable, args,
                                               positional, names);
    if (args != small) {
        PyMem_Free(args);
    }
    return result;
}

static Py_NO_INLINE PyValue
call_vector(PyThreadState *tstate, _PyInterpreterFrame *frame,
            const _PyIRWord *head, PyValue callable)
{
    return call_vector_impl(tstate, frame, head, callable, PyValue_NULL, 0);
}

/* The native call of a specialized handler: the callee reads operands first
 * to the end of the call's operand list, copied to a C array that the callee
 * borrows for the call.  The copy is made before the callee runs, so the
 * callee sees the operands as they stood at entry (the old value of a local
 * that the callee rebinds through its frame's proxy stays alive in
 * f_overwritten_fast_locals, and a temporary cannot change).  The handler
 * publishes the result and releases the temporaries, as after call_vector().
 *
 * A callee written against the value API (METH_VALUE) is called through its
 * value entry; an object-API callee through the object vectorcall of the
 * function or descriptor, which boxes the inline words, as the dispatcher of
 * _PyValue_VectorcallTstate() does.  The handler's guard checked the call
 * flags, so the bridge is one test of METH_VALUE.  The recursion limit is
 * checked by the guard (_CHECK_RECURSION_LIMIT), which deopts to the generic
 * path that raises.
 *
 * More than IR_OPERANDS_PER_WORD operands are gathered by call_vector(). */

/* A builtin function.  Its handler's guard checked the call flags and the
 * number of arguments of a fixed-arity value form (METH_VALUE | METH_O or
 * METH_NOARGS). */
static inline Py_ALWAYS_INLINE PyValue
call_builtin_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    const _PyIRWord *head, _PyIRWord word,
                    PyValue callable, int first)
{
    PyCFunctionObject *func =
        (PyCFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
    int flags = func->m_ml->ml_flags;
    int n = (int)IR_C(word) - first;
    assert(!(flags & (METH_O | METH_NOARGS)) || n == ((flags & METH_O) ? 1 : 0));
    if (Py_UNLIKELY(first + n > IR_OPERANDS_PER_WORD)) {
        return call_vector(tstate, frame, head, callable);
    }
    PyValue args[IR_OPERANDS_PER_WORD];
    CALL_NATIVE_GATHER(head, _PyFrame_Slots(frame), first, n, args);
    if (flags & METH_VALUE) {
        _PyCFunctionValue meth = (_PyCFunctionValue)(void(*)(void))
                                 func->m_ml->ml_meth;
        PyValue result = meth(_PyCFunction_GET_SELF_VALUE((PyObject *)func),
                              args, n, NULL);
        _PyValue_ASSERT_RESULT(tstate, result);
        return result;
    }
    return _PyValue_CallAdapter(
        tstate, (PyObject *)func, func->vectorcall, args, (size_t)n, NULL);
}

/* A builtin class that has a vectorcall (its guard checked): the class's value
 * vectorcall, else its object vectorcall through the adapter. */
static inline Py_ALWAYS_INLINE PyValue
call_class_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  const _PyIRWord *head, _PyIRWord word, PyValue callable,
                  int first)
{
    int n = (int)IR_C(word) - first;
    if (Py_UNLIKELY(first + n > IR_OPERANDS_PER_WORD)) {
        return call_vector(tstate, frame, head, callable);
    }
    PyValue args[IR_OPERANDS_PER_WORD];
    CALL_NATIVE_GATHER(head, _PyFrame_Slots(frame), first, n, args);
    PyTypeObject *cls = (PyTypeObject *)PyValue_AsKnownObjectBorrow(callable);
    _Py_vectorcallvaluefunc value_call;
    memcpy(&value_call, &cls->tp_value_vectorcall, sizeof(value_call));
    if (value_call != NULL) {
        PyValue result = value_call(tstate, (PyObject *)cls, args, (size_t)n,
                                    NULL);
        _PyValue_ASSERT_RESULT(tstate, result);
        return result;
    }
    return _PyValue_CallAdapter(tstate, (PyObject *)cls, cls->tp_vectorcall,
                                args, (size_t)n, NULL);
}

/* A method descriptor: the receiver is operand first, and the callee takes it
 * with the arguments.  Its handler's guard checked the call flags, the number
 * of arguments and that the receiver is an exact instance of the
 * descriptor's type. */
static inline Py_ALWAYS_INLINE PyValue
call_descriptor_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       const _PyIRWord *head, _PyIRWord word,
                       PyValue callable, int first)
{
    PyMethodDescrObject *descr =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(callable);
    int flags = descr->d_method->ml_flags;
    int n = (int)IR_C(word) - first;
    assert(n > 0);
    assert(!(flags & (METH_O | METH_NOARGS)) || n == ((flags & METH_O) ? 2 : 1));
    PyValue *slots = _PyFrame_Slots(frame);
    assert(PyValue_TYPE(slots[_PyIR_CallOperand(head, first)]) ==
           descr->d_common.d_type);
    if (Py_UNLIKELY(first + n > IR_OPERANDS_PER_WORD)) {
        return call_vector(tstate, frame, head, callable);
    }
    PyValue args[IR_OPERANDS_PER_WORD];
    CALL_NATIVE_GATHER(head, slots, first, n, args);
    if (flags & METH_VALUE) {
        _PyCFunctionValue meth = (_PyCFunctionValue)(void(*)(void))
                                 descr->d_method->ml_meth;
        PyValue result = meth(PyValue_Borrow(args[0]), args + 1, n - 1, NULL);
        _PyValue_ASSERT_RESULT(tstate, result);
        return result;
    }
    return _PyValue_CallAdapter(
        tstate, (PyObject *)descr, descr->vectorcall, args, (size_t)n, NULL);
}

/* Move or count the first count (at most four) operands of fields, the twelve
 * bit slot fields of an operand word, and locals, its local bits, into dest:
 * a local is counted, a temporary moves out of its home. */
static inline Py_ALWAYS_INLINE void
call_take_fields(PyValue *slots, uint64_t fields, unsigned int locals,
                 int count, PyValue *dest)
{
#define CALL_TAKE(K) \
    if (count > (K)) { \
        PyValue *home = &slots[(fields >> (12 * (K))) & IR_FIELD_MAX]; \
        PyValue value = *home; \
        if (locals & (1u << (K))) { \
            value = PyValue_DUP(value); \
        } \
        else { \
            *home = PyValue_NULL; \
        } \
        dest[K] = value; \
    }
    CALL_TAKE(0)
    CALL_TAKE(1)
    CALL_TAKE(2)
    CALL_TAKE(3)
#undef CALL_TAKE
}

/* The same for any count, across the operand words after head: count
 * operands from operand first on, four to a word. */
static Py_NO_INLINE void
call_take_loop(PyValue *slots, const _PyIRWord *head, int first, int count,
               PyValue *dest)
{
    int w = first / IR_OPERANDS_PER_WORD;
    int s = first % IR_OPERANDS_PER_WORD;
    for (; count > 0; w++, s = 0) {
        int n = IR_OPERANDS_PER_WORD - s;
        if (n > count) {
            n = count;
        }
        _PyIRWord ops = head[1 + w];
        call_take_fields(slots, ops >> (16 + 12 * s),
                         (unsigned int)(ops >> (9 + s)), n, dest);
        dest += n;
        count -= n;
    }
}

static inline Py_ALWAYS_INLINE void
call_take_operands(PyValue *slots, const _PyIRWord *head, int first, int count,
                   PyValue *dest)
{
    if (!Py_UNLIKELY(first + count > IR_OPERANDS_PER_WORD)) {
        _PyIRWord ops = head[1];
        call_take_fields(slots, ops >> (16 + 12 * first),
                         (unsigned int)(ops >> (9 + first)), count, dest);
        return;
    }
    call_take_loop(slots, head, first, count, dest);
}

/* Release the callable operand of a call, a temporary that the call owns. */
static inline void
call_release_callable(_PyInterpreterFrame *frame, _PyIRWord word)
{
    if (!(word & IR_F_LOCAL_B)) {
        PyValue *home = &_PyFrame_Slots(frame)[IR_B(word)];
        PyValue owner = *home;
        *home = PyValue_NULL;
        PyValue_XCLOSE(owner);
    }
}

/* The plain return, certified RETURN_VALUE_EMPTY: retire a thread frame whose
 * scratch is empty, so that only the code's locals can own values, as
 * upstream does.  The frame is already unlinked, the caller holds the result
 * in C, and each owner is NULLed and closed in turn: a finalizer that runs
 * sees a consistent frame, and nested calls may grow the arena but must
 * restore this reservation, which stays until the code is closed.  The
 * common frame is the one routine here: the clear, the function and the code,
 * and the pop.  A frame that has a frame object, or that a sampling profiler
 * anchors, takes the general retire out of line: the frame object takes the
 * locals. */
static inline Py_ALWAYS_INLINE PyValue
_PyEval_RetireThreadFrameWithResult(PyThreadState *tstate,
                                    _PyInterpreterFrame *frame, PyValue result)
{
    assert(!PyValue_IsNull(result));
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    assert(tstate->current_frame != frame);
    assert((PyObject **)frame == tstate->datastack_limit);
    if (Py_UNLIKELY(frame->frame_obj != NULL
                    || tstate->last_profiled_frame == frame)) {
        return _PyEval_FrameClearAndPopGeneralWithResult(tstate, frame, result);
    }
    _PyFrame_ClearSlots(frame, (int)_PyFrame_Exec(frame)->nfixed);
    // The frame holds both, and is dead: nothing reads the pointers again
    assert(frame->f_funcobj != NULL);
    Py_DECREF(frame->f_funcobj);
    Py_DECREF(frame->f_executable);
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}

/* The function of the call at head as an owner for its callee: a temporary
 * moves out of its home and a local is counted.  expanded: callable is a
 * bound method, whose function is counted; the method stays in its home. */
static inline Py_ALWAYS_INLINE PyValue
call_exact_function(PyValue *slots, _PyIRWord word, PyValue callable,
                    int expanded)
{
    if (expanded) {
        PyMethodObject *method =
            (PyMethodObject *)PyValue_AsKnownObjectBorrow(callable);
        return PyValue_DUP(PyValue_FromObjectBorrow(method->im_func));
    }
    PyValue *home = &slots[IR_B(word)];
    PyValue function = *home;
    if (Py_UNLIKELY(word & IR_F_LOCAL_B)) {
        return PyValue_DUP(function);
    }
    *home = PyValue_NULL;
    return function;
}

/* Move the operands of the call at head, whose callee takes exactly these
 * arguments, to arguments, the first slots of its frame: a temporary moves
 * and a local is counted.  expanded: callable is a bound method, whose
 * receiver comes first, counted, and the method operand is released. */
static inline Py_ALWAYS_INLINE void
call_fill_exact(PyValue *slots, _PyInterpreterFrame *frame,
                const _PyIRWord *head, _PyIRWord word, PyValue callable,
                int call_state, PyValue *arguments)
{
    int expanded = (call_state & CALL_EXPANDED) != 0;
    if (expanded) {
        PyMethodObject *method =
            (PyMethodObject *)PyValue_AsKnownObjectBorrow(callable);
        *arguments++ = PyValue_DUP(method->im_self);
    }
    int n = (int)IR_C(word);
    // The self operand is NULL at run time for a class attribute that is
    // not a method, and for a bound-method operand: the arguments follow
    // its slot
    int skip = (word & IR_F_NO_SELF) == 0 &&
        (expanded || !(call_state & CALL_SELF_PRESENT));
    if (Py_UNLIKELY(n > IR_OPERANDS_PER_WORD)) {
        call_take_loop(slots, head, skip, n - skip, arguments);
    }
    else {
        // All the operands are in the first operand word; a skipped self
        // slot is its first field
        _PyIRWord ops = head[1];
        uint64_t fields = ops >> 16;
        unsigned int locals = (unsigned int)(ops >> 9) & 15;
        if (Py_UNLIKELY(skip)) {
            fields >>= 12;
            locals >>= 1;
            n--;
        }
        call_take_fields(slots, fields, locals, n, arguments);
    }
    if (expanded) {
        call_release_callable(frame, word);
    }
}

/* Push the frame of the Python function callable for the call at head, whose
 * operands fill its first arguments; with CALL_EXPANDED, of the function of
 * the bound method callable, with its receiver first.  The guards checked the
 * stack space and that the callee takes exactly these arguments
 * (call_fill_exact()). */
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
call_push_exact(PyThreadState *tstate, _PyInterpreterFrame *frame,
                const _PyIRWord *head, _PyIRWord word, PyValue callable,
                PyValue self, int call_state)
{
    int expanded = (call_state & CALL_EXPANDED) != 0;
    // The self operand moves from its home with the arguments; a bound
    // method's is NULL
    assert(!expanded || PyValue_IsNull(self));
    (void)self;
    PyValue *slots = _PyFrame_Slots(frame);
    PyValue function = call_exact_function(slots, word, callable, expanded);
    _PyInterpreterFrame *pushed = _PyFrame_PushUnchecked(tstate, function, frame);
    call_fill_exact(slots, frame, head, word, callable, call_state, pushed->base);
    return pushed;
}

/* The C rows of the Call and Return families (Python/register_ops/calls.rop).
 * An Args operand is the instruction's head, whose word and operand words
 * give the count, the self role and the homes; a Src operand is its home. */

static inline bool
_PyRop_DefaultEvalFrame(PyThreadState *tstate)
{
    return !IS_PEP523_HOOKED(tstate);
}

static inline bool
_PyRop_FunctionVersion(PyValueBorrow fn, const _PyFbCall *fb)
{
    if (!PyValue_IsObject(fn)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(fn);
    return PyFunction_Check(object) &&
        ((PyFunctionObject *)object)->func_version == fb->func_version;
}

/* The call_state of the call at head: whether its self is present at run
 * time (IR_CALL_SELF_STATE) */
static inline int
rop_call_self_state(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    if (head[0] & IR_F_NO_SELF) {
        return 0;
    }
    PyValue self = _PyFrame_Slots(frame)[_PyIR_CallOperand(head, 0)];
    return PyValue_IsNull(self) ? 0 : CALL_SELF_PRESENT;
}

/* The number of arguments the callee receives: the operands, less a self
 * that is NULL at run time */
static inline int
rop_call_argcount(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    _PyIRWord word = head[0];
    int n = (int)IR_C(word);
    if (!(word & IR_F_NO_SELF) && !rop_call_self_state(frame, head)) {
        n--;
    }
    return n;
}

static inline bool
_PyRop_ExactPositional(_PyInterpreterFrame *frame, PyValueBorrow fn,
                       const _PyIRWord *head)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(fn);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    return code->co_argcount == rop_call_argcount(frame, head);
}

static inline bool
_PyRop_FrameSpace(PyThreadState *tstate, PyValueBorrow fn)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(fn);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    return _PyThreadState_HasStackSpace(tstate, code->co_framesize);
}

static inline bool
_PyRop_RecursionBudget(PyThreadState *tstate)
{
    return tstate->py_recursion_remaining > 1;
}

static inline _PyInterpreterFrame *
_PyRop_PushExact(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 PyValue *fn, const _PyIRWord *head)
{
    _PyIRWord word = head[0];
    assert(fn == &_PyFrame_Slots(frame)[IR_B(word)]);
    return call_push_exact(tstate, frame, head, word, *fn, PyValue_NULL,
                           rop_call_self_state(frame, head));
}

static inline bool
_PyRop_SelfAbsent(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    return rop_call_self_state(frame, head) == 0;
}

static inline bool
_PyRop_MethodVersion(PyValueBorrow fn, const _PyFbCall *fb)
{
    if (!PyValue_IsObject(fn)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(fn);
    if (Py_TYPE(object) != &PyMethod_Type) {
        return false;
    }
    PyObject *func = ((PyMethodObject *)object)->im_func;
    return PyFunction_Check(func) &&
        ((PyFunctionObject *)func)->func_version == fb->func_version;
}

static inline bool
_PyRop_NoWeakrefs(PyValueBorrow fn)
{
    return ((PyMethodObject *)PyValue_AsKnownObjectBorrow(fn))->im_weakreflist == NULL;
}

/* The code of the function of a bound method that method_version admitted */
static inline PyCodeObject *
rop_method_code(PyValueBorrow fn)
{
    PyMethodObject *method = (PyMethodObject *)PyValue_AsKnownObjectBorrow(fn);
    return (PyCodeObject *)((PyFunctionObject *)method->im_func)->func_code;
}

static inline bool
_PyRop_ExactPositionalBound(_PyInterpreterFrame *frame, PyValueBorrow fn,
                            const _PyIRWord *head)
{
    return rop_method_code(fn)->co_argcount == rop_call_argcount(frame, head) + 1;
}

static inline bool
_PyRop_FrameSpaceBound(PyThreadState *tstate, PyValueBorrow fn)
{
    return _PyThreadState_HasStackSpace(tstate, rop_method_code(fn)->co_framesize);
}

static inline _PyInterpreterFrame *
_PyRop_PushBoundExact(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      PyValue *fn, const _PyIRWord *head)
{
    _PyIRWord word = head[0];
    assert(fn == &_PyFrame_Slots(frame)[IR_B(word)]);
    return call_push_exact(tstate, frame, head, word, *fn, PyValue_NULL,
                           CALL_EXPANDED);
}

static inline bool
_PyRop_ClassVersion(PyValueBorrow fn, const _PyFbCall *fb)
{
    if (!PyValue_IsObject(fn) || fb->type_version == 0) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(fn);
    return PyType_Check(object) &&
        FT_ATOMIC_LOAD_UINT32_RELAXED(((PyTypeObject *)object)->tp_version_tag) ==
            fb->type_version;
}

static inline bool
_PyRop_InitFrameReady(PyThreadState *tstate, PyValueBorrow fn)
{
    PyHeapTypeObject *cls = (PyHeapTypeObject *)PyValue_AsKnownObjectBorrow(fn);
    assert(cls->ht_type.tp_new == PyBaseObject_Type.tp_new);
    PyFunctionObject *init =
        (PyFunctionObject *)FT_ATOMIC_LOAD_PTR_ACQUIRE(cls->_spec_cache.init);
    if (init->vectorcall != _PyFunction_Vectorcall) {
        return false;
    }
    PyCodeObject *code = (PyCodeObject *)init->func_code;
    return _PyThreadState_HasStackSpace(
        tstate, code->co_framesize + _Py_InitCleanup.co_framesize);
}

/* The operands of the call at head that are still in their homes after a
 * failure, and the callable, are closed with the exception set aside */
static Py_NO_INLINE void
rop_call_retire_operands(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyObject *exc = PyErr_GetRaisedException();
    _PyIRWord word = head[0];
    call_release_rest(_PyFrame_Slots(frame), head, word, (int)IR_C(word),
                      UINT32_MAX);
    PyErr_SetRaisedException(exc);
}

static Py_NO_INLINE _PyInterpreterFrame *
_PyRop_PushInit(PyThreadState *tstate, _PyInterpreterFrame *frame, PyValue *fn,
                _PyInstr *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    _PyInterpreterFrame *child = _PyEval_CreateInitFrame(
        tstate, frame, head, *fn, call_return_pc(head));
    if (child == NULL) {
        rop_call_retire_operands(frame, head);
        return NULL;
    }
    // _PyEval_CreateInitFrame took the cleanup frame's recursion budget
    assert(child->previous->previous == frame);
    return child;
}

static inline void
_PyRop_RetireFailedArgs(_PyInterpreterFrame *frame, PyValue *fn,
                        const _PyIRWord *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    (void)fn;
    rop_call_retire_operands(frame, head);
}

/* Closes the temporary operands of the call at head, all but the home kept,
 * which the call's result now holds.  Every home is cleared before the first
 * value is closed, so no finalizer finds an operand still in the frame
 * (handlers81 rule 5); the values close in reverse order. */
static Py_NO_INLINE void
_PyRop_SpendArgs(_PyInterpreterFrame *frame, const _PyIRWord *head,
                 const PyValue *kept)
{
    PyValue *slots = _PyFrame_Slots(frame);
    int operands = (int)IR_C(head[0]);
    PyValue owners[operands + 1];
    int taken = 0;
    for (int w = 1, left = operands; left > 0;
         w++, left -= IR_OPERANDS_PER_WORD)
    {
        _PyIRWord ops = head[w];
        uint64_t homes = ops >> 16;
        int count = Py_MIN(left, IR_OPERANDS_PER_WORD);
        for (int k = 0; k < count; k++, homes >>= 12) {
            PyValue *home = &slots[homes & IR_FIELD_MAX];
            if (home != kept && !(ops & (IR_F_LOCAL_A << k))) {
                owners[taken++] = _PyFrame_TakeHome(frame, home);
            }
        }
    }
    while (taken > 0) {
        PyValue_XCLOSE(owners[--taken]);
    }
}

/* The Interrupt operand of a call word: whether the call polls when it
 * returns (INTERRUPTIBLE is 0) */
static inline bool
_PyRop_Polls(uint32_t interrupt)
{
    return interrupt == 0;
}

static inline int
_PyRop_EndPoll(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    return check_periodics_at_end(tstate, frame);
}

/* The rows of AttrWrite, SuperAttr, SuperMethod and SpecialLookup
 * (Python/register_ops/attrs.rop).  Every source is borrowed. */

static Py_NO_INLINE int
_PyRop_SetAttr(PyValueBorrow owner, PyObject *name, PyValueBorrow value)
{
    return (_PyValue_SetAttr)(owner, name, value);
}

/* The deletion protocol: a store of no value */
static Py_NO_INLINE int
_PyRop_DelAttr(PyValueBorrow owner, PyObject *name)
{
    return (_PyValue_SetAttr)(owner, name, PyValue_Borrow(PyValue_NULL));
}

/* super(class, self), or super() when the class was implied; the class and
 * the receiver are counted for the call as its argument vector needs. */
static Py_NO_INLINE PyValue
_PyRop_SuperCall(PyValueBorrow sup, PyValueBorrow cls, PyValueBorrow self,
                 bool explicit)
{
    PyValue stack[] = {PyValue_DUP(cls), PyValue_DUP(self)};
    PyValue super = _PyValue_Vectorcall(sup, stack, explicit ? 2 : 0, NULL);
    PyValue_CLOSE(stack[1]);
    PyValue_CLOSE(stack[0]);
    return super;
}

static void rop_take_items(const _PyRopSpan *span, PyValue *vals);

/* The argument that the CALL event reports besides `super`: the class, or
 * MISSING when the class was implied */
static inline PyValueBorrow
_PyRop_SuperEventArg(PyValueBorrow cls, bool explicit)
{
    return explicit ? cls
                    : PyValue_FromObjectBorrow(&_PyInstrumentation_MISSING);
}

/* The attribute of super(...): the span holds super, the class and the
 * receiver, which the row owns and closes on both outcomes.  With observed
 * set, the CALL event runs before the call and C_RETURN or C_RAISE after it,
 * as INSTRUMENTED_LOAD_SUPER_ATTR did; a callback that raises in C_RETURN
 * closes the super object. */
static inline PyValue
rop_super_lookup(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site, const _PyRopSpan *parts, bool explicit,
                 PyObject *name, bool observed)
{
    PyValue vals[3];
    rop_take_items(parts, vals);
    PyValueBorrow sup = PyValue_Borrow(vals[0]);
    PyValueBorrow cls = PyValue_Borrow(vals[1]);
    PyValueBorrow self = PyValue_Borrow(vals[2]);
    PyValue attr = PyValue_NULL;
    PyValueBorrow arg = _PyRop_SuperEventArg(cls, explicit);
    if (observed &&
        (_Py_call_instrumentation_2args)(tstate, PY_MONITORING_EVENT_CALL,
                                         frame, site, sup, arg) < 0) {
        goto done;
    }
    PyValue super = _PyRop_SuperCall(sup, cls, self, explicit);
    if (PyValue_IsNull(super)) {
        if (observed) {
            (_Py_call_instrumentation_exc2)(
                tstate, PY_MONITORING_EVENT_C_RAISE, frame, site, sup, arg);
        }
        goto done;
    }
    if (observed &&
        (_Py_call_instrumentation_2args)(tstate, PY_MONITORING_EVENT_C_RETURN,
                                         frame, site, sup, arg) < 0) {
        PyValue_CLEAR(super);
        goto done;
    }
    attr = (_PyValue_GetAttr)(PyValue_Borrow(super), name);
    PyValue_CLOSE(super);
done:
    {
        PyObject *exc = PyValue_IsNull(attr) ? PyErr_GetRaisedException()
                                             : NULL;
        for (int i = 0; i < 3; i++) {
            PyValue_XCLOSE(vals[i]);
        }
        if (exc != NULL) {
            PyErr_SetRaisedException(exc);
        }
    }
    return attr;
}

static Py_NO_INLINE PyValue
_PyRop_SuperLookup(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                   const _PyRopSpan *parts, PyObject *name)
{
    return rop_super_lookup(tstate, frame, site, parts, false, name,
                            false);
}

static Py_NO_INLINE PyValue
_PyRop_SuperLookupObserved(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                           const _PyRopSpan *parts, PyObject *name)
{
    return rop_super_lookup(tstate, frame, site, parts, false, name,
                            true);
}

static Py_NO_INLINE PyValue
_PyRop_SuperLookupExplicit(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                           const _PyRopSpan *parts, PyObject *name)
{
    return rop_super_lookup(tstate, frame, site, parts, true, name,
                            false);
}

static Py_NO_INLINE PyValue
_PyRop_SuperLookupExplicitObserved(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                                   const _PyRopSpan *parts, PyObject *name)
{
    return rop_super_lookup(tstate, frame, site, parts, true, name,
                            true);
}

/* The method forms of the lookups above: the attribute is bound, so the call
 * has no receiver of its own */
#define ROP_SUPER_METHOD_WRAPPER(name, from)                                  \
    static Py_NO_INLINE PyValue                                               \
    name(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,   \
         const _PyRopSpan *parts, PyObject *attr_name, PyValue *self)        \
    {                                                                         \
        *self = PyValue_NULL;                                                 \
        return from(tstate, frame, site, parts, attr_name);                   \
    }
ROP_SUPER_METHOD_WRAPPER(_PyRop_SuperMethod, _PyRop_SuperLookup)
ROP_SUPER_METHOD_WRAPPER(_PyRop_SuperMethodObserved, _PyRop_SuperLookupObserved)
ROP_SUPER_METHOD_WRAPPER(_PyRop_SuperMethodExplicit, _PyRop_SuperLookupExplicit)
ROP_SUPER_METHOD_WRAPPER(_PyRop_SuperMethodExplicitObserved,
                         _PyRop_SuperLookupExplicitObserved)
#undef ROP_SUPER_METHOD_WRAPPER

/* The part i of a super span, borrowed from its home */
static inline PyValueBorrow
rop_super_part(const _PyRopSpan *parts, int i)
{
    int slot = _PyIR_SpanElement(parts->head, parts->handler, parts->first + i);
    return PyValue_Borrow(parts->frame->base[slot]);
}

/* `super` is the builtin and the class is a type: the lookup needs no super
 * object */
static inline bool
_PyRop_SuperBuiltinReady(const _PyRopSpan *parts)
{
    PyValueBorrow sup = rop_super_part(parts, 0);
    PyValueBorrow cls = rop_super_part(parts, 1);
    return PyValue_IsObject(sup) && PyValue_IsObject(cls)
        && PyValue_AsKnownObjectBorrow(sup) == (PyObject *)&PySuper_Type
        && PyType_Check(PyValue_AsKnownObjectBorrow(cls));
}

/* The case of the adaptation row: 0 when the site can use the builtin
 * lookup, 1 when it cannot */
static inline int
_PyRop_ClassifySuper(const _PyRopSpan *parts, _PyInstr *site,
                     const _PyFbUnary *fb)
{
    (void)site;
    (void)fb;
    return _PyRop_SuperBuiltinReady(parts) ? 0 : 1;
}

/* super(...).name without the super object, as LOAD_SUPER_ATTR_ATTR did: the
 * parts are the row's to close on both outcomes */
static Py_NO_INLINE PyValue
_PyRop_SuperLookupBuiltin(const _PyRopSpan *parts, PyObject *name)
{
    PyValue vals[3];
    rop_take_items(parts, vals);
    _PyStatusValue found = _PySuper_LookupValue(
        (PyTypeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(vals[1])),
        PyValue_Borrow(vals[2]), name, 0);
    PyObject *exc = found.status < 0 ? PyErr_GetRaisedException() : NULL;
    for (int i = 0; i < 3; i++) {
        PyValue_XCLOSE(vals[i]);
    }
    if (exc != NULL) {
        PyErr_SetRaisedException(exc);
        return PyValue_NULL;
    }
    return found.value;
}

/* LOAD_SUPER_ATTR_METHOD: an unbound method comes with the receiver as the
 * call's self, which moves from the parts to *self; a bound result closes
 * it.  Only a receiver whose type has the generic getattro can have a method
 * returned unbound.  The parts are the row's to close on both outcomes. */
static Py_NO_INLINE PyValue
_PyRop_SuperLookupBuiltinMethod(const _PyRopSpan *parts, PyObject *name,
                                PyValue *self)
{
    PyValue vals[3];
    rop_take_items(parts, vals);
    *self = PyValue_NULL;
    _PyStatusValue found = _PySuper_LookupValue(
        (PyTypeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(vals[1])),
        PyValue_Borrow(vals[2]), name,
        PyValue_TYPE(PyValue_Borrow(vals[2]))->tp_getattro
            == PyObject_GenericGetAttr);
    PyObject *exc = found.status < 0 ? PyErr_GetRaisedException() : NULL;
    int keep_self = found.status > 0;
    for (int i = 0; i < 3; i++) {
        if (i == 2 && keep_self) {
            *self = vals[2];
            continue;
        }
        PyValue_XCLOSE(vals[i]);
    }
    if (exc != NULL) {
        PyErr_SetRaisedException(exc);
        return PyValue_NULL;
    }
    return found.value;
}

/* The method of a special name on the receiver's type.  The lookup works on
 * a counted copy of the receiver, which it keeps (*self) when the method is
 * unbound and closes when the method is bound.  A missing method raises the
 * protocol's TypeError, naming the asynchronous protocol when the type has
 * it. */
static Py_NO_INLINE PyValue
_PyRop_LookupSpecial(PyValueBorrow obj, int which, PyValue *self)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue receiver = PyValue_DUP(obj);
    PyValue method = PyValue_NULL;
    int rc = _PyObject_LookupSpecialMethod(NULL, _Py_SpecialMethods[which].name,
                                           &method, &receiver);
    *self = PyValue_NULL;
    if (rc <= 0) {
        if (rc == 0) {
            PyTypeObject *owner_type = PyValue_TYPE(receiver);
            const char *errfmt = _PyEval_SpecialMethodCanSuggest(owner_type, which)
                ? _Py_SpecialMethods[which].error_suggestion
                : _Py_SpecialMethods[which].error;
            assert(!_PyErr_Occurred(tstate));
            assert(errfmt != NULL);
            _PyErr_Format(tstate, PyExc_TypeError, errfmt, owner_type);
        }
        PyValue_XCLOSE(receiver);
        return PyValue_NULL;
    }
    *self = receiver;
    return method;
}

static Py_NO_INLINE void
_PyRop_InitResultError(PyValueBorrow value)
{
    PyErr_Format(PyExc_TypeError,
                 "__init__() should return None, not '%.200s'",
                 PyValue_TYPE(value)->tp_name);
}

static inline bool _PyRop_PythonTarget(PyThreadState *tstate,
                                       _PyInterpreterFrame *frame,
                                       PyValue *fn, const _PyIRWord *head);
static _PyInterpreterFrame *_PyRop_PushPython(PyThreadState *tstate,
                                              _PyInterpreterFrame *frame,
                                              PyValue *fn, _PyInstr *head);
/* The rows of Function and Intrinsic (Python/register_ops/functions.rop) */
static PyValue _PyRop_FunctionMake(_PyInterpreterFrame *frame, PyValue code,
                                   const _PyRopSpan *fields, uint32_t present);
static PyValue _PyRop_IntrinsicUnary(PyThreadState *tstate, uint32_t id,
                                     PyValue x);
static PyValue _PyRop_IntrinsicImportStar(PyThreadState *tstate, PyValue x);
static PyValue _PyRop_IntrinsicStopIteration(PyThreadState *tstate, PyValue x);
static PyValue _PyRop_IntrinsicBinary(PyThreadState *tstate, uint32_t id,
                                      PyValue x, PyValue y);
static PyValue _PyRop_IntrinsicAttr(PyThreadState *tstate, uint32_t id,
                                    PyValue x, PyValue y);

static inline PyValue _PyRop_CallNative(PyThreadState *tstate,
                                        _PyInterpreterFrame *frame, PyValue *fn,
                                        const _PyIRWord *head);
static int _PyRop_CallEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            _PyInstr *site, PyValue *fn,
                            const _PyIRWord *head);
static PyValue _PyRop_CallNativeObserved(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame,
                                         _PyInstr *site, PyValue *fn,
                                         const _PyIRWord *head);

static int _PyRop_EdgeEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            _PyInstr *site, bool taken, int event);

/* The row of BranchTruth.Observed and BranchNone.Observed: the event of the
 * edge the word took, as ir_event_edge() fires it. */
static Py_NO_INLINE int
_PyRop_BranchEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, bool taken)
{
    return _PyRop_EdgeEvent(tstate, frame, site, taken,
                            taken ? PY_MONITORING_EVENT_BRANCH_RIGHT
                                  : PY_MONITORING_EVENT_BRANCH_LEFT);
}

/* The row of Jump.Observed: the JUMP event of the word's one edge */
static Py_NO_INLINE int
_PyRop_JumpEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site)
{
    return _PyRop_EdgeEvent(tstate, frame, site, true, PY_MONITORING_EVENT_JUMP);
}

/* The backedge's poll (Jump.Poll, Jump.Observed): signals, the GC and other
 * pending events, as _CHECK_PERIODIC */
static inline int
_PyRop_Poll(PyThreadState *tstate)
{
    return check_periodics(tstate);
}

/* Whether the jump word at site goes back */
static inline bool
_PyRop_GoesBack(_PyInstr *site, _PyInstr *target)
{
    return target <= site;
}

/* The event of the edge of the branch or jump word at site that the word
 * has committed to.  A callback that moves the frame leaves the word to run
 * in frame->return_pc, where the redirect exit goes on. */
static int
_PyRop_EdgeEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site, bool taken, int event)
{
    // An instruction event may have made the word transparent: its target is
    // in the format of the handler it wraps
    const _PyIRFormat *fmt = _PyIR_FormatOf(_PyIR_RealHandler(frame, site));
    // The wrapper's own length is one word; the fall-through continues past
    // the fixed operand words of the word it wraps
    _PyInstr *dest = taken ? site + 1 + _PyIR_FormatDistance(fmt, *site)
                           : site + 1 + fmt->fixed_words;
    frame->return_pc = dest;
    _PyIRObserverResult result = _PyIR_ObserveCommittedBranch(
        tstate, frame, site, site, dest, event,
        taken ? _PyIR_EDGE_TAKEN : _PyIR_EDGE_FALLTHROUGH);
    PyObject *exception = result.status < 0 ? PyErr_GetRaisedException() : NULL;
    frame->instr_ptr = result.next;
    if (result.status < 0) {
        PyErr_SetRaisedException(exception);
        return -1;
    }
    if (result.status > 0) {
        frame->return_pc = result.next;
    }
    return result.status;
}

/* The rows of Return.Observed */

static Py_NO_INLINE PyValue
_PyRop_ReturnEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, PyValue value)
{
    if (_Py_call_instrumentation_arg(tstate, PY_MONITORING_EVENT_PY_RETURN,
                                     frame, site, value)) {
        PyValue_CLOSE(value);
        return PyValue_NULL;
    }
    return value;
}

static inline bool
_PyRop_GeneratorFrame(_PyInterpreterFrame *frame)
{
    return frame->owner == FRAME_OWNED_BY_GENERATOR;
}

static inline _PyInterpreterFrame *
_PyRop_PreviousFrame(_PyInterpreterFrame *frame)
{
    return frame->previous;
}

static inline _PyInterpreterFrame *
_PyRop_Depart(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    assert(tstate->current_frame == frame);
    DTRACE_FUNCTION_RETURN();
    _Py_LeaveRecursiveCallPy(tstate);
    // GH-99729: the frame is unlinked before it is cleared
    tstate->current_frame = frame->previous;
    return frame;
}

static inline void _PyRop_RetireFrame(PyThreadState *tstate,
                                      _PyInterpreterFrame *dying);
static inline void _PyRop_RetireFrameEmpty(PyThreadState *tstate,
                                           _PyInterpreterFrame *dying);

static inline _PyInstr *
_PyRop_Deliver(_PyInterpreterFrame *caller, PyValue v)
{
    assert(_PyValue_IsHome(v));
    (void)_PyFrame_SetReturnValue(caller, v);
    return caller->return_pc;
}

static inline void
_PyRop_LeaveActivation(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
    assert(_PyFrame_IsIncomplete(frame));
    assert(!_PyErr_Occurred(tstate));
    tstate->current_frame = frame->previous;
}

/* The prologue of the code of a generator, coroutine or async generator that
 * _PyGen_NewCall() made and whose arguments are bound, which the first send
 * does not run: a cell for each cell variable (an argument moves into its
 * cell) and, for a generator expression, the iterator of its argument.  The
 * free variables are already in the frame.  0, or -1 with an exception set;
 * the caller then discards the object (_PyGen_DiscardCall), which still owns
 * the arguments. */
static Py_NO_INLINE int
gen_prologue_run(PyThreadState *tstate, PyGenObject *gen, PyCodeObject *code)
{
    _PyInterpreterFrame *frame = &gen->gi_iframe;
    int ncell = code->co_ncellvars;
    const char *kinds = PyBytes_AS_STRING(code->co_localspluskinds);
    for (int i = 0; ncell > 0; i++) {
        if (!(kinds[i] & CO_FAST_CELL)) {
            continue;
        }
        ncell--;
        // The slot holds the argument, or nothing for a cell variable
        PyObject *cell = PyCell_NewValue(frame->base[i]);
        if (cell == NULL) {
            return -1;
        }
        PyValue argument = frame->base[i];
        frame->base[i] = PyValue_FromObjectSteal(cell);
        PyValue_XCLOSE(argument);
    }
    _PyCodeExec *exec = code->_co_exec;
    if (exec->first_traceable > 1) {
        /* A generator expression: its code begins with RESUME, a bound
         * LocalRead of the argument and GET_ITER or GET_AITER, before its
         * RETURN_GENERATOR, and nothing executes them.  The iterator, and for GET_ITER the
         * index, are the first two slots of the stack, where the loop reads
         * them: iter() of the outermost iterable runs here, at creation, as
         * the language requires, and the code takes any iterable as its
         * argument.  Its errors are those of the call. */
        int handler = _PyIR_BaseOpcode(IR_HANDLER(exec->words[2]));
        assert(IR_HANDLER(exec->words[1]) == ROP_HANDLER_LocalRead_Bound);
        assert(handler == GET_ITER || handler == GET_AITER);
        assert(IR_A(exec->words[2]) == exec->nfixed);
        PyValue *stack = frame->base + exec->nfixed;
        PyValue iterable = PyValue_DUP(frame->base[0]);
        if (handler == GET_AITER) {
            stack[0] = _PyEval_GetAIter(tstate, iterable);
        }
        else {
            _PyValuePair got = _PyEval_GetIter(iterable, 0);
            stack[0] = got.first;
            stack[1] = got.second;
        }
        if (PyValue_IsNull(stack[0])) {
            return -1;
        }
    }
    return 0;
}

/* gen_prologue_run() for the code that has one: most generator functions have
 * no cell variables and are not generator expressions (whose first traceable
 * word follows the prelude). */
static inline Py_ALWAYS_INLINE int
gen_prologue(PyThreadState *tstate, PyGenObject *gen, PyCodeObject *code)
{
    if (code->co_ncellvars == 0 && code->_co_exec->first_traceable == 1) {
        return 0;
    }
    return gen_prologue_run(tstate, gen, code);
}

/* The generator that the call at head creates, when the callee is a generator
 * function that takes exactly these arguments (CALL_PY_GENERATOR): the
 * operands fill its frame, with no frame of the thread.  The method operand
 * of an expanded call is not a form of it.  The null value is an error. */
static inline Py_ALWAYS_INLINE PyValue
call_new_generator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   const _PyIRWord *head, _PyIRWord word, PyValue callable,
                   int call_state)
{
    assert(!(call_state & CALL_EXPANDED));
    PyValue *slots = _PyFrame_Slots(frame);
    PyFunctionObject *func =
        (PyFunctionObject *)_PyValue_AsKnownObjectBorrowPointer(callable);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    PyValue function = call_exact_function(slots, word, callable, 0);
    PyGenObject *gen = _PyGen_NewCall(tstate, function, NULL, code, frame);
    if (gen == NULL) {
        return PyValue_NULL;
    }
    call_fill_exact(slots, frame, head, word, PyValue_NULL, call_state,
                    gen->gi_iframe.base);
    if (gen_prologue(tstate, gen, code) < 0) {
        _PyGen_DiscardCall(gen);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal((PyObject *)gen);
}

static inline Py_ALWAYS_INLINE int
call_refresh(PyThreadState *tstate, _PyInterpreterFrame *frame,
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

/* The word of an immortal or caller-pinned object for a monitoring argument:
 * uncounted, never closed. */
static inline PyValue
borrowed_word(PyObject *obj)
{
    PyValue word;
    _PyValue_WindowFromObjects(&word, &obj, 1);
    return word;
}

/* The handlers' calls (call_run_*), after the binder
 * (eval_frame_push_and_init_code), which they use. */
static _PyCallResult call_run_keywords(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *, int);
static _PyCallResult call_run_ex(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *, int);
static _PyCallResult call_bind_positional(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValue, PyValue,
    const _PyBindPlan *);

typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    PyCodeObject *code;
    Py_ssize_t current_word;
    Py_ssize_t next_word;
} _PyNativeCallPublication;

static _PyNativeCallPublication
native_call_publication(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *next)
{
    _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
    assert(exec != NULL && frame->ir_words == exec->words);
    return (_PyNativeCallPublication){
        tstate, frame, _PyFrame_GetCode(frame),
        frame->instr_ptr - exec->words, next - exec->words,
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
    frame->instr_ptr = exec->words + state->current_word;
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
    assert(next == exec->words + state->next_word);
    (void)next;
    frame->instr_ptr = exec->words + state->current_word;
    return 0;
}

/* CALL_BUILTIN_O's native call, out of line (spec 3.5, R-c): the scope, its
 * result and the publication record are addressed by the runtime API and by
 * the publish callbacks, so they live in this function's own frame, not the
 * handler's.  arg is the home of the call's argument, which stays where it is
 * and is read in place.  Returns the result, which the handler publishes to
 * the call's dst, or PyValue_NULL with the error set. */
static Py_NO_INLINE PyValue
native_call_builtin_o(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *next, PyValue callable, PyValue *arg)
{
    PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
    _PyNativeScope scope;
    PyValue pending = PyValue_NULL;
    _PyNativeCallPublication publication = native_call_publication(
        tstate, frame, next);
    int status = _PyNativeScope_Begin(
        &scope, tstate, arg, 1, 1, PyValue_NULL, NULL);
    if (status == 0) {
        status = _PyCFunction_CallScoped(&scope, callable_o, &scope.result);
    }
    if (status < 0) {
        _PyNativeResult_Abort(&scope.result, native_call_publish_pc, &publication);
        (void)native_call_finish(&publication, &scope, NULL, NULL, next);
        return PyValue_NULL;
    }
    _PyNativeResult_Publish(&scope.result, &pending,
                            native_call_publish_pc, &publication);
    if (native_call_finish(&publication, &scope, NULL, NULL, next) < 0) {
        PyValue_XCLOSE(pending);
        return PyValue_NULL;
    }
    return pending;
}

static void clear_error_frame(PyThreadState *, _PyInterpreterFrame *);

/* Release the slots from first up, from the top, each taken (NULL) before it
 * is closed, so that a finalizer finds the slots not yet reached intact and
 * the rest NULL. */
static void
error_clear_slots(_PyInterpreterFrame *frame, int first)
{
    assert(first >= 0 && first <= _PyFrame_NumSlots(frame));
    for (int home = _PyFrame_NumSlots(frame); home-- > first;) {
        if (PyValue_IsNull(frame->base[home])) {
            continue;
        }
        PyValue owner = _PyFrame_TakeHome(frame, &frame->base[home]);
        PyValue_XCLOSE(owner);
    }
}

/* Whether a pair other than a done one has source s */
static int
parallel_pending_source(int n, const int32_t *src, int stride,
                        const uint64_t *done, int s)
{
    for (int q = 0; q < n; q++) {
        if (!((done[q >> 6] >> (q & 63)) & 1) && src[q * stride] == s) {
            return 1;
        }
    }
    return 0;
}

/* The parallel move of n pairs of homes, src[q * stride] to
 * dst[q * stride], as if all moved at once: the Move instruction's and the
 * unwinder's ingress recipe's (pyjit/design/proposals/rir90.md).  A home
 * that no pending pair still reads takes its value by a swap, which leaves
 * what the home held at the source; what is left forms cycles, each turned
 * through one value held in a C local, the rooted temporary.  Nothing is
 * closed: a source that is no destination ends with what its destination
 * held, which the caller closes, if anything. */
void
_PyFrame_ParallelMove(_PyInterpreterFrame *frame, int n, const int32_t *src,
                      const int32_t *dst, int stride)
{
    assert(n >= 0 && n <= IR_MAX_SLOTS);
    uint64_t done[IR_MAX_SLOTS / 64 + 1] = {0};
#define MOVE_DONE(Q) (done[(Q) >> 6] |= (uint64_t)1 << ((Q) & 63))
#define MOVE_IS_DONE(Q) ((done[(Q) >> 6] >> ((Q) & 63)) & 1)
    int progress = 1;
    while (progress) {
        progress = 0;
        for (int q = 0; q < n; q++) {
            int s = src[q * stride], h = dst[q * stride];
            if (MOVE_IS_DONE(q) ||
                parallel_pending_source(n, src, stride, done, h)) {
                continue;
            }
            PyValue value = _PyFrame_TakeHome(frame, &frame->base[s]);
            PyValue held = _PyFrame_TakeHome(frame, &frame->base[h]);
            _PyFrame_WriteHome(frame, &frame->base[h], value);
            _PyFrame_WriteHome(frame, &frame->base[s], held);
            MOVE_DONE(q);
            progress = 1;
        }
    }
    for (int q = 0; q < n; q++) {
        if (MOVE_IS_DONE(q)) {
            continue;
        }
        // Every home of a cycle is a source of it
        int s = src[q * stride];
        PyValue first = _PyFrame_TakeHome(frame, &frame->base[s]);
        int vacant = s;
        for (;;) {
            int r = -1;
            for (int p = 0; p < n && r < 0; p++) {
                if (!MOVE_IS_DONE(p) && dst[p * stride] == vacant) {
                    r = p;
                }
            }
            assert(r >= 0);
            MOVE_DONE(r);
            if (r == q) {
                _PyFrame_WriteHome(frame, &frame->base[vacant], first);
                break;
            }
            int from = src[r * stride];
            _PyFrame_WriteHome(frame, &frame->base[vacant],
                               _PyFrame_TakeHome(frame, &frame->base[from]));
            vacant = from;
        }
    }
#undef MOVE_DONE
#undef MOVE_IS_DONE
}

/* Apply an ingress recipe, recipe[0] pairs of source and home: put each
 * source in its home as if all moved at once, then release what the homes
 * held, which the sources that are no home are left with. */
void
_PyFrame_ApplyRecipe(_PyInterpreterFrame *frame, const int32_t *recipe)
{
    int n = recipe[0];
    assert(n > 0 && n <= IR_MAX_SLOTS);
    _PyFrame_ParallelMove(frame, n, &recipe[1], &recipe[2], 2);
    for (int q = 0; q < n; q++) {
        int s = recipe[1 + 2 * q];
        int home = 0;
        for (int p = 0; p < n; p++) {
            home |= recipe[2 + 2 * p] == s;
        }
        if (!home) {
            PyValue_XCLOSE(_PyFrame_TakeHome(frame, &frame->base[s]));
        }
    }
}

static void
error_apply_recipe(_PyInterpreterFrame *frame, int index)
{
    _PyFrame_ApplyRecipe(frame, _PyIR_Recipe(
        _PyCode_IR(_PyFrame_GetCode(frame)), index));
}

/* The rows of the checked local families (Python/register_ops/values.rop) */
void
_PyRop_UnboundLocal(_PyInterpreterFrame *frame, PyValue *home)
{
    int index = (int)(home - frame->base);
    _PyEval_FormatExcCheckArg(
        _PyThreadState_GET(), PyExc_UnboundLocalError, UNBOUNDLOCAL_ERROR_MSG,
        _PyTuple_GET_OBJECT(_PyFrame_GetCode(frame)->co_localsplusnames, index));
}

PyValue
_PyRop_DeleteLocal(_PyInterpreterFrame *frame, PyValue *home)
{
    if (PyValue_IsNull(*home)) {
        _PyRop_UnboundLocal(frame, home);
        return PyValue_NULL;
    }
    return _PyFrame_TakeHome(frame, home);
}

/* The rows of the cell families (Python/register_ops/cells.rop) */
void
_PyRop_UnboundCell(_PyInterpreterFrame *frame, PyValue *home)
{
    _PyEval_FormatExcUnbound(_PyThreadState_GET(), _PyFrame_GetCode(frame),
                             (int)(home - frame->base));
}

PyValue
_PyRop_MakeCell(_PyInterpreterFrame *frame, PyValue *home)
{
    (void)frame;
    // The cell holds its own count, so closing the binding cannot finalize it
    PyValue binding = *home;
    *home = PyValue_NULL;
    PyObject *cell = PyCell_NewValue(binding);
    PyValue_XCLOSE(binding);
    if (cell == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(cell);
}

_PyStatusValue
_PyRop_LoadFromDictCell(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        PyValueBorrow mapping, PyValue *home)
{
    assert(PyValue_IsObject(mapping));
    int index = (int)(home - frame->base);
    PyObject *name = _PyTuple_GET_OBJECT(
        _PyFrame_GetCode(frame)->co_localsplusnames, index);
    return value_load_from_dict(tstate, PyValue_AsKnownObjectBorrow(mapping), name);
}

/* The rows of the namespace families (Python/register_ops/namespaces.rop).
 * Each is the body of the stack op's old handler.  A row that takes an
 * owner closes it on every path. */

PyValue
_PyRop_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                uint32_t index)
{
    return _PyEval_LoadName(tstate, frame, _PyRop_NameAt(frame, index));
}

PyValue
_PyRop_LoadLocals(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    PyObject *l = frame->f_locals;
    if (l == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "no locals found");
        return PyValue_NULL;
    }
    return PyValue_DUP(PyValue_FromObjectBorrow(l));
}

PyValue
_PyRop_LoadBuildClass(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyStatusValue found = _PyValue_GetOptionalItem(
        PyValue_FromObjectBorrow(frame->f_builtins),
        PyValue_FromObjectBorrow(&_Py_ID(__build_class__)));
    if (found.status < 0) {
        return PyValue_NULL;
    }
    if (found.status == 0) {
        _PyErr_SetString(tstate, PyExc_NameError,
                         "__build_class__ not found");
        return PyValue_NULL;
    }
    return found.value;
}

_PyStatusValue
_PyRop_LoadFromDict(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValueBorrow mapping, uint32_t index)
{
    assert(PyValue_IsObject(mapping));
    return value_load_from_dict(tstate, PyValue_AsKnownObjectBorrow(mapping),
                                _PyRop_NameAt(frame, index));
}

PyValue
_PyRop_LoadGlobalOrBuiltin(_PyInterpreterFrame *frame, uint32_t index)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    return value_load_global(frame->f_globals, frame->f_builtins, name);
}

/* A NULL value deletes: _PyEval_StoreName's convention */
int
_PyRop_StoreName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 uint32_t index, PyValue v)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    int error = _PyEval_StoreName(tstate, PyValue_Borrow(v), name,
                                  frame->f_locals);
    PyValue_XCLOSE(v);
    return error ? -1 : 0;
}

int
_PyRop_DeleteName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  uint32_t index)
{
    return _PyRop_StoreName(tstate, frame, index, PyValue_NULL);
}

int
_PyRop_StoreGlobal(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   uint32_t index, PyValue v)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    int err = PyDict_SetValue(frame->f_globals, PyValue_FromObjectBorrow(name),
                              PyValue_Borrow(v));
    PyValue_XCLOSE(v);
    return err < 0 ? -1 : 0;
}

int
_PyRop_DeleteGlobal(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    uint32_t index)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    int err = PyDict_PopValue(frame->f_globals, PyValue_FromObjectBorrow(name),
                              NULL);
    if (err == 0) {
        _PyEval_FormatExcCheckArg(tstate, PyExc_NameError, NAME_ERROR_MSG,
                                  name);
        return -1;
    }
    return err < 0 ? -1 : 0;
}

int
_PyRop_SetupAnnotations(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    PyObject *locals = frame->f_locals;
    if (locals == NULL) {
        _PyErr_Format(tstate, PyExc_SystemError,
                      "no locals found when setting up annotations");
        return -1;
    }
    /* check if __annotations__ in locals()... */
    _PyStatusValue found = _PyValue_GetOptionalItem(
        PyValue_FromObjectBorrow(locals),
        PyValue_FromObjectBorrow(&_Py_ID(__annotations__)));
    if (found.status < 0) {
        return -1;
    }
    if (found.status > 0) {
        PyValue_CLOSE(found.value);
        return 0;
    }
    PyObject *ann_dict = PyDict_New();
    if (ann_dict == NULL) {
        return -1;
    }
    int err = _PyValue_SetItem(PyValue_FromObjectBorrow(locals),
                               PyValue_FromObjectBorrow(&_Py_ID(__annotations__)),
                               PyValue_FromObjectBorrow(ann_dict));
    Py_DECREF(ann_dict);
    return err ? -1 : 0;
}

PyValue
_PyRop_ImportName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyObject *name, PyValue level, PyValue fromlist,
                  uint32_t kind)
{
    // The compiler loads fromlist from a constant: None or a tuple of str
    PyObject *fromlist_o = PyValue_AsKnownObjectBorrow(fromlist);
    // The level is an inline int: read it without a box
    int ilevel = value_import_level(level);
    PyObject *res_o = NULL;
    if (ilevel != -1 || !_PyErr_Occurred(tstate)) {
        if (kind != IMPORT_EAGER) {
            res_o = _PyEval_LazyImportName(tstate, frame->f_builtins,
                                           frame->f_globals, frame->f_locals,
                                           name, fromlist_o, ilevel,
                                           kind == IMPORT_LAZY);
        }
        else {
            res_o = _PyEval_ImportName(tstate, frame->f_builtins,
                                       frame->f_globals, frame->f_locals,
                                       name, fromlist_o, ilevel);
        }
    }
    PyValue_XCLOSE(level);
    PyValue_XCLOSE(fromlist);
    return res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
}

PyValue
_PyRop_ImportFrom(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue module, uint32_t index)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    // The module may be a number, if __import__ returned one
    PyObject *from_o = PyValue_ObjectOrNull(PyValue_Borrow(module));
    PyValue res;
    if (from_o != NULL && PyLazyImport_CheckExact(from_o)) {
        PyObject *res_o = _PyEval_LazyImportFrom(tstate, frame, from_o, name);
        res = res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
    }
    else {
        res = _PyEval_ImportFromValue(tstate, PyValue_Borrow(module), name);
    }
    PyValue_XCLOSE(module);
    return res;
}

/* The rows of the Move arms (Python/register_ops/values.rop).  The homes of
 * a span are decoded once; a set has at most IR_FIELD_MAX pairs. */
static int
span_slots(const _PyRopSpan *span, int32_t *slots)
{
    for (int i = 0; i < span->count; i++) {
        slots[i] = _PyIR_SpanElement(span->head, span->handler, span->first + i);
    }
    return span->count;
}

void
_PyRop_ParallelMove(const _PyRopSpan *d, const _PyRopSpan *s)
{
    int32_t dst[IR_FIELD_MAX + 1], src[IR_FIELD_MAX + 1];
    int n = span_slots(d, dst);
    assert(s->count >= n);
    for (int i = 0; i < n; i++) {
        src[i] = _PyIR_SpanElement(s->head, s->handler, s->first + i);
    }
    _PyFrame_ParallelMove(d->frame, n, src, dst, 1);
}

void
_PyRop_CloseRest(const _PyRopSpan *d, const _PyRopSpan *s)
{
    // Every home is written before any close runs a finalizer
    PyValue taken[IR_FIELD_MAX + 1];
    int n = 0;
    for (int i = d->count; i < s->count; i++) {
        int slot = _PyIR_SpanElement(s->head, s->handler, s->first + i);
        taken[n++] = _PyFrame_TakeHome(s->frame, &s->frame->base[slot]);
    }
    for (int i = 0; i < n; i++) {
        PyValue_XCLOSE(taken[i]);
    }
}

void
_PyRop_CopyHomes(const _PyRopSpan *d, const _PyRopSpan *s)
{
    assert(d->count == s->count);
    for (int i = 0; i < d->count; i++) {
        int from = _PyIR_SpanElement(s->head, s->handler, s->first + i);
        int to = _PyIR_SpanElement(d->head, d->handler, d->first + i);
        PyValue old = _PyFrame_TakeHome(d->frame, &d->frame->base[to]);
        assert(PyValue_IsNull(old));
        (void)old;
        _PyFrame_WriteHome(d->frame, &d->frame->base[to],
                           PyValue_DUP(PyValue_Borrow(d->frame->base[from])));
    }
}

/* The rows of Build, AddElement, AddEntry, UpdateContainer and
 * MergeKeywords (Python/register_ops/displays.rop). */

/* The owners of the items of a span, in order: a temporary's owner moves out
 * of its home, and a local's binding gets an owner of its own.  Every home
 * of a temporary is vacant on return. */
static void
rop_take_items(const _PyRopSpan *span, PyValue *vals)
{
    _PyInterpreterFrame *frame = span->frame;
    int nlocals = _PyCode_IRFixed(_PyFrame_GetCode(frame));
    for (int i = 0; i < span->count; i++) {
        int slot = _PyIR_SpanElement(span->head, span->handler, span->first + i);
        PyValue *home = &frame->base[slot];
        vals[i] = slot < nlocals ? PyValue_DUP(PyValue_Borrow(*home))
                                 : _PyFrame_TakeHome(frame, home);
    }
}

static void
rop_close_items(PyValue *vals, int n)
{
    for (int i = 0; i < n; i++) {
        PyValue_XCLOSE(vals[i]);
    }
}

PyValue
_PyRop_BuildTuple(const _PyRopSpan *items)
{
    int n = items->count;
    PyValue vals[n + 1];
    rop_take_items(items, vals);
    PyObject *tup = _PyTuple_FromValueStealOnSuccess(vals, n);
    if (tup == NULL) {
        rop_close_items(vals, n);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(tup);
}

PyValue
_PyRop_BuildList(const _PyRopSpan *items)
{
    int n = items->count;
    PyValue vals[n + 1];
    rop_take_items(items, vals);
    PyObject *list = _PyList_FromValueStealOnSuccess(vals, n);
    if (list == NULL) {
        rop_close_items(vals, n);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(list);
}

PyValue
_PyRop_BuildSet(const _PyRopSpan *items)
{
    int n = items->count;
    PyValue vals[n + 1];
    rop_take_items(items, vals);
    PyObject *set = PySet_New(NULL);
    int err = set == NULL;
    for (int i = 0; i < n; i++) {
        if (err == 0) {
            // Steals the value, on failure too
            err = _PySet_AddValueTakeRef((PySetObject *)set, vals[i]);
        }
        else {
            PyValue_CLOSE(vals[i]);
        }
    }
    if (err) {
        Py_XDECREF(set);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(set);
}

PyValue
_PyRop_BuildMap(const _PyRopSpan *items)
{
    int n = items->count;
    assert(n % 2 == 0);
    PyValue vals[n + 1];
    rop_take_items(items, vals);
    PyObject *map = _PyDict_FromValueItems(vals, 2, vals + 1, 2, n / 2);
    rop_close_items(vals, n);
    if (map == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(map);
}

int
_PyRop_ListAppend(PyValue *list, PyValue item)
{
    PyListObject *list_o = (PyListObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(*list));
    return _PyList_AppendTakeValueInline(list_o, item) < 0 ? -1 : 0;
}

int
_PyRop_SetAdd(PyValue *set, PyValue item)
{
    PySetObject *set_o = (PySetObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(*set));
    return _PySet_AddValueTakeRef(set_o, item) ? -1 : 0;
}

int
_PyRop_DictPut(PyValue *dict, PyValue key, PyValue value)
{
    PyObject *dict_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*dict));
    assert(PyDict_CheckExact(dict_o));
    return _PyDict_SetValue_Take2((PyDictObject *)dict_o, key, value) != 0 ? -1 : 0;
}

/* The three updates read the container and the source from their homes. A
 * count of their own holds each across the call, which runs Python and may
 * reassign a local that names either. */
int
_PyRop_ListExtend(PyValue *list_home, PyValue *it_home)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue list_st = PyValue_DUP(PyValue_Borrow(*list_home));
    PyValue iterable_st = PyValue_DUP(PyValue_Borrow(*it_home));
    int res = 0;
    // A number is not iterable
    if (!PyValue_IsObject(PyValue_Borrow(iterable_st))) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "Value after * must be an iterable, not %.200s",
                      PyValue_TYPE(PyValue_Borrow(iterable_st))->tp_name);
        res = -1;
        goto done;
    }
    PyObject *list = PyValue_AsKnownObjectBorrow(PyValue_Borrow(list_st));
    PyObject *iterable = PyValue_AsKnownObjectBorrow(PyValue_Borrow(iterable_st));
    PyObject *none_val = _PyList_Extend((PyListObject *)list, iterable);
    if (none_val == NULL) {
        int matches = _PyErr_ExceptionMatches(tstate, PyExc_TypeError);
        if (matches &&
            (Py_TYPE(iterable)->tp_iter == NULL &&
             !_PyValue_SequenceCheck(PyValue_FromObjectBorrow(iterable))))
        {
            _PyErr_Clear(tstate);
            _PyErr_Format(tstate, PyExc_TypeError,
                          "Value after * must be an iterable, not %.200s",
                          Py_TYPE(iterable)->tp_name);
        }
        res = -1;
        goto done;
    }
    assert(Py_IsNone(none_val));
done:
    PyValue_CLOSE(iterable_st);
    PyValue_CLOSE(list_st);
    return res;
}

int
_PyRop_SetUnion(PyValue *set_home, PyValue *it_home)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue set = PyValue_DUP(PyValue_Borrow(*set_home));
    PyValue iterable = PyValue_DUP(PyValue_Borrow(*it_home));
    int res = 0;
    // A number is not iterable
    if (!PyValue_IsObject(PyValue_Borrow(iterable))) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object is not iterable",
                      PyValue_TYPE(PyValue_Borrow(iterable))->tp_name);
        res = -1;
    }
    else if (_PySet_Update(PyValue_AsKnownObjectBorrow(PyValue_Borrow(set)),
                           PyValue_AsKnownObjectBorrow(PyValue_Borrow(iterable))) < 0)
    {
        res = -1;
    }
    PyValue_CLOSE(iterable);
    PyValue_CLOSE(set);
    return res;
}

int
_PyRop_DictOverlay(PyValue *dict_home, PyValue *update_home)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue dict = PyValue_DUP(PyValue_Borrow(*dict_home));
    PyValue update = PyValue_DUP(PyValue_Borrow(*update_home));
    int res = 0;
    // A number is not a mapping
    if (!PyValue_IsObject(PyValue_Borrow(update))) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%N' object is not a mapping",
                      PyValue_TYPE(PyValue_Borrow(update)));
        res = -1;
        goto done;
    }
    PyObject *dict_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(dict));
    PyObject *update_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(update));
    if (PyDict_Update(dict_o, update_o) < 0) {
        int matches = _PyErr_ExceptionMatches(tstate, PyExc_AttributeError);
        if (matches) {
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            int has_keys = PyObject_HasAttrWithError(update_o, &_Py_ID(keys));
            if (has_keys == 0) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'%T' object is not a mapping",
                              update_o);
                Py_XDECREF(exc);
            }
            else {
                _PyErr_ChainExceptions1(exc);
            }
        }
        res = -1;
    }
done:
    PyValue_CLOSE(update);
    PyValue_CLOSE(dict);
    return res;
}

int
_PyRop_MergeKeywords(PyValue *fn_home, PyValue *dict_home, PyValue *update_home)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue dict = PyValue_DUP(PyValue_Borrow(*dict_home));
    PyValue update = PyValue_DUP(PyValue_Borrow(*update_home));
    PyValue callable = PyValue_DUP(PyValue_Borrow(*fn_home));
    int res = 0;
    // A number is not a mapping
    if (!PyValue_IsObject(PyValue_Borrow(update))) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "Value after ** must be a mapping, not %N",
                      PyValue_TYPE(PyValue_Borrow(update)));
        res = -1;
        goto done;
    }
    PyObject *dict_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(dict));
    PyObject *update_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(update));
    _PyStatusObject merged = value_dict_merge_uniq(dict_o, update_o);
    if (merged.status < 0) {
        _PyEval_FormatKwargsError(tstate, PyValue_Borrow(callable), update_o,
                                  merged.object);
        Py_XDECREF(merged.object);
        res = -1;
    }
done:
    PyValue_CLOSE(callable);
    PyValue_CLOSE(update);
    PyValue_CLOSE(dict);
    return res;
}

/* Unpack and UnpackStar (Python/register_ops/displays.rop): the values of
 * the iterable as owners the publish moves into the homes of the targets,
 * which are in stack order (the first target is the last element).  The source stays in its home and is held across the call. */
static int
rop_unpack(PyValue *source_home, int before, int after, Py_ssize_t count,
           _PyRopOwners *out)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue seq = PyValue_DUP(PyValue_Borrow(*source_home));
    int ok = 0;
    // A number is not iterable
    if (!PyValue_IsObject(PyValue_Borrow(seq))) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "cannot unpack non-iterable %.200s object",
                      PyValue_TYPE(PyValue_Borrow(seq))->tp_name);
        goto done;
    }
    PyValue *items = count ? PyMem_Malloc(count * sizeof(PyValue)) : NULL;
    if (count && items == NULL) {
        _PyErr_NoMemory(tstate);
        goto done;
    }
    PyObject *seq_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(seq));
    if (!_PyEval_UnpackIterableValue(tstate, seq_o, before, after, items)) {
        PyMem_Free(items);
        goto done;
    }
    // The outputs are in stack order, bottom first: the first target is last
    for (Py_ssize_t i = 0, j = count - 1; i < j; i++, j--) {
        PyValue tmp = items[i];
        items[i] = items[j];
        items[j] = tmp;
    }
    out->items = items;
    out->count = count;
    ok = 1;
done:
    PyValue_CLOSE(seq);
    return ok ? 0 : -1;
}

int
_PyRop_UnpackIterable(PyValue *source_home, Py_ssize_t n, _PyRopOwners *out)
{
    return rop_unpack(source_home, (int)n, -1, n, out);
}

int
_PyRop_UnpackStar(PyValue *source_home, Py_ssize_t shape, _PyRopOwners *out)
{
    int before = shape & 0xFF, after = shape >> 8;
    return rop_unpack(source_home, before, after, before + 1 + after, out);
}

/* The specialized arms: the items of an exact tuple or list of n items,
 * duplicated, in stack order.  The guard has checked the type and length; the
 * list's length is checked again under its lock, and a list that changed goes
 * the generic way. */
int
_PyRop_UnpackTuple(PyValue *source_home, Py_ssize_t n, _PyRopOwners *out)
{
    PyObject *seq_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*source_home));
    assert(PyTuple_CheckExact(seq_o) && PyTuple_GET_SIZE(seq_o) == n);
    PyValue *items = n ? PyMem_Malloc(n * sizeof(PyValue)) : NULL;
    if (n && items == NULL) {
        _PyErr_NoMemory(_PyThreadState_GET());
        return -1;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        items[n - 1 - i] = PyValue_DUP(PyValue_Borrow(_PyTuple_ITEMS(seq_o)[i]));
    }
    out->items = items;
    out->count = n;
    return 0;
}

int
_PyRop_UnpackList(PyValue *source_home, Py_ssize_t n, _PyRopOwners *out)
{
    PyObject *seq_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*source_home));
    assert(PyList_CheckExact(seq_o));
    PyValue *items = n ? PyMem_Malloc(n * sizeof(PyValue)) : NULL;
    if (n && items == NULL) {
        _PyErr_NoMemory(_PyThreadState_GET());
        return -1;
    }
    int same = 1;
    Py_BEGIN_CRITICAL_SECTION(seq_o);
    if (PyList_GET_SIZE(seq_o) != n) {
        same = 0;
    }
    else {
        for (Py_ssize_t i = 0; i < n; i++) {
            items[n - 1 - i] = PyValue_DUP(PyValue_Borrow(_PyList_ITEMS(seq_o)[i]));
        }
    }
    Py_END_CRITICAL_SECTION();
    if (!same) {
        PyMem_Free(items);
        return _PyRop_UnpackIterable(source_home, n, out);
    }
    out->items = items;
    out->count = n;
    return 0;
}

bool
_PyRop_TupleHasLength(PyValueBorrow v, Py_ssize_t n)
{
    return (PyValue_IsObject)(v)
           && PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(v))
           && PyTuple_GET_SIZE(PyValue_AsKnownObjectBorrow(v)) == n;
}

bool
_PyRop_ListHasLength(PyValueBorrow v, Py_ssize_t n)
{
    return (PyValue_IsObject)(v)
           && PyList_CheckExact(PyValue_AsKnownObjectBorrow(v))
           && PyList_GET_SIZE(PyValue_AsKnownObjectBorrow(v)) == n;
}

/* The case of classify_unpack: 0 an exact tuple of n items, 1 an exact list
 * of n items, 2 any other */
int
_PyRop_ClassifyUnpack(PyValueBorrow v, Py_ssize_t n)
{
    if (_PyRop_TupleHasLength(v, n)) {
        return 0;
    }
    return _PyRop_ListHasLength(v, n) ? 1 : 2;
}

void
_PyRop_StoreOwners(const _PyRopSpan *d, _PyRopOwners *v)
{
    // The row that made v returns count(d) items
    assert(v->count == d->count);
    for (int i = 0; i < d->count; i++) {
        int to = _PyIR_SpanElement(d->head, d->handler, d->first + i);
        PyValue old = _PyFrame_TakeHome(d->frame, &d->frame->base[to]);
        _PyFrame_WriteHome(d->frame, &d->frame->base[to], v->items[i]);
        v->items[i] = old;
    }
}

void
_PyRop_CloseOwners(_PyRopOwners *v)
{
    // Every home is written before any close runs a finalizer
    for (Py_ssize_t i = 0; i < v->count; i++) {
        PyValue_XCLOSE(v->items[i]);
    }
    PyMem_Free(v->items);
    v->items = NULL;
    v->count = 0;
}

/* All descriptors are local to this reached cold scope; no successful handler
 * gains an owner record. */
static Py_NO_INLINE _PyIRErrorResult
_PyEval_ProcessError(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *origin, uint32_t entry)
{
    unsigned int ingress = (entry >> 8) & 15;
    unsigned int checkpoint = entry & 255;
    if ((entry & ~UINT32_C(4095)) || ingress > _PyIR_ERROR_INGRESS_SOURCE ||
        (checkpoint != 0 && ingress != _PyIR_ERROR_INGRESS_RAISE &&
         ingress != _PyIR_ERROR_INGRESS_UNWIND))
    {
        Py_FatalError("invalid or unsupported emitted error checkpoint");
    }
    for (;;) {
        assert(frame == tstate->current_frame);
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);

        /* Retain a source callback's immutable recovery triple across bridge
         * cleanup. Its visible source location survives until traceback and
         * raise monitoring, just as on the original error path. */
        _PyEvalActivation *activation = ((_PyThreadStateImpl *)tstate)->eval_activation;
        int has_source = activation != NULL && activation->source_error_frame == frame;
        _PyExceptionHandler source = {0};
        if (has_source) {
            source = activation->source_error_handler;
        }
        if (ingress == _PyIR_ERROR_INGRESS_THROW) {
            monitor_throw(tstate, frame, origin + 1);
            ingress = _PyIR_ERROR_INGRESS_RAISE;
        }
        if (ingress == _PyIR_ERROR_INGRESS_RAISE ||
            ingress == _PyIR_ERROR_INGRESS_CALLER ||
            ingress == _PyIR_ERROR_INGRESS_SOURCE)
        {
#ifdef NDEBUG
            if (!_PyErr_Occurred(tstate)) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "error return without exception set");
            }
#else
            assert(_PyErr_Occurred(tstate));
#endif
            if (!_PyFrame_IsIncomplete(frame)) {
                PyFrameObject *f = _PyFrame_GetFrameObject(frame);
                if (f != NULL) {
                    PyTraceBack_Here(f);
                }
            }
            _PyEval_MonitorRaise(tstate, frame, origin);
        }
        int exiting = ingress == _PyIR_ERROR_INGRESS_EXIT ||
                      ingress == _PyIR_ERROR_INGRESS_EXIT_NOTRACE;
        _PyExceptionHandler found = {0};
        if (!exiting) {
            found = get_frame_exception_handler(
                frame, (int)_PyFrame_InstrOffset(frame, origin));
            if (has_source) {
                found = source;
            }
        }
        if (found.handled) {
            if (found.recipe) {
                error_apply_recipe(frame, found.recipe);
            }
            int first = found.exc_slot - found.lasti;
            error_clear_slots(frame, first);
            if (found.lasti) {
                _PyFrame_WriteHome(frame, &frame->base[first],
                    PyValue_TagIndex(_PyFrame_InstrOffset(frame, frame->instr_ptr)));
            }
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            _PyFrame_WriteHome(frame, &frame->base[found.exc_slot],
                              PyValue_FromObjectSteal(exc));
            _PyInstr *next = _PyFrame_InstrAt(frame, found.handler);
            int err = monitor_handled(tstate, frame, next, exc);
            if (err < 0) {
                /* Match the existing handler-entry error origin while using
                 * a fresh initialized snapshot for the next cleanup. */
                origin = next - 1;
                ingress = _PyIR_ERROR_INGRESS_HANDLED_FAILURE;
                checkpoint = 0;
                continue;
            }
#ifdef Py_DEBUG
            if (frame->lltrace >= 5) {
                lltrace_resume_frame(frame);
            }
#endif
            return (_PyIRErrorResult){next, _PyIR_ERROR_HANDLED};
        }
        assert(_PyErr_Occurred(tstate));
        /* Even a direct evaluator exit owns its remaining scratch. Pass only
         * bindings to M2, rather than asking GEN retirement to scan old N. */
        error_clear_slots(frame, (int)_PyFrame_Exec(frame)->nfixed);
        if (!exiting) {
            monitor_unwind(tstate, frame, origin);
        }
        if (ingress != _PyIR_ERROR_INGRESS_EXIT_NOTRACE) {
            DTRACE_FUNCTION_RETURN();
        }
        _Py_LeaveRecursiveCallPy(tstate);
        _PyInterpreterFrame *dying = frame;
        frame = tstate->current_frame = dying->previous;
        frame->pending_redirect = 0;
        clear_error_frame(tstate, dying);
        frame->pending_redirect = 0;
        if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
            tstate->current_frame = frame->previous;
            return (_PyIRErrorResult){NULL, _PyIR_ERROR_EXIT};
        }
        origin = frame->instr_ptr;
        ingress = _PyIR_ERROR_INGRESS_CALLER;
        checkpoint = 0;
    }
}

/* The helpers of the generator transfer labels (Python/ceval_generator.h),
 * which are in the evaluator below. */

static inline PyValue
generator_payload(_PyIRWord payload)
{
    return _PyValue_FromTransferWord(payload);
}

static inline uint64_t
generator_control_generation(_PyInterpreterFrame *frame)
{
    /* Interpreter entry frames do not initialize frame_obj. */
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }
    return frame->frame_obj == NULL ? 0 : frame->frame_obj->f_lineno_generation;
}

/* The generator that has just completed (or an awaited coroutine) is closed
 * by its last owner, which frees it without running code if it is plain. */
static inline bool
gen_try_close_completed(PyValue receiver)
{
    return PyValue_RefcountOnObject(receiver) &&
        _PyGen_TryCloseClearedNoReentry(PyValue_AsKnownObjectBorrow(receiver));
}

/* Close one input of a completed send. */
static inline bool
gen_close_input(PyValue input, bool receiver)
{
    return (receiver && gen_try_close_completed(input)) ||
        _PyValue_TryXCloseNoReentry(input);
}

/* The kind that a generator table gives a word's handler (the enums of
 * Python/ceval_generator.h), or 0 for none.  Ordinary outcomes need one
 * dispatch-table-shaped lookup.  A transparent monitor keeps the same word
 * fields but resolves its source only here, on the actual observation path. */
static Py_NO_INLINE uint8_t
generator_transparent_site_kind(_PyInterpreterFrame *frame, _PyInstr *site,
                                const uint8_t *table)
{
    return table[_PyIR_RealHandler(frame, site)];
}

static inline uint8_t
generator_site_kind(_PyInterpreterFrame *frame, _PyInstr *site,
                    const uint8_t *table)
{
    /* The word of an interpreter entry frame is the trampoline's, which no
     * table names and no monitor wraps. */
    int handler = IR_HANDLER(*site);
    uint8_t kind = table[handler];
    if (kind == 0 && (_PyIR_Formats[handler].flags & _PyIR_TRANSPARENT)) {
        return generator_transparent_site_kind(frame, site, table);
    }
    return kind;
}

/* The two halves of the plain-transfer test, which contain no effect.  A
 * monitored or redirected transfer publishes its homes before running the
 * ordinary source's callback boundary.  The thread's half cannot change
 * while a transfer runs no callback, so a transfer that is a chain of
 * levels tests it once and each level tests the frame's half. */
static inline bool
generator_thread_is_plain(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    return false;
#else
    if (IS_PEP523_HOOKED(tstate)) {
        return false;
    }
#ifdef WITH_DTRACE
    if (PyDTrace_FUNCTION_ENTRY_ENABLED() || PyDTrace_FUNCTION_RETURN_ENABLED()) {
        return false;
    }
#endif
    return true;
#endif
}

static inline bool
generator_frame_is_plain(_PyInterpreterFrame *frame)
{
    if (frame->pending_redirect || _PyFrame_GetCode(frame)->_co_monitoring != NULL) {
        return false;
    }
#ifdef Py_DEBUG
    if (frame->lltrace >= 5) {
        return false;
    }
#endif
    return true;
}

static inline bool
generator_transfer_is_plain(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    return generator_thread_is_plain(tstate) && generator_frame_is_plain(frame);
}

/* The caller holds value in C, which is an external reference to the cyclic
 * collector. Every frame write precedes the first close. */
static inline void
generator_publish_redirected(PyThreadState *tstate, _PyInterpreterFrame *frame,
                             PyValue value)
{
    if (frame->pending_redirect) {
        _PyFrame_CompleteRedirect(frame);
    }
    _PyFrame_PublishReturnSlot(frame, value);
}

/* The publication of GEN_COMPLETE_EFFECT, out of line: the detached owners
 * are an array, which would be the handler's. Detaches the remaining
 * owners, writes result to the destination, observes the completed step,
 * and closes everything detached. Returns the exception that observation
 * raised, or NULL. */
static Py_NO_INLINE PyObject *
generator_complete_publish(PyThreadState *tstate, _PyInterpreterFrame *frame,
                           _PyInstr *site, _PyInstr *exit, int base,
                           int remaining_count, int destination,
                           bool throwing, PyValue result)
{
    /* Detach every remaining owner before an overlapping A publication. */
    PyValue detached[5];
    int ndetached = 0;
    for (int k = remaining_count; k-- > 0;) {
        detached[ndetached++] = frame->base[base + k];
        frame->base[base + k] = PyValue_NULL;
    }
    // The releasing write: a local's or the discard slot's old value closes
    // with the detached owners
    PyValue displaced = frame->base[destination];
    assert(destination < (int)_PyFrame_Exec(frame)->nfixed ||
           destination == _PyFrame_DiscardSlot(frame) ||
           PyValue_IsNull(displaced));
    frame->base[destination] = result;
    _PyValue_CheckHome(result);
    detached[ndetached++] = displaced;
    frame->return_pc = exit;
    _PyIRStepResult post = {.next = exit, .status = _PY_IR_STEP_EXIT};
    if (_PyFrame_GetCode(frame)->_co_monitoring != NULL) {
        post = _PyIR_ObserveStep(tstate, frame, site, exit,
            PyValue_Borrow(PyValue_None), throwing ? _PY_IR_SEND_THROW : _PY_IR_SEND, 1);
    }
    if (post.status >= 0) { frame->instr_ptr = post.next; }
    PyObject *error = post.status < 0 ? PyErr_GetRaisedException() : NULL;
    for (int k = 0; k < ndetached; k++) {
        PyValue_XCLOSE(detached[k]);
    }
    return error;
}

/* The function forms (tiers87, C2).  Every handler's body is also a function
 * of its own, ir_run_X(tstate, frame, next_instr, ir_word), and every VM
 * routine (Python/ceval_routines.h) and DSL label is a function ir_NAME; each
 * returns a _PyIRTransfer.  The evaluator's stub of an outlined handler calls
 * its form (IR_RUN_OUTLINED()); the function-form executor,
 * ir_execute_forms(), calls them all, as compiled code will.  Here the
 * transfer macros are returns: the next word is returned; a deoptimisation,
 * a frame entry, a VM routine and a handler run on the same word are tail
 * calls; an error or the end of the activation returns no word and is
 * described in the activation, for the caller to resolve. */
#pragma push_macro("DISPATCH")
#undef DISPATCH
#pragma push_macro("DISPATCH_SAME_OPARG")
#undef DISPATCH_SAME_OPARG
#pragma push_macro("DISPATCH_GOTO")
#undef DISPATCH_GOTO
#pragma push_macro("DISPATCH_INLINED")
#undef DISPATCH_INLINED
#pragma push_macro("PROCESS_ERROR_AT")
#undef PROCESS_ERROR_AT
#pragma push_macro("IR_EXIT")
#undef IR_EXIT
#pragma push_macro("JUMP_TO_PREDICTED")
#undef JUMP_TO_PREDICTED
#pragma push_macro("IR_DEOPT_BINARY_OP")
#undef IR_DEOPT_BINARY_OP
#pragma push_macro("IR_OBSERVE_EDGE")
#undef IR_OBSERVE_EDGE
#pragma push_macro("IR_HOLD_NEXT")
#undef IR_HOLD_NEXT
#pragma push_macro("IR_STEP_DONE")
#undef IR_STEP_DONE
#pragma push_macro("IR_REPORT_EDGE")
#undef IR_REPORT_EDGE
#pragma push_macro("IR_GEN_YIELD_AT")
#undef IR_GEN_YIELD_AT
#pragma push_macro("IR_GEN_COMPLETE_AT")
#undef IR_GEN_COMPLETE_AT
#pragma push_macro("IR_GEN_RETURN")
#undef IR_GEN_RETURN
#pragma push_macro("IR_GEN_RESUME")
#undef IR_GEN_RESUME
#pragma push_macro("IR_ROUTINE")
#undef IR_ROUTINE
#pragma push_macro("IR_HOT_ROUTINE")
#undef IR_HOT_ROUTINE
#pragma push_macro("GEN_DISPATCH")
#undef GEN_DISPATCH
#pragma push_macro("GEN_STEP_JUMP")
#undef GEN_STEP_JUMP
#pragma push_macro("GEN_STEP_EFFECT")
#undef GEN_STEP_EFFECT
#pragma push_macro("IR_JIT_EVENT")
#undef IR_JIT_EVENT
#pragma push_macro("IR_JIT_EVENT_AT")
#undef IR_JIT_EVENT_AT
#pragma push_macro("IR_JIT_RESUME")
#undef IR_JIT_RESUME
#pragma push_macro("IR_STORE_RETURN")
#undef IR_STORE_RETURN
#pragma push_macro("IR_PIN_DEPARTING")
#undef IR_PIN_DEPARTING
#define DISPATCH() return (_PyIRTransfer){frame, next_instr}
#define DISPATCH_SAME_OPARG() return (_PyIRTransfer){frame, next_instr}
#define DISPATCH_GOTO() \
    do { \
        Py_MUSTTAIL return ir_handler_forms[opcode](tstate, frame, next_instr, ir_word); \
    } while (0)
#define DISPATCH_INLINED(NEW_FRAME) \
    do { \
        assert(!IS_PEP523_HOOKED(tstate)); \
        assert((NEW_FRAME)->previous == frame || \
               (NEW_FRAME)->previous->previous == frame); \
        frame = tstate->current_frame = (NEW_FRAME); \
        CALL_STAT_INC(inlined_py_calls); \
        Py_MUSTTAIL return ir_start_frame(tstate, frame, next_instr); \
    } while (0)
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_kind = _PyIR_EXIT_RAISE; \
        ir_exit->exit_next = (origin); \
        ir_exit->exit_entry = \
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        return (_PyIRTransfer){frame, NULL}; \
    } while (0)
#define IR_EXIT(V) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_kind = _PyIR_EXIT_VALUE; \
        ir_exit->exit_value = (V); \
        return (_PyIRTransfer){frame, NULL}; \
    } while (0)
#define JUMP_TO_PREDICTED(name) \
    do { \
        Py_MUSTTAIL return ir_run_##name(tstate, frame, this_instr, ir_word); \
    } while (0)
#define IR_DEOPT_BINARY_OP(F) \
    do { \
        Py_MUSTTAIL return ir_binary_op( \
            tstate, frame, this_instr, ir_word, \
            (int)((_PyFbHeader *)IR_FB(F))->arg); \
    } while (0)
#define IR_HOLD_NEXT() ((void)0)
#define IR_STEP_DONE(STEP) return (STEP)
#define IR_REPORT_EDGE(TAKEN) (IR_ACTIVATION()->edge = (TAKEN))
#define IR_OBSERVE_EDGE(SOURCE, NEXT, EVENT, EDGE) \
    return ir_event_edge(tstate, frame, this_instr, (SOURCE), (NEXT), (EVENT), (EDGE))
#define IR_GEN_YIELD_AT(SITE, V) \
    do { \
        Py_MUSTTAIL return ir_gen_yield(tstate, frame, (SITE), (V)); \
    } while (0)
#define IR_GEN_COMPLETE_AT(SITE, V) \
    do { \
        Py_MUSTTAIL return ir_gen_complete(tstate, frame, (SITE), (V)); \
    } while (0)
#define IR_GEN_RETURN(V, DYING, EMPTY) \
    do { \
        Py_MUSTTAIL return ir_gen_return(tstate, frame, next_instr, (V), \
                                         (DYING), (EMPTY)); \
    } while (0)
#define IR_GEN_RESUME(V) \
    do { \
        Py_MUSTTAIL return ir_gen_resume(tstate, frame, next_instr, (V)); \
    } while (0)
#define IR_ROUTINE(NAME, ...) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_##NAME(PyThreadState *tstate, _PyInterpreterFrame *frame, \
              _PyInstr *next_instr, __VA_ARGS__)
#define IR_HOT_ROUTINE IR_ROUTINE
#define GEN_DISPATCH() return (_PyIRTransfer){frame, next_instr}
#define GEN_STEP_JUMP(NAME, F, IP, V) \
    do { \
        Py_MUSTTAIL return ir_##NAME(tstate, (F), (IP), _PyValue_ToTransferWord(V)); \
    } while (0)
#define GEN_STEP_EFFECT(REMAINING, IP, V) \
    do { \
        Py_MUSTTAIL return ir_GEN_COMPLETE_EFFECT(tstate, frame, (IP), _PyValue_ToTransferWord(V), \
                                                  (REMAINING)); \
    } while (0)
/* Compiled code runs the forms: it is in its unit already, and leaves it
 * at every frame switch for the driver, which finds the next frame's unit
 * itself.  So a return store tests only a pending redirect, not the
 * frame's _PyFrame_JIT_RESUME flag. */
#define IR_JIT_EVENT() ((void)0)
#define IR_JIT_EVENT_AT(NEXT) ((void)(NEXT))
#define IR_JIT_RESUME() ((void)0)
/* The pin's release status is not needed: every pinned arm runs Python, so
 * the form's continuation tests RETIRED when the word stays in its frame */
#define IR_PIN_DEPARTING() ((void)_PyJit_PinDeparting(tstate, frame))
#define IR_STORE_RETURN(V) \
    do { \
        assert(_PyValue_IsHome(V)); \
        if (frame->pending_redirect) { \
            _PyFrame_SetReturnValueRare(frame, (V)); \
            next_instr = frame->return_pc; \
        } \
        else { \
            _PyFrame_PublishReturnSlot(frame, (V)); \
        } \
    } while (0)

/* The event of an instrumented branch, jump or loop step, after the effect
 * (rule 12).  It is out of the evaluator, where the instrumented handlers
 * only call it.  Its result is the word to run (the callbacks can redirect
 * it) or no word for an error. */
static Py_NO_INLINE _PyIRTransfer
ir_event_edge(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _PyInstr *this_instr, _PyInstr *source, _PyInstr *next,
              int event, int edge)
{
    frame->return_pc = next;
    _PyIRObserverResult result = _PyIR_ObserveCommittedBranch(
        tstate, frame, this_instr, source, next, event, edge);
    PyObject *exception = result.status < 0 ? PyErr_GetRaisedException() : NULL;
    assert(result.status >= 0 || exception != NULL);
    frame->instr_ptr = result.next;
    if (result.status < 0) {
        PyErr_SetRaisedException(exception);
        PROCESS_ERROR_AT(RAISE, 0, result.next);
    }
    return (_PyIRTransfer){frame, result.next};
}

/* The events that precede the effect of an instrumented twin (rule 12).  Each
 * leaves the state as the twin's handler left it at the event: an error
 * unwinds with the operand as the event took it. */
static Py_NO_INLINE _PyIRTransfer
ir_event_yield(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _PyInstr *next_instr, _PyIRWord ir_word)
{
    _PyInstr *this_instr = next_instr;
    frame->instr_ptr = this_instr;
    PyValue val = IR_SLOT(IR_B(ir_word));
    int err = _Py_call_instrumentation_arg(
        tstate, PY_MONITORING_EVENT_PY_YIELD, frame, this_instr, val);
    if (err) {
        PROCESS_ERROR_AT(RAISE, 0, this_instr);
    }
    /* The callbacks can have jumped. */
    return (_PyIRTransfer){frame, frame->instr_ptr};
}

static Py_NO_INLINE _PyIRTransfer
ir_event_end_for(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *next_instr, _PyIRWord ir_word)
{
    _PyInstr *this_instr = next_instr;
    PyValueBorrow value = IR_VIEW(IR_C(ir_word));
    PyValue receiver = IR_SLOT(IR_A(ir_word));
    if (PyValue_GenCheck(receiver)) {
        int err = monitor_stop_iteration(tstate, frame, this_instr, value);
        if (err) {
            PROCESS_ERROR_AT(RAISE, 0, this_instr);
        }
    }
    return (_PyIRTransfer){frame, this_instr};
}

/* The helpers of two outlined handlers (BINARY_SLICE, CALL_INTRINSIC_1), in
 * line in the functions that run them: the handler is a function of its own
 * already, and a second call in front of the work only adds a frame. They
 * are defined below, with the code they share. */
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_BinarySlice)(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop);
#define _PyEval_BinarySlice(V0, V1, V2) \
    (_PyEval_BinarySlice)(PyValue_Borrow(V0), PyValue_Borrow(V1), PyValue_Borrow(V2))
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_CallIntrinsic1)(PyThreadState *tstate, int oparg, PyValueBorrow value);
#define _PyEval_CallIntrinsic1(T, O, V) (_PyEval_CallIntrinsic1)(T, O, PyValue_Borrow(V))

/* The rows of the item families (Python/register_ops/items.rop) */
static inline PyValue
_PyRop_Subscript(PyValueBorrow container, PyValueBorrow key)
{
    return value_subscript(container, key);
}

static inline int
_PyRop_SetItem(PyValueBorrow container, PyValueBorrow key, PyValueBorrow value)
{
    return _PyValue_SetItem(container, key, value);
}

static inline int
_PyRop_DelItem(PyValueBorrow container, PyValueBorrow key)
{
    return _PyValue_DelItem(container, key);
}

static inline PyValue
_PyRop_BinarySlice(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop)
{
    return (_PyEval_BinarySlice)(container, start, stop);
}

static inline int
_PyRop_StoreSlice(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop,
                  PyValueBorrow value)
{
    return (_PyEval_StoreSlice)(value, container, start, stop);
}

static inline bool
_PyRop_ExactTuple(PyValueBorrow v)
{
    return PyValue_IsObject(v) && PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_ExactSlice(PyValueBorrow v)
{
    return PyValue_IsObject(v) && PySlice_Check(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_CompactAsciiStr(PyValueBorrow v)
{
    if (!PyValue_IsObject(v)) {
        return false;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    return PyUnicode_CheckExact(o) && PyUnicode_IS_COMPACT_ASCII(o);
}

/* The index of a list folds from the end, as the old FROM_END guard did */
static inline Py_ssize_t
_PyRop_ListIndex(PyObject *list, PyValueBorrow k)
{
    Py_ssize_t index = PyValue_InlineInt64(k);
    return index < 0 ? index + PyList_GET_SIZE(list) : index;
}

static inline bool
_PyRop_ListInBounds(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *list = PyValue_AsKnownObjectBorrow(v);
    Py_ssize_t index = _PyRop_ListIndex(list, k);
    return 0 <= index && index < PyList_GET_SIZE(list);
}

static inline PyValueBorrow
_PyRop_ListItem(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *list = PyValue_AsKnownObjectBorrow(v);
    return PyValue_Borrow(_PyList_ITEMS(list)[_PyRop_ListIndex(list, k)]);
}

static inline bool
_PyRop_TupleInBounds(PyValueBorrow v, PyValueBorrow k)
{
    Py_ssize_t index = PyValue_InlineInt64(k);
    return 0 <= index && index < PyTuple_GET_SIZE(PyValue_AsKnownObjectBorrow(v));
}

static inline PyValueBorrow
_PyRop_TupleItem(PyValueBorrow v, PyValueBorrow k)
{
    return PyTuple_GET_VALUE(PyValue_AsKnownObjectBorrow(v), PyValue_InlineInt64(k));
}

static inline bool
_PyRop_StrInBounds(PyValueBorrow v, PyValueBorrow k)
{
    Py_ssize_t index = PyValue_InlineInt64(k);
    return 0 <= index && index < PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(v));
}

static inline PyValueBorrow
_PyRop_AsciiChar(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *str = PyValue_AsKnownObjectBorrow(v);
    uint8_t c = PyUnicode_1BYTE_DATA(str)[PyValue_InlineInt64(k)];
    assert(c < 128);
    return PyValue_FromObjectBorrow((PyObject *)&_Py_SINGLETON(strings).ascii[c]);
}

static inline PyValueBorrow
_PyRop_UnicodeAsciiChar(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *str = PyValue_AsKnownObjectBorrow(v);
    Py_UCS4 c = PyUnicode_READ_CHAR(str, PyValue_InlineInt64(k));
    if (Py_ARRAY_LENGTH(_Py_SINGLETON(strings).ascii) <= c) {
        return PyValue_Borrow(PyValue_NULL);
    }
    return PyValue_FromObjectBorrow((PyObject *)&_Py_SINGLETON(strings).ascii[c]);
}

static inline bool
_PyRop_DictSubscriptSlot(PyValueBorrow v)
{
    if (!PyValue_IsObject(v)) {
        return false;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    return tp->tp_as_mapping != NULL
        && tp->tp_as_mapping->mp_subscript == _PyDict_Subscript;
}

static inline bool
_PyRop_DictAssignSlot(PyValueBorrow v)
{
    if (!PyValue_IsObject(v)) {
        return false;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    return tp->tp_as_mapping != NULL
        && tp->tp_as_mapping->mp_ass_subscript == _PyDict_StoreSubscript;
}

static inline bool
_PyRop_DictFirstProbe(PyValueBorrow v, PyValueBorrow k, PyValue *out)
{
    PyValue hit = _PyDict_TryGetFirstProbeValue(v, k);
    if (PyValue_IsNull(hit)) {
        return false;
    }
    *out = hit;
    return true;
}

/* The lock is held for the exchange only.  The guard proved the index, and
 * this build has the GIL. */
static inline PyValue
_PyRop_ListExchange(PyValueBorrow v, PyValueBorrow k, PyValue x)
{
    PyObject *list = PyValue_AsKnownObjectBorrow(v);
    int locked = LOCK_OBJECT(list);
    assert(locked);
    (void)locked;
    Py_ssize_t index = _PyRop_ListIndex(list, k);
    assert(0 <= index && index < PyList_GET_SIZE(list));
    PyValue old = _PyValueSlot_ExchangeFilled(_PyList_ITEMS(list) + index, x);
    UNLOCK_OBJECT(list);
    return old;
}

static inline PyValue
_PyRop_ListSlice(PyValueBorrow v, PyValueBorrow s)
{
    PyObject *res = _PyList_SliceSubscript(PyValue_AsKnownObjectBorrow(v),
                                           PyValue_AsKnownObjectBorrow(s));
    return res == NULL ? PyValue_NULL : PyValue_FromNonNumberSteal(res);
}

static inline int
_PyRop_DictStore(PyValueBorrow v, PyValueBorrow k, PyValueBorrow x)
{
    return _PyDict_SetValue((PyDictObject *)PyValue_AsKnownObjectBorrow(v), k, x);
}

/* The slice of two or three parts.  Each temporary part leaves its home and
 * is closed on both paths, a local stays; the error is set aside while a
 * finalizer may run. */
static PyValue
_PyRop_SliceFromParts(const _PyRopSpan *parts)
{
    _PyInterpreterFrame *frame = parts->frame;
    int n = parts->count;
    assert(n == 2 || n == 3);
    PyValue *home[3];
    for (int i = 0; i < n; i++) {
        int slot = _PyIR_SpanElement(parts->head, parts->handler, parts->first + i);
        home[i] = &frame->base[slot];
    }
    // The bounds are stored as the values they are: no box of an inline one
    PyValueBorrow step = PyValue_Borrow(PyValue_NULL);
    if (n == 3) {
        step = PyValue_Borrow(*home[2]);
    }
    PyObject *slice = _PySlice_FromValues(
        PyValue_Borrow(*home[0]), PyValue_Borrow(*home[1]), step);
    PyObject *exc = slice == NULL ? PyErr_GetRaisedException() : NULL;
    PyValue taken[3];
    for (int i = 0; i < n; i++) {
        taken[i] = IR_IS_LOCAL(home[i] - frame->base)
            ? PyValue_NULL : _PyFrame_TakeHome(frame, home[i]);
    }
    for (int i = 0; i < n; i++) {
        PyValue_XCLOSE(taken[i]);
    }
    if (slice == NULL) {
        PyErr_SetRaisedException(exc);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(slice);
}

#include "ceval_generator.h"

/* The routines and handlers are printed for the evaluator too, whose
 * bindings of the transfer macros read some locals that the function forms'
 * do not (an `int opcode` before a dispatch, a local that only a dispatch
 * keeps live). */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
/* The routines call one another: gen_return completes a send, and a send
 * whose consumer is a return returns. */
IR_HOT_ROUTINE(gen_complete, PyValue gen_value);
#include "ceval_routines.h"

#define IR_FUNCTION_FORMS
#include "generated_ir_cases.c.h"
#undef IR_FUNCTION_FORMS
/* The whole-word completion stubs of the planned arms (regions89 G1a),
 * from the same handler text, with the forms' bindings of the macros they
 * do not bind themselves */
#include "generated_ir_stubs.c.h"
/* The forms and stubs of the register instruction arms (Python/register_ops);
 * the tables above list them */
#include "generated_rop_forms.c.h"
#include "generated_rop_stubs.c.h"
#pragma GCC diagnostic pop

#ifdef _PY_JIT_SUPPORTED
const void *const *
_PyEval_HandlerForms(void)
{
    return (const void *const *)ir_handler_forms;
}

/* The raise exit of a baseline JIT template (Python/jit_template.c): its
 * handler's PROCESS_ERROR_AT(RAISE, 0, this_instr), where the template
 * published this_instr as frame->instr_ptr.  Machine code calls it and
 * takes the transfer in rax:rdx. */
static _PyIRTransfer
jit_raise_exit(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyInstr *this_instr = frame->instr_ptr;
    PROCESS_ERROR_AT(RAISE, 0, this_instr);
}

const void *const _PyEval_JitRaiseExit = (const void *)jit_raise_exit;
/* The unit that frame goes on in, or NULL to go on in the evaluator: its
 * code's newest unit, unless that is invalid, when the warm-up event
 * decides (a stale unit is recompiled; a retired one is not run).  It is
 * looked up afresh each time: monitoring retires units and a recompile
 * replaces them while the frame waits. */
static inline _PyJitUnit *
jit_unit_of(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        return NULL;
    }
#ifdef Py_DEBUG
    if (frame->lltrace >= 5) {
        return NULL;
    }
#endif
    _PyJitUnit *unit = _PyFrame_Exec(frame)->jit_unit;
    if (unit != NULL && unit->invalid != 0) {
        unit = _PyJit_Event(tstate, frame);
    }
    return unit;
}

typedef _PyIRTransfer (*jit_entry)(PyThreadState *tstate,
                                   _PyInterpreterFrame *frame,
                                   const void *entry);

/* The baseline JIT's driver, which the evaluator calls when frame, at the
 * word next, is to go on in unit (IR_JIT_EVENT(), IR_JIT_RESUME() in
 * Python/ceval_macros.h).  Its native call runs frames by switching between
 * units (the switch routine) until a transfer has no unit to go on in, has
 * raised, or leaves the last entered frame for a word that its unit does not
 * run; the driver goes on only in that last case, while the frame's newest
 * unit is not retired.  The switch routine names the last entered frame in
 * jit_frame; a leave in another frame is one that the routine declined
 * (g3/design.md section 1, "jit_run after G3a").  The C stack stays one unit
 * deep. */
static Py_NO_INLINE _PyIRTransfer
jit_run(PyThreadState *tstate, _PyJitUnit *unit, _PyInterpreterFrame *frame,
        _PyInstr *next)
{
    _PyThreadStateImpl *impl = (_PyThreadStateImpl *)tstate;
    _PyInterpreterFrame *outer = impl->jit_frame;
    _PyEvalActivation *activation = IR_ACTIVATION();
    for (;;) {
        /* The unit addresses the words from frame->ir_words */
        assert(frame->ir_words == unit->words);
        assert(next >= frame->ir_words && next < frame->ir_words + unit->nwords);
        frame->jit_flags |= _PyFrame_JIT_RESUME;
        impl->jit_frame = frame;
#ifndef NDEBUG
        activation->jit_unit = unit;
#endif
        tstate->interp->ceval.jit_turns++;
        _PyIRTransfer t = ((jit_entry)unit->code.start)(
            tstate, frame, unit->entries[next - frame->ir_words]);
        _PyInterpreterFrame *entered = impl->jit_frame;
        impl->jit_frame = outer;
#ifndef NDEBUG
        /* Before the release, which may free the unit just left */
        activation->jit_unit = NULL;
#endif
        /* No unit runs: the code that a unit departed from last may go */
        Py_CLEAR(activation->jit_pin);
        if (t.next == NULL || t.frame != entered) {
            return t;
        }
        /* Not a warm-up event: a unit leaves for a word that it does not run
         * natively, as after a specialisation, and the newest unit runs on
         * as it is while it is only stale */
        unit = _PyFrame_Exec(t.frame)->jit_unit;
        if (unit->invalid & _PyJIT_RETIRED) {
            return t;
        }
        frame = t.frame;
        next = t.next;
    }
}

/* Control has come back to frame, which ran in its code's unit, at next in
 * the evaluator (IR_JIT_RESUME()): it goes back to the unit if the code has
 * one to run; otherwise it stays in the evaluator, and is no longer marked. */
static Py_NO_INLINE _PyIRTransfer
jit_resume(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *next)
{
    _PyJitUnit *unit = jit_unit_of(tstate, frame);
    if (unit == NULL) {
        frame->jit_flags &= ~_PyFrame_JIT_RESUME;
        return (_PyIRTransfer){frame, next};
    }
    return jit_run(tstate, unit, frame, next);
}
#endif

/* The function-form executor (-X ir_forms), a test of the forms: runs the
 * activation's frames by calling each word's function form, as compiled
 * code does, in place of ir_evaluate().  A form that returns no word has
 * raised, which the error routine resolves, or has ended the activation. */
static Py_NO_INLINE PyValue
ir_execute_forms(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _PyEvalActivation *activation = IR_ACTIVATION();
    _PyIRTransfer t = throwflag ? ir_throw_error(tstate, frame, frame->instr_ptr)
                                : ir_start_frame(tstate, frame, frame->instr_ptr);
    for (;;) {
        if (t.next == NULL) {
            if (activation->exit_kind == _PyIR_EXIT_VALUE) {
                return activation->exit_value;
            }
            assert(activation->exit_kind == _PyIR_EXIT_RAISE);
            t = ir_process_error(tstate, t.frame, NULL, activation->exit_next,
                                 activation->exit_entry);
            continue;
        }
        frame = t.frame;
        _PyInstr *next_instr = t.next;
        _PyIRWord ir_word;
        int opcode;
        NEXTOPARG();
        PRE_DISPATCH_GOTO();
        t = ir_handler_forms[opcode](tstate, frame, next_instr, ir_word);
    }
}
#pragma pop_macro("DISPATCH")
#pragma pop_macro("DISPATCH_SAME_OPARG")
#pragma pop_macro("DISPATCH_GOTO")
#pragma pop_macro("DISPATCH_INLINED")
#pragma pop_macro("PROCESS_ERROR_AT")
#pragma pop_macro("IR_EXIT")
#pragma pop_macro("JUMP_TO_PREDICTED")
#pragma pop_macro("IR_DEOPT_BINARY_OP")
#pragma pop_macro("IR_OBSERVE_EDGE")
#pragma pop_macro("IR_HOLD_NEXT")
#pragma pop_macro("IR_STEP_DONE")
#pragma pop_macro("IR_REPORT_EDGE")
#pragma pop_macro("IR_GEN_YIELD_AT")
#pragma pop_macro("IR_GEN_COMPLETE_AT")
#pragma pop_macro("IR_GEN_RETURN")
#pragma pop_macro("IR_GEN_RESUME")
#pragma pop_macro("IR_ROUTINE")
#pragma pop_macro("IR_HOT_ROUTINE")
#pragma pop_macro("GEN_DISPATCH")
#pragma pop_macro("GEN_STEP_JUMP")
#pragma pop_macro("GEN_STEP_EFFECT")
#pragma pop_macro("IR_JIT_EVENT_AT")
#pragma pop_macro("IR_JIT_EVENT")
#pragma pop_macro("IR_JIT_RESUME")
#pragma pop_macro("IR_STORE_RETURN")
#pragma pop_macro("IR_PIN_DEPARTING")

/* The evaluator: every handler and label of the interpreter in one
 * function, which dispatches by computed goto (Python/ceval_macros.h).  Its
 * locals are the interpreter's state and are never address-taken; the
 * activation record and the entry frame, whose addresses are taken, are in
 * the caller, _PyEval_EvalFrameLoop().  It runs the frame to its end: the
 * value that INTERPRETER_EXIT takes, or NULL with an exception set.
 * start_frame takes the word from frame. */
static Py_NO_INLINE PyValue
ir_evaluate(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
#include "ir_targets.h"
    _PyInstr *next_instr = frame->instr_ptr;
    _PyIRWord ir_word = 0;
    /* The operands of the shared bodies that the labels below read: the
     * generic BINARY_OP's operation and the VM routines' (Python/
     * ceval_routines.h), which the transfer macros set.  None is live across
     * a dispatch. */
    int binary_op_operation;
    _PyInstr *process_error_origin;
    uint32_t process_error_entry;
    PyValue gen_value;
    _PyInterpreterFrame *gen_dying;
    int gen_empty;
    _PyIRTransfer gen_transfer;

    if (throwflag) {
        goto LABEL_throw_error;
    }
    /* Enter through a computed goto, as dispatch does.  GCC's static profile
     * then reaches the handler labels from here; a plain goto carries a
     * "predicted unlikely" estimate and leaves every handler at count 0, so
     * it is optimised for size and its closes stay out of line. */
    goto *&&LABEL_start_frame;

#include "generated_ir_cases.c.h"
#include "generated_rop_cases.c.h"

    /* A function form returned no word: it raised (ir_execute_forms() has the
     * same contract), and process_error resolves it. */
    LABEL_outlined_exit:
    {
        _PyEvalActivation *activation = IR_ACTIVATION();
        assert(activation->exit_kind == _PyIR_EXIT_RAISE);
        process_error_origin = activation->exit_next;
        process_error_entry = activation->exit_entry;
        goto LABEL_process_error;
    }

    /* The shared dispatch blocks of the generator routines: gen_dispatch runs
     * the word at next_instr, and gen_stepped runs what a step returned. */
    HOT_LABEL(gen_dispatch)
    {
        int opcode;
        DISPATCH();
    }

    HOT_LABEL(gen_stepped)
    {
        frame = gen_transfer.frame;
        next_instr = gen_transfer.next;
        if (next_instr == NULL) {
            goto LABEL_outlined_exit;
        }
        goto LABEL_gen_dispatch;
    }

#include "ceval_routines.h"
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

/* The evaluation loop: one activation.  It is a borrow scope with a floor
 * (pyjit/design/native-boundary.md, section 2).
 *
 * The release invariant: when _Py_HandlePending() releases the entries above
 * eval_floor, no live C frame between the floor and the current instruction
 * holds a mark or a borrow.  Every caller of _Py_HandlePending() is at an
 * instruction boundary of this activation, where all native code it called
 * has returned.  So the floor is set here, around the evaluator, and not in
 * _PyEval_EvalFrame(): a PEP 523 hook runs C code between the two, and
 * anything it pushed would be released while still live.
 *
 * The loop sets up the activation and calls the evaluator, ir_evaluate(),
 * which runs the frames until INTERPRETER_EXIT or an unhandled error leaves
 * it.  It keeps an external name so that debuggers and profilers
 * (Tools/gdb/libpython.py) can find its frames. */
PyValue _PyEval_EvalFrameLoop(PyThreadState *tstate,
                              _PyInterpreterFrame *frame, int throwflag);

Py_NO_INLINE PyValue _Py_HOT_FUNCTION DONT_SLP_VECTORIZE
_PyEval_EvalFrameLoop(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    check_invalid_reentrancy();
    CALL_STAT_INC(pyeval_calls);

    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
        _PyEval_FrameClearAndPop(tstate, frame);
        return PyValue_NULL;
    }

    _PyBorrowStack *borrow = &((_PyThreadStateImpl *)tstate)->borrow;
    PyObject **outer_floor = borrow->eval_floor;
    /* Entries below this activation's floor can only have requested a
       release before it started, so the only request of theirs that its
       instruction boundaries can consume is one pending now. */
    int outer_request = _Py_eval_breaker_bit_is_set(tstate,
                                                    _PY_BORROW_RELEASE_BIT);
    PyObject **floor = borrow->top;
    borrow->eval_floor = floor;
    int outer_in_call = borrow->in_call;
    borrow->in_call = 0;
    PyValue result;

    /* The activation's record, linked from the thread state until it
     * returns.  It lies on this C stack between the stack pointer and the
     * record of the activation that this one runs inside. */
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    _PyEvalActivation activation;
    activation.previous = tstate_impl->eval_activation;
    activation.source_error_frame = NULL;
    activation.source_error_handler = (_PyExceptionHandler){0};
    activation.observer_context = NULL;
    activation.jit_pin = NULL;
    activation.jit_unit = NULL;
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
    /* Returns and yields resume at the trampoline's Return word. */
    entry->frame.instr_ptr = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry->frame.ir_words = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry->frame.owner = FRAME_OWNED_BY_INTERPRETER;
    entry->frame.visited = 0;
    /* What the frame returns or yields lands in entry->stack[0] */
    entry->frame.return_slot = 1;
    entry->frame.return_pc = (_PyInstr *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS;
    entry->frame.pending_kwargs = IR_NO_DST;
    entry->frame.pending_redirect = 0;
    entry->frame.jit_flags = 0;
#ifdef Py_DEBUG
    entry->frame.lltrace = 0;
#endif
    /* Push frame */
    entry->frame.previous = tstate->current_frame;
    frame->previous = &entry->frame;
    tstate->current_frame = frame;

    if (throwflag) {
        /* support for generator.throw() */
        if (_Py_EnterRecursivePy(tstate)) {
            goto early_exit;
        }
        /* Because this avoids the RESUME, we need to update instrumentation */
        _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
    }
    result = tstate->interp->ceval.ir_forms
        ? ir_execute_forms(tstate, frame, throwflag)
        : ir_evaluate(tstate, frame, throwflag);
    /* The handler that returned, Return.External or exit_unwind_notrace,
     * left the entry frame */
    assert(tstate_impl->eval_activation == &activation);
    tstate_impl->eval_activation = activation.previous;
    goto leave;

early_exit:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame = dying->previous;
    _PyEval_FrameClearAndPop(tstate, dying);
    assert(frame == &entry->frame);
    /* Restore previous frame and exit */
    tstate->current_frame = frame->previous;
    tstate_impl->eval_activation = activation.previous;
    result = PyValue_NULL;

leave:
    /* The pin of the last departure that compiled code or a function form
     * made in this activation, now that nothing of it runs (the backstop of
     * -X ir_forms, which has no jit_run()).  A callback that its release
     * runs runs in a loop of its own. */
    Py_CLEAR(activation.jit_pin);
    assert(borrow->eval_floor == floor);
    _PyBorrowScope_Exit(tstate, floor);
    borrow->eval_floor = outer_floor;
    borrow->in_call = outer_in_call;
    if (outer_request && outer_floor != NULL && borrow->top != outer_floor) {
        /* Renew the request that this activation may have consumed.  Entries
           that requested nothing stay unrequested: a live argument vector
           sits above the outer floor during every callback from a builtin,
           and its own scope releases it. */
        _PyBorrow_RequestRelease(tstate);
    }
    return result;
}


#ifdef DO_NOT_OPTIMIZE_INTERP_LOOP
#  pragma optimize("", on)
#endif

/* The evaluation loop for callers that take the value of the frame as it is
 * (the value vectorcall of a Python function, generators).  A PEP 523 hook
 * returns an object, which it steals back. */
static inline Py_ALWAYS_INLINE PyValue
eval_frame_value(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 int throwflag)
{
    if (tstate->interp->eval_frame == NULL) {
        EVAL_CALL_STAT_INC(EVAL_CALL_TOTAL);
        return _PyEval_EvalFrameLoop(tstate, frame, throwflag);
    }
    return _PyValue_FromObjectResult(_PyEval_EvalFrame(tstate, frame, throwflag));
}

PyValue
_PyEval_EvalFrameValue(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       int throwflag)
{
    return eval_frame_value(tstate, frame, throwflag);
}

/* The exported evaluation loop and the PEP 523 default: an object result,
 * which boxes an inline value. */
PyObject *
_PyEval_EvalFrameDefault(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    PyValue result = _PyEval_EvalFrameLoop(tstate, frame, throwflag);
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
                if(_PyList_AppendValue((PyListObject *)posonly_names, PyValue_FromObjectBorrow(kwname)) != 0) {
                    goto fail;
                }
                posonly_conflicts++;
                continue;
            }

            int cmp = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(posonly_name), PyValue_FromObjectBorrow(kwname), Py_EQ);

            if ( cmp > 0) {
                if(_PyList_AppendValue((PyListObject *)posonly_names, PyValue_FromObjectBorrow(kwname)) != 0) {
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
        localsplus[i] = PyValue_FromObjectSteal(kwdict);
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
        localsplus[total_args] = PyValue_FromObjectSteal(u);
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

            int err = _PyDict_SetValue((PyDictObject *)kwdict, PyValue_FromObjectBorrow(keyword),
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
            localsplus[j] = value_value;
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
                    localsplus[m+i] = PyValue_XDUP(defs[i]);
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
                /* A stored key's __eq__ can replace __kwdefaults__; the
                 * lookup borrows its dictionary, so pin it here. */
                PyObject *kwdefs = Py_NewRef(func->func_kwdefaults);
                PyValue def;
                /* Public entry: PyEval_EvalCodeEx() stores its kwdefs
                 * argument unchecked, and a non-dict raises SystemError. */
                int found = PyDict_GetValue(kwdefs,
                                            PyValue_FromObjectBorrow(varname),
                                            &def);
                Py_DECREF(kwdefs);
                if (found < 0) {
                    goto fail_post_args;
                }
                if (found > 0) {
                    localsplus[i] = def;
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

/* Retire a thread frame as upstream does: it is unlinked, the caller holds
 * the result in C, and each owner is NULLed and closed in turn. A finalizer
 * that runs sees a consistent frame; nested calls may grow the arena but
 * must restore this reservation, which stays until the code is closed. */
static inline PyValue
retire_thread_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue result, int extent)
{
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    assert((PyObject **)frame == tstate->datastack_limit);
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
    _PyFrame_ClearExceptCode(frame, extent);
    Py_CLEAR(frame->f_executable);
#ifndef NDEBUG
    assert(tstate->datastack_chunk == chunk);
    assert(tstate->datastack_top == top);
    assert(tstate->datastack_limit == limit);
#endif
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}

static PyValue
clear_thread_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyValue result)
{
    return retire_thread_frame(tstate, frame, result,
                               (int)_PyFrame_Exec(frame)->nfixed);
}

static PyValue
clear_gen_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                PyValue result)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    frame->previous = NULL;
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
    _PyFrame_ClearGenerator(gen, _PyFrame_NumSlots(frame));
    // No escaping call may follow this store before evaluator return.
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
    return result;
}

static void
clear_error_frame(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        (void)clear_thread_frame(tstate, frame, PyValue_NULL);
        return;
    }
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
    FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_CLEARED);
    assert(tstate->exc_info == &gen->gi_exc_state);
    tstate->exc_info = gen->gi_exc_state.previous_item;
    gen->gi_exc_state.previous_item = NULL;
    frame->previous = NULL;
    _PyFrame_ClearGenerator(gen, (int)_PyFrame_Exec(frame)->nfixed);
    ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_RETURN;
}

static inline Py_ALWAYS_INLINE _PyIRStepResult
complete_step_impl(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, int base, int destination,
                   _PyInstr *exit, PyValue value, enum _PyIRStepKind kind)
{
    assert(_PyValue_IsHome(value));
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
    window[0] = window[1] = PyValue_NULL;
    if (kind != _PY_IR_FOR_ITER) {
        window[2] = PyValue_NULL;
    }
    if (kind == _PY_IR_SEND_THROW) {
        window[3] = PyValue_NULL;
    }
    PyValue displaced = PyValue_NULL;
    if (kind != _PY_IR_FOR_ITER) {
        assert(destination < _PyFrame_NumSlots(frame));
        displaced = frame->base[destination];
        assert(destination < (int)_PyFrame_Exec(frame)->nfixed ||
               destination == _PyFrame_DiscardSlot(frame) ||
               PyValue_IsNull(displaced));
        frame->base[destination] = value;
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
_PyEval_CompleteForIter(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *site, int base, _PyInstr *exit, PyValue value)
{
    return complete_step_impl(tstate, frame, site, base, IR_NO_DST, exit,
                              value, _PY_IR_FOR_ITER);
}

_PyIRStepResult
_PyEval_CompleteSend(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *site, int base, int destination,
                     _PyInstr *exit, PyValue value)
{
    return complete_step_impl(tstate, frame, site, base, destination, exit,
                              value, _PY_IR_SEND);
}

_PyIRStepResult
_PyEval_CompleteSendThrow(PyThreadState *tstate, _PyInterpreterFrame *frame,
                          _PyInstr *site, int base, int destination,
                          _PyInstr *exit, PyValue value)
{
    return complete_step_impl(tstate, frame, site, base, destination, exit,
                              value, _PY_IR_SEND_THROW);
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
    assert(offset >= 0 && offset < _PyCode_IRSize(code));
    /* SEND/FOR completion uses the immutable A/B/signed-D layout, including
     * when the executing site has a transparent instrumentation wrapper. */
    _PyIRWord word = _PyCode_IRWords(code)[offset];
    int opcode = _PyIR_HandlerBase[IR_HANDLER(word)];
    assert(opcode == SEND || opcode == FOR_ITER);
    _PyInstr *exit = frame->return_pc + IR_DS(word);
    if (opcode == SEND) {
        return complete_step_impl(tstate, frame, site, IR_B(word), IR_A(word),
                                  exit, value, _PY_IR_SEND);
    }
    return complete_step_impl(tstate, frame, site, IR_B(word), IR_A(word),
                              exit, value, _PY_IR_FOR_ITER);
}

static PyValue
frame_clear_and_pop(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue result)
{
    // By this point, tstate->current_frame is already set to the parent frame.
    _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
    if (frame->owner == FRAME_OWNED_BY_THREAD) {
        return clear_thread_frame(tstate, frame, result);
    }
    return clear_gen_frame(tstate, frame, result);
}

/* Retire the unlinked frame of a return; the caller holds the result
 * (_PyEval_FrameClearAndPopGeneralWithResult) */
static inline void
_PyRop_RetireFrame(PyThreadState *tstate, _PyInterpreterFrame *dying)
{
    if (dying->owner != FRAME_OWNED_BY_THREAD) {
        (void)frame_clear_and_pop(tstate, dying, PyValue_NULL);
        return;
    }
    _PyThreadState_UpdateLastProfiledFrame(tstate, dying, tstate->current_frame);
    (void)retire_thread_frame(tstate, dying, PyValue_NULL, _PyFrame_NumSlots(dying));
}

/* The same for a frame whose scratch is vacant, so that only its locals can
 * own values (_PyEval_RetireThreadFrameWithResult) */
static inline void
_PyRop_RetireFrameEmpty(PyThreadState *tstate, _PyInterpreterFrame *dying)
{
    assert(dying->owner == FRAME_OWNED_BY_THREAD);
    assert(tstate->current_frame != dying);
    assert((PyObject **)dying == tstate->datastack_limit);
    if (Py_UNLIKELY(dying->frame_obj != NULL
                    || tstate->last_profiled_frame == dying)) {
        _PyRop_RetireFrame(tstate, dying);
        return;
    }
    _PyFrame_ClearSlots(dying, (int)_PyFrame_Exec(dying)->nfixed);
    assert(dying->f_funcobj != NULL);
    Py_DECREF(dying->f_funcobj);
    Py_DECREF(dying->f_executable);
    _PyThreadState_PopFrame(tstate, dying);
}

PyValue
_PyEval_FrameClearAndPopWithResult(PyThreadState *tstate,
                                  _PyInterpreterFrame *frame, PyValue result)
{
    assert(!PyValue_IsNull(result));
    return frame_clear_and_pop(tstate, frame, result);
}

Py_NO_INLINE PyValue
_PyEval_FrameClearAndPopGeneralWithResult(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame,
                                         PyValue result)
{
    assert(!PyValue_IsNull(result));
    if (frame->owner != FRAME_OWNED_BY_THREAD) {
        return frame_clear_and_pop(tstate, frame, result);
    }
    _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
    return retire_thread_frame(tstate, frame, result, _PyFrame_NumSlots(frame));
}

void
_PyEval_FrameClearAndPop(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    (void)frame_clear_and_pop(tstate, frame, PyValue_NULL);
}

/* Whether a call of the function creates an object (a generator, coroutine or
 * async generator) instead of running its code in a frame. */
static inline int
code_creates_generator(PyCodeObject *code)
{
    return (code->co_flags &
            (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) != 0;
}

/* What the binding of a call makes: the frame of a function, which the
 * evaluator runs, or the object that a call of a generator function creates
 * (a generator, coroutine or async generator, whose frame no thread runs).
 * Neither is an error, with the exception set. */
typedef struct {
    _PyInterpreterFrame *frame;
    PyGenObject *gen;
} _PyBound;

static void
bound_close_arguments(PyObject *locals, PyValue *args, size_t argcount,
                      PyObject *kwnames)
{
    Py_XDECREF(locals);
    size_t total = argcount +
        (kwnames == NULL ? 0 : (size_t)PyTuple_GET_SIZE(kwnames));
    for (size_t i = 0; i < total; i++) {
        PyValue_CLOSE(args[i]);
    }
}

/* The generator of a call of the generator function func with the given
 * arguments: the object is created first, the arguments are bound into its
 * frame and then its prologue runs.  Consumes func, locals and the arguments
 * on both outcomes. */
static PyGenObject *
eval_new_generator(PyThreadState *tstate, PyValue func, PyObject *locals,
                   PyValue *args, size_t argcount, PyObject *kwnames,
                   PyCodeObject *code)
{
    PyFunctionObject *func_obj =
        (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    CALL_STAT_INC(frames_pushed);
    PyGenObject *gen = _PyGen_NewCall(tstate, func, locals, code,
                                      tstate->current_frame);
    if (gen == NULL) {
        bound_close_arguments(NULL, args, argcount, kwnames);
        return NULL;
    }
    if (initialize_locals(tstate, func_obj, code, gen->gi_iframe.base, args,
                          argcount, kwnames))
    {
        _PyGen_DiscardCall(gen);
        return NULL;
    }
    if (gen_prologue(tstate, gen, code) < 0) {
        _PyGen_DiscardCall(gen);
        return NULL;
    }
    return gen;
}

/* The binding of every call of a Python function that has no plan of its
 * own: code is independently pinned through preparation and binding.
 * Consumes func, locals and args, but never reselects the function's mutable
 * code.  A generator function's call creates its generator here, the one
 * place that does for the calls that bind generally. */
static _PyBound
eval_frame_push_and_init_code(PyThreadState *tstate, PyValue func,
                        PyObject *locals, PyValue *args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous, PyCodeObject *code)
{
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(func);
    CALL_STAT_INC(frames_pushed);
    if (_PyCode_GetExec(code) == NULL) {
        goto fail;
    }
    if (code_creates_generator(code)) {
        return (_PyBound){NULL, eval_new_generator(
            tstate, func, locals, args, argcount, kwnames, code)};
    }
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
    if (frame == NULL) {
        PyErr_NoMemory();
        goto fail;
    }
    _PyFrame_Initialize(tstate, frame, func, locals, code, frame->base, previous);
    _PyFrame_CopyFreeVars(frame, code);
    if (initialize_locals(tstate, func_obj, code, frame->base, args, argcount, kwnames)) {
        assert(frame->owner == FRAME_OWNED_BY_THREAD);
        _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
        (void)clear_thread_frame(tstate, frame, PyValue_NULL);
        return (_PyBound){NULL, NULL};
    }
    return (_PyBound){frame, NULL};
fail:
    /* Consume the references */
    PyValue_CLOSE(func);
    bound_close_arguments(locals, args, argcount, kwnames);
    return (_PyBound){NULL, NULL};
}

/* Same as eval_frame_push_and_init_code but takes an args tuple and kwargs dict.
   Steals references to func, callargs and kwargs.
*/
static _PyBound
eval_frame_push_and_init_ex_code(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs,
    _PyInterpreterFrame *previous, PyCodeObject *code)
{
    bool has_dict = (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0);
    PyObject *kwnames = NULL;
    PyValue *newargs = NULL;
    PyObject *const *object_array = NULL;
    PyValue stack_array[8] = {0};
    Py_ssize_t total = nargs;
    _PyBound bound = {NULL, NULL};
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
    bound = eval_frame_push_and_init_code(
        tstate, func, locals,
        newargs, nargs, kwnames, previous, code
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
    return bound;
}

/* Retire the shim when binding the __init__ frame failed. Its self owner
 * closes with the raised exception set aside. */
static void
clear_init_shim(PyThreadState *tstate, _PyInterpreterFrame *shim)
{
    PyObject *error = _PyErr_GetRaisedException(tstate);
    _PyFrame_ClearTemporaries(shim, 0);
    _PyEval_FrameClearAndPop(tstate, shim);
    /* Deallocators preserve the raised-exception state they entered with.
     * Thus restoring the saved error below cannot close a new exception. */
    assert(!_PyErr_Occurred(tstate));
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
    shim->base[0] = PyValue_DUP(window[1]);

    PyValue small[8];
    int argcount = nargs + 1;
    PyValue *arguments = small;
    if (argcount > (int)Py_ARRAY_LENGTH(small)) {
        arguments = PyMem_Malloc(sizeof(PyValue) * argcount);
        if (arguments == NULL) {
            PyErr_NoMemory();
            clear_init_shim(tstate, shim);
            /* No input moved: the caller's error unwind still owns them. */
            return NULL;
        }
    }
    PyValue function = window[0];
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(function);
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(func->func_code);
    window[0] = PyValue_NULL;
    for (int i = 0; i < argcount; i++) {
        arguments[i] = window[i + 1];
        window[i + 1] = PyValue_NULL;
    }
    _PyBound bound = eval_frame_push_and_init_code(
        tstate, function, NULL, arguments, argcount, NULL, shim, code);
    /* The binder consumed every argument on both outcomes. */
    if (arguments != small) {
        PyMem_Free(arguments);
    }
    Py_DECREF(code);
    _PyInterpreterFrame *child = bound.frame;
    if (bound.gen != NULL) {
        /* A generator function as __init__ returns its object, and the
         * shim's check of the result would reject it. */
        PyTypeObject *result_type = Py_TYPE(bound.gen);
        Py_DECREF(bound.gen);
        PyErr_Format(PyExc_TypeError,
                     "__init__() should return None, not '%.200s'",
                     result_type->tp_name);
        clear_init_shim(tstate, shim);
        return NULL;
    }
    if (child == NULL) {
        clear_init_shim(tstate, shim);
        return NULL;
    }
    caller->return_slot = (uint16_t)destination;
    caller->return_pc = next;
    tstate->py_recursion_remaining--;
    return child;
}

/* Reuse the ordinary typed initializer and exact positional mover. All
 * callback-capable binding stays in the outlined continuation above. */
static Py_NO_INLINE _PyInterpreterFrame *
create_init_frame(PyThreadState *tstate, _PyInterpreterFrame *caller,
                  PyValue *window, int nargs, int destination,
                  _PyInstr *next)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(window[0]);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    assert(func->vectorcall == _PyFunction_Vectorcall);
    if (!(code->co_flags & CO_OPTIMIZED) || code_creates_generator(code) ||
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
    shim->base[0] = PyValue_DUP(window[1]);
    PyValue function = window[0];
    window[0] = PyValue_NULL;
    _PyInterpreterFrame *child = _PyFrame_PushUnchecked(tstate, function, shim);
    _PyFrame_MoveArgs(child, window + 1, nargs + 1);
    caller->return_slot = (uint16_t)destination;
    caller->return_pc = next;
    tstate->py_recursion_remaining--;
    return child;
}

/* The frame of a class's __init__ that the operands do not fill directly
 * (a defaulted or variadic __init__, or too little stack): the operands are
 * gathered with init and self into a window that create_init_frame binds
 * from. */
static Py_NO_INLINE _PyInterpreterFrame *
create_init_frame_window(PyThreadState *tstate, _PyInterpreterFrame *caller,
                         _PyInstr *current, PyValue init, PyValue self,
                         _PyInstr *next, _PyIRWord word, int first, int nargs)
{
    PyValue small[8];
    PyValue *window = small;
    if (nargs + 2 > (int)Py_ARRAY_LENGTH(small)) {
        window = PyMem_Malloc(sizeof(PyValue) * (nargs + 2));
        if (window == NULL) {
            PyErr_NoMemory();
            PyValue_CLOSE(init);
            PyValue_CLOSE(self);
            return NULL;
        }
    }
    window[0] = init;
    window[1] = self;
    for (int k = 0; k < nargs; k++) {
        window[2 + k] = call_operand_take(caller, current, first + k);
    }
    _PyInterpreterFrame *child = create_init_frame(
        tstate, caller, window, nargs, (int)IR_A(word), next);
    if (child != NULL) {
        call_release_callable(caller, word);
    }
    else {
        for (int k = nargs + 2; k-- > 0;) {
            PyValue_CLEAR(window[k]);
        }
    }
    if (window != small) {
        PyMem_Free(window);
    }
    return child;
}

/* The frame of a class's __init__ for CALL_ALLOC_AND_ENTER_INIT, called with
 * the call at current, whose callable operand is the class: the guards
 * (_CHECK_OBJECT, _CHECK_INIT_FUNCTION) checked the class and its cached
 * __init__, which the helper counts, and it allocates the instance.  The
 * operands fill the arguments after the instance: a temporary moves, a local
 * is counted, and the class operand is released.  On failure it has closed
 * what it took and the error is set; the caller's unwind clears the homes
 * that remain, the class among them. */
_PyInterpreterFrame *
_PyEval_CreateInitFrame(PyThreadState *tstate, _PyInterpreterFrame *caller,
                        _PyInstr *current, PyValue callable, _PyInstr *next)
{
    PyTypeObject *tp = (PyTypeObject *)PyValue_AsKnownObjectBorrow(callable);
    PyFunctionObject *init_func = (PyFunctionObject *)FT_ATOMIC_LOAD_PTR_ACQUIRE(
        ((PyHeapTypeObject *)tp)->_spec_cache.init);
    // Counted first: the allocation may collect, and a finalizer may
    // replace the class's __init__
    PyValue init = PyValue_DUP(PyValue_FromObjectBorrow(init_func));
    PyObject *self_o = PyType_GenericAlloc(tp, 0);
    if (self_o == NULL) {
        PyValue_CLOSE(init);
        return NULL;
    }
    PyValue self = PyValue_FromObjectSteal(self_o);
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*current);
    /* The class call has no self: its operand list starts with the slot of a
     * self that was NULL at run time, which is not an argument. */
    int first = (word & IR_F_NO_SELF) ? 0 : 1;
    int nargs = (int)IR_C(word) - first;
    PyCodeObject *init_code = (PyCodeObject *)init_func->func_code;
    if ((init_code->co_flags & CO_OPTIMIZED) &&
        !code_creates_generator(init_code) &&
        !(init_code->co_flags & (CO_VARARGS | CO_VARKEYWORDS)) &&
        !init_code->co_kwonlyargcount && init_code->co_argcount == nargs + 1 &&
        init_code->_co_exec != NULL &&
        _PyThreadState_HasStackSpace(tstate,
            init_code->co_framesize + _Py_InitCleanup.co_framesize))
    {
        /* The arguments fill the child straight from the operands: the
         * receiver is its first, a temporary moves and a local is counted. */
        PyValue *slots = _PyFrame_Slots(caller);
        _PyInterpreterFrame *shim = _PyFrame_PushTrampolineUnchecked(
            tstate, (PyCodeObject *)&_Py_InitCleanup, 1, caller);
        shim->base[0] = PyValue_DUP(self);
        _PyInterpreterFrame *child = _PyFrame_PushUnchecked(tstate, init, shim);
        child->base[0] = self;
        call_take_operands(slots, current, first, nargs, child->base + 1);
        call_release_callable(caller, word);
        caller->return_slot = (uint16_t)IR_A(word);
        caller->return_pc = next;
        tstate->py_recursion_remaining--;
        return child;
    }
    return create_init_frame_window(tstate, caller, current, init, self, next,
                                    word, first, nargs);
}

/* Generic calls.
 *
 * The generic CALL, CALL_KW and CALL_FUNCTION_EX, their instrumented forms
 * and the guarded calls that missed their binding plan run here, on the
 * operand helpers that the specialized handlers use.  A call reads its
 * operands once, in place: the callable is field B of the head and operand k
 * is the slot that the k-th field of the operand words names.  The values
 * the call holds across a step that can run Python are explicit: a view of a
 * bound method's function and receiver, which the method operand keeps alive,
 * the tuple of a CALL_FUNCTION_EX, which the call owns, and the references
 * that a Python callee takes.  Everything else is read from the homes again,
 * and a temporary cannot change while the call runs: Python cannot see it,
 * and a local's old value survives a write through its frame's proxy.  After
 * a step that can run Python the frame is checked and its instr_ptr is the
 * call's again (call_refresh).
 *
 * A Python callee takes its arguments from the operands (a temporary moves,
 * a local is counted) after every test, and the child is current before the
 * first release, which is the method operand's.  A native callee borrows
 * them, and the result is published first (call_publish).  An error
 * leaves the operands that the call did not move in their homes, which the
 * unwinder clears. */

/* The operand of the call at head that is its first argument: the self
 * operand's slot is NULL at run time for a class attribute that is not a
 * method, and the arguments follow it. */
static inline Py_ALWAYS_INLINE int
call_skipped_operands(const PyValue *slots, const _PyIRWord *head,
                      _PyIRWord word)
{
    return (word & IR_F_NO_SELF) == 0 &&
        PyValue_IsNull(slots[_PyIR_CallOperand(head, 0)]);
}

/* The callable of a call and its receiver.  A bound method without a self
 * operand is its function and its receiver, which go before the arguments:
 * views of the method, which its operand keeps alive until the call is
 * published or has pushed its frame, and which the call counts if it keeps
 * them.  Otherwise the receiver is NULL. */
static inline Py_ALWAYS_INLINE PyValue
call_expand_method(PyValue callable, int flags, PyValue *receiver)
{
    *receiver = PyValue_NULL;
    if ((flags & CALL_SELF_PRESENT) ||
        PyValue_TYPE(callable) != &PyMethod_Type) {
        return callable;
    }
    PyMethodObject *method = (PyMethodObject *)PyValue_AsKnownObjectBorrow(callable);
    *receiver = method->im_self;
    return _PyIR_AdoptTemp(PyValue_FromObjectBorrow(method->im_func));
}

/* The first argument that a CALL event reports, as a view: the receiver or
 * the self operand, else the first argument.  NULL if the call has none. */
static inline PyValue
call_first_argument(const PyValue *slots, const _PyIRWord *head,
                    PyValue receiver, int keywords)
{
    if (!PyValue_IsNull(receiver)) {
        return receiver;
    }
    _PyIRWord word = *head;
    int skip = call_skipped_operands(slots, head, word);
    if ((int)IR_C(word) - keywords <= skip) {
        return PyValue_NULL;
    }
    return slots[_PyIR_CallOperand(head, skip)];
}

/* The CALL event, before the call runs.  The callback can run Python. */
static _Py_COLD_FUNCTION int
call_monitor(PyThreadState *tstate, _PyInterpreterFrame *frame,
             _PyInstr *head, PyValue callable, PyValue first)
{
    int status = _Py_call_instrumentation_2args(
        tstate, PY_MONITORING_EVENT_CALL, frame, head, callable,
        PyValue_IsNull(first) ? borrowed_word(&_PyInstrumentation_MISSING)
                              : first);
    if (call_refresh(tstate, frame, head) < 0) {
        return -1;
    }
    return status < 0 ? -1 : 0;
}

/* The C_RETURN or C_RAISE event of a call that ran native code.  failed: the
 * call raised.  Returns whether it failed, which a callback that raises in
 * C_RETURN makes true. */
static _Py_COLD_FUNCTION int
call_monitor_result(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    _PyInstr *head, PyValue callable, PyValue first,
                    int failed)
{
    PyValue argument = PyValue_IsNull(first)
        ? borrowed_word(&_PyInstrumentation_MISSING) : first;
    if (failed) {
        _Py_call_instrumentation_exc2(
            tstate, PY_MONITORING_EVENT_C_RAISE, frame, head, callable,
            argument);
        return 1;
    }
    return _Py_call_instrumentation_2args(
        tstate, PY_MONITORING_EVENT_C_RETURN, frame, head, callable,
        argument) < 0;
}

/* Whether callable is a plain Python function that a call may push a frame
 * for: PEP 523 is not hooked and it runs through _PyFunction_Vectorcall. */
static inline Py_ALWAYS_INLINE int
call_is_python_function(PyThreadState *tstate, PyValue callable)
{
    if (!PyValue_IsObject(callable) || IS_PEP523_HOOKED(tstate)) {
        return 0;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(callable);
    return Py_TYPE(object) == &PyFunction_Type &&
        ((PyFunctionObject *)object)->vectorcall == _PyFunction_Vectorcall;
}

/* The child frame is complete and the caller's operands are in it: make it
 * the call's callee.  Nothing may fail or run Python between the first
 * write and the last. */
static inline Py_ALWAYS_INLINE void
call_commit(PyThreadState *tstate, _PyInterpreterFrame *frame,
            _PyInstr *head, _PyInterpreterFrame *child)
{
    assert(child->previous == frame);
    assert(child->ir_words == _PyFrame_GetCode(child)->_co_exec->words);
    frame->return_slot = (uint16_t)IR_A(*head);
    frame->return_pc = call_return_pc(head);
    tstate->current_frame = child;
}

/* The most arguments that fill a *args tuple, and the most keyword-only
 * parameters, that the binding below handles: it holds their values in
 * arrays. */
#define CALL_BIND_EXTRAS_MAX _Py_CALL_BIND_EXTRAS_MAX
#define CALL_BIND_KWONLY_MAX _Py_CALL_BIND_SLOTS_MAX

/* Release what the binding below prepared before it pushed anything: the
 * __kwdefaults__ values that kwonly_mask names, and the *args tuple. */
static Py_NO_INLINE void
call_bind_discard(PyValue *kwdefaults, unsigned int kwonly_mask,
                  PyObject *extras, PyObject *kwargs)
{
    for (unsigned int bits = kwonly_mask; bits; bits &= bits - 1) {
        PyValue_XCLOSE(kwdefaults[__builtin_ctz(bits)]);
    }
    Py_XDECREF(extras);
    Py_XDECREF(kwargs);
}

/* The keyword-only parameters that no keyword fills (kwonly_mask) take their
 * __kwdefaults__ entry, each looked up as an owner in kwdefaults.  The
 * dictionary is mutable: it is read here, as long as a lookup cannot run
 * Python (an exact dict whose keys are all str).  Returns 0, with nothing
 * held, for anything else, and for a missing entry, which the general binder
 * reports.  The rare shapes of the binding live in the noinline functions
 * here, so that the common call carries none of their state. */
static Py_NO_INLINE int
call_bind_kwdefaults(PyFunctionObject *func, PyCodeObject *code,
                     unsigned int kwonly_mask, PyValue *kwdefaults)
{
    PyObject *kwdefs = func->func_kwdefaults;
    if (kwdefs == NULL || !PyDict_CheckExact(kwdefs) ||
        !DK_IS_UNICODE(((PyDictObject *)kwdefs)->ma_keys)) {
        return 0;
    }
    int argcount = code->co_argcount;
    for (unsigned int bits = kwonly_mask; bits; bits &= bits - 1) {
        int j = __builtin_ctz(bits);
        /* Parameter names are interned, and their hash is cached. */
        PyObject *name = _PyTuple_GET_OBJECT(code->co_localsplusnames,
                                             argcount + j);
        if (PyUnstable_Unicode_GET_CACHED_HASH(name) == -1 ||
            !_PyDict_GetUnicodeValue((PyDictObject *)kwdefs, name,
                                     &kwdefaults[j])) {
            call_bind_discard(kwdefaults, bits ^ kwonly_mask, NULL, NULL);
            return 0;
        }
    }
    return 1;
}

/* The *args tuple of the extra positional arguments (filled is how many
 * the named parameters take).  The operands stay in their homes; it has
 * counts of its own.  NULL with the error set on failure. */
static Py_NO_INLINE PyObject *
call_bind_extras(PyValue *slots, _PyInstr *head, PyObject *tuple, int skip,
                 int filled, int lead, int extras)
{
    PyValue gathered[CALL_BIND_EXTRAS_MAX];
    for (int i = 0; i < extras; i++) {
        gathered[i] = tuple != NULL
            ? _PyTuple_ITEMS(tuple)[filled + i - lead]
            : slots[_PyIR_CallOperand(head, skip + filled + i - lead)];
    }
    return _PyTuple_FromValueArray(gathered, extras);
}

/* The **kwargs dictionary of the keywords that the plan routes to it
 * (destinations is NULL for a call without keywords); the dictionary takes
 * counts of its own, and the operands stay in their homes.  NULL with the
 * error set on failure. */
static Py_NO_INLINE PyObject *
call_bind_kwargs(_PyInterpreterFrame *frame, PyValue *slots, _PyInstr *head,
                 _PyIRWord word, int skip, int lead, int positional,
                 int kwcount, const uint16_t *destinations)
{
    PyValue keys[_Py_CALL_BIND_KEYWORD_CAPACITY];
    PyValue values[_Py_CALL_BIND_KEYWORD_CAPACITY];
    int count = 0;
    if (kwcount > 0) {
        PyObject *names = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(
            _PyFrame_GetCode(frame)->co_consts,
            _PyIR_CallOperand(head, (int)IR_C(word) - 1)));
        for (int i = 0; i < kwcount; i++) {
            if (destinations[i] == _Py_CALL_BIND_TO_KWARGS) {
                keys[count] = _PyTuple_ITEMS(names)[i];
                values[count] = slots[_PyIR_CallOperand(
                    head, skip + positional + i - lead)];
                count++;
            }
        }
    }
    return _PyDict_FromValueItems(keys, 1, values, 1, count);
}

/* Move the operands that the child's named parameters do not take: those of
 * the *args tuple (the tuple has counts of its own, so they are released)
 * and the keywords, to their parameters or, for **kwargs, released. */
static Py_NO_INLINE void
call_bind_take_rest(PyValue *slots, _PyInstr *head, int skip, int lead,
                    int filled, int extras, int positional, int kwcount,
                    const uint16_t *destinations, PyValue *base)
{
    PyValue taken[CALL_BIND_EXTRAS_MAX];
    call_take_operands(slots, head, skip + filled - lead, extras, taken);
    for (int i = 0; i < extras; i++) {
        PyValue_CLOSE(taken[i]);
    }
    for (int i = 0; i < kwcount; i++) {
        PyValue value;
        int slot = destinations[i];
        call_take_operands(slots, head, skip + positional - lead + i, 1,
                           slot == _Py_CALL_BIND_TO_KWARGS ? &value : &base[slot]);
        if (slot == _Py_CALL_BIND_TO_KWARGS) {
            PyValue_CLOSE(value);
        }
    }
}

/* The items of the **kwargs dict of a CALL_FUNCTION_EX, bound by name:
 * dests[i] is the slot that item i fills (a positional-or-keyword or
 * keyword-only parameter), or _Py_CALL_BIND_TO_KWARGS for a name that no
 * parameter has, which the callee's **kwargs takes.  The items are views of
 * the dict, which the call's operand keeps alive and unchanged until the
 * frame is complete. */
#define CALL_BIND_DICT_MAX 16

typedef struct {
    int count;
    uint16_t dests[CALL_BIND_DICT_MAX];
    PyValueBorrow keys[CALL_BIND_DICT_MAX];
    PyValueBorrow values[CALL_BIND_DICT_MAX];
} _CallDict;

/* Decide where each item of dict goes, for a call of code whose first
 * filled parameters positional arguments fill.  Returns 0, having changed
 * nothing, when the generic binder must take the call: a dict that is not
 * exact, too many items, a key that is not an exact str (comparing it
 * could run Python), a name that two arguments give, and a name that no
 * parameter has when there is no **kwargs: it reports each error with its
 * message.  Parameter names are interned, so most keys match by identity;
 * the rest are compared as strings, which runs nothing. */
static Py_NO_INLINE int
call_bind_dict_destinations(PyCodeObject *code, int filled, PyObject *dict,
                            _CallDict *items)
{
    if (!PyDict_CheckExact(dict) ||
        PyDict_GET_SIZE(dict) > CALL_BIND_DICT_MAX) {
        return 0;
    }
    int total = code->co_argcount + code->co_kwonlyargcount;
    int first = code->co_posonlyargcount;
    PyObject *names = code->co_localsplusnames;
    int count = 0;
    Py_ssize_t position = 0;
    PyValueBorrow key, value;
    while (_PyDict_NextBorrowed(dict, &position, &key, &value)) {
        if (!PyValue_IsObject(key)) {
            return 0;
        }
        PyObject *name = PyValue_AsKnownObjectBorrow(key);
        if (!PyUnicode_CheckExact(name)) {
            return 0;
        }
        int slot = first;
        while (slot < total && _PyTuple_GET_OBJECT(names, slot) != name) {
            slot++;
        }
        if (slot == total) {
            for (slot = first; slot < total; slot++) {
                if (_PyUnicode_Equal(_PyTuple_GET_OBJECT(names, slot), name)) {
                    break;
                }
            }
        }
        if (slot == total) {
            if (!(code->co_flags & CO_VARKEYWORDS)) {
                return 0;
            }
            slot = _Py_CALL_BIND_TO_KWARGS;
        }
        else if (slot < filled) {
            return 0;
        }
        items->dests[count] = (uint16_t)slot;
        items->keys[count] = key;
        items->values[count] = value;
        count++;
    }
    items->count = count;
    return 1;
}

/* The **kwargs dictionary of the items that no parameter takes. */
static Py_NO_INLINE PyObject *
call_bind_dict_kwargs(const _CallDict *items)
{
    PyValue keys[CALL_BIND_DICT_MAX];
    PyValue values[CALL_BIND_DICT_MAX];
    int count = 0;
    for (int i = 0; i < items->count; i++) {
        if (items->dests[i] == _Py_CALL_BIND_TO_KWARGS) {
            keys[count] = _PyIR_AdoptTemp(items->keys[i]);
            values[count] = _PyIR_AdoptTemp(items->values[i]);
            count++;
        }
    }
    return _PyDict_FromValueItems(keys, 1, values, 1, count);
}

/* The call is done and its result is the object that it created, a
 * generator that has taken the operands: store it in the destination and
 * release the callable and whatever operands the call still holds. */
static Py_NO_INLINE _PyCallResult
call_publish_object(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    _PyInstr *head, _PyIRWord word, PyGenObject *gen,
                    int operands)
{
    PyValue displaced = call_publish(
        frame, head, word, PyValue_FromObjectSteal((PyObject *)gen), operands);
    PyValue_XCLOSE(displaced);
    /* Releasing the operands can run a finalizer. */
    return (_PyCallResult){call_refresh(tstate, frame, head), NULL};
}

/* Push the frame of the Python function callable, and fill it as the
 * evaluator binds a call, from the positional arguments (the receiver of an
 * expanded bound method, then the operands of the call at head after the
 * self operand that was NULL, or the items of tuple for a CALL_FUNCTION_EX)
 * and, for a CALL_KW, the keywords, which the plan maps to parameters: the
 * call needs no per-call search of names.  The plan (pycore_feedback.h)
 * says the rest: which parameters take defaults, which keyword-only
 * parameters take their __kwdefaults__ entry, and whether *args and
 * **kwargs are empty, filled or pending.  The call reads only what the plan
 * leaves to the function: the length of __defaults__ and the
 * __kwdefaults__ dict.  The caller has shown that callable is a Python
 * function that a frame may be pushed for (PEP 523 is not hooked, the
 * function version or call_is_python_function()).
 *
 * Every test and every allocation precedes the first move, so on a miss
 * (status 1) nothing happened and the operands are as they were: the caller
 * runs the call generically, which binds anything else and reports each
 * error with its message.  A hit leaves the child current and complete.  An
 * allocation failure is an error (status -1) with the operands untouched.
 * receiver, if it is not NULL, is the self of an expanded bound method:
 * callable is its function, and the method operand is released once the
 * child is current.  A tuple call leaves the operands and the tuple to the
 * caller.
 *
 * A call of a generator function (_Py_CALL_BIND_GENERATOR) creates the
 * generator with _PyGen_NewCall, the one creation routine, binds the
 * arguments into its frame and runs gen_prologue; the call's result is the
 * generator, published by call_publish_object as the generic path publishes
 * it, and the status is 0 with no child. */
static inline Py_ALWAYS_INLINE _PyCallResult
call_bind_core(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _PyInstr *head, PyValue callable, PyValue receiver,
               const _PyBindPlan *plan, PyObject *tuple,
               const _CallDict *items, int simple)
{
#ifdef Py_GIL_DISABLED
    return (_PyCallResult){1, NULL};
#else
    int bits = plan->flags;
    if (Py_UNLIKELY(!(bits & _Py_CALL_BIND_VALID))) {
        return (_PyCallResult){1, NULL};
    }
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*head);
    PyValue *slots = _PyFrame_Slots(frame);
    PyFunctionObject *func =
        (PyFunctionObject *)_PyValue_AsKnownObjectBorrowPointer(callable);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int lead = !PyValue_IsNull(receiver);
    int skip = 0;
    if (tuple == NULL) {
        skip = call_skipped_operands(slots, head, word);
        if (Py_UNLIKELY(skip != !!(bits & _Py_CALL_BIND_SKIP))) {
            return (_PyCallResult){1, NULL};
        }
    }
    /* A simple plan (not _Py_CALL_BIND_GENERAL) needs no *args tuple, no
     * **kwargs dict, no keyword-only default and no generator: the binding
     * of the common call is the straight line of this function. */
    int generator = !simple && (bits & _Py_CALL_BIND_GENERATOR) != 0;
    if (!simple && tuple != NULL && generator) {
        /* The caller releases a tuple call's operands once it has a child,
         * which a generator call does not have. */
        return (_PyCallResult){1, NULL};
    }
    if (Py_UNLIKELY(!generator &&
        !_PyThreadState_HasStackSpace(tstate, code->co_framesize))) {
        return (_PyCallResult){1, NULL};
    }
    int argcount = code->co_argcount;
    int kwcount = plan->keywords;
    int positional = plan->positional;
    int filled = plan->filled;
    int extras = simple ? 0 : positional - filled;
    const uint16_t *destinations = items != NULL ? items->dests : plan->keyword_dest;
    /* The positional parameters that no argument and no keyword fills take a
     * default: the function's own, which a function of the same code may
     * have changed, so its length is tested: it covers need slots. */
    PyObject *defaults = func->func_defaults;
    Py_ssize_t ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    int need = plan->need;
    if (Py_UNLIKELY((size_t)(ndefaults - need) > (size_t)(argcount - need))) {
        return (_PyCallResult){1, NULL};
    }
    unsigned int kwonly_mask = simple ? 0 : plan->kwonly_mask;
    PyValue kwdefaults[CALL_BIND_KWONLY_MAX];
    if (!simple && kwonly_mask &&
        !call_bind_kwdefaults(func, code, kwonly_mask, kwdefaults)) {
        return (_PyCallResult){1, NULL};
    }
    /* The *args tuple and the **kwargs dictionary hold counts of their own
     * (the operands stay in their homes until the moves below); creating
     * them can fail and schedules the GC, which does not run Python before
     * the complete child is published. */
    PyObject *extras_tuple = NULL;
    PyObject *kwargs = NULL;
    if (!simple) {
        if (extras > 0) {
            extras_tuple = call_bind_extras(slots, head, tuple, skip, filled,
                                            lead, extras);
            if (extras_tuple == NULL) {
                call_bind_discard(kwdefaults, kwonly_mask, NULL, NULL);
                return (_PyCallResult){-1, NULL};
            }
        }
        if (bits & _Py_CALL_BIND_NEW_KWARGS) {
            kwargs = items != NULL
                ? call_bind_dict_kwargs(items)
                : call_bind_kwargs(frame, slots, head, word, skip, lead,
                                   positional, kwcount, destinations);
            if (kwargs == NULL) {
                call_bind_discard(kwdefaults, kwonly_mask, extras_tuple, NULL);
                return (_PyCallResult){-1, NULL};
            }
        }
    }

    /* All tests precede this transfer. The target is complete before it
     * becomes current: nothing fails, calls back or releases. */
    PyValue function;
    if (tuple != NULL || lead || (word & IR_F_LOCAL_B)) {
        function = PyValue_DUP(callable);
    }
    else {
        function = callable;
        slots[IR_B(word)] = PyValue_NULL;
    }
    _PyInterpreterFrame *target;
    PyGenObject *gen = NULL;
    if (!simple && generator) {
        gen = _PyGen_NewCall(tstate, function, NULL, code, frame);
        if (gen == NULL) {
            call_bind_discard(kwdefaults, kwonly_mask, extras_tuple, kwargs);
            return (_PyCallResult){-1, NULL};
        }
        target = &gen->gi_iframe;
    }
    else {
        target = _PyFrame_PushUnchecked(tstate, function, frame);
    }
    PyValue *base = target->base;
    PyValue *arguments = base;
    if (lead) {
        *arguments++ = PyValue_DUP(receiver);
    }
    if (tuple != NULL) {
        for (int i = 0; i < filled - lead; i++) {
            arguments[i] = PyValue_DUP(_PyTuple_ITEMS(tuple)[i]);
        }
        if (items != NULL) {
            for (int i = 0; i < kwcount; i++) {
                if (destinations[i] != _Py_CALL_BIND_TO_KWARGS) {
                    base[destinations[i]] = PyValue_DUP(items->values[i]);
                }
            }
        }
    }
    else {
        call_take_operands(slots, head, skip, filled - lead, arguments);
        if (kwcount > 0 || extras > 0) {
            call_bind_take_rest(slots, head, skip, lead, filled, extras,
                                positional, kwcount, destinations, base);
        }
    }
    int first_default = argcount - (int)ndefaults;
    for (unsigned int mask = plan->default_mask; mask; mask &= mask - 1) {
        int slot = __builtin_ctz(mask);
        base[slot] = PyValue_DUP(_PyTuple_ITEMS(defaults)[slot - first_default]);
    }
    if (!simple) {
        for (unsigned int mask = kwonly_mask; mask; mask &= mask - 1) {
            int j = __builtin_ctz(mask);
            base[argcount + j] = kwdefaults[j];
        }
    }
    if (!simple) {
        int slot = argcount + code->co_kwonlyargcount;
        if (bits & _Py_CALL_BIND_VARARGS) {
            base[slot++] = extras_tuple != NULL
                ? PyValue_FromObjectSteal(extras_tuple)
                : (PyValue)_PyValue_STATIC_IMMORTAL(&_Py_SINGLETON(tuple_empty));
        }
        if (kwargs != NULL) {
            base[slot] = PyValue_FromObjectSteal(kwargs);
        }
    }
    if (bits & _Py_CALL_BIND_VIRTUAL_KWARGS) {
        /* A simple plan has no *args (which makes it general) and is never
         * new: the slot follows the keyword-only parameters */
        target->pending_kwargs = (uint16_t)(argcount + code->co_kwonlyargcount +
            ((bits & _Py_CALL_BIND_VARARGS) != 0));
    }
    if (kwcount) {
        CALL_STAT_INC(keyword_plan_hits);
    }
    if (!simple && generator) {
        if (gen_prologue(tstate, gen, code) < 0) {
            _PyGen_DiscardCall(gen);
            return (_PyCallResult){-1, NULL};
        }
        return call_publish_object(tstate, frame, head, word, gen,
                                   (int)IR_C(word) - (kwcount != 0));
    }
    call_commit(tstate, frame, head, target);
    if (lead && tuple == NULL) {
        /* The method operand has kept the function and the receiver alive;
         * releasing it can run a weakref callback or a finalizer, with the
         * child current and not started.  A tuple call's caller releases
         * its operands. */
        call_release_callable(frame, word);
    }
    return (_PyCallResult){0, target};
#endif
}

/* The bindings, each decided at compile time: the call's operands (receiver
 * is the self of an expanded bound method, else NULL; a CALL_KW's plan
 * names its keywords), and the items of the tuple of a CALL_FUNCTION_EX.  A
 * simple plan binds in the straight line of a function of its own, which
 * calls nothing. */
static Py_NO_INLINE _PyCallResult
call_bind_general(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  _PyInstr *head, PyValue callable, PyValue receiver,
                  const _PyBindPlan *plan)
{
    return call_bind_core(tstate, frame, head, callable, receiver, plan, NULL, NULL, 0);
}

static Py_NO_INLINE _PyCallResult
call_bind_simple(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *head, PyValue callable, PyValue receiver,
                 const _PyBindPlan *plan)
{
    return call_bind_core(tstate, frame, head, callable, receiver, plan, NULL, NULL, 1);
}

/* Both bindings are tail calls from here: with call_bind_core inlined into
 * this function the compiler gave it a frame before the test, which cost a
 * general plan (a callee with *args or **kwargs) the prologue and epilogue. */
static Py_NO_INLINE _PyCallResult
call_bind_positional(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *head, PyValue callable, PyValue receiver,
                     const _PyBindPlan *plan)
{
    if (plan->flags & _Py_CALL_BIND_GENERAL) {
        return call_bind_general(tstate, frame, head, callable, receiver, plan);
    }
    return call_bind_simple(tstate, frame, head, callable, receiver, plan);
}

static Py_NO_INLINE _PyCallResult
call_bind_tuple(PyThreadState *tstate, _PyInterpreterFrame *frame,
                _PyInstr *head, PyValue callable, PyValue receiver,
                PyObject *tuple, const _CallDict *items,
                const _PyBindPlan *plan)
{
    return call_bind_core(tstate, frame, head, callable, receiver, plan,
                          tuple, items, 0);
}

/* The frame of any Python function call: the operands are gathered into an
 * array that eval_frame_push_and_init_code binds from (defaults, *args,
 * **kwargs, keywords) and consumes on both outcomes, a temporary moving
 * from its home and a local counted.  The names of a CALL_KW are the
 * constant that the last operand indexes.  Returns {0, child}, or {-1, NULL}
 * with the error set and the operands that were not moved in their homes. */
static Py_NO_INLINE _PyCallResult
call_push_python(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *head, PyValue callable, PyValue receiver,
                 int keywords)
{
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*head);
    PyValue *slots = _PyFrame_Slots(frame);
    int skip = call_skipped_operands(slots, head, word);
    int lead = !PyValue_IsNull(receiver);
    int operands = (int)IR_C(word) - keywords;
    int count = lead + operands - skip;
    PyValue small[16];
    PyValue *storage = small;
    if (count > (int)Py_ARRAY_LENGTH(small)) {
        storage = PyMem_Malloc((size_t)count * sizeof(PyValue));
        if (storage == NULL) {
            PyErr_NoMemory();
            return (_PyCallResult){-1, NULL};
        }
    }
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
    /* The code is pinned across the binding, which the function's mutable
     * code could outlive. */
    PyCodeObject *code = (PyCodeObject *)Py_NewRef(func->func_code);
    PyObject *locals = code->co_flags & CO_OPTIMIZED ? NULL
        : Py_NewRef(func->func_globals);
    PyObject *names = NULL;
    int positional = count;
    if (keywords) {
        names = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(
            _PyFrame_GetCode(frame)->co_consts,
            _PyIR_CallOperand(head, (int)IR_C(word) - 1)));
        positional -= (int)PyTuple_GET_SIZE(names);
    }
    PyValue function;
    if (lead || (word & IR_F_LOCAL_B)) {
        function = PyValue_DUP(callable);
    }
    else {
        function = callable;
        slots[IR_B(word)] = PyValue_NULL;
    }
    if (lead) {
        storage[0] = PyValue_DUP(receiver);
    }
    call_take_operands(slots, head, skip, operands - skip, storage + lead);
    _PyBound bound = eval_frame_push_and_init_code(
        tstate, function, locals, storage, (size_t)positional, names, frame,
        code);
    if (storage != small) {
        PyMem_Free(storage);
    }
    Py_DECREF(code);
    if (bound.gen != NULL) {
        /* The call is done: its object is the result */
        return call_publish_object(tstate, frame, head, word, bound.gen,
                                   (int)IR_C(word) - keywords);
    }
    _PyInterpreterFrame *child = bound.frame;
    if (child == NULL) {
        (void)call_refresh(tstate, frame, head);
        return (_PyCallResult){-1, NULL};
    }
    call_commit(tstate, frame, head, child);
    if (lead) {
        call_release_callable(frame, word);
    }
    return (_PyCallResult){0, child};
}

/* The end of an instrumented call to native code: the events of its result.
 * Out of line, as the events are rare. */
static Py_NO_INLINE _PyCallResult
call_run_instrumented_result(PyThreadState *tstate, _PyInterpreterFrame *frame,
                             _PyInstr *head, _PyIRWord word, PyValue callable,
                             PyValue first, PyValue result, int flags,
                             int keywords)
{
    /* An instrumented call: the callee can run Python, so check the frame
     * before the event's callback reads it, and again after the callback,
     * which can run Python too. */
    int refreshed = call_refresh(tstate, frame, head);
    int failed = call_monitor_result(tstate, frame, head, callable, first,
                                     PyValue_IsNull(result) || refreshed < 0);
    if (!failed && refreshed == 0) {
        refreshed = call_refresh(tstate, frame, head);
    }
    if (failed || refreshed < 0) {
        PyValue_XCLOSE(result);
        return (_PyCallResult){-1, NULL};
    }
    PyValue displaced = call_publish_cold(frame, head, word, result,
                                          (int)IR_C(word) - keywords);
    PyValue_XCLOSE(displaced);
    /* Releasing the operands can run a finalizer. */
    int status = call_refresh(tstate, frame, head);
    if (status == 0 && (flags & CALL_PERIODIC)) {
        /* Signal handlers and the GC can run Python. */
        if (check_periodics_at_end(tstate, frame) < 0 ||
            call_refresh(tstate, frame, head) < 0) {
            status = -1;
        }
    }
    return (_PyCallResult){status, NULL};
}

/* The generic CALL and CALL_KW: their words, instrumented or not, and the
 * guarded calls that missed.  Returns {-1, NULL} on error, {0, child} for a
 * pushed frame and {0, NULL} for a finished native call.  The handler keeps
 * what it did before the call in flags (CALL_SELF_PRESENT for a self operand
 * that is not NULL, CALL_INSTRUMENTED and CALL_PERIODIC for what the handler
 * is). */
static inline Py_ALWAYS_INLINE _PyCallResult
call_run(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *head,
         int flags, int keywords)
{
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*head);
    PyValue *slots = _PyFrame_Slots(frame);
    PyValue receiver;
    PyValue callable = call_expand_method(slots[IR_B(word)], flags, &receiver);
    int instrumented = (flags & CALL_INSTRUMENTED) != 0;
    PyValue first = PyValue_NULL;
    if (Py_UNLIKELY(instrumented)) {
        first = call_first_argument(slots, head, receiver, keywords);
        if (call_monitor(tstate, frame, head, callable, first) < 0) {
            return (_PyCallResult){-1, NULL};
        }
    }
    if (call_is_python_function(tstate, callable)) {
        /* A keyword call has no plan here (no site's keyword destinations):
         * the general binding takes it */
        if (!keywords) {
            _PyBindPlan plan;
            int skip = call_skipped_operands(slots, head, word);
            PyFunctionObject *func =
                (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
            if (_PyBindPlan_ForCall(&plan, (PyCodeObject *)func->func_code,
                                    !PyValue_IsNull(receiver) + (int)IR_C(word) - skip,
                                    skip, !PyValue_IsNull(receiver)))
            {
                _PyCallResult pushed = call_bind_positional(
                    tstate, frame, head, callable, receiver, &plan);
                if (pushed.status <= 0) {
                    return pushed;
                }
            }
        }
        return call_push_python(tstate, frame, head, callable, receiver,
                                keywords);
    }
    PyValue result = call_vector_impl(tstate, frame, head, callable, receiver,
                                      keywords);
    if (Py_UNLIKELY(instrumented)) {
        return call_run_instrumented_result(tstate, frame, head, word,
                                            callable, first, result, flags,
                                            keywords);
    }
    /* The callee may have run Python, but nothing that the rest of the
     * call reads can have changed.  A native callee returns on the C
     * stack above this activation, so this frame is the current one;
     * the executing copy of the code, and so the head, is fixed for the
     * life of the code object (register-ir.md 1.10), as value_operation_
     * refresh() asserts; and the frame's slots stay where they are.  The
     * temporaries are owned by their homes and a local that the callee
     * rebound keeps its old value alive, so publishing reads nothing
     * stale.  A finalizer that the release runs is in the same position.
     * Only an event's callback (call_run_instrumented_result) reads the
     * frame after the call. */
    assert(tstate->current_frame == frame);
    assert(_PyFrame_GetCode(frame)->_co_exec != NULL &&
           frame->ir_words == _PyFrame_GetCode(frame)->_co_exec->words);
    if (PyValue_IsNull(result)) {
        return (_PyCallResult){-1, NULL};
    }
    /* The keyword names operand of CALL_KW is not a slot. */
    PyValue displaced = call_publish(frame, head, word, result,
                                     (int)IR_C(word) - keywords);
    PyValue_XCLOSE(displaced);
    if ((flags & CALL_PERIODIC) && check_periodics_at_end(tstate, frame) < 0) {
        return (_PyCallResult){-1, NULL};
    }
    return (_PyCallResult){0, NULL};
}

static Py_NO_INLINE _PyCallResult
call_run_keywords(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  _PyInstr *head, int flags)
{
    return call_run(tstate, frame, head, flags, 1);
}

/* The rows of Call.Generic (Python/register_ops/calls.rop) on call_run's
 * paths.  A generator function takes the native path: vectorcall creates the
 * generator. */

static inline bool
_PyRop_PythonTarget(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue *fn, const _PyIRWord *head)
{
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    if (!call_is_python_function(tstate, callable)) {
        return false;
    }
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
    return !code_creates_generator((PyCodeObject *)func->func_code);
}

static Py_NO_INLINE _PyInterpreterFrame *
_PyRop_PushPython(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue *fn, _PyInstr *head)
{
    _PyIRWord word = head[0];
    assert(fn == &_PyFrame_Slots(frame)[IR_B(word)]);
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    int lead = !PyValue_IsNull(receiver);
    int skip = call_skipped_operands(_PyFrame_Slots(frame), head, word);
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
    _PyBindPlan plan;
    _PyCallResult pushed = {1, NULL};
    if (_PyBindPlan_ForCall(&plan, (PyCodeObject *)func->func_code,
                            lead + (int)IR_C(word) - skip, skip, lead)) {
        pushed = call_bind_positional(tstate, frame, head, callable, receiver,
                                      &plan);
    }
    if (pushed.status > 0) {
        pushed = call_push_python(tstate, frame, head, callable, receiver, 0);
    }
    if (pushed.status < 0) {
        rop_call_retire_operands(frame, head);
        return NULL;
    }
    assert(pushed.child != NULL);
    return pushed.child;
}

static inline PyValue
_PyRop_CallNative(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue *fn, const _PyIRWord *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    return call_vector_impl(tstate, frame, head, callable, receiver, 0);
}

/* The rows of Call.Observed: the events of call_run with CALL_INSTRUMENTED */

static Py_NO_INLINE int
_PyRop_CallEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site, PyValue *fn, const _PyIRWord *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    PyValue first = call_first_argument(_PyFrame_Slots(frame), head,
                                        receiver, 0);
    return call_monitor(tstate, frame, site, callable, first);
}

/* A Python function, which a generator function is, fires no C event: the
 * old call pushed its frame */
static Py_NO_INLINE PyValue
_PyRop_CallNativeObserved(PyThreadState *tstate, _PyInterpreterFrame *frame,
                          _PyInstr *site, PyValue *fn, const _PyIRWord *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    PyValue first = call_first_argument(_PyFrame_Slots(frame), head,
                                        receiver, 0);
    PyValue result = call_vector_impl(tstate, frame, site, callable, receiver,
                                      0);
    if (call_is_python_function(tstate, callable)) {
        return result;
    }
    int refreshed = call_refresh(tstate, frame, site);
    int failed = call_monitor_result(tstate, frame, site, callable, first,
                                     PyValue_IsNull(result) || refreshed < 0);
    if (!failed && refreshed == 0) {
        refreshed = call_refresh(tstate, frame, site);
    }
    if (failed || refreshed < 0) {
        PyValue_XCLOSE(result);
        return PyValue_NULL;
    }
    return result;
}

/* The positional arguments of a CALL_FUNCTION_EX that are not a tuple, as a
 * new tuple: the call keeps it, and the operand keeps the iterable until the
 * call is over.  Preparing it can run Python (__iter__, __getitem__); the
 * messages of the errors are upstream's. */
static Py_NO_INLINE PyObject *
call_args_tuple(PyThreadState *tstate, _PyInterpreterFrame *frame,
                _PyInstr *head, PyValue callable, PyValue args)
{
    PyObject *args_object = PyValue_AsObject(args);
    if (args_object == NULL) {
        return NULL;
    }
    PyObject *tuple = NULL;
    PyObject *callable_object = PyValue_AsObject(callable);
    if (callable_object != NULL &&
        _Py_Check_ArgsIterable(tstate, callable_object, args_object) >= 0) {
        tuple = _PyValue_SequenceTuple(PyValue_FromObjectBorrow(args_object));
    }
    Py_XDECREF(callable_object);
    Py_DECREF(args_object);
    if (call_refresh(tstate, frame, head) < 0) {
        Py_CLEAR(tuple);
    }
    return tuple;
}

/* The generic CALL_FUNCTION_EX, instrumented or not.  The operands are the
 * positional iterable or tuple and, with CALL_KWARGS_PRESENT, the kwargs
 * dict.  A Python callee takes new references to them and to the function
 * (and the tuple, if the call prepared one), and the operands are released
 * once its frame is current; a native callee and every instrumented call go
 * through _PyValue_Call() as upstream's do. */
static Py_NO_INLINE _PyCallResult
call_run_ex(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *head,
            int flags)
{
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*head);
    PyValue *slots = _PyFrame_Slots(frame);
    PyValue callable = slots[IR_B(word)];
    PyValue args = slots[_PyIR_CallOperand(head, 0)];
    PyObject *kwargs = (flags & CALL_KWARGS_PRESENT)
        ? PyValue_AsKnownObjectBorrow(slots[_PyIR_CallOperand(head, 1)]) : NULL;
    int operands = 1 + ((flags & CALL_KWARGS_PRESENT) != 0);
    int instrumented = (flags & CALL_INSTRUMENTED) != 0;
    PyObject *tuple = NULL;
    if (!PyValue_IsObject(args) ||
        !PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(args))) {
        tuple = call_args_tuple(tstate, frame, head, callable, args);
        if (tuple == NULL) {
            return (_PyCallResult){-1, NULL};
        }
    }
    PyObject *callargs = tuple != NULL ? tuple : PyValue_AsKnownObjectBorrow(args);
    // The first argument is for the monitoring events alone
    PyValue first = PyValue_NULL;
    if (instrumented) {
        if (PyTuple_GET_SIZE(callargs)) {
            first = _PyIR_AdoptTemp(PyTuple_GET_VALUE(callargs, 0));
        }
        if (call_monitor(tstate, frame, head, callable, first) < 0) {
            Py_XDECREF(tuple);
            return (_PyCallResult){-1, NULL};
        }
    }
    PyValue receiver = PyValue_NULL;
    PyValue target = instrumented ? callable
        : call_expand_method(callable, 0, &receiver);
    if (!instrumented && call_is_python_function(tstate, target)) {
        /* The items of the tuple and the dict (an empty one is nothing)
         * bind in place, after the receiver of a bound method: the child
         * takes counts of them, and the operands and the tuple that this
         * call prepared are released once it is current.  What the plan
         * does not cover binds below. */
        PyFunctionObject *callee =
            (PyFunctionObject *)PyValue_AsKnownObjectBorrow(target);
        PyCodeObject *callee_code = (PyCodeObject *)callee->func_code;
        int lead = !PyValue_IsNull(receiver);
        int count = lead + (int)Py_MIN(PyTuple_GET_SIZE(callargs), INT_MAX - 1);
        _CallDict items;
        const _CallDict *dict_items = NULL;
        int plannable = 1;
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0) {
            plannable = call_bind_dict_destinations(
                callee_code, Py_MIN(count, callee_code->co_argcount), kwargs,
                &items);
            dict_items = &items;
        }
        /* The site's plan holds for the code and the arity it was made for,
         * and for a call without items of a dict.  A site past the limit of
         * the feedback vector shares entry 0, a header alone. */
        const _PyFbCall *fb = (const _PyFbCall *)IR_FB(IR_D(word));
        _PyBindPlan plan;
        const _PyBindPlan *planned = &plan;
        if (dict_items == NULL && fb->h.kind == FB_CALL &&
            fb->func_version == callee_code->co_version &&
            fb->plan.positional == count &&
            !!(fb->plan.flags & _Py_CALL_BIND_LEAD) == lead &&
            (fb->plan.flags & _Py_CALL_BIND_VALID))
        {
            planned = &fb->plan;
        }
        else if (!plannable ||
                 !(dict_items != NULL
                   ? _PyBindPlan_Init(&plan, callee_code, count, 0, lead,
                                      items.dests, items.count)
                   : _PyBindPlan_ForCall(&plan, callee_code, count, 0, lead)))
        {
            planned = NULL;
        }
        if (planned != NULL) {
            _PyCallResult pushed = call_bind_tuple(
                tstate, frame, head, target, receiver, callargs, dict_items,
                planned);
            if (pushed.status < 0) {
                Py_XDECREF(tuple);
                return pushed;
            }
            if (pushed.status == 0) {
                call_release_rest(_PyFrame_Slots(frame), head, word, operands, UINT32_MAX);
                Py_XDECREF(tuple);
                return pushed;
            }
        }
    }
    if (!instrumented && PyValue_IsNull(receiver) &&
        call_is_python_function(tstate, callable)) {
        PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
        PyCodeObject *code = (PyCodeObject *)Py_NewRef(func->func_code);
        PyObject *locals = code->co_flags & CO_OPTIMIZED ? NULL
            : Py_NewRef(func->func_globals);
        PyValue function = PyValue_DUP(callable);
        Py_ssize_t nargs = PyTuple_GET_SIZE(callargs);
        /* The binding consumes these on both outcomes */
        PyObject *owned_args = tuple != NULL ? tuple : Py_NewRef(callargs);
        PyObject *owned_kwargs = Py_XNewRef(kwargs);
        _PyBound bound = eval_frame_push_and_init_ex_code(
            tstate, function, locals, nargs, owned_args, owned_kwargs, frame,
            code);
        Py_DECREF(code);
        if (bound.gen != NULL) {
            /* The call is done: its object is the result, and the
             * operands are still in their homes. */
            return call_publish_object(tstate, frame, head, word, bound.gen,
                                       operands);
        }
        _PyInterpreterFrame *child = bound.frame;
        if (child == NULL) {
            (void)call_refresh(tstate, frame, head);
            return (_PyCallResult){-1, NULL};
        }
        call_commit(tstate, frame, head, child);
        call_release_rest(_PyFrame_Slots(frame), head, word, operands, UINT32_MAX);
        return (_PyCallResult){0, child};
    }
    PyValue result = _PyValue_Call(tstate, callable, callargs, kwargs);
    /* The call can run Python: check the frame before monitoring reads it. */
    int refreshed = call_refresh(tstate, frame, head);
    int failed = PyValue_IsNull(result);
    if (instrumented &&
        !(PyValue_IsObject(callable) &&
          (PyFunction_Check(PyValue_AsKnownObjectBorrow(callable)) ||
           PyMethod_Check(PyValue_AsKnownObjectBorrow(callable))))) {
        failed = call_monitor_result(tstate, frame, head, callable, first,
                                     failed || refreshed < 0);
        /* The event can run Python too. */
        if (!failed && refreshed == 0) {
            refreshed = call_refresh(tstate, frame, head);
        }
    }
    if (failed || refreshed < 0) {
        PyValue_XCLOSE(result);
        Py_XDECREF(tuple);
        return (_PyCallResult){-1, NULL};
    }
    PyValue displaced = call_publish(frame, head, word, result, operands);
    PyValue_XCLOSE(displaced);
    Py_XDECREF(tuple);
    /* Releasing the operands can run a finalizer. */
    int status = call_refresh(tstate, frame, head);
    if (status == 0 && (flags & CALL_PERIODIC)) {
        /* Signal handlers and the GC can run Python. */
        if (check_periodics_at_end(tstate, frame) < 0 ||
            call_refresh(tstate, frame, head) < 0) {
            status = -1;
        }
    }
    return (_PyCallResult){status, NULL};
}

/* Whether a call of code with nargs positional arguments and no keywords
 * binds each argument to its own local: no *args, **kwargs, keyword-only
 * parameter or default to fill. */
static inline Py_ALWAYS_INLINE int
code_binds_exactly(PyCodeObject *code, Py_ssize_t nargs)
{
    return nargs == code->co_argcount && code->co_kwonlyargcount == 0 &&
           !(code->co_flags & (CO_VARARGS | CO_VARKEYWORDS));
}

/* Argument i of a call as a borrowed word: the window word, or the word of
 * the object of an object vector. */
static inline Py_ALWAYS_INLINE PyValueBorrow
vector_arg(const void *args, Py_ssize_t i, int objects)
{
    if (objects) {
        return PyValue_FromObjectBorrow(((PyObject *const *)args)[i]);
    }
    return PyValue_Borrow(((const PyValue *)args)[i]);
}

/* The entry's binding of a call that does not bind each argument to its own
 * local (keywords, defaults, *args, **kwargs, a generator function): it
 * gathers owners of the arguments for the general binder, which consumes
 * func's and locals' counts too.  The frame is current for the evaluator, or
 * the call made a generator. */
static Py_NO_INLINE _PyBound
vector_bind(PyThreadState *tstate, PyFunctionObject *func, PyObject *locals,
            const void *args, int objects, Py_ssize_t nargs, PyObject *kwnames)
{
    Py_ssize_t total = nargs + (kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames));
    PyValue small[8];
    PyValue *owners = small;
    if (total > (Py_ssize_t)Py_ARRAY_LENGTH(small)) {
        owners = PyMem_Malloc(sizeof(PyValue) * (size_t)total);
        if (owners == NULL) {
            Py_XDECREF(locals);
            _PyErr_NoMemory(tstate);
            return (_PyBound){NULL, NULL};
        }
    }
    for (Py_ssize_t i = 0; i < total; i++) {
        owners[i] = PyValue_DUP(vector_arg(args, i, objects));
    }
    /* The code is pinned across the binding, which the function's mutable
     * code could outlive. */
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    Py_INCREF(code);
    _PyBound bound = eval_frame_push_and_init_code(
        tstate, PyValue_DUP(PyValue_FromObjectBorrow(func)), locals, owners,
        nargs, kwnames, NULL, code);
    Py_DECREF(code);
    if (owners != small) {
        PyMem_Free(owners);
    }
    return bound;
}

/* The entry of every native caller of a Python function: it pushes the
 * frame, binds the arguments and runs it.  The arguments are the window of
 * words that value code holds, or the vector of objects that the object API
 * holds (objects is a constant of each caller).  A call that binds exactly
 * dups each argument into its local.  locals is a new reference, or NULL,
 * that the frame consumes.  The frame is current for the evaluator, which
 * consumes it. */
static inline Py_ALWAYS_INLINE PyValue
eval_vector(PyThreadState *tstate, PyFunctionObject *func, PyObject *locals,
            const void *args, int objects, size_t nargsf, PyObject *kwnames)
{
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    _PyInterpreterFrame *frame;
    /* A generator function's call creates its generator, not a frame to
     * run: it takes the general path, which binds into the generator. */
    if (kwnames == NULL && code_binds_exactly(code, nargs) &&
        !code_creates_generator(code))
    {
        /* Nothing here runs Python, so func's code cannot change before
         * the frame holds a count of it. */
        CALL_STAT_INC(frames_pushed);
        if (_PyCode_GetExec(code) == NULL) {
            Py_XDECREF(locals);
            return PyValue_NULL;
        }
        frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
        if (frame == NULL) {
            Py_XDECREF(locals);
            _PyErr_NoMemory(tstate);
            return PyValue_NULL;
        }
        _PyFrame_Initialize(tstate, frame,
                            PyValue_DUP(PyValue_FromObjectBorrow(func)),
                            locals, code, frame->base, NULL);
        _PyFrame_CopyFreeVars(frame, code);
        for (Py_ssize_t i = 0; i < nargs; i++) {
            frame->base[i] = PyValue_DUP(vector_arg(args, i, objects));
        }
    }
    else {
        _PyBound bound = vector_bind(tstate, func, locals, args, objects,
                                     nargs, kwnames);
        if (bound.gen != NULL) {
            return PyValue_FromObjectSteal((PyObject *)bound.gen);
        }
        frame = bound.frame;
        if (frame == NULL) {
            return PyValue_NULL;
        }
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_VECTOR);
    PyValue result = eval_frame_value(tstate, frame, 0);
#ifndef NDEBUG
    /* The first root chunk is retained even after its initial frame exits. */
    assert(chunk == NULL || tstate->datastack_chunk == chunk);
    assert(chunk == NULL || tstate->datastack_top == top);
    assert(chunk == NULL || tstate->datastack_limit == limit);
#endif
    return result;
}

static inline Py_ALWAYS_INLINE PyObject *
function_locals(PyFunctionObject *func)
{
    return ((PyCodeObject *)func->func_code)->co_flags & CO_OPTIMIZED ? NULL
        : Py_NewRef(func->func_globals);
}

/* The value vectorcall of a Python function (func_value_vectorcall). */
Py_NO_INLINE PyValue
_PyEval_VectorValue(PyThreadState *tstate, PyObject *func,
                    const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    EVAL_CALL_STAT_INC(EVAL_CALL_FUNCTION_VECTORCALL);
    PyFunctionObject *function = (PyFunctionObject *)func;
    return eval_vector(tstate, function, function_locals(function), args, 0,
                       nargsf, kwnames);
}

/* The object vectorcall of a Python function.  A function whose vectorcall
 * was replaced (PyFunction_SetVectorcall()) has no value form: an extension
 * that cached this entry before the replacement reaches the new one. */
Py_VALUE_ENTRY PyObject *
_PyFunction_Vectorcall(PyObject *func, PyObject *const *stack, size_t nargsf,
                       PyObject *kwnames)
{
    PyFunctionObject *function = (PyFunctionObject *)func;
    assert((function->vectorcall == _PyFunction_Vectorcall)
           == (function->func_value_vectorcall != NULL));
    if (function->func_value_vectorcall == NULL) {
        return function->vectorcall(func, stack, nargsf, kwnames);
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_FUNCTION_VECTORCALL);
    PyValue result = eval_vector(_PyThreadState_GET(), function,
                                 function_locals(function), stack, 1, nargsf,
                                 kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

/* The entry of code that runs with its own locals (a module or class body,
 * exec): its arguments are objects. */
Py_NO_INLINE PyObject *
_PyEval_Vector(PyThreadState *tstate, PyFunctionObject *func,
               PyObject *locals,
               PyObject* const* args, size_t argcount,
               PyObject *kwnames)
{
    PyValue result = eval_vector(tstate, func, Py_XNewRef(locals), args, 1,
                                 argcount, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
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

        PyValue match_owner = _PyTuple_GetValue(pair, 0);
        *match = PyValue_IsNull(match_owner) ? NULL
            : _PyValue_AsObjectStealAt(match_owner, BOX_SITE_TUPLE);
        if (*match == NULL) {
            Py_DECREF(pair);
            return -1;
        }
        PyValue rest_owner = _PyTuple_GetValue(pair, 1);
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

/* Iterate v (borrowed) argcnt times and store the results in out[0], out[1],
   ... in the order they come.  Return 1 for success, 0 if error: the values
   already stored are closed.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target, whose list is stored after the argcnt values,
   followed by the argcntafter values that end the iterable.
*/

Py_NO_INLINE int
_PyEval_UnpackIterableValue(PyThreadState *tstate, PyObject *v,
                       int argcnt, int argcntafter, PyValue *out)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it = NULL;  /* iter(v) */
    _PyStatusValue step;
    PyObject *l = NULL; /* variable list */
    assert(v != NULL);
    assert(argcnt >= 0 && argcntafter >= -1);
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
        out[i] = step.value;
        _PyValue_CheckTemp(out[i]);
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
    out[i++] = PyValue_FromObjectSteal(l);

    ll = PyList_GET_SIZE(l);
    if (ll < argcntafter) {
        _PyErr_Format(tstate, PyExc_ValueError,
            "not enough values to unpack (expected at least %d, got %zd)",
            argcnt + argcntafter, argcnt + ll);
        goto Error;
    }

    /* Pop the "after-variable" args off the list. */
    for (j = argcntafter; j > 0; j--, i++) {
        out[i] = _PyList_ITEMS(l)[ll - j];
        _PyList_ITEMS(l)[ll - j] = PyValue_NULL;
    }
    /* Resize the list. */
    Py_SET_SIZE(l, ll - argcntafter);
    result = 1;
    goto Done;

Error:
    while (i > 0) {
        PyValue_CLOSE(out[--i]);
    }
Done:
    Py_XDECREF(it);
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
        int found = _PyDict_GetValue(
            (PyDictObject *)globals, _PyValue_FromObjectKnown(&_Py_ID(__builtins__)),
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
        if (_PyDict_SetValue((PyDictObject *)globals, _PyValue_FromObjectKnown(&_Py_ID(__builtins__)), PyValue_FromObjectBorrow(builtins)) < 0) {
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
static inline Py_ALWAYS_INLINE PyValue
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

/* The value forms: a number cannot be an exception class, which
   _PyEval_CheckExceptTypeValid reports for any other non-class */
int
(_PyEval_CheckExceptTypeValidValue)(PyThreadState *tstate, PyValueBorrow right)
{
    if (!PyValue_IsObject(right)) {
        _PyErr_SetString(tstate, PyExc_TypeError, CANNOT_CATCH_MSG);
        return -1;
    }
    return _PyEval_CheckExceptTypeValid(tstate, PyValue_AsKnownObjectBorrow(right));
}

int
(_PyEval_CheckExceptStarTypeValidValue)(PyThreadState *tstate, PyValueBorrow right)
{
    if (!PyValue_IsObject(right)) {
        _PyErr_SetString(tstate, PyExc_TypeError, CANNOT_CATCH_MSG);
        return -1;
    }
    return _PyEval_CheckExceptStarTypeValid(tstate, PyValue_AsKnownObjectBorrow(right));
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

/* The binary intrinsics other than GETATTR and SETATTR take objects:
   a type parameter, a function, an exception group. Boxes them, as above. */
Py_NO_INLINE PyValue
(_PyEval_CallIntrinsic2)(PyThreadState *tstate, int oparg,
                         PyValueBorrow value2, PyValueBorrow value1)
{
    PyObject *boxes[2];
    _PyValueTemps temps = {boxes, 0, 2};
    PyValue res = PyValue_NULL;
    PyObject *value1_o = _PyValueTemps_Object(&temps, value1);
    PyObject *value2_o = value1_o != NULL ? _PyValueTemps_Object(&temps, value2) : NULL;
    if (value2_o != NULL) {
        PyObject *res_o = _PyIntrinsics_BinaryFunctions[oparg].func(tstate, value2_o, value1_o);
        if (res_o != NULL) {
            res = PyValue_FromObjectSteal(res_o);
        }
    }
    _PyValueTemps_Close(&temps);
    return res;
}

/* The method descriptors of METH_O and METH_NOARGS take objects
   (ml_meth(PyObject *, PyObject *)): these box a number in a record that
   the handler does not carry, and return the result, or PyValue_NULL with an
   exception set. */
Py_NO_INLINE PyValue
(_PyEval_CallMethodDescriptorO)(PyThreadState *tstate, PyCFunction cfunc,
                              PyValueBorrow self, PyValueBorrow arg)
{
    PyObject *boxes[2];
    _PyValueTemps temps = {boxes, 0, 2};
    PyValue res = PyValue_NULL;
    PyObject *self_o = _PyValueTemps_Object(&temps, self);
    PyObject *arg_o = self_o != NULL ? _PyValueTemps_Object(&temps, arg) : NULL;
    if (arg_o != NULL) {
        _PyBorrowMark borrow_mark = _PyBorrowScope_Enter(tstate);
        PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, self_o, arg_o);
        _PyBorrowScope_Exit(tstate, borrow_mark);
        assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
        if (res_o != NULL) {
            res = PyValue_FromObjectSteal(res_o);
        }
    }
    _PyValueTemps_Close(&temps);
    return res;
}

Py_NO_INLINE PyValue
(_PyEval_CallMethodDescriptorNoArgs)(PyThreadState *tstate, PyCFunction cfunc,
                                   PyValueBorrow self)
{
    PyObject *boxes[1];
    _PyValueTemps temps = {boxes, 0, 1};
    PyValue res = PyValue_NULL;
    PyObject *self_o = _PyValueTemps_Object(&temps, self);
    if (self_o != NULL) {
        _PyBorrowMark borrow_mark = _PyBorrowScope_Enter(tstate);
        PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, self_o, NULL);
        _PyBorrowScope_Exit(tstate, borrow_mark);
        assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
        if (res_o != NULL) {
            res = PyValue_FromObjectSteal(res_o);
        }
    }
    _PyValueTemps_Close(&temps);
    return res;
}

/* The intrinsics of CALL_INTRINSIC_1 other than unary plus take objects.
   Only PRINT and ASYNC_GEN_WRAP can get a number: this boxes it here, in a
   record that the handler does not carry. Returns the result, or
   PyValue_NULL with an exception set. */
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_CallIntrinsic1)(PyThreadState *tstate, int oparg, PyValueBorrow value)
{
    if (PyValue_IsPointerSpace(value)) {
        // An object goes to the intrinsic as it is; only a number needs a box
        PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(
            tstate, PyValue_AsKnownObjectBorrow(value));
        return res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
    }
    PyObject *boxes[1];
    _PyValueTemps temps = {boxes, 0, 1};
    PyValue res = PyValue_NULL;
    PyObject *value_o = _PyValueTemps_Object(&temps, value);
    if (value_o != NULL) {
        PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(tstate, value_o);
        if (res_o != NULL) {
            res = PyValue_FromObjectSteal(res_o);
        }
    }
    _PyValueTemps_Close(&temps);
    return res;
}

/* Function.Make and Intrinsic.*: the rows (functions.rop) */

/* The function over the code, and the attributes of `present`: bit k is
 * the compiler's flag 1 << k, and the span holds the attributes in the order
 * of their bits.  The code and every attribute are spent on both outcomes. */
static PyValue
_PyRop_FunctionMake(_PyInterpreterFrame *frame, PyValue code,
                    const _PyRopSpan *fields, uint32_t present)
{
    int n = fields->count;
    PyValue vals[n + 1];
    rop_take_items(fields, vals);
    PyObject *codeobj = PyValue_AsKnownObjectBorrow(code);
    PyFunctionObject *func_obj = (PyFunctionObject *)
        PyFunction_New(codeobj, frame->f_globals);
    if (func_obj == NULL) {
        // Close the owners with the exception set aside
        PyObject *exc = PyErr_GetRaisedException();
        PyValue_XCLOSE(code);
        rop_close_items(vals, n);
        PyErr_SetRaisedException(exc);
        return PyValue_NULL;
    }
    _PyFunction_SetVersion(
        func_obj, ((PyCodeObject *)func_obj->func_code)->co_version);
    PyValue_XCLOSE(code);
    int i = 0;
    for (uint32_t bits = present; bits != 0; bits &= bits - 1) {
        _PyFunction_StoreAttribute(func_obj, bits & -bits,
                                   PyValue_AsKnownObjectSteal(vals[i++]));
    }
    assert(i == n);
    return PyValue_FromObjectSteal((PyObject *)func_obj);
}

/* The compiler's number of each member of IntrinsicId, in declaration order */
static const uint8_t rop_intrinsic_number[] = {
    INTRINSIC_PRINT, INTRINSIC_IMPORT_STAR, INTRINSIC_STOPITERATION_ERROR,
    INTRINSIC_ASYNC_GEN_WRAP, INTRINSIC_UNARY_POSITIVE,
    INTRINSIC_LIST_TO_TUPLE, INTRINSIC_TYPEVAR, INTRINSIC_PARAMSPEC,
    INTRINSIC_TYPEVARTUPLE, INTRINSIC_SUBSCRIPT_GENERIC,
    INTRINSIC_TYPEALIAS, INTRINSIC_BUILD_FROZENSET,
    INTRINSIC_PREP_RERAISE_STAR, INTRINSIC_TYPEVAR_WITH_BOUND,
    INTRINSIC_TYPEVAR_WITH_CONSTRAINTS, INTRINSIC_SET_FUNCTION_TYPE_PARAMS,
    INTRINSIC_SET_TYPEPARAM_DEFAULT, INTRINSIC_ADD_CONDITIONAL_ANNOTATION,
    INTRINSIC_MATCH_CLASS_ISINSTANCE,
    INTRINSIC_GETATTR, INTRINSIC_SETATTR,
};

static PyValue
_PyRop_IntrinsicUnary(PyThreadState *tstate, uint32_t id, PyValue x)
{
    PyValue value = x;
    int number = rop_intrinsic_number[id];
    PyValue res;
    if (number == INTRINSIC_UNARY_POSITIVE) {
        res = _PyValue_Positive(value);
    }
    else {
        // The other intrinsics take objects: the helper boxes a number
        res = _PyEval_CallIntrinsic1(tstate, number, value);
    }
    PyValue_XCLOSE(value);
    return res;
}

static PyValue
_PyRop_IntrinsicImportStar(PyThreadState *tstate, PyValue x)
{
    PyValue value = x;
    PyValue res = _PyEval_CallIntrinsic1(tstate, INTRINSIC_IMPORT_STAR, value);
    PyValue_XCLOSE(value);
    return res;
}

static PyValue
_PyRop_IntrinsicStopIteration(PyThreadState *tstate, PyValue x)
{
    PyValue value = x;
    PyValue res = _PyEval_CallIntrinsic1(tstate, INTRINSIC_STOPITERATION_ERROR,
                                         value);
    PyValue_XCLOSE(value);
    return res;
}

/* x is the deeper stack value, the function's first argument */
static PyValue
_PyRop_IntrinsicBinary(PyThreadState *tstate, uint32_t id, PyValue x, PyValue y)
{
    PyValue held2 = x;
    PyValue held1 = y;
    PyValue res = _PyEval_CallIntrinsic2(tstate, rop_intrinsic_number[id],
                                         held2, held1);
    PyValue_XCLOSE(held1);
    PyValue_XCLOSE(held2);
    return res;
}

static PyValue
_PyRop_IntrinsicAttr(PyThreadState *tstate, uint32_t id, PyValue x, PyValue y)
{
    PyValue held2 = x;
    PyValue held1 = y;
    PyValue res;
    if (rop_intrinsic_number[id] == INTRINSIC_GETATTR) {
        // The owner is a value; the name is a str
        PyObject *name = PyValue_ObjectOrNull(held1);
        assert(name != NULL && PyUnicode_CheckExact(name));
        res = _PyValue_GetAttr(held2, name);
    }
    else {
        // held2 is the value, NULL for a deletion; held1 is the tuple
        // (owner, name)
        assert(rop_intrinsic_number[id] == INTRINSIC_SETATTR);
        PyObject *target = PyValue_ObjectOrNull(held1);
        assert(target != NULL && PyTuple_CheckExact(target)
               && PyTuple_GET_SIZE(target) == 2);
        PyValueBorrow owner = PyTuple_GET_VALUE(target, 0);
        PyObject *attr_name = _PyTuple_GET_OBJECT(target, 1);
        int err = _PyValue_SetAttr(owner, attr_name, held2);
        res = err < 0 ? PyValue_NULL : PyValue_None;
    }
    PyValue_XCLOSE(held1);
    PyValue_XCLOSE(held2);
    return res;
}

void
(_PyEval_FormatKwargsError)(PyThreadState *tstate, PyValueBorrow func, PyObject *kwargs, PyObject *dupkey)
{
    if (dupkey != NULL) {
        // The callable may be a number: boxed here, out of the handler
        PyObject *func_o = PyValue_AsObject(func);
        if (func_o == NULL) {
            return;
        }
        PyObject *funcstr = _PyObject_FunctionStr(func_o);
        Py_DECREF(func_o);
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
        PyValue_XCLOSE(old);
    }
}

PyObject *
(_PyEval_GetAwaitable)(PyValueBorrow iterable_value, int oparg)
{
    if (!PyValue_IsObject(iterable_value)) {
        // A number is not awaitable: _PyCoro_GetAwaitableIter's error,
        // which _PyEval_FormatAwaitableError replaces for `async with`
        PyTypeObject *type = PyValue_TYPE(iterable_value);
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object can't be awaited", type->tp_name);
        _PyEval_FormatAwaitableError(PyThreadState_GET(), type, oparg);
        return NULL;
    }
    PyObject *iterable = PyValue_AsKnownObjectBorrow(iterable_value);
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

/* Out of line: the lookups pass the address of a local (_PyDict_GetValue's
 * out-parameter), which would be the handler's */
Py_NO_INLINE PyValue
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
    found.status = _PyDict_GetValue((PyDictObject *)namespace,
                                    PyValue_FromObjectBorrow(name),
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

/* FOR_ITER's step: the item and 1, the completion value and 0, or the
 * error and -1; a StopIteration is the completion.  The cursor goes in and
 * comes out, advanced by an item of an index-protocol iterator. */
static inline Py_ALWAYS_INLINE _PyForIterStep
_PyForIter_VirtualIteratorNextInline(PyThreadState *tstate,
                                     _PyInterpreterFrame *frame,
                                     PyValueBorrow iter, PyValue cursor)
{
    _PyForIterStep result = {0, PyValue_NULL, cursor};
    PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
    PyValueBorrow index = PyValue_Borrow(cursor);
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
        result.cursor = PyValue_TagIndex(i);
        result.status = 1;
        return result;
    }
    PyTypeObject *iter_type = Py_TYPE(iter_o);
    _Py_iternextvaluefunc next_value = _PyType_VALUE_SLOT(iter_type, iternext);
    _PyStatusValue step;
    if (next_value != NULL) {
        _PyType_CHECK_VALUE_ROW(iter_type, _Py_VALUE_SLOT_INDEX_iternext);
        step = next_value(iter_o);
        assert(step.status != PYGEN_NEXT || !PyValue_IsNull(step.value));
        assert(step.status != PYGEN_ERROR || PyValue_IsNull(step.value));
    }
    else {
        step = _PyValue_IterStep(iter_o);
    }
    /* An item, or the semantic completion, which reports an implicit
     * StopIteration once and keeps the returned owner without fabricating
     * a separate RAISE. Both are the caller's status and value as they are. */
    if (step.status != PYGEN_ERROR) {
        result.status = step.status;
        result.value = step.value;
        return result;
    }
    if (_PyForIter_StopIsExhaustion(tstate, frame)) {
        return result;
    }
    result.status = -1;
    return result;
}

/* Called with an error set by a FOR_ITER step: a StopIteration is the
 * exhaustion, so report it to monitoring, clear it and return 1. Any
 * other exception stays set and the result is 0. */
static Py_NO_INLINE int
_PyForIter_StopIsExhaustion(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    if (!_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
        return 0;
    }
    _PyEval_MonitorRaise(tstate, frame, frame->instr_ptr);
    _PyErr_Clear(tstate);
    return 1;
}

/* Out of line, as when it was exported, for the steps that are not
 * FOR_ITER's own. */
static Py_NO_INLINE _PyStatusValue
_PyForIter_VirtualIteratorNext(PyThreadState *tstate, _PyInterpreterFrame *frame,
                               PyValueBorrow iter, PyValue *index_ptr)
{
    _PyForIterStep step = _PyForIter_VirtualIteratorNextInline(tstate, frame, iter, *index_ptr);
    *index_ptr = step.cursor;
    return (_PyStatusValue){step.status, step.value};
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
_PyEval_SpecialMethodCanSuggest(PyTypeObject *type, int oparg)
{
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
