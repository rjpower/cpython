# Integration C: final attribute publication

Runtime commit `7780e32e0e2`; the worktree is clean. Release, debug and stats
build identities match this source, with 117 modules each. Cold review C
accepts the integrated source with no blocking finding
([review](cold-c-review.md)). The basic matched
screen is complete and mixed, with no broad win established. Release and debug
suites both passed 54,361 tests with no failures (release: 2,467 skips in
3m05s; debug: 2,405 skips in 9m11s). Each profile reports 537 test files
passing, 79 skipped and 11 resource-denied. Integration C passed the
functional suite gate. Final-v4 measurements are complete: candidate / upstream
and candidate / interim elapsed factors are 1.263136 / 1.054877 on 29 kernels
and 1.084920 / 0.902391 on the declared 12-result subset. They establish no
convincing broad elapsed-time win or upstream parity. The [final report](../../bench/results/interpreter-throughput-20261106.md)
contains the full evidence, limits and retained collection history.

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

The retry-only canonical `measure.py` helper is committed as `d9cd6c08fb7`,
accepted by [source review](measurement-retry-source-review.md), and passed
eight focused fixtures. Its role was limited to retaining all attempts and
retrying exact strict load-guard failures. The report records the rejected
prefixes and accepted final-v4 schedule. Next work and admission boundaries
are in the [throughput follow-up](follow-up.md).

Evidence is retained under
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-throughput-20261106/`.
