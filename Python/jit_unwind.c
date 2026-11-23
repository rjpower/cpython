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

#include "pycore_lock.h"            // PyMutex

#include <elf.h>

/* libgcc exports these, but not every distribution's headers declare them. */
extern void __register_frame(const void *);
extern void __deregister_frame(const void *);

size_t
_PyJitUnwind_Terminate(_PyJitUnwind *u)
{
    put_u32(u, 0);
    return (size_t)(u->p - u->start);
}

/* gdb's JIT interface: gdb sets a breakpoint in __jit_debug_register_code()
 * and, when it is hit, reads the symbol file (an ELF image in this process's
 * memory) of descriptor.relevant_entry.  The list and the descriptor are
 * guarded by one lock. */
enum {
    JIT_NOACTION = 0,
    JIT_REGISTER_FN = 1,
    JIT_UNREGISTER_FN = 2,
};

struct jit_code_entry {
    struct jit_code_entry *next;
    struct jit_code_entry *prev;
    const char *symfile_addr;
    uint64_t symfile_size;
};

struct jit_descriptor {
    uint32_t version;
    uint32_t action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
};

static PyMutex jit_debug_mutex;

Py_EXPORTED_SYMBOL volatile struct jit_descriptor __jit_debug_descriptor = {
    1, JIT_NOACTION, NULL, NULL
};

Py_EXPORTED_SYMBOL void __attribute__((noinline))
__jit_debug_register_code(void)
{
    /* gdb's breakpoint; keep the call from being optimised away */
    __asm__ __volatile__("" ::: "memory");
}

static void
gdb_announce(struct jit_code_entry *entry, uint32_t action)
{
    __jit_debug_descriptor.relevant_entry = entry;
    __jit_debug_descriptor.action_flag = action;
    __jit_debug_register_code();
    __jit_debug_descriptor.action_flag = JIT_NOACTION;
    __jit_debug_descriptor.relevant_entry = NULL;
}

/* An ELF image for gdb of one unit: .text, which is the code in this
 * process's memory (no bytes in the image), one function symbol, and the
 * unit's .eh_frame, which gdb unwinds with.  The FDE addresses the code
 * pc-relative, so .eh_frame is declared at the address it has in the code
 * block.  Returns the entry, or NULL if there is no memory. */
