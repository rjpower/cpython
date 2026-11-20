// Per-type canonical lookup cache. Names are owned interned strings; values
// borrow their semantic source and may only be promoted after version admission.

#include "Python.h"
#include "pycore_typecache.h"
#include "pycore_interp.h"
#include "pycore_pymem.h"
#include "pycore_pystate.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_typeobject.h"
#include "pycore_stats.h"

// No entry of the shared empty sentinel is read: available and used are zero.
// Its entries are zero, which is an empty name and PyValue_NULL.
static struct type_cache empty_cache = {
    .mask = _Py_TYPECACHE_MINSIZE - 1,
    .version_tag = 0,
    .available = 0,
    .used = 0,
};

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
cache_insert(struct type_cache *cache, PyObject *name, _PyValueGuarded value)
{
    Py_hash_t hash = PyUnstable_Unicode_GET_CACHED_HASH(name);
    assert(hash != -1);
    uint32_t index = hash & cache->mask;
    for (;;) {
        if (cache->hashtable[index].name == NULL) {
            Py_INCREF(name);
            cache->hashtable[index].value = value;
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
(_PyTypeCache_Insert)(PyTypeObject *type, PyObject *name, PyValueBorrow value,
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
    cache_insert(cache, name, _PyValueGuarded_FromBorrow(value));
}

/* The lookup path reads the version tag and nothing else; it does not ask the
   type whether it is admitted. A nonzero cache->version_tag is stored only by
   _PyTypeCache_Insert(), which admits the type (typeobject.c,
   type_cache_admitted()) and installs the table for exactly the tag the type
   has. A cache hit therefore needs only cache->version_tag == the type's
   current tp_version_tag, nonzero. The rest of the proof is in typeobject.c
   at "Version tags and the lookup cache".

   A static builtin type shares its tag between interpreters but not its
   table, so the table is read from this interpreter's state. A state that
   does not belong to the type means the type is not ready here yet (or no
   longer): there is no table. */
static inline struct type_cache *
cache_get_for_lookup(PyTypeObject *type)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        managed_static_type_state *state =
            _PyStaticType_GetState(_PyInterpreterState_GET(), type);
        if (state->type != type) {
            return NULL;
        }
        return (struct type_cache *)FT_ATOMIC_LOAD_PTR_ACQUIRE(state->_tp_cache);
    }
    return (struct type_cache *)FT_ATOMIC_LOAD_PTR_ACQUIRE(type->_tp_cache);
}

unsigned int
_PyTypeCache_Lookup(PyTypeObject *type, PyObject *name, PyValueBorrow *out)
{
    assert(PyUnicode_CheckExact(name) && PyUnicode_CHECK_INTERNED(name));
    unsigned int version = FT_ATOMIC_LOAD_UINT_RELAXED(type->tp_version_tag);
    if (version == 0) {
        return 0;
    }
    struct type_cache *cache = cache_get_for_lookup(type);
    if (cache == NULL || cache->version_tag != version) {
        return 0;
    }
    Py_hash_t hash = PyUnstable_Unicode_GET_CACHED_HASH(name);
    assert(hash != -1);
    uint32_t index = hash & cache->mask;
    for (;;) {
        PyObject *entry_name = cache->hashtable[index].name;
        if (entry_name == name) {
            // Match before touching a potentially dead borrowed value.
            *out = _PyValueGuarded_Load(cache->hashtable[index].value);
            return version;
        }
        if (entry_name == NULL) {
            return 0;
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
