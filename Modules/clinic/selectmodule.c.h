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
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(select_select__doc__,
"select($module, rlist, wlist, xlist, timeout=None, /)\n"
"--\n"
"\n"
"Wait until one or more file descriptors are ready for some kind of I/O.\n"
"\n"
"The first three arguments are iterables of file descriptors to be waited\n"
"for:\n"
"rlist -- wait until ready for reading\n"
"wlist -- wait until ready for writing\n"
"xlist -- wait for an \"exceptional condition\"\n"
"If only one kind of condition is required, pass [] for the other lists.\n"
"\n"
"A file descriptor is either a socket or file object, or a small integer\n"
"gotten from a fileno() method call on one of those.\n"
"\n"
"The optional 4th argument specifies a timeout in seconds; it may be\n"
"a non-integer to specify fractions of seconds.  If it is absent\n"
"or None, the call will never time out.\n"
"\n"
"The return value is a tuple of three lists corresponding to the first\n"
"three arguments; each contains the subset of the corresponding file\n"
"descriptors that are ready.\n"
"\n"
"*** IMPORTANT NOTICE ***\n"
"On Windows, only sockets are supported; on Unix, all file\n"
"descriptors can be used.");

#define SELECT_SELECT_METHODDEF    \
    {"select", _PyCFunction_CAST(select_select), METH_VALUE, select_select__doc__},

static PyValue
select_select_impl(PyObject *module, PyValueBorrow rlist,
                   PyValueBorrow wlist, PyValueBorrow xlist,
                   PyValueBorrow timeout_obj);

static PyValue
select_select(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow rlist;
    PyValueBorrow wlist;
    PyValueBorrow xlist;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("select", nargs, 3, 4)) {
        goto exit;
    }
    rlist = PyValue_Borrow(args[0]);
    wlist = PyValue_Borrow(args[1]);
    xlist = PyValue_Borrow(args[2]);
    if (nargs < 4) {
        goto skip_optional;
    }
    timeout_obj = PyValue_Borrow(args[3]);
skip_optional:
    return_value = select_select_impl(PyValue_AsKnownObjectBorrow(module), rlist, wlist, xlist, timeout_obj);

exit:
    return return_value;
}

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_register__doc__,
"register($self, fd,\n"
"         eventmask=select.POLLIN | select.POLLPRI | select.POLLOUT, /)\n"
"--\n"
"\n"
"Register a file descriptor with the polling object.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_POLL_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(select_poll_register), METH_VALUE, select_poll_register__doc__},

static PyValue
select_poll_register_impl(pollObject *self, int fd, unsigned short eventmask);

static PyValue
select_poll_register(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!_PyArg_CheckPositional("register", nargs, 1, 2)) {
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
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &eventmask, sizeof(unsigned short),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned short)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned short");
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_poll_register_impl((pollObject *)PyValue_AsKnownObjectBorrow(self), fd, eventmask);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_modify__doc__,
"modify($self, fd, eventmask, /)\n"
"--\n"
"\n"
"Modify an already registered file descriptor.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    a bitmask describing the type of events to check for");

#define SELECT_POLL_MODIFY_METHODDEF    \
    {"modify", _PyCFunction_CAST(select_poll_modify), METH_VALUE, select_poll_modify__doc__},

static PyValue
select_poll_modify_impl(pollObject *self, int fd, unsigned short eventmask);

static PyValue
select_poll_modify(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    unsigned short eventmask;

    if (!_PyArg_CheckPositional("modify", nargs, 2, 2)) {
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
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &eventmask, sizeof(unsigned short),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned short)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned short");
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_poll_modify_impl((pollObject *)PyValue_AsKnownObjectBorrow(self), fd, eventmask);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_unregister__doc__,
"unregister($self, fd, /)\n"
"--\n"
"\n"
"Remove a file descriptor being tracked by the polling object.");

#define SELECT_POLL_UNREGISTER_METHODDEF    \
    {"unregister", _PyCFunction_CAST(select_poll_unregister), METH_VALUE|METH_O, select_poll_unregister__doc__},

static PyValue
select_poll_unregister_impl(pollObject *self, int fd);

static PyValue
select_poll_unregister(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_poll_unregister_impl((pollObject *)PyValue_AsKnownObjectBorrow(self), fd);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_poll__doc__,
"poll($self, timeout=None, /)\n"
"--\n"
"\n"
"Polls the set of registered file descriptors.\n"
"\n"
"  timeout\n"
"    The maximum time to wait in milliseconds, or else None (or a negative\n"
"    value) to wait indefinitely.\n"
"\n"
"Returns a list containing any descriptors that have events or errors\n"
"to report, as a list of (fd, event) 2-tuples.");

