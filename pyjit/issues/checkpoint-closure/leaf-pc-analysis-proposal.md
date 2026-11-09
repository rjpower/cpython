# Leaf-PC source attribution proposal

Status: bounded tooling proposal and implementation, 2026-11-06. This reads
the retained depth-one profile captures only. It does not sample, benchmark,
change runtime sources or interpret a PC as a caller stack. The coordinator
owns publication of final analysis results.

## Inputs and identity checks

The input directory is the sealed
`profiles-6a0e4c12c17-diagnostic` set: upstream and candidate captures for
`deltablue`, `logging_silent` and `comprehensions`. For every role, use the
retained `<role>-python` ELF. The matching case `command.json` supplies the
original executable pathname. Require that exact absolute pathname in the
capture's maps, compare the retained ELF build ID with the saved
`<role>-elf-notes.txt`, and inspect its program headers. Every executable
`PT_LOAD` must have equal file offset and virtual address; non-executable
segments need not satisfy that relation. The original executable need not
still exist at its former path.

For a PC in the exact main executable mapping, compute
`file_offset = pc - map_start + map_file_offset`. Require that offset to fall
inside a retained executable load. Symbolize only against the retained main
ELF. Other mapped objects stay grouped by exact mapped path and file offset;
anonymous, absent and malformed mappings remain explicit unresolved buckets.

## Attribution rules

Load sized, defined text symbols once per role with `nm -n -S --defined-only`.
Map a PC only when it falls in a symbol's half-open address range. A zero-size
symbol, a PC outside all sized ranges, or a PC covered by distinct overlapping
ranges is unresolved; do not use the nearest preceding symbol. Same-range
aliases form one bucket with all names. Send all uniquely symbolized main
addresses for a role to one batched `addr2line -a -f -i` request.

Every input sample contributes exactly once to one main-symbol, external
path+offset or unresolved bucket. Source-line totals use only the first
innermost `addr2line` location, once per sample. Remaining inline locations
are retained as compile-time inline metadata, not additional samples or
caller frames. Keep the original per-case sample total as the denominator for
all shares.

The analyzer writes JSON with all per-PC counts, full innermost source-line
totals, and bounded top rows, plus a Markdown view of top functions, source
locations, PCs, external mappings and unresolved reasons. It refuses an
existing output directory. Run it as:

```sh
uv run python pyjit/tools/sampler/analyze-leaf-pcs.py \
  --profiles /path/to/profiles-6a0e4c12c17-diagnostic \
  --output /new/path/leaf-pc-analysis
```

## Validation and limits

Validation is limited to syntax compilation, a known retained-ELF symbol
address/range check, and verifying that all six analyzed captures conserve
their input sample counts. No sampling or timing is part of this work. The
first delivery prioritizes top PCs and bounded function attribution; source
and inline rows are reported only where the retained ELF has usable debug
information. External shared-library PCs are not symbolized in this increment.
The result can guide source inspection but cannot establish that a source line
caused a benchmark slowdown or a PyValue box.

Peer review and post-review accepted the size-bounded attribution rules.
The coordinator executed the analyzer on all six retained captures in
`leaf-pc-analysis-6a0e4c12c17/`; every input sample is conserved. The later
hardware captures have their own module identities and native call chains;
they do not retroactively identify the earlier external libraries.
