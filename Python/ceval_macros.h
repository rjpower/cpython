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

/* The word that dispatch loads, from which the handler extracts its fields
 * (Include/internal/pycore_ir.h). */
#   define TAIL_CALL_OPERANDS _PyIRWord ir_word
#   define TAIL_CALL_OPERAND_ARGS ir_word
#   define TAIL_CALL_NO_OPERANDS 0

#ifdef Py_STATS
#   define TAIL_CALL_PARAMS _PyStackRef *slots, PyThreadState *tstate, _PyInstr *next_instr, TAIL_CALL_OPERANDS, int lastopcode
#   define TAIL_CALL_ARGS slots, tstate, next_instr, TAIL_CALL_OPERAND_ARGS, lastopcode
#else
#   define TAIL_CALL_PARAMS _PyStackRef *slots, PyThreadState *tstate, _PyInstr *next_instr, TAIL_CALL_OPERANDS
#   define TAIL_CALL_ARGS slots, tstate, next_instr, TAIL_CALL_OPERAND_ARGS
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
/* A handler finds the frame in the thread state, which is not one of the
 * registers that the handlers pass on. */
#   define TAIL_CALL_LOAD_FRAME() \
        _PyInterpreterFrame *frame = tstate->current_frame

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
                Py_MUSTTAIL return (_TAIL_CALL_##name)(slots, tstate, this_instr, TAIL_CALL_OPERAND_ARGS, lastopcode); \
            } while (0)
#   else
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(slots, tstate, this_instr, TAIL_CALL_OPERAND_ARGS); \
            } while (0)
#   endif
#    define LABEL(name) TARGET(name)
#elif USE_COMPUTED_GOTOS
#  define TARGET(op) TARGET_##op:
#  define DISPATCH_GOTO() goto *opcode_targets[opcode]
#  define JUMP_TO_LABEL(name) goto name;
#  define JUMP_TO_PREDICTED(name) goto PREDICTED_##name;
#  define LABEL(name) name:
#  define TAIL_CALL_LOAD_FRAME() ((void)0)
#else
#  define TARGET(op) case op: TARGET_##op:
#  define DISPATCH_GOTO() goto dispatch_opcode
#  define JUMP_TO_LABEL(name) goto name;
#  define JUMP_TO_PREDICTED(name) goto PREDICTED_##name;
#  define LABEL(name) name:
#  define TAIL_CALL_LOAD_FRAME() ((void)0)
#endif

/* Dispatch loads the word of the instruction into ir_word and takes the
 * handler from it.  A handler reads field A, B, C or D, or CD, of its word W,
 * which is this_instr, as IR_FIELD_A(W) and so on.  A handler that runs
 * another on the same word, as INSTRUMENTED_LINE does and a specialisation
 * that deoptimises to its family head's PREDICTED_ label, leaves ir_word as
 * it is, so the other handler finds the same fields.
 *
 * Fields never change once a block is lowered, so with the GIL the
 * computed-goto build reads each from W in memory, with a 16-bit load and a
 * shift or a mask.  Decoding ir_word instead keeps it live through the
 * handler and costs a copy, a shift and a mask per field; GCC then keeps
 * next_instr on the stack, which adds a load and a store to every handler.
 * The tail-call build, which receives ir_word as an argument, and the
 * free-threaded build, which loads it atomically, decode ir_word. */
#if USE_COMPUTED_GOTOS && !_Py_TAIL_CALL_INTERP \
    && !defined(Py_GIL_DISABLED) && PY_LITTLE_ENDIAN
/* The unsigned integer of N bytes at byte K of the word at W */
static inline uintptr_t
ir_bytes_at(const _PyIRWord *w, int k, int n)
{
    if (n == 2) {
        uint16_t v;
        memcpy(&v, (const char *)w + k, sizeof(v));
        return v;
    }
    uint32_t v;
    memcpy(&v, (const char *)w + k, sizeof(v));
    return v;
}
/* In a debug build, the field in memory is the one in ir_word */
static inline uintptr_t
ir_check_field(uintptr_t field, uint64_t expected)
{
    assert(field == expected);
    (void)expected;
    return field;
}
#  define IR_MEMORY_FIELD(W, K, N, SHIFT, MASK, F) \
    ir_check_field((ir_bytes_at((W), (K), (N)) >> (SHIFT)) & (MASK), F(ir_word))
