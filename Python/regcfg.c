/* The register CFG (boil/design/07-compiler-passes.md): conversion from
 * stack instructions, liveness, compaction and the verifier.  The passes
 * are in Python/regcfg_passes.c; Include/internal/pycore_regcfg.h
 * describes the representation.
 *
 * The flowgraph optimiser hands over its blocks in layout order after it
 * has finished (_PyCfg_OptimizedCfgToRegCfg() in Python/flowgraph.c): the
 * pseudo-instructions are gone, the jumps are normalised and
 * optimize_load_fast() has decided which loads borrow.  _PyRegCfg_Build()
 * then
 *
 *   1. translates every stack op into register ops over value identities
 *      (translate());
 *   2. runs the passes, once each, in a fixed order (run_passes());
 *   3. computes liveness and assigns each temporary a frame slot
 *      (_PyRegCfg_Compact()).
 *
 * Debug builds verify the CFG after each pass, and every build checks what
 * the encoding relies on after compaction.
 */

#include "Python.h"

#include "pycore_initconfig.h"        // _Py_GetEnv()
#include "pycore_intrinsics.h"        // MAX_INTRINSIC_1
#include "pycore_bitutils.h"          // _Py_bit_length()
#include "pycore_ir.h"                // _PyIR_FormatOf()
#include "pycore_opcode_metadata.h"   // _PyOpcode_num_popped()
#include "pycore_opcode_utils.h"      // IS_SCOPE_EXIT_OPCODE()
#include "pycore_pystate.h"           // _Py_GetConfig()
#include "pycore_regcfg.h"
#include "pycore_rop_metadata.h"       // ROP_HANDLER_GetAttr_Generic
#include "pycore_rop_metadata.h"      // ROP_HANDLER_Add_Generic, ROP_HANDLER_Call_Generic
#include "pycore_rop_rows.h"          // BRANCH_IF_FALSE
#include "pycore_jit.h"             // _PyJIT_OFF

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) < 0) {          \
        return ERROR;       \
    }


/* Building */

_PyRegCfg *
_PyRegCfg_New(int nlocalsplus, int maxdepth)
{
    _PyRegCfg *cfg = PyMem_Calloc(1, sizeof(_PyRegCfg));
    if (cfg == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    cfg->nlocalsplus = nlocalsplus;
    cfg->maxdepth = maxdepth;
    cfg->ncanon = IR_MAX_SLOTS;
    cfg->nvregs = _PyReg_FRESH0(cfg);
    return cfg;
}

void
_PyRegCfg_Free(_PyRegCfg *cfg)
{
    if (cfg == NULL) {
        return;
    }
    PyMem_Free(cfg->ops);
    PyMem_Free(cfg->operands);
    PyMem_Free(cfg->blocks);
    PyMem_Free(cfg->slot);
    PyMem_Free(cfg->const_values);
    PyMem_Free(cfg);
}

static int
kwargs_read(const _PyRegOp *op, int slot)
{
    return op->oparg == slot &&
        (op->opcode == LOAD_FAST || op->opcode == LOAD_FAST_CHECK ||
         op->opcode == LOAD_FAST_BORROW);
}

/* Run before conversion and load folding. An immutable kwargs binding needs
 * a barrier on its read paths only; unrelated local loads keep their ABI. */
int
_PyRegCfg_PrepareKwargs(_PyRegCfg *cfg, int slot)
{
    int reads = 0;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->oparg != slot) {
            continue;
        }
        if (kwargs_read(op, slot)) {
            reads++;
        }
        else if (OPCODE_HAS_LOCAL(op->opcode) || op->opcode == MAKE_CELL ||
                 op->opcode == LOAD_DEREF || op->opcode == STORE_DEREF ||
                 op->opcode == DELETE_DEREF || op->opcode == LOAD_CLOSURE)
        {
            return SUCCESS;
        }
    }
    _PyRegOp *ops = PyMem_Calloc((size_t)cfg->nops + reads, sizeof(*ops));
    if (ops == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    int next = 0;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *block = &cfg->blocks[b];
        int start = block->start, end = block->end;
        block->start = next;
        for (int i = start; i < end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (kwargs_read(op, slot)) {
                ops[next++] = (_PyRegOp) {
                    .opcode = MATERIALIZE_KWARGS,
                    .oparg = slot,
                    .target = -1,
                    .except = op->except,
                    .dst = -1,
                    .loc = op->loc,
                    .srcoff = op->srcoff,
                };
            }
            ops[next++] = *op;
        }
        block->end = next;
    }
    PyMem_Free(cfg->ops);
    cfg->ops = ops;
    cfg->nops = cfg->aops = next;
    cfg->virtual_kwargs = 1;
    return SUCCESS;
}

static int
grow(void **array, int *alloc, int needed, size_t item)
{
    if (needed <= *alloc) {
        return SUCCESS;
    }
    int n = Py_MAX(16, *alloc);
    while (n < needed) {
        if (n > INT_MAX / 2) {
            PyErr_NoMemory();
            return ERROR;
        }
        n *= 2;
    }
    void *p = PyMem_Realloc(*array, (size_t)n * item);
    if (p == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    *array = p;
    *alloc = n;
    return SUCCESS;
}

/* Add a block after the others.  handler_depth is the stack depth at its
 * entry if it is an exception handler, exception and lasti included, or
 * -1; depth_hint is the depth that the flowgraph found, which only
 * unreachable code uses, or -1. */
int
_PyRegCfg_AddBlock(_PyRegCfg *cfg, int handler_depth, int preserve_lasti,
                   int depth_hint)
{
    RETURN_IF_ERROR(grow((void **)&cfg->blocks, &cfg->ablocks,
                         cfg->nblocks + 1, sizeof(_PyRegBlock)));
    _PyRegBlock *b = &cfg->blocks[cfg->nblocks++];
    memset(b, 0, sizeof(*b));
    b->start = b->end = cfg->nops;
    b->startdepth = handler_depth;
    b->is_handler = handler_depth >= 0;
    b->preserve_lasti = preserve_lasti != 0;
    b->word = depth_hint;   // until mark_blocks() has the depths
    return SUCCESS;
}

/* Add a stack instruction to the last block.  target and except are block
 * indices or -1. */
int
_PyRegCfg_AddInstr(_PyRegCfg *cfg, int opcode, int oparg,
                   _Py_SourceLocation loc, int srcoff, int target, int except)
{
    assert(cfg->nblocks > 0);
    RETURN_IF_ERROR(grow((void **)&cfg->ops, &cfg->aops, cfg->nops + 1,
                         sizeof(_PyRegOp)));
    int self = cfg->nblocks - 1;
    // The direction of an unconditional jump follows from the layout
    if (opcode == JUMP) {
        opcode = target > self ? JUMP_FORWARD : JUMP_BACKWARD;
    }
    else if (opcode == JUMP_NO_INTERRUPT) {
        opcode = target > self ? JUMP_FORWARD : JUMP_BACKWARD_NO_INTERRUPT;
    }
    assert(!IS_PSEUDO_INSTR(opcode));
    _PyRegOp *op = &cfg->ops[cfg->nops++];
    memset(op, 0, sizeof(*op));
    op->opcode = opcode;
    op->oparg = oparg;
    op->loc = loc;
    op->srcoff = srcoff;
    // The oparg names a word: the target, or for END_ASYNC_FOR the SEND
    op->target = (_PyRegCfg_FormatOf(opcode, oparg)->flags
                  & (_PyIR_JUMPS | _PyIR_FORWARD)) ? target : -1;
    op->except = except;
    op->dst = -1;
    op->reader = -1;
    op->depth = -1;
    op->fb = -1;
    cfg->blocks[self].end = cfg->nops;
    return SUCCESS;
}


/* Formats */

const _PyIRFormat *
_PyRegCfg_FormatOf(int opcode, int oparg)
{
    if (opcode == YIELD_VALUE && oparg == 1) {
        return _PyIR_FormatOf(YIELD_DELEGATED);
    }
    // The handlers of BINARY_OP's operations share its format
    if (opcode == BINARY_OP) {
        return _PyIR_FormatOf(_PyIR_BINARY_OP_FIRST);
    }
    return _PyIR_FormatOf(opcode);
}

/* Whether op jumps to its target.  END_ASYNC_FOR names the SEND of its
 * loop, for sys.monitoring, but does not go there (_PyIR_NAMES_TARGET). */
static inline int
op_jumps(const _PyRegOp *op)
{
    return op->target >= 0
           && !(_PyRegCfg_FormatOf(op->opcode, op->oparg)->flags & _PyIR_NAMES_TARGET);
}

/* The stack depth at the entry of exception handler block h, without the
 * exception and lasti: the depth the unwinder pops to. */
static inline int
handler_base(const _PyRegCfg *cfg, int h)
{
    const _PyRegBlock *hb = &cfg->blocks[h];
    return hb->startdepth - 1 - hb->preserve_lasti;
}


/* Translation */

/* The depth that the block needs at its start not to underflow, or -1 with
 * an exception set. */
static int
unreached_depth(const _PyRegCfg *cfg, const _PyRegBlock *blk)
{
    int d = 0, low = 0;
    for (int i = blk->start; i < blk->end; i++) {
        const _PyRegOp *op = &cfg->ops[i];
        int popped = _PyOpcode_num_popped(op->opcode, op->oparg);
        int pushed = _PyOpcode_num_pushed(op->opcode, op->oparg);
        if (popped < 0 || pushed < 0) {
            PyErr_SetString(PyExc_SystemError, "invalid opcode");
            return -1;
        }
        low = Py_MIN(low, d - popped);
        d += pushed - popped;
    }
    return -low;
}

/* Fill in the flags of the blocks and their depths at entry. */
static int
mark_blocks(_PyRegCfg *cfg)
{
    int n = cfg->nblocks;
    for (int b = 0; b < n; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        blk->falls_through = 1;
        if (blk->end > blk->start) {
            int last = cfg->ops[blk->end - 1].opcode;
            blk->falls_through = !_PyRegOp_EndsFlow(last);
        }
        if (b == n - 1) {
            blk->falls_through = 0;
        }
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->target >= 0) {
                cfg->blocks[op->target].is_target = 1;
            }
        }
    }
    for (int b = 1; b < n; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        blk->continued = !blk->is_target && !blk->is_handler
                         && cfg->blocks[b - 1].falls_through;
    }

    // Depths, from the entry and from the handlers that ops name
    int *depth = PyMem_Malloc(Py_MAX(n, 1) * sizeof(int));
    int *todo = PyMem_Malloc(Py_MAX(n, 1) * sizeof(int));
    if (depth == NULL || todo == NULL) {
        PyMem_Free(depth);
        PyMem_Free(todo);
        PyErr_NoMemory();
        return ERROR;
    }
    for (int b = 0; b < n; b++) {
        depth[b] = -1;
    }
    int ntodo = 0;
    // A push that disagrees with a depth already found is an error in code
    // that runs; code that never runs keeps the depth it found first
    int dead = 0;
#define PUSH(B, D) \
    do { \
        int b_ = (B), d_ = (D); \
        if (depth[b_] < 0) { \
            depth[b_] = d_; \
            todo[ntodo++] = b_; \
        } \
        else if (depth[b_] != d_ && !dead) { \
            PyErr_Format(PyExc_SystemError, \
                         "inconsistent stack depth at block %d", b_); \
            goto error; \
        } \
    } while (0)
    if (n > 0) {
        PUSH(0, 0);
    }
    int scan = 0;
    for (;;) {
        while (ntodo > 0) {
            int b = todo[--ntodo];
            _PyRegBlock *blk = &cfg->blocks[b];
            int d = depth[b];
            for (int i = blk->start; i < blk->end; i++) {
                _PyRegOp *op = &cfg->ops[i];
                int popped = _PyOpcode_num_popped(op->opcode, op->oparg);
                int pushed = _PyOpcode_num_pushed(op->opcode, op->oparg);
                if (popped < 0 || pushed < 0 || d < popped) {
                    if (dead) {
                        break;
                    }
                    PyErr_Format(PyExc_SystemError,
                                 "stack underflow at line %d", op->loc.lineno);
                    goto error;
                }
                if (op->except >= 0) {
                    PUSH(op->except, cfg->blocks[op->except].startdepth);
                }
                d += pushed - popped;
                if (op_jumps(op)) {
                    PUSH(op->target, d);
                }
            }
            if (blk->falls_through) {
                PUSH(b + 1, d);
            }
        }
        if (!dead) {
            for (int b = 0; b < n; b++) {
                cfg->blocks[b].reachable = depth[b] >= 0;
            }
            dead = 1;
        }
        // Code that never runs still has stack depths that agree along its
        // own edges, which the loops in it need.  Start from each block that
        // no other block enters, with the depth its handler entry or the
        // flowgraph gives, or the depth it needs not to underflow.
        int seed = -1;
        while (scan < n) {
            _PyRegBlock *blk = &cfg->blocks[scan];
            if (depth[scan] < 0 &&
                (blk->is_handler ||
                 (!blk->is_target && !(scan > 0 && cfg->blocks[scan - 1].falls_through))))
            {
                seed = blk->is_handler ? blk->startdepth
                       : blk->word >= 0 ? blk->word
                       : unreached_depth(cfg, blk);
                if (seed < 0) {
                    goto error;
                }
                break;
            }
            scan++;
        }
        if (scan >= n) {
            break;
        }
        PUSH(scan, seed);
    }
#undef PUSH
    for (int b = 0; b < n; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        int hint = blk->word;
        blk->word = 0;
        if (depth[b] >= 0) {
            blk->startdepth = depth[b];
            if (!blk->reachable) {
                blk->continued = 0;
            }
            continue;
        }
        // No path reaches the block, so it never runs.  It gets the depth
        // the flowgraph found, or the depth it needs not to underflow.
        blk->continued = 0;
        if (blk->is_handler) {
            continue;
        }
        if (hint >= 0) {
            blk->startdepth = hint;
            continue;
        }
        blk->startdepth = unreached_depth(cfg, blk);
        if (blk->startdepth < 0) {
            goto error;
        }
    }
    // A continued block is entered with its predecessor's names, so both
    // must be reachable or neither
    for (int b = 1; b < n; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (blk->continued && !cfg->blocks[b - 1].reachable) {
            blk->continued = 0;
        }
    }
    PyMem_Free(depth);
    PyMem_Free(todo);
    return SUCCESS;
error:
    PyMem_Free(depth);
    PyMem_Free(todo);
    return ERROR;
}

/* Give each instruction at NEXT_LOCATION the location of the next. */
static void
resolve_next_locations(_PyRegCfg *cfg)
{
    for (int i = cfg->nops - 1; i >= 0; i--) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->loc.lineno == NEXT_LOCATION.lineno) {
            if (IS_TERMINATOR_OPCODE(op->opcode)) {
                op->loc = NO_LOCATION;
            }
            else {
                assert(i < cfg->nops - 1);
                op->loc = op[1].loc;
            }
        }
    }
}

/* The translator.
 *
 * Stack ops are transport: their meaning lives in the rules below, which
 * turn each one into register ops at first call.  The translator keeps,
 * only while it runs, the stack of value identities: the virtual register
 * at each stack position.  A rule pops and pushes identities and emits the
 * register ops that name them.  rules[] is indexed by opcode; an opcode
 * without a rule is an unported family, which _PyRegTr_Legacy() turns into
 * its old handler op with the operand conventions that handler expects.
 *
 * At a block that is not continued, position k holds the block parameter
 * CANON(k), whose home is slot nlocalsplus + k (a bridge constraint until
 * every consumer reads named operands): every edge into the block, and
 * every op whose handler finds position k, gives CANON(k) the identity at
 * position k.  It does so by renaming the identity, where the parameter is
 * free from the identity's definition on; after SWAP has permuted
 * identities it may not be.  A normal edge then runs a parallel move (an
 * op with _PyReg_MOVES) and an exceptional edge carries an ingress recipe:
 * the identities at the handler's positions, which the unwinder moves to
 * the parameters' homes.  There is one translation; allocation places the
 * homes and stages the values that an op needs elsewhere. */

struct _PyRegTr {
    _PyRegCfg *cfg;
    _PyRegOp *ops;          // the translated ops
    int nops;
    int aops;
    int *stack;             // the identity at each stack position
    int sp;
    int astack;
    int region_op;          // the first op of the current region
    int block_op;           // the first op of the current block
    int *def;               // per fresh register, the op that defines it
    int *natural;           // per fresh register, its stack position there
    int adef;
    int anatural;
    int maxpos;             // the deepest stack position numbered
    int touched;            // ... in any translation, kept or not
    /* Per parameter CANON(k): whether an identity holds it now, and the op
     * that last took it in this region (-1: none) */
    char *held;
    int *free_since;
    int permuted;           // the region elided a SWAP
    int src_sp;             // the depth before the stack op being translated
    /* The recipe of the op being translated, and the moves of an edge:
     * pairs of an identity and the stack position of the parameter that it
     * becomes */
    int *rec;
    int nrec;
    int arec;
    int *mv;
    int nmv;
    int amv;
    /* Per fresh register, whether it holds a binding that SaveLocal took,
     * which may be absent; per stack position, whether such a binding was
     * ever there, so that its parameter may hold one */
    char *maybe;
    int amaybe;
    char *maybe_pos;
    /* The Move.Copy ops of this region that rule_copy() emitted and that
     * collapse_unwound() may still merge, by op index */
    int *copies;
    int ncopies;
    int acopies;
};

int
_PyRegTr_Fresh(_PyRegTr *t, int *v)
{
    _PyRegCfg *cfg = t->cfg;
    int k = cfg->nvregs - _PyReg_FRESH0(cfg);
    RETURN_IF_ERROR(grow((void **)&t->def, &t->adef, k + 1, sizeof(int)));
    RETURN_IF_ERROR(grow((void **)&t->natural, &t->anatural, k + 1,
                         sizeof(int)));
    RETURN_IF_ERROR(grow((void **)&t->maybe, &t->amaybe, k + 1, 1));
    t->def[k] = t->nops;
    t->natural[k] = -1;
    t->maybe[k] = 0;
    *v = cfg->nvregs++;
    return SUCCESS;
}

int
_PyRegTr_Pop(_PyRegTr *t, int n, int *ids)
{
    if (n > t->sp) {
        PyErr_SetString(PyExc_SystemError, "translator stack underflow");
        return ERROR;
    }
    t->sp -= n;
    for (int k = 0; ids != NULL && k < n; k++) {
        ids[k] = t->stack[t->sp + k];
    }
    return SUCCESS;
}

int
_PyRegTr_Push(_PyRegTr *t, int id)
{
    RETURN_IF_ERROR(grow((void **)&t->stack, &t->astack, t->sp + 2,
                         sizeof(int)));
    _PyRegCfg *cfg = t->cfg;
    if (id >= _PyReg_FRESH0(cfg)) {
        int *natural = &t->natural[id - _PyReg_FRESH0(cfg)];
        if (*natural < 0) {
            *natural = t->sp;
        }
    }
    t->stack[t->sp++] = id;
    t->maxpos = Py_MAX(t->maxpos, t->sp);
    t->touched = Py_MAX(t->touched, t->sp);
    return SUCCESS;
}

static void
rename_ops(_PyRegCfg *cfg, _PyRegOp *ops, int from_op, int to_op,
           int from, int to)
{
    for (int i = from_op; i < to_op; i++) {
        _PyRegOp *op = &ops[i];
        int *in = &cfg->operands[op->in];
        for (int k = 0; k < op->nread + op->nimplicit; k++) {
            if (in[k] == from) {
                in[k] = to;
            }
        }
        int *out = &cfg->operands[op->out];
        for (int j = 0; j < op->nout; j++) {
            if (out[j] == from) {
                out[j] = to;
            }
        }
        int *stage = &cfg->operands[op->stage];
        for (int q = 0; q < op->nstage; q++) {
            if (stage[2 * q] == from) {
                stage[2 * q] = to;
            }
        }
    }
}

/* The stack position where identity v was defined: where the allocator
 * puts it, unless a parameter's home or a bump moves it */
static int
natural_pos(const _PyRegTr *t, int v)
{
    const _PyRegCfg *cfg = t->cfg;
    if (v < cfg->nlocalsplus) {
        return -1;
    }
    if (v < _PyReg_FRESH0(cfg)) {
        return v - cfg->nlocalsplus;
    }
    return t->natural[v - _PyReg_FRESH0(cfg)];
}

/* Whether the op that defines v can put it in the home of the parameter of
 * position k: at its own position always; elsewhere only an op whose one
 * new output the allocator places freely (a register-style op, a call, or
 * a window without inputs, whose base follows its output). */
static int
placeable(const _PyRegTr *t, int v, int k)
{
    const _PyRegCfg *cfg = t->cfg;
    int f = v - _PyReg_FRESH0(cfg);
    if (t->natural[f] == k) {
        return 1;
    }
    const _PyRegOp *op = &t->ops[t->def[f]];
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
    if (fmt->flags & _PyIR_WINDOW) {
        return op->nread == 0 && op->nout == 1;
    }
    return op->nout - _PyRegOp_Through(op, fmt) == 1;
}

/* Whether op i reads the parameter of position k at that position, so
 * that its output there may take the parameter's home: a handler may write
 * an output over the input at its own stack position only. */
static int
own_input(const _PyRegTr *t, int i, int k)
{
    const _PyRegCfg *cfg = t->cfg;
    const _PyRegOp *op = &t->ops[i];
    const int *in = _PyReg_IN(cfg, op);
    for (int q = 0; q < op->nread; q++) {
        if (in[q] == _PyReg_CANON(cfg, k)) {
            return op->depth - op->nread + q == k;
        }
    }
    return 0;
}

/* Track which parameters op holds and takes. */
static void
note_op(_PyRegTr *t, const _PyRegOp *op)
{
    _PyRegCfg *cfg = t->cfg;
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    int index = (int)(op - t->ops);
    for (int k = op->nread - op->npop; k < op->nread + op->nimplicit; k++) {
        int v = in[k];
        if (!_PyReg_IS_CANON(cfg, v)) {
            continue;
        }
        int passed = 0;
        for (int j = 0; j < op->nout; j++) {
            passed |= out[j] == v;
        }
        if (!passed) {
            t->held[v - cfg->nlocalsplus] = 0;
            t->free_since[v - cfg->nlocalsplus] = index;
        }
    }
    for (int j = 0; j < op->nout; j++) {
        if (_PyReg_IS_CANON(cfg, out[j])) {
            t->held[out[j] - cfg->nlocalsplus] = 1;
        }
    }
}

/* Whether the identity at stack position k is its block parameter, or can
 * be renamed to it: a fresh identity whose definition can put it in the
 * parameter's home, which no other value holds from there on.  Under last
 * in, first out that always holds. */
