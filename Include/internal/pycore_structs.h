/* This files contains various key structs that are widely used
 * and do not depend on other headers. */

#ifndef Py_INTERNAL_STRUCTS_H
#define Py_INTERNAL_STRUCTS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>      // uint16_t


typedef struct {
    uint16_t value_and_backoff;
} _Py_BackoffCounter;

/* A code unit of the stack bytecode: 1-byte opcode + 1-byte oparg.  Code
 * objects hold IR words instead (_PyIRWord, Include/internal/pycore_ir.h);
 * only the thread-local bytecode arrays of the free-threaded build still
 * use this type.
 */
typedef union {
    uint16_t cache;
    struct {
        uint8_t code;
        uint8_t arg;
    } op;
} _Py_CODEUNIT;

/* A word of the register IR; see pycore_ir.h. */
typedef uint64_t _PyIRWord;

/* What an interpreter instruction pointer points at */
typedef _PyIRWord _PyInstr;


/* Abstract tree node. */
typedef struct {
    PyObject_HEAD
} PyHamtNode;


/* An HAMT immutable mapping collection. */
typedef struct {
    PyObject_HEAD
    PyHamtNode *h_root;
    PyObject *h_weakreflist;
    Py_ssize_t h_count;
} PyHamtObject;

typedef struct {
    PyObject_VAR_HEAD
    uint32_t b_bitmap;
    PyObject *b_array[1];
} PyHamtNode_Bitmap;

#include "pycore_context.h"       // _PyContextTokenMissing

// An owned C-local value workspace, initialized/retired with the CValue API.
// Mortal values carry real counts in both builds. This is not a precise root
// registration; native conservative scanning remains a separate policy.
typedef struct _PyCValue {
    PyValue ref;
} _PyCValue;


#ifdef __cplusplus
}
#endif
#endif /* Py_INTERNAL_STRUCTS_H */
