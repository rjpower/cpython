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
 * handlers of the IR only (IR_JUMP, BINARY_OP_ADD, ...)
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
 *     |      | its target in CD, and for FOR_ITER, SEND and the fused       |
 *     |      | compare-and-jumps the signed distance in words from the next |
 *     |      | word to the target in D                                      |
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
/* The dst field of a call whose result nothing reads: outside the slot range,
 * it tells the handler to release the result (_PyIR_DISCARDS), where a
 * CALL; POP_TOP pair took two words. */
#define IR_NO_DST       IR_FIELD_MAX
/* A target in a 12-bit field is a signed distance in words from the next
 * word, -2048 to 2047.  FOR_ITER and SEND reach at most IR_MAX_FORWARD, the
 * non-negative half (2^11 - 1), and the assembler relaxes a loop whose exit
 * is further away.  The fused compare-and-jumps go forward only; the
 * assembler leaves the pair unfused where the target is further
 * (Python/assemble.c). */
#define IR_MAX_FORWARD  2047
#define IR_MIN_DISTANCE (-2048)

#define IR_HANDLER(W)   ((int)((W) & 0xFFFF))
#define IR_A(W)         ((uint32_t)((W) >> 16) & IR_FIELD_MAX)
#define IR_B(W)         ((uint32_t)((W) >> 28) & IR_FIELD_MAX)
#define IR_C(W)         ((uint32_t)((W) >> 40) & IR_FIELD_MAX)
#define IR_D(W)         ((uint32_t)((W) >> 52))
#define IR_CD(W)        ((uint32_t)((W) >> 40))
/* D as a signed distance */
#define IR_DS(W)        ((int)(IR_D(W) ^ 0x800) - 0x800)

#define IR_WORD(H, A, B, C, D) \
    ((_PyIRWord)(uint16_t)(H) | (_PyIRWord)((A) & IR_FIELD_MAX) << 16 | \
     (_PyIRWord)((B) & IR_FIELD_MAX) << 28 | \
     (_PyIRWord)((C) & IR_FIELD_MAX) << 40 | \
     (_PyIRWord)((D) & IR_FIELD_MAX) << 52)

/* The fb field is an offset in 8-byte units into the feedback vector, and
 * each kind of entry takes its own size (pycore_feedback.h): the entry is at
 * entries + (fb << 3), with no table.  Entry 0 is the 16-byte header, so
 * entries start at unit IR_FB_FIRST_UNIT, and an entry ends by unit
 * IR_FB_UNITS_MAX, a vector of at most 32 KB. */
#define IR_FB_UNIT_SHIFT    3
#define IR_FB_FIRST_UNIT    2
#define IR_FB_UNITS_MAX     4096

/* Field roles; input k of a register-style instruction is _PyIR_ROLE_IN0 + k.
 * A field that holds the oparg has role IMM, or one of the slot roles when
 * the oparg names a slot below co_nlocalsplus that the handler reads or
 * writes itself (_PyIR_RoleIsImm()). */
enum {
    _PyIR_ROLE_NONE,
    _PyIR_ROLE_DST,
    _PyIR_ROLE_BASE,
    _PyIR_ROLE_FB,
    _PyIR_ROLE_IMM,
    _PyIR_ROLE_LOCAL_IN,    // reads the local, which keeps its value
    _PyIR_ROLE_LOCAL_OUT,   // writes the local, releasing its old value
    _PyIR_ROLE_LOCAL_CLEAR, // leaves the local NULL, releasing its old
                            // value (DELETE_FAST)
    _PyIR_ROLE_LOCAL_MOVE,  // leaves the local NULL; its old value moves to
                            // an output (LOAD_FAST_AND_CLEAR)
    _PyIR_ROLE_CELL,        // reads the cell or free variable in the slot
    _PyIR_ROLE_IN0,
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
    _PyIR_LOC_FBARG,    // the oparg, in the arg field of the feedback entry
};