static int
rename_param(_PyRegTr *t, int k)
{
    _PyRegCfg *cfg = t->cfg;
    int v = t->stack[k];
    int canon = _PyReg_CANON(cfg, k);
    if (v == canon) {
        return 1;
    }
    if (v < _PyReg_FRESH0(cfg) || k >= IR_MAX_SLOTS) {
        return 0;
    }
    int from_op = t->def[v - _PyReg_FRESH0(cfg)];
    if (t->held[k] || t->free_since[k] > from_op ||
        from_op < t->region_op || !placeable(t, v, k) ||
        (t->free_since[k] == from_op && !own_input(t, from_op, k)))
    {
        return 0;
    }
    rename_ops(cfg, t->ops, Py_MAX(from_op, t->region_op), t->nops,
               v, canon);
    t->stack[k] = canon;
    t->held[k] = 1;
    return 1;
}

static int
stack_error(int v, int k)
{
    PyErr_Format(PyExc_SystemError, "register %d at stack position %d", v, k);
    return ERROR;
}

/* The recipe of an exceptional edge to a handler of the given base: each
 * position below it whose identity is not its parameter */
static int
handler_recipe(_PyRegTr *t, int base)
{
    _PyRegCfg *cfg = t->cfg;
    int n = Py_MIN(base, t->sp);
    t->nrec = 0;
    RETURN_IF_ERROR(grow((void **)&t->rec, &t->arec, Py_MAX(2 * n, 1),
                         sizeof(int)));
    for (int k = 0; k < n; k++) {
        int v = t->stack[k];
        if (v == _PyReg_CANON(cfg, k)) {
            continue;
        }
        if (v < cfg->nlocalsplus || k >= IR_MAX_SLOTS) {
            return stack_error(v, k);
        }
        t->rec[2 * t->nrec] = v;
        t->rec[2 * t->nrec + 1] = k;
        t->nrec++;
    }
    return SUCCESS;
}

static int new_op(_PyRegTr *t, const _PyRegOp *src, int nuses, int ndefs,
                  _PyRegOp **result);

/* An edge's target finds the parameters below depth: rename where that is
 * legal, and move the other identities to their parameters in parallel,
 * in an op before the edge.  The move's recipe is the stack before it:
 * resolution gives each of its words the recipe of the state there. */
static int
bind_edge(_PyRegTr *t, int depth, const _PyRegOp *src)
{
    _PyRegCfg *cfg = t->cfg;
    // Code that never runs may name a handler deeper than its stack
    int n = Py_MIN(depth, t->sp);
    RETURN_IF_ERROR(grow((void **)&t->mv, &t->amv, Py_MAX(2 * n, 1),
                         sizeof(int)));
    t->nmv = 0;
    for (int k = 0; k < n; k++) {
        if (!rename_param(t, k)) {
            if (k >= IR_MAX_SLOTS) {
                return stack_error(t->stack[k], k);
            }
            t->mv[2 * t->nmv] = t->stack[k];
            t->mv[2 * t->nmv + 1] = k;
            t->nmv++;
        }
    }
    int nmv = t->nmv;
    if (nmv == 0) {
        return SUCCESS;
    }
    // A raise before the move finds the stack as it was
    t->nrec = 0;
    if (src->except >= 0) {
        RETURN_IF_ERROR(handler_recipe(t, handler_base(cfg, src->except)));
    }
    int nrec = t->nrec;
    for (int q = 0; q < nmv; q++) {
        t->stack[t->mv[2 * q + 1]] = _PyReg_CANON(cfg, t->mv[2 * q + 1]);
    }
    _PyRegOp *op;
    RETURN_IF_ERROR(new_op(t, src, nmv + nrec, nmv, &op));
    if (nrec) {
        RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, nrec, &op->recipe));
    }
    op->opcode = SWAP;
    op->oparg = 0;
    op->flags = _PyReg_MOVES;
    op->target = -1;
    op->dst = -1;
    op->reader = -1;
    op->completion = 0;
    op->kwnames = 0;
    // Its sources are implicit: no word reads them
    op->nread = op->npop = 0;
    op->nimplicit = (int16_t)nmv;
    int *in = _PyReg_IN(cfg, op);
    int *out = _PyReg_OUT(cfg, op);
    for (int q = 0; q < nmv; q++) {
        in[q] = t->mv[2 * q];
        out[q] = _PyReg_CANON(cfg, t->mv[2 * q + 1]);
    }
    note_op(t, op);
    for (int q = 0; q < nrec; q++) {
        in[nmv + q] = t->rec[2 * q];
        cfg->operands[op->recipe + q] = t->rec[2 * q + 1];
    }
    op->nimplicit = (int16_t)(nmv + nrec);
    op->nrecipe = (int16_t)nrec;
    return SUCCESS;
}

int
_PyRegCfg_AddOperands(_PyRegCfg *cfg, int n, int *index)
{
    RETURN_IF_ERROR(grow((void **)&cfg->operands, &cfg->aoperands,
                         cfg->noperands + n, sizeof(int)));
    *index = cfg->noperands;
    cfg->noperands += n;
    return SUCCESS;
}

/* Append an op that stands for the stack op src: a copy of src, with uses
 * and defs to be filled in by the caller. */
static int
new_op(_PyRegTr *t, const _PyRegOp *src, int nuses, int ndefs,
       _PyRegOp **result)
{
    _PyRegCfg *cfg = t->cfg;
    int in, out;
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, nuses, &in));
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, ndefs, &out));
    RETURN_IF_ERROR(grow((void **)&t->ops, &t->aops, t->nops + 1,
                         sizeof(_PyRegOp)));
    _PyRegOp *op = &t->ops[t->nops++];
    *op = *src;
    op->in = in;
    op->out = out;
    op->nread = (int16_t)nuses;
    op->npop = (int16_t)nuses;
    op->nimplicit = 0;
    op->nout = (int16_t)ndefs;
    op->nrecipe = 0;
    op->recipe = 0;
    op->depth = t->sp;
    *result = op;
    return SUCCESS;
}

#ifndef NDEBUG
/* Whether identity v is at a stack position in [lo, hi) */
static int
on_stack(const _PyRegTr *t, int lo, int hi, int v)
{
    for (int k = lo; k < hi; k++) {
        if (t->stack[k] == v) {
            return 1;
        }
    }
    return 0;
}
#endif

/* An op that reads uses and defines defs.  A rule emits an op before it
 * pops the op's stack inputs: the op's depth is the stack's with them, and
 * its handler's recipe is taken from that stack, so a raise finds every
 * position below the handler's depth where the stack op left it.  An input
 * that an earlier op of the same stack op already popped is an error,
 * unless the identity is still on the stack below. */
int
_PyRegTr_Emit(_PyRegTr *t, const _PyRegOp *src, int opcode,
              const int *uses, int nuses, const int *defs, int ndefs,
              _PyRegOp **result)
{
    _PyRegCfg *cfg = t->cfg;
#ifndef NDEBUG
    for (int k = 0; k < nuses; k++) {
        assert(!on_stack(t, t->sp, Py_MIN(t->src_sp, t->astack), uses[k]) ||
               on_stack(t, 0, t->sp, uses[k]));
    }
#endif
    // An op that may raise carries its handler's recipe
    t->nrec = 0;
    if (src->except >= 0) {
        RETURN_IF_ERROR(handler_recipe(t, handler_base(cfg, src->except)));
    }
    int nrec = t->nrec;
    _PyRegOp *op;
    RETURN_IF_ERROR(new_op(t, src, nuses + nrec, ndefs, &op));
    if (nrec) {
        RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, nrec, &op->recipe));
    }
    op->opcode = opcode;
    op->nread = op->npop = (int16_t)nuses;
    int *in = _PyReg_IN(cfg, op);
    if (nuses > 0) {
        memcpy(in, uses, nuses * sizeof(int));
    }
    if (ndefs > 0) {
        memcpy(_PyReg_OUT(cfg, op), defs, ndefs * sizeof(int));
    }
    note_op(t, op);
    for (int q = 0; q < nrec; q++) {
        in[nuses + q] = t->rec[2 * q];
        cfg->operands[op->recipe + q] = t->rec[2 * q + 1];
    }
    op->nimplicit = (int16_t)nrec;
    op->nrecipe = (int16_t)nrec;
    if (result != NULL) {
        *result = op;
    }
    return SUCCESS;
}

/* How many times op names register v, as an input of any kind or an output */
static int
names_of(const _PyRegCfg *cfg, const _PyRegOp *op, int v)
{
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    int n = 0;
    for (int k = 0; k < op->nread + op->nimplicit; k++) {
        n += in[k] == v;
    }
    for (int j = 0; j < op->nout; j++) {
        n += out[j] == v;
    }
    return n;
}

/* The raise being translated leaves the value at stack position k to the
 * unwinder.  If that value is the source of a provisional Move.Copy whose
 * copy an op T has already taken, one owner covers both uses: T takes the
 * original instead, and the unwinder finds nothing to release.  This is
 * the cleanup of an except or with body, COPY 3; POP_EXCEPT; RERAISE 1,
 * where POP_EXCEPT spends the copy of the previous exception and RERAISE
 * leaves the original behind.  It holds when
 *
 *   - no op between the Move.Copy and the raise jumps or binds an edge,
 *     and the copy is not a parameter, so no other block sees either;
 *   - T names the copy once, among the inputs it takes, and does not name
 *     the original; no other op names the copy;
 *   - no op after T names the original: T is its last use;
 *   - the raise's handler, if any, does not find position k: k is at or
 *     above the handler's depth, so the unwinder clears its home, which T
 *     left empty.
 *
 * Then the copy becomes the original in T, the Move.Copy becomes a
 * source-only op, and the raise does not list the original as a value it
 * leaves.  Returns 1 if it merged, 0 if not. */
static int
collapse_unwound(_PyRegTr *t, const _PyRegOp *instr, int k)
{
    _PyRegCfg *cfg = t->cfg;
    int x = t->stack[k];
    if (instr->except >= 0 && k < handler_base(cfg, instr->except)) {
        return 0;
    }
    for (int q = 0; q < t->ncopies; q++) {
        int c = t->copies[q];
        _PyRegOp *copy = &t->ops[c];
        if (copy->opcode != ROP_HANDLER_Move_Copy || copy->nread != 1 ||
            copy->nout != 1 || _PyReg_IN(cfg, copy)[0] != x)
        {
            continue;
        }
        int y = _PyReg_OUT(cfg, copy)[0];
        if (y < _PyReg_FRESH0(cfg)) {
            return 0;
        }
        int taker = -1;
        for (int i = c + 1; i < t->nops; i++) {
            const _PyRegOp *op = &t->ops[i];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            if (op->target >= 0 || (op->flags & (_PyReg_MOVES | _PyReg_JUMP_OUTPUT))) {
                return 0;
            }
            int ny = names_of(cfg, op, y), nx = names_of(cfg, op, x);
            if (ny == 0) {
                if (nx != 0 && taker >= 0) {
                    return 0;
                }
                continue;
            }
            if (taker >= 0 || ny != 1 || nx != 0) {
                return 0;
            }
            const int *in = _PyReg_IN(cfg, op);
            int taken = 0;
            for (int j = op->nread - op->npop; j < op->nread; j++) {
                taken |= in[j] == y;
            }
            if (!taken) {
                return 0;
            }
            taker = i;
        }
        if (taker < 0) {
            return 0;
        }
        _PyRegOp *T = &t->ops[taker];
        int *in = _PyReg_IN(cfg, T);
        for (int j = 0; j < T->nread; j++) {
            if (in[j] == y) {
                in[j] = x;
            }
        }
        // The Move.Copy stays for its offset, as an op with no word: its
        // recipe follows its one use
        copy->opcode = NOP;
        copy->in++;
        copy->nread = copy->npop = 0;
        copy->nout = 0;
        t->copies[q] = t->copies[--t->ncopies];
        return 1;
    }
    return 0;
}

/* The rule of an unported stack op: its old handler op.  It reads the top
 * nread identities, the popped ones on top, and for a raise every identity
 * below; a register-style op's outputs below its dst pass their inputs
 * through, and a window's are its stack positions from its base. */
int
_PyRegTr_Legacy(_PyRegTr *t, const _PyRegOp *instr)
{
    _PyRegCfg *cfg = t->cfg;
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(instr->opcode, instr->oparg);
    int window = (fmt->flags & _PyIR_WINDOW) != 0;
    int popped = _PyOpcode_num_popped(instr->opcode, instr->oparg);
    // A scope exit leaves nothing behind for this frame: RETURN_VALUE
    // pushes its result to the caller's, and RERAISE does not go on
    int pushed = IS_SCOPE_EXIT_OPCODE(instr->opcode)
        ? 0 : _PyOpcode_num_pushed(instr->opcode, instr->oparg);
    if (instr->opcode == YIELD_VALUE && instr->oparg == 1) {
        /* The frontend leaves receiver/cursor below its one-value yield.
         * Canonical delegated IR retains both as explicit logical uses;
         * allocation must see their identities before assigning homes. */
        popped = 3;
        pushed = 3;
    }
    // Every handler finds its parameters before the op can raise: renamed
    // while the region keeps stack order, else by the edge's recipe
    if (instr->except >= 0 && !t->permuted) {
        int hbase = Py_MIN(handler_base(cfg, instr->except), t->sp);
        for (int k = 0; k < hbase; k++) {
            (void)rename_param(t, k);
        }
    }
    int depth = t->sp;
    assert(popped >= 0 && pushed >= 0 && popped <= depth);
    int jumps = op_jumps(instr);
    if (jumps) {
        // The target finds its parameters.  The window of FOR_ITER and
        // SEND is the stack they leave there.
        RETURN_IF_ERROR(bind_edge(t, window ? depth : depth - popped, instr));
    }
    t->nrec = 0;
    if (instr->except >= 0) {
        RETURN_IF_ERROR(handler_recipe(t, handler_base(cfg, instr->except)));
    }
    int nrec = t->nrec;
    int nread = window ? popped : Py_MAX(popped, fmt->ninputs);
    if (nread > depth) {
        PyErr_Format(PyExc_SystemError, "stack underflow at line %d",
                     instr->loc.lineno);
        return ERROR;
    }
    int nimplicit = IS_RAISE_OPCODE(instr->opcode) ? depth - popped : 0;
    // The values the raise leaves, less those whose one owner a copy
    // already spent (collapse_unwound())
    char unwound[IR_MAX_SLOTS];
    int nleft = 0;
    for (int k = 0; k < nimplicit; k++) {
        int merged = k < IR_MAX_SLOTS && collapse_unwound(t, instr, k);
        if (k < IR_MAX_SLOTS) {
            unwound[k] = (char)merged;
        }
        nleft += !merged;
    }
    // Register style: the outputs below dst pass their inputs through
    int through = 0;
    if (!window) {
        through = _PyIR_CALL_SHAPED(fmt->operands) ? 0
                : fmt->dst == _PyIR_NO_DST ? pushed : popped + fmt->dst;
        if (through < 0 || through > pushed) {
            PyErr_Format(PyExc_SystemError, "bad format for opcode %d",
                         instr->opcode);
            return ERROR;
        }
    }
    // The outputs first: a fresh one records the op about to be appended
    int base = depth - popped;
    int defs[4];
    int *out = pushed <= 4 ? defs : PyMem_Malloc(pushed * sizeof(int));
    if (out == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    int err = SUCCESS;
    for (int j = 0; j < pushed && err == SUCCESS; j++) {
        if (j < through) {
            out[j] = t->stack[base + j];
        }
        else if (jumps) {
            out[j] = _PyReg_CANON(cfg, base + j);
        }
        else {
            err = _PyRegTr_Fresh(t, &out[j]);
        }
    }
    _PyRegOp *op;
    if (err == SUCCESS) {
        err = new_op(t, instr, nread + nleft + nrec, pushed, &op);
    }
    if (err == SUCCESS && nrec) {
        err = _PyRegCfg_AddOperands(cfg, nrec, &op->recipe);
    }
    if (err == SUCCESS) {
        int *in = _PyReg_IN(cfg, op);
        for (int k = 0; k < nread; k++) {
            in[k] = t->stack[depth - nread + k];
        }
        int left = 0;
        for (int k = 0; k < nimplicit; k++) {
            if (k >= IR_MAX_SLOTS || !unwound[k]) {
                in[nread + left++] = t->stack[k];
            }
        }
        nimplicit = left;
        // A window's outputs go from its base: where its first input is,
        // unless allocation stages its inputs
        int wbase = window && nread > 0 ? natural_pos(t, in[0]) : -1;
        op->nread = (int16_t)nread;
        op->npop = (int16_t)popped;
        op->nimplicit = (int16_t)nimplicit;
        memcpy(_PyReg_OUT(cfg, op), out, pushed * sizeof(int));
        note_op(t, op);
        // The recipe's identities live until the op, which takes none
        for (int q = 0; q < nrec; q++) {
            in[nread + nimplicit + q] = t->rec[2 * q];
            cfg->operands[op->recipe + q] = t->rec[2 * q + 1];
        }
        op->nimplicit = (int16_t)(nimplicit + nrec);
        op->nrecipe = (int16_t)nrec;
        for (int j = 0; wbase >= 0 && j < pushed; j++) {
            if (out[j] >= _PyReg_FRESH0(cfg)) {
                t->natural[out[j] - _PyReg_FRESH0(cfg)] = wbase + j;
            }
        }
        t->sp = base;
        for (int j = 0; j < pushed && err == SUCCESS; j++) {
            err = _PyRegTr_Push(t, out[j]);
        }
    }
    if (out != defs) {
        PyMem_Free(out);
    }
    return err;
}

static int
stack_underflow(const _PyRegOp *src)
{
    PyErr_Format(PyExc_SystemError, "stack underflow at line %d",
                 src->loc.lineno);
    return ERROR;
}

/* COPY n pushes a second use of the value n - 1 below the top.  One owner
 * covers both uses where that is proved; a second owner exists only where
 * two consumers each spend the value.  The rule takes the first of these
 * that applies:
 *
 *   1. The value is a read of local x on the COPY's line, and no op since
 *      that read may change x (a write to it, or code that may rebind it
 *      through frame.f_locals, or a line start, where a trace function
 *      may).  The copy is a second read of x, and x owns the
 *      value for both uses.  CopyProp folds that read into its reader's
 *      field, which borrows x in place, under the same proof carried on to
 *      the reader; where it cannot, the read stays a LocalRead word.
 *   2. The value is a constant that an op of this region loaded.  The copy
 *      is the same Const op again, which yields the same object.
 *   3. Otherwise a Move.Copy into a fresh register: the second owner.  It
 *      is provisional.  Where one owner turns out to cover both uses (the
 *      copy is spent, and the original is left to the unwinder at a raise
 *      with nothing reading it in between), collapse_unwound() merges the
 *      two and the Move.Copy goes.
 *
 * In every case the stack never holds one identity at two positions, so
 * edges, recipes, windows and retirements see each owner once. */

static int read_local(_PyRegTr *t, const _PyRegOp *src, int local,
                      int handler);

/* Whether op may change the binding of local x: write, clear or vacate it,
 * or run code that may rebind it through frame.f_locals (the rule of
 * CopyProp and of check_folded_load()) */
static int
may_change_local(const _PyRegOp *op, int x)
{
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
    int role = _PyIR_OpargRole(fmt);
    return _PyRegOp_WritesLocal(op, x)
        || (op->oparg == x
            && (op->opcode == ROP_HANDLER_SaveLocal_Take
                || role == _PyIR_ROLE_LOCAL_OUT
                || role == _PyIR_ROLE_LOCAL_CLEAR
                || role == _PyIR_ROLE_LOCAL_MOVE))
        || _PyIR_MayRunPython(fmt, op->dst >= 0);
}

/* The local whose binding identity v still is when src runs: v was read
 * from it in this region and nothing since may have changed it; else -1.
 * Like CopyProp, it also asks that no line starts in between, where a
 * trace function could rebind the local through frame.f_locals. */
static int
stable_local(const _PyRegTr *t, int v, const _PyRegOp *src)
{
    const _PyRegCfg *cfg = t->cfg;
    if (v < _PyReg_FRESH0(cfg)) {
        return -1;
    }
    int p = t->def[v - _PyReg_FRESH0(cfg)];
    if (p < t->region_op || p >= t->nops) {
        return -1;
    }
    const _PyRegOp *read = &t->ops[p];
    if ((read->opcode != ROP_HANDLER_LocalRead_Bound &&
         read->opcode != ROP_HANDLER_LocalRead_Checked) ||
        read->nout != 1 || _PyReg_OUT(cfg, read)[0] != v)
    {
        return -1;
    }
    int x = read->oparg;
    if (src->loc.lineno != read->loc.lineno) {
        return -1;
    }
    for (int i = p + 1; i < t->nops; i++) {
        const _PyRegOp *op = &t->ops[i];
        if (op->opcode != _PyReg_DEAD &&
            (op->loc.lineno != read->loc.lineno || may_change_local(op, x)))
        {
            return -1;
        }
    }
    return x;
}

/* The Const op of this region that defined identity v, or NULL */
static const _PyRegOp *
const_producer(const _PyRegTr *t, int v)
{
    const _PyRegCfg *cfg = t->cfg;
    if (v < _PyReg_FRESH0(cfg)) {
        return NULL;
    }
    int p = t->def[v - _PyReg_FRESH0(cfg)];
    if (p < t->region_op || p >= t->nops) {
        return NULL;
    }
    const _PyRegOp *op = &t->ops[p];
    switch (op->opcode) {
        case ROP_HANDLER_Const_Pool:
        case ROP_HANDLER_Const_Small:
        case ROP_HANDLER_Const_Common:
            break;
        default:
            return NULL;
    }
    return op->nout == 1 && _PyReg_OUT(cfg, op)[0] == v ? op : NULL;
}

static int
rule_copy(_PyRegTr *t, const _PyRegOp *src)
{
    int n = src->oparg;
    if (n < 1 || n > t->sp) {
        return stack_underflow(src);
    }
    int v = t->stack[t->sp - n];
    int x = stable_local(t, v, src);
    if (x >= 0) {
        return read_local(t, src, x, ROP_SELECT_LocalRead_bound);
    }
    const _PyRegOp *k = const_producer(t, v);
    if (k != NULL) {
        int opcode = k->opcode, oparg = k->oparg;
        int dst;
        _PyRegOp *op;
        RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
        RETURN_IF_ERROR(_PyRegTr_Emit(t, src, opcode, NULL, 0, &dst, 1, &op));
        op->oparg = oparg;
        return _PyRegTr_Push(t, dst);
    }
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_SELECT_Move_copy,
                                  &v, 1, &dst, 1, &op));
    op->npop = 0;
    op->oparg = 0;
    RETURN_IF_ERROR(grow((void **)&t->copies, &t->acopies, t->ncopies + 1,
                         sizeof(int)));
    t->copies[t->ncopies++] = (int)(op - t->ops);
    return _PyRegTr_Push(t, dst);
}

