# First quiet interpreter wall-time diagnostic

The first once-only elapsed diagnostic completed on 2026-10-29 at 15:30 PDT,
under the coordinator's exact e9a46e75 plan/939e1a59 launch grant and independent
FINAL_QUIET_BINDING_CLEAR. Both shared JSON-tail witnesses and all 29 paired
application rows completed; final cleanup and source/product guards passed.
The measured upstream/fork geometric mean is **0.828243** (fork elapsed time
about 1.207 times upstream). This is a five-sample initial diagnostic; actual
closure independently clears ACTUAL_EARLY_TIMING_SCOPE_CLEAR in baseline review
round 19. The fork is unlanded ce2ac with unresolved stress
failures, different eval-unit flags and limited workload-origin evidence.
This result establishes neither parity, the ~2× target, correctness nor landing
qualification. The retained design and history below describe the bounded
transaction and preserve its limitations. No production benchmark runner changed.

## Inputs and one JSON-tail witness

Reuse the unchanged run.py registry and driver.py. The 29 application rows,
loop counts, variants and shared source hashes are already frozen in
workload-origins-v1/source-plan.json (4f1c1423); memory/micro rows do not enter
the 29-row score. Freeze the final fork executable, its own source/stdlib/
extensions, native inventory and corresponding origin evidence before
the exact timing launch. Upstream keeps its owned 321925 tree and 584
products. Never borrow an active fork stdlib or old baseline extensions.

One **named zero-sample time-mode witness**, regex_compile, closes the
shared driver's extra JSON-tail import for each compared environment:

```
BUILD/python -S -v -X pycache_prefix=Q/json-tail/BUILD/pycache
  P/boil/bench/internal/driver.py time
  P/boil/bench/internal/kernels/regex_compile.py 1 0
```

The last 0 is RUNS: driver.py performs setup and its one warmup run, skips
all timed iterations, imports json and prints `[]`. It calls no timer in
that empty loop. This is a proposed separately granted origin witness,
not authorized by the previous run-only campaign. Reuse its trace/ELF
rules and fresh cache/tmp paths; require own JSON source and _json backend.

The source argument for using one named case is narrow: all 29 use the
same frozen driver tail; only regex_compile changes sys.path, by adding
the two already-hashed corpus helpers' directory, which contains no JSON
module. Source search found no kernel replacement of sys.modules,
meta_path, path_hooks or __import__. The witness therefore exercises the
existing path-changing case and an actual first JSON import after setup/
run. It is **not per-workload time-mode tracing**. No 29-witness campaign
is proposed. Raw run-mode workload evidence and the shared source argument
retain their separate scopes.

## Quiet window and common execution policy

The coordinator reserves a bounded quiet window after active native builds,
tests and gates reach terminal cleanup; other agents launch none during it. Use existing
machine-slot/build-lock supervision, reserve both normal gate slots for
the window, and protect both compared build paths through final cleanup.
If either reservation is unavailable, wait before any timing; do not
change slot policy or compete with a gate. Preserve current source/tool/
native/shared input checks before and after; no old 0200 test-cache reads
or permission changes are needed.

Propose one fixed physical core: logical CPU 0, sibling 8 left idle, inherited
by both runtimes. The current allowed affinity is 0–15; CPU 0 is core 0 on
socket 0, amd-pstate-epp with powersave governor. Record actual affinity,
topology, governor/boost policy, temperature/frequency data where readable,
and host load before/after. Make no automatic governor or power changes.
Use the same policy for both runtimes and keep the established max-load 4
guard. Current load around 7.4 is not a quiet window. Pinning alone cannot
establish absence of unrelated host interference.

Use the clean environment already established for origin checks, with
PYTHONHASHSEED=0 and a declared common temporary filesystem. Remove -v
and LD_DEBUG for measured children. Fresh task-owned process caches are
filled by the unchanged driver's **one warmup call** before measurement.
Preserve startup as a separate future metric; this score is warm run time.

## Minimal paired collection and stopping

