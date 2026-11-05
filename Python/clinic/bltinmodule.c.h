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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(builtin___import____doc__,
"__import__($module, /, name, globals=None, locals=None, fromlist=(),\n"
"           level=0)\n"
"--\n"
"\n"
"Import a module.\n"
"\n"
"Because this function is meant for use by the Python\n"
"interpreter and not for general use, it is better to use\n"
"importlib.import_module() to programmatically import a module.\n"
"\n"
"The globals argument is only used to determine the context;\n"
"they are not modified.  The locals argument is unused.  The fromlist\n"
"should be a list of names to emulate ``from name import ...``, or an\n"
"empty list to emulate ``import name``.\n"
"When importing a module from a package, note that __import__(\'A.B\', ...)\n"
"returns package A when fromlist is empty, but its submodule B when\n"
"fromlist is not empty.  The level argument is used to determine whether\n"
"to perform absolute or relative imports: 0 is absolute, while a positive\n"
"number is the number of parent directories to search relative to the\n"
"current module.");

#define BUILTIN___IMPORT___METHODDEF    \
    {"__import__", _PyCFunction_CAST(builtin___import__), METH_VALUE, builtin___import____doc__},

static PyValue
builtin___import___impl(PyObject *module, PyValueBorrow name,
                        PyValueBorrow globals, PyValueBorrow locals,
                        PyValueBorrow fromlist, int level);

