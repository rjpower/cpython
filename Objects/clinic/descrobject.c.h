/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(mappingproxy_new__doc__,
"mappingproxy(mapping)\n"
"--\n"
"\n"
"Read-only proxy of a mapping.");

static PyObject *
mappingproxy_new_impl(PyTypeObject *type, PyObject *mapping);

static PyObject *
mappingproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(mapping), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mapping", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "mappingproxy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *mapping;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    mapping = PyValue_Object(fastargs[0]);
    if (mapping == NULL) {
        goto exit;
    }
    return_value = mappingproxy_new_impl(type, mapping);

exit:
    return return_value;
}

PyDoc_STRVAR(property___set_name____doc__,
"__set_name__($self, owner, name, /)\n"
"--\n"
"\n"
"Method to set name of a property.");

#define PROPERTY___SET_NAME___METHODDEF    \
    {"__set_name__", _PyCFunction_CAST(property___set_name__), METH_FASTCALL, property___set_name____doc__},

static PyObject *
property___set_name___impl(propertyobject *self, PyObject *owner,
                           PyObject *name);

static PyValue
property___set_name__(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *owner;
    PyObject *name;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("__set_name__", nargs, 2, 2)) {
        goto exit;
    }
    owner = PyValue_Object(args[0]);
    if (owner == NULL) {
        goto exit;
    }
    name = PyValue_Object(args[1]);
    if (name == NULL) {
        goto exit;
    }
    _return_value = property___set_name___impl((propertyobject *)self, owner, name);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(property_init__doc__,
"property(fget=None, fset=None, fdel=None, doc=None)\n"
"--\n"
"\n"
"Property attribute.\n"
"\n"
"  fget\n"
"    function to be used for getting an attribute value\n"
"  fset\n"
"    function to be used for setting an attribute value\n"
"  fdel\n"
"    function to be used for del\'ing an attribute\n"
"  doc\n"
"    docstring\n"
"\n"
"Typical use is to define a managed attribute x:\n"
"\n"
"class C(object):\n"
"    def getx(self): return self._x\n"
"    def setx(self, value): self._x = value\n"
"    def delx(self): del self._x\n"
"    x = property(getx, setx, delx, \"I\'m the \'x\' property.\")\n"
"\n"
"Decorators make defining new properties or modifying existing ones easy:\n"
"\n"
"class C(object):\n"
"    @property\n"
"    def x(self):\n"
"        \"I am the \'x\' property.\"\n"
"        return self._x\n"
"    @x.setter\n"
"    def x(self, value):\n"
"        self._x = value\n"
"    @x.deleter\n"
"    def x(self):\n"
"        del self._x");

static int
property_init_impl(propertyobject *self, PyObject *fget, PyObject *fset,
                   PyObject *fdel, PyObject *doc);

static int
property_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(fget), &_Py_ID(fset), &_Py_ID(fdel), &_Py_ID(doc), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"fget", "fset", "fdel", "doc", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "property",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *fget = NULL;
    PyObject *fset = NULL;
    PyObject *fdel = NULL;
    PyObject *doc = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        fget = PyValue_Object(fastargs[0]);
        if (fget == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[1])) {
        fset = PyValue_Object(fastargs[1]);
        if (fset == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[2])) {
        fdel = PyValue_Object(fastargs[2]);
        if (fdel == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    doc = PyValue_Object(fastargs[3]);
    if (doc == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = property_init_impl((propertyobject *)self, fget, fset, fdel, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=9e0a6d5eb792085c input=a9049054013a1b77]*/