/* NOP runs nothing.  It stays a source-only op: the assembler gives it a
 * position record and its line event, and no word. */
static int
rule_nop(_PyRegTr *t, const _PyRegOp *src)
{
    return _PyRegTr_Emit(t, src, NOP, NULL, 0, NULL, 0, NULL);
}

/* SWAP n exchanges the identities at the top and n - 1 below it.  No word
 * runs; a NOP keeps its origin for the source map and line events. */
static int
rule_swap(_PyRegTr *t, const _PyRegOp *src)
{
    int n = src->oparg;
    if (n < 2 || n > t->sp) {
        PyErr_Format(PyExc_SystemError, "stack underflow at line %d",
                     src->loc.lineno);
        return ERROR;
    }
    int top = t->stack[t->sp - 1];
    t->stack[t->sp - 1] = t->stack[t->sp - n];
    t->stack[t->sp - n] = top;
    t->permuted = 1;
    return _PyRegTr_Emit(t, src, NOP, NULL, 0, NULL, 0, NULL);
}

/* POP_TOP removes a use of the value on top and owes its release.  It
 * runs no instruction of its own: the rule discharges the obligation at
 * once, the first way that applies.
 *
 *   1. Nothing to release.  The producer only makes the value (a local
 *      read, a constant, a second owner, a pure op over locals), takes no
 *      temporary, and nothing else read the value: the producer goes too.
 *      It must share the POP_TOP's line and handler, so no line event goes.
 *   2. The producer absorbs it: an op that may discard its result
 *      (_PyIR_DISCARDS), which nothing else read, writes it nowhere
 *      (_PyReg_DISCARD).
 *   3. A retirement: Move.Retire of the value, which closes it and may run
 *      Python.  A retirement right after another on the same line and
 *      under the same handler joins it: one word closes both, in pop
 *      order.  An edge's Move set that follows takes it in (bind_edge()).
 *
 * The obligation never waits past the POP_TOP's own op, so every value is
 * released where it was, or in the next word with no op between: a raise,
 * a call, a yield or a join never sees it pending.  A POP_TOP that left no
 * word of its own stays a source-only op, for its line and offset. */

/* Whether the op only makes its one value, so that dropping both is
 * unobservable */
static int
only_makes_value(const _PyRegCfg *cfg, const _PyRegOp *p)
{
    switch (p->opcode) {
        case ROP_HANDLER_LocalRead_Bound:
        case ROP_HANDLER_Const_Pool:
        case ROP_HANDLER_Const_Small:
        case ROP_HANDLER_Const_Common:
        case ROP_HANDLER_Move_Copy:
            break;
        default: {
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(p->opcode, p->oparg);
            if (!(fmt->effects & _PyIR_PURE) ||
                (fmt->effects & (_PyIR_RAISES | _PyIR_ESCAPES)))
            {
                return 0;
            }
        }
    }
    // It takes no temporary: its taken inputs are locals, which it borrows
    const int *in = _PyReg_IN(cfg, p);
    for (int k = p->nread - p->npop; k < p->nread; k++) {
        if (in[k] >= cfg->nlocalsplus) {
            return 0;
        }
    }
    return 1;
}

/* The op in this region that made v, its only output, if no later op reads
 * v; else NULL */
static _PyRegOp *
sole_producer(_PyRegTr *t, int v)
{
    _PyRegCfg *cfg = t->cfg;
    if (v < _PyReg_FRESH0(cfg)) {
        return NULL;
    }
    int p = t->def[v - _PyReg_FRESH0(cfg)];
    if (p < t->region_op || p >= t->nops) {
        return NULL;
    }
    _PyRegOp *prod = &t->ops[p];
    if (prod->opcode == _PyReg_DEAD || prod->nout != 1 ||
        _PyReg_OUT(cfg, prod)[0] != v || prod->dst >= 0 ||
        (prod->flags & (_PyReg_MOVES | _PyReg_DISCARD)))
    {
        return NULL;
    }
    for (int i = p + 1; i < t->nops; i++) {
        const _PyRegOp *op = &t->ops[i];
        const int *in = _PyReg_IN(cfg, op);
        for (int k = 0; k < op->nread + op->nimplicit; k++) {
            if (in[k] == v) {
                return NULL;
            }
        }
    }
    // v is on top; no position below holds it too
    for (int k = 0; k < t->sp - 1; k++) {
        if (t->stack[k] == v) {
            return NULL;
        }
    }
    return prod;
}

/* The retirement that the op being translated may join: the last op of the
 * block, past source-only ops, if it is a Move.Retire on src's line under
 * src's handler */
static _PyRegOp *
joinable_retire(_PyRegTr *t, const _PyRegOp *src)
{
    for (int i = t->nops - 1; i >= t->block_op; i--) {
        _PyRegOp *op = &t->ops[i];
        if (op->loc.lineno != src->loc.lineno || op->except != src->except) {
            return NULL;
        }
        if (op->opcode == ROP_SELECT_Move_retire) {
            return op->nout == 0 ? op : NULL;
        }
        if (op->opcode != NOP || op->nread != 0 || op->nout != 0) {
            return NULL;
        }
    }
    return NULL;
}

/* Retirement op also closes v, after its other sources */
static int
retire_also(_PyRegTr *t, _PyRegOp *op, int v)
{
    _PyRegCfg *cfg = t->cfg;
    int n = op->nread + op->nimplicit;
    int in;
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, n + 1, &in));
    int *old = &cfg->operands[op->in];
    int *new = &cfg->operands[in];
    memcpy(new, old, op->nread * sizeof(int));
    new[op->nread] = v;
    memcpy(new + op->nread + 1, old + op->nread, op->nimplicit * sizeof(int));
    op->in = in;
    op->nread++;
    op->npop++;
    note_op(t, op);
    return SUCCESS;
}

/* The module that the last IMPORT_FROM of a statement reads is popped
 * after it (`from m import a, b` and `import p.q as r`).  If the last op of
 * this block that reads v is an ImportFrom.From of v, the read spends the
 * module and no retirement follows.  No op between may need v at its
 * handler (v at or above its base), and the read may not carry v in its own
 * recipe, so that v is dead when the read has run. */
static int
spend_import_module(_PyRegTr *t, int v)
{
    _PyRegCfg *cfg = t->cfg;
    int pos = t->sp - 1;
    for (int i = t->nops - 1; i >= t->block_op; i--) {
        _PyRegOp *op = &t->ops[i];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        const int *in = _PyReg_IN(cfg, op);
        int reads = 0;
        for (int k = 0; k < op->nread + op->nimplicit; k++) {
            reads |= in[k] == v;
        }
        if (!reads) {
            if (op->except >= 0 && handler_base(cfg, op->except) > pos) {
                return 0;
            }
            continue;
        }
        if (op->opcode != ROP_HANDLER_ImportFrom_From || op->nread != 1 ||
            in[0] != v || op->nimplicit != 0)
        {
            return 0;
        }
        if (op->except >= 0 && handler_base(cfg, op->except) > pos) {
            return 0;
        }
        op->opcode = ROP_HANDLER_ImportFrom_FromLast;
        op->npop = 1;
        return 1;
    }
    return 0;
}

static int
rule_pop_top(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int v = t->stack[t->sp - 1];
    _PyRegOp *prod = sole_producer(t, v);
    _PyRegOp *retire = NULL, *dead = NULL;
    int handler = NOP;
    if (prod != NULL && prod->loc.lineno == src->loc.lineno &&
        prod->except == src->except && only_makes_value(t->cfg, prod))
    {
        prod->opcode = _PyReg_DEAD;
        dead = prod;
    }
    else if (prod != NULL &&
             (_PyRegCfg_FormatOf(prod->opcode, prod->oparg)->flags
              & _PyIR_DISCARDS))
    {
        prod->flags |= _PyReg_DISCARD;
    }
    else if (prod == NULL && spend_import_module(t, v)) {
        // The read of the module spent it: only the origin remains
    }
    else if ((retire = joinable_retire(t, src)) != NULL) {
        RETURN_IF_ERROR(retire_also(t, retire, v));
    }
    else {
        handler = ROP_SELECT_Move_retire;
    }
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &v,
                                  handler == NOP ? 0 : 1, NULL, 0, &op));
    if (dead != NULL) {
        // The source-only op stands for both, with the producer's span
        op->loc = dead->loc;
        op->flags |= dead->flags & _PyReg_SOURCE_ORIGIN;
    }
    return _PyRegTr_Pop(t, 1, NULL);
}

/* LOAD_ATTR is the GetAttr family, or GetMethod when oparg's low bit asks
 * for a method.  The rule names its operands: the receiver is the one use;
 * GetAttr defines dst, GetMethod the callable and then self, pushed in the
 * order the stack held them, so the legacy CALL finds them as its window.
 * Every site starts under the family's Generic arm, which adapts it. */
static int
rule_load_attr(_PyRegTr *t, const _PyRegOp *src)
{
    // The family's name operand is the index into co_names; the stack op's
    // oparg carries the method flag below it
    int name = src->oparg >> 1;
    _PyRegOp *op;
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int obj = t->stack[t->sp - 1];
    if (!(src->oparg & 1)) {
        int dst;
        RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
        RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_GetAttr_Generic,
                                      &obj, 1, &dst, 1, &op));
        op->oparg = name;
        RETURN_IF_ERROR(_PyRegTr_Pop(t, 1, NULL));
        return _PyRegTr_Push(t, dst);
    }
    int defs[2];
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &defs[0]));
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &defs[1]));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_GetMethod_Generic,
                                  &obj, 1, defs, 2, &op));
    op->oparg = name;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 1, NULL));
    RETURN_IF_ERROR(_PyRegTr_Push(t, defs[0]));
    return _PyRegTr_Push(t, defs[1]);
}

/* LOAD_GLOBAL is the GlobalRead family (Python/register_ops/names.rop).  The
 * name is oparg >> 1.  The low bit asks for the NULL that a call takes as its
 * absent self; it is not a value of the family, so the rule pushes a PUSH_NULL
 * after the result, as the stack held them.  ElideNull gives it no word. */
static int
rule_load_global(_PyRegTr *t, const _PyRegOp *src)
{
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_GlobalRead_Generic,
                                  NULL, 0, &dst, 1, &op));
    op->oparg = src->oparg >> 1;
    RETURN_IF_ERROR(_PyRegTr_Push(t, dst));
    if (!(src->oparg & 1)) {
        return SUCCESS;
    }
    _PyRegOp null = *src;
    null.opcode = PUSH_NULL;
    null.oparg = 0;
    return _PyRegTr_Legacy(t, &null);
}

/* BINARY_OP.  The selector routes to a family; this is the one place that
 * does.  Addition is the Add family (Python/register_ops/add.rop) and
 * subscription the GetItem family (items.rop): the register instruction is
 * the family's Generic arm, with the operands in their declared order (x and
 * y, or obj and key) in and dst out.  The word starts on that arm, which
 * installs the arm the operands call for.  The other operations are
 * unported (-1). */
static int
binary_op_handler(int oparg)
{
    switch (oparg) {
        case NB_ADD:
            return ROP_HANDLER_Add_Generic;
        case NB_SUBSCR:
            return ROP_HANDLER_GetItem_Generic;
        default:
            return -1;
    }
}

static int
rule_binary_op(_PyRegTr *t, const _PyRegOp *src)
{
    int handler = binary_op_handler(src->oparg);
    if (handler < 0) {
        return _PyRegTr_Legacy(t, src);
    }
    if (t->sp < 2) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler,
                                  &t->stack[t->sp - 2], 2, &dst, 1, NULL));
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 2, NULL));
    return _PyRegTr_Push(t, dst);
}

/* The item and slice stack ops (Python/register_ops/items.rop).  The uses
 * are named in the family's operand order, which is not always the stack's:
 * the stored value is below the container.  Each word starts on the
 * family's Generic arm. */
static int
item_op(_PyRegTr *t, const _PyRegOp *src, int handler, const int *depth,
        int nuses, int has_dst)
{
    if (t->sp < nuses) {
        return stack_underflow(src);
    }
    int uses[4];
    for (int i = 0; i < nuses; i++) {
        uses[i] = t->stack[t->sp - depth[i]];
    }
    int dst = -1;
    if (has_dst) {
        RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    }
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, uses, nuses,
                                  has_dst ? &dst : NULL, has_dst, NULL));
    RETURN_IF_ERROR(_PyRegTr_Pop(t, nuses, NULL));
    return has_dst ? _PyRegTr_Push(t, dst) : SUCCESS;
}

/* STORE_SUBSCR: value, container, key (top) */
static int
rule_store_subscr(_PyRegTr *t, const _PyRegOp *src)
{
    static const int depth[] = {2, 1, 3};
    return item_op(t, src, ROP_HANDLER_SetItem_Generic, depth, 3, 0);
}

/* DELETE_SUBSCR: container, key (top) */
static int
rule_delete_subscr(_PyRegTr *t, const _PyRegOp *src)
{
    static const int depth[] = {2, 1};
    return item_op(t, src, ROP_HANDLER_DelItem_Generic, depth, 2, 0);
}

/* BINARY_SLICE: container, start, stop (top) */
static int
rule_binary_slice(_PyRegTr *t, const _PyRegOp *src)
{
    static const int depth[] = {3, 2, 1};
    return item_op(t, src, ROP_HANDLER_SliceRead_Generic, depth, 3, 1);
}

/* STORE_SLICE: value, container, start, stop (top) */
static int
rule_store_slice(_PyRegTr *t, const _PyRegOp *src)
{
    static const int depth[] = {3, 2, 1, 4};
    return item_op(t, src, ROP_HANDLER_SliceWrite_Generic, depth, 4, 0);
}

/* BUILD_SLICE: two or three parts, as the span of the family */
static int
rule_build_slice(_PyRegTr *t, const _PyRegOp *src)
{
    int n = src->oparg;
    if (t->sp < n) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_BuildSlice_Generic,
                                  &t->stack[t->sp - n], n, &dst, 1, NULL));
    RETURN_IF_ERROR(_PyRegTr_Pop(t, n, NULL));
    return _PyRegTr_Push(t, dst);
}

/* TO_BOOL and UNARY_NOT: the Truth family (Python/register_ops/truth.rop),
 * whose register instruction starts on Truth.Generic with the operand in,
 * dst out and the sense in the immediate; the word installs the arm the
 * operand calls for. */
static int
rule_truth(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_Truth_Generic,
                                  &t->stack[t->sp - 1], 1, &dst, 1, &op));
    op->oparg = src->opcode == UNARY_NOT;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 1, NULL));
    return _PyRegTr_Push(t, dst);
}

/* COMPARE_OP, IS_OP and CONTAINS_OP: the Compare, Identity and Contains
 * families (Python/register_ops/tests.rop).  The register instruction of a
 * compare starts on Compare.Generic, which installs the arm the operands
 * call for; the immediate is the relation and whether the result is a bool.
 * IS_OP and CONTAINS_OP carry their inversion (is not, not in) as the
 * sense. */
static int
rule_test(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 2) {
        return stack_underflow(src);
    }
    int handler, imm;
    switch (src->opcode) {
        case COMPARE_OP:
            handler = ROP_HANDLER_Compare_Generic;
            imm = ROP_PACK_Compare_relation(src->oparg >> 5)
                  | ROP_PACK_Compare_result(src->oparg & 16 ? ROP_RESULT_TRUTH
                                                             : ROP_RESULT_RICH);
            break;
        case IS_OP:
            handler = ROP_HANDLER_Identity_Test, imm = src->oparg;
            break;
        default:
            assert(src->opcode == CONTAINS_OP);
            handler = ROP_HANDLER_Contains_Generic, imm = src->oparg;
            break;
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &t->stack[t->sp - 2], 2,
                                  &dst, 1, &op));
    op->oparg = imm;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 2, NULL));
    return _PyRegTr_Push(t, dst);
}

/* The conditional jumps.  A branch reads its operand and, on the jump
 * edge, binds the target's parameters as the stack op did, so the rule is
 * the stack op's with the register instruction's handler and immediate:
 * the mode of BranchTruth (the jump is taken when the test is true or
 * false), the sense of BranchNone (the jump is taken when the operand is
 * None). */
static int
rule_branch(_PyRegTr *t, const _PyRegOp *src)
{
    int handler, imm;
    switch (src->opcode) {
        case POP_JUMP_IF_FALSE:
            handler = ROP_HANDLER_BranchTruth_Generic, imm = BRANCH_IF_FALSE;
            break;
        case POP_JUMP_IF_TRUE:
            handler = ROP_HANDLER_BranchTruth_Generic, imm = BRANCH_IF_TRUE;
            break;
        case POP_JUMP_IF_NONE:
            handler = ROP_HANDLER_BranchNone_Test, imm = 1;
            break;
        default:
            assert(src->opcode == POP_JUMP_IF_NOT_NONE);
            handler = ROP_HANDLER_BranchNone_Test, imm = 0;
            break;
    }
    RETURN_IF_ERROR(_PyRegTr_Legacy(t, src));
    _PyRegOp *op = &t->ops[t->nops - 1];
    assert(op->opcode == src->opcode);
    op->opcode = handler;
    op->oparg = imm;
    return SUCCESS;
}

/* The jumps: the Jump family (Python/register_ops/truth.rop).  A backedge
 * that polls is the arm that the baseline JIT's setting selects: the one
 * that is also its warm-up event while the JIT is on. */
static int
rule_jump(_PyRegTr *t, const _PyRegOp *src)
{
    int interrupt = src->opcode == JUMP_BACKWARD ? ROP_SELECT_INTERRUPTIBLE
                                                 : ROP_SELECT_UNINTERRUPTED;
    int jit = _PyInterpreterState_GET()->ceval.jit != _PyJIT_OFF
                  ? ROP_SELECT_JIT_ON : ROP_SELECT_JIT_OFF;
    int handler = _PyRop_SelectJump(interrupt, jit);
    assert(handler >= 0);
    RETURN_IF_ERROR(_PyRegTr_Legacy(t, src));
    _PyRegOp *op = &t->ops[t->nops - 1];
    assert(op->opcode == src->opcode);
    op->opcode = handler;
    return SUCCESS;
}

/* Whether identity v may hold an absent binding (a Maybe value): one that
 * SaveLocal took, or a parameter of a position where one has been.  The
 * positions over-approximate the edges, which is safe: a Restore of a
 * present value is a Store. */
static int
maybe_absent(const _PyRegTr *t, int v)
{
    const _PyRegCfg *cfg = t->cfg;
    if (v >= _PyReg_FRESH0(cfg)) {
        return t->maybe[v - _PyReg_FRESH0(cfg)];
    }
    return _PyReg_IS_CANON(cfg, v) && t->maybe_pos[v - cfg->nlocalsplus];
}

/* Locals are homes: a read names the local as its source and a store as
 * its destination (Python/register_ops/values.rop). */

/* LOAD_FAST, LOAD_FAST_BORROW and LOAD_FAST_CHECK: the local's binding
 * into a fresh temporary.  CopyProp later folds a read into the field of
 * its one reader where nothing between can change the local. */
static int
read_local(_PyRegTr *t, const _PyRegOp *src, int local, int handler)
{
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &local, 1, &dst, 1, &op));
    op->oparg = local;
    return _PyRegTr_Push(t, dst);
}

/* STORE_FAST: the value into the local, which releases its old binding.
 * A binding that SaveLocal took may be absent: Restore. */
static int
write_local(_PyRegTr *t, const _PyRegOp *src, int local)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int value = t->stack[t->sp - 1];
    _PyRegOp *op;
    int handler = maybe_absent(t, value) ? ROP_SELECT_LocalWrite_restore
                                         : ROP_SELECT_LocalWrite_store;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &value, 1, &local, 1, &op));
    op->oparg = local;
    op->dst = local;
    t->sp--;
    return SUCCESS;
}

static int
rule_load_fast(_PyRegTr *t, const _PyRegOp *src)
{
    return read_local(t, src, src->oparg, ROP_SELECT_LocalRead_bound);
}

static int
rule_load_fast_check(_PyRegTr *t, const _PyRegOp *src)
{
    return read_local(t, src, src->oparg, ROP_SELECT_LocalRead_checked);
}

static int
rule_load_fast_load_fast(_PyRegTr *t, const _PyRegOp *src)
{
    RETURN_IF_ERROR(read_local(t, src, src->oparg >> 4,
                               ROP_SELECT_LocalRead_bound));
    return read_local(t, src, src->oparg & 15, ROP_SELECT_LocalRead_bound);
}

static int
rule_store_fast(_PyRegTr *t, const _PyRegOp *src)
{
    return write_local(t, src, src->oparg);
}

static int
rule_store_fast_load_fast(_PyRegTr *t, const _PyRegOp *src)
{
    RETURN_IF_ERROR(write_local(t, src, src->oparg >> 4));
    return read_local(t, src, src->oparg & 15, ROP_SELECT_LocalRead_bound);
}

static int
rule_store_fast_store_fast(_PyRegTr *t, const _PyRegOp *src)
{
    RETURN_IF_ERROR(write_local(t, src, src->oparg >> 4));
    return write_local(t, src, src->oparg & 15);
}

/* LOAD_FAST_AND_CLEAR: the comprehension takes the local's binding, which
 * may be absent, and leaves the local vacant */
static int
rule_load_fast_and_clear(_PyRegTr *t, const _PyRegOp *src)
{
    int dst, local = src->oparg;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_SaveLocal_Take,
                                  &local, 1, &dst, 1, &op));
    op->oparg = local;
    t->maybe[dst - _PyReg_FRESH0(t->cfg)] = 1;
    return _PyRegTr_Push(t, dst);
}

static int
rule_delete_fast(_PyRegTr *t, const _PyRegOp *src)
{
    int local = src->oparg;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_LocalDelete_Checked,
                                  NULL, 0, &local, 1, &op));
    op->oparg = local;
    op->dst = local;
    return SUCCESS;
}

/* Cells and free variables (Python/register_ops/cells.rop).  The home of a
 * cell is a slot, named by the oparg; the word reads the cell through it
 * and never writes it, except MAKE_CELL, which wraps the local's binding. */

/* MAKE_CELL: the local's binding into a new cell in the same home */
static int
rule_make_cell(_PyRegTr *t, const _PyRegOp *src)
{
    int local = src->oparg;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CellMake_Wrap,
                                  NULL, 0, &local, 1, &op));
    op->oparg = local;
    op->dst = local;
    return SUCCESS;
}

