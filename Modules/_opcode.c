#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "Python.h"
#include "compile.h"
#include "opcode.h"
#include "pycore_ceval.h"           // SPECIAL_MAX
#include "pycore_code.h"
#include "pycore_compile.h"
#include "pycore_intrinsics.h"
#include "pycore_ir.h"              // _PyIR_Formats
#include "pycore_ir_exception.h"    // shared semantic container decoder
#include "pycore_list.h"            // PyList_GET_VALUE()
#include "pycore_opcode_metadata.h" // IS_VALID_OPCODE, OPCODE_HAS_*, etc
#include "pycore_opcode_utils.h"
#include "pycore_long_value.h"      // _PyValue_AsLong()
#include "pycore_value.h"           // PyValue_IsNull()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()

typedef struct {
    PyTypeObject *ir_contract_type;     // _opcode.IRContract
} opcode_state;

static inline opcode_state *
get_opcode_state(PyObject *module)
{
    return (opcode_state *)PyModule_GetState(module);
}

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _opcode
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=117442e66eb376e6]*/

#include "clinic/_opcode.c.h"

/*[clinic input]

_opcode.stack_effect -> int

  opcode: int
  oparg: object = None
  /
  *
  jump: object = None

Compute the stack effect of the opcode.
[clinic start generated code]*/

static int
_opcode_stack_effect_impl(PyObject *module, int opcode, PyValueBorrow oparg,
                          PyValueBorrow jump)
/*[clinic end generated code: output=1208a7277aac7df2 input=461c9d4a44851898]*/
{
    int oparg_int = 0;
    int jump_int;

    if (!PyValue_IsNone(oparg)) {
        oparg_int = (int)_PyValue_AsLong(oparg);
        if ((oparg_int == -1) && PyErr_Occurred()) {
            return -1;
        }
    }

    if (PyValue_IsNone(jump)) {
        jump_int = -1;
    }
    else if (PyValue_IsTrue(jump)) {
        jump_int = 1;
    }
    else if (PyValue_IsFalse(jump)) {
        jump_int = 0;
    }
    else {
        PyErr_SetString(PyExc_ValueError,
                "stack_effect: jump must be False, True or None");
        return -1;
    }
    int effect = PyCompile_OpcodeStackEffectWithJump(opcode, oparg_int, jump_int);
    if (effect == PY_INVALID_STACK_EFFECT) {
        PyErr_SetString(PyExc_ValueError, "invalid opcode or oparg");
        return -1;
    }
    return effect;
}

/*[clinic input]

_opcode.is_valid -> bool

  opcode: int

Return True if opcode is valid, False otherwise.
[clinic start generated code]*/

static int
_opcode_is_valid_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=b0d918ea1d073f65 input=fe23e0aa194ddae0]*/
{
    return IS_VALID_OPCODE(opcode);
}

/*[clinic input]

_opcode.has_arg -> bool

  opcode: int

Return True if the opcode uses its oparg, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_arg_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=7a062d3b2dcc0815 input=93d878ba6361db5f]*/
{
    return IS_VALID_OPCODE(opcode) && OPCODE_HAS_ARG(opcode);
}

/*[clinic input]

_opcode.has_const -> bool

  opcode: int

Return True if the opcode accesses a constant, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_const_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=c646d5027c634120 input=a6999e4cf13f9410]*/
{
    return IS_VALID_OPCODE(opcode) && OPCODE_HAS_CONST(opcode);
}

/*[clinic input]
@permit_long_summary
_opcode.has_name -> bool

  opcode: int

Return True if the opcode accesses an attribute by name, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_name_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=b49a83555c2fa517 input=8faf669024d97fad]*/
{
    return IS_VALID_OPCODE(opcode) && OPCODE_HAS_NAME(opcode);
}

/*[clinic input]

_opcode.has_jump -> bool

  opcode: int

Return True if the opcode has a jump target, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_jump_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=e9c583c669f1c46a input=35f711274357a0c3]*/
{
    return IS_VALID_OPCODE(opcode) && OPCODE_HAS_JUMP(opcode);
}

/*[clinic input]

_opcode.has_free -> bool

  opcode: int

Return True if the opcode accesses a free variable, False otherwise.

Note that 'free' in this context refers to names in the current scope
that are referenced by inner scopes or names in outer scopes that are
referenced from this scope. It does not include references to global
or builtin scopes.
[clinic start generated code]*/

