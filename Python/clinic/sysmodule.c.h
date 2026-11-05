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
#include "pycore_long_value.h"    // _PyValue_AsDouble()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(sys_addaudithook__doc__,
"addaudithook($module, /, hook)\n"
"--\n"
"\n"
"Adds a new audit hook callback.");

#define SYS_ADDAUDITHOOK_METHODDEF    \
    {"addaudithook", _PyCFunction_CAST(sys_addaudithook), METH_VALUE, sys_addaudithook__doc__},

static PyValue
sys_addaudithook_impl(PyObject *module, PyValueBorrow hook);

static PyValue
sys_addaudithook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(hook)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hook", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "addaudithook",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow hook;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    hook = PyValue_Borrow(args[_idx[0]]);
    return_value = sys_addaudithook_impl(PyValue_AsKnownObjectBorrow(module), hook);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_audit__doc__,
"audit($module, event, /, *args)\n"
"--\n"
"\n"
"Passes the event to any audit hooks that are attached.");

#define SYS_AUDIT_METHODDEF    \
    {"audit", _PyCFunction_CAST(sys_audit), METH_VALUE, sys_audit__doc__},

static PyValue
sys_audit_impl(PyObject *module, const char *event, PyObject *args);

static PyValue
sys_audit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *event;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_NoKwnamesValue("sys.audit()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("audit", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("audit", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("audit", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    event = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (event == NULL) {
        goto exit;
    }
    __clinic_args = _PyTuple_FromValueArray(args + 1,
                                          Py_MAX(nargs - 1, 0));
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = sys_audit_impl(PyValue_AsKnownObjectBorrow(module), event, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(sys_displayhook__doc__,
"displayhook($module, object, /)\n"
"--\n"
"\n"
"Print an object to sys.stdout and also save it in builtins._");

#define SYS_DISPLAYHOOK_METHODDEF    \
    {"displayhook", _PyCFunction_CAST(sys_displayhook), METH_VALUE, sys_displayhook__doc__},

static PyValue
sys_displayhook_impl(PyObject *module, PyValueBorrow o);

static PyValue
sys_displayhook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow o;

    if (!_PyArg_CheckArityValue("sys.displayhook()", nargs, kwnames, 1)) {
        goto exit;
    }
    o = PyValue_Borrow(args[0]);
    return_value = sys_displayhook_impl(PyValue_AsKnownObjectBorrow(module), o);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_excepthook__doc__,
"excepthook($module, exctype, value, traceback, /)\n"
"--\n"
"\n"
"Handle an exception by displaying it with a traceback on sys.stderr.");

#define SYS_EXCEPTHOOK_METHODDEF    \
    {"excepthook", _PyCFunction_CAST(sys_excepthook), METH_VALUE, sys_excepthook__doc__},

static PyValue
sys_excepthook_impl(PyObject *module, PyValueBorrow exctype,
                    PyValueBorrow value, PyValueBorrow traceback);

static PyValue
sys_excepthook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exctype;
    PyValueBorrow value;
    PyValueBorrow traceback;

    if (!_PyArg_NoKwnamesValue("sys.excepthook()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("excepthook", nargs, 3, 3)) {
        goto exit;
    }
    exctype = PyValue_Borrow(args[0]);
    value = PyValue_Borrow(args[1]);
    traceback = PyValue_Borrow(args[2]);
    return_value = sys_excepthook_impl(PyValue_AsKnownObjectBorrow(module), exctype, value, traceback);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_exception__doc__,
"exception($module, /)\n"
"--\n"
"\n"
"Return the current exception.\n"
"\n"
"Return the most recent exception caught by an except clause\n"
"in the current stack frame or in an older stack frame, or None\n"
"if no such exception exists.");

#define SYS_EXCEPTION_METHODDEF    \
    {"exception", _PyCFunction_CAST(sys_exception), METH_VALUE, sys_exception__doc__},

static PyValue
sys_exception_impl(PyObject *module);

static PyValue
sys_exception(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.exception()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_exception_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_exc_info__doc__,
"exc_info($module, /)\n"
"--\n"
"\n"
"Return current exception information: (type, value, traceback).\n"
"\n"
"Return information about the most recent exception caught by an except\n"
"clause in the current stack frame or in an older stack frame.");

#define SYS_EXC_INFO_METHODDEF    \
    {"exc_info", _PyCFunction_CAST(sys_exc_info), METH_VALUE, sys_exc_info__doc__},

static PyValue
sys_exc_info_impl(PyObject *module);

static PyValue
sys_exc_info(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.exc_info()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_exc_info_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_unraisablehook__doc__,
"unraisablehook($module, unraisable, /)\n"
"--\n"
"\n"
"Handle an unraisable exception.\n"
"\n"
"The unraisable argument has the following attributes:\n"
"\n"
"* exc_type: Exception type.\n"
"* exc_value: Exception value, can be None.\n"
"* exc_traceback: Exception traceback, can be None.\n"
"* err_msg: Error message, can be None.\n"
"* object: Object causing the exception, can be None.");

#define SYS_UNRAISABLEHOOK_METHODDEF    \
    {"unraisablehook", _PyCFunction_CAST(sys_unraisablehook), METH_VALUE, sys_unraisablehook__doc__},

static PyValue
sys_unraisablehook_impl(PyObject *module, PyValueBorrow unraisable);

static PyValue
sys_unraisablehook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow unraisable;

    if (!_PyArg_CheckArityValue("sys.unraisablehook()", nargs, kwnames, 1)) {
        goto exit;
    }
    unraisable = PyValue_Borrow(args[0]);
    return_value = sys_unraisablehook_impl(PyValue_AsKnownObjectBorrow(module), unraisable);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_exit__doc__,
"exit($module, status=None, /)\n"
"--\n"
"\n"
"Exit the interpreter by raising SystemExit(status).\n"
"\n"
"If the status is omitted or None, it defaults to zero (i.e., success).\n"
"If the status is an integer, it will be used as the system exit status.\n"
"If it is another kind of object, it will be printed and the system\n"
"exit status will be one (i.e., failure).");

#define SYS_EXIT_METHODDEF    \
    {"exit", _PyCFunction_CAST(sys_exit), METH_VALUE, sys_exit__doc__},

static PyValue
sys_exit_impl(PyObject *module, PyValueBorrow status);

static PyValue
sys_exit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow status = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("sys.exit()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("exit", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    status = PyValue_Borrow(args[0]);
skip_optional:
    return_value = sys_exit_impl(PyValue_AsKnownObjectBorrow(module), status);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getdefaultencoding__doc__,
"getdefaultencoding($module, /)\n"
"--\n"
"\n"
"Return the current default encoding used by the Unicode implementation.");

#define SYS_GETDEFAULTENCODING_METHODDEF    \
    {"getdefaultencoding", _PyCFunction_CAST(sys_getdefaultencoding), METH_VALUE, sys_getdefaultencoding__doc__},

