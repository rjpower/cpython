#ifndef Py_INTERNAL_JIT_STENCILS_H
#define Py_INTERNAL_JIT_STENCILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* The types of the baseline JIT's table of stencils,
   Python/generated_jit_stencils.h, which Tools/jit/stencils.py writes in
   the build tree from Python/jit_stencils.o: the interpreter's arms,
   compiled once more with the word's fields and the arm's exits as holes.

   A stencil has up to three parts: the hot code, the cold code that GCC
   split off (the paths to its declared cold exits), and the data of its
   jump tables.  The emitter copies each part and patches each hole of it;
   the support image (the local functions and constants that the arms
   reach) is copied once, into the JIT's code region, so that every part
   reaches it by rel32. */

#include <stdint.h>

/* What a hole is bound to */
typedef enum {
    _PyJit_HOLE_FIELD_A,        // field A of the word, and so on
    _PyJit_HOLE_FIELD_B,
    _PyJit_HOLE_FIELD_C,
    _PyJit_HOLE_FIELD_D,
    _PyJit_HOLE_CONTINUE,       // the next word's code
    _PyJit_HOLE_TARGET,         // the code of the word's branch target
    _PyJit_HOLE_SWITCH,         // next_instr of frame, after a frame transfer
    _PyJit_HOLE_REDIRECT,       // next_instr of frame, after a redirect
    _PyJit_HOLE_AGAIN,          // the word, under the arm it now names
    _PyJit_HOLE_MISS,           // the family's generic arm (generic), at the word
    _PyJit_HOLE_PROCESS_ERROR,  // the interpreter's routine process_error
    _PyJit_HOLE_CLOSE_DISPATCH, // ... close_dispatch
    _PyJit_HOLE_GEN_RETURN,     // ... gen_return
    _PyJit_HOLE_EXTERN,         // _PyJit_StencilSymbols[symbol]
    _PyJit_HOLE_TPOFF,          // the offset of thread-local `symbol`
    _PyJit_HOLE_PART,           // part `target` of the same stencil
    _PyJit_HOLE_SUPPORT,        // the support image
} _PyJitHoleKind;

typedef enum {
    _PyJit_PART_HOT,
    _PyJit_PART_COLD,
    _PyJit_PART_DATA,
} _PyJitPartKind;

/* How a hole's 32 bits are written, with S the bound address or value, A
   the addend and P the hole's address */
typedef enum {
    _PyJit_PATCH_REL32,         // S + A - P, within 32 signed bits
    _PyJit_PATCH_VALUE32,       // S + A - P modulo 2^32 (a field's value)
    _PyJit_PATCH_IMM32,         // S, a signed 32-bit immediate
    _PyJit_PATCH_ABS64,         // S + A, 64 bits: a slot of the GOT
} _PyJitPatch;

typedef struct {
    uint32_t offset;            // in its part
    int32_t addend;
    uint16_t symbol;
    uint8_t part;               // _PyJitPartKind
    uint8_t kind;               // _PyJitHoleKind
    uint8_t patch;              // _PyJitPatch
    uint8_t target;             // _PyJitPartKind, for _PyJit_HOLE_PART
} _PyJitHole;

/* One op of a part's call frame information, at offset pc of the part:
   op is the DWARF CFA opcode (Include/internal/pycore_jit_unwind.h):
   def_cfa (reg, value), def_cfa_register (reg), def_cfa_offset (value),
   offset (reg saved at CFA + value), restore (reg), remember_state,
   restore_state. */
typedef struct {
    uint32_t pc;
    uint8_t op;
    uint8_t reg;
    int32_t value;
} _PyJitCfaOp;

typedef struct {
    const uint8_t *bytes;       // NULL for a part the stencil does not have
    uint32_t size;
    const _PyJitCfaOp *cfa;
    uint16_t ncfa;
} _PyJitPart;

typedef struct {
    const char *name;           // NULL for an arm with no stencil
    _PyJitPart parts[3];        // by _PyJitPartKind
    const _PyJitHole *holes;
    uint16_t nholes;
    int16_t generic;            // the handler id of _PyJit_HOLE_MISS's arm, or -1
} _PyJitStencil;

#ifdef __cplusplus
}
#endif
#endif   // !Py_INTERNAL_JIT_STENCILS_H
