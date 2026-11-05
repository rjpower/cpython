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
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

#if (defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4))

PyDoc_STRVAR(_socket_socket__accept__doc__,
"_accept($self, /)\n"
"--\n"
"\n"
"Wait for an incoming connection.\n"
"\n"
"Return a new socket file descriptor representing the connection, and\n"
"the address of the client.  For IP sockets, the address info is a\n"
"pair (hostaddr, port).");

#define _SOCKET_SOCKET__ACCEPT_METHODDEF    \
    {"_accept", _PyCFunction_CAST(_socket_socket__accept), METH_VALUE|METH_NOARGS, _socket_socket__accept__doc__},

static PyValue
_socket_socket__accept_impl(PySocketSockObject *s);

static PyValue
_socket_socket__accept(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket__accept_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

#endif /* (defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)) */

PyDoc_STRVAR(_socket_socket_setblocking__doc__,
"setblocking($self, flag, /)\n"
"--\n"
"\n"
"Set the socket to blocking (flag is true) or non-blocking (false).\n"
"\n"
"setblocking(True) is equivalent to settimeout(None);\n"
"setblocking(False) is equivalent to settimeout(0.0).");

#define _SOCKET_SOCKET_SETBLOCKING_METHODDEF    \
    {"setblocking", _PyCFunction_CAST(_socket_socket_setblocking), METH_VALUE|METH_O, _socket_socket_setblocking__doc__},

static PyValue
_socket_socket_setblocking_impl(PySocketSockObject *s, int flag);

static PyValue
_socket_socket_setblocking(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag;

    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
    return_value = _socket_socket_setblocking_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_socket_getblocking__doc__,
"getblocking($self, /)\n"
"--\n"
"\n"
"Return True if socket is in blocking mode, False if in non-blocking.");

#define _SOCKET_SOCKET_GETBLOCKING_METHODDEF    \
    {"getblocking", _PyCFunction_CAST(_socket_socket_getblocking), METH_VALUE|METH_NOARGS, _socket_socket_getblocking__doc__},

static PyValue
_socket_socket_getblocking_impl(PySocketSockObject *s);

static PyValue
_socket_socket_getblocking(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_getblocking_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

PyDoc_STRVAR(_socket_socket_settimeout__doc__,
"settimeout($self, timeout, /)\n"
"--\n"
"\n"
"Set a timeout on socket operations.\n"
"\n"
"\'timeout\' can be a float, giving in seconds, or None.  Setting a\n"
"timeout of None disables the timeout feature and is equivalent to\n"
"setblocking(1).  Setting a timeout of zero is the same as\n"
"setblocking(0).");

#define _SOCKET_SOCKET_SETTIMEOUT_METHODDEF    \
    {"settimeout", _PyCFunction_CAST(_socket_socket_settimeout), METH_VALUE|METH_O, _socket_socket_settimeout__doc__},

static PyValue
_socket_socket_settimeout_impl(PySocketSockObject *s, PyValueBorrow arg);

static PyValue
_socket_socket_settimeout(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _socket_socket_settimeout_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), arg);

    return return_value;
}

PyDoc_STRVAR(_socket_socket_gettimeout__doc__,
"gettimeout($self, /)\n"
"--\n"
"\n"
"Return the timeout in seconds (float) of socket operations.\n"
"\n"
"A timeout of None indicates that timeouts on socket operations are\n"
"disabled.");

#define _SOCKET_SOCKET_GETTIMEOUT_METHODDEF    \
    {"gettimeout", _PyCFunction_CAST(_socket_socket_gettimeout), METH_VALUE|METH_NOARGS, _socket_socket_gettimeout__doc__},

static PyValue
_socket_socket_gettimeout_impl(PySocketSockObject *self);

static PyValue
_socket_socket_gettimeout(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_gettimeout_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_socket_socket_getsockopt__doc__,
"getsockopt($self, level, option, buffersize=0, /)\n"
"--\n"
"\n"
"Get a socket option.\n"
"\n"
"See the Unix manual for level and option.  If a nonzero buffersize\n"
"argument is given, the return value is a bytes object of that\n"
"length; otherwise it is an integer.");

#define _SOCKET_SOCKET_GETSOCKOPT_METHODDEF    \
    {"getsockopt", _PyCFunction_CAST(_socket_socket_getsockopt), METH_VALUE, _socket_socket_getsockopt__doc__},

static PyValue
_socket_socket_getsockopt_impl(PySocketSockObject *s, int level, int optname,
                               int buflen);

static PyValue
_socket_socket_getsockopt(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int level;
    int optname;
    int buflen = 0;

    if (!_PyArg_CheckPositional("getsockopt", nargs, 2, 3)) {
        goto exit;
    }
    level = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
    optname = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (optname == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    buflen = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (buflen == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_getsockopt_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), level, optname, buflen);

exit:
    return return_value;
}

#if defined(HAVE_BIND)

PyDoc_STRVAR(_socket_socket_bind__doc__,
"bind($self, address, /)\n"
"--\n"
"\n"
"Bind the socket to a local address.\n"
"\n"
"For IP sockets, the address is a pair (host, port); the host must\n"
"refer to the local host.  For raw packet sockets the address is a\n"
"tuple (ifname, proto [,pkttype [,hatype [,addr]]]).");

#define _SOCKET_SOCKET_BIND_METHODDEF    \
    {"bind", _PyCFunction_CAST(_socket_socket_bind), METH_VALUE|METH_O, _socket_socket_bind__doc__},

static PyValue
_socket_socket_bind_impl(PySocketSockObject *s, PyValueBorrow addro);

static PyValue
_socket_socket_bind(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow addro;

    addro = PyValue_Borrow(args[0]);
    return_value = _socket_socket_bind_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), addro);

    return return_value;
}

#endif /* defined(HAVE_BIND) */

PyDoc_STRVAR(_socket_socket_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"close()\n"
"\n"
"Close the socket.  It cannot be used after this call.");

#define _SOCKET_SOCKET_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_socket_socket_close), METH_VALUE|METH_NOARGS, _socket_socket_close__doc__},

static PyValue
_socket_socket_close_impl(PySocketSockObject *s);

static PyValue
_socket_socket_close(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_close_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

PyDoc_STRVAR(_socket_socket_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n"
"Close the socket object without closing the file descriptor.\n"
"\n"
"The object cannot be used after this call, but the file descriptor\n"
"can be reused for other purposes.  The file descriptor is returned.");

#define _SOCKET_SOCKET_DETACH_METHODDEF    \
    {"detach", _PyCFunction_CAST(_socket_socket_detach), METH_VALUE|METH_NOARGS, _socket_socket_detach__doc__},

static PyValue
_socket_socket_detach_impl(PySocketSockObject *s);

static PyValue
_socket_socket_detach(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_detach_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

#if defined(HAVE_CONNECT)

PyDoc_STRVAR(_socket_socket_connect__doc__,
"connect($self, address, /)\n"
"--\n"
"\n"
"Connect the socket to a remote address.\n"
"\n"
"For IP sockets, the address is a pair (host, port).");

#define _SOCKET_SOCKET_CONNECT_METHODDEF    \
    {"connect", _PyCFunction_CAST(_socket_socket_connect), METH_VALUE|METH_O, _socket_socket_connect__doc__},

static PyValue
_socket_socket_connect_impl(PySocketSockObject *s, PyValueBorrow addro);

static PyValue
_socket_socket_connect(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow addro;

    addro = PyValue_Borrow(args[0]);
    return_value = _socket_socket_connect_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), addro);

    return return_value;
}

#endif /* defined(HAVE_CONNECT) */

#if defined(HAVE_CONNECT)

PyDoc_STRVAR(_socket_socket_connect_ex__doc__,
"connect_ex($self, address, /)\n"
"--\n"
"\n"
"Connect the socket to a remote address.\n"
"\n"
"This is like connect(address), but returns an error code (the errno\n"
"value) instead of raising an exception when an error occurs.");

