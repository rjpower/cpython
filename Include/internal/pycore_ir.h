#ifndef Py_INTERNAL_IR_H
#define Py_INTERNAL_IR_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_feedback.h"    // _PyFbBlock, _PyFbHeader
#include "pycore_structs.h"     // _PyIRWord

/* The register IR (boil/design/01-internal-ir.md).
 *
 * Built with --with-ir-interp, the interpreter runs a translation of each
 * code object's bytecode instead of the bytecode itself.  An instruction is
 * one 64-bit word or two:
 *
 *     | Word | Bits  | Field   | Contents                                      |
 *     |------|-------|---------|-----------------------------------------------|
 *     | 0    | 0-15  | handler | index into the dispatch tables                |
 *     | 0    | 16-31 | A       | first field                                   |
 *     | 0    | 32-47 | B       | second field                                  |
 *     | 0    | 48-63 | C       | third field                                   |
 *     | 1    | 0-31  | D       | the oparg, two-word formats only              |
 *     | 1    | 32-63 | F       | the feedback distance, two-word formats only  |
 *
 * The handler of an instruction that has an opcode is that opcode.  The
 * handlers of the IR only (IR_LOWER, IR_EXT, ...) follow from
 * IR_FIRST_ONLY_HANDLER, 256.
 *
 * A field plays one of these roles, which _PyIR_Formats gives per handler:
 *
 *     | Role | Contents                                                     |
 *     |------|--------------------------------------------------------------|
 *     | slot | (index into localsplus) << 1, with IR_BORROW in bit 0        |
 *     | fb   | words from the instruction to its feedback entry             |
 *     | imm  | the oparg: a constant or name index, a count, a local; for   |
 *     |      | a jump the distance in words, which the opcode signs as the  |
 *     |      | oparg of the bytecode does                                   |
 *
 * Slots are numbered as in localsplus, so locals, cells, free variables and
 * temporaries share one numbering; the temporary at depth d is slot
 * co_nlocalsplus + d.  A register-style instruction names its first output
 * (dst) and its inputs, bottom first; a window-style instruction names the
 * slot of its deepest input (base) and binds the rest by offset.  IR_BORROW
 * on an input means that the slot is a local, which the instruction reads
 * as a borrowed reference: it sets Py_TAG_REFCNT on the value once it knows
 * the value to be a pointer.
 *
 * Formats with more fields than fit move the oparg into the feedback
 * entry's arg field, or take word 1.  IR_EXT in front of an instruction
 * holds the high 16 bits of A, B and C in its own A, B and C.  Jumps and
 * exception handlers land on the first word of an instruction, IR_EXT
 * included; instrumentation rewrites the handler of the word after it.
 *
 * Code objects start with the block _PyIR_LowerStub, whose handler IR_LOWER
 * lowers the bytecode (Python/ir_lower.c) at first execution.
 * The lowered block keeps, per word, the bytecode offsets of the first code
 * unit the instruction consumed (start) and of the instruction that does
 * the work (anchor), from which f_lasti, line numbers and exception
 * handlers follow.
 */

#define IR_FIRST_ONLY_HANDLER 256

#define IR_HANDLER(W)   ((int)((W) & 0xFFFF))
#define IR_A(W)         ((uint32_t)(((W) >> 16) & 0xFFFF))
#define IR_B(W)         ((uint32_t)(((W) >> 32) & 0xFFFF))
#define IR_C(W)         ((uint32_t)((W) >> 48))
#define IR_D(W)         ((uint32_t)(W))
#define IR_F(W)         ((uint32_t)((W) >> 32))

#define IR_WORD(H, A, B, C) \
    ((_PyIRWord)(uint16_t)(H) | (_PyIRWord)(uint16_t)(A) << 16 | \
     (_PyIRWord)(uint16_t)(B) << 32 | (_PyIRWord)(uint16_t)(C) << 48)
#define IR_WORD1(D, F) \
    ((_PyIRWord)(uint32_t)(D) | (_PyIRWord)(uint32_t)(F) << 32)

/* A slot field */
#define IR_BORROW           1
#define IR_SLOT_FIELD(I)    ((uint32_t)(I) << 1)
#define IR_SLOT_INDEX(F)    ((F) >> 1)

/* Field roles; input k of a register-style instruction is _PyIR_ROLE_IN0 + k. */
enum {
    _PyIR_ROLE_NONE,
    _PyIR_ROLE_DST,
    _PyIR_ROLE_BASE,
    _PyIR_ROLE_FB,
    _PyIR_ROLE_IMM,
    _PyIR_ROLE_IN0,
};

/* Where a format keeps its oparg and its feedback distance. */
enum {
    _PyIR_LOC_NONE,
    _PyIR_LOC_A,
    _PyIR_LOC_B,
    _PyIR_LOC_C,
    _PyIR_LOC_D,
    _PyIR_LOC_F,
    _PyIR_LOC_FBARG,    // the oparg, in the arg field of the feedback entry
};

/* _PyIRFormat.flags */
#define _PyIR_WINDOW        1   // window style
#define _PyIR_JUMPS         2   // the oparg is a jump distance in words
#define _PyIR_NO_FOLD       4   // an output may alias a borrowed input
#define _PyIR_TRANSPARENT   8   // runs another handler on the same fields

