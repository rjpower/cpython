#ifndef Py_INTERNAL_REGCFG_H
#define Py_INTERNAL_REGCFG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"          // IR_MAX_SLOTS
#include "pycore_opcode_utils.h" // IS_NO_FALLTHROUGH_OPCODE
#include "pycore_rop_metadata.h" // ROP_OP_CallKw_Generic
#include "pycore_symtable.h"    // _Py_SourceLocation

/* The translator's row of a stack op (_PyStack_Formats, generated from
 * Tools/cases_generator/stack_ops.py into pycore_regcfg_metadata.h) and of
 * a register instruction as the translator places it (_PyRegCfg_ArmFormat).
 * Window ops read and rewrite consecutive slots from a base (role GROUP);
 * the others read their inputs from SRC fields and write dst. */
typedef struct {
    int8_t k;
    uint8_t form;           // _PyStack_EXT_*
} _PyStackExtent;

typedef struct {
    uint16_t flags;         // _PyIR_VALID, _PyIR_FORWARD, ..., _PyStack_*
    uint16_t effects;       // _PyIR_RAISES, ...
    uint8_t roles[4];       // of fields A, B, C and D
    uint8_t imm;            // _PyIR_LOC_*: where the oparg lives
    uint8_t ninputs;        // the inputs that SRC fields name
    int8_t dst;             // slot of dst less the depth before, or for a
                            // window less its base; or _PyStack_NO_DST
    uint8_t borrow_mask;    // bit k: input k may name a borrowed local
    uint8_t operands;       // _PyIR_OPERANDS_*
    _PyStackExtent through; // the deepest inputs that the op leaves in
                            // their slots with their values
} _PyStackFormat;

/* _PyStackFormat.flags of the translator's own, in bits that _PyIRFormat's
 * flags leave free (pycore_ir.h) */
#define _PyStack_WINDOW     0x2     // window style
#define _PyStack_NO_FOLD    0x10    // an output may alias a borrowed input
#define _PyStack_STORE_DST  0x40    // dst may name a local: a folded
                                    // STORE_FAST, which releases the local's
                                    // old value (_PyStack_MayRunPython())
#define _PyStack_DISCARDS   0x800   // dst may be IR_NO_DST: the handler
                                    // releases the result (the calls)

/* _PyStackFormat.dst of a row without a result */
#define _PyStack_NO_DST     INT8_MIN

/* A count of slots as a function of the oparg: k plus the part of the oparg
 * that form names */
enum {
    _PyStack_EXT_FIXED,     // k
    _PyStack_EXT_OPARG,     // k + oparg
    _PyStack_EXT_OPARG2,    // k + 2 * oparg
    _PyStack_EXT_BIT0,      // k + (oparg & 1)
    _PyStack_EXT_BYTES,     // k + (oparg & 0xFF) + (oparg >> 8): UNPACK_EX
};

static inline int
_PyStack_Extent(_PyStackExtent e, int oparg)
{
    switch (e.form) {
        case _PyStack_EXT_OPARG:
            return e.k + oparg;
        case _PyStack_EXT_OPARG2:
            return e.k + 2 * oparg;
        case _PyStack_EXT_BIT0:
            return e.k + (oparg & 1);
        case _PyStack_EXT_BYTES:
            return e.k + (oparg & 0xFF) + (oparg >> 8);
        default:
            return e.k;
    }
}

