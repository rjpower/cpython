/* Frame object implementation */

#include "Python.h"
#include "pycore_cell.h"          // _PyCell_GetValue()
#include "pycore_ceval.h"         // _PyEval_SetOpcodeTrace()
#include "pycore_code.h"          // CO_FAST_LOCAL
#include "pycore_dict.h"          // _PyDict_LoadBuiltinsFromGlobals()
#include "pycore_frame.h"         // PyFrameObject
#include "pycore_function.h"      // _PyFunction_FromConstructor()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_interpframe.h"   // _PyFrame_GetLocalsArray()
#include "pycore_ir_source.h"
#include "pycore_ir_outcomes.h"
#include "pycore_list.h"          // _PyList_AppendTakeRef()
#include "pycore_long_value.h"    // _PyValue_AsLongAndOverflow()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_opcode_utils.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_UINT64_RELAXED()
#include "pycore_setobject.h"     // PySet_AddValue()
#include "pycore_tstate.h"
#include "pycore_tuple.h"         // _PyTuple_FromPair
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()
#include "pycore_value_ops.h"     // _PyValue_GetItem()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include "frameobject.h"          // PyFrameLocalsProxyObject
#include "opcode.h"               // EXTENDED_ARG

#include "clinic/frameobject.c.h"


#define PyFrameObject_CAST(op)  \
    (assert(PyObject_TypeCheck((op), &PyFrame_Type)), (PyFrameObject *)(op))

#define PyFrameLocalsProxyObject_CAST(op)                           \
    (                                                               \
        assert(PyObject_TypeCheck((op), &PyFrameLocalsProxy_Type)), \
        (PyFrameLocalsProxyObject *)(op)                            \
    )

#define OFF(x) offsetof(PyFrameObject, x)

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class frame "PyFrameObject *" "&PyFrame_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2d1dbf2e06cf351f]*/


// Returns a new reference, or NULL if the variable is unset. Boxing an
// inline value can fail, which returns NULL with an exception set.
static PyObject *
framelocalsproxy_getval(_PyInterpreterFrame *frame, PyCodeObject *co, int i)
{
    PyValue *fast = _PyFrame_GetLocalsArray(frame);
    _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

    if (kind & CO_FAST_CONST) {
        // A constant slot is not a variable
        return NULL;
    }
    if (PyValue_IsInline(fast[i])) {
        return _PyValue_AsObjectAt(fast[i], BOX_SITE_FRAME);
    }
    PyObject *value = PyValue_AsKnownObjectBorrow(fast[i]);
    PyObject *cell = NULL;

    if (value == NULL) {
        return NULL;
    }

    if (kind == CO_FAST_FREE || kind & CO_FAST_CELL) {
        // The cell was set when the frame was created from
        // the function's closure.
        // GH-128396: With PEP 709, it's possible to have a fast variable in
        // an inlined comprehension that has the same name as the cell variable
        // in the frame, where the `kind` obtained from frame can not guarantee
        // that the variable is a cell.
        // If the variable is not a cell, we are okay with it and we can simply
        // return the value.
        if (PyCell_Check(value)) {
            cell = value;
        }
    }

    if (cell != NULL) {
        PyValue cv = PyCell_GetValue(cell);
        value = PyValue_IsNull(cv) ? NULL : _PyValue_AsObjectAt(cv, BOX_SITE_CELL);
        PyValue_XCLOSE(cv);
    }
    else {
        Py_XINCREF(value);
    }

    if (value == NULL) {
        return NULL;
    }

    return value;
}

static bool
framelocalsproxy_hasval(_PyInterpreterFrame *frame, PyCodeObject *co, int i)
{
    if (_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_CONST) {
        return false;
    }
    if (PyValue_IsInline(_PyFrame_GetLocalsArray(frame)[i])) {
        return true;
    }
    PyObject *value = framelocalsproxy_getval(frame, co, i);
    if (value == NULL) {
        return false;
    }
    Py_DECREF(value);
    return true;
}

static int
framelocalsproxy_is_first_occurrence(PyObject *seen, PyObject *name)
{
    int found = PySet_ContainsValue(seen, PyValue_FromObjectBorrow(name));
    if (found < 0) {
        return -1;
    }
    if (found) {
        return 0;
    }
    if (PySet_AddValue(seen, PyValue_FromObjectBorrow(name)) < 0) {
        return -1;
    }
    return 1;
}

static int
framelocalsproxy_getkeyindex(PyFrameObject *frame, PyObject *key, bool read, PyObject **value_ptr)
{
    /*
     * Returns -2 (!) if an error occurred; exception will be set.
     * Returns the fast locals index of the key on success:
     *   - if read == true, returns the index if the value is not NULL
     *   - if read == false, returns the index if the value is not hidden
     * Otherwise returns -1.
     *
     * If read == true and value_ptr is not NULL, *value_ptr is set to
     * the value of the key if it is found (with a new reference).
     */

    // value_ptr should only be given if we are reading the value
    assert(read || value_ptr == NULL);

    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);

    // Ensure that the key is hashable.
    Py_hash_t key_hash = _PyValue_Hash(PyValue_FromObjectBorrow(key));
    if (key_hash == -1) {
        return -2;
    }

    bool found = false;

    // We do 2 loops here because it's highly possible the key is interned
    // and we can do a pointer comparison.
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
        if (name == key) {
            if (read) {
                PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
                if (value != NULL) {
                    if (value_ptr != NULL) {
                        *value_ptr = value;
                    }
                    else {
                        Py_DECREF(value);
                    }
                    return i;
                }
                if (PyErr_Occurred()) {
                    return -2;
                }
            } else {
                if (!(_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_HIDDEN)) {
                    return i;
                }
            }
            found = true;
        }
    }
    if (found) {
        // This is an attempt to read an unset local variable or
        // write to a variable that is hidden from regular write operations
        return -1;
    }
    // This is unlikely, but we need to make sure. This means the key
    // is not interned.
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
        Py_hash_t name_hash = _PyValue_Hash(PyValue_FromObjectBorrow(name));
        assert(name_hash != -1);  // keys are exact unicode
        if (name_hash != key_hash) {
            continue;
        }
        int same = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(name), PyValue_FromObjectBorrow(key), Py_EQ);
        if (same < 0) {
            return -2;
        }
        if (same) {
            if (read) {
                PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
                if (value != NULL) {
                    if (value_ptr != NULL) {
                        *value_ptr = value;
                    }
                    else {
                        Py_DECREF(value);
                    }
                    return i;
                }
                if (PyErr_Occurred()) {
                    return -2;
                }
            } else {
                if (!(_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_HIDDEN)) {
                    return i;
                }
            }
        }
    }

    return -1;
}

static PyObject *
framelocalsproxy_getitem(PyObject *self, PyObject *key)
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyObject *value = NULL;

    int i = framelocalsproxy_getkeyindex(frame, key, true, &value);
    if (i == -2) {
        return NULL;
    }
    if (i >= 0) {
        assert(value != NULL);
        return value;
    }
    assert(value == NULL);

    // Okay not in the fast locals, try extra locals

    PyObject *extra = frame->f_extra_locals;
    if (extra != NULL) {
        PyValue ev;
        if (PyDict_GetValue(extra, PyValue_FromObjectBorrow(key), &ev) < 0) {
            return NULL;
        }
        if (!PyValue_IsNull(ev)) {
            value = _PyValue_AsObjectAt(ev, BOX_SITE_DICT);
            PyValue_CLOSE(ev);
            return value;
        }
    }

    PyErr_Format(PyExc_KeyError, "local variable '%R' is not defined", key);
    return NULL;
}

static int
add_overwritten_fast_local(PyFrameObject *frame, PyObject *obj)
{
    Py_ssize_t new_size;
    if (frame->f_overwritten_fast_locals == NULL) {
        new_size = 1;
    }
    else {
        Py_ssize_t size = PyTuple_Size(frame->f_overwritten_fast_locals);
        if (size == -1) {
            return -1;
        }
        new_size = size + 1;
    }
    PyObject *new_tuple = PyTuple_New(new_size);
    if (new_tuple == NULL) {
        return -1;
    }
    for (Py_ssize_t i = 0; i < new_size - 1; i++) {
        PyTuple_SET_VALUE(new_tuple, i, PyValue_XDUP(
            PyTuple_GET_VALUE(frame->f_overwritten_fast_locals, i)));
    }
    PyTuple_SET_VALUE(new_tuple, new_size - 1,
                     PyValue_XDUP(PyValue_FromObjectBorrow(obj)));
    Py_XSETREF(frame->f_overwritten_fast_locals, new_tuple);
    return 0;
}

static int
framelocalsproxy_setitem(PyObject *self, PyObject *key, PyObject *value)
{
    /* Merge locals into fast locals */
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyValue *fast = _PyFrame_GetLocalsArray(frame->f_frame);
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);

    int i = framelocalsproxy_getkeyindex(frame, key, false, NULL);
    if (i == -2) {
        return -1;
    }
    if (i >= 0) {
        if (value == NULL) {
            PyErr_SetString(PyExc_ValueError, "cannot remove local variables from FrameLocalsProxy");
            return -1;
        }

        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);
        PyValueBorrow oldvalue = PyValue_Borrow(fast[i]);
        PyObject *cell = NULL;
        if (kind == CO_FAST_FREE) {
            // The cell was set when the frame was created from
            // the function's closure.
            assert(!PyValue_IsNull(oldvalue) && PyCell_Check(PyValue_AsKnownObjectBorrow(oldvalue)));
            cell = PyValue_AsKnownObjectBorrow(oldvalue);
        } else if (kind & CO_FAST_CELL && PyValue_IsObject(oldvalue)) {
            PyObject *as_obj = PyValue_AsKnownObjectBorrow(oldvalue);
            if (PyCell_Check(as_obj)) {
                cell = as_obj;
            }
        }
        if (cell != NULL) {
            PyValue replacement = value == NULL ? PyValue_NULL :
                PyValue_XDUP(PyValue_FromObjectBorrow(value));
            _PyCell_SetValue((PyCellObject *)cell, replacement);
        } else if (PyValue_IsInline(oldvalue)) {
            // An inline value holds no reference to keep alive.
            fast[i] = PyValue_XDUP(PyValue_FromObjectBorrow(value));
        } else if (value != PyValue_AsKnownObjectBorrow(oldvalue)) {
            PyObject *old_obj = PyValue_AsKnownObjectBorrow(fast[i]);
            if (old_obj != NULL && !_Py_IsImmortal(old_obj)) {
                if (add_overwritten_fast_local(frame, old_obj) < 0) {
                    return -1;
                }
                // f_overwritten_fast_locals now keeps old_obj alive until the
                // frame dies, so LOAD_FAST_BORROW references to it elsewhere
                // on the stack stay valid after the slot's own reference goes.
                _Py_value_allow_borrows_outlive_close(fast[i]);
                PyValue_CLOSE(fast[i]);
            }
            fast[i] = PyValue_XDUP(PyValue_FromObjectBorrow(value));
        }
        return 0;
    }

    // Okay not in the fast locals, try extra locals

    PyObject *extra = frame->f_extra_locals;

    if (extra == NULL) {
        if (value == NULL) {
            _PyErr_SetKeyError(key);
            return -1;
        }
        extra = PyDict_New();
        if (extra == NULL) {
            return -1;
        }
        frame->f_extra_locals = extra;
    }

    assert(PyDict_Check(extra));

    if (value == NULL) {
        return PyDict_DelValue(extra, PyValue_FromObjectBorrow(key));
    } else {
        return PyDict_SetValue(extra, PyValue_FromObjectBorrow(key), PyValue_FromObjectBorrow(value));
    }
}

