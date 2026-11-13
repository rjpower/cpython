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

enum {
    _PyIR_SEMANTIC_UNKNOWN,
    _PyIR_SEMANTIC_PRESERVE,
    _PyIR_SEMANTIC_RELEASE,
    _PyIR_SEMANTIC_FRESH,
    _PyIR_SEMANTIC_NULL,
    _PyIR_SEMANTIC_DUPLICATE,
    _PyIR_SEMANTIC_MOVE,
};

typedef struct {
    int kind;
    int source;
} _PyIRSemanticTransfer;

/* Logical value/owner transport, independent of the physical NULL proof below.
 * This cold query is only authoritative for positively checked normal rows;
 * it says nothing about their intervening callbacks or exceptional exits. */
static inline _PyIRSemanticTransfer
_PyIR_NormalSemanticTransfer(_PyIRWord word, int arg, int home, int nl)
{
    const _PyIRSemanticTransfer preserve = {_PyIR_SEMANTIC_PRESERVE, -1};
    const _PyIRSemanticTransfer release = {_PyIR_SEMANTIC_RELEASE, -1};
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int dst = -1, input = -1, base = -1;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_DST) {
            dst = (int)fields[f];
        }
        else if (fmt->roles[f] == _PyIR_ROLE_BASE) {
            base = (int)fields[f];
        }
        else if (fmt->roles[f] == _PyIR_ROLE_IN0) {
            input = (int)fields[f];
        }
    }
    switch (fmt->transfer_kind) {
        case _PyIR_TRANSFER_FRESH:
            return home == dst && dst != IR_NO_DST
                ? (_PyIRSemanticTransfer){_PyIR_SEMANTIC_FRESH, -1} : preserve;
        case _PyIR_TRANSFER_LOCAL_DUP:
        case _PyIR_TRANSFER_LOCAL_MOVE:
            if (home == dst && dst != IR_NO_DST) {
                return (_PyIRSemanticTransfer){
                    fmt->transfer_kind == _PyIR_TRANSFER_LOCAL_DUP
                        ? _PyIR_SEMANTIC_DUPLICATE : _PyIR_SEMANTIC_MOVE, arg};
            }
            if (home == arg && fmt->transfer_kind == _PyIR_TRANSFER_LOCAL_MOVE) {
                return (_PyIRSemanticTransfer){_PyIR_SEMANTIC_NULL, -1};
            }
            return preserve;
        case _PyIR_TRANSFER_LOCAL_STORE:
            if (home == arg) {
                return (_PyIRSemanticTransfer){_PyIR_SEMANTIC_MOVE, input};
            }
            return home == input && input >= nl ? release : preserve;
        case _PyIR_TRANSFER_LOCAL_CLEAR:
            return home == arg
                ? (_PyIRSemanticTransfer){_PyIR_SEMANTIC_NULL, -1} : preserve;
        case _PyIR_TRANSFER_RETIRE:
        case _PyIR_TRANSFER_EXIT:
            return home == input && input >= nl ? release : preserve;
        case _PyIR_TRANSFER_COPY:
            return home == base + arg
                ? (_PyIRSemanticTransfer){_PyIR_SEMANTIC_DUPLICATE, base} : preserve;
        case _PyIR_TRANSFER_SWAP:
            if (home == base || home == base + arg - 1) {
                return (_PyIRSemanticTransfer){_PyIR_SEMANTIC_MOVE,
                    home == base ? base + arg - 1 : base};
            }
            return preserve;
        case _PyIR_TRANSFER_PRESERVE:
            return preserve;
        default:
            return (_PyIRSemanticTransfer){_PyIR_SEMANTIC_UNKNOWN, -1};
    }
}

static inline _PyIRSlotOutcome _PyIR_Outcome(_PyIRWord word, int taken);
static inline int _PyIR_DefinesHome(_PyIRWord word, int arg, int home);

/* Constructor words are pristine; monitoring runs their twin. Resolve the
 * single generated provider fact from that twin, or from a direct no-twin
 * helper such as SEND. This is a cold declaration, not a success inference. */
static inline int
_PyIR_ObserverOutcome(_PyIRWord word, int edge)
{
    if (edge < 0 || edge > 1) {
        return _PyIR_OBSERVER_NONE;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->twin != 0) {
        fmt = _PyIR_FormatOf(fmt->twin);
    }
    return fmt->observer_outcome[edge];
}

