from dataclasses import dataclass
import itertools
import lexer
import parser
import re
import root_check
from typing import Optional, Callable

from parser import Stmt, SimpleStmt, BlockStmt, IfStmt, WhileStmt, ForStmt, MacroIfStmt

@dataclass
class EscapingCall:
    stmt: SimpleStmt
    call: lexer.Token

@dataclass
class Properties:
    escaping_calls: dict[SimpleStmt, EscapingCall]
    escapes: bool
    error_with_pop: bool
    error_without_pop: bool
    deopts: bool
    oparg: bool
    jumps: bool
    eval_breaker: bool
    needs_this: bool
    always_exits: bool
    sync_sp: bool
    uses_co_consts: bool
    uses_co_names: bool
    uses_locals: bool
    has_free: bool
    side_exit: bool
    pure: bool
    uses_opcode: bool
    tier: int | None = None
    needs_prev: bool = False
    no_save_ip: bool = False
    inline_ok: bool = False
    # The IR binds this instruction in window style (see ir_generator.py).
    window: bool = False
    # Assigns `frame`: what follows runs in another frame.
    switches_frame: bool = False
    # Annotated `returns`: pops its frame, and pushes its outputs to the
    # caller's.
    returns: bool = False
    # Annotated `stores_early`: the body stores the output that goes to the
    # caller's frame with IR_RETURN_EARLY() before the frame switch, so the
    # caller's slot holds it while the source frame is unlinked and cleared
    # (boil/design/11-precise-roots.md section 4.4).  The output stays
    # declared for the stack effects; no store is generated for it.
    stores_early: bool = False

    def dump(self, indent: str) -> None:
        simple_properties = self.__dict__.copy()
        del simple_properties["escaping_calls"]
        text = "escaping_calls:\n"
        for tkns in self.escaping_calls.values():
            text += f"{indent}    {tkns}\n"
        text += ", ".join([f"{key}: {value}" for (key, value) in simple_properties.items()])
        print(indent, text, sep="")

    @staticmethod
    def from_list(properties: list["Properties"]) -> "Properties":
        escaping_calls: dict[SimpleStmt, EscapingCall] = {}
        for p in properties:
            escaping_calls.update(p.escaping_calls)
        return Properties(
            escaping_calls=escaping_calls,
            escapes = any(p.escapes for p in properties),
            error_with_pop=any(p.error_with_pop for p in properties),
            error_without_pop=any(p.error_without_pop for p in properties),
            deopts=any(p.deopts for p in properties),
            oparg=any(p.oparg for p in properties),
            jumps=any(p.jumps for p in properties),
            eval_breaker=any(p.eval_breaker for p in properties),
            needs_this=any(p.needs_this for p in properties),
            always_exits=any(p.always_exits for p in properties),
            sync_sp=any(p.sync_sp for p in properties),
            uses_co_consts=any(p.uses_co_consts for p in properties),
            uses_co_names=any(p.uses_co_names for p in properties),
            uses_locals=any(p.uses_locals for p in properties),
            uses_opcode=any(p.uses_opcode for p in properties),
            has_free=any(p.has_free for p in properties),
            side_exit=any(p.side_exit for p in properties),
            pure=all(p.pure for p in properties),
            needs_prev=any(p.needs_prev for p in properties),
            no_save_ip=all(p.no_save_ip for p in properties),
            inline_ok=all(p.inline_ok for p in properties),
            window=any(p.window for p in properties),
            switches_frame=any(p.switches_frame for p in properties),
            returns=any(p.returns for p in properties),
            stores_early=any(p.stores_early for p in properties),
        )

    @property
    def infallible(self) -> bool:
        return not self.error_with_pop and not self.error_without_pop

SKIP_PROPERTIES = Properties(
    escaping_calls={},
    escapes=False,
    error_with_pop=False,
    error_without_pop=False,
    deopts=False,
    oparg=False,
    jumps=False,
    eval_breaker=False,
    needs_this=False,
    always_exits=False,
    sync_sp=False,
    uses_co_consts=False,
    uses_co_names=False,
    uses_locals=False,
    uses_opcode=False,
    has_free=False,
    side_exit=False,
    pure=True,
    no_save_ip=False,
    inline_ok=True,
)


@dataclass
class Skip:
    "Unused cache entry"
    size: int

    @property
    def name(self) -> str:
        return f"unused/{self.size}"

    @property
    def properties(self) -> Properties:
        return SKIP_PROPERTIES


class Flush:
    @property
    def properties(self) -> Properties:
        return SKIP_PROPERTIES

    @property
    def name(self) -> str:
        return "flush"

    @property
    def size(self) -> int:
        return 0




@dataclass
class StackItem:
    name: str
    size: str
    peek: bool = False
    used: bool = False

    def __str__(self) -> str:
        size = f"[{self.size}]" if self.size else ""
        return f"{self.name}{size} {self.peek}"

    def is_array(self) -> bool:
        return self.size != ""

    def get_size(self) -> str:
        return self.size if self.size else "1"


@dataclass
class StackEffect:
    inputs: list[StackItem]
    outputs: list[StackItem]

    def __str__(self) -> str:
        return f"({', '.join([str(i) for i in self.inputs])} -- {', '.join([str(i) for i in self.outputs])})"


@dataclass
class CacheEntry:
    name: str
    size: int

    def __str__(self) -> str:
        return f"{self.name}/{self.size}"


