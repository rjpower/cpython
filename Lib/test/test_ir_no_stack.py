"""The register IR's end condition: no stack reference in the IR or the
interpreter.

The sources of the IR and of the interpreter name no stack opcode (a name in
stack_ops.py) and no stack-shaped table or field, except where ALLOWED says
so, one pattern and one reason each.  Comments are not scanned.  An allowance
that matches no line is stale and fails, so the list shrinks as the
references go.  The translator and the public co_code, dis and pcmap
transport are allowed by file."""

from dataclasses import dataclass
from pathlib import Path
import re
import unittest

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import stack_ops

ROOT = Path(test_tools.basepath)

# Every runtime source is scanned, so that a new file is scanned by default;
# the files that stay on the stack side are excluded by name, one reason each
SCANNED = (
    "Python/*.c", "Python/*.h", "Objects/*.c", "Objects/*.h",
    "Include/internal/*.h", "Python/generated_rop_*.c.h",
    "Python/register_ops/*.rop",
)

EXCLUDED = {
    "Python/compile.c": "the compiler emits stack bytecode",
    "Python/codegen.c": "the compiler emits stack bytecode",
    "Python/flowgraph.c": "the compiler's flow graph is stack bytecode",
    "Python/assemble.c": "the compiler's assembler writes co_code",
    "Python/ast_preprocess.c": "the compiler",
    "Python/ir_assemble.c": "the translator's assembler",
    "Python/regcfg.c": "the translator",
    "Python/regcfg_passes.c": "the translator",
    "Include/internal/pycore_regcfg.h": "the translator",
    "Include/internal/pycore_regcfg_passes.h": "the translator",
    "Include/internal/pycore_opcode_metadata.h": "the opcode schema",
    "Include/internal/pycore_opcode_utils.h": "the opcode schema",
    "Include/internal/pycore_opcode_ids.h": "the opcode schema",
    "Include/opcode_ids.h": "the opcode schema",
    "Include/internal/pycore_flowgraph.h": "the compiler",
    "Include/internal/pycore_compile.h": "the compiler",
    "Include/internal/pycore_instruction_sequence.h": "the compiler",
    "Python/instruction_sequence.c": "the compiler",
    "Python/symtable.c": "the compiler",
    "Python/Python-ast.c": "the generated parser",
    "Include/internal/pycore_ast.h": "the generated parser",
    "Include/internal/pycore_ast_state.h": "the generated parser",
}

# Stack-shaped names that are no opcode: the tables and fields that map a
# handler to a stack op, and the opcode-space helpers of the old design
SHAPED = (
    r"_PyIR_BaseOpcode", r"_PyIR_OpcodeBase", r"_PyIR_HandlerBase",
    r"_PyIR_OpcodeHandler", r"_PyIR_HandlerOpcode", r"_PyIR_StackOnlyFormats",
    r"_PyIR_EventOpcode", r"_PyIR_OriginOpcode", r"\bshadow\b", r"\b_PyStack(?:Format|Op|Opcode)\w*",
    r"_PyOpcode_Deopt", r"_PyOpcode_Caches", r"_PyOpcode_OpName",
    r"MIN_INSTRUMENTED_OPCODE", r"IS_SCOPE_EXIT_OPCODE", r"\bINSTRUMENTED_[A-Z_]+",
)


@dataclass(frozen=True)
class Allowed:
    files: str                      # a glob, relative to the source root
    functions: tuple[str, ...] | None   # the top-level functions, or None for the file
    pattern: str                    # a regex, searched in a line
    reason: str


ALLOWED: tuple[Allowed, ...] = ()


def stack_names() -> set[str]:
    return {op.name for op in stack_ops.STACK_OPS}


def hit_pattern() -> re.Pattern:
    names = sorted(stack_names(), key=len, reverse=True)
    return re.compile(r"\b(" + "|".join(names) + r")\b|" + "|".join(SHAPED))


def strip_comments(text: str, rop: bool) -> str:
    if not rop:
        text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"),
                      text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def scanned_files() -> list[Path]:
    files = set()
    for pattern in SCANNED:
        files.update(ROOT.glob(pattern))
    return sorted(f for f in files if f.relative_to(ROOT).as_posix() not in EXCLUDED)


# A top-level C function starts at column 0 with its name and an open
# parenthesis (the return type is on the line before, or leads the line)
FUNCTION = re.compile(r"^(?:[A-Za-z_][\w \*]*?[ \*])?(\w+)\([^;]*$")


def hits() -> list[tuple[str, str, int, str]]:
    """(file, enclosing top-level function, line number, line)"""
    pattern = hit_pattern()
    found = []
    for path in scanned_files():
        rel = path.relative_to(ROOT).as_posix()
        text = strip_comments(path.read_text(errors="replace"), rel.endswith(".rop"))
        function = ""
        for number, line in enumerate(text.split("\n"), 1):
            match = FUNCTION.match(line)
            if match and not line.startswith(("#", "if", "for", "while", "switch", "return")):
                function = match.group(1)
            if pattern.search(line):
                found.append((rel, function, number, line.strip()))
    return found


def allowed_by(rel: str, function: str, line: str) -> list[Allowed]:
    return [a for a in ALLOWED
            if (ROOT / rel).match(str(ROOT / a.files))
            and (a.functions is None or function in a.functions)
            and re.search(a.pattern, line)]


class NoStackTests(unittest.TestCase):
    def test_scanned_sources_exist(self):
        self.assertGreater(len(scanned_files()), 100)
        for pattern in SCANNED:
            with self.subTest(pattern=pattern):
                self.assertTrue(list(ROOT.glob(pattern)))

    def test_exclusions_are_files_with_reasons(self):
        for name, reason in EXCLUDED.items():
            with self.subTest(name=name):
                self.assertTrue((ROOT / name).is_file())
                self.assertTrue(reason.strip())

    def test_no_unlisted_stack_reference(self):
        unlisted = [f"{rel}:{number} ({function}): {line}"
                    for rel, function, number, line in hits()
                    if not allowed_by(rel, function, line)]
        self.assertEqual(unlisted, [], "a stack reference outside the allowlist")

    def test_allowances_are_used_and_reasoned(self):
        used = set()
        for rel, function, number, line in hits():
            used.update(allowed_by(rel, function, line))
        for allowance in ALLOWED:
            with self.subTest(files=allowance.files, pattern=allowance.pattern):
                self.assertTrue(allowance.reason.strip())
                self.assertIn(allowance, used, "a stale allowance")


if __name__ == "__main__":
    unittest.main()
