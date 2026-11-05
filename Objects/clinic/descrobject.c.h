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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(mappingproxy_new__doc__,
"mappingproxy(mapping)\n"
"--\n"
"\n"
"Read-only proxy of a mapping.");

static PyValue
mappingproxy_new_impl(PyTypeObject *type, PyValueBorrow mapping);

static PyValue
mappingproxy_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(mapping)), },
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
    Py_ssize_t _idx[1];
    PyValueBorrow mapping;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    mapping = PyValue_Borrow(args[_idx[0]]);
    return_value = mappingproxy_new_impl(type, mapping);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
mappingproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(mappingproxy_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = mappingproxy_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(property___set_name____doc__,
"__set_name__($self, owner, name, /)\n"
"--\n"
"\n"
"Method to set name of a property.");

#define PROPERTY___SET_NAME___METHODDEF    \
    {"__set_name__", _PyCFunction_CAST(property___set_name__), METH_VALUE, property___set_name____doc__},

static PyValue
property___set_name___impl(propertyobject *self, PyValueBorrow owner,
                           PyValueBorrow name);

static PyValue
property___set_name__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow owner;
    PyValueBorrow name;

    if (!_PyArg_NoKwnamesValueMethod(&PyProperty_Type, "__set_name__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__set_name__", nargs, 2, 2)) {
        goto exit;
    }
    owner = PyValue_Borrow(args[0]);
    name = PyValue_Borrow(args[1]);
    return_value = property___set_name___impl((propertyobject *)PyValue_AsKnownObjectBorrow(self), owner, name);

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
property_init_impl(propertyobject *self, PyValueBorrow fget,
                   PyValueBorrow fset, PyValueBorrow fdel, PyValueBorrow doc);

static int
property_init_value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(fget)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fset)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fdel)), _PyValue_STATIC_IMMORTAL(&_Py_ID(doc)), },
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
    Py_ssize_t _idx[4];
    PyValueBorrow fget = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow fset = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow fdel = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow doc = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        fget = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        fset = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        fdel = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        doc = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = property_init_impl((propertyobject *)self, fget, fset, fdel, doc);

exit:
    return return_value;
}

static int
property_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return property_init_value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = property_init_value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}
/*[clinic end generated code: output=3a24ea0d0ab853be input=a9049054013a1b77]*/
