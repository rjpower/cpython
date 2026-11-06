/* microprotocols.c - minimalist and non-validating protocols implementation
 *
 * Copyright (C) 2003-2004 Federico Di Gregorio <fog@debian.org>
 *
 * This file is part of psycopg and was adapted for pysqlite. Federico Di
 * Gregorio gave the permission to use it within pysqlite under the following
 * license:
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include <Python.h>

#include "pycore_dict.h"          // PyDict_SetValue()
#include "pycore_tuple.h"         // _PyTuple_PackValues()
#include "pycore_value.h"         // PyValue
#include "pycore_value_call.h"    // _PyValue_CallOneArg()

#include "cursor.h"
#include "microprotocols.h"
#include "prepare_protocol.h"


/* pysqlite_microprotocols_init - initialize the adapters dictionary */

int
pysqlite_microprotocols_init(PyObject *module)
{
    /* create adapters dictionary and put it in module namespace */
    pysqlite_state *state = pysqlite_get_state(module);
    state->psyco_adapters = PyDict_New();
    if (state->psyco_adapters == NULL) {
        return -1;
    }

    return PyModule_AddObjectRef(module, "adapters", state->psyco_adapters);
}


/* pysqlite_microprotocols_add - add a reverse type-caster to the dictionary */

int
pysqlite_microprotocols_add(pysqlite_state *state, PyTypeObject *type,
                            PyObject *proto, PyObject *cast)
{
    PyObject* key;
    int rc;

    assert(type != NULL);
    assert(proto != NULL);
    key = _PyTuple_PackValues(
        2, (PyValueBorrow[]){PyValue_FromObjectBorrow((PyObject *)type),
                             PyValue_FromObjectBorrow(proto)});
    if (!key) {
        return -1;
    }

    rc = PyDict_SetValue(state->psyco_adapters, PyValue_FromObjectBorrow(key),
                         PyValue_FromObjectBorrow(cast));
    Py_DECREF(key);

    return rc;
}

/* pysqlite_microprotocols_adapt - adapt an object to the built-in protocol */

PyValue
pysqlite_microprotocols_adapt(pysqlite_state *state, PyValueBorrow obj,
                              PyValueBorrow proto, PyValueBorrow alt)
{
    PyObject *key = _PyTuple_PackValues(
        2, (PyValueBorrow[]){PyValue_FromObjectBorrow((PyObject *)PyValue_TYPE(obj)),
                             proto});
    if (key == NULL) {
        return PyValue_NULL;
    }
    PyValue adapter;
    int found = PyDict_GetValue(state->psyco_adapters,
                                PyValue_FromObjectBorrow(key), &adapter);
    Py_DECREF(key);
    if (found < 0) {
        return PyValue_NULL;
    }
    if (found) {
        PyValue result = _PyValue_CallOneArg(_PyThreadState_GET(),
                                             PyValue_Borrow(adapter), obj);
        PyValue_CLOSE(adapter);
        return result;
    }

    _PyStatusValue attr = _PyValue_GetOptionalAttr(proto, state->str___adapt__);
    if (attr.status < 0) {
        return PyValue_NULL;
    }
    if (attr.status) {
        PyValue result = _PyValue_CallOneArg(_PyThreadState_GET(),
                                             PyValue_Borrow(attr.value), obj);
        PyValue_CLOSE(attr.value);
        if (PyValue_IsNone(result)) {
            PyValue_CLOSE(result);
        }
        else if (!PyValue_IsNull(result) || !PyErr_ExceptionMatches(PyExc_TypeError)) {
            return result;
        }
        else {
            PyErr_Clear();
        }
    }

    attr = _PyValue_GetOptionalAttr(obj, state->str___conform__);
    if (attr.status < 0) {
        return PyValue_NULL;
    }
    if (attr.status) {
        PyValue result = _PyValue_CallOneArg(_PyThreadState_GET(),
                                             PyValue_Borrow(attr.value), proto);
        PyValue_CLOSE(attr.value);
        if (PyValue_IsNone(result)) {
            PyValue_CLOSE(result);
        }
        else if (!PyValue_IsNull(result) || !PyErr_ExceptionMatches(PyExc_TypeError)) {
            return result;
        }
        else {
            PyErr_Clear();
        }
    }
    if (!PyValue_IsNull(alt)) {
        return PyValue_DUP(alt);
    }
    PyErr_SetString(state->ProgrammingError, "can't adapt");
    return PyValue_NULL;
}
