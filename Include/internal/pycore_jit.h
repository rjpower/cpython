#ifndef Py_INTERNAL_JIT_H
#define Py_INTERNAL_JIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"              // _PyCodeExec
#include "pycore_jit_memory.h"      // _PyJitCode, _PY_JIT_SUPPORTED
#include "pycore_jit_unwind.h"      // _PyJitUnwindEntry

/* The baseline JIT (Python/jit.c).

   A unit is the machine code of one executing copy (_PyCodeExec): each head
   word is a copy of the stencil of the arm that the word named when the
   unit was compiled (Python/jit_stencils.c), with the word's fields patched
   in and the arm's exits bound: the next word's copy follows, or is jumped
   to; a branch goes to its target's copy; every other exit is a tail call
   of the normal evaluator (_PyEval_JitDispatch() and the others,
   Python/ceval_evaluator.h).  Copies run in the evaluator's tail-call
   convention on the driver's native frame, as the arms do: a unit has no
   frame, prologue or exit of its own.

   The transfer protocol is one rule: a frame switch goes to the target
   frame's word through its executing copy's entry table,
   _PyCodeExec.jit_entries, which is the entry table of the code's newest
   unit while that unit is valid, else NULL (Python/jit.c, publish()).  So
   a frame runs compiled whenever its code has live entries.  Compiled code
   switches through _PyEval_JitDispatch(); the evaluator tests the table at
   its own switches (IR_JIT_SWITCH()) and its warm-up events (Resume.Warm,
   Jump.Warm: IR_JIT_EVENT()), and enters the unit by a tail call of the
   word's entry.  The switch word has no entry and stays with the evaluator.

   A guard miss runs the family's generic arm in the evaluator at the word,
   and an error the evaluator's process_error; the frame then goes on in the
   evaluator until a warm-up event or a switch enters the unit again.  A
   word whose arm specialisation rewrites marks the unit stale, and its copy
   in each of the code's units now leaves for the evaluator at its start
   (_PyJit_MarkWordChanged()): an arm reads the feedback of the arm that it
   was compiled for, which a frame still in the copy, as a loop is, must not
   meet.  Compiled code tests nothing, and the next warm-up event
   recompiles.

   A unit outlives its code while a thread may still run it.  A departing
   word (a return or a yield) clears its frame, which may drop the code's
   last reference, and the copy runs on until it leaves the unit: the
   thread is in a departure window.  The word opens it by setting the
   thread's jit_departing flag (IR_MARK_DEPARTURE() in the stencils); every
   exit into the evaluator closes it.  A dead code's units wait on the
   interpreter's dead list (_PyJit_FreeUnits()), which is freed once no
   thread is in a window (_PyJit_Reclaim()): at a compile, when a thread
   leaves its outermost activation, and at finalization.  A thread that
   releases the GIL is quiescent for this purpose only once it has left the
   unit's code, which its cleared flag records.  On a free-threaded build
   the list would be a QSBR deferral (Python/qsbr.c) whose detaches and
   polls count as quiescent only while the flag is clear.

   The JIT runs in the GIL build on Linux x86-64 (_PY_JIT_SUPPORTED). */

/* The completion status of a whole-word stub (g1a/abi.md), in eax:
 * 0 the word completed and nothing reentrant ran; MISS a guard failed with
 * the word untouched (alone); ESCAPED an executed path may have reentered
 * Python (test RETIRED before the successor); RAISED the word raised, its
 * raise checkpoint in force; BRANCH the word completed at its static
 * target; AGAIN the word is untouched and runs again under its current
 * handler (a specialisation's redispatch); REDIRECT the word completed and
 * the frame's control moved while it ran (a debugger's jump, or a switch
 * of evaluator, which names the switch word): the word's successor is
 * frame->return_pc, not the next word. */
#define _PyIR_MISS      1
#define _PyIR_ESCAPED   2
#define _PyIR_RAISED    4
#define _PyIR_BRANCH    8
#define _PyIR_AGAIN     16
#define _PyIR_REDIRECT  32

/* The plan of a handler (Python/generated_rop_stubs.c.h, by handler): NONE
 * keeps the template or the form's call; INLINE is a template whose misses
 * run the family's generic stub; SHARED is a call of the arm's stub. */
