"""Static code-object statistics over compiled modules.

Measures, for every code object reachable from the module code of each source
file (walking ``co_consts`` recursively): instruction count, ``co_stacksize``,
``co_nlocalsplus``, constants, specialisable instructions (those with inline
cache entries), longest jump and ``co_code`` bytes.

Invariants: files are compiled with ``compile()`` by the interpreter under
test, run in a subprocess so that opcode metadata matches that interpreter.
Files that fail to compile are counted in ``skipped`` and never abort the run.
Cache entries are not counted as instructions; ``co_code`` bytes include them.
``sizeof`` sums ``sys.getsizeof(code)``, which includes the typed feedback
block on interpreters that have one.

On a build with the register IR (``--with-ir-interp``), each code object is
also lowered: ``ir_words`` counts its words, IR_EXT and second words included,
``ir_absorbed`` its absorbed loads, and ``ir_bytes`` the growth of
``sys.getsizeof(code)`` that lowering causes (words, feedback and pcmap).
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_INTERPRETER = REPO / "build" / "baseline-default" / "python"

# Runs inside the interpreter under test and prints one JSON document.
WORKER = r'''
import dis, json, opcode, sys
from pathlib import Path
from types import CodeType

try:
    from _testinternalcapi import get_ir
except ImportError:
    get_ir = None

CACHE = opcode._inline_cache_entries
JUMPS = set(dis.hasjump)
KEYS = ("instructions", "stacksize", "nlocalsplus", "consts", "names",
        "specialisable", "longest_jump", "co_code_bytes", "slots")
IR_KEYS = ("ir_instructions", "ir_words", "ir_ext", "ir_absorbed", "ir_bytes")
SUM_KEYS = ("instructions", "specialisable", "co_code_bytes", "sizeof") + (
    IR_KEYS if get_ir else ())


def walk(code):
    yield code
    for c in code.co_consts:
        if isinstance(c, CodeType):
            yield from walk(c)


def nlocalsplus(code):
    # Not exposed to Python: locals, plus cells that are not arguments, plus frees.
    local_and_cell = set(code.co_varnames) | set(code.co_cellvars)
    return len(local_and_cell) + len(code.co_freevars)


def stats(code):
    ninstr = nspec = longest = 0
    for ins in dis.get_instructions(code, show_caches=False):
        ninstr += 1
        if CACHE.get(ins.opname, 0):
            nspec += 1
        if ins.opcode in JUMPS and isinstance(ins.argval, int):
            longest = max(longest, abs(ins.argval - ins.offset) // 2)
    nlp = nlocalsplus(code)
    return {
        "instructions": ninstr,
        "stacksize": code.co_stacksize,
        "nlocalsplus": nlp,
        "consts": len(code.co_consts),
        "names": len(code.co_names),
        "specialisable": nspec,
        "longest_jump": longest,
        "co_code_bytes": len(code.co_code),
        "slots": nlp + code.co_stacksize,
        "sizeof": sys.getsizeof(code),
    }


def ir_stats(code):
    before = sys.getsizeof(code)
    ir = get_ir(code)
    # get_ir folds an IR_EXT word into the fields that it widens
    ext = sum(max(a, b, c) > 0xFFFF for _, _, _, a, b, c, _ in ir)
    return {
        "ir_instructions": len(ir),
        "ir_words": len(ir) + ext + sum(word1 is not None for *_, word1 in ir),
        "ir_ext": ext,
        "ir_absorbed": sum(name.startswith("IR_ABSORBED") for _, _, name, *_ in ir),
        "ir_bytes": sys.getsizeof(code) - before,
    }


def main():
    files = [Path(p) for p in sys.stdin.read().split("\n") if p]
    totals = dict.fromkeys(SUM_KEYS, 0)
    maxima = dict.fromkeys(KEYS, 0)
    argmax = {}
    ncode = skipped = 0
    for f in files:
        try:
            top = compile(f.read_bytes(), str(f), "exec")
        except (SyntaxError, ValueError, RecursionError, MemoryError):
            skipped += 1
            continue
        for code in walk(top):
            s = stats(code)
            if get_ir:
                s.update(ir_stats(code))
            ncode += 1
            for k in SUM_KEYS:
                totals[k] += s[k]
            for k in KEYS:
                if s[k] > maxima[k]:
                    maxima[k] = s[k]
                    argmax[k] = f"{f}:{code.co_qualname}"
    print(json.dumps({"files": len(files), "skipped": skipped,
                      "code_objects": ncode, "totals": totals,
                      "maxima": maxima, "argmax": argmax}))


main()
'''


def lib_dir(interpreter: Path) -> Path:
    """Return the Lib directory of the source tree the interpreter was built from."""
    out = subprocess.run(
        [str(interpreter), "-c", "import sysconfig;print(sysconfig.get_config_var('abs_srcdir') or '')"],
        capture_output=True, text=True, check=True).stdout.strip()
    src = Path(out) if out else REPO
    return src / "Lib" if (src / "Lib").is_dir() else REPO / "Lib"


def source_files(lib: Path, with_tests: bool) -> list[Path]:
    """Return every .py file under lib, excluding Lib/test unless requested."""
    files = sorted(lib.rglob("*.py"))
    if with_tests:
        return files
    test_dir = lib / "test"
    return [f for f in files if test_dir not in f.parents]


def collect(interpreter: Path, files: list[Path]) -> dict:
    """Run the worker under the interpreter and return its JSON result."""
    proc = subprocess.run(
        [str(interpreter), "-X", "utf8", "-c", WORKER],
        input="\n".join(map(str, files)), capture_output=True, text=True, check=False)
    if proc.returncode:
        raise RuntimeError(proc.stderr)
    return json.loads(proc.stdout)


def summary(name: str, r: dict) -> str:
    """Return a human summary."""
    t, m = r["totals"], r["maxima"]
    share = 100 * t["specialisable"] / t["instructions"]
    lines = [
        f"[{name}] files={r['files']} skipped={r['skipped']} code_objects={r['code_objects']}",
        f"  instructions={t['instructions']} co_code_bytes={t['co_code_bytes']}",
        f"  specialisable share={share:.1f}%",
        f"  bytes per code object: sizeof={t['sizeof'] / r['code_objects']:.0f}"
        f" co_code={t['co_code_bytes'] / r['code_objects']:.0f}",
        f"  max nlocalsplus+stacksize={m['slots']} (nlocalsplus {m['nlocalsplus']}, stacksize {m['stacksize']})",
        f"  max nlocalsplus={m['nlocalsplus']} max stacksize={m['stacksize']}",
        f"  max consts={m['consts']} max names={m['names']}",
        f"  max specialisable in one code object={m['specialisable']}",
        f"  longest jump={m['longest_jump']} code units",
        f"  max instructions in one code object={m['instructions']}",
        "  design note: 70/803 slots, 4048 consts, 368 names, 5458 jump, 510/1181 specialisable, 32%",
    ]
    if "ir_words" in t:
        units = t["co_code_bytes"] / 2
        lines.append(
            f"  IR: words per instruction={t['ir_words'] / t['ir_instructions']:.3f}"
            f" IR_EXT={t['ir_ext']} absorbed loads={100 * t['ir_absorbed'] / t['ir_instructions']:.1f}%"
            f" block bytes per code unit={t['ir_bytes'] / units:.2f}")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--interpreter", type=Path, default=DEFAULT_INTERPRETER)
    ap.add_argument("--lib", type=Path, help="Lib directory (default: interpreter's source tree)")
    ap.add_argument("--tests", action="store_true", help="also include Lib/test (reported separately)")
    ap.add_argument("--json", type=Path, help="write JSON here (default: stdout)")
    args = ap.parse_args()
    lib = args.lib or lib_dir(args.interpreter)
    result = {"lib": str(lib), "no_tests": collect(args.interpreter, source_files(lib, False))}
    print(summary("no tests", result["no_tests"]), file=sys.stderr)
    if args.tests:
        result["with_tests"] = collect(args.interpreter, source_files(lib, True))
        print(summary("with tests", result["with_tests"]), file=sys.stderr)
    text = json.dumps(result, indent=2)
    if args.json:
        args.json.write_text(text + "\n")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
