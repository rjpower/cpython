/* The state that Python/ceval.c, the evaluator, and Python/ceval_forms.c,
 * the function forms, share: the includes, the types and macros of the
 * register rows, the helpers that both inline, and the prototypes of the
 * out-of-line helpers that Python/ceval.c defines. */
#ifndef Py_CEVAL_SHARED_H
#define Py_CEVAL_SHARED_H

#include "ceval.h"
#include "pycore_attr_recipe.h"   // _PyAttr_AdmittedInstanceSlot()
#include "pycore_global_recipe.h" // _PyGlobalRecipe
#include "pycore_descrobject.h"   // _PyGetSet_ValueRead()
#include "pycore_dict.h"          // PyDict_GetValue()
#include "pycore_function.h"      // FUNC_VERSION_FIRST_VALID
#include "pycore_jit_lower.h"     // _PyEval_JitRaiseExit
#include "pycore_jit.h"           // _PyJitUnit
#include "pycore_interpolation.h" // _PyInterpolation_BuildValue()
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_long.h"
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_native_scope.h"
#include "pycore_rop_metadata.h"  // ROP_HANDLER_*
#include "pycore_rop_rows.h"      // _PyRop_AnchorForCall()
#include "pycore_setobject.h"     // PySet_AddValue()
#include "pycore_tuple.h"         // _PyTuple_PackValues()
#include "pycore_template.h"      // _PyTemplate_Build()
#include "pycore_value_call.h"    // _PyValue_VectorcallTstate()
#include "pycore_value_ops.h"     // _PyValue_AsSsize_t()

/* The prefix of Python/ceval.c: helpers that the rows call */

typedef struct {
    PyValue first;
    PyValue second;
} _PyValuePair;

typedef struct {
    int status;
    PyObject *object;
} _PyStatusObject;

typedef struct {
    int status;
    _PyInterpreterFrame *child;
} _PyCallResult;

typedef struct {
    PyObject *match;
    PyObject *rest;
} _PyObjectPair;

/* The flags of a generic call (call_run_*()): what the handler is and what it
 * knows of its word, in its register local call_state.  CALL_EFFECTS marks a
 * call that specialization already acted on, which finishes on its word
 * generically. */
#define CALL_EFFECTS 1
#define CALL_INSTRUMENTED 8
#define CALL_PERIODIC 16
#define CALL_SELF_PRESENT 32
#define CALL_KWARGS_PRESENT 64
/* The callable operand is a bound method whose function and receiver the
 * push takes in its place: the operands stay the method and NULL. */
#define CALL_EXPANDED 256

/* What a call handler reads from its word: whether the call has a self
 * operand (CALL and CALL_KW), whether it has a kwargs operand
 * (CALL_FUNCTION_EX), and the number of words of the instruction.  The
 * operands are read through the words after the head (pycore_ir.h):
 * IR_OPERAND(k) is the slot of operand k, a view that the handler reads once
 * and does not close. */
/* The self operand of a method call that LOAD_ATTR resolved to a plain
 * attribute (a module member, a class attribute) is NULL at run time: the
 * call then has no self. */
#define IR_CALL_SELF_STATE(W) \
    ((((W) & IR_F_NO_SELF) || PyValue_IsNull(IR_OPERAND(0))) \
     ? 0 : CALL_SELF_PRESENT)
#define IR_CALL_KWARGS_STATE(W) (((W) & IR_F_NO_KW) ? 0 : CALL_KWARGS_PRESENT)
#define IR_CALL_LENGTH(W) _PyIR_CallWordLength(W)
#define IR_CALL_HAS_SELF(W) (!((W) & IR_F_NO_SELF))
#define IR_OPERAND(K) IR_SLOT(_PyIR_CallOperand(this_instr, (K)))
/* Argument K of the call as the specialized handlers count them: the self
 * when there is one, else the first argument, which follows the self
 * operand's slot if the word has one that is NULL at run time. */
#define IR_CALL_ARG(K) \
    IR_OPERAND((K) + ((call_state & CALL_SELF_PRESENT) ? 0 : IR_CALL_HAS_SELF(ir_word)))
/* The first operand that a native handler passes to the callee: operand 0,
 * or operand 1 when the word has a self operand that is NULL at run time. */
#define IR_CALL_FIRST() \
    ((call_state & CALL_SELF_PRESENT) ? 0 : IR_CALL_HAS_SELF(ir_word))
/* The tuple of a CALL_KW's keyword names: its last operand is the index in
 * co_consts, which the code keeps alive. */
#define IR_CALL_KWNAMES() \
    _PyIR_AdoptTemp(PyTuple_GET_VALUE(FRAME_CO_CONSTS, \
        _PyIR_CallOperand(this_instr, (int)IR_C(ir_word) - 1)))

typedef enum {
    CALL_SELF_ABSENT,
    CALL_SELF_SUPPLIED,
} _PyCallSelfRole;

void
_PyEval_value_revalidation_error(const char *message);

/* Whether frame is still the thread's current frame and still runs the
 * executing copy of its code.  An operation can run Python; the frame and
 * its code are values the handler keeps in registers, so the check needs
 * no record of them (R-b). */
static inline int
value_frame_is_current(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    if (tstate->current_frame != frame) {
        return 0;
    }
    _PyCodeExec *exec = _PyFrame_GetCode(frame)->_co_exec;
    return exec != NULL && frame->ir_words == exec->words;
}

/* IMPORT_NAME's level as a C int, PyLong_AsInt() of it: -1 with an exception
 * set on error.  The compiler's level is an inline int. */
static inline int
value_import_level(PyValue level)
{
    if (PyValue_IsInlineInt(level)) {
        int64_t i = PyValue_InlineInt64(level);
        if (INT_MIN <= i && i <= INT_MAX) {
            return (int)i;
        }
    }
    PyObject *level_o = PyValue_AsObject(level);
    if (level_o == NULL) {
        return -1;
    }
    int ilevel = _PyValue_AsInt(PyValue_FromObjectBorrow(level_o));
    Py_DECREF(level_o);
    return ilevel;
}

PyValue
_PyEval_value_subscript(PyValueBorrow container, PyValueBorrow key);

PyValue
_PyRop_ConvertValue(int kind, PyValueBorrow v);

PyValue
_PyRop_FormatPlain(PyValueBorrow v);

PyValue
_PyRop_FormatValue(PyValueBorrow v, PyValueBorrow spec);

PyValue
_PyRop_ValueLength(PyValueBorrow v);

int
_PyRop_TypeHasFlag(int kind, PyValueBorrow v);

PyValue
_PyRop_MatchClass(int count, PyValueBorrow subject, PyValueBorrow type,
                  PyValueBorrow names);

PyValue
_PyRop_MatchKeys(PyValueBorrow subject, PyValueBorrow keys);

PyValue
_PyEval_value_binary_op(int oparg, PyValueBorrow lhs, PyValueBorrow rhs);

/* The wrappers below keep a runtime-API out-parameter in their own C frame,
 * so that the handler that calls them takes the address of none of its
 * locals (spec 3.5, R-c).  Py_NO_INLINE keeps the address from returning to
 * the handler. */




#include "ceval_macros.h"


/* Helper functions to keep the size of the largest uops down */



int _Py_CheckRecursiveCallPy(
    PyThreadState *tstate);

/* The value that a frame returns or yields to the entry frame lands in
 * entry.stack[0], slot 1 of the entry frame. */
static_assert(offsetof(_PyEntryFrame, stack) ==
              offsetof(_PyInterpreterFrame, owned_slots) + sizeof(PyValue),
              "the entry frame's stack must follow owned_slots[0]");

/* The entry frame's words: the Return arm of the entry frame
 * (Return.External).  Its slot 1 is a temporary, which the arm takes, so the
 * word carries no slot-class flag; the header precedes the words, as in
 * every executing copy, for IR_IS_LOCAL(). */
#define _Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS (entry_trampoline.words)




/* _PyEval_EvalFrameDefault is too large to optimize for speed with PGO on MSVC.
 */
#if (defined(_MSC_VER) && \
     (_MSC_VER < 1943) && \
     defined(_Py_USING_PGO))
#define DO_NOT_OPTIMIZE_INTERP_LOOP
#endif

#ifdef DO_NOT_OPTIMIZE_INTERP_LOOP
#  pragma optimize("t", off)
/* This setting is reversed below following _PyEval_EvalFrameDefault */
#endif


/* The iterator of iterable and its index: {iterable, tagged index 0} when
   the interpreter indexes it, {iterator, PyValue_NULL} otherwise, and
   {PyValue_NULL, PyValue_NULL} with an exception set.  Consumes iterable.
   Both come back in registers (spec 3.5, R-a); out of line, as when it was
   exported. */

PyValue
_PyEval_GetAIter(PyThreadState *tstate, PyValue obj);

int (_PyEval_StoreName)(PyThreadState *tstate, PyValueBorrow v, PyObject *name, PyObject* ns);

/* The wrappers below keep the address of a C local out of the interpreter
   handlers (spec 3.5, R-c): each runtime API with an out-parameter runs in
   its own C frame and returns its outputs by value. */


/* Out of line: _PyEval_LoadGlobalValue writes the value through a pointer.
   Returns PyValue_NULL, with an exception set, on error. */

/* The next item of iter: {1, item} (owned), {0, PyValue_NULL} at the end
   with no exception set, {-1, PyValue_NULL} with an exception set.  The
   index lives in the caller's frame slot, *index_ptr, which it updates
   (spec 3.5, R-a and R-d). */
static inline Py_ALWAYS_INLINE _PyForIterStep
_PyForIter_VirtualIteratorNextInline(PyThreadState *tstate,
                                     _PyInterpreterFrame *frame,
                                     PyValueBorrow iter, PyValue cursor);
int _PyForIter_StopIsExhaustion(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame);

#if (defined(__GNUC__) && __GNUC__ >= 10 && !defined(__clang__)) && defined(__x86_64__)
/*
 * gh-129987: The SLP autovectorizer can cause poor code generation for
 * opcode dispatch in some GCC versions (observed in GCCs 12 through 15,
 * probably caused by https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115777),
 * negating any benefit we get from vectorization elsewhere in the
 * interpreter loop. Disabling it significantly affected older GCC versions
 * (prior to GCC 9, 40% performance drop), so we have to selectively disable
 * it.
 */
#define DONT_SLP_VECTORIZE __attribute__((optimize ("no-tree-slp-vectorize", "no-omit-frame-pointer")))
#else
#define DONT_SLP_VECTORIZE
#endif

#ifdef WITH_DTRACE
static void
dtrace_function_entry(_PyInterpreterFrame *frame)
{
    const char *filename;
    const char *funcname;
    int lineno;

    PyCodeObject *code = _PyFrame_GetCode(frame);
    PyObject *exc = PyErr_GetRaisedException();
    filename = PyUnicode_AsUTF8(code->co_filename);
    funcname = PyUnicode_AsUTF8(code->co_name);
    PyErr_SetRaisedException(exc);
    if (filename == NULL) {
        filename = "<unknown>";
    }
    if (funcname == NULL) {
        funcname = "<unknown>";
    }
    lineno = PyUnstable_InterpreterFrame_GetLine(frame);

    PyDTrace_FUNCTION_ENTRY(filename, funcname, lineno);
}

void
_PyEval_dtrace_function_return(_PyInterpreterFrame *frame);
#endif

#ifndef NDEBUG
/* Whether the instrumentation of code, which a running frame of tstate
 * executes, is as the eval loop keeps it.  Setting events re-instruments
 * every code object with a frame on a thread's stack
 * (instrument_all_executing_code_objects), and a frame's RESUME
 * re-instruments its code when it starts, except while tracing in the
 * default build (_MAYBE_INSTRUMENT).  A walk that fails stops with the error
 * set; once Python code has handled the error, the frames the walk did not
 * reach run code older than the version that failed, until a later walk
 * completes. */
static int
instrumentation_is_current(PyThreadState *tstate, PyCodeObject *code)
{
    uintptr_t epoch = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) &
                      ~_PY_EVAL_EVENTS_MASK;
    uintptr_t version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(code->_co_instrumentation_version)
                        & ~_PyCode_OBSERVED_MARK;
    uintptr_t failed = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(
        tstate->interp->ceval.instrumentation_failed_version);
    if (version == epoch || version < failed || PyErr_Occurred()) {
        return 1;
    }
#ifdef Py_GIL_DISABLED
    return 0;
#else
    return tstate->tracing != 0;
#endif
}
#endif

Py_NO_INLINE int
_PyEval_value_operation_refresh_failed(PyThreadState *tstate,
                               _PyInterpreterFrame *frame, _PyInstr *current,
                               PyValue failed_result);

static inline Py_ALWAYS_INLINE int
value_operation_refresh(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *current, PyValue failed_result)
{
    if (tstate->current_frame != frame) {
        return _PyEval_value_operation_refresh_failed(tstate, frame, current,
                                              failed_result);
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || frame->ir_words != exec->words ||
        current < exec->words || current >= exec->words + exec->nwords) {
        return _PyEval_value_operation_refresh_failed(tstate, frame, current,
                                              failed_result);
    }
    assert(instrumentation_is_current(tstate, code));
    frame->instr_ptr = current;
    return 0;
}

/* Operand k of the call whose head is head, as an owner: a local is counted
 * and a temporary gives up its home.  The caller reads each operand it takes
 * once, before anything runs Python. */
static inline PyValue
call_operand_take(_PyInterpreterFrame *frame, const _PyIRWord *head, int k)
{
    PyValue *home = &_PyFrame_Slots(frame)[_PyIR_CallOperand(head, k)];
    if (head[1 + k / IR_OPERANDS_PER_WORD] &
        (IR_F_LOCAL_A << (k % IR_OPERANDS_PER_WORD))) {
        return PyValue_DUP(*home);
    }
    PyValue owner = *home;
    *home = PyValue_NULL;
    return owner;
}

/* The word after the call that starts at current: the pc to which its
 * callee returns.  A call is a head and its operand words. */
static inline _PyInstr *
call_return_pc(_PyInstr *current)
{
    return current + _PyIR_CallWordLength(FT_ATOMIC_LOAD_UINT64_RELAXED(*current));
}

/* The release of one temporary home of a call: the owner gives up its home,
 * and a count that is not the last closes at once, which runs nothing.
 * Returns 1 and leaves the owner in its home when it holds the last count,
 * which _PyEval_call_release_rest() closes.  A home may be NULL: a self operand's
 * is, and a callable that a Python callee took from its home. */
static inline Py_ALWAYS_INLINE int
call_release_home(PyValue *slots, uint32_t home)
{
    PyValue owner = slots[home];
    if (PyValue_RefcountOnObject(owner)) {
        if (Py_REFCNT(BITS_TO_PTR(owner)) <= 1) {
            return 1;
        }
        Py_DECREF_MORTAL(BITS_TO_PTR(owner));
    }
    slots[home] = PyValue_NULL;
    return 0;
}

/* The release pass over what a native call of at most IR_OPERANDS_PER_WORD
 * operands owes: its temporary operands, which are all in the first operand
 * word, and its callable, each taken from its home before it closes.  The
 * facts of the word say what is owed (IR_F_CALL_OWES_CALLABLE and
 * IR_F_CALL_OWES_OPERANDS), so a call that names only locals reads none of
 * them.  OPERANDS is the number of operands that name slots: all of the
 * word's, but the names of a CALL_KW.  A local is only borrowed, and so is
 * the home named DST, which the caller has written.
 *
 * An owner that holds the last count is left in its home, which keeps the
 * loop free of deallocations (and so of the registers that a call would cost
 * every handler); the others close at once, which runs nothing.  The return
 * value says whether a pass is still owed, and _PyEval_call_release_rest() makes it. */
static inline Py_ALWAYS_INLINE int
call_release_pass(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                  int operands, uint32_t dst)
{
    int owed = 0;
    if (word & IR_F_CALL_OWES_OPERANDS) {
        _PyIRWord ops = head[1];
        uint64_t homes = ops >> 16;
        for (int k = 0; k < operands; k++, homes >>= 12) {
            uint32_t home = (uint32_t)(homes & IR_FIELD_MAX);
            if (home != dst && !(ops & (IR_F_LOCAL_A << k))) {
                owed |= call_release_home(slots, home);
            }
        }
    }
    if (word & IR_F_CALL_OWES_CALLABLE) {
        owed |= call_release_home(slots, IR_B(word));
    }
    return owed;
}

/* The pass for any count of operands, and the one that call_release_pass()
 * owes.  It reads the fields and the slot-class flags of the words, not the
 * facts of the head, so a call that has not stored its result may name a dst
 * that nothing went to (UINT32_MAX).  It walks the operand words, four
 * operands to a word, and moves the owners that hold the last count to
 * OWNERS, which then close in reverse order, so that all of them have left
 * the frame before the first deallocation can run Python (handlers81
 * rule 5). */
static inline Py_ALWAYS_INLINE void
call_release_owners(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                    int operands, uint32_t dst, PyValue *owners)
{
    int last = 0;
#define CALL_DETACH(HOME) \
    do { \
        uint32_t home_ = (HOME); \
        PyValue owner_ = slots[home_]; \
        slots[home_] = PyValue_NULL; \
        if (PyValue_RefcountOnObject(owner_)) { \
            if (Py_REFCNT(BITS_TO_PTR(owner_)) <= 1) { \
                owners[last++] = owner_; \
            } \
            else { \
                Py_DECREF_MORTAL(BITS_TO_PTR(owner_)); \
            } \
        } \
    } while (0)
    for (int w = 0, left = operands; left > 0;
         w++, left -= IR_OPERANDS_PER_WORD)
    {
        _PyIRWord ops = head[1 + w];
        uint64_t homes = ops >> 16;
        int count = left < IR_OPERANDS_PER_WORD ? left : IR_OPERANDS_PER_WORD;
        for (int k = 0; k < count; k++, homes >>= 12) {
            uint32_t home = (uint32_t)(homes & IR_FIELD_MAX);
            if (home != dst && !(ops & (IR_F_LOCAL_A << k))) {
                CALL_DETACH(home);
            }
        }
    }
    uint32_t callable = IR_B(word);
    if (callable != dst && !(word & IR_F_LOCAL_B)) {
        CALL_DETACH(callable);
    }
#undef CALL_DETACH
    while (last > 0) {
        PyValue_XCLOSE(owners[--last]);
    }
}



Py_NO_INLINE void
_PyEval_call_release_rest(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                  int operands, uint32_t dst);

/* Release the temporary operands and the callable of a call: its owners that
 * hold other counts at once, and the rest out of line.  A longer call than
 * one operand word holds is all out of line. */
static inline Py_ALWAYS_INLINE void
call_release_operands(PyValue *slots, const _PyIRWord *head, _PyIRWord word,
                      int operands, uint32_t dst)
{
    if (Py_UNLIKELY(((word & IR_F_CALL_OWES_OPERANDS) &&
                     operands > IR_OPERANDS_PER_WORD) ||
                    call_release_pass(slots, head, word, operands, dst))) {
        _PyEval_call_release_rest(slots, head, word, operands, dst);
    }
}

/* The result of a native call, which the handler computed from the operand
 * homes themselves: store it in dst, then release the call's temporaries
 * (call_release_operands).  The result goes first (handlers81 rules 6 and 7),
 * so a dst that is one of the temporary homes, or a local, displaces its old
 * value, which the caller closes last (ir_old). */
static inline Py_ALWAYS_INLINE PyValue
call_publish(_PyInterpreterFrame *frame, const _PyIRWord *head, _PyIRWord word,
             PyValue result, int operands)
{
    PyValue *slots = _PyFrame_Slots(frame);
    uint32_t dst = IR_A(word);
    PyValue displaced = slots[dst];
    _PyValue_CheckHome(result);
    slots[dst] = result;
    call_release_operands(slots, head, word, operands, dst);
    return displaced;
}


/* Copy N operands of the call at HEAD, from operand FIRST on, to ARGS.  They
 * are all in the first operand word (FIRST + N is at most
 * IR_OPERANDS_PER_WORD), which is read once. */
#define CALL_NATIVE_GATHER(HEAD, SLOTS, FIRST, N, ARGS) \
    do { \
        uint64_t fields_ = (HEAD)[1] >> (16 + 12 * (FIRST)); \
        for (int i_ = 0; i_ < (N); i_++) { \
            (ARGS)[i_] = (SLOTS)[fields_ & IR_FIELD_MAX]; \
            fields_ >>= 12; \
        } \
    } while (0)

/* The same for any count, across the operand words after head: from operand
 * FIRST on, N operands, four to a word. */
static inline Py_ALWAYS_INLINE void
call_gather_words(PyValue *slots, const _PyIRWord *head, int first, int n,
                  PyValue *args)
{
    int w = first / IR_OPERANDS_PER_WORD;
    int s = first % IR_OPERANDS_PER_WORD;
    for (; n > 0; w++, s = 0) {
        int count = IR_OPERANDS_PER_WORD - s;
        if (count > n) {
            count = n;
        }
        CALL_NATIVE_GATHER(&head[w], slots, s, count, args);
        args += count;
        n -= count;
    }
}

/* The native vectorcall of a call, on its operands as they stand in their
 * homes: they are borrowed for the call, as a temporary cannot change and a
 * local's old value survives a write through its frame's proxy.  The caller
 * publishes the result, which releases the temporaries.  The arguments follow
 * the self slot, which is NULL at run time for a class attribute that is not
 * a method.  receiver, if it is not NULL, goes before them: the self of a
 * bound method that the call expanded.  keywords: the last operand is the
 * index of the keyword names in co_consts. */
