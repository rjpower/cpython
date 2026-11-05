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
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(warnings_acquire_lock__doc__,
"_acquire_lock($module, /)\n"
"--\n"
"\n");

#define WARNINGS_ACQUIRE_LOCK_METHODDEF    \
    {"_acquire_lock", _PyCFunction_CAST(warnings_acquire_lock), METH_VALUE|METH_NOARGS, warnings_acquire_lock__doc__},

static PyValue
warnings_acquire_lock_impl(PyObject *module);

static PyValue
warnings_acquire_lock(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = warnings_acquire_lock_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(warnings_release_lock__doc__,
"_release_lock($module, /)\n"
"--\n"
"\n");

#define WARNINGS_RELEASE_LOCK_METHODDEF    \
    {"_release_lock", _PyCFunction_CAST(warnings_release_lock), METH_VALUE|METH_NOARGS, warnings_release_lock__doc__},

static PyValue
warnings_release_lock_impl(PyObject *module);

static PyValue
warnings_release_lock(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = warnings_release_lock_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(warnings_warn__doc__,
"warn($module, /, message, category=None, stacklevel=1, source=None, *,\n"
"     skip_file_prefixes=())\n"
"--\n"
"\n"
"Issue a warning, or maybe ignore it or raise an exception.\n"
"\n"
"  message\n"
"    Text of the warning message.\n"
"  category\n"
"    The Warning category subclass. Defaults to UserWarning.\n"
"  stacklevel\n"
"    How far up the call stack to make this warning appear. A value of 2 for\n"
"    example attributes the warning to the caller of the code calling warn().\n"
"  source\n"
"    If supplied, the destroyed object which emitted a ResourceWarning\n"
"  skip_file_prefixes\n"
"    An optional tuple of module filename prefixes indicating frames to skip\n"
"    during stacklevel computations for stack frame attribution.");

#define WARNINGS_WARN_METHODDEF    \
    {"warn", _PyCFunction_CAST(warnings_warn), METH_VALUE, warnings_warn__doc__},

static PyValue
warnings_warn_impl(PyObject *module, PyValueBorrow message,
                   PyValueBorrow category, Py_ssize_t stacklevel,
                   PyValueBorrow source, PyTupleObject *skip_file_prefixes);

static PyValue
warnings_warn(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(message)), _PyValue_STATIC_IMMORTAL(&_Py_ID(category)), _PyValue_STATIC_IMMORTAL(&_Py_ID(stacklevel)), _PyValue_STATIC_IMMORTAL(&_Py_ID(source)), _PyValue_STATIC_IMMORTAL(&_Py_ID(skip_file_prefixes)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"message", "category", "stacklevel", "source", "skip_file_prefixes", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "warn",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow message;
    PyValueBorrow category = PyValue_Borrow(PyValue_None);
    Py_ssize_t stacklevel = 1;
    PyValueBorrow source = PyValue_Borrow(PyValue_None);
    PyTupleObject *skip_file_prefixes = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    message = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        category = PyValue_Borrow(args[_idx[1]]);
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
            stacklevel = ival;
        }
    }
    if (_idx[3] >= 0) {
        source = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[4]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("warn", "argument 'skip_file_prefixes'", "tuple", PyValue_Borrow(args[_idx[4]]));
            goto exit;
        }
        skip_file_prefixes = (PyTupleObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]]));
    }
    return_value = warnings_warn_impl(PyValue_AsKnownObjectBorrow(module), message, category, stacklevel, source, skip_file_prefixes);

exit:
    return return_value;
}

PyDoc_STRVAR(warnings_warn_explicit__doc__,
"warn_explicit($module, /, message, category, filename, lineno,\n"
"              module=<unrepresentable>, registry=None,\n"
"              module_globals=None, source=None)\n"
"--\n"
"\n"
"Issue a warning, or maybe ignore it or raise an exception.");

#define WARNINGS_WARN_EXPLICIT_METHODDEF    \
    {"warn_explicit", _PyCFunction_CAST(warnings_warn_explicit), METH_VALUE, warnings_warn_explicit__doc__},

static PyValue
warnings_warn_explicit_impl(PyObject *module, PyValueBorrow message,
                            PyValueBorrow category, PyObject *filename,
                            int lineno, PyValueBorrow mod,
                            PyValueBorrow registry,
                            PyValueBorrow module_globals,
                            PyValueBorrow sourceobj);

static PyValue
warnings_warn_explicit(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(message)), _PyValue_STATIC_IMMORTAL(&_Py_ID(category)), _PyValue_STATIC_IMMORTAL(&_Py_ID(filename)), _PyValue_STATIC_IMMORTAL(&_Py_ID(lineno)), _PyValue_STATIC_IMMORTAL(&_Py_ID(module)), _PyValue_STATIC_IMMORTAL(&_Py_ID(registry)), _PyValue_STATIC_IMMORTAL(&_Py_ID(module_globals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(source)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"message", "category", "filename", "lineno", "module", "registry", "module_globals", "source", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "warn_explicit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[8];
    PyValueBorrow message;
    PyValueBorrow category;
    PyObject *filename;
    int lineno;
    PyValueBorrow mod = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow registry = PyValue_Borrow(PyValue_None);
    PyValueBorrow module_globals = PyValue_Borrow(PyValue_None);
    PyValueBorrow sourceobj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 4, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, _idx, 8)) {
        goto exit;
    }
    message = PyValue_Borrow(args[_idx[0]]);
    category = PyValue_Borrow(args[_idx[1]]);
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
        _PyArg_BadArgumentValue("warn_explicit", "argument 'filename'", "str", PyValue_Borrow(args[_idx[2]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
        _PyArg_BadArgument("warn_explicit", "argument 'filename'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        goto exit;
    }
    filename = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    lineno = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
    if (lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[4] >= 0) {
        mod = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        registry = PyValue_Borrow(args[_idx[5]]);
    }
    if (_idx[6] >= 0) {
        module_globals = PyValue_Borrow(args[_idx[6]]);
    }
    if (_idx[7] >= 0) {
        sourceobj = PyValue_Borrow(args[_idx[7]]);
    }
    return_value = warnings_warn_explicit_impl(PyValue_AsKnownObjectBorrow(module), message, category, filename, lineno, mod, registry, module_globals, sourceobj);

exit:
    return return_value;
}

PyDoc_STRVAR(warnings_filters_mutated_lock_held__doc__,
"_filters_mutated_lock_held($module, /)\n"
"--\n"
"\n");

#define WARNINGS_FILTERS_MUTATED_LOCK_HELD_METHODDEF    \
    {"_filters_mutated_lock_held", _PyCFunction_CAST(warnings_filters_mutated_lock_held), METH_VALUE|METH_NOARGS, warnings_filters_mutated_lock_held__doc__},

static PyValue
warnings_filters_mutated_lock_held_impl(PyObject *module);

static PyValue
warnings_filters_mutated_lock_held(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = warnings_filters_mutated_lock_held_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}
/*[clinic end generated code: output=a99e727f480b37b1 input=a9049054013a1b77]*/
