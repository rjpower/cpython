"""The closed catalogs of the register instruction definition language.

Types, schemas, effects, exits, operand roles, checkpoint roles, debts,
facts, row kinds, C conventions and head layouts.  Each catalog is closed:
a new runtime concept is a reviewed change here, never a free word in a
`.rop` file.  The language reference is the F0 report's `language.md`.
"""

import enum
from dataclasses import dataclass


class Scalar(enum.Enum):
    Unit = "Unit"
    Bool = "Bool"
    U8 = "U8"
    U16 = "U16"
    U32 = "U32"
    U64 = "U64"
    I32 = "I32"
    I64 = "I64"
    F64 = "F64"
    I51 = "I51"
    Scaled51 = "Scaled51"
    Size = "Size"

    @property
    def bits(self) -> int:
        """The width an immediate of this type needs in a head field."""
        return SCALAR_BITS[self]


SCALAR_BITS = {
    Scalar.Unit: 0, Scalar.Bool: 1, Scalar.U8: 8, Scalar.U16: 16, Scalar.U32: 32,
    Scalar.U64: 64, Scalar.I32: 32, Scalar.I64: 64, Scalar.F64: 64, Scalar.I51: 51,
    Scalar.Scaled51: 64, Scalar.Size: 64,
}


class Handle(enum.Enum):
    """Handles: values and resources that are not plain numbers.

    `B` is a borrowed PyValue, `O` an owning one.  `Src`, `Dst`, `Local`,
    `Cell`, `Free` name homes; `Name`, `Const` index the code's tables;
    `Head` is an instruction head of this image; `NearHead` and `FarHead`
    are target operands, signed and relative to the next head, of 12 and
    24 bits, which a graph reads as the `Head` they name; `Point` is a
    checkpoint."""
    B = "B"
    O = "O"
    Src = "Src"
    Dst = "Dst"
    Local = "Local"
    Cell = "Cell"
    Free = "Free"
    Name = "Name"
    Const = "Const"
    Head = "Head"
    NearHead = "NearHead"
    FarHead = "FarHead"
    Point = "Point"
    Site = "Site"
    Thread = "Thread"
    Activation = "Activation"
    Frame = "Frame"
    NewFrame = "NewFrame"
    DeadFrame = "DeadFrame"
    Code = "Code"
    Obj = "Obj"
    Fun = "Fun"
    Gen = "Gen"
    Cursor = "Cursor"
    IterPair = "IterPair"
    Args = "Args"
    Anchors = "Anchors"
    Err = "Err"
    Route = "Route"
    Claim = "Claim"
    Pin = "Pin"
    Stamp = "Stamp"


class Wrap(enum.Enum):
    """One-argument type constructors over types."""
    Maybe = "Maybe"
    Span = "Span"
    InOut = "InOut"


class SchemaCtor(enum.Enum):
    """Type constructors over a feedback schema."""
    Fb = "Fb"
    Recipe = "Recipe"
    Candidate = "Candidate"
    Rows = "Rows"
    Ticket = "Ticket"
    Arm = "Arm"


class Schema(enum.Enum):
    Binary = "Binary"
    Attr = "Attr"
    Global = "Global"
    Truth = "Truth"
    Compare = "Compare"
    Contains = "Contains"
    Item = "Item"
    Unpack = "Unpack"
    Iter = "Iter"
    Send = "Send"
    Call = "Call"
    Super = "Super"


# The fields of each schema's entry, beyond the header's `counter`.  A
# `feedback(read(entry.f))` clause names one of them.  F2 owns their C
# layout; this list is what definitions may name.
SCHEMA_FIELDS: dict[Schema, tuple[str, ...]] = {
    Schema.Binary: ("kind", "descr"),
    Schema.Attr: ("kind", "type_version", "offset", "shape", "keys_version", "index",
                  "descr", "rows"),
    Schema.Global: ("kind", "module_keys_version", "builtin_keys_version", "index"),
    Schema.Truth: ("kind", "type_version"),
    Schema.Compare: ("kind",),
    Schema.Contains: ("kind", "type_version"),
    Schema.Item: ("kind", "type_version", "func_version"),
    Schema.Unpack: ("kind", "length"),
    Schema.Iter: ("kind", "type_version", "func_version"),
    Schema.Send: ("kind", "func_version"),
    Schema.Call: ("kind", "func_version", "type_version", "argcount", "convention"),
    Schema.Super: ("kind", "type_version"),
}

