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
 * The interpreter runs a translation of each code object's bytecode instead
 * of the bytecode itself.  An instruction is one 64-bit word:
 *
 *      63      52 51      40 39      28 27      16 15       0
 *     +----------+----------+----------+----------+----------+
 *     |    D     |    C     |    B     |    A     |  handler |
 *     +----------+----------+----------+----------+----------+
 *
 * The handler of an instruction that has an opcode is that opcode.  The
 * handlers of the IR only (IR_LOWER, BINARY_OP_ADD, ...) follow from
 * IR_FIRST_ONLY_HANDLER, 256.  A, B, C and D are 12-bit fields, and C and D
 * together form a 24-bit field CD.  A field plays one of these roles, which
 * _PyIR_Formats gives per handler:
 *
 *     | Role | Contents                                                     |
 *     |------|--------------------------------------------------------------|
 *     | slot | an index into localsplus, 0 to IR_MAX_SLOTS - 1              |
 *     | fb   | an index into the feedback vector of the block               |
 *     | imm  | the oparg: a count or a local in a 12-bit field, a constant  |
 *     |      | or name index in CD; for a jump the absolute word index of   |
 *     |      | its target in CD, and for FOR_ITER and SEND the distance in  |
 *     |      | words from the next word to the exit in D                    |
 *
 * Slots are numbered as in localsplus, so locals, cells, free variables and
 * temporaries share one numbering; the temporary at depth d is slot
 * co_nlocalsplus + d.  A register-style instruction names its first output
 * (dst) and its inputs, bottom first, except that a store names the object
 * it stores into first and the value last.  A window-style instruction
 * names its dst, if it has one, and the slot of its deepest input (base),
 * and binds the rest by offset.
 *
 * An instruction whose consumer absorbed a LOAD_FAST_BORROW reads the local
 * through its input field.  Such an input is borrowed exactly when its
 * field differs from the input's canonical slot, which follows from dst.
 *
 * The static operand of an instruction that has a feedback entry, such as
 * the name of LOAD_ATTR, lives in the arg field of the entry unless a field
 * of the word is free for it.  BINARY_OP has a handler per operation instead.
 * Entry 0 of every feedback vector is shared by the sites past the
 * IR_FIELD_MAX-th; it never specialises, and those sites find their static
 * operand in the bytecode.
 *
 * Code objects start with the block _PyIR_LowerStub, whose handler IR_LOWER
 * lowers the bytecode (Python/ir_lower.c) at first execution.
 * The lowered block keeps, per word, the bytecode offsets of the first code
 * unit the instruction consumed (start) and of the instruction that does
 * the work (anchor), from which f_lasti, line numbers and exception
 * handlers follow.
 */

#define IR_FIRST_ONLY_HANDLER 256

#define IR_FIELD_MAX    0xFFF       // the largest value of a 12-bit field
#define IR_CD_MAX       0xFFFFFF    // and of CD
/* A frame has at most this many slots, so a slot field is 0 to 4094. */
#define IR_MAX_SLOTS    IR_FIELD_MAX
/* FOR_ITER and SEND reach at most this far; the lowering relaxes a loop
 * whose exit is further away (Python/ir_lower.c). */
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
#define _PyIR_NO_FOLD       16  // an output may alias a borrowed input
#define _PyIR_TRANSPARENT   32  // runs another handler on the same fields

#define _PyIR_NO_DST        INT8_MIN

/* The format of a handler, shared by a family, its instrumented twins and
 * the IR-only handlers named after a member.  Generated in
 * pycore_ir_metadata.h by Tools/cases_generator/ir_metadata_generator.py. */
typedef struct {
    uint8_t flags;
    uint8_t roles[4];       // of fields A, B, C and D
    uint8_t imm;            // _PyIR_LOC_*
    uint8_t fb;             // _PyIR_LOC_*
    uint8_t ninputs;        // register style
    int8_t dst;             // slot of dst less the depth before, or for
                            // window style less base; or _PyIR_NO_DST
    uint8_t borrow_mask;    // bit k: input k may name a borrowed local
    uint16_t shadow;        // the opcode that the handler runs for in co_code
} _PyIRFormat;

