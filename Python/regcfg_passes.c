/* The passes over the register CFG (boil/design/07-compiler-passes.md).
 *
 * Each pass is a function over the blocks of a _PyRegCfg; the driver in
 * Python/regcfg.c runs them once each, in order, and debug builds verify
 * the CFG after each.  Passes rewrite fresh temporaries only, which live
 * within one region (Include/internal/pycore_regcfg.h): a canonical
 * temporary carries a value across a block edge or to an exception
 * handler, so its definitions and uses stay as the conversion made them.
 */

#include "Python.h"

#include "pycore_ir.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_regcfg.h"

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) == -1) {        \
        return ERROR;       \
    }

static inline int
is_fresh(const _PyRegCfg *cfg, int v)
{
    return v >= _PyReg_FRESH0(cfg);
}

static int reads_count(const _PyRegCfg *cfg, const _PyRegOp *op, int v);


/* The regions of the reachable blocks, as ranges of ops: calls fn on each,
 * last first, so that a pass that looks forward from an op sees the ops
 * after it already rewritten. */
static int
for_each_region_backward(_PyRegCfg *cfg, int (*fn)(_PyRegCfg *, int, int))
{
    int end = cfg->nblocks;
    for (int b = cfg->nblocks - 1; b >= 0; b--) {
        if (cfg->blocks[b].continued) {
            continue;
        }
        // Blocks b .. end - 1 form a region
        int last = end - 1;
        while (last >= b && !cfg->blocks[last].reachable) {
            last--;
        }
        if (cfg->blocks[b].reachable && last >= b) {
            RETURN_IF_ERROR(fn(cfg, cfg->blocks[b].start,
                               cfg->blocks[last].end));
        }
        end = b;
    }
    return SUCCESS;
}


/* SWAP by renaming (pass c).
 *
 * A SWAP whose outputs one register-style op reads, all of them and
 * nothing else, as STORE_ATTR and STORE_SUBSCR do after an augmented
 * assignment, goes: that op reads the SWAP's inputs in the swapped order.
 * The reader must be on the SWAP's line with every op in between, so that
 * no line starts while the values sit out of stack order: f_lineno
 * assignment pops the stack by depth.  A SWAP that feeds a window, or
 * whose values go different ways, stays. */

static int
swap_rename_region(_PyRegCfg *cfg, int start, int end)
{
    for (int i = end - 1; i >= start; i--) {
        _PyRegOp *swap = &cfg->ops[i];
        if (swap->opcode != SWAP) {
            continue;
        }
        int n = swap->nout;
        const int *sin = _PyReg_IN(cfg, swap);
        const int *sout = _PyReg_OUT(cfg, swap);
        int ok = n >= 2 && swap->nread == n;
        for (int j = 0; ok && j < n; j++) {
            ok = is_fresh(cfg, sout[j]);
        }
        // The first op after the SWAP that reads any of its outputs
        _PyRegOp *reader = NULL;
        for (int r = i + 1; ok && r < end; r++) {
            _PyRegOp *op = &cfg->ops[r];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            int uses = 0;
            for (int j = 0; j < n; j++) {
                uses += reads_count(cfg, op, sout[j]);
            }
            if (reader == NULL) {
                if (op->loc.lineno != swap->loc.lineno) {
                    ok = 0;
                }
                else if (uses > 0) {
                    reader = op;
                    ok = uses == n;
                }
            }
            else if (uses > 0) {
                ok = 0;
            }
        }
        if (!ok || reader == NULL) {
            continue;
        }
        const _PyIRFormat *fmt = _PyRegCfg_FormatOf(reader->opcode);
        if (fmt->flags & _PyIR_WINDOW) {
            continue;
        }
        int *in = _PyReg_IN(cfg, reader);
        // Each output once, among the inputs the reader pops
        int popped = 1;
        for (int k = 0; k < reader->nread + reader->nimplicit; k++) {
            for (int j = 0; j < n; j++) {
                if (in[k] == sout[j] && k < reader->nread - reader->npop) {
                    popped = 0;
                }
            }
        }
        if (!popped) {
            continue;
        }
        for (int k = 0; k < reader->nread; k++) {
            for (int j = 0; j < n; j++) {
                if (in[k] == sout[j]) {
                    int from = j == 0 ? n - 1 : j == n - 1 ? 0 : j;
                    in[k] = sin[from];
                    break;
                }
            }
        }
        swap->opcode = _PyReg_DEAD;
    }
    return SUCCESS;
}

