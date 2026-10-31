/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_abc__reset_registry__doc__,
"_reset_registry($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper to reset registry of a given class.\n"
"\n"
"Should be only used by refleak.py");

#define _ABC__RESET_REGISTRY_METHODDEF    \
    {"_reset_registry", _PyCFunction_CAST(_abc__reset_registry), METH_O, _abc__reset_registry__doc__},

static PyObject *
_abc__reset_registry_impl(PyObject *module, PyObject *self);

static PyValue
_abc__reset_registry(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *_return_value;

    self = PyValue_Object(arg);
    if (self == NULL) {
        goto exit;
    }
    _return_value = _abc__reset_registry_impl(module, self);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__reset_caches__doc__,
"_reset_caches($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper to reset both caches of a given class.\n"
"\n"
"Should be only used by refleak.py");

#define _ABC__RESET_CACHES_METHODDEF    \
    {"_reset_caches", _PyCFunction_CAST(_abc__reset_caches), METH_O, _abc__reset_caches__doc__},

static PyObject *
_abc__reset_caches_impl(PyObject *module, PyObject *self);

static PyValue
_abc__reset_caches(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *_return_value;

    self = PyValue_Object(arg);
    if (self == NULL) {
        goto exit;
    }
    _return_value = _abc__reset_caches_impl(module, self);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__get_dump__doc__,
"_get_dump($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper for cache and registry debugging.\n"
"\n"
"Return shallow copies of registry, of both caches, and\n"
"negative cache version. Don\'t call this function directly,\n"
"instead use ABC._dump_registry() for a nice repr.");

#define _ABC__GET_DUMP_METHODDEF    \
    {"_get_dump", _PyCFunction_CAST(_abc__get_dump), METH_O, _abc__get_dump__doc__},

static PyObject *
_abc__get_dump_impl(PyObject *module, PyObject *self);

static PyValue
_abc__get_dump(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *_return_value;

    self = PyValue_Object(arg);
    if (self == NULL) {
        goto exit;
    }
    _return_value = _abc__get_dump_impl(module, self);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_init__doc__,
"_abc_init($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper for class set-up. Should be never used outside abc module.");

#define _ABC__ABC_INIT_METHODDEF    \
    {"_abc_init", _PyCFunction_CAST(_abc__abc_init), METH_O, _abc__abc_init__doc__},

static PyObject *
_abc__abc_init_impl(PyObject *module, PyObject *self);

static PyValue
_abc__abc_init(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *_return_value;

    self = PyValue_Object(arg);
    if (self == NULL) {
        goto exit;
    }
    _return_value = _abc__abc_init_impl(module, self);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_register__doc__,
"_abc_register($module, self, subclass, /)\n"
"--\n"
"\n"
"Internal ABC helper for subclasss registration. Should be never used outside abc module.");

#define _ABC__ABC_REGISTER_METHODDEF    \
    {"_abc_register", _PyCFunction_CAST(_abc__abc_register), METH_FASTCALL, _abc__abc_register__doc__},

static PyObject *
_abc__abc_register_impl(PyObject *module, PyObject *self, PyObject *subclass);

static PyValue
_abc__abc_register(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *subclass;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_abc_register", nargs, 2, 2)) {
        goto exit;
    }
    self = PyValue_Object(args[0]);
    if (self == NULL) {
        goto exit;
    }
    subclass = PyValue_Object(args[1]);
    if (subclass == NULL) {
        goto exit;
    }
    _return_value = _abc__abc_register_impl(module, self, subclass);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_instancecheck__doc__,
"_abc_instancecheck($module, self, instance, /)\n"
"--\n"
"\n"
"Internal ABC helper for instance checks. Should be never used outside abc module.");

#define _ABC__ABC_INSTANCECHECK_METHODDEF    \
    {"_abc_instancecheck", _PyCFunction_CAST(_abc__abc_instancecheck), METH_FASTCALL, _abc__abc_instancecheck__doc__},

static PyObject *
_abc__abc_instancecheck_impl(PyObject *module, PyObject *self,
                             PyObject *instance);

static PyValue
_abc__abc_instancecheck(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *instance;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_abc_instancecheck", nargs, 2, 2)) {
        goto exit;
    }
    self = PyValue_Object(args[0]);
    if (self == NULL) {
        goto exit;
    }
    instance = PyValue_Object(args[1]);
    if (instance == NULL) {
        goto exit;
    }
    _return_value = _abc__abc_instancecheck_impl(module, self, instance);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_subclasscheck__doc__,
"_abc_subclasscheck($module, self, subclass, /)\n"
"--\n"
"\n"
"Internal ABC helper for subclasss checks. Should be never used outside abc module.");

#define _ABC__ABC_SUBCLASSCHECK_METHODDEF    \
    {"_abc_subclasscheck", _PyCFunction_CAST(_abc__abc_subclasscheck), METH_FASTCALL, _abc__abc_subclasscheck__doc__},

static PyObject *
_abc__abc_subclasscheck_impl(PyObject *module, PyObject *self,
                             PyObject *subclass);

static PyValue
_abc__abc_subclasscheck(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *self;
    PyObject *subclass;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_abc_subclasscheck", nargs, 2, 2)) {
        goto exit;
    }
    self = PyValue_Object(args[0]);
    if (self == NULL) {
        goto exit;
    }
    subclass = PyValue_Object(args[1]);
    if (subclass == NULL) {
        goto exit;
    }
    _return_value = _abc__abc_subclasscheck_impl(module, self, subclass);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc_get_cache_token__doc__,
"get_cache_token($module, /)\n"
"--\n"
"\n"
"Returns the current ABC cache token.\n"
"\n"
"The token is an opaque object (supporting equality testing) identifying\n"
"the current version of the ABC cache for virtual subclasses.  The token\n"
"changes with every call to register() on any ABC.");

#define _ABC_GET_CACHE_TOKEN_METHODDEF    \
    {"get_cache_token", (PyCFunction)_abc_get_cache_token, METH_NOARGS, _abc_get_cache_token__doc__},

static PyObject *
_abc_get_cache_token_impl(PyObject *module);

static PyObject *
_abc_get_cache_token(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _abc_get_cache_token_impl(module);
}
/*[clinic end generated code: output=d070bd610041a12a input=a9049054013a1b77]*/
