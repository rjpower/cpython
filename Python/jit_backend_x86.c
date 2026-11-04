#include "Python.h"
#include "pycore_jit.h"
#include "pycore_jit_ir.h"
#include "pycore_pystate.h"
#include "pycore_ceval.h"
#include <stddef.h>
#if _PyJIT_BASELINE_SUPPORTED
#  include <sys/mman.h>
#  include <unistd.h>
#  include "jit_x86.h"

static void
free_buffer(jit_buf *buffer)
{
    PyMem_Free(buffer->code);
    _PyJit_ReleaseBytes((size_t)buffer->capacity);
    PyMem_Free(buffer->labels);
    _PyJit_ReleaseBytes((size_t)buffer->labels_capacity * sizeof(*buffer->labels));
    PyMem_Free(buffer->fixups);
    _PyJit_ReleaseBytes((size_t)buffer->fixups_capacity * sizeof(*buffer->fixups));
}

static void
free_labels(int *labels, size_t bytes)
{
    PyMem_Free(labels);
    _PyJit_ReleaseBytes(bytes);
}

static void
endbr(jit_buf *buffer)
{
    jit_byte(buffer, 0xf3); jit_byte(buffer, 0x0f);
    jit_byte(buffer, 0x1e); jit_byte(buffer, 0xfa);
}

static void
call_absolute(jit_buf *buffer, uintptr_t address)
{
    jit_mov_imm64(buffer, X86_RAX, address);
    jit_call_reg(buffer, X86_RAX);
}

static const jit_reg scalar_regs[6] = {X86_RAX, X86_RCX, X86_RDX, X86_R8, X86_R9, X86_R10};

static void
read_value(jit_buf *b, _PyJitGraph *g, uint16_t id, jit_reg dst)
{
    JitSsaValue *v = &g->values[id];
    if (v->reg >= 0) jit_mov32(b, dst, scalar_regs[v->reg]);
    else jit_load32(b, dst, X86_RSP, v->spill * 8);
}

static void
write_value(jit_buf *b, _PyJitGraph *g, uint16_t id, jit_reg src)
{
    JitSsaValue *v = &g->values[id];
    if (v->reg >= 0) jit_mov32(b, scalar_regs[v->reg], src);
    else jit_store(b, X86_RSP, v->spill * 8, src);
}

static void
save_values(jit_buf *b, _PyJitGraph *g, int at, int defined)
{
    for (int id = 0; id < g->nvalues; id++) {
        JitSsaValue *v = &g->values[id];
        if (v->reg >= 0 && (v->first < at || (defined && v->first == at)) && v->last >= at)
            jit_store(b, X86_RSP, v->spill * 8, scalar_regs[v->reg]);
    }
}

static void
reload_values(jit_buf *b, _PyJitGraph *g, int at)
{
    for (int id = 0; id < g->nvalues; id++) {
        JitSsaValue *v = &g->values[id];
        if (v->reg >= 0 && v->first <= at && v->last >= at)
            jit_load32(b, scalar_regs[v->reg], X86_RSP, v->spill * 8);
    }
}

static void
scalar_call(jit_buf *b, uintptr_t function, uintptr_t data)
{
    jit_mov(b, X86_RDI, X86_R12);
    jit_mov_imm64(b, X86_RSI, data);
    call_absolute(b, function);
}

static void
scalar_result(jit_buf *b, int epilogue, int fallback)
{
    jit_cmp_imm32(b, X86_RAX, _PyJIT_BRANCH);
    jit_jcc(b, X86_E, fallback);
    jit_cmp_imm32(b, X86_RAX, _PyJIT_CONTINUE);
    jit_jcc(b, X86_NE, epilogue);
}

