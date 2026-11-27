// Macros and other things needed by ceval.c and the generated arms

/* The compilation of the evaluator (Python/ceval_evaluator.h): 0 for the
 * normal one in Python/ceval_normal.c, 1 for the observing evaluator in
 * Python/ceval_observing.c.  The function forms (Python/ceval_forms.c) are
 * compiled once, with 0, and their executor picks a word's form by the
 * observed mark of its code. */
#ifndef IR_OBSERVING
#  define IR_OBSERVING 0
#endif
/* IR_COMPLETIONS(SITE): the completions of the consumer's word at SITE,
 * which the observing evaluator takes from its family's observer where a
 * tool observes the word */
#if IR_OBSERVING
#  define IR_COMPLETIONS(SITE) \
    ((_PyFrame_Observed(frame, (SITE)) & _PyIR_OBSERVE_EVENTS) \
        ? _PyRop_ObservingCompletions : _PyRop_Completions)
#else
#  define IR_COMPLETIONS(SITE) _PyRop_Completions
#endif

/* Use macros rather than inline functions, to make it as clear as possible
 * to the C compiler that the tracing check is a simple test then branch.
 * We want to be sure that the compiler knows this before it generates
 * the CFG.
 */

#ifdef WITH_DTRACE
#define OR_DTRACE_LINE | (PyDTrace_LINE_ENABLED() ? 255 : 0)
#else
#define OR_DTRACE_LINE
#endif

#ifdef Py_STATS
/* Count the entered handler, including IR-only and specialized targets.  The
 * previous handler, for pair counts, is the activation's. */
#define INSTRUCTION_STATS(op) \
    do { \
        int entry_opcode = (op); \
        int *last_handler = &IR_ACTIVATION()->last_handler; \
        assert(entry_opcode >= 0 && entry_opcode < IR_NUM_HANDLERS); \
        assert(_PyIR_HandlerFlags[entry_opcode] & _PyIR_H_DISPATCH); \
        assert(*last_handler >= 0 && *last_handler < IR_NUM_HANDLERS); \
        assert(*last_handler == 0 || \
               (_PyIR_HandlerFlags[*last_handler] & _PyIR_H_DISPATCH)); \
        PyStats *s = _PyStats_GET(); \
        if (s != NULL) { \
            s->opcode_stats[entry_opcode].execution_count++; \
            s->opcode_stats[*last_handler].pair_count[entry_opcode]++; \
        } \
        *last_handler = entry_opcode; \
    } while (0)
#else
#define INSTRUCTION_STATS(op) ((void)0)
#endif

/* The interpreter is a set of functions, one per handler, label and VM
 * routine, of one type (Python/ceval_tail.h), and dispatch is a guaranteed
 * tail call through the table of the evaluator's compilation
 * (Python/ceval_evaluator.h).  The interpreter's state is the functions'
 * parameters, which the calling convention keeps in registers, so a handler
 * that makes a call on a cold path costs nothing on its fast path.  The
 * state is:
 *
 *   frame       the executing frame, tstate->current_frame; a handler that
 *               switches frames assigns it before it dispatches;
 *   tstate      the thread state;
 *   next_instr  the word to run next;
 *   ir_word     the word of the instruction, which dispatch loads and from
 *               which the handler decodes its fields, IR_A(ir_word) and so
 *               on (Include/internal/pycore_ir.h).
 *
 * None has its address taken: that would put it in memory in every handler
 * and add a stack-protector canary.  A handler never reloads one after a
 * call that cannot change it.  Helpers take values and return a value, a
 * status or a {frame, next} pair; none jumps into the interpreter.
 *
 * A handler that runs another on the same word, as a specialisation that
 * deoptimises to its family head does, tail-calls it with ir_word
 * unchanged, so the other handler finds the same fields.
 *
 * Every handler also has a function form, ir_run_X(tstate, frame,
 * next_instr, ir_word), which returns a _PyIRTransfer (tiers87, C2).  The
 * bodies leave only through the transfer macros below, which this file binds
 * for the evaluator and Python/ceval_forms.c binds for the function forms;
 * the evaluator's shared paths are the VM routines of
 * Python/ceval_routines.h, bound the same way. */

#define TARGET(op) IR_TAIL_FN(op)

/* The evaluator's binding of the VM routines (Python/ceval_routines.h): a
 * routine is a function of the interpreter, and its operands
 * (IR_ROUTINE_OPERAND), and the error routine's origin and entry, travel in
 * the activation, as for the function forms. */
#define IR_ROUTINE(NAME) IR_TAIL_FN(NAME)
#define IR_ROUTINE_OPERAND(NAME) (IR_ACTIVATION()->NAME)
#define GEN_DISPATCH() \
    do { \
        int opcode; \
        DISPATCH(); \
    } while (0)
