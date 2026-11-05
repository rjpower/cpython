/* The int and float object extractors are compatibility thunks of their
   value rows; this unit calls none of them (value-api-v2a1a.md, D12). */
#define Py_VALUE_MARK_NUMBERS 1

#include "Python.h"

#include "pycore_dict.h"          // PyDict_SetValue(), _PyDict_NextBorrowed()
#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
#include "pycore_initconfig.h"    // _Py_GetEnv()
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _Py_GetConfig()
#include "pycore_pystats.h"     // _PyStats_BoxOwner()
#include "pycore_value.h"
#include "pycore_value_api.h"     // _PyValue_FromObjectKnown()

PyValue
PyValue_PrepareOwned(PyObject *obj)
{
    if (obj == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(obj);
}

PyValue
_PyValue_FromNumberSteal(PyObject *obj)
{
    assert(PyLong_CheckExact(obj) || PyFloat_CheckExact(obj));
    uintptr_t identity = _PyObject_ValueIdentity(obj);
    if (identity < Py_DOUBLE_OFFSET) {
        return _PyValue_PointerSteal(obj);
    }
    /* The release contract: freeing an exact int or float box runs no
       Python and leaves the current exception unchanged. */
    _PyReleaseCheck_BEGIN(check);
    Py_DECREF(obj);
    _PyReleaseCheck_END(check, NULL);
    return (PyValue){.bits = identity};
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
(_PyValueTemps_Box)(_PyValueTemps *temps, PyValueBorrow value _PyBox_AT_PARAM)
{
    PyObject *box = (PyValue_AsObject)(PyValue_Borrow(value) _PyBox_AT_ARG(at));
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

// Out of line, so that the test before each call is all that hot paths carry.
Py_NO_INLINE PyValue
(_PyValue_BoxInlineValue)(PyValueBorrow ref _PyBox_AT_PARAM)
{
    assert(PyValue_IsInline(ref));
#ifdef Py_STATS
    assert(at.site < BOX_SITE_KINDS && at.kind < BOX_KINDS);
    OBJECT_STAT_INC(box[at.site]);
    OBJECT_STAT_INC(box_kind[at.kind]);
    if (_PyStats_GET() != NULL) {
        _PyStats_BoxOwner(at.site, at.kind, at.owner, _Py_BOX_CALLER());
    }
#endif
    PyObject *box;
    if (PyValue_IsInlineInt(ref)) {
        OBJECT_STAT_INC(inline_int_boxes);
        box = PyLong_FromLongLong(PyValue_InlineInt64(ref));
#ifdef Py_STATS
        if (-_PY_NSMALLNEGINTS <= PyValue_InlineInt64(ref)
            && PyValue_InlineInt64(ref) < _PY_NSMALLPOSINTS)
        {
            OBJECT_STAT_INC(box_cached);
        }
#endif
    }
    else {
        OBJECT_STAT_INC(inline_float_boxes);
        box = PyFloat_FromDouble(PyValue_InlineFloat(ref));
    }
    if (box == NULL) {
        return (PyValue){ref.bits};  // an inline value owns nothing
    }
    return _PyValue_PointerSteal(box);
}

Py_NO_INLINE void _Py_NO_RETURN
(_PyValue_FatalNotObject)(PyValueBorrow ref)
{
    _Py_FatalErrorFormat(__func__,
                         "PyValue_AsKnownObject*() of a value that holds no "
                         "object: %#" PRIx64, (uint64_t)ref.bits);
}

uintptr_t
_PyObject_ValueIdentity(PyObject *op)
{
    if (PyLong_CheckExact(op)) {
        if (_PyLong_IsCompact((PyLongObject *)op)) {
            return _PyValue_IntBits(_PyLong_CompactValue((PyLongObject *)op));
        }
        /* The entry primitive calls only typed kernels: the span reader,
           which never raises. */
        int overflow;
        int64_t value = _PyLong_SpanAsInt64AndOverflow(
            _PyLong_SpanOf((PyLongObject *)op), &overflow);
        if (!overflow && _PyValue_IntFitsInline(value)) {
            return _PyValue_IntBits(value);
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
    assert(!_PyValue_IntFitsInline(value));
    PyObject *op = PyLong_FromLongLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return _PyValue_PointerSteal(op);
}

PyValue
_PyValue_BoxUInt64(uint64_t value)
{
    assert(value > (uint64_t)INT64_MAX);
    PyObject *op = PyLong_FromUnsignedLongLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return _PyValue_PointerSteal(op);
}

PyValue
_PyValue_BoxDouble(double value)
{
    assert(isnan(value));
    PyObject *op = PyFloat_FromDouble(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(op);
}

int
_PyValue_ClearingImmortals(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    // Without a thread state, assume the worst: the check stays quiet.
    return tstate == NULL || tstate->interp->clearing_immortals;
}

#ifdef Py_DEBUG
#define HOME_SITE_MAX 256

/* The kinds of violation, which the report names. */
enum {
    HOME_INT_BOX,         // an exact int that fits inline, held as a pointer
    HOME_FLOAT_BOX,       // an exact non-NaN float held as a pointer
    HOME_STATIC_TAG0,     // a static immortal without the no-count tag
    HOME_NOT_VALUE,       // an index or a malformed pattern
    HOME_TAGGED_MORTAL,   // a mortal with the no-count tag (H1)
    HOME_KINDS
};

static const char *const home_kind_names[HOME_KINDS] = {
    "int-box", "float-box", "static-tag0", "not-value", "tagged-mortal",
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
home_kind(PyValueBorrow ref)
{
    if (!PyValue_IsLanguageValue(ref)) {
        return HOME_NOT_VALUE;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    if ((ref.bits & Py_TAG_BITS) == 0 && _Py_IsStaticImmortal(obj)) {
        return HOME_STATIC_TAG0;
    }
    if (_PyObject_ValueIdentity(obj) == (uintptr_t)obj) {
        // Canonical, so H1 failed: a tagged mortal
        return HOME_TAGGED_MORTAL;
    }
    return PyFloat_CheckExact(obj) ? HOME_FLOAT_BOX : HOME_INT_BOX;
}

/* Is _PYTHON_VALUE_HOME_REPORT set? Read once, at the first violation, as
   the configuration reads its environment variables (-E applies). */
static int
home_report_only(void)
{
    static int report_only = -1;
    int value = _Py_atomic_load_int_relaxed(&report_only);
    if (value < 0) {
        value = _Py_GetEnv(_Py_GetConfig()->use_environment,
                           "_PYTHON_VALUE_HOME_REPORT") != NULL;
        _Py_atomic_store_int_relaxed(&report_only, value);
    }
    return value;
}

void
_PyValue_ReportHome(PyValueBorrow ref, const char *site, int recheck)
{
    int kind = home_kind(ref);
    if (!home_report_only()) {
        _Py_FatalErrorFormat(__func__,
                             "%s at %s: a non-canonical value (%s), bits %#" PRIx64,
                             recheck ? "recheck" : "store", site,
                             home_kind_names[kind], (uint64_t)ref.bits);
    }
    if (!recheck) {
        _Py_atomic_add_ssize(&home_total, 1);
    }
    recheck = recheck != 0;
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
            || PyDict_SetValue(sites, PyValue_FromObjectBorrow(key), PyValue_FromObjectBorrow(count)) < 0)
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
            || PyDict_SetValueString(sites, "<dropped: table full>", PyValue_FromObjectBorrow(count)) < 0)
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

/* Call-scratch window primitives (pyjit/design/proposals/value-api-m3.md,
   section 1.1). A window holds words with no count, so only this file
   writes the bits of one (the dict primitive owns its words). */

void
_PyValue_WindowFromObjects(PyValue *dst, PyObject *const *src, Py_ssize_t n)
{
    for (Py_ssize_t i = 0; i < n; i++) {
        dst[i].bits = PyValue_FromObjectBorrow(src[i]).bits;
    }
}

void
_PyValue_WindowCopy(PyValue *dst, const PyValue *src, Py_ssize_t n)
{
    memcpy(dst, src, n * sizeof(PyValue));
}

Py_ssize_t
_PyValue_WindowFromDictValues(PyValue *dst, PyObject *dict, PyObject **kwnames)
{
    assert(PyDict_Check(dict));
    *kwnames = NULL;
    Py_ssize_t n = PyDict_GET_SIZE(dict);
    PyObject *names = PyTuple_New(n);
    if (names == NULL) {
        return -1;
    }
    Py_ssize_t pos = 0;
    PyValueBorrow key, value;
    Py_ssize_t i = 0;
    while (_PyDict_NextBorrowed(dict, &pos, &key, &value)) {
        if (!PyValue_IsObject(key)
            || !PyUnicode_Check(PyValue_AsKnownObjectBorrow(key)))
        {
            PyErr_SetString(PyExc_TypeError, "keywords must be strings");
            goto error;
        }
        assert(i < n);
        PyTuple_SET_VALUE(names, i, PyValue_DUP(key));
        dst[i] = PyValue_DUP(value);
        i++;
    }
    assert(i == n);
    *kwnames = names;
    return n;
error:
    while (i > 0) {
        PyValue_CLOSE(dst[--i]);
    }
    Py_DECREF(names);
    return -1;
}

Py_hash_t
(_PyValue_IdentityHash)(PyValueBorrow v)
{
    void *identity;
    if (PyValue_IsNull(v)) {
        identity = NULL;
    }
    else if (PyValue_IsInline(v)) {
        identity = (void *)v.bits;
    }
    else {
        identity = (void *)_PyObject_ValueIdentity(
            PyValue_AsKnownObjectBorrow(v));
    }
    return Py_HashPointer(identity);
}
