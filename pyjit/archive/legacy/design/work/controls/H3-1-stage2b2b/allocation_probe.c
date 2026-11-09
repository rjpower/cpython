#define Py_BUILD_CORE_MODULE
#include "Python.h"
#include "pycore_instruments.h"

#define CAPACITY 128

typedef struct {
    PyMemAllocatorEx original;
    void *pointers[CAPACITY];
    size_t sizes[CAPACITY];
    int live[CAPACITY];
    int count;
    int overflow;
    int untracked_frees;
    size_t current;
    size_t peak;
} Recorder;

static void *
record_malloc(void *ctx, size_t size)
{
    Recorder *record = ctx;
    int index = record->count;
    if (index >= CAPACITY) {
        record->overflow = 1;
        return record->original.malloc(record->original.ctx, size);
    }
    record->count++;
    record->sizes[index] = size;
    void *ptr = record->original.malloc(record->original.ctx, size);
    record->pointers[index] = ptr;
    record->live[index] = ptr != NULL;
    if (ptr != NULL) {
        record->current += size;
        if (record->current > record->peak) {
            record->peak = record->current;
        }
    }
    return ptr;
}

static void
record_free(void *ctx, void *ptr)
{
    Recorder *record = ctx;
    int found = ptr == NULL;
    for (int i = record->count; i-- > 0;) {
        if (record->live[i] && record->pointers[i] == ptr) {
            found = 1;
            record->live[i] = 0;
            record->current -= record->sizes[i];
            break;
        }
    }
    record->untracked_frees += !found;
    record->original.free(record->original.ctx, ptr);
}

static void *
record_calloc(void *ctx, size_t count, size_t size)
{
    Recorder *record = ctx;
    record->overflow = 1;
    return record->original.calloc(record->original.ctx, count, size);
}

static void *
record_realloc(void *ctx, void *ptr, size_t size)
{
    Recorder *record = ctx;
    record->overflow = 1;
    return record->original.realloc(record->original.ctx, ptr, size);
}

static PyObject *
profile(PyObject *self, PyObject *args)
{
    PyObject *callable, *callargs;
    if (!PyArg_ParseTuple(args, "OO!:profile", &callable, &PyTuple_Type,
                          &callargs)) {
        return NULL;
    }
    Recorder record = {0};
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &record.original);
    PyMemAllocatorEx wrapper = {&record, record_malloc, record_calloc,
                               record_realloc, record_free};
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &wrapper);
    PyObject *result = PyObject_Call(callable, callargs, NULL);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &record.original);
    int failed = result == NULL;
    if (failed) {
        if (!PyErr_ExceptionMatches(PyExc_MemoryError)) {
            return NULL;
        }
        PyErr_Clear();
    }
    else {
        Py_DECREF(result);
    }
    if (record.overflow) {
        PyErr_SetString(PyExc_AssertionError, "untracked allocation operation");
        return NULL;
    }
    PyObject *entries = PyList_New(record.count);
    if (entries == NULL) {
        return NULL;
    }
    for (int i = 0; i < record.count; i++) {
        PyObject *entry = Py_BuildValue("(nii)", (Py_ssize_t)record.sizes[i],
                                        record.pointers[i] != NULL, record.live[i]);
        if (entry == NULL) {
            Py_DECREF(entries);
            return NULL;
        }
        PyList_SET_ITEM(entries, i, entry);
    }
    return Py_BuildValue("(Nnniii)", entries, (Py_ssize_t)record.peak,
                         (Py_ssize_t)record.current, failed,
                         (int)sizeof(_PyCoLineInstrumentationData), record.untracked_frees);
}

static PyObject *
inspect_lines(PyObject *self, PyObject *args)
{
    PyCodeObject *code;
    if (!PyArg_ParseTuple(args, "O!:inspect_lines", &PyCode_Type, &code)) {
        return NULL;
    }
    Py_ssize_t nwords = Py_SIZE(code);
    if (nwords < 0 || nwords > CAPACITY || code->_co_monitoring == NULL ||
        code->_co_monitoring->lines == NULL) {
        PyErr_SetString(PyExc_ValueError, "requires initialized LINE data of at most 128 words");
        return NULL;
    }
    uint16_t tokens[CAPACITY];
    int lines[CAPACITY];
    _PyCoLineInstrumentationData *data = code->_co_monitoring->lines;
    for (int i = 0; i < nwords; i++) {
        tokens[i] = _PyCode_GetOriginalOpcode(data, i);
        lines[i] = _Py_Instrumentation_GetLine(data, i);
    }
    PyObject *result = PyList_New(nwords);
    if (result == NULL) {
        return NULL;
    }
    for (int i = 0; i < nwords; i++) {
        PyObject *entry = Py_BuildValue("(ii)", lines[i], tokens[i]);
        if (entry == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, entry);
    }
    return result;
}

static PyMethodDef methods[] = {
    {"profile", profile, METH_VARARGS, NULL},
    {"inspect_lines", inspect_lines, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};
static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT, "_h3_allocation", NULL, -1, methods
};
PyMODINIT_FUNC PyInit__h3_allocation(void) { return PyModule_Create(&module); }