static int
framelocalsproxy_merge(PyObject* self, PyObject* other)
{
    if (!PyDict_Check(other) && !PyFrameLocalsProxy_Check(other)) {
        return -1;
    }

    PyObject *keys = _PyValue_MappingKeys(PyValue_FromObjectBorrow(other));
    if (keys == NULL) {
        return -1;
    }

    PyObject *iter = _PyValue_GetIter(PyValue_FromObjectBorrow(keys));
    Py_DECREF(keys);
    if (iter == NULL) {
        return -1;
    }

    PyObject *key = NULL;
    PyObject *value = NULL;

    while ((key = PyIter_Next(iter)) != NULL) {
        PyValue value_v = _PyValue_GetItem(PyValue_FromObjectBorrow(other),
                                           PyValue_FromObjectBorrow(key));
        value = PyValue_IsNull(value_v) ? NULL
              : _PyValue_AsObjectStealAt(value_v, BOX_SITE_FRAME);
        if (value == NULL) {
            Py_DECREF(key);
            Py_DECREF(iter);
            return -1;
        }

        if (framelocalsproxy_setitem(self, key, value) < 0) {
            Py_DECREF(key);
            Py_DECREF(value);
            Py_DECREF(iter);
            return -1;
        }

        Py_DECREF(key);
        Py_DECREF(value);
    }

    Py_DECREF(iter);

    if (PyErr_Occurred()) {
        return -1;
    }

    return 0;
}

static PyObject *
framelocalsproxy_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *names = PyList_New(0);
    if (names == NULL) {
        return NULL;
    }
    // An inlined comprehension cell can share a name with a free var.
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        Py_DECREF(names);
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        if (framelocalsproxy_hasval(frame->f_frame, co, i)) {
            PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
            int first = framelocalsproxy_is_first_occurrence(seen, name);
            if (first < 0) {
                goto error;
            }
            if (first) {
                if (PyList_AppendValue(names, PyValue_FromObjectBorrow(name)) < 0) {
                    goto error;
                }
            }
        }
    }
    Py_DECREF(seen);

    // Iterate through the extra locals
    if (frame->f_extra_locals) {
        assert(PyDict_Check(frame->f_extra_locals));

        Py_ssize_t i = 0;
        PyValue key = PyValue_NULL;
        PyValue value = PyValue_NULL;

        int rc;
        while ((rc = PyDict_NextValue(frame->f_extra_locals, &i, &key, &value)) > 0) {
            int err = PyList_AppendValue(names, key);
            PyValue_CLOSE(key);
            PyValue_CLOSE(value);
            if (err < 0) {
                Py_DECREF(names);
                return NULL;
            }
        }
        if (rc < 0) {
            Py_DECREF(names);
            return NULL;
        }
    }

    return names;

error:
    Py_DECREF(seen);
    Py_DECREF(names);
    return NULL;
}

static void
framelocalsproxy_dealloc(PyObject *self)
{
    PyFrameLocalsProxyObject *proxy = PyFrameLocalsProxyObject_CAST(self);
    PyObject_GC_UnTrack(self);
    Py_CLEAR(proxy->frame);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
framelocalsproxy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (PyTuple_GET_SIZE(args) != 1) {
        PyErr_Format(PyExc_TypeError,
                     "FrameLocalsProxy expected 1 argument, got %zd",
                     PyTuple_GET_SIZE(args));
        return NULL;
    }
    PyValueBorrow item_value = PyTuple_GET_VALUE(args, 0);
    if (!PyValue_IsObject(item_value) ||
        !PyFrame_Check(PyValue_AsKnownObjectBorrow(item_value))) {
        PyErr_Format(PyExc_TypeError, "expect frame, not %.200s",
                     PyValue_TYPE(item_value)->tp_name);
        return NULL;
    }
    PyObject *item = PyValue_AsKnownObjectBorrow(item_value);
    PyFrameObject *frame = (PyFrameObject*)item;

    if (kwds != NULL && PyDict_Size(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError,
                        "FrameLocalsProxy takes no keyword arguments");
        return 0;
    }

    PyFrameLocalsProxyObject *self = (PyFrameLocalsProxyObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    ((PyFrameLocalsProxyObject*)self)->frame = (PyFrameObject*)Py_NewRef(frame);

    return (PyObject *)self;
}

static int
framelocalsproxy_tp_clear(PyObject *self)
{
    PyFrameLocalsProxyObject *proxy = PyFrameLocalsProxyObject_CAST(self);
    Py_CLEAR(proxy->frame);
    return 0;
}

static int
framelocalsproxy_visit(PyObject *self, visitproc visit, void *arg)
{
    PyFrameLocalsProxyObject *proxy = PyFrameLocalsProxyObject_CAST(self);
    Py_VISIT(proxy->frame);
    return 0;
}

static PyObject *
framelocalsproxy_iter(PyObject *self)
{
    PyObject* keys = framelocalsproxy_keys(self, NULL);
    if (keys == NULL) {
        return NULL;
    }

    PyObject* iter = _PyValue_GetIter(PyValue_FromObjectBorrow(keys));
    Py_XDECREF(keys);

    return iter;
}

static PyObject *
framelocalsproxy_richcompare(PyObject *lhs, PyObject *rhs, int op)
{
    PyFrameLocalsProxyObject *self = PyFrameLocalsProxyObject_CAST(lhs);
    if (PyFrameLocalsProxy_Check(rhs)) {
        PyFrameLocalsProxyObject *other = (PyFrameLocalsProxyObject *)rhs;
        bool result = self->frame == other->frame;
        if (op == Py_EQ) {
            return PyBool_FromLong(result);
        } else if (op == Py_NE) {
            return PyBool_FromLong(!result);
        }
    } else if (PyDict_Check(rhs)) {
        PyObject *dct = PyDict_New();
        if (dct == NULL) {
            return NULL;
        }

        if (PyDict_Update(dct, lhs) < 0) {
            Py_DECREF(dct);
            return NULL;
        }

        PyValue result_v = _PyValue_RichCompare(PyValue_FromObjectBorrow(dct),
                                                PyValue_FromObjectBorrow(rhs), op);
        Py_DECREF(dct);
        return PyValue_IsNull(result_v) ? NULL
             : _PyValue_AsObjectStealAt(result_v, BOX_SITE_COMPARE);
    }

    Py_RETURN_NOTIMPLEMENTED;
}

static PyObject *
framelocalsproxy_repr(PyObject *self)
{
    int i = Py_ReprEnter(self);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("{...}") : NULL;
    }

    PyObject *dct = PyDict_New();
    if (dct == NULL) {
        Py_ReprLeave(self);
        return NULL;
    }

    if (PyDict_Update(dct, self) < 0) {
        Py_DECREF(dct);
        Py_ReprLeave(self);
        return NULL;
    }

    PyObject *repr = _PyValue_Repr(PyValue_FromObjectBorrow(dct));
    Py_DECREF(dct);

    Py_ReprLeave(self);

    return repr;
}

static PyObject*
framelocalsproxy_or(PyObject *self, PyObject *other)
{
    if (!PyDict_Check(other) && !PyFrameLocalsProxy_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *result = PyDict_New();
    if (result == NULL) {
        return NULL;
    }

    if (PyDict_Update(result, self) < 0) {
        Py_DECREF(result);
        return NULL;
    }

    if (PyDict_Update(result, other) < 0) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_inplace_or(PyObject *self, PyObject *other)
{
    if (!PyDict_Check(other) && !PyFrameLocalsProxy_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (framelocalsproxy_merge(self, other) < 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    return Py_NewRef(self);
}

static PyObject *
framelocalsproxy_values(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *values = PyList_New(0);
    if (values == NULL) {
        return NULL;
    }
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        Py_DECREF(values);
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
        if (value == NULL && PyErr_Occurred()) {
            goto error;
        }
        if (value) {
            PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
            int first = framelocalsproxy_is_first_occurrence(seen, name);
            if (first == 1) {
                if (PyList_AppendValue(values, PyValue_FromObjectBorrow(value)) < 0) {
                    Py_DECREF(value);
                    goto error;
                }
            }
            Py_DECREF(value);
            if (first < 0) {
                goto error;
            }
        }
    }
    Py_DECREF(seen);

    // Iterate through the extra locals
    if (frame->f_extra_locals) {
        Py_ssize_t j = 0;
        PyValue key = PyValue_NULL;
        PyValue value = PyValue_NULL;
        int rc;
        while ((rc = PyDict_NextValue(frame->f_extra_locals, &j, &key, &value)) > 0) {
            int err = PyList_AppendValue(values, value);
            PyValue_CLOSE(key);
            PyValue_CLOSE(value);
            if (err < 0) {
                Py_DECREF(values);
                return NULL;
            }
        }
        if (rc < 0) {
            Py_DECREF(values);
            return NULL;
        }
    }

    return values;

error:
    Py_DECREF(seen);
    Py_DECREF(values);
    return NULL;
}

static PyObject *
framelocalsproxy_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    PyObject *items = PyList_New(0);
    if (items == NULL) {
        return NULL;
    }
    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        Py_DECREF(items);
        return NULL;
    }

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
        PyObject *value = framelocalsproxy_getval(frame->f_frame, co, i);
        if (value == NULL && PyErr_Occurred()) {
            goto error;
        }

        if (value) {
            int first = framelocalsproxy_is_first_occurrence(seen, name);
            if (first == 1) {
                PyObject *pair = _PyTuple_FromPairTakeValue(PyValue_DUP(PyValue_FromObjectBorrow(name)), PyValue_PrepareOwned(value));
                if (pair == NULL) {
                    goto error;
                }
                if (_PyList_AppendTakeValue((PyListObject *)items, PyValue_PrepareOwned(pair)) < 0) {
                    goto error;
                }
            }
            else {
                Py_DECREF(value);
                if (first < 0) {
                    goto error;
                }
            }
        }
    }
    Py_DECREF(seen);
    seen = NULL;

    // Iterate through the extra locals
    if (frame->f_extra_locals) {
        Py_ssize_t j = 0;
        PyValue key = PyValue_NULL;
        PyValue value = PyValue_NULL;
        int rc;
        while ((rc = PyDict_NextValue(frame->f_extra_locals, &j, &key, &value)) > 0) {
            PyObject *pair = _PyTuple_PackValues(2, (PyValueBorrow[]){PyValue_Borrow(key), PyValue_Borrow(value)});
            PyValue_CLOSE(key);
            PyValue_CLOSE(value);
            if (pair == NULL) {
                goto error;
            }

            if (_PyList_AppendTakeValue((PyListObject *)items, PyValue_PrepareOwned(pair)) < 0) {
                goto error;
            }
        }
        if (rc < 0) {
            goto error;
        }
    }

    return items;

error:
    Py_XDECREF(seen);
    Py_DECREF(items);
    return NULL;
}

