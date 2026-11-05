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
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(marshal_dump__doc__,
"dump($module, value, file, version=version, /, *, allow_code=True)\n"
"--\n"
"\n"
"Write the value on the open file.\n"
"\n"
"  value\n"
"    Must be a supported type.\n"
"  file\n"
"    Must be a writeable binary file.\n"
"  version\n"
"    Indicates the data format that dump should use.\n"
"  allow_code\n"
"    Allow to write code objects.\n"
"\n"
"If the value has (or contains an object that has) an unsupported type, a\n"
"ValueError exception is raised - but garbage data will also be written\n"
"to the file. The object will not be properly read back by load().");

#define MARSHAL_DUMP_METHODDEF    \
    {"dump", _PyCFunction_CAST(marshal_dump), METH_VALUE|METH_KEYWORDS, marshal_dump__doc__},

static PyValue
marshal_dump_impl(PyObject *module, PyValueBorrow value, PyValueBorrow file,
                  int version, int allow_code);

static PyValue
marshal_dump(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(allow_code)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "", "allow_code", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dump",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow value;
    PyValueBorrow file;
    int version = Py_MARSHAL_VERSION;
    int allow_code = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    value = PyValue_Borrow(args[_idx[0]]);
    file = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        version = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (version == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        allow_code = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (allow_code < 0) {
            goto exit;
        }
    }
    return_value = marshal_dump_impl(PyValue_AsKnownObjectBorrow(module), value, file, version, allow_code);

exit:
    return return_value;
}

PyDoc_STRVAR(marshal_load__doc__,
"load($module, file, /, *, allow_code=True)\n"
"--\n"
"\n"
"Read one value from the open file and return it.\n"
"\n"
"  file\n"
"    Must be readable binary file.\n"
"  allow_code\n"
"    Allow to load code objects.\n"
"\n"
"If no valid value is read (e.g. because the data has a different Python\n"
"version\'s incompatible marshal format), raise EOFError, ValueError or\n"
"TypeError.\n"
"\n"
"Note: If an object containing an unsupported type was marshalled with\n"
"dump(), load() will substitute None for the unmarshallable type.");

#define MARSHAL_LOAD_METHODDEF    \
    {"load", _PyCFunction_CAST(marshal_load), METH_VALUE|METH_KEYWORDS, marshal_load__doc__},

static PyValue
marshal_load_impl(PyObject *module, PyValueBorrow file, int allow_code);

static PyValue
marshal_load(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(allow_code)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "allow_code", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "load",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow file;
    int allow_code = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    file = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        allow_code = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (allow_code < 0) {
            goto exit;
        }
    }
    return_value = marshal_load_impl(PyValue_AsKnownObjectBorrow(module), file, allow_code);

exit:
    return return_value;
}

PyDoc_STRVAR(marshal_dumps__doc__,
"dumps($module, value, version=version, /, *, allow_code=True)\n"
"--\n"
"\n"
"Return the bytes object that would be written to a file by dump(value, file).\n"
"\n"
"  value\n"
"    Must be a supported type.\n"
"  version\n"
"    Indicates the data format that dumps should use.\n"
"  allow_code\n"
"    Allow to write code objects.\n"
"\n"
"Raise a ValueError exception if value has (or contains an object that\n"
"has) an unsupported type.");

#define MARSHAL_DUMPS_METHODDEF    \
    {"dumps", _PyCFunction_CAST(marshal_dumps), METH_VALUE|METH_KEYWORDS, marshal_dumps__doc__},

static PyValue
marshal_dumps_impl(PyObject *module, PyValueBorrow value, int version,
                   int allow_code);

static PyValue
marshal_dumps(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(allow_code)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "allow_code", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "dumps",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow value;
    int version = Py_MARSHAL_VERSION;
    int allow_code = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    value = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        version = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (version == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        allow_code = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (allow_code < 0) {
            goto exit;
        }
    }
    return_value = marshal_dumps_impl(PyValue_AsKnownObjectBorrow(module), value, version, allow_code);

exit:
    return return_value;
}

PyDoc_STRVAR(marshal_loads__doc__,
"loads($module, bytes, /, *, allow_code=True)\n"
"--\n"
"\n"
"Convert the bytes-like object to a value.\n"
"\n"
"  allow_code\n"
"    Allow to load code objects.\n"
"\n"
"If no valid value is found, raise EOFError, ValueError or TypeError.\n"
"Extra bytes in the input are ignored.");

#define MARSHAL_LOADS_METHODDEF    \
    {"loads", _PyCFunction_CAST(marshal_loads), METH_VALUE|METH_KEYWORDS, marshal_loads__doc__},

static PyValue
marshal_loads_impl(PyObject *module, Py_buffer *bytes, int allow_code);

static PyValue
marshal_loads(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(allow_code)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "allow_code", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "loads",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer bytes = {NULL, NULL};
    int allow_code = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &bytes, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        allow_code = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (allow_code < 0) {
            goto exit;
        }
    }
    return_value = marshal_loads_impl(PyValue_AsKnownObjectBorrow(module), &bytes, allow_code);

exit:
    /* Cleanup for bytes */
    if (bytes.obj) {
       PyBuffer_Release(&bytes);
    }

    return return_value;
}
/*[clinic end generated code: output=ba99b8c50384fe25 input=a9049054013a1b77]*/