/* _PyIRFormat.flags: the format, shared by a family, its instrumented twins
 * and the IR-only handlers named after a member.  What a flag says holds
 * for a word of the format whichever of them runs. */
#define _PyIR_VALID         0x1     // the handler exists
#define _PyIR_WINDOW        0x2     // window style
#define _PyIR_JUMPS         0x4     // the oparg is the absolute target in
                                    // words, in CD
#define _PyIR_FORWARD       0x8     // the oparg is a signed distance in
                                    // words from the next word, in D
#define _PyIR_NO_FOLD       0x10    // an output may alias a borrowed input
#define _PyIR_TRANSPARENT   0x20    // runs another handler on the same fields
#define _PyIR_STORE_DST     0x40    // dst may name a local: a folded
                                    // STORE_FAST, which releases the local's
                                    // old value (_PyIR_MayRunPython())
#define _PyIR_RUNS_CODE     0x80    // a member may run Python code, which may
                                    // rebind a local through frame.f_locals
#define _PyIR_JUMP_ALWAYS   0x100   // a jump with one successor, its target;
                                    // other jumps also go to the next word
#define _PyIR_NAMES_TARGET  0x200   // the oparg names a word for monitoring
                                    // only, which control never reaches
                                    // (END_ASYNC_FOR names its END_SEND)
#define _PyIR_FUSED         0x400   // a compare, containment or identity test
                                    // fused with the conditional jump after
                                    // it: FORWARD, with the operation in the
                                    // arg of the entry (bits 0 to 7) and the
                                    // sense of the jump (_PyIR_FUSED_IF_FALSE)

/* The arg of the entry of a fused word: the operation of the compare (its
 * oparg, or the invert bit of CONTAINS_OP and IS_OP) and the sense of the
 * jump.  The first call seeds the taken history of its entry. */
#define IR_FUSED_OP_MASK    0xFF
#define _PyIR_FUSED_IF_FALSE 0x100  // jump where the condition is false
#define _PyIR_DISCARDS      0x800   // dst may be IR_NO_DST: the handler
                                    // releases the result (the calls)
#define _PyIR_UNPACKS       0x1000  // dst names the first of a run of locals
                                    // that receive the elements of an
                                    // unpack in order, or is IR_NO_DST: the
                                    // elements stay in the window
#define _PyIR_ABRUPT_PREFIX 0x2000  // checked complete suspension footprint;
                                    // exceptional absence only, not readability

/* _PyIRFormat.effects: what the handler itself may do, from the cases
 * generator's analysis of its instruction.  For a stack opcode they agree
 * with the HAS_*_FLAG flags of _PyOpcode_opcode_metadata. */
#define _PyIR_RAISES        0x1     // may raise (HAS_ERROR_FLAG)
#define _PyIR_ESCAPES       0x2     // makes an escaping call (HAS_ESCAPES_FLAG)
#define _PyIR_RUNS_PYTHON   0x4     // may run Python code; the format's
                                    // _PyIR_RUNS_CODE is the union over its
                                    // handlers
#define _PyIR_DEOPTS        0x8     // may deoptimise: to the family head, and
                                    // in BINARY_OP's family to the handler of
                                    // the operation that the entry records
#define _PyIR_SUSPENDS      0x10    // suspends its frame (a generator's)
#define _PyIR_PUSHES_FRAME  0x20    // may push a Python frame and run it
#define _PyIR_POLLS         0x40    // polls the eval breaker
#define _PyIR_ALLOCATES     0x80    // may allocate: conservatively, it may
                                    // raise, escape, box, push a frame or
                                    // call a constructor
#define _PyIR_PURE          0x100   // annotated pure (HAS_PURE_FLAG)
#define _PyIR_SPECIALIZES   0x200   // a generic handler whose entry's
                                    // counter drives specialisation
#define _PyIR_BOXES         0x400   // may box a value into the value record
                                    // (_PyValueTemps), which it then needs

#define _PyIR_NO_DST        INT8_MIN

