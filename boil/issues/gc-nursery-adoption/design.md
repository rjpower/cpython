# Adopt age admission, minor semantics and checking

Design proposal, 2026-10-31. Inspected merged source
`d98291018c9665ae3db6952da0b0a40a4e04992b`. The user has decided to keep the
nursery architecture. This design removes the temporary whole-runtime switch;
it does not reopen that architectural decision. No runtime edits or native
work accompany this proposal.

## One runtime contract

Remove `GCState.young_enabled`, its initialization, every conditional consumer,
`_testinternalcapi.gc_set_young_enabled` and its method registration. Add no
replacement environment variable, command-line option, feature flag or build
configuration. Age admission belongs to allocation paths: existing exact
cells/nonempty tuples are young from initialized publication, while other
families remain old from birth until a reviewed allocation-specific change.
Empty immortal tuples and tuple subclasses retain their current treatment.

`_PyGC_YoungAllocated` always sets age and the non-rooting pending hint for
its admitted allocation. It uses the existing attached-thread constructor
contract rather than inventing a supported nursery-off context. Keep the
existing shared-GIL/mimalloc platform scope; this change does not claim a new
free-threaded or foreign-interpreter lifetime implementation. Existing domain
and ownership guards remain, rather than silently changing generation zero
into a full collection on an unsupported request.

| Entry or observation | Adopted behavior |
| --- | --- |
| `gc.collect(0)` | Actual minor, including when automatic GC is disabled |
| `gc.collect(1)`, `gc.collect(2)`, `gc.collect()` | Full major through the same mark/death engine |
| C full-collection entries, shutdown | Full major; preserve existing C enabled/disabled semantics |
| `gc.get_objects(0)` | Current tracked young cohort, without implicit collection or promotion |
| `gc.get_objects(1)` | Empty reserved generation, as in the existing fork |
| `gc.get_objects(2)` | Current tracked old objects |
| `gc.get_objects()` | All current tracked objects, without implicit promotion |
| callbacks/statistics | Generation zero for real minors, two for real majors; retain public totals and byte units |

Retain stable addresses, all-old conservative retention, ordinary lifecycle
preparation/sweep and successful-trace survivor promotion. No old-to-young
barrier or new allocator is introduced. Debug/SAVEALL, reftracer, weakref,
resurrection, hook and constructor/OOM rules still apply. The existing
`gc.enable/disable` and threshold-zero APIs control automatic scheduling; they
do not disable age admission or manual minor semantics.

## Scheduling hypothesis and major debt

First establish mandatory admission/checking against the current automatic
major-only schedule. That is the adopted source baseline for comparing one
small cadence change, not a permanent runtime choice exposed to programs.

Candidate cadence: issue at most one automatic minor request halfway through
each existing major allocation budget, then perform the major at the existing
budget. A request can still perform the existing bounded lifecycle follow-up
passes; the one-request limit does not suppress required callback work.
Do not reinterpret threshold0 as object counts, raise the major budget, use
threshold1/2 as hidden nursery toggles or retune the trigger during comparison.

The current `alloc_bytes` is allocated bytes since the last major. Keep it.
For the cadence candidate, separate the major debt deadline from the existing
`alloc_limit` scheduling/retry probe, and retain one per-major-epoch cadence
state. These are scheduling fields, not nursery admission modes:

- A successful major resets debt to zero, sets its new deadline to
  `max(threshold0, live_bytes)` as today and starts a fresh minor opportunity.
- Its minor probe is halfway to that deadline, bounded to at least one byte.
  At a probe, prefer a major whenever its debt deadline is already due.
  Otherwise, if young work is pending and this epoch has not had a successful
  minor, request a minor. Success consumes that opportunity and schedules the
  unchanged major deadline. If no young work is pending, retire the automatic
  opportunity without a collection and schedule the major deadline; do not
  spin at the already-passed halfway probe. A conservatively stale pending
  hint may cause extra work; it may never prevent the major.
- A successful explicit minor can consume the same epoch's opportunity. It
  preserves allocated-byte debt and the major deadline. Repeated explicit
  minors cannot postpone automatic old-cycle/resource reclamation.
