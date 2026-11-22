"""bytecode_helper - support tools for testing correct bytecode generation"""

import unittest
import dis
import io
import opcode
try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None

_UNSPECIFIED = object()

def instructions_with_positions(instrs, co_positions):
    # Return (instr, positions) pairs from the instrs list and co_positions
    # iterator. co_positions has one entry per word, and a call's operand
    # words follow its head with the head's position.

    positions = list(co_positions or ())
    for instr in instrs:
        yield instr, (positions[instr.offset]
                      if instr.offset < len(positions) else ())

def encode_words(*words):
    """Return co_code bytes for the given (handler, a, b, c, d) words.

    Missing fields are zero.  The handler may be a name, which may carry
    word flags as dis names them ("CALL+NO_SELF").
    """
    out = bytearray()
    for word in words:
        handler, *fields = word
        if isinstance(handler, str):
            handler = dis._handler_field(handler)
        fields += [0] * (4 - len(fields))
        value = handler
        for shift, field in zip((16, 28, 40, 52), fields):
            assert 0 <= field < 4096
            value |= field << shift
        out += value.to_bytes(8, 'little')
    return bytes(out)

def shift_for_discard_slot(words, nlocals):
    """Return words with the frame slots of the temporaries made room for
    the discard slot.

    A code object keeps a hidden local, the discard slot, after its
    constants and before its free variables; the temporaries start after it.
    Words written for a frame without it name a temporary as nlocals + k
    where the code object has nlocals + 1 + k.  This adds one to each slot
    field of a word (a destination, window base or input) that is at least
    nlocals, other than the discard sentinel 0xFFF.  A feedback field written
    as 0, for a site without an entry, becomes the shared entry 0xFFF.
    """
    import _opcode
    contract = _opcode.get_ir_contract()
    shifted = []
    for word in words:
        handler, *fields = word
        if isinstance(handler, str):
            handler = dis._handler_field(handler)
        fields = list(fields) + [0] * (4 - len(fields))
        roles = contract[handler & _opcode.IR_HANDLER_MASK].roles
        if handler & _opcode.IR_HANDLER_MASK == _opcode.IR_OPERANDS:
            # The operand words of a call name slots in all four fields
            roles = (_opcode.IR_ROLE_IN0,) * 4
        for k, role in enumerate(roles):
            is_slot = (role in (_opcode.IR_ROLE_DST, _opcode.IR_ROLE_BASE)
                       or role >= _opcode.IR_ROLE_IN0)
            if is_slot and nlocals <= fields[k] < 0xFFF:
                fields[k] += 1
            elif role == _opcode.IR_ROLE_FB and fields[k] == 0:
                fields[k] = 0xFFF
        shifted.append((handler, *fields))
    return shifted

def constant_slot_reads(x):
    """Yield (instruction, value) for each constant that an instruction of x
    reads from a constant slot.

    The code's co_constslots pairs a frame slot (a hidden CO_FAST_CONST
    local, pre-filled at frame creation) with an index into co_consts.
    An instruction reads the slot when it is in Instruction.srcs.
    """
    co = dis._get_code_object(x)
    table = co.co_constslots
    slots = {}
    for off in range(0, len(table), 8):
        slot = int.from_bytes(table[off:off + 4], 'little')
        index = int.from_bytes(table[off + 4:off + 8], 'little')
        slots[slot] = co.co_consts[index]
    for instr in dis.get_instructions(x):
        for src in instr.srcs:
            if src in slots:
                yield instr, slots[src]