/* A count of slots as a function of the oparg: k plus the part of the oparg
 * that form names.  Window extents and pass-through inputs, as the stack
 * effect of the handler's instruction has them. */
enum {
    _PyIR_EXT_FIXED,        // k
    _PyIR_EXT_OPARG,        // k + oparg
    _PyIR_EXT_OPARG2,       // k + 2 * oparg
    _PyIR_EXT_BIT0,         // k + (oparg & 1)
    _PyIR_EXT_BYTES,        // k + (oparg & 0xFF) + (oparg >> 8): UNPACK_EX
};

typedef struct {
    int8_t k;
    uint8_t form;           // _PyIR_EXT_*
} _PyIRExtent;

static inline int
_PyIR_Extent(_PyIRExtent e, int oparg)
{
    switch (e.form) {
        case _PyIR_EXT_OPARG:
            return e.k + oparg;
        case _PyIR_EXT_OPARG2:
            return e.k + 2 * oparg;
        case _PyIR_EXT_BIT0:
            return e.k + (oparg & 1);
        case _PyIR_EXT_BYTES:
            return e.k + (oparg & 0xFF) + (oparg >> 8);
        default:
            return e.k;
    }
}

/* Success-path fact for the first output of a STORE_DST handler. A temporary
 * home owns its canonical result before receiver retirement. A folded local
 * delays this handler's overwrite until the final store; callbacks may change
 * that home. This says nothing about earlier callbacks, exceptional exits or
 * additional outputs. */
enum {
    _PyIR_RESULT_PHASE_NONE,
    _PyIR_RESULT_PHASE_DST_TEMP_CANONICAL_BEFORE_RETIRE,
};

/* The instruction contract of a handler: what a word that it runs reads,
 * writes and may do (pyjit/design/register-ir.md section 5).  Generated in
 * pycore_ir_metadata.h by Tools/cases_generator/ir_metadata_generator.py,
 * indexed by handler; Python reads it through _opcode.get_ir_contract().
 * Heap effects, guards and result type facts are not in it yet. */
enum {
    _PyIR_OUTPUT_REGISTER_DST,
    _PyIR_OUTPUT_WINDOW,
    _PyIR_OUTPUT_WINDOW_DST_FIRST,
    _PyIR_OUTPUT_OUTCOME,
};

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

enum {
    _PyIR_OWNER_LEAF,
    _PyIR_OWNER_READ,
    _PyIR_OWNER_TAKE,
    _PyIR_OWNER_FRESH,
    _PyIR_OWNER_RESTORE,
    _PyIR_OWNER_PHASE,
    _PyIR_OWNER_ERROR_BRANCH,
    _PyIR_OWNER_NORMAL_EXIT,
    _PyIR_OWNER_ERROR_EXIT,
    _PyIR_OWNER_CREATED_COPY,
    _PyIR_OWNER_NORMAL_CLEAR_INPUTS,
    _PyIR_OWNER_CALL_DRAIN,
    _PyIR_OWNER_NORMAL_COMPLETION,
    _PyIR_OWNER_UNPACK_LOCAL_DRAIN,
};

#define _PyIR_COMPLETION_HAS_RESULT 1u
#define _PyIR_NORMAL_WINDOW_RESULT 1u
#define _PyIR_NORMAL_DISJOINT_CHILD 2u

typedef struct {
    uint8_t kind, flags;
    uint16_t dst, src, extent;
} _PyIROwnershipEvent;
static_assert(sizeof(_PyIROwnershipEvent) == 8,
              "ownership event encoding must remain fixed");

/* A checked observer edge whose terminal provider has committed its physical
 * outputs before the callback. Generated from the provider, not inferred from
 * its event name. An absent fact cannot authorize POST inspection. */
enum {
    _PyIR_OBSERVER_NONE,
    _PyIR_OBSERVER_FORMAT_VALUE,
    _PyIR_OBSERVER_FORMAT_NULL_FIRST,
    _PyIR_OBSERVER_FOR_BODY,
    _PyIR_OBSERVER_COMPLETION,
};

