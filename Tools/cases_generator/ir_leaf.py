"""Leaf uops: the value primitives, the leaf subset and the leaf IR.

A leaf uop's body uses only the closed set of value primitives listed here,
checked integer arithmetic, compares, guard and error exits and calls of
declared helpers, so a compiler can translate it mechanically (tiers87.md,
C8).  This module is the one place that lists what a leaf may use:

- PRIMITIVES: each value primitive, its C definition, its signature over the
  subset's types, its effects and its semantics as a Python function over
  64-bit words and memory (the spec).  The spec is executable: the reference
  evaluator (ir_leaf_eval.py) runs it, and Lib/test/test_ir_leaf.py checks
  it against the C definition through _testinternalcapi.leaf_primitive();
- FIELDS: the struct fields and elements a leaf may load or store;
- HELPERS: the out-of-line functions a leaf may call, with their effects.

The subset checker, the translator to leaf IR and its dump follow the lists."""

from dataclasses import dataclass, field
from rop_primitives import (
    Block,
    C_TYPES,
    EFFECTS,
    FIELDS,
    HELPERS,
    LeafBody,
    MAY_RAISE,
    Op,
    POINTER_TARGETS,
    PRIMITIVES,
    Primitive,
    READS_HEAP,
    STRUCTS,
    SYMBOLS,
    VALUE,
    VALUE_CONSTANTS,
    WRITES_HEAP,
    bits_of,
    is_pointer,
    is_signed,
    wrap,
)




# The facts an `assume` op may state: those with a C assertion
# (ir_contract.ASSERTION_FORMS), each of which the templates check
ASSUMABLE = ("nonnull", "object", "inline", "inline_int", "inline_float",
             "tagged_index", "exact", "encoding")





# The scalar types of the subset, and the C types that name them
SCALARS = ("bool", "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f64")






CONSTANTS = (
    "Py_PTR_LIMIT", "Py_INDEX_BASE", "Py_DOUBLE_OFFSET", "Py_INT_SHIFT",
    "Py_INT_BASE", "Py_INT_MIN", "Py_INT_MAX", "Py_OWNED_MASK",
    "Py_TAG_REFCNT", "_PY_EVAL_EVENTS_MASK",
)
# Element sizes that pointer arithmetic needs
SIZES = ("PyValue", "PyDictUnicodeEntry")

def intrinsic(name: str):
    """The INTRINSIC row of a form that has no operation of its own in the
    build that the generator generates (the GIL build, outside Emscripten),
    or the MACRO row of a form that moves its argument (TAKE): by result,
    SCALAR or VIEW is the identity on the last argument, moved(k) is
    argument k, LITERAL(k) the constant, NONE nothing"""
    import ir_contract
    row = ir_contract.rows().get(name)
    if row is None:
        return None
    if row.kind == "INTRINSIC" and row.builds & {ir_contract.BUILD, "all"}:
        return row
    if (row.kind == "MACRO" and row.takes_view and row.result.moved >= 0
            and row.result.shape == "VALUE"):
        # A value transport (TAKE); OWNER may be NULL whatever its argument is
        return row
    return None


def erased(name: str) -> bool:
    "Whether a statement of this name compiles to nothing in the leaf IR"
    import ir_contract
    row = intrinsic(name)
    if row is not None:
        return row.result.shape == "NONE"
    return name in ir_contract.ERASED_CONSTRUCTS
COMPLETIONS = {"_PY_IR_FOR_ITER": "_PyEval_CompleteForIter"}


# Op kinds.  Operands: arg (a uop input, by name), cache (a feedback field),
# oparg, imm (a word immediate), const (an integer, a double, a word
# constant `sym+tag`, or an address `&sym`).  Computation: prim (a listed
# primitive), call (a declared helper), load and store (of a field, an
# element or through a pointer; attr names the field or element type),
# ptradd and ptrdiff (scaled by an element type), arithmetic (add sub mul
# sdiv srem udiv urem and or xor shl ashr lshr neg not lnot fadd fsub fmul
# fdiv fneg), checked arithmetic (add_ovf sub_ovf mul_ovf, two results),
# conversions (sext zext trunc sitofp uitofp fptosi tobool), compares (cmp
# with attr eq ne slt sle sgt sge ult ule ugt uge; fcmp with oeq une olt ole
# ogt oge, une true on unordered as C's != is), select.  Exits: exit_if (attr
# EXIT or DEOPT; the guard fails if its operand is true), raise_if,
# report_edge.  Outputs: out (attr: the output's name).
TERMINATORS = ("br", "jmp", "next", "raise", "exit", "complete")


class LeafError(Exception):
    "A uop body outside the leaf subset"

    def __init__(self, message: str, token=None) -> None:
        super().__init__(message)
        self.token = token

    def __str__(self) -> str:
        where = f" (line {self.token.line})" if self.token is not None else ""
        return f"{self.args[0]}{where}"





@dataclass
class Expr:
    kind: str           # name num str call unary cast binary cond member index
    token: object
    value: object = None
    args: tuple = ()


_BINARY_PRECEDENCE = {
    "||": 1, "&&": 2, "|": 3, "^": 4, "&": 5, "==": 6, "!=": 6,
    "<": 7, "<=": 7, ">": 7, ">=": 7, "<<": 8, ">>": 8,
    "+": 9, "-": 9, "*": 10, "/": 10, "%": 10,
}
_TYPE_WORDS = {"INT", "LONG", "DOUBLE", "CHAR", "UNSIGNED", "SIGNED", "SHORT",
               "VOID", "STRUCT", "CONST", "FLOAT"}


def _type_start(tokens, i) -> bool:
    if i >= len(tokens):
        return False
    t = tokens[i]
    if t.kind in _TYPE_WORDS:
        return True
    return t.kind == "IDENTIFIER" and (t.text in C_TYPES or t.text in STRUCTS or
                                       t.text in OTHER_TYPES)


# Types that bodies use outside the subset: named so that a declaration of
# one is refused as such, not as a syntax error
OTHER_TYPES = {"_PyInterpreterFrame", "PyThreadState", "destructor", "_PyStatusValue",
               "_Py_iternextvaluefunc", "_PyValueIndexPair", "PyInterpreterState",
               "PyGenObject", "PyCFunctionObject", "PyMethodDescrObject",
               "PyHeapTypeObject", "PyCellObject", "PySliceObject", "PyFrameObject",
               "_PyIRTransfer", "_PyFbAttr", "PyUnicodeObject", "vectorcallfunc",
               "PyCFunction", "PyCFunctionFast", "PyCFunctionFastWithKeywords",
               "_PyObjectPair", "PyBoundMethodObject", "PyByteArrayObject",
               "PyBytesObject", "PySetObject", "PyFloatObject"}