static Py_ssize_t
framelocalsproxy_length(PyObject *self)
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;
    PyCodeObject *co = _PyFrame_GetCode(frame->f_frame);
    Py_ssize_t size = 0;

    if (frame->f_extra_locals != NULL) {
        assert(PyDict_Check(frame->f_extra_locals));
        size += PyDict_Size(frame->f_extra_locals);
    }

    PyObject *seen = PySet_New(NULL);
    if (seen == NULL) {
        return -1;
    }
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        if (framelocalsproxy_hasval(frame->f_frame, co, i)) {
            PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
            int first = framelocalsproxy_is_first_occurrence(seen, name);
            if (first < 0) {
                Py_DECREF(seen);
                return -1;
            }
            else if (first) {
                size++;
            }
        }
    }
    Py_DECREF(seen);
    return size;
}

static int
framelocalsproxy_contains(PyObject *self, PyObject *key)
{
    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;

    int i = framelocalsproxy_getkeyindex(frame, key, true, NULL);
    if (i == -2) {
        return -1;
    }
    if (i >= 0) {
        return 1;
    }

    PyObject *extra = frame->f_extra_locals;
    if (extra != NULL) {
        return PyDict_ContainsValue(extra, PyValue_FromObjectBorrow(key));
    }

    return 0;
}

static PyObject* framelocalsproxy___contains__(PyObject *self, PyObject *key)
{
    int result = framelocalsproxy_contains(self, key);
    if (result < 0) {
        return NULL;
    }
    return PyBool_FromLong(result);
}

static PyObject*
framelocalsproxy_update(PyObject *self, PyObject *other)
{
    if (framelocalsproxy_merge(self, other) < 0) {
        PyErr_SetString(PyExc_TypeError, "update() argument must be dict or another FrameLocalsProxy");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
framelocalsproxy_get(PyObject* self, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "get expected 1 or 2 arguments");
        return NULL;
    }

    PyObject *key = args[0];
    PyObject *default_value = Py_None;

    if (nargs == 2) {
        default_value = args[1];
    }

    PyObject *result = framelocalsproxy_getitem(self, key);

    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_Clear();
            return Py_XNewRef(default_value);
        }
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_setdefault(PyObject* self, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "setdefault expected 1 or 2 arguments");
        return NULL;
    }

    PyObject *key = args[0];
    PyObject *default_value = Py_None;

    if (nargs == 2) {
        default_value = args[1];
    }

    PyObject *result = framelocalsproxy_getitem(self, key);

    if (result == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_Clear();
            if (framelocalsproxy_setitem(self, key, default_value) < 0) {
                return NULL;
            }
            return Py_XNewRef(default_value);
        }
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_pop(PyObject* self, PyObject *const *args, Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("pop", nargs, 1, 2)) {
        return NULL;
    }

    PyObject *key = args[0];
    PyObject *default_value = NULL;

    if (nargs == 2) {
        default_value = args[1];
    }

    PyFrameObject *frame = PyFrameLocalsProxyObject_CAST(self)->frame;

    int i = framelocalsproxy_getkeyindex(frame, key, false, NULL);
    if (i == -2) {
        return NULL;
    }

    if (i >= 0) {
        PyErr_SetString(PyExc_ValueError, "cannot remove local variables from FrameLocalsProxy");
        return NULL;
    }

    PyObject *result = NULL;

    if (frame->f_extra_locals == NULL) {
        if (default_value != NULL) {
            return Py_XNewRef(default_value);
        } else {
            _PyErr_SetKeyError(key);
            return NULL;
        }
    }

    PyValue popped;
    if (PyDict_PopValue(frame->f_extra_locals, PyValue_FromObjectBorrow(key), &popped) < 0) {
        return NULL;
    }

    if (!PyValue_IsNull(popped)) {
        result = _PyValue_AsObjectAt(popped, BOX_SITE_DICT);
        PyValue_CLOSE(popped);
        if (result == NULL) {
            return NULL;
        }
    }

    if (result == NULL) {
        if (default_value != NULL) {
            return Py_XNewRef(default_value);
        } else {
            _PyErr_SetKeyError(key);
            return NULL;
        }
    }

    return result;
}

static PyObject*
framelocalsproxy_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* result = PyDict_New();

    if (result == NULL) {
        return NULL;
    }

    if (PyDict_Update(result, self) < 0) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static PyObject*
framelocalsproxy_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *result = framelocalsproxy_keys(self, NULL);

    if (result == NULL) {
        return NULL;
    }

    if (PyList_Reverse(result) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

static PyNumberMethods framelocalsproxy_as_number = {
    .nb_or = framelocalsproxy_or,
    .nb_inplace_or = framelocalsproxy_inplace_or,
};

static PySequenceMethods framelocalsproxy_as_sequence = {
    .sq_contains = framelocalsproxy_contains,
};

static PyMappingMethods framelocalsproxy_as_mapping = {
    .mp_length = framelocalsproxy_length,
    .mp_subscript = framelocalsproxy_getitem,
    .mp_ass_subscript = framelocalsproxy_setitem,
};

static PyMethodDef framelocalsproxy_methods[] = {
    {"__contains__", framelocalsproxy___contains__, METH_O | METH_COEXIST, NULL},
    {"__getitem__", framelocalsproxy_getitem, METH_O | METH_COEXIST, NULL},
    {"update", framelocalsproxy_update, METH_O, NULL},
    {"__reversed__", framelocalsproxy_reversed, METH_NOARGS, NULL},
    {"copy", framelocalsproxy_copy, METH_NOARGS, NULL},
    {"keys", framelocalsproxy_keys, METH_NOARGS, NULL},
    {"values", framelocalsproxy_values, METH_NOARGS, NULL},
    {"items", _PyCFunction_CAST(framelocalsproxy_items), METH_NOARGS, NULL},
    {"get", _PyCFunction_CAST(framelocalsproxy_get), METH_FASTCALL, NULL},
    {"pop", _PyCFunction_CAST(framelocalsproxy_pop), METH_FASTCALL, NULL},
    {
        "setdefault",
        _PyCFunction_CAST(framelocalsproxy_setdefault),
        METH_FASTCALL,
        NULL
    },
    {NULL, NULL}   /* sentinel */
};

PyDoc_STRVAR(framelocalsproxy_doc,
"FrameLocalsProxy($frame)\n"
"--\n"
"\n"
"Create a write-through view of the locals dictionary for a frame.\n"
"\n"
"  frame\n"
"    the frame object to wrap.");

PyTypeObject PyFrameLocalsProxy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "FrameLocalsProxy",
    .tp_basicsize = sizeof(PyFrameLocalsProxyObject),
    .tp_dealloc = framelocalsproxy_dealloc,
    .tp_repr = &framelocalsproxy_repr,
    .tp_as_number = &framelocalsproxy_as_number,
    .tp_as_sequence = &framelocalsproxy_as_sequence,
    .tp_as_mapping = &framelocalsproxy_as_mapping,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_setattro = PyObject_GenericSetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_MAPPING,
    .tp_traverse = framelocalsproxy_visit,
    .tp_clear = framelocalsproxy_tp_clear,
    .tp_richcompare = framelocalsproxy_richcompare,
    .tp_iter = framelocalsproxy_iter,
    .tp_methods = framelocalsproxy_methods,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = framelocalsproxy_new,
    .tp_free = PyObject_GC_Del,
    .tp_doc = framelocalsproxy_doc,
};

PyObject *
_PyFrameLocalsProxy_New(PyFrameObject *frame)
{
    PyObject* args = _PyTuple_PackValues(1, (PyValueBorrow[]){_PyValue_FromObjectKnown(_PyObject_CAST(frame))});
    if (args == NULL) {
        return NULL;
    }

    PyObject* proxy = framelocalsproxy_new(&PyFrameLocalsProxy_Type, args, NULL);
    Py_DECREF(args);
    return proxy;
}

static PyMemberDef frame_memberlist[] = {
    {"f_trace_lines",   Py_T_BOOL,         OFF(f_trace_lines), 0},
    {NULL}      /* Sentinel */
};

/*[clinic input]
@critical_section
@getter
frame.f_locals as frame_locals

Return the mapping used by the frame to look up local variables.
[clinic start generated code]*/

static PyObject *
frame_locals_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=b4ace8bb4cae71f4 input=7bd444d0dc8ddf44]*/
{
    assert(!_PyFrame_IsIncomplete(self->f_frame));

    PyCodeObject *co = _PyFrame_GetCode(self->f_frame);

    if (!(co->co_flags & CO_OPTIMIZED) && !_PyFrame_HasHiddenLocals(self->f_frame)) {
        if (self->f_frame->f_locals == NULL) {
            // We found cases when f_locals is NULL for non-optimized code.
            // We fill the f_locals with an empty dict to avoid crash until
            // we find the root cause.
            self->f_frame->f_locals = PyDict_New();
            if (self->f_frame->f_locals == NULL) {
                return NULL;
            }
        }
        return Py_NewRef(self->f_frame->f_locals);
    }

    return _PyFrameLocalsProxy_New(self);
}

