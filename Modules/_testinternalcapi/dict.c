#include "parts.h"

#include "pycore_dict.h"
#include "pycore_tuple.h"
#include "pycore_value.h"


static PyObject *
dict_value_seed(PyObject *Py_UNUSED(module), PyObject *key)
{
    PyValue owned_key = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(key));
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        PyValue_CLOSE(owned_key);
        return NULL;
    }
    if (_PyDict_SetValue_Take2((PyDictObject *)dict, owned_key,
                              PyValue_FromInt32(INT32_MAX)) < 0) {
        Py_XDECREF(dict);
        return NULL;
    }
    return dict;
}

typedef struct {
    PyMemAllocatorEx saved;
    int called;
} DictBoxFailure;

static void *
dict_fail_malloc(void *context, size_t size)
{
    DictBoxFailure *failure = context;
    failure->called++;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure->saved);
    return NULL;
}

static void *
dict_fail_calloc(void *context, size_t count, size_t size)
{
    return dict_fail_malloc(context, 0);
}

static void *
dict_fail_realloc(void *context, void *pointer, size_t size)
{
    return dict_fail_malloc(context, 0);
}

static void
dict_fail_free(void *context, void *pointer)
{
    DictBoxFailure *failure = context;
    failure->saved.free(failure->saved.ctx, pointer);
}

static PyObject *
dict_value_pop_boxing_oom(PyObject *module, PyObject *key)
{
    if (!PyUnicode_CheckExact(key)) {
        PyErr_SetString(PyExc_TypeError, "expected an exact Unicode query");
        return NULL;
    }
    PyObject *dict = dict_value_seed(module, key);
    if (dict == NULL) {
        return NULL;
    }
    PyValue query = PyValue_MakeHeapSafe(PyValue_FromObjectBorrow(key));
    Py_hash_t hash = PyObject_Hash(key);
    if (hash == -1) {
        PyValue_CLOSE(query);
        Py_XDECREF(dict);
        return NULL;
    }
    DictBoxFailure failure = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    PyMemAllocatorEx failing = {
        .ctx = &failure, .malloc = dict_fail_malloc,
        .calloc = dict_fail_calloc, .realloc = dict_fail_realloc,
        .free = dict_fail_free,
    };
    PyObject *result = NULL;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failing);
    int status = PyDict_Pop(dict, key, &result);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    int valid = failure.called == 1 && status == -1 && result == NULL &&
                PyErr_ExceptionMatches(PyExc_MemoryError);
    PyErr_Clear();
    Py_XDECREF(result);
    PyValue found = PyValue_NULL;
    status = _PyDict_GetValue_KnownHash(dict, query, hash, &found);
    valid = valid && status == 1 && PyValue_IsInlineInt(found) &&
            PyValue_InlineInt(found) == INT32_MAX && PyDict_GET_SIZE(dict) == 1;
    PyValue_XCLOSE(found);
    result = NULL;
    if (valid) {
        status = PyDict_Pop(dict, key, &result);
        valid = status == 1 && result != NULL &&
                PyLong_AsLong(result) == INT32_MAX && PyDict_GET_SIZE(dict) == 0;
    }
    Py_XDECREF(result);
    PyValue_CLOSE(query);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError,
                            "dict pop boxing OOM consumed or lost the selected item");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
dict_always_delete(PyObject *value, void *context)
{
    return 1;
}

static PyObject *
dict_value_delete_if(PyObject *module, PyObject *args)
{
    if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected (dict, query)");
        return NULL;
    }
    PyObject *dict = PyTuple_GetItemRef(args, 0);
    PyObject *key = dict == NULL ? NULL : PyTuple_GetItemRef(args, 1);
    int status = -1;
    if (dict != NULL && key != NULL) {
        if (!PyDict_Check(dict)) {
            PyErr_SetString(PyExc_TypeError, "expected dict");
        }
        else {
            status = _PyDict_DelItemIf(dict, key, dict_always_delete, NULL);
        }
    }
    Py_XDECREF(key);
    Py_XDECREF(dict);
    return status < 0 || PyErr_Occurred() ? NULL : PyLong_FromLong(status);
}

/* Fail the first PYMEM_DOMAIN_MEM allocation of size bytes, then restore
 * the saved allocator. */
typedef struct {
    PyMemAllocatorEx allocator;
    size_t size;
    int called;
} DictStorageFailure;

static void *
dict_storage_malloc(void *context, size_t size)
{
    DictStorageFailure *state = context;
    if (size == state->size && !state->called) {
        state->called = 1;
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &state->allocator);
        return NULL;
    }
    return state->allocator.malloc(state->allocator.ctx, size);
}

