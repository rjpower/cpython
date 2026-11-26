#ifndef Py_INTERNAL_IR_H
#define Py_INTERNAL_IR_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "opcode.h"              // NB_ADD
#include "ir_handler_ids.h"      // IR_H_*, IR_NUM_HANDLERS

/* The handlers of the words that are not instructions: handler-only names */
#define IR_OPERANDS IR_H_IR_OPERANDS
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
 * handlers of the IR only (Jump.Trailing, BINARY_OP_ADD, ...) take the numbers
 * below 256 that no opcode holds, then continue from 256 (a
 * pseudo-instruction has no handler, so its number is free).
 * _PyIR_IsOnlyHandler tells them from the opcodes.  A, B, C and D are 12-bit fields,
 * and C and D together form a 24-bit field CD.  A field plays one of these
 * roles, which _PyIR_Formats gives per handler:
 *
 *     | Role | Contents                                                     |
 *     |------|--------------------------------------------------------------|
 *     | slot | an index into localsplus, 0 to IR_MAX_SLOTS - 1              |
 *     | fb   | an index into the feedback vector of the executing copy      |
 *     | imm  | the oparg: a count or a local in a 12-bit field, a constant  |
 *     |      | or name index in CD; for a jump the absolute word index of   |
 *     |      | its target in CD, and for FOR_ITER, SEND and the conditional |
 *     |      | register instructions the signed distance in words from the  |
 *     |      | next word to the target in D                                 |
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
 * The oparg of an instruction that has a feedback entry lives in a field of
 * the word (the name of LOAD_GLOBAL in CD, that of LOAD_ATTR, STORE_ATTR and
 * LOAD_SUPER_ATTR in D) unless no field is free: COMPARE_OP and CONTAINS_OP
 * keep their operation in the arg field of the entry, and the assembler
 * gives them entries before any other site.  The code object's feedback
 * table, which follows its words, holds it until the first call copies it
 * into the entry.  BINARY_OP has a handler per operation instead.  Entry 0
 * of every feedback vector is a header only, shared by the sites that get
 * no entry of their own; it never specialises and nothing but its header is
 * ever written.  No handler reads an operand from it, so no handler has a
 * second form for those sites; a COMPARE_OP or CONTAINS_OP that gets no
 * entry becomes CALL_INTRINSIC_2.
 *
 * Word indices are the offsets of the IR: f_lasti, the line table and the
 * exception table count words.
 */

#define IR_FIELD_MAX    0xFFF       // the largest value of a 12-bit field
#define IR_CD_MAX       0xFFFFFF    // and of CD
/* A frame has at most this many slots, so a slot field is 0 to 4094. */
#define IR_MAX_SLOTS    IR_FIELD_MAX
/* The dst field of a word whose result nothing reads (_PyIR_HasDst()), where
 * a CALL; POP_TOP pair took two words: outside the slot range.  The executing
 * copy of the code replaces it with the frame's discard slot (_PyCodeExec),
 * so no handler tests for it. */
#define IR_NO_DST       IR_FIELD_MAX

/* A private native transfer target receives an owned value instead of a word.
 * The target, rather than a runtime tag, defines the payload's meaning. */
typedef uint64_t _PyIRPayload;

/* A target in a 12-bit field is a signed distance in words from the next
 * word, -2048 to 2047.  FOR_ITER and SEND reach at most IR_MAX_FORWARD, the
 * non-negative half (2^11 - 1), and the assembler relaxes a loop whose exit
 * is further away.  */
#define IR_MAX_FORWARD  2047
#define IR_MIN_DISTANCE (-2048)

/* The 16-bit handler field holds the handler id in its low IR_HANDLER_BITS
 * bits and per-site flags in the rest (IR_FLAGS_MASK).  Dispatch indexes the
 * table with the id alone, and a handler tests its flags in ir_word, so one
 * handler serves every variant of a site that a flag distinguishes.
 * Specialisation and instrumentation rewrite the id and keep the flags. */
#define IR_HANDLER_BITS 9
#define IR_HANDLER_MASK ((1 << IR_HANDLER_BITS) - 1)
#define IR_FLAGS_MASK   (0xFFFF & ~IR_HANDLER_MASK)
static_assert(IR_NUM_HANDLERS <= IR_HANDLER_MASK + 1,
              "handler ids must fit in IR_HANDLER_BITS");

#define IR_HANDLER(W)   ((int)((W) & IR_HANDLER_MASK))
#define IR_FLAGS(W)     ((int)((W) & IR_FLAGS_MASK))

/* Slot-class flags: field X of the word names a local, a cell or a free
 * variable, a slot below co_nlocalsplus, rather than a temporary.  They
 * depend only on the fields and co_nlocalsplus, so the executing copy
 * derives them (_PyIR_ExecLocalFlags()) and the code object's words, which
 * the .pyc holds, never carry them.  A word that discards its result names
 * IR_NO_DST; the executing copy names the frame's discard slot instead, and
 * flags it like a local. */
#define IR_F_LOCAL_A    (1 << 9)
#define IR_F_LOCAL_B    (1 << 10)
#define IR_F_LOCAL_C    (1 << 11)
#define IR_F_LOCAL_D    (1 << 12)
#define IR_F_LOCAL_MASK (IR_F_LOCAL_A | IR_F_LOCAL_B | IR_F_LOCAL_C | IR_F_LOCAL_D)

/* Call-role flags of a call-family word, which the code object's words and
 * the .pyc carry.  IR_F_NO_SELF, on CALL and CALL_KW: the call has no self
 * operand (the compiler proved it NULL), so operand 0 is its first argument.
 * IR_F_NO_KW, on CALL_FUNCTION_EX: it has no kwargs operand.  A handler of
 * the family reads them from its word, so specialisation and instrumentation
 * keep them by keeping the flags (_PyIRFormat.word_flags). */
