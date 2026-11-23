"""Tests of the JIT's x86-64 assembler and code memory
(Include/internal/pycore_jit_asm.h, Python/jit_asm.c, Python/jit_memory.c).

A program is a list of tuples, ("name", argument...), run by the hooks in
Modules/_testinternalcapi/jit_asm.c.  An argument is an int (register,
condition, immediate, label number) or a memory operand (base, disp) or
(base, index, scale, disp).
"""

import itertools
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from test import support
from test.support import import_helper, warnings_helper

_testinternalcapi = import_helper.import_module("_testinternalcapi")
if not hasattr(_testinternalcapi, "jit_asm_place"):
    raise unittest.SkipTest("the JIT assembler is not supported on this build")

from _testinternalcapi import (
    jit_asm_assemble, jit_asm_place, jit_asm_call, jit_asm_address,
    jit_memory_info, jit_memory_alloc, jit_memory_free)


REGS64 = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]
REGS32 = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
          "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"]
REGS16 = ["ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
          "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"]
REGS8 = ["al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
         "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"]
RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15 = range(16)
CONDS = ["o", "no", "b", "ae", "e", "ne", "be", "a",
         "s", "ns", "p", "np", "l", "ge", "le", "g"]
(O, NO, B, AE, E, NE, BE, A, S, NS, P, NP, L, GE, LE, G) = range(16)
ALU = {"add": 0, "or": 1, "and": 4, "sub": 5, "xor": 6, "cmp": 7}
UNARY = {"not": 0, "neg": 1, "inc": 2, "dec": 3}
SHIFT = {"shl": 4, "shr": 5, "sar": 7}
SSE = {"addsd": 0, "subsd": 1, "mulsd": 2, "divsd": 3, "sqrtsd": 4, "ucomisd": 5}

IMM32 = [0, 1, -1, 127, -128, 128, -129, 2**31 - 1, -2**31]
DISPS = [0, 1, -1, 127, -128, 128, -129, 2**31 - 1, -2**31]
BASES = [RAX, RCX, RSP, RBP, RSI, R8, R12, R13, R15]
INDEXES = [RAX, RBP, R8, R12, R15]   # rsp cannot be an index

ORIGIN = 0x10000
TARGET = ORIGIN + 0x4321             # a call target within rel32's reach


def _mem_text(mem):
    if len(mem) == 2:
        base, disp = mem
        inner = REGS64[base]
    else:
        base, index, scale, disp = mem
        inner = f"{REGS64[base]} + {REGS64[index]}*{1 << scale}"
    if disp:
        inner += f" + {disp}" if disp > 0 else f" - {-disp}"
    return f"[{inner}]"


MEMS = (
    [(b, d) for b in BASES for d in DISPS]
    + [(b, i, s, d) for b in BASES[::2] for i in INDEXES for s in range(4)
       for d in (0, 1, 128)]
)


# The dimensions of an instruction form's operands.  Each is a list of
# (our argument, the assembly text).
def _regs(names):
    return [(i, name) for i, name in enumerate(names)]


DIMS = {
    "r": _regs(REGS64),
    "r32": _regs(REGS32),
    "r8": _regs(REGS8),
    "r16": _regs(REGS16),
    "x": [(i, f"xmm{i}") for i in range(16)],
    "m": [(m, _mem_text(m)) for m in MEMS],
    "i": [(i, str(i)) for i in IMM32],
    "i8": [(i, str(i)) for i in (0, 1, -1, 127, -128, 128, 255)],
    "i16": [(i, str(i)) for i in (0, 1, -1, 127, -128, 128, 255, 256, 0x7fff,
                                   0x8000, -0x8000, 0xffff)],
    "c": _regs(CONDS),
    "shift": [(i, str(i)) for i in (0, 1, 2, 7, 31, 32, 63)],
    "alu": [(v, k) for k, v in ALU.items()],
    "unary": [(v, k) for k, v in UNARY.items()],
    "shiftop": [(v, k) for k, v in SHIFT.items()],
    "sse": [(v, k) for k, v in SSE.items()],
}


def _cases(kinds):
    """Operand combinations of a form: all of them if there are few, else
    every combination of the two largest dimensions with the others cycled,
    so that every value of every dimension is tried."""
    dims = [DIMS[kind] for kind in kinds]
    total = math.prod(len(d) for d in dims)
    if total <= 4000:
        yield from itertools.product(*dims)
        return
    order = sorted(range(len(dims)), key=lambda k: -len(dims[k]))
    big = order[:2]
    for n, picked in enumerate(itertools.product(*(dims[k] for k in big))):
        row = [None] * len(dims)
        for k, value in zip(big, picked):
            row[k] = value
        for k in order[2:]:
            row[k] = dims[k][n % len(dims[k])]
        yield tuple(row)


