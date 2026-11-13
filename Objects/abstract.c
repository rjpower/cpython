/* Abstract Object Interface (many thanks to Jim Fulton) */

/* This file holds the slot dispatchers, so it is an adapter unit:
   Tools/build/check_value_api.py does not list its slot-field reads. */
#define Py_VALUE_ADAPTER 1

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_crossinterp.h"   // _Py_CallInInterpreter()
#include "pycore_dict.h"          // PyDict_GetValue()
#include "pycore_genobject.h"     // _PyGen_FetchStopIterationValue()
#include "pycore_list.h"          // _PyList_AppendTakeRef()
#include "pycore_long.h"          // _PyLong_IsNegative()
#include "pycore_object.h"        // _Py_CheckSlotResult()
#include "pycore_value.h"         // PyValue
#include "pycore_pybuffer.h"      // _PyBuffer_ReleaseInInterpreterAndRawFree()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_FromArraySteal()
#include "pycore_unionobject.h"   // _PyUnion_Check()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_long_value.h"    // _PyValue_IntAsDouble()
#include "pycore_value_ops.h"     // _PyValue_Add()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

#include "opcode.h"               // NB_ADD

#include <stddef.h>               // offsetof()


/* Shorthands to return certain errors */

static PyObject *
type_error(const char *msg, PyObject *obj)
{
    PyErr_Format(PyExc_TypeError, msg, Py_TYPE(obj)->tp_name);
    return NULL;
}

static PyObject *
null_error(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                         "null argument to internal routine");
    }
    return NULL;
}

/* The value dispatchers of the subscript, item, contains, iternext and send
   families (pyjit/design/proposals/value-api-m1b-m2.md, sections 1.2 and
   1.3). Their receivers are objects: no int or float defines these slots,
   so a dispatcher whose receiver is a value raises upstream's TypeError
   for an inline one before it reads a slot (R1). A type's value slot is
   called if it is set, else its object slot through the adapter below,
   which boxes only an inline key, item or value (SLOT_ADAPTER, under the
   class SEQUENCE or ITERATION). */

#define seq_adapter_box(v, SLOT) \
    _PyValue_AdapterBox((v), BOX_SITE_SEQUENCE, (SLOT))
#define iter_adapter_box(v, SLOT) \
    _PyValue_AdapterBox((v), BOX_SITE_ITERATION, (SLOT))
#define seq_adapter_release(v, obj) _PyValue_AdapterRelease((v), (obj))

static void value_type_error(const char *msg, PyValueBorrow v);

static PyValue
call_subscript(PyObject *o, binaryfunc slot, PyValueBorrow key)
{
    PyTypeObject *t = Py_TYPE(o);
    _Py_subscriptvaluefunc f = _PyType_VALUE_SLOT(t, subscript);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_subscript);
        return f(o, key);
    }
    PyObject *k = seq_adapter_box(key, slot);
    if (k == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = slot(o, k);
    seq_adapter_release(key, k);
    return _PyValue_FromObjectResult(res);
}

static PyValue
call_item(PyObject *o, ssizeargfunc slot, Py_ssize_t i)
{
    PyTypeObject *t = Py_TYPE(o);
    _Py_itemvaluefunc f = _PyType_VALUE_SLOT(t, item);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_item);
        return f(o, i);
    }
    return _PyValue_FromObjectResult(slot(o, i));
}

// value is PyValue_NULL to delete.
static int
call_ass_subscript(PyObject *o, objobjargproc slot, PyValueBorrow key,
                   PyValueBorrow value)
{
    PyTypeObject *t = Py_TYPE(o);
    _Py_asssubscriptvaluefunc f = _PyType_VALUE_SLOT(t, ass_subscript);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_ass_subscript);
        return f(o, key, value);
    }
    PyObject *k = seq_adapter_box(key, slot);
    if (k == NULL) {
        return -1;
    }
    PyObject *v = NULL;
    if (!PyValue_IsNull(value)) {
        v = seq_adapter_box(value, slot);
        if (v == NULL) {
            seq_adapter_release(key, k);
            return -1;
        }
    }
    int res = slot(o, k, v);
    if (v != NULL) {
        seq_adapter_release(value, v);
    }
    seq_adapter_release(key, k);
    return res;
}

// value is PyValue_NULL to delete.
static int
call_ass_item(PyObject *o, ssizeobjargproc slot, Py_ssize_t i,
              PyValueBorrow value)
{
    PyTypeObject *t = Py_TYPE(o);
    _Py_assitemvaluefunc f = _PyType_VALUE_SLOT(t, ass_item);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_ass_item);
        return f(o, i, value);
    }
    if (PyValue_IsNull(value)) {
        return slot(o, i, NULL);
    }
    PyObject *v = seq_adapter_box(value, slot);
    if (v == NULL) {
        return -1;
    }
    int res = slot(o, i, v);
    seq_adapter_release(value, v);
    return res;
}

static int
call_contains(PyObject *o, objobjproc slot, PyValueBorrow item)
{
    PyTypeObject *t = Py_TYPE(o);
    _Py_containsvaluefunc f = _PyType_VALUE_SLOT(t, contains);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_contains);
        return f(o, item);
    }
    PyObject *x = seq_adapter_box(item, slot);
    if (x == NULL) {
        return -1;
    }
    int res = slot(o, x);
    seq_adapter_release(item, x);
    return res;
}

/* The ITERNEXT adapter: an object tp_iternext's item is NEXT; NULL with
   nothing pending is RETURN with no value; NULL with any exception, a
   StopIteration included, is ERROR with the exception untouched (R11). */
static _PyStatusValue
adapt_iternext(iternextfunc slot, PyObject *it)
{
    PyObject *res = slot(it);
    if (res != NULL) {
        return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(res)};
    }
    if (!PyErr_Occurred()) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
}

static _PyStatusValue
call_send(PyObject *it, sendfunc slot, PyValueBorrow arg)
{
    PyTypeObject *t = Py_TYPE(it);
    _Py_sendvaluefunc f = _PyType_VALUE_SLOT(t, send);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_send);
        return f(it, arg);
    }
    PyObject *a = iter_adapter_box(arg, slot);
    if (a == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    PyObject *res = NULL;
    PySendResult status = slot(it, a, &res);
    seq_adapter_release(arg, a);
    if (status == PYGEN_ERROR) {
        assert(res == NULL);
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    return (_PyStatusValue){status, PyValue_FromObjectSteal(res)};
}

/* Operations on any object */

PyObject *
PyObject_Type(PyObject *o)
{
    PyObject *v;

    if (o == NULL) {
        return null_error();
    }

    v = (PyObject *)Py_TYPE(o);
    return Py_NewRef(v);
}

Py_ssize_t
(_PyValue_Size)(PyValueBorrow v)
{
    if (PyValue_IsNull(v)) {
        null_error();
        return -1;
    }
    if (PyValue_IsInline(v)) {
        // As PyMapping_Size() reports for int and float.
        value_type_error("object of type '%.200s' has no len()", v);
        return -1;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);

    PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_length) {
        Py_ssize_t len = m->sq_length(o);
        assert(_Py_CheckSlotResult(o, "__len__", len >= 0));
        return len;
    }

    return _PyValue_MappingSize(v);
}

Py_ssize_t
PyObject_Size(PyObject *o)
{
    return _PyValue_Size(_Py_VALUE_ARG(o));
}

#undef PyObject_Length
Py_ssize_t
PyObject_Length(PyObject *o)
{
    return _PyValue_Size(_Py_VALUE_ARG(o));
}
#define PyObject_Length PyObject_Size

int
(_PyValue_HasLen)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no sq_length or mp_length.
        return 0;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    return (Py_TYPE(o)->tp_as_sequence && Py_TYPE(o)->tp_as_sequence->sq_length) ||
        (Py_TYPE(o)->tp_as_mapping && Py_TYPE(o)->tp_as_mapping->mp_length);
}

int
_PyObject_HasLen(PyObject *o) {
    return _PyValue_HasLen(_Py_VALUE_ARG(o));
}

/* The length hint function returns a non-negative value from o.__len__()
   or o.__length_hint__(). If those methods aren't found the defaultvalue is
   returned.  If one of the calls fails with an exception other than TypeError
   this function returns -1.
*/

Py_ssize_t
(_PyValue_LengthHint)(PyValueBorrow v, Py_ssize_t defaultvalue)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no __len__() or __length_hint__().
        return defaultvalue;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PyObject *hint, *result;
    Py_ssize_t res;
    if (_PyValue_HasLen(v)) {
        res = _PyValue_Size(v);
        if (res < 0) {
            PyThreadState *tstate = _PyThreadState_GET();
            assert(_PyErr_Occurred(tstate));
            if (!_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
                return -1;
            }
            _PyErr_Clear(tstate);
        }
        else {
            return res;
        }
    }
    hint = _PyObject_LookupSpecial(o, &_Py_ID(__length_hint__));
    if (hint == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return defaultvalue;
    }
    result = _PyObject_CallNoArgs(hint);
    Py_DECREF(hint);
    if (result == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_Clear(tstate);
            return defaultvalue;
        }
        return -1;
    }
    else if (result == Py_NotImplemented) {
        Py_DECREF(result);
        return defaultvalue;
    }
    if (!PyLong_Check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "%T.__length_hint__() must return an int, not %T",
                     o, result);
        Py_DECREF(result);
        return -1;
    }
    res = _PyValue_IntAsSsize_t(PyValue_FromObjectBorrow(result));
    Py_DECREF(result);
    if (res < 0 && PyErr_Occurred()) {
        return -1;
    }
    if (res < 0) {
        PyErr_Format(PyExc_ValueError,
                     "%T.__length_hint__() must return a non-negative int", o);
        return -1;
    }
    return res;
}

Py_ssize_t
PyObject_LengthHint(PyObject *o, Py_ssize_t defaultvalue)
{
    return _PyValue_LengthHint(_Py_VALUE_ARG(o), defaultvalue);
}

// type[key] and Class[key]: the type's __class_getitem__, whose argument
// is an object.
static PyValue
value_class_getitem(PyObject *o, PyValueBorrow key)
{
    PyObject *k = _PyValue_AsObjectAt(key, BOX_SITE_SEQUENCE);
    if (k == NULL) {
        return PyValue_NULL;
    }
    PyObject *result;
    // Special case type[int], but disallow other types so str[int] fails
    if ((PyTypeObject*)o == &PyType_Type) {
        result = Py_GenericAlias(o, k);
        Py_DECREF(k);
        return _PyValue_FromObjectResult(result);
    }

    PyObject *meth;
    if (PyObject_GetOptionalAttr(o, &_Py_ID(__class_getitem__), &meth) < 0) {
        Py_DECREF(k);
        return PyValue_NULL;
    }
    if (meth && meth != Py_None) {
        result = PyObject_CallOneArg(meth, k);
        Py_DECREF(meth);
        Py_DECREF(k);
        return _PyValue_FromObjectResult(result);
    }
    Py_XDECREF(meth);
    Py_DECREF(k);
    PyErr_Format(PyExc_TypeError, "type '%.200s' is not subscriptable",
                 ((PyTypeObject *)o)->tp_name);
    return PyValue_NULL;
}

PyValue
(_PyValue_GetItem)(PyValueBorrow v, PyValueBorrow key)
{
    if (PyValue_IsNull(v) || PyValue_IsNull(key)) {
        null_error();
        return PyValue_NULL;
    }
    if (PyValue_IsInline(v)) {
        value_type_error("'%.200s' object is not subscriptable", v);
        return PyValue_NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);

    PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_subscript) {
        PyValue item = call_subscript(o, m->mp_subscript, key);
        assert(_Py_CheckSlotResult(o, "__getitem__", !PyValue_IsNull(item)));
        return item;
    }

    PySequenceMethods *ms = Py_TYPE(o)->tp_as_sequence;
    if (ms && ms->sq_item) {
        if (_PyValue_IndexCheck(key)) {
            Py_ssize_t key_value;
            key_value = _PyValue_AsSsize_t(key, PyExc_IndexError);
            if (key_value == -1 && PyErr_Occurred())
                return PyValue_NULL;
            return _PyValue_SequenceGetItem(v, key_value);
        }
        else {
            value_type_error("sequence index must "
                             "be integer, not '%.200s'", key);
            return PyValue_NULL;
        }
    }

    if (PyType_Check(o)) {
        return value_class_getitem(o, key);
    }

    value_type_error("'%.200s' object is not subscriptable", v);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PyObject_GetItem(PyObject *o, PyObject *key)
{
    return _PyValue_THUNK_EXIT(
        _PyValue_GetItem(_Py_VALUE_ARG(o), _Py_VALUE_ARG(key)), BOX_SITE_SEQUENCE);
}

_PyStatusValue
(_PyValue_GetOptionalItem)(PyValueBorrow v, PyValueBorrow key)
{
    if (PyValue_IsObject(v)
        && PyAnyDict_CheckExact(PyValue_AsKnownObjectBorrow(v)))
    {
        // The entry's word, unboxed.
        PyValue item;
        int found = PyDict_GetValue(PyValue_AsKnownObjectBorrow(v), key,
                                    &item);
        return (_PyStatusValue){found, found > 0 ? item : PyValue_NULL};
    }

    PyValue item = _PyValue_GetItem(v, key);
    if (!PyValue_IsNull(item)) {
        return (_PyStatusValue){1, item};
    }
    assert(PyErr_Occurred());
    if (!PyErr_ExceptionMatches(PyExc_KeyError)) {
        return (_PyStatusValue){-1, PyValue_NULL};
    }
    PyErr_Clear();
    return (_PyStatusValue){0, PyValue_NULL};
}

Py_VALUE_ENTRY int
PyMapping_GetOptionalItem(PyObject *obj, PyObject *key, PyObject **result)
{
    return _PyValue_THUNK_EXIT_STATUS(
        _PyValue_GetOptionalItem(_Py_VALUE_ARG(obj), _Py_VALUE_ARG(key)),
        result, BOX_SITE_SEQUENCE);
}

Py_VALUE_ENTRY PyObject*
_PyMapping_GetOptionalItem2(PyObject *obj, PyObject *key, int *err)
{
    PyObject* result;
    *err = _PyValue_THUNK_EXIT_STATUS(
        _PyValue_GetOptionalItem(_Py_VALUE_ARG(obj), _Py_VALUE_ARG(key)),
        &result, BOX_SITE_SEQUENCE);
    return result;
}

// o[key] = value, or del o[key] for a PyValue_NULL value.
static int
value_ass_item(PyValueBorrow v, PyValueBorrow key, PyValueBorrow value)
{
    int deleting = PyValue_IsNull(value);
    if (PyValue_IsNull(v) || PyValue_IsNull(key)) {
        null_error();
        return -1;
    }
    if (!PyValue_IsInline(v)) {
        PyObject *o = PyValue_AsKnownObjectBorrow(v);
        PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
        /* The exact dict and list stores and deletions that take the key
           and value as they are: no box of an inline key or value. */
        if (PyDict_CheckExact(o)) {
            return deleting ? PyDict_DelValue(o, key)
                            : PyDict_SetValue(o, key, value);
        }
        if (!deleting && PyList_CheckExact(o) && PyValue_IsInlineInt(key)) {
            Py_ssize_t index = PyValue_InlineInt64(key);
            int err;
            Py_BEGIN_CRITICAL_SECTION(o);
            if (index < 0) {
                index += PyList_GET_SIZE(o);
            }
            err = _PyList_SetItemTakeValue((PyListObject *)o, index,
                                           PyValue_DUP(value));
            Py_END_CRITICAL_SECTION();
            return err;
        }
        if (m && m->mp_ass_subscript) {
            int res = call_ass_subscript(o, m->mp_ass_subscript, key, value);
            assert(_Py_CheckSlotResult(o, deleting ? "__delitem__"
                                                   : "__setitem__",
                                       res >= 0));
            return res;
        }

        if (Py_TYPE(o)->tp_as_sequence) {
            if (_PyValue_IndexCheck(key)) {
                Py_ssize_t key_value;
                key_value = _PyValue_AsSsize_t(key, PyExc_IndexError);
                if (key_value == -1 && PyErr_Occurred())
                    return -1;
                if (deleting) {
                    return _PyValue_SequenceDelItem(v, key_value);
                }
                return _PyValue_SequenceSetItem(v, key_value, value);
            }
            else if (Py_TYPE(o)->tp_as_sequence->sq_ass_item) {
                value_type_error("sequence index must be "
                                 "integer, not '%.200s'", key);
                return -1;
            }
        }
    }

    value_type_error(deleting
                     ? "'%.200s' object does not support item deletion"
                     : "'%.200s' object does not support item assignment", v);
    return -1;
}