#define IR_F_NO_SELF    (1 << 13)
/* The kind of a CALL_KW or CALL_FUNCTION_EX word, which its handler id does
 * not tell: INSTRUMENTED_LINE runs the word's own handler with the id of the
 * line handler in head[0].  IR_F_KW marks CALL_KW, whose last operand is the
 * names.  IR_F_EX marks CALL_FUNCTION_EX, which has no self operand: both
 * bits 14 and 15, which no other word carries together.  On an EX word, bit
 * 13 is IR_F_NO_KW.
 *
 * IR_F_UNINTERRUPTED, on CALL: the call does not poll for signals and
 * collection when it returns.  The translator sets it on the call that
 * follows a special-method lookup, so that `with` enters its body before a
 * pending event runs (the Interrupt operand of the Call family).  It is bit
 * 15 alone. */
#define IR_F_KW         (1 << 14)
#define IR_F_UNINTERRUPTED (1 << 15)
#define IR_F_EX         (IR_F_KW | IR_F_UNINTERRUPTED)
#define IR_F_NO_KW      (1 << 13)
#define IR_IS_EX_WORD(W) (((W) & IR_F_EX) == IR_F_EX)
#define IR_IS_KW_WORD(W) (((W) & IR_F_EX) == IR_F_KW)
#define IR_F_CALL_ROLES (IR_F_NO_SELF | IR_F_KW | IR_F_EX)

/* What a native call owes, on the head of a call-family word, which the
 * executing copy derives from the fields and co_nlocalsplus
 * (_PyIR_ExecCallFlags()) and the code object's words never carry.  The call's
 * count and feedback offset, fields C and D, are no slots, so the slot-class
 * flags of those fields are free on a head.  A native call stores its result
 * in dst before it releases anything, so what it owes is a temporary other
 * than dst, whose old owner the store displaces:
 *   IR_F_CALL_OWES_CALLABLE  the callable is such a temporary;
 *   IR_F_CALL_OWES_OPERANDS  some operand that names a slot is.
 * A call that names only locals owes its operands nothing, and its release
 * pass does not read them. */
#define IR_F_CALL_OWES_CALLABLE IR_F_LOCAL_C
#define IR_F_CALL_OWES_OPERANDS IR_F_LOCAL_D
/* Operand words (pyjit/design/proposals/operand-calls80.md section 1).  Every
 * member of the CALL, CALL_KW and CALL_FUNCTION_EX families is a head word
 * followed by ceil(n / 4) words with handler IR_OPERANDS, where n is the
 * head's C field: fields A to D of operand word i name operands 4i to 4i + 3,
 * and an unused field holds IR_FIELD_MAX.  The head's A is the dst, B the
 * callable and D the feedback offset.  The operands are
 *
 *   CALL         the self slot (unless IR_F_NO_SELF), then the arguments;
 *   CALL_KW      the same, then the keyword values, then the index in
 *                co_consts of the tuple of their names (not a slot);
 *   CALL_FUNCTION_EX
 *                the positional iterable or tuple, then the kwargs mapping
 *                unless IR_F_NO_KW.
 *
 * A slot below co_nlocalsplus is a borrowed local, any other a temporary that
 * the call owns.  Those words are part of the call's instruction: a jump never
 * targets one and no handler is dispatched on one. */
#define IR_OPERANDS_PER_WORD 4

#define IR_A(W)         ((uint32_t)((W) >> 16) & IR_FIELD_MAX)
#define IR_B(W)         ((uint32_t)((W) >> 28) & IR_FIELD_MAX)
#define IR_C(W)         ((uint32_t)((W) >> 40) & IR_FIELD_MAX)
#define IR_D(W)         ((uint32_t)((W) >> 52))
#define IR_CD(W)        ((uint32_t)((W) >> 40))
#define IR_AB(W)        ((uint32_t)((W) >> 16) & IR_CD_MAX)
#define IR_BC(W)        ((uint32_t)((W) >> 28) & IR_CD_MAX)
/* D as a signed distance */
#define IR_DS(W)        ((int)(IR_D(W) ^ 0x800) - 0x800)

#define IR_WORD(H, A, B, C, D) \
    ((_PyIRWord)(uint16_t)(H) | (_PyIRWord)((A) & IR_FIELD_MAX) << 16 | \
     (_PyIRWord)((B) & IR_FIELD_MAX) << 28 | \
     (_PyIRWord)((C) & IR_FIELD_MAX) << 40 | \
     (_PyIRWord)((D) & IR_FIELD_MAX) << 52)

/* The fb field counts 8-byte units up toward entry 0, the shared header,
 * whose field is IR_FB_SHARED: the entry is at entry 0 - (IR_FB_SHARED - fb)
 * * 8, with no table.  The entries precede the words in the executing copy
 * (so a frame finds them from word 0 alone), and each kind takes its own size
 * (pycore_feedback.h); the last entry ends where entry 0 begins.  A site with
 * no entry of its own holds IR_FB_SHARED.  The vector has IR_FB_FIRST_UNIT
 * units for entry 0, and an entry begins by field 0, so it is at most 32 KB. */
#define IR_FB_UNIT_SHIFT    3
#define IR_FB_FIRST_UNIT    2
#define IR_FB_UNITS_MAX     4096
#define IR_FB_SHARED        IR_FIELD_MAX

/* Field roles; source k of a register instruction is _PyIR_ROLE_SRC + k, and
 * its home group's part 0 is the home of its GROUP field (part i is that
 * home plus i).  A translator's row (Include/internal/pycore_regcfg.h) gives
 * the base of a window role GROUP.  A field that holds the oparg has role IMM, or one of the slot roles when
 * the oparg names a slot below co_nlocalsplus that the handler reads or
 * writes itself (_PyIR_RoleIsImm()). */
enum {
    _PyIR_ROLE_NONE,
    _PyIR_ROLE_DST,
    _PyIR_ROLE_GROUP,
    _PyIR_ROLE_FB,
    _PyIR_ROLE_IMM,
    _PyIR_ROLE_LOCAL_IN,    // reads the local, which keeps its value
    _PyIR_ROLE_LOCAL_OUT,   // writes the local, releasing its old value
    _PyIR_ROLE_LOCAL_CLEAR, // leaves the local NULL, releasing its old
                            // value (DELETE_FAST)
    _PyIR_ROLE_LOCAL_MOVE,  // leaves the local NULL; its old value moves to
                            // an output (LOAD_FAST_AND_CLEAR)
    _PyIR_ROLE_CELL,        // reads the cell or free variable in the slot
    _PyIR_ROLE_COUNT,       // the length of a span operand
    _PyIR_ROLE_ELEMENT,     // an element of the word's spans
    _PyIR_ROLE_SRC,
};