static void
phi_moves(jit_buf *b, _PyJitGraph *g)
{
    int source[JIT_GRAPH_HOMES], dest[JIT_GRAPH_HOMES], count = 0;
    for (int h = 0; h < g->nhomes; h++) if (g->imports[h] != JIT_NO_VALUE) {
        JitSsaValue *input = &g->values[g->carried[h]], *output = &g->values[g->imports[h]];
        int s = input->reg >= 0 ? input->reg : 100 + input->spill;
        int d = output->reg >= 0 ? output->reg : 100 + output->spill;
        if (s != d) { source[count] = s; dest[count++] = d; }
    }
    /* Parallel physical copies: register PHIs stay registers. R11 holds one
     * cycle value; RDI handles other moves, including actual pressure spills. */
    while (count) {
        int ready = -1;
        for (int i = 0; i < count; i++) {
            int used = 0;
            for (int j = 0; j < count; j++) used |= source[j] == dest[i];
            if (!used) { ready = i; break; }
        }
        if (ready < 0) {
            int saved = dest[0];
            if (saved < 100) jit_mov32(b, X86_R11, scalar_regs[saved]);
            else jit_load32(b, X86_R11, X86_RSP, (saved - 100) * 8);
            for (int j = 0; j < count; j++) if (source[j] == saved) source[j] = -2;
            continue;
        }
        if (source[ready] == -2) jit_mov32(b, X86_RDI, X86_R11);
        else if (source[ready] < 100) jit_mov32(b, X86_RDI, scalar_regs[source[ready]]);
        else jit_load32(b, X86_RDI, X86_RSP, (source[ready] - 100) * 8);
        if (dest[ready] < 100) jit_mov32(b, scalar_regs[dest[ready]], X86_RDI);
        else jit_store(b, X86_RSP, (dest[ready] - 100) * 8, X86_RDI);
        count--;
        source[ready] = source[count]; dest[ready] = dest[count];
    }
}

static void
trace_after(jit_buf *b, const JitSsaNode *n)
{
    jit_mov32(b, X86_RSI, X86_RAX);
    jit_mov(b, X86_RDI, X86_R12);
    jit_mov_imm64(b, X86_RDX, (uintptr_t)n);
    call_absolute(b, (uintptr_t)_PyJit_TraceAfter);
}

static void
trace_witness(jit_buf *b, int offset)
{
    int seen = jit_new_label(b);
    jit_load(b, X86_RDI, X86_R12, offsetof(_PyJitContext, unit));
    jit_op_mem(b, 0, 0x80, 7, X86_RDI, offset); /* cmp byte [unit+offset], 0 */
    jit_byte(b, 0);
    jit_jcc(b, X86_NE, seen);
    jit_op_mem(b, 0, 0xC6, 0, X86_RDI, offset);
    jit_byte(b, 1);
    jit_bind(b, seen);
}