int
(_PyValue_SetItem)(PyValueBorrow v, PyValueBorrow key, PyValueBorrow value)
{
    if (PyValue_IsNull(value)) {
        null_error();
        return -1;
    }
    return value_ass_item(v, key, value);
}

int
(_PyValue_DelItem)(PyValueBorrow v, PyValueBorrow key)
{
    return value_ass_item(v, key, PyValue_Borrow(PyValue_NULL));
}

int
PyObject_SetItem(PyObject *o, PyObject *key, PyObject *value)
{
    return _PyValue_SetItem(_Py_VALUE_ARG(o), _Py_VALUE_ARG(key),
                            _Py_VALUE_ARG(value));
}

int
PyObject_DelItem(PyObject *o, PyObject *key)
{
    return _PyValue_DelItem(_Py_VALUE_ARG(o), _Py_VALUE_ARG(key));
}

int
(_PyValue_DelItemString)(PyValueBorrow o, const char *key)
{
    // _PyValue_DelItem() reports an int or float receiver.
    PyObject *okey = PyUnicode_FromString(key);
    if (okey == NULL) {
        return -1;
    }
    int ret = _PyValue_DelItem(o, _Py_VALUE_ARG(okey));
    Py_DECREF(okey);
    return ret;
}

int
PyObject_DelItemString(PyObject *o, const char *key)
{
    if (o == NULL || key == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_DelItemString(_Py_VALUE_ARG(o), key);
}


/* Return 1 if the getbuffer function is available, otherwise return 0. */
int
PyObject_CheckBuffer(PyObject *obj)
{
    PyBufferProcs *tp_as_buffer = Py_TYPE(obj)->tp_as_buffer;
    return (tp_as_buffer != NULL && tp_as_buffer->bf_getbuffer != NULL);
}

// Old buffer protocols (deprecated, abi only)

/* Checks whether an arbitrary object supports the (character, single segment)
   buffer interface.

   Returns 1 on success, 0 on failure.

   We release the buffer right after use of this function which could
   cause issues later on.  Don't use these functions in new code.
 */
PyAPI_FUNC(int) /* abi_only */
PyObject_CheckReadBuffer(PyObject *obj)
{
    PyBufferProcs *pb = Py_TYPE(obj)->tp_as_buffer;
    Py_buffer view;

    if (pb == NULL ||
        pb->bf_getbuffer == NULL)
        return 0;
    if ((*pb->bf_getbuffer)(obj, &view, PyBUF_SIMPLE) == -1) {
        PyErr_Clear();
        return 0;
    }
    PyBuffer_Release(&view);
    return 1;
}

static int
as_read_buffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)
{
    Py_buffer view;

    if (obj == NULL || buffer == NULL || buffer_len == NULL) {
        null_error();
        return -1;
    }
    if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) != 0)
        return -1;

    *buffer = view.buf;
    *buffer_len = view.len;
    PyBuffer_Release(&view);
    return 0;
}

/* Takes an arbitrary object which must support the (character, single segment)
   buffer interface and returns a pointer to a read-only memory location
   usable as character based input for subsequent processing.

   Return 0 on success.  buffer and buffer_len are only set in case no error
   occurs. Otherwise, -1 is returned and an exception set. */
PyAPI_FUNC(int) /* abi_only */
PyObject_AsCharBuffer(PyObject *obj,
                      const char **buffer,
                      Py_ssize_t *buffer_len)
{
    return as_read_buffer(obj, (const void **)buffer, buffer_len);
}

/* Same as PyObject_AsCharBuffer() except that this API expects (readable,
   single segment) buffer interface and returns a pointer to a read-only memory
   location which can contain arbitrary data.

   0 is returned on success.  buffer and buffer_len are only set in case no
   error occurs.  Otherwise, -1 is returned and an exception set. */
PyAPI_FUNC(int) /* abi_only */
PyObject_AsReadBuffer(PyObject *obj,
                      const void **buffer,
                      Py_ssize_t *buffer_len)
{
    return as_read_buffer(obj, buffer, buffer_len);
}

/* Takes an arbitrary object which must support the (writable, single segment)
   buffer interface and returns a pointer to a writable memory location in
   buffer of size 'buffer_len'.

   Return 0 on success.  buffer and buffer_len are only set in case no error
   occurs. Otherwise, -1 is returned and an exception set. */
PyAPI_FUNC(int) /* abi_only */
PyObject_AsWriteBuffer(PyObject *obj,
                       void **buffer,
                       Py_ssize_t *buffer_len)
{
    PyBufferProcs *pb;
    Py_buffer view;

    if (obj == NULL || buffer == NULL || buffer_len == NULL) {
        null_error();
        return -1;
    }
    pb = Py_TYPE(obj)->tp_as_buffer;
    if (pb == NULL ||
        pb->bf_getbuffer == NULL ||
        ((*pb->bf_getbuffer)(obj, &view, PyBUF_WRITABLE) != 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected a writable bytes-like object");
        return -1;
    }

    *buffer = view.buf;
    *buffer_len = view.len;
    PyBuffer_Release(&view);
    return 0;
}

/* Buffer C-API for Python 3.0 */

int
PyObject_GetBuffer(PyObject *obj, Py_buffer *view, int flags)
{
    if (flags != PyBUF_SIMPLE) {  /* fast path */
        if (flags == PyBUF_READ || flags == PyBUF_WRITE) {
            PyErr_BadInternalCall();
            return -1;
        }
    }
    PyBufferProcs *pb = Py_TYPE(obj)->tp_as_buffer;

    if (pb == NULL || pb->bf_getbuffer == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "a bytes-like object is required, not '%.100s'",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }
    int res = (*pb->bf_getbuffer)(obj, view, flags);
    assert(_Py_CheckSlotResult(obj, "getbuffer", res >= 0));
    return res;
}

static int
_IsFortranContiguous(const Py_buffer *view)
{
    Py_ssize_t sd, dim;
    int i;

    /* 1) len = product(shape) * itemsize
       2) itemsize > 0
       3) len = 0 <==> exists i: shape[i] = 0 */
    if (view->len == 0) return 1;
    if (view->strides == NULL) {  /* C-contiguous by definition */
        /* Trivially F-contiguous */
        if (view->ndim <= 1) return 1;

        /* ndim > 1 implies shape != NULL */
        assert(view->shape != NULL);

        /* Effectively 1-d */
        sd = 0;
        for (i=0; i<view->ndim; i++) {
            if (view->shape[i] > 1) sd += 1;
        }
        return sd <= 1;
    }

    /* strides != NULL implies both of these */
    assert(view->ndim > 0);
    assert(view->shape != NULL);

    sd = view->itemsize;
    for (i=0; i<view->ndim; i++) {
        dim = view->shape[i];
        if (dim > 1 && view->strides[i] != sd) {
            return 0;
        }
        sd *= dim;
    }
    return 1;
}

static int
_IsCContiguous(const Py_buffer *view)
{
    Py_ssize_t sd, dim;
    int i;

    /* 1) len = product(shape) * itemsize
       2) itemsize > 0
       3) len = 0 <==> exists i: shape[i] = 0 */
    if (view->len == 0) return 1;
    if (view->strides == NULL) return 1; /* C-contiguous by definition */

    /* strides != NULL implies both of these */
    assert(view->ndim > 0);
    assert(view->shape != NULL);

    sd = view->itemsize;
    for (i=view->ndim-1; i>=0; i--) {
        dim = view->shape[i];
        if (dim > 1 && view->strides[i] != sd) {
            return 0;
        }
        sd *= dim;
    }
    return 1;
}

int
PyBuffer_IsContiguous(const Py_buffer *view, char order)
{

    if (view->suboffsets != NULL) return 0;

    if (order == 'C')
        return _IsCContiguous(view);
    else if (order == 'F')
        return _IsFortranContiguous(view);
    else if (order == 'A')
        return (_IsCContiguous(view) || _IsFortranContiguous(view));
    return 0;
}


void*
PyBuffer_GetPointer(const Py_buffer *view, const Py_ssize_t *indices)
{
    char* pointer;
    int i;
    pointer = (char *)view->buf;
    for (i = 0; i < view->ndim; i++) {
        pointer += view->strides[i]*indices[i];
        if ((view->suboffsets != NULL) && (view->suboffsets[i] >= 0)) {
            pointer = *((char**)pointer) + view->suboffsets[i];
        }
    }
    return (void*)pointer;
}


static void
_Py_add_one_to_index_F(int nd, Py_ssize_t *index, const Py_ssize_t *shape)
{
    int k;

    for (k=0; k<nd; k++) {
        if (index[k] < shape[k]-1) {
            index[k]++;
            break;
        }
        else {
            index[k] = 0;
        }
    }
}

static void
_Py_add_one_to_index_C(int nd, Py_ssize_t *index, const Py_ssize_t *shape)
{
    int k;

    for (k=nd-1; k>=0; k--) {
        if (index[k] < shape[k]-1) {
            index[k]++;
            break;
        }
        else {
            index[k] = 0;
        }
    }
}

Py_ssize_t
PyBuffer_SizeFromFormat(const char *format)
{
    PyObject *calcsize = NULL;
    PyObject *res = NULL;
    PyObject *fmt = NULL;
    Py_ssize_t itemsize = -1;

    calcsize = PyImport_ImportModuleAttrString("struct", "calcsize");
    if (calcsize == NULL) {
        goto done;
    }

    fmt = PyUnicode_FromString(format);
    if (fmt == NULL) {
        goto done;
    }

    res = PyObject_CallFunctionObjArgs(calcsize, fmt, NULL);
    if (res == NULL) {
        goto done;
    }

    itemsize = _PyValue_IntAsSsize_t(PyValue_FromObjectBorrow(res));
    if (itemsize < 0) {
        goto done;
    }

done:
    Py_XDECREF(calcsize);
    Py_XDECREF(fmt);
    Py_XDECREF(res);
    return itemsize;
}

int
PyBuffer_FromContiguous(const Py_buffer *view, const void *buf, Py_ssize_t len, char order)
{
    int k;
    void (*addone)(int, Py_ssize_t *, const Py_ssize_t *);
    Py_ssize_t *indices, elements;
    char *ptr;
    const char *src;

    if (len > view->len) {
        len = view->len;
    }

    if (PyBuffer_IsContiguous(view, order)) {
        /* simplest copy is all that is needed */
        memcpy(view->buf, buf, len);
        return 0;
    }

    /* Otherwise a more elaborate scheme is needed */

    /* view->ndim <= 64 */
    indices = (Py_ssize_t *)PyMem_Malloc(sizeof(Py_ssize_t)*(view->ndim));
    if (indices == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (k=0; k<view->ndim;k++) {
        indices[k] = 0;
    }

    if (order == 'F') {
        addone = _Py_add_one_to_index_F;
    }
    else {
        addone = _Py_add_one_to_index_C;
    }
    src = buf;
    /* XXX : This is not going to be the fastest code in the world
             several optimizations are possible.
     */
    elements = len / view->itemsize;
    while (elements--) {
        ptr = PyBuffer_GetPointer(view, indices);
        memcpy(ptr, src, view->itemsize);
        src += view->itemsize;
        addone(view->ndim, indices, view->shape);
    }

    PyMem_Free(indices);
    return 0;
}

int PyObject_CopyData(PyObject *dest, PyObject *src)
{
    Py_buffer view_dest, view_src;
    int k;
    Py_ssize_t *indices, elements;
    char *dptr, *sptr;

    if (!PyObject_CheckBuffer(dest) ||
        !PyObject_CheckBuffer(src)) {
        PyErr_SetString(PyExc_TypeError,
                        "both destination and source must be "\
                        "bytes-like objects");
        return -1;
    }

    if (PyObject_GetBuffer(dest, &view_dest, PyBUF_FULL) != 0) return -1;
    if (PyObject_GetBuffer(src, &view_src, PyBUF_FULL_RO) != 0) {
        PyBuffer_Release(&view_dest);
        return -1;
    }

    if (view_dest.len < view_src.len) {
        PyErr_SetString(PyExc_BufferError,
                        "destination is too small to receive data from source");
        PyBuffer_Release(&view_dest);
        PyBuffer_Release(&view_src);
        return -1;
    }

    if ((PyBuffer_IsContiguous(&view_dest, 'C') &&
         PyBuffer_IsContiguous(&view_src, 'C')) ||
        (PyBuffer_IsContiguous(&view_dest, 'F') &&
         PyBuffer_IsContiguous(&view_src, 'F'))) {
        /* simplest copy is all that is needed */
        memcpy(view_dest.buf, view_src.buf, view_src.len);
        PyBuffer_Release(&view_dest);
        PyBuffer_Release(&view_src);
        return 0;
    }

    /* Otherwise a more elaborate copy scheme is needed */

    /* XXX(nnorwitz): need to check for overflow! */
    indices = (Py_ssize_t *)PyMem_Malloc(sizeof(Py_ssize_t)*view_src.ndim);
    if (indices == NULL) {
        PyErr_NoMemory();
        PyBuffer_Release(&view_dest);
        PyBuffer_Release(&view_src);
        return -1;
    }
    for (k=0; k<view_src.ndim;k++) {
        indices[k] = 0;
    }
    elements = 1;
    for (k=0; k<view_src.ndim; k++) {
        /* XXX(nnorwitz): can this overflow? */
        elements *= view_src.shape[k];
    }
    while (elements--) {
        _Py_add_one_to_index_C(view_src.ndim, indices, view_src.shape);
        dptr = PyBuffer_GetPointer(&view_dest, indices);
        sptr = PyBuffer_GetPointer(&view_src, indices);
        memcpy(dptr, sptr, view_src.itemsize);
    }
    PyMem_Free(indices);
    PyBuffer_Release(&view_dest);
    PyBuffer_Release(&view_src);
    return 0;
}

void
PyBuffer_FillContiguousStrides(int nd, Py_ssize_t *shape,
                               Py_ssize_t *strides, int itemsize,
                               char order)
{
    int k;
    Py_ssize_t sd;

    sd = itemsize;
    if (order == 'F') {
        for (k=0; k<nd; k++) {
            strides[k] = sd;
            sd *= shape[k];
        }
    }
    else {
        for (k=nd-1; k>=0; k--) {
            strides[k] = sd;
            sd *= shape[k];
        }
    }
    return;
}

int
PyBuffer_FillInfo(Py_buffer *view, PyObject *obj, void *buf, Py_ssize_t len,
                  int readonly, int flags)
{
    if (view == NULL) {
        PyErr_SetString(PyExc_BufferError,
                        "PyBuffer_FillInfo: view==NULL argument is obsolete");
        return -1;
    }

    if (flags != PyBUF_SIMPLE) {  /* fast path */
        if (flags == PyBUF_READ || flags == PyBUF_WRITE) {
            PyErr_BadInternalCall();
            return -1;
        }
        if (((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE) &&
            (readonly == 1)) {
            PyErr_SetString(PyExc_BufferError,
                            "Object is not writable.");
            return -1;
        }
    }

    view->obj = Py_XNewRef(obj);
    view->buf = buf;
    view->len = len;
    view->readonly = readonly;
    view->itemsize = 1;
    view->format = NULL;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        view->format = "B";
    view->ndim = 1;
    view->shape = NULL;
    if ((flags & PyBUF_ND) == PyBUF_ND)
        view->shape = &(view->len);
    view->strides = NULL;
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
        view->strides = &(view->itemsize);
    view->suboffsets = NULL;
    view->internal = NULL;
    return 0;
}

void
PyBuffer_Release(Py_buffer *view)
{
    PyObject *obj = view->obj;
    PyBufferProcs *pb;
    if (obj == NULL)
        return;
    pb = Py_TYPE(obj)->tp_as_buffer;
    if (pb && pb->bf_releasebuffer) {
        pb->bf_releasebuffer(obj, view);
    }
    view->obj = NULL;
    Py_DECREF(obj);
}

static int
_buffer_release_call(void *arg)
{
    PyBuffer_Release((Py_buffer *)arg);
    return 0;
}

int
_PyBuffer_ReleaseInInterpreter(PyInterpreterState *interp,
                               Py_buffer *view)
{
    return _Py_CallInInterpreter(interp, _buffer_release_call, view);
}

int
_PyBuffer_ReleaseInInterpreterAndRawFree(PyInterpreterState *interp,
                                         Py_buffer *view)
{
    return _Py_CallInInterpreterAndRawFree(interp, _buffer_release_call, view);
}

/* format(obj, format_spec): obj.__format__(format_spec), with obj as a
   value, so that an inline int or float is not boxed. format_spec is
   PyValue_NULL for none. */
PyObject *
(_PyValue_Format)(PyValueBorrow obj, PyValueBorrow format_spec)
{
    PyObject *spec = NULL;
    PyObject *result = NULL;

    if (!PyValue_IsNull(format_spec)) {
        if (!PyValue_IsObject(format_spec)
            || !PyUnicode_Check(PyValue_AsKnownObjectBorrow(format_spec)))
        {
            PyErr_Format(PyExc_SystemError,
                         "Format specifier must be a string, not %.200s",
                         PyValue_TYPE(format_spec)->tp_name);
            return NULL;
        }
        spec = PyValue_AsKnownObjectBorrow(format_spec);
    }

    /* Fast path for common types. */
    if (spec == NULL || PyUnicode_GET_LENGTH(spec) == 0) {
        if (PyValue_IsObject(obj)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(obj);
            if (PyUnicode_CheckExact(o)) {
                return Py_NewRef(o);
            }
            if (PyLong_CheckExact(o)) {
                return (_PyValue_Str)(obj);
            }
        }
        else if (PyValue_IsInlineInt(obj)) {
            return (_PyValue_Str)(obj);
        }
    }

    /* If no format_spec is provided, use an empty string */
    PyObject *empty = NULL;
    if (spec == NULL) {
        empty = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
        spec = empty;
    }

    /* Find the (unbound!) __format__ method */
    PyThreadState *tstate = _PyThreadState_GET();
    _PyStatusValue lookup = (_PyValue_LookupSpecial)(obj, &_Py_ID(__format__));
    if (lookup.status <= 0) {
        if (lookup.status == 0 && !_PyErr_Occurred(tstate)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "Type %.100s doesn't define __format__",
                          PyValue_TYPE(obj)->tp_name);
        }
        goto done;
    }
    PyValue meth = lookup.value;

    /* And call it. */
    PyValue res = (_PyValue_CallOneArg)(tstate, PyValue_Borrow(meth),
                                        PyValue_FromObjectBorrow(spec));
    PyValue_CLOSE(meth);
    if (PyValue_IsNull(res)) {
        goto done;
    }
    result = _PyValue_AsObjectStealAt(res, BOX_SITE_REPR);
    if (result && !PyUnicode_Check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "%N.__format__() must return a str, not %T",
                     PyValue_TYPE(obj), result);
        Py_SETREF(result, NULL);
        goto done;
    }

done:
    Py_XDECREF(empty);
    return result;
}

PyObject *
PyObject_Format(PyObject *obj, PyObject *format_spec)
{
    return (_PyValue_Format)(_Py_VALUE_ARG(obj), _Py_VALUE_ARG(format_spec));
}
/* Operations on numbers */

int
PyNumber_Check(PyObject *o)
{
    if (o == NULL)
        return 0;
    PyNumberMethods *nb = Py_TYPE(o)->tp_as_number;
    return nb && (nb->nb_index || nb->nb_int || nb->nb_float || PyComplex_Check(o));
}

/* Binary operators */

#define NB_SLOT(x) offsetof(PyNumberMethods, x)
#define NB_BINOP(nb_methods, slot) \
        (*(binaryfunc*)(& ((char*)nb_methods)[slot]))
#define NB_TERNOP(nb_methods, slot) \
        (*(ternaryfunc*)(& ((char*)nb_methods)[slot]))
// The value row of a number slot.
#define NB_ROW(x) _Py_VALUE_SLOT_INDEX_##x

/* The number protocol on values (pyjit/design/proposals/value-api-v2a0.md,
   section 4.6). Each dispatcher _PyValue_<Op> is the port of the object
   function of the operation, which is now its thunk. Where upstream calls
   a type's object slot, the dispatcher calls the type's value slot of the
   same row if it is set, else the object slot through the adapter: the
   same slot function either way, with the same arguments in the same
   order. Deduplication compares object slots, as upstream does (D8): under
   the pairing invariant the object slot identifies the pair. */

#ifndef NDEBUG
// _Py_CheckSlotResult() for a slot of type, whose operand may be inline.
static int
check_value_slot_result(PyTypeObject *type, const char *slot_name,
                        int success)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!success && !_PyErr_Occurred(tstate)) {
        _Py_FatalErrorFormat(__func__,
                             "Slot %s of type %s failed "
                             "without setting an exception",
                             slot_name, type->tp_name);
    }
    if (success && _PyErr_Occurred(tstate)) {
        _Py_FatalErrorFormat(__func__,
                             "Slot %s of type %s succeeded "
                             "with an exception set",
                             slot_name, type->tp_name);
    }
    return 1;
}
#endif

