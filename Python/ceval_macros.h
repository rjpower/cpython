// Macros and other things needed by ceval.c, and bytecodes.c

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

/* The interpreter is one function, ir_evaluate() (Python/ceval.c), with a
 * label for each handler and each shared label, and dispatch is a computed
 * goto through opcode_targets (Python/ir_targets.h).  GCC and Clang keep the
 * interpreter's state in registers for the whole activation, so a handler
 * that makes a call on a cold path costs nothing on its fast path
 * (pyjit/design/proposals/handlers81.md, section 3).  The state is:
 *
 *   frame       the executing frame, tstate->current_frame; a handler that
 *               switches frames assigns it before it dispatches;
 *   tstate      the thread state;
 *   next_instr  the word to run next;
 *   ir_word     the word of the instruction, which dispatch loads and from
 *               which the handler decodes its fields, IR_A(ir_word) and so
 *               on (Include/internal/pycore_ir.h).
 *
 * These are locals of ir_evaluate(), and none has its address taken: that
 * would put it in memory in every handler and add a stack-protector canary.
 * A handler never reloads one after a call that cannot change it.  Helpers
 * take values and return a value, a status or a {frame, next} pair; none
 * jumps into the evaluator.
 *
 * A handler that runs another on the same word, as INSTRUMENTED_LINE does
 * and a specialisation that deoptimises to its family head, jumps to it with
 * ir_word unchanged, so the other handler finds the same fields.
 *
 * Every handler also has a function form, ir_run_X(tstate, frame,
 * next_instr, ir_word), which returns a _PyIRTransfer (tiers87, C2).  The
 * bodies leave only through the transfer macros below, which this file binds
 * for the evaluator and Python/ceval.c binds for the function forms; the
 * evaluator's shared paths are the VM routines of Python/ceval_routines.h,
 * bound the same way. */
#if !defined(__GNUC__) && !defined(__clang__)
#   error "The interpreter needs a C compiler with computed gotos."
#endif

#define TARGET(op) TARGET_##op:
#define LABEL(name) LABEL_##name:

/* A label that the inline helpers of its code are to be inlined into: GCC
 * estimates the target of a plain goto as unlikely and optimises the code
 * for size, which outlines them. */
#define HOT_LABEL(name) LABEL_##name: __attribute__((hot));

/* The evaluator's binding of the VM routines (Python/ceval_routines.h): a
 * routine is a label, its operands are locals of ir_evaluate() of the same
 * names, and its exits share the dispatch blocks gen_dispatch and
 * gen_stepped, so that the evaluator has one dispatch for them.  The plain
 * generator transfers are hot labels (HOT_LABEL); the error routine is not,
 * since the profile estimate that a hot label gives every error path that
 * reaches it would optimise the handlers' fast paths for size. */
#define IR_ROUTINE(NAME, ...) LABEL_##NAME:
#define IR_HOT_ROUTINE(NAME, ...) HOT_LABEL(NAME)
#define GEN_DISPATCH() goto LABEL_gen_dispatch
#define GEN_STEP_JUMP(NAME, F, IP, V) \
    do { \
        gen_transfer = ir_##NAME(tstate, (F), (IP), (V).bits); \
        goto LABEL_gen_stepped; \
    } while (0)
#define GEN_STEP_EFFECT(REMAINING, IP, V) \
    do { \
        gen_transfer = ir_GEN_COMPLETE_EFFECT(tstate, frame, (IP), (V).bits, \
                                              (REMAINING)); \
        goto LABEL_gen_stepped; \
    } while (0)