@dataclass
class Uop:
    name: str
    context: parser.Context | None
    annotations: list[str]
    stack: StackEffect
    caches: list[CacheEntry]
    local_stores: list[lexer.Token]
    body: BlockStmt
    properties: Properties
    _size: int = -1
    implicitly_created: bool = False
    # Size of the instruction(s), only set for uops containing the INSTRUCTION_SIZE macro
    instruction_size: int | None = None
    # Scalar stack inputs that the body converts to PyObject *
    object_inputs: tuple[str, ...] = ()
    # Scalar stack inputs that the body asserts to be objects
    asserted_objects: tuple[str, ...] = ()
    # Outputs that the body assigns from an input, `out = in;`
    aliased_outputs: tuple[str, ...] = ()

    def dump(self, indent: str) -> None:
        print(
            indent, self.name, ", ".join(self.annotations) if self.annotations else ""
        )
        print(indent, self.stack, ", ".join([str(c) for c in self.caches]))
        self.properties.dump("    " + indent)

    @property
    def size(self) -> int:
        if self._size < 0:
            self._size = sum(c.size for c in self.caches)
        return self._size

    def why_not_viable(self) -> str | None:
        if self.name == "_SAVE_RETURN_SLOT":
            return None  # Adjusts next_instr, but only in tier 1 code
        if "INSTRUMENTED" in self.name:
            return "is instrumented"
        if "replaced" in self.annotations:
            return "is replaced"
        if self.name in ("INTERPRETER_EXIT", "JUMP_BACKWARD"):
            return "has tier 1 control flow"
        if self.properties.needs_this:
            return "uses the 'this_instr' variable"
        if len([c for c in self.caches if c.name != "unused"]) > 2:
            return "has too many cache entries"
        if self.properties.error_with_pop and self.properties.error_without_pop:
            return "has both popping and not-popping errors"
        return None

    def is_viable(self) -> bool:
        return self.why_not_viable() is None

    def is_guard(self) -> bool:
        """A guard only inspects its inputs: it may exit, but it does not
        escape and leaves the stack unchanged."""
        return (
            (self.properties.deopts or self.properties.side_exit)
            and not self.properties.escapes
            and len(self.stack.inputs) == len(self.stack.outputs)
            and all(item.peek for item in self.stack.inputs)
        )

    def is_super(self) -> bool:
        for tkn in self.body.tokens():
            if tkn.kind == "IDENTIFIER" and tkn.text == "oparg1":
                return True
        return False


class Label:

    def __init__(self, name: str, spilled: bool, body: BlockStmt, properties: Properties):
        self.name = name
        self.spilled = spilled
        self.body = body
        self.properties = properties

    size:int = 0
    local_stores: list[lexer.Token] = []
    instruction_size = None

    def __str__(self) -> str:
        return f"label({self.name})"


Part = Uop | Skip | Flush
CodeSection = Uop | Label


@dataclass
class Instruction:
    where: lexer.Token
    name: str
    parts: list[Part]
    _properties: Properties | None
    is_target: bool = False
    family: Optional["Family"] = None
    opcode: int = -1

    @property
    def properties(self) -> Properties:
        if self._properties is None:
            self._properties = self._compute_properties()
        return self._properties

    def _compute_properties(self) -> Properties:
        return Properties.from_list([part.properties for part in self.parts])

    def dump(self, indent: str) -> None:
        print(indent, self.name, "=", ", ".join([part.name for part in self.parts]))
        self.properties.dump("    " + indent)

    @property
    def size(self) -> int:
        return 1 + sum(part.size for part in self.parts)

    def is_super(self) -> bool:
        if len(self.parts) != 1:
            return False
        uop = self.parts[0]
        if isinstance(uop, Uop):
            return uop.is_super()
        else:
            return False


@dataclass
class PseudoInstruction:
    name: str
    stack: StackEffect
    targets: list[Instruction]
    as_sequence: bool
    flags: list[str]
    opcode: int = -1

    def dump(self, indent: str) -> None:
        print(indent, self.name, "->", " or ".join([t.name for t in self.targets]))

    @property
    def properties(self) -> Properties:
        return Properties.from_list([i.properties for i in self.targets])


@dataclass
class Family:
    name: str
    size: str
    members: list[Instruction]

    def dump(self, indent: str) -> None:
        print(indent, self.name, "= ", ", ".join([m.name for m in self.members]))


@dataclass
class Analysis:
    instructions: dict[str, Instruction]
    uops: dict[str, Uop]
    families: dict[str, Family]
    pseudos: dict[str, PseudoInstruction]
    labels: dict[str, Label]
    opmap: dict[str, int]
    have_arg: int
    min_instrumented: int
    # Instructions annotated `ir_only`: handlers of the register IR that
    # have no opcode, kept apart so that the opcode tables do not see them.
    ir_only: dict[str, Instruction]


def analysis_error(message: str, tkn: lexer.Token) -> SyntaxError:
    # To do -- support file and line output
    # Construct a SyntaxError instance from message and token
    return lexer.make_syntax_error(message, tkn.filename, tkn.line, tkn.column, "")


def override_error(
    name: str,
    context: parser.Context | None,
    prev_context: parser.Context | None,
    token: lexer.Token,
) -> SyntaxError:
    return analysis_error(
        f"Duplicate definition of '{name}' @ {context} "
        f"previous definition @ {prev_context}",
        token,
    )


def convert_stack_item(
    item: parser.StackEffect, replace_op_arg_1: str | None
) -> StackItem:
    return StackItem(item.name, item.size)

def check_unused(stack: list[StackItem], input_names: dict[str, lexer.Token]) -> None:
    "Unused items cannot be on the stack above used, non-peek items"
    seen_unused = False
    for item in reversed(stack):
        if item.name == "unused":
            seen_unused = True
        elif item.peek:
            break
        elif seen_unused:
            raise analysis_error(f"Cannot have used input '{item.name}' below an unused value on the stack", input_names[item.name])


