/* The collector's walk of C stacks (boil/design/11-precise-roots.md,
 * section 3).
 *
 * The walk goes outward from a start context, one frame at a time, with
 * the frame's descriptor decoded from its object's .eh_frame: the CFA, the
 * return address's slot and the slots where the frame saved callee-saved
 * registers.  Each word of the stack has one owner, the frame whose extent
 * [sp, CFA) holds it; a save slot, and a register's value, is owned by the
 * first frame outward that saves that register (3.1).  A word is scanned,
 * conservatively, if and only if its owner is opaque; a precise frame
 * (section py_gc_precise, a registered range, or for now an eval loop
 * activation) anchors what it holds where the collector sees it.  A frame
 * the walk cannot decode or validate ends it: the rest of the stack is
 * scanned conservatively, and counted (3.4).
 */

#include "Python.h"
#include "pycore_gc_trace.h"      // _PyGCRootStats
#include "pycore_gc_walk.h"
#include "pycore_lock.h"          // PyMutex
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tstate.h"        // _PyEvalActivation

#include <string.h>

#ifdef _PyGC_WALK
#  include <dlfcn.h>              // _dl_find_object()
#  include <link.h>               // dl_iterate_phdr()
#endif


/* --- registered code ranges ----------------------------------------------- */

#define MAX_RANGES 64

static struct {
    PyMutex mutex;
    int n;
    // Changed by each registration and removal: a cached descriptor for an
    // address in a removed range is stale (_PyGC_WalkCacheCheck())
    unsigned int generation;
    struct {
        uintptr_t start, end;
        int cls, flags;
        int has_desc;
        _PyGCFrameDesc desc;
    } r[MAX_RANGES];
} ranges;

int
_PyGC_RegisterCodeRange(uintptr_t start, uintptr_t end, int cls, int flags,
                        const _PyGCFrameDesc *desc)
{
    PyMutex_Lock(&ranges.mutex);
    int rc = -1;
    if (ranges.n < MAX_RANGES) {
        ranges.r[ranges.n].start = start;
        ranges.r[ranges.n].end = end;
        ranges.r[ranges.n].cls = cls;
        ranges.r[ranges.n].flags = flags;
        ranges.r[ranges.n].has_desc = desc != NULL;
        if (desc != NULL) {
            ranges.r[ranges.n].desc = *desc;
        }
        ranges.n++;
        _Py_atomic_add_uint(&ranges.generation, 1);
        rc = 0;
    }
    PyMutex_Unlock(&ranges.mutex);
    return rc;
}

void
_PyGC_UnregisterCodeRange(uintptr_t start)
{
    PyMutex_Lock(&ranges.mutex);
    for (int i = 0; i < ranges.n; i++) {
        if (ranges.r[i].start == start) {
            ranges.r[i] = ranges.r[--ranges.n];
            _Py_atomic_add_uint(&ranges.generation, 1);
            break;
        }
    }
    PyMutex_Unlock(&ranges.mutex);
}

#ifdef _PyGC_WALK
/* The registered descriptor for pc, if a range holding it has one. */
static int
range_desc(uintptr_t pc, _PyGCFrameDesc *out)
{
    int found = 0;
    if (_Py_atomic_load_int_relaxed(&ranges.n) == 0) {
        return 0;
    }
    PyMutex_Lock(&ranges.mutex);
    for (int i = 0; i < ranges.n; i++) {
        if (ranges.r[i].start <= pc && pc < ranges.r[i].end) {
            if (ranges.r[i].has_desc) {
                *out = ranges.r[i].desc;
                out->func_start = ranges.r[i].start;
                out->func_end = ranges.r[i].end;
                found = 1;
            }
            break;
        }
    }
    PyMutex_Unlock(&ranges.mutex);
    return found;
}
#endif

/* The class of a registered range holding pc, or 0. */
static int
range_class(uintptr_t pc, int *flags)
{
    int cls = 0;
    if (_Py_atomic_load_int_relaxed(&ranges.n) == 0) {
        return 0;
    }
    PyMutex_Lock(&ranges.mutex);
    for (int i = 0; i < ranges.n; i++) {
        if (ranges.r[i].start <= pc && pc < ranges.r[i].end) {
            cls = ranges.r[i].cls;
            *flags = ranges.r[i].flags;
            break;
        }
    }
    PyMutex_Unlock(&ranges.mutex);
    return cls;
}


#ifdef _PyGC_WALK

/* --- decoding .eh_frame ---------------------------------------------------- */

// x86-64 DWARF register numbers
#define DW_RBP   6
#define DW_RSP   7
#define DW_RA    16
#define NREGS    17

