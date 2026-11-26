// The definitions in this file are inherited register instructions.
// They remain until their families port to Python/register_ops/*.rop, and
// they only shrink.  Write new instructions in Python/register_ops/, never
// here: Lib/test/test_ir_ceiling.py fails on a new name outside that ceiling
// (Lib/test/ir_handler_ceiling.txt), and a port deletes its lines.

// This file contains instruction definitions.
// It is read by generators stored in Tools/cases_generator/
// to generate Python/generated_ir_cases.c.h and others.
// Note that there is some dummy C code at the top and bottom of the file
// to fool text editors like VS Code into believing this is valid C code.
// The actual instruction definitions start at // BEGIN BYTECODES //.
// See Tools/cases_generator/README.md for more information.

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_audit.h"         // _PySys_Audit()
#include "pycore_backoff.h"
#include "pycore_call.h"          // _Py_METH_CALL_FLAGS
#include "pycore_cell.h"          // _PyCell_GetValue()
#include "pycore_ceval.h"         // _PyEval_LazyImportName(), _PyEval_LazyImportFrom()
#include "pycore_code.h"
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS
#include "pycore_function.h"
#include "pycore_import.h"        // _PyImport_LoadLazyImportTstate()
#include "pycore_instruments.h"
#include "pycore_interpolation.h" // _PyInterpolation_Build()
#include "pycore_intrinsics.h"
#include "pycore_lazyimportobject.h"  // PyLazyImport_CheckExact()
#include "pycore_long.h"          // _PyLong_ExactDealloc(), _PyLong_GetZero()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_opcode_metadata.h"  // uop names
#include "pycore_opcode_utils.h"  // MAKE_FUNCTION_*
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_*
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_value.h"
#include "pycore_template.h"      // _PyTemplate_Build()
#include "pycore_tuple.h"         // _PyStolenTuple_Free(), _PyTuple_ITEMS()
#include "pycore_typeobject.h"    // _PySuper_Lookup()

#include "pycore_dict.h"
#include "dictobject.h"
#include "pycore_frame.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"


#include "ceval_macros.h"

/* Flow control macros */

#define inst(name, ...) case name:
#define op(name, ...) /* NAME is ignored */
#define macro(name) static int MACRO_##name
#define super(name) static int SUPER_##name
#define family(name, ...) static int family_##name
#define pseudo(name) static int pseudo_##name
#define label(name) name:

/* Annotations */
#define guard
#define override
#define specializing
#define tier1
#define no_save_ip

// Dummy variables for stack effects.
static PyObject *value, *value1, *value2, *left, *right, *res, *sum, *prod, *sub;
static PyObject *container, *start, *stop, *v, *lhs, *rhs, *res2;
static PyObject *list, *tuple, *dict, *owner, *set, *str, *tup, *map, *keys;
static PyObject *exit_func, *lasti, *val, *retval, *obj, *iter, *exhausted;
static PyObject *aiter, *awaitable, *iterable, *w, *exc_value, *bc, *locals;
static PyObject *orig, *excs, *update, *b, *fromlist, *level, *from;
static PyObject **pieces, **values;
static size_t jump;
// Dummy variables for cache effects
static uint16_t invert, index, hint;
#define unused 0  // Used in a macro def, can't be static
static uint32_t type_version;