- Retry suppression in `can_collect` may move only the next scheduling probe,
  not the major debt deadline. Failed marking/inventory retains the graph,
  age and debt. Lifecycle-preparation failure also retains objects and debt,
  but need not undo survivor promotion already committed by a successful
  mark/inventory. Dead records retained after preparation failure keep their
  young age and pending hint. Failure gets the existing finite allocation
  retry distance; it neither advances the successful cadence nor resets debt.
- `gc.set_threshold` intentionally establishes the caller's new deadline
  relative to current debt, preserving existing byte-threshold semantics.
  Reset the cadence opportunity consistently with that explicit change.
  Threshold zero and `gc.disable()` suppress automatic requests. Re-enabling
  automatic GC honors debt already accrued; it does not reclassify objects.
- Initialization, stress reconfiguration and teardown initialize/retire all
  scheduling fields together. Use bounded nonnegative arithmetic for new
  deadlines; avoid signed-overflow additions in the new policy.

For the cadence increment, stress remains an execution/checking policy. At each
existing Nth-allocation stress opportunity, use the same epoch ordering:
minor first, major next, repeating after successful majors. Preserve inline
no-Python queue delivery and breaker draining. A due major takes priority;
failures do not advance the successful cadence. Stress-off returns to the
existing debt deadlines without clearing age. Stress does not skip a chosen
minor solely because the pending hint is empty: it must exercise the actual
transaction. Verify both real kinds execute.

The halfway policy may lose: each minor still scans old objects and inventories
the heap. If it fails the declared cost gate, retain mandatory admission,
manual minors and automatic majors while selecting a separately bounded
better cadence. Do not retain both schedules behind a runtime flag. The
architecture stays adopted while a poor policy increment is rejected.

## Verification must execute the adopted minor

Remove the blanket verifier/stress rejection from `_PyGC_Collect`. Neither
KILL nor retain may silently run a major for a minor request. Exercise the same
minor transaction in those modes, including extras `pymem` and `structs`.
Do not remove the five nursery tests from the KILL selection or preserve their
current unsupported-mode skips.

Source inspection identifies a necessary A/B provenance change, beyond
deleting that rejection. `verify_mark_b` currently reports a previous retained
candidate as reached "from the roots" whenever A now marks it. A minor's
artificial all-old boundary is not proof of such a root reappearance.

Carry an explicit collection-kind value into the existing verifier scratch
context. On a minor, new B-only candidates and KILL/quarantine eligibility
are restricted to young objects. Old objects retain their required minor
protection. The retained ledger `ts->q` is diagnostic metadata, never roots.
Use these exact minor rules after A and B have completed, before the inventory
clears their mark bits:

| Prior retained record | Minor action |
| --- | --- |
| Old object | Carry its original record; the old boundary protects its storage |
| Young object marked by A or B | Carry its original record and provenance |
| Young object marked by neither | Retire the record without a root report; the object can die normally |
| New young B-only object in retain mode | Insert its origin record, unless an older carried record already identifies that object |

Suppress prior-record root-reappearance reports for **both** ages on every
minor. An earlier verifier update followed by inventory OOM can leave a
retained candidate young; a later old-owner closure may mark it artificially.
Testing only the old age bit would therefore still issue a false report.
The next genuine major decides root reappearance using the ordinary genuine
A roots, not a minor's artificial retention boundary.

Do not let `verify_mark_b` replace `q` with only the new young list, as its
current wholesale replacement would lose carried old records. Construct the
full replacement by an explicit sorted merge of carried records and new young
B records. Deduplicate by object identity, preserving the existing record's
original origin/collection provenance when both inputs identify it. Do not
insert a prior record into tracing to preserve it. Truly dead young records
must leave the ledger before their blocks can sweep. Any violated old-boundary
storage invariant fails the mark closed rather than retaining a stale pointer.

Build the replacement and any required KILL membership metadata in scratch
storage. Install the complete replacement only after all fallible ledger work
succeeds; on ledger OOM, keep the original ledger intact, discard scratch,
mark the attempt failed, clear scratch marks and free/promote nothing. Preserve
the existing KILL diagnostic-buffer fallback only if its missing origins
cannot affect quarantine ownership or eligibility. A partial ledger merge is
never installed. This is a verifier-ledger transaction, not a new object store.

The ledger commit boundary is the completed A/B comparison, before the later
dead-object inventory and promotion. An inventory OOM after that commit can
retain all objects with their previous ages; the committed diagnostic ledger
still describes the completed comparison. A lifecycle-preparation OOM occurs
later still: safe survivor promotion has already happened and is not rolled
back. The all-age minor suppression and carry rules cover both cases without
pretending every retained candidate has become old. Major reports remain
observations of genuine A roots in that checked attempt, even if a later
inventory/preparation failure prevents reclamation.

