"""Read the baseline JIT's stencils from Python/jit_stencils.o and write
their table, generated_jit_stencils.h.

The object is the interpreter's arms compiled once more under the
stencil bindings (Python/jit_stencils.c), for x86-64 ELF, by GCC with the
interpreter's code model.  Each arm is a function _JIT_STENCIL_<arm> in a
section of its own, and GCC may move its cold paths to a second section;
those are the stencil's hot and cold parts.  A jump table of an arm is a
third part, its data.  The local functions and read-only data that the
arms reach form one support image, which the JIT copies once.

Every relocation of a part becomes a hole of a kind in HOLES, or the arm is
refused, and a refused arm that is not a declared gap (GAPS) fails the
build.  Each arm's holes must be among those that its declaration
(Python/generated_jit_stencil_decls.h, which the object carries in its
.jit_decls section) allows.  Standard library only: this host has no LLVM.
"""

from __future__ import annotations

import argparse
import dataclasses
import enum
import io
import struct
import sys
from collections.abc import Iterator
from dataclasses import dataclass, field

# ELF

SHT_SYMTAB = 2
SHT_RELA = 4
SHT_NOBITS = 8
SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHF_MERGE = 0x10
STT_OBJECT = 1
STT_FUNC = 2
STT_SECTION = 3
STT_TLS = 6
STB_LOCAL = 0
SHN_UNDEF = 0
EM_X86_64 = 62
ET_REL = 1


class Reloc(enum.IntEnum):
    """The x86-64 relocation kinds; the reader handles some (RELOCS)"""
    R_X86_64_64 = 1
    R_X86_64_PC32 = 2
    R_X86_64_GOT32 = 3
    R_X86_64_PLT32 = 4
    R_X86_64_GOTPCREL = 9
    R_X86_64_32 = 10
    R_X86_64_32S = 11
    R_X86_64_TLSGD = 19
    R_X86_64_TLSLD = 20
    R_X86_64_DTPOFF32 = 21
    R_X86_64_GOTTPOFF = 22
    R_X86_64_TPOFF32 = 23
    R_X86_64_PC64 = 24
    R_X86_64_GOTPCRELX = 41
    R_X86_64_REX_GOTPCRELX = 42


@dataclass
class Section:
    index: int
    name: str
    type: int
    flags: int
    size: int
    align: int
    data: bytes


@dataclass
class Symbol:
    name: str
    value: int
    size: int
    type: int
    bind: int
    shndx: int


@dataclass
class Rela:
    offset: int
    type: int
    symbol: Symbol
    addend: int


class Elf:
    """The sections, symbols and relocations of an x86-64 relocatable ELF"""

    def __init__(self, data: bytes) -> None:
        if data[:4] != b"\x7fELF" or data[4] != 2 or data[5] != 1:
            raise Refused(["not a 64-bit little-endian ELF object"])
        (e_type, e_machine, _, _, _, e_shoff, _, _, _, _, e_shentsize,
         e_shnum, e_shstrndx) = struct.unpack_from("<HHIQQQIHHHHHH", data, 16)
        if e_type != ET_REL or e_machine != EM_X86_64:
            raise Refused(["not an x86-64 relocatable object"])
        headers = [struct.unpack_from("<IIQQQQIIQQ", data, e_shoff + i * e_shentsize)
                   for i in range(e_shnum)]
        names = headers[e_shstrndx]
        strtab = data[names[4]:names[4] + names[5]]
        self.sections: list[Section] = []
        for i, (name, typ, flags, _, off, size, link, info, align, entsize) in enumerate(headers):
            body = b"" if typ == SHT_NOBITS else data[off:off + size]
            self.sections.append(Section(i, cstring(strtab, name), typ, flags, size,
                                         align, body))
        self.symbols: list[Symbol] = []
        for h in headers:
            if h[1] != SHT_SYMTAB:
                continue
            strs = headers[h[6]]
            names_ = data[strs[4]:strs[4] + strs[5]]
            for off in range(h[4], h[4] + h[5], 24):
                name, info, _, shndx, value, size = struct.unpack_from("<IBBHQQ", data, off)
                self.symbols.append(Symbol(cstring(names_, name), value, size,
                                           info & 0xF, info >> 4, shndx))
        # The relocations of each section, by the section's index
        self.relocations: dict[int, list[Rela]] = {}
        for h in headers:
            if h[1] != SHT_RELA:
                continue
            relas = []
            for off in range(h[4], h[4] + h[5], 24):
                r_offset, r_info, r_addend = struct.unpack_from("<QQq", data, off)
                relas.append(Rela(r_offset, r_info & 0xFFFFFFFF,
                                  self.symbols[r_info >> 32], r_addend))
            self.relocations[h[7]] = sorted(relas, key=lambda r: r.offset)
        self.by_name = {s.name: s for s in self.sections}

    def symbol_section(self, symbol: Symbol) -> Section | None:
        if symbol.shndx == SHN_UNDEF or symbol.shndx >= len(self.sections):
            return None
        return self.sections[symbol.shndx]


def cstring(table: bytes, offset: int) -> str:
    return table[offset:table.index(b"\0", offset)].decode()


# The table of holes