static inline int
value_is_notimplemented(PyValue res)
{
    return PyValue_Is(res, PyValue_NotImplemented);
}

static void
value_type_error(const char *msg, PyValueBorrow v)
{
    PyErr_Format(PyExc_TypeError, msg, PyValue_TYPE(v)->tp_name);
}

/* The adapter (D9), one function per signature kind: an object slot called
   with value operands, boxed under site class NUMBER (_PyValue_AdapterBox()).
   The result becomes a value through PyValue_FromObjectSteal(), which
   makes an exact int or float inline. */

#define adapter_box(v, f) _PyValue_AdapterBox((v), BOX_SITE_NUMBER, (f))
#define adapter_release(v, obj) _PyValue_AdapterRelease((v), (obj))

static PyValue
adapt_unary(unaryfunc f, PyValueBorrow v)
{
    PyObject *a = adapter_box(v, f);
    if (a == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = f(a);
    adapter_release(v, a);
    return _PyValue_FromObjectResult(res);
}

static PyValue
adapt_binary(binaryfunc f, PyValueBorrow v, PyValueBorrow w)
{
    PyObject *a = adapter_box(v, f);
    if (a == NULL) {
        return PyValue_NULL;
    }
    PyObject *b = adapter_box(w, f);
    if (b == NULL) {
        adapter_release(v, a);
        return PyValue_NULL;
    }
    PyObject *res = f(a, b);
    adapter_release(w, b);
    adapter_release(v, a);
    return _PyValue_FromObjectResult(res);
}

static PyValue
adapt_ternary(ternaryfunc f, PyValueBorrow v, PyValueBorrow w,
              PyValueBorrow z)
{
    PyObject *a = adapter_box(v, f);
    if (a == NULL) {
        return PyValue_NULL;
    }
    PyObject *b = adapter_box(w, f);
    if (b == NULL) {
        adapter_release(v, a);
        return PyValue_NULL;
    }
    PyObject *c = adapter_box(z, f);
    if (c == NULL) {
        adapter_release(w, b);
        adapter_release(v, a);
        return PyValue_NULL;
    }
    PyObject *res = f(a, b, c);
    adapter_release(z, c);
    adapter_release(w, b);
    adapter_release(v, a);
    return _PyValue_FromObjectResult(res);
}

// The sequence stays an object; an inline other operand is boxed.
static PyValue
adapt_concat(binaryfunc f, PyObject *seq, PyValueBorrow w)
{
    PyObject *b = adapter_box(w, f);
    if (b == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = f(seq, b);
    adapter_release(w, b);
    return _PyValue_FromObjectResult(res);
}

/* Call the slot of row of type t, whose object side is slot: the value
   slot if it is set, else slot through the adapter. */

static inline PyValue
call_unary(PyTypeObject *t, unaryfunc slot, int row, PyValueBorrow v)
{
    _Py_unaryvaluefunc f = (_Py_unaryvaluefunc)_PyType_VALUE_FIELD_AT(t, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, row);
        return f(v);
    }
    return adapt_unary(slot, v);
}

/* The value field of row of type t. The binary and ternary dispatchers
   read it where they read the object slot beside it, as a pair: a slot
   that they call first can run Python code that changes the type, and
   upstream then still calls the object slot that it read before. */
static inline _Py_valuefunc
read_value_field(PyTypeObject *t, int row)
{
    _Py_valuefunc f = _PyType_VALUE_FIELD_AT(t, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, row);
    }
    return f;
}

// Call the pair (slot, value) read by read_value_field().
static inline PyValue
call_binary(binaryfunc slot, _Py_valuefunc value,
            PyValueBorrow v, PyValueBorrow w)
{
    if (value != NULL) {
        return ((_Py_binaryvaluefunc)value)(v, w);
    }
    return adapt_binary(slot, v, w);
}

static inline PyValue
call_ternary(ternaryfunc slot, _Py_valuefunc value,
             PyValueBorrow v, PyValueBorrow w, PyValueBorrow z)
{
    if (value != NULL) {
        return ((_Py_ternaryvaluefunc)value)(v, w, z);
    }
    return adapt_ternary(slot, v, w, z);
}

PyValue
_PyValue_CallPowerBase(PyTypeObject *base, ternaryfunc slot,
                       PyValueBorrow v, PyValueBorrow w, PyValueBorrow z)
{
    int row = _Py_VALUE_SLOT_INDEX_nb_power;
    _Py_ternaryvaluefunc f =
        (_Py_ternaryvaluefunc)_PyType_VALUE_FIELD_AT(base, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(base, row);
        return f(v, w, z);
    }
    return adapt_ternary(slot, v, w, z);
}

// sq_concat and sq_inplace_concat; v holds the sequence, an object.
static inline PyValue
call_concat(PyTypeObject *t, binaryfunc slot, int row,
            PyValueBorrow v, PyValueBorrow w)
{
    PyObject *seq = PyValue_AsKnownObjectBorrow(v);
    _Py_concatvaluefunc f =
        (_Py_concatvaluefunc)_PyType_VALUE_FIELD_AT(t, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, row);
        return f(seq, w);
    }
    return adapt_concat(slot, seq, w);
}

/*
  Calling scheme used for binary operations:

  Order operations are tried until either a valid result or error:
    w.op(v,w)[*], v.op(v,w), w.op(v,w)

  [*] only when Py_TYPE(v) != Py_TYPE(w) && Py_TYPE(w) is a subclass of
      Py_TYPE(v)
 */

static PyValue
binary_op1(PyValueBorrow v, PyValueBorrow w, const int op_slot,
           const int row, const char *op_name)
{
    PyTypeObject *tv = PyValue_TYPE(v);
    PyTypeObject *tw = PyValue_TYPE(w);
    binaryfunc slotv;
    if (tv->tp_as_number != NULL) {
        slotv = NB_BINOP(tv->tp_as_number, op_slot);
    }
    else {
        slotv = NULL;
    }

    binaryfunc slotw;
    if (tw != tv && tw->tp_as_number != NULL) {
        slotw = NB_BINOP(tw->tp_as_number, op_slot);
        if (slotw == slotv) {
            slotw = NULL;
        }
    }
    else {
        slotw = NULL;
    }
    _Py_valuefunc valuev = slotv ? read_value_field(tv, row) : NULL;
    _Py_valuefunc valuew = slotw ? read_value_field(tw, row) : NULL;

    if (slotv) {
        PyValue x;
        if (slotw && PyType_IsSubtype(tw, tv)) {
            x = call_binary(slotw, valuew, v, w);
            if (!value_is_notimplemented(x))
                return x;
            PyValue_CLOSE(x); /* can't do it */
            slotw = NULL;
        }
        x = call_binary(slotv, valuev, v, w);
        assert(check_value_slot_result(tv, op_name, !PyValue_IsNull(x)));
        if (!value_is_notimplemented(x)) {
            return x;
        }
        PyValue_CLOSE(x); /* can't do it */
    }
    if (slotw) {
        PyValue x = call_binary(slotw, valuew, v, w);
        assert(check_value_slot_result(tw, op_name, !PyValue_IsNull(x)));
        if (!value_is_notimplemented(x)) {
            return x;
        }
        PyValue_CLOSE(x); /* can't do it */
    }
    return PyValue_NotImplemented;
}

#define BINARY_OP1(v, w, op, op_name) \
    binary_op1(v, w, NB_SLOT(op), NB_ROW(op), op_name)

// BINARY_OP1() for object operands, with an object result.
#define OBJECT_BINARY_OP1(v, w, op, op_name) \
    _PyValue_THUNK_EXIT(BINARY_OP1(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w), \
                                   op, op_name), BOX_SITE_NUMBER)

static PyValue
binop_type_error(PyValueBorrow v, PyValueBorrow w, const char *op_name)
{
    PyErr_Format(PyExc_TypeError,
                 "unsupported operand type(s) for %.100s: "
                 "'%.100s' and '%.100s'",
                 op_name,
                 PyValue_TYPE(v)->tp_name,
                 PyValue_TYPE(w)->tp_name);
    return PyValue_NULL;
}

static PyValue
binary_op(PyValueBorrow v, PyValueBorrow w, const int op_slot,
          const int row, const char *op_name)
{
    PyValue result = binary_op1(v, w, op_slot, row, op_name);
    if (value_is_notimplemented(result)) {
        PyValue_CLOSE(result);
        return binop_type_error(v, w, op_name);
    }
    return result;
}


/*
  Calling scheme used for ternary operations:

  Order operations are tried until either a valid result or error:
    v.op(v,w,z), w.op(v,w,z), z.op(v,w,z)
 */

static PyValue
ternary_op(PyValueBorrow v,
           PyValueBorrow w,
           PyValueBorrow z,
           const int op_slot,
           const int row,
           const char *op_name
           )
{
    PyTypeObject *tv = PyValue_TYPE(v);
    PyTypeObject *tw = PyValue_TYPE(w);
    PyNumberMethods *mv = tv->tp_as_number;
    PyNumberMethods *mw = tw->tp_as_number;

    ternaryfunc slotv;
    if (mv != NULL) {
        slotv = NB_TERNOP(mv, op_slot);
    }
    else {
        slotv = NULL;
    }

    ternaryfunc slotw;
    if (tw != tv && mw != NULL) {
        slotw = NB_TERNOP(mw, op_slot);
        if (slotw == slotv) {
            slotw = NULL;
        }
    }
    else {
        slotw = NULL;
    }
    _Py_valuefunc valuev = slotv ? read_value_field(tv, row) : NULL;
    _Py_valuefunc valuew = slotw ? read_value_field(tw, row) : NULL;

    if (slotv) {
        PyValue x;
        if (slotw && PyType_IsSubtype(tw, tv)) {
            x = call_ternary(slotw, valuew, v, w, z);
            if (!value_is_notimplemented(x)) {
                return x;
            }
            PyValue_CLOSE(x); /* can't do it */
            slotw = NULL;
        }
        x = call_ternary(slotv, valuev, v, w, z);
        assert(check_value_slot_result(tv, op_name, !PyValue_IsNull(x)));
        if (!value_is_notimplemented(x)) {
            return x;
        }
        PyValue_CLOSE(x); /* can't do it */
    }
    if (slotw) {
        PyValue x = call_ternary(slotw, valuew, v, w, z);
        assert(check_value_slot_result(tw, op_name, !PyValue_IsNull(x)));
        if (!value_is_notimplemented(x)) {
            return x;
        }
        PyValue_CLOSE(x); /* can't do it */
    }

    PyTypeObject *tz = PyValue_TYPE(z);
    PyNumberMethods *mz = tz->tp_as_number;
    if (mz != NULL) {
        ternaryfunc slotz = NB_TERNOP(mz, op_slot);
        if (slotz == slotv || slotz == slotw) {
            slotz = NULL;
        }
        if (slotz) {
            PyValue x = call_ternary(slotz, read_value_field(tz, row),
                                     v, w, z);
            assert(check_value_slot_result(tz, op_name, !PyValue_IsNull(x)));
            if (!value_is_notimplemented(x)) {
                return x;
            }
            PyValue_CLOSE(x); /* can't do it */
        }
    }

    if (PyValue_Is(z, PyValue_None)) {
        PyErr_Format(
            PyExc_TypeError,
            "unsupported operand type(s) for %.100s: "
            "'%.100s' and '%.100s'",
            op_name,
            tv->tp_name,
            tw->tp_name);
    }
    else {
        PyErr_Format(
            PyExc_TypeError,
            "unsupported operand type(s) for %.100s: "
            "'%.100s', '%.100s', '%.100s'",
            op_name,
            tv->tp_name,
            tw->tp_name,
            tz->tp_name);
    }
    return PyValue_NULL;
}