# name: (our op name, operand kinds, assembly template).  {0}.. are the
# operand texts; the operands are our arguments in the same order, except
# that the opcode group (alu, unary, ...) is the first of our arguments and
# is named in the template as {op}.
FORMS = {
    "mov_rr": ("mov_rr", "r r", "mov {0}, {1}"),
    "mov_rm": ("mov_rm", "r m", "mov {0}, qword ptr {1}"),
    "mov_mr": ("mov_mr", "m r", "mov qword ptr {0}, {1}"),
    "mov_mi": ("mov_mi", "m i", "mov qword ptr {0}, {1}"),
    "mov_rm32": ("mov_rm32", "r32 m", "mov {0}, dword ptr {1}"),
    "mov_mr32": ("mov_mr32", "m r32", "mov dword ptr {0}, {1}"),
    "mov_mi32": ("mov_mi32", "m i", "mov dword ptr {0}, {1}"),
    "mov_mr8": ("mov_mr8", "m r8", "mov byte ptr {0}, {1}"),
    "mov_mi8": ("mov_mi8", "m i8", "mov byte ptr {0}, {1}"),
    "mov_mr16": ("mov_mr16", "m r16", "mov word ptr {0}, {1}"),
    "mov_mi16": ("mov_mi16", "m i16", "mov word ptr {0}, {1}"),
    "movzx_rm8": ("movzx_rm8", "r32 m", "movzx {0}, byte ptr {1}"),
    "movzx_rm16": ("movzx_rm16", "r32 m", "movzx {0}, word ptr {1}"),
    "movsx_rm8": ("movsx_rm8", "r m", "movsx {0}, byte ptr {1}"),
    "movsx_rm16": ("movsx_rm16", "r m", "movsx {0}, word ptr {1}"),
    "movsx_rm32": ("movsx_rm32", "r m", "movsxd {0}, dword ptr {1}"),
    "lea": ("lea", "r m", "lea {0}, {1}"),
    "alu_rr": ("alu_rr", "alu r r", "{0} {1}, {2}"),
    "alu_rm": ("alu_rm", "alu r m", "{0} {1}, qword ptr {2}"),
    "alu_mr": ("alu_mr", "alu m r", "{0} qword ptr {1}, {2}"),
    "alu_ri": ("alu_ri", "alu r i", "{0} {1}, {2}"),
    "alu_mi": ("alu_mi", "alu m i", "{0} qword ptr {1}, {2}"),
    "alu_mi32": ("alu_mi32", "alu m i", "{0} dword ptr {1}, {2}"),
    "test_rr": ("test_rr", "r r", "test {0}, {1}"),
    "test_mr": ("test_mr", "m r", "test qword ptr {0}, {1}"),
    "test_ri": ("test_ri", "r i", "test {0}, {1}"),
    "test_mi": ("test_mi", "m i", "test qword ptr {0}, {1}"),
    "imul_rr": ("imul_rr", "r r", "imul {0}, {1}"),
    "imul_rm": ("imul_rm", "r m", "imul {0}, qword ptr {1}"),
    "imul_rri": ("imul_rri", "r r i", "imul {0}, {1}, {2}"),
    "unary_r": ("unary_r", "unary r", "{0} {1}"),
    "unary_m": ("unary_m", "unary m", "{0} qword ptr {1}"),
    "unary_m32": ("unary_m32", "unary m", "{0} dword ptr {1}"),
    "shift_ri": ("shift_ri", "shiftop r shift", "{0} {1}, {2}"),
    "shift_rcl": ("shift_rcl", "shiftop r", "{0} {1}, cl"),
    "setcc": ("setcc", "c r8", "set{0} {1}"),
    "cmov_rr": ("cmov_rr", "c r r", "cmov{0} {1}, {2}"),
    "cmov_rm": ("cmov_rm", "c r m", "cmov{0} {1}, qword ptr {2}"),
    "jmp_r": ("jmp_r", "r", "jmp {0}"),
    "call_r": ("call_r", "r", "call {0}"),
    "jmp_m": ("jmp_m", "m", "jmp qword ptr {0}"),
    "call_m": ("call_m", "m", "call qword ptr {0}"),
    "push": ("push", "r", "push {0}"),
    "pop": ("pop", "r", "pop {0}"),
    "movsd_rr": ("movsd_rr", "x x", "movsd {0}, {1}"),
    "movsd_rm": ("movsd_rm", "x m", "movsd {0}, qword ptr {1}"),
    "movsd_mr": ("movsd_mr", "m x", "movsd qword ptr {0}, {1}"),
    "movq_xr": ("movq_xr", "x r", "movq {0}, {1}"),
    "movq_rx": ("movq_rx", "r x", "movq {0}, {1}"),
    "sse_rr": ("sse_rr", "sse x x", "{0} {1}, {2}"),
    "sse_rm": ("sse_rm", "sse x m", "{0} {1}, qword ptr {2}"),
    "cvtsi2sd": ("cvtsi2sd", "x r", "cvtsi2sd {0}, {1}"),
    "cvttsd2si": ("cvttsd2si", "r x", "cvttsd2si {0}, {1}"),
}
# Forms whose first operand kind names an opcode group put the group's
# value first in our arguments and its name in the template, like the rest.


