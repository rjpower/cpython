#include "Python.h"
#include "pycore_jit.h"
#include "pycore_pyerrors.h"
#include <stddef.h>
#if _PyJIT_BASELINE_SUPPORTED
#  include <sys/mman.h>
#  include <unistd.h>
#  include "jit_x86.h"

static void
restore_backend_error(PyObject *saved)
{
    if (saved != NULL) {
        if (PyErr_Occurred()) {
            _PyErr_ChainExceptions1(saved);
        }
        else {
            PyErr_SetRaisedException(saved);
        }
    }
}

static void
free_buffer(jit_buf *buffer)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyMem_Free(buffer->code);
    restore_backend_error(saved);
    saved = PyErr_GetRaisedException();
    PyMem_Free(buffer->labels);
    restore_backend_error(saved);
    saved = PyErr_GetRaisedException();
    PyMem_Free(buffer->fixups);
    restore_backend_error(saved);
}

static void
free_labels(int *labels)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyMem_Free(labels);
    restore_backend_error(saved);
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
#endif

int
_PyJit_EmitBaseline(_PyJitCodeUnit *unit)
{
#if !_PyJIT_BASELINE_SUPPORTED
    return -1;
#else
    if (unit->nops == 0 || unit->nops > 4096 || unit->ops == NULL || unit->offsets == NULL) {
        return -1;
    }
    for (uint32_t i = 0; i < unit->nops; i++) {
        _PyJitLirOp *op = &unit->ops[i];
        if (op->word != i || op->kind > _PyJIT_LIR_EXIT ||
            (op->kind != _PyJIT_LIR_EXIT && op->helper == NULL) ||
            (op->kind == _PyJIT_LIR_BRANCH && op->target >= unit->nops) ||
            op->fallthrough > unit->nops) {
            return -1;
        }
    }
    jit_buf buffer = {0};
    int epilogue = jit_new_label(&buffer);
    if (buffer.failed || PyErr_Occurred()) {
        free_buffer(&buffer);
        return -1;
    }
    int *labels = PyMem_Malloc(unit->nops * sizeof(*labels));
    if (labels == NULL || PyErr_Occurred()) {
        free_labels(labels);
        free_buffer(&buffer);
        return -1;
    }
    for (uint32_t i = 0; i < unit->nops; i++) {
        labels[i] = jit_new_label(&buffer);
    }
    endbr(&buffer);
    jit_push(&buffer, X86_R12);  /* incoming rsp=8mod16, now aligned for C calls */
    jit_mov(&buffer, X86_R12, X86_RDI);
    call_absolute(&buffer, (uintptr_t)_PyJit_NativeEntered);
    jit_load(&buffer, X86_RAX, X86_R12, offsetof(_PyJitContext, entry_target));
    jit_jmp_reg(&buffer, X86_RAX); /* target chosen/validated by the pinned unit */
    for (uint32_t i = 0; i < unit->nops; i++) {
        _PyJitLirOp *op = &unit->ops[i];
        jit_bind(&buffer, labels[i]);
        unit->offsets[i] = buffer.size;
        endbr(&buffer); /* every resume target is an IBT indirect-jump target */
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
        call_absolute(&buffer, (uintptr_t)_PyJit_AfterHelper);
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
            uintptr_t target = (uintptr_t)op->decoded.instruction +
                ((uintptr_t)op->target - i) * sizeof(_PyIRWord);
            jit_mov_imm64(&buffer, X86_RCX,
                          target);
            jit_cmp(&buffer, X86_RAX, X86_RCX);
            jit_jcc(&buffer, X86_E, labels[op->target]);
        }
        if (op->fallthrough < unit->nops) {
            jit_jmp(&buffer, labels[op->fallthrough]);
        }
        else {
            /* A validated unconditional final branch always took its target.
             * An impossible helper result must never wrap to word zero. */
            jit_byte(&buffer, 0x0f); jit_byte(&buffer, 0x0b);
        }
    }
    jit_bind(&buffer, epilogue);
    jit_pop(&buffer, X86_R12);
    jit_ret(&buffer);
    int ok = jit_resolve(&buffer);
    free_labels(labels);
    if (ok < 0 || PyErr_Occurred()) {
        free_buffer(&buffer);
        return -1;
    }
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0 || buffer.size <= 0 || (size_t)buffer.size > SIZE_MAX - (size_t)page) {
        free_buffer(&buffer);
        return -1;
    }
    size_t size = ((size_t)buffer.size + (size_t)page - 1) / (size_t)page * (size_t)page;
    void *mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED || mapping == NULL) {
        if (mapping == NULL) {
            munmap(mapping, size);
        }
        free_buffer(&buffer);
        return -1;
    }
    memcpy(mapping, buffer.code, (size_t)buffer.size);
    if (mprotect(mapping, size, PROT_READ | PROT_EXEC) < 0) {
        munmap(mapping, size);
        free_buffer(&buffer);
        return -1;
    }
    unit->mapping = mapping;
    unit->mapping_size = size;
    unit->code_size = (size_t)buffer.size;
    unit->entry = (_PyJitEntry)mapping;
    free_buffer(&buffer);
    return PyErr_Occurred() ? -1 : 0;
#endif
}