/* LOAD_DEREF: the cell's contents into a fresh temporary */
static int
rule_load_deref(_PyRegTr *t, const _PyRegOp *src)
{
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CellRead_Get,
                                  NULL, 0, &dst, 1, &op));
    op->oparg = src->oparg;
    return _PyRegTr_Push(t, dst);
}

/* LOAD_FROM_DICT_OR_DEREF: the mapping is the one use */
static int
rule_load_from_dict_or_deref(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CellMapRead_Lookup,
                                  &t->stack[t->sp - 1], 1, &dst, 1, &op));
    op->oparg = src->oparg;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 1, NULL));
    return _PyRegTr_Push(t, dst);
}

/* STORE_DEREF: the value into the cell, whose old contents are closed */
static int
rule_store_deref(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CellStore_Set,
                                  &t->stack[t->sp - 1], 1, NULL, 0, &op));
    op->oparg = src->oparg;
    return _PyRegTr_Pop(t, 1, NULL);
}

static int
rule_delete_deref(_PyRegTr *t, const _PyRegOp *src)
{
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CellDelete_Unbind,
                                  NULL, 0, NULL, 0, &op));
    op->oparg = src->oparg;
    return SUCCESS;
}

/* COPY_FREE_VARS: frame entry copies the closure's cells
 * (_PyFrame_CopyFreeVars), so the stack op has no word and no origin: it is
 * the code's first op and has no location */
static int
rule_copy_free_vars(_PyRegTr *t, const _PyRegOp *src)
{
    (void)t;
    (void)src;
    return SUCCESS;
}

/* LOAD_CONST, LOAD_SMALL_INT and LOAD_COMMON_CONSTANT: the Const arm of
 * the kind, its index in the oparg.  ConstSlots later gives most constants
 * hidden homes. */
static int
load_const(_PyRegTr *t, const _PyRegOp *src, int handler)
{
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, NULL, 0, &dst, 1, NULL));
    return _PyRegTr_Push(t, dst);
}

static int
rule_load_const(_PyRegTr *t, const _PyRegOp *src)
{
    return load_const(t, src, ROP_SELECT_Const_pool);
}

static int
rule_load_small_int(_PyRegTr *t, const _PyRegOp *src)
{
    return load_const(t, src, ROP_SELECT_Const_small);
}

static int
rule_load_common_constant(_PyRegTr *t, const _PyRegOp *src)
{
    return load_const(t, src, ROP_SELECT_Const_common);
}

/* CALL is the Call family (Python/register_ops/calls.rop): fn, then the
 * arguments, self first, in; dst out.  The word starts on Call.Generic,
 * which installs the arm the call calls for. */
static int
rule_call(_PyRegTr *t, const _PyRegOp *src)
{
    int nuses = 2 + src->oparg;
    if (nuses > t->sp) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_Call_Generic,
                                  &t->stack[t->sp - nuses], nuses, &dst, 1,
                                  NULL));
    t->sp -= nuses;
    return _PyRegTr_Push(t, dst);
}

/* The pseudo op that PrepareKwargs puts before a read of a lazy **kwargs
 * local is MaterializeKwargs: the home it may write is the local's. */
static int
rule_materialize_kwargs(_PyRegTr *t, const _PyRegOp *src)
{
    int local = src->oparg;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_MaterializeKwargs_Pending,
                                  NULL, 0, &local, 1, &op));
    op->oparg = local;
    op->dst = local;
    return SUCCESS;
}

/* CALL_KW is CallKw (Python/register_ops/calls.rop): fn, self, the arguments
 * and the constant tuple of the names (which pass f0 reads off the load
 * before it) in; dst out. */
static int
rule_call_kw(_PyRegTr *t, const _PyRegOp *src)
{
    int nuses = 3 + src->oparg;
    if (nuses > t->sp) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CallKw_Generic,
                                  &t->stack[t->sp - nuses], nuses, &dst, 1,
                                  NULL));
    t->sp -= nuses;
    return _PyRegTr_Push(t, dst);
}

/* CALL_FUNCTION_EX is CallEx: fn, the reserved NULL, the positional
 * arguments and the kwargs (virtual NULL for none) in; dst out. */
static int
rule_call_function_ex(_PyRegTr *t, const _PyRegOp *src)
{
    if (4 > t->sp) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_CallEx_Generic,
                                  &t->stack[t->sp - 4], 4, &dst, 1, NULL));
    t->sp -= 4;
    return _PyRegTr_Push(t, dst);
}

/* MAKE_FUNCTION is Function.Make: the code in, the function out.  The
 * SET_FUNCTION_ATTRIBUTEs after it add their attributes to the word
 * (rule_set_function_attribute). */
static int
rule_make_function(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_Function_Make,
                                  &t->stack[t->sp - 1], 1, &dst, 1, &op));
    // The attributes of the SET_FUNCTION_ATTRIBUTEs that follow join the word
    op->oparg = 0;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 1, NULL));
    return _PyRegTr_Push(t, dst);
}

/* SET_FUNCTION_ATTRIBUTE adds an attribute to the Function.Make that made
 * the function: the compiler computes every attribute before the code, so
 * the word reads them all.  The attribute (below the function) joins the
 * word's span at the place of its bit, and the stack op stays as a
 * source-only op, for its line and offset.  The oparg is the compiler's
 * flag, 1 << k for bit k of the word's `present`. */
static int
rule_set_function_attribute(_PyRegTr *t, const _PyRegOp *src)
{
    _PyRegCfg *cfg = t->cfg;
    int flag = src->oparg;
    if (t->sp < 2) {
        return stack_underflow(src);
    }
    if (flag <= 0 || flag > MAKE_FUNCTION_ANNOTATE || (flag & (flag - 1))) {
        PyErr_SetString(PyExc_SystemError, "bad SET_FUNCTION_ATTRIBUTE flag");
        return ERROR;
    }
    int k = _Py_bit_length((unsigned)flag) - 1;
    int attr = t->stack[t->sp - 2];
    int func = t->stack[t->sp - 1];
    // The word that made the function: the last of the block, past source-only ops
    _PyRegOp *make = NULL;
    for (int i = t->nops - 1; i >= t->block_op; i--) {
        _PyRegOp *op = &t->ops[i];
        if (op->opcode == NOP && op->nread == 0 && op->nout == 0) {
            continue;
        }
        make = op;
        break;
    }
    if (make == NULL || make->opcode != ROP_HANDLER_Function_Make ||
        _PyReg_OUT(cfg, make)[0] != func || (make->oparg & flag))
    {
        PyErr_Format(PyExc_SystemError,
                     "SET_FUNCTION_ATTRIBUTE at line %d does not follow its MAKE_FUNCTION",
                     src->loc.lineno);
        return ERROR;
    }
    int n = make->nread + make->nimplicit;
    int in;
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, n + 1, &in));
    const int *old = &cfg->operands[make->in];
    int *new = &cfg->operands[in];
    // The code, the attributes of lower bits, this one, the rest
    int at = 1 + _Py_popcount32((unsigned)(make->oparg & (flag - 1)));
    memcpy(new, old, at * sizeof(int));
    new[at] = attr;
    memcpy(new + at + 1, old + at, (n - at) * sizeof(int));
    make->in = in;
    make->nread++;
    make->npop++;
    make->oparg |= flag;
    note_op(t, make);
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, NOP, NULL, 0, NULL, 0, NULL));
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 2, NULL));
    return _PyRegTr_Push(t, func);
}

/* CALL_INTRINSIC_1 is the Intrinsic family and CALL_INTRINSIC_2 IntrinsicPair
 * (functions.rop).  The compiler's number is the member of IntrinsicId: n - 1
 * for the unary table, n + 11 for the binary one, whose escapes (comparisons,
 * GETATTR, SETATTR) follow its seven functions.  The number also picks the
 * arm. */
static int
rule_call_intrinsic(_PyRegTr *t, const _PyRegOp *src)
{
    int unary = src->opcode == CALL_INTRINSIC_1;
    int n = src->oparg;
    int nuses = unary ? 1 : 2;
    int id, handler;
    if (unary) {
        if (n < 1 || n > MAX_INTRINSIC_1) {
            PyErr_SetString(PyExc_SystemError, "invalid intrinsic function");
            return ERROR;
        }
        id = n - 1;
        handler = _PyRop_SelectIntrinsic(
            n == INTRINSIC_IMPORT_STAR ? ROP_SELECT_IMPORT_STAR
            : n == INTRINSIC_STOPITERATION_ERROR ? ROP_SELECT_STOPITERATION
            : ROP_SELECT_UNARY);
    }
    else {
        if (n < 1 || n > MAX_INTRINSIC_2) {
            PyErr_SetString(PyExc_SystemError, "invalid intrinsic function");
            return ERROR;
        }
        // The ids of IntrinsicId follow the unary ones in order; the
        // numbers 8 to 21 are unused
        id = n < INTRINSIC_GETATTR ? n + 11 : n - 3;
        handler = _PyRop_SelectIntrinsicPair(
            n < INTRINSIC_GETATTR ? ROP_SELECT_BINARY : ROP_SELECT_ATTR);
    }
    if (t->sp < nuses) {
        return stack_underflow(src);
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &t->stack[t->sp - nuses],
                                  nuses, &dst, 1, &op));
    op->oparg = id;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, nuses, NULL));
    return _PyRegTr_Push(t, dst);
}

/* RETURN_VALUE is the Return family: value in.  The frame it leaves and
 * whether a scratch slot may still own a value select the arm: a return
 * that finds only its value on the stack leaves none, since each op
 * retires the inputs it takes and the unwinder the stack above a
 * handler's depth. */
static int
rule_return_value(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int handler = _PyRop_SelectReturn(
        t->cfg->generator ? ROP_SELECT_GENERATOR_FRAME
                          : ROP_SELECT_THREAD_FRAME,
        t->sp == 1 ? ROP_SELECT_VACANT_SCRATCH : ROP_SELECT_LIVE_SCRATCH);
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &t->stack[t->sp - 1], 1,
                                  NULL, 0, NULL));
    t->sp--;
    return SUCCESS;
}

/* The namespace and import stack ops (Python/register_ops/namespaces.rop).
 * Each names its namespace statically, so the rule picks the arm. */

static int
read_namespace(_PyRegTr *t, const _PyRegOp *src, int handler, int name)
{
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, NULL, 0, &dst, 1, &op));
    op->oparg = name;
    return _PyRegTr_Push(t, dst);
}

static int
rule_load_name(_PyRegTr *t, const _PyRegOp *src)
{
    return read_namespace(t, src, ROP_SELECT_NamespaceRead_named, src->oparg);
}

static int
rule_load_locals(_PyRegTr *t, const _PyRegOp *src)
{
    return read_namespace(t, src, ROP_SELECT_NamespaceRead_locals_scope, 0);
}

static int
rule_load_build_class(_PyRegTr *t, const _PyRegOp *src)
{
    return read_namespace(t, src, ROP_SELECT_NamespaceRead_build_class_scope, 0);
}

/* LOAD_FROM_DICT_OR_GLOBALS: the mapping is the one use */
static int
rule_load_from_dict_or_globals(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_HANDLER_NamespaceReadFrom_Mapping,
                                  &t->stack[t->sp - 1], 1, &dst, 1, &op));
    op->oparg = src->oparg;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 1, NULL));
    return _PyRegTr_Push(t, dst);
}

/* STORE_NAME and STORE_GLOBAL.  The compiler writes `del name` as PUSH_NULL
 * and the store: the elided PUSH_NULL's register holds NULL, which the
 * word's optional value reads as absent, so the arm deletes. */
static int
store_namespace(_PyRegTr *t, const _PyRegOp *src, int handler)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &t->stack[t->sp - 1], 1,
                                  NULL, 0, &op));
    op->oparg = src->oparg;
    return _PyRegTr_Pop(t, 1, NULL);
}

static int
rule_store_name(_PyRegTr *t, const _PyRegOp *src)
{
    return store_namespace(t, src, ROP_SELECT_NamespaceWrite_name_scope);
}

static int
rule_store_global(_PyRegTr *t, const _PyRegOp *src)
{
    return store_namespace(t, src, ROP_SELECT_NamespaceWrite_global_scope);
}

static int
rule_setup_annotations(_PyRegTr *t, const _PyRegOp *src)
{
    return _PyRegTr_Emit(t, src, ROP_HANDLER_SetupAnnotations_Setup,
                         NULL, 0, NULL, 0, NULL);
}

/* IMPORT_NAME: level and fromlist are the two uses; the oparg's low bits
 * say how the import may be lazy, and the rest is the name. */
static int
rule_import_name(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 2) {
        return stack_underflow(src);
    }
    int handler = (src->oparg & 0x02) ? ROP_SELECT_Import_eager_import
                : (src->oparg & 0x01) ? ROP_SELECT_Import_lazy_import
                                      : ROP_SELECT_Import_policy_import;
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &t->stack[t->sp - 2], 2,
                                  &dst, 1, &op));
    op->oparg = src->oparg >> 2;
    RETURN_IF_ERROR(_PyRegTr_Pop(t, 2, NULL));
    return _PyRegTr_Push(t, dst);
}

/* IMPORT_FROM reads the module and leaves it on the stack; the POP_TOP
 * after the last read turns the word into ImportFrom.FromLast */
static int
rule_import_from(_PyRegTr *t, const _PyRegOp *src)
{
    if (t->sp < 1) {
        return stack_underflow(src);
    }
    int dst;
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, ROP_SELECT_ImportFrom_module_kept,
                                  &t->stack[t->sp - 1], 1, &dst, 1, &op));
    op->npop = 0;
    op->oparg = src->oparg;
    return _PyRegTr_Push(t, dst);
}

/* The display families (Python/register_ops/displays.rop).  A span is the
 * homes of the identities it names, wherever allocation puts them, so no
 * rule here stages a value to make its inputs adjacent. */

/* BUILD_TUPLE, BUILD_LIST, BUILD_SET and BUILD_MAP: the items are the top
 * identities (a map's are its keys and values in order), the container the
 * one output.  The kind is the arm. */
static int
rule_build(_PyRegTr *t, const _PyRegOp *src)
{
    int n = src->opcode == BUILD_MAP ? 2 * src->oparg : src->oparg;
    if (n > t->sp) {
        return stack_underflow(src);
    }
    int handler;
    switch (src->opcode) {
        case BUILD_TUPLE: handler = ROP_SELECT_Build_tuple; break;
        case BUILD_LIST: handler = ROP_SELECT_Build_list; break;
        case BUILD_SET: handler = ROP_SELECT_Build_set; break;
        default: handler = ROP_SELECT_Build_map; break;
    }
    int dst;
    RETURN_IF_ERROR(_PyRegTr_Fresh(t, &dst));
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, &t->stack[t->sp - n], n,
                                  &dst, 1, NULL));
    RETURN_IF_ERROR(_PyRegTr_Pop(t, n, NULL));
    return _PyRegTr_Push(t, dst);
}

/* LIST_APPEND, SET_ADD, MAP_ADD, LIST_EXTEND, SET_UPDATE, DICT_UPDATE and
 * DICT_MERGE: the container lies oparg positions below the top operand (the
 * callable of DICT_MERGE three below the container) and stays; the top
 * operands go.  The instruction names the container's home, so no stack
 * depth is an operand.  The reads are in stack order, the popped ones last. */
static int
rule_insert(_PyRegTr *t, const _PyRegOp *src)
{
    int handler, popped, extra = 0;
    switch (src->opcode) {
        case LIST_APPEND: handler = ROP_SELECT_AddElement_append; popped = 1; break;
        case SET_ADD: handler = ROP_SELECT_AddElement_add; popped = 1; break;
        case MAP_ADD: handler = ROP_HANDLER_AddEntry_Put; popped = 2; break;
        case LIST_EXTEND: handler = ROP_SELECT_UpdateContainer_extend; popped = 1; break;
        case SET_UPDATE: handler = ROP_SELECT_UpdateContainer_union; popped = 1; break;
        case DICT_UPDATE: handler = ROP_SELECT_UpdateContainer_overlay; popped = 1; break;
        default:
            handler = ROP_HANDLER_MergeKeywords_Merge;
            popped = 1;
            extra = 3;
            break;
    }
    int oparg = src->oparg;
    int container = t->sp - popped - oparg;
    if (oparg < 1 || container - extra < 0) {
        return stack_underflow(src);
    }
    int uses[3], n = 0;
    if (extra) {
        uses[n++] = t->stack[container - extra];
    }
    uses[n++] = t->stack[container];
    for (int k = 0; k < popped; k++) {
        uses[n++] = t->stack[t->sp - popped + k];
    }
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, src, handler, uses, n, NULL, 0, &op));
    op->npop = (int16_t)popped;
    return _PyRegTr_Pop(t, popped, NULL);
}

/* The rules, by opcode.  NULL is the legacy rule. */
static const _PyRegRule rules[256] = {
    [BINARY_OP] = rule_binary_op,
    [COMPARE_OP] = rule_test,
    [IS_OP] = rule_test,
    [CONTAINS_OP] = rule_test,
    [TO_BOOL] = rule_truth,
    [UNARY_NOT] = rule_truth,
    [POP_JUMP_IF_FALSE] = rule_branch,
    [POP_JUMP_IF_TRUE] = rule_branch,
    [POP_JUMP_IF_NONE] = rule_branch,
    [POP_JUMP_IF_NOT_NONE] = rule_branch,
    [JUMP_FORWARD] = rule_jump,
    [JUMP_BACKWARD] = rule_jump,
    [JUMP_BACKWARD_NO_INTERRUPT] = rule_jump,
    [CALL] = rule_call,
    [CALL_KW] = rule_call_kw,
    [MATERIALIZE_KWARGS] = rule_materialize_kwargs,
    [CALL_FUNCTION_EX] = rule_call_function_ex,
    [COPY] = rule_copy,
    [DELETE_FAST] = rule_delete_fast,
    [MAKE_CELL] = rule_make_cell,
    [LOAD_DEREF] = rule_load_deref,
    [LOAD_FROM_DICT_OR_DEREF] = rule_load_from_dict_or_deref,
    [STORE_DEREF] = rule_store_deref,
    [DELETE_DEREF] = rule_delete_deref,
    [COPY_FREE_VARS] = rule_copy_free_vars,
    [LOAD_COMMON_CONSTANT] = rule_load_common_constant,
    [LOAD_CONST] = rule_load_const,
    [LOAD_FAST] = rule_load_fast,
    [LOAD_FAST_AND_CLEAR] = rule_load_fast_and_clear,
    [LOAD_FAST_BORROW] = rule_load_fast,
    [LOAD_FAST_BORROW_LOAD_FAST_BORROW] = rule_load_fast_load_fast,
    [LOAD_FAST_CHECK] = rule_load_fast_check,
    [LOAD_FAST_LOAD_FAST] = rule_load_fast_load_fast,
    [LOAD_SMALL_INT] = rule_load_small_int,
    [NOP] = rule_nop,
    [POP_TOP] = rule_pop_top,
    [RETURN_VALUE] = rule_return_value,
    [STORE_FAST] = rule_store_fast,
    [STORE_FAST_LOAD_FAST] = rule_store_fast_load_fast,
    [STORE_FAST_STORE_FAST] = rule_store_fast_store_fast,
    [SWAP] = rule_swap,
    [LOAD_ATTR] = rule_load_attr,
    [STORE_SUBSCR] = rule_store_subscr,
    [DELETE_SUBSCR] = rule_delete_subscr,
    [BINARY_SLICE] = rule_binary_slice,
    [STORE_SLICE] = rule_store_slice,
    [BUILD_SLICE] = rule_build_slice,
    [MAKE_FUNCTION] = rule_make_function,
    [SET_FUNCTION_ATTRIBUTE] = rule_set_function_attribute,
    [CALL_INTRINSIC_1] = rule_call_intrinsic,
    [CALL_INTRINSIC_2] = rule_call_intrinsic,
    [LOAD_NAME] = rule_load_name,
    [LOAD_LOCALS] = rule_load_locals,
    [LOAD_BUILD_CLASS] = rule_load_build_class,
    [LOAD_FROM_DICT_OR_GLOBALS] = rule_load_from_dict_or_globals,
    [STORE_NAME] = rule_store_name,
    [STORE_GLOBAL] = rule_store_global,
    [SETUP_ANNOTATIONS] = rule_setup_annotations,
    [IMPORT_NAME] = rule_import_name,
    [IMPORT_FROM] = rule_import_from,
    [BUILD_TUPLE] = rule_build,
    [BUILD_LIST] = rule_build,
    [BUILD_SET] = rule_build,
    [BUILD_MAP] = rule_build,
    [LIST_APPEND] = rule_insert,
    [SET_ADD] = rule_insert,
    [MAP_ADD] = rule_insert,
    [LIST_EXTEND] = rule_insert,
    [SET_UPDATE] = rule_insert,
    [DICT_UPDATE] = rule_insert,
    [DICT_MERGE] = rule_insert,
    [LOAD_GLOBAL] = rule_load_global,
};

/* An edge binds the parameters of the positions below depth: one that
 * holds a binding that may be absent gives its parameter that type */
static void
mark_maybe(_PyRegTr *t, int depth)
{
    for (int k = 0; k < depth && k < t->sp && k < IR_MAX_SLOTS; k++) {
        if (maybe_absent(t, t->stack[k])) {
            t->maybe_pos[k] = 1;
        }
    }
}

static int
translate_instr(_PyRegTr *t, const _PyRegOp *instr)
{
    _PyRegRule rule = rules[instr->opcode & 255];
    if (rule == NULL || instr->opcode > 255) {
        rule = _PyRegTr_Legacy;
    }
    t->src_sp = t->sp;
    // The edges of instr: its handler's, and its jump's
    if (instr->except >= 0) {
        mark_maybe(t, handler_base(t->cfg, instr->except));
    }
    if (op_jumps(instr)) {
        mark_maybe(t, t->sp);
    }
    return rule(t, instr);
}

/* A test that the conditional branch after it consumes, on the same line
 * and under the same handler: the pair that becomes one TestBranch.  The
 * result is a bool the branch pops, so the test has no other reader; a
 * COMPARE_OP whose result is not converted (bit 16) has a TO_BOOL between
 * and is not adjacent. */
static int
feeds_branch(const _PyRegOp *cmp, const _PyRegOp *next)
{
    switch (cmp->opcode) {
        case COMPARE_OP:
            if (!(cmp->oparg & 16)) {
                return 0;
            }
            break;
        case IS_OP:
        case CONTAINS_OP:
            break;
        default:
            return 0;
    }
    return (next->opcode == POP_JUMP_IF_FALSE || next->opcode == POP_JUMP_IF_TRUE)
           && next->loc.lineno == cmp->loc.lineno
           && next->except == cmp->except && op_jumps(next);
}

