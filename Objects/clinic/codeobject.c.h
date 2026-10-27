/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(code_new__doc__,
"code(argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize,\n"
"     flags, codestring, constants, names, varnames, filename, name,\n"
"     qualname, firstlineno, linetable, exceptiontable, freevars=(),\n"
"     cellvars=(), fbinit=b\'\', /)\n"
"--\n"
"\n"
"Create a code object.  Not for the faint of heart.");

static PyObject *
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              PyObject *qualname, int firstlineno, PyObject *linetable,
              PyObject *exceptiontable, PyObject *freevars,
              PyObject *cellvars, PyObject *fbinit);

static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
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

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("code", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("code", PyTuple_GET_SIZE(args), 16, 19)) {
        goto exit;
    }
    argcount = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 0));
    if (argcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    posonlyargcount = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 1));
    if (posonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    kwonlyargcount = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 2));
    if (kwonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    nlocals = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 3));
    if (nlocals == -1 && PyErr_Occurred()) {
        goto exit;
    }
    stacksize = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 4));
    if (stacksize == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flags = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 5));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 6));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 7", "bytes", _arg);
            goto exit;
        }
        code = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 7));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 8", "tuple", _arg);
            goto exit;
        }
        consts = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 8));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 9", "tuple", _arg);
            goto exit;
        }
        names = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 9));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 10", "tuple", _arg);
            goto exit;
        }
        varnames = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 10));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 11", "str", _arg);
            goto exit;
        }
        filename = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 11));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 12", "str", _arg);
            goto exit;
        }
        name = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 12));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 13", "str", _arg);
            goto exit;
        }
        qualname = _arg;
    }
    firstlineno = PyLong_AsIntValue(PyTuple_GET_VALUE(args, 13));
    if (firstlineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 14));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 15", "bytes", _arg);
            goto exit;
        }
        linetable = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 15));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 16", "bytes", _arg);
            goto exit;
        }
        exceptiontable = _arg;
    }
    if (PyTuple_GET_SIZE(args) < 17) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 16));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 17", "tuple", _arg);
            goto exit;
        }
        freevars = _arg;
    }
    if (PyTuple_GET_SIZE(args) < 18) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 17));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 18", "tuple", _arg);
            goto exit;
        }
        cellvars = _arg;
    }
    if (PyTuple_GET_SIZE(args) < 19) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 18));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            _PyArg_BadArgument("code", "argument 19", "bytes", _arg);
            goto exit;
        }
        fbinit = _arg;
    }
skip_optional:
    return_value = code_new_impl(type, argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize, flags, code, consts, names, varnames, filename, name, qualname, firstlineno, linetable, exceptiontable, freevars, cellvars, fbinit);

exit:
    return return_value;
}

PyDoc_STRVAR(code_replace__doc__,
"replace($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy of the code object with new values for the specified fields.");

#define CODE_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(code_replace), METH_FASTCALL|METH_KEYWORDS, code_replace__doc__},

static PyObject *
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyObject *co_code, PyObject *co_consts,
                  PyObject *co_names, PyObject *co_varnames,
                  PyObject *co_freevars, PyObject *co_cellvars,
                  PyObject *co_filename, PyObject *co_name,
                  PyObject *co_qualname, PyObject *co_linetable,
                  PyObject *co_exceptiontable, PyObject *co_fbinit);

