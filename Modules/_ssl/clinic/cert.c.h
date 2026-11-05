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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_ssl_Certificate_public_bytes__doc__,
"public_bytes($self, /, format=Encoding.PEM)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_PUBLIC_BYTES_METHODDEF    \
    {"public_bytes", _PyCFunction_CAST(_ssl_Certificate_public_bytes), METH_VALUE|METH_KEYWORDS, _ssl_Certificate_public_bytes__doc__},

static PyValue
_ssl_Certificate_public_bytes_impl(PySSLCertificate *self, int format);

static PyValue
_ssl_Certificate_public_bytes(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "public_bytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int format = PY_SSL_ENCODING_PEM;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        format = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (format == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _ssl_Certificate_public_bytes_impl((PySSLCertificate *)PyValue_AsKnownObjectBorrow(self), format);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_Certificate_get_info__doc__,
"get_info($self, /)\n"
"--\n"
"\n");

#define _SSL_CERTIFICATE_GET_INFO_METHODDEF    \
    {"get_info", _PyCFunction_CAST(_ssl_Certificate_get_info), METH_VALUE|METH_NOARGS, _ssl_Certificate_get_info__doc__},

static PyValue
_ssl_Certificate_get_info_impl(PySSLCertificate *self);

static PyValue
_ssl_Certificate_get_info(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _ssl_Certificate_get_info_impl((PySSLCertificate *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=6f9933d51d8487fd input=a9049054013a1b77]*/
