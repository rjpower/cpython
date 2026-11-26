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
        translator_only tier1 inst(MATERIALIZE_KWARGS, (--))
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

        /* A stack op only: STORE_FAST is the LocalWrite family (Python/register_ops/values.rop). */
        tier1 inst(STORE_FAST, (value: consumed --))
            exits(abort)
        {
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            GETLOCAL(oparg) = value;
            DEAD(value);
            PyValue_XCLOSE(tmp);
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        pseudo(STORE_FAST_MAYBE_NULL, (unused --)) = {
            STORE_FAST,
        };

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

        /* A stack op only: the END_SEND of a SEND has no word; the completion of the
         * Send family is its end (Python/register_ops/suspend.rop). */
        tier1 inst(END_SEND,
                (receiver: consumed, index_or_null: closed, value: consumed
                 -- val: moved(value)))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        tier1 inst(INSTRUMENTED_END_SEND,
                (receiver: consumed, index_or_null: closed, value: consumed
                 -- val: moved(value)))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
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

        /* A stack op only: GET_AITER is the AsyncIterOpen family (Python/register_ops/suspend.rop). */
        tier1 inst(GET_AITER, (obj: consumed -- iter: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: GET_ANEXT is the AsyncIterNext family (Python/register_ops/suspend.rop). */
        tier1 inst(GET_ANEXT, (aiter: assumed(object) by GET_AITER -- aiter, awaitable: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: GET_AWAITABLE is the Awaitable family (Python/register_ops/suspend.rop). */
        tier1 inst(GET_AWAITABLE, (iterable: consumed -- iter: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: SEND is the Send family (Python/register_ops/suspend.rop). */
        tier1 inst(SEND,
                (unused/1,
                 receiver: assumed(object) by compiler_send_loop,
                 index: consumed assumed(encoding(tagged_index|null)) by compiler_send_loop,
                 v: taken
                 -- receiver, next_index: inline encoding(tagged_index|null) replaces(index),
                 retval: owned))
            effects(releases) exits(next, jump)
        {
            JUMPBY(oparg);
            PyValue_CLOSE(v);
            next_index = index;
            DEAD(index);
            retval = PyValue_None;
        }

        /* Stack ops only: YIELD_VALUE and its instrumented form are the Yield
         * and YieldFrom families (Python/register_ops/suspend.rop). */
        tier1 inst(YIELD_VALUE, (retval: consumed -- value: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* The observed form of YIELD_VALUE is the Observed arm of the Yield and
         * YieldFrom families.  It is a macro, as it was, so that the instrumented
         * opcodes keep their numbers. */
        tier1 op(_YIELD_VALUE_OBSERVED, (retval: consumed -- value: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        macro(INSTRUMENTED_YIELD_VALUE) = _YIELD_VALUE_OBSERVED;

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

        /* Stack ops only: END_ASYNC_FOR and its instrumented form are the
         * EndAsyncFor family (Python/register_ops/cleanup.rop).  The word names
         * the SEND of its loop and does not go there: JUMPBY(0) gives the stack
         * op its target for the CFG (names_target_only). */
        tier1 inst(END_ASYNC_FOR,
                (awaitable_st: taken, exc_st: taken assumed(object) by exception_unwind -- ))
            effects(releases) exits(next, jump)
        {
            (void)oparg;
            PyValue_CLOSE(awaitable_st);
            PyValue_CLOSE(exc_st);
            JUMPBY(0);
        }

        tier1 inst(INSTRUMENTED_END_ASYNC_FOR,
                (awaitable_st: taken, exc_st: taken assumed(object) by exception_unwind -- ))
            effects(releases) exits(next, jump)
        {
            (void)oparg;
            PyValue_CLOSE(awaitable_st);
            PyValue_CLOSE(exc_st);
            JUMPBY(0);
        }

        /* A stack op only: CLEANUP_THROW is the CleanupThrow family
         * (Python/register_ops/cleanup.rop).  It has no oparg in the stack form;
         * its word names the exit. */
        tier1 inst(CLEANUP_THROW,
                (sub_iter: taken, null_in: taken nullable, last_sent_val: taken,
                 exc_value_st: taken assumed(object) by exception_unwind
                 -- none: owned, null_out: owned, value: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: LOAD_COMMON_CONSTANT is Const.Common (Python/register_ops/values.rop). */
        tier1 inst(LOAD_COMMON_CONSTANT, ( -- value: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
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

        /* A stack op only: BUILD_STRING is the BuildText family (Python/register_ops/text.rop). */
        tier1 inst(BUILD_STRING,
                (pieces[oparg]: consumed -- str: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: BUILD_INTERPOLATION is the BuildText family (Python/register_ops/text.rop). */
        tier1 inst(BUILD_INTERPOLATION,
                (value: consumed, str: consumed, format[oparg & 1]: consumed
                 -- interpolation: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: BUILD_TEMPLATE is the BuildText family (Python/register_ops/text.rop). */
        tier1 inst(BUILD_TEMPLATE,
                (strings: consumed, interpolations: consumed -- template: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
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
        translator_only inst(JUMP_IF_FALSE_OR_POP, (cond: consumed -- res: moved(cond) nullable))
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

        translator_only inst(JUMP_IF_TRUE_OR_POP, (cond: consumed -- res: moved(cond) nullable))
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

        translator_only tier1 inst(INSTRUMENTED_JUMP_IF_FALSE_OR_POP, (cond: closed -- ))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        translator_only tier1 inst(INSTRUMENTED_JUMP_IF_TRUE_OR_POP, (cond: closed -- ))
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

        /* A stack op only: GET_LEN is in the family of Python/register_ops/patterns.rop. */
        tier1 inst(GET_LEN,
                (obj -- obj, len: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: MATCH_CLASS is in the family of Python/register_ops/patterns.rop. */
        tier1 inst(MATCH_CLASS,
                (subject: consumed, type: consumed, names: consumed -- attrs: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: MATCH_MAPPING is in the family of Python/register_ops/patterns.rop. */
        tier1 inst(MATCH_MAPPING,
                (subject -- subject, res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: MATCH_SEQUENCE is in the family of Python/register_ops/patterns.rop. */
        tier1 inst(MATCH_SEQUENCE,
                (subject -- subject, res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: MATCH_KEYS is in the family of Python/register_ops/patterns.rop. */
        tier1 inst(MATCH_KEYS,
                (subject, keys -- subject, keys, values_or_none: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
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

        /* A stack op only: WITH_EXCEPT_START is the WithExit family
         * (Python/register_ops/cleanup.rop). */
        tier1 inst(WITH_EXCEPT_START,
                (exit_func, exit_self: nullable, lasti, unused,
                 val: assumed(object) by PUSH_EXC_INFO
                 -- exit_func, exit_self, lasti, unused, val, res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
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

        /* A stack op only: the marker at the start of a generator's code.  A
         * call creates the generator (_PyGen_NewCall()) with its frame at the
         * entry RESUME, and the assembler emits no word for the marker. */
        tier1 inst(RETURN_GENERATOR, (-- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: BUILD_SLICE is the BuildSlice family (Python/register_ops/items.rop). */
        tier1 inst(BUILD_SLICE, (args[oparg]: consumed -- slice: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: CONVERT_VALUE is the Convert family (Python/register_ops/text.rop). */
        tier1 inst(CONVERT_VALUE,
                (value: consumed -- result: owned))
            exits(abort)
        {
            (void)oparg;
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: FORMAT_SIMPLE is the Format family (Python/register_ops/text.rop). */
        tier1 inst(FORMAT_SIMPLE,
                (value: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
        }

        /* A stack op only: FORMAT_WITH_SPEC is the Format family (Python/register_ops/text.rop). */
        tier1 inst(FORMAT_WITH_SPEC,
                (value: consumed, fmt_spec: consumed -- res: owned))
            exits(abort)
        {
            assert(0 && "Executing a stack-only instruction.");
            Py_FatalError("Executing a stack-only instruction.");
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
