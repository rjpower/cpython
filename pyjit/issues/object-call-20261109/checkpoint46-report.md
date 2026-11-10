# Object/call checkpoint: implementation, performance and next work

**Keep golden39 unchanged; hold performance promotion.** Reviewed candidate46
`b8e7e1bee6611eeda320ce163cd3ce89323da877` has large local improvements and
preserves logging parity, but repeated application regressions remain. Across
29 unchanged kernels it takes **1.00382× frozen39 time and 1.18120× upstream**.
The [complete tables](checkpoint46-tables.md), [independent review](final-review46.md),
[active plan](plan.md) and [status](../../STATUS.md) separate current outcomes
from proposals. No native method/tracing JIT tier is enabled or complete.

## What is integrated

Universal eight-byte PyValues already cover interpreter homes, containers,
attributes and rebuilt extensions. The remaining work is execution cost and
boundary quality, rather than finishing a second frame-only representation.
This round adds:

- Canonical guarded class-value feedback for admitted non-descriptor loads.
  Acquire the result before receiver retirement can invalidate its protecting
  class. Existing layout, shadow, MRO, descriptor and custom-hook rules remain.
- Reusable plans for one to three fixed keywords, independent of function arity.
  The positional prefix overlaps the child frame; only bounded keyword owners
  move through C locals. Defaults, recipient and continuation use the same ABI.
- Current-default-shape checks for functions sharing code/version but carrying
  different defaults. Cached offsets alone were unsound. These guards remain
  necessary even if immutable plan checks later move out of the hit path.
- The existing release inline policy at the shared overlap/frame-initialization
  primitives. This removes newly outlined unconditional construction boundaries;
  conditional constant-slot filling can remain a real helper call. No universal
  runtime dispatcher or second binder was introduced.
- Independent guarded cache fields, keeping the existing value/union checker
  strict. Attribute feedback grows 40→48 bytes, snapshot payloads 8→16 and poly
  records 88→120. Magic 64417 versions serialized feedback geometry. The eight-byte
  IR instruction encoding and register/window calling contract are unchanged.

Sol agents implemented and reviewed the lookup/call lanes. Astra independently
reviewed proposals, actual changes and evidence. The coordinator integrated,
rebuilt, qualified and measured the merged runtime. Earlier 43 was rejected by
the API gate; 44 exposed missing serialized-format versioning; 45's full run found
one stale disassembly fixture. Those issues were repaired without checker waivers
or deleting caches. Their failed records remain separate from accepted evidence.

## Performance

Ratios below are elapsed candidate/control. Lower is better. Full29 uses two
balanced pairs per kernel; micros use six longer pairs. Separate campaigns are
not multiplied or treated as an annotation/layout ablation.

| Workload | /frozen39 | /upstream |
| --- | ---: | ---: |
| All29 geometric mean | 1.00382 | 1.18120 |
| Logging silent, full29 | 0.96472 | 0.88807 |
| Coroutines, full29 | 1.05458 | 1.34524 |
| DeltaBlue, full29 | 0.99291 | 1.63233 |
| Class-read micro | 0.16013 | 1.00434 |
| Keyword-call micro | 0.60957 | 1.05220 |

Class reads are about 6.25× faster than39; keywords about 1.64×. The class/upstream
mean is near parity but pair ratios range 0.9698–1.0804; this is not tight parity
evidence. The keyword micro remains consistently above upstream. These shapes
do not measure their application frequency.

Six-pair confirmations against39 show coroutines **+5.78%**, spectral norm
**+8.11%**, and comprehensions **+9.48%**, with every pair slower. Logging silent
improves 2.68% and DeltaBlue 1.42%, with every pair faster. Generators/nbody are
near baseline with mixed pairs; logging simple is variable. The near-flat
aggregate does not waive the three repeated losses.

### Where the upstream gap remains

Current hot-interval PMU medians, candidate/upstream:

| Case | Instructions | Cycles | CPI |
| --- | ---: | ---: | ---: |
| Logging silent | 1.03636 | 0.87315 | 0.84252 |
| Coroutines | 1.38388 | 1.33412 | 0.96404 |
| DeltaBlue | 1.87058 | 1.65718 | 0.88592 |

Thus coroutines and DeltaBlue still do substantially more executed work per
unchanged Python workload. Raw micro instruction ratios are 1.16753/class and
1.50180/keyword versus upstream, despite smaller elapsed gaps. Skeleton-subtracted
micros and hot-interval kernel counters have different boundaries; do not pool
them or compare unlike VM instructions as a uniform per-bytecode tax.

Matched current/golden hot-interval counters now narrow the three regressions:

| Case,46/39 | Instructions | Cycles | CPI |
| --- | ---: | ---: | ---: |
| Coroutines | 0.99940 | 1.05625 | 1.05688 |
| Spectral norm | 1.00055 | 1.05377 | 1.05319 |
| Comprehensions | 1.00068 | 1.09562 | 1.09487 |

These show higher aggregate CPI for nearly unchanged executed work. Branch
counts are effectively unchanged. Miss rates 39→46 are 0.37578%→0.37579%,
0.00481%→0.00482%, and 0.29997%→0.30377%, respectively; their observed ranges
overlap. The packet does not show extra branch misses explaining the loss.
Instruction/data locality, front-end layout and dependency/scheduling effects
remain hypotheses. A same-GCC component/compiled-path experiment should isolate
them; padding or broader inline directives would not establish a repair.

Both event-group campaigns pass 18/18 intervals with 100% running coverage,
matching source/executable/input identities and FIFO/native checks. Each uses
three rounds, with 2/1 role ordering; these are diagnostic per-hot-loop medians,
separate from the balanced elapsed experiments.

The [current profile analysis](current-profile46-analysis.md) audits six retained
recordings with zero lost samples. Coroutine samples locate exact Python calls,
RETURN_GENERATOR allocation/copy and return/completion/clear work; no yield
handler is sampled. This kernel creates/awaits recursive coroutines without
suspending, so a SEND-only delivery change misses much of its cost. DeltaBlue
spreads work over method/shape/generic/type-cache paths and exact calls. Within-
record sample shares locate work; they are not removable fractions of the gap.
Inline DWARF ancestry also does not prove a physical helper call or nonempty
constant-slot filling.

## Qualification and cache footprint

Release/debug/stats builds identify46 and check 117 modules each. Candidate45's
focused checks pass 564 tests per release/debug build. Both full45 diagnostics
execute 54,674 tests. They retain known GDB display failures 16/21, nine frame-jump
errors in each, and one debug trace-format failure. Both also found eight stale
disassembly expectations; the fixture-only46 successor passes the complete 143-test
module on both builds. Stats46 smoke passes 150 tests. The API45 run covers 306
units with zero failing findings/parse errors;46 changes no production C/checker
input. This is explicit reuse of that result, not a newly run full46 suite or an
all-green qualification. Optional tooling transcripts do not define Python behavior.

The [storage/format review](post-review44.md) and [qualification/census audit](current-review46.md)
verify the implementation. The new 700-site test proves 682 private attr entries
and 18 shared fallbacks at the 4096-unit limit; mixed entry layouts have their own
capacity. Actual scoped admission stayed private in all four inspected samples:

| Scope | Code objects | Private attr-family sites | Warm allocated entry bytes39→46 |
| --- | ---: | ---: | ---: |
| Class load | 1 | 1 | 344→352 |
| Logging, including stdlib definitions | 165 | 633 | 12,144→13,104 |
| DeltaBlue | 67 | 328 | 24,200→26,664 |
| Coroutines | 3 | 1 | 720→728 |