typedef struct _PyIRFormat {
    uint16_t flags;         // _PyIR_VALID, ...: the format
    uint16_t effects;       // _PyIR_RAISES, ...: the handler
    uint8_t roles[4];       // of fields A, B, C and D
    uint8_t imm;            // _PyIR_LOC_*
    uint8_t fb;             // _PyIR_LOC_*
    uint8_t ninputs;        // register style
    int8_t dst;             // slot of dst less the depth before, or for
                            // window style less base; or _PyIR_NO_DST
    uint8_t borrow_mask;    // bit k: input k may name a borrowed local
    uint8_t fb_kind;        // _PyFbKind of the entry, where fb is a field
    uint8_t fb_size;        // the bytes of that entry's type, or 0
    uint8_t result_phase;   // _PyIR_RESULT_PHASE_*: conditional success fact
    uint8_t observer_outcome[2]; // checked fallthrough/taken POST facts
    uint8_t output_kind;    // actual physical publication, not fold capability:
                            // REGISTER_DST: outputs at dst; WINDOW: at base;
                            // WINDOW_DST_FIRST: base outputs, with only the
                            // result at base+dst moved to the explicit dst;
                            // OUTCOME: use the semantic per-edge contract.
                            // UNPACKS additionally transfers to its local run.
    uint8_t transfer_kind;  // checked normal-edge _PyIR_TRANSFER_* contract
    uint32_t owner_program;
    uint16_t owner_events;
    uint8_t call_input_holes; // canonical virtual-NULL window inputs
    _PyIRExtent reads;      // the stack inputs, from the deepest
    _PyIRExtent writes;     // the stack outputs, from the same slot
    _PyIRExtent through;    // the deepest inputs that the handler leaves
                            // in their slots with their values: it never
                            // assigns, reassigns or takes the address of
                            // them (passing an array on counts).  The
                            // count stops at the first input that is not
                            // such, so an unchanged input above a changed
                            // one (the middle of SWAP) is not counted.
                            // The other inputs are consumed, and a
                            // temporary that names them is cleared
    uint16_t shadow;        // the instruction that the handler stands for
    uint16_t twin;          // the handler that instrumentation runs on the
                            // word (itself for a twin), or 0
    uint32_t events;        // bit e: semantic handler or twin can fire event e
} _PyIRFormat;

#define _PyIR_CALL_HOLE_SELF (1u << 1)
#define _PyIR_CALL_HOLE_KWARGS (1u << 3)

typedef enum {
    _PyIR_CALL_INPUT_VALUE,
    _PyIR_CALL_INPUT_VIRTUAL_NULL,
} _PyIRCallInputRole;

static inline _PyIRCallInputRole
_PyIR_CallInputRole(const _PyIRFormat *fmt, int offset)
{
    return (offset == 1 || offset == 3) &&
        (fmt->call_input_holes & (1u << offset))
        ? _PyIR_CALL_INPUT_VIRTUAL_NULL : _PyIR_CALL_INPUT_VALUE;
}

#include "pycore_ir_metadata.h"

/* The role of the field of format fmt that holds the oparg: IMM or a slot
 * role; NONE if no single field holds it. */
static inline int
_PyIR_OpargRole(const _PyIRFormat *fmt)
{
    if (fmt->imm >= _PyIR_LOC_A && fmt->imm <= _PyIR_LOC_D) {
        return fmt->roles[fmt->imm - _PyIR_LOC_A];
    }
    return _PyIR_ROLE_NONE;
}

/* Whether a word of format fmt may run Python code, which may rebind any
 * local through frame.f_locals: its format may, or it releases the old
 * value of a local, whose __del__ may.  It does so through a dst that
 * names a local (_PyIR_STORE_DST, where dst_local says that dst does), and
 * through an oparg whose role is _PyIR_ROLE_LOCAL_OUT or
 * _PyIR_ROLE_LOCAL_CLEAR. */
