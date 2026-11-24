/* The baseline JIT: units, warm-up and the switches
 * (Include/internal/pycore_jit.h, pyjit/design/proposals/tiers87.md Phase 2).
 *
 * B1 compiles every word as a call of the function form of the handler that
 * the word names at compile time (ir_handler_forms in Python/ceval.c), and
 * continues natively when the form's transfer is the same frame and a word
 * known at compile time.  The machine code goes through the assembler and
 * code memory of Python/jit_asm.c and Python/jit_memory.c. */

#include "Python.h"
#include "pycore_ceval.h"           // _PyEvalActivation
#include "pycore_initconfig.h"      // _Py_GetEnv()
#include "pycore_interpframe.h"     // _PyFrame_Exec()
#include "pycore_ir.h"
#include "pycore_jit.h"
#include "pycore_jit_unwind.h"
#include "pycore_pystate.h"         // _PyInterpreterState_GET()

#include <ctype.h>                  // isdigit()
#include <errno.h>
#include <wchar.h>

/* The mode named by value, or -1.  Upstream's PYTHON_JIT=0 and 1 are off
 * and on. */
static int
mode_named(const char *value)
{
    if (strcmp(value, "off") == 0 || strcmp(value, "0") == 0) {
        return _PyJIT_OFF;
    }
    if (strcmp(value, "on") == 0 || strcmp(value, "1") == 0) {
        return _PyJIT_ON;
    }
    if (strcmp(value, "always") == 0) {
        return _PyJIT_ALWAYS;
    }
    return -1;
}

PyStatus
_PyJit_InitInterpreter(PyInterpreterState *interp)
{
    const PyConfig *config = &interp->config;
    const wchar_t *option = _Py_get_xoption(&config->xoptions, L"jit");
    int mode = _PyJIT_OFF;
    if (option != NULL) {
        const wchar_t *sep = wcschr(option, L'=');
        char value[8] = "";
        if (sep != NULL && wcslen(sep + 1) < sizeof(value)) {
            for (size_t i = 0; sep[1 + i] != L'\0'; i++) {
                value[i] = sep[1 + i] < 128 ? (char)sep[1 + i] : '?';
            }
        }
        mode = mode_named(value);
        if (mode < 0) {
            return _PyStatus_ERR("-X jit: expected off, on or always");
        }
    }
    else {
        const char *env = _Py_GetEnv(config->use_environment, "PYTHON_JIT");
        if (env != NULL) {
            mode = mode_named(env);
            if (mode < 0) {
                return _PyStatus_ERR("PYTHON_JIT: expected off, on, always, 0 or 1");
            }
        }
    }
    /* -X ir_forms, a test of the function forms, runs no unit: the JIT is
     * off under it */
    if (mode != _PyJIT_OFF &&
        (interp->ceval.ir_forms || !_PyJit_IsAvailable())) {
        mode = _PyJIT_OFF;
    }
    interp->ceval.jit = mode;
    interp->ceval.jit_warmup = mode == _PyJIT_ALWAYS ? 1 : _PyJIT_WARMUP;
    /* PYTHON_JIT_WARMUP=N: the warm-up events before a code compiles under
     * on, from startup (always compiles at the first event) */
    const char *warmup = _Py_GetEnv(config->use_environment, "PYTHON_JIT_WARMUP");
    if (warmup != NULL) {
        char *end = NULL;
        errno = 0;
        unsigned long n = isdigit((unsigned char)warmup[0])
                          ? strtoul(warmup, &end, 10) : 0;
        if (end == NULL || *end != '\0' || errno != 0 || n == 0 || n > UINT32_MAX) {
            return _PyStatus_ERR("PYTHON_JIT_WARMUP: expected an integer in 1..2**32-1");
        }
        if (mode == _PyJIT_ON) {
            interp->ceval.jit_warmup = (uint32_t)n;
        }
    }
    return _PyStatus_OK();
}

int
_PyJit_PinDeparting(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyEvalActivation *activation = ((_PyThreadStateImpl *)tstate)->eval_activation;
    PyObject *old = activation->jit_pin;
    activation->jit_pin = Py_NewRef(frame->f_executable);
    if (old == NULL) {
        return 0;
    }
    int status = Py_REFCNT(old) == 1 ? _PyIR_ESCAPED : 0;
    Py_DECREF(old);
    return status;
}

#ifndef _PY_JIT_SUPPORTED

int
_PyJit_IsAvailable(void)
{
    return 0;
}

#else

#include "pycore_jit_asm.h"
#include "pycore_jit_lower.h"       // _PyJit_Templates

int
_PyJit_IsAvailable(void)
{
    return _PyJitMemory_Init() == 0;
}

/* Where an operand word's entry leads (no transfer names one), and in a
 * debug build a frame whose base moved. */
static void
bad_entry(void)
{
    Py_FatalError("baseline JIT: an operand word entered, a frame's base moved, "
                  "or a switch named a frame that is not current");
}

/* What compiled code tests after a word: RETIRED after a word that may run
 * Python (which may have enabled monitoring), anything at a backward jump,
 * where a stale unit is replaced. */
#define MAY_RUN_PYTHON \
    (_PyIR_ESCAPES | _PyIR_RUNS_PYTHON | _PyIR_DEOPTS | _PyIR_PUSHES_FRAME | \
     _PyIR_POLLS)

/* The static target of the word at i, a head word, or -1 */
static int
static_target(const _PyIRWord *words, uint32_t i)
{
    _PyIRWord w = words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(w));
    if (fmt->flags & _PyIR_NAMES_TARGET) {
        return -1;
    }
    if (fmt->flags & _PyIR_JUMPS) {
        return (int)IR_CD(w);
    }
    if (fmt->flags & _PyIR_FORWARD) {
        return (int)i + 1 + IR_DS(w);
    }
    return -1;
}

#define R(X) _PyJitReg_##X
#define AT(B, D) _PyJitMem_At(R(B), (D))
#define WORD_AT(I) AT(R14, (int32_t)((I) * sizeof(_PyIRWord)))

/* The registers that a unit's frame saves below rbp, in push order, and
 * their DWARF numbers. */
