/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(zoneinfo_ZoneInfo__doc__,
"ZoneInfo(key)\n"
"--\n"
"\n"
"Create a new ZoneInfo instance.");

static PyObject *
zoneinfo_ZoneInfo_impl(PyTypeObject *type, PyObject *key);

static PyObject *
zoneinfo_ZoneInfo(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(key), },
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
    PyValue argsbuf[1];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *key;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    key = PyValue_Object(fastargs[0]);
    if (key == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(type);
    return_value = zoneinfo_ZoneInfo_impl(type, key);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_from_file__doc__,
"from_file($type, file_obj, /, key=None)\n"
"--\n"
"\n"
"Create a ZoneInfo file from a file object.");

#define ZONEINFO_ZONEINFO_FROM_FILE_METHODDEF    \
    {"from_file", _PyCFunction_CAST(zoneinfo_ZoneInfo_from_file), METH_METHOD|METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_from_file__doc__},

static PyObject *
zoneinfo_ZoneInfo_from_file_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyObject *file_obj, PyObject *key);

static PyValue
zoneinfo_ZoneInfo_from_file(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(key), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *file_obj;
    PyObject *key = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file_obj = PyValue_Object(args[0]);
    if (file_obj == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    key = PyValue_Object(args[1]);
    if (key == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = zoneinfo_ZoneInfo_from_file_impl((PyTypeObject *)type, cls, file_obj, key);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_no_cache__doc__,
"no_cache($type, /, key)\n"
"--\n"
"\n"
"Get a new instance of ZoneInfo, bypassing the cache.");

#define ZONEINFO_ZONEINFO_NO_CACHE_METHODDEF    \
    {"no_cache", _PyCFunction_CAST(zoneinfo_ZoneInfo_no_cache), METH_METHOD|METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_no_cache__doc__},

static PyObject *
zoneinfo_ZoneInfo_no_cache_impl(PyTypeObject *type, PyTypeObject *cls,
                                PyObject *key);

static PyValue
zoneinfo_ZoneInfo_no_cache(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(key), },
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
    PyValue argsbuf[1];
    PyObject *key;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = PyValue_Object(args[0]);
    if (key == NULL) {
        goto exit;
    }
    _return_value = zoneinfo_ZoneInfo_no_cache_impl((PyTypeObject *)type, cls, key);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_clear_cache__doc__,
"clear_cache($type, /, *, only_keys=None)\n"
"--\n"
"\n"
"Clear the ZoneInfo cache.");

#define ZONEINFO_ZONEINFO_CLEAR_CACHE_METHODDEF    \
    {"clear_cache", _PyCFunction_CAST(zoneinfo_ZoneInfo_clear_cache), METH_METHOD|METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_clear_cache__doc__},

static PyObject *
zoneinfo_ZoneInfo_clear_cache_impl(PyTypeObject *type, PyTypeObject *cls,
                                   PyObject *only_keys);

static PyValue
zoneinfo_ZoneInfo_clear_cache(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(only_keys), },
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
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *only_keys = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    only_keys = PyValue_Object(args[0]);
    if (only_keys == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    Py_BEGIN_CRITICAL_SECTION(type);
    _return_value = zoneinfo_ZoneInfo_clear_cache_impl((PyTypeObject *)type, cls, only_keys);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_utcoffset__doc__,
"utcoffset($self, dt, /)\n"
"--\n"
"\n"
"Retrieve a timedelta representing the UTC offset in a zone at the given datetime.");

#define ZONEINFO_ZONEINFO_UTCOFFSET_METHODDEF    \
    {"utcoffset", _PyCFunction_CAST(zoneinfo_ZoneInfo_utcoffset), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zoneinfo_ZoneInfo_utcoffset__doc__},

static PyObject *
zoneinfo_ZoneInfo_utcoffset_impl(PyObject *self, PyTypeObject *cls,
                                 PyObject *dt);

static PyValue
zoneinfo_ZoneInfo_utcoffset(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dt;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("utcoffset", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("utcoffset", nargs, 1, 1)) {
        goto exit;
    }
    dt = PyValue_Object(args[0]);
    if (dt == NULL) {
        goto exit;
    }
    _return_value = zoneinfo_ZoneInfo_utcoffset_impl(self, cls, dt);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_dst__doc__,
"dst($self, dt, /)\n"
"--\n"
"\n"
"Retrieve a timedelta representing the amount of DST applied in a zone at the given datetime.");

#define ZONEINFO_ZONEINFO_DST_METHODDEF    \
    {"dst", _PyCFunction_CAST(zoneinfo_ZoneInfo_dst), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zoneinfo_ZoneInfo_dst__doc__},

static PyObject *
zoneinfo_ZoneInfo_dst_impl(PyObject *self, PyTypeObject *cls, PyObject *dt);

static PyValue
zoneinfo_ZoneInfo_dst(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dt;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("dst", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("dst", nargs, 1, 1)) {
        goto exit;
    }
    dt = PyValue_Object(args[0]);
    if (dt == NULL) {
        goto exit;
    }
    _return_value = zoneinfo_ZoneInfo_dst_impl(self, cls, dt);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_tzname__doc__,
"tzname($self, dt, /)\n"
"--\n"
"\n"
"Retrieve a string containing the abbreviation for the time zone that applies in a zone at a given datetime.");

#define ZONEINFO_ZONEINFO_TZNAME_METHODDEF    \
    {"tzname", _PyCFunction_CAST(zoneinfo_ZoneInfo_tzname), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zoneinfo_ZoneInfo_tzname__doc__},

static PyObject *
zoneinfo_ZoneInfo_tzname_impl(PyObject *self, PyTypeObject *cls,
                              PyObject *dt);

static PyValue
zoneinfo_ZoneInfo_tzname(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dt;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("tzname", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tzname", nargs, 1, 1)) {
        goto exit;
    }
    dt = PyValue_Object(args[0]);
    if (dt == NULL) {
        goto exit;
    }
    _return_value = zoneinfo_ZoneInfo_tzname_impl(self, cls, dt);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo__unpickle__doc__,
"_unpickle($type, key, from_cache, /)\n"
"--\n"
"\n"
"Private method used in unpickling.");

#define ZONEINFO_ZONEINFO__UNPICKLE_METHODDEF    \
    {"_unpickle", _PyCFunction_CAST(zoneinfo_ZoneInfo__unpickle), METH_METHOD|METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo__unpickle__doc__},

static PyObject *
zoneinfo_ZoneInfo__unpickle_impl(PyTypeObject *type, PyTypeObject *cls,
                                 PyObject *key, unsigned char from_cache);

static PyValue
zoneinfo_ZoneInfo__unpickle(PyObject *type, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *key;
    unsigned char from_cache;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("_unpickle", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_unpickle", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Object(args[0]);
    if (key == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            Py_ssize_t _bytes = PyLong_AsNativeBytes(_arg, &from_cache, sizeof(unsigned char),
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
    }
    _return_value = zoneinfo_ZoneInfo__unpickle_impl((PyTypeObject *)type, cls, key, from_cache);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo___init_subclass____doc__,
"__init_subclass__($type, /, **kwargs)\n"
"--\n"
"\n"
"Function to initialize subclasses.");

#define ZONEINFO_ZONEINFO___INIT_SUBCLASS___METHODDEF    \
    {"__init_subclass__", _PyCFunction_CAST(zoneinfo_ZoneInfo___init_subclass__), METH_VARARGS|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo___init_subclass____doc__},

static PyObject *
zoneinfo_ZoneInfo___init_subclass___impl(PyTypeObject *type,
                                         PyObject *kwargs);

static PyObject *
zoneinfo_ZoneInfo___init_subclass__(PyObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_NoPositional("__init_subclass__", args)) {
        goto exit;
    }
    if (kwargs == NULL) {
        __clinic_kwargs = PyDict_New();
        if (__clinic_kwargs == NULL) {
            goto exit;
        }
    }
    else {
        __clinic_kwargs = Py_NewRef(kwargs);
    }
    return_value = zoneinfo_ZoneInfo___init_subclass___impl((PyTypeObject *)type, __clinic_kwargs);

exit:
    /* Cleanup for kwargs */
    Py_XDECREF(__clinic_kwargs);

    return return_value;
}
/*[clinic end generated code: output=885dd19b0dc78d23 input=a9049054013a1b77]*/
