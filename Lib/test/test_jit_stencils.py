"""Tests of the baseline JIT's stencils: Tools/jit/stencils.py reads the
interpreter's arms, compiled under the stencil bindings
(Python/jit_stencils.o in the build tree), into the table
Python/generated_jit_stencils.h."""

import platform
import re
import struct
import sys
import sysconfig
import unittest
from pathlib import Path

from test import test_tools
from test.support import captured_stderr, os_helper

test_tools.skip_if_missing("jit")
with test_tools.imports_under_tool("jit"):
    import stencils

ROOT = Path(test_tools.basepath)
BUILD = Path(sysconfig.get_config_var("abs_builddir") or ".")
OBJECT = BUILD / "Python" / "jit_stencils.o"
TABLE = BUILD / "Python" / "generated_jit_stencils.h"

if not (sys.platform == "linux" and platform.machine() == "x86_64"):
    raise unittest.SkipTest("the stencils are built for x86-64 Linux only")
if not OBJECT.exists():
    raise unittest.SkipTest(f"{OBJECT} is not built")

Kind = stencils.Kind


def object_bytes() -> bytes:
    return OBJECT.read_bytes()


def section_offsets(data: bytes) -> dict[str, int]:
    """The file offset of each section, by name"""
    e_shoff, = struct.unpack_from("<Q", data, 0x28)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x3A)
    headers = [struct.unpack_from("<IIQQQQ", data, e_shoff + i * e_shentsize)
               for i in range(e_shnum)]
    names = headers[e_shstrndx]
    out = {}
    for name, _, _, _, off, _ in headers:
        out[stencils.cstring(data[names[4]:names[4] + names[5]], name)] = off
    return out


def find_relocation(data: bytes, arm: str, hole: str) -> tuple[int, int]:
    """The file offset of the first relocation of the arm's hot part against
    the hole symbol, and that of the code it patches"""
    elf = stencils.Elf(data)
    offsets = section_offsets(data)
    text = ".text." + stencils.STENCIL + arm
    rela = offsets[".rela" + text]
    for i in range(0, elf.by_name[".rela" + text].size, 24):
        r_offset, r_info = struct.unpack_from("<QQ", data, rela + i)
        if elf.symbols[r_info >> 32].name == hole:
            return rela + i, offsets[text] + r_offset
    raise AssertionError(f"{arm} has no relocation against {hole}")


def arms_of_cases() -> set[str]:
    cases = (ROOT / "Python" / "generated_rop_cases.c.h").read_text()
    return set(re.findall(r"^\s*TARGET\((\w+)\)", cases, re.MULTILINE))


