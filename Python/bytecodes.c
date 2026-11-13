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
        pure inst(NOP, (--)) {
        }

        inst(MATERIALIZE_KWARGS, (--)) {
            assert(frame->pending_kwargs == IR_NO_DST ||
                   frame->pending_kwargs == oparg);
            int err = _PyFrame_MaterializeKwargs(frame);
            ERROR_IF(err < 0);
        }

        op(_CHECK_PERIODIC, (--)) {
            int err = check_periodics(tstate);
            ERROR_IF(err != 0);
        }

        replaced op(_CHECK_PERIODIC_AT_END, (--)) {
            int err = check_periodics_at_end(tstate, frame);
            ERROR_IF(err != 0);
        }

        op(_CHECK_PERIODIC_IF_NOT_YIELD_FROM, (--)) {
            if ((oparg & RESUME_OPARG_LOCATION_MASK) < RESUME_AFTER_YIELD_FROM) {
                int err = check_periodics(tstate);
                ERROR_IF(err != 0);
            }
        }

        tier1 op(_MAYBE_INSTRUMENT, (--)) {
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
                    DISPATCH();
                }
            }
        }

        // A frame that starts or resumes at a RESUME skips it when it has
        // nothing to do (IR_SKIP_RESUME); it runs only after the words that
        // make cells and copy free variables, at a generator's first start,
        // and when the eval breaker or instrumentation needs it.
        macro(RESUME) =
            unused/1 +
            _MAYBE_INSTRUMENT +
            _CHECK_PERIODIC_IF_NOT_YIELD_FROM;

        op(_MONITOR_RESUME, (--)) {
            int err = _Py_call_instrumentation(
                    tstate, oparg == 0 ? PY_MONITORING_EVENT_PY_START : PY_MONITORING_EVENT_PY_RESUME, frame, this_instr);
            RELOAD_STACK();
            ERROR_IF(err);
            if (frame->instr_ptr != this_instr) {
                /* Instrumentation has jumped */
                next_instr = frame->instr_ptr;
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

        inst(LOAD_FAST_CHECK, (-- value)) {
            PyValueBorrow value_s = PyValue_Borrow(GETLOCAL(oparg));
            if (PyValue_IsNull(value_s)) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
                    UNBOUNDLOCAL_ERROR_MSG,
                    _PyTuple_GET_OBJECT(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
                );
                ERROR_IF(true);
            }
            value = PyValue_DUP(value_s);
        }

        pure inst(LOAD_FAST, (-- value)) {
            assert(!PyValue_IsNull(GETLOCAL(oparg)));
            value = PyValue_DUP(GETLOCAL(oparg));
        }

        // A temporary owns its value (pyjit/design/pyvalue.md D4): the load
        // takes a count of a mortal object. A load that a fold absorbs
        // reads the local in place instead (Python/regcfg_passes.c).
        pure inst (LOAD_FAST_BORROW, (-- value)) {
            assert(!PyValue_IsNull(GETLOCAL(oparg)));
            value = PyValue_DUP(GETLOCAL(oparg));
        }

        inst(LOAD_FAST_AND_CLEAR, (-- value)) {
            value = GETLOCAL(oparg);
            _PyFrame_WriteHome(frame, &GETLOCAL(oparg), PyValue_NULL);
        }

        // The assembler (Python/assemble.c) emits constant loads as the
        // inline forms.
        family(LOAD_CONST, 0) = {
            LOAD_CONST_INLINE_INT,
            LOAD_CONST_INLINE_FLOAT,
        };

        pure inst(LOAD_CONST, (-- value)) {
            value = PyValue_XDUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));
        }

        // The constant is an inline int.
        inst(LOAD_CONST_INLINE_INT, (-- value)) {
            value = PyValue_DUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));
            assert(PyValue_IsInlineInt(value));
        }

        // The constant is an exact float other than NaN. Only builds with
        // inline values quicken to this.
        inst(LOAD_CONST_INLINE_FLOAT, (-- value)) {
            value = PyValue_DUP(PyTuple_GET_VALUE(FRAME_CO_CONSTS, oparg));
            assert(PyValue_IsInlineFloat(value));
        }

        family(LOAD_SMALL_INT, 0) = {
            LOAD_SMALL_INT_INLINE,
        };

        pure inst(LOAD_SMALL_INT, (-- value)) {
            assert(oparg < _PY_NSMALLPOSINTS);
            value = PyValue_FromInlineInt(oparg);
        }

        inst(LOAD_SMALL_INT_INLINE, (-- value)) {
            assert(oparg < _PY_NSMALLPOSINTS);
            value = PyValue_FromInlineInt(oparg);
            ERROR_IF(PyValue_IsNull(value));
        }

        macro(STORE_FAST) = _SWAP_FAST + POP_TOP;

        op(_SWAP_FAST, (value:nullable -- trash)) {
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            _PyFrame_WriteHome(frame, &GETLOCAL(oparg), value);
            _PyValue_CheckHome(value);
            DEAD(value);
            trash = tmp;
        }

        pseudo(STORE_FAST_MAYBE_NULL, (unused --)) = {
            STORE_FAST,
        };

        pure inst(POP_TOP, (value:nullable --)) {
            PyValue_XCLOSE(value);
        }

        inline_ok op(_POP_TOP_INT, (value --)) {
            assert(PyValue_LongCheckExact(value));
            PyValue_CLOSE_SPECIALIZED(value, _PyLong_ExactDealloc);
        }

        inline_ok op(_POP_TOP_FLOAT, (value --)) {
            assert(PyValue_IsInlineFloat(value));
            PyValue_CLOSE_SPECIALIZED(value, _PyFloat_ExactDealloc);
        }

        // An operand of float arithmetic, an inline float or int.
        inline_ok op(_POP_TOP_NUMBER, (value --)) {
            assert(PyValue_IsInlineFloat(value) || PyValue_IsInlineInt(value));
            PyValue_CLOSE_SPECIALIZED(value, _PyFloat_OrLongExactDealloc);
        }

        op(_POP_TOP_UNICODE, (value --)) {
            assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value)));
            PyValue_CLOSE_SPECIALIZED(value, _PyUnicode_ExactDealloc);
        }

        op(_POP_TOP_OPARG, (args[oparg]:nullable -- )) {
            IR_CLOSE_INPUTS(args);
        }

        pure inst(PUSH_NULL, (-- res)) {
            res = PyValue_NULL;
        }

        no_save_ip inst(END_FOR, (value -- )) {
            /* Don't update instr_ptr, so that POP_ITER sees
             * the FOR_ITER as the previous instruction.
             * This has the benign side effect that if value is
             * finalized it will see the location as the FOR_ITER's.
             */
            PyValue_CLOSE(value);
        }


        inst(POP_ITER, (iter, index_or_null:nullable -- )) {
            (void)index_or_null;
            DEAD(index_or_null);
            PyValue_CLOSE(iter);
        }

        no_save_ip tier1 inst(INSTRUMENTED_END_FOR, (receiver, index_or_null:nullable, value -- receiver, index_or_null)) {
            /* Need to create a fake StopIteration error here,
             * to conform to PEP 380 */
            if (PyValue_GenCheck(receiver)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, PyValue_AsKnownObjectBorrow(value));
                if (err) {
                    ERROR_NO_POP();
                }
            }
            PyValue_CLOSE(value);
        }

        tier1 inst(INSTRUMENTED_POP_ITER, (iter, index_or_null:nullable -- )) {
            PyValue held = TAKE(iter);
            (void)index_or_null;
            DEAD(index_or_null);
            IR_DEFER_CLOSE(held);
            IR_OBSERVE_COMMITTED_BRANCH(prev_instr, next_instr,
                PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_FALLTHROUGH);
        }

        pure inst(END_SEND, (receiver, index_or_null:nullable, value -- val)) {
            val = TAKE(value);
            (void)index_or_null;
            DEAD(index_or_null);
            PyValue_CLOSE(receiver);
        }

        tier1 inst(INSTRUMENTED_END_SEND, (receiver, index_or_null:nullable, value -- val)) {
            PyObject *receiver_o = PyValue_AsKnownObjectBorrow(receiver);
            PyObject *value_o = PyValue_AsKnownObjectBorrow(value);
            // Taken before the monitoring call, which can run code
            PyValue held = TAKE(value);
            if (PyGen_Check(receiver_o) || PyCoro_CheckExact(receiver_o)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, value_o);
                if (err) {
                    PyValue_CLOSE(held);
                    ERROR_NO_POP();
                }
            }
            // The end of an `async for`'s send loop is its left branch. The
            // tools of other sends are none, and the call returns at once.
            if (!tstate->tracing) {
                _PyIRWord *dest = _Py_call_instrumentation_jump(
                    this_instr, tstate, PY_MONITORING_EVENT_BRANCH_LEFT, frame,
                    this_instr, next_instr);
                if (dest == NULL) {
                    PyValue_CLOSE(held);
                    ERROR_NO_POP();
                }
                next_instr = dest;
            }
            val = held;
            (void)index_or_null;
            DEAD(index_or_null);
            PyValue_CLOSE(receiver);
        }

        macro(UNARY_NEGATIVE) = _UNARY_NEGATIVE + POP_TOP;

        // Inline values are negated without a box. Only -Py_INT_MIN leaves
        // the inline range; negating a float that is not NaN gives one that
        // is not.
        inline_ok op(_UNARY_NEGATIVE, (value -- res, v)) {
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

        // UNARY_NOT tests the truth of any value: the compiler drops the
        // TO_BOOL before it (Python/flowgraph.c).  A bool is the fast path.
        inst(UNARY_NOT, (value -- res)) {
            int truth = _PyValue_IsTrue(value);
            PyValue_CLOSE(value);
            ERROR_IF(truth < 0);
            res = truth ? PyValue_False : PyValue_True;
        }

        family(TO_BOOL, INLINE_CACHE_ENTRIES_TO_BOOL) = {
            TO_BOOL_ALWAYS_TRUE,
            TO_BOOL_BOOL,
            TO_BOOL_INT,
            TO_BOOL_LIST,
            TO_BOOL_NONE,
            TO_BOOL_STR,
        };

        specializing op(_SPECIALIZE_TO_BOOL, (value -- value)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_ToBool(value, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(TO_BOOL);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // Floats do not specialize, so inline values are tested here
        // without a box.
        inline_ok op(_TO_BOOL, (value -- res)) {
            int err = _PyValue_IsTrue(value);
            PyValue_CLOSE(value);
            ERROR_IF(err < 0);
            res = err ? PyValue_True : PyValue_False;
        }

        macro(TO_BOOL) = unused/1 + _SPECIALIZE_TO_BOOL + unused/2 + _TO_BOOL;

        op(_GUARD_TOS_BOOL, (value -- value)) {
            EXIT_IF(!PyValue_BoolCheck(value));
            STAT_INC(TO_BOOL, hit);
        }

        macro(TO_BOOL_BOOL) = unused/1 + unused/2 + _GUARD_TOS_BOOL;

        inline_ok op(_TO_BOOL_INT, (value -- res, v)) {
            STAT_INC(TO_BOOL, hit);
            bool zero;
            if (PyValue_IsInlineInt(value)) {
                zero = PyValue_InlineInt64(value) == 0;
            }
            else {
                zero = _PyLong_IsZero((PyLongObject *)PyValue_AsKnownObjectBorrow(value));
            }
            res = zero ? PyValue_False : PyValue_True;
            v = value;
            DEAD(value);
        }

        macro(TO_BOOL_INT) =
            _GUARD_TOS_EXACT_INT + unused/1 + unused/2 + _TO_BOOL_INT + _POP_TOP_INT;

        op(_GUARD_NOS_LIST, (nos, unused -- nos, unused)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_TOS_LIST, (tos -- tos)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_TOS_SLICE, (tos -- tos)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PySlice_Check(o));
        }

        macro(TO_BOOL_LIST) = _GUARD_TOS_LIST + unused/1 + unused/2 + _TO_BOOL_LIST + POP_TOP;

        op(_TO_BOOL_LIST, (value -- res, v)) {
            PyObject *value_o = PyValue_AsKnownObjectBorrow(value);
            assert(PyList_CheckExact(value_o));
            STAT_INC(TO_BOOL, hit);
            res = PyList_GET_SIZE(value_o) ? PyValue_True : PyValue_False;
            v = value;
            DEAD(value);
        }

        op(_TO_BOOL_NONE, (value -- res)) {
            // This one is a bit weird, because we expect *some* failures:
            EXIT_IF(!PyValue_IsNone(value));
            DEAD(value);
            STAT_INC(TO_BOOL, hit);
            res = PyValue_False;
        }

        macro(TO_BOOL_NONE) = unused/1 + unused/2 + _TO_BOOL_NONE;

        op(_GUARD_NOS_COMPACT_ASCII, (nos, unused -- nos, unused)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyUnicode_CheckExact(o));
            EXIT_IF(!PyUnicode_IS_COMPACT_ASCII(o));
        }

        op(_GUARD_NOS_UNICODE, (nos, unused -- nos, unused)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyUnicode_CheckExact(o));
        }

        op(_GUARD_TOS_UNICODE, (value -- value)) {
            PyObject *value_o = PyValue_AsKnownObjectBorrow(value);
            EXIT_IF(!PyUnicode_CheckExact(value_o));
        }

        op(_TO_BOOL_STR, (value -- res, v)) {
            STAT_INC(TO_BOOL, hit);
            PyObject *value_o = PyValue_AsKnownObjectBorrow(value);
            res = value_o == &_Py_STR(empty) ? PyValue_False : PyValue_True;
            v = value;
            DEAD(value);
        }

        macro(TO_BOOL_STR) =
            _GUARD_TOS_UNICODE + unused/1 + unused/2 + _TO_BOOL_STR + _POP_TOP_UNICODE;

        op(_REPLACE_WITH_TRUE, (value -- res, v)) {
            res = PyValue_True;
            v = value;
            DEAD(value);
        }

        macro(TO_BOOL_ALWAYS_TRUE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _REPLACE_WITH_TRUE +
            POP_TOP;

        macro(UNARY_INVERT) = _UNARY_INVERT + POP_TOP;

        op(_UNARY_INVERT, (value -- res, v)) {
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
        inline_ok op(_GUARD_NOS_INT, (left, unused -- left, unused)) {
            EXIT_IF(!PyValue_IsInlineInt(left));
        }

        inline_ok op(_GUARD_TOS_INT, (value -- value)) {
            EXIT_IF(!PyValue_IsInlineInt(value));
        }

        inline_ok op(_GUARD_TOS_EXACT_INT, (value -- value)) {
            EXIT_IF(!PyValue_LongCheckExact(value));
        }

        // Int arithmetic on inline operands, on scaled values (see
        // _PyValue_IntScaled()): int64 overflow is exactly a result outside
        // the inline range, which is boxed.
        inline_ok pure op(_BINARY_OP_MULTIPLY_INT, (left, right -- res, l, r)) {
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

        inline_ok pure op(_BINARY_OP_ADD_INT, (left, right -- res, l, r)) {
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

        inline_ok pure op(_BINARY_OP_SUBTRACT_INT, (left, right -- res, l, r)) {
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

        macro(BINARY_OP_ADD_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_ADD_INT + _POP_TOP_INT + _POP_TOP_INT;

        macro(BINARY_OP_SUBTRACT_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_SUBTRACT_INT + _POP_TOP_INT + _POP_TOP_INT;

        // Python's // rounds the quotient towards minus infinity and % takes
        // the sign of the divisor, where C truncates. Division by zero
        // exits, and BINARY_OP raises. The operands are int51, so int64
        // division does not trap; only Py_INT_MIN // -1 leaves the inline
        // range, and PyValue_FromInt64() boxes it.
        inline_ok pure op(_BINARY_OP_FLOOR_DIVIDE_INT, (left, right -- res, l, r)) {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b == 0);
            int64_t a = PyValue_InlineInt64(left);
            STAT_INC(BINARY_OP, hit);
            int64_t quotient = a / b;
            if (a % b != 0 && (a ^ b) < 0) {
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
        inline_ok pure op(_BINARY_OP_REMAINDER_INT, (left, right -- res, l, r)) {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b == 0);
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
        inline_ok pure op(_BINARY_OP_AND_INT, (left, right -- res, l, r)) {
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromInlineInt(PyValue_InlineInt64(left) & PyValue_InlineInt64(right));
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        inline_ok pure op(_BINARY_OP_OR_INT, (left, right -- res, l, r)) {
            STAT_INC(BINARY_OP, hit);
            res = PyValue_FromInlineInt(PyValue_InlineInt64(left) | PyValue_InlineInt64(right));
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        inline_ok pure op(_BINARY_OP_XOR_INT, (left, right -- res, l, r)) {
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
        inline_ok pure op(_BINARY_OP_LSHIFT_INT, (left, right -- res, l, r)) {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b < 0);
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
        inline_ok pure op(_BINARY_OP_RSHIFT_INT, (left, right -- res, l, r)) {
            int64_t b = PyValue_InlineInt64(right);
            EXIT_IF(b < 0);
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
        inline_ok op(_GUARD_NOS_FLOAT, (left, unused -- left, unused)) {
            EXIT_IF(!PyValue_IsInlineFloat(left));
        }

        inline_ok op(_GUARD_TOS_FLOAT, (value -- value)) {
            EXIT_IF(!PyValue_IsInlineFloat(value));
        }

        // Float arithmetic takes two inline floats, or an inline float and
        // an inline int (see PyValue_FloatOperandsCheck()). The test and the
        // reads are one step, so that two floats get a path of their own.
        // The result is inline unless it is NaN.
        inline_ok pure op(_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res, l, r)) {
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

        inline_ok pure op(_BINARY_OP_ADD_FLOAT, (left, right -- res, l, r)) {
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

        inline_ok pure op(_BINARY_OP_SUBTRACT_FLOAT, (left, right -- res, l, r)) {
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
        inline_ok pure op(_BINARY_OP_TRUEDIV_FLOAT, (left, right -- res, l, r)) {
            double a, b;
            EXIT_IF(!PyValue_FloatOperandValues(left, right, &a, &b));
            EXIT_IF(b == 0.0);
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
        macro(BINARY_OP_ADD_FLOAT) =
            unused/5 + _BINARY_OP_ADD_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;
        macro(BINARY_OP_SUBTRACT_FLOAT) =
            unused/5 + _BINARY_OP_SUBTRACT_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;
        macro(BINARY_OP_TRUEDIV_FLOAT) =
            unused/5 + _BINARY_OP_TRUEDIV_FLOAT + _POP_TOP_NUMBER + _POP_TOP_NUMBER;

        pure op(_BINARY_OP_ADD_UNICODE, (left, right -- res, l, r)) {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
            assert(PyUnicode_CheckExact(left_o));
            assert(PyUnicode_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = PyUnicode_Concat(left_o, right_o);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            res = PyValue_FromObjectSteal(res_o);
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_ADD_UNICODE) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_ADD_UNICODE + _POP_TOP_UNICODE + _POP_TOP_UNICODE;

        // This is a subtle one. We write NULL to the local that dst
        // names, the target of the STORE_FAST folded into the word, and
        // leave the result for IR_STORE_RESULT to store.
        op(_BINARY_OP_INPLACE_ADD_UNICODE, (left, right -- res)) {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            assert(PyUnicode_CheckExact(left_o));
            assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(right)));

            assert(IR_IS_LOCAL(IR_DST));
            PyValue *target_local = &IR_SLOT(IR_DST);
            assert(PyUnicode_CheckExact(left_o));
            EXIT_IF(PyValue_AsKnownObjectBorrow(*target_local) != left_o);
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
            // The local still holds the string
            assert(Py_REFCNT(left_o) >= 1);
            PyObject *temp = PyValue_AsKnownObjectSteal(*target_local);
            PyObject *right_o = PyValue_AsKnownObjectSteal(right);
            _PyFrame_WriteHome(frame, target_local, PyValue_NULL);
            temp = value_unicode_append(temp, right_o);
            _Py_DECREF_SPECIALIZED(right_o, _PyUnicode_ExactDealloc);
            ERROR_IF(temp == NULL);
            res = PyValue_FromObjectSteal(temp);
        }

       // These legacy descriptors guard objects. Decline canonical inline
       // operands before creating a C bridge; the generic operation handles
       // them without carrying private box owners through a deopt.
       inline_ok op(_BINARY_OP_EXTEND, (descr/4, left, right -- res, l, r)) {
            EXIT_IF(!PyValue_IsObject(left) || !PyValue_IsObject(right));
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
            assert(INLINE_CACHE_ENTRIES_BINARY_OP == 5);
            _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr*)descr;
            assert(d != NULL);
            EXIT_IF(Py_TYPE(left_o) != d->lhs_type || Py_TYPE(right_o) != d->rhs_type);

            STAT_INC(BINARY_OP, hit);

            PyObject *res_o = d->action(left_o, right_o);
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
            unused/1 + _BINARY_OP_EXTEND + POP_TOP + POP_TOP;

        macro(BINARY_OP_INPLACE_ADD_UNICODE) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_INPLACE_ADD_UNICODE;

        specializing op(_SPECIALIZE_BINARY_SLICE, (container, start, stop -- container, start, stop)) {
            // Placeholder until we implement BINARY_SLICE specialization
            #if ENABLE_SPECIALIZATION
            OPCODE_DEFERRED_INC(BINARY_SLICE);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // Inline bounds slice a list, tuple or str without boxes or a slice
        // object; see _PyEval_BinarySlice().
        inline_ok op(_BINARY_SLICE, (container, start, stop -- res)) {
            res = _PyEval_BinarySlice(container, start, stop);
            DECREF_INPUTS();
            ERROR_IF(PyValue_IsNull(res));
        }

        macro(BINARY_SLICE) = _SPECIALIZE_BINARY_SLICE + _BINARY_SLICE;

        specializing op(_SPECIALIZE_STORE_SLICE, (v, container, start, stop -- v, container, start, stop)) {
            // Placeholder until we implement STORE_SLICE specialization
            #if ENABLE_SPECIALIZATION
            OPCODE_DEFERRED_INC(STORE_SLICE);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // Inline bounds store to a list slice without boxes or a slice
        // object; see _PyEval_StoreSlice().
        inline_ok op(_STORE_SLICE, (v, container, start, stop -- )) {
            int err = _PyEval_StoreSlice(v, container, start, stop);
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        macro(STORE_SLICE) = _SPECIALIZE_STORE_SLICE + _STORE_SLICE;

        macro(BINARY_OP_SUBSCR_LIST_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_LIST + unused/5 + _BINARY_OP_SUBSCR_LIST_INT + _POP_TOP_INT + POP_TOP;

        inline_ok op(_BINARY_OP_SUBSCR_LIST_INT, (list_st, sub_st -- res, ls, ss)) {
            PyObject *list = PyValue_AsKnownObjectBorrow(list_st);
            assert(PyList_CheckExact(list));

            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            if (index < 0) {
                index += PyList_GET_SIZE(list);
            }
            EXIT_IF(index < 0 || index >= PyList_GET_SIZE(list));
            res = PyValue_XDUP(PyList_GET_VALUE(list, index));
            assert(!PyValue_IsNull(res));
            STAT_INC(BINARY_OP, hit);
            ls = list_st;
            ss = sub_st;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_SUBSCR_LIST_SLICE) =
            _GUARD_TOS_SLICE + _GUARD_NOS_LIST + unused/5 + _BINARY_OP_SUBSCR_LIST_SLICE + POP_TOP + POP_TOP;

        op(_BINARY_OP_SUBSCR_LIST_SLICE, (list_st, sub_st -- res, ls, ss)) {
            PyObject *sub = PyValue_AsKnownObjectBorrow(sub_st);
            PyObject *list = PyValue_AsKnownObjectBorrow(list_st);

            assert(PySlice_Check(sub));
            assert(PyList_CheckExact(list));

            PyObject *res_o = _PyList_SliceSubscript(list, sub);
            STAT_INC(BINARY_OP, hit);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            res = PyValue_FromObjectSteal(res_o);
            ls = list_st;
            ss = sub_st;
            INPUTS_DEAD();
        }

        macro(BINARY_OP_SUBSCR_STR_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_COMPACT_ASCII + unused/5 + _BINARY_OP_SUBSCR_STR_INT + _POP_TOP_INT + _POP_TOP_UNICODE;

        inline_ok op(_BINARY_OP_SUBSCR_STR_INT, (str_st, sub_st -- res, s, i)) {
            PyObject *str = PyValue_AsKnownObjectBorrow(str_st);
            assert(PyUnicode_CheckExact(str));

            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            EXIT_IF(index < 0);
            EXIT_IF(PyUnicode_GET_LENGTH(str) <= index);
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

        inline_ok op(_BINARY_OP_SUBSCR_USTR_INT, (str_st, sub_st -- res, s, i)) {
            PyObject *str = PyValue_AsKnownObjectBorrow(str_st);
            assert(PyUnicode_CheckExact(str));

            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            EXIT_IF(index < 0);
            EXIT_IF(PyUnicode_GET_LENGTH(str) <= index);
            // Specialize for reading an ASCII character from any string:
            Py_UCS4 c = PyUnicode_READ_CHAR(str, index);
            EXIT_IF(Py_ARRAY_LENGTH(_Py_SINGLETON(strings).ascii) <= c);
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = (PyObject*)&_Py_SINGLETON(strings).ascii[c];
            s = str_st;
            i = sub_st;
            INPUTS_DEAD();
            res = PyValue_DUP(PyValue_FromObjectBorrow(res_o));
        }

        op(_GUARD_NOS_TUPLE, (nos, unused -- nos, unused)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        op(_GUARD_TOS_TUPLE, (tos -- tos)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        macro(BINARY_OP_SUBSCR_TUPLE_INT) =
            _GUARD_TOS_INT +
            _GUARD_NOS_TUPLE +
            _GUARD_BINARY_OP_SUBSCR_TUPLE_INT_BOUNDS +
            unused/5 +
            _BINARY_OP_SUBSCR_TUPLE_INT +
            _POP_TOP_INT +
            POP_TOP;

        // A guard that checks that the tuple subscript is within bounds
        inline_ok op(_GUARD_BINARY_OP_SUBSCR_TUPLE_INT_BOUNDS, (tuple_st, sub_st -- tuple_st, sub_st)) {
            PyObject *tuple = PyValue_AsKnownObjectBorrow(tuple_st);
            assert(PyTuple_CheckExact(tuple));

            // Deopt unless 0 <= sub < PyTuple_Size(list)
            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            EXIT_IF(index < 0);
            EXIT_IF(index >= PyTuple_GET_SIZE(tuple));
        }

        inline_ok op(_BINARY_OP_SUBSCR_TUPLE_INT, (tuple_st, sub_st -- res, ts, ss)) {
            PyObject *tuple = PyValue_AsKnownObjectBorrow(tuple_st);
            assert(PyTuple_CheckExact(tuple));

            STAT_INC(BINARY_OP, hit);
            Py_ssize_t index = PyValue_InlineInt64(sub_st);
            assert(0 <= index && index < PyTuple_GET_SIZE(tuple));
            res = PyValue_XDUP(PyTuple_GET_VALUE(tuple, index));
            assert(!PyValue_IsNull(res));
            ts = tuple_st;
            ss = sub_st;
            INPUTS_DEAD();
        }

        op(_GUARD_NOS_DICT_SUBSCRIPT, (nos, unused -- nos, unused)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            DEOPT_IF(!Py_TYPE(o)->tp_as_mapping);
            DEOPT_IF(Py_TYPE(o)->tp_as_mapping->mp_subscript != _PyDict_Subscript);
        }

        op(_GUARD_NOS_DICT_STORE_SUBSCRIPT, (unused, nos, unused -- unused, nos, unused)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(nos);
            DEOPT_IF(!Py_TYPE(o)->tp_as_mapping);
            DEOPT_IF(Py_TYPE(o)->tp_as_mapping->mp_ass_subscript != _PyDict_StoreSubscript);
        }

        op(_GUARD_TOS_ANY_DICT, (tos -- tos)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyAnyDict_CheckExact(o));
        }

        macro(BINARY_OP_SUBSCR_DICT) =
            _GUARD_NOS_DICT_SUBSCRIPT + unused/5 + _BINARY_OP_SUBSCR_DICT;

        inline_ok op(_BINARY_OP_SUBSCR_DICT, (dict_st, sub_st -- res)) {
            IR_TRY_DICT_FIRST_PROBE();
            PyValue held_dict = TAKE(dict_st);
            PyValue held_sub = TAKE(sub_st);
            INPUTS_DEAD();
            SYNC_SP();
            STAT_INC(BINARY_OP, hit);
            PyValue pending = value_subscript(held_dict, held_sub);
            if (PyValue_IsNull(pending)) {
                IR_RESTORE_VALUE_INPUTS(held_dict, held_sub);
                value_operation_close3(held_sub, held_dict, PyValue_NULL);
                IR_ABORT_VALUE_READER();
            }
            IR_COMPLETE_VALUE_READER(pending, held_sub, held_dict);
            res = pending;
        }

        inline_ok op(_BINARY_OP_SUBSCR_CHECK_FUNC, (container, unused -- container, unused, getitem)) {
            EXIT_IF(!PyValue_IsObject(container));
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(container));
            EXIT_IF(!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE));
            PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
            PyObject *getitem_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(ht->_spec_cache.getitem);
            EXIT_IF(getitem_o == NULL);
            assert(PyFunction_Check(getitem_o));
            uint32_t cached_version = FT_ATOMIC_LOAD_UINT32_RELAXED(ht->_spec_cache.getitem_version);
            EXIT_IF(((PyFunctionObject *)getitem_o)->func_version != cached_version);
            PyCodeObject *code = (PyCodeObject *)PyFunction_GET_CODE(getitem_o);
            assert(code->co_argcount == 2);
            EXIT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            getitem = PyValue_DUP(PyValue_FromObjectBorrow(getitem_o));
        }

        op(_BINARY_OP_SUBSCR_INIT_CALL, (container, sub, getitem -- new_frame)) {
            STAT_INC(BINARY_OP, hit);
            _PyInterpreterFrame* pushed_frame = _PyFrame_PushUnchecked(tstate, getitem, frame);
            _PyFrame_WriteHome(pushed_frame, pushed_frame->base, TAKE(container));
            _PyFrame_WriteHome(pushed_frame, pushed_frame->base + 1, TAKE(sub));
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

        inline_ok inst(LIST_APPEND, (list, unused[oparg-1], v -- list, unused[oparg-1])) {
            PyValue item = TAKE(v);
            int err = _PyList_AppendTakeValue((PyListObject *)PyValue_AsKnownObjectBorrow(list), item);
            ERROR_IF(err < 0);
        }

        inline_ok inst(SET_ADD, (set, unused[oparg-1], v -- set, unused[oparg-1])) {
            PyValue item = TAKE(v);
            int err = _PySet_AddValueTakeRef((PySetObject *)PyValue_AsKnownObjectBorrow(set), item);
            ERROR_IF(err);
        }

        family(STORE_SUBSCR, INLINE_CACHE_ENTRIES_STORE_SUBSCR) = {
            STORE_SUBSCR_DICT,
            STORE_SUBSCR_LIST_INT,
        };

        specializing op(_SPECIALIZE_STORE_SUBSCR, (container, sub -- container, sub)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_StoreSubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(STORE_SUBSCR);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        inline_ok op(_STORE_SUBSCR, (v, container, sub -- )) {
            /* container[sub] = v */
            PyValue held_value = TAKE(v);
            PyValue held_container = TAKE(container);
            PyValue held_sub = TAKE(sub);
            INPUTS_DEAD();
            SYNC_SP();
            int err = _PyValue_SetItem(held_container, held_sub, held_value);
            value_operation_close3(held_sub, held_container, held_value);
            IR_FINISH_VALUE_OPERATION();
            ERROR_IF(err);
        }

        macro(STORE_SUBSCR) = unused/1 + _SPECIALIZE_STORE_SUBSCR + _STORE_SUBSCR;

        // The index is released by POP_TOP, which does not read it: the
        // release of the replaced item before it can run code (S7)
        macro(STORE_SUBSCR_LIST_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_LIST + unused/1 + _STORE_SUBSCR_LIST_INT + POP_TOP + POP_TOP;

        // The index and stored value stay in canonical representation.
        inline_ok op(_STORE_SUBSCR_LIST_INT, (value, list_st, sub_st -- ls, ss)) {
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
                DEOPT_IF(true);
            }
            STAT_INC(STORE_SUBSCR, hit);

            // a move: replaced below
            PyValue old_value = _PyList_ITEMS(list)[index];
            _PyList_ITEMS(list)[index] = TAKE(value);
            assert(!PyValue_IsNull(old_value));
            UNLOCK_OBJECT(list);  // unlock before decrefs!
            INPUTS_DEAD();
            ls = list_st;
            ss = sub_st;
            PyValue_CLOSE(old_value);
        }

        macro(STORE_SUBSCR_DICT) =
            _GUARD_NOS_DICT_STORE_SUBSCRIPT + unused/1 + _STORE_SUBSCR_DICT;

        inline_ok op(_STORE_SUBSCR_DICT, (value, dict_st, sub --)) {
            PyObject *dict = PyValue_AsKnownObjectBorrow(dict_st);
            assert(Py_TYPE(dict)->tp_as_mapping->mp_ass_subscript == _PyDict_StoreSubscript);
            STAT_INC(STORE_SUBSCR, hit);
            PyValue key = TAKE(sub);
            PyValue stored = TAKE(value);
            PyValue retained_dict = TAKE(dict_st);
            INPUTS_DEAD();
            SYNC_SP();
            int err = _PyDict_SetValue_Take2((PyDictObject *)dict, key, stored);
            value_operation_close3(retained_dict, PyValue_NULL, PyValue_NULL);
            IR_FINISH_VALUE_OPERATION();
            ERROR_IF(err);
        }

        inst(DELETE_SUBSCR, (container, sub --)) {
            /* del container[sub] */
            int err = _PyValue_DelItem(container, sub);
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        inline_ok op(_CALL_INTRINSIC_1, (value -- res, v)) {
            assert(oparg <= MAX_INTRINSIC_1);
            if (oparg == INTRINSIC_UNARY_POSITIVE) {
                res = _PyValue_Positive(value);
                if (PyValue_IsNull(res)) {
                    ERROR_NO_POP();
                }
            }
            else {
                // The other intrinsics take objects
                PyObject *value_o = _PyValueTemps_Object(value_temps, value);
                if (value_o == NULL) {
                    ERROR_NO_POP();
                }
                PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(tstate, value_o);
                if (res_o == NULL) {
                    ERROR_NO_POP();
                }
                res = PyValue_FromObjectSteal(res_o);
            }
            v = value;
            DEAD(value);
        }

        macro(CALL_INTRINSIC_1) = _CALL_INTRINSIC_1 + POP_TOP;

        inline_ok op(_CALL_INTRINSIC_2, (value2_st, value1_st -- res, vs1, vs2)) {
            assert(oparg <= MAX_INTRINSIC_2);
            // Taken first: the calls below can run code that rebinds a local
            PyValue first = TAKE(value1_st);
            PyValue second = TAKE(value2_st);
            INPUTS_DEAD();
            if (oparg == INTRINSIC_GETATTR) {
                // The owner is a value; the name is a str
                assert(PyValue_IsObject(first));
                res = _PyValue_GetAttr(second, PyValue_AsKnownObjectBorrow(first));
                if (PyValue_IsNull(res)) {
                    value_operation_close3(first, second, PyValue_NULL);
                    IR_ABORT_VALUE_READER();
                }
            }
            else if (oparg == INTRINSIC_SETATTR) {
                // second is the value, NULL for a deletion; first is the
                // tuple (owner, name)
                PyObject *target = PyValue_AsKnownObjectBorrow(first);
                assert(PyTuple_CheckExact(target) && PyTuple_GET_SIZE(target) == 2);
                PyValue owner = PyTuple_GetValue(target, 0);
                PyValue attr_name = PyTuple_GetValue(target, 1);
                assert(PyValue_IsObject(attr_name));
                int err = _PyValue_SetAttr(owner, PyValue_AsKnownObjectBorrow(attr_name),
                                           second);
                PyValue_CLOSE(owner);
                PyValue_CLOSE(attr_name);
                if (err < 0) {
                    value_operation_close3(first, second, PyValue_NULL);
                    IR_ABORT_VALUE_READER();
                }
                res = PyValue_None;
            }
            else {
                PyObject *value1 = _PyValueTemps_Object(value_temps, first);
                PyObject *value2 = value1 == NULL ? NULL :
                    _PyValueTemps_Object(value_temps, second);
                PyObject *res_o = value2 == NULL ? NULL :
                    _PyIntrinsics_BinaryFunctions[oparg].func(tstate, value2, value1);
                if (res_o == NULL) {
                    value_operation_close3(first, second, PyValue_NULL);
                    IR_ABORT_VALUE_READER();
                }
                res = PyValue_FromObjectSteal(res_o);
            }
            vs1 = first;
            vs2 = second;
        }

        macro(CALL_INTRINSIC_2) = _CALL_INTRINSIC_2 + POP_TOP + POP_TOP;

        tier1 inst(RAISE_VARARGS, (args[oparg] -- )) {
            assert(oparg < 3);
            _PyValue_RecheckWindow(args, oparg);
            PyObject *exc = oparg > 0 ? _PyValueTemps_Object(value_temps, args[0]) : NULL;
            if (oparg > 0 && exc == NULL) {
                ERROR_NO_POP();
            }
            PyObject *cause = oparg == 2 ? _PyValueTemps_Object(value_temps, args[1]) : NULL;
            if (oparg == 2 && cause == NULL) {
                ERROR_NO_POP();
            }
            /* do_raise consumes independent object owners; homes remain
             * canonical until their semantic input ownership is detached. */
            Py_XINCREF(exc);
            Py_XINCREF(cause);
            DECREF_INPUTS();
            int err = do_raise(tstate, exc, cause);
            if (err) {
                assert(oparg == 0);
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
            ERROR_IF(true);
        }

        inline_ok tier1 inst(INTERPRETER_EXIT, (retval --)) {
            assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
            assert(_PyFrame_IsIncomplete(frame));
            /* Restore previous frame and return. */
            tstate->current_frame = frame->previous;
            assert(!_PyErr_Occurred(tstate));
            // The value goes to the caller of the evaluation loop as it is:
            // an inline value is not boxed here
            PyValue result = TAKE(retval);
            INPUTS_DEAD();
            assert(frame == &IR_ACTIVATION()->entry.frame);
            LLTRACE_RESUME_FRAME();
            return result;
        }

        // The returned or yielded value goes to another frame, a home
        op(_MAKE_HEAP_SAFE, (value -- taken)) {
            taken = TAKE(value);
        }

        // The result remains C-owned through child retirement. Completion
        // publishes it to the caller or consumes it as a loop/send outcome.
        returns op(_RETURN_VALUE, (retval --)) {
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            PyValue temp = retval;
            DEAD(retval);
            SAVE_STACK();
            DTRACE_FUNCTION_RETURN();
            _Py_LeaveRecursiveCallPy(tstate);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            int generator_return = dying->owner == FRAME_OWNED_BY_GENERATOR;
            frame = tstate->current_frame = dying->previous;
            temp = _PyEval_FrameClearAndPopGeneralWithResult(tstate, dying, temp);
            RELOAD_STACK();
            IR_RETURN_TO_CALLER(temp, generator_return);
        }

        ir_only returns op(_RETURN_VALUE_EMPTY, (retval --)) {
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            PyValue temp = retval;
            DEAD(retval);
            SAVE_STACK();
            DTRACE_FUNCTION_RETURN();
            _Py_LeaveRecursiveCallPy(tstate);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            int generator_return = dying->owner == FRAME_OWNED_BY_GENERATOR;
            frame = tstate->current_frame = dying->previous;
            temp = _PyEval_FrameClearAndPopWithResult(tstate, dying, temp);
            RELOAD_STACK();
            IR_RETURN_TO_CALLER(temp, generator_return);
        }

        macro(RETURN_VALUE) =
            _MAKE_HEAP_SAFE +
            _RETURN_VALUE;

        macro(RETURN_VALUE_EMPTY) =
            _MAKE_HEAP_SAFE +
            _RETURN_VALUE_EMPTY;

        // The value is taken before the event, which can run code
        tier1 op(_RETURN_VALUE_EVENT, (val -- taken)) {
            PyValue held = TAKE(val);
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_RETURN,
                    frame, this_instr, held);
            if (err) {
                PyValue_CLOSE(held);
                ERROR_IF(true);
            }
            taken = held;
        }

        macro(INSTRUMENTED_RETURN_VALUE) =
            _RETURN_VALUE_EVENT +
            _RETURN_VALUE;

        macro(INSTRUMENTED_RETURN_VALUE_EMPTY) =
            _RETURN_VALUE_EVENT +
            _RETURN_VALUE_EMPTY;

        inst(GET_AITER, (obj -- iter)) {
            unaryfunc getter = NULL;
            PyObject *obj_o = PyValue_AsKnownObjectBorrow(obj);
            PyObject *iter_o;
            PyTypeObject *type = Py_TYPE(obj_o);

            if (type->tp_as_async != NULL) {
                getter = type->tp_as_async->am_aiter;
            }

            if (getter == NULL) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' requires an object with "
                              "__aiter__ method, got %.100s",
                              type->tp_name);
                PyValue_CLOSE(obj);
                ERROR_IF(true);
            }

            iter_o = (*getter)(obj_o);
            PyValue_CLOSE(obj);
            ERROR_IF(iter_o == NULL);

            if (Py_TYPE(iter_o)->tp_as_async == NULL ||
                    Py_TYPE(iter_o)->tp_as_async->am_anext == NULL) {

                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' received an object from __aiter__ "
                              "that does not implement __anext__: %.100s",
                              Py_TYPE(iter_o)->tp_name);
                Py_DECREF(iter_o);
                ERROR_IF(true);
            }
            iter = PyValue_FromObjectSteal(iter_o);
        }

        inst(GET_ANEXT, (aiter -- aiter, awaitable)) {
            PyObject *awaitable_o = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(aiter));
            if (awaitable_o == NULL) {
                ERROR_NO_POP();
            }
            awaitable = PyValue_FromObjectSteal(awaitable_o);
        }

        inst(GET_AWAITABLE, (iterable -- iter)) {
            PyObject *iter_o = _PyEval_GetAwaitable(PyValue_AsKnownObjectBorrow(iterable), oparg);
            PyValue_CLOSE(iterable);
            ERROR_IF(iter_o == NULL);
            iter = PyValue_FromObjectSteal(iter_o);
        }

        family(SEND, INLINE_CACHE_ENTRIES_SEND) = {
            SEND_GEN,
            SEND_VIRTUAL,
            SEND_ASYNC_GEN,
        };

        specializing op(_SPECIALIZE_SEND, (receiver, unused, unused -- receiver, unused, unused)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_Send(receiver, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(SEND);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        tier1 window op(_SEND, (receiver, null_or_index:nullable, v -- receiver, null_or_index, retval)) {
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
            if (!PyValue_IsNull(null_or_index) && PyValue_IsNone(v)) {
                _PyStatusValue next = _PyForIter_VirtualIteratorNext(tstate, frame, receiver, &window[1]);
                null_or_index = _PyFrame_ReadHome(frame, window + 1);
                PyValue item = next.value;
                int found = next.status;
                if (found <= 0) {
                    if (found < 0) {
                        ERROR_NO_POP();
                    }
                    PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;
                    IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);
                }
                retval = item;
                DEAD(v);
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
                retval = sent.value;
            }
        }

        macro(SEND) = unused/1 + _SPECIALIZE_SEND + _SEND;

        inline_ok op(_SEND_GEN_FRAME, (receiver, null, v -- receiver, null, gen_frame)) {
            EXIT_IF(!PyValue_IsObject(receiver));
            PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(receiver);
            EXIT_IF(Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type);
            EXIT_IF(FT_ATOMIC_LOAD_INT8_RELAXED(gen->gi_frame_state) == FRAME_CREATED && !PyValue_IsNone(v));
            EXIT_IF(!gen_try_set_executing((PyGenObject *)gen));
            STAT_INC(SEND, hit);
            IR_ENTER_GENERATOR(gen, _PY_IR_SEND);
        }

        macro(SEND_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _SEND_GEN_FRAME +
            _PUSH_FRAME;

        op(_GUARD_TOS_IS_NONE, (val -- val)) {
            EXIT_IF(!PyValue_IsNone(val));
        }

        macro(FOR_ITER) = unused/1 + _SPECIALIZE_FOR_ITER + _FOR_ITER;

        op(_GUARD_NOS_NOT_NULL, (nos:nullable, unused -- nos, unused)) {
            EXIT_IF(PyValue_IsNull(nos));
        }

        replaced op(_SEND_VIRTUAL, (iter, null_or_index, none -- iter, null_or_index, next)) {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            Py_ssize_t index = PyValue_UntagIndex(null_or_index);
            _PyValueIndexPair next_index = Py_TYPE(iter_o)->_tp_iteritem(iter_o, index);
            index = next_index.index;
            if (PyValue_IsNull(next_index.value)) {
                if (index < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_SEND, completed, oparg);
            }
            DEAD(none);
            next = next_index.value;
            null_or_index = PyValue_TagIndex(index);
        }

        macro(SEND_VIRTUAL) =
            unused/1 +
            _GUARD_TOS_IS_NONE +
            _GUARD_NOS_NOT_NULL +
            _SEND_VIRTUAL;

        op(_GUARD_3OS_ASYNC_GEN_ASEND, (iter, null_or_index, value -- iter, null_or_index, value)) {
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(!PyAsyncGenASend_CheckExact(iter_o));
        }

        replaced op(_SEND_ASYNC_GEN, (iter, null_in:nullable, v -- asend, null_out, retval)) {
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

        window op(_YIELD_VALUE, (retval -- value)) {
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

        tier1 op(_YIELD_VALUE_EVENT, (val -- val)) {
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_YIELD,
                    frame, this_instr, val);
            if (err) {
                RELOAD_STACK();
                ERROR_NO_POP();
            }
            if (frame->instr_ptr != this_instr) {
                RELOAD_STACK();
                next_instr = frame->instr_ptr;
                DISPATCH();
            }
        }

        macro(INSTRUMENTED_YIELD_VALUE) =
            _YIELD_VALUE_EVENT +
            _MAKE_HEAP_SAFE +
            _YIELD_VALUE;

        ir_only no_fold op(_YIELD_DELEGATED,
            (receiver, unused_cursor, retval -- receiver, unused_cursor, value)) {
            IR_METADATA_PASSTHROUGH(1);
            PyValue outgoing = TAKE(retval);
            IR_SUSPEND_DELEGATED(outgoing);
        }

        macro(YIELD_DELEGATED) = _YIELD_DELEGATED;

        macro(INSTRUMENTED_YIELD_DELEGATED) =
            _YIELD_VALUE_EVENT + _YIELD_DELEGATED;

        inst(POP_EXCEPT, (exc_value -- )) {
            _PyErr_StackItem *exc_info = tstate->exc_info;
            Py_XSETREF(exc_info->exc_value,
                   PyValue_IsNone(exc_value)
                    ? NULL : PyValue_AsKnownObjectSteal(exc_value));
        }

        tier1 inst(RERAISE, (values[oparg], exc_st -- values[oparg])) {
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

        tier1 window op(_END_ASYNC_FOR, (awaitable_st, exc_st -- )) {
            JUMPBY(0); // Pretend jump as we need source offset for monitoring
            (void)oparg;
            PyObject *exc = PyValue_AsKnownObjectBorrow(exc_st);

            assert(exc && PyExceptionInstance_Check(exc));
            int matches = PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration);
            if (matches) {
                DECREF_INPUTS();
            }
            else {
                Py_INCREF(exc);
                _PyErr_SetRaisedException(tstate, exc);
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
        }

        tier1 op(_MONITOR_END_ASYNC_FOR, ( -- )) {
            assert(_PyIR_BaseOpcode(_PyIR_RealHandler(frame, JUMP_TARGET(oparg))) == SEND);
            INSTRUMENTED_JUMP(JUMP_TARGET(oparg), this_instr+1, PY_MONITORING_EVENT_BRANCH_RIGHT);
        }

        macro(INSTRUMENTED_END_ASYNC_FOR) =
            _MONITOR_END_ASYNC_FOR +
            _END_ASYNC_FOR;

        macro(END_ASYNC_FOR) = _END_ASYNC_FOR;

        tier1 inst(CLEANUP_THROW, (sub_iter, null_in, last_sent_val, exc_value_st -- none, null_out, value)) {
            PyObject *exc_value = PyValue_AsKnownObjectBorrow(exc_value_st);
            assert(exc_value && PyExceptionInstance_Check(exc_value));
            int matches = PyErr_GivenExceptionMatches(exc_value, PyExc_StopIteration);
            if (!matches) {
                _PyErr_SetRaisedException(tstate, Py_NewRef(exc_value));
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
            PyValue completed = PyValue_DUP(((PyStopIterationObject *)exc_value)->value);
            IR_COMPLETE_STEP(_PY_IR_SEND_THROW, completed, oparg);
        }

        pure inst(LOAD_COMMON_CONSTANT, ( -- value)) {
            // Keep in sync with _common_constants in opcode.py
            assert(oparg < NUM_COMMON_CONSTANTS);
            value = PyValue_DUP(tstate->interp->common_consts[oparg]);
        }

        inst(LOAD_BUILD_CLASS, ( -- bc)) {
            _PyStatusValue found = _PyValue_GetOptionalItem(
                PyValue_FromObjectBorrow(BUILTINS()),
                PyValue_FromObjectBorrow(&_Py_ID(__build_class__)));
            ERROR_IF(found.status < 0);
            if (found.status == 0) {
                _PyErr_SetString(tstate, PyExc_NameError,
                                 "__build_class__ not found");
                ERROR_IF(true);
            }
            bc = found.value;
        }

        inst(STORE_NAME, (v -- )) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *ns = LOCALS();
            int error = _PyEval_StoreName(tstate, v, name, ns);
            PyValue_XCLOSE(v);
            ERROR_IF(error);
        }

        family(UNPACK_SEQUENCE, INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE) = {
            UNPACK_SEQUENCE_TWO_TUPLE,
            UNPACK_SEQUENCE_TUPLE,
            UNPACK_SEQUENCE_LIST,
        };

        specializing op(_SPECIALIZE_UNPACK_SEQUENCE, (seq -- seq)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_UnpackSequence(seq, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(UNPACK_SEQUENCE);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
            (void)seq;
        }

        window op(_UNPACK_SEQUENCE, (seq -- unused[oparg], top[0])) {
            PyObject *seq_o = PyValue_AsKnownObjectSteal(seq);
            int res = _PyEval_UnpackIterableValue(tstate, seq_o, oparg, -1, top);
            ERROR_IF(res == 0);
        }

        macro(UNPACK_SEQUENCE) = unused/1 + _SPECIALIZE_UNPACK_SEQUENCE + _UNPACK_SEQUENCE;

        macro(UNPACK_SEQUENCE_TWO_TUPLE) =
            _GUARD_TOS_TUPLE + unused/1 + _UNPACK_SEQUENCE_TWO_TUPLE;

        op(_UNPACK_SEQUENCE_TWO_TUPLE, (seq -- val1, val0)) {
            assert(oparg == 2);
            PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
            assert(PyTuple_CheckExact(seq_o));
            EXIT_IF(PyTuple_GET_SIZE(seq_o) != 2);
            STAT_INC(UNPACK_SEQUENCE, hit);
            val0 = PyValue_XDUP(PyTuple_GET_VALUE(seq_o, 0));
            val1 = PyValue_XDUP(PyTuple_GET_VALUE(seq_o, 1));
            PyValue_CLOSE(seq);
        }

        macro(UNPACK_SEQUENCE_TUPLE) =
            _GUARD_TOS_TUPLE + unused/1 + _UNPACK_SEQUENCE_TUPLE;

        op(_UNPACK_SEQUENCE_TUPLE, (seq -- values[oparg])) {
            PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
            assert(PyTuple_CheckExact(seq_o));
            EXIT_IF(PyTuple_GET_SIZE(seq_o) != oparg);
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyValue *items = _PyTuple_ITEMS(seq_o);
            for (int i = 0; i < oparg; i++) {
                IR_WRITE_OUTPUT(values, i, PyValue_XDUP(items[oparg - 1 - i]));
            }
            DECREF_INPUTS();
        }

        macro(UNPACK_SEQUENCE_LIST) =
            _GUARD_TOS_LIST + unused/1 + _UNPACK_SEQUENCE_LIST;

        op(_UNPACK_SEQUENCE_LIST, (seq -- values[oparg])) {
            PyObject *seq_o = PyValue_AsKnownObjectBorrow(seq);
            assert(PyList_CheckExact(seq_o));
            DEOPT_IF(!LOCK_OBJECT(seq_o));
            if (PyList_GET_SIZE(seq_o) != oparg) {
                UNLOCK_OBJECT(seq_o);
                DEOPT_IF(true);
            }
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyValue *items = _PyList_ITEMS(seq_o);
            for (int i = 0; i < oparg; i++) {
                IR_WRITE_OUTPUT(values, i, PyValue_XDUP(items[oparg - 1 - i]));
            }
            UNLOCK_OBJECT(seq_o);
            DECREF_INPUTS();
        }

        window inst(UNPACK_EX, (seq -- unused[oparg & 0xFF], unused, unused[oparg >> 8], top[0])) {
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

        specializing op(_SPECIALIZE_STORE_ATTR, (v, owner -- v, owner)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                if (!PyValue_IsNull(v)) {
                    PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
                    next_instr = this_instr;
                    _Py_Specialize_StoreAttr(owner, next_instr, name);
                    DISPATCH_SAME_OPARG();
                }
            }
            OPCODE_DEFERRED_INC(STORE_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        inline_ok op(_STORE_ATTR, (v:nullable, owner --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyValue held_value = TAKE(v);
            PyValue receiver = TAKE(owner);
            INPUTS_DEAD();
            SYNC_SP();
            int err = _PyValue_SetAttr(receiver, name, held_value);
            value_operation_close3(receiver, held_value, PyValue_NULL);
            IR_FINISH_VALUE_OPERATION();
            ERROR_IF(err);
        }

        macro(STORE_ATTR) = unused/1 + _SPECIALIZE_STORE_ATTR + unused/3 + _STORE_ATTR;

        inst(STORE_GLOBAL, (v:nullable --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err;
            if (PyValue_IsNull(v)) {
                DEAD(v);
                err = PyDict_PopValue(GLOBALS(), PyValue_FromObjectBorrow(name), NULL);
                if (err == 0) {
                    err = -1;
                    _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                            NAME_ERROR_MSG, name);
                }
            }
            else {
                err = PyDict_SetValue(GLOBALS(), PyValue_FromObjectBorrow(name), v);
                PyValue_CLOSE(v);
            }
            ERROR_IF(err < 0);
        }

        inst(LOAD_LOCALS, ( -- locals)) {
            PyObject *l = LOCALS();
            if (l == NULL) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "no locals found");
                ERROR_IF(true);
            }
            locals = PyValue_DUP(PyValue_FromObjectBorrow(l));
        }

        inst(LOAD_FROM_DICT_OR_GLOBALS, (mod_or_class_dict -- v)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *namespace = PyValue_AsKnownObjectBorrow(mod_or_class_dict);
            // Taken before the lookup, which can run code
            PyValue held_namespace = TAKE(mod_or_class_dict);
            _PyStatusValue found = value_load_from_dict(tstate, namespace, name);
            PyValue_CLOSE(held_namespace);
            ERROR_IF(found.status < 0);
            if (found.status == 0) {
                v = value_load_global(GLOBALS(), BUILTINS(), name);
                ERROR_IF(PyValue_IsNull(v));
            }
            else {
                v = found.value;
            }
        }

        inst(LOAD_NAME, (-- v)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            v = _PyEval_LoadName(tstate, frame, name);
            ERROR_IF(PyValue_IsNull(v));
        }

        family(LOAD_GLOBAL, INLINE_CACHE_ENTRIES_LOAD_GLOBAL) = {
            LOAD_GLOBAL_MODULE,
            LOAD_GLOBAL_BUILTIN,
        };

        specializing op(_SPECIALIZE_LOAD_GLOBAL, ( -- )) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_GLOBAL);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // Publish the result before operation cleanup can reenter.
        op(_LOAD_GLOBAL, ( -- res[1])) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            IR_WRITE_OUTPUT(res, 0, value_load_global(GLOBALS(), BUILTINS(), name));
            IR_FINISH_VALUE_OPERATION();
            ERROR_IF(PyValue_IsNull(res[0]));
        }

        op(_PUSH_NULL_CONDITIONAL, ( -- null[oparg & 1])) {
            if (oparg & 1) {
                IR_WRITE_OUTPUT(null, 0, PyValue_NULL);
            }
        }

        // The NULL of a call's window needs no write: the slot is NULL by the
        // NULL invariant, and the assembler clears the bit in the word
        // (register-ir.md G4).  The stack model keeps the output.
        op(_NULL_SLOT_CONDITIONAL, ( -- unused[oparg & 1])) {
        }

        macro(LOAD_GLOBAL) =
            unused/1 +
            _SPECIALIZE_LOAD_GLOBAL +
            unused/3 +
            _LOAD_GLOBAL +
            _NULL_SLOT_CONDITIONAL;

        op(_GUARD_GLOBALS_VERSION, (globals_version/1 --)) {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != globals_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
        }

        op(_LOAD_GLOBAL_MODULE, (globals_version/1, unused/1, index/1 -- res))
        {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != globals_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            DEOPT_IF(index >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries));
            PyValueBorrow found = PyValue_Borrow(entries[index].me_value);
            DEOPT_IF(PyValue_IsNull(found));
            res = PyValue_XDUP(found);
            STAT_INC(LOAD_GLOBAL, hit);
        }

        op(_LOAD_GLOBAL_BUILTINS, (builtins_version/1, index/1 -- res))
        {
            PyDictObject *dict = (PyDictObject *)BUILTINS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != builtins_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            DEOPT_IF(index >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries));
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

        inst(DELETE_FAST, (--)) {
            PyValueBorrow v = PyValue_Borrow(GETLOCAL(oparg));
            if (PyValue_IsNull(v)) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
                    UNBOUNDLOCAL_ERROR_MSG,
                    _PyTuple_GET_OBJECT(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
                );
                ERROR_IF(true);
            }
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            _PyFrame_WriteHome(frame, &GETLOCAL(oparg), PyValue_NULL);
            PyValue_XCLOSE(tmp);
        }

        inst(MAKE_CELL, (--)) {
            // "initial" is probably NULL but not if it's an arg (or set
            // via the f_locals proxy before MAKE_CELL has run).
            PyObject *cell = PyCell_NewValue(GETLOCAL(oparg));
            if (cell == NULL) {
                ERROR_NO_POP();
            }
            PyValue tmp = GETLOCAL(oparg);  // a move: replaced below
            _PyFrame_WriteHome(frame, &GETLOCAL(oparg), PyValue_FromObjectSteal(cell));
            PyValue_XCLOSE(tmp);
        }

        inst(DELETE_DEREF, (--)) {
            PyObject *cell = PyValue_AsKnownObjectBorrow(GETLOCAL(oparg));
            // Can't use ERROR_IF here.
            // Fortunately we don't need its superpower.
            PyValue old = _PyCell_SwapValue((PyCellObject *)cell, PyValue_NULL);
            if (PyValue_IsNull(old)) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_NO_POP();
            }
            PyValue_CLOSE(old);
        }

        inst(LOAD_FROM_DICT_OR_DEREF, (class_dict_st -- value)) {
            PyObject *name;
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
                PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(GETLOCAL(oparg));
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

        inst(LOAD_DEREF, ( -- value)) {
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(GETLOCAL(oparg));
            value = _PyCell_GetValue(cell);
            if (PyValue_IsNull(value)) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_IF(true);
            }
        }

        inst(STORE_DEREF, (v --)) {
            PyCellObject *cell = (PyCellObject *)PyValue_AsKnownObjectBorrow(GETLOCAL(oparg));
            PyValue owned = v;
            DEAD(v);
            _PyCell_SetValue(cell, owned);
        }

        inst(COPY_FREE_VARS, (--)) {
            /* Copy closure variables to free variables */
            PyCodeObject *co = _PyFrame_GetCode(frame);
            PyFunctionObject *func = _PyFrame_GetFunction(frame);
            PyObject *closure = func->func_closure;
            assert(oparg == co->co_nfreevars);
            int offset = co->co_nlocalsplus - oparg;
            for (int i = 0; i < oparg; ++i) {
                _PyFrame_WriteHome(frame, frame->base + offset + i,
                                   PyValue_XDUP(PyTuple_GET_VALUE(closure, i)));
            }
        }

        inst(BUILD_STRING, (pieces[oparg] -- str)) {
            PyObject *str_o = IR_BUILD_STRING(pieces);
            DEAD(pieces);
            ERROR_IF(str_o == NULL);
            str = PyValue_FromObjectSteal(str_o);
        }

        inst(BUILD_INTERPOLATION, (value, str, format[oparg & 1] -- interpolation)) {
            PyObject *value_o = PyValue_AsKnownObjectBorrow(value);
            PyObject *str_o = PyValue_AsKnownObjectBorrow(str);
            int conversion = oparg >> 2;
            PyObject *format_o;
            if (oparg & 1) {
                format_o = PyValue_AsKnownObjectBorrow(format[0]);
            }
            else {
                format_o = &_Py_STR(empty);
            }
            PyObject *interpolation_o = _PyInterpolation_Build(value_o, str_o, conversion, format_o);
            // The handler clears the slots of the array before it closes
            // the element, so the element moves to a C variable first
            PyValue format_ref = (oparg & 1) ? format[0] : PyValue_NULL;
            DEAD(format);
            PyValue_XCLOSE(format_ref);
            PyValue_CLOSE(str);
            PyValue_CLOSE(value);
            ERROR_IF(interpolation_o == NULL);
            interpolation = PyValue_FromObjectSteal(interpolation_o);
        }

        inst(BUILD_TEMPLATE, (strings, interpolations -- template)) {
            PyObject *strings_o = PyValue_AsKnownObjectBorrow(strings);
            PyObject *interpolations_o = PyValue_AsKnownObjectBorrow(interpolations);
            PyObject *template_o = _PyTemplate_Build(strings_o, interpolations_o);
            PyValue_CLOSE(interpolations);
            PyValue_CLOSE(strings);
            ERROR_IF(template_o == NULL);
            template = PyValue_FromObjectSteal(template_o);
        }

        inst(BUILD_TUPLE, (values[oparg] -- tup)) {
            PyObject *tup_o = IR_BUILD_TUPLE(values);
            if (tup_o == NULL) {
                ERROR_NO_POP();
            }
            INPUTS_DEAD();
            tup = PyValue_FromObjectSteal(tup_o);
        }

        inst(BUILD_LIST, (values[oparg] -- list)) {
            PyObject *list_o = IR_BUILD_LIST(values);
            if (list_o == NULL) {
                ERROR_NO_POP();
            }
            INPUTS_DEAD();
            list = PyValue_FromObjectSteal(list_o);
        }

        op(_LIST_EXTEND, (list_st, unused[oparg-1], iterable_st -- list_st, unused[oparg-1], i)) {
            PyObject *list = PyValue_AsKnownObjectBorrow(list_st);
            PyObject *iterable = PyValue_AsKnownObjectBorrow(iterable_st);

            PyObject *none_val = _PyList_Extend((PyListObject *)list, iterable);
            if (none_val == NULL) {
                int matches = _PyErr_ExceptionMatches(tstate, PyExc_TypeError);
                if (matches &&
                   (Py_TYPE(iterable)->tp_iter == NULL && !_PyValue_SequenceCheck(PyValue_FromObjectBorrow(iterable))))
                {
                    _PyErr_Clear(tstate);
                    _PyErr_Format(tstate, PyExc_TypeError,
                          "Value after * must be an iterable, not %.200s",
                          Py_TYPE(iterable)->tp_name);
                }
                ERROR_NO_POP();
            }
            assert(Py_IsNone(none_val));
            i = iterable_st;
            DEAD(iterable_st);
        }

        macro(LIST_EXTEND) = _LIST_EXTEND + POP_TOP;

        op(_SET_UPDATE, (set, unused[oparg-1], iterable -- set, unused[oparg-1], i)) {
            int err = _PySet_Update(PyValue_AsKnownObjectBorrow(set),
                                    PyValue_AsKnownObjectBorrow(iterable));
            if (err < 0) {
                ERROR_NO_POP();
            }
            i = iterable;
            DEAD(iterable);
        }

        macro(SET_UPDATE) = _SET_UPDATE + POP_TOP;

        inst(BUILD_SET, (values[oparg] -- set)) {
            PyObject *set_o = PySet_New(NULL);
            if (set_o == NULL) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }

            int err = 0;
            for (Py_ssize_t i = 0; i < oparg; i++) {
                PyValue value = IR_TAKE_INPUT(values, i);
                if (err == 0) {
                    err = _PySet_AddValueTakeRef((PySetObject *)set_o,
                                                 value);
                }
                else {
                    PyValue_CLOSE(value);
                }
            }
            DEAD(values);
            if (err) {
                Py_DECREF(set_o);
                ERROR_IF(true);
            }

            INPUTS_DEAD();
            set = PyValue_FromObjectSteal(set_o);
        }

        inline_ok inst(BUILD_MAP, (values[oparg*2] -- map)) {
            PyObject *map_o = IR_BUILD_MAP(values);
            DEAD(values);
            int failed = map_o == NULL;
            map = failed ? PyValue_NULL : PyValue_FromObjectSteal(map_o);
            SYNC_SP();
            IR_FINISH_VALUE_OPERATION();
            ERROR_IF(failed);
        }

        inst(SETUP_ANNOTATIONS, (--)) {
            if (LOCALS() == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                ERROR_IF(true);
            }
            /* check if __annotations__ in locals()... */
            _PyStatusValue found = _PyValue_GetOptionalItem(
                PyValue_FromObjectBorrow(LOCALS()),
                PyValue_FromObjectBorrow(&_Py_ID(__annotations__)));
            ERROR_IF(found.status < 0);
            int err;
            if (found.status == 0) {
                PyObject *ann_dict;
                ann_dict = PyDict_New();
                ERROR_IF(ann_dict == NULL);
                err = _PyValue_SetItem(PyValue_FromObjectBorrow(LOCALS()),
                                       PyValue_FromObjectBorrow(&_Py_ID(__annotations__)),
                                       PyValue_FromObjectBorrow(ann_dict));
                Py_DECREF(ann_dict);
                ERROR_IF(err);
            }
            else {
                PyValue_CLOSE(found.value);
            }
        }

        pseudo(ANNOTATIONS_PLACEHOLDER, (--)) = {
            NOP,
        };

        op(_DICT_UPDATE, (dict, unused[oparg - 1], update -- dict, unused[oparg - 1], upd)) {
            PyObject *dict_o = PyValue_AsKnownObjectBorrow(dict);
            PyObject *update_o = PyValue_AsKnownObjectBorrow(update);

            int err = PyDict_Update(dict_o, update_o);
            if (err < 0) {
                int matches = _PyErr_ExceptionMatches(tstate, PyExc_AttributeError);
                if (matches) {
                    PyObject *exc = _PyErr_GetRaisedException(tstate);
                    int has_keys = PyObject_HasAttrWithError(update_o, &_Py_ID(keys));
                    if (has_keys == 0) {
                        _PyErr_Format(tstate, PyExc_TypeError,
                                        "'%T' object is not a mapping",
                                        update_o);
                        Py_DECREF(exc);
                    }
                    else {
                        _PyErr_ChainExceptions1(exc);
                    }
                }
                ERROR_NO_POP();
            }
            upd = update;
            DEAD(update);
        }

        macro(DICT_UPDATE) = _DICT_UPDATE + POP_TOP;

        op(_DICT_MERGE, (callable, unused, unused, dict, unused[oparg - 1], update -- callable, unused, unused, dict, unused[oparg - 1], u)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyObject *dict_o = PyValue_AsKnownObjectBorrow(dict);
            PyObject *update_o = PyValue_AsKnownObjectBorrow(update);

            _PyStatusObject merged = value_dict_merge_uniq(dict_o, update_o);
            if (merged.status < 0) {
                _PyEval_FormatKwargsError(tstate, callable_o, update_o, merged.object);
                Py_XDECREF(merged.object);
                ERROR_NO_POP();
            }
            u = update;
            DEAD(update);
        }

        macro(DICT_MERGE) = _DICT_MERGE + POP_TOP;

        inline_ok inst(MAP_ADD, (dict_st, unused[oparg - 1], key, value -- dict_st, unused[oparg - 1])) {
            PyObject *dict = PyValue_AsKnownObjectBorrow(dict_st);
            assert(PyDict_CheckExact(dict));
            /* dict[key] = value */
            // Do not DECREF INPUTS because the function steals the references
            PyValue stored_key = TAKE(key);
            PyValue stored_value = TAKE(value);
            int err = _PyDict_SetValue_Take2(
                (PyDictObject *)dict, stored_key, stored_value);
            ERROR_IF(err != 0);
        }

        macro(INSTRUMENTED_LOAD_SUPER_ATTR) =
            unused/1 +
            _LOAD_SUPER_ATTR +
            _PUSH_NULL_CONDITIONAL;

        family(LOAD_SUPER_ATTR, INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR) = {
            LOAD_SUPER_ATTR_ATTR,
            LOAD_SUPER_ATTR_METHOD,
        };

        specializing op(_SPECIALIZE_LOAD_SUPER_ATTR, (global_super_st, class_st, unused -- global_super_st, class_st, unused)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            int load_method = oparg & 1;
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_LoadSuperAttr(global_super_st, class_st, next_instr, load_method);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_SUPER_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        tier1 op(_LOAD_SUPER_ATTR, (global_super_st, class_st, self_st -- attr)) {
            PyObject *global_super = PyValue_AsKnownObjectBorrow(global_super_st);
            PyObject *class = PyValue_AsKnownObjectBorrow(class_st);

            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                PyObject *arg = oparg & 2 ? class : &_PyInstrumentation_MISSING;
                int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_CALL,
                        frame, this_instr, PyValue_FromObjectBorrow(global_super),
                        PyValue_FromObjectBorrow(arg));
                if (err) {
                    DECREF_INPUTS();
                    ERROR_IF(true);
                }
            }
            // we make no attempt to optimize here; specializations should
            // handle any case whose performance we care about
            PyValue super = super_call(global_super, class, self_st, oparg & 2);
            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                PyObject *arg = oparg & 2 ? class : &_PyInstrumentation_MISSING;
                if (PyValue_IsNull(super)) {
                    _Py_call_instrumentation_exc2(
                        tstate, PY_MONITORING_EVENT_C_RAISE,
                        frame, this_instr, PyValue_FromObjectBorrow(global_super),
                        PyValue_FromObjectBorrow(arg));
                }
                else {
                    int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_C_RETURN,
                        frame, this_instr, PyValue_FromObjectBorrow(global_super),
                        PyValue_FromObjectBorrow(arg));
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
            _PUSH_NULL_CONDITIONAL;

        inline_ok inst(LOAD_SUPER_ATTR_ATTR, (unused/1, global_super_st, class_st, self_st -- attr_st)) {
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

        op(_GUARD_LOAD_SUPER_ATTR_METHOD, (global_super_st, class_st, unused -- global_super_st, class_st, unused)) {
            PyObject *global_super = PyValue_AsKnownObjectBorrow(global_super_st);
            PyObject *class = PyValue_AsKnownObjectBorrow(class_st);
            assert(oparg & 1);
            EXIT_IF(global_super != (PyObject *)&PySuper_Type);
            EXIT_IF(!PyType_Check(class));
        }

        op(_LOAD_SUPER_ATTR_METHOD, (global_super_st, class_st, self_st -- attr, self_or_null)) {
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

        family(LOAD_ATTR, INLINE_CACHE_ENTRIES_LOAD_ATTR) = {
            LOAD_ATTR_INSTANCE_VALUE,
            LOAD_ATTR_MODULE,
            LOAD_ATTR_WITH_HINT,
            LOAD_ATTR_SLOT,
            LOAD_ATTR_CLASS,
            LOAD_ATTR_CLASS_WITH_METACLASS_CHECK,
            LOAD_ATTR_CLASSMETHOD,
            LOAD_ATTR_PROPERTY,
            LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN,
            LOAD_ATTR_METHOD_WITH_VALUES,
            LOAD_ATTR_METHOD_NO_DICT,
            LOAD_ATTR_METHOD_LAZY_DICT,
            LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES,
            LOAD_ATTR_NONDESCRIPTOR_NO_DICT,
            LOAD_ATTR_SHAPE,
        };

        specializing op(_SPECIALIZE_LOAD_ATTR, (owner -- owner)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        inline_ok op(_LOAD_ATTR, (owner -- attr, unused[oparg&1])) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);
            int method = oparg & 1;
            PyValue receiver = TAKE(owner);
            INPUTS_DEAD();
            SYNC_SP();
            PyValue pending = PyValue_NULL;
            PyValue pending_self = PyValue_NULL;
            if (method) {
                _PyStatusValue found = _PyValue_GetMethod(receiver, name);
                pending = found.value;
                if (found.status > 0) {
                    pending_self = PyValue_DUP(receiver);
                }
            }
            else {
                pending = _PyValue_GetAttr(receiver, name);
            }
            if (PyValue_IsNull(pending)) {
                value_operation_close3(receiver, PyValue_NULL, PyValue_NULL);
                IR_ABORT_VALUE_READER();
            }
            IR_COMPLETE_ATTR_READER(pending, pending_self, receiver, method);
            attr = pending;
        }

        macro(LOAD_ATTR) =
            unused/1 + _SPECIALIZE_LOAD_ATTR +
            unused/8 +
            _LOAD_ATTR;

        op(_GUARD_TYPE_VERSION, (type_version/2, owner -- owner)) {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(owner));
            assert(type_version != 0);
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version);
        }

        op(_GUARD_TYPE_VERSION_LOCKED, (type_version/2, owner -- owner)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(type_version != 0);
            PyTypeObject *tp = Py_TYPE(owner_o);
            if (FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version) {
                UNLOCK_OBJECT(owner_o);
                EXIT_IF(true);
            }
        }

        op(_CHECK_MANAGED_OBJECT_HAS_VALUES, (owner -- owner)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            EXIT_IF(!FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid));
        }

        op(_LOAD_ATTR_INSTANCE_VALUE, (offset/1, owner -- attr)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValue *value_ptr = (PyValue *)(((char *)owner_o) + offset);
            PyValueBorrow found = PyValue_Borrow(*value_ptr);
            DEOPT_IF(PyValue_IsNull(found));
            attr = PyValue_XDUP(found);
            STAT_INC(LOAD_ATTR, hit);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_INSTANCE_VALUE) =
            unused/1 + // Skip over the entry distance
            _GUARD_TYPE_VERSION +
            _CHECK_MANAGED_OBJECT_HAS_VALUES +
            _LOAD_ATTR_INSTANCE_VALUE +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        /* The shape handlers (Py_SHAPES, see pycore_dict.h).  They follow a
         * type version guard, which proves that the owner has inline values
         * to read the shape from and lets descriptors added to the class
         * win.  The shape fields take no inline cache units.  Shapes exist
         * only in builds with the GIL, so the stores take no object lock and
         * need not check for a dict: an instance with a dict has no shape. */
        op(_GUARD_SHAPE, (shape/0, owner -- owner)) {
            EXIT_IF(_PyObject_Shape(PyValue_AsKnownObjectBorrow(owner)) != shape);
        }

        op(_LOAD_ATTR_SHAPE, (offset/1, owner -- attr)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValueBorrow found =
                PyValue_Borrow(*(PyValue *)((char *)owner_o + offset));
            // The shape proves the slot filled.
            assert(!PyValue_IsNull(found));
            attr = IR_CANONICAL_HOME_DUP(found);
            STAT_INC(LOAD_ATTR, hit);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_SHAPE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_SHAPE +
            _LOAD_ATTR_SHAPE +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        inline_ok op(_LOAD_ATTR_MODULE, (dict_version/2, index/1, owner -- attr)) {
            EXIT_IF(!PyValue_IsObject(owner));
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            EXIT_IF(!PyModule_CheckExact(owner_o));
            PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner_o)->md_dict;
            assert(dict != NULL);
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            EXIT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != dict_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
            EXIT_IF(index >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries));
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(keys) + index;
            PyValueBorrow found = PyValue_Borrow(ep->me_value);
            EXIT_IF(PyValue_IsNull(found));
            attr = PyValue_XDUP(found);
            STAT_INC(LOAD_ATTR, hit);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_MODULE) =
            unused/1 +
            _LOAD_ATTR_MODULE +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            EXIT_IF(dict == NULL);
            PyDictKeysObject *dk = FT_ATOMIC_LOAD_PTR(dict->ma_keys);
            assert(PyDict_CheckExact((PyObject *)dict));
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_Py_IsOwnedByCurrentThread((PyObject *)dict) && !_PyObject_GC_IS_SHARED(dict));
#endif
            PyValue found;
            if (hint >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_nentries)) {
                EXIT_IF(true);
            }

            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            if (dk->dk_kind != DICT_KEYS_UNICODE) {
                EXIT_IF(true);
            }
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dk) + hint;
            if (FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key) != name) {
                EXIT_IF(true);
            }
            found = ep->me_value;
            if (PyValue_IsNull(found)) {
                EXIT_IF(true);
            }
            STAT_INC(LOAD_ATTR, hit);
            attr = PyValue_XDUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_WITH_HINT +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_LOAD_ATTR_SLOT, (index/1, owner -- attr)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            PyValueBorrow value =
                PyValue_Borrow(*(PyValue *)((char *)owner_o + index));
            EXIT_IF(PyValue_IsNull(value));
            _PyValue_RecheckHome(value);
            attr = PyValue_DUP(value);
            PyValue receiver = OWNER(owner);
            STAT_INC(LOAD_ATTR, hit);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_SLOT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_SLOT +  // NOTE: This action may also deopt
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_CHECK_ATTR_CLASS, (class_version/2, owner -- owner)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            EXIT_IF(!PyType_Check(owner_o));
            assert(class_version != 0);
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(((PyTypeObject *)owner_o)->tp_version_tag) != class_version);
        }

        op(_LOAD_ATTR_CLASS, (class_value/4, owner -- attr)) {
            STAT_INC(LOAD_ATTR, hit);
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            assert(!PyValue_IsNull(found));
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_CLASS) =
            unused/1 +
            _CHECK_ATTR_CLASS +
            unused/2 +
            _LOAD_ATTR_CLASS +
            _PUSH_NULL_CONDITIONAL;

        macro(LOAD_ATTR_CLASS_WITH_METACLASS_CHECK) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_ATTR_CLASS +
            _LOAD_ATTR_CLASS +
            _PUSH_NULL_CONDITIONAL;

        op(_GUARD_BUILTIN_METACLASS, (owner -- owner)) {
            #ifdef Py_GIL_DISABLED
            const bool gil_disabled = true;
            #else
            const bool gil_disabled = false;
            #endif
            EXIT_IF(gil_disabled);
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            EXIT_IF(Py_TYPE(owner_o) != &PyType_Type);
        }

        op(_LOAD_ATTR_CLASSMETHOD, (class_value/4, owner -- attr, self)) {
            assert(oparg & 1);
            PyValueBorrow descriptor = _PyValueGuarded_Load(class_value);
            assert(PyValue_IsObject(descriptor));
            PyObject *cm = PyValue_AsKnownObjectBorrow(descriptor);
            EXIT_IF(!Py_IS_TYPE(cm, &PyClassMethod_Type));
            // __init__ can replace the wrapped callable without invalidating
            // the class version. Capture it before explicit argument evaluation.
            PyObject *target = _PyClassMethod_GetFunc(cm);
            EXIT_IF(target == NULL || !Py_IS_TYPE(target, &PyFunction_Type));
            attr = PyValue_DUP(PyValue_FromObjectBorrow(target));
            self = TAKE(owner);
            STAT_INC(LOAD_ATTR, hit);
        }

        macro(LOAD_ATTR_CLASSMETHOD) =
            unused/1 +
            _GUARD_BUILTIN_METACLASS +
            _CHECK_ATTR_CLASS +
            unused/2 +
            _LOAD_ATTR_CLASSMETHOD;

        op(_LOAD_ATTR_PROPERTY_FRAME, (func_version/2, fget/4, owner -- new_frame)) {
            assert((oparg & 1) == 0);
            assert(Py_IS_TYPE(fget, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)fget;
            EXIT_IF(f->func_version != func_version);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            EXIT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            STAT_INC(LOAD_ATTR, hit);
            _PyInterpreterFrame *pushed_frame = _PyFrame_PushUnchecked(tstate, PyValue_DUP(PyValue_FromObjectBorrow(fget)), frame);
            _PyFrame_WriteHome(pushed_frame, pushed_frame->base, TAKE(owner));
            new_frame = PyValue_Wrap(pushed_frame);
        }

        macro(LOAD_ATTR_PROPERTY) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_PEP_523 +
            _LOAD_ATTR_PROPERTY_FRAME +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_FRAME, (func_version/2, getattribute/4, owner -- new_frame)) {
            assert((oparg & 1) == 0);
            assert(Py_IS_TYPE(getattribute, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)getattribute;
            assert(func_version != 0);
            EXIT_IF(f->func_version != func_version);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            assert(code->co_argcount == 2);
            EXIT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            STAT_INC(LOAD_ATTR, hit);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);
            _PyInterpreterFrame *pushed_frame = _PyFrame_PushUnchecked(
                tstate, PyValue_DUP(PyValue_FromObjectBorrow(f)), frame);
            _PyFrame_WriteHome(pushed_frame, pushed_frame->base, TAKE(owner));
            _PyFrame_WriteHome(pushed_frame, pushed_frame->base + 1,
                               PyValue_DUP(PyValue_FromObjectBorrow(name)));
            new_frame = PyValue_Wrap(pushed_frame);
        }

        macro(LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_PEP_523 +
            _LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_FRAME +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_GUARD_DORV_NO_DICT, (owner -- owner)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            if (_PyObject_GetManagedDict(owner_o) ||
                    !FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid)) {
                UNLOCK_OBJECT(owner_o);
                EXIT_IF(true);
            }
        }

        inline_ok op(_STORE_ATTR_INSTANCE_VALUE, (offset/1, value, owner --)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            assert(_PyObject_GetManagedDict(owner_o) == NULL);
            PyValue *value_ptr = (PyValue *)(((char *)owner_o) + offset);
            PyValue old_value = *value_ptr;  // a move: replaced below
#ifdef Py_SHAPES
            /* This store does not grow the type's shapes.  An addition whose
             * transition is missing goes to STORE_ATTR, which creates it, so
             * that the instance keeps a shape. */
            if (PyValue_IsNull(old_value)) {
                PyDictValues *values = _PyObject_InlineValues(owner_o);
                if (_PyShape_TransitionMissing(Py_TYPE(owner_o), values->shape,
                                               value_ptr - values->values)) {
                    UNLOCK_OBJECT(owner_o);
                    EXIT_IF(true);
                }
            }
#endif
            STAT_INC(STORE_ATTR, hit);
            *value_ptr = TAKE(value);
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
            PyValue receiver = OWNER(owner);
            SYNC_SP();
            IR_RETIRE_PUBLISHED_PAIR(old_value, receiver);
        }

        op(_LOCK_OBJECT, (value -- value)) {
            DEOPT_IF(!LOCK_OBJECT(PyValue_AsKnownObjectBorrow(value)));
        }

        macro(STORE_ATTR_INSTANCE_VALUE) =
            unused/1 +
            _LOCK_OBJECT +
            _GUARD_TYPE_VERSION_LOCKED +
            _GUARD_DORV_NO_DICT +
            _STORE_ATTR_INSTANCE_VALUE;

        inline_ok op(_STORE_ATTR_SHAPE, (offset/1, value, owner --)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValue *value_ptr = (PyValue *)((char *)owner_o + offset);
            PyValue old_value = *value_ptr;  // a move: replaced below
            assert(!PyValue_IsNull(old_value));
            *value_ptr = TAKE(value);
            STAT_INC(STORE_ATTR, hit);
            PyValue receiver = OWNER(owner);
            SYNC_SP();
            IR_RETIRE_PUBLISHED_PAIR(old_value, receiver);
        }

        macro(STORE_ATTR_SHAPE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_SHAPE +
            _STORE_ATTR_SHAPE;

        inline_ok op(_STORE_ATTR_SHAPE_ADD, (shape_to/0, offset/1, value, owner --)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            PyValue *value_ptr = (PyValue *)((char *)owner_o + offset);
            assert(PyValue_IsNull(*value_ptr));
            *value_ptr = TAKE(value);
            PyDictValues *values = _PyObject_InlineValues(owner_o);
            _PyDictValues_AddToInsertionOrder(values, value_ptr - values->values);
            _PyDictValues_SetShape(values, (uint8_t)shape_to);
            STAT_INC(STORE_ATTR, hit);
            PyValue receiver = OWNER(owner);
            SYNC_SP();
            IR_RETIRE_PUBLISHED(receiver);
        }

        macro(STORE_ATTR_SHAPE_ADD) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_SHAPE +
            _STORE_ATTR_SHAPE_ADD;

        inline_ok op(_STORE_ATTR_WITH_HINT, (hint/1, value, owner --)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            DEOPT_IF(dict == NULL);
            DEOPT_IF(!LOCK_OBJECT(dict));
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyDictKeysObject *dk = dict->ma_keys;
            // Split keys are shared with other dicts, which append to them
            // under the keys lock rather than this dict's lock.
            if (hint >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_nentries) ||
                    dk->dk_kind == DICT_KEYS_GENERAL) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dk) + hint;
            if (FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key) != name) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            // A split table keeps its values, such as an object's inline
            // values, apart from its keys.
            PyValue *value_ptr = dict->ma_values == NULL ?
                &ep->me_value : &dict->ma_values->values[hint];
            PyValue old_value = *value_ptr;  // a move: replaced below
            if (PyValue_IsNull(old_value)) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            if (dict->_ma_watcher_tag & DICT_WATCHER_MASK) {
                /* A watcher error's unraisable hook may reenter. Do not
                 * retain a raw slot pointer across the provider call. */
                PyValue watched_dict = PyValue_DUP(PyValue_FromObjectBorrow((PyObject *)dict));
                PyValue release_value = TAKE(value);
                PyValue receiver = TAKE(owner);
                UNLOCK_OBJECT(dict);
                INPUTS_DEAD();
                SYNC_SP();
                int err = PyDict_SetValue((PyObject *)dict,
                    PyValue_FromObjectBorrow(name), release_value);
                if (err < 0) {
                    /* ERROR_NO_POP kept these temporary owners until the
                     * ordinary full-frame unwind, including deeper temps. */
                    IR_RESTORE_VALUE_INPUTS(release_value, receiver);
                    value_operation_close3(watched_dict, release_value, receiver);
                }
                else {
                    value_operation_close3(release_value, watched_dict, receiver);
                }
                IR_FINISH_VALUE_OPERATION();
                ERROR_IF(err < 0);
            }
            else {
                *value_ptr = TAKE(value);
                PyValue receiver = OWNER(owner);
                UNLOCK_OBJECT(dict);
                INPUTS_DEAD();
                SYNC_SP();
                IR_RETIRE_PUBLISHED_PAIR(old_value, receiver);
            }
            STAT_INC(STORE_ATTR, hit);
        }

        macro(STORE_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _STORE_ATTR_WITH_HINT;

        op(_STORE_ATTR_SLOT, (index/1, value, owner --)) {
            PyObject *owner_o = PyValue_AsKnownObjectBorrow(owner);

            DEOPT_IF(!LOCK_OBJECT(owner_o));
            char *addr = (char *)owner_o + index;
            STAT_INC(STORE_ATTR, hit);
            PyValue old_value = *(PyValue *)addr;  // a move: replaced below
            *(PyValue *)addr = TAKE(value);
            PyValue receiver = OWNER(owner);
            UNLOCK_OBJECT(owner_o);
            INPUTS_DEAD();
            SYNC_SP();
            IR_RETIRE_PUBLISHED_PAIR(old_value, receiver);
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

        specializing op(_SPECIALIZE_COMPARE_OP, (left, right -- left, right)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_CompareOp(left, right, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(COMPARE_OP);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_COMPARE_OP, (left, right -- res)) {
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

        inline_ok op(_COMPARE_OP_FLOAT, (left, right -- res, l, r)) {
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
        inline_ok op(_COMPARE_OP_INT, (left, right -- res, l, r)) {
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
        op(_COMPARE_OP_STR, (left, right -- res, l, r)) {
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

        macro(IS_OP) = _IS_OP + POP_TOP + POP_TOP;

        // A bit compare: the operands are canonical (see "Value identity"
        // in pycore_value.h).
        inline_ok op(_IS_OP, (left, right -- b, l, r)) {
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

        op(_CONTAINS_OP, (left, right -- b, l, r)) {
            int res = _PyValue_Contains(right, left);
            if (res < 0) {
                ERROR_NO_POP();
            }
            b = (res ^ oparg) ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        specializing op(_SPECIALIZE_CONTAINS_OP, (left, right -- left, right)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_ContainsOp(left, right, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CONTAINS_OP);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CONTAINS_OP) = unused/1 + _SPECIALIZE_CONTAINS_OP + _CONTAINS_OP + POP_TOP + POP_TOP;

        op(_GUARD_TOS_ANY_SET, (tos -- tos)) {
            PyObject *o = PyValue_AsKnownObjectBorrow(tos);
            EXIT_IF(!PyAnySet_CheckExact(o));
        }

        macro(CONTAINS_OP_SET) = _GUARD_TOS_ANY_SET + unused/1 + _CONTAINS_OP_SET + POP_TOP + POP_TOP;

        // inline_ok: left is handled by value if inline, so only a pointer
        // is ever read as an object and nothing is boxed.
        inline_ok op(_CONTAINS_OP_SET, (left, right -- b, l, r)) {
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);

            assert(PyAnySet_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            // Note: both set and frozenset use the same seq_contains method!
            int res;
            if (PyValue_IsInline(left)) {
                // An inline int or float is hashable and never a set, so
                // _PySet_Contains()'s frozenset fallback cannot apply.
                res = PySet_ContainsValue(right_o, left);
            }
            else {
                res = _PySet_Contains((PySetObject *)right_o,
                                      PyValue_AsKnownObjectBorrow(left));
            }
            if (res < 0) {
                ERROR_NO_POP();
            }
            b = (res ^ oparg) ? PyValue_True : PyValue_False;
            l = left;
            r = right;
            INPUTS_DEAD();
        }

        macro(CONTAINS_OP_DICT) = _GUARD_TOS_ANY_DICT + unused/1 + _CONTAINS_OP_DICT + POP_TOP + POP_TOP;

        op(_CONTAINS_OP_DICT, (left, right -- b, l, r)) {
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);

            assert(PyAnyDict_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            // The key is looked up as a value: an inline key needs no box.
            int res = PyDict_ContainsValue(right_o, left);
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
         * of the compare) and IR_FUSED_IF_FALSE.  The history of the jump is
         * in the entry's taken half-word.  There is no backward form, so the
         * handlers never poll the eval breaker.  The handlers stand for the
         * compare (their shadow, see ir_metadata_generator.py), and the
         * specialisations are those of the compare, in families of their own. */
        ir_only op(_JUMP_IF_COND, (cond -- )) {
            assert(PyValue_BoolCheck(cond));
            int flag = PyValue_IsTrue(cond) ^ IR_FUSED_IF_FALSE();
            DEAD(cond);
            RECORD_BRANCH_TAKEN(FB_TAKEN(), flag);
            if (flag) {
                JUMPBY(IR_FUSED_DISTANCE());
            }
        }

        ir_only op(_INSTRUMENTED_JUMP_IF_COND, (cond -- )) {
            assert(PyValue_BoolCheck(cond));
            int flag = PyValue_IsTrue(cond) ^ IR_FUSED_IF_FALSE();
            DEAD(cond);
            RECORD_BRANCH_TAKEN(FB_TAKEN(), flag);
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

        family(CONTAINS_OP_JUMP, INLINE_CACHE_ENTRIES_CONTAINS_OP) = {
            CONTAINS_OP_JUMP_DICT,
            CONTAINS_OP_JUMP_SET,
        };

        macro(CONTAINS_OP_JUMP) =
            unused/1 + _SPECIALIZE_CONTAINS_OP + _CONTAINS_OP + POP_TOP + POP_TOP + _JUMP_IF_COND;

        macro(CONTAINS_OP_JUMP_SET) =
            _GUARD_TOS_ANY_SET + unused/1 + _CONTAINS_OP_SET + POP_TOP + POP_TOP + _JUMP_IF_COND;

        macro(CONTAINS_OP_JUMP_DICT) =
            _GUARD_TOS_ANY_DICT + unused/1 + _CONTAINS_OP_DICT + POP_TOP + POP_TOP + _JUMP_IF_COND;

        macro(INSTRUMENTED_CONTAINS_OP_JUMP) =
            unused/1 + _CONTAINS_OP + POP_TOP + POP_TOP + _INSTRUMENTED_JUMP_IF_COND;

        macro(IS_OP_JUMP) = _IS_OP + POP_TOP + POP_TOP + _JUMP_IF_COND;

        macro(INSTRUMENTED_IS_OP_JUMP) =
            _IS_OP + POP_TOP + POP_TOP + _INSTRUMENTED_JUMP_IF_COND;

        inst(CHECK_EG_MATCH, (exc_value_st, match_type_st -- rest, match)) {
            PyObject *exc_value = PyValue_AsKnownObjectBorrow(exc_value_st);
            PyObject *match_type = PyValue_AsKnownObjectBorrow(match_type_st);
            // Taken before the check, which can run code
            PyValue held_exc = TAKE(exc_value_st);
            PyValue held_type = TAKE(match_type_st);
            int err = _PyEval_CheckExceptStarTypeValid(tstate, match_type);
            if (err < 0) {
                PyValue_CLOSE(held_type);
                PyValue_CLOSE(held_exc);
                ERROR_IF(true);
            }

            _PyObjectPair matched = value_exception_group_match(frame, exc_value, match_type);
            PyObject *match_o = matched.match;
            PyObject *rest_o = matched.rest;
            PyValue_CLOSE(held_type);
            PyValue_CLOSE(held_exc);
            // A null match is an error, including a failed match (res < 0)
            ERROR_IF(match_o == NULL);

            if (!Py_IsNone(match_o)) {
                PyErr_SetHandledException(match_o);
            }
            rest = PyValue_FromObjectSteal(rest_o);
            match = PyValue_FromObjectSteal(match_o);
        }

        inst(CHECK_EXC_MATCH, (left, right -- left, b)) {
            PyObject *left_o = PyValue_AsKnownObjectBorrow(left);
            PyObject *right_o = PyValue_AsKnownObjectBorrow(right);
            // Taken before the check, which can run code
            PyValue held_right = TAKE(right);

            assert(PyExceptionInstance_Check(left_o));
            int err = _PyEval_CheckExceptTypeValid(tstate, right_o);
            if (err < 0) {
                PyValue_CLOSE(held_right);
                ERROR_NO_POP();
            }

            int res = PyErr_GivenExceptionMatches(left_o, right_o);
            PyValue_CLOSE(held_right);
            b = res ? PyValue_True : PyValue_False;
        }

        inst(IMPORT_NAME, (level, fromlist -- res)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            PyObject *res_o;
            // The level is an inline int: read it without a box
            int ilevel = value_import_level(level);
            if (ilevel == -1 && _PyErr_Occurred(tstate)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            if (!(oparg & 0x02)) {
                res_o = _PyEval_LazyImportName(tstate, BUILTINS(), GLOBALS(),
                                LOCALS(), name,
                                PyValue_AsKnownObjectBorrow(fromlist),
                                ilevel, oparg & 0x01);

            }
            else {
                res_o = _PyEval_ImportName(tstate, BUILTINS(), GLOBALS(),
                                LOCALS(), name,
                                PyValue_AsKnownObjectBorrow(fromlist),
                                ilevel);
            }
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyValue_FromObjectSteal(res_o);
        }

        inst(IMPORT_FROM, (from -- from, res)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            if (PyValue_IsObject(from) &&
                PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(from)))
            {
                PyObject *res_o = _PyEval_LazyImportFrom(
                    tstate, frame, PyValue_AsKnownObjectBorrow(from), name);
                ERROR_IF(res_o == NULL);
                res = PyValue_FromObjectSteal(res_o);
            }
            else {
                res = _PyEval_ImportFromValue(tstate, PyValue_Borrow(from), name);
                ERROR_IF(PyValue_IsNull(res));
            }
        }

        tier1 inst(JUMP_FORWARD, (--)) {
            JUMPTO(oparg);
        }

        macro(JUMP_BACKWARD) =
            unused/1 +
            _CHECK_PERIODIC +
            JUMP_BACKWARD_NO_INTERRUPT;

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

        replaced op(_POP_JUMP_IF_FALSE, (cond -- )) {
            assert(PyValue_BoolCheck(cond));
            int flag = PyValue_IsFalse(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(FB_BRANCH(), flag);
            if (flag) {
                JUMPTO(oparg);
            }
        }

        replaced op(_POP_JUMP_IF_TRUE, (cond -- )) {
            assert(PyValue_BoolCheck(cond));
            int flag = PyValue_IsTrue(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(FB_BRANCH(), flag);
            if (flag) {
                JUMPTO(oparg);
            }
        }

        op(_IS_NONE, (value -- b)) {
            if (PyValue_IsNone(value)) {
                b = PyValue_True;
                DEAD(value);
            }
            else {
                b = PyValue_False;
                DECREF_INPUTS();
            }
        }

        // A conditional jump tests the truth of any value: the compiler
        // drops the TO_BOOL before it (Python/flowgraph.c), so the jumps
        // specialise as TO_BOOL does, and their entries keep the branch
        // history with the specialisation counter.
        family(POP_JUMP_IF_FALSE, INLINE_CACHE_ENTRIES_TO_BOOL) = {
            POP_JUMP_IF_FALSE_ALWAYS_TRUE,
            POP_JUMP_IF_FALSE_BOOL,
            POP_JUMP_IF_FALSE_INT,
            POP_JUMP_IF_FALSE_LIST,
            POP_JUMP_IF_FALSE_NONE,
            POP_JUMP_IF_FALSE_STR,
        };

        family(POP_JUMP_IF_TRUE, INLINE_CACHE_ENTRIES_TO_BOOL) = {
            POP_JUMP_IF_TRUE_ALWAYS_TRUE,
            POP_JUMP_IF_TRUE_BOOL,
            POP_JUMP_IF_TRUE_INT,
            POP_JUMP_IF_TRUE_LIST,
            POP_JUMP_IF_TRUE_NONE,
            POP_JUMP_IF_TRUE_STR,
        };

        specializing op(_SPECIALIZE_POP_JUMP_IF, (value -- value)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_PopJumpIf(value, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(TO_BOOL);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(POP_JUMP_IF_TRUE) =
            unused/1 + _SPECIALIZE_POP_JUMP_IF + unused/2 + _TO_BOOL + _POP_JUMP_IF_TRUE;

        macro(POP_JUMP_IF_FALSE) =
            unused/1 + _SPECIALIZE_POP_JUMP_IF + unused/2 + _TO_BOOL + _POP_JUMP_IF_FALSE;

        macro(POP_JUMP_IF_FALSE_BOOL) =
            unused/1 + unused/2 + _GUARD_TOS_BOOL + _POP_JUMP_IF_FALSE;
        macro(POP_JUMP_IF_TRUE_BOOL) =
            unused/1 + unused/2 + _GUARD_TOS_BOOL + _POP_JUMP_IF_TRUE;
        macro(POP_JUMP_IF_FALSE_INT) =
            _GUARD_TOS_EXACT_INT + unused/1 + unused/2 + _TO_BOOL_INT + _POP_TOP_INT + _POP_JUMP_IF_FALSE;
        macro(POP_JUMP_IF_TRUE_INT) =
            _GUARD_TOS_EXACT_INT + unused/1 + unused/2 + _TO_BOOL_INT + _POP_TOP_INT + _POP_JUMP_IF_TRUE;
        macro(POP_JUMP_IF_FALSE_LIST) =
            _GUARD_TOS_LIST + unused/1 + unused/2 + _TO_BOOL_LIST + POP_TOP + _POP_JUMP_IF_FALSE;
        macro(POP_JUMP_IF_TRUE_LIST) =
            _GUARD_TOS_LIST + unused/1 + unused/2 + _TO_BOOL_LIST + POP_TOP + _POP_JUMP_IF_TRUE;
        macro(POP_JUMP_IF_FALSE_NONE) =
            unused/1 + unused/2 + _TO_BOOL_NONE + _POP_JUMP_IF_FALSE;
        macro(POP_JUMP_IF_TRUE_NONE) =
            unused/1 + unused/2 + _TO_BOOL_NONE + _POP_JUMP_IF_TRUE;
        macro(POP_JUMP_IF_FALSE_STR) =
            _GUARD_TOS_UNICODE + unused/1 + unused/2 + _TO_BOOL_STR + _POP_TOP_UNICODE + _POP_JUMP_IF_FALSE;
        macro(POP_JUMP_IF_TRUE_STR) =
            _GUARD_TOS_UNICODE + unused/1 + unused/2 + _TO_BOOL_STR + _POP_TOP_UNICODE + _POP_JUMP_IF_TRUE;
        macro(POP_JUMP_IF_FALSE_ALWAYS_TRUE) =
            unused/1 + _GUARD_TYPE_VERSION + _REPLACE_WITH_TRUE + POP_TOP + _POP_JUMP_IF_FALSE;
        macro(POP_JUMP_IF_TRUE_ALWAYS_TRUE) =
            unused/1 + _GUARD_TYPE_VERSION + _REPLACE_WITH_TRUE + POP_TOP + _POP_JUMP_IF_TRUE;

        macro(POP_JUMP_IF_NONE) = unused/1 + _IS_NONE + _POP_JUMP_IF_TRUE;

        macro(POP_JUMP_IF_NOT_NONE) = unused/1 + _IS_NONE + _POP_JUMP_IF_FALSE;

        // The value of "x and y" or "x or y": the jump keeps the value it
        // tests where it jumps and releases it where it falls through, so
        // neither a COPY before it nor a POP_TOP after it runs.  The
        // register passes make it from those three (Python/regcfg_passes.c);
        // the result slot is NULL where it falls through.  It has no
        // feedback entry, since its fields fill the word.
        inst(JUMP_IF_FALSE_OR_POP, (cond -- res)) {
            // Taken before the truth test, which can run code
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

        inst(JUMP_IF_TRUE_OR_POP, (cond -- res)) {
            // Taken before the truth test, which can run code
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

        replaced inst(JUMP_BACKWARD_NO_INTERRUPT, (--)) {
            /* This bytecode is used in the `yield from` or `await` loop.
             * If there is an interrupt, we want it handled in the innermost
             * generator or coroutine, so we deliberately do not check it here.
             * (see bpo-30039).
             */
            assert(JUMP_TARGET(oparg) < next_instr);
            JUMPTO(oparg);
        }

        inst(GET_LEN, (obj -- obj, len)) {
            // PUSH(len(TOS))
            Py_ssize_t len_i = _PyValue_Size(obj);
            ERROR_IF(len_i < 0);
            len = PyValue_FromInt64(len_i);
            ERROR_IF(PyValue_IsNull(len));
        }

        op(_MATCH_CLASS, (subject, type, names -- attrs, s, tp, n)) {
            // Pop TOS and TOS1. Set TOS to a tuple of attributes on success, or
            // None on failure.
            assert(PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(names)));
            PyObject *attrs_o = _PyEval_MatchClass(tstate,
                subject,
                PyValue_AsKnownObjectBorrow(type), oparg,
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

        macro(MATCH_CLASS) = _MATCH_CLASS + POP_TOP + POP_TOP + POP_TOP;

        inst(MATCH_MAPPING, (subject -- subject, res)) {
            int match = PyValue_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            res = match ? PyValue_True : PyValue_False;
        }

        inst(MATCH_SEQUENCE, (subject -- subject, res)) {
            int match = PyValue_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE;
            res = match ? PyValue_True : PyValue_False;
        }

        inst(MATCH_KEYS, (subject, keys -- subject, keys, values_or_none)) {
            // On successful match, PUSH(values). Otherwise, PUSH(None).
            PyObject *values_or_none_o = _PyEval_MatchKeys(tstate,
                PyValue_AsKnownObjectBorrow(subject), PyValue_AsKnownObjectBorrow(keys));
            ERROR_IF(values_or_none_o == NULL);
            values_or_none = PyValue_FromObjectSteal(values_or_none_o);
        }

        family(GET_ITER, INLINE_CACHE_ENTRIES_GET_ITER) = {
            GET_ITER_SELF,
            GET_ITER_VIRTUAL,
        };

        specializing op(_SPECIALIZE_GET_ITER, (iterable -- iterable)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_GetIter(iterable, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(GET_ITER);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_GET_ITER, (iterable -- iter, index_or_null)) {
            _PyValuePair got = _PyEval_GetIter(iterable, oparg);
            DEAD(iterable);
            ERROR_IF(PyValue_IsNull(got.first));
            iter = got.first;
            index_or_null = got.second;
        }

        macro(GET_ITER) =
            unused/1 + _SPECIALIZE_GET_ITER +
            _GET_ITER;

        op(_GUARD_ITERATOR, (iterable -- iterable)) {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(iterable));
            EXIT_IF(tp->tp_iter != PyObject_SelfIter);
            STAT_INC(GET_ITER, hit);
        }

        macro(GET_ITER_SELF) =
            unused/1 +
            _GUARD_ITERATOR +
            PUSH_NULL;

        op(_GUARD_ITER_VIRTUAL, (iterable -- iterable)) {
            PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(iterable));
            EXIT_IF(tp->_tp_iteritem == NULL);
            STAT_INC(GET_ITER, hit);
        }

        op(_PUSH_TAGGED_ZERO, ( -- zero)) {
            zero = PyValue_TagIndex(0);
        }

        macro(GET_ITER_VIRTUAL) =
            unused/1 +
            _GUARD_ITER_VIRTUAL +
            _PUSH_TAGGED_ZERO;

        // The body outcome preserves the iterator/cursor pair and defines
        // the item recipient. Exhaustion retires the pair, preserves the
        // last item binding and continues at the real exit target.

        family(FOR_ITER, INLINE_CACHE_ENTRIES_FOR_ITER) = {
            FOR_ITER_LIST,
            FOR_ITER_TUPLE,
            FOR_ITER_RANGE,
            FOR_ITER_GEN,
            FOR_ITER_VIRTUAL,
        };

        specializing op(_SPECIALIZE_FOR_ITER, (iter, null_or_index:nullable -- iter, null_or_index)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_ForIter(iter, null_or_index, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(FOR_ITER);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        replaced op(_FOR_ITER, (iter, null_or_index:nullable -- iter, null_or_index, next)) {
            _PyStatusValue step = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &window[1]);
            null_or_index = _PyFrame_ReadHome(frame, window + 1);
            PyValue item = step.value;
            int found = step.status;
            if (found <= 0) {
                if (found < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
            next = item;
        }

        macro(FOR_ITER) = unused/1 + _SPECIALIZE_FOR_ITER + _FOR_ITER;

        op(_GUARD_TOS_NOT_NULL, (null_or_index:nullable -- null_or_index)) {
            EXIT_IF(PyValue_IsNull(null_or_index));
        }

        replaced op(_FOR_ITER_VIRTUAL, (iter, null_or_index -- iter, null_or_index, next)) {
            assert(PyValue_IsObject(iter));
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            Py_ssize_t index = PyValue_UntagIndex(null_or_index);
            _PyValueIndexPair next_index = Py_TYPE(iter_o)->_tp_iteritem(iter_o, index);
            index = next_index.index;
            if (PyValue_IsNull(next_index.value)) {
                if (index < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
            null_or_index = PyValue_TagIndex(index);
            next = next_index.value;
        }

        macro(FOR_ITER_VIRTUAL) =
            unused/1 +  // Skip over the entry distance
            _GUARD_TOS_NOT_NULL +
            _FOR_ITER_VIRTUAL;

        inst(INSTRUMENTED_FOR_ITER, (unused/1, iter, null_or_index:nullable -- iter, null_or_index, next)) {
            _PyStatusValue step = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &window[1]);
            null_or_index = _PyFrame_ReadHome(frame, window + 1);
            PyValue item = step.value;
            int found = step.status;
            if (found <= 0) {
                if (found < 0) {
                    ERROR_NO_POP();
                }
                PyValue completed = PyValue_IsNull(item) ? PyValue_None : item;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
            next = item;
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
        }

        op(_ITER_CHECK_LIST, (iter, null_or_index -- iter, null_or_index)) {
            // GET_ITER always leaves an object, never an inline value.
            assert(PyValue_IsObject(iter));
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(iter_o) != &PyList_Type);
            assert(PyValue_IsTaggedIndex(null_or_index));
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_Py_IsOwnedByCurrentThread(iter_o) && !_PyObject_GC_IS_SHARED(iter_o));
#endif
        }

        replaced op(_ITER_JUMP_LIST, (iter, null_or_index -- iter, null_or_index)) {
#ifdef Py_GIL_DISABLED
            // For free-threaded Python, the loop exit can happen at any point during
            // item retrieval, so it doesn't make much sense to check and jump
            // separately before item retrieval. Any length check we do here can be
            // invalid by the time we actually try to fetch the item.
#else
            PyObject *list_o = PyValue_AsKnownObjectBorrow(iter);
            assert(Py_TYPE(list_o) == &PyList_Type);
            STAT_INC(FOR_ITER, hit);
            if ((size_t)PyValue_UntagIndex(null_or_index) >= (size_t)PyList_GET_SIZE(list_o)) {
                null_or_index = PyValue_TagIndex(-1);
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
#endif
        }

        replaced op(_ITER_NEXT_LIST, (iter, null_or_index -- iter, null_or_index, next)) {
            PyObject *list_o = PyValue_AsKnownObjectBorrow(iter);
            assert(PyList_CheckExact(list_o));
            next = PyValue_XDUP(PyList_GET_VALUE(list_o, PyValue_UntagIndex(null_or_index)));
            null_or_index = PyValue_IncrementTaggedIndexNoOverflow(null_or_index);
        }

        macro(FOR_ITER_LIST) =
            unused/1 +  // Skip over the entry distance
            _ITER_CHECK_LIST +
            _ITER_JUMP_LIST +
            _ITER_NEXT_LIST;

        op(_ITER_CHECK_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
            assert(PyValue_IsObject(iter));
            PyObject *iter_o = PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(iter_o) != &PyTuple_Type);
            assert(PyValue_IsTaggedIndex(null_or_index));
        }

        replaced op(_ITER_JUMP_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
            PyObject *tuple_o = PyValue_AsKnownObjectBorrow(iter);
            (void)tuple_o;
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            STAT_INC(FOR_ITER, hit);
            if ((size_t)PyValue_UntagIndex(null_or_index) >= (size_t)PyTuple_GET_SIZE(tuple_o)) {
                null_or_index = PyValue_TagIndex(-1);
                PyValue completed = PyValue_None;
                IR_COMPLETE_STEP(_PY_IR_FOR_ITER, completed, oparg);
            }
        }

        op(_ITER_NEXT_TUPLE, (iter, null_or_index -- iter, null_or_index, next)) {
            PyObject *tuple_o = PyValue_AsKnownObjectBorrow(iter);
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            uintptr_t i = PyValue_UntagIndex(null_or_index);
            assert((size_t)i < (size_t)PyTuple_GET_SIZE(tuple_o));
            next = PyValue_XDUP(PyTuple_GET_VALUE(tuple_o, i));
            null_or_index = PyValue_IncrementTaggedIndexNoOverflow(null_or_index);
        }

        macro(FOR_ITER_TUPLE) =
            unused/1 +  // Skip over the entry distance
            _ITER_CHECK_TUPLE +
            _ITER_JUMP_TUPLE +
            _ITER_NEXT_TUPLE;

        op(_ITER_CHECK_RANGE, (iter, null_or_index -- iter, null_or_index)) {
            assert(PyValue_IsObject(iter));
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(r) != &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_PyObject_IsUniquelyReferenced((PyObject *)r));
#endif
        }

        replaced op(_ITER_JUMP_RANGE, (iter, null_or_index -- iter, null_or_index)) {
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

        op(_ITER_NEXT_RANGE, (iter, null_or_index -- iter, null_or_index, next)) {
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

        op(_FOR_ITER_GEN_FRAME, (iter, null -- iter, null, gen_frame)) {
            assert(PyValue_IsObject(iter));
            PyGenObject *gen = (PyGenObject *)PyValue_AsKnownObjectBorrow(iter);
            EXIT_IF(Py_TYPE(gen) != &PyGen_Type);
            EXIT_IF(!gen_try_set_executing((PyGenObject *)gen));
            STAT_INC(FOR_ITER, hit);
            IR_ENTER_GENERATOR(gen, _PY_IR_FOR_ITER);
        }

        macro(FOR_ITER_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _FOR_ITER_GEN_FRAME +
            _PUSH_FRAME;

        // A window: the method and self go to the canonical slots, which a
        // folded input, a local, would not name
        window op(_INSERT_NULL, (self -- method_and_self[2])) {
            IR_WRITE_OUTPUT(method_and_self, 1, self);
            IR_WRITE_OUTPUT(method_and_self, 0, PyValue_NULL);
            DEAD(self);
        }

        op(_LOAD_SPECIAL, (method_and_self[2] -- method_and_self[2])) {
            PyObject *name = _Py_SpecialMethods[oparg].name;
            int err = IR_LOOKUP_SPECIAL(name, method_and_self);
            if (err <= 0) {
                if (err == 0) {
                    PyObject *owner = _PyValueTemps_Object(value_temps, method_and_self[1]);
                    if (owner == NULL) {
                        ERROR_NO_POP();
                    }
                    const char *errfmt = _PyEval_SpecialMethodCanSuggest(owner, oparg)
                        ? _Py_SpecialMethods[oparg].error_suggestion
                        : _Py_SpecialMethods[oparg].error;
                    assert(!_PyErr_Occurred(tstate));
                    assert(errfmt != NULL);
                    _PyErr_Format(tstate, PyExc_TypeError, errfmt, owner);
                }
                ERROR_NO_POP();
            }
        }

        macro(LOAD_SPECIAL) =
            _INSERT_NULL +
            _LOAD_SPECIAL;

        inst(WITH_EXCEPT_START, (exit_func, exit_self:nullable, lasti, unused, val -- exit_func, exit_self, lasti, unused, val, res)) {
            /* At the top of the stack are 4 values:
               - val: TOP = exc_info()
               - unused: SECOND = previous exception
               - lasti: THIRD = lasti of exception in exc_info()
               - exit_self: FOURTH = the context or NULL
               - exit_func: FIFTH = the context.__exit__ function or context.__exit__ bound method
               We call FOURTH(type(TOP), TOP, GetTraceback(TOP)).
               Then we push the __exit__ return value.
            */
            PyObject *exc, *tb;

            PyObject *val_o = PyValue_AsKnownObjectBorrow(val);
            PyObject *exit_func_o = PyValue_AsKnownObjectBorrow(exit_func);

            assert(val_o && PyExceptionInstance_Check(val_o));
            exc = PyExceptionInstance_Class(val_o);
            PyObject *original_tb = tb = PyException_GetTraceback(val_o);
            if (tb == NULL) {
                tb = Py_None;
            }
            assert(PyValue_IsTaggedIndex(lasti));
            (void)lasti; // Shut up compiler warning if asserts are off
            PyObject *res_o = with_exit_call(exit_func_o,
                PyValue_AsKnownObjectBorrow(exit_self),
                !PyValue_IsNull(exit_self), exc, val_o, tb);
            Py_XDECREF(original_tb);
            ERROR_IF(res_o == NULL);
            res = PyValue_FromObjectSteal(res_o);
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

        window inst(PUSH_EXC_INFO, (exc -- prev_exc, new_exc)) {

            _PyErr_StackItem *exc_info = tstate->exc_info;
            if (exc_info->exc_value != NULL) {
                prev_exc = PyValue_FromObjectSteal(exc_info->exc_value);
            }
            else {
                prev_exc = PyValue_None;
            }
            assert(PyValue_ExceptionInstanceCheck(exc));
            exc_info->exc_value = PyValue_AsPyObjectNew(exc);
            new_exc = exc;
            DEAD(exc);
        }

        op(_LOAD_ATTR_METHOD_WITH_VALUES, (class_value/4, owner -- attr, self)) {
            assert(oparg & 1);
            /* Cached method object */
            STAT_INC(LOAD_ATTR, hit);
            assert(PyValue_IsObject(_PyValueGuarded_Load(class_value)));
            assert(_PyType_HasFeature(
                Py_TYPE(PyValue_AsKnownObjectBorrow(_PyValueGuarded_Load(class_value))),
                Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyValue_DUP(_PyValueGuarded_Load(class_value));
            self = TAKE(owner);
            IR_CANONICAL_OUTPUTS(attr, self);
        }

        macro(LOAD_ATTR_METHOD_WITH_VALUES) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_MANAGED_OBJECT_HAS_VALUES +
            unused/2 +
            _LOAD_ATTR_METHOD_WITH_VALUES;

        op(_LOAD_ATTR_METHOD_NO_DICT, (class_value/4, owner -- attr, self)) {
            assert(oparg & 1);
            assert(Py_TYPE(PyValue_AsKnownObjectBorrow(owner))->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(PyValue_IsObject(_PyValueGuarded_Load(class_value)));
            assert(_PyType_HasFeature(
                Py_TYPE(PyValue_AsKnownObjectBorrow(_PyValueGuarded_Load(class_value))),
                Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyValue_DUP(_PyValueGuarded_Load(class_value));
            self = TAKE(owner);
            IR_CANONICAL_OUTPUTS(attr, self);
        }

        macro(LOAD_ATTR_METHOD_NO_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_METHOD_NO_DICT;

        op(_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES, (class_value/4, owner -- attr)) {
            assert((oparg & 1) == 0);
            STAT_INC(LOAD_ATTR, hit);
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            assert(!PyValue_IsNull(found));
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_MANAGED_OBJECT_HAS_VALUES +
            unused/2 +
            _LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES;

        op(_LOAD_ATTR_NONDESCRIPTOR_NO_DICT, (class_value/4, owner -- attr)) {
            assert((oparg & 1) == 0);
            assert(Py_TYPE(PyValue_AsKnownObjectBorrow(owner))->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            PyValueBorrow found = _PyValueGuarded_Load(class_value);
            assert(!PyValue_IsNull(found));
            attr = IR_CANONICAL_HOME_DUP(found);
            PyValue receiver = OWNER(owner);
            IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
        }

        macro(LOAD_ATTR_NONDESCRIPTOR_NO_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_NONDESCRIPTOR_NO_DICT;

        op(_CHECK_ATTR_METHOD_LAZY_DICT, (dictoffset/1, owner -- owner)) {
            char *ptr = ((char *)PyValue_AsKnownObjectBorrow(owner)) + MANAGED_DICT_OFFSET + dictoffset;
            PyObject *dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*(PyObject **)ptr);
            /* This object has a __dict__, just not yet created */
            EXIT_IF(dict != NULL);
        }

        op(_LOAD_ATTR_METHOD_LAZY_DICT, (class_value/4, owner -- attr, self)) {
            assert(oparg & 1);
            STAT_INC(LOAD_ATTR, hit);
            assert(PyValue_IsObject(_PyValueGuarded_Load(class_value)));
            assert(_PyType_HasFeature(
                Py_TYPE(PyValue_AsKnownObjectBorrow(_PyValueGuarded_Load(class_value))),
                Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyValue_DUP(_PyValueGuarded_Load(class_value));
            self = TAKE(owner);
            IR_CANONICAL_OUTPUTS(attr, self);
        }

        macro(LOAD_ATTR_METHOD_LAZY_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_ATTR_METHOD_LAZY_DICT +
            unused/1 +
            _LOAD_ATTR_METHOD_LAZY_DICT;

        // Feedback: func_version
        // CALL_INTRINSIC_1/2, CALL_KW, and CALL_FUNCTION_EX aren't members!
        family(CALL, INLINE_CACHE_ENTRIES_CALL) = {
            CALL_BOUND_METHOD_EXACT_ARGS,
            CALL_PY_EXACT_ARGS,
            CALL_TYPE_1,
            CALL_STR_1,
            CALL_TUPLE_1,
            CALL_BUILTIN_CLASS,
            CALL_BUILTIN_O,
            CALL_BUILTIN_FAST,
            CALL_BUILTIN_FAST_WITH_KEYWORDS,
            CALL_LEN,
            CALL_ISINSTANCE,
            CALL_LIST_APPEND,
            CALL_METHOD_DESCRIPTOR_O,
            CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS,
            CALL_METHOD_DESCRIPTOR_NOARGS,
            CALL_METHOD_DESCRIPTOR_FAST,
            CALL_ALLOC_AND_ENTER_INIT,
            CALL_PY_GENERAL,
            CALL_BOUND_METHOD_GENERAL,
            CALL_NON_PY_GENERAL,
        };

        specializing op(_SPECIALIZE_CALL, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            #if ENABLE_SPECIALIZATION
            {
                _PyFbHeader *feedback = FB_COUNTER();
                if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                    /* The specialisation is an effect of the call
                     * transaction, which then finishes the call generically
                     * on this word: DISPATCH_SAME_OPARG() goes to its
                     * fallback, and next_instr stays the next word. */
                    IR_BEGIN_CALL_EFFECTS();
                    _Py_Specialize_Call(callable, self_or_null, this_instr, oparg + ((call_state & CALL_SELF_PRESENT) != 0));
                    IR_REFRESH_CALL();
                    DISPATCH_SAME_OPARG();
                }
                OPCODE_DEFERRED_INC(CALL);
                ADVANCE_ADAPTIVE_COUNTER(feedback);
            }
            #endif  /* ENABLE_SPECIALIZATION */
        }

        inline_ok op(_MAYBE_EXPAND_METHOD, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            IR_PREPARE_CALL_METHOD();
            callable = window[0];
            self_or_null = call_state & CALL_SELF_PRESENT ? window[1] : PyValue_NULL;
        }

        // When calling Python, inline the call using DISPATCH_INLINED().
        inline_ok op(_DO_CALL, (callable, self_or_null, args[oparg] -- res)) {
            IR_INVOKE_CALL();
        }

        inline_ok op(_MONITOR_CALL, (func, maybe_self, args[oparg] -- func, maybe_self, args[oparg])) {
            IR_MONITOR_CALL();
            func = window[0];
            maybe_self = call_state & CALL_SELF_PRESENT ? window[1] : PyValue_NULL;
            args = window + 2;
        }

        macro(CALL) = unused/1 + _SPECIALIZE_CALL + unused/2 + _MAYBE_EXPAND_METHOD + _DO_CALL + _CHECK_PERIODIC_AT_END;
        macro(INSTRUMENTED_CALL) = unused/3 + _MAYBE_EXPAND_METHOD + _MONITOR_CALL + _DO_CALL + _CHECK_PERIODIC_AT_END;

        inline_ok op(_PY_FRAME_GENERAL, (callable, self_or_null, args[oparg] -- new_frame)) {
            IR_INVOKE_CALL();
        }

        inline_ok op(_PY_FRAME_GENERAL_GUARDED, (callable, self_or_null, args[oparg] -- new_frame)) {
            IR_INVOKE_GUARDED_PYTHON_CALL();
        }

        op(_CHECK_FUNCTION_VERSION, (func_version/2, callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(!PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            EXIT_IF(func->func_version != func_version);
        }

        macro(CALL_PY_GENERAL) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION +
            _CHECK_RECURSION_REMAINING +
            _PY_FRAME_GENERAL_GUARDED +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_CHECK_METHOD_VERSION, (func_version/2, callable, null, unused[oparg] -- callable, null, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(call_state & CALL_SELF_PRESENT);
        }

        inline_ok op(_EXPAND_METHOD, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            IR_PREPARE_CALL_METHOD();
            callable = window[0];
            self_or_null = call_state & CALL_SELF_PRESENT ? window[1] : PyValue_NULL;
        }

        macro(CALL_BOUND_METHOD_GENERAL) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_METHOD_VERSION +
            _EXPAND_METHOD +
            flush + // so that self is in the argument array
            _CHECK_RECURSION_REMAINING +
            _PY_FRAME_GENERAL +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        inline_ok op(_CHECK_IS_NOT_PY_CALLABLE, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            if (PyValue_IsObject(callable)) {
                PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
                EXIT_IF(PyFunction_Check(callable_o));
                EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type);
            }
        }

        inline_ok op(_CALL_NON_PY_GENERAL, (callable, self_or_null, args[oparg] -- res)) {
            IR_INVOKE_CALL();
        }

        macro(CALL_NON_PY_GENERAL) =
            unused/1 + // Skip over the entry distance
            unused/2 +
            _CHECK_IS_NOT_PY_CALLABLE +
            _CALL_NON_PY_GENERAL +
            _CHECK_PERIODIC_AT_END;

        op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
            EXIT_IF(call_state & CALL_SELF_PRESENT);
            EXIT_IF(Py_TYPE(PyValue_AsKnownObjectBorrow(callable)) != &PyMethod_Type);
        }

        op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            assert(!(call_state & CALL_SELF_PRESENT));
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            STAT_INC(CALL, hit);
            self_or_null = PyValue_DUP(((PyMethodObject *)callable_o)->im_self);
            call_state |= CALL_SELF_PRESENT | CALL_EFFECTS;
            PyValue temp = callable;
            callable = PyValue_DUP(PyValue_FromObjectBorrow(((PyMethodObject *)callable_o)->im_func));
            PyValue_CLOSE(temp);
        }

        op(_CHECK_PEP_523, (--)) {
            DEOPT_IF(IS_PEP523_HOOKED(tstate));
        }

        op(_CHECK_FUNCTION_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            assert(PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            EXIT_IF(code->co_argcount != oparg + ((call_state & CALL_SELF_PRESENT) != 0));
        }

        op(_CHECK_STACK_SPACE, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
        }

        op(_CHECK_RECURSION_REMAINING, (--)) {
            DEOPT_IF(tstate->py_recursion_remaining <= 1);
        }

        pure op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame)) {
            int has_self = (call_state & CALL_SELF_PRESENT) != 0;
            STAT_INC(CALL, hit);
            // The window: callable, self_or_null, then the arguments
            PyValue *window_base = args - 2;
            PyFunctionObject *func = (PyFunctionObject *)PyValue_AsKnownObjectBorrow(callable);
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            PyValue *base = args - has_self;
            if (frame->owner != FRAME_OWNED_BY_THREAD) {
                base = _PyDataStack_Slots(tstate->datastack_top);
            }
            _PyInterpreterFrame *pushed_frame = _PyFrame_ReserveForCall(
                tstate, code, base, oparg + has_self, frame);
            _PyFrame_Initialize(tstate, pushed_frame, callable, NULL,
                                code, base, frame);
            if (frame->owner != FRAME_OWNED_BY_THREAD) {
                call_transfer_exact_args(pushed_frame, frame, window_base, oparg,
                                         has_self ? CALL_SELF_SUPPLIED : CALL_SELF_ABSENT);
            }
            // The callee holds callable as its f_funcobj
            _PyFrame_WriteHome(frame, window_base, PyValue_NULL);
            INPUTS_DEAD();
            IR_WINDOW_TAKEN();
            new_frame = PyValue_Wrap(pushed_frame);
        }

        op(_PUSH_FRAME, (new_frame -- )) {
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

        macro(CALL_BOUND_METHOD_EXACT_ARGS) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_CALL_BOUND_METHOD_EXACT_ARGS +
            _INIT_CALL_BOUND_METHOD_EXACT_ARGS +
            flush + // In case the following deopt
            _CHECK_FUNCTION_VERSION +
            _CHECK_FUNCTION_EXACT_ARGS +
            _CHECK_STACK_SPACE +
            _CHECK_RECURSION_REMAINING +
            _INIT_CALL_PY_EXACT_ARGS +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        macro(CALL_PY_EXACT_ARGS) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION +
            _CHECK_FUNCTION_EXACT_ARGS +
            _CHECK_STACK_SPACE +
            _CHECK_RECURSION_REMAINING +
            _INIT_CALL_PY_EXACT_ARGS +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_GUARD_NOS_NULL, (null, unused -- null, unused)) {
            EXIT_IF(call_state & CALL_SELF_PRESENT);
        }

        op(_GUARD_THIRD_NULL, (null, unused, unused -- null, unused, unused)) {
            EXIT_IF(call_state & CALL_SELF_PRESENT);
        }

        op(_GUARD_CALLABLE_TYPE_1, (callable, unused, unused -- callable, unused, unused)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(callable_o != (PyObject *)&PyType_Type);
        }

        inline_ok op(_CALL_TYPE_1, (callable, null, arg -- res, a)) {
            assert(oparg == 1);
            STAT_INC(CALL, hit);
            res = PyValue_DUP(PyValue_FromObjectBorrow(PyValue_TYPE(arg)));
            a = arg;
            INPUTS_DEAD();
        }

        macro(CALL_TYPE_1) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_TYPE_1 +
            _CALL_TYPE_1 +
            POP_TOP;

        op(_GUARD_CALLABLE_STR_1, (callable, unused, unused -- callable, unused, unused)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(callable_o != (PyObject *)&PyUnicode_Type);
        }

        op(_CALL_STR_1, (callable, null, arg -- res, a)) {
            assert(oparg == 1);
            STAT_INC(CALL, hit);
            PyObject *res_o = _PyValue_Str(arg);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            a = arg;
            INPUTS_DEAD();
            res = PyValue_FromObjectSteal(res_o);
        }

        macro(CALL_STR_1) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_STR_1 +
            _CALL_STR_1 +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_TUPLE_1, (callable, unused, unused -- callable, unused, unused)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(callable_o != (PyObject *)&PyTuple_Type);
        }

        op(_CALL_TUPLE_1, (callable, null, arg -- res, a)) {
            assert(oparg == 1);
            STAT_INC(CALL, hit);
            PyObject *res_o = _PyValue_SequenceTuple(arg);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            a = arg;
            INPUTS_DEAD();
            res = PyValue_FromObjectSteal(res_o);
        }

        macro(CALL_TUPLE_1) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_TUPLE_1 +
            _CALL_TUPLE_1 +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_CHECK_OBJECT, (type_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(call_state & CALL_SELF_PRESENT);
            EXIT_IF(!PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            EXIT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(tp->tp_version_tag) != type_version);
        }

        op(_ALLOCATE_OBJECT, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            assert(!(call_state & CALL_SELF_PRESENT));
            assert(PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            assert(tp->tp_new == PyBaseObject_Type.tp_new);
            assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
            assert(tp->tp_alloc == PyType_GenericAlloc);

            PyHeapTypeObject *cls = (PyHeapTypeObject *)callable_o;
            PyFunctionObject *init_func = (PyFunctionObject *)FT_ATOMIC_LOAD_PTR_ACQUIRE(cls->_spec_cache.init);
            EXIT_IF(init_func->vectorcall != _PyFunction_Vectorcall);
            PyCodeObject *code = (PyCodeObject *)init_func->func_code;
            EXIT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize + _Py_InitCleanup.co_framesize));
            PyValue init_owner = PyValue_DUP(PyValue_FromObjectBorrow(init_func));
            STAT_INC(CALL, hit);
            PyObject *self_o = PyType_GenericAlloc(tp, 0);
            if (self_o == NULL) {
                PyValue_CLOSE(init_owner);
                ERROR_NO_POP();
            }
            self_or_null = PyValue_FromObjectSteal(self_o);
            call_state |= CALL_SELF_PRESENT | CALL_EFFECTS;
            PyValue temp = callable;
            callable = init_owner;
            PyValue_CLOSE(temp);
        }

        op(_CREATE_INIT_FRAME, (init, self, args[oparg] -- init_frame)) {
            _PyInterpreterFrame *temp = _PyEval_CreateInitFrame(
                tstate, frame, args-2, oparg, IR_DST_SLOT, next_instr);
            if (temp == NULL) {
                /* Preparation can leave the original window intact. Unwind
                 * its actual homes before DEAD emits any successful clear. */
                ERROR_NO_POP();
            }
            DEAD(init);
            DEAD(self);
            DEAD(args);
            SYNC_SP();
            init_frame = PyValue_Wrap(temp);
        }

        macro(CALL_ALLOC_AND_ENTER_INIT) =
            unused/1 +
            _CHECK_PEP_523 +
            _CHECK_OBJECT +
            _CHECK_RECURSION_REMAINING +
            _ALLOCATE_OBJECT +
            _CREATE_INIT_FRAME +
            _PUSH_FRAME;

        inst(EXIT_INIT_CHECK, (should_be_none -- )) {
            if (!PyValue_IsNone(should_be_none)) {
                PyErr_Format(PyExc_TypeError,
                    "__init__() should return None, not '%.200s'",
                    PyValue_TYPE(should_be_none)->tp_name);
                ERROR_NO_POP();
            }
            DEAD(should_be_none);
        }

        op(_GUARD_CALLABLE_BUILTIN_CLASS, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(!PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            EXIT_IF(tp->tp_vectorcall == NULL);
        }

        op(_CALL_BUILTIN_CLASS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            int total_args = oparg;
            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
                total_args++;
            }
            STAT_INC(CALL, hit);
            PyValue result = _PyValue_VectorcallTstate(
                tstate, callable, arguments, total_args, NULL);
            if (PyValue_IsNull(result)) {
                ERROR_NO_POP();
            }
            PyValue temp = callable;
            callable = result;
            PyValue_CLOSE(temp);
        }

        macro(CALL_BUILTIN_CLASS) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_BUILTIN_CLASS +
            _CALL_BUILTIN_CLASS +
            _POP_TOP_OPARG +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_BUILTIN_O, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(!PyCFunction_CheckExact(callable_o));
            EXIT_IF((PyCFunction_GET_FLAGS(callable_o) &
                     _Py_METH_CALL_FLAGS) != METH_O);
            int total_args = oparg;
            if (call_state & CALL_SELF_PRESENT) {
                total_args++;
            }
            EXIT_IF(total_args != 1);
        }

        op(_CALL_BUILTIN_O, (callable, self_or_null, args[oparg] -- res)) {
            /* Builtin METH_O functions */
            if (call_state & CALL_SELF_PRESENT) {
                args--;
            }
            STAT_INC(CALL, hit);
            PyValue result = native_call_builtin_o(
                tstate, frame, next_instr, IR_BASE, oparg + 2, callable, args,
                call_state);
            if (PyValue_IsNull(result)) {
                ERROR_NO_POP();
            }
            this_instr = frame->instr_ptr;
            ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);
            slots = _PyFrame_Slots(frame);
            window = &slots[IR_BASE];
            INPUTS_DEAD();
            res = result;
            SYNC_SP();
        }

        macro(CALL_BUILTIN_O) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_BUILTIN_O +
            _CHECK_RECURSION_LIMIT +
            _CALL_BUILTIN_O +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_BUILTIN_FAST, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(!PyCFunction_CheckExact(callable_o));
            int flags = PyCFunction_GET_FLAGS(callable_o) & _Py_METH_CALL_FLAGS;
            /* A METH_VALUE function (alone or with METH_NOARGS or METH_O)
               reaches the same value vectorcall, as METH_FASTCALL does.
               With METH_KEYWORDS it takes the _WITH_KEYWORDS handler. */
            EXIT_IF(flags != METH_FASTCALL
                    && (flags & ~(METH_NOARGS | METH_O)) != METH_VALUE);
        }

        op(_CALL_BUILTIN_FAST, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            /* Builtin METH_FASTCALL functions, without keywords */
            int total_args = oparg;
            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
                total_args++;
            }
            STAT_INC(CALL, hit);
            PyValue result = _PyValue_VectorcallTstate(
                tstate, callable, arguments, total_args, NULL);
            if (PyValue_IsNull(result)) {
                ERROR_NO_POP();
            }
            PyValue temp = callable;
            callable = result;
            PyValue_CLOSE(temp);
        }

        macro(CALL_BUILTIN_FAST) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_BUILTIN_FAST +
            _CALL_BUILTIN_FAST +
            _POP_TOP_OPARG +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_BUILTIN_FAST_WITH_KEYWORDS, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            EXIT_IF(!PyCFunction_CheckExact(callable_o));
            int flags = PyCFunction_GET_FLAGS(callable_o) & _Py_METH_CALL_FLAGS;
            EXIT_IF(flags != (METH_FASTCALL | METH_KEYWORDS)
                    && flags != (METH_VALUE | METH_KEYWORDS));
        }

        op(_CALL_BUILTIN_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            /* Builtin METH_FASTCALL | METH_KEYWORDS functions */
            int total_args = oparg;
            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
                total_args++;
            }
            STAT_INC(CALL, hit);
            PyValue result = _PyValue_VectorcallTstate(
                tstate, callable, arguments, total_args, NULL);
            if (PyValue_IsNull(result)) {
                ERROR_NO_POP();
            }
            PyValue temp = callable;
            callable = result;
            PyValue_CLOSE(temp);
        }

        macro(CALL_BUILTIN_FAST_WITH_KEYWORDS) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_BUILTIN_FAST_WITH_KEYWORDS +
            _CALL_BUILTIN_FAST_WITH_KEYWORDS +
            _POP_TOP_OPARG +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        macro(CALL_LEN) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_LEN +
            _CALL_LEN +
            POP_TOP +
            POP_TOP;

        op(_GUARD_CALLABLE_LEN, (callable, unused, unused -- callable, unused, unused)){
            // A rebinding to an inline value must exit before the borrow.
            PyInterpreterState *interp = tstate->interp;
            EXIT_IF(!PyValue_IsObject(callable)
                    || PyValue_AsKnownObjectBorrow(callable)
                       != interp->callable_cache.len);
        }

        op(_CALL_LEN, (callable, null, arg -- res, a, c)) {
            /* len(o) */
            STAT_INC(CALL, hit);
            Py_ssize_t len_i = _PyValue_Size(arg);
            if (len_i < 0) {
                ERROR_NO_POP();
            }
            // The length is inline when it fits in int51.
            res = PyValue_FromInt64(len_i);
            if (PyValue_IsNull(res)) {
                ERROR_NO_POP();
            }
            a = arg;
            c = callable;
            INPUTS_DEAD();
        }

        op(_GUARD_CALLABLE_ISINSTANCE, (callable, unused, unused, unused -- callable, unused, unused, unused)) {
            // A rebinding to an inline value must exit before the borrow.
            PyInterpreterState *interp = tstate->interp;
            EXIT_IF(!PyValue_IsObject(callable)
                    || PyValue_AsKnownObjectBorrow(callable)
                       != interp->callable_cache.isinstance);
        }

        inline_ok op(_CALL_ISINSTANCE, (callable, null, instance, cls -- res)) {
            /* isinstance(o, o2) */
            STAT_INC(CALL, hit);
            int retval;
            if (PyValue_IsInline(instance) || PyValue_IsInline(cls)) {
                retval = _PyEval_IsInstanceValue(instance, cls);
            }
            else {
                retval = PyObject_IsInstance(PyValue_AsKnownObjectBorrow(instance),
                                             PyValue_AsKnownObjectBorrow(cls));
            }
            if (retval < 0) {
                ERROR_NO_POP();
            }
            (void)null; // Silence compiler warnings about unused variables
            PyValue_CLOSE(cls);
            PyValue_CLOSE(instance);
            DEAD(null);
            PyValue_CLOSE(callable);
            res = retval ? PyValue_True : PyValue_False;
            assert((!PyValue_IsNull(res)) ^ (_PyErr_Occurred(tstate) != NULL));
        }

        macro(CALL_ISINSTANCE) =
            unused/1 +
            unused/2 +
            _GUARD_THIRD_NULL +
            _GUARD_CALLABLE_ISINSTANCE +
            _CALL_ISINSTANCE;

        macro(CALL_LIST_APPEND) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_LIST_APPEND +
            _GUARD_NOS_NOT_NULL +
            _GUARD_NOS_LIST +
            _CALL_LIST_APPEND +
            POP_TOP +
            POP_TOP;

        op(_GUARD_CALLABLE_LIST_APPEND, (callable, unused, unused -- callable, unused, unused)){
            // A rebinding to an inline value must exit before the borrow.
            PyInterpreterState *interp = tstate->interp;
            EXIT_IF(!PyValue_IsObject(callable)
                    || PyValue_AsKnownObjectBorrow(callable)
                       != interp->callable_cache.list_append);
        }

        inline_ok op(_CALL_LIST_APPEND, (callable, self, arg -- none, c, s)) {
            assert(oparg == 1);
            PyObject *self_o = PyValue_AsKnownObjectBorrow(self);

            DEOPT_IF(!LOCK_OBJECT(self_o));
            STAT_INC(CALL, hit);
            PyValue item = TAKE(arg);
            int err = _PyList_AppendTakeValue((PyListObject *)self_o, item);
            UNLOCK_OBJECT(self_o);
            if (err) {
                // arg is consumed even on failure: close the other inputs
                // and pop them all, so that arg is not closed a second time.
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            c = callable;
            s = self;
            DEAD(callable);
            DEAD(self);
            none = PyValue_None;
        }

        op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_O, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            EXIT_IF((method->d_method->ml_flags & _Py_METH_CALL_FLAGS) !=
                    METH_O);
            int total_args = oparg;
            if (call_state & CALL_SELF_PRESENT) {
                total_args++;
            }
            EXIT_IF(total_args != 2);
            PyValue self = !(call_state & CALL_SELF_PRESENT) ? args[0] : self_or_null;
            EXIT_IF(PyValue_TYPE(self) != method->d_common.d_type);
        }

         op(_CALL_METHOD_DESCRIPTOR_O, (callable, self_or_null, args[oparg] -- res, c, s, a)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;

            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
            }
            STAT_INC(CALL, hit);
            PyCFunction cfunc = method->d_method->ml_meth;
            PyObject *self = _PyValueTemps_Object(value_temps, arguments[0]);
            if (self == NULL) {
                ERROR_NO_POP();
            }
            PyObject *arg = _PyValueTemps_Object(value_temps, arguments[1]);
            if (arg == NULL) {
                ERROR_NO_POP();
            }
            _PyBorrowMark borrow_mark = _PyBorrowScope_Enter(tstate);
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, self, arg);
            _PyBorrowScope_Exit(tstate, borrow_mark);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            c = callable;
            s = arguments[0];
            a = arguments[1];
            INPUTS_DEAD();
            res = PyValue_FromObjectSteal(res_o);
        }

        op(_CHECK_RECURSION_LIMIT, ( -- )) {
            EXIT_IF(_Py_ReachedRecursionLimit(tstate));
        }

        macro(CALL_METHOD_DESCRIPTOR_O) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_METHOD_DESCRIPTOR_O +
            _CHECK_RECURSION_LIMIT +
            _CALL_METHOD_DESCRIPTOR_O +
            POP_TOP +
            POP_TOP +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            int flags = method->d_method->ml_flags & _Py_METH_CALL_FLAGS;
            EXIT_IF(flags != (METH_FASTCALL | METH_KEYWORDS)
                    && flags != (METH_VALUE | METH_KEYWORDS));
            int total_args = oparg;
            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
                total_args++;
            }
            EXIT_IF(total_args == 0);
            EXIT_IF(PyValue_TYPE(arguments[0]) != method->d_common.d_type);
        }

        op(_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            int total_args = oparg;
            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
                total_args++;
            }
            STAT_INC(CALL, hit);
            PyValue result = _PyValue_VectorcallTstate(
                tstate, callable, arguments, total_args, NULL);
            if (PyValue_IsNull(result)) {
                ERROR_NO_POP();
            }
            PyValue temp = callable;
            callable = result;
            PyValue_CLOSE(temp);
        }

        macro(CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS +
            _CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS +
            _POP_TOP_OPARG +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_NOARGS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            EXIT_IF((method->d_method->ml_flags & _Py_METH_CALL_FLAGS) !=
                    METH_NOARGS);
            int total_args = oparg;
            if (call_state & CALL_SELF_PRESENT) {
                total_args++;
            }
            EXIT_IF(total_args != 1);
            PyValue self = !(call_state & CALL_SELF_PRESENT) ? args[0] : self_or_null;
            EXIT_IF(PyValue_TYPE(self) != method->d_common.d_type);
        }

        op(_CALL_METHOD_DESCRIPTOR_NOARGS, (callable, self_or_null, args[oparg] -- res, c, s)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;

            assert(oparg == 1 || (call_state & CALL_SELF_PRESENT));
            if (call_state & CALL_SELF_PRESENT) {
                args--;
            }
            PyObject *self = _PyValueTemps_Object(value_temps, args[0]);
            if (self == NULL) {
                ERROR_NO_POP();
            }
            STAT_INC(CALL, hit);
            PyCFunction cfunc = method->d_method->ml_meth;
            _PyBorrowMark borrow_mark = _PyBorrowScope_Enter(tstate);
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, self, NULL);
            _PyBorrowScope_Exit(tstate, borrow_mark);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            c = callable;
            s = args[0];
            INPUTS_DEAD();
            res = PyValue_FromObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_NOARGS) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_METHOD_DESCRIPTOR_NOARGS +
            _CHECK_RECURSION_LIMIT +
            _CALL_METHOD_DESCRIPTOR_NOARGS +
            POP_TOP +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            /* Builtin METH_FASTCALL methods, without keywords */
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            int flags = method->d_method->ml_flags & _Py_METH_CALL_FLAGS;
            EXIT_IF(flags != METH_FASTCALL
                    && (flags & ~(METH_NOARGS | METH_O)) != METH_VALUE);
            int total_args = oparg;
            if (call_state & CALL_SELF_PRESENT) {
                total_args++;
            }
            EXIT_IF(total_args == 0);
            PyValue self = !(call_state & CALL_SELF_PRESENT) ? args[0] : self_or_null;
            EXIT_IF(PyValue_TYPE(self) != method->d_common.d_type);
        }

        op(_CALL_METHOD_DESCRIPTOR_FAST, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
            int total_args = oparg;
            PyValue *arguments = args;
            if (call_state & CALL_SELF_PRESENT) {
                arguments--;
                total_args++;
            }
            STAT_INC(CALL, hit);
            PyValue result = _PyValue_VectorcallTstate(
                tstate, callable, arguments, total_args, NULL);
            if (PyValue_IsNull(result)) {
                ERROR_NO_POP();
            }
            PyValue temp = callable;
            callable = result;
            PyValue_CLOSE(temp);
        }

        macro(CALL_METHOD_DESCRIPTOR_FAST) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST +
            _CALL_METHOD_DESCRIPTOR_FAST +
            _POP_TOP_OPARG +
            POP_TOP +
            _CHECK_PERIODIC_AT_END;

        // Feedback: func_version
        family(CALL_KW, INLINE_CACHE_ENTRIES_CALL_KW) = {
            CALL_KW_BOUND_METHOD,
            CALL_KW_PY,
            CALL_KW_NON_PY,
        };

        inline_ok op(_MONITOR_CALL_KW, (callable, self_or_null, args[oparg], unused -- callable, self_or_null, args[oparg], unused)) {
            IR_MONITOR_CALL();
            callable = window[0];
            self_or_null = call_state & CALL_SELF_PRESENT ? window[1] : PyValue_NULL;
            args = window + 2;
        }

        inline_ok op(_MAYBE_EXPAND_METHOD_KW, (callable, self_or_null, unused[oparg], unused -- callable, self_or_null, unused[oparg], unused)) {
            IR_PREPARE_CALL_METHOD();
            callable = window[0];
            self_or_null = call_state & CALL_SELF_PRESENT ? window[1] : PyValue_NULL;
        }

        inline_ok op(_DO_CALL_KW, (callable, self_or_null, args[oparg], kwnames -- res)) {
            IR_INVOKE_CALL();
        }

        inline_ok op(_PY_FRAME_KW, (callable, self_or_null, args[oparg], kwnames -- new_frame)) {
            IR_INVOKE_CALL();
        }

        inline_ok op(_PY_FRAME_KW_GUARDED, (callable, self_or_null, args[oparg], kwnames -- new_frame)) {
            IR_INVOKE_GUARDED_PYTHON_CALL();
        }

        op(_CHECK_FUNCTION_VERSION_KW, (func_version/2, callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused)) {
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

        op(_CHECK_METHOD_VERSION_KW, (func_version/2, callable, null, unused[oparg], unused -- callable, null, unused[oparg], unused)) {
            PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(call_state & CALL_SELF_PRESENT);
        }

        inline_ok op(_EXPAND_METHOD_KW, (callable, self_or_null, unused[oparg], unused -- callable, self_or_null, unused[oparg], unused)) {
            IR_PREPARE_CALL_METHOD();
            callable = window[0];
            self_or_null = call_state & CALL_SELF_PRESENT ? window[1] : PyValue_NULL;
        }

        macro(CALL_KW_BOUND_METHOD) =
            unused/1 + // Skip over the entry distance
            _CHECK_PEP_523 +
            _CHECK_METHOD_VERSION_KW +
            _EXPAND_METHOD_KW +
            flush + // so that self is in the argument array
            _PY_FRAME_KW +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        specializing op(_SPECIALIZE_CALL_KW, (callable, self_or_null, unused[oparg], kwnames -- callable, self_or_null, unused[oparg], kwnames)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                /* As in _SPECIALIZE_CALL */
                IR_BEGIN_CALL_EFFECTS();
                _Py_Specialize_CallKw(callable, self_or_null, kwnames, this_instr, oparg);
                IR_REFRESH_CALL();
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CALL_KW);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
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

        inline_ok op(_CHECK_IS_NOT_PY_CALLABLE_KW, (callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused)) {
            if (PyValue_IsObject(callable)) {
                PyObject *callable_o = PyValue_AsKnownObjectBorrow(callable);
                EXIT_IF(PyFunction_Check(callable_o));
                EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type);
            }
        }


        inline_ok op(_CALL_KW_NON_PY, (callable, self_or_null, args[oparg], kwnames -- res)) {
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

        inline_ok op(_MAKE_CALLARGS_A_TUPLE, (func, unused, callargs, kwargs -- func, unused, callargs, kwargs)) {
            IR_PREPARE_CALL_TUPLE();
            func = window[0];
            callargs = window[2];
            kwargs = call_state & CALL_KWARGS_PRESENT ? window[3] : PyValue_NULL;
        }

        inline_ok op(_DO_CALL_FUNCTION_EX, (func_st, null, callargs_st, kwargs_st -- result)) {
            IR_INVOKE_CALL();
        }

        specializing op(_SPECIALIZE_CALL_FUNCTION_EX, (func, unused, unused, unused -- func, unused, unused, unused)) {
        #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                /* As in _SPECIALIZE_CALL */
                IR_BEGIN_CALL_EFFECTS();
                _Py_Specialize_CallFunctionEx(func, this_instr);
                IR_REFRESH_CALL();
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CALL_FUNCTION_EX);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
        #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CALL_FUNCTION_EX) =
            unused/1 + _SPECIALIZE_CALL_FUNCTION_EX +
            _MAKE_CALLARGS_A_TUPLE +
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC_AT_END;

        op(_CHECK_IS_PY_CALLABLE_EX, (func_st, unused, unused, unused -- func_st, unused, unused, unused)) {
            PyObject *func = PyValue_AsKnownObjectBorrow(func_st);
            EXIT_IF(Py_TYPE(func) != &PyFunction_Type);
            EXIT_IF(((PyFunctionObject *)func)->vectorcall != _PyFunction_Vectorcall);
        }

        inline_ok op(_PY_FRAME_EX, (func_st, null, callargs_st, kwargs_st -- ex_frame)) {
            IR_INVOKE_CALL();
        }

        macro(CALL_EX_PY) =
            unused/1 +
            _CHECK_PEP_523 +
            _CHECK_IS_PY_CALLABLE_EX +
            _MAKE_CALLARGS_A_TUPLE +
            _CHECK_IS_PY_CALLABLE_EX +
            _PY_FRAME_EX +
            _SAVE_RETURN_SLOT +
            _PUSH_FRAME;

        op(_CHECK_IS_NOT_PY_CALLABLE_EX, (func_st, unused, unused, unused -- func_st, unused, unused, unused)) {
            PyObject *func = PyValue_AsKnownObjectBorrow(func_st);
            EXIT_IF(Py_TYPE(func) == &PyFunction_Type && ((PyFunctionObject *)func)->vectorcall == _PyFunction_Vectorcall);
        }

        inline_ok op(_CALL_FUNCTION_EX_NON_PY_GENERAL, (func_st, null, callargs_st, kwargs_st -- result)) {
            IR_INVOKE_CALL();
        }

        macro(CALL_EX_NON_PY_GENERAL) =
            unused/1 +
            _CHECK_IS_NOT_PY_CALLABLE_EX +
            _MAKE_CALLARGS_A_TUPLE +
            _CHECK_IS_NOT_PY_CALLABLE_EX +
            _CALL_FUNCTION_EX_NON_PY_GENERAL +
            _CHECK_PERIODIC_AT_END;

        macro(INSTRUMENTED_CALL_FUNCTION_EX) =
            unused/1 +
            _MAKE_CALLARGS_A_TUPLE +
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC_AT_END;

        op(_MAKE_FUNCTION, (codeobj_st -- func, co)) {
            PyObject *codeobj = PyValue_AsKnownObjectBorrow(codeobj_st);

            PyFunctionObject *func_obj = (PyFunctionObject *)
                PyFunction_New(codeobj, GLOBALS());

            if (func_obj == NULL) {
                ERROR_NO_POP();
            }
            co = codeobj_st;
            DEAD(codeobj_st);

            // The function's own reference: PyFunction_New can run code
            _PyFunction_SetVersion(
                func_obj, ((PyCodeObject *)func_obj->func_code)->co_version);
            func = PyValue_FromObjectSteal((PyObject *)func_obj);
        }

        macro(MAKE_FUNCTION) = _MAKE_FUNCTION + POP_TOP;

        inst(SET_FUNCTION_ATTRIBUTE, (attr_st, func_in -- func_out)) {
            PyObject *func = PyValue_AsKnownObjectBorrow(func_in);
            PyObject *attr = PyValue_AsKnownObjectSteal(attr_st);
            func_out = TAKE(func_in);
            assert(PyFunction_Check(func));
            size_t offset = _Py_FunctionAttributeOffsets[oparg];
            assert(offset != 0);
            PyObject **ptr = (PyObject **)(((char *)func) + offset);
            assert(*ptr == NULL);
            *ptr = attr;
        }

        window inst(RETURN_GENERATOR, (-- res)) {
            PyFunctionObject *func = _PyFrame_GetFunction(frame);
            PyGenObject *gen = (PyGenObject *)_Py_MakeCoro(
                func, _PyFrame_GetCode(frame));
            ERROR_IF(gen == NULL);
            SAVE_STACK();
            _PyInterpreterFrame *gen_frame = &gen->gi_iframe;
            IR_CREATED_COPY(frame, gen_frame);
            // The value of the first send takes the slot of res
            gen_frame->return_slot = (uint16_t)IR_DST_SLOT;
            assert(frame->frame_obj == NULL);
            gen->gi_frame_state = FRAME_CREATED;
            gen_frame->owner = FRAME_OWNED_BY_GENERATOR;
            _Py_LeaveRecursiveCallPy(tstate);
            _PyInterpreterFrame *prev = frame->previous;
            _PyThreadState_UpdateLastProfiledFrame(tstate, frame, prev);
            _PyThreadState_PopFrame(tstate, frame);
            frame = tstate->current_frame = prev;
            next_instr = frame->return_pc;
            RELOAD_STACK();
            res = PyValue_FromObjectSteal((PyObject *)gen);
            LLTRACE_RESUME_FRAME();
        }

        inst(BUILD_SLICE, (args[oparg] -- slice)) {
            _PyValue_RecheckWindow(args, oparg);
            // The bounds are stored as the values they are: no box of an
            // inline one.
            PyObject *slice_o = _PySlice_FromValues(
                args[0], args[1], oparg == 3 ? args[2] : PyValue_NULL);
            DECREF_INPUTS();
            ERROR_IF(slice_o == NULL);
            slice = PyValue_FromObjectSteal(slice_o);
        }

        inst(CONVERT_VALUE, (value -- result)) {
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

        inline_ok inst(FORMAT_SIMPLE, (value -- res)) {
            /* If value is a unicode object, then we know the result
             * of format(value) is value itself. An inline int or float
             * formats as it prints: format(x, "") is str(x). */
            if (PyValue_IsInline(value)) {
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

        inst(FORMAT_WITH_SPEC, (value, fmt_spec -- res)) {
            PyObject *res_o = _PyValue_Format(value, fmt_spec);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyValue_FromObjectSteal(res_o);
        }

        pure inst(COPY, (bottom:nullable, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
            top = PyValue_XDUP(bottom);
        }

        specializing op(_SPECIALIZE_BINARY_OP, (lhs, rhs -- lhs, rhs)) {
            #if ENABLE_SPECIALIZATION
            _PyFbHeader *feedback = FB_COUNTER();
            if (ADAPTIVE_COUNTER_TRIGGERS(feedback)) {
                next_instr = this_instr;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, LOCALS_ARRAY);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(BINARY_OP);
            ADVANCE_ADAPTIVE_COUNTER(feedback);
            #endif  /* ENABLE_SPECIALIZATION */
            assert(NB_ADD <= oparg);
            assert(oparg <= NB_OPARG_LAST);
        }

        inline_ok op(_BINARY_OP, (lhs, rhs -- res)) {
            IR_BINARY_READER(lhs, rhs, res);
        }

        macro(BINARY_OP) = unused/1 + _SPECIALIZE_BINARY_OP + unused/4 + _BINARY_OP;

        pure inst(SWAP, (bottom:nullable, unused[oparg-2], top:nullable --
                    bottom, unused[oparg-2], top)) {
            PyValue temp = bottom;
            bottom = top;
            top = temp;
        }

        inst(INSTRUMENTED_LINE, ( -- )) {
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
            if (_PyFb_HasEntry(original_opcode)) {
                /* Prevent the underlying instruction from specializing
                * and overwriting the instrumentation. */
                PAUSE_ADAPTIVE_COUNTER(FB_HEADER_OF(next_instr, original_opcode));
            }
            opcode = ORIGINAL_HANDLER(original_opcode);
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }

        inst(INSTRUMENTED_INSTRUCTION, ( -- )) {
            int next_opcode = _Py_call_instrumentation_instruction(
                tstate, frame, this_instr);
            ERROR_IF(next_opcode < 0);
            next_instr = this_instr;
            if (_PyFb_HasEntry(next_opcode)) {
                PAUSE_ADAPTIVE_COUNTER(FB_HEADER_OF(next_instr, next_opcode));
            }
            assert(next_opcode > 0 && next_opcode < 256);
            opcode = ORIGINAL_HANDLER(next_opcode);
            DISPATCH_GOTO();
        }

        inst(INSTRUMENTED_JUMP_FORWARD, ( -- )) {
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg),
                PY_MONITORING_EVENT_JUMP, _PyIR_EDGE_TAKEN);
        }

        op(_MONITOR_JUMP_BACKWARD, (-- )) {
            IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg),
                PY_MONITORING_EVENT_JUMP, _PyIR_EDGE_TAKEN);
        }

        macro(INSTRUMENTED_JUMP_BACKWARD) =
            unused/1 +
            _CHECK_PERIODIC +
            _MONITOR_JUMP_BACKWARD;

        op(_INSTRUMENTED_POP_JUMP_IF_TRUE, (cond -- )) {
            assert(PyValue_BoolCheck(cond));
            int jump = PyValue_IsTrue(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(FB_BRANCH(), jump);
            if (jump) {
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        op(_INSTRUMENTED_POP_JUMP_IF_FALSE, (cond -- )) {
            assert(PyValue_BoolCheck(cond));
            int jump = PyValue_IsFalse(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(FB_BRANCH(), jump);
            if (jump) {
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        macro(INSTRUMENTED_POP_JUMP_IF_TRUE) =
            unused/1 + unused/2 + _TO_BOOL + _INSTRUMENTED_POP_JUMP_IF_TRUE;

        macro(INSTRUMENTED_POP_JUMP_IF_FALSE) =
            unused/1 + unused/2 + _TO_BOOL + _INSTRUMENTED_POP_JUMP_IF_FALSE;

        inst(INSTRUMENTED_POP_JUMP_IF_NONE, (unused/1, value -- )) {
            int jump = PyValue_IsNone(value);
            RECORD_BRANCH_TAKEN(FB_BRANCH(), jump);
            if (jump) {
                DEAD(value);
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                PyValue_CLOSE(value);
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        inst(INSTRUMENTED_JUMP_IF_FALSE_OR_POP, (cond -- res)) {
            // Taken before the truth test, which can run code
            PyValue held = TAKE(cond);
            int truth = _PyValue_IsTrue(held);
            if (truth < 0) {
                PyValue_CLOSE(held);
                ERROR_NO_POP();
            }
            if (!truth) {
                res = held;
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                IR_DEFER_CLOSE(held);
                res = PyValue_NULL;
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        inst(INSTRUMENTED_JUMP_IF_TRUE_OR_POP, (cond -- res)) {
            // Taken before the truth test, which can run code
            PyValue held = TAKE(cond);
            int truth = _PyValue_IsTrue(held);
            if (truth < 0) {
                PyValue_CLOSE(held);
                ERROR_NO_POP();
            }
            if (truth) {
                res = held;
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                IR_DEFER_CLOSE(held);
                res = PyValue_NULL;
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        inst(INSTRUMENTED_POP_JUMP_IF_NOT_NONE, (unused/1, value -- )) {
            int jump = !PyValue_IsNone(value);
            RECORD_BRANCH_TAKEN(FB_BRANCH(), jump);
            if (jump) {
                PyValue_CLOSE(value);
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, JUMP_TARGET(oparg), PY_MONITORING_EVENT_BRANCH_RIGHT, _PyIR_EDGE_TAKEN);
            }
            else {
                DEAD(value);
                IR_OBSERVE_COMMITTED_BRANCH(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT, _PyIR_EDGE_FALLTHROUGH);
            }
        }

        tier1 inst(CACHE, (--)) {
            assert(0 && "Executing a cache.");
            Py_FatalError("Executing a cache.");
        }

        tier1 inst(RESERVED, (--)) {
            assert(0 && "Executing RESERVED instruction.");
            Py_FatalError("Executing RESERVED instruction.");
        }

        /* Handlers of the register IR only (Include/internal/pycore_ir.h).
         * They have no opcode. */

        // The jumps that the assembler adds after a relaxed FOR_ITER or
        // SEND (Python/assemble.c), which no instruction of the stack
        // machine stands for.  They leave frame->instr_ptr at the loop, so
        // that the events after them see the loop as the instruction before.
        ir_only no_save_ip inst(IR_JUMP, ( -- )) {
            JUMPTO(oparg);
        }

        op(_SAVE_RETURN_SLOT, (--)) {
            frame->return_slot = (uint16_t)IR_DST_SLOT;
            frame->return_pc = next_instr;
        }

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
