/* The body of the evaluator, which Python/ceval.c compiles as ir_evaluate()
 * and Python/ceval_observing.c compiles again, with IR_OBSERVING set, as
 * _PyEval_EvalObserving(): the same handlers, labels and routines over the
 * dispatch table of each (Python/ir_targets.h).  The observing table sends
 * the words of the families that declare events to their observers.  A frame
 * runs in the evaluator that its code's observed mark asks for; it moves
 * between the two at the switch word (IR_H_SWITCH, _PyEval_SwitchLoop()).
 *
 * The parameters are tstate, frame and throwflag: 0 starts frame, 1 throws
 * into it, and IR_ENTER_SWITCH goes on at frame->return_pc, where the other
 * evaluator left it.  The frame's instr_ptr stays the word that ran last,
 * which a LINE event compares its word with. */

#include "ir_targets.h"
    _PyInstr *next_instr = frame->instr_ptr;
    _PyIRWord ir_word = 0;
    /* The operands of the shared bodies that the labels below read: the
     * VM routines' (Python/ceval_routines.h), which the transfer macros set.
     * None is live across a dispatch. */
    _PyInstr *process_error_origin;
    uint32_t process_error_entry;
    PyValue gen_value;
    _PyInterpreterFrame *gen_dying;
    int gen_empty;
    _PyIRTransfer gen_transfer;

    if (throwflag) {
        if (throwflag == IR_ENTER_SWITCH) {
            goto LABEL_switch_entry;
        }
        goto LABEL_throw_error;
    }
    /* Enter through a computed goto, as dispatch does.  GCC's static profile
     * then reaches the handler labels from here; a plain goto carries a
     * "predicted unlikely" estimate and leaves every handler at count 0, so
     * it is optimised for size and its closes stay out of line. */
    goto *&&LABEL_start_frame;

#include "ceval_labels.h"
#include "generated_rop_cases.c.h"

    /* A function form returned no word: it raised (_PyEval_ExecuteForms() has the
     * same contract), and process_error resolves it. */
    LABEL_outlined_exit:
    {
        _PyEvalActivation *activation = IR_ACTIVATION();
        assert(activation->exit_kind == _PyIR_EXIT_RAISE);
        process_error_origin = activation->exit_next;
        process_error_entry = activation->exit_entry;
        goto LABEL_process_error;
    }

    /* The shared dispatch blocks of the generator routines: gen_dispatch runs
     * the word at next_instr, and gen_stepped runs what a step returned. */
    HOT_LABEL(gen_dispatch)
    {
        int opcode;
        DISPATCH();
    }

    HOT_LABEL(gen_stepped)
    {
        frame = gen_transfer.frame;
        next_instr = gen_transfer.next;
        if (next_instr == NULL) {
            goto LABEL_outlined_exit;
        }
        goto LABEL_gen_dispatch;
    }

    /* The other evaluator switched frame to this one: go on at its word */
    LABEL_switch_entry:
    {
        next_instr = frame->return_pc;
        int opcode;
        DISPATCH();
    }

    /* The switch word: go on at the word that it stands for, in this
     * evaluator or the other one (_PyEval_SwitchesLoop()) */
    LABEL_switch_loop:
    {
        next_instr = _PyEval_SwitchLoop(tstate, frame);
        if (_PyEval_SwitchesLoop(frame, IR_OBSERVING)) {
            assert(frame->return_pc == next_instr);
            Py_MUSTTAIL return IR_OTHER_EVALUATOR(tstate, frame, IR_ENTER_SWITCH);
        }
        int opcode;
        DISPATCH();
    }

#include "ceval_routines.h"
