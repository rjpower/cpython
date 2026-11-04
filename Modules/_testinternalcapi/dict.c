#include "parts.h"

#include "pycore_dict.h"
#include "pycore_tuple.h"
#include "pycore_value.h"


static void
dict_fixture_close(PyObject *object)
{
    PyObject *saved = PyErr_GetRaisedException();
    Py_XDECREF(object);
    _PyValue_RestoreException(saved);
}

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
        dict_fixture_close(dict);
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
    if (hash == -1 || PyErr_Occurred()) {
        PyValue_CLOSE(query);
        dict_fixture_close(dict);
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
    dict_fixture_close(result);
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
    dict_fixture_close(result);
    PyValue_CLOSE(query);
    dict_fixture_close(dict);
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
    dict_fixture_close(key);
    dict_fixture_close(dict);
    return status < 0 || PyErr_Occurred() ? NULL : PyLong_FromLong(status);
}

/* Restore the allocator before callbacks; the enclosing fixture owns this
 * record and all objects until the actual copy/detach operation returns. */
typedef struct {
    PyMemAllocatorEx allocator;
    size_t size;
    PyObject *dict;
    PyObject *instance;
    PyObject *replacement;
    int called;
    int fail;
} DictStorageReentry;

static int
dict_storage_reenter(DictStorageReentry *state)
{
    if (state->replacement != NULL) {
        return PyObject_SetAttrString(state->instance, "__dict__", state->replacement);
    }
    PyDict_Clear(state->dict);
    if (PyErr_Occurred()) {
        return -1;
    }
    for (int i = 1000; i < 1200; i++) {
        if (PyDict_SetValue(state->dict, PyValue_FromInt32(i),
                            PyValue_FromInt32(i + 1)) < 0) {
            return -1;
        }
    }
    return 0;
}

static void *
dict_storage_malloc(void *context, size_t size)
{
    DictStorageReentry *state = context;
    if (size == state->size && !state->called) {
        state->called = 1;
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &state->allocator);
        if (state->fail) {
            return NULL;
        }
        if (dict_storage_reenter(state) < 0 || PyErr_Occurred()) {
            return NULL;
        }
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
    DictStorageReentry *state = context;
    return state->allocator.realloc(state->allocator.ctx, pointer, size);
}

static void
dict_storage_free(void *context, void *pointer)
{
    DictStorageReentry *state = context;
    state->allocator.free(state->allocator.ctx, pointer);
}

static void
dict_storage_install(DictStorageReentry *state)
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

static int
dict_storage_check_copy(PyObject *dict, PyObject *copy, int called)
{
    if (copy == NULL || PyErr_Occurred()) {
        return -1;
    }
    PyValue found = PyValue_NULL;
    int valid = called == 1 && PyDict_GET_SIZE(dict) == 200 && PyDict_GET_SIZE(copy) == 200 &&
        _PyDict_GetValue_KnownHash(copy, PyValue_FromInt32(1199), 1199, &found) == 1 &&
        PyValue_IsInlineInt(found) && PyValue_InlineInt(found) == 1200;
    PyValue_XCLOSE(found);
    if (!valid && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_AssertionError, "copy did not reacquire the allocation-mutated source");
    }
    return valid && !PyErr_Occurred() ? 0 : -1;
}

