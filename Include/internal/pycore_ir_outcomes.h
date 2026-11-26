#ifndef Py_INTERNAL_IR_OUTCOMES_H
#define Py_INTERNAL_IR_OUTCOMES_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"
#include "pycore_rop_metadata.h"   // ROP_SELECT_Move_copy

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
        else if (fmt->roles[f] == _PyIR_ROLE_GROUP) {
            base = (int)fields[f];
        }
        else if (fmt->roles[f] == _PyIR_ROLE_SRC) {
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

/* The homes of a register instruction.  A word names up to six homes: its
 * fields with a home role and the further parts of its home group, part i
 * at the group's field plus i.  For each the format declares whether the
 * word reads it, whether it may be NULL when read, and what each normal
 * edge leaves there (_PyIR_HOME_*), the meet of the points its family's
 * arms publish at that edge.  The readers below answer from these, and
 * from the spans, moves and call operands that no head field names; a home
 * the word does not name is preserved. */
typedef struct {
    const _PyIRFormat *fmt;
    int slot[_PyIR_HOMES];  // the home's slot, or -1 if the word names none
    uint8_t read;           // bit h: the word reads home h
    uint8_t dst;            // bit h: home h is a destination field
} _PyIRHomes;

static inline void
_PyIR_WordHomes(_PyIRWord word, _PyIRHomes *v)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    v->fmt = fmt;
    v->read = v->dst = 0;
    for (int h = 0; h < _PyIR_HOMES; h++) {
        v->slot[h] = -1;
    }
    int at = -1;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_DST) {
            if (fields[f] != IR_NO_DST) {
                v->slot[f] = (int)fields[f];
            }
            v->dst |= 1u << f;
        }
        else if (role == _PyIR_ROLE_GROUP) {
            /* A defined group (GROUP_OUT) is not read */
            at = f;
            v->slot[f] = (int)fields[f];
            if (!(fmt->flags & _PyIR_GROUP_OUT)) {
                v->read |= 1u << f;
            }
        }
        else if (role == _PyIR_ROLE_CELL || role >= _PyIR_ROLE_SRC) {
            v->slot[f] = (int)fields[f];
            v->read |= 1u << f;
        }
    }
    if (at >= 0) {
        int first = v->slot[at];
        for (int i = 1; i < fmt->group && 3 + i < _PyIR_HOMES; i++) {
            v->slot[3 + i] = first < 0 ? -1 : first + i;
            if (v->read & (1u << at)) {
                v->read |= 1u << (3 + i);
            }
        }
    }
}

static inline int
_PyIR_HomeCode(const _PyIRFormat *fmt, int edge, int h)
{
    return (int)((fmt->edges >> (2 * (_PyIR_HOMES * edge + h))) & 3);
}

/* What edge leaves in home: -1 if the word names no such home.  Two
 * operands can name one home (a completion value in its receiver's home):
 * a definition is stored after the operands retire, so it wins; any other
 * disagreement is unknown.  Edge -1 is every normal edge the word takes. */
static inline int
_PyIR_HomeAfter(const _PyIRHomes *v, int edge, int home)
{
    if (edge < 0) {
        int state = -2;
        for (int e = 0; e < 2; e++) {
            if (v->fmt->normal & (1u << e)) {
                int after = _PyIR_HomeAfter(v, e, home);
                state = state == -2 || state == after ? after : _PyIR_HOME_UNKNOWN;
            }
        }
        return state == -2 ? _PyIR_HOME_UNKNOWN : state;
    }
    int state = -1;
    for (int h = 0; h < _PyIR_HOMES; h++) {
        if (v->slot[h] != home) {
            continue;
        }
        int code = _PyIR_HomeCode(v->fmt, edge, h);
        if (state < 0 || code == _PyIR_HOME_DEFINE) {
            state = code;
        }
        else if (state != code && state != _PyIR_HOME_DEFINE) {
            state = _PyIR_HOME_UNKNOWN;
        }
    }
    return state;
}

/* Whether the word reads home through a home operand; *vacant: whether
 * every such read accepts NULL */
static inline int
_PyIR_HomeRead(const _PyIRHomes *v, int home, int *vacant)
{
    int seen = 0;
    *vacant = 1;
    for (int h = 0; h < _PyIR_HOMES; h++) {
        if (v->slot[h] == home && (v->read & (1u << h))) {
            seen = 1;
            *vacant &= (v->fmt->vacant >> h) & 1;
        }
    }
    *vacant &= seen;
    return seen;
}

/* Whether some edge leaves home other than it was.  An edge the word does
 * not take leaves every home as it was. */
static inline int
_PyIR_HomeWritten(const _PyIRHomes *v, int home)
{
    for (int e = 0; e < 2; e++) {
        int after = _PyIR_HomeAfter(v, e, home);
        if (after >= 0 && after != _PyIR_HOME_PRESERVE) {
            return 1;
        }
    }
    return 0;
}