int
PyFrame_GetLineNumber(PyFrameObject *f)
{
    assert(f != NULL);
    if (f->f_lineno == -1) {
        // We should calculate it once. If we can't get the line number,
        // set f->f_lineno to 0.
        f->f_lineno = PyUnstable_InterpreterFrame_GetLine(f->f_frame);
        if (f->f_lineno < 0) {
            f->f_lineno = 0;
            return -1;
        }
    }

    if (f->f_lineno > 0) {
        return f->f_lineno;
    }
    return PyUnstable_InterpreterFrame_GetLine(f->f_frame);
}

/*[clinic input]
@critical_section
@getter
frame.f_lineno as frame_lineno

Return the current line number in the frame.
[clinic start generated code]*/

static PyObject *
frame_lineno_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=70f35de5ac7ad630 input=87b9ec648b742936]*/
{
    int lineno = PyFrame_GetLineNumber(self);
    if (lineno < 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(lineno);
}

/*[clinic input]
@critical_section
@getter
frame.f_lasti as frame_lasti

Return the index of the last attempted instruction in the frame.
[clinic start generated code]*/

static PyObject *
frame_lasti_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=03275b4f0327d1a2 input=0225ed49cb1fbeeb]*/
{
    int lasti = PyUnstable_InterpreterFrame_GetLasti(self->f_frame);
    if (lasti < 0) {
        return PyLong_FromLong(-1);
    }
    return PyLong_FromLong(lasti);
}

/*[clinic input]
@critical_section
@getter
frame.f_globals as frame_globals

Return the global variables in the frame.
[clinic start generated code]*/

static PyObject *
frame_globals_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=7758788c32885528 input=7fff7241357d314d]*/
{
    PyObject *globals = self->f_frame->f_globals;
    if (globals == NULL) {
        globals = Py_None;
    }
    return Py_NewRef(globals);
}

/*[clinic input]
@critical_section
@getter
frame.f_builtins as frame_builtins

Return the built-in variables in the frame.
[clinic start generated code]*/

static PyObject *
frame_builtins_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=45362faa6d42c702 input=27c696d6ffcad2c7]*/
{
    PyObject *builtins = self->f_frame->f_builtins;
    if (builtins == NULL) {
        builtins = Py_None;
    }
    return Py_NewRef(builtins);
}

/*[clinic input]
@getter
frame.f_code as frame_code

Return the code object being executed in this frame.
[clinic start generated code]*/

static PyObject *
frame_code_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=a5ed6207395a8cef input=e127e7098c124816]*/
{
    if (PySys_Audit("object.__getattr__", "Os", self, "f_code") < 0) {
        return NULL;
    }
    return (PyObject *)PyFrame_GetCode(self);
}

/*[clinic input]
@critical_section
@getter
frame.f_back as frame_back
[clinic start generated code]*/

static PyObject *
frame_back_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=3a84c22a55a63c79 input=9e528570d0e1f44a]*/
{
    PyObject *res = (PyObject *)PyFrame_GetBack(self);
    if (res == NULL && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return res;
}

/*[clinic input]
@critical_section
@getter
frame.f_trace_opcodes as frame_trace_opcodes

Return True if opcode tracing is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
frame_trace_opcodes_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=53ff41d09cc32e87 input=4eb91dc88e04677a]*/
{
    return self->f_trace_opcodes ? Py_True : Py_False;
}

/*[clinic input]
@critical_section
@setter
frame.f_trace_opcodes as frame_trace_opcodes
[clinic start generated code]*/

static int
frame_trace_opcodes_set_impl(PyFrameObject *self, PyObject *value)
/*[clinic end generated code: output=92619da2bfccd449 input=7e286eea3c0333ff]*/
{
    if (!PyBool_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute value type must be bool");
        return -1;
    }
    if (value == Py_True) {
        self->f_trace_opcodes = 1;
        if (self->f_trace) {
            return _PyEval_SetOpcodeTrace(self, true);
        }
    }
    else {
        self->f_trace_opcodes = 0;
        return _PyEval_SetOpcodeTrace(self, false);
    }
    return 0;
}

/* Physical successors retain relaxed IR_JUMP words as real instructions. */
static void
register_resume_successors(PyCodeObject *code, int i, int next[2])
{
    _PyIRWord word = code->co_words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    next[0] = i + 1;
    next[1] = -1;
    if ((fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) &&
        !(fmt->flags & _PyIR_NAMES_TARGET))
    {
        next[1] = fmt->flags & (_PyIR_FORWARD | _PyIR_FUSED)
            ? i + 1 + IR_DS(word) : (int)IR_CD(word);
    }
    if (IS_SCOPE_EXIT_OPCODE(_PyIR_HandlerBase[IR_HANDLER(word)]) ||
        (fmt->flags & _PyIR_JUMP_ALWAYS))
    {
        next[0] = -1;
    }
}

/* Debugger redirection must use the executable register contract. A slot
 * containing an unrelated value is not a reconstruction of a missing input.
 * This cold dataflow computes incoming owners for the recipes below. NULL
 * protocol inputs are omitted only under the whole-code invariant. */
static unsigned char *
register_resume_sites(_PyInterpreterFrame *frame, bool allow_null,
                      uint64_t **required)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int n = (int)Py_SIZE(code), nl = code->co_nlocalsplus;
    int nt = code->co_framesize - FRAME_SPECIALS_SIZE - nl;
    int nb = (nt + 63) / 64;
    size_t stride = (size_t)Py_MAX(nb, 1);
    if ((size_t)n > (16 * 1024 * 1024) / (stride * sizeof(uint64_t))) {
        PyErr_SetString(PyExc_ValueError, "register resume analysis is too large");
        return NULL;
    }
    uint64_t *live = PyMem_Calloc((size_t)Py_MAX(n, 1) * stride, sizeof(uint64_t));
    uint64_t *work = PyMem_Calloc(stride, sizeof(uint64_t));
    uint64_t *edge = PyMem_Calloc(stride, sizeof(uint64_t));
    unsigned char *safe = PyMem_Calloc((size_t)Py_MAX(n, 1), 1);
    unsigned char *null_home = PyMem_Calloc((size_t)Py_MAX(nt, 1), 1);
    if (live == NULL || work == NULL || edge == NULL || safe == NULL ||
        null_home == NULL)
    {
        PyMem_Free(live); PyMem_Free(work); PyMem_Free(edge); PyMem_Free(safe);
        PyMem_Free(null_home);
        PyErr_NoMemory();
        return NULL;
    }
    /* Entry initializes scratch to NULL. Admit only homes with no possible
     * boundary writer anywhere in this code, including transport writes.
     * CALL's private binding phase is excluded by the setter event gate;
     * active child overlap uses the separate, stricter analysis below. */
    if (allow_null) {
        memset(null_home, 1, (size_t)nt);
        for (int i = 0; i < n; i++) {
            _PyIRWord word = code->co_words[i];
            const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
            int arg = _PyCode_InstrOparg(code, i);
            int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
            uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
            int window = -1;
            for (int f = 0; f < 4; f++) {
                int role = fmt->roles[f];
                if (role >= _PyIR_ROLE_LOCAL_IN && role <= _PyIR_ROLE_CELL &&
                    fields[f] >= (uint32_t)nl)
                {
                    PyErr_SetString(PyExc_ValueError,
                                    "jump requires valid named-local operands");
                    goto error;
                }
                if (role == _PyIR_ROLE_BASE) {
                    window = (int)fields[f];
                }
            }
            for (int j = 0; j < nt; j++) {
                int home = nl + j;
                bool written = _PyIR_DefinesHome(word, arg, home);
                for (int taken = 0; taken < 2; taken++) {
                    _PyIRSlotOutcome outcome = _PyIR_Outcome(word, taken);
                    written |= outcome.semantic &&
                        ((home >= outcome.first &&
                          home < outcome.first + outcome.count) ||
                         home == outcome.dst);
                }
                if (opcode == SWAP && window >= 0) {
                    written |= home == window || home == window + arg - 1;
                }
                /* Unknown activation adapters can reuse the scratch tail. */
                if ((fmt->effects & _PyIR_PUSHES_FRAME) &&
                    opcode != CALL && opcode != CALL_KW &&
                    opcode != CALL_FUNCTION_EX)
                {
                    written |= window < 0 || home >= window;
                }
                if (written) {
                    null_home[j] = 0;
                }
            }
        }
        /* These facts describe boundary states, never an active child's
         * aliased argument cells. Validate the actual source snapshot too. */
        for (int j = 0; j < nt; j++) {
            if (null_home[j] && !PyValue_IsNull(frame->base[nl + j])) {
                null_home[j] = 0;
            }
        }
    }
    bool changed;
    do {
        changed = false;
        for (int i = n; i-- > 0;) {
            _PyIRWord word = code->co_words[i];
            const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
            memset(work, 0, stride * sizeof(uint64_t));
            int successors[2];
            register_resume_successors(code, i, successors);
            for (int k = 0; k < 2; k++) {
                int next = successors[k];
                if (next >= 0 && next < n) {
                    memcpy(edge, live + (size_t)next * stride, stride * sizeof(uint64_t));
                    _PyIRSlotOutcome outcome = _PyIR_Outcome(word, k == 1);
                    if (outcome.semantic) {
                        for (int r = 0; r <= outcome.count; r++) {
                            int slot = (r < outcome.count ? outcome.first + r : outcome.dst) - nl;
                            if (slot >= 0 && slot < nt && slot + nl != IR_NO_DST) {
                                edge[slot / 64] &= ~((uint64_t)1 << (slot % 64));
                            }
                        }
                    }
                    for (int j = 0; j < nb; j++) {
                        work[j] |= edge[j];
                    }
                }
            }
            int arg = _PyCode_InstrOparg(code, i);
            /* Conditional and semantic outputs are handled per edge above.
             * Other definitions use the actual generated output mapping. */
            if (!_PyIR_Outcome(word, 0).semantic &&
                !(fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)))
            {
                for (int j = 0; j < nt; j++) {
                    if (_PyIR_DefinesHome(word, arg, nl + j)) {
                        work[j / 64] &= ~((uint64_t)1 << (j % 64));
                    }
                }
            }
            /* Transparent observation can hide the runtime reader. Use its
             * recovered handler, never a family-wide NULL capability. */
            _PyInstr *instr = _PyFrame_InstrAt(frame, i);
            int handler = _PyIR_RealHandler(frame, instr);
            _PyIRWord actual = (word & ~(_PyIRWord)0xffff) | handler;
            for (int j = 0; j < nt; j++) {
                int home = nl + j;
                if (_PyIR_ReadsHome(word, arg, home) &&
                    !(null_home[j] && _PyIR_NullableHome(actual, arg, home)))
                {
                    work[j / 64] |= (uint64_t)1 << (j % 64);
                }
            }
            /* Immutable entry constants have a valid home at every site. */
            const uint32_t *constants = _PyCode_CONSTSLOTS(code);
            for (int j = 0; j < code->co_nconstslots; j++) {
                int slot = (int)constants[j * 2] - nl;
                if (slot >= 0 && slot < nt) {
                    work[slot / 64] &= ~((uint64_t)1 << (slot % 64));
                }
            }
            uint64_t *before = live + (size_t)i * stride;
            if (memcmp(before, work, stride * sizeof(uint64_t))) {
                memcpy(before, work, stride * sizeof(uint64_t));
                changed = true;
            }
        }
    } while (changed);
    for (int i = 0; i < n; i++) {
        safe[i] = 1;
        for (int j = 0; j < nb; j++) {
            safe[i] &= live[(size_t)i * stride + j] == 0;
        }
    }
    if (required != NULL) {
        *required = live;
    }
    else {
        PyMem_Free(live);
    }
    PyMem_Free(work); PyMem_Free(edge); PyMem_Free(null_home);
    return safe;
