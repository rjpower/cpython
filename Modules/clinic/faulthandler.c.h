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
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(faulthandler_dump_traceback_py__doc__,
"dump_traceback($module, /, file=sys.stderr, all_threads=True, *,\n"
"               max_threads=100)\n"
"--\n"
"\n"
"Dump the traceback of the current thread into file.\n"
"\n"
"Dump the traceback of all threads if all_threads is true. max_threads\n"
"caps the number of threads dumped.");

#define FAULTHANDLER_DUMP_TRACEBACK_PY_METHODDEF    \
    {"dump_traceback", _PyCFunction_CAST(faulthandler_dump_traceback_py), METH_VALUE|METH_KEYWORDS, faulthandler_dump_traceback_py__doc__},

static PyValue
faulthandler_dump_traceback_py_impl(PyObject *module, PyValueBorrow file,
                                    int all_threads, Py_ssize_t max_threads);

static PyValue
faulthandler_dump_traceback_py(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), _PyValue_STATIC_IMMORTAL(&_Py_ID(all_threads)), _PyValue_STATIC_IMMORTAL(&_Py_ID(max_threads)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "all_threads", "max_threads", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump_traceback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow file = PyValue_Borrow(PyValue_NULL);
    int all_threads = 1;
    Py_ssize_t max_threads = 100;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        file = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        all_threads = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (all_threads < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            max_threads = ival;
        }
    }
    return_value = faulthandler_dump_traceback_py_impl(PyValue_AsKnownObjectBorrow(module), file, all_threads, max_threads);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_dump_c_stack_py__doc__,
"dump_c_stack($module, /, file=sys.stderr)\n"
"--\n"
"\n"
"Dump the C stack of the current thread.");

#define FAULTHANDLER_DUMP_C_STACK_PY_METHODDEF    \
    {"dump_c_stack", _PyCFunction_CAST(faulthandler_dump_c_stack_py), METH_VALUE|METH_KEYWORDS, faulthandler_dump_c_stack_py__doc__},

static PyValue
faulthandler_dump_c_stack_py_impl(PyObject *module, PyValueBorrow file);

static PyValue
faulthandler_dump_c_stack_py(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump_c_stack",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow file = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        file = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = faulthandler_dump_c_stack_py_impl(PyValue_AsKnownObjectBorrow(module), file);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_py_enable__doc__,
"enable($module, /, file=sys.stderr, all_threads=True, c_stack=True, *,\n"
"       max_threads=100)\n"
"--\n"
"\n"
"Enable the fault handler.");

#define FAULTHANDLER_PY_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(faulthandler_py_enable), METH_VALUE|METH_KEYWORDS, faulthandler_py_enable__doc__},

static PyValue
faulthandler_py_enable_impl(PyObject *module, PyValueBorrow file,
                            int all_threads, int c_stack,
                            Py_ssize_t max_threads);

static PyValue
faulthandler_py_enable(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), _PyValue_STATIC_IMMORTAL(&_Py_ID(all_threads)), _PyValue_STATIC_IMMORTAL(&_Py_ID(c_stack)), _PyValue_STATIC_IMMORTAL(&_Py_ID(max_threads)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "all_threads", "c_stack", "max_threads", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow file = PyValue_Borrow(PyValue_NULL);
    int all_threads = 1;
    int c_stack = 1;
    Py_ssize_t max_threads = 100;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        file = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        all_threads = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (all_threads < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        c_stack = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (c_stack < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[3]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            max_threads = ival;
        }
    }
    return_value = faulthandler_py_enable_impl(PyValue_AsKnownObjectBorrow(module), file, all_threads, c_stack, max_threads);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_disable_py__doc__,
"disable($module, /)\n"
"--\n"
"\n"
"Disable the fault handler.");

#define FAULTHANDLER_DISABLE_PY_METHODDEF    \
    {"disable", _PyCFunction_CAST(faulthandler_disable_py), METH_VALUE|METH_NOARGS, faulthandler_disable_py__doc__},

static PyValue
faulthandler_disable_py_impl(PyObject *module);