#define GEN_STEP_JUMP(NAME, F, IP, V) \
    IR_STEP_DONE(_PyEval_ir_##NAME(tstate, (F), (IP), _PyValue_ToTransferWord(V)))
/* The error that a completion arm reported in the activation: unwind it */
#define GEN_RAISED() JUMP_TO_LABEL(process_error)

/* The generator transfers.  The plain send, yield and return of a delegation
 * chain run in the VM routine gen_return (Python/ceval_routines.h): a
 * handler passes the routine its operands, in the activation.  A transfer
 * that is not plain, because a callback, a hook or a pending redirect must
 * observe it, runs the steps of
 * Python/ceval_generator.h (rule 13): functions that take the value that
 * they transfer as the word of their call, and return the frame and next
 * word to run.  A handler calls one with IR_GEN_JUMP() by name and runs what
 * it returns. */
#define IR_STEP_DONE(STEP) \
    do { \
        _PyIRTransfer ir_t = (STEP); \
        frame = ir_t.frame; \
        next_instr = ir_t.next; \
        if (next_instr == NULL) { \
            JUMP_TO_LABEL(process_error); \
        } \
        int opcode; \
        DISPATCH(); \
    } while (0)
#define IR_GEN_JUMP(NAME, F, IP, V) \
    IR_STEP_DONE(_PyEval_ir_##NAME(tstate, (F), (IP), _PyValue_ToTransferWord(V)))

/* The generator frame DYING has returned V and been unlinked and debited;
 * frame is its resumer.  EMPTY: the return is the certified
 * RETURN_VALUE_EMPTY, so that only the locals of DYING own values. */
#define IR_GEN_RETURN(V, DYING, EMPTY) \
    do { \
        _PyEvalActivation *ir_gen = IR_ACTIVATION(); \
        ir_gen->gen_value = (V); \
        ir_gen->gen_dying = (DYING); \
        ir_gen->gen_empty = (EMPTY); \
        JUMP_TO_LABEL(gen_return); \
    } while (0)

/* The frame transfers of the register instructions (Python/register_ops),
 * over the bindings of DISPATCH_INLINED, DISPATCH and IR_GEN_RETURN that the
 * evaluator and the function forms each make.
 *
 * ROP_ENTER: `exit enter(NEW) resume P`.  The caller goes on at next_instr
 * with the child's value in SLOT, P's definition, when NEW returns.
 * ROP_RETURN: `exit return(CALLER, HEAD)`.  The value is in the caller's
 * slot already; the caller goes on at HEAD, in its unit if it ran in one.
 * ROP_GEN_RETURN: `tail generator_return(CALLER, V, DYING)`;
 * ROP_GEN_RETURN_EMPTY: generator_return_empty's, for a frame whose scratch
 * is vacant, whose retirement clears only its fixed slots. */
#define ROP_ENTER(NEW, SLOT) \
    do { \
        frame->return_slot = (uint16_t)(SLOT); \
        frame->return_pc = next_instr; \
        DISPATCH_INLINED(NEW); \
    } while (0)
#define ROP_RETURN(CALLER, HEAD) \
    do { \
        frame = (CALLER); \
        next_instr = (HEAD); \
        IR_JIT_RESUME(); \
        LLTRACE_RESUME_FRAME(); \
        DISPATCH(); \
    } while (0)
#define ROP_GEN_RETURN(CALLER, V, DYING) \
    do { \
        frame = (CALLER); \
        IR_GEN_RETURN((V), (DYING), 0); \
    } while (0)
#define ROP_GEN_RETURN_EMPTY(CALLER, V, DYING) \
    do { \
        frame = (CALLER); \
        IR_GEN_RETURN((V), (DYING), 1); \
    } while (0)

/* ROP_YIELD: `exit yield(V) resume P` of a Yield word, ROP_YIELD_FROM of a
 * YieldFrom word (a delegation, which suspends in the state that a throw into
 * it must forward).  The generator suspends and V goes to its consumer's
 * return slot, as the consumer's frame, which the generator's return_slot
 * and its next word record for the resumption: SLOT is the home that the
 * value sent in takes when it resumes, P's definition.  The suspension is a
 * delegation chain's (pyjit G4): every plain consumer that would run its
 * yield-from loop's chain(resume) word suspends too, and V goes to the last
 * consumer (_PyRop_SuspendChain, Python/ceval_shared.h). */
#define ROP_SUSPEND(V, SLOT, STATE) \
    do { \
        PyValue rop_yielded = (V); \
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER); \
        frame->instr_ptr = this_instr + 1; \
        PyGenObject *rop_gen = _PyGen_GetGeneratorFromFrame(frame); \
        DTRACE_FUNCTION_RETURN(); \
        tstate->exc_info = rop_gen->gi_exc_state.previous_item; \
        rop_gen->gi_exc_state.previous_item = NULL; \
        _Py_LeaveRecursiveCallPy(tstate); \
        _PyInterpreterFrame *rop_suspended = frame; \
        rop_suspended->return_slot = (uint16_t)(SLOT); \
        _PyThreadState_UpdateLastProfiledFrame(tstate, rop_suspended, \
                                               rop_suspended->previous); \
        frame = tstate->current_frame = rop_suspended->previous; \
        rop_suspended->previous = NULL; \
        ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_YIELD; \
        FT_ATOMIC_STORE_INT8_RELEASE(rop_gen->gi_frame_state, (STATE)); \
        frame = _PyRop_SuspendChain(tstate, frame); \
        next_instr = frame->return_pc; \
        LLTRACE_RESUME_FRAME(); \
        IR_STORE_RETURN(rop_yielded); \
        DISPATCH(); \
    } while (0)
#define ROP_YIELD(V, SLOT) ROP_SUSPEND((V), (SLOT), FRAME_SUSPENDED)
#define ROP_YIELD_FROM(V, SLOT) ROP_SUSPEND((V), (SLOT), FRAME_SUSPENDED_YIELD_FROM)

/* A call that the function forms make in place of a jump, which must not
 * grow the C stack: a chain of deoptimisations or frame entries stays one
 * C frame deep. */
#define Py_MUSTTAIL __attribute__((musttail))

/* A planned arm's uop that may reenter Python: the whole-word completion
 * stub's ESCAPED bit (Python/generated_rop_stubs.c.h); nothing elsewhere */
#define IR_STUB_ESCAPE() ((void)0)
/* The word's frame had its control moved while it ran (a debugger's jump:
 * pending_redirect and the continuation in return_pc): commit the jump and
 * go on there.  The word's results were discarded before. */
#define IR_REDIRECT() \
    do { \
        if (frame->pending_redirect) { \
            _PyFrame_CompleteRedirect(frame); \
        } \
        next_instr = frame->return_pc; \
        DISPATCH(); \
    } while (0)
/* A register instruction's branch or jump: go on at the word its target
 * operand names (a stub reports BRANCH instead, Tools/cases_generator/rop_generator.py) */
#define IR_BRANCH(TARGET) \
    do { \
        next_instr = (TARGET); \
        DISPATCH(); \
    } while (0)
#define DISPATCH_GOTO() \
    do { \
        Py_MUSTTAIL return ir_tail_table[opcode](tstate, frame, next_instr, ir_word); \
    } while (0)
/* The closes before an arm's dispatching exit (Tools/cases_generator/rop_cgen.py,
 * lower_tail_closes).  Each drops its count inline
 * (_PyValue_DropNoDealloc()); a value left with no count is not
 * deallocated, and the arm goes to its cold block COLD, which parks it, and
 * each later close's value left with no count, in the activation and
 * leaves through the VM routine close_dispatch (Python/ceval_evaluator.h),
 * which gives them their last counts back and closes them in order, then
 * dispatches.  No code runs between the decrement and the restore, and no
 * arm calls on its way to its next word. */
#define ROP_TAIL_CLOSE(V, COLD) \
    do { \
        if (Py_UNLIKELY(_PyValue_DropNoDealloc(V))) { \
            goto COLD; \
        } \
    } while (0)
#define ROP_TAIL_PARK_FIRST(V) \
    do { \
        _PyEvalActivation *ir_tail = IR_ACTIVATION(); \
        ir_tail->tail_close[0] = (V); \
        ir_tail->tail_closes = 1; \
    } while (0)
#define ROP_TAIL_PARK(V) \
    do { \
        PyValue ir_parked = (V); \
        if (_PyValue_DropNoDealloc(ir_parked)) { \
            _PyEvalActivation *ir_tail = IR_ACTIVATION(); \
            ir_tail->tail_close[ir_tail->tail_closes++] = ir_parked; \
        } \
    } while (0)
#define ROP_TAIL_DISPATCH() JUMP_TO_LABEL(close_dispatch)
#define ROP_TAIL_BRANCH(T) \
    do { \
        next_instr = (T); \
        JUMP_TO_LABEL(close_dispatch); \
    } while (0)
/* An error leaves the handler for the routine process_error, which unwinds
 * by the exception table and either dispatches at the handler found or ends
 * the activation. */
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        _PyEvalActivation *ir_exit = IR_ACTIVATION(); \
        ir_exit->exit_next = (origin); \
        ir_exit->exit_entry = \
            _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        JUMP_TO_LABEL(process_error); \
    } while (0)