static int
_opcode_has_free_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=d81ae4d79af0ee26 input=117dcd5c19c1139b]*/
{
    return IS_VALID_OPCODE(opcode) && OPCODE_HAS_FREE(opcode);
}

/*[clinic input]

_opcode.has_local -> bool

  opcode: int

Return True if the opcode accesses a local variable, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_local_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=da5a8616b7a5097b input=9a798ee24aaef49d]*/
{
    return IS_VALID_OPCODE(opcode) && OPCODE_HAS_LOCAL(opcode);
}

/*[clinic input]

_opcode.has_exc -> bool

  opcode: int

Return True if the opcode sets an exception handler, False otherwise.
[clinic start generated code]*/

static int
_opcode_has_exc_impl(PyObject *module, int opcode)
/*[clinic end generated code: output=41b68dff0ec82a52 input=db0e4bdb9bf13fa5]*/
{
    return IS_VALID_OPCODE(opcode) && IS_BLOCK_PUSH_OPCODE(opcode);
}

/*[clinic input]

_opcode.get_specialization_stats

Return the specialization stats
[clinic start generated code]*/

static PyValue
_opcode_get_specialization_stats_impl(PyObject *module)
/*[clinic end generated code: output=dc0b106c2fcef79c input=e1f60db68d8ce5f6]*/
{
#ifdef Py_STATS
    return _PyValue_FromObjectResult(_Py_GetSpecializationStats());
#else
    return PyValue_None;
#endif
}

/*[clinic input]

_opcode.get_nb_ops

Return array of symbols of binary ops.

Indexed by the BINARY_OP oparg value.
[clinic start generated code]*/

