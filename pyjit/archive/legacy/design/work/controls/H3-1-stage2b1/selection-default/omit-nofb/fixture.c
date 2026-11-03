#define Py_BUILD_CORE_MODULE
#include "Python.h"
#include "pycore_ir.h"
static int
copied_handler_for(PyCodeObject *co, int i, int opcode)
{
    int handler = _PyCode_InstrHandler(co, i);
    if (_PyIR_BaseOpcode(handler) == opcode) {
        return handler;
    }
    return opcode;
}

static PyObject *select_word(PyObject *self, PyObject *args) {
    PyCodeObject *co;
    int index, installed, restored;
    if (!PyArg_ParseTuple(args, "O!iii", &PyCode_Type, &co,
                          &index, &installed, &restored)) return NULL;
    _PyIRWord word = ((const _PyIRWord *)co->co_words)[index];
    if (!_PyIR_SetHandler(&word, copied_handler_for(co, index, installed))) {
        PyErr_SetString(PyExc_AssertionError, "install rejected");
        return NULL;
    }
    unsigned long long first = word;
    if (!_PyIR_SetHandler(&word, copied_handler_for(co, index, restored))) {
        PyErr_SetString(PyExc_AssertionError, "restore rejected");
        return NULL;
    }
    return Py_BuildValue("KK", first, (unsigned long long)word);
}
static PyMethodDef methods[] = {
    {"select", select_word, METH_VARARGS}, {NULL, NULL, 0, NULL}
};
static struct PyModuleDef definition = {
    PyModuleDef_HEAD_INIT, "_h3_selection", NULL, -1, methods
};
PyMODINIT_FUNC PyInit__h3_selection(void) {
    return PyModule_Create(&definition);
}
