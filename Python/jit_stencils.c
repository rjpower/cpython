/* The stencils of the baseline JIT: the interpreter's arms
 * (Python/generated_rop_cases.c.h), compiled once more, with the C rows
 * that the normal evaluator inlines (Python/ceval_normal.c), under the
 * bindings below.  The build compiles this unit with the evaluator's flags
 * and the interpreter's code model (CFLAGS_JIT_STENCILS, Makefile.pre.in)
 * and never links it: Tools/jit/stencils.py reads the object and writes
 * the table of stencils, Python/generated_jit_stencils.h, in the build
 * tree.
 *
 * A stencil is its arm as a function _JIT_STENCIL_<arm> of the evaluator's
 * type (Python/ceval_tail.h), with the word's fields and the arm's exits
 * left as relocations against the hole symbols _JIT_*, which the emitter
 * patches where it copies the stencil:
 *
 *   the fields A to D of the word: _JIT_A to _JIT_D, hidden symbols whose
 *   address is the field's value.  The arm reads a field with lea
 *   _JIT_A(%rip) into a 32-bit register (IR_JIT_FIELD below): the patched
 *   displacement is the value less the end of the instruction, modulo
 *   2^32.  The reader refuses a field hole in any other instruction;
 *
 *   the exits: tail calls of the code holes below, each with the arm's
 *   state.  The declared cold exits are tail calls of functions declared
 *   cold, which is what makes GCC move the paths that lead to them into
 *   the stencil's .cold part.
 *
 * The word's own address is next_instr on entry, which compiled code keeps
 * in its register as the interpreter does; the other reads of the word
 * itself (its slot-class flags, flag immediates and branch distances) load
 * it from there.
 *
 * Every arm's holes must be among those that its declaration
 * (Python/generated_jit_stencil_decls.h, from the .rop sources) allows:
 * the reader checks each one. */

#define IR_JIT_STENCILS 1
#include "ceval_normal.c"   // the shared headers and the inlined rows
#include "ir_targets.h"
#include "ceval_tail.h"

/* The holes.  The names, kinds and coldness are those of the reader's
 * table (Tools/jit/stencils.py, HOLES). */
#define IR_JIT_FIELD_HOLE(X) \
    extern char _JIT_##X[] __attribute__((visibility("hidden")))
IR_JIT_FIELD_HOLE(A);
IR_JIT_FIELD_HOLE(B);
IR_JIT_FIELD_HOLE(C);
IR_JIT_FIELD_HOLE(D);

#define IR_JIT_EXIT(NAME) \
    extern Py_IR_TAIL_CC __attribute__((visibility("hidden"))) \
    PyValue NAME(Py_IR_TAIL_PARAMS)
#define IR_JIT_COLD_EXIT(NAME) \
    extern Py_IR_TAIL_CC __attribute__((visibility("hidden"), cold)) \
    PyValue NAME(Py_IR_TAIL_PARAMS)
IR_JIT_EXIT(_JIT_CONTINUE);         // the next word
IR_JIT_EXIT(_JIT_TARGET);           // the word's branch or jump target
IR_JIT_EXIT(_JIT_SWITCH);           // next_instr of frame, after a transfer
IR_JIT_COLD_EXIT(_JIT_REDIRECT);    // next_instr of frame, after a redirect
IR_JIT_COLD_EXIT(_JIT_AGAIN);       // the word again, under its new arm
IR_JIT_COLD_EXIT(_JIT_MISS);        // the family's generic arm, at the word
/* The VM routines (Python/ceval_routines.h, Python/ceval_evaluator.h) that
 * arms leave through: an arm that reaches any other does not compile. */
IR_JIT_COLD_EXIT(_JIT_LABEL_process_error);
IR_JIT_COLD_EXIT(_JIT_LABEL_close_dispatch);
IR_JIT_EXIT(_JIT_LABEL_gen_return);

