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

// No entry of the shared empty sentinel is read: available and used are zero.
// Its entries are zero, which is an empty name and PyValue_NULL.
static struct type_cache empty_cache = {
    .mask = _Py_TYPECACHE_MINSIZE - 1,
    .version_tag = 0,
    .available = 0,
    .used = 0,
};

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
    return cache;
}

// The cache owns strong references to its interned names. Releasing an
// interned string runs no Python, so nothing can reach the table meanwhile.
static void
cache_free_delayed(struct type_cache *cache)
{
    if (cache == NULL || cache == &empty_cache) {
        return;
    }
    for (uint32_t i = 0; i < cache_size(cache); i++) {
        Py_XDECREF(cache->hashtable[i].name);
    }
    // Delay the freeing of old cache for concurrent lock-free readers
    _PyMem_FreeDelayed(cache, cache_nbytes(cache));
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

// Allocation, copying and retiring the old table run no Python (the release
// contract), so the table installed when the caller checked is still installed.
static void
cache_resize(PyTypeObject *type, uint32_t size, unsigned int expected)
{
    struct type_cache *replacement = cache_allocate(size);
    if (replacement == NULL) {
        return;
    }
    struct type_cache *current = cache_get(type);
    // Copying only increments names. A stale table's borrowed values are
    // never read.
    if (current != NULL && current != &empty_cache &&
        current->version_tag == expected) {
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
    cache_free_delayed(current);
}

void
_PyTypeCache_Insert(PyTypeObject *type, PyObject *name, PyValue value,
                    unsigned int expected_version)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!_PyType_MatchesVersion(interp, type, expected_version)) {
        return;
    }
    struct type_cache *cache = cache_get(type);
    if (cache == NULL || cache == &empty_cache ||
        cache->version_tag != expected_version || cache->available == 0) {
        uint32_t size = _Py_TYPECACHE_MINSIZE;
        if (cache != NULL && cache != &empty_cache &&
            cache->version_tag == expected_version) {
            size = cache_size(cache) * 2;
        }
        if (size > _Py_TYPECACHE_MAXSIZE) {
            OBJECT_STAT_INC(type_cache_too_big);
            return;
        }
        cache_resize(type, size, expected_version);
        cache = cache_get(type);
        if (cache == NULL || cache->version_tag != expected_version ||
            cache->available == 0) {
            // out of memory, don't cache the value
            return;
        }
    }
    cache_insert(cache, name, value);
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

struct type_cache *
_PyTypeCache_Detach(PyTypeObject *type)
{
    OBJECT_STAT_INC(type_cache_invalidations);
    struct type_cache *cache = cache_get(type);
    cache_set(type, &empty_cache);
    return cache;
}

void
_PyTypeCache_Retire(struct type_cache *cache)
{
    cache_free_delayed(cache);
}

void
_PyTypeCache_Invalidate(PyTypeObject *type)
{
    _PyTypeCache_Retire(_PyTypeCache_Detach(type));
}
