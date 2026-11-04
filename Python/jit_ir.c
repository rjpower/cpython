#include "Python.h"
#include "pycore_jit_ir.h"
#include "pycore_opcode_utils.h"

static int
value(_PyJitGraph *g, int home)
{
    if (g->nvalues == JIT_GRAPH_VALUES) return -1;
    int id = g->nvalues++;
    g->values[id] = (JitSsaValue){g->nnodes, g->nnodes, -1, -1, (int16_t)home};
    return id;
}

static JitSsaNode *
node(_PyJitGraph *g, int kind, uint32_t word)
{
    if (g->nnodes == JIT_GRAPH_NODES) return NULL;
    JitSsaNode *n = &g->nodes[g->nnodes++];
    *n = (JitSsaNode){.word=word, .kind=(uint8_t)kind, .home=-1,
        .left=JIT_NO_VALUE, .right=JIT_NO_VALUE, .value=JIT_NO_VALUE,
        .before=JIT_NO_VALUE, .after=JIT_NO_VALUE};
    return n;
}

static int
snapshot(_PyJitGraph *g, uint16_t *homes, uint32_t word, int effect)
{
    if (g->nstates == JIT_GRAPH_STATES || g->map_entries + g->nhomes > JIT_GRAPH_MAPS) return -1;
    int id = g->nstates++;
    JitFrameState *state = &g->states[id];
    state->word = word;
    state->effect = (uint16_t)effect;
    memcpy(state->value, homes, g->nhomes * sizeof(*homes));
    g->map_entries += g->nhomes;
    return id;
}

static int
imports(_PyJitGraph *g, uint16_t *homes, const JitHome *observed, uint32_t word, int phi)
{
    for (int h = 0; h < g->nhomes; h++) {
        homes[h] = observed[h].kind == JIT_HOME_NULL ? JIT_NULL_VALUE : JIT_NO_VALUE;
        if (observed[h].kind != JIT_HOME_INLINE && observed[h].kind != JIT_HOME_BOXED) continue;
        /* An owning boxed temporary remains canonical and its consumer uses
         * the shared helper. It is not an int32 import/ownership promise. */
        if (!phi && h >= g->nlocals && observed[h].kind == JIT_HOME_BOXED) continue;
        /* Loop-carried temporaries require an ownership PHI not provided by
         * this first scalar graph. Borrowed boxed locals remain eligible. */
        if (phi && h >= g->nlocals) return -1;
        int v = value(g, h);
        JitSsaNode *n = node(g, phi ? JIT_SSA_PHI : JIT_SSA_IMPORT, word);
        if (v < 0 || n == NULL) return -1;
        n->home = (int16_t)h;
        n->value = homes[h] = (uint16_t)v;
        if (phi) g->imports[h] = (uint16_t)v;
    }
    return 0;
}

