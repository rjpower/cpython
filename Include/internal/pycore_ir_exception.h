#ifndef Py_INTERNAL_IR_EXCEPTION_H
#define Py_INTERNAL_IR_EXCEPTION_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

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