/* The generator transfers.  The plain send, yield and return of a delegation
 * chain run in the VM routines gen_resume, gen_yield, gen_return and
 * gen_complete (Python/ceval_routines.h): a handler passes the routine its
 * operands, here in the locals of ir_evaluate() that the routine's label
 * reads.  A transfer that is not plain, because a callback, a hook or a
 * pending redirect must observe it, runs the steps of
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
            goto LABEL_outlined_exit; \
        } \
        int opcode; \
        DISPATCH(); \
    } while (0)
#define IR_GEN_JUMP(NAME, F, IP, V) \
    IR_STEP_DONE(ir_##NAME(tstate, (F), (IP), (V).bits))

/* The yielded value V passes up from the SEND at SITE, whose YIELD follows
 * it, through every delegating frame above. */
#define IR_GEN_YIELD_AT(SITE, V) \
    do { \
        gen_value = (V); \
        next_instr = (SITE); \
        goto LABEL_gen_yield; \
    } while (0)
/* The SEND or CLEANUP_THROW at SITE completes with V, whose word has a
 * native completed outcome. */
#define IR_GEN_COMPLETE_AT(SITE, V) \
    do { \
        gen_value = (V); \
        next_instr = (SITE); \
        goto LABEL_gen_complete; \
    } while (0)
/* The generator frame DYING has returned V and been unlinked and debited;
 * frame is its resumer.  EMPTY: the return is the certified
 * RETURN_VALUE_EMPTY, so that only the locals of DYING own values. */
#define IR_GEN_RETURN(V, DYING, EMPTY) \
    do { \
        gen_value = (V); \
        gen_dying = (DYING); \
        gen_empty = (EMPTY); \
        goto LABEL_gen_return; \
    } while (0)
/* The generator that frame has just entered resumes with V, and follows the
 * chain of delegating frames that the words of the frame describe. */
#define IR_GEN_RESUME(V) \
    do { \
        gen_value = (V); \
        goto LABEL_gen_resume; \
    } while (0)

/* The forms of these that do not depend on the binding */
#define IR_GEN_YIELD(V) IR_GEN_YIELD_AT(this_instr, (V))
/* Likewise V from the frame that a delegating YIELD has just suspended, if
 * its resumer is a SEND with a native yielded outcome.  A leaf YIELD does not
 * test: its resumer is a FOR_ITER_GEN or the C entry as often as a SEND, and
 * the YIELD that follows a SEND carries a value up the chain from there. */
#define IR_TRY_NATIVE_YIELD(V) \
    do { \
        if (generator_site_kind(frame, frame->instr_ptr, \
                                generator_yielded_targets) && \
            !frame->pending_redirect) { \
            assert(_PyValue_IsHome(V)); \
            IR_GEN_YIELD_AT(frame->instr_ptr, (V)); \
        } \
    } while (0)
#define IR_GEN_COMPLETE(V) IR_GEN_COMPLETE_AT(this_instr, (V))

/* A call that the function forms make in place of a jump, which must not
 * grow the C stack: a chain of deoptimisations or frame entries stays one
 * C frame deep. */
#define Py_MUSTTAIL __attribute__((musttail))

/* Every IR word is one word, so every handler's entry computes the same
 * next_instr + 1.  Left alone, RTL partial redundancy elimination hoists that
 * addition into the dispatch blocks, where it is a sixth value live across
 * the dispatch (frame, tstate, next_instr, ir_word and next_instr + 1), more
 * than the callee-saved registers hold.  The empty asm changes next_instr in
 * a way that the optimiser cannot see through, so the advance stays in the
 * handler.  The structural alternative, where DISPATCH advances and a handler
 * uses next_instr - 1 (8957f9fc4dd, reverted by 2052561d888), clears the
 * hoist too but costs one more instruction per dispatched word: it measured
 * about 1% more instructions on every kernel than this form (29c4c49c650). */
#define IR_HOLD_NEXT() __asm__("" : "+r"(next_instr))
#define DISPATCH_GOTO() goto *opcode_targets[opcode]
/* An error leaves the handler for the shared label process_error, which
 * unwinds by the exception table and either dispatches at the handler found
 * or leaves the evaluator. */
