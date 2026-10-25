#include "Python.h"
#include "pycore_bitutils.h"      // _Py_popcount32()
#include "pycore_call.h"          // _PyObject_VectorcallTstate()
#include "pycore_ceval.h"         // _PY_EVAL_EVENTS_BITS
#include "pycore_code.h"          // _PyCode_InitAddressRange()
#include "pycore_critical_section.h" // _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED()
#include "pycore_frame.h"         // PyFrameObject
#include "pycore_interpframe.h"   // _PyFrame_GetBytecode()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_modsupport.h"    // _PyModule_CreateInitialized()
#include "pycore_namespace.h"     // _PyNamespace_New()
#include "pycore_opcode_metadata.h" // IS_VALID_OPCODE()
#include "pycore_opcode_utils.h"  // IS_CONDITIONAL_JUMP_OPCODE()
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_STORE_UINTPTR_RELEASE()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime_structs.h" // _PyCoMonitoringData
#include "pycore_tuple.h"         // _PyTuple_FromArraySteal()

#include "opcode_ids.h"


/* Uncomment this to dump debugging output when assertions fail */
// #define INSTRUMENT_DEBUG 1

#if defined(Py_DEBUG) && defined(Py_GIL_DISABLED)

#define ASSERT_WORLD_STOPPED_OR_LOCKED(obj)                         \
    if (!_PyInterpreterState_GET()->stoptheworld.world_stopped) {   \
        _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(obj);             \
    }
#define ASSERT_WORLD_STOPPED() assert(_PyInterpreterState_GET()->stoptheworld.world_stopped);

#else

#define ASSERT_WORLD_STOPPED_OR_LOCKED(obj)
#define ASSERT_WORLD_STOPPED()

#endif

#ifdef Py_GIL_DISABLED

#define LOCK_CODE(code)                                             \
    assert(!_PyInterpreterState_GET()->stoptheworld.world_stopped); \
    Py_BEGIN_CRITICAL_SECTION(code)

#define UNLOCK_CODE()   Py_END_CRITICAL_SECTION()

#else

#define LOCK_CODE(code)
#define UNLOCK_CODE()

#endif

/* Instrumentation rewrites the handlers of the executing copy of a code
 * object, which force_instrument_lock_held() makes sure exists, and indexes
 * its tables by word.  Every build has one executing copy per code object.
 * The words of a relaxed FOR_ITER or SEND after its own (IR_JUMP) belong to
 * it, as inline caches did to their instruction: they have no events of
 * their own. */

static inline _PyIRWord *
exec_words(PyCodeObject *code)
{
    assert(code->_co_exec != NULL);
    return code->_co_exec->words;
}

/* The opcode of the instruction at index i, without instrumentation or
 * specialisation, as the compiler emitted it. */
static inline int
base_opcode(PyCodeObject *code, int i)
{
    return _PyIR_BaseOpcode(_PyCode_InstrHandler(code, i));
}

/* The opcode that the word at index i of words, an executing copy, runs:
 * an instrumented opcode, INSTRUMENTED_LINE, INSTRUMENTED_INSTRUCTION or
 * the base opcode of a generic or specialised handler. */
static inline int
word_opcode(_PyIRWord *words, int i)
{
    return _PyIR_BaseOpcode(IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(words[i])));
}

/* Make the word at index i of the executing copy of code run opcode */
static inline void
set_word_opcode(PyCodeObject *code, _PyIRWord *words, int i, int opcode)
{
    _PyIR_SetHandler(&words[i], _PyCode_HandlerFor(code, i, opcode));
}

/* The words of the instruction at index i: one, or for a relaxed FOR_ITER or
 * SEND its own and the jumps after it. */
static inline int
instr_length(PyCodeObject *code, int i)
{
    int n = 1;
    while (i + n < Py_SIZE(code) && _PyCode_InstrHandler(code, i + n) == IR_JUMP) {
        n++;
    }
    return n;
}

/* The word index that the jump at index i lands on, through the jump that
 * a relaxed FOR_ITER or SEND adds */
static int
jump_dest(PyCodeObject *code, int i)
{
    int dest = _PyCode_InstrTarget(code, i);
    assert(dest >= 0);
    if (_PyCode_InstrHandler(code, dest) == IR_JUMP) {
        dest = _PyCode_InstrTarget(code, dest);
    }
    return dest;
}

/* The word index of the instruction at instr, an instruction pointer of
 * frame, and of the instruction that a jump to dest lands on. */
static inline int
instr_offset(_PyInterpreterFrame *frame, _PyInstr *instr)
{
    return _PyFrame_InstrOffset(frame, instr);
}

static inline int
dest_offset(_PyInterpreterFrame *frame, _PyInstr *dest)
{
    int i = _PyFrame_InstrOffset(frame, dest);
    if (IR_HANDLER(*dest) == IR_JUMP) {
        // A relaxed FOR_ITER or SEND goes on through a jump that it added
        i = _PyCode_InstrTarget(_PyFrame_GetCode(frame), i);
    }
    return i;
}

PyObject _PyInstrumentation_DISABLE = _PyObject_HEAD_INIT(&PyBaseObject_Type);

PyObject _PyInstrumentation_MISSING = _PyObject_HEAD_INIT(&PyBaseObject_Type);

