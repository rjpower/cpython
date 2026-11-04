#include "Python.h"

#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_value.h"
#include "pycore_value_api.h"     // _PyValue_SwapBoxSite()

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
PyValue_PrepareOwned(PyObject *obj)
{
    if (obj == NULL) {
        return PyValue_NULL;
    }
    return _PyValue_Canonicalize(PyValue_FromPyObjectSteal(obj));
}

PyValue
_PyValue_Canonicalize(PyValue source)
{
    if (!PyValue_IsObject(source)) {
        return source;
    }
    PyObject *obj = PyValue_AsKnownObjectBorrow(source);
    uintptr_t identity = _PyObject_ValueIdentity(obj);
    if (identity < Py_DOUBLE_OFFSET) {
        return source;
    }
    if (PyValue_RefcountOnObject(source)) {
        /* The release contract: freeing an exact int or float box runs no
           Python and leaves the current exception unchanged. */
        _PyReleaseCheck_BEGIN(check);
        Py_DECREF(obj);
        _PyReleaseCheck_END(check, NULL);
    }
    return (PyValue){.bits = identity};
}

PyObject *
_PyValueTemps_Box(_PyValueTemps *temps, PyValue value)
{
    PyObject *box = PyValue_AsObject(value);
    if (box == NULL) {
        return NULL;
    }
    assert(temps != NULL && temps->count < temps->capacity);
    temps->objects[temps->count++] = box;
    return box;
}

/* The boxes are exact ints and floats: their release runs no Python and
 * leaves the current exception unchanged (the release contract). */
void
_PyValueTemps_CloseNonempty(_PyValueTemps *temps)
{
    while (temps->count != 0) {
        PyObject *obj = temps->objects[--temps->count];
        assert(PyLong_CheckExact(obj) || PyFloat_CheckExact(obj));
        Py_DECREF(obj);
    }
}

// The tag bits (Py_TAG_BITS) of an object pointer must be clear.
static_assert(_Alignof(PyObject) >= 8, "PyObject must be 8-byte aligned");

#ifdef Py_STATS
/* The site class of the current thread's boxes, set by the At forms of
   pycore_value_api.h for the duration of a boxing call. */
static _Py_thread_local int box_site = BOX_SITE_UNATTRIBUTED;

int
_PyValue_SwapBoxSite(int site)
{
    assert(0 <= site && site < BOX_SITE_KINDS);
    int previous = box_site;
    box_site = site;
    return previous;
}

int
_PyValue_BoxSiteOr(int fallback)
{
    return box_site != BOX_SITE_UNATTRIBUTED ? box_site : fallback;
}
#endif

// Out of line, so that the test before each call is all that hot paths carry.
Py_NO_INLINE PyValue
_PyValue_BoxInlineValue(PyValue ref)
{
    assert(PyValue_IsInline(ref));
    OBJECT_STAT_INC(box[box_site]);
    PyObject *box;
    if (PyValue_IsInlineInt(ref)) {
        OBJECT_STAT_INC(inline_int_boxes);
        box = PyLong_FromLong(PyValue_InlineInt(ref));
    }
    else {
        OBJECT_STAT_INC(inline_float_boxes);
        box = PyFloat_FromDouble(PyValue_InlineFloat(ref));
    }
    if (box == NULL) {
        return ref;
    }
    return PyValue_FromPyObjectSteal(box);
}