_PyJitGraph *
_PyJit_BuildGraph(const _PyJitRecord *record)
{
    if (!record->closed || record->aborted || record->count == 0 ||
        record->count > JIT_RECORD_EVENTS || record->nhomes == 0 ||
        record->nhomes > JIT_GRAPH_HOMES || record->nlocals > record->nhomes) return NULL;
    if (!_PyJit_ReserveBytes(sizeof(_PyJitGraph))) return NULL;
    _PyJitGraph *g = PyMem_Calloc(1, sizeof(*g));
    if (g == NULL) { _PyJit_ReleaseBytes(sizeof(*g)); return NULL; }
    g->charged_bytes = sizeof(*g);
    g->nhomes = record->nhomes;
    g->nlocals = record->nlocals;
    g->header = record->header;
    g->backedge = record->backedge;
    g->event_count = (uint16_t)record->count;
    for (uint32_t e = 0; e < record->count; e++) {
        /* A logical return PC has one canonical import adapter in this first
         * trace. Repeated occurrences need an explicit trace-position token. */
        for (uint32_t p = 0; p < e; p++) if (record->events[p].word == record->events[e].word) goto fail;
        g->record_words[e] = record->events[e].word;
        g->record_successors[e] = record->events[e].successor;
        g->python_calls += record->events[e].action == _PyJIT_ENTER_FRAME;
    }
    for (int h = 0; h < JIT_GRAPH_HOMES; h++) g->imports[h] = g->carried[h] = JIT_NO_VALUE;
    uint16_t homes[JIT_GRAPH_HOMES];
    if (PyErr_Occurred() || imports(g, homes, record->events[0].before, g->header, 1) < 0) goto fail;
    g->nimports = g->nnodes;
    for (int h = 0; h < g->nhomes; h++) g->boxed_imports += record->events[0].before[h].kind == JIT_HOME_BOXED;
    int effect = 0, arithmetic = 0;
    for (uint32_t e = 0; e < record->count; e++) {
        const JitRecordEvent *event = &record->events[e];
        int before = snapshot(g, homes, event->word, effect);
        if (before < 0) goto fail;
        int input[2] = {-1, -1}, dst = -1;
        for (int f = 0; f < 4; f++) {
            unsigned int role = event->effect.roles[f];
            unsigned int field = event->decoded.fields[f];
            if (role == _PyIR_ROLE_DST) dst = (int)field;
            if (role >= _PyIR_ROLE_IN0 && role < _PyIR_ROLE_IN0 + 2) input[role - _PyIR_ROLE_IN0] = (int)field;
        }
        int kind = JIT_SSA_HELPER;
        switch (event->decoded.handler) {
            case BINARY_OP_ADD: kind = JIT_SSA_ADD; break;
            case BINARY_OP_SUBTRACT: kind = JIT_SSA_SUB; break;
            case BINARY_OP_MULTIPLY: kind = JIT_SSA_MUL; break;
            case BINARY_OP_AND: kind = JIT_SSA_AND; break;
            case COMPARE_OP: case COMPARE_OP_NOFB:
                if (e + 1 < record->count && dst >= g->nlocals && dst < g->nhomes &&
                    (record->events[e+1].decoded.handler == POP_JUMP_IF_FALSE ||
                     record->events[e+1].decoded.handler == POP_JUMP_IF_TRUE) &&
                    record->events[e+1].decoded.fields[0] == (uint32_t)dst &&
                    (event->decoded.oparg >> 5) >= Py_LT && (event->decoded.oparg >> 5) <= Py_GE)
                    kind = JIT_SSA_CMP;
                break;
            case LOAD_SMALL_INT: case LOAD_SMALL_INT_INLINE:
                kind = JIT_SSA_CONST; break;
            case JUMP_BACKWARD:
                if (event->word == g->backedge && event->successor == g->header) kind = JIT_SSA_POLL;
                break;
        }
        int scalar = (kind >= JIT_SSA_ADD && kind <= JIT_SSA_AND) || kind == JIT_SSA_CMP;
        if (scalar) {
            for (int k = 0; k < 2; k++) {
                int h = input[k];
                if (h < 0 || h >= g->nhomes || homes[h] >= g->nvalues ||
                    (h >= g->nlocals && event->before[h].kind != JIT_HOME_INLINE &&
                     g->values[homes[h]].import_home >= 0)) kind = JIT_SSA_HELPER;
            }
        }
        if ((scalar || kind == JIT_SSA_CONST) && (dst < 0 || dst >= g->nhomes)) kind = JIT_SSA_HELPER;
        JitSsaNode *n = node(g, kind, event->word);
        if (n == NULL) goto fail;
        n->before = (uint16_t)before;
        n->successor = event->successor;
        n->effect = (uint16_t)effect;
        if (kind == JIT_SSA_CMP) {
            const JitRecordEvent *branch = &record->events[++e];
            int taken = branch->successor == (uint32_t)branch->decoded.oparg;
            int expected = branch->decoded.handler == POP_JUMP_IF_TRUE ? taken : !taken;
            n->left = homes[input[0]]; n->right = homes[input[1]];
            n->immediate = (event->decoded.oparg >> 5) | (expected << 8);
            n->successor = branch->successor;
            for (int k = 0; k < 2; k++) if (input[k] >= g->nlocals) homes[input[k]] = JIT_NULL_VALUE;
            homes[dst] = JIT_NULL_VALUE; /* ephemeral immortal bool, consumed by branch */
            int after = snapshot(g, homes, n->successor, effect);
            if (after < 0) goto fail;
            n->after = (uint16_t)after;
        }
        else if (kind == JIT_SSA_HELPER) {
            effect++;
            /* The completed helper owns a fully canonical frame, including
             * arbitrary callback changes. No pre-helper SSA fact survives. */
            for (int h = 0; h < g->nhomes; h++) homes[h] = JIT_NO_VALUE;
            int after = snapshot(g, homes, event->successor, effect);
            if (after < 0) goto fail;
            n->after = (uint16_t)after;
            int start = g->nnodes;
            if (imports(g, homes, event->after, event->successor, 0) < 0) goto fail;
            for (int j = start; j < g->nnodes; j++) {
                g->nodes[j].before = g->nodes[j].after = (uint16_t)after;
                g->nodes[j].effect = (uint16_t)effect;
            }
        }
        else if (kind == JIT_SSA_POLL) {
            effect++;
            n->after = n->before;
        }
        else {
            int id = value(g, -1);
            if (id < 0) goto fail;
            g->values[id].first = g->values[id].last = g->nnodes - 1;
            n->value = (uint16_t)id;
            n->home = (int16_t)dst;
            n->immediate = event->decoded.oparg;
            if (kind != JIT_SSA_CONST) {
                n->left = homes[input[0]];
                n->right = homes[input[1]];
                for (int k = 0; k < 2; k++) {
                    n->input_home[k] = (uint16_t)input[k];
                    if (input[k] >= g->nlocals) {
                        n->clears |= (uint8_t)(1u << k);
                        homes[input[k]] = JIT_NULL_VALUE;
                    }
                }
                arithmetic++;
            }
            homes[dst] = (uint16_t)id;
            /* Local commits close displaced owners; don't CSE across them. */
            if (dst < g->nlocals) effect++;
            int after = snapshot(g, homes, event->successor, effect);
            if (after < 0) goto fail;
            n->after = (uint16_t)after;
        }
    }
    for (int h = 0; h < g->nhomes; h++) {
        if (g->imports[h] == JIT_NO_VALUE) continue;
        if (homes[h] >= g->nvalues) goto fail;
        g->carried[h] = homes[h];
    }
    _PyJit_OptimizeGraph(g);
    if (arithmetic == 0 || _PyJit_VerifyGraph(g) < 0 || _PyJit_AllocateGraph(g) < 0) goto fail;
    return g;
fail:
    _PyJit_FreeGraph(g);
    return NULL;
}

