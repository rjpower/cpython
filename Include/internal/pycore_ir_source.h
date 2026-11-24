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
#define _PyIR_POSITION_FIELDS 5
#define _PyIR_POSITION_RECORD (_PyIR_POSITION_FIELDS * 4)

typedef struct {
    const unsigned char *records;
    uint32_t nrecords;
} _PyIRPositions;

/* offset is the code-unit offset in co_code of the stack instruction that the
 * source-only operation stands for, the f_lasti of its line event. */
typedef struct {
    int32_t line, end_line, column, end_column, offset;
} _PyIRPosition;
static_assert(sizeof(_PyIRPosition) == _PyIR_POSITION_RECORD,
              "position serialization must not contain padding");

static inline _PyIRPositions
_PyIRPositions_Stored(PyCodeObject *code)
{
    const _PyCodeIR *ir = _PyCode_IR(code);
    return (_PyIRPositions){ir->positions, ir->nrecords};
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

/* This join is a temporary C view, never a code-owned side allocation. Missing
 * positions suppress location delivery, not recovery or owner obligations. */
typedef struct {
    _PyIRSemanticMap semantic;
    _PyIRPositions positions;
    uint32_t nrecords;
} _PyIRObservationMap;

typedef struct {
    int32_t from, to, edge;
    int32_t line, end_line, column, end_column, offset;
    int32_t handler, exc_slot, lasti, recipe;
} _PyIRObservationEvent;

static inline _PyIRObservationMap
_PyIRObservation_Stored(PyCodeObject *code)
{
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(code);
    _PyIRPositions positions = _PyIRPositions_Stored(code);
    assert(!positions.nrecords || positions.nrecords == semantic.nrecords);
    return (_PyIRObservationMap){semantic, positions, semantic.nrecords};
}

static inline void
_PyIRObservation_Event(const _PyIRObservationMap *map, uint32_t index,
                       _PyIRObservationEvent *event)
{
    _PyIRRecoveryEvent recovery;
    _PyIRSemantic_Event(&map->semantic, index, &recovery);
    _PyIRPosition position = {-1, -1, -1, -1, -1};
    if (map->positions.nrecords) {
        _PyIRPositions_Event(&map->positions, index, &position);
    }
    *event = (_PyIRObservationEvent){
        recovery.from, recovery.to, recovery.edge,
        position.line, position.end_line, position.column, position.end_column,
        position.offset,
        recovery.handler, recovery.exc_slot, recovery.lasti, recovery.recipe,
    };
}

#ifdef __cplusplus
}
#endif
#endif
