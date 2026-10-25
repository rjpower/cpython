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

/* The operands that dispatch decodes: the oparg, or for the register IR
 * fields A, B and C of word 0 (Include/internal/pycore_ir.h). */
#   define TAIL_CALL_OPERANDS uint32_t ir_a, uint32_t ir_b, uint32_t ir_c
#   define TAIL_CALL_OPERAND_ARGS ir_a, ir_b, ir_c
#   define TAIL_CALL_NO_OPERANDS 0, 0, 0

#ifdef Py_STATS
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _PyInstr *next_instr, TAIL_CALL_OPERANDS, int lastopcode
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, TAIL_CALL_OPERAND_ARGS, lastopcode
#else
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _PyInstr *next_instr, TAIL_CALL_OPERANDS
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, TAIL_CALL_OPERAND_ARGS
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
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, TAIL_CALL_OPERAND_ARGS, lastopcode); \
            } while (0)
#   else
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, TAIL_CALL_OPERAND_ARGS); \
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

/* The computed-goto build of the register IR decodes fields A, B and C of
 * word 0 in each handler rather than at dispatch, so that a handler pays only
 * for the fields it uses.  Dispatch after NEXTOPARG() enters a handler at its
 * TARGET, where IR_ENTRY() decodes ir_a, ir_b and ir_c from ir_word into
 * locals.  DISPATCH_GOTO() runs another handler on the fields at hand, which
 * IR_EXT may have widened: it passes them in ir_fields to the handler's
 * IR_BODY_ label, past the decoding, and sets ir_word to IR_FIELDS_PASSED.
 * A specialisation that deoptimises jumps to the PREDICTED_ label of its
 * family head, which takes the fields from wherever its entry found them.
 * The tail-call and switch builds decode the fields at dispatch.
 *
 * Fields never change once a block is lowered, so with the GIL a handler
 * reads each from the word in memory with one 16-bit load, where shifting it
 * out of ir_word takes three instructions.  The free-threaded build decodes
 * ir_word, which it loaded atomically. */
#if USE_COMPUTED_GOTOS && !_Py_TAIL_CALL_INTERP
#  define IR_LAZY_FIELDS 1
/* No main word runs IR_EXT. */
#  define IR_FIELDS_PASSED ((_PyIRWord)IR_EXT)
#  define IR_DECODE_WORD_FIELDS() \
    (ir_a = IR_A(ir_word), ir_b = IR_B(ir_word), ir_c = IR_C(ir_word))
#  if !defined(Py_GIL_DISABLED) && PY_LITTLE_ENDIAN
/* 16-bit unit K of the word at W, through memcpy since W is a uint64_t */
static inline uintptr_t
ir_field_in_memory(const _PyIRWord *w, int k)
{
    uint16_t field;
    memcpy(&field, (const char *)w + 2 * k, sizeof(field));
    return field;
}
#    define IR_DECODE_FIELDS() \
    (ir_a = ir_field_in_memory(next_instr, 1), \
     ir_b = ir_field_in_memory(next_instr, 2), \
     ir_c = ir_field_in_memory(next_instr, 3))
#  else
#    define IR_DECODE_FIELDS() IR_DECODE_WORD_FIELDS()
#  endif
#  define IR_TAKE_FIELDS() \
    (ir_a = ir_fields[0], ir_b = ir_fields[1], ir_c = ir_fields[2])
#  undef DISPATCH_GOTO
#  define DISPATCH_GOTO() \
    do { \
        ir_fields[0] = ir_a; \
        ir_fields[1] = ir_b; \
        ir_fields[2] = ir_c; \
        ir_word = IR_FIELDS_PASSED; \
        goto *ir_body_targets[opcode]; \
    } while (0)