enum {
    _PyIR_OBSERVED_UNKNOWN,
    _PyIR_OBSERVED_MISSING,
    _PyIR_OBSERVED_VALUE,
    _PyIR_OBSERVED_NULL,
};

typedef struct {
    int home[8];
    uint8_t physical[8];
    uint8_t old_cut[8];
    int count;
} _PyIRObservedHomes;

static inline int
_PyIR_ObserveHome(_PyIRObservedHomes *observed, int home, int nl, int limit,
                  int physical, int old_cut)
{
    if (home < nl) {
        return 1;  /* Named-local restoration has its own owner proof. */
    }
    if (home >= limit) {
        return 0;
    }
    int k;
    for (k = 0; k < observed->count; k++) {
        if (observed->home[k] == home) {
            break;
        }
    }
    if (k == observed->count) {
        if (k == 8) {
            return 0;
        }
        observed->home[k] = home;
        observed->physical[k] = _PyIR_OBSERVED_UNKNOWN;
        observed->old_cut[k] = 0;
        observed->count++;
    }
    observed->physical[k] = physical;
    observed->old_cut[k] |= old_cut;
    return 1;
}

/* Decode only a positively declared, committed observer edge. This records
 * physical publication separately from an old input's retired provenance:
 * storing a replacement into the same home never revives the old owner. */
static inline int
_PyIR_ObservedHomes(_PyIRWord word, int edge, int nl, int limit,
                    _PyIRObservedHomes *observed)
{
    int kind = _PyIR_ObserverOutcome(word, edge);
    if (kind == _PyIR_OBSERVER_NONE || nl < 0 || limit < nl) {
        return 0;
    }
    *observed = (_PyIRObservedHomes){0};
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int base = -1, dst = -1;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_BASE) {
            base = (int)fields[f];
        }
        else if (role == _PyIR_ROLE_DST) {
            dst = (int)fields[f];
        }
        else if (role >= _PyIR_ROLE_IN0 &&
                 !_PyIR_ObserveHome(observed, (int)fields[f], nl, limit,
                                    _PyIR_OBSERVED_MISSING, 1))
        {
            return 0;
        }
    }
    int arg = fmt->imm == _PyIR_LOC_FBARG ? IR_MAX_SLOTS
                                              : _PyIR_FieldOparg(word);
    int reads = _PyIR_Extent(fmt->reads, arg);
    int writes = _PyIR_Extent(fmt->writes, arg);
    int through = _PyIR_Extent(fmt->through, arg);
    if (reads < 0 || writes < 0 || through < 0 || through > reads ||
        writes < through || reads > 8 || writes > 8)
    {
        return 0;
    }
    if (base >= 0) {
        if (base < nl || base > limit - Py_MAX(reads, writes)) {
            return 0;
        }
        for (int k = 0; k < reads; k++) {
            if (!_PyIR_ObserveHome(observed, base + k, nl, limit,
                                   k < through ? _PyIR_OBSERVED_UNKNOWN
                                               : _PyIR_OBSERVED_MISSING,
                                   k >= through))
            {
                return 0;
            }
        }
    }
    if (kind == _PyIR_OBSERVER_FOR_BODY) {
        if (base < 0 || reads != 2 || writes != 3 || fmt->dst != 2 ||
            (dst >= nl && dst != IR_NO_DST && dst != base + 2))
        {
            return 0;
        }
        if (!_PyIR_ObserveHome(observed, base + 1, nl, limit,
                               _PyIR_OBSERVED_UNKNOWN, 1) ||
            !_PyIR_ObserveHome(observed, base + 2, nl, limit,
                dst == IR_NO_DST || dst < nl ? _PyIR_OBSERVED_NULL
                                            : _PyIR_OBSERVED_VALUE, 0))
        {
            return 0;
        }
    }
    else if (kind == _PyIR_OBSERVER_COMPLETION) {
        _PyIRSlotOutcome outcome = _PyIR_Outcome(word, edge);
        if (!outcome.semantic || outcome.first < nl || outcome.count > 8 ||
            outcome.first > limit - outcome.count)
        {
            return 0;
        }
        for (int k = 0; k < outcome.count; k++) {
            if (!_PyIR_ObserveHome(observed, outcome.first + k, nl, limit,
                                   _PyIR_OBSERVED_MISSING, 1))
            {
                return 0;
            }
        }
        if (outcome.dst != IR_NO_DST &&
            !_PyIR_ObserveHome(observed, outcome.dst, nl, limit,
                               _PyIR_OBSERVED_VALUE, 0))
        {
            return 0;
        }
    }
    else if (kind == _PyIR_OBSERVER_FORMAT_VALUE ||
             kind == _PyIR_OBSERVER_FORMAT_NULL_FIRST)
    {
        int first_null = kind == _PyIR_OBSERVER_FORMAT_NULL_FIRST;
        if (fmt->output_kind == _PyIR_OUTPUT_REGISTER_DST) {
            if (writes - through > 0 && dst < 0) {
                return 0;
            }
            for (int k = 0; k < writes - through; k++) {
                if (dst == IR_NO_DST) {
                    break;
                }
                if (!_PyIR_ObserveHome(observed, dst + k, nl, limit,
                        first_null && k == 0 ? _PyIR_OBSERVED_NULL
                                             : _PyIR_OBSERVED_VALUE, 0))
                {
                    return 0;
                }
            }
        }
        else if (fmt->output_kind == _PyIR_OUTPUT_WINDOW ||
                 fmt->output_kind == _PyIR_OUTPUT_WINDOW_DST_FIRST)
        {
            if (base < 0 ||
                (fmt->output_kind == _PyIR_OUTPUT_WINDOW_DST_FIRST &&
                 (fmt->dst < through || fmt->dst >= writes ||
                  (dst >= nl && dst != IR_NO_DST && dst != base + fmt->dst))))
            {
                return 0;
            }
            for (int k = through; k < writes; k++) {
                int moved = fmt->output_kind == _PyIR_OUTPUT_WINDOW_DST_FIRST &&
                            k == fmt->dst && (dst == IR_NO_DST || dst < nl);
                if (!_PyIR_ObserveHome(observed, base + k, nl, limit,
                        moved || (first_null && k == fmt->dst)
                            ? _PyIR_OBSERVED_NULL : _PyIR_OBSERVED_VALUE, 0))
                {
                    return 0;
                }
            }
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }
    return 1;
}

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
    switch (_PyIR_HandlerBase[IR_HANDLER(word)]) {
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
    if (_PyIR_CallInputRole(fmt, relative) == _PyIR_CALL_INPUT_VIRTUAL_NULL) {
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

/* The successful final stack commit consumes inputs outside its unchanged
 * window prefix. Through homes retain their logical values even when they
 * are read; fresh outputs are handled independently by the caller. */
static inline int
_PyIR_ConsumesNormalInput(_PyIRWord word, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int through = _PyIR_Extent(fmt->through, arg);
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_BASE &&
            home >= (int)fields[f] && home - (int)fields[f] < through)
        {
            return 0;
        }
    }
    return _PyIR_ReadsHome(word, arg, home);
}

