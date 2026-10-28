/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()

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
    {"__reduce__", (PyCFunction)BaseException___reduce__, METH_NOARGS, BaseException___reduce____doc__},

static PyObject *
BaseException___reduce___impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___reduce___impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(BaseException___setstate__), METH_O, BaseException___setstate____doc__},

static PyObject *
BaseException___setstate___impl(PyBaseExceptionObject *self, PyObject *state);

static PyValue
BaseException___setstate__(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *state;
    PyObject *_return_value;

    state = PyValue_Object(arg);
    if (state == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(state);
    _return_value = BaseException___setstate___impl((PyBaseExceptionObject *)self, state);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(BaseException_with_traceback__doc__,
"with_traceback($self, tb, /)\n"
"--\n"
"\n"
"Set self.__traceback__ to tb and return self.");

#define BASEEXCEPTION_WITH_TRACEBACK_METHODDEF    \
    {"with_traceback", _PyCFunction_CAST(BaseException_with_traceback), METH_O, BaseException_with_traceback__doc__},

static PyObject *
BaseException_with_traceback_impl(PyBaseExceptionObject *self, PyObject *tb);

static PyValue
BaseException_with_traceback(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *tb;
    PyObject *_return_value;

    tb = PyValue_Object(arg);
    if (tb == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseException_with_traceback_impl((PyBaseExceptionObject *)self, tb);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(BaseException_add_note__doc__,
"add_note($self, note, /)\n"
"--\n"
"\n"
"Add a note to the exception");

#define BASEEXCEPTION_ADD_NOTE_METHODDEF    \
    {"add_note", _PyCFunction_CAST(BaseException_add_note), METH_O, BaseException_add_note__doc__},

static PyObject *
BaseException_add_note_impl(PyBaseExceptionObject *self, PyObject *note);

static PyValue
BaseException_add_note(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *note;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("add_note", "argument", "str", _arg);
            goto exit;
        }
        note = _arg;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseException_add_note_impl((PyBaseExceptionObject *)self, note);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

static PyObject *
BaseException_args_get_impl(PyBaseExceptionObject *self);

static PyValue
BaseException_args_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseException_args_get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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

static PyValue
BaseException___traceback___get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseException___traceback___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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

static PyValue
BaseException___context___get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseException___context___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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

static PyValue
BaseException___cause___get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseException___cause___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"derive", _PyCFunction_CAST(BaseExceptionGroup_derive), METH_O, BaseExceptionGroup_derive__doc__},

static PyObject *
BaseExceptionGroup_derive_impl(PyBaseExceptionGroupObject *self,
                               PyObject *excs);

static PyValue
BaseExceptionGroup_derive(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *excs;
    PyObject *_return_value;

    excs = PyValue_Object(arg);
    if (excs == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseExceptionGroup_derive_impl((PyBaseExceptionGroupObject *)self, excs);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_split__doc__,
"split($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(BaseExceptionGroup_split), METH_O, BaseExceptionGroup_split__doc__},

static PyObject *
BaseExceptionGroup_split_impl(PyBaseExceptionGroupObject *self,
                              PyObject *matcher_value);

static PyValue
BaseExceptionGroup_split(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *matcher_value;
    PyObject *_return_value;

    matcher_value = PyValue_Object(arg);
    if (matcher_value == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseExceptionGroup_split_impl((PyBaseExceptionGroupObject *)self, matcher_value);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_subgroup__doc__,
"subgroup($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SUBGROUP_METHODDEF    \
    {"subgroup", _PyCFunction_CAST(BaseExceptionGroup_subgroup), METH_O, BaseExceptionGroup_subgroup__doc__},

static PyObject *
BaseExceptionGroup_subgroup_impl(PyBaseExceptionGroupObject *self,
                                 PyObject *matcher_value);

static PyValue
BaseExceptionGroup_subgroup(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *matcher_value;
    PyObject *_return_value;

    matcher_value = PyValue_Object(arg);
    if (matcher_value == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = BaseExceptionGroup_subgroup_impl((PyBaseExceptionGroupObject *)self, matcher_value);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
#define BASEEXCEPTION_ARGS_GETSETDEF {"args", BaseException_args_get, (setter)BaseException_args_set, NULL},

#define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", BaseException___traceback___get, (setter)BaseException___traceback___set, NULL},

#define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", BaseException___context___get, (setter)BaseException___context___set, NULL},

#define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", BaseException___cause___get, (setter)BaseException___cause___set, NULL},

/*[clinic end generated code: output=4bcebc0a500c3b29 input=a9049054013a1b77]*/