/* A dispatcher and its thunk, the object function. */
#define BINARY_FUNC(func, name, op, op_name) \
    PyValue \
    (_PyValue_##name)(PyValueBorrow v, PyValueBorrow w) \
    { \
        return binary_op(v, w, NB_SLOT(op), NB_ROW(op), op_name); \
    } \
    \
    PyObject * \
    func(PyObject *v, PyObject *w) { \
        return _PyValue_THUNK_EXIT( \
            (_PyValue_##name)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w)), \
            BOX_SITE_NUMBER); \
    }

BINARY_FUNC(PyNumber_Or, Or, nb_or, "|")
BINARY_FUNC(PyNumber_Xor, Xor, nb_xor, "^")
BINARY_FUNC(PyNumber_And, And, nb_and, "&")
BINARY_FUNC(PyNumber_Lshift, Lshift, nb_lshift, "<<")
BINARY_FUNC(PyNumber_Rshift, Rshift, nb_rshift, ">>")
BINARY_FUNC(PyNumber_Subtract, Subtract, nb_subtract, "-")
BINARY_FUNC(PyNumber_Divmod, Divmod, nb_divmod, "divmod()")

/* Sequence concatenation after the number slots: sq_concat's value side
   takes the other operand as a borrow (section 4.6). */
PyValue
(_PyValue_Add)(PyValueBorrow v, PyValueBorrow w)
{
    PyValue result = BINARY_OP1(v, w, nb_add, "+");
    if (!value_is_notimplemented(result)) {
        return result;
    }
    PyValue_CLOSE(result);

    PyTypeObject *tv = PyValue_TYPE(v);
    PySequenceMethods *m = tv->tp_as_sequence;
    if (m && m->sq_concat) {
        result = call_concat(tv, m->sq_concat, _Py_VALUE_SLOT_INDEX_sq_concat,
                             v, w);
        assert(check_value_slot_result(tv, "+", !PyValue_IsNull(result)));
        return result;
    }

    return binop_type_error(v, w, "+");
}

PyObject *
PyNumber_Add(PyObject *v, PyObject *w)
{
    return _PyValue_THUNK_EXIT(
        (_PyValue_Add)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w)), BOX_SITE_NUMBER);
}

/* seq * n; seq holds a sequence, an object. sq_repeat has no value side. */
static PyValue
sequence_repeat(ssizeargfunc repeatfunc, PyValueBorrow seq, PyValueBorrow n)
{
    Py_ssize_t count;
    PyNumberMethods *mn = PyValue_TYPE(n)->tp_as_number;
    if (mn != NULL && mn->nb_index != NULL) {
        count = (_PyValue_AsSsize_t)(n, PyExc_OverflowError);
        if (count == -1 && PyErr_Occurred()) {
            return PyValue_NULL;
        }
    }
    else {
        value_type_error("can't multiply sequence by "
                         "non-int of type '%.200s'", n);
        return PyValue_NULL;
    }
    PyObject *obj = PyValue_AsKnownObjectBorrow(seq);
    PyObject *res = (*repeatfunc)(obj, count);
    assert(_Py_CheckSlotResult(obj, "*", res != NULL));
    return _PyValue_FromObjectResult(res);
}

PyValue
(_PyValue_Multiply)(PyValueBorrow v, PyValueBorrow w)
{
    PyValue result = BINARY_OP1(v, w, nb_multiply, "*");
    if (value_is_notimplemented(result)) {
        PySequenceMethods *mv = PyValue_TYPE(v)->tp_as_sequence;
        PySequenceMethods *mw = PyValue_TYPE(w)->tp_as_sequence;
        PyValue_CLOSE(result);
        if  (mv && mv->sq_repeat) {
            return sequence_repeat(mv->sq_repeat, v, w);
        }
        else if (mw && mw->sq_repeat) {
            return sequence_repeat(mw->sq_repeat, w, v);
        }
        result = binop_type_error(v, w, "*");
    }
    return result;
}

PyObject *
PyNumber_Multiply(PyObject *v, PyObject *w)
{
    return _PyValue_THUNK_EXIT(
        (_PyValue_Multiply)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w)),
        BOX_SITE_NUMBER);
}

BINARY_FUNC(PyNumber_MatrixMultiply, MatrixMultiply, nb_matrix_multiply, "@")
BINARY_FUNC(PyNumber_FloorDivide, FloorDivide, nb_floor_divide, "//")
BINARY_FUNC(PyNumber_TrueDivide, TrueDivide, nb_true_divide, "/")
BINARY_FUNC(PyNumber_Remainder, Remainder, nb_remainder, "%")

PyValue
(_PyValue_Power)(PyValueBorrow v, PyValueBorrow w, PyValueBorrow z)
{
    return ternary_op(v, w, z, NB_SLOT(nb_power), NB_ROW(nb_power),
                      "** or pow()");
}

PyObject *
PyNumber_Power(PyObject *v, PyObject *w, PyObject *z)
{
    return _PyValue_THUNK_EXIT((_PyValue_Power)(
        _Py_VALUE_ARG(v), _Py_VALUE_ARG(w), _Py_VALUE_ARG(z)),
        BOX_SITE_NUMBER);
}

/* Binary in-place operators */

/* The in-place operators are defined to fall back to the 'normal',
   non in-place operations, if the in-place methods are not in place.

   - If the left hand object has the appropriate struct members, and
     they are filled, call the appropriate function and return the
     result.  No coercion is done on the arguments; the left-hand object
     is the one the operation is performed on, and it's up to the
     function to deal with the right-hand object.

   - Otherwise, in-place modification is not supported. Handle it exactly as
     a non in-place operation of the same kind.

   */

static PyValue
binary_iop1(PyValueBorrow v, PyValueBorrow w, const int iop_slot,
            const int iop_row, const int op_slot, const int op_row,
            const char *op_name)
{
    PyTypeObject *tv = PyValue_TYPE(v);
    PyNumberMethods *mv = tv->tp_as_number;
    if (mv != NULL) {
        binaryfunc slot = NB_BINOP(mv, iop_slot);
        if (slot) {
            PyValue x = call_binary(slot, read_value_field(tv, iop_row),
                                    v, w);
            assert(check_value_slot_result(tv, op_name, !PyValue_IsNull(x)));
            if (!value_is_notimplemented(x)) {
                return x;
            }
            PyValue_CLOSE(x);
        }
    }
    return binary_op1(v, w, op_slot, op_row, op_name);
}

#define BINARY_IOP1(v, w, iop, op, op_name) \
    binary_iop1(v, w, NB_SLOT(iop), NB_ROW(iop), NB_SLOT(op), NB_ROW(op), \
                op_name)

// BINARY_IOP1() for object operands, with an object result.
#define OBJECT_BINARY_IOP1(v, w, iop, op, op_name) \
    _PyValue_THUNK_EXIT(BINARY_IOP1(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w), \
                                    iop, op, op_name), BOX_SITE_NUMBER)

static PyValue
binary_iop(PyValueBorrow v, PyValueBorrow w, const int iop_slot,
           const int iop_row, const int op_slot, const int op_row,
           const char *op_name)
{
    PyValue result = binary_iop1(v, w, iop_slot, iop_row, op_slot, op_row,
                                 op_name);
    if (value_is_notimplemented(result)) {
        PyValue_CLOSE(result);
        return binop_type_error(v, w, op_name);
    }
    return result;
}

static PyValue
ternary_iop(PyValueBorrow v, PyValueBorrow w, PyValueBorrow z,
            const int iop_slot, const int iop_row, const int op_slot,
            const int op_row, const char *op_name)
{
    PyTypeObject *tv = PyValue_TYPE(v);
    PyNumberMethods *mv = tv->tp_as_number;
    if (mv != NULL) {
        ternaryfunc slot = NB_TERNOP(mv, iop_slot);
        if (slot) {
            PyValue x = call_ternary(slot, read_value_field(tv, iop_row),
                                     v, w, z);
            if (!value_is_notimplemented(x)) {
                return x;
            }
            PyValue_CLOSE(x);
        }
    }
    return ternary_op(v, w, z, op_slot, op_row, op_name);
}

#define INPLACE_BINOP(func, name, iop, op, op_name) \
    PyValue \
    (_PyValue_InPlace##name)(PyValueBorrow v, PyValueBorrow w) \
    { \
        return binary_iop(v, w, NB_SLOT(iop), NB_ROW(iop), \
                          NB_SLOT(op), NB_ROW(op), op_name); \
    } \
    \
    PyObject * \
    func(PyObject *v, PyObject *w) { \
        return _PyValue_THUNK_EXIT( \
            (_PyValue_InPlace##name)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w)), \
            BOX_SITE_NUMBER); \
    }

INPLACE_BINOP(PyNumber_InPlaceOr, Or, nb_inplace_or, nb_or, "|=")
INPLACE_BINOP(PyNumber_InPlaceXor, Xor, nb_inplace_xor, nb_xor, "^=")
INPLACE_BINOP(PyNumber_InPlaceAnd, And, nb_inplace_and, nb_and, "&=")
INPLACE_BINOP(PyNumber_InPlaceLshift, Lshift, nb_inplace_lshift, nb_lshift, "<<=")
INPLACE_BINOP(PyNumber_InPlaceRshift, Rshift, nb_inplace_rshift, nb_rshift, ">>=")
INPLACE_BINOP(PyNumber_InPlaceSubtract, Subtract, nb_inplace_subtract, nb_subtract, "-=")
INPLACE_BINOP(PyNumber_InPlaceMatrixMultiply, MatrixMultiply, nb_inplace_matrix_multiply, nb_matrix_multiply, "@=")
INPLACE_BINOP(PyNumber_InPlaceFloorDivide, FloorDivide, nb_inplace_floor_divide, nb_floor_divide, "//=")
INPLACE_BINOP(PyNumber_InPlaceTrueDivide, TrueDivide, nb_inplace_true_divide, nb_true_divide,  "/=")
INPLACE_BINOP(PyNumber_InPlaceRemainder, Remainder, nb_inplace_remainder, nb_remainder, "%=")

PyValue
(_PyValue_InPlaceAdd)(PyValueBorrow v, PyValueBorrow w)
{
    PyValue result = BINARY_IOP1(v, w, nb_inplace_add, nb_add, "+=");
    if (value_is_notimplemented(result)) {
        PyTypeObject *tv = PyValue_TYPE(v);
        PySequenceMethods *m = tv->tp_as_sequence;
        PyValue_CLOSE(result);
        if (m != NULL) {
            binaryfunc func = m->sq_inplace_concat;
            int row = _Py_VALUE_SLOT_INDEX_sq_inplace_concat;
            if (func == NULL) {
                func = m->sq_concat;
                row = _Py_VALUE_SLOT_INDEX_sq_concat;
            }
            if (func != NULL) {
                result = call_concat(tv, func, row, v, w);
                assert(check_value_slot_result(tv, "+=",
                                               !PyValue_IsNull(result)));
                return result;
            }
        }
        result = binop_type_error(v, w, "+=");
    }
    return result;
}

PyObject *
PyNumber_InPlaceAdd(PyObject *v, PyObject *w)
{
    return _PyValue_THUNK_EXIT(
        (_PyValue_InPlaceAdd)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w)),
        BOX_SITE_NUMBER);
}

PyValue
(_PyValue_InPlaceMultiply)(PyValueBorrow v, PyValueBorrow w)
{
    PyValue result = BINARY_IOP1(v, w, nb_inplace_multiply, nb_multiply,
                                 "*=");
    if (value_is_notimplemented(result)) {
        ssizeargfunc f = NULL;
        PySequenceMethods *mv = PyValue_TYPE(v)->tp_as_sequence;
        PySequenceMethods *mw = PyValue_TYPE(w)->tp_as_sequence;
        PyValue_CLOSE(result);
        if (mv != NULL) {
            f = mv->sq_inplace_repeat;
            if (f == NULL)
                f = mv->sq_repeat;
            if (f != NULL)
                return sequence_repeat(f, v, w);
        }
        else if (mw != NULL) {
            /* Note that the right hand operand should not be
             * mutated in this case so sq_inplace_repeat is not
             * used. */
            if (mw->sq_repeat)
                return sequence_repeat(mw->sq_repeat, w, v);
        }
        result = binop_type_error(v, w, "*=");
    }
    return result;
}

PyObject *
PyNumber_InPlaceMultiply(PyObject *v, PyObject *w)
{
    return _PyValue_THUNK_EXIT(
        (_PyValue_InPlaceMultiply)(_Py_VALUE_ARG(v), _Py_VALUE_ARG(w)),
        BOX_SITE_NUMBER);
}

PyValue
(_PyValue_InPlacePower)(PyValueBorrow v, PyValueBorrow w, PyValueBorrow z)
{
    return ternary_iop(v, w, z, NB_SLOT(nb_inplace_power),
                       NB_ROW(nb_inplace_power), NB_SLOT(nb_power),
                       NB_ROW(nb_power), "**=");
}

PyObject *
PyNumber_InPlacePower(PyObject *v, PyObject *w, PyObject *z)
{
    return _PyValue_THUNK_EXIT((_PyValue_InPlacePower)(
        _Py_VALUE_ARG(v), _Py_VALUE_ARG(w), _Py_VALUE_ARG(z)),
        BOX_SITE_NUMBER);
}

/* The value dispatchers by BINARY_OP's oparg (D14). */

static PyValue
value_power_no_mod(PyValueBorrow v, PyValueBorrow w)
{
    return (_PyValue_Power)(v, w, PyValue_Borrow(PyValue_None));
}

static PyValue
value_inplace_power_no_mod(PyValueBorrow v, PyValueBorrow w)
{
    return (_PyValue_InPlacePower)(v, w, PyValue_Borrow(PyValue_None));
}

const _PyValue_binaryop _PyValue_BinaryOps[] = {
    [NB_ADD] = (_PyValue_Add),
    [NB_AND] = (_PyValue_And),
    [NB_FLOOR_DIVIDE] = (_PyValue_FloorDivide),
    [NB_LSHIFT] = (_PyValue_Lshift),
    [NB_MATRIX_MULTIPLY] = (_PyValue_MatrixMultiply),
    [NB_MULTIPLY] = (_PyValue_Multiply),
    [NB_REMAINDER] = (_PyValue_Remainder),
    [NB_OR] = (_PyValue_Or),
    [NB_POWER] = value_power_no_mod,
    [NB_RSHIFT] = (_PyValue_Rshift),
    [NB_SUBTRACT] = (_PyValue_Subtract),
    [NB_TRUE_DIVIDE] = (_PyValue_TrueDivide),
    [NB_XOR] = (_PyValue_Xor),
    [NB_INPLACE_ADD] = (_PyValue_InPlaceAdd),
    [NB_INPLACE_AND] = (_PyValue_InPlaceAnd),
    [NB_INPLACE_FLOOR_DIVIDE] = (_PyValue_InPlaceFloorDivide),
    [NB_INPLACE_LSHIFT] = (_PyValue_InPlaceLshift),
    [NB_INPLACE_MATRIX_MULTIPLY] = (_PyValue_InPlaceMatrixMultiply),
    [NB_INPLACE_MULTIPLY] = (_PyValue_InPlaceMultiply),
    [NB_INPLACE_REMAINDER] = (_PyValue_InPlaceRemainder),
    [NB_INPLACE_OR] = (_PyValue_InPlaceOr),
    [NB_INPLACE_POWER] = value_inplace_power_no_mod,
    [NB_INPLACE_RSHIFT] = (_PyValue_InPlaceRshift),
    [NB_INPLACE_SUBTRACT] = (_PyValue_InPlaceSubtract),
    [NB_INPLACE_TRUE_DIVIDE] = (_PyValue_InPlaceTrueDivide),
    [NB_INPLACE_XOR] = (_PyValue_InPlaceXor),
    [NB_SUBSCR] = NULL,
};
static_assert(Py_ARRAY_LENGTH(_PyValue_BinaryOps) == NB_SUBSCR + 1,
              "one dispatcher per BINARY_OP oparg");


/* Unary operators and functions */

