#ifndef Py_INTERNAL_JIT_IR_H
#define Py_INTERNAL_JIT_IR_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif
#include "pycore_jit.h"

#define JIT_RECORD_EVENTS 512
#define JIT_GRAPH_VALUES 1024
#define JIT_GRAPH_NODES 2048
#define JIT_GRAPH_STATES 256
#define JIT_GRAPH_MAPS 4096
#define JIT_GRAPH_HOMES 64
#define JIT_GRAPH_SPILLS 64
#define JIT_NO_VALUE UINT16_MAX
#define JIT_NULL_VALUE (UINT16_MAX - 1)

/* A recorded home: NULL, an inline int (the only scalar a trace imports; an
 * int outside the inline range is a box and opaque), or anything else. */
typedef enum { JIT_HOME_OPAQUE, JIT_HOME_NULL, JIT_HOME_INLINE } JitHomeKind;
typedef struct { uint8_t kind; } JitHome;
typedef struct {
    uint32_t word, successor;
    _PyJitDecodedOp decoded; /* instruction is NULL: no exec borrow survives */
    _PyJitHomeEffect effect;
    uint8_t action, phase;
    JitHome before[JIT_GRAPH_HOMES], after[JIT_GRAPH_HOMES];
} JitRecordEvent;
struct _PyJitRecord {
    _PyInterpreterFrame *frame; /* linked-frame identity, not an owner */
    uint64_t source_stamp, generation, pending_token;
    uint32_t header, backedge, count, pending_word;
    uint16_t nhomes, nlocals;
    uint8_t paused, open, closed, aborted;
    JitRecordEvent events[JIT_RECORD_EVENTS];
};

typedef enum {
    JIT_SSA_IMPORT, JIT_SSA_CONST, JIT_SSA_PHI,
    JIT_SSA_ADD, JIT_SSA_SUB, JIT_SSA_MUL, JIT_SSA_AND,
    JIT_SSA_HELPER, JIT_SSA_POLL, JIT_SSA_COMMIT, JIT_SSA_CMP, JIT_SSA_DEAD
} JitNodeKind;
typedef struct {
    uint16_t value[JIT_GRAPH_HOMES];
    uint32_t word;
    uint16_t effect;
} JitFrameState;
typedef struct {
    uint32_t word, successor;
    uint16_t left, right, value, before, after, effect;
    int64_t immediate; /* CONST: the int51 value, unscaled; CMP: oparg and branch */
    int16_t home;
    uint8_t kind, clears; /* bit k consumes input k's temporary */
    uint16_t input_home[2];
} JitSsaNode;
typedef struct {
    uint16_t first, last;
    int16_t reg, spill;
    int16_t import_home;
} JitSsaValue;
struct _PyJitGraph {
    uint16_t nnodes, nvalues, nstates, nspills, nhomes, nlocals, nimports, nscratch;
    uint32_t header, backedge;
    uint16_t imports[JIT_GRAPH_HOMES], carried[JIT_GRAPH_HOMES];
    JitSsaNode nodes[JIT_GRAPH_NODES];
    JitSsaValue values[JIT_GRAPH_VALUES];
    JitFrameState states[JIT_GRAPH_STATES];
    uint32_t map_entries;
    size_t charged_bytes;
    uint16_t event_count, python_calls, optimized_values;
    uint32_t record_words[JIT_RECORD_EVENTS], record_successors[JIT_RECORD_EVENTS];
};

extern _PyJitAction _PyJit_RecordBefore(_PyJitContext *, const _PyJitLirOp *);
extern _PyJitAction _PyJit_RecordAfter(_PyJitContext *, _PyJitAction, const _PyJitLirOp *);
extern void _PyJit_RecordPending(_PyJitFlow *, const _PyJitPending *);
extern void _PyJit_RecordReturn(_PyJitFlow *, _PyInterpreterFrame *, _PyIRWord *, uint64_t);
extern void _PyJit_RecordRetire(_PyJitFlow *, _PyInterpreterFrame *);
extern void _PyJit_FreeRecord(_PyJitRecord *);
extern _PyJitGraph *_PyJit_BuildGraph(const _PyJitRecord *);
extern int _PyJit_VerifyGraph(const _PyJitGraph *);
extern void _PyJit_OptimizeGraph(_PyJitGraph *);
extern int _PyJit_AllocateGraph(_PyJitGraph *);
extern void _PyJit_FreeGraph(_PyJitGraph *);
extern int _PyJit_LowerGraph(_PyJitCodeUnit *);
extern _PyJitAction _PyJit_TraceImport(_PyJitContext *, const JitSsaNode *);
extern _PyJitAction _PyJit_TracePublish(_PyJitContext *, const JitFrameState *);
extern _PyJitAction _PyJit_TraceCommit(_PyJitContext *, const JitSsaNode *);
extern _PyJitAction _PyJit_TraceFallback(_PyJitContext *, uint32_t);
extern _PyJitAction _PyJit_TraceAfter(_PyJitContext *, _PyJitAction, const JitSsaNode *);
extern void _PyJit_TraceEntered(_PyJitContext *);
PyAPI_FUNC(PyObject *) _PyJit_TestRecordIndex(void);
#endif
