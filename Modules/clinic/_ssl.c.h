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
#include "pycore_long.h"          // _PyLong_UnsignedLongLong_Converter()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(_ssl__SSLSocket_do_handshake__doc__,
"do_handshake($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_DO_HANDSHAKE_METHODDEF    \
    {"do_handshake", _PyCFunction_CAST(_ssl__SSLSocket_do_handshake), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_do_handshake__doc__},

static PyValue
_ssl__SSLSocket_do_handshake_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_do_handshake(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_do_handshake_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__test_decode_cert__doc__,
"_test_decode_cert($module, path, /)\n"
"--\n"
"\n");

#define _SSL__TEST_DECODE_CERT_METHODDEF    \
    {"_test_decode_cert", _PyCFunction_CAST(_ssl__test_decode_cert), METH_VALUE|METH_O, _ssl__test_decode_cert__doc__},

static PyValue
_ssl__test_decode_cert_impl(PyObject *module, PyObject *path);

static PyValue
_ssl__test_decode_cert(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *path = NULL;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "expected str, bytes or os.PathLike object, not %.200s", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (!PyUnicode_FSConverter(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &path)) {
        goto exit;
    }
    return_value = _ssl__test_decode_cert_impl(PyValue_AsKnownObjectBorrow(module), path);

exit:
    /* Cleanup for path */
    Py_XDECREF(path);

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_getpeercert__doc__,
"getpeercert($self, der=False, /)\n"
"--\n"
"\n"
"Returns the certificate for the peer.\n"
"\n"
"If no certificate was provided, returns None.  If a certificate was\n"
"provided, but not validated, returns an empty dictionary.  Otherwise\n"
"returns a dict containing information about the peer certificate.\n"
"\n"
"If the optional argument is True, returns a DER-encoded copy of the\n"
"peer certificate, or None if no certificate was provided.  This will\n"
"return the certificate even if it wasn\'t validated.");

#define _SSL__SSLSOCKET_GETPEERCERT_METHODDEF    \
    {"getpeercert", _PyCFunction_CAST(_ssl__SSLSocket_getpeercert), METH_VALUE, _ssl__SSLSocket_getpeercert__doc__},

static PyValue
_ssl__SSLSocket_getpeercert_impl(PySSLSocket *self, int binary_mode);