error:
    PyMem_Free(live); PyMem_Free(work); PyMem_Free(edge);
    PyMem_Free(null_home); PyMem_Free(safe);
    return NULL;
}

/* A pair survives only untouched physical operations. Even a read is a
 * conservative cut: COPY and moves cannot manufacture the original owner.
 * Unknown child overlap is a cut unless the pair lies below its window. */
static bool
register_anchor_cut(PyCodeObject *code, int i, int base)
{
    _PyIRWord word = code->co_words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int arg = _PyCode_InstrOparg(code, i);
    for (int home = base; home < base + 2; home++) {
        if (_PyIR_ReadsHome(word, arg, home) ||
            _PyIR_DefinesHome(word, arg, home))
        {
            return true;
        }
        for (int taken = 0; taken < 2; taken++) {
            _PyIRSlotOutcome outcome = _PyIR_Outcome(word, taken);
            if (outcome.semantic &&
                ((home >= outcome.first && home < outcome.first + outcome.count) ||
                 home == outcome.dst))
            {
                return true;
            }
        }
    }
    if (fmt->effects & _PyIR_PUSHES_FRAME) {
        uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
        for (int f = 0; f < 4; f++) {
            if (fmt->roles[f] == _PyIR_ROLE_BASE) {
                return (int)fields[f] <= base + 1;
            }
        }
        return true;
    }
    return false;
}

/* Establish a dynamic body phase, not a static producer identity. INVALID
 * propagates over every path except the candidate's successful body edge,
 * which establishes a new valid generation of its iterator/cursor pair. */
static bool
register_anchor_phase(PyCodeObject *code, int header, int base,
                      int current, int target, unsigned char *phase)
{
    int n = (int)Py_SIZE(code);
    memset(phase, 0, (size_t)n);
    phase[0] = 1;  /* reachable without a surviving candidate body anchor */
    bool changed;
    do {
        changed = false;
        for (int i = 0; i < n; i++) {
            if (!phase[i]) {
                continue;
            }
            int next[2];
            register_resume_successors(code, i, next);
            bool cut = register_anchor_cut(code, i, base);
            for (int k = 0; k < 2; k++) {
                int to = next[k];
                if (to < 0 || to >= n) {
                    continue;
                }
                unsigned char outgoing = cut ? 1 : phase[i];
                if (i == header) {
                    outgoing = k == 0 ? 2 : 1;
                }
                unsigned char merged = phase[to] | outgoing;
                if (merged != phase[to]) {
                    phase[to] = merged;
                    changed = true;
                }
            }
        }
    } while (changed);
    return phase[current] == 2 && phase[target] == 2;
}

/* The initial recipe is a backward replay inside one acyclic body slice.
 * First prove the target dominates current from body entry. Then require
 * every path from target to first current to avoid cuts and re-entry. */
static bool
register_anchor_slice(PyCodeObject *code, int header, int base,
                      int current, int target, unsigned char *seen, int *queue)
{
    int n = (int)Py_SIZE(code);
    for (int pass = 0; pass < 2; pass++) {
        memset(seen, 0, (size_t)n);
        int head = 0, tail = 0;
        int start = pass == 0 ? header + 1 : target;
        bool arrived = false;
        if (start < 0 || start >= n) {
            return false;
        }
        queue[tail++] = start;
        seen[start] = 1;
        while (head < tail) {
            int i = queue[head++];
            if (i == current) {
                if (pass == 0) {
                    return false;  /* body ingress bypasses target */
                }
                arrived = true;
                continue;
            }
            if (pass == 0 && i == target) {
                continue;
            }
            if (i == header || register_anchor_cut(code, i, base)) {
                if (pass == 1) {
                    return false;
                }
                continue;
            }
            int next[2];
            register_resume_successors(code, i, next);
            bool successor = false;
            for (int k = 0; k < 2; k++) {
                int to = next[k];
                if (to < 0 || to >= n) {
                    continue;
                }
                successor = true;
                if (pass == 1 && to <= i) {
                    return false;  /* no iteration/reinitialization replay */
                }
                if (!seen[to]) {
                    seen[to] = 1;
                    queue[tail++] = to;
                }
            }
            if (pass == 1 && !successor) {
                return false;
            }
        }
        if (pass == 1 && !arrived) {
            return false;
        }
    }
    return true;
}

static int
register_resume_recipe(_PyInterpreterFrame *frame, int current, int target,
                       unsigned char *safe, uint64_t *required,
                       bool allow_anchor, int *preserve)
{
    *preserve = -1;
    if (safe[current] && safe[target]) {
        return 1;
    }
    if (!allow_anchor || target >= current) {
        return 0;
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int n = (int)Py_SIZE(code), nl = code->co_nlocalsplus;
    int nt = code->co_framesize - FRAME_SPECIALS_SIZE - nl;
    size_t stride = (size_t)Py_MAX((nt + 63) / 64, 1);
    unsigned char *phase = NULL;
    int *queue = NULL;
    int result = 0;
    for (int header = 0; header < n; header++) {
        _PyIRWord word = code->co_words[header];
        if (_PyIR_BaseOpcode(IR_HANDLER(word)) != FOR_ITER) {
            continue;
        }
        int base = (int)IR_B(word);
        int item = (int)IR_A(word);
        if (base < nl || base + 1 >= nl + nt ||
            item == base || item == base + 1)
        {
            continue;
        }
        bool fits = true;
        for (int j = 0; j < nt; j++) {
            uint64_t bit = (uint64_t)1 << (j % 64);
            if ((required[(size_t)current * stride + j / 64] |
                 required[(size_t)target * stride + j / 64]) & bit)
            {
                fits &= nl + j == base || nl + j == base + 1;
            }
        }
        if (!fits || !PyValue_IsObject(frame->base[base]) ||
            !(PyValue_IsNull(frame->base[base + 1]) ||
              PyValue_IsTaggedIndex(frame->base[base + 1])))
        {
            continue;
        }
        PyObject *receiver = PyValue_AsKnownObjectBorrow(frame->base[base]);
        if (PyValue_IsNull(frame->base[base + 1])) {
            if (!PyIter_Check(receiver)) {
                continue;
            }
        }
        else if (PyValue_UntagIndex(frame->base[base + 1]) < 0 ||
                 Py_TYPE(receiver)->_tp_iteritem == NULL)
        {
            continue;
        }
        if (phase == NULL) {
            phase = PyMem_Malloc((size_t)n);
            queue = PyMem_New(int, n);
            if (phase == NULL || queue == NULL) {
                PyErr_NoMemory();
                result = -1;
                break;
            }
        }
        if (register_anchor_phase(code, header, base, current, target, phase) &&
            register_anchor_slice(code, header, base, current, target, phase, queue))
        {
            *preserve = base;
            result = 1;
            break;
        }
    }
    PyMem_Free(phase);
    PyMem_Free(queue);
    return result;
}

static int *
marklines(PyCodeObject *code, int len)
{
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(code, &bounds);
    assert (bounds.ar_end == 0);
    int last_line = -1;

    int *linestarts = PyMem_New(int, len);
    if (linestarts == NULL) {
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        linestarts[i] = -1;
    }

    while (_PyLineTable_NextAddressRange(&bounds)) {
        assert(bounds.ar_start < len);
        if (bounds.ar_line != last_line && bounds.ar_line != -1) {
            linestarts[bounds.ar_start] = bounds.ar_line;
            last_line = bounds.ar_line;
        }
    }
    return linestarts;
}

static int
first_line_not_before(int *lines, int len, int line)
{
    int result = INT_MAX;
    for (int i = 0; i < len; i++) {
        if (lines[i] < result && lines[i] >= line) {
            result = lines[i];
        }
    }
    if (result == INT_MAX) {
        return -1;
    }
    return result;
}

static bool frame_is_suspended(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    if (frame->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame->f_frame);
        return FRAME_STATE_SUSPENDED(gen->gi_frame_state);
    }
    return false;
}

/* Setter for f_lineno - you can set f_lineno from within a trace function in
 * order to jump to a given line of code, subject to some restrictions.  Most
 * lines are OK to jump to because they don't make any assumptions about the
 * state of the stack (obvious because you could remove the line and the code
 * would still work without any stack errors), but there are some constructs
 * that limit jumping:
 *
 *  o Any exception handlers.
 *  o 'for' and 'async for' loops can't be jumped into because the
 *    iterator needs to be on the stack.
 *  o Jumps cannot be made from within a trace function invoked with a
 *    'return' or 'exception' event since the eval loop has been exited at
 *    that time.
 */
/*[clinic input]
@critical_section
@setter
frame.f_lineno as frame_lineno
[clinic start generated code]*/