static inline Py_ALWAYS_INLINE PyValue
call_vector_impl(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 const _PyIRWord *head, PyValue callable, PyValue receiver,
                 int keywords)
{
    _PyIRWord word = *head;
    PyValue *slots = _PyFrame_Slots(frame);
    int operands = (int)IR_C(word) - keywords;
    int first = (word & IR_F_NO_SELF) == 0 &&
        PyValue_IsNull(slots[_PyIR_CallOperand(head, 0)]);
    int lead = !PyValue_IsNull(receiver);
    int nargs = lead + operands - first;
    PyValue small[16];
    PyValue *args = small;
    if (nargs > (int)Py_ARRAY_LENGTH(small)) {
        args = PyMem_Malloc((size_t)nargs * sizeof(PyValue));
        if (args == NULL) {
            PyErr_NoMemory();
            return PyValue_NULL;
        }
    }
    if (lead) {
        args[0] = receiver;
    }
    if (operands <= IR_OPERANDS_PER_WORD) {
        CALL_NATIVE_GATHER(head, slots, first, operands - first, args + lead);
    }
    else {
        call_gather_words(slots, head, first, operands - first, args + lead);
    }
    PyObject *names = NULL;
    size_t positional = (size_t)nargs;
    if (keywords) {
        names = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(
            _PyFrame_GetCode(frame)->co_consts,
            _PyIR_CallOperand(head, (int)IR_C(word) - 1)));
        positional -= (size_t)PyTuple_GET_SIZE(names);
    }
    PyValue result = _PyValue_VectorcallTstate(tstate, callable, args,
                                               positional, names);
    if (args != small) {
        PyMem_Free(args);
    }
    return result;
}

Py_NO_INLINE PyValue
_PyEval_call_vector(PyThreadState *tstate, _PyInterpreterFrame *frame,
            const _PyIRWord *head, PyValue callable);

/* The native call of a specialized handler: the callee reads operands first
 * to the end of the call's operand list, copied to a C array that the callee
 * borrows for the call.  The copy is made before the callee runs, so the
 * callee sees the operands as they stood at entry (the old value of a local
 * that the callee rebinds through its frame's proxy stays alive in
 * f_overwritten_fast_locals, and a temporary cannot change).  The handler
 * publishes the result and releases the temporaries, as after _PyEval_call_vector().
 *
 * A callee written against the value API (METH_VALUE) is called through its
 * value entry; an object-API callee through the object vectorcall of the
 * function or descriptor, which boxes the inline words, as the dispatcher of
 * _PyValue_VectorcallTstate() does.  The handler's guard checked the call
 * flags, so the bridge is one test of METH_VALUE.  The recursion limit is
 * checked by the guard (_CHECK_RECURSION_LIMIT), which deopts to the generic
 * path that raises.
 *
 * More than IR_OPERANDS_PER_WORD operands are gathered by _PyEval_call_vector(). */

/* A builtin function.  Its handler's guard checked the call flags and the
 * number of arguments of a fixed-arity value form (METH_VALUE | METH_O or
 * METH_NOARGS). */
static inline Py_ALWAYS_INLINE PyValue
call_builtin_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    const _PyIRWord *head, _PyIRWord word,
                    PyValue callable, int first)
{
    PyCFunctionObject *func =
        (PyCFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
    int flags = func->m_ml->ml_flags;
    int n = (int)IR_C(word) - first;
    assert(!(flags & (METH_O | METH_NOARGS)) || n == ((flags & METH_O) ? 1 : 0));
    if (Py_UNLIKELY(first + n > IR_OPERANDS_PER_WORD)) {
        return _PyEval_call_vector(tstate, frame, head, callable);
    }
    PyValue args[IR_OPERANDS_PER_WORD];
    CALL_NATIVE_GATHER(head, _PyFrame_Slots(frame), first, n, args);
    if (flags & METH_VALUE) {
        _PyCFunctionValue meth = (_PyCFunctionValue)(void(*)(void))
                                 func->m_ml->ml_meth;
        PyValue result = meth(_PyCFunction_GET_SELF_VALUE((PyObject *)func),
                              args, n, NULL);
        _PyValue_ASSERT_RESULT(tstate, result);
        return result;
    }
    return _PyValue_CallAdapter(
        tstate, (PyObject *)func, func->vectorcall, args, (size_t)n, NULL);
}

/* A builtin class that has a vectorcall (its guard checked): the class's value
 * vectorcall, else its object vectorcall through the adapter. */
static inline Py_ALWAYS_INLINE PyValue
call_class_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  const _PyIRWord *head, _PyIRWord word, PyValue callable,
                  int first)
{
    int n = (int)IR_C(word) - first;
    if (Py_UNLIKELY(first + n > IR_OPERANDS_PER_WORD)) {
        return _PyEval_call_vector(tstate, frame, head, callable);
    }
    PyValue args[IR_OPERANDS_PER_WORD];
    CALL_NATIVE_GATHER(head, _PyFrame_Slots(frame), first, n, args);
    PyTypeObject *cls = (PyTypeObject *)PyValue_AsKnownObjectBorrow(callable);
    _Py_vectorcallvaluefunc value_call;
    memcpy(&value_call, &cls->tp_value_vectorcall, sizeof(value_call));
    if (value_call != NULL) {
        PyValue result = value_call(tstate, (PyObject *)cls, args, (size_t)n,
                                    NULL);
        _PyValue_ASSERT_RESULT(tstate, result);
        return result;
    }
    return _PyValue_CallAdapter(tstate, (PyObject *)cls, cls->tp_vectorcall,
                                args, (size_t)n, NULL);
}

/* A method descriptor: the receiver is operand first, and the callee takes it
 * with the arguments.  Its handler's guard checked the call flags, the number
 * of arguments and that the receiver is an exact instance of the
 * descriptor's type. */
static inline Py_ALWAYS_INLINE PyValue
call_descriptor_values(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       const _PyIRWord *head, _PyIRWord word,
                       PyValue callable, int first)
{
    PyMethodDescrObject *descr =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(callable);
    int flags = descr->d_method->ml_flags;
    int n = (int)IR_C(word) - first;
    assert(n > 0);
    assert(!(flags & (METH_O | METH_NOARGS)) || n == ((flags & METH_O) ? 2 : 1));
    PyValue *slots = _PyFrame_Slots(frame);
    assert(PyValue_TYPE(slots[_PyIR_CallOperand(head, first)]) ==
           descr->d_common.d_type);
    if (Py_UNLIKELY(first + n > IR_OPERANDS_PER_WORD)) {
        return _PyEval_call_vector(tstate, frame, head, callable);
    }
    PyValue args[IR_OPERANDS_PER_WORD];
    CALL_NATIVE_GATHER(head, slots, first, n, args);
    if (flags & METH_VALUE) {
        _PyCFunctionValue meth = (_PyCFunctionValue)(void(*)(void))
                                 descr->d_method->ml_meth;
        PyValue result = meth(PyValue_Borrow(args[0]), args + 1, n - 1, NULL);
        _PyValue_ASSERT_RESULT(tstate, result);
        return result;
    }
    return _PyValue_CallAdapter(
        tstate, (PyObject *)descr, descr->vectorcall, args, (size_t)n, NULL);
}

/* Move or count the first count (at most four) operands of fields, the twelve
 * bit slot fields of an operand word, and locals, its local bits, into dest:
 * a local is counted, a temporary moves out of its home. */
static inline Py_ALWAYS_INLINE void
call_take_fields(PyValue *slots, uint64_t fields, unsigned int locals,
                 int count, PyValue *dest)
{
#define CALL_TAKE(K) \
    if (count > (K)) { \
        PyValue *home = &slots[(fields >> (12 * (K))) & IR_FIELD_MAX]; \
        PyValue value = *home; \
        if (locals & (1u << (K))) { \
            value = PyValue_DUP(value); \
        } \
        else { \
            *home = PyValue_NULL; \
        } \
        dest[K] = value; \
    }
    CALL_TAKE(0)
    CALL_TAKE(1)
    CALL_TAKE(2)
    CALL_TAKE(3)
#undef CALL_TAKE
}

Py_NO_INLINE void
_PyEval_call_take_loop(PyValue *slots, const _PyIRWord *head, int first, int count,
               PyValue *dest);

static inline Py_ALWAYS_INLINE void
call_take_operands(PyValue *slots, const _PyIRWord *head, int first, int count,
                   PyValue *dest)
{
    if (!Py_UNLIKELY(first + count > IR_OPERANDS_PER_WORD)) {
        _PyIRWord ops = head[1];
        call_take_fields(slots, ops >> (16 + 12 * first),
                         (unsigned int)(ops >> (9 + first)), count, dest);
        return;
    }
    _PyEval_call_take_loop(slots, head, first, count, dest);
}

/* Release the callable operand of a call, a temporary that the call owns. */
static inline void
call_release_callable(_PyInterpreterFrame *frame, _PyIRWord word)
{
    if (!(word & IR_F_LOCAL_B)) {
        PyValue *home = &_PyFrame_Slots(frame)[IR_B(word)];
        PyValue owner = *home;
        *home = PyValue_NULL;
        PyValue_XCLOSE(owner);
    }
}

/* The plain return, certified RETURN_VALUE_EMPTY: retire a thread frame whose
 * scratch is empty, so that only the code's locals can own values, as
 * upstream does.  The frame is already unlinked, the caller holds the result
 * in C, and each owner is NULLed and closed in turn: a finalizer that runs
 * sees a consistent frame, and nested calls may grow the arena but must
 * restore this reservation, which stays until the code is closed.  The
 * common frame is the one routine here: the clear, the function and the code,
 * and the pop.  A frame that has a frame object, or that a sampling profiler
 * anchors, takes the general retire out of line: the frame object takes the
 * locals. */
static inline Py_ALWAYS_INLINE PyValue
_PyEval_RetireThreadFrameWithResult(PyThreadState *tstate,
                                    _PyInterpreterFrame *frame, PyValue result)
{
    assert(!PyValue_IsNull(result));
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    assert(tstate->current_frame != frame);
    assert((PyObject **)frame == tstate->datastack_limit);
    if (Py_UNLIKELY(frame->frame_obj != NULL
                    || tstate->last_profiled_frame == frame)) {
        return _PyEval_FrameClearAndPopGeneralWithResult(tstate, frame, result);
    }
    _PyFrame_ClearSlots(frame, (int)_PyFrame_Exec(frame)->nfixed);
    // The frame holds both, and is dead: nothing reads the pointers again
    assert(frame->f_funcobj != NULL);
    Py_DECREF(frame->f_funcobj);
    Py_DECREF(frame->f_executable);
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}

/* The function of the call at head as an owner for its callee: a temporary
 * moves out of its home and a local is counted.  expanded: callable is a
 * bound method, whose function is counted; the method stays in its home. */
static inline Py_ALWAYS_INLINE PyValue
call_exact_function(PyValue *slots, _PyIRWord word, PyValue callable,
                    int expanded)
{
    if (expanded) {
        PyMethodObject *method =
            (PyMethodObject *)PyValue_AsKnownObjectBorrow(callable);
        return PyValue_DUP(PyValue_FromObjectBorrow(method->im_func));
    }
    PyValue *home = &slots[IR_B(word)];
    PyValue function = *home;
    if (Py_UNLIKELY(word & IR_F_LOCAL_B)) {
        return PyValue_DUP(function);
    }
    *home = PyValue_NULL;
    return function;
}

/* Move the operands of the call at head, whose callee takes exactly these
 * arguments, to arguments, the first slots of its frame: a temporary moves
 * and a local is counted.  expanded: callable is a bound method, whose
 * receiver comes first, counted, and the method operand is released. */
static inline Py_ALWAYS_INLINE void
call_fill_exact(PyValue *slots, _PyInterpreterFrame *frame,
                const _PyIRWord *head, _PyIRWord word, PyValue callable,
                int call_state, PyValue *arguments)
{
    int expanded = (call_state & CALL_EXPANDED) != 0;
    if (expanded) {
        PyMethodObject *method =
            (PyMethodObject *)PyValue_AsKnownObjectBorrow(callable);
        *arguments++ = PyValue_DUP(method->im_self);
    }
    int n = (int)IR_C(word);
    // The self operand is NULL at run time for a class attribute that is
    // not a method, and for a bound-method operand: the arguments follow
    // its slot
    int skip = (word & IR_F_NO_SELF) == 0 &&
        (expanded || !(call_state & CALL_SELF_PRESENT));
    if (Py_UNLIKELY(n > IR_OPERANDS_PER_WORD)) {
        _PyEval_call_take_loop(slots, head, skip, n - skip, arguments);
    }
    else {
        // All the operands are in the first operand word; a skipped self
        // slot is its first field
        _PyIRWord ops = head[1];
        uint64_t fields = ops >> 16;
        unsigned int locals = (unsigned int)(ops >> 9) & 15;
        if (Py_UNLIKELY(skip)) {
            fields >>= 12;
            locals >>= 1;
            n--;
        }
        call_take_fields(slots, fields, locals, n, arguments);
    }
    if (expanded) {
        call_release_callable(frame, word);
    }
}

/* Push the frame of the Python function callable for the call at head, whose
 * operands fill its first arguments; with CALL_EXPANDED, of the function of
 * the bound method callable, with its receiver first.  The guards checked the
 * stack space and that the callee takes exactly these arguments
 * (call_fill_exact()). */
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
call_push_exact(PyThreadState *tstate, _PyInterpreterFrame *frame,
                const _PyIRWord *head, _PyIRWord word, PyValue callable,
                PyValue self, int call_state)
{
    int expanded = (call_state & CALL_EXPANDED) != 0;
    // The self operand moves from its home with the arguments; a bound
    // method's is NULL
    assert(!expanded || PyValue_IsNull(self));
    (void)self;
    PyValue *slots = _PyFrame_Slots(frame);
    PyValue function = call_exact_function(slots, word, callable, expanded);
    _PyInterpreterFrame *pushed = _PyFrame_PushUnchecked(tstate, function, frame);
    call_fill_exact(slots, frame, head, word, callable, call_state, pushed->base);
    return pushed;
}

/* The C rows of the Call and Return families (Python/register_ops/calls.rop).
 * An Args operand is the instruction's head, whose word and operand words
 * give the count, the self role and the homes; a Src operand is its home. */

static inline bool
_PyRop_DefaultEvalFrame(PyThreadState *tstate)
{
    return !IS_PEP523_HOOKED(tstate);
}

static inline bool
_PyRop_FunctionVersion(PyValueBorrow fn, const _PyFbCall *fb)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(fn);
    return PyFunction_Check(object) &&
        ((PyFunctionObject *)object)->func_version == fb->func_version;
}

/* The call_state of the call at head: whether its self is present at run
 * time (IR_CALL_SELF_STATE) */
static inline int
rop_call_self_state(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    if (!_PyIR_CallHasSelf(head[0])) {
        return 0;
    }
    PyValue self = _PyFrame_Slots(frame)[_PyIR_CallOperand(head, 0)];
    return PyValue_IsNull(self) ? 0 : CALL_SELF_PRESENT;
}

/* The number of arguments the callee receives: the operands, less a self
 * that is NULL at run time */
static inline int
rop_call_argcount(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    _PyIRWord word = head[0];
    int n = (int)IR_C(word);
    if (!(word & IR_F_NO_SELF) && !rop_call_self_state(frame, head)) {
        n--;
    }
    return n;
}

static inline bool
_PyRop_ExactPositional(_PyInterpreterFrame *frame, PyValueBorrow fn,
                       const _PyIRWord *head)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(fn);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    return code->co_argcount == rop_call_argcount(frame, head);
}

static inline bool
_PyRop_FrameSpace(PyThreadState *tstate, PyValueBorrow fn)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(fn);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    return _PyThreadState_HasStackSpace(tstate, code->co_framesize);
}

static inline bool
_PyRop_RecursionBudget(PyThreadState *tstate)
{
    return tstate->py_recursion_remaining > 1;
}

static inline _PyInterpreterFrame *
_PyRop_PushExact(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 PyValue *fn, const _PyIRWord *head)
{
    _PyIRWord word = head[0];
    assert(fn == &_PyFrame_Slots(frame)[IR_B(word)]);
    return call_push_exact(tstate, frame, head, word, *fn, PyValue_NULL,
                           rop_call_self_state(frame, head));
}

static inline bool
_PyRop_SelfAbsent(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    return rop_call_self_state(frame, head) == 0;
}

static inline bool
_PyRop_MethodVersion(PyValueBorrow fn, const _PyFbCall *fb)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(fn);
    if (Py_TYPE(object) != &PyMethod_Type) {
        return false;
    }
    PyObject *func = ((PyMethodObject *)object)->im_func;
    return PyFunction_Check(func) &&
        ((PyFunctionObject *)func)->func_version == fb->func_version;
}

static inline bool
_PyRop_NoWeakrefs(PyValueBorrow fn)
{
    return ((PyMethodObject *)PyValue_AsKnownObjectBorrow(fn))->im_weakreflist == NULL;
}

/* The code of the function of a bound method that method_version admitted */
static inline PyCodeObject *
rop_method_code(PyValueBorrow fn)
{
    PyMethodObject *method = (PyMethodObject *)PyValue_AsKnownObjectBorrow(fn);
    return (PyCodeObject *)((PyFunctionObject *)method->im_func)->func_code;
}

static inline bool
_PyRop_ExactPositionalBound(_PyInterpreterFrame *frame, PyValueBorrow fn,
                            const _PyIRWord *head)
{
    return rop_method_code(fn)->co_argcount == rop_call_argcount(frame, head) + 1;
}

static inline bool
_PyRop_FrameSpaceBound(PyThreadState *tstate, PyValueBorrow fn)
{
    return _PyThreadState_HasStackSpace(tstate, rop_method_code(fn)->co_framesize);
}

static inline _PyInterpreterFrame *
_PyRop_PushBoundExact(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      PyValue *fn, const _PyIRWord *head)
{
    _PyIRWord word = head[0];
    assert(fn == &_PyFrame_Slots(frame)[IR_B(word)]);
    return call_push_exact(tstate, frame, head, word, *fn, PyValue_NULL,
                           CALL_EXPANDED);
}

static inline bool
_PyRop_ClassVersion(PyValueBorrow fn, const _PyFbCall *fb)
{
    if (!_PyRop_IsObject(fn) || fb->type_version == 0) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(fn);
    return PyType_Check(object) &&
        FT_ATOMIC_LOAD_UINT32_RELAXED(((PyTypeObject *)object)->tp_version_tag) ==
            fb->type_version;
}

static inline bool
_PyRop_InitFrameReady(PyThreadState *tstate, PyValueBorrow fn)
{
    PyHeapTypeObject *cls = (PyHeapTypeObject *)PyValue_AsKnownObjectBorrow(fn);
    assert(cls->ht_type.tp_new == PyBaseObject_Type.tp_new);
    PyFunctionObject *init =
        (PyFunctionObject *)FT_ATOMIC_LOAD_PTR_ACQUIRE(cls->_spec_cache.init);
    if (init->vectorcall != _PyFunction_Vectorcall) {
        return false;
    }
    PyCodeObject *code = (PyCodeObject *)init->func_code;
    return _PyThreadState_HasStackSpace(
        tstate, code->co_framesize + _Py_InitCleanup.co_framesize);
}

Py_NO_INLINE void
_PyEval_rop_call_retire_operands(_PyInterpreterFrame *frame, const _PyIRWord *head);

Py_NO_INLINE _PyInterpreterFrame *
_PyRop_PushInit(PyThreadState *tstate, _PyInterpreterFrame *frame, PyValue *fn,
                _PyInstr *head);

static inline void
_PyRop_RetireFailedArgs(_PyInterpreterFrame *frame, PyValue *fn,
                        const _PyIRWord *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    (void)fn;
    _PyEval_rop_call_retire_operands(frame, head);
}

Py_NO_INLINE void
_PyRop_SpendArgs(_PyInterpreterFrame *frame, const _PyIRWord *head,
                 const PyValue *kept);

/* The Interrupt operand of a call word: whether the call polls when it
 * returns (INTERRUPTIBLE is 0) */
static inline bool
_PyRop_Polls(uint32_t interrupt)
{
    return interrupt == 0;
}

/* The two evaluators (Python/ceval_evaluator.h) and their switch.  A frame
 * moves to the other evaluator at the switch word (_PyEval_SwitchWord,
 * Include/internal/pycore_ceval.h).  The redirections happen where a frame
 * starts or resumes (RESUME), at a poll after a monitoring change, and for
 * the frames already on a stack, during the stop-the-world change
 * (_PyEval_RedirectFrames()).  Each evaluator's entry
 * (_PyEval_EnterNormal(), _PyEval_EnterObserving()) is declared with the
 * interpreter's functions (Python/ceval_tail.h). */

/* Whether frame, which runs in the evaluator that observing names, goes on
 * in the other one: the one that its code's observed mark asks for.  A
 * generator frame does not leave the observing evaluator, since its return
 * goes on in its consumer through a completion (gen_return) in the evaluator
 * that it ran in, and the consumer may be observed. */
static inline bool
_PyEval_SwitchesLoop(_PyInterpreterFrame *frame, int observing)
{
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return false;
    }
    if (_PyFrame_Exec(frame) == &_PyIR_InitCleanupExec) {
        /* The cleanup shim of __init__ has no RESUME of its own and returns
         * to the frame that called the class: it goes on in that frame's
         * evaluator */
        frame = frame->previous;
    }
    if (_PyCode_IsObserved(_PyFrame_GetCode(frame))) {
        return !observing;
    }
    return observing && frame->owner != FRAME_OWNED_BY_GENERATOR;
}

/* A frame whose code has live entries goes on in the code's unit when its
 * child returns, from a unit or the evaluator (IR_JIT_SWITCH()), and only
 * code that is not observed has live entries: monitoring retires the unit
 * of code that it observes, which unpublishes them (_PyJit_Retire()), before
 * the walk.  So the frame needs no switch.  Any other frame may still be
 * resumed by a unit's transfer, which hands the switch word to an
 * evaluator. */
