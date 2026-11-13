# Independent cold checkpoint admission assessment

A bounded correctness-only checkpoint at
`61015a9452d9069646fb5fede9ff963896833b89` is reasonable with the explicit
qualification limits below. I find no demonstrated material blocker in the
reviewed three-file correction. This is not a green full-suite, performance
winner, upstream win or release-readiness verdict. The coordinator owns the
selected decision; this review changes no selection.

The benefit is concrete: the old public frame-to-generator helper copied a
NULL association then asserted it equaled the passed frame object. Root
reproduced that baseline failure. The correction establishes an owning
association, supplies the missing discard-first-input recipe, and completes
subtype initialization before publication. It also moves fallible coroutine
origin creation before owner transfer. Independent actual-source/native
reviews found the bounded ownership protocol coherent; the normal creation
path remains separate. Its own 808 focused tests per role, direct subtype/
escaped-mapping regressions, 48 injection points × 3 subtypes × 2 roles with
reference-count checks, and strict 306-unit AST gate support this fix. They
do not replace broader qualification. See [qualification](qualification.md)
and [native review](cold-construction-post-review.md).

Direct performance evidence supports a modest checkpoint tradeoff rather
than a claimed optimization. Against golden51, the two-pair all29 aggregate
is 1.0063812430291168, with 11/29 workload geomeans below 1. The separate six-pair
hot aggregate is 0.9966694075820465: generators 0.9969878364,
coroutines 0.9978604167, DeltaBlue 1.0056459408, Richards 1.0070190876,
comprehensions 0.9938829135 and logging 0.9788791556. The material repeated
coroutine losses that rejected the performance experiments are absent from
this candidate's measured hot confirmation. This is an observation of these
samples, not proof of elapsed equivalence or a bound on future regressions.

Logging remains a limitation: its all29 result is 1.1383467977 in two similarly
losing pairs, while its hot confirmation is 0.9788791556 with a wide pair
range 0.876076–1.005017. Retain both; do not dismiss the all29 loss, pool the
campaigns or invent a hardware explanation. The actual normalized hot native
sequences match selected51 in both roles, but unchanged source/sequences do
not prove identical elapsed behavior. Fresh upstream all29 is 1.1565432755
with 6/29 wins, so no upstream performance leadership is supported. The first
upstream hot6 campaign failed its load gate; partial results are not evidence
of a completed confirmation. [Tables](benchmark-tables.md) preserve scopes.

Both full runs failed after 54,692 tests each. Nine debugger identities match
the established baseline diagnostic failures; debug LLTrace also matches.
The GC-verifier failure reproduces on selected51 and the candidate in 4/6
alternating targeted runs each, establishing that diagnostic's preexisting
instability without proving an exact conservative root or absence of every
GC defect. No new quarantine or weakened expectation is justified.

The profiling failure is the material unresolved qualification risk. It uses
fake-clock event accounting, so it is not wall-clock jitter. Its original
candidate full-run failure remains real and unexplained. Three targeted
repeats plus whole-module and package comparisons pass on each debug build
(ten subprocesses), but no selected51 failure was reproduced. Therefore it is
**neither proven preexisting nor fixed**. Source scope supplies no demonstrated
connection to the cold constructor, but cannot exclude one. Retain the finding
and exact evidence as an open limit; do not label the suite green, call it
flaky, or quietly omit it. See [profiling triage](profiling-diagnostic.md).

On that basis, selecting this as a local correctness checkpoint with those
visible limits makes sense: it fixes a reproduced ownership/construction bug
with independent direct qualification and no demonstrated new regression
mechanism. Selection must not imply that the profiling regression question
has been resolved. A policy requiring every novel full-run failure to be
classified before any checkpoint would keep this candidate pending; that
stronger clean-full condition has not been met. Do not admit any rejected
performance lane or widen this patch to repair unrelated diagnostics as part
of this decision. No further jobs or source changes were performed for this
assessment.