def analyze_stack(
    op: parser.InstDef | parser.Pseudo, replace_op_arg_1: str | None = None
) -> StackEffect:
    inputs: list[StackItem] = [
        convert_stack_item(i, replace_op_arg_1)
        for i in op.inputs
        if isinstance(i, parser.StackEffect)
    ]
    outputs: list[StackItem] = [
        convert_stack_item(i, replace_op_arg_1) for i in op.outputs
    ]
    # Mark variables with matching names at the base of the stack as "peek"
    modified = False
    input_names: dict[str, lexer.Token] = { i.name : i.first_token for i in op.inputs if i.name != "unused" }
    for input, output in itertools.zip_longest(inputs, outputs):
        if output is None:
            pass
        elif input is None:
            if output.name in input_names:
                raise analysis_error(
                    f"Reuse of variable '{output.name}' at different stack location",
                    input_names[output.name])
        elif input.name == output.name:
            if not modified:
                input.peek = output.peek = True
        else:
            modified = True
            if output.name in input_names:
                raise analysis_error(
                    f"Reuse of variable '{output.name}' at different stack location",
                    input_names[output.name])
    if isinstance(op, parser.InstDef):
        output_names = [out.name for out in outputs]
        for input in inputs:
            if (
                variable_used(op, input.name)
                or (not input.peek and input.name in output_names)
            ):
                input.used = True
        for output in outputs:
            if variable_used(op, output.name):
                output.used = True
    check_unused(inputs, input_names)
    return StackEffect(inputs, outputs)


def analyze_caches(inputs: list[parser.InputEffect]) -> list[CacheEntry]:
    caches: list[parser.CacheEffect] = [
        i for i in inputs if isinstance(i, parser.CacheEffect)
    ]
    if caches:
        # Middle entries are allowed to be unused. Check first and last caches.
        for index in (0, -1):
            cache = caches[index]
            if cache.name == "unused":
                position = "First" if index == 0 else "Last"
                msg = f"{position} cache entry in op is unused. Move to enclosing macro."
                raise analysis_error(msg, cache.tokens[0])
    return [CacheEntry(i.name, int(i.size)) for i in caches]


def find_variable_stores(node: parser.InstDef) -> list[lexer.Token]:
    res: list[lexer.Token] = []
    outnames = { out.name for out in node.outputs }
    innames = { out.name for out in node.inputs }

    def find_stores_in_tokens(tokens: list[lexer.Token], callback: Callable[[lexer.Token], None]) -> None:
        while tokens and tokens[0].kind == "COMMENT":
            tokens = tokens[1:]
        if len(tokens) < 4:
            return
        if tokens[1].kind == "EQUALS":
            if tokens[0].kind == "IDENTIFIER":
                name = tokens[0].text
                if name in outnames or name in innames:
                    callback(tokens[0])
        #Passing the address of a local is also a definition
        for idx, tkn in enumerate(tokens):
            if tkn.kind == "AND":
                name_tkn = tokens[idx+1]
                if name_tkn.text in outnames:
                    callback(name_tkn)

    def visit(stmt: Stmt) -> None:
        if isinstance(stmt, IfStmt):
            def error(tkn: lexer.Token) -> None:
                raise analysis_error("Cannot define variable in 'if' condition", tkn)
            find_stores_in_tokens(stmt.condition, error)
        elif isinstance(stmt, SimpleStmt):
            find_stores_in_tokens(stmt.contents, res.append)

    node.block.accept(visit)
    return res


def variable_used(node: parser.CodeDef, name: str) -> bool:
    """Determine whether a variable with a given name is used in a node."""
    return any(
        token.kind == "IDENTIFIER" and token.text == name for token in node.block.tokens()
    )


def oparg_used(node: parser.CodeDef) -> bool:
    """Determine whether `oparg` is used in a node."""
    return any(
        token.kind == "IDENTIFIER" and token.text == "oparg" for token in node.tokens
    )


def tier_variable(node: parser.CodeDef) -> int | None:
    """Determine whether a tier variable is used in a node."""
    if isinstance(node, parser.LabelDef):
        return None
    for token in node.tokens:
        if token.kind == "ANNOTATION":
            if token.text == "specializing":
                return 1
            if re.fullmatch(r"tier\d", token.text):
                return int(token.text[-1])
    return None


def has_error_with_pop(op: parser.CodeDef) -> bool:
    return (
        variable_used(op, "ERROR_IF")
        or variable_used(op, "exception_unwind")
    )


def has_error_without_pop(op: parser.CodeDef) -> bool:
    return (
        variable_used(op, "ERROR_NO_POP")
        or variable_used(op, "exception_unwind")
    )


