## INSTRUMENTED_FOR_ITER

Before:
```c
TARGET(INSTRUMENTED_FOR_ITER) {
            int opcode = INSTRUMENTED_FOR_ITER;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_FOR_ITER);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            int oparg = IR_DS(ir_word);
            (void)oparg;
            PyValue *window = &IR_SLOT(IR_B(ir_word));
            (void)window;
            PyValue iter;
            PyValue null_or_index;
            PyValue next;
            null_or_index = window[1];
            iter = window[0];
            _PyStatusValue step = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &window[1]);
            null_or_index = window[1];
            PyValue item = step.value;
            int found = step.status;
            if (found <= 0) {
                if (found < 0) {
                    IR_PUBLISH_TEMP(window[1], null_or_index);
                    JUMP_TO_LABEL(error);
                }
                PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;{
                    _PyIRStepResult ir_step_result = _PyEval_CompleteStep(tstate, frame, this_instr, IR_B(ir_word), IR_A(ir_word), next_instr + oparg, completed, _PY_IR_FOR_ITER);
                    next_instr = ir_step_result.next;
                    if (ir_step_result.status < 0) { JUMP_TO_LABEL(error); }
                    DISPATCH();
                }
            }
            next = item;
            INSTRUMENTED_JUMP(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT);
            IR_PUBLISH_TEMP(window[1], null_or_index);
            IR_PUBLISH_TEMP(window[2], next);
            IR_MOVE_TO_DST_OR_DISCARD(IR_A(ir_word), window[2]);
            if (!PyValue_IsNull(ir_displaced) &&
                finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) {
                next_instr = frame->instr_ptr + 1;
                JUMP_TO_LABEL(error);
            }
            DISPATCH();
        }
```

After:
```c
TARGET(INSTRUMENTED_FOR_ITER) {
            int opcode = INSTRUMENTED_FOR_ITER;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_FOR_ITER);
            PyValue ir_displaced = PyValue_NULL;
            (void)ir_displaced;
            int oparg = IR_DS(ir_word);
            (void)oparg;
            PyValue *window = &IR_SLOT(IR_B(ir_word));
            (void)window;
            PyValue iter;
            PyValue null_or_index;
            PyValue next;
            null_or_index = window[1];
            iter = window[0];
            _PyStatusValue step = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &window[1]);
            null_or_index = window[1];
            PyValue item = step.value;
            int found = step.status;
            if (found <= 0) {
                if (found < 0) {
                    IR_PUBLISH_TEMP(window[1], null_or_index);
                    JUMP_TO_LABEL(error);
                }
                PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;{
                    _PyIRStepResult ir_step_result = _PyEval_CompleteStep(tstate, frame, this_instr, IR_B(ir_word), IR_A(ir_word), next_instr + oparg, completed, _PY_IR_FOR_ITER);
                    next_instr = ir_step_result.next;
                    if (ir_step_result.status < 0) { JUMP_TO_LABEL(error); }
                    DISPATCH();
                }
            }
            next = item;{
                PyValue ir_observer_deferred = PyValue_NULL;
                IR_PUBLISH_TEMP(window[1], null_or_index);
                IR_PUBLISH_TEMP(window[2], next);
                IR_MOVE_OBSERVED_DST(IR_A(ir_word), window[2], ir_observer_deferred);
                _PyInstr *ir_observer_next = next_instr;
                frame->return_pc = ir_observer_next;
                _PyIRObserverResult ir_observer_result = _PyIR_ObserveCommittedBranch(
                tstate, frame, this_instr, this_instr, ir_observer_next, PY_MONITORING_EVENT_BRANCH_LEFT, 0);
                PyObject *ir_observer_exception = ir_observer_result.status < 0
                ? PyErr_GetRaisedException() : NULL;
                assert(ir_observer_result.status >= 0 || ir_observer_exception != NULL);
                frame->instr_ptr = ir_observer_result.next;
                PyValue_XCLOSE(ir_observer_deferred);
                _PyValueTemps_Close(NULL);
                if (ir_observer_result.status < 0) {
                    PyErr_SetRaisedException(ir_observer_exception);
                    next_instr = frame->instr_ptr + 1;
                    JUMP_TO_LABEL(error);
                }
                next_instr = frame->instr_ptr;
            DISPATCH();
}
        }
```

## INSTRUMENTED_JUMP_IF_TRUE_OR_POP

