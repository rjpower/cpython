// Macros and other things needed by ceval.c, and bytecodes.c

/* Computed GOTOs, or
       the-optimization-commonly-but-improperly-known-as-"threaded code"
   using gcc's labels-as-values extension
   (http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html).

   The traditional bytecode evaluation loop uses a "switch" statement, which
   decent compilers will optimize as a single indirect branch instruction
   combined with a lookup table of jump addresses. However, since the
   indirect jump instruction is shared by all opcodes, the CPU will have a
   hard time making the right prediction for where to jump next (actually,
   it will be always wrong except in the uncommon case of a sequence of
   several identical opcodes).

   "Threaded code" in contrast, uses an explicit jump table and an explicit
   indirect jump instruction at the end of each opcode. Since the jump
   instruction is at a different address for each opcode, the CPU will make a
   separate prediction for each of these instructions, which is equivalent to
   predicting the second opcode of each opcode pair. These predictions have
   a much better chance to turn out valid, especially in small bytecode loops.

   A mispredicted branch on a modern CPU flushes the whole pipeline and
   can cost several CPU cycles (depending on the pipeline depth),
   and potentially many more instructions (depending on the pipeline width).
   A correctly predicted branch, however, is nearly free.

   At the time of this writing, the "threaded code" version is up to 15-20%
   faster than the normal "switch" version, depending on the compiler and the
   CPU architecture.

   NOTE: care must be taken that the compiler doesn't try to "optimize" the
   indirect jumps by sharing them between all opcodes. Such optimizations
   can be disabled on gcc by using the -fno-gcse flag (or possibly
   -fno-crossjumping).
*/

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

#ifdef HAVE_COMPUTED_GOTOS
    #ifndef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 1
    #endif
#else
    #if defined(USE_COMPUTED_GOTOS) && USE_COMPUTED_GOTOS
    #error "Computed gotos are not supported on this compiler."
    #endif
    #undef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 0
#endif

#ifdef Py_STATS
#define INSTRUCTION_STATS(op) \
    do { \
        PyStats *s = _PyStats_GET(); \
        OPCODE_EXE_INC(op); \
        if (s) s->opcode_stats[lastopcode].pair_count[op]++; \
        lastopcode = op; \
    } while (0)
#else
#define INSTRUCTION_STATS(op) ((void)0)
#endif

#ifdef Py_STATS
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _Py_CODEUNIT *next_instr, int oparg, int lastopcode
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, oparg, lastopcode
#else
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _Py_CODEUNIT *next_instr, int oparg
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, oparg
#endif

#if _Py_TAIL_CALL_INTERP
#   if defined(__clang__) || defined(__GNUC__)
#       if !_Py__has_attribute(preserve_none) || !_Py__has_attribute(musttail)
#           error "This compiler does not have support for efficient tail calling."
#       endif
#   elif defined(_MSC_VER) && (_MSC_VER < 1950)
#       error "You need at least VS 2026 / PlatformToolset v145 for tail calling."
#   endif
#   if defined(_MSC_VER) && !defined(__clang__)
#      define Py_MUSTTAIL [[msvc::musttail]]
#      define Py_PRESERVE_NONE_CC __preserve_none
#   else
#       define Py_MUSTTAIL __attribute__((musttail))
#       define Py_PRESERVE_NONE_CC __attribute__((preserve_none))
#   endif
    typedef PyObject *(Py_PRESERVE_NONE_CC *py_tail_call_funcptr)(TAIL_CALL_PARAMS);

#   define TARGET(op) Py_NO_INLINE PyObject *Py_PRESERVE_NONE_CC _TAIL_CALL_##op(TAIL_CALL_PARAMS)

#   define DISPATCH_GOTO() \
        do { \
            Py_MUSTTAIL return (instruction_funcptr_handler_table[opcode])(TAIL_CALL_ARGS); \
        } while (0)
#   define JUMP_TO_LABEL(name) \
        do { \
            Py_MUSTTAIL return (_TAIL_CALL_##name)(TAIL_CALL_ARGS); \
        } while (0)
#   ifdef Py_STATS
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, oparg, lastopcode); \
            } while (0)
#   else
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, oparg); \
            } while (0)
#   endif
#    define LABEL(name) TARGET(name)
#elif USE_COMPUTED_GOTOS
#  define TARGET(op) TARGET_##op:
#  define DISPATCH_GOTO() goto *opcode_targets[opcode]
#  define JUMP_TO_LABEL(name) goto name;
#  define JUMP_TO_PREDICTED(name) goto PREDICTED_##name;
#  define LABEL(name) name:
#else
#  define TARGET(op) case op: TARGET_##op:
#  define DISPATCH_GOTO() goto dispatch_opcode
#  define JUMP_TO_LABEL(name) goto name;
#  define JUMP_TO_PREDICTED(name) goto PREDICTED_##name;
#  define LABEL(name) name:
#endif

