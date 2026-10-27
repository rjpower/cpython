#ifndef Py_INTERNAL_IR_H
#define Py_INTERNAL_IR_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "opcode.h"              // NB_ADD
#include "pycore_feedback.h"    // _PyFbBlock, _PyFbHeader
#include "pycore_structs.h"     // _PyIRWord

/* The register IR (boil/design/06-architecture-v2.md, section 2).
 *
 * The compiler emits the IR: a code object holds one 64-bit word per
 * instruction, and the interpreter runs a copy of them.
 *
 *      63      52 51      40 39      28 27      16 15       0
 *     +----------+----------+----------+----------+----------+
 *     |    D     |    C     |    B     |    A     |  handler |
 *     +----------+----------+----------+----------+----------+
 *
 * The handler of an instruction that has an opcode is that opcode.  The
 * handlers of the IR only (IR_JUMP, BINARY_OP_ADD, LOAD_ATTR_NOFB, ...)
 * follow from IR_FIRST_ONLY_HANDLER, after the pseudo-instructions of the
 * compiler, so that opcode.opname names each number once.  A, B, C and D are 12-bit fields,
 * and C and D together form a 24-bit field CD.  A field plays one of these
 * roles, which _PyIR_Formats gives per handler:
 *
 *     | Role | Contents                                                     |
 *     |------|--------------------------------------------------------------|
 *     | slot | an index into localsplus, 0 to IR_MAX_SLOTS - 1              |
 *     | fb   | an index into the feedback vector of the executing copy      |
 *     | imm  | the oparg: a count or a local in a 12-bit field, a constant  |
 *     |      | or name index in CD; for a jump the absolute word index of   |
 *     |      | its target in CD, and for FOR_ITER and SEND the distance in  |
 *     |      | words from the next word to the exit in D                    |
 *
 * Slots are numbered as in localsplus, so locals, cells, free variables and
 * temporaries share one numbering; the temporaries take the slots from
 * co_nlocalsplus up, which the register CFG assigns (Python/regcfg.c), and
 * the inputs of a window are consecutive.  A register-style instruction
 * names its first output (dst) and its inputs, bottom first, except that a
 * store names the object it stores into first and the value last.  A
 * window-style instruction names its dst, if it has one, and the slot of
 * its deepest input (base), and binds the rest by offset.
 *
 * The static operand of an instruction that has a feedback entry, such as
 * the name of LOAD_ATTR, lives in the arg field of the entry unless a field
 * of the word is free for it; the code object's feedback table, which
 * follows its words, holds it until the first call copies it into the
 * entry.  BINARY_OP has a handler per operation instead.  Entry 0 of every
 * feedback vector is shared by the sites past the IR_FIELD_MAX-th and never
 * specialises; those of them that need a static operand run a NOFB form of
 * their handler, which carries it in a field of the word.
 *
 * Word indices are the offsets of the IR: f_lasti, the line table and the
 * exception table count words.
 */

#define IR_FIELD_MAX    0xFFF       // the largest value of a 12-bit field
#define IR_CD_MAX       0xFFFFFF    // and of CD
/* A frame has at most this many slots, so a slot field is 0 to 4094. */
#define IR_MAX_SLOTS    IR_FIELD_MAX
/* FOR_ITER and SEND reach at most this far; the assembler relaxes a loop
 * whose exit is further away (Python/assemble.c). */
#define IR_MAX_FORWARD  2047

#define IR_HANDLER(W)   ((int)((W) & 0xFFFF))
#define IR_A(W)         ((uint32_t)((W) >> 16) & IR_FIELD_MAX)
#define IR_B(W)         ((uint32_t)((W) >> 28) & IR_FIELD_MAX)
#define IR_C(W)         ((uint32_t)((W) >> 40) & IR_FIELD_MAX)
#define IR_D(W)         ((uint32_t)((W) >> 52))
#define IR_CD(W)        ((uint32_t)((W) >> 40))

#define IR_WORD(H, A, B, C, D) \
    ((_PyIRWord)(uint16_t)(H) | (_PyIRWord)((A) & IR_FIELD_MAX) << 16 | \
     (_PyIRWord)((B) & IR_FIELD_MAX) << 28 | \
     (_PyIRWord)((C) & IR_FIELD_MAX) << 40 | \
     (_PyIRWord)((D) & IR_FIELD_MAX) << 52)

/* Feedback entries are FB_STRIDE bytes apart, the size of the largest, so
 * that an index finds its entry without a table. */
#define FB_STRIDE       sizeof(_PyFbAttr)

