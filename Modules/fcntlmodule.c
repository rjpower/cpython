/* fcntl module */

// Argument Clinic uses the internal C API
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_abstract.h"  // _PyIndex_Check()
#include "pycore_long_value.h" // _PyValue_AsLong()
#include "pycore_value.h"      // PyValue_AsKnownObjectBorrow()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()

#include <errno.h>                // EINTR
#include <fcntl.h>                // fcntl()
#include <string.h>               // memcpy()
#include <sys/ioctl.h>            // ioctl()
#ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>           // flock()
#endif
#ifdef HAVE_LINUX_FS_H
#  include <linux/fs.h>
#endif
#ifdef HAVE_STROPTS_H
#  include <stropts.h>            // I_FLUSHBAND
#endif

#define GUARDSZ 8
// NUL followed by random bytes.
static const char guard[GUARDSZ] _Py_NONSTRING = "\x00\xfa\x69\xc4\x67\xa3\x6c\x58";

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

// _PyIndex_Check() of a value: an inline int has __index__, a float not.
static int
fcntl_index_check(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        return PyValue_IsInlineInt(v);
    }
    return _PyIndex_Check(PyValue_AsKnownObjectBorrow(v));
}

// PySys_Audit() takes objects: box an inline value for the event arguments.
// Returns a new reference (None for PyValue_NULL) or NULL with an exception.
static PyObject *
fcntl_audit_arg(PyValueBorrow v)
{
    if (PyValue_IsNull(v)) {
        return Py_NewRef(Py_None);
    }
    return _PyValue_AsObjectAt(v, BOX_SITE_PARSER);
}

// PyArg_Parse() of a value, which may be an inline int.
static int
fcntl_arg_parse(PyValueBorrow v, const char *format, void *result)
{
    PyObject *obj = _PyValue_AsObjectAt(v, BOX_SITE_PARSER);
    if (obj == NULL) {
        return 0;
    }
    int ok = PyArg_Parse(obj, format, result);
    Py_DECREF(obj);
    return ok;
}

/*[clinic input]
module fcntl
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=124b58387c158179]*/

#include "clinic/fcntlmodule.c.h"

/*[clinic input]
fcntl.fcntl

    fd: fildes
    cmd as code: int
    arg: object(c_default='NULL') = 0
    /

Perform the operation cmd on file descriptor fd.

The values used for cmd are operating system dependent, and are
available as constants in the fcntl module, using the same names as used
in the relevant C header files.  The argument arg is optional, and
defaults to 0; it may be an integer, a bytes-like object or a string.
If arg is given as a string, it will be encoded to binary using the
UTF-8 encoding.

If the arg given is an integer or if none is specified, the result value
is an integer corresponding to the return value of the fcntl() call in
the C code.

If arg is given as a bytes-like object, the return value of fcntl() is a
bytes object of that length, containing the resulting value put in the
arg buffer by the operating system.
[clinic start generated code]*/

