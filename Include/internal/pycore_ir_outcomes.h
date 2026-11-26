#ifndef Py_INTERNAL_IR_OUTCOMES_H
#define Py_INTERNAL_IR_OUTCOMES_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"
#include "pycore_rop_metadata.h"   // ROP_SELECT_Move_copy

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
            if (dst < 0) {
                dst = (int)fields[f];
            }
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
            dst = dst < 0 ? (int)fields[f] : dst;
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

/* The operands of a call word (calls80): the slots that its operand words
 * name, which no field of the head does.  Operand k is the self slot only
 * if the word has one, and for CALL_FUNCTION_EX the second operand is the
 * keyword arguments.  Both may be NULL at run time; no other operand is. */
static inline int
_PyIR_IsCallWord(_PyIRWord word)
{
    return _PyIR_CALL_SHAPED(_PyIR_FormatOf(IR_HANDLER(word))->operands);
}

static inline int
_PyIR_CallNumSlots(_PyIRWord word)
{
    return _PyIR_CallSlotOperands(word);
}

static inline int
_PyIR_CallOperandNullable(_PyIRWord word, int k)
{
    if (_PyIR_FormatOf(IR_HANDLER(word))->operands == _PyIR_OPERANDS_EX) {
        return k == 1;
    }
    return k == 0 && _PyIR_CallHasSelf(word);
}

/* Whether a call word reads home as one of its operands, and whether every
 * such read accepts NULL */
static inline int
_PyIR_CallReadsOperandHome(const _PyIRWord *wp, int home, int *nullable)
{
    int found = 0;
    *nullable = 1;
    int n = _PyIR_CallNumSlots(*wp);
    for (int k = 0; k < n; k++) {
        if (_PyIR_CallOperand(wp, k) == home) {
            found = 1;
            *nullable &= _PyIR_CallOperandNullable(*wp, k);
        }
    }
    return found;
}

/* Inputs whose ABI accepts NULL: window-relative positions for WINDOW rows,
 * semantic IN0+k positions for register rows. This is a read capability,
 * not a definition or retirement: cold consumers must independently prove
 * the home is NULL. A call's callable is not nullable; its operands are
 * judged by _PyIR_CallOperandNullable(). */
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
        case CALL_FUNCTION_EX:
            return 0;  /* Only operands accept NULL: see above. */
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
        case YIELD_DELEGATED:
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

/* A Move word (Python/register_ops/values.rop): its destination count, its
 * source count, and where element j of its spans is */
static inline int
_PyIR_IsMoveWord(_PyIRWord word)
{
    return _PyIR_IsMoveFormat(_PyIR_FormatOf(IR_HANDLER(word)));
}

/* Whether home is an element of a span that the word at wp reads */
static inline int
_PyIR_SpanReadsHome(const _PyIRWord *wp, int home)
{
    int handler = IR_HANDLER(*wp);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->operands != _PyIR_OPERANDS_SPANS) {
        return 0;
    }
    int nspans = _PyIR_SpanCountFields(fmt);
    for (int k = 0; k < nspans; k++) {
        int first, is_source;
        int count = _PyIR_SpanBounds(wp, handler, k, &first, &is_source);
        for (int j = 0; is_source && j < count; j++) {
            if (_PyIR_SpanElement(wp, handler, first + j) == home) {
                return 1;
            }
        }
    }
    return 0;
}

/* The pair of a Move whose source is home, or -1; *extra says that home is
 * a source past the pairs, which Move.Retire closes */
static inline int
_PyIR_MoveSourceOf(const _PyIRWord *wp, int home, int *extra)
{
    int nd = (int)IR_A(*wp), ns = (int)IR_B(*wp);
    *extra = 0;
    for (int k = 0; k < ns; k++) {
        if (_PyIR_SpanElement(wp, IR_HANDLER(*wp), nd + k) == home) {
            *extra = k >= nd;
            return k < nd ? k : -1;
        }
    }
    return -1;
}

/* The source home that a Move gives home, or -1 if home is no destination */
static inline int
_PyIR_MoveGives(const _PyIRWord *wp, int home)
{
    int nd = (int)IR_A(*wp);
    for (int k = 0; k < nd; k++) {
        if (_PyIR_SpanElement(wp, IR_HANDLER(*wp), k) == home) {
            return _PyIR_SpanElement(wp, IR_HANDLER(*wp), nd + k);
        }
    }
    return -1;
}