static inline int
_PyIR_RoleIsImm(int role)
{
    return role >= _PyIR_ROLE_IMM && role <= _PyIR_ROLE_CELL;
}

/* Where a format keeps its oparg and its feedback index. */
enum {
    _PyIR_LOC_NONE,
    _PyIR_LOC_A,
    _PyIR_LOC_B,
    _PyIR_LOC_C,
    _PyIR_LOC_D,
    _PyIR_LOC_CD,       // C and D, 24 bits
    _PyIR_LOC_AB,       // A and B, 24 bits
    _PyIR_LOC_FBARG,    // the oparg, in the arg field of the feedback entry
    _PyIR_LOC_BC,       // B and C, 24 bits
};

/* _PyIRFormat.flags: the format, shared by a family, its instrumented twins
 * and the IR-only handlers named after a member.  What a flag says holds
 * for a word of the format whichever of them runs.  The translator's rows
 * (Include/internal/pycore_regcfg.h) have flags of their own in the bits
 * that are free here: 0x2, 0x10, 0x40, 0x800. */
#define _PyIR_VALID         0x1     // the handler exists
#define _PyIR_JUMPS         0x4     // the oparg is the absolute target in
                                    // words, in CD
#define _PyIR_FORWARD       0x8     // the oparg is a signed distance in
                                    // words from the next word, in D
#define _PyIR_TRANSPARENT   0x20    // runs another handler on the same fields
#define _PyIR_RUNS_CODE     0x80    // a member may run Python code, which may
                                    // rebind a local through frame.f_locals
#define _PyIR_JUMP_ALWAYS   0x100   // a jump with one successor, its target;
                                    // other jumps also go to the next word
#define _PyIR_NAMES_TARGET  0x200   // the oparg names a word for monitoring
                                    // only, which control never reaches
                                    // (END_ASYNC_FOR names its END_SEND)
#define _PyIR_GROUP_OUT     0x400   // the word defines its home group
                                    // and does not read it (Out<G>)

/* _PyIRFormat.effects: what the handler itself may do, a projection of its
 * arm's declaration (Python/register_ops): the effects it declares, the
 * transfers it has, or a field of its format. */
#define _PyIR_RAISES        0x1     // may raise: an error exit, or Python
                                    // that it runs (a callee, a throw()
                                    // into a yield) raises at its word
#define _PyIR_ESCAPES       0x2     // may run Python, its own or a finalizer
#define _PyIR_RUNS_PYTHON   0x4     // escapes, or fires a monitoring event;
                                    // the format's _PyIR_RUNS_CODE is the
                                    // union over its handlers
#define _PyIR_DEOPTS        0x8     // may deoptimise: to the family head, and
                                    // in BINARY_OP's family to the handler of
                                    // the operation that the entry records
#define _PyIR_SUSPENDS      0x10    // suspends its frame (a generator's)
#define _PyIR_PUSHES_FRAME  0x20    // may push a Python frame and run it
#define _PyIR_POLLS         0x40    // polls the eval breaker
#define _PyIR_PURE          0x100   // annotated pure (HAS_PURE_FLAG)
#define _PyIR_SPECIALIZES   0x200   // a generic handler whose entry's
                                    // counter drives specialisation
#define _PyIR_LEARNED_ONLY  0x1000  // requires a reached feedback nomination;
                                    // never admitted in a code image


/* Positively checked normal-edge ownership contracts. UNKNOWN deliberately
 * grants no logical initialization or ownership fact during NULL shadowing. */
enum {
    _PyIR_TRANSFER_UNKNOWN,
    _PyIR_TRANSFER_FRESH,
    _PyIR_TRANSFER_LOCAL_DUP,
    _PyIR_TRANSFER_LOCAL_MOVE,
    _PyIR_TRANSFER_LOCAL_STORE,
    _PyIR_TRANSFER_LOCAL_CLEAR,
    _PyIR_TRANSFER_RETIRE,
    _PyIR_TRANSFER_COPY,
    _PyIR_TRANSFER_SWAP,
    _PyIR_TRANSFER_PRESERVE,
    _PyIR_TRANSFER_EXIT,
};

/* _PyIRFormat.operands */
enum {
    _PyIR_OPERANDS_NONE,
    _PyIR_OPERANDS_CALL,
    _PyIR_OPERANDS_KW,
    _PyIR_OPERANDS_EX,
    _PyIR_OPERANDS_SPANS,   // span operands: counts and elements (below)
};

/* Whether a format's operand words are those of a call (calls80) */
#define _PyIR_CALL_SHAPED(OPERANDS) \
    ((OPERANDS) != _PyIR_OPERANDS_NONE && (OPERANDS) != _PyIR_OPERANDS_SPANS)

/* The instruction contract of a handler: the word format of a register
 * instruction.  Generated in pycore_ir_metadata.h from the families of
 * Python/register_ops by Tools/cases_generator/rop_generator.py, indexed by
 * handler; Python reads it through _opcode.get_ir_contract(). */
