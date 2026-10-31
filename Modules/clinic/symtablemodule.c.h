/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(_symtable_symtable__doc__,
"symtable($module, source, filename, startstr, /, *, module=None)\n"
"--\n"
"\n"
"Return symbol and scope dictionaries used internally by compiler.\n"
"\n"
"The source can be a string, a bytes object, or an AST object.");

#define _SYMTABLE_SYMTABLE_METHODDEF    \
    {"symtable", _PyCFunction_CAST(_symtable_symtable), METH_FASTCALL|METH_KEYWORDS, _symtable_symtable__doc__},

static PyObject *
_symtable_symtable_impl(PyObject *module, PyObject *source,
                        PyObject *filename, const char *startstr,
                        PyObject *modname);

static PyValue
_symtable_symtable(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(module), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "module", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "symtable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PyObject *source;
    PyObject *filename = NULL;
    const char *startstr;
    PyObject *modname = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
            _PyArg_BadArgument("symtable", "argument 3", "str", _arg);
            goto exit;
        }
        startstr = _PyUnicode_AsUTF8NoNUL(_arg);
        if (startstr == NULL) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    modname = PyValue_Object(args[3]);
    if (modname == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = _symtable_symtable_impl(module, source, filename, startstr, modname);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for filename */
    Py_XDECREF(filename);

    return return_value;
}
/*[clinic end generated code: output=6521016e203f8cef input=a9049054013a1b77]*/
