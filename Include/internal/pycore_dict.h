#ifndef Py_INTERNAL_DICT_H
#define Py_INTERNAL_DICT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object.h"               // PyManagedDictPointer
#include "pycore_shape.h"                // _PyObject_SetShape()
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_SSIZE_ACQUIRE
#include "pycore_value.h"                // PyValue
#include "pycore_dict_state.h"           // BUILTINS_WATCHER_ID
#include "pycore_stats.h"

// Unsafe flavor of PyDict_GetItemWithError(): no error checking
extern PyObject* _PyDict_GetItemWithError(PyObject *dp, PyObject *key);

// Delete an item from a dict if a predicate is true
// Returns -1 on error, 1 if the item was deleted, 0 otherwise
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_DelItemIf(PyObject *mp, PyObject *key,
                                  int (*predicate)(PyObject *value, void *arg),
                                  void *arg);

// Atomically replace an existing value if it is expected (by identity).
// Return 1 if replaced, 0 if absent or different, or -1 on error.
extern int _PyDict_ReplaceItemIf(PyObject *dict, PyObject *key,
                               PyObject *expected, PyObject *replacement);

// "KnownHash" variants
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key,
                                          PyObject *item, Py_hash_t hash);
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_DelItem_KnownHash(PyObject *mp, PyObject *key,
                                          Py_hash_t hash);

// Exported for external JIT support
PyAPI_FUNC(int) _PyDict_DelItem_KnownHash_LockHeld(PyObject *mp, PyObject *key,
                                              Py_hash_t hash);

extern int _PyDict_Contains_KnownHash(PyObject *, PyValue, Py_hash_t);

extern void _PyDict_ClearKeysVersionLockHeld(PyObject *mp);

extern int _PyDict_Next(
    PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value, Py_hash_t *hash);
// _PyDict_Next() with the key and the value as they are, without boxes.
extern int _PyDict_NextValue(
    PyObject *mp, Py_ssize_t *pos, PyValue *key, PyValue *value, Py_hash_t *hash);

extern int _PyDict_HasOnlyStringKeys(PyObject *mp);

PyAPI_FUNC(PyObject *) _PyDict_Subscript(PyObject *self, PyObject *key);
PyAPI_FUNC(PyObject *) _PyDict_SubscriptKnownHash(PyObject *self, PyObject *key, Py_hash_t hash);
PyAPI_FUNC(int) _PyDict_StoreSubscript(PyObject *self, PyObject *key, PyObject *value);

// Export for '_ctypes' shared extension
PyAPI_FUNC(Py_ssize_t) _PyDict_SizeOf(PyDictObject *);

extern Py_ssize_t _PyDict_SizeOf_LockHeld(PyDictObject *);

/* Like PyDict_Merge, but override can be 0, 1 or 2.  If override is 0,
   the first occurrence of a key wins, if override is 1, the last occurrence
   of a key wins, if override is 2, a KeyError with conflicting key as
   argument is raised.
*/
PyAPI_FUNC(int) _PyDict_MergeUniq(PyObject *mp, PyObject *other, PyObject **dupkey);



/* _PyDictView */

typedef struct {
    PyObject_HEAD
    PyDictObject *dv_dict;
} _PyDictViewObject;

extern PyObject* _PyDictView_New(PyObject *, PyTypeObject *);
extern PyObject* _PyDictView_Intersect(PyObject* self, PyObject *other);

/* other API */

/* Keys and values are canonical PyValues (PyValue_NULL when absent), except
   the key of a Unicode entry, which is always a str object. */
typedef struct {
    /* Cached hash code of me_key. */
    Py_hash_t me_hash;
    PyValue me_key;
    PyValue me_value; /* This field is only meaningful for combined tables */
} PyDictKeyEntry;

typedef struct {
    PyObject *me_key;   /* The key must be Unicode and have hash. */
    PyValue me_value;   /* This field is only meaningful for combined tables */
} PyDictUnicodeEntry;

extern PyObject *_PyDict_FromKeys(PyObject *, PyObject *, PyObject *);

/* Implementations of the `|` and `|=` operators for dict, used by the
 * specializing interpreter. */
extern PyObject *_PyDict_Or(PyObject *self, PyObject *other);
extern PyObject *_PyDict_IOr(PyObject *self, PyObject *other);

/* Gets a version number unique to the current state of the keys of dict, if possible.
 * Returns the version number, or zero if it was not possible to get a version number.
 * Exported for external JIT support */
PyAPI_FUNC(uint32_t) _PyDictKeys_GetVersionForCurrentState(
        PyInterpreterState *interp, PyDictKeysObject *dictkeys);

/* Gets a version number unique to the current state of the keys of dict, if possible.
 *
 * In free-threaded builds ensures that the dict can be used for lock-free
 * reads if a version was assigned.
 *
 * The caller must hold the per-object lock on dict.
 *
 * Returns the version number, or zero if it was not possible to get a version number.
 * Exported for external JIT support */
PyAPI_FUNC(uint32_t) _PyDict_GetKeysVersionForCurrentState(
        PyInterpreterState *interp, PyDictObject *dict);