#include "pycore_ir_metadata.h"

/* Per word, the bytecode offsets of the first code unit consumed and of the
 * instruction that does the work.  _PyIR_PCMAP_WIDE selects 32-bit entries,
 * for code objects of more than 65535 code units.  The words that a relaxed
 * FOR_ITER adds after its own share its entry. */
typedef struct {
    uint16_t start;
    uint16_t anchor;
} _PyIRPc16;

typedef struct {
    uint32_t start;
    uint32_t anchor;
} _PyIRPc32;

#define _PyIR_PCMAP_WIDE    1
/* Some instruction has no word of its own: a load absorbed into its
 * consumer.  INSTRUCTION events need the code lowered again, one word per
 * instruction. */
#define _PyIR_FUSED         2

/* The IR of a code object: PyCodeObject._co_ir.  A lowered block is one
 * allocation, laid out as
 *
 *     | _PyIRBlock | words | _PyFbBlock | entries | pcmap |
 *
 * Frames keep words and entries in ir_words and ir_feedback, which handlers
 * index with their jump targets and feedback indices.  The free-threaded
 * build shares one block between threads and does not specialise it. */
typedef struct _PyIRBlock {
    _PyIRWord *words;
    char *entries;          // feedback entry 0, or NULL if no instruction
                            // has an entry
    uint32_t nwords;
    uint32_t flags;         // _PyIR_PCMAP_WIDE, _PyIR_FUSED
    void *pcmap;            // _PyIRPc16 or _PyIRPc32, one per word
    _PyFbBlock *feedback;   // the header before entries, or NULL
    uint16_t *real;         // once the code is instrumented, per word
                            // the handler that the lowering chose, which
                            // instrumentation displaces and restores
    struct _PyIRBlock *fused;   // the block that this one replaced, in
                                // which frames may still be; its words
                                // run IR_REMAP
} _PyIRBlock;

/* The block of code objects not lowered yet.  Its two words run IR_LOWER:
 * a frame at word 0 starts at bytecode offset 0, and a frame at word 1
 * (_PyIR_STUB_RESUMED) counts as past the first RESUME, which frames that
 * never run need for f_lasti. */
PyAPI_DATA(_PyIRBlock) _PyIR_LowerStub;
#define _PyIR_STUB_RESUMED (_PyIR_LowerStub.words + 1)

/* The static block of _Py_InitCleanup (Python/specialize.c). */
extern _PyIRBlock _PyIR_InitCleanupBlock;

/* Lower co if it is not lowered yet and return its block, or set an
 * exception and return NULL.  Malformed bytecode, and code that needs more
 * than IR_MAX_SLOTS slots, raise SystemError. */
PyAPI_FUNC(_PyIRBlock *) _PyIR_Lower(PyCodeObject *co);

/* The farthest exit of FOR_ITER and SEND that the lowering does not relax,
 * IR_MAX_FORWARD.  Tests lower it to relax every loop. */
PyAPI_DATA(int) _PyIR_MaxForward;

/* Lower the code of frame, whose instr_ptr is in _PyIR_LowerStub, and move
 * the frame to the lowered block. */
struct _PyInterpreterFrame;
extern int _PyIR_LowerFrame(PyThreadState *tstate, struct _PyInterpreterFrame *frame);

/* Free the block of a code object that is being freed. */
extern void _PyIR_Clear(PyCodeObject *co);

/* The bytes that co's block uses, for sys.getsizeof(). */
extern size_t _PyIR_SizeOf(PyCodeObject *co);

/* Offsets map words to bytecode and back; they are bytecode offsets in code
 * units, as f_lasti. */
PyAPI_FUNC(int) _PyIR_Anchor(const _PyIRBlock *block, const _PyIRWord *instr);
PyAPI_FUNC(int) _PyIR_Start(const _PyIRBlock *block, const _PyIRWord *instr);

/* The word of the instruction that covers bytecode offset offset, which
 * jumps and exception handlers land on. */
PyAPI_FUNC(_PyIRWord *) _PyIR_InstrAt(const _PyIRBlock *block, int offset);

/* Whether the word at instr is one that a relaxed FOR_ITER added after its
 * own, which belongs to no bytecode instruction. */
