#ifndef Py_INTERNAL_FEEDBACK_H
#define Py_INTERNAL_FEEDBACK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_backoff.h"     // _Py_BackoffCounter
#include "pycore_structs.h"     // _Py_CODEUNIT
#include "opcode_ids.h"         // RESUME, LOAD_ATTR, ...

/* Typed feedback.
 *
 * Every instruction with inline cache units except the conditional branches
 * owns one feedback entry.  The entries of a copy of the bytecode live in one
 * block that follows the bytecode in the same allocation, at the next 8-byte
 * boundary:
 *
 *     | code units, inline caches included | pad | _PyFbBlock | entries |
 *
 * The main copy is co_code_adaptive, and PyCodeObject._co_feedback points at
 * its block.  Each thread-local copy of the free-threaded build carries its
 * own block in the same layout.
 *
 * Unit 0 of an instruction's inline cache holds the distance in code units
 * from the instruction to its entry, so a handler finds its entry with one
 * load and no base register (_PyFb_Entry).  Because the block sits at the
 * same offset in every copy, unit 0 is the same in every copy.  0 means no
 * entry: the static code object _Py_InitCleanup, and instructions more than
 * 65535 code units from their entry, which only happens in code objects of
 * more than about 64K code units.  Such instructions never specialise.  The
 * other cache units are reserved and stay zero, so co_code, dis offsets and
 * pyc files keep their layout; code_getcodeadaptive() rebuilds the old cache
 * contents from the entries for _co_code_adaptive and dis.
 *
 * Each entry begins with a 16-byte header.  Kind, size and opcodes:
 *
 *     COUNTER  16  RESUME, JUMP_BACKWARD
 *     BRANCH    -  POP_JUMP_IF_FALSE, _TRUE, _NONE, _NOT_NONE
 *     UNARY    24  TO_BOOL, UNPACK_SEQUENCE, FOR_ITER, GET_ITER, SEND
 *     BINARY   32  BINARY_OP, COMPARE_OP, CONTAINS_OP, STORE_SUBSCR
 *     ATTR     40  LOAD_ATTR, STORE_ATTR, LOAD_SUPER_ATTR
 *     GLOBAL   32  LOAD_GLOBAL
 *     CALL     24  CALL, CALL_KW, CALL_FUNCTION_EX
 *
 * A branch keeps its 16-bit taken history, newest in bit 0, in its single
 * cache unit, as before.  Branches are the only instructions whose fast path
 * writes feedback on every execution, and reaching an entry there costs five
 * instructions per branch, about 2 percent on richards.  The IR block of
 * design note 01 gives BRANCH a 16-byte entry with the history in arg.
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

/* LOAD_ATTR, STORE_ATTR, LOAD_SUPER_ATTR.  Design note 04 (shapes) turns
 * _pad into the shape offset and appends a shape pointer, for 48 bytes. */
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
        PyObject *descr;
        PyObject *fget;
        PyObject *getattribute;
    };
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
 * function and callable type versions for CALL.  index[i] and descr[i] are
 * the entry's index and descr fields, handler[i] the specialised opcode. */
typedef struct {
    uint32_t type_version[FB_POLY_SIZE];
    uint32_t aux[FB_POLY_SIZE];
    uint32_t index[FB_POLY_SIZE];
    uint16_t handler[FB_POLY_SIZE];
    const void *descr[FB_POLY_SIZE];    // borrowed, as in the entry
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

/* The conditional branches, which have entries in the register IR only. */
#define _PyFb_BRANCHES(X) \
    X(POP_JUMP_IF_FALSE,    _PyFbHeader, FB_BRANCH) \
    X(POP_JUMP_IF_TRUE,     _PyFbHeader, FB_BRANCH) \
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

/* Alignment of the block and of every entry.  Bytecode copies start at
 * least this aligned, so the block has the same offset in all of them. */
#define FB_ALIGN 8

/* Offset in bytes of the block from the start of a copy of the bytecode
 * with SIZE code units. */
static inline Py_ssize_t
_PyFb_BlockOffset(Py_ssize_t size)
{
    return (Py_ssize_t)_Py_SIZE_ROUND_UP(size * (Py_ssize_t)sizeof(_Py_CODEUNIT),
                                         FB_ALIGN);
}

static inline _PyFbBlock *
_PyFb_Block(_Py_CODEUNIT *bytecode, Py_ssize_t size)
{
    return (_PyFbBlock *)((char *)bytecode + _PyFb_BlockOffset(size));
}

/* The entry of an instruction that has one. */
static inline void *
_PyFb_Entry(const _Py_CODEUNIT *instr)
{
    assert(instr[1].cache != 0);
    return (void *)(instr + instr[1].cache);
}

/* The entry header of an instruction of a family with entries, or NULL if
 * the instruction has none. */
static inline _PyFbHeader *
_PyFb_Header(const _Py_CODEUNIT *instr)
{
    uint16_t distance = instr[1].cache;
    if (distance == 0) {
        return NULL;
    }
    _PyFbHeader *h = (_PyFbHeader *)(instr + distance);
    if (h == NULL) {
        // Tells the compiler that callers need no second NULL test
        Py_UNREACHABLE();
    }
    return h;
}

/* Free the poly tables of a block whose bytecode copy is being freed. */
static inline void
_PyFb_Clear(_PyFbBlock *block)
{
    PyMem_Free(block->poly);
    block->poly = NULL;
    block->npoly = 0;
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FEEDBACK_H */
