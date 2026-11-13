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
#include "pycore_ir_exception.h"
#include "pycore_list.h"          // _PyList_AppendTakeRef()
#include "pycore_long_value.h"    // _PyValue_AsLongAndOverflow()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_opcode_utils.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_UINT64_RELAXED()
#include "pycore_setobject.h"     // PySet_AddValue()
#include "pycore_tstate.h"
#include "pycore_transition_roots.h" // _PyTransitionPrefix
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
    /* Project optimized forms through their frontend shadow before removing
     * instrumentation. Encoded family identity alone is not a CFG opcode. */
    int opcode = _PyIR_HandlerBase[_PyIR_BaseOpcode(IR_HANDLER(word))];
    if (IS_SCOPE_EXIT_OPCODE(opcode) ||
        (fmt->flags & _PyIR_JUMP_ALWAYS))
    {
        next[0] = -1;
    }
}

typedef struct {
    int from, to, next, reverse_next, floor;
    /* -1 is a pre-operation error; 0/1 is a committed normal outcome. */
    int phase;
} RegisterResumeEdge;

typedef struct {
    int n, nl, nt;
    size_t stride;
    unsigned char *safe;
    uint64_t *required;
    uint64_t *null_states, *value_required, *null_required;
    _PyIRExceptionEntry *handlers;
    int *context, *origins, *parents;
    int *save_tops, *save_parents, *save_columns;
    unsigned char *save_acyclic;
    int nsave_columns;
    int *edge_heads, *reverse_heads, nedges;
    RegisterResumeEdge *edges;
    int *source_heads, *source_next;
    _PyIRRecoveryEvent *source_events;
    uint32_t nsource;
    int *stopped_row;
} RegisterResumeAnalysis;

typedef struct {
    int local, home;
} RegisterLocalRestore;

typedef struct {
    uint64_t *preserve;
    size_t stride;
    RegisterLocalRestore *restores;
    int nrestores;
} RegisterResumePlan;

static void
register_resume_plan_clear(RegisterResumePlan *plan)
{
    PyMem_Free(plan->preserve);
    PyMem_Free(plan->restores);
    memset(plan, 0, sizeof(*plan));
}

static bool
register_plan_restores_local(const RegisterResumePlan *plan, int local)
{
    for (int i = 0; i < plan->nrestores; i++) {
        if (plan->restores[i].local == local) {
            return true;
        }
    }
    return false;
}

typedef struct {
    enum {
        REGISTER_RESUME_OTHER,
        REGISTER_RESUME_PRE_WORD,
        REGISTER_RESUME_COMMITTED_NEUTRAL,
        REGISTER_RESUME_COMMITTED,
    } kind;
    int pc;
    /* PRE/neutral views borrow exact site rows. Other committed views bind
     * request-owned transferred rows, never the merged destination rows. */
    int context;
    const int *save_tops, *origins;
    _PyIRObserverSpec scope;
} RegisterStoppedState;

static bool
register_committed_scope(_PyInterpreterFrame *frame,
                         const _PyIRObserverSpec *scope)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    uintptr_t first = (uintptr_t)frame->ir_words;
    uintptr_t site = (uintptr_t)scope->site;
    size_t size = (size_t)Py_SIZE(code) * sizeof(_PyIRWord);
    if (scope->phase != _PY_IR_OBSERVER_POST_COMMITTED ||
        scope->edge < 0 || scope->edge > 1 || site < first ||
        site - first >= size || (site - first) % sizeof(_PyIRWord))
    {
        return false;
    }
    int pc = (int)((site - first) / sizeof(_PyIRWord));
    _PyIRWord word = code->co_words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    _PyIRObservedHomes homes;
    if ((fmt->effects & (_PyIR_PUSHES_FRAME | _PyIR_SUSPENDS)) ||
        !_PyIR_ObservedHomes(word, scope->edge, code->co_nlocalsplus,
                            code->co_framesize - FRAME_SPECIALS_SIZE, &homes))
    {
        return false;
    }
    int successors[2];
    register_resume_successors(code, pc, successors);
    int next = successors[scope->edge];
    return next >= 0 && next < Py_SIZE(code) &&
           scope->next == _PyFrame_InstrAt(frame, next);
}

/* The common committed-scope proof has already checked the physical edge. */
static bool
register_neutral_jump_word(_PyIRWord word, int arg)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (_PyIR_ObserverOutcome(word, _PyIR_EDGE_TAKEN) != _PyIR_OBSERVER_FORMAT_VALUE ||
        !(fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) ||
        !(fmt->flags & _PyIR_JUMP_ALWAYS) ||
        (fmt->effects & (_PyIR_PUSHES_FRAME | _PyIR_SUSPENDS)) ||
        _PyIR_Extent(fmt->reads, arg) != 0 ||
        _PyIR_Extent(fmt->writes, arg) != 0 ||
        _PyIR_Extent(fmt->through, arg) != 0 ||
        _PyIR_Outcome(word, _PyIR_EDGE_TAKEN).semantic)
    {
        return false;
    }
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] != _PyIR_ROLE_NONE &&
            fmt->roles[f] != _PyIR_ROLE_IMM && fmt->roles[f] != _PyIR_ROLE_FB)
        {
            return false;
        }
    }
    return true;
}

/* Capture one proved stopped phase; bind its rows after analysis. Raw event
 * bits alone authorize neither a PRE view nor a committed edge. */
static RegisterStoppedState
register_stopped_state(PyFrameObject *self, int current, int what_event,
                       bool pending_call, _PyInterpreterFrame *observed)
{
    RegisterStoppedState phase = {.kind = REGISTER_RESUME_OTHER, .pc = current};
    _PyInterpreterFrame *frame = self->f_frame;
    _PyIRObserverSpec context;
    if (!_PyEval_GetObserverContext(PyThreadState_GET(), frame, what_event, &context)) {
        return phase;
    }
    bool committed = register_committed_scope(frame, &context);
    bool neutral_jump = false;
    if (committed && context.event == PY_MONITORING_EVENT_JUMP &&
        context.edge == _PyIR_EDGE_TAKEN)
    {
        int pc = _PyFrame_InstrOffset(frame, context.site);
        PyCodeObject *code = _PyFrame_GetCode(frame);
        neutral_jump = register_neutral_jump_word(code->co_words[pc],
                                                  _PyCode_InstrOparg(code, pc));
    }
    bool original_pre = context.phase == _PY_IR_OBSERVER_PRE_WORD &&
                        context.site == frame->instr_ptr &&
                        context.generation == self->f_lineno_generation;
    /* A completed setter publishes a fresh PRE landing before its closes.
     * The enclosing LINE/source adapter already honors generation changes;
     * its old input/output facts are no longer authority for this landing. */
    bool committed_pre = context.generation != self->f_lineno_generation &&
                         (context.phase == _PY_IR_OBSERVER_PRE_WORD ||
                          context.phase == _PY_IR_OBSERVER_SOURCE_POST ||
                          committed) &&
                         frame->instr_ptr == frame->return_pc;
    bool line_phase = what_event == PY_MONITORING_EVENT_LINE &&
                      (original_pre || committed_pre);
    bool fresh_committed_landing = committed && committed_pre;
    if ((line_phase || fresh_committed_landing) && observed == frame &&
        !pending_call && !frame->pending_redirect &&
        self->f_source_event < 0 && self->f_source_event_error < 0 &&
        current >= 0 && current < Py_SIZE(_PyFrame_GetCode(frame)) &&
        frame->instr_ptr == _PyFrame_InstrAt(frame, current) &&
        IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*frame->instr_ptr)) == INSTRUMENTED_LINE)
    {
        phase.kind = REGISTER_RESUME_PRE_WORD;
    }
    else if (neutral_jump && what_event == PY_MONITORING_EVENT_JUMP &&
             observed == frame && !pending_call && !frame->pending_redirect &&
             self->f_source_event < 0 && self->f_source_event_error < 0 &&
             context.generation == self->f_lineno_generation &&
             frame->instr_ptr == context.site && frame->return_pc == context.next &&
             current >= 0 && current < Py_SIZE(_PyFrame_GetCode(frame)) &&
             context.site == _PyFrame_InstrAt(frame, current))
    {
        phase.kind = REGISTER_RESUME_COMMITTED_NEUTRAL;
    }
    else if (committed && observed == frame && !pending_call &&
             !frame->pending_redirect && self->f_source_event < 0 &&
             self->f_source_event_error < 0 &&
             context.generation == self->f_lineno_generation &&
             frame->instr_ptr == context.site && frame->return_pc == context.next &&
             current >= 0 && current < Py_SIZE(_PyFrame_GetCode(frame)) &&
             context.site == _PyFrame_InstrAt(frame, current))
    {
        phase.kind = REGISTER_RESUME_COMMITTED;
        phase.scope = context;
    }
    return phase;
}

static void
register_resume_analysis_clear(RegisterResumeAnalysis *analysis)
{
    PyMem_Free(analysis->safe);
    PyMem_Free(analysis->required);
    PyMem_Free(analysis->null_states);
    PyMem_Free(analysis->handlers);
    PyMem_Free(analysis->context);
    PyMem_Free(analysis->origins);
    PyMem_Free(analysis->parents);
    PyMem_Free(analysis->save_tops);
    PyMem_Free(analysis->save_parents);
    PyMem_Free(analysis->save_columns);
    PyMem_Free(analysis->save_acyclic);
    PyMem_Free(analysis->edge_heads);
    PyMem_Free(analysis->reverse_heads);
    PyMem_Free(analysis->edges);
    PyMem_Free(analysis->source_heads);
    PyMem_Free(analysis->source_next);
    PyMem_Free(analysis->source_events);
    PyMem_Free(analysis->stopped_row);
    memset(analysis, 0, sizeof(*analysis));
}

