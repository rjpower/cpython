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

#if defined(HAVE_SHM_OPEN)

PyDoc_STRVAR(_posixshmem_shm_open__doc__,
"shm_open($module, /, path, flags, mode=511)\n"
"--\n"
"\n"
"Open a shared memory object.  Returns a file descriptor (integer).");

#define _POSIXSHMEM_SHM_OPEN_METHODDEF    \
    {"shm_open", _PyCFunction_CAST(_posixshmem_shm_open), METH_VALUE|METH_KEYWORDS, _posixshmem_shm_open__doc__},

static int
_posixshmem_shm_open_impl(PyObject *module, PyObject *path, int flags,
                          int mode);

static PyValue
_posixshmem_shm_open(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(path)), _PyValue_STATIC_IMMORTAL(&_Py_ID(flags)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "flags", "mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "shm_open",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyObject *path;
    int flags;
    int mode = 511;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("shm_open", "argument 'path'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("shm_open", "argument 'path'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    path = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    flags = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        mode = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _posixshmem_shm_open_impl(PyValue_AsKnownObjectBorrow(module), path, flags, mode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_OPEN) */

#if defined(HAVE_SHM_UNLINK)

PyDoc_STRVAR(_posixshmem_shm_unlink__doc__,
"shm_unlink($module, path, /)\n"
"--\n"
"\n"
"Remove a shared memory object (similar to unlink()).\n"
"\n"
"Remove a shared memory object name, and, once all processes have\n"
"unmapped the object, de-allocates and destroys the contents of the\n"
"associated memory region.");

#define _POSIXSHMEM_SHM_UNLINK_METHODDEF    \
    {"shm_unlink", _PyCFunction_CAST(_posixshmem_shm_unlink), METH_VALUE|METH_O, _posixshmem_shm_unlink__doc__},

static PyValue
_posixshmem_shm_unlink_impl(PyObject *module, PyObject *path);

static PyValue
_posixshmem_shm_unlink(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *path;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("shm_unlink", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("shm_unlink", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    path = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _posixshmem_shm_unlink_impl(PyValue_AsKnownObjectBorrow(module), path);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_UNLINK) */

#ifndef _POSIXSHMEM_SHM_OPEN_METHODDEF
    #define _POSIXSHMEM_SHM_OPEN_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_OPEN_METHODDEF) */

#ifndef _POSIXSHMEM_SHM_UNLINK_METHODDEF
    #define _POSIXSHMEM_SHM_UNLINK_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_UNLINK_METHODDEF) */
/*[clinic end generated code: output=b338bb7cf9e5d601 input=a9049054013a1b77]*/