/* The activation ends with the value V (INTERPRETER_EXIT, or an error that no
 * handler catches) */
#define IR_EXIT(V) return (V)
#define JUMP_TO_LABEL(name) IR_TAIL_GOTO(name)
/* A specialisation deoptimises to its family head, which runs the word
 * again from its start. */
#define JUMP_TO_PREDICTED(name) \
    do { \
        Py_MUSTTAIL return IR_TAIL_ID(name)(tstate, frame, this_instr, ir_word); \
    } while (0)

/* The activation of the evaluation loop that runs the handler
 * (_PyEvalActivation, Include/internal/pycore_ceval.h) */
#define IR_ACTIVATION() (((_PyThreadStateImpl *)tstate)->eval_activation)

/* Whether frame runs the executing copy of its code, as it must at every
 * dispatch (pyjit/design/register-ir.md 1.10).  An entry frame runs the
 * trampoline's words and has no code. */
#define IR_EXECUTING_COPY_OK(frame) \
    ((frame)->owner == FRAME_OWNED_BY_INTERPRETER || \
     (_PyFrame_GetCode(frame)->_co_exec != NULL && \
      (frame)->ir_words == _PyFrame_GetCode(frame)->_co_exec->words))

/* PRE_DISPATCH_GOTO() checks the handlers' state and does lltrace if
 * enabled, in a debug build.  Normally a no-op.  The first check fails if a
 * handler switches frames without assigning the new one to frame. */
#ifdef Py_DEBUG
#define PRE_DISPATCH_GOTO() \
    assert(frame == tstate->current_frame); \
    assert(IR_EXECUTING_COPY_OK(frame)); \
    if (frame->lltrace >= 5) { \
        lltrace_instruction(frame, next_instr, opcode); }
#else
#define PRE_DISPATCH_GOTO() ((void)0)
#endif

#ifdef Py_DEBUG
#define LLTRACE_RESUME_FRAME() \
do { \
    int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS()); \
    frame->lltrace = lltrace; \
} while (0)
#else
#define LLTRACE_RESUME_FRAME() ((void)0)
#endif

#ifdef Py_GIL_DISABLED
#define QSBR_QUIESCENT_STATE(tstate) _Py_qsbr_quiescent_state(((_PyThreadStateImpl *)tstate)->qsbr)
#else
#define QSBR_QUIESCENT_STATE(tstate)
#endif


/* Dispatch the word at next_instr.  The observing evaluator first tests
 * what monitoring observes at the word (observe_word,
 * Python/ceval_evaluator.h); the normal one never reads it.  The switch
 * word is no word of the code, and nothing observes it. */
#if IR_OBSERVING
#define DISPATCH() \
    { \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        if (opcode != IR_H_SWITCH && _PyFrame_Observed(frame, next_instr)) { \
            JUMP_TO_LABEL(observe_word); \
        } \
        DISPATCH_GOTO(); \
    }
#else
#define DISPATCH() \
    { \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }
#endif

/* A monitoring callback redirected the frame (a debugger's f_lineno jump):
 * the word leaves for frame->instr_ptr, the redirect exit */
#define DISPATCH_REDIRECT() \
    do { \
        next_instr = frame->instr_ptr; \
        DISPATCH(); \
    } while (0)