static int
frame_lineno_set_impl(PyFrameObject *self, PyObject *value)
/*[clinic end generated code: output=e64c86ff6be64292 input=36ed3c896b27fb91]*/
{
    PyCodeObject *code = _PyFrame_GetCode(self->f_frame);
    /* f_lineno must be an integer. */
    if (!PyLong_CheckExact(value)) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno must be an integer");
        return -1;
    }

    bool is_suspended = frame_is_suspended(self);
    /*
     * This code preserves the historical restrictions on
     * setting the line number of a frame.
     * Jumps are forbidden on a 'return' trace event (except after a yield).
     * Jumps from 'call' trace events are also forbidden.
     * In addition, jumps are forbidden when not tracing,
     * as this is a debugging feature.
     */
    int what_event = PyThreadState_GET()->what_event;
    if (what_event < 0) {
        PyErr_Format(PyExc_ValueError,
                    "f_lineno can only be set in a trace function");
        return -1;
    }
    switch (what_event) {
        case PY_MONITORING_EVENT_PY_RESUME:
        case PY_MONITORING_EVENT_JUMP:
        case PY_MONITORING_EVENT_BRANCH:
        case PY_MONITORING_EVENT_BRANCH_LEFT:
        case PY_MONITORING_EVENT_BRANCH_RIGHT:
        case PY_MONITORING_EVENT_LINE:
        case PY_MONITORING_EVENT_PY_YIELD:
            /* Setting f_lineno is allowed for the above events */
            break;
        case PY_MONITORING_EVENT_PY_START:
            PyErr_Format(PyExc_ValueError,
                     "can't jump from the 'call' trace event of a new frame");
            return -1;
        case PY_MONITORING_EVENT_CALL:
        case PY_MONITORING_EVENT_C_RETURN:
            PyErr_SetString(PyExc_ValueError,
                "can't jump during a call");
            return -1;
        case PY_MONITORING_EVENT_PY_RETURN:
        case PY_MONITORING_EVENT_PY_UNWIND:
        case PY_MONITORING_EVENT_PY_THROW:
        case PY_MONITORING_EVENT_RAISE:
        case PY_MONITORING_EVENT_C_RAISE:
        case PY_MONITORING_EVENT_INSTRUCTION:
        case PY_MONITORING_EVENT_EXCEPTION_HANDLED:
            PyErr_Format(PyExc_ValueError,
                "can only jump from a 'line' trace event");
            return -1;
        default:
            PyErr_SetString(PyExc_SystemError,
                "unexpected event type");
            return -1;
    }

    int new_lineno;

    /* Fail if the line falls outside the code block and
        select first line with actual code. */
    int overflow;
    long l_new_lineno = _PyValue_AsLongAndOverflow(PyValue_FromObjectBorrow(value), &overflow);
    if (overflow
#if SIZEOF_LONG > SIZEOF_INT
        || l_new_lineno > INT_MAX
        || l_new_lineno < INT_MIN
#endif
    ) {
        PyErr_SetString(PyExc_ValueError,
                        "lineno out of range");
        return -1;
    }
    new_lineno = (int)l_new_lineno;

    if (new_lineno < code->co_firstlineno) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes before the current code block",
                    new_lineno);
        return -1;
    }

    /* PyCode_NewWithPosOnlyArgs limits co_code to be under INT_MAX so this
     * should never overflow. */
    int len = (int)Py_SIZE(code);
    int *lines = marklines(code, len);
    if (lines == NULL) {
        return -1;
    }

    int requested_line = new_lineno;
    new_lineno = first_line_not_before(lines, len, requested_line);
    _PyIRSourceMap source = _PyIRSource_Stored(code->co_linetable);
    for (uint32_t j = 0; j < source.nrecords; j++) {
        _PyIRSourceEvent event;
        _PyIRSource_Event(&source, j, &event);
        if (event.line >= requested_line &&
            (new_lineno < 0 || event.line < new_lineno))
        {
            new_lineno = event.line;
        }
    }
    if (new_lineno < 0) {
        PyErr_Format(PyExc_ValueError,
                    "line %d comes after the current code block",
                    (int)l_new_lineno);
        PyMem_Free(lines);
        return -1;
    }

    /* Repeating the actual real-LINE phase needs no reconstruction, even
     * inside a protected region. Source-only callbacks publish a predecessor
     * and can carry another EH recipe, so logical offset equality is not this
     * proof. Keep every owner and the handled-exception link unchanged. */
    int identity_target = -1;
    for (int i = 0; i < len; i++) {
        if (lines[i] == new_lineno) {
            identity_target = i;
            break;
        }
    }
    _PyInterpreterFrame *frame_at_line = self->f_frame;
    int real_current = _PyInterpreterFrame_LASTI(frame_at_line);
    _PyThreadStateImpl *line_tstate = (_PyThreadStateImpl *)PyThreadState_GET();
    if (what_event == PY_MONITORING_EVENT_LINE && !is_suspended &&
        line_tstate->observed_frame == frame_at_line &&
        !frame_at_line->pending_redirect &&
        self->f_source_event < 0 && self->f_source_event_error < 0 &&
        real_current >= 0 && real_current < len &&
        identity_target == real_current &&
        IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*frame_at_line->instr_ptr)) ==
            INSTRUMENTED_LINE &&
        PyCode_Addr2Line(code, real_current) == new_lineno)
    {
        PyMem_Free(lines);
        if (self->f_lineno_generation == UINT64_MAX) {
            PyErr_SetString(PyExc_OverflowError, "debugger redirect generation exhausted");
            return -1;
        }
        frame_at_line->return_pc = frame_at_line->instr_ptr;
        self->f_line_next = -1;
        self->f_lineno = 0;
        self->f_lineno_generation++;
        return 0;
    }

    /* Protected and suspended resumptions need explicit owner/exception
     * recipes. Do not let a frontend stack projection authorize them. */
    if (is_suspended || PyBytes_GET_SIZE(code->co_exceptiontable)) {
        PyMem_Free(lines);
        PyErr_SetString(PyExc_ValueError,
                        "jump requires a suspended or protected owner recipe");
        return -1;
    }
    unsigned char *safe = NULL;
    uint64_t *required = NULL;
    int current = _PyInterpreterFrame_LASTI(self->f_frame);
    bool pending_call = false;
    _PyInterpreterFrame *observed =
        ((_PyThreadStateImpl *)PyThreadState_GET())->observed_frame;
    _PyInterpreterFrame *child = observed;
    if (observed != self->f_frame) {
        while (child != NULL && child->previous != self->f_frame) {
            child = child->previous;
        }
        if (child == NULL) {
            PyMem_Free(safe);
            PyMem_Free(lines);
            PyErr_SetString(PyExc_ValueError, "frame is not an active observer ancestor");
            return -1;
        }
    }
    else {
        child = NULL;
    }
    if (child != NULL) {
        int opcode = _PyIR_EventOpcode(_PyCode_InstrHandler(code, current));
        if (child->external || self->f_frame->return_slot != IR_NO_DST ||
            (opcode != CALL && opcode != CALL_KW))
        {
            PyMem_Free(safe);
            PyMem_Free(lines);
            PyErr_SetString(PyExc_ValueError,
                            "jump requires an unsupported active call recipe");
            return -1;
        }
        /* The child owns the argument window. Its successful completion,
         * rather than the consumed CALL input state, is the source state. */
        current = _PyFrame_InstrOffset(self->f_frame, self->f_frame->return_pc);
        pending_call = true;
    }
    if (!pending_call && self->f_source_event >= 0) {
        _PyIRSourceEvent event;
        _PyIRSource_Event(&source, (uint32_t)self->f_source_event, &event);
        current = event.to;
    }
    safe = register_resume_sites(self->f_frame, !pending_call, &required);
    if (safe == NULL) {
        PyMem_Free(lines);
        return -1;
    }
    int best_addr = -1, preserve = -1;
    int recipe = 0;
    if (current >= 0 && current < len) {
        for (int i = 0; i < len; i++) {
            if (lines[i] != new_lineno) {
                continue;
            }
            recipe = register_resume_recipe(self->f_frame, current, i, safe,
                                            required, !pending_call, &preserve);
            if (recipe != 0) {
                if (recipe > 0) {
                    best_addr = i;
                }
                break;
            }
        }
        for (uint32_t j = 0; best_addr < 0 && recipe >= 0 && j < source.nrecords; j++) {
            _PyIRSourceEvent event;
            _PyIRSource_Event(&source, j, &event);
            if (event.line == new_lineno && event.handler < 0) {
                recipe = register_resume_recipe(self->f_frame, current, event.to,
                                                safe, required, !pending_call,
                                                &preserve);
                if (recipe > 0) {
                    best_addr = event.to;
                    break;
                }
            }
        }
    }
    PyMem_Free(required);
    PyMem_Free(safe);
    PyMem_Free(lines);
    if (best_addr < 0) {
        if (recipe >= 0) {
            PyErr_SetString(PyExc_ValueError,
                            "jump requires an incoming temporary register owner");
        }
        return -1;
    }
    // Populate any NULL locals that the compiler might have "proven" to exist
    // in the new location. Rather than crashing or changing co_code, just bind
    // None instead:
    int unbound = 0;
    for (int i = 0; i < code->co_nlocalsplus; i++) {
        // Counting every unbound local is overly-cautious, but a full flow
        // analysis (like we do in the compiler) is probably too expensive:
        unbound += PyValue_IsNull(self->f_frame->base[i]);
    }
    if (self->f_lineno_generation == UINT64_MAX) {
        PyErr_SetString(PyExc_OverflowError, "debugger redirect generation exhausted");
        return -1;
    }
    _PyInterpreterFrame *frame = self->f_frame;
    _PyInstr *before_pc = frame->instr_ptr;
    _PyInstr *before_return = frame->return_pc;
    int before_event = self->f_source_event;
    uint64_t before_generation = self->f_lineno_generation;
    int before_pending = frame->pending_redirect;
    if (unbound) {
        const char *e = "assigning None to %d unbound local%s";
        const char *s = (unbound == 1) ? "" : "s";
        if (PyErr_WarnFormat(PyExc_RuntimeWarning, 0, e, unbound, s)) {
            return -1;
        }
        /* Warning hooks may execute Python and redirect this same frame.
         * Honor that redirect. Otherwise repeat the cold proof against the
         * current handlers and homes before mutating any owner. */
        if (self->f_frame != frame || frame->instr_ptr != before_pc ||
            frame->return_pc != before_return ||
            self->f_source_event != before_event ||
            self->f_lineno_generation != before_generation ||
            frame->pending_redirect != before_pending)
        {
            return 0;
        }
        safe = register_resume_sites(frame, !pending_call, &required);
        if (safe == NULL) {
            return -1;
        }
        int valid = register_resume_recipe(frame, current, best_addr, safe,
                                           required, !pending_call, &preserve);
        PyMem_Free(required);
        PyMem_Free(safe);
        if (valid < 0) {
            return -1;
        }
        if (!valid) {
            PyErr_SetString(PyExc_ValueError,
                            "jump owner state changed during warning");
            return -1;
        }
    }
    int nslots = code->co_framesize - FRAME_SPECIALS_SIZE;
    PyValue *retired = NULL;
    if (!pending_call) {
        retired = PyMem_New(PyValue, Py_MAX(nslots - code->co_nlocalsplus, 1));
        if (retired == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    /* No callback is possible between the final proof and the commit. */
    for (int i = 0; i < code->co_nlocalsplus; i++) {
        if (PyValue_IsNull(frame->base[i])) {
            frame->base[i] = PyValue_None;
        }
    }
    int nretired = 0;
    if (!pending_call) {
        const uint32_t *constants = _PyCode_CONSTSLOTS(code);
        for (int slot = code->co_nlocalsplus; slot < nslots; slot++) {
            bool constant = false;
            for (int j = 0; j < code->co_nconstslots; j++) {
                constant |= constants[j * 2] == (uint32_t)slot;
            }
            if (!constant &&
                !(preserve >= 0 && (slot == preserve || slot == preserve + 1))) {
                retired[nretired++] = frame->base[slot];
                frame->base[slot] = PyValue_NULL;
            }
        }
    }
    /* Commit the continuation before any closing callback. A suspended
     * caller's overlap remains untouched until its child retires. */
    self->f_lineno_generation++;
    self->f_lineno = 0;
    self->f_line_next = -1;
    self->f_frame->return_pc = _PyFrame_InstrAt(self->f_frame, best_addr);
    self->f_source_event = -1;
    self->f_source_event_error = -1;
    if (pending_call) {
        self->f_frame->pending_redirect = 1;
        return 0;
    }
    self->f_frame->instr_ptr = self->f_frame->return_pc;
    /* All surplus owners are detached before the first callback. A nested
     * redirect can now install new owners without later closes clearing them. */
    for (int i = 0; i < nretired; i++) {
        PyValue_XCLOSE(retired[i]);
    }
    PyMem_Free(retired);
    return 0;
}

/*[clinic input]
@permit_long_summary
@critical_section
@getter
frame.f_trace as frame_trace

Return the trace function for this frame, or None if no trace function is set.
[clinic start generated code]*/

static PyObject *
frame_trace_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=5475cbfce07826cd input=e4eacf2c68cac577]*/
{
    PyObject* trace = self->f_trace;
    if (trace == NULL) {
        trace = Py_None;
    }
    return Py_NewRef(trace);
}

