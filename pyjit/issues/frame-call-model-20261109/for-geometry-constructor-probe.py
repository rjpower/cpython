"""Read-only constructor probe for FOR_ITER destination geometry.

Run with the current built fork's Python. This compiles and constructs code
objects; it never executes a mutated code object. Compare baseline and patched
constructor acceptance before promoting assertions into Lib/test/test_ir.py.
"""

import opcode
import types

from test.support.bytecode_helper import encode_words


NO_DST = 4095
FOR = opcode.opmap["FOR_ITER"]
INSTRUMENTED_FOR = opcode.opmap["INSTRUMENTED_FOR_ITER"]


def function_code(repetitions):
    body = "        acc += 1\n" * repetitions or "        pass\n"
    source = "def f(xs):\n    acc = 0\n    for x in xs:\n" + body + "    return acc\n"
    module = compile(source, "<for-geometry-probe>", "exec")
    return next(const for const in module.co_consts if isinstance(const, types.CodeType))


def for_word(code):
    for index in range(len(code.co_code) // 8):
        word = int.from_bytes(code.co_code[8 * index:8 * (index + 1)], "little")
        if (word & 0xFFFF) == FOR:
            fields = tuple((word >> shift) & 0xFFF for shift in (16, 28, 40, 52))
            return index, fields
    raise AssertionError("compiled function has no pristine FOR_ITER word")


def construct(code, index, fields, handler, dst):
    a, b, c, d = fields
    assert b + 3 < NO_DST
    assert dst == NO_DST or dst < NO_DST
    enlarged_stack = max(code.co_stacksize, b + 4 - code.co_nlocals)
    replacement = encode_words((handler, dst, b, c, d))
    bytes_ = code.co_code[:8 * index] + replacement + code.co_code[8 * (index + 1):]
    try:
        # Structural code changes require a distinct, equal source-map object.
        # Keep all source records and offsets exactly as compiled.
        code.replace(co_stacksize=enlarged_stack, co_code=bytes_,
                     co_linetable=memoryview(code.co_linetable).tobytes())
    except ValueError as exc:
        return f"REJECT {exc}"
    return "ACCEPT"


def main():
    for name, repetitions in (("near", 0), ("far", 3200)):
        code = function_code(repetitions)
        index, fields = for_word(code)
        a, b, c, d = fields
        signed_distance = ((d ^ 0x800) - 0x800)
        successors = [opcode.opname[int.from_bytes(
            code.co_code[8 * j:8 * (j + 1)], "little") & 0xFFFF]
            for j in (index + 1, index + 2)]
        print(name, "site", index, "A/B/C/D", fields,
              "signed distance", signed_distance, "successors", successors,
              "nlocals", code.co_nlocals, "stacksize", code.co_stacksize)
        if name == "far":
            # assemble.c relaxes FOR to a three-word bundle. The encoded
            # distance becomes 1 and the next two words are IR_JUMP adapters.
            assert signed_distance == 1
            assert successors == ["IR_JUMP", "IR_JUMP"], successors
        else:
            assert successors != ["IR_JUMP", "IR_JUMP"], successors
        assert b >= code.co_nlocals
        for handler in (FOR, INSTRUMENTED_FOR):
            for label, dst in (("original", a), ("temp B+2", b + 2),
                               ("local 0", 0), ("NoDst", NO_DST),
                               ("invalid temp B+3", b + 3)):
                print(name, opcode.opname[handler], label,
                      construct(code, index, fields, handler, dst))


if __name__ == "__main__":
    main()
