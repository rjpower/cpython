# One halfway scheduling increment

2026-10-31. Host-only design refinement against adopted source
`ef0af95904b87d888eee4a98b6a745835ff95085`. No runtime/test/helper edit or
native execution accompanies it. The accepted causal controls establish the
manual-minor and debt oracles; their source, helpers and evidence stay frozen.
The nursery architecture remains adopted. This proposal changes one default
automatic schedule and adds no runtime choice between schedules.

The implementation still requires independent design review and a coordinator
source grant. The original overall deadline remains 2026-11-01 04:18:11 UTC.
Read this with `boil/issues/gc-nursery-adoption/design.md` and the separate
`build/cadence-boundary-notes.json` (SHA256
`712c701f1ae4740d1104ce41b1c1cdcb63062a0ce7733b5bbb828f080b09606b`).

## Exactly two new state fields

Add two `Py_ssize_t` fields to GCState:

| Field | Meaning |
| --- | --- |
| `major_deadline` | Allocation debt at which the current epoch owes a major |
| `minor_deadline` | Halfway debt target; `PY_SSIZE_T_MAX` means this epoch's minor opportunity has been consumed or retired |

Keep `alloc_bytes` as bytes allocated since the last successful major and
`alloc_limit` as the next probe, including retry suppression. Keep the existing
non-rooting `young_pending` hint. There is no new enabled flag, age field,
success counter, allocation registry or persistent collector variant.

A separate halfway deadline is necessary. A boolean alone cannot reconstruct
the original halfway target after disabled allocations move the retry probe,
or after set_threshold establishes a new budget relative to nonzero debt.
The sentinel replaces that boolean; no third epoch field is needed. Halfway
initialization/configuration arms H only in the existing minor-capable domain;
otherwise H=INF while M retains the unchanged major budget. The selector also
checks that capability before choosing a minor, in case the allocator context
changed: retire H and wait for M in N0, or keep the existing stress-major
request. It must not run an early major at the old halfway probe solely because
the domain is unsupported.

Let D=alloc_bytes, M=major_deadline, H=minor_deadline, P=alloc_limit,
T=alloc_threshold and INF=PY_SSIZE_T_MAX. All new sums saturate at INF;
ceil(B/2) is B/2+B%2, avoiding an overflowing B+1. New scheduling arithmetic
never subtracts a larger nonnegative deadline to decide whether debt is due.
This does not expand the task into a global allocation-counter rewrite.

| Transition | Exact state change |
| --- | --- |
| Before GC initialization / after close | M=H=P=INF |
| Initialization after collector/trace state exists | If T>0, M=sat(D+T), H=sat(D+ceil(T/2)); otherwise M=H=INF; arm P as below |
| Successful major, from any entry | D=0; if T>0, M=max(T,live_bytes), H=ceil(M/2); otherwise M=H=INF; arm P |
| Successful minor, including zero deaths | H=INF; preserve D and M; arm P |
| Eligible N0 halfway probe with no young_pending | H=INF; preserve D and M; P=M; return without collecting |
| set_threshold(T>0) | Preserve D; M=sat(D+T), H=sat(D+ceil(T/2)); arm P; caller intentionally establishes a new epoch budget |
| set_threshold(0 or negative) | Preserve D; M=H=P=INF; suppress automatic requests |
| Disable | Change existing enabled state only; preserve D/M/H and current probe |
| Enable, only on disabled-to-enabled transition | Preserve D/M/H; rearm P from the stored deadlines; no collection inside enable() |
| Enable when already enabled | No scheduling change; do not erase OOM retry suppression |
| Stress N change | Preserve D/M/H and existing thread counter semantics; N>0 with T>0 arms P=0; stress off rearms stored byte deadlines |
| Mark/inventory/preparation failure | Preserve D/M/H unless an explicit callback changed configuration; update P using existing retry policy only |

Ordinary rearming chooses P=min(M,H). If that target is already behind D,
leave it due so the next allocation/breaker handles it. With stress N>0 and
T>0, P=0. With T=0, P=INF even if stress remains configured. Disabling GC or
threshold-zero never disables admission, age inspection or explicit minors.
The public C enable/disable APIs are the common path used by Python APIs.
Explicit disable/re-enable may rearm a due budget; a no-op enable must not.

A newly admitted object after the no-young opportunity was retired waits until
the normal major; its admission does not reopen that epoch. A stale true
pending hint may cause one unhelpful minor, but cannot cause repeated minors or
postpone the major. When a budget is one byte or saturates so H equals M,
major priority wins; there is no requirement to manufacture a minor first.

