# Integration C: final attribute publication

Runtime commit `7780e32e0e2`; the worktree is clean. Release, debug and stats
build identities match this source, with 117 modules each. Cold review C
accepts the integrated source with no blocking finding
([review](cold-c-review.md)). The basic matched
screen is complete and mixed, with no broad win established. Release and debug
suites both passed 54,361 tests with no failures (release: 2,467 skips in
3m05s; debug: 2,405 skips in 9m11s). Across each, 616/627 files passed and 79
were skipped, including 11 resource-denied skips. Integration C has passed the
functional suite gate. The final 29-kernel and pyperformance comparisons are
running serially, so no performance conclusion is available yet.

The reviewed change reuses the existing `published_temporary_result`
postcondition for the five specialized attribute-load retirement flows. Their
result is first published before receiver release. Once retirement completes,
the handler's unchanged result/home postcondition lets the final flush omit a
second temporary-destination write. A folded-local destination still receives
its final `IR_STORE_DST`; displaced-local retirement and failure cleanup remain
in place. No new cache, opcode flag or general publication tracker was added.

The regenerated diff has exactly five final-store wrappers changed to the
local-only form; generated metadata is unchanged. The focused generator unit
tests pass (2 tests). All three build profiles pass and each reports 117
modules. Each release/debug focused run passes 729 tests (release skips 11;
debug skips 10). The new before/after-handler semantic fixture passes in both
release and debug. The subsequent full release/debug suites pass as recorded
above. The five specialized handlers' measured generated C footprint changes
from 6,313 to 5,668 bytes, a source/code-size reduction of 645 bytes. This
static footprint fact is not a dynamic cost or performance measurement.

The completed basic comparison is mixed and supports no broad performance
claim. The final matched kernel and pyperformance campaign is running serially;
do not infer speedup from the generated-store or code-size reduction. The next
work and admission boundaries are in the [throughput follow-up](follow-up.md).

Evidence is retained under
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-throughput-20261106/`.