class Kind(enum.Enum):
    """What a hole is bound to.  The names of the hole symbols are the
    stencil bindings' (Python/jit_stencils.c)."""
    FIELD_A = "_JIT_A"
    FIELD_B = "_JIT_B"
    FIELD_C = "_JIT_C"
    FIELD_D = "_JIT_D"
    CONTINUE = "_JIT_CONTINUE"
    TARGET = "_JIT_TARGET"
    SWITCH = "_JIT_SWITCH"
    REDIRECT = "_JIT_REDIRECT"
    AGAIN = "_JIT_AGAIN"
    MISS = "_JIT_MISS"
    PROCESS_ERROR = "_JIT_LABEL_process_error"
    CLOSE_DISPATCH = "_JIT_LABEL_close_dispatch"
    GEN_RETURN = "_JIT_LABEL_gen_return"
    EXTERN = "extern"           # a symbol of libpython
    TPOFF = "tpoff"             # a thread-local's offset from the thread pointer
    PART = "part"               # another part of the same stencil
    SUPPORT = "support"         # the support image


HOLES = {k.value: k for k in Kind if k.value.startswith("_JIT_")}
FIELDS = {Kind.FIELD_A: "A", Kind.FIELD_B: "B", Kind.FIELD_C: "C", Kind.FIELD_D: "D"}


class Patch(enum.Enum):
    """How the emitter writes a hole's 32 bits, with S the bound value,
    A the addend and P the hole's address"""
    REL32 = "rel32"         # S + A - P, which must fit in 32 signed bits
    VALUE32 = "value32"     # S + A - P modulo 2^32: a field's value
    IMM32 = "imm32"         # S as a signed 32-bit immediate
    ABS64 = "abs64"         # S + A: an address in a slot of the GOT


# The exits of the declarations (rop_model's exit kinds) and the holes the
# bindings lower each to.  `next`, `branch` and `jump` take tail closes
# through close_dispatch (rop_cgen's lower_tail_closes); an entry raises
# RecursionError through process_error (IR_START_FRAME); a generator's
# return leaves through gen_return (IR_GEN_RETURN).
EXIT_HOLES: dict[str, frozenset[Kind]] = {
    "next": frozenset({Kind.CONTINUE, Kind.CLOSE_DISPATCH}),
    "branch": frozenset({Kind.TARGET, Kind.CLOSE_DISPATCH}),
    "jump": frozenset({Kind.TARGET, Kind.CLOSE_DISPATCH}),
    "miss": frozenset({Kind.MISS}),
    "raise": frozenset({Kind.PROCESS_ERROR}),
    "unwind": frozenset({Kind.PROCESS_ERROR}),
    "redirect": frozenset({Kind.REDIRECT}),
    "again": frozenset({Kind.AGAIN}),
    "enter": frozenset({Kind.SWITCH, Kind.PROCESS_ERROR}),
    "return": frozenset({Kind.SWITCH, Kind.GEN_RETURN}),
    "yield": frozenset({Kind.SWITCH}),
    "exit": frozenset(),
}

# The holes that any arm may have, whatever it declares
FREE_HOLES = frozenset({Kind.EXTERN, Kind.TPOFF, Kind.PART, Kind.SUPPORT})

# The arms that get no stencil, each with its reason (the scope cut): the
# JIT runs them through the interpreter's arm at the word
GAPS: dict[str, str] = {}

STENCIL = "_JIT_STENCIL_"
GOT = "GOT "
PARTS = ("hot", "cold", "data")


@dataclass
class Hole:
    part: int               # index in PARTS
    offset: int
    kind: Kind
    patch: Patch
    addend: int
    symbol: str = ""        # EXTERN, TPOFF: the symbol's name
    target: int = 0         # PART: the part it refers to
    # SUPPORT: the item (section, start) whose place in the image read()
    # adds to the addend, which counts from the item's start until then
    item: tuple[int, int] | None = None


@dataclass
class CfaOp:
    pc: int                 # the offset in the part where the op applies
    op: int                 # the DWARF CFA opcode (high bits only for offset/restore)
    reg: int = 0
    value: int = 0


@dataclass
class Part:
    data: bytearray
    cfa: list[CfaOp] = field(default_factory=list)


@dataclass
class Stencil:
    name: str
    generic: str | None
    fields: frozenset[str]
    exits: frozenset[str]
    parts: list[Part | None]
    holes: list[Hole]


@dataclass
class Table:
    stencils: list[Stencil]
    support: Part
    support_holes: list[Hole]
    # (name, offset, size) of each item of the support image
    support_items: list[tuple[str, int, int]]
    # The offset in the support image of the PLT entry of each symbol that
    # a call or a jump names
    plt: dict[str, int] = field(default_factory=dict)


ENDBR64 = b"\xF3\x0F\x1E\xFA"


class Refused(Exception):
    def __init__(self, problems: list[str]) -> None:
        super().__init__("\n".join(problems))
        self.problems = problems


# Instructions at a relocation

def is_lea_rip(code: bytes, at: int) -> bool:
    """lea disp32(%rip), %reg with disp32 at `at`"""
    return at >= 2 and code[at - 2] == 0x8D and code[at - 1] & 0xC7 == 0x05


def is_call_or_branch(code: bytes, at: int) -> bool:
    """call rel32, jmp rel32 or jcc rel32 with rel32 at `at`"""
    return (at >= 1 and code[at - 1] in (0xE8, 0xE9)) or (
        at >= 2 and code[at - 2] == 0x0F and 0x80 <= code[at - 1] <= 0x8F)


def is_branch(code: bytes, at: int) -> bool:
    """jmp rel32 or jcc rel32 with rel32 at `at`"""
    return (at >= 1 and code[at - 1] == 0xE9) or (
        at >= 2 and code[at - 2] == 0x0F and 0x80 <= code[at - 1] <= 0x8F)