/* The branch's target finds the parameters below the test's operands,
 * which the test leaves alone.  Their moves go before the test, so that
 * the pair stays adjacent and becomes one word: the moves hold on both
 * edges, as they did before the test.  The branch then finds its
 * parameters bound and emits no move.
 *
 * The moves write parameter homes and rename identities below the operands,
 * so the hoist stands only when the operands are disjoint from them: no
 * operand is a parameter below, or the identity of a position below.  Else
 * the moves stay with the branch, between the test and it, the two do not
 * fuse, and *stood is 0.  (A TestBranch word could not take them after it:
 * the jump edge has no block of its own to put them in.) */
static int
hoist_edge(_PyRegTr *t, const _PyRegOp *cmp, const _PyRegOp *branch,
           int *stood)
{
    _PyRegCfg *cfg = t->cfg;
    *stood = 0;
    int below = Py_MIN(t->sp - _PyOpcode_num_popped(cmp->opcode, cmp->oparg),
                       t->sp);
    for (int j = below; j < t->sp; j++) {
        int v = t->stack[j];
        if (_PyReg_IS_CANON(cfg, v)
            && v - cfg->nlocalsplus < below)
        {
            return SUCCESS;
        }
        for (int k = 0; k < below; k++) {
            if (t->stack[k] == v) {
                return SUCCESS;
            }
        }
    }
    int operands[2];
    int n = t->sp - below;
    assert(n <= 2);
    for (int j = 0; j < n; j++) {
        operands[j] = t->stack[below + j];
    }
    mark_maybe(t, below + 1);
    RETURN_IF_ERROR(bind_edge(t, below, branch));
    for (int j = 0; j < n; j++) {
        assert(t->stack[below + j] == operands[j]);
    }
    *stood = 1;
    return SUCCESS;
}

/* A test and the conditional jump on its result as one register
 * instruction: TestBranch (Python/register_ops/tests.rop).  The word has
 * the test's origin, so a raise in the test and its line stay the test's,
 * and the jump's target; its source range runs on over the jump, where
 * instrumentation finds the offset that branch events report.  The test
 * names its operands and nothing names the bool between.  The edge's
 * moves were hoisted before the test (hoist_edge()). */
static int
translate_test_branch(_PyRegTr *t, const _PyRegOp *cmp, const _PyRegOp *jump)
{
    if (t->sp < 2) {
        return stack_underflow(cmp);
    }
    _PyRegCfg *cfg = t->cfg;
    t->src_sp = t->sp;
    if (cmp->except >= 0) {
        mark_maybe(t, handler_base(cfg, cmp->except));
        if (!t->permuted) {
            int hbase = Py_MIN(handler_base(cfg, cmp->except), t->sp);
            for (int k = 0; k < hbase; k++) {
                (void)rename_param(t, k);
            }
        }
    }
    // The branch is taken when the test's answer is the sense; `is not` and
    // `not in` invert the answer, which the sense absorbs
    int sense = jump->opcode == POP_JUMP_IF_TRUE;
    int handler, test;
    switch (cmp->opcode) {
        case COMPARE_OP:
            handler = ROP_HANDLER_TestBranch_Generic;
            test = ROP_TEST_LT + (cmp->oparg >> 5);
            break;
        case IS_OP:
            handler = ROP_HANDLER_TestBranch_Identity;
            test = ROP_TEST_IDENTITY;
            sense ^= cmp->oparg;
            break;
        default:
            assert(cmp->opcode == CONTAINS_OP);
            handler = ROP_HANDLER_TestBranch_Generic;
            test = ROP_TEST_CONTAINS;
            sense ^= cmp->oparg;
            break;
    }
    _PyRegOp *op;
    RETURN_IF_ERROR(_PyRegTr_Emit(t, cmp, handler, &t->stack[t->sp - 2], 2,
                                  NULL, 0, &op));
    op->target = jump->target;
    op->oparg = ROP_PACK_TestBranch_test(test) | ROP_PACK_TestBranch_sense(sense);
    return _PyRegTr_Pop(t, 2, NULL);
}

static int
translate(_PyRegCfg *cfg)
{
    RETURN_IF_ERROR(mark_blocks(cfg));
    resolve_next_locations(cfg);
    _PyRegTr t = {.cfg = cfg};
    int n = cfg->nblocks;
    // The stack ops of each block: the translated ops replace them
    int *range = PyMem_Malloc(Py_MAX(2 * n, 1) * sizeof(int));
    t.held = PyMem_Calloc(IR_MAX_SLOTS, 1);
    t.free_since = PyMem_Malloc(IR_MAX_SLOTS * sizeof(int));
    t.maybe_pos = PyMem_Calloc(IR_MAX_SLOTS, 1);
    if (range == NULL || t.held == NULL || t.free_since == NULL ||
        t.maybe_pos == NULL)
    {
        PyMem_Free(range);
        PyMem_Free(t.held);
        PyMem_Free(t.free_since);
        PyMem_Free(t.maybe_pos);
        PyErr_NoMemory();
        return ERROR;
    }
    for (int k = 0; k < IR_MAX_SLOTS; k++) {
        t.free_since[k] = -1;
    }
    for (int b = 0; b < n; b++) {
        range[2 * b] = cfg->blocks[b].start;
        range[2 * b + 1] = cfg->blocks[b].end;
    }
    int err = SUCCESS;
    for (int b = 0; b < n && err == SUCCESS; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        int start = range[2 * b], end = range[2 * b + 1];
        if (!blk->continued) {
            t.region_op = t.nops;
            t.permuted = 0;
            t.ncopies = 0;
            int used = Py_MIN(Py_MAX(t.touched, blk->startdepth) + 1,
                              IR_MAX_SLOTS);
            for (int k = 0; k < used; k++) {
                t.held[k] = k < blk->startdepth;
                t.free_since[k] = -1;
            }
            t.sp = 0;
            for (int k = 0; err == SUCCESS && k < blk->startdepth; k++) {
                err = _PyRegTr_Push(&t, _PyReg_CANON(cfg, k));
            }
        }
        else if (t.sp != blk->startdepth) {
            PyErr_Format(PyExc_SystemError,
                         "inconsistent stack depth at block %d", b);
            err = ERROR;
        }
        blk->start = t.block_op = t.nops;
        for (int i = start; err == SUCCESS && i < end; i++) {
            if (i + 1 < end && feeds_branch(&cfg->ops[i], &cfg->ops[i + 1])) {
                int stood;
                err = hoist_edge(&t, &cfg->ops[i], &cfg->ops[i + 1], &stood);
                if (err != SUCCESS) {
                    break;
                }
                if (stood) {
                    err = translate_test_branch(&t, &cfg->ops[i],
                                                &cfg->ops[i + 1]);
                    i++;
                    continue;
                }
            }
            err = translate_instr(&t, &cfg->ops[i]);
        }
        if (err == SUCCESS && blk->falls_through
            && !cfg->blocks[b + 1].continued)
        {
            // The moves of the edge into the next region go at the end of
            // the block, under the handler of its last op
            _PyRegOp edge = {.except = -1, .dst = -1, .target = -1,
                             .reader = -1, .depth = -1, .fb = -1};
            mark_maybe(&t, t.sp);
            err = bind_edge(&t, t.sp, end > start ? &cfg->ops[end - 1] : &edge);
        }
        blk->end = t.nops;
    }
    PyMem_Free(range);
    PyMem_Free(t.stack);
    PyMem_Free(t.def);
    PyMem_Free(t.natural);
    PyMem_Free(t.held);
    PyMem_Free(t.free_since);
    PyMem_Free(t.rec);
    PyMem_Free(t.mv);
    PyMem_Free(t.maybe);
    PyMem_Free(t.maybe_pos);
    PyMem_Free(t.copies);
    if (err != SUCCESS) {
        PyMem_Free(t.ops);
        return ERROR;
    }
    PyMem_Free(cfg->ops);
    cfg->ops = t.ops;
    cfg->nops = t.nops;
    cfg->aops = t.aops;
    // The passes rename registers to parameters that exist, so no register
    // of a higher position appears after translation
    cfg->ncanon = Py_MIN(t.maxpos, IR_MAX_SLOTS);
    return SUCCESS;
}

void
_PyRegCfg_Rename(_PyRegCfg *cfg, int b, int from, int to)
{
    int start = b;
    while (start > 0 && cfg->blocks[start].continued) {
        start--;
    }
    int end = b + 1;
    while (end < cfg->nblocks && cfg->blocks[end].continued) {
        end++;
    }
    rename_ops(cfg, cfg->ops, cfg->blocks[start].start,
                         cfg->blocks[end - 1].end, from, to);
}


/* Liveness: backward dataflow over the blocks, with the exception edges,
 * to a fixpoint.  A temporary is live from its definition to the op that
 * reads it last; a raise reads every value it leaves on the stack, which
 * the unwinder releases or hands to a handler.  Bit sets cover the
 * temporaries, canonical ones first. */

static inline int
temp_bit(const _PyRegCfg *cfg, int v)
{
    assert(v >= cfg->nlocalsplus);
    if (v < _PyReg_FRESH0(cfg)) {
        return v - cfg->nlocalsplus;
    }
    return _PyReg_INDEX(cfg, v);
}

#define BIT_GET(S, I)   (((S)[(I) >> 6] >> ((I) & 63)) & 1)
#define BIT_SET(S, I)   ((S)[(I) >> 6] |= (uint64_t)1 << ((I) & 63))
#define BIT_CLEAR(S, I) ((S)[(I) >> 6] &= ~((uint64_t)1 << ((I) & 63)))

void
_PyRegCfg_FreeLiveness(_PyRegLiveness *lv)
{
    PyMem_Free(lv->last);
    if (lv->live_in != NULL) {
        PyMem_Free(lv->live_in[0]);
    }
    PyMem_Free(lv->live_in);
    PyMem_Free(lv->live_out);
    memset(lv, 0, sizeof(*lv));
}

/* The staging moves before op (_PyRegCfg_Compact()) define the registers
 * that they give values to and read their sources */
static void
transfer_stage(_PyRegCfg *cfg, _PyRegOp *op, uint64_t *live)
{
    const int *stage = &cfg->operands[op->stage];
    for (int q = 0; q < op->nstage; q++) {
        BIT_CLEAR(live, temp_bit(cfg, stage[2 * q + 1]));
    }
    for (int q = 0; q < op->nstage; q++) {
        if (stage[2 * q] >= cfg->nlocalsplus) {
            BIT_SET(live, temp_bit(cfg, stage[2 * q]));
        }
    }
}

/* The live set before op, given live, the set after it, in place.  When
 * last is not NULL, record which operands the op reads last or writes
 * dead.  With stage_apart, the set is the one after op's staging moves. */
static void
transfer_op(_PyRegCfg *cfg, _PyRegLiveness *lv, _PyRegOp *op, uint64_t *live,
            uint8_t *last, int stage_apart)
{
    if (op->opcode == _PyReg_DEAD) {
        return;
    }
    int nl = cfg->nlocalsplus;
    int *in = _PyReg_IN(cfg, op);
    int *out = _PyReg_OUT(cfg, op);
    for (int j = 0; j < op->nout; j++) {
        if (out[j] >= nl) {
            int bit = temp_bit(cfg, out[j]);
            if (last != NULL) {
                last[op->out + j] = !BIT_GET(live, bit);
            }
        }
    }
    int nin = op->nread + op->nimplicit;
    if (last != NULL) {
        for (int k = 0; k < nin; k++) {
            if (in[k] >= nl) {
                last[op->in + k] = !BIT_GET(live, temp_bit(cfg, in[k]));
            }
        }
    }
    for (int j = 0; j < op->nout; j++) {
        if (out[j] >= nl) {
            BIT_CLEAR(live, temp_bit(cfg, out[j]));
        }
    }
    for (int k = 0; k < nin; k++) {
        if (in[k] >= nl) {
            BIT_SET(live, temp_bit(cfg, in[k]));
        }
    }
    // The handler finds the values below its depth where they are, so
    // they are live while the op runs.  Those of its recipe are the op's
    // implicit inputs.
    if (op->except >= 0) {
        int hd = handler_base(cfg, op->except);
        uint64_t *h = lv->live_in[op->except];
        for (int k = 0; k < hd; k++) {
            if (BIT_GET(h, k) && !_PyRegOp_RecipeDst(cfg, op, k)) {
                BIT_SET(live, k);
            }
        }
    }
    if (!stage_apart) {
        transfer_stage(cfg, op, live);
    }
}

static void
transfer(_PyRegCfg *cfg, _PyRegLiveness *lv, _PyRegOp *op, uint64_t *live,
         uint8_t *last)
{
    transfer_op(cfg, lv, op, live, last, 0);
}

int
_PyRegCfg_Liveness(_PyRegCfg *cfg, _PyRegLiveness *lv)
{
    memset(lv, 0, sizeof(*lv));
    int n = cfg->nblocks;
    int nbits = cfg->ncanon + (cfg->nvregs - _PyReg_FRESH0(cfg));
    int nwords = (nbits + 63) / 64;
    lv->nwords = nwords;
    lv->last = PyMem_Calloc(Py_MAX(cfg->noperands, 1), 1);
    lv->live_in = PyMem_Calloc(Py_MAX(n, 1), sizeof(uint64_t *));
    lv->live_out = PyMem_Calloc(Py_MAX(n, 1), sizeof(uint64_t *));
    uint64_t *sets = PyMem_Calloc((size_t)(2 * n + 1) * nwords, sizeof(uint64_t));
    if (lv->last == NULL || lv->live_in == NULL || lv->live_out == NULL
        || sets == NULL)
    {
        PyMem_Free(sets);
        _PyRegCfg_FreeLiveness(lv);
        PyErr_NoMemory();
        return ERROR;
    }
    for (int b = 0; b < n; b++) {
        lv->live_in[b] = sets + (size_t)(2 * b) * nwords;
        lv->live_out[b] = sets + (size_t)(2 * b + 1) * nwords;
    }
    if (n == 0) {
        lv->live_in[0] = sets;
        return SUCCESS;
    }
    uint64_t *work = sets + (size_t)(2 * n) * nwords;
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int b = n - 1; b >= 0; b--) {
            _PyRegBlock *blk = &cfg->blocks[b];
            uint64_t *lout = lv->live_out[b];
            memset(work, 0, nwords * sizeof(uint64_t));
            if (blk->falls_through) {
                uint64_t *s = lv->live_in[b + 1];
                for (int w = 0; w < nwords; w++) {
                    work[w] |= s[w];
                }
            }
            for (int i = blk->start; i < blk->end; i++) {
                _PyRegOp *op = &cfg->ops[i];
                if (op->opcode != _PyReg_DEAD && op_jumps(op)) {
                    uint64_t *s = lv->live_in[op->target];
                    for (int w = 0; w < nwords; w++) {
                        work[w] |= s[w];
                    }
                }
            }
            memcpy(lout, work, nwords * sizeof(uint64_t));
            for (int i = blk->end - 1; i >= blk->start; i--) {
                transfer(cfg, lv, &cfg->ops[i], work, NULL);
            }
            uint64_t *lin = lv->live_in[b];
            if (memcmp(lin, work, nwords * sizeof(uint64_t)) != 0) {
                memcpy(lin, work, nwords * sizeof(uint64_t));
                changed = 1;
            }
        }
    }
    // The last reads, from the fixpoint
    for (int b = 0; b < n; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        memcpy(work, lv->live_out[b], nwords * sizeof(uint64_t));
        for (int i = blk->end - 1; i >= blk->start; i--) {
            transfer(cfg, lv, &cfg->ops[i], work, lv->last);
        }
    }
    return SUCCESS;
}


/* Slots of an op's fields */

void
_PyRegCfg_OpSlots(const _PyRegCfg *cfg, const _PyRegOp *op,
                  _PyRegSlots *slots)
{
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    int nl = cfg->nlocalsplus;
    // The slot the stack numbering would give: what words whose fields
    // nothing reads keep
    int top = nl + op->depth;
    for (int k = 0; k < 4; k++) {
        slots->in[k] = 0;
    }
    slots->out[1] = 0;
    if (op->opcode >= IR_NUM_OLD_HANDLERS && op->nout > 1) {
        // A register family's second output has a DST role of its own
        slots->out[1] = (op->flags & _PyReg_DISCARD) ? IR_NO_DST
                    : _PyRegCfg_Slot(cfg, out[1]);
    }
    if (fmt->flags & _PyIR_WINDOW) {
        slots->base = op->nread > 0 ? _PyRegCfg_Slot(cfg, in[0]) : op->base;
        // The top ninputs inputs of a window are its sources
        int first = op->nread - fmt->ninputs;
        for (int k = 0; k < fmt->ninputs && k < 4; k++) {
            slots->in[k] = first + k >= 0 ? _PyRegCfg_Slot(cfg, in[first + k]) : 0;
        }
        if (fmt->flags & _PyIR_UNPACKS) {
            slots->out[0] = op->dst >= 0 ? op->dst : IR_NO_DST;
            return;
        }
        slots->out[0] = (op->flags & _PyReg_DISCARD) ? IR_NO_DST
                   : op->dst >= 0 ? op->dst : slots->base + fmt->dst;
        return;
    }
    slots->base = top - op->npop;
    if (_PyIR_CALL_SHAPED(fmt->operands)) {
        // The callable is the deepest input, and the rest are operand words
        slots->in[0] = _PyRegCfg_Slot(cfg, in[0]);
        if (op->flags & _PyReg_DISCARD) {
            slots->out[0] = IR_NO_DST;
        }
        else if (op->dst >= 0) {
            slots->out[0] = op->dst;
        }
        else {
            slots->out[0] = _PyRegCfg_Slot(cfg, out[0]);
        }
        return;
    }
    int first = op->nread - fmt->ninputs;
    for (int k = 0; k < fmt->ninputs && k < 4; k++) {
        slots->in[k] = first + k >= 0 ? _PyRegCfg_Slot(cfg, in[first + k]) : 0;
    }
    if (fmt->operands == _PyIR_OPERANDS_SPANS) {
        // The outputs of a span word are new: its first is the destination
        // that the format names, if any
        slots->out[0] = (op->flags & _PyReg_DISCARD) ? IR_NO_DST
                      : op->dst >= 0 ? op->dst
                      : op->nout > 0 ? _PyRegCfg_Slot(cfg, out[0]) : 0;
        return;
    }
    if (op->flags & _PyReg_DISCARD) {
        slots->out[0] = IR_NO_DST;
    }
    else if (op->dst >= 0) {
        slots->out[0] = op->dst;
    }
    else if (fmt->dst == _PyIR_NO_DST) {
        slots->out[0] = 0;
    }
    else {
        int j = Py_MAX(op->npop + fmt->dst, 0);
        slots->out[0] = j < op->nout ? _PyRegCfg_Slot(cfg, out[j]) : top + fmt->dst;
    }
}


/* Compaction: a slot for every temporary.  A temporary holds its slot
 * from its definition until an op takes it, since the slots of dead
 * temporaries are NULL and an owned value stays until its consumer
 * releases it; liveness alone would free the slot of a value that a loop
 * without an exit keeps.  A temporary defined later takes a slot above
 * every temporary held at its definition; a canonical temporary has the
 * slot of its stack position.  Window outputs go where their window is.
 * Hence
 *
 *   - a window's inputs, the stack top when it runs, are consecutive;
 *   - the temporaries live across an exception handler's range, which the
 *     handler finds below its depth, are canonical, and every other
 *     temporary live in the range was defined after them, so it lies at or
 *     above the handler's first slot, where the unwinder releases it;
 *   - a temporary live across a call was defined before the call's
 *     window, so it lies below it.
 *
 * The verifier checks all three. */

/* The slot limit that the allocator enforces, IR_MAX_SLOTS.  Tests lower it
 * to reach the error, which no stdlib source does. */
int _PyIR_MaxSlots = IR_MAX_SLOTS;

/* How many times o reads register v */
static int
reads_of(const _PyRegCfg *cfg, const _PyRegOp *o, int v)
{
    const int *in = _PyReg_IN(cfg, o);
    int n = 0;
    for (int k = 0; k < o->nread + o->nimplicit; k++) {
        n += in[k] == v;
    }
    return n;
}

static int
slot_error(const _PyRegOp *op, const char *what)
{
    PyErr_Format(PyExc_SystemError, "register allocation: %s at line %d "
                 "(opcode %d)", what, op->loc.lineno, op->opcode);
    return ERROR;
}

typedef struct {
    int *occ;               // per slot less nlocalsplus, its register or -1
    int cap;                // the slots that occ has room for; later ones are free
    int top;                // the slots from top up are free
    int lenient;            // unreachable code, which never runs
    _PyRegOp **reserved;    // folded loads that hold their slot
    int nreserved;
    /* The parameters' definitions in the current region: canon_head[k]
     * is the entry of the first, in op order, of CANON(k); an entry is its
     * op (canon_op) and the next entry (canon_next).  take[] is per fresh
     * register the last op that reads it, or -1. */
    int *canon_head;
    int *canon_op;
    int *canon_next;
    int *take;
    int region_end;         // the end of the current region's ops
} occupancy;

static inline int
occ_at(const occupancy *o, int s)
{
    return s < o->cap ? o->occ[s] : -1;
}