NON_ESCAPING_FUNCTIONS = (
    "PyCFunction_GET_FLAGS",
    "PyCFunction_GET_FUNCTION",
    "PyCFunction_GET_SELF",
    "PyCell_GetRef",
    "PyCell_New",
    "PyCell_GET_VALUE",
    "PyCell_SET_VALUE",
    "_PyCell_GetValue",
    "_PyCell_SwapValue",
    "PyExceptionInstance_Class",
    "PyException_GetCause",
    "PyException_GetContext",
    "PyException_GetTraceback",
    "PyFloat_AS_DOUBLE",
    "PyFloat_FromDouble",
    "PyFunction_GET_CODE",
    "PyFunction_GET_GLOBALS",
    "PyList_GET_ITEM",
    "PyList_GET_VALUE",
    "PyList_GET_SIZE",
    "PyList_SET_ITEM",
    "PyLong_AsLong",
    "PyLong_FromLong",
    "PyLong_FromSsize_t",
    "PySlice_New",
    "PyValue_ObjectBorrow",
    # Nothing is reference counted: a decref or a clear releases nothing
    # and runs no code.
    "Py_DECREF",
    "Py_XDECREF",
    "Py_CLEAR",
    "Py_SETREF",
    "Py_XSETREF",
    "_PyGC_CLEAR_ARRAY",
    # Frame bookkeeping, which runs no code
    "_PyFrame_Copy",
    "_PyThreadState_PopFrame",
    "_PyThreadState_UpdateLastProfiledFrame",
    "_PyThreadState_PushValueRoot",
    "_PyThreadState_PopValueRoot",
    "PyValue_FromObject",
    "_PyValue_False",
    "_PyValue_IsNone",
    "PyValue_Is",
    "_PyValue_IsTrue",
    "_PyValue_IsFalse",
    "PyValue_IsNull",
    "_PyValue_None",
    "PyValue_Type",
    "_PyValue_True",
    "PyBytes_GET_SIZE",
    "PyDict_GET_SIZE",
    "PySet_GET_SIZE",
    "PyTuple_GET_ITEM",
    "PyTuple_GET_VALUE",
    "PyTuple_GET_SIZE",
    "PyType_HasFeature",
    "PyUnicode_Concat",
    "PyUnicode_GET_LENGTH",
    "PyUnicode_READ_CHAR",
    "PyUnicode_IS_COMPACT_ASCII",
    "PyUnicode_1BYTE_DATA",
    "Py_ARRAY_LENGTH",
    "Py_FatalError",
    "Py_INCREF",
    "Py_IS_TYPE",
    "Py_NewRef",
    "Py_REFCNT",
    "Py_SIZE",
    "Py_TYPE",
    "Py_UNREACHABLE",
    "Py_Unicode_GET_LENGTH",
    "_PyCode_CODE",
    "_PyObject_SetShape",
    "_PyErr_Occurred",
    "_PyFb_Entry",
    "_PyFb_Header",
    "_PyFb_HasEntry",
    "_PyFrame_GetBytecode",
    "_PyFrame_GetCode",
    "_PyFrame_IsIncomplete",
    "_PyFrame_PushUnchecked",
    "_PyFrame_PushOverlapping",
    "_PyFrame_SetReturnValue",
    "_PyFrame_Slots",
    "_PyFrame_AssertTemporariesClear",
    "_PyEval_GeneratorReturnTarget",
    "_PyFunction_SetVersion",
    "_PyGen_GetGeneratorFromFrame",
    "gen_try_set_executing",
    "_PyInterpreterState_GET",
    "_PyList_AppendValue",
    "_PyList_ITEMS",
    "_PyLong_CompactValue",
    "_PyLong_DigitCount",
    "_PyLong_IsCompact",
    "_PyLong_IsNegative",
    "_PyLong_IsNonNegativeCompact",
    "_PyLong_IsZero",
    "_PyLong_BothAreCompact",
    "_PyObject_GC_IS_SHARED",
    "_PyObject_GC_IS_TRACKED",
    "_PyObject_GC_MAY_BE_TRACKED",
    "_PyObject_GC_TRACK",
    "_PyObject_IsUniquelyReferenced",
    "_PyObject_SHAPE",
    "_PyObject_DictAt",
    "_PyFrame_FbPoly",
    "_PyFb_ShapeRow",
    "_PyThreadState_HasStackSpace",
    "_PyTuple_FromValues",
    "_PyTuple_ITEMS",
    "_PyType_HasFeature",
    "_PyUnicode_Equal",
    "_PyUnicode_JoinArray",
    "_Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY",
    "_Py_ID",
    "_Py_IsImmortal",
    "_Py_IsOwnedByCurrentThread",
    "_Py_LeaveRecursiveCallPy",
    "_Py_LeaveRecursiveCallTstate",
    "_Py_NewRef",
    "_Py_SINGLETON",
    "_Py_STR",
    "_Py_TryIncrefCompare",
    "_Py_atomic_compare_exchange_uint8",
    "_Py_atomic_load_ptr_acquire",
    "_Py_atomic_load_uintptr_relaxed",
    "_Py_set_eval_breaker_bit",
    "advance_backoff_counter",
    "assert",
    "backoff_counter_triggers",
    "JUMP_TO_LABEL",
    "restart_backoff_counter",
    "_Py_ReachedRecursionLimit",
    "_PyValue_IsTaggedIndex",
    "_PyValue_TagIndex",
    "_PyValue_UntagIndex",
    "_PyValue_IncrementTaggedIndexNoOverflow",
    "PyValue_IsObject",
    "_PyValue_IsPointerSpace",
    "_PyValue_IsPointerSpaceAt",
    "_PyValue_IsInlineAt",
    "PyValue_IsInline",
    "PyValue_IsInlineInt",
    "PyValue_IsInlineFloat",
    "_PyValue_FitsKind",
    "_PyValue_BothInlineInt",
    "PyValue_FromInt32",
    "_PyValue_FromBits",
    "_PyValue_DoubleBits",
    "PyValue_Int32",
    "_PyValue_IsCompactInt",
    "_PyValue_CompactInt",
    "_PyValue_BoxInt64",
    "PyValue_Double",
    "_PyValue_FloatCheckExact",
    "_PyValue_FloatOperandsCheck",
    "_PyValue_NumberAsDouble",
    "_PyValue_FloatOperandValues",
    "isfinite",
    "pow",
    # Allocate an int or float; see PyValue_Object below.
    "PyValue_FromDouble",
    "_PyValue_LshiftInt32",
    # Boxing allocates an int or float, which runs no Python code and
    # cannot start a collection.
    "PyValue_Object",
    "_PyValue_BoxInline",
    "_PyValue_ObjectRooted",
    "_PyValue_UnrootObject",
    "_PyValue_IsError",
    "_PyValue_IsValid",
    "_PyValue_Wrap",
    "_PyValue_Unwrap",
    "_PyLong_CheckExactAndCompact",
    "_Py_unset_eval_breaker_bit",
    "_Py_set_eval_breaker_bit",
    "doesnt_escape",
    "_Py_GatherStats_GetIter",
    "_PyStolenTuple_Free",
    "PyObject_GC_UnTrack",
    "_PyErr_ExceptionMatches",
)


