#ifndef Py_INTERNAL_TIER0_H
#define Py_INTERNAL_TIER0_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_TIER0

#include "pycore_ir.h"          // _PyIRBlock

/* Tier 0, a baseline compiler of IR blocks to x86-64 machine code that
 * calls the handlers of the IR (boil/design/05-tier0-jit.md).  Built with
 * --with-tier0.  Python/tier0.c compiles; Python/ceval_tier0.c.h holds the
 * handlers in call mode and the hooks through which the interpreter enters
 * machine code. */

/* A block compiles once its heat, 16 per call and 1 per backward jump,
 * reaches the threshold: 64 calls or 1024 backward jumps. */
#define _PyTier0_THRESHOLD 1024
#define _PyTier0_CALL_HEAT 16

/* Instrumentation discards a block's code; it compiles again at most this
 * many times. */
#define _PyTier0_MAX_COMPILES 4

/* Set in the frame that a handler in call mode returns when the
 * interpreter must run the frame's next word. */
#define _PyTier0_INTERPRET 1

typedef struct _PyTier0Code {
    /* The machine code, which the interpreter tail-calls as a handler of
     * the frame at next_instr, with field A the offset of the word's code
     * from the start. */
    void *entry;
    const int32_t *offsets;     // per word, that offset, or 0 if none
    size_t size;                // of the mapping at entry
    int valid;                  // 0 once instrumentation changed the block
    /* Code that this replaced, which frames may still be running: the
     * block frees all of it with the code object. */
    struct _PyTier0Code *older;
} _PyTier0Code;

/* What the machine code calls, from Python/ceval_tier0.c.h */
typedef struct {
    const void *call_table;     // the handlers in call mode, by index
    const void *resume;         // _PyTier0_Continue()
} _PyTier0Runtime;

extern const _PyTier0Runtime _PyTier0_Runtime;

/* Compile block and return its code, or NULL if the block does
 * not qualify now (instrumented, too large) or ever.  Never raises. */
PyAPI_FUNC(_PyTier0Code *) _PyTier0_Compile(_PyIRBlock *block);

/* Discard the code of block, which instrumentation is about to change.
 * Frames that run it leave for the interpreter at their next word. */
extern void _PyTier0_Invalidate(_PyIRBlock *block);

/* Free the code of a block that is being freed. */
extern void _PyTier0_Clear(_PyIRBlock *block);

static inline _PyTier0Code *
_PyTier0_ValidCode(_PyIRBlock *block)
{
    _PyTier0Code *code = block->tier0;
    return (code != NULL && code->valid) ? code : NULL;
}

#endif /* Py_TIER0 */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TIER0_H */
