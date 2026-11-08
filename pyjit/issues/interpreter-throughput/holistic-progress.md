# Holistic design review progress

2026-11-06: Read current frame stack, generic consuming binder, exact call
overlap, compiler liveness/occupancy and verifier, frame GC/copy/cleanup, and
generator flush/publication state at runtime `6a0e4c12c17`. Wrote canonical
call/frame lifetime contract and bounded holistic review. Reviewed Terra's
call proposal and frame-liveness proposal; sent generator worker a request
to prove an emitted duplicate before changing generic publication.

Recommended actual increments: one common prepare/guard/commit binding route,
reuse already-NULL overlap homes and initialize only a new tail, and remove a
proven generated duplicate publication. No new bitmap, runtime representation,
second compiler or normal-return cleanup rewrite. Runtime/builds remain
read-only for this reviewer; root owns implementation integration and checks.

Follow-up: accepted the actual call artifact after fixing post-cleanup refresh
and adding warmed lifetime/frame/generator tests; reviewed the typed empty-tuple
owner correction. Accepted the separate overlap initialization artifact.
Rejected distributed generator cache state in favor of one retirement macro
owning clear-inputs/publication; source post-review is in publication-review.md.
All runtime edits, generator execution, compilation and tests remain with the
coordinator and implementation agents.

Follow-up C proposal: reuse the existing published-result postcondition so
specialized attribute retirement leaves only the final folded-local store.
Recorded current GCC ELF identity and exact remaining final-store offsets in
final-publication-proposal.md. Also accepted the status!=0 assertion predicate
for the legitimate no-child fallback case. No runtime changes or execution by
the reviewer.

Reviewed the focused C semantic test: fresh mortal receiver, temporary/folded
local destination, GC reentry, old/new local finalizer order, weakref lifetime,
and specialization installation before/after the observed call. Source accepted
with explicit dynamic-coverage limitations; no test execution by reviewer.

Conditional admission synthesis: recomputed A/B/C seven-kernel screens and
read retained B GCC output. Wrote admission-review.md while full suites run.
Keep frozen through scheduled final evidence; B's elision has no demonstrated
overall win yet and should not be sold as a speedup. Prefer dropping its extra
arithmetic absent benefit, preserving independent assertion/comment fixes.
No new implementation, tests, builds or measurements by reviewer.