static PyValue
_opcode_get_nb_ops_impl(PyObject *module)
/*[clinic end generated code: output=1dca136d0a389e92 input=9462fc544c823176]*/
{
    PyObject *list = PyList_New(NB_OPARG_LAST + 1);
    if (list == NULL) {
        return PyValue_NULL;
    }
#define ADD_NB_OP(NUM, STR) \
    do { \
        PyObject *pair = Py_BuildValue("ss", #NUM, STR); \
        if (pair == NULL) { \
            Py_DECREF(list); \
            return PyValue_NULL; \
        } \
        PyList_SET_VALUE(list, (NUM), PyValue_PrepareOwned(_PyObject_CAST(pair))); \
    } while(0);

    ADD_NB_OP(NB_ADD, "+");
    ADD_NB_OP(NB_AND, "&");
    ADD_NB_OP(NB_FLOOR_DIVIDE, "//");
    ADD_NB_OP(NB_LSHIFT, "<<");
    ADD_NB_OP(NB_MATRIX_MULTIPLY, "@");
    ADD_NB_OP(NB_MULTIPLY, "*");
    ADD_NB_OP(NB_REMAINDER, "%");
    ADD_NB_OP(NB_OR, "|");
    ADD_NB_OP(NB_POWER, "**");
    ADD_NB_OP(NB_RSHIFT, ">>");
    ADD_NB_OP(NB_SUBTRACT, "-");
    ADD_NB_OP(NB_TRUE_DIVIDE, "/");
    ADD_NB_OP(NB_XOR, "^");
    ADD_NB_OP(NB_INPLACE_ADD, "+=");
    ADD_NB_OP(NB_INPLACE_AND, "&=");
    ADD_NB_OP(NB_INPLACE_FLOOR_DIVIDE, "//=");
    ADD_NB_OP(NB_INPLACE_LSHIFT, "<<=");
    ADD_NB_OP(NB_INPLACE_MATRIX_MULTIPLY, "@=");
    ADD_NB_OP(NB_INPLACE_MULTIPLY, "*=");
    ADD_NB_OP(NB_INPLACE_REMAINDER, "%=");
    ADD_NB_OP(NB_INPLACE_OR, "|=");
    ADD_NB_OP(NB_INPLACE_POWER, "**=");
    ADD_NB_OP(NB_INPLACE_RSHIFT, ">>=");
    ADD_NB_OP(NB_INPLACE_SUBTRACT, "-=");
    ADD_NB_OP(NB_INPLACE_TRUE_DIVIDE, "/=");
    ADD_NB_OP(NB_INPLACE_XOR, "^=");
    ADD_NB_OP(NB_SUBSCR, "[]");

#undef ADD_NB_OP

    for(int i = 0; i <= NB_OPARG_LAST; i++) {
        if (PyValue_IsNull(PyList_GET_VALUE(list, i))) {
            Py_DECREF(list);
            PyErr_Format(PyExc_ValueError,
                         "Missing initialization for NB_OP %d",
                         i);
            return PyValue_NULL;
        }
    }
    return _PyValue_FromObjectResult(list);
}

/*[clinic input]

_opcode.get_intrinsic1_descs

Return a list of names of the unary intrinsics.
[clinic start generated code]*/

static PyValue
_opcode_get_intrinsic1_descs_impl(PyObject *module)
/*[clinic end generated code: output=70223d058698c63f input=13b51c712618459b]*/
{
    PyObject *list = PyList_New(MAX_INTRINSIC_1 + 1);
    if (list == NULL) {
        return PyValue_NULL;
    }
    for (int i=0; i <= MAX_INTRINSIC_1; i++) {
        PyObject *name = _PyCompile_GetUnaryIntrinsicName(i);
        if (name == NULL) {
            Py_DECREF(list);
            return PyValue_NULL;
        }
        PyList_SET_VALUE(list, i, PyValue_PrepareOwned(name));
    }
    return _PyValue_FromObjectResult(list);
}


/*[clinic input]

_opcode.get_intrinsic2_descs

Return a list of names of the binary intrinsics.
[clinic start generated code]*/

static PyValue
_opcode_get_intrinsic2_descs_impl(PyObject *module)
/*[clinic end generated code: output=baee7177a6d2d600 input=e83068f249f5471b]*/
{
    PyObject *list = PyList_New(MAX_INTRINSIC_2 + 1);
    if (list == NULL) {
        return PyValue_NULL;
    }
    for (int i=0; i <= MAX_INTRINSIC_2; i++) {
        PyObject *name = _PyCompile_GetBinaryIntrinsicName(i);
        if (name == NULL) {
            Py_DECREF(list);
            return PyValue_NULL;
        }
        PyList_SET_VALUE(list, i, PyValue_PrepareOwned(name));
    }
    return _PyValue_FromObjectResult(list);
}

/*[clinic input]

_opcode.get_special_method_names

Return a list of special method names.
[clinic start generated code]*/

static PyValue
_opcode_get_special_method_names_impl(PyObject *module)
/*[clinic end generated code: output=c513a3d7e8fb6c64 input=25f2115560bdf163]*/
{
    PyObject *list = PyList_New(SPECIAL_MAX + 1);
    if (list == NULL) {
        return PyValue_NULL;
    }
    for (int i=0; i <= SPECIAL_MAX; i++) {
        PyObject *name = _Py_SpecialMethods[i].name;
        if (name == NULL) {
            Py_DECREF(list);
            return PyValue_NULL;
        }
        PyList_SET_VALUE(list, i, PyValue_PrepareOwned(name));
    }
    return _PyValue_FromObjectResult(list);
}

/* The fields of an _opcode.IRContract, in the order of _PyIRFormat, with
 * base after shadow */
static PyStructSequence_Field ir_contract_fields[] = {
    {"flags", "_PyIR_VALID, ...: the format"},
    {"effects", "_PyIR_RAISES, ...: the handler"},
    {"roles", "the roles of fields A to D"},
    {"imm", "where the oparg lives, an IR_LOC_* constant"},
    {"fb", "where the feedback index lives"},
    {"ninputs", "the inputs of a register-style handler"},
    {"dst", "the slot of dst relative to the stack, or None"},
    {"borrow_mask", "bit k: input k may name a borrowed local"},
    {"fb_kind", "the kind of the feedback entry"},
    {"fb_size", "the size of the feedback entry, or 0"},
    {"result_phase", "the first output's conditional publication phase"},
    {"output_kind", "the handler's physical output publication mapping"},
    {"reads", "the stack inputs, a (k, form) extent"},
    {"writes", "the stack outputs, a (k, form) extent"},
    {"through", "the deepest inputs left in place, a (k, form) extent"},
    {"shadow", "the instruction that the handler stands for"},
    {"base", "the opcode that the handler stands for"},
    {"twin", "the handler that instrumentation runs, or None"},
    {"events", "bit e: the semantic handler or its twin can fire monitoring event e"},
    {"call_input_holes", "bit k: CALL window input k is virtual NULL and is not read"},
    {"error_routes", "complete exceptional routes, including conditional bridges and local publication"},
    {NULL}
};

static PyStructSequence_Desc ir_contract_desc = {
    "_opcode.IRContract",
    "The instruction contract of a handler of the register IR: _PyIRFormat\n"
    "in Include/internal/pycore_ir.h, which documents the fields.",
    ir_contract_fields,
    Py_ARRAY_LENGTH(ir_contract_fields) - 1,
};

static PyObject *
handler_or_none(int h)
{
    return h == 0 ? Py_NewRef(Py_None) : PyLong_FromLong(h);
}

static PyObject *
ir_contract_row(PyTypeObject *type, int h)
{
    const _PyIRFormat *fmt = &_PyIR_Formats[h];
    PyObject *row = PyStructSequence_New(type);
    if (row == NULL) {
        return NULL;
    }
    PyObject *items[] = {
        PyLong_FromLong(fmt->flags),
        PyLong_FromLong(fmt->effects),
        Py_BuildValue("(iiii)", fmt->roles[0], fmt->roles[1], fmt->roles[2],
                      fmt->roles[3]),
        PyLong_FromLong(fmt->imm),
        PyLong_FromLong(fmt->fb),
        PyLong_FromLong(fmt->ninputs),
        fmt->dst == _PyIR_NO_DST ? Py_NewRef(Py_None)
                                 : PyLong_FromLong(fmt->dst),
        PyLong_FromLong(fmt->borrow_mask),
        PyLong_FromLong(fmt->fb_kind),
        PyLong_FromLong(fmt->fb_size),
        PyLong_FromLong(fmt->result_phase),
        PyLong_FromLong(fmt->output_kind),
        Py_BuildValue("(ii)", fmt->reads.k, fmt->reads.form),
        Py_BuildValue("(ii)", fmt->writes.k, fmt->writes.form),
        Py_BuildValue("(ii)", fmt->through.k, fmt->through.form),
        PyLong_FromLong(fmt->shadow),
        PyLong_FromLong(_PyIR_BaseOpcode(h)),
        handler_or_none(fmt->twin),
        PyLong_FromUnsignedLong(fmt->events),
        PyLong_FromLong(fmt->call_input_holes),
        PyLong_FromLong(fmt->error_routes),
    };
    static_assert(Py_ARRAY_LENGTH(items)
                  == Py_ARRAY_LENGTH(ir_contract_fields) - 1,
                  "a value for each field");
    int failed = 0;
    for (Py_ssize_t i = 0; i < (Py_ssize_t)Py_ARRAY_LENGTH(items); i++) {
        failed |= items[i] == NULL;
        PyStructSequence_SET_VALUE(row, i, PyValue_PrepareOwned(items[i]));
    }
    if (failed) {
        Py_DECREF(row);
        return NULL;
    }
    return row;
}

/*[clinic input]

_opcode.get_ir_contract

Return the instruction contract of the handlers of the register IR.

Item h describes handler h: None if there is none, else an IRContract,
whose fields are those of _PyIRFormat in Include/internal/pycore_ir.h,
which documents them, and base, the opcode that the handler stands for.
roles holds the roles of fields A to D; reads, writes and through are
(k, form) extents; dst is None for a format without one; twin
is a handler or None.
[clinic start generated code]*/

static PyValue
_opcode_get_ir_contract_impl(PyObject *module)
/*[clinic end generated code: output=cad532d9315c6dc9 input=0fe500acc87ed1d2]*/
{
    PyTypeObject *type = get_opcode_state(module)->ir_contract_type;
    PyObject *contract = PyTuple_New(IR_NUM_HANDLERS);
    if (contract == NULL) {
        return PyValue_NULL;
    }
    for (int h = 0; h < IR_NUM_HANDLERS; h++) {
        PyObject *item = (_PyIR_Formats[h].flags & _PyIR_VALID)
                         ? ir_contract_row(type, h) : Py_NewRef(Py_None);
        if (item == NULL) {
            Py_DECREF(contract);
            return PyValue_NULL;
        }
        PyTuple_SET_VALUE(contract, h, PyValue_PrepareOwned(item));
    }
    return _PyValue_FromObjectResult(contract);
}

/*[clinic input]

_opcode.get_exception_entries

    code: object(type="PyCodeObject *", subclass_of="&PyCode_Type")
    /

Return admitted (start, end, target, slot, lasti) exception entries.
[clinic start generated code]*/

static PyValue
_opcode_get_exception_entries_impl(PyObject *module, PyCodeObject *code)
/*[clinic end generated code: output=05d34b778bd6dfb2 input=2edb896bcdc0d4be]*/
{
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(code->co_exceptiontable);
    const unsigned char *p = semantic.exceptions;
    const unsigned char *end = p + semantic.exceptions_size;
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return PyValue_NULL;
    }
    while (p < end) {
        _PyIRExceptionEntry entry;
        int valid = _PyIR_ExceptionNext(&p, end, &entry);
        assert(valid == 0);
        (void)valid;
        PyObject *row = Py_BuildValue("(iiiiO)", entry.start, entry.end,
                                      entry.target, entry.slot,
                                      entry.lasti ? Py_True : Py_False);
        if (row == NULL || PyList_AppendValue(result, PyValue_FromObjectBorrow(row)) < 0) {
            Py_XDECREF(row);
            Py_DECREF(result);
            return PyValue_NULL;
        }
        Py_DECREF(row);
    }
    return _PyValue_FromObjectResult(result);
}

