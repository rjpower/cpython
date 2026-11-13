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
#include "pycore_list.h"            // PyList_GET_VALUE()
#include "pycore_regcfg.h"
#include "pycore_value.h"           // PyValue_FromInlineInt()

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
 * a local becomes that field: the op reads the local in place, borrowed,
 * when it runs rather than when the load would have.  So nothing in
 * between may change the local.  A pending load dies at a write to the
 * local, and at an op that may run Python code, which may rebind it
 * through frame.f_locals (_PyIR_MayRunPython()): a call, a generic
 * LOAD_GLOBAL or LOAD_ATTR, a binary op, anything whose family the
 * generator marks _PyIR_RUNS_CODE (Tools/cases_generator/ir_generator.py,
 * runs_code()), and a write that releases the old value of a local.  The
 * verifier checks the same rule (check_folded_load() in
 * Python/regcfg.c).  The load may sit
 * under constants, other locals, cells and the like.  A load also keeps its
 * op if the line changes before its reader, so that line events, and the
 * stack that f_lineno assignment sees at a line start, stay where they
 * were.
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
    if (fmt->borrow_mask == 0) {
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

/* The local op writes, besides through dst, or -1: its oparg, if the
 * oparg's role says the handler stores to it, clears it or moves it out */
static int
written_local(const _PyRegOp *op)
{
    int role = _PyIR_OpargRole(_PyRegCfg_FormatOf(op->opcode));
    if (role == _PyIR_ROLE_LOCAL_OUT || role == _PyIR_ROLE_LOCAL_CLEAR
        || role == _PyIR_ROLE_LOCAL_MOVE)
    {
        return op->oparg;
    }
    return -1;
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

static int keep_across_code = 0;

int
_PyRegPass_SetKeepAcrossCode(int keep)
{
    int old = keep_across_code;
    keep_across_code = keep;
    return old;
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
            int runs_code = !keep_across_code
                && _PyIR_MayRunPython(_PyRegCfg_FormatOf(op->opcode),
                                      op->dst >= 0);
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
                    if (load->oparg != written && load->oparg != op->dst
                        && !runs_code)
                    {
                        pend[n++] = pend[p];
                    }
                    continue;
                }
                if (uses == 1 && load->except == op->except) {
                    for (int k = 0; k < op->nread; k++) {
                        if (in[k] == v && borrow_field(cfg, op, k) >= 0) {
                            in[k] = load->oparg;
                            op->flags |= load->flags & _PyReg_SOURCE_ORIGIN;
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
    if (_PyRegOp_WritesLocal(op, z) || written_local(op) == z) {
        return 1;
    }
    // A handler that reads the slot its oparg names
    int role = _PyIR_OpargRole(_PyRegCfg_FormatOf(op->opcode));
    if ((role == _PyIR_ROLE_LOCAL_IN || role == _PyIR_ROLE_CELL)
        && op->oparg == z)
    {
        return 1;
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
            || (_PyRegCfg_FormatOf(op->opcode)->effects
                & (_PyIR_RAISES | _PyIR_ESCAPES))
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
            if (p == NULL || p->opcode == LOAD_FAST_BORROW
                || p->opcode == PUSH_NULL || p->dst >= 0
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


/* Unpacks to locals (pass b2).
 *
 * An UNPACK_SEQUENCE or UNPACK_EX whose n live successors are STORE_FASTs
 * of its outputs on its own line, in element order, into n consecutive
 * locals t, t+1, ..., becomes a word whose dst field is t: the handler
 * stores the element i to local t+i and the STORE_FASTs die.  Any other
 * targets keep the stores. */

static _PyRegOp *live_neighbour(_PyRegCfg *cfg, int start, int end, int i,
                                int step);

enum {
    UNPACK_OK, UNPACK_NOT_STORE_FAST, UNPACK_OTHER_LINE, UNPACK_WRONG_ORDER,
    UNPACK_NOT_CONSECUTIVE
};

static int
unpack_locals_site(_PyRegCfg *cfg, int start, int end, _PyRegOp *op,
                   _PyRegOp **stores)
{
    int n = op->nout;
    const int *out = _PyReg_OUT(cfg, op);
    _PyRegOp *prev = op;
    int first = -1;
    for (int k = 0; k < n; k++) {
        _PyRegOp *s = live_neighbour(cfg, start, end, (int)(prev - cfg->ops), 1);
        if (s == NULL || s->opcode != STORE_FAST) {
            return UNPACK_NOT_STORE_FAST;
        }
        if (s->loc.lineno != op->loc.lineno) {
            return UNPACK_OTHER_LINE;
        }
        if (_PyReg_IN(cfg, s)[0] != out[n - 1 - k]) {
            return UNPACK_WRONG_ORDER;
        }
        if (k == 0) {
            first = s->oparg;
        }
        else if (s->oparg != first + k) {
            return UNPACK_NOT_CONSECUTIVE;
        }
        stores[k] = s;
        prev = s;
    }
    return UNPACK_OK;
}

static int
unpack_locals_region(_PyRegCfg *cfg, int start, int end)
{
    _PyRegOp **stores = NULL;
    int astores = 0;
    for (int i = start; i < end; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if ((op->opcode != UNPACK_SEQUENCE && op->opcode != UNPACK_EX)
            || op->dst >= 0 || op->nout < 1)
        {
            continue;
        }
        if (op->nout > astores) {
            PyMem_Free(stores);
            astores = op->nout;
            stores = PyMem_Malloc(astores * sizeof(_PyRegOp *));
            if (stores == NULL) {
                PyErr_NoMemory();
                return ERROR;
            }
        }
        int why = unpack_locals_site(cfg, start, end, op, stores);
        if (why != UNPACK_OK) {
            continue;
        }
        op->dst = stores[0]->oparg;
        op->flags |= _PyReg_UNPACK_LOCALS;
        for (int k = 0; k < op->nout; k++) {
            stores[k]->opcode = _PyReg_DEAD;
            stores[k]->flags |= _PyReg_STORE_FOLDED;
        }
    }
    PyMem_Free(stores);
    return SUCCESS;
}

int
_PyRegPass_UnpackLocals(_PyRegCfg *cfg)
{
    return for_each_region_backward(cfg, unpack_locals_region);
}


/* POP_TOP of a pure value (pass c).
 *
 * A POP_TOP of a value that a pure op made from locals and constants
 * alone, which cannot fail and runs no other code, goes with its
 * producer: the value never exists.  Pure is the DSL's annotation; the
 * value must have no other reader.  Other POP_TOPs stay, as the CLEAR
 * that releases their value.  If the pair is all there is of its line, a
 * NOP keeps the line's event.  A POP_TOP that copy propagation gave a
 * local, as for the statement "x", does nothing: only the loads that
 * cannot fail fold. */

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

/* Remove the ops prod (if not NULL) and pop, keeping a NOP for the line
 * if nothing else in the region is on it. */
static void
remove_with_line(_PyRegCfg *cfg, int start, int end, _PyRegOp *prod,
                 _PyRegOp *pop)
{
    _PyRegOp *first = prod != NULL ? prod : pop;
    if (prod != NULL && (prod->loc.lineno != pop->loc.lineno ||
                         prod->except != pop->except))
    {
        return;
    }
    _PyRegOp *before = live_neighbour(cfg, start, end,
                                      (int)(first - cfg->ops), -1);
    _PyRegOp *after = live_neighbour(cfg, start, end,
                                     (int)(pop - cfg->ops), 1);
    int line = first->loc.lineno;
    int keeps_line = (before != NULL && before->loc.lineno == line &&
                      before->except == first->except &&
                      (before->flags & _PyReg_SOURCE_ORIGIN)) ||
                     (after != NULL && after->loc.lineno == line &&
                      after->except == first->except &&
                      (after->flags & _PyReg_SOURCE_ORIGIN));
    if (prod != NULL) {
        prod->opcode = _PyReg_DEAD;
    }
    if (keeps_line && pop->loc.lineno == line) {
        pop->opcode = _PyReg_DEAD;
    }
    else {
        pop->opcode = NOP;
        pop->oparg = 0;
        pop->loc = first->loc;
        pop->flags = (pop->flags & ~_PyReg_SOURCE_ORIGIN) |
                     (first->flags & _PyReg_SOURCE_ORIGIN);
        pop->nread = pop->npop = 0;
    }
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
        if (v < nl) {
            remove_with_line(cfg, start, end, NULL, pop);
            continue;
        }
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
        int effects = _PyRegCfg_FormatOf(prod->opcode)->effects;
        if (!(effects & _PyIR_PURE) || (effects & (_PyIR_RAISES | _PyIR_ESCAPES))
            || prod->dst >= 0)
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
        remove_with_line(cfg, start, end, prod, pop);
    }
    return SUCCESS;
}

int
_PyRegPass_PopTop(_PyRegCfg *cfg)
{
    return for_each_region_backward(cfg, pop_top_region);
}


/* An unread result is a semantic sink, not a register followed by a release
 * instruction. Source lines are retained independently of executable words. */

static int
discard_region(_PyRegCfg *cfg, int start, int end)
{
    for (int i = start; i < end; i++) {
        _PyRegOp *pop = &cfg->ops[i];
        if (pop->opcode != POP_TOP) {
            continue;
        }
        int v = _PyReg_IN(cfg, pop)[0];
        if (!is_fresh(cfg, v)) {
            continue;
        }
        _PyRegOp *prod = NULL;
        int other_read = 0;
        for (int j = start; j < end; j++) {
            _PyRegOp *op = &cfg->ops[j];
            if (op->opcode == _PyReg_DEAD || op == pop) {
                continue;
            }
            for (int k = 0; k < op->nread + op->nimplicit; k++) {
                other_read |= _PyReg_IN(cfg, op)[k] == v;
            }
            if (j < i && op->nout == 1 && _PyReg_OUT(cfg, op)[0] == v) {
                prod = op;
            }
        }
        if (other_read || prod == NULL || prod->dst >= 0 ||
            !(_PyRegCfg_FormatOf(prod->opcode)->flags & _PyIR_DISCARDS))
        {
            continue;
        }
        prod->flags |= _PyReg_DISCARD;
        pop->opcode = NOP;
        pop->oparg = 0;
        pop->nread = pop->npop = pop->nout = 0;
    }
    return SUCCESS;
}

int
_PyRegPass_Discard(_PyRegCfg *cfg)
{
    return for_each_region_backward(cfg, discard_region);
}


/* Truth folding in value context (pass d).
 *
 * "a.x and b" tests a copy of its first value and pops the value where it
 * goes on:
 *
 *     COPY 1; POP_JUMP_IF_FALSE L; POP_TOP; <b>; L:
 *
 * JUMP_IF_FALSE_OR_POP does all three: it keeps the value where it jumps
 * and releases it where it falls through.  The POP_TOP must be the next op
 * on the fall-through path, which only the jump reaches, and all three on
 * one line, so that no line's event goes.  For a value loaded from a
 * local the flowgraph turns the COPY into a second load, which copy
 * propagation folds into the jump:
 *
 *     LOAD_FAST x; POP_JUMP_IF_FALSE L <- x; POP_TOP; <b>; L:
 *
 * and the jump becomes JUMP_IF_FALSE_OR_POP of the first load's value. */

/* The POP_TOP that pops v where jump falls through, on jump's line */
static _PyRegOp *
fall_through_pop(_PyRegCfg *cfg, int start, int end, _PyRegOp *jump, int v)
{
    int j = (int)(jump - cfg->ops);
    _PyRegOp *next = live_neighbour(cfg, start, end, j, 1);
    if (next == NULL || next->opcode != POP_TOP
        || next->loc.lineno != jump->loc.lineno
        || _PyReg_IN(cfg, next)[0] != v)
    {
        return NULL;
    }
    return next;
}

static inline int
truth_jump(int opcode)
{
    return opcode == POP_JUMP_IF_FALSE ? JUMP_IF_FALSE_OR_POP
         : opcode == POP_JUMP_IF_TRUE ? JUMP_IF_TRUE_OR_POP : 0;
}

/* LOAD_FAST x (v); POP_JUMP_IF_* <- x; POP_TOP v */
static int
fold_local_truth(_PyRegCfg *cfg, int start, int end, _PyRegOp *load)
{
    int v = _PyReg_OUT(cfg, load)[0];
    _PyRegOp *jump = live_neighbour(cfg, start, end,
                                    (int)(load - cfg->ops), 1);
    if (jump == NULL || !truth_jump(jump->opcode) || jump->nread != 1
        || jump->loc.lineno != load->loc.lineno
        || _PyReg_IN(cfg, jump)[0] != load->oparg
        || !_PyReg_IS_CANON(cfg, v))
    {
        return SUCCESS;
    }
    _PyRegOp *pop = fall_through_pop(cfg, start, end, jump, v);
    if (pop == NULL) {
        return SUCCESS;
    }
    // The load gets a fresh register for the jump to take, and the jump
    // gives the value its stack position's register where it goes
    int in, out;
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, 1, &in));
    RETURN_IF_ERROR(_PyRegCfg_AddOperands(cfg, 1, &out));
    int t = cfg->nvregs++;
    _PyReg_OUT(cfg, load)[0] = t;
    cfg->operands[in] = t;
    cfg->operands[out] = v;
    jump->opcode = truth_jump(jump->opcode);
    jump->in = in;
    jump->out = out;
    jump->nread = jump->npop = 1;
    jump->nout = 1;
    jump->depth = pop->depth;
    jump->flags |= _PyReg_JUMP_OUTPUT;
    pop->opcode = _PyReg_DEAD;
    return SUCCESS;
}

static int
truth_fold_region(_PyRegCfg *cfg, int start, int end)
{
    for (int i = start; i < end; i++) {
        _PyRegOp *copy = &cfg->ops[i];
        if (copy->opcode == LOAD_FAST && copy->nout == 1 && copy->dst < 0) {
            RETURN_IF_ERROR(fold_local_truth(cfg, start, end, copy));
            continue;
        }
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
        _PyRegOp *next = fall_through_pop(cfg, start, end, jump, cout[0]);
        if (next == NULL) {
            continue;
        }
        copy->opcode = truth_jump(jump->opcode);
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


/* Constant slots (G6).
 *
 * An inline scalar constant (small or inline int, inline float, None, True,
 * False) that an op inside a loop reads through a field that may name a
 * local becomes a hidden local, filled when the frame is pushed
 * (CO_FAST_CONST, _PyFrame_FillConstSlots()): the load goes, as a load of a
 * local does under copy propagation, and the reader names the slot.  The
 * same conditions apply as there, less the rules about writes: nobody writes
 * a constant slot.  Window inputs, constants outside loops and every other
 * constant keep their loads.
 *
 * The slots sit between the cells and the free variables, so the free
 * variables and everything above them move up, and so do the opargs that
 * name them.  The pass runs after compaction, which the caller repeats. */

typedef struct {
    int load;               // the op of the load
    int reader;
    int reader_k;
    uintptr_t bits;         // the constant
    int index;              // in consts, or -1 for a small int not in it
    int oparg;              // of a LOAD_SMALL_INT
} const_fold;

/* The loop blocks: those from the target of a backward jump to the block
 * of the jump. */
static uint8_t *
loop_blocks(const _PyRegCfg *cfg, int *any)
{
    uint8_t *in_loop = PyMem_Calloc(Py_MAX(cfg->nblocks, 1), 1);
    if (in_loop == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    *any = 0;
    for (int b = 0; b < cfg->nblocks; b++) {
        const _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; i < blk->end; i++) {
            const _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD || op->target < 0
                || (_PyRegCfg_FormatOf(op->opcode)->flags & _PyIR_NAMES_TARGET)
                || op->target > b)
            {
                continue;
            }
            for (int t = op->target; t <= b; t++) {
                in_loop[t] = 1;
            }
            *any = 1;
        }
    }
    return in_loop;
}

/* Whether value is None, True or False */
static int
is_singleton(PyValueBorrow value)
{
    if (!PyValue_IsObject(value)) {
        return 0;
    }
    PyObject *obj = PyValue_AsKnownObjectBorrow(value);
    return obj == Py_None || obj == Py_True || obj == Py_False;
}

/* Whether load is a load of a constant that can live in a slot, and if so
 * its bits and its index in consts (-1 if a small int is not there). */
static int
const_of_load(const _PyRegOp *load, PyObject *consts, uintptr_t *bits,
              int *index)
{
    if (load->opcode == LOAD_SMALL_INT) {
        *bits = _PyValue_Identity(PyValue_FromInlineInt(load->oparg));
        *index = -1;
        // An equal entry may exist
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(consts); i++) {
            if (_PyValue_Identity(PyList_GET_VALUE(consts, i)) == *bits) {
                *index = (int)i;
                break;
            }
        }
        return 1;
    }
    if (load->opcode != LOAD_CONST || load->oparg >= PyList_GET_SIZE(consts)) {
        return 0;
    }
    PyValueBorrow value = PyList_GET_VALUE(consts, load->oparg);
    if (!PyValue_IsInlineInt(value) && !PyValue_IsInlineFloat(value)
        && !is_singleton(value))
    {
        return 0;
    }
    *bits = _PyValue_Identity(value);
    *index = load->oparg;
    return 1;
}

static int
find_const_folds(_PyRegCfg *cfg, PyObject *consts, const uint8_t *in_loop,
                 const_fold **folds, int *nfolds)
{
    pending_load *pend = PyMem_Malloc(Py_MAX(cfg->nops, 1) * sizeof(pending_load));
    *folds = PyMem_Malloc(Py_MAX(cfg->nops, 1) * sizeof(const_fold));
    if (pend == NULL || *folds == NULL) {
        PyMem_Free(pend);
        PyMem_Free(*folds);
        PyErr_NoMemory();
        return ERROR;
    }
    *nfolds = 0;
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
            int n = 0;
            for (int p = 0; p < npend; p++) {
                _PyRegOp *load = pend[p].load;
                int v = pend[p].reg;
                if (load->loc.lineno != op->loc.lineno) {
                    continue;
                }
                int uses = reads_count(cfg, op, v);
                if (uses == 0) {
                    pend[n++] = pend[p];
                    continue;
                }
                if (uses != 1) {
                    continue;
                }
                for (int k = 0; k < op->nread; k++) {
                    if (in[k] == v && borrow_field(cfg, op, k) >= 0) {
                        const_fold *f = &(*folds)[(*nfolds)++];
                        f->load = (int)(load - cfg->ops);
                        f->reader = i;
                        f->reader_k = k;
                        uintptr_t bits;
                        const_of_load(load, consts, &bits, &f->index);
                        f->bits = bits;
                        f->oparg = load->oparg;
                    }
                }
            }
            npend = n;
            uintptr_t bits;
            int index;
            if (in_loop[b] && (op->opcode == LOAD_CONST
                               || op->opcode == LOAD_SMALL_INT)
                && is_fresh(cfg, _PyReg_OUT(cfg, op)[0])
                && const_of_load(op, consts, &bits, &index))
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

/* Move the registers from first_free up, and the opargs that name slots
 * from there, up by k. */
static void
shift_slots(_PyRegCfg *cfg, int first_free, int k)
{
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        int *in = _PyReg_IN(cfg, op);
        for (int j = 0; j < op->nread + op->nimplicit; j++) {
            if (in[j] >= first_free) {
                in[j] += k;
            }
        }
        int *out = _PyReg_OUT(cfg, op);
        for (int j = 0; j < op->nout; j++) {
            if (out[j] >= first_free) {
                out[j] += k;
            }
        }
        if (op->dst >= first_free) {
            op->dst += k;
        }
        if (op->opcode == _PyReg_DEAD) {
            continue;
        }
        int role = _PyIR_OpargRole(_PyRegCfg_FormatOf(op->opcode));
        if ((role == _PyIR_ROLE_LOCAL_IN || role == _PyIR_ROLE_LOCAL_OUT
             || role == _PyIR_ROLE_LOCAL_CLEAR || role == _PyIR_ROLE_LOCAL_MOVE
             || role == _PyIR_ROLE_CELL)
            && op->oparg >= first_free)
        {
            op->oparg += k;
        }
    }
    cfg->nlocalsplus += k;
    cfg->nvregs += k;
}

int
_PyRegPass_ConstSlots(_PyRegCfg *cfg, PyObject *consts, int nfree)
{
    int any;
    uint8_t *in_loop = loop_blocks(cfg, &any);
    if (in_loop == NULL) {
        return ERROR;
    }
    const_fold *folds = NULL;
    int nfolds = 0;
    int nconst = 0;
    uintptr_t *keys = NULL;
    int *key_index = NULL;
    int *fold_slot = NULL;
    if (!any) {
        goto done;
    }
    if (find_const_folds(cfg, consts, in_loop, &folds, &nfolds) < 0) {
        PyMem_Free(in_loop);
        return ERROR;
    }
    // One slot per distinct constant, in order of use, while the frame fits
    keys = PyMem_Malloc(Py_MAX(nfolds, 1) * sizeof(uintptr_t));
    key_index = PyMem_Malloc(Py_MAX(nfolds, 1) * sizeof(int));
    fold_slot = PyMem_Malloc(Py_MAX(nfolds, 1) * sizeof(int));
    if (keys == NULL || key_index == NULL || fold_slot == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    int room = IR_MAX_SLOTS - cfg->nlocalsplus - cfg->ntemps;
    for (int f = 0; f < nfolds; f++) {
        int j;
        for (j = 0; j < nconst && keys[j] != folds[f].bits; j++) {
        }
        if (j == nconst) {
            if (nconst >= room) {
                fold_slot[f] = -1;
                continue;
            }
            keys[nconst] = folds[f].bits;
            key_index[nconst] = folds[f].index;
            nconst++;
        }
        fold_slot[f] = j;
    }
    if (nconst == 0) {
        goto done;
    }
    // A small int that consts lacks gets an entry
    for (int j = 0; j < nconst; j++) {
        if (key_index[j] >= 0) {
            continue;
        }
        int oparg = -1;
        for (int f = 0; f < nfolds; f++) {
            if (fold_slot[f] == j) {
                oparg = folds[f].oparg;
                break;
            }
        }
        if (PyList_AppendValue(consts, PyValue_FromInlineInt(oparg)) < 0) {
            goto error;
        }
        key_index[j] = (int)PyList_GET_SIZE(consts) - 1;
        assert(_PyValue_Identity(PyList_GET_VALUE(consts, key_index[j]))
               == keys[j]);
    }
    int first_free = cfg->nlocalsplus - nfree;
    shift_slots(cfg, first_free, nconst);
    for (int f = 0; f < nfolds; f++) {
        if (fold_slot[f] < 0) {
            continue;
        }
        _PyRegOp *load = &cfg->ops[folds[f].load];
        _PyRegOp *reader = &cfg->ops[folds[f].reader];
        int slot = first_free + fold_slot[f];
        _PyReg_IN(cfg, reader)[folds[f].reader_k] = slot;
        load->oparg = slot;
        load->opcode = _PyReg_DEAD;
        load->flags |= _PyReg_LOAD_FOLDED;
        load->reader = folds[f].reader;
        load->reader_k = folds[f].reader_k;
    }
    cfg->const_first = first_free;
    cfg->nconst = nconst;
    cfg->const_index = PyMem_Malloc(nconst * sizeof(int));
    if (cfg->const_index == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    memcpy(cfg->const_index, key_index, nconst * sizeof(int));
done:
    PyMem_Free(in_loop);
    PyMem_Free(folds);
    PyMem_Free(keys);
    PyMem_Free(key_index);
    PyMem_Free(fold_slot);
    return nconst;
error:
    PyMem_Free(in_loop);
    PyMem_Free(folds);
    PyMem_Free(keys);
    PyMem_Free(key_index);
    PyMem_Free(fold_slot);
    return ERROR;
}


/* Fusing a compare with the jump on its result.
 *
 * COMPARE_OP, CONTAINS_OP or IS_OP followed by POP_JUMP_IF_FALSE or _TRUE
 * on its result can run as one word (the fused handlers of
 * Python/bytecodes.c).  This pass only marks the compare: the CFG, its
 * liveness and the verifier keep seeing the pair, and the assembler decides
 * after it has placed the words whether the target is near enough.  The pair
 * is left alone where
 *
 *   - the jump is not the next live op of the block, or does not read the
 *     compare's only output, or that output is read again (the jump is not
 *     its last reader);
 *   - a store is folded into the compare (it has a local dst);
 *   - the two are on different lines, or in different exception ranges: a
 *     fused word has one location, the compare's;
 *   - the compare is a COMPARE_OP whose result is not converted to a bool
 *     (its oparg lacks 16): the jump would test an arbitrary object.
 */
int
_PyRegPass_FuseJump(_PyRegCfg *cfg)
{
    _PyRegLiveness lv;
    int have_liveness = 0;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *cmp = &cfg->ops[i];
            if (cmp->opcode != COMPARE_OP && cmp->opcode != CONTAINS_OP
                && cmp->opcode != IS_OP)
            {
                continue;
            }
            if (cmp->opcode == COMPARE_OP && !(cmp->oparg & 16)) {
                continue;
            }
            if (cmp->dst >= 0 || cmp->nread != 2 || cmp->npop != 2
                || cmp->nout != 1)
            {
                continue;
            }
            _PyRegOp *jump = live_neighbour(cfg, blk->start, blk->end, i, 1);
            if (jump == NULL
                || (jump->opcode != POP_JUMP_IF_FALSE
                    && jump->opcode != POP_JUMP_IF_TRUE)
                || jump->nread != 1 || jump->npop != 1 || jump->nimplicit != 0
                || jump->target < 0
                || jump->loc.lineno != cmp->loc.lineno
                || jump->except != cmp->except
                || _PyReg_IN(cfg, jump)[0] != _PyReg_OUT(cfg, cmp)[0])
            {
                continue;
            }
            if (!have_liveness) {
                if (_PyRegCfg_Liveness(cfg, &lv) < 0) {
                    return -1;
                }
                have_liveness = 1;
            }
            if (lv.last[jump->in]) {
                cmp->fuse = (int)(jump - cfg->ops);
            }
        }
    }
    if (have_liveness) {
        _PyRegCfg_FreeLiveness(&lv);
    }
    return 0;
}


/* PUSH_NULL goes (pass f).
 *
 * PUSH_NULL writes NULL into a temporary, and the NULL invariant
 * (register-ir.md section 1) says that a temporary nothing has written, or
 * that a handler has consumed, is NULL.  The op keeps its register and so
 * its slot, which compaction takes and the verifier checks like any other
 * (two live temporaries never share a slot), but the assembler gives it no
 * word.  If it alone holds its line, a NOP keeps the line's event. */

static int
elide_null_region(_PyRegCfg *cfg, int start, int end)
{
    for (int i = start; i < end; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode != PUSH_NULL) {
            continue;
        }
        assert(op->dst < 0 && op->nout == 1);
        _PyRegOp *before = live_neighbour(cfg, start, end, i, -1);
        _PyRegOp *after = live_neighbour(cfg, start, end, i, 1);
        int line = op->loc.lineno;
        int keeps_line = (before != NULL && before->loc.lineno == line &&
                          before->except == op->except &&
                          (before->flags & _PyReg_SOURCE_ORIGIN)) ||
                         (after != NULL && after->loc.lineno == line &&
                          after->except == op->except &&
                          (after->flags & _PyReg_SOURCE_ORIGIN));
        op->flags |= _PyReg_ELIDED | (keeps_line ? 0 : _PyReg_ELIDED_NOP);
    }
    return SUCCESS;
}

int
_PyRegPass_ElideNull(_PyRegCfg *cfg)
{
    RETURN_IF_ERROR(for_each_region_backward(cfg, elide_null_region));
    // Unreachable code never runs, but must not name a handler that is gone
    for (int i = 0; i < cfg->nops; i++) {
        if (cfg->ops[i].opcode == PUSH_NULL) {
            cfg->ops[i].flags |= _PyReg_ELIDED;
        }
    }
    return SUCCESS;
}

/* This is logical producer provenance, not a test of the compacted slot.
 * Canonical registers can have several definitions. Intersect predecessor
 * facts before selecting a role, including the retained exception prefix. */
typedef struct {
    int nwords, nblocks, count, head, tail;
    uint64_t *entries;
    int *queue;
    uint8_t *reached, *queued;
} call_null_proof;

static int
call_null_get(const _PyRegCfg *cfg, const uint64_t *state, int v)
{
    int bit = v - cfg->nlocalsplus;
    return bit >= 0 && v < cfg->nvregs &&
        ((state[bit / 64] >> (bit % 64)) & 1);
}

static void
call_null_set(const _PyRegCfg *cfg, uint64_t *state, int v, int value)
{
    int bit = v - cfg->nlocalsplus;
    if (bit >= 0 && v < cfg->nvregs) {
        uint64_t mask = UINT64_C(1) << (bit % 64);
        state[bit / 64] = (state[bit / 64] & ~mask) | (value ? mask : 0);
    }
}

static void
call_null_merge(call_null_proof *proof, int block, const uint64_t *state,
                int limit)
{
    uint64_t *entry = proof->entries + (size_t)block * proof->nwords;
    int changed = !proof->reached[block];
    for (int w = 0; w < proof->nwords; w++) {
        int bits = Py_MIN(Py_MAX(limit - w * 64, 0), 64);
        uint64_t mask = bits == 64 ? UINT64_MAX : (UINT64_C(1) << bits) - 1;
        uint64_t incoming = state[w] & mask;
        uint64_t next = proof->reached[block] ? entry[w] & incoming : incoming;
        changed |= entry[w] != next;
        entry[w] = next;
    }
    proof->reached[block] = 1;
    if (changed && !proof->queued[block]) {
        proof->queued[block] = 1;
        proof->queue[proof->tail] = block;
        proof->tail = (proof->tail + 1) % proof->nblocks;
        proof->count++;
    }
}

static void
call_null_step(_PyRegCfg *cfg, _PyRegOp *op, uint64_t *state, uint8_t *outputs)
{
    int *in = _PyReg_IN(cfg, op), *out = _PyReg_OUT(cfg, op);
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
    int through = _PyIR_Extent(fmt->through, op->oparg);
    for (int j = 0; j < op->nout; j++) {
        int value = 0;
        if (op->opcode == PUSH_NULL ||
            (op->opcode == LOAD_GLOBAL && (op->oparg & 1) && j == op->nout - 1))
        {
            value = 1;
        }
        else if (op->opcode == COPY) {
            int k = j < op->nread ? j : 0;
            value = call_null_get(cfg, state, in[k]);
        }
        else if (op->opcode == SWAP) {
            int k = j == 0 ? op->nread - 1 : j == op->nout - 1 ? 0 : j;
            value = call_null_get(cfg, state, in[k]);
        }
        else if (j < through && j < op->nread) {
            value = call_null_get(cfg, state, in[j]);
        }
        outputs[j] = (uint8_t)value;
    }
    for (int k = op->nread - op->npop; k < op->nread; k++) {
        call_null_set(cfg, state, in[k], 0);
    }
    for (int j = 0; j < op->nout; j++) {
        call_null_set(cfg, state, out[j], outputs[j]);
    }
}

static int
call_roles(_PyRegCfg *cfg, int verify)
{
    int n = cfg->nblocks;
    if (n == 0) {
        return SUCCESS;
    }
    int needed = 0;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        needed |= verify
            ? (op->flags & (_PyReg_CALL_NO_SELF | _PyReg_CALL_NO_KWARGS)) != 0
            : op->opcode == CALL || op->opcode == CALL_KW ||
              op->opcode == CALL_FUNCTION_EX;
    }
    if (!needed) {
        return SUCCESS;
    }
    int canonical = Py_MIN(cfg->maxdepth, IR_MAX_SLOTS);
    int nwords = Py_MAX(1, (canonical + 63) / 64);
    int nbits = cfg->nvregs - cfg->nlocalsplus;
    int work_words = Py_MAX(1, nbits / 64 + (nbits % 64 != 0));
    if ((size_t)n > SIZE_MAX / sizeof(uint64_t) / nwords) {
        PyErr_NoMemory();
        return ERROR;
    }
    call_null_proof proof = {.nwords = nwords, .nblocks = n};
    proof.entries = PyMem_Calloc((size_t)n * nwords, sizeof(uint64_t));
    uint64_t *state = PyMem_Calloc(work_words, sizeof(uint64_t));
    proof.queue = PyMem_New(int, n);
    proof.reached = PyMem_Calloc((size_t)n * 2, 1);
    int max_outputs = 1;
    for (int i = 0; i < cfg->nops; i++) {
        max_outputs = Py_MAX(max_outputs, cfg->ops[i].nout);
    }
    uint8_t *outputs = PyMem_New(uint8_t, max_outputs);
    int result = ERROR;
    if (proof.entries == NULL || state == NULL || proof.queue == NULL ||
        proof.reached == NULL || outputs == NULL)
    {
        PyErr_NoMemory();
        goto done;
    }
    proof.queued = proof.reached + n;
    call_null_merge(&proof, 0, state, canonical);
    while (proof.count) {
        int b = proof.queue[proof.head];
        proof.head = (proof.head + 1) % n;
        proof.count--;
        proof.queued[b] = 0;
        assert(!cfg->blocks[b].continued);
        int next = b + 1;
        while (next < n && cfg->blocks[next].continued) {
            next++;
        }
        /* Fresh registers are defined before use within this region. Each
         * definition replaces its fact, so only canonical entry bits join. */
        memcpy(state, proof.entries + (size_t)b * nwords,
               (size_t)nwords * sizeof(*state));
        for (int i = cfg->blocks[b].start; i < cfg->blocks[next - 1].end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            if (op->except >= 0) {
                _PyRegBlock *handler = &cfg->blocks[op->except];
                int depth = handler->startdepth - 1 - handler->preserve_lasti;
                call_null_merge(&proof, op->except, state, depth);
            }
            call_null_step(cfg, op, state, outputs);
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
            if ((fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) &&
                !(fmt->flags & _PyIR_NAMES_TARGET))
            {
                call_null_merge(&proof, op->target, state, canonical);
            }
        }
        if (cfg->blocks[next - 1].falls_through) {
            call_null_merge(&proof, next, state, canonical);
        }
    }
    /* Only the stable meet can select a canonical input role. */
    for (int b = 0; b < n; b++) {
        if (cfg->blocks[b].continued) {
            continue;
        }
        int next = b + 1;
        while (next < n && cfg->blocks[next].continued) {
            next++;
        }
        memcpy(state, proof.entries + (size_t)b * nwords,
               (size_t)nwords * sizeof(*state));
        for (int i = cfg->blocks[b].start; i < cfg->blocks[next - 1].end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            int roles = 0;
            if (proof.reached[b] && (op->opcode == CALL || op->opcode == CALL_KW ||
                                    op->opcode == CALL_FUNCTION_EX))
            {
                int *in = _PyReg_IN(cfg, op);
                if (call_null_get(cfg, state, in[1])) {
                    roles |= _PyReg_CALL_NO_SELF;
                    if (op->opcode == CALL_FUNCTION_EX &&
                        call_null_get(cfg, state, in[3]))
                    {
                        roles |= _PyReg_CALL_NO_KWARGS;
                    }
                }
            }
            int mask = _PyReg_CALL_NO_SELF | _PyReg_CALL_NO_KWARGS;
            if (verify) {
                if ((op->flags & mask) & ~roles) {
                    PyErr_SetString(PyExc_SystemError,
                                    "call role lacks virtual NULL provenance");
                    goto done;
                }
            }
            else {
                op->flags = (op->flags & ~mask) | roles;
            }
            call_null_step(cfg, op, state, outputs);
        }
    }
    result = SUCCESS;
done:
    PyMem_Free(proof.entries);
    PyMem_Free(state);
    PyMem_Free(proof.queue);
    PyMem_Free(proof.reached);
    PyMem_Free(outputs);
    return result;
}

int
_PyRegPass_CallRoles(_PyRegCfg *cfg)
{
    return call_roles(cfg, 0);
}

int
_PyRegCfg_VerifyCallRoles(_PyRegCfg *cfg)
{
    return call_roles(cfg, 1);
}