def _form_cases(form):
    """[(our op, assembly line)] for every case of a form."""
    name, kinds, template = FORMS[form]
    cases = []
    for row in _cases(kinds.split()):
        args = []
        for arg in row:
            args.append(arg[0])
        op = (name, *args)
        cases.append((op, template.format(*(arg[1] for arg in row))))
    return cases


def _mov_imm_cases():
    cases = []
    for reg in range(16):
        for imm in (0, 1, 127, 128, 2**31 - 1, 2**31, 2**32 - 1):
            cases.append((("mov_imm", reg, imm), f"mov {REGS32[reg]}, {imm}"))
        for imm in (-1, -128, -129, -2**31):
            cases.append((("mov_imm", reg, imm), f"mov {REGS64[reg]}, {imm}"))
        for imm in (2**32, 2**63 - 1, -2**31 - 1, -2**63, 0x1122334455667788):
            cases.append((("mov_imm", reg, imm), f"movabs {REGS64[reg]}, {imm}"))
    return cases


def _simple_cases():
    return [(("ret",), "ret")]


def _branch_program():
    """Jumps in both directions, a call and a table of label addresses,
    in one program; returns (ops, assembly lines)."""
    ops, lines = [], []
    def emit(op, line):
        ops.append(op)
        lines.append(line)
    emit(("bind", 0), ".L0:")
    for cond in range(16):
        emit(("jcc", cond, 1), f"{{disp32}} j{CONDS[cond]} .L1")
    emit(("jmp", 1), "{disp32} jmp .L1")
    emit(("bind", 1), ".L1:")
    for cond in range(16):
        emit(("jcc", cond, 0), f"{{disp32}} j{CONDS[cond]} .L0")
    emit(("jmp", 0), "{disp32} jmp .L0")
    emit(("jmp", 2), "{disp32} jmp .L2")
    emit(("bind", 2), ".L2:")
    for reg in range(16):
        emit(("lea_label", reg, 0), f"lea {REGS64[reg]}, [rip + .L0]")
        emit(("lea_label", reg, 3), f"lea {REGS64[reg]}, [rip + .L3]")
    emit(("call_abs", TARGET), f"call S + {TARGET - ORIGIN}")
    emit(("call_abs", ORIGIN - 0x1000), f"call S + {-0x1000}")
    emit(("align", 8), ".balign 8, 0xcc")
    emit(("bind", 3), ".L3:")
    emit(("quad_label", 0), f".quad .L0 - S + {ORIGIN}")
    emit(("quad_label", 3), f".quad .L3 - S + {ORIGIN}")
    emit(("ret",), "ret")
    return ops, lines


@support.requires_subprocess()
@unittest.skipUnless(shutil.which("as") and shutil.which("objcopy"),
                     "GNU as and objcopy are needed")