def check_escaping_calls(instr: parser.CodeDef, escapes: dict[SimpleStmt, EscapingCall]) -> None:
    error: lexer.Token | None = None
    calls = {e.call for e in escapes.values()}

    def visit(stmt: Stmt) -> None:
        nonlocal error
        if isinstance(stmt, IfStmt) or isinstance(stmt, WhileStmt):
            for tkn in stmt.condition:
                if tkn in calls:
                    error = tkn
        elif isinstance(stmt, SimpleStmt):
            in_if = 0
            tkn_iter = iter(stmt.contents)
            for tkn in tkn_iter:
                if tkn.kind == "IDENTIFIER" and tkn.text in ("DEOPT_IF", "ERROR_IF", "EXIT_IF"):
                    in_if = 1
                    next(tkn_iter)
                elif tkn.kind == "LPAREN":
                    if in_if:
                        in_if += 1
                elif tkn.kind == "RPAREN":
                    if in_if:
                        in_if -= 1
                elif tkn in calls and in_if:
                    error = tkn


    instr.block.accept(visit)
    if error is not None:
        raise analysis_error(f"Escaping call '{error.text} in condition", error)

def escaping_call_in_simple_stmt(stmt: SimpleStmt, result: dict[SimpleStmt, EscapingCall]) -> None:
    tokens = stmt.contents
    for idx, tkn in enumerate(tokens):
        try:
            next_tkn = tokens[idx+1]
        except IndexError:
            break
        if next_tkn.kind != lexer.LPAREN:
            continue
        if tkn.kind == lexer.IDENTIFIER:
            if tkn.text.upper() == tkn.text:
                # simple macro
                continue
            #if not tkn.text.startswith(("Py", "_Py", "monitor")):
            #    continue
            if tkn.text.endswith("Check"):
                continue
            if tkn.text.startswith("Py_Is"):
                continue
            if tkn.text.endswith("CheckExact"):
                continue
            if tkn.text in NON_ESCAPING_FUNCTIONS:
                continue
        elif tkn.kind == "RPAREN":
            prev = tokens[idx-1]
            if prev.text.endswith("_t") or prev.text == "*" or prev.text == "int":
                #cast
                continue
        elif tkn.kind != "RBRACKET":
            continue
        result[stmt] = EscapingCall(stmt, tkn)


def find_escaping_api_calls(instr: parser.CodeDef) -> dict[SimpleStmt, EscapingCall]:
    result: dict[SimpleStmt, EscapingCall] = {}

    def visit(stmt: Stmt) -> None:
        if not isinstance(stmt, SimpleStmt):
            return
        escaping_call_in_simple_stmt(stmt, result)

    instr.block.accept(visit)
    check_escaping_calls(instr, result)
    return result


EXITS = {
    "DISPATCH",
    "Py_UNREACHABLE",
    "DISPATCH_INLINED",
    "DISPATCH_GOTO",
}


def always_exits(op: parser.CodeDef) -> bool:
    depth = 0
    tkn_iter = iter(op.tokens)
    for tkn in tkn_iter:
        if tkn.kind == "LBRACE":
            depth += 1
        elif tkn.kind == "RBRACE":
            depth -= 1
        elif depth > 1:
            continue
        elif tkn.kind == "GOTO" or tkn.kind == "RETURN":
            return True
        elif tkn.kind == "KEYWORD":
            if tkn.text in EXITS:
                return True
        elif tkn.kind == "IDENTIFIER":
            if tkn.text in EXITS:
                return True
            if tkn.text == "DEOPT_IF" or tkn.text == "ERROR_IF":
                next(tkn_iter)  # '('
                t = next(tkn_iter)
                if t.text in ("true", "1"):
                    return True
    return False


def stack_effect_only_peeks(instr: parser.InstDef) -> bool:
    stack_inputs = [s for s in instr.inputs if not isinstance(s, parser.CacheEffect)]
    if len(stack_inputs) != len(instr.outputs):
        return False
    if len(stack_inputs) == 0:
        return False
    return all(
        (s.name == other.name and s.size == other.size)
        for s, other in zip(stack_inputs, instr.outputs)
    )


def stmt_is_simple_exit(stmt: Stmt) -> bool:
    if not isinstance(stmt, SimpleStmt):
        return False
    tokens = stmt.contents
    if len(tokens) < 4:
        return False
    return (
        tokens[0].text in ("ERROR_IF", "DEOPT_IF", "EXIT_IF")
        and
        tokens[1].text == "("
        and
        tokens[2].text in ("true", "1")
        and
        tokens[3].text == ")"
    )


def stmt_list_escapes(stmts: list[Stmt]) -> bool:
    if not stmts:
        return False
    if stmt_is_simple_exit(stmts[-1]):
        return False
    for stmt in stmts:
        if stmt_escapes(stmt):
            return True
    return False


