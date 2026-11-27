/* The tail-calling binding of the interpreter (Python/ceval_evaluator.h),
 * which Python/ceval_normal.c compiles for the normal evaluator and
 * Python/ceval_observing.c, with IR_OBSERVING set, for the observing one.
 *
 * Every handler, label and VM routine is a function of one type,
 * _PyIRTailFn: (tstate, frame, next_instr, ir_word), returning the
 * activation's result.  Dispatch is a guaranteed tail call of the next word's
 * function through the compilation's table, ir_tail_table, so every arm has
 * its own register allocation and no dispatch grows the C stack.  The
 * bodies are the generated arms (Python/generated_rop_cases.c.h), the labels
 * (Python/ceval_labels.h) and the routines (Python/ceval_routines.h); the
 * macros through which they leave are bound in Python/ceval_macros.h:
 *
 *   dispatch, a deoptimisation, a frame entry, an error, a routine, a switch
 *   to the other evaluator
 *       a tail call (Py_MUSTTAIL), of a function of the same type;
 *   the end of the activation (IR_EXIT)
 *       a return of the value, to the driver, ir_evaluate();
 *   a call that returns a transfer (a generator step, an observed edge)
 *       an ordinary call, which has returned before the arm dispatches its
 *       result.
 *
 * A routine's operands, and the error routine's origin and entry, travel in
 * the activation (IR_ROUTINE_OPERAND).  The functions of the normal
 * evaluator are ir_tail_<name>, those of the observing one ir_observe_<name>.
 *
 * The functions save no register for their caller, which is what keeps an
 * arm from saving registers it uses at its entry.  Clang
 * (preserve_none) also passes the four arguments in r12-r15, which ordinary
 * C functions preserve, so an arm keeps the interpreter's state across its
 * calls without saving it.  GCC 15 has no preserve_none, but
 * no_callee_saved_registers (x86-64) gives the first property: the arguments
 * stay in the SysV registers.  Only the driver, ir_evaluate(), saves its
 * caller's registers, once per activation.  configure requires a compiler
 * with musttail (GCC 15 or clang).
 *
 * Frames: a function reached by a tail call has no caller to return to, so
 * it needs no frame record of its own for a frame-pointer unwinder (perf,
 * the perf trampoline).  The two units of the evaluator hold its functions
 * and, in the normal one, the C rows that its arms inline
 * (Python/ceval_normal.c).  A build with frame pointers compiles both
 * without frames and with the frame pointer reserved (CFLAGS_EVALUATOR,
 * Makefile.pre.in): no function of theirs writes rbp, which keeps the
 * record of the driver's frame.  The record of a C function that an arm
 * calls links, through the arm's return address, to the frame below the
 * driver, _PyEval_EvalFrameLoop().  A frame-pointer unwinder shows the arm
 * and leaves out only the driver, which called the first function; below a
 * row that an arm calls out of line, it leaves out the arm as well.
 * Unwinders that read the call frame information (gdb, faulthandler) show
 * every frame.  The driver and every function outside the two units keep
 * their frames. */

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

/* The layout rule: the functions of one evaluator form one section, in the
 * order of their first declaration (no_reorder), which Python/ceval_evaluator.h
 * gives by IR_TARGET_LAYOUT: the arms of the families declared `hot` in the
 * .rop sources first, then the others (Tools/cases_generator/rop_generator.py).
 * The normal evaluator's functions are contiguous and its hot arms share few
 * pages, and an arm added after them moves none of them; the observing
 * evaluator's functions have a section of their own.  (A function that names
 * its section is not split into hot and cold parts by the compiler.) */
#if IR_OBSERVING
#  define IR_TAIL_SECTION ".text.ir_observing"
#else
#  define IR_TAIL_SECTION ".text.ir_normal"
#endif
#define IR_TAIL_HEAD(FN) \
    static Py_NO_INLINE __attribute__((section(IR_TAIL_SECTION), no_reorder)) \
    Py_IR_TAIL_CC PyValue FN(Py_IR_TAIL_PARAMS)
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

/* The driver's entries into the normal evaluator: a frame from its start,
 * a frame thrown into (generator.throw()) */
extern Py_IR_TAIL_CC PyValue _PyEval_StartFrame(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_ThrowFrame(Py_IR_TAIL_PARAMS);

#ifdef _PY_JIT_SUPPORTED
/* The baseline JIT's exits from compiled code into the normal evaluator
 * (Python/ceval_evaluator.h; Python/jit.c binds the stencils' holes to
 * them) */
extern Py_IR_TAIL_CC PyValue _PyEval_JitDispatch(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_JitInterpret(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_JitMiss(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_JitProcessError(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_JitCloseDispatch(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_JitGenReturn(Py_IR_TAIL_PARAMS);
extern Py_IR_TAIL_CC PyValue _PyEval_JitBadEntry(Py_IR_TAIL_PARAMS);
#endif
#if IR_OBSERVING
#  define IR_THIS_EVALUATOR _PyEval_EnterObserving
#  define IR_OTHER_EVALUATOR _PyEval_EnterNormal
#else
#  define IR_THIS_EVALUATOR _PyEval_EnterNormal
#  define IR_OTHER_EVALUATOR _PyEval_EnterObserving
#endif

/* The evaluator's switch where an error routine resumes frame at
 * next_instr: to the other evaluator if the frame goes on there */
#define IR_ERROR_SWITCH() \
    do { \
        if (_PyEval_SwitchesLoop(frame, IR_OBSERVING)) { \
            Py_MUSTTAIL return IR_OTHER_EVALUATOR(tstate, frame, next_instr, ir_word); \
        } \
    } while (0)