For each row, run a fresh upstream process and a fresh fork process next
to each other. Alternate which build goes first across the fixed registry
order, and record that order. Use identical registered loops and variants.
The measured child is the existing command:

```
BUILD/python -S -X pycache_prefix=Q/ROW/BUILD/pycache
  P/boil/bench/internal/driver.py time BENCHPATH LOOPS 5 [VARIANT]
```

Five timed calls are an inexpensive initial estimate, following the
existing runner default, **not a reliability threshold**. They are serial
samples within one process, not five independent process experiments.
Preserve every value and its order, common warmup/loop counts, actual
child stdout/stderr and process metadata. Do not retain only a median.

Use run.py's existing Context.command and measure_time's JSON parsing/loop
normalization and result fields in the paired host adapter. The coordinator
accepted an explicit subprocess.run equivalent: run_checked hardcodes its
inherited CHILD_ENV and discards failed child output. Persist raw stdout/
stderr and the command receipt as well as all five values before a post-run quietness
check can reject the sample. Do not replace the timer or target driver. The
unchanged CLI intentionally writes nothing on --max-load failure, so
blind CLI retries would lose rejected samples. Exact host adapter/launch
bytes still require review; execution remains held.

Stop after this initial complete 29-row diagnostic if it identifies the
large recurring costs needed to choose the next structural work. Report
within-process spread and trends descriptively; do not manufacture a
confidence interval from dependent samples. Extend only for a named
instability or a decision that the current uncertainty cannot resolve:
for example order-coupled drift, a warmup trend, or an individual regression
whose direction would change the next action. Predeclare the extra fresh
process pairs, balanced order and quiet-time cost limit for that question.
Use process pairs as the units for any later uncertainty model, disclose
its assumptions and retain all previous/rejected records. No automatic 20
repetitions, retry-until-green or collection-until-target rule is used.

## Honest score and remaining limits

Publish all 29 per-workload upstream/fork medians and raw vectors, including
regressions. Score is exp(mean(log(upstream_time/fork_time))); roughly 2x
means roughly half upstream's elapsed time. Existing compare.py prints
fork/upstream, so label its ratio direction and take the reciprocal for
the stated speedup score. Missing/nonpositive rows make the full score
incomplete; do not silently change its denominator or add micro wins.

The first cohort can be a **source-policy diagnostic**: common actual
GCC/O3/GIL/nondebug/nonstats/JIT-off/retained-FP/no-PGO-LTO-BOLT policy,
with upstream's empty CFLAGS_CEVAL versus fork's deliberate -fno-gcse
disclosed. Freeze actual flags and product/tool identities, not just
configure labels. This diagnostic may show comparative numbers; it cannot
establish parity, object-model-only attribution or the 2x target. Require
the separately declared upstream sensitivity or a fully matched eval
cohort before those claims. SDK consumed-header provenance and any other
remaining environment differences remain explicit. No extra build is
authorized to close them now.

## Source package and remaining binding

The original source proposal is preserved by SHA256 3aacbf22eb215f7f6df3448b2d3177757a2cbd62861429073694111c4a77b5b2.
The coordinator subsequently authorized task-owned source implementation and
host-only controls. `build/checkpoint-i-baseline-binding/quiet-timing-v1/`
contains `paired.py`, `phase_runner.py`, `freeze_template.py`,
`host_controls.py`, `plan.json`, `launch-template.json` and `provenance.json`.
Production registry/driver, upstream source and accepted native products
remain unchanged. The latest host-only run passed 26 controls; no benchmark
module or target interpreter was executed by those controls. Earlier 20/23-control
outputs remain in `host-controls-round1/` and `host-controls-round2/` rather than being overwritten.

The template is deliberately **not launchable**: `runtimes.fork` and
`quiet_reservation` are null. A coordinator-selected qualified fork must
provide its own executable/build root, exact source commit and hashed source
manifest, complete product manifest, declared external ELF manifest, actual
qualification/origin receipt hashes, qualified identity and disclosed cohort.
The source manifest uses `{root, commit, entries}`; product manifest uses
`{source_commit, build_root, products}` with the existing regular-file/alias
descriptors; ELF manifest uses `accepted_external_resolved`. These can point
to existing accepted closures when their fields match. No borrowed upstream
stdlib or extensions satisfy the fork witness. Root selects and reviews the
actual binding, receipt scope and final launch hash before execution.