static struct jit_code_entry *
gdb_register(const uint8_t *code, size_t frame_offset, size_t frame_size,
             const char *name)
{
    enum {SH_NULL, SH_TEXT, SH_EH_FRAME, SH_SHSTRTAB, SH_STRTAB, SH_SYMTAB, SH_NUM};
    static const char shstrtab[] =
        "\0.text\0.eh_frame\0.shstrtab\0.strtab\0.symtab";
    const size_t sh_text = 1;
    const size_t sh_eh_frame = sh_text + sizeof(".text");
    const size_t sh_shstrtab = sh_eh_frame + sizeof(".eh_frame");
    const size_t sh_strtab = sh_shstrtab + sizeof(".shstrtab");
    const size_t sh_symtab = sh_strtab + sizeof(".strtab");
    const size_t strtab_size = 1 + strlen(name) + 1;
    const size_t symtab_size = 3 * sizeof(Elf64_Sym);

    size_t eh_off = _Py_SIZE_ROUND_UP(sizeof(Elf64_Ehdr), 16);
    size_t shstr_off = eh_off + frame_size;
    size_t str_off = shstr_off + sizeof(shstrtab);
    size_t sym_off = _Py_SIZE_ROUND_UP(str_off + strtab_size, 8);
    size_t sh_off = _Py_SIZE_ROUND_UP(sym_off + symtab_size, sizeof(Elf64_Shdr));
    size_t total_size = sh_off + SH_NUM * sizeof(Elf64_Shdr);

    struct jit_code_entry *entry = PyMem_RawCalloc(1, sizeof(*entry) + total_size);
    if (entry == NULL) {
        return NULL;
    }
    uint8_t *buf = (uint8_t *)(entry + 1);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buf;
    memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
    ehdr->e_ident[EI_CLASS] = ELFCLASS64;
    ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr->e_ident[EI_VERSION] = EV_CURRENT;
    ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
    ehdr->e_type = ET_DYN;
    ehdr->e_machine = EM_X86_64;
    ehdr->e_version = EV_CURRENT;
    ehdr->e_shoff = sh_off;
    ehdr->e_ehsize = sizeof(Elf64_Ehdr);
    ehdr->e_shentsize = sizeof(Elf64_Shdr);
    ehdr->e_shnum = SH_NUM;
    ehdr->e_shstrndx = SH_SHSTRTAB;

    memcpy(buf + eh_off, code + frame_offset, frame_size);
    memcpy(buf + shstr_off, shstrtab, sizeof(shstrtab));
    memcpy(buf + str_off + 1, name, strlen(name));

    Elf64_Sym *syms = (Elf64_Sym *)(buf + sym_off);
    syms[1].st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
    syms[1].st_shndx = SH_TEXT;
    syms[2].st_name = 1;
    syms[2].st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC);
    syms[2].st_other = STV_DEFAULT;
    syms[2].st_shndx = SH_TEXT;
    syms[2].st_value = (Elf64_Addr)(uintptr_t)code;     // absolute, as in ET_DYN
    syms[2].st_size = frame_offset;

    Elf64_Shdr *shdrs = (Elf64_Shdr *)(buf + sh_off);
    shdrs[SH_TEXT] = (Elf64_Shdr){
        .sh_name = sh_text, .sh_type = SHT_NOBITS,
        .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
        .sh_addr = (Elf64_Addr)(uintptr_t)code, .sh_size = frame_offset,
        .sh_addralign = 16};
    shdrs[SH_EH_FRAME] = (Elf64_Shdr){
        .sh_name = sh_eh_frame, .sh_type = SHT_PROGBITS, .sh_flags = SHF_ALLOC,
        .sh_addr = (Elf64_Addr)(uintptr_t)(code + frame_offset),
        .sh_offset = eh_off, .sh_size = frame_size, .sh_addralign = 8};
    shdrs[SH_SHSTRTAB] = (Elf64_Shdr){
        .sh_name = sh_shstrtab, .sh_type = SHT_STRTAB,
        .sh_offset = shstr_off, .sh_size = sizeof(shstrtab), .sh_addralign = 1};
    shdrs[SH_STRTAB] = (Elf64_Shdr){
        .sh_name = sh_strtab, .sh_type = SHT_STRTAB,
        .sh_offset = str_off, .sh_size = strtab_size, .sh_addralign = 1};
    shdrs[SH_SYMTAB] = (Elf64_Shdr){
        .sh_name = sh_symtab, .sh_type = SHT_SYMTAB,
        .sh_offset = sym_off, .sh_size = symtab_size,
        .sh_link = SH_STRTAB, .sh_info = 2,         // the first global symbol
        .sh_addralign = 8, .sh_entsize = sizeof(Elf64_Sym)};

    entry->symfile_addr = (const char *)buf;
    entry->symfile_size = total_size;
    PyMutex_Lock(&jit_debug_mutex);
    entry->next = __jit_debug_descriptor.first_entry;
    if (entry->next != NULL) {
        entry->next->prev = entry;
    }
    __jit_debug_descriptor.first_entry = entry;
    gdb_announce(entry, JIT_REGISTER_FN);
    PyMutex_Unlock(&jit_debug_mutex);
    return entry;
}

static void
gdb_deregister(struct jit_code_entry *entry)
{
    PyMutex_Lock(&jit_debug_mutex);
    if (entry->prev != NULL) {
        entry->prev->next = entry->next;
    }
    else {
        __jit_debug_descriptor.first_entry = entry->next;
    }
    if (entry->next != NULL) {
        entry->next->prev = entry->prev;
    }
    gdb_announce(entry, JIT_UNREGISTER_FN);
    PyMutex_Unlock(&jit_debug_mutex);
    PyMem_RawFree(entry);
}

void
_PyJitUnwind_Register(_PyJitUnwindEntry *entry, const _PyJitCode *code,
                      size_t frame_offset, const char *name)
{
    entry->eh_frame = code->start + frame_offset;
    __register_frame(entry->eh_frame);
    entry->gdb = gdb_register(code->start, frame_offset,
                              code->size - frame_offset, name);
}

void
_PyJitUnwind_Deregister(_PyJitUnwindEntry *entry)
{
    if (entry->gdb != NULL) {
        gdb_deregister(entry->gdb);
    }
    __deregister_frame(entry->eh_frame);
}

void
_PyJitUnwind_AfterFork_Child(void)
{
    /* Another thread may have held the lock at fork(); only this thread
     * runs in the child, and it is not inside a registration. */
    jit_debug_mutex = (PyMutex){0};
}

#endif  // _PY_JIT_SUPPORTED