def relax_gotpcrelx(code: bytearray, at: int) -> tuple[int, int] | None:
    """Rewrite a GOT load or call at `at` as ld does for a symbol defined in
    the same image: mov foo@GOTPCREL(%rip) to lea foo(%rip), call *foo@GOTPCREL(%rip)
    to addr32 call foo, jmp *foo@GOTPCREL(%rip) to jmp foo; nop.  Returns the
    rel32's offset and the addend change, or None for any other form."""
    if at < 2:
        return None
    if code[at - 2] == 0x8B and code[at - 1] & 0xC7 == 0x05:
        code[at - 2] = 0x8D
        return at, 0
    if code[at - 2] == 0xFF and code[at - 1] == 0x15:
        code[at - 2:at] = b"\x67\xE8"
        return at, 0
    if code[at - 2] == 0xFF and code[at - 1] == 0x25:
        code[at - 2:at + 4] = b"\xE9" + bytes(4) + b"\x90"
        return at - 1, 0
    return None


def relax_gottpoff(code: bytearray, at: int) -> bool:
    """Rewrite mov foo@GOTTPOFF(%rip), %reg as mov $tpoff, %reg (ld's
    initial-exec to local-exec); only the 64-bit mov with a REX prefix"""
    if at < 3:
        return False
    rex, op, modrm = code[at - 3], code[at - 2], code[at - 1]
    if rex & 0xF8 != 0x48 or op != 0x8B or modrm & 0xC7 != 0x05:
        return False
    reg = (modrm >> 3) & 7
    # REX.R names the register in ModRM.reg; the immediate form takes it in
    # ModRM.rm, so it moves to REX.B
    code[at - 3] = 0x48 | ((rex >> 2) & 1)
    code[at - 2] = 0xC7
    code[at - 1] = 0xC0 | reg
    return True


# The declarations

def declarations(elf: Elf) -> dict[str, tuple[str | None, frozenset[str], frozenset[str]]]:
    sec = elf.by_name.get(".jit_decls")
    if sec is None:
        raise Refused(["the object has no .jit_decls section"])
    decls = {}
    for text in sec.data.split(b"\0"):
        if not text:
            continue
        head, fields, exits = text.decode().split("|")
        name, generic = head.split(" ")
        unknown = set(exits.split()) - EXIT_HOLES.keys()
        if unknown:
            raise Refused([f"{name}: exits {sorted(unknown)} have no holes in EXIT_HOLES"])
        decls[name] = (None if generic == "-" else generic,
                       frozenset(fields.split()), frozenset(exits.split()))
    return decls


def allowed(decl: tuple[str | None, frozenset[str], frozenset[str]]) -> frozenset[Kind]:
    generic, fields, exits = decl
    kinds = set(FREE_HOLES)
    kinds |= {k for k, f in FIELDS.items() if f in fields}
    for e in exits:
        kinds |= EXIT_HOLES[e]
    if generic is None:
        kinds.discard(Kind.MISS)
    return frozenset(kinds)


# Call frame information

def uleb(data: bytes, i: int) -> tuple[int, int]:
    v = shift = 0
    while True:
        b = data[i]
        i += 1
        v |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            return v, i


def sleb(data: bytes, i: int) -> tuple[int, int]:
    v = shift = 0
    while True:
        b = data[i]
        i += 1
        v |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            if b & 0x40:
                v -= 1 << shift
            return v, i


@dataclass
class Cie:
    code_align: int
    data_align: int
    instructions: bytes


def cfa_ops(program: bytes, cie: Cie, where: str) -> list[CfaOp]:
    """Decode a CFA program into ops at absolute offsets in the part"""
    ops: list[CfaOp] = []
    pc = i = 0
    while i < len(program):
        b = program[i]
        i += 1
        high, low = b & 0xC0, b & 0x3F
        if high == 0x40:
            pc += low * cie.code_align
        elif high == 0x80:
            off, i = uleb(program, i)
            ops.append(CfaOp(pc, 0x80, low, off * cie.data_align))
        elif high == 0xC0:
            ops.append(CfaOp(pc, 0xC0, low))
        elif b == 0x00:
            pass
        elif b in (0x02, 0x03, 0x04):
            size = {0x02: 1, 0x03: 2, 0x04: 4}[b]
            pc += int.from_bytes(program[i:i + size], "little") * cie.code_align
            i += size
        elif b == 0x0C:
            reg, i = uleb(program, i)
            off, i = uleb(program, i)
            ops.append(CfaOp(pc, 0x0C, reg, off))
        elif b == 0x0D:
            reg, i = uleb(program, i)
            ops.append(CfaOp(pc, 0x0D, reg))
        elif b == 0x0E:
            off, i = uleb(program, i)
            ops.append(CfaOp(pc, 0x0E, 0, off))
        elif b == 0x05:
            reg, i = uleb(program, i)
            off, i = uleb(program, i)
            ops.append(CfaOp(pc, 0x80, reg, off * cie.data_align))
        elif b == 0x06:
            reg, i = uleb(program, i)
            ops.append(CfaOp(pc, 0xC0, reg))
        elif b in (0x0A, 0x0B):
            ops.append(CfaOp(pc, b))
        else:
            raise Refused([f"{where}: CFA opcode {b:#x} is not in the reader's table"])
    return ops