/* The arguments of an exit: the arm's state, and the word for the
 * interpreter's arms (the generic arm, the arm the word now names), which
 * decode it.  Every other exit takes an unspecified word, which costs no
 * instruction. */
#define IR_JIT_NO_WORD() \
    ({ _PyIRWord ir_jit_word; __asm__("" : "=r"(ir_jit_word)); ir_jit_word; })
#define IR_JIT_GOTO(HOLE, WORD) \
    do { \
        Py_MUSTTAIL return HOLE(tstate, frame, next_instr, (WORD)); \
    } while (0)

/* The bindings of the arms */
#undef TARGET
#define TARGET(op) \
    Py_IR_TAIL_CC PyValue _JIT_STENCIL_##op( \
        PyThreadState *tstate, _PyInterpreterFrame *frame, \
        _PyInstr *next_instr, _PyIRWord ir_jit_word_in)

/* A field is 12 bits (Include/internal/pycore_ir.h).  The hole is the
 * displacement of a 32-bit lea, whose result x86 zero-extends; GCC, told
 * only of a uint32_t, may compute it with a 64-bit lea and mask it after
 * scaling it into an address (movabs, shl, and).  So the binding is the
 * 32-bit lea itself, a 64-bit value, with the field's range: a scaled field
 * then folds into the address that uses it. */
#undef IR_A
#undef IR_B
#undef IR_C
#undef IR_D
#define IR_JIT_FIELD(X) \
    ({ \
        uint64_t ir_jit_field; \
        __asm__("lea _JIT_" #X "(%%rip), %k0" : "=r"(ir_jit_field)); \
        if (ir_jit_field > IR_FIELD_MAX) { \
            __builtin_unreachable(); \
        } \
        (uint32_t)ir_jit_field; \
    })
#define IR_A(W) IR_JIT_FIELD(A)
#define IR_B(W) IR_JIT_FIELD(B)
#define IR_C(W) IR_JIT_FIELD(C)
#define IR_D(W) IR_JIT_FIELD(D)
#define ir_word FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr)

#undef DISPATCH
#define DISPATCH() IR_JIT_GOTO(_JIT_CONTINUE, IR_JIT_NO_WORD())
#undef IR_BRANCH
#define IR_BRANCH(T) \
    do { \
        next_instr = (T); \
        IR_JIT_GOTO(_JIT_TARGET, IR_JIT_NO_WORD()); \
    } while (0)
/* A frame switch (an entry, a return, a suspension) goes on at next_instr
 * of frame: at its copy, in the frame's unit, if the code has live entries
 * and next_instr is not the switch word, as _PyEval_JitDispatch() does.
 * The copy jumps there itself, through an indirect jump of its own that
 * the branch predictor keeps apart per site; like every exit from a copy,
 * it first closes the departure window.  Otherwise it leaves for the
 * evaluator (_JIT_SWITCH). */
#undef IR_DISPATCH_SWITCHED
#define IR_DISPATCH_SWITCHED() \
    do { \
        const void *const *ir_entries = _PyFrame_JitEntries(frame); \
        if (ir_entries != NULL && next_instr != &_PyEval_SwitchWord && \
            !IR_JIT_TRACED()) { \
            ((_PyThreadStateImpl *)tstate)->jit_departing = 0; \
            IR_JIT_SWITCH_STAT(); \
            Py_MUSTTAIL return ((_PyIRTailFn)ir_entries[next_instr - frame->ir_words])( \
                tstate, frame, next_instr, IR_JIT_NO_WORD()); \
        } \
        IR_JIT_GOTO(_JIT_SWITCH, IR_JIT_NO_WORD()); \
    } while (0)
#undef IR_DISPATCH_REDIRECTED
#define IR_DISPATCH_REDIRECTED() IR_JIT_GOTO(_JIT_REDIRECT, IR_JIT_NO_WORD())
#undef DISPATCH_SAME_OPARG
#define DISPATCH_SAME_OPARG() IR_JIT_GOTO(_JIT_AGAIN, ir_word)
#undef JUMP_TO_PREDICTED
#define JUMP_TO_PREDICTED(name) \
    do { \
        Py_MUSTTAIL return _JIT_MISS(tstate, frame, this_instr, ir_word); \
    } while (0)
#undef JUMP_TO_LABEL
#define JUMP_TO_LABEL(name) IR_JIT_GOTO(_JIT_LABEL_##name, IR_JIT_NO_WORD())
/* An arm's tail closes (ROP_TAIL_CLOSE, Python/ceval_macros.h) close what
 * they parked by a call, and the copy goes on to its next word's or its
 * target's copy, as the arm would without them: the frame stays in the
 * unit */
#undef ROP_TAIL_DISPATCH
#define ROP_TAIL_DISPATCH() \
    do { \
        _PyEval_CloseParked(tstate); \
        DISPATCH(); \
    } while (0)
#undef ROP_TAIL_BRANCH
#define ROP_TAIL_BRANCH(T) \
    do { \
        _PyInstr *rop_close_target = (T); \
        _PyEval_CloseParked(tstate); \
        IR_BRANCH(rop_close_target); \
    } while (0)
/* An outlined cold region runs in the normal evaluator, from its entry for
 * compiled code (IR_OUTLINED_JIT_ENTRY, Python/ceval_evaluator.h): a symbol
 * of libpython, which the reader takes as an EXTERN hole */
#undef IR_OUTLINE_GOTO
#define IR_OUTLINE_GOTO(NAME) \
    do { \
        extern Py_IR_TAIL_CC __attribute__((visibility("hidden"), cold)) \
        PyValue _PyEval_JitOut_##NAME(PyThreadState *, _PyInterpreterFrame *, \
                                      _PyInstr *, _PyIRWord); \
        Py_MUSTTAIL return _PyEval_JitOut_##NAME(tstate, frame, this_instr, ir_word); \
    } while (0)
