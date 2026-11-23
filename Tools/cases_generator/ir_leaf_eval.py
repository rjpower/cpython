"""The reference evaluator of leaf uops (ir_leaf.py).

The specs of the value primitives run here, on a Context: the layout of the
running interpreter (numbers from C), memory, the frame and the helpers'
models.  Memory reads go to the process's real memory through ctypes; writes
go to an overlay and never reach real memory, so the evaluator can run a
uop on live objects and report what the uop would change, which a test then
compares with what the C uop changed.
"""

import ctypes
import math
from dataclasses import dataclass, field

import ir_leaf


@dataclass(frozen=True)
class Layout:
    "The numbers that the specs name: ir_leaf.CONSTANTS, FIELDS, SIZES, SYMBOLS"
    constants: dict[str, int]
    offsets: dict[str, int]
    sizes: dict[str, int]
    symbols: dict[str, int]

    @classmethod
    def from_c(cls, capi) -> "Layout":
        "The layout of the running interpreter, from _testinternalcapi"
        return cls(*capi.leaf_layout())


_CTYPES = {
    "bool": ctypes.c_bool, "i8": ctypes.c_int8, "u8": ctypes.c_uint8,
    "i16": ctypes.c_int16, "u16": ctypes.c_uint16, "i32": ctypes.c_int32,
    "u32": ctypes.c_uint32, "i64": ctypes.c_int64, "u64": ctypes.c_uint64,
    "f64": ctypes.c_double, ir_leaf.VALUE: ctypes.c_uint64,
}


def _ctype(t: str):
    return ctypes.c_uint64 if ir_leaf.is_pointer(t) else _CTYPES[t]


class RealMemory:
    "The process's memory, read only"

    def load(self, addr: int, t: str):
        if addr < 4096:
            raise MemoryError(f"load of {t} at {addr:#x}")
        return _ctype(t).from_address(addr).value

    def store(self, addr: int, t: str, x) -> None:
        raise MemoryError("the evaluator never writes real memory")


class Overlay:
    """Copy on write over another memory: writes stay here, and reads see
    them.  writes() lists each written location with its value before the
    first write and after the last."""

    def __init__(self, base) -> None:
        self.base = base
        self.cells: dict[int, tuple[str, object]] = {}
        self.before: dict[int, object] = {}

    def load(self, addr: int, t: str):
        if addr in self.cells:
            ct, x = self.cells[addr]
            if ct != t:
                raise MemoryError(f"load of {t} at {addr:#x}, stored as {ct}")
            return x
        return self.base.load(addr, t)

    def store(self, addr: int, t: str, x) -> None:
        if addr not in self.before:
            self.before[addr] = self.base.load(addr, t)
        self.cells[addr] = (t, x)

    def writes(self) -> list[tuple[int, str, object, object]]:
        return [(addr, t, self.before[addr], x)
                for addr, (t, x) in sorted(self.cells.items())]


class Box(int):
    """A value that a helper made: a new object, which has no address the
    evaluator knows.  Its word is a fake address in pointer space, owned and
    mortal, so that the tag tests see an object; any load through it fails.
    type and value say what object it is."""

    _next = 0x1000

    def __new__(cls, type: str, value):
        Box._next += 0x10
        self = super().__new__(cls, Box._next)
        self.type = type
        self.value = value
        return self

    def __repr__(self) -> str:
        return f"Box({self.type}, {self.value!r})"


def _box_int64(cx, i):
    return Box("int", i)


def _box_double(cx, d):
    return Box("float", d)


def _multiply_int64(cx, a, b):
    return Box("int", a * b)


def _lshift_int64(cx, v, n):
    return Box("int", v << n)


# The language result of each helper, which is all a test of a uop can
# compare: a box's address is the allocator's.  A release is recorded.
HELPER_MODELS = {
    "_PyValue_BoxInt64": _box_int64,
    "_PyValue_BoxDouble": _box_double,
    "_PyValue_MultiplyInt64": _multiply_int64,
    "_PyValue_LshiftInt64": _lshift_int64,
}


@dataclass
class Context:
    "What a spec reads: the layout, memory, the frame and the helpers"
    layout: Layout
    mem: object
    frame: int = 0
    calls: list = field(default_factory=list)

    def k(self, name: str) -> int:
        return self.layout.constants[name]

    def off(self, name: str) -> int:
        return self.layout.offsets[name]

    def size(self, name: str) -> int:
        return self.layout.sizes[name]

    def addr(self, name: str) -> int:
        return self.layout.symbols[name]

    def load(self, addr: int, t: str):
        if isinstance(addr, Box):
            raise MemoryError(f"load through {addr!r}")
        return self.mem.load(addr, t)

    def store(self, addr: int, t: str, x) -> None:
        if isinstance(addr, Box):
            raise MemoryError(f"store through {addr!r}")
        self.mem.store(addr, t, x)

    def call(self, name: str, *args):
        self.calls.append((name, args))
        model = HELPER_MODELS.get(name)
        return model(self, *args) if model is not None else None