#define SELECT_POLL_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_poll_poll), METH_VALUE, select_poll_poll__doc__},

static PyValue
select_poll_poll_impl(pollObject *self, PyValueBorrow timeout_obj);

static PyValue
select_poll_poll(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("poll", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    timeout_obj = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_poll_poll_impl((pollObject *)PyValue_AsKnownObjectBorrow(self), timeout_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_register__doc__,
"register($self, fd,\n"
"         eventmask=select.POLLIN | select.POLLPRI | select.POLLOUT, /)\n"
"--\n"
"\n"
"Register a file descriptor with the polling object.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_DEVPOLL_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(select_devpoll_register), METH_VALUE, select_devpoll_register__doc__},

static PyValue
select_devpoll_register_impl(devpollObject *self, int fd,
                             unsigned short eventmask);

static PyValue
select_devpoll_register(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!_PyArg_CheckPositional("register", nargs, 1, 2)) {
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
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &eventmask, sizeof(unsigned short),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned short)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned short");
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_devpoll_register_impl((devpollObject *)PyValue_AsKnownObjectBorrow(self), fd, eventmask);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_modify__doc__,
"modify($self, fd,\n"
"       eventmask=select.POLLIN | select.POLLPRI | select.POLLOUT, /)\n"
"--\n"
"\n"
"Modify a possible already registered file descriptor.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_DEVPOLL_MODIFY_METHODDEF    \
    {"modify", _PyCFunction_CAST(select_devpoll_modify), METH_VALUE, select_devpoll_modify__doc__},

static PyValue
select_devpoll_modify_impl(devpollObject *self, int fd,
                           unsigned short eventmask);

static PyValue
select_devpoll_modify(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!_PyArg_CheckPositional("modify", nargs, 1, 2)) {
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
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &eventmask, sizeof(unsigned short),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned short)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned short");
            goto exit;
        }
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_devpoll_modify_impl((devpollObject *)PyValue_AsKnownObjectBorrow(self), fd, eventmask);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_unregister__doc__,
"unregister($self, fd, /)\n"
"--\n"
"\n"
"Remove a file descriptor being tracked by the polling object.");

#define SELECT_DEVPOLL_UNREGISTER_METHODDEF    \
    {"unregister", _PyCFunction_CAST(select_devpoll_unregister), METH_VALUE|METH_O, select_devpoll_unregister__doc__},

static PyValue
select_devpoll_unregister_impl(devpollObject *self, int fd);

static PyValue
select_devpoll_unregister(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_devpoll_unregister_impl((devpollObject *)PyValue_AsKnownObjectBorrow(self), fd);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_poll__doc__,
"poll($self, timeout=None, /)\n"
"--\n"
"\n"
"Polls the set of registered file descriptors.\n"
"\n"
"  timeout\n"
"    The maximum time to wait in milliseconds, or else None (or\n"
"    a negative value) to wait indefinitely.\n"
"\n"
"Returns a list containing any descriptors that have events or errors\n"
"to report, as a list of (fd, event) 2-tuples.");

#define SELECT_DEVPOLL_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_devpoll_poll), METH_VALUE, select_devpoll_poll__doc__},

static PyValue
select_devpoll_poll_impl(devpollObject *self, PyValueBorrow timeout_obj);

static PyValue
select_devpoll_poll(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("poll", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    timeout_obj = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_devpoll_poll_impl((devpollObject *)PyValue_AsKnownObjectBorrow(self), timeout_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the devpoll file descriptor.\n"
"\n"
"Further operations on the devpoll object will raise an exception.");

#define SELECT_DEVPOLL_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(select_devpoll_close), METH_VALUE|METH_NOARGS, select_devpoll_close__doc__},

static PyValue
select_devpoll_close_impl(devpollObject *self);

static PyValue
select_devpoll_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_devpoll_close_impl((devpollObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the file descriptor.");

#define SELECT_DEVPOLL_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(select_devpoll_fileno), METH_VALUE|METH_NOARGS, select_devpoll_fileno__doc__},

static PyValue
select_devpoll_fileno_impl(devpollObject *self);

static PyValue
select_devpoll_fileno(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_devpoll_fileno_impl((devpollObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll__doc__,
"poll($module, /)\n"
"--\n"
"\n"
"Returns a polling object.\n"
"\n"
"This object supports registering and unregistering file descriptors, and\n"
"then polling them for I/O events.");

#define SELECT_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_poll), METH_VALUE|METH_NOARGS, select_poll__doc__},

static PyValue
select_poll_impl(PyObject *module);

static PyValue
select_poll(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = select_poll_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) */

#if ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll__doc__,
"devpoll($module, /)\n"
"--\n"
"\n"
"Returns a polling object.\n"
"\n"
"This object supports registering and unregistering file descriptors, and\n"
"then polling them for I/O events.");

#define SELECT_DEVPOLL_METHODDEF    \
    {"devpoll", _PyCFunction_CAST(select_devpoll), METH_VALUE|METH_NOARGS, select_devpoll__doc__},

static PyValue
select_devpoll_impl(PyObject *module);

static PyValue
select_devpoll(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = select_devpoll_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* ((defined(HAVE_POLL) || defined(HAVE_PPOLL)) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll__doc__,
"epoll(sizehint=-1, flags=0)\n"
"--\n"
"\n"
"Returns an epolling object.\n"
"\n"
"  sizehint\n"
"    The expected number of events to be registered.  It must be positive,\n"
"    or -1 to use the default.  It is only used on older systems where\n"
"    epoll_create1() is not available; otherwise it has no effect (though its\n"
"    value is still checked).\n"
"  flags\n"
"    Deprecated and completely ignored.  However, when supplied, its value\n"
"    must be 0 or select.EPOLL_CLOEXEC, otherwise OSError is raised.");

static PyValue
select_epoll_impl(PyTypeObject *type, int sizehint, int flags);

static PyValue
select_epoll_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sizehint)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sizehint", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "epoll",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int sizehint = -1;
    int flags = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        sizehint = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (sizehint == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = select_epoll_impl(type, sizehint, flags);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
select_epoll(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(select_epoll_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = select_epoll_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the epoll control file descriptor.\n"
"\n"
"Further operations on the epoll object will raise an exception.");

#define SELECT_EPOLL_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(select_epoll_close), METH_VALUE|METH_NOARGS, select_epoll_close__doc__},

static PyValue
select_epoll_close_impl(pyEpoll_Object *self);

static PyValue
select_epoll_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_epoll_close_impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the epoll control file descriptor.");

#define SELECT_EPOLL_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(select_epoll_fileno), METH_VALUE|METH_NOARGS, select_epoll_fileno__doc__},

static PyValue
select_epoll_fileno_impl(pyEpoll_Object *self);

static PyValue
select_epoll_fileno(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = select_epoll_fileno_impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_fromfd__doc__,
"fromfd($type, fd, /)\n"
"--\n"
"\n"
"Create an epoll object from a given control fd.");

#define SELECT_EPOLL_FROMFD_METHODDEF    \
    {"fromfd", _PyCFunction_CAST(select_epoll_fromfd), METH_VALUE|METH_O|METH_CLASS, select_epoll_fromfd__doc__},

static PyValue
select_epoll_fromfd_impl(PyTypeObject *type, int fd);

static PyValue
select_epoll_fromfd(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;

    fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = select_epoll_fromfd_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_register__doc__,
"register($self, /, fd,\n"
"         eventmask=select.EPOLLIN | select.EPOLLPRI | select.EPOLLOUT)\n"
"--\n"
"\n"
"Registers a new fd or raises an OSError if the fd is already registered.\n"
"\n"
"  fd\n"
"    the target file descriptor of the operation\n"
"  eventmask\n"
"    a bit set composed of the various EPOLL constants\n"
"\n"
"The epoll interface supports all file descriptors that support poll.");

#define SELECT_EPOLL_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(select_epoll_register), METH_VALUE|METH_KEYWORDS, select_epoll_register__doc__},

static PyValue
select_epoll_register_impl(pyEpoll_Object *self, int fd,
                           unsigned int eventmask);

static PyValue
select_epoll_register(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(fd)), _PyValue_STATIC_IMMORTAL(&_Py_ID(eventmask)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"fd", "eventmask", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "register",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int fd;
    unsigned int eventmask = EPOLLIN | EPOLLPRI | EPOLLOUT;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[_idx[0]]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
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
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[_idx[0]]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[1]]), &eventmask, sizeof(unsigned int),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned int)) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    "integer value out of range", 1) < 0)
                {
                    goto exit;
                }
            }
        }
    }
    return_value = select_epoll_register_impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self), fd, eventmask);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_modify__doc__,