class _Parser:
    def __init__(self, tokens) -> None:
        self.tokens = [t for t in tokens if t.kind != "COMMENT"]
        self.i = 0

    def peek(self, k: int = 0):
        j = self.i + k
        return self.tokens[j] if j < len(self.tokens) else None

    def next(self):
        t = self.peek()
        if t is None:
            raise LeafError("unexpected end of expression",
                            self.tokens[-1] if self.tokens else None)
        self.i += 1
        return t

    def expect(self, text: str):
        t = self.next()
        if t.text != text:
            raise LeafError(f"expected '{text}', found '{t.text}'", t)
        return t

    def at(self, text: str) -> bool:
        t = self.peek()
        return t is not None and t.text == text

    def done(self) -> bool:
        return self.i >= len(self.tokens)

    def type_name(self) -> str:
        "A C type name: base words, then stars"
        words = []
        start = self.peek()
        while _type_start(self.tokens, self.i) or (words and self.peek() is not None and
                                                    self.peek().kind in _TYPE_WORDS):
            t = self.next()
            if t.kind == "CONST":
                continue
            words.append(t.text)
        if not words:
            raise LeafError("expected a type", start)
        base = " ".join(words)
        stars = 0
        while self.at("*"):
            self.next()
            stars += 1
        return base + "*" * stars

    def expression(self) -> Expr:
        cond = self.binary(1)
        if self.at("?"):
            t = self.next()
            a = self.expression()
            self.expect(":")
            b = self.expression()
            return Expr("cond", t, None, (cond, a, b))
        return cond

    def binary(self, level: int) -> Expr:
        left = self.unary()
        while True:
            t = self.peek()
            if t is None or _BINARY_PRECEDENCE.get(t.text, 0) < level:
                return left
            prec = _BINARY_PRECEDENCE[t.text]
            self.next()
            right = self.binary(prec + 1)
            left = Expr("binary", t, t.text, (left, right))

    def unary(self) -> Expr:
        t = self.peek()
        if t is None:
            raise LeafError("unexpected end of expression")
        if t.text in ("!", "~", "-", "+", "*", "&"):
            self.next()
            return Expr("unary", t, t.text, (self.unary(),))
        if t.text == "(" and _type_start(self.tokens, self.i + 1):
            self.next()
            ctype = self.type_name()
            self.expect(")")
            return Expr("cast", t, ctype, (self.unary(),))
        return self.postfix(self.primary())

    def primary(self) -> Expr:
        t = self.next()
        if t.text == "(":
            e = self.expression()
            self.expect(")")
            return e
        if t.kind == "NUMBER":
            return Expr("num", t, t.text)
        if t.kind == "STRING":
            return Expr("str", t, t.text)
        if t.kind == "IDENTIFIER":
            return Expr("name", t, t.text)
        raise LeafError(f"'{t.text}' is outside the leaf subset", t)

    def postfix(self, e: Expr) -> Expr:
        while True:
            t = self.peek()
            if t is None:
                return e
            if t.text == "(" and e.kind == "name":
                self.next()
                args = []
                if not self.at(")"):
                    while True:
                        if e.value == "Py_ARITHMETIC_RIGHT_SHIFT" and not args:
                            args.append(Expr("type", self.peek(), self.type_name()))
                        else:
                            args.append(self.expression())
                        if self.at(","):
                            self.next()
                            continue
                        break
                self.expect(")")
                e = Expr("call", e.token, e.value, tuple(args))
            elif t.text in ("->", "."):
                self.next()
                name = self.next()
                e = Expr("member", t, (name.text, t.text == "->"), (e,))
            elif t.text == "[":
                self.next()
                index = self.expression()
                self.expect("]")
                e = Expr("index", t, None, (e, index))
            else:
                return e


def parse_expression(tokens) -> Expr:
    p = _Parser(tokens)
    e = p.expression()
    if not p.done():
        raise LeafError(f"unexpected '{p.peek().text}'", p.peek())
    return e





_BASE_TYPES = {
    "int": "i32", "signed": "i32", "signed int": "i32", "unsigned": "u32",
    "unsigned int": "u32", "long": "i64", "long int": "i64", "long long": "i64",
    "unsigned long": "u64", "unsigned long long": "u64", "short": "i16",
    "unsigned short": "u16", "char": "i8", "unsigned char": "u8", "double": "f64",
}


def c_type(name: str, token=None) -> str:
    "The subset type of C type name"
    stars = len(name) - len(name.rstrip("*"))
    base = name.rstrip("*").strip()
    if stars > 1:
        raise LeafError(f"type {name}: a pointer to a pointer", token)
    if stars == 1:
        if base in ("char", "void"):
            raise LeafError(f"type {name}: untyped memory", token)
        # No byte pointers: an object's value slots are _PyObject_ValueSlot()
        target = base
        if target not in POINTER_TARGETS + ("PyDictValues",):
            raise LeafError(f"type {name} is outside the leaf subset", token)
        return target + "*"
    t = _BASE_TYPES.get(base) or C_TYPES.get(base)
    if t is None:
        raise LeafError(f"type {name} is outside the leaf subset", token)
    return t


def ir_type(t: str) -> str:
    return "ptr" if is_pointer(t) else t


def _rank(t: str) -> int:
    return bits_of(t)


def _promote(t: str) -> str:
    if t in ("bool", "i8", "u8", "i16", "u16"):
        return "i32"
    return t


def common_type(a: str, b: str) -> str:
    "C's usual arithmetic conversions on two subset scalar types"
    if "f64" in (a, b):
        return "f64"
    a, b = _promote(a), _promote(b)
    if a == b:
        return a
    if is_signed(a) == is_signed(b):
        return a if _rank(a) >= _rank(b) else b
    s, u = (a, b) if is_signed(a) else (b, a)
    if _rank(u) >= _rank(s):
        return u
    return s       # i64 holds every u32


# Element types and their sizes' names in the layout
ELEMENT_SIZES = {"PyValue*": "PyValue", "PyDictUnicodeEntry*": "PyDictUnicodeEntry"}


def _element(pointer: str) -> str:
    "The subset type of what pointer points to"
    return VALUE if pointer == "PyValue*" else pointer[:-1]










