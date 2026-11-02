// Per-type canonical lookup cache. Names are owned interned strings; values
// borrow their semantic source and may only be promoted after version admission.

#include "Python.h"
#include "pycore_typecache.h"
#include "pycore_fileutils.h"     // _Py_write_noraise()
#include "pycore_interp.h"
#include "pycore_pymem.h"
#include "pycore_pystate.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_typeobject.h"
#include "pycore_stats.h"

#include <stdlib.h>              // abort()

#define EMPTY_ENTRY { .name = NULL, .value = { .bits = PyValue_NULL_BITS } }
// No entry of the shared empty sentinel is read: available and used are zero.
static struct type_cache empty_cache = {
    .mask = _Py_TYPECACHE_MINSIZE - 1,
    .version_tag = 0,
    .available = 0,
    .used = 0,
    .hashtable = {EMPTY_ENTRY, EMPTY_ENTRY, EMPTY_ENTRY, EMPTY_ENTRY,
                  EMPTY_ENTRY, EMPTY_ENTRY, EMPTY_ENTRY, EMPTY_ENTRY},
};
#undef EMPTY_ENTRY

void _Py_NO_RETURN
_PyType_AbortClosedLookup(void)
{
    static const char message[] =
        "Fatal Python error: type lookup after type finalization\n";
    (void)_Py_write_noraise(2, message, sizeof(message) - 1);
    abort();
}

static inline uint32_t
cache_size(struct type_cache *cache)
{
    return cache->mask + 1;
}

static inline size_t
cache_nbytes(struct type_cache *cache)
{
    return offsetof(struct type_cache, hashtable)
        + (size_t)cache_size(cache) * sizeof(struct type_cache_entry);
}

static struct type_cache *
cache_allocate(uint32_t size)
{
    assert((size & (size - 1)) == 0);
    size_t nbytes = offsetof(struct type_cache, hashtable)
        + (size_t)size * sizeof(struct type_cache_entry);
    struct type_cache *cache = PyMem_Calloc(1, nbytes);
    if (cache == NULL) {
        return NULL;
    }
    cache->mask = size - 1;
    cache->available = size - (size >> 2);
    for (uint32_t i = 0; i < size; i++) {
        // Zero bits are not the canonical null value.
        cache->hashtable[i].value = PyValue_NULL;
    }
    return cache;
}

// The table is detached and privately owned. Publish each empty slot before
// retiring its name; callbacks cannot rediscover or retire this table again.
static void
cache_free_delayed(struct type_cache *cache)
{
    if (cache == NULL || cache == &empty_cache) {
        return;
    }
    size_t nbytes = cache_nbytes(cache);
    uint32_t size = cache_size(cache);
    for (uint32_t i = 0; i < size; i++) {
        PyObject *name = cache->hashtable[i].name;
        cache->hashtable[i].name = NULL;
        cache->hashtable[i].value = PyValue_NULL;
        PyObject *saved = PyErr_GetRaisedException();
        Py_XDECREF(name);
        _PyValue_RestoreException(saved);
    }
    PyObject *saved = PyErr_GetRaisedException();
    _PyMem_FreeDelayed(cache, nbytes);
    _PyValue_RestoreException(saved);
}

static inline void **
cache_slot(PyTypeObject *type)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, type);
        assert(state != NULL && state->type == type);
        return &state->_tp_cache;
    }
    return &type->_tp_cache;
}

static inline struct type_cache *
cache_get(PyTypeObject *type)
{
    return (struct type_cache *)FT_ATOMIC_LOAD_PTR_ACQUIRE(*cache_slot(type));
}

static inline void
cache_set(PyTypeObject *type, struct type_cache *cache)
{
    FT_ATOMIC_STORE_PTR_RELEASE(*cache_slot(type), cache);
}

void
_PyTypeCache_InitType(PyTypeObject *type)
{
    *cache_slot(type) = &empty_cache;
}

static void
cache_insert(struct type_cache *cache, PyObject *name, PyValue value)
{
    Py_hash_t hash = PyUnstable_Unicode_GET_CACHED_HASH(name);
    assert(hash != -1);
    uint32_t index = hash & cache->mask;
    for (;;) {
        if (cache->hashtable[index].name == NULL) {
            Py_INCREF(name);
            cache->hashtable[index].value = PyValue_Borrow(value);
            FT_ATOMIC_STORE_PTR_RELEASE(cache->hashtable[index].name, name);
            cache->used++;
            cache->available--;
            return;
        }
        if (cache->hashtable[index].name == name) {
            return;
        }
        index = (index + 1) & cache->mask;
    }
}

