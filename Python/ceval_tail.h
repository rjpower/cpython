/* The tail-calling binding of the interpreter (Python/ceval_evaluator.h),
 * which Python/ceval.c compiles for the normal evaluator and
 * Python/ceval_observing.c, with IR_OBSERVING set, for the observing one.
 *
 * Every handler, label and VM routine is a function of one type,
 * _PyIRTailFn: (tstate, frame, next_instr, ir_word), returning the
 * activation's result.  Dispatch is a guaranteed tail call of the next word's
 * function through the compilation's table, ir_tail_table, so every arm has
 * its own register allocation and no dispatch grows the C stack.  The
 * bodies are the generated arms (Python/generated_rop_cases.c.h), the labels
 * (Python/ceval_labels.h) and the routines (Python/ceval_routines.h); this
 * file rebinds the macros through which they leave (Python/ceval_macros.h
 * has the bindings that the bodies share with the function forms):
 *
 *   dispatch, a deoptimisation, a frame entry, an error, a routine, a switch
 *   to the other evaluator
 *       a tail call (Py_MUSTTAIL), of a function of the same type;
 *   the end of the activation (IR_EXIT)
 *       a return of the value, to the driver, ir_evaluate();
 *   a call that returns a transfer (a generator step, an outlined form, an
 *   observed edge, the baseline JIT's jit_run())
 *       an ordinary call, which has returned before the arm dispatches its
 *       result.
 *
 * A routine's operands, and the error routine's origin and entry, travel in
 * the activation (IR_ROUTINE_OPERAND), as for the function forms.  The
 * functions of the normal evaluator are ir_tail_<name>, those of the
 * observing one ir_observe_<name>.
 *
 * The functions save no register for their caller, which is what keeps an
 * arm from saving registers it uses at its entry.  Clang
 * (preserve_none) also passes the four arguments in r12-r15, which ordinary
 * C functions preserve, so an arm keeps the interpreter's state across its
 * calls without saving it.  GCC 15 has no preserve_none, but
 * no_callee_saved_registers (x86-64) gives the first property: the arguments
 * stay in the SysV registers.  Only the driver, ir_evaluate(), saves its
 * caller's registers, once per activation.  configure requires a compiler
 * with musttail (GCC 15 or clang). */

#if defined(__clang__) && __has_attribute(preserve_none)
#  define Py_IR_TAIL_CC __attribute__((preserve_none))
#elif !defined(__clang__) && defined(__x86_64__) && \
      __has_attribute(no_callee_saved_registers)
#  define Py_IR_TAIL_CC __attribute__((no_callee_saved_registers))
#else
#  define Py_IR_TAIL_CC
#endif

#define Py_IR_TAIL_PARAMS \
    PyThreadState *tstate, _PyInterpreterFrame *frame, \
    _PyInstr *next_instr, _PyIRWord ir_word

typedef PyValue (Py_IR_TAIL_CC *_PyIRTailFn)(Py_IR_TAIL_PARAMS);

/* The function of NAME in this compilation: ir_tail_NAME, or
 * ir_observe_NAME in the observing evaluator */
#if IR_OBSERVING
#  define IR_TAIL_PREFIX ir_observe_
#else
#  define IR_TAIL_PREFIX ir_tail_
#endif
#define IR_TAIL_CAT_(A, B) A##B
#define IR_TAIL_CAT(A, B) IR_TAIL_CAT_(A, B)
#define IR_TAIL_ID(NAME) IR_TAIL_CAT(IR_TAIL_PREFIX, NAME)

#define IR_TAIL_HEAD(FN) \
    static Py_NO_INLINE Py_IR_TAIL_CC PyValue FN(Py_IR_TAIL_PARAMS)
#define IR_TAIL_FN(NAME) IR_TAIL_HEAD(IR_TAIL_ID(NAME))

#define IR_TAIL_GOTO(NAME) \
    do { \
        Py_MUSTTAIL return IR_TAIL_ID(NAME)(tstate, frame, next_instr, ir_word); \
    } while (0)

/* The entries of the two evaluators, where a frame goes on at next_instr
 * after the other one switched it (the switch word, IR_H_SWITCH, or an
 * error that unwinds into a frame of the other evaluator) */
