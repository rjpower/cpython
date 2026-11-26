"""Disassembler of the register IR that a code object lowers to.

The tests of the IR read it through this module.  The IR of a code object is
one 8-byte word per instruction (a call or a span instruction also has
operand words), a 16-bit handler and four 12-bit fields.  get_instructions()
yields one Word per head: the arm that it runs (Family.Arm), its operands by
their declared names, roles and kinds, its edges, its line and its origin.

Everything about the words comes from the generated metadata, which the
runtime exports: the handler names (_opcode.get_ir_handler_names), the
contract of each handler (_opcode.get_ir_contract) and the operand table of
each family (_testinternalcapi.arm_operands).  Nothing here says which stack
instruction a word came from, except origin_opcode(), which reads co_code at
the word's origin offset.

Offsets are word indices; a word's origin is the byte offset in co_code of
the stack instruction that the word covers.
"""

import collections
import io
import struct
import sys
import types

import _opcode
import _testinternalcapi

__all__ = ["dis", "disassemble", "get_instructions", "findlinestarts",
           "findlabels", "Bytecode", "Word", "Operand", "Edge", "IRCode",
           "origin_opcode", "encode_words", "specialized_arms",
           "constant_slot_reads"]

_have_code = (types.MethodType, types.FunctionType, types.CodeType,
              classmethod, staticmethod, type)

_ir_contract = _opcode.get_ir_contract()
_operand_tables = _testinternalcapi.arm_operands()

# The name of each handler id: arms, their observed wrappers, IR_OPERANDS
_all_opname = _opcode.get_ir_handler_names()
_all_opmap = {name: h for h, name in enumerate(_all_opname) if name is not None}

_WORD_SIZE = 8
_FIELD_MAX = 0xFFF
_HANDLER_MASK = _opcode.IR_HANDLER_MASK
# The canonical flags of the handler field, which the name of a word shows
# after its handler's as +NAME: "Call.Generic+NO_SELF" is a call whose self
# input is virtual NULL.  The executing copy's slot-class flags are not shown.
_WORD_FLAGS = {"NO_SELF": _opcode.IR_F_NO_SELF, "NO_KW": _opcode.IR_F_NO_KW,
               "KW": _opcode.IR_F_KW, "EX": _opcode.IR_F_EX,
               "UNINTERRUPTED": _opcode.IR_F_UNINTERRUPTED}
_IR_JUMPS = _opcode.IR_JUMPS
_IR_FORWARD = _opcode.IR_FORWARD
_ROLE_ELEMENT = _opcode.IR_ROLE_ELEMENT
_FEEDBACK_KINDS, _FEEDBACK_STATES = _testinternalcapi.get_feedback_names()
_NO_LINENO = '  --'


def _handler_name(field):
    """The name of a word's handler field: its handler's, then its flags"""
    # EX is both of the bits that KW and UNINTERRUPTED are one of; bit 13
    # is NO_KW on an EX word and NO_SELF on any other
    kind = field & _opcode.IR_F_EX
    names = {_opcode.IR_F_EX: ["EX"], _opcode.IR_F_KW: ["KW"],
             _opcode.IR_F_UNINTERRUPTED: ["UNINTERRUPTED"], 0: []}[kind]
    if field & _opcode.IR_F_NO_SELF:
        names.insert(0, "NO_KW" if kind == _opcode.IR_F_EX else "NO_SELF")
    return _all_opname[field & _HANDLER_MASK] + "".join("+" + n for n in names)


def _handler_field(name):
    """The handler field of the word that _handler_name() names name"""
    handler, *flags = name.split("+")
    return _all_opmap[handler] | sum(_WORD_FLAGS[flag] for flag in flags)