enum {
    _PyIR_BINDING_NO_READ,
    _PyIR_BINDING_CHECKED_NULL,
    _PyIR_BINDING_REQUIRES_VALUE,
};

/* A saved NULL fast-local can reach a checked read, but not an unchecked
 * local borrow or any other physical alias that requires a value. */
static inline int
_PyIR_BindingRead(_PyIRWord word, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
    int result = _PyIR_BINDING_NO_READ;
    for (int f = 0; f < 4; f++) {
        if ((int)fields[f] != home) {
            continue;
        }
        switch (fmt->roles[f]) {
            case _PyIR_ROLE_LOCAL_IN:
                if (opcode == LOAD_FAST_CHECK) {
                    result = Py_MAX(result, _PyIR_BINDING_CHECKED_NULL);
                }
                else {
                    return _PyIR_BINDING_REQUIRES_VALUE;
                }
                break;
            case _PyIR_ROLE_LOCAL_CLEAR:
                if (opcode == DELETE_FAST) {
                    result = Py_MAX(result, _PyIR_BINDING_CHECKED_NULL);
                }
                else {
                    return _PyIR_BINDING_REQUIRES_VALUE;
                }
                break;
            case _PyIR_ROLE_LOCAL_MOVE:
            case _PyIR_ROLE_CELL:
                return _PyIR_BINDING_REQUIRES_VALUE;
        }
    }
    if (_PyIR_ReadsHome(word, arg, home)) {
        return _PyIR_BINDING_REQUIRES_VALUE;
    }
    return result;
}

/* A physical write need not bind a value: STORE_FAST can receive a saved
 * NULL, and DELETE_FAST deliberately clears its local. Only a checked
 * committed output that guarantees a value can discharge this obligation. */