#  define DISPATCH_DECODE() goto *opcode_targets[opcode]
#  define IR_ENTRY(op) \
    uintptr_t ir_a, ir_b, ir_c; \
    IR_DECODE_FIELDS(); \
    if (0) { \
        IR_BODY_##op: \
        IR_TAKE_FIELDS(); \
    } \
    (void)ir_a; (void)ir_b; (void)ir_c
#  define IR_PREDICTED(op) \
    if (0) { \
        PREDICTED_##op: \
        if (ir_word == IR_FIELDS_PASSED) { \
            IR_TAKE_FIELDS(); \
        } \
        else { \
            IR_DECODE_WORD_FIELDS(); \
        } \
    }
#else
#  define IR_LAZY_FIELDS 0
#  define DISPATCH_DECODE() DISPATCH_GOTO()
#  define IR_ENTRY(op) ((void)0)
#  define IR_PREDICTED(op) PREDICTED_##op:
#endif

/* PRE_DISPATCH_GOTO() does lltrace if enabled. Normally a no-op */
#ifdef Py_DEBUG
#define PRE_DISPATCH_GOTO() if (frame->lltrace >= 5) { \
    lltrace_instruction(frame, stack_pointer, next_instr, opcode); }
#elif defined(Py_DEBUG)
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
        DISPATCH_DECODE(); \
    }

#define DISPATCH_SAME_OPARG() \
    { \
        opcode = IR_HANDLER(*next_instr); \
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

/* In words: the IR has no bytecode offsets but those of its pcmap. */
#define INSTR_OFFSET() ((int)(next_instr - _PyFrame_IRWords(frame)))
#if IR_LAZY_FIELDS
#define NEXTOPARG()  do { \
        ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*next_instr); \
        opcode = IR_HANDLER(ir_word); \
    } while (0)
#else
#define NEXTOPARG()  do { \
        _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*next_instr); \
        opcode = IR_HANDLER(word); \
        ir_a = IR_A(word); \
        ir_b = IR_B(word); \
        ir_c = IR_C(word); \
    } while (0)
#endif

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
        if (ADAPTIVE_COUNTER_TRIGGERS(_PyIR_FbHeaderOf(frame, this_instr, opcode))) { \
            STAT_INC((INSTNAME), deopt);                         \
        }                                                        \
    } while (0)
#elif defined(Py_STATS)
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
    _PyInstr *_dest = (dest); \
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
 * F is a field variable, ir_a, ir_b or ir_c. */

static_assert(IR_BORROW == Py_TAG_REFCNT,
              "IR_BORROW must be the tag of a borrowed reference");

/* The slot that a slot field names, an lvalue. */
#define IR_SLOT(F) (frame->localsplus[IR_SLOT_INDEX(F)])

/* REF, read through field F and known to be a pointer, borrowed if F
 * carries IR_BORROW. */
#ifdef Py_STACKREF_DEBUG
#  define IR_BORROW_KNOWN(REF, F) (((F) & IR_BORROW) ? PyStackRef_Borrow(REF) : (REF))
#else
#  define IR_BORROW_KNOWN(REF, F) \
    ((_PyStackRef){ .bits = (REF).bits | ((F) & IR_BORROW) })
#endif

/* Before a deopt, drop REF if IR_BORROW_KNOWN made it a borrow of the local
 * that F names: the family head borrows the local again.  Only the
 * Py_STACKREF_DEBUG checker counts such borrows. */
#ifdef Py_STACKREF_DEBUG
#  define IR_DROP_BORROW(REF, F) \
    do { \
        if ((F) & IR_BORROW) { \
            PyStackRef_CLOSE(REF); \
        } \
    } while (0)
#else
#  define IR_DROP_BORROW(REF, F) ((void)0)
#endif

/* The feedback entry at distance F from the instruction. */
#define IR_FB(INSTR, F) ((void *)((INSTR) + (F)))

/* A superinstruction runs the POP_JUMP_IF_FALSE or POP_JUMP_IF_TRUE word
 * JUMP at next_instr, which jumps if TAKEN, as that handler does. */