static PyObject *
dict_value_copy_reentry(PyObject *module, PyObject *unused)
{
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }
    for (int i = 0; i < 16; i++) {
        if (PyDict_SetValue(dict, PyValue_FromInt32(i), PyValue_FromInt32(i)) < 0) {
            dict_fixture_close(dict);
            return NULL;
        }
    }
    PyDictKeysObject *keys = ((PyDictObject *)dict)->ma_keys;
    size_t bytes = sizeof(PyDictKeysObject) + ((size_t)1 << keys->dk_log2_index_bytes) +
        (keys->dk_nentries + keys->dk_usable) * sizeof(PyDictKeyEntry);
    DictStorageReentry state = {.dict = dict, .size = bytes};
    dict_storage_install(&state);
    PyObject *copy = PyDict_Copy(dict);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &state.allocator);
    int status = dict_storage_check_copy(dict, copy, state.called);
    dict_fixture_close(copy);
    dict_fixture_close(dict);
    if (status < 0 || PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_value_split_copy_reentry(PyObject *module, PyObject *instance)
{
    PyObject *dict = PyObject_GenericGetDict(instance, NULL);
    if (dict == NULL) {
        return NULL;
    }
    PyDictValues *values = ((PyDictObject *)dict)->ma_values;
    if (values == NULL) {
        dict_fixture_close(dict);
        PyErr_SetString(PyExc_AssertionError, "fixture needs split storage");
        return NULL;
    }
    DictStorageReentry state = {.dict = dict, .size = dict_fixture_values_size(values)};
    dict_storage_install(&state);
    PyObject *copy = PyDict_Copy(dict);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &state.allocator);
    int status = dict_storage_check_copy(dict, copy, state.called);
    dict_fixture_close(copy);
    dict_fixture_close(dict);
    if (status < 0 || PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
dict_value_detach_reentry(PyObject *module, PyObject *instance)
{
    PyObject *old = PyObject_GenericGetDict(instance, NULL);
    PyObject *inner = old == NULL ? NULL : PyDict_New();
    PyObject *outer = inner == NULL ? NULL : PyDict_New();
    if (outer == NULL) {
        dict_fixture_close(inner);
        dict_fixture_close(old);
        return NULL;
    }
    PyDictValues *values = ((PyDictObject *)old)->ma_values;
    if (values == NULL || !values->embedded) {
        dict_fixture_close(outer);
        dict_fixture_close(inner);
        dict_fixture_close(old);
        PyErr_SetString(PyExc_AssertionError, "fixture needs materialized inline storage");
        return NULL;
    }
    Py_ssize_t old_size = PyDict_GET_SIZE(old);
    DictStorageReentry state = {.dict = old, .instance = instance,
        .replacement = inner, .size = dict_fixture_values_size(values)};
    dict_storage_install(&state);
    int status = PyObject_SetAttrString(instance, "__dict__", outer);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &state.allocator);
    PyObject *current = status < 0 ? NULL : PyObject_GenericGetDict(instance, NULL);
    int valid = status == 0 && current == outer && state.called == 1 &&
        PyDict_GET_SIZE(old) == old_size && ((PyDictObject *)old)->ma_values != NULL &&
        !((PyDictObject *)old)->ma_values->embedded;
    dict_fixture_close(current);
    dict_fixture_close(outer);
    dict_fixture_close(inner);
    dict_fixture_close(old);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "detach reused an association changed during allocation");
        }
        return NULL;
    }
    Py_RETURN_NONE;
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
    dict_fixture_close(duplicate);
    dict_fixture_close(mode_object);
    dict_fixture_close(input);
    dict_fixture_close(target);
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
    if (hash == -1 || PyErr_Occurred()) {
        dict_fixture_close(dict);
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
    dict_fixture_close(result);
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
    dict_fixture_close(result);
    dict_fixture_close(dict);
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
        dict_fixture_close(dict);
        PyErr_SetString(PyExc_AssertionError, "fixture needs materialized inline values");
        return NULL;
    }
    PyObject *primary = PyObject_CallFunction(PyExc_ValueError, "s", "preserved entry");
    if (primary == NULL) {
        dict_fixture_close(dict);
        return NULL;
    }
    DictStorageReentry state = {.dict = dict, .fail = 1,
        .size = dict_fixture_values_size(values)};
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
    dict_fixture_close(observed);
    dict_fixture_close(primary);
    dict_fixture_close(dict);
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
            dict_fixture_close(report);
            report = NULL;
            goto done;
        }
        PyTuple_SET_ITEM(report, 0, status_object);
        PyTuple_SET_ITEM(report, 1, Py_NewRef(observed));
        PyTuple_SET_ITEM(report, 2, Py_NewRef(result != NULL ? result : Py_None));
    }
done:
    dict_fixture_close(result);
    dict_fixture_close(observed);
    dict_fixture_close(primary);
    dict_fixture_close(key);
    dict_fixture_close(dict);
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
        dict_fixture_close(dict);
        dict_fixture_close(primary);
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
    dict_fixture_close(observed);
    dict_fixture_close(result);
    dict_fixture_close(primary);
    dict_fixture_close(dict);
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
    dict_fixture_close(result);
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError, "iterator fixture did not reach boxing OOM");
        return NULL;
    }
    return PyIter_Next(iterator);
}