#define DISPATCH_SAME_OPARG() \
    { \
        opcode = IR_HANDLER(*next_instr); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

/* The handler of an outlined instruction (Python/register_ops, rule 4): its body is the function FN, which returns the frame and next word
 * to run, or no word for an exit that the activation describes.  The handler
 * assigns the state and dispatches, as a helper of rule 13 requires. */
#define IR_RUN_OUTLINED(FN) IR_STEP_DONE(FN(tstate, frame, next_instr, ir_word))

/* A conditional jump of an outlined handler reports whether it jumped, which
 * its instrumented twin needs for its event and cannot tell from the word it
 * is to run when the jump is of no distance (rule 12).  The evaluator's own
 * handlers do not report. */
#define IR_REPORT_EDGE(TAKEN) ((void)(TAKEN))

/* The event of an instrumented instruction that follows its effect (rule 12)
 * runs in a cold function, _PyEval_ir_event_edge(), which returns the word to run or
 * no word for an error that the activation describes. */
#define IR_OBSERVE_EDGE(SOURCE, NEXT, EVENT, EDGE) \
    IR_STEP_DONE(_PyEval_ir_event_edge(tstate, frame, this_instr, \
                                       (SOURCE), (NEXT), (EVENT), (EDGE)))

/* Whether NEW, the frame that an arm enters, is linked below frame: its
 * child, the child of an __init__ call's cleanup frame, or the deepest
 * frame of a delegation chain (_PyRop_EnterChain) */
static inline bool
_PyRop_EntersBelow(_PyInterpreterFrame *new_frame, _PyInterpreterFrame *frame)
{
    for (_PyInterpreterFrame *f = new_frame->previous; f != NULL; f = f->previous) {
        if (f == frame) {
            return true;
        }
    }
    return false;
}

#define DISPATCH_INLINED(NEW_FRAME)                              \
    do {                                                         \
        assert(!IS_PEP523_HOOKED(tstate));                       \
        assert(_PyRop_EntersBelow((NEW_FRAME), frame));          \
        frame = tstate->current_frame = (NEW_FRAME);             \
        CALL_STAT_INC(inlined_py_calls);                         \
        IR_START_FRAME();                                        \
    } while (0)

/* Tuple access macros */

static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i) {
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    return _PyTuple_GET_OBJECT(v, i);
}

/* Code access macros */

#define NEXTOPARG()  do { \
        ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*next_instr); \
        opcode = IR_HANDLER(ir_word); \
    } while (0)

/* JUMPBY makes the generator identify the instruction as a jump. SKIP_OVER is
 * for advancing to the next instruction, taking into account cache entries
 * and skipped instructions.
 */
#define JUMPBY(x)       (next_instr += (x))
/* The word at absolute index T, the target of a jump, and a jump there. */
#define JUMP_TARGET(T)  (IR_CHECK_BLOCK(), frame->ir_words + (T))
#define JUMPTO(T)       (next_instr = JUMP_TARGET(T))
#define SKIP_OVER(x)    (next_instr += (x))



/* Data access macros */
#define FRAME_CO_CONSTS (_PyFrame_GetCode(frame)->co_consts)
#define FRAME_CO_NAMES  (_PyFrame_GetCode(frame)->co_names)

/* Local variable macros */

/* The frame's slots, which the IR's fields index.  Handlers read them from
 * the frame where used: a register for them would be one more value live
 * across every dispatch. */
#define IR_SLOTS()      (frame->base)
#define GETLOCAL(i)     (IR_SLOTS()[i])



// Try to lock an object in the free threading build, if it's not already
// locked. Use with a DEOPT_IF() to deopt if the object is already locked.
// These are no-ops in the default GIL build. The general pattern is:
//
// DEOPT_IF(!LOCK_OBJECT(op));
// if (/* condition fails */) {
//     UNLOCK_OBJECT(op);
//     DEOPT_IF(true);
//  }
//  ...
//  UNLOCK_OBJECT(op);
//
// NOTE: The object must be unlocked on every exit code path and you should
// avoid any potentially escaping calls (like PyValue_CLOSE) while the
// object is locked.
#ifdef Py_GIL_DISABLED
#  define LOCK_OBJECT(op) PyMutex_LockFast(&(_PyObject_CAST(op))->ob_mutex)
#  define UNLOCK_OBJECT(op) PyMutex_Unlock(&(_PyObject_CAST(op))->ob_mutex)
#else
#  define LOCK_OBJECT(op) (1)
#  define UNLOCK_OBJECT(op) ((void)0)
#endif

#define GLOBALS() frame->f_globals
#define BUILTINS() frame->f_builtins
#define LOCALS() frame->f_locals

#if defined(WITH_DTRACE) && !defined(Py_BUILD_CORE_MODULE)
static void dtrace_function_entry(_PyInterpreterFrame *);
void _PyEval_dtrace_function_return(_PyInterpreterFrame *);

#define DTRACE_FUNCTION_ENTRY()  \
    if (PyDTrace_FUNCTION_ENTRY_ENABLED()) { \
        dtrace_function_entry(frame); \
    }

#define DTRACE_FUNCTION_RETURN() \
    if (PyDTrace_FUNCTION_RETURN_ENABLED()) { \
        _PyEval_dtrace_function_return(frame); \
    }
#else
#define DTRACE_FUNCTION_ENTRY() ((void)0)
#define DTRACE_FUNCTION_RETURN() ((void)0)
#endif

/* Counters live in feedback entries (pycore_feedback.h).  fb is the entry
 * header of the instruction, or NULL if it has none, which never triggers.
 * Entry 0, the header that the sites without an entry share, never triggers
 * either: its counter is unreachable (make_exec() in ir.c), which
 * backoff_counter_triggers() never reports, so nothing after a trigger
 * writes past a header there (asserted where a handler gets its entry,
 * IR_FB()).  Every specializing op tests and advances its
 * counter with these two functions. */
