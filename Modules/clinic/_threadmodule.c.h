/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

static PyObject *
_thread__ThreadHandle_ident_get_impl(PyObject *self);

static PyObject *
_thread__ThreadHandle_ident_get(PyObject *self, void *Py_UNUSED(context))
{
    return _thread__ThreadHandle_ident_get_impl(self);
}

PyDoc_STRVAR(_thread__ThreadHandle_join__doc__,
"join($self, timeout=None, /)\n"
"--\n"
"\n");

#define _THREAD__THREADHANDLE_JOIN_METHODDEF    \
    {"join", _PyCFunction_CAST(_thread__ThreadHandle_join), METH_VALUE, _thread__ThreadHandle_join__doc__},

static PyValue
_thread__ThreadHandle_join_impl(PyObject *self, PyValueBorrow timeout_obj);

static PyValue
_thread__ThreadHandle_join(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("join", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    timeout_obj = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _thread__ThreadHandle_join_impl(PyValue_AsKnownObjectBorrow(self), timeout_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread__ThreadHandle_is_done__doc__,
"is_done($self, /)\n"
"--\n"
"\n");

#define _THREAD__THREADHANDLE_IS_DONE_METHODDEF    \
    {"is_done", _PyCFunction_CAST(_thread__ThreadHandle_is_done), METH_VALUE|METH_NOARGS, _thread__ThreadHandle_is_done__doc__},

static PyValue
_thread__ThreadHandle_is_done_impl(PyObject *self);

static PyValue
_thread__ThreadHandle_is_done(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__ThreadHandle_is_done_impl(PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread__ThreadHandle__set_done__doc__,
"_set_done($self, /)\n"
"--\n"
"\n");

#define _THREAD__THREADHANDLE__SET_DONE_METHODDEF    \
    {"_set_done", _PyCFunction_CAST(_thread__ThreadHandle__set_done), METH_VALUE|METH_NOARGS, _thread__ThreadHandle__set_done__doc__},

static PyValue
_thread__ThreadHandle__set_done_impl(PyObject *self);

static PyValue
_thread__ThreadHandle__set_done(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__ThreadHandle__set_done_impl(PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_lock_acquire__doc__,
"acquire($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"Lock the lock.\n"
"\n"
"Without argument, this blocks if the lock is already locked\n"
"(even by the same thread), waiting for another thread to release\n"
"the lock, and return True once the lock is acquired.\n"
"With an argument, this will only block if the argument is true,\n"
"and the return value reflects whether the lock is acquired.\n"
"The blocking operation is interruptible.");

#define _THREAD_LOCK_ACQUIRE_METHODDEF    \
    {"acquire", _PyCFunction_CAST(_thread_lock_acquire), METH_VALUE|METH_KEYWORDS, _thread_lock_acquire__doc__},

static PyValue
_thread_lock_acquire_impl(lockobject *self, int blocking,
                          PyValueBorrow timeoutobj);

static PyValue
_thread_lock_acquire(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(blocking)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int blocking = 1;
    PyValueBorrow timeoutobj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        blocking = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (blocking < 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        timeoutobj = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _thread_lock_acquire_impl((lockobject *)PyValue_AsKnownObjectBorrow(self), blocking, timeoutobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_lock_acquire_lock__doc__,
"acquire_lock($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"An obsolete synonym of acquire().");

#define _THREAD_LOCK_ACQUIRE_LOCK_METHODDEF    \
    {"acquire_lock", _PyCFunction_CAST(_thread_lock_acquire_lock), METH_VALUE|METH_KEYWORDS, _thread_lock_acquire_lock__doc__},

static PyValue
_thread_lock_acquire_lock_impl(lockobject *self, int blocking,
                               PyValueBorrow timeoutobj);

static PyValue
_thread_lock_acquire_lock(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(blocking)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire_lock",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int blocking = 1;
    PyValueBorrow timeoutobj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        blocking = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (blocking < 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        timeoutobj = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _thread_lock_acquire_lock_impl((lockobject *)PyValue_AsKnownObjectBorrow(self), blocking, timeoutobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_lock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the lock.\n"
"\n"
"Allows another thread that is blocked waiting for\n"
"the lock to acquire the lock.  The lock must be in the locked state,\n"
"but it needn\'t be locked by the same thread that unlocks it.");

#define _THREAD_LOCK_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_thread_lock_release), METH_VALUE|METH_NOARGS, _thread_lock_release__doc__},

static PyValue
_thread_lock_release_impl(lockobject *self);

static PyValue
_thread_lock_release(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_lock_release_impl((lockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_lock_release_lock__doc__,
"release_lock($self, /)\n"
"--\n"
"\n"
"An obsolete synonym of release().");

#define _THREAD_LOCK_RELEASE_LOCK_METHODDEF    \
    {"release_lock", _PyCFunction_CAST(_thread_lock_release_lock), METH_VALUE|METH_NOARGS, _thread_lock_release_lock__doc__},

static PyValue
_thread_lock_release_lock_impl(lockobject *self);

static PyValue
_thread_lock_release_lock(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_lock_release_lock_impl((lockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_lock___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Lock the lock.");

#define _THREAD_LOCK___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(_thread_lock___enter__), METH_VALUE|METH_NOARGS, _thread_lock___enter____doc__},

static PyValue
_thread_lock___enter___impl(lockobject *self);

static PyValue
_thread_lock___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_lock___enter___impl((lockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_lock___exit____doc__,
"__exit__($self, exc_type, exc_value, exc_tb, /)\n"
"--\n"
"\n"
"Release the lock.");

#define _THREAD_LOCK___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_thread_lock___exit__), METH_VALUE, _thread_lock___exit____doc__},

static PyValue
_thread_lock___exit___impl(lockobject *self, PyValueBorrow exc_type,
                           PyValueBorrow exc_value, PyValueBorrow exc_tb);

static PyValue
_thread_lock___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exc_type;
    PyValueBorrow exc_value;
    PyValueBorrow exc_tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = PyValue_Borrow(args[0]);
    exc_value = PyValue_Borrow(args[1]);
    exc_tb = PyValue_Borrow(args[2]);
    return_value = _thread_lock___exit___impl((lockobject *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_lock_locked__doc__,
"locked($self, /)\n"
"--\n"
"\n"
"Return whether the lock is in the locked state.");

#define _THREAD_LOCK_LOCKED_METHODDEF    \
    {"locked", _PyCFunction_CAST(_thread_lock_locked), METH_VALUE|METH_NOARGS, _thread_lock_locked__doc__},

static PyValue
_thread_lock_locked_impl(lockobject *self);

static PyValue
_thread_lock_locked(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_lock_locked_impl((lockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_lock_locked_lock__doc__,
"locked_lock($self, /)\n"
"--\n"
"\n"
"An obsolete synonym of locked().");

#define _THREAD_LOCK_LOCKED_LOCK_METHODDEF    \
    {"locked_lock", _PyCFunction_CAST(_thread_lock_locked_lock), METH_VALUE|METH_NOARGS, _thread_lock_locked_lock__doc__},

static PyValue
_thread_lock_locked_lock_impl(lockobject *self);

static PyValue
_thread_lock_locked_lock(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_lock_locked_lock_impl((lockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#if defined(HAVE_FORK)

PyDoc_STRVAR(_thread_lock__at_fork_reinit__doc__,
"_at_fork_reinit($self, /)\n"
"--\n"
"\n");

#define _THREAD_LOCK__AT_FORK_REINIT_METHODDEF    \
    {"_at_fork_reinit", _PyCFunction_CAST(_thread_lock__at_fork_reinit), METH_VALUE|METH_NOARGS, _thread_lock__at_fork_reinit__doc__},

static PyValue
_thread_lock__at_fork_reinit_impl(lockobject *self);

static PyValue
_thread_lock__at_fork_reinit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_lock__at_fork_reinit_impl((lockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_FORK) */

static PyValue
lock_new_impl(PyTypeObject *type);

static PyValue
lock_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->lock_type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("lock", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("lock", kwnames)) {
        goto exit;
    }
    return_value = lock_new_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
lock_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(lock_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = lock_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_thread_RLock_acquire__doc__,
"acquire($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"Lock the lock.\n"
"\n"
"`blocking` indicates whether we should wait\n"
"for the lock to be available or not.  If `blocking` is False\n"
"and another thread holds the lock, the method will return False\n"
"immediately.  If `blocking` is True and another thread holds\n"
"the lock, the method will wait for the lock to be released,\n"
"take it and then return True.\n"
"(note: the blocking operation is interruptible.)\n"
"\n"
"In all other cases, the method will return True immediately.\n"
"Precisely, if the current thread already holds the lock, its\n"
"internal counter is simply incremented. If nobody holds the lock,\n"
"the lock is taken and its internal counter initialized to 1.");

#define _THREAD_RLOCK_ACQUIRE_METHODDEF    \
    {"acquire", _PyCFunction_CAST(_thread_RLock_acquire), METH_VALUE|METH_KEYWORDS, _thread_RLock_acquire__doc__},

static PyValue
_thread_RLock_acquire_impl(rlockobject *self, int blocking,
                           PyValueBorrow timeoutobj);

static PyValue
_thread_RLock_acquire(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(blocking)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int blocking = 1;
    PyValueBorrow timeoutobj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        blocking = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (blocking < 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        timeoutobj = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _thread_RLock_acquire_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self), blocking, timeoutobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_RLock___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Lock the lock.");

#define _THREAD_RLOCK___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(_thread_RLock___enter__), METH_VALUE|METH_NOARGS, _thread_RLock___enter____doc__},

static PyValue
_thread_RLock___enter___impl(rlockobject *self);

static PyValue
_thread_RLock___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock___enter___impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_RLock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the lock.\n"
"\n"
"Allows another thread that is blocked waiting for the lock\n"
"to acquire the lock.  The lock must be in the locked state,\n"
"and must be locked by the same thread that unlocks it; otherwise a\n"
"`RuntimeError` is raised.\n"
"\n"
"Do note that if the lock was acquire()d several times in a row by\n"
"the current thread, release() needs to be called as many times for\n"
"the lock to be available for other threads.");

#define _THREAD_RLOCK_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_thread_RLock_release), METH_VALUE|METH_NOARGS, _thread_RLock_release__doc__},

static PyValue
_thread_RLock_release_impl(rlockobject *self);

static PyValue
_thread_RLock_release(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock_release_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_RLock___exit____doc__,
"__exit__($self, exc_type, exc_value, exc_tb, /)\n"
"--\n"
"\n"
"Release the lock.");

#define _THREAD_RLOCK___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_thread_RLock___exit__), METH_VALUE, _thread_RLock___exit____doc__},

static PyValue
_thread_RLock___exit___impl(rlockobject *self, PyValueBorrow exc_type,
                            PyValueBorrow exc_value, PyValueBorrow exc_tb);

static PyValue
_thread_RLock___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exc_type;
    PyValueBorrow exc_value;
    PyValueBorrow exc_tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = PyValue_Borrow(args[0]);
    exc_value = PyValue_Borrow(args[1]);
    exc_tb = PyValue_Borrow(args[2]);
    return_value = _thread_RLock___exit___impl((rlockobject *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_RLock_locked__doc__,
"locked($self, /)\n"
"--\n"
"\n"
"Return a boolean indicating whether this object is locked right now.");

#define _THREAD_RLOCK_LOCKED_METHODDEF    \
    {"locked", _PyCFunction_CAST(_thread_RLock_locked), METH_VALUE|METH_NOARGS, _thread_RLock_locked__doc__},

static PyValue
_thread_RLock_locked_impl(rlockobject *self);

static PyValue
_thread_RLock_locked(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock_locked_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_RLock__acquire_restore__doc__,
"_acquire_restore($self, state, /)\n"
"--\n"
"\n"
"For internal use by `threading.Condition`.");

#define _THREAD_RLOCK__ACQUIRE_RESTORE_METHODDEF    \
    {"_acquire_restore", _PyCFunction_CAST(_thread_RLock__acquire_restore), METH_VALUE|METH_O, _thread_RLock__acquire_restore__doc__},

static PyValue
_thread_RLock__acquire_restore_impl(rlockobject *self, PyValueBorrow state);

static PyValue
_thread_RLock__acquire_restore(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    state = PyValue_Borrow(args[0]);
    return_value = _thread_RLock__acquire_restore_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self), state);

    return return_value;
}

PyDoc_STRVAR(_thread_RLock__release_save__doc__,
"_release_save($self, /)\n"
"--\n"
"\n"
"For internal use by `threading.Condition`.");

#define _THREAD_RLOCK__RELEASE_SAVE_METHODDEF    \
    {"_release_save", _PyCFunction_CAST(_thread_RLock__release_save), METH_VALUE|METH_NOARGS, _thread_RLock__release_save__doc__},

static PyValue
_thread_RLock__release_save_impl(rlockobject *self);

static PyValue
_thread_RLock__release_save(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock__release_save_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_RLock__recursion_count__doc__,
"_recursion_count($self, /)\n"
"--\n"
"\n"
"For internal use by reentrancy checks.");

#define _THREAD_RLOCK__RECURSION_COUNT_METHODDEF    \
    {"_recursion_count", _PyCFunction_CAST(_thread_RLock__recursion_count), METH_VALUE|METH_NOARGS, _thread_RLock__recursion_count__doc__},

static PyValue
_thread_RLock__recursion_count_impl(rlockobject *self);

static PyValue
_thread_RLock__recursion_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock__recursion_count_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_thread_RLock__is_owned__doc__,
"_is_owned($self, /)\n"
"--\n"
"\n"
"For internal use by `threading.Condition`.");

#define _THREAD_RLOCK__IS_OWNED_METHODDEF    \
    {"_is_owned", _PyCFunction_CAST(_thread_RLock__is_owned), METH_VALUE|METH_NOARGS, _thread_RLock__is_owned__doc__},

static PyValue
_thread_RLock__is_owned_impl(rlockobject *self);

static PyValue
_thread_RLock__is_owned(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock__is_owned_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

static PyValue
rlock_new_impl(PyTypeObject *type);

static PyValue
rlock_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->rlock_type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("RLock", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("RLock", kwnames)) {
        goto exit;
    }
    return_value = rlock_new_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
rlock_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(rlock_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = rlock_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

#if defined(HAVE_FORK)

PyDoc_STRVAR(_thread_RLock__at_fork_reinit__doc__,
"_at_fork_reinit($self, /)\n"
"--\n"
"\n");

#define _THREAD_RLOCK__AT_FORK_REINIT_METHODDEF    \
    {"_at_fork_reinit", _PyCFunction_CAST(_thread_RLock__at_fork_reinit), METH_VALUE|METH_NOARGS, _thread_RLock__at_fork_reinit__doc__},

static PyValue
_thread_RLock__at_fork_reinit_impl(rlockobject *self);

static PyValue
_thread_RLock__at_fork_reinit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_RLock__at_fork_reinit_impl((rlockobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_FORK) */

PyDoc_STRVAR(_thread_daemon_threads_allowed__doc__,
"daemon_threads_allowed($module, /)\n"
"--\n"
"\n"
"Return True if daemon threads are allowed in the current interpreter.\n"
"\n"
"Return False otherwise.");

#define _THREAD_DAEMON_THREADS_ALLOWED_METHODDEF    \
    {"daemon_threads_allowed", _PyCFunction_CAST(_thread_daemon_threads_allowed), METH_VALUE|METH_NOARGS, _thread_daemon_threads_allowed__doc__},

static PyValue
_thread_daemon_threads_allowed_impl(PyObject *module);

static PyValue
_thread_daemon_threads_allowed(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_daemon_threads_allowed_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_thread_start_new_thread__doc__,
"start_new_thread($module, function, args, kwargs={}, /)\n"
"--\n"
"\n"
"Start a new thread and return its identifier.\n"
"\n"
"The thread will call the function with positional arguments from the\n"
"tuple args and keyword arguments taken from the optional dictionary\n"
"kwargs.  The thread exits when the function returns; the return value\n"
"is ignored.  The thread will also exit when the function raises an\n"
"unhandled exception; a stack trace will be printed unless the exception\n"
"is SystemExit.");

#define _THREAD_START_NEW_THREAD_METHODDEF    \
    {"start_new_thread", _PyCFunction_CAST(_thread_start_new_thread), METH_VALUE, _thread_start_new_thread__doc__},

static PyValue
_thread_start_new_thread_impl(PyObject *module, PyValueBorrow func,
                              PyObject *args, PyObject *kwargs);

static PyValue
_thread_start_new_thread(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow func;
    PyObject *__clinic_args;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("start_new_thread", nargs, 2, 3)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("start_new_thread", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    __clinic_args = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[2])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("start_new_thread", "argument 3", "dict", PyValue_Borrow(args[2]));
        goto exit;
    }
    __clinic_kwargs = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2]));
skip_optional:
    return_value = _thread_start_new_thread_impl(PyValue_AsKnownObjectBorrow(module), func, __clinic_args, __clinic_kwargs);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_start_joinable_thread__doc__,
"start_joinable_thread($module, /, function, handle=None, daemon=True)\n"
"--\n"
"\n"
"*For internal use only*: start a new thread.\n"
"\n"
"Like start_new_thread(), this starts a new thread calling the given\n"
"function.  Unlike start_new_thread(), this returns a handle object with\n"
"methods to join or detach the given thread.\n"
"This function is not for third-party code, please use the `threading`\n"
"module instead.  During finalization the runtime will not wait for the\n"
"thread to exit if daemon is True.  If handle is provided it must be a\n"
"newly created thread._ThreadHandle instance.");

#define _THREAD_START_JOINABLE_THREAD_METHODDEF    \
    {"start_joinable_thread", _PyCFunction_CAST(_thread_start_joinable_thread), METH_VALUE|METH_KEYWORDS, _thread_start_joinable_thread__doc__},

static PyValue
_thread_start_joinable_thread_impl(PyObject *module, PyValueBorrow func,
                                   PyValueBorrow hobj, int daemon);

static PyValue
_thread_start_joinable_thread(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(function)), _PyValue_STATIC_IMMORTAL(&_Py_ID(handle)), _PyValue_STATIC_IMMORTAL(&_Py_ID(daemon)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"function", "handle", "daemon", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "start_joinable_thread",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow func;
    PyValueBorrow hobj = PyValue_Borrow(PyValue_None);
    int daemon = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    func = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        hobj = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        daemon = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (daemon < 0) {
            goto exit;
        }
    }
    return_value = _thread_start_joinable_thread_impl(PyValue_AsKnownObjectBorrow(module), func, hobj, daemon);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_exit__doc__,
"exit($module, /)\n"
"--\n"
"\n"
"Raise SystemExit.\n"
"\n"
"It will cause the current thread to exit silently unless the exception\n"
"is caught.");

#define _THREAD_EXIT_METHODDEF    \
    {"exit", _PyCFunction_CAST(_thread_exit), METH_VALUE|METH_NOARGS, _thread_exit__doc__},

static PyValue
_thread_exit_impl(PyObject *module);

static PyValue
_thread_exit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_exit_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_thread_interrupt_main__doc__,
"interrupt_main($module, signum=signal.SIGINT, /)\n"
"--\n"
"\n"
"Simulate the arrival of the given signal in the main thread.\n"
"\n"
"The corresponding signal handler will be executed.\n"
"If *signum* is omitted, SIGINT is assumed.\n"
"A subthread can use this function to interrupt the main thread.\n"
"\n"
"Note: the default signal handler for SIGINT raises\n"
"``KeyboardInterrupt``.");

#define _THREAD_INTERRUPT_MAIN_METHODDEF    \
    {"interrupt_main", _PyCFunction_CAST(_thread_interrupt_main), METH_VALUE, _thread_interrupt_main__doc__},

static PyValue
_thread_interrupt_main_impl(PyObject *module, int signum);

static PyValue
_thread_interrupt_main(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int signum = SIGINT;

    if (!_PyArg_CheckPositional("interrupt_main", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    signum = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (signum == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _thread_interrupt_main_impl(PyValue_AsKnownObjectBorrow(module), signum);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_allocate_lock__doc__,
"allocate_lock($module, /)\n"
"--\n"
"\n"
"Create a new lock object.\n"
"\n"
"See help(type(threading.Lock())) for information about locks.");

#define _THREAD_ALLOCATE_LOCK_METHODDEF    \
    {"allocate_lock", _PyCFunction_CAST(_thread_allocate_lock), METH_VALUE|METH_NOARGS, _thread_allocate_lock__doc__},

static PyValue
_thread_allocate_lock_impl(PyObject *module);

static PyValue
_thread_allocate_lock(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_allocate_lock_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_thread_get_ident__doc__,
"get_ident($module, /)\n"
"--\n"
"\n"
"Return a non-zero integer that uniquely identifies the current thread.\n"
"\n"
"It is unique amongst other threads that exist simultaneously.\n"
"This may be used to identify per-thread resources.\n"
"Even though on some platforms threads identities may appear to be\n"
"allocated consecutive numbers starting at 1, this behavior should not\n"
"be relied upon, and the number should be seen purely as a magic cookie.\n"
"A thread\'s identity may be reused for another thread after it exits.");

#define _THREAD_GET_IDENT_METHODDEF    \
    {"get_ident", _PyCFunction_CAST(_thread_get_ident), METH_VALUE|METH_NOARGS, _thread_get_ident__doc__},

static PyValue
_thread_get_ident_impl(PyObject *module);

static PyValue
_thread_get_ident(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_get_ident_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(PY_HAVE_THREAD_NATIVE_ID)

PyDoc_STRVAR(_thread_get_native_id__doc__,
"get_native_id($module, /)\n"
"--\n"
"\n"
"Return a non-negative integer identifying the thread.\n"
"\n"
"It is reported by the OS (kernel).  This may be used to uniquely\n"
"identify a particular thread within a system.");

#define _THREAD_GET_NATIVE_ID_METHODDEF    \
    {"get_native_id", _PyCFunction_CAST(_thread_get_native_id), METH_VALUE|METH_NOARGS, _thread_get_native_id__doc__},

static PyValue
_thread_get_native_id_impl(PyObject *module);

static PyValue
_thread_get_native_id(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread_get_native_id_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(PY_HAVE_THREAD_NATIVE_ID) */

PyDoc_STRVAR(_thread__count__doc__,
"_count($module, /)\n"
"--\n"
"\n"
"Return the number of currently running Python threads.\n"
"\n"
"The main thread is excluded.  The returned number comprises all threads\n"
"created through `start_new_thread()` as well as `threading.Thread`, and\n"
"not yet finished.\n"
"\n"
"This function is meant for internal and specialized purposes only.\n"
"In most applications `threading.enumerate()` should be used instead.");

#define _THREAD__COUNT_METHODDEF    \
    {"_count", _PyCFunction_CAST(_thread__count), METH_VALUE|METH_NOARGS, _thread__count__doc__},

static PyValue
_thread__count_impl(PyObject *module);

static PyValue
_thread__count(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__count_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_thread_stack_size__doc__,
"stack_size($module, size=0, /)\n"
"--\n"
"\n"
"Return the thread stack size used when creating new threads.\n"
"\n"
"The optional size argument specifies the stack size (in bytes) to be\n"
"used for subsequently created threads, and must be 0 (use platform or\n"
"configured default) or a positive integer value of at least 32,768 (32\n"
"KiB).  If changing the thread stack size is unsupported, a ThreadError\n"
"exception is raised.  If the specified size is invalid, a ValueError\n"
"exception is raised, and the stack size is unmodified.  32 KiB\n"
"currently is the minimum supported stack size value to guarantee\n"
"sufficient stack space for the interpreter itself.\n"
"\n"
"Note that some systems may have particular restrictions on values for\n"
"the stack size, such as requiring a minimum stack size larger than 32\n"
"KiB or requiring allocation in multiples of the system memory page size\n"
"- platform documentation should be referred to for more information\n"
"(4 KiB pages are common; using multiples of 4096 for the stack size is\n"
"the suggested approach in the absence of more specific information).");

#define _THREAD_STACK_SIZE_METHODDEF    \
    {"stack_size", _PyCFunction_CAST(_thread_stack_size), METH_VALUE, _thread_stack_size__doc__},

static PyValue
_thread_stack_size_impl(PyObject *module, Py_ssize_t new_size);

static PyValue
_thread_stack_size(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t new_size = 0;

    if (!_PyArg_CheckPositional("stack_size", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        new_size = ival;
    }
skip_optional:
    return_value = _thread_stack_size_impl(PyValue_AsKnownObjectBorrow(module), new_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread__excepthook__doc__,
"_excepthook($module, args, /)\n"
"--\n"
"\n"
"Handle uncaught Thread.run() exception.");

#define _THREAD__EXCEPTHOOK_METHODDEF    \
    {"_excepthook", _PyCFunction_CAST(_thread__excepthook), METH_VALUE|METH_O, _thread__excepthook__doc__},

static PyValue
_thread__excepthook_impl(PyObject *module, PyValueBorrow args);

static PyValue
_thread__excepthook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow __clinic_args;

    __clinic_args = PyValue_Borrow(args[0]);
    return_value = _thread__excepthook_impl(PyValue_AsKnownObjectBorrow(module), __clinic_args);

    return return_value;
}

PyDoc_STRVAR(_thread__is_main_interpreter__doc__,
"_is_main_interpreter($module, /)\n"
"--\n"
"\n"
"Return True if the current interpreter is the main Python interpreter.");

#define _THREAD__IS_MAIN_INTERPRETER_METHODDEF    \
    {"_is_main_interpreter", _PyCFunction_CAST(_thread__is_main_interpreter), METH_VALUE|METH_NOARGS, _thread__is_main_interpreter__doc__},

static PyValue
_thread__is_main_interpreter_impl(PyObject *module);

static PyValue
_thread__is_main_interpreter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__is_main_interpreter_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_thread__shutdown__doc__,
"_shutdown($module, /)\n"
"--\n"
"\n"
"Wait for all non-daemon threads (other than the calling thread) to stop.");

#define _THREAD__SHUTDOWN_METHODDEF    \
    {"_shutdown", _PyCFunction_CAST(_thread__shutdown), METH_VALUE|METH_NOARGS, _thread__shutdown__doc__},

static PyValue
_thread__shutdown_impl(PyObject *module);

static PyValue
_thread__shutdown(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__shutdown_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_thread__make_thread_handle__doc__,
"_make_thread_handle($module, ident, /)\n"
"--\n"
"\n"
"Internal only.\n"
"\n"
"Make a thread handle for threads not spawned by the _thread or\n"
"threading module.");

#define _THREAD__MAKE_THREAD_HANDLE_METHODDEF    \
    {"_make_thread_handle", _PyCFunction_CAST(_thread__make_thread_handle), METH_VALUE|METH_O, _thread__make_thread_handle__doc__},

static PyValue
_thread__make_thread_handle_impl(PyObject *module, PyValueBorrow identobj);

static PyValue
_thread__make_thread_handle(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow identobj;

    identobj = PyValue_Borrow(args[0]);
    return_value = _thread__make_thread_handle_impl(PyValue_AsKnownObjectBorrow(module), identobj);

    return return_value;
}

PyDoc_STRVAR(_thread__get_main_thread_ident__doc__,
"_get_main_thread_ident($module, /)\n"
"--\n"
"\n"
"Internal only.\n"
"\n"
"Return a non-zero integer that uniquely identifies the main thread of\n"
"the main interpreter.");

#define _THREAD__GET_MAIN_THREAD_IDENT_METHODDEF    \
    {"_get_main_thread_ident", _PyCFunction_CAST(_thread__get_main_thread_ident), METH_VALUE|METH_NOARGS, _thread__get_main_thread_ident__doc__},

static PyValue
_thread__get_main_thread_ident_impl(PyObject *module);

static PyValue
_thread__get_main_thread_ident(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__get_main_thread_ident_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if (defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP) || defined(MS_WINDOWS))

PyDoc_STRVAR(_thread__get_name__doc__,
"_get_name($module, /)\n"
"--\n"
"\n"
"Get the name of the current thread.");

#define _THREAD__GET_NAME_METHODDEF    \
    {"_get_name", _PyCFunction_CAST(_thread__get_name), METH_VALUE|METH_NOARGS, _thread__get_name__doc__},

static PyValue
_thread__get_name_impl(PyObject *module);

static PyValue
_thread__get_name(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _thread__get_name_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* (defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP) || defined(MS_WINDOWS)) */

#if (defined(HAVE_PTHREAD_SETNAME_NP) || defined(HAVE_PTHREAD_SET_NAME_NP) || defined(MS_WINDOWS))

PyDoc_STRVAR(_thread_set_name__doc__,
"set_name($module, /, name)\n"
"--\n"
"\n"
"Set the name of the current thread.");

#define _THREAD_SET_NAME_METHODDEF    \
    {"set_name", _PyCFunction_CAST(_thread_set_name), METH_VALUE|METH_KEYWORDS, _thread_set_name__doc__},

static PyValue
_thread_set_name_impl(PyObject *module, PyObject *name_obj);

static PyValue
_thread_set_name(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_name",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyObject *name_obj;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("set_name", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("set_name", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    name_obj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    return_value = _thread_set_name_impl(PyValue_AsKnownObjectBorrow(module), name_obj);

exit:
    return return_value;
}

#endif /* (defined(HAVE_PTHREAD_SETNAME_NP) || defined(HAVE_PTHREAD_SET_NAME_NP) || defined(MS_WINDOWS)) */

#ifndef _THREAD_LOCK__AT_FORK_REINIT_METHODDEF
    #define _THREAD_LOCK__AT_FORK_REINIT_METHODDEF
#endif /* !defined(_THREAD_LOCK__AT_FORK_REINIT_METHODDEF) */

#ifndef _THREAD_RLOCK__AT_FORK_REINIT_METHODDEF
    #define _THREAD_RLOCK__AT_FORK_REINIT_METHODDEF
#endif /* !defined(_THREAD_RLOCK__AT_FORK_REINIT_METHODDEF) */

#ifndef _THREAD_GET_NATIVE_ID_METHODDEF
    #define _THREAD_GET_NATIVE_ID_METHODDEF
#endif /* !defined(_THREAD_GET_NATIVE_ID_METHODDEF) */

#ifndef _THREAD__GET_NAME_METHODDEF
    #define _THREAD__GET_NAME_METHODDEF
#endif /* !defined(_THREAD__GET_NAME_METHODDEF) */

#ifndef _THREAD_SET_NAME_METHODDEF
    #define _THREAD_SET_NAME_METHODDEF
#endif /* !defined(_THREAD_SET_NAME_METHODDEF) */
#define _THREAD__THREADHANDLE_IDENT_GETSETDEF {"ident", (getter)_thread__ThreadHandle_ident_get, (setter)NULL, NULL},

/*[clinic end generated code: output=8e64b88458d78936 input=a9049054013a1b77]*/
