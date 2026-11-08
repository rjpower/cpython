"""List the opaque functions that inline a collecting precise function.

A function marked Py_GC_PRECISE keeps its objects in spans, and the stack
walk does not scan its frame.  If the compiler inlines it into a function
that is not marked, its locals live in that opaque frame, which the walk
scans conservatively: a stale word there keeps a dead object alive.
Marking a callee without its caller is a half form (10a-2b.md, the item 1a
hazard).  This lists each such caller with the precise functions inlined
into it; the C mode's non-collecting functions are left out.  By default
the callees are the functions of the marked .c files.  With --headers it
also counts the static inline functions of the headers, such as
PyValue_Object(), which nearly every opaque function inlines: that list is
the backlog of the conversion, not a set of half forms.

It reads the DWARF of a linked image or an object file with the inline
audit's reader (Tools/build/gc_inline_audit.py).  The hits are a worklist,
not a gate: some callers stay opaque by ruling, such as the dealloc paths,
which run in the sweep, where no collection starts.

Usage: opaque_inliners.py [--headers] IMAGE    (prints
"caller: callee, ..." sorted by caller; exit status 0)
"""
import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Tools" / "build"))
import gc_inline_audit as audit
import root_check_c


def source_functions(sources: list[Path]) -> set[str]:
    """The functions defined in the marked .c files."""
    names: set[str] = set()
    for p in sources:
        if p.suffix == ".c":
            toks = root_check_c.tokens_of(p.read_text(errors="replace"))
            names |= {fn.name for fn in root_check_c.find_functions(toks)[0]}
    return names


def opaque_inliners(path: str, headers: bool) -> dict[str, set[str]]:
    img = audit.read_image(path)
    sources = audit.marked_sources()
    tables = audit.source_tables(sources)
    in_c = source_functions(sources)
    in_section = {name for name, syms in img.symbols.items()
                  if any(idx in img.sections for _, idx in syms)}
    precise = in_section | set(tables.precise)
    subprograms, candidates = audit.read_dwarf(path, img)
    found: dict[str, set[str]] = {}
    for d, inlines in candidates:
        name = audit.name_of(d, subprograms)
        if img.relocatable or d.low_pc is None or (d.low_pc == 0 and d.ranges):
            is_precise = name in in_section
        else:
            is_precise = any(start <= d.low_pc < end
                             for start, end in img.sections.values())
        if is_precise or name is None:
            continue
        for inl in inlines:
            origin = subprograms.get(inl.origin) if inl.origin is not None else None
            iname = audit.name_of(origin, subprograms) if origin is not None else None
            if (iname in precise and (headers or iname in in_c)
                    and tables.classifier.kind(iname) != "none"):
                found.setdefault(name, set()).add(iname)
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--headers", action="store_true")
    ap.add_argument("image")
    args = ap.parse_args()
    found = opaque_inliners(args.image, args.headers)
    for caller, callees in sorted(found.items()):
        print(f"{caller}: {', '.join(sorted(callees))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