class EncodingTests(unittest.TestCase):
    """Every instruction form, byte for byte against GNU as."""

    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.mkdtemp()
        cls.cases_run = 0
        cls.forms_run = 0

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tmp)
        print(f"\n{cls.forms_run} forms, {cls.cases_run} cases",
              file=sys.stderr)

    def gas(self, lines):
        source = os.path.join(self.tmp, "x.s")
        obj = os.path.join(self.tmp, "x.o")
        binary = os.path.join(self.tmp, "x.bin")
        with open(source, "w") as f:
            f.write(".intel_syntax noprefix\n.text\nS:\n")
            f.write("\n".join(lines) + "\n")
        subprocess.run(["as", "--64", "-o", obj, source], check=True,
                       stderr=subprocess.PIPE, text=True)
        subprocess.run(["objcopy", "-O", "binary", "-j", ".text", obj, binary],
                       check=True)
        with open(binary, "rb") as f:
            return f.read()

    def compare(self, cases, origin=ORIGIN):
        ops = [op for op, _ in cases]
        lines = [line for _, line in cases]
        ours = jit_asm_assemble(ops, origin)
        theirs = self.gas(lines)
        EncodingTests.forms_run += 1
        EncodingTests.cases_run += len(cases)
        if ours == theirs:
            return
        # Find the first case whose bytes differ, one at a time.
        for op, line in cases:
            ours = jit_asm_assemble([op], origin)
            theirs = self.gas([line])
            self.assertEqual(ours.hex(" "), theirs.hex(" "),
                             f"{op} against {line!r}")
        self.fail("streams differ but no single case does")

    def test_forms(self):
        for form in FORMS:
            with self.subTest(form=form):
                self.compare(_form_cases(form))

    def test_mov_imm(self):
        self.compare(_mov_imm_cases())

    def test_ret(self):
        self.compare(_simple_cases())

    def test_labels_calls_and_data(self):
        ops, lines = _branch_program()
        self.compare(list(zip(ops, lines)))

    def test_register_coverage(self):
        # The tables reach every register as every kind of operand.
        self.assertGreater(len(FORMS), 40)
        for kind in ("r", "r32", "r8", "x"):
            self.assertEqual(len(DIMS[kind]), 16)
        bases = {m[0] for m in MEMS}
        self.assertTrue({RSP, RBP, R12, R13, R15} <= bases)


def run(program, ints=(), doubles=(), returns_double=False):
    """Place a program, call it as a C function, free it."""
    placed = jit_asm_place(program)
    assert placed is not None
    address, size = placed
    try:
        return jit_asm_call(address, tuple(ints), tuple(doubles), returns_double)
    finally:
        jit_memory_free(address, size)


def bits(value):
    return value & (2**64 - 1)


def signed(value):
    value = bits(value)
    return value - 2**64 if value >= 2**63 else value