static inline int
_PyIR_MoveReads(const _PyIRWord *wp, int home)
{
    int extra;
    return _PyIR_MoveSourceOf(wp, home, &extra) >= 0 || extra;
}

/* A physical home can alias several semantic inputs. Every actual read,
 * including an independent window use, must accept NULL. */
static inline int
_PyIR_NullableHomeAs(_PyIRWord word, const _PyIRWord *wp, int arg, int home)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (_PyIR_IsMoveFormat(fmt)) {
        // A move moves what it finds, NULL too
        return _PyIR_MoveReads(wp, home);
    }
    if (_PyIR_SpanReadsHome(wp, home)) {
        // The elements of a span are values that the word takes
        return 0;
    }
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
            // A swap moves what it finds, NULL too: resolution moves a
            // value to a vacant home by one
            int moves = _PyIR_BaseOpcode(IR_HANDLER(word)) == SWAP;
            if (relative < 0 || relative >= 32 ||
                (!moves && !(inputs & (1u << relative))))
            {
                return 0;
            }
            seen = 1;
        }
    }
    if (_PyIR_IsCallWord(word)) {
        int nullable;
        if (_PyIR_CallReadsOperandHome(wp, home, &nullable)) {
            if (!nullable) {
                return 0;
            }
            seen = 1;
        }
    }
    return seen;
}

/* The head of the word at wp as it would read with handler word, which is
 * wp's own except where transparent observation hides the real handler */
static inline int
_PyIR_NullableHome(const _PyIRWord *wp, int arg, int home)
{
    return _PyIR_NullableHomeAs(*wp, wp, arg, home);
}

static inline int
_PyIR_ReadsHome(const _PyIRWord *wp, int arg, int home)
{
    _PyIRWord word = *wp;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (_PyIR_IsMoveFormat(fmt)) {
        return _PyIR_MoveReads(wp, home);
    }
    if (_PyIR_SpanReadsHome(wp, home)) {
        return 1;
    }
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] >= _PyIR_ROLE_IN0 && (int)fields[f] == home) {
            return 1;
        }
    }
    if (_PyIR_IsCallWord(word)) {
        int nullable;
        return _PyIR_CallReadsOperandHome(wp, home, &nullable);
    }
    return _PyIR_ReadsWindowHome(word, arg, home);
}

/* The homes [*lo, *hi) that contain every home the word reads, so that a
 * caller testing each home for _PyIR_ReadsHome() can skip the rest. */
static inline void
_PyIR_ReadBounds(const _PyIRWord *wp, int arg, int *lo, int *hi)
{
    _PyIRWord word = *wp;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int low = INT_MAX, high = INT_MIN;
    int window = -1;
    if (fmt->operands == _PyIR_OPERANDS_SPANS) {
        int nspans = _PyIR_SpanCountFields(fmt);
        for (int s = 0; s < nspans; s++) {
            int first, is_source;
            int count = _PyIR_SpanBounds(wp, IR_HANDLER(word), s, &first,
                                         &is_source);
            for (int k = 0; is_source && k < count; k++) {
                int src = _PyIR_SpanElement(wp, IR_HANDLER(word), first + k);
                low = Py_MIN(low, src);
                high = Py_MAX(high, src + 1);
            }
        }
    }
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] >= _PyIR_ROLE_IN0) {
            low = Py_MIN(low, (int)fields[f]);
            high = Py_MAX(high, (int)fields[f] + 1);
        }
        if (fmt->roles[f] == _PyIR_ROLE_BASE) {
            window = (int)fields[f];
        }
    }
    int reads = _PyIR_Extent(fmt->reads, arg);
    if (window >= 0 && reads > 0) {
        low = Py_MIN(low, window);
        high = Py_MAX(high, window + reads);
    }
    if (_PyIR_IsCallWord(word)) {
        int n = _PyIR_CallNumSlots(word);
        for (int k = 0; k < n; k++) {
            int operand = _PyIR_CallOperand(wp, k);
            low = Py_MIN(low, operand);
            high = Py_MAX(high, operand + 1);
        }
    }
    *lo = low;
    *hi = high;
}