/* Whether the word reads home through a home operand, or writes it */
static inline int
_PyIR_HomeTouched(const _PyIRHomes *v, int home)
{
    int vacant;
    return _PyIR_HomeRead(v, home, &vacant) || _PyIR_HomeWritten(v, home);
}

/* The first home of the word's group, or -1 */
static inline int
_PyIR_HomeGroup(const _PyIRHomes *v)
{
    int at = _PyIR_GroupField(v->fmt);
    return at < 0 ? -1 : v->slot[at];
}

/* The word's home operands lie in the frame: a destination below limit or
 * none, a read home below limit, a group's parts in scratch, a cell among
 * the locals */
static inline int
_PyIR_HomesInFrame(_PyIRWord word, const _PyIRHomes *v, int nl, int limit)
{
    const _PyIRFormat *fmt = v->fmt;
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_GROUP) {
            int first = v->slot[f];
            if (first < nl || first > limit - fmt->group) {
                return 0;
            }
        }
        else if (role == _PyIR_ROLE_DST) {
            if (fields[f] != IR_NO_DST && fields[f] >= (uint32_t)limit) {
                return 0;
            }
        }
        else if (role == _PyIR_ROLE_CELL) {
            if (fields[f] >= (uint32_t)nl) {
                return 0;
            }
        }
        else if (role >= _PyIR_ROLE_SRC && fields[f] >= (uint32_t)limit) {
            return 0;
        }
    }
    return 1;
}

/* Whether edge publishes its homes before its callbacks run.  Constructor
 * words are pristine; monitoring runs their twin, which declares it. */
