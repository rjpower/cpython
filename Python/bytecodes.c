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

        outlined inst(MATERIALIZE_KWARGS, (--))
            effects(writes, allocates, raises) exits(next, raise)
        {
            assert(frame->pending_kwargs == IR_NO_DST ||
                   frame->pending_kwargs == oparg);
            int err = _PyFrame_MaterializeKwargs(frame);
            ERROR_IF(err < 0);
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

        op(_CHECK_PERIODIC_IF_NOT_YIELD_FROM, (--))
            effects(runs_python, polls) exits(next, raise)
        {
            if ((oparg & RESUME_OPARG_LOCATION_MASK) < RESUME_AFTER_YIELD_FROM) {
                int err = check_periodics(tstate);
                ERROR_IF(err != 0);
            }
        }

        // After instrumenting the code it runs its word again, now with its
        // instrumented handler: a jump to the word
        tier1 op(_MAYBE_INSTRUMENT, (--))
            effects(runs_python) exits(next, jump, raise)
        {
            #ifdef Py_GIL_DISABLED
            // For thread-safety, we need to check instrumentation version
            // even when tracing. Otherwise, another thread may concurrently
            // re-write the bytecode while we are executing this function.
            int check_instrumentation = 1;
            #else
            int check_instrumentation = (tstate->tracing == 0);
            #endif
            if (check_instrumentation) {
                uintptr_t global_version = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & ~_PY_EVAL_EVENTS_MASK;
                uintptr_t code_version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(_PyFrame_GetCode(frame)->_co_instrumentation_version);
                if (code_version != global_version) {
                    int err = _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
                    if (err) {
                        ERROR_NO_POP();
                    }
                    next_instr = this_instr;
                    DISPATCH_SAME_OPARG();
                }
            }
        }

        // A frame that starts or resumes at a RESUME skips it when it has
        // nothing to do (IR_SKIP_RESUME); it runs only after the words that
        // make cells and copy free variables, at a generator's first start,
        // and when the eval breaker or instrumentation needs it.
        // The family's generic arm: compiled code runs it as a stub after
        // RESUME_CHECK's miss
        macro(RESUME) region(shared) =
            unused/1 +
            _MAYBE_INSTRUMENT +
            _CHECK_PERIODIC_IF_NOT_YIELD_FROM;

        // The baseline JIT's check at a frame's entry (g3/design.md section
        // 1): a RESUME has nothing to do while no event is pending and the
        // code is instrumented as the interpreter is.  Otherwise it misses,
        // and the generic RESUME polls or instruments.
        leaf ir_only op(_RESUME_CHECK, (--))
            effects(reads) exits(next, miss)
        {
            EXIT_IF(_Py_EvalBreaker(tstate) != FRAME_CODE_INSTRUMENTATION_VERSION(),
                    "an event is pending, or the code's instrumentation is out of date");
        }

        // The arm that the baseline compiles for RESUME's words; the
        // interpreter never writes it (it skips RESUME, IR_SKIP_RESUME())
        macro(RESUME_CHECK) region(inline) =
            unused/1 +
            _RESUME_CHECK;

        // Under -X jit=on and always the lowering gives every RESUME and
        // JUMP_BACKWARD the form that ends in _JIT_EVENT
        // (Python/ir_assemble.c): a warm-up event of the code, which enters
        // the code's compiled unit at the next word once it has one
        // (IR_JIT_EVENT(), Python/jit.c).  Compiled code runs the forms,
        // where the event does nothing.
        // Contract (G0d1): IR_JIT_EVENT's row (EFFECTS(... runs_python),
        // EXITS(redirect, raise, exit)): the unit it starts runs this frame's
        // words, which enter and leave frames by their own records; the frame
        // goes on where the unit stopped (redirect). Otherwise the word
        // continues (next).
        ir_only op(_JIT_EVENT, (--))
            effects(runs_python) exits(next, redirect, raise, exit)
        {
            IR_JIT_EVENT();
        }

        family(RESUME, 1) baseline(RESUME_CHECK) = {
            RESUME_JIT,
            RESUME_CHECK,
        };

        macro(RESUME_JIT) =
            unused/1 +
            _MAYBE_INSTRUMENT +
            _CHECK_PERIODIC_IF_NOT_YIELD_FROM +
            _JIT_EVENT;

        op(_MONITOR_RESUME, (--))
            effects(runs_python) exits(next, raise, redirect)
        {
            int err = _Py_call_instrumentation(
                    tstate, oparg == 0 ? PY_MONITORING_EVENT_PY_START : PY_MONITORING_EVENT_PY_RESUME, frame, this_instr);
            RELOAD_STACK();
            ERROR_IF(err);
            if (frame->instr_ptr != this_instr) {
                /* Instrumentation has jumped */
                DISPATCH_REDIRECT();
            }
        }

        macro(INSTRUMENTED_RESUME) =
            unused/1 +
            _MAYBE_INSTRUMENT +
            _CHECK_PERIODIC_IF_NOT_YIELD_FROM +
            _MONITOR_RESUME;

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

        macro(UNARY_NEGATIVE) = _UNARY_NEGATIVE + _POP_TOP_ANY;

        // Inline values are negated without a box. Only -Py_INT_MIN leaves
        // the inline range; negating a float that is not NaN gives one that
        // is not.
        inline_ok op(_UNARY_NEGATIVE, (value: taken -- res: owned, v: moved(value)))
            effects(runs_python) exits(next, raise)
        {
            if (PyValue_IsInlineInt(value)) {
                res = PyValue_FromInt64(-PyValue_InlineInt64(value));
            }
            else if (PyValue_IsInlineFloat(value)) {
                res = PyValue_FromDoubleNotNaN(-PyValue_InlineFloat(value));
            }
            else {
                res = _PyValue_Negative(value);
            }
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            v = value;
            DEAD(value);
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

        leaf op(_GUARD_NOS_LIST, (nos: object, unused -- nos: exact(PyList_Type), unused))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_TOS_LIST, (tos: object -- tos: exact(PyList_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_TOS_SLICE, (tos: object -- tos))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PySlice_Check(o), "the operand is not a slice");
        }

        op(_GUARD_NOS_COMPACT_ASCII,
                (nos: object, unused -- nos: exact(PyUnicode_Type), unused))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyUnicode_CheckExact(o));
            EXIT_IF(!PyUnicode_IS_COMPACT_ASCII(o), "the str is not compact ASCII");
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

        macro(UNARY_INVERT) = _UNARY_INVERT + _POP_TOP_ANY;

        op(_UNARY_INVERT, (value: taken -- res: owned, v: moved(value)))
            effects(runs_python) exits(next, raise)
        {
            res = _PyValue_Invert(value);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            v = value;
            DEAD(value);
        }

        family(BINARY_OP, INLINE_CACHE_ENTRIES_BINARY_OP) = {
            BINARY_OP_MULTIPLY_INT,
            BINARY_OP_ADD_INT,
            BINARY_OP_SUBTRACT_INT,
            BINARY_OP_FLOOR_DIVIDE_INT,
            BINARY_OP_REMAINDER_INT,
            BINARY_OP_AND_INT,
            BINARY_OP_OR_INT,
            BINARY_OP_XOR_INT,
            BINARY_OP_LSHIFT_INT,
            BINARY_OP_RSHIFT_INT,
            BINARY_OP_MULTIPLY_FLOAT,
            BINARY_OP_ADD_FLOAT,
            BINARY_OP_SUBTRACT_FLOAT,
            BINARY_OP_TRUEDIV_FLOAT,
            BINARY_OP_ADD_UNICODE,
            BINARY_OP_SUBSCR_LIST_INT,
            BINARY_OP_SUBSCR_LIST_SLICE,
            BINARY_OP_SUBSCR_TUPLE_INT,
            BINARY_OP_SUBSCR_STR_INT,
            BINARY_OP_SUBSCR_USTR_INT,
            BINARY_OP_SUBSCR_DICT,
            BINARY_OP_SUBSCR_GETITEM,
            BINARY_OP_INPLACE_ADD_UNICODE,
            BINARY_OP_EXTEND,
        };

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

        // Int arithmetic on inline operands, on scaled values (see
        // _PyValue_IntScaled()): int64 overflow is exactly a result outside
        // the inline range, which is boxed.
        leaf inline_ok pure op(_BINARY_OP_MULTIPLY_INT,
                (left: taken exact(PyLong_Type) inline inline_int, right: taken exact(PyLong_Type) inline inline_int
                 -- res: owned exact(PyLong_Type), l: moved(left), r: moved(right)))
            effects(allocates, raises) exits(next, raise)
        {
            int64_t a = _PyValue_IntScaled(left);
            int64_t b = PyValue_InlineInt64(right);
            int64_t product;
            STAT_INC(BINARY_OP, hit);
            if (__builtin_mul_overflow(a, b, &product)) {
                res = _PyValue_MultiplyInt64(PyValue_InlineInt64(left), b);
            }
            else {
                res = _PyValue_FromIntScaled(product);
            }
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        leaf inline_ok pure op(_BINARY_OP_ADD_INT,
                (left: taken inline_int, right: taken inline_int
                 -- res: owned exact(PyLong_Type), l: moved(left), r: moved(right)))
            effects(allocates, raises) exits(next, raise)
        {
            int64_t a = _PyValue_IntScaled(left);
            int64_t b = _PyValue_IntScaled(right);
            int64_t sum;
            STAT_INC(BINARY_OP, hit);
            if (__builtin_add_overflow(a, b, &sum)) {
                res = _PyValue_BoxInt64(PyValue_InlineInt64(left) + PyValue_InlineInt64(right));
            }
            else {
                res = _PyValue_FromIntScaled(sum);
            }
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        leaf inline_ok pure op(_BINARY_OP_SUBTRACT_INT,
                (left: taken exact(PyLong_Type) inline inline_int, right: taken exact(PyLong_Type) inline inline_int
                 -- res: owned exact(PyLong_Type), l: moved(left), r: moved(right)))
            effects(allocates, raises) exits(next, raise)
        {
            int64_t a = _PyValue_IntScaled(left);
            int64_t b = _PyValue_IntScaled(right);
            int64_t difference;
            STAT_INC(BINARY_OP, hit);
            if (__builtin_sub_overflow(a, b, &difference)) {
                res = _PyValue_BoxInt64(PyValue_InlineInt64(left) - PyValue_InlineInt64(right));
            }
            else {
                res = _PyValue_FromIntScaled(difference);
            }
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_MULTIPLY_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_MULTIPLY_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_ADD_INT) region(inline) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_ADD_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_SUBTRACT_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_SUBTRACT_INT + _POP_TOP_INT + _POP_TOP_INT;

        // Python's // rounds the quotient towards minus infinity and % takes
        // the sign of the divisor, where C truncates. Division by zero
        // exits, and BINARY_OP raises. The operands are int51, so int64
        // division does not trap; only Py_INT_MIN // -1 leaves the inline
        // range, and PyValue_FromInt64() boxes it.
        leaf inline_ok pure op(_BINARY_OP_FLOOR_DIVIDE_INT,
                (left: taken inline_int, right: taken inline_int
                 -- res: owned exact(PyLong_Type), l: moved(left), r: moved(right)))
            effects(allocates, raises) exits(next, miss, raise)
        {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b == 0, "the divisor is zero");
            int64_t a = PyValue_InlineInt64(left);
            STAT_INC(BINARY_OP, hit);
            // One division: the quotient is inexact when it does not
            // multiply back, and the operands differ in sign
            int64_t quotient = a / b;
            if (quotient * b != a && (a ^ b) < 0) {
                quotient--;
            }
            res = PyValue_FromInt64(quotient);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        // The remainder is smaller than the divisor in magnitude, so it is
        // inline.
        leaf inline_ok pure op(_BINARY_OP_REMAINDER_INT,
                (left: taken inline_int, right: taken inline_int
                 -- res: owned exact(PyLong_Type) inline_int, l: moved(left), r: moved(right)))
            effects(raises) exits(next, miss, raise)
        {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b == 0, "the divisor is zero");
            int64_t a = PyValue_InlineInt64(left);
            STAT_INC(BINARY_OP, hit);
            int64_t remainder = a % b;
            if (remainder != 0 && (remainder ^ b) < 0) {
                remainder += b;
            }
            res = PyValue_FromInlineInt(remainder);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        // Ints behave as infinite two's complement under &, | and ^, so the
        // result for int51 operands is the int51 result.
        inline_ok pure op(_BINARY_OP_AND_INT,
                (left: taken exact(PyLong_Type) inline inline_int, right: taken exact(PyLong_Type) inline inline_int
                 -- res: owned exact(PyLong_Type) inline_int, l: moved(left), r: moved(right)))
            effects(raises) exits(next, raise)
        {
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromInlineInt(PyValue_InlineInt64(left) & PyValue_InlineInt64(right));
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        inline_ok pure op(_BINARY_OP_OR_INT,
                (left: taken exact(PyLong_Type) inline inline_int, right: taken exact(PyLong_Type) inline inline_int
                 -- res: owned exact(PyLong_Type) inline_int, l: moved(left), r: moved(right)))
            effects(raises) exits(next, raise)
        {
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromInlineInt(PyValue_InlineInt64(left) | PyValue_InlineInt64(right));
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        inline_ok pure op(_BINARY_OP_XOR_INT,
                (left: taken exact(PyLong_Type) inline inline_int, right: taken exact(PyLong_Type) inline inline_int
                 -- res: owned exact(PyLong_Type) inline_int, l: moved(left), r: moved(right)))
            effects(raises) exits(next, raise)
        {
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromInlineInt(PyValue_InlineInt64(left) ^ PyValue_InlineInt64(right));
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        // A negative shift count exits, and BINARY_OP raises. A shift whose
        // result fits in int64 is made there, and PyValue_FromInt64() boxes
        // it if it leaves the inline range; a longer shift of anything but 0
        // makes a long int.
        inline_ok pure op(_BINARY_OP_LSHIFT_INT,
                (left: taken inline_int, right: taken inline_int
                 -- res: owned exact(PyLong_Type), l: moved(left), r: moved(right)))
            effects(allocates, raises) exits(next, miss, raise)
        {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b < 0, "the shift count is negative");
            int64_t a = PyValue_InlineInt64(left);
            STAT_INC(BINARY_OP, hit);
            int64_t shifted = b < 63 ? (int64_t)((uint64_t)a << b) : 0;
            if (a == 0) {
                res = PyValue_FromInlineInt(0);
            }
            else if (b < 63 && Py_ARITHMETIC_RIGHT_SHIFT(int64_t, shifted, b) == a) {
                res = PyValue_FromInt64(shifted);
            }
            else {
                res = _PyValue_LshiftInt64(a, b);
            }
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        // Python's >> rounds towards minus infinity, as an arithmetic shift
        // does; shifting an int51 by 63 places or more leaves its sign.
        inline_ok pure op(_BINARY_OP_RSHIFT_INT,
                (left: taken inline_int, right: taken inline_int
                 -- res: owned exact(PyLong_Type) inline_int, l: moved(left), r: moved(right)))
            effects(raises) exits(next, miss, raise)
        {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b < 0, "the shift count is negative");
            int64_t a = PyValue_InlineInt64(left);
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromInlineInt(Py_ARITHMETIC_RIGHT_SHIFT(int64_t, a, b < 63 ? b : 63));
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_FLOOR_DIVIDE_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_FLOOR_DIVIDE_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_REMAINDER_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_REMAINDER_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_AND_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_AND_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_OR_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_OR_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_XOR_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_XOR_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_LSHIFT_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_LSHIFT_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_RSHIFT_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_RSHIFT_INT + _POP_TOP_INT + _POP_TOP_INT;

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

        // Float arithmetic takes two inline floats, or an inline float and
        // an inline int (see PyValue_FloatOperandsCheck()). The test and the
        // reads are one step, so that two floats get a path of their own.
        // The result is inline unless it is NaN.
        leaf inline_ok pure op(_BINARY_OP_MULTIPLY_FLOAT,
                (left: taken, right: taken
                 -- res: owned exact(PyFloat_Type), l: moved(left) inline, r: moved(right) inline))
            effects(allocates, raises) exits(next, miss, raise)
        {
            double a, b;
            EXIT_IF(!PyValue_FloatOperandValues(left, right, &a, &b));
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromDouble(a * b);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        leaf inline_ok pure op(_BINARY_OP_ADD_FLOAT,
                (left: taken, right: taken
                 -- res: owned exact(PyFloat_Type), l: moved(left) inline, r: moved(right) inline))
            effects(allocates, raises) exits(next, miss, raise)
        {
            double a, b;
            EXIT_IF(!PyValue_FloatOperandValues(left, right, &a, &b));
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromDouble(a + b);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        leaf inline_ok pure op(_BINARY_OP_SUBTRACT_FLOAT,
                (left: taken, right: taken
                 -- res: owned exact(PyFloat_Type), l: moved(left) inline, r: moved(right) inline))
            effects(allocates, raises) exits(next, miss, raise)
        {
            double a, b;
            EXIT_IF(!PyValue_FloatOperandValues(left, right, &a, &b));
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromDouble(a - b);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        // Division by zero exits, and BINARY_OP raises.
        leaf inline_ok pure op(_BINARY_OP_TRUEDIV_FLOAT,
                (left: taken, right: taken
                 -- res: owned exact(PyFloat_Type), l: moved(left) inline, r: moved(right) inline))
            effects(allocates, raises) exits(next, miss, raise)
        {
            double a, b;
            EXIT_IF(!PyValue_FloatOperandValues(left, right, &a, &b));
            EXIT_IF(b == 0.0, "the divisor is zero");
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromDouble(a / b);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_MULTIPLY_FLOAT) =
            unused/5 + _BINARY_OP_MULTIPLY_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;
        macro(BINARY_OP_ADD_FLOAT) region(inline) =
            unused/5 + _BINARY_OP_ADD_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;
        macro(BINARY_OP_SUBTRACT_FLOAT) =
            unused/5 + _BINARY_OP_SUBTRACT_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;
        macro(BINARY_OP_TRUEDIV_FLOAT) =
            unused/5 + _BINARY_OP_TRUEDIV_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;

        pure op(_BINARY_OP_ADD_UNICODE,
                (left: taken object, right: taken object
                 -- res: owned, l: moved(left), r: moved(right)))
            effects(reads, allocates, raises) exits(next, raise)
        {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
            assert(PyUnicode_CheckExact(left_o));
            assert(PyUnicode_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = PyUnicode_Concat(left_o, right_o);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            res = PyValue_FromNonNumberSteal(res_o);
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_ADD_UNICODE) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_ADD_UNICODE + _POP_TOP_UNICODE + _POP_TOP_UNICODE;

        // This is a subtle one. We write NULL to the local that dst
        // names, the target of the STORE_FAST folded into the word, and
        // leave the result for IR_STORE_RESULT to store.
        op(_BINARY_OP_INPLACE_ADD_UNICODE,
                (left: consumed exact(PyUnicode_Type), right: consumed exact(PyUnicode_Type)
                 -- res: owned))
            effects(reads, writes, allocates, raises, releases, runs_python) exits(next, miss, raise)
        {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            assert(PyUnicode_CheckExact(left_o));
            assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(right)));

            assert(IR_IS_LOCAL(IR_DST));
            assert(PyUnicode_CheckExact(left_o));
            // The local may hold any word (an inline number, None, NULL):
            // compare the words, not an object read from it.  `held` is a
            // copy; the local keeps its owner until it is cleared below
            PyValue held = IR_SLOT(IR_DST);
            EXIT_IF(!PyValue_Is(held, left),
                    "the next word stores the result to the left operand's local");
            STAT_INC(BINARY_OP, hit);
            /* Handle `left = left + right` or `left += right` for str.
             *
             * When possible, extend `left` in place rather than
             * allocating a new PyUnicodeObject. This attempts to avoid
             * quadratic behavior when one neglects to use str.join().
             *
             * If `left` has only two references remaining (one from
             * the stack, one in the locals), DECREFing `left` leaves
             * only the locals reference, so PyUnicode_Append knows
             * that the string is safe to mutate.  A `left` read in place
             * from the local holds no reference of its own, and the
             * generator does not release it (pyjit/design/pyvalue.md D4).
             */
            /* gh-143403: It's critical to close this reference *before*
             * we append. Otherwise, append can move the underlying
             * unicode object, which will cause a use after free!
             * It is also closed before the local is stolen, because
             * `left` may be a borrow of that local (gh-131527).
             */
            PyValue_CLOSE_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            DEAD(left);
            // The local still holds the string; its owner moves to `held`,
            // which the guard showed is left's string
            assert(Py_REFCNT(left_o) >= 1);
            IR_SLOT(IR_DST) = PyValue_NULL;
            PyObject *temp = PyValue_AsKnownObjectSteal(held);
            PyObject *right_o = PyValue_AsKnownObjectSteal(right);
            temp = value_unicode_append(temp, right_o);
            _Py_DECREF_SPECIALIZED(right_o, _PyUnicode_ExactDealloc);
            ERROR_IF(temp == NULL);
            res = PyValue_FromNonNumberSteal(temp);
        }

       // These legacy descriptors guard objects. Decline canonical inline
       // operands before creating a C bridge; the generic operation handles
       // them without carrying private box owners through a deopt.
       inline_ok op(_BINARY_OP_EXTEND,
               (descr/4, left: taken, right: taken
                -- res: owned, l: moved(left) object, r: moved(right) object))
           effects(runs_python) exits(next, miss, raise)
       {
            EXIT_IF(!PyValue_IsObject(left) || !PyValue_IsObject(right));
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
            assert(INLINE_CACHE_ENTRIES_BINARY_OP == 5);
            _PyBinaryOpSpecializationDescr *d = _PyBinaryOp_ExtendDescr(descr);
            assert(d != NULL);
            EXIT_IF(Py_TYPE(left_o) != d->lhs_type || Py_TYPE(right_o) != d->rhs_type,
                    "the operand types are not the descriptor's");

            STAT_INC(BINARY_OP, hit);

            binaryopactionfunc action = d->action;
            PyObject *res_o = action(left_o, right_o);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            assert(d->result_type == NULL || Py_TYPE(res_o) == d->result_type);
            assert(!d->result_unique || Py_REFCNT(res_o) == 1 || _Py_IsImmortal(res_o));

            res = PyValue_FromObjectSteal(res_o);
            l = left;
            r = right;
            DEAD(left);
            DEAD(right);
        }

        macro(BINARY_OP_EXTEND) =
            unused/1 + _BINARY_OP_EXTEND + _POP_TOP_ANY + _POP_TOP_ANY;

        macro(BINARY_OP_INPLACE_ADD_UNICODE) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_INPLACE_ADD_UNICODE;

        specializing op(_SPECIALIZE_BINARY_SLICE,
                (container, start, stop -- container, start, stop))
            effects() exits(next)
        {
            // Placeholder until we implement BINARY_SLICE specialization
            #if ENABLE_SPECIALIZATION
            OPCODE_DEFERRED_INC(BINARY_SLICE);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // Inline bounds slice a list, tuple or str without boxes or a slice
        // object; see _PyEval_BinarySlice().
        inline_ok op(_BINARY_SLICE,
                (container: consumed, start: consumed, stop: consumed -- res: owned))
            effects(runs_python) exits(next, raise)
        {
            res = _PyEval_BinarySlice(container, start, stop);
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(res));
        }

        outlined macro(BINARY_SLICE) = _SPECIALIZE_BINARY_SLICE + _BINARY_SLICE;

        specializing op(_SPECIALIZE_STORE_SLICE,
                (v, container, start, stop -- v, container, start, stop))
            effects() exits(next)
        {
            // Placeholder until we implement STORE_SLICE specialization
            #if ENABLE_SPECIALIZATION
            OPCODE_DEFERRED_INC(STORE_SLICE);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // Inline bounds store to a list slice without boxes or a slice
        // object; see _PyEval_StoreSlice().
        inline_ok op(_STORE_SLICE,
                (v: consumed, container: consumed, start: consumed, stop: consumed --))
            effects(runs_python) exits(next, raise)
        {
            int err = _PyEval_StoreSlice(v, container, start, stop);
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        outlined macro(STORE_SLICE) = _SPECIALIZE_STORE_SLICE + _STORE_SLICE;

        leaf inline_ok op(_GUARD_ELEMENT_INT, (value -- value: inline_int))
            effects() exits(next, miss)
        {
            IR_GUARD_INLINE_INT(value);
        }

        leaf inline_ok op(_GUARD_ELEMENT_LIST,
                (container, unused -- container: exact(PyList_Type), unused))
            effects(reads) exits(next, miss)
        {
            IR_GUARD_ELEMENT_TYPE(container, PyList_Type);
        }

        leaf inline_ok op(_GUARD_ELEMENT_TUPLE,
                (container, unused -- container: exact(PyTuple_Type), unused))
            effects(reads) exits(next, miss)
        {
            IR_GUARD_ELEMENT_TYPE(container, PyTuple_Type);
        }

        macro(BINARY_OP_SUBSCR_LIST_INT) =
            _GUARD_ELEMENT_INT + _GUARD_ELEMENT_LIST + unused/5 + _BINARY_OP_SUBSCR_LIST_INT + _POP_TOP_INT + _POP_TOP_ANY;

        leaf inline_ok op(_BINARY_OP_SUBSCR_LIST_INT,
                (list_st: consumed, sub_st: consumed
                 -- res: owned, ls: moved(list_st), ss: moved(sub_st) in_bounds(list_st)))
            effects(reads) exits(next, miss)
        {
            IR_GUARD_ELEMENT_INDEX(list_st, sub_st, index, FROM_END);
            res = IR_DUP_CANONICAL_ELEMENT(list_st, index);
            assert(!PyValue_IsNull(res));
            STAT_INC(BINARY_OP, hit);
            ls = list_st;
            ss = sub_st;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_SUBSCR_LIST_SLICE) =
            _GUARD_TOS_SLICE + _GUARD_NOS_LIST + unused/5 + _BINARY_OP_SUBSCR_LIST_SLICE + _POP_TOP_ANY + _POP_TOP_ANY;

        op(_BINARY_OP_SUBSCR_LIST_SLICE,
                (list_st: taken object, sub_st: taken object
                 -- res: owned, ls: moved(list_st), ss: moved(sub_st)))
            effects(runs_python) exits(next, raise)
        {
            PyObject *sub = PyValue_AsKnownObjectBorrow(sub_st);
            PyObject *list = PyValue_AsKnownObjectBorrow(list_st);

            assert(PySlice_Check(sub));
            assert(PyList_CheckExact(list));

            PyObject *res_o = _PyList_SliceSubscript(list, sub);
            STAT_INC(BINARY_OP, hit);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            res = PyValue_FromNonNumberSteal(res_o);
            ls = list_st;
            ss = sub_st;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_SUBSCR_STR_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_COMPACT_ASCII + unused/5 + _BINARY_OP_SUBSCR_STR_INT + _POP_TOP_INT + _POP_TOP_UNICODE;

        inline_ok op(_BINARY_OP_SUBSCR_STR_INT,
                (str_st: consumed object, sub_st: consumed inline_int
                 -- res: owned, s: moved(str_st), i: moved(sub_st)))
            effects(reads) exits(next, miss)
        {
            PyObject *str = PyValue_AsKnownObjectBorrow(str_st);
            assert(PyUnicode_CheckExact(str));

            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            EXIT_IF(index < 0, "the index is negative");
            EXIT_IF(PyUnicode_GET_LENGTH(str) <= index, "the index is out of range");
            uint8_t c = PyUnicode_1BYTE_DATA(str)[index];
            assert(c < 128);
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = (PyObject*)&_Py_SINGLETON(strings).ascii[c];
            s = str_st;
            i = sub_st;
            INPUTS_DEAD();
            res = PyValue_DUP(PyValue_FromObjectBorrow(res_o));
        }

        macro(BINARY_OP_SUBSCR_USTR_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_SUBSCR_USTR_INT + _POP_TOP_INT + _POP_TOP_UNICODE;

        inline_ok op(_BINARY_OP_SUBSCR_USTR_INT,
                (str_st: consumed object, sub_st: consumed inline_int
                 -- res: owned, s: moved(str_st), i: moved(sub_st)))
            effects(reads) exits(next, miss)
        {
            PyObject *str = PyValue_AsKnownObjectBorrow(str_st);
            assert(PyUnicode_CheckExact(str));

            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            EXIT_IF(index < 0, "the index is negative");
            EXIT_IF(PyUnicode_GET_LENGTH(str) <= index, "the index is out of range");
            // Specialize for reading an ASCII character from any string:
            Py_UCS4 c = PyUnicode_READ_CHAR(str, index);
            EXIT_IF(Py_ARRAY_LENGTH(_Py_SINGLETON(strings).ascii) <= c,
                    "the character is not ASCII");
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = (PyObject*)&_Py_SINGLETON(strings).ascii[c];
            s = str_st;
            i = sub_st;
            INPUTS_DEAD();
            res = PyValue_DUP(PyValue_FromObjectBorrow(res_o));
        }

        op(_GUARD_NOS_TUPLE, (nos: object, unused -- nos: exact(PyTuple_Type), unused))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        op(_GUARD_TOS_TUPLE, (tos: object -- tos: exact(PyTuple_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        macro(BINARY_OP_SUBSCR_TUPLE_INT) =
            _GUARD_ELEMENT_INT +
            _GUARD_ELEMENT_TUPLE +
            unused/5 +
            _BINARY_OP_SUBSCR_TUPLE_INT +
            _POP_TOP_INT +
            _POP_TOP_ANY;

        leaf inline_ok op(_BINARY_OP_SUBSCR_TUPLE_INT,
                (tuple_st: consumed, sub_st: consumed
                 -- res: owned, ts: moved(tuple_st), ss: moved(sub_st) in_bounds(tuple_st)))
            effects(reads) exits(next, miss)
        {
            IR_GUARD_ELEMENT_INDEX(tuple_st, sub_st, index, NONNEGATIVE);
            res = IR_DUP_CANONICAL_ELEMENT(tuple_st, index);
            STAT_INC(BINARY_OP, hit);
            assert(!PyValue_IsNull(res));
            ts = tuple_st;
            ss = sub_st;
            INPUTS_DEAD();
        }

        op(_GUARD_NOS_DICT_SUBSCRIPT, (nos: object, unused -- nos, unused))
            effects(reads) exits(next, miss, deopt)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            DEOPT_IF(!Py_TYPE(o)->tp_as_mapping, "the type has no mapping slots");
            DEOPT_IF(Py_TYPE(o)->tp_as_mapping->mp_subscript != _PyDict_Subscript,
                     "the type's mp_subscript is not dict's");
        }

        op(_GUARD_NOS_DICT_STORE_SUBSCRIPT,
                (unused, nos: object, unused -- unused, nos, unused))
            effects(reads) exits(next, miss, deopt)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            DEOPT_IF(!Py_TYPE(o)->tp_as_mapping, "the type has no mapping slots");
            DEOPT_IF(Py_TYPE(o)->tp_as_mapping->mp_ass_subscript != _PyDict_StoreSubscript,
                     "the type's mp_ass_subscript is not dict's");
        }

        op(_GUARD_TOS_ANY_DICT, (tos: object -- tos: exact(PyAnyDict_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyAnyDict_CheckExact(o));
        }

        macro(BINARY_OP_SUBSCR_DICT) =
            _GUARD_NOS_DICT_SUBSCRIPT + unused/5 + _BINARY_OP_SUBSCR_DICT;

        inline_ok op(_BINARY_OP_SUBSCR_DICT,
                (dict_st: consumed, sub_st: consumed -- res: owned))
            effects(reads, writes, raises, releases, runs_python) exits(next, raise)
        {
            IR_TRY_DICT_FIRST_PROBE();
            STAT_INC(BINARY_OP, hit);
            res = value_subscript(dict_st, sub_st);
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(res));
        }

        inline_ok op(_BINARY_OP_SUBSCR_CHECK_FUNC,
                (container, unused -- container: object, unused, getitem: owned))
            effects(reads) exits(next, miss)
        {
            EXIT_IF(!PyValue_IsObject(container));
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(container));
            EXIT_IF(!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE), "the type is not a heap type");
            PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
            PyObject *getitem_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(ht->_spec_cache.getitem);
            EXIT_IF(getitem_o == NULL, "the type caches no __getitem__");
            assert(PyFunction_Check(getitem_o));
            uint32_t cached_version = FT_ATOMIC_LOAD_UINT32_RELAXED(ht->_spec_cache.getitem_version);
            EXIT_IF(((PyFunctionObject *)getitem_o)->func_version != cached_version,
                    "the cached __getitem__ changed version");
            PyCodeObject *code = (PyCodeObject *)PyFunction_GET_CODE(getitem_o);
            assert(code->co_argcount == 2);
            EXIT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize),
                    "the thread has no stack space for the frame");
            getitem = PyValue_DUP(PyValue_FromObjectBorrow(getitem_o));
        }

        op(_BINARY_OP_SUBSCR_INIT_CALL,
                (container: consumed, sub: consumed, getitem: consumed -- new_frame: owned))
            effects(reads, writes, allocates) exits(next)
        {
            STAT_INC(BINARY_OP, hit);
            _PyInterpreterFrame* pushed_frame = _PyFrame_PushUnchecked(tstate, getitem, frame);
            pushed_frame->base[0] = TAKE(container);
            pushed_frame->base[1] = TAKE(sub);
            INPUTS_DEAD();
            frame->return_slot = (uint16_t)IR_DST_SLOT;
            frame->return_pc = next_instr;
            new_frame = PyValue_Wrap(pushed_frame);
        }

        macro(BINARY_OP_SUBSCR_GETITEM) =
            unused/5 + // Skip over the entry distance and reserved units
            _CHECK_PEP_523 +
            _BINARY_OP_SUBSCR_CHECK_FUNC +
            _BINARY_OP_SUBSCR_INIT_CALL +
            _PUSH_FRAME;

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

        family(STORE_SUBSCR, INLINE_CACHE_ENTRIES_STORE_SUBSCR) = {
            STORE_SUBSCR_DICT,
            STORE_SUBSCR_LIST_INT,
        };

        specializing op(_SPECIALIZE_STORE_SUBSCR, (container, sub -- container, sub))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_StoreSubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(STORE_SUBSCR);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        inline_ok op(_STORE_SUBSCR, (v: consumed, container: consumed, sub: consumed -- ))
            effects(runs_python) exits(next, raise)
        {
            /* container[sub] = v */
            int err = _PyValue_SetItem(container, sub, v);
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        macro(STORE_SUBSCR) = unused/1 + _SPECIALIZE_STORE_SUBSCR + _STORE_SUBSCR;

        // The index is released by _POP_TOP_ANY, which does not read it: the
        // release of the replaced item before it can run code (S7)
        macro(STORE_SUBSCR_LIST_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_LIST + unused/1 + _STORE_SUBSCR_LIST_INT + _POP_TOP_ANY + _POP_TOP_ANY;

        // The index and stored value stay in canonical representation.
        leaf inline_ok op(_STORE_SUBSCR_LIST_INT,
                (value: consumed, list_st: taken exact(PyList_Type), sub_st: taken inline_int
                 -- ls: moved(list_st) exact(PyList_Type), ss: moved(sub_st) inline_int))
            effects(reads, writes, releases) exits(next, deopt)
        {
            PyObject *list = PyValue_AsKnownObjectBorrow(list_st);
            assert(PyList_CheckExact(list));

            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            DEOPT_IF(!LOCK_OBJECT(list));
            Py_ssize_t len = PyList_GET_SIZE(list);
            // Ensure index < len(list)
            if (index < 0) {
                index += len;
            }
            if (index < 0 || index >= len) {
                UNLOCK_OBJECT(list);
                DEOPT_IF(true, "the index is out of range");
            }
            STAT_INC(STORE_SUBSCR, hit);

            // A list's items are never NULL, and the index is in range
            PyValue item = TAKE(value);
            PyValue old_value = _PyValueSlot_ExchangeFilled(_PyList_ITEMS(list) + index, item);
            UNLOCK_OBJECT(list);  // unlock before decrefs!
            ls = list_st;
            ss = sub_st;
            INPUTS_DEAD();
            PyValue_CLOSE(old_value);
        }

        macro(STORE_SUBSCR_DICT) =
            _GUARD_NOS_DICT_STORE_SUBSCRIPT + unused/1 + _STORE_SUBSCR_DICT;

        inline_ok op(_STORE_SUBSCR_DICT,
                (value: consumed, dict_st: consumed object, sub: consumed --))
            effects(runs_python) exits(next, raise)
        {
            PyObject *dict = PyValue_AsKnownObjectBorrow(dict_st);
            assert(Py_TYPE(dict)->tp_as_mapping->mp_ass_subscript == _PyDict_StoreSubscript);
            STAT_INC(STORE_SUBSCR, hit);
            int err = _PyDict_SetValue((PyDictObject *)dict, sub, value);
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        outlined inst(DELETE_SUBSCR, (container: consumed, sub: consumed --))
            effects(runs_python) exits(next, raise)
        {
            /* del container[sub] */
            int err = _PyValue_DelItem(container, sub);
            DECREF_INPUTS();
            ERROR_IF(err);
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

        outlined tier1 inst(RAISE_VARARGS, (args[oparg]: consumed -- ))
            effects(runs_python) exits(raise)
        {
            assert(oparg < 3);
            _PyValue_RecheckWindow(args, oparg);
            PyObject *exc = NULL;
            PyObject *cause = NULL;
            if (oparg > 0) {
                // A number is not an exception
                exc = PyValue_ObjectOrNull(args[0]);
                if (exc == NULL) {
                    _PyErr_SetString(tstate, PyExc_TypeError,
                                     "exceptions must derive from BaseException");
                    ERROR_IF(true, holding(args));
                }
            }
            // A number is not a cause: do_raise reports it after the
            // exception is made
            bool cause_is_number = false;
            if (oparg == 2) {
                cause = PyValue_ObjectOrNull(args[1]);
                cause_is_number = cause == NULL;
            }
            /* do_raise consumes independent object owners, made before
             * the inputs are released */
            PyObject *exc_owner = Py_XNewRef(exc);
            PyObject *cause_owner = Py_XNewRef(cause);
            DECREF_INPUTS();
            int err = do_raise(tstate, exc_owner, cause_owner, cause_is_number);
            if (err) {
                assert(oparg == 0);
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
            ERROR_IF(true);
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

        macro(FOR_ITER) = unused/1 + _SPECIALIZE_FOR_ITER + _FOR_ITER;

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

        outlined inst(POP_EXCEPT, (exc_value: consumed assumed(object) by _PUSH_EXC_INFO -- ))
            effects(reads, releases) exits(next)
        {
            _PyErr_StackItem *exc_info = tstate->exc_info;
            if (PyValue_IsNone(exc_value)) {
                DEAD(exc_value);    // immortal: it owes no count
                Py_CLEAR(exc_info->exc_value);
            }
            else {
                PyObject *exc = PyValue_AsKnownObjectSteal(exc_value);
                Py_XSETREF(exc_info->exc_value, exc);
            }
        }

        outlined tier1 inst(RERAISE,
                (values[oparg], exc_st: consumed assumed(object) by exception_unwind -- values[oparg]))
            effects(runs_python) exits(raise)
        {
            // An exception being handled is an instance of BaseException;
            // the generator asserts the declared assumed(object)
            PyObject *exc = PyValue_AsKnownObjectSteal(exc_st);

            assert(oparg >= 0 && oparg <= 2);
            if (oparg) {
                frame->instr_ptr = INSTR_AT_OFFSET(PyValue_UntagIndex(values[0]));
            }
            assert(exc && PyExceptionInstance_Check(exc));
            _PyErr_SetRaisedException(tstate, exc);
            monitor_reraise(tstate, frame, this_instr);
            goto exception_unwind;
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

        family(UNPACK_SEQUENCE, INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE) = {
            UNPACK_SEQUENCE_TUPLE,
            UNPACK_SEQUENCE_LIST,
        };

        specializing op(_SPECIALIZE_UNPACK_SEQUENCE, (seq -- seq))
            effects(runs_python) exits(next, jump)
            feedback(read(entry), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_UnpackSequence(seq, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(UNPACK_SEQUENCE);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
            (void)seq;
        }

        window op(_UNPACK_SEQUENCE, (seq: consumed -- unused[oparg], top[0]: owned))
            effects(runs_python) exits(next, raise)
        {
            // A number is not iterable
            if (!PyValue_IsObject(seq)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "cannot unpack non-iterable %.200s object",
                              PyValue_TYPE(seq)->tp_name);
                DEAD(seq);
                ERROR_IF(true);
            }
            PyObject *seq_o = PyValue_AsKnownObjectSteal(seq);
            int res = _PyEval_UnpackIterableValue(tstate, seq_o, oparg, -1, top);
            ERROR_IF(res == 0);
        }

        macro(UNPACK_SEQUENCE) region(shared) = unused/1 + _SPECIALIZE_UNPACK_SEQUENCE + _UNPACK_SEQUENCE;

        macro(UNPACK_SEQUENCE_TUPLE) region(shared) =
            _GUARD_TOS_TUPLE + unused/1 + _UNPACK_SEQUENCE_TUPLE;

        op(_UNPACK_SEQUENCE_TUPLE, (seq: consumed exact(PyTuple_Type) -- values[oparg]: owned))
            effects(reads, releases) exits(next, miss)
        {
            PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
            assert(PyTuple_CheckExact(seq_o));
            EXIT_IF(PyTuple_GET_SIZE(seq_o) != oparg);
            STAT_INC(UNPACK_SEQUENCE, hit);
            IR_UNPACK_ITEMS(values, _PyTuple_ITEMS(seq_o), oparg);
            DECREF_INPUTS();
        }

        macro(UNPACK_SEQUENCE_LIST) region(shared) =
            _GUARD_TOS_LIST + unused/1 + _UNPACK_SEQUENCE_LIST;

        op(_UNPACK_SEQUENCE_LIST, (seq: consumed exact(PyList_Type) -- values[oparg]: owned))
            effects(reads, releases) exits(next, deopt)
        {
            PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
            assert(PyList_CheckExact(seq_o));
            DEOPT_IF(!LOCK_OBJECT(seq_o));
            if (PyList_GET_SIZE(seq_o) != oparg) {
                UNLOCK_OBJECT(seq_o);
                DEOPT_IF(true);
            }
            STAT_INC(UNPACK_SEQUENCE, hit);
            IR_UNPACK_ITEMS(values, _PyList_ITEMS(seq_o), oparg);
            UNLOCK_OBJECT(seq_o);
            DECREF_INPUTS();
        }

        outlined window inst(UNPACK_EX,
                (seq: consumed
                 -- unused[oparg & 0xFF], unused, unused[oparg >> 8], top[0]: owned))
            effects(runs_python) exits(next, raise)
        {
            // A number is not iterable
            if (!PyValue_IsObject(seq)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "cannot unpack non-iterable %.200s object",
                              PyValue_TYPE(seq)->tp_name);
                DEAD(seq);
                ERROR_IF(true);
            }
            PyObject *seq_o = PyValue_AsKnownObjectSteal(seq);
            int res = _PyEval_UnpackIterableValue(tstate, seq_o, oparg & 0xFF, oparg >> 8, top);
            ERROR_IF(res == 0);
        }

        family(STORE_ATTR, INLINE_CACHE_ENTRIES_STORE_ATTR) = {
            STORE_ATTR_INSTANCE_VALUE,
            STORE_ATTR_SLOT,
            STORE_ATTR_WITH_HINT,
            STORE_ATTR_SHAPE,
            STORE_ATTR_SHAPE_ADD,
        };

        specializing op(_SPECIALIZE_STORE_ATTR, (v, owner -- v, owner))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                if (!PyValue_IsNull(v)) {
                    PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
                    next_instr = this_instr;
                    _Py_Specialize_StoreAttr(owner, next_instr, name);
                    DISPATCH_SAME_OPARG();
                }
            }
            OPCODE_DEFERRED_INC(STORE_ATTR);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        inline_ok op(_STORE_ATTR, (v: consumed, owner: consumed --))
            effects(runs_python) exits(next, raise)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = _PyValue_SetAttr(owner, name, v);
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        macro(STORE_ATTR) = unused/1 + _SPECIALIZE_STORE_ATTR + unused/3 + _STORE_ATTR;

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

        family(LOAD_GLOBAL, INLINE_CACHE_ENTRIES_LOAD_GLOBAL) = {
            LOAD_GLOBAL_MODULE,
            LOAD_GLOBAL_BUILTIN,
        };

        specializing op(_SPECIALIZE_LOAD_GLOBAL, ( -- ))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_GLOBAL);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_LOAD_GLOBAL, ( -- res[1]: owned))
            effects(runs_python) exits(next, raise)
        {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            PyValue found = value_load_global(GLOBALS(), BUILTINS(), name);
            ERROR_IF(PyValue_IsNull(found));
            IR_WRITE_OUTPUT(res, 0, found);
        }

        // The NULL of a call's window needs no write: the slot is NULL by the
        // NULL invariant, and the assembler clears the bit in the word
        // (register-ir.md G4).  The stack model keeps the output.
        leaf op(_NULL_SLOT_CONDITIONAL, ( -- unused[oparg & 1]))
            effects() exits(next)
        {
        }

        macro(LOAD_GLOBAL) =
            unused/1 +
            _SPECIALIZE_LOAD_GLOBAL +
            unused/3 +
            _LOAD_GLOBAL +
            _NULL_SLOT_CONDITIONAL;

        // Globals are always a dict, and a keys object has one owner and a
        // version that is never reused, so a match on the keys version
        // names the dict the specialiser saw.  Builtins can be any object,
        // so _LOAD_GLOBAL_BUILTINS still checks the type before it reads
        // the keys.
        leaf op(_GUARD_GLOBALS_VERSION, (globals_version/1 --))
            effects(reads) exits(next, deopt) paths(globals: keys_version(globals_version))
        {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != globals_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
        }

        leaf op(_LOAD_GLOBAL_MODULE, (globals_version/1, unused/1, index/1 -- res: owned))
            effects(reads) exits(next, deopt) paths(globals: keys_version(globals_version))
        {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != globals_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            DEOPT_IF(index >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries),
                     "the index is past the entries of the keys");
            PyValueBorrow found = PyValue_Borrow(entries[index].me_value);
            DEOPT_IF(PyValue_IsNull(found));
            res = PyValue_XDUP(found);
            STAT_INC(LOAD_GLOBAL, hit);
        }

        leaf op(_LOAD_GLOBAL_BUILTINS, (builtins_version/1, index/1 -- res: owned))
            effects(reads) exits(next, deopt) paths(builtins: keys_version(builtins_version))
        {
            PyDictObject *dict = (PyDictObject *)BUILTINS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != builtins_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            DEOPT_IF(index >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries),
                     "the index is past the entries of the keys");
            PyValueBorrow found = PyValue_Borrow(entries[index].me_value);
            DEOPT_IF(PyValue_IsNull(found));
            res = PyValue_XDUP(found);
            STAT_INC(LOAD_GLOBAL, hit);
        }

        macro(LOAD_GLOBAL_MODULE) =
            unused/1 + // Skip over the entry distance
            _LOAD_GLOBAL_MODULE +
            _NULL_SLOT_CONDITIONAL;

        macro(LOAD_GLOBAL_BUILTIN) =
            unused/1 + // Skip over the entry distance
            _GUARD_GLOBALS_VERSION +
            _LOAD_GLOBAL_BUILTINS +
            _NULL_SLOT_CONDITIONAL;

        outlined inst(MAKE_CELL, (--))
            effects(runs_python) exits(next, raise)
        {
            // "initial" is probably NULL but not if it's an arg (or set
            // via the f_locals proxy before MAKE_CELL has run).
            PyObject *cell = PyCell_NewValue(GETLOCAL(oparg));
            if (cell == NULL) {
                ERROR_NO_POP();
            }
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            GETLOCAL(oparg) = PyValue_FromObjectSteal(cell);
            PyValue_XCLOSE(tmp);
        }

        outlined inst(DELETE_DEREF, (--))
            effects(runs_python) exits(next, raise)
        {
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyObject *cell = PyValue_AsKnownObjectBorrow(cell_v);
            // Can't use ERROR_IF here.
            // Fortunately we don't need its superpower.
            PyValue old = _PyCell_SwapValue((PyCellObject *)cell, PyValue_NULL);
            if (PyValue_IsNull(old)) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_NO_POP();
            }
            PyValue_CLOSE(old);
        }

        // The operand is the class namespace, as for LOAD_FROM_DICT_OR_GLOBALS
        outlined inst(LOAD_FROM_DICT_OR_DEREF,
                (class_dict_st: consumed assumed(object) by compiler -- value: owned))
            effects(runs_python) exits(next, raise)
        {
            PyObject *name;
            // Also declared above; the generator's object analysis reads the assert
            assert(PyValue_IsObject(class_dict_st));
            PyObject *class_dict = PyValue_AsKnownObjectBorrow(class_dict_st);
            // Taken before the lookup, which can run code
            PyValue held_class_dict = TAKE(class_dict_st);

            assert(class_dict);
            assert(oparg >= 0 && oparg < _PyFrame_GetCode(frame)->co_nlocalsplus);
            name = _PyTuple_GET_OBJECT(_PyFrame_GetCode(frame)->co_localsplusnames, oparg);
            _PyStatusValue found = value_load_from_dict(tstate, class_dict, name);
            if (found.status < 0) {
                PyValue_CLOSE(held_class_dict);
                ERROR_NO_POP();
            }
            PyValue selected;
            if (found.status == 0) {
                PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
                PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(cell_v);
                selected = _PyCell_GetValue(cell);
                if (PyValue_IsNull(selected)) {
                    _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                    PyValue_CLOSE(held_class_dict);
                    ERROR_NO_POP();
                }
            }
            else {
                selected = found.value;
            }
            PyValue_CLOSE(held_class_dict);
            value = selected;
        }

        inst(LOAD_DEREF, ( -- value: owned))
            effects(runs_python) exits(next, raise)
        {
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(cell_v);
            value = _PyCell_GetValue(cell);
            if (PyValue_IsNull(value)) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_IF(true);
            }
        }

        inst(STORE_DEREF, (v: consumed --))
            effects(writes, releases) exits(next)
        {
            PyValueBorrow cell_v = _PyValue_CellLocal(GETLOCAL(oparg));
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(cell_v);
            PyValue owned = v;
            DEAD(v);
            _PyCell_SetValue(cell, owned);
        }

        outlined inst(COPY_FREE_VARS, (--))
            effects(reads, writes) exits(next)
        {
            /* Copy closure variables to free variables */
            PyCodeObject *co = _PyFrame_GetCode(frame);
            PyFunctionObject *func = _PyFrame_GetFunction(frame);
            PyObject *closure = func->func_closure;
            assert(oparg == co->co_nfreevars);
            int offset = co->co_nlocalsplus - oparg;
            for (int i = 0; i < oparg; ++i) {
                frame->base[offset + i] = PyValue_XDUP(PyTuple_GET_VALUE(closure, i));
            }
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

        macro(INSTRUMENTED_LOAD_SUPER_ATTR) =
            unused/1 +
            _LOAD_SUPER_ATTR +
            _NULL_SLOT_CONDITIONAL;

        family(LOAD_SUPER_ATTR, INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR) = {
            LOAD_SUPER_ATTR_ATTR,
            LOAD_SUPER_ATTR_METHOD,
        };

        specializing op(_SPECIALIZE_LOAD_SUPER_ATTR,
                (global_super_st, class_st, unused -- global_super_st, class_st, unused))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            int load_method = oparg & 1;
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_LoadSuperAttr(global_super_st, class_st, next_instr, load_method);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_SUPER_ATTR);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        tier1 op(_LOAD_SUPER_ATTR,
                (global_super_st: consumed, class_st: consumed, self_st: consumed
                 -- attr: owned))
            effects(runs_python) exits(next, raise)
        {
            // `super` and `__class__` can name numbers, which the call
            // reports; both stay values
            // we make no attempt to optimize here; specializations should
            // handle any case whose performance we care about
            PyValueBorrow arg = oparg & 2
                ? PyValue_Borrow(class_st)
                : PyValue_FromObjectBorrow(&_PyInstrumentation_MISSING);
            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_CALL,
                        frame, this_instr, global_super_st, arg);
                if (err) {
                    DECREF_INPUTS();
                    ERROR_IF(true);
                }
            }
            PyValue super = super_call(global_super_st, class_st, self_st, oparg & 2);
            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                if (PyValue_IsNull(super)) {
                    _Py_call_instrumentation_exc2(
                        tstate, PY_MONITORING_EVENT_C_RAISE,
                        frame, this_instr, global_super_st, arg);
                }
                else {
                    int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_C_RETURN,
                        frame, this_instr, global_super_st, arg);
                    if (err < 0) {
                        PyValue_CLEAR(super);
                    }
                }
            }
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(super));
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            attr = _PyValue_GetAttr(super, name);
            PyValue_CLOSE(super);
            ERROR_IF(PyValue_IsNull(attr));
        }

        macro(LOAD_SUPER_ATTR) =
            unused/1 + _SPECIALIZE_LOAD_SUPER_ATTR +
            _LOAD_SUPER_ATTR +
            _NULL_SLOT_CONDITIONAL;

        inline_ok inst(LOAD_SUPER_ATTR_ATTR,
                (unused/1, global_super_st: consumed, class_st: consumed, self_st: consumed
                 -- attr_st: owned))
            effects(runs_python) exits(next, miss, raise)
        {
            EXIT_IF(!PyValue_IsObject(global_super_st) || !PyValue_IsObject(class_st));
            PyObject *global_super = PyValue_AsKnownObjectBorrow(global_super_st);
            PyObject *class = PyValue_AsKnownObjectBorrow(class_st);

            assert(!(oparg & 1));
            EXIT_IF(global_super != (PyObject *)&PySuper_Type);
            EXIT_IF(!PyType_Check(class));
            STAT_INC(LOAD_SUPER_ATTR, hit);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            _PyStatusValue found = _PySuper_LookupValue((PyTypeObject *)class, self_st, name, 0);
            DECREF_INPUTS();
            ERROR_IF(found.status < 0);
            attr_st = found.value;
        }

        macro(LOAD_SUPER_ATTR_METHOD) =
            unused/1 +
            _GUARD_LOAD_SUPER_ATTR_METHOD +
            _LOAD_SUPER_ATTR_METHOD;

        op(_GUARD_LOAD_SUPER_ATTR_METHOD, (global_super_st: object, class_st: object, unused -- global_super_st: is(PySuper_Type), class_st: subtype(PyType_Type), unused))
            effects(reads) exits(next, miss)
        {
            PyObject *global_super = PyValue_AsKnownObjectBorrow(global_super_st);
            PyObject *class = PyValue_AsKnownObjectBorrow(class_st);
            assert(oparg & 1);
            EXIT_IF(global_super != (PyObject *)&PySuper_Type);
            EXIT_IF(!PyType_Check(class));
        }

        op(_LOAD_SUPER_ATTR_METHOD,
                (global_super_st: taken, class_st: taken object, self_st: taken
                 -- attr: owned, self_or_null: moved(self_st) nullable))
            effects(runs_python) exits(next, raise)
        {
            PyObject *class = PyValue_AsKnownObjectBorrow(class_st);

            STAT_INC(LOAD_SUPER_ATTR, hit);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            PyTypeObject *cls = (PyTypeObject *)class;
            // Only a type with the generic getattro can have a method
            // returned unbound
            _PyStatusValue found = _PySuper_LookupValue(cls, self_st, name,
                PyValue_TYPE(self_st)->tp_getattro == PyObject_GenericGetAttr);
            if (found.status < 0) {
                ERROR_NO_POP();
            }
            if (found.status) {
                self_or_null = self_st; // transfer ownership
                DEAD(self_st);
            } else {
                PyValue_CLOSE(self_st);
                self_or_null = PyValue_NULL;
            }
            DECREF_INPUTS();

            attr = found.value;
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

        leaf op(_GUARD_TYPE_VERSION,
                (type_version/2, owner: object -- owner: type_version(type_version)))
            effects(reads) exits(next, miss)
        {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(owner));
            assert(type_version != 0);
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version);
        }

        op(_GUARD_TYPE_VERSION_LOCKED,
                (type_version/2, owner: object -- owner: type_version(type_version)))
            effects(reads) exits(next, miss)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(type_version != 0);
            PyTypeObject *tp = Py_TYPE(owner_o);
            if (FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version) {
                UNLOCK_OBJECT(owner_o);
                EXIT_IF(true);
            }
        }

        leaf op(_CHECK_MANAGED_OBJECT_HAS_VALUES, (owner: object -- owner: has_values))
            effects(reads) exits(next, miss)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            EXIT_IF(!FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid));
        }



        /* The shape handlers (Py_SHAPES, see pycore_dict.h).  They follow a
         * type version guard, which proves that the owner has inline values
         * to read the shape from and lets descriptors added to the class
         * win.  The shape fields take no inline cache units.  Shapes exist
         * only in builds with the GIL, so the stores take no object lock and
         * need not check for a dict: an instance with a dict has no shape. */
        leaf op(_GUARD_SHAPE, (shape/0, owner: object -- owner))
            effects(reads) exits(next, miss)
        {
            EXIT_IF(_PyObject_Shape(PyValue_AsKnownObjectBorrow(owner)) != shape,
                    "the owner's inline values have another shape");
        }

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







        op(_GUARD_DORV_NO_DICT, (owner: object -- owner))
            effects(reads) exits(next, miss)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            if (_PyObject_GetManagedDict(owner_o) ||
                    !FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid)) {
                UNLOCK_OBJECT(owner_o);
                EXIT_IF(true, "the owner has a dict, or its inline values are not valid");
            }
        }

        inline_ok op(_STORE_ATTR_INSTANCE_VALUE,
                (offset/1, value: consumed, owner: consumed object --))
            effects(reads, writes, releases) exits(next, miss)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            assert(_PyObject_GetManagedDict(owner_o) == NULL);
            PyValue *value_ptr = _PyObject_ValueSlot(owner_o, offset);