#define PROCESS_ERROR_AT(ingress, checkpoint, origin) \
    do { \
        process_error_origin = (origin); \
        process_error_entry = _PyIR_ERROR_ENTRY(_PyIR_ERROR_INGRESS_##ingress, checkpoint); \
        goto LABEL_process_error; \
    } while (0)
/* The activation ends with the value V (INTERPRETER_EXIT, or an error that no
 * handler catches) */
#define IR_EXIT(V) return (V)
#define JUMP_TO_LABEL(name) goto LABEL_##name
/* A specialisation deoptimises to its family head, which runs the word
 * again from its start. */
#define JUMP_TO_PREDICTED(name) \
    do { \
        next_instr = this_instr; \
        goto TARGET_##name; \
    } while (0)

/* The activation of the evaluation loop that runs the handler
 * (_PyEvalActivation, Python/ceval.h) */
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


/* Do interpreter dispatch accounting for tracing and instrumentation */
#define DISPATCH() \
    { \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_SAME_OPARG() \
    { \
        opcode = IR_HANDLER(*next_instr); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

/* The handler of an outlined instruction (Tools/cases_generator/ir_generator.py,
 * rule 4): its body is the function FN, which returns the frame and next word
 * to run, or no word for an exit that the activation describes.  The handler
 * assigns the state and dispatches, as a helper of rule 13 requires. */
#define IR_RUN_OUTLINED(FN) \
    do { \
        _PyIRTransfer ir_transfer = FN(tstate, frame, next_instr, ir_word); \
        frame = ir_transfer.frame; \
        next_instr = ir_transfer.next; \
        if (next_instr == NULL) { \
            goto LABEL_outlined_exit; \
        } \
        int opcode; \
        DISPATCH(); \
    } while (0)

/* A conditional jump of an outlined handler reports whether it jumped, which
 * its instrumented twin needs for its event and cannot tell from the word it
 * is to run when the jump is of no distance (rule 12).  The evaluator's own
 * handlers do not report. */
#define IR_REPORT_EDGE(TAKEN) ((void)(TAKEN))

/* The event of an instrumented instruction that follows its effect (rule 12)
 * runs in a cold function, ir_event_edge(), which returns the word to run or
 * no word for an error that the activation describes. */
#define IR_OBSERVE_EDGE(SOURCE, NEXT, EVENT, EDGE) \
    do { \
        _PyIRTransfer ir_t = ir_event_edge(tstate, frame, this_instr, \
                                           (SOURCE), (NEXT), (EVENT), (EDGE)); \
        next_instr = ir_t.next; \
        if (next_instr == NULL) { \
            goto LABEL_outlined_exit; \
        } \
        DISPATCH(); \
    } while (0)

/* The handler of an instrumented twin whose event precedes the effect (rule
 * 12): the event runs in the cold function FN, then the twin goes to the
 * handler of its base with the same word.  FN returns the word to run, the
 * twin's own word to go on, or no word for an error. */
#define IR_RUN_EVENT(FN, BASE) \
    do { \
        _PyIRTransfer ir_t = FN(tstate, frame, next_instr, ir_word); \
        if (ir_t.next == NULL) { \
            goto LABEL_outlined_exit; \
        } \
        if (ir_t.next != next_instr) { \
            next_instr = ir_t.next; \
            int opcode; \
            DISPATCH(); \
        } \
        goto TARGET_##BASE; \
    } while (0)

#define DISPATCH_INLINED(NEW_FRAME)                              \
    do {                                                         \
        assert(!IS_PEP523_HOOKED(tstate));                       \
        assert((NEW_FRAME)->previous == frame);                  \
        frame = tstate->current_frame = (NEW_FRAME);             \
        CALL_STAT_INC(inlined_py_calls);                         \
        JUMP_TO_LABEL(start_frame);                              \
    } while (0)

/* Tuple access macros */

#ifndef Py_DEBUG
#define GETITEM(v, i) _PyTuple_GET_OBJECT((v), (i))
#else
static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i) {
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    return _PyTuple_GET_OBJECT(v, i);
}
#endif

/* Code access macros */

/* In words */
#define INSTR_OFFSET() ((int)(next_instr - frame->ir_words))
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
#define LOCALS_ARRAY    IR_SLOTS()
#define GETLOCAL(i)     (IR_SLOTS()[i])


#ifdef Py_STATS
#define UPDATE_MISS_STATS(INSTNAME)                              \
    do {                                                         \
        STAT_INC(opcode, miss);                                  \
        STAT_INC((INSTNAME), miss);                              \
        if (ADAPTIVE_COUNTER_TRIGGERS(_PyIR_FbHeaderOf(frame, this_instr, opcode))) { \
            STAT_INC((INSTNAME), deopt);                         \
        }                                                        \
    } while (0)
#else
#define UPDATE_MISS_STATS(INSTNAME) ((void)0)
#endif


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
#define CONSTS() _PyFrame_GetCode(frame)->co_consts
#define NAMES() _PyFrame_GetCode(frame)->co_names

#if defined(WITH_DTRACE) && !defined(Py_BUILD_CORE_MODULE)
static void dtrace_function_entry(_PyInterpreterFrame *);
static void dtrace_function_return(_PyInterpreterFrame *);

#define DTRACE_FUNCTION_ENTRY()  \
    if (PyDTrace_FUNCTION_ENTRY_ENABLED()) { \
        dtrace_function_entry(frame); \
    }

#define DTRACE_FUNCTION_RETURN() \
    if (PyDTrace_FUNCTION_RETURN_ENABLED()) { \
        dtrace_function_return(frame); \
    }
#else
#define DTRACE_FUNCTION_ENTRY() ((void)0)
#define DTRACE_FUNCTION_RETURN() ((void)0)
#endif

/* Counters live in feedback entries (pycore_feedback.h).  FB is the entry
 * header of the instruction, or NULL if it has none, which never triggers.
 * Entry 0, the header that the sites without an entry share, never triggers
 * either, so nothing after a trigger writes past a header there. */
#ifdef Py_DEBUG
#  define IR_SHARED_ENTRY_NEVER_TRIGGERS(FB) \
    (assert((char *)(FB) != (char *)_PyIR_FbEntry(frame->ir_words, IR_FB_SHARED) \
            || !backoff_counter_triggers((FB)->counter)), 1)
#else
#  define IR_SHARED_ENTRY_NEVER_TRIGGERS(FB) 1
#endif
#define ADAPTIVE_COUNTER_TRIGGERS(FB) \
    ((FB) != NULL && IR_SHARED_ENTRY_NEVER_TRIGGERS(FB) \
     && backoff_counter_triggers((FB)->counter))

#ifdef Py_GIL_DISABLED
/* Counters are unreachable when thread-local bytecode is disabled,
 * so there is no need to update them. */
#define ADVANCE_ADAPTIVE_COUNTER(FB) \
    do { \
        if ((FB) != NULL && !backoff_counter_is_unreachable((FB)->counter)) { \
            (FB)->counter = advance_backoff_counter((FB)->counter); \
        } \
    } while (0);

#define PAUSE_ADAPTIVE_COUNTER(FB) \
    do { \
        _PyFbHeader *fb_ = (FB); \
        if (fb_ != NULL && !backoff_counter_is_unreachable(fb_->counter)) { \
            fb_->counter = pause_backoff_counter(fb_->counter); \
        } \
    } while (0);
#else
#define ADVANCE_ADAPTIVE_COUNTER(FB) \
    do { \
        if ((FB) != NULL) { \
            (FB)->counter = advance_backoff_counter((FB)->counter); \
        } \
    } while (0);

#define PAUSE_ADAPTIVE_COUNTER(FB) \
    do { \
        _PyFbHeader *fb_ = (FB); \
        if (fb_ != NULL) { \
            fb_->counter = pause_backoff_counter(fb_->counter); \
        } \
    } while (0);
#endif

/* The sense and the distance of a fused compare-and-jump, from the arg of
 * its entry and its word (Include/internal/pycore_ir.h) */
#define IR_FUSED_IF_FALSE() \
    ((int)(((_PyFbHeader *)fb)->arg >> 8) & 1)
#define IR_FUSED_DISTANCE() IR_DS(ir_word)

#define UNBOUNDLOCAL_ERROR_MSG \
    "cannot access local variable '%s' where it is not associated with a value"
#define UNBOUNDFREE_ERROR_MSG \
    "cannot access free variable '%s' where it is not associated with a value" \
    " in enclosing scope"
#define NAME_ERROR_MSG "name '%.200s' is not defined"

// If a trace function sets a new f_lineno and
// *then* raises, we use the destination when searching
// for an exception handler, displaying the traceback, and so on
#define INSTRUMENTED_JUMP(src, dest, event) \
do { \
    _PyInstr *_dest = (dest); \
    if (tstate->tracing) {\
        next_instr = _dest; \
    } else { \
        next_instr = _Py_call_instrumentation_jump(this_instr, tstate, event, frame, src, _dest); \
        if (next_instr == NULL) { \
            next_instr = frame->instr_ptr + 1; \
            PROCESS_ERROR_AT(RAISE, 0, next_instr - 1); \
        } \
    } \
} while (0);


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

#define _PyGC_CLEAR_ARRAY(ARR) ((void)0)

#define IS_PEP523_HOOKED(tstate) (tstate->interp->eval_frame != NULL)

/* Whether the thread has an event for _Py_HandlePending(): one load and test
 * of its eval breaker, the periodic checks' fast path. */
static inline bool
_Py_EvalBreakerHasEvents(PyThreadState *tstate)
{
    return (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) != 0;
}

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
    // Do not handle pending interrupts if the previous instruction was LOAD_SPECIAL
    // This may also not handle interrupts if a cache looks like LOAD_SPECIAL,
    // but this is benign as we won't skip periodic checks indefinitely.
    if (IR_HANDLER(frame->instr_ptr[-1]) == LOAD_SPECIAL) {
        return 0;
    }
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

#define CALL_TP_ITERITEM_NO_ESCAPE(ITER, INDEX) \
    Py_TYPE(ITER)->_tp_iteritem((ITER), (INDEX))

/* Operands of the register IR handlers (Tools/cases_generator/ir_generator.py).
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

/* Selected only by the checked interpreter-exit native-home contract. */
#define IR_DETACH_NATIVE(W, X, V) \
    (assert(frame->owner == FRAME_OWNED_BY_INTERPRETER), \
     IR_SLOT(IR_FIELD(W, X)) = PyValue_NULL, _PyIR_AdoptTemp(V))

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

/* IR_TAKEN() of a view of a required input, which is not PyValue_NULL, and
 * of one that a guard has shown to be in pointer space: a local's value then
 * gets its count with one test fewer (see PyValue_DUP() and
 * _PyValue_DUP_POINTER()). */
static inline PyValue
_PyIR_TakenRequired(PyValueBorrow view, PyValue *owner)
{
    PyValue taken = *owner;
    if (PyValue_IsNull(taken)) {
        return PyValue_DUP(view);
    }
    *owner = PyValue_NULL;
    return taken;
}
#define IR_TAKEN_REQUIRED(V, OWN) _PyIR_TakenRequired(PyValue_Borrow(V), &(OWN))

static inline PyValue
_PyIR_TakenPointer(PyValueBorrow view, PyValue *owner)
{
    PyValue taken = *owner;
    if (PyValue_IsNull(taken)) {
        return _PyValue_DUP_POINTER(view);
    }
    *owner = PyValue_NULL;
    return taken;
}
#define IR_TAKEN_POINTER(V, OWN) _PyIR_TakenPointer(PyValue_Borrow(V), &(OWN))

/* Values are canonical where they are made (the PyValue_From*()
 * constructors), so a store writes V as it is; the debug check proves it. */

/* A result whose format puts it only in a temporary: a plain store into a
 * slot that owns nothing. */
#define IR_STORE_TEMP(SLOT, V) \
    do { \
        assert(PyValue_IsNull(SLOT)); \
        (SLOT) = (V); \
        _PyValue_CheckTemp(SLOT); \
    } while (0)

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

/* IR_MOVE_TO_DST for the result V of a window-style handler, which the
 * slot SLOT above the inputs' would only pass on: a local gets V directly
 * and the slot stays empty. */
#define IR_MOVE_RESULT_TO_DST(W, X, SLOT, V) \
    do { \
        if (IR_IS_LOCAL_FIELD(W, X)) { \
            ir_old = IR_SLOT(IR_FIELD(W, X)); \
            assert(_PyValue_IsHome(V)); \
            IR_SLOT(IR_FIELD(W, X)) = (V); \
            _PyValue_CheckHome(IR_SLOT(IR_FIELD(W, X))); \
        } \
        else { \
            IR_SET_TEMP(SLOT, V); \
        } \
    } while (0)

/* The elements of an unpack go to the locals LOCALS[0..N-1], the first of
 * a run: element I goes to LOCALS[N - 1 - I] (the elements are in stack
 * order) and the old value of that local to the window's slot SLOT, if it
 * owns an object, or the slot stays empty.  The frame owns every value
 * throughout.  The handler then closes the old values from the window
 * (IR_CLOSE_WINDOW), after all the stores and only if one owns an object. */
#define IR_UNPACK_DIRECT(LOCALS, SLOT, I, N, X) \
    do { \
        PyValue *ir_local = (LOCALS) + ((N) - 1 - (I)); \
        PyValue ir_displaced = *ir_local; \
        *ir_local = (X); \
        _PyValue_CheckHome(*ir_local); \
        bool ir_owns = PyValue_RefcountOnObject(ir_displaced); \
        *(SLOT) = ir_owns ? ir_displaced : PyValue_NULL; \
        ir_unpack_owned |= ir_owns; \
    } while (0)

/* One element of an unpack: the item goes to the local, and the local's old
 * value to the window slot if it owns an object, or the slot is left empty
 * (zero), as IR_UNPACK_DIRECT does. The result says whether the slot holds
 * an owner. */
static inline bool
_PyIR_UnpackOneToLocal(PyValue *local, PyValue *slot, const PyValue *item)
{
    PyValue dup = PyValue_DUP(*item);
    PyValue old = *local;
    *local = dup;
    _PyValue_CheckHome(*local);
    bool owns = PyValue_RefcountOnObject(old);
    *slot = owns ? old : PyValue_NULL;
    return owns;
}

/* The elements of an unpack of ITEMS[0..N-1]: ITEMS[J] goes to LOCALS[J] and
 * the old value of that local to WINDOW[N - 1 - J] if it owns an object, or
 * the slot is left empty (zero). The frame owns every value throughout. The
 * result says whether the window holds an owner to close.
 *
 * A leaf function, so that the evaluator does not carry the loop's state
 * across its many live values. Two and three elements, the common unpacks,
 * are straight-line copies with no loop; the loop selects an old value's
 * ownership with a mask, without a branch. */
static Py_NO_INLINE bool
_PyIR_UnpackItemsToLocals(PyValue *locals, PyValue *window,
                          const PyValue *items, int n)
{
    if (n == 2) {
        bool owns0 = _PyIR_UnpackOneToLocal(&locals[0], &window[1], &items[0]);
        bool owns1 = _PyIR_UnpackOneToLocal(&locals[1], &window[0], &items[1]);
        return owns0 | owns1;
    }
    if (n == 3) {
        bool owns0 = _PyIR_UnpackOneToLocal(&locals[0], &window[2], &items[0]);
        bool owns1 = _PyIR_UnpackOneToLocal(&locals[1], &window[1], &items[1]);
        bool owns2 = _PyIR_UnpackOneToLocal(&locals[2], &window[0], &items[2]);
        return owns0 | owns1 | owns2;
    }
    uintptr_t owned = 0;
    PyValue *slot = window + (n - 1);
    for (int j = 0; j < n; j++, slot--) {
        PyValue item = PyValue_DUP(items[j]);
        PyValue old = locals[j];
        locals[j] = item;
        _PyValue_CheckHome(locals[j]);
        uintptr_t owns = (old.bits != 0) & ((old.bits & Py_OWNED_MASK) == 0);
        slot->bits = old.bits & -owns;
        owned |= owns;
    }
    return owned != 0;
}

/* The elements of an unpack of ITEMS[0..N-1] into the window's homes, in
 * stack order: ITEMS[N - 1 - I] to WINDOW[I]. */
static Py_NO_INLINE void
_PyIR_UnpackItemsToWindow(_PyInterpreterFrame *frame, PyValue *window,
                          const PyValue *items, int n)
{
    for (int i = 0; i < n; i++) {
        _PyFrame_WriteHome(frame, &window[i], PyValue_XDUP(items[n - 1 - i]));
    }
}

/* The elements of an unpack that a function wrote into WINDOW in stack
 * order (element i in WINDOW[N - 1 - i]) go to the locals LOCALS[0..N-1].
 * Each local's old value changes places with the element's slot if it owns
 * an object, and the slot is left empty otherwise.  The frame owns every
 * value throughout.  The result says whether the window holds an owner to
 * close. */
static inline bool
_PyIR_UnpackToLocals(PyValue *locals, PyValue *window, int n)
{
    bool owned = false;
    for (int i = 0; i < n; i++) {
        PyValue *element = &window[n - 1 - i];
        PyValue old = locals[i];
        locals[i] = *element;
        _PyValue_CheckHome(locals[i]);
        bool owns = PyValue_RefcountOnObject(old);
        *element = owns ? old : PyValue_NULL;
        owned |= owns;
    }
    return owned;
}
#define IR_UNPACK_TO_LOCALS(L, W, N) _PyIR_UnpackToLocals((L), (W), (N))

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

/* Clear N slots from PTR on; N may be zero or less. */
#define IR_CLEAR_SLOTS(PTR, N) \
    do { \
        PyValue *ir_p = (PTR); \
        for (int ir_i = 0; ir_i < (int)(N); ir_i++) { \
            ir_p[ir_i] = PyValue_NULL; \
        } \
    } while (0)

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

/* The feedback entry at index F. */
#define IR_FB(F) (IR_CHECK_BLOCK(), (void *)_PyIR_FbEntry(frame->ir_words, (F)))

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
 * (fb_shape_read_set()): its handler is LOAD_ATTR_SHAPE and its aux is a
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

/* The method that the poly row of OWNER's type holds (a LOAD_ATTR_METHOD_
 * WITH_VALUES or _NO_DICT observation, nominated by fb_method_poly_set()),
 * or a null value to deoptimize: no row, or an inline-values receiver that
 * lost its values.  The type has a version (_GUARD_TYPE_HAS_VERSION).  The
 * type version proves, as in the monomorphic forms, that the cached class
 * value is still what the lookup finds. */
static inline PyValueBorrow
method_poly_lookup(const _PyIRWord *words, const _PyFbHeader *h, PyObject *owner)
{
    int i = poly_row_index(words, h, Py_TYPE(owner)->tp_version_tag);
    if (i < 0) {
        return PyValue_Borrow(PyValue_NULL);
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(words), h);
    if (poly->handler[i] == LOAD_ATTR_METHOD_WITH_VALUES &&
        !FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner)->valid))
    {
        return PyValue_Borrow(PyValue_NULL);
    }
    return _PyValueGuarded_Load(poly->payload[i].class_value);
}