// The callee-saved registers' indices in _PyGCContext.regs, by DWARF number
static const int8_t saved_index[NREGS] = {
    -1, -1, -1, 0, -1, -1, 1, -1, -1, -1, -1, -1, 2, 3, 4, 5, -1,
};

// Pointer encodings (LSB 3.0, section 10.5)
#define PE_OMIT     0xff
#define PE_FORMAT   0x0f
#define PE_APPLY    0x70
#define PE_INDIRECT 0x80
#define PE_PCREL    0x10
#define PE_DATAREL  0x30

typedef struct {
    const uint8_t *p;
    const uint8_t *end;
    int bad;
} reader;

static uint8_t
rd_u8(reader *r)
{
    if (r->p >= r->end) {
        r->bad = 1;
        return 0;
    }
    return *r->p++;
}

static uint64_t
rd_fixed(reader *r, int size)
{
    if (r->end - r->p < size) {
        r->bad = 1;
        return 0;
    }
    uint64_t v = 0;
    memcpy(&v, r->p, size);  // little-endian
    r->p += size;
    return v;
}

static uint64_t
rd_uleb(reader *r)
{
    uint64_t v = 0;
    int shift = 0;
    uint8_t b;
    do {
        b = rd_u8(r);
        if (shift < 64) {
            v |= (uint64_t)(b & 0x7f) << shift;
        }
        shift += 7;
    } while ((b & 0x80) && !r->bad);
    return v;
}

static int64_t
rd_sleb(reader *r)
{
    int64_t v = 0;
    int shift = 0;
    uint8_t b;
    do {
        b = rd_u8(r);
        if (shift < 64) {
            v |= (int64_t)(b & 0x7f) << shift;
        }
        shift += 7;
    } while ((b & 0x80) && !r->bad);
    if (shift < 64 && (b & 0x40)) {
        v |= -((int64_t)1 << shift);
    }
    return v;
}

/* A pointer in encoding enc; datarel is the base for DW_EH_PE_datarel.
   Only absolute, pc-relative and data-relative applications, which is
   what GCC and Clang emit for .eh_frame and .eh_frame_hdr. */
static uintptr_t
rd_encoded(reader *r, uint8_t enc, uintptr_t datarel)
{
    if (enc == PE_OMIT) {
        return 0;
    }
    uintptr_t here = (uintptr_t)r->p;
    uint64_t v;
    switch (enc & PE_FORMAT) {
        case 0x00: v = rd_fixed(r, 8); break;                      // absptr
        case 0x01: v = rd_uleb(r); break;
        case 0x02: v = rd_fixed(r, 2); break;
        case 0x03: v = rd_fixed(r, 4); break;
        case 0x04: v = rd_fixed(r, 8); break;
        case 0x09: v = (uint64_t)rd_sleb(r); break;
        case 0x0a: v = (uint64_t)(int64_t)(int16_t)rd_fixed(r, 2); break;
        case 0x0b: v = (uint64_t)(int64_t)(int32_t)rd_fixed(r, 4); break;
        case 0x0c: v = rd_fixed(r, 8); break;
        default: r->bad = 1; return 0;
    }
    switch (enc & PE_APPLY) {
        case 0: break;
        case PE_PCREL: v += here; break;
        case PE_DATAREL: v += datarel; break;
        default: r->bad = 1; return 0;
    }
    // An indirect pointer is only the personality's, which is skipped
    return (uintptr_t)v;
}

/* The rules of one row of the CFA table */
typedef struct {
    int cfa_reg;
    int64_t cfa_off;
    int8_t kind[NREGS];         // _PyGC_RULE_*; 3 for an unsupported rule
    int64_t off[NREGS];
} row;

#define RULE_UNSUPPORTED 3
#define MAX_STATE 8

typedef struct {
    uint64_t code_align;
    int64_t data_align;
    uint64_t ra_reg;
    uint8_t fde_enc;
    int has_z;                  // the FDEs have augmentation data
    int signal;
    const uint8_t *insns;
    const uint8_t *insns_end;
} cie_info;