typedef struct _PyIRFormat {
    uint16_t flags;         // _PyIR_VALID, ...: the format
    uint16_t effects;       // _PyIR_RAISES, ...: the handler
    uint8_t roles[4];       // of fields A, B, C and D
    uint8_t imm;            // _PyIR_LOC_*
    uint8_t fb;             // _PyIR_LOC_*
    uint8_t target;         // _PyIR_LOC_*: where a register instruction keeps
                            // its jump target (FORWARD), or NONE: D
    uint8_t operands;       // _PyIR_OPERANDS_*: the words of the format have
                            // operand words after the head (a call)
    uint8_t fixed_words;    // the operand words after the head that a
                            // register instruction's encoding has (an arm)
    uint8_t fb_kind;        // _PyFbKind of the entry, where fb is a field
    uint8_t fb_size;        // the bytes of that entry's type, or 0
    uint8_t transfer_kind;  // checked normal-edge _PyIR_TRANSFER_* contract
    /* The homes a register instruction names (_PyIR_WordHomes): home h is
     * field h for h < 4, and part h - 3 of its home group for h = 4, 5. */
    uint8_t group;          // the parts of the GROUP operand, or 0
    uint8_t normal;         // bit e: an arm of the family takes edge e
    uint8_t vacant;         // bit h: home h may be NULL when the word reads it
    uint8_t committed;      // bit e: edge e publishes before its callbacks
    uint16_t word_flags;    // the canonical flags a word of the handler
                            // may carry: IR_F_CALL_ROLES or a subset
    uint32_t edges;         // _PyIR_HOME_* of home h after edge e, in the two
                            // bits at 2 * (_PyIR_HOMES * e + h)
    uint16_t shadow;        // the stack opcode that the handler stands for, or _PyIR_NO_OPCODE
    uint16_t twin;          // the handler that instrumentation runs on the
                            // word (itself for a twin), or _PyIR_NO_HANDLER
    uint32_t events;        // bit e: semantic handler or twin can fire event e
} _PyIRFormat;


#define _PyIR_HOMES 6
/* A home after a normal edge of the word */
enum {
    _PyIR_HOME_PRESERVE,    // untouched, or read and kept with its owner
    _PyIR_HOME_VACATE,      // its owner retired; the home is left NULL
    _PyIR_HOME_DEFINE,      // holds what the word published there
    _PyIR_HOME_UNKNOWN,     // the arms of the family leave it differently
};

/* "None" for a handler id or a stack opcode: 0 is a valid id of each */
#define _PyIR_NO_HANDLER 0xffff
#define _PyIR_NO_OPCODE 0xffff

/* The number of words of the instruction that starts with word: 1, or for
 * a call head 1 + ceil(n / 4).  Every walker over the words, of the code
 * object or of an executing copy, steps by it, and none keeps a length of
 * its own.  It reads the head's C field, which specialisation and
 * instrumentation keep. */
static inline int _PyIR_WordLength(_PyIRWord word);

#include "pycore_ir_metadata.h"

/* The signed distance in words from the next word that a word of a FORWARD
 * format holds.  The legacy formats keep it in D; a register instruction
 * keeps its target where its family's encoding puts it (fmt->target). */
static inline int
_PyIR_FormatDistance(const _PyIRFormat *fmt, _PyIRWord word)
{
    switch (fmt->target) {
        case _PyIR_LOC_C:
            return (int)(IR_C(word) ^ 0x800) - 0x800;
        case _PyIR_LOC_AB:
            return (int)(((uint32_t)(word >> 16) & 0xFFFFFF) ^ 0x800000) - 0x800000;
        default:
            return IR_DS(word);
    }
}

/* Whether the word format has a destination home of its own (a group it
 * defines is not one): a result that nothing reads goes to the frame's
 * discard slot, and a destination may name a local */
static inline int
_PyIR_HasDst(const _PyIRFormat *fmt)
{
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_DST) {
            return 1;
        }
    }
    return 0;
}

/* The field of the word format's GROUP operand, or -1 */
static inline int
_PyIR_GroupField(const _PyIRFormat *fmt)
{
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_GROUP) {
            return f;
        }
    }
    return -1;
}

/* Whether words of the generic handler opcode specialise from their entry
 * (_PyIR_SPECIALIZES), so that instrumentation must keep their counter
 * from triggering. */
static inline int
_PyFb_HasEntry(int opcode)
{
    assert(opcode >= 0 && opcode < IR_NUM_HANDLERS);
    assert(_PyIR_Formats[opcode].flags & _PyIR_VALID);
    return (_PyIR_Formats[opcode].effects & _PyIR_SPECIALIZES) != 0;
}

/* The executing copy of a code object: PyCodeObject._co_exec.  The words of
 * the code object (co_words, which co_code returns as bytes) never change;
 * the first call copies them into one allocation laid out as
 *
 *     | entries | entry 0 | _PyFbBlock | _PyCodeExec | words |
 *
 * with a feedback vector (entries and entry 0, the shared header) set up
 * from the code object's feedback table, or none if the code has no entry.
 * The vector ends at the block, so entry 0 is a fixed distance before the
 * words and every entry is at a fixed offset from word 0 (_PyIR_FbEntry()).
 * Specialisation and instrumentation rewrite the handler id of the copy
 * only, so the code object's words are the generic handlers that both
 * restore.  The copy also carries the slot-class flags (IR_F_LOCAL_A..D),
 * derived from the fields, which the code object's words do not.  Frames keep
 * word 0 of the copy in ir_words, from which handlers index their jump
 * targets and feedback entries.  The free-threaded build shares one copy
 * between threads and does not specialise it. */
/* One immutable semantic plan owns normal-return certificates. */
typedef struct {
    uint32_t nsites;
    uint32_t flags;
    uint32_t sites[];
} _PyIRCodePlan;

#define _PyIR_PLAN_ALL_RETURNS 1u
#define _PyIR_PHASE_RANGE_NULLABLE 1u

typedef enum _PyIRHomeAuthority {
    _PyIR_HOME_NONE,
    _PyIR_HOME_MUST_READABLE,
    /* Explicit development adapter, never emitted by a code-plan builder. */
    _PyIR_HOME_NULL_CONTROL_READABLE,
} _PyIRHomeAuthority;

typedef struct {
    uint16_t begin, end;
    uint8_t flags, reserved;
} _PyIRPhaseRange;

static inline size_t
_PyIR_PlanSize(const _PyIRCodePlan *plan)
{
    return sizeof(*plan) + (size_t)plan->nsites * sizeof(uint32_t);
}

enum {
    _PyIR_RETIRE_NONE,
    _PyIR_RETIRE_NORMAL_RETURN,
    _PyIR_RETIRE_ERROR_UNWIND,
    _PyIR_RETIRE_HEAP_CLEAR,
    _PyIR_RETIRE_STAGED_FAILURE,
};

enum {
    _PyIR_SHADOW_UNCOVERED,
    _PyIR_SHADOW_EXACT,
    _PyIR_SHADOW_MISMATCH,
};


/* The most constant slots that a frame copies and clears in line, in pairs
 * (_PyFrame_Initialize(), _PyFrame_ClearSlots()); a block of more is copied
 * and cleared by a call. */
