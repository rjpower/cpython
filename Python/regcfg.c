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
 *   1. converts every instruction into an op over virtual registers
 *      (convert());
 *   2. runs the passes, once each, in a fixed order (run_passes());
 *   3. computes liveness and assigns each temporary a frame slot
 *      (_PyRegCfg_Compact()).
 *
 * Debug builds verify the CFG after each pass, and every build checks what
 * the encoding relies on after compaction.
 */

#include "Python.h"

#include "pycore_ir.h"                // _PyIR_FormatOf()
#include "pycore_opcode_metadata.h"   // _PyOpcode_num_popped()
#include "pycore_opcode_utils.h"      // IS_SCOPE_EXIT_OPCODE()
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
    PyMem_Free(cfg);
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
                   _Py_SourceLocation loc, int target, int except)
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
    op->target = OPCODE_HAS_JUMP(opcode) ? target : -1;
    op->except = except;
    op->dst = -1;
    op->depth = -1;
    op->fb = -1;
    cfg->blocks[self].end = cfg->nops;
    return SUCCESS;
}


/* Formats */

const _PyIRFormat *
_PyRegCfg_FormatOf(int opcode)
{
    // The handlers of BINARY_OP's operations share its format
    if (opcode == BINARY_OP) {
        return _PyIR_FormatOf(_PyIR_BINARY_OP_FIRST);
    }
    return _PyIR_FormatOf(opcode);
}

/* Whether op jumps to its target.  END_ASYNC_FOR names the SEND of its
 * loop, for sys.monitoring, but does not go there. */
static inline int
op_jumps(const _PyRegOp *op)
{
    return op->target >= 0 && op->opcode != END_ASYNC_FOR;
}

static inline int
op_ends_flow(int opcode)
{
    return IS_SCOPE_EXIT_OPCODE(opcode) || IS_UNCONDITIONAL_JUMP_OPCODE(opcode);
}

/* The stack depth at the entry of exception handler block h, without the
 * exception and lasti: the depth the unwinder pops to. */
static inline int
handler_base(const _PyRegCfg *cfg, int h)
{
    const _PyRegBlock *hb = &cfg->blocks[h];
    return hb->startdepth - 1 - hb->preserve_lasti;
}