#define _SOCKET_SOCKET_CONNECT_EX_METHODDEF    \
    {"connect_ex", _PyCFunction_CAST(_socket_socket_connect_ex), METH_VALUE|METH_O, _socket_socket_connect_ex__doc__},

static PyValue
_socket_socket_connect_ex_impl(PySocketSockObject *s, PyValueBorrow addro);

static PyValue
_socket_socket_connect_ex(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow addro;

    addro = PyValue_Borrow(args[0]);
    return_value = _socket_socket_connect_ex_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), addro);

    return return_value;
}

#endif /* defined(HAVE_CONNECT) */

PyDoc_STRVAR(_socket_socket_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the integer file descriptor of the socket.");

#define _SOCKET_SOCKET_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(_socket_socket_fileno), METH_VALUE|METH_NOARGS, _socket_socket_fileno__doc__},

static PyValue
_socket_socket_fileno_impl(PySocketSockObject *s);

static PyValue
_socket_socket_fileno(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_fileno_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

#if defined(HAVE_GETSOCKNAME)

PyDoc_STRVAR(_socket_socket_getsockname__doc__,
"getsockname($self, /)\n"
"--\n"
"\n"
"Return the address of the local endpoint.\n"
"\n"
"The format depends on the address family.  For IPv4 sockets, the\n"
"address info is a pair (hostaddr, port). For IPv6 sockets, the\n"
"address info is a 4-tuple (hostaddr, port, flowinfo, scope_id).");

#define _SOCKET_SOCKET_GETSOCKNAME_METHODDEF    \
    {"getsockname", _PyCFunction_CAST(_socket_socket_getsockname), METH_VALUE|METH_NOARGS, _socket_socket_getsockname__doc__},

static PyValue
_socket_socket_getsockname_impl(PySocketSockObject *s);

static PyValue
_socket_socket_getsockname(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_getsockname_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

#endif /* defined(HAVE_GETSOCKNAME) */

#if defined(HAVE_GETPEERNAME)

PyDoc_STRVAR(_socket_socket_getpeername__doc__,
"getpeername($self, /)\n"
"--\n"
"\n"
"Return the address of the remote endpoint.\n"
"\n"
"For IP sockets, the address info is a pair (hostaddr, port).");

#define _SOCKET_SOCKET_GETPEERNAME_METHODDEF    \
    {"getpeername", _PyCFunction_CAST(_socket_socket_getpeername), METH_VALUE|METH_NOARGS, _socket_socket_getpeername__doc__},

static PyValue
_socket_socket_getpeername_impl(PySocketSockObject *s);

static PyValue
_socket_socket_getpeername(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_socket_getpeername_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s));

    return return_value;
}

#endif /* defined(HAVE_GETPEERNAME) */

#if defined(HAVE_LISTEN)

PyDoc_STRVAR(_socket_socket_listen__doc__,
"listen($self, backlog=min(SOMAXCONN, 128), /)\n"
"--\n"
"\n"
"Enable a server to accept connections.\n"
"\n"
"If backlog is specified, it must be at least 0 (if it is lower, it\n"
"is set to 0); it specifies the number of unaccepted connections that\n"
"the system will allow before refusing new connections. If not\n"
"specified, a default reasonable value is chosen.");

#define _SOCKET_SOCKET_LISTEN_METHODDEF    \
    {"listen", _PyCFunction_CAST(_socket_socket_listen), METH_VALUE, _socket_socket_listen__doc__},

static PyValue
_socket_socket_listen_impl(PySocketSockObject *s, int backlog);

static PyValue
_socket_socket_listen(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int backlog = Py_MIN(SOMAXCONN, 128);

    if (!_PyArg_CheckPositional("listen", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    backlog = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (backlog == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_listen_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), backlog);

exit:
    return return_value;
}

#endif /* defined(HAVE_LISTEN) */

PyDoc_STRVAR(_socket_socket_recv__doc__,
"recv($self, buffersize, flags=0, /)\n"
"--\n"
"\n"
"Receive up to buffersize bytes from the socket.\n"
"\n"
"For the optional flags argument, see the Unix manual. When no data\n"
"is available, block until at least one byte is available or until\n"
"the remote end is closed. When the remote end is closed and all data\n"
"is read, return the empty string.");

#define _SOCKET_SOCKET_RECV_METHODDEF    \
    {"recv", _PyCFunction_CAST(_socket_socket_recv), METH_VALUE, _socket_socket_recv__doc__},

static PyValue
_socket_socket_recv_impl(PySocketSockObject *s, Py_ssize_t recvlen,
                         int flags);

static PyValue
_socket_socket_recv(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t recvlen;
    int flags = 0;

    if (!_PyArg_CheckPositional("recv", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        recvlen = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recv_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), recvlen, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_socket_recv_into__doc__,
"recv_into($self, /, buffer, nbytes=0, flags=0)\n"
"--\n"
"\n"
"Receive up to nbytes bytes from the socket, storing into a buffer.\n"
"\n"
"A version of recv() that stores its data into a buffer rather than\n"
"creating a new bytes object.  If nbytes is not specified (or 0),\n"
"receive up to the size available in the given buffer.\n"
"\n"
"See recv() for documentation about the flags.");

#define _SOCKET_SOCKET_RECV_INTO_METHODDEF    \
    {"recv_into", _PyCFunction_CAST(_socket_socket_recv_into), METH_VALUE|METH_KEYWORDS, _socket_socket_recv_into__doc__},

static PyValue
_socket_socket_recv_into_impl(PySocketSockObject *s, Py_buffer *pbuf,
                              Py_ssize_t recvlen, int flags);

static PyValue
_socket_socket_recv_into(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer)), _PyValue_STATIC_IMMORTAL(&_Py_ID(nbytes)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"buffer", "nbytes", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "recv_into",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer pbuf = {NULL, NULL};
    Py_ssize_t recvlen = 0;
    int flags = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("recv_into", "argument 'buffer'", "read-write bytes-like object", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &pbuf, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("recv_into", "argument 'buffer'", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            recvlen = ival;
        }
    }
    if (_idx[2] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _socket_socket_recv_into_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), &pbuf, recvlen, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

#if defined(HAVE_RECVFROM)

PyDoc_STRVAR(_socket_socket_recvfrom__doc__,
"recvfrom($self, buffersize, flags=0, /)\n"
"--\n"
"\n"
"Receive up to buffersize bytes and the sender\'s address info.\n"
"\n"
"Like recv(buffersize, flags), but return a (data, address info)\n"
"pair.");

#define _SOCKET_SOCKET_RECVFROM_METHODDEF    \
    {"recvfrom", _PyCFunction_CAST(_socket_socket_recvfrom), METH_VALUE, _socket_socket_recvfrom__doc__},

static PyValue
_socket_socket_recvfrom_impl(PySocketSockObject *s, Py_ssize_t recvlen,
                             int flags);

static PyValue
_socket_socket_recvfrom(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t recvlen;
    int flags = 0;

    if (!_PyArg_CheckPositional("recvfrom", nargs, 1, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        recvlen = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recvfrom_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), recvlen, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_RECVFROM) */

#if defined(HAVE_RECVFROM)

PyDoc_STRVAR(_socket_socket_recvfrom_into__doc__,
"recvfrom_into($self, /, buffer, nbytes=0, flags=0)\n"
"--\n"
"\n"
"Receive up to nbytes bytes and the sender\'s address info.\n"
"\n"
"Like recv_into(buffer, nbytes, flags), but return a (nbytes, address\n"
"info) pair.");

#define _SOCKET_SOCKET_RECVFROM_INTO_METHODDEF    \
    {"recvfrom_into", _PyCFunction_CAST(_socket_socket_recvfrom_into), METH_VALUE|METH_KEYWORDS, _socket_socket_recvfrom_into__doc__},

static PyValue
_socket_socket_recvfrom_into_impl(PySocketSockObject *s, Py_buffer *pbuf,
                                  Py_ssize_t recvlen, int flags);

static PyValue
_socket_socket_recvfrom_into(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer)), _PyValue_STATIC_IMMORTAL(&_Py_ID(nbytes)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"buffer", "nbytes", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "recvfrom_into",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer pbuf = {NULL, NULL};
    Py_ssize_t recvlen = 0;
    int flags = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("recvfrom_into", "argument 'buffer'", "read-write bytes-like object", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &pbuf, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("recvfrom_into", "argument 'buffer'", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    if (_idx[1] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            recvlen = ival;
        }
    }
    if (_idx[2] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _socket_socket_recvfrom_into_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), &pbuf, recvlen, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

#endif /* defined(HAVE_RECVFROM) */

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_socket_recvmsg__doc__,
"recvmsg($self, bufsize, ancbufsize=0, flags=0, /)\n"
"--\n"
"\n"
"Receive normal data and ancillary data from the socket.\n"
"\n"
"Receive up to bufsize bytes of normal data.  The ancbufsize argument\n"
"sets the size in bytes of the internal buffer used to receive the\n"
"ancillary data; it defaults to 0, meaning that no ancillary data\n"
"will be received.  Appropriate buffer sizes for ancillary data can\n"
"be calculated using CMSG_SPACE() or CMSG_LEN(), and items which do\n"
"not fit into the buffer might be truncated or discarded.  The flags\n"
"argument defaults to 0 and has the same meaning as for recv().\n"
"\n"
"The return value is a 4-tuple: (data, ancdata, msg_flags, address).\n"
"The data item is a bytes object holding the non-ancillary data\n"
"received.  The ancdata item is a list of zero or more tuples\n"
"(cmsg_level, cmsg_type, cmsg_data) representing the ancillary data\n"
"(control messages) received: cmsg_level and cmsg_type are integers\n"
"specifying the protocol level and protocol-specific type\n"
"respectively, and cmsg_data is a bytes object holding the associated\n"
"data.  The msg_flags item is the bitwise OR of various flags\n"
"indicating conditions on the received message; see your system\n"
"documentation for details.  If the receiving socket is unconnected,\n"
"address is the address of the sending socket, if available;\n"
"otherwise, its value is unspecified.\n"
"\n"
"If recvmsg() raises an exception after the system call returns, it\n"
"will first attempt to close any file descriptors received via the\n"
"SCM_RIGHTS mechanism.");

#define _SOCKET_SOCKET_RECVMSG_METHODDEF    \
    {"recvmsg", _PyCFunction_CAST(_socket_socket_recvmsg), METH_VALUE, _socket_socket_recvmsg__doc__},

static PyValue
_socket_socket_recvmsg_impl(PySocketSockObject *s, Py_ssize_t bufsize,
                            Py_ssize_t ancbufsize, int flags);

static PyValue
_socket_socket_recvmsg(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t bufsize;
    Py_ssize_t ancbufsize = 0;
    int flags = 0;

    if (!_PyArg_CheckPositional("recvmsg", nargs, 1, 3)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        bufsize = ival;
    }
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
        ancbufsize = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recvmsg_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), bufsize, ancbufsize, flags);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_socket_recvmsg_into__doc__,
"recvmsg_into($self, buffers, ancbufsize=0, flags=0, /)\n"
"--\n"
"\n"
"Receive normal and ancillary data, scattering the normal data.\n"
"\n"
"The buffers argument must be an iterable of objects that export\n"
"writable buffers (e.g.  bytearray objects); these will be filled\n"
"with successive chunks of the non-ancillary data until it has all\n"
"been written or there are no more buffers.  The ancbufsize argument\n"
"sets the size in bytes of the internal buffer used to receive the\n"
"ancillary data; it defaults to 0, meaning that no ancillary data\n"
"will be received.  Appropriate buffer sizes for ancillary data can\n"
"be calculated using CMSG_SPACE() or CMSG_LEN(), and items which do\n"
"not fit into the buffer might be truncated or discarded.  The flags\n"
"argument defaults to 0 and has the same meaning as for recv().\n"
"\n"
"The return value is a 4-tuple: (nbytes, ancdata, msg_flags,\n"
"address). The nbytes item is the total number of bytes of\n"
"non-ancillary data written into the buffers.  The ancdata item is a\n"
"list of zero or more tuples (cmsg_level, cmsg_type, cmsg_data)\n"
"representing the ancillary data (control messages) received:\n"
"cmsg_level and cmsg_type are integers specifying the protocol level\n"
"and protocol-specific type respectively, and cmsg_data is a bytes\n"
"object holding the associated data.  The msg_flags item is the\n"
"bitwise OR of various flags indicating conditions on the received\n"
"message; see your system documentation for details.  If the\n"
"receiving socket is unconnected, address is the address of the\n"
"sending socket, if available; otherwise, its value is unspecified.\n"
"\n"
"If recvmsg_into() raises an exception after the system call returns,\n"
"it will first attempt to close any file descriptors received via the\n"
"SCM_RIGHTS mechanism.");

#define _SOCKET_SOCKET_RECVMSG_INTO_METHODDEF    \
    {"recvmsg_into", _PyCFunction_CAST(_socket_socket_recvmsg_into), METH_VALUE, _socket_socket_recvmsg_into__doc__},

static PyValue
_socket_socket_recvmsg_into_impl(PySocketSockObject *s,
                                 PyValueBorrow buffers_arg,
                                 Py_ssize_t ancbufsize, int flags);

static PyValue
_socket_socket_recvmsg_into(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow buffers_arg;
    Py_ssize_t ancbufsize = 0;
    int flags = 0;

    if (!_PyArg_CheckPositional("recvmsg_into", nargs, 1, 3)) {
        goto exit;
    }
    buffers_arg = PyValue_Borrow(args[0]);
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
        ancbufsize = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_recvmsg_into_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), buffers_arg, ancbufsize, flags);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

PyDoc_STRVAR(_socket_socket_send__doc__,
"send($self, data, flags=0, /)\n"
"--\n"
"\n"
"Send a data string to the socket.\n"
"\n"
"For the optional flags argument, see the Unix manual.\n"
"Return the number of bytes sent; this may be less than len(data) if\n"
"the network is busy.");

#define _SOCKET_SOCKET_SEND_METHODDEF    \
    {"send", _PyCFunction_CAST(_socket_socket_send), METH_VALUE, _socket_socket_send__doc__},

static PyValue
_socket_socket_send_impl(PySocketSockObject *s, Py_buffer *pbuf, int flags);

static PyValue
_socket_socket_send(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer pbuf = {NULL, NULL};
    int flags = 0;

    if (!_PyArg_CheckPositional("send", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &pbuf, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_send_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), &pbuf, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

PyDoc_STRVAR(_socket_socket_sendall__doc__,
"sendall($self, data, flags=0, /)\n"
"--\n"
"\n"
"Send a data string to the socket.\n"
"\n"
"For the optional flags argument, see the Unix manual.\n"
"This calls send() repeatedly until all data is sent.\n"
"If an error occurs, it\'s impossible to tell how much data has been\n"
"sent.");

#define _SOCKET_SOCKET_SENDALL_METHODDEF    \
    {"sendall", _PyCFunction_CAST(_socket_socket_sendall), METH_VALUE, _socket_socket_sendall__doc__},

static PyValue
_socket_socket_sendall_impl(PySocketSockObject *s, Py_buffer *pbuf,
                            int flags);

static PyValue
_socket_socket_sendall(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer pbuf = {NULL, NULL};
    int flags = 0;

    if (!_PyArg_CheckPositional("sendall", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &pbuf, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socket_sendall_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), &pbuf, flags);

exit:
    /* Cleanup for pbuf */
    if (pbuf.obj) {
       PyBuffer_Release(&pbuf);
    }

    return return_value;
}

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_socket_sendmsg__doc__,
"sendmsg($self, buffers, ancdata=(), flags=0, address=None, /)\n"
"--\n"
"\n"
"Send normal and ancillary data to the socket.\n"
"\n"
"It gathering the non-ancillary data from a series of buffers\n"
"and concatenating it into a single message.\n"
"The buffers argument specifies the non-ancillary\n"
"data as an iterable of bytes-like objects (e.g. bytes objects).\n"
"The ancdata argument specifies the ancillary data (control messages)\n"
"as an iterable of zero or more tuples (cmsg_level, cmsg_type,\n"
"cmsg_data), where cmsg_level and cmsg_type are integers specifying\n"
"the protocol level and protocol-specific type respectively, and\n"
"cmsg_data is a bytes-like object holding the associated data.  The\n"
"flags argument defaults to 0 and has the same meaning as for send().\n"
"If address is supplied and not None, it sets a destination address\n"
"for the message.  The return value is the number of bytes of\n"
"non-ancillary data sent.");

#define _SOCKET_SOCKET_SENDMSG_METHODDEF    \
    {"sendmsg", _PyCFunction_CAST(_socket_socket_sendmsg), METH_VALUE, _socket_socket_sendmsg__doc__},

static PyValue
_socket_socket_sendmsg_impl(PySocketSockObject *s, PyValueBorrow data_arg,
                            PyValueBorrow cmsg_arg, int flags,
                            PyValueBorrow addr_arg);

static PyValue
_socket_socket_sendmsg(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow data_arg;
    PyValueBorrow cmsg_arg = PyValue_Borrow(PyValue_NULL);
    int flags = 0;
    PyValueBorrow addr_arg = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("sendmsg", nargs, 1, 4)) {
        goto exit;
    }
    data_arg = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    cmsg_arg = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    addr_arg = PyValue_Borrow(args[3]);
skip_optional:
    return_value = _socket_socket_sendmsg_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), data_arg, cmsg_arg, flags, addr_arg);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