/* The handler's view of its frame's feedback, which a handler body does not
 * name (it may not mention frame). */
#define IR_METHOD_POLY_LOOKUP(H, OWNER) \
    method_poly_lookup(frame->ir_words, (H), (OWNER))

/* The oparg that the generic handler of COMPARE_OP or CONTAINS_OP, with
 * entry FB at unit F, keeps in the arg field of the entry.  These sites
 * always have an entry of their own: the assembler assigns theirs first and
 * lowers a site that gets none to CALL_INTRINSIC_2. */
#define IR_FB_ARG(FB, F) \
    (assert((F) != IR_FB_SHARED), (int)((_PyFbHeader *)(FB))->arg)

/* A specialisation of BINARY_OP deoptimises to the generic body, run with
 * the operation that its entry, at index F, records. */
#define IR_DEOPT_BINARY_OP(F) \
    do { \
        binary_op_operation = (int)((_PyFbHeader *)IR_FB(F))->arg; \
        next_instr = this_instr; \
        goto LABEL_binary_op; \
    } while (0)

/* On entry to a frame at next_instr, its instr_ptr: if the word there is a
 * RESUME that would do nothing, because the eval breaker has nothing pending
 * and the code's instrumentation is current, start at the word after it, as
 * if it had run.  An instrumented RESUME always runs. */