/* The register CFG (boil/design/07-compiler-passes.md).
 *
 * The flowgraph optimiser (Python/flowgraph.c) hands its basic blocks of
 * stack instructions to Python/regcfg.c, which converts each instruction
 * into a register op over virtual registers, runs the passes of
 * Python/regcfg_passes.c over the blocks, and gives every virtual register
 * a frame slot.  The assembler (Python/assemble.c) encodes the ops.
 *
 * Virtual registers.  Register v < nlocalsplus is the local, cell or free
 * variable of slot v.  Register _PyReg_CANON(cfg, k) is the canonical
 * temporary of stack position k, whose slot is nlocalsplus + k: at the
 * entry of a block that a jump or more than one edge reaches, and at an
 * exception handler, the value at stack position k is in it, so that the
 * predecessors agree without phi nodes.  Registers from _PyReg_FRESH0(cfg)
 * up are the temporaries that the conversion allocates for every push;
 * each is defined once and lives within one region (below).
 *
 * Regions.  A block that only its predecessor in the layout reaches, by
 * falling through, continues that block's names: the conversion does not
 * canonicalise between them.  A chain of such blocks is a region.  The
 * stack is canonical at every jump (for its target), at the end of a
 * region, and below the depth of the exception handler of every op that
 * has one, where the handler finds it.
 *
 * Ops.  An op keeps the stack instruction it stands for, so that the
 * metadata of the opcode and its IR format still apply.  in[] are the
 * registers it reads, in stack order, bottom first: the top npop of them
 * are popped, the rest stay; then nimplicit registers that the op does not
 * read but that are live until it, the values that a raise leaves for the
 * unwinder.  out[] are the registers it pushes, bottom first.  For a
 * register-style op an output below the position of dst passes its input
 * through and is the same register.  A window-style op reads its inputs
 * from consecutive slots from its base and writes output j to base + j;
 * compaction keeps the slots consecutive, and the verifier checks them.
 */

/* The opcode of a deleted op */
#define _PyReg_DEAD             (-1)

/* _PyRegOp.flags */
#define _PyReg_SOURCE_ORIGIN   256 // explicit source span, not propagated location
#define _PyReg_COMPLETED       512 // frontend completion performed by its producer
#define _PyReg_CALL_NO_SELF   1024 // the reserved self input is virtual NULL
#define _PyReg_CALL_NO_KWARGS 2048 // EX's reserved kwargs input is virtual NULL
#define _PyReg_CALL_UNINTERRUPTED 65536 // the call does not poll when it returns
#define _PyReg_LOAD_FOLDED      1   // a load that its consumer reads in place
#define _PyReg_STORE_FOLDED     2   // a store that its producer does
#define _PyReg_JUMP_OUTPUT      4   // outputs only where it jumps
#define _PyReg_DISCARD          64  // a call whose result nothing reads: its
                                    // dst field is IR_NO_DST
#define _PyReg_ELIDED           16  // a PUSH_NULL that has no word: the NULL
                                    // invariant makes its slot NULL already
#define _PyReg_ELIDED_NOP       32  // ... but a NOP stands for it, because it
                                    // alone holds its line

                                    // of locals from dst; out[] stay as the
                                    // window's scratch registers
#define _PyReg_KWNAMES         4096 // an elided LOAD_CONST of the names of
                                    // the CALL_KW that reads it: the call
                                    // has the index in its kwnames
#define _PyReg_MOVES           8192 // a parallel move (opcode SWAP): output
                                    // j takes the value of implicit input j
#define _PyReg_SLOT_RECIPE    16384 // its recipe's operands are pairs of a
                                    // source slot and a home, not registers
#define _PyReg_MOVE_SLOTS     32768 // a Move that resolution made: in[]
                                    // holds its two counts, then its
                                    // destination and source slots

typedef struct _PyRegOp {
    int opcode;             // the stack instruction, or _PyReg_DEAD
    int oparg;
    int16_t nread;          // registers read, the popped ones on top
    int16_t npop;
    int16_t nimplicit;      // registers live until the op, after in[nread]
    int16_t nout;
    /* The ingress recipe of the exceptional edge: the handler finds the
     * last nrecipe implicit inputs at the parameter homes that
     * operands[recipe ...] number by stack position.  The unwinder moves
     * them there. */
    int16_t nrecipe;
    int recipe;
    /* Moves that allocation put before the op, run in parallel: pairs of a
     * source register and the register that takes its value, whose later
     * reads name it, in operands[stage ...] */
    int16_t nstage;
    int stage;
    int in;                 // index of in[0] in the operand pool
    int out;                // index of out[0]
    int dst;                // the local that dst names instead of out[], -1
    int target;             // the block a jump goes to, or -1
    int except;             // the exception handler's block, or -1
    int depth;              // stack depth before the op
    int flags;
    int reader;             // a folded load: the op that reads the local,
    int reader_k;           // and its input index
    int completion;         // index + 1 of the frontend END_SEND whose
                            // allocated recipient this semantic producer owns
    int noperands;          // a call: the number of operand fields, set by
                            // the assembler (IR_C of its head)
    int kwnames;            // a CALL_KW: the index in co_consts of the tuple
                            // of its names, plus one, set by
                            // _PyRegPass_KwNames()
    _Py_SourceLocation loc;
    int srcoff;             // the code-unit offset in co_code of the stack
                            // instruction that the op stands for
    int base;               // the slot of a window, set by compaction
    /* Set by the assembler */
    int handler;
    int fb;
    int word;
    int nwords;
} _PyRegOp;

