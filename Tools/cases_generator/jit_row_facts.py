"""What the JIT optimizer knows of the C primitives and helpers that a
template's leaf IR calls (jit_backend's passes), beyond what rop_primitives
declares.

An entry is keyed by a primitive or helper of rop_primitives.  The effects
of a symbol are its declared ones there; an entry adds what no other place
states: whether it takes or returns a count, what a test of it shows about
NULL, whether its result is never NULL, whether its failure comes only from
a slow path, the test that conjoins it and the form it takes for a word
known not NULL.  A symbol without an entry is unknown to the optimizer: its
calls may change any memory.  Folding these facts into the extern clauses
of the .rop files is for the JIT phase (R/d/gaps.md)."""

from dataclasses import dataclass


@dataclass(frozen=True)
class RowFacts:
    # The call also polls, which rop_primitives' effects cannot say
    polls: bool = False
    # It consumes or takes a count of an argument, or returns a new owner
    counts: bool = False
    # It loads a frame home, other than the homes' base
    home_load: bool = False
    # A test of one word: (the word is not NULL where it holds, the test is
    # the one for NULL); None for anything else
    test: tuple[bool, bool] | None = None
    # The value result is declared never NULL
    never_null: bool = False
    # The result is NULL only when its slow path's helper fails
    slow: bool = False
    # The two-word test that this test is half of
    conjoins: str = ""
    # The primitive to use in its place for a word that is not NULL
    nonnull_form: str = ""


ROW_FACTS: dict[str, RowFacts] = {
    'BUILTINS': RowFacts(),
    'COMPARISON_BIT': RowFacts(),
    'DK_UNICODE_ENTRIES': RowFacts(),
    'FRAME_CODE_INSTRUMENTATION_VERSION': RowFacts(),
    'FRAME_CO_CONSTS': RowFacts(),
    'GETLOCAL': RowFacts(home_load=True),
    'GLOBALS': RowFacts(),
    'IR_SLOTS': RowFacts(),
    'PyDict_CheckExact': RowFacts(test=(True, False)),
    'PyFunction_Check': RowFacts(test=(True, False)),
    'PyList_CheckExact': RowFacts(test=(True, False)),
    'PyList_GET_SIZE': RowFacts(),
    'PyList_GET_VALUE': RowFacts(),
    'PyModule_CheckExact': RowFacts(test=(True, False)),
    'PyTuple_CheckExact': RowFacts(test=(True, False)),
    'PyTuple_GET_SIZE': RowFacts(),
    'PyTuple_GET_VALUE': RowFacts(),
    'PyUnicode_CheckExact': RowFacts(test=(True, False)),
    'PyValue_AsKnownObjectBorrow': RowFacts(),
    'PyValue_BoolCheck': RowFacts(test=(True, False)),
    'PyValue_BothInlineInt': RowFacts(conjoins='PyValue_IsInlineInt'),
    'PyValue_CLOSE': RowFacts(counts=True),
    'PyValue_CLOSE_SPECIALIZED': RowFacts(counts=True),
    'PyValue_DUP': RowFacts(counts=True, never_null=True, nonnull_form='PyValue_XDUP'),
    'PyValue_FloatOperandValues': RowFacts(),
    'PyValue_FromDouble': RowFacts(counts=True, slow=True),
    'PyValue_FromDoubleNotNaN': RowFacts(never_null=True),
    'PyValue_FromInlineInt': RowFacts(never_null=True),
    'PyValue_FromInt64': RowFacts(counts=True, slow=True),
    'PyValue_IncrementTaggedIndexNoOverflow': RowFacts(counts=True, never_null=True),
    'PyValue_InlineFloat': RowFacts(),
    'PyValue_InlineInt64': RowFacts(),
    'PyValue_Is': RowFacts(),
    'PyValue_IsFalse': RowFacts(test=(True, False)),
    'PyValue_IsInline': RowFacts(test=(True, False)),
    'PyValue_IsInlineFloat': RowFacts(test=(True, False)),
    'PyValue_IsInlineInt': RowFacts(test=(True, False)),
    'PyValue_IsNone': RowFacts(test=(True, False)),
    'PyValue_IsNull': RowFacts(test=(False, True)),
    'PyValue_IsObject': RowFacts(test=(True, False)),
    'PyValue_IsPointerSpace': RowFacts(test=(False, False)),
    'PyValue_IsTaggedIndex': RowFacts(test=(True, False)),
    'PyValue_IsTrue': RowFacts(test=(True, False)),
    'PyValue_LongCheckExact': RowFacts(test=(True, False)),
    'PyValue_TagIndex': RowFacts(never_null=True),
    'PyValue_UntagIndex': RowFacts(),
    'PyValue_XCLOSE': RowFacts(counts=True),
    'PyValue_XDUP': RowFacts(counts=True),
    'Py_TYPE': RowFacts(),
    '_PyDictValues_AddToInsertionOrder': RowFacts(),
    '_PyDictValues_SetShape': RowFacts(),
    '_PyFloat_ExactDealloc': RowFacts(),
    '_PyList_ITEMS': RowFacts(),
    '_PyLong_ExactDealloc': RowFacts(),
    '_PyLong_IsZero': RowFacts(test=(False, False)),
    '_PyObject_InlineValues': RowFacts(),
    '_PyObject_Shape': RowFacts(),
    '_PyObject_ShapeReadDup': RowFacts(counts=True, never_null=True),
    '_PyObject_ValueSlot': RowFacts(),
    '_PyTuple_ITEMS': RowFacts(),
    '_PyValueGuarded_Load': RowFacts(never_null=True),
    '_PyValueSlot_Exchange': RowFacts(counts=True),
    '_PyValueSlot_ExchangeFilled': RowFacts(counts=True, never_null=True),
    '_PyValueSlot_Fill': RowFacts(counts=True),
    '_PyValue_AsKnownObjectBorrowPointer': RowFacts(),
    '_PyValue_BoundLocal': RowFacts(never_null=True),
    '_PyValue_BoxDouble': RowFacts(counts=True),
    '_PyValue_BoxInt64': RowFacts(counts=True),
    '_PyValue_CellLocal': RowFacts(never_null=True),
    '_PyValue_CodeConstant': RowFacts(never_null=True),
    '_PyValue_CommonConstant': RowFacts(never_null=True),
    '_PyValue_FromIntScaled': RowFacts(never_null=True),
    '_PyValue_IntScaled': RowFacts(),
    '_PyValue_LshiftInt64': RowFacts(counts=True),
    '_PyValue_MultiplyInt64': RowFacts(counts=True),
    '_PyValue_NamesObject': RowFacts(),
    '_PyValue_PublishedItem': RowFacts(never_null=True),
    '_Py_EvalBreaker': RowFacts(),
    '_Py_EvalBreakerHasEvents': RowFacts(),
    '_Py_HandlePending': RowFacts(polls=True),
}
