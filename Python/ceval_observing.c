/* The observing evaluator: the evaluator (Python/ceval_evaluator.h)
 * compiled again, with IR_OBSERVING set, over the observing dispatch table
 * (Python/ir_targets.h), which sends the words of every family that
 * declares events to the family's observer.  The code of an observed code
 * object (_PyCode_OBSERVED_MARK) runs here; the normal evaluator, in
 * Python/ceval.c, has no monitoring in its table.  A frame moves between the
 * two at the switch word, entering this one at _PyEval_EnterObserving(). */

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

#include "ceval_evaluator.h"