/* The successful final stack commit consumes inputs outside its unchanged
 * window prefix. Through homes retain their logical values even when they
 * are read; fresh outputs are handled independently by the caller. */
static inline int
_PyIR_ConsumesNormalInput(const _PyIRWord *wp, int arg, int home)
{
    _PyIRWord word = *wp;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int through = _PyIR_Extent(fmt->through, arg);
    for (int f = 0; f < 4; f++) {
        if (!(fmt->flags & _PyIR_WINDOW) && fmt->roles[f] >= _PyIR_ROLE_IN0 &&
            fmt->roles[f] - _PyIR_ROLE_IN0 < through &&
            (int)fields[f] == home)
        {
            return 0;
        }
        if (fmt->roles[f] == _PyIR_ROLE_BASE &&
            home >= (int)fields[f] && home - (int)fields[f] < through)
        {
            return 0;
        }
    }
    return _PyIR_ReadsHome(wp, arg, home);
}

enum {
    _PyIR_BINDING_NO_READ,
    _PyIR_BINDING_CHECKED_NULL,
    _PyIR_BINDING_REQUIRES_VALUE,
};

/* A saved NULL fast-local can reach a checked read, but not an unchecked
 * local borrow or any other physical alias that requires a value. */
static inline int
_PyIR_BindingRead(const _PyIRWord *wp, int arg, int home)
{
    _PyIRWord word = *wp;
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
    // The checked arms of the local families test the local first
    if ((IR_HANDLER(word) == ROP_HANDLER_LocalRead_Checked &&
         (int)IR_B(word) == home) ||
        (IR_HANDLER(word) == ROP_HANDLER_LocalDelete_Checked &&
         (int)IR_A(word) == home))
    {
        return _PyIR_BINDING_CHECKED_NULL;
    }
    if (_PyIR_ReadsHome(wp, arg, home)) {
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
 * preserve owner provenance; they cannot reconstruct an unavailable input.
 * The homes of a word are the range [lo, hi) but for except, and single.
 * Callers that test many homes of one word decode it once. */
typedef struct {
    int lo, hi, single, except;
} _PyIRDefines;

static inline _PyIRDefines
_PyIR_DefinesOf(_PyIRWord word, int arg)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    const _PyIRDefines none = {0, 0, INT_MIN, INT_MIN};
    // A swap or a move transports values: it makes none
    if (_PyIR_Outcome(word, 0).semantic ||
        _PyIR_BaseOpcode(IR_HANDLER(word)) == SWAP)
    {
        return none;
    }
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int dst = -1, window = -1;
    for (int f = 0; f < 4; f++) {
        if (fmt->roles[f] == _PyIR_ROLE_DST) {
            if (dst < 0) {
                dst = (int)fields[f];
            }
        }
        if (fmt->roles[f] == _PyIR_ROLE_BASE) {
            window = (int)fields[f];
        }
    }
    int through = _PyIR_Extent(fmt->through, arg);
    int writes = _PyIR_Extent(fmt->writes, arg);
    switch (fmt->output_kind) {
        case _PyIR_OUTPUT_REGISTER_DST:
            if (dst >= 0 && dst != IR_NO_DST) {
                return (_PyIRDefines){dst, dst + writes - through,
                                      INT_MIN, INT_MIN};
            }
            return none;
        case _PyIR_OUTPUT_WINDOW:
            if (window >= 0) {
                return (_PyIRDefines){window + through, window + writes,
                                      INT_MIN, INT_MIN};
            }
            return none;
        case _PyIR_OUTPUT_WINDOW_DST_FIRST: {
            _PyIRDefines defines = none;
            if (dst >= 0 && dst != IR_NO_DST) {
                defines.single = dst;
            }
            if (window >= 0) {
                defines.lo = window + through;
                defines.hi = window + writes;
                defines.except = window + fmt->dst;
            }
            return defines;
        }
        default:
            return none;
    }
}

static inline int
_PyIR_DefinesAt(const _PyIRDefines *defines, int home)
{
    return home == defines->single ||
           (home >= defines->lo && home < defines->hi &&
            home != defines->except);
}

static inline int
_PyIR_DefinesHome(_PyIRWord word, int arg, int home)
{
    _PyIRDefines defines = _PyIR_DefinesOf(word, arg);
    return _PyIR_DefinesAt(&defines, home);
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

enum {
    /* Every home is unknown */
    _PyIR_PHYS_UNKNOWN,
    _PyIR_PHYS_OPERANDS,
    /* Only the home of A is cleared */
    _PyIR_PHYS_CLEAR_A,
    /* Only the home of B is cleared: a store takes its value's owner */
    _PyIR_PHYS_CLEAR_B,
    _PyIR_PHYS_COPY,
    _PyIR_PHYS_SWAP,
    _PyIR_PHYS_MOVE,
    _PyIR_PHYS_YIELD,
    _PyIR_PHYS_FOR_ITER,
    _PyIR_PHYS_COMPLETE,
    _PyIR_PHYS_GENERIC,
};

/* What a word does to the physical homes of its frame, decoded once for the
 * homes that _PyIR_PhysicalAt() then tests one at a time. */
typedef struct {
    int mode, nl, limit, edge, arg;
    int base, dst, through, writes, window_reads;
    int completion;
    uint32_t fields[4];
    const _PyIRWord *wp;
    const _PyIRFormat *fmt;
    _PyIRSlotOutcome outcome;
    _PyIRDefines defines;
} _PyIRPhysicalWord;

static inline void
_PyIR_PhysicalWord(const _PyIRWord *wp, int arg, int edge, int nl, int limit,
                   _PyIRPhysicalWord *p)
{
    _PyIRWord word = *wp;
    p->mode = _PyIR_PHYS_UNKNOWN;
    int handler = IR_HANDLER(word);
    if (handler >= IR_NUM_HANDLERS || nl < 0 || limit > IR_MAX_SLOTS ||
        limit < nl || edge < 0 || edge > 1)
    {
        return;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (!(fmt->flags & _PyIR_VALID) ||
        (edge && !(fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD))))
    {
        return;
    }
    if (_PyIR_IsMoveFormat(fmt)) {
        if (edge) {
            return;
        }
        p->wp = wp;
        p->fmt = fmt;
        p->nl = nl;
        p->limit = limit;
        p->edge = edge;
        p->arg = arg;
        p->mode = _PyIR_PHYS_MOVE;
        return;
    }
    p->fields[0] = IR_A(word);
    p->fields[1] = IR_B(word);
    p->fields[2] = IR_C(word);
    p->fields[3] = IR_D(word);
    if (handler == ROP_HANDLER_LocalWrite_Store ||
        handler == ROP_HANDLER_LocalWrite_Restore)
    {
        // The local (A) is no scratch home; the value's (B) is left vacant
        p->nl = nl;
        p->limit = limit;
        p->mode = _PyIR_PHYS_CLEAR_B;
        return;
    }
    const uint32_t *fields = p->fields;
    int base = -1;
    int dst = -1;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_BASE) {
            base = (int)fields[f];
        }
        else if (role == _PyIR_ROLE_DST) {
            dst = dst < 0 ? (int)fields[f] : dst;
            if (dst != IR_NO_DST && dst >= limit) {
                return;
            }
            if (dst == IR_NO_DST &&
                !(fmt->flags & _PyIR_DISCARDS)) {
                return;
            }
        }
        else if (role >= _PyIR_ROLE_IN0 && fields[f] >= (uint32_t)limit) {
            return;
        }
        else if (role >= _PyIR_ROLE_LOCAL_IN &&
                 role <= _PyIR_ROLE_CELL && fields[f] >= (uint32_t)nl)
        {
            return;
        }
    }
    /* Extent arithmetic uses int; reject malformed arguments before it. */
    if (arg < -IR_FIELD_MAX || arg > IR_CD_MAX) {
        return;
    }
    int reads = _PyIR_Extent(fmt->reads, arg);
    int writes = _PyIR_Extent(fmt->writes, arg);
    int through = _PyIR_Extent(fmt->through, arg);
    if (reads < 0 || writes < 0 || through < 0 || through > reads ||
        writes < through ||
        (base >= 0 && (base < nl || base > limit - Py_MAX(reads, writes))))
    {
        return;
    }
    p->wp = wp;
    p->fmt = fmt;
    p->nl = nl;
    p->limit = limit;
    p->edge = edge;
    p->arg = arg;
    p->base = base;
    p->dst = dst;
    p->through = through;
    p->writes = writes;

    if (_PyIR_CALL_SHAPED(fmt->operands)) {
        /* Every successful call route takes its callable and operand
         * homes: native retirement clears them, and a Python child moves
         * them into its frame, which lies above the parent's.  Only the
         * result's home is rewritten besides, and it is no NULL fact. */
        if (edge || fmt->roles[0] != _PyIR_ROLE_DST ||
            fmt->roles[1] != _PyIR_ROLE_IN0 || arg < 0 ||
            (dst >= nl && dst != IR_NO_DST && dst >= limit))
        {
            return;
        }
        int n = _PyIR_CallNumSlots(word);
        for (int k = 0; k < n; k++) {
            if (_PyIR_CallOperand(wp, k) >= limit) {
                return;
            }
        }
        p->mode = _PyIR_PHYS_OPERANDS;
        return;
    }
    switch (_PyIR_BaseOpcode(handler)) {
        case COPY:
            if (edge || fmt->roles[0] != _PyIR_ROLE_BASE ||
                fmt->roles[1] != _PyIR_ROLE_IMM || arg != (int)IR_B(word) ||
                arg < 1 || reads != arg || writes != arg + 1 ||
                through != arg || base < 0)
            {
                return;
            }
            p->mode = _PyIR_PHYS_COPY;
            return;
        case SWAP:
            if (edge || fmt->roles[0] != _PyIR_ROLE_BASE ||
                fmt->roles[1] != _PyIR_ROLE_IMM || arg != (int)IR_B(word) ||
                arg < 2 || reads != arg || writes != arg || base < 0)
            {
                return;
            }
            p->mode = _PyIR_PHYS_SWAP;
            return;
        case STORE_FAST:
            if (edge || fmt->roles[0] != _PyIR_ROLE_IN0 ||
                fmt->roles[1] != _PyIR_ROLE_LOCAL_OUT ||
                arg != (int)IR_B(word) || IR_B(word) >= (uint32_t)nl ||
                reads != 1 || writes != 0)
            {
                return;
            }
            p->mode = _PyIR_PHYS_CLEAR_A;
            return;
        case POP_EXCEPT:
            /* It detaches scratch A, the saved exception */
            if (edge || handler != ROP_HANDLER_ExceptLeave_Restore ||
                fmt->roles[0] != _PyIR_ROLE_IN0 ||
                fmt->roles[1] != _PyIR_ROLE_NONE ||
                fmt->roles[2] != _PyIR_ROLE_NONE ||
                fmt->roles[3] != _PyIR_ROLE_NONE ||
                IR_A(word) < (uint32_t)nl)
            {
                return;
            }
            p->mode = _PyIR_PHYS_CLEAR_A;
            return;
        case SEND:
        case CLEANUP_THROW:
            /* A completion that discards its value retires the whole window
             * (gen_complete): the receiver, the cursor, the sent value and,
             * for a throw, the exception. */
            if (edge == 1 && base >= 0 && (fmt->flags & _PyIR_DISCARDS) &&
                fmt->roles[1] == _PyIR_ROLE_BASE)
            {
                p->completion = _PyIR_BaseOpcode(handler) == SEND ? 3 : 4;
                if (base > limit - p->completion) {
                    return;
                }
                p->mode = _PyIR_PHYS_COMPLETE;
                return;
            }
            break;
        case YIELD_VALUE:
            if (_PyIR_HandlerBase[handler] == YIELD_DELEGATED) {
                /* A send's delegated yield names its inputs in IN fields;
                 * the generic rule keeps every other home. */
                break;
            }
            if (edge || fmt->roles[0] != _PyIR_ROLE_DST ||
                fmt->roles[1] != _PyIR_ROLE_BASE || reads != 1 || writes != 1 ||
                through != 0 || base < 0)
            {
                return;
            }
            p->mode = _PyIR_PHYS_YIELD;
            return;
        case FOR_ITER:
            if (fmt->roles[0] != _PyIR_ROLE_DST ||
                fmt->roles[1] != _PyIR_ROLE_BASE || base < 0 ||
                reads != 2 || fmt->dst != 2 ||
                !((writes == 3 && through <= 2) ||
                  (writes == 2 && through == 2 &&
                   (fmt->effects & _PyIR_PUSHES_FRAME))) ||
                (dst >= nl && dst != IR_NO_DST && dst != base + 2))
            {
                return;
            }
            /* A pristine FOR word can run the child-entering GEN variant.
             * Only the completed receiver/cursor pair has a shared fact. */
            p->completion = edge == 1 &&
                _PyIR_ObserverOutcome(word, edge) == _PyIR_OBSERVER_COMPLETION;
            p->mode = _PyIR_PHYS_FOR_ITER;
            return;
    }

    /* A send enters the receiver generator's own frame, never this frame's
     * arena, so homes outside its window survive like any window word's. */
    if ((fmt->effects & _PyIR_PUSHES_FRAME) &&
        !(_PyIR_BaseOpcode(handler) == SEND && base >= 0))
    {
        /* Only an actual independent reservation preserves the parent's
         * homes outside its consumed inputs and eventual recipient. */
        if (fmt->owner_events != 1) {
            return;
        }
        const _PyIROwnershipEvent *event = _PyIR_OwnerPrograms + fmt->owner_program;
        if (event->kind != _PyIR_OWNER_NORMAL_CLEAR_INPUTS ||
            !(event->flags & _PyIR_NORMAL_DISJOINT_CHILD))
        {
            return;
        }
    }
    int window_reads = reads;
    if ((fmt->flags & _PyIR_WINDOW) && fmt->owner_events == 1) {
        int event = _PyIR_OwnerPrograms[fmt->owner_program].kind;
        if (event == _PyIR_OWNER_NORMAL_CLEAR_INPUTS ||
            event == _PyIR_OWNER_UNPACK_LOCAL_DRAIN)
        {
            /* The actual final stack commit reads trailing scalar inputs
             * through their IN fields, not their former window positions.
             * Folding one into a local leaves that scratch tail untouched.
             * Outputs still use their complete physical window extent. */
            if (fmt->ninputs > reads - through) {
                return;
            }
            window_reads -= fmt->ninputs;
        }
    }
    p->window_reads = window_reads;
    p->outcome = _PyIR_Outcome(word, edge);
    p->defines = _PyIR_DefinesOf(word, arg);
    p->mode = _PyIR_PHYS_GENERIC;
}