#define IR_JUMP_NEXT(JUMP, TAKEN) \
    do { \
        _PyFbHeader *fb_ = (_PyFbHeader *)IR_FB(next_instr, IR_B(JUMP)); \
        RECORD_BRANCH_TAKEN(fb_->history, (TAKEN)); \
        next_instr++; \
        next_instr += (TAKEN) ? (int)IR_C(JUMP) \
                              : IR_HANDLER(*next_instr) == NOT_TAKEN; \
    } while (0)

/* The stack pointer before an instruction whose dst field F names the slot
 * at OFF from it.  The loads that an instruction absorbs have no words, so
 * the stack pointer at its dispatch may lag behind.  A dst never carries
 * IR_BORROW, so F is the slot index times two. */
#define IR_STACK_POINTER(F, OFF) \
    ((_PyStackRef *)((char *)frame->localsplus + \
                     (F) * (sizeof(_PyStackRef) / 2)) - (OFF))

/* An absorbed load leaves the canonical slot of its value as it was, so
 * before code that reads the stack runs, a handler clears the slot of an
 * input that it reads through field F if F names a local. */
#define IR_CLEAR_IF_BORROWED(F, SLOT) \
    do { \
        if ((F) & IR_BORROW) { \
            (SLOT) = PyStackRef_NULL; \
        } \
    } while (0)

/* Every temporary lives in its canonical slot, at OFF from stack_pointer.
 * An input of N may instead carry IR_BORROW and name a local. */
#ifdef Py_DEBUG
#  define IR_CHECK_SLOT(F, OFF) \
    assert(((F) & IR_BORROW) == 0 && &IR_SLOT(F) == &stack_pointer[(OFF)])
#  define IR_CHECK_INPUT(F, OFF, N) \
    assert(((F) & IR_BORROW) \
           ? (int)IR_SLOT_INDEX(F) < _PyFrame_GetCode(frame)->co_nlocalsplus \
           : &IR_SLOT(F) == &stack_pointer[(OFF)])
#else
#  define IR_CHECK_SLOT(F, OFF) ((void)0)
#  define IR_CHECK_INPUT(F, OFF, N) ((void)0)
#endif

/* IR_EXT: widen the fields with its own and go on to the next word. */
#define IR_EXTEND_FIELDS() \
    do { \
        _PyIRWord word_ = FT_ATOMIC_LOAD_UINT64_RELAXED(*next_instr); \
        opcode = IR_HANDLER(word_); \
        ir_a = ir_a << 16 | IR_A(word_); \
        ir_b = ir_b << 16 | IR_B(word_); \
        ir_c = ir_c << 16 | IR_C(word_); \
    } while (0)

/* The oparg and feedback entry of another instruction, as HANDLER reads
 * them. */
#define _PyIR_Oparg(FRAME, INSTR) \
    _PyIR_InstrOparg(_PyFrame_IRBlock(FRAME), (INSTR), IR_HANDLER(*(INSTR)))
#define _PyIR_FbHeaderOf(FRAME, INSTR, HANDLER) \
    _PyIR_InstrFeedback(_PyFrame_IRBlock(FRAME), (INSTR), (HANDLER))

/* Where a generator returns to in its caller: past the SEND or FOR_ITER
 * at the caller's instr_ptr, or the trampoline's INTERPRETER_EXIT.
 * YIELD_VALUE reads the offset from gi_yield_offset, which the resuming
 * instruction records, rather than through this chain of dependent loads;
 * debug builds check that the two agree. */
static inline int
_PyIR_YieldReturnOffset(_PyInterpreterFrame *frame)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(*frame->instr_ptr));
    if (fmt->flags & _PyIR_TRANSPARENT) {
        fmt = _PyIR_FormatOf(_PyIR_RealHandler(frame, frame->instr_ptr));
    }
    return fmt->nwords;
}
