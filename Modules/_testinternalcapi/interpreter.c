#ifndef Py_BUILD_CORE
#define Py_BUILD_CORE
#endif
#ifndef Py_BUILD_CORE_MODULE
#define Py_BUILD_CORE_MODULE
#endif

#include "Python.h"

/* Test_EvalFrame, the evaluator that set_eval_frame_interp() installs, runs
 * the default interpreter. */

#include "pycore_ceval.h"         // _PyEval_EvalFrameDefault()

int Test_EvalFrame_Resumes, Test_EvalFrame_Loads;

PyObject *
Test_EvalFrame(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
{
    return _PyEval_EvalFrameDefault(tstate, frame, throwflag);
}
