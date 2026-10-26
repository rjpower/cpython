#ifndef Py_INTERNAL_REGCFG_H
#define Py_INTERNAL_REGCFG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"          // IR_MAX_SLOTS
#include "pycore_symtable.h"    // _Py_SourceLocation

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
#define _PyReg_LOAD_FOLDED      1   // a load that its consumer reads in place
#define _PyReg_STORE_FOLDED     2   // a store that its producer does

typedef struct _PyRegOp {
    int opcode;             // the stack instruction, or _PyReg_DEAD
    int oparg;
    int16_t nread;          // registers read, the popped ones on top
    int16_t npop;
    int16_t nimplicit;      // registers live until the op, after in[nread]
    int16_t nout;
    int in;                 // index of in[0] in the operand pool
    int out;                // index of out[0]
    int dst;                // the local that dst names instead of out[], -1
    int target;             // the block a jump goes to, or -1
    int except;             // the exception handler's block, or -1
    int depth;              // stack depth before the op
    int flags;
    int reader;             // a folded load: the op that reads the local,
    int reader_k;           // and its input index
    _Py_SourceLocation loc;
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
} _PyRegCfg;

/* Canonical temporaries are numbered by stack position below the frame
 * limit; the fresh ones follow. */
#define _PyReg_CANON(CFG, K)    ((CFG)->nlocalsplus + (K))
#define _PyReg_FRESH0(CFG)      ((CFG)->nlocalsplus + IR_MAX_SLOTS)
#define _PyReg_IS_LOCAL(CFG, V) ((V) < (CFG)->nlocalsplus)
#define _PyReg_IS_CANON(CFG, V) \
    ((V) >= (CFG)->nlocalsplus && (V) < _PyReg_FRESH0(CFG))
#define _PyReg_IN(CFG, OP)      (&(CFG)->operands[(OP)->in])
#define _PyReg_OUT(CFG, OP)     (&(CFG)->operands[(OP)->out])

/* Building: blocks in layout order, each with its stack instructions,
 * whose targets and handlers are block indices. */
_PyRegCfg *_PyRegCfg_New(int nlocalsplus, int maxdepth);
void _PyRegCfg_Free(_PyRegCfg *cfg);
int _PyRegCfg_AddBlock(_PyRegCfg *cfg, int handler_depth, int preserve_lasti,
                       int depth_hint);
int _PyRegCfg_AddInstr(_PyRegCfg *cfg, int opcode, int oparg,
                       _Py_SourceLocation loc, int target, int except);

/* Convert the stack instructions into register ops, run the passes and
 * assign the slots. */
int _PyRegCfg_Build(_PyRegCfg *cfg);

/* The slot of register v: v for a local, its slot for a temporary. */
static inline int
_PyRegCfg_Slot(const _PyRegCfg *cfg, int v)
{
    if (v < cfg->nlocalsplus) {
        return v;
    }
    return cfg->slot[v - cfg->nlocalsplus];
}

/* The slot of each role of an op's word */
typedef struct {
    int dst;
    int base;           // the deepest input of a window-style op
    int in[4];          // the inputs of a register-style one, bottom first
} _PyRegSlots;

void _PyRegCfg_OpSlots(const _PyRegCfg *cfg, const _PyRegOp *op,
                       _PyRegSlots *slots);

/* The format of the stack instruction of an op: the generic handler's,
 * which its family shares. */
struct _PyIRFormat;
const struct _PyIRFormat *_PyRegCfg_FormatOf(int opcode);

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
int _PyRegPass_SwapRename(_PyRegCfg *cfg);
int _PyRegPass_CopyProp(_PyRegCfg *cfg);
int _PyRegPass_StoreElim(_PyRegCfg *cfg);
int _PyRegPass_PopTop(_PyRegCfg *cfg);

/* Replace register from by to in every op of the region of block b. */
void _PyRegCfg_Rename(_PyRegCfg *cfg, int b, int from, int to);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_REGCFG_H */