def capability_expansion(name: str, args: list[str], pointer_test: str | None = None):
    if name == "IR_GUARD_INLINE_INT":
        return [("guard", f"!PyValue_IsInlineInt({args[0]})")]
    if name == "IR_GUARD_ELEMENT_TYPE":
        value, tp = args
        prefix = {"PyList_Type": "PyList", "PyTuple_Type": "PyTuple"}[tp]
        test = pointer_test or f"PyValue_IsObject({value})"
        items = [("gil_disabled_miss",)] if tp == "PyList_Type" else []
        items.append(("guard", f"!{test} || !{prefix}_CheckExact("
                               f"_PyValue_AsKnownObjectBorrowPointer({value}))"))
        return items
    if name == "IR_GUARD_ELEMENT_INDEX":
        container, subscript, index, mode = args
        prefix = "PyList" if mode == "FROM_END" else "PyTuple"
        size = f"{prefix}_GET_SIZE(_PyValue_AsKnownObjectBorrowPointer({container}))"
        items = [("stmt", f"Py_ssize_t {index} = PyValue_InlineInt64({subscript});")]
        if mode == "FROM_END":
            items.append(("stmt", f"if ({index} < 0) {{ {index} += {size}; }}"))
            items.append(("guard", f"{index} < 0 || {index} >= {size}"))
        else:
            items.append(("guard", f"{index} < 0"))
            items.append(("guard", f"{index} >= {size}"))
        return items
    if name == "IR_DUP_CANONICAL_ELEMENT":
        container, index, kind = args
        prefix = {"LIST": "PyList", "TUPLE": "PyTuple"}[kind]
        return [("expr", f"PyValue_DUP({prefix}_GET_VALUE("
                         f"_PyValue_AsKnownObjectBorrowPointer({container}), {index}))")]
    if name == "IR_GUARD_SHAPE_READ":
        pointer, version, shape = args
        return [("stmt", f"PyTypeObject *tp = Py_TYPE({pointer});"),
                ("stmt", f"assert({version} != 0);"),
                ("guard", f"FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != {version}"),
                ("guard", f"_PyObject_Shape({pointer}) != {shape}")]
    if name == "IR_DUP_SHAPE_READ":
        owner, offset = args
        return [("expr", f"_PyObject_ShapeReadDup(_PyValue_AsKnownObjectBorrowPointer({owner}), {offset})")]
    raise KeyError(name)


def element_kind(uop) -> str:
    """The kind of container that IR_DUP_CANONICAL_ELEMENT reads: the one
    that the uop's IR_GUARD_ELEMENT_INDEX checked its index for"""
    toks = [t.text for t in uop.body.tokens()]
    mode = toks[toks.index("IR_GUARD_ELEMENT_INDEX") + 8]
    return "LIST" if mode == "FROM_END" else "TUPLE"


CAPABILITY_MACROS = ("IR_GUARD_INLINE_INT", "IR_GUARD_ELEMENT_TYPE",
                     "IR_GUARD_ELEMENT_INDEX", "IR_DUP_CANONICAL_ELEMENT",
                     "IR_GUARD_SHAPE_READ", "IR_DUP_SHAPE_READ")

# Fields that are arrays in place: `p->f` is an address, not a load
ARRAY_FIELDS = {"PyTupleObject.ob_item", "PyDictKeysObject.dk_indices", "PyDictValues.values"}
# Cache field types that hold a word
# Names a leaf may not read: the evaluator's state
REFUSED_NAMES = {"tstate", "frame", "next_instr", "this_instr", "ir_word", "fb",
                 "opcode", "stack_pointer"}
UNDEF = -1

_ASSIGN_OPS = {"=": None, "+=": "+", "-=": "-", "*=": "*", "/=": "/", "%=": "%",
               "&=": "&", "|=": "|", "^=": "^", "<<=": "<<", ">>=": ">>"}
_ARITH = {"+": ("add", "fadd"), "-": ("sub", "fsub"), "*": ("mul", "fmul"),
          "&": ("and", None), "|": ("or", None), "^": ("xor", None),
          "<<": ("shl", None)}
_COMPARE = {"==": ("eq", "eq", "oeq"), "!=": ("ne", "ne", "une"),
            "<": ("slt", "ult", "olt"), "<=": ("sle", "ule", "ole"),
            ">": ("sgt", "ugt", "ogt"), ">=": ("sge", "uge", "oge")}


class _Loc:
    "A memory location: base pointer, the field read at it (or None) and its type"
    def __init__(self, base: int, field: str | None, type: str, home: bool = False) -> None:
        self.base, self.field, self.type, self.home = base, field, type, home