def frames(elf: Elf) -> dict[tuple[int, int], list[CfaOp]]:
    """The CFA ops of every FDE, by the (section, offset) its code starts at"""
    sec = elf.by_name[".eh_frame"]
    data = sec.data
    relas = {r.offset: r for r in elf.relocations.get(sec.index, [])}
    cies: dict[int, Cie] = {}
    out: dict[tuple[int, int], list[CfaOp]] = {}
    i = 0
    while i < len(data):
        (length,) = struct.unpack_from("<I", data, i)
        if length == 0:
            break
        if length == 0xFFFFFFFF:
            raise Refused([".eh_frame: 64-bit entries are not in the reader's table"])
        start, end = i + 4, i + 4 + length
        (cie_id,) = struct.unpack_from("<I", data, start)
        if cie_id == 0:
            j = start + 5
            aug = cstring(data, j)
            j += len(aug) + 1
            code_align, j = uleb(data, j)
            data_align, j = sleb(data, j)
            _, j = uleb(data, j)                    # the return address column
            if aug != "zR":
                raise Refused([f".eh_frame: CIE augmentation {aug!r}"])
            auglen, j = uleb(data, j)
            if data[j] != 0x1B:                     # pcrel | sdata4
                raise Refused([f".eh_frame: FDE pointer encoding {data[j]:#x}"])
            j += auglen
            cies[i] = Cie(code_align, data_align, data[j:end])
        else:
            cie = cies[start - cie_id]
            r = relas.get(start + 4)
            if r is None or r.type != Reloc.R_X86_64_PC32:
                raise Refused([f".eh_frame: FDE at {i:#x} has no PC32 start"])
            target = elf.symbol_section(r.symbol)
            # pcrel: the pointer is S + A, the start of the code
            offset = r.symbol.value + r.addend if r.symbol.type != STT_SECTION else r.addend
            j = start + 12
            auglen, j = uleb(data, j)
            j += auglen
            where = f"FDE of {target.name}+{offset:#x}"
            ops = cfa_ops(cie.instructions, cie, where) + cfa_ops(data[j:end], cie, where)
            out[(target.index, offset)] = ops
        i = end
    return out


# Reading the stencils