static void *
dict_storage_calloc(void *context, size_t count, size_t size)
{
    if (count != 0 && size > SIZE_MAX / count) {
        return NULL;
    }
    void *pointer = dict_storage_malloc(context, count * size);
    if (pointer != NULL) {
        memset(pointer, 0, count * size);
    }
    return pointer;
}

static void *
dict_storage_realloc(void *context, void *pointer, size_t size)
{
    DictStorageFailure *state = context;
    return state->allocator.realloc(state->allocator.ctx, pointer, size);
}

static void
dict_storage_free(void *context, void *pointer)
{
    DictStorageFailure *state = context;
    state->allocator.free(state->allocator.ctx, pointer);
}

static void
dict_storage_install(DictStorageFailure *state)
{
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &state->allocator);
    PyMemAllocatorEx allocator = {.ctx = state,
        .malloc = dict_storage_malloc, .calloc = dict_storage_calloc,
        .realloc = dict_storage_realloc, .free = dict_storage_free};
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &allocator);
}

static size_t
dict_fixture_values_size(PyDictValues *values)
{
    return offsetof(PyDictValues, values) + values->capacity * sizeof(PyValue) +
        _Py_SIZE_ROUND_UP(values->capacity, sizeof(PyObject *));
}

static PyObject *
dict_value_merge_mode(PyObject *module, PyObject *args)
{
    if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) != 3) {
        PyErr_SetString(PyExc_TypeError, "expected (target, source, mode)");
        return NULL;
    }
    PyObject *target = PyTuple_GetItemRef(args, 0);
    PyObject *input = target == NULL ? NULL : PyTuple_GetItemRef(args, 1);
    PyObject *mode_object = input == NULL ? NULL : PyTuple_GetItemRef(args, 2);
    long mode = mode_object == NULL ? -1 : PyLong_AsLong(mode_object);
    PyObject *duplicate = NULL;
    int status = -1;
    if (mode >= 0 && mode <= 2 && !PyErr_Occurred()) {
        status = mode == 2 ? _PyDict_MergeUniq(target, input, &duplicate) :
            PyDict_Merge(target, input, mode);
    }
    else if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_ValueError, "mode must be 0, 1 or 2");
    }
    if (status == -2 && !PyErr_Occurred()) {
        PyErr_SetObject(PyExc_KeyError, duplicate);
    }
    Py_XDECREF(duplicate);
    Py_XDECREF(mode_object);
    Py_XDECREF(input);
    Py_XDECREF(target);
    if (status < 0 || PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_value_setdefault_boxing_oom(PyObject *module, PyObject *key)
{
    if (!PyUnicode_CheckExact(key)) {
        PyErr_SetString(PyExc_TypeError, "expected exact Unicode query");
        return NULL;
    }
    PyObject *dict = dict_value_seed(module, key);
    if (dict == NULL) {
        return NULL;
    }
    Py_hash_t hash = PyObject_Hash(key);
    if (hash == -1) {
        Py_XDECREF(dict);
        return NULL;
    }
    DictBoxFailure failure = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    PyMemAllocatorEx allocator = {.ctx = &failure, .malloc = dict_fail_malloc,
        .calloc = dict_fail_calloc, .realloc = dict_fail_realloc, .free = dict_fail_free};
    PyObject *result = NULL;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    int status = PyDict_SetDefaultRef(dict, key, Py_None, &result);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    int valid = status == -1 && result == NULL && failure.called == 1 &&
        PyErr_ExceptionMatches(PyExc_MemoryError);
    PyErr_Clear();
    Py_XDECREF(result);
    result = NULL;
    PyValue found = PyValue_NULL;
    valid = valid && _PyDict_GetValue_KnownHash(dict,
        PyValue_FromObjectBorrow(key), hash, &found) == 1 &&
        PyValue_IsInlineInt(found) && PyValue_InlineInt(found) == INT32_MAX;
    PyValue_XCLOSE(found);
    if (valid) {
        status = PyDict_SetDefaultRef(dict, key, Py_None, &result);
        valid = status == 1 && result != NULL && PyLong_AsLong(result) == INT32_MAX;
    }
    Py_XDECREF(result);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "setdefault boxing OOM contract failed");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_value_clear_preserves_exception(PyObject *module, PyObject *instance)
{
    PyObject *dict = PyObject_GenericGetDict(instance, NULL);
    if (dict == NULL) {
        return NULL;
    }
    PyDictValues *values = ((PyDictObject *)dict)->ma_values;
    if (values == NULL || !values->embedded) {
        Py_XDECREF(dict);
        PyErr_SetString(PyExc_AssertionError, "fixture needs materialized inline values");
        return NULL;
    }
    PyObject *primary = PyObject_CallFunction(PyExc_ValueError, "s", "preserved entry");
    if (primary == NULL) {
        Py_XDECREF(dict);
        return NULL;
    }
    DictStorageFailure state = {.size = dict_fixture_values_size(values)};
    dict_storage_install(&state);
    PyErr_SetRaisedException(Py_NewRef(primary));
    /* This live-object mechanism witness pairs with the Python zero-RC
     * destruction test; it does not manufacture a refcount-zero instance. */
    PyObject_ClearManagedDict(instance);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &state.allocator);
    PyObject *observed = PyErr_GetRaisedException();
    int valid = observed == primary && state.called == 1 &&
        PyDict_GET_SIZE(dict) == 0 && ((PyDictObject *)dict)->ma_values == NULL &&
        _PyObject_GetManagedDict(instance) == NULL;
    Py_XDECREF(observed);
    Py_XDECREF(primary);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "managed clear lost entry exception or valid empty state");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Return the actual checked-read status, error owner and result while a known
 * entry exception distinguishes preservation from a new operation failure. */
