# Proposed follow-up queue for runtime `6a0e4c12c17`

Status: proposed execution order, 2026-11-06. The runtime is functionally
qualified: release and debug each passed 54,351 tests. It is not R27-qualified.
The [full census](full-census-6a0e4c12c17.md) reports 11,028,818
core/core-pending/unresolved boxes, incomplete process accounting and 11
static-gate findings. The [kernel capture](kernels-6a0e4c12c17.md) contains 87
raw records over 29 kernels: 18/29 are startup-inclusive raw-zero, all 29 loop
deltas are zero, and the strict gate fails. Treat these as diagnostic results
and port-priority data. Performance work may proceed when behavior is correct
and the workload, identity, event and accounting evidence is trustworthy; a
zero-box full census is not an admission veto.

The implementation references are [value API](../../design/value-api.md),
[native boundary](../../design/native-boundary.md),
[process accounting](../../design/process-accounting.md),
[numeric formatting](../../design/numeric-formatting.md), and
[register IR](../../design/register-ir.md). Follow their ownership and status
contracts; this queue does not revise their selected designs.

The queue follows [PROCESS](../../PROCESS.md): propose the exact ownership and
semantic contract, obtain independent review, implement the bounded packet,
run its smallest meaningful tests, obtain post-review, then hand it to the
coordinator for merged integration. A worker packet does not claim integration.
Workers use release syntax/compile checks and focused tests, not full suites or
shared builds. The coordinator owns shared headers, merged builds, full suites,
workload-accounting review, and the final performance comparison. The R27 box
census remains a diagnostic, not a required zero-count gate. If a source
boundary or test would cross ownership, pause that packet and obtain agreement
before editing.

## Ordered queue

| Priority | Packet and boundary | Owner / review | Minimal evidence and exit condition |
| --- | --- | --- | --- |
| P0 | Repair only accounting defects that prevent trustworthy workload evidence. Reproduce the full-census `test_subprocess` failure; audit `.run`/`.done`/`.zero`/`.child` identity and event matching, preserving the 539 unfinished markers, 251 orphan `.done` markers, 1,233 identity errors, 1,774 accounting errors and 437 predicted-uncounted events as evidence. The 437 are not unique children. Do not allowlist or infer missing dispositions. | Terra, medium, process-accounting/checker ownership; Sol reviews accounting semantics; root owns producer/reader interfaces and integration. | Preserve and explain actual child/start/exit linkage for the workloads being compared, prevent silent event loss, and retain the original stats regrtest failure. Use focused producer/reader and subprocess/accounting fixtures. Full-census zero boxes are not an exit condition; unresolved census records remain published and labeled. |
| P1 | Keep the 11 static R27 findings and production callers accurately inventoried. Verify signatures and ownership for any API touched by a performance or porting packet; never add marks solely to quiet the scanner. Static findings and the 11,028,818-box census remain useful port-priority diagnostics, not global performance admission gates. | Luna, medium, source inventory and focused checker fixtures; Sol reviews signatures/ownership; root owns checker policy and integration. | Per touched API, record the exact callers, provider status/ownership, callback behavior and relevant tests. A packet may proceed when its own behavior and measured workload accounting are trustworthy; no blanket exemptions or false closure claims. |
| P2 | Normalize selected workload work before ranking ports. Add targeted dispatch/semantic-operation counters so hardware instructions are related to benchmark operations, not mislabeled as per-bytecode cost. Count relevant call shapes and frame slots, and record PyValue bridge allocations/time by real workload and call path. | Terra, medium, instrumentation design and source ownership; Sol reviews counter meaning and sampling; root owns benchmark integration and shared measurements. | Start with the three counter cases and preserve exact workload/loop denominators. Check counter conservation, operation counts and call/frame attribution against source-derived expectations. Report unassigned work explicitly; do not infer per-bytecode costs from aggregate hardware counts. |
| P3 | Reduce measured GCC source overhead in bounded code-generation packets: generated repeated publication/refresh sequences and unnecessary helper calls. Keep GCC and build flags fixed; no Clang/two-compiler comparison. Inspect emitted GCC code at attributed hot sites before proposing a change. | Terra, medium, one generator or helper cluster at a time; Sol reviews effects/ownership; root owns generator regeneration, integration and matched measurement. | Add the smallest relevant generator/compiler tests plus a workload-specific semantic test. Preserve publication order, roots and callback visibility. Exit when the generated/source change removes the targeted work, unrelated behavior is stable, and the same workload shows repeatable measured movement beyond run spread. |
| P4 | Reduce Python-call duplicate/bind/retire work and broad frame initialization/cleanup where measured call shapes and live-slot counts support a safe change. Preserve error order, monitoring, tracing, recursion, reentry and finalizer visibility. | Terra, medium, separately owned call and frame packets; Luna prepares exact source inventories and focused cases; Sol reviews lifecycle/effect rules; root owns shared integration. | First rank call shapes and initialized/cleared frame slots by selected workload. Use focused call/descriptor/reentry and generator/frame tests appropriate to each packet. Keep broad liveness changes separate from call preparation and reject changes that do not move the intended measured cost. |
| P5 | Finish useful PyValue ports according to measured workload impact. Use the normalized dispatch/operation, call-shape, frame-slot and bridge-cost evidence from P2, then select direct-value continuations that preserve behavior and ownership. Keep ctypes and all LRU implementation on explicit approval hold. | Terra, medium, bounded bulk ports after reviewed recipes; Luna, medium, mechanical callers and tests; Sol reviews design/ownership; root integrates and ranks measured effects. | Each packet names exact files/functions, owners, callbacks, error order, minimal tests and workload contribution before editing. Do not optimize for box counts alone. Keep the full R27 census as a diagnostic trend; only promote a zero-box target when it aligns with a measured useful workload. |

