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
#include "pycore_value.h"       // _PyValueGuarded
#include "opcode_ids.h"         // RESUME, LOAD_ATTR, ...

/* Typed feedback.
 *
 * Every instruction of a family below, and every conditional branch, owns
 * one feedback entry unless the vector is full.  The assembler places the
 * entries in 8-byte units, entry 0 first, each ending by unit 4,096, and
 * stores in the IR word the unit at which its entry starts
 * (Include/internal/pycore_ir.h).  The code object keeps one 32-bit
 * initialiser per entry, in order of unit: its kind, its unit and the
 * operand that does not fit the word (_PyIR_FBINIT).  At the first call the executing copy is allocated with the entries after the
 * words:
 *
 *     | _PyCodeExec | words | _PyFbBlock | entries |
 *
 * Entry 0 is shared by every instruction that has no entry of its own: the
 * instructions past the limit of the vector and _Py_InitCleanup.  It never
 * specialises, because its counter never triggers.  dis reads the entries of the executing copy for
 * show_caches and adaptive.
 *
 * Each entry begins with a 16-byte header.  The entry type of each family,
 * and so its kind and size, is in FEEDBACK_LAYOUTS in
 * Tools/cases_generator/ir_generator.py, from which the instruction
 * contract's fb_kind and fb_size (Include/internal/pycore_ir.h) and the
 * _PyFb_<family> typedefs come.  Entry 0 is a header of kind COUNTER.
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
    union {
        uint16_t misses;            // reserved: the counter holds them, see above
        uint16_t taken;             // fused compare-and-jumps: the 16-bit taken
                                    // history of the jump, newest in bit 0
    };
    uint16_t hits;                  // reserved: call-site heat for the JIT stage
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

/* LOAD_ATTR, STORE_ATTR, LOAD_SUPER_ATTR.  The shape fields are used with
 * Py_SHAPES (Include/internal/pycore_dict.h). */
typedef struct {
    _PyFbHeader h;
    uint32_t type_version;          // Py_TYPE(owner)
    union {
        uint32_t dict_version;      // LOAD_ATTR_MODULE: keys version of the module dict
        uint32_t func_version;      // property getter or __getattribute__
        uint32_t class_version;     // owner, for loads from a class
        uint32_t shape;             // the owner's shape index
    };
    union {
        uint32_t index;
        uint32_t offset;
        uint32_t hint;
        uint32_t dictoffset;
    };
    uint32_t read_kind;             // checked indexed-read nomination, or zero
    union {                         // non-owning; valid while the versions match
        PyObject *descr;
        PyObject *fget;
        PyObject *getattribute;
        uint32_t shape_to;          // STORE_ATTR_SHAPE_ADD: the shape after the store
    };
    // A guarded word must not share storage with native pointer/shape fields.
    // Instance class values and methods use type_version; direct class values use
    // class_version and, when required, the metaclass type_version guard.
    _PyValueGuarded class_value;
} _PyFbAttr;

static_assert(sizeof(_PyFbAttr) == 48,
              "attribute feedback size changed");

#define _Py_FB_READ_SHAPE 1u

// LOAD_GLOBAL
typedef struct {
    _PyFbHeader h;
    uint32_t globals_version;       // keys versions of globals and builtins
    uint32_t builtins_version;
    uint32_t index;
    uint32_t _pad;
} _PyFbGlobal;

// CALL, CALL_KW, CALL_FUNCTION_EX
#define _Py_CALL_BIND_KEYWORD_CAPACITY 3

typedef struct {
    _PyFbHeader h;
    union {
        uint32_t func_version;
        uint32_t type_version;      // CALL_ALLOC_AND_ENTER_INIT: the class
    };
    uint32_t callable_type_version; // recorded but not guarded
    union {
        struct {
            uint16_t bound_count;
            uint16_t default_start;
            uint16_t kwargs_slot;
        } positional;
        uint16_t keyword_dest[_Py_CALL_BIND_KEYWORD_CAPACITY];
    } binding;
    uint16_t binding_flags;        // _Py_CALL_BIND_*; scalar-only guarded plan
} _PyFbCall;

static_assert(sizeof(_PyFbCall) == sizeof(_PyFbHeader) + 16,
              "keyword destinations reuse the positional binding payload");

#define _Py_CALL_BIND_DIRECT 1
#define _Py_CALL_BIND_VARARGS 2
#define _Py_CALL_BIND_VIRTUAL_KWARGS 4
#define _Py_CALL_BIND_KEYWORDS 8
#define _Py_CALL_BIND_KEYWORD_COUNT_SHIFT 4
#define _Py_CALL_BIND_KEYWORD_COUNT_MASK (3 << _Py_CALL_BIND_KEYWORD_COUNT_SHIFT)
#define _Py_CALL_BIND_SELF 64

/* One row per observation.  type_version[i] is the first version of the
 * observation and aux[i] the second: the operand types for BINARY, the
 * receiver type and the dict, function or class version for ATTR, the
 * function and callable type versions for CALL. index[i] and payload[i] hold
 * the entry's index and non-owning payload, interpreted by handler[i]. */
typedef struct {
    union {
        const void *descr;
        uint32_t shape_to;
        uint32_t read_kind;         // scalar SHAPE-read capability, never a pointer
    };
    _PyValueGuarded class_value;
} _PyFbPayload;

static_assert(sizeof(_PyFbPayload) == 16, "feedback payload size changed");

typedef struct {
    uint32_t type_version[FB_POLY_SIZE];
    uint32_t aux[FB_POLY_SIZE];
    uint32_t index[FB_POLY_SIZE];
    uint16_t handler[FB_POLY_SIZE];
    _PyFbPayload payload[FB_POLY_SIZE];
} _PyFbPoly;

static_assert(sizeof(_PyFbPoly) == 120, "polymorphic feedback size changed");

typedef struct _PyFbBlock {
    uint32_t size;          // bytes from the start of the block to the end of the last entry
    uint32_t npoly;         // poly tables in use; capacity is npoly rounded up to a power of 2
    _PyFbPoly *poly;
} _PyFbBlock;

/* The entry type and kind of each family (_PyFb_LOAD_ATTR and so on) are
 * in the instruction contract: Include/internal/pycore_ir_metadata.h, from
 * FEEDBACK_LAYOUTS in Tools/cases_generator/ir_generator.py. */

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