static PyValue
fcntl_fcntl_impl(PyObject *module, int fd, int code, PyValueBorrow arg)
/*[clinic end generated code: output=8e55a63e262e9288 input=77340720f11665da]*/
{
    int ret;
    int async_err = 0;

    PyObject *audit_arg = fcntl_audit_arg(arg);
    if (audit_arg == NULL) {
        return PyValue_NULL;
    }
    int audit_res = PySys_Audit("fcntl.fcntl", "iiO", fd, code, audit_arg);
    Py_DECREF(audit_arg);
    if (audit_res < 0) {
        return PyValue_NULL;
    }

    if (PyValue_IsNull(arg) || fcntl_index_check(arg)) {
        unsigned int int_arg = 0;
        if (!PyValue_IsNull(arg)) {
            if (!fcntl_arg_parse(arg, "I", &int_arg)) {
                return PyValue_NULL;
            }
        }

        do {
            Py_BEGIN_ALLOW_THREADS
            ret = fcntl(fd, code, (int)int_arg);
            Py_END_ALLOW_THREADS
        } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
        if (ret < 0) {
            return _PyValue_FromObjectResult(!async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL);
        }
        return PyValue_FromInt64(ret);
    }
    if ((PyValue_IsObject(arg) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(arg))) || (PyValue_IsObject(arg) && PyObject_CheckBuffer(PyValue_AsKnownObjectBorrow(arg)))) {
        Py_buffer view;
#define FCNTL_BUFSZ 1024
        /* argument plus NUL byte plus guard to detect a buffer overflow */
        char buf[FCNTL_BUFSZ+GUARDSZ];

        if (!PyArg_Parse(PyValue_AsKnownObjectBorrow(arg), "s*", &view)) {
            return PyValue_NULL;
        }
        Py_ssize_t len = view.len;
        if (len <= FCNTL_BUFSZ) {
            memcpy(buf, view.buf, len);
            memcpy(buf + len, guard, GUARDSZ);
            PyBuffer_Release(&view);

            do {
                Py_BEGIN_ALLOW_THREADS
                ret = fcntl(fd, code, buf);
                Py_END_ALLOW_THREADS
            } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
            if (ret < 0) {
                return _PyValue_FromObjectResult(!async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL);
            }
            if (memcmp(buf + len, guard, GUARDSZ) != 0) {
                PyErr_SetString(PyExc_SystemError,
                        "Possible stack corruption in fcntl() due to "
                        "buffer overflow. "
                        "Provide an argument of sufficient size as "
                        "determined by the operation.");
                return PyValue_NULL;
            }
            return _PyValue_FromObjectResult(PyBytes_FromStringAndSize(buf, len));
        }
        else {
            PyBytesWriter *writer = PyBytesWriter_Create(len + GUARDSZ);
            if (writer == NULL) {
                PyBuffer_Release(&view);
                return PyValue_NULL;
            }
            char *ptr = PyBytesWriter_GetData(writer);
            memcpy(ptr, view.buf, len);
            memcpy(ptr + len, guard, GUARDSZ);
            PyBuffer_Release(&view);

            do {
                Py_BEGIN_ALLOW_THREADS
                ret = fcntl(fd, code, ptr);
                Py_END_ALLOW_THREADS
            } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
            if (ret < 0) {
                if (!async_err) {
                    PyErr_SetFromErrno(PyExc_OSError);
                }
                PyBytesWriter_Discard(writer);
                return PyValue_NULL;
            }
            if (memcmp(ptr + len, guard, GUARDSZ) != 0) {
                PyErr_SetString(PyExc_SystemError,
                        "Memory corruption in fcntl() due to "
                        "buffer overflow. "
                        "Provide an argument of sufficient size as "
                        "determined by the operation.");
                PyBytesWriter_Discard(writer);
                return PyValue_NULL;
            }
            // Truncate the trailing guard bytes
            return _PyValue_FromObjectResult(PyBytesWriter_FinishWithSize(writer, len));
        }
#undef FCNTL_BUFSZ
    }
    PyErr_Format(PyExc_TypeError,
                 "fcntl() argument 3 must be an integer, "
                 "a bytes-like object, or a string, not %vT",
                 arg);
    return PyValue_NULL;
}


/*[clinic input]
fcntl.ioctl

    fd: fildes
    request as code: unsigned_long(bitwise=True)
    arg: object(c_default='NULL') = 0
    mutate_flag as mutate_arg: bool = True
    /

Perform the operation request on file descriptor fd.

The values used for request are operating system dependent, and are
available as constants in the fcntl or termios library modules, using
the same names as used in the relevant C header files.

The argument arg is optional, and defaults to 0; it may be an integer, a
bytes-like object or a string.  If arg is given as a string, it will be
encoded to binary using the UTF-8 encoding.

If the arg given is an integer or if none is specified, the result value
is an integer corresponding to the return value of the ioctl() call in
the C code.

If the argument is a mutable buffer (such as a bytearray) and the
mutate_flag argument is true (default) then the buffer is (in effect)
passed to the operating system and changes made by the OS will be
reflected in the contents of the buffer after the call has returned.
The return value is the integer returned by the ioctl() system call.

If the argument is a mutable buffer and the mutable_flag argument is
false, the behavior is as if an immutable buffer had been passed.

If the argument is an immutable buffer then a copy of the buffer is
passed to the operating system and the return value is a bytes object of
the same length containing whatever the operating system put in the
buffer.
[clinic start generated code]*/

static PyValue
fcntl_ioctl_impl(PyObject *module, int fd, unsigned long code,
                 PyValueBorrow arg, int mutate_arg)
