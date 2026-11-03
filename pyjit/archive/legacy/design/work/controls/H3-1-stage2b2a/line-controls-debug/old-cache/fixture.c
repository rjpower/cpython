#define Py_BUILD_CORE_MODULE
#include "Python.h"
#include "pycore_code.h"
#include "pycore_instruments.h"
#define NO_LINE (-2)
static int
compute_line(PyCodeObject *code, int line_delta)
{
    if (line_delta == NO_LINE) {
        return -1;
    }
    assert(line_delta > NO_LINE);
    return code->co_firstlineno + line_delta;
}
static inline int
get_line_delta(_PyCoLineInstrumentationData *line_data, int index)
{
    uint8_t *ptr = &line_data->data[index*line_data->bytes_per_entry+1];
    assert(line_data->bytes_per_entry >= 2);
    uint32_t value = *ptr;
    for (int idx = 2; idx < line_data->bytes_per_entry; idx++) {
        ptr++;
        int shift = (idx-1)*8;
        value |= ((uint32_t)(*ptr)) << shift;
    }
    assert(value < INT_MAX);
    /* NO_LINE is stored as zero. */
    return ((int)value) + NO_LINE;
}
static int
copied_monitor_line(PyCodeObject *code, _PyCoLineInstrumentationData *line_data, int index)
{
    assert(line_data != NULL);
    assert(index < Py_SIZE(code));
    int line_delta = get_line_delta(line_data, index);
    int line = compute_line(code, line_delta);
    return line;
}
static int
copied_safe_line(PyCodeObject *co, int addrq)
{
    if (addrq < 0) {
        return co->co_firstlineno;
    }
    if (co->_co_monitoring && co->_co_monitoring->lines) {
        return copied_monitor_line(co, co->_co_monitoring->lines, addrq);
    }
    if (!(addrq >= 0 && addrq < Py_SIZE(co))) {
        return -1;
    }
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
