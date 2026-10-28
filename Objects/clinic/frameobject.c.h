/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

PyDoc_STRVAR(frame_locals__doc__,
"Return the mapping used by the frame to look up local variables.");

static PyObject *
frame_locals_get_impl(PyFrameObject *self);

static PyValue
frame_locals_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_locals_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_lineno__doc__,
"Return the current line number in the frame.");

static PyObject *
frame_lineno_get_impl(PyFrameObject *self);

static PyValue
frame_lineno_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_lineno_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_lasti__doc__,
"Return the index of the last attempted instruction in the frame.");

static PyObject *
frame_lasti_get_impl(PyFrameObject *self);

static PyValue
frame_lasti_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_lasti_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_globals__doc__,
"Return the global variables in the frame.");

static PyObject *
frame_globals_get_impl(PyFrameObject *self);

static PyValue
frame_globals_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_globals_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_builtins__doc__,
"Return the built-in variables in the frame.");

static PyObject *
frame_builtins_get_impl(PyFrameObject *self);

static PyValue
frame_builtins_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_builtins_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_code__doc__,
"Return the code object being executed in this frame.");

static PyObject *
frame_code_get_impl(PyFrameObject *self);

static PyValue
frame_code_get(PyObject *self, void *Py_UNUSED(context))
{
    return PyValue_FromResult(frame_code_get_impl((PyFrameObject *)self));
}

static PyObject *
frame_back_get_impl(PyFrameObject *self);

static PyValue
frame_back_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_back_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_trace_opcodes__doc__,
"Return True if opcode tracing is enabled, False otherwise.");

static PyObject *
frame_trace_opcodes_get_impl(PyFrameObject *self);

static PyValue
frame_trace_opcodes_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_trace_opcodes_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static int
frame_trace_opcodes_set_impl(PyFrameObject *self, PyObject *value);

static int
frame_trace_opcodes_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (PyValue_IsNull(arg)) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'f_trace_opcodes' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = PyValue_Object(arg);
    if (value == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = frame_trace_opcodes_set_impl((PyFrameObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
frame_lineno_set_impl(PyFrameObject *self, PyObject *value);

static int
frame_lineno_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (PyValue_IsNull(arg)) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'f_lineno' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = PyValue_Object(arg);
    if (value == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = frame_lineno_set_impl((PyFrameObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(frame_trace__doc__,
"Return the trace function for this frame, or None if no trace function is set.");

static PyObject *
frame_trace_get_impl(PyFrameObject *self);

static PyValue
frame_trace_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_trace_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

static int
frame_trace_set_impl(PyFrameObject *self, PyObject *value);

static int
frame_trace_set(PyObject *self, PyValue arg, void *Py_UNUSED(context))
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
    return_value = frame_trace_set_impl((PyFrameObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(frame_generator__doc__,
"Return the generator or coroutine associated with this frame, or None.");

static PyObject *
frame_generator_get_impl(PyFrameObject *self);

static PyValue
frame_generator_get(PyObject *self, void *Py_UNUSED(context))
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = frame_generator_get_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(frame_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Clear all references held by the frame.");

#define FRAME_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)frame_clear, METH_NOARGS, frame_clear__doc__},

static PyObject *
frame_clear_impl(PyFrameObject *self);

static PyObject *
frame_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = frame_clear_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(frame___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the frame in memory, in bytes.");

#define FRAME___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)frame___sizeof__, METH_NOARGS, frame___sizeof____doc__},

static PyObject *
frame___sizeof___impl(PyFrameObject *self);

static PyObject *
frame___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = frame___sizeof___impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
frame_repr_impl(PyFrameObject *self);

static PyObject *
frame_repr(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = frame_repr_impl((PyFrameObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
#define FRAME_F_LOCALS_GETSETDEF {"f_locals", frame_locals_get, NULL, frame_locals__doc__},

#define FRAME_F_LINENO_GETSETDEF {"f_lineno", frame_lineno_get, frame_lineno_set, frame_lineno__doc__},

#define FRAME_F_LASTI_GETSETDEF {"f_lasti", frame_lasti_get, NULL, frame_lasti__doc__},

#define FRAME_F_GLOBALS_GETSETDEF {"f_globals", frame_globals_get, NULL, frame_globals__doc__},

#define FRAME_F_BUILTINS_GETSETDEF {"f_builtins", frame_builtins_get, NULL, frame_builtins__doc__},

#define FRAME_F_CODE_GETSETDEF {"f_code", frame_code_get, NULL, frame_code__doc__},

#define FRAME_F_BACK_GETSETDEF {"f_back", frame_back_get, NULL, NULL},

#define FRAME_F_TRACE_OPCODES_GETSETDEF {"f_trace_opcodes", frame_trace_opcodes_get, frame_trace_opcodes_set, frame_trace_opcodes__doc__},

#define FRAME_F_TRACE_GETSETDEF {"f_trace", frame_trace_get, frame_trace_set, frame_trace__doc__},

#define FRAME_F_GENERATOR_GETSETDEF {"f_generator", frame_generator_get, NULL, frame_generator__doc__},

/*[clinic end generated code: output=b0e06acc3eb1c7dc input=a9049054013a1b77]*/
