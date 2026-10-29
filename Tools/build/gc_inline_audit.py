"""The inline audit of precise C frames (boil/design/11-precise-roots.md,
section 2, "The inline audit").

A function marked Py_GC_PRECISE runs in a frame the collector does not
scan; root_check's C mode checks its source.  If the compiler inlines an
unchecked function into it, the inlined function's locals live in that
frame unchecked.  This audit reads the DWARF of a linked image (or an
object file) and fails unless every function inlined into a function in
section py_gc_precise (or py_gc_precise_pn, the preserve_none handlers of
the tail-call interpreter) is itself marked precise, is non-collecting by the
C mode's classification, or has a body that the C mode passes as a precise
function's.  The classification consults the tables and the bodies of the
static inline functions in Include/ and of the functions in the marked
sources; the bodies checked are theirs.  It also fails on:

- an image without .debug_info;
- a function symbol in the section with no DWARF entry;
- a .cold part of a precise function, or a clone of one (.isra,
  .constprop, .part) outside the section;
- in a linked image, a static function that code in the precise sections
  calls or jumps to, that the C mode classifies as collecting, and that
  this build left outlined outside the sections, unless OUTLINED_OPAQUE
  gives the reason its frame may be opaque.  A build that does not inline
  a static callee (debug and ASan, at -Og) gives it a frame of its own
  between the precise caller's and the collector's, which the stack walk
  scans; a stale word there keeps a dead object alive (10a-2b.md, the
  outlined-callee audit).

Usage: gc_inline_audit.py IMAGE [SOURCE...]
With no SOURCE, the marked sources are the core .c files that contain
Py_GC_PRECISE.  Exit status 1 with one line per finding.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Tools" / "cases_generator"))
import root_check_c  # noqa: E402

SECTION = "py_gc_precise"
# The precise sections: the second is Py_GC_PRECISE_PRESERVE_NONE's
SECTIONS = (SECTION, "py_gc_precise_pn")
CORE_DIRS = ("Objects", "Python", "Modules", "Parser")
HEADER_DIR = "Include"
# Static callees of precise functions whose outlined frames may stay
# opaque, and why.  Each other one is marked Py_GC_PRECISE.
OUTLINED_OPAQUE = {
    "_PyValue_ObjectRooted":
        "a span primitive: it pushes the root that its caller pops, and "
        "holds only the inline value across its box's allocation",
    "_PyValueRoot_PushObject":
        "a span primitive, as _PyValue_ObjectRooted() is: it pushes the root "
        "that its caller pops, and holds only the inline value across its "
        "box's allocation",
    "collect":
        "the collection, which runs below the collector's entry: the walk "
        "of the collecting thread's stack starts above it "
        "(collect_from_entry()); Py_NO_INLINE, so that no build folds it "
        "into collect_pass()'s precise frame",
    "set_update_local":
        "Py_NO_INLINE: its static callees, the merges, the resize and the "
        "entry adds, are not precise, so it is opaque in every build",
    "borrow_across_bare":
        "a test fixture whose frame is opaque by design "
        "(test_gc.test_borrow_across_collection)",
    "lltrace_instruction":
        "debug tracing, which runs only under PYTHON_LLTRACE",
    "lltrace_resume_frame":
        "debug tracing, which runs only under PYTHON_LLTRACE",
    "run_queue_ex":
        "the existing opaque queue body, renamed by step 7a: its saved "
        "exception and active live_call payload rely on this frame; removal "
        "requires rung 3c's explicit spans and active-call/saved-exception "
        "lifetime fixtures. The run_queue wrapper has no managed locals "
        "and is precise",
    "update_slot_after_setattr":
        "Py_NO_INLINE: the slot update's walk over the subclasses, "
        "update_subclasses() and its callback, is not precise; removal: "
        "rung 3's conversion list",
    "object_set_class_world_stopped":
        "Py_NO_INLINE: __class__ assignment's layout checks and its entry "
        "into dict mode are not precise; removal: rung 3's conversion list",
    "fromitems_setitem":
        "Py_NO_INLINE: the dict's insertion core, setitem_value_lock_held() "
        "with the key's hash and insertdict(), is not precise "
        "(Objects/dictobject.c); removal: rung 3's conversion list",
    "vectorcall_call_dict":
        "opaque until _PyStack_UnpackDict() returns its names in a span "
        "(Objects/call.c)",
}
CLONE = re.compile(r"^(.*?)\.(cold|isra|constprop|part|lto_priv)(\.\d+)*$")
DIE = re.compile(r"^\s*<(\d+)><([0-9a-f]+)>: Abbrev Number: (\d+)(?: \((\w+)\))?")
ATTR = re.compile(r"^\s*<[0-9a-f]+>\s+(DW_AT_\w+)\s*:\s*(.*)$")


@dataclass
class Die:
    offset: int
    tag: str
    name: str | None = None
    origin: int | None = None       # DW_AT_abstract_origin or specification
    low_pc: int | None = None
    ranges: bool = False


@dataclass
class Image:
    relocatable: bool
    has_debug_info: bool
    # The precise sections' [start, end) in the image, by section index
    sections: dict[int, tuple[int, int]]
    symbols: dict[str, list[tuple[int, int]]] = field(default_factory=dict)
    # name -> [(value, section index)]


def run(cmd: list[str]) -> str:
    return subprocess.run(cmd, check=True, capture_output=True,
                          text=True).stdout


def read_image(path: str) -> Image:
    header = run(["readelf", "-hW", path])
    relocatable = re.search(r"Type:\s+REL\b", header) is not None
    sections = run(["readelf", "-SW", path])
    has_debug = False
    precise: dict[int, tuple[int, int]] = {}
    for line in sections.splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+"
                     r"\s+([0-9a-f]+)", line)
        if not m:
            continue
        if m.group(2) == ".debug_info":
            has_debug = True
        if m.group(2) in SECTIONS:
            start = int(m.group(3), 16)
            precise[int(m.group(1))] = (start, start + int(m.group(4), 16))
    img = Image(relocatable, has_debug, precise)
    for line in run(["readelf", "-sW", path]).splitlines():
        parts = line.split()
        if len(parts) < 8 or parts[3] != "FUNC" or not parts[6].isdigit():
            continue
        img.symbols.setdefault(parts[7].split("@")[0], []).append(
            (int(parts[1], 16), int(parts[6])))
    return img


def read_dwarf(path: str, img: Image) -> tuple[dict[int, Die],
                                               list[tuple[Die, list[Die]]]]:
    """All subprogram DIEs by offset, and each precise subprogram with the
    inlined subroutines anywhere below it."""
    subprograms: dict[int, Die] = {}
    candidates: list[tuple[Die, list[Die]]] = []
    stack: list[tuple[int, Die, list[Die] | None]] = []   # depth, die, inlines
    current: Die | None = None
    current_depth = 0

    def close(depth: int) -> None:
        while stack and stack[-1][0] >= depth:
            stack.pop()

    def finish(d: Die | None, depth: int) -> None:
        if d is None:
            return
        inlines = None
        if d.tag == "DW_TAG_subprogram":
            subprograms[d.offset] = d
            if d.low_pc is not None or d.ranges:
                inlines = []
                candidates.append((d, inlines))
        elif d.tag == "DW_TAG_inlined_subroutine":
            for _, _, outer in reversed(stack):
                if outer is not None:
                    outer.append(d)
                    break
        stack.append((depth, d, inlines))

    proc = subprocess.Popen(["readelf", "--debug-dump=info", path],
                            stdout=subprocess.PIPE, text=True,
                            errors="replace")
    assert proc.stdout is not None
    for line in proc.stdout:
        m = DIE.match(line)
        if m:
            finish(current, current_depth)
            depth = int(m.group(1))
            close(depth)
            tag = m.group(4)
            current_depth = depth
            current = Die(int(m.group(2), 16), tag) if tag else None
            continue
        if current is None:
            continue
        a = ATTR.match(line)
        if not a:
            continue
        key, val = a.group(1), a.group(2).strip()
        if key == "DW_AT_name":
            current.name = val.rsplit("): ", 1)[-1] if val.startswith("(") else val
        elif key in ("DW_AT_abstract_origin", "DW_AT_specification"):
            r = re.search(r"<0x([0-9a-f]+)>", val)
            if r and current.origin is None:
                current.origin = int(r.group(1), 16)
        elif key == "DW_AT_low_pc":
            # DWARF 5 (clang) gives an index into .debug_addr, then the value
            if val.startswith("("):
                val = val.rsplit("): ", 1)[-1]
            r = re.match(r"(0x[0-9a-f]+|\d+)", val)
            if r:
                current.low_pc = int(r.group(1), 0)
        elif key == "DW_AT_ranges":
            current.ranges = True
    finish(current, current_depth)
    proc.stdout.close()
    if proc.wait() != 0:
        raise SystemExit(f"readelf failed on {path}")
    return subprograms, candidates


def name_of(d: Die, subprograms: dict[int, Die]) -> str | None:
    seen = set()
    while d.name is None and d.origin is not None and d.origin not in seen:
        seen.add(d.origin)
        nxt = subprograms.get(d.origin)
        if nxt is None:
            return None
        d = nxt
    return d.name


def marked_sources() -> list[Path]:
    """The core .c files and the headers that use the marker: a static
    inline function marked in a header is precise wherever it is inlined,
    and so is its outlined copy."""
    files = []
    dirs = ([(d, "*.c") for d in CORE_DIRS] + [(d, "*.h") for d in CORE_DIRS]
            + [(HEADER_DIR, "*.h")])
    for d, pattern in dirs:
        for p in sorted((ROOT / d).rglob(pattern)):
            try:
                if re.search(r"\bPy_GC_PRECISE\b", p.read_text(errors="replace")):
                    files.append(p)
            except OSError:
                continue
    return files


@dataclass
class Tables:
    precise: set[str]
    classifier: root_check_c.Classifier
    functions: dict[str, root_check_c.Function]
    typedefs: set[str]
    returns: dict[str, str]
    args_anchored: set[str]
    checked: dict[str, list[root_check_c.Error]] = field(default_factory=dict)

    def check(self, name: str) -> list[root_check_c.Error] | None:
        """The C mode's errors in name's body checked as precise, or None if
        there is no body."""
        if name not in self.checked:
            fn = self.functions.get(name)
            if fn is None:
                return None
            self.checked[name] = root_check_c.check_function(
                fn, self.classifier, self.typedefs, self.returns,
                self.precise, self.args_anchored)
        return self.checked[name]


def private_headers(sources: list[Path]) -> list[Path]:
    """The headers beside the sources that they include, such as
    Python/ceval_macros.h: their static inline functions are inlined into
    precise ones too."""
    found: set[Path] = set()
    for p in sources:
        text = p.read_text(errors="replace")
        for m in re.finditer(r'^\s*#\s*include\s*"([^"]+)"', text, re.M):
            name = m.group(1)
            h = p.parent / name
            if name.endswith(".h") and not name.endswith(".c.h") and h.is_file():
                found.add(h)
    return sorted(found)


def source_tables(sources: list[Path]) -> Tables:
    """The functions marked precise in the sources, a classifier that knows
    the headers' inline bodies and the sources' functions, and what the C
    mode needs to check those bodies."""
    inc = root_check_c.default_include_dirs() + private_headers(sources)
    bodies, header_returns = root_check_c.header_inline_bodies(inc)
    bodies = dict(bodies)
    functions = dict(root_check_c.header_inline_functions(inc))
    typedefs = set(root_check_c.header_typedefs(inc))
    returns = dict(header_returns)
    precise: set[str] = set()
    for p in sources:
        toks = root_check_c.tokens_of(p.read_text(errors="replace"))
        funcs, names, rets = root_check_c.find_functions(toks)
        precise |= names
        returns.update(rets)
        typedefs |= root_check_c.typedef_names(toks)
        for fn in funcs:
            bodies[fn.name] = fn.body_toks
            functions[fn.name] = fn
        bodies.update(root_check_c.macro_bodies(toks))
    args_anchored = {n for n, fn in functions.items() if fn.args_anchored}
    return Tables(precise, root_check_c.Classifier(bodies), functions,
                  typedefs, returns, args_anchored)


CALL = re.compile(r"^\s*[0-9a-f]+:\s+(?:call|jmp)\w*\s+[0-9a-f]+ "
                  r"<([^>+]+)>\s*$")
FUNC_START = re.compile(r"^[0-9a-f]+ <([^>]+)>:$")


def outlined_callees(path: str, img: Image) -> dict[str, set[str]]:
    """The local functions outside the precise sections that code in them
    calls or jumps to, by name, with their precise callers."""
    local: dict[str, set[int]] = {}
    for line in run(["readelf", "-sW", path]).splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[3] == "FUNC" and parts[4] == "LOCAL" \
                and parts[6].isdigit():
            local.setdefault(parts[7], set()).add(int(parts[6]))
    present = set(re.findall(r"\]\s+(\S+)\s", run(["readelf", "-SW", path])))
    found: dict[str, set[str]] = {}
    for sec in SECTIONS:
        if sec not in present:
            continue
        current = None
        for line in run(["objdump", "-d", "--no-show-raw-insn", "-j", sec,
                         path]).splitlines():
            m = FUNC_START.match(line)
            if m:
                current = m.group(1)
                continue
            m = CALL.match(line)
            if not m or current is None:
                continue
            target = m.group(1)
            idxs = local.get(target)
            if target == current or not idxs or idxs & img.sections.keys():
                continue
            found.setdefault(target, set()).add(current)
    return found


def audit(path: str, sources: list[Path]) -> list[str]:
    img = read_image(path)
    if not img.has_debug_info:
        return [f"{path}: no .debug_info; the audit needs -g"]
    tables = source_tables(sources)
    precise_src = tables.precise
    in_section = {name for name, syms in img.symbols.items()
                  if any(idx in img.sections for _, idx in syms)}
    errors: list[str] = []
    # Parts and clones of precise functions
    for name, syms in img.symbols.items():
        m = CLONE.match(name)
        if not m:
            continue
        base = m.group(1)
        if base not in in_section and base not in precise_src:
            continue
        if m.group(2) == "cold":
            errors.append(f"{path}: '{name}' is a cold part split out of "
                          f"precise '{base}'")
        elif not any(idx in img.sections for _, idx in syms):
            errors.append(f"{path}: '{name}', a clone of precise '{base}', is "
                          f"outside section {SECTION}")
    if not img.sections:
        return errors
    subprograms, candidates = read_dwarf(path, img)
    found: set[str] = set()
    for d, inlines in candidates:
        name = name_of(d, subprograms)
        if img.relocatable or d.low_pc is None or (d.low_pc == 0 and d.ranges):
            is_precise = name in in_section
        else:
            is_precise = any(start <= d.low_pc < end
                             for start, end in img.sections.values())
        if not is_precise:
            continue
        found.add(name or "")
        for inl in inlines:
            origin = subprograms.get(inl.origin) if inl.origin is not None else None
            iname = name_of(origin, subprograms) if origin is not None else None
            if iname is None:
                errors.append(f"{path}: precise '{name}' inlines a function "
                              "with no name in the DWARF")
                continue
            if iname in precise_src or iname in in_section:
                continue
            if tables.classifier.kind(iname) == "none":
                continue
            if iname in root_check_c.SPAN_PRIMITIVES:
                continue
            # Its locals live in the precise frame: its body is checked as
            # a precise function's (10a-2.md, rule 6).
            found_errors = tables.check(iname)
            if found_errors == []:
                continue
            if found_errors is None:
                errors.append(f"{path}: precise '{name}' inlines '{iname}', "
                              "which is collecting and has no body the C "
                              "mode can check")
                continue
            for e in found_errors:
                errors.append(f"{path}: precise '{name}' inlines '{iname}', "
                              f"whose body fails the C mode: {e.message}")
    for name in sorted(in_section - found):
        if not CLONE.match(name):
            errors.append(f"{path}: '{name}' is in section {SECTION} but has "
                          "no DWARF entry")
    if img.relocatable:
        return errors
    for target, callers in sorted(outlined_callees(path, img).items()):
        m = CLONE.match(target)
        base = m.group(1) if m else target
        fn = tables.functions.get(base)
        if fn is None or not fn.static or base in OUTLINED_OPAQUE:
            continue
        if tables.classifier.kind(base) == "none":
            continue
        errors.append(f"{path}: static '{target}', which precise "
                      f"{', '.join(repr(c) for c in sorted(callers))} "
                      "calls, is outlined and collecting, and is not "
                      "marked Py_GC_PRECISE or listed in OUTLINED_OPAQUE")
    return errors


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("image")
    ap.add_argument("sources", nargs="*", type=Path)
    args = ap.parse_args(argv)
    sources = args.sources or marked_sources()
    errors = audit(args.image, sources)
    for e in errors:
        print(e)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