/*[clinic input]

_opcode.get_recovery_entries

    code: object(type="PyCodeObject *", subclass_of="&PyCode_Type")
    /

Return semantic recovery rows.

Each row contains (from, to, edge, handler, exception slot, lasti).
[clinic start generated code]*/

static PyValue
_opcode_get_recovery_entries_impl(PyObject *module, PyCodeObject *code)
/*[clinic end generated code: output=b4d0ef6562b1c562 input=d1c948c4bb399c17]*/
{
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(code->co_exceptiontable);
    PyObject *result = PyTuple_New(semantic.nrecords);
    if (result == NULL) {
        return PyValue_NULL;
    }
    for (uint32_t i = 0; i < semantic.nrecords; i++) {
        _PyIRRecoveryEvent event;
        _PyIRSemantic_Event(&semantic, i, &event);
        PyObject *row = Py_BuildValue("(iiiiii)", event.from, event.to, event.edge,
                                      event.handler, event.exc_slot, event.lasti);
        if (row == NULL) {
            Py_DECREF(result);
            return PyValue_NULL;
        }
        PyTuple_SET_VALUE(result, i, PyValue_PrepareOwned(row));
    }
    return _PyValue_FromObjectResult(result);
}

static PyMethodDef
opcode_functions[] =  {
    _OPCODE_STACK_EFFECT_METHODDEF
    _OPCODE_IS_VALID_METHODDEF
    _OPCODE_HAS_ARG_METHODDEF
    _OPCODE_HAS_CONST_METHODDEF
    _OPCODE_HAS_NAME_METHODDEF
    _OPCODE_HAS_JUMP_METHODDEF
    _OPCODE_HAS_FREE_METHODDEF
    _OPCODE_HAS_LOCAL_METHODDEF
    _OPCODE_HAS_EXC_METHODDEF
    _OPCODE_GET_SPECIALIZATION_STATS_METHODDEF
    _OPCODE_GET_NB_OPS_METHODDEF
    _OPCODE_GET_INTRINSIC1_DESCS_METHODDEF
    _OPCODE_GET_INTRINSIC2_DESCS_METHODDEF
    _OPCODE_GET_SPECIAL_METHOD_NAMES_METHODDEF
    _OPCODE_GET_IR_CONTRACT_METHODDEF
    _OPCODE_GET_EXCEPTION_ENTRIES_METHODDEF
    _OPCODE_GET_RECOVERY_ENTRIES_METHODDEF
    {NULL, NULL, 0, NULL}
};