int
_PyRegPass_SwapRename(_PyRegCfg *cfg)
{
    return for_each_region_backward(cfg, swap_rename_region);
}


/* Copy propagation (pass a).
 *
 * A load of a local whose value an op reads through a field that may name
 * a local becomes that field: the op reads the local in place, borrowed.
 * The load may sit under other values, such as a constant, a global or a
 * call's result, as long as nothing writes the local in between, which the
 * kill on every write checks.  A load keeps its op if the line changes
 * before its reader, so that line events, and the stack that f_lineno
 * assignment sees at a line start, stay where they were.  A call in
 * between may still rebind the local through frame.f_locals; the reader
 * then sees the new value, where the stack would have kept the old one.
 *
 * Only LOAD_FAST_BORROW folds: optimize_load_fast() makes a load strong
 * when its value outlives the local's reference or escapes, as into a
 * store or a container, and a handler that reads a field as borrowed
 * relies on neither happening.  The value must be the reader's popped
 * input and nothing else may read it, so the temporary disappears with
 * the load. */

typedef struct {
    _PyRegOp *load;
    int reg;                // the fresh temporary the load defines
} pending_load;

/* The field of op that reads in[k], or -1 if in[k] is not a field that
 * may name a local and that the op consumes. */
static int
borrow_field(const _PyRegCfg *cfg, const _PyRegOp *op, int k)
{
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
    if ((fmt->flags & _PyIR_WINDOW) || fmt->borrow_mask == 0) {
        return -1;
    }
    int f = k - (op->nread - fmt->ninputs);
    if (k >= op->nread || k < op->nread - op->npop || f < 0
        || !(fmt->borrow_mask & (1 << f)))
    {
        return -1;
    }
    return f;
}

/* The local op writes, besides through dst, or -1 */
static int
written_local(const _PyRegOp *op)
{
    switch (op->opcode) {
        case STORE_FAST:
        case DELETE_FAST:
        case LOAD_FAST_AND_CLEAR:
            return op->oparg;
        default:
            return -1;
    }
}

static int
reads_count(const _PyRegCfg *cfg, const _PyRegOp *op, int v)
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

int
_PyRegPass_CopyProp(_PyRegCfg *cfg)
{
    pending_load *pend = PyMem_Malloc(Py_MAX(cfg->nops, 1) * sizeof(pending_load));
    if (pend == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    int npend = 0;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->continued || !blk->reachable) {
            npend = 0;
        }
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            int *in = _PyReg_IN(cfg, op);
            int written = written_local(op);
            int n = 0;
            for (int p = 0; p < npend; p++) {
                _PyRegOp *load = pend[p].load;
                int v = pend[p].reg;
                if (load->loc.lineno != op->loc.lineno) {
                    continue;
                }
                int uses = reads_count(cfg, op, v);
                if (uses == 0) {
                    // Nothing here reads it; the local must keep its value
                    if (load->oparg != written && load->oparg != op->dst) {
                        pend[n++] = pend[p];
                    }
                    continue;
                }
                if (uses == 1) {
                    for (int k = 0; k < op->nread; k++) {
                        if (in[k] == v && borrow_field(cfg, op, k) >= 0) {
                            in[k] = load->oparg;
                            load->opcode = _PyReg_DEAD;
                            load->flags |= _PyReg_LOAD_FOLDED;
                            load->reader = i;
                            load->reader_k = k;
                        }
                    }
                }
            }
            npend = n;
            if (op->opcode == LOAD_FAST_BORROW
                && is_fresh(cfg, _PyReg_OUT(cfg, op)[0]))
            {
                pend[npend].load = op;
                pend[npend].reg = _PyReg_OUT(cfg, op)[0];
                npend++;
            }
        }
    }
    PyMem_Free(pend);
    return SUCCESS;
}


/* Store elimination (pass b).
 *
 * A STORE_FAST z of the value an op produces, where the op's format lets
 * dst name a local: the op writes z, and its handler releases z's old
 * value as STORE_FAST would.  The ops in between, if any, must not read or
 * write z, must not read the value, and must not raise, jump or run other
 * code, so that nothing can see z change early: if the producer itself
 * raises, as in z = f(z), z keeps its old value. */