static inline bool
_PyEval_SwitchSkips(_PyInterpreterFrame *frame)
{
#ifdef _PY_JIT_SUPPORTED
    return frame->owner != FRAME_OWNED_BY_INTERPRETER &&
           _PyFrame_JitEntries(frame) != NULL;
#else
    (void)frame;
    return false;
#endif
}

/* Make frame go on at real through the switch word: push the switch and
 * redirect return_pc.  Returns 1, the status of a redirect. */
int _PyEval_SwitchAt(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *real);

/* The word that frame goes on at, whose resume point the switch word
 * stands in for: the switch's newest entry for frame, which this removes.
 * An entry frame's switch passes the switch on to the frame that called the
 * activation, at its next poll. */
_PyInstr *_PyEval_SwitchLoop(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Drop the switch of frame, an error unwinds into it */
void _PyEval_SwitchDrop(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* The pending events of a poll, with the loop switch first: the poll's word
 * runs again at the switch word, real, and the other events wait for it */
static Py_NO_INLINE _Py_COLD_FUNCTION int
_PyRop_HandlePoll(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  _PyInstr *real)
{
    if (_Py_eval_breaker_bit_is_set(tstate, _PY_EVAL_LOOP_SWITCH_BIT)) {
        _Py_unset_eval_breaker_bit(tstate, _PY_EVAL_LOOP_SWITCH_BIT);
        if (!_PyEval_SwitchSkips(frame)) {
            return _PyEval_SwitchAt(tstate, frame, real);
        }
    }
    return _Py_HandlePending(tstate);
}

/* The poll after a call: as _PyRop_Poll(), where a loop switch goes on at
 * the next word, next */
static inline int
_PyRop_EndPoll(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _PyInstr *next)
{
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (Py_UNLIKELY(_Py_EvalBreakerHasEvents(tstate))) {
        return _PyRop_HandlePoll(tstate, frame, next);
    }
    return 0;
}

/* The rows of AttrWrite, SuperAttr, SuperMethod and SpecialLookup
 * (Python/register_ops/attrs.rop).  Every source is borrowed. */

Py_NO_INLINE int
_PyRop_SetAttr(PyValueBorrow owner, PyObject *name, PyValueBorrow value);

Py_NO_INLINE int
_PyRop_DelAttr(PyValueBorrow owner, PyObject *name);

Py_NO_INLINE PyValue
_PyRop_SuperCall(PyValueBorrow sup, PyValueBorrow cls, PyValueBorrow self,
                 bool explicit);

void _PyEval_rop_take_items(const _PyRopSpan *span, PyValue *vals);

/* The argument that the CALL event reports besides `super`: the class, or
 * MISSING when the class was implied */
static inline PyValueBorrow
_PyRop_SuperEventArg(PyValueBorrow cls, bool explicit)
{
    return explicit ? cls
                    : PyValue_FromObjectBorrow(&_PyInstrumentation_MISSING);
}

/* The attribute of super(...): the span holds super, the class and the
 * receiver, which the row owns and closes on both outcomes.  With observed
 * set, the CALL event runs before the call and C_RETURN or C_RAISE after it,
 * as INSTRUMENTED_LOAD_SUPER_ATTR did; a callback that raises in C_RETURN
 * closes the super object. */
static inline PyValue
rop_super_lookup(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site, const _PyRopSpan *parts, bool explicit,
                 PyObject *name, bool observed)
{
    PyValue vals[3];
    _PyEval_rop_take_items(parts, vals);
    PyValueBorrow sup = PyValue_Borrow(vals[0]);
    PyValueBorrow cls = PyValue_Borrow(vals[1]);
    PyValueBorrow self = PyValue_Borrow(vals[2]);
    PyValue attr = PyValue_NULL;
    PyValueBorrow arg = _PyRop_SuperEventArg(cls, explicit);
    if (observed &&
        (_Py_call_instrumentation_2args)(tstate, PY_MONITORING_EVENT_CALL,
                                         frame, site, sup, arg) < 0) {
        goto done;
    }
    PyValue super = _PyRop_SuperCall(sup, cls, self, explicit);
    if (PyValue_IsNull(super)) {
        if (observed) {
            (_Py_call_instrumentation_exc2)(
                tstate, PY_MONITORING_EVENT_C_RAISE, frame, site, sup, arg);
        }
        goto done;
    }
    if (observed &&
        (_Py_call_instrumentation_2args)(tstate, PY_MONITORING_EVENT_C_RETURN,
                                         frame, site, sup, arg) < 0) {
        PyValue_CLEAR(super);
        goto done;
    }
    attr = (_PyValue_GetAttr)(PyValue_Borrow(super), name);
    PyValue_CLOSE(super);
done:
    {
        PyObject *exc = PyValue_IsNull(attr) ? PyErr_GetRaisedException()
                                             : NULL;
        for (int i = 0; i < 3; i++) {
            PyValue_XCLOSE(vals[i]);
        }
        if (exc != NULL) {
            PyErr_SetRaisedException(exc);
        }
    }
    return attr;
}

Py_NO_INLINE PyValue
_PyRop_SuperLookup(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                   const _PyRopSpan *parts, PyObject *name);

Py_NO_INLINE PyValue
_PyRop_SuperLookupObserved(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                           const _PyRopSpan *parts, PyObject *name);

Py_NO_INLINE PyValue
_PyRop_SuperLookupExplicit(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                           const _PyRopSpan *parts, PyObject *name);

Py_NO_INLINE PyValue
_PyRop_SuperLookupExplicitObserved(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
                                   const _PyRopSpan *parts, PyObject *name);

/* The part i of a super span, borrowed from its home */
static inline PyValueBorrow
rop_super_part(const _PyRopSpan *parts, int i)
{
    int slot = _PyRopSpan_Slot(parts, i);
    return PyValue_Borrow(parts->frame->base[slot]);
}

/* `super` is the builtin and the class is a type: the lookup needs no super
 * object */
static inline bool
_PyRop_SuperBuiltinReady(const _PyRopSpan *parts)
{
    PyValueBorrow sup = rop_super_part(parts, 0);
    PyValueBorrow cls = rop_super_part(parts, 1);
    return _PyRop_IsObject(sup) && _PyRop_IsObject(cls)
        && PyValue_AsKnownObjectBorrow(sup) == (PyObject *)&PySuper_Type
        && PyType_Check(PyValue_AsKnownObjectBorrow(cls));
}

/* The case of the adaptation row: 0 when the site can use the builtin
 * lookup, 1 when it cannot */
static inline int
_PyRop_ClassifySuper(const _PyRopSpan *parts, _PyInstr *site,
                     const _PyFbUnary *fb)
{
    (void)site;
    (void)fb;
    return _PyRop_SuperBuiltinReady(parts) ? 0 : 1;
}

Py_NO_INLINE PyValue
_PyRop_SuperLookupBuiltin(const _PyRopSpan *parts, PyObject *name);

Py_NO_INLINE PyValue
_PyRop_SuperLookupBuiltinMethod(const _PyRopSpan *parts, PyObject *name,
                                PyValue *self);

Py_NO_INLINE PyValue
_PyRop_LookupSpecial(PyValueBorrow obj, int which, PyValue *self);

Py_NO_INLINE void
_PyRop_InitResultError(PyValueBorrow value);

static inline bool _PyRop_PythonTarget(PyThreadState *tstate,
                                       _PyInterpreterFrame *frame,
                                       PyValue *fn, const _PyIRWord *head);
_PyInterpreterFrame *_PyRop_PushPython(PyThreadState *tstate,
                                              _PyInterpreterFrame *frame,
                                              PyValue *fn, _PyInstr *head);
/* The rows of Raise, Reraise, ExceptEnter, ExceptLeave, ExceptMatch and
 * ExceptGroupMatch (Python/register_ops/exceptions.rop) */
int _PyRop_RaiseArgs(PyThreadState *tstate, const _PyRopSpan *args);
void _PyRop_ReraisePending(PyThreadState *tstate,
                                  _PyInterpreterFrame *frame, _PyInstr *at);
void _PyRop_RestoreException(PyThreadState *tstate,
                                    _PyInterpreterFrame *frame, _PyInstr *at,
                                    PyValue *home);
void _PyRop_SetOrigin(_PyInterpreterFrame *frame, PyValue *home);
void _PyRop_RetireFailedOperand(_PyInterpreterFrame *frame,
                                       PyValue *home);
PyValue _PyRop_SwapHandled(PyThreadState *tstate, PyValue *home);
void _PyRop_RestoreHandled(PyThreadState *tstate, PyValue x);
int _PyRop_CheckExceptType(PyThreadState *tstate, PyValue *home);
bool _PyRop_ExceptionMatches(PyValueBorrow exc, PyValueBorrow pattern);
/* The rows of CleanupThrow, EndAsyncFor and WithExit
 * (Python/register_ops/cleanup.rop) */
bool _PyRop_IsStopIteration(PyValueBorrow exc);
PyValue _PyRop_StopIterationValue(PyValueBorrow exc);
bool _PyRop_IsStopAsyncIteration(PyValueBorrow exc);
int _PyRop_EndAsyncForEvent(PyThreadState *tstate,
                                   _PyInterpreterFrame *frame, _PyInstr *site);
PyValue _PyRop_WithExitCall(PyValueBorrow fn, PyValueBorrow self,
                                   PyValueBorrow exc);
PyValue _PyRop_WithExitBound(PyValueBorrow fn, PyValueBorrow exc);
int _PyRop_CheckExceptStarType(PyThreadState *tstate, PyValue *home);
PyValue _PyRop_GroupMatch(PyThreadState *tstate,
                                 _PyInterpreterFrame *frame, PyValue *exc,
                                 PyValue *pattern, PyValue *rest);
PyValue _PyRop_UnaryOp(uint32_t op, PyValue x);
/* The rows of Function and Intrinsic (Python/register_ops/functions.rop) */
PyValue _PyRop_FunctionMake(_PyInterpreterFrame *frame, PyValue code,
                                   const _PyRopSpan *fields, uint32_t present);
PyValue _PyRop_IntrinsicUnary(PyThreadState *tstate, uint32_t id,
                                     PyValue x);
PyValue _PyRop_IntrinsicImportStar(PyThreadState *tstate, PyValue x);
PyValue _PyRop_IntrinsicStopIteration(PyThreadState *tstate, PyValue x);
PyValue _PyRop_IntrinsicBinary(PyThreadState *tstate, uint32_t id,
                                      PyValue x, PyValue y);
PyValue _PyRop_IntrinsicAttr(PyThreadState *tstate, uint32_t id,
                                    PyValue x, PyValue y);

static inline PyValue _PyRop_CallNative(PyThreadState *tstate,
                                        _PyInterpreterFrame *frame, PyValue *fn,
                                        const _PyIRWord *head);
int _PyRop_CallEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            _PyInstr *site, PyValue *fn,
                            const _PyIRWord *head);

PyValue _PyRop_StarTuple(PyThreadState *tstate,
                                _PyInterpreterFrame *frame, PyValue *fn,
                                _PyInstr *head);
_PyInterpreterFrame *_PyRop_PushPythonEx(PyThreadState *tstate,
                                                _PyInterpreterFrame *frame,
                                                PyValue *fn, PyValue fo,
                                                PyValue ko, PyValue tup,
                                                _PyInstr *head,
                                                const _PyFbCall *fb);
PyValue _PyRop_CallNativeEx(PyThreadState *tstate,
                                   _PyInterpreterFrame *frame, PyValue fo,
                                   PyValue ko, PyValue tup, _PyInstr *head);
PyValue _PyRop_CallEventEx(PyThreadState *tstate,
                                  _PyInterpreterFrame *frame, _PyInstr *site,
                                  PyValue fo, PyValue tup, _PyInstr *head);
PyValue _PyRop_CallNativeObservedEx(PyThreadState *tstate,
                                           _PyInterpreterFrame *frame,
                                           _PyInstr *site, PyValue fo,
                                           PyValue ko, PyValue tup,
                                           _PyInstr *head);
bool _PyRop_PythonTargetHeld(PyThreadState *tstate,
                                    _PyInterpreterFrame *frame, PyValue fo,
                                    const _PyIRWord *head);
_PyInterpreterFrame *_PyRop_PushPythonHeld(PyThreadState *tstate,
                                                  _PyInterpreterFrame *frame,
                                                  PyValue *fn, PyValue fo,
                                                  _PyInstr *head);
PyValue _PyRop_CallNativeObservedHeld(PyThreadState *tstate,
                                             _PyInterpreterFrame *frame,
                                             _PyInstr *site, PyValue *fn,
                                             PyValue fo,
                                             const _PyIRWord *head);
static inline void _PyRop_RetireFailedHeld(_PyInterpreterFrame *frame,
                                           PyValue *fn, PyValue fo,
                                           const _PyIRWord *head);
PyValue _PyRop_HoldKwargs(_PyInterpreterFrame *frame,
                                 const _PyIRWord *head);
static inline void _PyRop_RetireFailedHeldEx(_PyInterpreterFrame *frame,
                                             PyValue *fn, PyValue fo,
                                             PyValue ko,
                                             const _PyIRWord *head);
static bool _PyRop_PlanFits(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyValueBorrow fn, const _PyIRWord *head,
                            const _PyFbCall *fb);
_PyInterpreterFrame *_PyRop_PushPlan(PyThreadState *tstate,
                                            _PyInterpreterFrame *frame,
                                            PyValue *fn, _PyInstr *head,
                                            const _PyFbCall *fb);
PyValue _PyRop_PlanGenerator(PyThreadState *tstate,
                                    _PyInterpreterFrame *frame, PyValue *fn,
                                    _PyInstr *head, const _PyFbCall *fb);
static bool _PyRop_NotPythonCallable(PyValueBorrow fn);
static bool _PyRop_PyCallableEx(PyValueBorrow fn);
static bool _PyRop_NotPyCallableEx(PyValueBorrow fn);
bool _PyRop_KwargsPending(_PyInterpreterFrame *frame, PyValue *local);
PyValue _PyRop_NewKwargs(_PyInterpreterFrame *frame, PyValue *local);


Py_NO_INLINE int
_PyRop_BranchEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, bool taken);

/* The rows of IterOpen and ForIter (iteration.rop) */

Py_NO_INLINE _PyStatusValue
_PyRop_OpenIter(_PyInterpreterFrame *frame, PyValueBorrow source, uint32_t mode);

static inline PyValue
_PyRop_TaggedZero(void)
{
    return PyValue_TagIndex(0);
}

Py_NO_INLINE _PyStatusValue
_PyRop_ForIterStep(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   PyValueBorrow iter, PyValueBorrow cursor, PyValue *next);

/* The tests and steps of the specialised IterOpen and ForIter arms */
static inline bool
_PyRop_IterByIndex(PyValueBorrow v)
{
    return _PyType_IteratesByIndex(PyValue_TYPE(v));
}

static inline bool
_PyRop_IterSelf(PyValueBorrow v)
{
    return PyValue_TYPE(v)->tp_iter == PyObject_SelfIter;
}

static inline bool
_PyRop_ExactRangeIter(PyValueBorrow v)
{
    if (!_PyRop_IsObject(v)
        || Py_TYPE(PyValue_AsKnownObjectBorrow(v)) != &PyRangeIter_Type) {
        return false;
    }
#ifdef Py_GIL_DISABLED
    // Only a range iterator that nothing else references: this thread alone
    // steps it
    return _PyObject_IsUniquelyReferenced(PyValue_AsKnownObjectBorrow(v));
#else
    return true;
#endif
}

static inline bool
_PyRop_ExactListIter(PyValueBorrow v)
{
    if (!_PyRop_IsObject(v)
        || !PyList_CheckExact(PyValue_AsKnownObjectBorrow(v))) {
        return false;
    }
#ifdef Py_GIL_DISABLED
    // A list another thread owns, and that is not shared, is not stepped here
    PyObject *list_o = PyValue_AsKnownObjectBorrow(v);
    return _Py_IsOwnedByCurrentThread(list_o) || _PyObject_GC_IS_SHARED(list_o);
#else
    return true;
#endif
}

static inline bool
_PyRop_CursorIsIndex(PyValueBorrow c)
{
    return PyValue_IsTaggedIndex(c);
}

static inline bool
_PyRop_CursorIsVacant(PyValueBorrow c)
{
    return PyValue_IsNull(c);
}

static inline bool
_PyRop_HasIternextValue(PyValueBorrow v)
{
    return _PyRop_IsObject(v)
        && _PyType_VALUE_SLOT(Py_TYPE(PyValue_AsKnownObjectBorrow(v)), iternext) != NULL;
}

/* IterOpen.GenMethod's guard: the type's cached __iter__ function, a
 * generator function of one argument, is at the version the cache holds */
static inline bool
_PyRop_IterGenMethod(PyValueBorrow v)
{
    if (!_PyRop_IsObject(v)) {
        return false;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    if (!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)) {
        return false;
    }
    PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
    PyObject *iter_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(ht->_spec_cache.iter);
    if (iter_o == NULL) {
        return false;
    }
    assert(PyFunction_Check(iter_o));
    return ((PyFunctionObject *)iter_o)->func_version ==
           FT_ATOMIC_LOAD_UINT32_RELAXED(ht->_spec_cache.iter_version);
}

/* One step of an exact list at its tagged cursor: 1 an item (the next cursor
 * in *next), 0 the end, whose value is None */
static inline _PyStatusValue
_PyRop_ListStep(PyValueBorrow it, PyValueBorrow cursor, PyValue *next)
{
    PyObject *list_o = PyValue_AsKnownObjectBorrow(it);
    assert(PyList_CheckExact(list_o));
    intptr_t i = PyValue_UntagIndex(cursor);
    if ((size_t)i >= (size_t)PyList_GET_SIZE(list_o)) {
        return (_PyStatusValue){0, PyValue_None};
    }
    *next = PyValue_TagIndex(i + 1);
    return (_PyStatusValue){1, PyValue_DUP(_PyValue_PublishedItem(
        PyList_GET_VALUE(list_o, i)))};
}

static inline _PyStatusValue
_PyRop_TupleStep(PyValueBorrow it, PyValueBorrow cursor, PyValue *next)
{
    PyObject *tuple_o = PyValue_AsKnownObjectBorrow(it);
    assert(PyTuple_CheckExact(tuple_o));
    intptr_t i = PyValue_UntagIndex(cursor);
    if ((size_t)i >= (size_t)PyTuple_GET_SIZE(tuple_o)) {
        return (_PyStatusValue){0, PyValue_None};
    }
    *next = PyValue_TagIndex(i + 1);
    return (_PyStatusValue){1, PyValue_DUP(_PyValue_PublishedItem(
        PyTuple_GET_VALUE(tuple_o, i)))};
}

/* A range iterator's step: status 0 the int, 1 the end (None), -1 raised.
 * start and len change before the int is made. */
static inline _PyStatusValue
_PyRop_RangeStep(PyValueBorrow it)
{
    _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(it);
    assert(Py_TYPE(r) == &PyRangeIter_Type);
    if (r->len <= 0) {
        return (_PyStatusValue){1, PyValue_None};
    }
    long value = r->start;
    r->start = value + r->step;
    r->len--;
    PyValue next = PyValue_FromInt64(value);
    if (PyValue_IsNull(next)) {
        return (_PyStatusValue){-1, PyValue_NULL};
    }
    return (_PyStatusValue){0, next};
}

Py_NO_INLINE _PyStatusValue
_PyRop_IndexedStep(PyValueBorrow iter, PyValueBorrow cursor, PyValue *next);

Py_NO_INLINE _PyStatusValue
_PyRop_SlotStep(PyThreadState *tstate, _PyInterpreterFrame *frame,
                PyValueBorrow iter);

Py_NO_INLINE int
_PyRop_StopIterationEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                          _PyInstr *site, PyValueBorrow receiver,
                          PyValue value);

Py_NO_INLINE int
_PyRop_JumpEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site);

/* The LINE and INSTRUCTION events of the word at site, which the observing
 * evaluator runs before the word where they are observed
 * (_PyIR_OBSERVE_LINE, _PyIR_OBSERVE_INSTRUCTION).  0 goes on to the word,
 * 1 means a LINE callback moved the frame (return_pc names the word to run)
 * and -1 that a callback raised. */
Py_NO_INLINE int
_PyEval_LocationEvents(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       _PyInstr *site);

/* The poll of a word that is interruptible (the backedge Jump arms, Resume):
 * signals, the GC and other pending events, and a loop switch, after which
 * the word runs again */
static inline int
_PyRop_Poll(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site)
{
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (_Py_EvalBreakerHasEvents(tstate)) {
        return _PyRop_HandlePoll(tstate, frame, site);
    }
    return 0;
}

/* The rows of Resume (Python/register_ops/resume.rop) */

/* The code's instrumentation is current and no event is pending */
static inline bool
_PyRop_InstrumentationCurrent(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    return _Py_EvalBreaker(tstate) == FRAME_CODE_INSTRUMENTATION_VERSION();
}

/* Whether a resume point of this kind polls (the delegation loops and a
 * generator expression's start do not) */
static inline bool
_PyRop_ResumePolls(uint32_t kind)
{
    return kind < RESUME_AFTER_YIELD_FROM;
}

/* The code's instrumentation is current and no event is pending, for the
 * observing evaluator: the code carries the observed mark */
static inline bool
_PyRop_ObservingCurrent(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    return (_Py_EvalBreaker(tstate) | _PyCode_OBSERVED_MARK) ==
           FRAME_CODE_INSTRUMENTATION_VERSION();
}

/* Instrument the code if its version is out of date, and switch the frame
 * to the other evaluator if the code's mark asks for it: in the normal
 * evaluator (observing 0) if the code is observed, in the observing one if
 * it is not */
Py_NO_INLINE int
_PyRop_InstrumentIfStale(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         _PyInstr *site);
Py_NO_INLINE int
_PyRop_ObserveIfStale(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *site);