static const int8_t EVENT_FOR_OPCODE[256] = {
    [RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [INSTRUMENTED_RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [CALL] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL] = PY_MONITORING_EVENT_CALL,
    [CALL_KW] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL_KW] = PY_MONITORING_EVENT_CALL,
    [CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [LOAD_SUPER_ATTR] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_LOAD_SUPER_ATTR] = PY_MONITORING_EVENT_CALL,
    [RESUME] = -1,
    [YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
    [INSTRUMENTED_YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
    [JUMP_FORWARD] = PY_MONITORING_EVENT_JUMP,
    [JUMP_BACKWARD] = PY_MONITORING_EVENT_JUMP,
    [POP_JUMP_IF_FALSE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [POP_JUMP_IF_TRUE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [POP_JUMP_IF_NONE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [POP_JUMP_IF_NOT_NONE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [INSTRUMENTED_JUMP_FORWARD] = PY_MONITORING_EVENT_JUMP,
    [INSTRUMENTED_JUMP_BACKWARD] = PY_MONITORING_EVENT_JUMP,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [FOR_ITER] = PY_MONITORING_EVENT_BRANCH_LEFT,
    [INSTRUMENTED_FOR_ITER] = PY_MONITORING_EVENT_BRANCH_LEFT,
    [POP_ITER] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [INSTRUMENTED_POP_ITER] = PY_MONITORING_EVENT_BRANCH_RIGHT,
    [END_FOR] = PY_MONITORING_EVENT_STOP_ITERATION,
    [INSTRUMENTED_END_FOR] = PY_MONITORING_EVENT_STOP_ITERATION,
    [END_SEND] = PY_MONITORING_EVENT_STOP_ITERATION,
    [INSTRUMENTED_END_SEND] = PY_MONITORING_EVENT_STOP_ITERATION,
    [NOT_TAKEN] = PY_MONITORING_EVENT_BRANCH_LEFT,
    [INSTRUMENTED_NOT_TAKEN] = PY_MONITORING_EVENT_BRANCH_LEFT,
    [END_ASYNC_FOR] = PY_MONITORING_EVENT_BRANCH_RIGHT,
};

static const uint8_t DE_INSTRUMENT[256] = {
    [INSTRUMENTED_RESUME] = RESUME,
    [INSTRUMENTED_RETURN_VALUE] = RETURN_VALUE,
    [INSTRUMENTED_CALL] = CALL,
    [INSTRUMENTED_CALL_KW] = CALL_KW,
    [INSTRUMENTED_CALL_FUNCTION_EX] = CALL_FUNCTION_EX,
    [INSTRUMENTED_YIELD_VALUE] = YIELD_VALUE,
    [INSTRUMENTED_JUMP_FORWARD] = JUMP_FORWARD,
    [INSTRUMENTED_JUMP_BACKWARD] = JUMP_BACKWARD,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = POP_JUMP_IF_NOT_NONE,
    [INSTRUMENTED_FOR_ITER] = FOR_ITER,
    [INSTRUMENTED_POP_ITER] = POP_ITER,
    [INSTRUMENTED_END_FOR] = END_FOR,
    [INSTRUMENTED_END_SEND] = END_SEND,
    [INSTRUMENTED_LOAD_SUPER_ATTR] = LOAD_SUPER_ATTR,
    [INSTRUMENTED_NOT_TAKEN] = NOT_TAKEN,
    [INSTRUMENTED_END_ASYNC_FOR] = END_ASYNC_FOR,
};

static const uint8_t INSTRUMENTED_OPCODES[256] = {
    [RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [INSTRUMENTED_RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [CALL] = INSTRUMENTED_CALL,
    [INSTRUMENTED_CALL] = INSTRUMENTED_CALL,
    [CALL_KW] = INSTRUMENTED_CALL_KW,
    [INSTRUMENTED_CALL_KW] = INSTRUMENTED_CALL_KW,
    [CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [INSTRUMENTED_CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [INSTRUMENTED_YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [RESUME] = INSTRUMENTED_RESUME,
    [INSTRUMENTED_RESUME] = INSTRUMENTED_RESUME,
    [JUMP_FORWARD] = INSTRUMENTED_JUMP_FORWARD,
    [INSTRUMENTED_JUMP_FORWARD] = INSTRUMENTED_JUMP_FORWARD,
    [JUMP_BACKWARD] = INSTRUMENTED_JUMP_BACKWARD,
    [INSTRUMENTED_JUMP_BACKWARD] = INSTRUMENTED_JUMP_BACKWARD,
    [POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    [END_FOR] = INSTRUMENTED_END_FOR,
    [INSTRUMENTED_END_FOR] = INSTRUMENTED_END_FOR,
    [END_SEND] = INSTRUMENTED_END_SEND,
    [INSTRUMENTED_END_SEND] = INSTRUMENTED_END_SEND,
    [FOR_ITER] = INSTRUMENTED_FOR_ITER,
    [INSTRUMENTED_FOR_ITER] = INSTRUMENTED_FOR_ITER,
    [POP_ITER] = INSTRUMENTED_POP_ITER,
    [INSTRUMENTED_POP_ITER] = INSTRUMENTED_POP_ITER,
    [LOAD_SUPER_ATTR] = INSTRUMENTED_LOAD_SUPER_ATTR,
    [INSTRUMENTED_LOAD_SUPER_ATTR] = INSTRUMENTED_LOAD_SUPER_ATTR,
    [NOT_TAKEN] = INSTRUMENTED_NOT_TAKEN,
    [INSTRUMENTED_NOT_TAKEN] = INSTRUMENTED_NOT_TAKEN,
    [END_ASYNC_FOR] = INSTRUMENTED_END_ASYNC_FOR,
    [INSTRUMENTED_END_ASYNC_FOR] = INSTRUMENTED_END_ASYNC_FOR,

    [INSTRUMENTED_LINE] = INSTRUMENTED_LINE,
    [INSTRUMENTED_INSTRUCTION] = INSTRUMENTED_INSTRUCTION,
};

static inline bool
opcode_has_event(int opcode)
{
    return (
        opcode != INSTRUMENTED_LINE &&
        INSTRUMENTED_OPCODES[opcode] > 0
    );
}

uint8_t
_PyCode_Deinstrument(uint8_t opcode)
{
    return DE_INSTRUMENT[opcode];
}

static inline bool
is_instrumented(int opcode)
{
    assert(opcode != 0);
    assert(opcode != RESERVED);
    return opcode >= MIN_INSTRUMENTED_OPCODE;
}

#ifndef NDEBUG
static inline bool
monitors_equals(_Py_LocalMonitors a, _Py_LocalMonitors b)
{
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        if (a.tools[i] != b.tools[i]) {
            return false;
        }
    }
    return true;
}
#endif

static inline _Py_LocalMonitors
monitors_sub(_Py_LocalMonitors a, _Py_LocalMonitors b)
{
    _Py_LocalMonitors res;
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        res.tools[i] = a.tools[i] & ~b.tools[i];
    }
    return res;
}

#ifndef NDEBUG
static inline _Py_LocalMonitors
monitors_and(_Py_LocalMonitors a, _Py_LocalMonitors b)
{
    _Py_LocalMonitors res;
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        res.tools[i] = a.tools[i] & b.tools[i];
    }
    return res;
}
#endif

/* The union of the *local* events in a and b.
 * Global events like RAISE are ignored.
 * Used for instrumentation, as only local
 * events get instrumented.
 */
static inline _Py_LocalMonitors
local_union(_Py_GlobalMonitors a, _Py_LocalMonitors b)
{
    _Py_LocalMonitors res;
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        res.tools[i] = a.tools[i] | b.tools[i];
    }
    return res;
}

static inline bool
monitors_are_empty(_Py_LocalMonitors m)
{
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        if (m.tools[i]) {
            return false;
        }
    }
    return true;
}

static inline bool
multiple_tools(_Py_LocalMonitors *m)
{
    for (int i = 0; i < _PY_MONITORING_UNGROUPED_EVENTS; i++) {
        if (_Py_popcount32(m->tools[i]) > 1) {
            return true;
        }
    }
    return false;
}

static inline _PyMonitoringEventSet
get_local_events(_Py_LocalMonitors *m, int tool_id)
{
    _PyMonitoringEventSet result = 0;
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        if ((m->tools[e] >> tool_id) & 1) {
            result |= (1 << e);
        }
    }
    return result;
}

static inline _PyMonitoringEventSet
get_events(_Py_GlobalMonitors *m, int tool_id)
{
    _PyMonitoringEventSet result = 0;
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        if ((m->tools[e] >> tool_id) & 1) {
            result |= (1 << e);
        }
    }
    return result;
}

/* Module code can have line 0, even though modules start at line 1,
 * so -1 is a legal delta. */
#define NO_LINE (-2)

/* Returns the line delta. Defined as:
 * if line is None:
 *     line_delta = NO_LINE
 * else:
 *     line_delta = line - first_line
 */
static int
compute_line_delta(PyCodeObject *code, int line)
{
    if (line < 0) {
        assert(line == -1);
        return NO_LINE;
    }
    int delta = line - code->co_firstlineno;
    assert(delta > NO_LINE);
    return delta;
}

static int
compute_line(PyCodeObject *code, int line_delta)
{
    if (line_delta == NO_LINE) {
        return -1;
    }
    assert(line_delta > NO_LINE);
    return code->co_firstlineno + line_delta;
}

static inline uint8_t *
get_original_opcode_ptr(_PyCoLineInstrumentationData *line_data, int index)
{
    return &line_data->data[index*line_data->bytes_per_entry];
}

static inline void
set_original_opcode(_PyCoLineInstrumentationData *line_data, int index, uint8_t opcode)
{
    line_data->data[index*line_data->bytes_per_entry] = opcode;
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

static inline void
set_line_delta(_PyCoLineInstrumentationData *line_data, int index, int line_delta)
{
    /* Store line_delta + 2 as we need -2 to represent no line number */
    assert(line_delta >= NO_LINE);
    uint32_t adjusted = line_delta - NO_LINE;
    uint8_t *ptr = &line_data->data[index*line_data->bytes_per_entry+1];
    assert(adjusted < (1ULL << ((line_data->bytes_per_entry-1)*8)));
    assert(line_data->bytes_per_entry >= 2);
    *ptr = adjusted & 0xff;
    for (int idx = 2; idx < line_data->bytes_per_entry; idx++) {
        ptr++;
        adjusted >>= 8;
        *ptr = adjusted & 0xff;
    }
}

#ifdef INSTRUMENT_DEBUG

static void
dump_instrumentation_data_tools(PyCodeObject *code, uint8_t *tools, int i, FILE*out)
{
    if (tools == NULL) {
        fprintf(out, "tools = NULL");
    }
    else {
        fprintf(out, "tools = %d", tools[i]);
    }
}

static void
dump_instrumentation_data_lines(PyCodeObject *code, _PyCoLineInstrumentationData *lines, int i, FILE*out)
{
    if (lines == NULL) {
        fprintf(out, ", lines = NULL");
    }
    else {
        int opcode = _PyCode_GetOriginalOpcode(lines, i);
        int line_delta = get_line_delta(lines, i);
        if (opcode == 0) {
            fprintf(out, ", lines = {original_opcode = No LINE (0), line_delta = %d)", line_delta);
        }
        else {
            fprintf(out, ", lines = {original_opcode = %s, line_delta = %d)", _PyOpcode_OpName[opcode], line_delta);
        }
    }
}

static void
dump_instrumentation_data_line_tools(PyCodeObject *code, uint8_t *line_tools, int i, FILE*out)
{
    if (line_tools == NULL) {
        fprintf(out, ", line_tools = NULL");
    }
    else {
        fprintf(out, ", line_tools = %d", line_tools[i]);
    }
}

static void
dump_instrumentation_data_per_instruction(PyCodeObject *code, _PyCoMonitoringData *data, int i, FILE*out)
{
    if (data->per_instruction_opcodes == NULL) {
        fprintf(out, ", per-inst opcode = NULL");
    }
    else {
        fprintf(out, ", per-inst opcode = %s", _PyOpcode_OpName[data->per_instruction_opcodes[i]]);
    }
    if (data->per_instruction_tools == NULL) {
        fprintf(out, ", per-inst tools = NULL");
    }
    else {
        fprintf(out, ", per-inst tools = %d", data->per_instruction_tools[i]);
    }
}

static void
dump_global_monitors(const char *prefix, _Py_GlobalMonitors monitors, FILE*out)
{
    fprintf(out, "%s monitors:\n", prefix);
    for (int event = 0; event < _PY_MONITORING_UNGROUPED_EVENTS; event++) {
        fprintf(out, "    Event %d: Tools %x\n", event, monitors.tools[event]);
    }
}

static void
dump_local_monitors(const char *prefix, _Py_LocalMonitors monitors, FILE*out)
{
    fprintf(out, "%s monitors:\n", prefix);
    for (int event = 0; event < _PY_MONITORING_UNGROUPED_EVENTS; event++) {
        fprintf(out, "    Event %d: Tools %x\n", event, monitors.tools[event]);
    }
}

/** NOTE:
 * Do not use PyCode_Addr2Line to determine the line number in instrumentation,
 * as `PyCode_Addr2Line` uses the monitoring data if it is available.
 */


/* No error checking -- Don't use this for anything but experimental debugging */
static void
dump_instrumentation_data(PyCodeObject *code, int star, FILE*out)
{
    _PyCoMonitoringData *data = code->_co_monitoring;
    fprintf(out, "\n");
    PyObject_Print(code->co_name, out, Py_PRINT_RAW);
    fprintf(out, "\n");
    if (data == NULL) {
        fprintf(out, "NULL\n");
        return;
    }
    dump_global_monitors("Global", _PyInterpreterState_GET()->monitors, out);
    dump_local_monitors("Code", data->local_monitors, out);
    dump_local_monitors("Active", data->active_monitors, out);
    int code_len = (int)Py_SIZE(code);
    bool starred = false;
    PyCodeAddressRange range;
    _PyCode_InitAddressRange(code, &range);
    for (int i = 0; i < code_len; i += instr_length(code, i)) {
        int opcode = code->_co_exec ? word_opcode(exec_words(code), i)
                                    : base_opcode(code, i);
        if (i == star) {
            fprintf(out, "**  ");
            starred = true;
        }
        fprintf(out, "Offset: %d, line: %d %s: ", i, _PyCode_CheckLineNumber(i, &range), _PyOpcode_OpName[opcode]);
        dump_instrumentation_data_tools(code, data->tools, i, out);
        dump_instrumentation_data_lines(code, data->lines, i, out);
        dump_instrumentation_data_line_tools(code, data->line_tools, i, out);
        dump_instrumentation_data_per_instruction(code, data, i, out);
        fprintf(out, "\n");
        ;
    }
    if (!starred && star >= 0) {
        fprintf(out, "Error offset not at valid instruction offset: %d\n", star);
        fprintf(out, "    ");
        dump_instrumentation_data_tools(code, data->tools, star, out);
        dump_instrumentation_data_lines(code, data->lines, star, out);
        dump_instrumentation_data_line_tools(code, data->line_tools, star, out);
        dump_instrumentation_data_per_instruction(code, data, star, out);
        fprintf(out, "\n");
    }
}

#define CHECK(test) do { \
    if (!(test)) { \
        dump_instrumentation_data(code, i, stderr); \
    } \
    assert(test); \
} while (0)

static bool
valid_opcode(int opcode)
{
    if (opcode == INSTRUMENTED_LINE) {
        return true;
    }
    if (IS_VALID_OPCODE(opcode) &&
        opcode != CACHE &&
        opcode != RESERVED)
    {
       return true;
    }
    return false;
}

static void
sanity_check_instrumentation(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    _PyCoMonitoringData *data = code->_co_monitoring;
    if (data == NULL) {
        return;
    }
    _Py_GlobalMonitors global_monitors = _PyInterpreterState_GET()->monitors;
    _Py_LocalMonitors active_monitors;
    if (code->_co_monitoring) {
        _Py_LocalMonitors local_monitors = code->_co_monitoring->local_monitors;
        active_monitors = local_union(global_monitors, local_monitors);
    }
    else {
        _Py_LocalMonitors empty = (_Py_LocalMonitors) { 0 };
        active_monitors = local_union(global_monitors, empty);
    }
    assert(monitors_equals(
        code->_co_monitoring->active_monitors,
        active_monitors));
    int code_len = (int)Py_SIZE(code);
    PyCodeAddressRange range;
    _PyCode_InitAddressRange(co, &range);
    _PyIRWord *words = exec_words(code);
    for (int i = 0; i < code_len;) {
        int opcode = word_opcode(words, i);
        int base = base_opcode(code, i);
        CHECK(valid_opcode(opcode));
        CHECK(valid_opcode(base));
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            opcode = data->per_instruction_opcodes[i];
            if (!is_instrumented(opcode)) {
                CHECK(_PyOpcode_Deopt[opcode] == opcode);
            }
        }
        if (opcode == INSTRUMENTED_LINE) {
            CHECK(data->lines);
            opcode = _PyCode_GetOriginalOpcode(data->lines, i);
            CHECK(valid_opcode(opcode));
            CHECK(opcode != END_FOR);
            CHECK(opcode != RESUME);
            CHECK(opcode != RESUME_CHECK);
            CHECK(opcode != INSTRUMENTED_RESUME);
            if (!is_instrumented(opcode)) {
                CHECK(_PyOpcode_Deopt[opcode] == opcode);
            }
            CHECK(opcode != INSTRUMENTED_LINE);
        }
        else if (data->lines) {
            /* If original_opcode is INSTRUMENTED_INSTRUCTION
             * *and* we are executing a INSTRUMENTED_LINE instruction
             * that has de-instrumented itself, then we will execute
             * an invalid INSTRUMENTED_INSTRUCTION */
            CHECK(_PyCode_GetOriginalOpcode(data->lines, i) != INSTRUMENTED_INSTRUCTION);
        }
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            CHECK(data->per_instruction_opcodes[i] != 0);
            opcode = data->per_instruction_opcodes[i];
        }
        if (is_instrumented(opcode)) {
            CHECK(DE_INSTRUMENT[opcode] == base);
            int event = EVENT_FOR_OPCODE[DE_INSTRUMENT[opcode]];
            if (event < 0) {
                /* RESUME fixup */
                event = _PyCode_InstrOparg(code, i) ? 1: 0;
            }
            CHECK(active_monitors.tools[event] != 0);
        }
        if (data->lines && _PyCode_GetOriginalOpcode(data->lines, i)) {
            int line1 = compute_line(code, get_line_delta(data->lines, i));
            int line2 = _PyCode_CheckLineNumber(i, &range);
            CHECK(line1 == line2);
        }
        CHECK(valid_opcode(opcode));
        if (data->tools) {
            uint8_t local_tools = data->tools[i];
            if (opcode_has_event(base)) {
                int event = EVENT_FOR_OPCODE[base];
                if (event == -1) {
                    /* RESUME fixup */
                    event = _PyCode_InstrOparg(code, i);
                }
                CHECK((active_monitors.tools[event] & local_tools) == local_tools);
            }
            else {
                CHECK(local_tools == 0xff);
            }
        }
        i += instr_length(code, i);
        assert(i <= code_len);
    }
}
#else

#define CHECK(test) assert(test)

#endif

/* Restart the specialisation warmup of the instruction at index i, whose
 * handler changes between its instrumented and uninstrumented forms.  The
 * shared entry never warms up. */
static void
warm_up(PyCodeObject *code, int i)
{
#ifndef Py_GIL_DISABLED
    if (!(ENABLE_SPECIALIZATION
          && _PyInterpreterState_GET()->opt_config.specialization_enabled))
    {
        return;
    }
    int fb = _PyCode_InstrFbIndex(code, i);
    if (fb <= 0) {
        return;
    }
    _PyFbHeader *h = _PyIR_FbEntry(code->_co_exec->entries, (uint32_t)fb);
    h->counter = adaptive_counter_warmup();
#endif
}

/* Where the instrumentation of the word at index i keeps the opcode that
 * de_instrument() and instrument() rewrite: the table of INSTRUMENTED_LINE
 * or INSTRUMENTED_INSTRUCTION if one has displaced it, or NULL for the
 * word itself.  *opcode is set to that opcode. */
static uint8_t *
displaced_opcode_ptr(_PyIRWord *words, _PyCoMonitoringData *monitoring, int i,
                     int *opcode)
{
    uint8_t *opcode_ptr = NULL;
    *opcode = word_opcode(words, i);
    if (*opcode == INSTRUMENTED_LINE) {
        opcode_ptr = get_original_opcode_ptr(monitoring->lines, i);
        *opcode = *opcode_ptr;
    }
    if (*opcode == INSTRUMENTED_INSTRUCTION) {
        opcode_ptr = &monitoring->per_instruction_opcodes[i];
        *opcode = *opcode_ptr;
    }
    return opcode_ptr;
}

static void
store_opcode(PyCodeObject *code, _PyIRWord *words, uint8_t *opcode_ptr, int i,
             int opcode)
{
    if (opcode_ptr != NULL) {
        FT_ATOMIC_STORE_UINT8_RELAXED(*opcode_ptr, opcode);
    }
    else {
        set_word_opcode(code, words, i, opcode);
    }
}

static void
de_instrument(PyCodeObject *code, _PyIRWord *words, _PyCoMonitoringData *monitoring, int i,
              int event)
{
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(event != PY_MONITORING_EVENT_LINE);

    int opcode;
    uint8_t *opcode_ptr = displaced_opcode_ptr(words, monitoring, i, &opcode);
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented == 0) {
        return;
    }
    CHECK(_PyOpcode_Deopt[deinstrumented] == deinstrumented);
    store_opcode(code, words, opcode_ptr, i, deinstrumented);
    if (_PyFb_HasEntry(deinstrumented)) {
        warm_up(code, i);
    }
}

static void
de_instrument_line(PyCodeObject *code, _PyIRWord *words, _PyCoMonitoringData *monitoring,
                   int i)
{
    int opcode = word_opcode(words, i);
    if (opcode != INSTRUMENTED_LINE) {
        return;
    }
    _PyCoLineInstrumentationData *lines = monitoring->lines;
    int original_opcode = _PyCode_GetOriginalOpcode(lines, i);
    CHECK(original_opcode != 0);
    CHECK(original_opcode == _PyOpcode_Deopt[original_opcode]);
    set_word_opcode(code, words, i, original_opcode);
    if (_PyFb_HasEntry(original_opcode)) {
        warm_up(code, i);
    }
    assert(word_opcode(words, i) != INSTRUMENTED_LINE);
}

static void
de_instrument_per_instruction(PyCodeObject *code, _PyIRWord *words,
                              _PyCoMonitoringData *monitoring, int i)
{
    uint8_t *opcode_ptr = NULL;
    int opcode = word_opcode(words, i);
    if (opcode == INSTRUMENTED_LINE) {
        opcode_ptr = get_original_opcode_ptr(monitoring->lines, i);
        opcode = *opcode_ptr;
    }
    if (opcode != INSTRUMENTED_INSTRUCTION) {
        return;
    }
    int original_opcode = monitoring->per_instruction_opcodes[i];
    CHECK(original_opcode != 0);
    CHECK(original_opcode == _PyOpcode_Deopt[original_opcode]);
    store_opcode(code, words, opcode_ptr, i, original_opcode);
    if (_PyFb_HasEntry(original_opcode)) {
        warm_up(code, i);
    }
    assert(word_opcode(words, i) != INSTRUMENTED_INSTRUCTION);
}

static void
instrument(PyCodeObject *code, _PyIRWord *words, _PyCoMonitoringData *monitoring, int i)
{
    int opcode;
    uint8_t *opcode_ptr = displaced_opcode_ptr(words, monitoring, i, &opcode);
    CHECK(opcode != INSTRUMENTED_INSTRUCTION && opcode != INSTRUMENTED_LINE);
    CHECK(opcode != 0);
    if (!is_instrumented(opcode)) {
        int deopt = _PyOpcode_Deopt[opcode];
        int instrumented = INSTRUMENTED_OPCODES[deopt];
        assert(instrumented);
        store_opcode(code, words, opcode_ptr, i, instrumented);
        if (_PyFb_HasEntry(deopt)) {
            warm_up(code, i);
        }
    }
}

static void
instrument_line(PyCodeObject *code, _PyIRWord *words, _PyCoMonitoringData *monitoring, int i)
{
    int opcode = word_opcode(words, i);
    if (opcode == INSTRUMENTED_LINE) {
        return;
    }
    set_original_opcode(monitoring->lines, i, _PyOpcode_Deopt[opcode]);
    CHECK(get_line_delta(monitoring->lines, i) > NO_LINE);
    set_word_opcode(code, words, i, INSTRUMENTED_LINE);
}

static void
instrument_per_instruction(PyCodeObject *code, _PyIRWord *words,
                           _PyCoMonitoringData *monitoring, int i)
{
    uint8_t *opcode_ptr = NULL;
    int opcode = word_opcode(words, i);
    if (opcode == INSTRUMENTED_LINE) {
        opcode_ptr = get_original_opcode_ptr(monitoring->lines, i);
        opcode = *opcode_ptr;
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        assert(monitoring->per_instruction_opcodes[i] > 0);
        return;
    }
    CHECK(opcode != 0);
    if (is_instrumented(opcode)) {
        monitoring->per_instruction_opcodes[i] = opcode;
    }
    else {
        assert(opcode != 0);
        assert(_PyOpcode_Deopt[opcode] != 0);
        assert(_PyOpcode_Deopt[opcode] != RESUME);
        monitoring->per_instruction_opcodes[i] = _PyOpcode_Deopt[opcode];
    }
    assert(monitoring->per_instruction_opcodes[i] > 0);
    store_opcode(code, words, opcode_ptr, i, INSTRUMENTED_INSTRUCTION);
}

static void
remove_tools(PyCodeObject * code, int offset, int event, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
    assert(opcode_has_event(base_opcode(code, offset)));
    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    assert(monitoring);
    bool should_de_instrument;
    if (monitoring->tools) {
        monitoring->tools[offset] &= ~tools;
        should_de_instrument = (monitoring->tools[offset] == 0);
    }
    else {
        /* Single tool */
        uint8_t single_tool = monitoring->active_monitors.tools[event];
        assert(_Py_popcount32(single_tool) <= 1);
        should_de_instrument = ((single_tool & tools) == single_tool);
    }
    if (should_de_instrument) {
        de_instrument(code, exec_words(code), monitoring, offset, event);
    }
}

#ifndef NDEBUG
static bool
tools_is_subset_for_event(PyCodeObject * code, int event, int tools)
{
    int global_tools = _PyInterpreterState_GET()->monitors.tools[event];
    int local_tools = code->_co_monitoring->local_monitors.tools[event];
    return tools == ((global_tools | local_tools) & tools);
}
#endif

static void
remove_line_tools(PyCodeObject * code, int offset, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    assert(monitoring);
    bool should_de_instrument;
    if (monitoring->line_tools)
    {
        uint8_t *toolsptr = &monitoring->line_tools[offset];
        *toolsptr &= ~tools;
        should_de_instrument = (*toolsptr == 0);
    }
    else {
        /* Single tool */
        uint8_t single_tool = monitoring->active_monitors.tools[PY_MONITORING_EVENT_LINE];
        assert(_Py_popcount32(single_tool) <= 1);
        should_de_instrument = ((single_tool & tools) == single_tool);
    }
    if (should_de_instrument) {
        de_instrument_line(code, exec_words(code), monitoring, offset);
        if (_PyCode_GetOriginalOpcode(monitoring->lines, offset) ==
            INSTRUMENTED_INSTRUCTION)
        {
            set_original_opcode(monitoring->lines, offset,
                                monitoring->per_instruction_opcodes[offset]);
        }
    }
}

static void
add_tools(PyCodeObject * code, int offset, int event, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
    assert(code->_co_monitoring);
    if (code->_co_monitoring &&
        code->_co_monitoring->tools
    ) {
        code->_co_monitoring->tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(_Py_popcount32(tools) == 1);
        assert(tools_is_subset_for_event(code, event, tools));
    }
    instrument(code, exec_words(code), code->_co_monitoring, offset);
}

static void
add_line_tools(PyCodeObject * code, int offset, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    assert(tools_is_subset_for_event(code, PY_MONITORING_EVENT_LINE, tools));
    assert(code->_co_monitoring);
    if (code->_co_monitoring->line_tools) {
        code->_co_monitoring->line_tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(_Py_popcount32(tools) == 1);
    }
    instrument_line(code, exec_words(code), code->_co_monitoring, offset);
}


static void
add_per_instruction_tools(PyCodeObject * code, int offset, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    assert(tools_is_subset_for_event(code, PY_MONITORING_EVENT_INSTRUCTION, tools));
    assert(code->_co_monitoring);
    if (code->_co_monitoring->per_instruction_tools) {
        code->_co_monitoring->per_instruction_tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(_Py_popcount32(tools) == 1);
    }
    instrument_per_instruction(code, exec_words(code), code->_co_monitoring, offset);
}


static void
remove_per_instruction_tools(PyCodeObject * code, int offset, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    assert(code->_co_monitoring);
    bool should_de_instrument;
    if (code->_co_monitoring->per_instruction_tools) {
        uint8_t *toolsptr = &code->_co_monitoring->per_instruction_tools[offset];
        *toolsptr &= ~tools;
        should_de_instrument = (*toolsptr == 0);
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_monitoring->active_monitors.tools[PY_MONITORING_EVENT_INSTRUCTION];
        assert(_Py_popcount32(single_tool) <= 1);
        should_de_instrument = ((single_tool & tools) == single_tool);
    }
    if (should_de_instrument) {
        de_instrument_per_instruction(code, exec_words(code), monitoring, offset);
    }
}


/* Return 1 if DISABLE returned, -1 if error, 0 otherwise */
static int
call_one_instrument(
    PyInterpreterState *interp, PyThreadState *tstate, PyObject **args,
    size_t nargsf, int8_t tool, int event)
{
    assert(0 <= tool && tool < 8);
    assert(tstate->tracing == 0);
    PyObject *instrument = interp->monitoring_callables[tool][event];
    if (instrument == NULL) {
        return 0;
    }
    int old_what = tstate->what_event;
    tstate->what_event = event;
    tstate->tracing++;
    PyObject *res = _PyObject_VectorcallTstate(tstate, instrument, args, nargsf, NULL);
    tstate->tracing--;
    tstate->what_event = old_what;
    if (res == NULL) {
        return -1;
    }
    if (res == &_PyInstrumentation_DISABLE) {
        assert(_Py_IsImmortal(res));
        return 1;
    }
    Py_DECREF(res);
    return 0;
}

static const int8_t MOST_SIGNIFICANT_BITS[16] = {
    -1, 0, 1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3,
    3, 3, 3, 3,
};

/* We could use _Py_bit_length here, but that is designed for larger (32/64)
 * bit ints, and can perform relatively poorly on platforms without the
 * necessary intrinsics. */
static inline int most_significant_bit(uint8_t bits) {
    assert(bits != 0);
    if (bits > 15) {
        return MOST_SIGNIFICANT_BITS[bits>>4]+4;
    }
    return MOST_SIGNIFICANT_BITS[bits];
}

static uint32_t
global_version(PyInterpreterState *interp)
{
    uint32_t version = (uint32_t)_Py_atomic_load_uintptr_relaxed(
        &interp->ceval.instrumentation_version);
#ifdef Py_DEBUG
    PyThreadState *tstate = _PyThreadState_GET();
    uint32_t thread_version =
        (uint32_t)(_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) &
                   ~_PY_EVAL_EVENTS_MASK);
    assert(thread_version == version);
#endif
    return version;
}

/* Atomically set the given version in the given location, without touching
   anything in _PY_EVAL_EVENTS_MASK. */
static void
set_version_raw(uintptr_t *ptr, uint32_t version)
{
    uintptr_t old = _Py_atomic_load_uintptr_relaxed(ptr);
    uintptr_t new;
    do {
        new = (old & _PY_EVAL_EVENTS_MASK) | version;
    } while (!_Py_atomic_compare_exchange_uintptr(ptr, &old, new));
}

static void
set_global_version(PyThreadState *tstate, uint32_t version)
{
    ASSERT_WORLD_STOPPED();

    assert((version & _PY_EVAL_EVENTS_MASK) == 0);
    PyInterpreterState *interp = tstate->interp;
    set_version_raw(&interp->ceval.instrumentation_version, version);

#ifdef Py_GIL_DISABLED
    // Set the version on all threads in free-threaded builds.
    _Py_FOR_EACH_TSTATE_BEGIN(interp, tstate) {
        set_version_raw(&tstate->eval_breaker, version);
    };
    _Py_FOR_EACH_TSTATE_END(interp);
#else
    // Normal builds take the current version from instrumentation_version when
    // attaching a thread, so we only have to set the current thread's version.
    set_version_raw(&tstate->eval_breaker, version);
#endif
}

static bool
is_version_up_to_date(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    return global_version(interp) == code->_co_instrumentation_version;
}

#ifndef NDEBUG
static bool
instrumentation_cross_checks(PyInterpreterState *interp, PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    _Py_LocalMonitors expected = local_union(
        interp->monitors,
        code->_co_monitoring->local_monitors);
    return monitors_equals(code->_co_monitoring->active_monitors, expected);
}

static int
debug_check_sanity(PyInterpreterState *interp, PyCodeObject *code)
{
    int res;
    LOCK_CODE(code);
    res = is_version_up_to_date(code, interp) &&
          instrumentation_cross_checks(interp, code);
    UNLOCK_CODE();
    return res;
}

#endif

static inline uint8_t
get_tools_for_instruction(PyCodeObject *code, PyInterpreterState *interp, int i, int event)
{
    uint8_t tools;
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    if (event >= _PY_MONITORING_UNGROUPED_EVENTS) {
        assert(event == PY_MONITORING_EVENT_C_RAISE ||
                event == PY_MONITORING_EVENT_C_RETURN);
        event = PY_MONITORING_EVENT_CALL;
    }
    assert(_PY_MONITORING_IS_UNGROUPED_EVENT(event));
    CHECK(debug_check_sanity(interp, code));
    if (PY_MONITORING_IS_INSTRUMENTED_EVENT(event)) {
        /* Instrumented events use per-instruction tool bitmaps. */
        if (code->_co_monitoring->tools) {
            tools = code->_co_monitoring->tools[i];
        }
        else {
            tools = code->_co_monitoring->active_monitors.tools[event];
        }
    }
    else {
        /* Other (non-instrumented) events are not tied to specific instructions;
         * use the code-object-level active_monitors bitmap instead. */
        tools = code->_co_monitoring->active_monitors.tools[event];
    }
    return tools;
}

static const char *const event_names [] = {
    [PY_MONITORING_EVENT_PY_START] = "PY_START",
    [PY_MONITORING_EVENT_PY_RESUME] = "PY_RESUME",
    [PY_MONITORING_EVENT_PY_RETURN] = "PY_RETURN",
    [PY_MONITORING_EVENT_PY_YIELD] = "PY_YIELD",
    [PY_MONITORING_EVENT_CALL] = "CALL",
    [PY_MONITORING_EVENT_LINE] = "LINE",
    [PY_MONITORING_EVENT_INSTRUCTION] = "INSTRUCTION",
    [PY_MONITORING_EVENT_JUMP] = "JUMP",
    [PY_MONITORING_EVENT_BRANCH] = "BRANCH",
    [PY_MONITORING_EVENT_BRANCH_LEFT] = "BRANCH_LEFT",
    [PY_MONITORING_EVENT_BRANCH_RIGHT] = "BRANCH_RIGHT",
    [PY_MONITORING_EVENT_C_RETURN] = "C_RETURN",
    [PY_MONITORING_EVENT_PY_THROW] = "PY_THROW",
    [PY_MONITORING_EVENT_RAISE] = "RAISE",
    [PY_MONITORING_EVENT_RERAISE] = "RERAISE",
    [PY_MONITORING_EVENT_EXCEPTION_HANDLED] = "EXCEPTION_HANDLED",
    [PY_MONITORING_EVENT_C_RAISE] = "C_RAISE",
    [PY_MONITORING_EVENT_PY_UNWIND] = "PY_UNWIND",
    [PY_MONITORING_EVENT_STOP_ITERATION] = "STOP_ITERATION",
};

/* Disable an "other" (non-instrumented) event (e.g. PY_UNWIND) for a single
 * tool on this code object.  Must be called with the world stopped or the
 * code lock held. */
static void
remove_local_tool(PyCodeObject *code, PyInterpreterState *interp,
                  int event, int tool)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    assert(_PY_MONITORING_IS_UNGROUPED_EVENT(event));
    assert(!PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
    assert(code->_co_monitoring);
    code->_co_monitoring->local_monitors.tools[event] &= ~(1 << tool);
    /* Recompute active_monitors for this event as the union of global and
     * (now updated) local monitors. */
    code->_co_monitoring->active_monitors.tools[event] =
        interp->monitors.tools[event] |
        code->_co_monitoring->local_monitors.tools[event];
}

static int
call_instrumentation_vector(
    _PyInstr *instr, PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *arg2, Py_ssize_t nargs, PyObject *args[])
{
    if (tstate->tracing) {
        return 0;
    }
    assert(!_PyErr_Occurred(tstate));
    assert(args[0] == NULL);
    PyCodeObject *code = _PyFrame_GetCode(frame);
    assert(args[1] == NULL);
    args[1] = (PyObject *)code;
    int offset = instr_offset(frame, instr);
    /* Offsets visible to users are word indices, like those of dis and of
     * the line and exception tables. */
    PyObject *arg2_obj = PyLong_FromLong(instr_offset(frame, arg2));
    if (arg2_obj == NULL) {
        return -1;
    }
    assert(args[2] == NULL);
    args[2] = arg2_obj;
    PyInterpreterState *interp = tstate->interp;
    uint8_t tools = get_tools_for_instruction(code, interp, offset, event);
    size_t nargsf = (size_t) nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject **callargs = &args[1];
    int err = 0;
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
        assert(tools & (1 << tool));
        tools ^= (1 << tool);
        int res = call_one_instrument(interp, tstate, callargs, nargsf, tool, event);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            err = -1;
            break;
        }
        else {
            /* DISABLE */
            if (PY_MONITORING_IS_INSTRUMENTED_EVENT(event)) {
                _PyEval_StopTheWorld(interp);
                remove_tools(code, offset, event, 1 << tool);
                _PyEval_StartTheWorld(interp);
            }
            else if (_PY_MONITORING_IS_UNGROUPED_EVENT(event)) {
                /* Other (non-instrumented) event: disable for this code object. */
                _PyEval_StopTheWorld(interp);
                remove_local_tool(code, interp, event, tool);
                _PyEval_StartTheWorld(interp);
            }
            else {
                PyErr_Format(PyExc_ValueError,
                              "Cannot disable %s events. Callback removed.",
                             event_names[event]);
                /* Clear tool to prevent infinite loop */
                Py_CLEAR(interp->monitoring_callables[tool][event]);
                err = -1;
                break;
            }
        }
    }
    Py_DECREF(arg2_obj);
    return err;
}