#define IR_CONST_IMAGE_MAX 8

/* The register IR of a code object, lowered from its stack bytecode (co_code)
 * at the first call by _PyIR_LowerCode() and immutable after.  Its words are
 * the pristine template that the executing copy (_PyCodeExec) is made from.
 * The arrays follow the struct in one allocation.
 *
 * Slots are numbered [locals, cells, frees, constant homes, discard,
 * temporaries]: co_nlocalsplus and co_localsplusnames are the stack code's,
 * and the IR adds the constant homes and the discard slot after them.  nfixed
 * counts the slots below the temporaries, nslots all of them. */
typedef struct _PyCodeIR {
    uint32_t nwords;
    uint32_t nfixed;
    uint32_t nslots;
    uint32_t first_traceable;   // word index of the entry RESUME
    uint32_t flags;             // _PyIR_CODE_*
    uint32_t nfeedback;         // feedback entries, entry 0 included, or 0
    uint32_t nconst;            // constant slots, from const_first
    uint32_t const_first;
    uint32_t nexceptions;       // bytes of exceptions
    uint32_t nrecords;
    uint32_t nbytes;            // of the allocation
    _PyIRWord *words;
    /* The value of each constant slot: an inline scalar or an immortal
     * object, which needs no owner */
    PyValueBorrow *const_values;
    /* The code-unit offset in co_code of the stack instruction that each
     * word stands for, non-decreasing. */
    uint32_t *pcmap;
    uint32_t *fbinit;           // per feedback entry, see _PyIR_FBINIT()
    /* The exception table in word units, upstream's encoding */
    uint8_t *exceptions;
    /* nrecords encoded recovery events (_PyIRSemantic_EncodeEvent()), and the
     * position of each (_PyIRPositions_EncodeEvent()) */
    uint8_t *records;
    uint8_t *positions;
    /* The ingress recipes of exceptional edges, which an exception entry
     * and a recovery event name by index + 1 (0: none).  A recipe is a
     * count n, then n pairs of a source slot and the home of a handler's
     * parameter: the unwinder takes every source, then puts each in its
     * home, releasing what the home held. */
    int32_t *recipes;
    uint32_t nrecipes;          // int32 entries of recipes
} _PyCodeIR;

/* The compiler proved that reads of the empty **kwargs binding materialize it */
#define _PyIR_CODE_VIRTUAL_KWARGS 1

typedef struct _PyCodeExec {
    _PyIRWord *words;
    /* The constant slots (CO_FAST_CONST), which are consecutive: nconst of
     * them from const_first, and the discard slot follows.  A code with no
     * constant has const_first at the discard slot.  const_image holds the
     * PyValue of each constant, which a frame copies into its slots and does
     * not own: co_consts keeps the constants alive, and the frame holds the
     * code.  The image is padded with a NULL to an even count, because the
     * copy takes two slots at a time: an odd count writes the discard slot,
     * which is NULL at the push. */
    const PyValueBorrow *const_image;
    /* The bind plan of the last call of this code that has no keyword and
     * whose site holds no plan (_PyBindPlan_ForCall()), and the shape it was
     * made for, or 0.  A plan follows from the code and the shape alone, so
     * it lasts as long as the code. */
    _PyBindPlan call_plan;
    uint32_t nwords;
    uint16_t const_first;
    uint16_t nconst;
    uint64_t call_plan_shape;
    /* The baseline JIT (Include/internal/pycore_jit.h): the newest unit
     * compiled from these words, valid or not, which chains the older ones;
     * the warm-up events since the last compile attempt; and the compiles. */
    struct _PyJitUnit *jit_unit;
    uint32_t jit_heat;
    uint32_t jit_compiles;
    uint32_t first_traceable;   // word index of the entry RESUME
    uint32_t nfixed;        // slots below the temporaries, which IR_IS_LOCAL() reads
} _PyCodeExec;

/* The feedback table of a code object, which follows its words: per entry, a
 * word of the kind (4 bits), the field of the entry (12 bits, see
 * IR_FB_SHARED) and its arg (16 bits): the operation of COMPARE_OP,
 * CONTAINS_OP and BINARY_OP, the only operands that live in an entry.  Entry
 * 0 (field IR_FB_SHARED) comes first; the others follow in order of address,
 * so of field.  An entry ends where the next begins, and the last where
 * entry 0 does; the arg of entry 0 is the number of units of the vector,
 * entry 0 included. */
#define _PyIR_FBINIT(KIND, FIELD, ARG) \
    ((uint32_t)(KIND) << 28 | (uint32_t)(FIELD) << 16 | (uint32_t)(ARG))
#define _PyIR_FBINIT_KIND(V)     ((int)((V) >> 28))
#define _PyIR_FBINIT_FIELD(V)    ((uint32_t)((V) >> 16) & IR_FIELD_MAX)
/* The entry's distance in units before entry 0: negative, 0 for entry 0 */
#define _PyIR_FBINIT_UNIT(V)     ((int)_PyIR_FBINIT_FIELD(V) - IR_FB_SHARED)
#define _PyIR_FBINIT_ARG(V)      ((uint32_t)(V) & 0xFFFF)

/* The executing copy of _Py_InitCleanup (Python/specialize.c), which runs
 * with self in slot 0 and the result of __init__ in slot 1. */
typedef struct {
    _PyCodeExec exec;
    _PyIRWord words[2];
} _PyIRStaticExec;
extern _PyIRStaticExec _PyIR_InitCleanup;
#define _PyIR_InitCleanupExec (_PyIR_InitCleanup.exec)
extern _PyCodeIR _PyIR_InitCleanupIR;

/* The register IR of co and its executing copy, made if this is the first
 * call, or NULL with an exception set (RuntimeError for a stack code that the
 * IR cannot hold).  Everything that needs the IR of code that has not run
 * calls one of these; the accessors below assert that it exists. */
PyAPI_FUNC(_PyCodeExec *) _PyCode_GetExec(PyCodeObject *co);
PyAPI_FUNC(const _PyCodeIR *) _PyCode_GetIR(PyCodeObject *co);

