#ifndef Py_INTERNAL_IR_SOURCE_H
#define Py_INTERNAL_IR_SOURCE_H
#ifdef __cplusplus
extern "C" {
#endif
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_ir_exception.h"

/* Optional positions have dense IDs matching semantic recovery rows. */
#define _PyIR_POSITIONS_HEADER 16
#define _PyIR_POSITION_FIELDS 4
#define _PyIR_POSITION_RECORD (_PyIR_POSITION_FIELDS * 4)

typedef struct {
    const unsigned char *locations;
    uint32_t locations_size;
    const unsigned char *records;
    uint32_t nrecords;
} _PyIRPositions;

typedef struct {
    int32_t line, end_line, column, end_column;
} _PyIRPosition;
static_assert(sizeof(_PyIRPosition) == _PyIR_POSITION_RECORD,
              "position serialization must not contain padding");

/* 0: raw optional constructor locations; 1: normalized IRSM2; -1: malformed.
 * Old IRSM recovery semantics are never imported from this optional field. */
static inline int
_PyIRPositions_Decode(const char *bytes, Py_ssize_t size, _PyIRPositions *map)
{
    const unsigned char *p = (const unsigned char *)bytes;
    if (size < 0 || (uint64_t)size > UINT32_MAX) {
        return -1;
    }
    if (size < 4 || memcmp(p, "IRSM", 4) != 0) {
        *map = (_PyIRPositions){p, (uint32_t)size, NULL, 0};
        return 0;
    }
    if (size < _PyIR_POSITIONS_HEADER || _PyIRSemantic_Read32(p + 4) != 2) {
        return -1;
    }
    uint32_t nlocations = _PyIRSemantic_Read32(p + 8);
    uint32_t nrecords = _PyIRSemantic_Read32(p + 12);
    uint64_t expected = _PyIR_POSITIONS_HEADER + (uint64_t)nlocations +
                        (uint64_t)nrecords * _PyIR_POSITION_RECORD;
    if (expected != (uint64_t)size) {
        return -1;
    }
    *map = (_PyIRPositions){p + _PyIR_POSITIONS_HEADER, nlocations,
                           p + _PyIR_POSITIONS_HEADER + nlocations, nrecords};
    return 1;
}

static inline _PyIRPositions
_PyIRPositions_Stored(PyObject *linetable)
{
    _PyIRPositions map;
    if (_PyIRPositions_Decode(PyBytes_AS_STRING(linetable),
                             PyBytes_GET_SIZE(linetable), &map) != 1)
    {
        Py_FatalError("invalid normalized IR positions");
    }
    return map;
}

static inline void
_PyIRPositions_Event(const _PyIRPositions *map, uint32_t index,
                     _PyIRPosition *event)
{
    assert(index < map->nrecords);
    const unsigned char *p = map->records + (size_t)index * _PyIR_POSITION_RECORD;
    int32_t values[_PyIR_POSITION_FIELDS];
    for (int i = 0; i < _PyIR_POSITION_FIELDS; i++) {
        values[i] = (int32_t)_PyIRSemantic_Read32(p + i * 4);
    }
    memcpy(event, values, sizeof(*event));
}

static inline void
_PyIRPositions_EncodeEvent(unsigned char *p, const _PyIRPosition *event)
{
    int32_t values[_PyIR_POSITION_FIELDS];
    memcpy(values, event, sizeof(*event));
    for (int i = 0; i < _PyIR_POSITION_FIELDS; i++) {
        _PyIRSemantic_Write32(p + i * 4, (uint32_t)values[i]);
    }
}

static inline PyObject *
_PyIRPositions_Pack(const unsigned char *locations, uint32_t locations_size,
                    const unsigned char *records, uint32_t nrecords)
{
    uint64_t size = _PyIR_POSITIONS_HEADER + (uint64_t)locations_size +
                    (uint64_t)nrecords * _PyIR_POSITION_RECORD;
    if (size > PY_SSIZE_T_MAX || size > UINT32_MAX) {
        return PyErr_NoMemory();
    }
    PyObject *result = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)size);
    if (result == NULL) {
        return NULL;
    }
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(result);
    memcpy(p, "IRSM", 4);
    _PyIRSemantic_Write32(p + 4, 2);
    _PyIRSemantic_Write32(p + 8, locations_size);
    _PyIRSemantic_Write32(p + 12, nrecords);
    if (locations_size) {
        memcpy(p + _PyIR_POSITIONS_HEADER, locations, locations_size);
    }
    if (nrecords) {
        memcpy(p + _PyIR_POSITIONS_HEADER + locations_size, records,
               (size_t)nrecords * _PyIR_POSITION_RECORD);
    }
    return result;
}

/* This join is a temporary C view, never a code-owned side allocation. Missing
 * positions suppress location delivery, not recovery or owner obligations. */
typedef struct {
    _PyIRSemanticMap semantic;
    _PyIRPositions positions;
    uint32_t nrecords;
} _PyIRObservationMap;

typedef struct {
    int32_t from, to, edge;
    int32_t line, end_line, column, end_column;
    int32_t handler, exc_slot, lasti;
} _PyIRObservationEvent;

static inline _PyIRObservationMap
_PyIRObservation_Stored(PyCodeObject *code)
{
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(code->co_exceptiontable);
    _PyIRPositions positions = _PyIRPositions_Stored(code->co_linetable);
    assert(!positions.nrecords || positions.nrecords == semantic.nrecords);
    return (_PyIRObservationMap){semantic, positions, semantic.nrecords};
}

static inline void
_PyIRObservation_Event(const _PyIRObservationMap *map, uint32_t index,
                       _PyIRObservationEvent *event)
{
    _PyIRRecoveryEvent recovery;
    _PyIRSemantic_Event(&map->semantic, index, &recovery);
    _PyIRPosition position = {-1, -1, -1, -1};
    if (map->positions.nrecords) {
        _PyIRPositions_Event(&map->positions, index, &position);
    }
    *event = (_PyIRObservationEvent){
        recovery.from, recovery.to, recovery.edge,
        position.line, position.end_line, position.column, position.end_column,
        recovery.handler, recovery.exc_slot, recovery.lasti,
    };
}

#ifdef __cplusplus
}
#endif
#endif