Before:
```c
TARGET(INSTRUMENTED_JUMP_IF_TRUE_OR_POP) {
            int opcode = INSTRUMENTED_JUMP_IF_TRUE_OR_POP;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_JUMP_IF_TRUE_OR_POP);
            int oparg = (int)IR_CD(ir_word);
            (void)oparg;
            PyValueBorrow cond;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            cond = IR_VIEW(IR_B(ir_word));
            ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), cond);
            PyValue held = IR_TAKE(cond, ir_own_B);
            int truth = _PyValue_IsTrue(held);
            if (truth < 0) {
                PyValue_CLOSE(held);
                JUMP_TO_LABEL(error);
            }
            if (truth) {
                res = held;
                INSTRUMENTED_JUMP(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT);
            }
            else {
                PyValue_CLOSE(held);
                res = PyValue_NULL;
                INSTRUMENTED_JUMP(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT);
            }
            IR_STORE_TEMP(IR_SLOT(IR_A(ir_word)), res);
            DISPATCH();
        }
```

After:
```c
TARGET(INSTRUMENTED_JUMP_IF_TRUE_OR_POP) {
            int opcode = INSTRUMENTED_JUMP_IF_TRUE_OR_POP;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_JUMP_IF_TRUE_OR_POP);
            int oparg = (int)IR_CD(ir_word);
            (void)oparg;
            PyValueBorrow cond;
            PyValue res;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            cond = IR_VIEW(IR_B(ir_word));
            ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), cond);
            PyValue held = IR_TAKE(cond, ir_own_B);
            int truth = _PyValue_IsTrue(held);
            if (truth < 0) {
                PyValue_CLOSE(held);
                JUMP_TO_LABEL(error);
            }
            if (truth) {
                res = held;{
                    PyValue ir_observer_deferred = PyValue_NULL;
                    IR_STORE_TEMP(IR_SLOT(IR_A(ir_word)), res);
                    _PyInstr *ir_observer_next = JUMP_TARGET ( oparg );
                    frame->return_pc = ir_observer_next;
                    _PyIRObserverResult ir_observer_result = _PyIR_ObserveCommittedBranch(
                    tstate, frame, this_instr, this_instr, ir_observer_next, PY_MONITORING_EVENT_BRANCH_RIGHT, 1);
                    PyObject *ir_observer_exception = ir_observer_result.status < 0
                    ? PyErr_GetRaisedException() : NULL;
                    assert(ir_observer_result.status >= 0 || ir_observer_exception != NULL);
                    frame->instr_ptr = ir_observer_result.next;
                    PyValue_XCLOSE(ir_observer_deferred);
                    _PyValueTemps_Close(NULL);
                    if (ir_observer_result.status < 0) {
                        PyErr_SetRaisedException(ir_observer_exception);
                        next_instr = frame->instr_ptr + 1;
                        JUMP_TO_LABEL(error);
                    }
                    next_instr = frame->instr_ptr;
                DISPATCH();
}
            }
            else {PyValue ir_observer_input_0 = held;
                held = PyValue_NULL;
                res = PyValue_NULL;{
                    PyValue ir_observer_deferred = PyValue_NULL;
                    IR_STORE_TEMP(IR_SLOT(IR_A(ir_word)), res);
                    _PyInstr *ir_observer_next = next_instr;
                    frame->return_pc = ir_observer_next;
                    _PyIRObserverResult ir_observer_result = _PyIR_ObserveCommittedBranch(
                    tstate, frame, this_instr, this_instr, ir_observer_next, PY_MONITORING_EVENT_BRANCH_LEFT, 0);
                    PyObject *ir_observer_exception = ir_observer_result.status < 0
                    ? PyErr_GetRaisedException() : NULL;
                    assert(ir_observer_result.status >= 0 || ir_observer_exception != NULL);
                    frame->instr_ptr = ir_observer_result.next;
                    PyValue_XCLOSE(ir_observer_deferred);
                    PyValue_XCLOSE(ir_observer_input_0);
                    _PyValueTemps_Close(NULL);
                    if (ir_observer_result.status < 0) {
                        PyErr_SetRaisedException(ir_observer_exception);
                        next_instr = frame->instr_ptr + 1;
                        JUMP_TO_LABEL(error);
                    }
                    next_instr = frame->instr_ptr;
                DISPATCH();
}
            }
        }
```
