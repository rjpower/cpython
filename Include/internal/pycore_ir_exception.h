#ifndef Py_INTERNAL_IR_EXCEPTION_H
#define Py_INTERNAL_IR_EXCEPTION_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Semantic recovery is independent of optional line/column information.
 * The shared empty bytes object is the canonical no-EH/no-recovery case. */
#define _PyIR_SEMANTIC_HEADER 16
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

/* There is no unversioned nonempty semantic representation. */
static inline int
_PyIRSemantic_Decode(const char *bytes, Py_ssize_t size, _PyIRSemanticMap *map)
{
    const unsigned char *p = (const unsigned char *)bytes;
    if (size == 0) {
        *map = (_PyIRSemanticMap){p, 0, NULL, 0};
        return 1;
    }
    if (size < _PyIR_SEMANTIC_HEADER || (uint64_t)size > UINT32_MAX ||
        memcmp(p, "IRSC", 4) != 0 || _PyIRSemantic_Read32(p + 4) != 1)
    {
        return -1;
    }
    uint32_t nexceptions = _PyIRSemantic_Read32(p + 8);
    uint32_t nrecords = _PyIRSemantic_Read32(p + 12);
    uint64_t expected = _PyIR_SEMANTIC_HEADER + (uint64_t)nexceptions +
                        (uint64_t)nrecords * _PyIR_RECOVERY_RECORD;
    if (expected != (uint64_t)size || (!nexceptions && !nrecords)) {
        return -1;
    }
    *map = (_PyIRSemanticMap){p + _PyIR_SEMANTIC_HEADER, nexceptions,
                             p + _PyIR_SEMANTIC_HEADER + nexceptions, nrecords};
    return 1;
}

static inline _PyIRSemanticMap
_PyIRSemantic_Stored(PyObject *exceptiontable)
{
    _PyIRSemanticMap map;
    if (_PyIRSemantic_Decode(PyBytes_AS_STRING(exceptiontable),
                            PyBytes_GET_SIZE(exceptiontable), &map) != 1)
    {
        Py_FatalError("invalid normalized IR semantic map");
    }
    return map;
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

static inline PyObject *
_PyIRSemantic_Pack(const unsigned char *exceptions, uint32_t exceptions_size,
                   const unsigned char *records, uint32_t nrecords)
{
    if (!exceptions_size && !nrecords) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    uint64_t size = _PyIR_SEMANTIC_HEADER + (uint64_t)exceptions_size +
                    (uint64_t)nrecords * _PyIR_RECOVERY_RECORD;
    if (size > PY_SSIZE_T_MAX || size > UINT32_MAX) {
        return PyErr_NoMemory();
    }
    PyObject *result = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)size);
    if (result == NULL) {
        return NULL;
    }
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(result);
    memcpy(p, "IRSC", 4);
    _PyIRSemantic_Write32(p + 4, 1);
    _PyIRSemantic_Write32(p + 8, exceptions_size);
    _PyIRSemantic_Write32(p + 12, nrecords);
    if (exceptions_size) {
        memcpy(p + _PyIR_SEMANTIC_HEADER, exceptions, exceptions_size);
    }
    if (nrecords) {
        memcpy(p + _PyIR_SEMANTIC_HEADER + exceptions_size, records,
               (size_t)nrecords * _PyIR_RECOVERY_RECORD);
    }
    return result;
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
