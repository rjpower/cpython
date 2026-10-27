#include "parts.h"
#include "util.h"
#include <stdbool.h>

// Test PyModule_* API

/* unittest Cases that use these functions are in:
 * Lib/test/test_capi/test_module.py
 */

PyABIInfo_VAR(abi_info);

static PyValue
module_from_slots_empty(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_slots_minimal(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_slots_null(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(NULL, spec));
}

static PyValue
module_from_slots_name(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_name, "currently ignored..."),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_slots_doc(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_doc, "the docstring"),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_slots_size(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_SIZE(Py_mod_state_size, (void*)123),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(mod);
}

static PyValue
a_method(PyObject *self, PyValue arg_value)
{
    PyObject *arg = PyValue_Object(arg_value);
    if (arg == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyTuple_Pack(2, self, arg));
}

static PyMethodDef a_methoddef_array[] = {
    {"a_method", _PyCFunction_CAST(a_method), METH_O},
    {0},
};

static PyValue
module_from_slots_methods(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_STATIC_DATA(Py_mod_methods, a_methoddef_array),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static int noop_traverse(PyObject *self, visitproc visit, void *arg) {
    return 0;
}
static int noop_clear(PyObject *self) { return 0; }
static void noop_free(void *self) { }

static PyValue
module_from_slots_gc(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_FUNC(Py_mod_state_traverse, noop_traverse),
        PySlot_FUNC(Py_mod_state_clear, noop_clear),
        PySlot_FUNC(Py_mod_state_free, noop_free),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return PyValue_NULL;
    }
    if (PyModule_Add(mod, "traverse", PyLong_FromVoidPtr(&noop_traverse)) < 0) {
        Py_DECREF(mod);
        return PyValue_NULL;
    }
    if (PyModule_Add(mod, "clear", PyLong_FromVoidPtr(&noop_clear)) < 0) {
        Py_DECREF(mod);
        return PyValue_NULL;
    }
    if (PyModule_Add(mod, "free", PyLong_FromVoidPtr(&noop_free)) < 0) {
        Py_DECREF(mod);
        return PyValue_NULL;
    }
    return PyValue_FromResult(mod);
}

static const char test_token;

static PyValue
module_from_slots_token(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_token, (void*)&test_token),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return PyValue_NULL;
    }
    void *got_token;
    if (PyModule_GetToken(mod, &got_token) < 0) {
        Py_DECREF(mod);
        return PyValue_NULL;
    }
    assert(got_token == &test_token);
    assert(PyModule_GetToken_DuringGC(mod, &got_token) >= 0);
    assert(got_token == &test_token);
    return PyValue_FromResult(mod);
}

static int
simple_exec(PyObject *module)
{
    return PyModule_AddIntConstant(module, "a_number", 456);
}

static PyValue
module_from_slots_exec(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_FUNC(Py_mod_exec, simple_exec),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    PyObject *mod = PyModule_FromSlotsAndSpec(slots, spec);
    if (!mod) {
        return PyValue_NULL;
    }
    int res = PyObject_HasAttrStringWithError(mod, "a_number");
    if (res < 0) {
        Py_DECREF(mod);
        return PyValue_NULL;
    }
    assert(res == 0);
    if (PyModule_Exec(mod) < 0) {
        Py_DECREF(mod);
        return PyValue_NULL;
    }
    return PyValue_FromResult(mod);
}

static PyObject *
create_attr_from_spec(PyObject *spec, PyModuleDef *def)
{
    assert(!def);
    return PyObject_GetAttrString(spec, "_gimme_this");
}

static PyValue
module_from_slots_create(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    const PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_FUNC(Py_mod_create, create_attr_from_spec),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}


static int
slot_from_object(PyObject *obj)
{
    PyObject *slot_id_obj = PyObject_GetAttrString(obj, "_test_slot_id");
    if (slot_id_obj == NULL) {
        return -1;
    }
    int slot_id = PyLong_AsInt(slot_id_obj);
    if (PyErr_Occurred()) {
        return -1;
    }
    return slot_id;
}

static PyValue
module_from_slots_repeat_slot(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return PyValue_NULL;
    }
    const PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_PTR_STATIC(slot_id, "anything"),
        PySlot_PTR_STATIC(slot_id, "anything_else"),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_slots_null_slot(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return PyValue_NULL;
    }
    const PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_PTR_STATIC(slot_id, NULL),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_def_slot(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    int slot_id = slot_from_object(spec);
    if (slot_id < 0) {
        return PyValue_NULL;
    }
    PyModuleDef_Slot slots[] = {
        {slot_id, "anything"},
        {Py_mod_abi, &abi_info},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "currently ignored",
        .m_slots = slots,
    };
    // PyModuleDef is normally static; the real requirement is that it
    // must outlive its module.
    // Here, module creation fails, so it's fine on the stack.
    PyObject *result = PyModule_FromDefAndSpec(&def, spec);
    assert(result == NULL);
    return PyValue_FromResult(result);
}

