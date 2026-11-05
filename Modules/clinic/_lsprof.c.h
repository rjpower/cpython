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
#include "pycore_long_value.h"    // _PyValue_AsDouble()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_lsprof_Profiler_getstats__doc__,
"getstats($self, /)\n"
"--\n"
"\n"
"list of profiler_entry objects.\n"
"\n"
"getstats() -> list of profiler_entry objects\n"
"\n"
"Return all information collected by the profiler.\n"
"Each profiler_entry is a tuple-like object with the\n"
"following attributes:\n"
"\n"
"    code          code object\n"
"    callcount     how many times this was called\n"
"    reccallcount  how many times called recursively\n"
"    totaltime     total time in this entry\n"
"    inlinetime    inline time in this entry (not in subcalls)\n"
"    calls         details of the calls\n"
"\n"
"The calls attribute is either None or a list of\n"
"profiler_subentry objects:\n"
"\n"
"    code          called code object\n"
"    callcount     how many times this is called\n"
"    reccallcount  how many times this is called recursively\n"
"    totaltime     total time spent in this call\n"
"    inlinetime    inline time (not in further subcalls)");

#define _LSPROF_PROFILER_GETSTATS_METHODDEF    \
    {"getstats", _PyCFunction_CAST(_lsprof_Profiler_getstats), METH_VALUE|METH_METHOD|METH_KEYWORDS, _lsprof_Profiler_getstats__doc__},

static PyValue
_lsprof_Profiler_getstats_impl(ProfilerObject *self, PyTypeObject *cls);

static PyValue
_lsprof_Profiler_getstats(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "getstats() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler_getstats_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pystart_callback__doc__,
"_pystart_callback($self, code, instruction_offset, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYSTART_CALLBACK_METHODDEF    \
    {"_pystart_callback", _PyCFunction_CAST(_lsprof_Profiler__pystart_callback), METH_VALUE, _lsprof_Profiler__pystart_callback__doc__},

static PyValue
_lsprof_Profiler__pystart_callback_impl(ProfilerObject *self,
                                        PyValueBorrow code,
                                        PyValueBorrow instruction_offset);

static PyValue
_lsprof_Profiler__pystart_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow code;
    PyValueBorrow instruction_offset;

    if (!_PyArg_CheckPositional("_pystart_callback", nargs, 2, 2)) {
        goto exit;
    }
    code = PyValue_Borrow(args[0]);
    instruction_offset = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler__pystart_callback_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), code, instruction_offset);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pythrow_callback__doc__,
"_pythrow_callback($self, code, instruction_offset, exception, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYTHROW_CALLBACK_METHODDEF    \
    {"_pythrow_callback", _PyCFunction_CAST(_lsprof_Profiler__pythrow_callback), METH_VALUE, _lsprof_Profiler__pythrow_callback__doc__},

static PyValue
_lsprof_Profiler__pythrow_callback_impl(ProfilerObject *self,
                                        PyValueBorrow code,
                                        PyValueBorrow instruction_offset,
                                        PyValueBorrow exception);

static PyValue
_lsprof_Profiler__pythrow_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow code;
    PyValueBorrow instruction_offset;
    PyValueBorrow exception;

    if (!_PyArg_CheckPositional("_pythrow_callback", nargs, 3, 3)) {
        goto exit;
    }
    code = PyValue_Borrow(args[0]);
    instruction_offset = PyValue_Borrow(args[1]);
    exception = PyValue_Borrow(args[2]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler__pythrow_callback_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), code, instruction_offset, exception);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__pyreturn_callback__doc__,
"_pyreturn_callback($self, code, instruction_offset, retval, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__PYRETURN_CALLBACK_METHODDEF    \
    {"_pyreturn_callback", _PyCFunction_CAST(_lsprof_Profiler__pyreturn_callback), METH_VALUE, _lsprof_Profiler__pyreturn_callback__doc__},

static PyValue
_lsprof_Profiler__pyreturn_callback_impl(ProfilerObject *self,
                                         PyValueBorrow code,
                                         PyValueBorrow instruction_offset,
                                         PyValueBorrow retval);

static PyValue
_lsprof_Profiler__pyreturn_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow code;
    PyValueBorrow instruction_offset;
    PyValueBorrow retval;

    if (!_PyArg_CheckPositional("_pyreturn_callback", nargs, 3, 3)) {
        goto exit;
    }
    code = PyValue_Borrow(args[0]);
    instruction_offset = PyValue_Borrow(args[1]);
    retval = PyValue_Borrow(args[2]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler__pyreturn_callback_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), code, instruction_offset, retval);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__ccall_callback__doc__,
"_ccall_callback($self, code, instruction_offset, callable, self_arg, /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__CCALL_CALLBACK_METHODDEF    \
    {"_ccall_callback", _PyCFunction_CAST(_lsprof_Profiler__ccall_callback), METH_VALUE, _lsprof_Profiler__ccall_callback__doc__},

static PyValue
_lsprof_Profiler__ccall_callback_impl(ProfilerObject *self,
                                      PyValueBorrow code,
                                      PyValueBorrow instruction_offset,
                                      PyValueBorrow callable,
                                      PyValueBorrow self_arg);

