/* A small x86-64 assembler for tier 0 (Python/tier0.c).
 *
 * It encodes the few instructions that tier 0 emits into a growable buffer:
 * moves, lea, integer arithmetic, shifts and compares, scalar double
 * arithmetic, conditional moves, conditional and indirect jumps, calls and
 * stack operations, with 8- and 32-bit displacements, 32-bit immediates and
 * RIP-relative operands.  Branch targets and
 * RIP-relative operands name labels, which may be bound after use; the
 * displacements are filled in by jit_resolve() once the code is complete.
 * Nothing here knows about Python.
 */

#ifndef Py_JIT_X86_H
#define Py_JIT_X86_H

#include <stdint.h>
#include <string.h>

typedef enum {
    X86_RAX, X86_RCX, X86_RDX, X86_RBX, X86_RSP, X86_RBP, X86_RSI, X86_RDI,
    X86_R8, X86_R9, X86_R10, X86_R11, X86_R12, X86_R13, X86_R14, X86_R15,
} jit_reg;

/* Condition codes, the low nibble of Jcc and SETcc */
typedef enum {
    X86_O = 0x0, X86_NO = 0x1, X86_B = 0x2, X86_AE = 0x3,
    X86_E = 0x4, X86_NE = 0x5, X86_BE = 0x6, X86_A = 0x7,
    X86_S = 0x8, X86_NS = 0x9, X86_P = 0xA, X86_NP = 0xB,
    X86_L = 0xC, X86_GE = 0xD, X86_LE = 0xE, X86_G = 0xF,
} jit_cond;

typedef struct {
    int at;         // offset of the 32-bit displacement
    int label;
} jit_fixup;

typedef struct {
    uint8_t *code;
    int size;
    int capacity;
    int *labels;    // offset of each label, or -1 while unbound
    int nlabels;
    int labels_capacity;
    jit_fixup *fixups;
    int nfixups;
    int fixups_capacity;
    int failed;     // out of memory; the buffer is unusable
} jit_buf;

static inline int
jit_grow(void **array, int *capacity, int need, size_t item)
{
    if (need <= *capacity) {
        return 0;
    }
    int capacity2 = *capacity ? *capacity : 64;
    while (capacity2 < need) {
        capacity2 *= 2;
    }
    void *array2 = PyMem_Realloc(*array, (size_t)capacity2 * item);
    if (array2 == NULL) {
        return -1;
    }
    *array = array2;
    *capacity = capacity2;
    return 0;
}

static inline void
jit_free(jit_buf *b)
{
    PyMem_Free(b->code);
    PyMem_Free(b->labels);
    PyMem_Free(b->fixups);
}

static inline void
jit_byte(jit_buf *b, int byte)
{
    if (jit_grow((void **)&b->code, &b->capacity, b->size + 1, 1) < 0) {
        b->failed = 1;
        return;
    }
    b->code[b->size++] = (uint8_t)byte;
}

static inline void
jit_u32(jit_buf *b, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        jit_byte(b, (v >> (8 * i)) & 0xFF);
    }
}

static inline void
jit_u64(jit_buf *b, uint64_t v)
{
    jit_u32(b, (uint32_t)v);
    jit_u32(b, (uint32_t)(v >> 32));
}

static inline void
jit_align(jit_buf *b, int alignment)
{
    while (b->size % alignment) {
        jit_byte(b, 0xCC);
    }
}

/* Labels */

static inline int
jit_new_label(jit_buf *b)
{
    if (jit_grow((void **)&b->labels, &b->labels_capacity, b->nlabels + 1,
                 sizeof(int)) < 0)
    {
        b->failed = 1;
        return 0;
    }
    b->labels[b->nlabels] = -1;
    return b->nlabels++;
}

static inline void
jit_bind(jit_buf *b, int label)
{
    if (!b->failed) {
        b->labels[label] = b->size;
    }
}

/* A 32-bit displacement to label, relative to the end of the four bytes,
 * which must end the instruction. */
static inline void
jit_rel32(jit_buf *b, int label)
{
    if (jit_grow((void **)&b->fixups, &b->fixups_capacity, b->nfixups + 1,
                 sizeof(jit_fixup)) < 0)
    {
        b->failed = 1;
        return;
    }
    b->fixups[b->nfixups++] = (jit_fixup){b->size, label};
    jit_u32(b, 0);
}

/* Fill in every displacement; 0 on success. */
static inline int
jit_resolve(jit_buf *b)
{
    if (b->failed) {
        return -1;
    }
    for (int i = 0; i < b->nfixups; i++) {
        jit_fixup f = b->fixups[i];
        int target = b->labels[f.label];
        if (target < 0) {
            return -1;
        }
        int32_t disp = target - (f.at + 4);
        memcpy(b->code + f.at, &disp, sizeof(disp));
    }
    return 0;
}

