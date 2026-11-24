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
#include "pycore_ir.h"                // _PyIR_FormatOf()
#include "pycore_opcode_metadata.h"   // _PyOpcode_num_popped()
#include "pycore_opcode_utils.h"      // IS_SCOPE_EXIT_OPCODE()
#include "pycore_pystate.h"           // _Py_GetConfig()
#include "pycore_regcfg.h"

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
    /* The recipe of the op being translated, and the moves of an edge:
     * pairs of an identity and the stack position of the parameter that it
     * becomes */
    int *rec;
    int nrec;
    int arec;
    int *mv;
    int nmv;
    int amv;
};

int
_PyRegTr_Fresh(_PyRegTr *t, int *v)
{
    _PyRegCfg *cfg = t->cfg;
    int k = cfg->nvregs - _PyReg_FRESH0(cfg);
    RETURN_IF_ERROR(grow((void **)&t->def, &t->adef, k + 1, sizeof(int)));
    RETURN_IF_ERROR(grow((void **)&t->natural, &t->anatural, k + 1,
                         sizeof(int)));
    t->def[k] = t->nops;
    t->natural[k] = -1;
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
    for (int k = 0; k < n; k++) {
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
    op->fuse = 0;
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

int
_PyRegTr_Emit(_PyRegTr *t, const _PyRegOp *src, int opcode,
              const int *uses, int nuses, const int *defs, int ndefs,
              _PyRegOp **result)
{
    _PyRegCfg *cfg = t->cfg;
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
    // Register style: the outputs below dst pass their inputs through
    int through = 0;
    if (!window) {
        through = fmt->operands != _PyIR_OPERANDS_NONE ? 0
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
        err = new_op(t, instr, nread + nimplicit + nrec, pushed, &op);
    }
    if (err == SUCCESS && nrec) {
        err = _PyRegCfg_AddOperands(cfg, nrec, &op->recipe);
    }
    if (err == SUCCESS) {
        int *in = _PyReg_IN(cfg, op);
        for (int k = 0; k < nread; k++) {
            in[k] = t->stack[depth - nread + k];
        }
        for (int k = 0; k < nimplicit; k++) {
            in[nread + k] = t->stack[k];
        }
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

/* The rules, by opcode.  NULL is the legacy rule. */
static const _PyRegRule rules[256] = {
    [SWAP] = rule_swap,
};

static int
translate_instr(_PyRegTr *t, const _PyRegOp *instr)
{
    _PyRegRule rule = rules[instr->opcode & 255];
    if (rule == NULL || instr->opcode > 255) {
        rule = _PyRegTr_Legacy;
    }
    return rule(t, instr);
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
    if (range == NULL || t.held == NULL || t.free_since == NULL) {
        PyMem_Free(range);
        PyMem_Free(t.held);
        PyMem_Free(t.free_since);
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
        blk->start = t.nops;
        for (int i = start; err == SUCCESS && i < end; i++) {
            err = translate_instr(&t, &cfg->ops[i]);
        }
        if (err == SUCCESS && blk->falls_through
            && !cfg->blocks[b + 1].continued)
        {
            // The moves of the edge into the next region go at the end of
            // the block, under the handler of its last op
            _PyRegOp edge = {.except = -1, .dst = -1, .target = -1,
                             .reader = -1, .depth = -1, .fb = -1};
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
    if (fmt->flags & _PyIR_WINDOW) {
        slots->base = op->nread > 0 ? _PyRegCfg_Slot(cfg, in[0]) : op->base;
        // The top ninputs inputs of a window are its sources
        int first = op->nread - fmt->ninputs;
        for (int k = 0; k < fmt->ninputs && k < 4; k++) {
            slots->in[k] = first + k >= 0 ? _PyRegCfg_Slot(cfg, in[first + k]) : 0;
        }
        if (fmt->flags & _PyIR_UNPACKS) {
            slots->dst = op->dst >= 0 ? op->dst : IR_NO_DST;
            return;
        }
        slots->dst = (op->flags & _PyReg_DISCARD) ? IR_NO_DST
                   : op->dst >= 0 ? op->dst : slots->base + fmt->dst;
        return;
    }
    slots->base = top - op->npop;
    if (fmt->operands != _PyIR_OPERANDS_NONE) {
        // The callable is the deepest input, and the rest are operand words
        slots->in[0] = _PyRegCfg_Slot(cfg, in[0]);
        if (op->flags & _PyReg_DISCARD) {
            slots->dst = IR_NO_DST;
        }
        else if (op->dst >= 0) {
            slots->dst = op->dst;
        }
        else {
            slots->dst = _PyRegCfg_Slot(cfg, out[0]);
        }
        return;
    }
    int first = op->nread - fmt->ninputs;
    for (int k = 0; k < fmt->ninputs && k < 4; k++) {
        slots->in[k] = first + k >= 0 ? _PyRegCfg_Slot(cfg, in[first + k]) : 0;
    }
    if (op->flags & _PyReg_DISCARD) {
        slots->dst = IR_NO_DST;
    }
    else if (op->dst >= 0) {
        slots->dst = op->dst;
    }
    else if (fmt->dst == _PyIR_NO_DST) {
        slots->dst = 0;
    }
    else {
        int j = op->npop + fmt->dst;
        slots->dst = j < op->nout ? _PyRegCfg_Slot(cfg, out[j]) : top + fmt->dst;
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

/* Outputs whose slots a parameter's home fixes: the live values in slots
 * lo .. hi - 1 that op neither reads nor writes move away first */
static int
evict(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int lo, int hi)
{
    int nl = cfg->nlocalsplus;
    for (int s = lo; s < hi; s++) {
        int w = occ_at(o, s - nl);
        if (w < 0 || op_reads(cfg, op, w)) {
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
 * where op's inputs and outputs are */
static int
place_evicted(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op, int i, int lo,
              int hi)
{
    int nl = cfg->nlocalsplus;
    for (int q = 0; q < op->nstage; q++) {
        int x = cfg->operands[op->stage + 2 * q + 1];
        if (op_reads(cfg, op, x)) {
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
        for (int r = 0; r < 4; r++) {
            int role = fmt->roles[r];
            if (role == _PyIR_ROLE_DST && fmt->dst != _PyIR_NO_DST
                && !(op->flags & _PyReg_DISCARD)) {
                high = Py_MAX(high, slots.dst);
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
            else if (fmt->operands != _PyIR_OPERANDS_NONE) {
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
    {"swap renaming", _PyRegPass_SwapRename},
    {"copy propagation", _PyRegPass_CopyProp},
    {"store elimination", _PyRegPass_StoreElim},
    {"unpack to locals", _PyRegPass_UnpackLocals},
    {"pop top", _PyRegPass_PopTop},
    {"discard", _PyRegPass_Discard},
    {"truth folding", _PyRegPass_TruthFold},
    {"discard after truth folding", _PyRegPass_Discard},
    {"jump fusion", _PyRegPass_FuseJump},
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
 * region's parameters at an edge.  Each becomes a sequence of SWAP words.
 * A move's target is vacant (NULL) or the source of another move of the
 * set, so a SWAP into a vacant target moves the value and leaves its source
 * NULL, and a cycle resolves by swapping and following the displaced value:
 * no value ever has two owners, and none is left behind.  Every word keeps
 * its op's handler, with the recipe that finds the handler's values where
 * the words before it put them (_PyReg_SLOT_RECIPE). */

typedef struct {
    int *src;           // pending moves, slot to slot
    int *dst;
    int n;
    int *swaps;         // pairs of slots, in order
    int nswaps;
} move_set;

static void
sequence_moves(move_set *m)
{
    m->nswaps = 0;
    while (m->n > 0) {
        int j = -1;
        for (int a = 0; a < m->n && j < 0; a++) {
            int free = 1;
            for (int b = 0; b < m->n; b++) {
                free &= b == a || m->src[b] != m->dst[a];
            }
            if (free) {
                j = a;
            }
        }
        int cycle = j < 0;
        if (cycle) {
            j = 0;
        }
        int from = m->src[j], to = m->dst[j];
        m->swaps[2 * m->nswaps] = from;
        m->swaps[2 * m->nswaps + 1] = to;
        m->nswaps++;
        m->n--;
        m->src[j] = m->src[m->n];
        m->dst[j] = m->dst[m->n];
        if (!cycle) {
            continue;
        }
        // The value that was at to is now at from
        for (int b = 0; b < m->n; b++) {
            if (m->src[b] == to) {
                m->src[b] = from;
                if (m->dst[b] == from) {
                    m->n--;
                    m->src[b] = m->src[m->n];
                    m->dst[b] = m->dst[m->n];
                }
                break;
            }
        }
    }
}

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
        extra += n;
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
    move_set m = {.src = buf, .dst = buf + max, .swaps = buf + 2 * max};
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
        sequence_moves(&m);
        for (int w = 0; w < m.nswaps; w++) {
            int a = m.swaps[2 * w], b = m.swaps[2 * w + 1];
            // The recipe before this word
            int *pairs = NULL;
            int npairs = 0;
            for (int q = 0; q < nrec; q++) {
                npairs += from[q] != home[q];
            }
            int at = 0;
            if (npairs && _PyRegCfg_AddOperands(cfg, 2 * npairs, &at) < 0) {
                goto done;
            }
            pairs = &cfg->operands[at];
            for (int q = 0, p = 0; q < nrec; q++) {
                if (from[q] != home[q]) {
                    pairs[2 * p] = from[q];
                    pairs[2 * p + 1] = home[q];
                    p++;
                }
            }
            _PyRegOp *swap = &ops[nops++];
            *swap = *op;
            swap->opcode = SWAP;
            swap->oparg = Py_ABS(b - a) + 1;
            swap->base = Py_MIN(a, b);
            swap->nread = swap->npop = swap->nimplicit = swap->nout = 0;
            swap->nstage = 0;
            swap->dst = swap->target = -1;
            swap->reader = -1;
            swap->fuse = swap->completion = swap->kwnames = 0;
            swap->flags = _PyReg_SLOT_RECIPE;
            swap->nrecipe = npairs;
            swap->recipe = at;
            for (int q = 0; q < nrec; q++) {
                from[q] = from[q] == a ? b : from[q] == b ? a : from[q];
            }
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
        if (op->fuse > 0) {
            op->fuse = index[op->fuse];
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