/* Room for slot s, which lies below the slot limit */
static int
occ_reserve(occupancy *o, int s)
{
    if (s < o->cap) {
        return SUCCESS;
    }
    if (s >= IR_MAX_SLOTS) {
        PyErr_SetString(PyExc_SyntaxError, "too many temporaries in function");
        return ERROR;
    }
    int cap = Py_MIN(Py_MAX(2 * o->cap, s + 1), IR_MAX_SLOTS);
    int *occ = PyMem_Realloc(o->occ, cap * sizeof(int));
    if (occ == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    for (int i = o->cap; i < cap; i++) {
        occ[i] = -1;
    }
    o->occ = occ;
    o->cap = cap;
    return SUCCESS;
}

static inline void
occ_free(occupancy *o, int s, int v)
{
    if (occ_at(o, s) == v) {
        o->occ[s] = -1;
        while (o->top > 0 && occ_at(o, o->top - 1) < 0) {
            o->top--;
        }
    }
}

static inline int
occ_take(occupancy *o, int s, int v, const _PyRegOp *op)
{
    if (s < 0) {
        return slot_error(op, "a negative slot");
    }
    if (s >= _PyIR_MaxSlots) {
        // A limit, not an invariant: source can in principle reach it.
        PyErr_Format(PyExc_SyntaxError,
                     "too many temporaries in function at line %d "
                     "(limit %d slots)", op->loc.lineno, _PyIR_MaxSlots);
        return ERROR;
    }
    RETURN_IF_ERROR(occ_reserve(o, s));
    if (o->occ[s] >= 0 && o->occ[s] != v && !o->lenient) {
        PyErr_Format(PyExc_SystemError, "register allocation: two live "
                     "temporaries in slot %d (%d and %d) at line %d "
                     "(opcode %d)", s, o->occ[s], v, op->loc.lineno,
                     op->opcode);
        return ERROR;
    }
    o->occ[s] = v;
    if (s + 1 > o->top) {
        o->top = s + 1;
    }
    return SUCCESS;
}

/* Whether a parameter whose home is slot s is defined after op i and
 * before op last.  A fresh temporary that lives over such a definition
 * cannot take the slot: SWAP's renames give parameters homes out of stack
 * order. */
static int
canon_defined(const _PyRegCfg *cfg, const occupancy *o, int s, int i,
              int last)
{
    int k = s - cfg->nlocalsplus;
    if (k < 0 || k >= cfg->ncanon) {
        return 0;
    }
    for (int e = o->canon_head[k]; e >= 0 && o->canon_op[e] < last;
         e = o->canon_next[e])
    {
        if (o->canon_op[e] > i) {
            return 1;
        }
    }
    return 0;
}

static inline int
take_of(const _PyRegCfg *cfg, const occupancy *o, int v)
{
    return v >= _PyReg_FRESH0(cfg) ? o->take[v - _PyReg_FRESH0(cfg)] : INT_MAX;
}

/* Whether outputs first .. nout - 1 of op i, from slot dst + first, would
 * meet a later parameter's home, a held slot or an input */
static int
fresh_conflict(_PyRegCfg *cfg, occupancy *o, const _PyRegOp *op, int i,
               int dst, int first, int strict)
{
    int nl = cfg->nlocalsplus;
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    for (int j = first; j < op->nout; j++) {
        int s = dst + j;
        if (out[j] < nl || _PyReg_IS_CANON(cfg, out[j])) {
            continue;
        }
        if (canon_defined(cfg, o, s, i, take_of(cfg, o, out[j]))) {
            return 1;
        }
        if (!strict) {
            continue;
        }
        if (occ_at(o, s - nl) >= 0) {
            return 1;
        }
        for (int k = 0; k < op->nread; k++) {
            if (in[k] >= nl && _PyRegCfg_Slot(cfg, in[k]) == s) {
                return 1;
            }
        }
    }
    return 0;
}

static int
assign(_PyRegCfg *cfg, occupancy *o, int v, int s, const _PyRegOp *op)
{
    int *slot = &cfg->slot[_PyReg_INDEX(cfg, v)];
    if (*slot >= 0 && *slot != s && !o->lenient) {
        return slot_error(op, "a temporary in two slots");
    }
    *slot = s;
    return SUCCESS;
}

/* Staging.  Where an op needs a value in another slot than its own (a
 * window's consecutive inputs, the input that an in-place handler turns
 * into its output), a move before the op gives it there to a fresh
 * register, which the op and the region's later ops read instead.  The
 * move takes the old register; its slot was vacant.  A compaction that runs
 * again finds the staged register unplaced and places it anew. */

static int
new_register(_PyRegCfg *cfg, occupancy *o, int *v)
{
    int f = cfg->nvregs - _PyReg_FRESH0(cfg);
    int *slot = PyMem_Realloc(cfg->slot, (cfg->ncanon + f + 1) * sizeof(int));
    if (slot == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    cfg->slot = slot;
    int *take = PyMem_Realloc(o->take, (f + 1) * sizeof(int));
    if (take == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    o->take = take;
    cfg->slot[cfg->ncanon + f] = -1;
    o->take[f] = -1;
    *v = cfg->nvregs++;
    return SUCCESS;
}

/* The register that a staging move of op gives the value of v, or -1 */
static int
staged_from(const _PyRegCfg *cfg, const _PyRegOp *op, int v)
{
    const int *stage = &cfg->operands[op->stage];
    for (int q = 0; q < op->nstage; q++) {
        if (stage[2 * q + 1] == v) {
            return stage[2 * q];
        }
    }
    return -1;
}

/* Rename the reads of v in ops first .. last - 1 to x, up to the op that
 * defines v again: a parameter has a definition per edge that binds it */
static void
rename_reads(_PyRegCfg *cfg, int first, int last, int v, int x)
{
    for (int i = first; i < last; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        int *in = _PyReg_IN(cfg, op);
        for (int k = 0; k < op->nread + op->nimplicit; k++) {
            if (in[k] == v) {
                in[k] = x;
            }
        }
        int *stage = &cfg->operands[op->stage];
        for (int q = 0; q < op->nstage; q++) {
            if (stage[2 * q] == v) {
                stage[2 * q] = x;
            }
        }
        const int *out = _PyReg_OUT(cfg, op);
        for (int j = 0; j < op->nout; j++) {
            if (out[j] == v) {
                return;
            }
        }
    }
}

/* A staging move before op i of v to a new register *x, unplaced, which
 * the reads of v from op i on name instead.  take_here: the op takes v. */
static int
add_stage(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int v,
          int take_here, int *x)
{
    int nl = cfg->nlocalsplus;
    if (v < _PyReg_FRESH0(cfg) && !take_here) {
        // An edge after the op would find the parameter gone
        return slot_error(op, "a parameter to stage that lives on");
    }
    RETURN_IF_ERROR(new_register(cfg, o, x));
    int index;
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, 2 * (op->nstage + 1), &index));
    if (op->nstage) {
        memcpy(&cfg->operands[index], &cfg->operands[op->stage],
               2 * op->nstage * sizeof(int));
    }
    op->stage = index;
    op->nstage++;
    int f = *x - _PyReg_FRESH0(cfg);
    if (v >= _PyReg_FRESH0(cfg)) {
        o->take[f] = o->take[v - _PyReg_FRESH0(cfg)];
        o->take[v - _PyReg_FRESH0(cfg)] = i;
    }
    else {
        o->take[f] = i;
    }
    rename_reads(cfg, i, o->region_end, v, *x);
    // The move takes v.  The staged sources were renamed too: set the pair.
    occ_free(o, _PyRegCfg_Slot(cfg, v) - nl, v);
    cfg->operands[op->stage + 2 * (op->nstage - 1)] = v;
    cfg->operands[op->stage + 2 * (op->nstage - 1) + 1] = *x;
    return SUCCESS;
}

/* Put the value of op's input k in slot s, by a staging move unless an
 * earlier compaction staged it already */
static int
stage_input(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int k, int s)
{
    int nl = cfg->nlocalsplus;
    int v = _PyReg_IN(cfg, op)[k];
    if (v < nl) {
        return slot_error(op, "a local to stage");
    }
    int x = v;
    if (staged_from(cfg, op, v) < 0) {
        RETURN_IF_ERROR(add_stage(cfg, o, op, i, v,
                                  k >= op->nread - op->npop, &x));
    }
    cfg->slot[_PyReg_INDEX(cfg, x)] = s;
    cfg->ntemps = Py_MAX(cfg->ntemps, s - nl + 1);
    if (k < op->nread - op->npop) {
        // The op reads it and leaves it
        RETURN_IF_ERROR(occ_take(o, s - nl, x, op));
    }
    return SUCCESS;
}

/* Whether op reads register v, or a staging move gives it */
static int
op_reads(const _PyRegCfg *cfg, const _PyRegOp *op, int v)
{
    const int *in = _PyReg_IN(cfg, op);
    for (int k = 0; k < op->nread; k++) {
        if (in[k] == v) {
            return 1;
        }
    }
    return 0;
}

/* A folded load's register that holds its slot for the reader (see
 * reserve_folded()) has no value: an output that needs the slot moves the
 * reservation to the first free slot from hi up, with no staging move.
 * Returns 1 if w was such a register, 0 if not. */
static int
relocate_folded(_PyRegCfg *cfg, occupancy *o, const _PyRegOp *op, int w,
                int hi)
{
    int nl = cfg->nlocalsplus;
    for (int r = 0; r < o->nreserved; r++) {
        if (_PyReg_OUT(cfg, o->reserved[r])[0] != w) {
            continue;
        }
        occ_free(o, _PyRegCfg_Slot(cfg, w) - nl, w);
        int s = Py_MAX(o->top, hi - nl);
        while (occ_at(o, s) >= 0) {
            s++;
        }
        cfg->slot[_PyReg_INDEX(cfg, w)] = nl + s;
        cfg->ntemps = Py_MAX(cfg->ntemps, s + 1);
        RETURN_IF_ERROR(occ_take(o, s, w, op));
        return 1;
    }
    return 0;
}

/* Outputs whose slots a parameter's home fixes: the live values in slots
 * lo .. hi - 1 that op neither reads nor writes move away first.  A Move
 * leaves its sources live, so a source there moves away too. */
static int
evict(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int lo, int hi)
{
    int nl = cfg->nlocalsplus;
    int move = _PyIR_IsMoveFormat(_PyRegCfg_FormatOf(op->opcode, op->oparg));
    for (int s = lo; s < hi; s++) {
        int w = occ_at(o, s - nl);
        if (w < 0 || (!move && op_reads(cfg, op, w))) {
            continue;
        }
        const int *out = _PyReg_OUT(cfg, op);
        int passed = 0;
        for (int j = 0; j < op->nout; j++) {
            passed |= out[j] == w;
        }
        if (passed) {
            continue;
        }
        if (w < _PyReg_FRESH0(cfg)) {
            return slot_error(op, "a parameter where an output goes");
        }
        int moved = relocate_folded(cfg, o, op, w, hi);
        RETURN_IF_ERROR(moved);
        if (moved) {
            continue;
        }
        int x;
        RETURN_IF_ERROR(add_stage(cfg, o, op, i, w, 0, &x));
    }
    return SUCCESS;
}

/* Whether the slots that op's parameter outputs fix hold a parameter that
 * op neither reads nor writes */
static int
outputs_blocked(_PyRegCfg *cfg, occupancy *o, const _PyRegOp *op)
{
    int nl = cfg->nlocalsplus;
    const int *out = _PyReg_OUT(cfg, op);
    int base = -1;
    for (int j = 0; j < op->nout && base < 0; j++) {
        if (_PyReg_IS_CANON(cfg, out[j])) {
            base = _PyRegCfg_Slot(cfg, out[j]) - j;
        }
    }
    for (int j = 0; base >= 0 && j < op->nout; j++) {
        int w = occ_at(o, base + j - nl);
        if (w < 0 || w >= _PyReg_FRESH0(cfg) || op_reads(cfg, op, w)) {
            continue;
        }
        int passed = 0;
        for (int p = 0; p < op->nout; p++) {
            passed |= out[p] == w;
        }
        if (!passed) {
            return 1;
        }
    }
    return 0;
}

/* Op's parameter outputs go to new registers instead, which a staging move
 * before the region's next op gives to the parameters: their homes hold a
 * parameter until then */
static int
defer_outputs(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i)
{
    int next = i + 1;
    while (next < o->region_end && cfg->ops[next].opcode == _PyReg_DEAD) {
        next++;
    }
    if (next == o->region_end) {
        return slot_error(op, "a parameter where an output goes");
    }
    _PyRegOp *q = &cfg->ops[next];
    for (int j = 0; j < op->nout; j++) {
        int c = _PyReg_OUT(cfg, op)[j];
        if (!_PyReg_IS_CANON(cfg, c) || op_reads(cfg, op, c)) {
            continue;
        }
        int y;
        RETURN_IF_ERROR(new_register(cfg, o, &y));
        o->take[y - _PyReg_FRESH0(cfg)] = next;
        int index;
        RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, 2 * (q->nstage + 1),
                                              &index));
        if (q->nstage) {
            memcpy(&cfg->operands[index], &cfg->operands[q->stage],
                   2 * q->nstage * sizeof(int));
        }
        cfg->operands[index + 2 * q->nstage] = y;
        cfg->operands[index + 2 * q->nstage + 1] = c;
        q->stage = index;
        q->nstage++;
        _PyReg_OUT(cfg, op)[j] = y;
    }
    return SUCCESS;
}

/* Slots for the registers that staging moves give values to and that op
 * does not read, as evict() makes them: vacant ones outside lo .. hi - 1,
 * where op's inputs and outputs are.  A Move's evicted source is read but
 * lives on: it is placed the same way. */
static int
place_evicted(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int lo,
              int hi)
{
    int nl = cfg->nlocalsplus;
    int move = _PyIR_IsMoveFormat(_PyRegCfg_FormatOf(op->opcode, op->oparg));
    for (int q = 0; q < op->nstage; q++) {
        int x = cfg->operands[op->stage + 2 * q + 1];
        if (!move && op_reads(cfg, op, x)) {
            continue;
        }
        if (_PyReg_IS_CANON(cfg, x)) {
            // A deferred output, to its home
            RETURN_IF_ERROR(occ_take(o, _PyRegCfg_Slot(cfg, x) - nl, x, op));
            continue;
        }
        if (_PyRegCfg_Slot(cfg, x) >= 0) {
            continue;
        }
        for (int s = nl + o->top;; s++) {
            if (s >= lo && s < hi) {
                continue;
            }
            int clash = occ_at(o, s - nl) >= 0
                || canon_defined(cfg, o, s, i, take_of(cfg, o, x));
            const int *in = _PyReg_IN(cfg, op);
            for (int k = 0; k < op->nread && !clash; k++) {
                clash = in[k] >= nl && _PyRegCfg_Slot(cfg, in[k]) == s;
            }
            for (int p = 0; p < op->nstage && !clash; p++) {
                int y = cfg->operands[op->stage + 2 * p + 1];
                clash = y >= nl && _PyRegCfg_Slot(cfg, y) == s;
            }
            if (!clash) {
                cfg->slot[_PyReg_INDEX(cfg, x)] = s;
                cfg->ntemps = Py_MAX(cfg->ntemps, s - nl + 1);
                RETURN_IF_ERROR(occ_take(o, s - nl, x, op));
                break;
            }
        }
    }
    return SUCCESS;
}

/* Whether slots s .. s + n - 1 are vacant, past the inputs that op takes,
 * and no parameter whose home is one of them is defined while the output
 * there lives */
static int
window_vacant(_PyRegCfg *cfg, occupancy *o, const _PyRegOp *op, int i,
              int s, int n)
{
    int nl = cfg->nlocalsplus;
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    // The op's other inputs stay where they are until it reads them
    int nwin = (fmt->flags & _PyIR_WINDOW) ? op->nread - fmt->ninputs : 1;
    for (int k = nwin; k < op->nread; k++) {
        int h = in[k] >= nl ? _PyRegCfg_Slot(cfg, in[k]) : -1;
        if (h >= s && h < s + n) {
            return 0;
        }
    }
    for (int j = 0; j < n; j++) {
        int v = occ_at(o, s + j - nl);
        int passed = j < op->nout && v == out[j];
        if (v >= 0 && !passed) {
            return 0;
        }
        if (j < op->nout && out[j] >= _PyReg_FRESH0(cfg) &&
            canon_defined(cfg, o, s + j, i, take_of(cfg, o, out[j])))
        {
            return 0;
        }
    }
    return 1;
}

/* The base of a window: its first input's slot, if its inputs are
 * consecutive from there and its outputs go to vacant slots, else a base
 * where the inputs are staged.  A window whose output is a parameter has the
 * base that puts the parameter in its home. */
static int
window_base(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int *base)
{
    int nl = cfg->nlocalsplus;
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
    int nwin = op->nread - fmt->ninputs;
    int n = Py_MAX(nwin, op->nout);
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    int required = -1;
    for (int j = 0; j < op->nout; j++) {
        if (_PyReg_IS_CANON(cfg, out[j])) {
            required = _PyRegCfg_Slot(cfg, out[j]) - j;
            break;
        }
    }
    int b = in[0] >= nl ? _PyRegCfg_Slot(cfg, in[0]) : -1;
    int ok = b >= nl && (required < 0 || b == required);
    for (int k = 1; ok && k < nwin; k++) {
        ok = in[k] >= nl && _PyRegCfg_Slot(cfg, in[k]) == b + k;
    }
    if (ok && required < 0 && window_vacant(cfg, o, op, i, b, n)) {
        *base = b;
        return SUCCESS;
    }
    if (required >= 0) {
        b = required;
        RETURN_IF_ERROR(evict(cfg, o, op, i, b, b + n));
        if (ok) {
            *base = b;
            return SUCCESS;
        }
    }
    else {
        b = nl + o->top;
        while (!window_vacant(cfg, o, op, i, b, n)) {
            b++;
        }
    }
    for (int k = 0; k < nwin; k++) {
        RETURN_IF_ERROR(stage_input(cfg, o, op, i, k, b + k));
    }
    *base = b;
    return SUCCESS;
}

/* GET_ITER's specialisations keep the iterable in its slot as the
 * iterator: its outputs go from its input's slot, which staging gives a
 * vacant place first where it has none. */
static int
in_place_dst(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int *dst)
{
    int nl = cfg->nlocalsplus;
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    int required = -1;
    for (int j = 0; j < op->nout; j++) {
        if (_PyReg_IS_CANON(cfg, out[j])) {
            required = _PyRegCfg_Slot(cfg, out[j]) - j;
            break;
        }
    }
    int s = in[0] >= nl ? _PyRegCfg_Slot(cfg, in[0]) : -1;
    if (required >= 0) {
        RETURN_IF_ERROR(evict(cfg, o, op, i, required, required + op->nout));
    }
    if (s >= nl && (required >= 0 ? s == required
                    : window_vacant(cfg, o, op, i, s, op->nout)))
    {
        *dst = s;
        return SUCCESS;
    }
    if (required >= 0) {
        s = required;
    }
    else {
        s = nl + o->top;
        while (!window_vacant(cfg, o, op, i, s, op->nout)) {
            s++;
        }
    }
    RETURN_IF_ERROR(stage_input(cfg, o, op, i, 0, s));
    *dst = s;
    return SUCCESS;
}

/* A vacant slot below the top for the one new output of op i, or -1.  A
 * temporary that an op takes below others leaves its slot vacant (SWAP
 * reorders the stack and emits no word), and without reuse the next output
 * would go above the survivor: straight-line code would creep up one slot
 * per such pair.  The slot keeps the three invariants of compaction:
 *
 *   - nothing holds it and no parameter whose home it is gets defined
 *     while the output lives (fresh_conflict());
 *   - it is no input's slot, so the op reads every input before it writes;
 *   - every handler whose range the output lives across finds it at or
 *     above the handler's first slot, where the unwinder releases it;
 *
 * A window that reads the output would stage it back to the top, so such
 * an output keeps the top.
 *
 * Reuse stays clear of what relies on stack order.  A jump that gives its
 * input as its output where it goes (_PyReg_JUMP_OUTPUT) leaves the value
 * in place, so the value must sit in the home of the output's parameter,
 * where stack order puts it: such a value keeps the top, and no reused slot
 * is the home of a parameter that the region defines later. */
static int
reuse_slot(_PyRegCfg *cfg, occupancy *o, const _PyRegOp *op, int i,
           int through)
{
    int nl = cfg->nlocalsplus;
    int v = _PyReg_OUT(cfg, op)[through];
    int take = take_of(cfg, o, v);
    if (v < _PyReg_FRESH0(cfg) || take <= i || take >= o->region_end ||
        (cfg->ops[take].flags & _PyReg_JUMP_OUTPUT))
    {
        return -1;
    }
    // A window takes its inputs from consecutive slots at the top: a value
    // it reads stays there rather than be staged back
    const _PyRegOp *reader = &cfg->ops[take];
    if (_PyRegCfg_FormatOf(reader->opcode, reader->oparg)->flags & _PyIR_WINDOW) {
        return -1;
    }
    int floor = 0;
    for (int j = i + 1; j <= take; j++) {
        const _PyRegOp *u = &cfg->ops[j];
        if (u->opcode != _PyReg_DEAD && u->except >= 0) {
            floor = Py_MAX(floor, handler_base(cfg, u->except));
        }
    }
    for (int s = floor; s < o->top; s++) {
        if (occ_at(o, s) < 0 &&
            !canon_defined(cfg, o, nl + s, i, o->region_end) &&
            !fresh_conflict(cfg, o, op, i, nl + s - through, through, 1))
        {
            return nl + s;
        }
    }
    return -1;
}