class Reader:
    def __init__(self, elf: Elf) -> None:
        self.elf = elf
        self.problems: list[str] = []
        self.frames = frames(elf)
        # The support image: its items by (section, start), laid out in the
        # order of first use, then sorted (layout())
        self.support_wanted: dict[tuple[int, int], int] = {}   # -> size
        # The symbols that an instruction loads from a GOT, which the reader
        # cannot rewrite as a direct reference (cmp foo@GOTPCREL(%rip), %r):
        # each takes a slot at the end of the support image, as a linker's
        # GOT would
        self.got: dict[str, int] = {}
        self.got_base = 0
        # The symbols that a call or a jump names, each of which gets a PLT
        # entry in the support image (`jmp *slot(%rip)` through its GOT
        # slot), as a linker makes for a symbol of another object: the
        # emitter takes it when the symbol is out of rel32 reach of the code
        # (libc's, from the JIT's region near libpython)
        self.plt: dict[str, int] = {}
        self.jump_tables = self.find_jump_tables()

    def refuse(self, where: str, why: str) -> None:
        self.problems.append(f"{where}: {why}")

    def find_jump_tables(self) -> dict[int, list[tuple[int, int, int]]]:
        """The runs of relocations of each read-only data section into an
        arm's code: section -> [(start, end, arm section)]"""
        out: dict[int, list[tuple[int, int, int]]] = {}
        for sec in self.elf.sections:
            if (sec.flags & SHF_EXECINSTR or not sec.flags & SHF_ALLOC
                    or sec.name == ".eh_frame"):
                continue
            runs: list[tuple[int, int, int]] = []
            for r in self.elf.relocations.get(sec.index, []):
                target = self.elf.symbol_section(r.symbol)
                if target is None or not target.flags & SHF_EXECINSTR:
                    continue
                if (runs and runs[-1][1] == r.offset and runs[-1][2] == self.arm_of(target)
                        and r.type == Reloc.R_X86_64_PC32):
                    runs[-1] = (runs[-1][0], r.offset + 4, runs[-1][2])
                else:
                    runs.append((r.offset, r.offset + 4, self.arm_of(target)))
            if runs:
                out[sec.index] = runs
        return out

    def arm_of(self, sec: Section) -> int:
        """The hot section of the function whose part `sec` is"""
        name = sec.name.removeprefix(".text.unlikely.").removeprefix(".text.")
        hot = self.elf.by_name.get(".text." + name)
        return hot.index if hot is not None else sec.index

    def read(self) -> Table:
        decls = declarations(self.elf)
        functions = {s.name[len(".text." + STENCIL):]: s for s in self.elf.sections
                     if s.name.startswith(".text." + STENCIL)}
        stencils = []
        for name in decls.keys() | functions.keys():
            if name not in functions:
                self.refuse(name, "declared, but the object has no stencil for it")
            elif name not in decls:
                self.refuse(name, "a stencil with no declaration")
        for name, decl in decls.items():
            if name in functions and name not in GAPS:
                stencils.append(self.stencil(name, decl, functions[name]))
        support, holes, items = self.layout_support()
        # The GOT's slots come last: complete the holes that load them
        for h in [h for st in stencils for h in st.holes] + holes:
            if h.kind is Kind.SUPPORT and h.symbol.startswith(GOT):
                h.addend += self.got_base + 8 * self.got[h.symbol[len(GOT):]]
            elif h.kind is Kind.SUPPORT:
                h.addend += self.support_at[h.item]
        if self.problems:
            raise Refused(self.problems)
        return Table(stencils, support, holes, items, dict(self.plt))

    def stencil(self, name: str, decl, hot: Section) -> Stencil:
        # The emitter diverts a copy to the evaluator by a jmp rel32 over the
        # bytes after its endbr64 (Python/jit.c, divert_copy())
        if not hot.data.startswith(ENDBR64) or len(hot.data) < len(ENDBR64) + 5:
            self.refuse(name, "the hot part does not start with endbr64 and "
                        "five more bytes")
        cold = self.elf.by_name.get(".text.unlikely." + STENCIL + name)
        tables = [(sec, start, end) for sec, runs in self.jump_tables.items()
                  for start, end, arm in runs if arm == hot.index]
        data = bytearray()
        table_at: dict[tuple[int, int], int] = {}
        for sec, start, end in sorted(tables):
            while len(data) % 4:
                data.append(0)
            table_at[(sec, start)] = len(data)
            data += self.elf.sections[sec].data[start:end]
        parts: list[Part | None] = [Part(bytearray(hot.data)),
                                    Part(bytearray(cold.data)) if cold else None,
                                    Part(data) if data else None]
        sections = {hot.index: 0}
        if cold is not None:
            sections[cold.index] = 1
        for sec, i in sections.items():
            ops = self.frames.get((sec, 0))
            if ops is None:
                self.refuse(name, f"{PARTS[i]} part has no FDE")
            else:
                parts[i].cfa = ops
        holes: list[Hole] = []
        for sec, i in sections.items():
            for r in self.elf.relocations.get(sec, []):
                hole = self.code_hole(name, parts[i].data, i, r, sections, tables, table_at)
                if hole is not None:
                    holes.append(hole)
        for sec, start, end in tables:
            for r in self.elf.relocations[sec]:
                if start <= r.offset < end:
                    target = sections.get(self.elf.symbol_section(r.symbol).index)
                    if target is None:
                        self.refuse(name, f"jump table entry at {r.offset:#x} leaves the arm")
                        continue
                    holes.append(Hole(2, table_at[(sec, start)] + r.offset - start, Kind.PART,
                                      Patch.REL32, self.offset_in(r), target=target))
        ok = allowed(decl)
        for h in holes:
            if h.kind not in ok:
                self.refuse(name, f"hole {h.kind.name} at {PARTS[h.part]}+{h.offset:#x} "
                                  f"is not allowed by its declaration")
        holes.sort(key=lambda h: (h.part, h.offset))
        return Stencil(name, decl[0], decl[1], decl[2], parts, holes)

    def got_hole(self, part: int, r: Rela) -> Hole:
        """A load of the symbol's address from its GOT slot: a support
        hole, whose addend read() completes with the slot's place"""
        self.got.setdefault(r.symbol.name, len(self.got))
        return Hole(part, r.offset, Kind.SUPPORT, Patch.REL32, r.addend,
                    symbol=GOT + r.symbol.name)

    def extern_hole(self, code: bytearray, part: int, at: int, addend: int,
                    name: str) -> Hole:
        """A rel32 to a symbol of another object; a call or a jump also
        gets the symbol a PLT entry"""
        if is_call_or_branch(code, at):
            self.plt.setdefault(name, 0)
        return Hole(part, at, Kind.EXTERN, Patch.REL32, addend, name)

    def offset_in(self, r: Rela) -> int:
        """The relocation's addend, counted from the start of the symbol's
        section"""
        return r.addend + (0 if r.symbol.type == STT_SECTION else r.symbol.value)

    def code_hole(self, name: str, code: bytearray, part: int, r: Rela,
                  sections: dict[int, int], tables, table_at) -> Hole | None:
        where = f"{name} {PARTS[part]}+{r.offset:#x}"
        sym = r.symbol
        t = r.type
        if sym.name in HOLES:
            kind = HOLES[sym.name]
            if t not in (Reloc.R_X86_64_PC32, Reloc.R_X86_64_PLT32):
                self.refuse(where, f"{reloc_name(t)} against {sym.name}")
                return None
            if kind in FIELDS:
                if t != Reloc.R_X86_64_PC32 or not is_lea_rip(code, r.offset):
                    self.refuse(where, f"field hole {sym.name} outside a lea disp32(%rip)")
                    return None
                return Hole(part, r.offset, kind, Patch.VALUE32, r.addend)
            if not is_branch(code, r.offset):
                self.refuse(where, f"exit hole {sym.name} outside a jmp or jcc rel32")
                return None
            return Hole(part, r.offset, kind, Patch.REL32, r.addend)
        if sym.name.startswith(STENCIL):
            self.refuse(where, f"refers to another stencil, {sym.name}")
            return None
        if sym.name.startswith("_JIT_"):
            self.refuse(where, f"{sym.name} is not in the table of holes")
            return None
        target = self.elf.symbol_section(sym)
        if sym.bind != STB_LOCAL and sym.type != STT_SECTION:
            # A symbol of libpython, which this object may define too (the rows
            # that ceval_normal.c defines are the same functions there)
            if t in (Reloc.R_X86_64_PC32, Reloc.R_X86_64_PLT32):
                return self.extern_hole(code, part, r.offset, r.addend, sym.name)
            if t in (Reloc.R_X86_64_GOTPCRELX, Reloc.R_X86_64_REX_GOTPCRELX):
                relaxed = relax_gotpcrelx(code, r.offset)
                if relaxed is None:
                    return self.got_hole(part, r)
                at, _ = relaxed
                return self.extern_hole(code, part, at, r.addend, sym.name)
            if t == Reloc.R_X86_64_GOTPCREL:
                return self.got_hole(part, r)
            if t == Reloc.R_X86_64_GOTTPOFF and sym.type == STT_TLS:
                if not relax_gottpoff(code, r.offset):
                    self.refuse(where, f"GOTTPOFF against {sym.name} outside a 64-bit mov")
                    return None
                return Hole(part, r.offset, Kind.TPOFF, Patch.IMM32, 0, sym.name)
            self.refuse(where, f"{reloc_name(t)} against {sym.name}")
            return None
        if target is None:
            self.refuse(where, f"local symbol {sym.name!r} with no section")
            return None
        if t not in (Reloc.R_X86_64_PC32, Reloc.R_X86_64_PLT32):
            self.refuse(where, f"{reloc_name(t)} against {target.name}")
            return None
        offset = self.offset_in(r)
        if target.index in sections:
            return Hole(part, r.offset, Kind.PART, Patch.REL32, offset,
                        target=sections[target.index])
        for sec, start, end in tables:
            # A jump table's address: lea table(%rip), so the table starts
            # at the addend plus the 4 bytes of the displacement
            if sec == target.index and start <= offset + 4 < end:
                if not is_lea_rip(code, r.offset):
                    self.refuse(where, f"jump table reference outside a lea")
                    return None
                return Hole(part, r.offset, Kind.PART, Patch.REL32,
                            offset + table_at[(sec, start)] - start, target=2)
        item = self.support_item(where, target, offset + 4 if target.flags & SHF_EXECINSTR == 0 else 0, sym)
        if item is None:
            return None
        sec, start = item
        return Hole(part, r.offset, Kind.SUPPORT, Patch.REL32, offset - start,
                    symbol=f"{self.elf.sections[sec].name}+{start:#x}", item=(sec, start))

    def support_item(self, where: str, target: Section, guess: int,
                     sym: Symbol) -> tuple[int, int] | None:
        """The support item that a local reference reaches: a whole code or
        merged-constant section, or a sized object of another read-only
        section.  Its start is the start of the section, or of the object."""
        if target.flags & SHF_WRITE or not target.flags & SHF_ALLOC:
            self.refuse(where, f"refers to writable or unallocated data, {target.name}")
            return None
        if target.flags & SHF_EXECINSTR or target.flags & SHF_MERGE:
            self.want_support(target.index, 0, target.size)
            return target.index, 0
        objects = [s for s in self.elf.symbols
                   if s.shndx == target.index and s.type == STT_OBJECT
                   and s.value <= guess < s.value + max(s.size, 1)]
        if len(objects) != 1:
            self.refuse(where, f"refers to {target.name}+{guess:#x}, which no one "
                               "sized object covers")
            return None
        (obj,) = objects
        self.want_support(target.index, obj.value, obj.size)
        return target.index, obj.value

    def want_support(self, sec: int, start: int, size: int) -> None:
        if (sec, start) in self.support_wanted:
            return
        self.support_wanted[(sec, start)] = size
        # An item's own relocations reach further items
        for r in self.elf.relocations.get(sec, []):
            if not start <= r.offset < start + size:
                continue
            target = self.elf.symbol_section(r.symbol)
            if r.symbol.bind == STB_LOCAL and target is not None and target.index != sec:
                self.support_item(f"support {self.elf.sections[sec].name}+{r.offset:#x}",
                                  target, self.offset_in(r) + 4, r.symbol)

    def layout_support(self) -> tuple[Part, list[Hole], list[tuple[str, int, int]]]:
        image = bytearray()
        at: dict[tuple[int, int], int] = {}
        self.support_at = at
        items = []
        for (sec, start), size in sorted(self.support_wanted.items()):
            s = self.elf.sections[sec]
            align = max(s.align, 1) if start == 0 else 16
            while len(image) % align:
                image.append(0xCC if s.flags & SHF_EXECINSTR else 0)
            at[(sec, start)] = len(image)
            items.append((f"{s.name}+{start:#x}", len(image), size))
            image += s.data[start:start + size]
        part = Part(image)
        holes: list[Hole] = []
        for (sec, start), size in sorted(self.support_wanted.items()):
            s = self.elf.sections[sec]
            base = at[(sec, start)]
            if s.flags & SHF_EXECINSTR:
                ops = self.frames.get((sec, 0))
                if ops is None:
                    self.refuse(f"support {s.name}", "no FDE")
                else:
                    # One program for the whole image: each function starts
                    # from the CIE's rule (its ops begin with it), so the
                    # registers that the one before saved are restored first
                    part.cfa += cfa_boundary(part.cfa, base)
                    part.cfa += [dataclasses.replace(op, pc=op.pc + base) for op in ops]
            for r in self.elf.relocations.get(sec, []):
                if not start <= r.offset < start + size:
                    continue
                p = base + r.offset - start
                where = f"support {s.name}+{r.offset:#x}"
                sym = r.symbol
                target = self.elf.symbol_section(sym)
                if sym.name.startswith("_JIT_"):
                    self.refuse(where, f"a support item refers to the hole {sym.name}")
                elif sym.bind != STB_LOCAL and sym.type != STT_SECTION:
                    hole = self.code_hole_extern(where, image, p, r)
                    if hole is not None:
                        holes.append(hole)
                elif target is not None and r.type in (Reloc.R_X86_64_PC32,
                                                       Reloc.R_X86_64_PLT32):
                    # Within the image: resolved here, as a linker would
                    off = self.offset_in(r)
                    key = next(((ts, tst) for (ts, tst) in at
                                if ts == target.index and tst <= off + 4
                                < tst + self.support_wanted[(ts, tst)]), None)
                    if key is None and (target.index, 0) in at:
                        key = (target.index, 0)
                    if key is None:
                        self.refuse(where, f"refers to {target.name}+{off:#x} outside the image")
                        continue
                    value = at[key] + off - key[1] - p
                    struct.pack_into("<i", image, p, value)
                else:
                    self.refuse(where, f"{reloc_name(r.type)} against {sym.name or target.name}")
        # The GOT: one slot per symbol, which the emitter fills, the PLT's
        # symbols' too
        for name in self.plt:
            self.got.setdefault(name, len(self.got))
        while len(image) % 8:
            image.append(0)
        self.got_base = len(image)
        for name in self.got:
            holes.append(Hole(0, len(image), Kind.EXTERN, Patch.ABS64, 0, name))
            image += bytes(8)
        if self.got:
            items.append(("GOT", self.got_base, 8 * len(self.got)))
        # The PLT: jmp *slot(%rip), padded to 8 bytes, with the CIE's rule
        plt_base = len(image)
        part.cfa += cfa_boundary(part.cfa, plt_base)
        for name in self.plt:
            at = len(image)
            self.plt[name] = at
            slot = self.got_base + 8 * self.got[name]
            image += b"\xFF\x25" + struct.pack("<i", slot - (at + 6)) + b"\xCC\xCC"
        if self.plt:
            items.append(("PLT", plt_base, len(image) - plt_base))
        holes.sort(key=lambda h: h.offset)
        return part, holes, items

    def code_hole_extern(self, where: str, image: bytearray, p: int, r: Rela) -> Hole | None:
        t, sym = r.type, r.symbol
        if t in (Reloc.R_X86_64_PC32, Reloc.R_X86_64_PLT32):
            return self.extern_hole(image, 0, p, r.addend, sym.name)
        if t in (Reloc.R_X86_64_GOTPCRELX, Reloc.R_X86_64_REX_GOTPCRELX):
            relaxed = relax_gotpcrelx(image, p)
            if relaxed is not None:
                return self.extern_hole(image, 0, relaxed[0], r.addend, sym.name)
            return dataclasses.replace(self.got_hole(0, r), offset=p)
        if t == Reloc.R_X86_64_GOTPCREL:
            return dataclasses.replace(self.got_hole(0, r), offset=p)
        if t == Reloc.R_X86_64_GOTTPOFF and sym.type == STT_TLS and relax_gottpoff(image, p):
            return Hole(0, p, Kind.TPOFF, Patch.IMM32, 0, sym.name)
        self.refuse(where, f"{reloc_name(t)} against {sym.name}")
        return None