#define UNARY_FUNC(func, name, op, meth_name, descr)                     \
    PyValue                                                              \
    (_PyValue_##name)(PyValueBorrow o)                                   \
    {                                                                    \
        if (PyValue_IsNull(o)) {                                         \
            null_error();                                                \
            return PyValue_NULL;                                         \
        }                                                                \
                                                                         \
        PyTypeObject *t = PyValue_TYPE(o);                               \
        PyNumberMethods *m = t->tp_as_number;                            \
        if (m && m->op) {                                                \
            PyValue res = call_unary(t, m->op, NB_ROW(op), o);           \
            assert(check_value_slot_result(t, #meth_name,                \
                                           !PyValue_IsNull(res)));       \
            return res;                                                  \
        }                                                                \
                                                                         \
        value_type_error("bad operand type for "descr": '%.200s'", o);   \
        return PyValue_NULL;                                             \
    }                                                                    \
                                                                         \
    PyObject *                                                           \
    func(PyObject *o) {                                                  \
        return _PyValue_THUNK_EXIT((_PyValue_##name)(_Py_VALUE_ARG(o)), \
                                   BOX_SITE_NUMBER); \
    }

UNARY_FUNC(PyNumber_Negative, Negative, nb_negative, __neg__, "unary -")
UNARY_FUNC(PyNumber_Positive, Positive, nb_positive, __pos__, "unary +")
UNARY_FUNC(PyNumber_Invert, Invert, nb_invert, __invert__, "unary ~")
UNARY_FUNC(PyNumber_Absolute, Absolute, nb_absolute, __abs__, "abs()")


int
PyIndex_Check(PyObject *obj)
{
    return _PyIndex_Check(obj);
}

static inline int
value_long_check(PyValueBorrow v)
{
    return PyType_FastSubclass(PyValue_TYPE(v), Py_TPFLAGS_LONG_SUBCLASS);
}

#define value_long_check(V) (value_long_check)(PyValue_Borrow(V))

static inline int
value_long_check_exact(PyValueBorrow v)
{
    return PyValue_TYPE(v) == &PyLong_Type;
}
#define value_long_check_exact(V) (value_long_check_exact)(PyValue_Borrow(V))


/* Return a Python int from the object item.
   Can return an instance of int subclass.
   Raise TypeError if the result is not an int
   or if the object cannot be interpreted as an index.
*/
PyValue
(_PyValue_Index)(PyValueBorrow item)
{
    if (PyValue_IsNull(item)) {
        null_error();
        return PyValue_NULL;
    }

    if (value_long_check(item)) {
        return PyValue_DUP(item);
    }
    PyTypeObject *t = PyValue_TYPE(item);
    if (t->tp_as_number == NULL || t->tp_as_number->nb_index == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "'%.200s' object cannot be interpreted "
                     "as an integer", t->tp_name);
        return PyValue_NULL;
    }

    PyValue result = call_unary(t, t->tp_as_number->nb_index,
                                NB_ROW(nb_index), item);
    assert(check_value_slot_result(t, "__index__", !PyValue_IsNull(result)));
    if (PyValue_IsNull(result) || value_long_check_exact(result)) {
        return result;
    }

    if (!value_long_check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "%N.__index__() must return an int, not %N",
                     t, PyValue_TYPE(result));
        PyValue_CLOSE(result);
        return PyValue_NULL;
    }
    /* Issue #17576: warn if 'result' not of exact type int. */
    if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
            "%N.__index__() must return an int, not %N.  "
            "The ability to return an instance of a strict subclass of int "
            "is deprecated, and may be removed in a future version of Python.",
            t, PyValue_TYPE(result))) {
        PyValue_CLOSE(result);
        return PyValue_NULL;
    }
    return result;
}

PyObject *
_PyNumber_Index(PyObject *item)
{
    return _PyValue_THUNK_EXIT((_PyValue_Index)(_Py_VALUE_ARG(item)),
                               BOX_SITE_NUMBER);
}

/* Return an exact Python int from the object item: an inline int or an
   exact int is itself, a subclass instance (or an __index__ result of a
   subclass) is copied.
   Raise TypeError if the result is not an int
   or if the object cannot be interpreted as an index.
*/
PyValue
(_PyValue_IndexExact)(PyValueBorrow item)
{
    PyValue result = (_PyValue_Index)(item);
    if (PyValue_IsNull(result) || value_long_check_exact(result)) {
        return result;
    }
    PyObject *copy = (PyObject *)_PyLong_Copy(
        (PyLongObject *)PyValue_AsKnownObjectBorrow(result));
    PyValue_CLOSE(result);
    return _PyValue_FromObjectResult(copy);
}

PyObject *
PyNumber_Index(PyObject *item)
{
    return _PyValue_THUNK_EXIT((_PyValue_IndexExact)(_Py_VALUE_ARG(item)),
                               BOX_SITE_NUMBER);
}

/* Return an error on Overflow only if err is not NULL*/

Py_ssize_t
(_PyValue_AsSsize_t)(PyValueBorrow item, PyObject *err)
{
    Py_ssize_t result;
    PyObject *runerr;
    PyValue value = (_PyValue_Index)(item);
    if (PyValue_IsNull(value))
        return -1;

    // An inline int fits: it has at most 51 bits.
    if (PyValue_IsInlineInt(value)) {
        return (Py_ssize_t)PyValue_InlineInt64(value);
    }

    /* We're done if _PyValue_IntAsSsize_t() returns without error. */
    result = _PyValue_IntAsSsize_t(value);
    if (result != -1)
        goto finish;

    PyThreadState *tstate = _PyThreadState_GET();
    runerr = _PyErr_Occurred(tstate);
    if (!runerr) {
        goto finish;
    }

    /* Error handling code -- only manage OverflowError differently */
    if (!PyErr_GivenExceptionMatches(runerr, PyExc_OverflowError)) {
        goto finish;
    }
    _PyErr_Clear(tstate);

    /* If no error-handling desired then the default clipping
       is sufficient. */
    if (!err) {
        PyObject *obj = PyValue_AsKnownObjectBorrow(value);
        assert(PyLong_Check(obj));
        /* Whether or not it is less than or equal to
           zero is determined by the sign of ob_size
        */
        if (_PyLong_IsNegative((PyLongObject *)obj))
            result = PY_SSIZE_T_MIN;
        else
            result = PY_SSIZE_T_MAX;
    }
    else {
        /* Otherwise replace the error with caller's error object. */
        _PyErr_Format(tstate, err,
                      "cannot fit '%.200s' into an index-sized integer",
                      PyValue_TYPE(item)->tp_name);
    }

 finish:
    PyValue_CLOSE(value);
    return result;
}

Py_ssize_t
PyNumber_AsSsize_t(PyObject *item, PyObject *err)
{
    return (_PyValue_AsSsize_t)(_Py_VALUE_ARG(item), err);
}


PyValue
(_PyValue_Long)(PyValueBorrow o)
{
    PyValue result;
    PyNumberMethods *m;
    Py_buffer view;

    if (PyValue_IsNull(o)) {
        null_error();
        return PyValue_NULL;
    }

    if (value_long_check_exact(o)) {
        return PyValue_DUP(o);
    }
    PyTypeObject *t = PyValue_TYPE(o);
    m = t->tp_as_number;
    if (m && m->nb_int) { /* This should include subclasses of int */
        /* Convert using the nb_int slot, which should return something
           of exact type int. */
        result = call_unary(t, m->nb_int, NB_ROW(nb_int), o);
        assert(check_value_slot_result(t, "__int__", !PyValue_IsNull(result)));
        if (PyValue_IsNull(result) || value_long_check_exact(result)) {
            return result;
        }

        if (!value_long_check(result)) {
            PyErr_Format(PyExc_TypeError,
                         "%N.__int__() must return an int, not %N",
                         t, PyValue_TYPE(result));
            PyValue_CLOSE(result);
            return PyValue_NULL;
        }
        /* Issue #17576: warn if 'result' not of exact type int. */
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "%N.__int__() must return an int, not %N.  "
                "The ability to return an instance of a strict subclass of int "
                "is deprecated, and may be removed in a future version of Python.",
                t, PyValue_TYPE(result))) {
            PyValue_CLOSE(result);
            return PyValue_NULL;
        }
        // A strict subclass of int is an object.
        PyObject *copy = _PyLong_Copy(
            (PyLongObject *)PyValue_AsKnownObjectBorrow(result));
        PyValue_CLOSE(result);
        return _PyValue_FromObjectResult(copy);
    }
    if (m && m->nb_index) {
        // As PyNumber_Index(): an exact int.
        PyValue index = (_PyValue_Index)(o);
        if (PyValue_IsNull(index) || value_long_check_exact(index)) {
            return index;
        }
        PyObject *copy = _PyLong_Copy(
            (PyLongObject *)PyValue_AsKnownObjectBorrow(index));
        PyValue_CLOSE(index);
        return _PyValue_FromObjectResult(copy);
    }

    // Every inline value is an int or a float, which have nb_int.
    PyObject *obj = PyValue_AsKnownObjectBorrow(o);
    if (PyUnicode_Check(obj))
        /* The below check is done in PyLong_FromUnicodeObject(). */
        return _PyValue_FromObjectResult(PyLong_FromUnicodeObject(obj, 10));

    if (PyBytes_Check(obj))
        /* need to do extra error checking that PyLong_FromString()
         * doesn't do.  In particular int('9\x005') must raise an
         * exception, not truncate at the null.
         */
        return _PyValue_FromObjectResult(_PyLong_FromBytes(
            PyBytes_AS_STRING(obj), PyBytes_GET_SIZE(obj), 10));

    if (PyByteArray_Check(obj))
        return _PyValue_FromObjectResult(_PyLong_FromBytes(
            PyByteArray_AS_STRING(obj), PyByteArray_GET_SIZE(obj), 10));

    if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) == 0) {
        PyObject *bytes;

        /* Copy to NUL-terminated buffer. */
        bytes = PyBytes_FromStringAndSize((const char *)view.buf, view.len);
        if (bytes == NULL) {
            PyBuffer_Release(&view);
            return PyValue_NULL;
        }
        PyObject *res = _PyLong_FromBytes(PyBytes_AS_STRING(bytes),
                                          PyBytes_GET_SIZE(bytes), 10);
        Py_DECREF(bytes);
        PyBuffer_Release(&view);
        return _PyValue_FromObjectResult(res);
    }

    type_error("int() argument must be a string, a bytes-like object "
               "or a real number, not '%.200s'", obj);
    return PyValue_NULL;
}

PyObject *
PyNumber_Long(PyObject *o)
{
    return _PyValue_THUNK_EXIT((_PyValue_Long)(_Py_VALUE_ARG(o)),
                               BOX_SITE_NUMBER);
}

PyValue
(_PyValue_Float)(PyValueBorrow o)
{
    if (PyValue_IsNull(o)) {
        null_error();
        return PyValue_NULL;
    }

    PyTypeObject *t = PyValue_TYPE(o);
    if (t == &PyFloat_Type) {
        return PyValue_DUP(o);
    }

    PyNumberMethods *m = t->tp_as_number;
    if (m && m->nb_float) { /* This should include subclasses of float */
        PyValue res = call_unary(t, m->nb_float, NB_ROW(nb_float), o);
        assert(check_value_slot_result(t, "__float__", !PyValue_IsNull(res)));
        if (PyValue_IsNull(res) || PyValue_TYPE(res) == &PyFloat_Type) {
            return res;
        }

        if (!PyType_IsSubtype(PyValue_TYPE(res), &PyFloat_Type)) {
            PyErr_Format(PyExc_TypeError,
                         "%N.__float__() must return a float, not %N",
                         t, PyValue_TYPE(res));
            PyValue_CLOSE(res);
            return PyValue_NULL;
        }
        /* Issue #26983: warn if 'res' not of exact type float. */
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "%N.__float__() must return a float, not %N.  "
                "The ability to return an instance of a strict subclass of float "
                "is deprecated, and may be removed in a future version of Python.",
                t, PyValue_TYPE(res))) {
            PyValue_CLOSE(res);
            return PyValue_NULL;
        }
        // A strict subclass of float is an object.
        double val = PyFloat_AS_DOUBLE(PyValue_AsKnownObjectBorrow(res));
        PyValue_CLOSE(res);
        return PyValue_FromDouble(val);
    }

    if (m && m->nb_index) {
        PyValue res = (_PyValue_Index)(o);
        if (PyValue_IsNull(res)) {
            return PyValue_NULL;
        }
        double val = _PyValue_IntAsDouble(res);
        PyValue_CLOSE(res);
        if (val == -1.0 && PyErr_Occurred()) {
            return PyValue_NULL;
        }
        return PyValue_FromDouble(val);
    }

    // Every inline value is an int or a float, which have nb_float.
    PyObject *obj = PyValue_AsKnownObjectBorrow(o);
    /* A float subclass with nb_float == NULL */
    if (PyFloat_Check(obj)) {
        return PyValue_FromDouble(PyFloat_AS_DOUBLE(obj));
    }
    return _PyValue_FromObjectResult(PyFloat_FromString(obj));
}

PyObject *
PyNumber_Float(PyObject *o)
{
    return _PyValue_THUNK_EXIT((_PyValue_Float)(_Py_VALUE_ARG(o)),
                               BOX_SITE_NUMBER);
}


PyObject *
PyNumber_ToBase(PyObject *n, int base)
{
    if (!(base == 2 || base == 8 || base == 10 || base == 16)) {
        PyErr_SetString(PyExc_SystemError,
                        "PyNumber_ToBase: base must be 2, 8, 10 or 16");
        return NULL;
    }
    PyObject *index = _PyValue_AsObjectResultAt(
        _PyValue_Index(PyValue_FromObjectBorrow(n)), BOX_SITE_NUMBER);
    if (!index)
        return NULL;
    PyObject *res = _PyLong_Format(index, base);
    Py_DECREF(index);
    return res;
}


/* Operations on sequences */

int
(_PyValue_SequenceCheck)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no sq_item.
        return 0;
    }
    PyObject *s = PyValue_AsKnownObjectBorrow(v);
    if (PyDict_Check(s))
        return 0;
    return Py_TYPE(s)->tp_as_sequence &&
        Py_TYPE(s)->tp_as_sequence->sq_item != NULL;
}

int
PySequence_Check(PyObject *s)
{
    return _PyValue_SequenceCheck(_Py_VALUE_ARG(s));
}

Py_ssize_t
(_PyValue_SequenceSize)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        value_type_error("object of type '%.200s' has no len()", v);
        return -1;
    }
    PyObject *s = PyValue_AsKnownObjectBorrow(v);
    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_length) {
        Py_ssize_t len = m->sq_length(s);
        assert(_Py_CheckSlotResult(s, "__len__", len >= 0));
        return len;
    }

    if (Py_TYPE(s)->tp_as_mapping && Py_TYPE(s)->tp_as_mapping->mp_length) {
        type_error("%.200s is not a sequence", s);
        return -1;
    }
    type_error("object of type '%.200s' has no len()", s);
    return -1;
}

Py_ssize_t
PySequence_Size(PyObject *s)
{
    if (s == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_SequenceSize(_Py_VALUE_ARG(s));
}

#undef PySequence_Length
Py_ssize_t
PySequence_Length(PyObject *s)
{
    if (s == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_SequenceSize(_Py_VALUE_ARG(s));
}
#define PySequence_Length PySequence_Size

PyValue
(_PyValue_SequenceConcat)(PyValueBorrow v, PyValueBorrow w)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no sq_concat and is not a sequence.
        value_type_error("'%.200s' object can't be concatenated", v);
        return PyValue_NULL;
    }
    PyTypeObject *tv = PyValue_TYPE(v);
    PySequenceMethods *m = tv->tp_as_sequence;
    if (m && m->sq_concat) {
        PyValue res = call_concat(tv, m->sq_concat,
                                  _Py_VALUE_SLOT_INDEX_sq_concat, v, w);
        assert(check_value_slot_result(tv, "+", !PyValue_IsNull(res)));
        return res;
    }

    /* Instances of user classes defining an __add__() method only
       have an nb_add slot, not an sq_concat slot.      So we fall back
       to nb_add if both arguments appear to be sequences. */
    if (_PyValue_SequenceCheck(v) && _PyValue_SequenceCheck(w)) {
        PyValue result = BINARY_OP1(v, w, nb_add, "+");
        if (!value_is_notimplemented(result))
            return result;
        PyValue_CLOSE(result);
    }
    value_type_error("'%.200s' object can't be concatenated", v);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PySequence_Concat(PyObject *s, PyObject *o)
{
    if (s == NULL || o == NULL) {
        return null_error();
    }
    return _PyValue_THUNK_EXIT(
        _PyValue_SequenceConcat(_Py_VALUE_ARG(s), _Py_VALUE_ARG(o)), BOX_SITE_SEQUENCE);
}

PyValue
(_PyValue_SequenceRepeat)(PyValueBorrow v, Py_ssize_t count)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no sq_repeat and is not a sequence.
        value_type_error("'%.200s' object can't be repeated", v);
        return PyValue_NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_repeat) {
        PyObject *res = m->sq_repeat(o, count);
        assert(_Py_CheckSlotResult(o, "*", res != NULL));
        return _PyValue_FromObjectResult(res);
    }

    /* Instances of user classes defining a __mul__() method only
       have an nb_multiply slot, not an sq_repeat slot. so we fall back
       to nb_multiply if o appears to be a sequence. */
    if (_PyValue_SequenceCheck(v)) {
        PyObject *n = PyLong_FromSsize_t(count);
        if (n == NULL)
            return PyValue_NULL;
        PyValue result = BINARY_OP1(v, _Py_VALUE_ARG(n), nb_multiply, "*");
        Py_DECREF(n);
        if (!value_is_notimplemented(result))
            return result;
        PyValue_CLOSE(result);
    }
    value_type_error("'%.200s' object can't be repeated", v);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PySequence_Repeat(PyObject *o, Py_ssize_t count)
{
    if (o == NULL) {
        return null_error();
    }
    return _PyValue_THUNK_EXIT(
        _PyValue_SequenceRepeat(_Py_VALUE_ARG(o), count), BOX_SITE_SEQUENCE);
}