#define _PyIR_NO_DST        INT8_MIN

/* The format of a handler, shared by a family, its instrumented twins and
 * the IR-only handlers named after a member.  Generated in
 * pycore_ir_metadata.h by Tools/cases_generator/ir_metadata_generator.py. */
typedef struct {
    uint8_t nwords;
    uint8_t flags;
    uint8_t roles[3];       // of fields A, B and C
    uint8_t imm;            // _PyIR_LOC_*
    uint8_t fb;             // _PyIR_LOC_*
    uint8_t ninputs;        // register style
    int8_t dst;             // slot of dst less the depth before, or _PyIR_NO_DST
    uint8_t borrow_mask;    // bit k: input k may carry IR_BORROW
    uint16_t shadow;        // the opcode that the handler runs for in co_code
} _PyIRFormat;

#include "pycore_ir_metadata.h"

/* Per word, the bytecode offsets of the first code unit consumed and of the
 * instruction that does the work.  _PyIR_PCMAP_WIDE selects 32-bit entries,
 * for code objects of more than 65535 code units. */
typedef struct {
    uint16_t start;
    uint16_t anchor;
} _PyIRPc16;

typedef struct {
    uint32_t start;
    uint32_t anchor;
} _PyIRPc32;

#define _PyIR_PCMAP_WIDE    1

/* The IR of a code object: PyCodeObject._co_ir.  A lowered block is one
 * allocation, laid out as
 *
 *     | _PyIRBlock | words | _PyFbBlock | entries | pcmap |
 *
 * so that a feedback distance is a word count.  The free-threaded build
 * shares one block between threads and does not specialise it. */
typedef struct _PyIRBlock {
    _PyIRWord *words;
    uint32_t nwords;
    uint32_t flags;         // _PyIR_PCMAP_WIDE
    void *pcmap;            // _PyIRPc16 or _PyIRPc32, one per word
    _PyFbBlock *feedback;   // NULL if no instruction has an entry
    uint16_t *real;         // once the code is instrumented, per main word
                            // the handler that the lowering chose, which
                            // instrumentation displaces and restores
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
 * exception and return NULL.  Malformed bytecode raises SystemError. */
PyAPI_FUNC(_PyIRBlock *) _PyIR_Lower(PyCodeObject *co);

/* Lower the code of frame, whose instr_ptr is in _PyIR_LowerStub, and move
 * frame->instr_ptr to the lowered block. */
struct _PyInterpreterFrame;
extern int _PyIR_LowerFrame(PyThreadState *tstate, struct _PyInterpreterFrame *frame);

/* Free the block of a code object that is being freed. */
extern void _PyIR_Clear(PyCodeObject *co);

/* The bytes that co's block uses, for sys.getsizeof(). */
extern size_t _PyIR_SizeOf(PyCodeObject *co);

/* Offsets map words to bytecode and back; they are bytecode offsets in code
 * units, as f_lasti.  instr may be any word of an instruction. */
PyAPI_FUNC(int) _PyIR_Anchor(const _PyIRBlock *block, const _PyIRWord *instr);
PyAPI_FUNC(int) _PyIR_Start(const _PyIRBlock *block, const _PyIRWord *instr);

/* The first word of the instruction that covers bytecode offset offset,
 * which jumps and exception handlers land on, and its main word, which
 * holds the handler and follows IR_EXT if there is one. */
PyAPI_FUNC(_PyIRWord *) _PyIR_InstrAt(const _PyIRBlock *block, int offset);
PyAPI_FUNC(_PyIRWord *) _PyIR_MainWordAt(const _PyIRBlock *block, int offset);

/* The oparg and the feedback entry (or NULL) of the instruction whose main
 * word is instr, decoded as handler would decode them.  A transparent
 * handler decodes as the handler it runs. */
PyAPI_FUNC(int) _PyIR_InstrOparg(const _PyIRBlock *block,
                                 const _PyIRWord *instr, int handler);
PyAPI_FUNC(_PyFbHeader *) _PyIR_InstrFeedback(const _PyIRBlock *block,
                                              const _PyIRWord *instr, int handler);

/* The handler with the format of the word at instr: its handler field, or
 * while INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION has displaced it, the
 * handler it displaced. */
PyAPI_FUNC(int) _PyIR_RealHandler(struct _PyInterpreterFrame *frame,
                                  const _PyIRWord *instr);

/* The handler that INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION at instr
 * goes on to, given the opcode that instrumentation returned for it: the
 * lowering's choice where that opcode is the uninstrumented one. */
PyAPI_FUNC(int) _PyIR_OriginalHandler(struct _PyInterpreterFrame *frame,
                                      const _PyIRWord *instr, int opcode);

/* Instrumentation (Python/instrumentation.c) keeps rewriting
 * co_code_adaptive, which the IR shadows.  _PyIR_PrepareInstrumentation()
 * runs before the first rewrite of a lowered block and may fail;
 * _PyIR_SyncInstrumentation() then copies the opcode at offset, an
 * instruction's anchor, to the handler of its main word. */
extern int _PyIR_PrepareInstrumentation(PyCodeObject *co);
extern void _PyIR_SyncInstrumentation(PyCodeObject *co, int offset);

/* The anchor of the instruction whose words cover bytecode offset offset:
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

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IR_H */