static const char parrot_name[] = "test_capi/parrot";
static const char parrot_doc[] = "created from redundant information";
static PyModuleDef parrot_def = {
    PyModuleDef_HEAD_INIT,
    .m_name = (void*)parrot_name,
    .m_doc = (void*)parrot_doc,
    .m_size = 123,
    .m_methods = a_methoddef_array,
    .m_traverse = noop_traverse,
    .m_clear = noop_clear,
    .m_free = (void*)noop_free,
    .m_slots = NULL /* set below */,
};
static PyModuleDef_Slot parrot_slots[] = {
    {Py_mod_abi, &abi_info},
    {Py_mod_name, (void*)parrot_name},
    {Py_mod_doc, (void*)parrot_doc},
    {Py_mod_state_size, (void*)123},
    {Py_mod_methods, a_methoddef_array},
    {Py_mod_state_traverse, noop_traverse},
    {Py_mod_state_clear, noop_clear},
    {Py_mod_state_free, (void*)noop_free},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_token, &parrot_def},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0},
};

static PyValue
module_from_def_slot_parrot(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    parrot_def.m_slots = parrot_slots;
    PyObject *module = PyModule_FromDefAndSpec(&parrot_def, spec);
    if (!module || (PyModule_Exec(module) < 0)) {
        return PyValue_NULL;
    }
    Py_ssize_t size;
    assert(PyModule_GetStateSize(module, &size) == 0);
    assert(size == 123);
    PyModuleDef *def = PyModule_GetDef(module);
    assert(def == &parrot_def);
    return PyValue_FromResult(module);
}

static PyValue
module_from_bad_abiinfo(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PyABIInfo bad_abi_info = {
        1, 0,
        .abi_version=0x02080000,
    };
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_abi, &bad_abi_info),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static PyValue
module_from_multiple_abiinfo(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    PyABIInfo extra_abi_info = {
        1, 0,
        .flags=PyABIInfo_STABLE | PyABIInfo_FREETHREADING_AGNOSTIC,
        .abi_version=0x03040000,
    };
    PySlot slots[] = {
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_abi, &abi_info),
        PySlot_DATA(Py_mod_abi, &extra_abi_info),
        PySlot_DATA(Py_mod_abi, &extra_abi_info),
        PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),
        PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),
        PySlot_END,
    };
    return PyValue_FromResult(PyModule_FromSlotsAndSpec(slots, spec));
}

static int
another_exec(PyObject *module)
{
    /* Make sure simple_exec was called */
    assert(PyObject_HasAttrString(module, "a_number"));

    /* Add or negate a global called 'another_number'  */
    PyObject *another_number;
    if (PyObject_GetOptionalAttrString(module, "another_number",
                                       &another_number) < 0) {
        return -1;
    }
    if (!another_number) {
        return PyModule_AddIntConstant(module, "another_number", 789);
    }
    PyObject *neg_number = PyNumber_Negative(another_number);
    Py_DECREF(another_number);
    if (!neg_number) {
        return -1;
    }
    int result = PyObject_SetAttrString(module, "another_number",
                                        neg_number);
    Py_DECREF(neg_number);
    return result;
}

static PyValue
module_from_def_multiple_exec(PyObject *self, PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    static PyModuleDef_Slot slots[] = {
        {Py_mod_abi, &abi_info},
        {Py_mod_exec, simple_exec},
        {Py_mod_exec, another_exec},
        {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    static PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "currently ignored",
        .m_slots = slots,
    };
    return PyValue_FromResult(PyModule_FromDefAndSpec(&def, spec));
}

static PyValue
pymodule_exec(PyObject *self, PyValue module_value)
{
    PyObject *module = PyValue_Object(module_value);
    if (module == NULL) {
        return PyValue_NULL;
    }
    if (PyModule_Exec(module) < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(Py_None);
}

static PyValue
pymodule_get_token(PyObject *self, PyValue module_value)
{
    PyObject *module = PyValue_Object(module_value);
    if (module == NULL) {
        return PyValue_NULL;
    }
    void *token;
    int res = PyModule_GetToken(module, &token);
    void *token_duringgc;
    int res_duringgc = PyModule_GetToken_DuringGC(module, &token_duringgc);
    assert(res == res_duringgc);
    assert(token == token_duringgc);
    if (res < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromVoidPtr(token));
}

static PyValue
pymodule_get_def(PyObject *self, PyValue module_value)
{
    PyObject *module = PyValue_Object(module_value);
    if (module == NULL) {
        return PyValue_NULL;
    }
    PyModuleDef *def = PyModule_GetDef(module);
    if (!def && PyErr_Occurred()) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromVoidPtr(def));
}

static PyValue
pymodule_get_state_size(PyObject *self, PyValue module_value)
{
    PyObject *module = PyValue_Object(module_value);
    if (module == NULL) {
        return PyValue_NULL;
    }
    Py_ssize_t size;
    if (PyModule_GetStateSize(module, &size) < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromSsize_t(size));
}

static PyObject *
module_from_null_def_slot(PyObject* Py_UNUSED(module), PyObject *args)
{
    long slot_number;
    PyObject *spec;
    if (!PyArg_ParseTuple(args, "lO", &slot_number, &spec)) {
        return NULL;
    }
    static PyModuleDef_Slot slots[] = {
        {0, NULL},
        {0},
    };
    static PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "mymod",
        .m_slots = slots,
    };
    // hack: def is supposed to be constant.
    // Don't do this at home; use PyModule_FromSlotsAndSpec throwaway
    // definitions!
    slots[0].slot = slot_number;
    return PyModule_FromDefAndSpec(&def, spec);
}

