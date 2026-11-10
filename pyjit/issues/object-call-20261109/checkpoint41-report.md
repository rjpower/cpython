# Candidate41: integration and performance checkpoint

**Hold performance promotion.** Candidate `0a35a382909` passes focused native
checks and independent source review, and removes substantial lookup/keyword
work. Across29 application kernels it uses 1.01024 times frozen39 elapsed time
and 1.18105 times upstream. Logging remains faster than upstream in this screen
(0.94493), but is 5.44% slower than frozen39. Coroutines regress 6.44% and DeltaBlue
3.89% against frozen39 in both initial balanced pairs. A six-round repeat and
compiled-code diagnosis are pending. These losses are not hidden in the mean.

The [tables](checkpoint41-tables.md) contain every kernel and longer micro
confirmation. The [active plan](plan.md) and [status](../../STATUS.md) own ongoing
work. Golden remains runtime39; no new golden promotion has occurred.

## Implemented and verified

- Feedback stores canonical class values as guarded metadata. Only a validated
  reader creates a transient borrow and acquires its result. Results are
  published before receiver retirement. Inline class scalars use the existing
  non-descriptor paths; descriptor, MRO, shadow and layout guards remain.
- Fixed one-to-three-keyword destination plans reuse the six-byte call payload,
  independent of function arity. Current name/default coverage guards precede
  ownership transfer. Bounded keyword owners move through C storage while the
  positional prefix reuses the overlapping frame. The return ABI is unchanged.
- Both positional and keyword plans validate current default coverage. Different
  functions created from the same code may share a version but have different
  defaults; cached offsets alone were unsound.
- Generator declaration mutability follows the existing checked publication
  contract, replacing another prospective opcode-name exception.

Release/debug/stats builds check 117 modules. Corrected focused release/debug
runs pass 464 tests each; stats smoke passes 150. Final release generator/module
checks also pass all 526 actual tests in 11 modules. Five admitted keyword shapes
record exactly1,000 fast hits each; four fallback controls record zero. The
standalone lifetime witness passes both receiver layouts on release/debug. It
crashes both frozen39 and upstream, so this is a shared lifetime hazard rather
than an introduced fork regression or an exact-finalizer-timing requirement.
Astra approved proposals, actual code and the positional repair. Full diagnostics
and the API check have not been rerun for this candidate; the prior checkpoint's
known tooling/frame-jump failures are not a green-suite claim for this image.

## What the timings show

Long class-read confirmation is 0.16914 times frozen39, about 5.9 times faster;
keyword calls are 0.63626, about1.57 times faster. Against upstream they are 1.02406
and 1.12412. Class results vary enough that parity is unproved; keyword calls are
consistently slower. These are elapsed results, distinct from historical raw or
skeleton-subtracted instruction ratios. Their application share is not implied.

Static disassembly identifies a plausible logging regression: GCC now outlines
`_PyFrame_PushOverlapping` at all three call sites, adding a call, six saved
registers and stack storage. Frozen39 inlines it. The shared call helper's kind
is constant-folded; this is not evidence of another runtime dispatcher. Dominant
coroutine recursion does not use that overlap route, and inspected unchanged
handlers do not show added work there. Do not claim outlining explains every
regression. See [triage and bounded proposal](regression-triage41.md).

The next experiment preserves release inlining at the shared frame primitive
using the project's existing policy. It must retain one ownership ABI, survive
actual-code review, and improve paired application results before admission.
Keep current41 counters/profiles and matching executables before changing it.

## Next design work

Keep initialized reusable homes, overlap and real result continuations. The
next generator step is a measured whole-cycle census and explicit resume
contract with generation-time producer policies. Adding an EMPTY_TEMP tag is
not approved: update/decode costs may cancel its saved owner load and it cannot
help the FOR-only resume micro. The independently reviewed cell-getset pair is a
small value-native boundary port using an existing ABI. Broader structured IR
effects should derive transfer/publication/reload facts once. A wholesale object
layout rewrite lacks evidence; guarded hash-entry coverage remains a bounded
experiment after hot lookup misses are characterized.

## Evidence and rejected runs

Local evidence is under `BASE/pyjit-link-wave1/object-call-20261109`, where BASE is
recorded in HANDOFF. `measure41-all29-{golden,upstream}` and four
`measure41-*-long` campaigns have stable pre/post identities, no capture errors,
fixed affinity/hash/JIT state and balanced order. `object41-*` logs,
`keyword-hits41.json` and `lookup-lifetime-after41.json` retain verification.
The [initial audit](performance-audit41.md) independently checks the short
campaign's identities and raw summaries; long/full audit remains pending.

The first final-focus command included two nonexistent test-module names: all
actual tests passed but the run failed overall. Corrected module selection
passes both builds. One upstream micro preflight rejected a wrong module-log
path before measuring. The first basic timing campaign rejects its own changed
helper hash; its values are diagnostic only. All failed/rejected logs remain;
none support the reported application or long-interval results.