typedef struct _PyRegBlock {
    int start;              // its ops are cfg->ops[start .. end - 1]
    int end;
    int startdepth;         // stack depth at entry, -1 if unreachable
    int word;               // index of its first word, set by the assembler
    unsigned reachable : 1;
    unsigned is_handler : 1;
    unsigned preserve_lasti : 1;
    unsigned continued : 1;     // continues the previous block's names
    unsigned is_target : 1;     // a jump goes there
    unsigned falls_through : 1; // control goes on to the next block
} _PyRegBlock;

typedef struct _PyRegCfg {
    int nlocalsplus;
    int virtual_kwargs;     // compiler-proven immutable, uncaptured kwargs local
    int generator;          // a return leaves a generator's frame
    int nvregs;             // registers allocated, fresh included
    _PyRegOp *ops;
    int nops;
    int aops;
    int *operands;          // in[] and out[] of the ops
    int noperands;
    int aoperands;
    _PyRegBlock *blocks;
    int nblocks;
    int ablocks;
    int *slot;              // per register less nlocalsplus: its frame slot,
                            // or -1, after compaction
    int ntemps;             // temporary slots the frame needs
    int maxdepth;           // the deepest stack of the stack instructions
    int ncanon;             // canonical temporaries in use: the deepest stack
                            // that conversion numbered, IR_MAX_SLOTS before it
    /* Constant slots (_PyRegCfg_AssignConstSlots()): the registers
     * const_first .. const_first + nconst - 1 are hidden locals that no op
     * writes, whose frame slots hold const_values[j].  Each is an inline
     * scalar or an immortal object. */
    int const_first;
    int nconst;
    PyValueBorrow *const_values;
} _PyRegCfg;

int _PyRegCfg_PrepareKwargs(_PyRegCfg *cfg, int slot);

/* Canonical temporaries are numbered by stack position below the frame
 * limit; the fresh ones follow.  The tables that cover every register
 * (slots, liveness) hold only the canonical ones in use, ncanon of them,
 * and then the fresh ones: _PyReg_INDEX() is the position of a register. */
#define _PyReg_CANON(CFG, K)    ((CFG)->nlocalsplus + (K))
#define _PyReg_FRESH0(CFG)      ((CFG)->nlocalsplus + IR_MAX_SLOTS)
#define _PyReg_INDEX(CFG, V) \
    ((V) < _PyReg_FRESH0(CFG) ? (V) - (CFG)->nlocalsplus \
     : (CFG)->ncanon + (V) - _PyReg_FRESH0(CFG))
#define _PyReg_IS_LOCAL(CFG, V) ((V) < (CFG)->nlocalsplus)
#define _PyReg_IS_CANON(CFG, V) \
    ((V) >= (CFG)->nlocalsplus && (V) < _PyReg_FRESH0(CFG))
#define _PyReg_IN(CFG, OP)      (&(CFG)->operands[(OP)->in])
#define _PyReg_OUT(CFG, OP)     (&(CFG)->operands[(OP)->out])

/* The number of leading outputs of a register-style op that pass an input
 * through, those below the output that dst names.  A call reads its inputs
 * from operand words and has none: its one output is new.  A Move's
 * destinations are new too. */