static PyValue
faulthandler_disable_py(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = faulthandler_disable_py_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(faulthandler_is_enabled__doc__,
"is_enabled($module, /)\n"
"--\n"
"\n"
"Check if the handler is enabled.");

#define FAULTHANDLER_IS_ENABLED_METHODDEF    \
    {"is_enabled", _PyCFunction_CAST(faulthandler_is_enabled), METH_VALUE|METH_NOARGS, faulthandler_is_enabled__doc__},

static int
faulthandler_is_enabled_impl(PyObject *module);

static PyValue
faulthandler_is_enabled(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = faulthandler_is_enabled_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_dump_traceback_later__doc__,
"dump_traceback_later($module, /, timeout, repeat=False,\n"
"                     file=sys.stderr, exit=False, *, max_threads=100)\n"
"--\n"
"\n"
"Dump the traceback of all threads in timeout seconds.\n"
"\n"
"If repeat is true, the tracebacks of all threads are dumped every\n"
"timeout seconds.  If exit is true, call _exit(1) which is not safe.\n"
"max_threads caps the number of threads dumped.");

#define FAULTHANDLER_DUMP_TRACEBACK_LATER_METHODDEF    \
    {"dump_traceback_later", _PyCFunction_CAST(faulthandler_dump_traceback_later), METH_VALUE|METH_KEYWORDS, faulthandler_dump_traceback_later__doc__},

static PyValue
faulthandler_dump_traceback_later_impl(PyObject *module,
                                       PyValueBorrow timeout_obj, int repeat,
                                       PyValueBorrow file, int exit,
                                       Py_ssize_t max_threads);

static PyValue
faulthandler_dump_traceback_later(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), _PyValue_STATIC_IMMORTAL(&_Py_ID(repeat)), _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), _PyValue_STATIC_IMMORTAL(&_Py_ID(exit)), _PyValue_STATIC_IMMORTAL(&_Py_ID(max_threads)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timeout", "repeat", "file", "exit", "max_threads", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump_traceback_later",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow timeout_obj;
    int repeat = 0;
    PyValueBorrow file = PyValue_Borrow(PyValue_NULL);
    int exit = 0;
    Py_ssize_t max_threads = 100;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    timeout_obj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        repeat = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (repeat < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        file = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        exit = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (exit < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[4]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            max_threads = ival;
        }
    }
    return_value = faulthandler_dump_traceback_later_impl(PyValue_AsKnownObjectBorrow(module), timeout_obj, repeat, file, exit, max_threads);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler_cancel_dump_traceback_later_py__doc__,
"cancel_dump_traceback_later($module, /)\n"
"--\n"
"\n"
"Cancel the previous call to dump_traceback_later().");

#define FAULTHANDLER_CANCEL_DUMP_TRACEBACK_LATER_PY_METHODDEF    \
    {"cancel_dump_traceback_later", _PyCFunction_CAST(faulthandler_cancel_dump_traceback_later_py), METH_VALUE|METH_NOARGS, faulthandler_cancel_dump_traceback_later_py__doc__},

static PyValue
faulthandler_cancel_dump_traceback_later_py_impl(PyObject *module);

