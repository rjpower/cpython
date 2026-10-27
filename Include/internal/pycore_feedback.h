#ifndef Py_INTERNAL_FEEDBACK_H
#define Py_INTERNAL_FEEDBACK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_backoff.h"     // _Py_BackoffCounter
#include "pycore_pymem.h"       // _PyMem_NoScanFree()
#include "opcode_ids.h"         // RESUME, LOAD_ATTR, ...

/* Typed feedback.
 *
 * Every instruction of a family below, and every conditional branch, owns
 * one feedback entry.  The assembler numbers the entries and stores in the
 * IR word the index of its entry (Include/internal/pycore_ir.h).  The code
 * object keeps one 32-bit initialiser per entry after its words, the kind
 * and the static operand that does not fit the word (_PyIR_FBINIT).  At
 * the first call the executing copy is allocated with the entries after the
 * words:
 *
 *     | _PyCodeExec | words | _PyFbBlock | entries |
 *
 * Entry 0 is shared by every instruction that has no entry of its own: the
 * instructions past the IR's entry limit and _Py_InitCleanup.  It never
 * specialises.  dis reads the entries of the executing copy for
 * show_caches and adaptive.
 *
 * Each entry begins with a 16-byte header.  Kind, size and opcodes:
 *
 *     COUNTER  16  RESUME, JUMP_BACKWARD
 *     BRANCH   16  POP_JUMP_IF_NONE, _NOT_NONE
 *              24  POP_JUMP_IF_FALSE, _TRUE
 *     UNARY    24  TO_BOOL, UNPACK_SEQUENCE, FOR_ITER, GET_ITER, SEND
 *     BINARY   32  BINARY_OP, COMPARE_OP, CONTAINS_OP, STORE_SUBSCR
 *     ATTR     48  LOAD_ATTR, STORE_ATTR, LOAD_SUPER_ATTR
 *     GLOBAL   32  LOAD_GLOBAL
 *     CALL     24  CALL, CALL_KW, CALL_FUNCTION_EX
 *
 * A branch keeps its 16-bit taken history, newest in bit 0, in arg.
 * POP_JUMP_IF_FALSE and _TRUE test the truth of any value and specialise
 * as TO_BOOL does, so their entries are UNARY entries of kind BRANCH: the
 * counter drives specialisation, and the history replaces the oparg.
 *
 * Fields carry the names of the cache effects in Python/bytecodes.c, which
 * is how the cases generator maps "type_version/2" to fb->type_version.
 * The sizes in the DSL now only describe the reserved units.
 *
 * Writers: the generic instruction advances the counter, and
 * Python/specialize.c writes everything else.  A specialisation starts its
 * counter at the cooldown value and every deoptimisation advances it, so the
 * counter already holds the misses since the last specialisation, and no
 * fast path writes misses.
 *
 * Specialisation re-specialises to the newest observation, as before, and
 * keeps up to four observations of a site that changes type in a
 * polymorphic table (state POLY); a fifth sets state MEGA.  Poly tables are
 * allocated on demand in an array owned by the block.
 */

typedef enum {
    FB_COUNTER,
    FB_BRANCH,
    FB_UNARY,
    FB_BINARY,
    FB_ATTR,
    FB_GLOBAL,
    FB_CALL,
} _PyFbKind;

typedef enum {
    FB_COLD,        // never specialised
    FB_MONO,        // specialised for one observation
    FB_POLY,        // the poly table holds two to four observations
    FB_MEGA,        // more than four observations
    FB_OFF,         // specialisation disabled; not entered yet
} _PyFbState;

#define FB_POLY_SIZE 4

typedef struct {
    uint8_t kind;                   // _PyFbKind
    uint8_t state;                  // _PyFbState
    _Py_BackoffCounter counter;     // warmup, cooldown and backoff
    uint16_t misses;                // reserved: the counter holds them, see above
    uint16_t hits;                  // reserved for call counts; not written yet
    union {
        uint32_t arg;               // the oparg, for IR formats that keep it here
        uint16_t history;           // BRANCH entries of the IR: see FB_BRANCH()
    };
    uint32_t poly;                  // 0, or 1 + index into the block's poly tables
} _PyFbHeader;

// TO_BOOL, UNPACK_SEQUENCE, FOR_ITER, GET_ITER, SEND
typedef struct {
    _PyFbHeader h;
    uint32_t type_version;          // operand type; guarded by TO_BOOL_ALWAYS_TRUE
    uint32_t _pad;
} _PyFbUnary;

// BINARY_OP, COMPARE_OP, CONTAINS_OP, STORE_SUBSCR
typedef struct {
    _PyFbHeader h;
    uint32_t lhs_version;           // operand types, recorded but not guarded
    uint32_t rhs_version;
    const void *descr;              // _PyBinaryOpSpecializationDescr for BINARY_OP_EXTEND
} _PyFbBinary;

/* LOAD_ATTR, STORE_ATTR, LOAD_SUPER_ATTR.  shape and shape_to are borrowed
 * from the owner's type, which outlives every object that has them
 * (Include/internal/pycore_shape.h). */
