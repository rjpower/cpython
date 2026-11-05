#ifndef PY_INTERNAL_TYPECACHE_H
#define PY_INTERNAL_TYPECACHE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"


#define _Py_TYPECACHE_MINSIZE (1 << 3)
#define _Py_TYPECACHE_MAXSIZE (1 << 16)

/* A value word that the cache holds without a count. Its owner is the type
   (its dict or a base's), and the type's version tag guards it: the word is
   read back as a borrow only after the version check, while the owner is
   known to be alive (_PyTypeCache_Lookup()). It is not a PyValue, because
   it owns nothing, and not a PyValueBorrow, because it outlives the C call
   that made it (pyjit/design/proposals/register-ir-v2-a5b.md, section 2.4).
   Private to the type cache. */
typedef struct { uintptr_t bits; } _PyValueGuarded;

static inline _PyValueGuarded
_PyValueGuarded_FromBorrow(PyValueBorrow value)
{
    _PyValueGuarded guarded = {value.bits};
    return guarded;
}

static inline PyValueBorrow
_PyValueGuarded_Load(_PyValueGuarded guarded)
{
    PyValueBorrow value = {guarded.bits};
    return value;
}

struct type_cache_entry {
    PyObject *name;     // name of the attribute or method, interned string, NULL if the entry is empty
    _PyValueGuarded value;  // canonical value, PyValue_NULL's word for a miss
};

// Per-type attribute lookup cache: speed up attribute and method lookups,
// see _PyTypeCache_Lookup().
struct type_cache {
    uint32_t mask;                                              // mask for indexing into hashtable, i.e. size of hashtable is mask + 1
    uint32_t available;                                         // number of available entries in hashtable
    uint32_t used;                                              // number of used entries in hashtable
    unsigned int version_tag;                                   // initialized from type->tp_version_tag
    struct type_cache_entry hashtable[_Py_TYPECACHE_MINSIZE];   // hashtable entries
};

struct _PyTypeCacheLookupResult {
    PyValue value;      // owned hit result; PyValue_NULL for a negative hit or miss
    int cache_hit;          // 1 if the cache entry is valid and matches the type's version tag, 0 otherwise
    unsigned int version_tag;   // version tag of the type when the value was cached
};


extern void _PyTypeCache_InitType(PyTypeObject *type);
/* type, name and the canonical semantic result stay independently owned by
   the caller through allocation and cleanup. Insertion never steals them and
   checks the same expected nonzero version again after maintenance callouts. */
extern void _PyTypeCache_Insert(PyTypeObject *type, PyObject *name,
                                PyValueBorrow value, unsigned int expected_version);
#define _PyTypeCache_Insert(T, N, V, E) \
    (_PyTypeCache_Insert)(T, N, PyValue_Borrow(V), E)
PyAPI_FUNC(struct _PyTypeCacheLookupResult) _PyTypeCache_Lookup(PyTypeObject *type, PyObject *name);
/* Detach publishes an empty table and transfers a private retirement owner.
   Retire consumes it after watcher notification; both are legal during teardown. */
extern struct type_cache *_PyTypeCache_Detach(PyTypeObject *type);
extern void _PyTypeCache_Retire(struct type_cache *cache);
PyAPI_FUNC(void) _PyTypeCache_Invalidate(PyTypeObject *type);

#ifdef __cplusplus
}
#endif
#endif /* PY_INTERNAL_TYPECACHE_H */