/* The IR of a code object that has one.  The words never change. */
static inline const _PyCodeIR *
_PyCode_IR(PyCodeObject *co)
{
    assert(co->_co_ir != NULL);
    return co->_co_ir;
}
#define _PyCode_IRWords(CO) (_PyCode_IR(CO)->words)
#define _PyCode_IRSize(CO) ((Py_ssize_t)_PyCode_IR(CO)->nwords)
#define _PyCode_FirstTraceable(CO) ((int)_PyCode_IR(CO)->first_traceable)
/* The slots below the temporaries: locals, cells, free variables, constant
 * homes and the discard slot.  co_nlocalsplus counts the first three. */
#define _PyCode_IRFixed(CO) ((int)_PyCode_IR(CO)->nfixed)

/* The offset in bytes into co_code of the stack instruction that word index
 * `word` of the IR stands for: what f_lasti, tb_lasti, the line table and the
 * monitoring events report.  Words of one instruction map to its offset, so
 * the result never decreases.  The end of the code, or a word before the
 * first, gives the length of co_code or -1. */
static inline int
_PyCode_WordOffset(PyCodeObject *co, int word)
{
    if (word < 0) {
        return -1;
    }
    const _PyCodeIR *ir = _PyCode_IR(co);
    if (word >= (int)ir->nwords) {
        return (int)(Py_SIZE(co) * sizeof(_Py_CODEUNIT));
    }
    return (int)(ir->pcmap[word] * sizeof(_Py_CODEUNIT));
}

/* The first word index of the instruction at byte offset `offset` of
 * co_code, or of the first instruction after it if it has no word of its own
 * (a load that folded into its consumer); the number of words past the end.
 * The inverse of _PyCode_WordOffset(). */
PyAPI_FUNC(int) _PyCode_OffsetWord(PyCodeObject *co, int offset);

/* Lower the stack bytecode of co: build its IR, or return NULL with an
 * exception set. */
extern _PyCodeIR *_PyIR_LowerCode(PyCodeObject *co);
extern void _PyIR_Free(_PyCodeIR *ir);

/* Prove the admitted immutable image of co and build its plan. NULL means no
 * certified site; allocation failure returns -1 with an exception. The caller
 * owns the plan and releases it with _PyCode_FreeSemanticPlan(). The runtime
 * does not keep plans: only tests and the shadow comparison below read them. */
PyAPI_FUNC(int) _PyCode_BuildSemanticPlan(PyCodeObject *co, const _PyIRCodePlan **plan);
PyAPI_FUNC(void) _PyCode_FreeSemanticPlan(const _PyIRCodePlan *plan);

/* No allocation or callback. Query once before retirement cursor changes;
 * this development comparison never changes NULL authority. */
struct _PyInterpreterFrame;
PyAPI_FUNC(uint8_t) _PyIR_ShadowRetirement(struct _PyInterpreterFrame *frame,
                                        uint8_t reason,
                                        const _PyIRCodePlan *plan);

/* Scratch ranges are relative to nbindings. Bindings retain dynamic NULL
 * authority. Borrow ranges only while independently pinning code; a view does
 * not survive callbacks or transitions. */
typedef struct {
    uint16_t nbindings, scratch_limit;
    uint32_t nranges;
    const _PyIRPhaseRange *ranges;
} _PyIRHomeView;

/* A finite admitted protocol role, independent of whole-home map authority.
 * The caller holds a suspended-frame read claim or its actual successful
 * saved-state execution claim. Returns the receiver home or -1 without
 * allocating, preparing code or reading a home. */
PyAPI_FUNC(int) _PyIR_DelegatedReceiverHome(struct _PyInterpreterFrame *frame);

/* Free the executing copy of a code object that is being freed. */
extern void _PyCode_ClearExec(PyCodeObject *co);

/* The bytes that co's executing copy uses, for sys.getsizeof(). */
extern size_t _PyCode_ExecSizeOf(PyCodeObject *co);

/* The farthest exit of FOR_ITER and SEND that the assembler does not relax,
 * IR_MAX_FORWARD.  Tests lower it to relax every loop compiled after. */
PyAPI_DATA(int) _PyIR_MaxForward;

/* The slot limit of the register allocator, IR_MAX_SLOTS.  Tests lower it to
 * reach the "too many temporaries" SyntaxError for code compiled after. */
PyAPI_DATA(int) _PyIR_MaxSlots;

/* Decoding the words of a code object (its IR), by word index.  These
 * read the code object only, never its executing copy, whose handler ids
 * specialisation and instrumentation rewrite and whose slot-class flags
 * the copy derives, but whose fields are the same. */

/* The generic handler of the instruction at index i: the head of its
 * family, the handler of a BINARY_OP operation, or an inline constant form,
 * as the assembler chose it. */
static inline int
_PyCode_InstrHandler(PyCodeObject *co, int i)
{
    return IR_HANDLER(_PyCode_IRWords(co)[i]);
}

/* The number of words of the instruction at index i of co: 1, or a call
 * head's operand words too.  It is _PyIR_WordLength() of the code object's
 * word, which every walker over the words steps by. */
static inline int
_PyCode_InstrLength(PyCodeObject *co, int i)
{
    return _PyIR_WordLength(_PyCode_IRWords(co)[i]);
}

/* The opcode of the stack instruction that handler stands for, without
 * specialisation or instrumentation: BINARY_OP for BINARY_OP_ADD, LOAD_ATTR
 * for LOAD_ATTR.  Jump.Trailing, which has no stack instruction, is
 * JUMP_FORWARD. */
PyAPI_FUNC(int) _PyIR_BaseOpcode(int handler);

/* The stack opcode that the translator lowered into the word at index
 * `word` of co: the first stack instruction the word covers, from the
 * public offset that pcmap records.  Instrumentation and the generator
 * machinery read a word's origin here, whatever handler the word has. */
PyAPI_FUNC(int) _PyIR_OriginOpcode(PyCodeObject *co, int word);

/* The handler that the executing copy of co starts with at index i: the
 * generic handler.
 * Deoptimisation and de-instrumentation restore it. */
PyAPI_FUNC(int) _PyCode_GenericHandler(PyCodeObject *co, int i);