Py_NO_INLINE int
_PyRop_ResumeEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, uint32_t kind);

/* Whether the jump word at site goes back */
static inline bool
_PyRop_GoesBack(_PyInstr *site, _PyInstr *target)
{
    return target <= site;
}


/* The rows of Return.Observed */

Py_NO_INLINE PyValue
_PyRop_ReturnEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, PyValue value);

Py_NO_INLINE int
_PyRop_YieldEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  _PyInstr *site, PyValue *home);

static inline bool
_PyRop_GeneratorFrame(_PyInterpreterFrame *frame)
{
    return frame->owner == FRAME_OWNED_BY_GENERATOR;
}

static inline _PyInterpreterFrame *
_PyRop_PreviousFrame(_PyInterpreterFrame *frame)
{
    return frame->previous;
}

static inline _PyInterpreterFrame *
_PyRop_Depart(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    assert(tstate->current_frame == frame);
    DTRACE_FUNCTION_RETURN();
    _Py_LeaveRecursiveCallPy(tstate);
    // GH-99729: the frame is unlinked before it is cleared
    tstate->current_frame = frame->previous;
    return frame;
}

static inline void _PyRop_RetireFrame(PyThreadState *tstate,
                                      _PyInterpreterFrame *dying);
static inline void _PyRop_RetireFrameEmpty(PyThreadState *tstate,
                                           _PyInterpreterFrame *dying);

static inline _PyInstr *
_PyRop_Deliver(_PyInterpreterFrame *caller, PyValue v)
{
    assert(_PyValue_IsHome(v));
    (void)_PyFrame_SetReturnValue(caller, v);
    return caller->return_pc;
}

static inline void
_PyRop_LeaveActivation(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
    assert(_PyFrame_IsIncomplete(frame));
    assert(!_PyErr_Occurred(tstate));
    tstate->current_frame = frame->previous;
}

Py_NO_INLINE int
_PyEval_gen_prologue_run(PyThreadState *tstate, PyGenObject *gen, PyCodeObject *code);

/* _PyEval_gen_prologue_run() for the code that has one: most generator functions have
 * no cell variables and are not generator expressions (whose first traceable
 * word follows the prelude). */
static inline Py_ALWAYS_INLINE int
gen_prologue(PyThreadState *tstate, PyGenObject *gen, PyCodeObject *code)
{
    if (code->co_ncellvars == 0 && code->_co_exec->first_traceable == 0) {
        return 0;
    }
    return _PyEval_gen_prologue_run(tstate, gen, code);
}

Py_NO_INLINE PyValue
_PyRop_NewIterGenerator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        PyValueBorrow iterable);

/* The generator that the call at head creates, when the callee is a generator
 * function that takes exactly these arguments (CALL_PY_GENERATOR): the
 * operands fill its frame, with no frame of the thread.  The method operand
 * of an expanded call is not a form of it.  The null value is an error. */
static inline Py_ALWAYS_INLINE PyValue
call_new_generator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   const _PyIRWord *head, _PyIRWord word, PyValue callable,
                   int call_state)
{
    assert(!(call_state & CALL_EXPANDED));
    PyValue *slots = _PyFrame_Slots(frame);
    PyFunctionObject *func =
        (PyFunctionObject *)_PyValue_AsKnownObjectBorrowPointer(callable);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    PyValue function = call_exact_function(slots, word, callable, 0);
    PyGenObject *gen = _PyGen_NewCall(tstate, function, NULL, code, frame);
    if (gen == NULL) {
        return PyValue_NULL;
    }
    call_fill_exact(slots, frame, head, word, PyValue_NULL, call_state,
                    gen->gi_iframe.base);
    if (gen_prologue(tstate, gen, code) < 0) {
        _PyGen_DiscardCall(gen);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal((PyObject *)gen);
}

Py_NO_INLINE PyValue
_PyRop_NewGenerator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue *fn, const _PyIRWord *head);

static inline Py_ALWAYS_INLINE int
call_refresh(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         _PyInstr *current)
{
    int status = value_operation_refresh(tstate, frame, current, PyValue_NULL);
    if (status == 0) {
        return 0;
    }
    if (!value_frame_is_current(tstate, frame)) {
        _PyEval_value_revalidation_error(
            "call preparation changed the active frame");
        return -1;
    }
    return status;
}

/* The word of an immortal or caller-pinned object for a monitoring argument:
 * uncounted, never closed. */
static inline PyValue
borrowed_word(PyObject *obj)
{
    PyValue word;
    _PyValue_WindowFromObjects(&word, &obj, 1);
    return word;
}

/* The handlers' calls (call_run_*), after the binder
 * (eval_frame_push_and_init_code), which they use. */
_PyCallResult _PyEval_call_bind_positional(
    PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValue, PyValue,
    const _PyBindPlan *);

typedef struct {
    PyThreadState *tstate;
    _PyInterpreterFrame *frame;
    PyCodeObject *code;
    Py_ssize_t current_word;
    Py_ssize_t next_word;
} _PyNativeCallPublication;











void
_PyRop_UnboundLocal(_PyInterpreterFrame *frame, PyValue *home);

PyValue
_PyRop_AppendText(PyValue *home, PyValueBorrow right);

PyValue
_PyRop_DeleteLocal(_PyInterpreterFrame *frame, PyValue *home);

void
_PyRop_UnboundCell(_PyInterpreterFrame *frame, PyValue *home);

PyValue
_PyRop_MakeCell(_PyInterpreterFrame *frame, PyValue *home);

PyValue
_PyRop_GetAwaitable(PyValue x, uint32_t origin);

PyValue
_PyRop_GetANext(PyValue *aiter_home);

/* The rows of Send (Python/register_ops/suspend.rop) */

/* Whether the receiver can be entered in place.  The guards of the old
 * _SEND: a generator or coroutine of the exact types, a created one given
 * None only, and the relaxed load of its frame state that gen_may_claim
 * makes.  The answer can be stale on the free-threaded build: the claim in
 * _PyRop_EnterGenerator decides. */
static inline bool
_PyRop_ClaimableGenerator(PyThreadState *tstate, PyValueBorrow receiver,
                          PyValueBorrow sent)
{
    if (IS_PEP523_HOOKED(tstate) || !_PyRop_IsObject(receiver)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(receiver);
    if (Py_TYPE(object) != &PyGen_Type && Py_TYPE(object) != &PyCoro_Type) {
        return false;
    }
    PyGenObject *gen = (PyGenObject *)object;
    if (FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state) == FRAME_CREATED &&
        !PyValue_IsNone(sent)) {
        return false;
    }
    return gen_may_claim(gen);
}

/* Link the claimed generator's frame under frame, which returns it: the
 * links of the old IR_ENTER_GENERATOR, without the resume value. */
static inline _PyInterpreterFrame *
_PyRop_LinkFrame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 PyGenObject *gen)
{
    _PyInterpreterFrame *resumed = &gen->gi_iframe;
    resumed->previous = frame;
    gen->gi_exc_state.previous_item = tstate->exc_info;
    tstate->exc_info = &gen->gi_exc_state;
    return resumed;
}

/* The links and the value sent in.  The recursion debit and the first word
 * are the frame start's. */
static inline _PyInterpreterFrame *
_PyRop_LinkGenerator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     PyGenObject *gen, PyValue sent)
{
    _PyInterpreterFrame *resumed = _PyRop_LinkFrame(tstate, frame, gen);
    _PyFrame_SetResumeValue(resumed, sent);
    return resumed;
}

/* A delegation chain moves in one transfer (pyjit G4, golden87's
 * gen_resume and gen_yield): the thread's share of the test, made once per
 * transfer.  The free-threaded build enters and suspends one level per
 * word, as golden87's generator_thread_is_plain decided: another thread
 * can claim a receiver between its guard and its claim.  A DTrace probe
 * reports each level's entry and return, which a frame start and a
 * suspension fire. */
static inline bool
_PyRop_ChainThreadPlain(void)
{
#if defined(Py_GIL_DISABLED)
    return false;
#elif defined(WITH_DTRACE)
    return !PyDTrace_FUNCTION_ENTRY_ENABLED() && !PyDTrace_FUNCTION_RETURN_ENABLED();
#else
    return true;
#endif
}

/* The Send of the next level of a chain that level, a claimed and linked
 * generator frame not yet started, would run, or NULL where the walk stops.
 * Level resumes after the chain(resume) word of a yield-from loop, whose
 * words are the chain(send) arm, that word, then the resume head (a Resume)
 * and a Jump.Direct back to the Send; the Send's value home is level's
 * resume slot; its receiver passes the arm's guard with the value sent in;
 * level's recursion debit cannot fail, so a RecursionError is raised in the
 * level that raises it now; and level's start would skip its Resume
 * (_PyRop_StartSkips: no observed code, no stale instrumentation, no
 * pending event, no Resume.Warm of code lowered for the JIT that has no
 * live entries: compile rewrites the heads of compiled code to Resume.Poll,
 * so the walk passes compiled levels).  The start's
 * test runs last: it counts a signal check under Emscripten, as the start
 * does.  A created generator, an ordinary yield, a relaxed loop, a Send of
 * another arm and a non-generator receiver end the walk. */
static inline Py_ALWAYS_INLINE _PyInstr *
_PyRop_ChainSend(PyThreadState *tstate, _PyInterpreterFrame *level,
                 PyValueBorrow sent)
{
    _PyInstr *resume = level->instr_ptr;
    _PyInstr *send = resume - 2;
    if (level->pending_redirect || send < level->ir_words ||
        !_PyRop_ChainResumes(IR_HANDLER(resume[-1])) ||
        tstate->py_recursion_remaining <= 0) {
        return NULL;
    }
#ifdef Py_DEBUG
    /* lltrace reports each frame's start */
    if (level->lltrace) {
        return NULL;
    }
#endif
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*send);
    _PyIRWord jump = FT_ATOMIC_LOAD_UINT64_RELAXED(resume[1]);
    if (!_PyRop_ChainSends(IR_HANDLER(word)) ||
        IR_HANDLER(jump) != ROP_HANDLER_Jump_Direct ||
        _PyIR_FormatDistance(_PyIR_FormatOf(ROP_HANDLER_Jump_Direct), jump) != -4 ||
        IR_B(word) + 2 != level->return_slot ||
        !_PyRop_ClaimableGenerator(tstate, PyValue_Borrow(level->base[IR_B(word)]),
                                   sent) ||
        !_PyRop_StartSkips(tstate, level, resume)) {
        return NULL;
    }
    return send;
}

/* The chain entry of enter_generator and enter_for_generator: child is the
 * receiver's frame, claimed and linked.  While child resumes at the Send of
 * another level (_PyRop_ChainSend), do what child's start, its Resume, its
 * Jump.Direct and that Send would do: the recursion debit, instr_ptr at the
 * Send, return_slot the Send's value home (already) and return_pc the
 * chain(resume) word after it, and the claim and link of the Send's
 * receiver, which becomes child.  The value sent in stays with the caller,
 * for the deepest frame: each level's value home stays vacant, as the Send
 * that took it left it.  Returns the deepest frame. */
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
_PyRop_EnterChain(PyThreadState *tstate, _PyInterpreterFrame *child,
                  PyValueBorrow sent)
{
    if (!_PyRop_ChainThreadPlain()) {
        return child;
    }
    _PyInstr *send;
    while ((send = _PyRop_ChainSend(tstate, child, sent)) != NULL) {
        tstate->py_recursion_remaining--;
        assert(PyValue_IsNull(child->base[child->return_slot]));
        child->instr_ptr = send;
        child->return_pc = send + 1;
        PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(
            child->base[IR_B(*send)]);
        int claimed = gen_claim(tstate, gen);
        assert(claimed);
        (void)claimed;
        child = _PyRop_LinkFrame(tstate, child, gen);
    }
    return child;
}

/* A delegation chain's suspension (pyjit G4, golden87's gen_yield): a
 * generator has suspended to frame, its consumer.  While frame would run,
 * at its return_pc, the chain(resume) word of its yield-from loop, whose
 * value and resume homes are frame's return slot, do what that word would
 * do: suspend frame too, in the delegating state, and go on to its
 * consumer.  The value stays with the caller, for the last consumer: each
 * level's value home stays vacant, as the word that took it left it.  The
 * walk stops at a frame that is not a generator's, has a pending redirect
 * (or a switch of evaluator: its return_pc is the switch word), runs
 * observed code (the word would be its observer, which reports PY_YIELD),
 * and at any other word (a relaxed loop's
 * trailing jump, a for loop).  The free-threaded build suspends one level
 * per word, as golden87's generator_thread_is_plain decided
 * (_PyRop_ChainThreadPlain).  Returns the last consumer. */
static inline Py_ALWAYS_INLINE _PyInterpreterFrame *
_PyRop_SuspendChain(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    if (!_PyRop_ChainThreadPlain()) {
        return frame;
    }
    for (;;) {
        if (frame->owner != FRAME_OWNED_BY_GENERATOR || frame->pending_redirect ||
            _PyCode_IsObserved(_PyFrame_GetCode(frame))) {
            return frame;
        }
#ifdef Py_DEBUG
        /* lltrace reports each word */
        if (frame->lltrace) {
            return frame;
        }
#endif
        _PyInstr *yield = frame->return_pc;
        _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*yield);
        if (!_PyRop_ChainResumes(IR_HANDLER(word)) || (word & IR_F_LOCAL_A) ||
            IR_A(word) != frame->return_slot || IR_B(word) != frame->return_slot) {
            return frame;
        }
        assert(PyValue_IsNull(frame->base[frame->return_slot]));
        frame->instr_ptr = yield + 1;
        PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
        tstate->exc_info = gen->gi_exc_state.previous_item;
        gen->gi_exc_state.previous_item = NULL;
        _Py_LeaveRecursiveCallPy(tstate);
        _PyInterpreterFrame *suspended = frame;
        _PyThreadState_UpdateLastProfiledFrame(tstate, suspended, suspended->previous);
        frame = tstate->current_frame = suspended->previous;
        suspended->previous = NULL;
        FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED_YIELD_FROM);
    }
}

/* Send.Gen: claim the generator that _PyRop_ClaimableGenerator accepted, as
 * SEND_GEN did, link its frame, and enter the chain below it.
 * Free-threaded, another thread can claim it first, and the send raises
 * what the generic send raises for an executing generator. */
static inline _PyInterpreterFrame *
_PyRop_EnterGenerator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      PyValueBorrow receiver, PyValue sent)
{
    PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
    if (!gen_claim(tstate, gen)) {
        PyObject *error = PyErr_GetRaisedException();
        PyValue_XCLOSE(sent);
        PyErr_SetRaisedException(error);
        return NULL;
    }
    _PyInterpreterFrame *child = _PyRop_EnterChain(
        tstate, _PyRop_LinkFrame(tstate, frame, gen), PyValue_Borrow(sent));
    _PyFrame_SetResumeValue(child, sent);
    return child;
}

Py_NO_INLINE bool
_PyRop_ClaimGenerator(PyThreadState *tstate, PyValueBorrow receiver,
                      PyValueBorrow sent);

Py_NO_INLINE _PyInterpreterFrame *
_PyRop_EnterClaimed(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValueBorrow receiver, PyValue sent);

/* Whether a Send site may be specialized on this receiver: a generator or
 * coroutine of the exact types while no PEP 523 hook is installed */
static inline bool
_PyRop_GeneratorReceiver(PyValueBorrow receiver)
{
    if (!_PyRop_IsObject(receiver) ||
        !_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET()))
    {
        return false;
    }
    PyTypeObject *type = Py_TYPE(PyValue_AsKnownObjectBorrow(receiver));
    return type == &PyGen_Type || type == &PyCoro_Type;
}

/* The rows of ForIter.Gen (Python/register_ops/iteration.rop): the guards of
 * the old _FOR_ITER_GEN_FRAME.  The answer of the first can be stale on the
 * free-threaded build: the claim in the second decides. */
static inline bool
_PyRop_ExactGeneratorIter(PyValueBorrow receiver)
{
    return _PyRop_IsObject(receiver) &&
        _PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET()) &&
        Py_TYPE(PyValue_AsKnownObjectBorrow(receiver)) == &PyGen_Type;
}

static inline bool
_PyRop_ClaimableForGenerator(PyThreadState *tstate, PyValueBorrow receiver)
{
    if (IS_PEP523_HOOKED(tstate) || !_PyRop_IsObject(receiver)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(receiver);
    return Py_TYPE(object) == &PyGen_Type && gen_may_claim((PyGenObject *)object);
}

/* ForIter.Generic's claim: the guards of the old _FOR_ITER_GEN_FRAME, with
 * the claim of gen_try_set_executing.  A generator that is running, or that
 * another thread holds, is stepped. */
static inline bool
_PyRop_ClaimForGenerator(PyThreadState *tstate, PyValueBorrow receiver)
{
    if (IS_PEP523_HOOKED(tstate) || !_PyRop_IsObject(receiver)) {
        return false;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(receiver);
    return Py_TYPE(object) == &PyGen_Type &&
        gen_try_set_executing((PyGenObject *)object);
}

static inline _PyInterpreterFrame *
_PyRop_EnterForClaimed(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       PyValueBorrow receiver)
{
    PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
    return _PyRop_LinkGenerator(tstate, frame, gen, PyValue_None);
}

/* Claim the generator that _PyRop_ClaimableForGenerator accepted and link its
 * frame under frame, with None to resume with, and enter the chain below
 * it.  NULL: another thread claimed it first, and ValueError is set. */
static inline _PyInterpreterFrame *
_PyRop_EnterForGenerator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         PyValueBorrow receiver)
{
    PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
    if (!gen_claim(tstate, gen)) {
        return NULL;
    }
    _PyInterpreterFrame *child = _PyRop_EnterChain(
        tstate, _PyRop_LinkFrame(tstate, frame, gen), PyValue_Borrow(PyValue_None));
    _PyFrame_SetResumeValue(child, PyValue_None);
    return child;
}

/* ForIter.Complete's receiver, whose generator the return cleared:
 * golden87's gen_try_close_completed, else a close */
static inline void
_PyRop_CloseCompletedReceiver(PyValue receiver)
{
    if (PyValue_RefcountOnObject(PyValue_Borrow(receiver)) &&
        _PyGen_TryCloseClearedNoReentry(PyValue_AsKnownObjectBorrow(receiver))) {
        return;
    }
    PyValue_XCLOSE(receiver);
}

static inline bool
_PyRop_HasIterItem(PyValueBorrow receiver)
{
    return _PyRop_IsObject(receiver) &&
           Py_TYPE(PyValue_AsKnownObjectBorrow(receiver))->_tp_iteritem != NULL;
}

static inline bool
_PyRop_IsAsyncGenASend(PyValueBorrow receiver)
{
    return _PyRop_IsObject(receiver) &&
           PyAsyncGenASend_CheckExact(PyValue_AsKnownObjectBorrow(receiver));
}

static inline bool
_PyRop_CursorPresent(PyValueBorrow cursor)
{
    return !PyValue_IsNull(cursor);
}

Py_NO_INLINE _PyStatusValue
_PyRop_SendVirtual(PyValueBorrow receiver, PyValueBorrow cursor, PyValue *next);

Py_NO_INLINE _PyStatusValue
_PyRop_SendStep(PyThreadState *tstate, _PyInterpreterFrame *frame,
                PyValueBorrow receiver, PyValueBorrow cursor,
                PyValueBorrow sent, PyValue *next);

Py_NO_INLINE _PyStatusValue
_PyRop_SendStopEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *site, PyValueBorrow receiver, PyValue value);

Py_NO_INLINE int
_PyRop_SendExitEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *site);

_PyStatusValue
_PyRop_LoadFromDictCell(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        PyValueBorrow mapping, PyValue *home);

/* The rows of the namespace families (Python/register_ops/namespaces.rop).
 * Each is the body of the stack op's old handler.  A row that takes an
 * owner closes it on every path. */

PyValue
_PyRop_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                uint32_t index);

PyValue
_PyRop_LoadLocals(PyThreadState *tstate, _PyInterpreterFrame *frame);

PyValue
_PyRop_LoadBuildClass(PyThreadState *tstate, _PyInterpreterFrame *frame);

_PyStatusValue
_PyRop_LoadFromDict(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValueBorrow mapping, uint32_t index);

PyValue
_PyRop_LoadGlobalOrBuiltin(_PyInterpreterFrame *frame, uint32_t index);

int
_PyRop_StoreName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 uint32_t index, PyValue v);

int
_PyRop_DeleteName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  uint32_t index);

int
_PyRop_StoreGlobal(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   uint32_t index, PyValue v);

int
_PyRop_DeleteGlobal(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    uint32_t index);

int
_PyRop_SetupAnnotations(PyThreadState *tstate, _PyInterpreterFrame *frame);

PyValue
_PyRop_ImportName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyObject *name, PyValue level, PyValue fromlist,
                  uint32_t kind);

PyValue
_PyRop_ImportFrom(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue module, uint32_t index);


void
_PyRop_MoveInOrder(const _PyRopSpan *d, const _PyRopSpan *s);

void
_PyRop_RotateHomes(_PyInterpreterFrame *frame, const _PyRopSpan *homes);

void
_PyRop_CloseSpan(const _PyRopSpan *s);

void
_PyRop_CopyHomes(const _PyRopSpan *d, const _PyRopSpan *s);

/* The rows of Build, AddElement, AddEntry, UpdateContainer and
 * MergeKeywords (Python/register_ops/displays.rop). */

void
_PyEval_rop_take_items(const _PyRopSpan *span, PyValue *vals);



PyValue
_PyRop_BuildTuple(const _PyRopSpan *items);

PyValue
_PyRop_BuildList(const _PyRopSpan *items);

PyValue
_PyRop_BuildSet(const _PyRopSpan *items);

PyValue
_PyRop_BuildMap(const _PyRopSpan *items);

