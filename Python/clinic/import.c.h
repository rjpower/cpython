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
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_imp_lock_held__doc__,
"lock_held($module, /)\n"
"--\n"
"\n"
"Return True if the import lock is currently held, else False.\n"
"\n"
"On platforms without threads, return False.");

#define _IMP_LOCK_HELD_METHODDEF    \
    {"lock_held", _PyCFunction_CAST(_imp_lock_held), METH_VALUE|METH_NOARGS, _imp_lock_held__doc__},

static PyValue
_imp_lock_held_impl(PyObject *module);

static PyValue
_imp_lock_held(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _imp_lock_held_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_imp_acquire_lock__doc__,
"acquire_lock($module, /)\n"
"--\n"
"\n"
"Acquires the interpreter\'s import lock for the current thread.\n"
"\n"
"This lock should be used by import hooks to ensure thread-safety when\n"
"importing modules.  On platforms without threads, this function does\n"
"nothing.");

#define _IMP_ACQUIRE_LOCK_METHODDEF    \
    {"acquire_lock", _PyCFunction_CAST(_imp_acquire_lock), METH_VALUE|METH_NOARGS, _imp_acquire_lock__doc__},

static PyValue
_imp_acquire_lock_impl(PyObject *module);

static PyValue
_imp_acquire_lock(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _imp_acquire_lock_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_imp_release_lock__doc__,
"release_lock($module, /)\n"
"--\n"
"\n"
"Release the interpreter\'s import lock.\n"
"\n"
"On platforms without threads, this function does nothing.");

#define _IMP_RELEASE_LOCK_METHODDEF    \
    {"release_lock", _PyCFunction_CAST(_imp_release_lock), METH_VALUE|METH_NOARGS, _imp_release_lock__doc__},

static PyValue
_imp_release_lock_impl(PyObject *module);

static PyValue
_imp_release_lock(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _imp_release_lock_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_imp__fix_co_filename__doc__,
"_fix_co_filename($module, code, path, /)\n"
"--\n"
"\n"
"Changes code.co_filename to specify the passed-in file path.\n"
"\n"
"  code\n"
"    Code object to change.\n"
"  path\n"
"    File path to use.");

#define _IMP__FIX_CO_FILENAME_METHODDEF    \
    {"_fix_co_filename", _PyCFunction_CAST(_imp__fix_co_filename), METH_VALUE, _imp__fix_co_filename__doc__},

static PyValue
_imp__fix_co_filename_impl(PyObject *module, PyCodeObject *code,
                           PyObject *path);