/* The oparg of word, whose format keeps it in a field or has none. */
PyAPI_FUNC(int) _PyIR_FieldOparg(_PyIRWord word);

/* The oparg of the instruction at index i of co as the stack instruction had
 * it, except that the oparg of a jump is the word index of its target, and
 * of FOR_ITER and SEND the distance in words from the next word to their
 * exit. */
PyAPI_FUNC(int) _PyCode_InstrOparg(PyCodeObject *co, int i);

/* The feedback index of the instruction at index i, or -1 if it has none. */
PyAPI_FUNC(int) _PyCode_InstrFbIndex(PyCodeObject *co, int i);

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

/* The handler that the word at instr runs once a LINE or INSTRUCTION wrapper
 * has passed it on; the observed one if monitoring displaced it. */
PyAPI_FUNC(int) _PyIR_DispatchedHandler(struct _PyInterpreterFrame *frame,
                                          const _PyIRWord *instr);

/* The handler of the word at index i of co while monitoring runs it: the
 * code object's handler, or when observed is true the observed twin that
 * its family declares (events(...)).  Specialisation is not kept. */
PyAPI_FUNC(int) _PyCode_HandlerFor(PyCodeObject *co, int i, int observed);

/* Whether the word at index i of co is a jump that the assembler adds after
 * a relaxed FOR_ITER, SEND or conditional branch (_PyIR_H_TRAILING). */
PyAPI_FUNC(int) _PyCode_IsTrailing(PyCodeObject *co, int i);

/* Rewrite the handler of the word at instr of an executing copy, for
 * specialisation and instrumentation.  The fields never change, but other
 * threads may be dispatching on the word. */
static inline void
_PyIR_SetHandler(_PyIRWord *instr, int handler)
{
    assert(handler >= 0 && handler <= IR_HANDLER_MASK);
    _PyIRWord word = (*instr & ~(_PyIRWord)IR_HANDLER_MASK) | (_PyIRWord)handler;
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
    // An empty row is no handler: a number of another space (a stack opcode,
    // a CFG op) was taken for an id, as a guard build (PYJIT_IR_ID_BASE) finds
    assert(_PyIR_Formats[handler].flags & _PyIR_VALID);
    return &_PyIR_Formats[handler];
}

/* Whether the flags of a canonical word, whose handler is valid, are ones
 * its handler admits: call-role flags on a call-family handler.  The
 * slot-class flags are the executing copy's. */
static inline int
_PyIR_CanonicalFlagsValid(_PyIRWord word)
{
    return (IR_FLAGS(word) & ~_PyIR_FormatOf(IR_HANDLER(word))->word_flags) == 0;
}

/* Whether the handler is a member of the CALL, CALL_KW or CALL_FUNCTION_EX
 * family or one of their instrumented twins: its words have operand words. */
static inline int
_PyIR_HandlerHasOperands(int handler)
{
    return _PyIR_CALL_SHAPED(_PyIR_FormatOf(handler)->operands);
}

/* The words of the call whose head is word: the head and its operand words.
 * The head's handler is not consulted, so it is right for a head that an
 * INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION word has replaced in the
 * executing copy (which keeps every other field). */
static inline int
_PyIR_CallWordLength(_PyIRWord word)
{
    return 1 + (int)((IR_C(word) + IR_OPERANDS_PER_WORD - 1)
                     / IR_OPERANDS_PER_WORD);
}

/* The span operands of a register instruction (rop_check.pack): each has a
 * count field (role COUNT), and their elements, the spans in declaration
 * order, fill the head's fields of role ELEMENT, then operand words of four
 * fields each after the fixed operand words.  Element j of the word whose
 * head is head: */
static inline int
_PyIR_SpanElement(const _PyIRWord *head, int handler, int j)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    int nhead = 0;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_ELEMENT) {
            if (nhead++ == j) {
                return (int)((head[0] >> (16 + 12 * f)) & IR_FIELD_MAX);
            }
        }
    }
    j -= nhead;
    return (int)((head[1 + fmt->fixed_words + j / IR_OPERANDS_PER_WORD]
                  >> (16 + 12 * (j % IR_OPERANDS_PER_WORD))) & IR_FIELD_MAX);
}

/* The words of a span instruction: the head, its fixed operand words, and
 * the words of the elements that the head's free fields do not hold */
static inline int
_PyIR_SpanWordLength(int handler, _PyIRWord word)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    int total = 0, nhead = 0;
    for (int f = 0; f < 4; f++) {
        int value = (int)((word >> (16 + 12 * f)) & IR_FIELD_MAX);
        total += fmt->roles[f] == _PyIR_ROLE_COUNT ? value : 0;
        nhead += fmt->roles[f] == _PyIR_ROLE_ELEMENT;
    }
    int rest = total > nhead ? total - nhead : 0;
    return 1 + fmt->fixed_words
           + (rest + IR_OPERANDS_PER_WORD - 1) / IR_OPERANDS_PER_WORD;
}

/* The spans of a span format: the number of COUNT fields, which are the
 * spans in declaration order, and the direction of each.  A Move has two,
 * its destinations and its sources.  A family with one span reads it when it
 * also has a destination of its own (Build's items) or no input of its own
 * (Raise's operands), and writes it otherwise (Unpack's destinations). */
static inline int
_PyIR_SpanCountFields(const _PyIRFormat *fmt)
{
    int n = 0;
    for (int f = 0; f < 4; f++) {
        n += fmt->roles[f] == _PyIR_ROLE_COUNT;
    }
    return n;
}

/* Whether the one span of a single-span format is read */
static inline int
_PyIR_SingleSpanReads(const _PyIRFormat *fmt)
{
    int has_dst = 0, has_input = 0;
    for (int f = 0; f < 4; f++) {
        has_dst |= fmt->roles[f] == _PyIR_ROLE_DST;
        has_input |= fmt->roles[f] >= _PyIR_ROLE_SRC;
    }
    return has_dst || !has_input;
}

/* A span format whose one span is destinations (Unpack's): it has no DST
 * role of its own and the span is not counted twice as Move's are */
