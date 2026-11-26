/* The observing evaluator: the body of the evaluator
 * (Python/ceval_evaluator.h) compiled again, with IR_OBSERVING set, over the
 * observing dispatch table (Python/ir_targets.h), which sends the words of
 * every family that declares events to the family's observer.  The code of
 * an observed code object (_PyCode_OBSERVED_MARK) runs here; the normal
 * evaluator, ir_evaluate() in Python/ceval.c, has no monitoring in its
 * table.  A frame moves between the two at the switch word. */

#define IR_OBSERVING 1
#include "ceval_shared.h"

#ifdef _PY_JIT_SUPPORTED
_PyIRTransfer _PyEval_JitRun(PyThreadState *tstate, _PyJitUnit *unit,
                             _PyInterpreterFrame *frame, _PyInstr *next);
_PyIRTransfer _PyEval_JitResume(PyThreadState *tstate,
                                _PyInterpreterFrame *frame, _PyInstr *next);
#define jit_run _PyEval_JitRun
#define jit_resume _PyEval_JitResume
#endif

PyValue
_PyEval_EvalObserving(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
#include "ceval_evaluator.h"
}