static const struct {
    _PyJitReg reg;
    unsigned dwarf;
} saved_regs[] = {
    {R(RBX), DWRF_REG_BX}, {R(R12), DWRF_REG_12}, {R(R13), DWRF_REG_13},
    {R(R14), DWRF_REG_14}, {R(R15), DWRF_REG_15},
};
#define NSAVED ((int)Py_ARRAY_LENGTH(saved_regs))

/* Where the frame's rules change: labels after the instructions of the
 * prologue and of the shared exit (emit_frame), for build_unwind(). */
typedef struct {
    _PyJitLabel pushed_rbp;
    _PyJitLabel framed;                 // after mov rbp, rsp
    _PyJitLabel pushed[NSAVED];
    _PyJitLabel popped[NSAVED];
    _PyJitLabel popped_rbp;             // at the ret
    _PyJitLabel returned;               // after the ret
} _PyJitFrameMarks;

/* The state of one unit's compilation: the words, a label per head word
 * (-1 for the operand words) and the unit's shared exits.  Every piece
 * below may be emitted anywhere in the code, hot or cold: none falls
 * through to what follows it. */
typedef struct {
    _PyJitAsm *a;
    PyCodeObject *code;
    _PyJitUnit *unit;
    const _PyIRWord *words;
    uint32_t nwords;
    _PyJitLabel *labels;
    _PyJitLabel leave;      // leave the unit with the transfer in rax:rdx
    _PyJitLabel backedge;   // a backward jump of an invalid unit, in its frame
    _PyJitLabel live;       // call the live handler of the word at rdx
    _PyJitLabel trap;       // an operand word's entry, a broken invariant
    _PyJitLabel switch_routine; // enter the current frame's unit (emit_frame)
    _PyJitFrameMarks marks;
    _PyJitLabel eh_frame;   // the unwind info, after the code
    /* The code has no compile left after this one: its unit is never marked
     * stale, so its code carries no mark */
    int capped;
} _PyJitCompiler;

/* A label here */
static _PyJitLabel
mark(_PyJitAsm *a)
{
    _PyJitLabel label = _PyJitAsm_NewLabel(a);
    _PyJitAsm_Bind(a, label);
    return label;
}

#ifdef Py_DEBUG
/* A frame that lltrace traces at level 5 runs in the evaluator, as
 * jit_unit_of() in Python/ceval.c decides */
static int
frame_traced(_PyInterpreterFrame *frame)
{
    return frame->lltrace >= 5;
}
#endif

/* The switch routine of emit_frame() (g3/design.md section 1), called at a
 * frame switch, when only r12 is known: the frame to run is
 * tstate->current_frame at its instr_ptr, an instruction head of its
 * executing copy, which the switching code has published.  It loads r13,
 * r14, r15 and rbx for that frame and returns the address to jump to in rax:
 * the entry of the frame's word in its code's newest unit.  A unit is
 * entered on the native frame that is running, whichever unit built it: every
 * unit's frame is this function's.  The cold paths are the driver's rules: a
 * frame whose code has no unit leaves the unit, with the frame and its word,
 * for jit_run(); an invalid unit goes through the warm-up event, which
 * recompiles a stale one (or clears STALE at the cap) and refuses a retired
 * one.  It sets the frame's _PyFrame_JIT_RESUME flag and tstate's jit_frame
 * as jit_run() does, and counts the switch. */
static void
emit_switch_routine(_PyJitCompiler *c)
{
    _PyJitAsm *a = c->a;
    _PyJitLabel enter = _PyJitAsm_NewLabel(a);
    _PyJitLabel invalid = _PyJitAsm_NewLabel(a);
    _PyJitLabel none = _PyJitAsm_NewLabel(a);
#ifndef NDEBUG
    _PyJitLabel bad = _PyJitAsm_NewLabel(a);
#endif
    _PyJitAsm_Bind(a, c->switch_routine);
    _PyJitAsm_MovRM(a, R(R13), AT(R12, offsetof(PyThreadState, current_frame)));
#ifdef Py_DEBUG
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(RDI), R(R13));
    _PyJitAsm_CallAbs(a, (const void *)frame_traced);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_TestRR32(a, R(RAX), R(RAX));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, none);
#endif
    /* The executing copy's header lies before its words (_PyFrame_Exec()) */
    _PyJitAsm_MovRM(a, R(R14), AT(R13, offsetof(_PyInterpreterFrame, ir_words)));
    _PyJitAsm_MovRM(a, R(RAX), AT(R14, (int32_t)offsetof(_PyCodeExec, jit_unit) -
                                       (int32_t)sizeof(_PyCodeExec)));
    _PyJitAsm_TestRR(a, R(RAX), R(RAX));
    _PyJitAsm_Jcc(a, _PyJitCond_E, none);
    _PyJitAsm_MovzxRM8(a, R(RCX), AT(RAX, offsetof(_PyJitUnit, invalid)));
    _PyJitAsm_TestRR(a, R(RCX), R(RCX));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, invalid);
    _PyJitAsm_Bind(a, enter);
    _PyJitAsm_MovRR(a, R(RBX), R(RAX));
    _PyJitAsm_MovRM(a, R(R15), AT(R13, offsetof(_PyInterpreterFrame, base)));
    _PyJitAsm_MovzxRM8(a, R(RCX), AT(R13, offsetof(_PyInterpreterFrame, jit_flags)));
    _PyJitAsm_AluRI(a, _PyJitAlu_OR, R(RCX), _PyFrame_JIT_RESUME);
    _PyJitAsm_MovMR8(a, AT(R13, offsetof(_PyInterpreterFrame, jit_flags)), R(RCX));
    _PyJitAsm_MovMR(a, AT(R12, offsetof(_PyThreadStateImpl, jit_frame)), R(R13));
    _PyJitAsm_MovRM(a, R(RCX), AT(R12, offsetof(PyThreadState, interp)));
    _PyJitAsm_UnaryM(a, _PyJitUnary_INC,
                     AT(RCX, offsetof(PyInterpreterState, ceval.jit_switches)));
    /* Words and entries are both 8 bytes: the word's offset indexes both */
    static_assert(sizeof(_PyIRWord) == sizeof(void *), "a word is an entry's size");
    _PyJitAsm_MovRM(a, R(RDX), AT(R13, offsetof(_PyInterpreterFrame, instr_ptr)));
    _PyJitAsm_AluRR(a, _PyJitAlu_SUB, R(RDX), R(R14));
