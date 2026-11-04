#include "Python.h"

#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_value.h"

PyValue
PyValue_FromObjectBorrow(PyObject *obj)
{
    if (obj == NULL) {
        return PyValue_NULL;
    }
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    uintptr_t identity = _PyObject_ValueIdentity(obj);
    if (identity >= Py_DOUBLE_OFFSET) {
        return (PyValue){.bits = identity};
    }
    return PyValue_FromPyObjectBorrow(obj);
}

PyValue
PyValue_PrepareOwned(PyObject *obj, PyObject **retired)
{
    assert(retired != NULL);
    if (obj == NULL) {
        *retired = NULL;
        return PyValue_NULL;
    }
    PyValue value = PyValue_FromObjectBorrow(obj);
    if (PyValue_IsInline(value)) {
        *retired = obj;
        return value;
    }
    *retired = NULL;
    return PyValue_FromPyObjectSteal(obj);
}

PyObject *
PyValue_AsObject(PyValue value)
{
    if (!PyValue_IsLanguageValue(value)) {
        PyErr_SetString(PyExc_SystemError, "internal PyValue is not a Python object");
        return NULL;
    }
    PyObject *object = PyValue_AsPyObjectNewBoxed(value);
    if (object != NULL && PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(object);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return object;
}

/* The source may be an owned object result or a borrowed frame input. The
 * returned canonical value has the same ownership; only an owned primitive
 * box becomes a separate release obligation. This operation cannot reenter. */
PyValue
_PyValue_Prepare(PyValue source, PyObject **retired)
{
    *retired = NULL;
    if (!PyValue_IsPointerSpace(source) || (source.bits & Py_TAG_INVALID) ||
        PyValue_IsNull(source)) {
        return source;
    }
    PyObject *obj = PyValue_AsPyObjectBorrow(source);
    PyValue value = PyValue_FromObjectBorrow(obj);
    if (!PyValue_IsInline(value)) {
        return source;
    }
    if (PyValue_RefcountOnObject(source)) {
        *retired = obj;
    }
    return value;
}

PyValue
_PyValueTemps_PublishObject(_PyValueTemps *temps, PyValue *source)
{
    PyObject *retired;
    PyValue value = _PyValue_Prepare(*source, &retired);
    if (retired != NULL) {
        assert(temps != NULL && temps->count < temps->capacity);
        temps->objects[temps->count++] = retired;
        /* The C view may still be used after an early publication. The
         * scoped bridge owns its box until the handler has finished. */
        *source = PyValue_Borrow(*source);
    }
    return value;
}

PyObject *
_PyValueTemps_Box(_PyValueTemps *temps, PyValue value)
{
    PyObject *box = PyValue_AsObject(value);
    if (box == NULL) {
        return NULL;
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_DECREF(box);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    assert(temps != NULL && temps->count < temps->capacity);
    temps->objects[temps->count++] = box;
    return box;
}

int
_PyValueTemps_CloseNonempty(_PyValueTemps *temps)
{
    assert(temps == NULL || temps->pending_local == NULL);
    if (temps == NULL || temps->count == 0) {
        return PyErr_Occurred() ? -1 : 0;
    }
    PyObject *saved = PyErr_GetRaisedException();
    while (temps->count != 0) {
        PyObject *obj = temps->objects[--temps->count];
        /* Detach each owner before a callback-capable allocator release. */
        temps->objects[temps->count] = NULL;
        Py_DECREF(obj);
        PyObject *raised = PyErr_GetRaisedException();
        if (raised != NULL) {
            if (saved == NULL) {
                saved = raised;
            }
            else {
                PyErr_SetRaisedException(raised);
                _PyValue_RestoreException(saved);
                saved = PyErr_GetRaisedException();
            }
        }
    }
    PyErr_SetRaisedException(saved);
    return PyErr_Occurred() ? -1 : 0;
}

void
_PyValue_RestoreException(PyObject *saved)
{
    PyObject *cleanup_error = PyErr_GetRaisedException();
    if (saved == NULL) {
        PyErr_SetRaisedException(cleanup_error);
        return;
    }
    if (cleanup_error != NULL) {
        /* Do not replace an existing context or create a cycle if a hook
         * raises a reused exception object. Walk without allocating. */
        PyObject *slow = cleanup_error;
        PyObject *fast = cleanup_error;
        int safe = ((PyBaseExceptionObject *)saved)->context == NULL;
        while (safe && fast != NULL) {
            if (slow == saved || fast == saved) {
                safe = 0;
                break;
            }
            slow = ((PyBaseExceptionObject *)slow)->context;
            fast = ((PyBaseExceptionObject *)fast)->context;
            if (fast == saved) {
                safe = 0;
                break;
            }
            if (fast != NULL) {
                fast = ((PyBaseExceptionObject *)fast)->context;
            }
            if (fast != NULL && fast == slow) {
                safe = 0;
            }
        }
        if (safe) {
            PyException_SetContext(saved, cleanup_error);
        }
        else {
            Py_DECREF(cleanup_error);
        }
    }
    PyErr_SetRaisedException(saved);
}


// The tag bits (Py_TAG_BITS) of an object pointer must be clear.
static_assert(_Alignof(PyObject) >= 8, "PyObject must be 8-byte aligned");

// Out of line, so that the test before each call is all that hot paths carry.
Py_NO_INLINE PyValue
_PyValue_BoxInlineValue(PyValue ref)
{
    assert(PyValue_IsInline(ref));
    PyObject *box;
    if (PyValue_IsInlineInt(ref)) {
        OBJECT_STAT_INC(inline_int_boxes);
        box = PyLong_FromLong(PyValue_InlineInt(ref));
    }
    else {
        OBJECT_STAT_INC(inline_float_boxes);
        box = PyFloat_FromDouble(PyValue_InlineFloat(ref));
    }
    if (box == NULL || PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(box);
        _PyValue_RestoreException(saved);
        return ref;
    }
    return PyValue_FromPyObjectSteal(box);
}

// The identity of a reference that is not NULL or ERROR.
static uintptr_t
value_identity(PyValue ref)
{
    if (!PyValue_IsPointerSpace(ref)) {
        return _PyValue_BITS(ref);
    }
    return _PyObject_ValueIdentity(BITS_TO_PTR_MASKED(ref));
}

int
_PyValue_IsSameValue(PyValue a, PyValue b)
{
    assert(PyValue_IsValid(a) && PyValue_IsValid(b));
    return value_identity(a) == value_identity(b);
}

uintptr_t
_PyObject_ValueIdentity(PyObject *op)
{
    if (PyLong_CheckExact(op)) {
        if (_PyLong_IsCompact((PyLongObject *)op)) {
            return _PyValue_Int32Bits((int32_t)_PyLong_CompactValue((PyLongObject *)op));
        }
        // Exact ints do not raise here.
        int overflow;
        long value = PyLong_AsLongAndOverflow(op, &overflow);
        if (!overflow && _PyValue_IntFitsInline(value)) {
            return _PyValue_Int32Bits((int32_t)value);
        }
    }
    else if (PyFloat_CheckExact(op) && !isnan(PyFloat_AS_DOUBLE(op))) {
        return _PyValue_DoubleBits(PyFloat_AS_DOUBLE(op));
    }
    return (uintptr_t)op;
}

PyValue
_PyValue_BoxInt64(int64_t value)
{
    PyObject *op = PyLong_FromLongLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(op);
}

PyValue
_PyValue_LshiftInt32(int32_t value, int32_t shift)
{
    assert(shift >= 0);
    PyObject *op = PyLong_FromLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = _PyLong_Lshift(op, shift);
    Py_DECREF(op);
    if (res == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(res);
}

PyValue
_PyValue_BoxDouble(double value)
{
    PyObject *op = PyFloat_FromDouble(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(op);
}

#ifdef Py_DEBUG
#define HOME_SITE_MAX 64

static struct {
    const char *file;
    int line;
    Py_ssize_t count;
} home_sites[HOME_SITE_MAX];
static Py_ssize_t home_total;

void
_PyValue_ReportHome(PyValue ref, const char *file, int line)
{
    _Py_atomic_add_ssize(&home_total, 1);
    for (int i = 0; i < HOME_SITE_MAX; i++) {
        if (home_sites[i].file == NULL) {
            home_sites[i].file = file;
            home_sites[i].line = line;
        }
        if (strcmp(home_sites[i].file, file) == 0 && home_sites[i].line == line) {
            _Py_atomic_add_ssize(&home_sites[i].count, 1);
            return;
        }
    }
}

Py_ssize_t
_PyValue_HomeViolationCount(void)
{
    return _Py_atomic_load_ssize(&home_total);
}

PyObject *
_PyValue_HomeViolationSites(void)
{
    PyObject *sites = PyDict_New();
    if (sites == NULL) {
        return NULL;
    }
    for (int i = 0; i < HOME_SITE_MAX && home_sites[i].file != NULL; i++) {
        PyObject *key = PyUnicode_FromFormat("%s:%d", home_sites[i].file,
                                             home_sites[i].line);
        PyObject *count = PyLong_FromSsize_t(home_sites[i].count);
        if (key == NULL || count == NULL
            || PyDict_SetItem(sites, key, count) < 0)
        {
            Py_XDECREF(key);
            Py_XDECREF(count);
            Py_DECREF(sites);
            return NULL;
        }
        Py_DECREF(key);
        Py_DECREF(count);
    }
    return sites;
}
#else
Py_ssize_t
_PyValue_HomeViolationCount(void)
{
    return 0;
}

PyObject *
_PyValue_HomeViolationSites(void)
{
    return PyDict_New();
}
#endif