static inline int
_PyIR_IsDstSpanFormat(const _PyIRFormat *fmt)
{
    if (fmt->operands != _PyIR_OPERANDS_SPANS || _PyIR_SpanCountFields(fmt) != 1) {
        return 0;
    }
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_DST) {
            return 0;
        }
    }
    return 1;
}

static inline int
_PyIR_IsMoveFormat(const _PyIRFormat *fmt)
{
    return fmt->operands == _PyIR_OPERANDS_SPANS && _PyIR_SpanCountFields(fmt) == 2;
}

/* Span k of the word at head, which runs as handler: its element count and
 * the index of its first element, and whether the word reads its elements */
static inline int
_PyIR_SpanBounds(const _PyIRWord *head, int handler, int k, int *first,
                 int *is_source)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    int seen = 0, before = 0, count = 0;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_COUNT) {
            int c = (int)((head[0] >> (16 + 12 * f)) & IR_FIELD_MAX);
            if (seen++ < k) {
                before += c;
            }
            else if (seen == k + 1) {
                count = c;
            }
        }
    }
    *first = before;
    *is_source = seen == 2 ? k == 1 : _PyIR_SingleSpanReads(fmt);
    return count;
}

static inline int
_PyIR_WordLength(_PyIRWord word)
{
    int handler = IR_HANDLER(word);
    if (_PyIR_FormatOf(handler)->operands == _PyIR_OPERANDS_SPANS) {
        return _PyIR_SpanWordLength(handler, word);
    }
    if (!_PyIR_HandlerHasOperands(handler)) {
        // A register instruction arm may have fixed operand words
        return 1 + _PyIR_FormatOf(handler)->fixed_words;
    }
    return _PyIR_CallWordLength(word);
}

/* Operand k of the call whose head is at head: the slot that it names, or for
 * the names of a CALL_KW the index in co_consts. */
static inline Py_ALWAYS_INLINE int
_PyIR_CallOperand(const _PyIRWord *head, int k)
{
    return (int)((head[1 + k / IR_OPERANDS_PER_WORD]
                  >> (16 + 12 * (k % IR_OPERANDS_PER_WORD))) & IR_FIELD_MAX);
}

/* The calls' operand layouts, from the base opcode of the head: whether the
 * first operand is the self slot, and how many trailing operands of the
 * head's C field are not slots (the names of a CALL_KW). */
static inline int
_PyIR_CallHasSelf(_PyIRWord word)
{
    return !(word & IR_F_NO_SELF) && !IR_IS_EX_WORD(word);
}

static inline int
_PyIR_CallSlotOperands(_PyIRWord word)
{
    return (int)IR_C(word) - IR_IS_KW_WORD(word);
}

/* The slot-class flags of word in an executing copy of code with
 * nlocalsplus locals. */
static inline _PyIRWord
_PyIR_ExecLocalFlags(_PyIRWord word, uint32_t nlocalsplus)
{
    return (IR_A(word) < nlocalsplus ? IR_F_LOCAL_A : 0)
         | (IR_B(word) < nlocalsplus ? IR_F_LOCAL_B : 0)
         | (IR_C(word) < nlocalsplus ? IR_F_LOCAL_C : 0)
         | (IR_D(word) < nlocalsplus ? IR_F_LOCAL_D : 0);
}

/* The slot-class flags of C and D of the call whose head is at head in an
 * executing copy of code with nlocalsplus locals: what the call owes
 * (IR_F_CALL_OWES_CALLABLE and IR_F_CALL_OWES_OPERANDS).  The operand words
 * have their own fields, not yet carrying slot-class flags, and a CALL_KW's
 * names are not slots. */
static inline _PyIRWord
_PyIR_ExecCallFlags(const _PyIRWord *head, uint32_t nlocalsplus)
{
    uint32_t dst = IR_A(*head);
    uint32_t callable = IR_B(*head);
    _PyIRWord flags = callable >= nlocalsplus && callable != dst
                      ? IR_F_CALL_OWES_CALLABLE : 0;
    int slots = _PyIR_CallSlotOperands(*head);
    for (int k = 0; k < slots; k++) {
        uint32_t home = (uint32_t)_PyIR_CallOperand(head, k);
        if (home >= nlocalsplus && home != dst) {
            return flags | IR_F_CALL_OWES_OPERANDS;
        }
    }
    return flags;
}

/* The feedback block of the executing copy whose word 0 is WORDS, which
 * is the one with entries (the code's nfeedback is not 0) */
static inline _PyFbBlock *
_PyIR_FbBlock(const _PyIRWord *words)
{
    return (_PyFbBlock *)((const _PyCodeExec *)words - 1) - 1;
}

/* The feedback entry of field F, in the executing copy whose word 0 is
 * WORDS.  Entry 0 is the header that ends the vector before the block. */
static inline _PyFbHeader *
_PyIR_FbEntry(const _PyIRWord *words, uint32_t f)
{
    assert(f <= IR_FB_SHARED);
    /* Written from field 0, entry 0 less the shared field's units, so that
     * the address is word 0 and the field scaled plus a constant */
    char *field0 = (char *)_PyIR_FbBlock(words) - sizeof(_PyFbHeader)
                   - ((size_t)IR_FB_SHARED << IR_FB_UNIT_SHIFT);
    return (_PyFbHeader *)(field0 + ((size_t)f << IR_FB_UNIT_SHIFT));
}

/* The index in the feedback table fbinit of n words of the entry whose field
 * is FIELD, or -1.  The table is in order of field after entry 0, which has
 * no entry of its own to find. */
static inline int
_PyIR_FbFind(const uint32_t *fbinit, int n, uint32_t field)
{
    int low = 1, high = n - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        uint32_t u = _PyIR_FBINIT_FIELD(fbinit[mid]);
        if (u == field) {
            return mid;
        }
        if (u < field) {
            low = mid + 1;
        }
        else {
            high = mid - 1;
        }
    }
    return -1;
}

/* The index in co_fbinit of the entry that starts at unit, or -1 */
PyAPI_FUNC(int) _PyCode_FbOrdinal(PyCodeObject *co, uint32_t unit);

/* The number of units of co's feedback vector, 0 without entries */
PyAPI_FUNC(uint32_t) _PyCode_FbUnits(PyCodeObject *co);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IR_H */
