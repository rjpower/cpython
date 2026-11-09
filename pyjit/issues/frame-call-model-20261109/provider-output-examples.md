## INSTRUMENTED_JUMP_FORWARD

Before:
```c
TARGET(INSTRUMENTED_JUMP_FORWARD) {
            int opcode = INSTRUMENTED_JUMP_FORWARD;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_JUMP_FORWARD);
            int oparg = (int)IR_CD(ir_word);
            (void)oparg;
            INSTRUMENTED_JUMP(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_JUMP);
            DISPATCH();
        }
```

After:
```c
TARGET(INSTRUMENTED_JUMP_FORWARD) {
            int opcode = INSTRUMENTED_JUMP_FORWARD;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_JUMP_FORWARD);
            int oparg = (int)IR_CD(ir_word);
            (void)oparg;
            {
                PyValue ir_observer_deferred = PyValue_NULL;
                _PyInstr *ir_observer_next = JUMP_TARGET ( oparg );
                frame->return_pc = ir_observer_next;
                _PyIRObserverResult ir_observer_result = _PyIR_ObserveCommittedBranch(
                tstate, frame, this_instr, this_instr, ir_observer_next, PY_MONITORING_EVENT_JUMP, 1);
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

## INSTRUMENTED_JUMP_BACKWARD

Before:
```c
TARGET(INSTRUMENTED_JUMP_BACKWARD) {
            int opcode = INSTRUMENTED_JUMP_BACKWARD;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_JUMP_BACKWARD);
            int oparg = (int)IR_CD(ir_word);
            (void)oparg;
            // _CHECK_PERIODIC
            {
                int err = check_periodics(tstate);
                if (err != 0) {
                    JUMP_TO_LABEL(error);
                }
            }
            // _MONITOR_JUMP_BACKWARD
            {
                INSTRUMENTED_JUMP(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_JUMP);
            }
            DISPATCH();
        }
```

After:
```c
TARGET(INSTRUMENTED_JUMP_BACKWARD) {
            int opcode = INSTRUMENTED_JUMP_BACKWARD;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_JUMP_BACKWARD);
            int oparg = (int)IR_CD(ir_word);
            (void)oparg;
            // _CHECK_PERIODIC
            {
                int err = check_periodics(tstate);
                if (err != 0) {
                    JUMP_TO_LABEL(error);
                }
            }
            // _MONITOR_JUMP_BACKWARD
            {
                {
                    PyValue ir_observer_deferred = PyValue_NULL;
                    _PyInstr *ir_observer_next = JUMP_TARGET ( oparg );
                    frame->return_pc = ir_observer_next;
                    _PyIRObserverResult ir_observer_result = _PyIR_ObserveCommittedBranch(
                    tstate, frame, this_instr, this_instr, ir_observer_next, PY_MONITORING_EVENT_JUMP, 1);
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
        }
```

## INSTRUMENTED_POP_ITER

Before:
```c
TARGET(INSTRUMENTED_POP_ITER) {
            int opcode = INSTRUMENTED_POP_ITER;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const prev_instr = frame->instr_ptr;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_POP_ITER);
            PyValueBorrow iter;
            PyValueBorrow index_or_null;
            PyValue ir_own_A = PyValue_NULL;
            (void)ir_own_A;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            index_or_null = IR_VIEW(IR_B(ir_word));
            iter = IR_VIEW(IR_A(ir_word));
            (void)index_or_null;
            ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), index_or_null);
            INSTRUMENTED_JUMP(prev_instr, this_instr+1, PY_MONITORING_EVENT_BRANCH_RIGHT);
            ir_own_A = IR_CONSUME_INPUT(IR_A(ir_word), iter);
            PyValue_XCLOSE(ir_own_A);
            DISPATCH();
        }
```

After:
```c
TARGET(INSTRUMENTED_POP_ITER) {
            int opcode = INSTRUMENTED_POP_ITER;
            (void)(opcode);
            _PyValueTemps *const value_temps = NULL;
            (void)value_temps;
            _PyIRWord* const prev_instr = frame->instr_ptr;
            _PyIRWord* const this_instr = next_instr;
            (void)this_instr;
            frame->instr_ptr = next_instr;
            next_instr += 1;
            INSTRUCTION_STATS(INSTRUMENTED_POP_ITER);
            PyValueBorrow iter;
            PyValueBorrow index_or_null;
            PyValue ir_own_A = PyValue_NULL;
            (void)ir_own_A;
            PyValue ir_own_B = PyValue_NULL;
            (void)ir_own_B;
            index_or_null = IR_VIEW(IR_B(ir_word));
            iter = IR_VIEW(IR_A(ir_word));
            ir_own_A = IR_CONSUME_INPUT(IR_A(ir_word), iter);
            PyValue held = IR_TAKE(iter, ir_own_A);
            (void)index_or_null;
            ir_own_B = IR_CONSUME_INPUT(IR_B(ir_word), index_or_null);
            PyValue ir_observer_input_0 = held;
            held = PyValue_NULL;
            {
                PyValue ir_observer_deferred = PyValue_NULL;
                _PyInstr *ir_observer_next = next_instr;
                frame->return_pc = ir_observer_next;
                _PyIRObserverResult ir_observer_result = _PyIR_ObserveCommittedBranch(
                tstate, frame, this_instr, prev_instr, ir_observer_next, PY_MONITORING_EVENT_BRANCH_RIGHT, 0);
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
```
