#ifndef Py_INTERNAL_JIT_MEMORY_H
#define Py_INTERNAL_JIT_MEMORY_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Code memory of the baseline JIT.

   One reservation of address space per process, placed so that every point
   of it is within +-2 GiB of the text of the object that holds the
   interpreter (the executable, or libpython).  A `call rel32` from
   generated code then reaches every C function of that object.  The
   reservation is private anonymous memory, so a forked child inherits its
   own copy.

   Blocks are whole pages.  A page is never writable and executable at the
   same time, and a page is only made writable while it belongs to one block
   that is being filled, or while a live block is patched
   (_PyJitMemory_Patch()), and no thread can be running code in it.  Because
   no block shares a page with another, filling a block never touches a page
   that holds live code.  A patch writes live code, under the GIL: a thread
   runs compiled code only while it holds the GIL, and one that released it
   in a call from compiled code takes it again before it returns there.  The
   cost is the unused tail of a block's last page.

   The JIT is supported on Linux x86-64 in the GIL build only. */

#if defined(__linux__) && defined(__x86_64__) && !defined(Py_GIL_DISABLED)
#  define _PY_JIT_SUPPORTED 1
#endif

#ifdef _PY_JIT_SUPPORTED

#include <stddef.h>
#include <stdint.h>

/* The final location of assembled code: `size` bytes at `start`. */
typedef struct {
    uint8_t *start;
    size_t size;
} _PyJitCode;

typedef struct {
    uintptr_t reservation;      // first byte of the reservation
    size_t reservation_size;
    uintptr_t text_low;         // the object whose C functions calls reach
    uintptr_t text_high;
    size_t pages_in_use;
    size_t bytes_in_use;        // sum of the sizes of the live blocks
    size_t page_size;
} _PyJitMemoryStats;

/* Make the reservation.  Idempotent.  Returns 0, or -1 if no address range
   within reach of the text could be reserved: the JIT is then unavailable,
   because generated code has no other way to call C. */
PyAPI_FUNC(int) _PyJitMemory_Init(void);

/* Allocate `size` bytes (rounded up to whole pages) of read-write memory and
   set *code.  Returns -1, with no exception set, if the reservation is
   exhausted or unavailable. */
PyAPI_FUNC(int) _PyJitMemory_Alloc(size_t size, _PyJitCode *code);

/* Make an allocated block read-execute.  Returns -1 if the system refuses. */
PyAPI_FUNC(int) _PyJitMemory_Seal(const _PyJitCode *code);

/* Write n bytes at `at`, in a sealed block, and seal its pages again.  The
   caller holds the GIL (see above).  Returns -1 if the system refuses. */
PyAPI_FUNC(int) _PyJitMemory_Patch(uint8_t *at, const void *bytes, size_t n);

/* Return the pages of a block, sealed or not, to the reservation. */
PyAPI_FUNC(void) _PyJitMemory_Free(_PyJitCode *code);

PyAPI_FUNC(void) _PyJitMemory_GetStats(_PyJitMemoryStats *stats);

#endif  // _PY_JIT_SUPPORTED

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_JIT_MEMORY_H */