extern size_t _PyDict_KeysSize(PyDictKeysObject *keys);

extern void _PyDictKeys_DecRef(PyDictKeysObject *keys);

/* _Py_dict_lookup() returns index of entry which can be used like DK_ENTRIES(dk)[index].
 * -1 when no entry found, -3 when compare raises error.
 * Exported for external JIT support
 */
PyAPI_FUNC(Py_ssize_t) _Py_dict_lookup(PyDictObject *mp, PyValue key, Py_hash_t hash, PyValue *value_addr);

extern int _PyDict_GetMethodValue(PyDictObject *dict, PyObject *name, PyValue *method);

// Exported for external JIT support
PyAPI_FUNC(Py_ssize_t) _PyDict_LookupIndexAndValue(PyDictObject *, PyObject *, PyValue *);
PyAPI_FUNC(Py_ssize_t) _PyDict_LookupIndex(PyDictObject *, PyObject *);
PyAPI_FUNC(Py_ssize_t) _PyDictKeys_StringLookup(PyDictKeysObject* dictkeys, PyObject *key);

/* Look up a string key in an all unicode dict keys, assign the keys object a version, and
 * store it in version.
 *
 * Returns DKIX_ERROR if key is not a string or if the keys object is not all
 * strings.
 *
 * Returns DKIX_EMPTY if the key is not present.
 *
 * Exported for external JIT support
 */
PyAPI_FUNC(Py_ssize_t) _PyDictKeys_StringLookupAndVersion(PyDictKeysObject* dictkeys, PyObject *key, uint32_t *version);
// Return the borrowed source dictionary, or NULL if absent or on error.
PyAPI_FUNC(PyObject *) _PyDict_LoadGlobalValue(PyDictObject *, PyDictObject *, PyObject *, PyValue *);

// Loads the __builtins__ object from the globals dict. Returns a new reference.
extern PyObject *_PyDict_LoadBuiltinsFromGlobals(PyObject *globals);

/* Set the item of the canonical key to the canonical value */
PyAPI_FUNC(int) _PyDict_SetItemValue(PyDictObject *op, PyValue key, PyValue value);
PyAPI_FUNC(int) _PyDict_GetItemValue(PyObject *op, PyValue key, PyValue *result);
PyAPI_FUNC(int) _PyDict_DelItemValue(PyObject *op, PyValue key);
PyAPI_FUNC(int) _PyDict_ContainsValue(PyObject *op, PyValue key);
PyAPI_FUNC(PyValue) _PyDict_SubscriptValue(PyObject *self, PyValue key);
// Exported for external JIT support
// Sets dict[name] to value, or deletes it if value is PyValue_NULL.
PyAPI_FUNC(int) _PyDict_SetItem_LockHeld(PyDictObject *dict, PyObject *name, PyValue value);
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash_LockHeld(PyDictObject *mp, PyObject *key,
                                                   PyObject *value, Py_hash_t hash);
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_GetItemRef_KnownHash_LockHeld(PyDictObject *op, PyObject *key, Py_hash_t hash, PyObject **result);
PyAPI_FUNC(int) _PyDict_GetItemRef_KnownHash(PyDictObject *op, PyObject *key, Py_hash_t hash, PyObject **result);
extern int _PyDict_GetItemRef_Unicode_LockHeld(PyDictObject *op, PyObject *key, PyObject **result);
PyAPI_FUNC(int) _PyObjectDict_SetItem(PyTypeObject *tp, PyObject *obj, PyObject **dictptr, PyObject *name, PyValue value);

extern int _PyDict_Pop_KnownHash(
    PyDictObject *dict,
    PyObject *key,
    Py_hash_t hash,
    PyObject **result);

extern void _PyDict_Clear_LockHeld(PyObject *op);


// Export for '_elementtree' shared extension
PyAPI_FUNC(PyObject*) _PyDict_CopyAsDict(PyObject *op);

#define DKIX_EMPTY (-1)
#define DKIX_DUMMY (-2)  /* Used internally */
#define DKIX_ERROR (-3)
#define DKIX_KEY_CHANGED (-4) /* Used internally */

typedef enum {
    DICT_KEYS_GENERAL = 0,
    DICT_KEYS_UNICODE = 1,
} DictKeysKind;

/* See dictobject.c for actual layout of DictKeysObject */
struct _dictkeysobject {
    Py_ssize_t dk_refcnt;

    /* Size of the hash table (dk_indices). It must be a power of 2. */
    uint8_t dk_log2_size;

    /* Size of the hash table (dk_indices) by bytes. */
    uint8_t dk_log2_index_bytes;

    /* Kind of keys */
    uint8_t dk_kind;


    /* Version number -- Reset to 0 by any modification to keys */
    uint32_t dk_version;

    /* Number of usable entries in dk_entries. */
    Py_ssize_t dk_usable;

    /* Number of used entries in dk_entries. */
    Py_ssize_t dk_nentries;