static int
compact_op(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i)
{
    int nl = cfg->nlocalsplus;
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
    int *in = _PyReg_IN(cfg, op);
    int *out = _PyReg_OUT(cfg, op);
    int nin = op->nread + op->nimplicit;
    int window = (fmt->flags & _PyIR_WINDOW) != 0;
    // A window without inputs starts at the stack top, above the values a
    // raise leaves for the unwinder
    int top_before = o->top;
    int stored = window ? fmt->dst : _PyRegOp_Through(op, fmt);
    // The staging moves of an earlier compaction take their sources
    for (int q = 0; q < op->nstage; q++) {
        int v = cfg->operands[op->stage + 2 * q];
        if (v >= nl && _PyRegCfg_Slot(cfg, v) >= nl) {
            occ_free(o, _PyRegCfg_Slot(cfg, v) - nl, v);
        }
    }
    // Inputs that the op takes free their slots: the popped ones, and
    // those a raise leaves for the unwinder, but not its recipe's.  A
    // value that is no longer read but not yet taken still holds its slot,
    // as in a loop that never exits.  A move takes its sources.
    for (int k = op->nread - op->npop; k < nin - op->nrecipe; k++) {
        int v = in[k];
        if (v < nl || _PyRegCfg_Slot(cfg, v) < nl) {
            continue;
        }
        int passed = 0;
        for (int j = 0; j < op->nout; j++) {
            passed |= out[j] == v;
        }
        if (!passed) {
            occ_free(o, _PyRegCfg_Slot(cfg, v) - nl, v);
        }
    }
    if (!(op->flags & _PyReg_MOVES) && outputs_blocked(cfg, o, op)) {
        RETURN_IF_ERROR(defer_outputs(cfg, o, op, i));
        in = _PyReg_IN(cfg, op);
        out = _PyReg_OUT(cfg, op);
    }
    if (op->flags & _PyReg_MOVES) {
        // Its outputs are parameters, whose homes are fixed
    }
    else if (window) {
        int base;
        if (op->nread > 0) {
            if (in[0] < nl) {
                return slot_error(op, "a local in a window");
            }
            RETURN_IF_ERROR(window_base(cfg, o, op, i, &base));
            in = _PyReg_IN(cfg, op);
            out = _PyReg_OUT(cfg, op);
        }
        else if (op->nout > 0 && _PyReg_IS_CANON(cfg, out[0])) {
            // Its output is a parameter, whose home is fixed
            base = _PyRegCfg_Slot(cfg, out[0]);
            RETURN_IF_ERROR(evict(cfg, o, op, i, base, base + op->nout));
            in = _PyReg_IN(cfg, op);
            out = _PyReg_OUT(cfg, op);
        }
        else {
            base = nl + top_before;
            while (fresh_conflict(cfg, o, op, i, base, 0, 1)) {
                base++;
            }
        }
        op->base = base;
        for (int j = 0; j < op->nout; j++) {
            // A result that a store elimination sent to a local passes
            // through the window
            if (out[j] >= nl) {
                RETURN_IF_ERROR(assign(cfg, o, out[j], base + j, op));
            }
        }
    }
    else {
        int through = _PyRegOp_Through(op, fmt);
        if (op->opcode == GET_ITER && op->dst < 0 && through == 0 &&
            op->nread == 1 && op->npop == 1 && op->nout > 0 && out[0] >= nl)
        {
            int dst;
            RETURN_IF_ERROR(in_place_dst(cfg, o, op, i, &dst));
            in = _PyReg_IN(cfg, op);
            out = _PyReg_OUT(cfg, op);
            for (int j = 0; j < op->nout; j++) {
                RETURN_IF_ERROR(assign(cfg, o, out[j], dst + j, op));
            }
        }
        else if (through < op->nout && out[through] >= nl) {
            int dst;
            int v0 = out[through];
            if (_PyReg_IS_CANON(cfg, v0)) {
                dst = _PyRegCfg_Slot(cfg, v0) - through;
                RETURN_IF_ERROR(evict(cfg, o, op, i, dst + through,
                                      dst + op->nout));
                in = _PyReg_IN(cfg, op);
                out = _PyReg_OUT(cfg, op);
            }
            else if (op->nout - through == 1 &&
                     (dst = reuse_slot(cfg, o, op, i, through)) >= 0)
            {
                dst -= through;
            }
            else {
                dst = nl + o->top - through;
                // An output must not land on an input that the handler
                // has not given up yet: only the input at its own stack
                // position, which the handler consumes before writing it.
                // A call is the exception: it reads its operands before it
                // writes, and the store of its result releases the operand
                // that it displaces.
                int bump = 0;
                for (int j = through; j < op->nout && !bump &&
                     fmt->operands == _PyIR_OPERANDS_NONE; j++) {
                    int pos = op->depth - op->npop + j;
                    for (int k = 0; k < op->nread; k++) {
                        int pin = op->depth - op->nread + k;
                        if (in[k] >= nl && pin != pos
                            && _PyRegCfg_Slot(cfg, in[k]) == dst + j)
                        {
                            bump = 1;
                        }
                    }
                }
                // A single output may take the slot of the input at its
                // own stack position, which the op takes
                int own = op->depth - op->npop + through;
                int k = own - (op->depth - op->nread);
                if (bump && op->nout - through == 1 && k >= 0
                    && k < op->nread && in[k] >= nl
                    && k >= op->nread - op->npop
                    && occ_at(o, _PyRegCfg_Slot(cfg, in[k]) - nl) < 0)
                {
                    bump = 0;
                    dst = _PyRegCfg_Slot(cfg, in[k]) - through;
                }
                if (bump) {
                    int high = nl + o->top;
                    for (int k = 0; k < op->nread; k++) {
                        if (in[k] >= nl) {
                            high = Py_MAX(high, _PyRegCfg_Slot(cfg, in[k]) + 1);
                        }
                    }
                    dst = high - through;
                }
                if (fresh_conflict(cfg, o, op, i, dst, through, 0)) {
                    dst = Py_MAX(dst + 1, nl + o->top - through);
                    while (fresh_conflict(cfg, o, op, i, dst, through, 1)) {
                        dst++;
                    }
                }
            }
            for (int j = through; j < op->nout; j++) {
                RETURN_IF_ERROR(assign(cfg, o, out[j], dst + j, op));
            }
        }
    }
    if (op->nstage) {
        int lo = INT_MAX, hi = 0;
        for (int j = 0; j < op->nout; j++) {
            int s = out[j] >= nl ? _PyRegCfg_Slot(cfg, out[j]) : -1;
            if (s >= 0) {
                lo = Py_MIN(lo, s);
                hi = Py_MAX(hi, s + 1);
            }
        }
        RETURN_IF_ERROR(place_evicted(cfg, o, op, i, lo, hi));
    }
    for (int j = 0; j < op->nout; j++) {
        int v = out[j];
        if (v < nl) {
            continue;
        }
        int s = _PyRegCfg_Slot(cfg, v);
        if (s < 0) {
            return slot_error(op, "an output without a slot");
        }
        cfg->ntemps = Py_MAX(cfg->ntemps, s - nl + 1);
        // A jump whose result a store elimination sent to a local leaves
        // the result's slot NULL where it falls through
        if ((op->flags & _PyReg_UNPACK_LOCALS)
            || (op->dst >= 0 && j == stored) || (op->flags & _PyReg_JUMP_OUTPUT)
            || ((op->flags & _PyReg_DISCARD) && j == stored)) {
            continue;
        }
        RETURN_IF_ERROR(occ_take(o, s - nl, v, op));
    }
    return SUCCESS;
}

/* A folded load under a temporary that its reader also reads keeps its
 * slot until the reader runs.  A handler may write an output only to the
 * slot of the input at the output's own stack position, or to a slot no
 * input holds; were the load's slot given to the values above it, the
 * reader's result would have to go above them, and a window that then
 * reads it would not be consecutive. */
static int
reserve_folded(_PyRegCfg *cfg, occupancy *o, _PyRegOp *load)
{
    int nl = cfg->nlocalsplus;
    const _PyRegOp *reader = &cfg->ops[load->reader];
    // A call reads its operands from words: no result has to clear them
    if (_PyIR_HandlerHasOperands(reader->opcode)) {
        return SUCCESS;
    }
    const int *in = _PyReg_IN(cfg, reader);
    int above = 0;
    for (int k = load->reader_k + 1; k < reader->nread; k++) {
        above |= in[k] >= nl;
    }
    if (!above) {
        return SUCCESS;
    }
    int v = _PyReg_OUT(cfg, load)[0];
    RETURN_IF_ERROR(assign(cfg, o, v, nl + o->top, load));
    RETURN_IF_ERROR(occ_take(o, o->top, v, load));
    cfg->ntemps = Py_MAX(cfg->ntemps, o->top);
    o->reserved[o->nreserved++] = load;
    return SUCCESS;
}

/* The reader of folded loads frees the slots they kept. */
static void
release_folded(_PyRegCfg *cfg, occupancy *o, int reader)
{
    int n = 0;
    for (int r = 0; r < o->nreserved; r++) {
        _PyRegOp *load = o->reserved[r];
        if (load->reader == reader) {
            int v = _PyReg_OUT(cfg, load)[0];
            occ_free(o, _PyRegCfg_Slot(cfg, v) - cfg->nlocalsplus, v);
        }
        else {
            o->reserved[n++] = load;
        }
    }
    o->nreserved = n;
}

static int run_passes(_PyRegCfg *cfg);

int
_PyRegCfg_Compact(_PyRegCfg *cfg)
{
    int nl = cfg->nlocalsplus;
    int nregs = cfg->ncanon + (cfg->nvregs - _PyReg_FRESH0(cfg));
    PyMem_Free(cfg->slot);
    cfg->slot = PyMem_Malloc(Py_MAX(nregs, 1) * sizeof(int));
    // The slots are numbered from nl, and the allocator rarely gets far
    // above the stack: occupancy grows when a slot needs room
    int cap = Py_MIN(cfg->ncanon + 8, IR_MAX_SLOTS);
    int *occ = PyMem_Malloc(cap * sizeof(int));
    if (cfg->slot == NULL || occ == NULL) {
        PyMem_Free(occ);
        PyErr_NoMemory();
        return ERROR;
    }
    for (int i = 0; i < nregs; i++) {
        cfg->slot[i] = i < cfg->ncanon ? nl + i : -1;
    }
    cfg->ntemps = 0;
    _PyRegOp **reserved = PyMem_Malloc(Py_MAX(cfg->nops, 1) * sizeof(_PyRegOp *));
    if (reserved == NULL) {
        PyMem_Free(occ);
        PyErr_NoMemory();
        return ERROR;
    }
    int nfresh = cfg->nvregs - _PyReg_FRESH0(cfg);
    int *take = PyMem_Malloc(Py_MAX(nfresh, 1) * sizeof(int));
    int *canon_head = PyMem_Malloc(Py_MAX(cfg->ncanon, 1) * sizeof(int));
    int *canon_op = PyMem_Malloc(Py_MAX(cfg->noperands, 1) * sizeof(int));
    int *canon_next = PyMem_Malloc(Py_MAX(cfg->noperands, 1) * sizeof(int));
    if (take == NULL || canon_head == NULL || canon_op == NULL
        || canon_next == NULL)
    {
        PyMem_Free(take);
        PyMem_Free(canon_head);
        PyMem_Free(canon_op);
        PyMem_Free(canon_next);
        PyMem_Free(reserved);
        PyMem_Free(occ);
        PyErr_NoMemory();
        return ERROR;
    }
    for (int f = 0; f < nfresh; f++) {
        take[f] = -1;
    }
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        const int *in = _PyReg_IN(cfg, op);
        for (int k = 0; k < op->nread + op->nimplicit; k++) {
            if (in[k] >= _PyReg_FRESH0(cfg)) {
                take[in[k] - _PyReg_FRESH0(cfg)] = i;
            }
        }
        for (int q = 0; q < op->nstage; q++) {
            int v = cfg->operands[op->stage + 2 * q];
            if (v >= _PyReg_FRESH0(cfg)) {
                take[v - _PyReg_FRESH0(cfg)] = i;
            }
        }
    }
    occupancy o = {.occ = occ, .cap = cap, .top = 0, .reserved = reserved,
                   .take = take, .canon_head = canon_head,
                   .canon_op = canon_op, .canon_next = canon_next};
    int err = SUCCESS;
    for (int b = 0; b < cfg->nblocks && err == SUCCESS; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        o.lenient = !blk->reachable;
        if (!blk->continued) {
            // The parameters that the region defines, in op order
            int last = b;
            while (last + 1 < cfg->nblocks && cfg->blocks[last + 1].continued) {
                last++;
            }
            o.region_end = cfg->blocks[last].end;
            for (int k = 0; k < cfg->ncanon; k++) {
                canon_head[k] = -1;
            }
            int nentries = 0;
            for (int i = cfg->blocks[last].end - 1; i >= blk->start; i--) {
                _PyRegOp *op = &cfg->ops[i];
                if (op->opcode == _PyReg_DEAD) {
                    continue;
                }
                const int *in = _PyReg_IN(cfg, op);
                const int *out = _PyReg_OUT(cfg, op);
                for (int j = 0; j < op->nout; j++) {
                    int v = out[j];
                    if (!_PyReg_IS_CANON(cfg, v) || v - nl >= cfg->ncanon) {
                        continue;
                    }
                    int passed = 0;
                    for (int k = 0; k < op->nread + op->nimplicit; k++) {
                        passed |= in[k] == v;
                    }
                    int h = canon_head[v - nl];
                    if (!passed && (h < 0 || canon_op[h] != i)) {
                        canon_op[nentries] = i;
                        canon_next[nentries] = h;
                        canon_head[v - nl] = nentries++;
                    }
                }
                // A deferred output's staging move defines its parameter
                for (int q = 0; q < op->nstage; q++) {
                    int v = cfg->operands[op->stage + 2 * q + 1];
                    if (!_PyReg_IS_CANON(cfg, v) || v - nl >= cfg->ncanon) {
                        continue;
                    }
                    int h = canon_head[v - nl];
                    if (h < 0 || canon_op[h] != i) {
                        canon_op[nentries] = i;
                        canon_next[nentries] = h;
                        canon_head[v - nl] = nentries++;
                    }
                }
            }
            for (int s = 0; s < o.cap; s++) {
                o.occ[s] = -1;
            }
            // The stack at a region's entry is canonical, and every value
            // on it holds its slot until an op takes it
            o.top = 0;
            o.nreserved = 0;
            for (int k = 0; k < blk->startdepth; k++) {
                err = occ_reserve(&o, k);
                if (err != SUCCESS) {
                    break;
                }
                o.occ[k] = _PyReg_CANON(cfg, k);
                o.top = k + 1;
            }
        }
        for (int i = blk->start; err == SUCCESS && i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode != _PyReg_DEAD) {
                release_folded(cfg, &o, i);
                err = compact_op(cfg, &o, op, i);
            }
            else if (op->flags & _PyReg_LOAD_FOLDED) {
                err = reserve_folded(cfg, &o, op);
            }
        }
    }
    PyMem_Free(reserved);
    PyMem_Free(o.occ);
    PyMem_Free(o.take);
    PyMem_Free(canon_head);
    PyMem_Free(canon_op);
    PyMem_Free(canon_next);
    RETURN_IF_ERROR(err);
    // The frame holds the values at every region's entry, where the
    // unwinder puts an exception, and every slot that a word names
    for (int b = 0; b < cfg->nblocks; b++) {
        cfg->ntemps = Py_MAX(cfg->ntemps, cfg->blocks[b].startdepth);
    }
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
        _PyRegSlots slots;
        _PyRegCfg_OpSlots(cfg, op, &slots);
        int high = -1;
        int ndst = 0;
        for (int r = 0; r < 4; r++) {
            int role = fmt->roles[r];
            if (role == _PyIR_ROLE_DST && fmt->dst != _PyIR_NO_DST
                && !(op->flags & _PyReg_DISCARD)) {
                high = Py_MAX(high, slots.out[ndst++]);
            }
            else if (role == _PyIR_ROLE_BASE) {
                // A window's handler reads and writes from its base up
                int n = Py_MAX(op->nread, op->nout);
                high = Py_MAX(high, slots.base + Py_MAX(n, 1) - 1);
            }
            else if (role >= _PyIR_ROLE_IN0) {
                high = Py_MAX(high, slots.in[role - _PyIR_ROLE_IN0]);
            }
        }
        cfg->ntemps = Py_MAX(cfg->ntemps, high + 1 - nl);
    }
    return SUCCESS;
}


/* The verifier.  Before compaction it checks the registers: every input
 * is defined on every path, windows hold temporaries, and only canonical
 * temporaries cross region boundaries.  It checks folded loads against the
 * instruction contract at every stage.  After compaction it checks the
 * slots: windows are consecutive, and the two constraints of note 06
 * section 12 hold. */

static int
verify_error(const char *after, const _PyRegOp *op, const char *what)
{
    PyErr_Format(PyExc_SystemError, "register CFG after %s: %s at line %d "
                 "(opcode %d)", after, what, op ? op->loc.lineno : -1,
                 op ? op->opcode : -1);
    return ERROR;
}

/* A folded load's reader reads the local when it runs, not when the load
 * would have, so no live op in between may write the local or run Python
 * code, which may rebind it through frame.f_locals (G1).  "May run Python"
 * depends on the operands (_PyIR_MayRunPython()): a word whose dst names a
 * local releases the local's old value, and that may run __del__.  Copy
 * propagation keeps this rule only because store elimination, which gives
 * words such a dst, runs after it; the check makes the order a checked
 * rule.  A reader that a later pass removed or rewrote reads nothing. */
static int
check_folded_load(_PyRegCfg *cfg, const char *after, int i)
{
    const _PyRegOp *load = &cfg->ops[i];
    int x = load->oparg;
    if (x >= cfg->const_first && x < cfg->const_first + cfg->nconst) {
        // A constant slot is written by nobody (checked in _PyRegCfg_Verify())
        return SUCCESS;
    }
    if (load->reader <= i || load->reader >= cfg->nops) {
        return verify_error(after, load, "a folded load without a reader "
                            "after it");
    }
    const _PyRegOp *reader = &cfg->ops[load->reader];
    if (reader->opcode == _PyReg_DEAD || load->reader_k >= reader->nread
        || _PyReg_IN(cfg, reader)[load->reader_k] != x)
    {
        return SUCCESS;
    }
    for (int j = i + 1; j < load->reader; j++) {
        const _PyRegOp *op = &cfg->ops[j];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
        int role = _PyIR_OpargRole(fmt);
        if (_PyRegOp_WritesLocal(op, x) || (op->oparg == x && (role == _PyIR_ROLE_LOCAL_OUT
                                                || role == _PyIR_ROLE_LOCAL_CLEAR
                                                || role == _PyIR_ROLE_LOCAL_MOVE)))
        {
            return verify_error(after, op, "a write to the local of a folded "
                                "load before its reader");
        }
        if (_PyIR_MayRunPython(fmt, op->dst >= 0)) {
            return verify_error(after, op, "code that may rebind the local "
                                "of a folded load before its reader");
        }
    }
    return SUCCESS;
}

/* Whether register v is a temporary: not a local, cell, free variable or
 * constant slot */
static int
is_temporary(const _PyRegCfg *cfg, int v)
{
    return v >= cfg->nlocalsplus
        && !(v >= cfg->const_first && v < cfg->const_first + cfg->nconst);
}

/* Whether a temporary occurs twice in vs[0 .. n - 1] */
static int
has_duplicate(const _PyRegCfg *cfg, const int *vs, int n)
{
    for (int a = 0; a < n; a++) {
        for (int b = a + 1; b < n; b++) {
            if (vs[a] == vs[b] && is_temporary(cfg, vs[a])) {
                return 1;
            }
        }
    }
    return 0;
}

/* Ownership: each value has one owner (RV change 9).  An op takes no value
 * twice; a parallel move, a recipe and a Move.Retire name each value once;
 * and within a region nothing names a temporary after an op took it,
 * unless an op has defined it again.  A wrong alias breaks one of these:
 * a value released twice, or a home read after it was emptied.  Ops that
 * resolution made name slots, not registers, and are not checked. */
static int
verify_owners(_PyRegCfg *cfg, const char *after)
{
    int nl = cfg->nlocalsplus;
    int *taken = PyMem_Malloc(Py_MAX(cfg->nvregs - nl, 1) * sizeof(int));
    if (taken == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    // taken[v - nl]: the region (block index + 1) in which v was taken
    memset(taken, 0, Py_MAX(cfg->nvregs - nl, 1) * sizeof(int));
    int err = SUCCESS, region = 0;
    for (int b = 0; b < cfg->nblocks && err == SUCCESS; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->continued) {
            region = b + 1;
        }
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; err == SUCCESS && i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD
                || (op->flags & (_PyReg_MOVE_SLOTS | _PyReg_SLOT_RECIPE)))
            {
                continue;
            }
            const int *in = _PyReg_IN(cfg, op);
            const int *out = _PyReg_OUT(cfg, op);
            const int *stage = &cfg->operands[op->stage];
            int nin = op->nread + op->nimplicit;
            for (int k = 0; k < nin && err == SUCCESS; k++) {
                if (is_temporary(cfg, in[k]) && taken[in[k] - nl] == region) {
                    err = verify_error(after, op, "a value named after the "
                                       "op that took it");
                }
            }
            for (int q = 0; q < op->nstage && err == SUCCESS; q++) {
                int v = stage[2 * q];
                if (is_temporary(cfg, v) && taken[v - nl] == region) {
                    err = verify_error(after, op, "a value named after the "
                                       "op that took it");
                }
            }
            if (err != SUCCESS) {
                break;
            }
            // The values the op takes: the popped inputs it does not pass
            // through, or a parallel move's sources
            int first = op->nread - op->npop, ntake = op->npop;
            if (op->flags & _PyReg_MOVES) {
                first = 0;
                ntake = op->nout;
            }
            if (has_duplicate(cfg, &in[first], ntake)) {
                err = verify_error(after, op, "a value that the op takes "
                                   "twice");
            }
            else if (has_duplicate(cfg, &in[nin - op->nrecipe], op->nrecipe)) {
                err = verify_error(after, op, "a value that a recipe moves "
                                   "twice");
            }
            if (err != SUCCESS) {
                break;
            }
            for (int k = first; k < first + ntake; k++) {
                int v = in[k];
                int through = 0;
                for (int j = 0; j < op->nout; j++) {
                    through |= out[j] == v;
                }
                if (is_temporary(cfg, v) && !through) {
                    taken[v - nl] = region;
                }
            }
            for (int q = 0; q < op->nstage; q++) {
                int v = stage[2 * q];
                if (v >= nl) {
                    taken[v - nl] = region;
                }
            }
            for (int q = 0; q < op->nstage; q++) {
                int v = stage[2 * q + 1];
                if (v >= nl) {
                    taken[v - nl] = 0;
                }
            }
            for (int j = 0; j < op->nout; j++) {
                if (out[j] >= nl) {
                    taken[out[j] - nl] = 0;
                }
            }
        }
    }
    PyMem_Free(taken);
    return err;
}

/* Whether the registers in live are in distinct slots of the frame */
static int
verify_slots(_PyRegCfg *cfg, _PyRegLiveness *lv, const uint64_t *live,
             const char *after, _PyRegOp *op)
{
    int nl = cfg->nlocalsplus;
    uint64_t seen[IR_MAX_SLOTS / 64 + 1] = {0};
    for (int bit = 0; bit < lv->nwords * 64; bit++) {
        if (!BIT_GET(live, bit)) {
            continue;
        }
        int v = bit < cfg->ncanon ? nl + bit
                : _PyReg_FRESH0(cfg) + bit - cfg->ncanon;
        int s = _PyRegCfg_Slot(cfg, v) - nl;
        if (s < 0 || s >= cfg->ntemps) {
            return verify_error(after, op, "a live temporary outside the "
                                "frame");
        }
        if ((seen[s >> 6] >> (s & 63)) & 1) {
            return verify_error(after, op, "two live temporaries in one "
                                "slot");
        }
        seen[s >> 6] |= (uint64_t)1 << (s & 63);
    }
    return SUCCESS;
}

