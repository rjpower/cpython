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
#include "pycore_long_value.h"    // _PyValue_AsNativeBytes()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(zoneinfo_ZoneInfo__doc__,
"ZoneInfo(key)\n"
"--\n"
"\n"
"Create a new ZoneInfo instance.");

static PyValue
zoneinfo_ZoneInfo_impl(PyTypeObject *type, PyValueBorrow key);

static PyValue
zoneinfo_ZoneInfo_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ZoneInfo",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow key;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(type));
    return_value = zoneinfo_ZoneInfo_impl(type, key);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
zoneinfo_ZoneInfo(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(zoneinfo_ZoneInfo_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = zoneinfo_ZoneInfo_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_from_file__doc__,
"from_file($type, file_obj, /, key=None)\n"
"--\n"
"\n"
"Create a ZoneInfo file from a file object.");

#define ZONEINFO_ZONEINFO_FROM_FILE_METHODDEF    \
    {"from_file", _PyCFunction_CAST(zoneinfo_ZoneInfo_from_file), METH_VALUE|METH_METHOD|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_from_file__doc__},

static PyValue
zoneinfo_ZoneInfo_from_file_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyValueBorrow file_obj, PyValueBorrow key);

static PyValue
zoneinfo_ZoneInfo_from_file(PyValueBorrow type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_file",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow file_obj;
    PyValueBorrow key = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    file_obj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        key = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = zoneinfo_ZoneInfo_from_file_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), cls, file_obj, key);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_no_cache__doc__,
"no_cache($type, /, key)\n"
"--\n"
"\n"
"Get a new instance of ZoneInfo, bypassing the cache.");

#define ZONEINFO_ZONEINFO_NO_CACHE_METHODDEF    \
    {"no_cache", _PyCFunction_CAST(zoneinfo_ZoneInfo_no_cache), METH_VALUE|METH_METHOD|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_no_cache__doc__},

static PyValue
zoneinfo_ZoneInfo_no_cache_impl(PyTypeObject *type, PyTypeObject *cls,
                                PyValueBorrow key);

static PyValue
zoneinfo_ZoneInfo_no_cache(PyValueBorrow type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "no_cache",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow key;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    return_value = zoneinfo_ZoneInfo_no_cache_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), cls, key);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_clear_cache__doc__,
"clear_cache($type, /, *, only_keys=None)\n"
"--\n"
"\n"
"Clear the ZoneInfo cache.");

#define ZONEINFO_ZONEINFO_CLEAR_CACHE_METHODDEF    \
    {"clear_cache", _PyCFunction_CAST(zoneinfo_ZoneInfo_clear_cache), METH_VALUE|METH_METHOD|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_clear_cache__doc__},

static PyValue
zoneinfo_ZoneInfo_clear_cache_impl(PyTypeObject *type, PyTypeObject *cls,
                                   PyValueBorrow only_keys);

static PyValue
zoneinfo_ZoneInfo_clear_cache(PyValueBorrow type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(only_keys)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"only_keys", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "clear_cache",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow only_keys = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        only_keys = PyValue_Borrow(args[_idx[0]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(type));
    return_value = zoneinfo_ZoneInfo_clear_cache_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), cls, only_keys);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_utcoffset__doc__,
"utcoffset($self, dt, /)\n"
"--\n"
"\n"
"Retrieve a timedelta representing the UTC offset in a zone at the given datetime.");

#define ZONEINFO_ZONEINFO_UTCOFFSET_METHODDEF    \
    {"utcoffset", _PyCFunction_CAST(zoneinfo_ZoneInfo_utcoffset), METH_VALUE|METH_METHOD|METH_KEYWORDS, zoneinfo_ZoneInfo_utcoffset__doc__},

static PyValue
zoneinfo_ZoneInfo_utcoffset_impl(PyObject *self, PyTypeObject *cls,
                                 PyValueBorrow dt);