/* Whether op reads or writes local z */
static int
touches_local(const _PyRegCfg *cfg, const _PyRegOp *op, int z)
{
    if (op->dst == z || written_local(op) == z) {
        return 1;
    }
    switch (op->opcode) {
        case LOAD_FAST:
        case LOAD_FAST_BORROW:
        case LOAD_FAST_CHECK:
            if (op->oparg == z) {
                return 1;
            }
            break;
        default:
            break;
    }
    return reads_count(cfg, op, z) > 0;
}

/* The op that may write the value of store into its local, or NULL */
static _PyRegOp *
store_producer(_PyRegCfg *cfg, int region_start, int s)
{
    _PyRegOp *store = &cfg->ops[s];
    int value = _PyReg_IN(cfg, store)[0];
    int z = store->oparg;
    if (value < cfg->nlocalsplus) {
        return NULL;
    }
    for (int i = s - 1; i >= region_start; i--) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        const int *out = _PyReg_OUT(cfg, op);
        if (op->nout > 0 && out[op->nout - 1] == value) {
            return op;
        }
        // An op that writes a local releases its old value, which may run
        // other code, as STORE_FAST does
        if (op->loc.lineno != store->loc.lineno || op->target >= 0
            || op->dst >= 0 || written_local(op) >= 0
            || OPCODE_HAS_ERROR(op->opcode) || OPCODE_HAS_ESCAPES(op->opcode)
            || reads_count(cfg, op, value) > 0 || touches_local(cfg, op, z))
        {
            return NULL;
        }
    }
    return NULL;
}

int
_PyRegPass_StoreElim(_PyRegCfg *cfg)
{
    int region_start = 0;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->continued) {
            region_start = blk->start;
        }
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode != STORE_FAST) {
                continue;
            }
            _PyRegOp *p = store_producer(cfg, region_start, i);
            if (p == NULL || p->opcode == LOAD_FAST_BORROW || p->dst >= 0
                || p->loc.lineno != op->loc.lineno)
            {
                continue;
            }
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(p->opcode);
            if (!(fmt->flags & _PyIR_STORE_DST) || fmt->dst == _PyIR_NO_DST) {
                continue;
            }
            int j = fmt->flags & _PyIR_WINDOW ? fmt->dst : p->npop + fmt->dst;
            // The result is the producer's last output, on top
            if (j != p->nout - 1) {
                continue;
            }
            p->dst = op->oparg;
            // A jump's outputs stay canonical for its target, where the
            // handler leaves the slot of the result NULL
            if (!(p->target >= 0)) {
                _PyReg_OUT(cfg, p)[j] = op->oparg;
            }
            op->opcode = _PyReg_DEAD;
            op->flags |= _PyReg_STORE_FOLDED;
        }
    }
    return SUCCESS;
}


/* POP_TOP of a pure value (pass c).
 *
 * A POP_TOP of a value that a pure op made from locals and constants
 * alone, which cannot fail and runs no other code, goes with its
 * producer: the value never exists.  Pure is the DSL's annotation; the
 * value must have no other reader.  Other POP_TOPs stay, as the CLEAR
 * that releases their value.  If the pair is all there is of its line, a
 * NOP keeps the line's event. */

static _PyRegOp *
live_neighbour(_PyRegCfg *cfg, int start, int end, int i, int step)
{
    for (int j = i + step; j >= start && j < end; j += step) {
        if (cfg->ops[j].opcode != _PyReg_DEAD) {
            return &cfg->ops[j];
        }
    }
    return NULL;
}