These are bounded code/handler snapshots, including dormant functions, not a
dynamic execution census. They exclude poly tables, instruction words, headers
and allocator overhead. All scoped source/identity pairs match and inspection
does not allocate feedback. Inputs are bracketed across inspection/warmup, not
the preceding module import/setup. No private-site loss here means the synthetic
limit is not reached here; it does not remove that limit elsewhere.

The class micro moves from generic LOAD_ATTR to the intended non-descriptor
handler. Application handler histograms in the other three scoped samples match.
This reinforces the need to find weighted lookup sites rather than extrapolate
one class-scalar micro gain to DeltaBlue. The lifetime witness that crashes both
frozen39 and upstream passes the corrected lookup implementation in41; focused
and full successor cache tests retain that retirement contract. Golden is a
frozen engineering control with known limits, not a claim that witness is repaired.

## Next design and workstreams

1. Isolate/remove the confirmed CPI regressions before promotion using one GCC,
   bounded component experiments and actual hot-path assembly. Keep correctness
   fixes separable. Root owns counters, integration and promotion.
2. Sol call lane: implement the approved [keyword invariant proposal](keyword-plan-invariants-proposal.md)
   and its [review gates](keyword-plan-invariants-review.md), moving only proved immutable destination checks to
   construction/debug assertions. Retain mutable name/default/provider guards.
   Require lower total instructions and unchanged supported/fallback behavior.
3. Sol design lane: census eligible exact generator/coroutine creation and prove
   direct construction in the final owner. This targets temporary thread-frame
   setup and immediate copying in the actual coroutine route. The
   [minimal scope](entry-scope-calibration.md) supersedes broader drafts: three
   outcomes and ordinary publication semantics where preparation cannot callback.
   Shell/GC, allocator/arena, pinned-code, prologue and continuation proofs precede
   implementation. A universal native publisher is not a prerequisite.
4. Sol lookup lane: weight DeltaBlue's actual shape/method/type/generic routes
   before adding guards or hash-entry hints. A wholesale layout rewrite is not
   supported. Names are already interned and shared-key/shape machinery exists.
5. Derive ownership/publication/reload facts once for IR lowering, generation and
   verification. Start with equivalent generated output for one proof. Terra/Luna
   can port bounded cell getter/setter pairs once measured exposure justifies it.

Keep initialized reusable storage, overlapping windows, explicit recipients and
real continuations. A register IR does not require duplicate frame construction
or repeated immutable-plan checks. Broader entry, object-layout and JIT designs
remain separate from implemented performance claims. Python's data model and
the selected ownership contract govern correctness, not exact CPython release
schedules or debugger formatting.

## Evidence and recovery

Runtime paths and immutable controls are in [HANDOFF](../../HANDOFF.md). Evidence
root is `BASE/pyjit-link-wave1/object-call-20261109`:

- `measure46-all29-{golden,upstream}`, `measure46-regressions-golden39` and four
  `measure46-*-long` campaigns: 376 timed invocations independently reconstructed.
- `object45-*` full/API/focused records; `object45b-*` builds; `object46-*` final
  builds/stats/disassembly; `coverage46-*` bounded feedback census.
- `instructions46-micro-v2`: accepted three-role micro counts. Initial
  `instructions46-micro` rejects load 3.01 above its 3.0 guard and yields no accepted
  report. Its stopped pipeline record remains. Later captures use a new journal.
- `gap46-perf-{stat,record}`: current upstream comparison and matching retained
  executables. `gap46-golden-{cycles-instructions,branches}`: matched control
  diagnostics for the three regressing cases, subject to their own status gates.

Source/build/input identities stay frozen during measurements. No rejected
capture contributes a result. [Canonical designs](../../README.md) and Loom
mirrors are updated together; historical41/43 records retain their own identities.

The [candidate branch](https://github.com/rjpower/cpython/tree/pyjit/frame-call-model-20261109)
is pushed at46. [Remote readback](publication46.json) verifies the candidate and
unchanged golden branch/tag; publishing this candidate does not promote it.