typedef struct {
    PyMemAllocatorEx saved;
    PyObject *iterator;
    PyObject *nested;
    int called;
} DictIteratorReentry;

static void *
dict_iterator_reentry_malloc(void *context, size_t size)
{
    DictIteratorReentry *state = context;
    state->called++;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state->saved);
    state->nested = PyIter_Next(state->iterator);
    /* Fail the outer box only after the nested next has advanced the cursor. */
    return NULL;
}

static void *
dict_iterator_reentry_calloc(void *context, size_t count, size_t size)
{
    return dict_iterator_reentry_malloc(context, 0);
}

static void *
dict_iterator_reentry_realloc(void *context, void *pointer, size_t size)
{
    return dict_iterator_reentry_malloc(context, 0);
}

static void
dict_iterator_reentry_free(void *context, void *pointer)
{
    DictIteratorReentry *state = context;
    state->saved.free(state->saved.ctx, pointer);
}

static PyObject *
dict_iterator_boxing_reentry(PyObject *module, PyObject *iterator)
{
    DictIteratorReentry state = {.iterator = iterator};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyMemAllocatorEx allocator = {.ctx = &state, .malloc = dict_iterator_reentry_malloc,
        .calloc = dict_iterator_reentry_calloc, .realloc = dict_iterator_reentry_realloc,
        .free = dict_iterator_reentry_free};
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    PyObject *outer = PyIter_Next(iterator);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    int valid = state.called == 1 && outer == NULL && state.nested != NULL &&
        PyErr_ExceptionMatches(PyExc_MemoryError);
    PyErr_Clear();
    dict_fixture_close(outer);
    if (!valid) {
        dict_fixture_close(state.nested);
        PyErr_SetString(PyExc_AssertionError, "iterator reentry did not advance before outer OOM");
        return NULL;
    }
    return state.nested;
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
    dict_fixture_close(pair);
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
        dict_fixture_close(dict);
        dict_fixture_close(position_object);
        return NULL;
    }
    Py_ssize_t position = PyLong_AsSsize_t(position_object);
    dict_fixture_close(position_object);
    if (position == -1 && PyErr_Occurred()) {
        dict_fixture_close(dict);
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
    dict_fixture_close(key);
    dict_fixture_close(value);
    dict_fixture_close(dict);
    return report;
}

static PyObject *
dict_owned_entry_boxing_oom(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *dict = PyDict_New();
    if (dict == NULL || _PyDict_SetValue_Take2((PyDictObject *)dict,
            PyValue_FromInt32(1), PyValue_FromInt32(INT32_MAX)) < 0)
    {
        dict_fixture_close(dict);
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
    dict_fixture_close(key);
    dict_fixture_close(value);
    key = value = NULL;
    if (valid) {
        status = PyDict_NextEntryRef(dict, &position, &key, &value);
        valid = status == 1 && key != NULL && value != NULL &&
            PyLong_AsLong(key) == 1 && PyLong_AsLong(value) == INT32_MAX &&
            PyDict_GET_SIZE(dict) == 1;
    }
    dict_fixture_close(key);
    dict_fixture_close(value);
    dict_fixture_close(dict);
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
    PyObject *dict;
    Py_ssize_t *position;
    PyObject *key;
    PyObject *value;
    int status;
    int called;
} DictEntryReentry;

static void *
dict_entry_reentry_malloc(void *context, size_t size)
{
    DictEntryReentry *state = context;
    state->called++;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state->saved);
    state->status = PyDict_NextEntryRef(state->dict, state->position,
                                       &state->key, &state->value);
    return NULL;
}

static void *
dict_entry_reentry_calloc(void *context, size_t count, size_t size)
{
    return dict_entry_reentry_malloc(context, 0);
}

static void *
dict_entry_reentry_realloc(void *context, void *pointer, size_t size)
{
    return dict_entry_reentry_malloc(context, 0);
}

static void
dict_entry_reentry_free(void *context, void *pointer)
{
    DictEntryReentry *state = context;
    state->saved.free(state->saved.ctx, pointer);
}

