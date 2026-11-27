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

/* The baseline JIT (pyjit/design/proposals/tiers87.md, Phase 2; Python/jit.c).

   A unit is the machine code of one executing copy (_PyCodeExec): every word
   is a call of the function form of the handler that the word named when the
   unit was compiled, followed by a native jump when the form's transfer is
   the same frame and a word known at compile time (the next word or the
   word's jump target).

   The transfer protocol is one rule: a frame switch goes to the target
   frame's word through its executing copy's entry table,
   _PyCodeExec.jit_entries, which is the entry table of the code's newest
   unit while that unit is valid, else NULL (Python/jit.c, publish()).  So
   a frame runs compiled whenever its code has live entries.  A switch in
   compiled code jumps through the table on the same native frame
   (emit_other_frame()); one whose code publishes none, or that names the
   switch word, leaves the unit with the transfer, and the driver (jit_run()
   in Python/ceval.c) recompiles a stale unit, refuses a retired one, or
   returns the transfer to the evaluator, ir_evaluate(), the cold tier.  The
   evaluator tests the table at its own switches, a return, a yield and an
   exception handled in a frame (IR_JIT_SWITCH()), and hands the frame to
   the unit; Resume.Warm and Jump.Warm, which the lowering gives a code
   under a mode other than off, count a warm-up event and enter the unit
   once the code has one.

   A unit is entered by calling its code as

       _PyIRTransfer enter(PyThreadState *tstate, _PyInterpreterFrame *frame,
                           const void *entry);

   where entry is entries[word index].  At every word boundary of the code
   r12 holds tstate, r13 frame, r14 word 0 of the frame's executing copy, r15
   frame->base and rbx the unit; the stack is 16-byte aligned; every value
   is in the frame.  A word that specialisation can rewrite runs the handler
   it named at compile time only while the word still names it, since
   specialisation rewrites the handler and its feedback entry together.

   The JIT runs in the GIL build on Linux x86-64 (_PY_JIT_SUPPORTED). */

/* The completion status of a whole-word stub (g1a/abi.md), in eax:
 * 0 the word completed and nothing reentrant ran; MISS a guard failed with
 * the word untouched (alone); ESCAPED an executed path may have reentered
 * Python (test RETIRED before the successor); RAISED the word raised, its
 * raise checkpoint in force; BRANCH the word completed at its static
 * target; AGAIN the word is untouched and runs again under its current
 * handler (a specialisation's redispatch); REDIRECT the word completed and
 * the frame's control moved while it ran (a debugger's jump): the word's
 * successor is frame->instr_ptr, not the next word. */
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
    /* 0 or _PyJIT_STALE | _PyJIT_RETIRED.  Compiled code tests it at
     * offset 0: RETIRED after each word that may run Python, both at each
     * backward jump. */
    uint8_t invalid;
    uint32_t nwords;
    const _PyIRWord *words;         // word 0 of the exec, r14 in the code
#ifdef _PY_JIT_SUPPORTED
    _PyJitCode code;                // the entry at code.start
    /* The unit's .eh_frame, in the code block after the code, and its
     * registrations: backtrace() and gdb walk through the unit
     * (Python/jit.c, build_unwind) */
    _PyJitUnwindEntry unwind;
#endif
    /* An address per word, in the code block; an operand word's traps */
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

/* The execution pin (g3/design.md section 2): compiled code (a stub or a
 * function form) that departs frame by a return or a yield calls this after
 * the arm's last guard, before anything can drop the last reference to the
 * frame's code.  Stores a new reference to the code in the evaluation
 * loop's activation, then releases the previous pin: store first, so that a
 * pin of the same code never drops it.  Returns _PyIR_ESCAPED if the
 * release deallocated (a code's weakref callbacks run Python), else 0. */
PyAPI_FUNC(int) _PyJit_PinDeparting(PyThreadState *tstate,
                                    struct _PyInterpreterFrame *frame);

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

/* Free the units of exec, which is being freed. */
extern void _PyJit_FreeUnits(_PyCodeExec *exec);

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

#endif  /* _PY_JIT_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_H */