static inline int
_PyIR_Committed(_PyIRWord word, int edge)
{
    if (edge < 0 || edge > 1) {
        return 0;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->twin != _PyIR_NO_HANDLER) {
        fmt = _PyIR_FormatOf(fmt->twin);
    }
    return (fmt->committed >> edge) & 1;
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

/* The homes a committed edge leaves, each with its physical state and
 * whether the owner read there is retired (old_cut): storing a replacement
 * into the same home never revives the old owner.  A home read and kept
 * holds its owner (unknown, not retired); a vacated one is missing and
 * retired; a destination defined holds a value; any other definition (a
 * group part, which may be published NULL) or unknown state is unknown,
 * and retires the owner read there. */
static inline int
_PyIR_ObservedHomes(_PyIRWord word, int edge, int nl, int limit,
                    _PyIRObservedHomes *observed)
{
    if (!_PyIR_Committed(word, edge) || nl < 0 || limit < nl) {
        return 0;
    }
    *observed = (_PyIRObservedHomes){0};
    _PyIRHomes v;
    _PyIR_WordHomes(word, &v);
    if (!_PyIR_HomesInFrame(word, &v, nl, limit)) {
        return 0;
    }
    for (int h = 0; h < _PyIR_HOMES; h++) {
        int home = v.slot[h];
        int first = 1, read = 0, dst = 0;
        for (int k = 0; k < _PyIR_HOMES; k++) {
            if (v.slot[k] == home) {
                first &= k >= h;
                read |= (v.read >> k) & 1;
                dst |= (v.dst >> k) & 1;
            }
        }
        if (home < 0 || !first) {
            continue;
        }
        int physical, cut;
        switch (_PyIR_HomeAfter(&v, edge, home)) {
            case _PyIR_HOME_PRESERVE:
                if (!read) {
                    continue;
                }
                physical = _PyIR_OBSERVED_UNKNOWN;
                cut = 0;
                break;
            case _PyIR_HOME_VACATE:
                physical = _PyIR_OBSERVED_MISSING;
                cut = 1;
                break;
            case _PyIR_HOME_DEFINE:
                physical = dst ? _PyIR_OBSERVED_VALUE : _PyIR_OBSERVED_UNKNOWN;
                cut = read;
                break;
            default:
                physical = _PyIR_OBSERVED_UNKNOWN;
                cut = read;
                break;
        }
        if (!_PyIR_ObserveHome(observed, home, nl, limit, physical, cut)) {
            return 0;
        }
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
    if (IR_IS_EX_WORD(word)) {
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

/* A move word's pair count and source count: a Retire has sources only */
static inline void
_PyIR_MoveCounts(const _PyIRWord *wp, int *nd, int *ns)
{
    if (_PyIR_FormatOf(IR_HANDLER(*wp))->flags & _PyIR_VACANT_SPAN) {
        *nd = 0;
        *ns = (int)IR_A(*wp);
        return;
    }
    *nd = (int)IR_A(*wp);
    *ns = (int)IR_B(*wp);
}

/* The pair of a Move whose source is home, or -1; *extra says that home is
 * a source past the pairs, which Retire.Close closes */
static inline int
_PyIR_MoveSourceOf(const _PyIRWord *wp, int home, int *extra)
{
    int nd, ns;
    _PyIR_MoveCounts(wp, &nd, &ns);
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
    int nd, ns;
    _PyIR_MoveCounts(wp, &nd, &ns);
    (void)ns;
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

/* Whether the word at wp, read with the format of handler word (its own,
 * except where transparent observation hides the real handler), reads home
 * and every read accepts NULL: a home operand declared vacant (vacant(...)
 * or Maybe<Src>), a call operand that may be NULL, or a move's source */
static inline int
_PyIR_NullableHomeAs(_PyIRWord word, const _PyIRWord *wp, int home)
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
    _PyIRHomes v;
    _PyIR_WordHomes(word, &v);
    int vacant;
    int seen = _PyIR_HomeRead(&v, home, &vacant);
    if (seen && !vacant) {
        return 0;
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

static inline int
_PyIR_NullableHome(const _PyIRWord *wp, int home)
{
    return _PyIR_NullableHomeAs(*wp, wp, home);
}

static inline int
_PyIR_ReadsHome(const _PyIRWord *wp, int home)
{
    _PyIRWord word = *wp;
    if (_PyIR_IsMoveWord(word)) {
        return _PyIR_MoveReads(wp, home);
    }
    if (_PyIR_SpanReadsHome(wp, home)) {
        return 1;
    }
    _PyIRHomes v;
    _PyIR_WordHomes(word, &v);
    int vacant;
    if (_PyIR_HomeRead(&v, home, &vacant)) {
        return 1;
    }
    int nullable;
    return _PyIR_IsCallWord(word) &&
           _PyIR_CallReadsOperandHome(wp, home, &nullable);
}

/* The homes [*lo, *hi) that contain every home the word reads, so that a
 * caller testing each home for _PyIR_ReadsHome() can skip the rest. */
static inline void
_PyIR_ReadBounds(const _PyIRWord *wp, int *lo, int *hi)
{
    _PyIRWord word = *wp;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int low = INT_MAX, high = INT_MIN;
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
    _PyIRHomes v;
    _PyIR_WordHomes(word, &v);
    for (int h = 0; h < _PyIR_HOMES; h++) {
        if (v.read & (1u << h)) {
            low = Py_MIN(low, v.slot[h]);
            high = Py_MAX(high, v.slot[h] + 1);
        }
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

enum {
    _PyIR_BINDING_NO_READ,
    _PyIR_BINDING_CHECKED_NULL,
    _PyIR_BINDING_REQUIRES_VALUE,
};

/* A saved NULL fast-local can reach a checked read, but not an unchecked
 * local borrow or any other physical alias that requires a value. */
static inline int
_PyIR_BindingRead(const _PyIRWord *wp, int home)
{
    _PyIRWord word = *wp;
    // The checked arms of the local families test the local first
    if ((IR_HANDLER(word) == ROP_HANDLER_LocalRead_Checked &&
         (int)IR_B(word) == home) ||
        (IR_HANDLER(word) == ROP_HANDLER_LocalDelete_Checked &&
         (int)IR_A(word) == home))
    {
        return _PyIR_BINDING_CHECKED_NULL;
    }
    return _PyIR_ReadsHome(wp, home) ? _PyIR_BINDING_REQUIRES_VALUE
                                     : _PyIR_BINDING_NO_READ;
}

/* A physical write need not bind a value: a local store can receive a
 * saved NULL, and a delete deliberately clears its local. Only a destination
 * that a committed edge defines guarantees a value. */
static inline int
_PyIR_DefinesBinding(_PyIRWord word, int edge, int home)
{
    if (!_PyIR_Committed(word, edge)) {
        return 0;
    }
    _PyIRHomes v;
    _PyIR_WordHomes(word, &v);
    for (int h = 0; h < _PyIR_HOMES; h++) {
        if (v.slot[h] == home && (v.dst & (1u << h)) &&
            _PyIR_HomeCode(v.fmt, edge, h) == _PyIR_HOME_DEFINE)
        {
            return 1;
        }
    }
    return 0;
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
    /* A call: its callable and operands are taken, its result stored */
    _PyIR_PHYS_OPERANDS,
    _PyIR_PHYS_MOVE,
    /* The word's homes */
    _PyIR_PHYS_HOMES,
};

/* What a word does to the physical homes of its frame on a normal edge, or
 * on every one (edge -1), decoded once for the homes that _PyIR_PhysicalAt()
 * then tests one at a time. */
typedef struct {
    int mode, nl, limit, edge;
    const _PyIRWord *wp;
    _PyIRHomes v;
} _PyIRPhysicalWord;

static inline void
_PyIR_PhysicalWord(const _PyIRWord *wp, int edge, int nl, int limit,
                   _PyIRPhysicalWord *p)
{
    _PyIRWord word = *wp;
    int handler = IR_HANDLER(word);
    p->mode = _PyIR_PHYS_UNKNOWN;
    p->v.fmt = NULL;
    p->wp = wp;
    p->nl = nl;
    p->limit = limit;
    p->edge = edge;
    if (handler >= IR_NUM_HANDLERS || nl < 0 || limit > IR_MAX_SLOTS ||
        limit < nl || edge < -1 || edge > 1)
    {
        return;
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (_PyIR_IsMoveFormat(fmt) || _PyIR_CALL_SHAPED(fmt->operands)) {
        /* Neither jumps: its one normal edge is the next word */
        if (edge > 0) {
            return;
        }
        if (_PyIR_IsMoveFormat(fmt)) {
            p->mode = _PyIR_PHYS_MOVE;
            return;
        }
        /* Every successful call route takes its callable and operand
         * homes: native retirement clears them, and a Python child moves
         * them into its frame, which lies above the parent's.  Only the
         * result's home is rewritten besides, and it is no NULL fact. */
        if (fmt->roles[0] != _PyIR_ROLE_DST || fmt->roles[1] != _PyIR_ROLE_SRC ||
            (IR_A(word) != IR_NO_DST && IR_A(word) >= (uint32_t)limit) ||
            IR_B(word) >= (uint32_t)limit)
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
    /* No arm of the family takes the edge (a return, a raise) */
    if (edge >= 0 ? !(fmt->normal & (1u << edge)) : !fmt->normal) {
        return;
    }
    _PyIR_WordHomes(word, &p->v);
    if (!_PyIR_HomesInFrame(word, &p->v, nl, limit)) {
        return;
    }
    p->mode = _PyIR_PHYS_HOMES;
}

/* A normal successor's physical scratch-home update. TRANSPORT reads the
 * incoming row: callers must calculate all destinations before changing it.
 * A home the word keeps or does not name is preserved, one it vacates is
 * NULL (the owner was taken), and one it defines or leaves unknown is
 * unknown.  This does not prove that a source is a valid executable input
 * owner. */
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
            if (home == (int)IR_A(*p->wp)) {
                return unknown;
            }
            int nullable;
            if (home == (int)IR_B(*p->wp) ||
                _PyIR_CallReadsOperandHome(p->wp, home, &nullable))
            {
                return set_null;
            }
            return preserve;
        }
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
    }
    assert(p->mode == _PyIR_PHYS_HOMES);
    switch (_PyIR_HomeAfter(&p->v, p->edge, home)) {
        case -1:
        case _PyIR_HOME_PRESERVE:
            return preserve;
        case _PyIR_HOME_VACATE:
            return set_null;
        default:
            return unknown;
    }
}

static inline _PyIRPhysicalTransfer
_PyIR_NormalPhysicalTransfer(const _PyIRWord *wp, int edge, int home,
                             int nl, int limit)
{
    _PyIRPhysicalWord word;
    _PyIR_PhysicalWord(wp, edge, nl, limit, &word);
    return _PyIR_PhysicalAt(&word, home);
}

enum {
    _PyIR_ABSENCE_UNKNOWN,
    _PyIR_ABSENCE_PRESERVE,
    _PyIR_ABSENCE_SET_EMPTY,
    _PyIR_ABSENCE_SET_VALUE,
};

/* What every normal edge does to the logical absence of home: a home left
 * as it was keeps its absence, a vacated one is empty, a defined one holds
 * a value.  A returned child can leave nonowning header bytes in the arena,
 * so a call states no absence; nor does a move. */
static inline int
_PyIR_NormalAbsence(const _PyIRWord *wp, int home, int nl, int limit)
{
    _PyIRWord word = *wp;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (home < nl || home >= limit || _PyIR_CALL_SHAPED(fmt->operands) ||
        _PyIR_IsMoveFormat(fmt) || !fmt->normal)
    {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    _PyIRHomes v;
    _PyIR_WordHomes(word, &v);
    if (!_PyIR_HomesInFrame(word, &v, nl, limit)) {
        return _PyIR_ABSENCE_UNKNOWN;
    }
    switch (_PyIR_HomeAfter(&v, -1, home)) {
        case -1:
        case _PyIR_HOME_PRESERVE:
            return _PyIR_ABSENCE_PRESERVE;
        case _PyIR_HOME_VACATE:
            return _PyIR_ABSENCE_SET_EMPTY;
        case _PyIR_HOME_DEFINE:
            return _PyIR_ABSENCE_SET_VALUE;
        default:
            return _PyIR_ABSENCE_UNKNOWN;
    }
}
#endif