"modify($self, /, fd, eventmask)\n"
"--\n"
"\n"
"Modify event mask for a registered file descriptor.\n"
"\n"
"  fd\n"
"    the target file descriptor of the operation\n"
"  eventmask\n"
"    a bit set composed of the various EPOLL constants");

#define SELECT_EPOLL_MODIFY_METHODDEF    \
    {"modify", _PyCFunction_CAST(select_epoll_modify), METH_VALUE|METH_KEYWORDS, select_epoll_modify__doc__},

static PyValue
select_epoll_modify_impl(pyEpoll_Object *self, int fd,
                         unsigned int eventmask);

static PyValue
select_epoll_modify(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(fd)), _PyValue_STATIC_IMMORTAL(&_Py_ID(eventmask)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"fd", "eventmask", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "modify",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int fd;
    unsigned int eventmask;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[_idx[0]]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
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
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[_idx[0]]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[1]]), &eventmask, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = select_epoll_modify_impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self), fd, eventmask);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_unregister__doc__,
"unregister($self, /, fd)\n"
"--\n"
"\n"
"Remove a registered file descriptor from the epoll object.\n"
"\n"
"  fd\n"
"    the target file descriptor of the operation");

#define SELECT_EPOLL_UNREGISTER_METHODDEF    \
    {"unregister", _PyCFunction_CAST(select_epoll_unregister), METH_VALUE|METH_KEYWORDS, select_epoll_unregister__doc__},