static int
_opcode_exec(PyObject *m) {
    if (PyModule_AddIntMacro(m, ENABLE_SPECIALIZATION) < 0) {
        return -1;
    }
    opcode_state *state = get_opcode_state(m);
    state->ir_contract_type = PyStructSequence_NewType(&ir_contract_desc);
    if (state->ir_contract_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->ir_contract_type) < 0) {
        return -1;
    }
#define ADD_IR_CONSTANT(NAME, VALUE) \
    if (PyModule_AddIntConstant(m, NAME, VALUE) < 0) { \
        return -1; \
    }
    ADD_IR_CONSTANT("IR_FIRST_ONLY_HANDLER", IR_FIRST_ONLY_HANDLER);
    ADD_IR_CONSTANT("IR_NUM_HANDLERS", IR_NUM_HANDLERS);
    ADD_IR_CONSTANT("IR_RESULT_PHASE_NONE", _PyIR_RESULT_PHASE_NONE);
    ADD_IR_CONSTANT("IR_RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE",
                    _PyIR_RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE);
    ADD_IR_CONSTANT("IR_ROLE_NONE", _PyIR_ROLE_NONE);
    ADD_IR_CONSTANT("IR_ROLE_DST", _PyIR_ROLE_DST);
    ADD_IR_CONSTANT("IR_ROLE_BASE", _PyIR_ROLE_BASE);
    ADD_IR_CONSTANT("IR_ROLE_FB", _PyIR_ROLE_FB);
    ADD_IR_CONSTANT("IR_ROLE_IMM", _PyIR_ROLE_IMM);
    ADD_IR_CONSTANT("IR_ROLE_LOCAL_IN", _PyIR_ROLE_LOCAL_IN);
    ADD_IR_CONSTANT("IR_ROLE_LOCAL_OUT", _PyIR_ROLE_LOCAL_OUT);
    ADD_IR_CONSTANT("IR_ROLE_LOCAL_CLEAR", _PyIR_ROLE_LOCAL_CLEAR);
    ADD_IR_CONSTANT("IR_ROLE_LOCAL_MOVE", _PyIR_ROLE_LOCAL_MOVE);
    ADD_IR_CONSTANT("IR_ROLE_CELL", _PyIR_ROLE_CELL);
    ADD_IR_CONSTANT("IR_ROLE_IN0", _PyIR_ROLE_IN0);
    ADD_IR_CONSTANT("IR_LOC_NONE", _PyIR_LOC_NONE);
    ADD_IR_CONSTANT("IR_LOC_A", _PyIR_LOC_A);
    ADD_IR_CONSTANT("IR_LOC_B", _PyIR_LOC_B);
    ADD_IR_CONSTANT("IR_LOC_C", _PyIR_LOC_C);
    ADD_IR_CONSTANT("IR_LOC_D", _PyIR_LOC_D);
    ADD_IR_CONSTANT("IR_LOC_CD", _PyIR_LOC_CD);
    ADD_IR_CONSTANT("IR_LOC_FBARG", _PyIR_LOC_FBARG);
    ADD_IR_CONSTANT("IR_VALID", _PyIR_VALID);
    ADD_IR_CONSTANT("IR_WINDOW", _PyIR_WINDOW);
    ADD_IR_CONSTANT("IR_JUMPS", _PyIR_JUMPS);
    ADD_IR_CONSTANT("IR_FORWARD", _PyIR_FORWARD);
    ADD_IR_CONSTANT("IR_NO_FOLD", _PyIR_NO_FOLD);
    ADD_IR_CONSTANT("IR_TRANSPARENT", _PyIR_TRANSPARENT);
    ADD_IR_CONSTANT("IR_STORE_DST", _PyIR_STORE_DST);
    ADD_IR_CONSTANT("IR_OUTPUT_REGISTER_DST", _PyIR_OUTPUT_REGISTER_DST);
    ADD_IR_CONSTANT("IR_OUTPUT_WINDOW", _PyIR_OUTPUT_WINDOW);
    ADD_IR_CONSTANT("IR_OUTPUT_WINDOW_DST_FIRST", _PyIR_OUTPUT_WINDOW_DST_FIRST);
    ADD_IR_CONSTANT("IR_OUTPUT_OUTCOME", _PyIR_OUTPUT_OUTCOME);
    ADD_IR_CONSTANT("IR_UNPACKS", _PyIR_UNPACKS);
    ADD_IR_CONSTANT("IR_ABRUPT_PREFIX", _PyIR_ABRUPT_PREFIX);
    ADD_IR_CONSTANT("IR_ABRUPT_OPERANDS", _PyIR_ABRUPT_OPERANDS);
    ADD_IR_CONSTANT("IR_ERROR_PROGRAM", _PyIR_ERROR_PROGRAM);
    ADD_IR_CONSTANT("IR_RUNS_CODE", _PyIR_RUNS_CODE);
    ADD_IR_CONSTANT("IR_JUMP_ALWAYS", _PyIR_JUMP_ALWAYS);
    ADD_IR_CONSTANT("IR_NAMES_TARGET", _PyIR_NAMES_TARGET);
    ADD_IR_CONSTANT("IR_FUSED", _PyIR_FUSED);
    ADD_IR_CONSTANT("IR_FUSED_IF_FALSE", _PyIR_FUSED_IF_FALSE);
    ADD_IR_CONSTANT("IR_FUSED_OP_MASK", IR_FUSED_OP_MASK);
    ADD_IR_CONSTANT("IR_RAISES", _PyIR_RAISES);
    ADD_IR_CONSTANT("IR_ESCAPES", _PyIR_ESCAPES);
    ADD_IR_CONSTANT("IR_RUNS_PYTHON", _PyIR_RUNS_PYTHON);
    ADD_IR_CONSTANT("IR_DEOPTS", _PyIR_DEOPTS);
    ADD_IR_CONSTANT("IR_SUSPENDS", _PyIR_SUSPENDS);
    ADD_IR_CONSTANT("IR_PUSHES_FRAME", _PyIR_PUSHES_FRAME);
    ADD_IR_CONSTANT("IR_POLLS", _PyIR_POLLS);
    ADD_IR_CONSTANT("IR_ALLOCATES", _PyIR_ALLOCATES);
    ADD_IR_CONSTANT("IR_PURE", _PyIR_PURE);
    ADD_IR_CONSTANT("IR_SPECIALIZES", _PyIR_SPECIALIZES);
    ADD_IR_CONSTANT("IR_BOXES", _PyIR_BOXES);
    ADD_IR_CONSTANT("IR_NATIVE_ONLY", _PyIR_NATIVE_ONLY);
    ADD_IR_CONSTANT("IR_ERROR_COMPLETE", _PyIR_ERROR_COMPLETE);
    ADD_IR_CONSTANT("IR_ERROR_DIRECT", _PyIR_ERROR_DIRECT);
    ADD_IR_CONSTANT("IR_ERROR_BRIDGE", _PyIR_ERROR_BRIDGE);
    ADD_IR_CONSTANT("IR_ERROR_LOCAL_DST", _PyIR_ERROR_LOCAL_DST);
    ADD_IR_CONSTANT("IR_EXT_FIXED", _PyIR_EXT_FIXED);
    ADD_IR_CONSTANT("IR_EXT_OPARG", _PyIR_EXT_OPARG);
    ADD_IR_CONSTANT("IR_EXT_OPARG2", _PyIR_EXT_OPARG2);
    ADD_IR_CONSTANT("IR_EXT_BIT0", _PyIR_EXT_BIT0);
    ADD_IR_CONSTANT("IR_EXT_BYTES", _PyIR_EXT_BYTES);
#undef ADD_IR_CONSTANT
    return 0;
}

static PyModuleDef_Slot module_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, _opcode_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static int
opcode_traverse(PyObject *module, visitproc visit, void *arg)
{
    Py_VISIT(get_opcode_state(module)->ir_contract_type);
    return 0;
}

static int
opcode_clear(PyObject *module)
{
    Py_CLEAR(get_opcode_state(module)->ir_contract_type);
    return 0;
}

static void
opcode_free(void *module)
{
    (void)opcode_clear((PyObject *)module);
}

static struct PyModuleDef opcodemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_opcode",
    .m_doc = "Opcode support module.",
    .m_size = sizeof(opcode_state),
    .m_methods = opcode_functions,
    .m_slots = module_slots,
    .m_traverse = opcode_traverse,
    .m_clear = opcode_clear,
    .m_free = opcode_free,
};

PyMODINIT_FUNC
PyInit__opcode(void)
{
    return PyModuleDef_Init(&opcodemodule);
}