static int
parse_cie(const uint8_t *cie, cie_info *out)
{
    reader r = {cie, cie + 4, 0};
    uint64_t len = rd_fixed(&r, 4);
    if (len == 0xffffffff) {
        return -1;  // 64-bit DWARF: not emitted for .eh_frame in practice
    }
    r.end = cie + 4 + len;
    if (rd_fixed(&r, 4) != 0) {
        return -1;  // not a CIE
    }
    uint8_t version = rd_u8(&r);
    const char *aug = (const char *)r.p;
    size_t auglen = strnlen(aug, r.end - r.p);
    r.p += auglen + 1;
    if (strstr(aug, "eh") != NULL) {
        r.p += 8;
    }
    out->code_align = rd_uleb(&r);
    out->data_align = rd_sleb(&r);
    out->ra_reg = version == 1 ? rd_u8(&r) : rd_uleb(&r);
    out->fde_enc = 0;
    out->has_z = aug[0] == 'z';
    out->signal = 0;
    if (out->has_z) {
        uint64_t n = rd_uleb(&r);
        const uint8_t *data_end = r.p + n;
        for (const char *a = aug + 1; *a && !r.bad; a++) {
            if (*a == 'R') {
                out->fde_enc = rd_u8(&r);
            }
            else if (*a == 'L') {
                (void)rd_u8(&r);
            }
            else if (*a == 'P') {
                uint8_t enc = rd_u8(&r);
                (void)rd_encoded(&r, enc & ~PE_INDIRECT, 0);
            }
            else if (*a == 'S') {
                out->signal = 1;
            }
            else {
                return -1;  // an augmentation this decoder does not know
            }
        }
        r.p = data_end;
    }
    else if (aug[0] != '\0') {
        return -1;
    }
    out->insns = r.p;
    out->insns_end = r.end;
    return r.bad ? -1 : 0;
}

/* Run CFA instructions up to target (the lookup pc).  initial is the row
   after the CIE's instructions, for DW_CFA_restore; NULL while running
   them.  0, 1 when the target is reached, or -1. */
static int
run_cfa(reader *r, const cie_info *cie, row *rw, const row *initial,
        uintptr_t *loc, uintptr_t target)
{
    row stack[MAX_STATE];
    int depth = 0;
    while (r->p < r->end && !r->bad) {
        uint8_t op = rd_u8(r);
        uint8_t low = op & 0x3f;
        uint64_t reg, delta;
        int64_t off;
        switch (op & 0xc0) {
            case 0x40:  // advance_loc
                delta = low * cie->code_align;
                goto advance;
            case 0x80:  // offset
                off = (int64_t)rd_uleb(r) * cie->data_align;
                reg = low;
                goto set_offset;
            case 0xc0:  // restore
                reg = low;
                goto restore;
        }
        switch (op) {
            case 0x00:  // nop
                continue;
            case 0x01:  // set_loc
                *loc = rd_encoded(r, cie->fde_enc, 0);
                if (*loc > target) {
                    return 1;
                }
                continue;
            case 0x02: delta = rd_fixed(r, 1) * cie->code_align; goto advance;
            case 0x03: delta = rd_fixed(r, 2) * cie->code_align; goto advance;
            case 0x04: delta = rd_fixed(r, 4) * cie->code_align; goto advance;
            case 0x05:  // offset_extended
                reg = rd_uleb(r);
                off = (int64_t)rd_uleb(r) * cie->data_align;
                goto set_offset;
            case 0x06:  // restore_extended
                reg = rd_uleb(r);
                goto restore;
            case 0x07:  // undefined
                reg = rd_uleb(r);
                if (reg < NREGS) {
                    rw->kind[reg] = _PyGC_RULE_UNDEFINED;
                }
                continue;
            case 0x08:  // same_value
                reg = rd_uleb(r);
                if (reg < NREGS) {
                    rw->kind[reg] = _PyGC_RULE_SAME;
                }
                continue;
            case 0x09:  // register: the caller's value is in another
                reg = rd_uleb(r);
                (void)rd_uleb(r);
                if (reg < NREGS) {
                    rw->kind[reg] = RULE_UNSUPPORTED;
                }
                continue;
            case 0x0a:  // remember_state
                if (depth == MAX_STATE) {
                    return -1;
                }
                stack[depth++] = *rw;
                continue;
            case 0x0b:  // restore_state
                if (depth == 0) {
                    return -1;
                }
                // The CFA rule too, as GCC's epilogues in mid-function
                // (remember, def_cfa rsp+8, ret, restore) require and as
                // libgcc's unwinder does
                *rw = stack[--depth];
                continue;
            case 0x0c:  // def_cfa
                rw->cfa_reg = (int)rd_uleb(r);
                rw->cfa_off = (int64_t)rd_uleb(r);
                continue;
            case 0x0d:  // def_cfa_register
                rw->cfa_reg = (int)rd_uleb(r);
                continue;
            case 0x0e:  // def_cfa_offset
                rw->cfa_off = (int64_t)rd_uleb(r);
                continue;
            case 0x0f:  // def_cfa_expression
                return -1;
            case 0x10:  // expression
            case 0x16:  // val_expression
                reg = rd_uleb(r);
                r->p += rd_uleb(r);
                if (reg < NREGS) {
                    rw->kind[reg] = RULE_UNSUPPORTED;
                }
                continue;
            case 0x11:  // offset_extended_sf
                reg = rd_uleb(r);
                off = rd_sleb(r) * cie->data_align;
                goto set_offset;
            case 0x12:  // def_cfa_sf
                rw->cfa_reg = (int)rd_uleb(r);
                rw->cfa_off = rd_sleb(r) * cie->data_align;
                continue;
            case 0x13:  // def_cfa_offset_sf
                rw->cfa_off = rd_sleb(r) * cie->data_align;
                continue;
            case 0x14:  // val_offset
                reg = rd_uleb(r);
                (void)rd_uleb(r);
                if (reg < NREGS) {
                    rw->kind[reg] = RULE_UNSUPPORTED;
                }
                continue;
            case 0x15:  // val_offset_sf
                reg = rd_uleb(r);
                (void)rd_sleb(r);
                if (reg < NREGS) {
                    rw->kind[reg] = RULE_UNSUPPORTED;
                }
                continue;
            case 0x2e:  // GNU_args_size
                (void)rd_uleb(r);
                continue;
            case 0x2f:  // GNU_negative_offset_extended
                reg = rd_uleb(r);
                off = -(int64_t)rd_uleb(r) * cie->data_align;
                goto set_offset;
            default:
                return -1;
        }
    advance:
        if (initial == NULL) {
            return -1;  // an advance in the CIE
        }
        *loc += delta;
        if (*loc > target) {
            return 1;
        }
        continue;
    set_offset:
        if (reg < NREGS) {
            rw->kind[reg] = _PyGC_RULE_OFFSET;
            rw->off[reg] = off;
        }
        continue;
    restore:
        if (initial == NULL) {
            return -1;
        }
        if (reg < NREGS) {
            rw->kind[reg] = initial->kind[reg];
            rw->off[reg] = initial->off[reg];
        }
        continue;
    }
    return r->bad ? -1 : 0;
}

