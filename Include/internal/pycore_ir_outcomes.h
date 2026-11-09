#ifndef Py_INTERNAL_IR_OUTCOMES_H
#define Py_INTERNAL_IR_OUTCOMES_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"

/* Cold consumers must distinguish an edge's definitions from the frontend
 * stack shape. A definition or a retired NULL home needs no incoming owner.
 * Ordinary handlers implement these fixed outcomes directly. */
typedef struct {
    int semantic;
    int first;
    int count;
    int dst;
} _PyIRSlotOutcome;

/* Inputs whose ABI accepts NULL: window-relative positions for WINDOW rows,
 * semantic IN0+k positions for register rows. This is a read capability,
 * not a definition or retirement: cold consumers must independently prove
 * the home is NULL. CALL_EX still requires its callable and argument tuple. */
static inline uint32_t
_PyIR_NullableInputs(_PyIRWord word)
{
    /* Deletion is a capability of these actual handlers, not of every
     * STORE_ATTR specialization. Static deletion sites never specialize. */
    if (IR_HANDLER(word) == STORE_ATTR) {
        return 1u;
    }
    switch (_PyIR_BaseOpcode(IR_HANDLER(word))) {
        case CALL:
        case CALL_KW:
            return 1u << 1;  /* Optional method receiver. */
        case CALL_FUNCTION_EX:
            return (1u << 1) | (1u << 3);  /* NULL self, optional kwargs. */
        case FOR_ITER:
        case SEND:
        case POP_ITER:
        case END_SEND:
            return 1u << 1;  /* Optional virtual-iterator cursor. */
        case CLEANUP_THROW:
            /* VM child entry has already taken the last sent value. */
            return (1u << 1) | (1u << 2);
        case WITH_EXCEPT_START:
            return 1u << 1;  /* Optional bound exit-method receiver. */
        case STORE_GLOBAL:
            return 1u << 0;  /* NULL value requests deletion, never a receiver. */
        default:
            return 0;
    }
}

static inline _PyIRSlotOutcome
_PyIR_Outcome(_PyIRWord word, int taken)
{
    int base = (int)IR_B(word);
    int dst = (int)IR_A(word);
    switch (_PyIR_BaseOpcode(IR_HANDLER(word))) {
        case FOR_ITER:
            return taken ? (_PyIRSlotOutcome){1, base, 2, IR_NO_DST}
                         : (_PyIRSlotOutcome){1, base + 1, 1, dst};
        case SEND:
            return taken ? (_PyIRSlotOutcome){1, base, 3, dst}
                         : (_PyIRSlotOutcome){1, base + 2, 1, IR_NO_DST};
        case CLEANUP_THROW:
            return (_PyIRSlotOutcome){1, base, 4, dst};
        case YIELD_VALUE:
            /* The outgoing value is taken; the next activation supplies A. */
            return (_PyIRSlotOutcome){1, base, 1, dst};
        case RETURN_GENERATOR:
            return (_PyIRSlotOutcome){1, 0, 0, IR_NO_DST};
        default:
            return (_PyIRSlotOutcome){0, 0, 0, IR_NO_DST};
    }
}

/* Physical input windows include untouched stack holes. They preserve owners
 * but do not read them. Keep explicit input fields independent of this audit:
 * collection updates can encode their trailing values outside the window.
 * Opaque call/activation adapters retain the conservative full-window rule. */
static inline int
_PyIR_ReadsWindowHome(_PyIRWord word, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int window = -1;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_BASE) {
            window = (int)fields[f];
        }
    }
    int relative = home - window;
    if (window < 0 || relative < 0 || relative >= _PyIR_Extent(fmt->reads, arg)) {
        return 0;
    }
    switch (_PyIR_BaseOpcode(IR_HANDLER(word))) {
        case LIST_APPEND:
        case SET_ADD:
        case MAP_ADD:
            return relative == 0;
        case LIST_EXTEND:
        case SET_UPDATE:
        case DICT_UPDATE:
            return relative == 0 || relative == arg;
        case DICT_MERGE:
            return relative == 0 || relative == 3 || relative == arg + 3;
        case COPY:
            return relative == 0;
        case SWAP:
            return relative == 0 || relative == arg - 1;
        case WITH_EXCEPT_START:
            return relative != 3;
        default:
            return 1;
    }
}

/* A physical home can alias several semantic inputs. Every actual read,
 * including an independent window use, must accept NULL. */
static inline int
_PyIR_NullableHome(_PyIRWord word, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t inputs = _PyIR_NullableInputs(word);
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int seen = 0;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role >= _PyIR_ROLE_IN0 && (int)fields[f] == home) {
            int input = role - _PyIR_ROLE_IN0;
            if (input >= 32 || !(inputs & (1u << input))) {
                return 0;
            }
            seen = 1;
        }
        if (role == _PyIR_ROLE_BASE && _PyIR_ReadsWindowHome(word, arg, home)) {
            int relative = home - (int)fields[f];
            if (relative < 0 || relative >= 32 ||
                !(inputs & (1u << relative)))
            {
                return 0;
            }
            seen = 1;
        }
    }
    return seen;
}

static inline int
_PyIR_ReadsHome(_PyIRWord word, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] >= _PyIR_ROLE_IN0 && (int)fields[f] == home) {
            return 1;
        }
    }
    return _PyIR_ReadsWindowHome(word, arg, home);
}

/* Ordinary fresh definitions use the emitter's physical output mapping.
 * Edge-specific semantic outcomes take precedence in callers. Permutations
 * preserve owner provenance; they cannot reconstruct an unavailable input. */
static inline int
_PyIR_DefinesHome(_PyIRWord word, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (_PyIR_Outcome(word, 0).semantic ||
        _PyIR_BaseOpcode(IR_HANDLER(word)) == SWAP)
    {
        return 0;
    }
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int dst = -1, window = -1;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_DST) {
            dst = (int)fields[f];
        }
        if (fmt->roles[f] == _PyIR_ROLE_BASE) {
            window = (int)fields[f];
        }
    }
    int through = _PyIR_Extent(fmt->through, arg);
    int writes = _PyIR_Extent(fmt->writes, arg);
    if (fmt->flags & _PyIR_UNPACKS) {
        int first = dst == IR_NO_DST ? window + through : dst;
        return home >= first && home - first < writes - through;
    }
    switch (fmt->output_kind) {
        case _PyIR_OUTPUT_REGISTER_DST:
            return dst >= 0 && dst != IR_NO_DST && home >= dst &&
                   home - dst < writes - through;
        case _PyIR_OUTPUT_WINDOW:
            return window >= 0 && home >= window + through && home - window < writes;
        case _PyIR_OUTPUT_WINDOW_DST_FIRST:
            return (dst >= 0 && dst != IR_NO_DST && home == dst) ||
                   (window >= 0 && home >= window + through && home - window < writes &&
                    home - window != fmt->dst);
        default:
            return 0;
    }
}

static inline int
_PyIR_CompletionWindow(int opcode)
{
    switch (opcode) {
        case FOR_ITER: return 2;
        case SEND: return 3;
        case CLEANUP_THROW: return 4;
        case YIELD_VALUE: return 1;
        default: return 0;
    }
}
#endif
