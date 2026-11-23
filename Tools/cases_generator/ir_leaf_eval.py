"""The reference evaluator of leaf uops (ir_leaf.py).

The specs of the value primitives run here, on a Context: the layout of the
running interpreter (numbers from C), memory, the frame and the helpers'
models.  Memory reads go to the process's real memory through ctypes; writes
go to an overlay and never reach real memory, so the evaluator can run a
uop on live objects and report what the uop would change, which a test then
compares with what the C uop changed.
"""

import ctypes
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
