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

// Branch hint for generated code, such as the test before boxing an inline
// value.
#if defined(__GNUC__) || defined(__clang__)
#  define Py_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#  define Py_UNLIKELY(x) (x)
#endif

#ifdef Py_STATS
/* The handlers of the register IR that have no opcode have no statistics. */
#define INSTRUCTION_STATS(op) \
    do { \
        if ((op) < 256) { \
            PyStats *s = _PyStats_GET(); \
            OPCODE_EXE_INC(op); \
            if (s) s->opcode_stats[lastopcode].pair_count[op]++; \
            lastopcode = op; \
        } \
    } while (0)
#else
#define INSTRUCTION_STATS(op) ((void)0)
#endif

/* The interpreter is a set of handlers, one C function per handler index
 * and per label, which dispatch to the next by a guaranteed tail call
 * (musttail) through instruction_funcptr_handler_table.  They pass the
 * interpreter's state in registers (pyjit/design/proposals/
 * register-ir-v2-a3.md, section 2):
 *
 *   frame       the executing frame, tstate->current_frame; a handler that
 *               switches frames assigns it before it dispatches;
 *   slots       its slots, _PyFrame_Slots(frame);
 *   tstate      the thread state;
 *   next_instr  the word to run next;
 *   ir_word     the word of the instruction, which dispatch loads and from
 *               which the handler decodes its fields, IR_A(ir_word) and so
 *               on (Include/internal/pycore_ir.h);
 *   lastopcode  under Py_STATS, the previous handler, for pair counts.
 *
 * A handler that runs another on the same word, as INSTRUMENTED_LINE does
 * and a specialisation that deoptimises to its family head, passes ir_word
 * on, so the other handler finds the same fields. */
#ifdef Py_STATS
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, PyValue *slots, \
        PyThreadState *tstate, _PyInstr *next_instr, _PyIRWord ir_word, \
        int lastopcode
#   define TAIL_CALL_ARGS frame, slots, tstate, next_instr, ir_word, lastopcode
#   define TAIL_CALL_ARGS_AT(INSTR) \
        frame, slots, tstate, (INSTR), ir_word, lastopcode
#else
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, PyValue *slots, \
        PyThreadState *tstate, _PyInstr *next_instr, _PyIRWord ir_word
#   define TAIL_CALL_ARGS frame, slots, tstate, next_instr, ir_word
#   define TAIL_CALL_ARGS_AT(INSTR) frame, slots, tstate, (INSTR), ir_word
#endif

/* The calling convention of the handlers.  Only the attribute differs
 * between the arms, which configure records as PY_TAIL_CALL_CC
 * (preserve_none, no_callee_saved_registers or none).  MSVC comes first,
 * because _Py__has_attribute is 0 there; it is untested.  preserve_none
 * (Clang, and GCC once it ships it) keeps the state in registers that
 * survive C calls; GCC's no_callee_saved_registers keeps it in the argument
 * registers.  The last arm is plain musttail, which configure accepts and
 * the measurement harness refuses. */
#if defined(_MSC_VER) && !defined(__clang__)
#   if _MSC_VER < 1950
#       error "You need at least VS 2026 / PlatformToolset v145 for tail calling."
#   endif
#   define Py_MUSTTAIL [[msvc::musttail]]
#   define Py_PRESERVE_NONE_CC __preserve_none
#else
#   if !_Py__has_attribute(musttail)
#       error "The interpreter needs a C compiler with the musttail attribute."
#   endif
#   define Py_MUSTTAIL __attribute__((musttail))
#   if _Py__has_attribute(preserve_none)
#       define Py_PRESERVE_NONE_CC __attribute__((preserve_none))
#   elif _Py__has_attribute(no_callee_saved_registers)
#       define Py_PRESERVE_NONE_CC __attribute__((no_callee_saved_registers))
#   else
#       define Py_PRESERVE_NONE_CC
#   endif
#endif
typedef PyObject *(Py_PRESERVE_NONE_CC *py_tail_call_funcptr)(TAIL_CALL_PARAMS);

#define TARGET(op) Py_NO_INLINE PyObject *Py_PRESERVE_NONE_CC _TAIL_CALL_##op(TAIL_CALL_PARAMS)
#define LABEL(name) TARGET(name)

#define DISPATCH_GOTO() \
    do { \
        Py_MUSTTAIL return (instruction_funcptr_handler_table[opcode])(TAIL_CALL_ARGS); \
    } while (0)