/* Encoding */

static inline void
jit_rex(jit_buf *b, int w, int reg, int index, int base)
{
    int rex = 0x40 | (w << 3) | ((reg & 8) >> 1) | ((index & 8) >> 2)
              | ((base & 8) >> 3);
    if (rex != 0x40) {
        jit_byte(b, rex);
    }
}

/* ModRM (and SIB) for [base + disp] */
static inline void
jit_mem(jit_buf *b, int reg, jit_reg base, int32_t disp)
{
    int mod = (disp == 0 && (base & 7) != X86_RBP) ? 0
              : (disp >= -128 && disp <= 127) ? 1 : 2;
    jit_byte(b, (mod << 6) | ((reg & 7) << 3) | (base & 7));
    if ((base & 7) == X86_RSP) {
        jit_byte(b, 0x24);
    }
    if (mod == 1) {
        jit_byte(b, disp & 0xFF);
    }
    else if (mod == 2) {
        jit_u32(b, (uint32_t)disp);
    }
}

/* op reg, [base + disp] (or the reverse, as the opcode says) */
static inline void
jit_op_mem(jit_buf *b, int w, int opcode, int reg, jit_reg base, int32_t disp)
{
    jit_rex(b, w, reg, 0, base);
    if (opcode > 0xFF) {
        jit_byte(b, opcode >> 8);
    }
    jit_byte(b, opcode & 0xFF);
    jit_mem(b, reg, base, disp);
}

/* op rm, reg in register form */
static inline void
jit_op_reg(jit_buf *b, int w, int opcode, int reg, int rm)
{
    jit_rex(b, w, reg, 0, rm);
    if (opcode > 0xFF) {
        jit_byte(b, opcode >> 8);
    }
    jit_byte(b, opcode & 0xFF);
    jit_byte(b, 0xC0 | ((reg & 7) << 3) | (rm & 7));
}

/* op reg, [rip + label] */
static inline void
jit_op_rip(jit_buf *b, int w, int opcode, int reg, int label)
{
    jit_rex(b, w, reg, 0, 0);
    if (opcode > 0xFF) {
        jit_byte(b, opcode >> 8);
    }
    jit_byte(b, opcode & 0xFF);
    jit_byte(b, ((reg & 7) << 3) | 5);
    jit_rel32(b, label);
}

/* Instructions; 64-bit operands unless the name ends in 32 */

static inline void jit_load(jit_buf *b, jit_reg dst, jit_reg base, int32_t disp) { jit_op_mem(b, 1, 0x8B, dst, base, disp); }
static inline void jit_store(jit_buf *b, jit_reg base, int32_t disp, jit_reg src) { jit_op_mem(b, 1, 0x89, src, base, disp); }
static inline void jit_load32(jit_buf *b, jit_reg dst, jit_reg base, int32_t disp) { jit_op_mem(b, 0, 0x8B, dst, base, disp); }
static inline void jit_load16(jit_buf *b, jit_reg dst, jit_reg base, int32_t disp) { jit_op_mem(b, 0, 0x0FB7, dst, base, disp); }
static inline void jit_lea(jit_buf *b, jit_reg dst, jit_reg base, int32_t disp) { jit_op_mem(b, 1, 0x8D, dst, base, disp); }
static inline void jit_cmp_mem(jit_buf *b, jit_reg reg, jit_reg base, int32_t disp) { jit_op_mem(b, 1, 0x3B, reg, base, disp); }
static inline void jit_add_mem(jit_buf *b, jit_reg dst, jit_reg base, int32_t disp) { jit_op_mem(b, 1, 0x03, dst, base, disp); }
static inline void jit_sub_mem(jit_buf *b, jit_reg dst, jit_reg base, int32_t disp) { jit_op_mem(b, 1, 0x2B, dst, base, disp); }
static inline void jit_mov(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 1, 0x89, src, dst); }
static inline void jit_mov32(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 0, 0x89, src, dst); }
static inline void jit_movsxd(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 1, 0x63, dst, src); }
static inline void jit_add(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 1, 0x01, src, dst); }
static inline void jit_add32(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 0, 0x01, src, dst); }
static inline void jit_sub(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 1, 0x29, src, dst); }
static inline void jit_sub32(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 0, 0x29, src, dst); }
static inline void jit_and(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 1, 0x21, src, dst); }
static inline void jit_cmp(jit_buf *b, jit_reg a, jit_reg c) { jit_op_reg(b, 1, 0x39, c, a); }
static inline void jit_cmp32(jit_buf *b, jit_reg a, jit_reg c) { jit_op_reg(b, 0, 0x39, c, a); }
static inline void jit_test(jit_buf *b, jit_reg a, jit_reg c) { jit_op_reg(b, 1, 0x85, c, a); }
static inline void jit_load_rip(jit_buf *b, jit_reg dst, int label) { jit_op_rip(b, 1, 0x8B, dst, label); }
static inline void jit_cmp_rip(jit_buf *b, jit_reg reg, int label) { jit_op_rip(b, 1, 0x3B, reg, label); }
static inline void jit_lea_rip(jit_buf *b, jit_reg dst, int label) { jit_op_rip(b, 1, 0x8D, dst, label); }
static inline void jit_or_rip(jit_buf *b, jit_reg dst, int label) { jit_op_rip(b, 1, 0x0B, dst, label); }
static inline void jit_test_rip(jit_buf *b, jit_reg reg, int label) { jit_op_rip(b, 1, 0x85, reg, label); }
static inline void jit_cmov_rip(jit_buf *b, jit_cond cond, jit_reg dst, int label) { jit_op_rip(b, 1, 0x0F40 | cond, dst, label); }
static inline void jit_imul32(jit_buf *b, jit_reg dst, jit_reg src) { jit_op_reg(b, 0, 0x0FAF, dst, src); }