# ---------------------------------------------------------------------------
# The evaluator of leaf IR (ir_leaf.LeafBody)

# The outcomes of a uop; leaf_probe() in _testinternalcapi numbers them so
OUTCOMES = ("NEXT", "EXIT", "DEOPT", "RAISE", "COMPLETE")


class UndefinedBehavior(Exception):
    "The body does what C leaves undefined: signed overflow, a bad shift"


@dataclass
class Result:
    outcome: str
    outputs: dict[str, object]
    branch: bool = False
    edge: int = 0               # 0, or 1 + the flag that report_edge saw
    completed: object = None


def _signed_check(t: str, x: int) -> int:
    "x, a mathematical result, as a value of signed type t; UB if it does not fit"
    n = ir_leaf.bits_of(t)
    if not -(1 << (n - 1)) <= x < (1 << (n - 1)):
        raise UndefinedBehavior(f"{t} overflow: {x}")
    return x


def _int_result(t: str, x: int) -> int:
    return _signed_check(t, x) if ir_leaf.is_signed(t) else ir_leaf.wrap(t, x)


def _unsigned(t: str, x: int) -> int:
    return x & ((1 << ir_leaf.bits_of(t)) - 1)


def _trunc_div(a: int, b: int) -> int:
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


_CMP = {"eq": lambda a, b: a == b, "ne": lambda a, b: a != b,
        "slt": lambda a, b: a < b, "sle": lambda a, b: a <= b,
        "sgt": lambda a, b: a > b, "sge": lambda a, b: a >= b,
        "ult": lambda a, b: a < b, "ule": lambda a, b: a <= b,
        "ugt": lambda a, b: a > b, "uge": lambda a, b: a >= b}
_FCMP = {"oeq": lambda a, b: a == b, "olt": lambda a, b: a < b,
         "ole": lambda a, b: a <= b, "ogt": lambda a, b: a > b,
         "oge": lambda a, b: a >= b,
         "one": lambda a, b: a == a and b == b and a != b}


def _const(cx: Context, t: str, x):
    if t == ir_leaf.VALUE and isinstance(x, str):
        symbol, tag = ir_leaf.VALUE_CONSTANTS[x]
        return 0 if symbol is None else cx.addr(symbol) | tag
    if isinstance(x, str) and x.startswith("&"):
        return cx.addr(x[1:])
    if isinstance(x, str):
        return cx.k(x)
    return x


