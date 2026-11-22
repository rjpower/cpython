#ifndef Py_INTERNAL_IR_EXCEPTION_H
#define Py_INTERNAL_IR_EXCEPTION_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir.h"          // _PyCodeIR

/* Semantic recovery is independent of optional line/column information.
 * The exceptions and the recovery rows are arrays of the code's _PyCodeIR. */
#define _PyIR_RECOVERY_FIELDS 6
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
 * runtime lookup. Geometry is passed explicitly for code construction. */
typedef struct {
    int start, end, target, slot, lasti;
} _PyIRExceptionEntry;

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
_PyIR_ExceptionNext(const unsigned char **cursor, const unsigned char *end,
                    _PyIRExceptionEntry *entry)
{
    int fields[4];
    for (int f = 0; f < 4; f++) {
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
                                   fields[2], fields[3] >> 1, fields[3] & 1};
    return 0;
}

static inline int
_PyIR_ExceptionTableValid(const unsigned char *cursor, const unsigned char *end,
                          int nwords, int nlocals, int nslots)
{
    int previous_end = 0;
    while (cursor < end) {
        _PyIRExceptionEntry entry;
        if (_PyIR_ExceptionNext(&cursor, end, &entry) < 0 ||
            entry.start < previous_end || entry.end <= entry.start ||
            entry.end > nwords || entry.target >= nwords ||
            entry.slot - entry.lasti < nlocals || entry.slot >= nslots)
        {
            return 0;
        }
        previous_end = entry.end;
    }
    return 1;
}
#endif