Use one exact sequence for provenance and quarantine age: a configured live
verifier attempt advances `live_collections` once after the initial scratch
set is available and the heap head lock is acquired, before quarantine release
or page inventory. The counter advances for majors and minors that reach this
point, including attempts that later fail. Failure before this point, a rejected
reentrant entry and inspection without collection advance it zero times.
Each actual lifecycle follow-up mark attempt counts separately. Move the
increment out of `verify_mark_b` so it cannot happen twice. This intentionally
changes the diagnostic counter from its current B-entry timing. Use that one
`now` value for origin `rec.collection`, quarantine tags/expiry and the
last-collection report; do not maintain competing sequence definitions.

For a KILL attempt numbered `now`, release blocks tagged with sequence at most
`now - QUARANTINE_COLLECTIONS` under that lock **before** building page ranges,
as required by the current stale-range guard. Newly quarantined blocks receive
`now`; a later OOM does not roll back the counter or resurrect expired blocks.
Count mixed minor/major attempts equally, not just successful sweeps. This
defines exactly when expiry proceeds while keeping freed pages out of the
upcoming inventory. Preserve origin identities and sorted/deduplicated records
through the mixed sequence.

This uses one existing tracer/checker with a collection-domain restriction,
not a second tracing engine or a production roots mode. The major continues
to check all families. Artificial old retention can mask a young root hole
through that retained old closure by design, so tests must include a genuinely
young B-only root fixture that has no retained old owner. Domain-aware KILL
and direct old-boundary controls together are required; a full-suite pass
alone cannot prove the restriction correct.

The separate post-collection checker `_PyGC_TraceAfterCollect` currently filters
out generation-zero entries. Inspect its assumption before changing that
filter: its exact full-reachability claim cannot treat intentional floating
old garbage as a leak. Keep explicit full verification a genuine major and
use the live minor A/B checker for minor safety. If any minor diagnostic is
exposed, label its age domain honestly; never claim full old-heap root coverage
from a conservative minor retention set.

## Tests and source scope

Migrate `MinimalNurseryTests` into ordinary nursery contract tests. Remove the
setter prelude, restore-mode cleanup, mode-change rejection test and verifier/
stress nursery skips. Test reentrant collection/debt preservation instead.
Keep explicit final major cleanup for resource tests; it is a semantic request,
not a way to turn the nursery off. Do not duplicate the suite into enabled and
disabled variants. Historical archived evidence keeps its original meaning.

Required useful controls before broad qualification:

1. An unconfigured child observes age admission, real generation-zero callbacks,
   young reclamation and in-place promotion. Old-owner-to-young payloads survive
   minors; unreachable old owners/cycles die on the subsequent major.
2. OFF, retain, KILL and their relevant extras run actual minors. A rooted
   young cell/tuple survives; a safely instrumented deliberately omitted young
   C root yields the existing retain diagnostic or KILL trap at the nominated
   read. No old owner accidentally masks the control.
3. Old and still-young retained verifier candidates cross minors without false
   root-reappearance reports or lost provenance; include the still-young case
   created by inventory OOM after ledger commit, followed by old-owner closure.
   A genuinely dead young record retires before its storage frees. The next
   major produces the expected genuine-root report/retirement. Verify sorted
   merging/deduplication, original provenance, ledger OOM leaving the original
   ledger intact, and exact quarantine expiry across failed and mixed attempts.
   In particular, witness expiry at the declared 16-attempt distance, rather
   than merely observing eventual storage release.
4. The halfway candidate's default automatic execution records a real minor
   followed by a major at the declared byte budget. The adopted major-only
   scheduling baseline records actual automatic majors and explicit minors.
   Repeated manual minors, rejected/reentrant requests,
   queue callbacks and mark/preparation OOM do not reset or postpone major
   debt. Stress1000 and focused stress1 execute both kinds with useful data.
   Check zero threshold, disabled scheduling, re-enable and threshold changes.
5. `get_objects` preserves age observations without forcing a collection;
   collections/stats/callbacks agree on kind and existing public byte totals.
   Inspection during collection keeps its existing no-reentrant-work rule.