/* mov word [base + disp], src */
static inline void
jit_store16(jit_buf *b, jit_reg base, int32_t disp, jit_reg src)
{
    jit_byte(b, 0x66);
    jit_op_mem(b, 0, 0x89, src, base, disp);
}

/* Group 1 arithmetic on [base + disp], a qword if w else a dword, with an
 * 8-bit immediate: /0 add, /5 sub, /7 cmp */
static inline void
jit_arith_mem_imm8(jit_buf *b, int w, int ext, jit_reg base, int32_t disp, int8_t imm)
{
    jit_op_mem(b, w, 0x83, ext, base, disp);
    jit_byte(b, imm & 0xFF);
}

/* SSE2 on scalar doubles: PREFIX [REX] 0F OPCODE /r */
static inline void
jit_sse(jit_buf *b, int prefix, int w, int opcode, int reg, int rm)
{
    jit_byte(b, prefix);
    jit_op_reg(b, w, 0x0F00 | opcode, reg, rm);
}

static inline void jit_movq_to_xmm(jit_buf *b, int xmm, jit_reg src) { jit_sse(b, 0x66, 1, 0x6E, xmm, src); }
static inline void jit_movq_from_xmm(jit_buf *b, jit_reg dst, int xmm) { jit_sse(b, 0x66, 1, 0x7E, xmm, dst); }
static inline void jit_addsd(jit_buf *b, int dst, int src) { jit_sse(b, 0xF2, 0, 0x58, dst, src); }
static inline void jit_mulsd(jit_buf *b, int dst, int src) { jit_sse(b, 0xF2, 0, 0x59, dst, src); }
static inline void jit_subsd(jit_buf *b, int dst, int src) { jit_sse(b, 0xF2, 0, 0x5C, dst, src); }
static inline void jit_ucomisd(jit_buf *b, int a, int c) { jit_sse(b, 0x66, 0, 0x2E, a, c); }

static inline void
jit_mov_imm32(jit_buf *b, jit_reg dst, uint32_t imm)
{
    jit_rex(b, 0, 0, 0, dst);
    jit_byte(b, 0xB8 + (dst & 7));
    jit_u32(b, imm);
}

static inline void
jit_mov_imm64(jit_buf *b, jit_reg dst, uint64_t imm)
{
    if (imm <= UINT32_MAX) {
        jit_mov_imm32(b, dst, (uint32_t)imm);
        return;
    }
    jit_rex(b, 1, 0, 0, dst);
    jit_byte(b, 0xB8 + (dst & 7));
    jit_u64(b, imm);
}

/* Group 1 arithmetic with an immediate: /0 add, /4 and, /5 sub, /7 cmp */
static inline void
jit_arith_imm(jit_buf *b, int ext, jit_reg dst, int32_t imm)
{
    jit_rex(b, 1, 0, 0, dst);
    if (imm >= -128 && imm <= 127) {
        jit_byte(b, 0x83);
        jit_byte(b, 0xC0 | (ext << 3) | (dst & 7));
        jit_byte(b, imm & 0xFF);
    }
    else {
        jit_byte(b, 0x81);
        jit_byte(b, 0xC0 | (ext << 3) | (dst & 7));
        jit_u32(b, (uint32_t)imm);
    }
}

static inline void jit_or_imm(jit_buf *b, jit_reg dst, int32_t imm) { jit_arith_imm(b, 1, dst, imm); }
static inline void jit_and_imm(jit_buf *b, jit_reg dst, int32_t imm) { jit_arith_imm(b, 4, dst, imm); }
static inline void jit_sub_imm(jit_buf *b, jit_reg dst, int32_t imm) { jit_arith_imm(b, 5, dst, imm); }
static inline void jit_cmp_imm(jit_buf *b, jit_reg dst, int32_t imm) { jit_arith_imm(b, 7, dst, imm); }