/* A normal successor's physical scratch-home update. TRANSPORT reads the
 * incoming row: callers must calculate all destinations before changing it.
 * This does not prove that a source is a valid executable input owner. */
static inline _PyIRPhysicalTransfer
_PyIR_PhysicalAt(const _PyIRPhysicalWord *p, int home)
{
    const _PyIRPhysicalTransfer unknown = {_PyIR_PHYSICAL_UNKNOWN, -1};
    const _PyIRPhysicalTransfer preserve = {_PyIR_PHYSICAL_PRESERVE, -1};
    const _PyIRPhysicalTransfer set_null = {_PyIR_PHYSICAL_SET_NULL, -1};
    if (p->mode == _PyIR_PHYS_UNKNOWN || home < p->nl || home >= p->limit) {
        return unknown;
    }
    switch (p->mode) {
        case _PyIR_PHYS_OPERANDS: {
            if (home == p->dst) {
                return unknown;
            }
            int nullable;
            if (home == (int)p->fields[1] ||
                _PyIR_CallReadsOperandHome(p->wp, home, &nullable))
            {
                return set_null;
            }
            return preserve;
        }
        case _PyIR_PHYS_CLEAR_A:
            return home == (int)p->fields[0] ? set_null : preserve;
        case _PyIR_PHYS_CLEAR_B:
            return home == (int)p->fields[1] ? set_null : preserve;
        case _PyIR_PHYS_COPY:
            if (home == p->base + p->arg) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT, p->base};
            }
            return preserve;
        case _PyIR_PHYS_MOVE: {
            /* A destination takes its source's value; a source that no
             * pair writes is left vacant by Transfer and Retire, and keeps
             * its value under Copy */
            int from = _PyIR_MoveGives(p->wp, home);
            if (from >= 0) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT, from};
            }
            if (_PyIR_MoveReads(p->wp, home)) {
                return IR_HANDLER(*p->wp) == ROP_SELECT_Move_copy
                    ? preserve : set_null;
            }
            return preserve;
        }
        case _PyIR_PHYS_SWAP:
            if (home == p->base) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT,
                                               p->base + p->arg - 1};
            }
            if (home == p->base + p->arg - 1) {
                return (_PyIRPhysicalTransfer){_PyIR_PHYSICAL_TRANSPORT, p->base};
            }
            return preserve;
        case _PyIR_PHYS_YIELD:
            if (home == p->dst) {
                return unknown;
            }
            return home == p->base ? set_null : preserve;
        case _PyIR_PHYS_FOR_ITER:
            if (p->completion && (home == p->base || home == p->base + 1)) {
                return set_null;
            }
            return unknown;
        case _PyIR_PHYS_COMPLETE:
            return home >= p->base && home - p->base < p->completion
                ? set_null : preserve;
    }
    assert(p->mode == _PyIR_PHYS_GENERIC);
    /* In every other format, a read outside the unchanged prefix or a
     * published result may replace the old physical value. An untouched
     * home, including LOAD_GLOBAL's unrequested optional-self slot, survives. */
    const _PyIRFormat *fmt = p->fmt;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role >= _PyIR_ROLE_IN0 && (int)p->fields[f] == home) {
            return unknown;
        }
        if (role >= _PyIR_ROLE_LOCAL_IN && role <= _PyIR_ROLE_CELL &&
            (int)p->fields[f] == home)
        {
            return unknown;
        }
    }
    if (p->base >= 0 && home >= p->base + p->through &&
        home - p->base < Py_MAX(p->window_reads, p->writes))
    {
        return unknown;
    }
    if (p->outcome.semantic &&
        ((home >= p->outcome.first &&
          home - p->outcome.first < p->outcome.count) ||
         home == p->outcome.dst))
    {
        return unknown;
    }
    if (_PyIR_DefinesAt(&p->defines, home)) {
        return unknown;
    }
    return preserve;
}

