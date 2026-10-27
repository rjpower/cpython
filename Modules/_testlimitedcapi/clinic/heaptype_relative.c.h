/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(make_heaptype_with_member__doc__,
"make_heaptype_with_member($module, /, extra_base_size=0, basicsize=0,\n"
"                          member_offset=0, add_relative_flag=False, *,\n"
"                          member_name=\'memb\', member_flags=0,\n"
"                          member_type=-1)\n"
"--\n"
"\n");

#define MAKE_HEAPTYPE_WITH_MEMBER_METHODDEF    \
    {"make_heaptype_with_member", _PyCFunction_CAST(make_heaptype_with_member), METH_FASTCALL|METH_KEYWORDS, make_heaptype_with_member__doc__},

static PyObject *
make_heaptype_with_member_impl(PyObject *module, int extra_base_size,
                               int basicsize, int member_offset,
                               int add_relative_flag,
                               const char *member_name, int member_flags,
                               int member_type);

static PyValue
make_heaptype_with_member(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(extra_base_size), &_Py_ID(basicsize), &_Py_ID(member_offset), &_Py_ID(add_relative_flag), &_Py_ID(member_name), &_Py_ID(member_flags), &_Py_ID(member_type), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"extra_base_size", "basicsize", "member_offset", "add_relative_flag", "member_name", "member_flags", "member_type", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "make_heaptype_with_member",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[7];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int extra_base_size = 0;
    int basicsize = 0;
    int member_offset = 0;
    int add_relative_flag = 0;
    const char *member_name = "memb";
    int member_flags = 0;
    int member_type = Py_T_BYTE;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        extra_base_size = PyLong_AsIntValue(args[0]);
        if (extra_base_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[1])) {
        basicsize = PyLong_AsIntValue(args[1]);
        if (basicsize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[2])) {
        member_offset = PyLong_AsIntValue(args[2]);
        if (member_offset == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[3])) {
        add_relative_flag = PyObject_IsTrueValue(args[3]);
        if (add_relative_flag < 0) {
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
    if (!PyValue_IsNull(args[4])) {
        {
            PyObject *_arg = PyValue_Object(args[4]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("make_heaptype_with_member", "argument 'member_name'", "str", _arg);
                goto exit;
            }
            member_name = _PyUnicode_AsUTF8NoNUL(_arg);
            if (member_name == NULL) {
                goto exit;
            }
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[5])) {
        member_flags = PyLong_AsIntValue(args[5]);
        if (member_flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    member_type = PyLong_AsIntValue(args[6]);
    if (member_type == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = make_heaptype_with_member_impl(module, extra_base_size, basicsize, member_offset, add_relative_flag, member_name, member_flags, member_type);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=e00f59ec16f3655c input=a9049054013a1b77]*/