class Translator:
    """Translates a uop body in the leaf subset to leaf IR, and refuses
    anything outside it (LeafError).  The one walk is both the checker and
    the translator, so they cannot disagree."""

    def __init__(self, uop, cache_types: dict[str, str] | None = None) -> None:
        self.uop = uop
        self.cache_types = cache_types or {}
        self.types: dict[int, str] = {}
        self.blocks: list[Block] = []
        self.block: Block | None = self.new_block()
        self.env: dict[str, tuple[int, str]] = {}
        self.effects: set[str] = set()
        self.helpers: list[str] = []
        self.cached: dict[object, tuple[int, str]] = {}
        self.consts: dict[int, object] = {}

    # -- building

    def new_block(self) -> Block:
        b = Block(len(self.blocks))
        self.blocks.append(b)
        return b

    def value(self, t: str) -> int:
        v = len(self.types)
        self.types[v] = ir_type(t)
        return v

    def emit(self, kind: str, result_types=(), args=(), attr=None, site=None) -> tuple[int, ...]:
        results = tuple(self.value(t) for t in result_types)
        if kind == "raise_if":
            self.add_effects((MAY_RAISE,))
        self.block.ops.append(Op(kind, results, tuple(args), attr, site))
        return results

    def emit1(self, kind, t, args=(), attr=None, site=None) -> tuple[int, str]:
        return self.emit(kind, (t,), args, attr, site)[0], t

    def const(self, t: str, x) -> tuple[int, str]:
        r = self.emit1("const", t, (), x)
        self.consts[r[0]] = x
        return r

    def terminate(self, term: tuple) -> None:
        if term[0] == "raise":
            self.add_effects((MAY_RAISE,))
        self.block.term = term
        self.block = None

    def add_effects(self, effects) -> None:
        self.effects.update(effects)

    def helper(self, name: str) -> None:
        if name not in self.helpers:
            self.helpers.append(name)

    # -- the body

    def assume(self, v: int, assumed, site=None) -> None:
        """A declared assumption (F7) of a value, `assume(fact)`: the leaf IR
        states it, and the templates check it in a build with assertions
        (an `assume` op of the fact's kind and argument, which changes
        nothing)"""
        for a in assumed:
            if a.fact.kind not in ASSUMABLE:
                raise LeafError(f"assumed {a.fact}, which no template can check", site)
            self.emit("assume", (), (v,), (a.fact.kind, a.fact.arg), site)

    def assume_result(self, name: str, results, tok) -> None:
        "the assumption of a row's result, after its call"
        import ir_contract
        row = ir_contract.rows().get(name)
        if row is not None and row.result.assumed and results:
            self.assume(results[0], row.result.assumed, tok)

    def translate(self) -> LeafBody:
        import ir_contract
        uop = self.uop
        contract = ir_contract.declared_contract(uop)
        inputs, outputs = [], []
        for item in uop.stack.inputs:
            if item.name == "unused":
                continue
            if item.is_array():
                raise LeafError(f"array input {item.name}[{item.size}]")
            inputs.append(item.name)
            self.env[item.name] = self.emit1("arg", VALUE, (), item.name)
            declared = contract.input(item.name)
            if declared is not None and declared.assumed:
                self.assume(self.env[item.name][0], declared.assumed)
        for item in uop.stack.outputs:
            if item.name == "unused":
                continue
            if item.is_array():
                raise LeafError(f"array output {item.name}[{item.size}]")
            outputs.append(item.name)
            self.env.setdefault(item.name, (UNDEF, VALUE))
        if any(t.text in ("JUMPTO", "JUMPBY") for t in uop.body.tokens()):
            self.env["%branch"] = self.const("bool", 0)
        self.statements(uop.body.body)
        if self.block is not None:
            for name in outputs:
                v, _ = self.read(name, None)
                self.emit("out", (), (v,), name)
            branch = self.env.get("%branch")
            self.terminate(("next", branch[0] if branch else None))
        return LeafBody(uop.name, self.blocks, self.types, tuple(inputs),
                        tuple(outputs), frozenset(self.effects), tuple(self.helpers))

    def statements(self, stmts) -> None:
        for s in stmts:
            if self.block is None:
                return      # after a terminal statement: unreachable
            self.statement(s)

    def statement(self, s) -> None:
        kind = type(s).__name__
        if kind == "BlockStmt":
            self.statements(s.body)
        elif kind == "IfStmt":
            self.if_statement(s.condition, s.body, s.else_body)
        elif kind == "MacroIfStmt":
            self.macro_if(s)
        elif kind == "SimpleStmt":
            self.simple(s)
        else:
            # ForStmt, WhileStmt: a leaf body is acyclic
            raise LeafError(f"a {kind[:-4].lower()} loop", next(s.tokens(), None))

    def macro_if(self, s) -> None:
        words = s.condition.text.split()
        name = words[1] if len(words) > 1 else ""
        if words[0] in ("#ifdef", "#ifndef") and name in ("Py_GIL_DISABLED", "Py_STATS", "Py_DEBUG"):
            # The release build with the GIL: the macro is undefined
            taken = s.body if words[0] == "#ifndef" else (s.else_body or [])
            self.statements(taken)
            return
        raise LeafError(f"conditional compilation on {s.condition.text.strip()}", s.condition)

    def if_statement(self, cond_tokens, body, else_body) -> None:
        tokens = [t for t in cond_tokens if t.kind != "COMMENT"]
        cond = self.condition(parse_expression(tokens))
        self.branch(cond, lambda: self.statement(body),
                    (lambda: self.statement(else_body)) if else_body is not None else None)

    def branch(self, cond: int, then, otherwise) -> None:
        "if (cond) then() else otherwise(), merging the environments"
        start_env = dict(self.env)
        then_b, else_b = self.new_block(), self.new_block()
        self.block.term = ("br", cond, then_b.id, else_b.id)
        ends = []
        for blk, arm in ((then_b, then), (else_b, otherwise)):
            self.block, self.env = blk, dict(start_env)
            if arm is not None:
                arm()
            if self.block is not None:
                ends.append((self.block, self.env))
        if (otherwise is None and not then_b.ops and then_b.term in (("raise",),) and
                len(ends) == 1 and ends[0][0] is else_b and not else_b.ops):
            # if (c) { ERROR_NO_POP(); }: one conditional raise
            del self.blocks[then_b.id:]
            self.block = self.blocks[-1]
            self.block.term = None
            self.env = start_env
            self.emit("raise_if", (), (cond,))
            return
        self.merge(ends)

    def merge(self, ends) -> None:
        if not ends:
            self.block = None
            return
        if len(ends) == 1:
            self.block, self.env = ends[0]
            return
        merge = self.new_block()
        env = {}
        names = [n for n in ends[0][1] if all(n in e for _, e in ends)]
        phis = []
        for name in names:
            vals = [e[name] for _, e in ends]
            if all(v == vals[0] for v in vals):
                env[name] = vals[0]
            elif any(v[0] == UNDEF for v in vals):
                env[name] = (UNDEF, vals[0][1])
            else:
                t = vals[0][1]
                if any(ir_type(v[1]) != ir_type(t) for v in vals):
                    raise LeafError(f"{name} has different types on two paths")
                p = self.value(t)
                merge.params.append(p)
                env[name] = (p, t)
                phis.append(name)
        for blk, e in ends:
            blk.term = ("jmp", merge.id, tuple(e[n][0] for n in phis))
        self.block, self.env = merge, env

    # -- simple statements

    def simple(self, s) -> None:
        tokens = [t for t in s.contents if t.kind != "COMMENT"]
        if tokens and tokens[-1].text == ";":
            tokens = tokens[:-1]
        if not tokens:
            return
        first = tokens[0]
        if first.kind in ("GOTO", "RETURN", "BREAK", "CONTINUE", "SWITCH", "DO"):
            raise LeafError(f"'{first.text}'", first)
        if _type_start(tokens, 0):
            self.declaration(tokens)
            return
        if len(tokens) >= 3 and tokens[0].text == "(" and tokens[1].kind == "VOID":
            return      # (void)x
        depth = 0
        for i, t in enumerate(tokens):
            if t.text in "([":
                depth += 1
            elif t.text in ")]":
                depth -= 1
            elif depth == 0 and t.text in _ASSIGN_OPS:
                self.assign(tokens[:i], t, tokens[i + 1:])
                return
        if tokens[-1].text in ("++", "--") or tokens[0].text in ("++", "--"):
            op = tokens[-1] if tokens[-1].text in ("++", "--") else tokens[0]
            target = tokens[:-1] if op is tokens[-1] else tokens[1:]
            one = [type(op)(op.filename, "NUMBER", "1", op.begin, op.end)]
            fake = type(op)(op.filename, "PLUSEQUAL" if op.text == "++" else "MINUSEQUAL",
                            "+=" if op.text == "++" else "-=", op.begin, op.end)
            self.assign(target, fake, one)
            return
        e = parse_expression(tokens)
        if e.kind != "call":
            raise LeafError("an expression statement that is not a call", first)
        self.call_statement(e)

    def declaration(self, tokens) -> None:
        p = _Parser(tokens)
        base = p.type_name()
        base_stars = len(base) - len(base.rstrip("*"))
        base = base.rstrip("*")
        while True:
            stars = base_stars
            while p.at("*"):
                p.next()
                stars += 1
            name = p.next()
            if name.kind != "IDENTIFIER":
                raise LeafError("a declarator that is not a name", name)
            t = c_type(base + "*" * stars, name)
            if p.at("["):
                raise LeafError(f"array local {name.text}", name)
            if p.at("="):
                p.next()
                start = p.i
                depth = 0
                while not p.done() and not (depth == 0 and p.at(",")):
                    tk = p.next()
                    depth += tk.text in "([" and 1 or (tk.text in ")]" and -1 or 0)
                init = parse_expression(p.tokens[start:p.i])
                self.env[name.text] = self.convert(self.rvalue(init), t, name)
            else:
                self.env[name.text] = (UNDEF, t)
            if p.done():
                return
            p.expect(",")

    def assign(self, lhs_tokens, op, rhs_tokens) -> None:
        lhs = parse_expression(lhs_tokens)
        rhs = parse_expression(rhs_tokens)
        binop = _ASSIGN_OPS[op.text]
        if lhs.kind == "name" and lhs.value in self.env:
            name = lhs.value
            t = self.env[name][1]
            if binop is None:
                value = self.rvalue(rhs)
            else:
                value = self.binary(binop, self.read(name, lhs.token), self.rvalue(rhs), op)
            self.env[name] = self.convert(value, t, op)
            return
        loc = self.lvalue(lhs)
        if binop is None:
            value = self.rvalue(rhs)
        else:
            value = self.binary(binop, self.load(loc, op), self.rvalue(rhs), op)
        self.store(loc, self.convert(value, loc.type, op), op)

    # -- DSL and call statements

    def call_statement(self, e: Expr) -> None:
        name, args, tok = e.value, e.args, e.token
        if erased(name):
            return
        if name in ("EXIT_IF", "DEOPT_IF"):
            kind = "EXIT" if name == "EXIT_IF" else "DEOPT"
            if len(args) == 2 and args[1].kind == "str":
                args = args[:1]
            self.guard(kind, args[0], tok)
        elif name == "ERROR_IF":
            if len(args) > 1:
                # holding(x): the raise leaves x in its slot, which the
                # leaf's raise terminator does not model
                raise LeafError("a raise checkpoint, holding(...), is outside "
                                "the leaf subset", tok)
            if self.is_true(args[0]):
                self.terminate(("raise",))
            else:
                self.emit("raise_if", (), (self.condition(args[0]),), None, tok)
        elif name == "DECREF_INPUTS":
            # Closes the inputs that the uop consumes
            outputs = {item.name for item in self.uop.stack.outputs}
            for item in self.uop.stack.inputs:
                if item.name != "unused" and item.name not in outputs:
                    arg = Expr("name", tok, item.name, ())
                    self.primitive(PRIMITIVES["PyValue_CLOSE"],
                                   Expr("call", tok, "PyValue_CLOSE", (arg,)), False)
        elif name == "ERROR_NO_POP":
            self.terminate(("raise",))
        elif name in ("JUMPTO", "JUMPBY"):
            target = args[0]
            if not (target.kind == "name" and target.value == "oparg"):
                raise LeafError(f"{name} to a computed target", tok)
            self.env["%branch"] = self.const("bool", 1)
        elif name == "IR_REPORT_EDGE":
            self.emit("report_edge", (), (self.condition(args[0]),), None, tok)
        elif name == "IR_COMPLETE_STEP":
            kind = args[0].value
            v, t = self.rvalue(args[1])
            if t != VALUE:
                raise LeafError("a completion that is not a value", tok)
            if kind not in COMPLETIONS:
                raise LeafError(f"a completion of kind {kind}", tok)
            self.helper(COMPLETIONS[kind])
            self.add_effects(HELPERS[COMPLETIONS[kind]].effects)
            self.terminate(("complete", kind, v))
        elif name in CAPABILITY_MACROS:
            self.capability(e, statement=True)
        else:
            self.call(e, want=False)

    def is_true(self, e: Expr) -> bool:
        return e.kind == "name" and e.value == "true"

    def guard(self, kind: str, cond_expr: Expr, site, part=None) -> None:
        if self.is_true(cond_expr):
            self.terminate(("exit", kind, site))
            return
        c = self.condition(cond_expr)
        self.emit("exit_if", (), (c,), kind, site if part is None else (site, *part))

    def capability(self, e: Expr, statement: bool):
        names = []
        for a in e.args:
            if a.kind == "name":
                names.append(a.value)
            elif a.kind == "call" and a.value == "PyValue_AsKnownObjectBorrow" and len(a.args) == 1:
                # IR_GUARD_SHAPE_READ's receiver: the object of a current input
                names.append(f"_PyValue_AsKnownObjectBorrowPointer({a.args[0].value})")
            else:
                raise LeafError(f"{e.value} with a computed argument", e.token)
        if e.value == "IR_DUP_CANONICAL_ELEMENT":
            names.append(element_kind(self.uop))
        items = capability_expansion(e.value, names)
        guards = [it for it in items if it[0] == "guard"]
        k = 0
        result = None
        for item in items:
            if item[0] == "gil_disabled_miss":
                continue
            text = item[1]
            tokens = list(_tokens(text, e.token))
            if item[0] == "stmt":
                self.inline_statement(text, e.token)
            elif item[0] == "guard":
                self.guard("EXIT" if "DEOPT" not in e.value else "DEOPT",
                           parse_expression(tokens), e.token, (k, len(guards)))
                k += 1
            else:
                result = self.rvalue(parse_expression(tokens))
        if statement and result is not None:
            raise LeafError(f"{e.value} as a statement", e.token)
        return result

    def inline_statement(self, text: str, anchor) -> None:
        "A statement of a capability macro's expansion"
        from parsing import Parser
        self.statement(Parser(text, anchor.filename).stmt())

    # -- expressions

    def condition(self, e: Expr) -> int:
        v, t = self.rvalue(e)
        return self.to_bool(v, t, e.token)

    def to_bool(self, v: int, t: str, token) -> int:
        if t == "bool":
            return v
        if t == VALUE:
            raise LeafError("a value used as a condition", token)
        if is_pointer(t):
            zero = self.const(t, 0)[0]
            return self.emit1("cmp", "bool", (v, zero), "ne")[0]
        if t == "f64":
            zero = self.const("f64", 0.0)[0]
            return self.emit1("fcmp", "bool", (v, zero), "une")[0]
        zero = self.const(t, 0)[0]
        return self.emit1("cmp", "bool", (v, zero), "ne")[0]

    def convert(self, value: tuple[int, str], to: str, token) -> tuple[int, str]:
        v, t = value
        if v == UNDEF:
            raise LeafError("a value read before it is assigned", token)
        if t == to or (is_pointer(t) and is_pointer(to)):
            return v, to
        if VALUE in (t, to) or is_pointer(t) or is_pointer(to):
            raise LeafError(f"a conversion from {t} to {to}", token)
        if to == "bool":
            return self.to_bool(v, t, token), "bool"
        x = self.consts.get(v)
        if isinstance(x, int) and to != "f64" and t != "f64":
            # A constant converts at translation time
            return self.const(to, wrap(to, x))
        return self.emit1("conv", to, (v,))

    def read(self, name: str, token) -> tuple[int, str]:
        v = self.env[name]
        if v[0] == UNDEF:
            raise LeafError(f"{name} is read before it is assigned on some path", token)
        return v

    def rvalue(self, e: Expr) -> tuple[int, str]:
        k = e.kind
        if k == "name":
            return self.name(e)
        if k == "num":
            return self.number(e)
        if k == "call":
            r = self.call(e, want=True)
            if r is None:
                raise LeafError(f"{e.value} has no value", e.token)
            return r
        if k == "cast":
            target = c_type(e.value, e.token)
            return self.convert(self.rvalue(e.args[0]), target, e.token)
        if k == "unary":
            return self.unary(e)
        if k == "binary":
            if e.value in ("&&", "||"):
                return self.logical(e)
            return self.binary(e.value, self.rvalue(e.args[0]), self.rvalue(e.args[1]), e.token)
        if k == "cond":
            return self.conditional(e)
        if k in ("member", "index"):
            loc = self.lvalue(e)
            return self.load(loc, e.token)
        raise LeafError(f"a {k} expression is outside the leaf subset", e.token)

    def name(self, e: Expr) -> tuple[int, str]:
        n = e.value
        if n in self.env:
            return self.read(n, e.token)
        if n in REFUSED_NAMES:
            raise LeafError(f"the evaluator's {n}", e.token)
        if n == "oparg":
            return self.once("oparg", lambda: self.emit1("oparg", "i32"))
        for c in self.uop.caches:
            if c.name == n and n != "unused":
                ctype = self.cache_types.get(n)
                if ctype is None:
                    t = {0: "u32", 1: "u16", 2: "u32", 4: "u64"}[c.size]
                elif ctype == "_PyValueGuarded":
                    t = VALUE
                else:
                    t = c_type(ctype.replace(" ", ""), e.token)
                return self.once(("cache", n), lambda: self.emit1("cache", t, (), n))
        prim = PRIMITIVES.get(n)
        if prim is not None and prim.frame and not prim.params:
            # FRAME_CO_CONSTS: a frame primitive written as a name
            return self.primitive(prim, Expr("call", e.token, n, ()), True)
        if n in VALUE_CONSTANTS:
            return self.const(VALUE, n)
        if n in ("true", "false"):
            return self.const("bool", int(n == "true"))
        if n == "NULL":
            return self.const("PyObject*", 0)
        raise LeafError(f"'{n}' is not a local, an operand or a listed constant", e.token)

    def once(self, key, make) -> tuple[int, str]:
        "An operand read once, in the entry block, where it dominates every use"
        if key not in self.cached:
            entry = self.blocks[0]
            saved = self.block
            self.block = Block(-1)
            r = make()
            entry.ops[0:0] = self.block.ops
            self.block = saved
            self.cached[key] = r
        return self.cached[key]

    def number(self, e: Expr) -> tuple[int, str]:
        text = e.value
        stripped = text.rstrip("uUlLfF")
        suffix = text[len(stripped):].lower()
        if any(c in stripped for c in ".eE") and not stripped.lower().startswith("0x"):
            return self.const("f64", float(stripped))
        x = int(stripped, 0) if not (len(stripped) > 1 and stripped[0] == "0" and
                                       stripped[1].isdigit()) else int(stripped, 8)
        if "u" in suffix:
            t = "u64" if "l" in suffix or x >= 1 << 32 else "u32"
        elif "l" in suffix or x >= 1 << 31:
            t = "i64"
        else:
            t = "i32"
        return self.const(t, x)

    def unary(self, e: Expr) -> tuple[int, str]:
        op, arg = e.value, e.args[0]
        if op == "!":
            return self.emit1("lnot", "bool", (self.condition(arg),))
        if op == "&":
            if arg.kind == "name" and arg.value in SYMBOLS:
                return self.const("PyObject*", f"&{arg.value}")
            raise LeafError("an address of anything but a listed symbol", e.token)
        if op == "*":
            return self.load(self.lvalue(e), e.token)
        v, t = self.rvalue(arg)
        if t in (VALUE,) or is_pointer(t):
            raise LeafError(f"'{op}' on a {t}", e.token)
        t2 = "f64" if t == "f64" else _promote(t)
        v, _ = self.convert((v, t), t2, e.token)
        if op == "+":
            return v, t2
        if op == "-":
            return self.emit1("fneg" if t2 == "f64" else "neg", t2, (v,))
        if t2 == "f64":
            raise LeafError("'~' on a double", e.token)
        return self.emit1("not", t2, (v,))

    def logical(self, e: Expr) -> tuple[int, str]:
        "&& and ||, which evaluate their right operand only if needed"
        a = self.condition(e.args[0])
        short = self.const("bool", 0 if e.value == "&&" else 1)[0]
        result = {}

        def right():
            self.env["%logical"] = (self.condition(e.args[1]), "bool")

        def left():
            self.env["%logical"] = (short, "bool")

        self.env["%logical"] = (UNDEF, "bool")
        if e.value == "&&":
            self.branch(a, right, left)
        else:
            self.branch(a, left, right)
        r = self.env.pop("%logical")
        return r

    def conditional(self, e: Expr) -> tuple[int, str]:
        c = self.condition(e.args[0])
        before = len(self.block.ops)
        a = self.rvalue(e.args[1])
        b = self.rvalue(e.args[2])
        if all(op.kind == "const" for op in self.block.ops[before:]):
            t = a[1] if a[1] == b[1] else common_type(a[1], b[1])
            a, b = self.convert(a, t, e.token), self.convert(b, t, e.token)
            return self.emit1("select", t, (c, a[0], b[0]))
        del self.block.ops[before:]
        out = {}

        def arm(x):
            def run():
                self.env["%cond"] = self.rvalue(x)
            return run

        self.env["%cond"] = (UNDEF, VALUE)
        self.branch(c, arm(e.args[1]), arm(e.args[2]))
        return self.env.pop("%cond")

    def binary(self, op: str, a, b, token) -> tuple[int, str]:
        (va, ta), (vb, tb) = a, b
        if VALUE in (ta, tb):
            raise LeafError(f"'{op}' on a value: use a primitive", token)
        if is_pointer(ta) or is_pointer(tb):
            return self.pointer_arith(op, a, b, token)
        if op in _COMPARE:
            t = common_type(ta, tb)
            va, _ = self.convert(a, t, token)
            vb, _ = self.convert(b, t, token)
            if t == "f64":
                return self.emit1("fcmp", "bool", (va, vb), _COMPARE[op][2])
            return self.emit1("cmp", "bool", (va, vb), _COMPARE[op][0 if is_signed(t) else 1])
        if op in ("<<", ">>"):
            t = _promote(ta)
            va, _ = self.convert(a, t, token)
            vb, _ = self.convert(b, t, token)
            kind = "shl" if op == "<<" else ("ashr" if is_signed(t) else "lshr")
            return self.emit1(kind, t, (va, vb))
        t = common_type(ta, tb)
        va, _ = self.convert(a, t, token)
        vb, _ = self.convert(b, t, token)
        if op in ("/", "%"):
            if t == "f64":
                if op == "%":
                    raise LeafError("'%' on doubles", token)
                return self.emit1("fdiv", t, (va, vb))
            kind = {"/": "div", "%": "rem"}[op]
            return self.emit1(("s" if is_signed(t) else "u") + kind, t, (va, vb))
        names = _ARITH.get(op)
        if names is None:
            raise LeafError(f"operator '{op}'", token)
        kind = names[1] if t == "f64" else names[0]
        if kind is None:
            raise LeafError(f"'{op}' on doubles", token)
        return self.emit1(kind, t, (va, vb))

    def pointer_arith(self, op: str, a, b, token) -> tuple[int, str]:
        (va, ta), (vb, tb) = a, b
        if op in ("==", "!=") and is_pointer(ta) and is_pointer(tb):
            return self.emit1("cmp", "bool", (va, vb), _COMPARE[op][1])
        if op in ("+", "-") and is_pointer(ta) and not is_pointer(tb):
            scale = ELEMENT_SIZES.get(ta)
            if scale is None:
                raise LeafError(f"arithmetic on {ta}", token)
            vb, _ = self.convert(b, "i64", token)
            if op == "-":
                vb = self.emit1("neg", "i64", (vb,))[0]
            return self.emit1("ptradd", ta, (va, vb), scale)
        if op == "-" and ta == tb and ta in ELEMENT_SIZES:
            return self.emit1("ptrdiff", "i64", (va, vb), ELEMENT_SIZES[ta])
        raise LeafError(f"'{op}' on {ta} and {tb}", token)

    # -- memory

    def lvalue(self, e: Expr) -> _Loc:
        if e.kind == "member":
            name, arrow = e.value
            base = e.args[0]
            if arrow:
                p, t = self.rvalue(base)
                if not is_pointer(t):
                    raise LeafError("'->' on a non-pointer", e.token)
                struct_ = t[:-1]
                key = f"{struct_}.{name}"
                f = FIELDS.get(key)
                if f is None:
                    raise LeafError(f"field {key} is not listed", e.token)
                return _Loc(p, key, f.type)
            loc = self.lvalue(base)
            if loc.field is not None:
                raise LeafError("a field of a field", e.token)
            key = f"{loc.type}.{name}"
            f = FIELDS.get(key)
            if f is None:
                raise LeafError(f"field {key} is not listed", e.token)
            return _Loc(loc.base, key, f.type)
        if e.kind == "index":
            p, t = self.rvalue(e.args[0])
            if t not in ELEMENT_SIZES:
                raise LeafError(f"indexing a {t}", e.token)
            i = self.convert(self.rvalue(e.args[1]), "i64", e.token)[0]
            addr = self.emit1("ptradd", t, (p, i), ELEMENT_SIZES[t])[0]
            return _Loc(addr, None, _element(t))
        if e.kind == "call" and e.value == "GETLOCAL" and len(e.args) == 1:
            # A home of the frame: a store to it is not a heap write (CR)
            slots = self.primitive(PRIMITIVES["IR_SLOTS"], Expr("call", e.token, "IR_SLOTS", ()), True)
            i = self.convert(self.rvalue(e.args[0]), "i64", e.token)[0]
            addr = self.emit1("ptradd", "PyValue*", (slots[0], i), "PyValue")[0]
            return _Loc(addr, None, VALUE, home=True)
        if e.kind == "unary" and e.value == "*":
            p, t = self.rvalue(e.args[0])
            if not is_pointer(t) or t not in ELEMENT_SIZES:
                raise LeafError(f"'*' on a {t}", e.token)
            return _Loc(p, None, _element(t))
        raise LeafError("an assignment to something that is not a local, a field or an element",
                        e.token)

    def load(self, loc: _Loc, token) -> tuple[int, str]:
        if loc.field in ARRAY_FIELDS:
            return self.emit1("fieldaddr", loc.type, (loc.base,), loc.field)
        t = loc.type
        if loc.field is None and t != VALUE:
            raise LeafError(f"a load of a whole {t}", token)
        self.add_effects((READS_HEAP,))
        return self.emit1("load", t, (loc.base,), loc.field or t)

    def store(self, loc: _Loc, value, token) -> None:
        if loc.field in ARRAY_FIELDS:
            raise LeafError("a store to an array field", token)
        if not loc.home:
            self.add_effects((WRITES_HEAP,))
        self.emit("store", (), (loc.base, value[0]), loc.field or loc.type)

    # -- calls

    def call(self, e: Expr, want: bool):
        name, args, tok = e.value, e.args, e.token
        form = intrinsic(name)
        if form is not None and form.result.shape != "NONE":
            if form.result.shape == "LITERAL":
                return self.const("bool", int(form.result.measure))
            if form.result.moved >= 0:
                return self.rvalue(args[form.result.moved])
            return self.rvalue(args[-1])
        if erased(name):
            if want:
                raise LeafError(f"{name} has no value", tok)
            return None
        if name in CAPABILITY_MACROS:
            return self.capability(e, statement=not want)
        if name in ("__builtin_add_overflow", "__builtin_sub_overflow",
                    "__builtin_mul_overflow"):
            return self.overflow(e)
        if name == "Py_ARITHMETIC_RIGHT_SHIFT":
            t = c_type(args[0].value, tok)
            v = self.convert(self.rvalue(args[1]), t, tok)[0]
            n = self.convert(self.rvalue(args[2]), t, tok)[0]
            return self.emit1("ashr", t, (v, n))
        if name == "COMPARISON_BIT":
            a, b = self.rvalue(args[0]), self.rvalue(args[1])
            t = common_type(a[1], b[1])
            va = self.convert(a, t, tok)[0]
            vb = self.convert(b, t, tok)[0]
            return self.emit1("prim", "i32", (va, vb), name, tok)
        prim = PRIMITIVES.get(name)
        if prim is not None:
            return self.primitive(prim, e, want)
        helper = HELPERS.get(name)
        if helper is not None:
            if helper.thread:
                args = self.thread_operand(e)
            vals = [self.convert(self.rvalue(a), t, tok)[0] for a, t in zip(args, helper.params)]
            if len(args) != len(helper.params):
                raise LeafError(f"{name} takes {len(helper.params)} arguments", tok)
            self.add_effects(helper.effects)
            self.helper(name)
            results = self.emit("call", helper.results, vals, name, tok)
            self.assume_result(name, results, tok)
            return (results[0], helper.results[0]) if results else None
        raise LeafError(f"a call of {name}, which is neither a listed primitive nor a "
                        f"declared helper", tok)

    def thread_operand(self, e: Expr) -> tuple:
        "The arguments of a call that takes the thread state first, without it"
        if not e.args or e.args[0].kind != "name" or e.args[0].value != "tstate":
            raise LeafError(f"{e.value}'s first argument is not tstate", e.token)
        return e.args[1:]

    def primitive(self, prim: Primitive, e: Expr, want: bool):
        args, tok = e.args, e.token
        if prim.thread:
            args = self.thread_operand(e)
        out_params = len(prim.results) - 1 if len(prim.results) > 1 else 0
        n_in = len(prim.params)
        if prim.name == "PyValue_CLOSE_SPECIALIZED":
            if len(args) != 2 or args[1].kind != "name" or args[1].value not in HELPERS:
                raise LeafError("PyValue_CLOSE_SPECIALIZED with an unlisted destructor", tok)
            v = self.convert(self.rvalue(args[0]), VALUE, tok)[0]
            self.add_effects(prim.effects)
            self.helper(args[1].value)
            self.emit("prim", (), (v,), (prim.name, args[1].value), tok)
            return None
        if len(args) != n_in + out_params:
            raise LeafError(f"{prim.name} takes {n_in + out_params} arguments", tok)
        vals = [self.convert(self.rvalue(a), t, tok)[0] for a, t in zip(args, prim.params)]
        self.add_effects(prim.effects)
        if prim.slow:
            self.helper(prim.slow)
        results = self.emit("prim", prim.results, vals, prim.name, tok)
        self.assume_result(prim.name, results, tok)
        # Out-parameters: &local, written by the primitive
        for a, r, t in zip(args[n_in:], results[1:], prim.results[1:]):
            if not (a.kind == "unary" and a.value == "&" and a.args[0].kind == "name" and
                    a.args[0].value in self.env):
                raise LeafError(f"{prim.name}'s out-parameter is not &local", tok)
            local = a.args[0].value
            self.env[local] = self.convert((r, t), self.env[local][1], tok)
        if not results:
            if want:
                raise LeafError(f"{prim.name} has no value", tok)
            return None
        return results[0], prim.results[0]

    def overflow(self, e: Expr) -> tuple[int, str]:
        a, b, out = e.args
        if not (out.kind == "unary" and out.value == "&" and out.args[0].kind == "name"
                and out.args[0].value in self.env):
            raise LeafError("an overflow builtin's result is not &local", e.token)
        local = out.args[0].value
        t = self.env[local][1]
        if t not in ("i64", "i32", "u64", "u32"):
            raise LeafError(f"an overflow builtin into a {t}", e.token)
        va = self.convert(self.rvalue(a), t, e.token)[0]
        vb = self.convert(self.rvalue(b), t, e.token)[0]
        kind = e.value[len("__builtin_"):]
        result, overflowed = self.emit(kind.replace("_overflow", "_ovf"), (t, "bool"), (va, vb))
        self.env[local] = (result, t)
        return overflowed, "bool"


