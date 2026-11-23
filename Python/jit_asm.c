/* An x86-64 encoder for the baseline JIT; see pycore_jit_asm.h. */

#include "Python.h"
#include "pycore_jit_asm.h"

#ifdef _PY_JIT_SUPPORTED

#define UNBOUND SIZE_MAX
// The cold section starts at a multiple of this, which bounds _PyJitAsm_Align.
#define COLD_ALIGNMENT 64

void
_PyJitAsm_Init(_PyJitAsm *a)
{
    *a = (_PyJitAsm){0};
}

void
_PyJitAsm_Fini(_PyJitAsm *a)
{
    PyMem_RawFree(a->sections[_PyJitSection_HOT].buffer);
    PyMem_RawFree(a->sections[_PyJitSection_COLD].buffer);
    PyMem_RawFree(a->labels);
    PyMem_RawFree(a->fixups);
    *a = (_PyJitAsm){0};
}

/* Make room for `count` more elements of a growable array. */
static int
grow_array(void **array, size_t size, size_t *capacity, size_t count,
           size_t element, size_t initial)
{
    if (size + count <= *capacity) {
        return 0;
    }
    size_t wanted = Py_MAX(Py_MAX(*capacity * 2, initial), size + count);
    void *grown = PyMem_RawRealloc(*array, wanted * element);
    if (grown == NULL) {
        return -1;
    }
    *array = grown;
    *capacity = wanted;
    return 0;
}

/* Reserve `count` bytes at the end of the current section; NULL if the
   assembler has failed. */
static uint8_t *
append(_PyJitAsm *a, size_t count)
{
    _PyJitBuffer *buffer = &a->sections[a->current];
    if (a->failed
        || grow_array((void **)&buffer->buffer, buffer->size,
                      &buffer->capacity, count, 1, 256) < 0)
    {
        a->failed = 1;
        return NULL;
    }
    uint8_t *p = buffer->buffer + buffer->size;
    buffer->size += count;
    return p;
}

static void
emit_u8(_PyJitAsm *a, uint8_t value)
{
    uint8_t *p = append(a, 1);
    if (p != NULL) {
        p[0] = value;
    }
}

static void
emit_u32(_PyJitAsm *a, uint32_t value)
{
    uint8_t *p = append(a, 4);
    if (p != NULL) {
        memcpy(p, &value, 4);    // x86-64 is little-endian
    }
}

static void
emit_u64(_PyJitAsm *a, uint64_t value)
{
    uint8_t *p = append(a, 8);
    if (p != NULL) {
        memcpy(p, &value, 8);
    }
}

static void
add_fixup(_PyJitAsm *a, _PyJitFixupKind kind, _PyJitLabel label,
          const void *target)
{
    if (a->failed
        || grow_array((void **)&a->fixups, a->nfixups, &a->fixups_capacity, 1,
                      sizeof(_PyJitFixup), 16) < 0)
    {
        a->failed = 1;
        return;
    }
    _PyJitFixup *fixup = &a->fixups[a->nfixups++];
    fixup->kind = kind;
    fixup->section = a->current;
    fixup->offset = a->sections[a->current].size;
    if (kind == _PyJitFixup_REL32_ABSOLUTE) {
        fixup->target = target;
    }
    else {
        fixup->label = label;
    }
}

_PyJitLabel
_PyJitAsm_NewLabel(_PyJitAsm *a)
{
    if (a->failed
        || grow_array((void **)&a->labels, a->nlabels,
                      &a->labels_capacity, 1, sizeof(_PyJitLabelInfo), 16) < 0)
    {
        a->failed = 1;
        return 0;
    }
    a->labels[a->nlabels].offset = UNBOUND;
    return (_PyJitLabel)a->nlabels++;
}

void
_PyJitAsm_Bind(_PyJitAsm *a, _PyJitLabel label)
{
    assert((label >= 0 && (size_t)label < a->nlabels) || a->failed);
    if (!a->failed) {
        assert(a->labels[label].offset == UNBOUND);
        a->labels[label] = (_PyJitLabelInfo){
            a->sections[a->current].size, a->current};
    }
}