static inline int
_PyRegOp_Through(const _PyRegOp *op, const _PyStackFormat *fmt)
{
    if (_PyIR_CALL_SHAPED(fmt->operands) ||
        fmt->operands == _PyIR_OPERANDS_SPANS) {
        return 0;
    }
    if (fmt->dst == _PyStack_NO_DST) {
        return op->nout;
    }
    // An op that reads an input without popping it (ImportFrom.From) puts
    // its output above the input: nothing passes through
    return Py_MAX(op->npop + fmt->dst, 0);
}

/* Whether op reads its input k through a field that may name a local: the
 * callable, an argument or a call's iterable or kwargs.  The self input
 * (always a temporary), and the names of CALL_KW, are not. */
static inline int
_PyRegOp_CallBorrows(const _PyRegOp *op, int k)
{
    return k != 1 && !(op->opcode == ROP_OP_CallKw_Generic && k == op->nread - 1);
}

/* The translator's role of field f of an arm's word format.  A word that
 * reads its home group and has a destination of its own (ForIter, Send,
 * CleanupThrow) steps the group as a window from its base (GROUP).  A
 * word that spends its group reads the parts as inputs after its sources
 * (IterClose).  A defined group (GROUP_OUT, IterOpen's pair) is the word's
 * result. */
static inline int
_PyRegCfg_ArmRole(const _PyIRFormat *fmt, int f)
{
    int role = fmt->roles[f];
    if (role != _PyIR_ROLE_GROUP) {
        return role;
    }
    if (fmt->flags & _PyIR_GROUP_OUT) {
        return _PyIR_ROLE_DST;
    }
    if (_PyIR_HasDst(fmt)) {
        return _PyIR_ROLE_GROUP;
    }
    int nsrc = 0;
    for (int g = 0; g < 4; g++) {
        nsrc += fmt->roles[g] >= _PyIR_ROLE_SRC;
    }
    return _PyIR_ROLE_SRC + nsrc;
}

void _PyRegCfg_ArmFormat(int handler, _PyStackFormat *row);

/* The translator's row of a CFG opcode: a stack op's (_PyStack_Formats), or
 * an arm's, which _PyRegCfg_ArmFormat derives from its word format into
 * row */
const _PyStackFormat *_PyRegCfg_FormatInto(int opcode, _PyStackFormat *row);

/* The same, into a row that lives as long as the enclosing block */
#define _PyRegCfg_FormatOf(opcode) \
    _PyRegCfg_FormatInto((opcode), &(_PyStackFormat){0})

/* The role of the field of row fmt that holds the oparg: IMM or a slot
 * role; NONE if no single field holds it. */
static inline int
_PyStack_OpargRole(const _PyStackFormat *fmt)
{
    if (fmt->imm >= _PyIR_LOC_A && fmt->imm <= _PyIR_LOC_D) {
        return fmt->roles[fmt->imm - _PyIR_LOC_A];
    }
    return _PyIR_ROLE_NONE;
}

/* Whether an op of row fmt may run Python code, which may rebind any
 * local through frame.f_locals: its row says so, or it releases the old
 * value of a local, whose __del__ may.  It does so through a dst that
 * names a local (_PyStack_STORE_DST, where dst_local says that dst does), and
 * through an oparg whose role is _PyIR_ROLE_LOCAL_OUT or
 * _PyIR_ROLE_LOCAL_CLEAR. */
static inline int
_PyStack_MayRunPython(const _PyStackFormat *fmt, int dst_local)
{
    int role = _PyStack_OpargRole(fmt);
    return (fmt->flags & _PyIR_RUNS_CODE)
           || (dst_local && (fmt->flags & _PyStack_STORE_DST))
           || role == _PyIR_ROLE_LOCAL_OUT
           || role == _PyIR_ROLE_LOCAL_CLEAR;
}

/* The spans of a span row: its COUNT fields (_PyIR_SpanCountFields) */
static inline int
_PyStack_SpanCount(const _PyStackFormat *fmt)
{
    int n = 0;
    for (int f = 0; f < 4; f++) {
        n += fmt->roles[f] == _PyIR_ROLE_COUNT;
    }
    return fmt->operands == _PyIR_OPERANDS_SPANS ? n : 0;
}

