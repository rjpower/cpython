#ifndef Py_INTERNAL_DICT_STATE_H
#define Py_INTERNAL_DICT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define DICT_MAX_WATCHERS 8
#define DICT_WATCHED_MUTATION_BITS 4

/* The first three dict watcher IDs are reserved for CPython,
 * so we don't need to check that they haven't been used */
#define BUILTINS_WATCHER_ID     0
#define GLOBALS_WATCHER_ID      1
#define MODULE_WATCHER_ID       2
#define FIRST_AVAILABLE_WATCHER 3
#define DICT_RESERVED_WATCHERS_MASK ((1 << FIRST_AVAILABLE_WATCHER) - 1)

/* A callback of CPython's own watchers: it takes the key and the new value
   as they are in the dict, so an event boxes nothing for it.  Either may be
   PyValue_NULL. */
typedef int (*_PyDict_CoreWatchCallback)(PyDict_WatchEvent event,
                                         PyObject *dict,
                                         PyValue key,
                                         PyValue new_value);

struct _Py_dict_state {
    uint32_t next_keys_version;
    PyMutex watcher_mutex;          // Protects the watchers array (free-threaded builds)
    // The reserved IDs' entries are always NULL: their callbacks are in
    // core_watchers.
    PyDict_WatchCallback watchers[DICT_MAX_WATCHERS];
    _PyDict_CoreWatchCallback core_watchers[FIRST_AVAILABLE_WATCHER];
};

#define _dict_state_INIT \
    { \
        .next_keys_version = 2, \
    }


#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DICT_STATE_H */
