#ifndef Py_INTERNAL_JIT_ASM_H
#define Py_INTERNAL_JIT_ASM_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_jit_memory.h"

#ifdef _PY_JIT_SUPPORTED

/* An x86-64 encoder over a growable buffer, with labels and relocations.

   Operands are 64 bits wide unless the function's name says otherwise.  A
   memory operand is [base + index * 2^scale + disp32].  Immediates are
   sign-extended 32-bit values unless the function takes an int64_t.  The
   encodings are the ones GNU as chooses for the same instruction, so that
   the tests can compare bytes.

   Emitting never fails visibly: a failed allocation is remembered and
   reported by _PyJitAsm_Place().  Jumps are always rel32.

   Code is built in three steps: emit into the buffer (offsets are positions
   in it), then _PyJitAsm_Place() allocates code memory, writes the bytes
   with labels and call targets resolved against the final address and makes
   the pages read-execute.  After a successful Place, the address of a label
   is code.start + _PyJitAsm_LabelOffset(). */

typedef enum {
    _PyJitReg_RAX, _PyJitReg_RCX, _PyJitReg_RDX, _PyJitReg_RBX,
    _PyJitReg_RSP, _PyJitReg_RBP, _PyJitReg_RSI, _PyJitReg_RDI,
    _PyJitReg_R8, _PyJitReg_R9, _PyJitReg_R10, _PyJitReg_R11,
    _PyJitReg_R12, _PyJitReg_R13, _PyJitReg_R14, _PyJitReg_R15,
} _PyJitReg;

typedef enum {
    _PyJitXmm_0, _PyJitXmm_1, _PyJitXmm_2, _PyJitXmm_3,
    _PyJitXmm_4, _PyJitXmm_5, _PyJitXmm_6, _PyJitXmm_7,
    _PyJitXmm_8, _PyJitXmm_9, _PyJitXmm_10, _PyJitXmm_11,
    _PyJitXmm_12, _PyJitXmm_13, _PyJitXmm_14, _PyJitXmm_15,
} _PyJitXmm;

/* The condition code of jcc, setcc and cmovcc, as the hardware numbers it. */
typedef enum {
    _PyJitCond_O, _PyJitCond_NO, _PyJitCond_B, _PyJitCond_AE,
    _PyJitCond_E, _PyJitCond_NE, _PyJitCond_BE, _PyJitCond_A,
    _PyJitCond_S, _PyJitCond_NS, _PyJitCond_P, _PyJitCond_NP,
    _PyJitCond_L, _PyJitCond_GE, _PyJitCond_LE, _PyJitCond_G,
} _PyJitCond;

/* The two-operand arithmetic group; the value is the opcode extension. */
typedef enum {
    _PyJitAlu_ADD = 0, _PyJitAlu_OR = 1, _PyJitAlu_AND = 4,
    _PyJitAlu_SUB = 5, _PyJitAlu_XOR = 6, _PyJitAlu_CMP = 7,
} _PyJitAlu;

typedef enum {
    _PyJitUnary_NOT, _PyJitUnary_NEG, _PyJitUnary_INC, _PyJitUnary_DEC,
} _PyJitUnary;

typedef enum {
    _PyJitShift_SHL = 4, _PyJitShift_SHR = 5, _PyJitShift_SAR = 7,
} _PyJitShift;

/* Scalar double operations of the form `op xmm, xmm/mem`. */
typedef enum {
    _PyJitSse_ADDSD, _PyJitSse_SUBSD, _PyJitSse_MULSD, _PyJitSse_DIVSD,
    _PyJitSse_SQRTSD, _PyJitSse_UCOMISD,
} _PyJitSse;

typedef struct {
    int8_t base;
    int8_t index;                 // -1: none
    uint8_t scale;                // log2 of the index multiplier, 0 to 3
    int32_t disp;
} _PyJitMem;

static inline _PyJitMem
_PyJitMem_At(_PyJitReg base, int32_t disp)
{
    return (_PyJitMem){base, -1, 0, disp};
}