extern Py_IR_TAIL_CC PyValue _PyEval_EnterNormal(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_EnterObserving(Py_IR_TAIL_PARAMS);
#if IR_OBSERVING
#  define IR_THIS_EVALUATOR _PyEval_EnterObserving
#  define IR_OTHER_EVALUATOR _PyEval_EnterNormal
#else
#  define IR_THIS_EVALUATOR _PyEval_EnterNormal
#  define IR_OTHER_EVALUATOR _PyEval_EnterObserving
#endif

/* The evaluator's switch where an error routine resumes frame at
 * next_instr: to the other evaluator if the frame goes on there */
#undef IR_ERROR_SWITCH
#define IR_ERROR_SWITCH() \
    do { \
        if (_PyEval_SwitchesLoop(frame, IR_OBSERVING)) { \
            Py_MUSTTAIL return IR_OTHER_EVALUATOR(tstate, frame, next_instr, ir_word); \
        } \
    } while (0)

#undef TARGET
#define TARGET(op) IR_TAIL_FN(op)

#undef DISPATCH_GOTO
#define DISPATCH_GOTO() \
    do { \
        Py_MUSTTAIL return ir_tail_table[opcode](tstate, frame, next_instr, ir_word); \
    } while (0)

#undef JUMP_TO_LABEL
#define JUMP_TO_LABEL(name) IR_TAIL_GOTO(name)

#undef JUMP_TO_PREDICTED
#define JUMP_TO_PREDICTED(name) \
    do { \
        Py_MUSTTAIL return IR_TAIL_ID(name)(tstate, frame, this_instr, ir_word); \
    } while (0)

#undef PROCESS_ERROR_AT
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_next = (origin); \
        ir_exit->exit_entry = \
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        IR_TAIL_GOTO(process_error); \
    } while (0)

/* A called function returned frame and next word, or no word for an error
 * that the activation describes */
#undef IR_STEP_DONE
#define IR_STEP_DONE(STEP) \
    do { \
        _PyIRTransfer ir_t = (STEP); \
        frame = ir_t.frame; \
        next_instr = ir_t.next; \
        if (next_instr == NULL) { \
            IR_TAIL_GOTO(process_error); \
        } \
        int opcode; \
        DISPATCH(); \
    } while (0)

#undef IR_RUN_OUTLINED
#define IR_RUN_OUTLINED(FN) IR_STEP_DONE(FN(tstate, frame, next_instr, ir_word))

#undef IR_OBSERVE_EDGE
#define IR_OBSERVE_EDGE(SOURCE, NEXT, EVENT, EDGE) \
    IR_STEP_DONE(_PyEval_ir_event_edge(tstate, frame, this_instr, \
                                       (SOURCE), (NEXT), (EVENT), (EDGE)))

#undef IR_GEN_RETURN
#define IR_GEN_RETURN(V, DYING, EMPTY) \
    do { \
        _PyEvalActivation *ir_gen = IR_ACTIVATION(); \
        ir_gen->gen_value = (V); \
        ir_gen->gen_dying = (DYING); \
        ir_gen->gen_empty = (EMPTY); \
        IR_TAIL_GOTO(gen_return); \
    } while (0)

#undef IR_ROUTINE
#define IR_ROUTINE(NAME) IR_TAIL_FN(NAME)
#undef IR_HOT_ROUTINE
#define IR_HOT_ROUTINE(NAME) IR_TAIL_FN(NAME)
#undef IR_ROUTINE_OPERAND
#define IR_ROUTINE_OPERAND(NAME) (IR_ACTIVATION()->NAME)

#undef GEN_DISPATCH
#define GEN_DISPATCH() \
    do { \
        int opcode; \
        DISPATCH(); \
    } while (0)
#undef GEN_STEP_JUMP
#define GEN_STEP_JUMP(NAME, F, IP, V) \
    IR_STEP_DONE(_PyEval_ir_##NAME(tstate, (F), (IP), _PyValue_ToTransferWord(V)))
/* The completion arm reported the error in the activation */
#undef GEN_RAISED
#define GEN_RAISED() IR_TAIL_GOTO(process_error)