#if defined(HAVE_SOCKADDR_ALG)

PyDoc_STRVAR(_socket_socket_sendmsg_afalg__doc__,
"sendmsg_afalg($self, /, msg=<unrepresentable>, *, op=<unrepresentable>,\n"
"              iv=<unrepresentable>, assoclen=<unrepresentable>, flags=0)\n"
"--\n"
"\n"
"Set operation mode, IV and associated data length.\n"
"\n"
"For an AF_ALG operation socket.");

#define _SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF    \
    {"sendmsg_afalg", _PyCFunction_CAST(_socket_socket_sendmsg_afalg), METH_VALUE|METH_KEYWORDS, _socket_socket_sendmsg_afalg__doc__},

static PyValue
_socket_socket_sendmsg_afalg_impl(PySocketSockObject *self,
                                  PyValueBorrow data_arg,
                                  PyValueBorrow opobj, Py_buffer *iv,
                                  PyValueBorrow assoclenobj, int flags);

static PyValue
_socket_socket_sendmsg_afalg(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(msg)), _PyValue_STATIC_IMMORTAL(&_Py_ID(op)), _PyValue_STATIC_IMMORTAL(&_Py_ID(iv)), _PyValue_STATIC_IMMORTAL(&_Py_ID(assoclen)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"msg", "op", "iv", "assoclen", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sendmsg_afalg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow data_arg = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow opobj = PyValue_Borrow(PyValue_NULL);
    Py_buffer iv = {NULL, NULL};
    PyValueBorrow assoclenobj = PyValue_Borrow(PyValue_NULL);
    int flags = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data_arg = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), &PyLong_Type)) {
            _PyArg_BadArgumentValue("sendmsg_afalg", "argument 'op'", "int", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        opobj = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[2]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])), &iv, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[3]])), &PyLong_Type)) {
            _PyArg_BadArgumentValue("sendmsg_afalg", "argument 'assoclen'", "int", PyValue_Borrow(args[_idx[3]]));
            goto exit;
        }
        assoclenobj = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _socket_socket_sendmsg_afalg_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(self), data_arg, opobj, &iv, assoclenobj, flags);

