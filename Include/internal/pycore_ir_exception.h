#ifndef Py_INTERNAL_IR_EXCEPTION_H
#define Py_INTERNAL_IR_EXCEPTION_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"          // _PyCodeIR

/* Semantic recovery is independent of optional line/column information.
 * The exceptions and the recovery rows are arrays of the code's _PyCodeIR. */
#define _PyIR_RECOVERY_FIELDS 7
#define _PyIR_RECOVERY_RECORD (_PyIR_RECOVERY_FIELDS * 4)
#define _PyIR_EDGE_FALLTHROUGH 0
#define _PyIR_EDGE_TAKEN 1

typedef struct {
    const unsigned char *exceptions;
    uint32_t exceptions_size;
    const unsigned char *records;
    uint32_t nrecords;
} _PyIRSemanticMap;

typedef struct {
    int32_t from, to, edge, handler, exc_slot, lasti;
    int32_t recipe;             // index + 1 in the code's recipes, or 0
} _PyIRRecoveryEvent;
static_assert(sizeof(_PyIRRecoveryEvent) == _PyIR_RECOVERY_RECORD,
              "recovery serialization must not contain padding");

static inline uint32_t
_PyIRSemantic_Read32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void
_PyIRSemantic_Write32(unsigned char *p, uint32_t value)
{
    for (int i = 0; i < 4; i++) {
        p[i] = (unsigned char)(value >> (8 * i));
    }
}

static inline _PyIRSemanticMap
_PyIRSemantic_Stored(PyCodeObject *code)
{
    const _PyCodeIR *ir = _PyCode_IR(code);
    return (_PyIRSemanticMap){ir->exceptions, ir->nexceptions, ir->records,
                              ir->nrecords};
}

static inline void
_PyIRSemantic_Event(const _PyIRSemanticMap *map, uint32_t index,
                    _PyIRRecoveryEvent *event)
{
    assert(index < map->nrecords);
    const unsigned char *p = map->records + (size_t)index * _PyIR_RECOVERY_RECORD;
    int32_t values[_PyIR_RECOVERY_FIELDS];
    for (int i = 0; i < _PyIR_RECOVERY_FIELDS; i++) {
        values[i] = (int32_t)_PyIRSemantic_Read32(p + i * 4);
    }
    memcpy(event, values, sizeof(*event));
}

static inline void
_PyIRSemantic_EncodeEvent(unsigned char *p, const _PyIRRecoveryEvent *event)
{
    int32_t values[_PyIR_RECOVERY_FIELDS];
    memcpy(values, event, sizeof(*event));
    for (int i = 0; i < _PyIR_RECOVERY_FIELDS; i++) {
        _PyIRSemantic_Write32(p + i * 4, (uint32_t)values[i]);
    }
}

/* Cold readers share the marked, sorted exception-table contract used by
 * runtime lookup. Geometry is passed explicitly for code construction.
 * The IR's entries have a fifth field, the ingress recipe of the edge
 * (_PyCodeIR.recipes); co_exceptiontable's have four. */
typedef struct {
    int start, end, target, slot, lasti, recipe;
} _PyIRExceptionEntry;

#define _PyIR_EXCEPTION_FIELDS 5
#define _PyCODE_EXCEPTION_FIELDS 4

static inline int
_PyIR_ExceptionVarint(const unsigned char **cursor, const unsigned char *end,
                      int *value)
{
    uint32_t result = 0;
    unsigned char byte;
    do {
        if (*cursor == end || result > ((uint32_t)INT_MAX >> 6)) {
            return -1;
        }
        byte = *(*cursor)++;
        result = (result << 6) | (byte & 63);
    } while (byte & 64);
    *value = (int)result;
    return 0;
}

static inline int
_PyIR_ExceptionDecode(const unsigned char **cursor, const unsigned char *end,
                      int nfields, _PyIRExceptionEntry *entry)
{
    int fields[_PyIR_EXCEPTION_FIELDS] = {0};
    for (int f = 0; f < nfields; f++) {
        const unsigned char *first = *cursor;
        if (*cursor == end || ((**cursor & 128) != (f == 0 ? 128 : 0)) ||
            _PyIR_ExceptionVarint(cursor, end, &fields[f]) < 0)
        {
            return -1;
        }
        for (const unsigned char *p = first + 1; p < *cursor; p++) {
            if (*p & 128) {
                return -1;
            }
        }
    }
    if (fields[1] > INT_MAX - fields[0]) {
        return -1;
    }
    *entry = (_PyIRExceptionEntry){fields[0], fields[0] + fields[1],
                                   fields[2], fields[3] >> 1, fields[3] & 1,
                                   fields[4]};
    return 0;
}