Py_NO_INLINE void _Py_NO_RETURN
_PyValue_FatalNotObject(PyValue ref)
{
    _Py_FatalErrorFormat(__func__,
                         "PyValue_AsKnownObject*() of a value that holds no "
                         "object: %#" PRIx64, (uint64_t)ref.bits);
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
#define HOME_SITE_MAX 256

/* The kinds of violation, which the report names. */
enum {
    HOME_INT_BOX,         // an exact int that fits inline, held as a pointer
    HOME_FLOAT_BOX,       // an exact non-NaN float held as a pointer
    HOME_STATIC_TAG0,     // a static immortal without the no-count tag
    HOME_NOT_VALUE,       // an index, ERROR or a malformed pattern
    HOME_KINDS
};

static const char *const home_kind_names[HOME_KINDS] = {
    "int-box", "float-box", "static-tag0", "not-value",
};

/* One table per recheck flag and kind. A slot is claimed by a
   compare-exchange on its site, so two threads (per-interpreter GILs)
   cannot interleave a claim. */
static struct {
    const char *site;
    Py_ssize_t count;
} home_sites[2][HOME_KINDS][HOME_SITE_MAX];
static Py_ssize_t home_total;
// Violations from call sites that did not fit in their table.
static Py_ssize_t home_dropped;

static int
home_kind(PyValue ref)
{
    if (!PyValue_IsLanguageValue(ref)) {
        return HOME_NOT_VALUE;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    if ((ref.bits & Py_TAG_BITS) == 0 && _Py_IsStaticImmortal(obj)) {
        return HOME_STATIC_TAG0;
    }
    return PyFloat_CheckExact(obj) ? HOME_FLOAT_BOX : HOME_INT_BOX;
}

void
_PyValue_ReportHome(PyValue ref, const char *site, int recheck)
{
    if (!recheck) {
        _Py_atomic_add_ssize(&home_total, 1);
    }
    recheck = recheck != 0;
    int kind = home_kind(ref);
    for (int i = 0; i < HOME_SITE_MAX; i++) {
        const char *claimed = _Py_atomic_load_ptr(&home_sites[recheck][kind][i].site);
        if (claimed == NULL) {
            void *expected = NULL;
            if (_Py_atomic_compare_exchange_ptr(&home_sites[recheck][kind][i].site,
                                                &expected, (void *)site)) {
                claimed = site;
            }
            else {
                claimed = expected;
            }
        }
        if (claimed == site || strcmp(claimed, site) == 0) {
            _Py_atomic_add_ssize(&home_sites[recheck][kind][i].count, 1);
            return;
        }
    }
    _Py_atomic_add_ssize(&home_dropped, 1);
}

Py_ssize_t
_PyValue_HomeViolationCount(void)
{
    return _Py_atomic_load_ssize(&home_total);
}

static int
add_home_sites(PyObject *sites, int recheck, int kind)
{
    for (int i = 0; i < HOME_SITE_MAX; i++) {
        const char *site = _Py_atomic_load_ptr(&home_sites[recheck][kind][i].site);
        if (site == NULL) {
            return 0;
        }
        PyObject *key = PyUnicode_FromFormat("%s%s %s",
                                             recheck ? "recheck " : "", site,
                                             home_kind_names[kind]);
        PyObject *count = PyLong_FromSsize_t(
            _Py_atomic_load_ssize(&home_sites[recheck][kind][i].count));
        if (key == NULL || count == NULL
            || PyDict_SetItem(sites, key, count) < 0)
        {
            Py_XDECREF(key);
            Py_XDECREF(count);
            return -1;
        }
        Py_DECREF(key);
        Py_DECREF(count);
    }
    return 0;
}

PyObject *
_PyValue_HomeViolationSites(void)
{
    PyObject *sites = PyDict_New();
    if (sites == NULL) {
        return NULL;
    }
    for (int recheck = 0; recheck < 2; recheck++) {
        for (int kind = 0; kind < HOME_KINDS; kind++) {
            if (add_home_sites(sites, recheck, kind) < 0) {
                Py_DECREF(sites);
                return NULL;
            }
        }
    }
    Py_ssize_t dropped = _Py_atomic_load_ssize(&home_dropped);
    if (dropped != 0) {
        PyObject *count = PyLong_FromSsize_t(dropped);
        if (count == NULL
            || PyDict_SetItemString(sites, "<dropped: table full>", count) < 0)
        {
            Py_XDECREF(count);
            Py_DECREF(sites);
            return NULL;
        }
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