## One automatic decision and the existing full entries

Both `_PyGC_LiveOnThreshold` and `_Py_RunGC` use the same static automatic
selector, after existing can_collect/get_live eligibility checks. No state is
consumed merely because an allocation crossed P, an entry was attempted, or a
nested callback called gc.collect. In N0, callers retain the outer D>=P
eligibility gate before invoking the selector: a queue-only breaker must not
bypass an OOM retry probe merely because M is already due. Major priority is
applied at an eligible probe. With stress enabled, only Nth inline opportunities
request automatic collections; the breaker drains queues without treating
P=0 as another byte-triggered collection. A due major wins at the next Nth
opportunity. Explicit/emergency/full entries retain their separate meanings.
The selector has three outcomes:

```text
if D >= M: choose major
else if not minor-capable:
    retire H; choose major for a stress opportunity, otherwise schedule M
else if this is a configured Nth-allocation stress opportunity:
    choose minor if H != INF, otherwise major
else if D < P: no request
else if H != INF and D >= H:
    if young_pending: choose minor
    else: retire H and schedule M, without a collection
else: no request; restore the appropriate deadline probe
```

The N0 probe/major budget is not retuned in response to results. A C allocation
burst can overshoot halfway before an eval breaker and owe the major already;
major priority legitimately skips that minor. The first hypothesis is at most
one automatic minor per epoch, not guaranteed execution on every allocation
shape. Stress bypasses the halfway timing and the empty-young hint so it runs
a real minor transaction; after that successful minor, the next stress
opportunity requests a major even if its byte deadline is not yet due.

Keep the existing supported main-interpreter/GIL/mimalloc domain guards.
Automatic minor selection is available only in that existing domain; outside
it the existing automatic-major path remains, without changing admission or
claiming support for explicit minors. Verifier OFF/retain/KILL/extras do not
participate in this capability decision and never convert a selected minor
into a major. The public explicit-minor guard remains unchanged.

`_PyGC_LiveCollectHere` is also an emergency/full entry called by
`Python/pyvalue.c:66`, `Objects/shapeobject.c:640`, and `_PyGC_InjectCollect`.
Preserve its signature and full-major behavior. Factor its existing no-Python
collection/queue-scheduling body into a static helper accepting the chosen
kind. The full entry invokes that helper with major after the existing guards;
only the two automatic dispatch paths select cadence kinds. Do not turn an
allocation-failure recovery or gc_collect_inline_then into a minor.

The inline path still runs no Python callbacks/finalizers. Its queue waits
for the breaker; `_Py_RunGC` still drains the queue even when no new collection
is due. No queue, ordering, ownership or callback delivery protocol changes.

## Commit success where it is known

Update M/H/P at the existing successful collect() commit boundary near
`Python/gc_live.c:1943`, after mark/inventory and lifecycle preparation. A
successful zero-death minor closes H. A failed preparation can already have
promoted safe survivors, but must not close H or reset D/M. Do not introduce
age rollback. Preserve existing verifier-ledger, sequence and quarantine rules.

Neither a zero/nonzero death result, collect_pass.ncompleted (lifecycle queue
work), `_PyGC_LiveCollectHere`'s accepted-entry return, nor generation statistics
is a success predicate. record_stats currently runs even after failed lifecycle
preparation. The selector chooses a kind without consuming its opportunity;
only this successful commit closes H, except the explicit no-young retirement.

An explicit gc.collect(0) always runs a real minor, even when H is already INF
or automatic GC is disabled. Its successful pass consumes the opportunity;
required bounded lifecycle follow-up passes still execute with the same kind
and may idempotently close H again. Explicit generations 1/2/default, C full
entries, emergency entries and shutdown remain majors. A successful major
starts a fresh epoch; it is not suppressed by a prior minor.

N0 rejected entries and scratch OOM retain the existing finite RETRY_BYTES
probe backoff, using saturated addition. Retry may place P beyond M; M itself
is unchanged, and the first eligible retry chooses the already-due major.
Do not clamp a due OOM probe back to M and cause an allocation-by-allocation
retry loop. Stress failures retry at the configured next Nth opportunity as
today; they leave H open. Stress1 intentionally checks every opportunity.

Callbacks retain their explicit API effects. A start callback can change T or
disable future automatic GC; the already-entered transaction keeps its kind.
A subsequent successful minor consumes the currently established H. A major
uses the current T when establishing its post-major budget. A stop/queue
callback runs after the commit, so its later threshold/stress changes stand.
No stale scheduling snapshot is restored over an explicit callback change.
Nested collection rejected by collecting/sweeping consumes no opportunity.

