"""The inline audit of precise C frames (boil/design/11-precise-roots.md,
section 2, "The inline audit").

A function marked Py_GC_PRECISE runs in a frame the collector does not
scan; root_check's C mode checks its source.  If the compiler inlines an
unchecked function into it, the inlined function's locals live in that
frame unchecked.  This audit reads the DWARF of a linked image (or an
object file) and fails unless every function inlined into a function in
section py_gc_precise is itself marked precise or is non-collecting by the
C mode's classification, which consults the tables and the bodies of the
static inline functions in Include/ and of the functions in the marked
sources.  It also fails on:

- an image without .debug_info;
- a function symbol in the section with no DWARF entry;
- a .cold part of a precise function, or a clone of one (.isra,
  .constprop, .part) outside the section.

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
CORE_DIRS = ("Objects", "Python", "Modules", "Parser")
HEADER_DIR = "Include/internal"
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
    section: tuple[int, int] | None          # [start, end) in the image
    section_index: int | None
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
    section = index = None
    for line in sections.splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)\s+\S+\s+([0-9a-f]+)\s+[0-9a-f]+"
                     r"\s+([0-9a-f]+)", line)
        if not m:
            continue
        if m.group(2) == ".debug_info":
            has_debug = True
        if m.group(2) == SECTION:
            index = int(m.group(1))
            start = int(m.group(3), 16)
            section = (start, start + int(m.group(4), 16))
    img = Image(relocatable, has_debug, section, index)
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
            r = re.match(r"(0x[0-9a-f]+|\d+)", val)
            if r:
                current.low_pc = int(r.group(1), 0)
        elif key == "DW_AT_ranges":
            current.ranges = True
    finish(current, current_depth)
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
    """The core .c files and internal headers that use the marker: a static
    inline function marked in a header is precise wherever it is inlined."""
    files = []
    dirs = [(d, "*.c") for d in CORE_DIRS] + [(HEADER_DIR, "*.h")]
    for d, pattern in dirs:
        for p in sorted((ROOT / d).rglob(pattern)):
            try:
                if re.search(r"\bPy_GC_PRECISE\b", p.read_text(errors="replace")):
                    files.append(p)
            except OSError:
                continue
    return files


def source_tables(sources: list[Path]) -> tuple[set[str], root_check_c.Classifier]:
    """The functions marked precise in the sources, and a classifier that
    knows the headers' inline bodies and the sources' functions."""
    bodies, _ = root_check_c.header_inline_bodies(
        root_check_c.default_include_dirs())
    bodies = dict(bodies)
    precise: set[str] = set()
    for p in sources:
        toks = root_check_c.tokens_of(p.read_text(errors="replace"))
        funcs, names, _ = root_check_c.find_functions(toks)
        precise |= names
        for fn in funcs:
            bodies[fn.name] = fn.body_toks
        bodies.update(root_check_c.macro_bodies(toks))
    return precise, root_check_c.Classifier(bodies)


def audit(path: str, sources: list[Path]) -> list[str]:
    img = read_image(path)
    if not img.has_debug_info:
        return [f"{path}: no .debug_info; the audit needs -g"]
    precise_src, classifier = source_tables(sources)
    in_section = {name for name, syms in img.symbols.items()
                  if any(idx == img.section_index for _, idx in syms)}
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
        elif not any(idx == img.section_index for _, idx in syms):
            errors.append(f"{path}: '{name}', a clone of precise '{base}', is "
                          f"outside section {SECTION}")
    if img.section is None:
        return errors
    subprograms, candidates = read_dwarf(path, img)
    start, end = img.section
    found: set[str] = set()
    for d, inlines in candidates:
        name = name_of(d, subprograms)
        if img.relocatable or d.low_pc is None or (d.low_pc == 0 and d.ranges):
            is_precise = name in in_section
        else:
            is_precise = start <= d.low_pc < end
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
            if classifier.kind(iname) == "none":
                continue
            errors.append(f"{path}: precise '{name}' inlines '{iname}', which "
                          "is neither precise nor non-collecting")
    for name in sorted(in_section - found):
        if not CLONE.match(name):
            errors.append(f"{path}: '{name}' is in section {SECTION} but has "
                          "no DWARF entry")
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