/* cmp word [base + disp], imm16 */
static inline void
jit_cmp16_mem_imm(jit_buf *b, jit_reg base, int32_t disp, uint16_t imm)
{
    jit_byte(b, 0x66);
    jit_op_mem(b, 0, 0x81, 7, base, disp);
    jit_byte(b, imm & 0xFF);
    jit_byte(b, imm >> 8);
}

/* test dword [base + disp], imm32 */
static inline void
jit_test_mem_imm32(jit_buf *b, jit_reg base, int32_t disp, uint32_t imm)
{
    jit_op_mem(b, 0, 0xF7, 0, base, disp);
    jit_u32(b, imm);
}

/* cmp byte [base + disp], imm8 */
static inline void
jit_cmp8_mem_imm(jit_buf *b, jit_reg base, int32_t disp, uint8_t imm)
{
    jit_op_mem(b, 0, 0x80, 7, base, disp);
    jit_byte(b, imm);
}

/* test reg, imm32 (sign-extended) */
static inline void
jit_test_imm(jit_buf *b, jit_reg reg, int32_t imm)
{
    jit_op_reg(b, 1, 0xF7, 0, reg);
    jit_u32(b, (uint32_t)imm);
}

/* Shifts by an immediate: /4 shl, /5 shr */
static inline void
jit_shift_imm(jit_buf *b, int ext, jit_reg dst, int imm)
{
    jit_rex(b, 1, 0, 0, dst);
    jit_byte(b, 0xC1);
    jit_byte(b, 0xC0 | (ext << 3) | (dst & 7));
    jit_byte(b, imm);
}

static inline void jit_shl_imm(jit_buf *b, jit_reg dst, int imm) { jit_shift_imm(b, 4, dst, imm); }
static inline void jit_shr_imm(jit_buf *b, jit_reg dst, int imm) { jit_shift_imm(b, 5, dst, imm); }

/* movsxd dst, dword [base + index * 1] */
static inline void
jit_movsxd_index(jit_buf *b, jit_reg dst, jit_reg base, jit_reg index)
{
    assert((base & 7) != X86_RBP);
    jit_rex(b, 1, dst, index, base);
    jit_byte(b, 0x63);
    jit_byte(b, ((dst & 7) << 3) | 4);
    jit_byte(b, ((index & 7) << 3) | (base & 7));
}

/* call qword [base + index * 8] */
static inline void
jit_call_table(jit_buf *b, jit_reg base, jit_reg index)
{
    assert((base & 7) != X86_RBP);
    jit_rex(b, 0, 0, index, base);
    jit_byte(b, 0xFF);
    jit_byte(b, (2 << 3) | 4);
    jit_byte(b, (3 << 6) | ((index & 7) << 3) | (base & 7));
}

static inline void
jit_jmp_reg(jit_buf *b, jit_reg target)
{
    jit_rex(b, 0, 0, 0, target);
    jit_byte(b, 0xFF);
    jit_byte(b, 0xC0 | (4 << 3) | (target & 7));
}

/* jmp qword [rip + label] */
static inline void
jit_jmp_rip(jit_buf *b, int label)
{
    jit_byte(b, 0xFF);
    jit_byte(b, (4 << 3) | 5);
    jit_rel32(b, label);
}

/* call qword [rip + label] */
static inline void
jit_call_rip(jit_buf *b, int label)
{
    jit_byte(b, 0xFF);
    jit_byte(b, (2 << 3) | 5);
    jit_rel32(b, label);
}

static inline void
jit_jmp(jit_buf *b, int label)
{
    jit_byte(b, 0xE9);
    jit_rel32(b, label);
}

static inline void
jit_jcc(jit_buf *b, jit_cond cond, int label)
{
    jit_byte(b, 0x0F);
    jit_byte(b, 0x80 | cond);
    jit_rel32(b, label);
}

static inline void
jit_push(jit_buf *b, jit_reg reg)
{
    jit_rex(b, 0, 0, 0, reg);
    jit_byte(b, 0x50 + (reg & 7));
}

static inline void jit_leave(jit_buf *b) { jit_byte(b, 0xC9); }

/* A NOP of n bytes, 1 to 8 */
static inline void
jit_nop(jit_buf *b, int n)
{
    static const uint8_t nops[8][8] = {
        {0x90},
        {0x66, 0x90},
        {0x0F, 0x1F, 0x00},
        {0x0F, 0x1F, 0x40, 0x00},
        {0x0F, 0x1F, 0x44, 0x00, 0x00},
        {0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00},
        {0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00},
        {0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00},
    };
    assert(n >= 1 && n <= 8);
    for (int i = 0; i < n; i++) {
        jit_byte(b, nops[n - 1][i]);
    }
}

#endif /* !Py_JIT_X86_H */