static int
pop_top_region(_PyRegCfg *cfg, int start, int end)
{
    int nl = cfg->nlocalsplus;
    for (int i = start; i < end; i++) {
        _PyRegOp *pop = &cfg->ops[i];
        if (pop->opcode != POP_TOP) {
            continue;
        }
        int v = _PyReg_IN(cfg, pop)[0];
        if (!is_fresh(cfg, v)) {
            continue;
        }
        // The producer, and no other reader in between
        int p = i - 1;
        int other = 0;
        for (; p >= start; p--) {
            _PyRegOp *op = &cfg->ops[p];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            if (op->nout == 1 && _PyReg_OUT(cfg, op)[0] == v) {
                break;
            }
            other |= reads_count(cfg, op, v) > 0;
        }
        if (p < start || other) {
            continue;
        }
        _PyRegOp *prod = &cfg->ops[p];
        if (!OPCODE_HAS_PURE(prod->opcode) || OPCODE_HAS_ERROR(prod->opcode)
            || OPCODE_HAS_ESCAPES(prod->opcode) || prod->dst >= 0)
        {
            continue;
        }
        const int *in = _PyReg_IN(cfg, prod);
        int locals_only = 1;
        for (int k = 0; k < prod->nread + prod->nimplicit; k++) {
            locals_only &= in[k] < nl;
        }
        if (!locals_only) {
            continue;
        }
        _PyRegOp *before = live_neighbour(cfg, start, end, p, -1);
        _PyRegOp *after = live_neighbour(cfg, start, end, i, 1);
        int line = prod->loc.lineno;
        int keeps_line = (before != NULL && before->loc.lineno == line)
                         || (after != NULL && after->loc.lineno == line);
        prod->opcode = _PyReg_DEAD;
        if (keeps_line && pop->loc.lineno == line) {
            pop->opcode = _PyReg_DEAD;
        }
        else {
            pop->opcode = NOP;
            pop->oparg = 0;
            pop->loc = prod->loc;
            pop->nread = pop->npop = 0;
        }
    }
    return SUCCESS;
}

int
_PyRegPass_PopTop(_PyRegCfg *cfg)
{
    return for_each_region_backward(cfg, pop_top_region);
}


/* Truth folding in value context (pass d).
 *
 * "a.x and b" tests a copy of its first value and pops the value where it
 * goes on:
 *
 *     COPY 1; POP_JUMP_IF_FALSE L; NOT_TAKEN; POP_TOP; <b>; L:
 *
 * JUMP_IF_FALSE_OR_POP does all three: it keeps the value where it jumps
 * and releases it where it falls through.  The POP_TOP must be the next op
 * on the fall-through path, which only the jump reaches, and all three on
 * one line, so that no line's event goes.  A value loaded from a local
 * does not come here: the flowgraph turns its COPY into a second load,
 * which folds into the jump. */

static int
truth_fold_region(_PyRegCfg *cfg, int start, int end)
{
    for (int i = start; i < end; i++) {
        _PyRegOp *copy = &cfg->ops[i];
        if (copy->opcode != COPY || copy->oparg != 1 || copy->nout != 2) {
            continue;
        }
        const int *cin = _PyReg_IN(cfg, copy);
        int *cout = _PyReg_OUT(cfg, copy);
        _PyRegOp *jump = live_neighbour(cfg, start, end, i, 1);
        if (jump == NULL || jump->loc.lineno != copy->loc.lineno
            || (jump->opcode != POP_JUMP_IF_FALSE
                && jump->opcode != POP_JUMP_IF_TRUE)
            || _PyReg_IN(cfg, jump)[0] != cout[1]
            || !is_fresh(cfg, cin[0]) || !_PyReg_IS_CANON(cfg, cout[0]))
        {
            continue;
        }
        int j = (int)(jump - cfg->ops);
        _PyRegOp *next = live_neighbour(cfg, start, end, j, 1);
        if (next != NULL && next->opcode == NOT_TAKEN) {
            next = live_neighbour(cfg, start, end, (int)(next - cfg->ops), 1);
        }
        if (next == NULL || next->opcode != POP_TOP
            || next->loc.lineno != copy->loc.lineno
            || _PyReg_IN(cfg, next)[0] != cout[0])
        {
            continue;
        }
        copy->opcode = jump->opcode == POP_JUMP_IF_FALSE
            ? JUMP_IF_FALSE_OR_POP : JUMP_IF_TRUE_OR_POP;
        copy->oparg = jump->oparg;
        copy->target = jump->target;
        copy->except = jump->except;
        copy->loc = jump->loc;
        copy->npop = 1;
        copy->nout = 1;
        copy->flags |= _PyReg_JUMP_OUTPUT;
        jump->opcode = _PyReg_DEAD;
        next->opcode = _PyReg_DEAD;
    }
    return SUCCESS;
}

int
_PyRegPass_TruthFold(_PyRegCfg *cfg)
{
    return for_each_region_backward(cfg, truth_fold_region);
}