static PyObject *
dict_value_getref_entry(PyObject *module, PyObject *args)
{
    if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected (dict, key)");
        return NULL;
    }
    PyObject *dict = PyTuple_GetItemRef(args, 0);
    PyObject *key = PyTuple_GetItemRef(args, 1);
    PyObject *primary = NULL, *result = NULL, *observed = NULL;
    PyObject *report = NULL;
    if (dict == NULL || key == NULL) {
        goto done;
    }
    primary = PyObject_CallFunction(PyExc_ValueError, "s", "entry marker");
    if (primary == NULL) {
        goto done;
    }
    PyErr_SetRaisedException(Py_NewRef(primary));
    int status = PyDict_GetItemRef(dict, key, &result);
    observed = PyErr_GetRaisedException();
    int valid = status < 0 ? observed != NULL && observed != primary && result == NULL
                           : observed == primary;
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError, "checked read confused entry and new errors");
        goto done;
    }
    report = PyTuple_New(3);
    if (report != NULL) {
        PyObject *status_object = PyLong_FromLong(status);
        if (status_object == NULL) {
            Py_XDECREF(report);
            report = NULL;
            goto done;
        }
        PyTuple_SET_ITEM(report, 0, status_object);
        PyTuple_SET_ITEM(report, 1, Py_NewRef(observed));
        PyTuple_SET_ITEM(report, 2, Py_NewRef(result != NULL ? result : Py_None));
    }
done:
    Py_XDECREF(result);
    Py_XDECREF(observed);
    Py_XDECREF(primary);
    Py_XDECREF(key);
    Py_XDECREF(dict);
    return report;
}

static PyObject *
dict_value_getref_boxing_oom(PyObject *module, PyObject *key)
{
    if (!PyUnicode_CheckExact(key)) {
        PyErr_SetString(PyExc_TypeError, "expected exact Unicode key");
        return NULL;
    }
    PyObject *dict = dict_value_seed(module, key);
    PyObject *primary = PyObject_CallFunction(PyExc_ValueError, "s", "entry marker");
    PyObject *result = NULL;
    Py_hash_t hash = PyObject_Hash(key);
    if (dict == NULL || primary == NULL || hash == -1) {
        Py_XDECREF(dict);
        Py_XDECREF(primary);
        return NULL;
    }
    DictBoxFailure state = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyMemAllocatorEx allocator = {.ctx = &state, .malloc = dict_fail_malloc,
        .calloc = dict_fail_calloc, .realloc = dict_fail_realloc, .free = dict_fail_free};
    PyErr_SetRaisedException(Py_NewRef(primary));
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    int status = _PyDict_GetItemRef_KnownHash((PyDictObject *)dict, key, hash, &result);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyObject *observed = PyErr_GetRaisedException();
    int valid = state.called == 1 && status == -1 && result == NULL &&
        observed != NULL && observed != primary &&
        PyObject_TypeCheck(observed, (PyTypeObject *)PyExc_MemoryError);
    Py_XDECREF(observed);
    Py_XDECREF(result);
    Py_XDECREF(primary);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "new boxing failure lost to entry exception");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_iterator_boxing_oom(PyObject *module, PyObject *iterator)
{
    DictBoxFailure state = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyMemAllocatorEx allocator = {.ctx = &state, .malloc = dict_fail_malloc,
        .calloc = dict_fail_calloc, .realloc = dict_fail_realloc, .free = dict_fail_free};
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    PyObject *result = PyIter_Next(iterator);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    int valid = state.called == 1 && result == NULL &&
        PyErr_ExceptionMatches(PyExc_MemoryError);
    PyErr_Clear();
    Py_XDECREF(result);
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError, "iterator fixture did not reach boxing OOM");
        return NULL;
    }
    return PyIter_Next(iterator);
}