def stmt_escapes(stmt: Stmt) -> bool:
    if isinstance(stmt, BlockStmt):
        return stmt_list_escapes(stmt.body)
    elif isinstance(stmt, SimpleStmt):
        d: dict[SimpleStmt, EscapingCall] = {}
        escaping_call_in_simple_stmt(stmt, d)
        return bool(d)
    elif isinstance(stmt, IfStmt):
        if stmt.else_body and stmt_escapes(stmt.else_body):
            return True
        return stmt_escapes(stmt.body)
    elif isinstance(stmt, MacroIfStmt):
        if stmt.else_body and stmt_list_escapes(stmt.else_body):
            return True
        return stmt_list_escapes(stmt.body)
    elif isinstance(stmt, ForStmt):
        return stmt_escapes(stmt.body)
    elif isinstance(stmt, WhileStmt):
        return stmt_escapes(stmt.body)
    else:
        assert False, "Unexpected statement type"

def find_aliased_outputs(op: parser.InstDef) -> tuple[str, ...]:
    """The outputs that a statement `out = in;` makes alias an input."""
    innames = {i.name for i in op.inputs if isinstance(i, parser.StackEffect)}
    outnames = {o.name for o in op.outputs} - innames
    tokens = [t for t in op.block.tokens() if t.kind != "COMMENT"]
    found: list[str] = []
    for i in range(len(tokens) - 3):
        if (tokens[i].text in outnames and tokens[i + 1].kind == "EQUALS"
                and tokens[i + 2].text in innames and tokens[i + 3].kind == "SEMI"
                and (i == 0 or tokens[i - 1].kind in ("SEMI", "LBRACE", "RBRACE"))
                and tokens[i].text not in found):
            found.append(tokens[i].text)
    return tuple(found)


def switches_frame(op: parser.CodeDef) -> bool:
    """Whether op assigns `frame`, directly or through DISPATCH_INLINED."""
    tokens = [t for t in op.tokens if t.kind != "COMMENT"]
    for i, tkn in enumerate(tokens):
        if tkn.kind != "IDENTIFIER":
            continue
        if tkn.text == "DISPATCH_INLINED":
            return True
        if (tkn.text == "frame" and i + 1 < len(tokens)
                and tokens[i + 1].kind == "EQUALS"
                and tokens[i - 1].kind in ("SEMI", "LBRACE", "RBRACE", "EQUALS")):
            return True
    return False


def compute_properties(op: parser.CodeDef) -> Properties:
    escaping_calls = find_escaping_api_calls(op)
    has_free = (
        variable_used(op, "PyCell_New")
        or variable_used(op, "PyCell_GetRef")
        or variable_used(op, "PyCell_SET_VALUE")
        or variable_used(op, "_PyCell_SwapValue")
        or variable_used(op, "_PyCell_GetValue")
    )
    deopts_if = variable_used(op, "DEOPT_IF")
    exits_if = variable_used(op, "EXIT_IF")
    if deopts_if and exits_if:
        tkn = op.tokens[0]
        raise lexer.make_syntax_error(
            "Op cannot contain both EXIT_IF and DEOPT_IF",
            tkn.filename,
            tkn.line,
            tkn.column,
            op.name,
        )
    error_with_pop = has_error_with_pop(op)
    error_without_pop = has_error_without_pop(op)
    escapes = stmt_escapes(op.block)
    pure = False if isinstance(op, parser.LabelDef) else "pure" in op.annotations
    no_save_ip = False if isinstance(op, parser.LabelDef) else "no_save_ip" in op.annotations
    inline_ok = False if isinstance(op, parser.LabelDef) else "inline_ok" in op.annotations
    window = False if isinstance(op, parser.LabelDef) else "window" in op.annotations
    returns = False if isinstance(op, parser.LabelDef) else "returns" in op.annotations
    stores_early = (not isinstance(op, parser.LabelDef)
                    and "stores_early" in op.annotations)
    if stores_early and not variable_used(op, "IR_RETURN_EARLY"):
        raise analysis_error(
            f"{op.name} is `stores_early` but its body has no IR_RETURN_EARLY()",
            op.tokens[0])
    return Properties(
        escaping_calls=escaping_calls,
        escapes=escapes,
        error_with_pop=error_with_pop,
        error_without_pop=error_without_pop,
        deopts=deopts_if,
        side_exit=exits_if,
        oparg=oparg_used(op),
        jumps=variable_used(op, "JUMPBY") or variable_used(op, "JUMPTO"),
        eval_breaker="CHECK_PERIODIC" in op.name,
        needs_this=(variable_used(op, "this_instr")
                    or variable_used(op, "FB_COUNTER")
                    or variable_used(op, "FB_BRANCH")),
        always_exits=always_exits(op),
        sync_sp=variable_used(op, "SYNC_SP") or variable_used(op, "SAVE_STACK") or variable_used(op, "RELOAD_STACK"),
        uses_co_consts=variable_used(op, "FRAME_CO_CONSTS"),
        uses_co_names=variable_used(op, "FRAME_CO_NAMES"),
        uses_locals=variable_used(op, "GETLOCAL") and not has_free,
        uses_opcode=variable_used(op, "opcode"),
        has_free=has_free,
        pure=pure,
        no_save_ip=no_save_ip,
        inline_ok=inline_ok,
        tier=tier_variable(op),
        needs_prev=variable_used(op, "prev_instr"),
        window=window,
        switches_frame=switches_frame(op),
        returns=returns,
        stores_early=stores_early,
    )

VALUE_TO_OBJECT = (
    "PyValue_ObjectBorrow",
)