def cfa_boundary(ops: list[CfaOp], pc: int) -> list[CfaOp]:
    """The ops that end a program's last function at pc: the CFA and every
    register that it left saved back to the CIE's rule"""
    saved: set[int] = set()
    for op in ops:
        if op.op == 0x80:
            saved.add(op.reg)
        elif op.op == 0xC0:
            saved.discard(op.reg)
    if not ops:
        return []
    return [CfaOp(pc, 0x0C, 7, 8)] + [CfaOp(pc, 0xC0, reg) for reg in sorted(saved)]


def reloc_name(t: int) -> str:
    try:
        return Reloc(t).name
    except ValueError:
        return f"relocation type {t}"


def read_stencils(data: bytes) -> Table:
    return Reader(Elf(data)).read()


# The table

def symbols_of(table: Table) -> tuple[list[str], list[str]]:
    """The EXTERN and TPOFF symbols, each in first-use order"""
    externs: dict[str, None] = {}
    tls: dict[str, None] = {}
    for holes in [s.holes for s in table.stencils] + [table.support_holes]:
        for h in holes:
            if h.kind is Kind.EXTERN:
                externs.setdefault(h.symbol)
            elif h.kind is Kind.TPOFF:
                tls.setdefault(h.symbol)
    return list(externs), list(tls)


