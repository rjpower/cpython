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

/* The baseline JIT (pyjit/design/proposals/tiers87.md, Phase 2; Python/jit.c).

   A unit is the machine code of one executing copy (_PyCodeExec): every word
   is a call of the function form of the handler that the word named when the
   unit was compiled, followed by a native jump when the form's transfer is
   the same frame and a word known at compile time (the next word or the
   word's jump target).  Any other transfer leaves the unit with the
   transfer, and the activation loop (ir_execute_forms() in Python/ceval.c)
   resolves it: it enters the unit of the transfer's frame through the
   unit's entry table, or runs the word by its form.

   A unit is entered by calling its code as

       _PyIRTransfer enter(PyThreadState *tstate, _PyInterpreterFrame *frame,
                           const void *entry);

   where entry is entries[word index].  At every word boundary of the code
   r12 holds tstate, r13 frame, r14 word 0 of the frame's executing copy, r15
   frame->base and rbx the unit; the stack is 16-byte aligned; every value
   is in the frame.  A word runs the handler it named at compile time only
   while the word still names it, since specialisation rewrites the handler
   and its feedback entry together.

   The JIT runs in the GIL build on Linux x86-64 (_PY_JIT_SUPPORTED). */

/* The jit modes (-X jit, PYTHON_JIT) */
enum {
    _PyJIT_OFF,
    _PyJIT_ON,          // compile a code object after warm-up
    _PyJIT_ALWAYS,      // compile a code object at its first warm-up event
};

/* _PyJitUnit.invalid, which compiled code tests */
#define _PyJIT_STALE    1   // the handler column has changed since compile
#define _PyJIT_RETIRED  2   // the code is monitored: leave at once

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
#endif
    /* An address per word, in the code block; an operand word's traps */
    const void *const *entries;
    struct _PyJitUnit *older;       // the unit this one replaced, or NULL
} _PyJitUnit;

/* Read -X jit and PYTHON_JIT into interp->ceval.jit.  An unknown value is
 * an error; a mode other than off on a process whose code memory cannot be
 * reserved leaves the JIT off. */
extern PyStatus _PyJit_InitInterpreter(PyInterpreterState *interp);

/* Whether the JIT can run in this process (its code memory is reserved). */
PyAPI_FUNC(int) _PyJit_IsAvailable(void);

#ifdef _PY_JIT_SUPPORTED

/* A warm-up event of frame, whose code has no valid unit: the frame enters
 * its code (a call, a return, a resume, an activation's start) or jumps
 * back.  Counts it, and compiles the code when it is warm or recompiles a
 * stale unit.  Returns the unit to run, or NULL to run the frame by the
 * function forms. */
extern _PyJitUnit *_PyJit_Event(PyThreadState *tstate,
                                struct _PyInterpreterFrame *frame);

/* The code of exec is monitored: compiled code leaves its unit at the next
 * test (Python/instrumentation.c). */
static inline void
_PyJit_Retire(_PyCodeExec *exec)
{
    if (exec->jit_unit != NULL) {
        exec->jit_unit->invalid |= _PyJIT_RETIRED;
    }
}

/* A handler of exec's column has been specialised or unspecialised
 * (Python/specialize.c): the unit is recompiled at the next warm-up event
 * while the code has compiles left. */
static inline void
_PyJit_NoteRewrite(_PyCodeExec *exec)
{
    if (exec->jit_unit != NULL && exec->jit_compiles < _PyJIT_MAX_COMPILES) {
        exec->jit_unit->invalid |= _PyJIT_STALE;
    }
}

/* Free the units of exec, which is being freed. */
extern void _PyJit_FreeUnits(_PyCodeExec *exec);

/* Compile the code of exec now, whatever its warm-up: the new unit, or NULL
 * if the code cannot be compiled (monitored, or no code memory).  For
 * _testinternalcapi. */
PyAPI_FUNC(_PyJitUnit *) _PyJit_Compile(PyCodeObject *code);

/* The function forms of the handlers, ir_handler_forms (Python/ceval.c), as
 * code addresses indexed by handler */
extern const void *const *_PyEval_HandlerForms(void);

#else  /* !_PY_JIT_SUPPORTED */

static inline void _PyJit_Retire(_PyCodeExec *Py_UNUSED(exec)) {}
static inline void _PyJit_NoteRewrite(_PyCodeExec *Py_UNUSED(exec)) {}
static inline void _PyJit_FreeUnits(_PyCodeExec *Py_UNUSED(exec)) {}

#endif  /* _PY_JIT_SUPPORTED */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_H */
