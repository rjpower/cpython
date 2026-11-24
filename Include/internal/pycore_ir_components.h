#ifndef Py_INTERNAL_IR_COMPONENTS_H
#define Py_INTERNAL_IR_COMPONENTS_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* The components that the bodies of Python/bytecodes.c call, with their
 * declared contracts: the definition list of the register IR's handlers.
 * Tools/cases_generator/ir_contract.py reads it and verifies every uop that
 * declares a contract against the rows it calls; the generator writes
 * static checks that hold each row's C type to its definition into
 * Python/generated_ir_cases.c.h.
 *
 * Rows:
 *   PRIMITIVE(name, ret, (params), ARGS(...), result, EFFECTS(...)[, FEEDBACK(...)])
 *       a value primitive: a C function or expression macro.  One that a
 *       leaf uop calls has a model and a machine lowering (ir_leaf.py,
 *       jit_lower.c), and a model needs a lowering (V11)
 *   HELPER(name, ret, (params), ARGS(...), result, EFFECTS(...)[, FEEDBACK(...)])
 *       an out-of-line C function, called with the SysV ABI
 *   ROUTINE(name, ret, (params), ARGS(...), result, EFFECTS(...), EXITS(...)
 *           [, FEEDBACK(...)])
 *       a VM routine: a C function that may leave the word by its own
 *       exits (the evaluator's label work)
 *   MACRO(name, ARGS(...), result, EFFECTS(...), EXITS(...)[, FEEDBACK(...)])
 *       a macro that the generator expands; its C is generated
 *
 * ARGS: per parameter a mode (borrowed, consumed, taken, scalar, object,
 * tstate, frame, out) and the facts it requires.
 * result: NONE, SCALAR, VIEW (the object of argument 0), PATH(name) (a
 * pointer along a named path from argument 0: its type, its inline
 * values), MEASURE(kind) of argument 0, TEST(true facts | false facts, ...)
 * per argument, CONTEXT("reason"), VALUE(mode facts [null_iff(k)])
 * (infallible; with null_iff(k) NULL exactly where argument k is),
 * VALUE_OR_NULL(mode facts) (NULL means raised), STATUS(ZERO | NEGATIVE |
 * NONZERO) (the failure value of an int status: raised, and the `out`
 * arguments unwritten).
 * EFFECTS: reads writes allocates raises releases runs_python polls
 * switches_frame.  A `releases` may run a finalizer, which may run Python
 * and write the frame's homes (FrameLocalsProxy).
 * FEEDBACK: the feedback the row reads and writes, transitively:
 * read(f) and write(f) with f one of counter (the entry header's counter),
 * entry (every field of the site's entry, the counter included) and
 * handler (the word's handler).  A uop declares the union of its callees'
 * in its feedback(...) clause; its cache operands are reads of their
 * fields.  The vocabulary is Tools/cases_generator/ir_contract.py's.
 *
 *   DESTRUCTOR(name, callback_free)
 *       a deallocator reviewed to release nothing: a close through it
 *       `writes` (frees memory) and does not `release`
 *   INTRINSIC(name, ARGS(...), result, BUILD(gil | free_threaded | all))
 *       a form that the generator treats as the identity or erases in the
 *       named builds
 *   FIELD(struct, field, ctype, ON(path), result, EFFECTS(...))
 *       a field read through a pointer along a path (`tp->tp_version_tag`
 *       with tp the type of an operand): a MEASURE or a TEST of the
 *       operand
 *
 * Rows for C functions are trusted declarations: the static checks hold
 * their types, not their effects, to the C definitions (rd/design.md
 * 3.2).  Each row's evidence (its callees, its failure paths and its
 * callbacks, or why there are none) is the comment above it.
 *
 * The regions89 pilot declares the rows of BINARY_OP's add arms and
 * generic form, UNPACK_SEQUENCE and LOAD_ATTR_INSTANCE_VALUE; the rest of
 * the handlers keep the generator's inferred records until the migration
 * (G0b) seeds them. */

/* Values and numbers */
#define _PyIR_COMPONENTS_VALUES(PRIMITIVE, HELPER, MACRO) \
    /* Tag tests: bits in, no memory */ \
    PRIMITIVE(PyValue_IsNull, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(| nonnull), EFFECTS()) \
    PRIMITIVE(PyValue_IsInlineInt, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(inline_int), EFFECTS()) \
    PRIMITIVE(PyValue_IsObject, int, (PyValueBorrow), \
              ARGS(borrowed), TEST(object | inline), EFFECTS()) \
    /* Each tag test below is a comparison of the word's bits against the \
     * region bounds of pycore_value.h (the cited predicate); none reads \
     * memory.  A test with no fact of the vocabulary to declare declares \
     * none */ \
    PRIMITIVE(PyValue_IsPointerSpace, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(), EFFECTS()) \
    PRIMITIVE(PyValue_IsInline, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(inline), EFFECTS()) \
    PRIMITIVE(PyValue_IsInlineFloat, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(inline_float), EFFECTS()) \
    PRIMITIVE(PyValue_IsTaggedIndex, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(), EFFECTS()) \
    /* (a & b) >= Py_INT_BASE: exactly IsInlineInt(a) and IsInlineInt(b) \
     * (the comment above its definition in pycore_value.h: the AND has the \
     * top Py_INT_SHIFT bits set only if both words do) */ \
    PRIMITIVE(PyValue_BothInlineInt, bool, (PyValueBorrow, PyValueBorrow), \
              ARGS(borrowed, borrowed), TEST(inline_int, inline_int), EFFECTS(), \
              CONJOINS(PyValue_IsInlineInt)) \
    /* Bits compared with bits (pycore_value.h: PyValue_Is, \
     * _PyValue_NamesObject, PyValue_IsTrue, IsFalse, IsNone).  A word that \
     * is True, False or None is an object word: a static object's address \
     * with Py_TAG_REFCNT */ \
    PRIMITIVE(PyValue_Is, bool, (PyValueBorrow, PyValueBorrow), \
              ARGS(borrowed, borrowed), TEST(,), EFFECTS()) \
    PRIMITIVE(_PyValue_NamesObject, bool, (PyValueBorrow, PyObject *), \
              ARGS(borrowed, object), TEST(,), EFFECTS()) \
    PRIMITIVE(PyValue_IsTrue, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(object), EFFECTS()) \
    PRIMITIVE(PyValue_IsFalse, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(object), EFFECTS()) \
    PRIMITIVE(PyValue_IsNone, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(object), EFFECTS()) \
    /* VALUE_CHECK_FUNC(Bool) and PyValue_LongCheckExact (pycore_value.h): \
     * an object word, and its type read through ob_type.  The object check \
     * dereferences the word, so NULL is not admitted */ \
    PRIMITIVE(PyValue_BoolCheck, bool, (PyValueBorrow), \
              ARGS(borrowed nonnull), TEST(exact(PyBool_Type)), EFFECTS(reads)) \
    PRIMITIVE(PyValue_LongCheckExact, bool, (PyValueBorrow), \
              ARGS(borrowed nonnull), TEST(exact(PyLong_Type)), EFFECTS(reads)) \
    /* Inline ints: scaled values, see _PyValue_IntScaled() */ \
    PRIMITIVE(_PyValue_IntScaled, int64_t, (PyValueBorrow), \
              ARGS(borrowed inline_int), SCALAR, EFFECTS()) \
    PRIMITIVE(PyValue_InlineInt64, int64_t, (PyValueBorrow), \
              ARGS(borrowed inline_int), SCALAR, EFFECTS()) \
    PRIMITIVE(_PyValue_FromIntScaled, PyValue, (int64_t), \
              ARGS(scalar), VALUE(inline inline_int), EFFECTS()) \
    /* Inline constructors (pycore_value.h): the caller has checked the \
     * range; the result is a word with tag bits set, never NULL */ \
    PRIMITIVE(PyValue_FromInlineInt, PyValue, (int64_t), \
              ARGS(scalar), VALUE(inline inline_int), EFFECTS()) \
    PRIMITIVE(PyValue_InlineFloat, double, (PyValueBorrow), \
              ARGS(borrowed inline_float), SCALAR, EFFECTS()) \
    PRIMITIVE(PyValue_FromDoubleNotNaN, PyValue, (double), \
              ARGS(scalar), VALUE(inline inline_float), EFFECTS()) \
    /* Tagged indices (pycore_value.h): arithmetic on the word */ \
    PRIMITIVE(PyValue_TagIndex, PyValue, (intptr_t), \
              ARGS(scalar), VALUE(inline), EFFECTS()) \
    PRIMITIVE(PyValue_UntagIndex, intptr_t, (PyValueBorrow), \
              ARGS(borrowed), SCALAR, EFFECTS()) \
    PRIMITIVE(PyValue_IncrementTaggedIndexNoOverflow, PyValue, (PyValue), \
              ARGS(consumed), VALUE(inline), EFFECTS()) \
    /* PyValue_FromInt64 (pycore_value.h): _PyValue_FromIntScaled when the \
     * int fits inline, else _PyValue_BoxInt64, which gives NULL only when \
     * its allocation fails */ \
    PRIMITIVE(PyValue_FromInt64, PyValue, (int64_t), \
              ARGS(scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
              EFFECTS(allocates, raises), SLOW(_PyValue_BoxInt64)) \
    /* A new int outside the inline range: allocates, and raises \
     * MemoryError when that fails; calls no Python */ \
    HELPER(_PyValue_BoxInt64, PyValue, (int64_t), \
           ARGS(scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
           EFFECTS(allocates, raises)) \
    /* Floats: the operands of float arithmetic are inline floats, or an \
     * inline float and an inline int; NaN results are boxed (allocates, \
     * MemoryError) */ \
    PRIMITIVE(PyValue_FloatOperandValues, bool, \
              (PyValueBorrow, PyValueBorrow, double *, double *), \
              ARGS(borrowed, borrowed, out, out), TEST(inline, inline), EFFECTS()) \
    /* A NaN is boxed by _PyValue_BoxDouble (PyValue_FromDouble in \
     * pycore_value.h), which gives NULL only when its allocation fails; any \
     * other double is an inline float */ \
    HELPER(_PyValue_BoxDouble, PyValue, (double), \
           ARGS(scalar), VALUE_OR_NULL(owned exact(PyFloat_Type)), \
           EFFECTS(allocates, raises)) \
    PRIMITIVE(PyValue_FromDouble, PyValue, (double), \
              ARGS(scalar), VALUE_OR_NULL(owned exact(PyFloat_Type)), \
              EFFECTS(allocates, raises), SLOW(_PyValue_BoxDouble)) \
    /* a * b of two words below 2**60 in magnitude: a new int outside the \
     * inline range, or NULL where allocation fails (pycore_value.h) */ \
    HELPER(_PyValue_MultiplyInt64, PyValue, (int64_t, int64_t), \
           ARGS(scalar, scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
           EFFECTS(allocates, raises)) \
    /* The close of an owned value through a named deallocator.  Its effect \
     * is the deallocator's: `writes` for a DESTRUCTOR row, `releases` for \
     * any other (ir_contract.py) */ \
    PRIMITIVE(PyValue_CLOSE_SPECIALIZED, void, (PyValue, destructor), \
              ARGS(consumed, destructor), NONE, EFFECTS()) \
    /* _PyLong_ExactDealloc (Objects/longobject.c): re-immortalizes a small \
     * int, else _Py_FREELIST_FREE or PyObject_Free; an int holds no \
     * reference, so nothing is released and no Python runs */ \
    DESTRUCTOR(_PyLong_ExactDealloc, callback_free) \
    /* _PyFloat_ExactDealloc (Objects/floatobject.c): _Py_FREELIST_FREE of \
     * the float, or PyObject_Free; a float holds no reference */ \
    DESTRUCTOR(_PyFloat_ExactDealloc, callback_free) \
    /* The generic binary operation: value_binary_op -> PyNumber_* -> an \
     * nb_* slot -> any __add__ and friends: runs Python */ \
    HELPER(value_binary_op, PyValue, (int, PyValueBorrow, PyValueBorrow), \
           ARGS(scalar, borrowed, borrowed), VALUE_OR_NULL(owned), \
           EFFECTS(runs_python))

/* Objects */
#define _PyIR_COMPONENTS_OBJECTS(PRIMITIVE, HELPER, MACRO) \
    PRIMITIVE(PyValue_AsKnownObjectBorrow, PyObject *, (PyValueBorrow), \
              ARGS(borrowed object), VIEW, EFFECTS()) \
    PRIMITIVE(_PyValue_AsKnownObjectBorrowPointer, PyObject *, (PyValueBorrow), \
              ARGS(borrowed object), VIEW, EFFECTS()) \
    /* The identity on a feedback word's bits (pycore_value.h) */ \
    PRIMITIVE(_PyValueGuarded_Load, PyValueBorrow, (_PyValueGuarded), \
              ARGS(scalar), SCALAR, EFFECTS()) \
    PRIMITIVE(PyValue_AsKnownObjectSteal, PyObject *, (PyValue), \
              ARGS(consumed object), VIEW, EFFECTS()) \
    /* The type of a value or an object, a pointer along the path `type`, \
     * and the version read through it */ \
    PRIMITIVE(PyValue_TYPE, PyTypeObject *, (PyValueBorrow), \
              ARGS(borrowed), PATH(type), EFFECTS(reads)) \
    PRIMITIVE(Py_TYPE, PyTypeObject *, (PyObject *), \
              ARGS(object), PATH(type), EFFECTS(reads)) \
    FIELD(PyTypeObject, tp_version_tag, unsigned int, ON(type), \
          MEASURE(type_version), EFFECTS(reads)) \
    /* An object's inline values (pycore_object.h: at tp_basicsize; the \
     * caller has the type's Py_TPFLAGS_INLINE_VALUES) and their flag */ \
    PRIMITIVE(_PyObject_InlineValues, PyDictValues *, (PyObject *), \
              ARGS(object), PATH(inline_values), EFFECTS(reads)) \
    FIELD(PyDictValues, valid, uint8_t, ON(inline_values), \
          TEST(has_values), EFFECTS(reads)) \
    /* The address of a value slot at a byte offset: arithmetic only */ \
    PRIMITIVE(_PyObject_ValueSlot, PyValue *, (PyObject *, size_t), \
              ARGS(object, scalar), SCALAR, EFFECTS()) \
    /* The shape of an object's inline values (pycore_dict.h: a read of \
     * _PyObject_InlineValues(obj)->shape) */ \
    PRIMITIVE(_PyObject_Shape, uint32_t, (PyObject *), \
              ARGS(object), SCALAR, EFFECTS(reads)) \
    /* The slot at an offset, a new count (pycore_dict.h): the shape guards \
     * have shown the slot filled, so the value is not NULL */ \
    PRIMITIVE(_PyObject_ShapeReadDup, PyValue, (PyObject *, uint32_t), \
              ARGS(object, scalar), VALUE(owned), EFFECTS(reads)) \
    /* The two writes of an object's inline values (pycore_dict.h) */ \
    PRIMITIVE(_PyDictValues_AddToInsertionOrder, void, (PyDictValues *, Py_ssize_t), \
              ARGS(scalar, scalar), NONE, EFFECTS(reads, writes)) \
    PRIMITIVE(_PyDictValues_SetShape, void, (PyDictValues *, uint8_t), \
              ARGS(scalar, scalar), NONE, EFFECTS(writes)) \
    /* A new owner of a value, PyValue_NULL for PyValue_NULL: a count only */ \
    PRIMITIVE(PyValue_XDUP, PyValue, (PyValueBorrow), \
              ARGS(borrowed), VALUE(owned null_iff(0)), EFFECTS()) \
    /* PyValue_DUP (pycore_value.h) is XDUP without the test for NULL: its \
     * argument is not NULL */ \
    PRIMITIVE(PyValue_DUP, PyValue, (PyValueBorrow), \
              ARGS(borrowed nonnull), VALUE(owned), EFFECTS(), \
              NONNULL_FORM(PyValue_XDUP)) \
    /* PyValue_XCLOSE and PyValue_CLOSE (pycore_value.h): the count falls, \
     * and the last count's release calls _Py_Dealloc, which may run a \
     * finalizer.  CLOSE is XCLOSE without the test for NULL; it declares \
     * no NONNULL_FORM, which test_columns_against_c would check by closing \
     * generated words */ \
    PRIMITIVE(PyValue_XCLOSE, void, (PyValue), \
              ARGS(consumed), NONE, EFFECTS(releases)) \
    PRIMITIVE(PyValue_CLOSE, void, (PyValue), \
              ARGS(consumed nonnull), NONE, EFFECTS(releases)) \
    /* Identities: a borrowed view of a value; in a build with the GIL the \
     * relaxed atomic loads are plain loads */ \
    INTRINSIC(PyValue_Borrow, ARGS(borrowed), VIEW, BUILD(all)) \
    INTRINSIC(FT_ATOMIC_LOAD_UINT_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    INTRINSIC(FT_ATOMIC_LOAD_UINT8, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* _PyErr_Format (Python/errors.c) -> _PyErr_FormatV: _PyErr_Clear \
     * releases the current exception (a finalizer); PyUnicode_FromFormatV \
     * allocates the message; _PyErr_SetObject -> _PyErr_CreateException \
     * calls the exception type, which runs Python for a class defined in \
     * Python, and chains the context (releases).  The row admits any \
     * type, so it declares runs_python */ \
    HELPER(_PyErr_Format, PyObject *, (PyThreadState *, PyObject *, const char *, ...), \
           ARGS(tstate, object, scalar), NONE, EFFECTS(runs_python))

/* Containers */
#define _PyIR_COMPONENTS_CONTAINERS(PRIMITIVE, HELPER, MACRO) \
    PRIMITIVE(PyList_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyList_Type)), EFFECTS(reads)) \
    PRIMITIVE(PyTuple_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyTuple_Type)), EFFECTS(reads)) \
    /* Py_IS_TYPE of the named type: a read of ob_type */ \
    PRIMITIVE(PyDict_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyDict_Type)), EFFECTS(reads)) \
    PRIMITIVE(PyModule_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyModule_Type)), EFFECTS(reads)) \
    PRIMITIVE(PyUnicode_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyUnicode_Type)), EFFECTS(reads)) \
    PRIMITIVE(PyFunction_Check, int, (PyObject *), \
              ARGS(object), TEST(exact(PyFunction_Type)), EFFECTS(reads)) \
    /* An element of a list or tuple: a borrow rooted by the container \
     * (pycore_list.h, pycore_tuple.h).  A tuple under construction holds \
     * NULL, so no fact is declared of it */ \
    PRIMITIVE(PyList_GET_VALUE, PyValueBorrow, (PyObject *, Py_ssize_t), \
              ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    PRIMITIVE(PyTuple_GET_VALUE, PyValueBorrow, (PyObject *, Py_ssize_t), \
              ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    /* pycore_long.h: the sign bits of lv_tag */ \
    PRIMITIVE(_PyLong_IsZero, bool, (const PyLongObject *), \
              ARGS(object), TEST(), EFFECTS(reads)) \
    /* pycore_dict.h: the address of the entries after the index table, \
     * from dk_log2_index_bytes */ \
    PRIMITIVE(DK_UNICODE_ENTRIES, PyDictUnicodeEntry *, (PyDictKeysObject *), \
              ARGS(scalar), SCALAR, EFFECTS(reads)) \
    PRIMITIVE(PyList_GET_SIZE, Py_ssize_t, (PyObject *), \
              ARGS(object), MEASURE(length), EFFECTS(reads)) \
    PRIMITIVE(PyTuple_GET_SIZE, Py_ssize_t, (PyObject *), \
              ARGS(object), MEASURE(length), EFFECTS(reads)) \
    PRIMITIVE(_PyList_ITEMS, PyValue *, (PyObject *), \
              ARGS(object), SCALAR, EFFECTS(reads)) \
    PRIMITIVE(_PyTuple_ITEMS, PyValue *, (PyObject *), \
              ARGS(object), SCALAR, EFFECTS(reads)) \
    /* The elements of a sequence into the arm's own outputs, the \
     * destination homes (publication, not an effect); the homes' displaced \
     * values are closed by the skeleton after the uop (IR_CLOSE_WINDOW) */ \
    MACRO(IR_UNPACK_ITEMS, ARGS(out, scalar, scalar), NONE, EFFECTS(reads), \
          EXITS()) \
    /* The generic unpack (Python/ceval.c): _PyValue_GetIter and the \
     * iterator's __next__ run Python.  It releases v on every return (Done: \
     * Py_DECREF(v)); it returns 0 with an exception set, having closed the \
     * values it wrote below sp (Error:), else 1 with argcnt values (and the \
     * list for argcntafter >= 0) written below sp */ \
    HELPER(_PyEval_UnpackIterableValue, int, \
           (PyThreadState *, PyObject *, int, int, PyValue *), \
           ARGS(tstate, consumed, scalar, scalar, out), STATUS(ZERO), \
           EFFECTS(runs_python)) \
    /* A free-threaded build's critical section: constant in this build */ \
    PRIMITIVE(LOCK_OBJECT, int, (PyObject *), \
              ARGS(object), CONTEXT("a free-threaded build's critical section"), \
              EFFECTS()) \
    PRIMITIVE(UNLOCK_OBJECT, void, (PyObject *), \
              ARGS(object), NONE, EFFECTS())

/* Specialization: the site's counter and the specializer */
#define _PyIR_COMPONENTS_SPECIALIZATION(PRIMITIVE, HELPER, MACRO) \
    /* The header of the word's feedback entry: an address, which each \
     * generator expands for its form (ir_generator.IR_INSTRUCTION_MACROS) */ \
    MACRO(FB_COUNTER, ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* The counter of an entry header (Python/ceval_macros.h): a test of \
     * it, and its advance; NULL has no counter */ \
    PRIMITIVE(_PyFb_CounterTriggers, int, (_PyFbHeader *), \
              ARGS(scalar), SCALAR, EFFECTS(reads), FEEDBACK(read(counter))) \
    PRIMITIVE(_PyFb_AdvanceCounter, void, (_PyFbHeader *), \
              ARGS(scalar), NONE, EFFECTS(writes), FEEDBACK(write(counter))) \
    /* Python/ceval_macros.h: a load and test of tstate->eval_breaker */ \
    PRIMITIVE(_Py_EvalBreakerHasEvents, bool, (PyThreadState *), \
              ARGS(tstate), CONTEXT("the thread's eval breaker"), EFFECTS(reads)) \
    /* _Py_HandlePending (Python/ceval_gil.c): stop-the-world, signal \
     * handlers and pending calls (Python code), the collector (finalizers); \
     * -1 with an exception set */ \
    HELPER(_Py_HandlePending, int, (PyThreadState *), \
           ARGS(tstate), STATUS(NEGATIVE), \
           EFFECTS(reads, writes, allocates, raises, releases, runs_python, polls)) \
    /* The specializers rewrite the word's handler and its feedback entry \
     * from the operands (Python/specialize.c).  Not reviewed: the effects \
     * are today's inference (all of them).  Their transitive callees \
     * include feedback-table allocation and, for BINARY_OP_SUBSCR_GETITEM, \
     * specialize_subscr_getitem -> prepare_function_for_inline -> \
     * _PyCode_GetExec (allocation, lowering and its failure handling); \
     * G0c reviews them with that evidence */ \
    HELPER(_Py_Specialize_BinaryOp, void, \
           (PyValueBorrow, PyValueBorrow, _PyInstr *, int, PyValue *), \
           ARGS(borrowed, borrowed, scalar, scalar, scalar), NONE, \
           EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    HELPER(_Py_Specialize_UnpackSequence, void, (PyValue, _PyInstr *, int), \
           ARGS(borrowed, scalar, scalar), NONE, EFFECTS(runs_python), \
           FEEDBACK(read(entry), write(entry), write(handler)))

/* The evaluator's labels (bytecodes.c label(...)): the routines behind them */
#define _PyIR_COMPONENTS_LABELS(PRIMITIVE, HELPER, MACRO) \
    /* error, exception_unwind, exit_unwind, exit_unwind_notrace and \
     * throw_error enter it through IR_PROCESS_ERROR (Python/ceval.c).  It \
     * records the traceback and runs the raise, handled and unwind \
     * monitoring callbacks (Python), clears the frame's scratch homes \
     * (releases), pops frames that do not handle the exception \
     * (switches_frame), and either returns the handler's word in the frame \
     * that handles it (jump) or leaves the evaluator (exit) */ \
    ROUTINE(_PyEval_ProcessError, _PyIRErrorResult, \
            (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, uint32_t), \
            ARGS(tstate, frame, scalar, scalar), SCALAR, \
            EFFECTS(runs_python, switches_frame), EXITS(jump, exit)) \
    /* start_frame (Python/ceval_macros.h): the recursion check, which \
     * counts down the thread's remaining depth; nonzero \
     * with RecursionError set */ \
    HELPER(_Py_EnterRecursivePy, int, (PyThreadState *), \
           ARGS(tstate), STATUS(NONZERO), EFFECTS(writes, allocates, raises))

/* The register IR's transaction macros (IR_*) that the uop bodies call,
 * which the generator expands or ceval_macros.h and pycore_ir.h define.
 * Written from today's inference tables (analyzer.IR_MARKER_EFFECTS and
 * IR_HELPER_EFFECTS, ir_records.TRANSFER_TOKENS and GUARD_TOKENS) and the
 * definitions, taking the larger declaration where they leave a choice;
 * G0c reviews them with evidence.  "escapes" is runs_python, "error" is
 * raises with a raise exit, "frame" is switches_frame. */
#define _PyIR_COMPONENTS_TRANSACTIONS(PRIMITIVE, HELPER, MACRO) \
    /* Word-field and frame accessors: C macros of Include/internal/pycore_ir.h \
     * and Python/ceval_macros.h that read the word, the frame or the \
     * activation, and write nothing */ \
    MACRO(IR_ACTIVATION, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* Frame fields of Python/ceval_macros.h: GLOBALS() is frame->f_globals, \
     * BUILTINS() frame->f_builtins, FRAME_CO_CONSTS the code object's \
     * constants, IR_SLOTS() frame->base (an address, fixed for the \
     * activation), GETLOCAL(i) the home IR_SLOTS()[i] */ \
    MACRO(GLOBALS, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(BUILTINS, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(FRAME_CO_CONSTS, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_SLOTS, ARGS(), SCALAR, EFFECTS(), EXITS()) \
    MACRO(GETLOCAL, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_C, ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    MACRO(IR_D, ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    MACRO(IR_HANDLER, ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    MACRO(IR_SLOT, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_OPERAND, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_CALL_ARG, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_CALL_FIRST, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_IS_LOCAL, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(IR_FUSED_DISTANCE, ARGS(), SCALAR, EFFECTS(), EXITS()) \
    MACRO(IR_FUSED_IF_FALSE, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* IR_REPORT_EDGE (ceval_macros.h): ((void)(TAKEN)) in the interpreter */ \
    MACRO(IR_REPORT_EDGE, ARGS(scalar), NONE, EFFECTS(), EXITS()) \
    /* IR_METHOD_POLY_LOOKUP (ceval_macros.h): method_poly_lookup() reads the \
     * polymorphic entry's payload and loads a guarded class value; NULL is \
     * a miss that the uop tests (no raise).  Result is the borrowed method */ \
    MACRO(IR_METHOD_POLY_LOOKUP, ARGS(scalar, object), \
          VALUE(borrowed), EFFECTS(reads), EXITS()) \
    /* Markers that emit no code or a fixed statement \
     * (ir_generator.py: generator_marker, metadata_passthrough, call_effects, \
     * and the prepare_call no-op; IR_MARKER_EFFECTS has an empty set for \
     * IR_BEGIN_CALL_EFFECTS) */ \
    MACRO(IR_GENERATOR_MARKER, ARGS(), NONE, EFFECTS(), EXITS()) \
    MACRO(IR_METADATA_PASSTHROUGH, ARGS(scalar), NONE, EFFECTS(), EXITS()) \
    MACRO(IR_BEGIN_CALL_EFFECTS, ARGS(), NONE, EFFECTS(), EXITS()) \
    /* IR_SKIP_RESUME (ceval_macros.h): reads the eval breaker, advances \
     * next_instr over a no-op RESUME; no table entry */ \
    MACRO(IR_SKIP_RESUME, ARGS(), NONE, EFFECTS(reads, writes), EXITS()) \
    /* IR_JIT_EVENT (ceval_macros.h): counts heat, may call _PyJit_Event and \
     * jit_run through IR_STEP_DONE, which runs the unit, which runs Python, \
     * and dispatches or leaves the evaluator; no table entry, so the larger \
     * declaration */ \
    MACRO(IR_JIT_EVENT, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python, \
                  switches_frame), EXITS(enter, return, raise, exit)) \
    /* Guards: ir_records.GUARD_TOKENS maps each to EXIT_GUARD (miss); \
     * IR_HELPER_EFFECTS gives IR_GUARD_INLINE_INT, IR_GUARD_ELEMENT_TYPE, \
     * IR_GUARD_ELEMENT_INDEX and IR_DUP_CANONICAL_ELEMENT an empty set.  \
     * The element index is a variable that the macro declares */ \
    MACRO(IR_GUARD_INLINE_INT, ARGS(borrowed), NONE, EFFECTS(reads), \
          EXITS(miss)) \
    MACRO(IR_GUARD_ELEMENT_TYPE, ARGS(borrowed, scalar), NONE, \
          EFFECTS(reads), EXITS(miss)) \
    MACRO(IR_GUARD_ELEMENT_INDEX, ARGS(borrowed, borrowed, scalar, scalar), \
          NONE, EFFECTS(reads), EXITS(miss)) \
    /* GUARD_TOKENS: IR_GUARD_SHAPE_READ is an EXIT_GUARD; its owner is a \
     * PyObject * (PyValue_AsKnownObjectBorrow), the rest scalars */ \
    MACRO(IR_GUARD_SHAPE_READ, ARGS(object, scalar, scalar), NONE, \
          EFFECTS(reads), EXITS(miss)) \
    /* Duplicating reads (ir_generator.py: shape_read_value, \
     * IR_DUP_CANONICAL_ELEMENT in IR_HELPER_EFFECTS with an empty set): a \
     * new reference to an element or a slot's value, no Python */ \
    MACRO(IR_DUP_CANONICAL_ELEMENT, ARGS(borrowed, scalar), VALUE(owned), \
          EFFECTS(reads), EXITS()) \
    MACRO(IR_DUP_SHAPE_READ, ARGS(borrowed, scalar), VALUE(owned), \
          EFFECTS(reads), EXITS()) \
    /* Home arrays (ir_generator.py: home_helper, take_input_home, \
     * close_input_homes, write_output_home).  IR_BUILD_TUPLE and \
     * IR_BUILD_LIST call _PyTuple_FromValueStealOnSuccess and its list \
     * twin: they allocate, return NULL on failure (the uop raises) and keep \
     * the values on failure (taken); IR_HELPER_EFFECTS has an empty set */ \
    MACRO(IR_BUILD_TUPLE, ARGS(taken), VALUE_OR_NULL(owned), \
          EFFECTS(reads, writes, allocates, raises), EXITS()) \
    MACRO(IR_BUILD_LIST, ARGS(taken), VALUE_OR_NULL(owned), \
          EFFECTS(reads, writes, allocates, raises), EXITS()) \
    /* _Py_BuildString_ValueSteal and _Py_BuildMap_ValueSteal: \
     * IR_HELPER_EFFECTS marks them "escapes" (they hash, compare and \
     * release, which may run Python).  The values are taken, as for the \
     * tuple */ \
    MACRO(IR_BUILD_STRING, ARGS(taken), VALUE_OR_NULL(owned), \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python), \
          EXITS()) \
    MACRO(IR_BUILD_MAP, ARGS(taken), VALUE_OR_NULL(owned), \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python), \
          EXITS()) \
    /* _PyObject_LookupSpecialMethod: 1 found, 0 missing (the uop raises), \
     * negative on error; writes the two output homes.  "escapes" in \
     * IR_HELPER_EFFECTS (a descriptor's __get__ may run Python).  The \
     * failure values are zero and negative, which no STATUS names */ \
    MACRO(IR_LOOKUP_SPECIAL, ARGS(object, out), SCALAR, \
          EFFECTS(reads, writes, allocates, raises, runs_python), EXITS()) \
    /* _PyFrame_WriteHome / _PyFrame_TakeHome: IR_HELPER_EFFECTS has an empty \
     * set for both.  A value written moves into the home; a value taken \
     * moves out of it */ \
    MACRO(IR_WRITE_OUTPUT, ARGS(out, scalar, consumed), NONE, \
          EFFECTS(writes), EXITS()) \
    MACRO(IR_TAKE_INPUT, ARGS(consumed, scalar), VALUE(owned), \
          EFFECTS(reads, writes), EXITS()) \
    /* _PyFrame_CloseHomeRange: "escapes" in IR_HELPER_EFFECTS; the close \
     * of owned values runs finalizers */ \
    MACRO(IR_CLOSE_INPUTS, ARGS(consumed), NONE, \
          EFFECTS(writes, releases, runs_python), EXITS()) \
    /* The observer's deferred close (ir_generator.py: defer_observer_input): \
     * the held value is closed after the observer; no table entry, so a \
     * release is declared */ \
    MACRO(IR_DEFER_CLOSE, ARGS(consumed), NONE, EFFECTS(writes, releases), \
          EXITS()) \
    /* The monitoring event of a committed branch: IR_MARKER_EFFECTS \
     * escapes, error; TRANSFER_TOKENS (BRANCH, RAISE).  Arguments are the \
     * instruction, the target, the event and the edge: all scalars */ \
    MACRO(IR_OBSERVE_COMMITTED_BRANCH, ARGS(scalar, scalar, scalar, scalar), \
          NONE, EFFECTS(reads, writes, raises, runs_python), \
          EXITS(branch, raise)) \
    /* IR_MARKER_EFFECTS: escapes, error, terminal; TRANSFER_TOKENS (BRANCH, \
     * RAISE).  Arguments are the kind, whether the step completed and the \
     * oparg */ \
    MACRO(IR_COMPLETE_STEP, ARGS(scalar, scalar, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python), EXITS(branch, raise)) \
    /* Errors: IR_MARKER_EFFECTS escapes, error, terminal, frame; \
     * TRANSFER_TOKENS (RAISE) only.  PROCESS_ERROR_AT with an EXIT ingress \
     * leaves the evaluator and unwinding may pop frames, so exit is added \
     * to the table's raise */ \
    MACRO(IR_PROCESS_ERROR, ARGS(scalar, scalar), NONE, \
          EFFECTS(reads, writes, raises, releases, runs_python, \
                  switches_frame), EXITS(raise, exit)) \
    /* Returns: IR_MARKER_EFFECTS escapes, error, terminal, frame; \
     * TRANSFER_TOKENS (RETURN, RAISE) for the three.  IR_RETURN_TO_CALLER \
     * moves the result to the caller; the two finish macros leave the \
     * value to the uop (it is passed on to the frame clear that follows) */ \
    MACRO(IR_RETURN_TO_CALLER, ARGS(consumed, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame), \
          EXITS(return, raise)) \
    MACRO(IR_FINISH_GENERATOR_RETURN, ARGS(borrowed, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame), \
          EXITS(return, raise)) \
    MACRO(IR_FINISH_GENERATOR_RETURN_EMPTY, ARGS(borrowed, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame), \
          EXITS(return, raise)) \
    /* IR_MARKER_EFFECTS escapes, error, terminal, frame; TRANSFER_TOKENS \
     * (ENTER_FRAME, RAISE) */ \
    MACRO(IR_ENTER_GENERATOR, ARGS(object, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame), \
          EXITS(enter, raise)) \
    /* IR_MARKER_EFFECTS escapes, terminal, frame (no error); \
     * TRANSFER_TOKENS (YIELD) */ \
    MACRO(IR_SUSPEND_DELEGATED, ARGS(consumed), NONE, \
          EFFECTS(reads, writes, runs_python, switches_frame), EXITS(yield)) \
    /* IR_MARKER_EFFECTS escapes, error; TRANSFER_TOKENS (RAISE).  A hit \
     * stores the result and dispatches, so the macro also leaves by next */ \
    MACRO(IR_TRY_DICT_FIRST_PROBE, ARGS(), NONE, \
          EFFECTS(reads, writes, raises, releases, runs_python), \
          EXITS(raise)) \
    /* Call preparation: IR_MARKER_EFFECTS escapes, error; TRANSFER_TOKENS \
     * (RAISE) */ \
    MACRO(IR_PREPARE_CALL_METHOD, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, runs_python), \
          EXITS(raise)) \
    MACRO(IR_PREPARE_CALL_TUPLE, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, runs_python), \
          EXITS(raise)) \
    MACRO(IR_MONITOR_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, raises, runs_python), EXITS(raise)) \
    MACRO(IR_REFRESH_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, raises, runs_python), EXITS(raise)) \
    /* Call invocation: IR_MARKER_EFFECTS escapes, error, terminal, frame; \
     * TRANSFER_TOKENS (ENTER_FRAME, RAISE).  The call runs the callee or \
     * enters its frame, and releases the operands */ \
    MACRO(IR_INVOKE_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python, \
                  switches_frame), EXITS(enter, raise)) \
    MACRO(IR_INVOKE_GUARDED_PYTHON_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python, \
                  switches_frame), EXITS(enter, raise))

#define _PyIR_COMPONENTS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_VALUES(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_OBJECTS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_CONTAINERS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_SPECIALIZATION(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_LABELS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_TRANSACTIONS(PRIMITIVE, HELPER, MACRO)

#endif /* !Py_INTERNAL_IR_COMPONENTS_H */