static inline int
_PyIR_MayRunPython(const _PyIRFormat *fmt, int dst_local)
{
    int role = _PyIR_OpargRole(fmt);
    return (fmt->flags & _PyIR_RUNS_CODE)
           || (dst_local && (fmt->flags & _PyIR_STORE_DST))
           || role == _PyIR_ROLE_LOCAL_OUT
           || role == _PyIR_ROLE_LOCAL_CLEAR;
}

/* Whether words of the generic handler opcode specialise from their entry
 * (_PyIR_SPECIALIZES), so that instrumentation must keep their counter
 * from triggering. */
static inline int
_PyFb_HasEntry(int opcode)
{
    return opcode >= 0 && opcode < IR_NUM_HANDLERS
           && (_PyIR_Formats[opcode].effects & _PyIR_SPECIALIZES);
}

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
/* One immutable semantic plan owns normal-return certificates and reached
 * callback shapes. Sparse sites precede sorted (site, phase) rows and interned
 * scratch ranges. Bindings retain dynamic NULL authority in every shape. */
typedef struct {
    uint32_t nsites;
    uint32_t nphases;
    uint32_t nranges;
    uint32_t flags;
    uint32_t sites[];
} _PyIRCodePlan;

#define _PyIR_PLAN_ALL_RETURNS 1u
#define _PyIR_PLAN_ALL_CREATED_EMPTY 2u
#define _PyIR_PHASE_RANGE_NULLABLE 1u

typedef enum _PyIRHomeAuthority {
    _PyIR_HOME_NONE,
    _PyIR_HOME_SHADOW_CANDIDATE,
    _PyIR_HOME_MUST_READABLE,
    /* Explicit development adapter, never emitted by a code-plan builder. */
    _PyIR_HOME_NULL_CONTROL_READABLE,
} _PyIRHomeAuthority;

typedef struct {
    uint32_t site, first, count;
    uint8_t phase, authority;
    uint8_t reserved[2];
} _PyIRPhaseSite;

typedef struct {
    uint16_t begin, end;
    uint8_t flags, reserved;
} _PyIRPhaseRange;

static inline const _PyIRPhaseSite *
_PyIR_PlanPhases(const _PyIRCodePlan *plan)
{
    return (const _PyIRPhaseSite *)(plan->sites + plan->nsites);
}

static inline const _PyIRPhaseRange *
_PyIR_PlanRanges(const _PyIRCodePlan *plan)
{
    return (const _PyIRPhaseRange *)(_PyIR_PlanPhases(plan) + plan->nphases);
}