/* The next entry of an IR exception table */
static inline int
_PyIR_ExceptionNext(const unsigned char **cursor, const unsigned char *end,
                    _PyIRExceptionEntry *entry)
{
    return _PyIR_ExceptionDecode(cursor, end, _PyIR_EXCEPTION_FIELDS, entry);
}

/* The recipe of index + 1 in the code's recipes, NULL for none.  It is the
 * parallel move of n pairs of a source slot and a home, sequenced when the
 * code is lowered (_PyRegCfg_SequenceRecipe()), so the unwinder solves
 * nothing:
 *     n, then n pairs (source, home);
 *     the pairs of no cycle come first, in order: each one exchanges its
 *     source and home, which leaves what the home held at the source;
 *     then the pairs of each cycle in turn, one after the other, each home
 *     being the next pair's source, which turn one place through a C local;
 *     then the number of cycles and the length of each;
 *     then the number of released sources and those slots: the sources of
 *     the exchanges that no home takes, left with what their homes held,
 *     which the unwinder closes. */
static inline const int32_t *
_PyIR_Recipe(const _PyCodeIR *ir, int recipe)
{
    return recipe ? &ir->recipes[recipe - 1] : NULL;
}

/* The entries of a recipe, its tail included */
static inline int
_PyIR_RecipeSize(const int32_t *r)
{
    int n = r[0], ncycles = r[1 + 2 * n];
    return 1 + 2 * n + 1 + ncycles + 1 + r[2 + 2 * n + ncycles];
}

/* Whether recipe index + 1 of ir is whole, with sources in the frame and
 * each home below the handler's floor once */
static inline int
_PyIR_RecipeValid(const _PyCodeIR *ir, int recipe, int nlocals, int floor)
{
    if (recipe == 0) {
        return 1;
    }
    if (recipe < 0 || (uint32_t)recipe > ir->nrecipes) {
        return 0;
    }
    const int32_t *r = &ir->recipes[recipe - 1];
    int n = r[0];
    if (n <= 0 || n > (int)ir->nslots ||
        (uint32_t)(recipe + 2 * n + 1) > ir->nrecipes)
    {
        return 0;
    }
    int ncycles = r[1 + 2 * n];
    if (ncycles < 0 || ncycles > n / 2 ||
        (uint32_t)(recipe + 2 * n + ncycles + 2) > ir->nrecipes)
    {
        return 0;
    }
    int nrel = r[2 + 2 * n + ncycles];
    if (nrel < 0 || nrel > n ||
        (uint32_t)(recipe - 1 + _PyIR_RecipeSize(r)) > ir->nrecipes)
    {
        return 0;
    }
    int cyclic = 0;
    for (int c = 0; c < ncycles; c++) {
        int len = r[2 + 2 * n + c];
        if (len < 2 || len > n - cyclic) {
            return 0;
        }
        cyclic += len;
    }
    int npath = n - cyclic;
    // Each cycle's homes are its sources in turn
    for (int c = 0, first = npath; c < ncycles; first += r[2 + 2 * n + c++]) {
        int len = r[2 + 2 * n + c];
        for (int i = 0; i < len; i++) {
            int next = first + (i + 1) % len;
            if (r[2 + 2 * (first + i)] != r[1 + 2 * next]) {
                return 0;
            }
        }
    }
    for (int q = 0; q < n; q++) {
        int src = r[1 + 2 * q], home = r[2 + 2 * q];
        if (src < nlocals || src >= (int)ir->nslots ||
            home < nlocals || home >= floor || src == home)
        {
            return 0;
        }
        for (int p = 0; p < q; p++) {
            if (r[2 + 2 * p] == home || r[1 + 2 * p] == src) {
                return 0;
            }
        }
    }
    for (int k = 0; k < nrel; k++) {
        int slot = r[3 + 2 * n + ncycles + k];
        if (slot < nlocals || slot >= (int)ir->nslots) {
            return 0;
        }
    }
    return 1;
}

static inline int
_PyIR_ExceptionTableValid(const _PyCodeIR *ir, const unsigned char *cursor,
                          const unsigned char *end,
                          int nwords, int nlocals, int nslots)
{
    int previous_end = 0;
    while (cursor < end) {
        _PyIRExceptionEntry entry;
        if (_PyIR_ExceptionNext(&cursor, end, &entry) < 0 ||
            entry.start < previous_end || entry.end <= entry.start ||
            entry.end > nwords || entry.target >= nwords ||
            entry.slot - entry.lasti < nlocals || entry.slot >= nslots ||
            !_PyIR_RecipeValid(ir, entry.recipe, nlocals,
                               entry.slot - entry.lasti))
        {
            return 0;
        }
        previous_end = entry.end;
    }
    return 1;
}
#endif