def _tokens(text: str, anchor):
    "The tokens of an expansion's text, placed at the macro's token"
    import lexer
    for t in lexer.tokenize(text):
        if t.kind in ("NEWLINE", "COMMENT"):
            continue
        yield lexer.Token(anchor.filename, t.kind, t.text, anchor.begin, anchor.end)


def translate(uop, cache_types: dict[str, str] | None = None) -> LeafBody:
    "The leaf IR of uop, or LeafError if its body is outside the subset"
    return Translator(uop, cache_types).translate()


def leaf_of(uop, cache_types: dict[str, str]) -> LeafBody | None:
    "The leaf IR of uop, or None if its body is outside the subset"
    try:
        return translate(uop, cache_types)
    except LeafError:
        return None


def check_leaf(record, parts) -> None:
    """A uop annotated `leaf` must be inside the subset.  The generator runs
    this after its own checks of the handler (beside check_record()), so that
    their diagnostics come first."""
    by_name = {part.name: part for part in parts}
    for uop in record.uops:
        part = by_name[uop.name]
        if "leaf" in part.annotations and uop.leaf is None:
            # Translate again for the diagnostic: only this error path needs it
            try:
                translate(part, {fb.name: fb.field.ctype for fb in uop.feedback})
            except LeafError as error:
                from analyzer import analysis_error
                raise analysis_error(f"{part.name} is annotated leaf, but {error}",
                                     error.token or part.body.open)