PyValue
(_PyValue_SequenceInPlaceConcat)(PyValueBorrow v, PyValueBorrow w)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no sq_concat and is not a sequence.
        value_type_error("'%.200s' object can't be concatenated", v);
        return PyValue_NULL;
    }
    PyTypeObject *tv = PyValue_TYPE(v);
    PySequenceMethods *m = tv->tp_as_sequence;
    if (m && m->sq_inplace_concat) {
        PyValue res = call_concat(tv, m->sq_inplace_concat,
                                  _Py_VALUE_SLOT_INDEX_sq_inplace_concat,
                                  v, w);
        assert(check_value_slot_result(tv, "+=", !PyValue_IsNull(res)));
        return res;
    }
    if (m && m->sq_concat) {
        PyValue res = call_concat(tv, m->sq_concat,
                                  _Py_VALUE_SLOT_INDEX_sq_concat, v, w);
        assert(check_value_slot_result(tv, "+", !PyValue_IsNull(res)));
        return res;
    }

    if (_PyValue_SequenceCheck(v) && _PyValue_SequenceCheck(w)) {
        PyValue result = BINARY_IOP1(v, w, nb_inplace_add, nb_add, "+=");
        if (!value_is_notimplemented(result))
            return result;
        PyValue_CLOSE(result);
    }
    value_type_error("'%.200s' object can't be concatenated", v);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PySequence_InPlaceConcat(PyObject *s, PyObject *o)
{
    if (s == NULL || o == NULL) {
        return null_error();
    }
    return _PyValue_THUNK_EXIT(
        _PyValue_SequenceInPlaceConcat(_Py_VALUE_ARG(s), _Py_VALUE_ARG(o)), BOX_SITE_SEQUENCE);
}

PyValue
(_PyValue_SequenceInPlaceRepeat)(PyValueBorrow v, Py_ssize_t count)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no sq_repeat and is not a sequence.
        value_type_error("'%.200s' object can't be repeated", v);
        return PyValue_NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_inplace_repeat) {
        PyObject *res = m->sq_inplace_repeat(o, count);
        assert(_Py_CheckSlotResult(o, "*=", res != NULL));
        return _PyValue_FromObjectResult(res);
    }
    if (m && m->sq_repeat) {
        PyObject *res = m->sq_repeat(o, count);
        assert(_Py_CheckSlotResult(o, "*", res != NULL));
        return _PyValue_FromObjectResult(res);
    }

    if (_PyValue_SequenceCheck(v)) {
        PyObject *n = PyLong_FromSsize_t(count);
        if (n == NULL)
            return PyValue_NULL;
        PyValue result = BINARY_IOP1(v, _Py_VALUE_ARG(n), nb_inplace_multiply,
                                     nb_multiply, "*=");
        Py_DECREF(n);
        if (!value_is_notimplemented(result))
            return result;
        PyValue_CLOSE(result);
    }
    value_type_error("'%.200s' object can't be repeated", v);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PySequence_InPlaceRepeat(PyObject *o, Py_ssize_t count)
{
    if (o == NULL) {
        return null_error();
    }
    return _PyValue_THUNK_EXIT(
        _PyValue_SequenceInPlaceRepeat(_Py_VALUE_ARG(o), count), BOX_SITE_SEQUENCE);
}

PyValue
(_PyValue_SequenceGetItem)(PyValueBorrow v, Py_ssize_t i)
{
    if (PyValue_IsNull(v)) {
        null_error();
        return PyValue_NULL;
    }
    if (PyValue_IsInline(v)) {
        value_type_error("'%.200s' object does not support indexing", v);
        return PyValue_NULL;
    }
    PyObject *s = PyValue_AsKnownObjectBorrow(v);

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_item) {
        if (i < 0) {
            if (m->sq_length) {
                Py_ssize_t l = (*m->sq_length)(s);
                assert(_Py_CheckSlotResult(s, "__len__", l >= 0));
                if (l < 0) {
                    return PyValue_NULL;
                }
                i += l;
            }
        }
        PyValue res = call_item(s, m->sq_item, i);
        assert(_Py_CheckSlotResult(s, "__getitem__", !PyValue_IsNull(res)));
        return res;
    }

    if (Py_TYPE(s)->tp_as_mapping && Py_TYPE(s)->tp_as_mapping->mp_subscript) {
        type_error("%.200s is not a sequence", s);
        return PyValue_NULL;
    }
    type_error("'%.200s' object does not support indexing", s);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PySequence_GetItem(PyObject *s, Py_ssize_t i)
{
    return _PyValue_THUNK_EXIT(_PyValue_SequenceGetItem(_Py_VALUE_ARG(s), i), BOX_SITE_SEQUENCE);
}

PyValue
(_PyValue_SequenceGetSlice)(PyValueBorrow v, Py_ssize_t i1, Py_ssize_t i2)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no mp_subscript.
        value_type_error("'%.200s' object is unsliceable", v);
        return PyValue_NULL;
    }
    PyObject *s = PyValue_AsKnownObjectBorrow(v);
    PyTypeObject *type = Py_TYPE(s);
    PyMappingMethods *mp = type->tp_as_mapping;
    binaryfunc object_subscript = mp ? mp->mp_subscript : NULL;
    _Py_subscriptvaluefunc value_subscript =
        _PyType_VALUE_SLOT(type, subscript);
    if (object_subscript != NULL) {
        if (value_subscript != NULL) {
            _PyType_CHECK_VALUE_ROW(type, _Py_VALUE_SLOT_INDEX_subscript);
        }
        PyObject *slice = _PySlice_FromIndices(i1, i2);
        if (!slice) {
            return PyValue_NULL;
        }
        PyValue res;
        if (value_subscript != NULL) {
            res = value_subscript(s, PyValue_FromObjectBorrow(slice));
            assert(_Py_CheckSlotResult(s, "__getitem__",
                                       !PyValue_IsNull(res)));
        }
        else {
            PyObject *object_result = object_subscript(s, slice);
            assert(_Py_CheckSlotResult(s, "__getitem__",
                                       object_result != NULL));
            res = _PyValue_FromObjectResult(object_result);
        }
        Py_DECREF(slice);
        return res;
    }

    type_error("'%.200s' object is unsliceable", s);
    return PyValue_NULL;
}

Py_VALUE_ENTRY PyObject *
PySequence_GetSlice(PyObject *s, Py_ssize_t i1, Py_ssize_t i2)
{
    if (!s) {
        return null_error();
    }
    return _PyValue_THUNK_EXIT(
        _PyValue_SequenceGetSlice(_Py_VALUE_ARG(s), i1, i2), BOX_SITE_SEQUENCE);
}

// s[i] = value, or del s[i] for a PyValue_NULL value.
static int
value_sequence_ass_item(PyValueBorrow v, Py_ssize_t i, PyValueBorrow value)
{
    int deleting = PyValue_IsNull(value);
    if (PyValue_IsNull(v)) {
        null_error();
        return -1;
    }
    if (!PyValue_IsInline(v)) {
        PyObject *s = PyValue_AsKnownObjectBorrow(v);
        PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
        if (m && m->sq_ass_item) {
            if (i < 0) {
                if (m->sq_length) {
                    Py_ssize_t l = (*m->sq_length)(s);
                    assert(_Py_CheckSlotResult(s, "__len__", l >= 0));
                    if (l < 0) {
                        return -1;
                    }
                    i += l;
                }
            }
            int res = call_ass_item(s, m->sq_ass_item, i, value);
            assert(_Py_CheckSlotResult(s, deleting ? "__delitem__"
                                                   : "__setitem__",
                                       res >= 0));
            return res;
        }

        if (Py_TYPE(s)->tp_as_mapping
            && Py_TYPE(s)->tp_as_mapping->mp_ass_subscript)
        {
            type_error("%.200s is not a sequence", s);
            return -1;
        }
    }
    value_type_error(deleting
                     ? "'%.200s' object doesn't support item deletion"
                     : "'%.200s' object does not support item assignment", v);
    return -1;
}

int
(_PyValue_SequenceSetItem)(PyValueBorrow v, Py_ssize_t i, PyValueBorrow value)
{
    /* Upstream passes a NULL item on to sq_ass_item, which deletes; the
       value form keeps that. */
    return value_sequence_ass_item(v, i, value);
}

int
(_PyValue_SequenceDelItem)(PyValueBorrow v, Py_ssize_t i)
{
    return value_sequence_ass_item(v, i, PyValue_Borrow(PyValue_NULL));
}

int
PySequence_SetItem(PyObject *s, Py_ssize_t i, PyObject *o)
{
    return _PyValue_SequenceSetItem(_Py_VALUE_ARG(s), i, _Py_VALUE_ARG(o));
}

int
PySequence_DelItem(PyObject *s, Py_ssize_t i)
{
    return _PyValue_SequenceDelItem(_Py_VALUE_ARG(s), i);
}

int
(_PyValue_SequenceSetSlice)(PyValueBorrow v, Py_ssize_t i1, Py_ssize_t i2,
                            PyValueBorrow w)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no mp_ass_subscript.
        value_type_error("'%.200s' object doesn't support slice assignment",
                         v);
        return -1;
    }
    PyObject *s = PyValue_AsKnownObjectBorrow(v);
    PyMappingMethods *mp = Py_TYPE(s)->tp_as_mapping;
    if (mp && mp->mp_ass_subscript) {
        // A NULL item deletes the slice, as upstream passes it through.
        PyObject *o = NULL;
        if (!PyValue_IsNull(w)) {
            o = _PyValue_AsObjectAt(w, BOX_SITE_SEQUENCE);
            if (o == NULL) {
                return -1;
            }
        }
        PyObject *slice = _PySlice_FromIndices(i1, i2);
        if (!slice) {
            Py_XDECREF(o);
            return -1;
        }
        int res = mp->mp_ass_subscript(s, slice, o);
        assert(_Py_CheckSlotResult(s, "__setitem__", res >= 0));
        Py_DECREF(slice);
        Py_XDECREF(o);
        return res;
    }

    type_error("'%.200s' object doesn't support slice assignment", s);
    return -1;
}

int
PySequence_SetSlice(PyObject *s, Py_ssize_t i1, Py_ssize_t i2, PyObject *o)
{
    if (s == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_SequenceSetSlice(_Py_VALUE_ARG(s), i1, i2,
                                     _Py_VALUE_ARG(o));
}

int
(_PyValue_SequenceDelSlice)(PyValueBorrow v, Py_ssize_t i1, Py_ssize_t i2)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no mp_ass_subscript.
        value_type_error("'%.200s' object doesn't support slice deletion", v);
        return -1;
    }
    PyObject *s = PyValue_AsKnownObjectBorrow(v);
    PyMappingMethods *mp = Py_TYPE(s)->tp_as_mapping;
    if (mp && mp->mp_ass_subscript) {
        PyObject *slice = _PySlice_FromIndices(i1, i2);
        if (!slice) {
            return -1;
        }
        int res = mp->mp_ass_subscript(s, slice, NULL);
        assert(_Py_CheckSlotResult(s, "__delitem__", res >= 0));
        Py_DECREF(slice);
        return res;
    }
    type_error("'%.200s' object doesn't support slice deletion", s);
    return -1;
}

int
PySequence_DelSlice(PyObject *s, Py_ssize_t i1, Py_ssize_t i2)
{
    if (s == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_SequenceDelSlice(_Py_VALUE_ARG(s), i1, i2);
}

PyObject *
(_PyValue_SequenceTuple)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // tuple(5) fails in PyObject_GetIter().
        value_type_error("'%.200s' object is not iterable", v);
        return NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PyObject *it;  /* iter(v) */

    /* Special-case the common tuple and list cases, for efficiency. */
    if (PyTuple_CheckExact(o)) {
        /* Note that we can't know whether it's safe to return
           a tuple *subclass* instance as-is, hence the restriction
           to exact tuples here.  In contrast, lists always make
           a copy, so there's no need for exactness below. */
        return Py_NewRef(o);
    }
    if (PyList_CheckExact(o))
        return PyList_AsTuple(o);

    /* Get iterator. */
    it = _PyValue_GetIter(v);
    if (it == NULL)
        return NULL;

    Py_ssize_t n;
    PyValue buffer[8];
    for (n = 0; n < 8; n++) {
        _PyStatusValue item = _PyValue_IterNext(it);
        if (item.status == PYGEN_ERROR) {
            goto fail;
        }
        if (item.status != PYGEN_NEXT) {
            PyObject *tuple = _PyTuple_FromValueStealOnSuccess(buffer, n);
            if (tuple == NULL) {
                goto fail;
            }
            Py_DECREF(it);
            return tuple;
        }
        buffer[n] = item.value;
    }
    PyListObject *list = (PyListObject *)PyList_New(16);
    if (list == NULL) {
        goto fail;
    }
    assert(n == 8);
    Py_SET_SIZE(list, n);
    for (Py_ssize_t j = 0; j < n; j++) {
        PyList_SET_VALUE(list, j, buffer[j]);
    }
    for (;;) {
        _PyStatusValue item = _PyValue_IterNext(it);
        if (item.status == PYGEN_ERROR) {
            Py_DECREF(list);
            Py_DECREF(it);
            return NULL;
        }
        if (item.status != PYGEN_NEXT) {
            break;
        }
        if (_PyList_AppendTakeValue(list, item.value) < 0) {
            Py_DECREF(list);
            Py_DECREF(it);
            return NULL;
        }
    }
    Py_DECREF(it);
    PyObject *res = _PyList_AsTupleAndClear(list);
    Py_DECREF(list);
    return res;
fail:
    Py_DECREF(it);
    while (n > 0) {
        n--;
        PyValue_CLOSE(buffer[n]);
    }
    return NULL;
}

PyObject *
PySequence_Tuple(PyObject *v)
{
    if (v == NULL) {
        return null_error();
    }
    return _PyValue_SequenceTuple(_Py_VALUE_ARG(v));
}

PyObject *
(_PyValue_SequenceList)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // list(5) fails in PyObject_GetIter().
        value_type_error("'%.200s' object is not iterable", v);
        return NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PyObject *result;  /* result list */
    PyObject *rv;          /* return value from PyList_Extend */

    result = PyList_New(0);
    if (result == NULL)
        return NULL;

    rv = _PyList_Extend((PyListObject *)result, o);
    if (rv == NULL) {
        Py_DECREF(result);
        return NULL;
    }
    Py_DECREF(rv);
    return result;
}

PyObject *
PySequence_List(PyObject *v)
{
    if (v == NULL) {
        return null_error();
    }
    return _PyValue_SequenceList(_Py_VALUE_ARG(v));
}

PyObject *
(_PyValue_SequenceFast)(PyValueBorrow v, const char *m)
{
    if (PyValue_IsInline(v)) {
        // PyObject_GetIter() fails with a TypeError, which becomes m.
        PyErr_SetString(PyExc_TypeError, m);
        return NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PyObject *it;

    if (PyList_CheckExact(o) || PyTuple_CheckExact(o)) {
        return Py_NewRef(o);
    }

    it = _PyValue_GetIter(v);
    if (it == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_SetString(tstate, PyExc_TypeError, m);
        }
        return NULL;
    }

    o = _PyValue_SequenceList(_Py_VALUE_ARG(it));
    Py_DECREF(it);

    return o;
}

PyObject *
PySequence_Fast(PyObject *v, const char *m)
{
    if (v == NULL) {
        return null_error();
    }
    return _PyValue_SequenceFast(_Py_VALUE_ARG(v), m);
}