/*[clinic input]
@permit_long_summary
@critical_section
@setter
@deleter
frame.f_trace as frame_trace
[clinic start generated code]*/

static int
frame_trace_set_impl(PyFrameObject *self, PyObject *value)
/*[clinic end generated code: output=d6fe08335cf76ae4 input=9fb7a5805196eae2]*/
{
    if (value == Py_None) {
        value = NULL;
    }
    if (value != self->f_trace) {
        Py_XSETREF(self->f_trace, Py_XNewRef(value));
        if (value != NULL && self->f_trace_opcodes) {
            return _PyEval_SetOpcodeTrace(self, true);
        }
    }
    return 0;
}

/*[clinic input]
@permit_long_summary
@critical_section
@getter
frame.f_generator as frame_generator

Return the generator or coroutine associated with this frame, or None.
[clinic start generated code]*/

static PyObject *
frame_generator_get_impl(PyFrameObject *self)
/*[clinic end generated code: output=97aeb2392562e55b input=3ffba57ba10f84be]*/
{
    if (self->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyObject *gen = (PyObject *)_PyGen_GetGeneratorFromFrame(self->f_frame);
        return Py_NewRef(gen);
    }
    Py_RETURN_NONE;
}


static PyGetSetDef frame_getsetlist[] = {
    FRAME_F_BACK_GETSETDEF
    FRAME_F_LOCALS_GETSETDEF
    FRAME_F_LINENO_GETSETDEF
    FRAME_F_TRACE_GETSETDEF
    FRAME_F_LASTI_GETSETDEF
    FRAME_F_GLOBALS_GETSETDEF
    FRAME_F_BUILTINS_GETSETDEF
    FRAME_F_CODE_GETSETDEF
    FRAME_F_TRACE_OPCODES_GETSETDEF
    FRAME_F_GENERATOR_GETSETDEF
    {0}
};

static void
frame_dealloc(PyObject *op)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the generator pointer */
    PyFrameObject *f = PyFrameObject_CAST(op);
    if (_PyObject_GC_IS_TRACKED(f)) {
        _PyObject_GC_UNTRACK(f);
    }

    FT_CLEAR_WEAKREFS(op, f->f_weakreflist);

    /* GH-106092: If f->f_frame was on the stack and we reached the maximum
     * nesting depth for deallocations, the trashcan may have delayed this
     * deallocation until after f->f_frame is freed. Avoid dereferencing
     * f->f_frame unless we know it still points to valid memory. */
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)f->_f_frame_data;

    /* Kill all local variables including specials, if we own them */
    if (f->f_frame == frame && frame->owner == FRAME_OWNED_BY_FRAME_OBJECT) {
        int nslots = _PyFrame_NumSlots(frame);
        PyValue_CLEAR(frame->f_executable);
        PyValue_CLEAR(frame->f_funcobj);
        Py_CLEAR(frame->f_locals);
        PyValue *locals = _PyFrame_GetLocalsArray(frame);
        PyValue *sp = locals + nslots;
        while (sp > locals) {
            sp--;
            PyValue_CLEAR(*sp);
        }
    }
    Py_CLEAR(f->f_back);
    Py_CLEAR(f->f_trace);
    Py_CLEAR(f->f_extra_locals);
    Py_CLEAR(f->f_locals_cache);
    Py_CLEAR(f->f_overwritten_fast_locals);
    PyObject_GC_Del(f);
}

static int
frame_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyFrameObject *f = PyFrameObject_CAST(op);
    Py_VISIT(f->f_back);
    Py_VISIT(f->f_trace);
    Py_VISIT(f->f_extra_locals);
    Py_VISIT(f->f_locals_cache);
    Py_VISIT(f->f_overwritten_fast_locals);
    if (f->f_frame->owner != FRAME_OWNED_BY_FRAME_OBJECT) {
        return 0;
    }
    assert(f->f_frame->frame_obj == NULL);
    return _PyFrame_Traverse(f->f_frame, visit, arg);
}

static int
frame_tp_clear(PyObject *op)
{
    PyFrameObject *f = PyFrameObject_CAST(op);
    f->f_line_next = -1;
    Py_CLEAR(f->f_trace);
    Py_CLEAR(f->f_extra_locals);
    Py_CLEAR(f->f_locals_cache);
    Py_CLEAR(f->f_overwritten_fast_locals);

    /* all slots */
    PyValue *locals = _PyFrame_GetLocalsArray(f->f_frame);
    PyValue *sp = locals + _PyFrame_NumSlots(f->f_frame);
    while (sp > locals) {
        sp--;
        PyValue_CLEAR(*sp);
    }
    Py_CLEAR(f->f_frame->f_locals);
    return 0;
}

/*[clinic input]
@critical_section
frame.clear

Clear all references held by the frame.
[clinic start generated code]*/

static PyValue
frame_clear_impl(PyFrameObject *self)
/*[clinic end generated code: output=67ecc9b5510d6379 input=c358f9cff5f9b681]*/
{
    if (self->f_frame->owner == FRAME_OWNED_BY_GENERATOR) {
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(self->f_frame);
        if (_PyGen_ClearFrame(gen) < 0) {
            return PyValue_NULL;
        }
    }
    else if (self->f_frame->owner == FRAME_OWNED_BY_THREAD) {
        PyErr_SetString(PyExc_RuntimeError,
                        "cannot clear an executing frame");
        return PyValue_NULL;
    }
    else {
        assert(self->f_frame->owner == FRAME_OWNED_BY_FRAME_OBJECT);
        (void)frame_tp_clear((PyObject *)self);
    }
    return PyValue_None;
}

/*[clinic input]
@critical_section
frame.__sizeof__

Return the size of the frame in memory, in bytes.
[clinic start generated code]*/

static PyValue
frame___sizeof___impl(PyFrameObject *self)
/*[clinic end generated code: output=c02939d1ade9b39e input=908f90a83e73131d]*/
{
    Py_ssize_t res;
    res = offsetof(PyFrameObject, _f_frame_data) + offsetof(_PyInterpreterFrame, owned_slots);
    PyCodeObject *code = _PyFrame_GetCode(self->f_frame);
    res += _PyFrame_NumSlotsForCodeObject(code) * sizeof(PyObject *);
    return PyValue_FromInt64(res);
}

/*[clinic input]
@critical_section
frame.__repr__
[clinic start generated code]*/

static PyObject *
frame_repr_impl(PyFrameObject *self)
/*[clinic end generated code: output=6dc361d24ba61076 input=cea0fd14e4749a55]*/
{
    int lineno = PyFrame_GetLineNumber(self);
    PyCodeObject *code = _PyFrame_GetCode(self->f_frame);
    return PyUnicode_FromFormat(
        "<frame at %p, file %R, line %d, code %S>",
        self, code->co_filename, lineno, code->co_name);
}

static PyMethodDef frame_methods[] = {
    FRAME_CLEAR_METHODDEF
    FRAME___SIZEOF___METHODDEF
    {NULL, NULL}  /* sentinel */
};

PyTypeObject PyFrame_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "frame",
    offsetof(PyFrameObject, _f_frame_data) +
    offsetof(_PyInterpreterFrame, owned_slots),
    sizeof(PyObject *),
    frame_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    frame_repr,                                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    frame_traverse,                             /* tp_traverse */
    frame_tp_clear,                             /* tp_clear */
    0,                                          /* tp_richcompare */
    OFF(f_weakreflist),                         /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    frame_methods,                              /* tp_methods */
    frame_memberlist,                           /* tp_members */
    frame_getsetlist,                           /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
};

static void
init_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
           PyFunctionObject *func, PyObject *locals)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    /* This is the direct heap initializer. Escape/generator destinations
     * instead receive a complete move and need no preliminary fill. */
    for (int i = 0; i < _PyFrame_NumSlotsForCodeObject(code); i++) {
        frame->owned_slots[i] = PyValue_NULL;
    }
    _PyFrame_Initialize(tstate, frame, PyValue_DUP(PyValue_FromObjectBorrow(func)),
                        Py_XNewRef(locals), code, frame->owned_slots, NULL);
}

PyFrameObject*
_PyFrame_New_NoTrack(PyCodeObject *code)
{
    CALL_STAT_INC(frame_objects_created);
    int slots = code->co_nlocalsplus + code->co_stacksize;
    PyFrameObject *f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, slots);
    if (f == NULL) {
        return NULL;
    }
    f->f_back = NULL;
    f->f_trace = NULL;
    f->f_trace_lines = 1;
    f->f_trace_opcodes = 0;
    f->f_lineno = 0;
    f->f_source_event = -1;
    f->f_source_event_error = -1;
    f->f_line_next = -1;
    f->f_line_previous = -1;
    f->f_line_epoch = 0;
    f->f_lineno_generation = 0;
    f->f_extra_locals = NULL;
    f->f_locals_cache = NULL;
    f->f_overwritten_fast_locals = NULL;
    f->f_weakreflist = NULL;
    return f;
}