The collector builds commands through unchanged `Context.command`, alternates
the first runtime across 29 fixed rows, preserves all 5 durations/normalized
values and raw command output, and computes the complete 29-row upstream/fork
score only after success. One regex_compile RUNS 0 trace per runtime reuses
the accepted Python/actual-ELF parser with that runtime's source and products;
it specifically requires its JSON package and _json extension. It does not
trace every time-mode workload. Explicit child environments contain no
LD_DEBUG/-v during measurements and use separate fresh per-process pycache/tmp
paths. CPU affinity is changed only for the task process and restored;
before/after readable topology/governor/frequency/thermal data are retained.
An idle sibling remains a coordinator quiet-window condition, not a claim
that affinity prevents unrelated host interference.

The runner adapts the accepted supervisor rather than changing gate_run:
one nonblocking task lock precedes admission; both normal machine slots are
acquired together, releasing a partial acquisition before waiting. A detached
supervisor retains both slot descriptions through timeout and cleanup. The
unchanged upstream gate takes FD8; a host bridge preserves it as FD11 while
the unchanged fork gate takes FD8. The locked collector verifies both actual
flock descriptions/inodes and both slots. Nested gates sweep their respective
executables before release; the supervisor checks complete product maps after
both gates return and writes the final receipt. The new nested-lock host
control executes only harmless host children with task-owned locks and
nonexistent target executable patterns. Existing accepted lock/parser controls
retain their separate scope; the new package does not claim target execution
or quietness from host-only controls.

Next: independent source review, actual stable fork/quiet-window binding and
exact coordinator launch grant. No witness, timing or extra build is granted
by this document or the template.

Independent source review found that HEAD and listed source hashes alone missed
untracked stdlib additions. The successor restores the accepted clean
`git status --porcelain` check before target work and after cleanup, alongside
commit and byte checks. Host-only copied-repository controls accept a clean
source and reject added or modified Python source. The initial frozen package
remains at `quiet-timing-reviewed-v1/`, provenance 85ee627e; this correction
does not qualify a fork or authorize any target execution.

## Early candidate binding after the stress failure

The coordinator permits a bounded quiet diagnostic before a source repair
only if failed-full termination, focused causal probes and actual machine
quiescence leave a useful window. A ready GC fix or IR source handoff takes
priority. Noisy/incomplete timing must not delay them; retain the frozen
parent products/plan rather than waiting indefinitely.

`build/checkpoint-i-baseline-binding/quiet-ce2ac-binding-v1/BINDING.md`
records the source-only packet: ce2ac release Python eafcda7a, all 6,322
source-bridge entries, the original expanded 286 product/alias map and 727
complete current build-directory products. Both native maps match capture
before/after. Static readelf closure declares nine exact external ELF inputs;
it is not observed-load evidence. Release-full receipt 7b57d7ec remains PASS
only for its actual normal-suite scope. The unresolved stress failures and
unlanded status are carried in diagnostic metadata; terminal failure closure
is still an owner/coordinator prerequisite for a quiet window, not a partial
log relabeled final. No old qualification receipt is changed.

The source-reviewed collector/runner bytes remain unchanged. The fork now
has its own source/config/native/ELF bindings and historical full-normal
runtime/CAPI journals; those journals are not actual-driver origins. The
single shared-tail witness and all timing are unperformed. Upstream's 9e
closure and accepted products are unchanged. The packet leaves
`quiet_reservation` null and cannot launch. Independent actual-binding review,
the final quiet reservation/plan hash and exact root grant remain due.
Result labels must continue to say early source-policy diagnostic: no
correctness, landing, parity or ~2x qualification follows, and the existing
fork -fno-gcse/SDK provenance limits remain.