static inline _PyIRPhysicalTransfer
_PyIR_NormalPhysicalTransfer(const _PyIRWord *wp, int arg, int edge, int home,
                             int nl, int limit)
{
    _PyIRPhysicalWord word;
    _PyIR_PhysicalWord(wp, arg, edge, nl, limit, &word);
    return _PyIR_PhysicalAt(&word, home);
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
_PyIR_NormalCallAbsence(const _PyIRWord *wp, int arg, int home, int nl, int limit)
{
    _PyIRWord word = *wp;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (home < nl || home >= limit) {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
    int is_call = _PyIR_CALL_SHAPED(fmt->operands);
    int base = (int)IR_B(word), dst = (int)IR_A(word);
    if (!is_call) {
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
        if (opcode == FOR_ITER && reads == 2 &&
            fmt->owner_events == 1 &&
            _PyIR_OwnerPrograms[fmt->owner_program].kind ==
                _PyIR_OWNER_NORMAL_COMPLETION)
        {
            /* The body result is published to A, including a named binding.
             * A nonselected BASE+2 is neither an input nor an output home:
             * retain only its incoming absence, never create a new one. */
            return home >= base && home - base < reads
                ? _PyIR_ABSENCE_UNKNOWN : _PyIR_ABSENCE_PRESERVE;
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
    if (arg < 0 || fmt->roles[0] != _PyIR_ROLE_DST ||
        fmt->roles[1] != _PyIR_ROLE_IN0 || fmt->roles[2] != _PyIR_ROLE_IMM ||
        _PyIR_Extent(fmt->writes, arg) > 1 ||
        _PyIR_Extent(fmt->through, arg) != 0 || fmt->dst != 0 ||
        fmt->output_kind != _PyIR_OUTPUT_REGISTER_DST)
    {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    if (home == dst) {
        return _PyIR_ABSENCE_SET_VALUE;
    }
    int nullable;
    if (home == base || _PyIR_CallReadsOperandHome(wp, home, &nullable)) {
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
