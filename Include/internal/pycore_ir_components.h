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
    /* Inline ints: scaled values, see _PyValue_IntScaled() */ \
    PRIMITIVE(_PyValue_IntScaled, int64_t, (PyValueBorrow), \
              ARGS(borrowed inline_int), SCALAR, EFFECTS()) \
    PRIMITIVE(PyValue_InlineInt64, int64_t, (PyValueBorrow), \
              ARGS(borrowed inline_int), SCALAR, EFFECTS()) \
    PRIMITIVE(_PyValue_FromIntScaled, PyValue, (int64_t), \
              ARGS(scalar), VALUE(inline inline_int), EFFECTS()) \
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
    PRIMITIVE(PyValue_FromDouble, PyValue, (double), \
              ARGS(scalar), VALUE_OR_NULL(owned exact(PyFloat_Type)), \
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
    /* The generic binary operation: value_binary_op -> PyNumber_* -> an \
     * nb_* slot -> any __add__ and friends: runs Python */ \
    HELPER(value_binary_op, PyValue, (int, PyValueBorrow, PyValueBorrow), \
           ARGS(scalar, borrowed, borrowed), VALUE_OR_NULL(owned), \
           EFFECTS(runs_python)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyValue_AsPyObjectNew, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.predicate_facts, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_BoolCheck, bool, (PyValueBorrow), \
        ARGS(borrowed), TEST(exact(PyBool_Type)), EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyValue_CLEAR, \
        ARGS(scalar), SCALAR, EFFECTS(releases), EXITS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_CLOSE, void, (PyValue), \
        ARGS(consumed), NONE, EFFECTS(releases)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_DUP, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.FLOAT_RESULTS, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_FromDoubleNotNaN, PyValue, (double), \
        ARGS(scalar), VALUE(owned exact(PyFloat_Type)), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.INLINE_INT_RESULTS, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_FromInlineInt, PyValue, (int64_t), \
        ARGS(scalar), VALUE(inline inline_int), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.INT_RESULTS, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_FromInt64, PyValue, (int64_t), \
        ARGS(scalar), VALUE(owned exact(PyLong_Type)), EFFECTS(allocates)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_FromNonNumberSteal, PyValue, (PyObject *), \
        ARGS(object), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_FromObjectBorrow, PyValueBorrow, (PyObject *), \
        ARGS(object), VALUE(borrowed), EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_FromObjectSteal, PyValue, (PyObject *), \
        ARGS(object), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_GenCheck, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_IncrementTaggedIndexNoOverflow, PyValue, (PyValue), \
        ARGS(consumed), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_InlineFloat, double, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_Is, bool, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_IsFalse, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_IsInline, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.predicate_facts, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_IsInlineFloat, bool, (PyValueBorrow), \
        ARGS(borrowed), TEST(inline_float), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_IsNone, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_IsTrue, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.predicate_facts, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_LongCheckExact, bool, (PyValueBorrow), \
        ARGS(borrowed), TEST(exact(PyLong_Type)), EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_ObjectOrNull, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_TagIndex, PyValue, (intptr_t), \
        ARGS(scalar), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_UntagIndex, intptr_t, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_Unwrap, void *, (PyValue), \
        ARGS(consumed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyValue_Wrap, PyValue, (void *), \
        ARGS(scalar), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyValue_XCLOSE, void, (PyValue), \
        ARGS(consumed), NONE, EFFECTS(releases)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_ARITHMETIC_RIGHT_SHIFT, \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from ir_generator.VIEW_TAKES, ir_records.effect_sites */ \
    MACRO(TAKE, ARGS(consumed), VALUE(owned), EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyIR_AdoptTemp, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE(owned), EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyIR_BaseOpcode, int, (int), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_PyLong_IsZero, bool, (const PyLongObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_ASCII, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from ir_leaf.ERASED */ \
    INTRINSIC(_PyValue_CheckHome, ARGS(scalar), NONE, BUILD(gil)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Contains, int, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_DelItem, int, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Format, PyObject *, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_GetAttr, PyValue, (PyValueBorrow, PyObject *), \
        ARGS(borrowed, object), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_GetMethod, _PyStatusValue, (PyValueBorrow, PyObject *), \
        ARGS(borrowed, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_GetOptionalItem, _PyStatusValue, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Invert, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_IsInstance, int, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_IsTrue, int, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.INT_RESULTS, ir_records.effect_sites */ \
    HELPER(_PyValue_LshiftInt64, PyValue, (int64_t, int64_t), \
        ARGS(scalar, scalar), VALUE(owned exact(PyLong_Type)), EFFECTS(allocates)) \
    /* seeded from the C prototype, ir_records.INT_RESULTS, ir_records.effect_sites */ \
    HELPER(_PyValue_MultiplyInt64, PyValue, (int64_t, int64_t), \
        ARGS(scalar, scalar), VALUE(owned exact(PyLong_Type)), EFFECTS(allocates)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_PyValue_NamesObject, bool, (PyValueBorrow, PyObject *), \
        ARGS(borrowed, object), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Negative, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_PointerBorrow, PyValueBorrow, (PyObject *), \
        ARGS(object), VALUE(borrowed), EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Positive, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from ir_leaf.ERASED */ \
    INTRINSIC(_PyValue_RecheckHome, ARGS(scalar), NONE, BUILD(gil)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_PyValue_RecheckWindow, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Repr, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_RichCompare, PyValue, (PyValueBorrow, PyValueBorrow, int), \
        ARGS(borrowed, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Send, _PyStatusValue, (PyObject *, PyValueBorrow), \
        ARGS(object, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_SequenceCheck, int, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_SequenceTuple, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_SetAttr, int, (PyValueBorrow, PyObject *, PyValueBorrow), \
        ARGS(borrowed, object, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_SetItem, int, (PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Size, Py_ssize_t, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyValue_Str, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python))

/* Objects */
#define _PyIR_COMPONENTS_OBJECTS(PRIMITIVE, HELPER, MACRO) \
    PRIMITIVE(PyValue_AsKnownObjectBorrow, PyObject *, (PyValueBorrow), \
              ARGS(borrowed object), VIEW, EFFECTS()) \
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
    /* A new owner of a value, PyValue_NULL for PyValue_NULL: a count only */ \
    PRIMITIVE(PyValue_XDUP, PyValue, (PyValueBorrow), \
              ARGS(borrowed), VALUE(owned null_iff(0)), EFFECTS()) \
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
           ARGS(tstate, object, scalar), NONE, EFFECTS(runs_python)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(CALL_STAT_INC, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ceval_macros.h, ir_records.effect_sites */ \
    MACRO(PAUSE_ADAPTIVE_COUNTER, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS(), FEEDBACK(write(counter))) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyCell_NewValue, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyErr_Format, PyObject *, (PyObject *, const char *, ...), \
        ARGS(object, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyErr_GivenExceptionMatches, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyErr_SetHandledException, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS(runs_python)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyExceptionInstance_Class, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyException_GetTraceback, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyLazyImport_CheckExact, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyModule_CheckExact, \
        ARGS(scalar), TEST(exact(PyModule_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyObject_HasAttrWithError, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyType_Check, int, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyType_GenericAlloc, PyObject *, (PyTypeObject *, Py_ssize_t), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyType_HasFeature, int, (PyTypeObject *, long unsigned int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyUnicode_1BYTE_DATA, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyUnicode_CheckExact, \
        ARGS(scalar), TEST(exact(PyUnicode_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyUnicode_Concat, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(reads, allocates)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyUnicode_GET_LENGTH, Py_ssize_t, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyUnicode_IS_COMPACT_ASCII, int, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyUnicode_READ_CHAR, Py_UCS4, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_ARRAY_LENGTH, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_DECREF, \
        ARGS(scalar), SCALAR, EFFECTS(releases), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_FatalError, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(Py_INCREF, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(Py_IS_TYPE, \
        ARGS(scalar, scalar), TEST(exact(a1), ), EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_IsNone, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_NewRef, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_SET_SIZE, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_SIZE, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_XDECREF, \
        ARGS(scalar), SCALAR, EFFECTS(releases), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(Py_XINCREF, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(Py_XSETREF, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(releases), EXITS()) \
    /* seeded from ir_leaf.ERASED */ \
    INTRINSIC(QSBR_QUIESCENT_STATE, ARGS(scalar), NONE, BUILD(gil)) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(SYNC_SP, \
        ARGS(), NONE, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyCell_GetValue, PyValue, (PyCellObject *), \
        ARGS(scalar), VALUE(owned), EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyCell_SetValue, void, (PyCellObject *, PyValue), \
        ARGS(scalar, consumed), NONE, EFFECTS(writes, releases)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyCell_SwapValue, PyValue, (PyCellObject *, PyValue), \
        ARGS(scalar, consumed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyCode_GetOriginalOpcode, uint8_t, (_PyCoLineInstrumentationData *, int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyCode_InstrHandler, int, (PyCodeObject *, int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_ChainExceptions1, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_Clear, void, (PyThreadState *), \
        ARGS(tstate), NONE, EFFECTS(releases)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_ExceptionMatches, int, (PyThreadState *, PyObject *), \
        ARGS(tstate, object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_GetRaisedException, PyObject *, (PyThreadState *), \
        ARGS(tstate), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_Occurred, PyObject *, (PyThreadState *), \
        ARGS(tstate), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_SetRaisedException, void, (PyThreadState *, PyObject *), \
        ARGS(tstate, object), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyErr_SetString, void, (PyThreadState *, PyObject *, const char *), \
        ARGS(tstate, object, scalar), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyInterpolation_BuildValue, PyObject *, (PyValueBorrow, PyObject *, int, PyObject *), \
        ARGS(borrowed, object, scalar, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from a definition under Py_GIL_DISABLED */ \
    INTRINSIC(_PyObject_GC_IS_SHARED, ARGS(scalar), SCALAR, BUILD(free_threaded)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyObject_GetManagedDict, PyDictObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyObject_IsUniquelyReferenced, int, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_PyObject_Shape, uint32_t, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyShape_FindTransition, uint8_t, (PyTypeObject *, uint8_t, Py_ssize_t), \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyShape_TransitionMissing, bool, (PyTypeObject *, uint8_t, Py_ssize_t), \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PySuper_LookupValue, _PyStatusValue, (PyTypeObject *, PyValueBorrow, PyObject *, int), \
        ARGS(scalar, borrowed, object, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyTemplate_Build, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyType_IteratesByIndex, int, (PyTypeObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_PyType_VALUE_SLOT, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyUnicode_Equal, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_PyValueGuarded_Load, PyValueBorrow, (_PyValueGuarded), \
        ARGS(scalar), VALUE(borrowed), EFFECTS()) \
    /* seeded from ir_leaf.ERASED */ \
    INTRINSIC(_Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY, ARGS(), NONE, BUILD(gil)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_Py_DECREF_SPECIALIZED, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(writes), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_HandlePending, int, (PyThreadState *), \
        ARGS(tstate), SCALAR, EFFECTS(runs_python)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_Py_ID, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a definition under Py_GIL_DISABLED */ \
    INTRINSIC(_Py_IsOwnedByCurrentThread, ARGS(scalar), SCALAR, BUILD(free_threaded)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_Py_SINGLETON, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_Py_STR, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(do_raise, int, (PyThreadState *, PyObject *, PyObject *, bool), \
        ARGS(tstate, object, object, scalar), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(native_call_builtin_o, PyValue, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValue, PyValue *), \
        ARGS(tstate, frame, scalar, consumed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(super_call, PyValue, (PyValueBorrow, PyValueBorrow, PyValue, int), \
        ARGS(borrowed, borrowed, consumed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_dict_merge_uniq, _PyStatusObject, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_exception_group_match, _PyObjectPair, (_PyInterpreterFrame *, PyObject *, PyObject *), \
        ARGS(frame, object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_import_level, int, (PyValue), \
        ARGS(consumed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_load_from_dict, _PyStatusValue, (PyThreadState *, PyObject *, PyObject *), \
        ARGS(tstate, object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_load_global, PyValue, (PyObject *, PyObject *, PyObject *), \
        ARGS(object, object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_subscript, PyValue, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(value_unicode_append, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(with_exit_call, PyValue, (PyValueBorrow, PyValueBorrow, PyObject *, PyObject *, PyObject *), \
        ARGS(borrowed, borrowed, object, object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from ir_records.NOT_RELEASES */ \
    DESTRUCTOR(_PyUnicode_ExactDealloc, callback_free)

/* Containers */
#define _PyIR_COMPONENTS_CONTAINERS(PRIMITIVE, HELPER, MACRO) \
    PRIMITIVE(PyList_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyList_Type)), EFFECTS(reads)) \
    PRIMITIVE(PyTuple_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyTuple_Type)), EFFECTS(reads)) \
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
              ARGS(object), NONE, EFFECTS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(DK_UNICODE_ENTRIES, PyDictUnicodeEntry *, (PyDictKeysObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(GETITEM, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyAnyDict_CheckExact, \
        ARGS(scalar), TEST(exact(PyAnyDict_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyAnySet_CheckExact, \
        ARGS(scalar), TEST(exact(PyAnySet_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyDict_CheckExact, \
        ARGS(scalar), TEST(exact(PyDict_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyDict_New, PyObject *, (), \
        ARGS(), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyDict_PopValue, int, (PyObject *, PyValueBorrow, PyValue *), \
        ARGS(object, borrowed, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyDict_SetValue, int, (PyObject *, PyValueBorrow, PyValueBorrow), \
        ARGS(object, borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyDict_Update, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyList_GET_VALUE, PyValueBorrow, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), VALUE(borrowed), EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PySet_New, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PySlice_Check, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(PyTuple_GET_VALUE, PyValueBorrow, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), VALUE(borrowed), EFFECTS(reads)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_PyDictValues_AddToInsertionOrder, void, (PyDictValues *, Py_ssize_t), \
        ARGS(scalar, scalar), NONE, EFFECTS(reads, writes)) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_PyDictValues_SetShape, void, (PyDictValues *, uint8_t), \
        ARGS(scalar, scalar), NONE, EFFECTS(reads, writes)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyDict_ContainsValue, int, (PyDictObject *, PyValueBorrow), \
        ARGS(scalar, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyDict_SetValue, int, (PyDictObject *, PyValueBorrow, PyValueBorrow), \
        ARGS(scalar, borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyDict_SetValue_Take2, int, (PyDictObject *, PyValue, PyValue), \
        ARGS(scalar, consumed, consumed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyList_AppendTakeValue, int, (PyListObject *, PyValue), \
        ARGS(scalar, consumed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyList_Extend, PyObject *, (PyListObject *, PyObject *), \
        ARGS(scalar, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyList_SliceSubscript, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PySet_AddValueTakeRef, int, (PySetObject *, PyValue), \
        ARGS(scalar, consumed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PySet_Contains, int, (PySetObject *, PyValueBorrow), \
        ARGS(scalar, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PySet_Update, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PySlice_FromValues, PyObject *, (PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyTuple_GET_OBJECT, PyObject *, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), SCALAR, EFFECTS(reads))

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
           FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(FB_HEADER_OF, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(ORIGINAL_HANDLER, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyFb_HasEntry, int, (int), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_Call, void, (PyValue, PyValue, _PyInstr *, int), \
        ARGS(borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_CallFunctionEx, void, (PyValue, PyValue, _PyInstr *), \
        ARGS(borrowed, borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_CallKw, void, (PyValue, PyValue, PyValue, _PyInstr *, int), \
        ARGS(borrowed, borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_CompareOp, void, (PyValueBorrow, PyValueBorrow, _PyInstr *, int), \
        ARGS(borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_ContainsOp, void, (PyValueBorrow, PyValueBorrow, _PyInstr *), \
        ARGS(borrowed, borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_ForIter, void, (PyValue, PyValue, _PyInstr *, int), \
        ARGS(borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_GetIter, void, (PyValue, _PyInstr *), \
        ARGS(borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_LoadAttr, void, (PyValueBorrow, _PyInstr *, PyObject *), \
        ARGS(borrowed, scalar, object), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_LoadGlobal, void, (PyObject *, PyObject *, _PyInstr *, PyObject *), \
        ARGS(object, object, scalar, object), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_LoadSuperAttr, void, (PyValue, PyValue, _PyInstr *, int), \
        ARGS(borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_PopJumpIf, void, (PyValueBorrow, _PyInstr *), \
        ARGS(borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_Send, void, (PyValue, _PyInstr *), \
        ARGS(borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_StoreAttr, void, (PyValueBorrow, _PyInstr *, PyObject *), \
        ARGS(borrowed, scalar, object), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_StoreSubscr, void, (PyValueBorrow, PyValueBorrow, _PyInstr *), \
        ARGS(borrowed, borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* seeded from the C prototype, the specializing annotation, G0a's specializer rows, ir_records.effect_sites */ \
    HELPER(_Py_Specialize_ToBool, void, (PyValue, _PyInstr *), \
        ARGS(borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler)))

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

/* Calls and frames */
#define _PyIR_COMPONENTS_CALLS(PRIMITIVE, HELPER, MACRO) \
    /* seeded from a C macro (no static check), ir_records.CONTEXT_PREDICATES, ir_records.effect_sites */ \
    MACRO(IS_PEP523_HOOKED, \
        ARGS(scalar), CONTEXT("a PEP 523 frame evaluator may replace the call"), EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyCFunction_CheckExact, \
        ARGS(scalar), TEST(exact(PyCFunction_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyCFunction_GET_FLAGS, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyFunction_Check, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyFunction_GET_CODE, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(PyFunction_New, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_FrameClearAndPopGeneralWithResult, PyValue, (PyThreadState *, _PyInterpreterFrame *, PyValue), \
        ARGS(tstate, frame, consumed), VALUE(owned), EFFECTS(releases)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyFrame_GetCode, PyCodeObject *, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyFrame_GetFunction, PyFunctionObject *, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyFrame_MaterializeKwargs, int, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyFrame_PushUnchecked, _PyInterpreterFrame *, (PyThreadState *, PyValue, _PyInterpreterFrame *), \
        ARGS(tstate, consumed, frame), SCALAR, EFFECTS(allocates)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyFunction_SetVersion, void, (PyFunctionObject *, uint32_t), \
        ARGS(scalar, scalar), NONE, EFFECTS(reads, writes)) \
    /* seeded from the C prototype, ir_records.CONTEXT_PREDICATES, ir_records.effect_sites */ \
    HELPER(_PyThreadState_HasStackSpace, bool, (PyThreadState *, int), \
        ARGS(tstate, scalar), CONTEXT("the data stack must hold the callee's frame"), EFFECTS(reads)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(_PyThreadState_UpdateLastProfiledFrame, \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(runs_python), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_LeaveRecursiveCallPy, void, (PyThreadState *), \
        ARGS(tstate), NONE, EFFECTS(reads, writes)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_LeaveRecursiveCallTstate, void, (PyThreadState *), \
        ARGS(tstate), NONE, EFFECTS(reads, writes)) \
    /* seeded from the C prototype, ir_records.CONTEXT_PREDICATES, ir_records.effect_sites */ \
    HELPER(_Py_ReachedRecursionLimit, int, (PyThreadState *), \
        ARGS(tstate), CONTEXT("the C recursion limit"), EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(call_builtin_values, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, consumed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(call_class_values, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, consumed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(call_descriptor_values, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, consumed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(call_push_exact, _PyInterpreterFrame *, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, consumed, consumed, scalar), SCALAR, EFFECTS(allocates)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(call_vector, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, PyValue), \
        ARGS(tstate, frame, scalar, consumed), VALUE_OR_NULL(owned), EFFECTS(runs_python))

/* Generators, coroutines and iteration */
#define _PyIR_COMPONENTS_GENERATORS(PRIMITIVE, HELPER, MACRO) \
    /* seeded from a C macro (no static check), ir_records.predicate_facts, ir_records.effect_sites */ \
    MACRO(PyAsyncGenASend_CheckExact, \
        ARGS(scalar), TEST(exact(PyAsyncGenASend_Type)), EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyCoro_CheckExact, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PyGen_Check, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_GetANext, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_GetAwaitable, PyObject *, (PyValueBorrow, int), \
        ARGS(borrowed, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyForIter_StopIsExhaustion, int, (PyThreadState *, _PyInterpreterFrame *), \
        ARGS(tstate, frame), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyForIter_VirtualIteratorNext, _PyStatusValue, (PyThreadState *, _PyInterpreterFrame *, PyValue, PyValue *), \
        ARGS(tstate, frame, consumed, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyForIter_VirtualIteratorNextInline, _PyStatusValue, (PyThreadState *, _PyInterpreterFrame *, PyValue, PyValue *), \
        ARGS(tstate, frame, consumed, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyGen_GetGeneratorFromFrame, PyGenObject *, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(coro_awaitable_can_transfer, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(gen_claim, int, (PyThreadState *, PyGenObject *), \
        ARGS(tstate, scalar), SCALAR, EFFECTS(reads, writes)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(gen_may_claim, bool, (PyGenObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(gen_try_set_executing, bool, (PyGenObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads, writes))

/* Errors */
#define _PyIR_COMPONENTS_ERRORS(PRIMITIVE, HELPER, MACRO) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_FormatExcCheckArg, void, (PyThreadState *, PyObject *, const char *, PyObject *), \
        ARGS(tstate, object, scalar, object), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_FormatExcUnbound, void, (PyThreadState *, PyCodeObject *, int), \
        ARGS(tstate, scalar, scalar), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_FormatKwargsError, void, (PyThreadState *, PyValueBorrow, PyObject *, PyObject *), \
        ARGS(tstate, borrowed, object, object), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(monitor_reraise, void, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *), \
        ARGS(tstate, frame, scalar), NONE, EFFECTS(runs_python))

/* Monitoring and the eval breaker */
#define _PyIR_COMPONENTS_MONITORING(PRIMITIVE, HELPER, MACRO) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(DTRACE_FUNCTION_ENTRY, \
        ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(DTRACE_FUNCTION_RETURN, \
        ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(INSTRUMENTED_JUMP, \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(runs_python), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(LLTRACE_RESUME_FRAME, \
        ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_leaf.PRIMITIVES, ir_records.effect_sites */ \
    PRIMITIVE(_Py_EvalBreakerHasEvents, bool, (PyThreadState *), \
        ARGS(tstate), SCALAR, EFFECTS(reads)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_Instrument, int, (PyCodeObject *, PyInterpreterState *), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation, int, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *), \
        ARGS(tstate, scalar, frame, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation_2args, int, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, frame, scalar, borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation_arg, int, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow), \
        ARGS(tstate, scalar, frame, scalar, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation_exc2, void, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, frame, scalar, borrowed, borrowed), NONE, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation_instruction, int, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *), \
        ARGS(tstate, frame, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation_jump, _PyInstr *, (_PyInstr *, PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, _PyInstr *), \
        ARGS(scalar, tstate, scalar, frame, scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_call_instrumentation_line, int, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, _PyInstr *), \
        ARGS(tstate, frame, scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(check_periodics, int, (PyThreadState *), \
        ARGS(tstate), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(check_periodics_at_end, int, (PyThreadState *, _PyInterpreterFrame *), \
        ARGS(tstate, frame), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(monitor_stop_iteration, int, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow), \
        ARGS(tstate, frame, scalar, borrowed), SCALAR, EFFECTS(runs_python))

/* The evaluator: dispatch, the frame's namespaces, atomics */
#define _PyIR_COMPONENTS_EVALUATOR(PRIMITIVE, HELPER, MACRO) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(BUILTINS, \
        ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(COMPARISON_BIT, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.TRANSFER_TOKENS, ir_records.effect_sites */ \
    MACRO(DISPATCH, \
        ARGS(), NONE, EFFECTS(), EXITS(next)) \
    /* seeded from a C macro (no static check), ir_records.TRANSFER_TOKENS, ir_records.effect_sites */ \
    MACRO(DISPATCH_GOTO, \
        ARGS(), NONE, EFFECTS(), EXITS(jump)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(FT_ATOMIC_LOAD_INT8_RELAXED, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(FT_ATOMIC_LOAD_PTR, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from ir_leaf.IDENTITIES */ \
    INTRINSIC(FT_ATOMIC_LOAD_PTR_ACQUIRE, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(FT_ATOMIC_LOAD_PTR_RELAXED, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from ir_leaf.IDENTITIES */ \
    INTRINSIC(FT_ATOMIC_LOAD_SSIZE_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* seeded from ir_leaf.IDENTITIES */ \
    INTRINSIC(FT_ATOMIC_LOAD_UINT32_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(FT_ATOMIC_LOAD_UINT64_RELAXED, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(FT_ATOMIC_LOAD_UINTPTR_ACQUIRE, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(FT_ATOMIC_STORE_INT8_RELEASE, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads, writes), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(GETLOCAL, \
        ARGS(scalar), VALUE(owned), EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(GLOBALS, \
        ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(INSTR_AT_OFFSET, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(INSTR_OFFSET_OF, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.TRANSFER_TOKENS, ir_records.effect_sites */ \
    MACRO(JUMPBY, \
        ARGS(scalar), NONE, EFFECTS(), EXITS(jump)) \
    /* seeded from a C macro (no static check), ir_records.TRANSFER_TOKENS, ir_records.effect_sites */ \
    MACRO(JUMPTO, \
        ARGS(scalar), NONE, EFFECTS(), EXITS(jump)) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(JUMP_TARGET, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(LOAD_IP, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(LOCALS, \
        ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* seeded from a C macro (no static check), ir_records.effect_sites */ \
    MACRO(PRE_DISPATCH_GOTO, \
        ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(RELOAD_STACK, \
        ARGS(), NONE, EFFECTS(), EXITS()) \
    /* seeded from generators_common.py's expansion, ir_records.effect_sites */ \
    MACRO(SAVE_STACK, \
        ARGS(), NONE, EFFECTS(), EXITS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_BinarySlice, PyValue, (PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CallIntrinsic1, PyValue, (PyThreadState *, int, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CallIntrinsic2, PyValue, (PyThreadState *, int, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CallMethodDescriptorNoArgs, PyValue, (PyThreadState *, PyCFunction, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CallMethodDescriptorO, PyValue, (PyThreadState *, PyCFunction, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CheckExceptStarTypeValidValue, int, (PyThreadState *, PyValueBorrow), \
        ARGS(tstate, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CheckExceptTypeValidValue, int, (PyThreadState *, PyValueBorrow), \
        ARGS(tstate, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_CreateInitFrame, _PyInterpreterFrame *, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValue, PyValue, _PyInstr *), \
        ARGS(tstate, frame, scalar, consumed, consumed, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_GetAIter, PyValue, (PyThreadState *, PyValue), \
        ARGS(tstate, consumed), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_GetIter, _PyValuePair, (PyValue, int), \
        ARGS(consumed, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_ImportFromValue, PyValue, (PyThreadState *, PyValueBorrow, PyObject *), \
        ARGS(tstate, borrowed, object), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_ImportName, PyObject *, (PyThreadState *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, int), \
        ARGS(tstate, object, object, object, object, object, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_LazyImportFrom, PyObject *, (PyThreadState *, _PyInterpreterFrame *, PyObject *, PyObject *), \
        ARGS(tstate, frame, object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_LazyImportName, PyObject *, (PyThreadState *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, int, int), \
        ARGS(tstate, object, object, object, object, object, scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_LoadName, PyValue, (PyThreadState *, _PyInterpreterFrame *, PyObject *), \
        ARGS(tstate, frame, object), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_MatchClass, PyObject *, (PyThreadState *, PyValueBorrow, PyValueBorrow, Py_ssize_t, PyObject *), \
        ARGS(tstate, borrowed, borrowed, scalar, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_MatchKeys, PyObject *, (PyThreadState *, PyObject *, PyObject *), \
        ARGS(tstate, object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_SpecialMethodCanSuggest, int, (PyTypeObject *, int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_StoreName, int, (PyThreadState *, PyValueBorrow, PyObject *, PyObject *), \
        ARGS(tstate, borrowed, object, object), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_PyEval_StoreSlice, int, (PyValueBorrow, PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    HELPER(_Py_atomic_load_uintptr_relaxed, uintptr_t, (const uintptr_t *), \
        ARGS(scalar), SCALAR, EFFECTS(reads))

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
     * The element index is a variable that the macro declares.  \
     * IR_GUARD_INLINE_INT expands to !PyValue_IsInlineInt(x) \
     * (ir_leaf.capability_expansion), a test of the operand's tag bits \
     * (the PyValue_IsInlineInt row: EFFECTS()), so it reads no heap or \
     * frame state */ \
    MACRO(IR_GUARD_INLINE_INT, ARGS(borrowed), NONE, EFFECTS(), \
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
     * the values on failure (taken); MACRO_EFFECTS gives the tuple \
     * ALLOCATES.  Neither reads nor writes in design 1.3's sense: the \
     * source is the uop's input home array, an operand; the only object \
     * written is the new one (and the GC list, through \
     * _PyObject_GC_TRACK); the empty tuple is immortal.  tuple_alloc and \
     * PyList_New set MemoryError, hence raises */ \
    MACRO(IR_BUILD_TUPLE, ARGS(taken), VALUE_OR_NULL(owned), \
          EFFECTS(allocates, raises), EXITS()) \
    MACRO(IR_BUILD_LIST, ARGS(taken), VALUE_OR_NULL(owned), \
          EFFECTS(allocates, raises), EXITS()) \
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
     * moves out of it.  IR_WRITE_OUTPUT stores into a home of the uop's \
     * own output array (_PyFrame_WriteHome is `*home = value`), which \
     * design 1.3 excludes from writes ("a home other than its outputs") */ \
    MACRO(IR_WRITE_OUTPUT, ARGS(out, scalar, consumed), NONE, \
          EFFECTS(), EXITS()) \
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
    _PyIR_COMPONENTS_CALLS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_GENERATORS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_ERRORS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_MONITORING(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_SPECIALIZATION(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_EVALUATOR(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_LABELS(PRIMITIVE, HELPER, MACRO) \
    _PyIR_COMPONENTS_TRANSACTIONS(PRIMITIVE, HELPER, MACRO)

#endif /* !Py_INTERNAL_IR_COMPONENTS_H */
