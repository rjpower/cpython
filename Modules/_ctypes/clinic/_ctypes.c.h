/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(_ctypes_CType_Type___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return memory consumption of the type object.");

#define _CTYPES_CTYPE_TYPE___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_ctypes_CType_Type___sizeof__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _ctypes_CType_Type___sizeof____doc__},

static PyObject *
_ctypes_CType_Type___sizeof___impl(PyObject *self, PyTypeObject *cls);

static PyValue
_ctypes_CType_Type___sizeof__(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__sizeof__() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_ctypes_CType_Type___sizeof___impl(self, cls));
}

static PyObject *
_ctypes_CType_Type___pointer_type___get_impl(PyObject *self);

static PyValue
_ctypes_CType_Type___pointer_type___get(PyObject *self, void *Py_UNUSED(context))
{
    return PyValue_FromResult(_ctypes_CType_Type___pointer_type___get_impl(self));
}

static int
_ctypes_CType_Type___pointer_type___set_impl(PyObject *self, PyObject *value);

static int
_ctypes_CType_Type___pointer_type___set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value = NULL;

    if (!PyValue_IsNull(arg)) {
        value = PyValue_Object(arg);
        if (value == NULL) {
            goto exit;
        }
    }
    return_value = _ctypes_CType_Type___pointer_type___set_impl(self, value);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_address__doc__,
"from_address($self, value, /)\n"
"--\n"
"\n"
"C.from_address(integer) -> C instance\n"
"\n"
"Access a C instance at the specified address.");

#define CDATATYPE_FROM_ADDRESS_METHODDEF    \
    {"from_address", _PyCFunction_CAST(CDataType_from_address), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_address__doc__},

static PyObject *
CDataType_from_address_impl(PyObject *type, PyTypeObject *cls,
                            PyObject *value);

static PyValue
CDataType_from_address(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_address", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_address", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = CDataType_from_address_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_buffer__doc__,
"from_buffer($self, obj, offset=0, /)\n"
"--\n"
"\n"
"C.from_buffer(object, offset=0) -> C instance\n"
"\n"
"Create a C instance from a writeable buffer.");

#define CDATATYPE_FROM_BUFFER_METHODDEF    \
    {"from_buffer", _PyCFunction_CAST(CDataType_from_buffer), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_buffer__doc__},

static PyObject *
CDataType_from_buffer_impl(PyObject *type, PyTypeObject *cls, PyObject *obj,
                           Py_ssize_t offset);

static PyValue
CDataType_from_buffer(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    Py_ssize_t offset = 0;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_buffer", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_buffer", nargs, 1, 2)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
            if (offset < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "offset cannot be negative");
                goto exit;
            }
    }
skip_optional:
    _return_value = CDataType_from_buffer_impl(type, cls, obj, offset);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_buffer_copy__doc__,
"from_buffer_copy($self, buffer, offset=0, /)\n"
"--\n"
"\n"
"C.from_buffer_copy(object, offset=0) -> C instance\n"
"\n"
"Create a C instance from a readable buffer.");

#define CDATATYPE_FROM_BUFFER_COPY_METHODDEF    \
    {"from_buffer_copy", _PyCFunction_CAST(CDataType_from_buffer_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_buffer_copy__doc__},

static PyObject *
CDataType_from_buffer_copy_impl(PyObject *type, PyTypeObject *cls,
                                Py_buffer *buffer, Py_ssize_t offset);

static PyValue
CDataType_from_buffer_copy(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset = 0;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_buffer_copy", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_buffer_copy", nargs, 1, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &buffer, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
            if (offset < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "offset cannot be negative");
                goto exit;
            }
    }
skip_optional:
    _return_value = CDataType_from_buffer_copy_impl(type, cls, &buffer, offset);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(CDataType_in_dll__doc__,
"in_dll($self, dll, name, /)\n"
"--\n"
"\n"
"C.in_dll(dll, name) -> C instance\n"
"\n"
"Access a C instance in a dll.");

#define CDATATYPE_IN_DLL_METHODDEF    \
    {"in_dll", _PyCFunction_CAST(CDataType_in_dll), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_in_dll__doc__},

static PyObject *
CDataType_in_dll_impl(PyObject *type, PyTypeObject *cls, PyObject *dll,
                      const char *name);

