/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(termios_tcgetattr__doc__,
"tcgetattr($module, fd, /)\n"
"--\n"
"\n"
"Get the tty attributes for file descriptor fd.\n"
"\n"
"Returns a list [iflag, oflag, cflag, lflag, ispeed, ospeed, cc]\n"
"where cc is a list of the tty special characters (each a string of\n"
"length 1, except the items with indices VMIN and VTIME, which are\n"
"integers when these fields are defined).  The interpretation of the\n"
"flags and the speeds as well as the indexing in the cc array must be\n"
"done using the symbolic constants defined in this module.");

#define TERMIOS_TCGETATTR_METHODDEF    \
    {"tcgetattr", _PyCFunction_CAST(termios_tcgetattr), METH_VALUE|METH_O, termios_tcgetattr__doc__},

static PyValue
termios_tcgetattr_impl(PyObject *module, int fd);

static PyValue
termios_tcgetattr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;

    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    return_value = termios_tcgetattr_impl(PyValue_AsKnownObjectBorrow(module), fd);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcsetattr__doc__,
"tcsetattr($module, fd, when, attributes, /)\n"
"--\n"
"\n"
"Set the tty attributes for file descriptor fd.\n"
"\n"
"The attributes to be set are taken from the attributes argument, which\n"
"is a list like the one returned by tcgetattr(). The when argument\n"
"determines when the attributes are changed: termios.TCSANOW to\n"
"change immediately, termios.TCSADRAIN to change after transmitting all\n"
"queued output, or termios.TCSAFLUSH to change after transmitting all\n"
"queued output and discarding all queued input.");

#define TERMIOS_TCSETATTR_METHODDEF    \
    {"tcsetattr", _PyCFunction_CAST(termios_tcsetattr), METH_VALUE, termios_tcsetattr__doc__},

static PyValue
termios_tcsetattr_impl(PyObject *module, int fd, int when,
                       PyValueBorrow term);

static PyValue
termios_tcsetattr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    int when;
    PyValueBorrow term;

    if (!_PyArg_CheckPositional("tcsetattr", nargs, 3, 3)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    when = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (when == -1 && PyErr_Occurred()) {
        goto exit;
    }
    term = PyValue_Borrow(args[2]);
    return_value = termios_tcsetattr_impl(PyValue_AsKnownObjectBorrow(module), fd, when, term);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcsendbreak__doc__,
"tcsendbreak($module, fd, duration, /)\n"
"--\n"
"\n"
"Send a break on file descriptor fd.\n"
"\n"
"A zero duration sends a break for 0.25-0.5 seconds; a nonzero duration\n"
"has a system dependent meaning.");

#define TERMIOS_TCSENDBREAK_METHODDEF    \
    {"tcsendbreak", _PyCFunction_CAST(termios_tcsendbreak), METH_VALUE, termios_tcsendbreak__doc__},

static PyValue
termios_tcsendbreak_impl(PyObject *module, int fd, int duration);

static PyValue
termios_tcsendbreak(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    int duration;

    if (!_PyArg_CheckPositional("tcsendbreak", nargs, 2, 2)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    duration = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (duration == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = termios_tcsendbreak_impl(PyValue_AsKnownObjectBorrow(module), fd, duration);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcdrain__doc__,
"tcdrain($module, fd, /)\n"
"--\n"
"\n"
"Wait until all output written to file descriptor fd has been transmitted.");

#define TERMIOS_TCDRAIN_METHODDEF    \
    {"tcdrain", _PyCFunction_CAST(termios_tcdrain), METH_VALUE|METH_O, termios_tcdrain__doc__},

static PyValue
termios_tcdrain_impl(PyObject *module, int fd);

static PyValue
termios_tcdrain(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;

    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    return_value = termios_tcdrain_impl(PyValue_AsKnownObjectBorrow(module), fd);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcflush__doc__,
"tcflush($module, fd, queue, /)\n"
"--\n"
"\n"
"Discard queued data on file descriptor fd.\n"
"\n"
"The queue selector specifies which queue: termios.TCIFLUSH for the input\n"
"queue, termios.TCOFLUSH for the output queue, or termios.TCIOFLUSH for\n"
"both queues.");

#define TERMIOS_TCFLUSH_METHODDEF    \
    {"tcflush", _PyCFunction_CAST(termios_tcflush), METH_VALUE, termios_tcflush__doc__},

static PyValue
termios_tcflush_impl(PyObject *module, int fd, int queue);

static PyValue
termios_tcflush(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    int queue;

    if (!_PyArg_CheckPositional("tcflush", nargs, 2, 2)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    queue = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (queue == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = termios_tcflush_impl(PyValue_AsKnownObjectBorrow(module), fd, queue);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcflow__doc__,
"tcflow($module, fd, action, /)\n"
"--\n"
"\n"
"Suspend or resume input or output on file descriptor fd.\n"
"\n"
"The action argument can be termios.TCOOFF to suspend output,\n"
"termios.TCOON to restart output, termios.TCIOFF to suspend input,\n"
"or termios.TCION to restart input.");

#define TERMIOS_TCFLOW_METHODDEF    \
    {"tcflow", _PyCFunction_CAST(termios_tcflow), METH_VALUE, termios_tcflow__doc__},

static PyValue
termios_tcflow_impl(PyObject *module, int fd, int action);

static PyValue
termios_tcflow(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    int action;

    if (!_PyArg_CheckPositional("tcflow", nargs, 2, 2)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    action = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (action == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = termios_tcflow_impl(PyValue_AsKnownObjectBorrow(module), fd, action);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcgetwinsize__doc__,
"tcgetwinsize($module, fd, /)\n"
"--\n"
"\n"
"Get the tty winsize for file descriptor fd.\n"
"\n"
"Returns a tuple (ws_row, ws_col).");

#define TERMIOS_TCGETWINSIZE_METHODDEF    \
    {"tcgetwinsize", _PyCFunction_CAST(termios_tcgetwinsize), METH_VALUE|METH_O, termios_tcgetwinsize__doc__},

static PyValue
termios_tcgetwinsize_impl(PyObject *module, int fd);

static PyValue
termios_tcgetwinsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;

    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    return_value = termios_tcgetwinsize_impl(PyValue_AsKnownObjectBorrow(module), fd);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcsetwinsize__doc__,
"tcsetwinsize($module, fd, winsize, /)\n"
"--\n"
"\n"
"Set the tty winsize for file descriptor fd.\n"
"\n"
"The winsize to be set is taken from the winsize argument, which\n"
"is a two-item tuple (ws_row, ws_col) like the one returned by\n"
"tcgetwinsize().");

#define TERMIOS_TCSETWINSIZE_METHODDEF    \
    {"tcsetwinsize", _PyCFunction_CAST(termios_tcsetwinsize), METH_VALUE, termios_tcsetwinsize__doc__},

static PyValue
termios_tcsetwinsize_impl(PyObject *module, int fd, PyValueBorrow winsz);

static PyValue
termios_tcsetwinsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    PyValueBorrow winsz;

    if (!_PyArg_CheckPositional("tcsetwinsize", nargs, 2, 2)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[0]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (fd < 0) {
            PyErr_Format(PyExc_ValueError,
                         "file descriptor cannot be a negative integer (%i)",
                         fd);
            goto exit;
        }
    }
    else {
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[0]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    winsz = PyValue_Borrow(args[1]);
    return_value = termios_tcsetwinsize_impl(PyValue_AsKnownObjectBorrow(module), fd, winsz);

exit:
    return return_value;
}
/*[clinic end generated code: output=e8815d247f509f85 input=a9049054013a1b77]*/
