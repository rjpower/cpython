#define Py_BUILD_CORE_MODULE
#include "Python.h"
#include "pycore_code.h"
#include "pycore_instruments.h"
static int
copied_safe_line(PyCodeObject *co, int addrq)
{
    if (addrq < 0) {
        return co->co_firstlineno;
    }
    if (!(addrq >= 0 && addrq < Py_SIZE(co))) {
        return -1;
    }
    /* This can run without an attached thread state. Keep it independent
       of lazily published monitoring storage. */
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(co, &bounds);
    return _PyCode_CheckLineNumber(addrq, &bounds);
}

static PyObject *lookup(PyObject *self, PyObject *args) {
    PyCodeObject *code;
    int offset;
    if (!PyArg_ParseTuple(args, "O!i", &PyCode_Type, &code, &offset)) return NULL;
    return PyLong_FromLong(copied_safe_line(code, offset));
}
static PyMethodDef methods[] = {
    {"lookup", lookup, METH_VARARGS}, {NULL, NULL, 0, NULL}
};
static struct PyModuleDef definition = {
    PyModuleDef_HEAD_INIT, "_h3_safe_line", NULL, -1, methods
};
PyMODINIT_FUNC PyInit__h3_safe_line(void) { return PyModule_Create(&definition); }