static inline _PyJitMem
_PyJitMem_Indexed(_PyJitReg base, _PyJitReg index, int scale, int32_t disp)
{
    return (_PyJitMem){base, index, scale, disp};
}

typedef int32_t _PyJitLabel;

/* Emission goes to one of two sections.  Placement lays out all hot bytes,
   then all cold bytes (starting at a 64-byte boundary), so that rarely run
   code does not interleave with the code that runs.  Labels and fixups
   record the section they were made in; jumps, calls and label addresses
   work across the two. */
typedef enum {
    _PyJitSection_HOT,
    _PyJitSection_COLD,
} _PyJitSection;

typedef enum {
    _PyJitFixup_REL32_LABEL,      // jmp, jcc: label - end of field
    _PyJitFixup_REL32_ABSOLUTE,   // call of a C address
    _PyJitFixup_ABS64_LABEL,      // the absolute address of a label
} _PyJitFixupKind;

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t capacity;
} _PyJitBuffer;

typedef struct {
    _PyJitFixupKind kind;
    _PyJitSection section;        // of the field
    size_t offset;                // of the field in its section
    union {
        _PyJitLabel label;
        const void *target;
    };
} _PyJitFixup;

typedef struct {
    size_t offset;                // in the section; SIZE_MAX until bound
    _PyJitSection section;
} _PyJitLabelInfo;

typedef struct {
    _PyJitBuffer sections[2];
    _PyJitSection current;
    _PyJitLabelInfo *labels;
    size_t nlabels;
    size_t labels_capacity;
    _PyJitFixup *fixups;
    size_t nfixups;
    size_t fixups_capacity;
    int failed;                   // an allocation failed
} _PyJitAsm;

PyAPI_FUNC(void) _PyJitAsm_Init(_PyJitAsm *a);
PyAPI_FUNC(void) _PyJitAsm_Fini(_PyJitAsm *a);

/* Direct the following emission, and the labels bound, to a section. */
PyAPI_FUNC(void) _PyJitAsm_Section(_PyJitAsm *a, _PyJitSection section);

/* The size of the placed code: the hot bytes, padding, then the cold bytes. */
PyAPI_FUNC(size_t) _PyJitAsm_Size(const _PyJitAsm *a);

PyAPI_FUNC(_PyJitLabel) _PyJitAsm_NewLabel(_PyJitAsm *a);
PyAPI_FUNC(void) _PyJitAsm_Bind(_PyJitAsm *a, _PyJitLabel label);
/* The offset of a bound label from the start of the code.  A cold label's
   offset follows the hot size, so it is final once emission is complete. */
PyAPI_FUNC(size_t) _PyJitAsm_LabelOffset(const _PyJitAsm *a, _PyJitLabel label);

/* Pad with int3 to a multiple of `alignment`, a power of two. */
PyAPI_FUNC(void) _PyJitAsm_Align(_PyJitAsm *a, size_t alignment);
/* Eight bytes holding the absolute address of the label, for entry tables.
   Aligns nothing: call _PyJitAsm_Align(a, 8) first if the table needs it. */
PyAPI_FUNC(void) _PyJitAsm_QuadLabel(_PyJitAsm *a, _PyJitLabel label);

/* Raw bytes, for data that lies with the code: the unit's unwind info. */
PyAPI_FUNC(void) _PyJitAsm_Data(_PyJitAsm *a, const void *data, size_t size);

/* mov.  MovImm picks the shortest of mov r32, imm32 (zero-extending),
   mov r64, simm32 and movabs. */
