# Candidate 45 disassembly feedback fixture repair

**Proposal only; runtime source is unchanged.** All eight observed `test_dis` failures are the same stale feedback-index expectation, repeated across two test classes and four quickened/adaptive combinations. Update the expected geometry in one fixture; keep the exact assertion and remaining cache/state checks.

## Evidence

Evidence root `E`: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.

The full release log `E/object45-full-release-release-full.log` records eight failures but omits their tracebacks. The coordinator authorized one release-only diagnostic through the existing runner:

```sh
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B \
  pyjit/issues/object-call-20261109/run-checks.py \
  object45-dis-diagnostic focus test_dis --role release --verbose
```

`E/object45-dis-diagnostic-focus-checks.json` records clean source `55b2a87cf51d29f8880114f4a73c9945e9ca73ec`, matching native identity, and return code 2. `E/object45-dis-diagnostic-release-focus.log` shows 143 tests and exactly eight failures. Every traceback ends at `Lib/test/test_dis.py:1371`, in `test_show_caches`, with:

```text
actual:   [15, 19, 23, 2, 5, 11]
expected: [14, 18, 22, 2, 5, 10]
```

The failures cover `(quickened, adaptive)` = `(False, False)`, `(False, True)`, `(True, False)`, `(True, True)` in both `DisTests` and inherited `DisWithFileTests`. The other test methods pass. The cache field/state assertions below this failing assertion were not reached in these subtests; their success still needs the post-patch rerun.

## Geometry proof

This fixture's function creates a list, iterates over `range(42)`, and calls `l.append(i)`. Feedback allocation in `Python/assemble.c:place_entries` puts loop sites before other sites, preserving source order within each class. Each entry advances by `fmt->fb_size >> IR_FB_UNIT_SHIFT`; a unit is eight bytes. Generated `LOAD_ATTR` metadata uses `sizeof(_PyFbAttr)`.

The approved representation repair grows attribute feedback from 40 to 48 bytes, hence five to six units. Header, unary, global and call entries retain their sizes. The allocation is therefore:

| Entry, in feedback allocation order | Old start | New start | Units now |
| --- | ---: | ---: | ---: |
| Header | 0 | 0 | 2 |
| Loop `FOR_ITER` | 2 | 2 | 3 |
| Loop `LOAD_ATTR append` | 5 | 5 | 6 |
| Loop `CALL append` | 10 | 11 | 4 |
| `LOAD_GLOBAL range` | 14 | 15 | 4 |
| `CALL range` | 18 | 19 | 4 |
| `GET_ITER` | 22 | 23 | 3 |

Disassembly walks instruction order, producing `[15, 19, 23, 2, 5, 11]`, exactly the observed list. The separate polymorphic-table growth does not enter this inline entry-index calculation. `Lib/dis.py:_feedback_entry` derives entry spans from adjacent `co_fbinit` entries; `_feedback_info` reads header fields and the first two 32-bit payload fields, whose offsets did not change. No disassembler change is indicated.

## Minimal approved scope requested

Change only `Lib/test/test_dis.py`, replacing `[14, 18, 22, 2, 5, 10]` with `[15, 19, 23, 2, 5, 11]`. Add a brief context comment that loop entries precede the other entries and the attribute entry occupies six eight-byte units. No executable runtime, disassembler, generator, metadata, expected instruction offset, handler or transcript changes.

This is meaningful project disassembler geometry: the exact list catches incorrect entry placement or decoded indices. Keep that assertion exact. It is not an upstream CPython transcript or byte-for-byte compatibility promise. Do not drop the index assertion, normalize offsets away, or generate its expectation from the same decoder under test. Existing checks for cache presence, six displayed feedback entries, field names and specialized state remain intact.

After source-freeze release and coordinator approval, rerun the complete focused `test_dis` module on release and debug through the runner. A clean rerun establishes that the later field/state assertions also pass. No rebuild should be required for this fixture-only change. Preserve candidate 45's failed full-suite evidence; report the repair and focused reruns separately rather than rewriting that result. No additional native jobs, builds or source edits were performed during this diagnosis.