#  define IR_FIELD_A(W) IR_MEMORY_FIELD(W, 2, 2, 0, IR_FIELD_MAX, IR_A)
#  define IR_FIELD_B(W) IR_MEMORY_FIELD(W, 3, 2, 4, IR_FIELD_MAX, IR_B)
#  define IR_FIELD_C(W) IR_MEMORY_FIELD(W, 5, 2, 0, IR_FIELD_MAX, IR_C)
#  define IR_FIELD_D(W) IR_MEMORY_FIELD(W, 6, 2, 4, IR_FIELD_MAX, IR_D)
#  define IR_FIELD_CD(W) IR_MEMORY_FIELD(W, 4, 4, 8, IR_CD_MAX, IR_CD)
#else
#  define IR_FIELD_A(W) IR_A(ir_word)
#  define IR_FIELD_B(W) IR_B(ir_word)
#  define IR_FIELD_C(W) IR_C(ir_word)
#  define IR_FIELD_D(W) IR_D(ir_word)
#  define IR_FIELD_CD(W) IR_CD(ir_word)
#endif
#define IR_PREDICTED(op) PREDICTED_##op:

/* PRE_DISPATCH_GOTO() does lltrace if enabled. Normally a no-op */
#ifdef Py_DEBUG
/* The tail-call handlers find the frame in tstate->current_frame. */
#define PRE_DISPATCH_GOTO() \
    assert(frame == tstate->current_frame); \
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


/* Stackref macros */

/* How much scratch space to give stackref to PyObject* conversion. */
#define MAX_STACKREF_SCRATCH 10

#ifdef Py_GC
/* The collector scans the C stack conservatively: the unused slots of the
   scratch array are cleared, or the words that earlier calls left in that
   stack memory would keep their objects alive. */