def c_bytes(out: io.StringIO, name: str, data: bytes) -> None:
    out.write(f"static const uint8_t {name}[{max(len(data), 1)}] = {{\n")
    for i in range(0, len(data), 16):
        out.write("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",\n")
    out.write("};\n")


def c_cfa(out: io.StringIO, name: str, ops: list[CfaOp]) -> None:
    out.write(f"static const _PyJitCfaOp {name}[{max(len(ops), 1)}] = {{\n")
    for op in ops:
        out.write(f"    {{{op.pc}, {op.op:#04x}, {op.reg}, {op.value}}},\n")
    out.write("};\n")


def c_holes(out: io.StringIO, name: str, holes: list[Hole],
            symbol_index: dict[str, int]) -> None:
    out.write(f"static const _PyJitHole {name}[{max(len(holes), 1)}] = {{\n")
    for h in holes:
        out.write(f"    {{{h.offset:#x}, {h.addend}, {symbol_index.get(h.symbol, 0)}, "
                  f"_PyJit_PART_{PARTS[h.part].upper()}, _PyJit_HOLE_{h.kind.name}, "
                  f"_PyJit_PATCH_{h.patch.name}, _PyJit_PART_{PARTS[h.target].upper()}}},\n")
    out.write("};\n")


def c_part(name: str, part: Part | None) -> str:
    if part is None:
        return "{NULL, 0, NULL, 0}"
    return f"{{{name}, {len(part.data)}, {name}_cfa, {len(part.cfa)}}}"


def write_table(table: Table, out: io.StringIO) -> None:
    out.write("// This file is generated by Tools/jit/stencils.py from\n"
              "// Python/jit_stencils.o.  Do not edit!\n\n"
              "#include \"pycore_jit_stencils.h\"\n\n")
    externs, tls = symbols_of(table)
    index = {name: i for i, name in enumerate(externs)}
    index |= {name: i for i, name in enumerate(tls)}
    out.write("// The symbols of libpython that the holes name\n")
    for i, name in enumerate(externs):
        out.write(f'extern char _PyJitSym_{i}[] __asm__("{name}");\n')
    for i, name in enumerate(tls):
        out.write(f'extern __thread char _PyJitTls_{i} __asm__("{name}") '
                  '__attribute__((visibility("hidden")));\n')
    out.write(f"\nstatic const void *const _PyJit_StencilSymbols[{max(len(externs), 1)}] = {{\n")
    for i, name in enumerate(externs):
        out.write(f"    _PyJitSym_{i},  // {name}\n")
    out.write("};\n")
    out.write("/* The offset of each symbol's PLT entry in the support image, or -1 */\n")
    out.write(f"static const int32_t _PyJit_StencilPlt[{max(len(externs), 1)}] = {{\n")
    for name in externs:
        out.write(f"    {table.plt.get(name, -1)},  // {name}\n")
    out.write("};\n")
    out.write(f"#define _PyJit_STENCIL_SYMBOL_COUNT {len(externs)}\n")
    out.write(f"#define _PyJit_STENCIL_TLS_COUNT {len(tls)}\n")
    out.write("/* The address of thread-local i in this thread, from which the\n"
              " * emitter derives its offset from the thread pointer */\n"
              "static inline void *\n_PyJit_StencilTls(int i)\n{\n    switch (i) {\n")
    for i, name in enumerate(tls):
        out.write(f"        case {i}: return &_PyJitTls_{i};  // {name}\n")
    out.write("    }\n    return NULL;\n}\n\n")

    out.write("// The support image\n")
    for name, off, size in table.support_items:
        out.write(f"//   {off:#07x} {size:#07x} {name}\n")
    c_bytes(out, "_PyJit_Support", table.support.data)
    c_cfa(out, "_PyJit_Support_cfa", table.support.cfa)
    c_holes(out, "_PyJit_SupportHoles", table.support_holes, index)
    out.write(f"static const _PyJitPart _PyJit_SupportPart = "
              f"{c_part('_PyJit_Support', table.support)};\n"
              f"#define _PyJit_SUPPORT_HOLES {len(table.support_holes)}\n\n")

    for s in table.stencils:
        out.write(f"// {s.name}\n")
        for i, part in enumerate(s.parts):
            if part is not None:
                c_bytes(out, f"_PyJit_{s.name}_{PARTS[i]}", part.data)
                c_cfa(out, f"_PyJit_{s.name}_{PARTS[i]}_cfa", part.cfa)
        c_holes(out, f"_PyJit_{s.name}_holes", s.holes, index)
        out.write("\n")
    out.write("static const _PyJitStencil _PyJit_Stencils[IR_NUM_HANDLERS] = {\n")
    for s in table.stencils:
        parts = ", ".join(c_part(f"_PyJit_{s.name}_{PARTS[i]}", p) for i, p in enumerate(s.parts))
        generic = f"ROP_HANDLER_{s.generic}" if s.generic else "-1"
        out.write(f"    [ROP_HANDLER_{s.name}] = {{\"{s.name}\", {{{parts}}}, "
                  f"_PyJit_{s.name}_holes, {len(s.holes)}, {generic}}},\n")
    out.write("};\n")
    if GAPS:
        out.write("\n// The arms with no stencil (Tools/jit/stencils.py, GAPS):\n")
        for name, why in sorted(GAPS.items()):
            out.write(f"//   {name}: {why}\n")


def table_text(table: Table) -> str:
    out = io.StringIO()
    write_table(table, out)
    return out.getvalue()


def stats_rows(table: Table) -> Iterator[dict[str, object]]:
    """Per arm: the bytes of each part and the holes by kind"""
    for s in table.stencils:
        row: dict[str, object] = {"arm": s.name}
        for i, p in enumerate(s.parts):
            row[f"{PARTS[i]}_bytes"] = len(p.data) if p else 0
        for k in Kind:
            row[k.name] = sum(1 for h in s.holes if h.kind is k)
        yield row


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("object", help="Python/jit_stencils.o")
    parser.add_argument("-o", "--output", required=True, help="the table to write")
    parser.add_argument("--stats", help="also write the per-arm CSV here")
    args = parser.parse_args(argv)
    with open(args.object, "rb") as f:
        data = f.read()
    try:
        table = read_stencils(data)
    except Refused as e:
        print(f"{args.object}: the stencils are refused:", file=sys.stderr)
        for p in e.problems:
            print(f"  {p}", file=sys.stderr)
        return 1
    text = table_text(table)
    with open(args.output, "w") as f:
        f.write(text)
    if args.stats:
        import csv
        rows = list(stats_rows(table))
        with open(args.stats, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0]))
            w.writeheader()
            w.writerows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