Py_NO_INLINE int
_Py_call_instrumentation(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr)
{
    PyObject *args[3] = { NULL, NULL, NULL };
    return call_instrumentation_vector(instr, tstate, event, frame, instr, 2, args);
}

Py_NO_INLINE int
_Py_call_instrumentation_arg(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyObject *arg)
{
    PyObject *args[4] = { NULL, NULL, NULL, arg };
    return call_instrumentation_vector(instr, tstate, event, frame, instr, 3, args);
}

Py_NO_INLINE int
_Py_call_instrumentation_2args(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyObject *arg0, PyObject *arg1)
{
    PyObject *args[5] = { NULL, NULL, NULL, arg0, arg1 };
    return call_instrumentation_vector(instr, tstate, event, frame, instr, 4, args);
}

Py_NO_INLINE _PyInstr *
_Py_call_instrumentation_jump(
    _PyInstr *instr, PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *src, _PyInstr *dest)
{
    assert(event == PY_MONITORING_EVENT_JUMP ||
           event == PY_MONITORING_EVENT_BRANCH_RIGHT ||
           event == PY_MONITORING_EVENT_BRANCH_LEFT);
    int to = dest_offset(frame, dest);
    PyObject *to_obj = PyLong_FromLong(to);
    if (to_obj == NULL) {
        return NULL;
    }
    PyObject *args[4] = { NULL, NULL, NULL, to_obj };
    _PyInstr *instr_ptr = frame->instr_ptr;
    int err = call_instrumentation_vector(instr, tstate, event, frame, src, 3, args);
    Py_DECREF(to_obj);
    if (err) {
        return NULL;
    }
    if (frame->instr_ptr != instr_ptr) {
        /* The callback has caused a jump (by setting the line number) */
        return frame->instr_ptr;
    }
    return dest;
}

