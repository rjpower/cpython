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
    SendWindow = "SendWindow"
    # A projection `g.p` of a home group: a temporary home the instruction
    # reads and writes, never a local's
    Part = "Part"
    Args = "Args"
    Anchors = "Anchors"
    Err = "Err"
    Route = "Route"
    # A handler id: the handler that a word runs (a displacing arm's dispatch)
    Handler = "Handler"
    Claim = "Claim"
    Pin = "Pin"
    Stamp = "Stamp"


@dataclass(frozen=True)
class HomePart:
    """One projection of a home group.  index: its declared type admits a
    tagged index (a loop's cursor is "a tagged index or nothing"), so a store
    to it is checked as a temporary."""
    name: str
    index: bool = False


# Home groups: one home field naming a run of adjacent temporaries, which
# the translator keeps for the life of the loop.  Projection i, `g.p`, is
# the home at the field plus i.
HOME_GROUP_PARTS = {
    Handle.IterPair: (HomePart("receiver"), HomePart("cursor", index=True)),
    Handle.SendWindow: (HomePart("receiver"), HomePart("cursor", index=True),
                        HomePart("value")),
}
HOME_GROUPS = {h: tuple(p.name for p in ps) for h, ps in HOME_GROUP_PARTS.items()}


class Wrap(enum.Enum):
    """One-argument type constructors over types."""
    Maybe = "Maybe"
    Span = "Span"
    InOut = "InOut"
    # A home group the word defines and does not read (a loop's open)
    Out = "Out"


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
                  "descr", "rows", "class_value", "shape_to"),
    Schema.Global: ("globals_version", "builtins_version", "index"),
    Schema.Truth: ("kind", "type_version"),
    Schema.Compare: ("kind",),
    Schema.Contains: ("kind", "type_version"),
    Schema.Item: ("kind", "type_version", "func_version"),
    Schema.Unpack: ("kind", "length"),
    Schema.Iter: ("kind", "type_version", "func_version"),
    Schema.Send: ("kind", "func_version"),
    Schema.Call: ("kind", "func_version", "type_version", "argcount", "convention",
                  "plan"),
    Schema.Super: ("kind", "type_version"),
}

# Every feedback site has these; `entry` covers `counter` and every field.
FEEDBACK_ROOTS = ("counter", "entry", "handler")


# The C layout of the schemas: the one place that states which C member and
# C type hold each field.  The C structs stay hand-written (they are shared
# with the stack handlers and the specialiser); the generator checks each
# member's type with a static assert (rop_generator.write_layout_checks), and
# a reader takes the member's offset from C (`&fb->member`), never from here.

@dataclass(frozen=True, slots=True)
class CField:
    """A field held by one C member (a path such as `h.poly`) of one C type,
    or by none yet (member None): a template that reads it is refused"""
    member: str | None
    ctype: str = ""
    # Other C members that name the same storage for this field, which C
    # code also reads it by
    aliases: tuple[str, ...] = ()


@dataclass(frozen=True, slots=True)
class CUnion:
    """Fields that one C union holds: an entry holds the one its installing
    arm wrote, so an arm reads at most one of them.  `members` are all the
    union's C members, which the static checks place at one offset; C code
    spells the storage by any of them."""
    fields: tuple[str, ...]
    members: tuple[str, ...]


@dataclass(frozen=True, slots=True)
class CLayout:
    struct: str
    fields: dict[str, CField]
    unions: tuple[CUnion, ...] = ()

    def union_of(self, field: str) -> CUnion | None:
        return next((u for u in self.unions if field in u.fields), None)

    def field_of_member(self, member: str) -> str | None:
        "The schema field that C code reads by this member, if any"
        for name, f in self.fields.items():
            if member == f.member or member in f.aliases:
                return name
        return None


_NONE = CField(None)
_U32 = "uint32_t"