static PyValue
code_replace(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 19
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(co_argcount), &_Py_ID(co_posonlyargcount), &_Py_ID(co_kwonlyargcount), &_Py_ID(co_nlocals), &_Py_ID(co_stacksize), &_Py_ID(co_flags), &_Py_ID(co_firstlineno), &_Py_ID(co_code), &_Py_ID(co_consts), &_Py_ID(co_names), &_Py_ID(co_varnames), &_Py_ID(co_freevars), &_Py_ID(co_cellvars), &_Py_ID(co_filename), &_Py_ID(co_name), &_Py_ID(co_qualname), &_Py_ID(co_linetable), &_Py_ID(co_exceptiontable), &_Py_ID(co_fbinit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"co_argcount", "co_posonlyargcount", "co_kwonlyargcount", "co_nlocals", "co_stacksize", "co_flags", "co_firstlineno", "co_code", "co_consts", "co_names", "co_varnames", "co_freevars", "co_cellvars", "co_filename", "co_name", "co_qualname", "co_linetable", "co_exceptiontable", "co_fbinit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[19];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int co_argcount = ((PyCodeObject *)self)->co_argcount;
    int co_posonlyargcount = ((PyCodeObject *)self)->co_posonlyargcount;
    int co_kwonlyargcount = ((PyCodeObject *)self)->co_kwonlyargcount;
    int co_nlocals = ((PyCodeObject *)self)->co_nlocals;
    int co_stacksize = ((PyCodeObject *)self)->co_stacksize;
    int co_flags = ((PyCodeObject *)self)->co_flags;
    int co_firstlineno = ((PyCodeObject *)self)->co_firstlineno;
    PyObject *co_code = NULL;
    PyObject *co_consts = ((PyCodeObject *)self)->co_consts;
    PyObject *co_names = ((PyCodeObject *)self)->co_names;
    PyObject *co_varnames = NULL;
    PyObject *co_freevars = NULL;
    PyObject *co_cellvars = NULL;
    PyObject *co_filename = ((PyCodeObject *)self)->co_filename;
    PyObject *co_name = ((PyCodeObject *)self)->co_name;
    PyObject *co_qualname = ((PyCodeObject *)self)->co_qualname;
    PyObject *co_linetable = ((PyCodeObject *)self)->co_linetable;
    PyObject *co_exceptiontable = ((PyCodeObject *)self)->co_exceptiontable;
    PyObject *co_fbinit = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[0])) {
        co_argcount = PyLong_AsIntValue(args[0]);
        if (co_argcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[1])) {
        co_posonlyargcount = PyLong_AsIntValue(args[1]);
        if (co_posonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[2])) {
        co_kwonlyargcount = PyLong_AsIntValue(args[2]);
        if (co_kwonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[3])) {
        co_nlocals = PyLong_AsIntValue(args[3]);
        if (co_nlocals == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[4])) {
        co_stacksize = PyLong_AsIntValue(args[4]);
        if (co_stacksize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[5])) {
        co_flags = PyLong_AsIntValue(args[5]);
        if (co_flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[6])) {
        co_firstlineno = PyLong_AsIntValue(args[6]);
        if (co_firstlineno == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[7])) {
        {
            PyObject *_arg = PyValue_Object(args[7]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyBytes_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_code'", "bytes", _arg);
                goto exit;
            }
            co_code = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[8])) {
        {
            PyObject *_arg = PyValue_Object(args[8]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyTuple_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_consts'", "tuple", _arg);
                goto exit;
            }
            co_consts = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[9])) {
        {
            PyObject *_arg = PyValue_Object(args[9]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyTuple_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_names'", "tuple", _arg);
                goto exit;
            }
            co_names = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[10])) {
        {
            PyObject *_arg = PyValue_Object(args[10]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyTuple_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_varnames'", "tuple", _arg);
                goto exit;
            }
            co_varnames = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[11])) {
        {
            PyObject *_arg = PyValue_Object(args[11]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyTuple_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_freevars'", "tuple", _arg);
                goto exit;
            }
            co_freevars = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[12])) {
        {
            PyObject *_arg = PyValue_Object(args[12]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyTuple_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_cellvars'", "tuple", _arg);
                goto exit;
            }
            co_cellvars = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[13])) {
        {
            PyObject *_arg = PyValue_Object(args[13]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_filename'", "str", _arg);
                goto exit;
            }
            co_filename = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[14])) {
        {
            PyObject *_arg = PyValue_Object(args[14]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_name'", "str", _arg);
                goto exit;
            }
            co_name = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[15])) {
        {
            PyObject *_arg = PyValue_Object(args[15]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_qualname'", "str", _arg);
                goto exit;
            }
            co_qualname = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[16])) {
        {
            PyObject *_arg = PyValue_Object(args[16]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyBytes_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_linetable'", "bytes", _arg);
                goto exit;
            }
            co_linetable = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[17])) {
        {
            PyObject *_arg = PyValue_Object(args[17]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyBytes_Check(_arg)) {
                _PyArg_BadArgument("replace", "argument 'co_exceptiontable'", "bytes", _arg);
                goto exit;
            }
            co_exceptiontable = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[18]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            _PyArg_BadArgument("replace", "argument 'co_fbinit'", "bytes", _arg);
            goto exit;
        }
        co_fbinit = _arg;
    }
skip_optional_kwonly:
    _return_value = code_replace_impl((PyCodeObject *)self, co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals, co_stacksize, co_flags, co_firstlineno, co_code, co_consts, co_names, co_varnames, co_freevars, co_cellvars, co_filename, co_name, co_qualname, co_linetable, co_exceptiontable, co_fbinit);
    return_value = PyValue_FromResult(_return_value);

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
    {"_varname_from_oparg", _PyCFunction_CAST(code__varname_from_oparg), METH_FASTCALL|METH_KEYWORDS, code__varname_from_oparg__doc__},

static PyObject *
code__varname_from_oparg_impl(PyCodeObject *self, int oparg);

static PyValue
code__varname_from_oparg(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(oparg), },
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
    PyValue argsbuf[1];
    int oparg;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    oparg = PyLong_AsIntValue(args[0]);
    if (oparg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = code__varname_from_oparg_impl((PyCodeObject *)self, oparg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=b298322a53565743 input=a9049054013a1b77]*/