#ifndef NDEBUG
    /* A successor out of the frame's words is a broken invariant (a payload
     * word's entry is trap in every build); the activation's record names
     * the unit entered */
    _PyJitAsm_MovRM32(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, nwords)));
    _PyJitAsm_ShiftRI(a, _PyJitShift_SHL, R(RCX), 3);
    _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
    _PyJitAsm_Jcc(a, _PyJitCond_AE, bad);
    _PyJitAsm_MovRM(a, R(RCX), AT(R12, offsetof(_PyThreadStateImpl, eval_activation)));
    _PyJitAsm_MovMR(a, AT(RCX, offsetof(_PyEvalActivation, jit_unit)), R(RBX));
#endif
    _PyJitAsm_MovRM(a, R(RAX), AT(RBX, offsetof(_PyJitUnit, entries)));
    _PyJitAsm_MovRM(a, R(RAX), _PyJitMem_Indexed(R(RAX), R(RDX), 0, 0));
    _PyJitAsm_Ret(a);

    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    _PyJitAsm_Bind(a, invalid);
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_CallAbs(a, (const void *)_PyJit_Event);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_TestRR(a, R(RAX), R(RAX));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, enter);
    /* No unit to run: drop the return address and leave with the frame and
     * its word */
    _PyJitAsm_Bind(a, none);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_MovRM(a, R(RDX), AT(R13, offsetof(_PyInterpreterFrame, instr_ptr)));
    _PyJitAsm_Jmp(a, c->leave);
#ifndef NDEBUG
    /* trap, with the stack as at a word boundary */
    _PyJitAsm_Bind(a, bad);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_Jmp(a, c->trap);
#endif
    _PyJitAsm_Section(a, section);
}

/* The entry, the shared exits and the stubs.  Registers at a word boundary:
 * r12 tstate, r13 frame, r14 word 0, r15 frame->base, rbx the unit; rsp is
 * 16-byte aligned.
 *
 * The frame is the one rule that the unwind info of build_unwind() states:
 * rbp, set by the prologue, points at the saved rbp, so from there to the
 * shared exit's pops the CFA is rbp + 16, the return address is at CFA - 8,
 * and rbp and the registers of saved_regs are at CFA - 16, - 24 and below.
 * The unit's code may change rsp freely (the stubs push; calls go below it),
 * but nothing between the prologue and the exit may write rbp or the save
 * slots, or run the exit's pops and ret anywhere else: the unwinder would
 * read the wrong frame.  A template that needs rbp, or that leaves the unit
 * by another path, must change the marks and the rows with it. */
static void
emit_frame(_PyJitCompiler *c)
{
    _PyJitAsm *a = c->a;
    _PyJitFrameMarks *m = &c->marks;
    /* enter(tstate, frame, address of the word's block) */
    _PyJitAsm_Push(a, R(RBP));
    m->pushed_rbp = mark(a);
    _PyJitAsm_MovRR(a, R(RBP), R(RSP));
    m->framed = mark(a);
    for (int i = 0; i < NSAVED; i++) {
        _PyJitAsm_Push(a, saved_regs[i].reg);
        m->pushed[i] = mark(a);
    }
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), _PyJIT_SPILL_BYTES);
    _PyJitAsm_MovRR(a, R(R12), R(RDI));
    _PyJitAsm_MovRR(a, R(R13), R(RSI));
    _PyJitAsm_MovRM(a, R(R14), AT(R13, offsetof(_PyInterpreterFrame, ir_words)));
    _PyJitAsm_MovRM(a, R(R15), AT(R13, offsetof(_PyInterpreterFrame, base)));
    _PyJitAsm_MovImm(a, R(RBX), (int64_t)(uintptr_t)c->unit);
    _PyJitAsm_JmpR(a, R(RDX));

    _PyJitAsm_Bind(a, c->leave);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), _PyJIT_SPILL_BYTES);
    for (int i = NSAVED - 1; i >= 0; i--) {
        _PyJitAsm_Pop(a, saved_regs[i].reg);
        m->popped[i] = mark(a);
    }
    _PyJitAsm_Pop(a, R(RBP));
    m->popped_rbp = mark(a);
    _PyJitAsm_Ret(a);
    m->returned = mark(a);
    emit_switch_routine(c);

    /* The stubs are cold */
    _PyJitAsm_Section(a, _PyJitSection_COLD);

    /* A warm-up event, which may recompile; then leave with the transfer,
     * whose frame is the unit's. */
    _PyJitAsm_Bind(a, c->backedge);
    _PyJitAsm_Push(a, R(RDX));
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, R(RSP), 8);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_CallAbs(a, (const void *)_PyJit_Event);
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, R(RSP), 8);
    _PyJitAsm_Pop(a, R(RDX));
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_Jmp(a, c->leave);

    _PyJitAsm_Bind(a, c->trap);
    _PyJitAsm_CallAbs(a, (const void *)bad_entry);

    /* Called with rdx at a word whose handler is no longer the one compiled:
     * runs the word's live handler by its form, as the word's own call
     * would. */
    _PyJitAsm_Bind(a, c->live);
    _PyJitAsm_MovRM(a, R(RCX), AT(RDX, 0));
    _PyJitAsm_MovRR(a, R(RAX), R(RCX));
    _PyJitAsm_AluRI(a, _PyJitAlu_AND, R(RAX), IR_HANDLER_MASK);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_MovImm(a, R(R8), (int64_t)(uintptr_t)_PyEval_HandlerForms());
    _PyJitAsm_JmpM(a, _PyJitMem_Indexed(R(R8), R(RAX), 3, 0));
    _PyJitAsm_Section(a, _PyJitSection_HOT);
}

/* Advance the unwind info's location to the offset of label. */
static void
advance_to(const _PyJitAsm *a, _PyJitUnwind *u, size_t *at, _PyJitLabel label)
{
    size_t offset = _PyJitAsm_LabelOffset(a, label);
    assert(offset >= *at);
    _PyJitUnwind_Advance(u, (uint32_t)(offset - *at));
    *at = offset;
}

