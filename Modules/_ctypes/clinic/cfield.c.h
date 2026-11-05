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
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

static PyValue
PyCField_new_impl(PyTypeObject *type, PyObject *name, PyValueBorrow proto,
                  Py_ssize_t byte_size, Py_ssize_t byte_offset,
                  Py_ssize_t index, int _internal_use,
                  PyValueBorrow bit_size_obj, PyValueBorrow bit_offset_obj);

static PyValue
PyCField_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(type)), _PyValue_STATIC_IMMORTAL(&_Py_ID(byte_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(byte_offset)), _PyValue_STATIC_IMMORTAL(&_Py_ID(index)), _PyValue_STATIC_IMMORTAL(&_Py_ID(_internal_use)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bit_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bit_offset)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "type", "byte_size", "byte_offset", "index", "_internal_use", "bit_size", "bit_offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "CField",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[8];
    PyObject *name;
    PyValueBorrow proto;
    Py_ssize_t byte_size;
    Py_ssize_t byte_offset;
    Py_ssize_t index;
    int _internal_use;
    PyValueBorrow bit_size_obj = PyValue_Borrow(PyValue_None);
    PyValueBorrow bit_offset_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 6, /*varpos*/ 0, _idx, 8)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("CField", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    proto = PyValue_Borrow(args[_idx[1]]);
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        byte_size = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[3]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        byte_offset = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[4]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    _internal_use = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
    if (_internal_use < 0) {
        goto exit;
    }
    if (_idx[6] >= 0) {
        bit_size_obj = PyValue_Borrow(args[_idx[6]]);
    }
    if (_idx[7] >= 0) {
        bit_offset_obj = PyValue_Borrow(args[_idx[7]]);
    }
    return_value = PyCField_new_impl(type, name, proto, byte_size, byte_offset, index, _internal_use, bit_size_obj, bit_offset_obj);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
PyCField_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(PyCField_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = PyCField_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
/*[clinic end generated code: output=41126f1f4b62991d input=a9049054013a1b77]*/