def find_object_inputs(
    op: parser.InstDef, stack: StackEffect
) -> tuple[tuple[str, ...], tuple[str, ...]]:
    """Find the scalar stack inputs that op converts to PyObject *, and those
    that it asserts to be objects.

    Only a conversion whose whole argument is the input's name counts.
    Conversions inside assert() are ignored, since they vanish from release
    builds. Arrays are left to the C helpers that convert their elements.

    An input that op asserts to be an object, with
    `assert(PyValue_IsObject(name))`, can never hold an inline value by
    construction of the bytecode, so it needs no test.
    """
    scalars = {item.name for item in stack.inputs if not item.is_array()}
    found: list[str] = []
    asserted: list[str] = []
    tokens = list(op.block.tokens())
    i = 0
    while i < len(tokens):
        tkn = tokens[i]
        if tkn.text == "assert":
            start = i
            depth = 0
            i += 1
            while i < len(tokens):
                if tokens[i].kind == lexer.LPAREN:
                    depth += 1
                elif tokens[i].kind == lexer.RPAREN:
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            texts = [t.text for t in tokens[start:i + 1]]
            if (len(texts) == 7 and texts[2] == "PyValue_IsObject"
                    and texts[4] in scalars):
                asserted.append(texts[4])
        elif tkn.text in VALUE_TO_OBJECT and i + 3 < len(tokens):
            arg, close = tokens[i + 2], tokens[i + 3]
            if arg.text in scalars and close.kind == lexer.RPAREN and arg.text not in found:
                found.append(arg.text)
        i += 1
    return (
        tuple(name for name in found if name not in asserted),
        tuple(asserted),
    )


def make_uop(
    name: str,
    op: parser.InstDef,
    inputs: list[parser.InputEffect],
) -> Uop:
    stack = analyze_stack(op)
    object_inputs, asserted_objects = find_object_inputs(op, stack)
    return Uop(
        name=name,
        context=op.context,
        annotations=op.annotations,
        stack=stack,
        caches=analyze_caches(inputs),
        local_stores=find_variable_stores(op),
        body=op.block,
        properties=compute_properties(op),
        object_inputs=object_inputs,
        asserted_objects=asserted_objects,
        aliased_outputs=find_aliased_outputs(op),
    )


def add_op(op: parser.InstDef, uops: dict[str, Uop]) -> None:
    assert op.kind == "op"
    if op.name in uops:
        if "override" not in op.annotations:
            raise override_error(
                op.name, op.context, uops[op.name].context, op.tokens[0]
            )
    uops[op.name] = make_uop(op.name, op, op.inputs)


def add_instruction(
    where: lexer.Token,
    name: str,
    parts: list[Part],
    instructions: dict[str, Instruction],
) -> None:
    instructions[name] = Instruction(where, name, parts, None)


def desugar_inst(
    inst: parser.InstDef, instructions: dict[str, Instruction], uops: dict[str, Uop]
) -> None:
    assert inst.kind == "inst"
    name = inst.name
    op_inputs: list[parser.InputEffect] = []
    parts: list[Part] = []
    uop_index = -1
    # Move unused cache entries to the Instruction, removing them from the Uop.
    for input in inst.inputs:
        if isinstance(input, parser.CacheEffect) and input.name == "unused":
            parts.append(Skip(input.size))
        else:
            op_inputs.append(input)
            if uop_index < 0:
                uop_index = len(parts)
                # Place holder for the uop.
                parts.append(Skip(0))
    uop = make_uop("_" + inst.name, inst, op_inputs)
    uop.implicitly_created = True
    uops[inst.name] = uop
    if uop_index < 0:
        parts.append(uop)
    else:
        parts[uop_index] = uop
    add_instruction(inst.first_token, name, parts, instructions)


def add_macro(
    macro: parser.Macro, instructions: dict[str, Instruction], uops: dict[str, Uop]
) -> None:
    parts: list[Part] = []
    for part in macro.uops:
        match part:
            case parser.OpName():
                if part.name == "flush":
                    parts.append(Flush())
                else:
                    if part.name not in uops:
                        raise analysis_error(
                            f"No Uop named {part.name}", macro.tokens[0]
                        )
                    parts.append(uops[part.name])
            case parser.CacheEffect():
                parts.append(Skip(part.size))
            case _:
                assert False
    assert parts
    add_instruction(macro.first_token, macro.name, parts, instructions)


def add_family(
    pfamily: parser.Family,
    instructions: dict[str, Instruction],
    families: dict[str, Family],
) -> None:
    family = Family(
        pfamily.name,
        pfamily.size,
        [instructions[member_name] for member_name in pfamily.members],
    )
    for member in family.members:
        member.family = family
    # The head of the family is an implicit jump target for DEOPTs
    instructions[family.name].is_target = True
    families[family.name] = family


def add_pseudo(
    pseudo: parser.Pseudo,
    instructions: dict[str, Instruction],
    pseudos: dict[str, PseudoInstruction],
) -> None:
    pseudos[pseudo.name] = PseudoInstruction(
        pseudo.name,
        analyze_stack(pseudo),
        [instructions[target] for target in pseudo.targets],
        pseudo.as_sequence,
        pseudo.flags,
    )


def add_label(
    label: parser.LabelDef,
    labels: dict[str, Label],
) -> None:
    properties = compute_properties(label)
    labels[label.name] = Label(label.name, label.spilled, label.block, properties)