static PyValue
_lsprof_Profiler__ccall_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow code;
    PyValueBorrow instruction_offset;
    PyValueBorrow callable;
    PyValueBorrow self_arg;

    if (!_PyArg_CheckPositional("_ccall_callback", nargs, 4, 4)) {
        goto exit;
    }
    code = PyValue_Borrow(args[0]);
    instruction_offset = PyValue_Borrow(args[1]);
    callable = PyValue_Borrow(args[2]);
    self_arg = PyValue_Borrow(args[3]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler__ccall_callback_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), code, instruction_offset, callable, self_arg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler__creturn_callback__doc__,
"_creturn_callback($self, code, instruction_offset, callable, self_arg,\n"
"                  /)\n"
"--\n"
"\n");

#define _LSPROF_PROFILER__CRETURN_CALLBACK_METHODDEF    \
    {"_creturn_callback", _PyCFunction_CAST(_lsprof_Profiler__creturn_callback), METH_VALUE, _lsprof_Profiler__creturn_callback__doc__},

static PyValue
_lsprof_Profiler__creturn_callback_impl(ProfilerObject *self,
                                        PyValueBorrow code,
                                        PyValueBorrow instruction_offset,
                                        PyValueBorrow callable,
                                        PyValueBorrow self_arg);

static PyValue
_lsprof_Profiler__creturn_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow code;
    PyValueBorrow instruction_offset;
    PyValueBorrow callable;
    PyValueBorrow self_arg;

    if (!_PyArg_CheckPositional("_creturn_callback", nargs, 4, 4)) {
        goto exit;
    }
    code = PyValue_Borrow(args[0]);
    instruction_offset = PyValue_Borrow(args[1]);
    callable = PyValue_Borrow(args[2]);
    self_arg = PyValue_Borrow(args[3]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler__creturn_callback_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), code, instruction_offset, callable, self_arg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_enable__doc__,
"enable($self, /, subcalls=True, builtins=True)\n"
"--\n"
"\n"
"Start collecting profiling information.\n"
"\n"
"  subcalls\n"
"    If True, also records for each function\n"
"    statistics separated according to its current caller.\n"
"  builtins\n"
"    If True, records the time spent in\n"
"    built-in functions separately from their caller.");

#define _LSPROF_PROFILER_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(_lsprof_Profiler_enable), METH_VALUE|METH_KEYWORDS, _lsprof_Profiler_enable__doc__},

static PyValue
_lsprof_Profiler_enable_impl(ProfilerObject *self, int subcalls,
                             int builtins);

static PyValue
_lsprof_Profiler_enable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(subcalls)), _PyValue_STATIC_IMMORTAL(&_Py_ID(builtins)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"subcalls", "builtins", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int subcalls = 1;
    int builtins = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        subcalls = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (subcalls < 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        builtins = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (builtins < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler_enable_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self), subcalls, builtins);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_disable__doc__,
"disable($self, /)\n"
"--\n"
"\n"
"Stop collecting profiling information.");

#define _LSPROF_PROFILER_DISABLE_METHODDEF    \
    {"disable", _PyCFunction_CAST(_lsprof_Profiler_disable), METH_VALUE|METH_NOARGS, _lsprof_Profiler_disable__doc__},

static PyValue
_lsprof_Profiler_disable_impl(ProfilerObject *self);

static PyValue
_lsprof_Profiler_disable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler_disable_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_lsprof_Profiler_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Clear all profiling information collected so far.");

#define _LSPROF_PROFILER_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(_lsprof_Profiler_clear), METH_VALUE|METH_NOARGS, _lsprof_Profiler_clear__doc__},

static PyValue
_lsprof_Profiler_clear_impl(ProfilerObject *self);

static PyValue
_lsprof_Profiler_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _lsprof_Profiler_clear_impl((ProfilerObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(profiler_init__doc__,
"Profiler(timer=None, timeunit=0.0, subcalls=True, builtins=True)\n"
"--\n"
"\n"
"Build a profiler object using the specified timer function.\n"
"\n"
"The default timer is a fast built-in one based on real time.\n"
"For custom timer functions returning integers, \'timeunit\' can\n"
"be a float specifying a scale (that is, how long each integer unit\n"
"is, in seconds).");

static int
profiler_init_impl(ProfilerObject *self, PyValueBorrow timer,
                   double timeunit, int subcalls, int builtins);

static int
profiler_init_value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(timer)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timeunit)), _PyValue_STATIC_IMMORTAL(&_Py_ID(subcalls)), _PyValue_STATIC_IMMORTAL(&_Py_ID(builtins)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timer", "timeunit", "subcalls", "builtins", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Profiler",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow timer = PyValue_Borrow(PyValue_NULL);
    double timeunit = 0.0;
    int subcalls = 1;
    int builtins = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        timer = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        timeunit = _PyValue_AsDouble(PyValue_Borrow(args[_idx[1]]));
        if (timeunit == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        subcalls = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (subcalls < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        builtins = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (builtins < 0) {
            goto exit;
        }
    }
    return_value = profiler_init_impl((ProfilerObject *)self, timer, timeunit, subcalls, builtins);

exit:
    return return_value;
}

static int
profiler_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return profiler_init_value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = profiler_init_value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}
/*[clinic end generated code: output=ace4df7255669292 input=a9049054013a1b77]*/