int
_PyRegCfg_Verify(_PyRegCfg *cfg, const char *after, int physical)
{
    int nl = cfg->nlocalsplus;
    _PyRegLiveness lv;
    RETURN_IF_ERROR(_PyRegCfg_Liveness(cfg, &lv));
    int err = SUCCESS;
    uint64_t *work = PyMem_Malloc(Py_MAX(lv.nwords, 1) * sizeof(uint64_t));
    if (work == NULL) {
        PyErr_NoMemory();
        err = ERROR;
        goto done;
    }
    for (int b = 0; b < cfg->nblocks && err == SUCCESS; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        uint64_t *lin = lv.live_in[b];
        if (!blk->reachable) {
            continue;
        }
        // Defined on every path: nothing is live at the entry, and a
        // handler finds only what lies below its depth
        int limit = b == 0 ? 0 : blk->is_handler ? blk->startdepth
                  : cfg->ncanon;
        for (int bit = 0; bit < lv.nwords * 64; bit++) {
            if (!BIT_GET(lin, bit)) {
                continue;
            }
            if (bit >= limit && (b == 0 || blk->is_handler)) {
                err = verify_error(after, NULL, "a temporary used before "
                                   "it is defined");
                break;
            }
            if (!blk->continued && bit >= cfg->ncanon) {
                err = verify_error(after, NULL, "a fresh temporary live "
                                   "into a block that is not continued");
                break;
            }
        }
        for (int i = blk->start; err == SUCCESS && i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
            int *in = _PyReg_IN(cfg, op);
            int *out = _PyReg_OUT(cfg, op);
            // A value that nothing reads is not an error: a loop without an
            // exit, as in a finally that never ends, keeps what is below it
            // until the frame is cleared or an exception unwinds it.
            if (op->flags & _PyReg_MOVES) {
                // A parallel move: its outputs are parameters in their homes
            }
            else if (fmt->flags & _PyIR_WINDOW) {
                int nwin = op->nread - fmt->ninputs;
                for (int k = 0; k < op->nread; k++) {
                    if (in[k] < nl) {
                        // Only a source that its field borrows may be a local
                        int f = k - nwin;
                        if (f < 0 || !(fmt->borrow_mask & (1 << f))) {
                            err = verify_error(after, op, "a local in a window");
                        }
                    }
                    else if (physical && k < nwin && _PyRegCfg_Slot(cfg, in[k])
                             != _PyRegCfg_Slot(cfg, in[0]) + k)
                    {
                        err = verify_error(after, op, "a window that is not "
                                           "consecutive");
                    }
                }
                for (int j = 0; physical && j < op->nout; j++) {
                    int base = op->nread > 0 ? _PyRegCfg_Slot(cfg, in[0])
                                             : op->base;
                    if (out[j] >= nl && _PyRegCfg_Slot(cfg, out[j]) != base + j) {
                        err = verify_error(after, op, "a window output out "
                                           "of place");
                    }
                }
            }
            else if (_PyIR_CALL_SHAPED(fmt->operands)) {
                for (int k = 0; k < op->nread; k++) {
                    if (in[k] < nl && !_PyRegOp_CallBorrows(op, k)) {
                        err = verify_error(after, op, "a local in an input "
                                           "that takes it");
                    }
                }
            }
            else {
                int first = op->nread - fmt->ninputs;
                for (int k = 0; k < op->nread; k++) {
                    if (in[k] >= nl) {
                        continue;
                    }
                    int f = k - first;
                    if (f < 0 || !(fmt->borrow_mask & (1 << f))) {
                        err = verify_error(after, op, "a local in a field "
                                           "that takes its input");
                    }
                }
            }
            if (op->dst >= 0
                && !(fmt->flags & (_PyIR_STORE_DST | _PyIR_UNPACKS))) {
                err = verify_error(after, op, "a local dst");
            }
            // A jump gives a temporary where it goes by leaving it in
            // place (a constant slot's readers name the slot itself)
            if (physical && (op->flags & _PyReg_JUMP_OUTPUT)
                && (op->nread != 1 || op->nout != 1
                    || (in[0] >= nl && _PyRegCfg_Slot(cfg, in[0])
                                       != _PyRegCfg_Slot(cfg, out[0]))))
            {
                err = verify_error(after, op, "a jump output that is not "
                                   "its input's slot");
            }
            if (cfg->nconst) {
                int role = _PyIR_OpargRole(fmt);
                int written = op->dst;
                if (role == _PyIR_ROLE_LOCAL_OUT
                    || role == _PyIR_ROLE_LOCAL_CLEAR
                    || role == _PyIR_ROLE_LOCAL_MOVE)
                {
                    written = op->oparg;
                }
                if (written >= cfg->const_first
                    && written < cfg->const_first + cfg->nconst)
                {
                    err = verify_error(after, op, "a write to a constant slot");
                }
            }
            if ((op->flags & _PyReg_ELIDED)
                && ((op->opcode != PUSH_NULL && !(op->flags & _PyReg_KWNAMES))
                    || op->dst >= 0 || op->nout != 1 || out[0] < nl)) {
                err = verify_error(after, op, "an elided op that is not a "
                                   "PUSH_NULL or the names of a call");
            }
            if (op->flags & _PyReg_DISCARD) {
                // A fresh register lives within its region
                int reads = 0, last = b;
                while (last + 1 < cfg->nblocks && cfg->blocks[last + 1].continued) {
                    last++;
                }
                for (int q = i + 1; q < cfg->blocks[last].end; q++) {
                    if (cfg->ops[q].opcode != _PyReg_DEAD) {
                        reads += reads_of(cfg, &cfg->ops[q], out[0]);
                    }
                }
                if (!(fmt->flags & _PyIR_DISCARDS) || op->dst >= 0
                    || op->nout != 1 || reads != 0)
                {
                    err = verify_error(after, op, "a discarded result that "
                                       "is read or cannot be discarded");
                }
            }
        }
    }
    for (int i = 0; i < cfg->nops && err == SUCCESS; i++) {
        const _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == _PyReg_DEAD && (op->flags & _PyReg_LOAD_FOLDED)) {
            err = check_folded_load(cfg, after, i);
        }
    }
    if (err == SUCCESS) {
        err = verify_owners(cfg, after);
    }
    if (err == SUCCESS && physical) {
        // The constraints of note 06 section 12, op by op
        for (int b = 0; b < cfg->nblocks && err == SUCCESS; b++) {
            _PyRegBlock *blk = &cfg->blocks[b];
            if (!blk->reachable) {
                continue;
            }
            memcpy(work, lv.live_out[b], lv.nwords * sizeof(uint64_t));
            for (int i = blk->end - 1; i >= blk->start && err == SUCCESS; i--) {
                _PyRegOp *op = &cfg->ops[i];
                if (op->opcode == _PyReg_DEAD) {
                    continue;
                }
                // work is the set live after op: those live across it
                // are live after it and not written by it
                int *out = _PyReg_OUT(cfg, op);
                if (op->except >= 0) {
                    int hd = handler_base(cfg, op->except);
                    int first = nl + hd;
                    for (int bit = 0; bit < lv.nwords * 64; bit++) {
                        if (!BIT_GET(work, bit)) {
                            continue;
                        }
                        int v = bit < cfg->ncanon ? nl + bit
                                : _PyReg_FRESH0(cfg) + bit - cfg->ncanon;
                        int s = _PyRegCfg_Slot(cfg, v);
                        int is_out = 0;
                        for (int j = 0; j < op->nout; j++) {
                            is_out |= out[j] == v;
                        }
                        if (is_out) {
                            continue;
                        }
                        // The recipe moves its values, and releases what
                        // its homes held
                        if (_PyRegOp_RecipeSource(cfg, op, v) ||
                            (s < first && _PyRegOp_RecipeDst(cfg, op, s - nl)))
                        {
                            continue;
                        }
                        if (bit < hd ? s != nl + bit : s < first) {
                            err = verify_error(after, op, "a temporary that "
                                               "the unwinder would not find "
                                               "or would release");
                            break;
                        }
                    }
                }
                // Compaction shares a slot only between temporaries that
                // are never live at once
                if (err == SUCCESS) {
                    err = verify_slots(cfg, &lv, work, after, op);
                }
                transfer_op(cfg, &lv, op, work, NULL, 1);
                if (err == SUCCESS && op->nstage
                    && !(op->flags & _PyReg_MOVES))
                {
                    // A staging move gives its value to a vacant slot
                    err = verify_slots(cfg, &lv, work, after, op);
                }
                transfer_stage(cfg, op, work);
            }
        }
    }
done:
    PyMem_Free(work);
    _PyRegCfg_FreeLiveness(&lv);
    if (err == SUCCESS) {
        err = _PyRegCfg_VerifyCallRoles(cfg);
    }
    return err;
}


/* A listing of the CFG, for debugging: set PYTHONREGCFGDUMP in a debug
 * build. */
#ifdef Py_DEBUG
static void
dump_reg(const _PyRegCfg *cfg, int v)
{
    if (v < cfg->nlocalsplus) {
        fprintf(stderr, " L%d", v);
    }
    else if (v < _PyReg_FRESH0(cfg)) {
        fprintf(stderr, " C%d", v - cfg->nlocalsplus);
    }
    else {
        fprintf(stderr, " t%d", v - _PyReg_FRESH0(cfg));
    }
    if (v >= cfg->nlocalsplus && cfg->slot != NULL) {
        fprintf(stderr, "@%d", _PyRegCfg_Slot(cfg, v));
    }
}

static void
dump(const _PyRegCfg *cfg, const char *after)
{
    fprintf(stderr, "--- register CFG after %s (nlocalsplus %d)\n", after,
            cfg->nlocalsplus);
    for (int b = 0; b < cfg->nblocks; b++) {
        const _PyRegBlock *blk = &cfg->blocks[b];
        fprintf(stderr, "B%d depth %d%s%s%s%s\n", b, blk->startdepth,
                blk->continued ? " continued" : "",
                blk->is_handler ? " handler" : "",
                blk->reachable ? "" : " unreachable",
                blk->falls_through ? "" : " ends");
        for (int i = blk->start; i < blk->end; i++) {
            const _PyRegOp *op = &cfg->ops[i];
            const char *name = op->opcode == _PyReg_DEAD ? "(dead)"
                               : op->opcode >= ROP_FIRST_HANDLER ? "(arm)"
                               : _PyOpcode_OpName[op->opcode];
            fprintf(stderr, "  %4d %-24s %3d", op->loc.lineno, name, op->oparg);
            const int *out = _PyReg_OUT(cfg, op);
            for (int j = 0; j < op->nout; j++) {
                dump_reg(cfg, out[j]);
            }
            if (op->dst >= 0) {
                fprintf(stderr, " dst=L%d", op->dst);
            }
            fprintf(stderr, " <-");
            const int *in = _PyReg_IN(cfg, op);
            for (int k = 0; k < op->nread + op->nimplicit; k++) {
                if (k == op->nread) {
                    fprintf(stderr, " |");
                }
                dump_reg(cfg, in[k]);
            }
            if (op->target >= 0) {
                fprintf(stderr, " -> B%d", op->target);
            }
            if (op->except >= 0) {
                fprintf(stderr, " except B%d", op->except);
            }
            if (op->flags & _PyReg_MOVES) {
                fprintf(stderr, " moves");
            }
            for (int q = 0; q < op->nstage; q++) {
                fprintf(stderr, q ? "," : " staged");
                dump_reg(cfg, cfg->operands[op->stage + 2 * q]);
                fprintf(stderr, " ->");
                dump_reg(cfg, cfg->operands[op->stage + 2 * q + 1]);
            }
            if (op->opcode != _PyReg_DEAD && op->nread == 0
                && (_PyRegCfg_FormatOf(op->opcode, op->oparg)->flags
                    & _PyIR_WINDOW))
            {
                fprintf(stderr, " base %d", op->base);
            }
            for (int q = 0; q < op->nrecipe; q++) {
                fprintf(stderr, q ? "," : " recipe");
                if (op->flags & _PyReg_SLOT_RECIPE) {
                    fprintf(stderr, " %d->%d", cfg->operands[op->recipe + 2 * q],
                            cfg->operands[op->recipe + 2 * q + 1]);
                }
                else {
                    fprintf(stderr, " C%d", cfg->operands[op->recipe + q]);
                }
            }
            fprintf(stderr, "\n");
        }
    }
}

static int
dump_wanted(void)
{
    static int wanted = -1;
    if (wanted < 0) {
        const char *env = getenv("PYTHONREGCFGDUMP");
        wanted = env != NULL && *env != '\0';
    }
    return wanted;
}
#  define DUMP(CFG, AFTER) \
    do { \
        if (dump_wanted()) { \
            dump((CFG), (AFTER)); \
        } \
    } while (0)
#else
#  define DUMP(CFG, AFTER) ((void)0)
#endif


/* The driver */

/* Whether to verify the CFG after conversion, after each pass and after
 * compaction.  A debug build always does; a release build does where
 * PYTHONREGCFGVERIFY is set, so that the checks can run over the standard
 * library at the speed of the optimised build (test_ir_contract).  The
 * variable is read once, and like the other PYTHON* variables, not under
 * -E or -I. */
static int
verify_wanted(void)
{
#ifdef Py_DEBUG
    return 1;
#else
    static int wanted = -1;
    int w = _Py_atomic_load_int_relaxed(&wanted);
    if (w < 0) {
        const PyConfig *config = _Py_GetConfig();
        w = _Py_GetEnv(config->use_environment, "PYTHONREGCFGVERIFY") != NULL;
        _Py_atomic_store_int_relaxed(&wanted, w);
    }
    return w;
#endif
}

typedef int (*pass_fn)(_PyRegCfg *);

static const struct {
    const char *name;
    pass_fn fn;
} passes[] = {
    {"copy propagation", _PyRegPass_CopyProp},
    {"store elimination", _PyRegPass_StoreElim},
    {"unpack to locals", _PyRegPass_UnpackLocals},
    {"truth folding", _PyRegPass_TruthFold},
    {"call keyword names", _PyRegPass_KwNames},
    {"null elision", _PyRegPass_ElideNull},
    {"call input roles", _PyRegPass_CallRoles},
};

static int
run_passes(_PyRegCfg *cfg)
{
    DUMP(cfg, "conversion");
    int verify = verify_wanted();
    if (verify) {
        RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, "conversion", 0));
    }
    for (size_t i = 0; i < Py_ARRAY_LENGTH(passes); i++) {
        RETURN_IF_ERROR(passes[i].fn(cfg));
        if (verify) {
            RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, passes[i].name, 0));
        }
    }
    return SUCCESS;
}

int
_PyRegCfg_Build(_PyRegCfg *cfg)
{
    RETURN_IF_ERROR(translate(cfg));
    RETURN_IF_ERROR(run_passes(cfg));
    int err = _PyRegCfg_Compact(cfg);
    DUMP(cfg, "compaction");
    RETURN_IF_ERROR(err);
    if (verify_wanted()) {
        RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, "compaction", 1));
    }
    return SUCCESS;
}

/* Resolution.  Allocation leaves moves where homes disagree: the staging
 * moves before an op and the parallel moves (_PyReg_MOVES) that bind a
 * region's parameters at an edge.  Each set becomes one Move.Transfer
 * word (Python/register_ops/values.rop), whose run is the parallel move
 * that the unwinder's recipes use too (_PyFrame_ParallelMove()).  A
 * move's target is vacant (NULL) or the source of another move of the
 * set, so no value ever has two owners, and none is left behind.  The word
 * keeps its op's handler, with the recipe that finds the handler's values
 * where they are before it (_PyReg_SLOT_RECIPE). */

typedef struct {
    int *src;           // pending moves, slot to slot
    int *dst;
    int n;
} move_set;

/* The parallel moves of op as slot pairs in m, and its recipe: the slot
 * before the moves of each value that the handler finds elsewhere than its
 * home, in from[], and the home, in home[]. */
static int
collect_moves(_PyRegCfg *cfg, _PyRegOp *op, move_set *m, int *from,
              int *home, int *nrec)
{
    const int *in = _PyReg_IN(cfg, op);
    const int *out = _PyReg_OUT(cfg, op);
    const int *stage = &cfg->operands[op->stage];
    m->n = 0;
    int moves = (op->flags & _PyReg_MOVES) != 0;
    // A parallel move runs with the staging moves before it as one
    int npairs = (moves ? op->nout : 0) + op->nstage;
    for (int q = 0; q < npairs; q++) {
        int st = q - (moves ? op->nout : 0);
        int v = st < 0 ? in[op->nread + q] : stage[2 * st];
        int x = st < 0 ? out[q] : stage[2 * st + 1];
        for (int p = 0; moves && st >= 0 && p < op->nout; p++) {
            if (in[op->nread + p] == x) {
                return slot_error(op, "a move of what a staging move gives");
            }
        }
        int a = _PyRegCfg_Slot(cfg, v), b = _PyRegCfg_Slot(cfg, x);
        if (a < cfg->nlocalsplus || b < cfg->nlocalsplus) {
            return slot_error(op, "a move from or to a local");
        }
        if (a != b) {
            m->src[m->n] = a;
            m->dst[m->n] = b;
            m->n++;
        }
    }
    *nrec = 0;
    const int *rsrc = &in[op->nread + op->nimplicit - op->nrecipe];
    for (int q = 0; q < op->nrecipe; q++) {
        int r = rsrc[q];
        int s = _PyRegCfg_Slot(cfg, r);
        // A parallel move's recipe names the values before it; an op's
        // names those that its staging gives
        for (int p = 0; !moves && p < op->nstage; p++) {
            if (stage[2 * p + 1] == r) {
                s = _PyRegCfg_Slot(cfg, stage[2 * p]);
            }
        }
        from[*nrec] = s;
        home[*nrec] = _PyRegCfg_Slot(
            cfg, _PyReg_CANON(cfg, cfg->operands[op->recipe + q]));
        (*nrec)++;
    }
    // A parameter that a staging move gives, as for a deferred output, is
    // where its source is until the move
    int hd = op->except >= 0 ? handler_base(cfg, op->except) : 0;
    for (int p = 0; p < op->nstage; p++) {
        int x = stage[2 * p + 1];
        if (_PyReg_IS_CANON(cfg, x) && x - cfg->nlocalsplus < hd) {
            from[*nrec] = _PyRegCfg_Slot(cfg, stage[2 * p]);
            home[*nrec] = _PyRegCfg_Slot(cfg, x);
            (*nrec)++;
        }
    }
    return SUCCESS;
}

static int
resolve_moves(_PyRegCfg *cfg)
{
    int extra = 0, max = 1;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        int n = ((op->flags & _PyReg_MOVES) ? op->nout : 0) + op->nstage;
        extra += n > 0;
        max = Py_MAX(max, n + op->nrecipe);
    }
    if (extra == 0) {
        return SUCCESS;
    }
    int err = ERROR;
    _PyRegOp *ops = PyMem_Calloc(cfg->nops + extra, sizeof(_PyRegOp));
    int *first = PyMem_Malloc((cfg->nops + 1) * sizeof(int));
    int *index = PyMem_Malloc((cfg->nops + 1) * sizeof(int));
    int *buf = PyMem_Malloc(8 * max * sizeof(int));
    if (ops == NULL || first == NULL || index == NULL || buf == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    move_set m = {.src = buf, .dst = buf + max};
    int *from = buf + 4 * max, *home = buf + 5 * max;
    int nops = 0;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        first[i] = nops;
        int moves = (op->flags & _PyReg_MOVES) != 0;
        if (op->opcode == _PyReg_DEAD || (!moves && op->nstage == 0)) {
            index[i] = nops;
            ops[nops++] = *op;
            continue;
        }
        int nrec;
        if (collect_moves(cfg, op, &m, from, &home[0], &nrec) < 0) {
            goto done;
        }
        if (m.n > 0) {
            // The recipe before the word: where the handler's values are
            int npairs = 0;
            for (int q = 0; q < nrec; q++) {
                npairs += from[q] != home[q];
            }
            int at = 0, elems;
            if ((npairs && _PyRegCfg_AddOperands(cfg, 2 * npairs, &at) < 0) ||
                _PyRegCfg_AddOperands(cfg, 2 + 2 * m.n, &elems) < 0)
            {
                goto done;
            }
            int *pairs = &cfg->operands[at];
            for (int q = 0, p = 0; q < nrec; q++) {
                if (from[q] != home[q]) {
                    pairs[2 * p] = from[q];
                    pairs[2 * p + 1] = home[q];
                    p++;
                }
            }
            // Its counts, then the destinations, then the sources
            int *e = &cfg->operands[elems];
            e[0] = e[1] = m.n;
            for (int q = 0; q < m.n; q++) {
                e[2 + q] = m.dst[q];
                e[2 + m.n + q] = m.src[q];
            }
            _PyRegOp *move = &ops[nops++];
            *move = *op;
            move->opcode = ROP_SELECT_Move_transfer;
            move->oparg = 0;
            move->in = elems;
            move->nread = move->npop = move->nimplicit = move->nout = 0;
            move->nstage = 0;
            move->dst = move->target = -1;
            move->reader = -1;
            move->completion = move->kwnames = 0;
            move->flags = _PyReg_SLOT_RECIPE | _PyReg_MOVE_SLOTS;
            move->nrecipe = npairs;
            move->recipe = at;
        }
        index[i] = nops;
        if (moves) {
            // Its words are the moves: it stays, without a word
            ops[nops] = *op;
            ops[nops].opcode = _PyReg_DEAD;
            ops[nops].flags = 0;
            nops++;
        }
        else {
            ops[nops] = *op;
            ops[nops].nstage = 0;
            nops++;
        }
    }
    first[cfg->nops] = index[cfg->nops] = nops;
    for (int i = 0; i < nops; i++) {
        _PyRegOp *op = &ops[i];
        if (op->flags & _PyReg_LOAD_FOLDED) {
            op->reader = index[op->reader];
        }
        if (op->completion > 0) {
            op->completion = index[op->completion - 1] + 1;
        }
    }
    for (int b = 0; b < cfg->nblocks; b++) {
        cfg->blocks[b].start = first[cfg->blocks[b].start];
        cfg->blocks[b].end = first[cfg->blocks[b].end];
    }
    PyMem_Free(cfg->ops);
    cfg->ops = ops;
    ops = NULL;
    cfg->nops = cfg->aops = nops;
    err = SUCCESS;
done:
    PyMem_Free(ops);
    PyMem_Free(first);
    PyMem_Free(index);
    PyMem_Free(buf);
    return err;
}

/* Constant slots (G6), after the passes and compaction have run */

int
_PyRegCfg_AssignConstSlots(_PyRegCfg *cfg, PyObject *consts)
{
    int n = _PyRegPass_ConstSlots(cfg, consts);
    RETURN_IF_ERROR(n);
    _PyRegPass_DiscardSlot(cfg);
    DUMP(cfg, "constant slots");
    if (verify_wanted()) {
        RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, "constant slots", 0));
    }
    int err = _PyRegCfg_Compact(cfg);
    DUMP(cfg, "second compaction");
    RETURN_IF_ERROR(err);
    if (verify_wanted()) {
        RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, "second compaction", 1));
    }
    err = resolve_moves(cfg);
    DUMP(cfg, "resolution");
    return err;
}