static inline int
_PyIR_DefinesBinding(_PyIRWord word, int arg, int edge, int home)
{
    (void)arg;
    int kind = _PyIR_ObserverOutcome(word, edge);
    if ((kind == _PyIR_OBSERVER_FOR_BODY ||
         kind == _PyIR_OBSERVER_COMPLETION) &&
        _PyIR_Outcome(word, edge).dst == home)
    {
        return 1;
    }
    return 0;
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

enum {
    _PyIR_PHYSICAL_PRESERVE,
    _PyIR_PHYSICAL_SET_NULL,
    _PyIR_PHYSICAL_UNKNOWN,
    _PyIR_PHYSICAL_TRANSPORT,
};

typedef struct {
    int kind;
    int source;
} _PyIRPhysicalTransfer;

/* A normal successor's physical scratch-home update. TRANSPORT reads the
 * incoming row: callers must calculate all destinations before changing it.
 * This does not prove that a source is a valid executable input owner. */
static inline _PyIRPhysicalTransfer
_PyIR_NormalPhysicalTransfer(_PyIRWord word, int arg, int edge, int home,
                             int nl, int limit)
{
    const _PyIRPhysicalTransfer unknown = {_PyIR_PHYSICAL_UNKNOWN, -1};
    const _PyIRPhysicalTransfer preserve = {_PyIR_PHYSICAL_PRESERVE, -1};
    int handler = IR_HANDLER(word);
    if (handler >= IR_NUM_HANDLERS || nl < 0 || limit > IR_MAX_SLOTS ||
        limit < nl || home < nl || home >= limit || edge < 0 || edge > 1)
    {
        return unknown;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (!(fmt->flags & _PyIR_VALID) ||
        (edge && !(fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD))))
    {
        return unknown;
    }
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int base = -1;
    int dst = -1;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_BASE) {
            base = (int)fields[f];
        }
        else if (role == _PyIR_ROLE_DST) {
            dst = (int)fields[f];
            if (dst != IR_NO_DST && dst >= limit) {
                return unknown;
            }
            if (dst == IR_NO_DST && !(fmt->flags & _PyIR_DISCARDS)) {
                return unknown;
            }
        }
        else if (role >= _PyIR_ROLE_IN0 && fields[f] >= (uint32_t)limit) {
            return unknown;
        }
        else if (role >= _PyIR_ROLE_LOCAL_IN &&
                 role <= _PyIR_ROLE_CELL && fields[f] >= (uint32_t)nl)
        {
            return unknown;
        }
    }
    /* Extent arithmetic uses int; reject malformed arguments before it. */
    if (arg < -IR_FIELD_MAX || arg > IR_CD_MAX) {
        return unknown;
    }
    int reads = _PyIR_Extent(fmt->reads, arg);
    int writes = _PyIR_Extent(fmt->writes, arg);
    int through = _PyIR_Extent(fmt->through, arg);
    if (reads < 0 || writes < 0 || through < 0 || through > reads ||
        writes < through ||
        (base >= 0 && (base < nl || base > limit - Py_MAX(reads, writes))))
    {
        return unknown;
    }

    switch (_PyIR_BaseOpcode(handler)) {
        case POP_TOP:
            /* The emitted scalar retirement detaches scratch A before
             * closing it. A normal continuation keeps other scratch homes. */
            if (edge || handler != POP_TOP ||
                fmt->roles[0] != _PyIR_ROLE_IN0 ||
                fmt->roles[1] != _PyIR_ROLE_NONE ||
                fmt->roles[2] != _PyIR_ROLE_NONE ||
                fmt->roles[3] != _PyIR_ROLE_NONE ||
                (fmt->flags & (_PyIR_WINDOW | _PyIR_JUMPS | _PyIR_FORWARD)) ||
                (fmt->effects & _PyIR_PUSHES_FRAME) ||
                fmt->output_kind != _PyIR_OUTPUT_REGISTER_DST ||
                reads != 1 || writes != 0 || through != 0 ||
                IR_A(word) < (uint32_t)nl ||
                _PyIR_Outcome(word, edge).semantic)
            {
                return unknown;
            }
            if (home == (int)IR_A(word)) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_SET_NULL, -1};
            }
            return preserve;
        case CALL:
            /* Every successful vector-CALL route drains its input window:
             * native retirement clears it, and a Python child consumes the
             * moved arguments before publishing the parent's recipient.
             * Direct specializations leave a temporary result at BASE. */
            if (edge || fmt->roles[0] != _PyIR_ROLE_DST ||
                fmt->roles[1] != _PyIR_ROLE_BASE ||
                fmt->roles[2] != _PyIR_ROLE_IMM ||
                fmt->roles[3] != _PyIR_ROLE_FB ||
                arg < 0 || arg != (int)IR_C(word) ||
                reads != arg + 2 || writes > 1 || through != 0 ||
                fmt->dst != 0 || base < 0 ||
                fmt->output_kind != _PyIR_OUTPUT_WINDOW_DST_FIRST ||
                (dst >= nl && dst != IR_NO_DST && dst != base))
            {
                return unknown;
            }
            if (home >= base && home - base < reads && home != dst) {
                if (_PyIR_CallInputRole(fmt, home - base) ==
                    _PyIR_CALL_INPUT_VIRTUAL_NULL)
                {
                    return preserve;
                }
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_SET_NULL, -1};
            }
            /* Neither a new result nor an overlapping child's unused tail
             * supplies a must-NULL fact. */
            return unknown;
        case COPY:
            if (edge || fmt->roles[0] != _PyIR_ROLE_BASE ||
                fmt->roles[1] != _PyIR_ROLE_IMM || arg != (int)IR_B(word) ||
                arg < 1 || reads != arg || writes != arg + 1 ||
                through != arg || base < 0)
            {
                return unknown;
            }
            if (home == base + arg) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT, base};
            }
            return preserve;
        case SWAP:
            if (edge || fmt->roles[0] != _PyIR_ROLE_BASE ||
                fmt->roles[1] != _PyIR_ROLE_IMM || arg != (int)IR_B(word) ||
                arg < 2 || reads != arg || writes != arg || base < 0)
            {
                return unknown;
            }
            if (home == base) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT,
                                               base + arg - 1};
            }
            if (home == base + arg - 1) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT, base};
            }
            return preserve;
        case STORE_FAST:
            if (edge || fmt->roles[0] != _PyIR_ROLE_IN0 ||
                fmt->roles[1] != _PyIR_ROLE_LOCAL_OUT ||
                arg != (int)IR_B(word) || IR_B(word) >= (uint32_t)nl ||
                reads != 1 || writes != 0)
            {
                return unknown;
            }
            if (home == (int)IR_A(word)) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_SET_NULL, -1};
            }
            return preserve;
        case YIELD_VALUE:
            if (edge || fmt->roles[0] != _PyIR_ROLE_DST ||
                fmt->roles[1] != _PyIR_ROLE_BASE || reads != 1 || writes != 1 ||
                through != 0 || base < 0)
            {
                return unknown;
            }
            if (home == dst) {
                return unknown;
            }
            if (home == base) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_SET_NULL, -1};
            }
            return preserve;
        case FOR_ITER:
            if (fmt->roles[0] != _PyIR_ROLE_DST ||
                fmt->roles[1] != _PyIR_ROLE_BASE || base < 0 ||
                reads != 2 || fmt->dst != 2 ||
                !((writes == 3 && through <= 2) ||
                  (writes == 2 && through == 2 &&
                   (fmt->effects & _PyIR_PUSHES_FRAME))) ||
                (dst >= nl && dst != IR_NO_DST && dst != base + 2))
            {
                return unknown;
            }
            if (edge == 1 && (home == base || home == base + 1)) {
                if (_PyIR_ObserverOutcome(word, edge) !=
                    _PyIR_OBSERVER_COMPLETION)
                {
                    return unknown;
                }
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_SET_NULL, -1};
            }
            /* A pristine FOR word can run the child-entering GEN variant.
             * Only the completed receiver/cursor pair has a shared fact. */
            return unknown;
    }

    if (fmt->effects & _PyIR_PUSHES_FRAME) {
        /* A child activation may reuse scratch homes beyond this row's
         * declared input and output window. */
        return unknown;
    }

    /* In every other format, a read outside the unchanged prefix or a
     * published result may replace the old physical value. An untouched
     * home, including LOAD_GLOBAL's unrequested optional-self slot, survives. */
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role >= _PyIR_ROLE_IN0 && (int)fields[f] == home) {
            return unknown;
        }
        if (role >= _PyIR_ROLE_LOCAL_IN && role <= _PyIR_ROLE_CELL &&
            (int)fields[f] == home)
        {
            return unknown;
        }
    }
    if (base >= 0 && home >= base + through &&
        home - base < Py_MAX(reads, writes))
    {
        return unknown;
    }
    _PyIRSlotOutcome outcome = _PyIR_Outcome(word, edge);
    if (outcome.semantic &&
        ((home >= outcome.first && home - outcome.first < outcome.count) ||
         home == outcome.dst))
    {
        return unknown;
    }
    if (_PyIR_DefinesHome(word, arg, home)) {
        return unknown;
    }
    return preserve;
}