static PyObject *
dict_iterator_prime_pair(PyObject *module, PyObject *iterator)
{
    PyObject *pair = PyIter_Next(iterator);
    if (pair == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "expected first item pair");
        }
        return NULL;
    }
    Py_hash_t hash = PyObject_Hash(pair);
    Py_XDECREF(pair);
    if (hash == -1 || PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromSsize_t(hash);
}

static PyObject *
dict_owned_entry(PyObject *module, PyObject *args)
{
    if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected (dictionary, position)");
        return NULL;
    }
    PyObject *dict = PyTuple_GetItemRef(args, 0);
    PyObject *position_object = PyTuple_GetItemRef(args, 1);
    if (dict == NULL || position_object == NULL) {
        Py_XDECREF(dict);
        Py_XDECREF(position_object);
        return NULL;
    }
    Py_ssize_t position = PyLong_AsSsize_t(position_object);
    Py_XDECREF(position_object);
    if (position == -1 && PyErr_Occurred()) {
        Py_XDECREF(dict);
        return NULL;
    }
    PyObject *key = NULL, *value = NULL;
    Py_hash_t hash = -1;
    int status = _PyDict_NextEntryRefKnownHash(dict, &position, &key, &value, &hash);
    PyObject *report = NULL;
    if (status >= 0) {
        report = Py_BuildValue("inOOn", status, position,
                              key != NULL ? key : Py_None,
                              value != NULL ? value : Py_None, hash);
    }
    Py_XDECREF(key);
    Py_XDECREF(value);
    Py_XDECREF(dict);
    return report;
}

static PyObject *
dict_owned_entry_boxing_oom(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *dict = PyDict_New();
    if (dict == NULL || _PyDict_SetValue_Take2((PyDictObject *)dict,
            PyValue_FromInt32(1), PyValue_FromInt32(INT32_MAX)) < 0)
    {
        Py_XDECREF(dict);
        return NULL;
    }
    DictBoxFailure state = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyMemAllocatorEx allocator = {.ctx = &state, .malloc = dict_fail_malloc,
        .calloc = dict_fail_calloc, .realloc = dict_fail_realloc, .free = dict_fail_free};
    PyObject *key = NULL, *value = NULL;
    Py_ssize_t position = 0;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    int status = PyDict_NextEntryRef(dict, &position, &key, &value);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    int valid = state.called == 1 && status == -1 && position == 0 &&
        key == NULL && value == NULL && PyErr_ExceptionMatches(PyExc_MemoryError);
    PyErr_Clear();
    Py_XDECREF(key);
    Py_XDECREF(value);
    key = value = NULL;
    if (valid) {
        status = PyDict_NextEntryRef(dict, &position, &key, &value);
        valid = status == 1 && key != NULL && value != NULL &&
            PyLong_AsLong(key) == 1 && PyLong_AsLong(value) == INT32_MAX &&
            PyDict_GET_SIZE(dict) == 1;
    }
    Py_XDECREF(key);
    Py_XDECREF(value);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "owned entry partial box rollback failed");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

typedef struct {
    PyMemAllocatorEx saved;
    PyObject *owner;
    Py_ssize_t *position;
    int called;
    int destroyed;
} DictEntryFinalPin;

static void
dict_entry_cursor_sentinel(PyObject *capsule)
{
    /* A capsule destructor runs in a deallocator, which must leave the
     * current exception as it found it; reporting a bad capsule as
     * unraisable would clear it. */
    PyObject *entry = PyErr_GetRaisedException();
    DictEntryFinalPin *state = PyCapsule_GetPointer(capsule, "dict-entry-cursor");
    if (state != NULL) {
        state->destroyed++;
        *state->position = 99;
    }
    else {
        PyErr_WriteUnraisable(capsule);
    }
    PyErr_SetRaisedException(entry);
}

static void
dict_entry_final_pin_release(DictEntryFinalPin *state)
{
    state->called++;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state->saved);
    PyObject *dict = state->owner;
    state->owner = NULL;
    /* The adapter's pin keeps dict alive until its own final cleanup. */
    Py_DECREF(dict);
}