exit:
    /* Cleanup for iv */
    if (iv.obj) {
       PyBuffer_Release(&iv);
    }

    return return_value;
}

#endif /* defined(HAVE_SOCKADDR_ALG) */

#if defined(HAVE_SHUTDOWN)

PyDoc_STRVAR(_socket_socket_shutdown__doc__,
"shutdown($self, flag, /)\n"
"--\n"
"\n"
"Shut down one or both halves of the connection.\n"
"\n"
"Shut down the reading side of the socket (flag == SHUT_RD), the\n"
"writing side of the socket (flag == SHUT_WR), or both ends (flag ==\n"
"SHUT_RDWR).");

#define _SOCKET_SOCKET_SHUTDOWN_METHODDEF    \
    {"shutdown", _PyCFunction_CAST(_socket_socket_shutdown), METH_VALUE|METH_O, _socket_socket_shutdown__doc__},

static PyValue
_socket_socket_shutdown_impl(PySocketSockObject *s, PyValueBorrow arg);

static PyValue
_socket_socket_shutdown(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _socket_socket_shutdown_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), arg);

    return return_value;
}

#endif /* defined(HAVE_SHUTDOWN) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_socket_socket_share__doc__,
"share($self, process_id, /)\n"
"--\n"
"\n"
"Share the socket with another process.\n"
"\n"
"The target process id must be provided and the resulting bytes\n"
"object passed to the target process.  There the shared socket can be\n"
"instantiated by calling socket.fromshare().");

#define _SOCKET_SOCKET_SHARE_METHODDEF    \
    {"share", _PyCFunction_CAST(_socket_socket_share), METH_VALUE|METH_O, _socket_socket_share__doc__},

static PyValue
_socket_socket_share_impl(PySocketSockObject *s, unsigned int processId);

static PyValue
_socket_socket_share(PyValueBorrow s, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned int processId;

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &processId, sizeof(unsigned int),
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
    return_value = _socket_socket_share_impl((PySocketSockObject *)PyValue_AsKnownObjectBorrow(s), processId);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

static int
sock_initobj_impl(PySocketSockObject *self, int family, int type, int proto,
                  PyValueBorrow fdobj);

static int
sock_initobj_value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(family)), _PyValue_STATIC_IMMORTAL(&_Py_ID(type)), _PyValue_STATIC_IMMORTAL(&_Py_ID(proto)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fileno)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"family", "type", "proto", "fileno", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "socket",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    int family = -1;
    int type = -1;
    int proto = -1;
    PyValueBorrow fdobj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        family = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (family == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        type = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (type == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        proto = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (proto == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        fdobj = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = sock_initobj_impl((PySocketSockObject *)self, family, type, proto, fdobj);

exit:
    return return_value;
}

static int
sock_initobj(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return sock_initobj_value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = sock_initobj_value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

#if defined(HAVE_GETHOSTNAME)

PyDoc_STRVAR(_socket_gethostname__doc__,
"gethostname($module, /)\n"
"--\n"
"\n"
"Return the current host name.");

#define _SOCKET_GETHOSTNAME_METHODDEF    \
    {"gethostname", _PyCFunction_CAST(_socket_gethostname), METH_VALUE|METH_NOARGS, _socket_gethostname__doc__},

static PyValue
_socket_gethostname_impl(PyObject *module);

static PyValue
_socket_gethostname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_gethostname_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_GETHOSTNAME) */

#if defined(HAVE_SETHOSTNAME)

PyDoc_STRVAR(_socket_sethostname__doc__,
"sethostname($module, name, /)\n"
"--\n"
"\n"
"Set the hostname to name.");

#define _SOCKET_SETHOSTNAME_METHODDEF    \
    {"sethostname", _PyCFunction_CAST(_socket_sethostname), METH_VALUE|METH_O, _socket_sethostname__doc__},

static PyValue
_socket_sethostname_impl(PyObject *module, PyValueBorrow hnobj);

static PyValue
_socket_sethostname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow hnobj;

    hnobj = PyValue_Borrow(args[0]);
    return_value = _socket_sethostname_impl(PyValue_AsKnownObjectBorrow(module), hnobj);

    return return_value;
}

#endif /* defined(HAVE_SETHOSTNAME) */

#if defined(HAVE_GETADDRINFO)

PyDoc_STRVAR(_socket_gethostbyname__doc__,
"gethostbyname($module, host, /)\n"
"--\n"
"\n"
"Return the IP address for a host.\n"
"\n"
"The address is a string of the form \'255.255.255.255\'.");

#define _SOCKET_GETHOSTBYNAME_METHODDEF    \
    {"gethostbyname", _PyCFunction_CAST(_socket_gethostbyname), METH_VALUE|METH_O, _socket_gethostbyname__doc__},

static PyValue
_socket_gethostbyname_impl(PyObject *module, PyValueBorrow hobj);

static PyValue
_socket_gethostbyname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow hobj;

    hobj = PyValue_Borrow(args[0]);
    return_value = _socket_gethostbyname_impl(PyValue_AsKnownObjectBorrow(module), hobj);

    return return_value;
}

