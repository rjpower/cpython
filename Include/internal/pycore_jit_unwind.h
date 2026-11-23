#ifndef Py_INTERNAL_JIT_UNWIND_H
#define Py_INTERNAL_JIT_UNWIND_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_jit_memory.h"      // _PY_JIT_SUPPORTED

#include <stddef.h>
#include <stdint.h>

/* DWARF call frame information (an .eh_frame) for generated code, written by
   one encoder for both users: the baseline JIT's units (Python/jit.c), which
   register their frame with libgcc's unwinder, and -X perf_jit's trampolines
   (Python/perf_jit_trampoline.c), which hand theirs to perf.

   A frame is one CIE and one FDE.  The code it describes lies immediately
   before the frame, padded to a multiple of 8 bytes, so the FDE addresses it
   pc-relative (DW_EH_PE_pcrel | DW_EH_PE_sdata4) and the frame is valid at
   whatever address the two are placed together.  In a unit, that is its
   code block; in a jitdump, the synthesised DSO of perf's inject. */

#if defined(PY_HAVE_PERF_TRAMPOLINE) || defined(_PY_JIT_SUPPORTED)

/* DWARF CFA opcodes */
enum {
    DWRF_CFA_nop = 0x0,
    DWRF_CFA_offset_extended = 0x5,
    DWRF_CFA_def_cfa = 0xc,
    DWRF_CFA_def_cfa_register = 0xd,
    DWRF_CFA_def_cfa_offset = 0xe,
    DWRF_CFA_offset_extended_sf = 0x11,
    DWRF_CFA_advance_loc = 0x40,        // | delta, for deltas below 64
    DWRF_CFA_offset = 0x80,             // | register, then the offset
#if defined(__aarch64__)
    DWRF_CFA_AARCH64_negate_ra_state = 0x2d,
#endif
    DWRF_CFA_restore = 0xc0             // | register
};

/* DWARF register numbers, which the ABI fixes */
enum {
#ifdef __x86_64__
    DWRF_REG_AX, DWRF_REG_DX, DWRF_REG_CX, DWRF_REG_BX,
    DWRF_REG_SI, DWRF_REG_DI, DWRF_REG_BP, DWRF_REG_SP,
    DWRF_REG_8, DWRF_REG_9, DWRF_REG_10, DWRF_REG_11,
    DWRF_REG_12, DWRF_REG_13, DWRF_REG_14, DWRF_REG_15,
    DWRF_REG_RA,    // the return address, which the CIE saves at CFA - 8
#elif defined(__aarch64__) && defined(__AARCH64EL__) && !defined(__ILP32__)
    DWRF_REG_FP = 29,
    DWRF_REG_RA = 30,
    DWRF_REG_SP = 31,
#else
#    error "Unsupported target architecture"
#endif
};

/* Pointer encodings of the FDE and of perf's .eh_frame_hdr */
enum {
    DWRF_EH_PE_udata4 = 0x03,
    DWRF_EH_PE_sdata4 = 0x0b,
    DWRF_EH_PE_pcrel = 0x10,
    DWRF_EH_PE_datarel = 0x30,
};

typedef struct {
    uint8_t *start;     // the first byte of the frame
    uint8_t *p;         // the next byte to write
    uint8_t *fde;       // the FDE's length field
} _PyJitUnwind;

/* Start a frame in buffer for code_size bytes of code, which the frame
   follows at the next multiple of 8: the CIE, whose rules are the ABI's at
   a function's first instruction, and the FDE up to its CFI program.  The
   buffer must hold the whole frame, which the program's length decides. */
extern void _PyJitUnwind_Begin(_PyJitUnwind *u, uint8_t *buffer, size_t code_size);

/* The CFI program: rows are rules that hold from the code offset they
   advance to.  Deltas are in the CIE's code alignment factor, which is 1 on
   x86-64 and 4 on AArch64. */
extern void _PyJitUnwind_Advance(_PyJitUnwind *u, uint32_t delta);
extern void _PyJitUnwind_DefCfa(_PyJitUnwind *u, unsigned reg, uint32_t offset);
extern void _PyJitUnwind_DefCfaOffset(_PyJitUnwind *u, uint32_t offset);
extern void _PyJitUnwind_DefCfaRegister(_PyJitUnwind *u, unsigned reg);
/* reg is saved at CFA - words * 8 */
extern void _PyJitUnwind_Offset(_PyJitUnwind *u, unsigned reg, uint32_t words);
extern void _PyJitUnwind_Restore(_PyJitUnwind *u, unsigned reg);
extern void _PyJitUnwind_RememberState(_PyJitUnwind *u);
extern void _PyJitUnwind_RestoreState(_PyJitUnwind *u);
#if defined(__aarch64__)
extern void _PyJitUnwind_NegateRaState(_PyJitUnwind *u);
#endif

/* Finish the FDE; returns the size of the frame. */
extern size_t _PyJitUnwind_End(_PyJitUnwind *u);

#endif  // perf trampoline or JIT

#ifdef _PY_JIT_SUPPORTED

/* A frame that ends with a zero word, which libgcc's registration reads as
   the end of the frame list: End, then this; returns the size with it. */
extern size_t _PyJitUnwind_Terminate(_PyJitUnwind *u);

/* A unit's unwind info, registered with the two unwinders of a process.
   The code block holds the code, then at frame_offset (a multiple of 8) the
   frame that Begin to Terminate wrote.  libgcc's unwinder (backtrace(),
   _Unwind_Backtrace, C++ exceptions, faulthandler's C stack) reads the frame
   in the block until the unit is deregistered, which must come before the
   block is freed.  gdb gets the unit as a symbol of `name` through its JIT
   interface; that part is skipped if there is no memory for it. */
typedef struct {
    const uint8_t *eh_frame;
    void *gdb;
} _PyJitUnwindEntry;

extern void _PyJitUnwind_Register(_PyJitUnwindEntry *entry, const _PyJitCode *code,
                                  size_t frame_offset, const char *name);
extern void _PyJitUnwind_Deregister(_PyJitUnwindEntry *entry);

/* In the child of a fork: the interface's lock may be held by a thread that
   does not exist there. */
extern void _PyJitUnwind_AfterFork_Child(void);

#endif  // _PY_JIT_SUPPORTED

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_UNWIND_H */