static PyValue
select_epoll_unregister_impl(pyEpoll_Object *self, int fd);

static PyValue
select_epoll_unregister(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(fd)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "unregister",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int fd;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (PyValue_IsInlineInt(PyValue_Borrow(args[_idx[0]]))) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
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
        PyObject *_obj = _PyValue_AsObjectAt(PyValue_Borrow(args[_idx[0]]), BOX_SITE_PARSER);
        if (_obj == NULL) {
            goto exit;
        }
        fd = PyObject_AsFileDescriptor(_obj);
        Py_DECREF(_obj);
        if (fd < 0) {
            goto exit;
        }
    }
    return_value = select_epoll_unregister_impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self), fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_poll__doc__,
"poll($self, /, timeout=None, maxevents=-1)\n"
"--\n"
"\n"
"Wait for events on the epoll file descriptor.\n"
"\n"
"  timeout\n"
"    the maximum time to wait in seconds (with fractions);\n"
"    a timeout of None or -1 makes poll wait indefinitely\n"
"  maxevents\n"
"    the maximum number of events returned; -1 means no limit\n"
"\n"
"Returns a list containing any descriptors that have events to\n"
"report, as a list of (fd, events) 2-tuples.");

#define SELECT_EPOLL_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_epoll_poll), METH_VALUE|METH_KEYWORDS, select_epoll_poll__doc__},

static PyValue
select_epoll_poll_impl(pyEpoll_Object *self, PyValueBorrow timeout_obj,
                       int maxevents);

static PyValue
select_epoll_poll(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(timeout)), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxevents)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timeout", "maxevents", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "poll",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow timeout_obj = PyValue_Borrow(PyValue_None);
    int maxevents = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        timeout_obj = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        maxevents = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (maxevents == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = select_epoll_poll_impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self), timeout_obj, maxevents);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define SELECT_EPOLL___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(select_epoll___enter__), METH_VALUE|METH_NOARGS, select_epoll___enter____doc__},

static PyValue
select_epoll___enter___impl(pyEpoll_Object *self);

static PyValue
select_epoll___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = select_epoll___enter___impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll___exit____doc__,
"__exit__($self, exc_type=None, exc_value=None, exc_tb=None, /)\n"
"--\n"
"\n");

#define SELECT_EPOLL___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(select_epoll___exit__), METH_VALUE, select_epoll___exit____doc__},

static PyValue
select_epoll___exit___impl(pyEpoll_Object *self, PyValueBorrow exc_type,
                           PyValueBorrow exc_value, PyValueBorrow exc_tb);

