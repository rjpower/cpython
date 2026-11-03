# Nursery adoption design review, 2026-10-31

**Clear the bounded first source increment:** mandatory existing-cohort
admission, real manual minors, age-aware retain/KILL checking and test migration,
with the existing automatic-major schedule. No nursery disable flag remains.
The halfway automatic cadence is a distinct later increment requiring its own
source review and the declared cost decision; it is not selected merely by
this design approval.

Reviewed `design.md` SHA-256
`046b8ab07625cb301f5cc6c2be952844a2fbefc714fbdf3e045c9e6635f68729`
and `status.md` SHA-256
`2b0dc61ccaf4e8469a222df265bf566441a4018ad6c81615a20b854e4110f9ed`,
against merged source `d98291018c9665ae3db6952da0b0a40a4e04992b`.
Inspected allocation admission, heap/age inspection, A/B marking and retained
records, quarantine release, collection/preparation, debt/retry/stress paths,
public GC entries and existing test consumers. No runtime source, native job,
build, benchmark or commit was made by this design reviewer.

## Alignment and bounded scope

The user's architecture decision is preserved. Removing `young_enabled`, the
private setter and every live conditional consumer gives one age/admission and
minor/major contract. `gc.disable()` and threshold zero continue to control
automatic scheduling rather than object age or explicit minor semantics.
Exact cells/nonempty base tuples remain the admitted family; other families
remain old from birth. This removes whole-runtime optionality without adding
an allocator, copying, barriers or a second collector.

Separating adoption/checker correctness from the halfway cadence is useful.
Mandatory tuple admission alone can add a promotion census on automatic
majors, while every minor still scans old objects and inventories the heap.
That cost must be measured explicitly, not hidden by an enabled/disabled
shipping mode or attributed to an unimplemented broader nursery. The first
checkpoint adds no cadence/deadline fields. The later source-only scheduling
comparison has a declared integrate/reject gate and retains a single schedule.

The existing attached-thread constructor and supported GIL/mimalloc/main-domain
boundaries remain implementation obligations. Removing the setter must not
remove the remaining domain guard or silently make an unsupported minor a
major. Every public entry, including full C/shutdown entries, keeps the
explicit kind described by the design.

## Blocking findings resolved during review

1. **Retained records need a real merge, not a young-only replacement.**
   `Python/gc_trace.c:3162` currently replaces `ts->q` with the latest B list.
   Restricting new minor candidates to young objects would discard all old
   retained records. The revised table carries protected old records and
   surviving young records, retires genuinely dead young records before sweep,
   and merges new young B records in sorted identity order. Duplicates preserve
   original provenance. Ledger records never become roots merely to keep
   their pointers valid.

2. **Old-only report suppression is insufficient.** An A/B ledger commit can
   precede inventory OOM and survivor promotion. A previous retained candidate
   may therefore remain young. A later minor's artificial old-owner closure
   can mark that young candidate without establishing genuine root
   reappearance. The revised design suppresses prior-record root reports for
   both ages on minors and lets a genuine major decide them. It requires the
   exact young-after-OOM control, not just an old-record example.

3. **OOM boundaries must describe actual commits.** Ledger scratch failure
   leaves the original ledger intact and makes the current mark fail closed.
   A completed A/B ledger may stand if the subsequent inventory fails; ages
   then stay unpromoted. Lifecycle preparation comes after successful inventory
   and safe survivor promotion, which need not be rolled back. Retained dead
   records keep young age/pending work. The revised text no longer promises
   an unimplemented general rollback of promoted survivors.

4. **Mixed-kind quarantine age needs an exact sequence.** The revised design
   intentionally moves the single sequence advance from B entry to the
   configured mark attempt under the head lock, after initial scratch setup
   and before quarantine release/page inventory. Later failure still counts;
   rejected/reentrant/no-mark work does not. Follow-up passes count separately.
   Provenance, killed records, expiry and diagnostics use that same sequence.
   Release at `now - 16` precedes page-range construction. There is no second
   increment in B and no rollback that could recreate expired storage.

The first increment also explicitly fixes the current failed-major-preparation
debt reset at `Python/gc_live.c:1926`. Only a successful major resets debt and
establishes its normal allocation limit; failure retains debt and uses bounded
retry. This is a focused correctness fix. The separate major deadline/retry
probe representation and automatic minor opportunity remain in the later
cadence increment.

There are no remaining blocking design findings in the reviewed snapshot.

## Implementation and evidence requirements

Independent source review must inspect the real ledger merge/publication and
failure exits. In particular, verify object storage is valid before reading
age, sorted identity/provenance remains correct, old records do not acquire
production-root status, and dead young records cannot outlive their blocks.
Any optional missing-origin diagnostic fallback must leave KILL membership,
quarantine ownership and age eligibility intact. The sequence change is an
intentional diagnostic behavior change and needs exact mixed/failed-attempt
expiry witnesses, not merely eventual cleanup.

Required first feedback includes unconfigured admission and generation-zero
callbacks, real young reclamation, stable-address promotion, old-owner payload
survival and eventual major reclamation. OFF/retain/KILL and relevant
`pymem`/`structs` variants must execute actual minors. A genuinely young omitted
C root must produce the expected retain diagnostic or instrumented KILL trap;
the existing old-instance missed-root fixture alone cannot prove this. Keep
its target out of artificial retained-old closure.

Exercise old and still-young retained records, genuinely dead young retirement,
merge deduplication, original origins, ledger/inventory/preparation failure,
reentrant requests, debt/statistics and readonly age inspection. Preserve
partial-publication protection and constructor/OOM safety. Narrow test fault
injection may follow existing GC allocation-failure patterns; it must not
become a production age switch or manufacture the behavior witness.

Remove all live setter/field uses and the five unsupported-mode test skips.
New qualification helpers must derive the actual current test roster and
unsupported policy; historical immutable helpers/receipts retain their original
meaning. Update the public GC documentation and active contracts with the
actual settled semantics. Keep unrelated recovered user notes untouched.

The accepted source checkpoint is ninety minutes, with independent source
review and first debug feedback within the further stated checkpoint and
eight-hour total dispatch. Stop and retain the first unexpected failure; a
budget expiry needs an explicit remaining-work decision. Qualification covers
the actual default architecture/checker change, rather than carrying the
opt-in source's narrower nursery support as proof.

The later cadence increment still needs its exact byte policy, finite-major
debt/retry proofs, literal process/control schedule and three frozen source
arms before measurement. The design's one-hour comparison and per-policy
acceptance gates are a useful bound. A losing cadence does not undo mandatory
admission or create a permanent runtime fallback; another scheduling design
receives its own bounded decision. Broader coroutine admission remains a
separate rebase/design task.
