# Proposed follow-up queue for runtime `6a0e4c12c17`

Status: proposed execution order, 2026-11-06. The runtime is functionally
qualified: release and debug each passed 54,351 tests. It is not R27-qualified.
The [full census](full-census-6a0e4c12c17.md) reports 11,028,818
core/core-pending/unresolved boxes, incomplete process accounting and 11
static-gate findings. The [kernel capture](kernels-6a0e4c12c17.md) contains 87
raw records over 29 kernels: 18/29 are startup-inclusive raw-zero, all 29 loop
deltas are zero, and the strict gate fails. This queue keeps correctness and
accounting work ahead of performance admission.

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
the R27 and kernel gates, and the final performance comparison. If a source
boundary or test would cross ownership, pause that packet and obtain agreement
before editing.

## Ordered queue

| Priority | Packet and boundary | Owner / review | Minimal evidence and exit condition |
| --- | --- | --- | --- |
| P0 | Repair the census contract and reproduce the full-census `test_subprocess` failure. Audit `.run`/`.done`/`.zero`/`.child` pair handling, incomplete identities, 539 unfinished markers, 251 orphan `.done` markers, 1,233 identity errors, 1,774 accounting errors, and the 437 predicted-uncounted events. These events are not unique children. Do not allowlist the orphans or infer an external disposition. | Terra, medium, process-accounting/checker ownership; Sol reviews accounting semantics; root owns producer/reader interfaces and integration. | Add narrowly targeted checker/producer fixtures for each repaired failure class; preserve the original stats regrtest error. Run the focused subprocess/accounting tests. Exit only when fixture failures are resolved and a fresh coherent stats integration has no unexplained identity or accounting errors, orphan/unfinished markers, or missing observed child. Then rerun full stats census. A standalone `test_subprocess` pass (371 pass, 48 skip) does not erase the recorded full-census failure. |
| P1 | Audit the 11 static R27 findings before changing annotations: one incomplete ownership inventory plus ten missing `Py_VALUE_COMPAT` declaration marks. For each API, enumerate production callers, verify the exact value-provider status/ownership contract, and either port callers or establish a reviewed no-production-caller alias. Never mark a declaration solely to quiet the scanner. | Luna, medium, caller inventory and straightforward ports; Sol reviews signatures/ownership; root owns checker policy and final metadata. | Per API, capture current `rg` caller inventory, declarations, provider and affected tests. Add checker positive/negative fixtures for any genuinely new manual row. Exit when all actual callers have reviewed dispositions, no blanket exemption was added, and the static scan reports no unexplained finding. Full suite is reserved for the integrated batch. |
| P2 | Port startup boxes with known value homes: `Objects/sliceobject.c:slice_new` argument parsing; `Modules/itertoolsmodule.c:repeat_new`; `Python/getargs.c` format `n`; `Objects/exceptions.c:UnicodeDecodeError_init`; `Objects/unicodeobject.c:unicode_decode_call_errorhandler_writer`. Keep argument values owned through integer/index conversion and callbacks; retain validation order, type checks, overflow text and cleanup. These sites contribute to logging, regex and XML raw rows; comprehension and JSON rows are attributed to `lru_cache_new`, which remains approval-held. | Terra, medium, constructor/parser ownership; Luna can take disjoint mechanical parser sites after interfaces are reviewed. Sol reviews the callback-heavy Unicode/error-handler conversion; root integrates shared parser/API changes. | `test_slice`, `test_itertools`, `Lib/test/test_capi/test_getargs.py`, `test_exceptions`, and `test_codecs`/multibyte codec tests as appropriate. Add focused inline int, subclass `__index__`, overflow, malformed callback tuple, Unicode/buffer and error-order cases where those semantics change. Exit when the targeted sites disappear from a fresh attributed capture, unrelated owners remain unchanged, and focused tests, release compile and post-review pass. Overall kernel zero is a coordinator gate, not a single-packet exit condition. |
| P3 | Resolve `Python/ceval.c:native_call_builtin_o` in regex (9 boxes) to the actual `METH_O` callee before changing the ABI. Then port only that identified callee if a paired value entry preserves its semantics. Trace the logging shared `PyValue_AsObject` (one box per logging kernel) to an owner-bearing call site before porting; the shared boxing PC alone does not identify its caller. Review `Modules/_stat.c:stat_S_ISLNK` (two regex boxes) as a bounded paired-value method candidate, preserving `__index__` behavior. | Luna, medium, exact callee inventory and bounded module port; Sol reviews the selected ABI/dispatch change; root owns `ceval.c`, shared call conventions and integration. | First deliver a trace artifact naming exact callee and call route; no source edit before that. Then use `test_re`, the selected module's focused tests, and logging tests only where touched. Add custom `__index__`/error and dispatch-preservation cases. Exit when each raw box has a source-backed owner and an integrated kernel retest removes only the intended site. |
| P4 | After P0–P3, obtain fresh owner-bearing traces for the largest remaining core-attributed census groups: `Objects/call.c` (2,284,628 boxes), `Python/bltinmodule.c` (743,952), `Modules/selectmodule.c` (403,727), `Modules/timemodule.c` (386,485), and math integer operations (370,203). These are census box counts, not elapsed-time costs. Identify the exact API/caller and signature before proposing a port; do not infer a module-wide migration from the totals. | Terra, medium, ownership-sensitive ports after exact trace; Luna, medium, source census and mechanical callers after review; Sol reviews call/result ownership; root owns shared headers and integration. | One file/caller cluster per proposal, with raw owner PCs, existing value API (if any), callback order and owner-close map. Choose focused tests only after the caller is identified (callables, builtins, select, time or math tests as appropriate). Exit packet-by-packet when targeted owners disappear and unrelated counts remain stable; only the coordinator reruns full gates. |

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