static PyValue
zoneinfo_ZoneInfo_utcoffset(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow dt;

    if (!_PyArg_NoKwnames("utcoffset", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("utcoffset", nargs, 1, 1)) {
        goto exit;
    }
    dt = PyValue_Borrow(args[0]);
    return_value = zoneinfo_ZoneInfo_utcoffset_impl(PyValue_AsKnownObjectBorrow(self), cls, dt);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_dst__doc__,
"dst($self, dt, /)\n"
"--\n"
"\n"
"Retrieve a timedelta representing the amount of DST applied in a zone at the given datetime.");

#define ZONEINFO_ZONEINFO_DST_METHODDEF    \
    {"dst", _PyCFunction_CAST(zoneinfo_ZoneInfo_dst), METH_VALUE|METH_METHOD|METH_KEYWORDS, zoneinfo_ZoneInfo_dst__doc__},

static PyValue
zoneinfo_ZoneInfo_dst_impl(PyObject *self, PyTypeObject *cls,
                           PyValueBorrow dt);

static PyValue
zoneinfo_ZoneInfo_dst(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow dt;

    if (!_PyArg_NoKwnames("dst", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("dst", nargs, 1, 1)) {
        goto exit;
    }
    dt = PyValue_Borrow(args[0]);
    return_value = zoneinfo_ZoneInfo_dst_impl(PyValue_AsKnownObjectBorrow(self), cls, dt);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_tzname__doc__,
"tzname($self, dt, /)\n"
"--\n"
"\n"
"Retrieve a string containing the abbreviation for the time zone that applies in a zone at a given datetime.");

#define ZONEINFO_ZONEINFO_TZNAME_METHODDEF    \
    {"tzname", _PyCFunction_CAST(zoneinfo_ZoneInfo_tzname), METH_VALUE|METH_METHOD|METH_KEYWORDS, zoneinfo_ZoneInfo_tzname__doc__},

static PyValue
zoneinfo_ZoneInfo_tzname_impl(PyObject *self, PyTypeObject *cls,
                              PyValueBorrow dt);

static PyValue
zoneinfo_ZoneInfo_tzname(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow dt;

    if (!_PyArg_NoKwnames("tzname", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tzname", nargs, 1, 1)) {
        goto exit;
    }
    dt = PyValue_Borrow(args[0]);
    return_value = zoneinfo_ZoneInfo_tzname_impl(PyValue_AsKnownObjectBorrow(self), cls, dt);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo__unpickle__doc__,
"_unpickle($type, key, from_cache, /)\n"
"--\n"
"\n"
"Private method used in unpickling.");

#define ZONEINFO_ZONEINFO__UNPICKLE_METHODDEF    \
    {"_unpickle", _PyCFunction_CAST(zoneinfo_ZoneInfo__unpickle), METH_VALUE|METH_METHOD|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo__unpickle__doc__},

static PyValue
zoneinfo_ZoneInfo__unpickle_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyValueBorrow key, unsigned char from_cache);

static PyValue
zoneinfo_ZoneInfo__unpickle(PyValueBorrow type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    unsigned char from_cache;

    if (!_PyArg_NoKwnames("_unpickle", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_unpickle", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &from_cache, sizeof(unsigned char),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned char)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = zoneinfo_ZoneInfo__unpickle_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), cls, key, from_cache);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo___init_subclass____doc__,
"__init_subclass__($type, /, **kwargs)\n"
"--\n"
"\n"
"Function to initialize subclasses.");

#define ZONEINFO_ZONEINFO___INIT_SUBCLASS___METHODDEF    \
    {"__init_subclass__", _PyCFunction_CAST(zoneinfo_ZoneInfo___init_subclass__), METH_VALUE|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo___init_subclass____doc__},

static PyValue
zoneinfo_ZoneInfo___init_subclass___impl(PyTypeObject *type,
                                         PyObject *kwargs);

static PyValue
zoneinfo_ZoneInfo___init_subclass__(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("__init_subclass__", nargs, 0, 0)) {
        goto exit;
    }
    __clinic_kwargs = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (__clinic_kwargs == NULL) {
        goto exit;
    }
    return_value = zoneinfo_ZoneInfo___init_subclass___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), __clinic_kwargs);

exit:
    /* Cleanup for kwargs */
    Py_XDECREF(__clinic_kwargs);

    return return_value;
}
/*[clinic end generated code: output=59e6ba0af0725f94 input=a9049054013a1b77]*/