/* Field roles; input k of a register-style instruction is _PyIR_ROLE_IN0 + k. */
enum {
    _PyIR_ROLE_NONE,
    _PyIR_ROLE_DST,
    _PyIR_ROLE_BASE,
    _PyIR_ROLE_FB,
    _PyIR_ROLE_IMM,
    _PyIR_ROLE_IN0,
};

/* Where a format keeps its oparg and its feedback index. */
enum {
    _PyIR_LOC_NONE,
    _PyIR_LOC_A,
    _PyIR_LOC_B,
    _PyIR_LOC_C,
    _PyIR_LOC_D,
    _PyIR_LOC_CD,       // C and D, 24 bits
    _PyIR_LOC_FBARG,    // the oparg, in the arg field of the feedback entry
};

/* _PyIRFormat.flags */
#define _PyIR_VALID         1   // the handler exists
#define _PyIR_WINDOW        2   // window style
#define _PyIR_JUMPS         4   // the oparg is the absolute target in words
#define _PyIR_FORWARD       8   // the oparg is a forward distance in words
#define _PyIR_TRANSPARENT   32  // runs another handler on the same fields
#define _PyIR_STORE_DST     64  // dst may name a local: a folded STORE_FAST

#define _PyIR_NO_DST        INT8_MIN

/* The format of a handler, shared by a family, its instrumented twins and
 * the IR-only handlers named after a member.  Generated in
 * pycore_ir_metadata.h by Tools/cases_generator/ir_metadata_generator.py. */
typedef struct _PyIRFormat {
    uint8_t flags;
    uint8_t roles[4];       // of fields A, B, C and D
    uint8_t imm;            // _PyIR_LOC_*
    uint8_t fb;             // _PyIR_LOC_*
    uint8_t ninputs;        // register style
    int8_t dst;             // slot of dst less the depth before, or for
                            // window style less base; or _PyIR_NO_DST
    uint8_t borrow_mask;    // bit k: input k may name a borrowed local
    uint16_t shadow;        // the instruction that the handler stands for
    uint16_t nofb;          // the NOFB form of a generic handler that keeps
                            // its oparg in its entry, or 0
} _PyIRFormat;

#include "pycore_ir_metadata.h"

/* The executing copy of a code object: PyCodeObject._co_exec.  The words of
 * the code object (co_words, which co_code returns as bytes) never change;
 * the first call copies them into one allocation laid out as
 *
 *     | _PyCodeExec | words | _PyFbBlock | entries |
 *
 * with a feedback vector set up from the code object's feedback table.
 * Specialisation and instrumentation rewrite the handler field of the copy
 * only, so the code object's words are the generic handlers that both
 * restore.  Frames keep the words and entries of the copy in ir_words and
 * ir_feedback, which handlers index with their jump targets and feedback
 * indices.  The free-threaded build shares one copy between threads and does
 * not specialise it. */
typedef struct _PyCodeExec {
    _PyIRWord *words;
    char *entries;          // feedback entry 0, or NULL if no instruction
                            // has an entry
    uint32_t nwords;
    uint32_t nlocalsplus;   // of the code, which IR_IS_LOCAL() reads
    _PyFbBlock *feedback;   // the header before entries, or NULL
} _PyCodeExec;

/* The feedback table of a code object, which follows its words: per entry,
 * entry 0 included, its kind and the static operand of its instruction. */
#define _PyIR_FBINIT(KIND, ARG)  ((uint32_t)(KIND) << 24 | (uint32_t)(ARG))
#define _PyIR_FBINIT_KIND(V)     ((int)((V) >> 24))
#define _PyIR_FBINIT_ARG(V)      ((uint32_t)(V) & IR_CD_MAX)

/* The executing copy of _Py_InitCleanup (Python/specialize.c), which runs
 * with self in slot 0 and the result of __init__ in slot 1. */
typedef struct {
    _PyCodeExec exec;
    _PyIRWord words[3];
} _PyIRStaticExec;
extern _PyIRStaticExec _PyIR_InitCleanup;
#define _PyIR_InitCleanupExec (_PyIR_InitCleanup.exec)

/* The executing copy of co, allocated if this is its first call, or NULL
 * with an exception set. */
PyAPI_FUNC(_PyCodeExec *) _PyCode_GetExec(PyCodeObject *co);

/* Free the executing copy of a code object that is being freed. */
extern void _PyCode_ClearExec(PyCodeObject *co);

/* The bytes that co's executing copy uses, for sys.getsizeof(). */
extern size_t _PyCode_ExecSizeOf(PyCodeObject *co);

/* The farthest exit of FOR_ITER and SEND that the assembler does not relax,
 * IR_MAX_FORWARD.  Tests lower it to relax every loop compiled after. */