// Never carry the installed table across allocation. The requested size is
// only a hint: reentrant work may have replaced, filled or invalidated it.
static void
cache_resize(PyInterpreterState *interp, PyTypeObject *type,
             uint32_t size, unsigned int expected)
{
    struct type_cache *replacement = cache_allocate(size);
    if (replacement == NULL) {
        return;
    }
    if (PyErr_Occurred() || !_PyType_MatchesVersion(interp, type, expected)) {
        cache_free_delayed(replacement);
        return;
    }
    struct type_cache *current = cache_get(type);
    int valid = current != NULL && current != &empty_cache &&
        current->version_tag == expected;
    if (valid && (current->available != 0 ||
                  current->used >= size - (size >> 2))) {
        cache_free_delayed(replacement);
        return;
    }
    // From admission to publication, copying only increments names and cannot
    // call out. A stale table's borrowed values are never read.
    if (valid) {
        for (uint32_t i = 0; i < cache_size(current); i++) {
            if (current->hashtable[i].name != NULL) {
                cache_insert(replacement, current->hashtable[i].name,
                             current->hashtable[i].value);
            }
        }
    }
    replacement->version_tag = expected;
    cache_set(type, replacement);
    OBJECT_STAT_INC(type_cache_resizes);
    // Neither pointer may be reused after retirement callbacks.
    cache_free_delayed(current);
}

// Best-effort maintenance must not export its own failures. Discarded error
// owners can themselves call out, so drain them before restoring the entry error.
static void
cache_restore_entry_error(PyObject *saved)
{
    while (PyErr_Occurred()) {
        PyObject *error = PyErr_GetRaisedException();
        Py_DECREF(error);
    }
    PyErr_SetRaisedException(saved);
}

void
_PyTypeCache_Insert(PyTypeObject *type, PyObject *name, PyValue value,
                    unsigned int expected_version)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!_PyType_MatchesVersion(interp, type, expected_version)) {
        return;
    }
    PyObject *saved = PyErr_GetRaisedException();
    struct type_cache *cache = cache_get(type);
    if (cache == NULL || cache == &empty_cache ||
        cache->version_tag != expected_version || cache->available == 0) {
        uint32_t size = _Py_TYPECACHE_MINSIZE;
        if (cache != NULL && cache != &empty_cache &&
            cache->version_tag == expected_version) {
            size = cache_size(cache) * 2;
        }
        // No installed-table pointer survives the allocator or free hooks.
        cache = NULL;
        if (size > _Py_TYPECACHE_MAXSIZE) {
            OBJECT_STAT_INC(type_cache_too_big);
            goto done;
        }
        cache_resize(interp, type, size, expected_version);
        if (PyErr_Occurred() ||
            !_PyType_MatchesVersion(interp, type, expected_version)) {
            goto done;
        }
        cache = cache_get(type);
    }
    if (cache != NULL && cache != &empty_cache && cache->available != 0 &&
        cache->version_tag == expected_version) {
        cache_insert(cache, name, value);
    }
done:
    cache_restore_entry_error(saved);
}

struct _PyTypeCacheLookupResult
_PyTypeCache_Lookup(PyTypeObject *type, PyObject *name)
{
    assert(PyUnicode_CheckExact(name) && PyUnicode_CHECK_INTERNED(name));
    struct _PyTypeCacheLookupResult miss = {PyValue_NULL, 0, 0};
    PyInterpreterState *interp = _PyInterpreterState_GET();
    unsigned int version = _PyType_GetVersionForCurrentState(interp, type);
    if (version == 0) {
        return miss;
    }
    struct type_cache *cache = cache_get(type);
    if (cache == NULL || cache == &empty_cache || cache->version_tag != version) {
        return miss;
    }
    Py_hash_t hash = PyUnstable_Unicode_GET_CACHED_HASH(name);
    assert(hash != -1);
    uint32_t index = hash & cache->mask;
    for (;;) {
        PyObject *entry_name = cache->hashtable[index].name;
        if (entry_name == name) {
            // Match before touching a potentially dead borrowed value.
            // Promotion, including borrowed pointers, is nonallocating.
            PyValue value = cache->hashtable[index].value;
            PyValue owner = PyValue_IsNull(value) ? PyValue_NULL : PyValue_NewRef(value);
            return (struct _PyTypeCacheLookupResult){owner, 1, version};
        }
        if (entry_name == NULL) {
            return miss;
        }
        index = (index + 1) & cache->mask;
    }
}

void
_PyTypeCache_Invalidate(PyTypeObject *type)
{
    OBJECT_STAT_INC(type_cache_invalidations);
    struct type_cache *cache = cache_get(type);
    cache_set(type, &empty_cache);
    cache_free_delayed(cache);
}