static void
rename_value(_PyJitGraph *g, uint16_t old, uint16_t replacement)
{
    for (int i = 0; i < g->nnodes; i++) {
        JitSsaNode *n = &g->nodes[i];
        if (n->left == old) n->left = replacement;
        if (n->right == old) n->right = replacement;
    }
    for (int i = 0; i < g->nstates; i++) for (int h = 0; h < g->nhomes; h++)
        if (g->states[i].value[h] == old) g->states[i].value[h] = replacement;
    for (int h = 0; h < g->nhomes; h++) if (g->carried[h] == old) g->carried[h] = replacement;
}

void
_PyJit_OptimizeGraph(_PyJitGraph *g)
{
    /* Effects and ownership commits remain in source order. Only the scalar
     * calculation can be folded or reused across equal effect epochs. */
    for (int i = 0; i < g->nnodes; i++) {
        JitSsaNode *n = &g->nodes[i];
        if (n->kind < JIT_SSA_CONST || n->kind > JIT_SSA_AND || n->kind == JIT_SSA_PHI) continue;
        if (n->kind >= JIT_SSA_ADD && n->kind <= JIT_SSA_AND) {
            const JitSsaNode *left = NULL, *right = NULL;
            for (int j = 0; j < i; j++) if (g->nodes[j].kind == JIT_SSA_CONST) {
                if (g->nodes[j].value == n->left) left = &g->nodes[j];
                if (g->nodes[j].value == n->right) right = &g->nodes[j];
            }
            if (left != NULL && right != NULL) {
                int64_t a = left->immediate, b = right->immediate;
                int64_t result = n->kind == JIT_SSA_ADD ? a+b : n->kind == JIT_SSA_SUB ? a-b :
                    n->kind == JIT_SSA_MUL ? a*b : (int32_t)((uint32_t)a & (uint32_t)b);
                if (_PyValue_IntFitsInline(result)) {
                    n->kind = JIT_SSA_CONST;
                    n->immediate = (int32_t)result;
                    n->left = n->right = JIT_NO_VALUE;
                    g->optimized_values++;
                }
            }
        }
        for (int j = i - 1; j >= 0; j--) {
            JitSsaNode *p = &g->nodes[j];
            if (p->effect != n->effect) break;
            if (p->kind == n->kind && p->left == n->left && p->right == n->right &&
                p->immediate == n->immediate && p->value != JIT_NO_VALUE) {
                uint16_t old = n->value;
                n->value = p->value;
                n->kind = JIT_SSA_COMMIT;
                rename_value(g, old, n->value);
                g->optimized_values++;
                break;
            }
        }
    }
    /* Backward DCE from required recovery states, loop PHIs and effects.
     * Checked overflow operations remain observable recovery boundaries even
     * when their numeric result is unused. Merely defining a value is no root. */
    uint8_t used[JIT_GRAPH_VALUES] = {0};
    uint8_t required[JIT_GRAPH_STATES] = {0};
    uint16_t ids[JIT_GRAPH_VALUES];
    for (int i = 0; i < g->nnodes; i++) {
        JitSsaNode *n = &g->nodes[i];
        if ((n->kind >= JIT_SSA_ADD && n->kind <= JIT_SSA_MUL) ||
            n->kind == JIT_SSA_CMP || n->kind == JIT_SSA_HELPER || n->kind == JIT_SSA_POLL ||
            (n->home >= 0 && n->home < g->nlocals && n->kind != JIT_SSA_IMPORT && n->kind != JIT_SSA_PHI))
            required[n->before] = 1;
    }
    for (int s = 0; s < g->nstates; s++) if (required[s]) for (int h = 0; h < g->nhomes; h++)
        if (g->states[s].value[h] < g->nvalues) used[g->states[s].value[h]] = 1;
    for (int h = 0; h < g->nhomes; h++) {
        if (g->imports[h] < g->nvalues) used[g->imports[h]] = 1;
        if (g->carried[h] < g->nvalues) used[g->carried[h]] = 1;
    }
    for (int i = g->nnodes - 1; i >= 0; i--) {
        JitSsaNode *n = &g->nodes[i];
        int effect = n->kind == JIT_SSA_HELPER || n->kind == JIT_SSA_POLL || n->kind == JIT_SSA_CMP ||
            (n->kind >= JIT_SSA_ADD && n->kind <= JIT_SSA_MUL) ||
            (n->home >= 0 && n->home < g->nlocals && n->kind != JIT_SSA_IMPORT && n->kind != JIT_SSA_PHI);
        if (effect || (n->value < g->nvalues && used[n->value])) {
            if (n->value < g->nvalues) used[n->value] = 1;
            if (n->left < g->nvalues) used[n->left] = 1;
            if (n->right < g->nvalues) used[n->right] = 1;
        }
        else {
            n->kind = JIT_SSA_DEAD;
            n->value = n->left = n->right = JIT_NO_VALUE;
        }
    }
    int count = 0;
    for (int v = 0; v < g->nvalues; v++) {
        ids[v] = used[v] ? (uint16_t)count++ : JIT_NO_VALUE;
        if (used[v]) g->values[ids[v]] = g->values[v];
    }
    for (int i = 0; i < g->nnodes; i++) {
        JitSsaNode *n = &g->nodes[i];
        if (n->value < g->nvalues) n->value = ids[n->value];
        if (n->left < g->nvalues) n->left = ids[n->left];
        if (n->right < g->nvalues) n->right = ids[n->right];
    }
    for (int s = 0; s < g->nstates; s++) for (int h = 0; h < g->nhomes; h++)
        if (g->states[s].value[h] < g->nvalues) g->states[s].value[h] = ids[g->states[s].value[h]];
    for (int h = 0; h < g->nhomes; h++) {
        if (g->imports[h] < g->nvalues) g->imports[h] = ids[g->imports[h]];
        if (g->carried[h] < g->nvalues) g->carried[h] = ids[g->carried[h]];
    }
    g->nvalues = (uint16_t)count;
}

