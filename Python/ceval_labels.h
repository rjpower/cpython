/* The labels and non-arm handlers of the interpreter, each written once.
 *
 * Python/ceval.c includes this file twice, as it includes the generated
 * handlers: with IR_FUNCTION_FORMS defined, for the function form of each
 * (ir_run_<handler> or ir_<label>), and in ir_evaluate(), for its TARGET or
 * LABEL.  The macros below open and close the two shapes, so the body is one
 * text for both. */

#ifdef IR_FUNCTION_FORMS
#  define IR_HANDLER_BEGIN(name) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_run_##name(PyThreadState *tstate, _PyInterpreterFrame *frame, \
                  _PyInstr *next_instr, _PyIRWord ir_word) {
#  define IR_ENTRY_BEGIN(name) \
    static Py_NO_INLINE _PyIRTransfer \
    ir_##name(PyThreadState *tstate, _PyInterpreterFrame *frame, \
              _PyInstr *next_instr) {
#  define IR_END }
#else
#  define IR_HANDLER_BEGIN(name) TARGET_##name: {
#  define IR_ENTRY_BEGIN(name) LABEL_##name: {
#  define IR_END }
#endif

/* The word kind that holds operands: its id is inert, so a word that is
 * zeroed or reached by a jump into operand words aborts. */
IR_HANDLER_BEGIN(IR_OPERANDS)
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
    int too_deep = _Py_EnterRecursivePy(tstate);
    if (too_deep) {
        PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr);
    }
    DTRACE_FUNCTION_ENTRY();
    next_instr = frame->instr_ptr;
    ROP_ENTER_GATE();
#ifdef Py_DEBUG
    int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS());
    if (lltrace < 0) {
        PROCESS_ERROR_AT(EXIT, 0, frame->instr_ptr);
    }
    frame->lltrace = lltrace;
    assert(!_PyErr_Occurred(tstate));
#endif
    int opcode;
    DISPATCH();
IR_END

IR_ENTRY_BEGIN(throw_error)
    PROCESS_ERROR_AT(THROW, 0, next_instr - 1);
IR_END

#ifndef IR_FUNCTION_FORMS
/* The exits that only the evaluator reaches by label */
IR_ENTRY_BEGIN(error)
    PROCESS_ERROR_AT(RAISE, 0, next_instr - 1);
IR_END

IR_ENTRY_BEGIN(exception_unwind)
    PROCESS_ERROR_AT(UNWIND, 0, next_instr - 1);
IR_END

IR_ENTRY_BEGIN(exit_unwind)
    PROCESS_ERROR_AT(EXIT, 0, frame->instr_ptr);
IR_END

IR_ENTRY_BEGIN(exit_unwind_notrace)
    PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr);
IR_END
#endif

#undef IR_HANDLER_BEGIN
#undef IR_ENTRY_BEGIN
#undef IR_END