/*[clinic end generated code: output=366c7cb70dd16486 input=954fe75c208cc492]*/
{
    /* We use the unsigned non-checked 'I' format for the 'code' parameter
       because the system expects it to be a 32bit bit field value
       regardless of it being passed as an int or unsigned long on
       various platforms.  See the termios.TIOCSWINSZ constant across
       platforms for an example of this.

       If any of the 64bit platforms ever decide to use more than 32bits
       in their unsigned long ioctl codes this will break and need
       special casing based on the platform being built on.
     */
    int ret;
    int async_err = 0;

    PyObject *audit_arg = fcntl_audit_arg(arg);
    if (audit_arg == NULL) {
        return PyValue_NULL;
    }
    int audit_res = PySys_Audit("fcntl.ioctl", "ikO", fd, code, audit_arg);
    Py_DECREF(audit_arg);
    if (audit_res < 0) {
        return PyValue_NULL;
    }

    if (PyValue_IsNull(arg) || fcntl_index_check(arg)) {
        int int_arg = 0;
        if (!PyValue_IsNull(arg)) {
            if (!fcntl_arg_parse(arg, "i", &int_arg)) {
                return PyValue_NULL;
            }
        }

        do {
            Py_BEGIN_ALLOW_THREADS
            ret = ioctl(fd, code, int_arg);
            Py_END_ALLOW_THREADS
        } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
        if (ret < 0) {
            return _PyValue_FromObjectResult(!async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL);
        }
        return PyValue_FromInt64(ret);
    }
    if ((PyValue_IsObject(arg) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(arg))) || (PyValue_IsObject(arg) && PyObject_CheckBuffer(PyValue_AsKnownObjectBorrow(arg)))) {
        Py_buffer view;
#define IOCTL_BUFSZ 1024
        /* argument plus NUL byte plus guard to detect a buffer overflow */
        char buf[IOCTL_BUFSZ+GUARDSZ];
        if (mutate_arg && !(PyValue_IsObject(arg) && PyBytes_Check(PyValue_AsKnownObjectBorrow(arg))) && !(PyValue_IsObject(arg) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(arg)))) {
            if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(arg), &view, PyBUF_WRITABLE) == 0) {
                Py_ssize_t len = view.len;
                void *ptr = view.buf;
                if (len <= IOCTL_BUFSZ) {
                    memcpy(buf, ptr, len);
                    memcpy(buf + len, guard, GUARDSZ);
                    ptr = buf;
                }
                do {
                    Py_BEGIN_ALLOW_THREADS
                    ret = ioctl(fd, code, ptr);
                    Py_END_ALLOW_THREADS
                } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
                if (ret < 0) {
                    if (!async_err) {
                        PyErr_SetFromErrno(PyExc_OSError);
                    }
                    PyBuffer_Release(&view);
                    return PyValue_NULL;
                }
                if (ptr == buf) {
                    memcpy(view.buf, buf, len);
                }
                PyBuffer_Release(&view);
                if (ptr == buf && memcmp(buf + len, guard, GUARDSZ) != 0) {
                    PyErr_SetString(PyExc_SystemError,
                            "Possible stack corruption in ioctl() due to "
                            "buffer overflow. "
                            "Provide a writable buffer argument of "
                            "sufficient size as determined by "
                            "the operation.");
                    return PyValue_NULL;
                }
                return PyValue_FromInt64(ret);
            }
            if (!PyErr_ExceptionMatches(PyExc_BufferError)) {
                return PyValue_NULL;
            }
            PyErr_Clear();
        }

        if (!PyArg_Parse(PyValue_AsKnownObjectBorrow(arg), "s*", &view)) {
            return PyValue_NULL;
        }
        Py_ssize_t len = view.len;
        if (len <= IOCTL_BUFSZ) {
            memcpy(buf, view.buf, len);
            memcpy(buf + len, guard, GUARDSZ);
            PyBuffer_Release(&view);

            do {
                Py_BEGIN_ALLOW_THREADS
                ret = ioctl(fd, code, buf);
                Py_END_ALLOW_THREADS
            } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
            if (ret < 0) {
                return _PyValue_FromObjectResult(!async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL);
            }
            if (memcmp(buf + len, guard, GUARDSZ) != 0) {
                PyErr_SetString(PyExc_SystemError,
                        "Possible stack corruption in ioctl() due to "
                        "buffer overflow. "
                        "Provide an argument of sufficient size as "
                        "determined by the operation.");
                return PyValue_NULL;
            }
            return _PyValue_FromObjectResult(PyBytes_FromStringAndSize(buf, len));
        }
        else {
            PyBytesWriter *writer = PyBytesWriter_Create(len + GUARDSZ);
            if (writer == NULL) {
                PyBuffer_Release(&view);
                return PyValue_NULL;
            }
            char *ptr = PyBytesWriter_GetData(writer);
            memcpy(ptr, view.buf, len);
            memcpy(ptr + len, guard, GUARDSZ);
            PyBuffer_Release(&view);

            do {
                Py_BEGIN_ALLOW_THREADS
                ret = ioctl(fd, code, ptr);
                Py_END_ALLOW_THREADS
            } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
            if (ret < 0) {
                if (!async_err) {
                    PyErr_SetFromErrno(PyExc_OSError);
                }
                PyBytesWriter_Discard(writer);
                return PyValue_NULL;
            }
            if (memcmp(ptr + len, guard, GUARDSZ) != 0) {
                PyErr_SetString(PyExc_SystemError,
                        "Memory corruption in ioctl() due to "
                        "buffer overflow. "
                        "Provide an argument of sufficient size as "
                        "determined by the operation.");
                PyBytesWriter_Discard(writer);
                return PyValue_NULL;
            }
            // Truncate the trailing guard bytes
            return _PyValue_FromObjectResult(PyBytesWriter_FinishWithSize(writer, len));
        }
