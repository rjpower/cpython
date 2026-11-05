#include "Python.h"

#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
#include "pycore_initconfig.h"    // _Py_GetEnv()
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _Py_GetConfig()
#include "pycore_value.h"
#include "pycore_value_api.h"     // _PyValue_SwapBoxSite()

PyValue
PyValue_PrepareOwned(PyObject *obj)
{
    if (obj == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(obj);
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
(_PyValueTemps_Box)(_PyValueTemps *temps, PyValueBorrow value)
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
(_PyValue_BoxInlineValue)(PyValueBorrow ref)
{
    assert(PyValue_IsInline(ref));
    OBJECT_STAT_INC(box[box_site]);
    PyObject *box;
    if (PyValue_IsInlineInt(ref)) {
        OBJECT_STAT_INC(inline_int_boxes);
        box = PyLong_FromLongLong(PyValue_InlineInt64(ref));
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
        // Exact ints do not raise here.
        int overflow;
        long long value = PyLong_AsLongLongAndOverflow(op, &overflow);
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

/* The operands are exact ints in the inline range, so the multiplication
   runs no Python; only the allocation can fail. */
PyValue
_PyValue_MultiplyInt64(int64_t a, int64_t b)
{
    assert(_PyValue_IntFitsInline(a) && _PyValue_IntFitsInline(b));
    int64_t product;
    if (!__builtin_mul_overflow(a, b, &product)) {
        return PyValue_FromInt64(product);
    }
    PyObject *x = PyLong_FromLongLong(a);
    if (x == NULL) {
        return PyValue_NULL;
    }
    PyObject *y = PyLong_FromLongLong(b);
    if (y == NULL) {
        Py_DECREF(x);
        return PyValue_NULL;
    }
    PyObject *res = PyNumber_Multiply(x, y);
    Py_DECREF(y);
    Py_DECREF(x);
    if (res == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(res);
}

PyValue
_PyValue_LshiftInt64(int64_t value, int64_t shift)
{
    assert(value != 0 && shift >= 0);
    PyObject *op = PyLong_FromLongLong(value);
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
    assert(isnan(value));
    PyObject *op = PyFloat_FromDouble(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromPyObjectSteal(op);
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