static inline int
_PyFb_CounterTriggers(_PyFbHeader *fb)
{
    return fb != NULL && backoff_counter_triggers(fb->counter);
}

static inline void
_PyFb_AdvanceCounter(_PyFbHeader *fb)
{
    if (fb == NULL) {
        return;
    }
#ifdef Py_GIL_DISABLED
    /* Counters are unreachable when thread-local bytecode is disabled,
     * so there is no need to update them. */
    if (backoff_counter_is_unreachable(fb->counter)) {
        return;
    }
#endif
    fb->counter = advance_backoff_counter(fb->counter);
}

/* The `tick` row (Python/register_ops/core.rop): quiet when the counter has
 * not triggered, which advances it. */
static inline bool
_PyFb_TickQuiet(_PyFbHeader *fb)
{
    if (_PyFb_CounterTriggers(fb)) {
        return false;
    }
    _PyFb_AdvanceCounter(fb);
    return true;
}

#define UNBOUNDLOCAL_ERROR_MSG \
    "cannot access local variable '%s' where it is not associated with a value"
#define UNBOUNDFREE_ERROR_MSG \
    "cannot access free variable '%s' where it is not associated with a value" \
    " in enclosing scope"
#define NAME_ERROR_MSG "name '%.200s' is not defined"

static inline int _Py_EnterRecursivePy(PyThreadState *tstate) {
    return (tstate->py_recursion_remaining-- <= 0) &&
        _Py_CheckRecursiveCallPy(tstate);
}

static inline void _Py_LeaveRecursiveCallPy(PyThreadState *tstate)  {
    tstate->py_recursion_remaining++;
}

/* Implementation of "macros" that modify the instruction pointer,
 * stack pointer, or frame pointer. */

#define LOAD_IP(OFFSET) do { \
        next_instr = frame->instr_ptr + (OFFSET); \
    } while (0)

/* There's no STORE_IP(), it's inlined by the code generator. */


/* Value/object bridge macros */

static inline bool
IS_PEP523_HOOKED(PyThreadState *tstate)
{
    return tstate->interp->eval_frame != NULL;
}

/* Store the attribute that SET_FUNCTION_ATTRIBUTE's oparg selects into its
 * slot of the function, which takes the attribute's reference. */
static inline void
_PyFunction_StoreAttribute(PyFunctionObject *func, int oparg, PyObject *attr)
{
    size_t offset = _Py_FunctionAttributeOffsets[oparg];
    assert(offset != 0);
    PyObject **ptr = (PyObject **)(((char *)func) + offset);
    assert(*ptr == NULL);
    *ptr = attr;
}

/* Whether the thread has an event for _Py_HandlePending(): one load and test
 * of its eval breaker, the periodic checks' fast path. */
static inline bool
_Py_EvalBreakerHasEvents(PyThreadState *tstate)
{
    return (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) != 0;
}

/* The thread's eval breaker: its event bits and the global instrumentation
 * version, by a relaxed load as _Py_EvalBreakerHasEvents() reads it.  The
 * entry check of a frame (_RESUME_CHECK) compares it with the frame's
 * code's version: equal when no event is pending and the code is
 * instrumented as the interpreter is. */
static inline uintptr_t
_Py_EvalBreaker(PyThreadState *tstate)
{
    return _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker);
}

/* The instrumentation version of the code that frame runs, by an acquire
 * load.  It has no event bits, so the eval breaker equals it only when no
 * event is pending. */
static inline uintptr_t
_PyFrame_CodeInstrumentationVersion(_PyInterpreterFrame *frame)
{
    uintptr_t version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(
        _PyFrame_GetCode(frame)->_co_instrumentation_version);
    assert((version & _PY_EVAL_EVENTS_MASK) == 0);
    return version;
}
#define FRAME_CODE_INSTRUMENTATION_VERSION() _PyFrame_CodeInstrumentationVersion(frame)

static inline int
check_periodics(PyThreadState *tstate) {
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (_Py_EvalBreakerHasEvents(tstate)) {
        return _Py_HandlePending(tstate);
    }
    return 0;
}

/* The handling of a pending event after an instruction: the flag test stays
 * in the handler and the rest runs here, out of line. */
static Py_NO_INLINE _Py_COLD_FUNCTION int
handle_periodics_at_end(PyThreadState *tstate, _PyInterpreterFrame *frame) {
    return _Py_HandlePending(tstate);
}

static inline int
check_periodics_at_end(PyThreadState *tstate, _PyInterpreterFrame *frame) {
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (Py_UNLIKELY(_Py_EvalBreakerHasEvents(tstate))) {
        return handle_periodics_at_end(tstate, frame);
    }
    return 0;
}

// Mark the generator as executing. Returns true if the state was changed,
// false if it was already executing or finished.
static inline bool
gen_try_set_executing(PyGenObject *gen)
{
#ifdef Py_GIL_DISABLED
    if (!_PyObject_IsUniquelyReferenced((PyObject *)gen)) {
        int8_t frame_state = _Py_atomic_load_int8_relaxed(&gen->gi_frame_state);
        while (frame_state < FRAME_SUSPENDED_YIELD_FROM_LOCKED) {
            if (_Py_atomic_compare_exchange_int8(&gen->gi_frame_state,
                                                 &frame_state,
                                                 FRAME_EXECUTING)) {
                return true;
            }
        }
        // NB: We return false for FRAME_SUSPENDED_YIELD_FROM_LOCKED as well.
        // That case is rare enough that we can just handle it in the deopt.
        return false;
    }
#endif
    // Use faster non-atomic modifications in the GIL-enabled build and when
    // the object is uniquely referenced in the free-threaded build.
    if (gen->gi_frame_state < FRAME_EXECUTING) {
        assert(gen->gi_frame_state != FRAME_SUSPENDED_YIELD_FROM_LOCKED);
        gen->gi_frame_state = FRAME_EXECUTING;
        return true;
    }
    return false;
}