Initial timeboxes and scope cuts: P0 first returns a fixture/failure-class map
in 30 minutes, then takes at most a half-day per reviewed accounting increment;
P1 inventories the 11 findings in its first 60 minutes; P2 takes one function
per packet (90 minutes for simple parsers, up to three hours for the callback
writer); P3 spends its first 60 minutes on exact callee tracing and stops if
attribution is ambiguous; P4 spends its first 60 minutes on owner tracing,
then takes one identified caller per reviewed packet, capped at two hours.
When a timebox expires, retain the reproducer, owner rows and unresolved
behavior, then propose a narrower next increment. Do not reduce test coverage
or weaken the admission gate to meet a deadline.

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

## Performance evidence: pending

The matched-build preflight passed identity checks but is ineligible as a
measurement. A diagnostic measurement run is active; no aggregate result is
available yet. When it finishes, the coordinator may publish the result as
diagnostic with its limits. Do not call the checkpoint complete or infer a
current speedup from historical ratios. After the strict R27 gate passes,
publish matched upstream/interim/candidate elapsed-time and RSS results, raw
vectors, spread, host conditions and qualification status. Report time as
baseline divided by candidate (`> 1` favors the candidate); reject samples at
load `>= 4` and retain the full 1/5/15-minute load tuple.

| Comparison | Time ratio | RSS result | Status / evidence |
| --- | ---: | ---: | --- |
| Candidate vs upstream `3a440b65695` | pending | pending | No aggregate result yet |
| Candidate vs interim `eba351fee7d` | pending | pending | No aggregate result yet |
| Kernel detail / spread | pending | pending | Fill from retained raw vectors |

Historical 1.73x kernel and 1.83x pyperformance ratios refer to runtime
`5de7bd62ef9`, not this source. Keep them labeled historical. After qualified
measurement, write a follow-up ranked by elapsed-time and RSS evidence, not
boxing counts.

## After checkpoint

Only after a coherent source passes functional tests, static/API review, full
R27 accounting and startup-inclusive kernel admission, and has qualified
performance results, close the PyValue/register-interpreter checkpoint with a
recap of status, limits and measured follow-ups. Then resume stack-bytecode
serialization recovery and the shared optimizing method/tracing JIT work.
Before native method/tracing optimization, review a shared effects/Action/
FrameState contract and LIR verifier, then resolve register IR's O2/O3
liveness/state-export and OSR predicate gaps, feedback invalidation and
generator resume. Keep interpreter homes, typed CFG SSA values, and backend
registers/spills distinct as in [runtime architecture](../../design/runtime-architecture.md)
and the [end-to-end JIT design](../../design/jit-e2e-system.md). G2 remains
deferred; do not reopen collector/nursery experiments.
