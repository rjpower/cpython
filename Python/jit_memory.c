/* Code memory of the baseline JIT; see pycore_jit_memory.h. */

#include "Python.h"
#include "pycore_jit_memory.h"

#ifdef _PY_JIT_SUPPORTED

#include <link.h>                 // dl_iterate_phdr()
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_FIXED_NOREPLACE
#  define MAP_FIXED_NOREPLACE 0x100000
#endif

#define RESERVATION_SIZE ((size_t)512 << 20)
// Every call from the reservation to the text must stay below 2 GiB.  The
// slack covers the rel32 field's own offset and rounding.
#define MAX_SPAN (((uintptr_t)1 << 31) - ((uintptr_t)1 << 20))
// Candidate addresses are tried at this stride away from the text.
#define SEARCH_STRIDE ((uintptr_t)4 << 20)

static struct {
    PyMutex lock;
    int state;                    // 0 = not tried, 1 = ready, -1 = unavailable
    uint8_t *base;
    size_t page_size;
    size_t npages;
    uint8_t *used;                // one byte per page: 1 when part of a block
    size_t next;                  // the search for free pages starts here
    uintptr_t text_low;
    uintptr_t text_high;
    size_t pages_in_use;
    size_t bytes_in_use;
} memory;

typedef struct {
    uintptr_t symbol;
    uintptr_t low;
    uintptr_t high;
} TextSearch;

/* The extent of the loaded object that contains the symbol: the span of its
   PT_LOAD segments. */
static int
find_text(struct dl_phdr_info *info, size_t size, void *data)
{
    TextSearch *search = (TextSearch *)data;
    uintptr_t low = UINTPTR_MAX, high = 0;
    int contains = 0;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[i];
        if (phdr->p_type != PT_LOAD) {
            continue;
        }
        uintptr_t start = info->dlpi_addr + phdr->p_vaddr;
        uintptr_t end = start + phdr->p_memsz;
        low = Py_MIN(low, start);
        high = Py_MAX(high, end);
        contains |= start <= search->symbol && search->symbol < end;
    }
    if (!contains) {
        return 0;
    }
    search->low = low;
    search->high = high;
    return 1;
}

static uint8_t *
try_reserve(uintptr_t address)
{
    void *p = mmap((void *)address, RESERVATION_SIZE, PROT_NONE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE
                   | MAP_FIXED_NOREPLACE, -1, 0);
    if (p == MAP_FAILED) {
        return NULL;
    }
    if ((uintptr_t)p != address) {
        // A kernel that ignores MAP_FIXED_NOREPLACE treats it as a hint.
        munmap(p, RESERVATION_SIZE);
        return NULL;
    }
    return p;
}

/* Reserve address space such that the whole reservation and the text lie in
   a window of less than MAX_SPAN: above the text first, then below. */
static uint8_t *
reserve_near(uintptr_t low, uintptr_t high)
{
    uintptr_t above = (high + SEARCH_STRIDE - 1) & ~(SEARCH_STRIDE - 1);
    for (uintptr_t a = above; a + RESERVATION_SIZE - low < MAX_SPAN;
         a += SEARCH_STRIDE)
    {
        uint8_t *p = try_reserve(a);
        if (p != NULL) {
            return p;
        }
    }
    uintptr_t below = (low & ~(SEARCH_STRIDE - 1)) - RESERVATION_SIZE;
    for (uintptr_t a = below; a >= SEARCH_STRIDE && high - a < MAX_SPAN;
         a -= SEARCH_STRIDE)
    {
        uint8_t *p = try_reserve(a);
        if (p != NULL) {
            return p;
        }
    }
    return NULL;
}

static int
init_locked(void)
{
    if (memory.state != 0) {
        return memory.state;
    }
    memory.state = -1;
    TextSearch search = {.symbol = (uintptr_t)&_PyJitMemory_Init};
    if (!dl_iterate_phdr(find_text, &search)) {
        return -1;
    }
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        return -1;
    }
    uint8_t *base = reserve_near(search.low, search.high);
    if (base == NULL) {
        return -1;
    }
    size_t npages = RESERVATION_SIZE / (size_t)page_size;
    uint8_t *used = PyMem_RawCalloc(npages, 1);
    if (used == NULL) {
        munmap(base, RESERVATION_SIZE);
        return -1;
    }
    memory.base = base;
    memory.page_size = (size_t)page_size;
    memory.npages = npages;
    memory.used = used;
    memory.text_low = search.low;
    memory.text_high = search.high;
    memory.state = 1;
    return 1;
}