enum {
    _PyIR_PLAN_NONE,
    _PyIR_PLAN_INLINE,
    _PyIR_PLAN_SHARED,
};

typedef struct {
    uint8_t region;
    /* A uop of the arm declares runs_python or polls, so its completions
     * report ESCAPED: the site's cold path tests RETIRED inline */
    uint8_t escapes;
    /* The handler whose template or stub a word of this plan compiles:
     * the handler itself, or its family's baseline(...) arm (RESUME's
     * words compile to RESUME_CHECK) */
    uint16_t arm;
    /* int stub(PyThreadState *, _PyInterpreterFrame *, _PyIRWord *site) */
    const void *stub;           // a SHARED arm's
    const void *generic;        // the family's generic, for a miss
} _PyIRStubPlan;

PyAPI_DATA(const _PyIRStubPlan) _PyIR_StubPlans[];

/* The jit modes (-X jit, PYTHON_JIT) */
enum {
    _PyJIT_OFF,
    _PyJIT_ON,          // compile a code object after warm-up
    _PyJIT_ALWAYS,      // compile a code object at its first warm-up event
};

/* _PyJitUnit.invalid, which compiled code tests */
#define _PyJIT_STALE    1   // compiled code found a word whose handler changed
#define _PyJIT_RETIRED  2   // the code is monitored: leave at once

/* The spill reservation of every unit's native frame, below the six saved
 * registers (g3/design.md section 6).  It is one size for every unit: after
 * a switch, a frame that one unit's prologue built is torn down by another
 * unit's shared exit.  8 keeps rsp 16-byte aligned at every word boundary;
 * continuation descriptors (G4) add to it. */
#define _PyJIT_SPILL_BYTES 8

/* Warm-up events (frame entries and backward transfers) before jit=on
 * compiles a code object, and the most compiles of one code object. */
#define _PyJIT_WARMUP       32
#define _PyJIT_MAX_COMPILES 8

typedef struct _PyJitUnit {
    /* 0 or _PyJIT_STALE | _PyJIT_RETIRED, which _PyJit_Event() reads;
     * compiled code reads neither */
    uint8_t invalid;
    /* Its compile rewrote the code's Resume.Warm heads to Resume.Poll, which
     * publish() and unpublish() keep in step with the entries
     * (Python/jit.c) */
    uint8_t warm_heads;
    uint32_t nwords;
    const _PyIRWord *words;         // word 0 of the exec
    /* The bytes of the copies' hot parts, and of their cold parts and jump
     * tables */
    uint32_t hot_bytes;
    uint32_t cold_bytes;
    /* The arm that each head word's copy is a copy of, by word index */
    uint16_t *arms;
#ifdef _PY_JIT_SUPPORTED
    _PyJitCode code;                // the copies, the entries, the unwind info
    /* The unit's .eh_frame, in the code block after the code: the stencils'
     * own rows, and its registrations: backtrace() and gdb walk through the
     * unit (Python/jit.c, emit_unit()) */
    _PyJitUnwindEntry unwind;
#endif
    /* An address per word, in the code block: a head word's copy, an
     * operand word's trap */
    const void *const *entries;
    struct _PyJitUnit *older;       // the unit this one replaced, or NULL
} _PyJitUnit;

/* Whether specialisation can rewrite the handler of word i of code's
 * executing copy: the generic handler of its instruction drives
 * specialisation from its feedback entry.  set_opcode() in
 * Python/specialize.c, the one writer of the column apart from
 * instrumentation (which retires units), rewrites only such words, so
 * compiled code checks the handler of these words only. */
static inline int
_PyJit_HandlerMayChange(PyCodeObject *code, int i)
{
    return _PyFb_HasEntry(_PyCode_InstrHandler(code, i));
}

/* The handler that compiled code runs for the word w: its own. */
static inline int
_PyJit_CompiledHandler(_PyIRWord w)
{
    return IR_HANDLER(w);
}

#ifdef _PY_JIT_SUPPORTED
/* The plan by which a unit compiles word w (Python/jit.c, emit_unit()): its
 * compiled handler's, or none in a unit at the recompile cap. */