File boundaries: P0 owns the assigned checker/fixture and subprocess-accounting
tests; the coordinator owns `Python/pystats.c` and `pyjit/bench/run.py`. P1 owns
the caller inventory and focused checker fixtures in
`Tools/build/check_value_api.py` and `Tools/build/value_ownership.toml`; no API
marks change without a production-caller review. P2 adds only counters required
to answer a specific workload question, with benchmark interfaces assigned by
the coordinator. P3 owns one generator/helper region and its producer. P4 owns
one call-preparation or frame-lifecycle region per packet. P5 ports are assigned
by exact file and caller after workload impact and ownership are reviewed.

### P0 evidence constraints

The [census fixture review](census-fixture-contract-review.md) proves that
`test_pyvalue_box_sites` and `test_value_slots` read and unlink their own
`.txt` after a stats dump while the producer leaves the paired `.done`. Their
tags account for the observed 247 and 4 orphan markers, but the exact per-call
provenance is unproved. Do not simply delete both members, suppress these
tags, or narrow suite coverage: the fixtures also clear counters and turn
collection off, so retain counter/enabled-state coverage and raw pair evidence.
The producer honors `-X pystats_dir` before `PYTHON_PYSTATS_DIR`; the fixture
readers currently consult only the environment/default. Give both sides one
explicit directory contract before isolation tests.

The two missing dump identities are retained forkserver `.run` records tagged
`test.test_multiprocessing_fork.test_misc` and
`test.test_multiprocessing_forkserver.test_misc`. Their argv names
`multiprocessing.forkserver.main(...)`; that is launch context, not evidence
of successful exit or a cause. Capture both dump publication and completion
markers with raw files retained, and establish matching start/exit identity
before assigning a cause.

Initial timeboxes and scope cuts: P0 first returns a workload-accounting
failure map within 30 minutes; investigate census-only anomalies separately
unless they affect correctness or measured workload evidence. P1 inventories
the 11 static findings within 60 minutes but does not gate unrelated throughput
work. P2 proposes a narrowly scoped instrumentation question within 60 minutes;
do not build a general profiling framework. P3–P5 each start with one reviewed
function cluster and a 90-minute proposal/first-increment budget (up to three
hours for a callback-heavy packet). When a timebox expires, retain the measured
question, reproducer and unresolved ownership, then cut scope. Do not reduce
test coverage to meet a deadline.

The [kernel attribution](kernels-6a-box-attribution.md) records bounded kernel
owners for `Modules/_stat.c:stat_S_ISLNK`, Unicode error handling and the generic
call route. It is separate from the full-census attribution below. Full census
dynamic counts are in `build/pyjit-link-wave1/checkpoint-closure/suite-6a0e4c12c17-census/debt-totals.json`.
They include `Objects/call.c` (2,284,628),
`bltinmodule.c` (743,952), `selectmodule.c` (403,727),
`timemodule.c` (386,485), and math integer operations (370,203). These are
box-attribution counts from the retained census, not elapsed-time costs and
not proof that each subsystem is a distinct implementation task. Get fresh
owner-bearing attribution and use timing profiles before ranking them by
performance impact.

