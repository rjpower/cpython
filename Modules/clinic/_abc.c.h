/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_abc__reset_registry__doc__,
"_reset_registry($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper to reset registry of a given class.\n"
"\n"
"Should be only used by refleak.py");

#define _ABC__RESET_REGISTRY_METHODDEF    \
    {"_reset_registry", _PyCFunction_CAST(_abc__reset_registry), METH_VALUE|METH_O, _abc__reset_registry__doc__},

static PyValue
_abc__reset_registry_impl(PyObject *module, PyValueBorrow self);

static PyValue
_abc__reset_registry(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;

    self = PyValue_Borrow(args[0]);
    return_value = _abc__reset_registry_impl(PyValue_AsKnownObjectBorrow(module), self);

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
    {"_reset_caches", _PyCFunction_CAST(_abc__reset_caches), METH_VALUE|METH_O, _abc__reset_caches__doc__},

static PyValue
_abc__reset_caches_impl(PyObject *module, PyValueBorrow self);

static PyValue
_abc__reset_caches(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;

    self = PyValue_Borrow(args[0]);
    return_value = _abc__reset_caches_impl(PyValue_AsKnownObjectBorrow(module), self);

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
    {"_get_dump", _PyCFunction_CAST(_abc__get_dump), METH_VALUE|METH_O, _abc__get_dump__doc__},

static PyValue
_abc__get_dump_impl(PyObject *module, PyValueBorrow self);

static PyValue
_abc__get_dump(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;

    self = PyValue_Borrow(args[0]);
    return_value = _abc__get_dump_impl(PyValue_AsKnownObjectBorrow(module), self);

    return return_value;
}

PyDoc_STRVAR(_abc__abc_init__doc__,
"_abc_init($module, self, /)\n"
"--\n"
"\n"
"Internal ABC helper for class set-up. Should be never used outside abc module.");

#define _ABC__ABC_INIT_METHODDEF    \
    {"_abc_init", _PyCFunction_CAST(_abc__abc_init), METH_VALUE|METH_O, _abc__abc_init__doc__},

static PyValue
_abc__abc_init_impl(PyObject *module, PyValueBorrow self);

static PyValue
_abc__abc_init(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;

    self = PyValue_Borrow(args[0]);
    return_value = _abc__abc_init_impl(PyValue_AsKnownObjectBorrow(module), self);

    return return_value;
}

PyDoc_STRVAR(_abc__abc_register__doc__,
"_abc_register($module, self, subclass, /)\n"
"--\n"
"\n"
"Internal ABC helper for subclasss registration. Should be never used outside abc module.");

#define _ABC__ABC_REGISTER_METHODDEF    \
    {"_abc_register", _PyCFunction_CAST(_abc__abc_register), METH_VALUE, _abc__abc_register__doc__},

static PyValue
_abc__abc_register_impl(PyObject *module, PyValueBorrow self,
                        PyValueBorrow subclass);

static PyValue
_abc__abc_register(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;
    PyValueBorrow subclass;

    if (!_PyArg_CheckPositional("_abc_register", nargs, 2, 2)) {
        goto exit;
    }
    self = PyValue_Borrow(args[0]);
    subclass = PyValue_Borrow(args[1]);
    return_value = _abc__abc_register_impl(PyValue_AsKnownObjectBorrow(module), self, subclass);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_instancecheck__doc__,
"_abc_instancecheck($module, self, instance, /)\n"
"--\n"
"\n"
"Internal ABC helper for instance checks. Should be never used outside abc module.");

#define _ABC__ABC_INSTANCECHECK_METHODDEF    \
    {"_abc_instancecheck", _PyCFunction_CAST(_abc__abc_instancecheck), METH_VALUE, _abc__abc_instancecheck__doc__},

static PyValue
_abc__abc_instancecheck_impl(PyObject *module, PyValueBorrow self,
                             PyValueBorrow instance);

static PyValue
_abc__abc_instancecheck(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;
    PyValueBorrow instance;

    if (!_PyArg_CheckPositional("_abc_instancecheck", nargs, 2, 2)) {
        goto exit;
    }
    self = PyValue_Borrow(args[0]);
    instance = PyValue_Borrow(args[1]);
    return_value = _abc__abc_instancecheck_impl(PyValue_AsKnownObjectBorrow(module), self, instance);

exit:
    return return_value;
}

PyDoc_STRVAR(_abc__abc_subclasscheck__doc__,
"_abc_subclasscheck($module, self, subclass, /)\n"
"--\n"
"\n"
"Internal ABC helper for subclasss checks. Should be never used outside abc module.");

#define _ABC__ABC_SUBCLASSCHECK_METHODDEF    \
    {"_abc_subclasscheck", _PyCFunction_CAST(_abc__abc_subclasscheck), METH_VALUE, _abc__abc_subclasscheck__doc__},

static PyValue
_abc__abc_subclasscheck_impl(PyObject *module, PyValueBorrow self,
                             PyValueBorrow subclass);

static PyValue
_abc__abc_subclasscheck(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow self;
    PyValueBorrow subclass;

    if (!_PyArg_CheckPositional("_abc_subclasscheck", nargs, 2, 2)) {
        goto exit;
    }
    self = PyValue_Borrow(args[0]);
    subclass = PyValue_Borrow(args[1]);
    return_value = _abc__abc_subclasscheck_impl(PyValue_AsKnownObjectBorrow(module), self, subclass);

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
    {"get_cache_token", _PyCFunction_CAST(_abc_get_cache_token), METH_VALUE|METH_NOARGS, _abc_get_cache_token__doc__},

static PyValue
_abc_get_cache_token_impl(PyObject *module);

static PyValue
_abc_get_cache_token(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _abc_get_cache_token_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}
/*[clinic end generated code: output=b3181360d4254121 input=a9049054013a1b77]*/