static PyValue
sys_getdefaultencoding_impl(PyObject *module);

static PyValue
sys_getdefaultencoding(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getdefaultencoding()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getdefaultencoding_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getfilesystemencoding__doc__,
"getfilesystemencoding($module, /)\n"
"--\n"
"\n"
"Return the encoding used to convert Unicode filenames to OS filenames.");

#define SYS_GETFILESYSTEMENCODING_METHODDEF    \
    {"getfilesystemencoding", _PyCFunction_CAST(sys_getfilesystemencoding), METH_VALUE, sys_getfilesystemencoding__doc__},

static PyValue
sys_getfilesystemencoding_impl(PyObject *module);

static PyValue
sys_getfilesystemencoding(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getfilesystemencoding()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getfilesystemencoding_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getfilesystemencodeerrors__doc__,
"getfilesystemencodeerrors($module, /)\n"
"--\n"
"\n"
"Return the error mode used Unicode to OS filename conversion.");

#define SYS_GETFILESYSTEMENCODEERRORS_METHODDEF    \
    {"getfilesystemencodeerrors", _PyCFunction_CAST(sys_getfilesystemencodeerrors), METH_VALUE, sys_getfilesystemencodeerrors__doc__},

static PyValue
sys_getfilesystemencodeerrors_impl(PyObject *module);

static PyValue
sys_getfilesystemencodeerrors(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getfilesystemencodeerrors()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getfilesystemencodeerrors_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_intern__doc__,
"intern($module, string, /)\n"
"--\n"
"\n"
"``Intern\'\' the given string.\n"
"\n"
"This enters the string in the (global) table of interned strings whose\n"
"purpose is to speed up dictionary lookups. Return the string itself or\n"
"the previously interned string object with the same value.");

#define SYS_INTERN_METHODDEF    \
    {"intern", _PyCFunction_CAST(sys_intern), METH_VALUE, sys_intern__doc__},

static PyValue
sys_intern_impl(PyObject *module, PyObject *s);