FB_LAYOUT: dict[Schema, CLayout] = {
    Schema.Binary: CLayout("_PyFbBinary", {
        "kind": _NONE, "descr": CField("descr", "const void *")}),
    Schema.Attr: CLayout("_PyFbAttr", {
        "kind": CField("read_kind", _U32),
        "type_version": CField("type_version", _U32),
        "offset": CField("offset", _U32),
        "shape": CField("shape", _U32),
        "keys_version": CField("dict_version", _U32, aliases=("class_version",)),
        "index": CField("index", _U32),
        "descr": CField("descr", "PyObject *"),
        "rows": CField("h.poly", _U32),
        "class_value": CField("class_value", "_PyValueGuarded"),
        "shape_to": CField("shape_to", _U32)},
        unions=(
            # A module's keys version, a class's version or a shape index
            CUnion(("keys_version", "shape"),
                   ("dict_version", "func_version", "class_version", "shape")),
            CUnion(("offset", "index"), ("index", "offset", "hint", "dictoffset")),
            CUnion(("descr", "shape_to"), ("descr", "fget", "getattribute", "shape_to")))),
    # The super sites record nothing but the counter: the guards read the
    # operands
    Schema.Super: CLayout("_PyFbUnary", {"kind": _NONE, "type_version": _NONE}),
    Schema.Global: CLayout("_PyFbGlobal", {
        "globals_version": CField("globals_version", _U32),
        "builtins_version": CField("builtins_version", _U32),
        "index": CField("index", _U32)}),
    Schema.Truth: CLayout("_PyFbUnary", {
        "kind": _NONE, "type_version": CField("type_version", _U32)}),
    Schema.Compare: CLayout("_PyFbCompare", {"kind": CField("kind", _U32)}),
    Schema.Contains: CLayout("_PyFbContains", {
        "kind": CField("kind", _U32),
        "type_version": CField("type_version", _U32)}),
    Schema.Unpack: CLayout("_PyFbUnary", {"kind": _NONE, "length": _NONE}),
    # The item sites share the binary entry; their evidence is the type
    # versions that it records
    Schema.Item: CLayout("_PyFbBinary", {
        "kind": _NONE, "type_version": _NONE, "func_version": _NONE}),
    Schema.Iter: CLayout("_PyFbUnary", {
        "kind": _NONE, "type_version": _NONE, "func_version": _NONE}),
    Schema.Call: CLayout("_PyFbCall", {
        "kind": _NONE, "func_version": CField("func_version", _U32),
        "type_version": CField("type_version", _U32),
        "argcount": _NONE, "convention": _NONE,
        "plan": CField("plan", "_PyBindPlan")}),
}

# A recipe (Recipe<S>): the fields that an admission row copies out of an
# entry together, as the rows' C struct holds them.  A template keeps a
# recipe in registers, one value per field.
RECIPE_LAYOUT: dict[Schema, CLayout] = {
    Schema.Attr: CLayout("_PyAttrRecipe", {
        "type_version": CField("type_version", _U32),
        "aux": CField("aux", _U32),
        "offset": CField("offset", _U32),
        "read_kind": CField("read_kind", _U32),
        "shape_to": CField("shape_to", _U32),
        "values_layout": CField("values_layout", "bool"),
        "class_value": CField("class_value", "_PyValueGuarded")}),
    Schema.Global: CLayout("_PyGlobalRecipe", {
        "globals_version": CField("globals_version", _U32),
        "builtins_version": CField("builtins_version", _U32),
        "index": CField("index", _U32)}),
}


class AccessMode(enum.Enum):
    read = "read"
    write = "write"


class Effect(enum.Enum):
    """The effect vocabulary of the declarations."""
    reads = "reads"
    writes = "writes"
    allocates = "allocates"
    raises = "raises"
    releases = "releases"
    runs_python = "runs_python"
    polls = "polls"
    switches_frame = "switches_frame"


READ_ONLY_EFFECTS = frozenset({Effect.reads})