/* The unit's .eh_frame, in buffer, for code_size bytes of code: the frame
 * of emit_frame, row by row (the rule that holds up to the first row is the
 * CIE's: CFA = rsp + 8, the return address at CFA - 8).
 *
 *   after push rbp          CFA = rsp + 16, rbp at CFA - 16
 *   after mov rbp, rsp      CFA = rbp + 16: the body's rule, which holds
 *                           through both sections
 *   after each push         that register at CFA - 24, - 32, ...
 *   the shared exit         remember the body's rule, then undo it pop by
 *                           pop: each register is restored, and after pop
 *                           rbp the CFA is rsp + 8
 *   after ret               the body's rule again: the code that follows
 *                           is the first word's block, then the cold stubs
 *
 * Returns the size with libgcc's terminator. */
static size_t
build_unwind(const _PyJitCompiler *c, size_t code_size, uint8_t *buffer)
{
    const _PyJitAsm *a = c->a;
    const _PyJitFrameMarks *m = &c->marks;
    _PyJitUnwind u;
    size_t at = 0;
    _PyJitUnwind_Begin(&u, buffer, code_size);
    advance_to(a, &u, &at, m->pushed_rbp);
    _PyJitUnwind_DefCfaOffset(&u, 16);
    _PyJitUnwind_Offset(&u, DWRF_REG_BP, 2);
    advance_to(a, &u, &at, m->framed);
    _PyJitUnwind_DefCfaRegister(&u, DWRF_REG_BP);
    for (int i = 0; i < NSAVED; i++) {
        advance_to(a, &u, &at, m->pushed[i]);
        _PyJitUnwind_Offset(&u, saved_regs[i].dwarf, 3 + i);
    }
    advance_to(a, &u, &at, c->leave);
    _PyJitUnwind_RememberState(&u);
    for (int i = NSAVED - 1; i >= 0; i--) {
        advance_to(a, &u, &at, m->popped[i]);
        _PyJitUnwind_Restore(&u, saved_regs[i].dwarf);
    }
    advance_to(a, &u, &at, m->popped_rbp);
    _PyJitUnwind_DefCfa(&u, DWRF_REG_SP, 8);
    _PyJitUnwind_Restore(&u, DWRF_REG_BP);
    advance_to(a, &u, &at, m->returned);
    _PyJitUnwind_RestoreState(&u);
    _PyJitUnwind_End(&u);
    return _PyJitUnwind_Terminate(&u);
}

/* A form's transfer to another frame, cold: a call, a return, a yield or a
 * resume goes on at its successor through the switch routine, in the unit
 * of that frame's code (g3/design.md section 1); one that raised or exited
 * leaves for the error route.  The form has made the frame current. */
static void
emit_other_frame(_PyJitCompiler *c, _PyJitLabel other)
{
    _PyJitAsm *a = c->a;
    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    _PyJitAsm_Bind(a, other);
    _PyJitAsm_TestRR(a, R(RDX), R(RDX));
    _PyJitAsm_Jcc(a, _PyJitCond_E, c->leave);
#ifndef NDEBUG
    _PyJitAsm_AluRM(a, _PyJitAlu_CMP, R(RAX),
                    AT(R12, offsetof(PyThreadState, current_frame)));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, c->trap);
#endif
    _PyJitAsm_MovMR(a, AT(RAX, offsetof(_PyInterpreterFrame, instr_ptr)), R(RDX));
    _PyJitAsm_Call(a, c->switch_routine);
    _PyJitAsm_JmpR(a, R(RAX));
    _PyJitAsm_Section(a, section);
}

/* The continuation of word i, with its transfer in rax:rdx: the next word or
 * the word's static target in the same frame by a native jump, another
 * frame by the switch routine, anything else by leaving.  The unit's invalid
 * byte is tested after a word that may run Python (RETIRED: monitoring went
 * on) and at a backward jump (either: the backedge stub may recompile). */
static void
emit_continuation(_PyJitCompiler *c, uint32_t i)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(w));
    uint32_t next = i + (uint32_t)_PyIR_WordLength(w);
    _PyJitLabel other = _PyJitAsm_NewLabel(a);
    _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RAX), R(R13));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, other);
    emit_other_frame(c, other);
    int target = static_target(c->words, i);
    if (target >= 0 && ((uint32_t)target >= c->nwords || c->labels[target] < 0)) {
        target = -1;
    }
    if (target >= 0 && (uint32_t)target <= i) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRR(a, R(RCX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, c->backedge);
    }
    else if (fmt->effects & MAY_RUN_PYTHON) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRI(a, R(RCX), _PyJIT_RETIRED);
        _PyJitAsm_Jcc(a, _PyJitCond_NE, c->leave);
    }
    if (target >= 0) {
        _PyJitAsm_Lea(a, R(RCX), WORD_AT((uint32_t)target));
        _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_E, c->labels[target]);
    }
    if (!(fmt->flags & _PyIR_JUMP_ALWAYS) && next < c->nwords) {
        _PyJitAsm_Lea(a, R(RCX), WORD_AT(next));
        _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_E, c->labels[next]);
    }
    _PyJitAsm_Jmp(a, c->leave);
}

/* STALE into the unit's invalid byte, RETIRED kept; nothing in a capped
 * unit.  Clobbers rcx. */
static void
emit_mark_stale(_PyJitCompiler *c)
{
    if (c->capped) {
        return;
    }
    _PyJitAsm *a = c->a;
    _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
    _PyJitAsm_AluRI(a, _PyJitAlu_OR, R(RCX), _PyJIT_STALE);
    _PyJitAsm_MovMR8(a, AT(RBX, offsetof(_PyJitUnit, invalid)), R(RCX));
}

/* Leave the unit for word dest of the frame */
static void
emit_leave_at(_PyJitCompiler *c, uint32_t dest)
{
    _PyJitAsm *a = c->a;
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(dest));
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_Jmp(a, c->leave);
}

/* The jump to word dest's block, or the leave when it has none */
static void
emit_goto(_PyJitCompiler *c, uint32_t dest)
{
    if (dest < c->nwords && c->labels[dest] >= 0) {
        _PyJitAsm_Jmp(c->a, c->labels[dest]);
    }
    else {
        emit_leave_at(c, dest);
    }
}