int
_PyJit_VerifyGraph(const _PyJitGraph *g)
{
    if (!g->nnodes || g->nnodes > JIT_GRAPH_NODES || g->nvalues > JIT_GRAPH_VALUES ||
        g->nstates > JIT_GRAPH_STATES || !g->nhomes || g->nhomes > JIT_GRAPH_HOMES ||
        g->nlocals > g->nhomes || g->nimports > g->nnodes ||
        g->map_entries != (uint32_t)g->nstates * g->nhomes ||
        g->map_entries > JIT_GRAPH_MAPS || g->event_count > JIT_RECORD_EVENTS ||
        g->nscratch > JIT_GRAPH_SPILLS || g->nspills > g->nscratch ||
        g->nodes[g->nnodes - 1].kind != JIT_SSA_POLL) return -1;
    for (int s = 0; s < g->nstates; s++) for (int h = 0; h < g->nhomes; h++) {
        uint16_t v = g->states[s].value[h];
        if (v != JIT_NO_VALUE && v != JIT_NULL_VALUE && v >= g->nvalues) return -1;
    }
    for (int v = 0; v < g->nvalues; v++) {
        const JitSsaValue *value = &g->values[v];
        if (value->first >= g->nnodes || value->last < value->first || value->last > g->nnodes ||
            value->import_home < -1 || value->import_home >= g->nhomes ||
            value->reg < -1 || value->reg >= 6 || value->spill < -1 ||
            value->spill >= JIT_GRAPH_SPILLS) return -1;
        const JitSsaNode *definition = &g->nodes[value->first];
        if (definition->value != v || definition->kind == JIT_SSA_DEAD ||
            definition->kind == JIT_SSA_COMMIT) return -1;
    }
    for (int h = 0; h < g->nhomes; h++) {
        uint16_t v = g->imports[h], carried = g->carried[h];
        if (v == JIT_NO_VALUE) { if (carried != JIT_NO_VALUE) return -1; }
        else if (v >= g->nvalues || carried >= g->nvalues ||
                 g->values[v].import_home != h || g->values[v].first >= g->nimports ||
                 g->nodes[g->values[v].first].kind != JIT_SSA_PHI) return -1;
    }
    for (int i = 0; i < g->nnodes; i++) {
        const JitSsaNode *n = &g->nodes[i];
        int importing = n->kind == JIT_SSA_IMPORT || n->kind == JIT_SSA_PHI;
        if (n->kind > JIT_SSA_DEAD || (n->clears & ~3)) return -1;
        if (n->kind == JIT_SSA_DEAD) continue;
        if (importing) {
            if (n->home < 0 || n->home >= g->nhomes || n->value >= g->nvalues ||
                g->values[n->value].import_home != n->home ||
                g->values[n->value].first != i ||
                (n->kind == JIT_SSA_PHI && (i >= g->nimports || n->home >= g->nlocals))) return -1;
        }
        else if (n->before >= g->nstates || n->after >= g->nstates) return -1;
        /* Import nodes read canonical homes, not a virtual predecessor map.
         * Every other BEFORE recipe must already dominate this operation. */
        if (!importing) for (int k = 0; k < 2; k++) {
            const JitFrameState *s = &g->states[k ? n->after : n->before];
            for (int h = 0; h < g->nhomes; h++) {
                uint16_t v = s->value[h];
                if (v < g->nvalues && g->values[v].first > i - (k == 0)) return -1;
            }
        }
        if (n->value != JIT_NO_VALUE && n->value >= g->nvalues) return -1;
        if (((n->kind >= JIT_SSA_ADD && n->kind <= JIT_SSA_AND) || n->kind == JIT_SSA_CMP) &&
            (n->left >= g->nvalues || n->right >= g->nvalues ||
             g->values[n->left].first >= i || g->values[n->right].first >= i)) return -1;
        if ((n->kind >= JIT_SSA_CONST && n->kind <= JIT_SSA_AND && n->kind != JIT_SSA_PHI) ||
            n->kind == JIT_SSA_COMMIT) {
            if (n->home < 0 || n->home >= g->nhomes || n->value >= g->nvalues) return -1;
            if (n->kind == JIT_SSA_COMMIT && g->values[n->value].first >= i) return -1;
        }
        if (n->kind >= JIT_SSA_ADD && n->kind <= JIT_SSA_AND) {
            for (int k = 0; k < 2; k++) if (n->input_home[k] >= g->nhomes ||
                ((n->clears & (1 << k)) && n->input_home[k] < g->nlocals)) return -1;
        }
        if (n->kind == JIT_SSA_CMP &&
            ((n->immediate & 255) > Py_GE || (n->immediate & ~0x1ff))) return -1;
    }
    return 0;
}