#endif /* defined(HAVE_GETADDRINFO) */

#if (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME))

PyDoc_STRVAR(_socket_gethostbyname_ex__doc__,
"gethostbyname_ex($module, host, /)\n"
"--\n"
"\n"
"Map a host name to its IP number.\n"
"\n"
"Return the true host name, a list of aliases, and a list of IP\n"
"addresses, which are strings of the form \'255.255.255.255\'.");

#define _SOCKET_GETHOSTBYNAME_EX_METHODDEF    \
    {"gethostbyname_ex", _PyCFunction_CAST(_socket_gethostbyname_ex), METH_VALUE|METH_O, _socket_gethostbyname_ex__doc__},

static PyValue
_socket_gethostbyname_ex_impl(PyObject *module, PyValueBorrow hobj);

static PyValue
_socket_gethostbyname_ex(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow hobj;

    hobj = PyValue_Borrow(args[0]);
    return_value = _socket_gethostbyname_ex_impl(PyValue_AsKnownObjectBorrow(module), hobj);

    return return_value;
}

#endif /* (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME)) */

#if (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYADDR))

PyDoc_STRVAR(_socket_gethostbyaddr__doc__,
"gethostbyaddr($module, host, /)\n"
"--\n"
"\n"
"Map an IP number or host name to the true host name and addresses.\n"
"\n"
"Return the true host name, a list of aliases, and a list of IP\n"
"addresses.  The host argument is a string giving a host name or IP\n"
"number.");

#define _SOCKET_GETHOSTBYADDR_METHODDEF    \
    {"gethostbyaddr", _PyCFunction_CAST(_socket_gethostbyaddr), METH_VALUE|METH_O, _socket_gethostbyaddr__doc__},

static PyValue
_socket_gethostbyaddr_impl(PyObject *module, PyValueBorrow hobj);

static PyValue
_socket_gethostbyaddr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow hobj;

    hobj = PyValue_Borrow(args[0]);
    return_value = _socket_gethostbyaddr_impl(PyValue_AsKnownObjectBorrow(module), hobj);

    return return_value;
}

#endif /* (defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYADDR)) */

#if defined(HAVE_GETSERVBYNAME)

PyDoc_STRVAR(_socket_getservbyname__doc__,
"getservbyname($module, servicename, protocolname=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return a port number from a service name and protocol name.\n"
"\n"
"The optional protocol name, if given, should be \'tcp\' or \'udp\',\n"
"otherwise any protocol will match.");

#define _SOCKET_GETSERVBYNAME_METHODDEF    \
    {"getservbyname", _PyCFunction_CAST(_socket_getservbyname), METH_VALUE, _socket_getservbyname__doc__},

static PyValue
_socket_getservbyname_impl(PyObject *module, const char *name,
                           const char *proto);