static void
call_instrumentation_vector_protected(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, Py_ssize_t nargs, PyObject *args[])
{
    assert(_PyErr_Occurred(tstate));
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    int err = call_instrumentation_vector(instr, tstate, event, frame, instr, nargs, args);
    if (err) {
        Py_XDECREF(exc);
    }
    else {
        _PyErr_SetRaisedException(tstate, exc);
    }
    assert(_PyErr_Occurred(tstate));
}

Py_NO_INLINE void
_Py_call_instrumentation_exc2(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _PyInstr *instr, PyObject *arg0, PyObject *arg1)
{
    assert(_PyErr_Occurred(tstate));
    PyObject *args[5] = { NULL, NULL, NULL, arg0, arg1 };
    call_instrumentation_vector_protected(tstate, event, frame, instr, 4, args);
}

int
_Py_Instrumentation_GetLine(PyCodeObject *code, _PyCoLineInstrumentationData *line_data, int index)
{
    assert(line_data != NULL);
    assert(index < Py_SIZE(code));
    int line_delta = get_line_delta(line_data, index);
    int line = compute_line(code, line_delta);
    return line;
}

Py_NO_INLINE int
_Py_call_instrumentation_line(PyThreadState *tstate, _PyInterpreterFrame* frame, _PyInstr *instr, _PyInstr *prev)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    assert(tstate->tracing == 0);
    assert(debug_check_sanity(tstate->interp, code));
    int i = instr_offset(frame, instr);

    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    _PyCoLineInstrumentationData *line_data = monitoring->lines;
    PyInterpreterState *interp = tstate->interp;
    int line = _Py_Instrumentation_GetLine(code, line_data, i);
    assert(line >= 0);
    assert(prev != NULL);
    int prev_index = instr_offset(frame, prev);
    int prev_line = _Py_Instrumentation_GetLine(code, line_data, prev_index);
    if (prev_line == line) {
        int prev_opcode = word_opcode(frame->ir_words, prev_index);
        /* RESUME and INSTRUMENTED_RESUME are needed for the operation of
            * instrumentation, so must never be hidden by an INSTRUMENTED_LINE.
            */
        if (prev_opcode != RESUME && prev_opcode != INSTRUMENTED_RESUME) {
            goto done;
        }
    }

    uint8_t tools = code->_co_monitoring->line_tools != NULL ?
        code->_co_monitoring->line_tools[i] :
        (interp->monitors.tools[PY_MONITORING_EVENT_LINE] |
         code->_co_monitoring->local_monitors.tools[PY_MONITORING_EVENT_LINE]
        );
    /* Special case sys.settrace to avoid boxing the line number,
     * only to immediately unbox it. */
    if (tools & (1 << PY_MONITORING_SYS_TRACE_ID)) {
        if (tstate->c_tracefunc != NULL) {
            PyFrameObject *frame_obj = _PyFrame_GetFrameObject(frame);
            if (frame_obj == NULL) {
                return -1;
            }
            if (frame_obj->f_trace_lines) {
                /* Need to set tracing and what_event as if using
                 * the instrumentation call. */
                int old_what = tstate->what_event;
                tstate->what_event = PY_MONITORING_EVENT_LINE;
                tstate->tracing++;
                /* Call c_tracefunc directly, having set the line number. */
                Py_INCREF(frame_obj);
                frame_obj->f_lineno = line;
                int err = tstate->c_tracefunc(tstate->c_traceobj, frame_obj, PyTrace_LINE, Py_None);
                frame_obj->f_lineno = 0;
                tstate->tracing--;
                tstate->what_event = old_what;
                Py_DECREF(frame_obj);
                if (err) {
                    return -1;
                }
            }
        }
        tools &= (255 - (1 << PY_MONITORING_SYS_TRACE_ID));
    }
    if (tools == 0) {
        goto done;
    }
    PyObject *line_obj = PyLong_FromLong(line);
    if (line_obj == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, (PyObject *)code, line_obj };
    do {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < PY_MONITORING_SYS_PROFILE_ID);
        assert(tools & (1 << tool));
        tools &= ~(1 << tool);
        int res = call_one_instrument(interp, tstate, &args[1],
                                      2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                      tool, PY_MONITORING_EVENT_LINE);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            Py_DECREF(line_obj);
            return -1;
        }
        else {
            /* DISABLE  */
            PyInterpreterState *interp = tstate->interp;
            _PyEval_StopTheWorld(interp);
            remove_line_tools(code, i, 1 << tool);
            _PyEval_StartTheWorld(interp);
        }
    } while (tools);
    Py_DECREF(line_obj);
    uint8_t original_opcode;
