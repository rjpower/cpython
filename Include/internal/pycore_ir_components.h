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
 * tstate, frame, out, lvalue: a macro argument it assigns) and the facts it requires.  On a `PyObject *`
 * parameter, `consumed object` is a stolen object pointer; on a PyValue,
 * `object` is the fact.
 * result: NONE, SCALAR, VIEW (the object of argument 0), PATH(name) (a
 * pointer along a named path from argument 0: its type, its inline
 * values; from the frame for a row without arguments: its globals and
 * builtins, which the activation does not replace), MEASURE(kind) of argument 0, TEST(true facts | false facts, ...)
 * per argument, CONTEXT("reason"), VALUE(mode facts [null_iff(k)])
 * (infallible; with null_iff(k) NULL exactly where argument k is),
 * VALUE(moved(k)) (argument k itself, a consumed one, with its
 * obligation: TAKE), VALUE(mode assumed(fact) by producer) (a fact the
 * value has by its producer's invariant, which the generator asserts
 * after each call; the call is bound to a local, `x = row(...);`),
 * VALUE_OR_NULL(mode facts) (NULL means raised), STATUS(ZERO | NEGATIVE |
 * NONZERO) (the failure value of an int status: raised, and the `out`
 * arguments unwritten), GUARD(facts per argument) (a row with a miss or
 * deopt exit: the facts on its normal return), CASES(case, ...) (a struct of
 * a status and values: per outcome, the signs of its integer fields
 * (status<0, status==1, ...), each value field's ownership and nullness
 * (value(owned), value(null), second(inline nullable)) and `raised` if an
 * exception is set; the walk follows each outcome from the call), HOME (a frame home:
 * GETLOCAL) and HOME(base) (the base of the homes: IR_SLOTS).
 * EFFECTS: reads writes allocates raises releases runs_python polls
 * switches_frame.  A `releases` may run a finalizer, which may run Python
 * and write the frame's homes (FrameLocalsProxy).
 * HOMES(canonical | unknown), OBSERVER(none): optional columns of a row
 * with a reentry effect.  Without HOMES, the code it runs may vacate
 * scratch homes (a debugger's redirect) but leaves the others canonical or
 * NULL; `canonical` says that it leaves initialized homes canonical or
 * NULL (a finalizer or a dunder method: no redirect), `unknown` that it
 * promises nothing.  OBSERVER(none): it opens no observer context of the
 * current frame (ir_error_effects.py derives the source's error-path
 * effects from them).
 * FEEDBACK: the feedback the row reads and writes, transitively:
 * read(f) and write(f) with f one of counter (the entry header's counter),
 * entry (every field of the site's entry, the counter included) and
 * handler (the word's handler).  A uop declares the union of its callees'
 * in its feedback(...) clause; its cache operands are reads of their
 * fields.  The vocabulary is Tools/cases_generator/ir_contract.py's.
 *
 *   DESTRUCTOR(name, type, callback_free)
 *       a deallocator of objects of exactly type, reviewed to release
 *       nothing: a close through it requires exact(type) of the value (or
 *       an inline value, which it does not reach), `writes` (frees memory)
 *       and does not `release`
 *   INTRINSIC(name, ARGS(...), result, BUILD(gil | free_threaded | all))
 *       a form that the generator treats as the identity or erases in the
 *       named builds
 *   FIELD(struct, field, ctype, ON(path), result, EFFECTS(...))
 *       a field read through a pointer along a path (`tp->tp_version_tag`
 *       with tp the type of an operand): a MEASURE or a TEST of the
 *       operand
 *   SLOT(name, ret, (params), ARGS(...), result, EFFECTS(...)[, FEEDBACK(...)])
 *       a protocol of function pointers, named by its C pointer type: the
 *       contract every target meets.  A FIELD with result SLOT(name) holds
 *       such a pointer; a local assigned only from such fields, without a
 *       cast, calls the protocol
 *   CONSTANT(name, VALUE(inline facts))
 *       a static immortal value (V8's roots): PyValue_None, PyValue_True,
 *       PyValue_False and PyValue_NULL, whose fact is `null`, the negation
 *       of nonnull
 *   CONSTANT(name, OBJECT(type))
 *       a static object, named by its address and held to its C type:
 *       `is(name)` of a value is that its object is this one
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
    /* Static immortal values (pycore_value.h): None, True and False are \
     * their static objects' addresses tagged Py_TAG_REFCNT, which no count \
     * changes, and NULL is bits 0 */ \
    /* Evidence (G0c8): pycore_value.h:490-493: PyValue_None, PyValue_True and \
     * PyValue_False are `{.bits = (uintptr_t)&_Py_NoneStruct | Py_TAG_REFCNT}` and its \
     * twins: the address of a static immortal object, tagged as an immortal reference. \
     * No count ever changes, so each is `inline` (owes nothing).  PyValue_NULL_BITS is 0 \
     * (pycore_value.h:249), the `null` fact. */ \
    CONSTANT(PyValue_None, VALUE(inline exact(_PyNone_Type))) \
    CONSTANT(PyValue_True, VALUE(inline exact(PyBool_Type))) \
    CONSTANT(PyValue_False, VALUE(inline exact(PyBool_Type))) \
    CONSTANT(PyValue_NULL, VALUE(inline null)) \
    /* Static objects that guards compare by address */ \
    /* Evidence (G0c8): Include/object.h:391 `PyAPI_DATA(PyTypeObject) PySuper_Type`: a static \
     * type object, compared by address (LOAD_SUPER_ATTR's guard). */ \
    CONSTANT(PySuper_Type, OBJECT(PyTypeObject)) \
    /* Types every instance of which is one of the static immortal values \
     * above, so an exact value of one owes no count (G0a5 desk D): \
     * bool_new and every bool result return Py_True or Py_False, and bool \
     * is final (no Py_TPFLAGS_BASETYPE, boolobject.c); NoneType's tp_new \
     * returns Py_None and NoneType is final (object.c) */ \
    CONSTANT(PyBool_Type, OBJECT(PyTypeObject), INSTANCES(PyValue_True, PyValue_False)) \
    CONSTANT(_PyNone_Type, OBJECT(PyTypeObject), INSTANCES(PyValue_None)) \
    /* Tag tests: bits in, no memory.  PyValue_IsNull is bits == 0: NULL \
     * exactly when it holds */ \
    PRIMITIVE(PyValue_IsNull, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(null | nonnull), EFFECTS()) \
    /* Reviewed G0c2. pycore_value.h: bits >= Py_INT_BASE, a compare of the   \
     * word; no memory read, no failure, no callback.                         */ \
    PRIMITIVE(PyValue_IsInlineInt, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(inline_int | rest), EFFECTS()) \
    /* PyValue_IsObject (Include/internal/pycore_pyvalue.h): bits > 2, \
     * bits < Py_PTR_LIMIT and bit 1 clear; a comparison of the word, no \
     * memory, no callee.  True: a pointer-space word naming an object.  \
     * NULL (bits 0) gives false, as do bits 1 and 2, a debug build's \
     * PyValue_Wrap pointer (bit 1 set), a tagged index (0x0001 top bits, \
     * at or above Py_PTR_LIMIT) and every inline number.  So the false arm \
     * says only that the word is not an object: for a language value it \
     * is an inline number, and for an iterator cursor it may be a tagged \
     * index; for NULL it is neither, which is why the argument must be \
     * nonnull.  Every caller in bytecodes.c tests a stack operand or an \
     * argument slot, which is not NULL.  The row's tokens are corrected \
     * by G0a5 (a partition of the encodings), not here */ \
    PRIMITIVE(PyValue_IsObject, int, (PyValueBorrow), \
              ARGS(borrowed nonnull), TEST(object | rest), EFFECTS()) \
    /* Each tag test below is a comparison of the word's bits against the \
     * region bounds of pycore_value.h (the cited predicate); none reads \
     * memory.  A test with no fact of the vocabulary to declare declares \
     * none.  Reviewed (G0c batch 1): PyValue_IsPointerSpace is `bits < \
     * Py_PTR_LIMIT`, which NULL (bits 0) and the tag-bit words 1 and 2 \
     * satisfy, so no fact (not even nonnull) follows from it; \
     * PyValue_IsTaggedIndex is `bits - Py_INDEX_BASE < Py_DOUBLE_OFFSET - \
     * Py_INDEX_BASE`; PyValue_IsInline is `bits >= Py_DOUBLE_OFFSET`, an \
     * int or a float, never a True, False or None word.  No call, no \
     * allocation, no callback, no failure.  This branch's vocabulary has \
     * no tagged_index fact, so PyValue_IsTaggedIndex says TEST(); G0a5's \
     * representation table corrects the token to TEST(tagged_index) */ \
    PRIMITIVE(PyValue_IsPointerSpace, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(), EFFECTS()) \
    /* PyValue_IsInline (Include/internal/pycore_value.h): bits >= \
     * Py_DOUBLE_OFFSET, a comparison with a region bound; no memory, no \
     * callee.  True: an inline float or int word (the unused NaN images \
     * are in the range too, and no language value holds them).  False: \
     * NULL, a pointer-space word (object, wrap pointer) or a tagged index \
     * (0x0001 top bits, below Py_DOUBLE_OFFSET).  So the true arm gives \
     * `inline`, and the false arm gives neither `object` nor `nonnull`: a \
     * body that needs `object` tests PyValue_IsObject */ \
    PRIMITIVE(PyValue_IsInline, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(inline | rest), EFFECTS()) \
    /* Reviewed G0c2. pycore_value.h: one subtract and compare of the         \
     * word's bits; no memory, no failure, no callback.                       */ \
    PRIMITIVE(PyValue_IsInlineFloat, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(inline_float | rest), EFFECTS()) \
    PRIMITIVE(PyValue_IsTaggedIndex, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(tagged_index | rest), EFFECTS()) \
    /* (a & b) >= Py_INT_BASE: exactly IsInlineInt(a) and IsInlineInt(b) \
     * (the comment above its definition in pycore_value.h: the AND has the \
     * top Py_INT_SHIFT bits set only if both words do) */ \
    /* Reviewed G0c2. pycore_value.h: (a & b) >= Py_INT_BASE on the two       \
     * words; no memory, no failure, no callback.                             */ \
    PRIMITIVE(PyValue_BothInlineInt, bool, (PyValueBorrow, PyValueBorrow), \
              ARGS(borrowed, borrowed), TEST(inline_int, inline_int), EFFECTS(), \
              CONJOINS(PyValue_IsInlineInt)) \
    /* Bits compared with bits (pycore_value.h: PyValue_Is, \
     * _PyValue_NamesObject, PyValue_IsTrue, IsFalse, IsNone).  A word that \
     * is True, False or None is an object word: a static object's address \
     * with Py_TAG_REFCNT.  Reviewed (G0c batch 1): PyValue_IsTrue and \
     * PyValue_IsFalse are `bits == (&_Py_TrueStruct | Py_TAG_REFCNT)` and \
     * the same for _Py_FalseStruct: no memory, no call, no failure.  A true \
     * result says the word is that constant, is(PyValue_True) or \
     * is(PyValue_False); the CONSTANT rows give them `inline \
     * exact(PyBool_Type)`, which implies object, so a reader of the true \
     * arm still has object.  (They declared TEST(object), which was true \
     * but less than the test says; PyValue_IsNone below is the same \
     * correction.) */ \
    /* Reviewed G0c2. pycore_value.h: compares the words' bits, ignoring      \
     * bit 0 of a pointer; no memory, no failure, no callback.                */ \
    PRIMITIVE(PyValue_Is, bool, (PyValueBorrow, PyValueBorrow), \
              ARGS(borrowed, borrowed), TEST(,), EFFECTS(), IDENTITY()) \
    /* Evidence (G0c batch 6): Include/internal/pycore_value.h: compares \
     * the word, tag bit cleared, with the address of op. No call, no \
     * failure, no callback. Its true result is the identity of the value's \
     * object with op; the row declares no fact because the second operand \
     * is an expression, not a CONSTANT (F21 admits `x == &K` only). */ \
    PRIMITIVE(_PyValue_NamesObject, bool, (PyValueBorrow, PyObject *), \
              ARGS(borrowed, object), TEST(,), EFFECTS()) \
    PRIMITIVE(PyValue_IsTrue, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(is(PyValue_True)), EFFECTS()) \
    PRIMITIVE(PyValue_IsFalse, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(is(PyValue_False)), EFFECTS()) \
    /* PyValue_IsNone (pycore_value.h, reviewed G0c batch 1) is `PyValue_Is(v, PyValue_None)`: \
     * the word's bits against None's bits (an object word, the static \
     * _Py_NoneStruct address with Py_TAG_REFCNT).  No call, no memory, no \
     * callback, no failure.  True means the word is None, so the fact is \
     * is(PyValue_None); the CONSTANT row gives None `inline \
     * exact(_PyNone_Type)`, which implies object, so readers of the true \
     * arm still get object */ \
    PRIMITIVE(PyValue_IsNone, bool, (PyValueBorrow), \
              ARGS(borrowed), TEST(is(PyValue_None)), EFFECTS()) \
    /* VALUE_CHECK_FUNC(Bool) and PyValue_LongCheckExact (pycore_value.h): \
     * an object word, and its type read through ob_type.  The object check \
     * dereferences the word, so NULL is not admitted.  Reviewed (G0c \
     * batch 1): PyValue_BoolCheck is `PyValue_IsPointerSpace(v) && \
     * PyBool_Check(AsKnownObjectBorrow(v))`; NULL is in the pointer space \
     * and would be dereferenced, hence ARGS(borrowed nonnull); an inline \
     * int or float is not in it and gives false.  PyBool_Check reads ob_type \
     * (reads) and calls nothing.  PyValue_LongCheckExact is `IsInlineInt(v) \
     * || (IsPointerSpace(v) && PyLong_CheckExact(...))`: the same, and it \
     * is true for an inline int too, which is why its fact is \
     * exact(PyLong_Type) and not object */ \
    PRIMITIVE(PyValue_BoolCheck, bool, (PyValueBorrow), \
              ARGS(borrowed nonnull), TEST(exact(PyBool_Type)), EFFECTS(reads)) \
    /* Reviewed G0c2. pycore_value.h: IsInlineInt, else IsPointerSpace and    \
     * PyLong_CheckExact (a read of ob_type: reads). NULL is pointer space    \
     * and would be dereferenced, hence nonnull. No failure, no callback.     */ \
    PRIMITIVE(PyValue_LongCheckExact, bool, (PyValueBorrow), \
              ARGS(borrowed nonnull), TEST(exact(PyLong_Type)), EFFECTS(reads)) \
    /* Inline ints: scaled values, see _PyValue_IntScaled() */ \
    /* Reviewed G0c2. pycore_value.h: the word shifted left by                \
     * Py_INT_SHIFT; inline int required (asserted); pure bit arithmetic.     */ \
    PRIMITIVE(_PyValue_IntScaled, int64_t, (PyValueBorrow), \
              ARGS(borrowed inline_int), SCALAR, EFFECTS()) \
    /* Reviewed G0c2. pycore_value.h: arithmetic shift of the scaled word;    \
     * the argument must be an inline int (asserted). No memory, no           \
     * failure.                                                               */ \
    PRIMITIVE(PyValue_InlineInt64, int64_t, (PyValueBorrow), \
              ARGS(borrowed inline_int), SCALAR, EFFECTS()) \
    /* Reviewed G0c2. pycore_value.h: ((uint64_t)s >> Py_INT_SHIFT) |         \
     * Py_INT_BASE; s must be a multiple of 2**Py_INT_SHIFT (asserted).       \
     * Never NULL, owns no count, no memory.                                  */ \
    PRIMITIVE(_PyValue_FromIntScaled, PyValue, (int64_t), \
              ARGS(scalar), VALUE(inline inline_int), EFFECTS()) \
    /* Inline constructors (pycore_value.h): the caller has checked the \
     * range; the result is a word with tag bits set, never NULL */ \
    /* Reviewed G0c2. pycore_value.h: Py_INT_BASE | bits of i; i must fit     \
     * inline (asserted). Never NULL, owns no count, no memory, no            \
     * callback.                                                              */ \
    PRIMITIVE(PyValue_FromInlineInt, PyValue, (int64_t), \
              ARGS(scalar), VALUE(inline inline_int), EFFECTS()) \
    /* Reviewed G0c2. pycore_value.h: the word minus Py_DOUBLE_OFFSET,        \
     * copied as a double; inline float required (asserted); no memory.       */ \
    PRIMITIVE(PyValue_InlineFloat, double, (PyValueBorrow), \
              ARGS(borrowed inline_float), SCALAR, EFFECTS()) \
    /* Reviewed G0c2. pycore_value.h: the double's bits plus                  \
     * Py_DOUBLE_OFFSET; d must not be NaN (asserted). Never NULL, no         \
     * count, no memory.                                                      */ \
    PRIMITIVE(PyValue_FromDoubleNotNaN, PyValue, (double), \
              ARGS(scalar), VALUE(inline inline_float), EFFECTS()) \
    /* Tagged indices (pycore_value.h): arithmetic on the word.  Reviewed \
     * G0c5.  PyValue_TagIndex builds the word Py_INDEX_BASE | (i & limit) \
     * (asserts -1 <= i < 2**47); it calls nothing, touches no memory, and \
     * the result is a tagged index word: no count, never NULL, and not an \
     * inline number (its bits lie below Py_DOUBLE_OFFSET, \
     * pycore_value.h:257-259, so PyValue_IsInline is false of it; the \
     * row's VALUE(inline) is a seed token that G0a5 corrects).  \
     * PyValue_IncrementTaggedIndexNoOverflow adds 1 to the bits of its \
     * argument, a tagged index, which owns no count; the result is the \
     * word advanced (a new value, not a pass-through of the old one: the \
     * seeded `consumed` states that, and a `scalar` mode would be equally \
     * true of an inline word).  Its precondition (0 <= index < 2**47 - 1) is asserted in C and kept by the \
     * bounds guards of the callers (_ITER_JUMP_LIST/TUPLE).  Callees, \
     * failure paths and callbacks: none */ \
    PRIMITIVE(PyValue_TagIndex, PyValue, (intptr_t), \
              ARGS(scalar), VALUE(inline tagged_index), EFFECTS()) \
    /* PyValue_UntagIndex (pycore_value.h): a shift of the word's bits, no \
     * memory, no failure.  It asserts PyValue_IsTaggedIndex(r) (a \
     * precondition of the caller's contract that this branch's \
     * vocabulary cannot state: no tagged_index fact yet) */ \
    PRIMITIVE(PyValue_UntagIndex, intptr_t, (PyValueBorrow), \
              ARGS(borrowed), SCALAR, EFFECTS()) \
    PRIMITIVE(PyValue_IncrementTaggedIndexNoOverflow, PyValue, (PyValue), \
              ARGS(consumed), VALUE(inline tagged_index), EFFECTS()) \
    /* Evidence (G0d4), pycore_value.h, a static inline function: a no-op \
     * (a cast to void, with an assert of the representation).  GET_ITER \
     * leaves a for loop's cursor as NULL (the object protocol) or a tagged \
     * index (the index protocol), both of which owe no count, and nothing \
     * but the loop's steps changes it; a step that consumes the cursor \
     * without a use of it drops it here (no memory, no call, no failure, \
     * no callback). */ \
    HELPER(_PyValue_DropCursor, void, (PyValue), \
              ARGS(consumed), NONE, EFFECTS()) \
    /* PyValue_FromInt64 (pycore_value.h): _PyValue_FromIntScaled when the \
     * int fits inline, else _PyValue_BoxInt64, which gives NULL only when \
     * its allocation fails */ \
    /* Reviewed G0c2. pycore_value.h: PyValue_FromInlineInt when the int      \
     * fits, else _PyValue_BoxInt64 (the SLOW column), whose row allocates    \
     * and raises; NULL means MemoryError. No other callee, no Python.        */ \
    PRIMITIVE(PyValue_FromInt64, PyValue, (int64_t), \
              ARGS(scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
              EFFECTS(allocates, raises), SLOW(_PyValue_BoxInt64)) \
    /* A new int outside the inline range: allocates, and raises \
     * MemoryError when that fails; calls no Python */ \
    /* Reviewed G0c2. Python/pyvalue.c: PyLong_FromLongLong then              \
     * _PyValue_PointerSteal; NULL (with MemoryError set) only if the         \
     * allocation fails. The argument is outside the inline range             \
     * (asserted), so the result is a new boxed int. No finalizer, no         \
     * Python.                                                                */ \
    HELPER(_PyValue_BoxInt64, PyValue, (int64_t), \
           ARGS(scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
           EFFECTS(allocates, raises)) \
    /* Floats: the operands of float arithmetic are inline floats, or an \
     * inline float and an inline int; NaN results are boxed (allocates, \
     * MemoryError) */ \
    /* Reviewed G0c2. pycore_value.h: tests the operands (two inline floats   \
     * first, then PyValue_FloatOperandsCheck) and writes *da and *db only    \
     * when it returns true; on false the outputs are unwritten. No memory    \
     * beyond the two out doubles, no failure, no callback. The true result   \
     * says both operands are inline.                                         */ \
    PRIMITIVE(PyValue_FloatOperandValues, bool, \
              (PyValueBorrow, PyValueBorrow, double *, double *), \
              ARGS(borrowed, borrowed, out, out), TEST(inline, inline), EFFECTS()) \
    /* A NaN is boxed by _PyValue_BoxDouble (PyValue_FromDouble in \
     * pycore_value.h), which gives NULL only when its allocation fails; any \
     * other double is an inline float */ \
    /* Reviewed G0c2. Python/pyvalue.c: PyFloat_FromDouble of a NaN           \
     * (asserted) then PyValue_FromObjectSteal; NULL with MemoryError only    \
     * if allocation fails (float freelist, then PyObject_Malloc). No         \
     * Python.                                                                */ \
    HELPER(_PyValue_BoxDouble, PyValue, (double), \
           ARGS(scalar), VALUE_OR_NULL(owned exact(PyFloat_Type)), \
           EFFECTS(allocates, raises)) \
    /* Reviewed G0c2. pycore_value.h: PyValue_FromDoubleNotNaN for a          \
     * non-NaN, else _PyValue_BoxDouble (the SLOW column: allocates,          \
     * MemoryError, NULL). No Python.                                         */ \
    PRIMITIVE(PyValue_FromDouble, PyValue, (double), \
              ARGS(scalar), VALUE_OR_NULL(owned exact(PyFloat_Type)), \
              EFFECTS(allocates, raises), SLOW(_PyValue_BoxDouble)) \
    /* a * b of two words below 2**60 in magnitude: a new int outside the \
     * inline range, or NULL where allocation fails (pycore_value.h) */ \
    /* Reviewed G0c2. Objects/longobject.c: __builtin_mul_overflow, then      \
     * PyValue_FromInt64 of the product (inline or boxed: the result is       \
     * exact int, not necessarily outside the inline range), else             \
     * long_mul_spans on digit buffers and _PyLong_OutValue. NULL with        \
     * MemoryError on allocation failure. No object is released, no Python.   */ \
    HELPER(_PyValue_MultiplyInt64, PyValue, (int64_t, int64_t), \
           ARGS(scalar, scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
           EFFECTS(allocates, raises)) \
    /* The close of an owned value through a named deallocator.  Its effect \
     * is the deallocator's: `writes` for a DESTRUCTOR row, `releases` for \
     * any other (ir_contract.py) */ \
    /* Reviewed G0c2. pycore_value.h: if the value owns a counted pointer,    \
     * Py_DECREF_MORTAL_SPECIALIZED calls the named destructor at zero; the   \
     * destructor row decides the effect (writes for a callback_free          \
     * DESTRUCTOR, releases otherwise), which is why this row declares        \
     * none. Consumes the value; no failure. No ir_leaf model: G0d.           */ \
    /* Evidence (G0d3): the release of a view's owner (the generator's IR \
     * forms): PyValue_XCLOSE_SPECIALIZED of the owner, which is NULL for a \
     * local, or _PyValue_XCLOSE_SPECIALIZED_POINTER for a value known to be \
     * in pointer space (pycore_value.h) */ \
    PRIMITIVE(PyValue_CLOSE_SPECIALIZED, void, (PyValue, destructor), \
              ARGS(consumed, destructor), NONE, EFFECTS(), \
              VIEW_CLOSE(PyValue_XCLOSE_SPECIALIZED), \
              POINTER_FORM(_PyValue_XCLOSE_SPECIALIZED_POINTER)) \
    /* _PyLong_ExactDealloc (Objects/longobject.c): re-immortalizes a small \
     * int, else _Py_FREELIST_FREE or PyObject_Free; an int holds no \
     * reference, so nothing is released and no Python runs */ \
    /* Reviewed G0c2 (Objects/longobject.c, _PyLong_ExactDealloc): a small    \
     * int is made immortal again; a compact int goes to the ints freelist    \
     * (_Py_FREELIST_FREE) else PyObject_Free; any other int is               \
     * PyObject_Free. An int holds no references, no finalizer, no weakref    \
     * callback (exact ints have no weaklist): callback_free confirmed.       */ \
    DESTRUCTOR(_PyLong_ExactDealloc, PyLong_Type, callback_free) \
    /* _PyFloat_ExactDealloc (Objects/floatobject.c): _Py_FREELIST_FREE of \
     * the float, or PyObject_Free; a float holds no reference */ \
    /* Reviewed G0c2 (Objects/floatobject.c): _Py_FREELIST_FREE(floats,       \
     * ...) else PyObject_Free; a float holds no references and an exact      \
     * float has no finalizer: callback_free confirmed.                       */ \
    DESTRUCTOR(_PyFloat_ExactDealloc, PyFloat_Type, callback_free) \
    /* The generic binary operation: value_binary_op -> PyNumber_* -> an \
     * nb_* slot -> any __add__ and friends: runs Python */ \
    /* Reviewed G0c2 (Python/ceval.c). value_binary_op(oparg, lhs, rhs):      \
     * NB_SUBSCR goes to value_subscript, any other operation to              \
     * _PyValue_BinaryOps[oparg](lhs, rhs) -> PyNumber_* -> binary_op1 ->     \
     * an nb_* slot -> any __add__ and friends, or the __r*__ fallback.       \
     * Both operands are borrowed on every path; the result is owned, or      \
     * NULL with an exception set (TypeError for no implementation, or        \
     * whatever the method raised). Callbacks: arbitrary Python.              */ \
    HELPER(value_binary_op, PyValue, (int, PyValueBorrow, PyValueBorrow), \
           ARGS(scalar, borrowed, borrowed), VALUE_OR_NULL(owned), \
           EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (pycore_value.h):                                        \
     * Py_NewRef(PyValue_AsKnownObjectBorrow(value)); a macro over two        \
     * inline functions with no code of its own, so it stays a MACRO row.     \
     * Takes a new reference (not an effect), cannot fail.                    */ \
    MACRO(PyValue_AsPyObjectNew, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* pycore_value.h: PyValue_XCLOSE of the local, then the local is set \
     * to PyValue_NULL; the release is XCLOSE's (releases, which can run \
     * a finalizer) */ \
    MACRO(PyValue_CLEAR, \
        ARGS(lvalue), SCALAR, EFFECTS(releases), EXITS()) \
    /* Reviewed G0c2 (pycore_value.h): _PyValue_PointerSteal of an object     \
     * that is not an exact int or float (asserted); the value owns the       \
     * argument's count. Never NULL (asserted), no callback.                  */ \
    HELPER(PyValue_FromNonNumberSteal, PyValue, (PyObject *), \
        ARGS(consumed object), VALUE(owned), EFFECTS()) \
    /* pycore_value.h: the canonical borrowed value of an object: for an \
     * exact int or float, _PyObject_ValueIdentity reads its payload \
     * (immutable), otherwise the pointer.  No count, no allocation, no \
     * callback; NULL gives PyValue_NULL.  Reads the object's type and \
     * payload */ \
    HELPER(PyValue_FromObjectBorrow, PyValueBorrow, (PyObject *), \
        ARGS(object), VALUE(borrowed), EFFECTS(reads)) \
    /* Include/internal/pycore_value.h PyValue_FromObjectSteal: a static \
     * inline that reads the object's type to choose its word (an exact int \
     * or float becomes an inline number and the object's reference is \
     * dropped, else the word is the pointer).  It steals the reference on \
     * every path and cannot fail.  The result is the argument's reference \
     * (or an inline number whose discharge is a no-op), which is a move: \
     * VALUE(moved(0)), so the argument must hold an obligation (G0d1). \
     * Callbacks: none for the identity path.  The drop of \
     * a boxed number's count is a deallocation by _PyLong_ExactDealloc or \
     * _PyFloat_ExactDealloc only when the box was unreferenced, which does \
     * not run Python */ \
    HELPER(PyValue_FromObjectSteal, PyValue, (PyObject *), \
        ARGS(consumed object), VALUE(moved(0)), EFFECTS()) \
    /* Evidence (G0c8): pycore_value.h VALUE_CHECK_FUNC(Gen): PyValue_IsPointerSpace(value) \
     * && PyGen_Check(object), i.e. PyObject_TypeCheck(op, &PyGen_Type): Py_IS_TYPE, else \
     * PyType_IsSubtype, which walks tp_mro (a read; no callback, no allocation, no \
     * exception).  A NULL or inline word is not pointer space and answers false, so the \
     * argument needs no `nonnull`.  The answer is a scalar: a subtype fact on a true \
     * result would be a heap fact (the MRO can change) and no uop uses one. */ \
    HELPER(PyValue_GenCheck, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(reads)) \
    /* pycore_value.h: PyValue_IsPointerSpace(v) ? BITS_TO_PTR_MASKED(v) \
     * : NULL, a range compare on the word's bits; no heap access, no \
     * callback (the seed's runs_python had no source) */ \
    HELPER(PyValue_ObjectOrNull, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS()) \
    /* seeded from the C prototype, ir_records.effect_sites */ \
    /* Evidence (Include/internal/pycore_value.h PyValue_Unwrap; G0c batch \
     * 6).  Bit conversion only: it strips the tag of a PyValue made by \
     * PyValue_Wrap, which holds a pointer to a frame (no object, no count). \
     * `consumed` states that the wrapped value's obligation, the frame's \
     * push, passes to the pointer.  No call, no failure, no callback. */ \
    HELPER(PyValue_Unwrap, void *, (PyValue), \
        ARGS(consumed), SCALAR, EFFECTS()) \
    /* pycore_value.h: the bits of a pointer, tagged invalid in a \
     * Py_DEBUG build; no access to the pointee, no count: the result \
     * owns nothing the pointer did not own */ \
    HELPER(PyValue_Wrap, PyValue, (void *), \
        ARGS(scalar), VALUE(owned), EFFECTS()) \
    /* Reviewed G0c2 (Include/pyport.h): an arithmetic shift of a signed      \
     * type; arithmetic on scalars only.                                      */ \
    MACRO(Py_ARITHMETIC_RIGHT_SHIFT, \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* seeded from ir_records.effect_sites; the \
     * result is argument 0 moved, value and obligation (F5).  Reviewed \
     * (G0c batch 1): TAKE is a generator replacer (ir_generator.py \
     * take_view, `_PyIR_Taken` in ceval_macros.h): in the interpreter \
     * form it is the value itself; in the IR forms the detached owner's \
     * count moves, or a local's value gets a new count (PyValue_XDUP: a \
     * count only).  No callback, no failure; it is not a C function */ \
    MACRO(TAKE, ARGS(consumed), VALUE(moved(0) null_iff(0)), EFFECTS(), EXITS(), TAKES_VIEW()) \
    /* Evidence (G0d3): OWNER(x) is a generator replacer like TAKE \
     * (IREmitter.take_view): the owner of x's input, PyValue_NULL for a \
     * local, which only IR_RETIRE_PUBLISHED may read; for a value that is \
     * not a view, x itself.  No callback, no failure; not a C function. \
     * G0d1: a local's view has no owner, so OWNER is NULL there whatever x \
     * is (G0d2's report): x's obligation or NULL, VALUE_OR_NULL(moved(0)) */ \
    MACRO(OWNER, ARGS(consumed), VALUE_OR_NULL(moved(0)), EFFECTS(), EXITS(), TAKES_VIEW()) \
    /* Evidence (G0c batch 6): Include/internal/pycore_interpframe.h: \
     * copies the bits of a view into an owner: no count changes. The \
     * caller must have cleared the slot that held the count (its comment). \
     * Used where the slot is cleared in the same expression it is correct \
     * (owned); _INIT_CALL_BOUND_METHOD_EXACT_ARGS and call_expand_method \
     * use it for a view that keeps the method's count and so adopt nothing \
     * (see g0c/b6/pending.md). */ \
    HELPER(_PyIR_AdoptTemp, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE(owned), EFFECTS()) \
    /* Python/ir.c: compares the handler against two ids and indexes the \
     * const tables _PyIR_Formats and _PyOpcode_Deopt, which nothing \
     * writes at run time.  No call, no heap read, no failure. */ \
    HELPER(_PyIR_BaseOpcode, int, (int), \
        ARGS(scalar), SCALAR, EFFECTS()) \
    /* Objects/object.c (_PyValue_ASCII): _PyValue_Repr, then \
     * _PyUnicode_AsASCIIString on a non-ASCII repr: __repr__ runs Python.  \
     * Returns a new str reference or NULL with an exception set; the \
     * argument is borrowed.  A NULL word prints as "<NULL>", so the \
     * argument is not required to be nonnull */ \
    HELPER(_PyValue_ASCII, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed (G0c batch 1).  _PyValue_CheckHome (pycore_value.h) is \
     * `((void)0)` unless Py_DEBUG; under Py_DEBUG it reads the word and \
     * its object's identity (_PyValue_IsHome) and, on a violation, \
     * reports it (fatal, or counted by _PYTHON_VALUE_HOME_REPORT); it \
     * runs no Python and takes no count.  The verified product is the \
     * build without Py_DEBUG, where it is erased (ir_leaf.ERASED) */ \
    INTRINSIC(_PyValue_CheckHome, ARGS(scalar), NONE, BUILD(gil)) \
    /* Reviewed G0c2 (Objects/abstract.c, pycore_value_ops.h). The            \
     * container's contains slot (sq_contains, or iteration with __eq__       \
     * when it has none): arbitrary Python. Returns 1, 0 or -1 with the       \
     * exception set (TypeError for a non-container); both arguments          \
     * borrowed.                                                              */ \
    HELPER(_PyValue_Contains, int, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/abstract.c (_PyValue_DelItem) -> value_ass_item with a \
     * NULL value: an exact dict goes to _PyDict_DelValue, anything else to \
     * mp_ass_subscript or sq_ass_item (Python: __delitem__, __eq__, \
     * __hash__).  Returns 0, or -1 with an exception set (TypeError when \
     * the type has no deletion).  Both arguments are borrowed */ \
    HELPER(_PyValue_DelItem, int, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/abstract.c _PyValue_Format (format(obj, spec)).  The spec may \
     * be PyValue_NULL (no spec: FORMAT_SIMPLE), which ARGS allows; a \
     * non-NULL spec that is not an exact-or-subclass str raises \
     * SystemError.  Callees: _PyValue_Str for an empty spec on an exact \
     * int or float, else the type's __format__ through a method lookup and \
     * call: arbitrary Python.  Failure: NULL with an exception set; both \
     * arguments borrowed on every path.  Result: a new reference (an exact \
     * str argument comes back as Py_NewRef of itself) */ \
    HELPER(_PyValue_Format, PyObject *, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Objects/object.c: _PyObject_CheckAttributeName (TypeError for a \
     * non-str name), then call_getattro on the value's type: \
     * tp_getattro, which is any __getattribute__ or __getattr__ (runs \
     * Python), and on NULL value_attribute_error_context adds the \
     * suggestion data.  Returns an owned value, or PyValue_NULL with an \
     * exception set.  Arguments borrowed */ \
    HELPER(_PyValue_GetAttr, PyValue, (PyValueBorrow, PyObject *), \
        ARGS(borrowed, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* pycore_value_ops.h: status 1 an unbound method, 0 the attribute, -1 an \
     * error; the value owned unless NULL (G0a4: a CASES result).  \
     * Objects/object.c: the type-cache lookup of name; a type whose \
     * getattro is not the generic one, or a name that is not an exact str, \
     * goes to _PyValue_GetAttr (any __getattribute__ or __getattr__ runs \
     * Python); a descriptor's tp_descr_get runs (a property runs Python); \
     * -1 returns PyValue_NULL with an exception.  Arguments borrowed; the \
     * caller keeps its own count of the receiver across the call */ \
    HELPER(_PyValue_GetMethod, _PyStatusValue, (PyValueBorrow, PyObject *), \
        ARGS(borrowed, object), \
        CASES(status==-1 value(null) raised, status==0 value(owned), status==1 value(owned)), EFFECTS(runs_python, raises)) \
    /* pycore_value_ops.h: 1 with the item, 0 for a missing key (no exception), -1 on another error (G0a4: a CASES result) */ \
    HELPER(_PyValue_GetOptionalItem, _PyStatusValue, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), \
        CASES(status==-1 value(null) raised, status==0 value(null), status==1 value(owned)), EFFECTS(runs_python, raises)) \
    /* Reviewed (G0c batch 1).  _PyValue_Invert and _PyValue_Negative \
     * (UNARY_FUNC in Objects/abstract.c): a NULL argument is null_error, \
     * NULL result; else the type's nb_invert or nb_negative slot through \
     * call_unary, which runs any __invert__ or __neg__ (runs_python); a \
     * type without the slot is value_type_error, NULL result.  Failure is \
     * exactly a NULL result with an exception set; the argument is \
     * borrowed on every path (the slot receives a borrow); the result is a \
     * new owner (an inline int or float, or an object with a count), \
     * allocated by the slot (covered by runs_python).  Callbacks: the \
     * slot, and the release of an old result a slot discards */ \
    HELPER(_PyValue_Invert, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises), \
        HOMES(canonical), OBSERVER(none)) \
    /* Evidence (G0c batch 6): Objects/abstract.c _PyValue_IsInstance \
     * (PyObject_IsInstance): exact-type fast path, then the metaclass's \
     * __instancecheck__ or the class's __bases__ walk (Python code and \
     * attribute lookups). Returns 1, 0, or -1 with an exception set; the \
     * arguments are borrowed. Callbacks: __instancecheck__, __class__ and \
     * __bases__ lookups: `runs_python`. */ \
    HELPER(_PyValue_IsInstance, int, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Reviewed (G0c batch 1).  _PyValue_IsTrue (pycore_value_ops.h) is a \
     * static inline function: bits of True, False, None, an inline int or \
     * float answer without a call; any other object goes to \
     * _PyValue_IsTrueSlow (Objects/object.c), which runs the type's \
     * nb_bool, mp_length or sq_length slot (any __bool__ or __len__: \
     * runs_python) and returns 1 when it has none.  Result 0 or 1, or a \
     * negative value with an exception set (the slot failed); the argument \
     * is borrowed on every path.  A NULL argument is not admitted: the \
     * slow path dereferences it */ \
    HELPER(_PyValue_IsTrue, int, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python, raises), \
        HOMES(canonical), OBSERVER(none)) \
    /* G0d1.  Objects/object.c:2537: the truth of a value that is no bool, \
     * None, int or float (asserted): PyValue_AsKnownObjectBorrow of it, so \
     * an object, then the type's nb_bool (a value row or the adapted slot), \
     * mp_length or sq_length; 1 without them.  Returns 1, 0, or a negative \
     * status with an exception set.  The slots are a class's __bool__ or \
     * __len__: Python, which leaves the homes canonical and opens no \
     * observer context (as _PyValue_IsTrue).  The argument is borrowed */ \
    HELPER(_PyValue_IsTrueSlow, int, (PyValueBorrow), \
        ARGS(borrowed object), SCALAR, EFFECTS(runs_python, raises), \
        HOMES(canonical), OBSERVER(none)) \
    /* Objects/longobject.c: the shift of a nonzero word by a count >= 0 \
     * through long_lshift_spans().  It gives PyValue_NULL with an exception \
     * set where the result would have too many digits (OverflowError) or \
     * the allocation fails (MemoryError), so the result is VALUE_OR_NULL \
     * and the row raises (the seed declared an infallible VALUE: wrong).  \
     * Callees: PyErr_SetString, _PyLong_SpanOfWord, long_lshift_spans, \
     * _PyLong_OutValue: arithmetic on digit buffers and one allocation; no \
     * object is released and no Python runs.  The result is an exact int, \
     * boxed or, for a small result, as _PyLong_OutValue makes it */ \
    HELPER(_PyValue_LshiftInt64, PyValue, (int64_t, int64_t), \
        ARGS(scalar, scalar), VALUE_OR_NULL(owned exact(PyLong_Type)), \
        EFFECTS(allocates, raises)) \
    /* _PyValue_Negative: see _PyValue_Invert above */ \
    HELPER(_PyValue_Negative, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises), \
        HOMES(canonical), OBSERVER(none)) \
    /* Evidence (G0c batch 6): Include/internal/pycore_value.h: builds the \
     * view of an object pointer. No count change, no call, no failure, no \
     * callback. The object must outlive the view: its owner is the \
     * caller's. */ \
    HELPER(_PyValue_PointerBorrow, PyValueBorrow, (PyObject *), \
        ARGS(object), VALUE(borrowed), EFFECTS()) \
    /* Evidence (G0c batch 6): Objects/abstract.c _PyValue_Positive \
     * (UNARY_FUNC): NULL argument: SystemError and NULL; otherwise the \
     * type's nb_positive slot is called on the value (an inline number's \
     * slot included; a heap type's __pos__ is Python code), or TypeError \
     * 'bad operand type for unary +' and NULL. Returns an owner, or NULL \
     * with an exception set. The argument is borrowed. `runs_python`. */ \
    HELPER(_PyValue_Positive, PyValue, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises), \
        HOMES(canonical), OBSERVER(none)) \
    /* pycore_value.h: a debug check, erased with the GIL */ \
    INTRINSIC(_PyValue_RecheckHome, ARGS(scalar), NONE, BUILD(gil)) \
    /* Include/internal/pycore_value.h: in assert builds a loop of \
     * _PyValue_RecheckHome over the window (a debug check that reports and \
     * never changes state); `((void)0)` in release.  A C macro that wraps \
     * plain code: converting it to a static inline function is listed, not \
     * done (scope cut) */ \
    MACRO(_PyValue_RecheckWindow, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Objects/object.c (_PyValue_Repr): PyErr_CheckSignals first (a \
     * signal handler runs Python), then tp_repr (Python), with a recursion \
     * check and a str-result check.  A NULL word prints as "<NULL>".  \
     * Returns a new str reference or NULL with an exception set; the \
     * argument is borrowed */ \
    HELPER(_PyValue_Repr, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Objects/object.c). NULL operands raise                  \
     * BadInternalCall; richcompare_exit decides fast identity and exact      \
     * cases; richcompare_protocol calls tp_richcompare of either operand     \
     * (arbitrary __eq__, __lt__ ... and the reflected form). Returns True    \
     * or False for the fast cases, any object otherwise, or NULL with an     \
     * exception set. Operands borrowed on every path.                        */ \
    HELPER(_PyValue_RichCompare, PyValue, (PyValueBorrow, PyValueBorrow, int), \
        ARGS(borrowed, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* pycore_value_ops.h: 1 with the yielded item, 0 with the return value, -1 on error (G0a4: a CASES result) */ \
    HELPER(_PyValue_Send, _PyStatusValue, (PyObject *, PyValueBorrow), \
        ARGS(object, borrowed), \
        CASES(status==-1 value(null) raised, status==0 value(owned), status==1 value(owned)), EFFECTS(runs_python, raises)) \
    /* Objects/abstract.c (_PyValue_SequenceCheck): an inline \
     * number is not a sequence; for an object it tests PyDict_Check and \
     * tp_as_sequence->sq_item through the type.  Reads only: no \
     * allocation, no callback, no failure.  Returns 0 or 1 */ \
    HELPER(_PyValue_SequenceCheck, int, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(reads)) \
    /* Evidence (G0c batch 6): Objects/abstract.c _PyValue_SequenceTuple \
     * (PySequence_Tuple): an inline number: TypeError 'not iterable' and \
     * NULL; an exact tuple: returned with a new count; a list: \
     * PyList_AsTuple; else the iterator is made and drained (__iter__, \
     * __next__ and the items' destructors: Python code). Returns a new \
     * reference to an exact tuple, or NULL with an exception set; the \
     * argument is borrowed. */ \
    HELPER(_PyValue_SequenceTuple, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Objects/object.c: SystemError if the value is NULL with an \
     * exception set, _PyObject_CheckAttributeName, interning of the \
     * name, then the type's setattro (any __setattr__, descriptor \
     * __set__ or __delete__ runs Python).  Returns 0, or -1 with an \
     * exception; a NULL value means deletion.  Arguments borrowed */ \
    HELPER(_PyValue_SetAttr, int, (PyValueBorrow, PyObject *, PyValueBorrow), \
        ARGS(borrowed, object, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/abstract.c (_PyValue_SetItem) -> value_ass_item: an \
     * exact dict goes to _PyDict_SetValue, an exact list with an inline int \
     * index stores in place (releasing the old item: finalizer), anything \
     * else calls mp_ass_subscript or sq_ass_item (Python).  Returns 0, or \
     * -1 with an exception set.  The three arguments are borrowed on every \
     * path */ \
    HELPER(_PyValue_SetItem, int, (PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Reviewed (G0c batch 1).  _PyValue_Size (Objects/abstract.c) asserts \
     * a non-NULL argument; an inline int or float is value_type_error \
     * (-1); else the type's sq_length (any __len__) or, failing that, \
     * _PyValue_MappingSize, which runs mp_length.  Result a length >= 0, \
     * or -1 with an exception set; the argument is borrowed on every \
     * path.  Callbacks: the length slots (runs_python) */ \
    HELPER(_PyValue_Size, Py_ssize_t, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/object.c (_PyValue_Str): PyErr_CheckSignals (Python), \
     * an exact str returns itself, else tp_str (Python) with a recursion \
     * check; a non-str result raises TypeError.  A NULL word prints as \
     * "<NULL>".  Returns a new str reference or NULL with an exception \
     * set; the argument is borrowed */ \
    HELPER(_PyValue_Str, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises))

/* Objects */
#define _PyIR_COMPONENTS_OBJECTS(PRIMITIVE, HELPER, MACRO) \
    /* pycore_value.h: the object pointer of a word that holds an object \
     * (the argument's fact `object`): bit arithmetic on the word, no \
     * heap access, no count */ \
    PRIMITIVE(PyValue_AsKnownObjectBorrow, PyObject *, (PyValueBorrow), \
              ARGS(borrowed object), VIEW, EFFECTS()) \
    /* pycore_value.h: as PyValue_AsKnownObjectBorrow, for a word in \
     * pointer space: bit arithmetic only, no heap access */ \
    PRIMITIVE(_PyValue_AsKnownObjectBorrowPointer, PyObject *, (PyValueBorrow), \
              ARGS(borrowed object), VIEW, EFFECTS()) \
    /* The identity on a feedback word's bits (pycore_value.h).  The word is \
     * a cache entry's guarded value (class_value/4), which the specializer \
     * stores only when it holds the attribute or method that the type \
     * version names (specialize.c), so it is never PyValue_NULL; the \
     * generator asserts it after each call (F7) */ \
    PRIMITIVE(_PyValueGuarded_Load, PyValueBorrow, (_PyValueGuarded), \
              ARGS(scalar), VALUE(borrowed assumed(nonnull) by specializer), EFFECTS()) \
    /* G0d1.  The same identity (pycore_value.h), for a site whose entry \
     * the specializer fills only with an object: LOAD_ATTR_CLASSMETHOD's \
     * classmethod and LOAD_ATTR_GETSET's getset descriptor \
     * (specialize.c).  The generator asserts it after each call (F7).  No \
     * leaf uop calls it, so it is a HELPER: no model or lowering (V11) */ \
    HELPER(_PyValueGuarded_LoadObject, PyValueBorrow, (_PyValueGuarded), \
              ARGS(scalar), VALUE(borrowed assumed(object) by specializer), EFFECTS()) \
    /* Identities on a word that state what the compiler shows of it \
     * (pycore_value.h); the generator asserts it after each call (F7). \
     * An item of a code object's co_consts: the compiler stores objects \
     * only (compile.c's constant cache), so it is never PyValue_NULL */ \
    PRIMITIVE(_PyValue_CodeConstant, PyValueBorrow, (PyValueBorrow), \
              ARGS(borrowed), VALUE(borrowed assumed(nonnull) by compiler), EFFECTS()) \
    /* A local that LOAD_FAST reads: the compiler emits LOAD_FAST only \
     * where flowgraph.c's analysis shows the local bound on every path, \
     * and LOAD_FAST_CHECK elsewhere */ \
    PRIMITIVE(_PyValue_BoundLocal, PyValueBorrow, (PyValueBorrow), \
              ARGS(borrowed), VALUE(borrowed assumed(nonnull) by compiler), EFFECTS()) \
    /* The home of a cell or free variable: MAKE_CELL (a cell variable) and \
     * COPY_FREE_VARS (a free variable, from the closure's cells), which the \
     * compiler emits at the start of the code, fill it with its cell, and \
     * no instruction stores anything else there */ \
    PRIMITIVE(_PyValue_CellLocal, PyValueBorrow, (PyValueBorrow), \
              ARGS(borrowed), VALUE(borrowed assumed(exact(PyCell_Type)) by compiler), \
              EFFECTS()) \
    /* An entry of the interpreter's common constants: pylifecycle.c's \
     * pycore_init_builtins fills each with an object (common_objs) before \
     * any code runs; pystate.c clears them at finalization only */ \
    PRIMITIVE(_PyValue_CommonConstant, PyValueBorrow, (PyValueBorrow), \
              ARGS(borrowed), VALUE(borrowed assumed(nonnull) by interpreter), EFFECTS()) \
    /* G0d1: an item within the length of a published list or tuple. \
     * Only a container under construction (PyList_New(n), PyTuple_New(n)) \
     * holds NULL, and code reads only published ones: listobject.c and \
     * tupleobject.c store an object into every slot before the container \
     * escapes, and a list's length bounds its set items */ \
    PRIMITIVE(_PyValue_PublishedItem, PyValueBorrow, (PyValueBorrow), \
              ARGS(borrowed), VALUE(borrowed assumed(nonnull) by container), EFFECTS()) \
    /* Reviewed G0c2 (pycore_value.h): for a pointer-space word returns its   \
     * pointer, taking over its count if the value owns one, else a new       \
     * reference (Py_NewRef) to the immortal or borrowed object; a            \
     * non-object word aborts (_PyValue_FatalNotObject), which the consumed   \
     * object requirement excludes. No failure path, no callback. It has no   \
     * ir_leaf model and no lowering: G0d.                                    */ \
    /* Evidence (G0d3), V11: no leaf uop calls it, so it has no leaf model and \
     * no lowering, and its uops run as their handler's function form: a C function taking the owner of its argument (TAKES_VIEW) */ \
    HELPER(PyValue_AsKnownObjectSteal, PyObject *, (PyValue), \
              ARGS(consumed object), VIEW, EFFECTS(), TAKES_VIEW()) \
    /* The type of a value or an object, a pointer along the path `type`, \
     * and the version read through it \
     * pycore_value.h: the type of a word: ob_type of an object, the int\
     * or float type of an inline number; a read, no callee, no failure,\
     * no callback */ \
    /* Evidence (G0d3), V11: no leaf uop calls it, so it has no leaf model and \
     * no lowering, and its uops run as their handler's function form: a read of ob_type or the inline type */ \
    HELPER(PyValue_TYPE, PyTypeObject *, (PyValueBorrow), \
              ARGS(borrowed), PATH(type), EFFECTS(reads)) \
    /* Include/object.h: a read of ob_type; no callee, no failure, no \
     * callback */ \
    PRIMITIVE(Py_TYPE, PyTypeObject *, (PyObject *), \
              ARGS(object), PATH(type), EFFECTS(reads)) \
    /* A plain read of the type's version tag: no callee, no failure, no \
     * callback */ \
    FIELD(PyTypeObject, tp_version_tag, unsigned int, ON(type), \
          MEASURE(type_version), EFFECTS(reads)) \
    /* The value slots that uops call (pycore_value_slots.h), read through \
     * _PyType_VALUE_SLOT as the slot table types them (G0a4).  A contains \
     * slot returns 1 or 0, or -1 with an exception; any target may run \
     * Python (a heap type's __contains__) */ \
    SLOT(_Py_containsvaluefunc, int, (PyObject *, PyValueBorrow), \
         ARGS(object, borrowed), SCALAR, EFFECTS(runs_python)) \
    FIELD(PyTypeObject, tp_value.contains, _Py_containsvaluefunc, ON(type), \
          SLOT(_Py_containsvaluefunc), EFFECTS(reads)) \
    /* An iternext slot: PYGEN_NEXT with an owned item; PYGEN_RETURN with \
     * an owned return value, or PyValue_NULL for None (R11), and no \
     * exception; PYGEN_ERROR with PyValue_NULL and an exception set.  Any \
     * target may run Python (a generator's frame, a heap type's __next__) */ \
    SLOT(_Py_iternextvaluefunc, _PyStatusValue, (PyObject *), ARGS(object), \
         CASES(status==-1 value(null) raised, status==0 value(owned nullable), \
               status==1 value(owned)), EFFECTS(runs_python)) \
    FIELD(PyTypeObject, tp_value.iternext, _Py_iternextvaluefunc, ON(type), \
          SLOT(_Py_iternextvaluefunc), EFFECTS(reads)) \
    /* An index-protocol step (object.h): the owned item at a nonnegative \
     * next index; PyValue_NULL at the end with no exception and a \
     * nonnegative index; PyValue_NULL with a negative index and an \
     * exception set.  The targets, list_iteritem, _PyTuple_IterValue, \
     * bytes_iteritem and unicode_iteritem (and subtypes, which inherit \
     * them), read the sequence and run no Python; unicode_iteritem \
     * allocates a character and raises when that fails */ \
    SLOT(_Py_iteritemfunc, _PyValueIndexPair, (PyObject *, Py_ssize_t), \
         ARGS(object, scalar), \
         CASES(index<0 value(null) raised, index>=0 value(null), \
               index>=0 value(owned)), EFFECTS(reads, allocates, raises)) \
    FIELD(PyTypeObject, _tp_iteritem, _Py_iteritemfunc, ON(type), \
          SLOT(_Py_iteritemfunc), EFFECTS(reads)) \
    /* An object's inline values (pycore_object.h: at tp_basicsize; the \
     * caller has the type's Py_TPFLAGS_INLINE_VALUES) and their flag */ \
    PRIMITIVE(_PyObject_InlineValues, PyDictValues *, (PyObject *), \
              ARGS(object), PATH(inline_values), EFFECTS(reads)) \
    /* A plain read of the values' validity byte (Include/internal/ \
     * pycore_dict.h): no callee, no failure, no callback */ \
    FIELD(PyDictValues, valid, uint8_t, ON(inline_values), \
          TEST(has_values), EFFECTS(reads)) \
    /* The address of a value slot at a byte offset: arithmetic only */ \
    PRIMITIVE(_PyObject_ValueSlot, PyValue *, (PyObject *, size_t), \
              ARGS(object, scalar), SCALAR, EFFECTS()) \
    /* Evidence (G0d4), pycore_object.h, a static inline function: loads the \
     * slot and stores the value into it (a read and a write, no call, no \
     * failure, no callback).  The value moves into the slot, and the \
     * previous contents come out as an owner: PyValue_NULL for a slot never \
     * assigned, with no exception set (VALUE_OR_NULL, as _PyCell_SwapValue: \
     * the caller closes it with PyValue_XCLOSE, which runs the finalizer \
     * after any unlock). */ \
    PRIMITIVE(_PyValueSlot_Exchange, PyValue, (PyValue *, PyValue), \
              ARGS(scalar, consumed), VALUE_OR_NULL(owned), EFFECTS(reads, writes)) \
    /* Evidence (G0d4), pycore_value.h: _PyValueSlot_Exchange for a slot that \
     * holds a value.  The caller's guards show it: a shape names only filled \
     * slots (_GUARD_SHAPE), a list item is never NULL once published (the \
     * list's length bounds the index), and the hint's entry is tested for \
     * NULL before the store.  The function asserts it. */ \
    PRIMITIVE(_PyValueSlot_ExchangeFilled, PyValue, (PyValue *, PyValue), \
              ARGS(scalar, consumed), VALUE(owned assumed(nonnull) by guards), \
              EFFECTS(reads, writes)) \
    /* Evidence (G0d4), pycore_value.h: a store of the value into a slot \
     * that holds NULL (the shape guard shows the attribute is absent): it \
     * moves in, nothing is displaced, no call, no failure, no callback. */ \
    PRIMITIVE(_PyValueSlot_Fill, void, (PyValue *, PyValue), \
              ARGS(scalar, consumed), NONE, EFFECTS(writes)) \
    /* The shape of an object's inline values (pycore_dict.h: a read of \
     * _PyObject_InlineValues(obj)->shape) \
     * Evidence: pycore_dict.h returns _PyObject_InlineValues(obj)->shape\
     * (a load) with Py_SHAPES, else _Py_SHAPE_NONE; no callee beyond the\
     * inline-values address, no failure, no callback */ \
    PRIMITIVE(_PyObject_Shape, uint32_t, (PyObject *), \
              ARGS(object), SCALAR, EFFECTS(reads)) \
    /* The slot at an offset, a new count (pycore_dict.h): the shape guards \
     * have shown the slot filled, so the value is not NULL */ \
    PRIMITIVE(_PyObject_ShapeReadDup, PyValue, (PyObject *, uint32_t), \
              ARGS(object, scalar), VALUE(owned), EFFECTS(reads)) \
    /* The two writes of an object's inline values (pycore_dict.h) \
     * Evidence: pycore_dict.h appends ix to the values' insertion-order\
     * array and bumps values->size (two stores, the second a release\
     * store); no allocation, no callback; the capacity is asserted */ \
    PRIMITIVE(_PyDictValues_AddToInsertionOrder, void, (PyDictValues *, Py_ssize_t), \
              ARGS(scalar, scalar), NONE, EFFECTS(reads, writes)) \
    /* Evidence: pycore_dict.h stores values->shape (a no-op without \
     * Py_SHAPES); no callback */ \
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
    /* Evidence (G0d3): the release of a view's owner: XCLOSE of the owner \
     * (NULL for a local), or _PyValue_XCLOSE_POINTER for a value known to \
     * be in pointer space (pycore_value.h) */ \
    PRIMITIVE(PyValue_XCLOSE, void, (PyValue), \
              ARGS(consumed), NONE, EFFECTS(releases), HOMES(canonical), OBSERVER(none), \
              VIEW_CLOSE(PyValue_XCLOSE), POINTER_FORM(_PyValue_XCLOSE_POINTER)) \
    /* Evidence (G0d3): as PyValue_XCLOSE: a view's owner is released by \
     * XCLOSE, which tests for the NULL of a local */ \
    PRIMITIVE(PyValue_CLOSE, void, (PyValue), \
              ARGS(consumed nonnull), NONE, EFFECTS(releases), HOMES(canonical), OBSERVER(none), \
              VIEW_CLOSE(PyValue_XCLOSE), POINTER_FORM(_PyValue_XCLOSE_POINTER)) \
    /* Identities: a borrowed view of a value; in a build with the GIL the \
     * relaxed atomic loads are plain loads */ \
    INTRINSIC(PyValue_Borrow, ARGS(borrowed), VIEW, BUILD(all)) \
    /* pycore_pyatomic_ft_wrappers.h: the identity in the build with the \
     * GIL */ \
    INTRINSIC(FT_ATOMIC_LOAD_UINT_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* pycore_pyatomic_ft_wrappers.h: the identity in the build with the \
     * GIL */ \
    INTRINSIC(FT_ATOMIC_LOAD_UINT8, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* _PyErr_Format (Python/errors.c) -> _PyErr_FormatV: _PyErr_Clear \
     * releases the current exception (a finalizer); PyUnicode_FromFormatV \
     * allocates the message; _PyErr_SetObject -> _PyErr_CreateException \
     * calls the exception type, which runs Python for a class defined in \
     * Python, and chains the context (releases).  The row admits any \
     * type, so it declares runs_python */ \
    HELPER(_PyErr_Format, PyObject *, (PyThreadState *, PyObject *, const char *, ...), \
           ARGS(tstate, object, scalar), NONE, EFFECTS(runs_python)) \
    /* Evidence (Include/internal/pycore_stats.h; G0c batch 6).  ((void)0) \
     * unless Py_STATS, where it increments a counter of the statistics block \
     * (call_stats.name++), which is no program state.  No call, no failure, \
     * no callback. */ \
    MACRO(CALL_STAT_INC, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h:481: stores pause_backoff_counter(fb->counter) into the \
     * entry header (nothing for NULL; the free-threaded build skips unreachable \
     * counters).  A write of the counter only.  It was a do-while C macro; it is now the static inline function \
     * of that name (ceval_macros.h), so the row is type checked. */ \
    HELPER(PAUSE_ADAPTIVE_COUNTER, void, (_PyFbHeader *), \
        ARGS(scalar), NONE, EFFECTS(writes), FEEDBACK(write(counter))) \
    /* Objects/cellobject.c: PyObject_GC_New allocates (NULL with \
     * MemoryError set; _PyObject_GC_Link only schedules a collection for \
     * the eval breaker, it runs none), a count of the input is taken \
     * first (PyValue_XDUP of a borrowed word) and the failure path \
     * closes it (the input's home still holds a count, so it is not the \
     * last), then _PyObject_GC_TRACK.  Returns a new cell or NULL with \
     * an exception.  The input is borrowed.  runs_python over-declares \
     * (allocates and raises is the contract); G1 may narrow it.  G0d1: \
     * VALUE_OR_NULL(owned), the new cell (SCALAR hid it) */ \
    HELPER(PyCell_NewValue, PyObject *, (PyValueBorrow), \
        ARGS(borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (G0c batch 6): Python/errors.c PyErr_Format -> \
     * _PyErr_FormatV: formats the message (PyUnicode_FromFormatV, an \
     * allocation) and sets the exception with _PyErr_SetObject; always \
     * returns NULL with an exception set, or MemoryError. The exception \
     * class is an argument: the caller's call here passes the built-in \
     * TypeError, whose creation is C code, but the row cannot assume it, \
     * and an allocation may start a collection whose finalizers run \
     * Python. So `runs_python`. The arguments are borrowed. */ \
    HELPER(PyErr_Format, PyObject *, (PyObject *, const char *, ...), \
        ARGS(object, scalar), SCALAR, EFFECTS(runs_python)) \
    /* Python/errors.c.  Callees: PyExceptionInstance_Check, \
     * PyExceptionClass_Check and PyType_IsSubtype (reads the MRO, no \
     * call into Python), and, for a tuple at the recursion limit, \
     * _PyErr_GetRaisedException, _PyErr_SetString (RecursionError), \
     * PyErr_FormatUnraisable (calls sys.unraisablehook, Python) and \
     * _PyErr_SetRaisedException, which restores the pending exception. \
     * No path leaves an exception set, and neither argument is stolen: \
     * it returns 0 or 1.  The hook is the only route to Python. */ \
    HELPER(PyErr_GivenExceptionMatches, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python)) \
    /* Reviewed G0c2 (Python/errors.c): Py_XSETREF of                         \
     * tstate->exc_info->exc_value with a new reference to the argument       \
     * (borrowed here, None meaning NULL). The old handled exception is       \
     * released, and its finalizer may run Python: runs_python covers it.     \
     * Cannot fail.                                                           */ \
    HELPER(PyErr_SetHandledException, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS(runs_python)) \
    /* Include/pyerrors.h: `_PyObject_CAST(Py_TYPE(x))`, a load of ob_type; \
     * no call, no failure, no ownership (a borrowed view of the class). */ \
    MACRO(PyExceptionInstance_Class, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Objects/exceptions.c: `Py_XNewRef(self->traceback)` under a \
     * critical section (free-threaded only).  A new reference or NULL \
     * with no exception set.  No callee that runs Python. */ \
    HELPER(PyException_GetTraceback, PyObject *, (PyObject *), \
        ARGS(object), VALUE_OR_NULL(owned), EFFECTS(reads)) \
    /* pycore_lazyimportobject.h (a static inline function): \
     * Py_IS_TYPE(op, &PyLazyImport_Type), a read of ob_type; no callee, \
     * no failure */ \
    /* Evidence (G0d3), V11: no leaf uop calls it, so it has no leaf model and \
     * no lowering, and its uops run as their handler's function form: a read of ob_type */ \
    HELPER(PyLazyImport_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyLazyImport_Type)), EFFECTS(reads)) \
    /* Objects/object.c PyObject_HasAttrWithError -> _PyValue_GetOptionalAttr: \
     * __getattribute__ and __getattr__ run Python.  Returns 1 (found), 0 \
     * (AttributeError cleared) or -1 with an exception set; the found value \
     * is closed inside, and both arguments are borrowed */ \
    HELPER(PyObject_HasAttrWithError, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python, raises)) \
    /* Include/object.h: PyType_FastSubclass(Py_TYPE(op), \
     * Py_TPFLAGS_TYPE_SUBCLASS), reads of ob_type and tp_flags; no \
     * callee, no failure, no callback */ \
    HELPER(PyType_Check, int, (PyObject *), \
        ARGS(object), TEST(subtype(PyType_Type)), EFFECTS(reads)) \
    /* Evidence (G0c batch 6): Objects/typeobject.c PyType_GenericAlloc -> \
     * _PyType_AllocNoTrack: allocates a zeroed object of type (inline \
     * values too) and tracks it if the type is GC. NULL with MemoryError \
     * on failure. Callbacks: none of its own, but an allocation may start \
     * a collection and its finalizers may run Python, so `runs_python`. */ \
    HELPER(PyType_GenericAlloc, PyObject *, (PyTypeObject *, Py_ssize_t), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Include/object.h): a static inline read of              \
     * type->tp_flags. No failure, no callback. A static inline function,     \
     * not an out-of-line HELPER: G0d kind.                                   */ \
    HELPER(PyType_HasFeature, int, (PyTypeObject *, long unsigned int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* Reviewed G0c2 (Include/cpython/unicodeobject.h): the address of the    \
     * str's characters, computed from its state (compact ASCII or compact    \
     * or legacy layout). Reads the state bits; no failure, no callback.      */ \
    MACRO(PyUnicode_1BYTE_DATA, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Reviewed G0c2 (Objects/unicodeobject.c). ensure_unicode(left) and a    \
     * PyUnicode_Check of right set TypeError; the sum of lengths             \
     * overflowing sets OverflowError; PyUnicode_New can set MemoryError:     \
     * each returns NULL with the exception set, with both arguments          \
     * untouched (borrowed). An empty operand gives PyUnicode_FromObject of   \
     * the other (a new reference). It copies characters and calls no         \
     * Python and releases nothing. The seed lacked `raises`.                 */ \
    HELPER(PyUnicode_Concat, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(reads, allocates, raises)) \
    /* Reviewed G0c2 (Include/cpython/unicodeobject.h): a read of the str's   \
     * length field (asserts PyUnicode_Check). No failure, no callback.       \
     * Inline, not an out-of-line HELPER: G0d kind.                           */ \
    HELPER(PyUnicode_GET_LENGTH, Py_ssize_t, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* Reviewed G0c2 (Include/cpython/unicodeobject.h): a read of the str's   \
     * state bits. No failure, no callback. Inline: G0d kind.                 */ \
    HELPER(PyUnicode_IS_COMPACT_ASCII, int, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* Reviewed G0c2 (Include/cpython/unicodeobject.h): reads the character   \
     * of a ready str by its kind; index must be in range (asserted). No      \
     * failure, no callback. A macro over inline functions: G0d kind.         */ \
    HELPER(PyUnicode_READ_CHAR, Py_UCS4, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    /* Reviewed G0c2 (Include/pymacro.h): sizeof(array)/sizeof(array[0]), a   \
     * compile-time constant; no code.                                        */ \
    MACRO(Py_ARRAY_LENGTH, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Include/object.h: drops a count, and on zero calls the \
     * deallocator, which can run a finalizer, weakref callbacks and \
     * __del__ (releases).  A C macro over public API: not converted.  \
     * G0d1: it consumes its argument's owner (SCALAR said it took none) */ \
    MACRO(Py_DECREF, \
        ARGS(consumed object), NONE, EFFECTS(releases), EXITS()) \
    /* Reviewed (G0c batch 1).  Py_FatalError(msg) is a C macro of \
     * Include/cpython/pyerrors.h: `_Py_FatalErrorFunc(__func__, msg)`, a \
     * _Py_NO_RETURN function (Python/pylifecycle.c) that prints the \
     * message and the traceback of every thread, then aborts the \
     * process.  It cannot become a function (it names __func__).  It \
     * never returns: its one exit is abort (G0d1), the declared exit of \
     * the handlers that never run */ \
    MACRO(Py_FatalError, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS(abort)) \
    /* Include/refcount.h, a static inline function: an immortality test \
     * and an increment; no call, no failure, no callback.  Free-threaded \
     * it is atomic, outside the scoped product. */ \
    HELPER(Py_INCREF, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS()) \
    /* Evidence (G0c batch 6): Include/object.h: Py_IS_TYPE(ob, type): a \
     * load of ob->ob_type compared with the address of type. No call, no \
     * failure, no callback. True of an object means exact(type) as the \
     * row's TEST(exact(a1)). */ \
    MACRO(Py_IS_TYPE, \
        ARGS(scalar, scalar), TEST(exact(a1), ), EFFECTS(reads), EXITS()) \
    /* Reviewed G0c2 (Include/object.h): Py_Is(x, Py_None), a pointer         \
     * compare with the None singleton; it reads no memory, so it declares    \
     * no effect (G0c9; the seed's `reads` was over-declared).                */ \
    MACRO(Py_IsNone, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0d2): Include/pyerrors.h: PyType_FastSubclass(Py_TYPE(x), \
     * Py_TPFLAGS_BASE_EXC_SUBCLASS), a load of the type's flags: no call \
     * into other code, no write.  Every use in bytecodes.c is in an assert. */ \
    MACRO(PyExceptionInstance_Check, \
        ARGS(object), SCALAR, EFFECTS(reads), EXITS()) \
    /* Include/refcount.h: `_Py_NewRef(_PyObject_CAST(obj))`, an increment \
     * (a count change is no effect, 1.3) returning obj: the result is a \
     * new reference the caller owes.  No call (immortal check inline), no \
     * failure.  G0d1: VALUE(owned), the new reference (SCALAR hid it) */ \
    MACRO(Py_NewRef, \
        ARGS(scalar), VALUE(owned), EFFECTS(), EXITS()) \
    /* Include/refcount.h: `_Py_XNewRef(_PyObject_CAST(obj))`, Py_NewRef \
     * with a NULL test: a new reference the caller owes, NULL exactly \
     * where obj is.  No call, no failure (G0d1: _RAISE_VARARGS's owners) */ \
    MACRO(Py_XNewRef, \
        ARGS(borrowed), VALUE(owned null_iff(0)), EFFECTS(), EXITS()) \
    /* Include/object.h: ob_size = size, a store through the object.  \
     * The seed said runs_python; the store runs nothing, so writes */ \
    MACRO(Py_SET_SIZE, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(writes), EXITS()) \
    /* Include/object.h: a read of ob_size */ \
    MACRO(Py_SIZE, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Include/object.h: as Py_DECREF, with a NULL test.  A C macro \
     * over public API: not converted.  G0d1: it consumes its argument's \
     * owner, which may be NULL */ \
    MACRO(Py_XDECREF, \
        ARGS(consumed), NONE, EFFECTS(releases), EXITS()) \
    /* Evidence (G0c batch 6): Include/refcount.h: a count increment of op \
     * if it is not NULL (and not immortal); no call, no failure, no \
     * callback. A count change is an ownership event, not an effect (the \
     * walk balances it): do_raise's XINCREF of exc and cause makes the \
     * counts that it then consumes. */ \
    HELPER(Py_XINCREF, void, (PyObject *), \
        ARGS(object), NONE, EFFECTS()) \
    /* Include/refcount.h: Py_XSETREF(dst, src) stores src into the lvalue \
     * dst, then Py_XDECREFs dst's old value.  It takes src's reference \
     * (consumed; src may be NULL, the X: ARGS has no nullable mark, the walk accepts a literal NULL there because it names no value and refuses a local that may be NULL) and returns nothing.  The old \
     * value's finalizer may run Python (releases).  Failure paths: none. \
     * Callbacks: the old value's deallocator.  Root's correction; the only \
     * uop caller is _POP_EXCEPT (G0a5) */ \
    MACRO(Py_XSETREF, \
        ARGS(lvalue, consumed object), NONE, EFFECTS(releases), EXITS()) \
    /* Include/refcount.h: Py_CLEAR(op) sets the lvalue op to NULL, then \
     * Py_DECREFs its old value if it was not NULL.  The old value's \
     * finalizer may run Python (releases).  No failure; callbacks: the old \
     * value's deallocator.  _POP_EXCEPT clears the handled exception */ \
    MACRO(Py_CLEAR, \
        ARGS(lvalue), NONE, EFFECTS(releases), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h:255-257: in the GIL build the macro is empty (no \
     * statement); in the free-threaded build it reports the thread's quiescent state to \
     * the QSBR reclaimer, which this scoped GIL product never builds. */ \
    INTRINSIC(QSBR_QUIESCENT_STATE, ARGS(scalar), NONE, BUILD(gil)) \
    /* generators_common.py's expansion: stores the stack pointer \
     * back for the code that follows.  No C call, no effect */ \
    MACRO(SYNC_SP, \
        ARGS(), NONE, EFFECTS(), EXITS()) \
    /* pycore_cell.h: a new owner of the cell's value (PyValue_XDUP: a count \
     * only, no allocation, no callback), PyValue_NULL for an empty cell.  \
     * No exception is set on that NULL: the caller says what an empty cell \
     * means (LOAD_DEREF raises NameError).  VALUE_OR_NULL is the closest \
     * shape: the caller tests the result before it uses it */ \
    HELPER(_PyCell_GetValue, PyValue, (PyCellObject *), \
        ARGS(scalar), VALUE_OR_NULL(owned), EFFECTS(reads)) \
    /* pycore_cell.h: _PyCell_SwapValue (a store of the value into the \
     * cell: it moves in) and then PyValue_XCLOSE of the previous value \
     * (releases: the previous owner's finalizer can run Python).  The \
     * value is consumed on every return; there is no failure path */ \
    HELPER(_PyCell_SetValue, void, (PyCellObject *, PyValue), \
        ARGS(scalar, consumed), NONE, EFFECTS(writes, releases)) \
    /* pycore_cell.h: stores the value into cell->ob_ref (it moves in) \
     * and returns the previous contents as an owner: PyValue_NULL for an \
     * empty cell, with no exception set (VALUE_OR_NULL, as \
     * _PyCell_GetValue: the caller says what an empty cell means).  No \
     * release and no callback: the previous owner is the caller's to \
     * close.  The value is consumed */ \
    HELPER(_PyCell_SwapValue, PyValue, (PyCellObject *, PyValue), \
        ARGS(scalar, consumed), VALUE_OR_NULL(owned), EFFECTS(writes)) \
    /* Evidence (G0c8): pycore_instruments.h:161: `line_data->data[index * bytes_per_entry]`, \
     * a read of the code's monitoring line data.  No callee; the caller (INSTRUMENTED_LINE \
     * while tracing) passes the index of this_instr. */ \
    HELPER(_PyCode_GetOriginalOpcode, uint8_t, (_PyCoLineInstrumentationData *, int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* Include/internal/pycore_ir.h, a static inline function: \
     * IR_HANDLER(_PyCode_IRWords(co)[i]), a load of the code object's IR \
     * word.  No call, no failure, no write.  Its only caller in this \
     * batch is the Py_DEBUG assertion of _YIELD_VALUE. */ \
    HELPER(_PyCode_InstrHandler, int, (PyCodeObject *, int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* Python/errors.c _PyErr_ChainExceptions1 -> \
     * _PyErr_ChainExceptions1Tstate: it steals the reference to exc, sets \
     * exc as the context of the exception now raised (PyException_SetContext) \
     * or restores exc as the raised exception.  _PyErr_SetRaisedException \
     * releases the exception it replaces, whose finalizer may run Python */ \
    HELPER(_PyErr_ChainExceptions1, void, (PyObject *), \
        ARGS(consumed object), NONE, EFFECTS(runs_python)) \
    /* Python/errors.c _PyErr_Clear -> _PyErr_Restore(NULL): releases the \
     * raised exception, whose deallocator can run Python (releases).  No \
     * failure path */ \
    HELPER(_PyErr_Clear, void, (PyThreadState *), \
        ARGS(tstate), NONE, EFFECTS(releases)) \
    /* Python/errors.c _PyErr_ExceptionMatches -> \
     * PyErr_GivenExceptionMatches: a subtype test through tp_mro / \
     * tp_bases and, for a tuple, each member; reads only, no allocation and \
     * no callback.  Returns 0 or 1 */ \
    HELPER(_PyErr_ExceptionMatches, int, (PyThreadState *, PyObject *), \
        ARGS(tstate, object), SCALAR, EFFECTS(reads)) \
    /* Python/errors.c _PyErr_GetRaisedException: moves \
     * tstate->current_exception to the caller (a new reference; the \
     * thread's field becomes NULL).  It reads and writes the thread state \
     * and calls nothing, so writes, not runs_python (the seed's) */ \
    HELPER(_PyErr_GetRaisedException, PyObject *, (PyThreadState *), \
        ARGS(tstate), VALUE_OR_NULL(owned), EFFECTS(reads, writes)) \
    /* pycore_pyerrors.h: moves the handled exception's reference out of \
     * the stack item (the field becomes NULL) and returns it, or NULL when \
     * none is handled.  No callee, no failure, no callback (G0d1: \
     * _PUSH_EXC_INFO's move out of a heap field, for \
     * PyValue_FromObjectSteal's moved(0)) */ \
    HELPER(_PyErr_StackItemTakeValue, PyObject *, (_PyErr_StackItem *), \
        ARGS(scalar), VALUE_OR_NULL(owned), EFFECTS(reads, writes)) \
    /* exceptions.c: a new owner of a StopIteration's value, None for \
     * PyValue_NULL (the member getter's reading; StopIteration.__new__ \
     * without __init__ leaves it NULL).  One load of the field and a \
     * count; no failure, no callback.  Its argument is a StopIteration */ \
    HELPER(_PyStopIteration_GetValue, PyValue, (PyObject *), \
        ARGS(object), VALUE(owned), EFFECTS(reads)) \
    /* pycore_pyerrors.h: reads tstate->current_exception; no callee, no \
     * callback */ \
    HELPER(_PyErr_Occurred, PyObject *, (PyThreadState *), \
        ARGS(tstate), SCALAR, EFFECTS(reads)) \
    /* Python/errors.c: `old = tstate->current_exception; \
     * tstate->current_exception = exc; Py_XDECREF(old)`.  It steals exc \
     * on its only path: the reference moves into the thread state.  exc \
     * may be NULL, which clears the exception: a literal NULL names no \
     * value and owes nothing (as for Py_XSETREF).  The displaced \
     * exception is released, and its destructor may run Python.  No \
     * failure path. */ \
    HELPER(_PyErr_SetRaisedException, void, (PyThreadState *, PyObject *), \
        ARGS(tstate, consumed object), NONE, EFFECTS(runs_python)) \
    /* Python/errors.c: _PyErr_SetObject with a new str message: \
     * PyUnicode_FromString allocates, _PyErr_CreateException calls the \
     * exception type (Python code for a class defined in Python), the \
     * context chain releases the exception that is being handled (a \
     * finalizer).  The row admits any exception type, so it declares \
     * runs_python, as _PyErr_Format */ \
    HELPER(_PyErr_SetString, void, (PyThreadState *, PyObject *, const char *), \
        ARGS(tstate, object, scalar), NONE, EFFECTS(runs_python)) \
    /* Objects/interpolationobject.c \
     * (_PyInterpolation_BuildValue): PyValue_AsObject boxes an inline \
     * number (allocation, MemoryError), _PyInterpolation_Build allocates \
     * the object, takes counts of value, str and format_spec, and for a \
     * bad conversion code raises SystemError and drops the object it just \
     * made; the box is dropped after.  No type slot or finalizer of user \
     * code is reached (the objects dropped hold counts the caller keeps).  \
     * Failure: NULL with an exception set; the arguments are borrowed.  \
     * Narrowed from the seed's runs_python to allocates, raises */ \
    HELPER(_PyInterpolation_BuildValue, PyObject *, (PyValueBorrow, PyObject *, int, PyObject *), \
        ARGS(borrowed, object, scalar, object), VALUE_OR_NULL(owned), EFFECTS(allocates, raises)) \
    /* free-threaded builds only (pycore_gc.h): reads the object's GC \
     * bits.  Not a component in the build with the GIL \
     * (BUILD(free_threaded)) */ \
    INTRINSIC(_PyObject_GC_IS_SHARED, ARGS(scalar), SCALAR, BUILD(free_threaded)) \
    /* pycore_object.h: a load of the object's managed-dict pointer (an \
     * acquire load in the free-threaded build); no callee, no callback */ \
    HELPER(_PyObject_GetManagedDict, PyDictObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* pycore_object.h: Py_REFCNT(ob) == 1 with the GIL; a read of the \
     * count, no callback */ \
    HELPER(_PyObject_IsUniquelyReferenced, int, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* pycore_dict.h: reads the type's heap-type flags, ht_cached_keys \
     * and its shape table; never grows it and never sets an exception \
     * (the loop scans the shapes after FROM for a child with the slot). \
     * No allocation, no callback */ \
    HELPER(_PyShape_FindTransition, uint8_t, (PyTypeObject *, uint8_t, Py_ssize_t), \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* pycore_dict.h: _PyShape_FindTransition and \
     * _PyType_Shapes(tp)->nshapes; reads only, no exception */ \
    HELPER(_PyShape_TransitionMissing, bool, (PyTypeObject *, uint8_t, Py_ssize_t), \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(reads)) \
    /* pycore_value_ops.h: status 1 an unbound method, 0 the attribute, -1 an error (G0a4: a CASES result) \
     * Evidence: Objects/typeobject.c: supercheck (TypeError on a bad\
     * object), do_super_lookup walks the MRO after su_type and calls a\
     * descriptor's tp_descr_get (a property or any descriptor runs\
     * Python); status -1 with an exception, 0 an attribute, 1 an unbound\
     * method; the owned value is the descriptor or the attribute.\
     * Arguments borrowed */ \
    HELPER(_PySuper_LookupValue, _PyStatusValue, (PyTypeObject *, PyValueBorrow, PyObject *, int), \
        ARGS(scalar, borrowed, object, scalar), \
        CASES(status==-1 value(null) raised, status==0 value(owned), status==1 value(owned)), EFFECTS(runs_python, raises)) \
    /* Objects/templateobject.c _PyTemplate_Build.  Callees: PyObject_GC_New \
     * (gc_alloc: MemoryError on failure, and _PyObject_GC_Link, which only \
     * schedules a collection through the eval breaker), Py_NewRef of both \
     * arguments, PyObject_GC_Track.  Failure: NULL with MemoryError set; the \
     * arguments are borrowed and untouched (the new object holds its own \
     * counts, so the caller still owns its references).  Callbacks: none; \
     * nothing is released and no type slot or finalizer is called.  Result: \
     * a new reference to a Template (the caller boxes it with \
     * PyValue_FromObjectSteal) */ \
    HELPER(_PyTemplate_Build, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(allocates, raises)) \
    /* G0d2.  Objects/funcobject.c: `return ((classmethod *)self)->cm_callable`, \
     * a load of the wrapped callable, borrowed from the classmethod: no call, no \
     * failure, no write.  Its caller was a name in analyzer.NON_ESCAPING_FUNCTIONS. */ \
    HELPER(_PyClassMethod_GetFunc, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* G0d1.  Include/internal/pycore_descrobject.h:65: the getter of a \
     * getset descriptor with a _PyValueGetSet record (d_value), read before \
     * the call, run on self: record->get(self, record->closure).  The getter \
     * is any type's C code, which may run Python and raise; the result is a \
     * new value, or NULL with an exception set.  descr is not used after the \
     * call; self is borrowed */ \
    HELPER(_PyGetSet_ValueRead, PyValue, (PyObject *, PyValueBorrow), \
        ARGS(object, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c5.  pycore_typeobject.h: type->_tp_iteritem != NULL && \
     * type->tp_iter != _PyType_SlotIter, two loads of the type and an \
     * address compare.  Callees: none.  It cannot fail and runs no \
     * callback; the result tells whether the iterator indexes its \
     * sequence (an _tp_iteritem cursor) or steps by iternext */ \
    HELPER(_PyType_IteratesByIndex, int, (PyTypeObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* pycore_value_slots.h: a cast of the type's tp_value.<field> \
     * pointer to the slot's C type: a read of the type, no callee, no \
     * failure, no callback.  The typed slots are the SLOT and FIELD rows */ \
    MACRO(_PyType_VALUE_SLOT, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Reviewed G0c2 (Objects/unicodeobject.c): identity, else unicode_eq     \
     * compares length, kind and bytes of two str (asserted). Reads;          \
     * returns 0 or 1; no failure, no allocation, no callback.                */ \
    HELPER(_PyUnicode_Equal, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(reads)) \
    /* Evidence (G0c8): pycore_emscripten_signal.h:25-26: the macro is empty outside \
     * __EMSCRIPTEN__, the only build that polls browser signals; the GIL x86-64 \
     * product has no statement here. */ \
    INTRINSIC(_Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY, ARGS(), NONE, BUILD(gil)) \
    /* Include/internal/pycore_object.h defines a static inline function, \
     * but Python/ceval.h #undefs it and redefines it as a do { } while (0) \
     * statement macro (a constant destructor call): the evaluator's C is \
     * the macro, so the row stays a MACRO and has no static type check. \
     * Converting it to the inline function is a change to ceval.h and a \
     * measurement: listed, not done.  Semantics: drops one count of op \
     * unless it is immortal and calls the destructor at zero.  Consumes \
     * op; no failure path.  Callbacks: only the destructor, whose own row \
     * decides the effect (a DESTRUCTOR row is `writes`, any other \
     * `releases`), as for PyValue_CLOSE_SPECIALIZED.  The reftracer hook \
     * is a debugging facility outside the language semantics */ \
    MACRO(_Py_DECREF_SPECIALIZED, \
        ARGS(consumed object, destructor), NONE, EFFECTS(writes), EXITS()) \
    /* Include/internal/pycore_global_strings.h: the address of a static \
     * immortal string, `&_Py_SINGLETON(...)`: an arithmetic constant.  A C \
     * macro over a table: not converted */ \
    MACRO(_Py_ID, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* free-threaded builds only (pycore_object.h): reads the object's \
     * owner thread id.  Not a component in the build with the GIL \
     * (BUILD(free_threaded)) */ \
    INTRINSIC(_Py_IsOwnedByCurrentThread, ARGS(scalar), SCALAR, BUILD(free_threaded)) \
    /* Reviewed G0c2 (pycore_global_objects.h): the address of a static       \
     * object in _PyRuntime's global objects; a constant address, no memory   \
     * read.                                                                  */ \
    MACRO(_Py_SINGLETON, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Reviewed (G0c batch 1).  _Py_STR(name) is a C macro of \
     * Include/internal/pycore_global_strings.h: the address of a static \
     * string object in the runtime's global table (an address constant): \
     * no memory is read, nothing can fail.  It cannot be a function: the \
     * argument is a token pasted into a member name */ \
    MACRO(_Py_STR, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (Python/ceval.h do_raise; G0c batch 6).  It consumes a count \
     * of exc and of cause on every return (its comment; each path closes \
     * them: Py_DECREF(exc) for a non-exception, Py_DECREF(value and type) \
     * after _PyErr_SetObject, raise_error's Py_XDECREF).  A bare raise \
     * (exc NULL) re-raises the topmost exception and returns 1 with it \
     * set; every other path returns 0 with an exception set (a number \
     * cause is the flag, with cause NULL): the failure is the exception, \
     * not the return value.  Callbacks: _PyErr_CreateExceptionFromArray \
     * calls the exception class (Python code: __new__, __init__), \
     * PyException_SetCause and the decrefs may run finalizers: \
     * `runs_python`.  The seed said `object`, borrowed, and no effects. */ \
    HELPER(do_raise, int, (PyThreadState *, PyObject *, PyObject *, bool), \
        ARGS(tstate, consumed object, consumed object, scalar), SCALAR, EFFECTS(runs_python)) \
    /* Evidence (G0c batch 6): Python/ceval.c: native_call_builtin_o: opens \
     * a _PyNativeScope on arg (the home, read in place), calls \
     * _PyCFunction_CallScoped on the callable, publishes the result or \
     * aborts the scope, finishes the publication (which may run Python and \
     * re-enter the frame). It never closes the callable or the argument: \
     * call_publish does that after the handler publishes the result. NULL \
     * with an exception set on failure; the result is a new reference. The \
     * callee and the scope's callbacks run arbitrary Python: \
     * `runs_python`. The seed said `consumed`. */ \
    HELPER(native_call_builtin_o, PyValue, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValue, PyValue *), \
        ARGS(tstate, frame, scalar, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: takes a count of class for the argument vector \
     * {class, self}, calls the super type through _PyValue_Vectorcall \
     * (the vector is the caller's window: arguments are not consumed), \
     * closes class's count and returns the new super object or \
     * PyValue_NULL with an exception.  `self` is read and not closed: \
     * the caller's DECREF_INPUTS closes it (the seed declared it \
     * consumed) */ \
    HELPER(super_call, PyValue, (PyValueBorrow, PyValueBorrow, PyValue, int), \
        ARGS(borrowed, borrowed, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c value_dict_merge_uniq -> _PyDict_MergeUniq(dict, update, \
     * &dupkey): inserts each item of update into dict, stopping at a key \
     * dict already holds.  A non-dict update is read through keys() and \
     * __getitem__, and keys are hashed and compared: Python.  Status >= 0 \
     * is success.  Status < 0 is failure with an exception set (KeyError \
     * for a duplicate, with the new reference to the duplicate key in \
     * .object; another error leaves .object NULL).  The caller owns \
     * .object and releases it (X form).  Both arguments are borrowed */ */ \
    HELPER(value_dict_merge_uniq, _PyStatusObject, (PyObject *, PyObject *), \
        ARGS(object, object), \
        CASES(status<0 object(owned nullable) raised, status>=0 object(null)), \
        EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c). Wraps _PyEval_ExceptionGroupMatch      \
     * (exception_group_match: isinstance checks, split()/derive of the       \
     * group: arbitrary Python). Returns {match, rest} as new references,     \
     * or {NULL, NULL} with an exception set. The frame is only read for      \
     * its f_exc state; exc_value and match_type are borrowed.                */ \
    HELPER(value_exception_group_match, _PyObjectPair, (_PyInterpreterFrame *, PyObject *, PyObject *), \
        ARGS(frame, object, object), \
        CASES(match(null) rest(null) raised, match(owned) rest(owned)), \
        EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: an inline int in range is returned as a C int with \
     * no allocation; otherwise PyValue_AsObject boxes the level, \
     * _PyValue_AsInt (PyLong_AsInt: __index__ for a non-int, \
     * OverflowError) and a release of the box.  -1 with an exception on \
     * error.  The argument is not closed (the seed declared it consumed: \
     * IMPORT_NAME's DECREF_INPUTS closes it).  The compiler always \
     * passes an int constant, so in practice no Python runs */ \
    HELPER(value_import_level, int, (PyValue), \
        ARGS(borrowed), SCALAR, EFFECTS(runs_python)) \
    /* ceval.c: as _PyValue_GetOptionalItem, -1 if the lazy import fails (G0a4: a CASES result) */ \
    HELPER(value_load_from_dict, _PyStatusValue, (PyThreadState *, PyObject *, PyObject *), \
        ARGS(tstate, object, object), \
        CASES(status==-1 value(null) raised, status==0 value(null), status==1 value(owned)), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: _PyEval_LoadGlobalValue through a local out- \
     * parameter: the exact-dict fast path _PyDict_LoadGlobalValue (hash \
     * of an exact str, no user code) or mapping lookups \
     * (PyMapping_GetOptionalItem: __getitem__ runs Python), NameError on \
     * a miss, and a lazy import in the result is reified \
     * (_PyLazyImport_Reify imports).  Returns an owned value or \
     * PyValue_NULL with an exception set.  Arguments borrowed */ \
    HELPER(value_load_global, PyValue, (PyObject *, PyObject *, PyObject *), \
        ARGS(object, object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c). An exact dict goes to                  \
     * _PyDict_GetValue (hashes and compares the key: __hash__ and __eq__     \
     * of key types run Python), a miss sets KeyError through                 \
     * _PyErr_SetKeyErrorValue and returns NULL. An exact list or tuple       \
     * with an in-range inline index reads the item, else _PyValue_GetItem    \
     * -> mp_subscript / sq_item / __class_getitem__ (any Python). Operands   \
     * borrowed on every path; the result owned or NULL with an exception     \
     * set.                                                                   */ \
    HELPER(value_subscript, PyValue, (PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: PyUnicode_Append(&left, right) in a frame of its \
     * own.  It consumes left on every return (extends it in place when \
     * its count allows, else replaces it with a new str and releases it, \
     * and releases it on error) and borrows right; it returns the new str \
     * owned, or NULL with an exception set.  Callees: PyUnicode_Append -> \
     * unicode_resize / PyUnicode_New and a copy: allocation and a realloc \
     * that may move the object, and the release of left, a str, whose \
     * deallocator runs no Python; no slot of either operand is called.  \
     * `releases` is declared for the deallocator of left, and runs_python \
     * kept as the seed had it: nothing here calls Python, so it is \
     * over-declared and sound */ \
    HELPER(value_unicode_append, PyObject *, (PyObject *, PyObject *), \
        ARGS(consumed object, object), VALUE_OR_NULL(owned), \
        EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: builds a four-slot window of borrowed words and \
     * calls _PyValue_Vectorcall(exit_func, ...), which runs any callable \
     * (an inline exit_func is rejected with TypeError).  All five \
     * arguments are borrowed (the window holds views).  Failure: NULL \
     * with an exception set; success: a new reference. */ \
    HELPER(with_exit_call, PyValue, (PyValueBorrow, PyValueBorrow, PyObject *, PyObject *, PyObject *), \
        ARGS(borrowed, borrowed, object, object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Objects/unicodeobject.c: frees an exact str's memory (the \
     * interned-state and the cached utf8 buffer are freed too) and \
     * releases no other object */ \
    DESTRUCTOR(_PyUnicode_ExactDealloc, PyUnicode_Type, callback_free)

/* Containers */
#define _PyIR_COMPONENTS_CONTAINERS(PRIMITIVE, HELPER, MACRO) \
    /* Include/cpython/listobject.h: Py_IS_TYPE(op, &PyList_Type), a \
     * read of ob_type compared with a static address.  No callee, no \
     * failure.  A static type cannot be reassigned by __class__, so the \
     * TEST's fact is a value fact */ \
    PRIMITIVE(PyList_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyList_Type)), EFFECTS(reads)) \
    /* Include/cpython/tupleobject.h: Py_IS_TYPE(op, &PyTuple_Type); \
     * as PyList_CheckExact */ \
    PRIMITIVE(PyTuple_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyTuple_Type)), EFFECTS(reads)) \
    /* Py_IS_TYPE of the named type: a read of ob_type */ \
    PRIMITIVE(PyDict_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyDict_Type)), EFFECTS(reads)) \
    /* Py_IS_TYPE(op, &PyModule_Type): a read of ob_type; no callee, no \
     * failure */ \
    PRIMITIVE(PyModule_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyModule_Type)), EFFECTS(reads)) \
    /* Include/cpython/unicodeobject.h: Py_IS_TYPE(op, &PyUnicode_Type); \
     * as PyList_CheckExact */ \
    PRIMITIVE(PyUnicode_CheckExact, int, (PyObject *), \
              ARGS(object), TEST(exact(PyUnicode_Type)), EFFECTS(reads)) \
    /* Evidence (G0c batch 6): Include/cpython/funcobject.h: Py_IS_TYPE(op, \
     * &PyFunction_Type), an exact-type test of a load of ob_type; no call, \
     * no failure, no callback. True means exact(PyFunction_Type); \
     * PyFunction_Type is immutable, so `__class__` assignment is refused \
     * and the fact is a value fact. */ \
    PRIMITIVE(PyFunction_Check, int, (PyObject *), \
              ARGS(object), TEST(exact(PyFunction_Type)), EFFECTS(reads)) \
    /* An element of a list or tuple: a borrow rooted by the container \
     * (pycore_list.h, pycore_tuple.h).  A tuple under construction holds \
     * NULL, so no fact is declared of it.  Reviewed G0c5 for \
     * PyList_GET_VALUE: a load of ob_item[index] (a read), no bounds check \
     * (the caller bounds the index: _ITER_JUMP_LIST), no callees, cannot \
     * fail, no callback.  A list built by PyList_New(n) holds NULL until \
     * filled; _ITER_NEXT_LIST reads only the items of a published list, \
     * whose items are set (the same trust as for a tuple).  Reviewed (G0c batch 1): \
     * PyTuple_GET_VALUE (pycore_tuple.h) is `PyValue_Borrow(tuple->ob_item \
     * [index])` with an assert of the bounds: one load of an item (reads), \
     * no call, no callback, no failure.  The result is a borrow rooted by \
     * the tuple; a caller that can run Python before using it takes a \
     * count first (the header comment).  No nonnull fact: an \
     * `assumed(nonnull)` result would need a row of its own for the code \
     * object's constants (_LOAD_CONST is left for it) */ \
    PRIMITIVE(PyList_GET_VALUE, PyValueBorrow, (PyObject *, Py_ssize_t), \
              ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    PRIMITIVE(PyTuple_GET_VALUE, PyValueBorrow, (PyObject *, Py_ssize_t), \
              ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    /* pycore_long.h: the sign bits of lv_tag.  Reviewed (G0c batch 1): \
     * `(op->long_value.lv_tag & SIGN_MASK) == SIGN_ZERO`, one load of the \
     * int's header (reads); no call, no failure.  The argument is a \
     * boxed int: an inline int has no PyLongObject */ \
    PRIMITIVE(_PyLong_IsZero, bool, (const PyLongObject *), \
              ARGS(object), TEST(), EFFECTS(reads)) \
    /* pycore_dict.h: the address of the entries after the index table, \
     * from dk_log2_index_bytes */ \
    PRIMITIVE(DK_UNICODE_ENTRIES, PyDictUnicodeEntry *, (PyDictKeysObject *), \
              ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* Include/cpython/listobject.h: a read of ob_size (the length \
     * changes at any write to the list, so facts built on it are heap \
     * facts); no callee, no failure */ \
    PRIMITIVE(PyList_GET_SIZE, Py_ssize_t, (PyObject *), \
              ARGS(object), MEASURE(length), EFFECTS(reads)) \
    /* Include/cpython/tupleobject.h: a read of ob_size, which a tuple \
     * never changes after construction; no callee, no failure */ \
    PRIMITIVE(PyTuple_GET_SIZE, Py_ssize_t, (PyObject *), \
              ARGS(object), MEASURE(length), EFFECTS(reads)) \
    /* Include/internal/pycore_list.h: the address of ob_item, a read \
     * of the list; the array it names is reallocated by any resize, so a \
     * pointer from it dies at every write.  No callee, no failure */ \
    PRIMITIVE(_PyList_ITEMS, PyValue *, (PyObject *), \
              ARGS(object), SCALAR, EFFECTS(reads)) \
    /* Include/internal/pycore_tuple.h: the address of the inline \
     * ob_item array; no callee, no failure.  No model or lowering exists \
     * (V11): listed for G0d, as it is called only from non-leaf uops */ \
    /* Include/internal/pycore_tuple.h (a macro): the address of the inline \
     * ob_item array: an offset from the tuple, no read of its own, no \
     * callee, no failure.  G0d3: modelled (ir_leaf.py) and lowered \
     * (jit_lower.c), V11 */ \
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
           EFFECTS(runs_python, raises)) \
    /* Evidence (G0d3), pycore_critical_section.h: a free-threaded build's \
     * critical section.  With the GIL, LOCK_OBJECT(op) is the constant 1 and \
     * UNLOCK_OBJECT(op) nothing; the operand is not evaluated.  Reclassified \
     * from PRIMITIVE: the leaf translator's CONSTANT_FORMS and ERASED \
     * (V11: no model of a row's own) */ \
    INTRINSIC(LOCK_OBJECT, ARGS(object), \
              LITERAL(1, "a free-threaded build's critical section"), BUILD(gil)) \
    INTRINSIC(UNLOCK_OBJECT, ARGS(object), NONE, BUILD(gil)) \
    /* ceval_macros.h (a static inline function, as in a Py_DEBUG build \
     * before): _PyTuple_GET_OBJECT(tuple, i), a read of the code's tuple \
     * of names; no callee beyond it, no failure path, no callback */ \
    HELPER(GETITEM, PyObject *, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), SCALAR, EFFECTS(reads)) \
    /* Reviewed G0c2 (Include/cpython/dictobject.h): Py_TYPE(op) is dict or   \
     * frozendict; a read of ob_type; no failure, no callback.                */ \
    MACRO(PyAnyDict_CheckExact, \
        ARGS(scalar), TEST(exact(PyAnyDict_Type)), EFFECTS(reads), EXITS()) \
    /* Reviewed G0c2 (Include/setobject.h): Py_TYPE(op) is set or             \
     * frozenset; a read of ob_type; no failure, no callback.                 */ \
    MACRO(PyAnySet_CheckExact, \
        ARGS(scalar), TEST(exact(PyAnySet_Type)), EFFECTS(reads), EXITS()) \
    /* Objects/dictobject.c: new_dict from the empty keys, with a free list \
     * or PyObject_GC_New.  Callees: the allocator and the GC link.  A \
     * collection is not run here: allocation only schedules it through \
     * the eval breaker, which a later poll runs.  Failure: NULL with \
     * MemoryError set; success: a new reference.  No callback. */ \
    HELPER(PyDict_New, PyObject *, (), \
        ARGS(), VALUE_OR_NULL(owned exact(PyDict_Type)), EFFECTS(allocates, raises)) \
    /* Objects/dictobject.c: pop_check_dict (TypeError for a frozendict, \
     * SystemError for a non-dict: -1) then _PyDict_PopValue -> \
     * pop_lock_held: _PyValue_Hash of the key (the key's __hash__ runs \
     * Python), dict_pop_knownhash (key comparison may run __eq__; the \
     * removed value is closed, which can run a finalizer).  Returns 1 \
     * when removed, 0 when absent (no exception), -1 with an exception; \
     * *result (if not NULL) is first set to NULL and then owns the \
     * removed value on 1.  The key is borrowed */ \
    HELPER(PyDict_PopValue, int, (PyObject *, PyValueBorrow, PyValue *), \
        ARGS(object, borrowed, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* G0d1.  Objects/dictobject.c:6173: one step of a dict item iterator \
     * (asserted PyDictIterItem_Type) into two homes: dictiter_capture reads \
     * the dict and advances the iterator (writes), and raises RuntimeError \
     * for a dict that changed size or keys (-1); at the end it drops the \
     * iterator's dict (Py_XDECREF: releases, 0).  On 1 the key and value \
     * are stored in targets[0] and targets[1] (writes) and their old values \
     * closed (releases).  No call of Python besides the finalizers of the \
     * releases.  targets is the address of a home (IR_SLOT) */ \
    HELPER(_PyDictIterItem_Next, int, (PyObject *, PyValue *), \
        ARGS(object, scalar), SCALAR, EFFECTS(reads, writes, raises, releases)) \
    /* Objects/dictobject.c: the dict check (-1 with TypeError or \
     * SystemError), then _PyDict_SetValue_Take2 with a count taken of \
     * key and value (the row's arguments stay borrowed), \
     * setitem_take2_lock_held: the key's hash and equality may run \
     * Python, insertdict closes the replaced value (a finalizer), and \
     * the failure paths close both new counts.  Returns 0, or -1 with an \
     * exception */ \
    HELPER(PyDict_SetValue, int, (PyObject *, PyValueBorrow, PyValueBorrow), \
        ARGS(object, borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/dictobject.c PyDict_Update -> dict_merge_api(a, b, 1): a \
     * dict or mapping b merged into dict a; for a non-dict b it calls \
     * b.keys() and b[key], hashes and compares keys, and resizes a \
     * (arbitrary Python).  Failure: -1 with an exception set (AttributeError \
     * when b has no keys(), which _DICT_UPDATE rewrites); both arguments are \
     * borrowed on every path.  Result 0 or -1 */ \
    HELPER(PyDict_Update, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/setobject.c PySet_New -> make_new_set.  With a NULL iterable \
     * (BUILD_SET's only call) it allocates an empty set (MemoryError on \
     * failure) and runs nothing; with an iterable it iterates, hashes and \
     * compares: Python.  The contract is the function's, so runs_python.  \
     * The result is a new reference, NULL with an exception set; the \
     * argument is borrowed */ \
    HELPER(PySet_New, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Include/sliceobject.h): Py_IS_TYPE(op,                  \
     * &PySlice_Type), a read of ob_type. No failure, no callback.            */ \
    MACRO(PySlice_Check, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Reviewed G0c2 (Objects/dictobject.c). A str key with a cached hash     \
     * probes unicode-only keys without callbacks; any other key goes to      \
     * dict_get_value, which hashes (__hash__) and compares (__eq__):         \
     * arbitrary Python. Returns 1, 0 or -1 with the exception set; both      \
     * arguments are borrowed (the status value it fetches is closed          \
     * inside).                                                               */ \
    HELPER(_PyDict_ContainsValue, int, (PyDictObject *, PyValueBorrow), \
        ARGS(scalar, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/dictobject.c (_PyDict_SetValue) -> _PyDict_SetValue_Take2 \
     * (PyValue_DUP of key and value first): hashing and comparing keys, \
     * resizing, and releasing the replaced value, whose finalizer runs \
     * Python; an unhashable key raises TypeError.  Failure: -1 with an \
     * exception set.  Key and value are borrowed on every path (the DUPs \
     * are consumed by the Take2 callee).  The dict must be an exact dict or \
     * subclass (asserted) */ \
    HELPER(_PyDict_SetValue, int, (PyDictObject *, PyValueBorrow, PyValueBorrow), \
        ARGS(scalar, borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/dictobject.c _PyDict_SetValue_Take2 -> \
     * setitem_take2_lock_held: hash (unhashable: setitem_unhashable closes \
     * both owners and returns -1), insert_to_emptydict or insertdict, which \
     * compare keys, resize, and release a replaced value.  Both owners are \
     * consumed on every return, success and failure.  Failure: -1 with an \
     * exception set.  Callbacks: __hash__, __eq__, and the finalizer of a \
     * replaced value */ \
    HELPER(_PyDict_SetValue_Take2, int, (PyDictObject *, PyValue, PyValue), \
        ARGS(scalar, consumed, consumed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/listobject.c _PyList_AppendTakeValue: \
     * list_resize on a full list (MemoryError, -1), then the store and the \
     * size update.  The owner is consumed on both outcomes: on a failed \
     * resize PyValue_CLOSE(value) releases it, which can run a finalizer, \
     * so the exact set is writes, allocates, raises, releases; runs_python \
     * covers it.  Callbacks: only that finalizer */ \
    HELPER(_PyList_AppendTakeValue, int, (PyListObject *, PyValue), \
        ARGS(scalar, consumed), SCALAR, EFFECTS(runs_python, raises)) \
    /* pycore_list.h: _PyList_AppendTakeValue with the store in line while \
     * the list has room (G0a5 item 3) */ \
    HELPER(_PyList_AppendTakeValueInline, int, (PyListObject *, PyValue), \
        ARGS(scalar, consumed), SCALAR, EFFECTS(runs_python)) \
    /* Objects/listobject.c _PyList_Extend -> _list_extend: an exact \
     * list or tuple is copied, anything else is iterated (__iter__, \
     * __next__, __length_hint__: Python).  Returns a new reference to None \
     * (immortal) or NULL with an exception set (a TypeError when the \
     * iterable is not iterable, which _LIST_EXTEND rewrites).  Arguments \
     * are borrowed */ \
    HELPER(_PyList_Extend, PyObject *, (PyListObject *, PyObject *), \
        ARGS(scalar, object), SCALAR, EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Objects/listobject.c). PySlice_Unpack calls __index__   \
     * of the slice parts (arbitrary Python) and can raise TypeError or       \
     * OverflowError; list_slice_wrap then allocates a list and increfs the   \
     * items (no Python). Returns a new list or NULL with an exception set;   \
     * both arguments are borrowed.                                           */ \
    HELPER(_PyList_SliceSubscript, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Objects/setobject.c _PySet_AddValueTakeRef: hashes the \
     * key (unhashable: set_unhashable_type, PyValue_XCLOSE(key), -1), then \
     * set_add_entry_takeref, which compares keys (__eq__) and resizes.  The \
     * owner is consumed on every return.  Failure: -1 with an exception \
     * set */ \
    HELPER(_PySet_AddValueTakeRef, int, (PySetObject *, PyValue), \
        ARGS(scalar, consumed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Objects/setobject.c). Hashes the key (a str's cached    \
     * hash, else the type's __hash__) and probes with the key's __eq__:      \
     * arbitrary Python. Returns 1, 0 or -1 with the exception set; both      \
     * arguments are borrowed.                                                */ \
    HELPER(_PySet_Contains, int, (PySetObject *, PyValueBorrow), \
        ARGS(scalar, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/setobject.c _PySet_Update -> set_update_internal: a set or \
     * dict is merged directly, any other iterable is iterated; hashing, \
     * __eq__ and iteration run Python.  A non-set first argument is \
     * PyErr_BadInternalCall.  Returns 0 or -1 with an exception set; both \
     * arguments are borrowed */ \
    HELPER(_PySet_Update, int, (PyObject *, PyObject *), \
        ARGS(object, object), SCALAR, EFFECTS(runs_python, raises)) \
    /* Objects/sliceobject.c _PySlice_FromValues: value_bound maps each of \
     * the three arguments to an owned copy, PyValue_None for PyValue_NULL \
     * (BUILD_SLICE passes NULL for an absent step), so every argument may be NULL (a row's ARGS require nonnull only \
     * when they say so).  slice_from_owned pops the slice freelist or calls \
     * PyObject_GC_New (MemoryError on failure, after which it closes the \
     * three copies: each was just duplicated from a value the caller still \
     * holds, so the close drops no last count), stores the copies and \
     * GC-tracks.  Failure: NULL with an exception set; the arguments are \
     * borrowed throughout.  Callbacks: none (the allocation only schedules \
     * a collection through the eval breaker).  Result: a new reference */ \
    HELPER(_PySlice_FromValues, PyObject *, (PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(allocates, raises)) \
    /* pycore_tuple.h: PyTuple_GET_VALUE(tuple, index) and \
     * PyValue_AsKnownObjectBorrow of it: a load of the item; the caller \
     * guarantees an object (code metadata, asserted); no callee beyond \
     * those, no callback */ \
    HELPER(_PyTuple_GET_OBJECT, PyObject *, (PyObject *, Py_ssize_t), \
        ARGS(object, scalar), SCALAR, EFFECTS(reads))

/* Specialization: the site's counter and the specializer */
#define _PyIR_COMPONENTS_SPECIALIZATION(PRIMITIVE, HELPER, MACRO) \
    /* The header of the word's feedback entry: an address, which each \
     * generator expands for its form (ir_generator.IR_INSTRUCTION_MACROS) */ \
    /* Evidence (G0c8): the generator's expansion (ir_generator.IR_INSTRUCTION_MACROS): \
     * `((_PyFbHeader *)fb)`, the handler's local `fb`, loaded from the word's feedback \
     * entry before the handler body runs.  It computes an address and reads no heap. */ \
    MACRO(FB_COUNTER, ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* BINARY_OP_EXTEND's descriptor (pycore_code.h): its cache operand, \
     * typed, a pointer along the path binary_op_descr; the action it \
     * holds.  The actions (specialize.c, binaryop_extend_descrs) are \
     * concatenations, repeats and dict | dict, which return a new \
     * reference or NULL with an exception, and may run Python (a key's \
     * __eq__ in dict | dict) (G0a4) */ \
    /* Reviewed G0c2 (Include/internal/pycore_code.h): the cache operand's    \
     * pointer, typed. No memory read of its own; the FIELD rows below read   \
     * through it.                                                            */ \
    /* Evidence (G0d3), V11: no leaf uop calls it, so it has no leaf model and \
     * no lowering, and its uops run as their handler's function form: a cast of the cache operand */ \
    HELPER(_PyBinaryOp_ExtendDescr, _PyBinaryOpSpecializationDescr *, (PyObject *), \
              ARGS(scalar), PATH(binary_op_descr), EFFECTS()) \
    SLOT(binaryopactionfunc, PyObject *, (PyObject *, PyObject *), \
         ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python)) \
    FIELD(_PyBinaryOpSpecializationDescr, action, binaryopactionfunc, \
          ON(binary_op_descr), SLOT(binaryopactionfunc), EFFECTS(reads)) \
    /* The counter of an entry header (Python/ceval_macros.h): a test of \
     * it, and its advance; NULL has no counter */ \
    /* Evidence (G0c8): ceval_macros.h `_PyFb_CounterTriggers`: `fb != NULL && \
     * backoff_counter_triggers(fb->counter)`: one read of the entry header's counter \
     * (NULL header: never triggers).  No callee that runs code or raises.  It has no \
     * leaf model and no lowering: the specializing ops that call it are not leaf), \
     * so it is a HELPER row, not a PRIMITIVE (V11). */ \
    HELPER(_PyFb_CounterTriggers, int, (_PyFbHeader *), \
              ARGS(scalar), SCALAR, EFFECTS(reads), FEEDBACK(read(counter))) \
    /* Evidence (G0c8): ceval_macros.h `_PyFb_AdvanceCounter`: stores \
     * advance_backoff_counter(fb->counter) into the header (nothing for a NULL header; \
     * the free-threaded build also skips unreachable counters).  One write of the \
     * feedback entry's counter; no callee that runs code or raises.  Like \
     * _PyFb_CounterTriggers it has no leaf model or lowering: a HELPER row. */ \
    HELPER(_PyFb_AdvanceCounter, void, (_PyFbHeader *), \
              ARGS(scalar), NONE, EFFECTS(writes), FEEDBACK(write(counter))) \
    /* Evidence (G0c8): ceval_macros.h: a relaxed load of tstate->eval_breaker masked with \
     * _PY_EVAL_EVENTS_MASK.  No callee but the atomic load; it neither runs code nor \
     * raises, and it is the fast path of every periodic check.  It has a leaf model \
     * (ir_leaf.py) and a lowering (jit_lower.c), so it stays a PRIMITIVE. */ \
    PRIMITIVE(_Py_EvalBreakerHasEvents, bool, (PyThreadState *), \
              ARGS(tstate), CONTEXT("the thread's eval breaker"), EFFECTS(reads)) \
    /* ceval_macros.h: the thread's eval breaker word, a relaxed load (a plain \
     * load on x86-64, as jit_lower.c lowers it).  No callee; it neither runs \
     * code nor raises.  _RESUME_CHECK compares it with \
     * FRAME_CODE_INSTRUMENTATION_VERSION().  A leaf model and a lowering. */ \
    PRIMITIVE(_Py_EvalBreaker, uintptr_t, (PyThreadState *), \
              ARGS(tstate), SCALAR, EFFECTS(reads)) \
    /* _Py_HandlePending (Python/ceval_gil.c): stop-the-world, signal \
     * handlers and pending calls (Python code), the collector (finalizers); \
     * -1 with an exception set */ \
    /* Evidence (G0c8): ceval_gil.c:1365: the checks it makes behind the eval breaker's bits: \
     * stop-the-world (the thread suspends and re-attaches: other threads run), pending \
     * signals (handle_signals: Python signal handlers), pending calls (Python), the \
     * scheduled collector (_Py_RunGC: finalizers and weakref callbacks), the borrow \
     * release (_PyBorrowScope_Exit).  Each may run Python, which may \
     * rewrite the frame's homes and instrumentation; an error is -1 with an exception \
     * set.  `polls` records that the eval breaker is the trigger. */ \
    HELPER(_Py_HandlePending, int, (PyThreadState *), \
           ARGS(tstate), STATUS(NEGATIVE), \
           EFFECTS(reads, writes, allocates, raises, releases, runs_python, polls)) \
    /* Evidence (G0c8), for every _Py_Specialize_* row: Python/specialize.c.  A specializer \
     * reads the operands (borrowed; none is closed or moved), looks up the type and its \
     * dicts (_PyType_Lookup*: a dict lookup that can compare a non-str key of equal \
     * hash, which runs __eq__), may lower a callee's code (_PyCode_GetExec, which \
     * allocates; prepare_function_exec clears its failure with PyErr_Clear, which \
     * releases the exception object and so may run a finalizer), registers dict \
     * watchers (PyDict_Watch) and releases the references it took (Py_DECREF of a \
     * looked-up __init__ or descriptor).  Then it writes the word's handler and the \
     * site's feedback entry (set_counter, specialize_handler).  It raises nothing: a \
     * failure leaves the generic word and sets the counter backoff.  The effect is \
     * `runs_python` (a lookup callback or a finalizer can run code and rewrite frame \
     * homes); the feedback is read(entry) (the cache fields it checks), write(entry) \
     * and write(handler).  The operands are `borrowed`: PyValue operands are views \
     * (the guards that cached them are the specialized words'). \
     * BinaryOp also reaches specialize_subscr_getitem -> prepare_function_for_inline -> \
     * _PyCode_GetExec for BINARY_OP_SUBSCR_GETITEM; its `PyValue *` out argument is \
     * the frame's homes (LOCALS_ARRAY), read to find the operands' slots. */ \
    HELPER(_Py_Specialize_BinaryOp, void, \
           (PyValueBorrow, PyValueBorrow, _PyInstr *, int, PyValue *), \
           ARGS(borrowed, borrowed, scalar, scalar, scalar), NONE, \
           EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* Evidence (G0c8): as _Py_Specialize_BinaryOp. Reads the sequence's type and \
     * size (tuple, list); an inline operand is refused before any object access. */ \
    HELPER(_Py_Specialize_UnpackSequence, void, (PyValue, _PyInstr *, int), \
           ARGS(borrowed, scalar, scalar), NONE, EFFECTS(runs_python), \
           FEEDBACK(read(entry), write(entry), write(handler))) \
    /* Evidence (G0c8): the generator's expansion `_PyIR_FbHeaderOf(frame, instr, handler)` \
     * (ceval_macros.h:1163): `_PyIR_InstrFeedback(code, exec, instr)`, which indexes the \
     * code's feedback-index table and the executing copy's entry array (ir.c:705) and \
     * returns an address, or NULL for a site without an entry.  Both tables are \
     * interpreter structures that no Python code or specializer replaces for a live \
     * word; the call reads no object, writes nothing and cannot raise. */ \
    MACRO(FB_HEADER_OF, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): the generator's expansion `_PyIR_OriginalHandler(frame, this_instr, op)` \
     * (ir.c:795): `_PyCode_HandlerFor`, which reads the code object's words (never \
     * rewritten: pycore_ir.h:841) and the handler format tables.  No object \
     * read, no write, no raise. */ \
    MACRO(ORIGINAL_HANDLER, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): pycore_ir.h:581: `opcode in range && (_PyIR_Formats[opcode].effects & \
     * _PyIR_SPECIALIZES)`, a read of the static const format table.  That table is not \
     * heap state (nothing rewrites it), so the row reads nothing: `reads` is removed. */ \
    HELPER(_PyFb_HasEntry, int, (int), \
        ARGS(scalar), SCALAR, EFFECTS()) \
    /* Evidence (G0c8): as _Py_Specialize_BinaryOp. Reads the callee and its \
     * argument tuple and builds the binding plan (_PyBindPlan_Init); no lowering. */ \
    HELPER(_Py_Specialize_CallFunctionEx, void, (PyValue, PyValue, _PyInstr *), \
        ARGS(borrowed, borrowed, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* Evidence (G0c8): as _Py_Specialize_BinaryOp. specialize_py_call_kw / \
     * specialize_keyword_binding build a binding plan (_PyBindPlan_Init) and lower \
     * the callee. */ \
    HELPER(_Py_Specialize_CallKw, void, (PyValue, PyValue, PyValue, _PyInstr *, int), \
        ARGS(borrowed, borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* Evidence (G0c8): as _Py_Specialize_BinaryOp. Compares the global `super` \
     * with PySuper_Type by address and tests the class's type flag; no lookup and \
     * no lowering. */ \
    HELPER(_Py_Specialize_LoadSuperAttr, void, (PyValue, PyValue, _PyInstr *, int), \
        ARGS(borrowed, borrowed, scalar, scalar), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler))) \
    /* Evidence (G0c8): as _Py_Specialize_BinaryOp. The instance, slot and dict- \
     * hint paths read shapes and keys (specialize_dict_access), as for loads. */ \
    HELPER(_Py_Specialize_StoreAttr, void, (PyValueBorrow, _PyInstr *, PyObject *), \
        ARGS(borrowed, scalar, object), NONE, EFFECTS(runs_python), FEEDBACK(read(entry), write(entry), write(handler)))

/* The evaluator's labels (bytecodes.c label(...)): the routines behind them */
#define _PyIR_COMPONENTS_LABELS(PRIMITIVE, HELPER, MACRO) \
    /* error, exception_unwind, exit_unwind, exit_unwind_notrace and \
     * throw_error enter it through IR_PROCESS_ERROR (Python/ceval.c).  It \
     * records the traceback and runs the raise, handled and unwind \
     * monitoring callbacks (Python), clears the frame's scratch homes \
     * (releases), pops frames that do not handle the exception \
     * (switches_frame), and either returns the handler's word in the frame \
     * that handles it (jump) or leaves the evaluator (exit) */ \
    /* Evidence (G0c8): ceval.c:2253, reached only through IR_PROCESS_ERROR (PROCESS_ERROR_AT): \
     * for the ingress it records the traceback (PyTraceBack_Here allocates), runs \
     * monitor_throw, _PyEval_MonitorRaise, monitor_handled and monitor_unwind (callbacks: \
     * Python), looks up the exception table, clears the frame's scratch slots down to the \
     * handler's (error_clear_slots: releases, finalizers), writes the exception and lasti \
     * homes (_PyFrame_WriteHome), and pops frames that do not handle the exception \
     * (switches_frame; the frames' locals are released).  It returns the handler's \
     * word (jump) or leaves the evaluator with the exception (exit).  It never returns \
     * to the word that raised. */ \
    ROUTINE(_PyEval_ProcessError, _PyIRErrorResult, \
            (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, uint32_t), \
            ARGS(tstate, frame, scalar, scalar), SCALAR, \
            EFFECTS(runs_python, switches_frame, retires_frame), EXITS(jump, exit)) \
    /* start_frame (Python/ceval_macros.h): the recursion check, which \
     * counts down the thread's remaining depth; nonzero \
     * with RecursionError set */ \
    /* Evidence (G0c8): ceval_macros.h: decrements tstate->py_recursion_remaining (a write) \
     * and, only at the limit, calls _Py_CheckRecursiveCallPy (ceval.c:1038): it raises \
     * RecursionError with _PyErr_Format (allocates the exception and its message, and \
     * chains the exception context, which may release the old context), or \
     * Py_FatalError.  The exception types are static C types: no Python runs.  Nonzero \
     * means the error is set.  `releases` is added: the context chain of \
     * _PyErr_SetObject releases a replaced context. */ \
    HELPER(_Py_EnterRecursivePy, int, (PyThreadState *), \
           ARGS(tstate), STATUS(NONZERO), EFFECTS(writes, allocates, raises, releases))

/* Calls and frames */
#define _PyIR_COMPONENTS_CALLS(PRIMITIVE, HELPER, MACRO) \
    /* A call handler's state, its register local call_state (ceval.c): \
     * an implicit operand of the call's uops, which a uop reads and writes \
     * only through its state(...) clause.  CALL_SELF_PRESENT is set where \
     * the call has a self operand; the generator initializes the local \
     * from the word with IR_CALL_SELF_STATE (G0a4).  CALL_EXPANDED is set \
     * where the callable operand is a bound method whose function and \
     * receiver the push takes in its place (G3a) */ \
    STATE(call_state, int, BITS(CALL_SELF_PRESENT: self(present) | self(absent), \
                                CALL_EXPANDED: method(expanded) | method(whole)), \
          INIT(IR_CALL_SELF_STATE)) \
    /* Evidence (Python/ceval_macros.h, a static inline function since G0c \
     * batch 6; it was a macro).  One load of interp->eval_frame and a \
     * comparison with NULL.  No call, no failure, no callback.  The result \
     * decides whether a frame evaluator hook is set: a fact of the thread \
     * that no operand states. */ \
    HELPER(IS_PEP523_HOOKED, bool, (PyThreadState *), \
        ARGS(tstate), CONTEXT("a PEP 523 frame evaluator may replace the call"), EFFECTS(reads)) \
    /* Evidence (G0c batch 6): Include/methodobject.h: Py_IS_TYPE(op, \
     * &PyCFunction_Type), an exact-type test of a load of ob_type. No \
     * call, no failure, no callback. PyCFunction_Type is an immutable \
     * static type. */ \
    MACRO(PyCFunction_CheckExact, \
        ARGS(scalar), TEST(exact(PyCFunction_Type)), EFFECTS(reads), EXITS()) \
    /* Evidence (G0c batch 6): Include/cpython/methodobject.h: \
     * PyCFunction_GET_FLAGS(func) is an exported function that loads \
     * m_ml->ml_flags of the function object; the macro only casts. No \
     * call, no failure, no callback. It could be a HELPER row with a \
     * static check; kept as a MACRO for G0a5's guards, which call it by \
     * this name. */ \
    MACRO(PyCFunction_GET_FLAGS, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Reviewed G0c2 (Include/cpython/funcobject.h): a static inline read     \
     * of func_code. No failure, no callback.                                 */ \
    HELPER(PyFunction_GET_CODE, PyObject *, (PyObject *), \
        ARGS(object), SCALAR, EFFECTS(reads)) \
    /* Evidence (G0c batch 6): Objects/funcobject.c PyFunction_New -> \
     * PyFunction_NewWithQualName: reads globals['__name__'] (a str-keyed \
     * lookup, no user code), builds the function with counts of code, \
     * globals and builtins, tracks it for the GC. Returns NULL with an \
     * exception set on failure (no object allocated, the arguments \
     * untouched: both are borrowed). Callbacks: the allocation can start a \
     * collection, whose finalizers run Python, so `runs_python`. Result: \
     * an exact function (not declared: the row's result is the raw \
     * pointer). */ \
    HELPER(PyFunction_New, PyObject *, (PyObject *, PyObject *), \
        ARGS(object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (Python/ceval.c _PyEval_FrameClearAndPopGeneralWithResult; \
     * G0c batch 6).  The frame is already unlinked (tstate->current_frame \
     * is its caller) and the result is held in C, not NULL.  Callees: \
     * _PyThreadState_UpdateLastProfiledFrame (plain stores to tstate); for \
     * a thread frame retire_thread_frame -> _PyFrame_ClearExceptCode \
     * (Python/frame.c) and Py_CLEAR(f_executable), then \
     * _PyThreadState_PopFrame (a store to the data stack); for a generator \
     * frame frame_clear_and_pop -> clear_gen_frame (stores gi_frame_state, \
     * tstate->exc_info and generator_return_kind) -> _PyFrame_ClearGenerator \
     * -> _PyFrame_ClearSlots, Py_CLEAR(f_funcobj), _PyErr_ClearExcState. \
     * Failure: none.  Every path returns result itself; nothing sets an \
     * exception.  The one failing call, _PyFrame_GetFrameObject in \
     * take_ownership (the f_back link of a frame object that outlives its \
     * frame), clears its MemoryError and restores the saved exception. \
     * Callbacks: the locals, the function, the code and the frame object \
     * are released, so their finalizers and destructors may run Python (and \
     * write the caller's locals through FrameLocalsProxy): `releases`. \
     * take_ownership copies the frame into its frame object and may \
     * allocate the caller's frame object: `allocates`.  The result is the \
     * argument, unchanged, with its obligation: moved(2). */ \
    HELPER(_PyEval_FrameClearAndPopGeneralWithResult, PyValue, (PyThreadState *, _PyInterpreterFrame *, PyValue), \
        ARGS(tstate, frame, consumed), VALUE(moved(2)), \
        EFFECTS(reads, writes, allocates, releases, retires_frame)) \
    /* Evidence (Python/ceval.c, static inline; G0c batch 6).  The retire \
     * of a thread frame whose scratch is empty.  The same contract as \
     * _PyEval_FrameClearAndPopGeneralWithResult, which it calls when the \
     * frame has a frame object or a profiler anchors it.  Otherwise: \
     * _PyFrame_ClearSlots (closes the locals), Py_DECREF of f_funcobj and \
     * f_executable, _PyThreadState_PopFrame.  Failure: none, the result is \
     * returned unchanged.  Callbacks: the closes and decrefs release, so \
     * finalizers may run Python.  It allocates only through the general \
     * path.  Both retire the frame: its code may die (retires_frame). */ \
    HELPER(_PyEval_RetireThreadFrameWithResult, PyValue, (PyThreadState *, _PyInterpreterFrame *, PyValue), \
        ARGS(tstate, frame, consumed), VALUE(moved(2)), \
        EFFECTS(reads, writes, allocates, releases, retires_frame), \
        CALLS(_PyEval_FrameClearAndPopGeneralWithResult)) \
    /* pycore_interpframe.h: the frame's executable code object \
     * (f_executable) from the frame; a read, no failure */ \
    HELPER(_PyFrame_GetCode, PyCodeObject *, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS(reads)) \
    /* pycore_interpframe.h: f->f_funcobj, a read of the frame; asserts \
     * that it is a function */ \
    HELPER(_PyFrame_GetFunction, PyFunctionObject *, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS(reads)) \
    /* Evidence (G0c8): frame.c:106: for a pending kwargs slot, PyDict_New and a store of the \
     * new dict's word into frame->base[slot] (a home write), then pending_kwargs is cleared. \
     * PyDict_New allocates (a free-list pop or a GC allocation; the collector is only \
     * scheduled by _PyObject_GC_Link, never run here: _Py_ScheduleGC sets an eval-breaker \
     * bit) and returns NULL on MemoryError, which this returns as -1.  No callback, no \
     * release.  The row had runs_python: narrowed to writes, allocates, raises.  The \
     * slot's old value is NULL (asserted), so nothing is released. */ \
    HELPER(_PyFrame_MaterializeKwargs, int, (_PyInterpreterFrame *), \
        ARGS(frame), STATUS(NEGATIVE), EFFECTS(writes, allocates, raises)) \
    /* pycore_interpframe.h: _PyThreadState_PushFrameUnchecked bumps the \
     * thread's data stack top (writes thread state; the caller's \
     * _PyThreadState_HasStackSpace guard shows there is room, so there \
     * is no failure path), _PyFrame_Initialize writes the new frame's \
     * header and moves func into it (the argument is consumed on the one \
     * return).  No callback, no release.  The frame's homes are the \
     * caller's to fill */ \
    HELPER(_PyFrame_PushUnchecked, _PyInterpreterFrame *, (PyThreadState *, PyValue, _PyInterpreterFrame *), \
        ARGS(tstate, consumed, frame), SCALAR, EFFECTS(reads, writes, allocates)) \
    /* Evidence (G0c batch 6): Objects/funcobject.c: stores func_version \
     * and, without the free-threaded build, the function-version cache \
     * entry (slot->func, slot->code); the function must have no version \
     * yet. No call, no failure, no callback. Called only from \
     * MAKE_FUNCTION. */ \
    HELPER(_PyFunction_SetVersion, void, (PyFunctionObject *, uint32_t), \
        ARGS(scalar, scalar), NONE, EFFECTS(reads, writes)) \
    /* Evidence (G0c batch 6): Python/ceval_macros.h: one store of the \
     * attribute into the slot that _Py_FunctionAttributeOffsets[oparg] \
     * names (the slot is empty: assert); the function's slot takes the \
     * attribute's reference. The function stays with its owner. No call, \
     * no failure, no callback. Called only from SET_FUNCTION_ATTRIBUTE. */ \
    HELPER(_PyFunction_StoreAttribute, void, (PyFunctionObject *, int, PyObject *), \
        ARGS(borrowed, scalar, consumed object), NONE, EFFECTS(writes)) \
    /* Evidence (G0c batch 6): Include/internal/pycore_interpframe.h: \
     * compares size words with the room between datastack_top and \
     * datastack_limit. A read of two tstate fields; no call, no failure, \
     * no callback. */ \
    HELPER(_PyThreadState_HasStackSpace, bool, (PyThreadState *, int), \
        ARGS(tstate, scalar), CONTEXT("the data stack must hold the callee's frame"), EFFECTS(reads)) \
    /* Include/internal/pycore_interpframe.h, a static inline function. \
     * Callees: none.  It compares tstate->last_profiled_frame with frame \
     * and, on equality, stores previous there and increments the sequence \
     * number: two stores to the thread state, no call, no allocation, no \
     * release, no failure path and no callback (a profiler reads the \
     * fields from another process).  The frame pointers are compared and \
     * stored, never dereferenced.  Every call passes a frame, the frame's \
     * previous or the thread's current frame. */ \
    HELPER(_PyThreadState_UpdateLastProfiledFrame, void, \
           (PyThreadState *, _PyInterpreterFrame *, _PyInterpreterFrame *), \
           ARGS(tstate, scalar, scalar), NONE, EFFECTS(reads, writes)) \
    /* Evidence (G0c batch 6): Python/ceval_macros.h: \
     * `tstate->py_recursion_remaining++`. No call, no failure, no \
     * callback. */ \
    HELPER(_Py_LeaveRecursiveCallPy, void, (PyThreadState *), \
        ARGS(tstate), NONE, EFFECTS(reads, writes)) \
    /* Evidence (G0c batch 6): Include/internal/pycore_ceval.h: an empty \
     * function, `(void)tstate` (the C recursion count is derived from the \
     * stack pointer). It reads and writes nothing, and the seed's reads \
     * and writes were over-declared. */ \
    HELPER(_Py_LeaveRecursiveCallTstate, void, (PyThreadState *), \
        ARGS(tstate), NONE, EFFECTS()) \
    /* Evidence (G0c batch 6): Include/internal/pycore_ceval.h: compares \
     * the machine stack pointer with tstate's c_stack_soft_limit. No call, \
     * no failure (the error is raised later by _Py_CheckRecursiveCall, \
     * which this row does not call), no callback. */ \
    HELPER(_Py_ReachedRecursionLimit, int, (PyThreadState *), \
        ARGS(tstate), CONTEXT("the C recursion limit"), EFFECTS(reads)) \
    /* Evidence (G0c batch 6): Python/ceval.c: call_builtin_values: reads \
     * the operands in place from the homes (CALL_NATIVE_GATHER copies the \
     * words, no counts), calls the builtin's value entry or its object \
     * vectorcall through _PyValue_CallAdapter (which boxes numbers), or \
     * call_vector for more operands than one word holds. Every callee \
     * BORROWS the callable and the operands: the handler publishes the \
     * result and call_publish releases the temporaries (the comment above \
     * call_vector_impl), so the callable is `borrowed` here. Failure: NULL \
     * with an exception set, nothing taken. Callbacks: the builtin may run \
     * any Python: `runs_python`. The seed said `consumed`. */ \
    HELPER(call_builtin_values, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (G0c batch 6): Python/ceval.c: call_class_values: as \
     * call_builtin_values for a class with a vectorcall \
     * (tp_value_vectorcall, else tp_vectorcall through the adapter): the \
     * callable and the operands are borrowed, the result is a new \
     * reference or NULL with an exception set, and the class's constructor \
     * runs arbitrary Python (__init__ through type vectorcall): \
     * `runs_python`. The seed said `consumed`. */ \
    HELPER(call_class_values, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (G0c batch 6): Python/ceval.c: call_descriptor_values: as \
     * call_builtin_values for a method descriptor: the receiver is operand \
     * `first`, the descriptor's value entry or object vectorcall is called \
     * on the operands in place; callable and operands are borrowed; NULL \
     * with an exception set on failure; the method runs arbitrary Python: \
     * `runs_python`. The seed said `consumed`. */ \
    HELPER(call_descriptor_values, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (G0c batch 6): Python/ceval.c: call_push_exact: pushes the \
     * frame of the exactly-matching Python function \
     * (_PyFrame_PushUnchecked: a write of the data stack and tstate) and \
     * moves the operands into its first slots (call_fill_exact: a \
     * temporary operand moves out of its home, a local is counted; for a \
     * bound method (CALL_EXPANDED) its function and receiver are counted \
     * and its method operand released by call_release_callable, which is a \
     * release: the guard _CHECK_CALL_BOUND_METHOD_EXACT_ARGS makes the \
     * method have no weakrefs, but the row's contract is the function's). \
     * callable is the callable operand, the function or (CALL_EXPANDED) \
     * the bound method; self is the self operand, NULL for a bound method; \
     * a temporary of either moves out of its home. No failure path \
     * (the guards checked space and arity). Callbacks: none in the checked \
     * case; the release of the method operand is `releases`. */ \
    HELPER(call_push_exact, _PyInterpreterFrame *, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, consumed, consumed, scalar), SCALAR, EFFECTS(reads, writes, allocates, releases)) \
    /* Evidence (G0c batch 6): Python/ceval.c: call_vector -> \
     * call_vector_impl: gathers the operands (copies of the words, no \
     * counts; a PyMem_Malloc array beyond 16, freed before return) and \
     * calls _PyValue_VectorcallTstate on the callable, which borrows \
     * everything. NULL with an exception set on failure (MemoryError if \
     * the array cannot be allocated); no argument is taken on any path. \
     * The callee may run arbitrary Python: `runs_python`. The caller \
     * publishes the result and call_publish releases the temporaries. The \
     * seed said `consumed`. */ \
    HELPER(call_vector, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, PyValue), \
        ARGS(tstate, frame, scalar, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* G0d1.  Python/ceval.c:2106: the generator of a call whose callee is a \
     * generator function that takes exactly these arguments \
     * (CALL_PY_GENERATOR).  call_exact_function moves the callable out of \
     * its home (a local is counted), _PyGen_NewCall makes the generator \
     * (allocates; on failure it closes the function and raises), \
     * call_fill_exact moves the operands into the generator's frame, and \
     * gen_prologue runs the prologue (iter() of a generator expression's \
     * iterable: Python), which on failure discards the generator and the \
     * arguments it owns.  Returns the generator, or NULL with an exception \
     * set.  The callable is consumed */ \
    HELPER(call_new_generator, PyValue, (PyThreadState *, _PyInterpreterFrame *, const _PyIRWord *, _PyIRWord, PyValue, int), \
        ARGS(tstate, frame, scalar, scalar, consumed, scalar), VALUE_OR_NULL(owned), \
        EFFECTS(runs_python, raises))

/* Generators, coroutines and iteration */
#define _PyIR_COMPONENTS_GENERATORS(PRIMITIVE, HELPER, MACRO) \
    /* Include/cpython/genobject.h: `Py_IS_TYPE(op, &_PyAsyncGenASend_Type)`: \
     * one load of ob_type and a compare against a static type; no call, \
     * no failure.  The type is static, so exact(...) is a value fact. */ \
    MACRO(PyAsyncGenASend_CheckExact, \
        ARGS(scalar), TEST(exact(PyAsyncGenASend_Type)), EFFECTS(reads), EXITS()) \
    /* Evidence (G0c8): Include/cpython/genobject.h: Py_IS_TYPE(op, &PyCoro_Type), one read of \
     * ob_type.  It dereferences the object: callers pass a PyObject * they know is an \
     * object.  Left a MACRO row: the definition is a public header macro.  The result \
     * could be TEST(exact(PyCoro_Type)) like PyAsyncGenASend_CheckExact; the only caller \
     * (_INSTRUMENTED_END_SEND) uses the answer for a monitoring event, and no fact is \
     * read, so none is declared. */ \
    MACRO(PyCoro_CheckExact, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c8): Include/cpython/genobject.h: PyObject_TypeCheck(op, &PyGen_Type): \
     * Py_IS_TYPE, else PyType_IsSubtype, which reads tp_mro.  No callback, allocation or \
     * exception.  Left a MACRO row: the definition is a public header macro.  Callers \
     * use the answer for a monitoring event only, so the result stays SCALAR. */ \
    MACRO(PyGen_Check, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Python/ceval.c.  Borrows aiter.  Callees: the am_anext slot (any \
     * __anext__, Python), _PyCoro_GetAwaitableIter (__await__, Python), \
     * PyErr_Format and _PyErr_FormatFromCause, Py_DECREF of next_iter. \
     * Failure: NULL with an exception set; success: a new reference to \
     * the awaitable. */ \
    HELPER(_PyEval_GetANext, PyObject *, (PyObject *), \
        ARGS(object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises), HOMES(canonical)) \
    /* Python/ceval.c.  Borrows its operand.  Callees: PyErr_Format and \
     * _PyEval_FormatAwaitableError (reads the type, may run Python \
     * through _PyEval_SpecialMethodCanSuggest's lookups), \
     * _PyCoro_GetAwaitableIter (__await__, Python), Py_CLEAR of the \
     * coroutine it rejects.  Failure: NULL with an exception set (a \
     * number is no awaitable; a coroutine already being awaited); \
     * success: a new reference. */ \
    HELPER(_PyEval_GetAwaitable, PyObject *, (PyValueBorrow, int), \
        ARGS(borrowed, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises), HOMES(unknown)) \
    /* Reviewed G0c5 (ceval.c).  Called with an error set.  Callees: \
     * _PyErr_ExceptionMatches (a read; PyErr_GivenExceptionMatches walks \
     * the exception's MRO, which a metaclass __subclasscheck__ cannot \
     * affect for a class check, so no Python), then for a StopIteration \
     * _PyEval_MonitorRaise (sys.monitoring RAISE callbacks: arbitrary \
     * Python, which may write the frame's homes) and _PyErr_Clear (drops \
     * the exception object: a finalizer may run).  Returns 1 with the \
     * exception cleared, 0 with it still set.  The seed's `reads` left out \
     * both of the last two: it is runs_python.  It never leaves a new \
     * exception of its own (do_monitor_exc, called by \
     * _PyEval_MonitorRaise, handles the callbacks' errors; its result is \
     * not read here) */ \
    HELPER(_PyForIter_StopIsExhaustion, int, (PyThreadState *, _PyInterpreterFrame *), \
        ARGS(tstate, frame), SCALAR, EFFECTS(runs_python)) \
    /* Python/ceval.c: the out-of-line wrapper of \
     * _PyForIter_VirtualIteratorNextInline, whose body it returns as it \
     * is (reviewed with batch 5's row for the inline form, which carries \
     * the callee chain).  Borrows iter.  Results: {1, item} with an \
     * owned item and the cursor *index_ptr advanced (a tagged index, \
     * inline); {0, NULL} at exhaustion with no exception (a tagged \
     * cursor's iteritem returned NULL, or a StopIteration the step \
     * raised, reported to monitoring and cleared); {0, v} where a step \
     * returned PYGEN_RETURN, v the owned return value or NULL; {-1, NULL} \
     * with an exception set.  Callbacks: the iterator's slot (any \
     * __next__), _PyEval_MonitorRaise's tools, and the release of the \
     * cleared exception; no argument is stolen. */ \
    HELPER(_PyForIter_VirtualIteratorNext, _PyStatusValue, (PyThreadState *, _PyInterpreterFrame *, PyValueBorrow, PyValue *), \
        ARGS(tstate, frame, borrowed, scalar), \
        CASES(status==-1 value(null) raised, status==0 value(owned nullable), status==1 value(owned)), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c5 (ceval.c; _PyForIter_VirtualIteratorNext is its \
     * out-of-line wrapper).  FOR_ITER's step of the iterator `iter` \
     * (borrowed) with its cursor at *index_ptr.  A tagged cursor: \
     * Py_TYPE(iter)->_tp_iteritem (the SLOT _Py_iteritemfunc: reads, \
     * allocates and raises, no Python) is called with the untagged index; \
     * an item advances *index_ptr (the only store: through the pointer, \
     * to the caller's window cell, a tagged index; the value form returns \
     * it in the result) and gives {1, item}; at \
     * the end {0, NULL} with no exception; on failure {-1, NULL} with an \
     * exception set and the cursor unchanged.  Any other cursor: the \
     * iternext value slot (the SLOT _Py_iternextvaluefunc: arbitrary \
     * Python, a generator's frame or a heap type's __next__) or \
     * _PyValue_IterStep, which adapts a tp_iternext (arbitrary Python). \
     * An item: {1, item} owned.  A PYGEN_RETURN step is passed through as \
     * {0, v}: v is the owned return value of the iterator, or NULL.  \
     * PYGEN_ERROR: a StopIteration is the exhaustion, cleared by \
     * _PyForIter_StopIsExhaustion (monitoring callbacks, runs_python) \
     * and gives {0, NULL}; any other exception gives {-1, NULL}.  So the \
     * seed's `status==0 value(null)` was an under-declaration: status 0 \
     * can carry an owned value (the callers pass it to IR_COMPLETE_STEP, \
     * which consumes it).  Borrowed `iter` is never released; the index \
     * pointer is not an ownership argument (the cursor is an inline \
     * word).  Callbacks: yes (runs_python covers the rest) */ \
    /* G0d4: the cursor goes in by value (consumed: it owes no count) and \
     * comes out in the result's `cursor`, a tagged index or NULL as it \
     * went in, advanced to the item's index on {1, item}; the other \
     * outcomes return it as it was.  The out-of-line form keeps the \
     * pointer for the steps that are not FOR_ITER's own. */ \
    HELPER(_PyForIter_VirtualIteratorNextInline, _PyForIterStep, (PyThreadState *, _PyInterpreterFrame *, PyValueBorrow, PyValue), \
        ARGS(tstate, frame, borrowed, consumed), \
        CASES(status==-1 value(null) cursor(inline encoding(tagged_index|null)) raised, \
              status==0 value(owned nullable) cursor(inline encoding(tagged_index|null)), \
              status==1 value(owned) cursor(inline encoding(tagged_index|null))), EFFECTS(runs_python, raises)) \
    /* Include/internal/pycore_genobject.h, a static inline function: \
     * `frame - offsetof(PyGenObject, gi_iframe)`, address arithmetic with \
     * no dereference (the owner assertion is Py_DEBUG only).  No call, \
     * no failure. */ \
    HELPER(_PyGen_GetGeneratorFromFrame, PyGenObject *, (_PyInterpreterFrame *), \
        ARGS(frame), SCALAR, EFFECTS()) \
    /* Python/ceval_macros.h, a static inline function: PyValue_IsObject, \
     * PyCoro_CheckExact (a load of ob_type) and one relaxed load of the \
     * coroutine's cr_frame_state.  No call into other code, no write, no \
     * failure; the operand is borrowed.  True means the identity arm of \
     * _PyEval_GetAwaitable: an exact coroutine that nobody awaits. */ \
    HELPER(coro_awaitable_can_transfer, bool, (PyValueBorrow), \
        ARGS(borrowed), SCALAR, EFFECTS(reads)) \
    /* Python/ceval_macros.h, a static inline function.  GIL build: \
     * `gen->gi_frame_state = FRAME_EXECUTING` and 1; no failure and no \
     * call.  Free-threaded it can lose the claim to another thread, \
     * set ValueError and return 0 (outside the scoped product, where \
     * the caller's ERROR_NO_POP is then unreachable).  gen was accepted \
     * by gen_may_claim. */ \
    HELPER(gen_claim, int, (PyThreadState *, PyGenObject *), \
        ARGS(tstate, scalar), SCALAR, EFFECTS(reads, writes)) \
    /* Python/ceval_macros.h, a static inline function: a relaxed load of \
     * gi_frame_state and a comparison with FRAME_EXECUTING; no write, no \
     * call, no failure. */ \
    HELPER(gen_may_claim, bool, (PyGenObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads)) \
    /* Python/ceval_macros.h, a static inline function.  GIL build: if \
     * gi_frame_state < FRAME_EXECUTING it stores FRAME_EXECUTING and \
     * returns true, else false (executing or finished).  One load, at \
     * most one store; no call, no exception. */ \
    HELPER(gen_try_set_executing, bool, (PyGenObject *), \
        ARGS(scalar), SCALAR, EFFECTS(reads, writes)) \
    /* G0d1.  Objects/genobject.c:1258: the generator, coroutine or async \
     * generator of a call of function, whose frame is made but not run.  It \
     * reads the function and code, may compute a coroutine's origin, \
     * allocates the object (PyObject_GC_NewVar), and writes its fields and \
     * frame.  function and locals are consumed: the frame owns them, and on \
     * failure (NULL with an exception set) they are closed (releases).  No \
     * Python runs.  The result is a new reference */ \
    HELPER(_PyGen_NewCall, PyGenObject *, (PyThreadState *, PyValue, PyObject *, PyCodeObject *, _PyInterpreterFrame *), \
        ARGS(tstate, consumed, scalar, scalar, frame), VALUE_OR_NULL(owned object), \
        EFFECTS(reads, writes, allocates, raises, releases)) \
    /* G0d1.  Python/ceval.c:2093: the prologue of a new generator's code: \
     * nothing for a code without cells that is no generator expression; \
     * else gen_prologue_run (ceval.c:2037) makes the cells (allocates, \
     * writes the generator's frame) and, for a generator expression, runs \
     * iter() of its argument (Python).  Returns 0, or -1 with an exception \
     * set; the generator is borrowed on both, and the caller discards it on \
     * -1 (G0d1 moved that discard out, so that the owner stays the \
     * caller's) */ \
    HELPER(gen_prologue, int, (PyThreadState *, PyGenObject *, PyCodeObject *), \
        ARGS(tstate, object, scalar), STATUS(NEGATIVE), EFFECTS(runs_python, raises)) \
    /* G0d1.  Objects/genobject.c:1331: abandons a generator from \
     * _PyGen_NewCall that never ran: marks its frame cleared (writes), \
     * clears the frame (gen_clear_frame: closes the arguments and function \
     * it owns) and drops the reference (Py_DECREF): releases, and consumes \
     * the generator.  No failure */ \
    HELPER(_PyGen_DiscardCall, void, (PyGenObject *), \
        ARGS(consumed object), NONE, EFFECTS(writes, releases))

/* Errors */
#define _PyIR_COMPONENTS_ERRORS(PRIMITIVE, HELPER, MACRO) \
    /* Python/ceval.c: PyUnicode_AsUTF8 of obj (returns without a message \
     * if obj is NULL or not str), _PyErr_Format (runs_python), then for \
     * NameError PyErr_GetRaisedException, PyErr_GivenExceptionMatches, \
     * _PyValue_SetAttr of `name` on the exception (setattr can run \
     * Python) and PyErr_SetRaisedException.  No failure value: the \
     * exception is set or the call returned without one.  Arguments \
     * borrowed */ \
    HELPER(_PyEval_FormatExcCheckArg, void, (PyThreadState *, PyObject *, const char *, PyObject *), \
        ARGS(tstate, object, scalar, object), NONE, EFFECTS(runs_python)) \
    /* Python/ceval.c: returns without a message if an exception is set, \
     * else _PyEval_FormatExcCheckArg with the local's name: \
     * UnboundLocalError below the first free variable, NameError \
     * otherwise.  Callees: _PyErr_Occurred, _PyTuple_GET_OBJECT, \
     * _PyEval_FormatExcCheckArg (runs_python) */ \
    HELPER(_PyEval_FormatExcUnbound, void, (PyThreadState *, PyCodeObject *, int), \
        ARGS(tstate, scalar, scalar), NONE, EFFECTS(runs_python)) \
    /* Python/ceval.c _PyEval_FormatKwargsError: builds a \
     * TypeError (or re-raises the KeyError) by formatting func's name and \
     * the duplicated key: PyObject_Str / repr of Python objects, so \
     * runs_python.  It always leaves an exception set.  Arguments are \
     * borrowed (dupkey is released by the caller) */ \
    HELPER(_PyEval_FormatKwargsError, void, (PyThreadState *, PyValueBorrow, PyObject *, PyObject *), \
        ARGS(tstate, borrowed, object, object), NONE, EFFECTS(runs_python)) \
    /* Python/ceval.h: no_tools_for_local_event reads the code's monitoring \
     * data; otherwise do_monitor_exc takes the pending exception \
     * (PyErr_GetRaisedException), calls the tools' RERAISE callbacks \
     * through _Py_call_instrumentation_arg (arbitrary Python) and puts \
     * the exception back on success.  A callback's failure replaces the \
     * pending exception and is not reported: the caller goes on to \
     * exception_unwind either way.  Returns nothing; no argument is \
     * stolen. */ \
    HELPER(monitor_reraise, void, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *), \
        ARGS(tstate, frame, scalar), NONE, EFFECTS(runs_python))

/* Monitoring and the eval breaker */
#define _PyIR_COMPONENTS_MONITORING(PRIMITIVE, HELPER, MACRO) \
    /* Evidence (Python/ceval_macros.h; G0c batch 6).  Without WITH_DTRACE \
     * it is ((void)0).  With it: when the probe is enabled, \
     * dtrace_function_entry (Python/ceval.c) saves the raised exception, \
     * calls PyUnicode_AsUTF8 on co_filename and co_name (which caches the \
     * UTF-8 form in the string: a write, and an allocation), restores the \
     * exception, and fires the probe, which is native code outside Python. \
     * Failure: none (a NULL name becomes "<unknown>").  Callbacks: none. \
     * The exception state is unchanged on return. */ \
    MACRO(DTRACE_FUNCTION_ENTRY, \
        ARGS(), SCALAR, EFFECTS(reads, writes, allocates), EXITS()) \
    /* Evidence (Python/ceval_macros.h; G0c batch 6).  As \
     * DTRACE_FUNCTION_ENTRY, with dtrace_function_return: no-op without \
     * WITH_DTRACE, else the same saved exception, AsUTF8 caches and probe. \
     * Failure: none.  Callbacks: none. */ \
    MACRO(DTRACE_FUNCTION_RETURN, \
        ARGS(), SCALAR, EFFECTS(reads, writes, allocates), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h:514, the jump's event.  While tracing it only sets \
     * next_instr to the destination.  Otherwise _Py_call_instrumentation_jump \
     * runs the tools' callbacks and the line events of the edge (Python \
     * code: runs_python); status -1 is an error, which the macro takes to \
     * PROCESS_ERROR_AT(RAISE) itself, with the instruction pointer set to the \
     * instruction after instr_ptr: the word's raise exit.  Status 1 is a \
     * callback's redirect of the frame (frame_lineno_set_impl, \
     * Objects/frameobject.c: the debugger's jump retires the temporaries the \
     * new word does not keep, and every successful jump redirects, the same \
     * word through f_lineno_generation); the macro dispatches the new word at \
     * once, the word's redirect exit.  It owns no values */ \
    MACRO(INSTRUMENTED_JUMP, \
        ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(runs_python), EXITS(raise, redirect)) \
    /* Evidence (G0c8): ceval_macros.h:245: empty outside Py_DEBUG; in a debug build it \
     * calls maybe_lltrace_resume_frame and stores frame->lltrace (a frame field). */ \
    MACRO(LLTRACE_RESUME_FRAME, \
        ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): instrumentation.c:2779: locks the code (LOCK_CODE) and runs \
     * instrument_lock_held, which rewrites the code's instrumentation; it may \
     * allocate and raise (MemoryError).  It rewrites the \
     * words' handlers: the caller dispatches the word again.  `runs_python` covers the allocation, the raise and the release of \
     * replaced data. */ \
    HELPER(_Py_Instrument, int, (PyCodeObject *, PyInterpreterState *), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* Evidence (G0c8): instrumentation.c:1473: call_instrumentation_vector builds three \
     * words, then call_one_instrument calls each tool's callback (arbitrary Python, \
     * which may rewrite the frame's locals and the instrumentation), may stop the \
     * world to disable a tool, and returns -1 with an exception set when a callback \
     * fails.  A debugger may also move frame->instr_ptr (observer redirect), which the \
     * caller reads back.  Returns 0 or -1. */ \
    HELPER(_Py_call_instrumentation, int, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *), \
        ARGS(tstate, scalar, frame, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* Python/instrumentation.c: two counts taken (PyValue_XDUP) for the \
     * argument vector, call_instrumentation_vector calls each tool's \
     * callback (arbitrary Python) and may change the word's \
     * instrumentation, then the counts are closed.  Returns 0, or -1 \
     * with an exception set.  Arguments borrowed */ \
    HELPER(_Py_call_instrumentation_2args, int, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, frame, scalar, borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Evidence (G0c8): instrumentation.c:1483: as _Py_call_instrumentation with one more \
     * argument: PyValue_XDUP(arg) takes a count and PyValue_XCLOSE(args[3]) releases it \
     * after the callbacks; arg itself stays the caller's.  Callbacks run Python. */ \
    HELPER(_Py_call_instrumentation_arg, int, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow), \
        ARGS(tstate, scalar, frame, scalar, borrowed), SCALAR, EFFECTS(runs_python)) \
    /* Python/instrumentation.c: two counts taken (PyValue_XDUP) for the \
     * argument vector, then call_instrumentation_vector_protected: the \
     * exception being raised is taken from the thread, the tools' \
     * callbacks run (arbitrary Python), and it is restored unless a \
     * callback raised, which replaces it.  No failure value, an \
     * exception is set on return.  Arguments borrowed */ \
    HELPER(_Py_call_instrumentation_exc2, void, (PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, frame, scalar, borrowed, borrowed), NONE, EFFECTS(runs_python)) \
    /* Evidence (G0c8): instrumentation.c:2080: reads the instruction's tools and calls \
     * their callbacks (Python); returns the opcode to run next, or -1 with an exception \
     * set.  It may redirect frame->instr_ptr through the debugger. */ \
    HELPER(_Py_call_instrumentation_instruction, int, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *), \
        ARGS(tstate, frame, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* Evidence (G0c8): instrumentation.c:1827: the jump or branch event \
     * (call_instrumentation_vector: callbacks run Python), then the line events of the \
     * source edge (source_edge_events).  Returns a status and the word to run: {0, dest}; \
     * {1, frame->instr_ptr} when an observer redirected the frame (observer_redirected, \
     * which sees the generation of a jump to the same word); {-1, NULL} with an \
     * exception set.  The result holds no value, so it is SCALAR; the caller tests the \
     * status. */ \
    HELPER(_Py_call_instrumentation_jump, _PyIRObserverResult, (_PyInstr *, PyThreadState *, int, _PyInterpreterFrame *, _PyInstr *, _PyInstr *), \
        ARGS(scalar, tstate, scalar, frame, scalar, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* Evidence (G0c8): instrumentation.c:1920: finds the line event for this word and \
     * calls the tools' callbacks (Python); returns the original opcode of the word, or \
     * -1 with an exception set.  It may redirect frame->instr_ptr. */ \
    HELPER(_Py_call_instrumentation_line, int, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, _PyInstr *), \
        ARGS(tstate, frame, scalar, scalar), SCALAR, EFFECTS(runs_python, raises)) \
    /* Evidence (G0c8): ceval_macros.h: _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY and \
     * QSBR_QUIESCENT_STATE (both empty in the GIL build), then _Py_EvalBreakerHasEvents \
     * and, if set, _Py_HandlePending, whose effects it takes (the row had runs_python \
     * without `polls`).  Returns 0, or -1 with an exception set. */ \
    HELPER(check_periodics, int, (PyThreadState *), \
        ARGS(tstate), STATUS(NEGATIVE), EFFECTS(runs_python, polls, raises)) \
    /* Evidence (G0c8): ceval_macros.h: the same checks as check_periodics, with the pending \
     * work in handle_periodics_at_end, which skips it after a LOAD_SPECIAL and otherwise \
     * calls _Py_HandlePending.  Effects as check_periodics.  Returns 0, or -1 with an \
     * exception set. */ \
    HELPER(check_periodics_at_end, int, (PyThreadState *, _PyInterpreterFrame *), \
        ARGS(tstate, frame), STATUS(NEGATIVE), EFFECTS(runs_python, polls, raises)) \
    /* Evidence (G0c8): ceval.h:488: asks for local tools first (no tools: returns 0). \
     * Otherwise PyValue_AsObject boxes a number (allocates; NULL: -1), sets a \
     * StopIteration with the value (PyErr_SetObject), releases its count, runs the \
     * STOP_ITERATION callbacks (Python), and clears the exception.  Returns 0 or -1 \
     * with the exception from a callback or a failed box.  `value` stays the caller's. */ \
    HELPER(monitor_stop_iteration, int, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValueBorrow), \
        ARGS(tstate, frame, scalar, borrowed), SCALAR, EFFECTS(runs_python, raises))

/* The evaluator: dispatch, the frame's namespaces, atomics */
#define _PyIR_COMPONENTS_EVALUATOR(PRIMITIVE, HELPER, MACRO) \
    /* Reviewed G0c2 (pycore_code.h): 1 << (2 * (x >= y) + (x <= y)),         \
     * arithmetic on two scalars; no memory, no failure.                      */ \
    MACRO(COMPARISON_BIT, \
        ARGS(scalar, scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h: NEXTOPARG (loads the word at next_instr), then goto \
     * the handler.  In the function forms (ceval.c:2579) it returns {frame, next_instr}. \
     * It runs no code, reads the word (immutable for the dispatch) and ends the word: \
     * the `next` exit.  A redirect by a monitoring callback is DISPATCH_REDIRECT, and \
     * DISPATCH_SAME_OPARG is the `jump` to this word. */ \
    MACRO(DISPATCH, \
        ARGS(), NONE, EFFECTS(), EXITS(next)) \
    /* Evidence (G0d1): ceval_macros.h: `next_instr = frame->instr_ptr; DISPATCH();`, \
     * after a monitoring callback moved frame->instr_ptr (a debugger's f_lineno jump, \
     * frame_lineno_set_impl).  It runs no code and ends the word at the new word without \
     * the rest of its handler: the `redirect` exit. */ \
    MACRO(DISPATCH_REDIRECT, \
        ARGS(), NONE, EFFECTS(), EXITS(redirect)) \
    /* Evidence (G0c8): ceval_macros.h:197 `goto *opcode_targets[opcode]` (ceval.c:2581 in the \
     * function forms): an indirect jump on the local `opcode`, which the body set from \
     * ORIGINAL_HANDLER; it ends the word without reading the next word: `jump`. */ \
    MACRO(DISPATCH_GOTO, \
        ARGS(), NONE, EFFECTS(), EXITS(jump)) \
    /* Include/internal/pycore_pyatomic_ft_wrappers.h.  GIL build: \
     * `#define FT_ATOMIC_LOAD_INT8_RELAXED(value) value`, the identity of \
     * its operand expression, whose own `->` loads the body scan sees. \
     * Free-threaded: _Py_atomic_load_int8_relaxed(&value), outside the \
     * scoped product, like its UINT8 and PTR siblings. */ \
    INTRINSIC(FT_ATOMIC_LOAD_INT8_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* Include/internal/pycore_pyatomic_ft_wrappers.h: with the GIL a \
     * plain load of the value; the argument's own `->` access is the \
     * body's read (V2) */ \
    INTRINSIC(FT_ATOMIC_LOAD_PTR, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* as FT_ATOMIC_LOAD_PTR: the identity in the build with the GIL \
     * (pycore_pyatomic_ft_wrappers.h) */ \
    INTRINSIC(FT_ATOMIC_LOAD_PTR_ACQUIRE, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* as FT_ATOMIC_LOAD_PTR: the identity in the build with the GIL */ \
    INTRINSIC(FT_ATOMIC_LOAD_PTR_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* as FT_ATOMIC_LOAD_PTR: the identity in the build with the GIL \
     * (pycore_pyatomic_ft_wrappers.h) */ \
    INTRINSIC(FT_ATOMIC_LOAD_SSIZE_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* as FT_ATOMIC_LOAD_PTR: the identity in the build with the GIL \
     * (pycore_pyatomic_ft_wrappers.h) */ \
    INTRINSIC(FT_ATOMIC_LOAD_UINT32_RELAXED, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* Evidence (G0c batch 6): \
     * Include/internal/pycore_pyatomic_ft_wrappers.h: in the GIL build the \
     * plain load of its operand (the free-threaded build uses a relaxed \
     * atomic load). The operand is the instruction word of the executing \
     * code, read again after a call. A read; no call, no failure, no \
     * callback. */ \
    MACRO(FT_ATOMIC_LOAD_UINT64_RELAXED, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c8): pycore_pyatomic_ft_wrappers.h:154: the GIL build defines it as its \
     * argument, a plain load of a word of the code object (_co_instrumentation_version) \
     * or of the thread state.  The free-threaded definition (acquire load, line 36) is \
     * not in the scoped product. */ \
    INTRINSIC(FT_ATOMIC_LOAD_UINTPTR_ACQUIRE, ARGS(scalar), SCALAR, BUILD(gil)) \
    /* Include/internal/pycore_pyatomic_ft_wrappers.h.  GIL build: \
     * `#define FT_ATOMIC_STORE_INT8_RELEASE(value, new_value) value = \
     * new_value`, one store to the object's field and no load: it \
     * writes, it reads nothing, it calls nothing, and it cannot fail. \
     * An assignment to its first argument, so it cannot be a function or \
     * an INTRINSIC (not an identity).  Free-threaded: a release store. */ \
    MACRO(FT_ATOMIC_STORE_INT8_RELEASE, \
        ARGS(scalar, scalar), NONE, EFFECTS(writes), EXITS()) \
    /* Reviewed (G0c batch 1).  INSTR_AT_OFFSET(n) is a generator \
     * expansion (generators_common.py: `_PyFrame_GetBytecode(frame) + n`; \
     * ir_generator.py: `_PyFrame_InstrAt(frame, n)`): an address of an \
     * instruction word, computed from the frame's code; no load of the \
     * instruction, no failure.  _RERAISE passes it to frame->instr_ptr */ \
    MACRO(INSTR_AT_OFFSET, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Tools/cases_generator/generators_common.py expands it to \
     * `{0} - _PyFrame_GetBytecode(frame)`: pointer arithmetic against the \
     * frame's bytecode base.  _PyFrame_GetBytecode reads the frame's \
     * executing copy (frame state, the word's own frame, not the heap), \
     * calls nothing and cannot fail.  Its only caller in this batch is an \
     * assertion under Py_DEBUG. */ \
    MACRO(INSTR_OFFSET_OF, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Python/ceval_macros.h: `(next_instr += (x))`, an advance of the \
     * word cursor: no memory access, no call.  The generator reads it as \
     * the word's jump (TRANSFER_TOKENS); JUMPBY(0), the only form in this \
     * batch (_END_ASYNC_FOR), advances nothing and only names the source \
     * word for sys.monitoring, so that handler continues at next. */ \
    MACRO(JUMPBY, \
        ARGS(scalar), NONE, EFFECTS(), EXITS(jump(successor))) \
    /* Reviewed (G0c batch 1).  JUMPTO(T) is `next_instr = JUMP_TARGET(T)` \
     * (Python/ceval_macros.h): an assignment to the evaluator's successor \
     * local, which the dispatch reads at the end of the handler; the \
     * macro continues after it (a `+ POP_TOP` after the uop still runs). \
     * No memory, no failure.  The `jump` exit names the successor it \
     * sets; a C macro that assigns a local cannot be a function */ \
    MACRO(JUMPTO, \
        ARGS(scalar), NONE, EFFECTS(), EXITS(jump(successor))) \
    /* Evidence (G0c8): ceval_macros.h:368: IR_CHECK_BLOCK (an assert) and the address \
     * frame->ir_words + T.  No load of the target word; the `jump` is taken by JUMPTO. */ \
    MACRO(JUMP_TARGET, \
        ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (Python/ceval_macros.h; G0c batch 6). \
     * `next_instr = frame->instr_ptr + OFFSET`: a load of the frame's \
     * instr_ptr and a store to the evaluator's local.  No call, no failure, \
     * no callback. */ \
    MACRO(LOAD_IP, \
        ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* ceval_macros.h: frame->f_locals, NULL or a mapping object: a read \
     * of the frame; no callee, no failure, no callback */ \
    MACRO(LOCALS, \
        ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h:235: empty outside Py_DEBUG; in a debug build it \
     * asserts the frame invariants and calls lltrace_instruction, which only prints. */ \
    MACRO(PRE_DISPATCH_GOTO, \
        ARGS(), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): generators_common.py's expansion: reloads the handler's cached stack \
     * locals from the frame after a callback that may have run Python.  It reads frame \
     * fields only, owns nothing and cannot raise. */ \
    MACRO(RELOAD_STACK, \
        ARGS(), NONE, EFFECTS(), EXITS()) \
    /* Evidence (generators_common.py emit_save and ir_generator.save_stack; \
     * G0c batch 6).  A generator expansion: it writes the stack items to \
     * their homes in the frame, which the skeleton owns, and calls nothing. \
     * No failure, no callback. */ \
    MACRO(SAVE_STACK, \
        ARGS(), NONE, EFFECTS(), EXITS()) \
    /* Python/ceval.c (_PyEval_BinarySlice).  An exact list, tuple \
     * or str with inline int or None bounds is sliced directly \
     * (PyList_GetSlice, PyTuple_GetSlice, PyUnicode_Substring); object \
     * bounds go through _PyList/_PyTuple/_PyUnicode_BinarySlice (__index__ \
     * runs Python); anything else builds a slice (slice_of_bounds, \
     * _PySlice_FromValues) and calls _PyValue_GetItem (__getitem__: \
     * Python).  Failure: PyValue_NULL with an exception set.  The three \
     * arguments are borrowed on every path; the result is an owner */ \
    HELPER(_PyEval_BinarySlice, PyValue, (PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c). An object goes to                      \
     * _PyIntrinsics_UnaryFunctions[oparg].func as it is; a number is boxed   \
     * in a _PyValueTemps record (allocation, closed on return). The          \
     * intrinsic may be any of them (PRINT calls sys.displayhook, TYPEVAR     \
     * and friends build objects, ASYNC_GEN_WRAP): arbitrary Python. The      \
     * argument is borrowed on every path; the result is owned or NULL with   \
     * an exception set.                                                      */ \
    HELPER(_PyEval_CallIntrinsic1, PyValue, (PyThreadState *, int, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c). Boxes the operands that are numbers    \
     * in a _PyValueTemps record, calls                                       \
     * _PyIntrinsics_BinaryFunctions[oparg].func(tstate, value2, value1)      \
     * (PREP_RERAISE_STAR, typevar constructors, SET_FUNCTION_TYPE_PARAMS:    \
     * object construction and exception group splitting, which can call      \
     * Python), closes the temps. Both operands borrowed; the result owned,   \
     * or NULL with an exception set.                                         */ \
    HELPER(_PyEval_CallIntrinsic2, PyValue, (PyThreadState *, int, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (G0c batch 6): Python/ceval.c: boxes self if it is a number \
     * into a temporary record, calls cfunc(self, NULL) inside a borrow \
     * scope, closes the temporaries (the boxes: a release). Returns a new \
     * reference, or NULL with an exception set (asserted exclusive). self \
     * is borrowed. cfunc is a C method that may run any Python: \
     * `runs_python`. */ \
    HELPER(_PyEval_CallMethodDescriptorNoArgs, PyValue, (PyThreadState *, PyCFunction, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Evidence (G0c batch 6): Python/ceval.c: as \
     * _PyEval_CallMethodDescriptorNoArgs, with cfunc(self, arg) and both \
     * arguments borrowed and boxed if numbers. Returns a new reference or \
     * NULL with an exception set; `runs_python`. */ \
    HELPER(_PyEval_CallMethodDescriptorO, PyValue, (PyThreadState *, PyCFunction, PyValueBorrow, PyValueBorrow), \
        ARGS(tstate, scalar, borrowed, borrowed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c): as the except form, then rejects     \
     * ExceptionGroup subclasses with PyObject_IsSubclass(exc,            \
     * PyExc_BaseExceptionGroup) (Objects/abstract.c object_issubclass: its \
     * cls is an exact `type` instance, so recursive_issubclass walks the   \
     * MRO and calls no __subclasscheck__): no Python.  Returns 0 or -1 with \
     * the exception set; the argument is borrowed.  The seed's runs_python \
     * was over-declared and is narrowed                                    */ \
    HELPER(_PyEval_CheckExceptStarTypeValidValue, int, (PyThreadState *, PyValueBorrow), \
        ARGS(tstate, borrowed), STATUS(NEGATIVE, , object), EFFECTS(reads, allocates, raises)) \
    /* Reviewed G0c2 (Python/ceval.c): a number is not an exception class     \
     * (TypeError); otherwise _PyEval_CheckExceptTypeValid walks a type or    \
     * a tuple of types with PyExceptionClass_Check (reads) and sets          \
     * TypeError through _PyErr_SetString (allocates the message). Returns    \
     * 0 or -1 with the exception set; the argument is borrowed. It calls     \
     * no slot and no Python, so it is narrower than the seed's               \
     * runs_python: reads, allocates, raises.                                 */ \
    HELPER(_PyEval_CheckExceptTypeValidValue, int, (PyThreadState *, PyValueBorrow), \
        ARGS(tstate, borrowed), STATUS(NEGATIVE, , object), EFFECTS(reads, allocates, raises)) \
    /* Evidence (G3a; Python/ceval.c _PyEval_CreateInitFrame).  callable \
     * is the class operand, borrowed: the guards _CHECK_OBJECT and \
     * _CHECK_INIT_FUNCTION checked it and its cached __init__.  The helper \
     * reads _spec_cache.init and counts it (PyValue_DUP), then allocates \
     * the instance (PyType_GenericAlloc: allocates; a collection may run \
     * finalizers, so runs_python).  On success it pushes the init \
     * trampoline shim and the init frame (_PyFrame_PushTrampolineUnchecked, \
     * _PyFrame_PushUnchecked: writes of the data stack and tstate), moves \
     * or counts the operands from the caller's homes into the frame \
     * (call_take_operands; create_init_frame_window binds defaults and \
     * keywords, which may raise and run Python), releases the class \
     * operand from its home (call_release_callable: releases, may run \
     * Python) and returns the child.  On failure (MemoryError from the \
     * allocation or the window, a binding TypeError) it returns NULL with \
     * the exception set, having closed init, the instance and every operand \
     * it took; the homes that remain, the class among them, are the \
     * caller's unwinder's (the uop declares them closed). */ \
    HELPER(_PyEval_CreateInitFrame, _PyInterpreterFrame *, (PyThreadState *, _PyInterpreterFrame *, _PyInstr *, PyValue, _PyInstr *), \
        ARGS(tstate, frame, scalar, borrowed, scalar), SCALAR, \
        EFFECTS(reads, writes, allocates, raises, releases, runs_python)) \
    /* Python/ceval.c.  Consumes obj on every return (an inline obj owes \
     * nothing): the non-object path raises TypeError through \
     * _PyErr_Format and owes nothing; every other path closes obj \
     * (PyValue_CLOSE, a release).  Callees: _PyErr_Format, the type's \
     * am_aiter slot (any __aiter__, Python), PyValue_CLOSE, Py_DECREF of \
     * the iterator it rejects.  Failure: NULL with an exception set; \
     * success: a new reference, an object whose type has am_anext. */ \
    HELPER(_PyEval_GetAIter, PyValue, (PyThreadState *, PyValue), \
        ARGS(tstate, consumed), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c5 (ceval.c).  Consumes `iterable` on every path: the \
     * by-index and self-iterator cases return it as `first`; the \
     * coroutine-in-yield-from case raises TypeError and closes it; the \
     * general case calls _PyValue_GetIter (PyObject_GetIter: tp_iter, \
     * arbitrary Python through a heap type's __iter__, or the sequence \
     * protocol; TypeError when not iterable) and then PyValue_CLOSE of \
     * the iterable (a finalizer may run).  Results: {iterable, tagged \
     * index 0}, {iterable or iterator, NULL}, or {NULL, NULL} with an \
     * exception set (the iterable closed).  `first` is an object when \
     * not NULL: an inline number's type has neither _tp_iteritem nor \
     * PyObject_SelfIter, so it only reaches _PyValue_GetIter, which \
     * raises.  An owner of `iterable`'s obligation moves to `first`; \
     * `second` is a tagged index or NULL (the seed's `inline nullable` \
     * token is corrected by G0a5).  Callbacks: yes */ \
    HELPER(_PyEval_GetIter, _PyValuePair, (PyValue, int), \
        ARGS(consumed, scalar), \
        CASES(first(null) second(null) raised, first(owned object) second(inline encoding(tagged_index|null))), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: _PyValue_GetOptionalAttr (getattr: runs Python); \
     * on a miss _eval_import_from_miss reads __name__ and __spec__ (and \
     * its origin) by getattr, formats the message (allocates) and sets \
     * ImportError (runs Python).  Returns an owned value, or \
     * PyValue_NULL with an exception set.  The module is borrowed */ \
    HELPER(_PyEval_ImportFromValue, PyValue, (PyThreadState *, PyValueBorrow, PyObject *), \
        ARGS(tstate, borrowed, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: PyMapping_GetOptionalItem of \
     * builtins['__import__'] (runs Python for a mapping that is not \
     * exact), ImportError if it is missing, then \
     * _PyEval_ImportNameWithImport: the default __import__ runs \
     * PyImport_ImportModuleLevelObject (arbitrary module code), else the \
     * user's __import__ is called.  A new reference, or NULL with an \
     * exception.  All arguments borrowed; locals may be NULL (treated as \
     * None) */ \
    HELPER(_PyEval_ImportName, PyObject *, (PyThreadState *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, int), \
        ARGS(tstate, object, object, object, object, object, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Objects/lazyimportobject.c: a new reference, or NULL with an exception \
     * set.  The arguments are borrowed.  Callees: lazy_import_get_loaded_attr \
     * (sys.modules lookups, getattr on the loaded module, which runs \
     * Python; it clears an Exception from a failed lookup and returns NULL \
     * without one, which falls to the new lazy object) and _PyLazyImport_New \
     * (allocates; NULL on failure) */ \
    HELPER(_PyEval_LazyImportFrom, PyObject *, (PyThreadState *, _PyInterpreterFrame *, PyObject *, PyObject *), \
        ARGS(tstate, frame, object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: the lazy-imports policy \
     * (PyImport_GetLazyImportsMode, check_lazy_import_compatibility: \
     * reads __lazy_modules__ from globals, runs Python), then \
     * _PyEval_ImportName when not lazy, or __lazy_import__ from builtins \
     * (the default builds a lazy object, else it is called).  A new \
     * reference, or NULL with an exception.  All arguments borrowed */ \
    HELPER(_PyEval_LazyImportName, PyObject *, (PyThreadState *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, int, int), \
        ARGS(tstate, object, object, object, object, object, scalar, scalar), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: frame->f_locals lookup through \
     * value_load_from_dict (a mapping's __getitem__ runs Python; a lazy \
     * import in the result is reified, which imports), then globals \
     * (_PyDict_GetValue, with the same reification) then builtins, then \
     * NameError.  Returns an owned value, or PyValue_NULL with an \
     * exception set.  SystemError if f_locals is NULL.  Arguments \
     * borrowed */ \
    HELPER(_PyEval_LoadName, PyValue, (PyThreadState *, _PyInterpreterFrame *, PyObject *), \
        ARGS(tstate, frame, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c). A non-class type_value sets            \
     * TypeError; _PyValue_IsInstance (any __instancecheck__) decides the     \
     * match; then positional and keyword attributes are fetched with         \
     * _PyValue_GetOptionalAttr (any __getattr__ or descriptor) into a new    \
     * tuple, with a set for duplicates. Returns the attribute tuple          \
     * (owned), or NULL with no exception for a failed match, or NULL with    \
     * an exception (the caller tells them apart with _PyErr_Occurred).       \
     * subject, type and kwargs are borrowed on every path. Callbacks:        \
     * arbitrary Python.                                                      */ \
    HELPER(_PyEval_MatchClass, PyObject *, (PyThreadState *, PyValueBorrow, PyValueBorrow, Py_ssize_t, PyObject *), \
        ARGS(tstate, borrowed, borrowed, scalar, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Reviewed G0c2 (Python/ceval.c). Looks up map.get with                  \
     * _PyValue_GetMethod, makes a set and a dummy object, then for each      \
     * key a set test and add (key __hash__/__eq__) and a dict read or a      \
     * call of get (arbitrary Python). Returns a new tuple of values, or a    \
     * new reference to None when a key is absent, or NULL with an            \
     * exception set (a duplicate key sets ValueError). map and keys are      \
     * borrowed on every path.                                                */ \
    HELPER(_PyEval_MatchKeys, PyObject *, (PyThreadState *, PyObject *, PyObject *), \
        ARGS(tstate, object, object), VALUE_OR_NULL(owned), EFFECTS(runs_python, raises)) \
    /* Python/ceval.c: type_has_special_method twice: _PyType_LookupRef \
     * (the type cache; on a miss find_name_in_mro compares dict keys, \
     * which can run a user __eq__) then Py_TYPE(r)->tp_descr_get and \
     * Py_XDECREF(r) (a release).  Returns 0 or 1, no exception; \
     * Py_FatalError for another oparg */ \
    HELPER(_PyEval_SpecialMethodCanSuggest, int, (PyTypeObject *, int), \
        ARGS(scalar, scalar), SCALAR, EFFECTS(runs_python)) \
    /* Python/ceval.c: a deletion (v is PyValue_NULL) is _PyValue_DelItem \
     * on the namespace, with NameError on failure; an exact dict takes \
     * _PyDict_SetValue; any other mapping _PyValue_SetItem.  Returns 0, \
     * or non-zero with an exception (1 for no locals).  Runs Python \
     * (__setitem__, __delitem__, hash and equality of the key).  The \
     * value is borrowed and is not closed: STORE_NAME closes it */ \
    HELPER(_PyEval_StoreName, int, (PyThreadState *, PyValueBorrow, PyObject *, PyObject *), \
        ARGS(tstate, borrowed, object, object), SCALAR, EFFECTS(runs_python, raises)) \
    /* Python/ceval.c (_PyEval_StoreSlice).  An exact list with \
     * inline bounds goes to _PyList_StoreSlice (list_ass_slice: \
     * allocation, and releasing the replaced items, whose finalizers run \
     * Python; the right side may be iterated); otherwise a slice object is \
     * built and _PyValue_SetItem runs __setitem__.  Returns 0, or -1 with \
     * an exception set.  The four arguments are borrowed on every path */ \
    HELPER(_PyEval_StoreSlice, int, (PyValueBorrow, PyValueBorrow, PyValueBorrow, PyValueBorrow), \
        ARGS(borrowed, borrowed, borrowed, borrowed), SCALAR, EFFECTS(runs_python, raises)) \
    /* Evidence (G0c8): pyatomic_gcc.h:374: a relaxed atomic load of a word (of the thread \
     * state's eval_breaker or a code object's version): one read, no callee. */ \
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
    /* Evidence (G0c8): ceval_macros.h:221: `((_PyThreadStateImpl *)tstate)->eval_activation`, \
     * one load of the thread state. */ \
    MACRO(IR_ACTIVATION, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* Frame fields of Python/ceval_macros.h: GLOBALS() is frame->f_globals, \
     * BUILTINS() frame->f_builtins (PATH: fixed for the activation; \
     * pycore_interpframe.h's _PyFrame_Initialize sets them, and \
     * _PyFrame_CopyHeader copies them with the frame), \
     * FRAME_CO_CONSTS the code object's \
     * constants, IR_SLOTS() frame->base (an address, fixed for the \
     * activation), GETLOCAL(i) the home IR_SLOTS()[i].  A home is not the \
     * heap (design 1.3, 3.2): GETLOCAL addresses a home, which every use \
     * in bytecodes.c indexes by oparg, and loads and stores through it \
     * are home accesses, which a home write, a reentry and the skeleton's \
     * close kill (HOME) */ \
    MACRO(GLOBALS, ARGS(), PATH(globals), EFFECTS(reads), EXITS()) \
    /* ceval_macros.h: the instrumentation version of the frame's code, an \
     * acquire load (a plain load on x86-64, as jit_lower.c lowers it; x86-64 \
     * loads are acquire), which asserts that it has no event bits.  It \
     * reads the code, which instrumentation writes. */ \
    MACRO(FRAME_CODE_INSTRUMENTATION_VERSION, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    MACRO(BUILTINS, ARGS(), PATH(builtins), EFFECTS(reads), EXITS()) \
    /* A frame path's dict keys (pycore_dict.h), and their version: a \
     * fact of the path, `paths(globals: keys_version(v))` (G0a4) */ \
    FIELD(PyDictObject, ma_keys, PyDictKeysObject *, ON(globals | builtins | object), \
          PATH(dict_keys), EFFECTS(reads)) \
    /* Fields along a path from an input (G0a5 item 1): the path names \
     * each field it steps through, `paths(callable.im_func: ...)`, and \
     * its facts die as heap facts.  PATH(object) holds an object, \
     * PATH(pointer) a pointer whose only fact is its NULL test */ \
    FIELD(PyMethodObject, im_func, PyObject *, ON(object), PATH(object), EFFECTS(reads)) \
    FIELD(PyMethodObject, im_weakreflist, PyObject *, ON(object), PATH(pointer), \
          EFFECTS(reads)) \
    FIELD(PyFunctionObject, func_version, uint32_t, ON(object), MEASURE(func_version), \
          EFFECTS(reads)) \
    FIELD(PyTypeObject, tp_vectorcall, vectorcallfunc, ON(object), PATH(pointer), \
          EFFECTS(reads)) \
    FIELD(PyModuleObject, md_dict, PyObject *, ON(object), PATH(object), EFFECTS(reads)) \
    /* A builtin function's and a method descriptor's PyMethodDef, and its \
     * calling convention, which _PyMethodDef_Convention reads: a fact of \
     * the path, `paths(callable.d_method: convention(_Py_CALL_O))` (G0a5 \
     * item 2) */ \
    FIELD(PyCFunctionObject, m_ml, PyMethodDef *, ON(object), PATH(method_def), \
          EFFECTS(reads)) \
    FIELD(PyMethodDescrObject, d_method, PyMethodDef *, ON(object), PATH(method_def), \
          EFFECTS(reads)) \
    FIELD(PyMethodDef, convention, _PyCallConvention, ON(method_def), MEASURE(convention), \
          EFFECTS(reads)) \
    FIELD(PyDictKeysObject, dk_version, uint32_t, ON(dict_keys), \
          MEASURE(keys_version), EFFECTS(reads)) \
    /* Evidence (G0c8): ceval_macros.h:375: `_PyFrame_GetCode(frame)->co_consts`, a load \
     * of the code object's constants tuple.  The tuple of a live code object never \
     * changes, so the read is not a heap fact; `reads` is kept so that the records of \
     * the constant loads (batch 1) stay as they are, and G0d may remove it with them. */ \
    MACRO(FRAME_CO_CONSTS, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h:383 `(frame->base)`: the address of the \
     * homes, fixed for the activation; no home is read or written by taking it */ \
    MACRO(IR_SLOTS, ARGS(), HOME(base), EFFECTS(), EXITS()) \
    MACRO(GETLOCAL, ARGS(scalar), HOME, EFFECTS(), EXITS()) \
    /* Evidence (G0c batch 6): Python/ceval_macros.h: the C field of the \
     * instruction word held in a local (a shift and mask). No memory \
     * access, no call, no failure, no callback. */ \
    MACRO(IR_C, ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): pycore_ir.h:208: `(uint32_t)(W >> 52)`, bits of the word. */ \
    MACRO(IR_D, ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): pycore_ir.h:147: `(int)(W & IR_HANDLER_MASK)`, bits of the word. */ \
    MACRO(IR_HANDLER, ARGS(scalar), SCALAR, EFFECTS(), EXITS()) \
    /* Reviewed G0c2 (ceval_macros.h): the frame's home slot addressed by     \
     * an operand field; no failure, no callback.  G0d1: ceval_macros.h's     \
     * `IR_SLOT(F)` is `IR_SLOTS()[(F)]`, the expansion of GETLOCAL, so it is \
     * a HOME as GETLOCAL is: a frame home, not the heap (no reads)           */ \
    MACRO(IR_SLOT, ARGS(scalar), HOME, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): the generator's expansion (ir_generator.py:2871): the operand word k of \
     * a call, read in place, never owned or closed (a view).  A read of the executing \
     * copy's words. */ \
    MACRO(IR_OPERAND, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c batch 6): Python/ceval.c: IR_CALL_ARG(K) is \
     * IR_OPERAND(K + self-offset): an lvalue naming the home of operand K \
     * of the call, read from the frame's slots through the word's field. A \
     * home read, no call, no failure, no callback; `&IR_CALL_ARG(0)` \
     * passes the home itself, which the callee reads in place. */ \
    MACRO(IR_CALL_ARG, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c batch 6): Python/ceval.c: IR_CALL_FIRST(): 0 if the \
     * call's self is present, else the word's flag of a NULL-at-run-time \
     * self slot (IR_CALL_HAS_SELF(ir_word)), read from call_state and the \
     * word. No call, no failure, no callback. */ \
    MACRO(IR_CALL_FIRST, ARGS(), SCALAR, EFFECTS(reads), EXITS()) \
    /* Evidence (G0c8): ceval_macros.h:681: `F < IR_NFIXED()`, a load of the executing copy's \
     * nfixed (the word before frame->ir_words). */ \
    MACRO(IR_IS_LOCAL, ARGS(scalar), SCALAR, EFFECTS(reads), EXITS()) \
    /* IR_REPORT_EDGE (ceval_macros.h): ((void)(TAKEN)) in the interpreter. \
     * Reviewed (G0c batch 1): in the function form (ceval.c) it stores the \
     * flag in the activation record, `IR_ACTIVATION()->edge = (TAKEN)`, a \
     * local C struct that is neither heap nor a home; no failure, no \
     * callback.  Per-form macros cannot be one function */ \
    MACRO(IR_REPORT_EDGE, ARGS(scalar), NONE, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): ir_generator.py:3626: a marker that emits no code; it is accepted \
     * only when the word's metadata-only passthrough is certified (argument 1). */ \
    MACRO(IR_METADATA_PASSTHROUGH, ARGS(scalar), NONE, EFFECTS(), EXITS()) \
    /* Evidence (G0c8): ir_generator.py:4198: emits `call_state |= CALL_EFFECTS`, a write of the \
     * call handler's register local (the `call_state` STATE row, which the uop names in \
     * its state(...) clause).  No heap access. */ \
    MACRO(IR_BEGIN_CALL_EFFECTS, ARGS(), NONE, EFFECTS(), EXITS()) \
    /* Evidence (IR_SKIP_RESUME_IF, Python/ceval_macros.h; G0c batch 6): \
     * when the word at next_instr is a RESUME, loads the eval breaker, \
     * the code's instrumentation version (and, free-threaded, the tlbc \
     * index) and advances the evaluator's local next_instr past it.  It \
     * stores nothing to the heap and the frame: `reads` only (the seed \
     * also declared `writes`, for the local).  No call, no failure, no \
     * callback. */ \
    MACRO(IR_SKIP_RESUME, ARGS(), NONE, EFFECTS(reads), EXITS()) \
    /* IR_JIT_EVENT (ceval_macros.h): counts heat, may call _PyJit_Event and \
     * jit_run through IR_STEP_DONE, which runs the unit, which runs Python, \
     * and dispatches or leaves the evaluator; no table entry, so the larger \
     * declaration */ \
    /* Evidence (G0c8): ceval_macros.h:1132 (GIL, _PY_JIT_SUPPORTED): a heat counter of the \
     * executing copy (a write); at the threshold _PyJit_Event compiles the code \
     * (allocates, may fail without raising) and jit_run runs the unit through \
     * IR_STEP_DONE, which dispatches the unit's transfer or leaves the evaluator.  The \
     * unit runs Python and switches frames.  In the function forms (ceval.c:2660) and \
     * without the JIT it is empty. */ \
    /* Corrected (G0d1): the unit runs this frame's own words from next_instr, and \
     * a frame it enters or returns from is entered or left by one of those words \
     * (a CALL's enter, a RETURN_VALUE's return), whose records say so.  The event \
     * word pushes no frame and returns none: the frame goes on at the word where \
     * the unit stopped (redirect), raises, or the evaluator exits.  The unit's \
     * code is runs_python, as for IR_STORE_RETURN's IR_JIT_RESUME.  Declared as \
     * enter and return, the event was a frame-pushing word (_PyIR_PUSHES_FRAME), \
     * and the debugger's stopped-state analysis took every temporary to die at \
     * RESUME_JIT and JUMP_BACKWARD_JIT. */ \
    MACRO(IR_JIT_EVENT, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python), \
          EXITS(redirect, raise, exit)) \
    /* The execution pin (g3/design.md section 2; Python/jit.c).  The \
     * generator prints IR_PIN_DEPARTING() after the last guard of every \
     * arm with a return or a yield; no body writes it.  \
     * _PyJit_PinDeparting stores a new reference to the frame's code in \
     * the evaluation loop's activation (jit_pin), then releases the old \
     * pin: a code's deallocation runs its weakref callbacks, so \
     * `releases`.  It returns _PyIR_ESCAPED when that release \
     * deallocated, else 0; no failure. */ \
    HELPER(_PyJit_PinDeparting, int, (PyThreadState *, _PyInterpreterFrame *), \
        ARGS(tstate, frame), SCALAR, EFFECTS(releases)) \
    MACRO(IR_PIN_DEPARTING, ARGS(), NONE, EFFECTS(releases), EXITS(), \
          CALLS(_PyJit_PinDeparting)) \
    /* Python/jit.c _PyJit_Event: the switch routine's cold call for a \
     * frame whose code has no valid unit.  It reads the code's units, \
     * counts the warm-up event (a write) and compiles or recompiles \
     * (allocates; a failure is cleared, never raised).  It returns the \
     * unit to run or NULL, and never runs Python. */ \
    HELPER(_PyJit_Event, _PyJitUnit *, (PyThreadState *, _PyInterpreterFrame *), \
        ARGS(tstate, frame), SCALAR, EFFECTS(reads, writes, allocates)) \
    /* Guards: ir_records.GUARD_TOKENS maps each to EXIT_GUARD (miss); \
     * IR_HELPER_EFFECTS gives IR_GUARD_INLINE_INT, IR_GUARD_ELEMENT_TYPE, \
     * IR_GUARD_ELEMENT_INDEX and IR_DUP_CANONICAL_ELEMENT an empty set.  \
     * The element index is a variable that the macro declares.  \
     * IR_GUARD_INLINE_INT expands to !PyValue_IsInlineInt(x) \
     * (ir_leaf.capability_expansion), a test of the operand's tag bits \
     * (the PyValue_IsInlineInt row: EFFECTS()), so it reads no heap or \
     * frame state */ \
    /* Reviewed G0c2: expands to !PyValue_IsInlineInt(x)                      \
     * (ir_leaf.capability_expansion); a tag test with no memory read.        */ \
    MACRO(IR_GUARD_INLINE_INT, ARGS(borrowed), GUARD(inline_int), EFFECTS(), \
          EXITS(miss)) \
    /* MARKER_FACTS: exact(T) of the value, T the type its scalar argument \
     * 1 names (LIST or TUPLE: PyList_Type or PyTuple_Type), bound by G0a4 */ \
    /* Reviewed G0c2 (ir_generator.element_operation): a pointer-space test   \
     * of the slot word and a compare of ob_type with PyList_Type or          \
     * PyTuple_Type; a miss restarts the word. Reads ob_type; no callback.    */ \
    MACRO(IR_GUARD_ELEMENT_TYPE, ARGS(borrowed, scalar), GUARD(exact(1)), \
          EFFECTS(reads), EXITS(miss)) \
    /* MARKER_FACTS: the subscript (argument 1) is in_bounds of the \
     * container (argument 0) */ \
    /* Reviewed G0c2 (ir_generator.element_operation): for a list, an index   \
     * from the end is folded and compared with the length; for a tuple the   \
     * index is nonnegative and compared with the length; a miss restarts     \
     * the word. Reads the container's size. No allocation, no callback.      */ \
    MACRO(IR_GUARD_ELEMENT_INDEX, ARGS(borrowed, borrowed, scalar, scalar), \
          GUARD(, in_bounds(0)), EFFECTS(reads), EXITS(miss)) \
    /* GUARD_TOKENS: IR_GUARD_SHAPE_READ is an EXIT_GUARD; its owner is a \
     * PyObject * (PyValue_AsKnownObjectBorrow), the rest scalars.  \
     * MARKER_FACTS: the owner's type version and shape are scalar \
     * arguments 1 and 2 */ \
    MACRO(IR_GUARD_SHAPE_READ, ARGS(object, scalar, scalar), \
          GUARD(type_version(1) shape(2)), EFFECTS(reads), EXITS(miss)) \
    /* Duplicating reads (ir_generator.py: shape_read_value, \
     * IR_DUP_CANONICAL_ELEMENT in IR_HELPER_EFFECTS with an empty set): a \
     * new reference to an element or a slot's value, no Python */ \
    /* Reviewed G0c2 (ir_generator.element_operation,                         \
     * ir_leaf.capability_expansion): reads an item of a checked list or      \
     * tuple at a checked index and takes a new reference (an inline word     \
     * takes none). The index was proved in bounds by                         \
     * IR_GUARD_ELEMENT_INDEX; no failure, no callback.                       */ \
    MACRO(IR_DUP_CANONICAL_ELEMENT, ARGS(borrowed, scalar), VALUE(owned), \
          EFFECTS(reads), EXITS()) \
    /* ceval_macros.h: _PyObject_ShapeReadDup(owner object, offset): a load \
     * of the slot and a new count (PyValue_DUP, which needs a value that \
     * is not NULL).  The slot is filled by the invariant of the shapes: \
     * the guards of the word (type version and shape) show the shape of \
     * the owner's values, and a shape names only filled slots.  The \
     * generator asserts it after the call.  No callee runs Python */ \
    MACRO(IR_DUP_SHAPE_READ, ARGS(borrowed, scalar), \
          VALUE(owned assumed(nonnull) by shape), EFFECTS(reads), EXITS()) \
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
    /* Evidence (Python/ceval.c _Py_BuildString_ValueSteal, which \
     * ir_generator.home_helper expands to): _PyUnicode_JoinArray over the \
     * borrowed window, then clear_value_arguments on every path, success \
     * and failure, so the pieces are consumed and never come back (the \
     * seed said taken, which leaves them to the skeleton on failure). \
     * Failure: NULL with an exception set.  Callbacks: the join reads str \
     * pieces only; the close releases them, which may run a finalizer */ \
    MACRO(IR_BUILD_STRING, ARGS(consumed), VALUE_OR_NULL(owned), \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python), \
          EXITS()) \
    /* Evidence (Python/ceval.c _Py_BuildMap_ValueSteal): \
     * _PyDict_FromValueItems over the window, then clear_value_arguments on \
     * every path, success and failure: the values are consumed, not taken. \
     * Failure: NULL with an exception set (unhashable key, MemoryError, a \
     * key's __hash__/__eq__ may run Python) */ \
    MACRO(IR_BUILD_MAP, ARGS(consumed), VALUE_OR_NULL(owned), \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python), \
          EXITS()) \
    /* Evidence (Python/ceval.c _Py_BuildSet_ValueSteal): PySet_New, then \
     * each value taken from its home (_PyFrame_TakeHome) and stolen by \
     * _PySet_AddValueTakeRef, or closed once an add has failed or the set \
     * was not made: the values are consumed on every path.  Failure: NULL \
     * with an exception set (MemoryError, unhashable, or an element's \
     * __hash__/__eq__, which may run Python); the half-built set is \
     * released */ \
    MACRO(IR_BUILD_SET, ARGS(consumed), VALUE_OR_NULL(owned), \
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
    /* ir_generator.take_input_home expands it to _PyFrame_TakeHome(frame, \
     * &values[i]) (pycore_interpframe.h): read the home's word, store \
     * PyValue_NULL there, return the word.  The result owns the count the \
     * home held (an owner created from the input array); it writes only a \
     * home of the input array, which design 1.3 does not count, so the \
     * seeded writes is kept for TA's CSE.  No callee that can fail or run \
     * Python */ \
    MACRO(IR_TAKE_INPUT, ARGS(consumed, scalar), VALUE(owned), \
          EFFECTS(reads, writes), EXITS()) \
    /* Reviewed (G0c batch 1).  IR_CLOSE_INPUTS(array) is a generator \
     * replacer (ir_generator.py close_input_homes) for \
     * `_PyFrame_CloseHomeRange(frame, homes, oparg)` \
     * (pycore_interpframe.h): for each of the oparg homes, from the last, \
     * _PyFrame_TakeHome empties the home (writes) and PyValue_CLOSE \
     * releases the owner; the release may run a finalizer (releases, \
     * which already covers Python entered through a destructor).  The \
     * walk's `array` is the window's homes, consumed whole; no failure \
     * path.  It declared runs_python as well, which nothing in the body \
     * does beyond that release: removed */ \
    MACRO(IR_CLOSE_INPUTS, ARGS(consumed), NONE, \
          EFFECTS(writes, releases), EXITS()) \
    /* The observer's deferred close (ir_generator.py: defer_observer_input): \
     * the held value is closed after the observer; no table entry, so a \
     * release is declared */ \
    /* Evidence (G0c8): ir_generator.py:4087 defer_observer_input: queues the owner's close \
     * (PyValue_XCLOSE) after the last output store, which precedes the terminal observer, \
     * and sets the local to PyValue_NULL.  The release runs after the stores: a \
     * finalizer may run Python, and the owner is consumed. */ \
    MACRO(IR_DEFER_CLOSE, ARGS(consumed), NONE, EFFECTS(writes, releases), \
          EXITS()) \
    /* The monitoring event of a committed branch: IR_MARKER_EFFECTS \
     * escapes, error; TRANSFER_TOKENS (BRANCH, RAISE).  Arguments are the \
     * instruction, the target, the event and the edge: all scalars */ \
    /* Evidence (G0c8): ir_generator.py:4097 and ceval.c:2679 ir_event_edge: the terminal \
     * observer.  It first commits the uop's outputs (storage.push_outputs, \
     * emit_final_outputs), then ir_event_edge stores frame->return_pc and frame->instr_ptr, \
     * runs _PyIR_ObserveCommittedBranch (the branch event's callbacks: Python; a redirect \
     * moves the destination) and returns the word to run, or PROCESS_ERROR_AT(RAISE) \
     * for an error.  Exits: `branch` (the word runs on, at the destination) or `raise`. \
     * The outputs are committed before the event, so they are live on the branch exit \
     * (G0a5: committed exit); the EXITS tokens are G0a5's. */ \
    MACRO(IR_OBSERVE_COMMITTED_BRANCH, ARGS(scalar, scalar, scalar, scalar), \
          NONE, EFFECTS(reads, writes, raises, runs_python), \
          EXITS(branch(committed), raise(committed))) \
    /* IR_MARKER_EFFECTS: escapes, error, terminal; TRANSFER_TOKENS (BRANCH, \
     * RAISE).  Arguments are the kind, the completion value and the \
     * oparg.  The completion helper (_PyEval_CompleteForIter, \
     * _PyEval_CompleteSend) owns the value on every exit, so the value is \
     * consumed (G0a4).  Reviewed G0c5: complete_step_impl (ceval.c) \
     * reads the window homes and, on a FOR_ITER completion, retires them \
     * (window[0] and window[1] are cleared and closed: the iterator and \
     * its cursor), publishes nothing else, and returns the step's next \
     * word (the exit).  Callbacks: _PyIR_ObserveStep runs the PRE and POST \
     * sys.monitoring callbacks (arbitrary Python) and the XCLOSEs run \
     * finalizers; an error from a callback leaves the exception set (the \
     * raise exit).  The value is closed on every path (consumed).  The \
     * effects hold on the branch and raise exits; on the uop's `next` \
     * path the macro is not reached */ \
    MACRO(IR_COMPLETE_STEP, ARGS(scalar, consumed, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python), EXITS(branch, raise)) \
    /* Errors: IR_MARKER_EFFECTS escapes, error, terminal, frame; \
     * TRANSFER_TOKENS (RAISE) only.  PROCESS_ERROR_AT with an EXIT ingress \
     * leaves the evaluator and unwinding may pop frames, so exit is added \
     * to the table's raise */ \
    /* Evidence (G0c8): ir_generator.py:5464 process_error: emits PROCESS_ERROR_AT(ingress, \
     * checkpoint, origin), which records the ingress and checkpoint and jumps to \
     * LABEL_process_error: _PyEval_ProcessError, whose effects and exits (jump, exit) \
     * are that ROUTINE row's.  At the call site it is the uop's `raise` exit; the word \
     * never continues. */ \
    MACRO(IR_PROCESS_ERROR, ARGS(scalar, scalar), NONE, \
          EFFECTS(reads, writes, raises, releases, runs_python, \
                  switches_frame, retires_frame), EXITS(raise, exit), \
          CALLS(_PyEval_ProcessError)) \
    /* Evidence (G0c batch 6; ir_generator.finish_generator_return, \
     * Python/ceval_routines.h gen_return).  Each macro consumes the value \
     * on both exits.  IR_FINISH_GENERATOR_RETURN{,_EMPTY}: an unconditional \
     * transfer of the value to gen_return, which retires the dying \
     * generator frame and completes the resumer's step.  Its code ends the \
     * uop's path, as IR_GEN_RETURN is a goto (interpreter) or a tail call \
     * (function form), so the value is not the uop's afterwards.  Failure: \
     * the step's error is raised, with the value closed.  TRANSFER_TOKENS \
     * (RETURN, RAISE). */ \
    MACRO(IR_FINISH_GENERATOR_RETURN, ARGS(consumed, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame, retires_frame), \
          EXITS(return, raise)) \
    /* Evidence (G0c8): ir_generator.py:5549: emits IR_GEN_RETURN(value, dying, 1): a tail call \
     * of ir_gen_return (ceval.c:2632), which completes the generator's return (callbacks: \
     * Python; may raise) and returns the next frame and word.  A terminal transfer \
     * (return or raise) that switches the frame. */ \
    MACRO(IR_FINISH_GENERATOR_RETURN_EMPTY, ARGS(consumed, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame, retires_frame), \
          EXITS(return, raise)) \
    /* ir_generator.py enter_generator: stores the sent value's slot into \
     * the resumed frame (frame->return_slot, return_pc, previous, the \
     * generator's exc_info), makes the generator's frame current, then \
     * either enters the recursion count (_Py_EnterRecursivePy, which can \
     * fail with RecursionError: the sent value is closed, \
     * PROCESS_ERROR_AT) or, in the generator boundary form, jumps through \
     * IR_GEN_RESUME.  The new frame runs Python, so runs_python and \
     * switches_frame; the sent value moves into the resumed frame, which \
     * the walk cannot say, so the uop declares it taken.  Exits: enter (the \
     * transfer), raise (the failed entry). */ \
    MACRO(IR_ENTER_GENERATOR, ARGS(object, scalar), NONE, \
          EFFECTS(reads, writes, raises, runs_python, switches_frame), \
          EXITS(enter, raise)) \
    /* IR_MARKER_EFFECTS escapes, terminal, frame (no error); \
     * TRANSFER_TOKENS (YIELD) */ \
    /* Evidence (G0c8): ir_generator.py:3634: suspends the frame for a delegated yield: stores \
     * instr_ptr, detaches the generator's exception state (writes), leaves the recursion \
     * count, switches to the previous frame, and stores the value to the caller's slot \
     * (IR_STORE_RETURN, which may enter the unit: IR_JIT_RESUME runs Python).  It \
     * consumes the yielded value into the caller's home and exits by `yield`. */ \
    MACRO(IR_SUSPEND_DELEGATED, ARGS(consumed), NONE, \
          EFFECTS(reads, writes, runs_python, switches_frame), EXITS(yield)) \
    /* ir_generator.try_dict_first_probe: _PyDict_TryGetFirstProbeValue \
     * (Objects/dictobject.c) reads an exact dict's first probe slot for an \
     * inline int key and returns a new reference to the value only on an \
     * identity hit; every other case returns NULL with the inputs, the \
     * table and the exception state untouched, and no hash or __eq__ runs. \
     * On a hit the macro detaches both inputs, stores the result in the \
     * destination, closes the inputs and the displaced destination \
     * (releases: a last reference may run a finalizer) and dispatches the \
     * next word.  It never raises: the tables' RAISE was wrong, and the \
     * raise sites of the uop lie after it, where the uop's own inputs are \
     * released.  On a miss it falls through with everything unchanged. \
     * The hit's inputs are consumed by the macro, which the walk does not \
     * model; the fall-through path is the one it follows */ \
    MACRO(IR_TRY_DICT_FIRST_PROBE, ARGS(), NONE, \
          EFFECTS(reads, writes, releases), \
          EXITS(next)) \
    /* Call preparation: IR_MARKER_EFFECTS escapes, error; TRANSFER_TOKENS \
     * (RAISE) */ \
    /* Evidence (G0c batch 6): ir_generator.prepare_call emits nothing: the \
     * preparations are steps of call_run_* in this order when the call is \
     * invoked. The method expansion (call_expand_method) takes views and \
     * no counts; the CALL event (call_monitor) runs Python. The effects \
     * are those of the steps, so that a uop that declares the marker \
     * declares them; the marker's own place has no code. */ \
    MACRO(IR_PREPARE_CALL_METHOD, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, runs_python), \
          EXITS(raise)) \
    /* Evidence (G0c batch 6): As IR_PREPARE_CALL_METHOD: a step of \
     * call_run_ex for CALL_FUNCTION_EX: the positional iterable is made a \
     * tuple that the call owns (_PyValue_SequenceTuple: allocates, runs \
     * __iter__ and __next__), and the kwargs a dict. The marker's own \
     * place has no code; the effects are the steps'. */ \
    MACRO(IR_PREPARE_CALL_TUPLE, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, runs_python), \
          EXITS(raise)) \
    /* Evidence (G0c8): ir_generator.py:4189 (prepare_call): the marker emits no code.  The CALL \
     * event is a step of call_run_*(), which runs it in order on the call's operands when \
     * the call is invoked: Python callbacks, an error (raise) and a rewrite of the \
     * operands.  The row carries those effects at the marker's place. */ \
    MACRO(IR_MONITOR_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, raises, runs_python), EXITS(raise)) \
    /* Evidence (G0c8): ir_generator.py:4204: `if (call_refresh(tstate, frame, this_instr) < 0)` \
     * error, then reloads the call's words and feedback (call_reload).  call_refresh \
     * (ceval.c:2091) revalidates the operands after the specializer ran \
     * (value_operation_refresh) and fails with an error if the frame changed. */ \
    MACRO(IR_REFRESH_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, raises, runs_python), EXITS(raise)) \
    /* Call invocation: IR_MARKER_EFFECTS escapes, error, terminal, frame; \
     * TRANSFER_TOKENS (ENTER_FRAME, RAISE).  The call runs the callee or \
     * enters its frame, and releases the operands */ \
    /* Evidence (G0c batch 6): ir_generator.invoke_call emits one out-of- \
     * line call_run_* (Python/ceval.c call_run, call_run_ex) on the call's \
     * operands in their homes: it expands a bound method (views, no \
     * counts), runs the CALL event if the word is instrumented \
     * (call_monitor: Python), then either binds and pushes a Python callee \
     * (the arguments move or are counted from the homes, the operands are \
     * released once the child is current: releases) and returns {0, \
     * child}, or calls the native callee with every operand borrowed \
     * (call_vector_impl: runs_python), publishes the result and releases \
     * the temporaries (call_publish; a finalizer may run) and returns {0, \
     * NULL}, or returns {-1, NULL} with an exception set and the unmoved \
     * operands left in their homes for the unwinder. The macro then \
     * dispatches the child (enter) or the next word (next: a native or \
     * generator call completed in place, with CALL_PERIODIC's check done \
     * inside call_run), or raises. The seed listed enter and raise, the \
     * transfer table's entries, and omitted the in-place completion. */ \
    MACRO(IR_INVOKE_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python, \
                  switches_frame), EXITS(next, enter, raise)) \
    /* Evidence (G0c batch 6): As IR_INVOKE_CALL, preceded by \
     * call_bind_positional with the cached binding plan (the guards \
     * checked PEP 523, the function version and the recursion budget); a \
     * plan that misses has no effect and the generic call_run runs. \
     * Effects and exits are IR_INVOKE_CALL's. */ \
    MACRO(IR_INVOKE_GUARDED_PYTHON_CALL, ARGS(), NONE, \
          EFFECTS(reads, writes, allocates, raises, releases, runs_python, \
                  switches_frame), EXITS(next, enter, raise))

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
