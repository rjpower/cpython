/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_ctypes_get_errno__doc__,
"get_errno($module, /)\n"
"--\n"
"\n"
"Return the current value of the ctypes-private copy of errno.");

#define _CTYPES_GET_ERRNO_METHODDEF    \
    {"get_errno", _PyCFunction_CAST(_ctypes_get_errno), METH_VALUE|METH_NOARGS, _ctypes_get_errno__doc__},

static PyValue
_ctypes_get_errno_impl(PyObject *module);

static PyValue
_ctypes_get_errno(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _ctypes_get_errno_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_ctypes_set_errno__doc__,
"set_errno($module, value, /)\n"
"--\n"
"\n");

#define _CTYPES_SET_ERRNO_METHODDEF    \
    {"set_errno", _PyCFunction_CAST(_ctypes_set_errno), METH_VALUE|METH_O, _ctypes_set_errno__doc__},

static PyValue
_ctypes_set_errno_impl(PyObject *module, int value);

static PyValue
_ctypes_set_errno(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int value;

    value = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _ctypes_set_errno_impl(PyValue_AsKnownObjectBorrow(module), value);

exit:
    return return_value;
}

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_get_last_error__doc__,
"get_last_error($module, /)\n"
"--\n"
"\n"
"Return the current value of the ctypes-private copy of the last error.");

#define _CTYPES_GET_LAST_ERROR_METHODDEF    \
    {"get_last_error", _PyCFunction_CAST(_ctypes_get_last_error), METH_VALUE|METH_NOARGS, _ctypes_get_last_error__doc__},

static PyValue
_ctypes_get_last_error_impl(PyObject *module);

static PyValue
_ctypes_get_last_error(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _ctypes_get_last_error_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_set_last_error__doc__,
"set_last_error($module, value, /)\n"
"--\n"
"\n");

#define _CTYPES_SET_LAST_ERROR_METHODDEF    \
    {"set_last_error", _PyCFunction_CAST(_ctypes_set_last_error), METH_VALUE|METH_O, _ctypes_set_last_error__doc__},

static PyValue
_ctypes_set_last_error_impl(PyObject *module, int value);

static PyValue
_ctypes_set_last_error(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int value;

    value = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _ctypes_set_last_error_impl(PyValue_AsKnownObjectBorrow(module), value);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes__check_HRESULT__doc__,
"_check_HRESULT($module, hresult, /)\n"
"--\n"
"\n");

#define _CTYPES__CHECK_HRESULT_METHODDEF    \
    {"_check_HRESULT", _PyCFunction_CAST(_ctypes__check_HRESULT), METH_VALUE|METH_O, _ctypes__check_HRESULT__doc__},

static PyValue
_ctypes__check_HRESULT_impl(PyObject *module, int hr);