#if defined(__EMSCRIPTEN__)
#  define IR_SKIP_RESUME_EMSCRIPTEN() \
    (_Py_emscripten_signal_clock != 0 && \
     (_Py_emscripten_signal_clock -= Py_EMSCRIPTEN_SIGNAL_HANDLING, 1))
#else
#  define IR_SKIP_RESUME_EMSCRIPTEN() 1
#endif
#ifdef Py_GIL_DISABLED
#  define IR_SKIP_RESUME_TLBC() \
    (frame->tlbc_index == ((_PyThreadStateImpl *)tstate)->tlbc_index)
#else
#  define IR_SKIP_RESUME_TLBC() 1
#endif
#define IR_SKIP_RESUME() IR_SKIP_RESUME_IF(IR_HANDLER(*next_instr) == RESUME)
/* IS_RESUME: the word at next_instr is a RESUME to skip */
#define IR_SKIP_RESUME_IF(IS_RESUME) \
    do { \
        assert(next_instr == frame->instr_ptr); \
        /* A predicted branch, not arithmetic on the three loads: dispatch \
         * from the next word does not wait on them. */ \
        if ((IS_RESUME) && \
            _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) == \
                FT_ATOMIC_LOAD_UINTPTR_ACQUIRE( \
                    _PyFrame_GetCode(frame)->_co_instrumentation_version) && \
            IR_SKIP_RESUME_TLBC() && IR_SKIP_RESUME_EMSCRIPTEN()) \
        { \
            next_instr++; \
        } \
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
#define IR_JIT_EVENT() \
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
                IR_STEP_DONE(jit_run(tstate, ir_unit, frame, next_instr)); \
            } \
        } \
    } while (0)
#define IR_JIT_RESUME() \
    do { \
        if (frame->jit_resume) { \
            IR_STEP_DONE(jit_resume(tstate, frame, next_instr)); \
        } \
    } while (0)
#else
#define IR_JIT_EVENT() ((void)0)
#define IR_JIT_RESUME() ((void)0)
#endif

/* The oparg and feedback entry of another instruction of the frame. */
#define _PyIR_Oparg(FRAME, INSTR) \
    _PyCode_InstrOparg(_PyFrame_GetCode(FRAME), \
                       _PyFrame_InstrOffset((FRAME), (INSTR)))
#define _PyIR_FbHeaderOf(FRAME, INSTR, HANDLER) \
    ((void)(HANDLER), \
     _PyIR_InstrFeedback(_PyFrame_GetCode(FRAME), _PyFrame_Exec(FRAME), (INSTR)))