class IRCode:
    """A code object as its register IR: the words as co_code, the feedback
    table as co_fbinit, and positions and lines per word.  Other attributes
    are the code object's."""

    def __init__(self, co):
        (self.co_code, self.co_fbinit, self.nfixed, self.nconst, self.nslots,
         self.const_values, self._exceptions, self._records, self._positions,
         pcmap, self.first_traceable, adaptive, self._co_feedback
         ) = _testinternalcapi.get_ir_raw(co)
        self._co = co
        self._pcmap = struct.unpack("<%dI" % (len(pcmap) // 4), pcmap)
        self._co_code_adaptive = self.co_code if adaptive is None else adaptive

    def __getattr__(self, name):
        return getattr(self._co, name)

    @property
    def const_slots(self):
        """The constant each constant slot holds, by slot: the hidden locals
        that follow the locals, cells and free variables, filled when a
        frame is created"""
        first = len(_testinternalcapi.get_co_localskinds(self._co))
        return {first + k: value for k, value in enumerate(self.const_values)}

    @property
    def stack_offsets(self):
        """The co_code byte offset of the stack instruction that each word
        stands for: the offset that monitoring events report"""
        return [2 * unit for unit in self._pcmap]

    def co_positions(self):
        positions = list(self._co.co_positions())
        # A code object with a short or empty location table has no position
        # for the units past its end
        none = (None, None, None, None)
        return iter([positions[unit] if unit < len(positions) else none
                     for unit in self._pcmap])

    def co_lines(self):
        """(start, end, line) per run of words on a line, in word indices"""
        runs = []
        for word, position in enumerate(self.co_positions()):
            line = position[0]
            if runs and runs[-1][2] == line:
                runs[-1][1] = word + 1
            else:
                runs.append([word, word + 1, line])
        return iter([tuple(run) for run in runs])


def _ir(co):
    return co if isinstance(co, IRCode) else IRCode(co)


def _get_code_object(x):
    """Helper to handle methods, compiled or raw code objects, and strings."""
    if hasattr(x, '__func__'):
        x = x.__func__
    if hasattr(x, '__code__'):
        x = x.__code__
    elif hasattr(x, 'gi_code'):
        x = x.gi_code
    elif hasattr(x, 'ag_code'):
        x = x.ag_code
    elif hasattr(x, 'cr_code'):
        x = x.cr_code
    if isinstance(x, str):
        x = _try_compile(x, "<disassembly>")
    if hasattr(x, 'co_code'):
        return _ir(x)
    raise TypeError("don't know how to disassemble %s objects" %
                    type(x).__name__)


def _try_compile(source, name):
    try:
        return compile(source, name, 'eval')
    except SyntaxError:
        pass
    return compile(source, name, 'exec')


# ---- The record of a word

Positions = collections.namedtuple(
    'Positions', ['lineno', 'end_lineno', 'col_offset', 'end_col_offset'],
    defaults=[None] * 4)

# One operand of a word, by its declared name.  kind is the declared field
# kind (home, imm, name, const, feedback, target, vector, flag) and role its
# role (none, src, dst, local, cell, free, iterpair, sendwindow).  value is
# the field: a slot for a home, the distance or index for a target, the unit
# for a feedback entry, a tuple of slots for a vector, None for an absent
# optional operand.  text is the value as the printed form shows it.
Operand = collections.namedtuple("Operand", "name kind role value text")

# A word's edge to another word: kind is the name of the target operand,
# target the word index and origin the co_code offset of the target's
# stack instruction
Edge = collections.namedtuple("Edge", "kind target origin")

_Word = collections.namedtuple(
    "_Word",
    ["offset", "length", "origin", "handler", "arm", "family", "operands",
     "dst", "srcs", "edges", "fb", "cache_info", "line_number", "starts_line",
     "label", "positions", "executing"])


class Word(_Word):
    """One head of the register IR of a code object.

       offset - index of the head word; operand words follow it
       length - number of words, head and operand words
       origin - byte offset in co_code of the stack instruction the word covers
       handler - arm name and the word's flags: "Call.Generic+NO_SELF"
       arm - "Family.Arm" that the word runs (the executing copy's with adaptive)
       family - the head of the arm's name
       operands - tuple of Operand in declared order
       dst - the slot that the word writes, or None
       srcs - the slots that the word reads
       edges - tuple of Edge to the words that the word may continue at
       fb - unit of the word's feedback entry, or None
       cache_info - the fields of the feedback entry (with show_caches)
       line_number, starts_line, label, positions - source information
       executing - the arm of the executing copy of the word"""

    @property
    def end_offset(self):
        return self.offset + self.length

    @property
    def is_jump_target(self):
        return self.label is not None

    def operand(self, name):
        """The Operand called name, or None if the word has no such operand"""
        for operand in self.operands:
            if operand.name == name:
                return operand
        return None

    def get(self, name):
        """The value of the operand called name (a name operand gives the
        name itself), or None"""
        operand = self.operand(name)
        return None if operand is None else operand.value


def _unpack_word(code, offset):
    return int.from_bytes(code[offset * _WORD_SIZE:(offset + 1) * _WORD_SIZE],
                          sys.byteorder)


def _fb_unit(field):
    """The unit of a 12-bit feedback field: the negative distance in 8-byte
    units from the start of entry 0, whose field is 0xFFF, which ends the
    vector, to the entry"""
    return field - _FIELD_MAX


def _feedback_entry(fbinit, unit):
    """The (kind, arg, units) of the entry of the feedback table fbinit that
    starts at unit, or None.  A word of the table is the kind (4 bits), the
    unit's field (12 bits) and the arg (16 bits); entry 0 comes first and is
    the 2 units of a header, the others follow in order of address, each
    ending where the next begins and the last where entry 0 does, and the
    arg of entry 0 is the number of units."""
    if fbinit is None:
        return None
    words = [int.from_bytes(fbinit[i:i + 4], sys.byteorder)
             for i in range(0, len(fbinit), 4)]
    units = [_fb_unit((w >> 16) & _FIELD_MAX) for w in words]
    try:
        i = units.index(unit)
    except ValueError:
        return None
    if i == 0:
        end = 2
    else:
        end = units[i + 1] if i + 1 < len(units) else 0
    return words[i] >> 28, words[i] & 0xFFFF, end - unit


def _feedback_info(fb, fbinit, feedback):
    """The cache_info of the feedback entry at unit fb: of the executing copy
    if feedback holds its entries, else the kind and static operand in
    fbinit, the code object's feedback table.  Entry 0 is the last 16 bytes
    of feedback."""
    if fb is None or fb == 0:
        return None
    found = _feedback_entry(fbinit, fb)
    if found is None:
        return None
    kind, arg, units = found
    if feedback is not None:
        start = len(feedback) - 16 + fb * 8
        entry = feedback[start:start + units * 8]
        # _PyFbHeader (Include/internal/pycore_feedback.h), then the first
        # two 32-bit fields of the entry if it has them
        layout = (("kind", 0, 1), ("state", 1, 1), ("counter", 2, 2),
                  ("arg", 8, 4), ("first", 16, 4), ("second", 20, 4))
        return [(name, size, entry[start:start + size])
                for name, start, size in layout if start + size <= len(entry)]
    return [("kind", 1, kind.to_bytes(1, sys.byteorder)),
            ("arg", 2, arg.to_bytes(2, sys.byteorder))]


def _field_at(code, offset, bit, bits):
    """The bits at bit of the image of the head and operand words that
    start at offset"""
    word = _unpack_word(code, offset + bit // 64)
    return (word >> (bit % 64)) & ((1 << bits) - 1)


def _signed(value, bits):
    return value - (1 << bits) if value >> (bits - 1) else value


def _vector_slots(code, offset, head, row, operands):
    """The elements of the vector operands of the word at offset, a tuple
    per vector in declared order.  A span format holds its spans' elements
    in the head's element fields, then operand words of four fields after
    its fixed operand words; a call has them in the operand words."""
    vectors = [op for op in operands if op[1] == "vector"]
    counts = [(head >> op[5]) & _FIELD_MAX for op in vectors]
    if row.operands == _opcode.IR_OPERANDS_SPANS:
        first = [k for k, role in enumerate(row.roles) if role == _ROLE_ELEMENT]
        slots = []
        for j in range(sum(counts)):
            if j < len(first):
                slots.append((head >> (16 + 12 * first[j])) & _FIELD_MAX)
            else:
                n = j - len(first)
                word = _unpack_word(code, offset + 1 + row.fixed_words + n // 4)
                slots.append((word >> (16 + 12 * (n % 4))) & _FIELD_MAX)
    else:
        slots = []
        for n in range(sum(counts)):
            word = _unpack_word(code, offset + 1 + n // 4)
            slots.append((word >> (16 + 12 * (n % 4))) & _FIELD_MAX)
    spans, start = [], 0
    for count in counts:
        spans.append(tuple(slots[start:start + count]))
        start += count
    return spans


def _decode(co, code, offset, offsets):
    """The operands, dst, srcs, edges and feedback unit of the word at offset
    of code, """
    word = _unpack_word(code, offset)
    handler = word & _HANDLER_MASK
    row = _ir_contract[handler]
    family = _all_opname[handler].partition(".")[0]
    table = _operand_tables.get(family, ())
    if row is None:
        return (), None, (), (), None
    spans = iter(_vector_slots(code, offset, word, row, table))
    operands, srcs, edges = [], [], []
    dst = fb = kwnames = None
    for name, kind, role, optional, bits, bit in table:
        if kind == "vector":
            value = next(spans)
            if row.operands == _opcode.IR_OPERANDS_KW:
                # The last operand of a keyword call indexes its names in
                # the constants
                kwnames = co.co_consts[value[-1]]
                value = value[:-1]
            text = ", ".join(f"r{slot}" for slot in value)
            text = f"[{text}]"
        elif kind == "flag":
            value = (word >> bit) & 1
            text = str(value)
        else:
            value = _field_at(code, offset, bit, bits)
            # An optional operand, and a destination that discards its
            # result, hold the all-ones sentinel when absent
            if (optional or role == "dst") and value == (1 << bits) - 1:
                value = None
            if value is None:
                text = "-"
            elif kind == "home":
                text = f"r{value}"
            elif kind == "name":
                text = co.co_names[value]
                value = text
            elif kind == "feedback":
                value = _fb_unit(value)
                fb = value
                text = f"fb{value}"
            elif kind == "target":
                if row.flags & _IR_FORWARD:
                    target = offset + 1 + _signed(value, bits)
                else:
                    target = value
                edges.append(Edge(name, target,
                                  offsets[target] if 0 <= target < len(offsets) else None))
                value = target
                text = f"-> {target}"
            else:
                text = str(value)
        operands.append(Operand(name, kind, role, value, text))
        if kind == "home" and value is not None:
            if role == "dst" and dst is None:
                dst = value
            elif role in ("src", "sendwindow", "iterpair"):
                srcs.append(value)
        elif kind == "vector" and role in ("src", "none"):
            srcs.extend(value)
    if kwnames is not None:
        operands.append(Operand("kwnames", "const", "none", kwnames, repr(kwnames)))
    return tuple(operands), dst, tuple(srcs), tuple(edges), fb


def get_instructions(x, *, first_line=None, show_caches=None, adaptive=False):
    """Iterator for the words of methods, functions or code

    Generates a Word per head of the supplied code.  If *first_line* is not
    None, it is the line number that should be reported for the first source
    line.  With *adaptive*, a word's arm is that of the executing copy."""
    co = _get_code_object(x)
    return _get_words(co, first_line, adaptive, labels_map=_labels_of(co))


def _get_words(co, first_line, adaptive, labels_map):
    line_offset = 0 if first_line is None else first_line - co.co_firstlineno
    linestarts = dict(findlinestarts(co))
    code = co.co_code
    running = co._co_code_adaptive if adaptive else code
    offsets = co.stack_offsets
    positions = co.co_positions()
    feedback = co._co_feedback if adaptive else None
    line_number = None
    offset = 0
    while offset * _WORD_SIZE < len(code):
        word = _unpack_word(code, offset)
        starts_line = offset in linestarts
        if starts_line:
            line_number = linestarts[offset]
            line_number = None if line_number is None else line_number + line_offset
        head_positions = Positions(*next(positions, ()))
        length = _opcode.get_ir_word_length(word)
        for _ in range(length - 1):
            next(positions, ())
        runs = _unpack_word(running, offset) & _HANDLER_MASK
        flags = word & ~_HANDLER_MASK
        operands, dst, srcs, edges, fb = _decode(co, code, offset, offsets)
        yield Word(offset, length, offsets[offset], _handler_name(runs | flags),
                   _all_opname[runs], _all_opname[runs].partition(".")[0],
                   operands, dst, srcs, edges, fb,
                   _feedback_info(fb, co.co_fbinit, feedback), line_number,
                   starts_line, labels_map.get(offset), head_positions,
                   _all_opname[runs])
        offset += length


def origin_opcode(x, word):
    """The name of the stack instruction at the word's origin in the co_code
    of x (a function or code object), for a test that asks which stack
    instruction a word came from"""
    import dis as stack_dis
    return stack_dis.opname[_get_code_object(x)._co.co_code[word.origin]]


def findlinestarts(code):
    """Find the word indices that start a line: generate (offset, lineno)"""
    lastline = False # None is a valid line number
    for start, end, line in _ir(code).co_lines():
        if line is not lastline:
            lastline = line
            yield start, line


def _labels_of(co, exception_entries=()):
    labels = set()
    for word in _get_words_nolabels(co):
        labels.update(edge.target for edge in word.edges)
    for start, end, target, *_ in exception_entries:
        labels.update((start, end, target))
    return {offset: i + 1 for i, offset in enumerate(sorted(labels))}


def _get_words_nolabels(co):
    return _get_words(co, None, False, labels_map={})


def findlabels(code):
    """The word indices that are the targets of the words' edges"""
    labels = []
    for word in _get_words_nolabels(_get_code_object(code)):
        for edge in word.edges:
            if edge.target not in labels:
                labels.append(edge.target)
    return labels


# ---- Printing

_ExceptionTableEntryBase = collections.namedtuple("_ExceptionTableEntryBase",
    "start end target slot lasti recipe")


class _ExceptionTableEntry(_ExceptionTableEntryBase):
    pass


def _parse_exception_table(code):
    return [_ExceptionTableEntry(*entry)
            for entry in _opcode.get_exception_entries(code._co)]


def _format_operand(operand):
    if operand.value is None:
        return f"{operand.name}=-"
    return f"{operand.name}={operand.text}"


class Formatter:

    def __init__(self, file=None, lineno_width=0, offset_width=0, label_width=0,
                 show_caches=False, *, show_positions=False):
        self.file = file
        self.lineno_width = lineno_width
        self.offset_width = offset_width
        self.label_width = label_width
        self.show_caches = show_caches
        self.show_positions = show_positions

    def print_word(self, word, mark_as_current=False):
        fields = []
        if self.lineno_width:
            if self.show_positions:
                ps = tuple('?' if p is None else p for p in word.positions)
                if all(p is None for p in word.positions):
                    text = _NO_LINENO
                else:
                    text = f"{ps[0]}:{ps[2]}-{ps[1]}:{ps[3]}"
                fields.append(f'{text:{self.lineno_width}}')
            elif word.starts_line:
                number = _NO_LINENO if word.line_number is None else word.line_number
                fields.append(f"{number:>{self.lineno_width}}")
            else:
                fields.append(' ' * self.lineno_width)
        if word.label is not None:
            fields.append(f"{'L%d:' % word.label:>{self.label_width}}")
        else:
            fields.append(' ' * self.label_width)
        if self.offset_width > 0:
            fields.append(f"{word.offset!r:>{self.offset_width}}  ")
        fields.append('-->' if mark_as_current else '   ')
        fields.append(word.handler.ljust(24))
        fields.extend(_format_operand(op) for op in word.operands
                      if op.kind != "feedback" or self.show_caches)
        print(' '.join(fields).rstrip(), file=self.file)
        if self.show_caches and word.cache_info:
            values = []
            for name, size, data in word.cache_info:
                value = int.from_bytes(data, sys.byteorder)
                if name == "kind" and value < len(_FEEDBACK_KINDS):
                    value = _FEEDBACK_KINDS[value]
                elif name == "state" and value < len(_FEEDBACK_STATES):
                    value = _FEEDBACK_STATES[value]
                values.append(f"{name}: {value}")
            print(' ' * (self.lineno_width + self.label_width)
                  + f"    feedback fb{word.fb}: " + ", ".join(values),
                  file=self.file)

    def print_exception_table(self, exception_entries, labels_map):
        file = self.file
        if exception_entries:
            print("ExceptionTable:", file=file)
            for entry in exception_entries:
                lasti = " lasti" if entry.lasti else ""
                moves = "".join(f" r{home}<-r{src}" for src, home in entry.recipe)
                print(f"  L{labels_map[entry.start]} to L{labels_map[entry.end]} "
                      f"-> L{labels_map[entry.target]} [r{entry.slot}]{lasti}{moves}",
                      file=file)


def _get_lineno_width(linestarts):
    maxlineno = max(filter(None, linestarts.values()), default=-1)
    if maxlineno == -1:
        return 0
    lineno_width = max(3, len(str(maxlineno)))
    if lineno_width < len(_NO_LINENO) and None in linestarts.values():
        lineno_width = len(_NO_LINENO)
    return lineno_width


def _get_positions_width(code):
    has_value = False
    values_width = 0
    for positions in code.co_positions():
        has_value |= any(isinstance(p, int) for p in positions)
        width = sum(1 if p is None else len(str(p)) for p in positions)
        values_width = max(width, values_width)
    if has_value:
        return 1 + max(len(_NO_LINENO), 3 + values_width)
    return 0


def disassemble(co, lasti=-1, *, file=None, show_caches=False, adaptive=False,
                show_offsets=False, show_positions=False, first_line=None):
    """Disassemble a code object."""
    co = _ir(co)
    linestarts = dict(findlinestarts(co))
    exception_entries = _parse_exception_table(co)
    labels_map = _labels_of(co, exception_entries)
    lineno_width = (_get_positions_width(co) if show_positions
                    else _get_lineno_width(linestarts))
    formatter = Formatter(
        file=file, lineno_width=lineno_width,
        offset_width=(len(str(max(len(co.co_code) // _WORD_SIZE - 1, 9999)))
                      if show_offsets else 0),
        label_width=4 + len(str(len(labels_map))),
        show_caches=show_caches, show_positions=show_positions)
    first = True
    for word in _get_words(co, first_line, adaptive, labels_map):
        if lineno_width and word.starts_line and not first:
            print(file=file)
        first = False
        formatter.print_word(word, word.offset == lasti)
    formatter.print_exception_table(exception_entries, labels_map)

disco = disassemble


def _disassemble_recursive(co, *, file=None, depth=None, **kwargs):
    co = _ir(co)
    disassemble(co, file=file, **kwargs)
    if depth is None or depth > 0:
        if depth is not None:
            depth = depth - 1
        for x in co.co_consts:
            if hasattr(x, 'co_code'):
                print(file=file)
                print("Disassembly of %r:" % (x,), file=file)
                _disassemble_recursive(x, file=file, depth=depth, **kwargs)


def dis(x=None, *, file=None, depth=None, show_caches=False, adaptive=False,
        show_offsets=False, show_positions=False):
    """Disassemble classes, methods, functions, and other compiled objects."""
    kwargs = dict(show_caches=show_caches, adaptive=adaptive,
                  show_offsets=show_offsets, show_positions=show_positions)
    if x is None:
        try:
            tb = sys.last_exc.__traceback__ if hasattr(sys, 'last_exc') else sys.last_traceback
        except AttributeError:
            raise RuntimeError("no last traceback to disassemble") from None
        while tb.tb_next:
            tb = tb.tb_next
        disassemble(tb.tb_frame.f_code, tb.tb_lasti, file=file, **kwargs)
        return
    if hasattr(x, '__func__'):
        x = x.__func__
    if hasattr(x, '__code__'):
        x = x.__code__
    elif hasattr(x, 'gi_code'):
        x = x.gi_code
    elif hasattr(x, 'ag_code'):
        x = x.ag_code
    elif hasattr(x, 'cr_code'):
        x = x.cr_code
    if hasattr(x, '__dict__'):  # Class or module
        for name, x1 in sorted(x.__dict__.items()):
            if isinstance(x1, _have_code):
                print("Disassembly of %s:" % name, file=file)
                try:
                    dis(x1, file=file, depth=depth, **kwargs)
                except TypeError as msg:
                    print("Sorry:", msg, file=file)
                print(file=file)
    elif hasattr(x, 'co_code'):
        _disassemble_recursive(x, file=file, depth=depth, **kwargs)
    elif isinstance(x, str):
        _disassemble_recursive(_try_compile(x, '<dis>'), file=file,
                               depth=depth, **kwargs)
    else:
        raise TypeError("don't know how to disassemble %s objects" %
                        type(x).__name__)


class Bytecode:
    """The words of a piece of code: iterating gives Word instances"""

    def __init__(self, x, *, first_line=None, current_offset=None,
                 show_caches=False, adaptive=False, show_offsets=False,
                 show_positions=False):
        self.codeobj = _get_code_object(x)
        self.first_line = first_line
        self._original_object = x
        self.current_offset = current_offset
        self.show_caches = show_caches
        self.adaptive = adaptive
        self.show_offsets = show_offsets
        self.show_positions = show_positions

    def __iter__(self):
        return get_instructions(self.codeobj, first_line=self.first_line,
                                adaptive=self.adaptive)

    def __repr__(self):
        return "{}({!r})".format(self.__class__.__name__, self._original_object)

    def dis(self):
        """Return a formatted view of the words."""
        with io.StringIO() as output:
            disassemble(self.codeobj,
                        -1 if self.current_offset is None else self.current_offset,
                        file=output, show_caches=self.show_caches,
                        adaptive=self.adaptive, show_offsets=self.show_offsets,
                        show_positions=self.show_positions,
                        first_line=self.first_line)
            return output.getvalue()


def specialized_arms(code):
    """The names of the arms that the words of code run in place of their
    family's generic arm.  A family with a Generic arm specializes by
    leaving it; the rest of the arms (Return.Empty, Resume.Poll) are not
    specializations."""
    arms = _testinternalcapi.arm_handlers()
    names = {word[0].partition("+")[0]
             for word in _testinternalcapi.get_ir(code, True)}
    return {name for name in names
            if not name.endswith(".Generic")
            and name.rpartition(".")[0] + ".Generic" in arms}


def encode_words(*words):
    """Return co_code bytes for the given (handler, a, b, c, d) words.

    Missing fields are zero.  The handler may be a name, which may carry
    word flags as _handler_name() shows them ("Call.Generic+NO_SELF").
    """
    out = bytearray()
    for word in words:
        handler, *fields = word
        if isinstance(handler, str):
            handler = _handler_field(handler)
        fields += [0] * (4 - len(fields))
        value = handler
        for shift, field in zip((16, 28, 40, 52), fields):
            assert 0 <= field < 4096
            value |= field << shift
        out += value.to_bytes(8, 'little')
    return bytes(out)


def constant_slot_reads(x):
    """Yield (word, value) for each constant that a word of x reads from a
    constant slot.  A word reads the slot when it is in Word.srcs."""
    slots = _get_code_object(x).const_slots
    for word in get_instructions(x):
        for src in word.srcs:
            if src in slots:
                yield word, slots[src]
