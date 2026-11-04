#include "parts.h"
#include "../_testcapi/util.h"  // NULLABLE, RETURN_INT

#include "pycore_critical_section.h"
#include "pycore_setobject.h"
#include "pycore_value.h"


static PyObject *
set_value_storage(PyObject *Py_UNUSED(module), PyObject *set)
{
    if (!PyAnySet_Check(set)) {
        PyErr_SetString(PyExc_TypeError, "expected set or frozenset");
        return NULL;
    }
    PySetObject *so = (PySetObject *)set;
    Py_ssize_t integers = 0, floats = 0, objects = 0;
    for (Py_ssize_t i = 0; i <= so->mask; i++) {
        setentry *entry = &so->table[i];
        if (PyValue_IsNull(entry->key)) {
            assert(entry->hash == 0);
            continue;
        }
        if (entry->hash == -1) {
            assert(PyValue_IsObject(entry->key));
            assert(PyValue_AsKnownObjectBorrow(entry->key) == _PySet_Dummy);
            continue;
        }
        assert(PyValue_IsLanguageValue(entry->key));
        assert(PyValue_IsHeapSafe(entry->key));
        if (PyValue_IsInlineInt(entry->key)) {
            integers++;
        }
        else if (PyValue_IsInlineFloat(entry->key)) {
            floats++;
        }
        else {
            objects++;
        }
    }
    assert(integers + floats + objects == so->used);
    return Py_BuildValue("nnn", integers, floats, objects);
}

typedef struct {
    PyMemAllocatorEx saved;
} SetBoxFailure;

static void *
set_box_fail_malloc(void *ctx, size_t size)
{
    return NULL;
}

static void *
set_box_fail_calloc(void *ctx, size_t count, size_t size)
{
    return NULL;
}

static void *
set_box_fail_realloc(void *ctx, void *ptr, size_t size)
{
    return NULL;
}

static void
set_box_fail_free(void *ctx, void *ptr)
{
    SetBoxFailure *failure = ctx;
    failure->saved.free(failure->saved.ctx, ptr);
}

static void
set_fail_boxing(SetBoxFailure *failure)
{
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &failure->saved);
    PyMemAllocatorEx allocator = {
        .ctx = failure, .malloc = set_box_fail_malloc,
        .calloc = set_box_fail_calloc, .realloc = set_box_fail_realloc,
        .free = set_box_fail_free,
    };
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
}

static PyObject *
set_value_frozenset_tracking(PyObject *Py_UNUSED(module), PyObject *key)
{
    PyObject *frozen = PyFrozenSet_New(NULL);
    if (frozen == NULL) {
        return NULL;
    }
    if (PyObject_GC_IsTracked(frozen) || !PyObject_GC_IsTracked(key)) {
        Py_DECREF(frozen);
        PyErr_SetString(PyExc_AssertionError, "invalid frozenset tracking fixture");
        return NULL;
    }
    if (PySet_AddValue(frozen, PyValue_FromObjectBorrow(key)) < 0) {
        Py_DECREF(frozen);
        return NULL;
    }
    if (!PyObject_GC_IsTracked(frozen)) {
        Py_DECREF(frozen);
        PyErr_SetString(PyExc_AssertionError,
                        "value insertion left a frozenset untracked");
        return NULL;
    }
    return frozen;
}