/* A declared region (`outline`): the function itself, which every copy of
 * the arm and the evaluator's arm run, after the copy closes the departure
 * window as its other exits do */
#undef IR_OUTLINE_HOT_GOTO
#define IR_OUTLINE_HOT_GOTO(NAME) \
    do { \
        extern Py_IR_TAIL_CC __attribute__((visibility("hidden"))) \
        PyValue _PyEval_JitOut_##NAME(PyThreadState *, _PyInterpreterFrame *, \
                                      _PyInstr *, _PyIRWord); \
        ((_PyThreadStateImpl *)tstate)->jit_departing = 0; \
        Py_MUSTTAIL return _PyEval_JitOut_##NAME(tstate, frame, this_instr, ir_word); \
    } while (0)
/* A departing arm opens the thread's departure window (Python/jit.c,
 * _PyJit_Reclaim()): the copy runs on after its frame, and maybe its code,
 * is gone.  The exits into the evaluator close it. */
#undef IR_MARK_DEPARTURE
#define IR_MARK_DEPARTURE() (((_PyThreadStateImpl *)tstate)->jit_departing = 1)
/* Compiled code is the unit: no warm-up event, no switch into it */
#undef IR_JIT_EVENT_AT
#define IR_JIT_EVENT_AT(NEXT) ((void)(NEXT))
#undef IR_JIT_SWITCH
#define IR_JIT_SWITCH() ((void)0)

/* Each arm's declaration, in the object for the reader:
 * "name generic|fields|exits" */
#define IR_JIT_DECL(NAME, GENERIC, FIELDS, EXITS) \
    __attribute__((section(".jit_decls"), used)) \
    static const char ir_jit_decl_##NAME[] = #NAME " " #GENERIC "|" FIELDS "|" EXITS;
#include "generated_jit_stencil_decls.h"

/* As for the evaluator (Python/ceval_evaluator.h) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-musttail-local-addr"
#endif
#include "generated_rop_cases.c.h"
#pragma GCC diagnostic pop