/* Where a section starts in the placed code. */
static size_t
section_base(const _PyJitAsm *a, _PyJitSection section)
{
    if (section == _PyJitSection_HOT) {
        return 0;
    }
    size_t hot = a->sections[_PyJitSection_HOT].size;
    return (hot + COLD_ALIGNMENT - 1) & ~(size_t)(COLD_ALIGNMENT - 1);
}

void
_PyJitAsm_Section(_PyJitAsm *a, _PyJitSection section)
{
    a->current = section;
}

size_t
_PyJitAsm_Size(const _PyJitAsm *a)
{
    size_t cold = a->sections[_PyJitSection_COLD].size;
    return cold == 0 ? a->sections[_PyJitSection_HOT].size
                     : section_base(a, _PyJitSection_COLD) + cold;
}

size_t
_PyJitAsm_LabelOffset(const _PyJitAsm *a, _PyJitLabel label)
{
    assert(label >= 0 && (size_t)label < a->nlabels);
    assert(a->labels[label].offset != UNBOUND);
    return section_base(a, a->labels[label].section) + a->labels[label].offset;
}

void
_PyJitAsm_Align(_PyJitAsm *a, size_t alignment)
{
    assert(alignment != 0 && (alignment & (alignment - 1)) == 0
           && alignment <= COLD_ALIGNMENT);
    size_t size = a->sections[a->current].size;
    size_t padding = (alignment - (size & (alignment - 1))) & (alignment - 1);
    uint8_t *p = append(a, padding);
    if (p != NULL) {
        memset(p, 0xCC, padding);
    }
}

void
_PyJitAsm_QuadLabel(_PyJitAsm *a, _PyJitLabel label)
{
    add_fixup(a, _PyJitFixup_ABS64_LABEL, label, NULL);
    emit_u64(a, 0);
}

/* Instruction encoding.

   An instruction is: an optional mandatory prefix (66, F2, F3), REX, one to
   three opcode bytes (`op`, most significant byte first), ModRM, SIB and
   displacement, then an immediate that the caller appends.  `reg` is the
   ModRM reg field: a register number, or an opcode extension. */

static inline int
fits_int8(int64_t value)
{
    return value >= INT8_MIN && value <= INT8_MAX;
}

static void
emit_opcode(_PyJitAsm *a, uint32_t op)
{
    if (op > 0xFFFF) {
        emit_u8(a, op >> 16);
    }
    if (op > 0xFF) {
        emit_u8(a, op >> 8);
    }
    emit_u8(a, op);
}

static void
emit_prefixes(_PyJitAsm *a, uint8_t prefix, int rex, int force_rex)
{
    if (prefix != 0) {
        emit_u8(a, prefix);
    }
    if (rex != 0 || force_rex) {
        emit_u8(a, 0x40 | rex);
    }
}

/* rex bits: W=8, R=4, X=2, B=1 */
static inline int
rex_bits(int w, int reg, int index, int base)
{
    return (w ? 8 : 0) | ((reg >> 3) << 2) | ((index >> 3) << 1) | (base >> 3);
}

/* Register-direct operand.  `byte_rm` marks an 8-bit rm operand, for which
   spl, bpl, sil and dil need an (empty) REX prefix. */
static void
emit_rr(_PyJitAsm *a, uint8_t prefix, int w, uint32_t op, int reg, int rm,
        int byte_rm)
{
    emit_prefixes(a, prefix, rex_bits(w, reg, 0, rm), byte_rm && rm >= 4);
    emit_opcode(a, op);
    emit_u8(a, 0xC0 | (reg & 7) << 3 | (rm & 7));
}

static void
emit_rm(_PyJitAsm *a, uint8_t prefix, int w, uint32_t op, int reg,
        _PyJitMem m)
{
    assert(m.index != _PyJitReg_RSP && m.index < 16 && m.base < 16);
    int index = m.index < 0 ? 0 : m.index;
    emit_prefixes(a, prefix, rex_bits(w, reg, index, m.base), 0);
    emit_opcode(a, op);
    int base = m.base & 7;
    int mod = m.disp == 0 && base != 5 ? 0 : fits_int8(m.disp) ? 1 : 2;
    int sib = m.index >= 0 || base == 4;
    emit_u8(a, mod << 6 | (reg & 7) << 3 | (sib ? 4 : base));
    if (sib) {
        emit_u8(a, m.scale << 6 | (m.index < 0 ? 4 : m.index & 7) << 3 | base);
    }
    if (mod == 1) {
        emit_u8(a, (uint8_t)m.disp);
    }
    else if (mod == 2) {
        emit_u32(a, (uint32_t)m.disp);
    }
}