static inline size_t
_PyIR_PlanSize(const _PyIRCodePlan *plan)
{
    return sizeof(*plan) + (size_t)plan->nsites * sizeof(uint32_t) +
        (size_t)plan->nphases * sizeof(_PyIRPhaseSite) +
        (size_t)plan->nranges * sizeof(_PyIRPhaseRange);
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

/* Phase IDs belong to a canonical family; coverage is separate from PRE=0. */
enum {
    _PyIR_PHASE_PRE,
    _PyIR_PHASE_POST,
    _PyIR_PHASE_ERROR,
    _PyIR_PHASE_RETIRING,
    _PyIR_PHASE_SUSPENDED,
    _PyIR_PHASE_BINARY_PROVIDER,
    _PyIR_PHASE_BINARY_RETIRE,
    _PyIR_PHASE_BINARY_ERROR_RESTORE,
};
#define _PyIR_PHASE_PUBLISHED (1u << 8)
#define _PyIR_PHASE_STATUS_SHIFT 9
#define _PyIR_PHASE_STATUS_MASK (3u << _PyIR_PHASE_STATUS_SHIFT)
#define _PyIR_PHASE_CAPTURED (1u << 11)
#define _PyIR_PHASE_CREATED (1u << 12)
#define _PyIR_PHASE_YIELD_DELEGATED (1u << 13)

typedef struct _PyCodeExec {
    _PyIRWord *words;
    char *entries;          // feedback entry 0, or NULL if no instruction
                            // has an entry
    const _PyIRCodePlan *semantic_plan;
    uint32_t nwords;
    uint32_t nlocalsplus;   // of the code, which IR_IS_LOCAL() reads
    _PyFbBlock *feedback;   // the header before entries, or NULL
} _PyCodeExec;

/* The feedback table of a code object, which follows its words: per entry,
 * entry 0 included, in order of unit, a word of the kind (4 bits), the unit
 * where the entry starts (12 bits) and its arg (16 bits): the operation of
 * COMPARE_OP, CONTAINS_OP and BINARY_OP, the only operands that live in an
 * entry.  An entry ends where the next begins; the arg of entry 0 is the
 * number of units of the vector. */
#define _PyIR_FBINIT(KIND, UNIT, ARG) \
    ((uint32_t)(KIND) << 28 | (uint32_t)(UNIT) << 16 | (uint32_t)(ARG))
#define _PyIR_FBINIT_KIND(V)     ((int)((V) >> 28))
#define _PyIR_FBINIT_UNIT(V)     ((uint32_t)((V) >> 16) & IR_FIELD_MAX)
#define _PyIR_FBINIT_ARG(V)      ((uint32_t)(V) & 0xFFFF)

/* The executing copy of _Py_InitCleanup (Python/specialize.c), which runs
 * with self in slot 0 and the result of __init__ in slot 1. */
typedef struct {
    _PyCodeExec exec;
    _PyIRWord words[2];
} _PyIRStaticExec;
extern _PyIRStaticExec _PyIR_InitCleanup;
#define _PyIR_InitCleanupExec (_PyIR_InitCleanup.exec)

/* The executing copy of co, allocated if this is its first call, or NULL
 * with an exception set. */
PyAPI_FUNC(_PyCodeExec *) _PyCode_GetExec(PyCodeObject *co);

/* Build from the admitted immutable image. NULL means no certified site;
 * allocation failure returns -1 with an exception. */
int _PyCode_BuildSemanticPlan(PyCodeObject *co, const _PyIRCodePlan **plan);

/* No allocation or callback. Query once before retirement cursor changes;
 * this development comparison never changes NULL authority. */
struct _PyInterpreterFrame;
PyAPI_FUNC(uint8_t) _PyIR_ShadowRetirement(struct _PyInterpreterFrame *frame,
                                        uint8_t reason);

/* Read-only NULL-control comparison, called only in a reached frame walk.
 * The published bit alone does not grant an arbitrary reader home access. */
PyAPI_FUNC(uint8_t) _PyIR_ShadowFramePhase(struct _PyInterpreterFrame *frame);

/* NULL-shadow candidate scratch ranges are relative to nbindings. A nullable
 * joined candidate may be canonical NULL on another path; it grants no stale
 * cell readability without join normalization/validity at the authority switch.
 * Bindings retain dynamic NULL authority. Borrow ranges only while independently
 * pinning code; this active view does not survive callbacks or transitions. */
typedef struct {
    uint16_t nbindings, scratch_limit;
    uint32_t nranges;
    const _PyIRPhaseRange *ranges;
} _PyIRHomeView;

/* Returns NONE before reading homes if the active publication is not covered.
 * Candidate authority permits the NULL-shadow comparison only. MUST_READABLE
 * proves every selected canonical home is initialized on every predecessor;
 * unconverted control/index roles cannot acquire that authority from owner bits.
 * A successful lookup selects a shape; it does not compare runtime values. */
PyAPI_FUNC(_PyIRHomeAuthority) _PyIR_GetFrameHomeView(
    struct _PyInterpreterFrame *frame, _PyIRHomeView *out);

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

/* Decoding the words of a code object (co_words), by word index.  These
 * read the code object only, never its executing copy, whose handlers
 * specialisation and instrumentation rewrite but whose fields are the
 * same. */

/* The generic handler of the instruction at index i: the head of its
 * family, the handler of a BINARY_OP operation, or an inline constant form,
 * as the assembler chose it. */
static inline int
_PyCode_InstrHandler(PyCodeObject *co, int i)
{
    return IR_HANDLER(((const _PyIRWord *)co->co_words)[i]);
}

/* The opcode of the stack instruction that handler stands for, without
 * specialisation or instrumentation: BINARY_OP for BINARY_OP_ADD, LOAD_ATTR
 * for LOAD_ATTR.  IR_JUMP, which has no stack instruction, is
 * JUMP_FORWARD. */
PyAPI_FUNC(int) _PyIR_BaseOpcode(int handler);

/* Cold specialization/instrumentation preserves the image's virtual inputs. */
PyAPI_FUNC(int) _PyIR_CallRoleHandler(int handler, int opcode);

/* The opcode that sys.monitoring sees the word as: the base opcode, except
 * for a fused compare-and-jump, which is a conditional jump
 * (POP_JUMP_IF_FALSE, or INSTRUMENTED_ for its twin). */
PyAPI_FUNC(int) _PyIR_EventOpcode(int handler);

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

/* The number of inputs of the instruction at index i that its fields read
 * from locals: the loads folded into it, which have no words and push
 * nothing that the frame sees before the instruction runs. */
PyAPI_FUNC(int) _PyCode_InstrFoldedInputs(PyCodeObject *co, int i);
/* The local that the dst field of word i names, into which a STORE_FAST of
 * its result is folded, or -1. */
PyAPI_FUNC(int) _PyCode_InstrStoredLocal(PyCodeObject *co, int i);

/* The number of temporaries that the frame holds before the word at index i
 * runs, as its fields prove it, -1 if they name no temporary, or
 * _PyCode_DEPTH_UNKNOWN if they name some but do not fix the depth.  The
 * temporaries are a prefix of the slots, so a word that names slot
 * co_nlocalsplus + d as its highest input, or the end of its window, or
 * the slot it pushes to, has d entries beneath.  A PUSH_NULL has no word:
 * its entry is a NULL slot that only the first word naming a higher slot
 * shows (Objects/frameobject.c:mark_stacks).  *floor is the depth once the
 * word has consumed its inputs. */
#define _PyCode_DEPTH_UNKNOWN (-2)
PyAPI_FUNC(int) _PyCode_InstrDepthBefore(PyCodeObject *co, int i, int *floor);

/* The depths of the entries that the loads folded into the word at index i
 * would have pushed, or none if the word names no temporary.  Returns their
 * number. */
PyAPI_FUNC(int) _PyCode_InstrHoles(PyCodeObject *co, int i, int holes[4]);

/* Whether the call at index i releases its result (its dst is IR_NO_DST):
 * the POP_TOP of the stack form is folded into it. */
PyAPI_FUNC(int) _PyCode_InstrDiscards(PyCodeObject *co, int i);

/* The number of elements that the unpack at index i stores to locals (its
 * dst names a run of them), where its stack instruction pushed them; 0 if
 * its elements stay in the window or it is no unpack. */
PyAPI_FUNC(int) _PyCode_InstrUnpackedLocals(PyCodeObject *co, int i);

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
 * code object's handler where that opcode is the uninstrumented one,
 * otherwise opcode. */
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

/* The feedback entry at unit I of a vector whose entry 0 is at ENTRIES */
static inline _PyFbHeader *
_PyIR_FbEntry(char *entries, uint32_t i)
{
    assert(entries != NULL && i <= IR_FIELD_MAX);
    return (_PyFbHeader *)(entries + ((size_t)i << IR_FB_UNIT_SHIFT));
}

/* The index in the feedback table fbinit of n words of the entry that starts
 * at unit, or -1.  The table is in order of unit. */
static inline int
_PyIR_FbFind(const uint32_t *fbinit, int n, uint32_t unit)
{
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        uint32_t u = _PyIR_FBINIT_UNIT(fbinit[mid]);
        if (u == unit) {
            return mid;
        }
        if (u < unit) {
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