done:
    original_opcode = _PyCode_GetOriginalOpcode(line_data, i);
    if (IR_HANDLER(*instr) == INSTRUMENTED_INSTRUCTION) {
        /* A LINE callback may have disabled the last LINE tool while
         * leaving INSTRUCTION monitoring enabled. */
        original_opcode = INSTRUMENTED_INSTRUCTION;
    }
    assert(original_opcode != 0);
    assert(original_opcode != INSTRUMENTED_LINE);
    assert(_PyOpcode_Deopt[original_opcode] == original_opcode);
    return original_opcode;
}

Py_NO_INLINE int
_Py_call_instrumentation_instruction(PyThreadState *tstate, _PyInterpreterFrame* frame, _PyInstr *instr)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int offset = instr_offset(frame, instr);
    _PyCoMonitoringData *instrumentation_data = code->_co_monitoring;
    assert(instrumentation_data->per_instruction_opcodes);
    int next_opcode = instrumentation_data->per_instruction_opcodes[offset];
    if (tstate->tracing) {
        return next_opcode;
    }
    assert(debug_check_sanity(tstate->interp, code));
    PyInterpreterState *interp = tstate->interp;
    uint8_t tools = instrumentation_data->per_instruction_tools != NULL ?
        instrumentation_data->per_instruction_tools[offset] :
        (interp->monitors.tools[PY_MONITORING_EVENT_INSTRUCTION] |
         code->_co_monitoring->local_monitors.tools[PY_MONITORING_EVENT_INSTRUCTION]
        );
    PyObject *offset_obj = PyLong_FromLong(offset);
    if (offset_obj == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, (PyObject *)code, offset_obj };
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
        assert(tools & (1 << tool));
        tools &= ~(1 << tool);
        int res = call_one_instrument(interp, tstate, &args[1],
                                      2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                      tool, PY_MONITORING_EVENT_INSTRUCTION);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            Py_DECREF(offset_obj);
            return -1;
        }
        else {
            /* DISABLE  */
            PyInterpreterState *interp = tstate->interp;
            _PyEval_StopTheWorld(interp);
            remove_per_instruction_tools(code, offset, 1 << tool);
            _PyEval_StartTheWorld(interp);
        }
    }
    Py_DECREF(offset_obj);
    assert(next_opcode != 0);
    return next_opcode;
}

static void
initialize_tools(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    uint8_t* tools = code->_co_monitoring->tools;

    assert(tools != NULL);
    int code_len = (int)Py_SIZE(code);
    _PyIRWord *words = exec_words(code);
    for (int i = 0; i < code_len; i++) {
        int opcode = word_opcode(words, i);
        int length = instr_length(code, i);
        if (opcode == INSTRUMENTED_LINE) {
            opcode = _PyCode_GetOriginalOpcode(code->_co_monitoring->lines, i);
        }
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            opcode = code->_co_monitoring->per_instruction_opcodes[i];
        }
        bool instrumented = is_instrumented(opcode);
        if (instrumented) {
            opcode = DE_INSTRUMENT[opcode];
            assert(opcode != 0);
        }
        opcode = _PyOpcode_Deopt[opcode];
        if (opcode_has_event(opcode)) {
            if (instrumented) {
                int8_t event;
                if (opcode == RESUME) {
                    event = _PyCode_InstrOparg(code, i) != 0;
                }
                else {
                    event = EVENT_FOR_OPCODE[opcode];
                    assert(event > 0);
                }
                assert(event >= 0);
                assert(PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
                tools[i] = code->_co_monitoring->active_monitors.tools[event];
                CHECK(tools[i] != 0);
            }
            else {
                tools[i] = 0;
            }
        }
#ifdef Py_DEBUG
        /* Initialize tools for invalid locations to all ones to try to catch errors */
        else {
            tools[i] = 0xff;
        }
        for (int j = 1; j < length; j++) {
            tools[i+j] = 0xff;
        }
#endif
        i += length - 1;
    }
}

static void
initialize_lines(_PyCoLineInstrumentationData *line_data, PyCodeObject *code, int bytes_per_entry)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    assert(line_data != NULL);
    line_data->bytes_per_entry = bytes_per_entry;
    int code_len = (int)Py_SIZE(code);
    PyCodeAddressRange range;
    _PyCode_InitAddressRange(code, &range);
    int current_line = -1;
    for (int i = 0; i < code_len; ) {
        int opcode = base_opcode(code, i);
        int line = _PyCode_CheckLineNumber(i, &range);
        set_line_delta(line_data, i, compute_line_delta(code, line));
        int length = instr_length(code, i);
        if (i < code->_co_firsttraceable) {
            set_original_opcode(line_data, i, 0);
        }
        else {
            switch (opcode) {
                case END_ASYNC_FOR:
                case END_FOR:
                case END_SEND:
                case RESUME:
                case POP_ITER:
                    /* END_FOR cannot start a line, as it is skipped by FOR_ITER
                    * END_SEND cannot start a line, as it is skipped by SEND
                    * RESUME and POP_ITER must not be instrumented with INSTRUMENTED_LINE */
                    set_original_opcode(line_data, i, 0);
                    break;
                default:
                    /* Set original_opcode to the opcode iff the instruction
                    * starts a line, and thus should be instrumented.
                    * This saves having to perform this check every time the
                    * we turn instrumentation on or off, and serves as a sanity
                    * check when debugging.
                    */
                    if (line != current_line && line >= 0) {
                        set_original_opcode(line_data, i, opcode);
                        CHECK(get_line_delta(line_data, i) != NO_LINE);
                    }
                    else {
                        set_original_opcode(line_data, i, 0);
                    }
                    current_line = line;
            }
        }
        for (int j = 1; j < length; j++) {
            set_original_opcode(line_data, i+j, 0);
            set_line_delta(line_data, i+j, NO_LINE);
        }
        i += length;
    }
    for (int i = code->_co_firsttraceable; i < code_len; ) {
        int opcode = base_opcode(code, i);
        int src = i;
        i += instr_length(code, i);
        int target = -1;
        switch (opcode) {
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case POP_JUMP_IF_NONE:
            case POP_JUMP_IF_NOT_NONE:
            case JUMP_FORWARD:
            case JUMP_BACKWARD:
            case JUMP_BACKWARD_NO_INTERRUPT:
            {
                target = jump_dest(code, src);
                break;
            }
            case FOR_ITER:
            case SEND:
            {
                /* Skip over END_FOR/END_SEND */
                target = jump_dest(code, src) + 1;
                break;
            }
            default:
                continue;
        }
        assert(target >= 0);
        if (get_line_delta(line_data, target) != NO_LINE) {
            int opcode = base_opcode(code, target);
            if (opcode != POP_ITER) {
                set_original_opcode(line_data, target, opcode);
            }
        }
    }
    /* Scan exception table */
    unsigned char *start = (unsigned char *)PyBytes_AS_STRING(code->co_exceptiontable);
    unsigned char *end = start + PyBytes_GET_SIZE(code->co_exceptiontable);
    unsigned char *scan = start;
    while (scan < end) {
        int start_offset, size, handler;
        scan = parse_varint(scan, &start_offset);
        assert(start_offset >= 0 && start_offset < code_len);
        scan = parse_varint(scan, &size);
        assert(size >= 0 && start_offset+size <= code_len);
        scan = parse_varint(scan, &handler);
        assert(handler >= 0 && handler < code_len);
        int slot_and_lasti;
        scan = parse_varint(scan, &slot_and_lasti);
        int original_opcode = base_opcode(code, handler);
        /* Skip if not the start of a line.
         * END_ASYNC_FOR is a bit special as it marks the end of
         * an `async for` loop, which should not generate its own
         * line event. */
        if (get_line_delta(line_data, handler) != NO_LINE && original_opcode != END_ASYNC_FOR) {
            set_original_opcode(line_data, handler, original_opcode);
        }
    }
}

static void
initialize_line_tools(PyCodeObject *code, _Py_LocalMonitors *all_events)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    uint8_t *line_tools = code->_co_monitoring->line_tools;

    assert(line_tools != NULL);
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        line_tools[i] = all_events->tools[PY_MONITORING_EVENT_LINE];
    }
}