static PyValue
_ssl__SSLSocket_getpeercert(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int binary_mode = 0;

    if (!_PyArg_CheckPositional("getpeercert", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    binary_mode = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (binary_mode < 0) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_getpeercert_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self), binary_mode);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_get_verified_chain__doc__,
"get_verified_chain($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_GET_VERIFIED_CHAIN_METHODDEF    \
    {"get_verified_chain", _PyCFunction_CAST(_ssl__SSLSocket_get_verified_chain), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_get_verified_chain__doc__},

static PyValue
_ssl__SSLSocket_get_verified_chain_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_get_verified_chain(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_get_verified_chain_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_get_unverified_chain__doc__,
"get_unverified_chain($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_GET_UNVERIFIED_CHAIN_METHODDEF    \
    {"get_unverified_chain", _PyCFunction_CAST(_ssl__SSLSocket_get_unverified_chain), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_get_unverified_chain__doc__},

static PyValue
_ssl__SSLSocket_get_unverified_chain_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_get_unverified_chain(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_get_unverified_chain_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_shared_ciphers__doc__,
"shared_ciphers($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_SHARED_CIPHERS_METHODDEF    \
    {"shared_ciphers", _PyCFunction_CAST(_ssl__SSLSocket_shared_ciphers), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_shared_ciphers__doc__},

static PyValue
_ssl__SSLSocket_shared_ciphers_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_shared_ciphers(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_shared_ciphers_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_cipher__doc__,
"cipher($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_CIPHER_METHODDEF    \
    {"cipher", _PyCFunction_CAST(_ssl__SSLSocket_cipher), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_cipher__doc__},

static PyValue
_ssl__SSLSocket_cipher_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_cipher(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_cipher_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_group__doc__,
"group($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_GROUP_METHODDEF    \
    {"group", _PyCFunction_CAST(_ssl__SSLSocket_group), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_group__doc__},

static PyValue
_ssl__SSLSocket_group_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_group(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_group_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_client_sigalg__doc__,
"client_sigalg($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_CLIENT_SIGALG_METHODDEF    \
    {"client_sigalg", _PyCFunction_CAST(_ssl__SSLSocket_client_sigalg), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_client_sigalg__doc__},

static PyValue
_ssl__SSLSocket_client_sigalg_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_client_sigalg(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_client_sigalg_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_server_sigalg__doc__,
"server_sigalg($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_SERVER_SIGALG_METHODDEF    \
    {"server_sigalg", _PyCFunction_CAST(_ssl__SSLSocket_server_sigalg), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_server_sigalg__doc__},

static PyValue
_ssl__SSLSocket_server_sigalg_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_server_sigalg(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_server_sigalg_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_version__doc__,
"version($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_VERSION_METHODDEF    \
    {"version", _PyCFunction_CAST(_ssl__SSLSocket_version), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_version__doc__},

static PyValue
_ssl__SSLSocket_version_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_version(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_version_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_selected_alpn_protocol__doc__,
"selected_alpn_protocol($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_SELECTED_ALPN_PROTOCOL_METHODDEF    \
    {"selected_alpn_protocol", _PyCFunction_CAST(_ssl__SSLSocket_selected_alpn_protocol), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_selected_alpn_protocol__doc__},

static PyValue
_ssl__SSLSocket_selected_alpn_protocol_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_selected_alpn_protocol(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_selected_alpn_protocol_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_compression__doc__,
"compression($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLSOCKET_COMPRESSION_METHODDEF    \
    {"compression", _PyCFunction_CAST(_ssl__SSLSocket_compression), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_compression__doc__},

static PyValue
_ssl__SSLSocket_compression_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_compression(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _ssl__SSLSocket_compression_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_context__doc__,
"This changes the context associated with the SSLSocket.\n"
"\n"
"This is typically used from within a callback function set by the\n"
"sni_callback on the SSLContext to change the certificate information\n"
"associated with the SSLSocket before the cryptographic exchange\n"
"handshake messages.");

static PyObject *
_ssl__SSLSocket_context_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_context_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_context_get_impl((PySSLSocket *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLSocket_context_set_impl(PySSLSocket *self, PyObject *value);

static int
_ssl__SSLSocket_context_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'context' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_context_set_impl((PySSLSocket *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_server_side__doc__,
"Whether this is a server-side socket.");

static PyObject *
_ssl__SSLSocket_server_side_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_server_side_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_server_side_get_impl((PySSLSocket *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_server_hostname__doc__,
"The currently set server hostname (for SNI).");

static PyObject *
_ssl__SSLSocket_server_hostname_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_server_hostname_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_server_hostname_get_impl((PySSLSocket *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_owner__doc__,
"The Python-level owner of this object.\n"
"\n"
"Passed as \"self\" in servername callback.");

static PyObject *
_ssl__SSLSocket_owner_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_owner_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_owner_get_impl((PySSLSocket *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLSocket_owner_set_impl(PySSLSocket *self, PyObject *value);

static int
_ssl__SSLSocket_owner_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'owner' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_owner_set_impl((PySSLSocket *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_uses_ktls_for_send__doc__,
"uses_ktls_for_send($self, /)\n"
"--\n"
"\n"
"Check if the Kernel TLS data-path is used for sending.");

#define _SSL__SSLSOCKET_USES_KTLS_FOR_SEND_METHODDEF    \
    {"uses_ktls_for_send", _PyCFunction_CAST(_ssl__SSLSocket_uses_ktls_for_send), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_uses_ktls_for_send__doc__},

static PyValue
_ssl__SSLSocket_uses_ktls_for_send_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_uses_ktls_for_send(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_uses_ktls_for_send_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_uses_ktls_for_recv__doc__,
"uses_ktls_for_recv($self, /)\n"
"--\n"
"\n"
"Check if the Kernel TLS data-path is used for receiving.");

#define _SSL__SSLSOCKET_USES_KTLS_FOR_RECV_METHODDEF    \
    {"uses_ktls_for_recv", _PyCFunction_CAST(_ssl__SSLSocket_uses_ktls_for_recv), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_uses_ktls_for_recv__doc__},

static PyValue
_ssl__SSLSocket_uses_ktls_for_recv_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_uses_ktls_for_recv(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_uses_ktls_for_recv_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if defined(BIO_get_ktls_send)

PyDoc_STRVAR(_ssl__SSLSocket_sendfile__doc__,
"sendfile($self, fd, offset, size, flags=0, /)\n"
"--\n"
"\n"
"Write size bytes from offset in the file descriptor fd to the SSL connection.\n"
"\n"
"This method uses the zero-copy technique and returns the number of\n"
"bytes written.  It should be called only when Kernel TLS is used for\n"
"sending data in the connection.\n"
"\n"
"The meaning of flags is platform dependent.");

#define _SSL__SSLSOCKET_SENDFILE_METHODDEF    \
    {"sendfile", _PyCFunction_CAST(_ssl__SSLSocket_sendfile), METH_VALUE, _ssl__SSLSocket_sendfile__doc__},

static PyValue
_ssl__SSLSocket_sendfile_impl(PySSLSocket *self, int fd, Py_off_t offset,
                              size_t size, int flags);

static PyValue
_ssl__SSLSocket_sendfile(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;
    Py_off_t offset;
    size_t size;
    int flags = 0;

    if (!_PyArg_CheckPositional("sendfile", nargs, 3, 4)) {
        goto exit;
    }
    fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    #ifdef HAVE_LARGEFILE_SUPPORT
    offset = _PyValue_AsLongLong(PyValue_Borrow(args[1]));
    #else
    offset = _PyValue_AsLong(PyValue_Borrow(args[1]));
    #endif
    if (PyErr_Occurred()) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[2]), &size, sizeof(size_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(size_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C size_t");
            goto exit;
        }
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    flags = _PyValue_AsInt(PyValue_Borrow(args[3]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_sendfile_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self), fd, offset, size, flags);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(BIO_get_ktls_send) */

PyDoc_STRVAR(_ssl__SSLSocket_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Writes the bytes-like object b into the SSL object.\n"
"\n"
"Returns the number of bytes written.");

#define _SSL__SSLSOCKET_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_ssl__SSLSocket_write), METH_VALUE|METH_O, _ssl__SSLSocket_write__doc__},

static PyValue
_ssl__SSLSocket_write_impl(PySSLSocket *self, Py_buffer *b);

static PyValue
_ssl__SSLSocket_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer b = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_write_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self), &b);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_pending__doc__,
"pending($self, /)\n"
"--\n"
"\n"
"Returns the number of already decrypted bytes available for read, pending on the connection.");

#define _SSL__SSLSOCKET_PENDING_METHODDEF    \
    {"pending", _PyCFunction_CAST(_ssl__SSLSocket_pending), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_pending__doc__},

static PyValue
_ssl__SSLSocket_pending_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_pending(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_pending_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_read__doc__,
"read(size, [buffer])\n"
"Read up to size bytes from the SSL socket.");

#define _SSL__SSLSOCKET_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_ssl__SSLSocket_read), METH_FASTCALL, _ssl__SSLSocket_read__doc__},

static PyObject *
_ssl__SSLSocket_read_impl(PySSLSocket *self, Py_ssize_t len,
                          int group_right_1, Py_buffer *buffer);

static PyObject *
_ssl__SSLSocket_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t len;
    int group_right_1 = 0;
    Py_buffer buffer = {NULL, NULL};

    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "_ssl._SSLSocket.read requires 1 to 2 arguments");
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_FromObjectBorrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        len = ival;
    }
    if (nargs >= 2) {
        if (PyObject_GetBuffer(args[1], &buffer, PyBUF_WRITABLE) < 0) {
            _PyArg_BadArgument("read", "argument 2", "read-write bytes-like object", args[1]);
            goto exit;
        }
        group_right_1 = 1;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_read_impl((PySSLSocket *)self, len, group_right_1, &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_shutdown__doc__,
"shutdown($self, /)\n"
"--\n"
"\n"
"Does the SSL shutdown handshake with the remote end.");

#define _SSL__SSLSOCKET_SHUTDOWN_METHODDEF    \
    {"shutdown", _PyCFunction_CAST(_ssl__SSLSocket_shutdown), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_shutdown__doc__},

static PyValue
_ssl__SSLSocket_shutdown_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_shutdown(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_shutdown_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_get_channel_binding__doc__,
"get_channel_binding($self, /, cb_type=\'tls-unique\')\n"
"--\n"
"\n"
"Get channel binding data for current connection.\n"
"\n"
"Raise ValueError if the requested `cb_type` is not supported.\n"
"Return bytes of the data or None if the data is not available (e.g.\n"
"before the handshake).\n"
"Only \'tls-unique\' channel binding data from RFC 5929 is supported.");

#define _SSL__SSLSOCKET_GET_CHANNEL_BINDING_METHODDEF    \
    {"get_channel_binding", _PyCFunction_CAST(_ssl__SSLSocket_get_channel_binding), METH_VALUE|METH_KEYWORDS, _ssl__SSLSocket_get_channel_binding__doc__},

static PyValue
_ssl__SSLSocket_get_channel_binding_impl(PySSLSocket *self,
                                         const char *cb_type);

static PyValue
_ssl__SSLSocket_get_channel_binding(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(cb_type)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cb_type", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_channel_binding",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *cb_type = "tls-unique";

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("get_channel_binding", "argument 'cb_type'", "str", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("get_channel_binding", "argument 'cb_type'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        cb_type = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        if (cb_type == NULL) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_get_channel_binding_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self), cb_type);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_verify_client_post_handshake__doc__,
"verify_client_post_handshake($self, /)\n"
"--\n"
"\n"
"Initiate TLS 1.3 post-handshake authentication");

#define _SSL__SSLSOCKET_VERIFY_CLIENT_POST_HANDSHAKE_METHODDEF    \
    {"verify_client_post_handshake", _PyCFunction_CAST(_ssl__SSLSocket_verify_client_post_handshake), METH_VALUE|METH_NOARGS, _ssl__SSLSocket_verify_client_post_handshake__doc__},

static PyValue
_ssl__SSLSocket_verify_client_post_handshake_impl(PySSLSocket *self);

static PyValue
_ssl__SSLSocket_verify_client_post_handshake(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLSocket_verify_client_post_handshake_impl((PySSLSocket *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_session__doc__,
"The underlying SSLSession object.");

static PyObject *
_ssl__SSLSocket_session_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_session_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_session_get_impl((PySSLSocket *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLSocket_session_set_impl(PySSLSocket *self, PyObject *value);

static int
_ssl__SSLSocket_session_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'session' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_session_set_impl((PySSLSocket *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLSocket_session_reused__doc__,
"Was the client session reused during handshake?");

static PyObject *
_ssl__SSLSocket_session_reused_get_impl(PySSLSocket *self);

static PyObject *
_ssl__SSLSocket_session_reused_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLSocket_session_reused_get_impl((PySSLSocket *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyValue
_ssl__SSLContext_impl(PyTypeObject *type, int proto_version);

static PyValue
_ssl__SSLContext_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = get_state_type(type)->PySSLContext_Type;
    int proto_version;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("_SSLContext", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_SSLContext", nargs, 1, 1)) {
        goto exit;
    }
    proto_version = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (proto_version == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(type));
    return_value = _ssl__SSLContext_impl(type, proto_version);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_ssl__SSLContext(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_ssl__SSLContext_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _ssl__SSLContext_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_ssl__SSLContext_set_ciphers__doc__,
"set_ciphers($self, cipherlist, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_CIPHERS_METHODDEF    \
    {"set_ciphers", _PyCFunction_CAST(_ssl__SSLContext_set_ciphers), METH_VALUE|METH_O, _ssl__SSLContext_set_ciphers__doc__},

static PyValue
_ssl__SSLContext_set_ciphers_impl(PySSLContext *self, const char *cipherlist);

static PyValue
_ssl__SSLContext_set_ciphers(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *cipherlist;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("set_ciphers", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("set_ciphers", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    cipherlist = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (cipherlist == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_ciphers_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), cipherlist);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_ciphersuites__doc__,
"set_ciphersuites($self, ciphersuites, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_CIPHERSUITES_METHODDEF    \
    {"set_ciphersuites", _PyCFunction_CAST(_ssl__SSLContext_set_ciphersuites), METH_VALUE|METH_O, _ssl__SSLContext_set_ciphersuites__doc__},

static PyValue
_ssl__SSLContext_set_ciphersuites_impl(PySSLContext *self,
                                       const char *ciphersuites);

static PyValue
_ssl__SSLContext_set_ciphersuites(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *ciphersuites;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("set_ciphersuites", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("set_ciphersuites", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    ciphersuites = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (ciphersuites == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_ciphersuites_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), ciphersuites);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_get_ciphers__doc__,
"get_ciphers($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_GET_CIPHERS_METHODDEF    \
    {"get_ciphers", _PyCFunction_CAST(_ssl__SSLContext_get_ciphers), METH_VALUE|METH_NOARGS, _ssl__SSLContext_get_ciphers__doc__},

static PyValue
_ssl__SSLContext_get_ciphers_impl(PySSLContext *self);

static PyValue
_ssl__SSLContext_get_ciphers(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_get_ciphers_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_groups__doc__,
"set_groups($self, grouplist, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_GROUPS_METHODDEF    \
    {"set_groups", _PyCFunction_CAST(_ssl__SSLContext_set_groups), METH_VALUE|METH_O, _ssl__SSLContext_set_groups__doc__},

static PyValue
_ssl__SSLContext_set_groups_impl(PySSLContext *self, const char *grouplist);

static PyValue
_ssl__SSLContext_set_groups(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *grouplist;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("set_groups", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("set_groups", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    grouplist = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (grouplist == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_groups_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), grouplist);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_get_groups__doc__,
"get_groups($self, /, *, include_aliases=False)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_GET_GROUPS_METHODDEF    \
    {"get_groups", _PyCFunction_CAST(_ssl__SSLContext_get_groups), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext_get_groups__doc__},

static PyValue
_ssl__SSLContext_get_groups_impl(PySSLContext *self, int include_aliases);

static PyValue
_ssl__SSLContext_get_groups(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(include_aliases)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"include_aliases", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_groups",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int include_aliases = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        include_aliases = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (include_aliases < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_get_groups_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), include_aliases);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_client_sigalgs__doc__,
"set_client_sigalgs($self, sigalgslist, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_CLIENT_SIGALGS_METHODDEF    \
    {"set_client_sigalgs", _PyCFunction_CAST(_ssl__SSLContext_set_client_sigalgs), METH_VALUE|METH_O, _ssl__SSLContext_set_client_sigalgs__doc__},

static PyValue
_ssl__SSLContext_set_client_sigalgs_impl(PySSLContext *self,
                                         const char *sigalgslist);

static PyValue
_ssl__SSLContext_set_client_sigalgs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *sigalgslist;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("set_client_sigalgs", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("set_client_sigalgs", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    sigalgslist = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (sigalgslist == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_client_sigalgs_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), sigalgslist);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_server_sigalgs__doc__,
"set_server_sigalgs($self, sigalgslist, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_SERVER_SIGALGS_METHODDEF    \
    {"set_server_sigalgs", _PyCFunction_CAST(_ssl__SSLContext_set_server_sigalgs), METH_VALUE|METH_O, _ssl__SSLContext_set_server_sigalgs__doc__},

static PyValue
_ssl__SSLContext_set_server_sigalgs_impl(PySSLContext *self,
                                         const char *sigalgslist);

static PyValue
_ssl__SSLContext_set_server_sigalgs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *sigalgslist;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("set_server_sigalgs", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("set_server_sigalgs", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    sigalgslist = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (sigalgslist == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_server_sigalgs_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), sigalgslist);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__set_alpn_protocols__doc__,
"_set_alpn_protocols($self, protos, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__SET_ALPN_PROTOCOLS_METHODDEF    \
    {"_set_alpn_protocols", _PyCFunction_CAST(_ssl__SSLContext__set_alpn_protocols), METH_VALUE|METH_O, _ssl__SSLContext__set_alpn_protocols__doc__},

static PyValue
_ssl__SSLContext__set_alpn_protocols_impl(PySSLContext *self,
                                          Py_buffer *protos);

static PyValue
_ssl__SSLContext__set_alpn_protocols(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer protos = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &protos, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext__set_alpn_protocols_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), &protos);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for protos */
    if (protos.obj) {
       PyBuffer_Release(&protos);
    }

    return return_value;
}

static int
_ssl__SSLContext_verify_mode_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_verify_mode_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ssl__SSLContext_verify_mode_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
_ssl__SSLContext_verify_mode_set_impl(PySSLContext *self, int value);

static int
_ssl__SSLContext_verify_mode_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    int value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'verify_mode' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = _PyValue_AsInt(PyValue_FromObjectBorrow(arg));
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_verify_mode_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static unsigned long
_ssl__SSLContext_verify_flags_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_verify_flags_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    unsigned long _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ssl__SSLContext_verify_flags_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == (unsigned long)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong(_return_value);

exit:
    return return_value;
}

static int
_ssl__SSLContext_verify_flags_set_impl(PySSLContext *self,
                                       unsigned long value);

static int
_ssl__SSLContext_verify_flags_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    unsigned long value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'verify_flags' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    if (!PyIndex_Check(arg)) {
        PyErr_Format(PyExc_TypeError, "attribute 'verify_flags' must be int, not %T", arg);
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(arg), &value, sizeof(unsigned long), Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_verify_flags_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
_ssl__SSLContext_minimum_version_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_minimum_version_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_minimum_version_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLContext_minimum_version_set_impl(PySSLContext *self,
                                          PyObject *value);

static int
_ssl__SSLContext_minimum_version_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'minimum_version' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_minimum_version_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_ssl__SSLContext_maximum_version_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_maximum_version_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_maximum_version_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLContext_maximum_version_set_impl(PySSLContext *self,
                                          PyObject *value);

static int
_ssl__SSLContext_maximum_version_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'maximum_version' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_maximum_version_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_num_tickets__doc__,
"Control the number of TLSv1.3 session tickets.");

static PyObject *
_ssl__SSLContext_num_tickets_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_num_tickets_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_num_tickets_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLContext_num_tickets_set_impl(PySSLContext *self, long value);

static int
_ssl__SSLContext_num_tickets_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    long value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'num_tickets' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = _PyValue_AsLong(PyValue_FromObjectBorrow(arg));
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_num_tickets_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_security_level__doc__,
"The current security level.");

static PyObject *
_ssl__SSLContext_security_level_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_security_level_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_security_level_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_ssl__SSLContext_options_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_options_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_options_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLContext_options_set_impl(PySSLContext *self,
                                  unsigned long long value);

static int
_ssl__SSLContext_options_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    unsigned long long value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'options' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    if (!_PyLong_UnsignedLongLong_Converter(arg, &value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_options_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static unsigned int
_ssl__SSLContext__host_flags_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext__host_flags_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    unsigned int _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ssl__SSLContext__host_flags_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    return return_value;
}

static int
_ssl__SSLContext__host_flags_set_impl(PySSLContext *self, unsigned int value);

static int
_ssl__SSLContext__host_flags_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    unsigned int value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '_host_flags' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(arg), &value, sizeof(unsigned int), Py_ASNATIVEBYTES_NATIVE_ENDIAN |
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext__host_flags_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
_ssl__SSLContext_check_hostname_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_check_hostname_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    int _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _ssl__SSLContext_check_hostname_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
_ssl__SSLContext_check_hostname_set_impl(PySSLContext *self, int value);

static int
_ssl__SSLContext_check_hostname_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    int value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'check_hostname' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = _PyValue_IsTrue(PyValue_FromObjectBorrow(arg));
    if (value < 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_check_hostname_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
_ssl__SSLContext_protocol_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_protocol_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_protocol_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_cert_chain__doc__,
"load_cert_chain($self, /, certfile, keyfile=None, password=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_CERT_CHAIN_METHODDEF    \
    {"load_cert_chain", _PyCFunction_CAST(_ssl__SSLContext_load_cert_chain), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext_load_cert_chain__doc__},

static PyValue
_ssl__SSLContext_load_cert_chain_impl(PySSLContext *self,
                                      PyValueBorrow certfile,
                                      PyValueBorrow keyfile,
                                      PyValueBorrow password);

static PyValue
_ssl__SSLContext_load_cert_chain(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(certfile)), _PyValue_STATIC_IMMORTAL(&_Py_ID(keyfile)), _PyValue_STATIC_IMMORTAL(&_Py_ID(password)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"certfile", "keyfile", "password", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load_cert_chain",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow certfile;
    PyValueBorrow keyfile = PyValue_Borrow(PyValue_None);
    PyValueBorrow password = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    certfile = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        keyfile = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        password = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _ssl__SSLContext_load_cert_chain_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), certfile, keyfile, password);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_verify_locations__doc__,
"load_verify_locations($self, /, cafile=None, capath=None, cadata=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_VERIFY_LOCATIONS_METHODDEF    \
    {"load_verify_locations", _PyCFunction_CAST(_ssl__SSLContext_load_verify_locations), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext_load_verify_locations__doc__},

static PyValue
_ssl__SSLContext_load_verify_locations_impl(PySSLContext *self,
                                            PyValueBorrow cafile,
                                            PyValueBorrow capath,
                                            PyValueBorrow cadata);

static PyValue
_ssl__SSLContext_load_verify_locations(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(cafile)), _PyValue_STATIC_IMMORTAL(&_Py_ID(capath)), _PyValue_STATIC_IMMORTAL(&_Py_ID(cadata)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cafile", "capath", "cadata", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load_verify_locations",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow cafile = PyValue_Borrow(PyValue_None);
    PyValueBorrow capath = PyValue_Borrow(PyValue_None);
    PyValueBorrow cadata = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        cafile = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        capath = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        cadata = PyValue_Borrow(args[_idx[2]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_load_verify_locations_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), cafile, capath, cadata);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_load_dh_params__doc__,
"load_dh_params($self, path, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_LOAD_DH_PARAMS_METHODDEF    \
    {"load_dh_params", _PyCFunction_CAST(_ssl__SSLContext_load_dh_params), METH_VALUE|METH_O, _ssl__SSLContext_load_dh_params__doc__},

static PyValue
_ssl__SSLContext_load_dh_params_impl(PySSLContext *self,
                                     PyValueBorrow filepath);

static PyValue
_ssl__SSLContext_load_dh_params(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filepath;

    filepath = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_load_dh_params_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), filepath);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__wrap_socket__doc__,
"_wrap_socket($self, /, sock, server_side, server_hostname=None, *,\n"
"             owner=None, session=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__WRAP_SOCKET_METHODDEF    \
    {"_wrap_socket", _PyCFunction_CAST(_ssl__SSLContext__wrap_socket), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext__wrap_socket__doc__},

static PyValue
_ssl__SSLContext__wrap_socket_impl(PySSLContext *self, PyObject *sock,
                                   int server_side,
                                   PyValueBorrow hostname_obj,
                                   PyValueBorrow owner,
                                   PyValueBorrow session);

static PyValue
_ssl__SSLContext__wrap_socket(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sock)), _PyValue_STATIC_IMMORTAL(&_Py_ID(server_side)), _PyValue_STATIC_IMMORTAL(&_Py_ID(server_hostname)), _PyValue_STATIC_IMMORTAL(&_Py_ID(owner)), _PyValue_STATIC_IMMORTAL(&_Py_ID(session)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sock", "server_side", "server_hostname", "owner", "session", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_wrap_socket",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyObject *sock;
    int server_side;
    PyValueBorrow hostname_obj = PyValue_Borrow(PyValue_None);
    PyValueBorrow owner = PyValue_Borrow(PyValue_None);
    PyValueBorrow session = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), get_state_ctx(self)->Sock_Type)) {
        _PyArg_BadArgumentValue("_wrap_socket", "argument 'sock'", (get_state_ctx(self)->Sock_Type)->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    sock = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    server_side = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
    if (server_side < 0) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        hostname_obj = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        owner = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        session = PyValue_Borrow(args[_idx[4]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext__wrap_socket_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), sock, server_side, hostname_obj, owner, session);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext__wrap_bio__doc__,
"_wrap_bio($self, /, incoming, outgoing, server_side,\n"
"          server_hostname=None, *, owner=None, session=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT__WRAP_BIO_METHODDEF    \
    {"_wrap_bio", _PyCFunction_CAST(_ssl__SSLContext__wrap_bio), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext__wrap_bio__doc__},

static PyValue
_ssl__SSLContext__wrap_bio_impl(PySSLContext *self, PySSLMemoryBIO *incoming,
                                PySSLMemoryBIO *outgoing, int server_side,
                                PyValueBorrow hostname_obj,
                                PyValueBorrow owner, PyValueBorrow session);

static PyValue
_ssl__SSLContext__wrap_bio(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(incoming)), _PyValue_STATIC_IMMORTAL(&_Py_ID(outgoing)), _PyValue_STATIC_IMMORTAL(&_Py_ID(server_side)), _PyValue_STATIC_IMMORTAL(&_Py_ID(server_hostname)), _PyValue_STATIC_IMMORTAL(&_Py_ID(owner)), _PyValue_STATIC_IMMORTAL(&_Py_ID(session)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"incoming", "outgoing", "server_side", "server_hostname", "owner", "session", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_wrap_bio",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PySSLMemoryBIO *incoming;
    PySSLMemoryBIO *outgoing;
    int server_side;
    PyValueBorrow hostname_obj = PyValue_Borrow(PyValue_None);
    PyValueBorrow owner = PyValue_Borrow(PyValue_None);
    PyValueBorrow session = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), get_state_ctx(self)->PySSLMemoryBIO_Type)) {
        _PyArg_BadArgumentValue("_wrap_bio", "argument 'incoming'", (get_state_ctx(self)->PySSLMemoryBIO_Type)->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    incoming = (PySSLMemoryBIO *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), get_state_ctx(self)->PySSLMemoryBIO_Type)) {
        _PyArg_BadArgumentValue("_wrap_bio", "argument 'outgoing'", (get_state_ctx(self)->PySSLMemoryBIO_Type)->tp_name, PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    outgoing = (PySSLMemoryBIO *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    server_side = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
    if (server_side < 0) {
        goto exit;
    }
    if (_idx[3] >= 0) {
        hostname_obj = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        owner = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        session = PyValue_Borrow(args[_idx[5]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext__wrap_bio_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), incoming, outgoing, server_side, hostname_obj, owner, session);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_session_stats__doc__,
"session_stats($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SESSION_STATS_METHODDEF    \
    {"session_stats", _PyCFunction_CAST(_ssl__SSLContext_session_stats), METH_VALUE|METH_NOARGS, _ssl__SSLContext_session_stats__doc__},

static PyValue
_ssl__SSLContext_session_stats_impl(PySSLContext *self);

static PyValue
_ssl__SSLContext_session_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_session_stats_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_default_verify_paths__doc__,
"set_default_verify_paths($self, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_DEFAULT_VERIFY_PATHS_METHODDEF    \
    {"set_default_verify_paths", _PyCFunction_CAST(_ssl__SSLContext_set_default_verify_paths), METH_VALUE|METH_NOARGS, _ssl__SSLContext_set_default_verify_paths__doc__},

static PyValue
_ssl__SSLContext_set_default_verify_paths_impl(PySSLContext *self);

static PyValue
_ssl__SSLContext_set_default_verify_paths(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_default_verify_paths_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_ecdh_curve__doc__,
"set_ecdh_curve($self, name, /)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_ECDH_CURVE_METHODDEF    \
    {"set_ecdh_curve", _PyCFunction_CAST(_ssl__SSLContext_set_ecdh_curve), METH_VALUE|METH_O, _ssl__SSLContext_set_ecdh_curve__doc__},

static PyValue
_ssl__SSLContext_set_ecdh_curve_impl(PySSLContext *self, PyValueBorrow name);

static PyValue
_ssl__SSLContext_set_ecdh_curve(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow name;

    name = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_ecdh_curve_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), name);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_sni_callback__doc__,
"Set a callback that will be called when a server name is provided by the SSL/TLS client in the SNI extension.\n"
"\n"
"If the argument is None then the callback is disabled.  The method\n"
"is called with the SSLSocket, the server name as a string, and the\n"
"SSLContext object.\n"
"\n"
"See RFC 6066 for details of the SNI extension.");

static PyObject *
_ssl__SSLContext_sni_callback_get_impl(PySSLContext *self);

static PyObject *
_ssl__SSLContext_sni_callback_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_sni_callback_get_impl((PySSLContext *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
_ssl__SSLContext_sni_callback_set_impl(PySSLContext *self, PyObject *value);

static int
_ssl__SSLContext_sni_callback_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute 'sni_callback' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl__SSLContext_sni_callback_set_impl((PySSLContext *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_cert_store_stats__doc__,
"cert_store_stats($self, /)\n"
"--\n"
"\n"
"Returns quantities of loaded X.509 certificates.\n"
"\n"
"X.509 certificates with a CA extension and certificate revocation\n"
"lists inside the context\'s cert store.\n"
"\n"
"NOTE: Certificates in a capath directory aren\'t loaded unless they\n"
"have been used at least once.");

#define _SSL__SSLCONTEXT_CERT_STORE_STATS_METHODDEF    \
    {"cert_store_stats", _PyCFunction_CAST(_ssl__SSLContext_cert_store_stats), METH_VALUE|METH_NOARGS, _ssl__SSLContext_cert_store_stats__doc__},

static PyValue
_ssl__SSLContext_cert_store_stats_impl(PySSLContext *self);

static PyValue
_ssl__SSLContext_cert_store_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_cert_store_stats_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_get_ca_certs__doc__,
"get_ca_certs($self, /, binary_form=False)\n"
"--\n"
"\n"
"Returns a list of dicts with information of loaded CA certs.\n"
"\n"
"If the optional argument is True, returns a DER-encoded copy of the\n"
"CA certificate.\n"
"\n"
"NOTE: Certificates in a capath directory aren\'t loaded unless they\n"
"have been used at least once.");

#define _SSL__SSLCONTEXT_GET_CA_CERTS_METHODDEF    \
    {"get_ca_certs", _PyCFunction_CAST(_ssl__SSLContext_get_ca_certs), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext_get_ca_certs__doc__},

static PyValue
_ssl__SSLContext_get_ca_certs_impl(PySSLContext *self, int binary_form);

static PyValue
_ssl__SSLContext_get_ca_certs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(binary_form)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"binary_form", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_ca_certs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int binary_form = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        binary_form = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (binary_form < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_get_ca_certs_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), binary_form);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_psk_client_callback__doc__,
"set_psk_client_callback($self, /, callback)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_PSK_CLIENT_CALLBACK_METHODDEF    \
    {"set_psk_client_callback", _PyCFunction_CAST(_ssl__SSLContext_set_psk_client_callback), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext_set_psk_client_callback__doc__},

static PyValue
_ssl__SSLContext_set_psk_client_callback_impl(PySSLContext *self,
                                              PyValueBorrow callback);

static PyValue
_ssl__SSLContext_set_psk_client_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(callback)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"callback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_psk_client_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow callback;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    callback = PyValue_Borrow(args[_idx[0]]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_psk_client_callback_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), callback);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl__SSLContext_set_psk_server_callback__doc__,
"set_psk_server_callback($self, /, callback, identity_hint=None)\n"
"--\n"
"\n");

#define _SSL__SSLCONTEXT_SET_PSK_SERVER_CALLBACK_METHODDEF    \
    {"set_psk_server_callback", _PyCFunction_CAST(_ssl__SSLContext_set_psk_server_callback), METH_VALUE|METH_KEYWORDS, _ssl__SSLContext_set_psk_server_callback__doc__},

static PyValue
_ssl__SSLContext_set_psk_server_callback_impl(PySSLContext *self,
                                              PyValueBorrow callback,
                                              const char *identity_hint);

static PyValue
_ssl__SSLContext_set_psk_server_callback(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(callback)), _PyValue_STATIC_IMMORTAL(&_Py_ID(identity_hint)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"callback", "identity_hint", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_psk_server_callback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow callback;
    const char *identity_hint = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    callback = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("set_psk_server_callback", "argument 'identity_hint'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            identity_hint = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            identity_hint = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (identity_hint == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("set_psk_server_callback", "argument 'identity_hint'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl__SSLContext_set_psk_server_callback_impl((PySSLContext *)PyValue_AsKnownObjectBorrow(self), callback, identity_hint);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyValue
_ssl_MemoryBIO_impl(PyTypeObject *type);

static PyValue
_ssl_MemoryBIO_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = get_state_type(type)->PySSLMemoryBIO_Type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("MemoryBIO", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("MemoryBIO", kwnames)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(type));
    return_value = _ssl_MemoryBIO_impl(type);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_ssl_MemoryBIO(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_ssl_MemoryBIO_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _ssl_MemoryBIO_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_ssl_MemoryBIO_pending__doc__,
"The number of bytes pending in the memory BIO.");

static PyObject *
_ssl_MemoryBIO_pending_get_impl(PySSLMemoryBIO *self);

static PyObject *
_ssl_MemoryBIO_pending_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_pending_get_impl((PySSLMemoryBIO *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_eof__doc__,
"Whether the memory BIO is at EOF.");

static PyObject *
_ssl_MemoryBIO_eof_get_impl(PySSLMemoryBIO *self);

static PyObject *
_ssl_MemoryBIO_eof_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_MemoryBIO_eof_get_impl((PySSLMemoryBIO *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read up to size bytes from the memory BIO.\n"
"\n"
"If size is not specified, read the entire buffer.\n"
"If the return value is an empty bytes instance, this means either\n"
"EOF or that no data is available. Use the \"eof\" property to\n"
"distinguish between the two.");

#define _SSL_MEMORYBIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_ssl_MemoryBIO_read), METH_VALUE, _ssl_MemoryBIO_read__doc__},

static PyValue
_ssl_MemoryBIO_read_impl(PySSLMemoryBIO *self, int len);

static PyValue
_ssl_MemoryBIO_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int len = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    len = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (len == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl_MemoryBIO_read_impl((PySSLMemoryBIO *)PyValue_AsKnownObjectBorrow(self), len);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Writes the bytes b into the memory BIO.\n"
"\n"
"Returns the number of bytes written.");

#define _SSL_MEMORYBIO_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_ssl_MemoryBIO_write), METH_VALUE|METH_O, _ssl_MemoryBIO_write__doc__},

static PyValue
_ssl_MemoryBIO_write_impl(PySSLMemoryBIO *self, Py_buffer *b);

static PyValue
_ssl_MemoryBIO_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer b = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl_MemoryBIO_write_impl((PySSLMemoryBIO *)PyValue_AsKnownObjectBorrow(self), &b);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_MemoryBIO_write_eof__doc__,
"write_eof($self, /)\n"
"--\n"
"\n"
"Write an EOF marker to the memory BIO.\n"
"\n"
"When all data has been read, the \"eof\" property will be True.");

#define _SSL_MEMORYBIO_WRITE_EOF_METHODDEF    \
    {"write_eof", _PyCFunction_CAST(_ssl_MemoryBIO_write_eof), METH_VALUE|METH_NOARGS, _ssl_MemoryBIO_write_eof__doc__},

static PyValue
_ssl_MemoryBIO_write_eof_impl(PySSLMemoryBIO *self);

static PyValue
_ssl_MemoryBIO_write_eof(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _ssl_MemoryBIO_write_eof_impl((PySSLMemoryBIO *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_time__doc__,
"Session creation time (seconds since epoch).");

static PyObject *
_ssl_SSLSession_time_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_time_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_time_get_impl((PySSLSession *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_timeout__doc__,
"Session timeout (delta in seconds).");

static PyObject *
_ssl_SSLSession_timeout_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_timeout_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_timeout_get_impl((PySSLSession *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_ticket_lifetime_hint__doc__,
"Ticket life time hint.");

static PyObject *
_ssl_SSLSession_ticket_lifetime_hint_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_ticket_lifetime_hint_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_ticket_lifetime_hint_get_impl((PySSLSession *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_id__doc__,
"Session ID.");

static PyObject *
_ssl_SSLSession_id_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_id_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_id_get_impl((PySSLSession *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_SSLSession_has_ticket__doc__,
"Does the session contain a ticket?");

static PyObject *
_ssl_SSLSession_has_ticket_get_impl(PySSLSession *self);

static PyObject *
_ssl_SSLSession_has_ticket_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _ssl_SSLSession_has_ticket_get_impl((PySSLSession *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_add__doc__,
"RAND_add($module, string, entropy, /)\n"
"--\n"
"\n"
"Mix string into the OpenSSL PRNG state.\n"
"\n"
"entropy (a float) is a lower bound on the entropy contained in\n"
"string.  See RFC 4086.");

#define _SSL_RAND_ADD_METHODDEF    \
    {"RAND_add", _PyCFunction_CAST(_ssl_RAND_add), METH_VALUE, _ssl_RAND_add__doc__},

static PyValue
_ssl_RAND_add_impl(PyObject *module, Py_buffer *view, double entropy);

static PyValue
_ssl_RAND_add(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer view = {NULL, NULL};
    double entropy;

    if (!_PyArg_CheckPositional("RAND_add", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&view, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &view, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    entropy = _PyValue_AsDouble(PyValue_Borrow(args[1]));
    if (entropy == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_RAND_add_impl(PyValue_AsKnownObjectBorrow(module), &view, entropy);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_bytes__doc__,
"RAND_bytes($module, n, /)\n"
"--\n"
"\n"
"Generate n cryptographically strong pseudo-random bytes.");

#define _SSL_RAND_BYTES_METHODDEF    \
    {"RAND_bytes", _PyCFunction_CAST(_ssl_RAND_bytes), METH_VALUE|METH_O, _ssl_RAND_bytes__doc__},

static PyValue
_ssl_RAND_bytes_impl(PyObject *module, int n);

static PyValue
_ssl_RAND_bytes(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int n;

    n = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_RAND_bytes_impl(PyValue_AsKnownObjectBorrow(module), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_RAND_status__doc__,
"RAND_status($module, /)\n"
"--\n"
"\n"
"Returns True if the OpenSSL PRNG has been seeded with enough data and False if not.\n"
"\n"
"It is necessary to seed the PRNG with RAND_add() on some platforms\n"
"before using the ssl() function.");

#define _SSL_RAND_STATUS_METHODDEF    \
    {"RAND_status", _PyCFunction_CAST(_ssl_RAND_status), METH_VALUE|METH_NOARGS, _ssl_RAND_status__doc__},

static PyValue
_ssl_RAND_status_impl(PyObject *module);

static PyValue
_ssl_RAND_status(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_RAND_status_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_get_default_verify_paths__doc__,
"get_default_verify_paths($module, /)\n"
"--\n"
"\n"
"Return search paths and environment vars that are used by SSLContext\'s set_default_verify_paths() to load default CAs.\n"
"\n"
"The values are \'cert_file_env\', \'cert_file\', \'cert_dir_env\', \'cert_dir\'.");

#define _SSL_GET_DEFAULT_VERIFY_PATHS_METHODDEF    \
    {"get_default_verify_paths", _PyCFunction_CAST(_ssl_get_default_verify_paths), METH_VALUE|METH_NOARGS, _ssl_get_default_verify_paths__doc__},

static PyValue
_ssl_get_default_verify_paths_impl(PyObject *module);

static PyValue
_ssl_get_default_verify_paths(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_get_default_verify_paths_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_ssl_get_sigalgs__doc__,
"get_sigalgs($module, /)\n"
"--\n"
"\n");

#define _SSL_GET_SIGALGS_METHODDEF    \
    {"get_sigalgs", _PyCFunction_CAST(_ssl_get_sigalgs), METH_VALUE|METH_NOARGS, _ssl_get_sigalgs__doc__},

static PyValue
_ssl_get_sigalgs_impl(PyObject *module);

static PyValue
_ssl_get_sigalgs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _ssl_get_sigalgs_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_ssl_txt2obj__doc__,
"txt2obj($module, /, txt, name=False)\n"
"--\n"
"\n"
"Lookup NID, short name, long name and OID of an ASN1_OBJECT.\n"
"\n"
"By default objects are looked up by OID. With name=True short and\n"
"long name are also matched.");

#define _SSL_TXT2OBJ_METHODDEF    \
    {"txt2obj", _PyCFunction_CAST(_ssl_txt2obj), METH_VALUE|METH_KEYWORDS, _ssl_txt2obj__doc__},

static PyValue
_ssl_txt2obj_impl(PyObject *module, const char *txt, int name);

static PyValue
_ssl_txt2obj(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(txt)), _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"txt", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "txt2obj",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    const char *txt;
    int name = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("txt2obj", "argument 'txt'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("txt2obj", "argument 'txt'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    txt = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (txt == NULL) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        name = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (name < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_txt2obj_impl(PyValue_AsKnownObjectBorrow(module), txt, name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_nid2obj__doc__,
"nid2obj($module, nid, /)\n"
"--\n"
"\n"
"Lookup NID, short name, long name and OID of an ASN1_OBJECT by NID.");

#define _SSL_NID2OBJ_METHODDEF    \
    {"nid2obj", _PyCFunction_CAST(_ssl_nid2obj), METH_VALUE|METH_O, _ssl_nid2obj__doc__},

static PyValue
_ssl_nid2obj_impl(PyObject *module, int nid);

static PyValue
_ssl_nid2obj(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int nid;

    nid = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (nid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_nid2obj_impl(PyValue_AsKnownObjectBorrow(module), nid);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if defined(_MSC_VER)

PyDoc_STRVAR(_ssl_enum_certificates__doc__,
"enum_certificates($module, /, store_name)\n"
"--\n"
"\n"
"Retrieve certificates from Windows\' cert store.\n"
"\n"
"store_name may be one of \'CA\', \'ROOT\' or \'MY\'.  The system may\n"
"provide more cert storages, too.  The function returns a list of\n"
"(bytes, encoding_type, trust) tuples.  The encoding_type flag can be\n"
"interpreted with X509_ASN_ENCODING or PKCS_7_ASN_ENCODING.  The\n"
"trust setting is either a set of OIDs or the boolean True.");

#define _SSL_ENUM_CERTIFICATES_METHODDEF    \
    {"enum_certificates", _PyCFunction_CAST(_ssl_enum_certificates), METH_VALUE|METH_KEYWORDS, _ssl_enum_certificates__doc__},

static PyValue
_ssl_enum_certificates_impl(PyObject *module, const char *store_name);

static PyValue
_ssl_enum_certificates(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(store_name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"store_name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enum_certificates",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *store_name;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("enum_certificates", "argument 'store_name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("enum_certificates", "argument 'store_name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    store_name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (store_name == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_enum_certificates_impl(PyValue_AsKnownObjectBorrow(module), store_name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(_MSC_VER) */

#if defined(_MSC_VER)

PyDoc_STRVAR(_ssl_enum_crls__doc__,
"enum_crls($module, /, store_name)\n"
"--\n"
"\n"
"Retrieve CRLs from Windows\' cert store.\n"
"\n"
"store_name may be one of \'CA\', \'ROOT\' or \'MY\'.  The system may provide\n"
"more cert storages, too.  The function returns a list of (bytes,\n"
"encoding_type) tuples.  The encoding_type flag can be interpreted with\n"
"X509_ASN_ENCODING or PKCS_7_ASN_ENCODING.");

#define _SSL_ENUM_CRLS_METHODDEF    \
    {"enum_crls", _PyCFunction_CAST(_ssl_enum_crls), METH_VALUE|METH_KEYWORDS, _ssl_enum_crls__doc__},

static PyValue
_ssl_enum_crls_impl(PyObject *module, const char *store_name);

static PyValue
_ssl_enum_crls(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(store_name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"store_name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "enum_crls",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *store_name;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("enum_crls", "argument 'store_name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("enum_crls", "argument 'store_name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    store_name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (store_name == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _ssl_enum_crls_impl(PyValue_AsKnownObjectBorrow(module), store_name);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(_MSC_VER) */

#ifndef _SSL__SSLSOCKET_SENDFILE_METHODDEF
    #define _SSL__SSLSOCKET_SENDFILE_METHODDEF
#endif /* !defined(_SSL__SSLSOCKET_SENDFILE_METHODDEF) */

#ifndef _SSL_ENUM_CERTIFICATES_METHODDEF
    #define _SSL_ENUM_CERTIFICATES_METHODDEF
#endif /* !defined(_SSL_ENUM_CERTIFICATES_METHODDEF) */

#ifndef _SSL_ENUM_CRLS_METHODDEF
    #define _SSL_ENUM_CRLS_METHODDEF
#endif /* !defined(_SSL_ENUM_CRLS_METHODDEF) */
#define _SSL__SSLSOCKET_CONTEXT_GETSETDEF {"context", (getter)_ssl__SSLSocket_context_get, (setter)_ssl__SSLSocket_context_set, _ssl__SSLSocket_context__doc__},

#define _SSL__SSLSOCKET_SERVER_SIDE_GETSETDEF {"server_side", (getter)_ssl__SSLSocket_server_side_get, (setter)NULL, _ssl__SSLSocket_server_side__doc__},

#define _SSL__SSLSOCKET_SERVER_HOSTNAME_GETSETDEF {"server_hostname", (getter)_ssl__SSLSocket_server_hostname_get, (setter)NULL, _ssl__SSLSocket_server_hostname__doc__},

#define _SSL__SSLSOCKET_OWNER_GETSETDEF {"owner", (getter)_ssl__SSLSocket_owner_get, (setter)_ssl__SSLSocket_owner_set, _ssl__SSLSocket_owner__doc__},

#define _SSL__SSLSOCKET_SESSION_GETSETDEF {"session", (getter)_ssl__SSLSocket_session_get, (setter)_ssl__SSLSocket_session_set, _ssl__SSLSocket_session__doc__},

#define _SSL__SSLSOCKET_SESSION_REUSED_GETSETDEF {"session_reused", (getter)_ssl__SSLSocket_session_reused_get, (setter)NULL, _ssl__SSLSocket_session_reused__doc__},

#define _SSL__SSLCONTEXT_VERIFY_MODE_GETSETDEF {"verify_mode", (getter)_ssl__SSLContext_verify_mode_get, (setter)_ssl__SSLContext_verify_mode_set, NULL},

#define _SSL__SSLCONTEXT_VERIFY_FLAGS_GETSETDEF {"verify_flags", (getter)_ssl__SSLContext_verify_flags_get, (setter)_ssl__SSLContext_verify_flags_set, NULL},

#define _SSL__SSLCONTEXT_MINIMUM_VERSION_GETSETDEF {"minimum_version", (getter)_ssl__SSLContext_minimum_version_get, (setter)_ssl__SSLContext_minimum_version_set, NULL},

#define _SSL__SSLCONTEXT_MAXIMUM_VERSION_GETSETDEF {"maximum_version", (getter)_ssl__SSLContext_maximum_version_get, (setter)_ssl__SSLContext_maximum_version_set, NULL},

#define _SSL__SSLCONTEXT_NUM_TICKETS_GETSETDEF {"num_tickets", (getter)_ssl__SSLContext_num_tickets_get, (setter)_ssl__SSLContext_num_tickets_set, _ssl__SSLContext_num_tickets__doc__},

#define _SSL__SSLCONTEXT_SECURITY_LEVEL_GETSETDEF {"security_level", (getter)_ssl__SSLContext_security_level_get, (setter)NULL, _ssl__SSLContext_security_level__doc__},

#define _SSL__SSLCONTEXT_OPTIONS_GETSETDEF {"options", (getter)_ssl__SSLContext_options_get, (setter)_ssl__SSLContext_options_set, NULL},

#define _SSL__SSLCONTEXT__HOST_FLAGS_GETSETDEF {"_host_flags", (getter)_ssl__SSLContext__host_flags_get, (setter)_ssl__SSLContext__host_flags_set, NULL},

#define _SSL__SSLCONTEXT_CHECK_HOSTNAME_GETSETDEF {"check_hostname", (getter)_ssl__SSLContext_check_hostname_get, (setter)_ssl__SSLContext_check_hostname_set, NULL},

#define _SSL__SSLCONTEXT_PROTOCOL_GETSETDEF {"protocol", (getter)_ssl__SSLContext_protocol_get, (setter)NULL, NULL},

#define _SSL__SSLCONTEXT_SNI_CALLBACK_GETSETDEF {"sni_callback", (getter)_ssl__SSLContext_sni_callback_get, (setter)_ssl__SSLContext_sni_callback_set, _ssl__SSLContext_sni_callback__doc__},

#define _SSL_MEMORYBIO_PENDING_GETSETDEF {"pending", (getter)_ssl_MemoryBIO_pending_get, (setter)NULL, _ssl_MemoryBIO_pending__doc__},

#define _SSL_MEMORYBIO_EOF_GETSETDEF {"eof", (getter)_ssl_MemoryBIO_eof_get, (setter)NULL, _ssl_MemoryBIO_eof__doc__},

#define _SSL_SSLSESSION_TIME_GETSETDEF {"time", (getter)_ssl_SSLSession_time_get, (setter)NULL, _ssl_SSLSession_time__doc__},

#define _SSL_SSLSESSION_TIMEOUT_GETSETDEF {"timeout", (getter)_ssl_SSLSession_timeout_get, (setter)NULL, _ssl_SSLSession_timeout__doc__},

#define _SSL_SSLSESSION_TICKET_LIFETIME_HINT_GETSETDEF {"ticket_lifetime_hint", (getter)_ssl_SSLSession_ticket_lifetime_hint_get, (setter)NULL, _ssl_SSLSession_ticket_lifetime_hint__doc__},

#define _SSL_SSLSESSION_ID_GETSETDEF {"id", (getter)_ssl_SSLSession_id_get, (setter)NULL, _ssl_SSLSession_id__doc__},

#define _SSL_SSLSESSION_HAS_TICKET_GETSETDEF {"has_ticket", (getter)_ssl_SSLSession_has_ticket_get, (setter)NULL, _ssl_SSLSession_has_ticket__doc__},

/*[clinic end generated code: output=19a11f8110ccffea input=a9049054013a1b77]*/