static PyObject *
set_value_boxing_errors(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *set = PySet_New(NULL);
    if (set == NULL) {
        return NULL;
    }
    if (PySet_AddValue(set, PyValue_FromInt32(INT32_MAX)) < 0) {
        Py_DECREF(set);
        return NULL;
    }
    PyObject *iter = PyObject_GetIter(set);
    if (iter == NULL) {
        Py_DECREF(set);
        return NULL;
    }
    /* Two-digit INT32_MAX bypasses the single-digit integer freelist. */
    SetBoxFailure failure;
    set_fail_boxing(&failure);
    PyObject *boxed = PyIter_Next(iter);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    int valid = boxed == NULL && PyErr_ExceptionMatches(PyExc_MemoryError);
    Py_XDECREF(boxed);
    PyErr_Clear();
    /* The failed step consumed the key, as iterators do (listiter_next()). */
    boxed = valid ? PyIter_Next(iter) : NULL;
    valid = valid && boxed == NULL && !PyErr_Occurred();
    Py_XDECREF(boxed);
    Py_DECREF(iter);

    set_fail_boxing(&failure);
    boxed = PySet_Pop(set);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    valid = valid && boxed == NULL && PyErr_ExceptionMatches(PyExc_MemoryError);
    Py_XDECREF(boxed);
    PyErr_Clear();
    valid = valid && PySet_GET_SIZE(set) == 1;

    set_fail_boxing(&failure);
    PyValue value = PySet_PopValue(set);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failure.saved);
    valid = valid && !PyErr_Occurred() && PyValue_IsInlineInt(value) &&
            PyValue_InlineInt(value) == INT32_MAX && PySet_GET_SIZE(set) == 0;
    PyValue_XCLOSE(value);
    Py_DECREF(set);
    if (!valid) {
        PyErr_Clear();
        PyErr_SetString(PyExc_AssertionError,
                        "set boxing lost OOM, kept an iterated key, "
                        "consumed a popped key or boxed a value pop");
        return NULL;
    }
    Py_RETURN_NONE;
}


/* Clear set with an exception pending. The keys' releases report their own
 * errors as unraisable; the pending exception must survive and the set must
 * end empty. */
static PyObject *
set_clear_pending(PyObject *Py_UNUSED(module), PyObject *set)
{
    if (!PySet_Check(set)) {
        PyErr_SetString(PyExc_TypeError, "expected a set");
        return NULL;
    }
    PyObject *pending = PyObject_CallFunction(PyExc_ValueError, "s",
                                              "pending set clear error");
    if (pending == NULL) {
        return NULL;
    }
    PyErr_SetRaisedException(Py_NewRef(pending));
    int status = PySet_Clear(set);
    PyObject *raised = PyErr_GetRaisedException();
    int valid = status == 0 && raised == pending && PySet_GET_SIZE(set) == 0;
    Py_XDECREF(raised);
    Py_DECREF(pending);
    if (!valid) {
        PyErr_SetString(PyExc_AssertionError,
                        "set clear changed the pending exception or kept a key");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
set_update(PyObject *self, PyObject *args)
{
    PyObject *set, *iterable;
    if (!PyArg_ParseTuple(args, "OO", &set, &iterable)) {
        return NULL;
    }
    NULLABLE(set);
    NULLABLE(iterable);
    RETURN_INT(_PySet_Update(set, iterable));
}

static PyObject *
set_next_entry(PyObject *self, PyObject *args)
{
    int rc;
    Py_ssize_t pos;
    Py_hash_t hash = (Py_hash_t)UNINITIALIZED_SIZE;
    PyObject *set, *item = UNINITIALIZED_PTR;
    if (!PyArg_ParseTuple(args, "On", &set, &pos)) {
        return NULL;
    }
    NULLABLE(set);
    Py_BEGIN_CRITICAL_SECTION(set);
    rc = _PySet_NextEntryRef(set, &pos, &item, &hash);
    Py_END_CRITICAL_SECTION();
    if (rc == 1) {
        PyObject *ret = Py_BuildValue("innO", rc, pos, hash, item);
        Py_DECREF(item);
        return ret;
    }
    assert(item == UNINITIALIZED_PTR);
    assert(hash == (Py_hash_t)UNINITIALIZED_SIZE);
    if (rc == -1) {
        return NULL;
    }
    assert(rc == 0);
    Py_RETURN_NONE;
}


static PyMethodDef TestMethods[] = {
    {"set_value_storage", set_value_storage, METH_O},
    {"set_value_frozenset_tracking", set_value_frozenset_tracking, METH_O},
    {"set_value_boxing_errors", set_value_boxing_errors, METH_NOARGS},
    {"set_clear_pending", set_clear_pending, METH_O},
    {"set_update", set_update, METH_VARARGS},
    {"set_next_entry", set_next_entry, METH_VARARGS},

    {NULL},
};

int
_PyTestInternalCapi_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