static int
allocate_instrumentation_data(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    if (code->_co_monitoring == NULL) {
        _PyCoMonitoringData *monitoring = PyMem_Malloc(sizeof(_PyCoMonitoringData));
        if (monitoring == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        monitoring->local_monitors = (_Py_LocalMonitors){ 0 };
        monitoring->active_monitors = (_Py_LocalMonitors){ 0 };
        memset(monitoring->tool_versions, 0, sizeof(monitoring->tool_versions));
        monitoring->tools = NULL;
        monitoring->lines = NULL;
        monitoring->line_tools = NULL;
        monitoring->per_instruction_opcodes = NULL;
        monitoring->per_instruction_tools = NULL;
        _Py_atomic_store_ptr_release(&code->_co_monitoring, monitoring);
    }
    return 0;
}

static int
update_instrumentation_data(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    int code_len = (int)Py_SIZE(code);
    if (allocate_instrumentation_data(code)) {
        return -1;
    }
    // If the local monitors are out of date, clear them up
    _Py_LocalMonitors *local_monitors = &code->_co_monitoring->local_monitors;
    for (int i = 0; i < PY_MONITORING_TOOL_IDS; i++) {
        if (code->_co_monitoring->tool_versions[i] != interp->monitoring_tool_versions[i]) {
            for (int j = 0; j < _PY_MONITORING_UNGROUPED_EVENTS; j++) {
                local_monitors->tools[j] &= ~(1 << i);
            }
        }
    }

    _Py_LocalMonitors all_events = local_union(
        interp->monitors,
        code->_co_monitoring->local_monitors);
    bool multitools = multiple_tools(&all_events);
    if (code->_co_monitoring->tools == NULL && multitools) {
        code->_co_monitoring->tools = PyMem_Malloc(code_len);
        if (code->_co_monitoring->tools == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        initialize_tools(code);
    }
    if (all_events.tools[PY_MONITORING_EVENT_LINE]) {
        if (code->_co_monitoring->lines == NULL) {
            PyCodeAddressRange range;
            _PyCode_InitAddressRange(code, &range);
            int max_line = code->co_firstlineno + 1;
            _PyCode_InitAddressRange(code, &range);
            for (int i = code->_co_firsttraceable; i < code_len; ) {
                int line = _PyCode_CheckLineNumber(i, &range);
                if (line > max_line) {
                    max_line = line;
                }
                i += instr_length(code, i);
            }
            int bytes_per_entry;
            int max_delta = max_line - code->co_firstlineno;
            /* We store delta+2 in the table, so 253 is max for one byte */
            if (max_delta < 256+NO_LINE) {
                bytes_per_entry = 2;
            }
            else if (max_delta < (1 << 16)+NO_LINE) {
                bytes_per_entry = 3;
            }
            else if (max_delta < (1 << 24)+NO_LINE) {
                bytes_per_entry = 4;
            }
            else {
                bytes_per_entry = 5;
            }
            _PyCoLineInstrumentationData *lines = PyMem_Malloc(1 + code_len * bytes_per_entry);
            if (lines == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            initialize_lines(lines, code, bytes_per_entry);
            _Py_atomic_store_ptr_release(&code->_co_monitoring->lines, lines);
        }
        if (multitools && code->_co_monitoring->line_tools == NULL) {
            code->_co_monitoring->line_tools = PyMem_Malloc(code_len);
            if (code->_co_monitoring->line_tools == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            initialize_line_tools(code, &all_events);
        }
    }
    if (all_events.tools[PY_MONITORING_EVENT_INSTRUCTION]) {
        if (code->_co_monitoring->per_instruction_opcodes == NULL) {
            code->_co_monitoring->per_instruction_opcodes = PyMem_Malloc(code_len * sizeof(_PyCoLineInstrumentationData));
            if (code->_co_monitoring->per_instruction_opcodes == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            // Initialize all of the instructions so if local events change while another thread is executing
            // we know what the original opcode was.
            _PyIRWord *words = exec_words(code);
            for (int i = 0; i < code_len; i++) {
                code->_co_monitoring->per_instruction_opcodes[i] = word_opcode(words, i);
            }
        }
        if (multitools && code->_co_monitoring->per_instruction_tools == NULL) {
            code->_co_monitoring->per_instruction_tools = PyMem_Malloc(code_len);
            if (code->_co_monitoring->per_instruction_tools == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            for (int i = 0; i < code_len; i++) {
                code->_co_monitoring->per_instruction_tools[i] = 0;
            }
        }
    }
    return 0;
}

static int
force_instrument_lock_held(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    int code_len = (int)Py_SIZE(code);
    /* Exit early to avoid creating instrumentation
     * data for potential statically allocated code
     * objects.
     * See https://github.com/python/cpython/issues/108390 */
    if (code->co_flags & CO_NO_MONITORING_EVENTS) {
        return 0;
    }
    if (_PyCode_GetExec(code) == NULL) {
        return -1;
    }
    if (update_instrumentation_data(code, interp)) {
        return -1;
    }
    _Py_LocalMonitors active_events = local_union(
        interp->monitors,
        code->_co_monitoring->local_monitors);
    _Py_LocalMonitors new_events;
    _Py_LocalMonitors removed_events;

    bool restarted = interp->last_restart_version > code->_co_instrumentation_version;
    if (restarted) {
        removed_events = code->_co_monitoring->active_monitors;
        new_events = active_events;
    }
    else {
        removed_events = monitors_sub(code->_co_monitoring->active_monitors, active_events);
        new_events = monitors_sub(active_events, code->_co_monitoring->active_monitors);
        assert(monitors_are_empty(monitors_and(new_events, removed_events)));
    }
    code->_co_monitoring->active_monitors = active_events;
    if (monitors_are_empty(new_events) && monitors_are_empty(removed_events)) {
        goto done;
    }
    /* Insert instrumentation */
    for (int i = code->_co_firsttraceable; i < code_len; i += instr_length(code, i)) {
        int base = base_opcode(code, i);
        CHECK(base != 0);
        if (opcode_has_event(base)) {
            int8_t event;
            if (base == RESUME) {
                event = _PyCode_InstrOparg(code, i) > 0;
            }
            else {
                event = EVENT_FOR_OPCODE[base];
                assert(event > 0);
            }
            uint8_t removed_tools = removed_events.tools[event];
            if (removed_tools) {
                remove_tools(code, i, event, removed_tools);
            }
            uint8_t new_tools = new_events.tools[event];
            if (new_tools) {
                add_tools(code, i, event, new_tools);
            }
        }
    }

    // GH-103845: We need to remove both the line and instruction instrumentation before
    // adding new ones, otherwise we may remove the newly added instrumentation.
    uint8_t removed_line_tools = removed_events.tools[PY_MONITORING_EVENT_LINE];
    uint8_t removed_per_instruction_tools = removed_events.tools[PY_MONITORING_EVENT_INSTRUCTION];

    if (removed_line_tools) {
        _PyCoLineInstrumentationData *line_data = code->_co_monitoring->lines;
        for (int i = code->_co_firsttraceable; i < code_len;) {
            if (_PyCode_GetOriginalOpcode(line_data, i)) {
                remove_line_tools(code, i, removed_line_tools);
            }
            i += instr_length(code, i);
        }
    }
    if (removed_per_instruction_tools) {
        for (int i = code->_co_firsttraceable; i < code_len;) {
            int opcode = base_opcode(code, i);
            if (opcode == RESUME || opcode == END_FOR) {
                i += instr_length(code, i);
                continue;
            }
            remove_per_instruction_tools(code, i, removed_per_instruction_tools);
            i += instr_length(code, i);
        }
    }
#ifdef INSTRUMENT_DEBUG
    sanity_check_instrumentation(code);
#endif

    uint8_t new_line_tools = new_events.tools[PY_MONITORING_EVENT_LINE];
    uint8_t new_per_instruction_tools = new_events.tools[PY_MONITORING_EVENT_INSTRUCTION];

    if (new_line_tools) {
        _PyCoLineInstrumentationData *line_data = code->_co_monitoring->lines;
        for (int i = code->_co_firsttraceable; i < code_len;) {
            if (_PyCode_GetOriginalOpcode(line_data, i)) {
                add_line_tools(code, i, new_line_tools);
            }
            i += instr_length(code, i);
        }
    }
    if (new_per_instruction_tools) {
        for (int i = code->_co_firsttraceable; i < code_len;) {
            int opcode = base_opcode(code, i);
            if (opcode == RESUME || opcode == END_FOR) {
                i += instr_length(code, i);
                continue;
            }
            add_per_instruction_tools(code, i, new_per_instruction_tools);
            i += instr_length(code, i);
        }
    }

done:
    FT_ATOMIC_STORE_UINTPTR_RELEASE(code->_co_instrumentation_version,
                                    global_version(interp));

#ifdef INSTRUMENT_DEBUG
    sanity_check_instrumentation(code);
#endif
    return 0;
}

static int
instrument_lock_held(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    if (is_version_up_to_date(code, interp)) {
        assert(
            interp->ceval.instrumentation_version == 0 ||
            instrumentation_cross_checks(interp, code)
        );
        return 0;
    }

    return force_instrument_lock_held(code, interp);
}

int
_Py_Instrument(PyCodeObject *code, PyInterpreterState *interp)
{
    int res;
    LOCK_CODE(code);
    res = instrument_lock_held(code, interp);
    UNLOCK_CODE();
    return res;
}

#define C_RETURN_EVENTS \
    ((1 << PY_MONITORING_EVENT_C_RETURN) | \
     (1 << PY_MONITORING_EVENT_C_RAISE))

#define C_CALL_EVENTS \
    (C_RETURN_EVENTS | (1 << PY_MONITORING_EVENT_CALL))


static int
instrument_all_executing_code_objects(PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED();

    int err = 0;
    _Py_FOR_EACH_TSTATE_BEGIN(interp, ts) {
        _PyInterpreterFrame *frame = ts->current_frame;
        while (frame) {
            if (frame->owner < FRAME_OWNED_BY_INTERPRETER) {
                err = instrument_lock_held(_PyFrame_GetCode(frame), interp);
                if (err) {
                    goto done;
                }
            }
            frame = frame->previous;
        }
    }
done:
    _Py_FOR_EACH_TSTATE_END(interp);
    return err;
}

static void
set_events(_Py_GlobalMonitors *m, int tool_id, _PyMonitoringEventSet events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        uint8_t *tools = &m->tools[e];
        int active = (events >> e) & 1;
        *tools &= ~(1 << tool_id);
        *tools |= (active << tool_id);
    }
}

static void
set_local_events(_Py_LocalMonitors *m, int tool_id, _PyMonitoringEventSet events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        uint8_t *tools = &m->tools[e];
        int val = (events >> e) & 1;
        *tools &= ~(1 << tool_id);
        *tools |= (val << tool_id);
    }
}

static int
check_tool(PyInterpreterState *interp, int tool_id)
{
    if (tool_id < PY_MONITORING_SYS_PROFILE_ID &&
        interp->monitoring_tool_names[tool_id] == NULL)
    {
        PyErr_Format(PyExc_ValueError, "tool %d is not in use", tool_id);
        return -1;
    }
    return 0;
}

/* We share the eval-breaker with flags, so the monitoring
 * version goes in the top 24 bits */
#define MONITORING_VERSION_INCREMENT (1 << _PY_EVAL_EVENTS_BITS)

int
_PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events)
{
    ASSERT_WORLD_STOPPED();
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;
    assert(events < (1 << _PY_MONITORING_UNGROUPED_EVENTS));
    if (check_tool(interp, tool_id)) {
        return -1;
    }

    uint32_t existing_events = get_events(&interp->monitors, tool_id);
    if (existing_events == events) {
        return 0;
    }
    uint32_t new_version = global_version(interp) + MONITORING_VERSION_INCREMENT;
    if (new_version == 0) {
        PyErr_Format(PyExc_OverflowError, "events set too many times");
        return -1;
    }
    set_events(&interp->monitors, tool_id, events);
    set_global_version(tstate, new_version);
    return instrument_all_executing_code_objects(interp);
}

int
_PyMonitoring_SetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet events)
{
    ASSERT_WORLD_STOPPED();

    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(events < (1 << _PY_MONITORING_UNGROUPED_EVENTS));
    if (code->_co_firsttraceable >= Py_SIZE(code)) {
        PyErr_Format(PyExc_SystemError, "cannot instrument shim code object '%U'", code->co_name);
        return -1;
    }
    if (check_tool(interp, tool_id)) {
        return -1;
    }

    if (allocate_instrumentation_data(code)) {
        return -1;
    }

    code->_co_monitoring->tool_versions[tool_id] = interp->monitoring_tool_versions[tool_id];

    _Py_LocalMonitors *local = &code->_co_monitoring->local_monitors;
    uint32_t existing_events = get_local_events(local, tool_id);
    if (existing_events == events) {
        return 0;
    }
    set_local_events(local, tool_id, events);

    return force_instrument_lock_held(code, interp);
}

int
_PyMonitoring_GetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet *events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (check_tool(interp, tool_id)) {
        return -1;
    }
    if (code->_co_monitoring == NULL) {
        *events = 0;
        return 0;
    }
    _Py_LocalMonitors *local = &code->_co_monitoring->local_monitors;
    *events = get_local_events(local, tool_id);
    return 0;
}

int _PyMonitoring_ClearToolId(int tool_id)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyInterpreterState *interp = _PyInterpreterState_GET();

    for (int i = 0; i < _PY_MONITORING_EVENTS; i++) {
        PyObject *func = _PyMonitoring_RegisterCallback(tool_id, i, NULL);
        if (func != NULL) {
            Py_DECREF(func);
        }
    }

    _PyEval_StopTheWorld(interp);
    if (_PyMonitoring_SetEvents(tool_id, 0) < 0) {
        _PyEval_StartTheWorld(interp);
        return -1;
    }

    uint32_t version = global_version(interp) + MONITORING_VERSION_INCREMENT;
    if (version == 0) {
        PyErr_Format(PyExc_OverflowError, "events set too many times");
        _PyEval_StartTheWorld(interp);
        return -1;
    }

    // monitoring_tool_versions[tool_id] is set to latest global version here to
    //   1. invalidate local events on all existing code objects
    //   2. be ready for the next call to set local events
    interp->monitoring_tool_versions[tool_id] = version;

    // Set the new global version so all the code objects can refresh the
    // instrumentation.
    set_global_version(_PyThreadState_GET(), version);
    int res = instrument_all_executing_code_objects(interp);
    _PyEval_StartTheWorld(interp);
    return res;
}

/*[clinic input]
module monitoring
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=37257f5987a360cf]*/
/*[clinic end generated code]*/

#include "clinic/instrumentation.c.h"