/* If RETIRED is set, go to leave_dest.  Clobbers rcx. */
static void
emit_retired_test(_PyJitCompiler *c, _PyJitLabel leave_dest)
{
    _PyJitAsm *a = c->a;
    _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
    _PyJitAsm_TestRI(a, R(RCX), _PyJIT_RETIRED);
    _PyJitAsm_Jcc(a, _PyJitCond_NE, leave_dest);
}

/* A call of the completion stub fn on word i: its status in eax */
static void
emit_stub_call(_PyJitCompiler *c, uint32_t i, const void *fn)
{
    _PyJitAsm *a = c->a;
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(i));
    _PyJitAsm_CallAbs(a, fn);
}

typedef int (*completion_stub)(PyThreadState *, _PyInterpreterFrame *, _PyIRWord *);

/* The plan that word w compiles to.  A unit at the recompile cap is never
 * marked or recompiled, so it plans nothing: each word keeps its handler
 * check and runs the live handler's form after the handler changes, as a
 * planned arm, which is compiled for one handler, cannot. */
const _PyIRStubPlan *
_PyJit_PlanOf(_PyIRWord w, int capped)
{
    static const _PyIRStubPlan none = {_PyIR_PLAN_NONE};
    return capped ? &none : &_PyIR_StubPlans[_PyJit_CompiledHandler(w)];
}

static const _PyIRStubPlan *
plan_of(_PyJitCompiler *c, _PyIRWord w)
{
    return _PyJit_PlanOf(w, c->capped);
}

/* r8 of resolve_status(): the word's handler bits as compiled */
#define COMPILED_HANDLER_BITS 0xFFFF

/* What resolve_status() decides, in rax:rdx as the unit's transfers are */
typedef struct {
    _PyInterpreterFrame *frame;
    _PyIRWord *next;
} resolved_status;

/* The cold path of a planned word's nonzero status (g1a/abi.md), shared by
 * every site: a miss runs the family's generic stub on the word, then the
 * plan-change test (staleness by execution, rd/design.md section 6: the
 * unit is marked only if the word's handler is no longer the one compiled;
 * RETIRED is kept; a unit at the cap has no planned words).  Returns a
 * frame and the word to leave for (no word: the error route, whose exit
 * the stub has set), or no frame to go on in the unit at the word
 * returned: the next word, or the same word again after a redispatch.  A
 * status with ESCAPED tests RETIRED before its successor.  No planned arm
 * branches: no status is BRANCH. */
static resolved_status
resolve_status(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyIRWord *site,
               int status, uint32_t compiled, _PyJitUnit *unit)
{
    if (status & _PyIR_MISS) {
        assert(status == _PyIR_MISS);
        int handler = _PyJit_CompiledHandler((_PyIRWord)(compiled & COMPILED_HANDLER_BITS));
        completion_stub generic = (completion_stub)_PyIR_StubPlans[handler].generic;
        status = generic(tstate, frame, site);
        assert(!(status & _PyIR_MISS));
    }
    /* The word's handler is not the one the unit compiled: a guard miss
     * or a specialisation found it (MISS, AGAIN), or a generic arm, which
     * never misses, ran on a word that specialised elsewhere */
    if ((*site & 0xFFFF) != (compiled & COMPILED_HANDLER_BITS)) {
        unit->invalid |= _PyJIT_STALE;
    }
    if (status & _PyIR_RAISED) {
        return (resolved_status){frame, NULL};
    }
    _PyIRWord *next = (status & _PyIR_AGAIN) ? site : site + _PyIR_WordLength(*site);
    if ((status & _PyIR_ESCAPED) && (unit->invalid & _PyJIT_RETIRED)) {
        return (resolved_status){frame, next};
    }
    return (resolved_status){NULL, next};
}

/* The cold path of planned word i, in the cold section: status (or -1),
 * entered with a stub's nonzero status in eax, or miss (or -1), entered
 * when the arm's guard failed with the word untouched.  resolve_status()
 * decides; the unit leaves with its transfer, or goes on at the next word
 * or the word again. */
static void
emit_status_cold(_PyJitCompiler *c, uint32_t i, _PyJitLabel status, _PyJitLabel miss)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    uint32_t next = i + (uint32_t)_PyIR_WordLength(w);
    _PyJitLabel resolve = _PyJitAsm_NewLabel(a);
    if (miss >= 0) {
        _PyJitAsm_Bind(a, miss);
        _PyJitAsm_MovImm(a, R(RCX), _PyIR_MISS);
        if (status >= 0) {
            _PyJitAsm_Jmp(a, resolve);
        }
    }
    if (status >= 0) {
        _PyJitAsm_Bind(a, status);
        if (_PyIR_StubPlans[_PyJit_CompiledHandler(w)].escapes &&
            next < c->nwords && c->labels[next] >= 0) {
            /* ESCAPED alone, the status of every completion of an arm that
             * declares it may run Python (a generic arm): the RETIRED
             * test and the plan-change test inline, the resolver only for a
             * retired unit or a changed handler.  Only the status's low
             * bits are defined. */
            _PyJitLabel general = _PyJitAsm_NewLabel(a);
            _PyJitAsm_TestRI(a, R(RAX),
                             _PyIR_MISS | _PyIR_RAISED | _PyIR_BRANCH | _PyIR_AGAIN);
            _PyJitAsm_Jcc(a, _PyJitCond_NE, general);
            _PyJitAsm_MovzxRM16(a, R(RCX), WORD_AT(i));
            _PyJitAsm_AluRI(a, _PyJitAlu_CMP, R(RCX), (int32_t)(w & 0xFFFF));
            _PyJitAsm_Jcc(a, _PyJitCond_NE, general);
            _PyJitAsm_TestMI(a, AT(RBX, offsetof(_PyJitUnit, invalid)), _PyJIT_RETIRED);
            _PyJitAsm_Jcc(a, _PyJitCond_E, c->labels[next]);
            _PyJitAsm_Bind(a, general);
        }
        _PyJitAsm_MovRR(a, R(RCX), R(RAX));
    }
    _PyJitAsm_Bind(a, resolve);
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(i));
    _PyJitAsm_MovImm(a, R(R8), (int64_t)(w & COMPILED_HANDLER_BITS));
    _PyJitAsm_MovRR(a, R(R9), R(RBX));
    _PyJitAsm_CallAbs(a, (const void *)resolve_status);
    _PyJitAsm_TestRR(a, R(RAX), R(RAX));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, c->leave);
    _PyJitAsm_Lea(a, R(RCX), WORD_AT(i));
    _PyJitAsm_AluRR(a, _PyJitAlu_CMP, R(RDX), R(RCX));
    _PyJitAsm_Jcc(a, _PyJitCond_E, c->labels[i]);
    emit_goto(c, next);
}