static void *
dict_entry_final_pin_malloc(void *context, size_t size)
{
    DictEntryFinalPin *state = context;
    dict_entry_final_pin_release(state);
    return state->saved.malloc(state->saved.ctx, size);
}

static void *
dict_entry_final_pin_calloc(void *context, size_t count, size_t size)
{
    DictEntryFinalPin *state = context;
    dict_entry_final_pin_release(state);
    return state->saved.calloc(state->saved.ctx, count, size);
}

static void *
dict_entry_final_pin_realloc(void *context, void *pointer, size_t size)
{
    DictEntryFinalPin *state = context;
    dict_entry_final_pin_release(state);
    return state->saved.realloc(state->saved.ctx, pointer, size);
}

static void
dict_entry_final_pin_free(void *context, void *pointer)
{
    DictEntryFinalPin *state = context;
    state->saved.free(state->saved.ctx, pointer);
}

static PyObject *
dict_owned_entry_final_pin(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t position = 0;
    DictEntryFinalPin state = {.position = &position};
    state.owner = PyDict_New();
    if (state.owner == NULL) {
        return NULL;
    }
    PyObject *capsule = PyCapsule_New(&state, "dict-entry-cursor", dict_entry_cursor_sentinel);
    if (capsule == NULL) {
        Py_XDECREF(state.owner);
        return NULL;
    }
    if (_PyDict_SetValue_Take2((PyDictObject *)state.owner, PyValue_FromInt32(1),
                              PyValue_FromInt32(INT32_MAX)) < 0)
    {
        Py_XDECREF(capsule);
        Py_XDECREF(state.owner);
        return NULL;
    }
    if (_PyDict_SetValue_Take2((PyDictObject *)state.owner, PyValue_FromInt32(2),
                              PyValue_FromPyObjectSteal(capsule)) < 0)
    {
        Py_XDECREF(state.owner);
        return NULL;
    }
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyMemAllocatorEx allocator = {.ctx = &state, .malloc = dict_entry_final_pin_malloc,
        .calloc = dict_entry_final_pin_calloc, .realloc = dict_entry_final_pin_realloc,
        .free = dict_entry_final_pin_free};
    PyObject *key = NULL, *value = NULL;
    PyObject *dict = state.owner;  /* Borrow, kept alive by owner until the hook. */
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    int status = PyDict_NextEntryRef(dict, &position, &key, &value);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    int valid = state.called == 1 && state.owner == NULL && state.destroyed == 1 &&
        status == 1 && position == 99 && key != NULL && value != NULL &&
        PyLong_AsLong(key) == 1 && PyLong_AsLong(value) == INT32_MAX;
    Py_XDECREF(state.owner);
    Py_XDECREF(key);
    Py_XDECREF(value);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "owned entry changed cursor after final dict cleanup");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_owned_entry_contract(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *dict = PyDict_New();
    if (dict == NULL || _PyDict_SetValue_Take2((PyDictObject *)dict,
            PyValue_FromInt32(1), PyValue_FromInt32(INT32_MAX)) < 0)
    {
        Py_XDECREF(dict);
        return NULL;
    }
    Py_ssize_t position = 0;
    PyObject *output = NULL;
    int status = PyDict_NextEntryRef(dict, &position, &output, &output);
    int valid = status == -1 && position == 0 && output == NULL &&
        PyErr_ExceptionMatches(PyExc_ValueError);
    PyErr_Clear();
    status = PyDict_NextEntryRef(dict, &position, NULL, &output);
    valid = valid && status == 1 && output != NULL &&
        PyLong_AsLong(output) == INT32_MAX;
    Py_XDECREF(output);
    output = Py_None;  /* Non-owning sentinel: end must overwrite it with NULL. */
    status = PyDict_NextEntryRef(dict, &position, NULL, &output);
    valid = valid && status == 0 && output == NULL;
    position = 0;
    PyObject *primary = PyObject_CallFunction(PyExc_ValueError, "s", "entry marker");
    if (primary == NULL) {
        Py_XDECREF(dict);
        return NULL;
    }
    /* Like PyDict_Next(), a step succeeds with an unrelated exception
     * pending and leaves it pending. */
    PyErr_SetRaisedException(Py_NewRef(primary));
    status = PyDict_NextEntryRef(dict, &position, NULL, &output);
    PyObject *observed = PyErr_GetRaisedException();
    valid = valid && status == 1 && position == 1 && output != NULL &&
        PyLong_AsLong(output) == INT32_MAX && observed == primary;
    Py_XDECREF(output);
    Py_XDECREF(observed);
    Py_XDECREF(primary);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "owned entry status/output contract failed");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_owned_migration(PyObject *module, PyObject *args)
{
    if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) != 4) {
        PyErr_SetString(PyExc_TypeError, "expected (operation, dictionary, key, default)");
        return NULL;
    }
    PyObject *operation = NULL, *dict = NULL, *key = NULL, *fallback = NULL;
    PyObject *value = NULL, *report = NULL;
    operation = PyTuple_GetItemRef(args, 0);
    if (operation == NULL) {
        goto done;
    }
    dict = PyTuple_GetItemRef(args, 1);
    if (dict == NULL) {
        goto done;
    }
    key = PyTuple_GetItemRef(args, 2);
    if (key == NULL) {
        goto done;
    }
    fallback = PyTuple_GetItemRef(args, 3);
    if (fallback == NULL) {
        goto done;
    }
    long mode = PyLong_AsLong(operation);
    if (mode == -1 && PyErr_Occurred()) {
        goto done;
    }
    if (!PyAnyDict_Check(dict)) {
        PyErr_SetString(PyExc_TypeError, "expected a dictionary");
        goto done;
    }
    int status;
    switch (mode) {
        case 0:
            status = PyDict_GetItemRef(dict, key, &value);
            break;
        case 1: {
            const char *name = PyUnicode_AsUTF8(key);
            if (name == NULL) {
                goto done;
            }
            status = PyDict_GetItemStringRef(dict, name, &value);
            break;
        }
        case 2: {
            Py_hash_t hash = PyObject_Hash(key);
            if (hash == -1) {
                goto done;
            }
            status = _PyDict_GetItemRef_KnownHash((PyDictObject *)dict,
                                                key, hash, &value);
            break;
        }
        case 3:
        case 4:
            status = PyDict_SetDefaultRef(dict, key, fallback,
                                         mode == 3 ? &value : NULL);
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "unknown dictionary operation");
            goto done;
    }
    if (status >= 0) {
        report = Py_BuildValue("iO", status, value != NULL ? value : Py_None);
    }
