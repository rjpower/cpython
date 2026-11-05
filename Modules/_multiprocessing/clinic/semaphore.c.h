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
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

#if defined(HAVE_MP_SEMAPHORE) && defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_acquire__doc__,
"acquire($self, /, block=True, timeout=None)\n"
"--\n"
"\n"
"Acquire the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF    \
    {"acquire", _PyCFunction_CAST(_multiprocessing_SemLock_acquire), METH_VALUE|METH_KEYWORDS, _multiprocessing_SemLock_acquire__doc__},

static PyValue
_multiprocessing_SemLock_acquire_impl(SemLockObject *self, int blocking,
                                      PyValueBorrow timeout_obj);

static PyValue
_multiprocessing_SemLock_acquire(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(block)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"block", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int blocking = 1;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

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
        timeout_obj = PyValue_Borrow(args[_idx[1]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock_acquire_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self), blocking, timeout_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) && defined(MS_WINDOWS) */

#if defined(HAVE_MP_SEMAPHORE) && defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_multiprocessing_SemLock_release), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock_release__doc__},

static PyValue
_multiprocessing_SemLock_release_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock_release(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock_release_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) && defined(MS_WINDOWS) */

#if defined(HAVE_MP_SEMAPHORE) && !defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_acquire__doc__,
"acquire($self, /, block=True, timeout=None)\n"
"--\n"
"\n"
"Acquire the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF    \
    {"acquire", _PyCFunction_CAST(_multiprocessing_SemLock_acquire), METH_VALUE|METH_KEYWORDS, _multiprocessing_SemLock_acquire__doc__},

static PyValue
_multiprocessing_SemLock_acquire_impl(SemLockObject *self, int blocking,
                                      PyValueBorrow timeout_obj);

static PyValue
_multiprocessing_SemLock_acquire(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(block)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"block", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int blocking = 1;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

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
        timeout_obj = PyValue_Borrow(args[_idx[1]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock_acquire_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self), blocking, timeout_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) && !defined(MS_WINDOWS) */

#if defined(HAVE_MP_SEMAPHORE) && !defined(MS_WINDOWS)

PyDoc_STRVAR(_multiprocessing_SemLock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_multiprocessing_SemLock_release), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock_release__doc__},

static PyValue
_multiprocessing_SemLock_release_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock_release(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock_release_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) && !defined(MS_WINDOWS) */

#if defined(HAVE_MP_SEMAPHORE)

static PyValue
_multiprocessing_SemLock_impl(PyTypeObject *type, int kind, int value,
                              int maxvalue, const char *name, int unlink);

static PyValue
_multiprocessing_SemLock_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(kind)), _PyValue_STATIC_IMMORTAL(&_Py_ID(value)), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxvalue)), _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(unlink)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"kind", "value", "maxvalue", "name", "unlink", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "SemLock",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    int kind;
    int value;
    int maxvalue;
    const char *name;
    int unlink;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 5, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    kind = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (kind == -1 && PyErr_Occurred()) {
        goto exit;
    }
    value = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    maxvalue = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (maxvalue == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
        _PyArg_BadArgumentValue("SemLock", "argument 'name'", "str", PyValue_Borrow(args[_idx[3]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])))) {
        _PyArg_BadArgument("SemLock", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
    if (name == NULL) {
        goto exit;
    }
    unlink = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
    if (unlink < 0) {
        goto exit;
    }
    return_value = _multiprocessing_SemLock_impl(type, kind, value, maxvalue, name, unlink);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_multiprocessing_SemLock(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_multiprocessing_SemLock_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _multiprocessing_SemLock_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock__rebuild__doc__,
"_rebuild($type, handle, kind, maxvalue, name, /)\n"
"--\n"
"\n");

#define _MULTIPROCESSING_SEMLOCK__REBUILD_METHODDEF    \
    {"_rebuild", _PyCFunction_CAST(_multiprocessing_SemLock__rebuild), METH_FASTCALL|METH_CLASS, _multiprocessing_SemLock__rebuild__doc__},

static PyObject *
_multiprocessing_SemLock__rebuild_impl(PyTypeObject *type, SEM_HANDLE handle,
                                       int kind, int maxvalue,
                                       const char *name);

static PyObject *
_multiprocessing_SemLock__rebuild(PyObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    SEM_HANDLE handle;
    int kind;
    int maxvalue;
    const char *name;

    if (!PyArg_ParseArray(args, nargs, ""F_SEM_HANDLE"iiz:_rebuild",
        &handle, &kind, &maxvalue, &name)) {
        goto exit;
    }
    return_value = _multiprocessing_SemLock__rebuild_impl((PyTypeObject *)type, handle, kind, maxvalue, name);

exit:
    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock__count__doc__,
"_count($self, /)\n"
"--\n"
"\n"
"Num of `acquire()`s minus num of `release()`s for this process.");

#define _MULTIPROCESSING_SEMLOCK__COUNT_METHODDEF    \
    {"_count", _PyCFunction_CAST(_multiprocessing_SemLock__count), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock__count__doc__},

static PyValue
_multiprocessing_SemLock__count_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock__count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock__count_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock__is_mine__doc__,
"_is_mine($self, /)\n"
"--\n"
"\n"
"Whether the lock is owned by this thread.");

#define _MULTIPROCESSING_SEMLOCK__IS_MINE_METHODDEF    \
    {"_is_mine", _PyCFunction_CAST(_multiprocessing_SemLock__is_mine), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock__is_mine__doc__},

static PyValue
_multiprocessing_SemLock__is_mine_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock__is_mine(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multiprocessing_SemLock__is_mine_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock__get_value__doc__,
"_get_value($self, /)\n"
"--\n"
"\n"
"Get the value of the semaphore.");

#define _MULTIPROCESSING_SEMLOCK__GET_VALUE_METHODDEF    \
    {"_get_value", _PyCFunction_CAST(_multiprocessing_SemLock__get_value), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock__get_value__doc__},

static PyValue
_multiprocessing_SemLock__get_value_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock__get_value(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multiprocessing_SemLock__get_value_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock__is_zero__doc__,
"_is_zero($self, /)\n"
"--\n"
"\n"
"Return whether semaphore has value zero.");

#define _MULTIPROCESSING_SEMLOCK__IS_ZERO_METHODDEF    \
    {"_is_zero", _PyCFunction_CAST(_multiprocessing_SemLock__is_zero), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock__is_zero__doc__},

static PyValue
_multiprocessing_SemLock__is_zero_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock__is_zero(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multiprocessing_SemLock__is_zero_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock__after_fork__doc__,
"_after_fork($self, /)\n"
"--\n"
"\n"
"Rezero the net acquisition count after fork().");

#define _MULTIPROCESSING_SEMLOCK__AFTER_FORK_METHODDEF    \
    {"_after_fork", _PyCFunction_CAST(_multiprocessing_SemLock__after_fork), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock__after_fork__doc__},

static PyValue
_multiprocessing_SemLock__after_fork_impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock__after_fork(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multiprocessing_SemLock__after_fork_impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Enter the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(_multiprocessing_SemLock___enter__), METH_VALUE|METH_NOARGS, _multiprocessing_SemLock___enter____doc__},

static PyValue
_multiprocessing_SemLock___enter___impl(SemLockObject *self);

static PyValue
_multiprocessing_SemLock___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock___enter___impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#if defined(HAVE_MP_SEMAPHORE)

PyDoc_STRVAR(_multiprocessing_SemLock___exit____doc__,
"__exit__($self, exc_type=None, exc_value=None, exc_tb=None, /)\n"
"--\n"
"\n"
"Exit the semaphore/lock.");

#define _MULTIPROCESSING_SEMLOCK___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_multiprocessing_SemLock___exit__), METH_VALUE, _multiprocessing_SemLock___exit____doc__},

static PyValue
_multiprocessing_SemLock___exit___impl(SemLockObject *self,
                                       PyValueBorrow exc_type,
                                       PyValueBorrow exc_value,
                                       PyValueBorrow exc_tb);

static PyValue
_multiprocessing_SemLock___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exc_type = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_value = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_tb = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("__exit__", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    exc_type = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    exc_value = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    exc_tb = PyValue_Borrow(args[2]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _multiprocessing_SemLock___exit___impl((SemLockObject *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_value, exc_tb);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_MP_SEMAPHORE) */

#ifndef _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK_ACQUIRE_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK_RELEASE_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK__REBUILD_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK__REBUILD_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK__REBUILD_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK__COUNT_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK__COUNT_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK__COUNT_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK__IS_MINE_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK__IS_MINE_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK__IS_MINE_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK__GET_VALUE_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK__GET_VALUE_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK__GET_VALUE_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK__IS_ZERO_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK__IS_ZERO_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK__IS_ZERO_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK__AFTER_FORK_METHODDEF
    #define _MULTIPROCESSING_SEMLOCK__AFTER_FORK_METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK__AFTER_FORK_METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK___ENTER___METHODDEF
    #define _MULTIPROCESSING_SEMLOCK___ENTER___METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK___ENTER___METHODDEF) */

#ifndef _MULTIPROCESSING_SEMLOCK___EXIT___METHODDEF
    #define _MULTIPROCESSING_SEMLOCK___EXIT___METHODDEF
#endif /* !defined(_MULTIPROCESSING_SEMLOCK___EXIT___METHODDEF) */
/*[clinic end generated code: output=e51daf9c8b4a6426 input=a9049054013a1b77]*/