class MonitoringEvent(enum.Enum):
    """The sys.monitoring events that a family's observer arm can fire,
    by the names of PY_MONITORING_EVENT_*.  LINE and INSTRUCTION are not a
    family's: every word has them."""
    PY_START = "PY_START"
    PY_RESUME = "PY_RESUME"
    PY_RETURN = "PY_RETURN"
    PY_YIELD = "PY_YIELD"
    CALL = "CALL"
    JUMP = "JUMP"
    BRANCH = "BRANCH"
    BRANCH_LEFT = "BRANCH_LEFT"
    BRANCH_RIGHT = "BRANCH_RIGHT"
    STOP_ITERATION = "STOP_ITERATION"
    RAISE = "RAISE"
    EXCEPTION_HANDLED = "EXCEPTION_HANDLED"
    PY_UNWIND = "PY_UNWIND"
    PY_THROW = "PY_THROW"
    RERAISE = "RERAISE"
    C_RETURN = "C_RETURN"
    C_RAISE = "C_RAISE"


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
    # Where a monitoring hook left the frame's instruction pointer (a callback
    # may have moved it): the error is raised from there
    left = "left"


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
    """`extern` kinds: a value primitive (has a leaf lowering) and an
    out-of-line helper (SysV call).  A row that leaves the word itself is a
    helper bound with the `transfer` convention."""
    primitive = "primitive"
    helper = "helper"


# The arm exits and their payload types.  `again`'s ticket is over the
# family's feedback schema: the checker substitutes it for SCHEMA_VAR.
class ExitKind(enum.Enum):
    next = "next"
    branch = "branch"
    jump = "jump"
    miss = "miss"
    again = "again"
    raise_ = "raise"
    # Propagates an exception the word already reported: no traceback
    # entry and no RAISE event (the old `goto exception_unwind`)
    unwind = "unwind"
    enter = "enter"
    return_ = "return"
    yield_ = "yield"
    exit = "exit"
    redirect = "redirect"
    # The word goes on under another handler, with its own fields: only an arm
    # declared `displacing` (the word that monitoring put in place of another)
    dispatch = "dispatch"


# The static evidence that selects an arm of a family without a miss
# (`select(...)`): the translator, or a static word array, knows it when it
# emits the word.  Each dimension has exactly one member that holds; an arm
# names at most one member per dimension, and omitting a dimension admits
# every member of it.
class Select(enum.Enum):
    thread_frame = "thread_frame"          # the code runs in a thread frame
    generator_frame = "generator_frame"    # in a generator-owned frame
    entry_frame = "entry_frame"            # the evaluator's entry frame
    vacant_scratch = "vacant_scratch"      # every temporary but the operand is vacant
    live_scratch = "live_scratch"          # some other temporary may hold a value
    transfer = "transfer"                  # Move: each source's owner goes to its destination
    copy = "copy"                          # Move: each destination takes a second owner
    pool = "pool"                          # Const: the code's constant
    small = "small"                        # Const: an immediate int
    common = "common"                      # Const: an interpreter root
    bound = "bound"                        # LocalRead: the compiler proved the local bound
    checked = "checked"                    # LocalRead: the local may be unbound
    store = "store"                        # LocalWrite: the value is present
    restore = "restore"                    # LocalWrite: a saved binding, maybe absent
    uninterrupted = "uninterrupted"        # Jump: no poll (a forward jump, a yield-from loop's)
    interruptible = "interruptible"        # Jump: a backedge polls
    jit_off = "jit_off"                    # the baseline JIT is off
    jit_on = "jit_on"                      # the baseline JIT is on, so a backedge warms the code
    unary = "unary"                        # Intrinsic: one object in, one out
    import_star = "import_star"            # Intrinsic: writes the locals namespace
    stopiteration = "stopiteration"        # Intrinsic: a generator's StopIteration conversion
    binary = "binary"                      # Intrinsic: type parameters, exception groups, isinstance
    attr = "attr"                          # Intrinsic: an attribute by a name the instruction cannot hold
    named = "named"                        # NamespaceRead: a name in the locals, globals and builtins
    locals_scope = "locals_scope"          # NamespaceRead: the locals themselves
    build_class_scope = "build_class_scope"  # NamespaceRead: the builtins' __build_class__
    name_scope = "name_scope"              # NamespaceWrite: the locals
    global_scope = "global_scope"          # NamespaceWrite: the globals
    eager_import = "eager_import"          # Import: forced eager
    policy_import = "policy_import"        # Import: lazy when the policy says so
    lazy_import = "lazy_import"            # Import: `lazy import`
    module_kept = "module_kept"            # ImportFrom: the module stays for the next read
    module_spent = "module_spent"          # ImportFrom: the last read spends the module
    tuple = "tuple"                        # Build: a tuple display
    list = "list"                          # Build: a list display
    set = "set"                            # Build: a set display
    map = "map"                            # Build: a dict display
    append = "append"                      # AddElement: a list takes the item
    add = "add"                            # AddElement: a set takes the item
    extend = "extend"                      # UpdateContainer: a list takes an iterable
    union = "union"                        # UpdateContainer: a set takes an iterable
    overlay = "overlay"                    # UpdateContainer: a dict takes a mapping
    join = "join"                          # BuildText: the pieces of an f-string
    interpolation = "interpolation"        # BuildText: one `{value!c:spec}` of a template
    template = "template"                  # BuildText: the strings and interpolations of a template
    present = "present"                    # the family's optional operands hold values
    absent = "absent"                      # its optional operands are vacant