6. Use temporary source negatives: force a requested minor to major and fail
   the same old-owner retention assertion; reset major debt during minor and
   fail the same finite-major witness; omit the young C root and get the
   expected checker failure. Never ship those ablations or a toggle for them.

Expected runtime paths: `Include/object.h`, internal `pycore_gc.h`,
`pycore_interp_structs.h` and, only if declarations change, `pycore_gc_trace.h`;
`Python/gc_live.c`, `Python/gc_trace.c`; `Modules/_testinternalcapi.c`,
`Modules/gcmodule.c`; `Lib/test/test_gc.py`. Exact cell/tuple constructors
already call the helper; preserve those sites. Add only a read-only/resettable
internal witness if existing stats cannot prove debt/dispatch. No witness may
set age policy. Update `Doc/library/gc.rst` and active Boil contracts/statuses
for the settled semantics. Search all live consumers of the removed setter and
field; preserve unrelated legacy user notes and immutable historical receipts.

## Finite policy comparison and acceptance

After independent design review, the coordinator dispatches at most eight
elapsed hours for implementation, review, feedback and the bounded policy
decision. The first source checkpoint, within 90 minutes, covers mandatory
admission, manual-minor semantics, age inspection, actual verifier/stress
support and test migration while retaining ordinary automatic-major scheduling.
It does not add the halfway automatic cadence or its scheduling fields.
Fix the existing failed-major-preparation debt reset in this first increment:
only a successful major resets `alloc_bytes` and establishes its new normal
allocation limit. A failed major preserves debt and uses the existing finite
retry mechanism. The separate major-deadline/probe representation remains
part of the later cadence increment; this fix does not add a cadence option.
Independent source review and first debug feedback follow within a further
90 minutes once a native slot is available.

After that correctness checkpoint, freeze its source as the adopted
automatic-major baseline. Only then review and implement the distinct halfway
cadence increment, including its separate debt deadline/probe fields. Stress
checks in the first increment already execute explicit real minors and retain
the existing automatic stress-major path; the later cadence comparison can
alternate automatic stress opportunities as described above. This separates
mandatory checker support from a discretionary automatic scheduling change.

Stop on the first unexpected failure and retain it. Scheduling waits do not
silently extend the overall dispatch or source budget; an expired batch needs
an explicit remaining-work decision rather than silent continuation.
Qualification follows the actual default GC/checker change under the existing
validation contract, not a bridge from the opt-in source's narrower coverage.

Before quiet measurement, freeze the byte policy and three source arms:
merged pre-adoption source, mandatory adoption with automatic majors, and
mandatory adoption with the halfway cadence. The pre-adoption source is a
temporary comparison artifact, never a retained runtime mode. Use the same
compiler/options/origins, fixed work and threshold0 of 32 MiB with the unchanged
`max(threshold0, live_bytes)` major rule. Record the actual major deadlines;
do not call the configured threshold a measured constant heap size. Keep
manual minor functional traffic separate from unchanged benchmark hot loops.

Allow one 60-minute quiet comparison: two rotating-order rounds, five fresh
process samples per arm for coroutines, generators and comprehensions. Include
Richards as a bounded low-GC control. Freeze exact order and control sample
counts in the manifest before launch. Record all raw elapsed/instruction
vectors, actual minor/major/debt witnesses, collection pauses, peak RSS and
post-major retained counts. Use separate debug feedback for otherwise intrusive
age/promotion instrumentation; total release costs must include its real work.

Accept the halfway cadence only if both rounds reduce the three GC-heavy rows'
geometric instruction cost by at least 1% against adopted automatic-major-only
scheduling, elapsed geometric medians improve in both rounds, no individual
row/control has a repeatable regression above 2%, peak RSS stays within 5%,
and repeated batch/full cleanup reaches a stable retained-object bound.
Report ranges as descriptive observations, not confidence intervals.

Separately compare mandatory adoption against the pre-adoption source. Any
repeatable regression above 2% requires a bounded explanation/correction before
acceptance; do not hide it in the cadence aggregate. If the cadence is rejected,
remove that source increment and settle the single automatic-major scheduling
policy for this increment. If mandatory semantics/checking fail, correct that
implementation; the user decision to keep and improve the nursery remains.
No outcome authorizes restoring global optionality or widening to coroutine
coverage, barriers, page ownership or copying without its own reviewed scope.