## Smallest files and interfaces

Runtime changes are limited to:

- `Include/internal/pycore_interp_structs.h`: two fields and INF initialization.
- `Python/gc_live.c`: transition/rearm helpers, shared automatic selector,
  successful commit, existing init/fini, enable/stress and dispatch consumers.
- `Include/internal/pycore_gc_trace.h`: one internal threshold-setting function
  declaration, colocated with the live collector interface; preserve full entry.
- `Modules/gcmodule.c`: delegate threshold0 scheduling to that helper while
  retaining threshold1/2 storage and the public API.
- `Python/gc_trace.c`: extend the existing read-only state getter with captured
  M/H/young_pending integers; no setter and no diagnostic ledger policy change.
- `Lib/test/test_gc.py` plus `Doc/library/gc.rst`: actual policy witnesses and
  settled automatic semantics. Update issue results separately after evidence.

No constructor, object allocator fast path, root map, native cleanup family,
coroutine allocation coverage, barrier or copying change. No new CAPI method
is required if the existing read-only state getter proves these fields. Ensure
its temporary collecting guard blocks automatic opportunity decisions while
it builds its result, as well as preserving its existing probe-restoration rule.

## Focused useful checks and source negatives

Keep all existing nursery method IDs/protections. Change the strengthened
finite-major test's budget observation from allocation_limit (now a probe) to
major_deadline. Keep the separate early debt/statistics oracle. Both remain
causal controls, not performance benchmarks.

Before broad qualification, implement a finite roster with these actual cases;
freeze child counts/modes and exact code in the reviewed launch manifest:

| Case / proposed ID | Required observation |
| --- | --- |
| test_automatic_minor_then_major_at_fixed_budget | In separate OFF, retain+extras, KILL+extras children, a bytecode allocation loop witnesses actual minor then major, unchanged M across minor, useful young reclamation/payload and old-cycle/resource reclamation within a predeclared measured-budget bound |
| test_manual_minor_consumes_only_successful_opportunity | A real zero-death minor closes H; repeated minors preserve D/M; a full major reopens H; no reliance on death count as success |
| test_empty_nursery_probe_retires_without_collection | OFF, empty diagnostic ledger, actual young_pending=0 prerequisite: old-only bounded allocation crosses H, closes H without gen0/gen2 count changes, then major runs at M; no policy setter clears the hint |
| test_due_major_overrides_halfway_opportunity | Accrue actual due debt with automatic GC disabled, enable, and observe major before minor; useful rooted young payload and old-cycle reclamation |
| test_threshold_disable_reenable_preserves_epoch | Check threshold0, nonzero relative deadline change, disabled admission/manual minors, re-enable due debt, no-op enable preserving retry and stress off restoring the same M/H |
| test_cadence_oom_preserves_opportunity | Use existing one-shot faults with actual prerequisites: minor mark/ledger/inventory stages1/2/3; major preparation stages1/2/3. Capture failed attempt fields, unchanged D/M/H and finite N0 retry, then real successful retry; retain existing promotion-after-prep-OOM expectation |
| existing minor_during_allocation_stress | Strengthen stress1/1000 to require both real generations with payload, while the budget is large enough that major-due priority does not mask the minor; inline evidence uses actual stats/state, not absent Python callbacks |
| existing inline/queue/compiled-root controls | Explicit inline recovery remains major even with an open H; callbacks/finalizers remain deferred/routed as before and retain useful resource checks |
| test_configuration_inside_collection_callbacks | Start/stop/nested callback threshold/stress/disable changes obey the declared transaction boundaries; rejected nested minor changes neither M nor H |

Ordinary automatic/OOM tests must prove the selected kind actually attempted.
For breaker-path OOM, a start callback can arm the existing one-shot fault for
that nominated kind; its stop snapshot captures the failed state before it
disables future automatic traffic. It must not call a policy setter or wrap
GC APIs. Validate each failure stage fires; do not fabricate minor lifecycle
work for an allocation family that cannot supply it. Existing manual failure
checks remain with their original scope. Inline/stress fault evidence cannot
claim callback execution when no-Python delivery intentionally suppresses it.

