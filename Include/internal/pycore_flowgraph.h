#ifndef Py_INTERNAL_CFG_H
#define Py_INTERNAL_CFG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_compile.h"
#include "pycore_instruction_sequence.h"
#include "pycore_opcode_utils.h"

struct _PyCfgBuilder;

int _PyCfgBuilder_UseLabel(struct _PyCfgBuilder *g, _PyJumpTargetLabel lbl);
int _PyCfgBuilder_Addop(struct _PyCfgBuilder *g, int opcode, int oparg, _Py_SourceLocation loc);

struct _PyCfgBuilder* _PyCfgBuilder_New(void);
void _PyCfgBuilder_Free(struct _PyCfgBuilder *g);
int _PyCfgBuilder_CheckSize(struct _PyCfgBuilder* g);

int _PyCfg_OptimizeCodeUnit(struct _PyCfgBuilder *g, PyObject *consts, PyObject *const_cache,
                            int nlocals, int nparams, int firstlineno);

struct _PyCfgBuilder* _PyCfg_FromInstructionSequence(_PyInstructionSequence *seq);
int _PyCfg_ToInstructionSequence(struct _PyCfgBuilder *g, _PyInstructionSequence *seq);
int _PyCfg_OptimizedCfgToInstructionSequence(struct _PyCfgBuilder *g, _PyCompile_CodeUnitMetadata *umd,
                                             int *stackdepth, int *nlocalsplus,
                                             _PyInstructionSequence *seq);

/* Slot assignment (Python/regalloc.c) */

/* The stack depth before each instruction of instrs, whose jump targets are
 * instruction indices, or -1 for an instruction that no path reaches. */
int _PyRegAlloc_Depths(_PyInstructionSequence *instrs, int *depths);

/* The slot of each role of a word */
typedef struct {
    int dst;
    int base;           // the deepest input of a window-style instruction
    int in[4];          // the inputs of a register-style one, bottom first
} _PyRegSlots;

/* The slots of an instruction of format fmt that pops popped values, with
 * depth values on the stack before it. */
struct _PyIRFormat;
void _PyRegAlloc_Slots(const struct _PyIRFormat *fmt, int nlocalsplus,
                       int depth, int popped, _PyRegSlots *slots);

PyCodeObject *
_PyAssemble_MakeCodeObject(_PyCompile_CodeUnitMetadata *u, PyObject *const_cache,
                           PyObject *consts, int maxdepth, _PyInstructionSequence *instrs,
                           int nlocalsplus, int code_flags, PyObject *filename);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(PyObject*) _PyCompile_OptimizeCfg(
        PyObject *instructions,
        PyObject *consts,
        int nlocals);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CFG_H */