static void
emit_graph(jit_buf *b, _PyJitCodeUnit *unit, int *labels, int epilogue, int fallback)
{
    _PyJitGraph *g = unit->graph;
    for (int at = 0; at < g->nnodes; at++) {
        uint32_t index = unit->nwords + (uint32_t)at;
        _PyJitLirOp *op = &unit->ops[index];
        JitSsaNode *n = &g->nodes[op->node];
        jit_bind(b, labels[index]);
        unit->op_offsets[index] = b->size;
        endbr(b);
        if (at == 0 || (n->kind == JIT_SSA_IMPORT && at > 0 && g->nodes[at - 1].kind == JIT_SSA_HELPER))
            unit->offsets[n->word] = b->size - 4;
        int guard = jit_new_label(b), done = jit_new_label(b);
        if (op->kind == _PyJIT_LIR_DEAD) {
            /* No calculation or owner effect survives the required snapshots. */
        }
        else if (op->kind == _PyJIT_LIR_IMPORT || op->kind == _PyJIT_LIR_PHI) {
            save_values(b, g, at, 0);
            scalar_call(b, (uintptr_t)_PyJit_TraceImport, (uintptr_t)n);
            scalar_result(b, epilogue, fallback);
            reload_values(b, g, at);
            if (at + 1 == g->nnodes ||
                (g->nodes[at + 1].kind != JIT_SSA_IMPORT && g->nodes[at + 1].kind != JIT_SSA_PHI)) {
                save_values(b, g, at + 1, 0);
                jit_mov(b, X86_RDI, X86_R12);
                call_absolute(b, (uintptr_t)_PyJit_TraceEntered);
                reload_values(b, g, at);
            }
        }
        else if (op->kind == _PyJIT_LIR_TRACE_HELPER) {
            save_values(b, g, at, 0);
            scalar_call(b, (uintptr_t)_PyJit_TracePublish, (uintptr_t)&g->states[n->before]);
            scalar_result(b, epilogue, fallback);
            scalar_call(b, (uintptr_t)op->helper, (uintptr_t)&op->decoded);
            trace_after(b, n);
            scalar_result(b, epilogue, fallback);
            /* New IMPORT definitions, not stale pre-call registers, follow. */
        }
        else if (op->kind == _PyJIT_LIR_POLL) {
            int slow = jit_new_label(b);
            jit_load(b, X86_RDI, X86_R12, offsetof(_PyJitContext, tstate));
            jit_load(b, X86_RSI, X86_RDI, offsetof(PyThreadState, eval_breaker));
            jit_test_imm(b, X86_RSI, _PY_EVAL_EVENTS_MASK);
            jit_jcc(b, X86_NE, slow);
            jit_mov(b, X86_R11, X86_RSI);
            jit_and_imm(b, X86_R11, ~_PY_EVAL_EVENTS_MASK);
            jit_mov_imm64(b, X86_RDI, unit->epoch);
            jit_cmp(b, X86_R11, X86_RDI);
            jit_jcc(b, X86_NE, slow);
            trace_witness(b, offsetof(_PyJitCodeUnit, trace_backedge_seen));
            phi_moves(b, g);
            jit_jmp(b, labels[unit->nwords + g->nimports]);
            jit_bind(b, slow);
            save_values(b, g, at, 0);
            scalar_call(b, (uintptr_t)_PyJit_TracePublish, (uintptr_t)&g->states[n->before]);
            scalar_result(b, epilogue, fallback);
            scalar_call(b, (uintptr_t)op->helper, (uintptr_t)&op->decoded);
            trace_after(b, n);
            scalar_result(b, epilogue, fallback);
            jit_jmp(b, fallback);
        }
        else if (op->kind == _PyJIT_LIR_CMP) {
            static const jit_cond condition[] = {X86_L, X86_LE, X86_E, X86_NE, X86_G, X86_GE};
            read_value(b, g, n->left, X86_R11);
            read_value(b, g, n->right, X86_RDI);
            jit_cmp32(b, X86_R11, X86_RDI);
            jit_cond failed = condition[n->immediate & 0xff];
            if (n->immediate & 256) failed = (jit_cond)(failed ^ 1);
            jit_jcc(b, failed, guard);
        }
        else {
            if (op->kind == _PyJIT_LIR_CONST) jit_mov_imm32(b, X86_R11, (uint32_t)n->immediate);
            else if (op->kind == _PyJIT_LIR_COMMIT) read_value(b, g, n->value, X86_R11);
            else {
                read_value(b, g, n->left, X86_R11);
                read_value(b, g, n->right, X86_RDI);
                if (op->kind == _PyJIT_LIR_ADD) jit_add32(b, X86_R11, X86_RDI);
                else if (op->kind == _PyJIT_LIR_SUB) jit_sub32(b, X86_R11, X86_RDI);
                else if (op->kind == _PyJIT_LIR_MUL) jit_imul32(b, X86_R11, X86_RDI);
                else jit_op_reg(b, 0, 0x21, X86_RDI, X86_R11);
                if (op->kind != _PyJIT_LIR_AND) jit_jcc(b, X86_O, guard);
            }
            write_value(b, g, n->value, X86_R11);
            trace_witness(b, offsetof(_PyJitCodeUnit, trace_scalar_seen));
            if (n->home < g->nlocals) {
                int inline_old = jit_new_label(b);
                jit_load(b, X86_RSI, X86_R12, offsetof(_PyJitContext, frame));
                jit_load(b, X86_RSI, X86_RSI, offsetof(_PyInterpreterFrame, base));
                jit_load16(b, X86_RDI, X86_RSI, n->home * 8 + 6);
                jit_cmp_imm32(b, X86_RDI, 2); /* IsInline, including debug Values */
                jit_jcc(b, X86_AE, inline_old);
                save_values(b, g, at, 1);
                scalar_call(b, (uintptr_t)_PyJit_TraceCommit, (uintptr_t)n);
                scalar_result(b, epilogue, fallback);
                reload_values(b, g, at);
                jit_jmp(b, done);
                jit_bind(b, inline_old);
                read_value(b, g, n->value, X86_R11);
                jit_mov_imm64(b, X86_RDI, Py_INT_BASE);
                jit_add(b, X86_R11, X86_RDI);
                jit_store(b, X86_RSI, n->home * 8, X86_R11);
            }
        }
        jit_bind(b, done);
        if (op->next_op < unit->nops) jit_jmp(b, labels[op->next_op]);
        else jit_jmp(b, epilogue);
        jit_bind(b, guard);
        if (n->before < g->nstates) {
            save_values(b, g, at, 0);
            scalar_call(b, (uintptr_t)_PyJit_TracePublish, (uintptr_t)&g->states[n->before]);
            scalar_result(b, epilogue, fallback);
            scalar_call(b, (uintptr_t)_PyJit_TraceFallback, n->word);
            scalar_result(b, epilogue, fallback);
            jit_jmp(b, fallback);
        }
        else jit_jmp(b, epilogue); /* import failures use TraceImport's fallback */
    }
}
#endif