SELECT_DIMENSIONS: dict[str, tuple[Select, ...]] = {
    "frame": (Select.thread_frame, Select.generator_frame, Select.entry_frame),
    "scratch": (Select.vacant_scratch, Select.live_scratch),
    "move": (Select.transfer, Select.copy),
    "const": (Select.pool, Select.small, Select.common),
    "read": (Select.bound, Select.checked),
    "write": (Select.store, Select.restore),
    "interrupt": (Select.uninterrupted, Select.interruptible),
    "jit": (Select.jit_off, Select.jit_on),
    "intrinsic1": (Select.unary, Select.import_star, Select.stopiteration),
    "intrinsic2": (Select.binary, Select.attr),
    "name_read": (Select.named, Select.locals_scope, Select.build_class_scope),
    "namespace": (Select.name_scope, Select.global_scope),
    "import_kind": (Select.eager_import, Select.policy_import, Select.lazy_import),
    "module_use": (Select.module_kept, Select.module_spent),
    "display": (Select.tuple, Select.list, Select.set, Select.map),
    "element": (Select.append, Select.add),
    "update": (Select.extend, Select.union, Select.overlay),
    "text": (Select.join, Select.interpolation, Select.template),
    "operand": (Select.present, Select.absent),
}


# The environment every graph may read: the declared execution context
ENVIRONMENT = {
    "thread": Handle.Thread,
    "activation": Handle.Activation,
    "frame": Handle.Frame,
    "site": Handle.Site,
    "next_head": Handle.Head,
    # The instruction pointer before the word saved it: a displacing arm's
    "prev_head": Handle.Head,
}

