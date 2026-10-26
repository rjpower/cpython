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

static inline int
is_fresh(const _PyRegCfg *cfg, int v)
{
    return v >= _PyReg_FRESH0(cfg);
}


/* COPY of a loaded local: a second load (pass c, first part).  Not yet. */
int
_PyRegPass_CopyToLoad(_PyRegCfg *cfg)
{
    (void)cfg;
    return SUCCESS;
}

/* SWAP by renaming (pass c).  Not yet. */
int
_PyRegPass_SwapRename(_PyRegCfg *cfg)
{
    (void)cfg;
    return SUCCESS;
}


/* Copy propagation (pass a).
 *
 * A LOAD_FAST_BORROW whose value the next op reads through a field that
 * may name a local becomes that field: the op reads the local in place,
 * borrowed, as the load would have pushed it.  Nothing runs between the
 * load and its reader, so nothing writes the local in between.  A load
 * keeps its op if its line is not its reader's, so that line events stay
 * where they were. */

/* Fold into op the loads run[0 .. nrun - 1], which push the top nrun
 * values that op finds on the stack. */
static void
fold_loads(_PyRegCfg *cfg, _PyRegOp *op, _PyRegOp **run, int nrun)
{
    const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode);
    if ((fmt->flags & _PyIR_WINDOW) || fmt->borrow_mask == 0) {
        return;
    }
    int *in = _PyReg_IN(cfg, op);
    int first_input = op->depth - fmt->ninputs;
    int first_field = op->nread - fmt->ninputs;
    for (int r = nrun - 1; r >= 0; r--) {
        _PyRegOp *load = run[r];
        int k = op->depth - nrun + r - first_input;
        if (k < 0 || !(fmt->borrow_mask & (1 << k))) {
            return;
        }
        if (load->loc.lineno != op->loc.lineno) {
            return;
        }
        int t = _PyReg_OUT(cfg, load)[0];
        if (!is_fresh(cfg, t) || in[first_field + k] != t) {
            return;
        }
        in[first_field + k] = load->oparg;
        load->opcode = _PyReg_DEAD;
        load->flags |= _PyReg_LOAD_FOLDED;
    }
}

int
_PyRegPass_CopyProp(_PyRegCfg *cfg)
{
    _PyRegOp **run = PyMem_Malloc(Py_MAX(cfg->nops, 1) * sizeof(_PyRegOp *));
    if (run == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    // The loads since the last op, which a region boundary ends
    int nrun = 0;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->continued) {
            nrun = 0;
        }
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->opcode == _PyReg_DEAD) {
                continue;
            }
            if (op->opcode == LOAD_FAST_BORROW) {
                run[nrun++] = op;
                continue;
            }
            if (nrun > 0) {
                fold_loads(cfg, op, run, nrun);
                nrun = 0;
            }
        }
    }
    PyMem_Free(run);
    return SUCCESS;
}


/* Store elimination (pass b).
 *
 * A STORE_FAST z just after the op that produces its value alone, on top,
 * where that op's format lets dst name a local: the op writes z, and its
 * handler releases z's old value as STORE_FAST would.  Nothing runs in
 * between, so nothing reads or writes z there. */
int
_PyRegPass_StoreElim(_PyRegCfg *cfg)
{
    _PyRegOp *prev = NULL;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        if (!blk->continued) {
            prev = NULL;
        }
        if (!blk->reachable) {
            continue;
        }
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            _PyRegOp *p = prev;
            prev = op;
            if (op->opcode != STORE_FAST || p == NULL
                || p->opcode == _PyReg_DEAD)
            {
                continue;
            }
            int value = _PyReg_IN(cfg, op)[0];
            if (p->opcode == LOAD_FAST_BORROW || p->dst >= 0
                || p->loc.lineno != op->loc.lineno || value < cfg->nlocalsplus)
            {
                continue;
            }
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(p->opcode);
            if (!(fmt->flags & _PyIR_STORE_DST) || fmt->dst == _PyIR_NO_DST) {
                continue;
            }
            int j = fmt->flags & _PyIR_WINDOW ? fmt->dst : p->npop + fmt->dst;
            // The result is the producer's last output, on top
            if (j != p->nout - 1 || _PyReg_OUT(cfg, p)[j] != value) {
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


/* POP_TOP of a dead pure value (pass c).  Not yet. */
int
_PyRegPass_PopTop(_PyRegCfg *cfg)
{
    (void)cfg;
    return SUCCESS;
}