int
_PyJit_EmitBaseline(_PyJitCodeUnit *unit)
{
#if !_PyJIT_BASELINE_SUPPORTED
    return -1;
#else
    if (unit->nwords == 0 || unit->nwords > 4096 || unit->nops > 4096 + JIT_GRAPH_NODES ||
        unit->ops == NULL || unit->offsets == NULL) {
        return -1;
    }
    for (uint32_t i = 0; i < unit->nwords; i++) {
        _PyJitLirOp *op = &unit->ops[i];
        if (op->word != i || op->kind > _PyJIT_LIR_EXIT ||
            (op->kind != _PyJIT_LIR_EXIT && op->helper == NULL) ||
            (op->kind == _PyJIT_LIR_BRANCH && op->target >= unit->nops) ||
            op->fallthrough > unit->nwords) {
            return -1;
        }
    }
    jit_buf buffer = {0};
    int epilogue = jit_new_label(&buffer);
    int fallback = jit_new_label(&buffer);
    if (buffer.failed) {
        free_buffer(&buffer);
        return -1;
    }
    size_t label_bytes = unit->nops * sizeof(int);
    if (!_PyJit_ReserveBytes(label_bytes)) { free_buffer(&buffer); return -1; }
    int *labels = PyMem_Malloc(label_bytes);
    if (labels == NULL) {
        free_labels(labels, label_bytes);
        free_buffer(&buffer);
        return -1;
    }
    size_t offset_bytes = unit->nops * sizeof(*unit->op_offsets);
    if (!_PyJit_ReserveBytes(offset_bytes)) {
        free_labels(labels, label_bytes); free_buffer(&buffer); return -1;
    }
    unit->charged_bytes += offset_bytes;
    unit->op_offsets = PyMem_Malloc(offset_bytes);
    if (unit->op_offsets == NULL) {
        free_labels(labels, label_bytes); free_buffer(&buffer); return -1;
    }
    for (uint32_t i = 0; i < unit->nops; i++) {
        labels[i] = jit_new_label(&buffer);
    }
    endbr(&buffer);
    jit_push(&buffer, X86_R12);  /* incoming rsp=8mod16, now aligned for C calls */
    jit_mov(&buffer, X86_R12, X86_RDI);
    int stack_size = unit->graph ? ((unit->graph->nscratch * 8 + 15) & ~15) : 0;
    if (stack_size) {
        jit_sub_imm(&buffer, X86_RSP, stack_size);
        jit_store(&buffer, X86_R12, offsetof(_PyJitContext, scratch), X86_RSP);
    }
    call_absolute(&buffer, (uintptr_t)_PyJit_NativeEntered);
    jit_load(&buffer, X86_RAX, X86_R12, offsetof(_PyJitContext, entry_target));
    jit_jmp_reg(&buffer, X86_RAX); /* target chosen/validated by the pinned unit */
    for (uint32_t i = 0; i < unit->nwords; i++) {
        _PyJitLirOp *op = &unit->ops[i];
        jit_bind(&buffer, labels[i]);
        unit->offsets[i] = buffer.size;
        unit->op_offsets[i] = buffer.size;
        endbr(&buffer); /* every resume target is an IBT indirect-jump target */
        if (unit->recording) {
            scalar_call(&buffer, (uintptr_t)_PyJit_RecordBefore, (uintptr_t)op);
            jit_cmp_imm32(&buffer, X86_RAX, _PyJIT_CONTINUE);
            jit_jcc(&buffer, X86_NE, epilogue);
        }
        jit_mov(&buffer, X86_RDI, X86_R12);
        jit_mov_imm64(&buffer, X86_RSI, (uintptr_t)&op->decoded);
        if (op->kind == _PyJIT_LIR_EXIT) {
            call_absolute(&buffer, (uintptr_t)_PyJit_ExitBefore);
            jit_jmp(&buffer, epilogue);
            continue;
        }
        call_absolute(&buffer, (uintptr_t)op->helper);
        jit_mov32(&buffer, X86_RSI, X86_RAX);
        jit_mov(&buffer, X86_RDI, X86_R12);
        jit_mov_imm64(&buffer, X86_RDX, (uintptr_t)op);
        call_absolute(&buffer, unit->recording ? (uintptr_t)_PyJit_RecordAfter : (uintptr_t)_PyJit_AfterHelper);
        int flow = jit_new_label(&buffer);
        jit_cmp_imm32(&buffer, X86_RAX, _PyJIT_CONTINUE);
        jit_jcc(&buffer, X86_E, flow);
        jit_cmp_imm32(&buffer, X86_RAX, _PyJIT_BRANCH);
        jit_jcc(&buffer, X86_NE, epilogue);
        jit_bind(&buffer, flow);
        if (op->kind == _PyJIT_LIR_BRANCH) {
            jit_load(&buffer, X86_RAX, X86_R12, offsetof(_PyJitContext, next_instr));
            /* Buffer allocation can reenter and replace the borrowed exec.
             * Encode addresses as integers; CompileCode rejects that stale
             * transaction before publication without touching freed words. */
            uintptr_t target_address = (uintptr_t)op->decoded.instruction +
                ((uintptr_t)op->target - i) * sizeof(_PyIRWord);
            jit_mov_imm64(&buffer, X86_RCX,
                          target_address);
            jit_cmp(&buffer, X86_RAX, X86_RCX);
            uint32_t target = unit->graph && op->target == unit->graph->header ? unit->nwords : op->target_op;
            jit_jcc(&buffer, X86_E, labels[target]);
        }
        if (op->fallthrough < unit->nops) {
            uint32_t next = unit->graph && op->fallthrough == unit->graph->header ? unit->nwords : op->next_op;
            jit_jmp(&buffer, labels[next]);
        }
        else {
            /* A validated unconditional final branch always took its target.
             * An impossible helper result must never wrap to word zero. */
            jit_byte(&buffer, 0x0f); jit_byte(&buffer, 0x0b);
        }
    }
    if (unit->graph != NULL) emit_graph(&buffer, unit, labels, epilogue, fallback);
    jit_bind(&buffer, fallback);
    jit_load(&buffer, X86_RAX, X86_R12, offsetof(_PyJitContext, entry_target));
    jit_jmp_reg(&buffer, X86_RAX);
    jit_bind(&buffer, epilogue);
    if (stack_size) {
        jit_mov_imm32(&buffer, X86_RDI, 0);
        jit_store(&buffer, X86_R12, offsetof(_PyJitContext, scratch), X86_RDI);
    }
    if (stack_size) jit_sub_imm(&buffer, X86_RSP, -stack_size);
    jit_pop(&buffer, X86_R12);
    jit_ret(&buffer);
    int ok = jit_resolve(&buffer);
    free_labels(labels, label_bytes);
    if (ok < 0) {
        free_buffer(&buffer);
        return -1;
    }
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0 || buffer.size <= 0 || (size_t)buffer.size > SIZE_MAX - (size_t)page) {
        free_buffer(&buffer);
        return -1;
    }
    size_t size = ((size_t)buffer.size + (size_t)page - 1) / (size_t)page * (size_t)page;
    if (!_PyJit_ReserveBytes(size)) { free_buffer(&buffer); return -1; }
    void *mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED || mapping == NULL) {
        if (mapping == NULL) {
            munmap(mapping, size);
        }
        _PyJit_ReleaseBytes(size);
        free_buffer(&buffer);
        return -1;
    }
    memcpy(mapping, buffer.code, (size_t)buffer.size);
    if (mprotect(mapping, size, PROT_READ | PROT_EXEC) < 0) {
        munmap(mapping, size);
        _PyJit_ReleaseBytes(size);
        free_buffer(&buffer);
        return -1;
    }
    unit->mapping = mapping;
    unit->mapping_size = size;
    unit->charged_bytes += size;
    unit->code_size = (size_t)buffer.size;
    unit->entry = (_PyJitEntry)mapping;
    free_buffer(&buffer);
    return 0;
#endif
}
