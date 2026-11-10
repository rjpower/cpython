# Project status

The active round targets **DeltaBlue ordinary call setup and teardown**, using
committed candidate46 `b8e7e1bee66` as its development baseline. The first
[integration](issues/deltablue-call-model-20261110/integration47.md), runtime47
`986c65ee5ab`, is committed and pushed: unified guarded class-value reads.
Release/debug pass571 focused tests each; API306 units has zero failing findings
or parse errors. A short two-pair screen gives DeltaBlue1.26303/upstream and
logging0.91755/upstream; logging is5.53% slower than the rebuilt46 control.
Full qualification and repeated target timings belong at the batch boundary.
See the
[new plan](issues/deltablue-call-model-20261110/plan.md) and
[grouped design](issues/deltablue-call-model-20261110/design-overview.md). One broad
Astra review is complete; it selected uniform canonical class reads, classmethod
pair forwarding and complete conditional leaf-region elision. Sol approved the
first lookup proposal and post-reviewed its implementation. Classmethod forwarding
is being implemented; the conditional leaf prototype is under component review.

The user permits useful incremental progress with documented tradeoffs. The
following table records the earlier46 checkpoint, not the newer47 screen. Earlier46
was held under the previous performance policy; its
[report](issues/object-call-20261109/checkpoint46-report.md) and
[tables](issues/object-call-20261109/checkpoint46-tables.md) remain historical
results, not a requirement to finish CPI investigations before improving calls.
Golden39 remains an immutable comparison control.

| Elapsed candidate/control | /frozen39 | /upstream |
| --- | ---: | ---: |
| All29 geometric mean | 1.00382 | 1.18120 |
| Logging silent, full29 | 0.96472 | 0.88807 |
| Coroutines, full29 | 1.05458 | 1.34524 |
| DeltaBlue, full29 | 0.99291 | 1.63233 |
| Long class-read micro | 0.16013 | 1.00434 |
| Long keyword-call micro | 0.60957 | 1.05220 |

Full29 uses two balanced pairs per kernel; micros six longer pairs. Class is
near upstream in aggregate with substantial pair variation. Six-pair golden
confirmations show coroutines +5.78%, spectral norm +8.11%, comprehensions
+9.48%, all pairs slower. Logging improves 2.68% and DeltaBlue 1.42% in every
confirmation pair. These repeated losses remain reported under the revised
incremental landing policy; they do not block structural call work.

Matched current/golden PMU shows nearly equal instructions and branches but
5–10% more cycles in the three regressions. Aggregate CPI is higher; branch-miss
rates barely change. The exact locality/layout/dependency mechanism is open.
Against upstream, coroutine/DeltaBlue instruction ratios remain 1.384/1.871.
Profiles locate exact calls, coroutine creation/copy/completion and several
lookup routes. The [cold review](issues/object-call-20261109/final-review46.md)
separates measured cost from hypotheses.

The candidate integrates canonical guarded class-value feedback, safe result
acquisition, fixed 1–3-keyword plans over the existing overlap ABI, current-default
shape guards and bounded shared-frame inlining. Guarded words now occupy separate
fields; the API checker stays strict. Attr/poly sizes are 48/120 bytes and magic
64417 versions serialized feedback geometry. Four scoped censuses retain private
site admission; warm entry storage grows about 8% in logging and 10% in DeltaBlue.

Release/debug/stats builds identify46 and check 117 modules. The production45
checkpoint passed 564 focused tests per release/debug build, clean API checks
(306 units, zero failing findings/parse errors), and full diagnostics of 54,674
tests each. Full runs retain known GDB display failures16/21, nine frame-jump
errors each, one debug trace-format failure, plus eight stale disassembly checks.
The fixture-only46 correction passes all 143 disassembly tests on both builds;
stats46 smoke passes 150. Production C/checker inputs are unchanged from45.
These are failed full diagnostic records plus a verified fixture repair, not an
all-green or newly executed full46 suite.

Golden39 **4d666e1b482** is published as `pyjit-golden-20261109-runtime`; combined
snapshot is `cb906f9bc52`. Both generations of immutable controls remain unchanged.
The [receipt](issues/golden-20261109/publication.json) and
[baseline closing report](issues/frame-call-model-20261109/closing-report.md)
retain known limits, including the shared lookup lifetime witness discussed in
the current report. No candidate performance result replaces those controls.

Universal PyValues are integrated across frames, containers, attributes and
rebuilt extensions. Native method/tracing JIT tiers remain unfinished. Keep
initialized reusable storage, overlap, explicit recipients and real continuations.
Next: canonical class-value reads, classmethod target forwarding and a reviewed
complete leaf-region certificate for conditional accessors. A wholesale object-layout rewrite is not justified. See
[IR](design/register-ir.md), [interpreter](design/interpreter.md),
[frames/calls](design/call-and-frame-lifetimes.md), [values](design/pyvalue.md)
and [handoff](HANDOFF.md).