static PyValue
sys_intern(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *s;

    if (!_PyArg_CheckArityValue("sys.intern()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("intern", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("intern", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    s = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = sys_intern_impl(PyValue_AsKnownObjectBorrow(module), s);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__is_interned__doc__,
"_is_interned($module, string, /)\n"
"--\n"
"\n"
"Return True if the given string is \"interned\".");

#define SYS__IS_INTERNED_METHODDEF    \
    {"_is_interned", _PyCFunction_CAST(sys__is_interned), METH_VALUE, sys__is_interned__doc__},

static int
sys__is_interned_impl(PyObject *module, PyObject *string);

static PyValue
sys__is_interned(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;
    int _return_value;

    if (!_PyArg_CheckArityValue("sys._is_interned()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("_is_interned", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("_is_interned", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    _return_value = sys__is_interned_impl(PyValue_AsKnownObjectBorrow(module), string);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(sys__is_immortal__doc__,
"_is_immortal($module, op, /)\n"
"--\n"
"\n"
"Return True if the given object is \"immortal\" per PEP 683.\n"
"\n"
"This function should be used for specialized purposes only.");

#define SYS__IS_IMMORTAL_METHODDEF    \
    {"_is_immortal", _PyCFunction_CAST(sys__is_immortal), METH_VALUE, sys__is_immortal__doc__},

static int
sys__is_immortal_impl(PyObject *module, PyValueBorrow op);

static PyValue
sys__is_immortal(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow op;
    int _return_value;

    if (!_PyArg_CheckArityValue("sys._is_immortal()", nargs, kwnames, 1)) {
        goto exit;
    }
    op = PyValue_Borrow(args[0]);
    _return_value = sys__is_immortal_impl(PyValue_AsKnownObjectBorrow(module), op);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(sys_settrace__doc__,
"settrace($module, function, /)\n"
"--\n"
"\n"
"Set the global debug tracing function.\n"
"\n"
"It will be called on each function call.  See the debugger chapter\n"
"in the library manual.");

#define SYS_SETTRACE_METHODDEF    \
    {"settrace", _PyCFunction_CAST(sys_settrace), METH_VALUE, sys_settrace__doc__},

static PyValue
sys_settrace_impl(PyObject *module, PyValueBorrow function);

static PyValue
sys_settrace(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow function;

    if (!_PyArg_CheckArityValue("sys.settrace()", nargs, kwnames, 1)) {
        goto exit;
    }
    function = PyValue_Borrow(args[0]);
    return_value = sys_settrace_impl(PyValue_AsKnownObjectBorrow(module), function);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__settraceallthreads__doc__,
"_settraceallthreads($module, function, /)\n"
"--\n"
"\n"
"Set the global debug tracing function in all running threads belonging to the current interpreter.\n"
"\n"
"It will be called on each function call. See the debugger chapter\n"
"in the library manual.");

#define SYS__SETTRACEALLTHREADS_METHODDEF    \
    {"_settraceallthreads", _PyCFunction_CAST(sys__settraceallthreads), METH_VALUE, sys__settraceallthreads__doc__},

static PyValue
sys__settraceallthreads_impl(PyObject *module, PyValueBorrow arg);

static PyValue
sys__settraceallthreads(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    if (!_PyArg_CheckArityValue("sys._settraceallthreads()", nargs, kwnames, 1)) {
        goto exit;
    }
    arg = PyValue_Borrow(args[0]);
    return_value = sys__settraceallthreads_impl(PyValue_AsKnownObjectBorrow(module), arg);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_gettrace__doc__,
"gettrace($module, /)\n"
"--\n"
"\n"
"Return the global debug tracing function set with sys.settrace.\n"
"\n"
"See the debugger chapter in the library manual.");

#define SYS_GETTRACE_METHODDEF    \
    {"gettrace", _PyCFunction_CAST(sys_gettrace), METH_VALUE, sys_gettrace__doc__},

static PyValue
sys_gettrace_impl(PyObject *module);

static PyValue
sys_gettrace(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.gettrace()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_gettrace_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_setprofile__doc__,
"setprofile($module, function, /)\n"
"--\n"
"\n"
"Set the profiling function.\n"
"\n"
"It will be called on each function call and return.  See the profiler\n"
"chapter in the library manual.");

#define SYS_SETPROFILE_METHODDEF    \
    {"setprofile", _PyCFunction_CAST(sys_setprofile), METH_VALUE, sys_setprofile__doc__},

static PyValue
sys_setprofile_impl(PyObject *module, PyValueBorrow function);

static PyValue
sys_setprofile(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow function;

    if (!_PyArg_CheckArityValue("sys.setprofile()", nargs, kwnames, 1)) {
        goto exit;
    }
    function = PyValue_Borrow(args[0]);
    return_value = sys_setprofile_impl(PyValue_AsKnownObjectBorrow(module), function);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__setprofileallthreads__doc__,
"_setprofileallthreads($module, function, /)\n"
"--\n"
"\n"
"Set the profiling function in all running threads belonging to the current interpreter.\n"
"\n"
"It will be called on each function call and return.  See the profiler\n"
"chapter in the library manual.");

#define SYS__SETPROFILEALLTHREADS_METHODDEF    \
    {"_setprofileallthreads", _PyCFunction_CAST(sys__setprofileallthreads), METH_VALUE, sys__setprofileallthreads__doc__},

static PyValue
sys__setprofileallthreads_impl(PyObject *module, PyValueBorrow arg);

static PyValue
sys__setprofileallthreads(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    if (!_PyArg_CheckArityValue("sys._setprofileallthreads()", nargs, kwnames, 1)) {
        goto exit;
    }
    arg = PyValue_Borrow(args[0]);
    return_value = sys__setprofileallthreads_impl(PyValue_AsKnownObjectBorrow(module), arg);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getprofile__doc__,
"getprofile($module, /)\n"
"--\n"
"\n"
"Return the profiling function set with sys.setprofile.\n"
"\n"
"See the profiler chapter in the library manual.");

#define SYS_GETPROFILE_METHODDEF    \
    {"getprofile", _PyCFunction_CAST(sys_getprofile), METH_VALUE, sys_getprofile__doc__},

static PyValue
sys_getprofile_impl(PyObject *module);

static PyValue
sys_getprofile(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getprofile()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getprofile_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_setswitchinterval__doc__,
"setswitchinterval($module, interval, /)\n"
"--\n"
"\n"
"Set the ideal thread switching delay inside the Python interpreter.\n"
"\n"
"The actual frequency of switching threads can be lower if the\n"
"interpreter executes long sequences of uninterruptible code\n"
"(this is implementation-specific and workload-dependent).\n"
"\n"
"The parameter must represent the desired switching delay in seconds\n"
"A typical value is 0.005 (5 milliseconds).");

#define SYS_SETSWITCHINTERVAL_METHODDEF    \
    {"setswitchinterval", _PyCFunction_CAST(sys_setswitchinterval), METH_VALUE, sys_setswitchinterval__doc__},

static PyValue
sys_setswitchinterval_impl(PyObject *module, double interval);

static PyValue
sys_setswitchinterval(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double interval;

    if (!_PyArg_CheckArityValue("sys.setswitchinterval()", nargs, kwnames, 1)) {
        goto exit;
    }
    interval = _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (interval == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_setswitchinterval_impl(PyValue_AsKnownObjectBorrow(module), interval);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getswitchinterval__doc__,
"getswitchinterval($module, /)\n"
"--\n"
"\n"
"Return the current thread switch interval; see sys.setswitchinterval().");

#define SYS_GETSWITCHINTERVAL_METHODDEF    \
    {"getswitchinterval", _PyCFunction_CAST(sys_getswitchinterval), METH_VALUE, sys_getswitchinterval__doc__},

static double
sys_getswitchinterval_impl(PyObject *module);

static PyValue
sys_getswitchinterval(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    if (!_PyArg_CheckArityValue("sys.getswitchinterval()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys_getswitchinterval_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_setrecursionlimit__doc__,
"setrecursionlimit($module, limit, /)\n"
"--\n"
"\n"
"Set the maximum depth of the Python interpreter stack to n.\n"
"\n"
"This limit prevents infinite recursion from causing an overflow of the C\n"
"stack and crashing Python.  The highest possible limit is platform-\n"
"dependent.");

#define SYS_SETRECURSIONLIMIT_METHODDEF    \
    {"setrecursionlimit", _PyCFunction_CAST(sys_setrecursionlimit), METH_VALUE, sys_setrecursionlimit__doc__},

static PyValue
sys_setrecursionlimit_impl(PyObject *module, int new_limit);

static PyValue
sys_setrecursionlimit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int new_limit;

    if (!_PyArg_CheckArityValue("sys.setrecursionlimit()", nargs, kwnames, 1)) {
        goto exit;
    }
    new_limit = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (new_limit == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_setrecursionlimit_impl(PyValue_AsKnownObjectBorrow(module), new_limit);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_coroutine_origin_tracking_depth__doc__,
"set_coroutine_origin_tracking_depth($module, /, depth)\n"
"--\n"
"\n"
"Enable or disable origin tracking for coroutine objects in this thread.\n"
"\n"
"Coroutine objects will track \'depth\' frames of traceback information\n"
"about where they came from, available in their cr_origin attribute.\n"
"\n"
"Set a depth of 0 to disable.");

#define SYS_SET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF    \
    {"set_coroutine_origin_tracking_depth", _PyCFunction_CAST(sys_set_coroutine_origin_tracking_depth), METH_VALUE, sys_set_coroutine_origin_tracking_depth__doc__},

static PyValue
sys_set_coroutine_origin_tracking_depth_impl(PyObject *module, int depth);

static PyValue
sys_set_coroutine_origin_tracking_depth(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(depth)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"depth", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_coroutine_origin_tracking_depth",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int depth;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    depth = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (depth == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_set_coroutine_origin_tracking_depth_impl(PyValue_AsKnownObjectBorrow(module), depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_coroutine_origin_tracking_depth__doc__,
"get_coroutine_origin_tracking_depth($module, /)\n"
"--\n"
"\n"
"Check status of origin tracking for coroutine objects in this thread.");

#define SYS_GET_COROUTINE_ORIGIN_TRACKING_DEPTH_METHODDEF    \
    {"get_coroutine_origin_tracking_depth", _PyCFunction_CAST(sys_get_coroutine_origin_tracking_depth), METH_VALUE, sys_get_coroutine_origin_tracking_depth__doc__},

static int
sys_get_coroutine_origin_tracking_depth_impl(PyObject *module);

static PyValue
sys_get_coroutine_origin_tracking_depth(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("sys.get_coroutine_origin_tracking_depth()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys_get_coroutine_origin_tracking_depth_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_asyncgen_hooks__doc__,
"set_asyncgen_hooks($module, /, firstiter=<unrepresentable>,\n"
"                   finalizer=<unrepresentable>)\n"
"--\n"
"\n"
"Set a finalizer for async generators objects.");

#define SYS_SET_ASYNCGEN_HOOKS_METHODDEF    \
    {"set_asyncgen_hooks", _PyCFunction_CAST(sys_set_asyncgen_hooks), METH_VALUE, sys_set_asyncgen_hooks__doc__},

static PyValue
sys_set_asyncgen_hooks_impl(PyObject *module, PyValueBorrow firstiter,
                            PyValueBorrow finalizer);

static PyValue
sys_set_asyncgen_hooks(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(firstiter)), _PyValue_STATIC_IMMORTAL(&_Py_ID(finalizer)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"firstiter", "finalizer", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_asyncgen_hooks",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow firstiter = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow finalizer = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        firstiter = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        finalizer = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = sys_set_asyncgen_hooks_impl(PyValue_AsKnownObjectBorrow(module), firstiter, finalizer);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_asyncgen_hooks__doc__,
"get_asyncgen_hooks($module, /)\n"
"--\n"
"\n"
"Return the installed asynchronous generators hooks.\n"
"\n"
"This returns a namedtuple of the form (firstiter, finalizer).");

#define SYS_GET_ASYNCGEN_HOOKS_METHODDEF    \
    {"get_asyncgen_hooks", _PyCFunction_CAST(sys_get_asyncgen_hooks), METH_VALUE, sys_get_asyncgen_hooks__doc__},

static PyValue
sys_get_asyncgen_hooks_impl(PyObject *module);

static PyValue
sys_get_asyncgen_hooks(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.get_asyncgen_hooks()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_get_asyncgen_hooks_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getrecursionlimit__doc__,
"getrecursionlimit($module, /)\n"
"--\n"
"\n"
"Return the current value of the recursion limit.\n"
"\n"
"The recursion limit is the maximum depth of the Python interpreter\n"
"stack.  This limit prevents infinite recursion from causing an overflow\n"
"of the C stack and crashing Python.");

#define SYS_GETRECURSIONLIMIT_METHODDEF    \
    {"getrecursionlimit", _PyCFunction_CAST(sys_getrecursionlimit), METH_VALUE, sys_getrecursionlimit__doc__},

static PyValue
sys_getrecursionlimit_impl(PyObject *module);

static PyValue
sys_getrecursionlimit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getrecursionlimit()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getrecursionlimit_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(sys_getwindowsversion__doc__,
"getwindowsversion($module, /)\n"
"--\n"
"\n"
"Return info about the running version of Windows as a named tuple.\n"
"\n"
"The members are named: major, minor, build, platform, service_pack,\n"
"service_pack_major, service_pack_minor, suite_mask, product_type and\n"
"platform_version. For backward compatibility, only the first 5 items\n"
"are available by indexing. All elements are numbers, except\n"
"service_pack and platform_type which are strings, and platform_version\n"
"which is a 3-tuple. Platform is always 2. Product_type may be 1 for a\n"
"workstation, 2 for a domain controller, 3 for a server.\n"
"Platform_version is a 3-tuple containing a version number that is\n"
"intended for identifying the OS rather than feature detection.");

#define SYS_GETWINDOWSVERSION_METHODDEF    \
    {"getwindowsversion", _PyCFunction_CAST(sys_getwindowsversion), METH_VALUE, sys_getwindowsversion__doc__},

static PyValue
sys_getwindowsversion_impl(PyObject *module);

static PyValue
sys_getwindowsversion(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getwindowsversion()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getwindowsversion_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_DLOPEN)

PyDoc_STRVAR(sys_setdlopenflags__doc__,
"setdlopenflags($module, flags, /)\n"
"--\n"
"\n"
"Set the flags used by the interpreter for dlopen calls.\n"
"\n"
"This is used, for example, when the interpreter loads extension\n"
"modules. Among other things, this will enable a lazy resolving of\n"
"symbols when importing a module, if called as sys.setdlopenflags(0).\n"
"To share symbols across extension modules, call as\n"
"sys.setdlopenflags(os.RTLD_GLOBAL).  Symbolic names for the flag\n"
"modules can be found in the os module (RTLD_xxx constants, e.g.\n"
"os.RTLD_LAZY).");

#define SYS_SETDLOPENFLAGS_METHODDEF    \
    {"setdlopenflags", _PyCFunction_CAST(sys_setdlopenflags), METH_VALUE, sys_setdlopenflags__doc__},

static PyValue
sys_setdlopenflags_impl(PyObject *module, int new_val);

static PyValue
sys_setdlopenflags(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int new_val;

    if (!_PyArg_CheckArityValue("sys.setdlopenflags()", nargs, kwnames, 1)) {
        goto exit;
    }
    new_val = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (new_val == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_setdlopenflags_impl(PyValue_AsKnownObjectBorrow(module), new_val);

exit:
    return return_value;
}

#endif /* defined(HAVE_DLOPEN) */

#if defined(HAVE_DLOPEN)

PyDoc_STRVAR(sys_getdlopenflags__doc__,
"getdlopenflags($module, /)\n"
"--\n"
"\n"
"Return the current value of the flags that are used for dlopen calls.\n"
"\n"
"The flag constants are defined in the os module.");

#define SYS_GETDLOPENFLAGS_METHODDEF    \
    {"getdlopenflags", _PyCFunction_CAST(sys_getdlopenflags), METH_VALUE, sys_getdlopenflags__doc__},

static PyValue
sys_getdlopenflags_impl(PyObject *module);

static PyValue
sys_getdlopenflags(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getdlopenflags()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getdlopenflags_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#endif /* defined(HAVE_DLOPEN) */

#if defined(USE_MALLOPT)

PyDoc_STRVAR(sys_mdebug__doc__,
"mdebug($module, flag, /)\n"
"--\n"
"\n");

#define SYS_MDEBUG_METHODDEF    \
    {"mdebug", _PyCFunction_CAST(sys_mdebug), METH_VALUE, sys_mdebug__doc__},

static PyValue
sys_mdebug_impl(PyObject *module, int flag);

static PyValue
sys_mdebug(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag;

    if (!_PyArg_CheckArityValue("sys.mdebug()", nargs, kwnames, 1)) {
        goto exit;
    }
    flag = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_mdebug_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

#endif /* defined(USE_MALLOPT) */

PyDoc_STRVAR(sys_get_int_max_str_digits__doc__,
"get_int_max_str_digits($module, /)\n"
"--\n"
"\n"
"Return the maximum string digits limit for non-binary int<->str conversions.");

#define SYS_GET_INT_MAX_STR_DIGITS_METHODDEF    \
    {"get_int_max_str_digits", _PyCFunction_CAST(sys_get_int_max_str_digits), METH_VALUE, sys_get_int_max_str_digits__doc__},

static PyValue
sys_get_int_max_str_digits_impl(PyObject *module);

static PyValue
sys_get_int_max_str_digits(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.get_int_max_str_digits()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_get_int_max_str_digits_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_int_max_str_digits__doc__,
"set_int_max_str_digits($module, /, maxdigits)\n"
"--\n"
"\n"
"Set the maximum string digits limit for non-binary int<->str conversions.");

#define SYS_SET_INT_MAX_STR_DIGITS_METHODDEF    \
    {"set_int_max_str_digits", _PyCFunction_CAST(sys_set_int_max_str_digits), METH_VALUE, sys_set_int_max_str_digits__doc__},

static PyValue
sys_set_int_max_str_digits_impl(PyObject *module, int maxdigits);

static PyValue
sys_set_int_max_str_digits(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(maxdigits)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"maxdigits", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_int_max_str_digits",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int maxdigits;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    maxdigits = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (maxdigits == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = sys_set_int_max_str_digits_impl(PyValue_AsKnownObjectBorrow(module), maxdigits);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getsizeof__doc__,
"getsizeof($module, /, object, default=<unrepresentable>)\n"
"--\n"
"\n"
"Return the size of object in bytes.");

#define SYS_GETSIZEOF_METHODDEF    \
    {"getsizeof", _PyCFunction_CAST(sys_getsizeof), METH_VALUE, sys_getsizeof__doc__},

static PyValue
sys_getsizeof_impl(PyObject *module, PyValueBorrow o, PyValueBorrow dflt);

static PyValue
sys_getsizeof(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(object)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"object", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getsizeof",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow o;
    PyValueBorrow dflt = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    o = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        dflt = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = sys_getsizeof_impl(PyValue_AsKnownObjectBorrow(module), o, dflt);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getrefcount__doc__,
"getrefcount($module, object, /)\n"
"--\n"
"\n"
"Return the reference count of object.\n"
"\n"
"The count returned is generally one higher than you might expect,\n"
"because it includes the (temporary) reference as an argument to\n"
"getrefcount().");

#define SYS_GETREFCOUNT_METHODDEF    \
    {"getrefcount", _PyCFunction_CAST(sys_getrefcount), METH_VALUE, sys_getrefcount__doc__},

static Py_ssize_t
sys_getrefcount_impl(PyObject *module, PyValueBorrow object);

static PyValue
sys_getrefcount(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow object;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckArityValue("sys.getrefcount()", nargs, kwnames, 1)) {
        goto exit;
    }
    object = PyValue_Borrow(args[0]);
    _return_value = sys_getrefcount_impl(PyValue_AsKnownObjectBorrow(module), object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

#if defined(Py_REF_DEBUG)

PyDoc_STRVAR(sys_gettotalrefcount__doc__,
"gettotalrefcount($module, /)\n"
"--\n"
"\n");

#define SYS_GETTOTALREFCOUNT_METHODDEF    \
    {"gettotalrefcount", _PyCFunction_CAST(sys_gettotalrefcount), METH_VALUE, sys_gettotalrefcount__doc__},

static Py_ssize_t
sys_gettotalrefcount_impl(PyObject *module);

static PyValue
sys_gettotalrefcount(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckArityValue("sys.gettotalrefcount()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys_gettotalrefcount_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

#endif /* defined(Py_REF_DEBUG) */

PyDoc_STRVAR(sys_getallocatedblocks__doc__,
"getallocatedblocks($module, /)\n"
"--\n"
"\n"
"Return the number of memory blocks currently allocated.");

#define SYS_GETALLOCATEDBLOCKS_METHODDEF    \
    {"getallocatedblocks", _PyCFunction_CAST(sys_getallocatedblocks), METH_VALUE, sys_getallocatedblocks__doc__},

static Py_ssize_t
sys_getallocatedblocks_impl(PyObject *module);

static PyValue
sys_getallocatedblocks(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckArityValue("sys.getallocatedblocks()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys_getallocatedblocks_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_getunicodeinternedsize__doc__,
"getunicodeinternedsize($module, /, *, _only_immortal=False)\n"
"--\n"
"\n"
"Return the number of elements of the unicode interned dictionary");

#define SYS_GETUNICODEINTERNEDSIZE_METHODDEF    \
    {"getunicodeinternedsize", _PyCFunction_CAST(sys_getunicodeinternedsize), METH_VALUE, sys_getunicodeinternedsize__doc__},

static Py_ssize_t
sys_getunicodeinternedsize_impl(PyObject *module, int _only_immortal);

static PyValue
sys_getunicodeinternedsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(_only_immortal)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"_only_immortal", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getunicodeinternedsize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int _only_immortal = 0;
    Py_ssize_t _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        _only_immortal = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (_only_immortal < 0) {
            goto exit;
        }
    }
    _return_value = sys_getunicodeinternedsize_impl(PyValue_AsKnownObjectBorrow(module), _only_immortal);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__getframe__doc__,
"_getframe($module, depth=0, /)\n"
"--\n"
"\n"
"Return a frame object from the call stack.\n"
"\n"
"If optional integer depth is given, return the frame object that many\n"
"calls below the top of the stack.  If that is deeper than the call\n"
"stack, ValueError is raised.  The default for depth is zero, returning\n"
"the frame at the top of the call stack.\n"
"\n"
"This function should be used for internal and specialized purposes\n"
"only.");

#define SYS__GETFRAME_METHODDEF    \
    {"_getframe", _PyCFunction_CAST(sys__getframe), METH_VALUE, sys__getframe__doc__},

static PyValue
sys__getframe_impl(PyObject *module, int depth);

static PyValue
sys__getframe(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int depth = 0;

    if (!_PyArg_NoKwnamesValue("sys._getframe()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_getframe", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    depth = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (depth == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = sys__getframe_impl(PyValue_AsKnownObjectBorrow(module), depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__current_frames__doc__,
"_current_frames($module, /)\n"
"--\n"
"\n"
"Return a dict mapping each thread\'s thread id to its current stack frame.\n"
"\n"
"This function should be used for specialized purposes only.");

#define SYS__CURRENT_FRAMES_METHODDEF    \
    {"_current_frames", _PyCFunction_CAST(sys__current_frames), METH_VALUE, sys__current_frames__doc__},

static PyValue
sys__current_frames_impl(PyObject *module);

static PyValue
sys__current_frames(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._current_frames()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__current_frames_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys__current_exceptions__doc__,
"_current_exceptions($module, /)\n"
"--\n"
"\n"
"Return a dict mapping each thread\'s identifier to its current raised exception.\n"
"\n"
"This function should be used for specialized purposes only.");

#define SYS__CURRENT_EXCEPTIONS_METHODDEF    \
    {"_current_exceptions", _PyCFunction_CAST(sys__current_exceptions), METH_VALUE, sys__current_exceptions__doc__},

static PyValue
sys__current_exceptions_impl(PyObject *module);

static PyValue
sys__current_exceptions(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._current_exceptions()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__current_exceptions_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_call_tracing__doc__,
"call_tracing($module, func, args, /)\n"
"--\n"
"\n"
"Call func(*args), while tracing is enabled.\n"
"\n"
"The tracing state is saved, and restored afterwards.  This is intended\n"
"to be called from a debugger from a checkpoint, to recursively debug\n"
"some other code.");

#define SYS_CALL_TRACING_METHODDEF    \
    {"call_tracing", _PyCFunction_CAST(sys_call_tracing), METH_VALUE, sys_call_tracing__doc__},

static PyValue
sys_call_tracing_impl(PyObject *module, PyValueBorrow func,
                      PyObject *funcargs);

static PyValue
sys_call_tracing(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow func;
    PyObject *funcargs;

    if (!_PyArg_NoKwnamesValue("sys.call_tracing()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("call_tracing", nargs, 2, 2)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("call_tracing", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    funcargs = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = sys_call_tracing_impl(PyValue_AsKnownObjectBorrow(module), func, funcargs);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__debugmallocstats__doc__,
"_debugmallocstats($module, /)\n"
"--\n"
"\n"
"Print summary info to stderr about the state of pymalloc\'s structures.\n"
"\n"
"In Py_DEBUG mode, also perform some expensive internal consistency\n"
"checks.");

#define SYS__DEBUGMALLOCSTATS_METHODDEF    \
    {"_debugmallocstats", _PyCFunction_CAST(sys__debugmallocstats), METH_VALUE, sys__debugmallocstats__doc__},

static PyValue
sys__debugmallocstats_impl(PyObject *module);

static PyValue
sys__debugmallocstats(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._debugmallocstats()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__debugmallocstats_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys__clear_type_cache__doc__,
"_clear_type_cache($module, /)\n"
"--\n"
"\n"
"Clear the internal type lookup cache.");

#define SYS__CLEAR_TYPE_CACHE_METHODDEF    \
    {"_clear_type_cache", _PyCFunction_CAST(sys__clear_type_cache), METH_VALUE, sys__clear_type_cache__doc__},

static PyValue
sys__clear_type_cache_impl(PyObject *module);

static PyValue
sys__clear_type_cache(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._clear_type_cache()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__clear_type_cache_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys__clear_internal_caches__doc__,
"_clear_internal_caches($module, /)\n"
"--\n"
"\n"
"Clear all internal performance-related caches.");

#define SYS__CLEAR_INTERNAL_CACHES_METHODDEF    \
    {"_clear_internal_caches", _PyCFunction_CAST(sys__clear_internal_caches), METH_VALUE, sys__clear_internal_caches__doc__},

static PyValue
sys__clear_internal_caches_impl(PyObject *module);

static PyValue
sys__clear_internal_caches(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._clear_internal_caches()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__clear_internal_caches_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_is_finalizing__doc__,
"is_finalizing($module, /)\n"
"--\n"
"\n"
"Return True if Python is exiting.");

#define SYS_IS_FINALIZING_METHODDEF    \
    {"is_finalizing", _PyCFunction_CAST(sys_is_finalizing), METH_VALUE, sys_is_finalizing__doc__},

static PyValue
sys_is_finalizing_impl(PyObject *module);

static PyValue
sys_is_finalizing(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.is_finalizing()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_is_finalizing_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_on__doc__,
"_stats_on($module, /)\n"
"--\n"
"\n"
"Turns on stats gathering (stats gathering is off by default).");

#define SYS__STATS_ON_METHODDEF    \
    {"_stats_on", _PyCFunction_CAST(sys__stats_on), METH_VALUE, sys__stats_on__doc__},

static PyValue
sys__stats_on_impl(PyObject *module);

static PyValue
sys__stats_on(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._stats_on()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__stats_on_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#endif /* defined(Py_STATS) */

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_off__doc__,
"_stats_off($module, /)\n"
"--\n"
"\n"
"Turns off stats gathering (stats gathering is off by default).");

#define SYS__STATS_OFF_METHODDEF    \
    {"_stats_off", _PyCFunction_CAST(sys__stats_off), METH_VALUE, sys__stats_off__doc__},

static PyValue
sys__stats_off_impl(PyObject *module);

static PyValue
sys__stats_off(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._stats_off()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__stats_off_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#endif /* defined(Py_STATS) */

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_clear__doc__,
"_stats_clear($module, /)\n"
"--\n"
"\n"
"Clears the stats.");

#define SYS__STATS_CLEAR_METHODDEF    \
    {"_stats_clear", _PyCFunction_CAST(sys__stats_clear), METH_VALUE, sys__stats_clear__doc__},

static PyValue
sys__stats_clear_impl(PyObject *module);

static PyValue
sys__stats_clear(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._stats_clear()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__stats_clear_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#endif /* defined(Py_STATS) */

#if defined(Py_STATS)

PyDoc_STRVAR(sys__stats_dump__doc__,
"_stats_dump($module, /)\n"
"--\n"
"\n"
"Dump stats to file, and clears the stats.\n"
"\n"
"Return False if no statistics were not dumped because stats gathering\n"
"was off.");

#define SYS__STATS_DUMP_METHODDEF    \
    {"_stats_dump", _PyCFunction_CAST(sys__stats_dump), METH_VALUE, sys__stats_dump__doc__},

static int
sys__stats_dump_impl(PyObject *module);

static PyValue
sys__stats_dump(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("sys._stats_dump()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys__stats_dump_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

#endif /* defined(Py_STATS) */

#if defined(ANDROID_API_LEVEL)

PyDoc_STRVAR(sys_getandroidapilevel__doc__,
"getandroidapilevel($module, /)\n"
"--\n"
"\n"
"Return the build time API version of Android as an integer.");

#define SYS_GETANDROIDAPILEVEL_METHODDEF    \
    {"getandroidapilevel", _PyCFunction_CAST(sys_getandroidapilevel), METH_VALUE, sys_getandroidapilevel__doc__},

static PyValue
sys_getandroidapilevel_impl(PyObject *module);

static PyValue
sys_getandroidapilevel(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.getandroidapilevel()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_getandroidapilevel_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

#endif /* defined(ANDROID_API_LEVEL) */

PyDoc_STRVAR(sys_activate_stack_trampoline__doc__,
"activate_stack_trampoline($module, backend, /)\n"
"--\n"
"\n"
"Activate stack profiler trampoline *backend*.");

#define SYS_ACTIVATE_STACK_TRAMPOLINE_METHODDEF    \
    {"activate_stack_trampoline", _PyCFunction_CAST(sys_activate_stack_trampoline), METH_VALUE, sys_activate_stack_trampoline__doc__},

static PyValue
sys_activate_stack_trampoline_impl(PyObject *module, const char *backend);

static PyValue
sys_activate_stack_trampoline(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *backend;

    if (!_PyArg_CheckArityValue("sys.activate_stack_trampoline()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("activate_stack_trampoline", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("activate_stack_trampoline", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    backend = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (backend == NULL) {
        goto exit;
    }
    return_value = sys_activate_stack_trampoline_impl(PyValue_AsKnownObjectBorrow(module), backend);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_deactivate_stack_trampoline__doc__,
"deactivate_stack_trampoline($module, /)\n"
"--\n"
"\n"
"Deactivate the current stack profiler trampoline backend.\n"
"\n"
"If no stack profiler is activated, this function has no effect.");

#define SYS_DEACTIVATE_STACK_TRAMPOLINE_METHODDEF    \
    {"deactivate_stack_trampoline", _PyCFunction_CAST(sys_deactivate_stack_trampoline), METH_VALUE, sys_deactivate_stack_trampoline__doc__},

static PyValue
sys_deactivate_stack_trampoline_impl(PyObject *module);

static PyValue
sys_deactivate_stack_trampoline(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.deactivate_stack_trampoline()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_deactivate_stack_trampoline_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_is_stack_trampoline_active__doc__,
"is_stack_trampoline_active($module, /)\n"
"--\n"
"\n"
"Return *True* if a stack profiler trampoline is active.");

#define SYS_IS_STACK_TRAMPOLINE_ACTIVE_METHODDEF    \
    {"is_stack_trampoline_active", _PyCFunction_CAST(sys_is_stack_trampoline_active), METH_VALUE, sys_is_stack_trampoline_active__doc__},

static PyValue
sys_is_stack_trampoline_active_impl(PyObject *module);

static PyValue
sys_is_stack_trampoline_active(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.is_stack_trampoline_active()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_is_stack_trampoline_active_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_is_remote_debug_enabled__doc__,
"is_remote_debug_enabled($module, /)\n"
"--\n"
"\n"
"Return True if remote debugging is enabled, False otherwise.");

#define SYS_IS_REMOTE_DEBUG_ENABLED_METHODDEF    \
    {"is_remote_debug_enabled", _PyCFunction_CAST(sys_is_remote_debug_enabled), METH_VALUE, sys_is_remote_debug_enabled__doc__},

static PyValue
sys_is_remote_debug_enabled_impl(PyObject *module);

static PyValue
sys_is_remote_debug_enabled(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.is_remote_debug_enabled()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_is_remote_debug_enabled_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_remote_exec__doc__,
"remote_exec($module, /, pid, script)\n"
"--\n"
"\n"
"Executes a file containing Python code in a given remote Python process.\n"
"\n"
"This function returns immediately, and the code will be executed by the\n"
"target process\'s main thread at the next available opportunity,\n"
"similarly to how signals are handled.  There is no interface to\n"
"determine when the code has been executed.  The caller is responsible\n"
"for making sure that the file still exists whenever the remote process\n"
"tries to read it and that it hasn\'t been overwritten.\n"
"\n"
"The remote process must be running a CPython interpreter of the same\n"
"major and minor version as the local process.  If either the local or\n"
"remote interpreter is pre-release (alpha, beta, or release candidate)\n"
"then the local and remote interpreters must be the same exact version.\n"
"\n"
"Args:\n"
"     pid (int): The process ID of the target Python process.\n"
"     script (str|bytes): The path to a file containing\n"
"         the Python code to be executed.");

#define SYS_REMOTE_EXEC_METHODDEF    \
    {"remote_exec", _PyCFunction_CAST(sys_remote_exec), METH_VALUE, sys_remote_exec__doc__},

static PyValue
sys_remote_exec_impl(PyObject *module, int pid, PyValueBorrow script);

static PyValue
sys_remote_exec(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pid)), _PyValue_STATIC_IMMORTAL(&_Py_ID(script)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "script", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "remote_exec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int pid;
    PyValueBorrow script;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    pid = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    script = PyValue_Borrow(args[_idx[1]]);
    return_value = sys_remote_exec_impl(PyValue_AsKnownObjectBorrow(module), pid, script);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__getframemodulename__doc__,
"_getframemodulename($module, /, depth=0)\n"
"--\n"
"\n"
"Return the name of the module for a calling frame.\n"
"\n"
"The default depth returns the module containing the call to this API.\n"
"A more typical use in a library will pass a depth of 1 to get the user\'s\n"
"module rather than the library module.\n"
"\n"
"If no frame, module, or name can be found, returns None.");

#define SYS__GETFRAMEMODULENAME_METHODDEF    \
    {"_getframemodulename", _PyCFunction_CAST(sys__getframemodulename), METH_VALUE, sys__getframemodulename__doc__},

static PyValue
sys__getframemodulename_impl(PyObject *module, int depth);

static PyValue
sys__getframemodulename(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(depth)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"depth", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_getframemodulename",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int depth = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        depth = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = sys__getframemodulename_impl(PyValue_AsKnownObjectBorrow(module), depth);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__get_cpu_count_config__doc__,
"_get_cpu_count_config($module, /)\n"
"--\n"
"\n"
"Private function for getting PyConfig.cpu_count");

#define SYS__GET_CPU_COUNT_CONFIG_METHODDEF    \
    {"_get_cpu_count_config", _PyCFunction_CAST(sys__get_cpu_count_config), METH_VALUE, sys__get_cpu_count_config__doc__},

static int
sys__get_cpu_count_config_impl(PyObject *module);

static PyValue
sys__get_cpu_count_config(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("sys._get_cpu_count_config()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys__get_cpu_count_config_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(sys__baserepl__doc__,
"_baserepl($module, /)\n"
"--\n"
"\n"
"Private function for getting the base REPL");

#define SYS__BASEREPL_METHODDEF    \
    {"_baserepl", _PyCFunction_CAST(sys__baserepl), METH_VALUE, sys__baserepl__doc__},

static PyValue
sys__baserepl_impl(PyObject *module);

static PyValue
sys__baserepl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys._baserepl()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys__baserepl_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys__is_gil_enabled__doc__,
"_is_gil_enabled($module, /)\n"
"--\n"
"\n"
"Return True if the GIL is currently enabled and False otherwise.");

#define SYS__IS_GIL_ENABLED_METHODDEF    \
    {"_is_gil_enabled", _PyCFunction_CAST(sys__is_gil_enabled), METH_VALUE, sys__is_gil_enabled__doc__},

static int
sys__is_gil_enabled_impl(PyObject *module);

static PyValue
sys__is_gil_enabled(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("sys._is_gil_enabled()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = sys__is_gil_enabled_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_lazy_imports_filter__doc__,
"set_lazy_imports_filter($module, /, filter)\n"
"--\n"
"\n"
"Set the lazy imports filter callback.\n"
"\n"
"The filter is a callable which disables lazy imports when they\n"
"would otherwise be enabled. Returns True if the import is still enabled\n"
"or False to disable it. The callable is called with:\n"
"\n"
"(importing_module_name, resolved_imported_module_name, [fromlist])\n"
"\n"
"Pass None to clear the filter.");

#define SYS_SET_LAZY_IMPORTS_FILTER_METHODDEF    \
    {"set_lazy_imports_filter", _PyCFunction_CAST(sys_set_lazy_imports_filter), METH_VALUE, sys_set_lazy_imports_filter__doc__},

static PyValue
sys_set_lazy_imports_filter_impl(PyObject *module, PyValueBorrow filter);

static PyValue
sys_set_lazy_imports_filter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(filter)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"filter", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_lazy_imports_filter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow filter;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    filter = PyValue_Borrow(args[_idx[0]]);
    return_value = sys_set_lazy_imports_filter_impl(PyValue_AsKnownObjectBorrow(module), filter);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_lazy_imports_filter__doc__,
"get_lazy_imports_filter($module, /)\n"
"--\n"
"\n"
"Get the current lazy imports filter callback.\n"
"\n"
"Returns the filter callable or None if no filter is set.");

#define SYS_GET_LAZY_IMPORTS_FILTER_METHODDEF    \
    {"get_lazy_imports_filter", _PyCFunction_CAST(sys_get_lazy_imports_filter), METH_VALUE, sys_get_lazy_imports_filter__doc__},

static PyValue
sys_get_lazy_imports_filter_impl(PyObject *module);

static PyValue
sys_get_lazy_imports_filter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.get_lazy_imports_filter()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_get_lazy_imports_filter_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(sys_set_lazy_imports__doc__,
"set_lazy_imports($module, /, mode)\n"
"--\n"
"\n"
"Sets the global lazy imports mode.\n"
"\n"
"The mode parameter must be one of the following strings:\n"
"- \"all\": All top-level imports become potentially lazy\n"
"- \"normal\": Only explicitly marked imports (with \'lazy\' keyword) are\n"
"  lazy\n"
"\n"
"In addition to the mode, lazy imports can be controlled via the filter\n"
"provided to sys.set_lazy_imports_filter");

#define SYS_SET_LAZY_IMPORTS_METHODDEF    \
    {"set_lazy_imports", _PyCFunction_CAST(sys_set_lazy_imports), METH_VALUE, sys_set_lazy_imports__doc__},

static PyValue
sys_set_lazy_imports_impl(PyObject *module, PyValueBorrow mode);

static PyValue
sys_set_lazy_imports(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_lazy_imports",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow mode;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    mode = PyValue_Borrow(args[_idx[0]]);
    return_value = sys_set_lazy_imports_impl(PyValue_AsKnownObjectBorrow(module), mode);

exit:
    return return_value;
}

PyDoc_STRVAR(sys_get_lazy_imports__doc__,
"get_lazy_imports($module, /)\n"
"--\n"
"\n"
"Gets the global lazy imports mode.\n"
"\n"
"Returns \"all\" if all top level imports are potentially lazy.\n"
"Returns \"normal\" if only explicitly marked imports are lazy.");

#define SYS_GET_LAZY_IMPORTS_METHODDEF    \
    {"get_lazy_imports", _PyCFunction_CAST(sys_get_lazy_imports), METH_VALUE, sys_get_lazy_imports__doc__},

static PyValue
sys_get_lazy_imports_impl(PyObject *module);

static PyValue
sys_get_lazy_imports(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("sys.get_lazy_imports()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = sys_get_lazy_imports_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(_jit_is_available__doc__,
"is_available($module, /)\n"
"--\n"
"\n"
"Return True if the current Python executable supports JIT compilation, and False otherwise.");

#define _JIT_IS_AVAILABLE_METHODDEF    \
    {"is_available", _PyCFunction_CAST(_jit_is_available), METH_VALUE, _jit_is_available__doc__},

static int
_jit_is_available_impl(PyObject *module);

static PyValue
_jit_is_available(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("_jit.is_available()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = _jit_is_available_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_jit_is_enabled__doc__,
"is_enabled($module, /)\n"
"--\n"
"\n"
"Return True if JIT compilation is enabled for the current Python process (implies sys._jit.is_available()), and False otherwise.");

#define _JIT_IS_ENABLED_METHODDEF    \
    {"is_enabled", _PyCFunction_CAST(_jit_is_enabled), METH_VALUE, _jit_is_enabled__doc__},

static int
_jit_is_enabled_impl(PyObject *module);

static PyValue
_jit_is_enabled(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("_jit.is_enabled()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = _jit_is_enabled_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_jit_is_active__doc__,
"is_active($module, /)\n"
"--\n"
"\n"
"Return True if the topmost Python frame is currently executing JIT code (implies sys._jit.is_enabled()), and False otherwise.");

#define _JIT_IS_ACTIVE_METHODDEF    \
    {"is_active", _PyCFunction_CAST(_jit_is_active), METH_VALUE, _jit_is_active__doc__},

static int
_jit_is_active_impl(PyObject *module);

static PyValue
_jit_is_active(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("_jit.is_active()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = _jit_is_active_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

#ifndef SYS_GETWINDOWSVERSION_METHODDEF
    #define SYS_GETWINDOWSVERSION_METHODDEF
#endif /* !defined(SYS_GETWINDOWSVERSION_METHODDEF) */

#ifndef SYS_SETDLOPENFLAGS_METHODDEF
    #define SYS_SETDLOPENFLAGS_METHODDEF
#endif /* !defined(SYS_SETDLOPENFLAGS_METHODDEF) */

#ifndef SYS_GETDLOPENFLAGS_METHODDEF
    #define SYS_GETDLOPENFLAGS_METHODDEF
#endif /* !defined(SYS_GETDLOPENFLAGS_METHODDEF) */

#ifndef SYS_MDEBUG_METHODDEF
    #define SYS_MDEBUG_METHODDEF
#endif /* !defined(SYS_MDEBUG_METHODDEF) */

#ifndef SYS_GETTOTALREFCOUNT_METHODDEF
    #define SYS_GETTOTALREFCOUNT_METHODDEF
#endif /* !defined(SYS_GETTOTALREFCOUNT_METHODDEF) */

#ifndef SYS__STATS_ON_METHODDEF
    #define SYS__STATS_ON_METHODDEF
#endif /* !defined(SYS__STATS_ON_METHODDEF) */

#ifndef SYS__STATS_OFF_METHODDEF
    #define SYS__STATS_OFF_METHODDEF
#endif /* !defined(SYS__STATS_OFF_METHODDEF) */

#ifndef SYS__STATS_CLEAR_METHODDEF
    #define SYS__STATS_CLEAR_METHODDEF
#endif /* !defined(SYS__STATS_CLEAR_METHODDEF) */

#ifndef SYS__STATS_DUMP_METHODDEF
    #define SYS__STATS_DUMP_METHODDEF
#endif /* !defined(SYS__STATS_DUMP_METHODDEF) */

#ifndef SYS_GETANDROIDAPILEVEL_METHODDEF
    #define SYS_GETANDROIDAPILEVEL_METHODDEF
#endif /* !defined(SYS_GETANDROIDAPILEVEL_METHODDEF) */
/*[clinic end generated code: output=67f5b48f0c6bbf62 input=a9049054013a1b77]*/
