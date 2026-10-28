/* Execute compiled code */

#include "ceval.h"
#include "pycore_long.h"

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
   if the stack pointer is beyond c_stack_soft_limit.  Not inlined: a cold
   error path. */
Py_GC_PRECISE Py_NO_INLINE int
_Py_CheckRecursiveCall(PyThreadState *tstate, const char *where)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    assert(_tstate->c_stack_soft_limit != 0);
    assert(_tstate->c_stack_hard_limit != 0);
    // Each #if branch balances its braces, which the C mode needs.
#if _Py_STACK_GROWS_DOWN
    int beyond_hard_limit = here_addr < _tstate->c_stack_hard_limit;
    int far_out = here_addr < _tstate->c_stack_hard_limit - _PyOS_STACK_MARGIN_BYTES;
    int kbytes_used = (int)(_tstate->c_stack_top - here_addr)/1024;
#else
    int beyond_hard_limit = here_addr > _tstate->c_stack_hard_limit;
    int far_out = here_addr > _tstate->c_stack_hard_limit + _PyOS_STACK_MARGIN_BYTES;
    int kbytes_used = (int)(here_addr - _tstate->c_stack_top)/1024;
#endif
    if (beyond_hard_limit) {
        if (far_out) {
            // Far out of bounds -- Assume stack switching has occurred
            return 0;
        }
        /* Too much stack used to safely raise an exception. Give up. */
        char buffer[80];
        snprintf(buffer, 80, "Unrecoverable stack overflow (used %d kB)%s", kbytes_used, where);
        Py_FatalError(buffer);
    }
    if (tstate->recursion_headroom) {
        return 0;
    }
    else {
        tstate->recursion_headroom++;
        _PyErr_Format(tstate, PyExc_RecursionError,
                    "Stack overflow (used %d kB)%s",
                    kbytes_used,
                    where);
        tstate->recursion_headroom--;
        return -1;
    }
}


const conversion_func _PyEval_ConversionFuncs[4] = {
    [FVC_STR] = PyObject_StrValue,
    [FVC_REPR] = PyObject_ReprValue,
    [FVC_ASCII] = PyObject_ASCIIValue
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
Py_NO_INLINE PyObject *
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
    // We use the two argument form of map.get(key, default) for two reasons:
    // - Atomically check for a key and get its value without error handling.
    // - Don't cause key creation or resizing in dict subclasses like
    //   collections.defaultdict that define __missing__ (or similar).
    _PyValueRoot self, method;
    _PyThreadState_PushValueRoot(tstate, &self);
    _PyThreadState_PushValueRoot(tstate, &method);
    self.value = PyValue_FromObject(map);
    int res = _PyObject_GetMethodValue(tstate, &self.value, &_Py_ID(get), &method.value);
    if (res < 0) {
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
        PyValue key = PyTuple_GET_VALUE(keys, i);
        if (_PySet_ContainsValue((PySetObject *)seen, key) ||
            _PySet_AddValue((PySetObject *)seen, key))
        {
            if (!_PyErr_Occurred(tstate)) {
                // Seen it before!  The message boxes an inline key.
                PyObject *key_obj = PyValue_Object(key);
                if (key_obj != NULL) {
                    _PyErr_Format(tstate, PyExc_ValueError,
                                  "mapping pattern checks duplicate key (%R)",
                                  key_obj);
                }
            }
            goto fail;
        }
        PyValue args[] = { self.value, key, PyValue_FromObject(dummy) };
        PyValue value;
        if (!PyValue_IsNull(self.value)) {
            value = PyObject_VectorcallValue(method.value, args, 3, NULL);
        }
        else {
            value = PyObject_VectorcallValue(method.value, &args[1], 2, NULL);
        }
        if (PyValue_IsNull(value)) {
            goto fail;
        }
        if (PyValue_Is(value, args[2])) {
            // key not in map!
            Py_DECREF(values);
            // Return None:
            values = Py_NewRef(Py_None);
            goto done;
        }
        PyTuple_SET_VALUE(values, i, value);
    }
    // Success:
done:
    _PyThreadState_PopValueRoot(tstate, &method);
    _PyThreadState_PopValueRoot(tstate, &self);
    Py_DECREF(seen);
    Py_DECREF(dummy);
    return values;
fail:
    _PyThreadState_PopValueRoot(tstate, &method);
    _PyThreadState_PopValueRoot(tstate, &self);
    Py_XDECREF(seen);
    Py_XDECREF(dummy);
    Py_XDECREF(values);
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
Py_NO_INLINE PyObject*
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
            assert(PyTuple_GET_ITEM(attrs, 0) == NULL);
            PyTuple_SET_ITEM(attrs, 0, Py_NewRef(subject));
        }
        else {
            for (Py_ssize_t i = 0; i < nargs; i++) {
                PyObject *name = PyTuple_GET_ITEM(match_args, i);
                if (!PyUnicode_CheckExact(name)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "__match_args__ elements must be strings "
                                  "(got %s)", Py_TYPE(name)->tp_name);
                    goto fail;
                }
                PyObject *attr = match_class_attr(tstate, subject, type, name,
                                                  seen);
                if (attr == NULL) {
                    goto fail;
                }
                assert(PyTuple_GET_ITEM(attrs, i) == NULL);
                PyTuple_SET_ITEM(attrs, i, attr);
            }
        }
        Py_CLEAR(match_args);
    }
    // Finally, the keyword subpatterns:
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        PyObject *name = PyTuple_GET_ITEM(kwargs, i);
        PyObject *attr = match_class_attr(tstate, subject, type, name, seen);
        if (attr == NULL) {
            goto fail;
        }
        assert(PyTuple_GET_ITEM(attrs, nargs + i) == NULL);
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
    PyObject *res = PyValue_Object(
        _PyEval_Vector(tstate, func, locals, NULL, 0, NULL));
    Py_DECREF(func);
    return res;
}


