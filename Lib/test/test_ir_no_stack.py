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

SCANNED = (
    "Python/ceval*.c", "Python/ceval*.h", "Python/ir*.c",
    "Python/instrumentation.c", "Python/legacy_tracing.c",
    "Objects/frameobject.c", "Objects/codeobject.c",
    "Include/internal/pycore_ir*.h", "Python/generated_rop_*.c.h",
    "Python/ir_targets.h", "Python/register_ops/*.rop",
)

# Stack-shaped names that are no opcode: the tables and fields that map a
# handler to a stack op, and the opcode-space helpers of the old design
SHAPED = (
    r"_PyIR_BaseOpcode", r"_PyIR_OpcodeBase", r"_PyIR_HandlerBase",
    r"_PyIR_OpcodeHandler", r"_PyIR_HandlerOpcode", r"_PyIR_StackOnlyFormats",
    r"_PyIR_EventOpcode", r"\bshadow\b", r"_PyOpcode_Deopt",
    r"_PyOpcode_Caches", r"_PyOpcode_OpName", r"MIN_INSTRUMENTED_OPCODE",
    r"IS_SCOPE_EXIT_OPCODE", r"\bINSTRUMENTED_[A-Z_]+",
)


@dataclass(frozen=True)
class Allowed:
    files: str      # a glob, relative to the source root
    pattern: str    # a regex, searched in a line; ".*" for the whole file
    reason: str


ALLOWED = (
    # The translator lowers stack bytecode: it is the stack side
    Allowed("Python/ir_assemble.c", r".*",
            "the translator's assembler reads and writes stack opcodes"),
    # The tables that map a handler to its stack op, and their definitions
    Allowed("Include/internal/pycore_ir_metadata.h", r"\.shadow = ",
            "the format's shadow column, which the translator reads (WF4 moves it)"),
    Allowed("Include/internal/pycore_ir_metadata.h",
            r"_PyIR_(HandlerBase|OpcodeBase|OpcodeHandler|HandlerOpcode|StackOnlyFormats)\b",
            "the handler-to-stack-op maps and the stack-only rows, for the translator"),
    Allowed("Include/internal/pycore_ir_metadata.h", r"^\[\d+\] = \{ \.flags = ",
            "stack-only format rows, which name their stack op in .shadow"),
    Allowed("Include/internal/pycore_ir.h", r"\bshadow\b",
            "the field of the format that holds the stack op"),
    Allowed("Include/internal/pycore_ir.h", r"_PyIR_BaseOpcode",
            "the map from a handler to its stack op, for the translator and the public views"),
    # Public transport: co_code, dis and the pcmap
    Allowed("Python/ir.c", r"_PyIR_BaseOpcode|_PyOpcode_Deopt|_PyOpcode_OpName|_PyIR_HandlerOpcode|\bshadow\b|return CACHE",
            "the handler-to-opcode map and the pcmap origin, for _opcode and dis"),
    Allowed("Objects/codeobject.c", r"_PyOpcode_Caches|EXTENDED_ARG|LOAD_FAST_AND_CLEAR|GET_ITER|LOAD_ATTR|LOAD_GLOBAL|LOAD_CONST|LOAD_COMMON_CONSTANT|RESUME|CACHE|RAISE_VARARGS|CONSTANT_ASSERTIONERROR",
            "validates, quickens and rewrites the public co_code, which stays stack bytecode"),
    Allowed("Python/instrumentation.c", r"_PyOpcode_Deopt|_PyOpcode_Caches|IS_CONDITIONAL_JUMP_OPCODE",
            "event_offset() reads the public co_code through the pcmap"),
    Allowed("Python/instrumentation.c", r'\[PY_MONITORING_EVENT_\w+\] = "',
            "sys.monitoring event names, which share spellings with stack ops"),
    Allowed("Python/ceval_macros.h", r"#define INSTRUMENTED_JUMP",
            "a macro named for the event of the stack op it replaced"),
    # The .rop sources declare the stack ops their families lower
    Allowed("Python/register_ops/*.rop", r"\bops\(",
            "the family's declaration of the stack ops that it lowers"),
    Allowed("Python/register_ops/*.rop", r"\bevents\(",
            "sys.monitoring event names, which share spellings with stack ops"),
    Allowed("Python/register_ops/*.rop", r'^\s*(review|contract|lowering|test):|^\s*"',
            "review evidence and prose in strings, sealed history"),
)


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
    return sorted(files)


def hits() -> list[tuple[str, int, str]]:
    pattern = hit_pattern()
    found = []
    for path in scanned_files():
        rel = path.relative_to(ROOT).as_posix()
        text = strip_comments(path.read_text(errors="replace"), rel.endswith(".rop"))
        for number, line in enumerate(text.split("\n"), 1):
            if pattern.search(line):
                found.append((rel, number, line.strip()))
    return found


def allowed_by(rel: str, line: str) -> list[Allowed]:
    return [a for a in ALLOWED
            if (ROOT / rel).match(str(ROOT / a.files)) and re.search(a.pattern, line)]


class NoStackTests(unittest.TestCase):
    def test_scanned_sources_exist(self):
        files = scanned_files()
        self.assertGreater(len(files), 40)
        for pattern in SCANNED:
            with self.subTest(pattern=pattern):
                self.assertTrue(list(ROOT.glob(pattern)))

    def test_no_unlisted_stack_reference(self):
        unlisted = [f"{rel}:{number}: {line}" for rel, number, line in hits()
                    if not allowed_by(rel, line)]
        self.assertEqual(unlisted, [], "a stack reference outside the allowlist")

    def test_allowances_are_used_and_reasoned(self):
        used = set()
        for rel, number, line in hits():
            used.update(allowed_by(rel, line))
        for allowance in ALLOWED:
            with self.subTest(files=allowance.files, pattern=allowance.pattern):
                self.assertTrue(allowance.reason.strip())
                self.assertIn(allowance, used, "a stale allowance")


if __name__ == "__main__":
    unittest.main()