/* Whether a specialized send or loop step can claim gen for the frame that
 * it pushes: gen is created or suspended, and no other thread holds it.  A
 * read, for the handler's guard; gen_claim() then claims it. */
static inline bool
gen_may_claim(PyGenObject *gen)
{
    int8_t frame_state = FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state);
#ifdef Py_GIL_DISABLED
    return frame_state < FRAME_SUSPENDED_YIELD_FROM_LOCKED;
#else
    assert(frame_state != FRAME_SUSPENDED_YIELD_FROM_LOCKED);
    return frame_state < FRAME_EXECUTING;
#endif
}

/* Claim gen, which gen_may_claim() accepted.  With the GIL the claim is a
 * store.  Free-threaded, another thread can claim gen between the guard and
 * the claim; gen is then executing, and the claim raises what the generic
 * send raises for an executing generator, and returns 0. */
static inline int
gen_claim(PyThreadState *tstate, PyGenObject *gen)
{
#ifdef Py_GIL_DISABLED
    if (!gen_try_set_executing(gen)) {
        _PyErr_SetString(tstate, PyExc_ValueError,
                         PyCoro_CheckExact(gen) ? "coroutine already executing"
                                                : "generator already executing");
        return 0;
    }
#else
    (void)tstate;
    gen->gi_frame_state = FRAME_EXECUTING;
#endif
    return 1;
}

/* Operands of the register IR handlers (Python/register_ops).
 * F is the value of a field, IR_A(ir_word) and so on.  The result and input
 * macros take the word W and the letter X of the field instead, so that they
 * test the word's slot-class flag. */

/* The slot that a slot field names, an lvalue. */
#define IR_SLOT(F) (IR_SLOTS()[(F)])

/* Whether field F names a local, a cell, a free variable or the frame's
 * discard slot (a hidden local) rather than a temporary: the executing copy
 * keeps the count of such slots in its header.  An input read through
 * a local is borrowed in place and never released; one read through a
 * temporary dies with the instruction, and the handler detaches it
 * (IR_DETACH) after its effect.  A result written through a local or the
 * discard slot releases the slot's old value (IR_STORE_DST). */
#define IR_NFIXED() \
    ((int)(((const _PyCodeExec *)frame->ir_words) - 1)->nfixed)
#define IR_IS_LOCAL(F) ((int)(F) < IR_NFIXED())

/* The value of field X of word W */
#define IR_FIELD(W, X) IR_##X(W)

/* IR_IS_LOCAL(IR_FIELD(W, X)) from the slot-class flag of W, an executing
 * word, which saves the loads of the header (_PyIR_ExecLocalFlags()). */