PyAPI_DATA(int) _PyIR_MaxForward;

/* Decoding the words of a code object (co_words), by word index.  These
 * read the code object only, never its executing copy, whose handlers
 * specialisation and instrumentation rewrite but whose fields are the
 * same. */

/* The generic handler of the instruction at index i: the head of its
 * family, the handler of a BINARY_OP operation, an inline constant form or
 * a NOFB form, as the assembler chose it. */
static inline int
_PyCode_InstrHandler(PyCodeObject *co, int i)
{
    return IR_HANDLER(((const _PyIRWord *)co->co_words)[i]);
}

/* The opcode of the stack instruction that handler stands for, without
 * specialisation or instrumentation: BINARY_OP for BINARY_OP_ADD, LOAD_ATTR
 * for LOAD_ATTR_NOFB.  IR_JUMP, which has no stack instruction, is
 * JUMP_FORWARD. */
PyAPI_FUNC(int) _PyIR_BaseOpcode(int handler);

/* The oparg of word, whose format keeps it in a field or has none. */
PyAPI_FUNC(int) _PyIR_FieldOparg(_PyIRWord word);

/* The oparg of the instruction at index i of co as the stack instruction had
 * it, except that the oparg of a jump is the word index of its target, and
 * of FOR_ITER and SEND the distance in words from the next word to their
 * exit. */
PyAPI_FUNC(int) _PyCode_InstrOparg(PyCodeObject *co, int i);

/* The feedback index of the instruction at index i, or -1 if it has none. */
PyAPI_FUNC(int) _PyCode_InstrFbIndex(PyCodeObject *co, int i);

/* The number of inputs of the instruction at index i that its fields read
 * from locals: the loads folded into it, which have no words and push
 * nothing that the frame sees before the instruction runs. */
PyAPI_FUNC(int) _PyCode_InstrFoldedInputs(PyCodeObject *co, int i);
/* The local that the dst field of word i names, into which a STORE_FAST of
 * its result is folded, or -1. */
PyAPI_FUNC(int) _PyCode_InstrStoredLocal(PyCodeObject *co, int i);

/* The word index that the instruction at index i jumps to, or -1 if it
 * does not jump.  END_ASYNC_FOR, which names its SEND, counts as a jump. */
PyAPI_FUNC(int) _PyCode_InstrTarget(PyCodeObject *co, int i);

/* The feedback entry of the instruction at instr in exec, a word of the
 * executing copy of co, or NULL. */
PyAPI_FUNC(_PyFbHeader *) _PyIR_InstrFeedback(PyCodeObject *co,
                                              const _PyCodeExec *exec,
                                              const _PyIRWord *instr);

/* The handler with the format of the word at instr, a word of frame's
 * executing copy: its handler field, or while INSTRUMENTED_LINE or
 * INSTRUMENTED_INSTRUCTION has displaced it, the code object's. */
struct _PyInterpreterFrame;
PyAPI_FUNC(int) _PyIR_RealHandler(struct _PyInterpreterFrame *frame,
                                  const _PyIRWord *instr);

/* The handler that INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION at instr
 * goes on to, given the opcode that instrumentation returned for it: the
 * code object's handler where that opcode is the uninstrumented one, the
 * NOFB form of an instrumented opcode for a NOFB word, otherwise opcode. */
PyAPI_FUNC(int) _PyIR_OriginalHandler(struct _PyInterpreterFrame *frame,
                                      const _PyIRWord *instr, int opcode);

/* The handler that runs opcode, an instrumented opcode or the base opcode
 * of the word at index i of co, on that word. */
PyAPI_FUNC(int) _PyCode_HandlerFor(PyCodeObject *co, int i, int opcode);

/* Rewrite the handler of the word at instr of an executing copy, for
 * specialisation and instrumentation.  The fields never change, but other
 * threads may be dispatching on the word. */
static inline void
_PyIR_SetHandler(_PyIRWord *instr, int handler)
{
    _PyIRWord word = (*instr & ~(_PyIRWord)0xFFFF) | (uint16_t)handler;
    *instr = word;
}

static inline const _PyIRFormat *
_PyIR_FormatOf(int handler)
{
    assert(handler >= 0 && handler < IR_NUM_HANDLERS);
    return &_PyIR_Formats[handler];
}

/* The feedback entry at index I of a vector whose entry 0 is at ENTRIES */
static inline _PyFbHeader *
_PyIR_FbEntry(char *entries, uint32_t i)
{
    assert(entries != NULL && i <= IR_FIELD_MAX);
    return (_PyFbHeader *)(entries + (size_t)i * FB_STRIDE);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IR_H */
