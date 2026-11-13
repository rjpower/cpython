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
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(code_new__doc__,
"code(argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize,\n"
"     flags, codestring, constants, names, varnames, filename, name,\n"
"     qualname, firstlineno, linetable, exceptiontable, freevars=(),\n"
"     cellvars=(), fbinit=b\'\', constslots=b\'\', /)\n"
"--\n"
"\n"
"Create a code object.  Not for the faint of heart.");

static PyValue
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              PyObject *qualname, int firstlineno, PyObject *linetable,
              PyObject *exceptiontable, PyObject *freevars,
              PyObject *cellvars, PyObject *fbinit, PyObject *constslots);

static PyValue
code_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = &PyCode_Type;
    int argcount;
    int posonlyargcount;
    int kwonlyargcount;
    int nlocals;
    int stacksize;
    int flags;
    PyObject *code;
    PyObject *consts;
    PyObject *names;
    PyObject *varnames;
    PyObject *filename;
    PyObject *name;
    PyObject *qualname;
    int firstlineno;
    PyObject *linetable;
    PyObject *exceptiontable;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;
    PyObject *fbinit = NULL;
    PyObject *constslots = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("code", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("code", nargs, 16, 20)) {
        goto exit;
    }
    argcount = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (argcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    posonlyargcount = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (posonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    kwonlyargcount = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (kwonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    nlocals = _PyValue_AsInt(PyValue_Borrow(args[3]));
    if (nlocals == -1 && PyErr_Occurred()) {
        goto exit;
    }
    stacksize = _PyValue_AsInt(PyValue_Borrow(args[4]));
    if (stacksize == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[5]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[6])), &PyBytes_Type)) {
        _PyArg_BadArgumentValue("code", "argument 7", "bytes", PyValue_Borrow(args[6]));
        goto exit;
    }
    code = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[6]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[7])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("code", "argument 8", "tuple", PyValue_Borrow(args[7]));
        goto exit;
    }
    consts = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[7]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[8])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("code", "argument 9", "tuple", PyValue_Borrow(args[8]));
        goto exit;
    }
    names = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[8]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[9])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("code", "argument 10", "tuple", PyValue_Borrow(args[9]));
        goto exit;
    }
    varnames = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[9]));
    if (!PyValue_IsObject(PyValue_Borrow(args[10]))) {
        _PyArg_BadArgumentValue("code", "argument 11", "str", PyValue_Borrow(args[10]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[10])))) {
        _PyArg_BadArgument("code", "argument 11", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[10])));
        goto exit;
    }
    filename = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[10]));
    if (!PyValue_IsObject(PyValue_Borrow(args[11]))) {
        _PyArg_BadArgumentValue("code", "argument 12", "str", PyValue_Borrow(args[11]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[11])))) {
        _PyArg_BadArgument("code", "argument 12", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[11])));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[11]));
    if (!PyValue_IsObject(PyValue_Borrow(args[12]))) {
        _PyArg_BadArgumentValue("code", "argument 13", "str", PyValue_Borrow(args[12]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[12])))) {
        _PyArg_BadArgument("code", "argument 13", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[12])));
        goto exit;
    }
    qualname = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[12]));
    firstlineno = _PyValue_AsInt(PyValue_Borrow(args[13]));
    if (firstlineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[14])), &PyBytes_Type)) {
        _PyArg_BadArgumentValue("code", "argument 15", "bytes", PyValue_Borrow(args[14]));
        goto exit;
    }
    linetable = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[14]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[15])), &PyBytes_Type)) {
        _PyArg_BadArgumentValue("code", "argument 16", "bytes", PyValue_Borrow(args[15]));
        goto exit;
    }
    exceptiontable = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[15]));
    if (nargs < 17) {
        goto skip_optional;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[16])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("code", "argument 17", "tuple", PyValue_Borrow(args[16]));
        goto exit;
    }
    freevars = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[16]));
    if (nargs < 18) {
        goto skip_optional;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[17])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("code", "argument 18", "tuple", PyValue_Borrow(args[17]));
        goto exit;
    }
    cellvars = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[17]));
    if (nargs < 19) {
        goto skip_optional;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[18])), &PyBytes_Type)) {
        _PyArg_BadArgumentValue("code", "argument 19", "bytes", PyValue_Borrow(args[18]));
        goto exit;
    }
    fbinit = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[18]));
    if (nargs < 20) {
        goto skip_optional;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[19])), &PyBytes_Type)) {
        _PyArg_BadArgumentValue("code", "argument 20", "bytes", PyValue_Borrow(args[19]));
        goto exit;
    }
    constslots = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[19]));