PyValue
_PyRop_JoinText(const _PyRopSpan *parts);

PyValue
_PyRop_BuildInterpolation(const _PyRopSpan *parts, int flags);

PyValue
_PyRop_BuildTemplate(const _PyRopSpan *parts);

int
_PyRop_ListAppend(PyValue *list, PyValue item);

int
_PyRop_SetAdd(PyValue *set, PyValue item);

int
_PyRop_DictPut(PyValue *dict, PyValue key, PyValue value);

int
_PyRop_ListExtend(PyValue *list_home, PyValue *it_home);

int
_PyRop_SetUnion(PyValue *set_home, PyValue *it_home);

int
_PyRop_DictOverlay(PyValue *dict_home, PyValue *update_home);

int
_PyRop_MergeKeywords(PyValue *fn_home, PyValue *dict_home, PyValue *update_home);


int
_PyRop_UnpackIterable(PyValue *source_home, Py_ssize_t n, _PyRopItems *out);

int
_PyRop_UnpackStar(PyValue *source_home, Py_ssize_t shape, _PyRopItems *out);

int
_PyRop_UnpackList(PyValue *source_home, Py_ssize_t n, _PyRopItems *out);

bool
_PyRop_TupleHasLength(PyValueBorrow v, Py_ssize_t n);

bool
_PyRop_ListHasLength(PyValueBorrow v, Py_ssize_t n);

int
_PyRop_ClassifyUnpack(PyValueBorrow v, Py_ssize_t n);

Py_NO_INLINE _PyIRErrorResult
_PyEval_ProcessError(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *origin, uint32_t entry);

/* The helpers of the generator transfer labels (Python/ceval_generator.h),
 * which are in the evaluator below. */

static inline PyValue
generator_payload(_PyIRWord payload)
{
    return _PyValue_FromTransferWord(payload);
}

static inline uint64_t
generator_control_generation(_PyInterpreterFrame *frame)
{
    /* Interpreter entry frames do not initialize frame_obj. */
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }
    return frame->frame_obj == NULL ? 0 : frame->frame_obj->f_lineno_generation;
}

/* The caller holds value in C, which is an external reference to the cyclic
 * collector. Every frame write precedes the first close. */
static inline void
generator_publish_redirected(PyThreadState *tstate, _PyInterpreterFrame *frame,
                             PyValue value)
{
    if (frame->pending_redirect) {
        _PyFrame_CompleteRedirect(frame);
    }
    _PyFrame_PublishReturnSlot(frame, value);
}
/* The helpers of two outlined handlers (BINARY_SLICE, CALL_INTRINSIC_1), in
 * line in the functions that run them: the handler is a function of its own
 * already, and a second call in front of the work only adds a frame. They
 * are defined below, with the code they share. */
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_BinarySlice)(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop);
#define _PyEval_BinarySlice(V0, V1, V2) \
    (_PyEval_BinarySlice)(PyValue_Borrow(V0), PyValue_Borrow(V1), PyValue_Borrow(V2))
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_CallIntrinsic1)(PyThreadState *tstate, int oparg, PyValueBorrow value);
#define _PyEval_CallIntrinsic1(T, O, V) (_PyEval_CallIntrinsic1)(T, O, PyValue_Borrow(V))

/* The rows of the item families (Python/register_ops/items.rop) */
static inline PyValue
_PyRop_Subscript(PyValueBorrow container, PyValueBorrow key)
{
    return _PyEval_value_subscript(container, key);
}

static inline int
_PyRop_SetItem(PyValueBorrow container, PyValueBorrow key, PyValueBorrow value)
{
    return _PyValue_SetItem(container, key, value);
}

static inline int
_PyRop_DelItem(PyValueBorrow container, PyValueBorrow key)
{
    return _PyValue_DelItem(container, key);
}

static inline PyValue
_PyRop_BinarySlice(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop)
{
    return (_PyEval_BinarySlice)(container, start, stop);
}

static inline int
_PyRop_StoreSlice(PyValueBorrow container, PyValueBorrow start, PyValueBorrow stop,
                  PyValueBorrow value)
{
    return (_PyEval_StoreSlice)(value, container, start, stop);
}

