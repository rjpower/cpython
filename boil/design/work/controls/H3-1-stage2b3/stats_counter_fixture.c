/* Task-only primitive counter fixture; no Python code runs while intercepted. */
#define Py_BUILD_CORE
#define Py_BUILD_CORE_MODULE
#include "Python.h"
#include "pycore_ir.h"
#include "pycore_tstate.h"      /* _PyThreadStateImpl declarations in stats. */
#include "pycore_stats.h"
#include <stdint.h>
#include <string.h>

#include "counter_macro.h"       /* Exact current or historical macro span. */
#include "old_stats_layout.h"    /* Historical typedefs, renamed, unchanged fields. */
#include "predicted_macros.h"    /* Actual normal-mode label/jump definitions. */

static int
new_activation(void)
{
    int lastopcode = 0;
    INSTRUCTION_STATS(LOAD_CONST);
    return lastopcode;
}

static void
predicted_route(void)
{
    int lastopcode = 0;
    int passes = 0;
    /* This is the adjacent counter/label span from generated LOAD_CONST. */
#include "predicted_span.h"
    if (++passes == 1) {
        JUMP_TO_PREDICTED(LOAD_CONST);
    }
}

static PyObject *
probe(PyObject *self, PyObject *ignored)
{
    (void)self;
    (void)ignored;
    static const char *names[] = {
        "sizeof_stats", "sizeof_old_stats", "sizeof_opcode", "sizeof_old_opcode",
        "allocation_succeeded", "allocation_zeroed", "domain", "low_id", "high_id", "nofb_id",
        "pre.low_entries", "pre.high_entries", "pre.nofb_entries", "pre.initial_pair",
        "pre.low_high_pair", "pre.high_nofb_pair", "pre.nofb_low_pair", "pre.last",
        "post.low_entries", "post.high_entries", "post.nofb_entries", "post.initial_pair",
        "post.low_high_pair", "post.high_nofb_pair", "post.nofb_high_pair",
        "post.outer_last", "post.inner_last", "predicted.entries", "predicted.self_pair"
    };
    uint64_t values[sizeof(names) / sizeof(names[0])] = {
        sizeof(PyStats), sizeof(OldPyStats), sizeof(OpcodeStats), sizeof(OldOpcodeStats),
        1, 1, IR_NUM_HANDLERS, LOAD_CONST, IR_JUMP, LOAD_ATTR_NOFB
    };
    PyStats *private = PyMem_RawCalloc(1, sizeof(PyStats));
    if (private == NULL) {
        return PyErr_NoMemory();
    }
    for (size_t i = 0; i < sizeof(PyStats); i++) {
        if (((unsigned char *)private)[i] != 0) {
            values[5] = 0;
        }
    }
    PyThreadState *tstate = PyThreadState_Get();
    PyStats *saved = tstate->pystats;
    int lastopcode = 0;

    /* No allocation, Python call, error return or thread detachment until
       the saved pointer is restored. The private buffer has no PyObjects. */
    tstate->pystats = private;
    INSTRUCTION_STATS(LOAD_CONST);
    INSTRUCTION_STATS(IR_JUMP);
    tstate->pystats = NULL;
    INSTRUCTION_STATS(LOAD_ATTR_NOFB);
    tstate->pystats = private;
    INSTRUCTION_STATS(LOAD_CONST);
    values[10] = private->opcode_stats[LOAD_CONST].execution_count;
    values[11] = private->opcode_stats[IR_JUMP].execution_count;
    values[12] = private->opcode_stats[LOAD_ATTR_NOFB].execution_count;
    values[13] = private->opcode_stats[0].pair_count[LOAD_CONST];
    values[14] = private->opcode_stats[LOAD_CONST].pair_count[IR_JUMP];
    values[15] = private->opcode_stats[IR_JUMP].pair_count[LOAD_ATTR_NOFB];
    values[16] = private->opcode_stats[LOAD_ATTR_NOFB].pair_count[LOAD_CONST];
    values[17] = lastopcode;

    /* The primitive clear mirrors stats_zero_all's memset, without invoking
       Python while intercepted. It leaves this activation's local intact. */
    memset(private, 0, sizeof(*private));
    INSTRUCTION_STATS(IR_JUMP);
    INSTRUCTION_STATS(LOAD_ATTR_NOFB);  /* Model same-activation frame change. */
    int inner_last = new_activation();
    INSTRUCTION_STATS(IR_JUMP);        /* Outer local survived the nested call. */
    values[18] = private->opcode_stats[LOAD_CONST].execution_count;
    values[19] = private->opcode_stats[IR_JUMP].execution_count;
    values[20] = private->opcode_stats[LOAD_ATTR_NOFB].execution_count;
    values[21] = private->opcode_stats[0].pair_count[LOAD_CONST];
    values[22] = private->opcode_stats[LOAD_CONST].pair_count[IR_JUMP];
    values[23] = private->opcode_stats[IR_JUMP].pair_count[LOAD_ATTR_NOFB];
    values[24] = private->opcode_stats[LOAD_ATTR_NOFB].pair_count[IR_JUMP];
    values[25] = lastopcode;
    values[26] = inner_last;
    memset(private, 0, sizeof(*private));
    predicted_route();
    values[27] = private->opcode_stats[LOAD_CONST].execution_count;
    values[28] = private->opcode_stats[LOAD_CONST].pair_count[LOAD_CONST];
    tstate->pystats = saved;
    PyMem_RawFree(private);

    PyObject *result = PyDict_New();
    if (result == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        PyObject *value = PyLong_FromUnsignedLongLong(values[i]);
        if (value == NULL || PyDict_SetItemString(result, names[i], value) < 0) {
            Py_XDECREF(value);
            Py_DECREF(result);
            return NULL;
        }
        Py_DECREF(value);
    }
    return result;
}

static PyMethodDef methods[] = {
    {"probe", probe, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};
static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT, "_stats_counter_fixture", NULL, -1, methods
};
PyMODINIT_FUNC
PyInit__stats_counter_fixture(void)
{
    return PyModule_Create(&module);
}