class BytecodeTestCase(unittest.TestCase):
    """Custom assertion methods for inspecting bytecode."""

    def get_disassembly_as_string(self, co):
        s = io.StringIO()
        dis.dis(co, file=s)
        return s.getvalue()

    # A constant slot read stands for the load of its constant that the
    # earlier forms of the IR emitted.
    CONSTANT_LOADS = ('LOAD_CONST', 'LOAD_SMALL_INT', 'LOAD_COMMON_CONSTANT')

    def matching_instructions(self, x, opname, argval=_UNSPECIFIED):
        """Yield the instructions of x that are opname with argval, and for
        a constant load opname, the instructions reading a constant slot
        whose constant is argval (same type and value)."""
        self.assertIn(opname, dis.opmap)
        for instr in dis.get_instructions(x):
            if instr.opname == opname:
                if argval is _UNSPECIFIED or instr.argval == argval:
                    yield instr
        if opname not in self.CONSTANT_LOADS:
            return
        for instr, value in constant_slot_reads(x):
            if not self.loadable_as(opname, value):
                continue
            if (argval is _UNSPECIFIED
                    or (type(value) is type(argval) and value == argval)):
                yield instr

    def loadable_as(self, opname, value):
        """Whether the load opname could have produced value."""
        if opname == 'LOAD_SMALL_INT':
            return type(value) is int and 0 <= value <= 255
        if opname == 'LOAD_COMMON_CONSTANT':
            return any(type(c) is type(value) and c == value
                       for c in dis._common_constants)
        return True

    def assertInBytecode(self, x, opname, argval=_UNSPECIFIED):
        """Returns instr if opname is found, otherwise throws AssertionError.

        For LOAD_CONST, LOAD_SMALL_INT and LOAD_COMMON_CONSTANT, an
        instruction that reads a constant slot holding argval also counts;
        the instruction returned is then the reader."""
        for instr in self.matching_instructions(x, opname, argval):
            return instr
        disassembly = self.get_disassembly_as_string(x)
        if argval is _UNSPECIFIED:
            msg = '%s not found in bytecode:\n%s' % (opname, disassembly)
        else:
            msg = '(%s,%r) not found in bytecode:\n%s'
            msg = msg % (opname, argval, disassembly)
        self.fail(msg)

    def assertNotInBytecode(self, x, opname, argval=_UNSPECIFIED):
        """Throws AssertionError if opname is found (see assertInBytecode)"""
        for instr in self.matching_instructions(x, opname, argval):
            disassembly = self.get_disassembly_as_string(x)
            if argval is _UNSPECIFIED:
                msg = '%s occurs in bytecode:\n%s' % (opname, disassembly)
            else:
                msg = '(%s,%r) occurs in bytecode:\n%s'
                msg = msg % (opname, argval, disassembly)
            self.fail(msg)

class CompilationStepTestCase(unittest.TestCase):

    HAS_ARG = set(dis.hasarg)
    HAS_TARGET = set(dis.hasjrel + dis.hasjabs + dis.hasexc)
    HAS_ARG_OR_TARGET = HAS_ARG.union(HAS_TARGET)

    class Label:
        pass

    def assertInstructionsMatch(self, actual_seq, expected):
        # get an InstructionSequence and an expected list, where each
        # entry is a label or an instruction tuple. Construct an expected
        # instruction sequence and compare with the one given.

        self.assertIsInstance(expected, list)
        actual = actual_seq.get_instructions()
        expected = self.seq_from_insts(expected).get_instructions()
        self.assertEqual(len(actual), len(expected))

        # compare instructions
        for act, exp in zip(actual, expected):
            if isinstance(act, int):
                self.assertEqual(exp, act)
                continue
            self.assertIsInstance(exp, tuple)
            self.assertIsInstance(act, tuple)
            idx = max([p[0] for p in enumerate(exp) if p[1] != -1])
            self.assertEqual(exp[:idx], act[:idx])

    def resolveAndRemoveLabels(self, insts):
        idx = 0
        res = []
        for item in insts:
            assert isinstance(item, (self.Label, tuple))
            if isinstance(item, self.Label):
                item.value = idx
            else:
                idx += 1
                res.append(item)

        return res

    def seq_from_insts(self, insts):
        labels = {item for item in insts if isinstance(item, self.Label)}
        for i, lbl in enumerate(labels):
            lbl.value = i

        seq = _testinternalcapi.new_instruction_sequence()
        for item in insts:
            if isinstance(item, self.Label):
                seq.use_label(item.value)
            else:
                op = item[0]
                if isinstance(op, str):
                    op = opcode.opmap[op]
                arg, *loc = item[1:]
                if isinstance(arg, self.Label):
                    arg = arg.value
                loc = loc + [-1] * (4 - len(loc))
                seq.addop(op, arg or 0, *loc)
        return seq

    def check_instructions(self, insts):
        for inst in insts:
            if isinstance(inst, self.Label):
                continue
            op, arg, *loc = inst
            if isinstance(op, str):
                op = opcode.opmap[op]
            self.assertEqual(op in opcode.hasarg,
                             arg is not None,
                             f"{opcode.opname[op]=} {arg=}")
            self.assertTrue(all(isinstance(l, int) for l in loc))


@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class CodegenTestCase(CompilationStepTestCase):

    def generate_code(self, ast):
        insts, _ = _testinternalcapi.compiler_codegen(ast, "my_file.py", 0)
        return insts


@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class CfgOptimizationTestCase(CompilationStepTestCase):

    def get_optimized(self, seq, consts, nlocals=0):
        insts = _testinternalcapi.optimize_cfg(seq, consts, nlocals)
        return insts, consts

@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
class AssemblerTestCase(CompilationStepTestCase):

    def get_code_object(self, filename, insts, metadata):
        co = _testinternalcapi.assemble_code_object(filename, insts, metadata)
        return co