# Every feedback site has these; `entry` covers `counter` and every field.
FEEDBACK_ROOTS = ("counter", "entry", "handler")


class AccessMode(enum.Enum):
    read = "read"
    write = "write"


class Effect(enum.Enum):
    """ir_contract.py's effect vocabulary, unchanged."""
    reads = "reads"
    writes = "writes"
    allocates = "allocates"
    raises = "raises"
    releases = "releases"
    runs_python = "runs_python"
    polls = "polls"
    switches_frame = "switches_frame"


READ_ONLY_EFFECTS = frozenset({Effect.reads})


class Region(enum.Enum):
    inline = "inline"
    shared = "shared"
    split = "split"


class FrameRole(enum.Enum):
    current = "current"
    caller = "caller"
    child = "child"
    consumer = "consumer"
    external = "external"


class PcRole(enum.Enum):
    site = "site"
    next_head = "next_head"
    target = "target"
    resume_head = "resume_head"
    saved_origin = "saved_origin"


class ExcRole(enum.Enum):
    clear = "clear"
    pending = "pending"
    handled = "handled"
    inherited = "inherited"


class DebtKind(enum.Enum):
    """The linear operands of `publish ... retire [...]`."""
    spent = "spent"
    old = "old"
    roots = "roots"
    owned = "owned"


class RowKind(enum.Enum):
    """`extern` kinds: a value primitive (has a leaf lowering), an
    out-of-line helper (SysV call) and a VM routine (may leave the word by
    its own exits)."""
    primitive = "primitive"
    helper = "helper"
    routine = "routine"


# The arm exits and their payload types.  `again`'s ticket is over the
# family's feedback schema: the checker substitutes it for SCHEMA_VAR.
class ExitKind(enum.Enum):
    next = "next"
    branch = "branch"
    jump = "jump"
    miss = "miss"
    again = "again"
    raise_ = "raise"
    enter = "enter"
    return_ = "return"
    yield_ = "yield"
    exit = "exit"
    redirect = "redirect"


# The environment every graph may read: the declared execution context
ENVIRONMENT = {
    "thread": Handle.Thread,
    "activation": Handle.Activation,
    "frame": Handle.Frame,
    "site": Handle.Site,
    "next_head": Handle.Head,
}

# Types of which an owner, error or resource must be accounted for: `_`
# cannot discard them (rule 2), and F1 tracks them as linear tokens.
LINEAR_HANDLES = frozenset({Handle.O, Handle.Err, Handle.Claim, Handle.Pin,
                            Handle.Anchors, Handle.NewFrame, Handle.DeadFrame,
                            Handle.Route})
LINEAR_SCHEMA_CTORS = frozenset({SchemaCtor.Ticket})


class Fact(enum.Enum):
    """Fact names.  F1 owns their meaning; F0 resolves the name, its number
    of subjects (FACT_ARITY) and its type argument (FACT_TYPED)."""
    nonnull = "nonnull"
    present = "present"
    object = "object"
    inline_int = "inline_int"
    inline_float = "inline_float"
    exact = "exact"
    outside_i51 = "outside_i51"
    incrementable = "incrementable"
    in_bounds = "in_bounds"
    type_version = "type_version"
    has_values = "has_values"
    tagged_index = "tagged_index"

    @property
    def arity(self) -> int:
        return FACT_ARITY.get(self, 1)

    @property
    def typed(self) -> bool:
        return self in FACT_TYPED


# in_bounds(i, o): index i is within o's length; type_version(o, r): o's
# type has the version recipe r names
FACT_ARITY = {Fact.in_bounds: 2, Fact.type_version: 2}
# exact<T>(v): v's type is exactly the builtin type T
FACT_TYPED = frozenset({Fact.exact})