class ExecutionTests(unittest.TestCase):

    def test_integer_arithmetic(self):
        # (a + b) * c - 7
        program = [
            ("mov_rr", RAX, RDI), ("alu_rr", ALU["add"], RAX, RSI),
            ("imul_rr", RAX, RDX), ("alu_ri", ALU["sub"], RAX, 7), ("ret",),
        ]
        self.assertEqual(run(program, (5, 6, 7)), 70)
        self.assertEqual(run(program, (-5, 2, 1000)), -3007)

    def test_logic_and_shifts(self):
        program = [
            ("mov_rr", RAX, RDI),
            ("alu_rr", ALU["xor"], RAX, RSI),
            ("alu_ri", ALU["and"], RAX, 0xffff),
            ("shift_ri", SHIFT["shl"], RAX, 4),
            ("mov_rr", RCX, RDX),
            ("shift_rcl", SHIFT["sar"], RAX),
            ("unary_r", UNARY["neg"], RAX),
            ("unary_r", UNARY["inc"], RAX),
            ("ret",),
        ]
        for a, b, c in [(0x1234, 0x00ff, 3), (-1, 1, 63), (0xabcdef, 0, 0)]:
            expected = (((a ^ b) & 0xffff) << 4) >> c
            self.assertEqual(run(program, (a, b, c)), -expected + 1)

    def test_loop(self):
        # sum of 1..n
        program = [
            ("mov_imm", RAX, 0),
            ("bind", 0),
            ("test_rr", RDI, RDI),
            ("jcc", E, 1),
            ("alu_rr", ALU["add"], RAX, RDI),
            ("alu_ri", ALU["sub"], RDI, 1),
            ("jmp", 0),
            ("bind", 1),
            ("ret",),
        ]
        self.assertEqual(run(program, (0,)), 0)
        self.assertEqual(run(program, (10,)), 55)
        self.assertEqual(run(program, (100000,)), 5000050000)

    def test_compare_setcc_cmov(self):
        less = [
            ("alu_rr", ALU["xor"], RAX, RAX),
            ("alu_rr", ALU["cmp"], RDI, RSI), ("setcc", L, RAX), ("ret",),
        ]
        maximum = [
            ("mov_rr", RAX, RDI), ("alu_rr", ALU["cmp"], RDI, RSI),
            ("cmov_rr", L, RAX, RSI), ("ret",),
        ]
        for a, b in [(1, 2), (2, 1), (3, 3), (-5, 4), (4, -5)]:
            self.assertEqual(run(less, (a, b)), int(a < b))
            self.assertEqual(run(maximum, (a, b)), max(a, b))

    def test_memory_operands_on_the_stack(self):
        # Store 64 bits below rsp (the red zone) and read parts of them.
        def reader(*load):
            return [("mov_mr", (RSP, -8), RDI), load, ("ret",)]
        value = 0x8081_8283_8485_8687
        for load, expected in [
            (("mov_rm", RAX, (RSP, -8)), signed(value)),
            (("mov_rm32", RAX, (RSP, -8)), value & 0xffffffff),
            (("movzx_rm8", RAX, (RSP, -8)), value & 0xff),
            (("movzx_rm16", RAX, (RSP, -8)), value & 0xffff),
            (("movsx_rm8", RAX, (RSP, -8)), -121),
            (("movsx_rm16", RAX, (RSP, -8)), struct.unpack("<h", struct.pack("<H", value & 0xffff))[0]),
            (("movsx_rm32", RAX, (RSP, -8)), struct.unpack("<i", struct.pack("<I", value & 0xffffffff))[0]),
            (("movzx_rm8", RAX, (RSP, -7)), (value >> 8) & 0xff),
        ]:
            with self.subTest(load=load):
                self.assertEqual(run(reader(*load), (signed(value),)), expected)

    def test_store_forms_and_lea(self):
        # The 32-bit store at -12 overwrites the high half of the qword at -16.
        program = [
            ("mov_mi", (RSP, -16), -2),
            ("mov_mr32", (RSP, -12), RDI),
            ("mov_rm", RAX, (RSP, -16)),
            ("lea", RCX, (RSI, RDX, 3, 5)),
            ("alu_rr", ALU["add"], RAX, RCX),
            ("ret",),
        ]
        a, b, c = 0x11, 1000, 20
        self.assertEqual(run(program, (a, b, c)),
                         ((a << 32) | 0xfffffffe) + b + 8 * c + 5)
        program = [
            ("mov_mi32", (RSP, -8), 7), ("mov_mi32", (RSP, -4), -1),
            ("mov_rm", RAX, (RSP, -8)), ("ret",),
        ]
        self.assertEqual(run(program), signed((0xffffffff << 32) | 7))

    def test_refcount_style_memory_updates(self):
        # 32-bit read-modify-write of the dword at [rsp-8]; the dword above
        # it must stay 7.
        program = [
            ("mov_mi32", (RSP, -4), 7),
            ("mov_mi32", (RSP, -8), 100),
            ("alu_mi32", ALU["add"], (RSP, -8), 5),
            ("alu_mi32", ALU["sub"], (RSP, -8), 1000),
            ("alu_mi32", ALU["add"], (RSP, -8), 100000),
            ("unary_m32", UNARY["inc"], (RSP, -8)),
            ("unary_m32", UNARY["dec"], (RSP, -8)),
            ("unary_m32", UNARY["inc"], (RSP, -8)),
            ("mov_imm", RCX, 2),
            ("alu_mi32", ALU["cmp"], (RSP, RCX, 2, -16), 99106),   # [rsp-8]
            ("alu_rr", ALU["xor"], RAX, RAX),
            ("setcc", E, RAX),
            ("mov_rm", RDX, (RSP, -8)),
            ("shift_ri", SHIFT["shl"], RAX, 63),
            ("alu_rr", ALU["or"], RAX, RDX),
            ("ret",),
        ]
        # 100 + 5 - 1000 + 100000 + 1 - 1 + 1 = 99106, and cmp finds it equal.
        self.assertEqual(run(program), signed((1 << 63) | (7 << 32) | 99106))
        # A decrement through zero wraps within 32 bits.
        program = [
            ("mov_mi32", (RSP, -4), 7),
            ("mov_mi32", (RSP, -8), 0),
            ("unary_m32", UNARY["dec"], (RSP, -8)),
            ("mov_rm", RAX, (RSP, -8)),
            ("ret",),
        ]
        self.assertEqual(run(program), (7 << 32) | 0xffffffff)

    def test_narrow_stores_leave_the_neighbours(self):
        # Fill 8 bytes with 0xff, store narrow pieces, read the qword back.
        def result(*stores, args):
            program = [("mov_mi", (RSP, -8), -1), *stores,
                       ("mov_rm", RAX, (RSP, -8)), ("ret",)]
            return run(program, args)

        def expect(*pieces):
            data = bytearray(b"\xff" * 8)
            for offset, size, value in pieces:
                data[offset:offset + size] = (value & (256**size - 1)).to_bytes(size, "little")
            return struct.unpack("<q", data)[0]

        # sil and dil are stored as themselves, not as dh and bh.
        self.assertEqual(
            result(("mov_mr8", (RSP, -8), RDI),
                   ("mov_mr8", (RSP, -6), RSI),
                   ("mov_imm", R9, 0xc3),
                   ("mov_mr8", (RSP, -5), R9),
                   ("mov_mi8", (RSP, -3), 0x5a),
                   ("mov_mi8", (RSP, -1), -2),
                   args=(0x1122334455667788, 0x99aa)),
            expect((0, 1, 0x88), (2, 1, 0xaa), (3, 1, 0xc3), (5, 1, 0x5a),
                   (7, 1, 0xfe)))
        # 16-bit stores, one through an index register (rsp + 2*2 - 10).
        self.assertEqual(
            result(("mov_mr16", (RSP, -8), RDI),
                   ("mov_mi16", (RSP, -4), 0xbeef),
                   ("mov_imm", RCX, 2),
                   ("mov_mr16", (RSP, RCX, 1, -10), RSI),
                   args=(0x1122334455667788, 0x99aa)),
            expect((0, 2, 0x7788), (2, 2, 0x99aa), (4, 2, 0xbeef)))

    def test_call_c_function_rel32(self):
        # call rel32 of a function in the interpreter's text, with the stack
        # aligned for the call.
        target = jit_asm_address("Py_HashPointer")
        program = [("push", RBP), ("call_abs", target), ("pop", RBP), ("ret",)]
        for pointer in (0x1000, 0x7f0000001230, 0xdeadbeef8):
            rotated = bits((pointer >> 4) | (pointer << 60))
            self.assertEqual(run(program, (pointer,)), signed(rotated))

    def test_call_out_of_reach_is_an_error(self):
        program = [("call_abs", 2**62), ("ret",)]
        self.assertIsNone(jit_asm_place(program))

    def test_call_through_register(self):
        target = jit_asm_address("call_target")
        program = [
            ("push", RBP),
            ("mov_imm", RAX, target), ("call_r", RAX),
            ("pop", RBP), ("ret",),
        ]
        self.assertEqual(run(program, (10, 4)), 34)

    def test_jump_through_register(self):
        program = [
            ("lea_label", RAX, 0), ("jmp_r", RAX),
            ("mov_imm", RAX, 1), ("ret",),
            ("bind", 0), ("mov_imm", RAX, 2), ("ret",),
        ]
        self.assertEqual(run(program), 2)

    def test_entry_table(self):
        # An entry table of absolute label addresses, indexed at run time.
        program = [
            ("lea_label", RAX, 10),
            ("jmp_m", (RAX, RDI, 3, 0)),
            ("bind", 0), ("mov_imm", RAX, 100), ("ret",),
            ("bind", 1), ("mov_imm", RAX, 200), ("ret",),
            ("bind", 2), ("mov_imm", RAX, 300), ("ret",),
            ("align", 8),
            ("bind", 10),
            ("quad_label", 0), ("quad_label", 1), ("quad_label", 2),
        ]
        for index, expected in [(0, 100), (1, 200), (2, 300), (1, 200)]:
            self.assertEqual(run(program, (index,)), expected)

    def test_double_computation(self):
        # sqrt(x*x + y*y) + n, with n converted from an integer.
        program = [
            ("sse_rr", SSE["mulsd"], 0, 0),
            ("sse_rr", SSE["mulsd"], 1, 1),
            ("sse_rr", SSE["addsd"], 0, 1),
            ("sse_rr", SSE["sqrtsd"], 0, 0),
            ("cvtsi2sd", 2, RDI),
            ("sse_rr", SSE["addsd"], 0, 2),
            ("ret",),
        ]
        self.assertEqual(run(program, (10,), (3.0, 4.0), True), 15.0)

    def test_double_through_memory_and_general_registers(self):
        program = [
            ("movq_rx", RAX, 0),            # the bits of the argument
            ("mov_mr", (RSP, -8), RAX),
            ("movsd_rm", 1, (RSP, -8)),
            ("sse_rr", SSE["subsd"], 1, 0),  # 0.0
            ("sse_rr", SSE["divsd"], 0, 1),  # x / 0.0 = inf
            ("movsd_mr", (RSP, -16), 0),
            ("mov_rm", RAX, (RSP, -16)),
            ("ret",),
        ]
        self.assertEqual(run(program, (), (2.5,)),
                         struct.unpack("<q", struct.pack("<d", math.inf))[0])

    def test_compare_and_truncate(self):
        program = [
            ("alu_rr", ALU["xor"], RAX, RAX),
            ("sse_rr", SSE["ucomisd"], 0, 1),
            ("setcc", A, RAX),               # x > y
            ("cvttsd2si", RCX, 0),
            ("shift_ri", SHIFT["shl"], RAX, 8),
            ("alu_rr", ALU["or"], RAX, RCX),
            ("ret",),
        ]
        self.assertEqual(run(program, (), (7.9, 2.0)), (1 << 8) | 7)
        self.assertEqual(run(program, (), (-7.9, 2.0)), -7)

    def test_push_pop(self):
        program = [
            ("push", RDI), ("push", R15), ("mov_imm", R15, 9),
            ("alu_rr", ALU["add"], RSI, R15), ("pop", R15), ("pop", RAX),
            ("alu_rr", ALU["add"], RAX, RSI), ("ret",),
        ]
        self.assertEqual(run(program, (40, 2)), 51)