#define IR_IS_LOCAL_FIELD(W, X) \
    (assert(!((W) & IR_F_LOCAL_##X) == !IR_IS_LOCAL(IR_FIELD(W, X))), \
     ((W) & IR_F_LOCAL_##X) != 0)

/* The view of an input that the handler reads through field F, which may
 * name a local (the instruction's borrow_mask): the slot's bits, as a
 * PyValueBorrow, which only C locals hold and nothing releases.  The one
 * load of the slot (pyjit/design/proposals/register-ir-v2-a5b.md, 4.2).
 * Reads borrow: the input stays in its slot, where the collector and the
 * unwinder see it, until the handler detaches it. */
#define IR_VIEW(F) PyValue_Borrow(IR_SLOT(F))

/* Detach, after the effect: the input V that the handler read through field
 * X of W dies with the instruction if it is a temporary.  Its owner moves to
 * a C local and its slot becomes NULL.  A local keeps its value, so it
 * yields PyValue_NULL, which owns nothing.  The handler closes the owner
 * after it stored its results. */
#define IR_DETACH(W, X, V) \
    (IR_IS_LOCAL_FIELD(W, X) ? PyValue_NULL \
        : _PyFrame_TakeKnownHome(frame, &IR_SLOT(IR_FIELD(W, X)), PyValue_Borrow(V)))

/* The DSL's TAKE(V) of view V whose field's owner is OWN, detached before:
 * a value with a count of its own, for a home that the handler stores
 * elsewhere.  A dying input's count moves, and OWN is empty after, so the
 * handler's close of it does nothing; a local's value, whose OWN is NULL,
 * gets a new count. */
static inline PyValue
_PyIR_Taken(PyValueBorrow view, PyValue *owner)
{
    PyValue taken = *owner;
    if (PyValue_IsNull(taken)) {
        return PyValue_XDUP(view);
    }
    *owner = PyValue_NULL;
    return taken;
}
#define IR_TAKEN(V, OWN) _PyIR_Taken(PyValue_Borrow(V), &(OWN))

/* A handler writes a temporary of its window, or one that it wrote
 * before. */
#define IR_SET_TEMP(SLOT, V) \
    do { \
        (SLOT) = (V); \
        _PyValue_CheckTemp(SLOT); \
    } while (0)

/* The releasing write of result V to dst field X of W, a temporary, a local
 * or the discard slot, which one test of the old value serves: the old value
 * moves to the handler's ir_old, and the handler closes it after its last
 * store, as STORE_FAST does.  A temporary's old value is NULL. */
#define IR_STORE_DST(W, X, V) \
    do { \
        ir_old = IR_SLOT(IR_FIELD(W, X)); \
        assert(IR_IS_LOCAL_FIELD(W, X) || PyValue_IsNull(ir_old)); \
        IR_SLOT(IR_FIELD(W, X)) = (V); \
        _PyValue_CheckHome(IR_SLOT(IR_FIELD(W, X))); \
    } while (0)

/* A window-style handler whose dst field X of W may name a local or the
 * discard slot moves its result from SLOT of its window there, with the
 * releasing write of IR_STORE_DST.  A temporary dst is SLOT itself. */
#define IR_MOVE_TO_DST(W, X, SLOT) \
    do { \
        if (IR_IS_LOCAL_FIELD(W, X)) { \
            ir_old = IR_SLOT(IR_FIELD(W, X)); \
            assert(_PyValue_IsHome(SLOT)); \
            IR_SLOT(IR_FIELD(W, X)) = (SLOT); \
            _PyValue_CheckHome(IR_SLOT(IR_FIELD(W, X))); \
            (SLOT) = PyValue_NULL; \
        } \
    } while (0)

/* Close the N owners in WINDOW, which are the old values of the locals that
 * received the elements, detaching each before it closes it.  The first
 * local's old value goes first, as the STORE_FASTs that the unpack replaces
 * would close it. */
static inline void
_PyIR_CloseWindow(PyValue *window, int n)
{
    for (int i = n; i-- > 0;) {
        PyValue owner = window[i];
        window[i] = PyValue_NULL;
        PyValue_XCLOSE(owner);
    }
}
#define IR_CLOSE_WINDOW(W, N) _PyIR_CloseWindow((W), (N))

/* The frame's view of its executing copy: frame->ir_words follows the copy
 * that instr_ptr points into. */
#define IR_CHECK_BLOCK() \
    assert(frame->ir_words == _PyFrame_Exec(frame)->words)

/* This is the callback-free identity arm of _PyEval_GetAwaitable. The other
 * states and custom __await__ implementations keep its ordinary error path. */
static inline bool
coro_awaitable_can_transfer(PyValueBorrow value)
{
    if (!PyValue_IsObject(value)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(value);
    if (!PyCoro_CheckExact(object)) {
        return false;
    }
    int8_t state = FT_ATOMIC_LOAD_INT8_RELAXED(
        ((PyCoroObject *)object)->cr_frame_state);
    return state != FRAME_SUSPENDED_YIELD_FROM &&
           state != FRAME_SUSPENDED_YIELD_FROM_LOCKED;
}

/* The feedback entry at index F.  Entry 0 (field IR_FB_SHARED), the header
 * that the sites without an entry of their own share, never triggers: its
 * counter is unreachable from make_exec() on, and nothing makes it
 * reachable (advancing and pausing keep its backoff bits).  The counter
 * functions take no frame, so the invariant is checked here, where a
 * handler gets its entry. */
static inline void *
_PyIR_FbOfHandler(_PyFbHeader *entry, uint32_t field)
{
    assert(field != IR_FB_SHARED || !backoff_counter_triggers(entry->counter));
    (void)field;
    return entry;
}
#define IR_FB(F) \
    (IR_CHECK_BLOCK(), _PyIR_FbOfHandler(_PyIR_FbEntry(frame->ir_words, (F)), (F)))

/* The poly row of the site whose entry header is H that records the
 * receiver type version VERSION, or -1.  WORDS is the frame's ir_words; the
 * block of the poly tables precedes the executing copy's header
 * (_PyIR_FbBlock()).  VERSION is not zero, which an unused row holds.  Every
 * form that executes a poly table finds its row here and then checks its
 * own payload. */
static inline int
poly_row_index(const _PyIRWord *words, const _PyFbHeader *h, uint32_t version)
{
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(words), h);
    assert(version != 0);
    for (int i = 0; i < FB_POLY_SIZE; i++) {
        if (poly->type_version[i] == version) {
            return i;
        }
    }
    return -1;
}

/* Every executable shape row was nominated by the checked managed-shape lookup
 * (fb_shape_read_set()): its handler is GetAttr.Shape and its aux is a
 * shape.  No pointer from the bank escapes this callback-free selection. */
static inline int64_t
shape_read_offset(const _PyIRWord *words, _PyFbAttr *entry, PyObject *owner)
{
#ifdef Py_SHAPES
    const _PyFbHeader *h = &entry->h;
    uint32_t version = Py_TYPE(owner)->tp_version_tag;
    int i = version != 0 ? poly_row_index(words, h, version) : -1;
    if (i < 0) {
        return -1;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(words), h);
    assert(poly->payload[i].read_kind == _Py_FB_READ_SHAPE);
    if (_PyObject_Shape(owner) != poly->aux[i]) {
        return -1;
    }
    return poly->index[i];
#else
    (void)words;
    (void)entry;
    (void)owner;
    return -1;
#endif
}

/* On entry to a frame at next_instr, its instr_ptr: if the word there is a
 * RESUME that would do nothing, because the eval breaker has nothing pending
 * and the code's instrumentation is current, start at the word after it, as
 * if it had run.  An instrumented RESUME always runs. */
#if defined(__EMSCRIPTEN__)
#  define ROP_ENTER_GATE_EMSCRIPTEN() \
    (_Py_emscripten_signal_clock != 0 && \
     (_Py_emscripten_signal_clock -= Py_EMSCRIPTEN_SIGNAL_HANDLING, 1))
#else
#  define ROP_ENTER_GATE_EMSCRIPTEN() 1
#endif
#ifdef Py_GIL_DISABLED
#  define ROP_ENTER_GATE_TLBC() \
    (frame->tlbc_index == ((_PyThreadStateImpl *)tstate)->tlbc_index)
#else
#  define ROP_ENTER_GATE_TLBC() 1
#endif
/* Whether the start of frame skips the word at RESUME, its instr_ptr: a
 * Resume.Poll that would do nothing.  One test per fact, each a predicted
 * branch, not arithmetic on the loads: dispatch from the next word does not
 * wait on them.  The frame start (ROP_ENTER_GATE()) and a delegation chain's
 * entry (_PyRop_EnterChain, Python/ceval_shared.h), which runs a level's
 * words only where its start would skip them, both test it.  The observing
 * evaluator never skips: RESUME's observer reports PY_START, and the RESUME
 * of code that is not observed sends the frame back to the normal
 * evaluator. */
static inline bool
_PyRop_StartSkips(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  const _PyInstr *resume)
{
#if IR_OBSERVING
    (void)tstate;
    (void)frame;
    (void)resume;
    return false;
#else
    if (IR_HANDLER(*resume) != ROP_HANDLER_Resume_Poll) {
        return false;
    }
    if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) !=
        FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(
            _PyFrame_GetCode(frame)->_co_instrumentation_version)) {
        return false;
    }
    return ROP_ENTER_GATE_TLBC() && ROP_ENTER_GATE_EMSCRIPTEN();
#endif
}
#define ROP_ENTER_GATE() \
    do { \
        assert(next_instr == frame->instr_ptr); \
        if (_PyRop_StartSkips(tstate, frame, next_instr)) { \
            next_instr++; \
        } \
    } while (0)
/* The start of frame, which has just become the current frame: its
 * recursion debit, which raises RecursionError in the frame before its
 * first word runs, then its first word, past a RESUME that would do nothing
 * (ROP_ENTER_GATE()).  An arm that enters a frame runs it in place
 * (DISPATCH_INLINED); the start_frame label runs it for the C entry and the
 * function forms (Python/ceval_labels.h). */
#ifdef Py_DEBUG
#  define IR_START_FRAME_LLTRACE() \
    do { \
        int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS()); \
        if (lltrace < 0) { \
            PROCESS_ERROR_AT(EXIT, 0, frame->instr_ptr); \
        } \
        frame->lltrace = lltrace; \
        assert(!_PyErr_Occurred(tstate)); \
    } while (0)
#else
#  define IR_START_FRAME_LLTRACE() ((void)0)
#endif
#define IR_START_FRAME() \
    do { \
        if (_Py_EnterRecursivePy(tstate)) { \
            PROCESS_ERROR_AT(EXIT_NOTRACE, 0, frame->instr_ptr); \
        } \
        DTRACE_FUNCTION_ENTRY(); \
        next_instr = frame->instr_ptr; \
        ROP_ENTER_GATE(); \
        IR_START_FRAME_LLTRACE(); \
        int opcode; \
        DISPATCH(); \
    } while (0)

/* After a frame switch, V goes to the slot of the frame that control came
 * back to (frame->return_slot).  If the frame has run in its code's unit, it
 * goes back to it (IR_JIT_RESUME()). */
#define IR_STORE_RETURN(V) \
    do { \
        assert(_PyValue_IsHome(V)); \
        if (_PyFrame_SetReturnValue(frame, (V))) { \
            next_instr = frame->return_pc; \
            IR_JIT_RESUME(); \
        } \
    } while (0)

/* The baseline JIT's hand-offs from the evaluator (Python/jit.c), which
 * jit_run() and jit_resume() in Python/ceval.c drive.  IR_JIT_EVENT(), the
 * end of RESUME_JIT and JUMP_BACKWARD_JIT, is a warm-up event of the frame's
 * code; once the code has a unit, the frame goes on in it at next_instr.
 * IR_JIT_RESUME(): control has come back to frame at next_instr, and the
 * frame goes back to its unit if it ran in one.  The function forms, which
 * compiled code calls, bind both to nothing. */
#ifdef _PY_JIT_SUPPORTED
#define IR_JIT_EVENT() IR_JIT_EVENT_AT(next_instr)
#define IR_JIT_EVENT_AT(NEXT) \
    do { \
        _PyCodeExec *ir_exec = _PyFrame_Exec(frame); \
        /* Most events count a code that has no unit and is not yet warm, \
         * which needs no call */ \
        if (ir_exec->jit_unit == NULL && \
            ir_exec->jit_heat + 1 < tstate->interp->ceval.jit_warmup) { \
            ir_exec->jit_heat++; \
        } \
        else { \
            _PyJitUnit *ir_unit = _PyJit_Event(tstate, frame); \
            if (ir_unit != NULL) { \
                IR_STEP_DONE(jit_run(tstate, ir_unit, frame, (NEXT))); \
            } \
        } \
    } while (0)
#define IR_JIT_RESUME() \
    do { \
        if (frame->jit_flags & _PyFrame_JIT_RESUME) { \
            IR_STEP_DONE(jit_resume(tstate, frame, next_instr)); \
        } \
    } while (0)
#else
#define IR_JIT_EVENT() ((void)0)
#define IR_JIT_EVENT_AT(NEXT) ((void)(NEXT))
#define IR_JIT_RESUME() ((void)0)
#endif

/* A backward jump's warm-up event (Jump.Warm): go on at TARGET in the
 * code's unit if it has one, otherwise in the evaluator.  The forms and
 * stubs do nothing for the event. */
#define ROP_JIT_WARM(TARGET) \
    do { \
        _PyInstr *rop_target = (TARGET); \
        IR_JIT_EVENT_AT(rop_target); \
        IR_BRANCH(rop_target); \
    } while (0)

/* The execution pin (g3/design.md section 2) that a departing arm takes
 * after its last guard.  The evaluator runs in no unit: nothing to keep
 * alive.  The function forms and the stubs bind it to _PyJit_PinDeparting(). */
#define IR_PIN_DEPARTING() ((void)0)

/* The feedback entry of another instruction of the frame. */
#define _PyIR_FbHeaderOf(FRAME, INSTR, HANDLER) \
    ((void)(HANDLER), \
     _PyIR_InstrFeedback(_PyFrame_GetCode(FRAME), _PyFrame_Exec(FRAME), (INSTR)))