/* Iterate over seq.  Result depends on the operation:
   PY_ITERSEARCH_COUNT:  -1 if error, else # of times obj appears in seq.
   PY_ITERSEARCH_INDEX:  0-based index of first occurrence of obj in seq;
    set ValueError and return -1 if none found; also return -1 on error.
   PY_ITERSEARCH_CONTAINS:  return 1 if obj in seq, else 0; -1 on error.
*/
Py_ssize_t
(_PyValue_IterSearch)(PyValueBorrow seq, PyValueBorrow obj, int operation)
{
    Py_ssize_t n;
    int wrapped;  /* for PY_ITERSEARCH_INDEX, true iff n wrapped around */
    PyObject *it;  /* iter(seq) */

    if (PyValue_IsNull(seq) || PyValue_IsNull(obj)) {
        null_error();
        return -1;
    }

    it = _PyValue_GetIter(seq);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            if (operation == PY_ITERSEARCH_CONTAINS) {
                value_type_error(
                    "argument of type '%.200s' is not a container or iterable",
                    seq
                    );
            }
            else {
                value_type_error("argument of type '%.200s' is not iterable",
                                 seq);
            }
        }
        return -1;
    }

    n = wrapped = 0;
    for (;;) {
        int cmp;
        _PyStatusValue step = _PyValue_IterNext(it);
        if (step.status < 0) {
            goto Fail;
        }
        if (step.status == 0) {
            break;
        }

        cmp = _PyValue_RichCompareBool(step.value, obj, Py_EQ);
        PyValue_CLOSE(step.value);
        if (cmp < 0)
            goto Fail;
        if (cmp > 0) {
            switch (operation) {
            case PY_ITERSEARCH_COUNT:
                if (n == PY_SSIZE_T_MAX) {
                    PyErr_SetString(PyExc_OverflowError,
                           "count exceeds C integer size");
                    goto Fail;
                }
                ++n;
                break;

            case PY_ITERSEARCH_INDEX:
                if (wrapped) {
                    PyErr_SetString(PyExc_OverflowError,
                           "index exceeds C integer size");
                    goto Fail;
                }
                goto Done;

            case PY_ITERSEARCH_CONTAINS:
                n = 1;
                goto Done;

            default:
                Py_UNREACHABLE();
            }
        }

        if (operation == PY_ITERSEARCH_INDEX) {
            if (n == PY_SSIZE_T_MAX)
                wrapped = 1;
            ++n;
        }
    }

    if (operation != PY_ITERSEARCH_INDEX)
        goto Done;

    PyErr_SetString(PyExc_ValueError,
                    "sequence.index(x): x not in sequence");
    /* fall into failure code */
Fail:
    n = -1;
    /* fall through */
Done:
    Py_DECREF(it);
    return n;

}

Py_ssize_t
_PySequence_IterSearch(PyObject *seq, PyObject *obj, int operation)
{
    return _PyValue_IterSearch(_Py_VALUE_ARG(seq), _Py_VALUE_ARG(obj),
                               operation);
}

/* Return # of times o appears in s. */
Py_ssize_t
(_PyValue_SequenceCount)(PyValueBorrow s, PyValueBorrow o)
{
    // An int or float receiver fails in PyObject_GetIter() inside the search.
    return _PyValue_IterSearch(s, o, PY_ITERSEARCH_COUNT);
}

Py_ssize_t
PySequence_Count(PyObject *s, PyObject *o)
{
    if (s == NULL || o == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_SequenceCount(_Py_VALUE_ARG(s), _Py_VALUE_ARG(o));
}

/* Return -1 if error; 1 if ob in seq; 0 if ob not in seq.
 * Use sq_contains if possible, else defer to _PySequence_IterSearch().
 */
int
(_PyValue_Contains)(PyValueBorrow seq, PyValueBorrow ob)
{
    if (PyValue_IsNull(ob)) {
        /* A NULL operand is a bad call even for an empty container, as
           PyObject_RichCompare() reports upstream. */
        PyErr_BadInternalCall();
        return -1;
    }
    if (!PyValue_IsInline(seq)) {
        PyObject *o = PyValue_AsKnownObjectBorrow(seq);
        PySequenceMethods *sqm = Py_TYPE(o)->tp_as_sequence;
        if (sqm != NULL && sqm->sq_contains != NULL) {
            int res = call_contains(o, sqm->sq_contains, ob);
            assert(_Py_CheckSlotResult(o, "__contains__", res >= 0));
            return res;
        }
    }
    Py_ssize_t result = _PyValue_IterSearch(seq, ob, PY_ITERSEARCH_CONTAINS);
    return Py_SAFE_DOWNCAST(result, Py_ssize_t, int);
}

int
PySequence_Contains(PyObject *seq, PyObject *ob)
{
    return _PyValue_Contains(_Py_VALUE_ARG(seq), _Py_VALUE_ARG(ob));
}

/* Backwards compatibility */
#undef PySequence_In
int
PySequence_In(PyObject *w, PyObject *v)
{
    return _PyValue_Contains(_Py_VALUE_ARG(w), _Py_VALUE_ARG(v));
}

Py_ssize_t
(_PyValue_SequenceIndex)(PyValueBorrow s, PyValueBorrow o)
{
    // An int or float receiver fails in PyObject_GetIter() inside the search.
    return _PyValue_IterSearch(s, o, PY_ITERSEARCH_INDEX);
}

Py_ssize_t
PySequence_Index(PyObject *s, PyObject *o)
{
    if (s == NULL || o == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_SequenceIndex(_Py_VALUE_ARG(s), _Py_VALUE_ARG(o));
}

/* Operations on mappings */

int
(_PyValue_MappingCheck)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // An int or float has no mp_subscript.
        return 0;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    return Py_TYPE(o)->tp_as_mapping &&
        Py_TYPE(o)->tp_as_mapping->mp_subscript;
}

int
PyMapping_Check(PyObject *o)
{
    if (o == NULL) {
        return 0;
    }
    return _PyValue_MappingCheck(_Py_VALUE_ARG(o));
}

Py_ssize_t
(_PyValue_MappingSize)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        value_type_error("object of type '%.200s' has no len()", v);
        return -1;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_length) {
        Py_ssize_t len = m->mp_length(o);
        assert(_Py_CheckSlotResult(o, "__len__", len >= 0));
        return len;
    }

    if (Py_TYPE(o)->tp_as_sequence && Py_TYPE(o)->tp_as_sequence->sq_length) {
        type_error("%.200s is not a mapping", o);
        return -1;
    }
    /* PyMapping_Size() can be called from PyObject_Size(). */
    type_error("object of type '%.200s' has no len()", o);
    return -1;
}

Py_ssize_t
PyMapping_Size(PyObject *o)
{
    if (o == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_MappingSize(_Py_VALUE_ARG(o));
}

#undef PyMapping_Length
Py_ssize_t
PyMapping_Length(PyObject *o)
{
    if (o == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_MappingSize(_Py_VALUE_ARG(o));
}
#define PyMapping_Length PyMapping_Size

PyValue
(_PyValue_MappingGetItemString)(PyValueBorrow o, const char *key)
{
    if (key == NULL) {
        null_error();
        return PyValue_NULL;
    }

    // _PyValue_GetItem() reports an int or float receiver.
    PyObject *okey = PyUnicode_FromString(key);
    if (okey == NULL)
        return PyValue_NULL;
    PyValue r = _PyValue_GetItem(o, _Py_VALUE_ARG(okey));
    Py_DECREF(okey);
    return r;
}

Py_VALUE_ENTRY PyObject *
PyMapping_GetItemString(PyObject *o, const char *key)
{
    if (o == NULL) {
        return null_error();
    }
    return _PyValue_THUNK_EXIT(
        _PyValue_MappingGetItemString(_Py_VALUE_ARG(o), key), BOX_SITE_SEQUENCE);
}

_PyStatusValue
(_PyValue_MappingGetOptionalItemString)(PyValueBorrow obj, const char *key)
{
    if (key == NULL) {
        null_error();
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    PyObject *okey = PyUnicode_FromString(key);
    if (okey == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    _PyStatusValue r = _PyValue_GetOptionalItem(obj, _Py_VALUE_ARG(okey));
    Py_DECREF(okey);
    return r;
}

Py_VALUE_ENTRY int
PyMapping_GetOptionalItemString(PyObject *obj, const char *key, PyObject **result)
{
    return _PyValue_THUNK_EXIT_STATUS(
        _PyValue_MappingGetOptionalItemString(_Py_VALUE_ARG(obj), key),
        result, BOX_SITE_SEQUENCE);
}

int
(_PyValue_MappingSetItemString)(PyValueBorrow o, const char *key,
                                PyValueBorrow value)
{
    if (key == NULL) {
        null_error();
        return -1;
    }

    // _PyValue_SetItem() reports an int or float receiver.
    PyObject *okey = PyUnicode_FromString(key);
    if (okey == NULL)
        return -1;
    int r = _PyValue_SetItem(o, _Py_VALUE_ARG(okey), value);
    Py_DECREF(okey);
    return r;
}

int
PyMapping_SetItemString(PyObject *o, const char *key, PyObject *value)
{
    if (o == NULL || value == NULL) {
        null_error();
        return -1;
    }
    return _PyValue_MappingSetItemString(_Py_VALUE_ARG(o), key,
                                         _Py_VALUE_ARG(value));
}

int
(_PyValue_MappingHasKeyStringWithError)(PyValueBorrow obj, const char *key)
{
    _PyStatusValue r = _PyValue_MappingGetOptionalItemString(obj, key);
    PyValue_XCLOSE(r.value);
    return r.status;
}

int
PyMapping_HasKeyStringWithError(PyObject *obj, const char *key)
{
    return _PyValue_MappingHasKeyStringWithError(_Py_VALUE_ARG(obj), key);
}

int
(_PyValue_MappingHasKeyWithError)(PyValueBorrow obj, PyValueBorrow key)
{
    _PyStatusValue r = _PyValue_GetOptionalItem(obj, key);
    PyValue_XCLOSE(r.value);
    return r.status;
}

int
PyMapping_HasKeyWithError(PyObject *obj, PyObject *key)
{
    return _PyValue_MappingHasKeyWithError(_Py_VALUE_ARG(obj),
                                           _Py_VALUE_ARG(key));
}

int
(_PyValue_MappingHasKeyString)(PyValueBorrow obj, const char *key)
{
    PyValue value = PyValue_NULL;
    int rc;
    if (PyValue_IsNull(obj)) {
        // For backward compatibility.
        // PyMapping_GetOptionalItemString() crashes if obj is NULL.
        null_error();
        rc = -1;
    }
    else {
        _PyStatusValue r = _PyValue_MappingGetOptionalItemString(obj, key);
        rc = r.status;
        value = r.value;
    }
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyMapping_HasKeyString(); consider using "
            "PyMapping_HasKeyStringWithError(), "
            "PyMapping_GetOptionalItemString() or PyMapping_GetItemString()");
        return 0;
    }
    PyValue_XCLOSE(value);
    return rc;
}

int
PyMapping_HasKeyString(PyObject *obj, const char *key)
{
    return _PyValue_MappingHasKeyString(_Py_VALUE_ARG(obj), key);
}

int
(_PyValue_MappingHasKey)(PyValueBorrow obj, PyValueBorrow key)
{
    PyValue value = PyValue_NULL;
    int rc;
    if (PyValue_IsNull(obj) || PyValue_IsNull(key)) {
        // For backward compatibility.
        // PyMapping_GetOptionalItem() crashes if any of them is NULL.
        null_error();
        rc = -1;
    }
    else {
        _PyStatusValue r = _PyValue_GetOptionalItem(obj, key);
        rc = r.status;
        value = r.value;
    }
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyMapping_HasKey(); consider using "
            "PyMapping_HasKeyWithError(), "
            "PyMapping_GetOptionalItem() or PyObject_GetItem()");
        return 0;
    }
    PyValue_XCLOSE(value);
    return rc;
}

int
PyMapping_HasKey(PyObject *obj, PyObject *key)
{
    return _PyValue_MappingHasKey(_Py_VALUE_ARG(obj), _Py_VALUE_ARG(key));
}

/* This function is quite similar to PySequence_Fast(), but specialized to be
   a helper for PyMapping_Keys(), PyMapping_Items() and PyMapping_Values().
 */
static PyObject *
method_output_as_list(PyObject *o, PyObject *meth)
{
    PyObject *it, *result, *meth_output;

    assert(o != NULL);
    meth_output = PyObject_CallMethodNoArgs(o, meth);
    if (meth_output == NULL || PyList_CheckExact(meth_output)) {
        return meth_output;
    }
    it = _PyValue_GetIter(_Py_VALUE_ARG(meth_output));
    if (it == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%T.%U() must return an iterable, not %T",
                          o, meth, meth_output);
        }
        Py_DECREF(meth_output);
        return NULL;
    }
    Py_DECREF(meth_output);
    result = _PyValue_SequenceList(_Py_VALUE_ARG(it));
    Py_DECREF(it);
    return result;
}

PyObject *
(_PyValue_MappingKeys)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // The AttributeError of the failed lookup records the receiver, so
        // the call is made on a box, as upstream makes it on the int.
        PyObject *box = _PyValue_AsObjectAt(v, BOX_SITE_ATTRIBUTE);
        if (box == NULL) {
            return NULL;
        }
        PyObject *res = method_output_as_list(box, &_Py_ID(keys));
        Py_DECREF(box);
        return res;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    if (PyAnyDict_CheckExact(o)) {
        return PyDict_Keys(o);
    }
    return method_output_as_list(o, &_Py_ID(keys));
}

PyObject *
PyMapping_Keys(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }
    return _PyValue_MappingKeys(_Py_VALUE_ARG(o));
}

PyObject *
(_PyValue_MappingItems)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // The AttributeError of the failed lookup records the receiver, so
        // the call is made on a box, as upstream makes it on the int.
        PyObject *box = _PyValue_AsObjectAt(v, BOX_SITE_ATTRIBUTE);
        if (box == NULL) {
            return NULL;
        }
        PyObject *res = method_output_as_list(box, &_Py_ID(items));
        Py_DECREF(box);
        return res;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    if (PyAnyDict_CheckExact(o)) {
        return PyDict_Items(o);
    }
    return method_output_as_list(o, &_Py_ID(items));
}

PyObject *
PyMapping_Items(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }
    return _PyValue_MappingItems(_Py_VALUE_ARG(o));
}

PyObject *
(_PyValue_MappingValues)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        // The AttributeError of the failed lookup records the receiver, so
        // the call is made on a box, as upstream makes it on the int.
        PyObject *box = _PyValue_AsObjectAt(v, BOX_SITE_ATTRIBUTE);
        if (box == NULL) {
            return NULL;
        }
        PyObject *res = method_output_as_list(box, &_Py_ID(values));
        Py_DECREF(box);
        return res;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    if (PyAnyDict_CheckExact(o)) {
        return PyDict_Values(o);
    }
    return method_output_as_list(o, &_Py_ID(values));
}

PyObject *
PyMapping_Values(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }
    return _PyValue_MappingValues(_Py_VALUE_ARG(o));
}

/* isinstance(), issubclass() */

/* abstract_get_bases() has logically 4 return states:
 *
 * 1. getattr(cls, '__bases__') could raise an AttributeError
 * 2. getattr(cls, '__bases__') could raise some other exception
 * 3. getattr(cls, '__bases__') could return a tuple
 * 4. getattr(cls, '__bases__') could return something other than a tuple
 *
 * Only state #3 is a non-error state and only it returns a non-NULL object
 * (it returns the retrieved tuple).
 *
 * Any raised AttributeErrors are masked by clearing the exception and
 * returning NULL.  If an object other than a tuple comes out of __bases__,
 * then again, the return value is NULL.  So yes, these two situations
 * produce exactly the same results: NULL is returned and no error is set.
 *
 * If some exception other than AttributeError is raised, then NULL is also
 * returned, but the exception is not cleared.  That's because we want the
 * exception to be propagated along.
 *
 * Callers are expected to test for PyErr_Occurred() when the return value
 * is NULL to decide whether a valid exception should be propagated or not.
 * When there's no exception to propagate, it's customary for the caller to
 * set a TypeError.
 */
static PyObject *
abstract_get_bases(PyObject *cls)
{
    PyObject *bases = NULL;
    int found = PyObject_GetOptionalAttr(cls, &_Py_ID(__bases__), &bases);
    if (found < 0) {
        Py_CLEAR(bases);
        return NULL;
    }
    if (bases != NULL && !PyTuple_Check(bases)) {
        Py_CLEAR(bases);
        return NULL;
    }
    return bases;
}


static int
abstract_issubclass(PyObject *derived, PyObject *cls)
{
    PyObject *selected = Py_NewRef(derived);
    PyObject *bases = NULL;
    Py_ssize_t i, n;
    int r = 0;

    while (1) {
        if (selected == cls) {
            r = 1;
            break;
        }
        PyObject *next_bases = abstract_get_bases(selected);
        if (next_bases == NULL) {
            r = PyErr_Occurred() ? -1 : 0;
            break;
        }
        Py_XSETREF(bases, next_bases);
        n = PyTuple_GET_SIZE(bases);
        if (n == 0) {
            break;
        }
        /* Avoid recursivity in the single inheritance case */
        if (n == 1) {
            PyObject *child = _PyValue_AsObjectAt(PyTuple_GET_VALUE(bases, 0), BOX_SITE_TUPLE);
            if (child == NULL) {
                Py_CLEAR(child);
                r = -1;
                break;
            }
            /* The selected child survives retirement of both the previous
             * selected object now and its containing tuple next iteration. */
            Py_SETREF(selected, child);
            continue;
        }
        assert(n >= 2);
        if (_Py_EnterRecursiveCall(" in __issubclass__")) {
            r = -1;
            break;
        }
        for (i = 0; i < n; i++) {
            PyObject *child = _PyValue_AsObjectAt(PyTuple_GET_VALUE(bases, i), BOX_SITE_TUPLE);
            if (child == NULL) {
                Py_CLEAR(child);
                r = -1;
                break;
            }
            r = abstract_issubclass(child, cls);
            Py_DECREF(child);
            if (r != 0) {
                break;
            }
        }
        _Py_LeaveRecursiveCall();
        break;
    }
    Py_CLEAR(selected);
    Py_XDECREF(bases);
    return r;
}