/* mov */

void
_PyJitAsm_MovRR(_PyJitAsm *a, _PyJitReg dst, _PyJitReg src)
{
    emit_rr(a, 0, 1, 0x89, src, dst, 0);
}

void
_PyJitAsm_MovImm(_PyJitAsm *a, _PyJitReg dst, int64_t imm)
{
    if (imm >= 0 && imm <= UINT32_MAX) {
        emit_prefixes(a, 0, rex_bits(0, 0, 0, dst), 0);
        emit_u8(a, 0xB8 | (dst & 7));
        emit_u32(a, (uint32_t)imm);
    }
    else if (imm >= INT32_MIN && imm <= INT32_MAX) {
        emit_rr(a, 0, 1, 0xC7, 0, dst, 0);
        emit_u32(a, (uint32_t)imm);
    }
    else {
        emit_prefixes(a, 0, rex_bits(1, 0, 0, dst), 0);
        emit_u8(a, 0xB8 | (dst & 7));
        emit_u64(a, (uint64_t)imm);
    }
}

void
_PyJitAsm_MovRM(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x8B, dst, src);
}

void
_PyJitAsm_MovMR(_PyJitAsm *a, _PyJitMem dst, _PyJitReg src)
{
    emit_rm(a, 0, 1, 0x89, src, dst);
}

void
_PyJitAsm_MovMI(_PyJitAsm *a, _PyJitMem dst, int32_t imm)
{
    emit_rm(a, 0, 1, 0xC7, 0, dst);
    emit_u32(a, (uint32_t)imm);
}

void
_PyJitAsm_MovRM32(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 0, 0x8B, dst, src);
}

void
_PyJitAsm_MovMR32(_PyJitAsm *a, _PyJitMem dst, _PyJitReg src)
{
    emit_rm(a, 0, 0, 0x89, src, dst);
}

void
_PyJitAsm_MovMI32(_PyJitAsm *a, _PyJitMem dst, int32_t imm)
{
    emit_rm(a, 0, 0, 0xC7, 0, dst);
    emit_u32(a, (uint32_t)imm);
}

// movzx to a 32-bit register zeroes the upper half, so no REX.W is needed.
void
_PyJitAsm_MovzxRM8(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 0, 0x0FB6, dst, src);
}

void
_PyJitAsm_MovzxRM16(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 0, 0x0FB7, dst, src);
}

void
_PyJitAsm_MovsxRM8(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x0FBE, dst, src);
}

void
_PyJitAsm_MovsxRM16(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x0FBF, dst, src);
}

void
_PyJitAsm_MovsxRM32(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x63, dst, src);
}

void
_PyJitAsm_Lea(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x8D, dst, src);
}

void
_PyJitAsm_LeaLabel(_PyJitAsm *a, _PyJitReg dst, _PyJitLabel label)
{
    emit_prefixes(a, 0, rex_bits(1, dst, 0, 0), 0);
    emit_u8(a, 0x8D);
    emit_u8(a, (dst & 7) << 3 | 5);    // mod 00, rm 101: [rip + disp32]
    add_fixup(a, _PyJitFixup_REL32_LABEL, label, NULL);
    emit_u32(a, 0);
}

/* add, or, and, sub, xor, cmp.  Opcode 8*op+1 is `op r/m, reg`, 8*op+3 is
   `op reg, r/m`, 8*op+5 is `op rax, imm32`; 0x83 and 0x81 take an imm8 and
   an imm32 and the extension in ModRM.reg. */

void
_PyJitAsm_AluRR(_PyJitAsm *a, _PyJitAlu op, _PyJitReg dst, _PyJitReg src)
{
    emit_rr(a, 0, 1, 8 * op + 1, src, dst, 0);
}

void
_PyJitAsm_AluRM(_PyJitAsm *a, _PyJitAlu op, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 8 * op + 3, dst, src);
}

void
_PyJitAsm_AluMR(_PyJitAsm *a, _PyJitAlu op, _PyJitMem dst, _PyJitReg src)
{
    emit_rm(a, 0, 1, 8 * op + 1, src, dst);
}