PyAPI_FUNC(int) _PyIR_IsTrampoline(const _PyIRBlock *block,
                                   const _PyIRWord *instr);

/* The oparg and the feedback entry (or NULL) of the instruction at instr
 * in block, a block of co, decoded as handler would decode them.  A
 * transparent handler decodes as the handler it runs.  The oparg of a jump
 * is the absolute word index of its target. */
PyAPI_FUNC(int) _PyIR_InstrOparg(PyCodeObject *co, const _PyIRBlock *block,
                                 const _PyIRWord *instr, int handler);
PyAPI_FUNC(_PyFbHeader *) _PyIR_InstrFeedback(const _PyIRBlock *block,
                                              const _PyIRWord *instr, int handler);

/* The oparg in co_code of the instruction that the word at instr of
 * frame's block runs, EXTENDED_ARG included.  An instruction that shares
 * feedback entry 0 finds its static operand here. */
PyAPI_FUNC(int) _PyIR_BytecodeOparg(struct _PyInterpreterFrame *frame,
                                    const _PyIRWord *instr);

/* The handler with the format of the word at instr: its handler field, or
 * while INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION has displaced it, the
 * handler it displaced. */
PyAPI_FUNC(int) _PyIR_RealHandler(struct _PyInterpreterFrame *frame,
                                  const _PyIRWord *instr);

/* The unspecialised handler of the instruction at instr in block, which
 * runs handler: the head of its family, or for BINARY_OP the handler of
 * its operation. */
PyAPI_FUNC(int) _PyIR_GenericHandler(const _PyIRBlock *block,
                                     const _PyIRWord *instr, int handler);

/* The handler that INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION at instr
 * goes on to, given the opcode that instrumentation returned for it: the
 * lowering's choice where that opcode is the uninstrumented one. */
PyAPI_FUNC(int) _PyIR_OriginalHandler(struct _PyInterpreterFrame *frame,
                                      const _PyIRWord *instr, int opcode);

/* Instrumentation (Python/instrumentation.c) keeps rewriting
 * co_code_adaptive, which the IR shadows.  _PyIR_PrepareInstrumentation()
 * runs before each update of the instrumentation of a lowered block and may
 * fail.  If instructions are on, it lowers a fused block again, one word per
 * instruction.  _PyIR_SyncInstrumentation() then copies the opcode at
 * offset, an instruction's anchor, to the handler of its word. */
extern int _PyIR_PrepareInstrumentation(PyCodeObject *co, int instructions);

/* The word in the block of co where a frame at instr, a word of the fused
 * block that co ran before, goes on: the word of the instruction that
 * starts at the same offset. */
PyAPI_FUNC(_PyIRWord *) _PyIR_Remap(PyCodeObject *co, const _PyIRWord *instr);
extern void _PyIR_SyncInstrumentation(PyCodeObject *co, int offset);

/* The anchor of the instruction whose word covers bytecode offset offset:
 * offset itself, unless offset is an EXTENDED_ARG prefix or a load that the
 * lowering absorbed into its consumer.  Before lowering, offset. */
extern int _PyIR_WordAnchor(PyCodeObject *co, int offset);

/* Write into instructions, a copy of co_code_adaptive, the handlers and
 * feedback of the lowered block in the layout of the stack bytecode, for
 * code._co_code_adaptive and dis.  write_field writes the fields of an
 * entry into the inline cache of the instruction it belongs to. */
extern int _PyIR_WriteAdaptive(
    PyCodeObject *co, _Py_CODEUNIT *instructions,
    void (*write_fields)(_Py_CODEUNIT *instr, const _PyFbHeader *h));

/* Rewrite the handler of the word at instr, for specialisation and
 * instrumentation.  The fields never change once a block is lowered, but
 * other threads may be dispatching on the word. */
static inline void
_PyIR_SetHandler(_PyIRWord *instr, int handler)
{
    _PyIRWord word = (*instr & ~(_PyIRWord)0xFFFF) | (uint16_t)handler;
#ifdef Py_GIL_DISABLED
    _Py_atomic_store_uint64_relaxed(instr, word);
#else
    *instr = word;
#endif
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