static int
check_class(PyObject *cls, const char *error)
{
    PyObject *bases = abstract_get_bases(cls);
    if (bases == NULL) {
        /* Do not mask errors. */
        PyThreadState *tstate = _PyThreadState_GET();
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_TypeError, error);
        }
        return 0;
    }
    Py_DECREF(bases);
    return -1;
}

static int
object_isinstance(PyObject *inst, PyObject *cls)
{
    PyObject *icls;
    int retval;
    if (PyType_Check(cls)) {
        retval = PyObject_TypeCheck(inst, (PyTypeObject *)cls);
        if (retval == 0) {
            retval = PyObject_GetOptionalAttr(inst, &_Py_ID(__class__), &icls);
            if (icls != NULL) {
                if (icls != (PyObject *)(Py_TYPE(inst)) && PyType_Check(icls)) {
                    retval = PyType_IsSubtype(
                        (PyTypeObject *)icls,
                        (PyTypeObject *)cls);
                }
                else {
                    retval = 0;
                }
                Py_DECREF(icls);
            }
        }
    }
    else {
        if (!check_class(cls,
            "isinstance() arg 2 must be a type, a tuple of types, or a union"))
            return -1;
        retval = PyObject_GetOptionalAttr(inst, &_Py_ID(__class__), &icls);
        if (icls != NULL) {
            retval = abstract_issubclass(icls, cls);
            Py_DECREF(icls);
        }
    }

    return retval;
}

/* Call the special method name of cls with the one argument arg, a value
   that the callee receives without a box. Returns 1 with the owned result
   in *res, 0 if cls has no such method, -1 with an exception set. */
static int
call_special_method(PyThreadState *tstate, PyObject *cls, PyObject *name,
                    const char *where, PyValueBorrow arg, PyValue *res)
{
    *res = PyValue_NULL;
    _PyCValue cref;
    _PyThreadState_PushCValue(tstate, &cref);
    PyValue self = PyValue_DUP(PyValue_FromObjectBorrow(cls));
    int found = _PyObject_LookupSpecialMethod(NULL, name, &cref.ref, &self);
    if (found <= 0) {
        goto done;
    }
    if (_Py_EnterRecursiveCallTstate(tstate, where)) {
        found = -1;
        goto done;
    }
    PyValue stack[2];
    Py_ssize_t n = 0;
    if (!PyValue_IsNull(self)) {
        _PyValue_WindowPut(&stack[n++], PyValue_Borrow(self));
    }
    _PyValue_WindowPut(&stack[n++], arg);
    /* A noncallable scalar is diagnosed by the normal call API. */
    *res = _PyValue_VectorcallTstate(tstate, cref.ref, stack, n, NULL);
    _Py_LeaveRecursiveCallTstate(tstate);
    if (PyValue_IsNull(*res)) {
        found = -1;
    }
done:
    PyValue_XCLOSE(self);
    _PyThreadState_PopCValue(tstate, &cref);
    return found;
}

/* isinstance() of an int or float, whose __class__ is its type. */
static int
object_isinstance_value(PyValueBorrow inst, PyObject *cls)
{
    if (!PyValue_IsInline(inst)) {
        return object_isinstance(PyValue_AsKnownObjectBorrow(inst), cls);
    }
    PyTypeObject *tp = PyValue_TYPE(inst);
    if (PyType_Check(cls)) {
        return PyType_IsSubtype(tp, (PyTypeObject *)cls);
    }
    if (!check_class(cls,
        "isinstance() arg 2 must be a type, a tuple of types, or a union"))
        return -1;
    return abstract_issubclass((PyObject *)tp, cls);
}

static int
object_recursive_isinstance_value(PyThreadState *tstate, PyValueBorrow inst,
                                  PyValueBorrow cls_value)
{
    if (PyValue_IsInline(cls_value)) {
        // An int or float is no class: object_isinstance()'s check_class().
        _PyErr_SetString(tstate, PyExc_TypeError,
            "isinstance() arg 2 must be a type, a tuple of types, "
            "or a union");
        return -1;
    }
    PyObject *cls = PyValue_AsKnownObjectBorrow(cls_value);

    /* Quick test for an exact match */
    if (PyValue_TYPE(inst) == (PyTypeObject *)cls) {
        return 1;
    }

    /* We know what type's __instancecheck__ does. */
    if (PyType_CheckExact(cls)) {
        return object_isinstance_value(inst, cls);
    }

    if (_PyUnion_Check(cls)) {
        cls = _Py_union_args(cls);
    }

    if (PyTuple_Check(cls)) {
        /* Not a general sequence -- that opens up the road to
           recursion and stack overflow. */
        if (_Py_EnterRecursiveCallTstate(tstate, " in __instancecheck__")) {
            return -1;
        }
        Py_ssize_t n = PyTuple_GET_SIZE(cls);
        int r = 0;
        for (Py_ssize_t i = 0; i < n; ++i) {
            PyValueBorrow item = PyTuple_GET_VALUE(cls, i);
            r = object_recursive_isinstance_value(tstate, inst, item);
            if (r != 0) {
                /* either found it, or got an error */
                break;
            }
        }
        _Py_LeaveRecursiveCallTstate(tstate);
        return r;
    }

    PyValue res;
    int found = call_special_method(tstate, cls, &_Py_ID(__instancecheck__),
                                    " in __instancecheck__", inst, &res);
    if (found > 0) {
        int ok = _PyValue_IsTrue(res);
        PyValue_CLOSE(res);
        return ok;
    }
    else if (found < 0) {
        return -1;
    }

    /* cls has no __instancecheck__() method */
    return object_isinstance_value(inst, cls);
}

int
(_PyValue_IsInstance)(PyValueBorrow inst, PyValueBorrow cls)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return object_recursive_isinstance_value(tstate, inst, cls);
}

int
PyObject_IsInstance(PyObject *inst, PyObject *cls)
{
    return _PyValue_IsInstance(PyValue_FromObjectBorrow(inst),
                               PyValue_FromObjectBorrow(cls));
}


static  int
recursive_issubclass(PyObject *derived, PyObject *cls)
{
    if (PyType_Check(cls) && PyType_Check(derived)) {
        /* Fast path (non-recursive) */
        return PyType_IsSubtype((PyTypeObject *)derived, (PyTypeObject *)cls);
    }
    if (!check_class(derived,
                     "issubclass() arg 1 must be a class"))
        return -1;

    if (!_PyUnion_Check(cls) && !check_class(cls,
                            "issubclass() arg 2 must be a class,"
                            " a tuple of classes, or a union")) {
        return -1;
    }

    return abstract_issubclass(derived, cls);
}

static int
object_issubclass(PyThreadState *tstate, PyObject *derived, PyObject *cls)
{
    /* We know what type's __subclasscheck__ does. */
    if (PyType_CheckExact(cls)) {
        /* Quick test for an exact match */
        if (derived == cls)
            return 1;
        return recursive_issubclass(derived, cls);
    }

    if (_PyUnion_Check(cls)) {
        cls = _Py_union_args(cls);
    }

    if (PyTuple_Check(cls)) {

        if (_Py_EnterRecursiveCallTstate(tstate, " in __subclasscheck__")) {
            return -1;
        }
        Py_ssize_t n = PyTuple_GET_SIZE(cls);
        int r = 0;
        for (Py_ssize_t i = 0; i < n; ++i) {
            PyObject *item = _PyValue_AsObjectAt(PyTuple_GET_VALUE(cls, i), BOX_SITE_TUPLE);
            if (item == NULL) {
                Py_CLEAR(item);
                r = -1;
                break;
            }
            r = object_issubclass(tstate, derived, item);
            Py_DECREF(item);
            if (r != 0)
                /* either found it, or got an error */
                break;
        }
        _Py_LeaveRecursiveCallTstate(tstate);
        return r;
    }

    PyValue res;
    int found = call_special_method(tstate, cls, &_Py_ID(__subclasscheck__),
                                    " in __subclasscheck__",
                                    PyValue_FromObjectBorrow(derived), &res);
    if (found > 0) {
        int ok = _PyValue_IsTrue(res);
        PyValue_CLOSE(res);
        return ok;
    }
    else if (found < 0) {
        return -1;
    }

    /* Can be reached when infinite recursion happens. */
    return recursive_issubclass(derived, cls);
}


int
PyObject_IsSubclass(PyObject *derived, PyObject *cls)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return object_issubclass(tstate, derived, cls);
}


int
_PyObject_RealIsInstance(PyObject *inst, PyObject *cls)
{
    return object_isinstance(inst, cls);
}

int
_PyObject_RealIsSubclass(PyObject *derived, PyObject *cls)
{
    return recursive_issubclass(derived, cls);
}


PyObject *
(_PyValue_GetIter)(PyValueBorrow v)
{
    if (PyValue_IsInline(v)) {
        value_type_error("'%.200s' object is not iterable", v);
        return NULL;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    PyTypeObject *t = Py_TYPE(o);
    getiterfunc f;

    f = t->tp_iter;
    if (f == NULL) {
        if (_PyValue_SequenceCheck(PyValue_FromObjectBorrow(o)))
            return PySeqIter_New(o);
        return type_error("'%.200s' object is not iterable", o);
    }
    else {
        PyObject *res = (*f)(o);
        if (res != NULL && !PyIter_Check(res)) {
            PyErr_Format(PyExc_TypeError,
                         "%T.__iter__() must return an iterator, not %T",
                         o, res);
            Py_SETREF(res, NULL);
        }
        return res;
    }
}

PyObject *
PyObject_GetIter(PyObject *o)
{
    return _PyValue_GetIter(_Py_VALUE_ARG(o));
}

PyObject *
PyObject_GetAIter(PyObject *o) {
    PyTypeObject *t = Py_TYPE(o);
    unaryfunc f;

    if (t->tp_as_async == NULL || t->tp_as_async->am_aiter == NULL) {
        return type_error("'%.200s' object is not an async iterable", o);
    }
    f = t->tp_as_async->am_aiter;
    PyObject *it = (*f)(o);
    if (it != NULL && !PyAIter_Check(it)) {
        PyErr_Format(PyExc_TypeError,
                     "%T.__aiter__() must return an async iterator, not %T",
                     o, it);
        Py_SETREF(it, NULL);
    }
    return it;
}

/* A presence test reads the object slot, which an extension iterator
   fills while its value slot is 0 (the adapter); the identity test of the
   pair table's _PyObject_NextNotImplemented reads its value side
   (value-api-m1b-m2.md, section 2.6). */
int
PyIter_Check(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    return (tp->tp_iternext != NULL &&
            _PyType_VALUE_SLOT(tp, iternext) != _PyValue_NextNotImplemented);
}

int
PyAIter_Check(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    return (tp->tp_as_async != NULL &&
            tp->tp_as_async->am_anext != NULL &&
            tp->tp_as_async->am_anext != &_PyObject_NextNotImplemented);
}

_PyStatusValue
_PyValue_IterStep(PyObject *it)
{
    PyTypeObject *t = Py_TYPE(it);
    _Py_iternextvaluefunc f = _PyType_VALUE_SLOT(t, iternext);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(t, _Py_VALUE_SLOT_INDEX_iternext);
        _PyStatusValue r = f(it);
        assert(r.status != PYGEN_NEXT || !PyValue_IsNull(r.value));
        assert(r.status != PYGEN_ERROR || PyValue_IsNull(r.value));
        return r;
    }
    return adapt_iternext(t->tp_iternext, it);
}

/* When the iterator is exhausted it gives RETURN, whose value the
   consumer drops, or raises StopIteration, which is cleared. */
_PyStatusValue
_PyValue_IterNext(PyObject *it)
{
    _PyStatusValue r = _PyValue_IterStep(it);
    if (r.status == PYGEN_NEXT) {
        return r;
    }
    if (r.status == PYGEN_RETURN) {
        PyValue_XCLOSE(r.value);
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
        _PyErr_Clear(tstate);
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    /* Error case: an exception (different than StopIteration) is set. */
    return r;
}

_PyStatusValue
_PyValue_NextNotImplemented(PyObject *it)
{
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object is not iterable",
                 Py_TYPE(it)->tp_name);
    return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
}

PyObject *
(_PyValue_IterNextThunkExit)(_PyStatusValue r _PyBox_AT_PARAM)
{
    if (r.status == PYGEN_NEXT) {
        return _PyValue_ThunkResult(r.value _PyBox_AT_ARG(at));
    }
    if (r.status == PYGEN_RETURN && !PyValue_IsNull(r.value)) {
        (void)_PyGen_SetStopIterationValue(r.value);
        PyValue_CLOSE(r.value);
    }
    return NULL;
}

/* Return 1 and set 'item' to the next item of 'iter' on success.
 * Return 0 and set 'item' to NULL when there are no remaining values.
 * Return -1, set 'item' to NULL and set an exception on error.
 */
Py_VALUE_ENTRY int
PyIter_NextItem(PyObject *iter, PyObject **item)
{
    assert(iter != NULL);
    assert(item != NULL);

    if (Py_TYPE(iter)->tp_iternext == NULL) {
        *item = NULL;
        PyErr_Format(PyExc_TypeError, "expected an iterator, got '%T'", iter);
        return -1;
    }

    return _PyValue_THUNK_EXIT_STATUS(_PyValue_IterNext(iter), item, BOX_SITE_ITERATION);
}

/* Return next item.
 *
 * If an error occurs, return NULL.  PyErr_Occurred() will be true.
 * If the iteration terminates normally, return NULL and clear the
 * PyExc_StopIteration exception (if it was set).  PyErr_Occurred()
 * will be false.
 * Else return the next object.  PyErr_Occurred() will be false.
 */
Py_VALUE_ENTRY PyObject *
PyIter_Next(PyObject *iter)
{
    PyObject *item;
    (void)_PyValue_THUNK_EXIT_STATUS(_PyValue_IterNext(iter), &item, BOX_SITE_ITERATION);
    return item;
}

// The value of a pending StopIteration, or of no exception (None), as RETURN.
static _PyStatusValue
send_fetch_return(void)
{
    PyValue value = _PyGen_FetchStopIterationValue();
    if (PyValue_IsNull(value)) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    return (_PyStatusValue){PYGEN_RETURN, value};
}

_PyStatusValue
(_PyValue_Send)(PyObject *iter, PyValueBorrow arg)
{
    assert(!PyValue_IsNull(arg));
    PyTypeObject *t = Py_TYPE(iter);
    if (t->tp_as_async && t->tp_as_async->am_send) {
        _PyStatusValue r = call_send(iter, t->tp_as_async->am_send, arg);
        assert(_Py_CheckSlotResult(iter, "am_send", r.status != PYGEN_ERROR));
        return r;
    }
    if (PyValue_IsNone(arg) && PyIter_Check(iter)) {
        _PyStatusValue r = _PyValue_IterStep(iter);
        if (r.status == PYGEN_NEXT) {
            return r;
        }
        if (r.status == PYGEN_RETURN) {
            if (PyValue_IsNull(r.value)) {
                r.value = PyValue_None;
            }
            return r;
        }
        return send_fetch_return();
    }
    // .send(arg): the call takes an object until value calls (M3).
    PyObject *a = _PyValue_AsObjectAt(arg, BOX_SITE_ITERATION);
    if (a == NULL) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    PyObject *res = PyObject_CallMethodOneArg(iter, &_Py_ID(send), a);
    Py_DECREF(a);
    if (res != NULL) {
        return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(res)};
    }
    return send_fetch_return();
}

Py_VALUE_ENTRY PySendResult
PyIter_Send(PyObject *iter, PyObject *arg, PyObject **result)
{
    assert(arg != NULL);
    assert(result != NULL);
    return (PySendResult)_PyValue_THUNK_EXIT_STATUS(
        _PyValue_Send(iter, _Py_VALUE_ARG(arg)), result, BOX_SITE_ITERATION);
}
