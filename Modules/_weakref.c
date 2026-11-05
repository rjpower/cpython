#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_dict.h"              // _PyDict_DelItemIf()
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_object.h"            // _PyObject_GET_WEAKREFS_LISTPTR()
#include "pycore_value.h"            // PyValue_AsKnownObjectBorrow(), _PyValue_AsObjectAt()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()
#include "pycore_weakref.h"           // _PyWeakref_IS_DEAD()

#define GET_WEAKREFS_LISTPTR(o) \
        ((PyWeakReference **) _PyObject_GET_WEAKREFS_LISTPTR(o))

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _weakref
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ffec73b85846596d]*/

#include "clinic/_weakref.c.h"

/*[clinic input]
_weakref.getweakrefcount -> Py_ssize_t

  object: object
  /

Return the number of weak references to 'object'.
[clinic start generated code]*/

static Py_ssize_t
_weakref_getweakrefcount_impl(PyObject *module, PyValueBorrow object)
/*[clinic end generated code: output=93f2722b5c118a2b input=7d4d04fcaccf64d5]*/
{
    PyObject *obj = _PyValue_AsObjectAt(object, BOX_SITE_PARSER);
    if (obj == NULL) {
        return -1;
    }
    Py_ssize_t count = _PyWeakref_GetWeakrefCount(obj);
    Py_DECREF(obj);
    return count;
}


static int
is_dead_weakref(PyObject *value, void *unused)
{
    if (!PyWeakref_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "not a weakref");
        return -1;
    }
    return _PyWeakref_IS_DEAD(value);
}

/*[clinic input]

_weakref._remove_dead_weakref -> object

  dct: object(subclass_of='&PyDict_Type')
  key: object
  /

Atomically remove key from dict if it points to a dead weakref.
[clinic start generated code]*/

static PyValue
_weakref__remove_dead_weakref_impl(PyObject *module, PyObject *dct,
                                   PyValueBorrow key)
/*[clinic end generated code: output=d81fbf4809689aeb input=19fc91f257f96a1d]*/
{
    PyObject *keyobj = _PyValue_AsObjectAt(key, BOX_SITE_PARSER);
    if (keyobj == NULL) {
        return PyValue_NULL;
    }
    int res = _PyDict_DelItemIf(dct, keyobj, is_dead_weakref, NULL);
    Py_DECREF(keyobj);
    if (res < 0) {
        return PyValue_NULL;
    }
    return PyValue_None;
}


/*[clinic input]
_weakref.getweakrefs
    object: object
    /

Return a list of all weak reference objects pointing to 'object'.
[clinic start generated code]*/

static PyValue
_weakref_getweakrefs_impl(PyObject *module, PyValueBorrow object)
/*[clinic end generated code: output=7934c4673294d996 input=00c6d0e5d3206693]*/
{
    if (!_PyType_SUPPORTS_WEAKREFS(PyValue_TYPE(object))) {
        return _PyValue_FromObjectResult(PyList_New(0));
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return PyValue_NULL;
    }

    // Only an object that supports weakrefs gets here, so it is not inline.
    PyObject *obj = PyValue_AsKnownObjectBorrow(object);
    LOCK_WEAKREFS(obj);
    PyWeakReference *current = *GET_WEAKREFS_LISTPTR(obj);
    while (current != NULL) {
        PyObject *curobj = (PyObject *) current;
        if (_Py_TryIncref(curobj)) {
            if (PyList_AppendValue(result, PyValue_FromObjectBorrow(curobj))) {
                UNLOCK_WEAKREFS(obj);
                Py_DECREF(curobj);
                Py_DECREF(result);
                return PyValue_NULL;
            }
            else {
                // Undo our _Py_TryIncref. This is safe to do with the lock
                // held in free-threaded builds; the list holds a reference to
                // curobj so we're guaranteed not to invoke the destructor.
                Py_DECREF(curobj);
            }
        }
        current = current->wr_next;
    }
    UNLOCK_WEAKREFS(obj);
    return _PyValue_FromObjectResult(result);
}


/*[clinic input]

_weakref.proxy
    object: object
    callback: object(c_default="NULL") = None
    /

Create a proxy object that weakly references 'object'.

'callback', if given, is called with a reference to the
proxy when 'object' is about to be finalized.
[clinic start generated code]*/

static PyValue
_weakref_proxy_impl(PyObject *module, PyValueBorrow object,
                    PyValueBorrow callback)
/*[clinic end generated code: output=df2571ecd901b803 input=4808adf22fd137e7]*/
{
    PyObject *obj = _PyValue_AsObjectAt(object, BOX_SITE_PARSER);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    PyObject *cb = NULL;
    if (!PyValue_IsNull(callback)) {
        cb = _PyValue_AsObjectAt(callback, BOX_SITE_PARSER);
        if (cb == NULL) {
            Py_DECREF(obj);
            return PyValue_NULL;
        }
    }
    PyObject *proxy = PyWeakref_NewProxy(obj, cb);
    Py_DECREF(obj);
    Py_XDECREF(cb);
    return _PyValue_FromObjectResult(proxy);
}


static PyMethodDef
weakref_functions[] =  {
    _WEAKREF_GETWEAKREFCOUNT_METHODDEF
    _WEAKREF__REMOVE_DEAD_WEAKREF_METHODDEF
    _WEAKREF_GETWEAKREFS_METHODDEF
    _WEAKREF_PROXY_METHODDEF
    {NULL, NULL, 0, NULL}
};

static int
weakref_exec(PyObject *module)
{
    if (PyModule_AddObjectRef(module, "ref", (PyObject *) &_PyWeakref_RefType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "ReferenceType",
                           (PyObject *) &_PyWeakref_RefType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "ProxyType",
                           (PyObject *) &_PyWeakref_ProxyType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "CallableProxyType",
                           (PyObject *) &_PyWeakref_CallableProxyType) < 0) {
        return -1;
    }

    return 0;
}

static struct PyModuleDef_Slot weakref_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, weakref_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef weakrefmodule = {
    PyModuleDef_HEAD_INIT,
    "_weakref",
    "Weak-reference support module.",
    0,
    weakref_functions,
    weakref_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__weakref(void)
{
    return PyModuleDef_Init(&weakrefmodule);
}
