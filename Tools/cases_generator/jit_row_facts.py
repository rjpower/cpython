"""What the JIT optimizer knows of the C primitives and helpers that a
template's leaf IR calls (jit_backend's passes).

Each entry is the contract of one symbol, reduced to the facts a pass reads:
its effects, whether it takes or returns a count, what a test of it shows
about NULL, whether its result is never NULL, whether its failure comes only
from a slow path, the test that conjoins it and the form it takes for a word
known not NULL.  A symbol without an entry is unknown to the optimizer: its
calls may change any memory."""

from dataclasses import dataclass


@dataclass(frozen=True)
class RowFacts:
    # The effects that the call declares: reads, writes, allocates, raises,
    # releases, runs_python, polls, switches_frame
    effects: frozenset[str] = frozenset()
    # It consumes or takes a count of an argument, or returns a new owner
    counts: bool = False
    # One argument is the destructor of the object that it closes
    destructor_arg: bool = False
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
    'BUILTINS': RowFacts(effects=frozenset({'reads'})),
    'COMPARISON_BIT': RowFacts(effects=frozenset()),
    'DK_UNICODE_ENTRIES': RowFacts(effects=frozenset({'reads'})),
    'FRAME_CODE_INSTRUMENTATION_VERSION': RowFacts(effects=frozenset({'reads'})),
    'FRAME_CO_CONSTS': RowFacts(effects=frozenset({'reads'})),
    'GETLOCAL': RowFacts(effects=frozenset(), home_load=True),
    'GLOBALS': RowFacts(effects=frozenset({'reads'})),
    'IR_SLOTS': RowFacts(effects=frozenset()),
    'PyDict_CheckExact': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyFunction_Check': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyList_CheckExact': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyList_GET_SIZE': RowFacts(effects=frozenset({'reads'})),
    'PyList_GET_VALUE': RowFacts(effects=frozenset({'reads'})),
    'PyModule_CheckExact': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyTuple_CheckExact': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyTuple_GET_SIZE': RowFacts(effects=frozenset({'reads'})),
    'PyTuple_GET_VALUE': RowFacts(effects=frozenset({'reads'})),
    'PyUnicode_CheckExact': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyValue_AsKnownObjectBorrow': RowFacts(effects=frozenset()),
    'PyValue_BoolCheck': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyValue_BothInlineInt': RowFacts(effects=frozenset(), conjoins='PyValue_IsInlineInt'),
    'PyValue_CLOSE': RowFacts(effects=frozenset({'releases'}), counts=True),
    'PyValue_CLOSE_SPECIALIZED': RowFacts(effects=frozenset(), counts=True, destructor_arg=True),
    'PyValue_DUP': RowFacts(effects=frozenset(), counts=True, never_null=True, nonnull_form='PyValue_XDUP'),
    'PyValue_FloatOperandValues': RowFacts(effects=frozenset()),
    'PyValue_FromDouble': RowFacts(effects=frozenset({'allocates', 'raises'}), counts=True, slow=True),
    'PyValue_FromDoubleNotNaN': RowFacts(effects=frozenset(), never_null=True),
    'PyValue_FromInlineInt': RowFacts(effects=frozenset(), never_null=True),
    'PyValue_FromInt64': RowFacts(effects=frozenset({'allocates', 'raises'}), counts=True, slow=True),
    'PyValue_IncrementTaggedIndexNoOverflow': RowFacts(effects=frozenset(), counts=True, never_null=True),
    'PyValue_InlineFloat': RowFacts(effects=frozenset()),
    'PyValue_InlineInt64': RowFacts(effects=frozenset()),
    'PyValue_Is': RowFacts(effects=frozenset()),
    'PyValue_IsFalse': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsInline': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsInlineFloat': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsInlineInt': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsNone': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsNull': RowFacts(effects=frozenset(), test=(False, True)),
    'PyValue_IsObject': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsPointerSpace': RowFacts(effects=frozenset(), test=(False, False)),
    'PyValue_IsTaggedIndex': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_IsTrue': RowFacts(effects=frozenset(), test=(True, False)),
    'PyValue_LongCheckExact': RowFacts(effects=frozenset({'reads'}), test=(True, False)),
    'PyValue_TagIndex': RowFacts(effects=frozenset(), never_null=True),
    'PyValue_UntagIndex': RowFacts(effects=frozenset()),
    'PyValue_XCLOSE': RowFacts(effects=frozenset({'releases'}), counts=True),
    'PyValue_XDUP': RowFacts(effects=frozenset(), counts=True),
    'Py_TYPE': RowFacts(effects=frozenset({'reads'})),
    '_PyDictValues_AddToInsertionOrder': RowFacts(effects=frozenset({'reads', 'writes'})),
    '_PyDictValues_SetShape': RowFacts(effects=frozenset({'writes'})),
    '_PyFloat_ExactDealloc': RowFacts(effects=frozenset({'writes'})),
    '_PyList_ITEMS': RowFacts(effects=frozenset({'reads'})),
    '_PyLong_ExactDealloc': RowFacts(effects=frozenset({'writes'})),
    '_PyLong_IsZero': RowFacts(effects=frozenset({'reads'}), test=(False, False)),
    '_PyObject_InlineValues': RowFacts(effects=frozenset({'reads'})),
    '_PyObject_Shape': RowFacts(effects=frozenset({'reads'})),
    '_PyObject_ShapeReadDup': RowFacts(effects=frozenset({'reads'}), counts=True, never_null=True),
    '_PyObject_ValueSlot': RowFacts(effects=frozenset()),
    '_PyTuple_ITEMS': RowFacts(effects=frozenset({'reads'})),
    '_PyUnicode_ExactDealloc': RowFacts(effects=frozenset({'writes'})),
    '_PyValueGuarded_Load': RowFacts(effects=frozenset(), never_null=True),
    '_PyValueSlot_Exchange': RowFacts(effects=frozenset({'reads', 'writes'}), counts=True),
    '_PyValueSlot_ExchangeFilled': RowFacts(effects=frozenset({'reads', 'writes'}), counts=True, never_null=True),
    '_PyValueSlot_Fill': RowFacts(effects=frozenset({'writes'}), counts=True),
    '_PyValue_AsKnownObjectBorrowPointer': RowFacts(effects=frozenset()),
    '_PyValue_BoundLocal': RowFacts(effects=frozenset(), never_null=True),
    '_PyValue_BoxDouble': RowFacts(effects=frozenset({'allocates', 'raises'}), counts=True),
    '_PyValue_BoxInt64': RowFacts(effects=frozenset({'allocates', 'raises'}), counts=True),
    '_PyValue_CellLocal': RowFacts(effects=frozenset(), never_null=True),
    '_PyValue_CodeConstant': RowFacts(effects=frozenset(), never_null=True),
    '_PyValue_CommonConstant': RowFacts(effects=frozenset(), never_null=True),
    '_PyValue_FromIntScaled': RowFacts(effects=frozenset(), never_null=True),
    '_PyValue_IntScaled': RowFacts(effects=frozenset()),
    '_PyValue_LshiftInt64': RowFacts(effects=frozenset({'allocates', 'raises'}), counts=True),
    '_PyValue_MultiplyInt64': RowFacts(effects=frozenset({'allocates', 'raises'}), counts=True),
    '_PyValue_NamesObject': RowFacts(effects=frozenset()),
    '_PyValue_PublishedItem': RowFacts(effects=frozenset(), never_null=True),
    '_Py_EvalBreaker': RowFacts(effects=frozenset({'reads'})),
    '_Py_EvalBreakerHasEvents': RowFacts(effects=frozenset({'reads'})),
    '_Py_HandlePending': RowFacts(effects=frozenset({'allocates', 'polls', 'raises', 'reads', 'releases', 'runs_python', 'writes'})),
}
