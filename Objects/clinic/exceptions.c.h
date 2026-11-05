/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_value_call.h"    // METH_VALUE

static PyObject *
BaseException_str_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_str(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_str_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException_repr_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_repr(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_repr_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(BaseException___reduce__), METH_VALUE, BaseException___reduce____doc__},

static PyValue
BaseException___reduce___impl(PyBaseExceptionObject *self);

static PyValue
BaseException___reduce__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("BaseException.__reduce__()", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = BaseException___reduce___impl((PyBaseExceptionObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(BaseException___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(BaseException___setstate__), METH_VALUE, BaseException___setstate____doc__},

static PyValue
BaseException___setstate___impl(PyBaseExceptionObject *self,
                                PyValueBorrow state);

static PyValue
BaseException___setstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    if (!_PyArg_CheckArityValue("BaseException.__setstate__()", nargs, kwnames, 1)) {
        goto exit;
    }
    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(state));
    return_value = BaseException___setstate___impl((PyBaseExceptionObject *)PyValue_AsKnownObjectBorrow(self), state);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(BaseException_with_traceback__doc__,
"with_traceback($self, tb, /)\n"
"--\n"
"\n"
"Set self.__traceback__ to tb and return self.");

#define BASEEXCEPTION_WITH_TRACEBACK_METHODDEF    \
    {"with_traceback", _PyCFunction_CAST(BaseException_with_traceback), METH_VALUE, BaseException_with_traceback__doc__},

static PyValue
BaseException_with_traceback_impl(PyBaseExceptionObject *self,
                                  PyValueBorrow tb);

static PyValue
BaseException_with_traceback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow tb;

    if (!_PyArg_CheckArityValue("BaseException.with_traceback()", nargs, kwnames, 1)) {
        goto exit;
    }
    tb = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = BaseException_with_traceback_impl((PyBaseExceptionObject *)PyValue_AsKnownObjectBorrow(self), tb);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(BaseException_add_note__doc__,
"add_note($self, note, /)\n"
"--\n"
"\n"
"Add a note to the exception");

#define BASEEXCEPTION_ADD_NOTE_METHODDEF    \
    {"add_note", _PyCFunction_CAST(BaseException_add_note), METH_VALUE, BaseException_add_note__doc__},

static PyValue
BaseException_add_note_impl(PyBaseExceptionObject *self, PyObject *note);

static PyValue
BaseException_add_note(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *note;

    if (!_PyArg_CheckArityValue("BaseException.add_note()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("add_note", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    note = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = BaseException_add_note_impl((PyBaseExceptionObject *)PyValue_AsKnownObjectBorrow(self), note);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
BaseException_args_get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_args_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_args_get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException_args_set_impl(PyBaseExceptionObject *self, PyObject *value);

static int
BaseException_args_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'args' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_args_set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException___traceback___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___traceback___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException___traceback___set_impl(PyBaseExceptionObject *self,
                                     PyObject *value);

static int
BaseException___traceback___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__traceback__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException___context___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___context___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___context___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException___context___set_impl(PyBaseExceptionObject *self,
                                   PyObject *value);

static int
BaseException___context___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__context__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___context___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
BaseException___cause___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___cause___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___cause___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
BaseException___cause___set_impl(PyBaseExceptionObject *self,
                                 PyObject *value);

static int
BaseException___cause___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__cause__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___cause___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_derive__doc__,
"derive($self, excs, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_DERIVE_METHODDEF    \
    {"derive", _PyCFunction_CAST(BaseExceptionGroup_derive), METH_VALUE, BaseExceptionGroup_derive__doc__},

static PyValue
BaseExceptionGroup_derive_impl(PyBaseExceptionGroupObject *self,
                               PyValueBorrow excs);

static PyValue
BaseExceptionGroup_derive(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow excs;

    if (!_PyArg_CheckArityValue("BaseExceptionGroup.derive()", nargs, kwnames, 1)) {
        goto exit;
    }
    excs = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = BaseExceptionGroup_derive_impl((PyBaseExceptionGroupObject *)PyValue_AsKnownObjectBorrow(self), excs);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_split__doc__,
"split($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(BaseExceptionGroup_split), METH_VALUE, BaseExceptionGroup_split__doc__},

static PyValue
BaseExceptionGroup_split_impl(PyBaseExceptionGroupObject *self,
                              PyValueBorrow matcher_value);

static PyValue
BaseExceptionGroup_split(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow matcher_value;

    if (!_PyArg_CheckArityValue("BaseExceptionGroup.split()", nargs, kwnames, 1)) {
        goto exit;
    }
    matcher_value = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = BaseExceptionGroup_split_impl((PyBaseExceptionGroupObject *)PyValue_AsKnownObjectBorrow(self), matcher_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_subgroup__doc__,
"subgroup($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SUBGROUP_METHODDEF    \
    {"subgroup", _PyCFunction_CAST(BaseExceptionGroup_subgroup), METH_VALUE, BaseExceptionGroup_subgroup__doc__},

static PyValue
BaseExceptionGroup_subgroup_impl(PyBaseExceptionGroupObject *self,
                                 PyValueBorrow matcher_value);

static PyValue
BaseExceptionGroup_subgroup(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow matcher_value;

    if (!_PyArg_CheckArityValue("BaseExceptionGroup.subgroup()", nargs, kwnames, 1)) {
        goto exit;
    }
    matcher_value = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = BaseExceptionGroup_subgroup_impl((PyBaseExceptionGroupObject *)PyValue_AsKnownObjectBorrow(self), matcher_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
#define BASEEXCEPTION_ARGS_GETSETDEF {"args", (getter)BaseException_args_get, (setter)BaseException_args_set, NULL},

#define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, (setter)BaseException___traceback___set, NULL},

#define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", (getter)BaseException___context___get, (setter)BaseException___context___set, NULL},

#define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", (getter)BaseException___cause___get, (setter)BaseException___cause___set, NULL},

/*[clinic end generated code: output=602fca8df101d5bf input=a9049054013a1b77]*/