/* Conversion */

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
            blk->falls_through = !op_ends_flow(last);
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
#define PUSH(B, D) \
    do { \
        int b_ = (B), d_ = (D); \
        if (depth[b_] < 0) { \
            depth[b_] = d_; \
            todo[ntodo++] = b_; \
        } \
        else if (depth[b_] != d_) { \
            PyErr_Format(PyExc_SystemError, \
                         "inconsistent stack depth at block %d", b_); \
            goto error; \
        } \
    } while (0)
    if (n > 0) {
        PUSH(0, 0);
    }
    while (ntodo > 0) {
        int b = todo[--ntodo];
        _PyRegBlock *blk = &cfg->blocks[b];
        int d = depth[b];
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            int popped = _PyOpcode_num_popped(op->opcode, op->oparg);
            int pushed = _PyOpcode_num_pushed(op->opcode, op->oparg);
            if (popped < 0 || pushed < 0 || d < popped) {
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
#undef PUSH
    for (int b = 0; b < n; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        int hint = blk->word;
        blk->word = 0;
        blk->reachable = depth[b] >= 0;
        if (depth[b] >= 0) {
            blk->startdepth = depth[b];
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
        int d = 0, low = 0;
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            int popped = _PyOpcode_num_popped(op->opcode, op->oparg);
            int pushed = _PyOpcode_num_pushed(op->opcode, op->oparg);
            if (popped < 0 || pushed < 0) {
                PyErr_SetString(PyExc_SystemError, "invalid opcode");
                goto error;
            }
            low = Py_MIN(low, d - popped);
            d += pushed - popped;
        }
        blk->startdepth = -low;
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

typedef struct {
    _PyRegCfg *cfg;
    _PyRegOp *ops;          // the converted ops
    int nops;
    int aops;
    int *stack;             // the register at each stack position
    int sp;
    int astack;
    int region_op;          // the first op of the current region
    int *def;               // per fresh register, the op that defines it
    int adef;
} converter;

static int
new_fresh(converter *c, int *v)
{
    _PyRegCfg *cfg = c->cfg;
    int k = cfg->nvregs - _PyReg_FRESH0(cfg);
    RETURN_IF_ERROR(grow((void **)&c->def, &c->adef, k + 1, sizeof(int)));
    c->def[k] = c->nops;
    *v = cfg->nvregs++;
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
    }
}

/* Make the register at stack position k canonical.  Stack positions are
 * last in, first out and the conversion renames nothing else, so the
 * canonical temporary of position k is dead since its value was popped,
 * and the fresh register now at k may take its name. */
static int
canonicalise(converter *c, int k)
{
    _PyRegCfg *cfg = c->cfg;
    int v = c->stack[k];
    int canon = _PyReg_CANON(cfg, k);
    if (v == canon) {
        return SUCCESS;
    }
    if (v < _PyReg_FRESH0(cfg)) {
        PyErr_Format(PyExc_SystemError,
                     "register %d at stack position %d", v, k);
        return ERROR;
    }
    int from_op = c->def[v - _PyReg_FRESH0(cfg)];
    rename_ops(cfg, c->ops, Py_MAX(from_op, c->region_op), c->nops,
                         v, canon);
    c->stack[k] = canon;
    return SUCCESS;
}

static int
canonicalise_below(converter *c, int depth)
{
    for (int k = 0; k < depth; k++) {
        RETURN_IF_ERROR(canonicalise(c, k));
    }
    return SUCCESS;
}

static int
add_operands(_PyRegCfg *cfg, int n, int *index)
{
    RETURN_IF_ERROR(grow((void **)&cfg->operands, &cfg->aoperands,
                         cfg->noperands + n, sizeof(int)));
    *index = cfg->noperands;
    cfg->noperands += n;
    return SUCCESS;
}

/* Convert the stack instruction instr into an op. */
static int
convert_instr(converter *c, const _PyRegOp *instr)
{
    _PyRegCfg *cfg = c->cfg;
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(instr->opcode);
    int window = (fmt->flags & _PyIR_WINDOW) != 0;
    int popped = _PyOpcode_num_popped(instr->opcode, instr->oparg);
    // A scope exit leaves nothing behind for this frame: RETURN_VALUE
    // pushes its result to the caller's, and RERAISE does not go on
    int pushed = IS_SCOPE_EXIT_OPCODE(instr->opcode)
        ? 0 : _PyOpcode_num_pushed(instr->opcode, instr->oparg);
    int depth = c->sp;
    assert(popped >= 0 && pushed >= 0 && popped <= depth);
    if (instr->except >= 0) {
        RETURN_IF_ERROR(canonicalise_below(c, handler_base(cfg, instr->except)));
    }
    int jumps = op_jumps(instr);
    if (jumps) {
        // The target finds the stack canonical.  The window of FOR_ITER
        // and SEND is the stack they leave there.
        RETURN_IF_ERROR(canonicalise_below(c, window ? depth : depth - popped));
    }
    int nread = window ? popped : Py_MAX(popped, fmt->ninputs);
    if (nread > depth) {
        PyErr_Format(PyExc_SystemError, "stack underflow at line %d",
                     instr->loc.lineno);
        return ERROR;
    }
    int nimplicit = IS_RAISE_OPCODE(instr->opcode) ? depth - popped : 0;
    int in, out;
    RETURN_IF_ERROR(add_operands(cfg, nread + nimplicit, &in));
    RETURN_IF_ERROR(add_operands(cfg, pushed, &out));
    for (int k = 0; k < nread; k++) {
        cfg->operands[in + k] = c->stack[depth - nread + k];
    }
    for (int k = 0; k < nimplicit; k++) {
        cfg->operands[in + nread + k] = c->stack[k];
    }
    // Register style: the outputs below dst pass their inputs through
    int through = 0;
    if (!window) {
        through = fmt->dst == _PyIR_NO_DST ? pushed : popped + fmt->dst;
        if (through < 0 || through > pushed) {
            PyErr_Format(PyExc_SystemError, "bad format for opcode %d",
                         instr->opcode);
            return ERROR;
        }
    }
    RETURN_IF_ERROR(grow((void **)&c->stack, &c->astack, depth + pushed + 1,
                         sizeof(int)));
    int base = depth - popped;
    for (int j = 0; j < pushed; j++) {
        int v;
        if (j < through) {
            v = c->stack[base + j];
        }
        else if (jumps) {
            v = _PyReg_CANON(cfg, base + j);
        }
        else {
            RETURN_IF_ERROR(new_fresh(c, &v));
        }
        cfg->operands[out + j] = v;
    }
    for (int j = 0; j < pushed; j++) {
        c->stack[base + j] = cfg->operands[out + j];
    }
    c->sp = base + pushed;

    RETURN_IF_ERROR(grow((void **)&c->ops, &c->aops, c->nops + 1,
                         sizeof(_PyRegOp)));
    _PyRegOp *op = &c->ops[c->nops++];
    *op = *instr;
    op->nread = (int16_t)nread;
    op->npop = (int16_t)popped;
    op->nimplicit = (int16_t)nimplicit;
    op->nout = (int16_t)pushed;
    op->in = in;
    op->out = out;
    op->depth = depth;
    return SUCCESS;
}

static int
convert(_PyRegCfg *cfg)
{
    RETURN_IF_ERROR(mark_blocks(cfg));
    resolve_next_locations(cfg);
    converter c = {.cfg = cfg};
    int err = SUCCESS;
    for (int b = 0; b < cfg->nblocks && err == SUCCESS; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        int start = blk->start, end = blk->end;
        if (!blk->continued) {
            c.region_op = c.nops;
            err = grow((void **)&c.stack, &c.astack, blk->startdepth + 1,
                       sizeof(int));
            for (int k = 0; err == SUCCESS && k < blk->startdepth; k++) {
                c.stack[k] = _PyReg_CANON(cfg, k);
            }
            c.sp = blk->startdepth;
        }
        else if (c.sp != blk->startdepth) {
            PyErr_Format(PyExc_SystemError,
                         "inconsistent stack depth at block %d", b);
            err = ERROR;
        }
        blk->start = c.nops;
        for (int i = start; err == SUCCESS && i < end; i++) {
            err = convert_instr(&c, &cfg->ops[i]);
        }
        blk->end = c.nops;
        if (err == SUCCESS && blk->falls_through
            && !cfg->blocks[b + 1].continued)
        {
            err = canonicalise_below(&c, c.sp);
        }
    }
    PyMem_Free(c.stack);
    PyMem_Free(c.def);
    if (err != SUCCESS) {
        PyMem_Free(c.ops);
        return ERROR;
    }
    PyMem_Free(cfg->ops);
    cfg->ops = c.ops;
    cfg->nops = c.nops;
    cfg->aops = c.aops;
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
    return IR_MAX_SLOTS + (v - _PyReg_FRESH0(cfg));
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

/* The live set before op, given live, the set after it, in place.  When
 * last is not NULL, record which operands the op reads last or writes
 * dead. */
static void
transfer(_PyRegCfg *cfg, _PyRegLiveness *lv, _PyRegOp *op, uint64_t *live,
         uint8_t *last)
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
    // they are live while the op runs
    if (op->except >= 0) {
        int hd = handler_base(cfg, op->except);
        uint64_t *h = lv->live_in[op->except];
        for (int k = 0; k < hd; k++) {
            if (BIT_GET(h, k)) {
                BIT_SET(live, k);
            }
        }
    }
}

int
_PyRegCfg_Liveness(_PyRegCfg *cfg, _PyRegLiveness *lv)
{
    memset(lv, 0, sizeof(*lv));
    int n = cfg->nblocks;
    int nbits = IR_MAX_SLOTS + (cfg->nvregs - _PyReg_FRESH0(cfg));
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
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
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
        slots->dst = op->dst >= 0 ? op->dst : slots->base + fmt->dst;
        return;
    }
    slots->base = top - op->npop;
    int first = op->nread - fmt->ninputs;
    for (int k = 0; k < fmt->ninputs && k < 4; k++) {
        slots->in[k] = first + k >= 0 ? _PyRegCfg_Slot(cfg, in[first + k]) : 0;
    }
    if (op->dst >= 0) {
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

static int
slot_error(const _PyRegOp *op, const char *what)
{
    PyErr_Format(PyExc_SystemError, "register allocation: %s at line %d "
                 "(opcode %d)", what, op->loc.lineno, op->opcode);
    return ERROR;
}

typedef struct {
    int *occ;               // per slot less nlocalsplus, its register or -1
    int top;                // the slots from top up are free
    int lenient;            // unreachable code, which never runs
} occupancy;

static inline void
occ_free(occupancy *o, int s, int v)
{
    if (o->occ[s] == v) {
        o->occ[s] = -1;
        while (o->top > 0 && o->occ[o->top - 1] < 0) {
            o->top--;
        }
    }
}

static inline int
occ_take(occupancy *o, int s, int v, const _PyRegOp *op)
{
    if (s < 0 || s >= IR_MAX_SLOTS) {
        return slot_error(op, "too many temporaries");
    }
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

static int
assign(_PyRegCfg *cfg, occupancy *o, int v, int s, const _PyRegOp *op)
{
    int *slot = &cfg->slot[v - cfg->nlocalsplus];
    if (*slot >= 0 && *slot != s && !o->lenient) {
        return slot_error(op, "a temporary in two slots");
    }
    *slot = s;
    return SUCCESS;
}

static int
compact_op(_PyRegCfg *cfg, occupancy *o, _PyRegOp *op)
{
    int nl = cfg->nlocalsplus;
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
    int *in = _PyReg_IN(cfg, op);
    int *out = _PyReg_OUT(cfg, op);
    int nin = op->nread + op->nimplicit;
    int window = (fmt->flags & _PyIR_WINDOW) != 0;
    // A window without inputs starts at the stack top, above the values a
    // raise leaves for the unwinder
    int top_before = o->top;
    int stored = window ? fmt->dst : op->npop + fmt->dst;
    // Inputs that the op takes free their slots: the popped ones, and
    // those a raise leaves for the unwinder.  A value that is no longer
    // read but not yet taken still holds its slot, as in a loop that
    // never exits.
    for (int k = op->nread - op->npop; k < nin; k++) {
        int v = in[k];
        if (v < nl) {
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
    if (window) {
        int base;
        if (op->nread > 0) {
            base = _PyRegCfg_Slot(cfg, in[0]);
            for (int k = 1; k < op->nread; k++) {
                if (in[k] < nl || _PyRegCfg_Slot(cfg, in[k]) != base + k) {
                    return slot_error(op, "a window that is not consecutive");
                }
            }
            if (in[0] < nl) {
                return slot_error(op, "a local in a window");
            }
        }
        else {
            base = nl + top_before;
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
        int through = fmt->dst == _PyIR_NO_DST ? op->nout
                                               : op->npop + fmt->dst;
        if (through < op->nout && out[through] >= nl) {
            int dst;
            int v0 = out[through];
            if (_PyReg_IS_CANON(cfg, v0)) {
                dst = _PyRegCfg_Slot(cfg, v0) - through;
            }
            else {
                dst = nl + o->top - through;
                // An output must not land on an input that the handler
                // has not given up yet: only the input at its own stack
                // position, which the handler consumes before writing it.
                int bump = 0;
                for (int j = through; j < op->nout && !bump; j++) {
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
                if (bump) {
                    int high = nl + o->top;
                    for (int k = 0; k < op->nread; k++) {
                        if (in[k] >= nl) {
                            high = Py_MAX(high, _PyRegCfg_Slot(cfg, in[k]) + 1);
                        }
                    }
                    dst = high - through;
                }
            }
            for (int j = through; j < op->nout; j++) {
                RETURN_IF_ERROR(assign(cfg, o, out[j], dst + j, op));
            }
        }
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
        if (op->dst >= 0 && j == stored) {
            continue;
        }
        RETURN_IF_ERROR(occ_take(o, s - nl, v, op));
    }
    return SUCCESS;
}

static int run_passes(_PyRegCfg *cfg);

int
_PyRegCfg_Compact(_PyRegCfg *cfg)
{
    int nl = cfg->nlocalsplus;
    int nregs = cfg->nvregs - nl;
    PyMem_Free(cfg->slot);
    cfg->slot = PyMem_Malloc(Py_MAX(nregs, 1) * sizeof(int));
    int *occ = PyMem_Malloc(IR_MAX_SLOTS * sizeof(int));
    if (cfg->slot == NULL || occ == NULL) {
        PyMem_Free(occ);
        PyErr_NoMemory();
        return ERROR;
    }
    for (int i = 0; i < nregs; i++) {
        cfg->slot[i] = i < IR_MAX_SLOTS ? nl + i : -1;
    }
    cfg->ntemps = 0;
    occupancy o = {.occ = occ, .top = 0};
    int err = SUCCESS;
    for (int b = 0; b < cfg->nblocks && err == SUCCESS; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        o.lenient = !blk->reachable;
        if (!blk->continued) {
            for (int s = 0; s < IR_MAX_SLOTS; s++) {
                occ[s] = -1;
            }
            // The stack at a region's entry is canonical, and every value
            // on it holds its slot until an op takes it
            o.top = 0;
            for (int k = 0; k < blk->startdepth; k++) {
                occ[k] = _PyReg_CANON(cfg, k);
                o.top = k + 1;
            }
        }
        for (int i = blk->start; err == SUCCESS && i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode != _PyReg_DEAD) {
                err = compact_op(cfg, &o, op);
            }
        }
    }
    PyMem_Free(occ);
    return err;
}


/* The verifier.  Before compaction it checks the registers: every input
 * is defined on every path, windows hold temporaries, and only canonical
 * temporaries cross region boundaries.  After compaction it checks the
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

/* The calls that may push the callee's frame over their window
 * (CALL_PY_EXACT_ARGS and CALL_BOUND_METHOD_EXACT_ARGS, Python/bytecodes.c) */
static inline int
overlapping_call(int opcode)
{
    return opcode == CALL;
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
                  : IR_MAX_SLOTS;
        for (int bit = 0; bit < lv.nwords * 64; bit++) {
            if (!BIT_GET(lin, bit)) {
                continue;
            }
            if (bit >= limit && (b == 0 || blk->is_handler)) {
                err = verify_error(after, NULL, "a temporary used before "
                                   "it is defined");
                break;
            }
            if (!blk->continued && bit >= IR_MAX_SLOTS) {
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
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
            int *in = _PyReg_IN(cfg, op);
            int *out = _PyReg_OUT(cfg, op);
            // A value that nothing reads is not an error: a loop without an
            // exit, as in a finally that never ends, keeps what is below it
            // until the frame is cleared or an exception unwinds it.
            if (fmt->flags & _PyIR_WINDOW) {
                for (int k = 0; k < op->nread; k++) {
                    if (in[k] < nl) {
                        err = verify_error(after, op, "a local in a window");
                    }
                    else if (physical && _PyRegCfg_Slot(cfg, in[k])
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
            if (op->dst >= 0 && !(fmt->flags & _PyIR_STORE_DST)) {
                err = verify_error(after, op, "a local dst");
            }
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
                int *in = _PyReg_IN(cfg, op);
                if (op->except >= 0) {
                    int hd = handler_base(cfg, op->except);
                    int first = nl + hd;
                    for (int bit = 0; bit < lv.nwords * 64; bit++) {
                        if (!BIT_GET(work, bit)) {
                            continue;
                        }
                        int v = bit < IR_MAX_SLOTS ? nl + bit
                                : _PyReg_FRESH0(cfg) + bit - IR_MAX_SLOTS;
                        int s = _PyRegCfg_Slot(cfg, v);
                        int is_out = 0;
                        for (int j = 0; j < op->nout; j++) {
                            is_out |= out[j] == v;
                        }
                        if (is_out) {
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
                if (overlapping_call(op->opcode) && op->nread > 0) {
                    int base = _PyRegCfg_Slot(cfg, in[0]);
                    for (int bit = 0; bit < lv.nwords * 64; bit++) {
                        if (!BIT_GET(work, bit)) {
                            continue;
                        }
                        int v = bit < IR_MAX_SLOTS ? nl + bit
                                : _PyReg_FRESH0(cfg) + bit - IR_MAX_SLOTS;
                        int is_out = 0;
                        for (int j = 0; j < op->nout; j++) {
                            is_out |= out[j] == v;
                        }
                        if (!is_out && _PyRegCfg_Slot(cfg, v) >= base) {
                            err = verify_error(after, op, "a temporary live "
                                               "across a call above its "
                                               "window");
                            break;
                        }
                    }
                }
                transfer(cfg, &lv, op, work, NULL);
            }
        }
    }
done:
    PyMem_Free(work);
    _PyRegCfg_FreeLiveness(&lv);
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

typedef int (*pass_fn)(_PyRegCfg *);

static const struct {
    const char *name;
    pass_fn fn;
} passes[] = {
    {"copy to load", _PyRegPass_CopyToLoad},
    {"swap renaming", _PyRegPass_SwapRename},
    {"copy propagation", _PyRegPass_CopyProp},
    {"store elimination", _PyRegPass_StoreElim},
    {"pop top", _PyRegPass_PopTop},
};

static int
run_passes(_PyRegCfg *cfg)
{
    DUMP(cfg, "conversion");
#ifdef Py_DEBUG
    RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, "conversion", 0));
#endif
    for (size_t i = 0; i < Py_ARRAY_LENGTH(passes); i++) {
        RETURN_IF_ERROR(passes[i].fn(cfg));
#ifdef Py_DEBUG
        RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, passes[i].name, 0));
#endif
    }
    return SUCCESS;
}

int
_PyRegCfg_Build(_PyRegCfg *cfg)
{
    RETURN_IF_ERROR(convert(cfg));
    RETURN_IF_ERROR(run_passes(cfg));
    int err = _PyRegCfg_Compact(cfg);
    DUMP(cfg, "compaction");
    RETURN_IF_ERROR(err);
#ifdef Py_DEBUG
    RETURN_IF_ERROR(_PyRegCfg_Verify(cfg, "compaction", 1));
#endif
    return SUCCESS;
}