done:
    Py_XDECREF(value);
    Py_XDECREF(fallback);
    Py_XDECREF(key);
    Py_XDECREF(dict);
    Py_XDECREF(operation);
    if (PyErr_Occurred()) {
        Py_XDECREF(report);
        return NULL;
    }
    return report;
}

static const char *
dict_public_value_kind(PyValue value)
{
    if (PyValue_IsNull(value)) {
        return "absent";
    }
    if (PyValue_IsInlineInt(value)) {
        return "int";
    }
    if (PyValue_IsInlineFloat(value)) {
        return "float";
    }
    return PyValue_IsNone(value) ? "none" : "object";
}

static PyObject *
dict_public_value_read(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 3) {
        PyErr_SetString(PyExc_TypeError, "expected dictionary, key, operation");
        return NULL;
    }
    PyObject *dict = Py_NewRef(args[0]), *key = Py_NewRef(args[1]);
    PyObject *object = NULL, *report = NULL;
    PyValue value = PyValue_NULL;
    long mode = PyLong_AsLong(args[2]);
    if (mode == -1 && PyErr_Occurred()) {
        goto done;
    }
    int status;
    if (mode == 0) {
        status = PyDict_GetValue(dict, PyValue_FromObjectBorrow(key), &value);
    }
    else if (mode == 1) {
        const char *name = PyUnicode_AsUTF8(key);
        if (name == NULL) {
            goto done;
        }
        status = PyDict_GetValueString(dict, name, &value);
    }
    else if (mode == 2) {
        status = PyDict_ContainsValue(dict, PyValue_FromObjectBorrow(key));
    }
    else {
        PyErr_SetString(PyExc_ValueError, "unknown dictionary read operation");
        goto done;
    }
    if (status < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (!PyValue_IsNull(value)) {
        object = PyValue_AsObject(value);
        if (object == NULL) {
            goto done;
        }
    }
    report = Py_BuildValue("iOs", status, object != NULL ? object : Py_None,
                           dict_public_value_kind(value));
done:
    PyValue_XCLOSE(value);
    Py_XDECREF(object);
    Py_XDECREF(key);
    Py_XDECREF(dict);
    if (PyErr_Occurred()) {
        Py_XDECREF(report);
        return NULL;
    }
    return report;
}