static inline bool
_PyRop_ExactTuple(PyValueBorrow v)
{
    return _PyRop_IsObject(v) && PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_ExactSlice(PyValueBorrow v)
{
    return _PyRop_IsObject(v) && PySlice_Check(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_CompactAsciiStr(PyValueBorrow v)
{
    if (!_PyRop_IsObject(v)) {
        return false;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    return PyUnicode_CheckExact(o) && PyUnicode_IS_COMPACT_ASCII(o);
}

/* The index of a list folds from the end, as the old FROM_END guard did */
static inline Py_ssize_t
_PyRop_ListIndex(PyObject *list, PyValueBorrow k)
{
    Py_ssize_t index = PyValue_InlineInt64(k);
    return index < 0 ? index + PyList_GET_SIZE(list) : index;
}

static inline bool
_PyRop_ListInBounds(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *list = PyValue_AsKnownObjectBorrow(v);
    Py_ssize_t index = _PyRop_ListIndex(list, k);
    return 0 <= index && index < PyList_GET_SIZE(list);
}

static inline PyValueBorrow
_PyRop_ListItem(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *list = PyValue_AsKnownObjectBorrow(v);
    return PyValue_Borrow(_PyList_ITEMS(list)[_PyRop_ListIndex(list, k)]);
}

static inline bool
_PyRop_TupleInBounds(PyValueBorrow v, PyValueBorrow k)
{
    Py_ssize_t index = PyValue_InlineInt64(k);
    return 0 <= index && index < PyTuple_GET_SIZE(PyValue_AsKnownObjectBorrow(v));
}

static inline PyValueBorrow
_PyRop_TupleItem(PyValueBorrow v, PyValueBorrow k)
{
    return PyTuple_GET_VALUE(PyValue_AsKnownObjectBorrow(v), PyValue_InlineInt64(k));
}

static inline bool
_PyRop_StrInBounds(PyValueBorrow v, PyValueBorrow k)
{
    Py_ssize_t index = PyValue_InlineInt64(k);
    return 0 <= index && index < PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(v));
}

static inline PyValueBorrow
_PyRop_AsciiChar(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *str = PyValue_AsKnownObjectBorrow(v);
    uint8_t c = PyUnicode_1BYTE_DATA(str)[PyValue_InlineInt64(k)];
    assert(c < 128);
    return PyValue_FromObjectBorrow((PyObject *)&_Py_SINGLETON(strings).ascii[c]);
}

static inline PyValueBorrow
_PyRop_UnicodeAsciiChar(PyValueBorrow v, PyValueBorrow k)
{
    PyObject *str = PyValue_AsKnownObjectBorrow(v);
    Py_UCS4 c = PyUnicode_READ_CHAR(str, PyValue_InlineInt64(k));
    if (Py_ARRAY_LENGTH(_Py_SINGLETON(strings).ascii) <= c) {
        return PyValue_Borrow(PyValue_NULL);
    }
    return PyValue_FromObjectBorrow((PyObject *)&_Py_SINGLETON(strings).ascii[c]);
}

static inline bool
_PyRop_DictSubscriptSlot(PyValueBorrow v)
{
    if (!_PyRop_IsObject(v)) {
        return false;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    return tp->tp_as_mapping != NULL
        && tp->tp_as_mapping->mp_subscript == _PyDict_Subscript;
}

static inline bool
_PyRop_DictAssignSlot(PyValueBorrow v)
{
    if (!_PyRop_IsObject(v)) {
        return false;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    return tp->tp_as_mapping != NULL
        && tp->tp_as_mapping->mp_ass_subscript == _PyDict_StoreSubscript;
}

static inline bool
_PyRop_DictFirstProbe(PyValueBorrow v, PyValueBorrow k, PyValue *out)
{
    PyValue hit = _PyDict_TryGetFirstProbeValue(v, k);
    if (PyValue_IsNull(hit)) {
        return false;
    }
    *out = hit;
    return true;
}

/* The old STORE_ATTR_INSTANCE_VALUE: lock the owner, check the type version
   and the inline values again, store.  true with the previous owner, which
   is NULL when the store added the attribute. */
static inline bool
_PyRop_StoreInstanceLocked(PyValueBorrow owner, _PyAttrRecipe r,
                            PyValueBorrow x, PyValue *old)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    if (!LOCK_OBJECT(o)) {
        return false;
    }
    if (!_PyAttr_InstanceVersion(o, r) || !_PyAttr_StoreValuesReady(o, r)) {
        UNLOCK_OBJECT(o);
        return false;
    }
    *old = PyValue_NULL;
    _PyAttr_StoreInstanceValue(owner, r, PyValue_DUP(x), old);
    UNLOCK_OBJECT(o);
    return true;
}

/* The old STORE_ATTR_SLOT: lock the owner, check the type version again,
   store.  true with the previous owner, which is NULL for a vacant slot. */
static inline bool
_PyRop_StoreMemberLocked(PyValueBorrow owner, _PyAttrRecipe r,
                          PyValueBorrow x, PyValue *old)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    if (!LOCK_OBJECT(o)) {
        return false;
    }
    if (!_PyAttr_InstanceVersion(o, r)) {
        UNLOCK_OBJECT(o);
        return false;
    }
    _PyAttr_StoreMemberSlot(owner, r, PyValue_DUP(x), old);
    UNLOCK_OBJECT(o);
    return true;
}

/* The old STORE_ATTR_WITH_HINT: lock the managed dictionary, check the
   hinted entry again, store.  true with the previous owner, never NULL. */
static inline bool
_PyRop_StoreHintedLocked(PyValueBorrow owner, PyObject *name,
                          _PyAttrRecipe r, PyValueBorrow x, PyValue *old)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    PyDictObject *dict = _PyObject_GetManagedDict(o);
    if (dict == NULL || !LOCK_OBJECT(dict)) {
        return false;
    }
    if (!_PyAttr_InstanceVersion(o, r) || !_PyAttr_HintStoreReady(o, name, r)) {
        UNLOCK_OBJECT(dict);
        return false;
    }
    *old = _PyAttr_StoreHinted(owner, r, x);
    UNLOCK_OBJECT(dict);
    return true;
}

/* The old _STORE_SUBSCR_LIST_INT: lock the list, check the bounds again,
 * exchange the item, unlock.  true with the displaced owner, which is never
 * NULL; false, having changed nothing and not counted x, when the lock is not
 * taken or the index is out of bounds under it.  The guard before the row
 * only keeps a word that cannot hit from taking the lock. */
static inline bool
_PyRop_ListExchangeLocked(PyValueBorrow v, PyValueBorrow k, PyValueBorrow x,
                          PyValue *old)
{
    PyObject *list = PyValue_AsKnownObjectBorrow(v);
    if (!LOCK_OBJECT(list)) {
        return false;
    }
    Py_ssize_t index = _PyRop_ListIndex(list, k);
    if (index < 0 || index >= PyList_GET_SIZE(list)) {
        UNLOCK_OBJECT(list);
        return false;
    }
    *old = _PyValueSlot_ExchangeFilled(_PyList_ITEMS(list) + index,
                                       PyValue_DUP(x));
    UNLOCK_OBJECT(list);
    return true;
}

static inline PyValue
_PyRop_ListSlice(PyValueBorrow v, PyValueBorrow s)
{
    PyObject *res = _PyList_SliceSubscript(PyValue_AsKnownObjectBorrow(v),
                                           PyValue_AsKnownObjectBorrow(s));
    return res == NULL ? PyValue_NULL : PyValue_FromNonNumberSteal(res);
}

static inline int
_PyRop_DictStore(PyValueBorrow v, PyValueBorrow k, PyValueBorrow x)
{
    return _PyDict_SetValue((PyDictObject *)PyValue_AsKnownObjectBorrow(v), k, x);
}

PyValue
_PyRop_SliceFromParts(const _PyRopSpan *parts);






/* The evaluation loop: one activation.  It is a borrow scope with a floor
 * (pyjit/design/native-boundary.md, section 2).
 *
 * The release invariant: when _Py_HandlePending() releases the entries above
 * eval_floor, no live C frame between the floor and the current instruction
 * holds a mark or a borrow.  Every caller of _Py_HandlePending() is at an
 * instruction boundary of this activation, where all native code it called
 * has returned.  So the floor is set here, around the evaluator, and not in
 * _PyEval_EvalFrame(): a PEP 523 hook runs C code between the two, and
 * anything it pushed would be released while still live.
 *
 * The loop sets up the activation and calls the evaluator, ir_evaluate(),
 * which runs the frames until INTERPRETER_EXIT or an unhandled error leaves
 * it.  It keeps an external name so that debuggers and profilers
 * (Tools/gdb/libpython.py) can find its frames. */
PyValue _PyEval_EvalFrameLoop(PyThreadState *tstate,
                              _PyInterpreterFrame *frame, int throwflag);

Py_NO_INLINE PyValue _Py_HOT_FUNCTION DONT_SLP_VECTORIZE
_PyEval_EvalFrameLoop(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag);


#ifdef DO_NOT_OPTIMIZE_INTERP_LOOP
#  pragma optimize("", on)
#endif

/* The evaluation loop for callers that take the value of the frame as it is
 * (the value vectorcall of a Python function, generators).  A PEP 523 hook
 * returns an object, which it steals back. */
static inline Py_ALWAYS_INLINE PyValue
eval_frame_value(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 int throwflag)
{
    if (tstate->interp->eval_frame == NULL) {
        EVAL_CALL_STAT_INC(EVAL_CALL_TOTAL);
        return _PyEval_EvalFrameLoop(tstate, frame, throwflag);
    }
    return _PyValue_FromObjectResult(_PyEval_EvalFrame(tstate, frame, throwflag));
}



#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER) /* MS_WINDOWS */
#  pragma warning(pop)
#endif







/* Retire a thread frame as upstream does: it is unlinked, the caller holds
 * the result in C, and each owner is NULLed and closed in turn. A finalizer
 * that runs sees a consistent frame; nested calls may grow the arena but
 * must restore this reservation, which stays until the code is closed. */
static inline PyValue
retire_thread_frame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue result, int extent)
{
    assert(frame->owner == FRAME_OWNED_BY_THREAD);
    assert((PyObject **)frame == tstate->datastack_limit);
    assert(frame->frame_obj == NULL || frame->frame_obj->f_frame == frame);
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
    _PyFrame_ClearExceptCode(frame, extent);
    Py_CLEAR(frame->f_executable);
#ifndef NDEBUG
    assert(tstate->datastack_chunk == chunk);
    assert(tstate->datastack_top == top);
    assert(tstate->datastack_limit == limit);
#endif
    _PyThreadState_PopFrame(tstate, frame);
    return result;
}




static inline Py_ALWAYS_INLINE _PyIRStepResult
complete_step_impl(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, int base, int destination,
                   _PyInstr *exit, PyValue value, enum _PyIRStepKind kind)
{
    assert(_PyValue_IsHome(value));
    PyValue *window = frame->base + base;
    PyValue receiver = window[0];
    PyCodeObject *code = _PyFrame_GetCode(frame);
    frame->instr_ptr = site;
    if (code->_co_monitoring != NULL && PyValue_IsObject(receiver)) {
        PyObject *object = PyValue_AsKnownObjectBorrow(receiver);
        if (PyGen_CheckExact(object) || PyCoro_CheckExact(object)) {
            _PyIRStepResult observed = _PyIR_ObserveStep(
                tstate, frame, site, exit, PyValue_Borrow(value), kind, 0);
            if (observed.status < 0) {
                PyObject *error = PyErr_GetRaisedException();
                assert(error != NULL);
                PyValue_XCLOSE(value);
                PyErr_SetRaisedException(error);
                observed.next = frame->instr_ptr + 1;
                return observed;
            }
            if (observed.status == _PY_IR_STEP_REDIRECT) {
                PyValue_XCLOSE(value);
                observed.next = frame->instr_ptr;
                return observed;
            }
        }
    }

    /* A nonredirecting PRE callback cannot move the active frame or mutate
     * its scratch window. Its homes anchor receiver throughout observation;
     * a redirect returned above, before any cached input is reused.
     *
     * Commit the outcome without callbacks. A SEND destination may overlap
     * any input; detach the whole window before publishing it. */
    PyValue cursor = window[1];
    PyValue sent = kind != _PY_IR_FOR_ITER ? window[2] : PyValue_NULL;
    window[0] = window[1] = PyValue_NULL;
    if (kind != _PY_IR_FOR_ITER) {
        window[2] = PyValue_NULL;
    }
    PyValue displaced = PyValue_NULL;
    if (kind != _PY_IR_FOR_ITER) {
        assert(destination < _PyFrame_NumSlots(frame));
        displaced = frame->base[destination];
        assert(destination < (int)_PyFrame_Exec(frame)->nfixed ||
               destination == _PyFrame_DiscardSlot(frame) ||
               PyValue_IsNull(displaced));
        frame->base[destination] = value;
        _PyValue_CheckHome(value);
        value = PyValue_NULL;
    }
    frame->return_pc = exit;
    _PyIRStepResult observed = {exit, _PY_IR_STEP_EXIT};
    if (code->_co_monitoring != NULL) {
        observed = _PyIR_ObserveStep(
            tstate, frame, site, exit, PyValue_Borrow(PyValue_None), kind, 1);
    }
    /* POST callbacks run at the semantic operation for exception attribution.
     * Once they succeed, finalizers see its completed continuation. */
    if (observed.status >= 0) {
        frame->instr_ptr = observed.next;
    }
    PyObject *error = observed.status < 0 ? PyErr_GetRaisedException() : NULL;
    assert(observed.status >= 0 || error != NULL);
    PyValue_XCLOSE(sent);
    PyValue_XCLOSE(cursor);
    PyValue_XCLOSE(receiver);
    PyValue_XCLOSE(displaced);
    PyValue_XCLOSE(value);
    if (observed.status < 0) {
        PyErr_SetRaisedException(error);
        observed.next = frame->instr_ptr + 1;
        return observed;
    }
    observed.next = frame->instr_ptr;
    return observed;
}


PyValue
_PyEval_frame_clear_and_pop(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue result);

/* Retire the unlinked frame of a return; the caller holds the result
 * (_PyEval_FrameClearAndPopGeneralWithResult) */
static inline void
_PyRop_RetireFrame(PyThreadState *tstate, _PyInterpreterFrame *dying)
{
    if (dying->owner != FRAME_OWNED_BY_THREAD) {
        (void)_PyEval_frame_clear_and_pop(tstate, dying, PyValue_NULL);
        return;
    }
    _PyThreadState_UpdateLastProfiledFrame(tstate, dying, tstate->current_frame);
    (void)retire_thread_frame(tstate, dying, PyValue_NULL, _PyFrame_NumSlots(dying));
}

/* The same for a frame whose scratch is vacant, so that only its locals can
 * own values (_PyEval_RetireThreadFrameWithResult) */
static inline void
_PyRop_RetireFrameEmpty(PyThreadState *tstate, _PyInterpreterFrame *dying)
{
    assert(dying->owner == FRAME_OWNED_BY_THREAD);
    assert(tstate->current_frame != dying);
    assert((PyObject **)dying == tstate->datastack_limit);
    if (Py_UNLIKELY(dying->frame_obj != NULL
                    || tstate->last_profiled_frame == dying)) {
        _PyRop_RetireFrame(tstate, dying);
        return;
    }
    _PyFrame_ClearSlots(dying, (int)_PyFrame_Exec(dying)->nfixed);
    assert(dying->f_funcobj != NULL);
    Py_DECREF(dying->f_funcobj);
    Py_DECREF(dying->f_executable);
    _PyThreadState_PopFrame(tstate, dying);
}


Py_NO_INLINE PyValue
_PyEval_FrameClearAndPopGeneralWithResult(PyThreadState *tstate,
                                         _PyInterpreterFrame *frame,
                                         PyValue result);


/* Whether a call of the function creates an object (a generator, coroutine or
 * async generator) instead of running its code in a frame. */
static inline int
code_creates_generator(PyCodeObject *code)
{
    return (code->co_flags &
            (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) != 0;
}

/* What the binding of a call makes: the frame of a function, which the
 * evaluator runs, or the object that a call of a generator function creates
 * (a generator, coroutine or async generator, whose frame no thread runs).
 * Neither is an error, with the exception set. */
typedef struct {
    _PyInterpreterFrame *frame;
    PyGenObject *gen;
} _PyBound;




/* Same as eval_frame_push_and_init_code but takes an args tuple and kwargs dict.
   Steals references to func, callargs and kwargs.
*/






/* Generic calls.
 *
 * The generic CALL, CALL_KW and CALL_FUNCTION_EX, their instrumented forms
 * and the guarded calls that missed their binding plan run here, on the
 * operand helpers that the specialized handlers use.  A call reads its
 * operands once, in place: the callable is field B of the head and operand k
 * is the slot that the k-th field of the operand words names.  The values
 * the call holds across a step that can run Python are explicit: a view of a
 * bound method's function and receiver, which the method operand keeps alive,
 * the tuple of a CALL_FUNCTION_EX, which the call owns, and the references
 * that a Python callee takes.  Everything else is read from the homes again,
 * and a temporary cannot change while the call runs: Python cannot see it,
 * and a local's old value survives a write through its frame's proxy.  After
 * a step that can run Python the frame is checked and its instr_ptr is the
 * call's again (call_refresh).
 *
 * A Python callee takes its arguments from the operands (a temporary moves,
 * a local is counted) after every test, and the child is current before the
 * first release, which is the method operand's.  A native callee borrows
 * them, and the result is published first (call_publish).  An error
 * leaves the operands that the call did not move in their homes, which the
 * unwinder clears. */

/* The operand of the call at head that is its first argument: the self
 * operand's slot is NULL at run time for a class attribute that is not a
 * method, and the arguments follow it. */
static inline Py_ALWAYS_INLINE int
call_skipped_operands(const PyValue *slots, const _PyIRWord *head,
                      _PyIRWord word)
{
    return (word & IR_F_NO_SELF) == 0 &&
        PyValue_IsNull(slots[_PyIR_CallOperand(head, 0)]);
}

/* The callable of a call and its receiver.  A bound method without a self
 * operand is its function and its receiver, which go before the arguments:
 * views of the method, which its operand keeps alive until the call is
 * published or has pushed its frame, and which the call counts if it keeps
 * them.  Otherwise the receiver is NULL. */
static inline Py_ALWAYS_INLINE PyValue
call_expand_method(PyValue callable, int flags, PyValue *receiver)
{
    *receiver = PyValue_NULL;
    if ((flags & CALL_SELF_PRESENT) ||
        PyValue_TYPE(callable) != &PyMethod_Type) {
        return callable;
    }
    PyMethodObject *method = (PyMethodObject *)PyValue_AsKnownObjectBorrow(callable);
    *receiver = method->im_self;
    return _PyIR_AdoptTemp(PyValue_FromObjectBorrow(method->im_func));
}

/* The first argument that a CALL event reports, as a view: the receiver or
 * the self operand, else the first argument.  NULL if the call has none. */
static inline PyValue
call_first_argument(const PyValue *slots, const _PyIRWord *head,
                    PyValue receiver, int keywords)
{
    if (!PyValue_IsNull(receiver)) {
        return receiver;
    }
    _PyIRWord word = *head;
    int skip = call_skipped_operands(slots, head, word);
    if ((int)IR_C(word) - keywords <= skip) {
        return PyValue_NULL;
    }
    return slots[_PyIR_CallOperand(head, skip)];
}

_Py_COLD_FUNCTION int
_PyEval_call_monitor(PyThreadState *tstate, _PyInterpreterFrame *frame,
             _PyInstr *head, PyValue callable, PyValue first);


/* Whether callable is a plain Python function that a call may push a frame
 * for: PEP 523 is not hooked and it runs through _PyFunction_Vectorcall. */
static inline Py_ALWAYS_INLINE int
call_is_python_function(PyThreadState *tstate, PyValue callable)
{
    if (!PyValue_IsObject(callable) || IS_PEP523_HOOKED(tstate)) {
        return 0;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(callable);
    return Py_TYPE(object) == &PyFunction_Type &&
        ((PyFunctionObject *)object)->vectorcall == _PyFunction_Vectorcall;
}

/* The child frame is complete and the caller's operands are in it: make it
 * the call's callee.  Nothing may fail or run Python between the first
 * write and the last. */
static inline Py_ALWAYS_INLINE void
call_commit(PyThreadState *tstate, _PyInterpreterFrame *frame,
            _PyInstr *head, _PyInterpreterFrame *child)
{
    assert(child->previous == frame);
    assert(child->ir_words == _PyFrame_GetCode(child)->_co_exec->words);
    frame->return_slot = (uint16_t)IR_A(*head);
    frame->return_pc = call_return_pc(head);
    tstate->current_frame = child;
}

/* The most arguments that fill a *args tuple, and the most keyword-only
 * parameters, that the binding below handles: it holds their values in
 * arrays. */
#define CALL_BIND_EXTRAS_MAX _Py_CALL_BIND_EXTRAS_MAX
#define CALL_BIND_KWONLY_MAX _Py_CALL_BIND_SLOTS_MAX

Py_NO_INLINE void
_PyEval_call_bind_discard(PyValue *kwdefaults, unsigned int kwonly_mask,
                  PyObject *extras, PyObject *kwargs);

Py_NO_INLINE int
_PyEval_call_bind_kwdefaults(PyFunctionObject *func, PyCodeObject *code,
                     unsigned int kwonly_mask, PyValue *kwdefaults);

Py_NO_INLINE PyObject *
_PyEval_call_bind_extras(PyValue *slots, _PyInstr *head, PyObject *tuple, int skip,
                 int filled, int lead, int extras);

Py_NO_INLINE PyObject *
_PyEval_call_bind_kwargs(_PyInterpreterFrame *frame, PyValue *slots, _PyInstr *head,
                 _PyIRWord word, int skip, int lead, int positional,
                 int kwcount, const uint16_t *destinations);

Py_NO_INLINE void
_PyEval_call_bind_take_rest(PyValue *slots, _PyInstr *head, int skip, int lead,
                    int filled, int extras, int positional, int kwcount,
                    const uint16_t *destinations, PyValue *base);

/* The items of the **kwargs dict of a CALL_FUNCTION_EX, bound by name:
 * dests[i] is the slot that item i fills (a positional-or-keyword or
 * keyword-only parameter), or _Py_CALL_BIND_TO_KWARGS for a name that no
 * parameter has, which the callee's **kwargs takes.  The items are views of
 * the dict, which the call's operand keeps alive and unchanged until the
 * frame is complete. */
#define CALL_BIND_DICT_MAX 16

typedef struct {
    int count;
    uint16_t dests[CALL_BIND_DICT_MAX];
    PyValueBorrow keys[CALL_BIND_DICT_MAX];
    PyValueBorrow values[CALL_BIND_DICT_MAX];
} _CallDict;


Py_NO_INLINE PyObject *
_PyEval_call_bind_dict_kwargs(const _CallDict *items);

Py_NO_INLINE _PyCallResult
_PyEval_call_publish_object(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    _PyInstr *head, _PyIRWord word, PyGenObject *gen,
                    int operands);

/* Push the frame of the Python function callable, and fill it as the
 * evaluator binds a call, from the positional arguments (the receiver of an
 * expanded bound method, then the operands of the call at head after the
 * self operand that was NULL, or the items of tuple for a CALL_FUNCTION_EX)
 * and, for a CALL_KW, the keywords, which the plan maps to parameters: the
 * call needs no per-call search of names.  The plan (pycore_feedback.h)
 * says the rest: which parameters take defaults, which keyword-only
 * parameters take their __kwdefaults__ entry, and whether *args and
 * **kwargs are empty, filled or pending.  The call reads only what the plan
 * leaves to the function: the length of __defaults__ and the
 * __kwdefaults__ dict.  The caller has shown that callable is a Python
 * function that a frame may be pushed for (PEP 523 is not hooked, the
 * function version or call_is_python_function()).
 *
 * Every test and every allocation precedes the first move, so on a miss
 * (status 1) nothing happened and the operands are as they were: the caller
 * runs the call generically, which binds anything else and reports each
 * error with its message.  A hit leaves the child current and complete.  An
 * allocation failure is an error (status -1) with the operands untouched.
 * receiver, if it is not NULL, is the self of an expanded bound method:
 * callable is its function, and the method operand is released once the
 * child is current.  A tuple call leaves the operands and the tuple to the
 * caller.
 *
 * A call of a generator function (_Py_CALL_BIND_GENERATOR) creates the
 * generator with _PyGen_NewCall, the one creation routine, binds the
 * arguments into its frame and runs gen_prologue; the call's result is the
 * generator, published by _PyEval_call_publish_object as the generic path publishes
 * it, and the status is 0 with no child. */
static inline Py_ALWAYS_INLINE _PyCallResult
call_bind_core(PyThreadState *tstate, _PyInterpreterFrame *frame,
               _PyInstr *head, PyValue callable, PyValue receiver,
               const _PyBindPlan *plan, PyObject *tuple,
               const _CallDict *items, int simple)
{
#ifdef Py_GIL_DISABLED
    return (_PyCallResult){1, NULL};
#else
    int bits = plan->flags;
    if (Py_UNLIKELY(!(bits & _Py_CALL_BIND_VALID))) {
        return (_PyCallResult){1, NULL};
    }
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*head);
    PyValue *slots = _PyFrame_Slots(frame);
    PyFunctionObject *func =
        (PyFunctionObject *)_PyValue_AsKnownObjectBorrowPointer(callable);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int lead = !PyValue_IsNull(receiver);
    int skip = 0;
    if (tuple == NULL) {
        skip = call_skipped_operands(slots, head, word);
        if (Py_UNLIKELY(skip != !!(bits & _Py_CALL_BIND_SKIP))) {
            return (_PyCallResult){1, NULL};
        }
    }
    /* A simple plan (not _Py_CALL_BIND_GENERAL) needs no *args tuple, no
     * **kwargs dict, no keyword-only default and no generator: the binding
     * of the common call is the straight line of this function. */
    int generator = !simple && (bits & _Py_CALL_BIND_GENERATOR) != 0;
    if (!simple && tuple != NULL && generator) {
        /* The caller releases a tuple call's operands once it has a child,
         * which a generator call does not have. */
        return (_PyCallResult){1, NULL};
    }
    if (Py_UNLIKELY(!generator &&
        !_PyThreadState_HasStackSpace(tstate, code->co_framesize))) {
        return (_PyCallResult){1, NULL};
    }
    int argcount = code->co_argcount;
    int kwcount = plan->keywords;
    int positional = plan->positional;
    int filled = plan->filled;
    int extras = simple ? 0 : positional - filled;
    const uint16_t *destinations = items != NULL ? items->dests : plan->keyword_dest;
    /* The positional parameters that no argument and no keyword fills take a
     * default: the function's own, which a function of the same code may
     * have changed, so its length is tested: it covers need slots. */
    PyObject *defaults = func->func_defaults;
    Py_ssize_t ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    int need = plan->need;
    if (Py_UNLIKELY((size_t)(ndefaults - need) > (size_t)(argcount - need))) {
        return (_PyCallResult){1, NULL};
    }
    unsigned int kwonly_mask = simple ? 0 : plan->kwonly_mask;
    PyValue kwdefaults[CALL_BIND_KWONLY_MAX];
    if (!simple && kwonly_mask &&
        !_PyEval_call_bind_kwdefaults(func, code, kwonly_mask, kwdefaults)) {
        return (_PyCallResult){1, NULL};
    }
    /* The *args tuple and the **kwargs dictionary hold counts of their own
     * (the operands stay in their homes until the moves below); creating
     * them can fail and schedules the GC, which does not run Python before
     * the complete child is published. */
    PyObject *extras_tuple = NULL;
    PyObject *kwargs = NULL;
    if (!simple) {
        if (extras > 0) {
            extras_tuple = _PyEval_call_bind_extras(slots, head, tuple, skip, filled,
                                            lead, extras);
            if (extras_tuple == NULL) {
                _PyEval_call_bind_discard(kwdefaults, kwonly_mask, NULL, NULL);
                return (_PyCallResult){-1, NULL};
            }
        }
        if (bits & _Py_CALL_BIND_NEW_KWARGS) {
            kwargs = items != NULL
                ? _PyEval_call_bind_dict_kwargs(items)
                : _PyEval_call_bind_kwargs(frame, slots, head, word, skip, lead,
                                   positional, kwcount, destinations);
            if (kwargs == NULL) {
                _PyEval_call_bind_discard(kwdefaults, kwonly_mask, extras_tuple, NULL);
                return (_PyCallResult){-1, NULL};
            }
        }
    }

    /* All tests precede this transfer. The target is complete before it
     * becomes current: nothing fails, calls back or releases. */
    PyValue function;
    if (tuple != NULL || lead || (word & IR_F_LOCAL_B)) {
        function = PyValue_DUP(callable);
    }
    else {
        function = callable;
        slots[IR_B(word)] = PyValue_NULL;
    }
    _PyInterpreterFrame *target;
    PyGenObject *gen = NULL;
    if (!simple && generator) {
        gen = _PyGen_NewCall(tstate, function, NULL, code, frame);
        if (gen == NULL) {
            _PyEval_call_bind_discard(kwdefaults, kwonly_mask, extras_tuple, kwargs);
            return (_PyCallResult){-1, NULL};
        }
        target = &gen->gi_iframe;
    }
    else {
        target = _PyFrame_PushUnchecked(tstate, function, frame);
    }
    PyValue *base = target->base;
    PyValue *arguments = base;
    if (lead) {
        *arguments++ = PyValue_DUP(receiver);
    }
    if (tuple != NULL) {
        for (int i = 0; i < filled - lead; i++) {
            arguments[i] = PyValue_DUP(_PyTuple_ITEMS(tuple)[i]);
        }
        if (items != NULL) {
            for (int i = 0; i < kwcount; i++) {
                if (destinations[i] != _Py_CALL_BIND_TO_KWARGS) {
                    base[destinations[i]] = PyValue_DUP(items->values[i]);
                }
            }
        }
    }
    else {
        call_take_operands(slots, head, skip, filled - lead, arguments);
        if (kwcount > 0 || extras > 0) {
            _PyEval_call_bind_take_rest(slots, head, skip, lead, filled, extras,
                                positional, kwcount, destinations, base);
        }
    }
    int first_default = argcount - (int)ndefaults;
    for (unsigned int mask = plan->default_mask; mask; mask &= mask - 1) {
        int slot = __builtin_ctz(mask);
        base[slot] = PyValue_DUP(_PyTuple_ITEMS(defaults)[slot - first_default]);
    }
    if (!simple) {
        for (unsigned int mask = kwonly_mask; mask; mask &= mask - 1) {
            int j = __builtin_ctz(mask);
            base[argcount + j] = kwdefaults[j];
        }
    }
    if (!simple) {
        int slot = argcount + code->co_kwonlyargcount;
        if (bits & _Py_CALL_BIND_VARARGS) {
            base[slot++] = extras_tuple != NULL
                ? PyValue_FromObjectSteal(extras_tuple)
                : (PyValue)_PyValue_STATIC_IMMORTAL(&_Py_SINGLETON(tuple_empty));
        }
        if (kwargs != NULL) {
            base[slot] = PyValue_FromObjectSteal(kwargs);
        }
    }
    if (bits & _Py_CALL_BIND_VIRTUAL_KWARGS) {
        /* A simple plan has no *args (which makes it general) and is never
         * new: the slot follows the keyword-only parameters */
        target->pending_kwargs = (uint16_t)(argcount + code->co_kwonlyargcount +
            ((bits & _Py_CALL_BIND_VARARGS) != 0));
    }
    if (kwcount) {
        CALL_STAT_INC(keyword_plan_hits);
    }
    if (!simple && generator) {
        if (gen_prologue(tstate, gen, code) < 0) {
            _PyGen_DiscardCall(gen);
            return (_PyCallResult){-1, NULL};
        }
        return _PyEval_call_publish_object(tstate, frame, head, word, gen,
                                   (int)IR_C(word) - (kwcount != 0));
    }
    call_commit(tstate, frame, head, target);
    if (lead && tuple == NULL) {
        /* The method operand has kept the function and the receiver alive;
         * releasing it can run a weakref callback or a finalizer, with the
         * child current and not started.  A tuple call's caller releases
         * its operands. */
        call_release_callable(frame, word);
    }
    return (_PyCallResult){0, target};
#endif
}



Py_NO_INLINE _PyCallResult
_PyEval_call_bind_positional(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *head, PyValue callable, PyValue receiver,
                     const _PyBindPlan *plan);


Py_NO_INLINE _PyCallResult
_PyEval_call_push_python(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *head, PyValue callable, PyValue receiver,
                 int keywords);

Py_NO_INLINE _PyCallResult
_PyEval_call_run_instrumented_result(PyThreadState *tstate, _PyInterpreterFrame *frame,
                             _PyInstr *head, _PyIRWord word, PyValue callable,
                             PyValue first, PyValue result, int flags,
                             int keywords);

/* The generic CALL and CALL_KW: their words, instrumented or not, and the
 * guarded calls that missed.  Returns {-1, NULL} on error, {0, child} for a
 * pushed frame and {0, NULL} for a finished native call.  The handler keeps
 * what it did before the call in flags (CALL_SELF_PRESENT for a self operand
 * that is not NULL, CALL_INSTRUMENTED and CALL_PERIODIC for what the handler
 * is). */
static inline Py_ALWAYS_INLINE _PyCallResult
call_run(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *head,
         int flags, int keywords)
{
    _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(*head);
    PyValue *slots = _PyFrame_Slots(frame);
    PyValue receiver;
    PyValue callable = call_expand_method(slots[IR_B(word)], flags, &receiver);
    int instrumented = (flags & CALL_INSTRUMENTED) != 0;
    PyValue first = PyValue_NULL;
    if (Py_UNLIKELY(instrumented)) {
        first = call_first_argument(slots, head, receiver, keywords);
        if (_PyEval_call_monitor(tstate, frame, head, callable, first) < 0) {
            return (_PyCallResult){-1, NULL};
        }
    }
    if (call_is_python_function(tstate, callable)) {
        /* A keyword call has no plan here (no site's keyword destinations):
         * the general binding takes it */
        if (!keywords) {
            _PyBindPlan plan;
            int skip = call_skipped_operands(slots, head, word);
            PyFunctionObject *func =
                (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
            if (_PyBindPlan_ForCall(&plan, (PyCodeObject *)func->func_code,
                                    !PyValue_IsNull(receiver) + (int)IR_C(word) - skip,
                                    skip, !PyValue_IsNull(receiver)))
            {
                _PyCallResult pushed = _PyEval_call_bind_positional(
                    tstate, frame, head, callable, receiver, &plan);
                if (pushed.status <= 0) {
                    return pushed;
                }
            }
        }
        return _PyEval_call_push_python(tstate, frame, head, callable, receiver,
                                keywords);
    }
    PyValue result = call_vector_impl(tstate, frame, head, callable, receiver,
                                      keywords);
    if (Py_UNLIKELY(instrumented)) {
        return _PyEval_call_run_instrumented_result(tstate, frame, head, word,
                                            callable, first, result, flags,
                                            keywords);
    }
    /* The callee may have run Python, but nothing that the rest of the
     * call reads can have changed.  A native callee returns on the C
     * stack above this activation, so this frame is the current one;
     * the executing copy of the code, and so the head, is fixed for the
     * life of the code object (register-ir.md 1.10), as value_operation_
     * refresh() asserts; and the frame's slots stay where they are.  The
     * temporaries are owned by their homes and a local that the callee
     * rebound keeps its old value alive, so publishing reads nothing
     * stale.  A finalizer that the release runs is in the same position.
     * Only an event's callback (_PyEval_call_run_instrumented_result) reads the
     * frame after the call. */
    assert(tstate->current_frame == frame);
    assert(_PyFrame_GetCode(frame)->_co_exec != NULL &&
           frame->ir_words == _PyFrame_GetCode(frame)->_co_exec->words);
    if (PyValue_IsNull(result)) {
        return (_PyCallResult){-1, NULL};
    }
    /* The keyword names operand of CALL_KW is not a slot. */
    PyValue displaced = call_publish(frame, head, word, result,
                                     (int)IR_C(word) - keywords);
    PyValue_XCLOSE(displaced);
    if ((flags & CALL_PERIODIC) && check_periodics_at_end(tstate, frame) < 0) {
        return (_PyCallResult){-1, NULL};
    }
    return (_PyCallResult){0, NULL};
}

/* Whether the call whose head is head is a CALL_KW: its last operand is the
 * constant tuple of the keyword names, which is no slot */
static inline int
rop_call_keywords(const _PyIRWord *head)
{
    return IR_IS_KW_WORD(head[0]);
}

/* The rows of Call.Generic (Python/register_ops/calls.rop) on call_run's
 * paths.  A generator function takes the native path: vectorcall creates the
 * generator. */

static inline bool
_PyRop_PythonTarget(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue *fn, const _PyIRWord *head)
{
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    if (!call_is_python_function(tstate, callable)) {
        return false;
    }
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
    return !code_creates_generator((PyCodeObject *)func->func_code);
}

Py_NO_INLINE _PyInterpreterFrame *
_PyRop_PushPython(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue *fn, _PyInstr *head);

static inline PyValue
_PyRop_CallNative(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue *fn, const _PyIRWord *head)
{
    assert(fn == &_PyFrame_Slots(frame)[IR_B(head[0])]);
    PyValue receiver;
    PyValue callable = call_expand_method(*fn, rop_call_self_state(frame, head),
                                          &receiver);
    return call_vector_impl(tstate, frame, head, callable, receiver,
                            rop_call_keywords(head));
}

/* The rows of the native specializations of Call (Python/register_ops/calls.rop,
 * Call.Type1 to Call.DescriptorNoargs), the guards and calls of golden87's
 * CALL_TYPE_1 to CALL_METHOD_DESCRIPTOR_FAST.  A native arm runs where the
 * call's operands stand: an operand is read through its home, and the arm's
 * publication releases the temporaries. */

/* The first operand that the callee receives (IR_CALL_FIRST) */
static inline int
rop_call_first(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    return rop_call_self_state(frame, head) ? 0 : _PyIR_CallHasSelf(head[0]);
}

/* The operand that the callee receives as its argument k */
static inline PyValue
rop_call_arg(_PyInterpreterFrame *frame, const _PyIRWord *head, int k)
{
    return _PyFrame_Slots(frame)[_PyIR_CallOperand(head, rop_call_first(frame, head) + k)];
}

/* The arguments that the call gives, as golden87's oparg counted them: the
 * operands, less a self slot */
static inline int
rop_call_oparg(const _PyIRWord *head)
{
    return (int)IR_C(head[0]) - _PyIR_CallHasSelf(head[0]);
}

static inline bool
_PyRop_SelfPresent(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    return rop_call_self_state(frame, head) != 0;
}

static inline bool
_PyRop_RecursionClear(PyThreadState *tstate)
{
    return !_Py_ReachedRecursionLimit(tstate);
}

static inline bool
_PyRop_CalleeIsType(PyValueBorrow fn)
{
    return _PyValue_NamesObject(fn, (PyObject *)&PyType_Type);
}

static inline bool
_PyRop_CalleeIsStr(PyValueBorrow fn)
{
    return _PyValue_NamesObject(fn, (PyObject *)&PyUnicode_Type);
}

static inline bool
_PyRop_CalleeIsTuple(PyValueBorrow fn)
{
    return _PyValue_NamesObject(fn, (PyObject *)&PyTuple_Type);
}

static inline bool
_PyRop_CalleeIsLen(PyThreadState *tstate, PyValueBorrow fn)
{
    return _PyValue_NamesObject(fn, tstate->interp->callable_cache.len);
}

static inline bool
_PyRop_CalleeIsIsinstance(PyThreadState *tstate, PyValueBorrow fn)
{
    return _PyValue_NamesObject(fn, tstate->interp->callable_cache.isinstance);
}

static inline bool
_PyRop_CalleeIsListAppend(PyThreadState *tstate, PyValueBorrow fn)
{
    return _PyValue_NamesObject(fn, tstate->interp->callable_cache.list_append);
}

static inline bool
_PyRop_BuiltinClass(PyValueBorrow fn)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(fn);
    return PyType_Check(callable) &&
        ((PyTypeObject *)callable)->tp_vectorcall != NULL;
}

static inline bool
_PyRop_BuiltinOCallable(_PyInterpreterFrame *frame, PyValueBorrow fn,
                        const _PyIRWord *head)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(fn);
    return PyCFunction_CheckExact(callable) &&
        (PyCFunction_GET_FLAGS(callable) & _Py_METH_CALL_FLAGS) == METH_O &&
        rop_call_argcount(frame, head) == 1;
}

static inline bool
_PyRop_BuiltinFastCallable(_PyInterpreterFrame *frame, PyValueBorrow fn,
                           const _PyIRWord *head)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(fn);
    if (!PyCFunction_CheckExact(callable)) {
        return false;
    }
    int flags = PyCFunction_GET_FLAGS(callable) & _Py_METH_CALL_FLAGS;
    // A METH_VALUE function (alone or with METH_NOARGS or METH_O) reaches the
    // same value vectorcall as METH_FASTCALL; with METH_KEYWORDS it takes
    // Call.BuiltinFastKeywords
    if (flags != METH_FASTCALL && (flags & ~(METH_NOARGS | METH_O)) != METH_VALUE) {
        return false;
    }
    int n = (int)IR_C(head[0]) - rop_call_first(frame, head);
    return !((flags & (METH_O | METH_NOARGS)) && n != ((flags & METH_O) ? 1 : 0));
}

