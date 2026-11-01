#include "Python.h"
#include "pycore_jit_ir.h"

static void
use(_PyJitGraph *g, uint16_t id, int at)
{
    if (id < g->nvalues && g->values[id].last < at) g->values[id].last = (uint16_t)at;
}

int
_PyJit_AllocateGraph(_PyJitGraph *g)
{
    if (_PyJit_VerifyGraph(g) < 0) return -1;
    /* Six actual integer registers. Snapshot-only and loop-backedge uses are
     * liveness roots, not values the emitter is allowed to silently forget. */
    for (int i = 0; i < g->nnodes; i++) {
        JitSsaNode *n = &g->nodes[i];
        use(g, n->left, i); use(g, n->right, i);
        for (int k = 0; k < 2; k++) {
            uint16_t state = k ? n->after : n->before;
            if (state == JIT_NO_VALUE) continue;
            JitFrameState *s = &g->states[state];
            for (int h = 0; h < g->nhomes; h++) use(g, s->value[h], i);
        }
    }
    for (int h = 0; h < g->nhomes; h++) {
        use(g, g->carried[h], g->nnodes);
    }
    int active[6] = {-1, -1, -1, -1, -1, -1};
    int slots[JIT_GRAPH_SPILLS];
    uint64_t spilled = 0;
    for (int s = 0; s < JIT_GRAPH_SPILLS; s++) slots[s] = -1;
    for (int id = 0; id < g->nvalues; id++) {
        JitSsaValue *v = &g->values[id];
        int slot = -1;
        for (int s = 0; s < JIT_GRAPH_SPILLS; s++) {
            if (slots[s] >= 0 && g->values[slots[s]].last < v->first) slots[s] = -1;
            if (slots[s] == -1) { slot = s; break; }
        }
        if (slot < 0) return -1;
        slots[slot] = id;
        v->spill = (int16_t)slot;
        if (g->nscratch <= slot) g->nscratch = (uint16_t)(slot + 1);
        int available = -1;
        for (int r = 0; r < 6; r++) {
            if (active[r] >= 0 && g->values[active[r]].last < v->first) active[r] = -1;
            if (active[r] == -1) available = r;
        }
        if (available >= 0) {
            v->reg = (int16_t)available;
            active[available] = id;
        }
        else {
            if (!(spilled & (UINT64_C(1) << slot))) {
                spilled |= UINT64_C(1) << slot;
                g->nspills++;
            }
        }
    }
    return 0;
}