static int
register_resume_handlers(PyCodeObject *code, RegisterResumeAnalysis *analysis)
{
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(code->co_exceptiontable);
    const unsigned char *cursor = semantic.exceptions;
    const unsigned char *end = cursor + semantic.exceptions_size;
    if (!_PyIR_ExceptionTableValid(cursor, end, analysis->n, analysis->nl,
                                   analysis->nl + analysis->nt))
    {
        PyErr_SetString(PyExc_ValueError, "jump requires a canonical exception table");
        return -1;
    }
    analysis->handlers = PyMem_Calloc((size_t)Py_MAX(analysis->n, 1),
                                      sizeof(*analysis->handlers));
    if (analysis->handlers == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (int i = 0; i < analysis->n; i++) {
        analysis->handlers[i].target = -1;
    }
    while (cursor < end) {
        _PyIRExceptionEntry entry;
        if (_PyIR_ExceptionNext(&cursor, end, &entry) < 0) {
            Py_FatalError("validated debugger exception table changed");
        }
        for (int i = entry.start; i < entry.end; i++) {
            analysis->handlers[i] = entry;
        }
    }
    return 0;
}

static void
register_resume_add_edge(RegisterResumeAnalysis *analysis, int from, int to,
                         int floor, int phase)
{
    int index = analysis->nedges++;
    analysis->edges[index] = (RegisterResumeEdge){from, to,
        analysis->edge_heads[from], analysis->reverse_heads[to], floor, phase};
    analysis->edge_heads[from] = analysis->reverse_heads[to] = index;
}

/* Decode and index immutable recovery records once per request. Both forward
 * ownership and reverse survival walks use the same sparse physical graph. */
static int
register_resume_graph(PyCodeObject *code, RegisterResumeAnalysis *analysis)
{
    _PyIRSemanticMap source = _PyIRSemantic_Stored(code->co_exceptiontable);
    size_t n = (size_t)Py_MAX(analysis->n, 1);
    size_t r = (size_t)source.nrecords;
    size_t budget = 16 * 1024 * 1024;
    if (n > budget / (3 * sizeof(RegisterResumeEdge) + 3 * sizeof(int)) ||
        r > budget / (sizeof(RegisterResumeEdge) + sizeof(_PyIRRecoveryEvent) + sizeof(int)) ||
        source.nrecords > INT_MAX)
    {
        PyErr_SetString(PyExc_ValueError, "debugger context graph is too large");
        return -1;
    }
    analysis->edge_heads = PyMem_New(int, n);
    analysis->reverse_heads = PyMem_New(int, n);
    analysis->source_heads = PyMem_New(int, n);
    analysis->edges = PyMem_New(RegisterResumeEdge, 3 * n + r);
    analysis->source_next = PyMem_New(int, Py_MAX(r, 1));
    analysis->source_events = PyMem_New(_PyIRRecoveryEvent, Py_MAX(r, 1));
    if (analysis->edge_heads == NULL || analysis->reverse_heads == NULL ||
        analysis->source_heads == NULL || analysis->edges == NULL ||
        analysis->source_next == NULL || analysis->source_events == NULL)
    {
        PyErr_NoMemory();
        return -1;
    }
    analysis->nsource = source.nrecords;
    for (int i = 0; i < analysis->n; i++) {
        analysis->edge_heads[i] = analysis->reverse_heads[i] = analysis->source_heads[i] = -1;
    }
    for (int i = 0; i < analysis->n; i++) {
        int next[2];
        register_resume_successors(code, i, next);
        for (int k = 0; k < 2; k++) {
            if (next[k] >= 0 && next[k] < analysis->n) {
                register_resume_add_edge(analysis, i, next[k], -1, k);
            }
        }
        _PyIRExceptionEntry handler = analysis->handlers[i];
        if (handler.target >= 0) {
            register_resume_add_edge(analysis, i, handler.target,
                                     handler.slot - handler.lasti, -1);
        }
    }
    for (uint32_t j = 0; j < source.nrecords; j++) {
        _PyIRRecoveryEvent *event = &analysis->source_events[j];
        _PyIRSemantic_Event(&source, j, event);
        analysis->source_next[j] = analysis->source_heads[event->from];
        analysis->source_heads[event->from] = (int)j;
        if (event->handler >= 0) {
            register_resume_add_edge(analysis, event->from, event->handler,
                                     event->exc_slot - event->lasti, event->edge);
        }
    }
    return 0;
}

/* Exception and lasti are supplied by unwind. Only preserved-prefix inputs
 * are requirements on the failing instruction's pre-success state. */
static void
register_resume_error_needs(RegisterResumeAnalysis *analysis, const uint64_t *live,
                             uint64_t *work,
                             int target, int slot, int lasti)
{
    if (target < 0) {
        return;
    }
    int prefix = slot - lasti - analysis->nl;
    const uint64_t *needed = live + (size_t)target * analysis->stride;
    for (int j = 0; j < prefix; j++) {
        if (needed[j / 64] & ((uint64_t)1 << (j % 64))) {
            work[j / 64] |= (uint64_t)1 << (j % 64);
        }
    }
}

enum {
    REGISTER_REQUIRE_ALL,
    REGISTER_REQUIRE_VALUE,
    REGISTER_REQUIRE_NULL_ONLY,
    REGISTER_REQUIRE_COUNT,
};

/* These are physical NULL facts, not owner identities. Only ordinary entry
 * initializes this analysis; resumed and child-overlap entries cannot borrow
 * that initialization. A successful transport always reads the old row. */
static int
register_resume_null_states(PyCodeObject *code, RegisterResumeAnalysis *analysis)
{
    int n = analysis->n, nt = analysis->nt, nl = analysis->nl;
    size_t stride = analysis->stride;
    if (n == 0) {
        return 0;
    }
    size_t budget = 16 * 1024 * 1024;
    /* Include the three requirement lanes and temporary work/queue storage. */
    if (stride > budget / (6 * sizeof(uint64_t))) {
        PyErr_SetString(PyExc_ValueError, "register resume analysis is too large");
        return -1;
    }
    size_t row_bytes = stride * sizeof(uint64_t);
    size_t node_bytes = (REGISTER_REQUIRE_COUNT + 1) * row_bytes + sizeof(int) + 2;
    if ((size_t)n > (budget - 6 * row_bytes) / node_bytes) {
        PyErr_SetString(PyExc_ValueError, "register resume analysis is too large");
        return -1;
    }
    analysis->null_states = PyMem_Calloc((size_t)n * stride, sizeof(uint64_t));
    uint64_t *work = PyMem_Calloc(stride, sizeof(uint64_t));
    int *queue = PyMem_New(int, n);
    unsigned char *reached = PyMem_Calloc((size_t)n, 1);
    unsigned char *queued = PyMem_Calloc((size_t)n, 1);
    if (analysis->null_states == NULL || work == NULL || queue == NULL ||
        reached == NULL || queued == NULL)
    {
        PyMem_Free(work); PyMem_Free(queue);
        PyMem_Free(reached); PyMem_Free(queued);
        PyErr_NoMemory();
        return -1;
    }
    uint64_t *entry = analysis->null_states;
    for (int j = 0; j < nt; j++) {
        entry[j / 64] |= (uint64_t)1 << (j % 64);
    }
    const uint32_t *constants = _PyCode_CONSTSLOTS(code);
    for (int j = 0; j < code->co_nconstslots; j++) {
        int slot = (int)constants[j * 2] - nl;
        if (slot >= 0 && slot < nt) {
            entry[slot / 64] &= ~((uint64_t)1 << (slot % 64));
        }
    }
    int head = 0, tail = 1 % n, count = 1;
    queue[0] = 0;
    reached[0] = queued[0] = 1;
    while (count) {
        int pc = queue[head];
        head = (head + 1) % n;
        count--;
        queued[pc] = 0;
        const uint64_t *before = analysis->null_states + (size_t)pc * stride;
        _PyIRWord word = code->co_words[pc];
        int arg = _PyCode_InstrOparg(code, pc);
        for (int e = analysis->edge_heads[pc]; e >= 0;
             e = analysis->edges[e].next)
        {
            RegisterResumeEdge edge = analysis->edges[e];
            memset(work, 0, stride * sizeof(uint64_t));
            /* Forget facts on all errors, including source errors. This does
             * not presume partial-error or inline callback publication. */
            if (edge.floor < 0 && edge.phase >= 0) {
                for (int j = 0; j < nt; j++) {
                    _PyIRPhysicalTransfer transfer = _PyIR_NormalPhysicalTransfer(
                        word, arg, edge.phase, nl + j, nl, nl + nt);
                    bool is_null = transfer.kind == _PyIR_PHYSICAL_SET_NULL;
                    int source = transfer.kind == _PyIR_PHYSICAL_TRANSPORT
                        ? transfer.source - nl : j;
                    if ((transfer.kind == _PyIR_PHYSICAL_PRESERVE ||
                         transfer.kind == _PyIR_PHYSICAL_TRANSPORT) &&
                        source >= 0 && source < nt)
                    {
                        is_null = (before[source / 64] >> (source % 64)) & 1;
                    }
                    if (is_null) {
                        work[j / 64] |= (uint64_t)1 << (j % 64);
                    }
                }
            }
            uint64_t *after = analysis->null_states + (size_t)edge.to * stride;
            bool changed = !reached[edge.to];
            for (size_t j = 0; j < stride; j++) {
                uint64_t value = reached[edge.to] ? after[j] & work[j] : work[j];
                changed |= value != after[j];
                after[j] = value;
            }
            reached[edge.to] = 1;
            if (changed && !queued[edge.to]) {
                queue[tail] = edge.to;
                tail = (tail + 1) % n;
                count++;
                queued[edge.to] = 1;
            }
        }
    }
    PyMem_Free(work); PyMem_Free(queue);
    PyMem_Free(reached); PyMem_Free(queued);
    return 0;
}

/* Map both demand kinds through the same simultaneous physical transport.
 * UNKNOWN is not an owner definition and cannot discharge a demand. */
static void
register_resume_transport_needs(_PyIRWord word, int arg, int outcome, int lane,
                               RegisterResumeAnalysis *analysis,
                               uint64_t *needs, uint64_t *mapped)
{
    int nl = analysis->nl, nt = analysis->nt;
    memcpy(mapped, needs, analysis->stride * sizeof(uint64_t));
    for (int j = 0; j < nt; j++) {
        _PyIRPhysicalTransfer transfer = _PyIR_NormalPhysicalTransfer(
            word, arg, outcome, nl + j, nl, nl + nt);
        if ((lane == REGISTER_REQUIRE_NULL_ONLY &&
             transfer.kind == _PyIR_PHYSICAL_SET_NULL) ||
            transfer.kind == _PyIR_PHYSICAL_TRANSPORT)
        {
            mapped[j / 64] &= ~((uint64_t)1 << (j % 64));
        }
    }
    for (int j = 0; j < nt; j++) {
        if (!(needs[j / 64] & ((uint64_t)1 << (j % 64)))) {
            continue;
        }
        _PyIRPhysicalTransfer transfer = _PyIR_NormalPhysicalTransfer(
            word, arg, outcome, nl + j, nl, nl + nt);
        int source = transfer.source - nl;
        if (transfer.kind == _PyIR_PHYSICAL_TRANSPORT && source >= 0 && source < nt) {
            mapped[source / 64] |= (uint64_t)1 << (source % 64);
        }
    }
    memcpy(needs, mapped, analysis->stride * sizeof(uint64_t));
}

/* Debugger redirection must use the executable register contract. A slot
 * containing an unrelated value is not a reconstruction of a missing input.
 * The strict lane omits NULL inputs only under the whole-code invariant.
 * Optional lanes retain pathwise NULL expectations for the bounded PRE Plan. */
static unsigned char *
register_resume_liveness(_PyInterpreterFrame *frame, bool allow_null,
                          RegisterResumeAnalysis *analysis)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int n = (int)Py_SIZE(code), nl = code->co_nlocalsplus;
    int nt = code->co_framesize - FRAME_SPECIALS_SIZE - nl;
    int nb = (nt + 63) / 64;
    size_t stride = (size_t)Py_MAX(nb, 1);
    if (stride > (16 * 1024 * 1024) / sizeof(uint64_t) ||
        (size_t)n > (16 * 1024 * 1024) / (stride * sizeof(uint64_t))) {
        PyErr_SetString(PyExc_ValueError, "register resume analysis is too large");
        return NULL;
    }
    int lanes = analysis->null_states != NULL ? REGISTER_REQUIRE_COUNT : 1;
    size_t rows = (size_t)Py_MAX(n, 1) * stride;
    uint64_t *live = PyMem_Calloc(rows * lanes, sizeof(uint64_t));
    uint64_t *work = PyMem_Calloc(stride, sizeof(uint64_t));
    uint64_t *edge = PyMem_Calloc(stride, sizeof(uint64_t));
    uint64_t *mapped = lanes > 1 ? PyMem_Calloc(stride, sizeof(uint64_t)) : NULL;
    unsigned char *safe = PyMem_Calloc((size_t)Py_MAX(n, 1), 1);
    unsigned char *null_home = PyMem_Calloc((size_t)Py_MAX(nt, 1), 1);
    if (live == NULL || work == NULL || edge == NULL ||
        (lanes > 1 && mapped == NULL) || safe == NULL ||
        null_home == NULL)
    {
        PyMem_Free(live); PyMem_Free(work); PyMem_Free(edge); PyMem_Free(safe);
        PyMem_Free(null_home);
        PyMem_Free(mapped);
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
        /* EH payload writes are real writers too. A home untouched by normal
         * words may still hold an exception or lasti on a protected path. */
        for (int i = 0; i < n; i++) {
            _PyIRExceptionEntry handler = analysis->handlers[i];
            if (handler.target >= 0) {
                null_home[handler.slot - nl] = 0;
                if (handler.lasti) {
                    null_home[handler.slot - nl - 1] = 0;
                }
            }
        }
        for (uint32_t r = 0; r < analysis->nsource; r++) {
            _PyIRRecoveryEvent event = analysis->source_events[r];
            if (event.handler >= 0) {
                null_home[event.exc_slot - nl] = 0;
                if (event.lasti) {
                    null_home[event.exc_slot - nl - 1] = 0;
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
    analysis->required = live;
    if (lanes == REGISTER_REQUIRE_COUNT) {
        analysis->value_required = live + REGISTER_REQUIRE_VALUE * rows;
        analysis->null_required = live + REGISTER_REQUIRE_NULL_ONLY * rows;
    }
    bool changed;
    do {
        changed = false;
        for (int i = n; i-- > 0;) {
            for (int lane = REGISTER_REQUIRE_ALL; lane < lanes; lane++) {
                uint64_t *lane_live = live + (size_t)lane * rows;
                _PyIRWord word = code->co_words[i];
                const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
                int arg = _PyCode_InstrOparg(code, i);
                memset(work, 0, stride * sizeof(uint64_t));
                int successors[2];
                register_resume_successors(code, i, successors);
                for (int k = 0; k < 2; k++) {
                    int next = successors[k];
                    if (next >= 0 && next < n) {
                        memcpy(edge, lane_live + (size_t)next * stride, stride * sizeof(uint64_t));
                        /* Source callbacks run after this normal outcome. Their
                         * error inputs pass through the same success definitions. */
                        for (int r = analysis->source_heads[i]; r >= 0;
                             r = analysis->source_next[r])
                        {
                            _PyIRRecoveryEvent event = analysis->source_events[r];
                            if (event.edge == k) {
                                register_resume_error_needs(analysis, lane_live, edge, event.handler,
                                                            event.exc_slot, event.lasti);
                            }
                        }
                        if (lane != REGISTER_REQUIRE_ALL) {
                            register_resume_transport_needs(word, arg, k, lane, analysis,
                                                            edge, mapped);
                        }
                        _PyIRSlotOutcome outcome = _PyIR_Outcome(word, k == 1);
                        if (outcome.semantic) {
                            for (int r = 0; r <= outcome.count; r++) {
                                int slot = (r < outcome.count ? outcome.first + r : outcome.dst) - nl;
                                if (slot >= 0 && slot < nt && slot + nl != IR_NO_DST) {
                                    if (lane == REGISTER_REQUIRE_VALUE && _PyIR_NormalPhysicalTransfer(
                                            word, arg, k, slot + nl, nl, nl + nt).kind ==
                                                _PyIR_PHYSICAL_SET_NULL)
                                    {
                                        continue;
                                    }
                                    edge[slot / 64] &= ~((uint64_t)1 << (slot % 64));
                                }
                            }
                        }
                        for (int j = 0; j < nb; j++) {
                            work[j] |= edge[j];
                        }
                    }
                }
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
                /* Ordinary exceptions happen before success definitions. Joining
                 * these last prevents a fresh destination from erasing old input
                 * requirements needed by the handler. */
                _PyIRExceptionEntry handler = analysis->handlers[i];
                register_resume_error_needs(analysis, lane_live, work, handler.target,
                                            handler.slot, handler.lasti);
                /* Transparent observation can hide the runtime reader. Use its
                 * recovered handler, never a family-wide NULL capability. */
                _PyInstr *instr = _PyFrame_InstrAt(frame, i);
                int real_handler = _PyIR_RealHandler(frame, instr);
                _PyIRWord actual = (word & ~(_PyIRWord)0xffff) | real_handler;
                for (int j = 0; j < nt; j++) {
                    int home = nl + j;
                    bool nullable = _PyIR_NullableHome(actual, arg, home);
                    bool path_null = analysis->null_states != NULL && nullable &&
                        (analysis->null_states[(size_t)i * stride + j / 64] &
                         ((uint64_t)1 << (j % 64)));
                    if (_PyIR_ReadsHome(word, arg, home) && !(null_home[j] && nullable) &&
                        (lane == REGISTER_REQUIRE_ALL ||
                         (lane == REGISTER_REQUIRE_VALUE && !path_null) ||
                         (lane == REGISTER_REQUIRE_NULL_ONLY && path_null)))
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
                uint64_t *before = lane_live + (size_t)i * stride;
                if (memcmp(before, work, stride * sizeof(uint64_t))) {
                    memcpy(before, work, stride * sizeof(uint64_t));
                    changed = true;
                }
            }
        }
    } while (changed);
    for (int i = 0; i < n; i++) {
        safe[i] = 1;
        for (int j = 0; j < nb; j++) {
            if (lanes == REGISTER_REQUIRE_COUNT) {
                /* Strict adapters retain every old requirement and every
                 * newly transported NULL/value requirement. */
                live[(size_t)i * stride + j] |=
                    live[REGISTER_REQUIRE_VALUE * rows + (size_t)i * stride + j] |
                    live[REGISTER_REQUIRE_NULL_ONLY * rows + (size_t)i * stride + j];
            }
            safe[i] &= live[(size_t)i * stride + j] == 0;
        }
    }
    PyMem_Free(work); PyMem_Free(edge); PyMem_Free(null_home); PyMem_Free(mapped);
    return safe;
error:
    PyMem_Free(live); PyMem_Free(work); PyMem_Free(edge);
    PyMem_Free(null_home); PyMem_Free(safe);
    PyMem_Free(mapped);
    analysis->required = NULL;
    return NULL;
}

enum { REGISTER_CONTEXT_UNREACHED = -1, REGISTER_CONTEXT_AMBIGUOUS = -2 };

/* Positive origin labels describe previous-exception values; negative labels
 * describe saved named-local states (which may be NULL). Neither owns Python
 * objects. The separate active contexts survive loss of every known home. */
static int
register_context_join(RegisterResumeAnalysis *analysis, int target,
                       int context, const int *save_tops, const int *origins)
{
    int *stored = analysis->origins + (size_t)target * analysis->nt;
    int *saved = analysis->save_tops + (size_t)target * analysis->nsave_columns;
    if (analysis->context[target] == REGISTER_CONTEXT_UNREACHED) {
        analysis->context[target] = context;
        memcpy(saved, save_tops, (size_t)analysis->nsave_columns * sizeof(int));
        memcpy(stored, origins, (size_t)analysis->nt * sizeof(int));
        return 1;
    }
    int changed = 0;
    if (analysis->context[target] != context &&
        analysis->context[target] != REGISTER_CONTEXT_AMBIGUOUS)
    {
        analysis->context[target] = REGISTER_CONTEXT_AMBIGUOUS;
        changed = 1;
    }
    for (int col = 0; col < analysis->nsave_columns; col++) {
        if (saved[col] != save_tops[col] && saved[col] != REGISTER_CONTEXT_AMBIGUOUS) {
            saved[col] = REGISTER_CONTEXT_AMBIGUOUS;
            changed = 1;
        }
    }
    for (int j = 0; j < analysis->nt; j++) {
        if (stored[j] && stored[j] != origins[j]) {
            stored[j] = 0;
            changed = 1;
        }
    }
    return changed;
}

static bool
register_origin_chain(const int *parents, int n, int node, int exclude, int limit)
{
    for (int depth = 0; depth < limit; depth++) {
        if (node == 0) {
            return true;
        }
        if (node < 0 || node > n || node == exclude) {
            return false;
        }
        node = parents[node - 1];
    }
    return node == 0;
}

static bool
register_context_chain(RegisterResumeAnalysis *analysis, int node, int exclude)
{
    return register_origin_chain(analysis->parents, analysis->n, node, exclude,
                                  CO_MAXBLOCKS);
}

static void
register_context_forget_reads(PyCodeObject *code, int pc,
                              RegisterResumeAnalysis *analysis, int *origins)
{
    _PyIRWord word = code->co_words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int arg = _PyCode_InstrOparg(code, pc);
    int base = -1;
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_BASE) {
            base = (int)fields[f];
        }
    }
    for (int j = 0; j < analysis->nt; j++) {
        int home = analysis->nl + j;
        if (_PyIR_ReadsHome(word, arg, home) ||
            ((fmt->effects & _PyIR_PUSHES_FRAME) && (base < 0 || home >= base)))
        {
            origins[j] = 0;
        }
    }
}

static int
register_context_normal(PyCodeObject *code, int pc, int taken,
                         RegisterResumeAnalysis *analysis, int context,
                         int *save_tops, int *origins)
{
    _PyIRWord word = code->co_words[pc];
    int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
    int arg = _PyCode_InstrOparg(code, pc);
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int base = -1;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_BASE ||
            (opcode == POP_EXCEPT && fmt->roles[f] == _PyIR_ROLE_IN0))
        {
            base = (int)fields[f] - analysis->nl;
        }
    }
    int local = -1, input = -1;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_LOCAL_MOVE ||
            fmt->roles[f] == _PyIR_ROLE_LOCAL_OUT)
        {
            local = (int)fields[f];
        }
        if (fmt->roles[f] == _PyIR_ROLE_IN0) {
            input = (int)fields[f] - analysis->nl;
        }
    }
    int input_origin = input >= 0 && input < analysis->nt ? origins[input] : 0;
    int old = base >= 0 && base < analysis->nt ? origins[base] : 0;
    int top = opcode == SWAP && base >= 0 && base < analysis->nt &&
              arg >= 2 && arg <= analysis->nt - base ? base + arg - 1 : -1;
    int old_top = top >= 0 && top < analysis->nt ? origins[top] : 0;
    if (opcode == SWAP && top < 0) {
        /* This proof tracks scratch-only permutations. A constructed window
         * crossing named locals cannot preserve a saved-owner label here. */
        memset(origins, 0, (size_t)analysis->nt * sizeof(int));
        for (int col = 0; col < analysis->nsave_columns; col++) {
            save_tops[col] = REGISTER_CONTEXT_AMBIGUOUS;
        }
        return REGISTER_CONTEXT_AMBIGUOUS;
    }
    if (opcode == POP_EXCEPT) {
        /* Use POP_EXCEPT's actual explicit register input. */
        if (context <= 0 || old != context ||
            !register_context_chain(analysis, context, -1))
        {
            context = REGISTER_CONTEXT_AMBIGUOUS;
        }
        else {
            context = analysis->parents[context - 1];
        }
    }
    if (opcode == STORE_FAST && input_origin < 0) {
        int col = local >= 0 && local < analysis->nl ? analysis->save_columns[local] : -1;
        if (col >= 0) {
            int top = save_tops[col];
            if (top <= 0 || input_origin != -top ||
                !register_origin_chain(analysis->save_parents, analysis->n, top, -1,
                                        analysis->n) ||
                _PyIR_BaseOpcode(IR_HANDLER(code->co_words[top - 1])) != LOAD_FAST_AND_CLEAR ||
                local != (int)IR_B(code->co_words[top - 1]))
            {
                /* Independent locals commute. Only a younger save of this
                 * same local prevents restoring an ancestor's token. */
                save_tops[col] = REGISTER_CONTEXT_AMBIGUOUS;
            }
            else {
                save_tops[col] = analysis->save_parents[top - 1];
            }
        }
        /* A token stored to a different, unsaved local does not restore its
         * original binding. Its original column remains active. */
    }
    if (opcode != COPY && opcode != SWAP) {
        register_context_forget_reads(code, pc, analysis, origins);
    }
    for (int j = 0; j < analysis->nt; j++) {
        if (_PyIR_DefinesHome(word, arg, analysis->nl + j)) {
            origins[j] = 0;
        }
    }
    _PyIRSlotOutcome outcome = _PyIR_Outcome(word, taken);
    if (outcome.semantic) {
        for (int j = 0; j < analysis->nt; j++) {
            int home = analysis->nl + j;
            if ((home >= outcome.first && home < outcome.first + outcome.count) ||
                home == outcome.dst)
            {
                origins[j] = 0;
            }
        }
    }
    if (opcode == COPY) {
        /* Actual COPY output is the new window tail; middle holes survive. */
        if (arg >= 1 && base >= 0 && base < analysis->nt &&
            arg < analysis->nt - base)
        {
            origins[base + arg] = old;
        }
    }
    else if (opcode == SWAP && base >= 0 && base < analysis->nt &&
             top < analysis->nt && top >= 0) {
        origins[base] = old_top;
        origins[top] = old;
    }
    else if (opcode == PUSH_EXC_INFO) {
        int node = pc + 1;
        int *parent = &analysis->parents[pc];
        if (context < 0 || !register_context_chain(analysis, context, node)) {
            *parent = REGISTER_CONTEXT_AMBIGUOUS;
        }
        else if (*parent == REGISTER_CONTEXT_UNREACHED) {
            *parent = context;
        }
        else if (*parent != context) {
            *parent = REGISTER_CONTEXT_AMBIGUOUS;
        }
        context = *parent < 0 ? REGISTER_CONTEXT_AMBIGUOUS : node;
        if (base >= 0 && base < analysis->nt) {
            origins[base] = context > 0 ? node : 0;
        }
    }
    if (opcode == LOAD_FAST_AND_CLEAR) {
        int node = pc + 1;
        int col = analysis->save_columns[local];
        int *parent = &analysis->save_parents[pc];
        if (save_tops[col] < 0 ||
            !register_origin_chain(analysis->save_parents, analysis->n,
                                    save_tops[col], node, analysis->n))
        {
            *parent = REGISTER_CONTEXT_AMBIGUOUS;
        }
        else if (*parent == REGISTER_CONTEXT_UNREACHED) {
            *parent = save_tops[col];
        }
        else if (*parent != save_tops[col]) {
            *parent = REGISTER_CONTEXT_AMBIGUOUS;
        }
        save_tops[col] = *parent < 0 ? REGISTER_CONTEXT_AMBIGUOUS : node;
        int dst = (int)IR_A(word) - analysis->nl;
        if (dst >= 0 && dst < analysis->nt) {
            /* A NULL saved value still has a binding-state token. Losing
             * its physical home never empties the active local column. */
            origins[dst] = save_tops[col] > 0 ? -node : 0;
        }
    }
    return context;
}