static inline bool
_PyRop_BuiltinFastKeywordsCallable(PyValueBorrow fn)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(fn);
    if (!PyCFunction_CheckExact(callable)) {
        return false;
    }
    int flags = PyCFunction_GET_FLAGS(callable) & _Py_METH_CALL_FLAGS;
    return flags == (METH_FASTCALL | METH_KEYWORDS) ||
           flags == (METH_VALUE | METH_KEYWORDS);
}

/* The descriptor's receiver is an exact instance of its type */
static inline bool
rop_descriptor_receiver(_PyInterpreterFrame *frame, const _PyIRWord *head,
                        PyMethodDescrObject *method)
{
    return PyValue_TYPE(rop_call_arg(frame, head, 0)) == method->d_common.d_type;
}

static inline bool
_PyRop_DescriptorOCallable(_PyInterpreterFrame *frame, PyValueBorrow fn,
                           const _PyIRWord *head)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyMethodDescrObject *method =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(fn);
    return Py_IS_TYPE(method, &PyMethodDescr_Type) &&
        (method->d_method->ml_flags & _Py_METH_CALL_FLAGS) == METH_O &&
        rop_call_argcount(frame, head) == 2 &&
        rop_descriptor_receiver(frame, head, method);
}

static inline bool
_PyRop_DescriptorNoargsCallable(_PyInterpreterFrame *frame, PyValueBorrow fn,
                                const _PyIRWord *head)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyMethodDescrObject *method =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(fn);
    return Py_IS_TYPE(method, &PyMethodDescr_Type) &&
        (method->d_method->ml_flags & _Py_METH_CALL_FLAGS) == METH_NOARGS &&
        rop_call_argcount(frame, head) == 1 &&
        rop_descriptor_receiver(frame, head, method);
}

static inline bool
_PyRop_DescriptorFastCallable(_PyInterpreterFrame *frame, PyValueBorrow fn,
                              const _PyIRWord *head)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyMethodDescrObject *method =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(fn);
    if (!Py_IS_TYPE(method, &PyMethodDescr_Type)) {
        return false;
    }
    int flags = method->d_method->ml_flags & _Py_METH_CALL_FLAGS;
    if (flags != METH_FASTCALL && (flags & ~(METH_NOARGS | METH_O)) != METH_VALUE) {
        return false;
    }
    int n = (int)IR_C(head[0]) - rop_call_first(frame, head);
    // The call has a receiver, and the value form's fixed arity fits
    if (n == 0 ||
        ((flags & (METH_O | METH_NOARGS)) && n != ((flags & METH_O) ? 2 : 1))) {
        return false;
    }
    return rop_descriptor_receiver(frame, head, method);
}

static inline bool
_PyRop_DescriptorFastKeywordsCallable(_PyInterpreterFrame *frame,
                                      PyValueBorrow fn, const _PyIRWord *head)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyMethodDescrObject *method =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(fn);
    if (!Py_IS_TYPE(method, &PyMethodDescr_Type)) {
        return false;
    }
    int flags = method->d_method->ml_flags & _Py_METH_CALL_FLAGS;
    if (flags != (METH_FASTCALL | METH_KEYWORDS) &&
        flags != (METH_VALUE | METH_KEYWORDS)) {
        return false;
    }
    return (int)IR_C(head[0]) - rop_call_first(frame, head) != 0 &&
        rop_descriptor_receiver(frame, head, method);
}

/* list.append(x): the receiver is operand 0, an exact list */
static inline bool
_PyRop_ListReceiver(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyValue self = _PyFrame_Slots(frame)[_PyIR_CallOperand(head, 0)];
    if (!_PyRop_IsObject(self) ||
        !PyList_CheckExact(PyValue_AsKnownObjectBorrow(self))) {
        return false;
    }
    return true;
}

/* The rows of the frame-pushing attribute arms (Python/register_ops/getattr.rop):
 * the function is the recipe's class value, its version the recipe's aux word */
extern PyObject *_PyAttr_FrameTarget(PyObject *owner, PyObject *name,
                                     bool *getattribute);
extern bool _PyAttr_FrameTargetPossible(PyObject *owner, PyObject *name);

static inline bool
_PyAttr_FrameReady(PyThreadState *tstate, _PyAttrRecipe r)
{
    PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(
        _PyValueGuarded_LoadObject(r.class_value));
    return func->func_version == r.aux &&
        _PyThreadState_HasStackSpace(
            tstate, ((PyCodeObject *)func->func_code)->co_framesize);
}

/* Push the frame of func for the load at site: the owner moves from its
 * home (a local is counted) into the first slot, and the name, when the
 * function takes one, into the second */
static inline _PyInterpreterFrame *
_PyAttr_PushFunction(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     const _PyIRWord *site, PyValue *home, PyObject *func,
                     PyObject *name)
{
    PyValue owner = *home;
    if (site[0] & IR_F_LOCAL_B) {
        owner = PyValue_DUP(owner);
    }
    else {
        *home = PyValue_NULL;
    }
    _PyInterpreterFrame *pushed = _PyFrame_PushUnchecked(
        tstate, PyValue_DUP(PyValue_FromObjectBorrow(func)), frame);
    pushed->base[0] = owner;
    if (name != NULL) {
        pushed->base[1] = PyValue_DUP(PyValue_FromObjectBorrow(name));
    }
    return pushed;
}

static inline _PyInterpreterFrame *
_PyAttr_PushProperty(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     const _PyIRWord *site, PyValue *owner, _PyAttrRecipe r)
{
    return _PyAttr_PushFunction(
        tstate, frame, site, owner,
        PyValue_AsKnownObjectBorrow(_PyValueGuarded_LoadObject(r.class_value)),
        NULL);
}

static inline _PyInterpreterFrame *
_PyAttr_PushGetattribute(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         const _PyIRWord *site, PyValue *owner, PyObject *name,
                         _PyAttrRecipe r)
{
    return _PyAttr_PushFunction(
        tstate, frame, site, owner,
        PyValue_AsKnownObjectBorrow(_PyValueGuarded_LoadObject(r.class_value)),
        name);
}

/* What the generic load runs in a frame of its own */
static inline bool
_PyAttr_FrameClaimable(PyThreadState *tstate, PyValueBorrow owner, PyObject *name)
{
    if (!_PyRop_IsObject(owner) || IS_PEP523_HOOKED(tstate) ||
        !_PyRop_RecursionBudget(tstate))
    {
        return false;
    }
    PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
    bool getattribute;
    if (!_PyAttr_FrameTargetPossible(owner_o, name)) {
        // The full analysis is for the sites that can have a target
        assert(_PyAttr_FrameTarget(owner_o, name, &getattribute) == NULL);
        return false;
    }
    PyObject *func = _PyAttr_FrameTarget(owner_o, name, &getattribute);
    return func != NULL && _PyThreadState_HasStackSpace(
        tstate, ((PyCodeObject *)((PyFunctionObject *)func)->func_code)->co_framesize);
}

static inline _PyInterpreterFrame *
_PyAttr_PushFrame(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  const _PyIRWord *site, PyValue *owner, PyObject *name)
{
    bool getattribute;
    PyObject *func = _PyAttr_FrameTarget(PyValue_AsKnownObjectBorrow(*owner), name,
                                         &getattribute);
    assert(func != NULL);
    return _PyAttr_PushFunction(tstate, frame, site, owner, func,
                                getattribute ? name : NULL);
}

/* The rows of GetItem.Getitem (Python/register_ops/items.rop): the cached
 * __getitem__ of a heap type, with the version it was cached under */
static inline PyFunctionObject *
_PyRop_CachedGetItem(PyValueBorrow owner)
{
    if (!PyValue_IsObject(owner)) {
        return NULL;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(owner));
    if (!(tp->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        return NULL;
    }
    PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
    PyFunctionObject *getitem = (PyFunctionObject *)
        FT_ATOMIC_LOAD_PTR_ACQUIRE(ht->_spec_cache.getitem);
    if (getitem == NULL ||
        getitem->func_version !=
            FT_ATOMIC_LOAD_UINT32_RELAXED(ht->_spec_cache.getitem_version))
    {
        return NULL;
    }
    return getitem;
}

static inline bool
_PyRop_GetItemFrameReady(PyThreadState *tstate, PyValueBorrow owner)
{
    PyFunctionObject *getitem = _PyRop_CachedGetItem(owner);
    return getitem != NULL && _PyThreadState_HasStackSpace(
        tstate, ((PyCodeObject *)getitem->func_code)->co_framesize);
}

static inline bool
_PyRop_GetItemFrameClaimable(PyThreadState *tstate, PyValueBorrow owner)
{
    return !IS_PEP523_HOOKED(tstate) && _PyRop_RecursionBudget(tstate) &&
        _PyRop_GetItemFrameReady(tstate, owner);
}

static inline _PyInterpreterFrame *
_PyRop_PushGetItem(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   const _PyIRWord *site, PyValue *container, PyValue *sub)
{
    PyFunctionObject *getitem = _PyRop_CachedGetItem(PyValue_Borrow(*container));
    assert(getitem != NULL);
    _PyInterpreterFrame *pushed = _PyFrame_PushUnchecked(
        tstate, PyValue_DUP(PyValue_FromObjectBorrow((PyObject *)getitem)), frame);
    _PyIRWord word = site[0];
    PyValue owner = *container;
    if (word & IR_F_LOCAL_B) {
        owner = PyValue_DUP(owner);
    }
    else {
        *container = PyValue_NULL;
    }
    PyValue key = *sub;
    if (word & IR_F_LOCAL_C) {
        key = PyValue_DUP(key);
    }
    else {
        *sub = PyValue_NULL;
    }
    pushed->base[0] = owner;
    pushed->base[1] = key;
    return pushed;
}

static inline PyValue
_PyRop_CallType1(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyValue arg = rop_call_arg(frame, head, 0);
    // A type is never an exact int or float, so it has no inline form
    return PyValue_DUP(_PyValue_PointerBorrow((PyObject *)PyValue_TYPE(arg)));
}

static inline PyValue
_PyRop_CallStr1(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyObject *result = _PyValue_Str(rop_call_arg(frame, head, 0));
    return result == NULL ? PyValue_NULL : PyValue_FromObjectSteal(result);
}

static inline PyValue
_PyRop_CallTuple1(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyObject *result = _PyValue_SequenceTuple(rop_call_arg(frame, head, 0));
    return result == NULL ? PyValue_NULL : PyValue_FromObjectSteal(result);
}

static inline PyValue
_PyRop_CallLen(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    Py_ssize_t length = _PyValue_Size(rop_call_arg(frame, head, 0));
    if (length < 0) {
        return PyValue_NULL;
    }
    // The length is inline when it fits in int51
    return PyValue_FromInt64(length);
}

static inline PyValue
_PyRop_CallIsinstance(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyValue instance = rop_call_arg(frame, head, 0);
    PyValue cls = rop_call_arg(frame, head, 1);
    // An instance of cls itself is the common answer
    int retval = 1;
    if (!_PyValue_NamesObject(cls, (PyObject *)PyValue_TYPE(instance))) {
        retval = _PyValue_IsInstance(instance, cls);
    }
    if (retval < 0) {
        return PyValue_NULL;
    }
    return retval ? PyValue_True : PyValue_False;
}

static inline PyValue
_PyRop_CallListAppend(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    PyObject *self_o = PyValue_AsKnownObjectBorrow(
        _PyFrame_Slots(frame)[_PyIR_CallOperand(head, 0)]);
    // The list takes a count of its own; the operand stays the frame's until
    // the call publishes its result
    PyValue item = PyValue_DUP(_PyFrame_Slots(frame)[_PyIR_CallOperand(head, 1)]);
    int err;
    // The list's lock is free in the GIL build
    Py_BEGIN_CRITICAL_SECTION(self_o);
    err = _PyList_AppendTakeValue((PyListObject *)self_o, item);
    Py_END_CRITICAL_SECTION();
    return err ? PyValue_NULL : PyValue_None;
}

Py_NO_INLINE PyValue
_PyRop_CallBuiltinO(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue *fn, const _PyIRWord *head);

static inline PyValue
_PyRop_CallBuiltinValues(PyThreadState *tstate, _PyInterpreterFrame *frame,
                         PyValue *fn, const _PyIRWord *head)
{
    return call_builtin_values(tstate, frame, head, head[0], *fn,
                               rop_call_first(frame, head));
}

static inline PyValue
_PyRop_CallClassValues(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       PyValue *fn, const _PyIRWord *head)
{
    return call_class_values(tstate, frame, head, head[0], *fn,
                             rop_call_first(frame, head));
}

static inline PyValue
_PyRop_CallDescriptorValues(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyValue *fn, const _PyIRWord *head)
{
    return call_descriptor_values(tstate, frame, head, head[0], *fn,
                                  rop_call_first(frame, head));
}

static inline PyValue
_PyRop_CallDescriptorO(PyThreadState *tstate, _PyInterpreterFrame *frame,
                       PyValue *fn, const _PyIRWord *head)
{
    PyMethodDescrObject *method =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(*fn);
    // ml_meth takes objects: the helper boxes numbers.  The receiver and the
    // argument are operands 0 and 1.
    return _PyEval_CallMethodDescriptorO(
        tstate, method->d_method->ml_meth, rop_call_arg(frame, head, 0),
        rop_call_arg(frame, head, 1));
}

static inline PyValue
_PyRop_CallDescriptorNoargs(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            PyValue *fn, const _PyIRWord *head)
{
    PyMethodDescrObject *method =
        (PyMethodDescrObject *)PyValue_AsKnownObjectBorrow(*fn);
    return _PyEval_CallMethodDescriptorNoArgs(
        tstate, method->d_method->ml_meth, rop_call_arg(frame, head, 0));
}

/* The row of the generator-creating plan arms (Python/register_ops/calls.rop):
 * plan_fits is every test that call_bind_core makes before it changes
 * anything, so that plan_generator has no decline: the plan is valid for this
 * call's self slot, the frame fits the arena, the function's own defaults
 * cover what the plan leaves to them, and its keyword-only defaults are
 * there.  (push_plan takes the binder's own decline instead.) */
static inline bool
_PyRop_PlanFits(PyThreadState *tstate, _PyInterpreterFrame *frame,
                PyValueBorrow fn, const _PyIRWord *head, const _PyFbCall *fb)
{
#ifdef Py_GIL_DISABLED
    return false;
#else
    const _PyBindPlan *plan = &fb->plan;
    int bits = plan->flags;
    if (!(bits & _Py_CALL_BIND_VALID)) {
        return false;
    }
    _PyIRWord word = head[0];
    PyFunctionObject *func =
        (PyFunctionObject *)PyValue_AsKnownObjectBorrow(fn);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    if (call_skipped_operands(_PyFrame_Slots(frame), head, word) !=
        !!(bits & _Py_CALL_BIND_SKIP)) {
        return false;
    }
    /* A generator's frame lives in its object, not in the arena */
    if (!(bits & _Py_CALL_BIND_GENERATOR) &&
        !_PyThreadState_HasStackSpace(tstate, code->co_framesize)) {
        return false;
    }
    int argcount = code->co_argcount;
    int need = plan->need;
    PyObject *defaults = func->func_defaults;
    Py_ssize_t ndefaults = defaults == NULL ? 0 : PyTuple_GET_SIZE(defaults);
    if ((size_t)(ndefaults - need) > (size_t)(argcount - need)) {
        return false;
    }
    unsigned int kwonly_mask = (bits & _Py_CALL_BIND_GENERAL)
        ? plan->kwonly_mask : 0;
    if (!kwonly_mask) {
        return true;
    }
    PyObject *kwdefs = func->func_kwdefaults;
    if (kwdefs == NULL || !PyDict_CheckExact(kwdefs) ||
        !DK_IS_UNICODE(((PyDictObject *)kwdefs)->ma_keys)) {
        return false;
    }
    for (unsigned int rest = kwonly_mask; rest; rest &= rest - 1) {
        PyObject *name = _PyTuple_GET_OBJECT(code->co_localsplusnames,
                                             argcount + __builtin_ctz(rest));
        PyValue value;
        if (PyUnstable_Unicode_GET_CACHED_HASH(name) == -1 ||
            !_PyDict_GetUnicodeValue((PyDictObject *)kwdefs, name, &value)) {
            return false;
        }
    }
    return true;
#endif
}

Py_NO_INLINE _PyInterpreterFrame *
_PyRop_PushPlan(PyThreadState *tstate, _PyInterpreterFrame *frame,
                PyValue *fn, _PyInstr *head, const _PyFbCall *fb);

Py_NO_INLINE PyValue
_PyRop_PlanGenerator(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     PyValue *fn, _PyInstr *head, const _PyFbCall *fb);

static inline bool
_PyRop_NotPythonCallable(PyValueBorrow fn)
{
    if (!_PyRop_IsObject(fn)) {
        return true;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(fn);
    return !PyFunction_Check(callable) && Py_TYPE(callable) != &PyMethod_Type;
}

static inline bool
_PyRop_PyCallableEx(PyValueBorrow fn)
{
    if (!_PyRop_IsObject(fn)) {
        return false;
    }
    PyObject *func = PyValue_AsKnownObjectBorrow(fn);
    return PyFunction_Check(func) &&
        ((PyFunctionObject *)func)->vectorcall == _PyFunction_Vectorcall;
}

static inline bool
_PyRop_NotPyCallableEx(PyValueBorrow fn)
{
    return !_PyRop_PyCallableEx(fn);
}

/* The rows of Call.Observed: the events of call_run with CALL_INSTRUMENTED */

Py_NO_INLINE int
_PyRop_CallEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 _PyInstr *site, PyValue *fn, const _PyIRWord *head);



/* The rows of CallEx (Python/register_ops/calls.rop): the tuple, the CALL
 * event, the binding of a Python callee, and the native call.  The tuple that star_tuple made is an owner of the call: each
 * row below takes it. */

static inline PyObject *
rop_ex_callargs(PyValue tup)
{
    return PyValue_AsKnownObjectBorrow(tup);
}

/* The kwargs dict of the call is the one that the call held first (a
 * NULL owner if the word has none): the iteration of the arguments can run
 * Python, which can rebind the local that the dict was read from */
static inline PyObject *
rop_ex_kwargs(PyValue ko)
{
    return PyValue_IsNull(ko) ? NULL : PyValue_AsKnownObjectBorrow(ko);
}

PyValue
_PyRop_HoldKwargs(_PyInterpreterFrame *frame, const _PyIRWord *head);

PyValue
_PyRop_StarTuple(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 PyValue *fn, _PyInstr *head);

PyValue
_PyRop_CallEventEx(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, PyValue fo, PyValue tup, _PyInstr *head);

PyValue
_PyRop_CallNativeEx(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue fo, PyValue ko, PyValue tup, _PyInstr *head);

PyValue
_PyRop_CallNativeObservedEx(PyThreadState *tstate, _PyInterpreterFrame *frame,
                            _PyInstr *site, PyValue fo, PyValue ko, PyValue tup,
                            _PyInstr *head);



_PyInterpreterFrame *
_PyRop_PushPythonEx(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    PyValue *fn, PyValue fo, PyValue ko, PyValue tup,
                    _PyInstr *head, const _PyFbCall *fb);

static inline void
_PyRop_RetireFailedHeldEx(_PyInterpreterFrame *frame, PyValue *fn, PyValue fo,
                          PyValue ko, const _PyIRWord *head)
{
    PyValue_XCLOSE(ko);
    PyValue_XCLOSE(fo);
    _PyRop_RetireFailedArgs(frame, fn, head);
}

/* The rows of the observed forms of Call and CallKw that hold the callable
 * (CALL event first): the shared rows on the held value */

bool
_PyRop_PythonTargetHeld(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        PyValue fo, const _PyIRWord *head);

_PyInterpreterFrame *
_PyRop_PushPythonHeld(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      PyValue *fn, PyValue fo, _PyInstr *head);

PyValue
_PyRop_CallNativeObservedHeld(PyThreadState *tstate, _PyInterpreterFrame *frame,
                              _PyInstr *site, PyValue *fn, PyValue fo,
                              const _PyIRWord *head);

static inline void
_PyRop_RetireFailedHeld(_PyInterpreterFrame *frame, PyValue *fn, PyValue fo,
                        const _PyIRWord *head)
{
    PyValue_XCLOSE(fo);
    _PyRop_RetireFailedArgs(frame, fn, head);
}

bool
_PyRop_KwargsPending(_PyInterpreterFrame *frame, PyValue *local);

PyValue
_PyRop_NewKwargs(_PyInterpreterFrame *frame, PyValue *local);

/* Whether a call of code with nargs positional arguments and no keywords
 * binds each argument to its own local: no *args, **kwargs, keyword-only
 * parameter or default to fill. */
static inline Py_ALWAYS_INLINE int
code_binds_exactly(PyCodeObject *code, Py_ssize_t nargs)
{
    return nargs == code->co_argcount && code->co_kwonlyargcount == 0 &&
           !(code->co_flags & (CO_VARARGS | CO_VARKEYWORDS));
}

/* Argument i of a call as a borrowed word: the window word, or the word of
 * the object of an object vector. */
static inline Py_ALWAYS_INLINE PyValueBorrow
vector_arg(const void *args, Py_ssize_t i, int objects)
{
    if (objects) {
        return PyValue_FromObjectBorrow(((PyObject *const *)args)[i]);
    }
    return PyValue_Borrow(((const PyValue *)args)[i]);
}

Py_NO_INLINE _PyBound
_PyEval_vector_bind(PyThreadState *tstate, PyFunctionObject *func, PyObject *locals,
            const void *args, int objects, Py_ssize_t nargs, PyObject *kwnames);

/* The entry of every native caller of a Python function: it pushes the
 * frame, binds the arguments and runs it.  The arguments are the window of
 * words that value code holds, or the vector of objects that the object API
 * holds (objects is a constant of each caller).  A call that binds exactly
 * dups each argument into its local.  locals is a new reference, or NULL,
 * that the frame consumes.  The frame is current for the evaluator, which
 * consumes it. */
static inline Py_ALWAYS_INLINE PyValue
eval_vector(PyThreadState *tstate, PyFunctionObject *func, PyObject *locals,
            const void *args, int objects, size_t nargsf, PyObject *kwnames)
{
#ifndef NDEBUG
    _PyStackChunk *chunk = tstate->datastack_chunk;
    PyObject **top = tstate->datastack_top;
    PyObject **limit = tstate->datastack_limit;
#endif
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    _PyInterpreterFrame *frame;
    /* A generator function's call creates its generator, not a frame to
     * run: it takes the general path, which binds into the generator. */
    if (kwnames == NULL && code_binds_exactly(code, nargs) &&
        !code_creates_generator(code))
    {
        /* Nothing here runs Python, so func's code cannot change before
         * the frame holds a count of it. */
        CALL_STAT_INC(frames_pushed);
        if (_PyCode_GetExec(code) == NULL) {
            Py_XDECREF(locals);
            return PyValue_NULL;
        }
        frame = _PyThreadState_PushFrame(tstate, code->co_framesize);
        if (frame == NULL) {
            Py_XDECREF(locals);
            _PyErr_NoMemory(tstate);
            return PyValue_NULL;
        }
        _PyFrame_Initialize(tstate, frame,
                            PyValue_DUP(PyValue_FromObjectBorrow(func)),
                            locals, code, frame->base, NULL);
        _PyFrame_CopyFreeVars(frame, code);
        for (Py_ssize_t i = 0; i < nargs; i++) {
            frame->base[i] = PyValue_DUP(vector_arg(args, i, objects));
        }
    }
    else {
        _PyBound bound = _PyEval_vector_bind(tstate, func, locals, args, objects,
                                     nargs, kwnames);
        if (bound.gen != NULL) {
            return PyValue_FromObjectSteal((PyObject *)bound.gen);
        }
        frame = bound.frame;
        if (frame == NULL) {
            return PyValue_NULL;
        }
    }
    EVAL_CALL_STAT_INC(EVAL_CALL_VECTOR);
    PyValue result = eval_frame_value(tstate, frame, 0);
#ifndef NDEBUG
    /* The first root chunk is retained even after its initial frame exits. */
    assert(chunk == NULL || tstate->datastack_chunk == chunk);
    assert(chunk == NULL || tstate->datastack_top == top);
    assert(chunk == NULL || tstate->datastack_limit == limit);
#endif
    return result;
}

static inline Py_ALWAYS_INLINE PyObject *
function_locals(PyFunctionObject *func)
{
    return ((PyCodeObject *)func->func_code)->co_flags & CO_OPTIMIZED ? NULL
        : Py_NewRef(func->func_globals);
}


Py_VALUE_ENTRY PyObject *
_PyFunction_Vectorcall(PyObject *func, PyObject *const *stack, size_t nargsf,
                       PyObject *kwnames);



/* Logic for matching an exception in an except* clause (too
   complicated for inlining).
*/


/* Iterate v (borrowed) argcnt times and store the results in out[0], out[1],
   ... in the order they come.  Return 1 for success, 0 if error: the values
   already stored are closed.

   If argcntafter == -1, do a simple unpack. If it is >= 0, do an unpack
   with a variable target, whose list is stored after the argcnt values,
   followed by the argcntafter values that end the iterable.
*/











































/* Extract a slice index from a PyLong or an object with the
   nb_index slot defined, and store in *pi.
   Silently reduce values larger than PY_SSIZE_T_MAX to PY_SSIZE_T_MAX,
   and silently boost values less than PY_SSIZE_T_MIN to PY_SSIZE_T_MIN.
   Return 0 on error, 1 on success.
*/



/* Inline values in BINARY_SLICE, STORE_SLICE and CALL_ISINSTANCE

   An int or float operand may be inline (see pycore_value.h). Bounds that
   are inline ints or None slice a list, tuple or str without boxes, and
   isinstance() of an inline value against a type needs no box. Other
   bounds go into a slice object by value, and the item is read or stored
   through _PyValue_GetItem() or _PyValue_SetItem(): no bound is boxed. */

/* The bounds of a slice with no step, as PySlice_Unpack() gives them, if
   start and stop are each an inline int or None, and one is an inline int. */
static bool
inline_slice_bounds(PyValueBorrow start, PyValueBorrow stop,
                    Py_ssize_t *istart, Py_ssize_t *istop)
{
    if (!PyValue_IsInlineInt(start) && !PyValue_IsInlineInt(stop)) {
        return false;
    }
    if (PyValue_IsInlineInt(start)) {
        *istart = PyValue_InlineInt64(start);
    }
    else if (PyValue_IsNone(start)) {
        *istart = 0;
    }
    else {
        return false;
    }
    if (PyValue_IsInlineInt(stop)) {
        *istop = PyValue_InlineInt64(stop);
    }
    else if (PyValue_IsNone(stop)) {
        *istop = PY_SSIZE_T_MAX;
    }
    else {
        return false;
    }
    return true;
}

/* The slice start:stop as a new object, with the bounds as they are (value
   fields, M4b): no box of an inline bound. NULL with an exception set. */
PyObject *
_PyEval_slice_of_bounds(PyValueBorrow start, PyValueBorrow stop);

// container[start:stop], for BINARY_SLICE: an owner of the item, or
// PyValue_NULL with an exception set.
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_BinarySlice)(PyValueBorrow container_st, PyValueBorrow start_st, PyValueBorrow stop_st)
{
    if (PyValue_IsPointerSpace(container_st)) {
        PyObject *container = PyValue_AsKnownObjectBorrow(container_st);
        PyObject *res = NULL;
        Py_ssize_t start, stop;
        if (inline_slice_bounds(start_st, stop_st, &start, &stop)) {
            if (PyList_CheckExact(container)) {
                PySlice_AdjustIndices(PyList_GET_SIZE(container), &start, &stop, 1);
                res = PyList_GetSlice(container, start, stop);
                goto done;
            }
            if (PyTuple_CheckExact(container)) {
                PySlice_AdjustIndices(PyTuple_GET_SIZE(container), &start, &stop, 1);
                res = PyTuple_GetSlice(container, start, stop);
                goto done;
            }
            if (PyUnicode_CheckExact(container)) {
                PySlice_AdjustIndices(PyUnicode_GET_LENGTH(container), &start, &stop, 1);
                res = PyUnicode_Substring(container, start, stop);
                goto done;
            }
        }
        else if (PyValue_IsPointerSpace(start_st) &&
                 PyValue_IsPointerSpace(stop_st))
        {
            /* Object bounds (None, or an object with __index__): the type's
               own slicing reads them without a slice object. */
            PyObject *start_o = PyValue_AsKnownObjectBorrow(start_st);
            PyObject *stop_o = PyValue_AsKnownObjectBorrow(stop_st);
            if (PyList_CheckExact(container)) {
                res = _PyList_BinarySlice(container, start_o, stop_o);
                goto done;
            }
            if (PyTuple_CheckExact(container)) {
                res = _PyTuple_BinarySlice(container, start_o, stop_o);
                goto done;
            }
            if (PyUnicode_CheckExact(container)) {
                res = _PyUnicode_BinarySlice(container, start_o, stop_o);
                goto done;
            }
        }
        goto generic;
done:
        return res == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res);
    }