static int
check_valid_tool(int tool_id)
{
    if (tool_id < 0 || tool_id >= PY_MONITORING_SYS_PROFILE_ID) {
        PyErr_Format(PyExc_ValueError, "invalid tool %d (must be between 0 and 5)", tool_id);
        return -1;
    }
    return 0;
}

/*[clinic input]
monitoring.use_tool_id

    tool_id: int
    name: object
    /

[clinic start generated code]*/

static PyObject *
monitoring_use_tool_id_impl(PyObject *module, int tool_id, PyObject *name)
/*[clinic end generated code: output=30d76dc92b7cd653 input=ebc453761c621be1]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_ValueError, "tool name must be a str");
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->monitoring_tool_names[tool_id] != NULL) {
        PyErr_Format(PyExc_ValueError, "tool %d is already in use", tool_id);
        return NULL;
    }
    interp->monitoring_tool_names[tool_id] = Py_NewRef(name);
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.clear_tool_id

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_clear_tool_id_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=04defc23470b1be7 input=af643d6648a66163]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (interp->monitoring_tool_names[tool_id] != NULL) {
        if (_PyMonitoring_ClearToolId(tool_id) < 0) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.free_tool_id

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_free_tool_id_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=86c2d2a1219a8591 input=a23fb6be3a8618e9]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (interp->monitoring_tool_names[tool_id] != NULL) {
        if (_PyMonitoring_ClearToolId(tool_id) < 0) {
            return NULL;
        }
    }

    Py_CLEAR(interp->monitoring_tool_names[tool_id]);
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.get_tool

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_get_tool_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=1c05a98b404a9a16 input=eeee9bebd0bcae9d]*/

/*[clinic end generated code]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *name = interp->monitoring_tool_names[tool_id];
    if (name == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(name);
}

/*[clinic input]
monitoring.register_callback


    tool_id: int
    event: int
    func: object
    /

[clinic start generated code]*/

static PyObject *
monitoring_register_callback_impl(PyObject *module, int tool_id, int event,
                                  PyObject *func)
/*[clinic end generated code: output=e64daa363004030c input=df6d70ea4cf81007]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (_Py_popcount32(event) != 1) {
        PyErr_SetString(PyExc_ValueError, "The callback can only be set for one event at a time");
        return NULL;
    }
    int event_id = _Py_bit_length(event)-1;
    if (event_id < 0 || event_id >= _PY_MONITORING_EVENTS) {
        PyErr_Format(PyExc_ValueError, "invalid event %d", event);
        return NULL;
    }
    if (PySys_Audit("sys.monitoring.register_callback", "O", func) < 0) {
        return NULL;
    }
    if (func == Py_None) {
        func = NULL;
    }
    func = _PyMonitoring_RegisterCallback(tool_id, event_id, func);
    if (func == NULL) {
        Py_RETURN_NONE;
    }
    return func;
}

/*[clinic input]
monitoring.get_events -> int

    tool_id: int
    /

[clinic start generated code]*/

static int
monitoring_get_events_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=4450cc13f826c8c0 input=a64b238f76c4b2f7]*/
{
    if (check_valid_tool(tool_id))  {
        return -1;
    }
    _Py_GlobalMonitors *m = &_PyInterpreterState_GET()->monitors;
    _PyMonitoringEventSet event_set = get_events(m, tool_id);
    return event_set;
}

/*[clinic input]
monitoring.set_events

    tool_id: int
    event_set: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_set_events_impl(PyObject *module, int tool_id, int event_set)
/*[clinic end generated code: output=1916c1e49cfb5bdb input=a77ba729a242142b]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (event_set < 0 || event_set >= (1 << _PY_MONITORING_EVENTS)) {
        PyErr_Format(PyExc_ValueError, "invalid event set 0x%x", event_set);
        return NULL;
    }
    if ((event_set & C_RETURN_EVENTS) && (event_set & C_CALL_EVENTS) != C_CALL_EVENTS) {
        PyErr_Format(PyExc_ValueError, "cannot set C_RETURN or C_RAISE events independently");
        return NULL;
    }
    event_set &= ~C_RETURN_EVENTS;
    if (event_set & (1 << PY_MONITORING_EVENT_BRANCH)) {
        event_set &= ~(1 << PY_MONITORING_EVENT_BRANCH);
        event_set |= (1 << PY_MONITORING_EVENT_BRANCH_RIGHT) | (1 << PY_MONITORING_EVENT_BRANCH_LEFT);
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    int err = _PyMonitoring_SetEvents(tool_id, event_set);
    _PyEval_StartTheWorld(interp);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.get_local_events -> int

    tool_id: int
    code: object
    /

[clinic start generated code]*/

static int
monitoring_get_local_events_impl(PyObject *module, int tool_id,
                                 PyObject *code)
/*[clinic end generated code: output=d3e92c1c9c1de8f9 input=bb0f927530386a94]*/
{
    if (!PyCode_Check(code)) {
        PyErr_Format(
            PyExc_TypeError,
            "code must be a code object"
        );
        return -1;
    }
    if (check_valid_tool(tool_id))  {
        return -1;
    }
    _PyMonitoringEventSet event_set = 0;
    _PyCoMonitoringData *data = ((PyCodeObject *)code)->_co_monitoring;
    if (data != NULL) {
        for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
            if ((data->local_monitors.tools[e] >> tool_id) & 1) {
                event_set |= (1 << e);
            }
        }
    }
    return event_set;
}

/*[clinic input]
monitoring.set_local_events

    tool_id: int
    code: object
    event_set: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_set_local_events_impl(PyObject *module, int tool_id,
                                 PyObject *code, int event_set)
/*[clinic end generated code: output=68cc755a65dfea99 input=5655ecd78d937a29]*/
{
    if (!PyCode_Check(code)) {
        PyErr_Format(
            PyExc_TypeError,
            "code must be a code object"
        );
        return NULL;
    }
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if ((event_set & C_RETURN_EVENTS) && (event_set & C_CALL_EVENTS) != C_CALL_EVENTS) {
        PyErr_Format(PyExc_ValueError, "cannot set C_RETURN or C_RAISE events independently");
        return NULL;
    }
    event_set &= ~C_RETURN_EVENTS;
    if (event_set & (1 << PY_MONITORING_EVENT_BRANCH)) {
        event_set &= ~(1 << PY_MONITORING_EVENT_BRANCH);
        event_set |= (1 << PY_MONITORING_EVENT_BRANCH_RIGHT) | (1 << PY_MONITORING_EVENT_BRANCH_LEFT);
    }
    if (event_set < 0 || event_set >= (1 << _PY_MONITORING_UNGROUPED_EVENTS)) {
        PyErr_Format(PyExc_ValueError, "invalid local event set 0x%x", event_set);
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyEval_StopTheWorld(interp);
    int err = _PyMonitoring_SetLocalEvents((PyCodeObject*)code, tool_id, event_set);
    _PyEval_StartTheWorld(interp);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.restart_events

[clinic start generated code]*/

static PyObject *
monitoring_restart_events_impl(PyObject *module)
/*[clinic end generated code: output=e025dd5ba33314c4 input=add8a855063c8008]*/
{
    /* We want to ensure that:
     * last restart version > instrumented version for all code objects
     * last restart version < current version
     */
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;

    _PyEval_StopTheWorld(interp);
    uint32_t restart_version = global_version(interp) + MONITORING_VERSION_INCREMENT;
    uint32_t new_version = restart_version + MONITORING_VERSION_INCREMENT;
    if (new_version <= MONITORING_VERSION_INCREMENT) {
        _PyEval_StartTheWorld(interp);
        PyErr_Format(PyExc_OverflowError, "events set too many times");
        return NULL;
    }
    interp->last_restart_version = restart_version;
    set_global_version(tstate, new_version);
    int res = instrument_all_executing_code_objects(interp);
    _PyEval_StartTheWorld(interp);

    if (res) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
add_power2_constant(PyObject *obj, const char *name, int i)
{
    PyObject *val = PyLong_FromLong(1<<i);
    if (val == NULL) {
        return -1;
    }
    int err = PyObject_SetAttrString(obj, name, val);
    Py_DECREF(val);
    return err;
}

/*[clinic input]
monitoring._all_events
[clinic start generated code]*/

static PyObject *
monitoring__all_events_impl(PyObject *module)
/*[clinic end generated code: output=6b7581e2dbb690f6 input=62ee9672c17b7f0e]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        uint8_t tools = interp->monitors.tools[e];
        if (tools == 0) {
            continue;
        }
        PyObject *tools_obj = PyLong_FromLong(tools);
        assert(tools_obj != NULL);
        int err = PyDict_SetItemString(res, event_names[e], tools_obj);
        Py_DECREF(tools_obj);
        if (err < 0) {
            Py_DECREF(res);
            return NULL;
        }
    }
    return res;
}

static PyMethodDef methods[] = {
    MONITORING_USE_TOOL_ID_METHODDEF
    MONITORING_CLEAR_TOOL_ID_METHODDEF
    MONITORING_FREE_TOOL_ID_METHODDEF
    MONITORING_GET_TOOL_METHODDEF
    MONITORING_REGISTER_CALLBACK_METHODDEF
    MONITORING_GET_EVENTS_METHODDEF
    MONITORING_SET_EVENTS_METHODDEF
    MONITORING_GET_LOCAL_EVENTS_METHODDEF
    MONITORING_SET_LOCAL_EVENTS_METHODDEF
    MONITORING_RESTART_EVENTS_METHODDEF
    MONITORING__ALL_EVENTS_METHODDEF
    {NULL, NULL}  // sentinel
};

static struct PyModuleDef monitoring_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "sys.monitoring",
    .m_size = -1, /* multiple "initialization" just copies the module dict. */
    .m_methods = methods,
};

PyObject *_Py_CreateMonitoringObject(void)
{
    PyObject *mod = _PyModule_CreateInitialized(&monitoring_module, PYTHON_API_VERSION);
    if (mod == NULL) {
        return NULL;
    }
    if (PyObject_SetAttrString(mod, "DISABLE", &_PyInstrumentation_DISABLE)) {
        goto error;
    }
    if (PyObject_SetAttrString(mod, "MISSING", &_PyInstrumentation_MISSING)) {
        goto error;
    }
    PyObject *events = _PyNamespace_New(NULL);
    if (events == NULL) {
        goto error;
    }
    int err = PyObject_SetAttrString(mod, "events", events);
    Py_DECREF(events);
    if (err) {
        goto error;
    }
    for (int i = 0; i < _PY_MONITORING_EVENTS; i++) {
        if (add_power2_constant(events, event_names[i], i)) {
            goto error;
        }
    }
    err = PyObject_SetAttrString(events, "NO_EVENTS", _PyLong_GetZero());
    if (err) goto error;
    PyObject *val = PyLong_FromLong(PY_MONITORING_DEBUGGER_ID);
    assert(val != NULL); /* Can't return NULL because the int is small. */
    err = PyObject_SetAttrString(mod, "DEBUGGER_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    val = PyLong_FromLong(PY_MONITORING_COVERAGE_ID);
    assert(val != NULL);
    err = PyObject_SetAttrString(mod, "COVERAGE_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    val = PyLong_FromLong(PY_MONITORING_PROFILER_ID);
    assert(val != NULL);
    err = PyObject_SetAttrString(mod, "PROFILER_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    val = PyLong_FromLong(PY_MONITORING_OPTIMIZER_ID);
    assert(val != NULL);
    err = PyObject_SetAttrString(mod, "OPTIMIZER_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    return mod;
error:
    Py_DECREF(mod);
    return NULL;
}


static int
capi_call_instrumentation(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                          PyObject **args, Py_ssize_t nargs, int event)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;

    uint8_t tools = state->active;
    assert(args[1] == NULL);
    args[1] = codelike;
    if (offset < 0) {
        PyErr_SetString(PyExc_ValueError, "offset must be non-negative");
        return -1;
    }
    PyObject *offset_obj = NULL;
    if (event != PY_MONITORING_EVENT_LINE) {
        offset_obj = PyLong_FromLong(offset);
        if (offset_obj == NULL) {
            return -1;
        }
        assert(args[2] == NULL);
        args[2] = offset_obj;
    }
    size_t nargsf = (size_t) nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject **callargs = &args[1];
    int err = 0;

    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
        assert(tools & (1 << tool));
        tools ^= (1 << tool);
        int res = call_one_instrument(interp, tstate, callargs, nargsf, tool, event);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            err = -1;
            break;
        }
        else {
            /* DISABLE */
            if (!PY_MONITORING_IS_INSTRUMENTED_EVENT(event)) {
                PyErr_Format(PyExc_ValueError,
                             "Cannot disable %s events. Callback removed.",
                             event_names[event]);
                /* Clear tool to prevent infinite loop */
                Py_CLEAR(interp->monitoring_callables[tool][event]);
                err = -1;
                break;
            }
            else {
                state->active &= ~(1 << tool);
            }
        }
    }
    Py_XDECREF(offset_obj);
    return err;
}

int
PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version,
                         const uint8_t *event_types, Py_ssize_t length)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (global_version(interp) == *version) {
        return 0;
    }

    _Py_GlobalMonitors *m = &interp->monitors;
    for (Py_ssize_t i = 0; i < length; i++) {
        int event = event_types[i];
        state_array[i].active = m->tools[event];
    }
    *version = global_version(interp);
    return 0;
}

int
PyMonitoring_ExitScope(void)
{
    return 0;
}

int
_PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    assert(state->active);
    PyObject *args[3] = { NULL, NULL, NULL };
    return capi_call_instrumentation(state, codelike, offset, args, 2,
                                     PY_MONITORING_EVENT_PY_START);
}