/* Bind one request-owned post-edge row. Token-creating transfers update the
 * analysis parent tables, so they need a different proof from this view. */
static int
register_stopped_state_bind(PyFrameObject *self, RegisterResumeAnalysis *analysis,
                            RegisterStoppedState *phase)
{
    int pc = phase->pc;
    if (pc < 0 || pc >= analysis->n) {
        return 0;
    }
    phase->context = analysis->context[pc];
    phase->save_tops = analysis->save_tops + (size_t)pc * analysis->nsave_columns;
    phase->origins = analysis->origins + (size_t)pc * analysis->nt;
    if (phase->kind != REGISTER_RESUME_COMMITTED) {
        return 1;
    }
    _PyInterpreterFrame *frame = self->f_frame;
    PyThreadState *tstate = PyThreadState_GET();
    RegisterStoppedState fresh = register_stopped_state(
        self, pc, tstate->what_event, false,
        ((_PyThreadStateImpl *)tstate)->observed_frame);
    if (fresh.kind != phase->kind || fresh.scope.site != phase->scope.site ||
        fresh.scope.next != phase->scope.next ||
        fresh.scope.edge != phase->scope.edge ||
        fresh.scope.generation != phase->scope.generation)
    {
        return 0;
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyIRWord word = code->co_words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (_PyIR_HandlerBase[IR_HANDLER(word)] == PUSH_EXC_INFO) {
        return 0;
    }
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_LOCAL_MOVE) {
            return 0;
        }
    }
    if (phase->context < 0) {
        return 0;
    }
    size_t columns = (size_t)analysis->nsave_columns;
    size_t count = columns + (size_t)analysis->nt;
    size_t alloc_count = count ? count : 1;
    if (count < columns || alloc_count > SIZE_MAX / sizeof(int)) {
        PyErr_NoMemory();
        return -1;
    }
    assert(analysis->stopped_row == NULL);
    analysis->stopped_row = PyMem_New(int, alloc_count);
    if (analysis->stopped_row == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    int *saved = analysis->stopped_row;
    int *origins = saved + columns;
    memcpy(saved, phase->save_tops, columns * sizeof(int));
    memcpy(origins, phase->origins, (size_t)analysis->nt * sizeof(int));
    int context = register_context_normal(code, pc, phase->scope.edge, analysis,
                                          phase->context, saved, origins);
    _PyIRObservedHomes homes;
    if (!_PyIR_ObservedHomes(word, phase->scope.edge, analysis->nl,
                            analysis->nl + analysis->nt, &homes))
    {
        return 0;
    }
    for (int k = 0; k < homes.count; k++) {
        if (homes.old_cut[k]) {
            origins[homes.home[k] - analysis->nl] = 0;
        }
    }
    /* This permission cancels expression inputs, not active binding or
     * exception protocols. Existing PRE/neutral adapters keep their proofs. */
    if (context != 0) {
        return 0;
    }
    for (int col = 0; col < analysis->nsave_columns; col++) {
        if (saved[col] != 0) {
            return 0;
        }
    }
    phase->context = context;
    phase->save_tops = saved;
    phase->origins = origins;
    return 1;
}