static PyObject *
dict_owned_entry_boxing_reentry(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }
    const int32_t values[] = {INT32_MAX, INT32_MIN, 17};
    for (int i = 0; i < 3; i++) {
        if (_PyDict_SetValue_Take2((PyDictObject *)dict, PyValue_FromInt32(i + 1),
                                  PyValue_FromInt32(values[i])) < 0)
        {
            dict_fixture_close(dict);
            return NULL;
        }
    }
    Py_ssize_t position = 0;
    DictEntryReentry state = {.dict = dict, .position = &position};
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    PyMemAllocatorEx allocator = {.ctx = &state, .malloc = dict_entry_reentry_malloc,
        .calloc = dict_entry_reentry_calloc, .realloc = dict_entry_reentry_realloc,
        .free = dict_entry_reentry_free};
    PyObject *key = NULL, *value = NULL;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
    int status = PyDict_NextEntryRef(dict, &position, &key, &value);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &state.saved);
    int valid = state.called == 1 && status == -1 && position == 2 &&
        key == NULL && value == NULL && state.status == 1 &&
        state.key != NULL && state.value != NULL &&
        PyErr_ExceptionMatches(PyExc_MemoryError);
    PyErr_Clear();
    valid = valid && PyLong_AsLong(state.key) == 2 &&
        PyLong_AsLong(state.value) == INT32_MIN;
    dict_fixture_close(state.key);
    dict_fixture_close(state.value);
    dict_fixture_close(key);
    dict_fixture_close(value);
    key = value = NULL;
    if (valid) {
        status = PyDict_NextEntryRef(dict, &position, &key, &value);
        valid = status == 1 && key != NULL && value != NULL &&
            PyLong_AsLong(key) == 3 && PyLong_AsLong(value) == 17;
    }
    dict_fixture_close(key);
    dict_fixture_close(value);
    dict_fixture_close(dict);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "owned entry overwrote nested cursor progress");
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
    PyObject *entry = PyErr_GetRaisedException();
    DictEntryFinalPin *state = PyCapsule_GetPointer(capsule, "dict-entry-cursor");
    if (state != NULL) {
        state->destroyed++;
        *state->position = 99;
    }
    else {
        PyErr_WriteUnraisable(capsule);
    }
    _PyValue_RestoreException(entry);
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
        dict_fixture_close(state.owner);
        return NULL;
    }
    if (_PyDict_SetValue_Take2((PyDictObject *)state.owner, PyValue_FromInt32(1),
                              PyValue_FromInt32(INT32_MAX)) < 0)
    {
        dict_fixture_close(capsule);
        dict_fixture_close(state.owner);
        return NULL;
    }
    if (_PyDict_SetValue_Take2((PyDictObject *)state.owner, PyValue_FromInt32(2),
                              PyValue_FromPyObjectSteal(capsule)) < 0)
    {
        dict_fixture_close(state.owner);
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
    dict_fixture_close(state.owner);
    dict_fixture_close(key);
    dict_fixture_close(value);
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
        dict_fixture_close(dict);
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
    dict_fixture_close(output);
    output = Py_None;  /* Non-owning sentinel: end must overwrite it with NULL. */
    status = PyDict_NextEntryRef(dict, &position, NULL, &output);
    valid = valid && status == 0 && output == NULL;
    position = 0;
    PyObject *primary = PyObject_CallFunction(PyExc_ValueError, "s", "entry marker");
    if (primary == NULL) {
        dict_fixture_close(dict);
        return NULL;
    }
    PyErr_SetRaisedException(Py_NewRef(primary));
    status = PyDict_NextEntryRef(dict, &position, NULL, &output);
    PyObject *observed = PyErr_GetRaisedException();
    valid = valid && status == -1 && position == 0 && output == NULL && observed == primary;
    dict_fixture_close(observed);
    dict_fixture_close(primary);
    dict_fixture_close(dict);
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
    dict_fixture_close(value);
    dict_fixture_close(fallback);
    dict_fixture_close(key);
    dict_fixture_close(dict);
    dict_fixture_close(operation);
    if (PyErr_Occurred()) {
        dict_fixture_close(report);
        return NULL;
    }
    return report;
}

static void
dict_public_close_value(PyValue value)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_XCLOSE(value);
    _PyValue_RestoreException(saved);
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
        if (object == NULL || PyErr_Occurred()) {
            goto done;
        }
    }
    report = Py_BuildValue("iOs", status, object != NULL ? object : Py_None,
                           dict_public_value_kind(value));
