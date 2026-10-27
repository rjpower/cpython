/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

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
    {"__import__", _PyCFunction_CAST(builtin___import__), METH_FASTCALL|METH_KEYWORDS, builtin___import____doc__},

static PyObject *
builtin___import___impl(PyObject *module, PyObject *name, PyObject *globals,
                        PyObject *locals, PyObject *fromlist, int level);

static PyValue
builtin___import__(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(globals), &_Py_ID(locals), &_Py_ID(fromlist), &_Py_ID(level), },
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
    PyValue argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *name;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    PyObject *fromlist = NULL;
    int level = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = PyValue_Object(args[0]);
    if (name == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        globals = PyValue_Object(args[1]);
        if (globals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[2])) {
        locals = PyValue_Object(args[2]);
        if (locals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[3])) {
        fromlist = PyValue_Object(args[3]);
        if (fromlist == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    level = PyLong_AsIntValue(args[4]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = builtin___import___impl(module, name, globals, locals, fromlist, level);
    return_value = PyValue_FromResult(_return_value);

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
    {"__lazy_import__", _PyCFunction_CAST(builtin___lazy_import__), METH_FASTCALL|METH_KEYWORDS, builtin___lazy_import____doc__},

static PyObject *
builtin___lazy_import___impl(PyObject *module, PyObject *name,
                             PyObject *globals, PyObject *locals,
                             PyObject *fromlist, int level);

static PyValue
builtin___lazy_import__(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(globals), &_Py_ID(locals), &_Py_ID(fromlist), &_Py_ID(level), },
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
    PyValue argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *name;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    PyObject *fromlist = NULL;
    int level = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = PyValue_Object(args[0]);
    if (name == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        globals = PyValue_Object(args[1]);
        if (globals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[2])) {
        locals = PyValue_Object(args[2]);
        if (locals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[3])) {
        fromlist = PyValue_Object(args[3]);
        if (fromlist == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    level = PyLong_AsIntValue(args[4]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = builtin___lazy_import___impl(module, name, globals, locals, fromlist, level);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_abs__doc__,
"abs($module, number, /)\n"
"--\n"
"\n"
"Return the absolute value of the argument.");

#define BUILTIN_ABS_METHODDEF    \
    {"abs", _PyCFunction_CAST(builtin_abs), METH_O, builtin_abs__doc__},

static PyObject *
builtin_abs_impl(PyObject *module, PyObject *number);

static PyValue
builtin_abs(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *number;
    PyObject *_return_value;

    number = PyValue_Object(arg);
    if (number == NULL) {
        goto exit;
    }
    _return_value = builtin_abs_impl(module, number);
    return_value = PyValue_FromResult(_return_value);

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
    {"all", _PyCFunction_CAST(builtin_all), METH_O, builtin_all__doc__},

static PyObject *
builtin_all_impl(PyObject *module, PyObject *iterable);

static PyValue
builtin_all(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *iterable;
    PyObject *_return_value;

    iterable = PyValue_Object(arg);
    if (iterable == NULL) {
        goto exit;
    }
    _return_value = builtin_all_impl(module, iterable);
    return_value = PyValue_FromResult(_return_value);

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
    {"any", _PyCFunction_CAST(builtin_any), METH_O, builtin_any__doc__},

static PyObject *
builtin_any_impl(PyObject *module, PyObject *iterable);

static PyValue
builtin_any(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *iterable;
    PyObject *_return_value;

    iterable = PyValue_Object(arg);
    if (iterable == NULL) {
        goto exit;
    }
    _return_value = builtin_any_impl(module, iterable);
    return_value = PyValue_FromResult(_return_value);

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
    {"ascii", _PyCFunction_CAST(builtin_ascii), METH_O, builtin_ascii__doc__},

static PyObject *
builtin_ascii_impl(PyObject *module, PyObject *obj);

static PyValue
builtin_ascii(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *_return_value;

    obj = PyValue_Object(arg);
    if (obj == NULL) {
        goto exit;
    }
    _return_value = builtin_ascii_impl(module, obj);
    return_value = PyValue_FromResult(_return_value);

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
    {"bin", _PyCFunction_CAST(builtin_bin), METH_O, builtin_bin__doc__},

static PyObject *
builtin_bin_impl(PyObject *module, PyObject *integer);

static PyValue
builtin_bin(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *integer;
    PyObject *_return_value;

    integer = PyValue_Object(arg);
    if (integer == NULL) {
        goto exit;
    }
    _return_value = builtin_bin_impl(module, integer);
    return_value = PyValue_FromResult(_return_value);

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
    {"callable", _PyCFunction_CAST(builtin_callable), METH_O, builtin_callable__doc__},

static PyObject *
builtin_callable_impl(PyObject *module, PyObject *obj);

static PyValue
builtin_callable(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *_return_value;

    obj = PyValue_Object(arg);
    if (obj == NULL) {
        goto exit;
    }
    _return_value = builtin_callable_impl(module, obj);
    return_value = PyValue_FromResult(_return_value);

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
    {"format", _PyCFunction_CAST(builtin_format), METH_FASTCALL, builtin_format__doc__},

static PyObject *
builtin_format_impl(PyObject *module, PyObject *value, PyObject *format_spec);

static PyValue
builtin_format(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *format_spec = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("format", nargs, 1, 2)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("format", "argument 2", "str", _arg);
            goto exit;
        }
        format_spec = _arg;
    }
skip_optional:
    _return_value = builtin_format_impl(module, value, format_spec);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_chr__doc__,
"chr($module, i, /)\n"
"--\n"
"\n"
"Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");

#define BUILTIN_CHR_METHODDEF    \
    {"chr", _PyCFunction_CAST(builtin_chr), METH_O, builtin_chr__doc__},

static PyObject *
builtin_chr_impl(PyObject *module, PyObject *i);

static PyValue
builtin_chr(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *i;
    PyObject *_return_value;

    i = PyValue_Object(arg);
    if (i == NULL) {
        goto exit;
    }
    _return_value = builtin_chr_impl(module, i);
    return_value = PyValue_FromResult(_return_value);

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
    {"compile", _PyCFunction_CAST(builtin_compile), METH_FASTCALL|METH_KEYWORDS, builtin_compile__doc__},

static PyObject *
builtin_compile_impl(PyObject *module, PyObject *source, PyObject *filename,
                     const char *mode, int flags, int dont_inherit,
                     int optimize, PyObject *modname, int feature_version);

static PyValue
builtin_compile(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(source), &_Py_ID(filename), &_Py_ID(mode), &_Py_ID(flags), &_Py_ID(dont_inherit), &_Py_ID(optimize), &_Py_ID(module), &_Py_ID(_feature_version), },
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
    PyValue argsbuf[8];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PyObject *source;
    PyObject *filename = NULL;
    const char *mode;
    int flags = 0;
    int dont_inherit = 0;
    int optimize = -1;
    PyObject *modname = Py_None;
    int feature_version = -1;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = PyValue_Object(args[0]);
    if (source == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_FSDecoder(_arg, &filename)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("compile", "argument 'mode'", "str", _arg);
            goto exit;
        }
        mode = _PyUnicode_AsUTF8NoNUL(_arg);
        if (mode == NULL) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[3])) {
        flags = PyLong_AsIntValue(args[3]);
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[4])) {
        dont_inherit = PyObject_IsTrueValue(args[4]);
        if (dont_inherit < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[5])) {
        optimize = PyLong_AsIntValue(args[5]);
        if (optimize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[6])) {
        modname = PyValue_Object(args[6]);
        if (modname == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    feature_version = PyLong_AsIntValue(args[7]);
    if (feature_version == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = builtin_compile_impl(module, source, filename, mode, flags, dont_inherit, optimize, modname, feature_version);
    return_value = PyValue_FromResult(_return_value);

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
    {"dir", _PyCFunction_CAST(builtin_dir), METH_FASTCALL, builtin_dir__doc__},

static PyObject *
builtin_dir_impl(PyObject *module, PyObject *arg);

static PyValue
builtin_dir(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("dir", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    arg = PyValue_Object(args[0]);
    if (arg == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = builtin_dir_impl(module, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_divmod__doc__,
"divmod($module, x, y, /)\n"
"--\n"
"\n"
"Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.");

#define BUILTIN_DIVMOD_METHODDEF    \
    {"divmod", _PyCFunction_CAST(builtin_divmod), METH_FASTCALL, builtin_divmod__doc__},

static PyObject *
builtin_divmod_impl(PyObject *module, PyObject *x, PyObject *y);

static PyValue
builtin_divmod(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *x;
    PyObject *y;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("divmod", nargs, 2, 2)) {
        goto exit;
    }
    x = PyValue_Object(args[0]);
    if (x == NULL) {
        goto exit;
    }
    y = PyValue_Object(args[1]);
    if (y == NULL) {
        goto exit;
    }
    _return_value = builtin_divmod_impl(module, x, y);
    return_value = PyValue_FromResult(_return_value);

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
    {"eval", _PyCFunction_CAST(builtin_eval), METH_FASTCALL|METH_KEYWORDS, builtin_eval__doc__},

static PyObject *
builtin_eval_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals);

static PyValue
builtin_eval(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(globals), &_Py_ID(locals), },
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
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = PyValue_Object(args[0]);
    if (source == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        globals = PyValue_Object(args[1]);
        if (globals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    locals = PyValue_Object(args[2]);
    if (locals == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = builtin_eval_impl(module, source, globals, locals);
    return_value = PyValue_FromResult(_return_value);

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
    {"exec", _PyCFunction_CAST(builtin_exec), METH_FASTCALL|METH_KEYWORDS, builtin_exec__doc__},

static PyObject *
builtin_exec_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals, PyObject *closure);

static PyValue
builtin_exec(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(globals), &_Py_ID(locals), &_Py_ID(closure), },
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
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;
    PyObject *closure = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = PyValue_Object(args[0]);
    if (source == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        globals = PyValue_Object(args[1]);
        if (globals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[2])) {
        locals = PyValue_Object(args[2]);
        if (locals == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    closure = PyValue_Object(args[3]);
    if (closure == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = builtin_exec_impl(module, source, globals, locals, closure);
    return_value = PyValue_FromResult(_return_value);

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
    {"globals", (PyCFunction)builtin_globals, METH_NOARGS, builtin_globals__doc__},

static PyObject *
builtin_globals_impl(PyObject *module);

static PyObject *
builtin_globals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_globals_impl(module);
}

PyDoc_STRVAR(builtin_hasattr__doc__,
"hasattr($module, obj, name, /)\n"
"--\n"
"\n"
"Return whether the object has an attribute with the given name.\n"
"\n"
"This is done by calling getattr(obj, name) and catching AttributeError.");

#define BUILTIN_HASATTR_METHODDEF    \
    {"hasattr", _PyCFunction_CAST(builtin_hasattr), METH_FASTCALL, builtin_hasattr__doc__},

static PyObject *
builtin_hasattr_impl(PyObject *module, PyObject *obj, PyObject *name);

static PyValue
builtin_hasattr(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *name;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("hasattr", nargs, 2, 2)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    name = PyValue_Object(args[1]);
    if (name == NULL) {
        goto exit;
    }
    _return_value = builtin_hasattr_impl(module, obj, name);
    return_value = PyValue_FromResult(_return_value);

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
    {"id", _PyCFunction_CAST(builtin_id), METH_O, builtin_id__doc__},

static PyObject *
builtin_id_impl(PyModuleDef *self, PyObject *v);

static PyValue
builtin_id(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *v;
    PyObject *_return_value;

    v = PyValue_Object(arg);
    if (v == NULL) {
        goto exit;
    }
    _return_value = builtin_id_impl((PyModuleDef *)self, v);
    return_value = PyValue_FromResult(_return_value);

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
    {"setattr", _PyCFunction_CAST(builtin_setattr), METH_FASTCALL, builtin_setattr__doc__},

static PyObject *
builtin_setattr_impl(PyObject *module, PyObject *obj, PyObject *name,
                     PyObject *value);

static PyValue
builtin_setattr(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *name;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("setattr", nargs, 3, 3)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    name = PyValue_Object(args[1]);
    if (name == NULL) {
        goto exit;
    }
    value = PyValue_Object(args[2]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = builtin_setattr_impl(module, obj, name, value);
    return_value = PyValue_FromResult(_return_value);

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
    {"delattr", _PyCFunction_CAST(builtin_delattr), METH_FASTCALL, builtin_delattr__doc__},

static PyObject *
builtin_delattr_impl(PyObject *module, PyObject *obj, PyObject *name);

static PyValue
builtin_delattr(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *name;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("delattr", nargs, 2, 2)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    name = PyValue_Object(args[1]);
    if (name == NULL) {
        goto exit;
    }
    _return_value = builtin_delattr_impl(module, obj, name);
    return_value = PyValue_FromResult(_return_value);

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
    {"hash", _PyCFunction_CAST(builtin_hash), METH_O, builtin_hash__doc__},

static PyObject *
builtin_hash_impl(PyObject *module, PyObject *obj);

static PyValue
builtin_hash(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *_return_value;

    obj = PyValue_Object(arg);
    if (obj == NULL) {
        goto exit;
    }
    _return_value = builtin_hash_impl(module, obj);
    return_value = PyValue_FromResult(_return_value);

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
    {"hex", _PyCFunction_CAST(builtin_hex), METH_O, builtin_hex__doc__},

static PyObject *
builtin_hex_impl(PyObject *module, PyObject *integer);

static PyValue
builtin_hex(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *integer;
    PyObject *_return_value;

    integer = PyValue_Object(arg);
    if (integer == NULL) {
        goto exit;
    }
    _return_value = builtin_hex_impl(module, integer);
    return_value = PyValue_FromResult(_return_value);

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
    {"iter", _PyCFunction_CAST(builtin_iter), METH_FASTCALL|METH_KEYWORDS, builtin_iter__doc__},

static PyObject *
builtin_iter_impl(PyObject *module, PyObject *object, PyObject *stop_value,
                  PyObject *stop_exception);

static PyValue
builtin_iter(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(stop_value), &_Py_ID(stop_exception), },
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
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *object;
    PyObject *stop_value = NULL;
    PyObject *stop_exception = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    object = PyValue_Object(args[0]);
    if (object == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        stop_value = PyValue_Object(args[1]);
        if (stop_value == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    stop_exception = PyValue_Object(args[2]);
    if (stop_exception == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = builtin_iter_impl(module, object, stop_value, stop_exception);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_len__doc__,
"len($module, obj, /)\n"
"--\n"
"\n"
"Return the number of items in a container.");

#define BUILTIN_LEN_METHODDEF    \
    {"len", _PyCFunction_CAST(builtin_len), METH_O, builtin_len__doc__},

static PyObject *
builtin_len_impl(PyObject *module, PyObject *obj);

static PyValue
builtin_len(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *_return_value;

    obj = PyValue_Object(arg);
    if (obj == NULL) {
        goto exit;
    }
    _return_value = builtin_len_impl(module, obj);
    return_value = PyValue_FromResult(_return_value);

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
    {"locals", (PyCFunction)builtin_locals, METH_NOARGS, builtin_locals__doc__},

static PyObject *
builtin_locals_impl(PyObject *module);

static PyObject *
builtin_locals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_locals_impl(module);
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
    {"oct", _PyCFunction_CAST(builtin_oct), METH_O, builtin_oct__doc__},

static PyObject *
builtin_oct_impl(PyObject *module, PyObject *integer);

static PyValue
builtin_oct(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *integer;
    PyObject *_return_value;

    integer = PyValue_Object(arg);
    if (integer == NULL) {
        goto exit;
    }
    _return_value = builtin_oct_impl(module, integer);
    return_value = PyValue_FromResult(_return_value);

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
    {"ord", _PyCFunction_CAST(builtin_ord), METH_O, builtin_ord__doc__},

static PyObject *
builtin_ord_impl(PyObject *module, PyObject *c);

static PyValue
builtin_ord(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *c;
    PyObject *_return_value;

    c = PyValue_Object(arg);
    if (c == NULL) {
        goto exit;
    }
    _return_value = builtin_ord_impl(module, c);
    return_value = PyValue_FromResult(_return_value);

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
    {"pow", _PyCFunction_CAST(builtin_pow), METH_FASTCALL|METH_KEYWORDS, builtin_pow__doc__},

static PyObject *
builtin_pow_impl(PyObject *module, PyObject *base, PyObject *exp,
                 PyObject *mod);

static PyValue
builtin_pow(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(base), &_Py_ID(exp), &_Py_ID(mod), },
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
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *base;
    PyObject *exp;
    PyObject *mod = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    base = PyValue_Object(args[0]);
    if (base == NULL) {
        goto exit;
    }
    exp = PyValue_Object(args[1]);
    if (exp == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    mod = PyValue_Object(args[2]);
    if (mod == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = builtin_pow_impl(module, base, exp, mod);
    return_value = PyValue_FromResult(_return_value);

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
    {"print", _PyCFunction_CAST(builtin_print), METH_FASTCALL|METH_KEYWORDS, builtin_print__doc__},

static PyObject *
builtin_print_impl(PyObject *module, const PyValue *objects,
                   Py_ssize_t objects_length, PyObject *sep, PyObject *end,
                   PyObject *file, int flush);

static PyValue
builtin_print(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sep), &_Py_ID(end), &_Py_ID(file), &_Py_ID(flush), },
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
    PyValue argsbuf[4];
    const PyValue *fastargs;
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const PyValue *objects;
    Py_ssize_t objects_length;
    PyObject *sep = Py_None;
    PyObject *end = Py_None;
    PyObject *file = Py_None;
    int flush = 0;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        sep = PyValue_Object(fastargs[0]);
        if (sep == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[1])) {
        end = PyValue_Object(fastargs[1]);
        if (end == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[2])) {
        file = PyValue_Object(fastargs[2]);
        if (file == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    flush = PyObject_IsTrueValue(fastargs[3]);
    if (flush < 0) {
        goto exit;
    }
skip_optional_kwonly:
    objects = args;
    objects_length = nargs;
    _return_value = builtin_print_impl(module, objects, objects_length, sep, end, file, flush);
    return_value = PyValue_FromResult(_return_value);

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
    {"input", _PyCFunction_CAST(builtin_input), METH_FASTCALL, builtin_input__doc__},

static PyObject *
builtin_input_impl(PyObject *module, PyObject *prompt);

static PyValue
builtin_input(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *prompt = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("input", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    prompt = PyValue_Object(args[0]);
    if (prompt == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = builtin_input_impl(module, prompt);
    return_value = PyValue_FromResult(_return_value);

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
    {"repr", _PyCFunction_CAST(builtin_repr), METH_O, builtin_repr__doc__},

static PyObject *
builtin_repr_impl(PyObject *module, PyObject *obj);

static PyValue
builtin_repr(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *_return_value;

    obj = PyValue_Object(arg);
    if (obj == NULL) {
        goto exit;
    }
    _return_value = builtin_repr_impl(module, obj);
    return_value = PyValue_FromResult(_return_value);

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
    {"round", _PyCFunction_CAST(builtin_round), METH_FASTCALL|METH_KEYWORDS, builtin_round__doc__},

static PyObject *
builtin_round_impl(PyObject *module, PyObject *number, PyObject *ndigits);

static PyValue
builtin_round(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(number), &_Py_ID(ndigits), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *number;
    PyObject *ndigits = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    number = PyValue_Object(args[0]);
    if (number == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    ndigits = PyValue_Object(args[1]);
    if (ndigits == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = builtin_round_impl(module, number, ndigits);
    return_value = PyValue_FromResult(_return_value);

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
    {"vars", _PyCFunction_CAST(builtin_vars), METH_FASTCALL, builtin_vars__doc__},

static PyObject *
builtin_vars_impl(PyObject *module, PyObject *v);

static PyValue
builtin_vars(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *v = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("vars", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    v = PyValue_Object(args[0]);
    if (v == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = builtin_vars_impl(module, v);
    return_value = PyValue_FromResult(_return_value);

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
    {"sum", _PyCFunction_CAST(builtin_sum), METH_FASTCALL|METH_KEYWORDS, builtin_sum__doc__},

static PyObject *
builtin_sum_impl(PyObject *module, PyObject *iterable, PyObject *start);

static PyValue
builtin_sum(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(start), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *iterable;
    PyObject *start = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    iterable = PyValue_Object(args[0]);
    if (iterable == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    start = PyValue_Object(args[1]);
    if (start == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = builtin_sum_impl(module, iterable, start);
    return_value = PyValue_FromResult(_return_value);

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
    {"isinstance", _PyCFunction_CAST(builtin_isinstance), METH_FASTCALL, builtin_isinstance__doc__},

static PyObject *
builtin_isinstance_impl(PyObject *module, PyObject *obj,
                        PyObject *class_or_tuple);

static PyValue
builtin_isinstance(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *class_or_tuple;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("isinstance", nargs, 2, 2)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    class_or_tuple = PyValue_Object(args[1]);
    if (class_or_tuple == NULL) {
        goto exit;
    }
    _return_value = builtin_isinstance_impl(module, obj, class_or_tuple);
    return_value = PyValue_FromResult(_return_value);

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
    {"issubclass", _PyCFunction_CAST(builtin_issubclass), METH_FASTCALL, builtin_issubclass__doc__},

static PyObject *
builtin_issubclass_impl(PyObject *module, PyObject *cls,
                        PyObject *class_or_tuple);

static PyValue
builtin_issubclass(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *cls;
    PyObject *class_or_tuple;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("issubclass", nargs, 2, 2)) {
        goto exit;
    }
    cls = PyValue_Object(args[0]);
    if (cls == NULL) {
        goto exit;
    }
    class_or_tuple = PyValue_Object(args[1]);
    if (class_or_tuple == NULL) {
        goto exit;
    }
    _return_value = builtin_issubclass_impl(module, cls, class_or_tuple);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=74bd3571d23f1c89 input=a9049054013a1b77]*/