/* PRE_DISPATCH_GOTO() does lltrace if enabled. Normally a no-op */
#ifdef Py_DEBUG
#define PRE_DISPATCH_GOTO() if (frame->lltrace >= 5) { \
    lltrace_instruction(frame, stack_pointer, next_instr, opcode, oparg); }
#else
#define PRE_DISPATCH_GOTO() ((void)0)
#endif

#ifdef Py_DEBUG
#define LLTRACE_RESUME_FRAME() \
do { \
    _PyFrame_SetStackPointer(frame, stack_pointer); \
    int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS()); \
    stack_pointer = _PyFrame_GetStackPointer(frame); \
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
        _PyFrame_StackAssertInvalid(frame); \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_SAME_OPARG() \
    { \
        opcode = next_instr->op.code; \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_INLINED(NEW_FRAME)                              \
    do {                                                         \
        assert(!IS_PEP523_HOOKED(tstate));                       \
        _PyFrame_SetStackPointer(frame, stack_pointer);          \
        _PyFrame_StackPointerValidate(frame);                    \
        assert((NEW_FRAME)->previous == frame);                  \
        frame = tstate->current_frame = (NEW_FRAME);             \
        CALL_STAT_INC(inlined_py_calls);                         \
        JUMP_TO_LABEL(start_frame);                              \
    } while (0)

/* Tuple access macros */

#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((v), (i))
#else
static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i) {
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    return PyTuple_GET_ITEM(v, i);
}
#endif

/* Code access macros */

/* The integer overflow is checked by an assertion below. */
#define INSTR_OFFSET() ((int)(next_instr - _PyFrame_GetBytecode(frame)))
#define NEXTOPARG()  do { \
        _Py_CODEUNIT word  = {.cache = FT_ATOMIC_LOAD_UINT16_RELAXED(*(uint16_t*)next_instr)}; \
        opcode = word.op.code; \
        oparg = word.op.arg; \
    } while (0)

/* JUMPBY makes the generator identify the instruction as a jump. SKIP_OVER is
 * for advancing to the next instruction, taking into account cache entries
 * and skipped instructions.
 */
#define JUMPBY(x)       (next_instr += (x))
#define SKIP_OVER(x)    (next_instr += (x))

#define STACK_LEVEL()     ((int)(stack_pointer - _PyFrame_Stackbase(frame)))
#define STACK_SIZE()      (_PyFrame_GetCode(frame)->co_stacksize)

#define WITHIN_STACK_BOUNDS() \
   (frame->owner == FRAME_OWNED_BY_INTERPRETER || (STACK_LEVEL() >= 0 && STACK_LEVEL() <= STACK_SIZE()))

/* Data access macros */
#define FRAME_CO_CONSTS (_PyFrame_GetCode(frame)->co_consts)
#define FRAME_CO_NAMES  (_PyFrame_GetCode(frame)->co_names)

/* Local variable macros */

#define LOCALS_ARRAY    (frame->localsplus)
#define GETLOCAL(i)     (frame->localsplus[i])


#ifdef Py_STATS
#define UPDATE_MISS_STATS(INSTNAME)                              \
    do {                                                         \
        STAT_INC(opcode, miss);                                  \
        STAT_INC((INSTNAME), miss);                              \
        if (ADAPTIVE_COUNTER_TRIGGERS(_PyFb_Header(this_instr))) { \
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
// avoid any potentially escaping calls (like PyStackRef_CLOSE) while the
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
    _Py_CODEUNIT *_dest = (dest); \
    if (tstate->tracing) {\
        next_instr = _dest; \
    } else { \
        _PyFrame_SetStackPointer(frame, stack_pointer); \
        next_instr = _Py_call_instrumentation_jump(this_instr, tstate, event, frame, src, _dest); \
        stack_pointer = _PyFrame_GetStackPointer(frame); \
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

#define LOAD_SP() \
stack_pointer = _PyFrame_GetStackPointer(frame)

#define SAVE_SP() \
_PyFrame_SetStackPointer(frame, stack_pointer)

/* Stackref macros */

/* How much scratch space to give stackref to PyObject* conversion. */
#define MAX_STACKREF_SCRATCH 10

#define STACKREFS_TO_PYOBJECTS(ARGS, ARG_COUNT, NAME) \
    /* +1 because vectorcall might use -1 to write self */ \
    PyObject *NAME##_temp[MAX_STACKREF_SCRATCH+1]; \
    PyObject **NAME = _PyObjectArray_FromStackRefArray(ARGS, ARG_COUNT, NAME##_temp);

#define STACKREFS_TO_PYOBJECTS_CLEANUP(NAME) \
    /* +1 because we +1 previously */ \
    _PyObjectArray_Free(NAME - 1, NAME##_temp);

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
        if (frame->instr_ptr[-1].op.code == LOAD_SPECIAL) {
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