The terminal stress failure is now receipt 64448666: FAIL/native exit 2,
50,722 native-reported tests, interpreter assertion, 20-minute GC
test_trashcan_threads timeout and HTTP ENV_CHANGED. The killed GC worker
has no end record, so actual-state pairing fails and there is no qualified
root aggregate. The updated packet binds this exact receipt, all three raw
streams and a read-only post-cleanup identity capture: source remains clean,
all 6,322 listed source bytes and all 727 products match. This is identity
evidence after failure, not correctness or a claimed aggregate. The earlier
cbbdaef1 binding packet is preserved separately. Quiet reservation remains
null; focused four-case interpreter probes take priority. No target, timing,
native build or original profile retry is granted by this update.

## Actual first diagnostic: 2026-10-29 15:28–15:30 PDT

Root granted the exact frozen launch after all four interpreter diagnostics
terminated and the native hold began. The host snapshot had one-minute load
0.509, no project native/compiler process, both machine slots and both build
locks available; root separately confirmed quiescence. The unchanged runner
retained both slots/build locks through cleanup. Children used CPU 0,
PYTHONHASHSEED=0, one warmup and five timed values per process; order alternated
by row. No sample or row was retried. Both named RUNS=0 JSON-tail witnesses
completed before measurements.

Closure `build/checkpoint-i-baseline-binding/quiet-timing-closure-v1.json`
SHA-256 `68da9ae2865d95c69bb00ef0d90dc8bf46bbbb42a17ffb34149a87444fb13553`
binds 2,385 files, including all raw outputs, 58 measurements and 290 timed
values. Final receipt `fadaac6d` reports native exit 0 and verified cleanup;
inner `ade5ec63` reports verified completion. Native before/after/post-cleanup
maps agree. Source/product checks passed; this is actual diagnostic evidence,
with independent ACTUAL_EARLY_TIMING_SCOPE_CLEAR in baseline review round 19.

The score is upstream elapsed / fork elapsed; above 1 favors the fork.
The 29-row geometric mean is 0.828243, with 22 slower rows and seven faster
rows in this initial collection. All rows remain visible:

| Workload | Upstream / fork |
| --- | ---: |
| coroutines | 0.365 |
| comprehensions | 0.528 |
| generators | 0.592 |
| deepcopy_reduce | 0.618 |
| xml_etree_iterparse | 0.621 |
| deepcopy | 0.661 |
| unpickle | 0.666 |
| logging_silent | 0.681 |
| json_loads | 0.713 |
| nqueens | 0.719 |
| logging_format | 0.738 |
| pickle | 0.741 |
| deepcopy_memo | 0.746 |
| logging_simple | 0.752 |
| regex_compile | 0.776 |
| xml_etree_generate | 0.781 |
| unpack_sequence | 0.799 |
| xml_etree_process | 0.805 |
| json_dumps | 0.830 |
| xml_etree_parse | 0.849 |
| fannkuch | 0.942 |
| deltablue | 0.978 |
| go | 1.039 |
| chaos | 1.065 |
| spectral_norm | 1.110 |
| raytrace | 1.130 |
| nbody | 1.603 |
| richards | 1.897 |
| float | 1.923 |

The dominant observed regressions are coroutines, comprehensions and
generators; float, richards and nbody are the largest observed wins. This
collection prioritizes follow-up; it does not attribute the differences to a
single optimization. Keep the frozen parent build for comparisons rather than
repeating the whole collection automatically. Extend samples only for a named
instability or decision, preserving these original values.

The fork remains the unlanded ce2ac candidate. The full-normal suite passed,
but stress100k failed and its killed GC worker prevents a qualified root
aggregate. The original-stress diagnostic's invalid adapter receipt and offline
interpretation remain separate; stress eligibility evidence is no correctness
repair. Fork CFLAGS_CEVAL includes -fno-gcse while upstream's is empty; common
compiler/O3/frame-pointer/zero-init settings are not complete eval-unit matching.
The fork lacks the upstream's 50-row actual-driver origin campaign. Its own
source/native bindings and this shared-tail witness are narrower evidence.
External SDK candidate inventories do not prove consumed header bytes. These
limits must accompany every interpretation of the first elapsed result.
