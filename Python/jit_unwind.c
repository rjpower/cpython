/* DWARF call frame information for generated code, and its registration
 * with libgcc's unwinder (Include/internal/pycore_jit_unwind.h). */

#include "Python.h"
#include "pycore_jit_unwind.h"

#if defined(PY_HAVE_PERF_TRAMPOLINE) || defined(_PY_JIT_SUPPORTED)

#include <string.h>

static void
put_u8(_PyJitUnwind *u, uint8_t x)
{
    *u->p++ = x;
}

static void
put_u16(_PyJitUnwind *u, uint16_t x)
{
    memcpy(u->p, &x, sizeof(x));
    u->p += sizeof(x);
}

static void
put_u32(_PyJitUnwind *u, uint32_t x)
{
    memcpy(u->p, &x, sizeof(x));
    u->p += sizeof(x);
}

/* ULEB128: seven bits at a time, low first, with the continuation bit set
 * on all but the last byte. */
static void
put_uleb128(_PyJitUnwind *u, uint32_t v)
{
    for (; v >= 0x80; v >>= 7) {
        put_u8(u, (uint8_t)((v & 0x7f) | 0x80));
    }
    put_u8(u, (uint8_t)v);
}

static void
put_sleb128(_PyJitUnwind *u, int32_t v)
{
    for (; (uint32_t)(v + 0x40) >= 0x80; v >>= 7) {
        put_u8(u, (uint8_t)((v & 0x7f) | 0x80));
    }
    put_u8(u, (uint8_t)(v & 0x7f));
}

/* Pad with DW_CFA_nop to a multiple of the pointer size from the frame's
 * start. */
static void
pad(_PyJitUnwind *u)
{
    while ((size_t)(u->p - u->start) % sizeof(uintptr_t) != 0) {
        put_u8(u, DWRF_CFA_nop);
    }
}

/* Close the entry whose length field is at `length`. */
static void
close_entry(_PyJitUnwind *u, uint8_t *length)
{
    uint32_t size = (uint32_t)((u->p - length) - 4);
    memcpy(length, &size, sizeof(size));
}

void
_PyJitUnwind_Begin(_PyJitUnwind *u, uint8_t *buffer, size_t code_size)
{
    u->start = u->p = buffer;

    /* The CIE */
    uint8_t *cie = u->p;
    put_u32(u, 0);                          // length, set below
    put_u32(u, 0);                          // CIE id
    put_u8(u, 1);                           // version
    put_u8(u, 'z');                         // augmentation "zR": data follows
    put_u8(u, 'R');                         // and holds the FDE pointer encoding
    put_u8(u, 0);
#ifdef __x86_64__
    put_uleb128(u, 1);                      // code alignment factor
#else
    put_uleb128(u, 4);                      // one instruction
#endif
    put_sleb128(u, -(int32_t)sizeof(uintptr_t));    // data alignment factor
    put_u8(u, DWRF_REG_RA);                 // return address column
    put_uleb128(u, 1);                      // augmentation data length
    put_u8(u, DWRF_EH_PE_pcrel | DWRF_EH_PE_sdata4);
    /* The rules at a function's first instruction */
    put_u8(u, DWRF_CFA_def_cfa);
    put_uleb128(u, DWRF_REG_SP);
#ifdef __x86_64__
    put_uleb128(u, sizeof(uintptr_t));      // CFA = rsp + 8, over the return address
    put_u8(u, DWRF_CFA_offset | DWRF_REG_RA);
    put_uleb128(u, 1);                      // at CFA - 8
#else
    put_uleb128(u, 0);                      // the return address is in the link register
#endif
    pad(u);
    close_entry(u, cie);

    /* The FDE */
    u->fde = u->p;
    put_u32(u, 0);                          // length, set by End
    put_u32(u, (uint32_t)(u->p - u->start));    // back to the CIE
    /* The code starts at the frame's offset 0 - round8(code_size) */
    int32_t code_to_frame = (int32_t)_Py_SIZE_ROUND_UP(code_size, 8);
    put_u32(u, (uint32_t)-(code_to_frame + (int32_t)(u->p - u->start)));
    put_u32(u, (uint32_t)code_size);        // the code's range
    put_uleb128(u, 0);                      // augmentation data length
}

void
_PyJitUnwind_Advance(_PyJitUnwind *u, uint32_t delta)
{
    if (delta == 0) {
        return;
    }
    if (delta < 0x40) {
        put_u8(u, (uint8_t)(DWRF_CFA_advance_loc | delta));
    }
    else if (delta < 0x100) {
        put_u8(u, 0x02);                    // DW_CFA_advance_loc1
        put_u8(u, (uint8_t)delta);
    }
    else if (delta < 0x10000) {
        put_u8(u, 0x03);                    // DW_CFA_advance_loc2
        put_u16(u, (uint16_t)delta);
    }
    else {
        put_u8(u, 0x04);                    // DW_CFA_advance_loc4
        put_u32(u, delta);
    }
}

void
_PyJitUnwind_DefCfa(_PyJitUnwind *u, unsigned reg, uint32_t offset)
{
    put_u8(u, DWRF_CFA_def_cfa);
    put_uleb128(u, reg);
    put_uleb128(u, offset);
}

void
_PyJitUnwind_DefCfaOffset(_PyJitUnwind *u, uint32_t offset)
{
    put_u8(u, DWRF_CFA_def_cfa_offset);
    put_uleb128(u, offset);
}

void
_PyJitUnwind_DefCfaRegister(_PyJitUnwind *u, unsigned reg)
{
    put_u8(u, DWRF_CFA_def_cfa_register);
    put_uleb128(u, reg);
}

void
_PyJitUnwind_Offset(_PyJitUnwind *u, unsigned reg, uint32_t words)
{
    assert(reg < 0x40);
    put_u8(u, (uint8_t)(DWRF_CFA_offset | reg));
    put_uleb128(u, words);
}

void
_PyJitUnwind_Restore(_PyJitUnwind *u, unsigned reg)
{
    assert(reg < 0x40);
    put_u8(u, (uint8_t)(DWRF_CFA_restore | reg));
}

void
_PyJitUnwind_RememberState(_PyJitUnwind *u)
{
    put_u8(u, 0x0a);                        // DW_CFA_remember_state
}

void
_PyJitUnwind_RestoreState(_PyJitUnwind *u)
{
    put_u8(u, 0x0b);                        // DW_CFA_restore_state
}

#if defined(__aarch64__)
void
_PyJitUnwind_NegateRaState(_PyJitUnwind *u)
{
    put_u8(u, DWRF_CFA_AARCH64_negate_ra_state);
}
#endif

size_t
_PyJitUnwind_End(_PyJitUnwind *u)
{
    pad(u);
    close_entry(u, u->fde);
    return (size_t)(u->p - u->start);
}

#endif  // perf trampoline or JIT

#ifdef _PY_JIT_SUPPORTED

/* libgcc exports these, but not every distribution's headers declare them. */
extern void __register_frame(const void *);
extern void __deregister_frame(const void *);

size_t
_PyJitUnwind_Terminate(_PyJitUnwind *u)
{
    put_u32(u, 0);
    return (size_t)(u->p - u->start);
}

void
_PyJitUnwind_Register(const uint8_t *eh_frame)
{
    __register_frame(eh_frame);
}

void
_PyJitUnwind_Deregister(const uint8_t *eh_frame)
{
    __deregister_frame(eh_frame);
}

#endif  // _PY_JIT_SUPPORTED