static PyValue
_socket_getservbyname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    const char *proto = NULL;

    if (!_PyArg_CheckPositional("getservbyname", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("getservbyname", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("getservbyname", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("getservbyname", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("getservbyname", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    proto = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (proto == NULL) {
        goto exit;
    }
skip_optional:
    return_value = _socket_getservbyname_impl(PyValue_AsKnownObjectBorrow(module), name, proto);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSERVBYNAME) */

#if defined(HAVE_GETSERVBYPORT)

PyDoc_STRVAR(_socket_getservbyport__doc__,
"getservbyport($module, port, protocolname=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return the service name from a port number and protocol name.\n"
"\n"
"The optional protocol name, if given, should be \'tcp\' or \'udp\',\n"
"otherwise any protocol will match.");

#define _SOCKET_GETSERVBYPORT_METHODDEF    \
    {"getservbyport", _PyCFunction_CAST(_socket_getservbyport), METH_VALUE, _socket_getservbyport__doc__},

static PyValue
_socket_getservbyport_impl(PyObject *module, int port, const char *proto);

static PyValue
_socket_getservbyport(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int port;
    const char *proto = NULL;

    if (!_PyArg_CheckPositional("getservbyport", nargs, 1, 2)) {
        goto exit;
    }
    port = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (port == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("getservbyport", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("getservbyport", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    proto = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (proto == NULL) {
        goto exit;
    }
skip_optional:
    return_value = _socket_getservbyport_impl(PyValue_AsKnownObjectBorrow(module), port, proto);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSERVBYPORT) */

#if defined(HAVE_GETPROTOBYNAME)

PyDoc_STRVAR(_socket_getprotobyname__doc__,
"getprotobyname($module, name, /)\n"
"--\n"
"\n"
"Return the protocol number for the named protocol.");

#define _SOCKET_GETPROTOBYNAME_METHODDEF    \
    {"getprotobyname", _PyCFunction_CAST(_socket_getprotobyname), METH_VALUE|METH_O, _socket_getprotobyname__doc__},

static PyValue
_socket_getprotobyname_impl(PyObject *module, const char *name);

static PyValue
_socket_getprotobyname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("getprotobyname", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("getprotobyname", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    return_value = _socket_getprotobyname_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETPROTOBYNAME) */

PyDoc_STRVAR(_socket_close__doc__,
"close($module, fd, /)\n"
"--\n"
"\n"
"Close a socket fd.\n"
"\n"
"This is like os.close(), but for sockets; on some platforms os.close()\n"
"won\'t work for socket file descriptors.");

#define _SOCKET_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_socket_close), METH_VALUE|METH_O, _socket_close__doc__},

static PyValue
_socket_close_impl(PyObject *module, PyValueBorrow fdobj);

static PyValue
_socket_close(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow fdobj;

    fdobj = PyValue_Borrow(args[0]);
    return_value = _socket_close_impl(PyValue_AsKnownObjectBorrow(module), fdobj);

    return return_value;
}

#if !defined(NO_DUP)

PyDoc_STRVAR(_socket_dup__doc__,
"dup($module, fd, /)\n"
"--\n"
"\n"
"Duplicate a socket descriptor.\n"
"\n"
"The new descriptor is non-inheritable.  This is like os.dup(), but for\n"
"sockets; on some platforms os.dup() won\'t work for socket file\n"
"descriptors.");

#define _SOCKET_DUP_METHODDEF    \
    {"dup", _PyCFunction_CAST(_socket_dup), METH_VALUE|METH_O, _socket_dup__doc__},

static PyValue
_socket_dup_impl(PyObject *module, PyValueBorrow fdobj);

static PyValue
_socket_dup(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow fdobj;

    fdobj = PyValue_Borrow(args[0]);
    return_value = _socket_dup_impl(PyValue_AsKnownObjectBorrow(module), fdobj);

    return return_value;
}

#endif /* !defined(NO_DUP) */

#if defined(HAVE_SOCKETPAIR)

PyDoc_STRVAR(_socket_socketpair__doc__,
"socketpair($module, family=AF_UNIX, type=SOCK_STREAM, proto=0, /)\n"
"--\n"
"\n"
"Create a pair of connected socket objects.\n"
"\n"
"The sockets are returned by the platform socketpair() function.  The\n"
"arguments are the same as for socket(), except that the default\n"
"family is AF_UNIX if defined on the platform; otherwise, the default\n"
"is AF_INET.");

#define _SOCKET_SOCKETPAIR_METHODDEF    \
    {"socketpair", _PyCFunction_CAST(_socket_socketpair), METH_VALUE, _socket_socketpair__doc__},

static PyValue
_socket_socketpair_impl(PyObject *module, int family, int type, int proto);

static PyValue
_socket_socketpair(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int family = SOCKETPAIR_DEFAULT_FAMILY;
    int type = SOCK_STREAM;
    int proto = 0;

    if (!_PyArg_CheckPositional("socketpair", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    family = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (family == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    type = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (type == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    proto = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _socket_socketpair_impl(PyValue_AsKnownObjectBorrow(module), family, type, proto);

exit:
    return return_value;
}

#endif /* defined(HAVE_SOCKETPAIR) */

PyDoc_STRVAR(_socket_ntohs__doc__,
"ntohs($module, integer, /)\n"
"--\n"
"\n"
"Convert a 16-bit unsigned integer from network to host byte order.");

#define _SOCKET_NTOHS_METHODDEF    \
    {"ntohs", _PyCFunction_CAST(_socket_ntohs), METH_VALUE|METH_O, _socket_ntohs__doc__},

static PyValue
_socket_ntohs_impl(PyObject *module, uint16_t x);

static PyValue
_socket_ntohs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    uint16_t x;

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &x, sizeof(uint16_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(uint16_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C uint16_t");
            goto exit;
        }
    }
    return_value = _socket_ntohs_impl(PyValue_AsKnownObjectBorrow(module), x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_ntohl__doc__,
"ntohl($module, integer, /)\n"
"--\n"
"\n"
"Convert a 32-bit unsigned integer from network to host byte order.");

#define _SOCKET_NTOHL_METHODDEF    \
    {"ntohl", _PyCFunction_CAST(_socket_ntohl), METH_VALUE|METH_O, _socket_ntohl__doc__},

static PyValue
_socket_ntohl_impl(PyObject *module, uint32_t x);

static PyValue
_socket_ntohl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    uint32_t x;

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &x, sizeof(uint32_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(uint32_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C uint32_t");
            goto exit;
        }
    }
    return_value = _socket_ntohl_impl(PyValue_AsKnownObjectBorrow(module), x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_htons__doc__,
"htons($module, integer, /)\n"
"--\n"
"\n"
"Convert a 16-bit unsigned integer from host to network byte order.");

#define _SOCKET_HTONS_METHODDEF    \
    {"htons", _PyCFunction_CAST(_socket_htons), METH_VALUE|METH_O, _socket_htons__doc__},

static PyValue
_socket_htons_impl(PyObject *module, uint16_t x);

static PyValue
_socket_htons(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    uint16_t x;

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &x, sizeof(uint16_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(uint16_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C uint16_t");
            goto exit;
        }
    }
    return_value = _socket_htons_impl(PyValue_AsKnownObjectBorrow(module), x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_htonl__doc__,
"htonl($module, integer, /)\n"
"--\n"
"\n"
"Convert a 32-bit unsigned integer from host to network byte order.");

#define _SOCKET_HTONL_METHODDEF    \
    {"htonl", _PyCFunction_CAST(_socket_htonl), METH_VALUE|METH_O, _socket_htonl__doc__},

static PyValue
_socket_htonl_impl(PyObject *module, uint32_t x);

static PyValue
_socket_htonl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    uint32_t x;

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &x, sizeof(uint32_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(uint32_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C uint32_t");
            goto exit;
        }
    }
    return_value = _socket_htonl_impl(PyValue_AsKnownObjectBorrow(module), x);

exit:
    return return_value;
}

PyDoc_STRVAR(_socket_inet_aton__doc__,
"inet_aton($module, ip_addr, /)\n"
"--\n"
"\n"
"Convert an IP address in string format (123.45.67.89) to the 32-bit packed binary format used in low-level network functions.");

#define _SOCKET_INET_ATON_METHODDEF    \
    {"inet_aton", _PyCFunction_CAST(_socket_inet_aton), METH_VALUE|METH_O, _socket_inet_aton__doc__},

static PyValue
_socket_inet_aton_impl(PyObject *module, const char *ip_addr);

static PyValue
_socket_inet_aton(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *ip_addr;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("inet_aton", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("inet_aton", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    ip_addr = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (ip_addr == NULL) {
        goto exit;
    }
    return_value = _socket_inet_aton_impl(PyValue_AsKnownObjectBorrow(module), ip_addr);

exit:
    return return_value;
}

#if defined(HAVE_INET_NTOA)

PyDoc_STRVAR(_socket_inet_ntoa__doc__,
"inet_ntoa($module, packed_ip, /)\n"
"--\n"
"\n"
"Convert an IP address from 32-bit packed binary format to string format.");

#define _SOCKET_INET_NTOA_METHODDEF    \
    {"inet_ntoa", _PyCFunction_CAST(_socket_inet_ntoa), METH_VALUE|METH_O, _socket_inet_ntoa__doc__},

static PyValue
_socket_inet_ntoa_impl(PyObject *module, Py_buffer *packed_ip);

static PyValue
_socket_inet_ntoa(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer packed_ip = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &packed_ip, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _socket_inet_ntoa_impl(PyValue_AsKnownObjectBorrow(module), &packed_ip);

exit:
    /* Cleanup for packed_ip */
    if (packed_ip.obj) {
       PyBuffer_Release(&packed_ip);
    }

    return return_value;
}

#endif /* defined(HAVE_INET_NTOA) */

#if defined(HAVE_INET_PTON)

PyDoc_STRVAR(_socket_inet_pton__doc__,
"inet_pton($module, af, ip, /)\n"
"--\n"
"\n"
"Convert an IP address from string format to a packed string.\n"
"\n"
"The string is suitable for use with low-level network functions.");

#define _SOCKET_INET_PTON_METHODDEF    \
    {"inet_pton", _PyCFunction_CAST(_socket_inet_pton), METH_VALUE, _socket_inet_pton__doc__},

static PyValue
_socket_inet_pton_impl(PyObject *module, int af, const char *ip);

static PyValue
_socket_inet_pton(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int af;
    const char *ip;

    if (!_PyArg_CheckPositional("inet_pton", nargs, 2, 2)) {
        goto exit;
    }
    af = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (af == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("inet_pton", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("inet_pton", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    ip = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (ip == NULL) {
        goto exit;
    }
    return_value = _socket_inet_pton_impl(PyValue_AsKnownObjectBorrow(module), af, ip);

exit:
    return return_value;
}

#endif /* defined(HAVE_INET_PTON) */

#if defined(HAVE_INET_PTON)

PyDoc_STRVAR(_socket_inet_ntop__doc__,
"inet_ntop($module, af, packed_ip, /)\n"
"--\n"
"\n"
"Convert a packed IP address of the given family to string format.");

#define _SOCKET_INET_NTOP_METHODDEF    \
    {"inet_ntop", _PyCFunction_CAST(_socket_inet_ntop), METH_VALUE, _socket_inet_ntop__doc__},

static PyValue
_socket_inet_ntop_impl(PyObject *module, int af, Py_buffer *packed_ip);

static PyValue
_socket_inet_ntop(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int af;
    Py_buffer packed_ip = {NULL, NULL};

    if (!_PyArg_CheckPositional("inet_ntop", nargs, 2, 2)) {
        goto exit;
    }
    af = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (af == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &packed_ip, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _socket_inet_ntop_impl(PyValue_AsKnownObjectBorrow(module), af, &packed_ip);

exit:
    /* Cleanup for packed_ip */
    if (packed_ip.obj) {
       PyBuffer_Release(&packed_ip);
    }

    return return_value;
}

#endif /* defined(HAVE_INET_PTON) */

#if defined(HAVE_GETADDRINFO)

PyDoc_STRVAR(_socket_getaddrinfo__doc__,
"getaddrinfo($module, /, host, port, family=AF_UNSPEC, type=0, proto=0,\n"
"            flags=0)\n"
"--\n"
"\n"
"Resolve host and port into a list of 5-tuples.\n"
"\n"
"Each tuple is (family, type, proto, canonname, sockaddr).");

#define _SOCKET_GETADDRINFO_METHODDEF    \
    {"getaddrinfo", _PyCFunction_CAST(_socket_getaddrinfo), METH_VALUE|METH_KEYWORDS, _socket_getaddrinfo__doc__},

static PyValue
_socket_getaddrinfo_impl(PyObject *module, PyValueBorrow hobj,
                         PyValueBorrow pobj, int family, int socktype,
                         int protocol, int flags);

static PyValue
_socket_getaddrinfo(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(host)), _PyValue_STATIC_IMMORTAL(&_Py_ID(port)), _PyValue_STATIC_IMMORTAL(&_Py_ID(family)), _PyValue_STATIC_IMMORTAL(&_Py_ID(type)), _PyValue_STATIC_IMMORTAL(&_Py_ID(proto)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"host", "port", "family", "type", "proto", "flags", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "getaddrinfo",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyValueBorrow hobj;
    PyValueBorrow pobj;
    int family = AF_UNSPEC;
    int socktype = 0;
    int protocol = 0;
    int flags = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    hobj = PyValue_Borrow(args[_idx[0]]);
    pobj = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        family = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (family == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        socktype = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (socktype == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        protocol = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (protocol == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _socket_getaddrinfo_impl(PyValue_AsKnownObjectBorrow(module), hobj, pobj, family, socktype, protocol, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETADDRINFO) */

#if defined(HAVE_GETNAMEINFO)

PyDoc_STRVAR(_socket_getnameinfo__doc__,
"getnameinfo($module, sockaddr, flags, /)\n"
"--\n"
"\n"
"Get host and port for a sockaddr.");

#define _SOCKET_GETNAMEINFO_METHODDEF    \
    {"getnameinfo", _PyCFunction_CAST(_socket_getnameinfo), METH_VALUE, _socket_getnameinfo__doc__},

static PyValue
_socket_getnameinfo_impl(PyObject *module, PyValueBorrow sa, int flags);

static PyValue
_socket_getnameinfo(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sa;
    int flags;

    if (!_PyArg_CheckPositional("getnameinfo", nargs, 2, 2)) {
        goto exit;
    }
    sa = PyValue_Borrow(args[0]);
    flags = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _socket_getnameinfo_impl(PyValue_AsKnownObjectBorrow(module), sa, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETNAMEINFO) */

PyDoc_STRVAR(_socket_getdefaulttimeout__doc__,
"getdefaulttimeout($module, /)\n"
"--\n"
"\n"
"Return the default timeout in seconds for new socket objects.\n"
"\n"
"A value of None indicates that new socket objects have no timeout.\n"
"When the socket module is first imported, the default is None.");

#define _SOCKET_GETDEFAULTTIMEOUT_METHODDEF    \
    {"getdefaulttimeout", _PyCFunction_CAST(_socket_getdefaulttimeout), METH_VALUE|METH_NOARGS, _socket_getdefaulttimeout__doc__},

static PyValue
_socket_getdefaulttimeout_impl(PyObject *module);

static PyValue
_socket_getdefaulttimeout(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_getdefaulttimeout_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_socket_setdefaulttimeout__doc__,
"setdefaulttimeout($module, timeout, /)\n"
"--\n"
"\n"
"Set the default timeout in seconds for new socket objects.\n"
"\n"
"A value of None indicates that new socket objects have no timeout.\n"
"When the socket module is first imported, the default is None.");

#define _SOCKET_SETDEFAULTTIMEOUT_METHODDEF    \
    {"setdefaulttimeout", _PyCFunction_CAST(_socket_setdefaulttimeout), METH_VALUE|METH_O, _socket_setdefaulttimeout__doc__},

static PyValue
_socket_setdefaulttimeout_impl(PyObject *module, PyValueBorrow arg);

static PyValue
_socket_setdefaulttimeout(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _socket_setdefaulttimeout_impl(PyValue_AsKnownObjectBorrow(module), arg);

    return return_value;
}

#if (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS_DESKTOP))

PyDoc_STRVAR(_socket_if_nameindex__doc__,
"if_nameindex($module, /)\n"
"--\n"
"\n"
"Return a list of network interface information (index, name) tuples.");

#define _SOCKET_IF_NAMEINDEX_METHODDEF    \
    {"if_nameindex", _PyCFunction_CAST(_socket_if_nameindex), METH_VALUE|METH_NOARGS, _socket_if_nameindex__doc__},

static PyValue
_socket_if_nameindex_impl(PyObject *module);

static PyValue
_socket_if_nameindex(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _socket_if_nameindex_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* (defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS_DESKTOP)) */

#if (defined(HAVE_IF_NAMETOINDEX) || defined(MS_WINDOWS_DESKTOP))

PyDoc_STRVAR(_socket_if_nametoindex__doc__,
"if_nametoindex($module, oname, /)\n"
"--\n"
"\n"
"Returns the interface index corresponding to the interface name if_name.");

#define _SOCKET_IF_NAMETOINDEX_METHODDEF    \
    {"if_nametoindex", _PyCFunction_CAST(_socket_if_nametoindex), METH_VALUE|METH_O, _socket_if_nametoindex__doc__},

static PyValue
_socket_if_nametoindex_impl(PyObject *module, PyObject *oname);

static PyValue
_socket_if_nametoindex(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *oname = NULL;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "expected str, bytes or os.PathLike object, not %.200s", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (!PyUnicode_FSConverter(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &oname)) {
        goto exit;
    }
    return_value = _socket_if_nametoindex_impl(PyValue_AsKnownObjectBorrow(module), oname);

exit:
    /* Cleanup for oname */
    Py_XDECREF(oname);

    return return_value;
}

#endif /* (defined(HAVE_IF_NAMETOINDEX) || defined(MS_WINDOWS_DESKTOP)) */

#if (defined(HAVE_IF_INDEXTONAME) || defined(MS_WINDOWS_DESKTOP))

PyDoc_STRVAR(_socket_if_indextoname__doc__,
"if_indextoname($module, if_index, /)\n"
"--\n"
"\n"
"Returns the interface name corresponding to the interface index if_index.");

#define _SOCKET_IF_INDEXTONAME_METHODDEF    \
    {"if_indextoname", _PyCFunction_CAST(_socket_if_indextoname), METH_VALUE|METH_O, _socket_if_indextoname__doc__},

static PyValue
_socket_if_indextoname_impl(PyObject *module, NET_IFINDEX index);

static PyValue
_socket_if_indextoname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    NET_IFINDEX index;

    if (!_PyLong_NetIfindex_Converter(PyValue_Borrow(args[0]), &index)) {
        goto exit;
    }
    return_value = _socket_if_indextoname_impl(PyValue_AsKnownObjectBorrow(module), index);

exit:
    return return_value;
}

#endif /* (defined(HAVE_IF_INDEXTONAME) || defined(MS_WINDOWS_DESKTOP)) */

#if defined(CMSG_LEN)

PyDoc_STRVAR(_socket_CMSG_LEN__doc__,
"CMSG_LEN($module, length, /)\n"
"--\n"
"\n"
"Return the total length of an ancillary data item with associated data.\n"
"\n"
"The associated data has the given length.  This value can often be\n"
"used as the buffer size for recvmsg() to receive a single item of\n"
"ancillary data, but RFC 3542 requires portable applications to use\n"
"CMSG_SPACE() and thus include space for padding, even when the item\n"
"will be the last in the buffer.  Raises OverflowError if length is\n"
"outside the permissible range of values.");

#define _SOCKET_CMSG_LEN_METHODDEF    \
    {"CMSG_LEN", _PyCFunction_CAST(_socket_CMSG_LEN), METH_VALUE|METH_O, _socket_CMSG_LEN__doc__},

static PyValue
_socket_CMSG_LEN_impl(PyObject *module, Py_ssize_t length);

static PyValue
_socket_CMSG_LEN(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t length;

    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = _socket_CMSG_LEN_impl(PyValue_AsKnownObjectBorrow(module), length);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) */

#if defined(CMSG_LEN) && defined(CMSG_SPACE)

PyDoc_STRVAR(_socket_CMSG_SPACE__doc__,
"CMSG_SPACE($module, length, /)\n"
"--\n"
"\n"
"Return the buffer size needed to receive an ancillary data item.\n"
"\n"
"The item has associated data of the given length, and the size\n"
"includes any trailing padding.  The buffer space needed to receive\n"
"multiple items is the sum of the CMSG_SPACE() values for their\n"
"associated data lengths.  Raises OverflowError if length is outside\n"
"the permissible range of values.");

#define _SOCKET_CMSG_SPACE_METHODDEF    \
    {"CMSG_SPACE", _PyCFunction_CAST(_socket_CMSG_SPACE), METH_VALUE|METH_O, _socket_CMSG_SPACE__doc__},

static PyValue
_socket_CMSG_SPACE_impl(PyObject *module, Py_ssize_t length);

static PyValue
_socket_CMSG_SPACE(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t length;

    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    return_value = _socket_CMSG_SPACE_impl(PyValue_AsKnownObjectBorrow(module), length);

exit:
    return return_value;
}

#endif /* defined(CMSG_LEN) && defined(CMSG_SPACE) */

#ifndef _SOCKET_SOCKET__ACCEPT_METHODDEF
    #define _SOCKET_SOCKET__ACCEPT_METHODDEF
#endif /* !defined(_SOCKET_SOCKET__ACCEPT_METHODDEF) */

#ifndef _SOCKET_SOCKET_BIND_METHODDEF
    #define _SOCKET_SOCKET_BIND_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_BIND_METHODDEF) */

#ifndef _SOCKET_SOCKET_CONNECT_METHODDEF
    #define _SOCKET_SOCKET_CONNECT_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_CONNECT_METHODDEF) */

#ifndef _SOCKET_SOCKET_CONNECT_EX_METHODDEF
    #define _SOCKET_SOCKET_CONNECT_EX_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_CONNECT_EX_METHODDEF) */

#ifndef _SOCKET_SOCKET_GETSOCKNAME_METHODDEF
    #define _SOCKET_SOCKET_GETSOCKNAME_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_GETSOCKNAME_METHODDEF) */

#ifndef _SOCKET_SOCKET_GETPEERNAME_METHODDEF
    #define _SOCKET_SOCKET_GETPEERNAME_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_GETPEERNAME_METHODDEF) */

#ifndef _SOCKET_SOCKET_LISTEN_METHODDEF
    #define _SOCKET_SOCKET_LISTEN_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_LISTEN_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVFROM_METHODDEF
    #define _SOCKET_SOCKET_RECVFROM_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVFROM_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVFROM_INTO_METHODDEF
    #define _SOCKET_SOCKET_RECVFROM_INTO_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVFROM_INTO_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVMSG_METHODDEF
    #define _SOCKET_SOCKET_RECVMSG_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVMSG_METHODDEF) */

#ifndef _SOCKET_SOCKET_RECVMSG_INTO_METHODDEF
    #define _SOCKET_SOCKET_RECVMSG_INTO_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_RECVMSG_INTO_METHODDEF) */

#ifndef _SOCKET_SOCKET_SENDMSG_METHODDEF
    #define _SOCKET_SOCKET_SENDMSG_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SENDMSG_METHODDEF) */

#ifndef _SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF
    #define _SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SENDMSG_AFALG_METHODDEF) */

#ifndef _SOCKET_SOCKET_SHUTDOWN_METHODDEF
    #define _SOCKET_SOCKET_SHUTDOWN_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SHUTDOWN_METHODDEF) */

#ifndef _SOCKET_SOCKET_SHARE_METHODDEF
    #define _SOCKET_SOCKET_SHARE_METHODDEF
#endif /* !defined(_SOCKET_SOCKET_SHARE_METHODDEF) */

#ifndef _SOCKET_GETHOSTNAME_METHODDEF
    #define _SOCKET_GETHOSTNAME_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTNAME_METHODDEF) */

#ifndef _SOCKET_SETHOSTNAME_METHODDEF
    #define _SOCKET_SETHOSTNAME_METHODDEF
#endif /* !defined(_SOCKET_SETHOSTNAME_METHODDEF) */

#ifndef _SOCKET_GETHOSTBYNAME_METHODDEF
    #define _SOCKET_GETHOSTBYNAME_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTBYNAME_METHODDEF) */

#ifndef _SOCKET_GETHOSTBYNAME_EX_METHODDEF
    #define _SOCKET_GETHOSTBYNAME_EX_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTBYNAME_EX_METHODDEF) */

#ifndef _SOCKET_GETHOSTBYADDR_METHODDEF
    #define _SOCKET_GETHOSTBYADDR_METHODDEF
#endif /* !defined(_SOCKET_GETHOSTBYADDR_METHODDEF) */

#ifndef _SOCKET_GETSERVBYNAME_METHODDEF
    #define _SOCKET_GETSERVBYNAME_METHODDEF
#endif /* !defined(_SOCKET_GETSERVBYNAME_METHODDEF) */

#ifndef _SOCKET_GETSERVBYPORT_METHODDEF
    #define _SOCKET_GETSERVBYPORT_METHODDEF
#endif /* !defined(_SOCKET_GETSERVBYPORT_METHODDEF) */

#ifndef _SOCKET_GETPROTOBYNAME_METHODDEF
    #define _SOCKET_GETPROTOBYNAME_METHODDEF
#endif /* !defined(_SOCKET_GETPROTOBYNAME_METHODDEF) */

#ifndef _SOCKET_DUP_METHODDEF
    #define _SOCKET_DUP_METHODDEF
#endif /* !defined(_SOCKET_DUP_METHODDEF) */

#ifndef _SOCKET_SOCKETPAIR_METHODDEF
    #define _SOCKET_SOCKETPAIR_METHODDEF
#endif /* !defined(_SOCKET_SOCKETPAIR_METHODDEF) */

#ifndef _SOCKET_INET_NTOA_METHODDEF
    #define _SOCKET_INET_NTOA_METHODDEF
#endif /* !defined(_SOCKET_INET_NTOA_METHODDEF) */

#ifndef _SOCKET_INET_PTON_METHODDEF
    #define _SOCKET_INET_PTON_METHODDEF
#endif /* !defined(_SOCKET_INET_PTON_METHODDEF) */

#ifndef _SOCKET_INET_NTOP_METHODDEF
    #define _SOCKET_INET_NTOP_METHODDEF
#endif /* !defined(_SOCKET_INET_NTOP_METHODDEF) */

#ifndef _SOCKET_GETADDRINFO_METHODDEF
    #define _SOCKET_GETADDRINFO_METHODDEF
#endif /* !defined(_SOCKET_GETADDRINFO_METHODDEF) */

#ifndef _SOCKET_GETNAMEINFO_METHODDEF
    #define _SOCKET_GETNAMEINFO_METHODDEF
#endif /* !defined(_SOCKET_GETNAMEINFO_METHODDEF) */

#ifndef _SOCKET_IF_NAMEINDEX_METHODDEF
    #define _SOCKET_IF_NAMEINDEX_METHODDEF
#endif /* !defined(_SOCKET_IF_NAMEINDEX_METHODDEF) */

#ifndef _SOCKET_IF_NAMETOINDEX_METHODDEF
    #define _SOCKET_IF_NAMETOINDEX_METHODDEF
#endif /* !defined(_SOCKET_IF_NAMETOINDEX_METHODDEF) */

#ifndef _SOCKET_IF_INDEXTONAME_METHODDEF
    #define _SOCKET_IF_INDEXTONAME_METHODDEF
#endif /* !defined(_SOCKET_IF_INDEXTONAME_METHODDEF) */

#ifndef _SOCKET_CMSG_LEN_METHODDEF
    #define _SOCKET_CMSG_LEN_METHODDEF
#endif /* !defined(_SOCKET_CMSG_LEN_METHODDEF) */

#ifndef _SOCKET_CMSG_SPACE_METHODDEF
    #define _SOCKET_CMSG_SPACE_METHODDEF
#endif /* !defined(_SOCKET_CMSG_SPACE_METHODDEF) */
/*[clinic end generated code: output=6c8c8e7381f3201c input=a9049054013a1b77]*/