static PyValue
_imp__fix_co_filename(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyCodeObject *code;
    PyObject *path;

    if (!_PyArg_CheckPositional("_fix_co_filename", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyCode_Type)) {
        _PyArg_BadArgumentValue("_fix_co_filename", "argument 1", (&PyCode_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    code = (PyCodeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("_fix_co_filename", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("_fix_co_filename", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    path = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _imp__fix_co_filename_impl(PyValue_AsKnownObjectBorrow(module), code, path);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_create_builtin__doc__,
"create_builtin($module, spec, /)\n"
"--\n"
"\n"
"Create an extension module.");

#define _IMP_CREATE_BUILTIN_METHODDEF    \
    {"create_builtin", _PyCFunction_CAST(_imp_create_builtin), METH_VALUE|METH_O, _imp_create_builtin__doc__},

static PyValue
_imp_create_builtin_impl(PyObject *module, PyValueBorrow spec);

static PyValue
_imp_create_builtin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow spec;

    spec = PyValue_Borrow(args[0]);
    return_value = _imp_create_builtin_impl(PyValue_AsKnownObjectBorrow(module), spec);

    return return_value;
}

PyDoc_STRVAR(_imp_extension_suffixes__doc__,
"extension_suffixes($module, /)\n"
"--\n"
"\n"
"Returns the list of file suffixes used to identify extension modules.");

#define _IMP_EXTENSION_SUFFIXES_METHODDEF    \
    {"extension_suffixes", _PyCFunction_CAST(_imp_extension_suffixes), METH_VALUE|METH_NOARGS, _imp_extension_suffixes__doc__},

static PyValue
_imp_extension_suffixes_impl(PyObject *module);

static PyValue
_imp_extension_suffixes(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _imp_extension_suffixes_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_imp_init_frozen__doc__,
"init_frozen($module, name, /)\n"
"--\n"
"\n"
"Initializes a frozen module.");

#define _IMP_INIT_FROZEN_METHODDEF    \
    {"init_frozen", _PyCFunction_CAST(_imp_init_frozen), METH_VALUE|METH_O, _imp_init_frozen__doc__},

static PyValue
_imp_init_frozen_impl(PyObject *module, PyObject *name);

static PyValue
_imp_init_frozen(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("init_frozen", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("init_frozen", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _imp_init_frozen_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_find_frozen__doc__,
"find_frozen($module, name, /, *, withdata=False)\n"
"--\n"
"\n"
"Return info about the corresponding frozen module (if there is one) or None.\n"
"\n"
"The returned info (a 2-tuple):\n"
"\n"
" * data         the raw marshalled bytes\n"
" * is_package   whether or not it is a package\n"
" * origname     the originally frozen module\'s name, or None if not\n"
"                a stdlib module (this will usually be the same as\n"
"                the module\'s current name)");

#define _IMP_FIND_FROZEN_METHODDEF    \
    {"find_frozen", _PyCFunction_CAST(_imp_find_frozen), METH_VALUE|METH_KEYWORDS, _imp_find_frozen__doc__},

static PyValue
_imp_find_frozen_impl(PyObject *module, PyObject *name, int withdata);

static PyValue
_imp_find_frozen(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(withdata)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "withdata", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "find_frozen",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyObject *name;
    int withdata = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("find_frozen", "argument 1", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("find_frozen", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        withdata = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (withdata < 0) {
            goto exit;
        }
    }
    return_value = _imp_find_frozen_impl(PyValue_AsKnownObjectBorrow(module), name, withdata);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_get_frozen_object__doc__,
"get_frozen_object($module, name, data=None, /)\n"
"--\n"
"\n"
"Create a code object for a frozen module.");

#define _IMP_GET_FROZEN_OBJECT_METHODDEF    \
    {"get_frozen_object", _PyCFunction_CAST(_imp_get_frozen_object), METH_VALUE, _imp_get_frozen_object__doc__},

static PyValue
_imp_get_frozen_object_impl(PyObject *module, PyObject *name,
                            PyValueBorrow dataobj);

static PyValue
_imp_get_frozen_object(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;
    PyValueBorrow dataobj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("get_frozen_object", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("get_frozen_object", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("get_frozen_object", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    dataobj = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _imp_get_frozen_object_impl(PyValue_AsKnownObjectBorrow(module), name, dataobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_is_frozen_package__doc__,
"is_frozen_package($module, name, /)\n"
"--\n"
"\n"
"Returns True if the module name is of a frozen package.");

#define _IMP_IS_FROZEN_PACKAGE_METHODDEF    \
    {"is_frozen_package", _PyCFunction_CAST(_imp_is_frozen_package), METH_VALUE|METH_O, _imp_is_frozen_package__doc__},

static PyValue
_imp_is_frozen_package_impl(PyObject *module, PyObject *name);

static PyValue
_imp_is_frozen_package(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("is_frozen_package", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("is_frozen_package", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _imp_is_frozen_package_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_is_builtin__doc__,
"is_builtin($module, name, /)\n"
"--\n"
"\n"
"Returns True if the module name corresponds to a built-in module.");

#define _IMP_IS_BUILTIN_METHODDEF    \
    {"is_builtin", _PyCFunction_CAST(_imp_is_builtin), METH_VALUE|METH_O, _imp_is_builtin__doc__},

static PyValue
_imp_is_builtin_impl(PyObject *module, PyObject *name);

static PyValue
_imp_is_builtin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("is_builtin", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("is_builtin", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _imp_is_builtin_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_is_frozen__doc__,
"is_frozen($module, name, /)\n"
"--\n"
"\n"
"Returns True if the module name corresponds to a frozen module.");

#define _IMP_IS_FROZEN_METHODDEF    \
    {"is_frozen", _PyCFunction_CAST(_imp_is_frozen), METH_VALUE|METH_O, _imp_is_frozen__doc__},

static PyValue
_imp_is_frozen_impl(PyObject *module, PyObject *name);

static PyValue
_imp_is_frozen(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("is_frozen", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("is_frozen", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _imp_is_frozen_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp__frozen_module_names__doc__,
"_frozen_module_names($module, /)\n"
"--\n"
"\n"
"Returns the list of available frozen modules.");

#define _IMP__FROZEN_MODULE_NAMES_METHODDEF    \
    {"_frozen_module_names", _PyCFunction_CAST(_imp__frozen_module_names), METH_VALUE|METH_NOARGS, _imp__frozen_module_names__doc__},

static PyValue
_imp__frozen_module_names_impl(PyObject *module);

static PyValue
_imp__frozen_module_names(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _imp__frozen_module_names_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_imp__override_frozen_modules_for_tests__doc__,
"_override_frozen_modules_for_tests($module, override, /)\n"
"--\n"
"\n"
"(internal-only) Override PyConfig.use_frozen_modules.\n"
"\n"
"(-1: \"off\", 1: \"on\", 0: no override)\n"
"See frozen_modules() in Lib/test/support/import_helper.py.");

#define _IMP__OVERRIDE_FROZEN_MODULES_FOR_TESTS_METHODDEF    \
    {"_override_frozen_modules_for_tests", _PyCFunction_CAST(_imp__override_frozen_modules_for_tests), METH_VALUE|METH_O, _imp__override_frozen_modules_for_tests__doc__},

static PyValue
_imp__override_frozen_modules_for_tests_impl(PyObject *module, int override);

static PyValue
_imp__override_frozen_modules_for_tests(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int override;

    override = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (override == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _imp__override_frozen_modules_for_tests_impl(PyValue_AsKnownObjectBorrow(module), override);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp__override_multi_interp_extensions_check__doc__,
"_override_multi_interp_extensions_check($module, override, /)\n"
"--\n"
"\n"
"(internal-only) Override PyInterpreterConfig.check_multi_interp_extensions.\n"
"\n"
"(-1: \"never\", 1: \"always\", 0: no override)");

#define _IMP__OVERRIDE_MULTI_INTERP_EXTENSIONS_CHECK_METHODDEF    \
    {"_override_multi_interp_extensions_check", _PyCFunction_CAST(_imp__override_multi_interp_extensions_check), METH_VALUE|METH_O, _imp__override_multi_interp_extensions_check__doc__},

static PyValue
_imp__override_multi_interp_extensions_check_impl(PyObject *module,
                                                  int override);

static PyValue
_imp__override_multi_interp_extensions_check(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int override;

    override = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (override == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _imp__override_multi_interp_extensions_check_impl(PyValue_AsKnownObjectBorrow(module), override);

exit:
    return return_value;
}

#if defined(HAVE_DYNAMIC_LOADING)

PyDoc_STRVAR(_imp_create_dynamic__doc__,
"create_dynamic($module, spec, file=<unrepresentable>, /)\n"
"--\n"
"\n"
"Create an extension module.");

#define _IMP_CREATE_DYNAMIC_METHODDEF    \
    {"create_dynamic", _PyCFunction_CAST(_imp_create_dynamic), METH_VALUE, _imp_create_dynamic__doc__},

static PyValue
_imp_create_dynamic_impl(PyObject *module, PyValueBorrow spec,
                         PyValueBorrow file);

static PyValue
_imp_create_dynamic(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow spec;
    PyValueBorrow file = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("create_dynamic", nargs, 1, 2)) {
        goto exit;
    }
    spec = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    file = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _imp_create_dynamic_impl(PyValue_AsKnownObjectBorrow(module), spec, file);

exit:
    return return_value;
}

#endif /* defined(HAVE_DYNAMIC_LOADING) */

#if defined(HAVE_DYNAMIC_LOADING)

PyDoc_STRVAR(_imp_exec_dynamic__doc__,
"exec_dynamic($module, mod, /)\n"
"--\n"
"\n"
"Initialize an extension module.");

#define _IMP_EXEC_DYNAMIC_METHODDEF    \
    {"exec_dynamic", _PyCFunction_CAST(_imp_exec_dynamic), METH_VALUE|METH_O, _imp_exec_dynamic__doc__},

static int
_imp_exec_dynamic_impl(PyObject *module, PyValueBorrow mod);

static PyValue
_imp_exec_dynamic(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow mod;
    int _return_value;

    mod = PyValue_Borrow(args[0]);
    _return_value = _imp_exec_dynamic_impl(PyValue_AsKnownObjectBorrow(module), mod);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_DYNAMIC_LOADING) */

PyDoc_STRVAR(_imp_exec_builtin__doc__,
"exec_builtin($module, mod, /)\n"
"--\n"
"\n"
"Initialize a built-in module.");

#define _IMP_EXEC_BUILTIN_METHODDEF    \
    {"exec_builtin", _PyCFunction_CAST(_imp_exec_builtin), METH_VALUE|METH_O, _imp_exec_builtin__doc__},

static int
_imp_exec_builtin_impl(PyObject *module, PyValueBorrow mod);

static PyValue
_imp_exec_builtin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow mod;
    int _return_value;

    mod = PyValue_Borrow(args[0]);
    _return_value = _imp_exec_builtin_impl(PyValue_AsKnownObjectBorrow(module), mod);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_imp_source_hash__doc__,
"source_hash($module, /, key, source)\n"
"--\n"
"\n");

#define _IMP_SOURCE_HASH_METHODDEF    \
    {"source_hash", _PyCFunction_CAST(_imp_source_hash), METH_VALUE|METH_KEYWORDS, _imp_source_hash__doc__},

static PyValue
_imp_source_hash_impl(PyObject *module, long key, Py_buffer *source);

static PyValue
_imp_source_hash(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(source)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "source", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "source_hash",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    long key;
    Py_buffer source = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    key = _PyValue_AsLong(PyValue_Borrow(args[_idx[0]]));
    if (key == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &source, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _imp_source_hash_impl(PyValue_AsKnownObjectBorrow(module), key, &source);

exit:
    /* Cleanup for source */
    if (source.obj) {
       PyBuffer_Release(&source);
    }

    return return_value;
}

PyDoc_STRVAR(_imp__set_lazy_attributes__doc__,
"_set_lazy_attributes($module, modobj, name, /)\n"
"--\n"
"\n"
"Remove the resolved module name from sys.lazy_modules.");

#define _IMP__SET_LAZY_ATTRIBUTES_METHODDEF    \
    {"_set_lazy_attributes", _PyCFunction_CAST(_imp__set_lazy_attributes), METH_VALUE, _imp__set_lazy_attributes__doc__},

static PyValue
_imp__set_lazy_attributes_impl(PyObject *module, PyValueBorrow modobj,
                               PyObject *name);

static PyValue
_imp__set_lazy_attributes(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow modobj;
    PyObject *name;

    if (!_PyArg_CheckPositional("_set_lazy_attributes", nargs, 2, 2)) {
        goto exit;
    }
    modobj = PyValue_Borrow(args[0]);
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("_set_lazy_attributes", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("_set_lazy_attributes", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _imp__set_lazy_attributes_impl(PyValue_AsKnownObjectBorrow(module), modobj, name);

exit:
    return return_value;
}

#ifndef _IMP_CREATE_DYNAMIC_METHODDEF
    #define _IMP_CREATE_DYNAMIC_METHODDEF
#endif /* !defined(_IMP_CREATE_DYNAMIC_METHODDEF) */

#ifndef _IMP_EXEC_DYNAMIC_METHODDEF
    #define _IMP_EXEC_DYNAMIC_METHODDEF
#endif /* !defined(_IMP_EXEC_DYNAMIC_METHODDEF) */
/*[clinic end generated code: output=5cc75a87bb7ff89a input=a9049054013a1b77]*/