/* A row whose one span is destinations (Unpack's): it has no DST role */
static inline int
_PyStack_IsDstSpan(const _PyStackFormat *fmt)
{
    if (_PyStack_SpanCount(fmt) != 1) {
        return 0;
    }
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_DST) {
            return 0;
        }
    }
    return 1;
}

/* A Move's row: two spans, its destinations and its sources */
static inline int
_PyStack_IsMove(const _PyStackFormat *fmt)
{
    return _PyStack_SpanCount(fmt) == 2;
}

/* Whether op writes local z through its dst, or, for a word whose outputs
 * are a destination span (Unpack), as one of its outputs */
static inline int
_PyRegOp_WritesLocal(const _PyRegCfg *cfg, const _PyRegOp *op, int z)
{
    if (op->dst >= 0) {
        return z == op->dst;
    }
    if (op->nout == 0 || !_PyStack_IsDstSpan(_PyRegCfg_FormatOf(op->opcode))) {
        return 0;
    }
    const int *out = _PyReg_OUT(cfg, op);
    for (int j = 0; j < op->nout; j++) {
        if (out[j] == z) {
            return 1;
        }
    }
    return 0;
}

/* Building: blocks in layout order, each with its stack instructions,
 * whose targets and handlers are block indices. */
_PyRegCfg *_PyRegCfg_New(int nlocalsplus, int maxdepth);
void _PyRegCfg_Free(_PyRegCfg *cfg);
int _PyRegCfg_AddBlock(_PyRegCfg *cfg, int handler_depth, int preserve_lasti,
                       int depth_hint);
int _PyRegCfg_AddInstr(_PyRegCfg *cfg, int opcode, int oparg,
                       _Py_SourceLocation loc, int srcoff, int target,
                       int except);

/* Translate the stack instructions into register ops, run the passes and
 * assign the slots. */
int _PyRegCfg_Build(_PyRegCfg *cfg);

/* The translator's rules (Python/regcfg.c).  A rule translates one stack
 * op: it emits register ops that name the identities the op consumes, then
 * pops them, and pushes the identities it produces.  An op is emitted
 * before its inputs are popped (see _PyRegTr_Emit()).  An emitted op keeps
 * src's origin (srcoff, loc, except). */
typedef struct _PyRegTr _PyRegTr;
typedef int (*_PyRegRule)(_PyRegTr *t, const _PyRegOp *src);

/* The top n identities, bottom first (ids may be NULL), removed from the
 * stack once the ops that read them are emitted */
int _PyRegTr_Pop(_PyRegTr *t, int n, int *ids);
int _PyRegTr_Push(_PyRegTr *t, int id);
/* A new identity, defined by the next op emitted */
int _PyRegTr_Fresh(_PyRegTr *t, int *id);
/* An op of the given opcode that reads uses and defines defs, by name */
int _PyRegTr_Emit(_PyRegTr *t, const _PyRegOp *src, int opcode,
                  const int *uses, int nuses, const int *defs, int ndefs,
                  _PyRegOp **op);
/* The same for an op that takes only the last ntaken of its inputs */
int _PyRegTr_EmitTaking(_PyRegTr *t, const _PyRegOp *src, int opcode,
                        const int *uses, int nuses, int ntaken,
                        const int *defs, int ndefs, _PyRegOp **op);
/* The rule of an unported stack op: its old handler op */
int _PyRegTr_Legacy(_PyRegTr *t, const _PyRegOp *src);

/* Give the inline scalar or immortal constants that ops read in place,
 * through register fields and call operands alike, a hidden slot each (G6),
 * eight at most: the constant loads that copy propagation would fold if the
 * constant were a local go, the readers name the slot, and the slots are
 * inserted after the locals, cells and free variables (nlocalsplus of them),
 * with the discard slot after them; the temporaries move up.  consts is the
 * tuple co_consts.  Compacts again. */
int _PyRegCfg_AssignConstSlots(_PyRegCfg *cfg, PyObject *consts);

/* Assign the constant slots and encode the ops as the IR of co: the second
 * half of _PyIR_LowerCode() (Python/ir.c). */
int _PyIR_Assemble(PyCodeObject *co, _PyRegCfg *cfg, _PyCodeIR **result);

/* Whether the recipe of op's exceptional edge supplies the parameter of
 * stack position k */