done:
    dict_public_close_value(value);
    dict_fixture_close(object);
    dict_fixture_close(key);
    dict_fixture_close(dict);
    if (PyErr_Occurred()) {
        dict_fixture_close(report);
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
    if (status < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (!PyValue_IsNull(key)) {
        key_object = PyValue_AsObject(key);
        if (key_object == NULL || PyErr_Occurred()) {
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
    dict_public_close_value(value);
    dict_public_close_value(key);
    dict_fixture_close(value_object);
    dict_fixture_close(key_object);
    dict_fixture_close(dict);
    if (PyErr_Occurred()) {
        dict_fixture_close(report);
        return NULL;
    }
    return report;
}

static PyObject *
dict_public_value_entry_error(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 4 || !PyExceptionInstance_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError, "expected dictionary, key, exception, operation");
        return NULL;
    }
    long mode = PyLong_AsLong(args[3]);
    if ((mode == -1 && PyErr_Occurred()) || mode < 0 || mode > 4) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "invalid dictionary operation");
        }
        return NULL;
    }
    PyObject *primary = Py_NewRef(args[2]);
    PyValue key = PyValue_FromObjectBorrow(args[1]);
    /* Immortal non-owning sentinels check that outputs are initialized. */
    PyValue first = PyValue_None, second = PyValue_None;
    Py_ssize_t position = 0;
    PyErr_SetRaisedException(Py_NewRef(primary));
    int status;
    switch (mode) {
        case 0:
            status = PyDict_GetValue(args[0], key, &first);
            break;
        case 1:
            status = PyDict_GetValueString(args[0], "key", &first);
            break;
        case 2:
            status = PyDict_ContainsValue(args[0], key);
            break;
        default:
            status = PyDict_NextValue(args[0], &position, &first,
                                      mode == 4 ? &first : &second);
            break;
    }
    PyObject *observed = PyErr_GetRaisedException();
    int valid = status == -1 && observed == primary && position == 0 &&
        (mode == 2 || PyValue_IsNull(first)) &&
        (mode != 3 || PyValue_IsNull(second));
    dict_public_close_value(first);
    dict_public_close_value(second);
    dict_fixture_close(observed);
    dict_fixture_close(primary);
    if (!valid || PyErr_Occurred()) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_AssertionError, "public dictionary entry error was changed");
        }
        return NULL;
    }
    Py_RETURN_NONE;
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
    dict_public_close_value(value);
    dict_fixture_close(object);
    if (PyErr_Occurred()) {
        dict_fixture_close(report);
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
    dict_public_close_value(output);
    dict_fixture_close(dict);
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
    {"dict_public_value_entry_error", _PyCFunction_CAST(dict_public_value_entry_error), METH_FASTCALL},
    {"dict_public_value_retirement", _PyCFunction_CAST(dict_public_value_retirement), METH_FASTCALL},
    {"dict_public_value_string_oom", _PyCFunction_CAST(dict_public_value_string_oom), METH_FASTCALL},
    {"dict_owned_migration", dict_owned_migration, METH_O},
    {"dict_owned_entry", dict_owned_entry, METH_O},
    {"dict_owned_entry_boxing_oom", dict_owned_entry_boxing_oom, METH_NOARGS},
    {"dict_owned_entry_boxing_reentry", dict_owned_entry_boxing_reentry, METH_NOARGS},
    {"dict_owned_entry_final_pin", dict_owned_entry_final_pin, METH_NOARGS},
    {"dict_owned_entry_contract", dict_owned_entry_contract, METH_NOARGS},
    {"dict_value_getref_entry", dict_value_getref_entry, METH_O},
    {"dict_value_getref_boxing_oom", dict_value_getref_boxing_oom, METH_O},
    {"dict_iterator_boxing_oom", dict_iterator_boxing_oom, METH_O},
    {"dict_iterator_boxing_reentry", dict_iterator_boxing_reentry, METH_O},
    {"dict_iterator_prime_pair", dict_iterator_prime_pair, METH_O},
    {"dict_value_setdefault_boxing_oom", dict_value_setdefault_boxing_oom, METH_O},
    {"dict_value_clear_preserves_exception", dict_value_clear_preserves_exception, METH_O},
    {"dict_value_copy_reentry", dict_value_copy_reentry, METH_NOARGS},
    {"dict_value_split_copy_reentry", dict_value_split_copy_reentry, METH_O},
    {"dict_value_detach_reentry", dict_value_detach_reentry, METH_O},
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
