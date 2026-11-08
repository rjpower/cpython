# Implementation contract progress

2026-11-08. Design-only assignment; runtime worktree read-only. Deadline
12:25 UTC, M1 first and M2b second. No builds or native workloads run here.

First source packet: the existing generator already implements sequential
two-owner MayDealloc checks in `retire_published`; reuse that code rather than
introducing a general effect-state engine. Dictionary first-probe lookup can
reuse `_PyLong_ValueHash` and dictionary-owned index helpers, returning an
owned canonical result or a no-effects decline. The generator must still
check its trusted provider/decline/completion pattern before omitting refresh.

M2b can be narrower than a new opcode/metadata migration: only the generated
exact-boolean taken-branch continuation needs to inspect an ordinary NOP
target. NOP retains its word and PC. Instrumented targets, debug lltrace,
DTrace-enabled builds, non-GIL builds and stale instrumentation can decline.
The completed contract now selects an explicit zero-I/O final DSL uop invoking
IR_EXECUTE_ORDINARY_NOP, appended only to the ordinary exact-boolean false
branch. The predicate reproduces ordinary NOP semantics: fresh ordinary target
handler under the GIL, debug lltrace disabled, precise PC store retained.
Epoch/observer absence and DTrace guards are unnecessary because ordinary NOP
has none; wrapped targets use their existing dispatch. The earlier conservative
draft predicate is superseded by this direct source-equivalence proof.

Coordinator priority interrupt: completed an independent P1/P2 source review,
with no must-fix finding, in `p2-source-review.md`. Release/debug compile and
executed qualification remain coordinator-owned and are not claimed here.

The M1/M2b contracts are complete and available for coordinator review in
implementation-contracts.md. M1's provider returns a canonical owned result or
no-effects NULL decline; a checked terminal generated edge consumes/publishes/
retires in order and leaves the old generic slow body unchanged. It reuses the
existing sequential retirement emitter. M2b retains its actual NOP word and
all wrappers and changes no word format or feedback.

Follow-up priority work: p2-post-review.md accepts the coordinator's executed
117-module release/debug builds and 540-test focus runs (release skips 11,
debug 9), with no performance claim. The disposable route diagnostic source
also received an independent bounded review; its terminal partitions and
overlapping shape subsets were distinguished. Coordinator route capture now
establishes ten exact-thread and ten successful overlap calls per loop, with
kwargs/varargs on every overlap and no hot consuming-binder fallback.

Coordinator accepted M1/M2b for bounded implementation. Added the separate
p3-proposal.md for guarded unbound Python-call entry, with shared record
initialization/commit and a success-invocation-only emitter override; its
generic fallback stays generic. The coordinator accepted that proposal too.

Reviewed Terra's actual M2b source, including its final boxing classifier
guard, in m2b-source-review.md: accepted with no must-fix. Whole-uop source
string duplication was removed during coordinator feedback; the final
validator uses trusted prefix sequence, narrow bool guard, scalar/ownership
shape, effects, terminal placement and no-box checks. New observer tests have
been inspected as source. Runtime builds, generated-product and executed/GCC
post-review were pending at that source-pass snapshot.

Final packet, 12:07 UTC: m2b-post-review-round2.md accepts the final seven-file
increment for coordinator landing after inspecting both successful 117-module
builds, both 805-test/one-skip focused runs, generated products and fresh GCC
handler assembly. The first test failure remains historical: dis.starts_line
is boolean here; the corrected line_number case passed old-R5 control and
M2b before the complete reruns. The tier1-only scope and unused legacy expansion
table limitation are explicit in the addendum. No performance/full integrated
qualification claim, runtime source edit, build or native workload by this
reviewer. The assigned design/source/evidence packet is complete; coordinator
owns landing, M1/P3 implementation and subsequent measurements.
