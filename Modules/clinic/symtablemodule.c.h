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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_symtable_symtable__doc__,
"symtable($module, source, filename, startstr, /, *, module=None)\n"
"--\n"
"\n"
"Return symbol and scope dictionaries used internally by compiler.\n"
"\n"
"The source can be a string, a bytes object, or an AST object.");

#define _SYMTABLE_SYMTABLE_METHODDEF    \
    {"symtable", _PyCFunction_CAST(_symtable_symtable), METH_VALUE|METH_KEYWORDS, _symtable_symtable__doc__},

static PyValue
_symtable_symtable_impl(PyObject *module, PyValueBorrow source,
                        PyObject *filename, const char *startstr,
                        PyValueBorrow modname);

static PyValue
_symtable_symtable(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(module)), },
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
    Py_ssize_t _idx[4];
    PyValueBorrow source;
    PyObject *filename = NULL;
    const char *startstr;
    PyValueBorrow modname = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
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
        _PyArg_BadArgumentValue("symtable", "argument 3", "str", PyValue_Borrow(args[_idx[2]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
        _PyArg_BadArgument("symtable", "argument 3", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        goto exit;
    }
    startstr = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
    if (startstr == NULL) {
        goto exit;
    }
    if (_idx[3] >= 0) {
        modname = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = _symtable_symtable_impl(PyValue_AsKnownObjectBorrow(module), source, filename, startstr, modname);

exit:
    /* Cleanup for filename */
    Py_XDECREF(filename);

    return return_value;
}
/*[clinic end generated code: output=14890ceb2dae516c input=a9049054013a1b77]*/