def evaluate(body, cx: Context, inputs: dict[str, int], caches: dict[str, int],
             oparg: int = 0, immediates: dict[str, int] | None = None) -> Result:
    """Run body: its exits, outputs and, through cx, its loads, stores and
    helper calls.  Signed overflow and out-of-range shifts raise
    UndefinedBehavior: the C uop's behavior is then not defined either."""
    types = body.types
    values: dict[int, object] = {}
    outputs: dict[str, object] = {}
    edge = 0
    blocks = {block.id: block for block in body.blocks}
    block = body.blocks[0]
    while True:
        for op in block.ops:
            a = [values[v] for v in op.args]
            t = types[op.results[0]] if op.results else None
            k = op.kind
            if k == "arg":
                r = inputs[op.attr]
            elif k == "cache":
                x = caches[op.attr]
                r = ir_leaf.wrap(t, x) if t not in (ir_leaf.VALUE, "ptr") else x
            elif k == "oparg":
                r = oparg
            elif k == "imm":
                r = (immediates or {})[op.attr]
            elif k == "const":
                r = _const(cx, t, op.attr)
            elif k == "prim":
                name = op.attr[0] if isinstance(op.attr, tuple) else op.attr
                prim = ir_leaf.PRIMITIVES[name]
                if isinstance(op.attr, tuple):        # CLOSE_SPECIALIZED
                    r = prim.spec(cx, *a, op.attr[1])
                else:
                    r = prim.spec(cx, *a)
            elif k == "call":
                r = cx.call(op.attr, *a)
            elif k == "load":
                field_type = (ir_leaf.FIELDS[op.attr].type if op.attr in ir_leaf.FIELDS
                              else op.attr)
                addr = a[0] + (cx.off(op.attr) if op.attr in ir_leaf.FIELDS else 0)
                r = cx.load(addr, field_type)
            elif k == "store":
                field_type = (ir_leaf.FIELDS[op.attr].type if op.attr in ir_leaf.FIELDS
                              else op.attr)
                addr = a[0] + (cx.off(op.attr) if op.attr in ir_leaf.FIELDS else 0)
                cx.store(addr, field_type, a[1])
                r = None
            elif k == "fieldaddr":
                r = a[0] + cx.off(op.attr)
            elif k == "ptradd":
                size = op.attr if isinstance(op.attr, int) else cx.size(op.attr)
                r = a[0] + a[1] * size
            elif k == "ptrdiff":
                size = op.attr if isinstance(op.attr, int) else cx.size(op.attr)
                r = (a[0] - a[1]) // size
            elif k in ("add", "sub", "mul"):
                x = {"add": a[0] + a[1], "sub": a[0] - a[1], "mul": a[0] * a[1]}[k]
                r = _int_result(t, x)
            elif k in ("sdiv", "srem", "udiv", "urem"):
                if a[1] == 0:
                    raise UndefinedBehavior("division by zero")
                q = _trunc_div(a[0], a[1])
                r = _int_result(t, q if k.endswith("div") else a[0] - q * a[1])
            elif k in ("and", "or", "xor"):
                x = {"and": a[0] & a[1], "or": a[0] | a[1], "xor": a[0] ^ a[1]}[k]
                r = ir_leaf.wrap(t, x)
            elif k in ("shl", "ashr", "lshr"):
                n = ir_leaf.bits_of(t)
                if not 0 <= a[1] < n:
                    raise UndefinedBehavior(f"shift by {a[1]} of a {t}")
                if k == "shl":
                    if ir_leaf.is_signed(t) and a[0] < 0:
                        raise UndefinedBehavior("left shift of a negative value")
                    r = _int_result(t, a[0] << a[1])
                elif k == "ashr":
                    r = a[0] >> a[1]
                else:
                    r = _unsigned(t, a[0]) >> a[1]
            elif k == "neg":
                r = _int_result(t, -a[0])
            elif k == "not":
                r = ir_leaf.wrap(t, ~a[0])
            elif k == "lnot":
                r = int(not a[0])
            elif k in ("fadd", "fsub", "fmul", "fdiv"):
                x, y = a
                if k == "fdiv":
                    r = (x / y if y != 0 else
                         math.copysign(math.inf, x) * math.copysign(1.0, y) if x != 0 and x == x
                         else math.nan)
                else:
                    r = {"fadd": x + y, "fsub": x - y, "fmul": x * y}[k]
            elif k == "fneg":
                r = -a[0]
            elif k in ("add_ovf", "sub_ovf", "mul_ovf"):
                x = {"add_ovf": a[0] + a[1], "sub_ovf": a[0] - a[1],
                     "mul_ovf": a[0] * a[1]}[k]
                rt = types[op.results[0]]
                values[op.results[0]] = ir_leaf.wrap(rt, x)
                values[op.results[1]] = int(ir_leaf.wrap(rt, x) != x)
                continue
            elif k == "conv":
                r = _convert(types[op.args[0]], t, a[0])
            elif k == "cmp":
                x, y = a
                if op.attr[0] == "u":
                    src = types[op.args[0]]
                    x, y = _unsigned(src, x), _unsigned(src, y)
                r = int(_CMP[op.attr](x, y))
            elif k == "fcmp":
                r = int(_FCMP[op.attr](*a))
            elif k == "select":
                r = a[1] if a[0] else a[2]
            elif k == "exit_if":
                if a[0]:
                    return Result(op.attr, {}, edge=edge)
                continue
            elif k == "raise_if":
                if a[0]:
                    return Result("RAISE", {}, edge=edge)
                continue
            elif k == "report_edge":
                edge = 1 + bool(a[0])
                continue
            elif k == "out":
                outputs[op.attr] = a[0]
                continue
            else:
                raise NotImplementedError(k)
            if op.results:
                if len(op.results) == 1:
                    values[op.results[0]] = r
                else:
                    for v, x in zip(op.results, r):
                        values[v] = x
        term = block.term
        kind = term[0]
        if kind == "br":
            block = blocks[term[2] if values[term[1]] else term[3]]
        elif kind == "jmp":
            target = blocks[term[1]]
            for param, arg in zip(target.params, term[2]):
                values[param] = values[arg]
            block = target
        elif kind == "next":
            branch = term[1] is not None and bool(values[term[1]])
            return Result("NEXT", outputs, branch, edge)
        elif kind == "raise":
            return Result("RAISE", {}, edge=edge)
        elif kind == "exit":
            return Result(term[1], {}, edge=edge)
        elif kind == "complete":
            return Result("COMPLETE", {}, edge=edge, completed=values[term[2]])
        else:
            raise NotImplementedError(kind)


def _convert(src: str, to: str, x):
    if to == "f64":
        return float(x)
    if src == "f64":
        if x != x or math.isinf(x):
            raise UndefinedBehavior(f"conversion of {x} to {to}")
        return _int_result(to, int(x)) if ir_leaf.is_signed(to) else ir_leaf.wrap(to, int(x))
    if to == "bool":
        return int(x != 0)
    if src == "ptr" or to == "ptr":
        return x
    return ir_leaf.wrap(to, x)