/* Word i of a shared arm: a call of its stub, one branch on the status, the
 * fall-through to the next word; the rest is cold.  The arm reads no
 * feedback that another member writes with another meaning (its family's
 * generic serves every member's entry), so the site has no handler check. */
static void
emit_stub_word(_PyJitCompiler *c, uint32_t i, const _PyIRStubPlan *plan)
{
    _PyJitAsm *a = c->a;
    uint32_t next = i + (uint32_t)_PyIR_WordLength(c->words[i]);
    _PyJitLabel status = _PyJitAsm_NewLabel(a);
    emit_stub_call(c, i, plan->stub);
    _PyJitAsm_TestRR32(a, R(RAX), R(RAX));
    _PyJitAsm_Jcc(a, _PyJitCond_NE, status);
    if (!(next < c->nwords && c->labels[next] >= 0)) {
        emit_leave_at(c, next);
    }
    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    emit_status_cold(c, i, status, -1);
    _PyJitAsm_Section(a, section);
}

/* Word i as a call of its handler's function form, with the word's address
 * and bits as immediates, then its continuation.  Specialisation rewrites a
 * word's handler and its feedback entry together, and a handler's guards are
 * only sound against the entry that its own specialisation wrote, so on a
 * word that specialisation can rewrite the compiled handler runs only while
 * the word still names it; otherwise the word's live handler runs.  B2's
 * slow path for a word is this code. */
static void
emit_form_word(_PyJitCompiler *c, uint32_t i)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    _PyJitLabel changed = _PyJitAsm_NewLabel(a);
    _PyJitLabel called = _PyJitAsm_NewLabel(a);
#ifndef NDEBUG
    /* base is fixed while a frame runs */
    _PyJitLabel base_ok = _PyJitAsm_NewLabel(a);
    _PyJitAsm_AluRM(a, _PyJitAlu_CMP, R(R15),
                    AT(R13, offsetof(_PyInterpreterFrame, base)));
    _PyJitAsm_Jcc(a, _PyJitCond_E, base_ok);
    _PyJitAsm_Jmp(a, c->trap);
    _PyJitAsm_Bind(a, base_ok);
#endif
    int guarded = _PyJit_HandlerMayChange(c->code, (int)i);
    if (guarded) {
        _PyJitAsm_MovzxRM16(a, R(RCX), WORD_AT(i));
        _PyJitAsm_AluRI(a, _PyJitAlu_CMP, R(RCX), (int32_t)(w & 0xFFFF));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, changed);
    }
    _PyJitAsm_MovRR(a, R(RDI), R(R12));
    _PyJitAsm_MovRR(a, R(RSI), R(R13));
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(i));
    _PyJitAsm_MovImm(a, R(RCX), (int64_t)w);
    _PyJitAsm_CallAbs(a, _PyEval_HandlerForms()[_PyJit_CompiledHandler(w)]);
    _PyJitAsm_Bind(a, called);
    emit_continuation(c, i);
    if (!guarded) {
        return;
    }

    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    _PyJitAsm_Bind(a, changed);
    /* An executed plan change: the next warm-up event recompiles */
    emit_mark_stale(c);
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(i));
    _PyJitAsm_LeaLabel(a, R(RAX), c->live);
    _PyJitAsm_CallR(a, R(RAX));
    _PyJitAsm_Jmp(a, called);
    _PyJitAsm_Section(a, section);
}

/* Word i, a template's, goes on to word dest of its frame by a native jump
 * (dest > i: the next word or a forward target), with the unit's tests that
 * emit_continuation() makes.  The tests are in a stub at stub when the word
 * needs them; the hot jump to dest is the caller's.  A word whose dest has
 * no block leaves with the transfer. */
static void
emit_template_stub(_PyJitCompiler *c, uint32_t i, uint32_t dest, _PyJitLabel stub)
{
    _PyJitAsm *a = c->a;
    _PyJitAsm_Bind(a, stub);
    _PyJitAsm_Lea(a, R(RDX), WORD_AT(dest));
    _PyJitAsm_MovRR(a, R(RAX), R(R13));
    _PyJitAsm_Jmp(a, dest <= i ? c->backedge : c->leave);
}

/* The tests on the way from word i to dest: a backward jump tests the
 * invalid byte, the hot path of a word that may run Python tests RETIRED.
 * A set bit goes to stub. */
static void
emit_template_tests(_PyJitCompiler *c, uint32_t i, uint32_t dest, int may_run_python,
                    _PyJitLabel stub)
{
    _PyJitAsm *a = c->a;
    if (dest <= i) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRR(a, R(RCX), R(RCX));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, stub);
    }
    else if (may_run_python) {
        _PyJitAsm_MovzxRM8(a, R(RCX), AT(RBX, offsetof(_PyJitUnit, invalid)));
        _PyJitAsm_TestRI(a, R(RCX), _PyJIT_RETIRED);
        _PyJitAsm_Jcc(a, _PyJitCond_NE, stub);
    }
}

static int
has_block(_PyJitCompiler *c, uint32_t dest)
{
    return dest < c->nwords && c->labels[dest] >= 0;
}

/* Word i by its handler's inline template (B2): the handler check of a
 * word that specialisation may rewrite, the template, and the exits.  The
 * template's hot path falls through to the next word, whose block follows,
 * or jumps to its static target.  Its failed guards go, in the cold
 * section, to the family's generic stub when the arm is planned inline
 * (emit_status_cold()), else to the word as emit_form_word() runs it.  The
 * hot path does not run the form, so it may run Python only through the
 * effects that the handler has besides its deopts, and through its
 * releases: a release that is the template's last code tests RETIRED on
 * its slow path, after the destructor, and leaves for the next word; any
 * other release puts the test on the fall-through. */