static PyObject *
dict_public_value_next(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 3) {
        PyErr_SetString(PyExc_TypeError, "expected dictionary, position, output mask");
        return NULL;
    }
    PyObject *dict = Py_NewRef(args[0]);
    PyObject *key_object = NULL, *value_object = NULL, *report = NULL;
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    Py_ssize_t position = PyLong_AsSsize_t(args[1]);
    if (position == -1 && PyErr_Occurred()) {
        goto done;
    }
    long mask = PyLong_AsLong(args[2]);
    if (mask == -1 && PyErr_Occurred()) {
        goto done;
    }
    if (mask < 0 || mask > 4) {
        PyErr_SetString(PyExc_ValueError, "invalid dictionary output mask");
        goto done;
    }
    int status = PyDict_NextValue(dict, &position,
        (mask & 1) || mask == 4 ? &key : NULL,
        mask == 4 ? &key : (mask & 2) ? &value : NULL);
    if (status < 0) {
        goto done;
    }
    if (!PyValue_IsNull(key)) {
        key_object = PyValue_AsObject(key);
        if (key_object == NULL) {
            goto done;
        }
    }
    if (!PyValue_IsNull(value)) {
        value_object = PyValue_AsObject(value);
        if (value_object == NULL || PyErr_Occurred()) {
            goto done;
        }
    }
    report = Py_BuildValue("inOOss", status, position,
        key_object != NULL ? key_object : Py_None,
        value_object != NULL ? value_object : Py_None,
        dict_public_value_kind(key), dict_public_value_kind(value));
done:
    PyValue_XCLOSE(value);
    PyValue_XCLOSE(key);
    Py_XDECREF(value_object);
    Py_XDECREF(key_object);
    Py_XDECREF(dict);
    if (PyErr_Occurred()) {
        Py_XDECREF(report);
        return NULL;
    }
    return report;
}

/* Run one public value API with an unrelated exception pending. It must
 * behave as with none pending and keep that exception, as upstream's
 * PyDict_GetItemRef(), PyDict_Contains(), PyDict_Next() and PyDict_DelItem()
 * do for keys that run no Python. Operations: 0 PyDict_GetValue(),
 * 1 PyDict_GetValueString(), 2 PyDict_ContainsValue(), 3 PyDict_NextValue()
 * from position 0, 4 PyDict_DelValue(). Return (status, first, second). */
static PyObject *
dict_public_value_pending(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 3) {
        PyErr_SetString(PyExc_TypeError, "expected dictionary, key, operation");
        return NULL;
    }
    long mode = PyLong_AsLong(args[2]);
    if (mode == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (mode < 0 || mode > 4) {
        PyErr_SetString(PyExc_ValueError, "invalid dictionary operation");
        return NULL;
    }
    const char *name = NULL;
    if (mode == 1) {
        name = PyUnicode_AsUTF8(args[1]);
        if (name == NULL) {
            return NULL;
        }
    }
    PyObject *marker = PyObject_CallFunction(PyExc_ValueError, "s", "pending");
    if (marker == NULL) {
        return NULL;
    }
    PyValue key = PyValue_FromObjectBorrow(args[1]);
    PyValue first = PyValue_NULL, second = PyValue_NULL;
    Py_ssize_t position = 0;
    int status;
    PyErr_SetRaisedException(Py_NewRef(marker));
    switch (mode) {
        case 0:
            status = PyDict_GetValue(args[0], key, &first);
            break;
        case 1:
            status = PyDict_GetValueString(args[0], name, &first);
            break;
        case 2:
            status = PyDict_ContainsValue(args[0], key);
            break;
        case 3:
            status = PyDict_NextValue(args[0], &position, &first, &second);
            break;
        default:
            status = PyDict_DelValue(args[0], key);
            break;
    }
    PyObject *observed = PyErr_GetRaisedException();
    PyObject *first_object = NULL, *second_object = NULL, *report = NULL;
    if (observed != marker) {
        if (observed == NULL) {
            PyErr_SetString(PyExc_AssertionError, "the pending exception was lost");
        }
        else {
            PyErr_Format(PyExc_AssertionError,
                         "the pending exception was replaced by %R", observed);
        }
        goto done;
    }
    first_object = PyValue_IsNull(first) ? Py_NewRef(Py_None) : PyValue_AsObject(first);
    second_object = PyValue_IsNull(second) ? Py_NewRef(Py_None) : PyValue_AsObject(second);
    if (first_object != NULL && second_object != NULL) {
        report = Py_BuildValue("iOO", status, first_object, second_object);
    }
done:
    PyValue_XCLOSE(first);
    PyValue_XCLOSE(second);
    Py_XDECREF(first_object);
    Py_XDECREF(second_object);
    Py_XDECREF(observed);
    Py_DECREF(marker);
    return report;
}