void
_PyJit_FreeGraph(_PyJitGraph *graph)
{
    if (graph == NULL) return;
    size_t bytes = graph->charged_bytes;
    PyMem_Free(graph);
    _PyJit_ReleaseBytes(bytes);
}

int
_PyJit_LowerGraph(_PyJitCodeUnit *unit)
{
    _PyJitGraph *g = unit->graph;
    if (_PyJit_VerifyGraph(g) < 0 || unit->nops != unit->nwords) return -1;
    if (g->header >= unit->nwords || g->backedge >= unit->nwords || g->header >= g->backedge) return -1;
    for (int s = 0; s < g->nstates; s++) if (g->states[s].word >= unit->nwords) return -1;
    for (int e = 0; e < g->event_count; e++) if (g->record_words[e] >= unit->nwords ||
        g->record_successors[e] >= unit->nwords) return -1;
    for (int v = 0; v < g->nvalues; v++) {
        if (g->values[v].spill < 0 || g->values[v].spill >= g->nscratch) return -1;
    }
    size_t count = (size_t)unit->nwords + g->nnodes;
    size_t bytes = count * sizeof(*unit->ops);
    size_t old_bytes = (size_t)unit->nops * sizeof(*unit->ops);
    /* Realloc may temporarily retain both allocations inside an allocator
     * hook. Reserve the full replacement, not merely its eventual growth. */
    if (!_PyJit_ReserveBytes(bytes)) return -1;
    _PyJitLirOp *ops = PyMem_Realloc(unit->ops, count * sizeof(*ops));
    if (ops == NULL) { _PyJit_ReleaseBytes(bytes); return -1; }
    unit->ops = ops;
    unit->charged_bytes += bytes - old_bytes;
    _PyJit_ReleaseBytes(old_bytes);
    if (PyErr_Occurred()) return -1;
    for (int i = 0; i < g->nnodes; i++) {
        JitSsaNode *n = &g->nodes[i];
        if (n->word >= unit->nwords || n->successor >= unit->nwords) return -1;
        _PyJitLirOp *op = &ops[unit->nwords + i];
        *op = ops[n->word];
        op->kind = (_PyJitLirKind)(_PyJIT_LIR_IMPORT + n->kind);
        op->node = (uint16_t)i;
        op->next_op = unit->nwords + i + 1;
        op->target_op = unit->nwords + g->nimports;
        op->word = n->word;
    }
    unit->nops = (uint32_t)count;
    return 0;
}