class TableTests(unittest.TestCase):
    """The table that the build wrote, against the object"""

    @classmethod
    def setUpClass(cls):
        cls.table = stencils.read_stencils(object_bytes())
        cls.by_name = {s.name: s for s in cls.table.stencils}

    def test_every_arm_has_a_stencil(self):
        arms = arms_of_cases()
        self.assertGreater(len(arms), 300)
        self.assertEqual(self.by_name.keys() | stencils.GAPS.keys(), arms)
        self.assertFalse(self.by_name.keys() & stencils.GAPS.keys())

    def test_holes_are_declared(self):
        for s in self.table.stencils:
            ok = stencils.allowed((s.generic, s.fields, s.exits))
            for h in s.holes:
                with self.subTest(arm=s.name, hole=h):
                    self.assertIn(h.kind, ok)
                    if h.kind in stencils.FIELDS:
                        self.assertIs(h.patch, stencils.Patch.VALUE32)
                        self.assertTrue(stencils.is_lea_rip(s.parts[h.part].data, h.offset))
                    elif h.kind.value in stencils.HOLES:
                        self.assertIs(h.patch, stencils.Patch.REL32)
                        self.assertTrue(stencils.is_branch(s.parts[h.part].data, h.offset))
                    self.assertIsNotNone(s.parts[h.part])
                    self.assertLessEqual(h.offset + 4, len(s.parts[h.part].data))

    def test_regenerates_byte_for_byte(self):
        text = stencils.table_text(self.table)
        self.assertEqual(text, TABLE.read_text())
        self.assertEqual(stencils.table_text(stencils.read_stencils(object_bytes())), text)

    def test_cold_parts(self):
        cold = [s for s in self.table.stencils if s.parts[1] is not None]
        self.assertGreater(len(cold), len(self.table.stencils) * 9 // 10)

    @unittest.skipUnless("-DNDEBUG" in (sysconfig.get_config_var("OPT") or ""),
                         "with assertions, GCC keeps some arms' shared miss tails hot")
    def test_misses_are_cold(self):
        # A miss leaves from the cold part of every arm that has a generic
        for s in self.table.stencils:
            for h in s.holes:
                if h.kind is Kind.MISS:
                    with self.subTest(arm=s.name, hole=h):
                        self.assertEqual(h.part, 1)

    def test_three_arms(self):
        add = self.by_name["Add_Int_TLL"]
        self.assertEqual(add.generic, "Add_Generic")
        kinds = {h.kind for h in add.holes}
        self.assertLessEqual({Kind.FIELD_A, Kind.FIELD_B, Kind.FIELD_C, Kind.CONTINUE,
                              Kind.MISS}, kinds)
        poll = self.by_name["Jump_Poll"]
        self.assertIsNone(poll.generic)
        self.assertIn(Kind.TARGET, {h.kind for h in poll.holes})
        self.assertNotIn(Kind.CONTINUE, {h.kind for h in poll.holes})

    def test_jump_table(self):
        s = self.by_name["Binary_Int"]
        data = s.parts[2]
        self.assertIsNotNone(data)
        entries = [h for h in s.holes if h.part == 2]
        self.assertTrue(entries)
        self.assertTrue(all(h.kind is Kind.PART and h.patch is stencils.Patch.REL32
                            for h in entries))
        self.assertEqual(len(data.data), 4 * len(entries))
        # The code reaches the table by a lea of the data part
        self.assertTrue(any(h.kind is Kind.PART and h.target == 2 and h.part != 2
                            for h in s.holes))

    def test_jump_table_spans_hot_and_cold(self):
        # GCC keeps Binary_Int's cases hot; point one entry at the cold part,
        # as it may when it splits a case off
        data = bytearray(object_bytes())
        elf = stencils.Elf(bytes(data))
        cold = elf.by_name[".text.unlikely." + stencils.STENCIL + "Binary_Int"]
        cold_symbol = next(i for i, sym in enumerate(elf.symbols)
                           if sym.type == stencils.STT_SECTION and sym.shndx == cold.index)
        hot = elf.by_name[".text." + stencils.STENCIL + "Binary_Int"]
        sec, start = next((sec, start)
                          for sec, runs in stencils.Reader(elf).find_jump_tables().items()
                          for start, _, arm in runs if arm == hot.index)
        rela_name = ".rela" + elf.sections[sec].name
        rela = section_offsets(data)[rela_name]
        for i in range(rela, rela + elf.by_name[rela_name].size, 24):
            r_offset, r_info = struct.unpack_from("<QQ", data, i)
            if r_offset == start:
                struct.pack_into("<Qq", data, i + 8,
                                 (cold_symbol << 32) | (r_info & 0xFFFFFFFF), 8)
                break
        else:
            self.fail("no relocation of the table's first entry")
        s = {s.name: s for s in stencils.read_stencils(bytes(data)).stencils}["Binary_Int"]
        entries = [h for h in s.holes if h.part == 2]
        self.assertEqual({h.target for h in entries}, {0, 1})
        moved = [h for h in entries if h.target == 1]
        self.assertEqual(len(moved), 1)
        # The entry is the cold target less the entry's place in the table
        self.assertEqual(moved[0].addend, 8)

    def test_support_image(self):
        names = [name for name, _, _ in self.table.support_items]
        self.assertEqual(len(names), len(set(names)))
        for h in self.table.support_holes:
            self.assertIn(h.kind, (Kind.EXTERN, Kind.TPOFF))
        size = len(self.table.support.data)
        for s in self.table.stencils:
            for h in s.holes:
                if h.kind is Kind.SUPPORT:
                    with self.subTest(arm=s.name, hole=h):
                        self.assertTrue(-8 <= h.addend < size + 8)


class RefusalTests(unittest.TestCase):
    """The reader refuses what it cannot bind, and the build fails"""

    def assertRefused(self, data: bytes, *words: str):
        with self.assertRaises(stencils.Refused) as cm:
            stencils.read_stencils(data)
        text = "\n".join(cm.exception.problems)
        for word in words:
            self.assertIn(word, text)

    def test_unknown_relocation_kind(self):
        data = bytearray(object_bytes())
        rela, _ = find_relocation(data, "Add_Int_TLL", "_JIT_A")
        r_info, = struct.unpack_from("<Q", data, rela + 8)
        struct.pack_into("<Q", data, rela + 8,
                         (r_info & ~0xFFFFFFFF) | stencils.Reloc.R_X86_64_64)
        self.assertRefused(bytes(data), "Add_Int_TLL", "R_X86_64_64", "_JIT_A")

    def test_field_outside_lea(self):
        data = bytearray(object_bytes())
        _, at = find_relocation(data, "Add_Int_TLL", "_JIT_B")
        self.assertEqual(data[at - 2], 0x8D)
        data[at - 2] = 0x8B         # mov _JIT_B(%rip), not lea
        self.assertRefused(bytes(data), "Add_Int_TLL", "outside a lea")

    def test_undeclared_hole(self):
        # Jump_Poll declares no miss: a MISS hole is not allowed in it
        data = bytearray(object_bytes())
        elf = stencils.Elf(bytes(data))
        rela, _ = find_relocation(data, "Jump_Poll", "_JIT_TARGET")
        miss = next(i for i, sym in enumerate(elf.symbols) if sym.name == "_JIT_MISS")
        r_info, = struct.unpack_from("<Q", data, rela + 8)
        struct.pack_into("<Q", data, rela + 8, (miss << 32) | (r_info & 0xFFFFFFFF))
        self.assertRefused(bytes(data), "Jump_Poll", "MISS", "not allowed")

    def test_main_fails(self):
        data = bytearray(object_bytes())
        data[:4] = b"\x7fXLF"
        with os_helper.temp_dir() as tmp:
            bad = Path(tmp) / "bad.o"
            bad.write_bytes(data)
            with captured_stderr() as err:
                rc = stencils.main([str(bad), "-o", str(Path(tmp) / "t.h")])
            self.assertEqual(rc, 1)
            self.assertIn("refused", err.getvalue())
            self.assertFalse((Path(tmp) / "t.h").exists())


class RelaxationTests(unittest.TestCase):
    """The rewrites of GOT and TLS loads, as ld makes them"""

    def test_gotpcrelx(self):
        cases = [
            # mov foo@GOTPCREL(%rip), %rax -> lea foo(%rip), %rax
            (b"\x48\x8b\x05", b"\x48\x8d\x05", 3),
            # call *foo@GOTPCREL(%rip) -> addr32 call foo
            (b"\xff\x15", b"\x67\xe8", 2),
            # jmp *foo@GOTPCREL(%rip) -> jmp foo; nop
            (b"\xff\x25", b"\xe9", 1),
        ]
        for before, after, at in cases:
            with self.subTest(before=before):
                code = bytearray(before + bytes(4))
                where = stencils.relax_gotpcrelx(code, len(before))
                self.assertEqual(where, (at, 0))
                self.assertEqual(bytes(code[:len(after)]), after)
                self.assertEqual(len(code), len(before) + 4)
        # cmp foo@GOTPCREL(%rip), %rcx needs the address in memory: a GOT slot
        code = bytearray(b"\x48\x3b\x0d" + bytes(4))
        self.assertIsNone(stencils.relax_gotpcrelx(code, 3))

    def test_gottpoff(self):
        # mov foo@GOTTPOFF(%rip), %r12 -> mov $foo@TPOFF, %r12
        code = bytearray(b"\x4c\x8b\x25" + bytes(4))
        self.assertTrue(stencils.relax_gottpoff(code, 3))
        self.assertEqual(bytes(code[:3]), b"\x49\xc7\xc4")
        code = bytearray(b"\x48\x8b\x05" + bytes(4))
        self.assertTrue(stencils.relax_gottpoff(code, 3))
        self.assertEqual(bytes(code[:3]), b"\x48\xc7\xc0")
        # add foo@GOTTPOFF(%rip), %rax is not a mov
        self.assertFalse(stencils.relax_gottpoff(bytearray(b"\x48\x03\x05" + bytes(4)), 3))

    def test_got_slot_in_object(self):
        # IterOpen_Generic compares a type with a GOT slot's address
        table = stencils.read_stencils(object_bytes())
        got = [h for s in table.stencils for h in s.holes
               if h.kind is Kind.SUPPORT and h.symbol.startswith(stencils.GOT)]
        self.assertTrue(got)
        slots = [h for h in table.support_holes if h.patch is stencils.Patch.ABS64]
        self.assertEqual(len(slots), len({h.symbol for h in got}))


if __name__ == "__main__":
    unittest.main()
