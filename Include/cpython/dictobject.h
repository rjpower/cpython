#ifndef Py_CPYTHON_DICTOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct _dictkeysobject PyDictKeysObject;
typedef struct _dictvalues PyDictValues;

/* The ma_values pointer is NULL for a combined table
 * or points to an array of owning canonical PyValue entries for a split table
 */
typedef struct {
    PyObject_HEAD

    /* Number of items in the dictionary */
    Py_ssize_t ma_used;

    /* This is a private field for CPython's internal use.
     * Bits 0-7 are for dict watchers.
     * Bits 8-11 are for the watched mutation counter
     * Bits 12-31 are currently unused
     * Bits 32-63 are a unique id in the free threading build (used for per-thread refcounting)
     */
    uint64_t _ma_watcher_tag;

    PyDictKeysObject *ma_keys;

    /* If ma_values is NULL, the table is "combined": keys and values
       are stored in ma_keys.

       If ma_values is not NULL, the table is split:
       keys are stored in ma_keys and values are stored in ma_values */
    PyDictValues *ma_values;
} PyDictObject;

// frozendict
PyAPI_DATA(PyTypeObject) PyFrozenDict_Type;
#define PyFrozenDict_Check(op) PyObject_TypeCheck((op), &PyFrozenDict_Type)
#define PyFrozenDict_CheckExact(op) Py_IS_TYPE((op), &PyFrozenDict_Type)

#define PyAnyDict_CheckExact(ob) \
    (PyDict_CheckExact(ob) || PyFrozenDict_CheckExact(ob))
#define PyAnyDict_Check(ob) \
    (PyDict_Check(ob) || PyFrozenDict_Check(ob))

/* Get the number of items of a dictionary. */
static inline Py_ssize_t PyDict_GET_SIZE(PyObject *op) {
    PyDictObject *mp;
    assert(PyAnyDict_Check(op));
    mp = _Py_CAST(PyDictObject*, op);
#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_ssize_relaxed(&mp->ma_used);
#else
    return mp->ma_used;
#endif
}
#define PyDict_GET_SIZE(op) PyDict_GET_SIZE(_PyObject_CAST(op))

PyAPI_FUNC(int) PyDict_ContainsString(PyObject *mp, const char *key);

PyAPI_FUNC(PyObject *) _PyDict_NewPresized(Py_ssize_t minused);

Py_VALUE_COMPAT(PyDict_PopValue)
PyAPI_FUNC(int) PyDict_Pop(PyObject *dict, PyObject *key, PyObject **result);
Py_VALUE_COMPAT(PyDict_PopValueString)
PyAPI_FUNC(int) PyDict_PopString(PyObject *dict, const char *key, PyObject **result);

// Use PyDict_Pop() instead
Py_DEPRECATED(3.14) PyAPI_FUNC(PyObject *) _PyDict_Pop(
    PyObject *dict,
    PyObject *key,
    PyObject *default_value);

/* Dictionary watchers */

#define PY_FOREACH_DICT_EVENT(V) \
    V(ADDED)                     \
    V(MODIFIED)                  \
    V(DELETED)                   \
    V(CLONED)                    \
    V(CLEARED)                   \
    V(DEALLOCATED)

typedef enum {
    #define _Py_DEF_EVENT(EVENT) PyDict_EVENT_##EVENT,
    PY_FOREACH_DICT_EVENT(_Py_DEF_EVENT)
    #undef _Py_DEF_EVENT
} PyDict_WatchEvent;

// Callback to be invoked when a watched dict is cleared, dealloced, or modified.
// In clear/dealloc case, key and new_value will be NULL. Otherwise, new_value will be the
// new value for key, NULL if key is being deleted.
/* key/new_value borrow strong snapshots for the callback duration.
 * PyValue_NULL marks an absent event field. CLONED passes the source dict
 * as key. The dict metadata argument retains its existing lifetime rules.
 * This is the one public declaration that names PyValueBorrow, which R26
 * allows as a callback's parameter type (pyjit/design/value-api.md,
 * section 1.6). The operations that read a borrow are internal, so only a
 * callback built with the core (Py_BUILD_CORE) can use these arguments. */
typedef int(*PyDict_WatchCallback)(PyDict_WatchEvent event, PyObject* dict,
                                 PyValueBorrow key, PyValueBorrow new_value);

// Register/unregister a dict-watcher callback
PyAPI_FUNC(int) PyDict_AddWatcher(PyDict_WatchCallback callback);
PyAPI_FUNC(int) PyDict_ClearWatcher(int watcher_id);

// Mark given dictionary as "watched" (callback will be called if it is modified)
PyAPI_FUNC(int) PyDict_Watch(int watcher_id, PyObject* dict);
PyAPI_FUNC(int) PyDict_Unwatch(int watcher_id, PyObject* dict);

// Create a frozendict. Create an empty dictionary if iterable is NULL.
PyAPI_FUNC(PyObject*) PyFrozenDict_New(PyObject *iterable);

/* Canonical reads: outputs own independent snapshots.
 * Return 1/hit, 0/miss or end, -1/error. Initialize outputs to PyValue_NULL;
 * preserve an exception pending on entry without running lookup callbacks.
 * GetValueString requires valid result storage and a NUL-terminated UTF-8
 * key. Close owned results.
 * NextValue requires valid cursor storage and distinct optional key/value
 * outputs. Error/end leave the cursor unchanged; success advances it once.
 * The forms that take a PyValueBorrow key (GetValue, SetValue, DelValue,
 * ContainsValue, PopValue, SetValueString) are internal (pycore_dict.h): the operations that make,
 * read or retain a borrow are internal, so no public function takes or
 * returns one. */
PyAPI_FUNC(int) PyDict_GetValueString(PyObject *, const char *, PyValue *);
PyAPI_FUNC(int) PyDict_NextValue(PyObject *, Py_ssize_t *, PyValue *, PyValue *);
/* As PyDict_PopString(): 1 with the removed value as an owner in *result
 * (which may be NULL), 0 if absent, -1 with an exception. */
PyAPI_FUNC(int) PyDict_PopValueString(PyObject *, const char *, PyValue *);