class BuiltinType(enum.Enum):
    """The builtin types a fact may name, and their C type objects (the
    names ir_contract.py's `exact(...)` uses), so the migration check
    compares them directly."""
    int = "PyLong_Type"
    float = "PyFloat_Type"
    complex = "PyComplex_Type"
    bool = "PyBool_Type"
    NoneType = "_PyNone_Type"
    str = "PyUnicode_Type"
    bytes = "PyBytes_Type"
    bytearray = "PyByteArray_Type"
    list = "PyList_Type"
    tuple = "PyTuple_Type"
    dict = "PyDict_Type"
    set = "PySet_Type"
    frozenset = "PyFrozenSet_Type"
    range = "PyRange_Type"
    range_iterator = "PyRangeIter_Type"
    list_iterator = "PyListIter_Type"
    tuple_iterator = "PyTupleIter_Type"
    slice = "PySlice_Type"
    function = "PyFunction_Type"
    method = "PyMethod_Type"
    builtin_function = "PyCFunction_Type"
    method_descriptor = "PyMethodDescr_Type"
    classmethod = "PyClassMethod_Type"
    property = "PyProperty_Type"
    cell = "PyCell_Type"
    code = "PyCode_Type"
    generator = "PyGen_Type"
    coroutine = "PyCoro_Type"
    async_generator = "PyAsyncGen_Type"
    type = "PyType_Type"
    module = "PyModule_Type"
    super = "PySuper_Type"
    object = "PyBaseObject_Type"


# Encoding.  A family's encoding is derived from its operand types (V8
# derives a bytecode's from its operand types); there is no layout to
# declare.  The head's low 16 bits are handler and flags; A, B, C and D are
# its 12-bit operand fields.  P0..P7 are the 16-bit fields of up to two
# operand words that follow the head.  Vectors (spans, argument
# descriptors) follow in a variable payload.
class Slot(enum.Enum):
    A = (16, 12)
    B = (28, 12)
    C = (40, 12)
    D = (52, 12)
    P0 = (64, 16)
    P1 = (80, 16)
    P2 = (96, 16)
    P3 = (112, 16)
    P4 = (128, 16)
    P5 = (144, 16)
    P6 = (160, 16)
    P7 = (176, 16)

    @property
    def offset(self) -> int:
        return self.value[0]

    @property
    def width(self) -> int:
        return self.value[1]

    @property
    def word(self) -> int:
        """0 for the head, 1 and 2 for the operand words."""
        return self.offset // 64


# Fixed fields are filled in this order (the packing rule, rop_check.pack)
SLOT_ORDER = tuple(Slot)


class FieldKind(enum.Enum):
    """What an operand field holds.  A home, index or feedback field needs
    12 bits, a target its declared width (TARGET_BITS); an operand whose
    value does not fit at run time goes through the Wide descriptor.  An
    immediate needs its type's width; a vector takes no fixed field."""
    home = "home"
    imm = "imm"
    name = "name"
    const = "const"
    feedback = "feedback"
    target = "target"
    vector = "vector"


INDEX_BITS = 12
TARGET_BITS = {Handle.NearHead: 12, Handle.FarHead: 24}
# Widest immediate the rule packs: two adjacent fields of one word
WIDEST_IMMEDIATE = 32


# C conventions: how a C entry point's result maps onto the row's logical
# result, as data.  F2 generates every adapter from these records with one
# generic mapping and no per-convention code.
class CReturn(enum.Enum):
    """What the C function returns."""
    void = "void"
    bool = "bool"
    scalar = "scalar"            # the C scalar of the logical scalar type
    value = "PyValue"            # an owned or inline word
    borrow = "PyValueBorrow"
    pointer = "pointer"          # an object or frame pointer handle
    handle = "handle"            # the C type of the row's logical handle result
    int_status = "int"
    status_value = "_PyStatusValue"   # {int status; PyValue value;}


class CTest(enum.Enum):
    """The test of the C result that selects an outcome."""
    always = "always"
    true = "true"
    false = "false"
    nonnull = "nonnull"
    null = "null"
    negative = "negative"
    zero = "zero"
    positive = "positive"
    nonnegative = "nonnegative"
    nonzero = "nonzero"


class CSource(enum.Enum):
    """Where a payload component of an outcome comes from."""
    result = "result"                # the C return value itself
    value = "value"                  # a status_value's value field
    exception = "exception"          # the pending exception: an Err token
    nonzero = "nonzero"              # the C result compared with 0, as a Bool
    out = "out"                      # the scalar written through the last C parameter
    token = "token"                  # a proof with no runtime value (a Ticket)


@dataclass(frozen=True, slots=True)
class COutcome:
    """One logical outcome: when `test` holds of `subject` (the result or a
    status field), the row produced the case at this position, with payload
    from `payload`; `raised` says an exception is set."""
    test: CTest
    subject: CSource
    payload: tuple[CSource, ...]
    raised: bool
    # The logical case this outcome selects, when the convention fixes it by
    # name; None leaves it to the row's case order
    case: str | None = None