skip_optional:
    return_value = code_new_impl(type, argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize, flags, code, consts, names, varnames, filename, name, qualname, firstlineno, linetable, exceptiontable, freevars, cellvars, fbinit, constslots);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(code_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = code_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(code_replace__doc__,
"replace($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy of the code object with new values for the specified fields.");

#define CODE_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(code_replace), METH_VALUE|METH_KEYWORDS, code_replace__doc__},

static PyValue
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyObject *co_code, PyObject *co_consts,
                  PyObject *co_names, PyObject *co_varnames,
                  PyObject *co_freevars, PyObject *co_cellvars,
                  PyObject *co_filename, PyObject *co_name,
                  PyObject *co_qualname, PyObject *co_linetable,
                  PyObject *co_exceptiontable, PyObject *co_fbinit,
                  PyObject *co_constslots);

static PyValue
code_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 20
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(co_argcount)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_posonlyargcount)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_kwonlyargcount)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_nlocals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_stacksize)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_flags)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_firstlineno)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_code)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_consts)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_names)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_varnames)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_freevars)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_cellvars)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_filename)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_qualname)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_linetable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_exceptiontable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_fbinit)), _PyValue_STATIC_IMMORTAL(&_Py_ID(co_constslots)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"co_argcount", "co_posonlyargcount", "co_kwonlyargcount", "co_nlocals", "co_stacksize", "co_flags", "co_firstlineno", "co_code", "co_consts", "co_names", "co_varnames", "co_freevars", "co_cellvars", "co_filename", "co_name", "co_qualname", "co_linetable", "co_exceptiontable", "co_fbinit", "co_constslots", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[20];
    int co_argcount = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_argcount;
    int co_posonlyargcount = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_posonlyargcount;
    int co_kwonlyargcount = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_kwonlyargcount;
    int co_nlocals = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_nlocals;
    int co_stacksize = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_stacksize;
    int co_flags = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_flags;
    int co_firstlineno = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_firstlineno;
    PyObject *co_code = NULL;
    PyObject *co_consts = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_consts;
    PyObject *co_names = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_names;
    PyObject *co_varnames = NULL;
    PyObject *co_freevars = NULL;
    PyObject *co_cellvars = NULL;
    PyObject *co_filename = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_filename;
    PyObject *co_name = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_name;
    PyObject *co_qualname = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_qualname;
    PyObject *co_linetable = ((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_linetable;
    PyObject *co_exceptiontable = NULL;
    PyObject *co_fbinit = NULL;
    PyObject *co_constslots = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 20)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        co_argcount = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (co_argcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        co_posonlyargcount = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (co_posonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        co_kwonlyargcount = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (co_kwonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        co_nlocals = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (co_nlocals == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        co_stacksize = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (co_stacksize == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        co_flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (co_flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        co_firstlineno = _PyValue_AsInt(PyValue_Borrow(args[_idx[6]]));
        if (co_firstlineno == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[7]])), &PyBytes_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_code'", "bytes", PyValue_Borrow(args[_idx[7]]));
            goto exit;
        }
        co_code = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[7]]));
    }
    if (_idx[8] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[8]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_consts'", "tuple", PyValue_Borrow(args[_idx[8]]));
            goto exit;
        }
        co_consts = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[8]]));
    }
    if (_idx[9] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[9]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_names'", "tuple", PyValue_Borrow(args[_idx[9]]));
            goto exit;
        }
        co_names = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[9]]));
    }
    if (_idx[10] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[10]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_varnames'", "tuple", PyValue_Borrow(args[_idx[10]]));
            goto exit;
        }
        co_varnames = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[10]]));
    }
    if (_idx[11] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[11]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_freevars'", "tuple", PyValue_Borrow(args[_idx[11]]));
            goto exit;
        }
        co_freevars = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[11]]));
    }
    if (_idx[12] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[12]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_cellvars'", "tuple", PyValue_Borrow(args[_idx[12]]));
            goto exit;
        }
        co_cellvars = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[12]]));
    }
    if (_idx[13] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[13]]))) {
            _PyArg_BadArgumentValue("replace", "argument 'co_filename'", "str", PyValue_Borrow(args[_idx[13]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[13]])))) {
            _PyArg_BadArgument("replace", "argument 'co_filename'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[13]])));
            goto exit;
        }
        co_filename = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[13]]));
    }
    if (_idx[14] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[14]]))) {
            _PyArg_BadArgumentValue("replace", "argument 'co_name'", "str", PyValue_Borrow(args[_idx[14]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[14]])))) {
            _PyArg_BadArgument("replace", "argument 'co_name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[14]])));
            goto exit;
        }
        co_name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[14]]));
    }
    if (_idx[15] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[15]]))) {
            _PyArg_BadArgumentValue("replace", "argument 'co_qualname'", "str", PyValue_Borrow(args[_idx[15]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[15]])))) {
            _PyArg_BadArgument("replace", "argument 'co_qualname'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[15]])));
            goto exit;
        }
        co_qualname = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[15]]));
    }
    if (_idx[16] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[16]])), &PyBytes_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_linetable'", "bytes", PyValue_Borrow(args[_idx[16]]));
            goto exit;
        }
        co_linetable = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[16]]));
    }
    if (_idx[17] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[17]])), &PyBytes_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_exceptiontable'", "bytes", PyValue_Borrow(args[_idx[17]]));
            goto exit;
        }
        co_exceptiontable = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[17]]));
    }
    if (_idx[18] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[18]])), &PyBytes_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_fbinit'", "bytes", PyValue_Borrow(args[_idx[18]]));
            goto exit;
        }
        co_fbinit = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[18]]));
    }
    if (_idx[19] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[19]])), &PyBytes_Type)) {
            _PyArg_BadArgumentValue("replace", "argument 'co_constslots'", "bytes", PyValue_Borrow(args[_idx[19]]));
            goto exit;
        }
        co_constslots = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[19]]));
    }
    return_value = code_replace_impl((PyCodeObject *)PyValue_AsKnownObjectBorrow(self), co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals, co_stacksize, co_flags, co_firstlineno, co_code, co_consts, co_names, co_varnames, co_freevars, co_cellvars, co_filename, co_name, co_qualname, co_linetable, co_exceptiontable, co_fbinit, co_constslots);

exit:
    return return_value;
}

PyDoc_STRVAR(code__varname_from_oparg__doc__,
"_varname_from_oparg($self, /, oparg)\n"
"--\n"
"\n"
"(internal-only) Return the local variable name for the given oparg.\n"
"\n"
"WARNING: this method is for internal use only and may change or go\n"
"away.");

#define CODE__VARNAME_FROM_OPARG_METHODDEF    \
    {"_varname_from_oparg", _PyCFunction_CAST(code__varname_from_oparg), METH_VALUE|METH_KEYWORDS, code__varname_from_oparg__doc__},

static PyValue
code__varname_from_oparg_impl(PyCodeObject *self, int oparg);

static PyValue
code__varname_from_oparg(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(oparg)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"oparg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_varname_from_oparg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int oparg;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    oparg = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (oparg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = code__varname_from_oparg_impl((PyCodeObject *)PyValue_AsKnownObjectBorrow(self), oparg);

exit:
    return return_value;
}
/*[clinic end generated code: output=7e922f7f68c138e3 input=a9049054013a1b77]*/