static inline int
_PyRegOp_RecipeDst(const _PyRegCfg *cfg, const _PyRegOp *op, int k)
{
    for (int q = 0; q < op->nrecipe; q++) {
        if (cfg->operands[op->recipe + q] == k) {
            return 1;
        }
    }
    return 0;
}

/* Whether register v is a value that op's recipe moves */
static inline int
_PyRegOp_RecipeSource(const _PyRegCfg *cfg, const _PyRegOp *op, int v)
{
    const int *src = &cfg->operands[op->in + op->nread + op->nimplicit
                                    - op->nrecipe];
    for (int q = 0; q < op->nrecipe; q++) {
        if (src[q] == v) {
            return 1;
        }
    }
    return 0;
}

/* The slot of register v: v for a local, its slot for a temporary. */
static inline int
_PyRegCfg_Slot(const _PyRegCfg *cfg, int v)
{
    if (v < cfg->nlocalsplus) {
        return v;
    }
    return cfg->slot[_PyReg_INDEX(cfg, v)];
}

/* The slot of each role of an op's word */
typedef struct {
    int out[2];         // the outputs, in the order of the encoding's DST roles
    int base;           // the deepest input of a window-style op
    int in[4];          // the inputs of a register-style one, bottom first
} _PyRegSlots;

void _PyRegCfg_OpSlots(const _PyRegCfg *cfg, const _PyRegOp *op,
                       _PyRegSlots *slots);

/* Internal to Python/regcfg.c and Python/regcfg_passes.c */

/* Liveness, per op: whether in[k] (implicit ones included) is read for the
 * last time there, and whether out[j] is never read.  Indexed like
 * operands. */
typedef struct {
    uint8_t *last;          // per operand
    uint64_t **live_in;     // per block, a bit set over temporaries
    uint64_t **live_out;
    int nwords;             // per bit set
} _PyRegLiveness;

int _PyRegCfg_Liveness(_PyRegCfg *cfg, _PyRegLiveness *lv);
void _PyRegCfg_FreeLiveness(_PyRegLiveness *lv);
int _PyRegCfg_Compact(_PyRegCfg *cfg);
int _PyRegCfg_Verify(_PyRegCfg *cfg, const char *after, int physical);

/* The passes, in the order the driver runs them */
int _PyRegPass_CopyProp(_PyRegCfg *cfg);
int _PyRegPass_StoreElim(_PyRegCfg *cfg);
int _PyRegPass_KwNames(_PyRegCfg *cfg);
int _PyRegPass_ElideNull(_PyRegCfg *cfg);
int _PyRegPass_CallRoles(_PyRegCfg *cfg);
int _PyRegCfg_VerifyCallRoles(_PyRegCfg *cfg);
int _PyRegPass_TruthFold(_PyRegCfg *cfg);
/* Returns the number of constant slots, or -1 */
int _PyRegPass_ConstSlots(_PyRegCfg *cfg, PyObject *consts);
void _PyRegPass_DiscardSlot(_PyRegCfg *cfg);

/* For tests only (_testinternalcapi.compile_without_copyprop_kill()): if
 * keep is set, copy propagation keeps a pending load across an op that may
 * run Python, a fold that the verifier must then reject.  Returns the old
 * setting. */
PyAPI_FUNC(int) _PyRegPass_SetKeepAcrossCode(int keep);

/* Reserve n operands in the pool, which may move it; *index is the first. */
int _PyRegCfg_AddOperands(_PyRegCfg *cfg, int n, int *index);

/* Replace register from by to in every op of the region of block b. */
void _PyRegCfg_Rename(_PyRegCfg *cfg, int b, int from, int to);

/* Whether no op after one with this opcode is reached from it.  A
 * CLEANUP_THROW falls through in the stack bytecode, but its word always
 * continues at the exit that it names. */
static inline int
_PyRegOp_EndsFlow(int opcode)
{
    return IS_NO_FALLTHROUGH_OPCODE(opcode)
           || opcode == CLEANUP_THROW
           || opcode == ROP_OP_CleanupThrow_Generic;
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_REGCFG_H */