static PyValue
CDataType_in_dll(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dll;
    const char *name;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("in_dll", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("in_dll", nargs, 2, 2)) {
        goto exit;
    }
    dll = PyValue_Object(args[0]);
    if (dll == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("in_dll", "argument 2", "str", _arg);
            goto exit;
        }
        name = _PyUnicode_AsUTF8NoNUL(_arg);
        if (name == NULL) {
            goto exit;
        }
    }
    _return_value = CDataType_in_dll_impl(type, cls, dll, name);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n"
"Convert a Python object into a function call parameter.");

#define CDATATYPE_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(CDataType_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_param__doc__},

static PyObject *
CDataType_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyValue
CDataType_from_param(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_param", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_param", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = CDataType_from_param_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCPointerType_set_type__doc__,
"set_type($self, type, /)\n"
"--\n"
"\n");

#define PYCPOINTERTYPE_SET_TYPE_METHODDEF    \
    {"set_type", _PyCFunction_CAST(PyCPointerType_set_type), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCPointerType_set_type__doc__},

static PyObject *
PyCPointerType_set_type_impl(PyTypeObject *self, PyTypeObject *cls,
                             PyObject *type);

static PyValue
PyCPointerType_set_type(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *type;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("set_type", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set_type", nargs, 1, 1)) {
        goto exit;
    }
    type = PyValue_Object(args[0]);
    if (type == NULL) {
        goto exit;
    }
    _return_value = PyCPointerType_set_type_impl((PyTypeObject *)self, cls, type);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCPointerType_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n"
"Convert a Python object into a function call parameter.");

#define PYCPOINTERTYPE_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(PyCPointerType_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCPointerType_from_param__doc__},

static PyObject *
PyCPointerType_from_param_impl(PyObject *type, PyTypeObject *cls,
                               PyObject *value);

static PyValue
PyCPointerType_from_param(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_param", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_param", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = PyCPointerType_from_param_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

static int
_ctypes_PyCArrayType_Type_raw_set_impl(CDataObject *self, Py_buffer *value);

static int
_ctypes_PyCArrayType_Type_raw_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    Py_buffer value = {NULL, NULL};

    if (PyValue_IsNull(arg)) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'raw' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &value, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ctypes_PyCArrayType_Type_raw_set_impl((CDataObject *)self, &value);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for value */
    if (value.obj) {
       PyBuffer_Release(&value);
    }

    return return_value;
}

static PyObject *
_ctypes_PyCArrayType_Type_raw_get_impl(CDataObject *self);

