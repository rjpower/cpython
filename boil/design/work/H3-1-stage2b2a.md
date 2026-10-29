# H3-1 stage 2b.2a: immutable safe line lookup

Status: implemented and frozen for independent review from 617bea5be1b,
whose native source is c5cf6ba6d28. ROOTS returned IMPLEMENT_2A and confirmed
the precise frame-to-code-to-linetable chain, including detached states.
Coordinator/ROOTS granted exactly code_safe_addr2line(code, int) after get_ir
and its method entry. The additional pycore_code.h declaration change to
PyAPI_FUNC(int) is separately approved: the real helper was hidden from the
shared test extension. This is private test-symbol visibility, not a stable
ABI guarantee. No storage helper or instruments include is added.

Owned source: codeobject.c's safe lookup, that one declaration, the exact
CAPI hook and code-location tests. No handler ID, word layout, monitoring
storage, frame publication, canonical guard or collector source changes.
The first oracle uses actual relaxed FOR_ITER/SEND words: all lookups use
the immutable co_lines table, even synthetic jumps whose old monitoring
cache deliberately said NO_LINE. Executing frame callbacks still report
real instruction heads. A copied old helper must fail the valid synthetic
offset assertion; it is never called with an invalid nonnegative offset.

The new lookup keeps the negative-offset firstlineno rule and checks all
nonnegative bounds before reading the immutable table. The old monitored
cache branch preceded that bounds check, so no old uniform out-of-range
return guarantee is claimed. Emergency freed-code/frame heuristics remain
best effort; removing the monitoring dependency does not repair them.

The exact old helper/line decoder copies, control patches, configuration,
source and interpreter identities are retained with the final gate
record. Builds remain release GCC O3 and qualified GCC Og. ROOTS matrix
owners coordinate short test slots; no live source is modified during gates.
H1 is paused at its separate design artifact. Stage 2b.2b remains design only.

The control runner compiles task-owned copies against each tested build's
actual configuration. Its positive uses the exact new helper body; its
negative restores the exact old body plus its unchanged line-cache decoder
dependencies. Both use the same real relaxed program and line table as the
unit oracle. For each build, the positive agrees at every valid word before
and during monitoring. Each old-cache run fails at the intended immutable
synthetic-line assertion: FOR_ITER word 5 and SEND word 8 report -1 where
the immutable table supplies a source line. Real instruction heads still
agree. No copied extension is installed into a live build, and no live
source variant is written. Inputs, include resolution, compiler commands,
body patches and interpreter/extension hashes are retained.

The immutable scan can cost more than the monitored O(1) cache lookup.
This checkpoint makes no speed claim or allocation-tracing cost measurement.
It establishes the lifetime prerequisite for later monitoring storage work;
it does not widen that storage or add a reader reclamation scheme.

## Validation and evidence

The gated source is 617bea5be1b plus the four source hashes in
[final-provenance.json](controls/H3-1-stage2b2a/final-provenance.json).
The evidence commit follows those runs; no historical full-suite run is
relabelled as a current-head run. No full suite, ASan run, concurrent
monitoring writer stress or new lifetime guarantee is claimed here.

| Check | Result |
|---|---|
| Final release/Og smoke | 1,924 tests each; 50/20 skipped; passed |
| Final release/Og affected modules | 1,519 tests each; 18/19 skipped; passed with `--fail-env-changed` |
| Og strict actual-kill affected modules | 1,519 tests, 19 skipped; passed with `--fail-env-changed` |
| Active-mode proof | All 11 workers report `(2, 0)`, `worker.main`, and the final expected CAPI extension origin/hash |
| Targeted root counters | 156 recorded processes, 1,498 collections, zero fallbacks and zero unpinned records |
| Copied controls | Current positive and old-cache negative pass their expected outcomes for both programs on both builds |
| Regeneration | `regen-cases clinic` leaves the tracked diff hash unchanged; all seven generated products match the native parent |
| Optional-module syntax checks | 19 configurations pass; `_tkinter`, `_dbm`, `_gdbm` and `readline` remain untested for missing headers |
| Ownership | Removing the exact additive CAPI hook/entry restores its entire native-parent file; all other prior fixtures and compiler/root-checker inputs are unchanged |

The affected set is test_code, test_ir, test_monitoring, test_tracemalloc,
test_faulthandler, test_traceback, test_frame, test_sys_settrace,
test_sys_setprofile, test_peepholer and test_generated_cases. It covers the
new collecting callback, normal/exception execution, existing compiler
phase oracle, constructor contracts and root-checker fixtures. It does not
exercise the asynchronous faulthandler/code-retirement race exhaustively.

Runs use `boil/tools/gate.sh 0/1`, four regrtest workers and the existing
build locks/two machine slots. The kill run sets `PYTHON_GCVERIFY=kill`, a
fresh absolute `PYTHON_GCROOTSTATS` path, and `--python` to the task-owned
[assert-kill-worker.py](controls/H3-1-stage2b2a/assert-kill-worker.py).
That launcher preserves regrtest's main setup and checks the live collector
rather than inferring its mode from the environment. Copied controls run
under the matching build lock with `timeout -k 30 600`, without live source
mutation. ROOTS paused only its release host scheduler between gates;
the H3 slot was released and ROOTS resumed immediately after the final suite.

Initial smoke logs are historical evidence for the interpreters recorded
in `pre-eof-rebuild.sha256`. Restoring an inadvertently removed EOF blank
line left both codeobject objects byte-identical but changed the relinked
interpreter hashes. Both smoke sets were therefore rerun on the final
binaries. The `*-final.log` files and final-provenance build hashes are the
current candidate evidence; the earlier smoke is not its landing evidence.
Final source/test hashes remained fixed throughout those runs.