static void
emit_imm8_or_32(_PyJitAsm *a, int32_t imm)
{
    if (fits_int8(imm)) {
        emit_u8(a, (uint8_t)imm);
    }
    else {
        emit_u32(a, (uint32_t)imm);
    }
}

void
_PyJitAsm_AluRI(_PyJitAsm *a, _PyJitAlu op, _PyJitReg dst, int32_t imm)
{
    if (dst == _PyJitReg_RAX && !fits_int8(imm)) {
        emit_u8(a, 0x48);
        emit_u8(a, 8 * op + 5);
        emit_u32(a, (uint32_t)imm);
        return;
    }
    emit_rr(a, 0, 1, fits_int8(imm) ? 0x83 : 0x81, op, dst, 0);
    emit_imm8_or_32(a, imm);
}

void
_PyJitAsm_AluMI(_PyJitAsm *a, _PyJitAlu op, _PyJitMem dst, int32_t imm)
{
    emit_rm(a, 0, 1, fits_int8(imm) ? 0x83 : 0x81, op, dst);
    emit_imm8_or_32(a, imm);
}

void
_PyJitAsm_TestRR(_PyJitAsm *a, _PyJitReg x, _PyJitReg y)
{
    emit_rr(a, 0, 1, 0x85, y, x, 0);
}

void
_PyJitAsm_TestMR(_PyJitAsm *a, _PyJitMem x, _PyJitReg y)
{
    emit_rm(a, 0, 1, 0x85, y, x);
}

void
_PyJitAsm_TestRI(_PyJitAsm *a, _PyJitReg x, int32_t imm)
{
    if (x == _PyJitReg_RAX) {
        emit_u8(a, 0x48);
        emit_u8(a, 0xA9);
    }
    else {
        emit_rr(a, 0, 1, 0xF7, 0, x, 0);
    }
    emit_u32(a, (uint32_t)imm);
}

void
_PyJitAsm_TestMI(_PyJitAsm *a, _PyJitMem x, int32_t imm)
{
    emit_rm(a, 0, 1, 0xF7, 0, x);
    emit_u32(a, (uint32_t)imm);
}

void
_PyJitAsm_ImulRR(_PyJitAsm *a, _PyJitReg dst, _PyJitReg src)
{
    emit_rr(a, 0, 1, 0x0FAF, dst, src, 0);
}

void
_PyJitAsm_ImulRM(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x0FAF, dst, src);
}

void
_PyJitAsm_ImulRRI(_PyJitAsm *a, _PyJitReg dst, _PyJitReg src, int32_t imm)
{
    emit_rr(a, 0, 1, fits_int8(imm) ? 0x6B : 0x69, dst, src, 0);
    emit_imm8_or_32(a, imm);
}

/* not, neg, inc and dec: opcode and ModRM extension. */
static const struct { uint8_t op; uint8_t ext; } unary_forms[] = {
    [_PyJitUnary_NOT] = {0xF7, 2},
    [_PyJitUnary_NEG] = {0xF7, 3},
    [_PyJitUnary_INC] = {0xFF, 0},
    [_PyJitUnary_DEC] = {0xFF, 1},
};

void
_PyJitAsm_UnaryR(_PyJitAsm *a, _PyJitUnary op, _PyJitReg dst)
{
    emit_rr(a, 0, 1, unary_forms[op].op, unary_forms[op].ext, dst, 0);
}

void
_PyJitAsm_UnaryM(_PyJitAsm *a, _PyJitUnary op, _PyJitMem dst)
{
    emit_rm(a, 0, 1, unary_forms[op].op, unary_forms[op].ext, dst);
}

void
_PyJitAsm_ShiftRI(_PyJitAsm *a, _PyJitShift op, _PyJitReg dst, int imm)
{
    assert(imm >= 0 && imm < 64);
    if (imm == 1) {
        emit_rr(a, 0, 1, 0xD1, op, dst, 0);
        return;
    }
    emit_rr(a, 0, 1, 0xC1, op, dst, 0);
    emit_u8(a, (uint8_t)imm);
}

void
_PyJitAsm_ShiftRCl(_PyJitAsm *a, _PyJitShift op, _PyJitReg dst)
{
    emit_rr(a, 0, 1, 0xD3, op, dst, 0);
}