typedef struct {
    _PyFbHeader h;
    uint32_t type_version;          // Py_TYPE(owner)
    union {
        uint32_t dict_version;      // LOAD_ATTR_MODULE: keys version of the module dict
        uint32_t func_version;      // property getter or __getattribute__
        uint32_t class_version;     // owner, for loads from a class
    };
    union {
        uint32_t index;
        uint32_t offset;
        uint32_t hint;
        uint32_t dictoffset;
    };
    uint32_t _pad;
    union {                         // borrowed; valid while the versions match
        PyObject *descr;            // LOAD_ATTR_PROPERTY: the property
        // LOAD_ATTR_CLASS, LOAD_ATTR_NONDESCRIPTOR_*: the class attribute,
        // canonical, which may be an inline value
        PyValue attr_value;
        PyObject *getattribute;
        _PyShape *shape_to;         // STORE_ATTR_SHAPE_ADD: the shape after the store
    };
    _PyShape *shape;                // the owner's shape, for the shape handlers
} _PyFbAttr;

// LOAD_GLOBAL
typedef struct {
    _PyFbHeader h;
    uint32_t globals_version;       // keys versions of globals and builtins
    uint32_t builtins_version;
    uint32_t index;
    uint32_t _pad;
} _PyFbGlobal;

// CALL, CALL_KW, CALL_FUNCTION_EX
typedef struct {
    _PyFbHeader h;
    union {
        uint32_t func_version;
        uint32_t type_version;      // CALL_ALLOC_AND_ENTER_INIT: the class
    };
    uint32_t callable_type_version; // recorded but not guarded
} _PyFbCall;

/* One row per observation.  type_version[i] is the first version of the
 * observation and aux[i] the second: the operand types for BINARY, the
 * receiver type and the dict, function or class version for ATTR, the
 * function and callable type versions for CALL.  index[i], descr[i] and
 * shape[i] are the entry's index, descr and shape fields, handler[i] the
 * specialised opcode. */
typedef struct {
    uint32_t type_version[FB_POLY_SIZE];
    uint32_t aux[FB_POLY_SIZE];
    uint32_t index[FB_POLY_SIZE];
    uint16_t handler[FB_POLY_SIZE];
    const void *descr[FB_POLY_SIZE];    // borrowed, as in the entry
    _PyShape *shape[FB_POLY_SIZE];      // ATTR: the entry's shape, borrowed
} _PyFbPoly;

typedef struct _PyFbBlock {
    uint32_t size;          // bytes from the start of the block to the end of the last entry
    uint32_t npoly;         // poly tables in use; capacity is npoly rounded up to a power of 2
    _PyFbPoly *poly;
} _PyFbBlock;

/* Generic opcode, entry type, kind. */
#define _PyFb_FAMILIES(X) \
    X(RESUME,               _PyFbHeader, FB_COUNTER) \
    X(JUMP_BACKWARD,        _PyFbHeader, FB_COUNTER) \
    X(TO_BOOL,              _PyFbUnary,  FB_UNARY) \
    X(POP_JUMP_IF_FALSE,    _PyFbUnary,  FB_BRANCH) \
    X(POP_JUMP_IF_TRUE,     _PyFbUnary,  FB_BRANCH) \
    X(UNPACK_SEQUENCE,      _PyFbUnary,  FB_UNARY) \
    X(FOR_ITER,             _PyFbUnary,  FB_UNARY) \
    X(GET_ITER,             _PyFbUnary,  FB_UNARY) \
    X(SEND,                 _PyFbUnary,  FB_UNARY) \
    X(BINARY_OP,            _PyFbBinary, FB_BINARY) \
    X(COMPARE_OP,           _PyFbBinary, FB_BINARY) \
    X(CONTAINS_OP,          _PyFbBinary, FB_BINARY) \
    X(STORE_SUBSCR,         _PyFbBinary, FB_BINARY) \
    X(LOAD_ATTR,            _PyFbAttr,   FB_ATTR) \
    X(STORE_ATTR,           _PyFbAttr,   FB_ATTR) \
    X(LOAD_SUPER_ATTR,      _PyFbAttr,   FB_ATTR) \
    X(LOAD_GLOBAL,          _PyFbGlobal, FB_GLOBAL) \
    X(CALL,                 _PyFbCall,   FB_CALL) \
    X(CALL_KW,              _PyFbCall,   FB_CALL) \
    X(CALL_FUNCTION_EX,     _PyFbCall,   FB_CALL)

/* The branches that do not specialise, whose entries keep only the history. */
#define _PyFb_BRANCHES(X) \
    X(POP_JUMP_IF_NONE,     _PyFbHeader, FB_BRANCH) \
    X(POP_JUMP_IF_NOT_NONE, _PyFbHeader, FB_BRANCH)

// _PyFb_LOAD_ATTR and so on: the entry type of each family, used by the
// generated cases.
#define _PyFb_TYPEDEF(OP, TYPE, KIND) typedef TYPE _PyFb_##OP;
_PyFb_FAMILIES(_PyFb_TYPEDEF)
_PyFb_BRANCHES(_PyFb_TYPEDEF)
#undef _PyFb_TYPEDEF

// Whether instructions with the generic opcode OPCODE have an entry.
static inline int
_PyFb_HasEntry(int opcode)
{
    switch (opcode) {
#define _PyFb_CASE(OP, TYPE, KIND) case OP:
    _PyFb_FAMILIES(_PyFb_CASE)
#undef _PyFb_CASE
        return 1;
    default:
        return 0;
    }
}

/* Free the poly tables of a block whose bytecode copy is being freed. */
static inline void
_PyFb_Clear(_PyFbBlock *block)
{
    _PyMem_NoScanFree(block->poly);
    block->poly = NULL;
    block->npoly = 0;
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FEEDBACK_H */