enum {
    _PyIR_ABSENCE_UNKNOWN,
    _PyIR_ABSENCE_PRESERVE,
    _PyIR_ABSENCE_SET_EMPTY,
    _PyIR_ABSENCE_SET_VALUE,
};

/* Successful CALL cleanup is a logical contract. A returned child can leave
 * nonowning header bytes in the arena; they are not physical NULL facts.
 * Native calls touch only their own window, so outside it this can preserve
 * a prior absence fact but cannot create one. */
static inline int
_PyIR_NormalCallAbsence(_PyIRWord word, int arg, int home, int nl, int limit)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (home < nl || home >= limit) {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
    int extra = opcode == CALL ? 2 : opcode == CALL_KW ? 3 :
        opcode == CALL_FUNCTION_EX ? 4 : -1;
    int base = (int)IR_B(word), dst = (int)IR_A(word);
    if (extra < 0) {
        /* Reached suspension/child outcomes leave their new recipient live.
         * Cursor/receiver updates have their own protocols, so the whole
         * affected window loses absence; no completion slot is guessed empty.
         * A prior empty tail stays owner-free after a completed child cleanup. */
        if (opcode != YIELD_VALUE && opcode != SEND && opcode != FOR_ITER &&
            opcode != CLEANUP_THROW)
        {
            return _PyIR_ABSENCE_UNKNOWN;
        }
        int reads = _PyIR_Extent(fmt->reads, arg);
        int writes = _PyIR_Extent(fmt->writes, arg);
        if (base < nl || reads < 0 || writes < 0 ||
            base > limit - Py_MAX(reads, writes) ||
            fmt->roles[0] != _PyIR_ROLE_DST || fmt->roles[1] != _PyIR_ROLE_BASE)
        {
            return _PyIR_ABSENCE_UNKNOWN;
        }
        if (home == dst) {
            return _PyIR_ABSENCE_SET_VALUE;
        }
        if (home >= base && home - base < Py_MAX(reads, writes)) {
            return opcode == YIELD_VALUE && reads == 1 && writes == 1 && home == base
                ? _PyIR_ABSENCE_SET_EMPTY : _PyIR_ABSENCE_UNKNOWN;
        }
        return _PyIR_ABSENCE_PRESERVE;
    }
    if (fmt->owner_events != 1 ||
        _PyIR_OwnerPrograms[fmt->owner_program].kind != _PyIR_OWNER_CALL_DRAIN)
    {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    int count = opcode == CALL_FUNCTION_EX ? 4 : arg + extra;
    if (arg < 0 || count < extra || base < nl || base > limit - count ||
        home < nl || home >= limit ||
        fmt->roles[0] != _PyIR_ROLE_DST || fmt->roles[1] != _PyIR_ROLE_BASE ||
        (opcode == CALL_FUNCTION_EX
            ? fmt->roles[2] != _PyIR_ROLE_FB || fmt->roles[3] != _PyIR_ROLE_NONE
            : fmt->roles[2] != _PyIR_ROLE_IMM || fmt->roles[3] != _PyIR_ROLE_FB) ||
        _PyIR_Extent(fmt->reads, arg) != count ||
        _PyIR_Extent(fmt->writes, arg) > 1 ||
        _PyIR_Extent(fmt->through, arg) != 0 || fmt->dst != 0 ||
        fmt->output_kind != _PyIR_OUTPUT_WINDOW_DST_FIRST)
    {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    if (home == dst) {
        return _PyIR_ABSENCE_SET_VALUE;
    }
    if (home >= base && home - base < count &&
        _PyIR_CallInputRole(fmt, home - base) != _PyIR_CALL_INPUT_VIRTUAL_NULL)
    {
        return _PyIR_ABSENCE_SET_EMPTY;
    }
    return _PyIR_ABSENCE_PRESERVE;
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