void
_PyJitAsm_Setcc(_PyJitAsm *a, _PyJitCond cond, _PyJitReg dst)
{
    emit_rr(a, 0, 0, 0x0F90 | cond, 0, dst, 1);
}

void
_PyJitAsm_CmovRR(_PyJitAsm *a, _PyJitCond cond, _PyJitReg dst, _PyJitReg src)
{
    emit_rr(a, 0, 1, 0x0F40 | cond, dst, src, 0);
}

void
_PyJitAsm_CmovRM(_PyJitAsm *a, _PyJitCond cond, _PyJitReg dst, _PyJitMem src)
{
    emit_rm(a, 0, 1, 0x0F40 | cond, dst, src);
}

/* Control flow */

void
_PyJitAsm_Jmp(_PyJitAsm *a, _PyJitLabel target)
{
    emit_u8(a, 0xE9);
    add_fixup(a, _PyJitFixup_REL32_LABEL, target, NULL);
    emit_u32(a, 0);
}

void
_PyJitAsm_Jcc(_PyJitAsm *a, _PyJitCond cond, _PyJitLabel target)
{
    emit_u8(a, 0x0F);
    emit_u8(a, 0x80 | cond);
    add_fixup(a, _PyJitFixup_REL32_LABEL, target, NULL);
    emit_u32(a, 0);
}

void
_PyJitAsm_JmpR(_PyJitAsm *a, _PyJitReg target)
{
    emit_rr(a, 0, 0, 0xFF, 4, target, 0);
}

void
_PyJitAsm_CallR(_PyJitAsm *a, _PyJitReg target)
{
    emit_rr(a, 0, 0, 0xFF, 2, target, 0);
}

void
_PyJitAsm_JmpM(_PyJitAsm *a, _PyJitMem target)
{
    emit_rm(a, 0, 0, 0xFF, 4, target);
}

void
_PyJitAsm_CallM(_PyJitAsm *a, _PyJitMem target)
{
    emit_rm(a, 0, 0, 0xFF, 2, target);
}

void
_PyJitAsm_CallAbs(_PyJitAsm *a, const void *target)
{
    emit_u8(a, 0xE8);
    add_fixup(a, _PyJitFixup_REL32_ABSOLUTE, 0, target);
    emit_u32(a, 0);
}

void
_PyJitAsm_Push(_PyJitAsm *a, _PyJitReg src)
{
    emit_prefixes(a, 0, rex_bits(0, 0, 0, src), 0);
    emit_u8(a, 0x50 | (src & 7));
}

void
_PyJitAsm_Pop(_PyJitAsm *a, _PyJitReg dst)
{
    emit_prefixes(a, 0, rex_bits(0, 0, 0, dst), 0);
    emit_u8(a, 0x58 | (dst & 7));
}

void
_PyJitAsm_Ret(_PyJitAsm *a)
{
    emit_u8(a, 0xC3);
}

/* SSE2 scalar doubles */

void
_PyJitAsm_MovsdRR(_PyJitAsm *a, _PyJitXmm dst, _PyJitXmm src)
{
    emit_rr(a, 0xF2, 0, 0x0F10, dst, src, 0);
}

void
_PyJitAsm_MovsdRM(_PyJitAsm *a, _PyJitXmm dst, _PyJitMem src)
{
    emit_rm(a, 0xF2, 0, 0x0F10, dst, src);
}

void
_PyJitAsm_MovsdMR(_PyJitAsm *a, _PyJitMem dst, _PyJitXmm src)
{
    emit_rm(a, 0xF2, 0, 0x0F11, src, dst);
}

void
_PyJitAsm_MovqXR(_PyJitAsm *a, _PyJitXmm dst, _PyJitReg src)
{
    emit_rr(a, 0x66, 1, 0x0F6E, dst, src, 0);
}

void
_PyJitAsm_MovqRX(_PyJitAsm *a, _PyJitReg dst, _PyJitXmm src)
{
    emit_rr(a, 0x66, 1, 0x0F7E, src, dst, 0);
}