## Approval-held work and other boundaries

`Modules/_ctypes` raw-pointer ownership work and all LRU implementation remain
on explicit approval hold after automatic review. The census attributes
2,834,207 boxes to ctypes and 717,517 to `Modules/_functoolsmodule.c`; these are
dynamic box counts, not elapsed-time priorities. Keep those areas out of source
packets until the coordinator records approval. Inventory/attribution work
may document these areas without changing source.

The current 64-bit IR word remains a 16-bit handler and four 12-bit fields;
slot class is determined at runtime. G2 class-bit experiments remain deferred.
The measured work in this queue targets PyValue ownership and call sites, not
an IR encoding change. Preserve the accepted IR design and its open questions
in [register-ir.md](../../design/register-ir.md); do not infer a reason to
change the encoding from box counts alone.

## Diagnostic performance evidence: complete; upstream parity remains open

The matched campaign is complete and its workload/build evidence passed review.
The candidate remains slower than upstream on the broad elapsed-time aggregates;
the separate R27 box census still fails but is not a reason to discard valid
measurements or block targeted work. Candidate/baseline means candidate divided
by baseline; lower is better.

| Metric | Candidate / upstream | Candidate / interim | Qualification |
| --- | ---: | ---: | --- |
| 29-kernel elapsed-time geomean | 1.2867708613 | 1.0753300431 | Diagnostic only |
| 12-result pyperformance elapsed-time geomean | 1.09643692 | 0.9130632923 | Diagnostic only |
| 29-kernel peak-RSS geomean | 1.259197157 | 1.061970975 | Diagnostic only |

The selected hardware-counter follow-up also completed: 54/54 positive counter
runs and 18/18 empty-region calibrations are valid; six user-only DWARF
captures report no lost samples. Source/build identities stayed unchanged. The independent
[hardware review](hardware-perf-review-6a0e4c12c17.md) accepted the evidence;
raw values and limits are in the [diagnostic report](../../bench/results/checkpoint-6a0e4c12c17.md).

Steady-state throughput is the follow-up priority; RSS is secondary. Candidate
startup RSS was 13,300 KiB (upstream 10,372; interim 12,036). Builds used
JIT-off GCC 15.2 `-O3` and mimalloc, without PGO or LTO; upstream used
computed-goto dispatch and the fork used register-IR tail-call handlers.
Source identities were unchanged. The pyperformance run warned on 35/36 results that
the samples did not establish one-percent precision at 95% certainty. Six
kernel rows were noisy. Do not claim statistical significance or that sampled
profile sites caused boxing or explain total runtime. The [full report](../../bench/results/checkpoint-6a0e4c12c17.md)
contains the per-kernel and per-result tables, raw vectors, RSS details,
commands, identities and limitations. Historical 1.73x kernel and 1.83x
pyperformance ratios refer to runtime `5de7bd62ef9`, not this source.

Completed source review and hardware counters prioritize GCC-only source
overhead, bounded attribute/accessor work, Python-call preparation, and frame
traffic. The three selected regressions retire 2.047x, 1.903x and 1.628x
the upstream instructions, while CPI is comparable or lower; cache counters
are too noisy to attribute. These findings do not establish the cause of the
elapsed gaps. Use existing evidence for bounded patches; add targeted
dispatch/semantic-operation counts, call-shape/frame-slot counts or bridge
costs only when needed to rank a specific change. P0 work must preserve correct
workload identity and accounting, but P1–P2 are not serial gates for every
throughput patch. The checkpoint needs functional correctness, trustworthy
workload evidence, reviewed changes and measured throughput results; an
exhaustive zero-box census is not required.

## After checkpoint

After a coherent source passes functional tests, has trustworthy accounting
for the workloads used to assess it, and has a reviewed throughput result,
close the PyValue/register-interpreter checkpoint with a
recap of status, limits and measured follow-ups. Then resume stack-bytecode
serialization recovery and the shared optimizing method/tracing JIT work.
Before native method/tracing optimization, review a shared effects/Action/
FrameState contract and LIR verifier, then resolve register IR's O2/O3
liveness/state-export and OSR predicate gaps, feedback invalidation and
generator resume. Keep interpreter homes, typed CFG SSA values, and backend
registers/spills distinct as in [runtime architecture](../../design/runtime-architecture.md)
and the [end-to-end JIT design](../../design/jit-e2e-system.md). G2 remains
deferred; do not reopen collector/nursery experiments.
