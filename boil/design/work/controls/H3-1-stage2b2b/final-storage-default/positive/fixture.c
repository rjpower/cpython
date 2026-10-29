#define Py_BUILD_CORE_MODULE
#include "Python.h"
#include "pycore_instruments.h"
#include "pycore_ir.h"
static PyObject *
monitoring_storage_roundtrip(PyObject *self, PyObject *args)
{
    int index, opcode, line;
    if (!PyArg_ParseTuple(args, "iii:monitoring_storage_roundtrip",
                          &index, &opcode, &line)) {
        return NULL;
    }
    uint16_t originals[3] = {0};
    const int nwords = Py_ARRAY_LENGTH(originals);
    if (index < 0 || index >= nwords || opcode < 0 || opcode > UINT16_MAX ||
        line < -1) {
        PyErr_SetString(PyExc_ValueError, "invalid monitoring storage input");
        return NULL;
    }
    struct {
        _PyCoLineInstrumentationData lines;
        int extra[Py_ARRAY_LENGTH(originals)];
    } storage = {0};
    uint16_t tokens[Py_ARRAY_LENGTH(originals)] = {255, 511, UINT16_MAX};
    int lines[Py_ARRAY_LENGTH(originals)] = {-1, 0, INT_MAX};
    tokens[index] = (uint16_t)opcode;
    lines[index] = line;
    storage.lines.original_opcodes = originals;
    for (int i = 0; i < nwords; i++) {
        _PyCode_SetOriginalOpcode(&storage.lines, i, tokens[i]);
        _PyCode_SetMonitoringLine(&storage.lines, i, lines[i]);
    }
    return Py_BuildValue("((ii)(ii)(ii))",
                         _PyCode_GetOriginalOpcode(&storage.lines, 0),
                         _Py_Instrumentation_GetLine(&storage.lines, 0),
                         _PyCode_GetOriginalOpcode(&storage.lines, 1),
                         _Py_Instrumentation_GetLine(&storage.lines, 1),
                         _PyCode_GetOriginalOpcode(&storage.lines, 2),
                         _Py_Instrumentation_GetLine(&storage.lines, 2));
}

static int
copied_deinstrument(int opcode)
{
    if (!_PyIR_HasProperty(opcode, _PyIR_H_METADATA)) {
        return -1;
    }
    return _PyIR_Deinstrument[_PyIR_BaseOpcode(opcode)];
}

static PyObject *deinstrument_wrapper(PyObject *self, PyObject *args) {
    int opcode;
    if (!PyArg_ParseTuple(args, "i", &opcode)) { return NULL; }
    return PyLong_FromLong(copied_deinstrument(opcode));
}
static PyMethodDef methods[] = {
    {"roundtrip", monitoring_storage_roundtrip, METH_VARARGS, NULL},
    {"deinstrument", deinstrument_wrapper, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};
static struct PyModuleDef module = {PyModuleDef_HEAD_INIT, "_h3_storage", NULL, -1, methods};
PyMODINIT_FUNC PyInit__h3_storage(void) { return PyModule_Create(&module); }