#undef IOCTL_BUFSZ
    }
    PyErr_Format(PyExc_TypeError,
                 "ioctl() argument 3 must be an integer, "
                 "a bytes-like object, or a string, not %vT",
                 arg);
    return PyValue_NULL;
}

/*[clinic input]
fcntl.flock

    fd: fildes
    operation as code: int
    /

Perform the lock operation on file descriptor fd.

See the Unix manual page for flock(2) for details (On some systems, this
function is emulated using fcntl()).
[clinic start generated code]*/

static PyValue
fcntl_flock_impl(PyObject *module, int fd, int code)
/*[clinic end generated code: output=322893ae68db993c input=ade68943e8599f0a]*/
{
    int ret;
    int async_err = 0;

    if (PySys_Audit("fcntl.flock", "ii", fd, code) < 0) {
        return PyValue_NULL;
    }

#ifdef HAVE_FLOCK
    do {
        Py_BEGIN_ALLOW_THREADS
        ret = flock(fd, code);
        Py_END_ALLOW_THREADS
    } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
#else

#ifndef LOCK_SH
#define LOCK_SH         1       /* shared lock */
#define LOCK_EX         2       /* exclusive lock */
#define LOCK_NB         4       /* don't block when locking */
#define LOCK_UN         8       /* unlock */
#endif
    {
        struct flock l;
        if (code == LOCK_UN)
            l.l_type = F_UNLCK;
        else if (code & LOCK_SH)
            l.l_type = F_RDLCK;
        else if (code & LOCK_EX)
            l.l_type = F_WRLCK;
        else {
            PyErr_SetString(PyExc_ValueError,
                            "unrecognized flock argument");
            return NULL;
        }
        l.l_whence = l.l_start = l.l_len = 0;
        do {
            Py_BEGIN_ALLOW_THREADS
            ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
            Py_END_ALLOW_THREADS
        } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    }
#endif /* HAVE_FLOCK */
    if (ret < 0) {
        return _PyValue_FromObjectResult(!async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL);
    }
    return PyValue_None;
}


/*[clinic input]
fcntl.lockf

    fd: fildes
    cmd as code: int
    len as lenobj: object(c_default='NULL') = 0
    start as startobj: object(c_default='NULL') = 0
    whence: int = 0
    /

A wrapper around the fcntl() locking calls.

fd is the file descriptor of the file to lock or unlock, and operation
is one of the following values:

    LOCK_UN - unlock
    LOCK_SH - acquire a shared lock
    LOCK_EX - acquire an exclusive lock

When operation is LOCK_SH or LOCK_EX, it can also be bitwise ORed with
LOCK_NB to avoid blocking on lock acquisition.  If LOCK_NB is used and
the lock cannot be acquired, an OSError will be raised and the exception
will have an errno attribute set to EACCES or EAGAIN (depending on the
operating system -- for portability, check for either value).

len is the number of bytes to lock, with the default meaning to lock to
EOF.  start is the byte offset, relative to whence, to that the lock
starts.  whence is as with fileobj.seek(), specifically:

    0 - relative to the start of the file (SEEK_SET)
    1 - relative to the current buffer position (SEEK_CUR)
    2 - relative to the end of the file (SEEK_END)
[clinic start generated code]*/

