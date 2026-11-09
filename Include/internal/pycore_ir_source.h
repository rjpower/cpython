#ifndef Py_INTERNAL_IR_SOURCE_H
#define Py_INTERNAL_IR_SOURCE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* co_linetable is one versioned container: compact-PC locations followed by
 * source-only edge records. Only observation reads the edge section. Integers
 * are little endian so marshal does not depend on native struct layout. */
#define _PyIR_SOURCE_HEADER 16
#define _PyIR_SOURCE_FIELDS 10
#define _PyIR_SOURCE_RECORD (_PyIR_SOURCE_FIELDS * 4)
#define _PyIR_SOURCE_FALLTHROUGH 0
#define _PyIR_SOURCE_TAKEN 1

typedef struct {
    const unsigned char *locations;
    uint32_t locations_size;
    const unsigned char *records;
    uint32_t nrecords;
} _PyIRSourceMap;

typedef struct {
    int32_t from, to, edge;
    int32_t line, end_line, column, end_column;
    int32_t handler, exc_slot, lasti;
} _PyIRSourceEvent;
static_assert(sizeof(_PyIRSourceEvent) == _PyIR_SOURCE_RECORD,
              "source-event serialization must not contain padding");

static inline uint32_t
_PyIRSource_Read32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void
_PyIRSource_Write32(unsigned char *p, uint32_t value)
{
    for (int i = 0; i < 4; i++) {
        p[i] = (unsigned char)(value >> (8 * i));
    }
}

/* Return -1 for a malformed container, 0 for a raw constructor location
 * table, and 1 for the normalized stored representation. */
static inline int
_PyIRSource_Decode(const char *bytes, Py_ssize_t size, _PyIRSourceMap *map)
{
    const unsigned char *p = (const unsigned char *)bytes;
    if (size < 0 || (uint64_t)size > UINT32_MAX) {
        return -1;
    }
    if (size < 4 || memcmp(p, "IRSM", 4) != 0) {
        *map = (_PyIRSourceMap){p, (uint32_t)size, NULL, 0};
        return 0;
    }
    if (size < _PyIR_SOURCE_HEADER || _PyIRSource_Read32(p + 4) != 1) {
        return -1;
    }
    uint32_t nlocations = _PyIRSource_Read32(p + 8);
    uint32_t nrecords = _PyIRSource_Read32(p + 12);
    uint64_t expected = _PyIR_SOURCE_HEADER + (uint64_t)nlocations +
                        (uint64_t)nrecords * _PyIR_SOURCE_RECORD;
    if (expected != (uint64_t)size) {
        return -1;
    }
    *map = (_PyIRSourceMap){p + _PyIR_SOURCE_HEADER, nlocations,
                           p + _PyIR_SOURCE_HEADER + nlocations, nrecords};
    return 1;
}

/* Stored code objects have one validated representation. Internal readers
 * must not continue with an uninitialized view if that invariant is broken. */
static inline _PyIRSourceMap
_PyIRSource_Stored(PyObject *linetable)
{
    _PyIRSourceMap map;
    if (_PyIRSource_Decode(PyBytes_AS_STRING(linetable),
                          PyBytes_GET_SIZE(linetable), &map) != 1)
    {
        Py_FatalError("invalid normalized IR source map");
    }
    return map;
}

static inline void
_PyIRSource_Event(const _PyIRSourceMap *map, uint32_t index,
                  _PyIRSourceEvent *event)
{
    assert(index < map->nrecords);
    const unsigned char *p = map->records + index * _PyIR_SOURCE_RECORD;
    int32_t values[_PyIR_SOURCE_FIELDS];
    for (int i = 0; i < _PyIR_SOURCE_FIELDS; i++) {
        values[i] = (int32_t)_PyIRSource_Read32(p + i * 4);
    }
    memcpy(event, values, sizeof(*event));
}

static inline void
_PyIRSource_EncodeEvent(unsigned char *p, const _PyIRSourceEvent *event)
{
    int32_t values[_PyIR_SOURCE_FIELDS];
    memcpy(values, event, sizeof(*event));
    for (int i = 0; i < _PyIR_SOURCE_FIELDS; i++) {
        _PyIRSource_Write32(p + i * 4, (uint32_t)values[i]);
    }
}

static inline PyObject *
_PyIRSource_Pack(const unsigned char *locations, uint32_t locations_size,
                  const unsigned char *records, uint32_t nrecords)
{
    uint64_t size = _PyIR_SOURCE_HEADER + (uint64_t)locations_size +
                    (uint64_t)nrecords * _PyIR_SOURCE_RECORD;
    if (size > PY_SSIZE_T_MAX) {
        return PyErr_NoMemory();
    }
    PyObject *result = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)size);
    if (result == NULL) {
        return NULL;
    }
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(result);
    memcpy(p, "IRSM", 4);
    _PyIRSource_Write32(p + 4, 1);
    _PyIRSource_Write32(p + 8, locations_size);
    _PyIRSource_Write32(p + 12, nrecords);
    if (locations_size) {
        memcpy(p + _PyIR_SOURCE_HEADER, locations, locations_size);
    }
    if (nrecords) {
        memcpy(p + _PyIR_SOURCE_HEADER + locations_size, records,
               (size_t)nrecords * _PyIR_SOURCE_RECORD);
    }
    return result;
}

#ifdef __cplusplus
}
#endif
#endif