static const struct { uint8_t prefix; uint16_t op; } sse_forms[] = {
    [_PyJitSse_ADDSD] = {0xF2, 0x0F58},
    [_PyJitSse_SUBSD] = {0xF2, 0x0F5C},
    [_PyJitSse_MULSD] = {0xF2, 0x0F59},
    [_PyJitSse_DIVSD] = {0xF2, 0x0F5E},
    [_PyJitSse_SQRTSD] = {0xF2, 0x0F51},
    [_PyJitSse_UCOMISD] = {0x66, 0x0F2E},
};

void
_PyJitAsm_SseRR(_PyJitAsm *a, _PyJitSse op, _PyJitXmm dst, _PyJitXmm src)
{
    emit_rr(a, sse_forms[op].prefix, 0, sse_forms[op].op, dst, src, 0);
}

void
_PyJitAsm_SseRM(_PyJitAsm *a, _PyJitSse op, _PyJitXmm dst, _PyJitMem src)
{
    emit_rm(a, sse_forms[op].prefix, 0, sse_forms[op].op, dst, src);
}

void
_PyJitAsm_Cvtsi2sd(_PyJitAsm *a, _PyJitXmm dst, _PyJitReg src)
{
    emit_rr(a, 0xF2, 1, 0x0F2A, dst, src, 0);
}

void
_PyJitAsm_Cvttsd2si(_PyJitAsm *a, _PyJitReg dst, _PyJitXmm src)
{
    emit_rr(a, 0xF2, 1, 0x0F2C, dst, src, 0);
}

/* Placement */

/* The offset in the placed code of a section's byte at `offset`. */
static size_t
final_offset(const _PyJitAsm *a, _PyJitSection section, size_t offset)
{
    return section_base(a, section) + offset;
}

int
_PyJitAsm_Resolve(const _PyJitAsm *a, uint8_t *dst, uintptr_t address)
{
    if (a->failed) {
        return -1;
    }
    for (int section = 0; section < 2; section++) {
        const _PyJitBuffer *buffer = &a->sections[section];
        if (buffer->size != 0) {
            memcpy(dst + final_offset(a, section, 0), buffer->buffer,
                   buffer->size);
        }
    }
    // The padding between the sections is never executed.
    size_t hot = a->sections[_PyJitSection_HOT].size;
    size_t cold_base = section_base(a, _PyJitSection_COLD);
    if (a->sections[_PyJitSection_COLD].size != 0) {
        memset(dst + hot, 0xCC, cold_base - hot);
    }
    for (size_t i = 0; i < a->nfixups; i++) {
        const _PyJitFixup *fixup = &a->fixups[i];
        size_t field_offset = final_offset(a, fixup->section, fixup->offset);
        uint8_t *field = dst + field_offset;
        if (fixup->kind == _PyJitFixup_REL32_ABSOLUTE) {
            int64_t rel = (int64_t)((uintptr_t)fixup->target
                                    - (address + field_offset + 4));
            if (rel != (int32_t)rel) {
                return -1;
            }
            int32_t rel32 = (int32_t)rel;
            memcpy(field, &rel32, 4);
            continue;
        }
        assert(fixup->label >= 0 && (size_t)fixup->label < a->nlabels);
        const _PyJitLabelInfo *label = &a->labels[fixup->label];
        if (label->offset == UNBOUND) {
            return -1;
        }
        size_t target = final_offset(a, label->section, label->offset);
        if (fixup->kind == _PyJitFixup_ABS64_LABEL) {
            uint64_t absolute = address + target;
            memcpy(field, &absolute, 8);
        }
        else {
            assert(fixup->kind == _PyJitFixup_REL32_LABEL);
            int32_t rel32 = (int32_t)((int64_t)target
                                      - (int64_t)(field_offset + 4));
            memcpy(field, &rel32, 4);
        }
    }
    return 0;
}

int
_PyJitAsm_Place(const _PyJitAsm *a, _PyJitCode *code)
{
    if (a->failed || _PyJitAsm_Size(a) == 0
        || _PyJitMemory_Alloc(_PyJitAsm_Size(a), code) < 0)
    {
        return -1;
    }
    if (_PyJitAsm_Resolve(a, code->start, (uintptr_t)code->start) < 0
        || _PyJitMemory_Seal(code) < 0)
    {
        _PyJitMemory_Free(code);
        return -1;
    }
    return 0;
}

#endif  // _PY_JIT_SUPPORTED