static void
register_context_unwind(RegisterResumeAnalysis *analysis, int *origins,
                         int slot, int lasti)
{
    for (int j = Py_MAX(slot - lasti - analysis->nl, 0); j < analysis->nt; j++) {
        origins[j] = 0;
    }
}

static int
register_resume_contexts(PyCodeObject *code, RegisterResumeAnalysis *analysis)
{
    int n = analysis->n, nt = analysis->nt;
    if ((size_t)Py_MAX(nt, 1) > (16 * 1024 * 1024) / sizeof(int) ||
        (size_t)Py_MAX(n, 1) > (16 * 1024 * 1024) /
                                    (sizeof(int) * (size_t)Py_MAX(nt, 1)))
    {
        PyErr_SetString(PyExc_ValueError, "debugger context analysis is too large");
        return -1;
    }
    size_t count = (size_t)Py_MAX(n, 1) * (size_t)Py_MAX(nt, 1);
    if ((size_t)Py_MAX(analysis->nl, 1) > (16 * 1024 * 1024) / sizeof(int)) {
        PyErr_SetString(PyExc_ValueError, "debugger saved-local map is too large");
        return -1;
    }
    analysis->save_columns = PyMem_New(int, Py_MAX(analysis->nl, 1));
    if (analysis->save_columns == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (int local = 0; local < analysis->nl; local++) {
        analysis->save_columns[local] = -1;
    }
    for (int i = 0; i < n; i++) {
        _PyIRWord word = code->co_words[i];
        if (_PyIR_BaseOpcode(IR_HANDLER(word)) == LOAD_FAST_AND_CLEAR) {
            int local = (int)IR_B(word);
            if (local < 0 || local >= analysis->nl) {
                PyErr_SetString(PyExc_ValueError, "jump requires valid saved-local operands");
                return -1;
            }
            if (analysis->save_columns[local] < 0) {
                analysis->save_columns[local] = analysis->nsave_columns++;
            }
        }
    }
    size_t save_stride = (size_t)Py_MAX(analysis->nsave_columns, 1);
    if (save_stride > (16 * 1024 * 1024) / sizeof(int) ||
        (size_t)Py_MAX(n, 1) > (16 * 1024 * 1024) / (sizeof(int) * save_stride)) {
        PyErr_SetString(PyExc_ValueError, "debugger saved-local analysis is too large");
        return -1;
    }
    analysis->context = PyMem_New(int, Py_MAX(n, 1));
    analysis->parents = PyMem_New(int, Py_MAX(n, 1));
    analysis->save_tops = PyMem_Calloc((size_t)Py_MAX(n, 1) * save_stride, sizeof(int));
    analysis->save_parents = PyMem_New(int, Py_MAX(n, 1));
    analysis->origins = PyMem_Calloc(count, sizeof(int));
    int *work = PyMem_Calloc((size_t)Py_MAX(nt, 1), sizeof(int));
    int *error = PyMem_Calloc((size_t)Py_MAX(nt, 1), sizeof(int));
    int *saved_work = PyMem_Calloc(save_stride, sizeof(int));
    if (analysis->context == NULL || analysis->parents == NULL ||
        analysis->origins == NULL || analysis->save_tops == NULL ||
        analysis->save_parents == NULL || work == NULL || error == NULL || saved_work == NULL)
    {
        PyMem_Free(work); PyMem_Free(error); PyMem_Free(saved_work);
        PyErr_NoMemory();
        return -1;
    }
    for (int i = 0; i < n; i++) {
        analysis->context[i] = analysis->parents[i] = REGISTER_CONTEXT_UNREACHED;
        analysis->save_parents[i] = REGISTER_CONTEXT_UNREACHED;
    }
    if (n) {
        analysis->context[0] = 0;
    }
    bool changed;
    do {
        changed = false;
        for (int i = 0; i < n; i++) {
            int context = analysis->context[i];
            if (context == REGISTER_CONTEXT_UNREACHED) {
                continue;
            }
            const int *saved_before = analysis->save_tops + (size_t)i * analysis->nsave_columns;
            const int *before = analysis->origins + (size_t)i * nt;
            _PyIRExceptionEntry handler = analysis->handlers[i];
            if (handler.target >= 0) {
                memcpy(error, before, (size_t)nt * sizeof(int));
                register_context_forget_reads(code, i, analysis, error);
                register_context_unwind(analysis, error, handler.slot, handler.lasti);
                changed |= register_context_join(analysis, handler.target, context,
                                                  saved_before, error);
            }
            int next[2];
            register_resume_successors(code, i, next);
            for (int k = 0; k < 2; k++) {
                if (next[k] < 0 || next[k] >= n) {
                    continue;
                }
                memcpy(work, before, (size_t)nt * sizeof(int));
                memcpy(saved_work, saved_before,
                       (size_t)analysis->nsave_columns * sizeof(int));
                int after = register_context_normal(code, i, k, analysis, context,
                                                      saved_work, work);
                changed |= register_context_join(analysis, next[k], after, saved_work, work);
                for (int r = analysis->source_heads[i]; r >= 0;
                     r = analysis->source_next[r])
                {
                    _PyIRRecoveryEvent event = analysis->source_events[r];
                    if (event.edge == k && event.handler >= 0) {
                        memcpy(error, work, (size_t)nt * sizeof(int));
                        register_context_unwind(analysis, error, event.exc_slot, event.lasti);
                        changed |= register_context_join(analysis, event.handler, after,
                                                      saved_work, error);
                    }
                }
            }
        }
    } while (changed);
    PyMem_Free(work); PyMem_Free(error); PyMem_Free(saved_work);
    return 0;
}

static int
register_resume_analyze(_PyInterpreterFrame *frame, bool allow_null, bool path_null,
                        RegisterResumeAnalysis *analysis)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    *analysis = (RegisterResumeAnalysis){0};
    if (Py_SIZE(code) > INT_MAX) {
        PyErr_SetString(PyExc_ValueError, "debugger context analysis is too large");
        return -1;
    }
    analysis->n = (int)Py_SIZE(code);
    analysis->nl = code->co_nlocalsplus;
    analysis->nt = code->co_framesize - FRAME_SPECIALS_SIZE - analysis->nl;
    analysis->stride = (size_t)Py_MAX((analysis->nt + 63) / 64, 1);
    for (int i = 0; i < analysis->n; i++) {
        _PyIRWord word = code->co_words[i];
        const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
        uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
        for (int f = 0; f < 4; f++) {
            if (fmt->roles[f] >= _PyIR_ROLE_LOCAL_IN &&
                fmt->roles[f] <= _PyIR_ROLE_CELL &&
                fields[f] >= (uint32_t)analysis->nl)
            {
                PyErr_SetString(PyExc_ValueError, "jump requires valid named-local operands");
                return -1;
            }
        }
    }
    if (register_resume_handlers(code, analysis) < 0 ||
        register_resume_graph(code, analysis) < 0)
    {
        goto error;
    }
    if (path_null && (code->co_flags & CO_OPTIMIZED) && !(code->co_flags &
                      (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) &&
        register_resume_null_states(code, analysis) < 0)
    {
        goto error;
    }
    analysis->safe = register_resume_liveness(frame, allow_null, analysis);
    if (analysis->safe == NULL || register_resume_contexts(code, analysis) < 0) {
        goto error;
    }
    return 0;
error:
    register_resume_analysis_clear(analysis);
    return -1;
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

static bool
register_plan_has(const RegisterResumePlan *plan, int relative)
{
    return (plan->preserve[relative / 64] & ((uint64_t)1 << (relative % 64))) != 0;
}

static bool
register_context_cut(PyCodeObject *code, RegisterResumeAnalysis *analysis,
                      RegisterResumePlan *plan, int pc)
{
    _PyIRWord word = code->co_words[pc];
    int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
    if (opcode == PUSH_EXC_INFO || opcode == POP_EXCEPT) {
        return true;
    }
    for (int j = 0; j < analysis->nt; j++) {
        if (register_plan_has(plan, j)) {
            int home = analysis->nl + j;
            int arg = _PyCode_InstrOparg(code, pc);
            if (_PyIR_ReadsHome(word, arg, home) || _PyIR_DefinesHome(word, arg, home)) {
                return true;
            }
            for (int k = 0; k < 2; k++) {
                _PyIRSlotOutcome outcome = _PyIR_Outcome(word, k);
                if (outcome.semantic &&
                    ((home >= outcome.first && home < outcome.first + outcome.count) ||
                     home == outcome.dst))
                {
                    return true;
                }
            }
            const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
            if (fmt->effects & _PyIR_PUSHES_FRAME) {
                uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
                int base = -1;
                for (int f = 0; f < 4; f++) {
                    if (fmt->roles[f] == _PyIR_ROLE_BASE) {
                        base = (int)fields[f];
                    }
                }
                if (base < 0 || home >= base) {
                    return true;
                }
            }
        }
    }
    return false;
}

static int
register_caught_slice(PyCodeObject *code, RegisterResumeAnalysis *analysis,
                      RegisterResumePlan *plan, int current, int target, int node,
                      bool forward)
{
    int n = analysis->n;
    int first = forward ? current : target;
    int stop = forward ? target : current;
    unsigned char *reach = PyMem_Calloc((size_t)Py_MAX(n, 1), 1);
    unsigned char *seen = PyMem_Calloc((size_t)Py_MAX(n, 1), 1);
    int *queue = PyMem_New(int, Py_MAX(n, 1));
    if (reach == NULL || seen == NULL || queue == NULL) {
        PyMem_Free(reach); PyMem_Free(seen); PyMem_Free(queue);
        PyErr_NoMemory();
        return -1;
    }
    /* Only paths capable of reaching this slice's landing matter. */
    int head = 0, tail = 0;
    reach[stop] = 1;
    queue[tail++] = stop;
    while (head < tail) {
        int to = queue[head++];
        for (int e = analysis->reverse_heads[to]; e >= 0;
             e = analysis->edges[e].reverse_next)
        {
            int from = analysis->edges[e].from;
            if (!reach[from]) {
                reach[from] = 1;
                queue[tail++] = from;
            }
        }
    }
    int result = 0;
    if (!reach[first]) {
        goto done;
    }
    if (!forward) {
        /* A backward target must dominate this arrival after the caught PUSH. */
        int start = node;  /* node == PUSH pc + 1 */
        if (start >= n) {
            goto done;
        }
        head = tail = 0;
        seen[start] = 1;
        queue[tail++] = start;
        while (head < tail) {
            int i = queue[head++];
            if (i == target) {
                continue;
            }
            if (i == current) {
                goto done;
            }
            for (int e = analysis->edge_heads[i]; e >= 0;
                 e = analysis->edges[e].next)
            {
                int j = analysis->edges[e].to;
                if (!seen[j] && reach[j]) {
                    seen[j] = 1;
                    queue[tail++] = j;
                }
            }
        }
    }
    memset(seen, 0, (size_t)n);
    head = tail = 0;
    seen[first] = 1;
    queue[tail++] = first;
    while (head < tail) {
        int i = queue[head++];
        if (i == stop) {
            continue;
        }
        if (analysis->context[i] != node || register_context_cut(code, analysis, plan, i)) {
            goto done;
        }
        for (int e = analysis->edge_heads[i]; e >= 0; e = analysis->edges[e].next) {
            int j = analysis->edges[e].to;
            if (!reach[j]) {
                continue;
            }
            if (forward && (analysis->edges[e].phase < 0 ||
                            analysis->edges[e].floor >= 0))
            {
                goto done;  /* Forward survival is proved only on normal edges. */
            }
            if (j <= i) {
                goto done;  /* repeated producer/iteration is not the same owner */
            }
            int floor = analysis->edges[e].floor;
            for (int h = 0; h < analysis->nt; h++) {
                if (register_plan_has(plan, h) && floor >= 0 &&
                    analysis->nl + h >= floor)
                {
                    goto done;
                }
            }
            if (!seen[j]) {
                seen[j] = 1;
                queue[tail++] = j;
            }
        }
    }
    result = 1;
done:
    PyMem_Free(reach); PyMem_Free(seen); PyMem_Free(queue);
    return result;
}

/* A static producer token identifies one dynamic binding only if that
 * producer cannot reenter. Cyclic producers need a stronger generation witness
 * before their saved values can be used for restoration. */
static int
register_save_acyclic(RegisterResumeAnalysis *analysis, int producer)
{
    int n = analysis->n;
    if (analysis->save_acyclic == NULL) {
        analysis->save_acyclic = PyMem_Calloc((size_t)Py_MAX(n, 1), 1);
        if (analysis->save_acyclic == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    if (analysis->save_acyclic[producer]) {
        return analysis->save_acyclic[producer] == 1;
    }
    if ((size_t)n > SIZE_MAX / sizeof(int)) {
        PyErr_NoMemory();
        return -1;
    }
    unsigned char *seen = PyMem_Calloc((size_t)Py_MAX(n, 1), 1);
    int *queue = PyMem_New(int, Py_MAX(n, 1));
    if (seen == NULL || queue == NULL) {
        PyMem_Free(seen); PyMem_Free(queue);
        PyErr_NoMemory();
        return -1;
    }
    int head = 0, tail = 0;
    seen[producer] = 1;
    queue[tail++] = producer;
    int result = 1;
    while (head < tail && result) {
        int pc = queue[head++];
        for (int edge = analysis->edge_heads[pc]; edge >= 0;
             edge = analysis->edges[edge].next)
        {
            int next = analysis->edges[edge].to;
            if (next == producer) {
                result = 0;
                break;
            }
            if (!seen[next]) {
                seen[next] = 1;
                queue[tail++] = next;
            }
        }
    }
    analysis->save_acyclic[producer] = result ? 1 : 2;
    PyMem_Free(seen); PyMem_Free(queue);
    return result;
}

/* Restoring a genuine unbound local must not expose it to a borrow whose
 * original control flow proved the binding existed. Keep that obligation on
 * every path until an encoded successful outcome guarantees a bound value.
 * Ordinary errors precede the definition; source callback errors follow it. */
static int
register_unbound_target_safe(PyCodeObject *code, RegisterResumeAnalysis *analysis,
                             int target, int local)
{
    size_t n = (size_t)Py_MAX(analysis->n, 1);
    if (n > SIZE_MAX / sizeof(int)) {
        PyErr_NoMemory();
        return -1;
    }
    unsigned char *seen = PyMem_Calloc(n, 1);
    int *queue = PyMem_New(int, n);
    if (seen == NULL || queue == NULL) {
        PyMem_Free(seen); PyMem_Free(queue);
        PyErr_NoMemory();
        return -1;
    }
    int head = 0, tail = 0, result = 1;
    seen[target] = 1;
    queue[tail++] = target;
    while (head < tail && result) {
        int pc = queue[head++];
        _PyIRWord word = code->co_words[pc];
        int arg = _PyCode_InstrOparg(code, pc);
        if (_PyIR_BindingRead(word, arg, local) == _PyIR_BINDING_REQUIRES_VALUE) {
            result = 0;
            break;
        }
        for (int e = analysis->edge_heads[pc]; e >= 0;
             e = analysis->edges[e].next)
        {
            RegisterResumeEdge edge = analysis->edges[e];
            if (edge.phase >= 0 &&
                _PyIR_DefinesBinding(word, arg, edge.phase, local))
            {
                continue;
            }
            if (!seen[edge.to]) {
                seen[edge.to] = 1;
                queue[tail++] = edge.to;
            }
        }
    }
    PyMem_Free(seen); PyMem_Free(queue);
    return result;
}

static int
register_saved_locals_plan(_PyInterpreterFrame *frame, int target,
                           RegisterResumeAnalysis *analysis, RegisterStoppedState state,
                           bool can_restore,
                           RegisterResumePlan *plan)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    const int *origins = state.origins;
    for (int local = 0; local < analysis->nl; local++) {
        int col = analysis->save_columns[local];
        if (col < 0) {
            continue;
        }
        int top = state.save_tops[col];
        if (analysis->save_tops[(size_t)target * analysis->nsave_columns + col] != 0 ||
            top < 0)
        {
            return 0;
        }
        if (top == 0) {
            continue;
        }
        _PyLocals_Kind kind = _PyLocals_GetKind(code->co_localspluskinds, local);
        if (!can_restore || !(kind & CO_FAST_LOCAL) ||
            (kind & (CO_FAST_CELL | CO_FAST_FREE | CO_FAST_CONST)) ||
            !register_origin_chain(analysis->save_parents, analysis->n, top, -1,
                                     analysis->n))
        {
            return 0;
        }
        int selected = -1;
        for (int node = top; node > 0; node = analysis->save_parents[node - 1]) {
            _PyIRWord word = code->co_words[node - 1];
            if (_PyIR_BaseOpcode(IR_HANDLER(word)) != LOAD_FAST_AND_CLEAR ||
                (int)IR_B(word) != local)
            {
                return 0;
            }
            int acyclic = register_save_acyclic(analysis, node - 1);
            if (acyclic <= 0) {
                return acyclic;
            }
            int home = -1;
            for (int j = 0; j < analysis->nt; j++) {
                if (origins[j] == -node) {
                    home = analysis->nl + j;
                    break;
                }
            }
            if (home < 0) {
                /* Losing any binding-state token leaves an obligation; a
                 * coincidentally NULL or equal home cannot replace it. */
                return 0;
            }
            if (analysis->save_parents[node - 1] == 0) {
                selected = home;
            }
        }
        if (selected < analysis->nl) {
            return 0;
        }
        if (PyValue_IsNull(frame->base[selected])) {
            int safe = register_unbound_target_safe(code, analysis, target, local);
            if (safe <= 0) {
                return safe;
            }
        }
        int relative = selected - analysis->nl;
        if (analysis->required[(size_t)target * analysis->stride + relative / 64] &
            ((uint64_t)1 << (relative % 64)))
        {
            return 0;
        }
        const uint32_t *constants = _PyCode_CONSTSLOTS(code);
        for (int i = 0; i < code->co_nconstslots; i++) {
            if (constants[i * 2] == (uint32_t)selected) {
                return 0;
            }
        }
        for (int i = 0; i < plan->nrestores; i++) {
            if (plan->restores[i].home == selected) {
                return 0;
            }
        }
        plan->restores[plan->nrestores++] = (RegisterLocalRestore){local, selected};
    }
    return 1;
}

static int
register_resume_plan(_PyInterpreterFrame *frame, int current, int target,
                     RegisterResumeAnalysis *analysis, bool allow_anchor,
                     RegisterStoppedState phase, RegisterResumePlan *plan)
{
    bool committed = phase.kind == REGISTER_RESUME_COMMITTED_NEUTRAL ||
                     phase.kind == REGISTER_RESUME_COMMITTED;
    memset(plan->preserve, 0, plan->stride * sizeof(uint64_t));
    plan->nrestores = 0;
    int context = phase.context;
    if (phase.kind == REGISTER_RESUME_COMMITTED &&
        (context != 0 || !analysis->safe[target]))
    {
        /* A pre-site survivor slice cannot prove an already consumed input. */
        return 0;
    }
    bool can_cancel = phase.pc == current &&
        (phase.kind == REGISTER_RESUME_PRE_WORD ||
         (committed && context == 0 && analysis->safe[target]));
    /* Other targets retain the existing strict recipes; the new committed
     * permission does not reconstruct their required protocol roots. */
    if (context < 0 || context != analysis->context[target] ||
        !register_context_chain(analysis, context, -1))
    {
        return 0;
    }
    if (phase.kind == REGISTER_RESUME_PRE_WORD && phase.pc == current &&
        context == 0 && analysis->null_states != NULL)
    {
        bool empty_saves = true, has_null = false, compatible = true;
        for (int col = 0; col < analysis->nsave_columns; col++) {
            empty_saves &= phase.save_tops[col] == 0 &&
                analysis->save_tops[(size_t)target * analysis->nsave_columns + col] == 0;
        }
        for (size_t j = 0; j < analysis->stride; j++) {
            uint64_t value = analysis->value_required[(size_t)target * analysis->stride + j];
            uint64_t needs = analysis->null_required[(size_t)target * analysis->stride + j];
            uint64_t all = analysis->required[(size_t)target * analysis->stride + j];
            uint64_t available = analysis->null_states[(size_t)current * analysis->stride + j];
            has_null |= needs != 0;
            compatible &= value == 0 && (all & ~needs) == 0 &&
                          (needs & ~available) == 0;
        }
        for (int j = 0; j < analysis->nt && compatible; j++) {
            if ((analysis->null_required[(size_t)target * analysis->stride + j / 64] &
                 ((uint64_t)1 << (j % 64))) &&
                !PyValue_IsNull(frame->base[analysis->nl + j]))
            {
                compatible = false;
            }
        }
        if (empty_saves && compatible && has_null) {
            /* A proved NULL input requires no owner to preserve or invent. */
            return 1;
        }
    }
    int restored = register_saved_locals_plan(frame, target, analysis, phase,
                                             can_cancel && allow_anchor, plan);
    if (restored <= 0) {
        return restored;
    }
    if (context == 0) {
        if (can_cancel && analysis->safe[target]) {
            /* Inputs needed only by the skipped operation can be retired.
             * Every target input and protocol obligation was proved above. */
            return 1;
        }
        int pair;
        /* The existing FOR dynamic-phase proof has ordinary edges only.
         * Keep that adapter bounded until its own exceptional survival proof
         * is extended; empty inherited recipes already include EH liveness. */
        bool ordinary_anchor = allow_anchor && _PyIRSemantic_Stored(
            _PyFrame_GetCode(frame)->co_exceptiontable).exceptions_size == 0;
        int result = register_resume_recipe(frame, current, target, analysis->safe,
                                             analysis->required, ordinary_anchor, &pair);
        if (result > 0 && pair >= 0) {
            for (int h = pair; h < pair + 2; h++) {
                int j = h - analysis->nl;
                plan->preserve[j / 64] |= (uint64_t)1 << (j % 64);
            }
        }
        return result;
    }
    bool forward = target > current;
    if (!allow_anchor || target == current) {
        return 0;
    }
    if (forward) {
        if (phase.kind != REGISTER_RESUME_PRE_WORD || phase.pc != current) {
            return 0;
        }
        for (int col = 0; col < analysis->nsave_columns; col++) {
            if (phase.save_tops[col] != 0 ||
                analysis->save_tops[(size_t)target * analysis->nsave_columns + col] != 0)
            {
                return 0;
            }
        }
    }
    const int *before = analysis->origins + (size_t)current * analysis->nt;
    const int *after = analysis->origins + (size_t)target * analysis->nt;
    for (int node = context; node > 0; node = analysis->parents[node - 1]) {
        bool found = false;
        for (int j = 0; j < analysis->nt; j++) {
            if (before[j] != node || after[j] != node) {
                continue;
            }
            PyValueBorrow value = PyValue_Borrow(frame->base[analysis->nl + j]);
            if (!PyValue_IsNone(value) &&
                !(PyValue_IsObject(value) &&
                  PyExceptionInstance_Check(PyValue_AsKnownObjectBorrow(value))))
            {
                return 0;
            }
            plan->preserve[j / 64] |= (uint64_t)1 << (j % 64);
            found = true;
        }
        if (!found) {
            return 0;
        }
    }
    for (size_t j = 0; j < analysis->stride; j++) {
        uint64_t needs = analysis->required[(size_t)target * analysis->stride + j];
        if (!can_cancel) {
            needs |= analysis->required[(size_t)current * analysis->stride + j];
        }
        if (needs & ~plan->preserve[j]) {
            return 0;
        }
    }
    return register_caught_slice(_PyFrame_GetCode(frame), analysis, plan,
                                  current, target, context, forward);
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

/* The redirect already committed its new frame state. These detached owners
 * must survive nested callbacks without reading that frame's old bindings. */
static Py_NO_INLINE void
retire_redirect_values_slow(_PyInterpreterFrame *frame, PyValue *values,
                           int begin, int count)
{
    _PyTransitionPrefix prefix = {
        .base = values,
        .capacity = count,
        .begin = begin,
        .end = count,
    };
    _PyTransitionRoots roots = {
        .frame = frame,
        .prefix = &prefix,
    };
    PyThreadState *tstate = _PyThreadState_GET();
    _PyThreadState_PushTransitionRoots(tstate, &roots);
    while (prefix.begin < prefix.end) {
        int slot = prefix.begin++;
        PyValue value = values[slot];
        values[slot] = PyValue_NULL;
        PyValue_XCLOSE(value);
    }
    _PyThreadState_PopTransitionRoots(tstate, &roots);
}

/* A debugger transfer is a cold transaction over an observed register phase.
 * Prove the target's incoming owners and semantic protocol roots before
 * changing any home: a line number cannot create an iterator, handled
 * exception or saved outer-local binding. A cancellable pre-operation phase
 * may retire expression inputs that the selected target does not need.
 * Detach surplus owners, commit the continuation, then run closing callbacks.
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
    /* A constructor binder has taken the evaluated scratch inputs. Its
     * caller's named bindings remain mutable, but no redirect recipe may
     * reconstruct those consumed inputs before publication or failure unwind. */
    for (_PyTransitionRoots *roots =
             ((_PyThreadStateImpl *)PyThreadState_GET())->transition_roots;
         roots != NULL; roots = roots->previous)
    {
        if (roots->caller_staged && roots->frame != NULL &&
            roots->frame->previous == self->f_frame)
        {
            PyErr_SetString(PyExc_ValueError,
                            "jump requires a staged constructor owner recipe");
            return -1;
        }
    }
    /* Event eligibility permits a request; the phase and owner proofs below
     * still decide whether this particular transfer can be performed. */
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
        case _PY_DEBUGGER_ENTRY_EVENT:
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
    _PyIRObservationMap source = _PyIRObservation_Stored(code);
    for (uint32_t j = 0; j < source.nrecords; j++) {
        _PyIRObservationEvent event;
        _PyIRObservation_Event(&source, j, &event);
        if (event.line >= 0 && event.line >= requested_line &&
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
    _PyInterpreterFrame *frame_at_line = self->f_frame;
    int real_current = _PyInterpreterFrame_LASTI(frame_at_line);
    _PyThreadStateImpl *line_tstate = (_PyThreadStateImpl *)PyThreadState_GET();
    RegisterStoppedState identity_phase = register_stopped_state(
        self, real_current, what_event, false, line_tstate->observed_frame);
    if (!is_suspended && identity_phase.kind == REGISTER_RESUME_PRE_WORD &&
        PyCode_Addr2Line(code, real_current) == new_lineno)
    {
        PyMem_Free(lines);
        if (self->f_lineno_generation == UINT64_MAX) {
            PyErr_SetString(PyExc_OverflowError, "debugger redirect generation exhausted");
            return -1;
        }
        frame_at_line->phase_state = 0;
        frame_at_line->return_pc = frame_at_line->instr_ptr;
        self->f_line_next = -1;
        self->f_lineno = 0;
        self->f_lineno_generation++;
        _PyEval_InvalidateSourceErrors(PyThreadState_GET(), frame_at_line);
        return 0;
    }

    /* Protected and suspended resumptions need explicit owner/exception
     * recipes. Do not let a frontend stack projection authorize them. */
    if (is_suspended || what_event == PY_MONITORING_EVENT_PY_YIELD) {
        PyMem_Free(lines);
        PyErr_SetString(PyExc_ValueError,
                        "jump requires a suspended or protected owner recipe");
        return -1;
    }
    RegisterResumeAnalysis analysis = {0};
    RegisterResumePlan plan = {0};
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
        if (child->external || _PyIRSemantic_Stored(code->co_exceptiontable).exceptions_size ||
            self->f_frame->return_slot != IR_NO_DST ||
            (opcode != CALL && opcode != CALL_KW))
        {
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
        _PyIRObservationEvent event;
        _PyIRObservation_Event(&source, (uint32_t)self->f_source_event, &event);
        current = event.to;
    }
    RegisterStoppedState phase = register_stopped_state(self, current, what_event,
                                                       pending_call, observed);
    if (register_resume_analyze(self->f_frame, !pending_call,
                              phase.kind == REGISTER_RESUME_PRE_WORD, &analysis) < 0) {
        PyMem_Free(lines);
        return -1;
    }
    int bound = register_stopped_state_bind(self, &analysis, &phase);
    if (bound <= 0) {
        register_resume_analysis_clear(&analysis);
        PyMem_Free(lines);
        if (bound == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "jump requires an incoming temporary register owner");
        }
        return -1;
    }
    plan.stride = analysis.stride;
    plan.preserve = PyMem_Calloc(plan.stride, sizeof(uint64_t));
    plan.restores = PyMem_Calloc((size_t)Py_MAX(analysis.nsave_columns, 1),
                                 sizeof(*plan.restores));
    if (plan.preserve == NULL || plan.restores == NULL) {
        register_resume_plan_clear(&plan);
        register_resume_analysis_clear(&analysis);
        PyMem_Free(lines);
        PyErr_NoMemory();
        return -1;
    }
    /* A source callback observes a post-operation edge. Require its actual
     * context to agree with the selected landing's derived context; merging
     * at event.to alone cannot establish the stopped phase. */
    if (!pending_call && self->f_source_event >= 0) {
        _PyIRObservationEvent event;
        _PyIRObservation_Event(&source, (uint32_t)self->f_source_event, &event);
        int *origins = PyMem_New(int, Py_MAX(analysis.nt, 1));
        if (origins == NULL) {
            register_resume_analysis_clear(&analysis);
            register_resume_plan_clear(&plan); PyMem_Free(lines);
            PyErr_NoMemory();
            return -1;
        }
        memcpy(origins, analysis.origins + (size_t)event.from * analysis.nt,
               (size_t)analysis.nt * sizeof(int));
        int *saved = PyMem_New(int, Py_MAX(analysis.nsave_columns, 1));
        if (saved == NULL) {
            PyMem_Free(origins); register_resume_analysis_clear(&analysis);
            register_resume_plan_clear(&plan); PyMem_Free(lines);
            PyErr_NoMemory();
            return -1;
        }
        memcpy(saved, analysis.save_tops + (size_t)event.from * analysis.nsave_columns,
               (size_t)analysis.nsave_columns * sizeof(int));
        int context = register_context_normal(code, event.from, event.edge,
                                              &analysis, analysis.context[event.from],
                                              saved, origins);
        PyMem_Free(origins);
        bool same_saves = memcmp(saved,
            analysis.save_tops + (size_t)current * analysis.nsave_columns,
            (size_t)analysis.nsave_columns * sizeof(int)) == 0;
        PyMem_Free(saved);
        if (context < 0 || context != analysis.context[current] || !same_saves) {
            current = -1;
        }
    }
    int best_addr = -1;
    RegisterStoppedState selected_phase = phase;
    RegisterStoppedState source_phase = {.kind = REGISTER_RESUME_OTHER, .pc = current};
    /* Source-only target selection retains its prior conservative site row,
     * never the transferred POST row or its cancellation authority. */
    if (current >= 0) {
        (void)register_stopped_state_bind(self, &analysis, &source_phase);
    }
    int recipe = 0;
    if (current >= 0 && current < len) {
        for (int i = 0; i < len; i++) {
            if (lines[i] != new_lineno) {
                continue;
            }
            recipe = register_resume_plan(self->f_frame, current, i, &analysis,
                                          !pending_call, phase, &plan);
            if (recipe != 0) {
                if (recipe > 0) {
                    best_addr = i;
                }
                break;
            }
        }
        for (uint32_t j = 0;
             phase.kind != REGISTER_RESUME_COMMITTED && best_addr < 0 &&
             recipe >= 0 && j < source.nrecords; j++) {
            _PyIRObservationEvent event;
            _PyIRObservation_Event(&source, j, &event);
            if (event.line == new_lineno && event.handler < 0) {
                recipe = register_resume_plan(self->f_frame, current, event.to,
                                              &analysis, !pending_call,
                                              source_phase,
                                              &plan);
                if (recipe > 0) {
                    best_addr = event.to;
                    selected_phase = source_phase;
                    break;
                }
            }
        }
    }
    register_resume_analysis_clear(&analysis);
    PyMem_Free(lines);
    if (best_addr < 0) {
        register_resume_plan_clear(&plan);
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
        unbound += PyValue_IsNull(self->f_frame->base[i]) &&
                   !register_plan_restores_local(&plan, i);
    }
    if (self->f_lineno_generation == UINT64_MAX) {
        register_resume_plan_clear(&plan);
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
            register_resume_plan_clear(&plan);
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
            register_resume_plan_clear(&plan);
            return 0;
        }
        if (register_resume_analyze(frame, !pending_call,
                                    selected_phase.kind == REGISTER_RESUME_PRE_WORD,
                                    &analysis) < 0) {
            register_resume_plan_clear(&plan);
            return -1;
        }
        bool phase_valid = true;
        if (selected_phase.kind != REGISTER_RESUME_OTHER) {
            _PyThreadStateImpl *impl = (_PyThreadStateImpl *)PyThreadState_GET();
            RegisterStoppedState fresh = register_stopped_state(
                self, current, PyThreadState_GET()->what_event,
                pending_call, impl->observed_frame);
            phase_valid = fresh.kind == selected_phase.kind && fresh.pc == current;
            selected_phase = fresh;
        }
        /* Every view borrows this analysis or its request-owned copied row;
         * the previous analysis was released before preparation callbacks. */
        if (phase_valid) {
            int bound = register_stopped_state_bind(self, &analysis, &selected_phase);
            if (bound < 0) {
                register_resume_analysis_clear(&analysis);
                register_resume_plan_clear(&plan);
                return -1;
            }
            phase_valid = bound > 0;
        }
        /* A source-only target deliberately selected OTHER must not gain
         * cancellation authority merely because preparation called Python. */
        int valid = phase_valid
            ? register_resume_plan(frame, current, best_addr, &analysis,
                                   !pending_call, selected_phase, &plan)
            : 0;
        register_resume_analysis_clear(&analysis);
        if (valid < 0) {
            register_resume_plan_clear(&plan);
            return -1;
        }
        if (!valid) {
            register_resume_plan_clear(&plan);
            PyErr_SetString(PyExc_ValueError,
                            "jump owner state changed during warning");
            return -1;
        }
    }
    int nslots = code->co_framesize - FRAME_SPECIALS_SIZE;
    PyValue *retired = NULL;
    PyValue *restored_values = NULL;
    if (!pending_call) {
        size_t count = (size_t)(nslots - code->co_nlocalsplus) + plan.nrestores;
        if (count > SIZE_MAX / sizeof(PyValue)) {
            register_resume_plan_clear(&plan);
            PyErr_NoMemory();
            return -1;
        }
        retired = PyMem_New(PyValue, Py_MAX(count, 1));
        restored_values = PyMem_Calloc((size_t)Py_MAX(plan.nrestores, 1), sizeof(PyValue));
        if (retired == NULL || restored_values == NULL) {
            PyMem_Free(retired); PyMem_Free(restored_values);
            register_resume_plan_clear(&plan);
            PyErr_NoMemory();
            return -1;
        }
    }
    /* No callback is possible between the final proof and the commit. */
    frame->phase_state = 0;
    int nretired = 0;
    for (int i = 0; i < plan.nrestores; i++) {
        RegisterLocalRestore action = plan.restores[i];
        restored_values[i] = frame->base[action.home];
        frame->base[action.home] = PyValue_NULL;
        retired[nretired++] = frame->base[action.local];
        frame->base[action.local] = PyValue_NULL;
    }
    if (!pending_call) {
        const uint32_t *constants = _PyCode_CONSTSLOTS(code);
        for (int slot = code->co_nlocalsplus; slot < nslots; slot++) {
            bool constant = false;
            for (int j = 0; j < code->co_nconstslots; j++) {
                constant |= constants[j * 2] == (uint32_t)slot;
            }
            if (!constant && !register_plan_has(&plan, slot - code->co_nlocalsplus)) {
                retired[nretired++] = frame->base[slot];
                frame->base[slot] = PyValue_NULL;
            }
        }
    }
    for (int i = 0; i < code->co_nlocalsplus; i++) {
        if (PyValue_IsNull(frame->base[i]) && !register_plan_restores_local(&plan, i)) {
            frame->base[i] = PyValue_None;
        }
    }
    for (int i = 0; i < plan.nrestores; i++) {
        frame->base[plan.restores[i].local] = restored_values[i];
        restored_values[i] = PyValue_NULL;
    }
    PyMem_Free(restored_values);
    register_resume_plan_clear(&plan);
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
    _PyEval_InvalidateSourceErrors(PyThreadState_GET(), self->f_frame);
    /* All surplus owners are detached before the first callback. A nested
     * redirect can now install new owners without later closes clearing them. */
    for (int i = 0; i < nretired; i++) {
        PyValue value = retired[i];
#ifdef Py_GIL_DISABLED
        if (PyValue_RefcountOnObject(value)) {
#else
        if (_PyValue_MayDealloc(value)) {
#endif
            retire_redirect_values_slow(self->f_frame, retired, i, nretired);
            break;
        }
        retired[i] = PyValue_NULL;
        PyValue_XCLOSE(value);
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

static Py_NO_INLINE void
frame_dealloc_owned(PyFrameObject *f, _PyInterpreterFrame *frame,
                    PyObject **observers, size_t nobservers)
{
    assert(frame->frame_obj == NULL);
    PyThreadState *tstate = _PyThreadState_GET();
    int nslots = _PyFrame_NumSlots(frame);
    _PyTransitionRoots roots;
    _PyFrame_InitClearRoots(frame, nslots, nslots, PyValue_NULL, &roots);
    roots.retire_reason = _PyIR_RETIRE_HEAP_CLEAR;
    _PyThreadState_PushTransitionRoots(tstate, &roots);

    FT_CLEAR_WEAKREFS((PyObject *)f, f->f_weakreflist);
    roots.header_mask &= ~_Py_TRANSITION_FUNCTION;
    Py_CLEAR(frame->f_funcobj);
    roots.header_mask &= ~_Py_TRANSITION_LOCALS;
    Py_CLEAR(frame->f_locals);
    _PyFrame_FinishClearExceptCode(frame, nslots, &roots);
    for (size_t i = 0; i < nobservers; i++) {
        Py_CLEAR(observers[i]);
    }
    /* Code also owns constants with finalizers. Keep its pin through every
     * home/metadata close, then remove authority before its final close. */
    roots.header_mask &= ~_Py_TRANSITION_CODE;
    Py_CLEAR(frame->f_executable);
    _PyThreadState_PopTransitionRoots(tstate, &roots);
}

static void
frame_dealloc(PyObject *op)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the generator pointer */
    PyFrameObject *f = PyFrameObject_CAST(op);
    if (_PyObject_GC_IS_TRACKED(f)) {
        _PyObject_GC_UNTRACK(f);
    }

    /* This refcount-zero object has no Python observer that can mutate its
     * bindings. Move metadata owners to actual native workspace before any
     * callback; the untracked object's fields are not conservative roots. */
    PyObject *observers[] = {
        (PyObject *)f->f_back, f->f_trace, f->f_extra_locals,
        f->f_locals_cache, f->f_overwritten_fast_locals,
    };
    f->f_back = NULL;
    f->f_trace = NULL;
    f->f_extra_locals = NULL;
    f->f_locals_cache = NULL;
    f->f_overwritten_fast_locals = NULL;

    /* GH-106092: If f->f_frame was on the stack and we reached the maximum
     * nesting depth for deallocations, the trashcan may have delayed this
     * deallocation until after f->f_frame is freed. Avoid dereferencing
     * f->f_frame unless we know it still points to valid memory. */
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)f->_f_frame_data;

    /* Kill all local variables including specials, if we own them */
    if (f->f_frame == frame && frame->owner == FRAME_OWNED_BY_FRAME_OBJECT) {
        frame_dealloc_owned(f, frame, observers, Py_ARRAY_LENGTH(observers));
    }
    else {
        FT_CLEAR_WEAKREFS(op, f->f_weakreflist);
        for (size_t i = 0; i < Py_ARRAY_LENGTH(observers); i++) {
            Py_CLEAR(observers[i]);
        }
    }
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

    _PyFrame_ClearLocals(f->f_frame);
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
    f->f_capture_authority = _PyIR_HOME_NONE;
    f->f_capture = (_PyIRHomeView){0};
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
    _PyFrame_GetNullControlHomeView(f->f_frame,
                                   _PyFrame_NumSlots(f->f_frame), &f->f_capture);
    f->f_capture_authority = _PyIR_HOME_NULL_CONTROL_READABLE;
    f->f_frame->phase_state = _PyIR_PHASE_CAPTURED;
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
          && frame->f_funcobj != NULL))
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