PyAPI_FUNC(const _PyIRStubPlan *) _PyJit_PlanOf(_PyIRWord w, int capped);
#endif

/* The handler whose template, stub or form word w compiles under plan: the
 * plan's arm, which is the handler itself or its family's baseline(...)
 * arm, or the compiled handler when the word is not planned */
static inline int
_PyJit_ArmOf(const _PyIRStubPlan *plan, _PyIRWord w)
{
    return plan->region == _PyIR_PLAN_NONE ? _PyJit_CompiledHandler(w) : plan->arm;
}

/* Read -X jit and PYTHON_JIT into interp->ceval.jit.  An unknown value is
 * an error; a mode other than off on a process whose code memory cannot be
 * reserved leaves the JIT off. */
extern PyStatus _PyJit_InitInterpreter(PyInterpreterState *interp);

/* Whether the JIT can run in this process (its code memory is reserved). */
PyAPI_FUNC(int) _PyJit_IsAvailable(void);

/* A warm-up event of frame: the frame enters or resumes its code, or jumps
 * back (Resume.Warm, Jump.Warm, a unit's backward jump when it is invalid),
 * or switches to code whose unit is invalid.  Returns the code's valid
 * unit; otherwise counts the event, and compiles the code when it is warm
 * or recompiles a stale unit.  Returns the unit to run, or NULL to run the
 * frame in the evaluator.  Declared in every build, for the generated
 * lowerings; defined where the JIT is supported. */
extern _PyJitUnit *_PyJit_Event(PyThreadState *tstate,
                                struct _PyInterpreterFrame *frame);

#ifdef _PY_JIT_SUPPORTED

/* The code of exec is monitored (Python/instrumentation.c): its entries
 * are unpublished, so no switch enters its unit, and compiled code running
 * in the unit leaves at the next test. */
PyAPI_FUNC(void) _PyJit_Retire(_PyCodeExec *exec);

/* exec is being freed: its units go to the interpreter's dead list. */
extern void _PyJit_FreeUnits(_PyCodeExec *exec);

/* Free the dead list unless a thread of interp is in a departure window. */
extern void _PyJit_Reclaim(PyInterpreterState *interp);

/* Free the dead list: interp is being cleared and runs no code. */
extern void _PyJit_FiniInterpreter(PyInterpreterState *interp);

/* Word instr of exec's code now names another arm (set_opcode(),
 * Python/specialize.c): its copy in every unit of the code goes to the
 * evaluator, and the newest unit, unless at the recompile cap, is stale. */
extern void _PyJit_MarkWordChanged(_PyCodeExec *exec, const _PyInstr *instr);

/* Whether word i's copy in unit goes to the evaluator, which
 * _PyJit_MarkWordChanged() makes it do */
PyAPI_FUNC(int) _PyJit_IsDiverted(const _PyJitUnit *unit, uint32_t i);

/* The arm that a miss of a word naming handler runs: its family's generic
 * arm (the stencil table's), or handler itself if it is that arm */
extern int _PyJit_GenericOf(int handler);

/* Compile the code of exec now, whatever its warm-up: the new unit, or NULL
 * if the code cannot be compiled (monitored, or no code memory).  For
 * _testinternalcapi. */
PyAPI_FUNC(_PyJitUnit *) _PyJit_Compile(PyCodeObject *code);

/* The function forms of the handlers, ir_handler_forms (Python/ceval.c), as
 * code addresses indexed by handler */
PyAPI_FUNC(const void *const *) _PyEval_HandlerForms(void);

#else  /* !_PY_JIT_SUPPORTED */

static inline void _PyJit_Retire(_PyCodeExec *Py_UNUSED(exec)) {}
static inline void _PyJit_FreeUnits(_PyCodeExec *Py_UNUSED(exec)) {}
static inline void _PyJit_MarkWordChanged(_PyCodeExec *Py_UNUSED(exec),
                                          const _PyInstr *Py_UNUSED(instr)) {}
static inline void _PyJit_Reclaim(PyInterpreterState *Py_UNUSED(interp)) {}
static inline void _PyJit_FiniInterpreter(PyInterpreterState *Py_UNUSED(interp)) {}

#endif  /* _PY_JIT_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_H */