The initial focused roster has thirteen parent methods and twenty-two fresh
children: seven new methods above (3+1+1+2+1+6+1 children), the existing
stress method (2), existing finite-major/debt/old-boundary methods (1 each),
and these two unchanged full-entry/queue controls (1 each):
`test.test_gc.TracingCollectorTests.test_settrace_runs_queued_callbacks_first`
and `test.test_gc.InterpreterQueueTests.test_queued_payload_survives_collection`.
All have zero planned skips on the current supported debug host. Keep the
other existing nursery cases for the subsequent coherent GC feedback/closure;
do not count them as executed by this focus roster. Each of the following
three source negatives selects one nominated child only.

Three small temporary source ablations make the new claims falsifiable:

1. Force the automatic selector's chosen minor to major, leaving manual entry
   untouched: the same default sequence case must fail its actual-minor witness.
2. Add RETRY_BYTES to M after each successful minor: the same strengthened
   finite-major case must exhaust its frozen bound and fail the named automatic
   major assertion, not merely an earlier debt equality check.
3. Close H before a selected minor enters marking: the same OOM opportunity
   case must fail its preserved-H assertion. This detects treating attempts or
   failed-preparation statistics as successful opportunity consumption.

Each negative gets reviewed exact source and fresh products; never patch a
sealed baseline image or ship an option. Existing force-manual-major and debt
reset controls remain parent evidence; independent review names any affected
reruns for this new source rather than relabeling old products a cadence pass.
Expected assertion failures are diagnostic auxiliaries. First unexpected
failure stops; missing kind/root/resource/fault witnesses are not passes.

## Elapsed-first cost-rule revision for review

This paragraph proposes replacing the older instruction-geomean acceptance
gate; it does not edit the frozen adoption or causal input documents. The
user's priority is actual performance. Instructions and text size diagnose a
result and do not veto a faster candidate by arbitrary proxy thresholds.

Retain one 60-minute quiet window, matched O3 source/tools/configuration,
unchanged workload hot loops and fixed threshold0=32 MiB with the original
max(threshold,live_bytes) major policy. Compare three frozen source arms:
historical pre-adoption default as a temporary comparison, adopted automatic
majors, and adopted halfway cadence. No historical source is offered as a
runtime mode. Freeze two rotating-order rounds, five fresh samples per arm for
coroutines/generators/comprehensions and Richards: 120 processes. Preserve all
raw elapsed/instruction vectors, medians/ranges, actual deadlines/kinds/pauses,
peak RSS and repeated post-major retained counts. Separate debug age evidence
from release measurement; include real scheduling/checking work in release cost.

Primary acceptance is elapsed: the GC-heavy three-row geometric median ratio
(baseline/candidate) must exceed 1 in both rounds, with a consistent result not
explained by order/load/control drift. No predeclared instruction improvement
or text-size ceiling applies. Report each workload and absolute time saved;
no workload/Richards may show a repeatable elapsed regression above 2% in both
rounds for this default-policy increment. A lower instruction count with slower
elapsed fails; higher instructions or larger text with faster elapsed can pass.
Five samples and descriptive ranges are not a confidence interval. Inconsistent
signs or material control drift mean inconclusive, not an automatic repeat or
a relabeled win; any further exposure requires a named remaining-budget choice.

Memory is an explicit resource tradeoff. Stable post-major retained bounds and
finite old/resource reclamation are correctness requirements. Peak RSS within
5% of the matched adopted baseline is the existing simple acceptance envelope.
A larger but bounded RSS increase is not an instruction-like correctness veto:
report absolute MiB and elapsed benefit for a coordinator decision before
acceptance. Unbounded retained growth, missed cleanup or exceeded concrete
resource constraints rejects the increment irrespective of speed. Do not hide
that tradeoff in an aggregate or silently change the envelope after results.
Also report mandatory-adoption versus historical elapsed separately; explain
or correct a repeatable >2% regression rather than burying it in a cadence gain.

If halfway loses elapsed, lacks a stable bound or has an unaccepted resource
tradeoff, reject that source increment and retain mandatory admission/manual
minors with the adopted automatic-major schedule. Do not restore optionality.
A different cadence needs its own bounded hypothesis and review.

## Finite handoff

Proposed source checkpoint: 45 minutes from first authorized source edit,
with a 15-minute concrete source/status checkpoint. Independent source review
then one focused feedback/negative batch with finite, downward-clamped budgets
precedes quiet measurement or broad qualification. Freeze literal rosters,
paths/configuration/products and exact negative failure conditions before any
native grant. Scheduling waits do not silently extend the original overall
deadline. If the remaining time cannot support a coherent decision/qualification,
report the held/rejected increment and the adopted baseline honestly.