int
_PyJitMemory_Init(void)
{
    PyMutex_Lock(&memory.lock);
    int state = init_locked();
    PyMutex_Unlock(&memory.lock);
    return state > 0 ? 0 : -1;
}

/* The first run of `count` free pages at or after memory.next, wrapping once;
   returns npages if there is none. */
static size_t
find_run(size_t count)
{
    size_t npages = memory.npages;
    size_t start = memory.next;
    for (int pass = 0; pass < 2; pass++) {
        size_t end = pass == 0 ? npages : Py_MIN(start + count, npages);
        size_t i = pass == 0 ? start : 0;
        size_t run = 0;
        for (; i < end; i++) {
            run = memory.used[i] ? 0 : run + 1;
            if (run == count) {
                return i + 1 - count;
            }
        }
    }
    return npages;
}

int
_PyJitMemory_Alloc(size_t size, _PyJitCode *code)
{
    if (_PyJitMemory_Init() < 0 || size == 0) {
        return -1;
    }
    PyMutex_Lock(&memory.lock);
    size_t page_size = memory.page_size;
    int result = -1;
    if (size <= RESERVATION_SIZE) {
        size_t count = (size + page_size - 1) / page_size;
        size_t first = find_run(count);
        if (first < memory.npages) {
            uint8_t *start = memory.base + first * page_size;
            if (mprotect(start, count * page_size, PROT_READ | PROT_WRITE) == 0) {
                memset(memory.used + first, 1, count);
                memory.next = first + count;
                memory.pages_in_use += count;
                memory.bytes_in_use += size;
                code->start = start;
                code->size = size;
                result = 0;
            }
        }
    }
    PyMutex_Unlock(&memory.lock);
    return result;
}

int
_PyJitMemory_Seal(const _PyJitCode *code)
{
    size_t length = (code->size + memory.page_size - 1) & ~(memory.page_size - 1);
    return mprotect(code->start, length, PROT_READ | PROT_EXEC);
}

int
_PyJitMemory_Patch(uint8_t *at, const void *bytes, size_t n)
{
    uintptr_t mask = ~(uintptr_t)(memory.page_size - 1);
    uint8_t *start = (uint8_t *)((uintptr_t)at & mask);
    size_t length = (((uintptr_t)at + n + memory.page_size - 1) & mask) -
                    (uintptr_t)start;
    if (mprotect(start, length, PROT_READ | PROT_WRITE) < 0) {
        return -1;
    }
    memcpy(at, bytes, n);
    return mprotect(start, length, PROT_READ | PROT_EXEC);
}

void
_PyJitMemory_Free(_PyJitCode *code)
{
    PyMutex_Lock(&memory.lock);
    size_t page_size = memory.page_size;
    size_t count = (code->size + page_size - 1) / page_size;
    size_t first = (size_t)(code->start - memory.base) / page_size;
    assert(first + count <= memory.npages);
    // Drop the contents, then the access.  The pages stay marked as used if
    // the system refuses, so that they are never handed out again.
    int dropped = madvise(code->start, count * page_size, MADV_DONTNEED) == 0
        && mprotect(code->start, count * page_size, PROT_NONE) == 0;
    assert(dropped);
    if (dropped) {
        memset(memory.used + first, 0, count);
        memory.next = Py_MIN(memory.next, first);
        memory.pages_in_use -= count;
    }
    memory.bytes_in_use -= code->size;
    PyMutex_Unlock(&memory.lock);
    code->start = NULL;
    code->size = 0;
}

void
_PyJitMemory_GetStats(_PyJitMemoryStats *stats)
{
    PyMutex_Lock(&memory.lock);
    *stats = (_PyJitMemoryStats){
        .reservation = (uintptr_t)memory.base,
        .reservation_size = memory.base != NULL ? RESERVATION_SIZE : 0,
        .text_low = memory.text_low,
        .text_high = memory.text_high,
        .pages_in_use = memory.pages_in_use,
        .bytes_in_use = memory.bytes_in_use,
        .page_size = memory.page_size,
    };
    PyMutex_Unlock(&memory.lock);
}

#endif  // _PY_JIT_SUPPORTED