static struct link_map *core_map;

/* The FDE for pc, through the object's .eh_frame_hdr, and whether the
   object is the interpreter's image. */
static const uint8_t *
find_fde(uintptr_t pc, int *extension)
{
    struct dl_find_object dlfo;
    if (_dl_find_object((void *)pc, &dlfo) != 0 || dlfo.dlfo_eh_frame == NULL) {
        return NULL;
    }
    if (core_map == NULL) {
        struct dl_find_object self;
        if (_dl_find_object((void *)&find_fde, &self) == 0) {
            core_map = self.dlfo_link_map;
        }
    }
    *extension = dlfo.dlfo_link_map != core_map;
    const uint8_t *hdr = dlfo.dlfo_eh_frame;
    reader r = {hdr, hdr + 4, 0};
    if (rd_u8(&r) != 1) {
        return NULL;
    }
    uint8_t ptr_enc = rd_u8(&r);
    uint8_t count_enc = rd_u8(&r);
    uint8_t table_enc = rd_u8(&r);
    // The pointer and the count, at most 8 bytes each: the table follows
    r.end = hdr + 4 + 16;
    (void)rd_encoded(&r, ptr_enc, (uintptr_t)hdr);
    uintptr_t count = rd_encoded(&r, count_enc, (uintptr_t)hdr);
    if (r.bad || table_enc != (PE_DATAREL | 0x0b) || count == 0) {
        return NULL;
    }
    const int32_t *table = (const int32_t *)r.p;
    // The last entry whose initial location is at most pc
    uintptr_t lo = 0, hi = count;
    while (hi - lo > 1) {
        uintptr_t mid = (lo + hi) / 2;
        if ((uintptr_t)hdr + table[2 * mid] <= pc) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    if ((uintptr_t)hdr + table[2 * lo] > pc) {
        return NULL;
    }
    return hdr + table[2 * lo + 1];
}

int
_PyGC_DecodeDescriptor(uintptr_t lookup_pc, _PyGCFrameDesc *out)
{
    memset(out, 0, sizeof(*out));
    if (range_desc(lookup_pc, out)) {
        out->pc = lookup_pc;
        out->ok = 1;
        return 0;
    }
    out->pc = lookup_pc;
    int extension = 0;
    const uint8_t *fde = find_fde(lookup_pc, &extension);
    if (fde == NULL) {
        return -1;
    }
    out->extension = (uint8_t)extension;
    reader r = {fde, fde + 4, 0};
    uint64_t len = rd_fixed(&r, 4);
    if (len == 0 || len == 0xffffffff) {
        return -1;
    }
    r.end = fde + 4 + len;
    const uint8_t *idfield = r.p;
    uint32_t cie_off = (uint32_t)rd_fixed(&r, 4);
    cie_info cie;
    if (cie_off == 0 || parse_cie(idfield - cie_off, &cie) < 0 || cie.signal
        || cie.ra_reg != DW_RA)
    {
        return -1;
    }
    uintptr_t start = rd_encoded(&r, cie.fde_enc, 0);
    uintptr_t range = rd_encoded(&r, cie.fde_enc & PE_FORMAT, 0);
    if (r.bad || lookup_pc < start || lookup_pc >= start + range) {
        return -1;
    }
    out->func_start = start;
    out->func_end = start + range;
    reader cr = {cie.insns, cie.insns_end, 0};
    row rw;
    memset(&rw, 0, sizeof(rw));  // every register "same" until a rule
    uintptr_t loc = start;
    if (run_cfa(&cr, &cie, &rw, NULL, &loc, lookup_pc) != 0) {
        return -1;
    }
    row initial = rw;
    if (cie.has_z) {
        r.p += rd_uleb(&r);  // the FDE's augmentation data (an LSDA)
    }
    if (run_cfa(&r, &cie, &rw, &initial, &loc, lookup_pc) < 0) {
        return -1;
    }
    // The descriptor
    if (rw.cfa_reg == DW_RSP) {
        out->cfa_reg = _PyGC_CFA_SP;
    }
    else if (rw.cfa_reg < NREGS && saved_index[rw.cfa_reg] >= 0) {
        out->cfa_reg = saved_index[rw.cfa_reg];
    }
    else {
        return -1;
    }
    if (rw.cfa_off < INT32_MIN || rw.cfa_off > INT32_MAX) {
        return -1;
    }
    out->cfa_off = (int32_t)rw.cfa_off;
    if (rw.kind[DW_RA] == _PyGC_RULE_UNDEFINED) {
        out->ra_undefined = 1;
    }
    else if (rw.kind[DW_RA] == _PyGC_RULE_OFFSET) {
        out->ra_off = (int32_t)rw.off[DW_RA];
    }
    else {
        return -1;
    }
    for (int reg = 0; reg < NREGS; reg++) {
        int i = saved_index[reg];
        if (i < 0) {
            continue;
        }
        if (rw.kind[reg] == RULE_UNSUPPORTED) {
            return -1;
        }
        out->rule[i] = rw.kind[reg];
        out->off[i] = (int32_t)rw.off[reg];
    }
    out->ok = 1;
    return 0;
}


/* --- the cache -------------------------------------------------------------- */

static int
generation_cb(struct dl_phdr_info *info, size_t size, void *arg)
{
    if (size >= offsetof(struct dl_phdr_info, dlpi_subs)
                + sizeof(info->dlpi_subs)) {
        *(unsigned long long *)arg = info->dlpi_subs;
    }
    return 1;  // the first object is enough: every one reports the counts
}

void
_PyGC_WalkCacheCheck(_PyGCWalkCache *cache, struct _PyGCRootStats *stats)
{
    unsigned long long subs = 0;
    (void)dl_iterate_phdr(generation_cb, &subs);
    unsigned int gen = _Py_atomic_load_uint(&ranges.generation);
    if (subs != cache->subs || gen != cache->ranges) {
        if (cache->used > 0) {
            cache->flushes++;
        }
        if (cache->slots != NULL) {
            memset(cache->slots, 0, (cache->mask + 1) * sizeof(*cache->slots));
        }
        cache->used = 0;
        cache->subs = subs;
        cache->ranges = gen;
    }
    if (stats != NULL) {
        stats->cache_flushes += cache->flushes;
        cache->flushes = 0;
    }
}

void
_PyGC_WalkCacheFini(_PyGCWalkCache *cache)
{
    free(cache->slots);
    memset(cache, 0, sizeof(*cache));
}

static inline size_t
hash_pc(uintptr_t pc)
{
    return (size_t)((pc * 0x9E3779B97F4A7C15ull) >> 20);
}

static int
cache_grow(_PyGCWalkCache *cache)
{
    size_t n = cache->slots == NULL ? 1024 : 2 * (cache->mask + 1);
    _PyGCFrameDesc *slots = calloc(n, sizeof(*slots));
    if (slots == NULL) {
        return -1;
    }
    for (size_t i = 0; cache->slots != NULL && i <= cache->mask; i++) {
        if (cache->slots[i].pc == 0) {
            continue;
        }
        size_t j = hash_pc(cache->slots[i].pc) & (n - 1);
        while (slots[j].pc != 0) {
            j = (j + 1) & (n - 1);
        }
        slots[j] = cache->slots[i];
    }
    free(cache->slots);
    cache->slots = slots;
    cache->mask = n - 1;
    return 0;
}

int
_PyGC_FramePointerDescriptor(_PyGCFrameDesc *out)
{
    // push %rbp; mov %rsp, %rbp: the CFA is rbp + 16, the return address
    // at CFA - 8 and the caller's rbp at CFA - 16
    memset(out, 0, sizeof(*out));
    out->cfa_reg = saved_index[DW_RBP];
    out->cfa_off = 16;
    out->ra_off = -8;
    out->rule[saved_index[DW_RBP]] = _PyGC_RULE_OFFSET;
    out->off[saved_index[DW_RBP]] = -16;
    out->ok = 1;
    return 0;
}

/* The descriptor for lookup_pc, or NULL.  Not decodable is cached too. */
static const _PyGCFrameDesc *
find_desc(_PyGCWalk *w, uintptr_t lookup_pc, _PyGCFrameDesc *scratch)
{
    _PyGCWalkCache *cache = w->cache;
    if (cache == NULL) {
        (void)_PyGC_DecodeDescriptor(lookup_pc, scratch);
        return scratch;
    }
    if (cache->slots != NULL) {
        size_t j = hash_pc(lookup_pc) & cache->mask;
        while (cache->slots[j].pc != 0) {
            if (cache->slots[j].pc == lookup_pc) {
                if (w->stats != NULL) {
                    w->stats->cache_hits++;
                }
                return &cache->slots[j];
            }
            j = (j + 1) & cache->mask;
        }
    }
    if (w->stats != NULL) {
        w->stats->cache_misses++;
    }
    // An object unloaded since the root phase began could have left
    // entries for addresses that another now occupies.
    _PyGC_WalkCacheCheck(cache, w->stats);
    (void)_PyGC_DecodeDescriptor(lookup_pc, scratch);
    if ((cache->used + 1) * 2 > (cache->slots ? cache->mask + 1 : 0)
        && cache_grow(cache) < 0)
    {
        return scratch;
    }
    size_t j = hash_pc(lookup_pc) & cache->mask;
    while (cache->slots[j].pc != 0) {
        j = (j + 1) & cache->mask;
    }
    cache->slots[j] = *scratch;
    cache->used++;
    return &cache->slots[j];
}


/* --- the walk ---------------------------------------------------------------- */

// The section's bounds, from the linker; absent while nothing is marked
extern char __start_py_gc_precise[] __attribute__((weak, visibility("hidden")));
extern char __stop_py_gc_precise[] __attribute__((weak, visibility("hidden")));
// The preserve_none functions' section (Py_GC_PRECISE_PRESERVE_NONE)
extern char __start_py_gc_precise_pn[] __attribute__((weak, visibility("hidden")));
extern char __stop_py_gc_precise_pn[] __attribute__((weak, visibility("hidden")));

/* The frame's class.  *lo is where its precise words begin: sp, except
   for an eval loop activation, whose words below the activation's lo, the
   arguments it passes on the stack, are scanned as today. */
static int
frame_class(_PyGCWalk *w, const _PyGCFrameDesc *d, uintptr_t pc,
            uintptr_t sp, uintptr_t cfa, int *flags, uintptr_t *lo)
{
    *flags = 0;
    *lo = sp;
    int cls = range_class(pc, flags);
    if (cls != 0) {
        return cls;
    }
    if (!d->extension && __start_py_gc_precise != NULL
        && (uintptr_t)__start_py_gc_precise <= pc
        && pc < (uintptr_t)__stop_py_gc_precise)
    {
        return _PyGC_FRAME_PRECISE;
    }
    if (!d->extension && __start_py_gc_precise_pn != NULL
        && (uintptr_t)__start_py_gc_precise_pn <= pc
        && pc < (uintptr_t)__stop_py_gc_precise_pn)
    {
        *flags |= _PyGC_RANGE_PRESERVES_NOTHING;
        return _PyGC_FRAME_PRECISE;
    }
    if (!w->activations_opaque) {
        for (_PyEvalActivation *a = w->activations; a != NULL; a = a->prev) {
            if (a->lo != 0 && a->lo < cfa && cfa <= a->hi) {
                *lo = Py_MAX(sp, a->lo);
                if (w->stats != NULL) {
                    w->stats->activations_skipped++;
                    w->stats->eval_words_skipped +=
                        (int64_t)((cfa - *lo) / sizeof(uintptr_t));
                }
                return _PyGC_FRAME_PRECISE;
            }
        }
    }
    return d->extension ? _PyGC_FRAME_EXTENSION : _PyGC_FRAME_CORE;
}

int
_PyGC_PreservesNothingSection(uintptr_t *start, uintptr_t *stop)
{
    if (__start_py_gc_precise_pn == NULL) {
        return 0;
    }
    *start = (uintptr_t)__start_py_gc_precise_pn;
    *stop = (uintptr_t)__stop_py_gc_precise_pn;
    return 1;
}

static void
scan_word(_PyGCWalk *w, uintptr_t *loc)
{
    w->scan(w->arg, (uintptr_t)loc, (uintptr_t)(loc + 1));
}

/* Scan [sp, cfa) except the save slots, which are their owners', with
   scan: the walk's, or for a precise frame the verifier's. */
static void
scan_extent(_PyGCWalk *w, void (*scan)(void *, uintptr_t, uintptr_t),
            const _PyGCFrameDesc *d, uintptr_t sp, uintptr_t cfa)
{
    uintptr_t slots[_PyGC_NSAVED];
    int n = 0;
    for (int i = 0; i < _PyGC_NSAVED; i++) {
        if (d->rule[i] == _PyGC_RULE_OFFSET) {
            uintptr_t s = cfa + d->off[i];
            int k = n++;
            while (k > 0 && slots[k - 1] > s) {
                slots[k] = slots[k - 1];
                k--;
            }
            slots[k] = s;
        }
    }
    uintptr_t start = sp;
    for (int k = 0; k < n; k++) {
        if (slots[k] > start) {
            scan(w->arg, start, slots[k]);
        }
        start = slots[k] + sizeof(uintptr_t);
    }
    if (cfa > start) {
        scan(w->arg, start, cfa);
    }
}

static void
fallback(_PyGCWalk *w, uintptr_t pc, uintptr_t sp, uintptr_t **loc,
         const char *why)
{
    if (sp < w->top) {
        w->scan(w->arg, sp, w->top);
    }
    for (int i = 0; i < _PyGC_NSAVED; i++) {
        if (loc[i] != NULL) {
            scan_word(w, loc[i]);
        }
    }
    int flags = 0;
    if (range_class(pc, &flags) && (flags & _PyGC_RANGE_TEST)) {
        w->test_fallbacks++;
    }
    else if (w->stats != NULL) {
        w->stats->fallbacks++;
    }
#ifdef Py_DEBUG
    static uintptr_t reported;
    if (reported == 0 && !(flags & _PyGC_RANGE_TEST)) {
        reported = pc;
        Dl_info info;
        const char *obj = "?";
        uintptr_t base = 0;
        if (dladdr((void *)pc, &info) && info.dli_fname != NULL) {
            obj = info.dli_fname;
            base = (uintptr_t)info.dli_fbase;
        }
        fprintf(stderr, "gc walk: %s at pc %p (%s+%#lx); the rest of the "
                "stack is scanned conservatively\n", why, (void *)pc, obj,
                (unsigned long)(pc - base));
    }
#endif
}

int
_PyGC_WalkStack(_PyGCWalk *w, const _PyGCContext *ctx)
{
    // The values of the callee-saved registers in the start frame: their
    // locations move outward to save slots as frames are passed.
    uintptr_t regs[_PyGC_NSAVED];
    memcpy(regs, ctx->regs, sizeof(regs));
    uintptr_t *loc[_PyGC_NSAVED];
    for (int i = 0; i < _PyGC_NSAVED; i++) {
        loc[i] = &regs[i];
    }
    uintptr_t pc = ctx->pc, sp = ctx->sp, prev_cfa = 0;
    int last_opaque = 1;
    _PyGCFrameDesc scratch;
    while (sp < w->top && pc != 0) {
        const _PyGCFrameDesc *d = find_desc(w, pc - 1, &scratch);
        if (!d->ok) {
            fallback(w, pc, sp, loc, "no frame descriptor");
            return 1;
        }
        uintptr_t base;
        if (d->cfa_reg == _PyGC_CFA_SP) {
            base = sp;
        }
        else if (loc[d->cfa_reg] != NULL) {
            base = *loc[d->cfa_reg];
        }
        else {
            fallback(w, pc, sp, loc, "a CFA register with no value");
            return 1;
        }
        uintptr_t cfa = base + d->cfa_off;
        int valid = cfa > sp && cfa > prev_cfa && cfa <= w->top;
        if (valid && !d->ra_undefined) {
            uintptr_t ra = cfa + d->ra_off;
            valid = ra >= sp && ra + sizeof(uintptr_t) <= cfa;
        }
        for (int i = 0; valid && i < _PyGC_NSAVED; i++) {
            if (d->rule[i] == _PyGC_RULE_OFFSET) {
                uintptr_t s = cfa + d->off[i];
                valid = s >= sp && s + sizeof(uintptr_t) <= cfa;
            }
        }
        if (!valid) {
            fallback(w, pc, sp, loc, "a frame that fails validation");
            return 1;
        }
        int flags;
        uintptr_t precise_lo;
        int cls = frame_class(w, d, pc, sp, cfa, &flags, &precise_lo);
        int opaque = cls != _PyGC_FRAME_PRECISE;
        // Before any of the frame's words, which the scans after it own
        if (w->frame != NULL) {
            w->frame(w->arg, pc, sp, cfa, cls);
        }
        if (!opaque && precise_lo > sp) {
            w->scan(w->arg, sp, precise_lo);
        }
        if (w->stats != NULL) {
            w->stats->frames_walked++;
        }
        for (int i = 0; i < _PyGC_NSAVED; i++) {
            int rule = d->rule[i];
            if (rule == _PyGC_RULE_SAME
                && !(flags & _PyGC_RANGE_PRESERVES_NOTHING))
            {
                continue;  // still the caller's, or further out
            }
            // This frame uses the register: the value pending is its own,
            // unless it is still the caller's, as before the frame changes
            // it, and then it is the caller's owner's to scan or anchor.
            uintptr_t *slot = rule == _PyGC_RULE_OFFSET
                ? (uintptr_t *)(cfa + d->off[i]) : NULL;
            if (loc[i] != NULL && (slot == NULL || *loc[i] != *slot)) {
                if (opaque) {
                    scan_word(w, loc[i]);
                }
                else if (w->precise != NULL) {
                    w->precise(w->arg, (uintptr_t)loc[i],
                               (uintptr_t)(loc[i] + 1));
                }
            }
            loc[i] = slot;
        }
        if (opaque) {
            scan_extent(w, w->scan, d, sp, cfa);
        }
        else if (w->precise != NULL) {
            scan_extent(w, w->precise, d, sp, cfa);
        }
        last_opaque = opaque;
        if (d->ra_undefined) {
            break;
        }
        pc = *(uintptr_t *)(cfa + d->ra_off);
        prev_cfa = cfa;
        sp = cfa;
    }
    // What is still pending belongs to the outermost frame
    for (int i = 0; i < _PyGC_NSAVED; i++) {
        if (loc[i] != NULL && last_opaque) {
            scan_word(w, loc[i]);
        }
        else if (loc[i] != NULL && w->precise != NULL) {
            w->precise(w->arg, (uintptr_t)loc[i], (uintptr_t)(loc[i] + 1));
        }
    }
    return 0;
}

#else  /* !_PyGC_WALK */

int
_PyGC_FramePointerDescriptor(_PyGCFrameDesc *out)
{
    memset(out, 0, sizeof(*out));
    return -1;
}

#ifndef _PyGC_CAPTURE_ASM
void
_PyGC_CaptureCaller(_PyGCContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    _PyGC_CaptureRegisters((uintptr_t *)ctx->regs);
    ctx->sp = (uintptr_t)__builtin_frame_address(0);
}
#endif

int
_PyGC_DecodeDescriptor(uintptr_t lookup_pc, _PyGCFrameDesc *out)
{
    memset(out, 0, sizeof(*out));
    out->pc = lookup_pc;
    return -1;
}

void
_PyGC_WalkCacheCheck(_PyGCWalkCache *cache, struct _PyGCRootStats *stats)
{
}

void
_PyGC_WalkCacheFini(_PyGCWalkCache *cache)
{
}

int
_PyGC_PreservesNothingSection(uintptr_t *start, uintptr_t *stop)
{
    return 0;
}

int
_PyGC_WalkStack(_PyGCWalk *w, const _PyGCContext *ctx)
{
    // No walk: the whole stack, and the registers, conservatively
    w->scan(w->arg, (uintptr_t)ctx->regs, (uintptr_t)(ctx->regs + _PyGC_NSAVED));
    if (ctx->sp < w->top) {
        w->scan(w->arg, ctx->sp, w->top);
    }
    return 0;
}

#endif  /* _PyGC_WALK */


/* --- the detach record ------------------------------------------------------ */

/* No walk here: the collector unwinds a pinned record lazily, under the
   GIL, when it visits the thread (question 16). */
void
_PyGC_RecordDetach(PyThreadState *tstate, const _PyGCContext *ctx,
                   int pinned)
{
    _PyGCDetachRecord *rec = &((_PyThreadStateImpl *)tstate)->gc_trace.detach;
    rec->cap = *ctx;
    rec->pinned = pinned;
    rec->valid = 1;
}