#define JUMP_TO_LABEL(name) \
    do { \
        if (value_temps != NULL && value_temps->count != 0) { \
            int value_had_error = PyErr_Occurred() != NULL; \
            if (finish_value_bridges(tstate, frame, next_instr, value_temps) < 0 && !value_had_error) { \
                next_instr = frame->instr_ptr + 1; \
                Py_MUSTTAIL return (_TAIL_CALL_error)(TAIL_CALL_ARGS); \
            } \
        } \
        Py_MUSTTAIL return (_TAIL_CALL_##name)(TAIL_CALL_ARGS); \
    } while (0)
/* A specialisation deoptimises to its family head, which runs the word
 * again from its start. */
#define JUMP_TO_PREDICTED(name) \
    do { \
        assert(value_temps == NULL || value_temps->count == 0); \
        Py_MUSTTAIL return (_TAIL_CALL_##name)(TAIL_CALL_ARGS_AT(this_instr)); \
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
 * enabled, in a debug build.  Normally a no-op.  frame is a parameter of
 * the handlers, so the first check fails if a handler switches frames
 * without passing the new one on. */
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
        if (value_temps != NULL && value_temps->count != 0 && \
            finish_value_bridges(tstate, frame, next_instr, value_temps) < 0) { \
            next_instr = frame->instr_ptr + 1; JUMP_TO_LABEL(error); \
        } \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_SAME_OPARG() \
    { \
        assert(value_temps == NULL || value_temps->count == 0); \
        opcode = IR_HANDLER(*next_instr); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_INLINED(NEW_FRAME)                              \
    do {                                                         \
        assert(!IS_PEP523_HOOKED(tstate));                       \
        assert((NEW_FRAME)->previous == frame);                  \
        if (value_temps != NULL && value_temps->count != 0 &&    \
            finish_value_bridges(tstate, frame, next_instr, value_temps) < 0) { \
            _PyEval_FrameClearAndPop(tstate, (NEW_FRAME));        \
            JUMP_TO_LABEL(error);                               \
        }                                                       \
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

#define LOCALS_ARRAY    (slots)
#define GETLOCAL(i)     (slots[i])


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
 * header of the instruction, or NULL if it has none, which never triggers. */
#define ADAPTIVE_COUNTER_TRIGGERS(FB) \
    ((FB) != NULL && backoff_counter_triggers((FB)->counter))

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

#ifdef ENABLE_SPECIALIZATION
/* Multiple threads may execute these concurrently if thread-local bytecode is
 * disabled and they all execute the main copy of the bytecode. Specialization
 * is disabled in that case so the value is unused, but the RMW cycle should be
 * free of data races.
 */
#define RECORD_BRANCH_TAKEN(bitset, flag) \
    FT_ATOMIC_STORE_UINT16_RELAXED(       \
        bitset, (FT_ATOMIC_LOAD_UINT16_RELAXED(bitset) << 1) | (flag))
#else
#define RECORD_BRANCH_TAKEN(bitset, flag)
#endif

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
            next_instr = _dest + 1; \
            JUMP_TO_LABEL(error); \
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

/* Open a borrow scope and push the argument window onto it as an object
 * vector (with the PY_VECTORCALL_ARGUMENTS_OFFSET slot before it).  The scope
 * is also the native call boundary: CLEANUP releases the vector and whatever
 * the callee left on the stack.  Run CLEANUP on every path, including after
 * CONVERSION_FAILED. */
#define VALUES_TO_PYOBJECTS(ARGS, ARG_COUNT, NAME) \
    PyThreadState *NAME##_tstate = _PyThreadState_GET(); \
    _PyBorrowMark NAME##_mark = _PyBorrowScope_Enter(NAME##_tstate); \
    PyObject **NAME = _PyValueArray_ToObjectVector((ARGS), (ARG_COUNT));

#define VALUES_TO_PYOBJECTS_CLEANUP(NAME, RESULT) \
    _PyBorrowScope_Exit(NAME##_tstate, NAME##_mark);

#define CONVERSION_FAILED(NAME) ((NAME) == NULL)

#define IS_PEP523_HOOKED(tstate) (tstate->interp->eval_frame != NULL)

static inline int
check_periodics(PyThreadState *tstate) {
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
        return _Py_HandlePending(tstate);
    }
    return 0;
}

static inline int
check_periodics_at_end(PyThreadState *tstate, _PyInterpreterFrame *frame) {
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
        // Do not handle pending interrupts if the previous instruction was LOAD_SPECIAL
        // This may also not handle interrupts if a cache looks like LOAD_SPECIAL,
        // but this is benign as we won't skip periodic checks indefinitely.
        if (IR_HANDLER(frame->instr_ptr[-1]) == LOAD_SPECIAL) {
            return 0;
        }
        return _Py_HandlePending(tstate);
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

#define CALL_TP_ITERITEM_NO_ESCAPE(ITER, INDEX) \
    Py_TYPE(ITER)->_tp_iteritem((ITER), (INDEX))

/* Operands of the register IR handlers (Tools/cases_generator/ir_generator.py).
 * F is a field, IR_A(ir_word) and so on. */

/* The slot that a slot field names, an lvalue.  `slots` is the frame's
 * slots, _PyFrame_Slots(frame), which the handlers keep in a register. */
#define IR_SLOT(F) (slots[(F)])

/* Whether field F names a local, a cell or a free variable rather than a
 * temporary: the executing copy keeps co_nlocalsplus in its header.  An
 * input read through a local is borrowed in place; one read through a
 * temporary is the handler's, which it takes, leaving NULL in the slot. */
#define IR_NLOCALSPLUS() \
    ((int)(((const _PyCodeExec *)frame->ir_words) - 1)->nlocalsplus)
#define IR_IS_LOCAL(F) ((int)(F) < IR_NLOCALSPLUS())

/* The view of an input that the handler reads through field F, which may
 * name a local (the instruction's borrow_mask): the slot's bits, as a
 * PyValueBorrow, which only C locals hold and nothing releases.  The one
 * load of the slot (pyjit/design/proposals/register-ir-v2-a5b.md, 4.2). */
#define IR_VIEW(F) PyValue_Borrow(IR_SLOT(F))

/* The owner of V, the view of a temporary whose slot the same expression
 * clears: the count moves from the slot to the C local.  The one
 * count-free conversion from a borrow to an owner; only IR_CONSUME_INPUT
 * uses it. */
static inline PyValue
_PyIR_AdoptTemp(PyValueBorrow view)
{
    PyValue owner = {view.bits};
    return owner;
}

/* The handler takes the input V that it read through field F: the owner
 * of a temporary's value, whose slot becomes NULL, or PyValue_NULL for a
 * local, which keeps its value and owns it.  The generator assigns it to
 * ir_own_<F> (Tools/cases_generator/ir_generator.py, IRStack.consume). */
#define IR_CONSUME_INPUT(F, V) \
    (IR_IS_LOCAL(F) ? PyValue_NULL \
                    : (IR_SLOT(F) = PyValue_NULL, _PyIR_AdoptTemp(V)))

/* The DSL's TAKE(V) of view V, whose field's owner is OWN: a value with a
 * count of its own, for a home.  A local's (OWN is NULL) takes a new
 * count; a temporary's is the handler's. */
#define IR_TAKE(V, OWN) \
    (PyValue_IsNull(OWN) ? PyValue_XDUP(V) : (OWN))

/* The truth of a value, as TO_BOOL tests it, or -1 with an exception set */
static inline int
_PyValue_Truth(PyValueBorrow ref)
{
    if (PyValue_IsTrue(ref)) {
        return 1;
    }
    if (PyValue_IsFalse(ref) || PyValue_IsNone(ref)) {
        return 0;
    }
    if (PyValue_IsInlineInt(ref)) {
        return PyValue_InlineInt64(ref) != 0;
    }
    if (PyValue_IsInlineFloat(ref)) {
        return PyValue_InlineFloat(ref) != 0.0;
    }
    return PyObject_IsTrue(PyValue_AsKnownObjectBorrow(ref));
}
#define _PyValue_Truth(V0) (_PyValue_Truth)(PyValue_Borrow(V0))

/* A handler writes a temporary into a slot that holds NULL. */
#define IR_STORE_TEMP(SLOT, V) \
    do { \
        assert(PyValue_IsNull(SLOT)); \
        (SLOT) = _PyValue_Publish(&(V)); \
        _PyValue_CheckTemp(SLOT); \
    } while (0)

/* A handler writes V into SLOT, a temporary of its window or one that it
 * wrote before. */
#define IR_PUBLISH_TEMP(SLOT, V) \
    do { \
        (SLOT) = _PyValue_Publish(&(V)); \
        _PyValue_CheckTemp(SLOT); \
    } while (0)

/* OLD, the value that a result displaced from a local.  Releasing a last
 * reference can run __del__, so that value waits in ir_displaced until the
 * handler finishes (release_displaced in the generator); any other value
 * cannot deallocate and is released at once. */
#define IR_RETIRE_LOCAL(OLD) \
    do { \
        if (_PyValue_MayDealloc(OLD)) { \
            assert(PyValue_IsNull(ir_displaced)); \
            ir_displaced = (OLD); \
        } \
        else { \
            PyValue_Close(OLD); \
        } \
    } while (0)

/* The result V goes to dst field F, which may name a local (a folded
 * STORE_FAST, _PyIR_STORE_DST): the local's old value is released after
 * it, as STORE_FAST does (IR_RETIRE_LOCAL).  A temporary's slot holds
 * NULL, or V when the handler wrote it there before (IR_STORE_TEMP_DST). */
#define IR_STORE_DST(F, V) \
    do { \
        if (IR_IS_LOCAL(F)) { \
            PyValue ir_old = IR_SLOT(F); \
            IR_SLOT(F) = _PyValue_Publish(&(V)); \
            _PyValue_CheckHome(IR_SLOT(F)); \
            IR_RETIRE_LOCAL(ir_old); \
        } \
        else { \
            assert(PyValue_IsNull(IR_SLOT(F)) \
                   || PyValue_Is(IR_SLOT(F), (V))); \
            IR_SLOT(F) = _PyValue_Publish(&(V)); \
            _PyValue_CheckTemp(IR_SLOT(F)); \
        } \
    } while (0)

/* Before code that may run other code, the result V goes to dst field F
 * if F names a temporary.  A local gets it at the end, IR_STORE_DST. */
#define IR_STORE_TEMP_DST(F, V) \
    do { \
        if (!IR_IS_LOCAL(F)) { \
            assert(PyValue_IsNull(IR_SLOT(F)) \
                   || PyValue_Is(IR_SLOT(F), (V))); \
            IR_SLOT(F) = _PyValue_Publish(&(V)); \
            _PyValue_CheckTemp(IR_SLOT(F)); \
        } \
    } while (0)

/* A window-style handler whose dst field F may name a local moves its
 * result from SLOT of its window there, releasing the local's old value
 * as IR_STORE_DST does. */
#define IR_MOVE_TO_DST(F, SLOT) \
    do { \
        if (IR_IS_LOCAL(F)) { \
            PyValue ir_old = IR_SLOT(F); \
            IR_SLOT(F) = _PyValue_Publish(&(SLOT)); \
            _PyValue_CheckHome(IR_SLOT(F)); \
            (SLOT) = PyValue_NULL; \
            IR_RETIRE_LOCAL(ir_old); \
        } \
    } while (0)

/* Clear N slots from PTR on; N may be zero or less. */
#define IR_CLEAR_SLOTS(PTR, N) \
    do { \
        PyValue *ir_p = (PTR); \
        for (int ir_i = 0; ir_i < (int)(N); ir_i++) { \
            ir_p[ir_i] = PyValue_NULL; \
        } \
    } while (0)

/* The frame's view of its executing copy: frame->ir_words and
 * frame->ir_feedback follow the copy that instr_ptr points into. */
#define IR_CHECK_BLOCK() \
    assert(frame->ir_words == _PyFrame_Exec(frame)->words && \
           frame->ir_feedback == _PyFrame_Exec(frame)->entries)

/* The feedback entry at index F. */
#define IR_FB(F) (IR_CHECK_BLOCK(), (void *)_PyIR_FbEntry(frame->ir_feedback, (F)))

/* The oparg that the generic handler of an instruction with entry FB, at
 * index F, keeps in the arg field of the entry.  The sites that share entry
 * 0 run NOFB forms, which keep theirs in the word. */
#define IR_FB_ARG(FB, F) \
    (assert((F) != 0), (int)((_PyFbHeader *)(FB))->arg)

/* A specialisation of BINARY_OP deoptimises to the handler of the operation
 * that its entry, at index F, records. */
#define IR_DEOPT_BINARY_OP(F) \
    do { \
        assert(value_temps == NULL || value_temps->count == 0); \
        opcode = _PyIR_BINARY_OP_FIRST + \
            (int)((_PyFbHeader *)IR_FB(F))->arg; \
        next_instr = this_instr; \
        DISPATCH_GOTO(); \
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
#define IR_SKIP_RESUME() \
    do { \
        if (IR_HANDLER(*next_instr) == RESUME && \
            _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) == \
                FT_ATOMIC_LOAD_UINTPTR_ACQUIRE( \
                    _PyFrame_GetCode(frame)->_co_instrumentation_version) && \
            IR_SKIP_RESUME_TLBC() && IR_SKIP_RESUME_EMSCRIPTEN()) \
        { \
            frame->instr_ptr = next_instr; \
            next_instr++; \
        } \
    } while (0)

/* After a frame switch, V goes to the slot of the frame that control came
 * back to (frame->return_slot). */
#define IR_STORE_RETURN(V) \
    _PyFrame_SetReturnValue(frame, _PyValue_Publish(&(V)))

/* The oparg and feedback entry of another instruction of the frame. */
#define _PyIR_Oparg(FRAME, INSTR) \
    _PyCode_InstrOparg(_PyFrame_GetCode(FRAME), \
                       _PyFrame_InstrOffset((FRAME), (INSTR)))
#define _PyIR_FbHeaderOf(FRAME, INSTR, HANDLER) \
    ((void)(HANDLER), \
     _PyIR_InstrFeedback(_PyFrame_GetCode(FRAME), _PyFrame_Exec(FRAME), (INSTR)))