static PyValue
fcntl_lockf_impl(PyObject *module, int fd, int code, PyValueBorrow lenobj,
                 PyValueBorrow startobj, int whence)
/*[clinic end generated code: output=9ecd71ee6c9392c7 input=369bef4d7a1c5ff4]*/
{
    int ret;
    int async_err = 0;

    PyObject *audit_len = fcntl_audit_arg(lenobj);
    PyObject *audit_start = audit_len == NULL ? NULL : fcntl_audit_arg(startobj);
    if (audit_start == NULL) {
        Py_XDECREF(audit_len);
        return PyValue_NULL;
    }
    int audit_res = PySys_Audit("fcntl.lockf", "iiOOi", fd, code, audit_len,
                                audit_start, whence);
    Py_DECREF(audit_len);
    Py_DECREF(audit_start);
    if (audit_res < 0) {
        return PyValue_NULL;
    }

#ifndef LOCK_SH
#define LOCK_SH         1       /* shared lock */
#define LOCK_EX         2       /* exclusive lock */
#define LOCK_NB         4       /* don't block when locking */
#define LOCK_UN         8       /* unlock */
#endif  /* LOCK_SH */
    {
        struct flock l;
        if (code == LOCK_UN)
            l.l_type = F_UNLCK;
        else if (code & LOCK_SH)
            l.l_type = F_RDLCK;
        else if (code & LOCK_EX)
            l.l_type = F_WRLCK;
        else {
            PyErr_SetString(PyExc_ValueError,
                            "unrecognized lockf argument");
            return PyValue_NULL;
        }
        l.l_start = l.l_len = 0;
        if (!PyValue_IsNull(startobj)) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
            l.l_start = _PyValue_AsLong(startobj);
#else
            l.l_start = PyValue_LongCheck(startobj) ?
                            _PyValue_AsLongLong(startobj) :
                    _PyValue_AsLong(startobj);
#endif
            if (PyErr_Occurred())
                return PyValue_NULL;
        }
        if (!PyValue_IsNull(lenobj)) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
            l.l_len = _PyValue_AsLong(lenobj);
#else
            l.l_len = PyValue_LongCheck(lenobj) ?
                            _PyValue_AsLongLong(lenobj) :
                    _PyValue_AsLong(lenobj);
#endif
            if (PyErr_Occurred())
                return PyValue_NULL;
        }
        l.l_whence = whence;
        do {
            Py_BEGIN_ALLOW_THREADS
            ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
            Py_END_ALLOW_THREADS
        } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    }
    if (ret < 0) {
        return _PyValue_FromObjectResult(!async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL);
    }
    return PyValue_None;
}

/* List of functions */