static PyValue
_ctypes_PyCArrayType_Type_raw_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ctypes_PyCArrayType_Type_raw_get_impl((CDataObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static PyObject *
_ctypes_PyCArrayType_Type_value_get_impl(CDataObject *self);

static PyValue
_ctypes_PyCArrayType_Type_value_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ctypes_PyCArrayType_Type_value_get_impl((CDataObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static int
_ctypes_PyCArrayType_Type_value_set_impl(CDataObject *self,
                                         PyBytesObject *value);

static int
_ctypes_PyCArrayType_Type_value_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyBytesObject *value;

    if (PyValue_IsNull(arg)) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'value' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            PyErr_Format(PyExc_TypeError, "attribute 'value' must be bytes, not %T", _arg);
            goto exit;
        }
        value = (PyBytesObject *)_arg;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ctypes_PyCArrayType_Type_value_set_impl((CDataObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(c_wchar_p_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n");

#define C_WCHAR_P_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(c_wchar_p_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, c_wchar_p_from_param__doc__},

static PyObject *
c_wchar_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyValue
c_wchar_p_from_param(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_param", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_param", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = c_wchar_p_from_param_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(c_char_p_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n");

#define C_CHAR_P_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(c_char_p_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, c_char_p_from_param__doc__},

static PyObject *
c_char_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyValue
c_char_p_from_param(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_param", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_param", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = c_char_p_from_param_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(c_void_p_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n");

#define C_VOID_P_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(c_void_p_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, c_void_p_from_param__doc__},

static PyObject *
c_void_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyValue
c_void_p_from_param(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_param", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_param", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = c_void_p_from_param_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCSimpleType_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n"
"Convert a Python object into a function call parameter.");

#define PYCSIMPLETYPE_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(PyCSimpleType_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCSimpleType_from_param__doc__},

static PyObject *
PyCSimpleType_from_param_impl(PyObject *type, PyTypeObject *cls,
                              PyObject *value);

static PyValue
PyCSimpleType_from_param(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("from_param", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("from_param", nargs, 1, 1)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = PyCSimpleType_from_param_impl(type, cls, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_PyCData___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define _CTYPES_PYCDATA___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(_ctypes_PyCData___reduce__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _ctypes_PyCData___reduce____doc__},

static PyObject *
_ctypes_PyCData___reduce___impl(PyObject *myself, PyTypeObject *cls);

static PyValue
_ctypes_PyCData___reduce__(PyObject *myself, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__reduce__() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(myself);
    _return_value = _ctypes_PyCData___reduce___impl(myself, cls);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_PyCData___setstate____doc__,
"__setstate__($self, dict, data, /)\n"
"--\n"
"\n");

#define _CTYPES_PYCDATA___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_ctypes_PyCData___setstate__), METH_FASTCALL, _ctypes_PyCData___setstate____doc__},

static PyObject *
_ctypes_PyCData___setstate___impl(PyObject *myself, PyObject *dict,
                                  const char *data, Py_ssize_t data_length);

static PyValue
_ctypes_PyCData___setstate__(PyObject *myself, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dict;
    const char *data;
    Py_ssize_t data_length;
    PyObject *_return_value;

    if (!PyArg_ParseArray(args, nargs, "O!s#:__setstate__",
        &PyDict_Type, &dict, &data, &data_length)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(myself);
    _return_value = _ctypes_PyCData___setstate___impl(myself, dict, data, data_length);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_PyCData___ctypes_from_outparam____doc__,
"__ctypes_from_outparam__($self, /)\n"
"--\n"
"\n"
"default __ctypes_from_outparam__ method returns self.");

#define _CTYPES_PYCDATA___CTYPES_FROM_OUTPARAM___METHODDEF    \
    {"__ctypes_from_outparam__", (PyCFunction)_ctypes_PyCData___ctypes_from_outparam__, METH_NOARGS, _ctypes_PyCData___ctypes_from_outparam____doc__},

static PyObject *
_ctypes_PyCData___ctypes_from_outparam___impl(PyObject *self);

static PyObject *
_ctypes_PyCData___ctypes_from_outparam__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _ctypes_PyCData___ctypes_from_outparam___impl(self);
}

static int
_ctypes_CFuncPtr_errcheck_set_impl(PyCFuncPtrObject *self, PyObject *value);

static int
_ctypes_CFuncPtr_errcheck_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value = NULL;

    if (!PyValue_IsNull(arg)) {
        value = PyValue_Object(arg);
        if (value == NULL) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ctypes_CFuncPtr_errcheck_set_impl((PyCFuncPtrObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_CFuncPtr_errcheck__doc__,
"a function to check for errors");

static PyObject *
_ctypes_CFuncPtr_errcheck_get_impl(PyCFuncPtrObject *self);

static PyValue
_ctypes_CFuncPtr_errcheck_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ctypes_CFuncPtr_errcheck_get_impl((PyCFuncPtrObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static int
_ctypes_CFuncPtr_restype_set_impl(PyCFuncPtrObject *self, PyObject *value);

static int
_ctypes_CFuncPtr_restype_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value = NULL;

    if (!PyValue_IsNull(arg)) {
        value = PyValue_Object(arg);
        if (value == NULL) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ctypes_CFuncPtr_restype_set_impl((PyCFuncPtrObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_CFuncPtr_restype__doc__,
"specify the result type");

static PyObject *
_ctypes_CFuncPtr_restype_get_impl(PyCFuncPtrObject *self);

static PyValue
_ctypes_CFuncPtr_restype_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ctypes_CFuncPtr_restype_get_impl((PyCFuncPtrObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static int
_ctypes_CFuncPtr_argtypes_set_impl(PyCFuncPtrObject *self, PyObject *value);

static int
_ctypes_CFuncPtr_argtypes_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value = NULL;

    if (!PyValue_IsNull(arg)) {
        value = PyValue_Object(arg);
        if (value == NULL) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ctypes_CFuncPtr_argtypes_set_impl((PyCFuncPtrObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_CFuncPtr_argtypes__doc__,
"specify the argument types");

static PyObject *
_ctypes_CFuncPtr_argtypes_get_impl(PyCFuncPtrObject *self);

static PyValue
_ctypes_CFuncPtr_argtypes_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ctypes_CFuncPtr_argtypes_get_impl((PyCFuncPtrObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static PyObject *
Array_item_impl(PyObject *myself, Py_ssize_t index);

static PyObject *
Array_item(PyObject *myself, Py_ssize_t index)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(myself);
    return_value = Array_item_impl(myself, index);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
Array_subscript_impl(PyObject *myself, PyObject *item);

static PyObject *
Array_subscript(PyObject *myself, PyObject *item)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(myself);
    return_value = Array_subscript_impl(myself, item);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
Array_ass_item_impl(PyObject *myself, Py_ssize_t index, PyObject *value);

static int
Array_ass_item(PyObject *myself, Py_ssize_t index, PyObject *value)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(myself);
    return_value = Array_ass_item_impl(myself, index, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
Array_ass_subscript_impl(PyObject *myself, PyObject *item, PyObject *value);

static int
Array_ass_subscript(PyObject *myself, PyObject *item, PyObject *value)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(myself);
    return_value = Array_ass_subscript_impl(myself, item, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ctypes_Simple_value_set_impl(CDataObject *self, PyObject *value);

static int
_ctypes_Simple_value_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (PyValue_IsNull(arg)) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'value' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = PyValue_Object(arg);
    if (value == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ctypes_Simple_value_set_impl((CDataObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
_ctypes_Simple_value_get_impl(CDataObject *self);

static PyValue
_ctypes_Simple_value_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ctypes_Simple_value_get_impl((CDataObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(Simple_from_outparm__doc__,
"__ctypes_from_outparam__($self, /)\n"
"--\n"
"\n");

#define SIMPLE_FROM_OUTPARM_METHODDEF    \
    {"__ctypes_from_outparam__", _PyCFunction_CAST(Simple_from_outparm), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, Simple_from_outparm__doc__},

static PyObject *
Simple_from_outparm_impl(PyObject *self, PyTypeObject *cls);

static PyValue
Simple_from_outparm(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__ctypes_from_outparam__() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(Simple_from_outparm_impl(self, cls));
}

static int
Simple_bool_impl(CDataObject *self);

static int
Simple_bool(PyObject *self)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = Simple_bool_impl((CDataObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
Pointer_item_impl(PyObject *myself, Py_ssize_t index);

static PyObject *
Pointer_item(PyObject *myself, Py_ssize_t index)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(myself);
    return_value = Pointer_item_impl(myself, index);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
Pointer_ass_item_impl(PyObject *myself, Py_ssize_t index, PyObject *value);

static int
Pointer_ass_item(PyObject *myself, Py_ssize_t index, PyObject *value)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(myself);
    return_value = Pointer_ass_item_impl(myself, index, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
Pointer_bool_impl(PyObject *self);

static int
Pointer_bool(PyObject *self)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = Pointer_bool_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
#define _CTYPES_CTYPE_TYPE___POINTER_TYPE___GETSETDEF {"__pointer_type__", _ctypes_CType_Type___pointer_type___get, _ctypes_CType_Type___pointer_type___set, NULL},

#define _CTYPES_PYCARRAYTYPE_TYPE_RAW_GETSETDEF {"raw", _ctypes_PyCArrayType_Type_raw_get, _ctypes_PyCArrayType_Type_raw_set, NULL},

#define _CTYPES_PYCARRAYTYPE_TYPE_VALUE_GETSETDEF {"value", _ctypes_PyCArrayType_Type_value_get, _ctypes_PyCArrayType_Type_value_set, NULL},

#define _CTYPES_CFUNCPTR_ERRCHECK_GETSETDEF {"errcheck", _ctypes_CFuncPtr_errcheck_get, _ctypes_CFuncPtr_errcheck_set, _ctypes_CFuncPtr_errcheck__doc__},

#define _CTYPES_CFUNCPTR_RESTYPE_GETSETDEF {"restype", _ctypes_CFuncPtr_restype_get, _ctypes_CFuncPtr_restype_set, _ctypes_CFuncPtr_restype__doc__},

#define _CTYPES_CFUNCPTR_ARGTYPES_GETSETDEF {"argtypes", _ctypes_CFuncPtr_argtypes_get, _ctypes_CFuncPtr_argtypes_set, _ctypes_CFuncPtr_argtypes__doc__},

#define _CTYPES_SIMPLE_VALUE_GETSETDEF {"value", _ctypes_Simple_value_get, _ctypes_Simple_value_set, NULL},

/*[clinic end generated code: output=751e2c96cc624858 input=a9049054013a1b77]*/