#ifdef Py_SHAPES
            /* This store does not grow the type's shapes.  An addition whose
             * transition is missing goes to STORE_ATTR, which creates it, so
             * that the instance keeps a shape. */
            if (PyValue_IsNull(*value_ptr)) {
                PyDictValues *values = _PyObject_InlineValues(owner_o);
                if (_PyShape_TransitionMissing(Py_TYPE(owner_o), values->shape,
                                               value_ptr - values->values)) {
                    UNLOCK_OBJECT(owner_o);
                    EXIT_IF(true, "the type's shape tree lacks the transition");
                }
            }
#endif
            STAT_INC(STORE_ATTR, hit);
            PyValue item = TAKE(value);
            PyValue old_value = _PyValueSlot_Exchange(value_ptr, item);
            if (PyValue_IsNull(old_value)) {
                PyDictValues *values = _PyObject_InlineValues(owner_o);
                Py_ssize_t index = value_ptr - values->values;
                _PyDictValues_AddToInsertionOrder(values, index);
#ifdef Py_SHAPES
                _PyDictValues_SetShape(values,
                    _PyShape_FindTransition(Py_TYPE(owner_o), values->shape, index));
#endif
            }
            UNLOCK_OBJECT(owner_o);
            PyValue_XCLOSE(owner);
            PyValue_XCLOSE(old_value);
        }

        op(_LOCK_OBJECT, (value: object -- value))
            effects() exits(next, miss, deopt)
        {
            DEOPT_IF(!LOCK_OBJECT(PyValue_AsKnownObjectBorrow(value)));
        }

        macro(STORE_ATTR_INSTANCE_VALUE) =
            unused/1 +
            _LOCK_OBJECT +
            _GUARD_TYPE_VERSION_LOCKED +
            _GUARD_DORV_NO_DICT +
            _STORE_ATTR_INSTANCE_VALUE;

        leaf inline_ok op(_STORE_ATTR_SHAPE,
                (offset/1, value: consumed, owner: consumed object --))
            effects(reads, writes, releases) exits(next)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValue *value_ptr = _PyObject_ValueSlot(owner_o, offset);
            PyValue item = TAKE(value);
            // The shape guard shows the slot filled
            PyValue old_value = _PyValueSlot_ExchangeFilled(value_ptr, item);
            STAT_INC(STORE_ATTR, hit);
            PyValue_XCLOSE(owner);
            PyValue_CLOSE(old_value);
        }

        macro(STORE_ATTR_SHAPE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_SHAPE +
            _STORE_ATTR_SHAPE;

        leaf inline_ok op(_STORE_ATTR_SHAPE_ADD,
                (shape_to/0, offset/1, value: consumed, owner: consumed object --))
            effects(reads, writes, releases) exits(next)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValue *value_ptr = _PyObject_ValueSlot(owner_o, offset);
            PyValue item = TAKE(value);
            // The shape guard shows the attribute absent: the slot holds NULL
            _PyValueSlot_Fill(value_ptr, item);
            PyDictValues *values = _PyObject_InlineValues(owner_o);
            _PyDictValues_AddToInsertionOrder(values, value_ptr - values->values);
            _PyDictValues_SetShape(values, (uint8_t)shape_to);
            STAT_INC(STORE_ATTR, hit);
            PyValue_XCLOSE(owner);
        }

        macro(STORE_ATTR_SHAPE_ADD) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_SHAPE +
            _STORE_ATTR_SHAPE_ADD;

        inline_ok op(_STORE_ATTR_WITH_HINT,
                (hint/1, value: consumed, owner: consumed object --))
            effects(reads, writes, releases, runs_python) exits(next, deopt, raise)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            DEOPT_IF(dict == NULL, "the owner has no dict");
            DEOPT_IF(!LOCK_OBJECT(dict), "the dict is locked by another thread");
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyDictKeysObject *dk = dict->ma_keys;
            // Split keys are shared with other dicts, which append to them
            // under the keys lock rather than this dict's lock.
            if (hint >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_nentries) ||
                    dk->dk_kind == DICT_KEYS_GENERAL) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true, "the hint is past the entries, or the keys are general");
            }
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dk) + hint;
            if (FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key) != name) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true, "the hinted entry names another key");
            }
            // A split table keeps its values, such as an object's inline
            // values, apart from its keys.
            PyValue *value_ptr = dict->ma_values == NULL ?
                &ep->me_value : &dict->ma_values->values[hint];
            if (PyValue_IsNull(*value_ptr)) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true, "the hinted entry holds no value");
            }
            if (dict->_ma_watcher_tag & DICT_WATCHER_MASK) {
                /* A watcher error's unraisable hook may reenter. Do not
                 * retain a raw slot pointer across the provider call. */
                UNLOCK_OBJECT(dict);
                int err = _PyDict_SetValue(dict,
                    PyValue_FromObjectBorrow(name), value);
                DECREF_INPUTS();
                ERROR_IF(err < 0);
            }
            else {
                PyValue item = TAKE(value);
                PyValue old_value = _PyValueSlot_ExchangeFilled(value_ptr, item);
                UNLOCK_OBJECT(dict);
                PyValue_XCLOSE(owner);
                PyValue_XCLOSE(old_value);
            }
            STAT_INC(STORE_ATTR, hit);
        }

        macro(STORE_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _STORE_ATTR_WITH_HINT;

        leaf op(_STORE_ATTR_SLOT, (index/1, value: consumed, owner: consumed object --))
            effects(reads, writes, releases) exits(next, deopt)
        {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            DEOPT_IF(!LOCK_OBJECT(owner_o));
            PyValue *addr = _PyObject_ValueSlot(owner_o, index);
            STAT_INC(STORE_ATTR, hit);
            PyValue item = TAKE(value);
            PyValue old_value = _PyValueSlot_Exchange(addr, item);
            UNLOCK_OBJECT(owner_o);
            PyValue_XCLOSE(owner);
            PyValue_XCLOSE(old_value);
        }

        macro(STORE_ATTR_SLOT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _STORE_ATTR_SLOT;

        family(COMPARE_OP, INLINE_CACHE_ENTRIES_COMPARE_OP) = {
            COMPARE_OP_FLOAT,
            COMPARE_OP_INT,
            COMPARE_OP_STR,
        };

        specializing op(_SPECIALIZE_COMPARE_OP, (left, right -- left, right))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_CompareOp(left, right, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(COMPARE_OP);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_COMPARE_OP, (left: consumed, right: consumed -- res: owned))
            effects(runs_python) exits(next, raise)
        {
            assert((oparg >> 5) <= Py_GE);
            PyValue cmp = _PyValue_RichCompare(left, right, oparg >> 5);
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(cmp));
            if (oparg & 16) {
                int res_bool = _PyValue_IsTrue(cmp);
                PyValue_CLOSE(cmp);
                ERROR_IF(res_bool < 0);
                res = res_bool ? PyValue_True : PyValue_False;
            }
            else {
                res = cmp;
            }
        }

        macro(COMPARE_OP) = unused/1 + _SPECIALIZE_COMPARE_OP + _COMPARE_OP;

        macro(COMPARE_OP_FLOAT) =
            _GUARD_TOS_FLOAT + _GUARD_NOS_FLOAT + unused/1 + _COMPARE_OP_FLOAT + _POP_TOP_FLOAT + _POP_TOP_FLOAT;

        macro(COMPARE_OP_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/1 + _COMPARE_OP_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(COMPARE_OP_STR) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/1 + _COMPARE_OP_STR + _POP_TOP_UNICODE + _POP_TOP_UNICODE;

        leaf inline_ok op(_COMPARE_OP_FLOAT,
                (left: consumed exact(PyFloat_Type) inline inline_float, right: consumed exact(PyFloat_Type) inline inline_float
                 -- res: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects() exits(next)
        {
            STAT_INC(COMPARE_OP, hit);
            double dleft = PyValue_InlineFloat(left);
            double dright = PyValue_InlineFloat(right);
            // 1 if NaN, 2 if <, 4 if >, 8 if ==; this matches low four bits of the oparg
            int sign_ish = COMPARISON_BIT(dleft, dright);
            l = left;
            r = right;
            DEAD(left);
            DEAD(right);
            res = (sign_ish & oparg) ? PyValue_True : PyValue_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        // Similar to COMPARE_OP_FLOAT
        leaf inline_ok op(_COMPARE_OP_INT,
                (left: consumed exact(PyLong_Type) inline inline_int, right: consumed exact(PyLong_Type) inline inline_int
                 -- res: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects() exits(next)
        {
            STAT_INC(COMPARE_OP, hit);
            // Scaled values compare as the values do.
            int64_t ileft = _PyValue_IntScaled(left);
            int64_t iright = _PyValue_IntScaled(right);
            // 2 if <, 4 if >, 8 if ==; this matches the low 4 bits of the oparg
            int sign_ish = COMPARISON_BIT(ileft, iright);
            l = left;
            r = right;
            DEAD(left);
            DEAD(right);
            res =  (sign_ish & oparg) ? PyValue_True : PyValue_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        // Similar to COMPARE_OP_FLOAT, but for ==, != only
        op(_COMPARE_OP_STR,
                (left: consumed object, right: consumed object
                 -- res: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects(reads) exits(next)
        {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);

            STAT_INC(COMPARE_OP, hit);
            int eq = _PyUnicode_Equal(left_o, right_o);
            assert((oparg >> 5) == Py_EQ || (oparg >> 5) == Py_NE);
            l = left;
            r = right;
            DEAD(left);
            DEAD(right);
            assert(eq == 0 || eq == 1);
            assert((oparg & 0xf) == COMPARISON_NOT_EQUALS || (oparg & 0xf) == COMPARISON_EQUALS);
            assert(COMPARISON_NOT_EQUALS + 1 == COMPARISON_EQUALS);
            res = ((COMPARISON_NOT_EQUALS + eq) & oparg) ? PyValue_True : PyValue_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        macro(IS_OP) = _IS_OP + _POP_TOP_ANY + _POP_TOP_ANY;

        // A bit compare: the operands are canonical (see "Value identity"
        // in pycore_value.h).
        leaf inline_ok op(_IS_OP,
                (left: consumed, right: consumed
                 -- b: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects() exits(next)
        {
            int res = PyValue_Is(left, right) ^ oparg;
            b = res ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        family(CONTAINS_OP, INLINE_CACHE_ENTRIES_CONTAINS_OP) = {
            CONTAINS_OP_SET,
            CONTAINS_OP_DICT,
        };

        op(_CONTAINS_OP,
                (left: taken, right: taken
                 -- b: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects(runs_python) exits(next, raise)
        {
            int res = _PyValue_Contains(right, left);
            if (res < 0) {
                ERROR_NO_POP();
            }
            b = (res ^ oparg) ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        specializing op(_SPECIALIZE_CONTAINS_OP, (left, right -- left, right))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_ContainsOp(left, right, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CONTAINS_OP);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CONTAINS_OP) = unused/1 + _SPECIALIZE_CONTAINS_OP + _CONTAINS_OP + _POP_TOP_ANY + _POP_TOP_ANY;

        op(_GUARD_TOS_ANY_SET, (tos: object -- tos: exact(PyAnySet_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyAnySet_CheckExact(o));
        }

        macro(CONTAINS_OP_SET) = _GUARD_TOS_ANY_SET + unused/1 + _CONTAINS_OP_SET + _POP_TOP_ANY + _POP_TOP_ANY;

        // inline_ok: left is tested as a value, so nothing is boxed.
        inline_ok op(_CONTAINS_OP_SET,
                (left: taken, right: taken object
                 -- b: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects(runs_python) exits(next, raise)
        {
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);

            assert(PyAnySet_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            // Note: both set and frozenset use the same seq_contains method!
            int res = _PySet_Contains((PySetObject *)right_o, left);
            if (res < 0) {
                ERROR_NO_POP();
            }
            b = (res ^ oparg) ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(CONTAINS_OP_DICT) = _GUARD_TOS_ANY_DICT + unused/1 + _CONTAINS_OP_DICT + _POP_TOP_ANY + _POP_TOP_ANY;

        op(_CONTAINS_OP_DICT,
                (left: taken, right: taken object
                 -- b: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects(runs_python) exits(next, raise)
        {
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);

            assert(PyAnyDict_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            // The key is looked up as a value: an inline key needs no box.
            int res = _PyDict_ContainsValue((PyDictObject *)right_o, left);
            if (res < 0) {
                ERROR_NO_POP();
            }
            b = (res ^ oparg) ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        /* A compare and the conditional jump that tests it, in one word of
         * the register IR (Python/assemble.c fuses the pair where both are on
         * one line and the target is within IR_MAX_FORWARD).  The word is
         * {lhs, rhs, fb, D}: D is the signed distance from the next word, and
         * the arg of the entry holds the operation in bits 0 to 7 (the oparg
         * of the compare) and IR_FUSED_IF_FALSE.  There is no backward form, so
         * the handlers never poll the eval breaker.  The handlers stand for the
         * compare (their shadow, see ir_metadata_generator.py), and the
         * specialisations are those of the compare, in families of their own. */
        // Contract (G0d1): cond is the compare's result, which _COMPARE_OP does not
        // declare an exact bool, so the skeleton closes it (closed); the sense and
        // the distance are fixed bits of the entry and the word; the branch jumps.
        leaf ir_only op(_JUMP_IF_COND, (cond: closed -- ))
            effects() exits(next, branch)
        {
            assert(PyValue_BoolCheck(cond));
            int flag = PyValue_IsTrue(cond) ^ IR_FUSED_IF_FALSE();
            DEAD(cond);
            IR_REPORT_EDGE(flag);
            if (flag) {
                JUMPBY(IR_FUSED_DISTANCE());
            }
        }

        // Contract (seed, reviewed G0d1): cond as _JUMP_IF_COND's; the branch event
        // (IR_OBSERVE_COMMITTED_BRANCH) runs tools and may raise.
        ir_only op(_INSTRUMENTED_JUMP_IF_COND, (cond: closed -- ))
            effects(runs_python) exits(next, branch, raise)
        {
            assert(PyValue_BoolCheck(cond));
            int flag = PyValue_IsTrue(cond) ^ IR_FUSED_IF_FALSE();
            DEAD(cond);
            if (flag) {
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr + IR_FUSED_DISTANCE(), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        family(COMPARE_OP_JUMP, INLINE_CACHE_ENTRIES_COMPARE_OP) = {
            COMPARE_OP_JUMP_FLOAT,
            COMPARE_OP_JUMP_INT,
            COMPARE_OP_JUMP_STR,
        };

        macro(COMPARE_OP_JUMP) =
            unused/1 + _SPECIALIZE_COMPARE_OP + _COMPARE_OP + _JUMP_IF_COND;

        macro(COMPARE_OP_JUMP_FLOAT) =
            _GUARD_TOS_FLOAT + _GUARD_NOS_FLOAT + unused/1 + _COMPARE_OP_FLOAT + _POP_TOP_FLOAT + _POP_TOP_FLOAT + _JUMP_IF_COND;

        macro(COMPARE_OP_JUMP_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/1 + _COMPARE_OP_INT + _POP_TOP_INT + _POP_TOP_INT + _JUMP_IF_COND;

        macro(COMPARE_OP_JUMP_STR) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/1 + _COMPARE_OP_STR + _POP_TOP_UNICODE + _POP_TOP_UNICODE + _JUMP_IF_COND;

        macro(INSTRUMENTED_COMPARE_OP_JUMP) =
            unused/1 + _COMPARE_OP + _INSTRUMENTED_JUMP_IF_COND;

        /* An exact str, tuple or list container: the type's contains
         * function is called by value from the handler, with no slot lookup
         * and no dispatcher. The guard is the exact type, so a subclass that
         * overrides __contains__ takes the generic handler. Only the fused
         * words have this form. */
        op(_GUARD_TOS_STR_TUPLE_LIST,
                (tos: object -- tos: exact(PyList_Type | PyTuple_Type | PyUnicode_Type)))
            effects(reads) exits(next, miss)
        {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyUnicode_CheckExact(o) && !PyTuple_CheckExact(o)
                    && !PyList_CheckExact(o), "not an exact str, tuple or list");
        }

        op(_CONTAINS_OP_SEQUENCE,
                (left: taken, right: taken object
                 -- b: owned exact(PyBool_Type), l: moved(left), r: moved(right)))
            effects(runs_python) exits(next, raise)
        {
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
            STAT_INC(CONTAINS_OP, hit);
            _Py_containsvaluefunc contains_slot = _PyType_VALUE_SLOT(Py_TYPE(right_o), contains);
            int res = contains_slot(right_o, PyValue_Borrow(left));
            if (res < 0) {
                ERROR_NO_POP();
            }
            b = (res ^ oparg) ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        family(CONTAINS_OP_JUMP, INLINE_CACHE_ENTRIES_CONTAINS_OP) = {
            CONTAINS_OP_JUMP_DICT,
            CONTAINS_OP_JUMP_SET,
            CONTAINS_OP_JUMP_SEQUENCE,
        };

        macro(CONTAINS_OP_JUMP) =
            unused/1 + _SPECIALIZE_CONTAINS_OP + _CONTAINS_OP + _POP_TOP_ANY + _POP_TOP_ANY + _JUMP_IF_COND;

        macro(CONTAINS_OP_JUMP_SET) =
            _GUARD_TOS_ANY_SET + unused/1 + _CONTAINS_OP_SET + _POP_TOP_ANY + _POP_TOP_ANY + _JUMP_IF_COND;

        macro(CONTAINS_OP_JUMP_DICT) =
            _GUARD_TOS_ANY_DICT + unused/1 + _CONTAINS_OP_DICT + _POP_TOP_ANY + _POP_TOP_ANY + _JUMP_IF_COND;

        macro(CONTAINS_OP_JUMP_SEQUENCE) =
            _GUARD_TOS_STR_TUPLE_LIST + unused/1 + _CONTAINS_OP_SEQUENCE + _POP_TOP_ANY + _POP_TOP_ANY + _JUMP_IF_COND;

        macro(INSTRUMENTED_CONTAINS_OP_JUMP) =
            unused/1 + _CONTAINS_OP + _POP_TOP_ANY + _POP_TOP_ANY + _INSTRUMENTED_JUMP_IF_COND;

        macro(IS_OP_JUMP) = _IS_OP + _POP_TOP_ANY + _POP_TOP_ANY + _JUMP_IF_COND;

        macro(INSTRUMENTED_IS_OP_JUMP) =
            _IS_OP + _POP_TOP_ANY + _POP_TOP_ANY + _INSTRUMENTED_JUMP_IF_COND;

        outlined inst(CHECK_EG_MATCH,
                (exc_value_st: consumed assumed(object) by exception_unwind, match_type_st: consumed
                 -- exc_rest: owned, exc_match: owned))
            effects(runs_python, releases) exits(next, raise)
        {
            // The exception being handled is an object; the generator
            // asserts the assumption
            PyObject *exc_value = PyValue_AsKnownObjectBorrow(exc_value_st);
            // Taken before the check, which can run code
            PyValue held_exc = TAKE(exc_value_st);
            PyValue held_type = TAKE(match_type_st);
            // A number is not an exception class: the check rejects it
            int err = _PyEval_CheckExceptStarTypeValidValue(tstate, held_type);
            if (err < 0) {
                PyValue_CLOSE(held_type);
                PyValue_CLOSE(held_exc);
                ERROR_IF(true);
            }
            PyObject *match_type = PyValue_AsKnownObjectBorrow(held_type);

            _PyObjectPair matched = value_exception_group_match(frame, exc_value, match_type);
            PyValue_CLOSE(held_type);
            PyValue_CLOSE(held_exc);
            // A null match is an error, including a failed match (res < 0)
            ERROR_IF(matched.match == NULL);

            if (!Py_IsNone(matched.match)) {
                PyErr_SetHandledException(matched.match);
            }
            exc_rest = PyValue_FromObjectSteal(matched.rest);
            exc_match = PyValue_FromObjectSteal(matched.match);
        }

        outlined inst(CHECK_EXC_MATCH,
                (left: assumed(object) by exception_unwind, right: consumed
                 -- left, b: owned exact(PyBool_Type)))
            effects(runs_python, releases) exits(next, raise)
        {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            // Taken before the check, which can run code
            PyValue held_right = TAKE(right);

            assert(PyExceptionInstance_Check(left_o));
            // A number is not an exception class: the check rejects it
            int err = _PyEval_CheckExceptTypeValidValue(tstate, held_right);
            if (err < 0) {
                PyValue_CLOSE(held_right);
                ERROR_NO_POP();
            }
            PyObject *right_o = PyValue_AsKnownObjectBorrow(held_right);

            int res = PyErr_GivenExceptionMatches(left_o, right_o);
            PyValue_CLOSE(held_right);
            b = res ? PyValue_True : PyValue_False;
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

        family(GET_ITER, INLINE_CACHE_ENTRIES_GET_ITER) = {
            GET_ITER_SELF,
            GET_ITER_VIRTUAL,
            GET_ITER_PY_GENERATOR,
        };

        specializing op(_SPECIALIZE_GET_ITER, (iterable -- iterable))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_GetIter(iterable, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(GET_ITER);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
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

        macro(GET_ITER) =
            unused/1 + _SPECIALIZE_GET_ITER +
            _GET_ITER;

        op(_GUARD_ITERATOR, (iterable: object -- iterable))
            effects(reads) exits(next, miss)
        {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(iterable));
            EXIT_IF(tp->tp_iter != PyObject_SelfIter, "the type's tp_iter is not PyObject_SelfIter");
            STAT_INC(GET_ITER, hit);
        }

        macro(GET_ITER_SELF) =
            unused/1 +
            _GUARD_ITERATOR +
            PUSH_NULL;

        op(_GUARD_ITER_VIRTUAL, (iterable: object -- iterable))
            effects(reads) exits(next, miss)
        {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(iterable));
            EXIT_IF(!_PyType_IteratesByIndex(tp), "the type iterates by index");
            STAT_INC(GET_ITER, hit);
        }

        op(_PUSH_TAGGED_ZERO, ( -- zero: owned))
            effects() exits(next)
        {
            zero = PyValue_TagIndex(0);
        }

        macro(GET_ITER_VIRTUAL) =
            unused/1 +
            _GUARD_ITER_VIRTUAL +
            _PUSH_TAGGED_ZERO;

        // The iterable's class defines __iter__ as a generator function of
        // the one argument: the type caches the function under its version
        // (as for __getitem__), which PyType_Modified() invalidates.  The
        // iterator is the generator, created at once with the iterable as its
        // argument and no call of the function.
        inline_ok op(_GUARD_ITER_PY_GENERATOR, (iterable -- iterable: object))
            effects(reads) exits(next, miss)
        {
            EXIT_IF(!PyValue_IsObject(iterable));
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(iterable));
            EXIT_IF(!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE),
                    "only a heap type caches its __iter__ function");
            PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
            PyObject *iter_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(ht->_spec_cache.iter);
            EXIT_IF(iter_o == NULL, "the type caches no __iter__ function");
            assert(PyFunction_Check(iter_o));
            EXIT_IF(((PyFunctionObject *)iter_o)->func_version !=
                    FT_ATOMIC_LOAD_UINT32_RELAXED(ht->_spec_cache.iter_version),
                    "the cached function changed version");
        }

        // Contract (G0d1): _PyGen_NewCall allocates the generator; if it fails the
        // iterable is still in its home (holding).  Then the iterable moves into the
        // generator's frame (TAKE), and a failed prologue discards the generator
        // with it (_PyGen_DiscardCall); the generator becomes iter.
        ir_only op(_GET_ITER_PY_GENERATOR,
                (iterable: consumed object
                 -- iter: owned object, index_or_null: inline encoding(null)))
            effects(runs_python) exits(next, raise)
        {
            STAT_INC(GET_ITER, hit);
            assert(PyValue_IsObject(iterable));
            PyHeapTypeObject *ht =
                (PyHeapTypeObject *)Py_TYPE(PyValue_AsKnownObjectBorrow(iterable));
            PyFunctionObject *func = (PyFunctionObject *)ht->_spec_cache.iter;
            PyGenObject *gen = _PyGen_NewCall(
                tstate, PyValue_DUP(PyValue_FromObjectBorrow((PyObject *)func)),
                NULL, (PyCodeObject *)func->func_code, frame);
            // The iterable is still in its home, for the unwinder
            ERROR_IF(gen == NULL, holding(iterable));
            gen->gi_iframe.base[0] = TAKE(iterable);
            if (gen_prologue(tstate, gen, (PyCodeObject *)func->func_code) < 0) {
                _PyGen_DiscardCall(gen);
                ERROR_NO_POP();
            }
            INPUTS_DEAD();
            iter = PyValue_FromObjectSteal((PyObject *)gen);
            index_or_null = PyValue_NULL;
        }

        macro(GET_ITER_PY_GENERATOR) =
            unused/1 +
            _CHECK_PEP_523 +
            _GUARD_ITER_PY_GENERATOR +
            _GET_ITER_PY_GENERATOR;

        // The body outcome preserves the iterator/cursor pair and defines
        // the item recipient. Exhaustion retires the pair, preserves the
        // last item binding and continues at the real exit target.

        family(FOR_ITER, INLINE_CACHE_ENTRIES_FOR_ITER) = {
            FOR_ITER_LIST,
            FOR_ITER_TUPLE,
            FOR_ITER_RANGE,
            FOR_ITER_GEN,
            FOR_ITER_VIRTUAL,
            FOR_ITER_DICT_ITEMS,
        };

        specializing op(_SPECIALIZE_FOR_ITER,
                (iter: assumed(object) by GET_ITER, null_or_index -- iter, null_or_index))
            effects(runs_python) exits(next, jump) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_ForIter(iter, null_or_index, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(FOR_ITER);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

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

        macro(FOR_ITER) = unused/1 + _SPECIALIZE_FOR_ITER + _FOR_ITER;

        // One form for both cursor protocols. A tagged cursor steps an
        // index-protocol iterator; a NULL cursor steps an object-protocol
        // iterator through its C value slot, and a type without the slot
        // exits.
        replaced op(_FOR_ITER_VIRTUAL,
                (iter: assumed(object) by GET_ITER, index: consumed
                 -- iter, next_index: inline encoding(tagged_index|null) replaces(index), next: owned))
            effects(runs_python) exits(next, branch, raise, miss)
        {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            if (PyValue_IsNull(index)) {
                _Py_iternextvaluefunc next_value = _PyType_VALUE_SLOT(Py_TYPE(iter_o), iternext);
                EXIT_IF(next_value == NULL, "the type has no iternext value slot");
                PyValue spent = TAKE(index);
                _PyValue_DropCursor(spent);
                _PyStatusValue step = next_value(iter_o);
                if (step.status != PYGEN_NEXT) {
                    // A StopIteration is the exhaustion; any other error
                    // propagates.
                    if (step.status == PYGEN_ERROR && !_PyForIter_StopIsExhaustion(tstate, frame)) {
                        ERROR_NO_POP();
                    }
                    PyValue completed = PyValue_IsNull(step.value) ? PyValue_None : step.value;
                    IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
                }
                next_index = PyValue_NULL;
                next = step.value;
            }
            else {
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
                    IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
                }
                next_index = PyValue_TagIndex(item.index);
                next = item.value;
            }
        }

        macro(FOR_ITER_VIRTUAL) =
            unused/1 +  // Skip over the entry distance
            _FOR_ITER_VIRTUAL;

        // `for key, value in d.items()`: FOR_ITER over a dict's items, whose
        // key and value go straight to the two locals that the unpack after
        // it names, with no pair and no unpack.  The specializer chooses it
        // only when the next word unpacks the item into a run of two locals;
        // the handler then continues after that word, which stays in place
        // for the generic form.  Instrumentation may replace that word, and
        // then the generic form runs it.
        // Contract (G0d1): the iterator is GET_ITER's object; the step
        // (_PyDictIterItem_Next) writes the two locals and may raise; exhaustion
        // completes the step (IR_COMPLETE_STEP: branch, runs Python); a wrong type
        // or an instrumented unpack word misses. A step continues after the
        // unpack word (`next_instr++`), not at the next word: jump (G0d1, V12).
        ir_only op(_FOR_ITER_DICT_ITEMS,
                (iter: assumed(object) by GET_ITER, null_or_index -- iter, null_or_index, unused))
            effects(runs_python) exits(jump, branch, raise, miss)
        {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(iter_o) != &PyDictIterItem_Type);
            _PyIRWord unpack = FT_ATOMIC_LOAD_UINT64_RELAXED(*next_instr);
            EXIT_IF(_PyIR_HandlerFlags[IR_HANDLER(unpack)] & _PyIR_H_INSTRUMENTED,
                    "the folded unpack word is instrumented");
            int found = _PyDictIterItem_Next(iter_o, &IR_SLOT(IR_D(unpack)));
            if (found <= 0) {
                if (found < 0) {
                    ERROR_NO_POP();
                }
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, PyValue_None, oparg);
            }
            next_instr++;
        }

        macro(FOR_ITER_DICT_ITEMS) =
            unused/1 +  // Skip over the entry distance
            _FOR_ITER_DICT_ITEMS;

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

        leaf op(_ITER_CHECK_LIST,
                (iter: assumed(object) by GET_ITER, null_or_index
                 -- iter: exact(PyList_Type), null_or_index))
            effects(reads) exits(next, miss)
        {
            // GET_ITER always leaves an object, never an inline value.
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(!PyList_CheckExact(iter_o));
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_Py_IsOwnedByCurrentThread(iter_o) && !_PyObject_GC_IS_SHARED(iter_o),
                    "a list another thread owns, and that is not shared");
#endif
        }

        // GET_ITER stores a tagged index beside an exact list (its
        // `_PyEval_GetIter` gives `PyValue_TagIndex(0)` for it), and the
        // guard before this uop has shown that the iterator is one: only
        // then is the cursor assumed, so the assertion follows the guard
        leaf replaced op(_ITER_JUMP_LIST,
                (iter: object, index: consumed assumed(tagged_index) by GET_ITER
                 -- iter, next_index: inline tagged_index replaces(index)))
            effects(reads, runs_python(branch|raise)) exits(next, branch, raise)
        {
#ifdef Py_GIL_DISABLED
            // For free-threaded Python, the loop exit can happen at any point during
            // item retrieval, so it doesn't make much sense to check and jump
            // separately before item retrieval. Any length check we do here can be
            // invalid by the time we actually try to fetch the item.
#else
            PyObject *list_o = PyValue_AsKnownObjectBorrow(iter);
            assert(Py_TYPE(list_o) == &PyList_Type);
            STAT_INC(FOR_ITER, hit);
            if ((size_t)PyValue_UntagIndex(index) >= (size_t)PyList_GET_SIZE(list_o)) {
                next_index = PyValue_TagIndex(-1);
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
#endif
            next_index = index;
            DEAD(index);
        }

        leaf replaced op(_ITER_NEXT_LIST,
                (iter: object, index: consumed -- iter, next_index: inline replaces(index), next: owned))
            effects(reads) exits(next)
        {
            PyObject *list_o = PyValue_AsKnownObjectBorrow(iter);
            assert(PyList_CheckExact(list_o));
            PyValueBorrow item = _PyValue_PublishedItem(
                PyList_GET_VALUE(list_o, PyValue_UntagIndex(index)));
            next = PyValue_DUP(item);
            next_index = PyValue_IncrementTaggedIndexNoOverflow(index);
            DEAD(index);
        }

        macro(FOR_ITER_LIST) =
            unused/1 +  // Skip over the entry distance
            _ITER_CHECK_LIST +
            _ITER_JUMP_LIST +
            _ITER_NEXT_LIST;

        leaf op(_ITER_CHECK_TUPLE,
                (iter: assumed(object) by GET_ITER, null_or_index
                 -- iter: exact(PyTuple_Type), null_or_index))
            effects(reads) exits(next, miss)
        {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(!PyTuple_CheckExact(iter_o));
        }

        // GET_ITER stores a tagged index beside an exact tuple (its
        // `_PyEval_GetIter` gives `PyValue_TagIndex(0)` for it), and the
        // guard before this uop has shown that the iterator is one: only
        // then is the cursor assumed, so the assertion follows the guard
        leaf replaced op(_ITER_JUMP_TUPLE,
                (iter: object, index: consumed assumed(tagged_index) by GET_ITER
                 -- iter, next_index: inline tagged_index replaces(index)))
            effects(reads, runs_python(branch|raise)) exits(next, branch, raise)
        {
            PyObject *tuple_o = PyValue_AsKnownObjectBorrow(iter);
            (void)tuple_o;
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            STAT_INC(FOR_ITER, hit);
            if ((size_t)PyValue_UntagIndex(index) >= (size_t)PyTuple_GET_SIZE(tuple_o)) {
                next_index = PyValue_TagIndex(-1);
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
            next_index = index;
            DEAD(index);
        }

        leaf op(_ITER_NEXT_TUPLE,
                (iter: object, index: consumed -- iter, next_index: inline replaces(index), next: owned))
            effects(reads) exits(next)
        {
            PyObject *tuple_o = PyValue_AsKnownObjectBorrow(iter);
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            uintptr_t i = PyValue_UntagIndex(index);
            assert((size_t)i < (size_t)PyTuple_GET_SIZE(tuple_o));
            PyValueBorrow item = _PyValue_PublishedItem(PyTuple_GET_VALUE(tuple_o, i));
            next = PyValue_DUP(item);
            next_index = PyValue_IncrementTaggedIndexNoOverflow(index);
            DEAD(index);
        }

        macro(FOR_ITER_TUPLE) =
            unused/1 +  // Skip over the entry distance
            _ITER_CHECK_TUPLE +
            _ITER_JUMP_TUPLE +
            _ITER_NEXT_TUPLE;

        leaf op(_ITER_CHECK_RANGE,
                (iter: assumed(object) by GET_ITER, null_or_index
                 -- iter: exact(PyRangeIter_Type), null_or_index))
            effects(reads) exits(next, miss)
        {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(r) != &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_PyObject_IsUniquelyReferenced((PyObject *)r),
                    "another reference to the range iterator exists");
#endif
        }

        leaf replaced op(_ITER_JUMP_RANGE,
                (iter: object, null_or_index -- iter, null_or_index))
            effects(reads, runs_python(branch|raise)) exits(next, branch, raise)
        {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            assert(_PyObject_IsUniquelyReferenced((PyObject *)r));
#endif
            STAT_INC(FOR_ITER, hit);
            if (r->len <= 0) {
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
        }

        leaf op(_ITER_NEXT_RANGE,
                (iter: object, null_or_index
                 -- iter, null_or_index, next: owned exact(PyLong_Type)))
            effects(reads, writes, allocates, raises) exits(next, raise)
        {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            assert(_PyObject_IsUniquelyReferenced((PyObject *)r));
#endif
            assert(r->len > 0);
            long value = r->start;
            r->start = value + r->step;
            r->len--;
            // A range may cross the inline range.
            next = PyValue_FromInt64(value);
            ERROR_IF(PyValue_IsNull(next));
        }

        macro(FOR_ITER_RANGE) =
            unused/1 +  // Skip over the entry distance
            _ITER_CHECK_RANGE +
            _ITER_JUMP_RANGE +
            _ITER_NEXT_RANGE;

        op(_FOR_ITER_GEN_FRAME,
                (iter: assumed(object) by GET_ITER, null -- iter, null))
            effects(reads, writes, raises, runs_python, switches_frame)
            exits(miss, raise, enter)
        {
            PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(gen) != &PyGen_Type, "the iterator is not a generator");
            EXIT_IF(!gen_may_claim(gen), "the generator runs or has finished");
            STAT_INC(FOR_ITER, hit);
            if (!gen_claim(tstate, gen)) {
                ERROR_NO_POP();
            }
            IR_ENTER_GENERATOR(gen, _PY_IR_FOR_ITER);
        }

        macro(FOR_ITER_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _FOR_ITER_GEN_FRAME;

        // A window: the method and self go to the canonical slots, which a
        // folded input, a local, would not name
        window op(_INSERT_NULL, (self: consumed -- method_and_self[2]: owned))
            effects() exits(next)
        {
            IR_WRITE_OUTPUT(method_and_self, 1, self);
            IR_WRITE_OUTPUT(method_and_self, 0, PyValue_NULL);
            DEAD(self);
        }

        op(_LOAD_SPECIAL, (method_and_self[2] -- method_and_self[2]))
            effects(runs_python) exits(next, raise)
        {
            PyObject *name = _Py_SpecialMethods[oparg].name;
            int err = IR_LOOKUP_SPECIAL(name, method_and_self);
            if (err <= 0) {
                if (err == 0) {
                    // The messages name the owner's type, as %N
                    PyTypeObject *owner_type = PyValue_TYPE(method_and_self[1]);
                    const char *errfmt = _PyEval_SpecialMethodCanSuggest(owner_type, oparg)
                        ? _Py_SpecialMethods[oparg].error_suggestion
                        : _Py_SpecialMethods[oparg].error;
                    assert(!_PyErr_Occurred(tstate));
                    assert(errfmt != NULL);
                    _PyErr_Format(tstate, PyExc_TypeError, errfmt, owner_type);
                }
                ERROR_NO_POP();
            }
        }

        outlined macro(LOAD_SPECIAL) =
            _INSERT_NULL +
            _LOAD_SPECIAL;

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

        outlined window inst(PUSH_EXC_INFO,
                (exc: consumed -- prev_exc: owned, new_exc: moved(exc)))
            effects(reads, writes) exits(next)
        {

            _PyErr_StackItem *exc_info = tstate->exc_info;
            PyObject *prev_o = _PyErr_StackItemTakeValue(exc_info);
            if (prev_o != NULL) {
                prev_exc = PyValue_FromObjectSteal(prev_o);
            }
            else {
                prev_exc = PyValue_None;
            }
            assert(PyValue_ExceptionInstanceCheck(exc));
            exc_info->exc_value = PyValue_AsPyObjectNew(exc);
            new_exc = exc;
            DEAD(exc);
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
            IR_SKIP_RESUME();
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

        // Feedback: func_version
        family(CALL_KW, INLINE_CACHE_ENTRIES_CALL_KW) = {
            CALL_KW_BOUND_METHOD,
            CALL_KW_PY,
            CALL_KW_NON_PY,
        };

        inline_ok op(_MONITOR_CALL_KW,
                (callable, self_or_null, args[oparg], unused
                 -- callable, self_or_null, args[oparg], unused))
            effects(runs_python) exits(next, raise)
        {
            IR_MONITOR_CALL();
        }

        inline_ok op(_MAYBE_EXPAND_METHOD_KW,
                (callable, self_or_null, unused[oparg], unused
                 -- callable, self_or_null, unused[oparg], unused))
            effects(runs_python) exits(next, raise)
        {
            IR_PREPARE_CALL_METHOD();
        }

        inline_ok op(_DO_CALL_KW,
                (callable: taken, self_or_null: taken, args[oparg]: taken, kwnames: borrowed
                 -- res: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_CALL();
        }

        inline_ok op(_PY_FRAME_KW,
                (callable: taken, self_or_null: taken, args[oparg]: taken, kwnames: borrowed
                 -- new_frame: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_CALL();
        }

        inline_ok op(_PY_FRAME_KW_GUARDED,
                (callable: taken, self_or_null: taken, args[oparg]: taken, kwnames: borrowed
                 -- new_frame: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_GUARDED_PYTHON_CALL();
        }

        op(_CHECK_FUNCTION_VERSION_KW,
                (func_version/2, callable: object, unused, unused[oparg], unused
                 -- callable: exact(PyFunction_Type) func_version(func_version), unused, unused[oparg], unused))
            effects(reads) exits(next, miss)
        {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(!PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            EXIT_IF(func->func_version != func_version);
        }

        macro(CALL_KW_PY) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION_KW +
            _CHECK_RECURSION_REMAINING +
            _PY_FRAME_KW_GUARDED +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_CHECK_METHOD_VERSION_KW,
                (func_version/2, callable: object, null, unused[oparg], unused
                 -- callable: exact(PyMethod_Type), null, unused[oparg], unused))
            effects(reads) exits(next, miss) state(call_state -- call_state: self(absent))
            paths(callable.im_func: exact(PyFunction_Type) func_version(func_version))
        {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(call_state & CALL_SELF_PRESENT);
        }

        inline_ok op(_EXPAND_METHOD_KW,
                (callable, self_or_null, unused[oparg], unused
                 -- callable, self_or_null, unused[oparg], unused))
            effects(runs_python) exits(next, raise)
        {
            IR_PREPARE_CALL_METHOD();
        }

        macro(CALL_KW_BOUND_METHOD) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_METHOD_VERSION_KW +
            _EXPAND_METHOD_KW +
            _PY_FRAME_KW +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        specializing op(_SPECIALIZE_CALL_KW,
                (callable, self_or_null, unused[oparg], kwnames
                 -- callable, self_or_null, unused[oparg], kwnames))
            effects(runs_python) exits(next, raise) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                /* As in _SPECIALIZE_CALL */
                IR_BEGIN_CALL_EFFECTS();
                _Py_Specialize_CallKw(callable, self_or_null, kwnames, this_instr, oparg);
                IR_REFRESH_CALL();
            }
            else {
                OPCODE_DEFERRED_INC(CALL_KW);
                _PyFb_AdvanceCounter(feedback);
            }
            #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CALL_KW) =
            unused/1 + _SPECIALIZE_CALL_KW +
            unused/2 +
            _MAYBE_EXPAND_METHOD_KW +
            _DO_CALL_KW;

        macro(INSTRUMENTED_CALL_KW) =
            unused/1 +
            unused/2 +
            _MAYBE_EXPAND_METHOD_KW +
            _MONITOR_CALL_KW +
            _DO_CALL_KW;

        inline_ok op(_CHECK_IS_NOT_PY_CALLABLE_KW,
                (callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused))
            effects(reads) exits(next, miss)
        {
            if (PyValue_IsObject(callable)) {
                PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
                EXIT_IF(PyFunction_Check(callable_o),
                        "the callable is a Python function");
                EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type,
                        "the callable is a bound method");
            }
        }


        inline_ok op(_CALL_KW_NON_PY,
                (callable: taken, self_or_null: taken, args[oparg]: taken, kwnames: borrowed
                 -- res: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_CALL();
        }

        macro(CALL_KW_NON_PY) =
            unused/1 + // Skip over the entry distance
            unused/2 +
            _CHECK_IS_NOT_PY_CALLABLE_KW +
            _CALL_KW_NON_PY;

        family(CALL_FUNCTION_EX, INLINE_CACHE_ENTRIES_CALL_FUNCTION_EX) = {
            CALL_EX_PY,
            CALL_EX_NON_PY_GENERAL,
        };

        inline_ok op(_MAKE_CALLARGS_A_TUPLE,
                (func, unused, callargs, kwargs -- func, unused, callargs, kwargs))
            effects(runs_python) exits(next, raise)
        {
            IR_PREPARE_CALL_TUPLE();
        }

        inline_ok op(_DO_CALL_FUNCTION_EX,
                (func_st: taken, null: taken, callargs_st: taken, kwargs_st: taken
                 -- result: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_CALL();
        }

        specializing op(_SPECIALIZE_CALL_FUNCTION_EX,
                (func, unused, callargs, unused -- func, unused, callargs, unused))
            effects(runs_python) exits(next, raise) feedback(read(counter), read(entry), write(counter), write(entry), write(handler))
        {
        #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                /* As in _SPECIALIZE_CALL */
                IR_BEGIN_CALL_EFFECTS();
                _Py_Specialize_CallFunctionEx(func, callargs, this_instr);
                IR_REFRESH_CALL();
            }
            else {
                OPCODE_DEFERRED_INC(CALL_FUNCTION_EX);
                _PyFb_AdvanceCounter(feedback);
            }
        #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CALL_FUNCTION_EX) =
            unused/1 + _SPECIALIZE_CALL_FUNCTION_EX +
            _MAKE_CALLARGS_A_TUPLE +
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC_AT_END;

        op(_CHECK_IS_PY_CALLABLE_EX,
                (func_st: object, unused, unused, unused
                 -- func_st: exact(PyFunction_Type), unused, unused, unused))
            effects(reads) exits(next, miss)
        {
            PyObject *func = PyValue_AsKnownObjectBorrow(func_st);
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->vectorcall != _PyFunction_Vectorcall,
                    "the function has its own vectorcall");
        }

        inline_ok op(_PY_FRAME_EX,
                (func_st: taken, null: taken, callargs_st: taken, kwargs_st: taken
                 -- ex_frame: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_CALL();
        }

        // call_run_ex() makes the argument tuple, which may run Python, and
        // then derives the kind of call from the callable again: the guard
        // before it is the only one
        macro(CALL_EX_PY) =
            unused/1 +
            _CHECK_PEP_523 +
            _CHECK_IS_PY_CALLABLE_EX +
            _MAKE_CALLARGS_A_TUPLE +
            _PY_FRAME_EX +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_CHECK_IS_NOT_PY_CALLABLE_EX,
                (func_st: object, unused, unused, unused
                 -- func_st, unused, unused, unused))
            effects(reads) exits(next, miss)
        {
            PyObject *func = PyValue_AsKnownObjectBorrow(func_st);
            EXIT_IF(Py_TYPE(func) == &PyFunction_Type && ((PyFunctionObject *)func)->vectorcall == _PyFunction_Vectorcall,
                    "not a Python function with the standard vectorcall");
        }

        inline_ok op(_CALL_FUNCTION_EX_NON_PY_GENERAL,
                (func_st: taken, null: taken, callargs_st: taken, kwargs_st: taken
                 -- result: owned))
            effects(runs_python, switches_frame) exits(next, raise, enter)
        {
            IR_INVOKE_CALL();
        }

        macro(CALL_EX_NON_PY_GENERAL) =
            unused/1 +
            _CHECK_IS_NOT_PY_CALLABLE_EX +
            _MAKE_CALLARGS_A_TUPLE +
            _CALL_FUNCTION_EX_NON_PY_GENERAL +
            _CHECK_PERIODIC_AT_END;

        macro(INSTRUMENTED_CALL_FUNCTION_EX) =
            unused/1 +
            _MAKE_CALLARGS_A_TUPLE +
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC_AT_END;

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

        inst(BUILD_SLICE, (args[oparg]: consumed -- slice: owned))
            effects(runs_python) exits(next, raise)
        {
            _PyValue_RecheckWindow(args, oparg);
            // The bounds are stored as the values they are: no box of an
            // inline one.
            PyObject *slice_o = _PySlice_FromValues(
                args[0], args[1], oparg == 3 ? args[2] : PyValue_NULL);
            DECREF_INPUTS();
            ERROR_IF(slice_o == NULL);
            slice = PyValue_FromObjectSteal(slice_o);
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

        specializing op(_SPECIALIZE_BINARY_OP, (lhs, rhs -- lhs, rhs))
            effects(runs_python) exits(next, jump)
            feedback(read(entry), write(entry), write(handler))
        {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (_PyFb_CounterTriggers(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, LOCALS_ARRAY);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(BINARY_OP);
            _PyFb_AdvanceCounter(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
            assert(NB_ADD <= oparg);
            assert(oparg <= NB_OPARG_LAST);
        }

        inline_ok op(_BINARY_OP, (lhs: consumed, rhs: consumed -- res: owned))
            effects(runs_python) exits(next, raise)
        {
            res = value_binary_op(oparg, lhs, rhs);
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(res));
        }

        macro(BINARY_OP) region(shared) = unused/1 + _SPECIALIZE_BINARY_OP + unused/4 + _BINARY_OP;

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
            IR_SKIP_RESUME();
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