static PyValue
builtin___import__(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(globals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(locals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fromlist)), _PyValue_STATIC_IMMORTAL(&_Py_ID(level)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "globals", "locals", "fromlist", "level", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__import__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow name;
    PyValueBorrow globals = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow locals = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow fromlist = PyValue_Borrow(PyValue_NULL);
    int level = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    name = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        globals = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        locals = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        fromlist = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        level = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = builtin___import___impl(PyValue_AsKnownObjectBorrow(module), name, globals, locals, fromlist, level);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin___lazy_import____doc__,
"__lazy_import__($module, /, name, globals=None, locals=None,\n"
"                fromlist=(), level=0)\n"
"--\n"
"\n"
"Lazily imports a module.\n"
"\n"
"Returns either the module to be imported or a imp.lazy_module object\n"
"which indicates the module to be lazily imported.");

#define BUILTIN___LAZY_IMPORT___METHODDEF    \
    {"__lazy_import__", _PyCFunction_CAST(builtin___lazy_import__), METH_VALUE, builtin___lazy_import____doc__},

static PyValue
builtin___lazy_import___impl(PyObject *module, PyValueBorrow name,
                             PyValueBorrow globals, PyValueBorrow locals,
                             PyValueBorrow fromlist, int level);

static PyValue
builtin___lazy_import__(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(globals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(locals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fromlist)), _PyValue_STATIC_IMMORTAL(&_Py_ID(level)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "globals", "locals", "fromlist", "level", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__lazy_import__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow name;
    PyValueBorrow globals = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow locals = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow fromlist = PyValue_Borrow(PyValue_NULL);
    int level = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    name = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        globals = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        locals = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        fromlist = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        level = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = builtin___lazy_import___impl(PyValue_AsKnownObjectBorrow(module), name, globals, locals, fromlist, level);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_abs__doc__,
"abs($module, number, /)\n"
"--\n"
"\n"
"Return the absolute value of the argument.");

#define BUILTIN_ABS_METHODDEF    \
    {"abs", _PyCFunction_CAST(builtin_abs), METH_VALUE, builtin_abs__doc__},

static PyValue
builtin_abs_impl(PyObject *module, PyValueBorrow number);

static PyValue
builtin_abs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow number;

    if (!_PyArg_CheckArityValue("abs()", nargs, kwnames, 1)) {
        goto exit;
    }
    number = PyValue_Borrow(args[0]);
    return_value = builtin_abs_impl(PyValue_AsKnownObjectBorrow(module), number);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_all__doc__,
"all($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for all values x in the iterable.\n"
"\n"
"If the iterable is empty, return True.");

#define BUILTIN_ALL_METHODDEF    \
    {"all", _PyCFunction_CAST(builtin_all), METH_VALUE, builtin_all__doc__},

static PyValue
builtin_all_impl(PyObject *module, PyValueBorrow iterable);

static PyValue
builtin_all(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;

    if (!_PyArg_CheckArityValue("all()", nargs, kwnames, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = builtin_all_impl(PyValue_AsKnownObjectBorrow(module), iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_any__doc__,
"any($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for any x in the iterable.\n"
"\n"
"If the iterable is empty, return False.");

#define BUILTIN_ANY_METHODDEF    \
    {"any", _PyCFunction_CAST(builtin_any), METH_VALUE, builtin_any__doc__},

static PyValue
builtin_any_impl(PyObject *module, PyValueBorrow iterable);

static PyValue
builtin_any(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;

    if (!_PyArg_CheckArityValue("any()", nargs, kwnames, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = builtin_any_impl(PyValue_AsKnownObjectBorrow(module), iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_ascii__doc__,
"ascii($module, obj, /)\n"
"--\n"
"\n"
"Return an ASCII-only representation of an object.\n"
"\n"
"As repr(), return a string containing a printable representation of an\n"
"object, but escape the non-ASCII characters in the string returned by\n"
"repr() using \\\\x, \\\\u or \\\\U escapes. This generates a string similar\n"
"to that returned by repr() in Python 2.");

#define BUILTIN_ASCII_METHODDEF    \
    {"ascii", _PyCFunction_CAST(builtin_ascii), METH_VALUE, builtin_ascii__doc__},

static PyValue
builtin_ascii_impl(PyObject *module, PyValueBorrow obj);

static PyValue
builtin_ascii(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    if (!_PyArg_CheckArityValue("ascii()", nargs, kwnames, 1)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    return_value = builtin_ascii_impl(PyValue_AsKnownObjectBorrow(module), obj);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_bin__doc__,
"bin($module, integer, /)\n"
"--\n"
"\n"
"Return the binary representation of an integer.\n"
"\n"
"   >>> bin(2796202)\n"
"   \'0b1010101010101010101010\'");

#define BUILTIN_BIN_METHODDEF    \
    {"bin", _PyCFunction_CAST(builtin_bin), METH_VALUE, builtin_bin__doc__},

static PyValue
builtin_bin_impl(PyObject *module, PyValueBorrow integer);

static PyValue
builtin_bin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow integer;

    if (!_PyArg_CheckArityValue("bin()", nargs, kwnames, 1)) {
        goto exit;
    }
    integer = PyValue_Borrow(args[0]);
    return_value = builtin_bin_impl(PyValue_AsKnownObjectBorrow(module), integer);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_callable__doc__,
"callable($module, obj, /)\n"
"--\n"
"\n"
"Return whether the object is callable (i.e., some kind of function).\n"
"\n"
"Note that classes are callable, as are instances of classes with a\n"
"__call__() method.");

#define BUILTIN_CALLABLE_METHODDEF    \
    {"callable", _PyCFunction_CAST(builtin_callable), METH_VALUE, builtin_callable__doc__},

static PyValue
builtin_callable_impl(PyObject *module, PyValueBorrow obj);

static PyValue
builtin_callable(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    if (!_PyArg_CheckArityValue("callable()", nargs, kwnames, 1)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    return_value = builtin_callable_impl(PyValue_AsKnownObjectBorrow(module), obj);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_format__doc__,
"format($module, value, format_spec=\'\', /)\n"
"--\n"
"\n"
"Return type(value).__format__(value, format_spec)\n"
"\n"
"Many built-in types implement format_spec according to the\n"
"Format Specification Mini-language. See help(\'FORMATTING\').\n"
"\n"
"If type(value) does not supply a method named __format__\n"
"and format_spec is empty, then str(value) is returned.\n"
"See also help(\'SPECIALMETHODS\').");

#define BUILTIN_FORMAT_METHODDEF    \
    {"format", _PyCFunction_CAST(builtin_format), METH_VALUE, builtin_format__doc__},

static PyValue
builtin_format_impl(PyObject *module, PyValueBorrow value,
                    PyObject *format_spec);

static PyValue
builtin_format(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;
    PyObject *format_spec = NULL;

    if (!_PyArg_NoKwnamesValue("format()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("format", nargs, 1, 2)) {
        goto exit;
    }
    value = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("format", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("format", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    format_spec = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
skip_optional:
    return_value = builtin_format_impl(PyValue_AsKnownObjectBorrow(module), value, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_chr__doc__,
"chr($module, i, /)\n"
"--\n"
"\n"
"Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");

#define BUILTIN_CHR_METHODDEF    \
    {"chr", _PyCFunction_CAST(builtin_chr), METH_VALUE, builtin_chr__doc__},

static PyValue
builtin_chr_impl(PyObject *module, PyValueBorrow i);

static PyValue
builtin_chr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow i;

    if (!_PyArg_CheckArityValue("chr()", nargs, kwnames, 1)) {
        goto exit;
    }
    i = PyValue_Borrow(args[0]);
    return_value = builtin_chr_impl(PyValue_AsKnownObjectBorrow(module), i);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_compile__doc__,
"compile($module, /, source, filename, mode, flags=0,\n"
"        dont_inherit=False, optimize=-1, *, module=None,\n"
"        _feature_version=-1)\n"
"--\n"
"\n"
"Compile source into a code object that can be executed by exec() or eval().\n"
"\n"
"The source code may represent a Python module, statement or\n"
"expression.\n"
"The filename will be used for run-time error messages.\n"
"The mode must be \'exec\' to compile a module, \'single\' to compile a\n"
"single (interactive) statement, or \'eval\' to compile an expression.\n"
"The flags argument, if present, controls which future statements\n"
"influence the compilation of the code.\n"
"The dont_inherit argument, if true, stops the compilation inheriting\n"
"the effects of any future statements in effect in the code calling\n"
"compile; if absent or false these statements do influence the\n"
"compilation, in addition to any features explicitly specified.");

#define BUILTIN_COMPILE_METHODDEF    \
    {"compile", _PyCFunction_CAST(builtin_compile), METH_VALUE, builtin_compile__doc__},

static PyValue
builtin_compile_impl(PyObject *module, PyValueBorrow source,
                     PyObject *filename, const char *mode, int flags,
                     int dont_inherit, int optimize, PyValueBorrow modname,
                     int feature_version);

static PyValue
builtin_compile(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(source)), _PyValue_STATIC_IMMORTAL(&_Py_ID(filename)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), _PyValue_STATIC_IMMORTAL(&_Py_ID(dont_inherit)), _PyValue_STATIC_IMMORTAL(&_Py_ID(optimize)), _PyValue_STATIC_IMMORTAL(&_Py_ID(module)), _PyValue_STATIC_IMMORTAL(&_Py_ID(_feature_version)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"source", "filename", "mode", "flags", "dont_inherit", "optimize", "module", "_feature_version", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[8];
    PyValueBorrow source;
    PyObject *filename = NULL;
    const char *mode;
    int flags = 0;
    int dont_inherit = 0;
    int optimize = -1;
    PyValueBorrow modname = PyValue_Borrow(PyValue_None);
    int feature_version = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 8)) {
        goto exit;
    }
    source = PyValue_Borrow(args[_idx[0]]);
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "expected str, bytes or os.PathLike object, not %.200s", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (!PyUnicode_FSDecoder(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &filename)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
        _PyArg_BadArgumentValue("compile", "argument 'mode'", "str", PyValue_Borrow(args[_idx[2]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
        _PyArg_BadArgument("compile", "argument 'mode'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        goto exit;
    }
    mode = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
    if (mode == NULL) {
        goto exit;
    }
    if (_idx[3] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        dont_inherit = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (dont_inherit < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        optimize = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (optimize == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        modname = PyValue_Borrow(args[_idx[6]]);
    }
    if (_idx[7] >= 0) {
        feature_version = _PyValue_AsInt(PyValue_Borrow(args[_idx[7]]));
        if (feature_version == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = builtin_compile_impl(PyValue_AsKnownObjectBorrow(module), source, filename, mode, flags, dont_inherit, optimize, modname, feature_version);

exit:
    /* Cleanup for filename */
    Py_XDECREF(filename);

    return return_value;
}

PyDoc_STRVAR(builtin_dir__doc__,
"dir($module, object=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return an alphabetized list of the attributes of the object.\n"
"\n"
"If called without an argument, return the names in the current scope.\n"
"Else, return an alphabetized list of names comprising (some of) the\n"
"attributes of the given object, and of attributes reachable from it.\n"
"If the object supplies a method named __dir__, it will be used;\n"
"otherwise the default dir() logic is used and returns:\n"
"  for a module object: the module\'s attributes.\n"
"  for a class object:  its attributes, and recursively the attributes\n"
"    of its bases.\n"
"  for any other object: its attributes, its class\'s attributes, and\n"
"    recursively the attributes of its class\'s base classes.");

#define BUILTIN_DIR_METHODDEF    \
    {"dir", _PyCFunction_CAST(builtin_dir), METH_VALUE, builtin_dir__doc__},

static PyValue
builtin_dir_impl(PyObject *module, PyValueBorrow arg);

static PyValue
builtin_dir(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("dir()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("dir", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    arg = PyValue_Borrow(args[0]);
skip_optional:
    return_value = builtin_dir_impl(PyValue_AsKnownObjectBorrow(module), arg);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_divmod__doc__,
"divmod($module, x, y, /)\n"
"--\n"
"\n"
"Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.");

#define BUILTIN_DIVMOD_METHODDEF    \
    {"divmod", _PyCFunction_CAST(builtin_divmod), METH_VALUE, builtin_divmod__doc__},

static PyValue
builtin_divmod_impl(PyObject *module, PyValueBorrow x, PyValueBorrow y);

static PyValue
builtin_divmod(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow x;
    PyValueBorrow y;

    if (!_PyArg_NoKwnamesValue("divmod()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("divmod", nargs, 2, 2)) {
        goto exit;
    }
    x = PyValue_Borrow(args[0]);
    y = PyValue_Borrow(args[1]);
    return_value = builtin_divmod_impl(PyValue_AsKnownObjectBorrow(module), x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_eval__doc__,
"eval($module, source, /, globals=None, locals=None)\n"
"--\n"
"\n"
"Evaluate the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing a Python expression\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.");

#define BUILTIN_EVAL_METHODDEF    \
    {"eval", _PyCFunction_CAST(builtin_eval), METH_VALUE, builtin_eval__doc__},

static PyValue
builtin_eval_impl(PyObject *module, PyValueBorrow source,
                  PyValueBorrow globals, PyValueBorrow locals);

static PyValue
builtin_eval(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(globals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(locals)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "globals", "locals", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "eval",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow source;
    PyValueBorrow globals = PyValue_Borrow(PyValue_None);
    PyValueBorrow locals = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    source = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        globals = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        locals = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = builtin_eval_impl(PyValue_AsKnownObjectBorrow(module), source, globals, locals);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_exec__doc__,
"exec($module, source, /, globals=None, locals=None, *, closure=None)\n"
"--\n"
"\n"
"Execute the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing one or more Python statements\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.\n"
"The closure must be a tuple of cellvars, and can only be used\n"
"when source is a code object requiring exactly that many cellvars.");

#define BUILTIN_EXEC_METHODDEF    \
    {"exec", _PyCFunction_CAST(builtin_exec), METH_VALUE, builtin_exec__doc__},

static PyValue
builtin_exec_impl(PyObject *module, PyValueBorrow source,
                  PyValueBorrow globals, PyValueBorrow locals,
                  PyValueBorrow closure);

static PyValue
builtin_exec(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(globals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(locals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(closure)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "globals", "locals", "closure", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow source;
    PyValueBorrow globals = PyValue_Borrow(PyValue_None);
    PyValueBorrow locals = PyValue_Borrow(PyValue_None);
    PyValueBorrow closure = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    source = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        globals = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        locals = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        closure = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = builtin_exec_impl(PyValue_AsKnownObjectBorrow(module), source, globals, locals, closure);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_globals__doc__,
"globals($module, /)\n"
"--\n"
"\n"
"Return the dictionary containing the current scope\'s global variables.\n"
"\n"
"NOTE: Updates to this dictionary *will* affect name lookups in the\n"
"current global scope and vice-versa.");

#define BUILTIN_GLOBALS_METHODDEF    \
    {"globals", _PyCFunction_CAST(builtin_globals), METH_VALUE, builtin_globals__doc__},

static PyValue
builtin_globals_impl(PyObject *module);

static PyValue
builtin_globals(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("globals()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = builtin_globals_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_hasattr__doc__,
"hasattr($module, obj, name, /)\n"
"--\n"
"\n"
"Return whether the object has an attribute with the given name.\n"
"\n"
"This is done by calling getattr(obj, name) and catching AttributeError.");

#define BUILTIN_HASATTR_METHODDEF    \
    {"hasattr", _PyCFunction_CAST(builtin_hasattr), METH_VALUE, builtin_hasattr__doc__},

static PyValue
builtin_hasattr_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow name);

static PyValue
builtin_hasattr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    PyValueBorrow name;

    if (!_PyArg_NoKwnamesValue("hasattr()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("hasattr", nargs, 2, 2)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    name = PyValue_Borrow(args[1]);
    return_value = builtin_hasattr_impl(PyValue_AsKnownObjectBorrow(module), obj, name);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_id__doc__,
"id($module, obj, /)\n"
"--\n"
"\n"
"Return the identity of an object.\n"
"\n"
"This is guaranteed to be unique among simultaneously existing objects.\n"
"(CPython uses the object\'s memory address, or for some ints and floats\n"
"a number derived from the value.)");

#define BUILTIN_ID_METHODDEF    \
    {"id", _PyCFunction_CAST(builtin_id), METH_VALUE, builtin_id__doc__},

static PyValue
builtin_id_impl(PyModuleDef *self, PyValueBorrow v);

static PyValue
builtin_id(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow v;

    if (!_PyArg_CheckArityValue("id()", nargs, kwnames, 1)) {
        goto exit;
    }
    v = PyValue_Borrow(args[0]);
    return_value = builtin_id_impl((PyModuleDef *)PyValue_AsKnownObjectBorrow(self), v);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_setattr__doc__,
"setattr($module, obj, name, value, /)\n"
"--\n"
"\n"
"Sets the named attribute on the given object to the specified value.\n"
"\n"
"setattr(x, \'y\', v) is equivalent to ``x.y = v``");

#define BUILTIN_SETATTR_METHODDEF    \
    {"setattr", _PyCFunction_CAST(builtin_setattr), METH_VALUE, builtin_setattr__doc__},

static PyValue
builtin_setattr_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow name,
                     PyValueBorrow value);

static PyValue
builtin_setattr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    PyValueBorrow name;
    PyValueBorrow value;

    if (!_PyArg_NoKwnamesValue("setattr()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("setattr", nargs, 3, 3)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    name = PyValue_Borrow(args[1]);
    value = PyValue_Borrow(args[2]);
    return_value = builtin_setattr_impl(PyValue_AsKnownObjectBorrow(module), obj, name, value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_delattr__doc__,
"delattr($module, obj, name, /)\n"
"--\n"
"\n"
"Deletes the named attribute from the given object.\n"
"\n"
"delattr(x, \'y\') is equivalent to ``del x.y``");

#define BUILTIN_DELATTR_METHODDEF    \
    {"delattr", _PyCFunction_CAST(builtin_delattr), METH_VALUE, builtin_delattr__doc__},

static PyValue
builtin_delattr_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow name);

static PyValue
builtin_delattr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    PyValueBorrow name;

    if (!_PyArg_NoKwnamesValue("delattr()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("delattr", nargs, 2, 2)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    name = PyValue_Borrow(args[1]);
    return_value = builtin_delattr_impl(PyValue_AsKnownObjectBorrow(module), obj, name);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_hash__doc__,
"hash($module, obj, /)\n"
"--\n"
"\n"
"Return the integer hash value for the given object.\n"
"\n"
"Two objects that compare equal must also have the same hash value, but\n"
"the reverse is not necessarily true.  Hash values may differ between\n"
"Python processes.  Not all objects are hashable; calling hash() on an\n"
"unhashable object raises TypeError.");

#define BUILTIN_HASH_METHODDEF    \
    {"hash", _PyCFunction_CAST(builtin_hash), METH_VALUE, builtin_hash__doc__},

static PyValue
builtin_hash_impl(PyObject *module, PyValueBorrow obj);

static PyValue
builtin_hash(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    if (!_PyArg_CheckArityValue("hash()", nargs, kwnames, 1)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    return_value = builtin_hash_impl(PyValue_AsKnownObjectBorrow(module), obj);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_hex__doc__,
"hex($module, integer, /)\n"
"--\n"
"\n"
"Return the hexadecimal representation of an integer.\n"
"\n"
"   >>> hex(12648430)\n"
"   \'0xc0ffee\'");

#define BUILTIN_HEX_METHODDEF    \
    {"hex", _PyCFunction_CAST(builtin_hex), METH_VALUE, builtin_hex__doc__},

static PyValue
builtin_hex_impl(PyObject *module, PyValueBorrow integer);

static PyValue
builtin_hex(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow integer;

    if (!_PyArg_CheckArityValue("hex()", nargs, kwnames, 1)) {
        goto exit;
    }
    integer = PyValue_Borrow(args[0]);
    return_value = builtin_hex_impl(PyValue_AsKnownObjectBorrow(module), integer);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_iter__doc__,
"iter($module, object, /, [stop_value], *, stop_exception=StopIteration)\n"
"--\n"
"\n"
"Get an iterator from an object.\n"
"\n"
"In the first form, the argument must supply its own iterator, or be a\n"
"sequence.  In the second form, the callable is called until it returns\n"
"the stop value or raises the specified exception.");

#define BUILTIN_ITER_METHODDEF    \
    {"iter", _PyCFunction_CAST(builtin_iter), METH_VALUE, builtin_iter__doc__},

static PyValue
builtin_iter_impl(PyObject *module, PyValueBorrow object,
                  PyValueBorrow stop_value, PyValueBorrow stop_exception);

static PyValue
builtin_iter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(stop_value)), _PyValue_STATIC_IMMORTAL(&_Py_ID(stop_exception)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "stop_value", "stop_exception", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "iter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow object;
    PyValueBorrow stop_value = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow stop_exception = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    object = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        stop_value = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        stop_exception = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = builtin_iter_impl(PyValue_AsKnownObjectBorrow(module), object, stop_value, stop_exception);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_len__doc__,
"len($module, obj, /)\n"
"--\n"
"\n"
"Return the number of items in a container.");

#define BUILTIN_LEN_METHODDEF    \
    {"len", _PyCFunction_CAST(builtin_len), METH_VALUE, builtin_len__doc__},

static PyValue
builtin_len_impl(PyObject *module, PyValueBorrow obj);

static PyValue
builtin_len(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    if (!_PyArg_CheckArityValue("len()", nargs, kwnames, 1)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    return_value = builtin_len_impl(PyValue_AsKnownObjectBorrow(module), obj);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_locals__doc__,
"locals($module, /)\n"
"--\n"
"\n"
"Return a dictionary containing the current scope\'s local variables.\n"
"\n"
"NOTE: Whether or not updates to this dictionary will affect name\n"
"lookups in the local scope and vice-versa is *implementation\n"
"dependent* and not covered by any backwards compatibility\n"
"guarantees.");

#define BUILTIN_LOCALS_METHODDEF    \
    {"locals", _PyCFunction_CAST(builtin_locals), METH_VALUE, builtin_locals__doc__},

static PyValue
builtin_locals_impl(PyObject *module);

static PyValue
builtin_locals(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("locals()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = builtin_locals_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_oct__doc__,
"oct($module, integer, /)\n"
"--\n"
"\n"
"Return the octal representation of an integer.\n"
"\n"
"   >>> oct(342391)\n"
"   \'0o1234567\'");

#define BUILTIN_OCT_METHODDEF    \
    {"oct", _PyCFunction_CAST(builtin_oct), METH_VALUE, builtin_oct__doc__},

static PyValue
builtin_oct_impl(PyObject *module, PyValueBorrow integer);

static PyValue
builtin_oct(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow integer;

    if (!_PyArg_CheckArityValue("oct()", nargs, kwnames, 1)) {
        goto exit;
    }
    integer = PyValue_Borrow(args[0]);
    return_value = builtin_oct_impl(PyValue_AsKnownObjectBorrow(module), integer);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_ord__doc__,
"ord($module, character, /)\n"
"--\n"
"\n"
"Return the ordinal value of a character.\n"
"\n"
"If the argument is a one-character string, return the Unicode code\n"
"point of that character.\n"
"\n"
"If the argument is a bytes or bytearray object of length 1, return its\n"
"single byte value.");

#define BUILTIN_ORD_METHODDEF    \
    {"ord", _PyCFunction_CAST(builtin_ord), METH_VALUE, builtin_ord__doc__},

static PyValue
builtin_ord_impl(PyObject *module, PyValueBorrow c);

static PyValue
builtin_ord(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow c;

    if (!_PyArg_CheckArityValue("ord()", nargs, kwnames, 1)) {
        goto exit;
    }
    c = PyValue_Borrow(args[0]);
    return_value = builtin_ord_impl(PyValue_AsKnownObjectBorrow(module), c);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_pow__doc__,
"pow($module, /, base, exp, mod=None)\n"
"--\n"
"\n"
"Equivalent to base**exp with 2 arguments or base**exp % mod with 3 arguments\n"
"\n"
"Some types, such as ints, are able to use a more efficient algorithm\n"
"when invoked using the three argument form.");

#define BUILTIN_POW_METHODDEF    \
    {"pow", _PyCFunction_CAST(builtin_pow), METH_VALUE, builtin_pow__doc__},

static PyValue
builtin_pow_impl(PyObject *module, PyValueBorrow base, PyValueBorrow exp,
                 PyValueBorrow mod);

static PyValue
builtin_pow(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(base)), _PyValue_STATIC_IMMORTAL(&_Py_ID(exp)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mod)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"base", "exp", "mod", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "pow",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow base;
    PyValueBorrow exp;
    PyValueBorrow mod = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    base = PyValue_Borrow(args[_idx[0]]);
    exp = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        mod = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = builtin_pow_impl(PyValue_AsKnownObjectBorrow(module), base, exp, mod);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_print__doc__,
"print($module, /, *objects, sep=\' \', end=\'\\n\', file=None, flush=False)\n"
"--\n"
"\n"
"Prints the values to a stream, or to sys.stdout by default.\n"
"\n"
"  sep\n"
"    string inserted between values, default a space.\n"
"  end\n"
"    string appended after the last value, default a newline.\n"
"  file\n"
"    a file-like object (stream); defaults to the current sys.stdout.\n"
"  flush\n"
"    whether to forcibly flush the stream.");

#define BUILTIN_PRINT_METHODDEF    \
    {"print", _PyCFunction_CAST(builtin_print), METH_VALUE, builtin_print__doc__},

static PyValue
builtin_print_impl(PyObject *module, const PyValue *objects,
                   Py_ssize_t objects_length, PyValueBorrow sep,
                   PyValueBorrow end, PyValueBorrow file, int flush);

static PyValue
builtin_print(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(end)), _PyValue_STATIC_IMMORTAL(&_Py_ID(file)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flush)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "end", "file", "flush", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "print",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    const PyValue *objects;
    Py_ssize_t objects_length;
    PyValueBorrow sep = PyValue_Borrow(PyValue_None);
    PyValueBorrow end = PyValue_Borrow(PyValue_None);
    PyValueBorrow file = PyValue_Borrow(PyValue_None);
    int flush = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, _idx, 4)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        sep = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        end = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        file = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        flush = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (flush < 0) {
            goto exit;
        }
    }
    objects = args;
    objects_length = nargs;
    return_value = builtin_print_impl(PyValue_AsKnownObjectBorrow(module), objects, objects_length, sep, end, file, flush);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_input__doc__,
"input($module, prompt=\'\', /)\n"
"--\n"
"\n"
"Read a string from standard input.  The trailing newline is stripped.\n"
"\n"
"The prompt string, if given, is printed to standard output without a\n"
"trailing newline before reading input.\n"
"\n"
"If the user hits EOF (*nix: Ctrl-D, Windows: Ctrl-Z+Return), raise\n"
"EOFError.\n"
"On *nix systems, readline is used if available.");

#define BUILTIN_INPUT_METHODDEF    \
    {"input", _PyCFunction_CAST(builtin_input), METH_VALUE, builtin_input__doc__},

static PyValue
builtin_input_impl(PyObject *module, PyValueBorrow prompt);

static PyValue
builtin_input(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow prompt = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("input()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("input", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    prompt = PyValue_Borrow(args[0]);
skip_optional:
    return_value = builtin_input_impl(PyValue_AsKnownObjectBorrow(module), prompt);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_repr__doc__,
"repr($module, obj, /)\n"
"--\n"
"\n"
"Return the canonical string representation of the object.\n"
"\n"
"For many object types, including most builtins, eval(repr(obj)) == obj.");

#define BUILTIN_REPR_METHODDEF    \
    {"repr", _PyCFunction_CAST(builtin_repr), METH_VALUE, builtin_repr__doc__},

static PyValue
builtin_repr_impl(PyObject *module, PyValueBorrow obj);

static PyValue
builtin_repr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    if (!_PyArg_CheckArityValue("repr()", nargs, kwnames, 1)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    return_value = builtin_repr_impl(PyValue_AsKnownObjectBorrow(module), obj);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_round__doc__,
"round($module, /, number, ndigits=None)\n"
"--\n"
"\n"
"Round a number to a given precision in decimal digits.\n"
"\n"
"The return value is an integer if ndigits is omitted or None.\n"
"Otherwise the return value has the same type as the number.  ndigits\n"
"may be negative.");

#define BUILTIN_ROUND_METHODDEF    \
    {"round", _PyCFunction_CAST(builtin_round), METH_VALUE, builtin_round__doc__},

static PyValue
builtin_round_impl(PyObject *module, PyValueBorrow number,
                   PyValueBorrow ndigits);

static PyValue
builtin_round(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(number)), _PyValue_STATIC_IMMORTAL(&_Py_ID(ndigits)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"number", "ndigits", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "round",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow number;
    PyValueBorrow ndigits = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    number = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        ndigits = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = builtin_round_impl(PyValue_AsKnownObjectBorrow(module), number, ndigits);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_vars__doc__,
"vars($module, object=<unrepresentable>, /)\n"
"--\n"
"\n"
"Show vars.\n"
"\n"
"Without arguments, equivalent to locals().\n"
"With an argument, equivalent to object.__dict__.");

#define BUILTIN_VARS_METHODDEF    \
    {"vars", _PyCFunction_CAST(builtin_vars), METH_VALUE, builtin_vars__doc__},

static PyValue
builtin_vars_impl(PyObject *module, PyValueBorrow v);

static PyValue
builtin_vars(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow v = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("vars()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("vars", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    v = PyValue_Borrow(args[0]);
skip_optional:
    return_value = builtin_vars_impl(PyValue_AsKnownObjectBorrow(module), v);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_sum__doc__,
"sum($module, iterable, /, start=0)\n"
"--\n"
"\n"
"Return the sum of a \'start\' value (default: 0) plus an iterable of numbers\n"
"\n"
"When the iterable is empty, return the start value.\n"
"This function is intended specifically for use with numeric values and\n"
"may reject non-numeric types.");

#define BUILTIN_SUM_METHODDEF    \
    {"sum", _PyCFunction_CAST(builtin_sum), METH_VALUE, builtin_sum__doc__},

static PyValue
builtin_sum_impl(PyObject *module, PyValueBorrow iterable,
                 PyValueBorrow start);

static PyValue
builtin_sum(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(start)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "start", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sum",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow iterable;
    PyValueBorrow start = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        start = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = builtin_sum_impl(PyValue_AsKnownObjectBorrow(module), iterable, start);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_isinstance__doc__,
"isinstance($module, obj, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether an object is an instance of a class or of a subclass thereof.\n"
"\n"
"A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the\n"
"target to check against.  This is equivalent to ``isinstance(x, A) or\n"
"isinstance(x, B) or ...`` etc.");

#define BUILTIN_ISINSTANCE_METHODDEF    \
    {"isinstance", _PyCFunction_CAST(builtin_isinstance), METH_VALUE, builtin_isinstance__doc__},

static PyValue
builtin_isinstance_impl(PyObject *module, PyValueBorrow obj,
                        PyValueBorrow class_or_tuple);

static PyValue
builtin_isinstance(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    PyValueBorrow class_or_tuple;

    if (!_PyArg_NoKwnamesValue("isinstance()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("isinstance", nargs, 2, 2)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[0]);
    class_or_tuple = PyValue_Borrow(args[1]);
    return_value = builtin_isinstance_impl(PyValue_AsKnownObjectBorrow(module), obj, class_or_tuple);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_issubclass__doc__,
"issubclass($module, cls, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether \'cls\' is derived from another class or is the same class.\n"
"\n"
"A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the\n"
"target to check against.  This is equivalent to ``issubclass(x, A) or\n"
"issubclass(x, B) or ...``.");

#define BUILTIN_ISSUBCLASS_METHODDEF    \
    {"issubclass", _PyCFunction_CAST(builtin_issubclass), METH_VALUE, builtin_issubclass__doc__},

static PyValue
builtin_issubclass_impl(PyObject *module, PyValueBorrow cls,
                        PyValueBorrow class_or_tuple);

static PyValue
builtin_issubclass(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow cls;
    PyValueBorrow class_or_tuple;

    if (!_PyArg_NoKwnamesValue("issubclass()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("issubclass", nargs, 2, 2)) {
        goto exit;
    }
    cls = PyValue_Borrow(args[0]);
    class_or_tuple = PyValue_Borrow(args[1]);
    return_value = builtin_issubclass_impl(PyValue_AsKnownObjectBorrow(module), cls, class_or_tuple);

exit:
    return return_value;
}
/*[clinic end generated code: output=54d1675e4963a1ce input=a9049054013a1b77]*/