generic:;
    PyObject *slice = _PyEval_slice_of_bounds(start_st, stop_st);
    if (slice == NULL) {
        return PyValue_NULL;
    }
    PyValue res = _PyValue_GetItem(container_st, PyValue_FromObjectBorrow(slice));
    Py_DECREF(slice);
    return res;
}

int
(_PyEval_StoreSlice)(PyValueBorrow v_st, PyValueBorrow container_st,
                   PyValueBorrow start_st, PyValueBorrow stop_st);









#define CANNOT_CATCH_MSG "catching classes that do not inherit from "\
                         "BaseException is not allowed"

#define CANNOT_EXCEPT_STAR_EG "catching ExceptionGroup with except* "\
                              "is not allowed. Use except instead."


/* The value forms: a number cannot be an exception class, which
   _PyEval_CheckExceptTypeValid reports for any other non-class */
int
(_PyEval_CheckExceptTypeValidValue)(PyThreadState *tstate, PyValueBorrow right);

int
(_PyEval_CheckExceptStarTypeValidValue)(PyThreadState *tstate, PyValueBorrow right);



/* The binary intrinsics other than GETATTR and SETATTR take objects:
   a type parameter, a function, an exception group. Boxes them, as above. */
Py_NO_INLINE PyValue
(_PyEval_CallIntrinsic2)(PyThreadState *tstate, int oparg,
                         PyValueBorrow value2, PyValueBorrow value1);

/* The method descriptors of METH_O and METH_NOARGS take objects
   (ml_meth(PyObject *, PyObject *)): these box a number in a record that
   the handler does not carry, and return the result, or PyValue_NULL with an
   exception set. */
Py_NO_INLINE PyValue
(_PyEval_CallMethodDescriptorO)(PyThreadState *tstate, PyCFunction cfunc,
                              PyValueBorrow self, PyValueBorrow arg);

Py_NO_INLINE PyValue
(_PyEval_CallMethodDescriptorNoArgs)(PyThreadState *tstate, PyCFunction cfunc,
                                   PyValueBorrow self);

/* The intrinsics of CALL_INTRINSIC_1 other than unary plus take objects.
   Only PRINT and ASYNC_GEN_WRAP can get a number: this boxes it here, in a
   record that the handler does not carry. Returns the result, or
   PyValue_NULL with an exception set. */
static inline Py_ALWAYS_INLINE PyValue
(_PyEval_CallIntrinsic1)(PyThreadState *tstate, int oparg, PyValueBorrow value)
{
    if (PyValue_IsPointerSpace(value)) {
        // An object goes to the intrinsic as it is; only a number needs a box
        PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(
            tstate, PyValue_AsKnownObjectBorrow(value));
        return res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
    }
    PyObject *boxes[1];
    _PyValueTemps temps = {boxes, 0, 1};
    PyValue res = PyValue_NULL;
    PyObject *value_o = _PyValueTemps_Object(&temps, value);
    if (value_o != NULL) {
        PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(tstate, value_o);
        if (res_o != NULL) {
            res = PyValue_FromObjectSteal(res_o);
        }
    }
    _PyValueTemps_Close(&temps);
    return res;
}

/* Function.Make and Intrinsic.*: the rows (functions.rop) */

PyValue
_PyRop_FunctionMake(_PyInterpreterFrame *frame, PyValue code,
                    const _PyRopSpan *fields, uint32_t present);

/* Raise, Reraise, ExceptEnter, ExceptLeave, ExceptMatch and ExceptGroupMatch:
 * the rows (exceptions.rop) */

int
_PyRop_RaiseArgs(PyThreadState *tstate, const _PyRopSpan *args);

void
_PyRop_ReraisePending(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *at);

void
_PyRop_RestoreException(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *at, PyValue *home);

void
_PyRop_SetOrigin(_PyInterpreterFrame *frame, PyValue *home);

void
_PyRop_RetireFailedOperand(_PyInterpreterFrame *frame, PyValue *home);

PyValue
_PyRop_SwapHandled(PyThreadState *tstate, PyValue *home);

void
_PyRop_RestoreHandled(PyThreadState *tstate, PyValue x);

int
_PyRop_CheckExceptType(PyThreadState *tstate, PyValue *home);

bool
_PyRop_ExceptionMatches(PyValueBorrow exc, PyValueBorrow pattern);

bool
_PyRop_IsStopIteration(PyValueBorrow exc);

PyValue
_PyRop_StopIterationValue(PyValueBorrow exc);

bool
_PyRop_IsStopAsyncIteration(PyValueBorrow exc);

Py_NO_INLINE int
_PyRop_EndAsyncForEvent(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *site);


PyValue
_PyRop_WithExitCall(PyValueBorrow fn, PyValueBorrow self, PyValueBorrow exc);

PyValue
_PyRop_WithExitBound(PyValueBorrow fn, PyValueBorrow exc);

int
_PyRop_CheckExceptStarType(PyThreadState *tstate, PyValue *home);

PyValue
_PyRop_GroupMatch(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue *exc, PyValue *pattern, PyValue *rest);

PyValue
_PyRop_UnaryOp(uint32_t op, PyValue x);

/* The compiler's number of each member of IntrinsicId, in declaration order */
static const uint8_t rop_intrinsic_number[] = {
    INTRINSIC_PRINT, INTRINSIC_IMPORT_STAR, INTRINSIC_STOPITERATION_ERROR,
    INTRINSIC_ASYNC_GEN_WRAP,
    INTRINSIC_LIST_TO_TUPLE, INTRINSIC_TYPEVAR, INTRINSIC_PARAMSPEC,
    INTRINSIC_TYPEVARTUPLE, INTRINSIC_SUBSCRIPT_GENERIC,
    INTRINSIC_TYPEALIAS, INTRINSIC_BUILD_FROZENSET,
    INTRINSIC_PREP_RERAISE_STAR, INTRINSIC_TYPEVAR_WITH_BOUND,
    INTRINSIC_TYPEVAR_WITH_CONSTRAINTS, INTRINSIC_SET_FUNCTION_TYPE_PARAMS,
    INTRINSIC_SET_TYPEPARAM_DEFAULT, INTRINSIC_ADD_CONDITIONAL_ANNOTATION,
    INTRINSIC_MATCH_CLASS_ISINSTANCE,
    INTRINSIC_GETATTR, INTRINSIC_SETATTR,
};

PyValue
_PyRop_IntrinsicUnary(PyThreadState *tstate, uint32_t id, PyValue x);

PyValue
_PyRop_IntrinsicImportStar(PyThreadState *tstate, PyValue x);

PyValue
_PyRop_IntrinsicStopIteration(PyThreadState *tstate, PyValue x);

PyValue
_PyRop_IntrinsicBinary(PyThreadState *tstate, uint32_t id, PyValue x, PyValue y);

PyValue
_PyRop_IntrinsicAttr(PyThreadState *tstate, uint32_t id, PyValue x, PyValue y);

void
(_PyEval_FormatKwargsError)(PyThreadState *tstate, PyValueBorrow func, PyObject *kwargs, PyObject *dupkey);






/* Implement Py_EnterRecursiveCall() and Py_LeaveRecursiveCall() as functions
   for the limited API. */





PyObject *
(_PyEval_GetAwaitable)(PyValueBorrow iterable_value, int oparg);


/* FOR_ITER's step: the item and 1, the completion value and 0, or the
 * error and -1; a StopIteration is the completion.  The cursor goes in and
 * comes out, advanced by an item of an index-protocol iterator. */
static inline Py_ALWAYS_INLINE _PyForIterStep
_PyForIter_VirtualIteratorNextInline(PyThreadState *tstate,
                                     _PyInterpreterFrame *frame,
                                     PyValueBorrow iter, PyValue cursor)
{
    _PyForIterStep result = {0, PyValue_NULL, cursor};
    PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
    PyValueBorrow index = PyValue_Borrow(cursor);
    if (PyValue_IsTaggedIndex(index)) {
        intptr_t i = PyValue_UntagIndex(index);
        assert(i >= 0);
        _PyValueIndexPair next_index = Py_TYPE(iter_o)->_tp_iteritem(iter_o, i);
        i = next_index.index;
        result.value = next_index.value;
        if (PyValue_IsNull(next_index.value)) {
            result.status = i < 0 ? -1 : 0;
            return result;
        }
        result.cursor = PyValue_TagIndex(i);
        result.status = 1;
        return result;
    }
    PyTypeObject *iter_type = Py_TYPE(iter_o);
    _Py_iternextvaluefunc next_value = _PyType_VALUE_SLOT(iter_type, iternext);
    _PyStatusValue step;
    if (next_value != NULL) {
        _PyType_CHECK_VALUE_ROW(iter_type, _Py_VALUE_SLOT_INDEX_iternext);
        step = next_value(iter_o);
        assert(step.status != PYGEN_NEXT || !PyValue_IsNull(step.value));
        assert(step.status != PYGEN_ERROR || PyValue_IsNull(step.value));
    }
    else {
        step = _PyValue_IterStep(iter_o);
    }
    /* An item, or the semantic completion, which reports an implicit
     * StopIteration once and keeps the returned owner without fabricating
     * a separate RAISE. Both are the caller's status and value as they are. */
    if (step.status != PYGEN_ERROR) {
        result.status = step.status;
        result.value = step.value;
        return result;
    }
    if (_PyForIter_StopIsExhaustion(tstate, frame)) {
        return result;
    }
    result.status = -1;
    return result;
}

Py_NO_INLINE int
_PyForIter_StopIsExhaustion(PyThreadState *tstate, _PyInterpreterFrame *frame);


/* Check if a 'cls' provides the given special method. */
static inline int
type_has_special_method(PyTypeObject *cls, PyObject *name)
{
    assert(!PyErr_Occurred());
    PyObject *r = _PyType_LookupRef(cls, name);
    int res = r != NULL && Py_TYPE(r)->tp_descr_get != NULL;
    Py_XDECREF(r);
    return res;
}


PyValue _PyRop_SuperMethod(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
    const _PyRopSpan *parts, PyObject *attr_name, PyValue *self);
PyValue _PyRop_SuperMethodObserved(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
    const _PyRopSpan *parts, PyObject *attr_name, PyValue *self);
PyValue _PyRop_SuperMethodExplicit(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
    const _PyRopSpan *parts, PyObject *attr_name, PyValue *self);
PyValue _PyRop_SuperMethodExplicitObserved(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *site,
    const _PyRopSpan *parts, PyObject *attr_name, PyValue *self);

/* The rows that the register arms always inline, wherever the arms run */
#include "generated_rop_inline.h"

/* The whole-word completion of each arm (Python/ceval_forms.c) */
typedef int (*_PyRopCompletion)(PyThreadState *, _PyInterpreterFrame *,
                                _PyIRWord *, PyValue);
extern const _PyRopCompletion _PyRop_Completions[ROP_NUM_HANDLERS];
/* The observing evaluator's: an observer's for each arm of an event family */
extern const _PyRopCompletion _PyRop_ObservingCompletions[ROP_NUM_HANDLERS];

/* The event of an instrumented branch (Python/ceval_forms.c), which the
 * evaluator calls, and the function-form executor, which it runs under
 * -X ir_forms */
Py_NO_INLINE _PyIRTransfer
_PyEval_ir_event_edge(PyThreadState *tstate, _PyInterpreterFrame *frame,
              _PyInstr *this_instr, _PyInstr *source, _PyInstr *next,
              int event, int edge);
_PyIRTransfer _PyEval_ir_GEN_REDIRECT_EFFECT(PyThreadState *tstate, _PyInterpreterFrame *frame,
                                     _PyInstr *next_instr, _PyIRWord ir_word);
PyValue _PyEval_ExecuteForms(PyThreadState *tstate, _PyInterpreterFrame *frame,
                             int throwflag);

#ifdef _PY_JIT_SUPPORTED
/* The baseline JIT's hand-offs (Python/ceval.c), which the evaluators call
 * (jit_run and jit_switch in Python/ceval_evaluator.h) */
_PyIRTransfer _PyEval_JitRun(PyThreadState *tstate, _PyJitUnit *unit,
                             _PyInterpreterFrame *frame, _PyInstr *next);
_PyIRTransfer _PyEval_JitSwitch(PyThreadState *tstate,
                                _PyInterpreterFrame *frame, _PyInstr *next);
#endif

#endif /* Py_CEVAL_SHARED_H */
