/* Hooks for Lib/test/test_jit_memory.py: the JIT's code memory
   (Python/jit_memory.c). */

#include "parts.h"

#include "pycore_jit_memory.h"

#ifdef _PY_JIT_SUPPORTED

/* jit_memory_info() -> dict of the reservation and its use, or None if the
   code memory cannot be reserved. */
static PyObject *
jit_memory_info(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    if (_PyJitMemory_Init() < 0) {
        Py_RETURN_NONE;
    }
    _PyJitMemoryStats s;
    _PyJitMemory_GetStats(&s);
    return Py_BuildValue(
        "{s:K,s:K,s:K,s:K,s:K,s:K,s:K}",
        "reservation", (unsigned long long)s.reservation,
        "reservation_size", (unsigned long long)s.reservation_size,
        "text_low", (unsigned long long)s.text_low,
        "text_high", (unsigned long long)s.text_high,
        "pages_in_use", (unsigned long long)s.pages_in_use,
        "bytes_in_use", (unsigned long long)s.bytes_in_use,
        "page_size", (unsigned long long)s.page_size);
}

/* jit_memory_alloc(size) -> address, or None if there is no room.  The
   pages are writable and untouched. */
static PyObject *
jit_memory_alloc(PyObject *module, PyObject *size)
{
    size_t n = PyLong_AsSize_t(size);
    if (n == (size_t)-1 && PyErr_Occurred()) {
        return NULL;
    }
    _PyJitCode code;
    if (_PyJitMemory_Alloc(n, &code) < 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromUnsignedLongLong((uintptr_t)code.start);
}

static PyObject *
jit_memory_free(PyObject *module, PyObject *args)
{
    unsigned long long address;
    size_t size;
    if (!PyArg_ParseTuple(args, "Kn", &address, &size)) {
        return NULL;
    }
    _PyJitCode code = {(uint8_t *)(uintptr_t)address, size};
    _PyJitMemory_Free(&code);
    Py_RETURN_NONE;
}

/* jit_memory_place(code) -> (address, size), or None if placement fails: the
   bytes are copied into code memory, which is then sealed.  The code stays
   placed until jit_memory_free(). */
static PyObject *
jit_memory_place(PyObject *module, PyObject *bytes)
{
    Py_buffer view;
    if (PyObject_GetBuffer(bytes, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    _PyJitCode code;
    if (view.len == 0 || _PyJitMemory_Alloc((size_t)view.len, &code) < 0) {
        PyBuffer_Release(&view);
        Py_RETURN_NONE;
    }
    memcpy(code.start, view.buf, (size_t)view.len);
    PyBuffer_Release(&view);
    if (_PyJitMemory_Seal(&code) < 0) {
        _PyJitMemory_Free(&code);
        Py_RETURN_NONE;
    }
    return Py_BuildValue("(KK)", (unsigned long long)(uintptr_t)code.start,
                         (unsigned long long)code.size);
}

/* jit_memory_call(address) -> int: calls placed code as int64_t f(void). */
static PyObject *
jit_memory_call(PyObject *module, PyObject *address)
{
    unsigned long long a = PyLong_AsUnsignedLongLong(address);
    if (a == (unsigned long long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    int64_t (*f)(void) = (void *)(uintptr_t)a;
    return PyLong_FromLongLong(f());
}

/* jit_memory_text_address() -> int: the address of a function of the
   interpreter's text, where call rel32 reaches. */
static PyObject *
jit_memory_text_address(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLongLong((uintptr_t)&Py_HashPointer);
}

static PyMethodDef test_methods[] = {
    {"jit_memory_info", jit_memory_info, METH_NOARGS},
    {"jit_memory_alloc", jit_memory_alloc, METH_O},
    {"jit_memory_free", jit_memory_free, METH_VARARGS},
    {"jit_memory_place", jit_memory_place, METH_O},
    {"jit_memory_call", jit_memory_call, METH_O},
    {"jit_memory_text_address", jit_memory_text_address, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_JitMemory(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}

#else

int
_PyTestInternalCapi_Init_JitMemory(PyObject *module)
{
    return 0;
}

#endif  // _PY_JIT_SUPPORTED