def report(records, parts) -> list[str]:
    """Report mode: every uop of the records, once per feedback layout it is
    used with, inside the subset with its effects and helpers, or outside with
    the first construct the checker refused.  parts maps uop names to the
    analyzer's uops."""
    uses = {(u.name, tuple(sorted((fb.name, fb.field.ctype) for fb in u.feedback)))
            for r in records.values() for u in r.uops}
    lines = []
    for name, cache_types in sorted(uses):
        try:
            result = translate(parts[name], dict(cache_types))
        except LeafError as error:
            lines.append(f"{name:44} out  {error}")
            continue
        effects = " ".join(e for e in EFFECTS if e in result.effects) or "-"
        helpers = " ".join(sorted(result.helpers)) or "-"
        lines.append(f"{name:44} LEAF effects {effects}; helpers {helpers}")
    inside = sum(" LEAF " in line for line in lines)
    lines.append(f"{inside} of {len(lines)} uops inside the leaf subset")
    return lines


def main() -> None:
    import argparse
    import sys
    from analyzer import Uop, analyze_files
    from generators_common import DEFAULT_INPUT
    import ir_generator
    parser = argparse.ArgumentParser(description="Check the uops against the leaf subset")
    parser.add_argument("-o", "--output", default=None)
    parser.add_argument("--dump", action="append", default=[],
                        help="print the leaf IR of this uop")
    parser.add_argument("input", nargs="*", default=[DEFAULT_INPUT])
    args = parser.parse_args()
    analysis = analyze_files(args.input)
    records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
    parts = {part.name: part
             for inst in ir_generator.handler_instructions(analysis).values()
             for part in inst.parts if isinstance(part, Uop)}
    lines = report(records, parts)
    for name in args.dump:
        leaf = next((u.leaf for r in records.values() for u in r.uops
                     if u.name == name and u.leaf is not None), None)
        if leaf is not None:
            lines += [f"{name}:"] + ["  " + line for line in leaf.dump()]
    out = open(args.output, "w") if args.output else sys.stdout
    with out:
        out.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