    /* Actual hash table of dk_size entries. It holds indices in dk_entries,
       or DKIX_EMPTY(-1) or DKIX_DUMMY(-2).

       Indices must be: 0 <= indice < USABLE_FRACTION(dk_size).

       The size in bytes of an indice depends on dk_size:

       - 1 byte if dk_size <= 0xff (char*)
       - 2 bytes if dk_size <= 0xffff (int16_t*)
       - 4 bytes if dk_size <= 0xffffffff (int32_t*)
       - 8 bytes otherwise (int64_t*)

       Dynamically sized, SIZEOF_VOID_P is minimum. */
    char dk_indices[];  /* char is required to avoid strict aliasing. */

    /* "PyDictKeyEntry or PyDictUnicodeEntry dk_entries[USABLE_FRACTION(DK_SIZE(dk))];" array follows:
       see the DK_ENTRIES() / DK_UNICODE_ENTRIES() functions below */
};

#define DK_LOG_SIZE(dk)  _Py_RVALUE((dk)->dk_log2_size)
#if SIZEOF_VOID_P > 4
#define DK_SIZE(dk)      (((int64_t)1)<<DK_LOG_SIZE(dk))
#else
#define DK_SIZE(dk)      (1<<DK_LOG_SIZE(dk))
#endif

static inline void* _DK_ENTRIES(PyDictKeysObject *dk) {
    int8_t *indices = (int8_t*)(dk->dk_indices);
    size_t index = (size_t)1 << dk->dk_log2_index_bytes;
    return (&indices[index]);
}

static inline PyDictKeyEntry* DK_ENTRIES(PyDictKeysObject *dk) {
    assert(dk->dk_kind == DICT_KEYS_GENERAL);
    return (PyDictKeyEntry*)_DK_ENTRIES(dk);
}
static inline PyDictUnicodeEntry* DK_UNICODE_ENTRIES(PyDictKeysObject *dk) {
    assert(dk->dk_kind != DICT_KEYS_GENERAL);
    return (PyDictUnicodeEntry*)_DK_ENTRIES(dk);
}

#define DK_IS_UNICODE(dk) ((dk)->dk_kind != DICT_KEYS_GENERAL)

#define DICT_VERSION_INCREMENT (1 << (DICT_MAX_WATCHERS + DICT_WATCHED_MUTATION_BITS))
#define DICT_WATCHER_MASK ((1 << DICT_MAX_WATCHERS) - 1)
#define DICT_WATCHER_AND_MODIFICATION_MASK ((1 << (DICT_MAX_WATCHERS + DICT_WATCHED_MUTATION_BITS)) - 1)
#define DICT_UNIQUE_ID_SHIFT (32)
#define DICT_UNIQUE_ID_MAX ((UINT64_C(1) << (64 - DICT_UNIQUE_ID_SHIFT)) - 1)

/* CPython's own watchers (pycore_dict_state.h) get the key and the value as
   they are.  The others get them as objects: an inline one is boxed for
   them.  Either may be PyValue_NULL. */
PyAPI_FUNC(void)
_PyDict_SendEvent(int watcher_bits,
                  PyDict_WatchEvent event,
                  PyDictObject *mp,
                  PyValue key,
                  PyValue value);

static inline void
_PyDict_NotifyEvent(PyDict_WatchEvent event,
                    PyDictObject *mp,
                    PyValue key,
                    PyValue value)
{
    assert(Py_REFCNT((PyObject*)mp) > 0);
    int watcher_bits = FT_ATOMIC_LOAD_UINT64_ACQUIRE(mp->_ma_watcher_tag) & DICT_WATCHER_MASK;
    if (watcher_bits) {
        RARE_EVENT_STAT_INC(watched_dict_modification);
        _PyDict_SendEvent(watcher_bits, event, mp, key, value);
    }
}


PyAPI_FUNC(PyObject *)_PyDict_FromItems(
        PyObject *const *keys, Py_ssize_t keys_offset,
        const PyValue *values, Py_ssize_t values_offset,
        Py_ssize_t length);

// A dict with room for MINUSED str keys, whose table is the str-keyed kind
// that the attribute handlers take (_PyDict_NewPresized() makes the general
// kind above five).
extern PyObject *_PyDict_NewPresizedUnicode(Py_ssize_t minused);


// Enables per-thread ref counting on this dict in the free threading build
extern void _PyDict_EnablePerThreadRefcounting(PyObject *op);

PyDictObject *_PyObject_MaterializeManagedDict_LockHeld(PyObject *);

// See `_Py_INCREF_TYPE()` in pycore_object.h
#  define _Py_INCREF_DICT Py_INCREF
#  define _Py_DECREF_DICT Py_DECREF
#  define _Py_INCREF_BUILTINS Py_INCREF
#  define _Py_DECREF_BUILTINS Py_DECREF

/* frozendict */
typedef struct {
    PyDictObject ob_base;
    Py_hash_t ma_hash;
} PyFrozenDictObject;

#define _PyFrozenDictObject_CAST(op) \
    (assert(PyFrozenDict_Check(op)), _Py_CAST(PyFrozenDictObject*, (op)))

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DICT_H */