@dataclass(frozen=True, slots=True)
class Convention:
    """`single` conventions map onto `-> T` rows (one outcome); the others
    onto `cases(...)` with exactly len(outcomes) cases, in order."""
    name: str
    returns: CReturn
    single: bool
    outcomes: tuple[COutcome, ...]


def _conv(name: str, returns: CReturn, *outcomes: COutcome, single: bool = False
          ) -> Convention:
    return Convention(name, returns, single, outcomes)


_RES, _VAL, _EXC = CSource.result, CSource.value, CSource.exception

CONVENTIONS: dict[str, Convention] = {c.name: c for c in (
    _conv("unit", CReturn.void, COutcome(CTest.always, _RES, (), False), single=True),
    _conv("bool", CReturn.bool, COutcome(CTest.always, _RES, (_RES,), False), single=True),
    # An int test (nonzero is true) as a Bool
    _conv("int_as_bool", CReturn.int_status,
          COutcome(CTest.always, _RES, (CSource.nonzero,), False), single=True),
    _conv("scalar", CReturn.scalar, COutcome(CTest.always, _RES, (_RES,), False),
          single=True),
    _conv("value", CReturn.value, COutcome(CTest.always, _RES, (_RES,), False),
          single=True),
    _conv("borrow", CReturn.borrow, COutcome(CTest.always, _RES, (_RES,), False),
          single=True),
    # The result is the C type of the logical handle (Anchors)
    _conv("handle", CReturn.handle, COutcome(CTest.always, _RES, (_RES,), False),
          single=True),
    _conv("pointer", CReturn.pointer, COutcome(CTest.always, _RES, (_RES,), False),
          single=True),
    # ok(v) when the word is not NULL, err(e) with the exception set
    _conv("value_or_null", CReturn.value,
          COutcome(CTest.nonnull, _RES, (_RES,), False),
          COutcome(CTest.null, _RES, (_EXC,), True)),
    # hit(p)/absent(): a pointer or NULL, no exception
    _conv("pointer_or_null", CReturn.pointer,
          COutcome(CTest.nonnull, _RES, (_RES,), False),
          COutcome(CTest.null, _RES, (), False)),
    # A status int: 0 is success, negative raised
    _conv("int_zero_ok", CReturn.int_status,
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # A truth int: 1 true, 0 false, negative raised
    _conv("int_truth", CReturn.int_status,
          COutcome(CTest.positive, _RES, (), False),
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # An adaptation status: positive changed (the ticket is a proof, not a
    # value), zero same, negative blocked; none of them raises
    _conv("ticket_status", CReturn.int_status,
          COutcome(CTest.positive, _RES, (CSource.token,), False),
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.negative, _RES, (), False)),
    # Whether a state bit is set: false the first case, true the second, whose
    # payload is a proof
    _conv("redirect_status", CReturn.bool,
          COutcome(CTest.false, _RES, (), False),
          COutcome(CTest.true, _RES, (CSource.token,), False)),
    # bool tests as two cases: true first
    _conv("bool_cases", CReturn.bool,
          COutcome(CTest.true, _RES, (), False),
          COutcome(CTest.false, _RES, (), False)),
    # A checked scalar operation: the bool says whether it overflowed, and the
    # scalar written through the last parameter is the result (false, the
    # `fits` case) or the wide result (true, the `overflow` case)
    _conv("bool_out", CReturn.bool,
          COutcome(CTest.false, _RES, (CSource.out,), False, "fits"),
          COutcome(CTest.true, _RES, (CSource.out,), False, "overflow")),
    # _PyStatusValue: status 0 the value, 1 the second kind, -1 raised
    _conv("status_value", CReturn.status_value,
          COutcome(CTest.zero, _RES, (_VAL,), False),
          COutcome(CTest.positive, _RES, (_VAL,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
)}


# The evidence an extern with a C body carries (root, 2026-11-24): the
# review note and why its effects and exits hold; a primitive also names
# its leaf lowering and the differential test of that lowering.
EVIDENCE_REQUIRED = ("review", "contract")
EVIDENCE_PRIMITIVE = ("lowering", "test")
EVIDENCE_KEYS = EVIDENCE_REQUIRED + EVIDENCE_PRIMITIVE


__all__ = [n for n in dir() if not n.startswith("_")]