static PyMethodDef fcntl_methods[] = {
    FCNTL_FCNTL_METHODDEF
    FCNTL_IOCTL_METHODDEF
    FCNTL_FLOCK_METHODDEF
    FCNTL_LOCKF_METHODDEF
    {NULL, NULL}  /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module performs file control and I/O control on file\n\
descriptors.  It is an interface to the fcntl() and ioctl() Unix\n\
routines.  File descriptors can be obtained with the fileno() method of\n\
a file or socket object.");

/* Module initialisation */


static int
all_ins(PyObject* m)
{
    if (PyModule_AddIntMacro(m, LOCK_SH)) return -1;
    if (PyModule_AddIntMacro(m, LOCK_EX)) return -1;
    if (PyModule_AddIntMacro(m, LOCK_NB)) return -1;
    if (PyModule_AddIntMacro(m, LOCK_UN)) return -1;
/* GNU extensions, as of glibc 2.2.4 */
#ifdef LOCK_MAND
    if (PyModule_AddIntMacro(m, LOCK_MAND)) return -1;
#endif
#ifdef LOCK_READ
    if (PyModule_AddIntMacro(m, LOCK_READ)) return -1;
#endif
#ifdef LOCK_WRITE
    if (PyModule_AddIntMacro(m, LOCK_WRITE)) return -1;
#endif
#ifdef LOCK_RW
    if (PyModule_AddIntMacro(m, LOCK_RW)) return -1;
#endif

#ifdef F_DUPFD
    if (PyModule_AddIntMacro(m, F_DUPFD)) return -1;
#endif
#ifdef F_DUPFD_CLOEXEC
    if (PyModule_AddIntMacro(m, F_DUPFD_CLOEXEC)) return -1;
#endif
#ifdef F_GETFD
    if (PyModule_AddIntMacro(m, F_GETFD)) return -1;
#endif
#ifdef F_SETFD
    if (PyModule_AddIntMacro(m, F_SETFD)) return -1;
#endif
#ifdef F_GETFL
    if (PyModule_AddIntMacro(m, F_GETFL)) return -1;
#endif
#ifdef F_SETFL
    if (PyModule_AddIntMacro(m, F_SETFL)) return -1;
#endif
#ifdef F_GETLK
    if (PyModule_AddIntMacro(m, F_GETLK)) return -1;
#endif
#ifdef F_SETLK
    if (PyModule_AddIntMacro(m, F_SETLK)) return -1;
#endif
#ifdef F_SETLKW
    if (PyModule_AddIntMacro(m, F_SETLKW)) return -1;
#endif
#ifdef F_OFD_GETLK
    if (PyModule_AddIntMacro(m, F_OFD_GETLK)) return -1;
#endif
#ifdef F_OFD_SETLK
    if (PyModule_AddIntMacro(m, F_OFD_SETLK)) return -1;
#endif
#ifdef F_OFD_SETLKW
    if (PyModule_AddIntMacro(m, F_OFD_SETLKW)) return -1;
#endif
#ifdef F_GETOWN
    if (PyModule_AddIntMacro(m, F_GETOWN)) return -1;
#endif
#ifdef F_SETOWN
    if (PyModule_AddIntMacro(m, F_SETOWN)) return -1;
#endif
#ifdef F_GETPATH
    if (PyModule_AddIntMacro(m, F_GETPATH)) return -1;
#endif
#ifdef F_GETSIG
    if (PyModule_AddIntMacro(m, F_GETSIG)) return -1;
#endif
#ifdef F_SETSIG
    if (PyModule_AddIntMacro(m, F_SETSIG)) return -1;
#endif
#ifdef F_RDLCK
    if (PyModule_AddIntMacro(m, F_RDLCK)) return -1;
#endif
#ifdef F_WRLCK
    if (PyModule_AddIntMacro(m, F_WRLCK)) return -1;
#endif
#ifdef F_UNLCK
    if (PyModule_AddIntMacro(m, F_UNLCK)) return -1;
#endif
/* LFS constants */
#ifdef F_GETLK64
    if (PyModule_AddIntMacro(m, F_GETLK64)) return -1;
#endif
#ifdef F_SETLK64
    if (PyModule_AddIntMacro(m, F_SETLK64)) return -1;
#endif
#ifdef F_SETLKW64
    if (PyModule_AddIntMacro(m, F_SETLKW64)) return -1;
#endif
/* GNU extensions, as of glibc 2.2.4. */
#ifdef FASYNC
    if (PyModule_AddIntMacro(m, FASYNC)) return -1;
#endif
#ifdef F_SETLEASE
    if (PyModule_AddIntMacro(m, F_SETLEASE)) return -1;
#endif
#ifdef F_GETLEASE
    if (PyModule_AddIntMacro(m, F_GETLEASE)) return -1;
#endif
#ifdef F_NOTIFY
    if (PyModule_AddIntMacro(m, F_NOTIFY)) return -1;
#endif
#ifdef F_DUPFD_QUERY
    if (PyModule_AddIntMacro(m, F_DUPFD_QUERY)) return -1;
#endif
/* Old BSD flock(). */
#ifdef F_EXLCK
    if (PyModule_AddIntMacro(m, F_EXLCK)) return -1;
#endif
#ifdef F_SHLCK
    if (PyModule_AddIntMacro(m, F_SHLCK)) return -1;
#endif

/* Linux specifics */
#ifdef F_SETPIPE_SZ
    if (PyModule_AddIntMacro(m, F_SETPIPE_SZ)) return -1;
#endif
#ifdef F_GETPIPE_SZ
    if (PyModule_AddIntMacro(m, F_GETPIPE_SZ)) return -1;
#endif

/* On Android, FICLONE is blocked by SELinux. */
#ifndef __ANDROID__
#ifdef FICLONE
    if (PyModule_AddIntMacro(m, FICLONE)) return -1;
#endif
#ifdef FICLONERANGE
    if (PyModule_AddIntMacro(m, FICLONERANGE)) return -1;
#endif
#endif

#ifdef F_GETOWN_EX
    // since Linux 2.6.32
    if (PyModule_AddIntMacro(m, F_GETOWN_EX)) return -1;
    if (PyModule_AddIntMacro(m, F_SETOWN_EX)) return -1;
    if (PyModule_AddIntMacro(m, F_OWNER_TID)) return -1;
    if (PyModule_AddIntMacro(m, F_OWNER_PID)) return -1;
    if (PyModule_AddIntMacro(m, F_OWNER_PGRP)) return -1;
#endif
#ifdef F_GET_RW_HINT
    // since Linux 4.13
    if (PyModule_AddIntMacro(m, F_GET_RW_HINT)) return -1;
    if (PyModule_AddIntMacro(m, F_SET_RW_HINT)) return -1;
    if (PyModule_AddIntMacro(m, F_GET_FILE_RW_HINT)) return -1;
    if (PyModule_AddIntMacro(m, F_SET_FILE_RW_HINT)) return -1;
#ifndef RWH_WRITE_LIFE_NOT_SET  // typo in Linux < 5.5
# define RWH_WRITE_LIFE_NOT_SET RWF_WRITE_LIFE_NOT_SET
#endif
    if (PyModule_AddIntMacro(m, RWH_WRITE_LIFE_NOT_SET)) return -1;
    if (PyModule_AddIntMacro(m, RWH_WRITE_LIFE_NONE)) return -1;
    if (PyModule_AddIntMacro(m, RWH_WRITE_LIFE_SHORT)) return -1;
    if (PyModule_AddIntMacro(m, RWH_WRITE_LIFE_MEDIUM)) return -1;
    if (PyModule_AddIntMacro(m, RWH_WRITE_LIFE_LONG)) return -1;
    if (PyModule_AddIntMacro(m, RWH_WRITE_LIFE_EXTREME)) return -1;
#endif

/* OS X specifics */
#ifdef F_FULLFSYNC
    if (PyModule_AddIntMacro(m, F_FULLFSYNC)) return -1;
#endif
#ifdef F_NOCACHE
    if (PyModule_AddIntMacro(m, F_NOCACHE)) return -1;
#endif

/* FreeBSD specifics */
#ifdef F_DUP2FD
    if (PyModule_AddIntMacro(m, F_DUP2FD)) return -1;
#endif
#ifdef F_DUP2FD_CLOEXEC
    if (PyModule_AddIntMacro(m, F_DUP2FD_CLOEXEC)) return -1;
#endif
#ifdef F_READAHEAD
    if (PyModule_AddIntMacro(m, F_READAHEAD)) return -1;
#endif
#ifdef F_RDAHEAD
    if (PyModule_AddIntMacro(m, F_RDAHEAD)) return -1;
#endif
#ifdef F_ISUNIONSTACK
    if (PyModule_AddIntMacro(m, F_ISUNIONSTACK)) return -1;
#endif
#ifdef F_KINFO
    if (PyModule_AddIntMacro(m, F_KINFO)) return -1;
#endif

/* NetBSD specifics */
#ifdef F_CLOSEM
    if (PyModule_AddIntMacro(m, F_CLOSEM)) return -1;
#endif
#ifdef F_MAXFD
    if (PyModule_AddIntMacro(m, F_MAXFD)) return -1;
#endif
#ifdef F_GETNOSIGPIPE
    if (PyModule_AddIntMacro(m, F_GETNOSIGPIPE)) return -1;
#endif
#ifdef F_SETNOSIGPIPE
    if (PyModule_AddIntMacro(m, F_SETNOSIGPIPE)) return -1;
#endif

/* For F_{GET|SET}FL */
#ifdef FD_CLOEXEC
    if (PyModule_AddIntMacro(m, FD_CLOEXEC)) return -1;
#endif

/* For F_NOTIFY */
#ifdef DN_ACCESS
    if (PyModule_AddIntMacro(m, DN_ACCESS)) return -1;
#endif
#ifdef DN_MODIFY
    if (PyModule_AddIntMacro(m, DN_MODIFY)) return -1;
#endif
#ifdef DN_CREATE
    if (PyModule_AddIntMacro(m, DN_CREATE)) return -1;
#endif
#ifdef DN_DELETE
    if (PyModule_AddIntMacro(m, DN_DELETE)) return -1;
#endif
#ifdef DN_RENAME
    if (PyModule_AddIntMacro(m, DN_RENAME)) return -1;
#endif
#ifdef DN_ATTRIB
    if (PyModule_AddIntMacro(m, DN_ATTRIB)) return -1;
#endif
#ifdef DN_MULTISHOT
    if (PyModule_AddIntMacro(m, DN_MULTISHOT)) return -1;
#endif

#ifdef HAVE_STROPTS_H
    /* Unix 98 guarantees that these are in stropts.h. */
    if (PyModule_AddIntMacro(m, I_PUSH)) return -1;
    if (PyModule_AddIntMacro(m, I_POP)) return -1;
    if (PyModule_AddIntMacro(m, I_LOOK)) return -1;
    if (PyModule_AddIntMacro(m, I_FLUSH)) return -1;
    if (PyModule_AddIntMacro(m, I_FLUSHBAND)) return -1;
    if (PyModule_AddIntMacro(m, I_SETSIG)) return -1;
    if (PyModule_AddIntMacro(m, I_GETSIG)) return -1;
    if (PyModule_AddIntMacro(m, I_FIND)) return -1;
    if (PyModule_AddIntMacro(m, I_PEEK)) return -1;
    if (PyModule_AddIntMacro(m, I_SRDOPT)) return -1;
    if (PyModule_AddIntMacro(m, I_GRDOPT)) return -1;
    if (PyModule_AddIntMacro(m, I_NREAD)) return -1;
    if (PyModule_AddIntMacro(m, I_FDINSERT)) return -1;
    if (PyModule_AddIntMacro(m, I_STR)) return -1;
    if (PyModule_AddIntMacro(m, I_SWROPT)) return -1;
#ifdef I_GWROPT
    /* despite the comment above, old-ish glibcs miss a couple... */
    if (PyModule_AddIntMacro(m, I_GWROPT)) return -1;
#endif
    if (PyModule_AddIntMacro(m, I_SENDFD)) return -1;
    if (PyModule_AddIntMacro(m, I_RECVFD)) return -1;
    if (PyModule_AddIntMacro(m, I_LIST)) return -1;
    if (PyModule_AddIntMacro(m, I_ATMARK)) return -1;
    if (PyModule_AddIntMacro(m, I_CKBAND)) return -1;
    if (PyModule_AddIntMacro(m, I_GETBAND)) return -1;
    if (PyModule_AddIntMacro(m, I_CANPUT)) return -1;
    if (PyModule_AddIntMacro(m, I_SETCLTIME)) return -1;
#ifdef I_GETCLTIME
    if (PyModule_AddIntMacro(m, I_GETCLTIME)) return -1;
#endif
    if (PyModule_AddIntMacro(m, I_LINK)) return -1;
    if (PyModule_AddIntMacro(m, I_UNLINK)) return -1;
    if (PyModule_AddIntMacro(m, I_PLINK)) return -1;
    if (PyModule_AddIntMacro(m, I_PUNLINK)) return -1;
#endif
#ifdef F_ADD_SEALS
    /* Linux: file sealing for memfd_create() */
    if (PyModule_AddIntMacro(m, F_ADD_SEALS)) return -1;
    if (PyModule_AddIntMacro(m, F_GET_SEALS)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_SEAL)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_SHRINK)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_GROW)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_WRITE)) return -1;
#ifdef F_SEAL_FUTURE_WRITE
    if (PyModule_AddIntMacro(m, F_SEAL_FUTURE_WRITE)) return -1;
#endif
#endif
    return 0;
}

static int
fcntl_exec(PyObject *module)
{
    if (all_ins(module) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot fcntl_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, fcntl_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef fcntlmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "fcntl",
    .m_doc = module_doc,
    .m_size = 0,
    .m_methods = fcntl_methods,
    .m_slots = fcntl_slots,
};

PyMODINIT_FUNC
PyInit_fcntl(void)
{
    return PyModuleDef_Init(&fcntlmodule);
}