# The C expression of each implicit name in every product.  The prologue of
# a case, a function form and a stub binds the same names: `this_instr` is
# the head's address (the stub's `site`), `next_instr` the head after the
# instruction.  The activation has no C value yet and is not listed.
ENVIRONMENT_C = {
    "thread": "tstate",
    "frame": "frame",
    "site": "this_instr",
    "next_head": "next_instr",
    "prev_head": "prev_instr",
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
    # An inline int or an inline float: what PyValue_FloatOperandsCheck admits
    inline_number = "inline_number"
    exact = "exact"
    outside_i51 = "outside_i51"
    incrementable = "incrementable"
    in_bounds = "in_bounds"
    type_version = "type_version"
    has_values = "has_values"
    tagged_index = "tagged_index"
    # Facts of the execution context that select evidence states
    # (SELECT_FACTS) and rows require
    thread_frame = "thread_frame"
    generator_frame = "generator_frame"
    entry_frame = "entry_frame"
    vacant_scratch = "vacant_scratch"
    interruptible = "interruptible"
    jit_on = "jit_on"
    # The observing evaluator runs the word: an `observes` arm's entry
    instrumented = "instrumented"

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


# The facts each select item states at the arm's entry, and their subjects:
# an environment name, the family operand of that name, or OPTIONAL, which
# stands for each of the family's Maybe<Src> operands.  The other items
# choose a body and state no fact.
OPTIONAL = "*optional"
SELECT_FACTS: dict[Select, tuple[tuple[Fact, str], ...]] = {
    Select.thread_frame: ((Fact.thread_frame, "frame"),),
    Select.generator_frame: ((Fact.generator_frame, "frame"),),
    Select.entry_frame: ((Fact.entry_frame, "frame"),),
    Select.vacant_scratch: ((Fact.vacant_scratch, "frame"),),
    Select.interruptible: ((Fact.interruptible, "site"),),
    Select.jit_on: ((Fact.jit_on, "thread"),),
    Select.bound: ((Fact.nonnull, "local"),),
    Select.present: ((Fact.present, OPTIONAL),),
}


class BuiltinType(enum.Enum):
    """The builtin types a fact may name, and their C type objects (the
    names an `exact(...)` fact uses), so a check compares them directly."""
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


# A value with one of these facts owns no count: an inline word, or an
# immortal object, whose references always carry Py_TAG_REFCNT
# (pycore_value.h).  Retiring it closes nothing.
UNCOUNTED_FACTS = frozenset({
    (Fact.inline_int, None), (Fact.inline_float, None), (Fact.inline_number, None),
    (Fact.exact, BuiltinType.bool), (Fact.exact, BuiltinType.NoneType),
})


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
    flag = "flag"


INDEX_BITS = 12
# One-bit enums that live in a flag of the word's handler field rather than
# in a field: the enum's name and the flag's bit (pycore_ir.h IR_F_*)
HEAD_FLAG_ENUMS = {"Interrupt": 15}
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
    # The C result is the index of the case, in declaration order; a case
    # with one payload value gets it through the last C parameter.  The row
    # declares any number of cases.
    indexed: bool = False
    # The callee performs the exit that each case names (TRANSFER_EXITS) and
    # does not return: the cases are stated for the checker and the tracer
    transfer: bool = False


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
    # Always raises: void, with the exception set
    _conv("raised", CReturn.void, COutcome(CTest.always, _RES, (_EXC,), True)),
    # A pointer, or NULL with the exception set
    _conv("pointer_or_raised", CReturn.pointer,
          COutcome(CTest.nonnull, _RES, (_RES,), False),
          COutcome(CTest.null, _RES, (_EXC,), True)),
    # A transfer row: the callee leaves the word by the exit its case names
    # (TRANSFER_EXITS), so no C result selects the case
    Convention("transfer", CReturn.void, False, (), transfer=True),
    # hit(p)/absent(): a pointer or NULL, no exception
    _conv("pointer_or_null", CReturn.pointer,
          COutcome(CTest.nonnull, _RES, (_RES,), False),
          COutcome(CTest.null, _RES, (), False)),
    # hit(v)/absent(): the bool says whether the value written through the
    # last parameter is there; no exception
    _conv("bool_out_hit", CReturn.bool,
          COutcome(CTest.true, _RES, (CSource.out,), False, "hit"),
          COutcome(CTest.false, _RES, (), False, "absent")),
    # ok(v, w) when the word is not NULL, w written through the last
    # parameter; err(e) with the exception set
    _conv("value_out_or_null", CReturn.value,
          COutcome(CTest.nonnull, _RES, (_RES, CSource.out), False),
          COutcome(CTest.null, _RES, (_EXC,), True)),
    # k(): the int is the position of the case among the row's cases
    Convention("case_index", CReturn.int_status, False, (), True),
    # A status int: 0 is success, negative raised
    _conv("int_zero_ok", CReturn.int_status,
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # ok(h): zero, the scalar written through the last parameter; err(e):
    # negative, the exception set
    _conv("out_or_raised", CReturn.int_status,
          COutcome(CTest.zero, _RES, (CSource.out,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # ok(h): zero, the scalar written through the last parameter; moved:
    # positive, the frame moved (the token is the route to the word);
    # negative raised
    _conv("out_redirect", CReturn.int_status,
          COutcome(CTest.zero, _RES, (CSource.out,), False),
          COutcome(CTest.positive, _RES, (CSource.token,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # ok(vals): zero, the items the publish writes (a Span<O>) written
    # through the last parameter; err(e): negative, the exception set and
    # nothing held
    _conv("items_or_raised", CReturn.int_status,
          COutcome(CTest.zero, _RES, (CSource.out,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # A truth int: 1 true, 0 false, negative raised
    _conv("int_truth", CReturn.int_status,
          COutcome(CTest.positive, _RES, (), False),
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # A branch event: zero ran on, positive moved the frame (the token is the
    # route to the word), negative raised
    _conv("int_redirect", CReturn.int_status,
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.positive, _RES, (CSource.token,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # A value that an event may take: zero the value, positive moved the
    # frame (the token is the route to the word), negative raised
    _conv("value_redirect", CReturn.status_value,
          COutcome(CTest.zero, _RES, (_VAL,), False),
          COutcome(CTest.positive, _RES, (CSource.token,), False),
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
    # _PyStatusValue of an optional lookup: found(value) for a positive
    # status, missing() for zero, raised for a negative one
    _conv("status_optional", CReturn.status_value,
          COutcome(CTest.positive, _RES, (_VAL,), False),
          COutcome(CTest.zero, _RES, (), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # A loop step as _PyStatusValue: positive an item, its value the first
    # payload and the next cursor written through the last parameter; zero
    # the end, its value the payload; negative raised
    _conv("step_out", CReturn.status_value,
          COutcome(CTest.positive, _RES, (_VAL, CSource.out), False),
          COutcome(CTest.zero, _RES, (_VAL,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
    # _PyStatusValue: status 1 an item (the next cursor written through the last
    # parameter), 0 the end; neither raises
    _conv("step_pair", CReturn.status_value,
          COutcome(CTest.positive, _RES, (_VAL, CSource.out), False),
          COutcome(CTest.zero, _RES, (_VAL,), False)),
    # _PyStatusValue: status 0 the value, 1 the second kind, -1 raised
    _conv("status_value", CReturn.status_value,
          COutcome(CTest.zero, _RES, (_VAL,), False),
          COutcome(CTest.positive, _RES, (_VAL,), False),
          COutcome(CTest.negative, _RES, (_EXC,), True)),
)}


# A transfer row's cases: each is named for the arm exit that the callee
# performs, and its payload is that exit's (a raise's point is the arm's)
TRANSFER_EXITS = {
    "jump": "jump", "branch": "branch", "return": "return",
    "redirect": "redirect", "exit": "exit", "raise": "raise",
}


# The evidence an extern with a C body carries (root, 2026-11-24): the
# review note and why its effects and exits hold; a primitive also names
# its leaf lowering and the differential test of that lowering.
EVIDENCE_REQUIRED = ("review", "contract")
EVIDENCE_PRIMITIVE = ("lowering", "test")
EVIDENCE_KEYS = EVIDENCE_REQUIRED + EVIDENCE_PRIMITIVE


__all__ = [n for n in dir() if not n.startswith("_")]