def assign_opcodes(
    instructions: dict[str, Instruction],
    families: dict[str, Family],
    pseudos: dict[str, PseudoInstruction],
) -> tuple[dict[str, int], int, int]:
    """Assigns opcodes, then returns the opmap,
    have_arg and min_instrumented values"""
    instmap: dict[str, int] = {}

    # 0 is reserved for cache entries. This helps debugging.
    instmap["CACHE"] = 0

    # 17 is reserved as it is the initial value for the specializing counter.
    # This helps catch cases where we attempt to execute a cache.
    instmap["RESERVED"] = 17

    # 128 is RESUME - it is hard coded as such in Tools/build/deepfreeze.py
    instmap["RESUME"] = 128

    # This is an historical oddity.
    instmap["BINARY_OP_INPLACE_ADD_UNICODE"] = 3

    instmap["INSTRUMENTED_LINE"] = 255

    instrumented = [name for name in instructions if name.startswith("INSTRUMENTED")]

    specialized: set[str] = set()
    no_arg: list[str] = []
    has_arg: list[str] = []

    for family in families.values():
        specialized.update(inst.name for inst in family.members)

    for inst in instructions.values():
        name = inst.name
        if name in specialized:
            continue
        if name in instrumented:
            continue
        if inst.properties.oparg:
            has_arg.append(name)
        else:
            no_arg.append(name)

    # Specialized ops appear in their own section, and those that do not fit
    # take the numbers below RESUME that no other instruction uses.
    # Instrumented opcodes are at the end of the valid range
    min_internal = instmap["RESUME"] + 1
    min_instrumented = 256 - len(instrumented)

    next_opcode = 1

    def add_instruction(name: str) -> None:
        nonlocal next_opcode
        if name in instmap:
            return  # Pre-defined name
        while next_opcode in instmap.values():
            next_opcode += 1
        instmap[name] = next_opcode
        next_opcode += 1

    for name in sorted(no_arg):
        add_instruction(name)
    for name in sorted(has_arg):
        add_instruction(name)
    # For compatibility
    next_opcode = min_internal
    for name in sorted(specialized):
        if next_opcode == min_instrumented:
            next_opcode = 1
        add_instruction(name)
        assert instmap[name] < min_instrumented
    next_opcode = min_instrumented
    for name in instrumented:
        add_instruction(name)

    for name in instructions:
        instructions[name].opcode = instmap[name]

    for op, name in enumerate(sorted(pseudos), 256):
        instmap[name] = op
        pseudos[name].opcode = op

    return instmap, len(no_arg), min_instrumented


def get_instruction_size_for_uop(instructions: dict[str, Instruction], uop: Uop) -> int | None:
    """Return the size of the instruction that contains the given uop or
    `None` if the uop does not contains the `INSTRUCTION_SIZE` macro.

    If there is more than one instruction that contains the uop,
    ensure that they all have the same size.
    """
    for tkn in uop.body.tokens():
        if tkn.text == "INSTRUCTION_SIZE":
            break
    else:
        return None

    size = None
    for inst in instructions.values():
        if uop in inst.parts:
            if size is None:
                size = inst.size
            if size != inst.size:
                raise analysis_error(
                    "All instructions containing a uop with the `INSTRUCTION_SIZE` macro "
                    f"must have the same size: {size} != {inst.size}",
                    tkn
                )
    if size is None:
        raise analysis_error(f"No instruction containing the uop '{uop.name}' was found", tkn)
    return size


def analyze_forest(forest: list[parser.AstNode]) -> Analysis:
    instructions: dict[str, Instruction] = {}
    uops: dict[str, Uop] = {}
    families: dict[str, Family] = {}
    pseudos: dict[str, PseudoInstruction] = {}
    labels: dict[str, Label] = {}
    for node in forest:
        match node:
            case parser.InstDef(name):
                if node.kind == "inst":
                    desugar_inst(node, instructions, uops)
                else:
                    assert node.kind == "op"
                    add_op(node, uops)
            case parser.Macro():
                pass
            case parser.Family():
                pass
            case parser.Pseudo():
                pass
            case parser.LabelDef():
                pass
            case _:
                assert False
    for node in forest:
        if isinstance(node, parser.Macro):
            add_macro(node, instructions, uops)
    for node in forest:
        match node:
            case parser.Family():
                add_family(node, instructions, families)
            case parser.Pseudo():
                add_pseudo(node, instructions, pseudos)
            case parser.LabelDef():
                add_label(node, labels)
            case _:
                pass
    for code in [*uops.values(), *labels.values()]:
        for tkn, msg in root_check.check_uop(code):
            raise analysis_error(msg, tkn)
    ir_only: dict[str, Instruction] = {}
    for name, inst in list(instructions.items()):
        if any(isinstance(part, Uop) and "ir_only" in part.annotations
               for part in inst.parts):
            ir_only[name] = instructions.pop(name)
            for part in inst.parts:
                if isinstance(part, Uop) and "ir_only" in part.annotations:
                    uops.pop(part.name if part.name in uops else name, None)
    for uop in uops.values():
        uop.instruction_size = get_instruction_size_for_uop(instructions, uop)
    # Special case BINARY_OP_INPLACE_ADD_UNICODE
    # BINARY_OP_INPLACE_ADD_UNICODE is not a normal family member,
    # as it is the wrong size, but we need it to maintain an
    # historical optimization.
    if "BINARY_OP_INPLACE_ADD_UNICODE" in instructions:
        inst = instructions["BINARY_OP_INPLACE_ADD_UNICODE"]
        inst.family = families["BINARY_OP"]
        families["BINARY_OP"].members.append(inst)
    opmap, first_arg, min_instrumented = assign_opcodes(instructions, families, pseudos)
    return Analysis(
        instructions, uops, families, pseudos, labels, opmap, first_arg,
        min_instrumented, ir_only
    )


def analyze_files(filenames: list[str]) -> Analysis:
    return analyze_forest(parser.parse_files(filenames))


def dump_analysis(analysis: Analysis) -> None:
    print("Uops:")
    for u in analysis.uops.values():
        u.dump("    ")
    print("Instructions:")
    for i in analysis.instructions.values():
        i.dump("    ")
    print("Families:")
    for f in analysis.families.values():
        f.dump("    ")
    print("Pseudos:")
    for p in analysis.pseudos.values():
        p.dump("    ")


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("No input")
    else:
        filenames = sys.argv[1:]
        dump_analysis(analyze_files(filenames))