HOT, COLD = 0, 1


class SectionTests(unittest.TestCase):
    """Hot and cold sections: placement lays out the hot bytes, then the
    cold bytes, and resolves jumps, calls and label addresses across them."""

    def test_jumps_between_sections(self):
        # x != 0 detours through the cold section and comes back.
        program = [
            ("mov_imm", RAX, 10),
            ("test_rr", RDI, RDI),
            ("jcc", NE, 0),
            ("bind", 1),
            ("alu_ri", ALU["add"], RAX, 1),
            ("ret",),
            ("section", COLD),
            ("bind", 0),
            ("alu_ri", ALU["add"], RAX, 5),
            ("jmp", 1),
            ("section", HOT),
            ("alu_ri", ALU["add"], RAX, 100),   # never reached; hot again
            ("ret",),
        ]
        self.assertEqual(run(program, (0,)), 11)
        self.assertEqual(run(program, (1,)), 16)

    def test_cold_code_follows_the_hot_code(self):
        hot = [("mov_imm", RAX, 1), ("ret",)]
        together = hot + [("section", COLD), ("mov_imm", RAX, 2), ("ret",)]
        a, b = jit_asm_assemble(hot, 0), jit_asm_assemble(together, 0)
        self.assertEqual(b[:len(a)], a)
        self.assertGreater(len(b), 64)           # the cold bytes start at 64
        self.assertEqual(b[64:], bytes.fromhex("b802000000c3"))
        self.assertEqual(set(b[len(a):64]), {0xCC})

    def test_call_c_from_the_cold_section(self):
        target = jit_asm_address("Py_HashPointer")
        program = [
            ("test_rr", RDI, RDI),
            ("jcc", NE, 0),
            ("mov_imm", RAX, -1),
            ("ret",),
            ("section", COLD),
            ("bind", 0),
            ("push", RBP), ("call_abs", target), ("pop", RBP), ("ret",),
        ]
        self.assertEqual(run(program, (0,)), -1)
        pointer = 0x7f0000001230
        self.assertEqual(run(program, (pointer,)),
                         signed(bits((pointer >> 4) | (pointer << 60))))

    def test_entry_table_into_both_sections(self):
        def table_program(table_section):
            entries = [(0, HOT), (1, COLD), (2, HOT), (3, COLD)]
            program = [
                ("lea_label", RAX, 10),
                ("jmp_m", (RAX, RDI, 3, 0)),
            ]
            bodies = {section: [] for section in (HOT, COLD)}
            for label, section in entries:
                bodies[section] += [
                    ("bind", label), ("mov_imm", RAX, 100 + label), ("ret",)]
            program += bodies[HOT]
            program += [("section", COLD)] + bodies[COLD]
            program += [("section", table_section), ("align", 8), ("bind", 10)]
            program += [("quad_label", label) for label, _ in entries]
            return program
        for table_section in (HOT, COLD):
            program = table_program(table_section)
            for index in range(4):
                with self.subTest(table_section=table_section, index=index):
                    self.assertEqual(run(program, (index,)), 100 + index)

    def test_unbound_cold_label_is_an_error(self):
        program = [("jmp", 3), ("section", COLD), ("ret",)]
        self.assertIsNone(jit_asm_place(program))