static PyValue
select_epoll___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow exc_type = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_value = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_tb = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("__exit__", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    exc_type = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    exc_value = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    exc_tb = PyValue_Borrow(args[2]);
skip_optional:
    return_value = select_epoll___exit___impl((pyEpoll_Object *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue__doc__,
"kqueue()\n"
"--\n"
"\n"
"Kqueue syscall wrapper.\n"
"\n"
"For example, to start watching a socket for input:\n"
">>> kq = kqueue()\n"
">>> sock = socket()\n"
">>> sock.connect((host, port))\n"
">>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_ADD)], 0)\n"
"\n"
"To wait one second for it to become writeable:\n"
">>> kq.control(None, 1, 1000)\n"
"\n"
"To stop listening:\n"
">>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_DELETE)], 0)");

static PyValue
select_kqueue_impl(PyTypeObject *type);

static PyValue
select_kqueue_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = _selectstate_by_type(type)->kqueue_queue_Type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("kqueue", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("kqueue", kwnames)) {
        goto exit;
    }
    return_value = select_kqueue_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
select_kqueue(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(select_kqueue_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = select_kqueue_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the kqueue control file descriptor.\n"
"\n"
"Further operations on the kqueue object will raise an exception.");

#define SELECT_KQUEUE_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(select_kqueue_close), METH_VALUE|METH_NOARGS, select_kqueue_close__doc__},

static PyValue
select_kqueue_close_impl(kqueue_queue_Object *self);

static PyValue
select_kqueue_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = select_kqueue_close_impl((kqueue_queue_Object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the kqueue control file descriptor.");

#define SELECT_KQUEUE_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(select_kqueue_fileno), METH_VALUE|METH_NOARGS, select_kqueue_fileno__doc__},

static PyValue
select_kqueue_fileno_impl(kqueue_queue_Object *self);

static PyValue
select_kqueue_fileno(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = select_kqueue_fileno_impl((kqueue_queue_Object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_fromfd__doc__,
"fromfd($type, fd, /)\n"
"--\n"
"\n"
"Create a kqueue object from a given control fd.");

#define SELECT_KQUEUE_FROMFD_METHODDEF    \
    {"fromfd", _PyCFunction_CAST(select_kqueue_fromfd), METH_VALUE|METH_O|METH_CLASS, select_kqueue_fromfd__doc__},

static PyValue
select_kqueue_fromfd_impl(PyTypeObject *type, int fd);

static PyValue
select_kqueue_fromfd(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;

    fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = select_kqueue_fromfd_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_control__doc__,
"control($self, changelist, maxevents, timeout=None, /)\n"
"--\n"
"\n"
"Calls the kernel kevent function.\n"
"\n"
"  changelist\n"
"    Must be an iterable of kevent objects describing the changes to be made\n"
"    to the kernel\'s watch list or None.\n"
"  maxevents\n"
"    The maximum number of events that the kernel will return.\n"
"  timeout\n"
"    The maximum time to wait in seconds, or else None to wait forever.\n"
"    This accepts non-integers for smaller timeouts, too.");

#define SELECT_KQUEUE_CONTROL_METHODDEF    \
    {"control", _PyCFunction_CAST(select_kqueue_control), METH_VALUE, select_kqueue_control__doc__},

static PyValue
select_kqueue_control_impl(kqueue_queue_Object *self,
                           PyValueBorrow changelist, int maxevents,
                           PyValueBorrow otimeout);

static PyValue
select_kqueue_control(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow changelist;
    int maxevents;
    PyValueBorrow otimeout = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("control", nargs, 2, 3)) {
        goto exit;
    }
    changelist = PyValue_Borrow(args[0]);
    maxevents = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (maxevents == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    otimeout = PyValue_Borrow(args[2]);
skip_optional:
    return_value = select_kqueue_control_impl((kqueue_queue_Object *)PyValue_AsKnownObjectBorrow(self), changelist, maxevents, otimeout);

exit:
    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#ifndef SELECT_POLL_REGISTER_METHODDEF
    #define SELECT_POLL_REGISTER_METHODDEF
#endif /* !defined(SELECT_POLL_REGISTER_METHODDEF) */

#ifndef SELECT_POLL_MODIFY_METHODDEF
    #define SELECT_POLL_MODIFY_METHODDEF
#endif /* !defined(SELECT_POLL_MODIFY_METHODDEF) */

#ifndef SELECT_POLL_UNREGISTER_METHODDEF
    #define SELECT_POLL_UNREGISTER_METHODDEF
#endif /* !defined(SELECT_POLL_UNREGISTER_METHODDEF) */

#ifndef SELECT_POLL_POLL_METHODDEF
    #define SELECT_POLL_POLL_METHODDEF
#endif /* !defined(SELECT_POLL_POLL_METHODDEF) */

#ifndef SELECT_DEVPOLL_REGISTER_METHODDEF
    #define SELECT_DEVPOLL_REGISTER_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_REGISTER_METHODDEF) */

#ifndef SELECT_DEVPOLL_MODIFY_METHODDEF
    #define SELECT_DEVPOLL_MODIFY_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_MODIFY_METHODDEF) */

#ifndef SELECT_DEVPOLL_UNREGISTER_METHODDEF
    #define SELECT_DEVPOLL_UNREGISTER_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_UNREGISTER_METHODDEF) */

#ifndef SELECT_DEVPOLL_POLL_METHODDEF
    #define SELECT_DEVPOLL_POLL_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_POLL_METHODDEF) */

#ifndef SELECT_DEVPOLL_CLOSE_METHODDEF
    #define SELECT_DEVPOLL_CLOSE_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_CLOSE_METHODDEF) */

#ifndef SELECT_DEVPOLL_FILENO_METHODDEF
    #define SELECT_DEVPOLL_FILENO_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_FILENO_METHODDEF) */

#ifndef SELECT_POLL_METHODDEF
    #define SELECT_POLL_METHODDEF
#endif /* !defined(SELECT_POLL_METHODDEF) */

#ifndef SELECT_DEVPOLL_METHODDEF
    #define SELECT_DEVPOLL_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_METHODDEF) */

#ifndef SELECT_EPOLL_CLOSE_METHODDEF
    #define SELECT_EPOLL_CLOSE_METHODDEF
#endif /* !defined(SELECT_EPOLL_CLOSE_METHODDEF) */

#ifndef SELECT_EPOLL_FILENO_METHODDEF
    #define SELECT_EPOLL_FILENO_METHODDEF
#endif /* !defined(SELECT_EPOLL_FILENO_METHODDEF) */

#ifndef SELECT_EPOLL_FROMFD_METHODDEF
    #define SELECT_EPOLL_FROMFD_METHODDEF
#endif /* !defined(SELECT_EPOLL_FROMFD_METHODDEF) */

#ifndef SELECT_EPOLL_REGISTER_METHODDEF
    #define SELECT_EPOLL_REGISTER_METHODDEF
#endif /* !defined(SELECT_EPOLL_REGISTER_METHODDEF) */

#ifndef SELECT_EPOLL_MODIFY_METHODDEF
    #define SELECT_EPOLL_MODIFY_METHODDEF
#endif /* !defined(SELECT_EPOLL_MODIFY_METHODDEF) */

#ifndef SELECT_EPOLL_UNREGISTER_METHODDEF
    #define SELECT_EPOLL_UNREGISTER_METHODDEF
#endif /* !defined(SELECT_EPOLL_UNREGISTER_METHODDEF) */

#ifndef SELECT_EPOLL_POLL_METHODDEF
    #define SELECT_EPOLL_POLL_METHODDEF
#endif /* !defined(SELECT_EPOLL_POLL_METHODDEF) */

#ifndef SELECT_EPOLL___ENTER___METHODDEF
    #define SELECT_EPOLL___ENTER___METHODDEF
#endif /* !defined(SELECT_EPOLL___ENTER___METHODDEF) */

#ifndef SELECT_EPOLL___EXIT___METHODDEF
    #define SELECT_EPOLL___EXIT___METHODDEF
#endif /* !defined(SELECT_EPOLL___EXIT___METHODDEF) */

#ifndef SELECT_KQUEUE_CLOSE_METHODDEF
    #define SELECT_KQUEUE_CLOSE_METHODDEF
#endif /* !defined(SELECT_KQUEUE_CLOSE_METHODDEF) */

#ifndef SELECT_KQUEUE_FILENO_METHODDEF
    #define SELECT_KQUEUE_FILENO_METHODDEF
#endif /* !defined(SELECT_KQUEUE_FILENO_METHODDEF) */

#ifndef SELECT_KQUEUE_FROMFD_METHODDEF
    #define SELECT_KQUEUE_FROMFD_METHODDEF
#endif /* !defined(SELECT_KQUEUE_FROMFD_METHODDEF) */

#ifndef SELECT_KQUEUE_CONTROL_METHODDEF
    #define SELECT_KQUEUE_CONTROL_METHODDEF
#endif /* !defined(SELECT_KQUEUE_CONTROL_METHODDEF) */
/*[clinic end generated code: output=e443ceaebcfda8a8 input=a9049054013a1b77]*/
