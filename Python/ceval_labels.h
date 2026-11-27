/* The labels and non-arm handlers of the interpreter, each written once.
 *
 * Python/ceval_forms.c includes this file with IR_FUNCTION_FORMS defined,
 * for the function form of each (ir_run_<handler> or ir_<label>), and
 * Python/ceval_evaluator.h as it includes the generated handlers, for the
 * tail-calling interpreter's function of each (IR_TAIL_FN,
 * Python/ceval_tail.h).  The macros below open and close the shapes, so the
 * body is one text for both. */

#ifdef IR_FUNCTION_FORMS
#  define IR_HANDLER_BEGIN(name) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_run_##name(PyThreadState *tstate, _PyInterpreterFrame *frame, \
                  _PyInstr *next_instr, _PyIRWord ir_word) {
#  define IR_ENTRY_BEGIN(name) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_##name(PyThreadState *tstate, _PyInterpreterFrame *frame, \
              _PyInstr *next_instr, _PyIRWord ir_word) {
#else
#  define IR_HANDLER_BEGIN(name) IR_TAIL_FN(name) {
#  define IR_ENTRY_BEGIN(name) IR_TAIL_FN(name) {
#endif
#define IR_END }

/* The word kind that holds operands: its id is inert, so a word that is
 * zeroed or reached by a jump into operand words aborts. */
IR_HANDLER_BEGIN(operand_words)
    Py_FatalError("Executing operand words.");
IR_END

/* The target of every id without a handler */
IR_HANDLER_BEGIN(UNKNOWN_OPCODE)
    int opcode = IR_HANDLER(*next_instr);
    _PyErr_Format(tstate, PyExc_SystemError,
                  "%U:%d: unknown opcode %d",
                  _PyFrame_GetCode(frame)->co_filename,
                  PyUnstable_InterpreterFrame_GetLine(frame),
                  opcode);
    PROCESS_ERROR_AT(RAISE, 0, next_instr);
IR_END

IR_ENTRY_BEGIN(start_frame)
    IR_START_FRAME();
IR_END

IR_ENTRY_BEGIN(throw_error)
    PROCESS_ERROR_AT(THROW, 0, next_instr - 1);
IR_END

#undef IR_HANDLER_BEGIN
#undef IR_ENTRY_BEGIN
#undef IR_END