static PyValue
faulthandler_cancel_dump_traceback_later_py(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = faulthandler_cancel_dump_traceback_later_py_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(FAULTHANDLER_USER)

PyDoc_STRVAR(faulthandler_register_py__doc__,
"register($module, /, signum, file=sys.stderr, all_threads=True,\n"
"         chain=False, *, max_threads=100)\n"
"--\n"
"\n"
"Register a handler for the signal \'signum\'.\n"
"\n"
"Dump the traceback of the current thread, or of all threads if\n"
"all_threads is True, into file. max_threads caps the number of threads\n"
"dumped.");

#define FAULTHANDLER_REGISTER_PY_METHODDEF    \
    {"register", _PyCFunction_CAST(faulthandler_register_py), METH_VALUE|METH_KEYWORDS, faulthandler_register_py__doc__},

static PyValue
faulthandler_register_py_impl(PyObject *module, int signum,
                              PyValueBorrow file, int all_threads, int chain,
                              Py_ssize_t max_threads);

static PyValue
faulthandler_register_py(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(signum)), _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), _PyValue_STATIC_IMMORTAL(&_Py_ID(all_threads)), _PyValue_STATIC_IMMORTAL(&_Py_ID(chain)), _PyValue_STATIC_IMMORTAL(&_Py_ID(max_threads)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"signum", "file", "all_threads", "chain", "max_threads", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "register",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    int signum;
    PyValueBorrow file = PyValue_Borrow(PyValue_NULL);
    int all_threads = 1;
    int chain = 0;
    Py_ssize_t max_threads = 100;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    signum = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (signum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        file = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        all_threads = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (all_threads < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        chain = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (chain < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[4]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            max_threads = ival;
        }
    }
    return_value = faulthandler_register_py_impl(PyValue_AsKnownObjectBorrow(module), signum, file, all_threads, chain, max_threads);

exit:
    return return_value;
}

#endif /* defined(FAULTHANDLER_USER) */

#if defined(FAULTHANDLER_USER)

PyDoc_STRVAR(faulthandler_unregister_py__doc__,
"unregister($module, signum, /)\n"
"--\n"
"\n"
"Unregister the handler of the signal \'signum\' registered by register().");

#define FAULTHANDLER_UNREGISTER_PY_METHODDEF    \
    {"unregister", _PyCFunction_CAST(faulthandler_unregister_py), METH_VALUE|METH_O, faulthandler_unregister_py__doc__},

static PyValue
faulthandler_unregister_py_impl(PyObject *module, int signum);

static PyValue
faulthandler_unregister_py(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int signum;

    signum = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (signum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = faulthandler_unregister_py_impl(PyValue_AsKnownObjectBorrow(module), signum);

exit:
    return return_value;
}

#endif /* defined(FAULTHANDLER_USER) */

PyDoc_STRVAR(faulthandler__sigsegv__doc__,
"_sigsegv($module, release_gil=False, /)\n"
"--\n"
"\n"
"Raise a SIGSEGV signal.");

#define FAULTHANDLER__SIGSEGV_METHODDEF    \
    {"_sigsegv", _PyCFunction_CAST(faulthandler__sigsegv), METH_VALUE, faulthandler__sigsegv__doc__},

static PyValue
faulthandler__sigsegv_impl(PyObject *module, int release_gil);

static PyValue
faulthandler__sigsegv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int release_gil = 0;

    if (!_PyArg_CheckPositional("_sigsegv", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    release_gil = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (release_gil < 0) {
        goto exit;
    }
skip_optional:
    return_value = faulthandler__sigsegv_impl(PyValue_AsKnownObjectBorrow(module), release_gil);

exit:
    return return_value;
}

PyDoc_STRVAR(faulthandler__fatal_error_c_thread__doc__,
"_fatal_error_c_thread($module, /)\n"
"--\n"
"\n"
"Call Py_FatalError() in a new C thread.");

#define FAULTHANDLER__FATAL_ERROR_C_THREAD_METHODDEF    \
    {"_fatal_error_c_thread", _PyCFunction_CAST(faulthandler__fatal_error_c_thread), METH_VALUE|METH_NOARGS, faulthandler__fatal_error_c_thread__doc__},

static PyValue
faulthandler__fatal_error_c_thread_impl(PyObject *module);

static PyValue
faulthandler__fatal_error_c_thread(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = faulthandler__fatal_error_c_thread_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(faulthandler__sigfpe__doc__,
"_sigfpe($module, /)\n"
"--\n"
"\n"
"Raise a SIGFPE signal.");

#define FAULTHANDLER__SIGFPE_METHODDEF    \
    {"_sigfpe", _PyCFunction_CAST(faulthandler__sigfpe), METH_VALUE|METH_NOARGS, faulthandler__sigfpe__doc__},

static PyValue
faulthandler__sigfpe_impl(PyObject *module);

static PyValue
faulthandler__sigfpe(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = faulthandler__sigfpe_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(faulthandler__sigabrt__doc__,
"_sigabrt($module, /)\n"
"--\n"
"\n"
"Raise a SIGABRT signal.");

#define FAULTHANDLER__SIGABRT_METHODDEF    \
    {"_sigabrt", _PyCFunction_CAST(faulthandler__sigabrt), METH_VALUE|METH_NOARGS, faulthandler__sigabrt__doc__},

static PyValue
faulthandler__sigabrt_impl(PyObject *module);

static PyValue
faulthandler__sigabrt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = faulthandler__sigabrt_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(FAULTHANDLER_USE_ALT_STACK)

PyDoc_STRVAR(faulthandler__stack_overflow__doc__,
"_stack_overflow($module, /)\n"
"--\n"
"\n"
"Recursive call to raise a stack overflow.");

#define FAULTHANDLER__STACK_OVERFLOW_METHODDEF    \
    {"_stack_overflow", _PyCFunction_CAST(faulthandler__stack_overflow), METH_VALUE|METH_NOARGS, faulthandler__stack_overflow__doc__},

static PyValue
faulthandler__stack_overflow_impl(PyObject *module);

static PyValue
faulthandler__stack_overflow(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = faulthandler__stack_overflow_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(FAULTHANDLER_USE_ALT_STACK) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(faulthandler__raise_exception__doc__,
"_raise_exception($module, code, flags=0, /)\n"
"--\n"
"\n"
"Call RaiseException(code, flags).");

#define FAULTHANDLER__RAISE_EXCEPTION_METHODDEF    \
    {"_raise_exception", _PyCFunction_CAST(faulthandler__raise_exception), METH_VALUE, faulthandler__raise_exception__doc__},

static PyValue
faulthandler__raise_exception_impl(PyObject *module, unsigned int code,
                                   unsigned int flags);

static PyValue
faulthandler__raise_exception(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned int code;
    unsigned int flags = 0;

    if (!_PyArg_CheckPositional("_raise_exception", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &code, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned int");
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &flags, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned int");
            goto exit;
        }
    }
skip_optional:
    return_value = faulthandler__raise_exception_impl(PyValue_AsKnownObjectBorrow(module), code, flags);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#ifndef FAULTHANDLER_REGISTER_PY_METHODDEF
    #define FAULTHANDLER_REGISTER_PY_METHODDEF
#endif /* !defined(FAULTHANDLER_REGISTER_PY_METHODDEF) */

#ifndef FAULTHANDLER_UNREGISTER_PY_METHODDEF
    #define FAULTHANDLER_UNREGISTER_PY_METHODDEF
#endif /* !defined(FAULTHANDLER_UNREGISTER_PY_METHODDEF) */

#ifndef FAULTHANDLER__STACK_OVERFLOW_METHODDEF
    #define FAULTHANDLER__STACK_OVERFLOW_METHODDEF
#endif /* !defined(FAULTHANDLER__STACK_OVERFLOW_METHODDEF) */

#ifndef FAULTHANDLER__RAISE_EXCEPTION_METHODDEF
    #define FAULTHANDLER__RAISE_EXCEPTION_METHODDEF
#endif /* !defined(FAULTHANDLER__RAISE_EXCEPTION_METHODDEF) */
/*[clinic end generated code: output=175e82a533635ec7 input=a9049054013a1b77]*/