#define STACKREFS_TO_PYOBJECTS(ARGS, ARG_COUNT, NAME) \
    /* +1 because vectorcall might use -1 to write self */ \
    PyObject *NAME##_temp[MAX_STACKREF_SCRATCH+1] = {0}; \
    PyObject **NAME = _PyObjectArray_FromStackRefArray(ARGS, ARG_COUNT, NAME##_temp);
#else
#define STACKREFS_TO_PYOBJECTS(ARGS, ARG_COUNT, NAME) \
    /* +1 because vectorcall might use -1 to write self */ \
    PyObject *NAME##_temp[MAX_STACKREF_SCRATCH+1]; \
    PyObject **NAME = _PyObjectArray_FromStackRefArray(ARGS, ARG_COUNT, NAME##_temp);
#endif

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
 * F is a field, IR_FIELD_A(this_instr) and so on. */

/* The slot that a slot field names, an lvalue.  `slots` is the frame's
 * slots, _PyFrame_Slots(frame), which the handlers keep in a register. */
#define IR_SLOT(F) (slots[(F)])

/* Whether field F names a local, a cell or a free variable rather than a
 * temporary: the executing copy keeps co_nlocalsplus in its header.  An
 * input read through a local is borrowed; one read through a temporary is
 * the handler's, which it takes, leaving NULL in the slot. */
#define IR_NLOCALSPLUS() \
    ((int)(((const _PyCodeExec *)frame->ir_words) - 1)->nlocalsplus)
#define IR_IS_LOCAL(F) ((int)(F) < IR_NLOCALSPLUS())

/* REF, read through field F and known to be a pointer, borrowed if F names
 * a local. */
static_assert(Py_TAG_REFCNT == 1, "IR_BORROW_KNOWN sets the tag from a test");
#ifdef Py_STACKREF_DEBUG
#  define IR_BORROW_KNOWN(REF, F) \
    (IR_IS_LOCAL(F) ? PyStackRef_Borrow(REF) : (REF))
#else
#  define IR_BORROW_KNOWN(REF, F) \
    ((_PyStackRef){ .bits = (REF).bits | (uintptr_t)IR_IS_LOCAL(F) })
#endif

/* The box of an inline value read through field F, which belongs to the
 * slot if F names a temporary.  A local keeps its inline value, and the
 * handler drops the box as it would a temporary's. */
#define IR_KEEP_BOX(REF, F) \
    do { \
        if (!IR_IS_LOCAL(F)) { \
            IR_SLOT(F) = (REF); \
        } \
    } while (0)

/* Before a deopt, drop REF, an object input read through field F, if F
 * names a local: the family head reads the local again.  REF is a borrow,
 * which only the Py_STACKREF_DEBUG checker counts, or the box of the
 * local's inline value. */
#define IR_DROP_BORROW(REF, F) \
    do { \
        if (IR_IS_LOCAL(F)) { \
            PyStackRef_CLOSE(REF); \
        } \
    } while (0)

/* An input that the handler consumes as it is, through field F: a
 * temporary, or a borrow of the local that F names, which is what
 * LOAD_FAST_BORROW would have pushed. */
#if defined(Py_STACKREF_DEBUG) || !defined(Py_STACKREF_INLINE_VALUES)
#  define IR_LOAD_BORROWED(F) \
    (IR_IS_LOCAL(F) ? PyStackRef_BorrowAt(&IR_SLOT(F)) : IR_SLOT(F))
#else
#  define IR_LOAD_BORROWED(F) \
    ((_PyStackRef){ .bits = IR_SLOT(F).bits \
        | ((uintptr_t)IR_IS_LOCAL(F) & _PyStackRef_BorrowBitAt(&IR_SLOT(F))) })
#endif

/* The truth of a value, as TO_BOOL tests it, or -1 with an exception set */
static inline int
_PyStackRef_Truth(_PyStackRef ref)
{
    if (PyStackRef_IsTrue(ref)) {
        return 1;
    }
    if (PyStackRef_IsFalse(ref) || PyStackRef_IsNone(ref)) {
        return 0;
    }
    if (PyStackRef_IsInlineInt(ref)) {
        return PyStackRef_InlineInt(ref) != 0;
    }
    if (PyStackRef_IsInlineFloat(ref)) {
        return PyStackRef_InlineFloat(ref) != 0.0;
    }
    return PyObject_IsTrue(PyStackRef_AsPyObjectBorrow(ref));
}

/* The handler takes the input that it read through field F: a temporary's
 * slot becomes NULL, and a local keeps its value. */
#define IR_CONSUME_INPUT(F) \
    do { \
        if (!IR_IS_LOCAL(F)) { \
            IR_SLOT(F) = PyStackRef_NULL; \
        } \
    } while (0)

/* V, the input read through field F or what replaced it, goes back to a
 * temporary's slot. */
#define IR_STORE_INPUT(F, V) \
    do { \
        if (!IR_IS_LOCAL(F)) { \
            IR_SLOT(F) = (V); \
        } \
    } while (0)

/* A handler writes a temporary into a slot that holds NULL. */
#define IR_STORE_TEMP(SLOT, V) \
    do { \
        assert(PyStackRef_IsNull(SLOT)); \
        (SLOT) = (V); \
    } while (0)

/* The result V goes to dst field F, which may name a local (a folded
 * STORE_FAST, _PyIR_STORE_DST): the local's old value is released after
 * it, as STORE_FAST does.  A temporary's slot holds NULL, or V when the
 * handler wrote it there before (IR_STORE_TEMP_DST). */
#define IR_STORE_DST(F, V) \
    do { \
        if (IR_IS_LOCAL(F)) { \
            _PyStackRef ir_old = IR_SLOT(F); \
            IR_SLOT(F) = (V); \
            PyStackRef_XCLOSE(ir_old); \
        } \
        else { \
            assert(PyStackRef_IsNull(IR_SLOT(F)) \
                   || _PyStackRef_BITS(IR_SLOT(F)) == _PyStackRef_BITS(V)); \
            IR_SLOT(F) = (V); \
        } \
    } while (0)

/* Before code that may run other code, the result V goes to dst field F
 * if F names a temporary.  A local gets it at the end, IR_STORE_DST. */
#define IR_STORE_TEMP_DST(F, V) \
    do { \
        if (!IR_IS_LOCAL(F)) { \
            assert(PyStackRef_IsNull(IR_SLOT(F)) \
                   || _PyStackRef_BITS(IR_SLOT(F)) == _PyStackRef_BITS(V)); \
            IR_SLOT(F) = (V); \
        } \
    } while (0)

/* A window-style handler whose dst field F may name a local moves its
 * result from SLOT of its window there, releasing the local's old value. */
#define IR_MOVE_TO_DST(F, SLOT) \
    do { \
        if (IR_IS_LOCAL(F)) { \
            _PyStackRef ir_old = IR_SLOT(F); \
            IR_SLOT(F) = (SLOT); \
            (SLOT) = PyStackRef_NULL; \
            PyStackRef_XCLOSE(ir_old); \
        } \
    } while (0)

/* Clear N slots from PTR on; N may be zero or less. */
#define IR_CLEAR_SLOTS(PTR, N) \
    do { \
        _PyStackRef *ir_p = (PTR); \
        for (int ir_i = 0; ir_i < (int)(N); ir_i++) { \
            ir_p[ir_i] = PyStackRef_NULL; \
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
#define IR_STORE_RETURN(V) _PyFrame_SetReturnValue(frame, (V))

/* The oparg and feedback entry of another instruction of the frame. */
#define _PyIR_Oparg(FRAME, INSTR) \
    _PyCode_InstrOparg(_PyFrame_GetCode(FRAME), \
                       _PyFrame_InstrOffset((FRAME), (INSTR)))
#define _PyIR_FbHeaderOf(FRAME, INSTR, HANDLER) \
    ((void)(HANDLER), \
     _PyIR_InstrFeedback(_PyFrame_GetCode(FRAME), _PyFrame_Exec(FRAME), (INSTR)))