/* Legacy API */
PyFrameObject*
PyFrame_New(PyThreadState *tstate, PyCodeObject *code,
            PyObject *globals, PyObject *locals)
{
    PyObject *builtins = _PyDict_LoadBuiltinsFromGlobals(globals);
    if (builtins == NULL) {
        return NULL;
    }
    PyFrameConstructor desc = {
        .fc_globals = globals,
        .fc_builtins = builtins,
        .fc_name = code->co_name,
        .fc_qualname = code->co_name,
        .fc_code = (PyObject *)code,
        .fc_defaults = NULL,
        .fc_kwdefaults = NULL,
        .fc_closure = NULL
    };
    PyFunctionObject *func = _PyFunction_FromConstructor(&desc);
    _Py_DECREF_BUILTINS(builtins);
    if (func == NULL) {
        return NULL;
    }
    if (_PyCode_GetExec(code) == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    PyFrameObject *f = _PyFrame_New_NoTrack(code);
    if (f == NULL) {
        Py_DECREF(func);
        return NULL;
    }
    init_frame(tstate, (_PyInterpreterFrame *)f->_f_frame_data, func, locals);
    f->f_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    f->f_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    // This frame needs to be "complete", so pretend that the first RESUME ran:
    f->f_frame->instr_ptr = f->f_frame->ir_words + code->_co_firsttraceable + 1;
    assert(!_PyFrame_IsIncomplete(f->f_frame));
    Py_DECREF(func);
    _PyObject_GC_TRACK(f);
    return f;
}

// Initialize frame free variables if needed
static void
frame_init_get_vars(_PyInterpreterFrame *frame)
{
    // COPY_FREE_VARS has no quickened forms, so no need to use _PyOpcode_Deopt
    // here:
    PyCodeObject *co = _PyFrame_GetCode(frame);
    int lasti = _PyInterpreterFrame_LASTI(frame);
    if (!(lasti < 0
          && _PyCode_InstrHandler(co, 0) == COPY_FREE_VARS
          && PyValue_FunctionCheck(frame->f_funcobj)))
    {
        /* Free vars are initialized */
        return;
    }

    /* Free vars have not been initialized -- Do that */
    PyFunctionObject *func = _PyFrame_GetFunction(frame);
    PyObject *closure = func->func_closure;
    int offset = PyUnstable_Code_GetFirstFree(co);
    for (int i = 0; i < co->co_nfreevars; ++i) {
        frame->base[offset + i] = PyValue_XDUP(PyTuple_GET_VALUE(closure, i));
    }
    // COPY_FREE_VARS doesn't have inline CACHEs, either:
    frame->instr_ptr = _PyFrame_InstrAt(frame, 0);
}


// Return 1 and set *pvalue to a new reference (NULL if the variable is
// unset), 0 if the variable is skipped, or -1 with an exception set.
static int
frame_get_var(_PyInterpreterFrame *frame, PyCodeObject *co, int i,
              PyObject **pvalue)
{
    _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

    /* If the namespace is unoptimized, then one of the
       following cases applies:
       1. It does not contain free variables, because it
          uses import * or is a top-level namespace.
       2. It is a class namespace.
       We don't want to accidentally copy free variables
       into the locals dict used by the class.
    */
    if (kind & CO_FAST_CONST) {
        return 0;
    }
    if (kind & CO_FAST_FREE && !(co->co_flags & CO_OPTIMIZED)) {
        return 0;
    }

    PyObject *value = NULL;
    {
        if (PyValue_IsInline(frame->base[i])) {
            value = _PyValue_AsObjectAt(frame->base[i], BOX_SITE_FRAME);
            if (value == NULL) {
                return -1;
            }
            *pvalue = value;
            return 1;
        }
        value = PyValue_AsKnownObjectBorrow(frame->base[i]);
        if (kind & CO_FAST_FREE) {
            // The cell was set by COPY_FREE_VARS.
            assert(value != NULL && PyCell_Check(value));
            PyValue cv = PyCell_GetValue(value);
            value = PyValue_IsNull(cv) ? NULL : _PyValue_AsObjectAt(cv, BOX_SITE_CELL);
            PyValue_XCLOSE(cv);
        }
        else if (kind & CO_FAST_CELL) {
            if (value != NULL) {
                if (PyCell_Check(value)) {
                    assert(!_PyFrame_IsIncomplete(frame));
                    PyValue cv = PyCell_GetValue(value);
                    value = PyValue_IsNull(cv) ? NULL : _PyValue_AsObjectAt(cv, BOX_SITE_CELL);
                    PyValue_XCLOSE(cv);
                }
                else {
                    // (likely) Otherwise it is an arg (kind & CO_FAST_LOCAL),
                    // with the initial value set when the frame was created...
                    // (unlikely) ...or it was set via the f_locals proxy.
                    Py_INCREF(value);
                }
            }
        }
        else {
            Py_XINCREF(value);
        }
    }
    if (value == NULL && PyErr_Occurred()) {
        return -1;
    }
    *pvalue = value;
    return 1;
}


bool
_PyFrame_HasHiddenLocals(_PyInterpreterFrame *frame)
{
    /*
     * This function returns if there are hidden locals introduced by PEP 709,
     * which are the isolated fast locals for inline comprehensions
     */
    PyCodeObject* co = _PyFrame_GetCode(frame);

    for (int i = 0; i < co->co_nlocalsplus; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);

        if (kind & CO_FAST_HIDDEN) {
            if (framelocalsproxy_hasval(frame, co, i)) {
                return true;
            }
        }
    }

    return false;
}


PyObject *
_PyFrame_GetLocals(_PyInterpreterFrame *frame)
{
    // We should try to avoid creating the FrameObject if possible.
    // So we check if the frame is a module or class level scope
    PyCodeObject *co = _PyFrame_GetCode(frame);

    if (!(co->co_flags & CO_OPTIMIZED) && !_PyFrame_HasHiddenLocals(frame)) {
        if (frame->f_locals == NULL) {
            // We found cases when f_locals is NULL for non-optimized code.
            // We fill the f_locals with an empty dict to avoid crash until
            // we find the root cause.
            frame->f_locals = PyDict_New();
            if (frame->f_locals == NULL) {
                return NULL;
            }
        }
        return Py_NewRef(frame->f_locals);
    }

    PyFrameObject* f = _PyFrame_GetFrameObject(frame);
    if (f == NULL) {
        return NULL;
    }

    return _PyFrameLocalsProxy_New(f);
}


PyObject *
PyFrame_GetVar(PyFrameObject *frame_obj, PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError, "name must be str, not %s",
                     Py_TYPE(name)->tp_name);
        return NULL;
    }

    _PyInterpreterFrame *frame = frame_obj->f_frame;
    frame_init_get_vars(frame);

    PyCodeObject *co = _PyFrame_GetCode(frame);
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        PyObject *var_name = _PyTuple_GET_OBJECT(co->co_localsplusnames, i);
        if (!_PyUnicode_Equal(var_name, name)) {
            continue;
        }

        PyObject *value;
        int r = frame_get_var(frame, co, i, &value);
        if (r < 0) {
            return NULL;
        }
        if (r == 0) {
            break;
        }
        if (value == NULL) {
            break;
        }
        return value;
    }

    PyErr_Format(PyExc_NameError, "variable %R does not exist", name);
    return NULL;
}


PyObject *
PyFrame_GetVarString(PyFrameObject *frame, const char *name)
{
    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }
    PyObject *value = PyFrame_GetVar(frame, name_obj);
    Py_DECREF(name_obj);
    return value;
}


int
PyFrame_FastToLocalsWithError(PyFrameObject *f)
{
    // Nothing to do here, as f_locals is now a write-through proxy in
    // optimized frames. Soft-deprecated, since there's no maintenance hassle.
    return 0;
}

void
PyFrame_FastToLocals(PyFrameObject *f)
{
    // Nothing to do here, as f_locals is now a write-through proxy in
    // optimized frames. Soft-deprecated, since there's no maintenance hassle.
    return;
}

void
PyFrame_LocalsToFast(PyFrameObject *f, int clear)
{
    // Nothing to do here, as f_locals is now a write-through proxy in
    // optimized frames. Soft-deprecated, since there's no maintenance hassle.
    return;
}

int
_PyFrame_IsEntryFrame(PyFrameObject *frame)
{
    assert(frame != NULL);
    _PyInterpreterFrame *f = frame->f_frame;
    assert(!_PyFrame_IsIncomplete(f));
    return f->previous && f->previous->owner == FRAME_OWNED_BY_INTERPRETER;
}

PyCodeObject *
PyFrame_GetCode(PyFrameObject *frame)
{
    assert(frame != NULL);
    PyObject *code;
    Py_BEGIN_CRITICAL_SECTION(frame);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    code = Py_NewRef(_PyFrame_GetCode(frame->f_frame));
    Py_END_CRITICAL_SECTION();
    return (PyCodeObject *)code;
}


PyFrameObject*
PyFrame_GetBack(PyFrameObject *frame)
{
    assert(frame != NULL);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    PyFrameObject *back = frame->f_back;
    if (back == NULL) {
        _PyInterpreterFrame *prev = frame->f_frame->previous;
        prev = _PyFrame_GetFirstComplete(prev);
        if (prev) {
            back = _PyFrame_GetFrameObject(prev);
            if (back == NULL) {
                return NULL;
            }
        }
    }
    return (PyFrameObject*)Py_XNewRef(back);
}

PyObject*
PyFrame_GetLocals(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_locals_get((PyObject *)frame, NULL);
}

PyObject*
PyFrame_GetGlobals(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_globals_get((PyObject *)frame, NULL);
}

PyObject*
PyFrame_GetBuiltins(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_builtins_get((PyObject *)frame, NULL);
}

int
PyFrame_GetLasti(PyFrameObject *frame)
{
    int ret;
    Py_BEGIN_CRITICAL_SECTION(frame);
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    int lasti = _PyInterpreterFrame_LASTI(frame->f_frame);
    ret = lasti;
    Py_END_CRITICAL_SECTION();
    return ret;
}

PyObject *
PyFrame_GetGenerator(PyFrameObject *frame)
{
    assert(!_PyFrame_IsIncomplete(frame->f_frame));
    return frame_generator_get((PyObject *)frame, NULL);
}