static PyObject *
dict_public_value_retirement(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 2) {
        PyErr_SetString(PyExc_TypeError, "expected [dictionary, query], operation");
        return NULL;
    }
    long mode = PyLong_AsLong(args[1]);
    if ((mode == -1 && PyErr_Occurred()) || (mode != 0 && mode != 2)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "expected GetValue or ContainsValue");
        }
        return NULL;
    }
    if (!PyList_CheckExact(args[0]) || PyList_GET_SIZE(args[0]) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected [dictionary, query]");
        return NULL;
    }
    /* Read both borrows without callbacks. Only the provider pins their owners;
     * hashing deliberately clears the holder and removes its last references. */
    PyValue source = PyList_GET_VALUE(args[0], 0);
    PyValue query = PyList_GET_VALUE(args[0], 1);
    if (!PyValue_IsObject(source)) {
        PyErr_SetString(PyExc_TypeError, "expected dictionary object");
        return NULL;
    }
    PyObject *dict = PyValue_AsKnownObjectBorrow(source);
    PyValue value = PyValue_NULL;
    int status = mode == 0 ? PyDict_GetValue(dict, query, &value)
                           : PyDict_ContainsValue(dict, query);
    /* No access to dict, query or list element addresses after the provider. */
    PyObject *object = NULL, *report = NULL;
    if (status >= 0 && !PyErr_Occurred()) {
        if (!PyValue_IsNull(value)) {
            object = PyValue_AsObject(value);
            if (object == NULL || PyErr_Occurred()) {
                goto done;
            }
        }
        report = Py_BuildValue("iO", status, object != NULL ? object : Py_None);
    }
done:
    PyValue_XCLOSE(value);
    Py_XDECREF(object);
    if (PyErr_Occurred()) {
        Py_XDECREF(report);
        return NULL;
    }
    return report;
}

static PyObject *
dict_public_value_string_oom(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 0) {
        PyErr_SetString(PyExc_TypeError, "expected no arguments");
        return NULL;
    }
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }
    DictBoxFailure failure = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    PyMemAllocatorEx allocator = {.ctx = &failure, .malloc = dict_fail_malloc,
        .calloc = dict_fail_calloc, .realloc = dict_fail_realloc,
        .free = dict_fail_free};
    PyValue output = PyValue_None;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    int status = PyDict_GetValueString(dict, "public-read-uncached-key-2147483647", &output);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    int valid = failure.called == 1 && status == -1 && PyValue_IsNull(output) &&
                PyErr_ExceptionMatches(PyExc_MemoryError);
    if (valid) {
        PyErr_Clear();
    }
    PyValue_XCLOSE(output);
    Py_XDECREF(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "public string read OOM contract failed");
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef methods[] = {
    {"dict_public_value_read", _PyCFunction_CAST(dict_public_value_read), METH_FASTCALL},
    {"dict_public_value_next", _PyCFunction_CAST(dict_public_value_next), METH_FASTCALL},
    {"dict_public_value_pending", _PyCFunction_CAST(dict_public_value_pending), METH_FASTCALL},
    {"dict_public_value_retirement", _PyCFunction_CAST(dict_public_value_retirement), METH_FASTCALL},
    {"dict_public_value_string_oom", _PyCFunction_CAST(dict_public_value_string_oom), METH_FASTCALL},
    {"dict_owned_migration", dict_owned_migration, METH_O},
    {"dict_owned_entry", dict_owned_entry, METH_O},
    {"dict_owned_entry_boxing_oom", dict_owned_entry_boxing_oom, METH_NOARGS},
    {"dict_owned_entry_final_pin", dict_owned_entry_final_pin, METH_NOARGS},
    {"dict_owned_entry_contract", dict_owned_entry_contract, METH_NOARGS},
    {"dict_value_getref_entry", dict_value_getref_entry, METH_O},
    {"dict_value_getref_boxing_oom", dict_value_getref_boxing_oom, METH_O},
    {"dict_iterator_boxing_oom", dict_iterator_boxing_oom, METH_O},
    {"dict_iterator_prime_pair", dict_iterator_prime_pair, METH_O},
    {"dict_value_setdefault_boxing_oom", dict_value_setdefault_boxing_oom, METH_O},
    {"dict_value_clear_preserves_exception", dict_value_clear_preserves_exception, METH_O},
    {"dict_value_merge_mode", dict_value_merge_mode, METH_O},
    {"dict_value_seed", dict_value_seed, METH_O},
    {"dict_value_pop_boxing_oom", dict_value_pop_boxing_oom, METH_O},
    {"dict_value_delete_if", dict_value_delete_if, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_Dict(PyObject *module)
{
    return PyModule_AddFunctions(module, methods);
}