static void
emit_template_word(_PyJitCompiler *c, uint32_t i, int arm, _PyJitTemplate template)
{
    _PyJitAsm *a = c->a;
    _PyIRWord w = c->words[i];
    /* The effects are the arm's, whose template runs: RESUME_CHECK's for a
     * RESUME_JIT word, which itself may run Python */
    const _PyIRFormat *fmt = _PyIR_FormatOf(arm);
    int planned = plan_of(c, w)->region == _PyIR_PLAN_INLINE;
    int may_run_python = (fmt->effects & (MAY_RUN_PYTHON & ~_PyIR_DEOPTS)) != 0;
    uint32_t next = i + (uint32_t)_PyIR_WordLength(w);
    int target = static_target(c->words, i);
    _PyJitWordCx cx = {
        .a = a, .words = c->words, .word = w, .index = i,
        .target = -1, .slow = _PyJitAsm_NewLabel(a), .leave = c->leave,
    };
    _PyJitLabel target_stub = -1;
    if (target >= 0) {
        if ((uint32_t)target > i && !may_run_python && has_block(c, (uint32_t)target)) {
            cx.target = c->labels[target];
        }
        else {
            cx.target = _PyJitAsm_NewLabel(a);
            target_stub = _PyJitAsm_NewLabel(a);
        }
    }
    /* A planned arm reads no feedback that another member writes (its
     * guards test the operands), so it is sound whatever the word's handler
     * is now: no handler check */
    if (!planned && _PyJit_HandlerMayChange(c->code, (int)i)) {
        _PyJitAsm_MovzxRM16(a, R(RCX), WORD_AT(i));
        _PyJitAsm_AluRI(a, _PyJitAlu_CMP, R(RCX), (int32_t)(w & 0xFFFF));
        _PyJitAsm_Jcc(a, _PyJitCond_NE, cx.slow);
    }
    template(&cx);
    assert(cx.cold_depth == 0);
    /* One release, the template's last code: its slow path tests RETIRED */
    int release_exit = cx.nreleases == 1 &&
        cx.release_end == a->sections[_PyJitSection_HOT].size;
    if (cx.nreleases > 0 && !release_exit) {
        may_run_python = 1;
    }
    /* The fall-through to the next word, whose block follows */
    _PyJitLabel next_stub = -1;
    if (may_run_python || !has_block(c, next)) {
        next_stub = _PyJitAsm_NewLabel(a);
        emit_template_tests(c, i, next, may_run_python, next_stub);
        if (!has_block(c, next)) {
            _PyJitAsm_Jmp(a, next_stub);
        }
    }
    _PyJitSection section = a->current;
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    if (next_stub >= 0) {
        emit_template_stub(c, i, next, next_stub);
    }
    if (target_stub >= 0) {
        /* The jump to the target, with its tests */
        _PyJitAsm_Bind(a, cx.target);
        emit_template_tests(c, i, (uint32_t)target, may_run_python, target_stub);
        if (has_block(c, (uint32_t)target)) {
            _PyJitAsm_Jmp(a, c->labels[target]);
        }
        else {
            _PyJitAsm_Jmp(a, target_stub);
        }
        emit_template_stub(c, i, (uint32_t)target, target_stub);
    }
    for (int k = 0; k < cx.nreleases && k < _PyJit_MAX_RELEASES; k++) {
        _PyJitAsm_Bind(a, cx.release_tail[k]);
        if (!release_exit) {
            _PyJitAsm_Jmp(a, cx.release_back[k]);
            continue;
        }
        _PyJitLabel leave_next = _PyJitAsm_NewLabel(a);
        emit_retired_test(c, leave_next);
        _PyJitAsm_Jmp(a, cx.release_back[k]);
        _PyJitAsm_Bind(a, leave_next);
        emit_leave_at(c, next);
    }
    if (planned) {
        emit_status_cold(c, i, -1, cx.slow);
    }
    else {
        _PyJitAsm_Bind(a, cx.slow);
        emit_form_word(c, i);
    }
    _PyJitAsm_Section(a, section);
}

/* The unit's code: the frame, a block per head word and the entry table,
 * then its unwind info.  Returns the label of the table. */
static _PyJitLabel
emit_unit(_PyJitCompiler *c)
{
    _PyJitAsm *a = c->a;
    c->leave = _PyJitAsm_NewLabel(a);
    c->backedge = _PyJitAsm_NewLabel(a);
    c->live = _PyJitAsm_NewLabel(a);
    c->trap = _PyJitAsm_NewLabel(a);
    c->switch_routine = _PyJitAsm_NewLabel(a);
    for (uint32_t i = 0; i < c->nwords; i += _PyIR_WordLength(c->words[i])) {
        c->labels[i] = _PyJitAsm_NewLabel(a);
    }
    emit_frame(c);
    for (uint32_t i = 0; i < c->nwords; i += _PyIR_WordLength(c->words[i])) {
        _PyJitAsm_Bind(a, c->labels[i]);
        const _PyIRStubPlan *plan = plan_of(c, c->words[i]);
        int arm = _PyJit_ArmOf(plan, c->words[i]);
        _PyJitTemplate template = _PyJit_TemplateOf(arm);
        if (plan->region == _PyIR_PLAN_SHARED) {
            emit_stub_word(c, i, plan);
        }
        else if (template != NULL) {
            emit_template_word(c, i, arm, template);
        }
        else {
            emit_form_word(c, i);
        }
    }
    _PyJitLabel table = _PyJitAsm_NewLabel(a);
    _PyJitAsm_Section(a, _PyJitSection_COLD);
    _PyJitAsm_Align(a, 8);
    _PyJitAsm_Bind(a, table);
    for (uint32_t i = 0; i < c->nwords; i++) {
        _PyJitAsm_QuadLabel(a, c->labels[i] >= 0 ? c->labels[i] : c->trap);
    }

    /* The unwind info lies in the unit's block, at the next multiple of 8
     * after the code, where its FDE expects it. */
    uint8_t frame[256];
    size_t code_size = _PyJitAsm_Size(a);
    _PyJitAsm_Align(a, 8);
    c->eh_frame = mark(a);
    assert(_PyJitAsm_LabelOffset(a, c->eh_frame) == _Py_SIZE_ROUND_UP(code_size, 8));
    size_t frame_size = build_unwind(c, code_size, frame);
    assert(frame_size <= sizeof(frame));
    _PyJitAsm_Data(a, frame, frame_size);
    return table;
}