static PyValue
module_from_def_nonstatic_nested(PyObject* Py_UNUSED(module), PyValue spec_value)
{
    PyObject *spec = PyValue_Object(spec_value);
    if (spec == NULL) {
        return PyValue_NULL;
    }
    static PyModuleDef_Slot subsubslots[] = {
        {Py_mod_gil, Py_MOD_GIL_NOT_USED},
        {0},
    };
    static PySlot subslots[] = {
        PySlot_DATA(Py_mod_slots, subsubslots),
        PySlot_END,
    };
    static PyModuleDef_Slot slots[] = {
        {Py_slot_subslots, subslots},
        {0},
    };
    static PyModuleDef def = {
        PyModuleDef_HEAD_INIT,
        .m_name = "mymod",
        .m_slots = slots,
    };
    return PyValue_FromResult(PyModule_FromDefAndSpec(&def, spec));
}

static PyMethodDef test_methods[] = {
    {"module_from_slots_empty", _PyCFunction_CAST(module_from_slots_empty), METH_O},
    {"module_from_slots_minimal", _PyCFunction_CAST(module_from_slots_minimal), METH_O},
    {"module_from_slots_null", _PyCFunction_CAST(module_from_slots_null), METH_O},
    {"module_from_slots_name", _PyCFunction_CAST(module_from_slots_name), METH_O},
    {"module_from_slots_doc", _PyCFunction_CAST(module_from_slots_doc), METH_O},
    {"module_from_slots_size", _PyCFunction_CAST(module_from_slots_size), METH_O},
    {"module_from_slots_methods", _PyCFunction_CAST(module_from_slots_methods), METH_O},
    {"module_from_slots_gc", _PyCFunction_CAST(module_from_slots_gc), METH_O},
    {"module_from_slots_token", _PyCFunction_CAST(module_from_slots_token), METH_O},
    {"module_from_slots_exec", _PyCFunction_CAST(module_from_slots_exec), METH_O},
    {"module_from_slots_create", _PyCFunction_CAST(module_from_slots_create), METH_O},
    {"module_from_slots_repeat_slot", _PyCFunction_CAST(module_from_slots_repeat_slot), METH_O},
    {"module_from_slots_null_slot", _PyCFunction_CAST(module_from_slots_null_slot), METH_O},
    {"module_from_def_multiple_exec", _PyCFunction_CAST(module_from_def_multiple_exec), METH_O},
    {"module_from_def_slot", _PyCFunction_CAST(module_from_def_slot), METH_O},
    {"module_from_def_slot_parrot", _PyCFunction_CAST(module_from_def_slot_parrot), METH_O},
    {"module_from_bad_abiinfo", _PyCFunction_CAST(module_from_bad_abiinfo), METH_O},
    {"module_from_multiple_abiinfo", _PyCFunction_CAST(module_from_multiple_abiinfo), METH_O},
    {"pymodule_get_token", _PyCFunction_CAST(pymodule_get_token), METH_O},
    {"pymodule_get_def", _PyCFunction_CAST(pymodule_get_def), METH_O},
    {"pymodule_get_state_size", _PyCFunction_CAST(pymodule_get_state_size), METH_O},
    {"pymodule_exec", _PyCFunction_CAST(pymodule_exec), METH_O},
    {"module_from_null_def_slot", module_from_null_def_slot, METH_VARARGS},
    {"module_from_def_nonstatic_nested", _PyCFunction_CAST(module_from_def_nonstatic_nested), METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Module(PyObject *m)
{
#define ADD_INT_MACRO(C) if (PyModule_AddIntConstant(m, #C, C) < 0) return -1;
    ADD_INT_MACRO(Py_mod_create);
    ADD_INT_MACRO(Py_mod_exec);
    ADD_INT_MACRO(Py_mod_multiple_interpreters);
    ADD_INT_MACRO(Py_mod_gil);
    ADD_INT_MACRO(Py_mod_name);
    ADD_INT_MACRO(Py_mod_doc);
    ADD_INT_MACRO(Py_mod_state_size);
    ADD_INT_MACRO(Py_mod_methods);
    ADD_INT_MACRO(Py_mod_state_traverse);
    ADD_INT_MACRO(Py_mod_state_clear);
    ADD_INT_MACRO(Py_mod_state_free);
    ADD_INT_MACRO(Py_mod_token);
#undef ADD_INT_MACRO
    if (PyModule_Add(m, "module_test_token",
                     PyLong_FromVoidPtr((void*)&test_token)) < 0)
    {
        return -1;
    }
    return PyModule_AddFunctions(m, test_methods);
}
