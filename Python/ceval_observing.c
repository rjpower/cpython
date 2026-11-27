/* The observing evaluator: the evaluator (Python/ceval_evaluator.h)
 * compiled again, with IR_OBSERVING set, over the observing dispatch table
 * (Python/ir_targets.h), which sends the words of every family that
 * declares events to the family's observer.  The code of an observed code
 * object (_PyCode_OBSERVED_MARK) runs here; the normal evaluator,
 * Python/ceval_normal.c, has no monitoring in its table.  A frame moves
 * between the two at the switch word, entering this one at
 * _PyEval_EnterObserving().
 *
 * This unit holds only the evaluator's functions, so that the build can
 * compile them without frames of their own (CFLAGS_EVALUATOR,
 * Python/ceval_tail.h). */

#define IR_OBSERVING 1
#include "ceval_shared.h"
#include "ceval_evaluator.h"