static PyValue
_ctypes__check_HRESULT(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int hr;

    hr = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (hr == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _ctypes__check_HRESULT_impl(PyValue_AsKnownObjectBorrow(module), hr);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_FormatError__doc__,
"FormatError($module, code=0, /)\n"
"--\n"
"\n"
"Convert a win32 error code into a string.\n"
"\n"
"If the error code is not given, the return value of a call to\n"
"GetLastError() is used.");

#define _CTYPES_FORMATERROR_METHODDEF    \
    {"FormatError", _PyCFunction_CAST(_ctypes_FormatError), METH_VALUE, _ctypes_FormatError__doc__},

static PyValue
_ctypes_FormatError_impl(PyObject *module, int code);

static PyValue
_ctypes_FormatError(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int code = 0;

    if (!_PyArg_CheckPositional("FormatError", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    code = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _ctypes_FormatError_impl(PyValue_AsKnownObjectBorrow(module), code);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_LoadLibrary__doc__,
"LoadLibrary($module, name, load_flags=0, /)\n"
"--\n"
"\n"
"Load an executable (usually a DLL), and return a handle to it.\n"
"\n"
"The handle may be used to locate exported functions in this module.\n"
"load_flags are as defined for LoadLibraryEx in the Windows API.");

#define _CTYPES_LOADLIBRARY_METHODDEF    \
    {"LoadLibrary", _PyCFunction_CAST(_ctypes_LoadLibrary), METH_VALUE, _ctypes_LoadLibrary__doc__},

static PyValue
_ctypes_LoadLibrary_impl(PyObject *module, PyObject *nameobj, int load_flags);

static PyValue
_ctypes_LoadLibrary(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *nameobj;
    int load_flags = 0;

    if (!_PyArg_CheckPositional("LoadLibrary", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("LoadLibrary", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("LoadLibrary", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    nameobj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    load_flags = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (load_flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _ctypes_LoadLibrary_impl(PyValue_AsKnownObjectBorrow(module), nameobj, load_flags);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_FreeLibrary__doc__,
"FreeLibrary($module, handle, /)\n"
"--\n"
"\n"
"Free the handle of an executable previously loaded by LoadLibrary.");

#define _CTYPES_FREELIBRARY_METHODDEF    \
    {"FreeLibrary", _PyCFunction_CAST(_ctypes_FreeLibrary), METH_VALUE|METH_O, _ctypes_FreeLibrary__doc__},

static PyValue
_ctypes_FreeLibrary_impl(PyObject *module, void *hMod);

static PyValue
_ctypes_FreeLibrary(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    void *hMod;

    if (!_parse_voidp(PyValue_Borrow(args[0]), &hMod)) {
        goto exit;
    }
    return_value = _ctypes_FreeLibrary_impl(PyValue_AsKnownObjectBorrow(module), hMod);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_CopyComPointer__doc__,
"CopyComPointer($module, src, dst, /)\n"
"--\n"
"\n"
"Copy a COM pointer and return the HRESULT value.");

#define _CTYPES_COPYCOMPOINTER_METHODDEF    \
    {"CopyComPointer", _PyCFunction_CAST(_ctypes_CopyComPointer), METH_VALUE, _ctypes_CopyComPointer__doc__},

static PyValue
_ctypes_CopyComPointer_impl(PyObject *module, PyValueBorrow p1,
                            PyValueBorrow p2);

static PyValue
_ctypes_CopyComPointer(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow p1;
    PyValueBorrow p2;

    if (!_PyArg_CheckPositional("CopyComPointer", nargs, 2, 2)) {
        goto exit;
    }
    p1 = PyValue_Borrow(args[0]);
    p2 = PyValue_Borrow(args[1]);
    return_value = _ctypes_CopyComPointer_impl(PyValue_AsKnownObjectBorrow(module), p1, p2);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if !defined(MS_WIN32) && defined(__APPLE__)

PyDoc_STRVAR(_ctypes__dyld_shared_cache_contains_path__doc__,
"_dyld_shared_cache_contains_path($module, path, /)\n"
"--\n"
"\n"
"Check whether a path is in the shared cache.");

#define _CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF    \
    {"_dyld_shared_cache_contains_path", _PyCFunction_CAST(_ctypes__dyld_shared_cache_contains_path), METH_VALUE|METH_O, _ctypes__dyld_shared_cache_contains_path__doc__},

static PyValue
_ctypes__dyld_shared_cache_contains_path_impl(PyObject *module,
                                              PyValueBorrow name);

static PyValue
_ctypes__dyld_shared_cache_contains_path(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow name;

    name = PyValue_Borrow(args[0]);
    return_value = _ctypes__dyld_shared_cache_contains_path_impl(PyValue_AsKnownObjectBorrow(module), name);

    return return_value;
}

#endif /* !defined(MS_WIN32) && defined(__APPLE__) */

#if !defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_dlopen__doc__,
"dlopen($module, name, mode=RTLD_NOW | RTLD_LOCAL, /)\n"
"--\n"
"\n"
"Open a shared library.");

#define _CTYPES_DLOPEN_METHODDEF    \
    {"dlopen", _PyCFunction_CAST(_ctypes_dlopen), METH_VALUE, _ctypes_dlopen__doc__},

static PyValue
_ctypes_dlopen_impl(PyObject *module, PyValueBorrow name, int mode);

static PyValue
_ctypes_dlopen(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow name;
    int mode = DLOPEN_DEFAULT_MODE;

    if (!_PyArg_CheckPositional("dlopen", nargs, 1, 2)) {
        goto exit;
    }
    name = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    mode = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _ctypes_dlopen_impl(PyValue_AsKnownObjectBorrow(module), name, mode);

exit:
    return return_value;
}

#endif /* !defined(MS_WIN32) */

#if !defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_dlclose__doc__,
"dlclose($module, handle, /)\n"
"--\n"
"\n"
"Close a shared library.");

#define _CTYPES_DLCLOSE_METHODDEF    \
    {"dlclose", _PyCFunction_CAST(_ctypes_dlclose), METH_VALUE|METH_O, _ctypes_dlclose__doc__},

static PyValue
_ctypes_dlclose_impl(PyObject *module, void *handle);

static PyValue
_ctypes_dlclose(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    void *handle;

    if (!_parse_voidp(PyValue_Borrow(args[0]), &handle)) {
        goto exit;
    }
    return_value = _ctypes_dlclose_impl(PyValue_AsKnownObjectBorrow(module), handle);

exit:
    return return_value;
}

#endif /* !defined(MS_WIN32) */

#if !defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_dlsym__doc__,
"dlsym($module, handle, name, /)\n"
"--\n"
"\n"
"Find a symbol in a shared library.");

#define _CTYPES_DLSYM_METHODDEF    \
    {"dlsym", _PyCFunction_CAST(_ctypes_dlsym), METH_VALUE, _ctypes_dlsym__doc__},

static PyValue
_ctypes_dlsym_impl(PyObject *module, void *handle, const char *name);

static PyValue
_ctypes_dlsym(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    void *handle;
    const char *name;

    if (!_PyArg_CheckPositional("dlsym", nargs, 2, 2)) {
        goto exit;
    }
    if (!_parse_voidp(PyValue_Borrow(args[0]), &handle)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("dlsym", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("dlsym", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (name == NULL) {
        goto exit;
    }
    return_value = _ctypes_dlsym_impl(PyValue_AsKnownObjectBorrow(module), handle, name);

exit:
    return return_value;
}

#endif /* !defined(MS_WIN32) */

#if !defined(MS_WIN32) && (defined(HAVE_DL_ITERATE_PHDR) && !defined(__APPLE__))

PyDoc_STRVAR(_ctypes_dllist__doc__,
"dllist($module, /)\n"
"--\n"
"\n"
"Return a list of loaded shared libraries.");

#define _CTYPES_DLLIST_METHODDEF    \
    {"dllist", _PyCFunction_CAST(_ctypes_dllist), METH_VALUE|METH_NOARGS, _ctypes_dllist__doc__},

static PyValue
_ctypes_dllist_impl(PyObject *module);

static PyValue
_ctypes_dllist(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _ctypes_dllist_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* !defined(MS_WIN32) && (defined(HAVE_DL_ITERATE_PHDR) && !defined(__APPLE__)) */

PyDoc_STRVAR(_ctypes_call_function__doc__,
"call_function($module, func, arguments, /)\n"
"--\n"
"\n");

#define _CTYPES_CALL_FUNCTION_METHODDEF    \
    {"call_function", _PyCFunction_CAST(_ctypes_call_function), METH_VALUE, _ctypes_call_function__doc__},

static PyValue
_ctypes_call_function_impl(PyObject *module, void *func, PyObject *arguments);

static PyValue
_ctypes_call_function(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    void *func;
    PyObject *arguments;

    if (!_PyArg_CheckPositional("call_function", nargs, 2, 2)) {
        goto exit;
    }
    if (!_parse_voidp(PyValue_Borrow(args[0]), &func)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("call_function", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    arguments = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _ctypes_call_function_impl(PyValue_AsKnownObjectBorrow(module), func, arguments);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_call_cdeclfunction__doc__,
"call_cdeclfunction($module, func, arguments, /)\n"
"--\n"
"\n");

#define _CTYPES_CALL_CDECLFUNCTION_METHODDEF    \
    {"call_cdeclfunction", _PyCFunction_CAST(_ctypes_call_cdeclfunction), METH_VALUE, _ctypes_call_cdeclfunction__doc__},

static PyValue
_ctypes_call_cdeclfunction_impl(PyObject *module, void *func,
                                PyObject *arguments);

static PyValue
_ctypes_call_cdeclfunction(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    void *func;
    PyObject *arguments;

    if (!_PyArg_CheckPositional("call_cdeclfunction", nargs, 2, 2)) {
        goto exit;
    }
    if (!_parse_voidp(PyValue_Borrow(args[0]), &func)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("call_cdeclfunction", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    arguments = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _ctypes_call_cdeclfunction_impl(PyValue_AsKnownObjectBorrow(module), func, arguments);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_sizeof__doc__,
"sizeof($module, obj, /)\n"
"--\n"
"\n"
"Return the size in bytes of a C instance.");

#define _CTYPES_SIZEOF_METHODDEF    \
    {"sizeof", _PyCFunction_CAST(_ctypes_sizeof), METH_VALUE|METH_O, _ctypes_sizeof__doc__},

static PyValue
_ctypes_sizeof_impl(PyObject *module, PyValueBorrow obj);

static PyValue
_ctypes_sizeof(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = _ctypes_sizeof_impl(PyValue_AsKnownObjectBorrow(module), obj);

    return return_value;
}

PyDoc_STRVAR(_ctypes_alignment__doc__,
"alignment($module, obj, /)\n"
"--\n"
"\n"
"Return the alignment requirements of a C instance.\n"
"\n"
"The argument is a C type or a C instance.");

#define _CTYPES_ALIGNMENT_METHODDEF    \
    {"alignment", _PyCFunction_CAST(_ctypes_alignment), METH_VALUE|METH_O, _ctypes_alignment__doc__},

static PyValue
_ctypes_alignment_impl(PyObject *module, PyValueBorrow obj);

static PyValue
_ctypes_alignment(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = _ctypes_alignment_impl(PyValue_AsKnownObjectBorrow(module), obj);

    return return_value;
}

PyDoc_STRVAR(_ctypes_byref__doc__,
"byref($module, obj, offset=0, /)\n"
"--\n"
"\n"
"Return a pointer lookalike to a C instance, only usable as function argument.");

#define _CTYPES_BYREF_METHODDEF    \
    {"byref", _PyCFunction_CAST(_ctypes_byref), METH_VALUE, _ctypes_byref__doc__},

static PyValue
_ctypes_byref_impl(PyObject *module, PyObject *obj, Py_ssize_t offset);

static PyValue
_ctypes_byref(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    Py_ssize_t offset = 0;

    if (!_PyArg_CheckPositional("byref", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->PyCData_Type)) {
        _PyArg_BadArgumentValue("byref", "argument 1", (clinic_state()->PyCData_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    obj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(obj));
    return_value = _ctypes_byref_impl(PyValue_AsKnownObjectBorrow(module), obj, offset);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_addressof__doc__,
"addressof($module, obj, /)\n"
"--\n"
"\n"
"Return the address of the C instance internal buffer");

#define _CTYPES_ADDRESSOF_METHODDEF    \
    {"addressof", _PyCFunction_CAST(_ctypes_addressof), METH_VALUE|METH_O, _ctypes_addressof__doc__},

static PyValue
_ctypes_addressof_impl(PyObject *module, PyObject *obj);

static PyValue
_ctypes_addressof(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->PyCData_Type)) {
        _PyArg_BadArgumentValue("addressof", "argument", (clinic_state()->PyCData_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    obj = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(obj));
    return_value = _ctypes_addressof_impl(PyValue_AsKnownObjectBorrow(module), obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_PyObj_FromPtr__doc__,
"PyObj_FromPtr($module, address, /)\n"
"--\n"
"\n");

#define _CTYPES_PYOBJ_FROMPTR_METHODDEF    \
    {"PyObj_FromPtr", _PyCFunction_CAST(_ctypes_PyObj_FromPtr), METH_VALUE|METH_O, _ctypes_PyObj_FromPtr__doc__},

static PyValue
_ctypes_PyObj_FromPtr_impl(PyObject *module, PyObject *ob);

static PyValue
_ctypes_PyObj_FromPtr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *ob;

    if (!_parse_voidp_object(PyValue_Borrow(args[0]), &ob)) {
        goto exit;
    }
    return_value = _ctypes_PyObj_FromPtr_impl(PyValue_AsKnownObjectBorrow(module), ob);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_Py_INCREF__doc__,
"Py_INCREF($module, obj, /)\n"
"--\n"
"\n"
"Increment the reference count of the object and return it.");

#define _CTYPES_PY_INCREF_METHODDEF    \
    {"Py_INCREF", _PyCFunction_CAST(_ctypes_Py_INCREF), METH_VALUE|METH_O, _ctypes_Py_INCREF__doc__},

static PyValue
_ctypes_Py_INCREF_impl(PyObject *module, PyValueBorrow arg);

static PyValue
_ctypes_Py_INCREF(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _ctypes_Py_INCREF_impl(PyValue_AsKnownObjectBorrow(module), arg);

    return return_value;
}

PyDoc_STRVAR(_ctypes_Py_DECREF__doc__,
"Py_DECREF($module, obj, /)\n"
"--\n"
"\n"
"Decrement the reference count of the object and return it.");

#define _CTYPES_PY_DECREF_METHODDEF    \
    {"Py_DECREF", _PyCFunction_CAST(_ctypes_Py_DECREF), METH_VALUE|METH_O, _ctypes_Py_DECREF__doc__},

static PyValue
_ctypes_Py_DECREF_impl(PyObject *module, PyValueBorrow arg);

static PyValue
_ctypes_Py_DECREF(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _ctypes_Py_DECREF_impl(PyValue_AsKnownObjectBorrow(module), arg);

    return return_value;
}

PyDoc_STRVAR(_ctypes_resize__doc__,
"resize($module, obj, size, /)\n"
"--\n"
"\n");

#define _CTYPES_RESIZE_METHODDEF    \
    {"resize", _PyCFunction_CAST(_ctypes_resize), METH_VALUE, _ctypes_resize__doc__},

static PyValue
_ctypes_resize_impl(PyObject *module, CDataObject *obj, Py_ssize_t size);

static PyValue
_ctypes_resize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    CDataObject *obj;
    Py_ssize_t size;

    if (!_PyArg_CheckPositional("resize", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->PyCData_Type)) {
        _PyArg_BadArgumentValue("resize", "argument 1", (clinic_state()->PyCData_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    obj = (CDataObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(obj));
    return_value = _ctypes_resize_impl(PyValue_AsKnownObjectBorrow(module), obj, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes__unpickle__doc__,
"_unpickle($module, cls, state, /)\n"
"--\n"
"\n");

#define _CTYPES__UNPICKLE_METHODDEF    \
    {"_unpickle", _PyCFunction_CAST(_ctypes__unpickle), METH_VALUE, _ctypes__unpickle__doc__},

static PyValue
_ctypes__unpickle_impl(PyObject *module, PyValueBorrow typ, PyObject *state);

static PyValue
_ctypes__unpickle(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow typ;
    PyObject *state;

    if (!_PyArg_CheckPositional("_unpickle", nargs, 2, 2)) {
        goto exit;
    }
    typ = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("_unpickle", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    state = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _ctypes__unpickle_impl(PyValue_AsKnownObjectBorrow(module), typ, state);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_buffer_info__doc__,
"buffer_info($module, obj, /)\n"
"--\n"
"\n"
"Return buffer interface information.");

#define _CTYPES_BUFFER_INFO_METHODDEF    \
    {"buffer_info", _PyCFunction_CAST(_ctypes_buffer_info), METH_VALUE|METH_O, _ctypes_buffer_info__doc__},

static PyValue
_ctypes_buffer_info_impl(PyObject *module, PyValueBorrow arg);

static PyValue
_ctypes_buffer_info(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _ctypes_buffer_info_impl(PyValue_AsKnownObjectBorrow(module), arg);

    return return_value;
}

#ifndef _CTYPES_GET_LAST_ERROR_METHODDEF
    #define _CTYPES_GET_LAST_ERROR_METHODDEF
#endif /* !defined(_CTYPES_GET_LAST_ERROR_METHODDEF) */

#ifndef _CTYPES_SET_LAST_ERROR_METHODDEF
    #define _CTYPES_SET_LAST_ERROR_METHODDEF
#endif /* !defined(_CTYPES_SET_LAST_ERROR_METHODDEF) */

#ifndef _CTYPES__CHECK_HRESULT_METHODDEF
    #define _CTYPES__CHECK_HRESULT_METHODDEF
#endif /* !defined(_CTYPES__CHECK_HRESULT_METHODDEF) */

#ifndef _CTYPES_FORMATERROR_METHODDEF
    #define _CTYPES_FORMATERROR_METHODDEF
#endif /* !defined(_CTYPES_FORMATERROR_METHODDEF) */

#ifndef _CTYPES_LOADLIBRARY_METHODDEF
    #define _CTYPES_LOADLIBRARY_METHODDEF
#endif /* !defined(_CTYPES_LOADLIBRARY_METHODDEF) */

#ifndef _CTYPES_FREELIBRARY_METHODDEF
    #define _CTYPES_FREELIBRARY_METHODDEF
#endif /* !defined(_CTYPES_FREELIBRARY_METHODDEF) */

#ifndef _CTYPES_COPYCOMPOINTER_METHODDEF
    #define _CTYPES_COPYCOMPOINTER_METHODDEF
#endif /* !defined(_CTYPES_COPYCOMPOINTER_METHODDEF) */

#ifndef _CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF
    #define _CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF
#endif /* !defined(_CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF) */

#ifndef _CTYPES_DLOPEN_METHODDEF
    #define _CTYPES_DLOPEN_METHODDEF
#endif /* !defined(_CTYPES_DLOPEN_METHODDEF) */

#ifndef _CTYPES_DLCLOSE_METHODDEF
    #define _CTYPES_DLCLOSE_METHODDEF
#endif /* !defined(_CTYPES_DLCLOSE_METHODDEF) */

#ifndef _CTYPES_DLSYM_METHODDEF
    #define _CTYPES_DLSYM_METHODDEF
#endif /* !defined(_CTYPES_DLSYM_METHODDEF) */

#ifndef _CTYPES_DLLIST_METHODDEF
    #define _CTYPES_DLLIST_METHODDEF
#endif /* !defined(_CTYPES_DLLIST_METHODDEF) */
/*[clinic end generated code: output=17ee0b371c8e6ffc input=a9049054013a1b77]*/
