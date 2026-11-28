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

/* The jit modes (-X jit, PYTHON_JIT) */
enum {
    _PyJIT_OFF,
    _PyJIT_ON,          // compile a code object after warm-up
    _PyJIT_ALWAYS,      // compile a code object at its first warm-up event
};

/* _PyJitUnit.invalid, which compiled code tests */
#define _PyJIT_STALE    1   // compiled code found a word whose handler changed
#define _PyJIT_RETIRED  2   // the code is monitored: leave at once

/* Warm-up events (frame entries and backward transfers) before jit=on
 * compiles a code object.  A stale unit recompiles after as many more
 * (Python/jit.c, _PyJit_Event()).  Words that change their arm make the
 * unit stale at most twice under on (V8 stops optimising a function after
 * repeated deoptimisations), and up to the cap under always, a test mode.
 * Stubs reached also recompile, each time compiling the words that now
 * have run, up to the cap of compiles of one code object. */
#define _PyJIT_WARMUP               32
#define _PyJIT_MAX_DEOPTS_ON        2
#define _PyJIT_MAX_COMPILES         8

/* Under on, a code whose compiled loop body would not fit the L1i is
 * declined when it would compile: the hot bytes of the copies (or stubs)
 * between a backward jump and its target, from the stencil table
 * (Python/jit.c, loop_body_bytes()).  The body counts every path through
 * the loop, of which an iteration runs one, so the bound is eight times a
 * 32 KB L1i: it declines a body of straight-line copies that no L1i holds
 * (unpack_sequence's 400 unpacks, 374 KB) but not a parser's long chain of
 * branches whose iterations each run a few (re's _parse and _compile,
 * 138 and 144 KB with the generic arms' sampling inline, 92 and 87 KB with
 * it outlined).  At a 32 KB bound, which declines them, regex_compile runs
 * 3% more instructions and cycles in the evaluator. */
#define _PyJIT_MAX_LOOP_BYTES   (256 * 1024)

/* The exits from compiled code into the evaluator, by kind
 * (interp->ceval.jit_exits, Python/ceval_evaluator.h) */
enum {
    _PyJIT_EXIT_MISS,       // a guard failed: the family's generic arm
    _PyJIT_EXIT_DIVERTED,   // the copy of a word that changed its arm
    _PyJIT_EXIT_STUB,       // a word that had not run when it compiled
    _PyJIT_EXIT_SWITCH,     // a frame switch to code without entries
    _PyJIT_EXIT_CLOSE,      // the cold tail close of an arm
    _PyJIT_EXIT_GEN_RETURN, // a generator's return
    _PyJIT_EXIT_ERROR,      // an error: process_error
    _PyJIT_EXIT_INTERPRET,  // a redirect, or a word again under a new arm
    _PyJIT_EXIT_KINDS
};

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
 * frame in the evaluator.  Declared in every build; defined where the JIT is
 * supported. */
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
 * evaluator, and the newest unit, unless at the recompile cap, is stale.
 * final: the word's entry is megamorphic, so an arm it takes now is its
 * last. */
extern void _PyJit_MarkWordChanged(_PyCodeExec *exec, const _PyInstr *instr,
                                   int final);

/* The stub of word instr of frame's code was reached (_PyEval_JitStub()):
 * the word has run, and the reach counts as a warm-up event of the code;
 * at the warm-up count the code's unit is stale, and the next warm-up event
 * recompiles it with the word's copy. */
extern void _PyJit_StubReached(PyThreadState *tstate,
                               struct _PyInterpreterFrame *frame,
                               const _PyInstr *instr);

/* Whether word i's copy in unit is a stub (a word that had not run) */
PyAPI_FUNC(int) _PyJit_IsStub(const _PyJitUnit *unit, uint32_t i);

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

#else  /* !_PY_JIT_SUPPORTED */

static inline void _PyJit_Retire(_PyCodeExec *Py_UNUSED(exec)) {}
static inline void _PyJit_FreeUnits(_PyCodeExec *Py_UNUSED(exec)) {}
static inline void _PyJit_MarkWordChanged(_PyCodeExec *Py_UNUSED(exec),
                                          const _PyInstr *Py_UNUSED(instr),
                                          int Py_UNUSED(final)) {}
static inline void _PyJit_Reclaim(PyInterpreterState *Py_UNUSED(interp)) {}
static inline void _PyJit_FiniInterpreter(PyInterpreterState *Py_UNUSED(interp)) {}

#endif  /* _PY_JIT_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_H */