static PyObject *
dummy_func(
    PyThreadState *tstate,
    _PyInterpreterFrame *frame,
    unsigned char opcode,
    unsigned int oparg,
    _PyIRWord *next_instr,
    PyValue *slots,
    int throwflag,
    PyObject *args[]
)
{
    // Dummy labels.
    error:
    // Dummy locals.
    PyObject *dummy;
    _PyIRWord *this_instr;
    PyObject *attr;
    PyObject *attrs;
    PyObject *bottom;
    PyObject *callable;
    PyObject *callargs;
    PyObject *codeobj;
    PyObject *cond;
    PyObject *descr;
    PyObject *exc;
    PyObject *exit;
    PyObject *fget;
    PyObject *fmt_spec;
    PyObject *func;
    uint32_t func_version;
    PyObject *getattribute;
    PyObject *kwargs;
    PyObject *kwdefaults;
    PyObject *len_o;
    PyObject *match;
    PyObject *match_type;
    PyObject *method;
    PyObject *mgr;
    Py_ssize_t min_args;
    PyObject *names;
    PyObject *new_exc;
    PyObject *next;
    PyObject *none;
    PyObject *null;
    PyObject *prev_exc;
    PyObject *receiver;
    PyObject *rest;
    int result;
    PyObject *self;
    PyObject *seq;
    PyObject *slice;
    PyObject *step;
    PyObject *subject;
    PyObject *top;
    PyObject *type;
    PyObject *typevars;
    PyObject *val0;
    PyObject *val1;
    int values_or_none;

    switch (opcode) {

// BEGIN BYTECODES //
        pure inst(NOP, (--))
            effects() exits(next)
        {
        }

        /* A stack op only: the pseudo op that PrepareKwargs puts before a read of
         * a lazy **kwargs local is the MaterializeKwargs family. */
        tier1 inst(MATERIALIZE_KWARGS, (--))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        leaf op(_CHECK_PERIODIC, (--))
            effects(runs_python, polls) exits(next, raise)
        {
            _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
            QSBR_QUIESCENT_STATE(tstate);
            if (_Py_EvalBreakerHasEvents(tstate)) {
                int err = _Py_HandlePending(tstate);
                ERROR_IF(err != 0);
            }
        }

        replaced op(_CHECK_PERIODIC_AT_END, (--))
            effects(runs_python, polls) exits(next, raise)
        {
            int err = check_periodics_at_end(tstate, frame);
            ERROR_IF(err != 0);
        }

        /* Stack ops only: RESUME is the Resume family (Python/register_ops/resume.rop);
         * its instrumented form runs Resume.Observed. */
        eval_break tier1 inst(RESUME, (unused/1 --))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        eval_break tier1 inst(INSTRUMENTED_RESUME, (unused/1 --))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        pseudo(LOAD_CLOSURE, (-- unused)) = {
            LOAD_FAST,
        };

        macro(STORE_FAST) = _SWAP_FAST + _POP_TOP_ANY;

        leaf op(_SWAP_FAST, (value: consumed -- trash: moved(home)))
            effects() exits(next)
        {
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            GETLOCAL(oparg) = value;
            _PyValue_CheckHome(value);
            DEAD(value);
            trash = tmp;
        }

        pseudo(STORE_FAST_MAYBE_NULL, (unused --)) = {
            STORE_FAST,
        };

        // The release that inherited register instructions end with.  The
        // POP_TOP stack op itself lowers to a retirement (rule_pop_top).
        leaf op(_POP_TOP_ANY, (value: consumed --))
            effects(releases) exits(next)
        {
            PyValue_XCLOSE(value);
        }

        leaf inline_ok op(_POP_TOP_INT, (value: consumed exact(PyLong_Type) --))
            effects(writes) exits(next)
        {
            assert(PyValue_LongCheckExact(value));
            PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
        }

        // An inline float owns no reference (see _POP_TOP_NUMBER).
        leaf inline_ok op(_POP_TOP_FLOAT, (value: closed --))
            effects() exits(next)
        {
            assert(PyValue_IsInlineFloat(value));
            DEAD(value);
        }

        // An operand of float arithmetic, an inline float or int. It owns no
        // reference, so there is nothing to close. The compiler cannot prove
        // this from the float guard's two ranges, and a close here would
        // cost a pointer test and a dealloc call per operand.
        leaf inline_ok op(_POP_TOP_NUMBER, (value: consumed inline --))
            effects() exits(next)
        {
            assert(PyValue_IsInlineFloat(value) || PyValue_IsInlineInt(value));
            DEAD(value);
        }

        op(_POP_TOP_UNICODE, (value: consumed exact(PyUnicode_Type) --))
            effects(writes) exits(next)
        {
            assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value)));
            PyValue_CLOSE_SPECIALIZED(value, _PyUnicode_ExactDealloc);
        }

        op(_POP_TOP_OPARG, (args[oparg]: consumed --))
            effects(writes, releases) exits(next)
        {
            IR_CLOSE_INPUTS(args);
        }

        pure inst(PUSH_NULL, (-- res: owned null))
            effects() exits(next)
        {
            res = PyValue_NULL;
        }

        no_save_ip inst(END_FOR, (value: consumed -- ))
            effects(releases) exits(next)
        {
            /* Don't update instr_ptr, so that POP_ITER sees
             * the FOR_ITER as the previous instruction.
             * This has the benign side effect that if value is
             * finalized it will see the location as the FOR_ITER's.
             */
            PyValue_CLOSE(value);
        }


        inst(POP_ITER, (iter: consumed, index_or_null: closed -- ))
            effects(releases) exits(next)
        {
            (void)index_or_null;
            DEAD(index_or_null);
            PyValue_CLOSE(iter);
        }

        no_save_ip tier1 inst(INSTRUMENTED_END_FOR,
                (receiver, index_or_null, value: taken -- receiver, index_or_null))
            effects(runs_python) exits(next, raise)
        {
            /* Need to create a fake StopIteration error here,
             * to conform to PEP 380 */
            if (PyValue_GenCheck(receiver)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, value);
                if (err) {
                    ERROR_NO_POP();
                }
            }
            PyValue_CLOSE(value);
        }

        tier1 inst(INSTRUMENTED_POP_ITER, (iter: consumed, index_or_null: closed -- ))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue held = TAKE(iter);
            (void)index_or_null;
            DEAD(index_or_null);
            IR_DEFER_CLOSE(held);
            IR_OBSERVE_COMMITTED_BRANCH(prev_instr, next_instr,
                PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_FALLTHROUGH);
        }

        inst(END_SEND,
                (receiver: consumed, index_or_null: closed, value: consumed
                 -- val: moved(value)))
            effects(releases) exits(next)
        {
            val = TAKE(value);
            (void)index_or_null;
            DEAD(index_or_null);
            PyValue_CLOSE(receiver);
        }

        tier1 inst(INSTRUMENTED_END_SEND,
                (receiver: consumed assumed(object) by compiler_send_loop,
                 index_or_null: closed, value: consumed -- val: moved(value)))
            effects(runs_python) exits(next, raise, redirect)
        {
            PyObject *receiver_o = PyValue_AsKnownObjectBorrow(receiver);
            // Taken before the monitoring call, which can run code
            PyValue held = TAKE(value);
            int stops = PyGen_Check(receiver_o) || PyCoro_CheckExact(receiver_o);
            (void)index_or_null;
            DEAD(index_or_null);
            PyValue_CLOSE(receiver);
            if (stops) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, held);
                if (err) {
                    PyValue_CLOSE(held);
                    ERROR_NO_POP();
                }
            }
            // The end of an `async for`'s send loop is its left branch. The
            // tools of other sends are none, and the call returns at once.
            if (!tstate->tracing) {
                _PyIRObserverResult jump = _Py_call_instrumentation_jump(
                    this_instr, tstate, PY_MONITORING_EVENT_BRANCH_LEFT, frame,
                    this_instr, next_instr);
                if (jump.status < 0) {
                    PyValue_CLOSE(held);
                    ERROR_NO_POP();
                }
                if (jump.status > 0) {
                    // An observer moved the frame: the word leaves for its
                    // choice, and the sent value is no longer the word's
                    PyValue_CLOSE(held);
                    DISPATCH_REDIRECT();
                }
                // Unredirected, the call returns the destination it was
                // given (instrumentation.c), so the word goes on to next
                assert(jump.next == next_instr);
            }
            val = held;
        }

        tier1 inst(UNARY_NEGATIVE, (value: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* Stack ops only: rule_truth, rule_branch and rule_jump (Python/regcfg.c)
         * lower them to Truth, BranchTruth, BranchNone and Jump
         * (Python/register_ops/truth.rop), and no handler runs them
         * (STACK_ONLY in Tools/cases_generator/ir_generator.py).  The bodies
         * give the compiler the opcode's flags (escapes, error, jump,
         * eval breaker; Python/flowgraph.c) and its stack effect. */
        inst(UNARY_NOT, (value: consumed -- res: owned exact(PyBool_Type)))
            effects(runs_python) exits(next, raise)
        {
            int truth = _PyValue_IsTrue(value);
            PyValue_CLOSE(value);
            ERROR_IF(truth < 0);
            res = truth ? PyValue_False : PyValue_True;
        }

        inst(TO_BOOL, (unused/3, value: consumed -- res: owned exact(PyBool_Type)))
            effects(runs_python) exits(next, raise)
        {
            int truth = _PyValue_IsTrue(value);
            PyValue_CLOSE(value);
            ERROR_IF(truth < 0);
            res = truth ? PyValue_True : PyValue_False;
        }

        op(_GUARD_TOS_LIST, (tos: object -- tos: exact(PyList_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_NOS_UNICODE, (nos: object, unused -- nos: exact(PyUnicode_Type), unused))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyUnicode_CheckExact(o));
        }

        op(_GUARD_TOS_UNICODE, (value: object -- value: exact(PyUnicode_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *value_o = PyValue_AsKnownObjectBorrow(value);
            EXIT_IF(!PyUnicode_CheckExact(value_o));
        }

        tier1 inst(UNARY_INVERT, (value: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* Stack ops only: UNARY_NEGATIVE, UNARY_INVERT and BINARY_OP are the Unary
         * and Binary families (Python/register_ops/numeric.rop), and Add and
         * GetItem for their selectors. */
        // The int guards accept an inline int only: a boxed exact int is
        // outside the inline range (canonical construction). The uops after
        // them read the value with PyValue_InlineInt64() or scaled.
        leaf inline_ok op(_GUARD_NOS_INT, (left, unused -- left: inline_int, unused))
            effects() exits(next, miss)
        {
            EXIT_IF(!PyValue_IsInlineInt(left));
        }

        leaf inline_ok op(_GUARD_TOS_INT, (value -- value: inline_int))
            effects() exits(next, miss)
        {
            EXIT_IF(!PyValue_IsInlineInt(value));
        }

        // The float guards accept an inline float only: a boxed exact float
        // is a NaN (canonical construction). The uops after them read the
        // value with PyValue_InlineFloat().
        leaf inline_ok op(_GUARD_NOS_FLOAT, (left, unused -- left: inline_float, unused))
            effects() exits(next, miss)
        {
            EXIT_IF(!PyValue_IsInlineFloat(left));
        }

        leaf inline_ok op(_GUARD_TOS_FLOAT, (value -- value: inline_float))
            effects() exits(next, miss)
        {
            EXIT_IF(!PyValue_IsInlineFloat(value));
        }

        /* Stack ops only: BINARY_SLICE and STORE_SLICE are the SliceRead and
         * SliceWrite families (Python/register_ops/items.rop). */
        tier1 inst(BINARY_SLICE, (container: consumed, start: consumed, stop: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(STORE_SLICE, (v: consumed, container: consumed, start: consumed, stop: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        op(_GUARD_TOS_TUPLE, (tos: object -- tos: exact(PyTuple_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        op(_GUARD_TOS_ANY_DICT, (tos: object -- tos: exact(PyAnyDict_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyAnyDict_CheckExact(o));
        }

        /* A stack op only: LIST_APPEND is the AddElement family (Python/register_ops/displays.rop). */
        tier1 inst(LIST_APPEND,
                (list: object, unused[oparg-1], v: consumed -- list, unused[oparg-1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: SET_ADD is the AddElement family (Python/register_ops/displays.rop). */
        tier1 inst(SET_ADD,
                (set: object, unused[oparg-1], v: consumed -- set, unused[oparg-1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* Stack ops only: STORE_SUBSCR and DELETE_SUBSCR are the SetItem and
         * DelItem families (Python/register_ops/items.rop). */
        tier1 inst(STORE_SUBSCR, (unused/1, v: consumed, container: consumed, sub: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(DELETE_SUBSCR, (container: consumed, sub: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: its words are Intrinsic.*. */
        tier1 inst(CALL_INTRINSIC_1, (value: consumed -- res: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(CALL_INTRINSIC_2, (value2_st: consumed nullable, value1_st: consumed -- res: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: RAISE_VARARGS is the Raise family (Python/register_ops/exceptions.rop). */
        tier1 inst(RAISE_VARARGS, (args[oparg]: consumed -- ))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: the entry trampoline's exit: its word is Return.External. */
        tier1 inst(INTERPRETER_EXIT,
                (retval: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // The returned or yielded value goes to another frame, a home
        op(_MAKE_HEAP_SAFE, (value: consumed -- taken: moved(value)))
            effects() exits(next)
        {
            taken = TAKE(value);
        }

        /* A stack op only: RETURN_VALUE is the Return family (Python/register_ops/calls.rop). */
        tier1 inst(RETURN_VALUE,
                (retval: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: RETURN_VALUE's instrumented form: the Return family's twins run Return.Observed. */
        tier1 inst(INSTRUMENTED_RETURN_VALUE,
                (retval: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        outlined inst(GET_AITER, (obj: consumed -- iter: owned))
            effects(runs_python) exits(next, raise)
        {
            PyValue iterator = _PyEval_GetAIter(tstate, obj);
            DEAD(obj);
            ERROR_IF(PyValue_IsNull(iterator));
            iter = iterator;
        }

        outlined inst(GET_ANEXT,
                (aiter: assumed(object) by GET_AITER -- aiter, awaitable: owned))
            effects(runs_python) exits(next, raise)
        {
            // GET_AITER accepted it: it has __anext__
            PyObject *awaitable_o = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(aiter));
            if (awaitable_o == NULL) {
                ERROR_NO_POP();
            }
            awaitable = PyValue_FromObjectSteal(awaitable_o);
        }

        outlined inst(GET_AWAITABLE, (iterable: consumed -- iter: owned))
            effects(runs_python) exits(next, raise)
        {
            if (coro_awaitable_can_transfer(iterable)) {
                iter = TAKE(iterable);
            }
            else {
                PyObject *iter_o = _PyEval_GetAwaitable(iterable, oparg);
                PyValue_CLOSE(iterable);
                ERROR_IF(iter_o == NULL);
                iter = PyValue_FromObjectSteal(iter_o);
            }
        }

        family(SEND, INLINE_CACHE_ENTRIES_SEND) = {
            SEND_GEN,
            SEND_VIRTUAL,
            SEND_ASYNC_GEN,
        };

        /* The generator-boundary variants SEND*_XFER_<consumer>, which
         * fuse the send with the word that consumes its result: the last
         * uop's fall-through is the delegated yield, and its completion
         * (its branch) runs the consumer's arm of the completion routine
         * (Python/ceval_routines.h), which returns from this frame for
         * RETURN and continues at the send's exit for the others. */
        boundary(SEND) at(last) yields(delegated) completion(send) completes(
            RETURN: exits(return, raise),
            STORE_LOCAL, DISCARD, HOME, STORE_FAST, POP_TOP: exits(branch, raise));

        specializing op(_SPECIALIZE_SEND,
                (receiver: assumed(object) by compiler_send_loop, unused, unused
                 -- receiver, unused, unused))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_Send(receiver, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(SEND);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // The cursor goes by value, as in _FOR_ITER: a tagged index steps an
        // index-protocol iterator, NULL sends to a generator or any other
        // iterator object.  The compiler's send loop pushes the cursor
        // slot as NULL (await, PUSH_NULL) or as GET_ITER's cursor, a
        // tagged index or NULL (yield from).
        tier1 window op(_SEND,
                (receiver: assumed(object) by compiler_send_loop,
                 index: consumed assumed(encoding(tagged_index|null)) by compiler_send_loop,
                 v: taken
                 -- receiver, next_index: inline encoding(tagged_index|null) replaces(index),
                 retval: owned))
            effects(runs_python, switches_frame) exits(next, branch, enter, raise)
        {
            // GET_AWAITABLE and GET_YIELD_FROM_ITER returned an iterator object
            PyObject *receiver_o = PyValue_AsKnownObjectBorrow(receiver);
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            if (!IS_PEP523_HOOKED(tstate) &&
                (Py_TYPE(receiver_o) == &PyGen_Type || Py_TYPE(receiver_o) == &PyCoro_Type) &&
                !(FT_ATOMIC_LOAD_INT8_RELAXED(((PyGenObject *)receiver_o)->gi_frame_state) == FRAME_CREATED && !PyValue_IsNone(v)) &&
                gen_try_set_executing((PyGenObject *)receiver_o))
            {
                PyGenObject *gen = (PyGenObject *)receiver_o;
                IR_ENTER_GENERATOR(gen, _PY_IR_SEND);
            }
            if (!PyValue_IsNull(index) && PyValue_IsNone(v)) {
                // v is None: it is dead before the cursor, which lies below it
                DEAD(v);
                PyValue cursor = TAKE(index);
                _PyForIterStep step = _PyForIter_VirtualIteratorNextInline(tstate, frame, PyValue_Borrow(receiver), cursor);
                PyValue item = step.value;
                int found = step.status;
                if (found <= 0) {
                    if (found < 0) {
                        ERROR_NO_POP();
                    }
                    PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;
                    IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);
                }
                next_index = step.cursor;
                retval = item;
            }
            else {
                _PyStatusValue sent = _PyValue_Send(receiver_o, v);
                if (sent.status == PYGEN_ERROR) {
                    ERROR_NO_POP();
                }
                if (sent.status == PYGEN_RETURN) {
                    PyValue completed = sent.value;
                    IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);
                }
                PyValue_CLOSE(v);
                next_index = index;
                DEAD(index);
                retval = sent.value;
            }
        }

        macro(SEND) = unused/1 + _SPECIALIZE_SEND + _SEND;

        inline_ok op(_SEND_GEN_FRAME,
                (receiver, null: nullable, v: taken -- receiver, null))
            effects(runs_python, switches_frame) exits(miss, enter, raise)
        {
            EXIT_IF(!PyValue_IsObject(receiver), "a number is no generator");
            PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
            EXIT_IF(Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type,
                    "the receiver is not a generator or a coroutine");
            EXIT_IF(FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state) == FRAME_CREATED && !PyValue_IsNone(v),
                    "a created generator accepts only None");
            EXIT_IF(!gen_may_claim(gen), "the generator runs or has finished");
            STAT_INC(SEND, hit);
            if (!gen_claim(tstate, gen)) {
                ERROR_NO_POP();
            }
            IR_ENTER_GENERATOR(gen, _PY_IR_SEND);
        }

        macro(SEND_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _SEND_GEN_FRAME;

        /* SEND_GEN enters the generator's frame: the delegated yield and
         * the consumer's arm run when that frame yields or returns to this
         * word (Python/ceval_routines.h), not in this word's uops */
        boundary(SEND_GEN) at(enter) yields(delegated) completion(send) completes(
            RETURN: exits(return, raise),
            STORE_LOCAL, DISCARD, HOME, STORE_FAST, POP_TOP: exits(branch, raise));

        op(_GUARD_TOS_IS_NONE, (val -- val: is(PyValue_None)))
            effects() exits(next, miss)
        {
            EXIT_IF(!PyValue_IsNone(val));
        }


        op(_GUARD_NOS_NOT_NULL, (nos, unused -- nos, unused))
            effects() exits(next, miss)
        {
            EXIT_IF(PyValue_IsNull(nos));
        }

        // The cursor goes by value, as in _FOR_ITER_VIRTUAL.  `none` is
        // released on a raise and its home is kept: it is dead first.
        replaced op(_SEND_VIRTUAL,
                (iter: assumed(object) by GET_ITER, index: consumed,
                 none: consumed is(PyValue_None)
                 -- iter, next_index: inline replaces(index), next: owned))
            effects(runs_python) exits(next, branch, raise)
        {
            // SEND specialized on an iterator object with _tp_iteritem
            DEAD(none);
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            Py_ssize_t position = PyValue_UntagIndex(index);
            PyValue spent = TAKE(index);
            _PyValue_DropCursor(spent);
            _Py_iteritemfunc iteritem = Py_TYPE(iter_o)->_tp_iteritem;
            _PyValueIndexPair item = iteritem(iter_o, position);
            if (PyValue_IsNull(item.value)) {
                if (item.index < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);
            }
            next = item.value;
            next_index = PyValue_TagIndex(item.index);
        }

        macro(SEND_VIRTUAL) =
            unused/1 +
            _GUARD_TOS_IS_NONE +
            _GUARD_NOS_NOT_NULL +
            _SEND_VIRTUAL;

        op(_GUARD_3OS_ASYNC_GEN_ASEND,
                (iter: object, null_or_index, value
                 -- iter: exact(PyAsyncGenASend_Type), null_or_index, value))
            effects(reads) exits(next, miss)
        {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(!PyAsyncGenASend_CheckExact(iter_o));
        }

        replaced op(_SEND_ASYNC_GEN,
                (iter: taken exact(PyAsyncGenASend_Type), null_in: taken nullable, v: taken
                 -- asend: moved(iter), null_out: moved(null_in) nullable, retval: owned))
            effects(runs_python) exits(next, branch, raise)
        {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            assert(PyAsyncGenASend_CheckExact(iter_o));
            _PyStatusValue sent = _PyValue_Send(iter_o, v);
            PySendResult what = (PySendResult)sent.status;
            if (what == PYGEN_ERROR) {
                ERROR_NO_POP();
            }
            if (what == PYGEN_RETURN) {
                PyValue completed = sent.value;
                IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);
            }
            PyValue_CLOSE(v);
            asend = iter;
            DEAD(iter);
            null_out = null_in;
            DEAD(null_in);
            retval = sent.value;
        }

        macro(SEND_ASYNC_GEN) =
            unused/1 +
            _GUARD_3OS_ASYNC_GEN_ASEND +
            _SEND_ASYNC_GEN;

        window op(_YIELD_VALUE, (retval: consumed -- value: moved(retval)))
            effects(reads, writes, switches_frame) exits(yield)
        {
            // NOTE: It's important that YIELD_VALUE never raises an exception!
            // The compiler treats any exception raised here as a failed close()
            // or throw() call.
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            frame->instr_ptr++;
            PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
            assert(FRAME_SUSPENDED_YIELD_FROM == FRAME_SUSPENDED + 1);
            assert(oparg == 0 || oparg == 1);
            PyValue temp = retval;
            DEAD(retval);
            SAVE_STACK();
            DTRACE_FUNCTION_RETURN();
            tstate->exc_info = gen->gi_exc_state.previous_item;
            gen->gi_exc_state.previous_item = NULL;
            _Py_LeaveRecursiveCallPy(tstate);
            _PyInterpreterFrame *gen_frame = frame;
            // The value sent in when the generator resumes takes the slot
            // of the value it yields
            gen_frame->return_slot = (uint16_t)IR_DST_SLOT;
            _PyThreadState_UpdateLastProfiledFrame(tstate, gen_frame, gen_frame->previous);
            frame = tstate->current_frame = frame->previous;
            gen_frame->previous = NULL;
            ((_PyThreadStateImpl *)tstate)->generator_return_kind = GENERATOR_YIELD;
            FT_ATOMIC_STORE_INT8_RELEASE(gen->gi_frame_state, FRAME_SUSPENDED + oparg);
            #ifdef Py_DEBUG
            if (frame->f_executable != NULL) {
                Py_ssize_t i = INSTR_OFFSET_OF(frame->instr_ptr);
                assert(i >= 0 && i <= INT_MAX);
                int opcode = _PyIR_BaseOpcode(
                    _PyCode_InstrHandler(_PyFrame_GetCode(frame), (int)i));
                assert(opcode == SEND || opcode == FOR_ITER);
            }
            #endif
            RELOAD_STACK();
            next_instr = frame->return_pc;
            value = temp;
            LLTRACE_RESUME_FRAME();
        }

        macro(YIELD_VALUE) =
            _MAKE_HEAP_SAFE +
            _YIELD_VALUE;

        /* The variants *_XFER_<consumer> of the yields: the consumer that
         * resumes the generator (a SEND, or the store of the sent value)
         * runs fused with the resume.  They add no exit. */
        boundary(YIELD_VALUE) at(last) resumes(SEND, STORE_LOCAL);

        tier1 op(_YIELD_VALUE_EVENT, (val -- val))
            effects(runs_python) exits(next, raise, redirect)
        {
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_YIELD,
                    frame, this_instr, val);
            if (err) {
                RELOAD_STACK();
                ERROR_NO_POP();
            }
            if (frame->instr_ptr != this_instr) {
                RELOAD_STACK();
                DISPATCH_REDIRECT();
            }
        }

        macro(INSTRUMENTED_YIELD_VALUE) =
            _YIELD_VALUE_EVENT +
            _MAKE_HEAP_SAFE +
            _YIELD_VALUE;

        boundary(INSTRUMENTED_YIELD_VALUE) at(last) resumes(SEND, STORE_LOCAL);

        // Contract (G0d1): retval moves into the caller's home through
        // IR_SUSPEND_DELEGATED (ARGS(consumed); its row's effects; EXITS(yield));
        // the outputs keep the homes of the inputs (`value` the home of retval).
        ir_only no_fold op(_YIELD_DELEGATED,
            (receiver, unused_cursor, retval: consumed
             -- receiver, unused_cursor, value: moved(retval)))
            effects(reads, writes, runs_python, switches_frame) exits(yield)
        {
            IR_METADATA_PASSTHROUGH(1);
            PyValue outgoing = TAKE(retval);
            IR_SUSPEND_DELEGATED(outgoing);
        }

        macro(YIELD_DELEGATED) = _YIELD_DELEGATED;

        boundary(YIELD_DELEGATED) at(last) resumes(SEND, STORE_LOCAL);

        macro(INSTRUMENTED_YIELD_DELEGATED) =
            _YIELD_VALUE_EVENT + _YIELD_DELEGATED;

        boundary(INSTRUMENTED_YIELD_DELEGATED) at(last) resumes(SEND, STORE_LOCAL);

        /* A stack op only: POP_EXCEPT is the ExceptLeave family. */
        tier1 inst(POP_EXCEPT, (exc_value: consumed assumed(object) by PUSH_EXC_INFO -- ))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: RERAISE is the Reraise family. */
        tier1 inst(RERAISE,
                (values[oparg], exc_st: consumed assumed(object) by exception_unwind -- values[oparg]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // exc_st is the exception that exception_unwind pushed into a
        // temporary; INSTRUMENTED_JUMP's callbacks in _MONITOR_END_ASYNC_FOR
        // can replace a temporary only by redirecting the frame, and a
        // redirect leaves the word
        tier1 window op(_END_ASYNC_FOR,
                (awaitable_st: taken, exc_st: taken assumed(object) by exception_unwind -- ))
            effects(runs_python) exits(next, jump, raise)
        {
            (void)oparg;
            // An exception being handled is an instance of BaseException
            assert(PyValue_IsObject(exc_st));
            PyObject *exc = PyValue_AsKnownObjectBorrow(exc_st);

            assert(exc && PyExceptionInstance_Check(exc));
            int matches = PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration);
            if (matches) {
                DECREF_INPUTS();
            }
            else {
                _PyErr_SetRaisedException(tstate, Py_NewRef(exc));
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
            // Pretend jump as we need source offset for monitoring; after
            // the raise, so that the word's successor is set only on the
            // path that completes
            JUMPBY(0);
        }

        tier1 op(_MONITOR_END_ASYNC_FOR, ( -- ))
            effects(runs_python) exits(next, raise, redirect)
        {
            assert(_PyIR_BaseOpcode(_PyIR_RealHandler(frame, JUMP_TARGET(oparg))) == SEND);
            INSTRUMENTED_JUMP(JUMP_TARGET(oparg), this_instr+1, PY_MONITORING_EVENT_BRANCH_RIGHT);
        }

        macro(INSTRUMENTED_END_ASYNC_FOR) =
            _MONITOR_END_ASYNC_FOR +
            _END_ASYNC_FOR;

        outlined macro(END_ASYNC_FOR) = _END_ASYNC_FOR;

        tier1 inst(CLEANUP_THROW,
                (sub_iter: taken, null_in: taken nullable, last_sent_val: taken,
                 exc_value_st: taken assumed(object) by exception_unwind
                 -- none: owned, null_out: owned, value: owned))
            effects(runs_python) exits(branch, raise)
        {
            // An exception being handled is an instance of BaseException
            PyObject *exc_value = PyValue_AsKnownObjectBorrow(exc_value_st);
            assert(exc_value && PyExceptionInstance_Check(exc_value));
            int matches = PyErr_GivenExceptionMatches(exc_value, PyExc_StopIteration);
            if (!matches) {
                _PyErr_SetRaisedException(tstate, Py_NewRef(exc_value));
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
            PyValue completed = _PyStopIteration_GetValue(exc_value);
            // IR_EXIT: the exit named by the word. co_code has no operand, and
            // the instruction after this one continues there.
            IR_COMPLETE_STEP(_PY_IR_SEND_THROW, completed, IR_EXIT);
        }

        /* As SEND's variants, for a throw into the delegated generator
         * that ended it: no delegated yield */
        boundary(CLEANUP_THROW) at(last) completion(throw) completes(
            RETURN: exits(return, raise),
            STORE_LOCAL, DISCARD, HOME, STORE_FAST, POP_TOP: exits(branch, raise));

        pure inst(LOAD_COMMON_CONSTANT, ( -- value: owned))
            effects(reads) exits(next)
        {
            // Keep in sync with _common_constants in opcode.py
            assert(oparg < NUM_COMMON_CONSTANTS);
            PyValueBorrow constant = _PyValue_CommonConstant(tstate->interp->common_consts[oparg]);
            value = PyValue_DUP(constant);
        }

        /* The namespace and import stack ops only: their rules in
         * Python/regcfg.c lower them to NamespaceRead, NamespaceReadFrom,
         * NamespaceWrite, NamespaceDelete, SetupAnnotations, Import and
         * ImportFrom (Python/register_ops/namespaces.rop).  A store of NULL
         * is the compiler's `del name`. */
        tier1 inst(LOAD_BUILD_CLASS, ( -- bc: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(STORE_NAME, (v: consumed -- ))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: UNPACK_SEQUENCE is the Unpack family (Python/register_ops/displays.rop). */
        tier1 inst(UNPACK_SEQUENCE,
                (unused/1, seq: consumed -- unused[oparg], top[0]: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: UNPACK_EX is the UnpackStar family (Python/register_ops/displays.rop). */
        tier1 inst(UNPACK_EX,
                (seq: consumed
                 -- unused[oparg & 0xFF], unused, unused[oparg >> 8], top[0]: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: its rule in Python/regcfg.c lowers it to the register
         * instructions of AttrWrite (Python/register_ops/attrs.rop). */
        tier1 inst(STORE_ATTR, (unused/4, v: consumed, owner: consumed --))
            exits(abort)
        {
            /* The name operand keeps dis and the name flag of the opcode */
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(STORE_GLOBAL, (v: consumed nullable --))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_LOCALS, ( -- locals: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // The operand is the class namespace that the compiler loads from the
        // __classdict__ cell: type.__build_class__ checked it as a mapping
        tier1 inst(LOAD_FROM_DICT_OR_GLOBALS,
                (mod_or_class_dict: consumed assumed(object) by compiler -- v: owned))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_NAME, (-- v: owned))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // The NULL of a call's window needs no write: the slot is NULL by the
        // NULL invariant, and the assembler clears the bit in the word
        // (register-ir.md G4).  The stack model keeps the output.
        leaf op(_NULL_SLOT_CONDITIONAL, ( -- unused[oparg & 1]))
            effects() exits(next)
        {
        }

        /* A stack op only: rule_load_global (Python/regcfg.c) lowers it to
         * GlobalRead (Python/register_ops/names.rop). */
        tier1 inst(LOAD_GLOBAL,
                (unused/4 -- res: owned, null[oparg&1]: owned nullable))
            exits(abort)
        {
            /* The name operand keeps dis and the name flag of the opcode */
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(MAKE_CELL, (--))
            exits(abort)
        {
            PyObject *cell = PyCell_NewValue(GETLOCAL(oparg));
            PyValue tmp = GETLOCAL(oparg);
            GETLOCAL(oparg) = PyValue_FromObjectSteal(cell);
            PyValue_XCLOSE(tmp);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(DELETE_DEREF, (--))
            exits(abort)
        {
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyObject *cell = PyValue_AsKnownObjectBorrow(cell_v);
            PyValue old = _PyCell_SwapValue((PyCellObject *)cell, PyValue_NULL);
            PyValue_XCLOSE(old);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // The operand is the class namespace, as for LOAD_FROM_DICT_OR_GLOBALS
        tier1 inst(LOAD_FROM_DICT_OR_DEREF,
                (class_dict_st: consumed assumed(object) by compiler -- value: owned))
            exits(abort)
        {
            PyValue_CLOSE(class_dict_st);
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(cell_v);
            value = _PyCell_GetValue(cell);
            PyValue_XCLOSE(value);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_DEREF, ( -- value: owned))
            exits(abort)
        {
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(cell_v);
            value = _PyCell_GetValue(cell);
            PyValue_XCLOSE(value);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(STORE_DEREF, (v: consumed --))
            exits(abort)
        {
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(cell_v);
            PyValue owned = v;
            DEAD(v);
            _PyCell_SetValue(cell, owned);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(COPY_FREE_VARS, (--))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        outlined inst(BUILD_STRING, (pieces[oparg]: consumed -- str: owned))
            effects(runs_python) exits(next, raise)
        {
            PyObject *str_o = IR_BUILD_STRING(pieces);
            DEAD(pieces);
            ERROR_IF(str_o == NULL);
            str = PyValue_FromObjectSteal(str_o);
        }

        outlined inst(BUILD_INTERPOLATION,
                (value: consumed, str: consumed object, format[oparg & 1]: consumed object
                 -- interpolation: owned))
            effects(runs_python, releases) exits(next, raise)
        {
            PyObject *str_o = PyValue_AsKnownObjectBorrow(str);
            int conversion = oparg >> 2;
            PyObject *format_o;
            if (oparg & 1) {
                format_o = PyValue_AsKnownObjectBorrow(format[0]);
            }
            else {
                format_o = &_Py_STR(empty);
            }
            // The value may be a number: the interpolation holds its box
            PyObject *interpolation_o = _PyInterpolation_BuildValue(value, str_o, conversion, format_o);
            DECREF_INPUTS();
            ERROR_IF(interpolation_o == NULL);
            interpolation = PyValue_FromObjectSteal(interpolation_o);
        }

        outlined inst(BUILD_TEMPLATE,
                (strings: consumed assumed(object) by BUILD_TUPLE,
                 interpolations: consumed assumed(object) by BUILD_TUPLE -- template: owned))
            effects(allocates, raises, releases) exits(next, raise)
        {
            // Both are tuples that the compiler built
            assert(PyValue_IsObject(strings));
            assert(PyValue_IsObject(interpolations));
            PyObject *strings_o = PyValue_AsKnownObjectBorrow(strings);
            PyObject *interpolations_o = PyValue_AsKnownObjectBorrow(interpolations);
            PyObject *template_o = _PyTemplate_Build(strings_o, interpolations_o);
            PyValue_CLOSE(interpolations);
            PyValue_CLOSE(strings);
            ERROR_IF(template_o == NULL);
            template = PyValue_FromObjectSteal(template_o);
        }

        /* A stack op only: BUILD_TUPLE is the Build family (Python/register_ops/displays.rop). */
        tier1 inst(BUILD_TUPLE,
                (values[oparg]: consumed -- tup: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: BUILD_LIST is the Build family (Python/register_ops/displays.rop). */
        tier1 inst(BUILD_LIST,
                (values[oparg]: consumed -- seq: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: LIST_EXTEND is the UpdateContainer family (Python/register_ops/displays.rop). */
        tier1 inst(LIST_EXTEND,
                (list_st: object, unused[oparg-1], iterable_st: consumed -- list_st, unused[oparg-1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }


        /* A stack op only: SET_UPDATE is the UpdateContainer family (Python/register_ops/displays.rop). */
        tier1 inst(SET_UPDATE,
                (set: object, unused[oparg-1], iterable: consumed -- set, unused[oparg-1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }


        /* A stack op only: BUILD_SET is the Build family (Python/register_ops/displays.rop). */
        tier1 inst(BUILD_SET,
                (values[oparg]: consumed -- items: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: BUILD_MAP is the Build family (Python/register_ops/displays.rop). */
        tier1 inst(BUILD_MAP,
                (values[oparg*2]: consumed -- mapping: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(SETUP_ANNOTATIONS, (--))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        pseudo(ANNOTATIONS_PLACEHOLDER, (--)) = {
            NOP,
        };

        /* A stack op only: DICT_UPDATE is the UpdateContainer family (Python/register_ops/displays.rop). */
        tier1 inst(DICT_UPDATE,
                (dict: object, unused[oparg - 1], update: consumed -- dict, unused[oparg - 1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }


        /* A stack op only: DICT_MERGE is the MergeKeywords family (Python/register_ops/displays.rop). */
        tier1 inst(DICT_MERGE,
                (callable, unused, unused, dict: object, unused[oparg - 1], update: consumed -- callable, unused, unused, dict, unused[oparg - 1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }


        /* A stack op only: MAP_ADD is the AddEntry family (Python/register_ops/displays.rop). */
        tier1 inst(MAP_ADD,
                (dict_st: object, unused[oparg - 1], key: consumed, value: consumed -- dict_st, unused[oparg - 1]))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: its rule in Python/regcfg.c lowers it to the register
         * instructions of SuperAttr and SuperMethod (Python/register_ops/attrs.rop). */
        tier1 inst(LOAD_SUPER_ATTR,
                (unused/1, global_super: consumed, class_st: consumed, self_st: consumed
                 -- attr: owned, self_or_null[oparg&1]: owned nullable))
            exits(abort)
        {
            /* The name operand keeps dis and the name flag of the opcode */
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_LOAD_SUPER_ATTR,
                (unused/1, global_super: consumed, class_st: consumed, self_st: consumed
                 -- attr: owned, self_or_null[oparg&1]: owned nullable))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: rule_load_attr (Python/regcfg.c) lowers it to
         * GetAttr or GetMethod (Python/register_ops/getattr.rop). */
        tier1 inst(LOAD_ATTR,
                (unused/9, owner: consumed -- attr: owned, self_or_null[oparg&1]: owned nullable))
            exits(abort)
        {
            /* The name operand keeps dis and the name flag of the opcode */
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* The shape handlers (Py_SHAPES, see pycore_dict.h).  They follow a
         * type version guard, which proves that the owner has inline values
         * to read the shape from and lets descriptors added to the class
         * win.  The shape fields take no inline cache units.  Shapes exist
         * only in builds with the GIL, so the stores take no object lock and
         * need not check for a dict: an instance with a dict has no shape. */
        leaf op(_GUARD_SHAPE_READ,
                (type_version/2, shape/0, owner: object
                 -- owner: shape(shape) type_version(type_version)))
            effects(reads) exits(next, miss)
        {
            IR_GUARD_SHAPE_READ(PyValue_AsKnownObjectBorrow(owner), type_version, shape);
        }



        leaf op(_GUARD_MODULE_EXACT, (owner: object -- owner: exact(PyModule_Type)))
            effects(reads) exits(next, miss)
        {
            EXIT_IF(!PyModule_CheckExact(PyValue_AsKnownObjectBorrow(owner)));
        }









        op(_CHECK_ATTR_CLASS, (class_version/2, owner: object -- owner: subtype(PyType_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            EXIT_IF(!PyType_Check(owner_o));
            assert(class_version != 0);
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(((PyTypeObject *)owner_o)->tp_version_tag) != class_version,
                    "the class's own version differs from the cached one");
        }




        op(_GUARD_BUILTIN_METACLASS, (owner: object -- owner: exact(PyType_Type)))
            effects(reads) exits(next, miss)
        {
            #ifdef Py_GIL_DISABLED
            const bool gil_disabled = true;
            #else
            const bool gil_disabled = false;
            #endif
            EXIT_IF(gil_disabled, "the free-threaded build never specializes this");
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            EXIT_IF(Py_TYPE(owner_o) != &PyType_Type);
        }

        /* A stack op only: COMPARE_OP is the Compare family (Python/register_ops/tests.rop). */
        tier1 inst(COMPARE_OP,
                (unused/1, left: consumed, right: consumed -- res: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: IS_OP is the Identity family (Python/register_ops/tests.rop). */
        tier1 inst(IS_OP,
                (left: consumed, right: consumed -- b: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CONTAINS_OP is the Contains family (Python/register_ops/tests.rop). */
        tier1 inst(CONTAINS_OP,
                (unused/1, left: consumed, right: consumed -- b: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CHECK_EG_MATCH is the ExceptGroupMatch family. */
        tier1 inst(CHECK_EG_MATCH,
                (exc_value_st: consumed assumed(object) by exception_unwind, match_type_st: consumed
                 -- exc_rest: owned, exc_match: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CHECK_EXC_MATCH is the ExceptMatch family. */
        tier1 inst(CHECK_EXC_MATCH,
                (left: assumed(object) by exception_unwind, right: consumed
                 -- left, b: owned exact(PyBool_Type)))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // The compiler loads fromlist from a constant: None or a tuple of str
        tier1 inst(IMPORT_NAME,
                (level: consumed, fromlist: consumed assumed(object) by compiler -- res: owned))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(IMPORT_FROM, (from -- from, res: owned))
            exits(abort)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            (void)name;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* Stack ops only: rule_jump, rule_branch and rule_truth
         * (Python/regcfg.c) lower them to Jump, BranchTruth, BranchNone and
         * Truth (Python/register_ops/truth.rop).  The jumps keep JUMPTO so that
         * the opcode metadata knows them as jumps. */
        leaf inst(JUMP_FORWARD, (--))
            effects() exits(next, jump)
        {
            JUMPTO(oparg);
        }

        macro(JUMP_BACKWARD) =
            unused/1 +
            _CHECK_PERIODIC +
            JUMP_BACKWARD_NO_INTERRUPT;


        inst(JUMP_BACKWARD_NO_INTERRUPT, (--))
            effects() exits(next, jump)
        {
            JUMPTO(oparg);
        }

        pseudo(JUMP, (--)) = {
            JUMP_FORWARD,
            JUMP_BACKWARD,
        };

        pseudo(JUMP_NO_INTERRUPT, (--)) = {
            JUMP_FORWARD,
            JUMP_BACKWARD_NO_INTERRUPT,
        };

        pseudo(JUMP_IF_FALSE, (cond -- cond)) = [
            COPY, TO_BOOL, POP_JUMP_IF_FALSE,
        ];

        pseudo(JUMP_IF_TRUE, (cond -- cond)) = [
            COPY, TO_BOOL, POP_JUMP_IF_TRUE,
        ];

        /* The keeping jumps of "x and y" and "x or y" are BranchTruth words
         * (the keep modes); these names remain for the family's ops and the
         * event tables of Python/instrumentation.c, with no handler. */
        inst(JUMP_IF_FALSE_OR_POP, (cond: consumed -- res: moved(cond) nullable))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue held = TAKE(cond);
            int truth = _PyValue_IsTrue(held);
            if (truth < 0) {
                PyValue_CLOSE(held);
                ERROR_NO_POP();
            }
            if (!truth) {
                res = held;
                JUMPTO(oparg);
            }
            else {
                PyValue_CLOSE(held);
                res = PyValue_NULL;
            }
        }

        inst(JUMP_IF_TRUE_OR_POP, (cond: consumed -- res: moved(cond) nullable))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue held = TAKE(cond);
            int truth = _PyValue_IsTrue(held);
            if (truth < 0) {
                PyValue_CLOSE(held);
                ERROR_NO_POP();
            }
            if (truth) {
                res = held;
                JUMPTO(oparg);
            }
            else {
                PyValue_CLOSE(held);
                res = PyValue_NULL;
            }
        }

        tier1 inst(INSTRUMENTED_JUMP_IF_FALSE_OR_POP, (cond: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_JUMP_IF_TRUE_OR_POP, (cond: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        inst(POP_JUMP_IF_FALSE, (unused/1, cond: consumed -- ))
            effects(runs_python) exits(next, branch, raise)
        {
            int truth = _PyValue_IsTrue(cond);
            PyValue_CLOSE(cond);
            ERROR_IF(truth < 0);
            if (!truth) {
                JUMPTO(oparg);
            }
        }

        inst(POP_JUMP_IF_TRUE, (unused/1, cond: consumed -- ))
            effects(runs_python) exits(next, branch, raise)
        {
            int truth = _PyValue_IsTrue(cond);
            PyValue_CLOSE(cond);
            ERROR_IF(truth < 0);
            if (truth) {
                JUMPTO(oparg);
            }
        }

        inst(POP_JUMP_IF_NONE, (unused/1, value: consumed -- ))
            effects(runs_python) exits(next, branch)
        {
            int jump = PyValue_IsNone(value);
            PyValue_CLOSE(value);
            if (jump) {
                JUMPTO(oparg);
            }
        }

        inst(POP_JUMP_IF_NOT_NONE, (unused/1, value: consumed -- ))
            effects(runs_python) exits(next, branch)
        {
            int jump = PyValue_IsNone(value);
            PyValue_CLOSE(value);
            if (!jump) {
                JUMPTO(oparg);
            }
        }

        outlined inst(GET_LEN, (obj -- obj, len: owned exact(PyLong_Type)))
            effects(runs_python) exits(next, raise)
        {
            // PUSH(len(TOS))
            Py_ssize_t len_i = _PyValue_Size(obj);
            ERROR_IF(len_i < 0);
            len = PyValue_FromInt64(len_i);
            ERROR_IF(PyValue_IsNull(len));
        }

        op(_MATCH_CLASS,
                (subject: taken, type: taken, names: taken assumed(object) by codegen_pattern_class
                 -- attrs: owned, s: moved(subject), tp: moved(type), n: moved(names)))
            effects(runs_python) exits(next, raise)
        {
            // Pop TOS and TOS1. Set TOS to a tuple of attributes on success, or
            // None on failure.
            assert(PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(names)));
            // The class pattern may name a number: MatchClass rejects it
            PyObject *attrs_o = _PyEval_MatchClass(tstate,
                subject,
                type, oparg,
                PyValue_AsKnownObjectBorrow(names));
            if (attrs_o) {
                assert(PyTuple_CheckExact(attrs_o));  // Success!
                attrs = PyValue_FromObjectSteal(attrs_o);
            }
            else {
                if (_PyErr_Occurred(tstate)) { // Error!
                    ERROR_NO_POP();
                }
                attrs = PyValue_None;  // Failure!
            }
            s = subject;
            tp = type;
            n = names;
            INPUTS_DEAD();
        }

        outlined macro(MATCH_CLASS) = _MATCH_CLASS + _POP_TOP_ANY + _POP_TOP_ANY + _POP_TOP_ANY;

        outlined inst(MATCH_MAPPING, (subject -- subject, res: owned exact(PyBool_Type)))
            effects(reads) exits(next)
        {
            int match = PyValue_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            res = match ? PyValue_True : PyValue_False;
        }

        outlined inst(MATCH_SEQUENCE, (subject -- subject, res: owned exact(PyBool_Type)))
            effects(reads) exits(next)
        {
            int match = PyValue_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE;
            res = match ? PyValue_True : PyValue_False;
        }

        outlined inst(MATCH_KEYS,
                (subject: assumed(object) by MATCH_MAPPING, keys: assumed(object) by codegen_pattern_mapping
                 -- subject, keys, values_or_none: owned))
            effects(runs_python) exits(next, raise)
        {
            // On successful match, PUSH(values). Otherwise, PUSH(None).
            // MATCH_MAPPING has accepted the subject: a mapping, not a number
            assert(PyValue_IsObject(subject));
            assert(PyValue_IsObject(keys));
            PyObject *values_or_none_o = _PyEval_MatchKeys(tstate,
                PyValue_AsKnownObjectBorrow(subject), PyValue_AsKnownObjectBorrow(keys));
            ERROR_IF(values_or_none_o == NULL);
            values_or_none = PyValue_FromObjectSteal(values_or_none_o);
        }

        op(_GET_ITER,
                (iterable: consumed -- iter: owned, index_or_null: inline encoding(tagged_index|null)))
            effects(runs_python) exits(next, raise)
        {
            _PyValuePair got = _PyEval_GetIter(iterable, oparg);
            DEAD(iterable);
            ERROR_IF(PyValue_IsNull(got.first));
            iter = got.first;
            index_or_null = got.second;
        }

        macro(GET_ITER) = unused/1 + _GET_ITER;

        // The body outcome preserves the iterator/cursor pair and defines
        // the item recipient. Exhaustion retires the pair, preserves the
        // last item binding and continues at the real exit target.

        replaced op(_FOR_ITER, (iter, index: consumed -- iter, next_index: inline encoding(tagged_index|null) replaces(index), next: owned))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue cursor = TAKE(index);
            _PyForIterStep step = _PyForIter_VirtualIteratorNextInline(tstate, frame, PyValue_Borrow(iter), cursor);
            PyValue item = step.value;
            int found = step.status;
            if (found <= 0) {
                if (found < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
            next_index = step.cursor;
            next = item;
        }

        macro(FOR_ITER) = unused/1 + _FOR_ITER;
        inst(INSTRUMENTED_FOR_ITER,
                (unused/1, iter: assumed(object) by GET_ITER, index: consumed
                 -- iter, next_index: inline encoding(tagged_index|null) replaces(index), next: owned))
            effects(runs_python) exits(next, branch, raise)
        {
            PyValue cursor = TAKE(index);
            _PyForIterStep step = _PyForIter_VirtualIteratorNextInline(tstate, frame, PyValue_Borrow(iter), cursor);
            PyValue item = step.value;
            int found = step.status;
            if (found <= 0) {
                if (found < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
            next_index = step.cursor;
            next = item;
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
        }

        /* A stack op only: rule_load_special (Python/regcfg.c) lowers it to
         * SpecialLookup (Python/register_ops/attrs.rop). */
        tier1 inst(LOAD_SPECIAL, (owner: consumed -- attr: owned, self_or_null: owned nullable))
            exits(abort)
        {
            /* The oparg keeps dis and the special name of the opcode */
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        outlined inst(WITH_EXCEPT_START,
                (exit_func, exit_self: nullable, lasti, unused,
                 val: assumed(object) by PUSH_EXC_INFO
                 -- exit_func, exit_self, lasti, unused, val, res: owned))
            effects(runs_python) exits(next, raise)
        {
            /* At the top of the stack are 4 values:
               - val: TOP = exc_info()
               - unused: SECOND = previous exception
               - lasti: THIRD = lasti of exception in exc_info()
               - exit_self: FOURTH = the context or NULL
               - exit_func: FIFTH = the context.__exit__ function or context.__exit__ bound method
               We call FOURTH(type(TOP), TOP, GetTraceback(TOP)).
               Then we push the __exit__ return value.
            */
            PyObject *exc;

            // The exit function is any attribute value, a scalar included;
            // the call raises upstream's TypeError for one.
            PyObject *val_o = PyValue_AsKnownObjectBorrow(val);

            assert(val_o && PyExceptionInstance_Check(val_o));
            exc = PyExceptionInstance_Class(val_o);
            // A new reference or NULL; the call is passed None for NULL
            PyObject *original_tb = PyException_GetTraceback(val_o);
            assert(PyValue_IsTaggedIndex(lasti));
            (void)lasti; // Shut up compiler warning if asserts are off
            res = with_exit_call(exit_func, exit_self, exc, val_o,
                                 original_tb == NULL ? Py_None : original_tb);
            Py_XDECREF(original_tb);
            ERROR_IF(PyValue_IsNull(res));
        }

        pseudo(SETUP_FINALLY, (-- unused), (HAS_ARG)) = {
            /* If an exception is raised, restore the stack position
             * and push one value before jumping to the handler.
             */
            NOP,
        };

        pseudo(SETUP_CLEANUP, (-- unused, unused), (HAS_ARG)) = {
            /* As SETUP_FINALLY, but push lasti as well */
            NOP,
        };

        pseudo(SETUP_WITH, (-- unused), (HAS_ARG)) = {
            /* If an exception is raised, restore the stack position to the
             * position before the result of __(a)enter__ and push 2 values
             * before jumping to the handler.
             */
            NOP,
        };

        pseudo(POP_BLOCK, (--)) = {
            NOP,
        };

        /* A stack op only: PUSH_EXC_INFO is the ExceptEnter family. */
        tier1 inst(PUSH_EXC_INFO,
                (exc: consumed -- prev_exc: owned, new_exc: moved(exc)))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }














        op(_CHECK_ATTR_METHOD_LAZY_DICT, (dictoffset/1, owner: object -- owner))
            effects(reads) exits(next, miss)
        {
            char *ptr = ((char *)PyValue_AsKnownObjectBorrow(owner)) + MANAGED_DICT_OFFSET + dictoffset;
            PyObject *dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*(PyObject **)ptr);
            /* This object has a __dict__, just not yet created */
            EXIT_IF(dict != NULL, "the owner's dict exists");
        }



        /* A stack op only: CALL is the Call family (Python/register_ops/calls.rop). */
        eval_break tier1 inst(CALL,
                (unused/3, callable: consumed, self_or_null: consumed, args[oparg]: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CALL's instrumented form: the Call family's twins run Call.Observed. */
        eval_break tier1 inst(INSTRUMENTED_CALL,
                (unused/3, callable: consumed, self_or_null: consumed, args[oparg]: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        op(_CHECK_PEP_523, (--))
            effects(reads) exits(next, deopt)
        {
            DEOPT_IF(IS_PEP523_HOOKED(tstate));
        }

        op(_CHECK_RECURSION_REMAINING, (--))
            effects(reads) exits(next, deopt)
        {
            DEOPT_IF(tstate->py_recursion_remaining <= 1, "the recursion budget is spent");
        }

        op(_PUSH_FRAME, (new_frame: consumed -- ))
            effects(switches_frame) exits(enter)
        {
            assert(!IS_PEP523_HOOKED(tstate));
            _PyInterpreterFrame *temp = PyValue_Unwrap(new_frame);
            DEAD(new_frame);
            SAVE_STACK();
            assert(temp->previous == frame || temp->previous->previous == frame);
            CALL_STAT_INC(inlined_py_calls);
            frame = tstate->current_frame = temp;
            tstate->py_recursion_remaining--;
            RELOAD_STACK();
            LOAD_IP(0);
            ROP_ENTER_GATE();
            DTRACE_FUNCTION_ENTRY();
            LLTRACE_RESUME_FRAME();
        }

        /* A stack op only: the __init__ shim's check: its word is InitResult.Check. */
        tier1 inst(EXIT_INIT_CHECK,
                (should_be_none: consumed --))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CALL_KW is the CallKw family (Python/register_ops/calls.rop). */
        eval_break tier1 inst(CALL_KW,
                (unused/3, callable: consumed, self_or_null: consumed, args[oparg]: consumed, kwnames: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CALL_KW's instrumented form: the CallKw family's twins run CallKw.Observed. */
        eval_break tier1 inst(INSTRUMENTED_CALL_KW,
                (unused/3, callable: consumed, self_or_null: consumed, args[oparg]: consumed, kwnames: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CALL_FUNCTION_EX is the CallEx family (Python/register_ops/calls.rop). */
        eval_break tier1 inst(CALL_FUNCTION_EX,
                (unused/1, func_st: consumed, null: consumed, callargs_st: consumed, kwargs_st: consumed -- result: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CALL_FUNCTION_EX's instrumented form: CallEx.Observed. */
        eval_break tier1 inst(INSTRUMENTED_CALL_FUNCTION_EX,
                (unused/1, func_st: consumed, null: consumed, callargs_st: consumed, kwargs_st: consumed -- result: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: its words are Function.Make. */
        tier1 inst(MAKE_FUNCTION, (codeobj_st: consumed -- func: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(SET_FUNCTION_ATTRIBUTE, (attr_st: consumed, func_in: consumed -- func_out: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        // The marker at the start of a generator's code: nothing executes it,
        // because a call creates the object (_PyGen_NewCall()) with its frame
        // past this word.  The word stays for the first send's resume point,
        // f_lasti and monitoring.
        window inst(RETURN_GENERATOR, (-- res: inline null))
            effects() exits(next)
        {
            IR_GENERATOR_MARKER();
            res = PyValue_NULL;
        }

        /* A stack op only: BUILD_SLICE is the BuildSlice family (Python/register_ops/items.rop). */
        tier1 inst(BUILD_SLICE, (args[oparg]: consumed -- slice: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        outlined inst(CONVERT_VALUE, (value: consumed -- result: owned))
            effects(runs_python) exits(next, raise)
        {
            assert(oparg >= FVC_STR && oparg <= FVC_ASCII);
            PyObject *result_o;
            if (oparg == FVC_STR) {
                result_o = _PyValue_Str(value);
            }
            else if (oparg == FVC_REPR) {
                result_o = _PyValue_Repr(value);
            }
            else {
                result_o = _PyValue_ASCII(value);
            }
            PyValue_CLOSE(value);
            ERROR_IF(result_o == NULL);
            result = PyValue_FromObjectSteal(result_o);
        }

        outlined inline_ok inst(FORMAT_SIMPLE, (value: consumed -- res: owned))
            effects(runs_python) exits(next, raise)
        {
            /* If value is a unicode object, then we know the result
             * of format(value) is value itself. An inline int or float
             * formats as it prints: format(x, "") is str(x). A language
             * value is an object or inline, so the test is the same as
             * the inline test and states `object` on the other path. */
            if (!PyValue_IsObject(value)) {
                PyObject *res_o = _PyValue_Str(value);
                PyValue_CLOSE(value);
                ERROR_IF(res_o == NULL);
                res = PyValue_FromObjectSteal(res_o);
            }
            else if (!PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value))) {
                PyObject *res_o = _PyValue_Format(value, PyValue_NULL);
                PyValue_CLOSE(value);
                ERROR_IF(res_o == NULL);
                res = PyValue_FromObjectSteal(res_o);
            }
            else {
                res = TAKE(value);
            }
        }

        outlined inst(FORMAT_WITH_SPEC, (value: consumed, fmt_spec: consumed -- res: owned))
            effects(runs_python) exits(next, raise)
        {
            PyObject *res_o = _PyValue_Format(value, fmt_spec);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyValue_FromObjectSteal(res_o);
        }

        tier1 inst(BINARY_OP, (unused/5, lhs: consumed, rhs: consumed -- res: owned))
            exits(abort)
        {
            /* The selector is the oparg, as dis shows it */
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        inst(INSTRUMENTED_LINE, ( -- ))
            effects(runs_python) exits(next, jump, raise) feedback(write(counter))
        {
            int original_opcode = 0;
            if (tstate->tracing) {
                PyCodeObject *code = _PyFrame_GetCode(frame);
                int index = INSTR_OFFSET_OF(this_instr);
                original_opcode = _PyCode_GetOriginalOpcode(code->_co_monitoring->lines, index);
                next_instr = this_instr;
            } else {
                original_opcode = _Py_call_instrumentation_line(
                        tstate, frame, this_instr, prev_instr);
                RELOAD_STACK();
                if (original_opcode < 0) {
                    next_instr = frame->instr_ptr+1;
                    goto error;
                }
                next_instr = frame->instr_ptr;
                if (next_instr != this_instr) {
                    DISPATCH();
                }
            }
            /* The handler decides: a stack-only opcode such as LOAD_ATTR has
             * no entry itself, its word runs a family's generic arm. */
            opcode = ORIGINAL_HANDLER(original_opcode);
            if (_PyFb_HasEntry(opcode)) {
                /* Prevent the underlying instruction from specializing
                * and overwriting the instrumentation. */
                PAUSE_ADAPTIVE_COUNTER(FB_HEADER_OF(next_instr, opcode));
            }
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }

        inst(INSTRUMENTED_INSTRUCTION, ( -- ))
            effects(runs_python) exits(jump, raise) feedback(write(counter))
        {
            int next_opcode = _Py_call_instrumentation_instruction(
                tstate, frame, this_instr);
            ERROR_IF(next_opcode < 0);
            next_instr = this_instr;
            assert(next_opcode > 0 && next_opcode < 256);
            opcode = ORIGINAL_HANDLER(next_opcode);
            if (_PyFb_HasEntry(opcode)) {
                PAUSE_ADAPTIVE_COUNTER(FB_HEADER_OF(next_instr, opcode));
            }
            DISPATCH_GOTO();
        }

        /* The instrumented forms of the stack ops that the Truth, BranchTruth,
         * BranchNone and Jump families lower.  Stack ops only: the words are
         * the families' Observed arms (Python/register_ops/truth.rop). */
        tier1 inst(INSTRUMENTED_JUMP_FORWARD, ( -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        eval_break tier1 inst(INSTRUMENTED_JUMP_BACKWARD, (unused/1 -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_POP_JUMP_IF_TRUE, (unused/1, cond: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_POP_JUMP_IF_FALSE, (unused/1, cond: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_POP_JUMP_IF_NONE, (unused/1, value: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_POP_JUMP_IF_NOT_NONE, (unused/1, value: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(CACHE, (--))
            exits(abort)
        {
            assert(0 && "Executing a cache.");
            Py_FatalError("Executing a cache.");
        }

        tier1 inst(RESERVED, (--))
            exits(abort)
        {
            assert(0 && "Executing RESERVED instruction.");
            Py_FatalError("Executing RESERVED instruction.");
        }

        /* Opcodes of the stack bytecode in co_code that no word of the
         * register IR has (STACK_ONLY in Tools/cases_generator/ir_generator.py):
         * lowering (Python/flowgraph.c, _PyCfg_FromCode()) folds EXTENDED_ARG,
         * drops NOT_TAKEN and splits the superinstructions.  The stack
         * effects are the compiler's. */
        tier1 inst(EXTENDED_ARG, ( -- ))
            exits(abort)
        {
            assert(oparg);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(NOT_TAKEN, ( -- ))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_FAST_LOAD_FAST, ( -- value1: owned, value2: owned))
            exits(abort)
        {
            assert(oparg);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_FAST_BORROW_LOAD_FAST_BORROW, ( -- value1: owned, value2: owned))
            exits(abort)
        {
            assert(oparg);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(STORE_FAST_LOAD_FAST, (value1: consumed -- value2: owned))
            exits(abort)
        {
            assert(oparg);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(STORE_FAST_STORE_FAST, (value2: consumed, value1: consumed --))
            exits(abort)
        {
            assert(oparg);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* The local, constant and shuffle stack ops: their rules in
         * Python/regcfg.c lower them to the Local*, Const and Move families
         * (Python/register_ops/values.rop), to renames, or to nothing.  The
         * statements before the abort keep the formats' operand roles and
         * transfers, which the CFG reads. */
        tier1 inst(LOAD_FAST, (-- value: owned))
            exits(abort)
        {
            PyValueBorrow local = _PyValue_BoundLocal(GETLOCAL(oparg));
            (void)local;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_FAST_BORROW, (-- value: owned))
            exits(abort)
        {
            PyValueBorrow local = _PyValue_BoundLocal(GETLOCAL(oparg));
            (void)local;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_FAST_CHECK, (-- value: owned))
            exits(abort)
        {
            PyValueBorrow local = _PyValue_BoundLocal(GETLOCAL(oparg));
            (void)local;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_FAST_AND_CLEAR, (-- value: owned))
            exits(abort)
        {
            PyValueBorrow local = PyValue_Borrow(GETLOCAL(oparg));
            (void)local;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(DELETE_FAST, (--))
            exits(abort)
        {
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            GETLOCAL(oparg) = PyValue_NULL;
            PyValue_XCLOSE(tmp);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_CONST, (-- value: owned))
            exits(abort)
        {
            PyValueBorrow constant = _PyValue_CodeConstant(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));
            (void)constant;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(LOAD_SMALL_INT, (-- value: owned exact(PyLong_Type) inline_int))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(COPY,
                (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top: owned))
            exits(abort)
        {
            (void)bottom;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(SWAP, (bottom, unused[oparg-2], top -- bottom, unused[oparg-2], top))
            exits(abort)
        {
            (void)bottom;
            (void)top;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(POP_TOP, (value: consumed --))
            exits(abort)
        {
            PyValue_XCLOSE(value);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* Handlers of the register IR only (Include/internal/pycore_ir.h).
         * They have no opcode. */

        // The jumps that the assembler adds after a relaxed FOR_ITER or
        // SEND (Python/assemble.c), which no instruction of the stack
        // machine stands for.  They leave frame->instr_ptr at the loop, so
        // that the events after them see the loop as the instruction before.
        // Contract (seed, reviewed G0d1): JUMP_FORWARD's, `effects() exits(next,
        // jump)`.
        ir_only no_save_ip inst(IR_JUMP, ( -- ))
            effects() exits(next, jump)
        {
            JUMPTO(oparg);
        }

        // The words after a call head that name its operands, four slots
        // each (_PyIR_WordLength()).  The head consumes them, so no
        // dispatch reaches one.
        ir_only no_save_ip inst(IR_OPERANDS, ( -- ))
            exits(abort)
        {
            assert(0 && "Executing operand words.");
            Py_FatalError("Executing operand words.");
        }

        op(_SAVE_RETURN_SLOT, (--))
            effects(reads, writes) exits(next)
        {
            frame->return_slot = (uint16_t)IR_DST_SLOT;
            frame->return_pc = next_instr;
        }

        // The error labels are one routine, _PyEval_ProcessError (its row has
        // the evidence), entered at an ingress: a body's `goto error` and
        // ERROR_IF are the word's raise exit at that site (G0c8 review).
        label(error) {
            IR_PROCESS_ERROR(RAISE, 0);
        }

        spilled label(exception_unwind) {
            IR_PROCESS_ERROR(UNWIND, 0);
        }

        spilled label(exit_unwind) {
            IR_PROCESS_ERROR(EXIT, 0);
        }

        spilled label(exit_unwind_notrace) {
            IR_PROCESS_ERROR(EXIT_NOTRACE, 0);
        }

        label(throw_error) {
            IR_PROCESS_ERROR(THROW, 0);
        }

        // A frame starts: the recursion check (_Py_EnterRecursivePy, whose row
        // says it raises RecursionError), and on failure the unwind that ends
        // the frame without a traceback entry; then the first word, which
        // skips a RESUME that has nothing to do (G0c8 review).
        spilled label(start_frame) {
            int too_deep = _Py_EnterRecursivePy(tstate);
            if (too_deep) {
                goto exit_unwind_notrace;
            }
            DTRACE_FUNCTION_ENTRY();
            next_instr = frame->instr_ptr;
            ROP_ENTER_GATE();
        #ifdef Py_DEBUG
            int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS());
            if (lltrace < 0) {
                IR_PROCESS_ERROR(EXIT, 0);
            }
            frame->lltrace = lltrace;
            /* _PyEval_EvalFrameDefault() must not be called with an exception set,
            because it can clear it (directly or indirectly) and so the
            caller loses its exception */
            assert(!_PyErr_Occurred(tstate));
        #endif
            RELOAD_STACK();
            int opcode;
            DISPATCH();
        }

// END BYTECODES //

    }
 dispatch_opcode:
 error:
 exception_unwind:
 exit_unwind:
 exit_unwind_notrace:
 handle_eval_breaker:
 resume_frame:
 start_frame:
 unbound_local_error:
    ;
}

// Future families go below this point //