static void
free_unit(_PyJitUnit *unit)
{
    /* The unwinder reads the frame to forget it: deregister before freeing */
    _PyJitUnwind_Deregister(&unit->unwind);
    _PyJitMemory_Free(&unit->code);
    PyMem_RawFree(unit);
}

/* Compile the words of exec, the executing copy of code, into a new unit
 * and make it the exec's.  Returns NULL if the words hold an instrumented
 * handler or the code cannot be placed. */
static _PyJitUnit *
compile(PyCodeObject *code, _PyCodeExec *exec)
{
    const _PyIRWord *words = exec->words;
    uint32_t nwords = exec->nwords;
    for (uint32_t i = 0; i < nwords; i += _PyIR_WordLength(words[i])) {
        if (_PyIR_HandlerFlags[IR_HANDLER(words[i])] & _PyIR_H_INSTRUMENTED) {
            return NULL;
        }
    }
    _PyJitUnit *unit = PyMem_RawCalloc(1, sizeof(_PyJitUnit));
    _PyJitLabel *labels = PyMem_RawMalloc(nwords * sizeof(_PyJitLabel));
    if (unit == NULL || labels == NULL) {
        PyMem_RawFree(unit);
        PyMem_RawFree(labels);
        return NULL;
    }
    for (uint32_t i = 0; i < nwords; i++) {
        labels[i] = -1;
    }
    unit->nwords = nwords;
    unit->words = words;
    _PyJitAsm a;
    _PyJitAsm_Init(&a);
    _PyJitCompiler c = {.a = &a, .code = code, .unit = unit, .words = words, .nwords = nwords,
                        .labels = labels,
                        .capped = exec->jit_compiles + 1 >= _PyJIT_MAX_COMPILES};
    _PyJitLabel table = emit_unit(&c);
    int placed = _PyJitAsm_Place(&a, &unit->code);
    size_t table_offset = placed < 0 ? 0 : _PyJitAsm_LabelOffset(&a, table);
    size_t frame_offset = placed < 0 ? 0 : _PyJitAsm_LabelOffset(&a, c.eh_frame);
    size_t switch_offset = placed < 0 ? 0 : _PyJitAsm_LabelOffset(&a, c.switch_routine);
    _PyJitAsm_Fini(&a);
    PyMem_RawFree(labels);
    exec->jit_compiles++;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    interp->ceval.jit_compiled_codes += exec->jit_compiles == 1;
    interp->ceval.jit_compiles++;
    interp->ceval.jit_compiled_words += nwords;
    if (placed < 0) {
        /* Out of code memory: do not try this code again */
        exec->jit_compiles = _PyJIT_MAX_COMPILES;
        PyMem_RawFree(unit);
        return NULL;
    }
    unit->entries = (const void *const *)(unit->code.start + table_offset);
    unit->switch_routine = unit->code.start + switch_offset;
    unit->older = exec->jit_unit;
    exec->jit_unit = unit;
    /* The unit's name for gdb and for the perf map */
    char name[512] = "py::jit";
    PyObject *exc = PyErr_GetRaisedException();
    const char *qualname = PyUnicode_AsUTF8(code->co_qualname);
    const char *filename = PyUnicode_AsUTF8(code->co_filename);
    if (qualname != NULL && filename != NULL) {
        PyOS_snprintf(name, sizeof(name), "py::jit:%s:%s", qualname, filename);
    }
    PyErr_Clear();
    PyErr_SetRaisedException(exc);
    _PyJitUnwind_Register(&unit->unwind, &unit->code, frame_offset, name);
    if (interp->config.perf_profiling == 1) {
        PyUnstable_WritePerfMapEntry(unit->code.start,
                                     (unsigned int)frame_offset, name);
    }
    return unit;
}

_PyJitUnit *
_PyJit_Event(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyCodeExec *exec = _PyFrame_Exec(frame);
    _PyJitUnit *unit = exec->jit_unit;
    if (unit != NULL && unit->invalid == 0) {
        return unit;
    }
    if (exec == &_PyIR_InitCleanupExec ||
        tstate->interp->ceval.jit == _PyJIT_OFF) {
        /* The static exec is shared by every interpreter and never freed;
         * a code lowered with the JIT on may run in an interpreter that has
         * it off. */
        return NULL;
    }
    if (unit != NULL && unit->invalid == _PyJIT_STALE) {
        if (exec->jit_compiles >= _PyJIT_MAX_COMPILES) {
            unit->invalid = 0;
            return unit;
        }
    }
    else {
        if (++exec->jit_heat < tstate->interp->ceval.jit_warmup ||
            exec->jit_compiles >= _PyJIT_MAX_COMPILES) {
            return NULL;
        }
    }
    exec->jit_heat = 0;
    return compile(_PyFrame_GetCode(frame), exec);
}

_PyJitUnit *
_PyJit_Compile(PyCodeObject *code)
{
    _PyCodeExec *exec = _PyCode_GetExec(code);
    if (exec == NULL || exec == &_PyIR_InitCleanupExec) {
        return NULL;
    }
    return compile(code, exec);
}

#ifndef NDEBUG
/* Whether an activation of the evaluation loop on this thread is running
 * unit natively (its active-unit record): freeing it would pull the code
 * from under it, which the execution pins exist to prevent. */
static int
unit_is_active(const _PyJitUnit *unit)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return 0;
    }
    for (_PyEvalActivation *activation = ((_PyThreadStateImpl *)tstate)->eval_activation;
         activation != NULL; activation = activation->previous)
    {
        if (activation->jit_unit == unit) {
            return 1;
        }
    }
    return 0;
}
#endif

void
_PyJit_FreeUnits(_PyCodeExec *exec)
{
    _PyJitUnit *unit = exec->jit_unit;
    exec->jit_unit = NULL;
    while (unit != NULL) {
        assert(!unit_is_active(unit));
        _PyJitUnit *older = unit->older;
        free_unit(unit);
        unit = older;
    }
}

#endif  /* _PY_JIT_SUPPORTED */