def maps_entry(address):
    """The permissions of the mapping that contains the address."""
    with open("/proc/self/maps") as f:
        for line in f:
            match = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S+) ", line)
            low, high = int(match[1], 16), int(match[2], 16)
            if low <= address < high:
                return match[3]
    return None


@unittest.skipUnless(os.path.exists("/proc/self/maps"), "needs /proc")
class MemoryTests(unittest.TestCase):

    def setUp(self):
        self.info = jit_memory_info()
        self.assertIsNotNone(self.info)
        self.before = self.info["pages_in_use"]

    def test_reservation_is_within_reach_of_the_text(self):
        info = self.info
        low = min(info["reservation"], info["text_low"])
        high = max(info["reservation"] + info["reservation_size"], info["text_high"])
        self.assertLess(high - low, 2**31)
        function = jit_asm_address("Py_HashPointer")
        self.assertTrue(info["text_low"] <= function < info["text_high"])

    def test_placed_pages_are_not_writable(self):
        program = [("mov_imm", RAX, 1), ("ret",)]
        address, size = jit_asm_place(program)
        try:
            self.assertEqual(maps_entry(address)[:3], "r-x")
            self.assertTrue(address % self.info["page_size"] == 0)
        finally:
            jit_memory_free(address, size)
        self.assertEqual(maps_entry(address)[:3], "---")

    def test_no_page_is_writable_and_executable(self):
        placed = [jit_asm_place([("ret",)]) for _ in range(8)]
        raw = jit_memory_alloc(10000)         # filled, not yet sealed
        try:
            self.assertEqual(maps_entry(raw)[:3], "rw-")
            info = self.info
            end = info["reservation"] + info["reservation_size"]
            with open("/proc/self/maps") as f:
                for line in f:
                    match = re.match(r"([0-9a-f]+)-([0-9a-f]+) (\S+) ", line)
                    low = int(match[1], 16)
                    if info["reservation"] <= low < end:
                        self.assertNotRegex(match[3], r"^.wx")
        finally:
            jit_memory_free(raw, 10000)
            for address, size in placed:
                jit_memory_free(address, size)

    def test_many_blocks(self):
        info = self.info
        blocks = []
        for i in range(1000):
            program = [("mov_imm", RAX, i), ("ret",)]
            blocks.append(jit_asm_place(program))
        self.assertEqual(len({address for address, _ in blocks}), 1000)
        after = jit_memory_info()
        self.assertEqual(after["pages_in_use"], self.before + 1000)
        self.assertEqual(after["bytes_in_use"] % 6, 0)
        for i, (address, size) in enumerate(blocks):
            self.assertEqual(jit_asm_call(address, (), (), False), i)
        # Free every other block, then place again: freed pages are reused.
        for address, size in blocks[::2]:
            jit_memory_free(address, size)
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before + 500)
        again = [jit_asm_place([("mov_imm", RAX, 7), ("ret",)]) for _ in range(500)]
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before + 1000)
        self.assertEqual({jit_asm_call(a, (), (), False) for a, _ in again}, {7})
        for address, size in again + blocks[1::2]:
            jit_memory_free(address, size)
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before)

    def test_large_block_spans_pages(self):
        size = 3 * self.info["page_size"] + 1
        address = jit_memory_alloc(size)
        try:
            self.assertEqual(jit_memory_info()["pages_in_use"], self.before + 4)
        finally:
            jit_memory_free(address, size)
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before)

    def test_exhausted_reservation_is_an_error(self):
        chunk = 64 << 20
        blocks = []
        while (address := jit_memory_alloc(chunk)) is not None:
            blocks.append(address)
            self.assertLessEqual(len(blocks), self.info["reservation_size"] // chunk)
        try:
            self.assertGreater(len(blocks), 0)
            self.assertIsNone(jit_asm_place([("ret",)]))
            self.assertIsNone(jit_memory_alloc(chunk))
        finally:
            for address in blocks:
                jit_memory_free(address, chunk)
        self.assertEqual(jit_asm_call(jit_asm_place([("mov_imm", RAX, 3), ("ret",)])[0],
                                      (), (), False), 3)

    def test_unusable_requests_are_errors(self):
        self.assertIsNone(jit_memory_alloc(2**60))
        self.assertIsNone(jit_memory_alloc(0))
        # An unbound label cannot be resolved.
        self.assertIsNone(jit_asm_place([("jmp", 5), ("ret",)]))
        self.assertIsNone(jit_asm_place([]))
        self.assertEqual(jit_memory_info()["pages_in_use"], self.before)

    @support.requires_fork()
    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_fork_keeps_placed_code_and_places_its_own(self):
        placed = jit_asm_place([("mov_imm", RAX, 11), ("ret",)])
        pid = os.fork()
        if pid == 0:
            ok = jit_asm_call(placed[0], (), (), False) == 11
            ok = ok and run([("mov_imm", RAX, 12), ("ret",)]) == 12
            os._exit(0 if ok else 1)
        _, status = os.waitpid(pid, 0)
        jit_memory_free(*placed)
        self.assertEqual(os.waitstatus_to_exitcode(status), 0)


if __name__ == "__main__":
    unittest.main()