PyAPI_FUNC(void) _PyJitAsm_MovRR(_PyJitAsm *a, _PyJitReg dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_MovImm(_PyJitAsm *a, _PyJitReg dst, int64_t imm);
PyAPI_FUNC(void) _PyJitAsm_MovRM(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_MovMR(_PyJitAsm *a, _PyJitMem dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_MovMI(_PyJitAsm *a, _PyJitMem dst, int32_t imm);
/* 32-bit mov: a load zero-extends to 64 bits. */
PyAPI_FUNC(void) _PyJitAsm_MovRM32(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_MovMR32(_PyJitAsm *a, _PyJitMem dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_MovMI32(_PyJitAsm *a, _PyJitMem dst, int32_t imm);
/* Stores of the low 8 or 16 bits of a register, or of an immediate (-128 to
   255, -32768 to 65535).  The 8-bit register forms never mean ah..bh. */
PyAPI_FUNC(void) _PyJitAsm_MovMR8(_PyJitAsm *a, _PyJitMem dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_MovMI8(_PyJitAsm *a, _PyJitMem dst, int imm);
PyAPI_FUNC(void) _PyJitAsm_MovMR16(_PyJitAsm *a, _PyJitMem dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_MovMI16(_PyJitAsm *a, _PyJitMem dst, int imm);
/* Zero-extending loads of 8 and 16 bits. */
PyAPI_FUNC(void) _PyJitAsm_MovzxRM8(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_MovzxRM16(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
/* Sign-extending loads of 8, 16 and 32 bits to 64. */
PyAPI_FUNC(void) _PyJitAsm_MovsxRM8(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_MovsxRM16(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_MovsxRM32(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_Lea(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
/* lea dst, [rip + label]: the address of a label, position independent. */
PyAPI_FUNC(void) _PyJitAsm_LeaLabel(_PyJitAsm *a, _PyJitReg dst, _PyJitLabel label);

/* add, or, and, sub, xor and cmp.  The R, M and I suffixes are the
   destination and source: register, memory, immediate. */
PyAPI_FUNC(void) _PyJitAsm_AluRR(_PyJitAsm *a, _PyJitAlu op, _PyJitReg dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_AluRM(_PyJitAsm *a, _PyJitAlu op, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_AluMR(_PyJitAsm *a, _PyJitAlu op, _PyJitMem dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_AluRI(_PyJitAsm *a, _PyJitAlu op, _PyJitReg dst, int32_t imm);
PyAPI_FUNC(void) _PyJitAsm_AluMI(_PyJitAsm *a, _PyJitAlu op, _PyJitMem dst, int32_t imm);

/* 32-bit read-modify-write on memory, for reference counts: `op dword [mem],
   imm` and `inc`/`dec`/`not`/`neg dword [mem]`. */
PyAPI_FUNC(void) _PyJitAsm_AluMI32(_PyJitAsm *a, _PyJitAlu op, _PyJitMem dst, int32_t imm);
PyAPI_FUNC(void) _PyJitAsm_UnaryM32(_PyJitAsm *a, _PyJitUnary op, _PyJitMem dst);

PyAPI_FUNC(void) _PyJitAsm_TestRR(_PyJitAsm *a, _PyJitReg x, _PyJitReg y);
PyAPI_FUNC(void) _PyJitAsm_TestMR(_PyJitAsm *a, _PyJitMem x, _PyJitReg y);
PyAPI_FUNC(void) _PyJitAsm_TestRI(_PyJitAsm *a, _PyJitReg x, int32_t imm);
PyAPI_FUNC(void) _PyJitAsm_TestMI(_PyJitAsm *a, _PyJitMem x, int32_t imm);

PyAPI_FUNC(void) _PyJitAsm_ImulRR(_PyJitAsm *a, _PyJitReg dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_ImulRM(_PyJitAsm *a, _PyJitReg dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_ImulRRI(_PyJitAsm *a, _PyJitReg dst, _PyJitReg src, int32_t imm);

PyAPI_FUNC(void) _PyJitAsm_UnaryR(_PyJitAsm *a, _PyJitUnary op, _PyJitReg dst);
PyAPI_FUNC(void) _PyJitAsm_UnaryM(_PyJitAsm *a, _PyJitUnary op, _PyJitMem dst);

/* Shift by an immediate in 0..63, or by cl. */
PyAPI_FUNC(void) _PyJitAsm_ShiftRI(_PyJitAsm *a, _PyJitShift op, _PyJitReg dst, int imm);
PyAPI_FUNC(void) _PyJitAsm_ShiftRCl(_PyJitAsm *a, _PyJitShift op, _PyJitReg dst);

/* setcc writes the low byte of dst and leaves the rest. */
PyAPI_FUNC(void) _PyJitAsm_Setcc(_PyJitAsm *a, _PyJitCond cond, _PyJitReg dst);
PyAPI_FUNC(void) _PyJitAsm_CmovRR(_PyJitAsm *a, _PyJitCond cond, _PyJitReg dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_CmovRM(_PyJitAsm *a, _PyJitCond cond, _PyJitReg dst, _PyJitMem src);

PyAPI_FUNC(void) _PyJitAsm_Jmp(_PyJitAsm *a, _PyJitLabel target);
PyAPI_FUNC(void) _PyJitAsm_Jcc(_PyJitAsm *a, _PyJitCond cond, _PyJitLabel target);
PyAPI_FUNC(void) _PyJitAsm_JmpR(_PyJitAsm *a, _PyJitReg target);
PyAPI_FUNC(void) _PyJitAsm_CallR(_PyJitAsm *a, _PyJitReg target);
PyAPI_FUNC(void) _PyJitAsm_JmpM(_PyJitAsm *a, _PyJitMem target);
PyAPI_FUNC(void) _PyJitAsm_CallM(_PyJitAsm *a, _PyJitMem target);
/* call rel32 of a C function.  _PyJitAsm_Place() fails if the function is
   beyond rel32's reach from the final address. */
PyAPI_FUNC(void) _PyJitAsm_CallAbs(_PyJitAsm *a, const void *target);
PyAPI_FUNC(void) _PyJitAsm_Push(_PyJitAsm *a, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_Pop(_PyJitAsm *a, _PyJitReg dst);
PyAPI_FUNC(void) _PyJitAsm_Ret(_PyJitAsm *a);

/* Scalar doubles.  movq moves the 64 bits between the register files. */
PyAPI_FUNC(void) _PyJitAsm_MovsdRR(_PyJitAsm *a, _PyJitXmm dst, _PyJitXmm src);
PyAPI_FUNC(void) _PyJitAsm_MovsdRM(_PyJitAsm *a, _PyJitXmm dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_MovsdMR(_PyJitAsm *a, _PyJitMem dst, _PyJitXmm src);
PyAPI_FUNC(void) _PyJitAsm_MovqXR(_PyJitAsm *a, _PyJitXmm dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_MovqRX(_PyJitAsm *a, _PyJitReg dst, _PyJitXmm src);
PyAPI_FUNC(void) _PyJitAsm_SseRR(_PyJitAsm *a, _PyJitSse op, _PyJitXmm dst, _PyJitXmm src);
PyAPI_FUNC(void) _PyJitAsm_SseRM(_PyJitAsm *a, _PyJitSse op, _PyJitXmm dst, _PyJitMem src);
PyAPI_FUNC(void) _PyJitAsm_Cvtsi2sd(_PyJitAsm *a, _PyJitXmm dst, _PyJitReg src);
PyAPI_FUNC(void) _PyJitAsm_Cvttsd2si(_PyJitAsm *a, _PyJitReg dst, _PyJitXmm src);

/* Write the code to `dst` (a buffer of _PyJitAsm_Size() bytes) as if it
   lived at `address`, resolving labels and call targets.  Returns -1 for an
   unbound label, a failed allocation or a call out of reach. */
PyAPI_FUNC(int) _PyJitAsm_Resolve(const _PyJitAsm *a, uint8_t *dst, uintptr_t address);

/* Allocate code memory, write the resolved code and make it executable.
   Returns -1, with no exception set, if any of that fails; the assembler is
   left as it was, so a failed Place can be abandoned with _PyJitAsm_Fini(). */
PyAPI_FUNC(int) _PyJitAsm_Place(const _PyJitAsm *a, _PyJitCode *code);

#endif  // _PY_JIT_SUPPORTED

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_ASM_H */
