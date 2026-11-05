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
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(tb_new__doc__,
"traceback(tb_next, tb_frame, tb_lasti, tb_lineno)\n"
"--\n"
"\n"
"Create a new traceback object.");

static PyValue
tb_new_impl(PyTypeObject *type, PyValueBorrow tb_next,
            PyFrameObject *tb_frame, int tb_lasti, int tb_lineno);

static PyValue
tb_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(tb_next)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tb_frame)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tb_lasti)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tb_lineno)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tb_next", "tb_frame", "tb_lasti", "tb_lineno", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "traceback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow tb_next;
    PyFrameObject *tb_frame;
    int tb_lasti;
    int tb_lineno;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 4, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    tb_next = PyValue_Borrow(args[_idx[0]]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), &PyFrame_Type)) {
        _PyArg_BadArgumentValue("traceback", "argument 'tb_frame'", (&PyFrame_Type)->tp_name, PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    tb_frame = (PyFrameObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    tb_lasti = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (tb_lasti == -1 && PyErr_Occurred()) {
        goto exit;
    }
    tb_lineno = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
    if (tb_lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = tb_new_impl(type, tb_next, tb_frame, tb_lasti, tb_lineno);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
tb_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(tb_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = tb_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyObject *
traceback_tb_next_get_impl(PyTracebackObject *self);

static PyObject *
traceback_tb_next_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = traceback_tb_next_get_impl((PyTracebackObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
traceback_tb_next_set_impl(PyTracebackObject *self, PyObject *value);

static int
traceback_tb_next_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'tb_next' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = traceback_tb_next_set_impl((PyTracebackObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
#define TRACEBACK_TB_NEXT_GETSETDEF {"tb_next", (getter)traceback_tb_next_get, (setter)traceback_tb_next_set, NULL},

/*[clinic end generated code: output=162910b88cc2a88f input=a9049054013a1b77]*/