int
_PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    assert(state->active);
    PyObject *args[3] = { NULL, NULL, NULL };
    return capi_call_instrumentation(state, codelike, offset, args, 2,
                                     PY_MONITORING_EVENT_PY_RESUME);
}



int
_PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                                PyObject* retval)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, retval };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_PY_RETURN);
}

int
_PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject* retval)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, retval };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_PY_YIELD);
}

int
_PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            PyObject* callable, PyObject *arg0)
{
    assert(state->active);
    PyObject *args[5] = { NULL, NULL, NULL, callable, arg0 };
    return capi_call_instrumentation(state, codelike, offset, args, 4,
                                     PY_MONITORING_EVENT_CALL);
}

int
_PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            int lineno)
{
    assert(state->active);
    PyObject *lno = PyLong_FromLong(lineno);
    if (lno == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, NULL, lno };
    int res= capi_call_instrumentation(state, codelike, offset, args, 2,
                                       PY_MONITORING_EVENT_LINE);
    Py_DECREF(lno);
    return res;
}

int
_PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            PyObject *target_offset)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, target_offset };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_JUMP);
}

int
_PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, target_offset };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_BRANCH_RIGHT);
}

int
_PyMonitoring_FireBranchRightEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, target_offset };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_BRANCH_RIGHT);
}

int
_PyMonitoring_FireBranchLeftEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, target_offset };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_BRANCH_LEFT);
}

int
_PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject *retval)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, retval };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_C_RETURN);
}

static inline int
exception_event_setup(PyObject **exc, int event) {
    *exc = PyErr_GetRaisedException();
    if (*exc == NULL) {
        PyErr_Format(PyExc_ValueError,
                     "Firing event %d with no exception set",
                     event);
        return -1;
    }
    return 0;
}


static inline int
exception_event_teardown(int err, PyObject *exc) {
    if (err == 0) {
        PyErr_SetRaisedException(exc);
    }
    else {
        assert(PyErr_Occurred());
        Py_XDECREF(exc);
    }
    return err;
}

int
_PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_PY_THROW;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_RAISE;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_C_RAISE;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_RERAISE;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_EXCEPTION_HANDLED;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_PY_UNWIND;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *value)
{
    int event = PY_MONITORING_EVENT_STOP_ITERATION;
    assert(state->active);
    assert(!PyErr_Occurred());
    PyErr_SetObject(PyExc_StopIteration, value);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    Py_DECREF(exc);
    return exception_event_teardown(err, NULL);
}



/* Handle legacy BRANCH event */

typedef struct _PyLegacyBranchEventHandler {
    PyObject_HEAD
    vectorcallfunc vectorcall;
    PyObject *handler;
    bool right;
    int tool_id;
} _PyLegacyBranchEventHandler;

#define _PyLegacyBranchEventHandler_CAST(op)    ((_PyLegacyBranchEventHandler *)(op))

static void
dealloc_branch_handler(PyObject *op)
{
    _PyLegacyBranchEventHandler *self = _PyLegacyBranchEventHandler_CAST(op);
    Py_CLEAR(self->handler);
    PyObject_Free(self);
}

static PyTypeObject _PyLegacyBranchEventHandler_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "sys.monitoring.branch_event_handler",
    sizeof(_PyLegacyBranchEventHandler),
    .tp_dealloc = dealloc_branch_handler,
    .tp_vectorcall_offset = offsetof(_PyLegacyBranchEventHandler, vectorcall),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_HAVE_VECTORCALL | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_call = PyVectorcall_Call,
};


static PyObject *
branch_handler_vectorcall(
    PyObject *op, PyObject *const *args,
    size_t nargsf, PyObject *kwnames
) {
    _PyLegacyBranchEventHandler *self = _PyLegacyBranchEventHandler_CAST(op);
    // Find the other instrumented instruction and remove tool
    // The spec (PEP 669) allows spurious events after a DISABLE,
    // so a best effort is good enough.
    assert(PyVectorcall_NARGS(nargsf) >= 3);
    PyCodeObject *code = (PyCodeObject *)args[0];
    int src_offset = PyLong_AsLong(args[1]);
    if (PyErr_Occurred()) {
        return NULL;
    }
    if (src_offset < 0 || src_offset >= Py_SIZE(code) || code->_co_exec == NULL) {
        return &_PyInstrumentation_DISABLE;
    }
    int opcode = word_opcode(exec_words(code), src_offset);
    if (!is_instrumented(opcode)) {
        /* Already disabled */
        return &_PyInstrumentation_DISABLE;
    }
    PyObject *res = PyObject_Vectorcall(self->handler, args, nargsf, kwnames);
    if (res == &_PyInstrumentation_DISABLE) {
        int offset;
        int other_event;
        if (opcode == FOR_ITER) {
            if (self->right) {
                offset = src_offset;
                other_event = PY_MONITORING_EVENT_BRANCH_LEFT;
            }
            else {
                // We don't know where the POP_ITER is, so
                // we cannot de-instrument it.
                return res;
            }
        }
        else if (IS_CONDITIONAL_JUMP_OPCODE(opcode)) {
            if (self->right) {
                offset = src_offset + 1;
                other_event = PY_MONITORING_EVENT_BRANCH_LEFT;
                assert(base_opcode(code, offset) == NOT_TAKEN);
            }
            else {
                offset = src_offset;
                other_event = PY_MONITORING_EVENT_BRANCH_RIGHT;
            }
        }
        else {
            // Orphaned NOT_TAKEN -- Jump removed by the compiler
            return res;
        }
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _PyEval_StopTheWorld(interp);
        remove_tools(code, offset, other_event, 1 << self->tool_id);
        _PyEval_StartTheWorld(interp);
    }
    return res;
}

static PyObject *make_branch_handler(int tool_id, PyObject *handler, bool right)
{
    _PyLegacyBranchEventHandler *callback =
        PyObject_NEW(_PyLegacyBranchEventHandler, &_PyLegacyBranchEventHandler_Type);
    if (callback == NULL) {
        return NULL;
    }
    callback->vectorcall = branch_handler_vectorcall;
    callback->handler = Py_NewRef(handler);
    callback->right = right;
    callback->tool_id = tool_id;
    return (PyObject *)callback;
}

PyObject *
_PyMonitoring_RegisterCallback(int tool_id, int event_id, PyObject *obj)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    assert(0 <= event_id && event_id < _PY_MONITORING_EVENTS);
    PyObject *res;
    if (event_id == PY_MONITORING_EVENT_BRANCH) {
        PyObject *left, *right;
        if (obj == NULL) {
            left = NULL;
            right = NULL;
        }
        else {
            right = make_branch_handler(tool_id, obj, true);
            if (right == NULL) {
                return NULL;
            }
            left = make_branch_handler(tool_id, obj, false);
            if (left == NULL) {
                Py_DECREF(right);
                return NULL;
            }
        }
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _PyEval_StopTheWorld(interp);
        PyObject *old_right = interp->monitoring_callables[tool_id][PY_MONITORING_EVENT_BRANCH_RIGHT];
        interp->monitoring_callables[tool_id][PY_MONITORING_EVENT_BRANCH_RIGHT] = right;
        res = interp->monitoring_callables[tool_id][PY_MONITORING_EVENT_BRANCH_LEFT];
        interp->monitoring_callables[tool_id][PY_MONITORING_EVENT_BRANCH_LEFT] = left;
        _PyEval_StartTheWorld(interp);
        Py_XDECREF(old_right);
    }
    else {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _PyEval_StopTheWorld(interp);
        res = interp->monitoring_callables[tool_id][event_id];
        interp->monitoring_callables[tool_id][event_id] = Py_XNewRef(obj);
        _PyEval_StartTheWorld(interp);
    }
    if (res != NULL && Py_TYPE(res) == &_PyLegacyBranchEventHandler_Type) {
        _PyLegacyBranchEventHandler *wrapper = (_PyLegacyBranchEventHandler *)res;
        res = Py_NewRef(wrapper->handler);
        Py_DECREF(wrapper);
    }
    return res;
}

/* Branch Iterator */

typedef struct {
    PyObject_HEAD
    PyCodeObject *bi_code;
    int bi_offset;
} branchesiterator;

#define branchesiterator_CAST(op)   ((branchesiterator *)(op))

static PyObject *
int_triple(int a, int b, int c) {
    PyObject *obja = PyLong_FromLong(a);
    PyObject *objb = NULL;
    PyObject *objc = NULL;
    if (obja == NULL) {
        goto error;
    }
    objb = PyLong_FromLong(b);
    if (objb == NULL) {
        goto error;
    }
    objc = PyLong_FromLong(c);
    if (objc == NULL) {
        goto error;
    }
    PyObject *array[3] = { obja, objb, objc };
    return _PyTuple_FromArraySteal(array, 3);
error:
    Py_XDECREF(obja);
    Py_XDECREF(objb);
    Py_XDECREF(objc);
    return NULL;
}

static PyObject *
branchesiter_next(PyObject *op)
{
    branchesiterator *bi = branchesiterator_CAST(op);
    PyCodeObject *code = bi->bi_code;
    int offset = bi->bi_offset;
    while (offset < Py_SIZE(code)) {
        int opcode = base_opcode(code, offset);
        int next_offset = offset + instr_length(code, offset);
        switch (opcode) {
            case FOR_ITER: {
                bi->bi_offset = next_offset;
                int target = jump_dest(code, offset) + 2; // Skips END_FOR and POP_ITER
                return int_triple(offset, next_offset, target);
            }
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case POP_JUMP_IF_NONE:
            case POP_JUMP_IF_NOT_NONE: {
                /* Skip NOT_TAKEN */
                int not_taken = next_offset + 1;
                bi->bi_offset = not_taken;
                return int_triple(offset, not_taken, jump_dest(code, offset));
            }
            case END_ASYNC_FOR: {
                // The END_SEND of the SEND that END_ASYNC_FOR names
                int src_offset = _PyCode_InstrTarget(code, offset);
                bi->bi_offset = next_offset;
                assert(base_opcode(code, src_offset) == END_SEND);
                assert(base_opcode(code, src_offset + 1) == NOT_TAKEN);
                int not_taken = src_offset + 2;
                return int_triple(src_offset, not_taken, next_offset);
            }
        }
        offset = next_offset;
    }
    return NULL;
}

static void
branchesiter_dealloc(PyObject *op)
{
    branchesiterator *bi = branchesiterator_CAST(op);
    Py_DECREF(bi->bi_code);
    PyObject_Free(bi);
}

static PyTypeObject _PyBranchesIterator = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "line_iterator",                    /* tp_name */
    sizeof(branchesiterator),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    .tp_dealloc = branchesiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = branchesiter_next,
    .tp_free = PyObject_Del,
};

PyObject *
_PyInstrumentation_BranchesIterator(PyCodeObject *code)
{

    branchesiterator *bi = (branchesiterator *)PyType_GenericAlloc(&_PyBranchesIterator, 0);
    if (bi == NULL) {
        return NULL;
    }
    bi->bi_code = (PyCodeObject*)Py_NewRef(code);
    bi->bi_offset = 0;
    return (PyObject *)bi;
}