/* Interpreter main loop */

PyObject *
PyEval_EvalFrame(PyFrameObject *f)
{
    /* Function kept for backward compatibility */
    PyThreadState *tstate = _PyThreadState_GET();
    return PyValue_Object(_PyEval_EvalFrame(tstate, f->f_frame, 0));
}

PyObject *
PyEval_EvalFrameEx(PyFrameObject *f, int throwflag)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return PyValue_Object(_PyEval_EvalFrame(tstate, f->f_frame, throwflag));
}

#include "ceval_macros.h"


/* WITH_EXCEPT_START's call of an __exit__ or __aexit__ with the type, the
 * value and the traceback of the exception val.  The traceback may be held
 * only by the argument array, which this function creates, so the array is
 * the frame's span (rule 3): the free slot, self, the type, the value and
 * the traceback, raw. */
Py_GC_PRECISE Py_NO_INLINE PyObject *
_PyEval_CallExitWithException(PyObject *exit_func, PyObject *exit_self,
                              PyObject *val)
{
    assert(val && PyExceptionInstance_Check(val));
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *exc = PyExceptionInstance_Class(val);
    PyObject *tb = PyException_GetTraceback(val);
    if (tb == NULL) {
        tb = Py_None;
    }
    PyValue stack[5];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, stack);
    _PyValueSpan_Append(&span, PyValue_NULL);
    _PyValueSpan_AppendObject(&span, exit_self);
    _PyValueSpan_AppendObject(&span, exc);
    _PyValueSpan_AppendObject(&span, val);
    _PyValueSpan_AppendObject(&span, tb);
    int has_self = exit_self != NULL;
    // Raw slots hold the objects' pointers, so the span's buffer is the
    // object array the call takes.
    PyObject *res = PyObject_Vectorcall(
        exit_func, (PyObject *const *)(stack + 2 - has_self),
        (3 + has_self) | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* Helper functions to keep the size of the largest uops down.
 *
 * The helpers that handlers call and that may run code are Py_NO_INLINE,
 * here and in ceval.h.  The eval loop's frame is precise, which the inline
 * audit checks for every function inlined into it: inlined, a helper's
 * locals, which may hold the only reference to what it made, as an argument
 * array holds the objects it passes on, would be in that frame.  Out of
 * line, each is opaque and scanned, or precise and holds what it uses
 * after a collecting call in its span. */

Py_GC_PRECISE Py_NO_INLINE PyValue
_Py_VectorCall_Values(
    PyValue callable,
    PyValue *arguments,
    int total_args,
    PyValue kwnames)
{
    PyObject *kwnames_o = PyValue_ObjectBorrow(kwnames);
    int positional_args = total_args;
    if (kwnames_o != NULL) {
        positional_args -= (int)PyTuple_GET_SIZE(kwnames_o);
    }
    PyValue res = PyObject_VectorcallValue(
        callable, arguments,
        positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames_o);
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    // arguments is a pointer into the GC visible stack,
    // so we must NULL out values as we clear them.
    for (int i = total_args-1; i >= 0; i--) {
        arguments[i] = PyValue_NULL;
    }
    return res;
}

/* Precise (note 11 section 4.1): its span holds the callable, the result
 * and the boxes for the monitoring callbacks, so its frame is not scanned.
 * An opaque frame here kept the stale words of earlier calls as roots
 * (10a-1b.md, item 2's record). */
Py_GC_PRECISE Py_NO_INLINE PyValue
_Py_VectorCallInstrumentation_Values(
    PyValue callable,
    PyValue* arguments,
    int total_args,
    PyValue kwnames,
    bool call_instrumentation,
    _PyInterpreterFrame* frame,
    _PyInstr* this_instr,
    PyThreadState* tstate)
{
    PyValue slots[4];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_Append(&span, callable);
    PyObject* kwnames_o = PyValue_ObjectBorrow(kwnames);
    int positional_args = total_args;
    if (kwnames_o != NULL) {
        positional_args -= (int)PyTuple_GET_SIZE(kwnames_o);
    }
    PyValue res = PyObject_VectorcallValue(
        callable, arguments,
        positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames_o);
    if (call_instrumentation) {
        _PyValueSpan_Append(&span, res);
        // Boxes of an inline callable or first argument, for the monitoring
        // callbacks: the span holds their pointers.
        PyObject *callable_o = PyValue_Object(callable);
        if (callable_o != NULL) {
            _PyValueSpan_AppendObject(&span, callable_o);
        }
        PyObject *arg = total_args == 0 ? &_PyInstrumentation_MISSING
                                        : PyValue_Object(arguments[0]);
        if (arg != NULL) {
            _PyValueSpan_AppendObject(&span, arg);
        }
        if (callable_o == NULL || arg == NULL) {
            res = PyValue_NULL;
        }
        else if (PyValue_IsNull(res)) {
            _Py_call_instrumentation_exc2(
                tstate, PY_MONITORING_EVENT_C_RAISE,
                frame, this_instr, callable_o, arg);
        }
        else {
            int err = _Py_call_instrumentation_2args(
                tstate, PY_MONITORING_EVENT_C_RETURN,
                frame, this_instr, callable_o, arg);
            if (err < 0) {
                res = PyValue_NULL;
            }
        }
    }
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    // arguments is a pointer into the GC visible stack,
    // so we must NULL out values as we clear them.
    for (int i = total_args - 1; i >= 0; i--) {
        arguments[i] = PyValue_NULL;
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

Py_GC_PRECISE Py_NO_INLINE PyValue
_Py_BuiltinCallFast_Values(
    PyValue callable,
    PyValue *arguments,
    int total_args)
{
    PyObject *callable_o = PyValue_ObjectBorrow(callable);
    PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable_o);
    PyValue res = _PyCFunctionFast_CAST(cfunc)(
        PyCFunction_GET_SELF(callable_o),
        arguments,
        total_args
    );
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    return res;
}

Py_GC_PRECISE Py_NO_INLINE PyValue
_Py_BuiltinCallFastWithKeywords_Values(
    PyValue callable,
    PyValue *arguments,
    int total_args)
{
    PyObject *callable_o = PyValue_ObjectBorrow(callable);
    PyCFunctionFastWithKeywords cfunc =
        _PyCFunctionFastWithKeywords_CAST(PyCFunction_GET_FUNCTION(callable_o));
    PyValue res = cfunc(PyCFunction_GET_SELF(callable_o), arguments,
                        total_args, NULL);
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    return res;
}

/* self is an object: a method called on an inline value boxes it. */
Py_GC_PRECISE Py_NO_INLINE PyValue
_PyCallMethodDescriptorFast_Values(
    PyValue callable,
    PyCFunctionFast cfunc,
    PyValue *arguments,
    int total_args)
{
    PyObject *self = PyValue_Object(arguments[0]);
    if (self == NULL) {
        return PyValue_NULL;
    }
    PyValue res = cfunc(self, arguments + 1, total_args - 1);
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    return res;
}

Py_GC_PRECISE Py_NO_INLINE PyValue
_PyCallMethodDescriptorFastWithKeywords_Values(
    PyValue callable,
    PyCFunctionFastWithKeywords cfunc,
    PyValue *arguments,
    int total_args)
{
    PyObject *self = PyValue_Object(arguments[0]);
    if (self == NULL) {
        return PyValue_NULL;
    }
    PyValue res = cfunc(self, arguments + 1, total_args - 1, NULL);
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    return res;
}

Py_GC_PRECISE Py_NO_INLINE PyValue
_Py_CallBuiltinClass_Values(
    PyValue callable,
    PyValue *arguments,
    int total_args)
{
    PyTypeObject *tp = (PyTypeObject *)PyValue_ObjectBorrow(callable);
    PyValue res = tp->tp_vectorcall(callable, arguments,
                                    total_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                    NULL);
    assert(PyValue_IsNull(res) ^ (PyErr_Occurred() == NULL));
    return res;
}

/* Precise.  BUILD_STRING's operands are str objects (FORMAT_SIMPLE,
 * FORMAT_WITH_SPEC, CONVERT_VALUE and str constants), never inline values,
 * so the registers are the object array the join takes and nothing is
 * boxed: the eval loop anchors them. */
Py_GC_PRECISE Py_NO_INLINE PyObject *
_Py_BuildString_Values(
    PyValue *arguments,
    int total_args)
{
#ifndef NDEBUG
    for (int i = 0; i < total_args; i++) {
        assert(PyValue_IsObject(arguments[i]));
        assert(PyUnicode_Check(PyValue_ObjectBorrow(arguments[i])));
    }
#endif
    PyObject *res = _PyUnicode_JoinArray(
        &_Py_STR(empty), (PyObject *const *)arguments, total_args);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    // arguments is a pointer into the GC visible stack,
    // so we must NULL out values as we clear them.
    for (int i = total_args-1; i >= 0; i--) {
        arguments[i] = PyValue_NULL;
    }
    return res;
}

/* Precise: the span holds the dict while the keys are hashed. */
Py_GC_PRECISE Py_NO_INLINE PyObject *
_Py_BuildMap_Values(
    PyValue *arguments,
    int half_args)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    PyObject *res = _PyDict_NewPresized(half_args);
    if (res != NULL) {
        _PyValueSpan_AppendObject(&span, res);
    }
    for (int i = 0; res != NULL && i < half_args; i++) {
        if (_PyDict_SetItemValue((PyDictObject *)res, arguments[2 * i],
                                 arguments[2 * i + 1]) < 0)
        {
            res = NULL;
        }
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    assert((res != NULL) ^ (PyErr_Occurred() != NULL));
    // arguments is a pointer into the GC visible stack,
    // so we must NULL out values as we clear them.
    for (int i = half_args*2-1; i >= 0; i--) {
        arguments[i] = PyValue_NULL;
    }
    return res;
}

Py_GC_PRECISE Py_NO_INLINE PyValue
_Py_LoadAttr_Values(
    PyThreadState *tstate, PyValue owner,
    PyObject *name, PyValue *self_or_null)
{
    // Use value roots to ensure that both method and self are visible to
    // the GC: the handler that calls this keeps self_or_null in a C
    // variable until it writes it to its slot.
    _PyValueRoot method, self;
    _PyThreadState_PushValueRoot(tstate, &method);
    _PyThreadState_PushValueRoot(tstate, &self);
    self.value = owner;  // steal reference to owner
    // NOTE: method.value is initialized to PyValue_NULL and remains null on
    // error, so we don't need to explicitly use the return code from the call.
    _PyObject_GetMethodValue(tstate, &self.value, name, &method.value);
    *self_or_null = _PyThreadState_PopValueRoot(tstate, &self);
    return _PyThreadState_PopValueRoot(tstate, &method);
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


Py_GC_PRECISE Py_NO_INLINE PyValue
_PyEval_GetIter(PyValue iterable, PyValue *index_or_null, int yield_from)
{
    PyTypeObject *tp = PyValue_Type(iterable);
    if (tp->_tp_iteritem != NULL) {
        /* Leave iterable on stack and push tagged index 0 */
        *index_or_null = _PyValue_TagIndex(0);
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
            return _PyValue_ERROR;
        }
        return iterable;
    }
    /* Pop iterable, and push iterator then NULL. An int or float is not
       iterable; PyObject_GetIterValue() raises the error. */
    PyValue iter = PyObject_GetIterValue(iterable);
    if (PyValue_IsNull(iter)) {
        return _PyValue_ERROR;
    }
    return iter;
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

    return PyObject_SetItemValue(PyValue_FromObject(ns), PyValue_FromObject(name), v);
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

Py_GC_PRECISE Py_NO_INLINE PyValue _Py_HOT_FUNCTION DONT_SLP_VECTORIZE
_PyEval_EvalFrameDefault(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    check_invalid_reentrancy();
    CALL_STAT_INC(pyeval_calls);
    _PyGC_CHECK_NOT_SWEEPING(tstate, "runs Python code");

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

    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
        _PyEval_FrameClearAndPop(tstate, frame);
        return PyValue_NULL;
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
    entry.frame.f_executable = _PyValue_None;
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
#       include "generated_ir_cases.c.h"
#endif


early_exit:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame = dying->previous;
    _PyEval_FrameClearAndPop(tstate, dying);
    assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
    /* Restore previous frame and exit */
    tstate->current_frame = frame->previous;
    return PyValue_NULL;
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
        name_str = PyList_GET_ITEM(names, 0);
        Py_INCREF(name_str);
        break;
    case 2:
        name_str = PyUnicode_FromFormat("%U and %U",
                                        PyList_GET_ITEM(names, len - 2),
                                        PyList_GET_ITEM(names, len - 1));
        break;
    default:
        tail = PyUnicode_FromFormat(", %U, and %U",
                                    PyList_GET_ITEM(names, len - 2),
                                    PyList_GET_ITEM(names, len - 1));
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
            PyObject *raw = PyTuple_GET_ITEM(co->co_localsplusnames, i);
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
        PyObject *first_parameter_name = PyTuple_GET_ITEM(co->co_localsplusnames, 0);
        /* If the receiver parameter is already declared, another hint would be misleading. */
        if (PyUnicode_CompareWithASCIIString(first_parameter_name, "self") == 0 ||
            PyUnicode_CompareWithASCIIString(first_parameter_name, "cls") == 0)
        {
            return 0;
        }
    }
    /* If the current function matches on the type, it's likely worth adding the hint. */
    PyTypeObject *self_cls = Py_TYPE(first_argument);
    PyFunctionObject *possibly_current_function =
        (PyFunctionObject *)_PyType_Lookup(self_cls, co->co_name);
    return possibly_current_function == func;
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
        PyObject* posonly_name = PyTuple_GET_ITEM(co->co_localsplusnames, k);

        for (int k2=0; k2<kwcount; k2++){
            /* Compare the pointers first and fallback to PyObject_RichCompareBool*/
            PyObject* kwname = PyTuple_GET_ITEM(kwnames, k2);
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
initialize_locals(PyThreadState *tstate, PyFunctionObject *func,
    PyValue *localsplus, PyValue const *args,
    Py_ssize_t argcount, PyObject *kwnames)
{
    PyCodeObject *co = (PyCodeObject*)func->func_code;
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
        localsplus[i] = PyValue_FromObject(kwdict);
    }
    else {
        kwdict = NULL;
    }

    /* For the missing self hint, which an int or float never gets: its type
       has no Python methods. */
    if (argcount > co->co_argcount && !(co->co_flags & CO_VARARGS)
        && !PyValue_IsInline(args[0]))
    {
        first_argument = PyValue_ObjectBorrow(args[0]);
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
            u = _PyTuple_FromValues(args + n, argcount - n);
            if (u == NULL) {
                for (Py_ssize_t i = n; i < argcount; i++) {
                }
            }
        }
        if (u == NULL) {
            goto fail_post_positional;
        }
        assert(PyValue_IsNull(localsplus[total_args]));
        localsplus[total_args] = PyValue_FromObject(u);
    }
    else if (argcount > n) {
        /* Too many positional args. Error is reported later */
        for (j = n; j < argcount; j++) {
        }
    }
    _PyGC_INJECT(tstate, _PyGC_SITE_INIT_LOCALS);

    /* Handle keyword arguments */
    if (kwnames != NULL) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (i = 0; i < kwcount; i++) {
            PyObject **co_varnames;
            PyObject *keyword = PyTuple_GET_ITEM(kwnames, i);
            PyValue arg_value = args[i+argcount];
            Py_ssize_t j;

            if (keyword == NULL || !PyUnicode_Check(keyword)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%U() keywords must be strings",
                              FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_qualname));
                goto kw_fail;
            }

            /* Speed hack: do raw pointer compares. As names are
            normally interned this should almost always hit. */
            co_varnames = _PyTuple_HEAP_ITEMS(co->co_localsplusnames);
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = co_varnames[j];
                if (varname == keyword) {
                    goto kw_found;
                }
            }

            /* Slow fallback, just in case */
            for (j = co->co_posonlyargcount; j < total_args; j++) {
                PyObject *varname = co_varnames[j];
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
                            PyList_SET_ITEM(possible_keywords, k - co->co_posonlyargcount, co_varnames[k]);
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

            int err = _PyDict_SetItemValue((PyDictObject *)kwdict,
                                           PyValue_FromObject(keyword),
                                           arg_value);
            if (err == -1) {
                goto kw_fail;
            }
            continue;

        kw_fail:
            for (;i < kwcount; i++) {
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
            localsplus[j] = arg_value;
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
            for (; i < defcount; i++) {
                if (PyValue_IsNull(localsplus[m+i])) {
                    localsplus[m+i] = PyTuple_GET_VALUE(func->func_defaults, i);
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
            PyObject *varname = PyTuple_GET_ITEM(co->co_localsplusnames, i);
            if (func->func_kwdefaults != NULL) {
                PyValue def;
                if (_PyDict_GetItemValue(func->func_kwdefaults,
                                         PyValue_FromObject(varname), &def) < 0) {
                    goto fail_post_args;
                }
                if (!PyValue_IsNull(def)) {
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
    }
    /* fall through */
fail_post_positional:
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (j = argcount; j < argcount+kwcount; j++) {
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
    frame->f_executable = PyValue_NULL;
    _PyThreadState_PopFrame(tstate, frame);
}

static void
clear_gen_frame(PyThreadState *tstate, _PyInterpreterFrame * frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
    // Unlinked, the frame no longer leads the collector to its generator.
    _PyValueRoot gen_root;
    _PyThreadState_PushValueRoot(tstate, &gen_root);
    gen_root.object = (PyObject *)gen;
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
    _PyThreadState_PopValueRoot(tstate, &gen_root);
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
    int opcode = _PyIR_BaseOpcode(_PyCode_InstrHandler(co, i));
    assert(opcode == SEND || opcode == FOR_ITER);
    if (opcode == FOR_ITER) {
        // dst, the frame's return slot, may be the loop target, a local,
        // which takes only the values the generator yields.  The return
        // value goes to the slot after the window of the iterator and its
        // NULL, where the value of an exhausted iterator would be pushed:
        // the END_FOR at the exit consumes that slot.
        frame->return_slot = (uint16_t)(IR_B(*frame->instr_ptr) + 2);
    }
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

/* Consumes references to func, locals and all the args */
_PyInterpreterFrame *
_PyEvalFramePushAndInit(PyThreadState *tstate, PyValue func,
                        PyObject *locals, PyValue const* args,
                        size_t argcount, PyObject *kwnames, _PyInterpreterFrame *previous)
{
    PyFunctionObject *func_obj = (PyFunctionObject *)PyValue_ObjectBorrow(func);
    PyCodeObject * code = (PyCodeObject *)func_obj->func_code;
    CALL_STAT_INC(frames_pushed);
    if (_PyCode_GetExec(code) == NULL) {
        goto fail;
    }
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
    if (frame == NULL) {
        goto fail;
    }
    _PyFrame_Initialize(tstate, frame, func, locals, code, frame->base, 0, previous);
    if (initialize_locals(tstate, func_obj, frame->base, args, argcount, kwnames)) {
        assert(frame->owner == FRAME_OWNED_BY_THREAD);
        _PyThreadState_UpdateLastProfiledFrame(tstate, frame, tstate->current_frame);
        clear_thread_frame(tstate, frame);
        return NULL;
    }
    return frame;
fail:
    /* Consume the references */
    Py_XDECREF(locals);
    for (size_t i = 0; i < argcount; i++) {
    }
    if (kwnames) {
        Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
        for (Py_ssize_t i = 0; i < kwcount; i++) {
        }
    }
    PyErr_NoMemory();
    return NULL;
}

/* Same as _PyEvalFramePushAndInit but takes an args tuple and kwargs dict.
   Steals references to func, callargs and kwargs.
   Opaque, and so scanned, until _PyStack_UnpackDict() returns its names in
   a span, like vectorcall_call_dict() in Objects/call.c (10a-2.md). */
Py_NO_INLINE _PyInterpreterFrame *
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, PyValue func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs, _PyInterpreterFrame *previous)
{
    bool has_dict = (kwargs != NULL && PyDict_GET_SIZE(kwargs) > 0);
    PyObject *kwnames = NULL;
    const PyValue *newargs = _PyTuple_ITEMS(callargs);
    if (has_dict) {
        newargs = _PyStack_UnpackDict(tstate, newargs, nargs, kwargs, &kwnames);
        if (newargs == NULL) {
            goto error;
        }
    }
    _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit(
        tstate, func, locals,
        newargs, nargs, kwnames, previous
    );
    if (has_dict) {
        _PyStack_UnpackDict_Free(newargs, nargs, kwnames);
    }
    /* No need to decref func here because the reference has been stolen by
       _PyEvalFramePushAndInit.
    */
    Py_DECREF(callargs);
    Py_XDECREF(kwargs);
    return new_frame;
error:
    Py_DECREF(callargs);
    Py_XDECREF(kwargs);
    return NULL;
}

PyValue
_PyEval_Vector(PyThreadState *tstate, PyFunctionObject *func,
               PyObject *locals,
               const PyValue *args, size_t argcount,
               PyObject *kwnames)
{
    /* _PyEvalFramePushAndInit copies the arguments */
    Py_XINCREF(locals);
    _PyInterpreterFrame *frame = _PyEvalFramePushAndInit(
        tstate, PyValue_FromObject(func), locals,
        args, argcount, kwnames, NULL);
    if (frame == NULL) {
        return PyValue_NULL;
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
    PyFunctionObject *func = NULL;
    // The canonical values of the arguments; args and kws keep them alive.
    PyValue *newargs = PyMem_Malloc(sizeof(PyValue) * (kwcount + argcount + 1));
    if (newargs == NULL) {
        PyErr_NoMemory();
        goto fail;
    }
    for (int i = 0; i < argcount; i++) {
        newargs[i] = PyValue_FromObject(args[i]);
    }
    if (kwcount != 0) {
        kwnames = PyTuple_New(kwcount);
        if (kwnames == NULL) {
            goto fail;
        }
        for (int i = 0; i < kwcount; i++) {
            PyTuple_SET_ITEM(kwnames, i, Py_NewRef(kws[2*i]));
            newargs[argcount+i] = PyValue_FromObject(kws[2*i+1]);
        }
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
    res = PyValue_Object(_PyEval_Vector(tstate, func, locals,
                                        newargs, argcount, kwnames));
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

Py_NO_INLINE int
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

        *match = Py_NewRef(PyTuple_GET_ITEM(pair, 0));
        *rest = Py_NewRef(PyTuple_GET_ITEM(pair, 1));
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

Py_NO_INLINE int
_PyEval_UnpackIterableValues(PyThreadState *tstate, PyObject *v,
                       int argcnt, int argcntafter, PyValue *sp)
{
    int i = 0, j = 0;
    Py_ssize_t ll = 0;
    PyObject *it;  /* iter(v) */
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
        PyValue item;
        if (PyIter_NextValue(PyValue_FromObject(it), &item) <= 0) {
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
        *--sp = item;
    }

    if (argcntafter == -1) {
        /* We better have exhausted the iterator now. */
        PyValue w;
        int status = PyIter_NextValue(PyValue_FromObject(it), &w);
        if (status < 0) {
            goto Error;
        }
        if (status == 0) {
            return 1;
        }

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
    *--sp = PyValue_FromObject(l);
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
        *--sp = PyList_GET_VALUE(l, ll - j);
    }
    /* Resize the list. */
    Py_SET_SIZE(l, ll - argcntafter);
    Py_DECREF(it);
    return 1;

Error:
    /* The slots that the values went to are temporaries, which hold NULL
       unless live */
    for (; i > 0; i--, sp++) {
        *sp = PyValue_NULL;
    }
    Py_XDECREF(it);
    return 0;
}



Py_GC_PRECISE Py_NO_INLINE void
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
    return PyUnicode_AsUTF8(f->func_name);
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
        *istart = PyValue_Int32(start);
    }
    else if (_PyValue_IsNone(start)) {
        *istart = 0;
    }
    else {
        return false;
    }
    if (PyValue_IsInlineInt(stop)) {
        *istop = PyValue_Int32(stop);
    }
    else if (_PyValue_IsNone(stop)) {
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
        *box = PyValue_Object(ref);
        return *box;
    }
    *box = NULL;
    return PyValue_ObjectBorrow(ref);
}

// container[start:stop] for a list, tuple or str whose bounds are not both
// inline ints or None, which these helpers take as objects.
static PyObject *
binary_slice_boxed(PyObject *(*slice)(PyObject *, PyObject *, PyObject *),
                   PyObject *container, PyValue start_st, PyValue stop_st)
{
    PyObject *start_box, *stop_box = NULL, *res = NULL;
    PyObject *start_o = borrow_or_box(start_st, &start_box);
    if (start_o == NULL) {
        goto done;
    }
    PyObject *stop_o = borrow_or_box(stop_st, &stop_box);
    if (stop_o == NULL) {
        goto done;
    }
    res = slice(container, start_o, stop_o);
done:
    Py_XDECREF(start_box);
    Py_XDECREF(stop_box);
    return res;
}

// container[start:stop], for BINARY_SLICE.
Py_NO_INLINE PyObject *
_PyEval_BinarySlice(PyValue container_st, PyValue start_st, PyValue stop_st)
{
    Py_ssize_t start, stop;
    if (_PyValue_IsPointerSpace(container_st) &&
        inline_slice_bounds(start_st, stop_st, &start, &stop))
    {
        PyObject *container = PyValue_ObjectBorrow(container_st);
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
    if (!PyValue_IsInline(container_st)) {
        PyObject *container = PyValue_ObjectBorrow(container_st);
        if (PyList_CheckExact(container)) {
            return binary_slice_boxed(_PyList_BinarySlice, container,
                                      start_st, stop_st);
        }
        if (PyTuple_CheckExact(container)) {
            return binary_slice_boxed(_PyTuple_BinarySlice, container,
                                      start_st, stop_st);
        }
        if (PyUnicode_CheckExact(container)) {
            return binary_slice_boxed(_PyUnicode_BinarySlice, container,
                                      start_st, stop_st);
        }
    }
    // A slice holds values, so its bounds are not boxed.
    PyObject *slice = _PySlice_FromValues(start_st, stop_st, PyValue_NULL);
    if (slice == NULL) {
        return NULL;
    }
    // This returns an object: an inline item is boxed.
    return PyValue_Object(PyObject_GetItemValue(container_st,
                                                PyValue_FromObject(slice)));
}

// container[start:stop] = v, for STORE_SLICE. Return 0, or -1 with an
// exception set.
Py_NO_INLINE int
_PyEval_StoreSlice(PyValue v_st, PyValue container_st,
                   PyValue start_st, PyValue stop_st)
{
    Py_ssize_t start, stop;
    if (_PyValue_IsPointerSpace(v_st) && _PyValue_IsPointerSpace(container_st) &&
        PyList_CheckExact(PyValue_ObjectBorrow(container_st)) &&
        inline_slice_bounds(start_st, stop_st, &start, &stop))
    {
        return _PyList_StoreSlice(PyValue_ObjectBorrow(container_st), start, stop,
                                  PyValue_ObjectBorrow(v_st));
    }
    // A slice holds values, so its bounds are not boxed.
    PyObject *slice = _PySlice_FromValues(start_st, stop_st, PyValue_NULL);
    if (slice == NULL) {
        return -1;
    }
    return PyObject_SetItemValue(container_st, PyValue_FromObject(slice),
                                 v_st);
}

/* isinstance(inst, cls), for CALL_ISINSTANCE when inst or cls may be inline.
   An int or float has no __class__ of its own, so against an exact type, or
   a tuple of them, the answer is that of PyObject_IsInstance(), whose
   object_isinstance() then tests the subtype relation alone. */
Py_NO_INLINE int
_PyEval_IsInstanceValue(PyValue inst_st, PyValue cls_st)
{
    if (PyValue_IsInline(inst_st) && _PyValue_IsPointerSpace(cls_st)) {
        PyTypeObject *type = PyValue_Type(inst_st);
        PyObject *cls = PyValue_ObjectBorrow(cls_st);
        if (PyType_CheckExact(cls)) {
            return PyType_IsSubtype(type, (PyTypeObject *)cls);
        }
        if (PyTuple_CheckExact(cls)) {
            Py_ssize_t n = PyTuple_GET_SIZE(cls);
            Py_ssize_t i = 0;
            for (; i < n && PyType_CheckExact(PyTuple_GET_ITEM(cls, i)); i++) {
                if (PyType_IsSubtype(type, (PyTypeObject *)PyTuple_GET_ITEM(cls, i))) {
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

Py_NO_INLINE PyObject *
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

Py_NO_INLINE PyObject *
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

Py_NO_INLINE PyObject *
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

Py_NO_INLINE int
_PyEval_CheckExceptTypeValid(PyThreadState *tstate, PyObject* right)
{
    if (PyTuple_Check(right)) {
        Py_ssize_t i, length;
        length = PyTuple_GET_SIZE(right);
        for (i = 0; i < length; i++) {
            PyObject *exc = PyTuple_GET_ITEM(right, i);
            if (!PyExceptionClass_Check(exc)) {
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

Py_NO_INLINE int
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
            PyObject *exc = PyTuple_GET_ITEM(right, i);
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

Py_GC_PRECISE Py_NO_INLINE int
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

Py_NO_INLINE void
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

Py_NO_INLINE void
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

Py_NO_INLINE void
_PyEval_FormatExcUnbound(PyThreadState *tstate, PyCodeObject *co, int oparg)
{
    PyObject *name;
    /* Don't stomp existing exception */
    if (_PyErr_Occurred(tstate))
        return;
    name = PyTuple_GET_ITEM(co->co_localsplusnames, oparg);
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


    if (interp->co_extra_user_count >= MAX_CO_EXTRA_USERS - 1) {
        return -1;
    }

    new_index = interp->co_extra_user_count;
    interp->co_extra_freefuncs[new_index] = free;

    // Publish freefuncs[new_index] before making the index visible.
    FT_ATOMIC_STORE_SSIZE_RELEASE(interp->co_extra_user_count, new_index + 1);

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

/* Precise: the span holds the __anext__ result while it is made awaitable. */
Py_GC_PRECISE Py_NO_INLINE PyObject *
_PyEval_GetANext(PyObject *aiter)
{
    PyTypeObject *type = Py_TYPE(aiter);
    if (PyAsyncGen_CheckExact(aiter)) {
        return type->tp_as_async->am_anext(aiter);
    }
    if (type->tp_as_async == NULL || type->tp_as_async->am_anext == NULL) {
        PyErr_Format(PyExc_TypeError,
                        "'async for' requires an iterator with "
                        "__anext__ method, got %.100s",
                        type->tp_name);
        return NULL;
    }
    PyObject *next_iter = type->tp_as_async->am_anext(aiter);
    if (next_iter == NULL) {
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, next_iter);
    PyObject *awaitable = _PyCoro_GetAwaitableIter(next_iter);
    if (awaitable == NULL) {
        _PyErr_FormatFromCause(
            PyExc_TypeError,
            "'async for' received an invalid object "
            "from __anext__: %.100s",
            Py_TYPE(next_iter)->tp_name);
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    Py_DECREF(next_iter);
    return awaitable;
}

Py_NO_INLINE void
_PyEval_LoadGlobalValue(PyObject *globals, PyObject *builtins, PyObject *name, PyValue *writeto)
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
        *writeto = PyValue_FromObject(res);
    }

    if (!PyValue_IsNull(*writeto) && PyValue_Type(*writeto) == &PyLazyImport_Type) {
        PyObject *res_o = PyValue_ObjectBorrow(*writeto);
        PyObject *l_v = _PyLazyImport_Reify(
            PyThreadState_GET(), res_o, name, namespace);
        *writeto = l_v == NULL ? PyValue_NULL : PyValue_FromObject(l_v);
    }
}

/* Precise: the span holds iterable, whose type the error names
   (the parameter rule). */
Py_GC_PRECISE Py_NO_INLINE PyObject *
_PyEval_GetAwaitable(PyObject *iterable, int oparg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, iterable);
    PyObject *iter = _PyCoro_GetAwaitableIter(iterable);

    if (iter == NULL) {
        _PyEval_FormatAwaitableError(tstate, Py_TYPE(iterable), oparg);
    }
    else if (PyCoro_CheckExact(iter)) {
        PyCoroObject *coro = (PyCoroObject *)iter;
        int8_t frame_state = FT_ATOMIC_LOAD_INT8_RELAXED(coro->cr_frame_state);
        if (frame_state == FRAME_SUSPENDED_YIELD_FROM ||
            frame_state == FRAME_SUSPENDED_YIELD_FROM_LOCKED)
        {
            /* `iter` is a coroutine object that is being awaited. */
            Py_CLEAR(iter);
            _PyErr_SetString(tstate, PyExc_RuntimeError,
                             "coroutine is being awaited already");
        }
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return iter;
}

Py_NO_INLINE /* namespace[name] as a value: 1, 0 if it is missing, or -1 with an
   exception set.  A dict gives the value with no box. */
static int
namespace_lookup(PyObject *namespace, PyObject *name, PyValue *value)
{
    if (PyDict_CheckExact(namespace)) {
        return _PyDict_GetItemValue(namespace, PyValue_FromObject(name), value);
    }
    PyObject *value_o;
    int found = PyMapping_GetOptionalItem(namespace, name, &value_o);
    *value = found > 0 ? PyValue_FromObject(value_o) : PyValue_NULL;
    return found;
}

PyValue
_PyEval_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject *name)
{
    PyValue value;
    PyObject *namespace = frame->f_locals;
    if (namespace == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                            "no locals found");
        return PyValue_NULL;
    }
    int found = namespace_lookup(namespace, name, &value);
    if (found == 0) {
        namespace = frame->f_globals;
        found = _PyDict_GetItemValue(namespace, PyValue_FromObject(name), &value);
    }
    if (found == 0) {
        namespace = frame->f_builtins;
        found = namespace_lookup(namespace, name, &value);
        if (found == 0) {
            _PyEval_FormatExcCheckArg(
                        tstate, PyExc_NameError,
                                NAME_ERROR_MSG, name);
        }
    }
    if (found <= 0) {
        return PyValue_NULL;
    }
    if (PyValue_IsObject(value) && PyLazyImport_CheckExact(PyValue_ObjectBorrow(value))) {
        PyObject *res = _PyLazyImport_Reify(tstate, PyValue_ObjectBorrow(value),
                                            name, namespace);
        return res == NULL ? PyValue_NULL : PyValue_FromObject(res);
    }
    return value;
}

PyValue _PyForIter_VirtualIteratorNext(PyThreadState* tstate, _PyInterpreterFrame* frame, PyValue iter, PyValue* index_ptr)
{
    PyObject *iter_o = PyValue_ObjectBorrow(iter);
    PyValue index = *index_ptr;
    if (_PyValue_IsTaggedIndex(index)) {
        intptr_t i = _PyValue_UntagIndex(index);
        assert(i >= 0);
        _PyValueIndexPair next_index = Py_TYPE(iter_o)->_tp_iteritem(iter_o, i);
        i = next_index.index;
        if (PyValue_IsNull(next_index.value)) {
            return i < 0 ? _PyValue_ERROR : PyValue_NULL;
        }
        *index_ptr = _PyValue_TagIndex(i);
        return next_index.value;
    }
    PyValue next = (*Py_TYPE(iter_o)->tp_iternext)(iter_o);
    if (PyValue_IsNull(next)) {
        if (_PyErr_Occurred(tstate)) {
            if (_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                _PyEval_MonitorRaise(tstate, frame, frame->instr_ptr);
                _PyErr_Clear(tstate);
            }
            else {
                return _PyValue_ERROR;
            }
        }
        return PyValue_NULL;
    }
    return next;
}

/* Check if a 'cls' provides the given special method. */
static inline int
type_has_special_method(PyTypeObject *cls, PyObject *name)
{
    // _PyType_Lookup() does not set an exception and returns a borrowed ref
    assert(!PyErr_Occurred());
    PyObject *r = _PyType_Lookup(cls, name);
    return r != NULL && Py_TYPE(r)->tp_descr_get != NULL;
}

Py_NO_INLINE int
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
