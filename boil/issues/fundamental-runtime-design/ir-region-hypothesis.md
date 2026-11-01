# Provisional region: compact-integer comparison followed by a forward branch

2026-11-01. **Design only, held for a separately reviewed workload census.**
The retained evidence does not establish a registered-workload hotness ranking.
No source implementation, compiler, target, census, timing or runtime option is
authorized by this document. VM0 helper preparation remains suspended.

The hypothesis is that one guarded, two-word region can execute a compact-int
comparison and consume its boolean branch result without a temporary-home
round trip or a second dispatch. Keep the existing words, frame layout, root
machinery, both IP publications and ordinary branch entry. Do not introduce a
general SSA pass, new instruction encoding or native interpreter ABI here.

## Selection evidence and the missing evidence

The host receipt
`build/fundamental-runtime-review-20261101/region-evidence.json` binds all
inputs below. The maintained runtime files are unchanged from the source
inspected in the [fundamental IR review](../../../build/fundamental-runtime-review-20261101/ir.md).
Future source work must bind its own exact base; this document grants none.

| Plausible short sequence | Actual retained execution evidence | Proof cost |
| --- | --- | --- |
| Compare → conditional branch | The H3 raw probe records `COMPARE_OP→POP_JUMP_IF_TRUE` three times, coverage version2 / computed dispatch mode0. Its source and successful invocation are retained. | A compact-int fast path can be nonallocating and nonreentrant; branch metadata and legal second-word entry must survive. |
| Compact-int add → return | The same raw probe records `BINARY_OP_ADD_INT→RETURN_VALUE` once, executing its exact `add(4,5)` consumer. | Addition can allocate on overflow; return switches frames, publishes a result into the caller and clears/unlinks the source. Much larger exceptional/root/observer boundary. |

This raw probe is **synthetic**, from source
`e9a1bcc65cf50c9316fc375500bcdaf4fb3a117b`. It does not select a benchmark or
prove its hotness. Its raw file hash is
`f9f6497d8d8d2c3a5adc2edf2a8b4a86306248fd77f238823f156347af78d990`.
In particular, its thousands of `LOAD_ATTR→LOAD_ATTR_NONDESCRIPTOR_SHAPE`
pairs are same-word adaptive redispatches, not evidence of adjacent operations
that a compiler could fuse.

The separate saved `coroutines.fibonacci` code from the **rejected constant
operand candidate** contains compare/false-branch at offsets 4/5 and add/return
at 30/31. The capture explicitly ran **zero workload calls**. Its comparison
destination and branch input both name temporary1; these are actual stored
words, not an executed-site count. They nominate a consumer to inspect, not
registered-workload selection evidence. Historical aggregate `dispatches` and
earlier stack-stage TO_BOOL counts do not fill that gap.

Choose compare/branch **provisionally for its smaller proof**, not because
3 versus1 ranks it as faster or hotter. Do not reopen call setup, frame-copy,
constant-operand or return-path experiments under this hypothesis.

Before implementation or timing, separately authorize at most one small
census on an already bound, unchanged computed-dispatch Py_STATS image:

1. Use registered `coroutines` with its unchanged loader/setup and loops 4.
   Perform one registered warmup, then one counter-only `module.run(4)` with
   stats clear/on/off around that call. No timing samples, hardware counters,
   new workload, Python tracing callback or hot-loop observer.
2. Outside counting, retain pristine and executing `get_ir` snapshots for the
   actual `fibonacci` code object, code/kernel/image/config/CAPI origins and
   feedback/line/exception metadata. Preserve actual coverage version and
   dispatch mode, complete raw stats and source-derived pair identities.
3. Compare actual `COMPARE_OP_INT→POP_JUMP_IF_FALSE_BOOL` and
   `BINARY_OP_ADD_INT→RETURN_VALUE` entries. Establish whether the first pair
   corresponds to the nominated eligible site in this closed kernel call graph;
   explicitly distinguish any other contributing code. Aggregate pairs alone
   cannot nominate a particular PC when multiple sites contribute.

Propose one 10-minute inclusive census window, using the accepted lock,
origin, raw-output, timeout and cleanup protocol with no retry or clock reset.
This is a minimal evidence request, not a census implementation or execution
grant. Existing `get_ir(..., True)` exposes the executing copy
(`Modules/_testinternalcapi.c:1591–1627`); it does not provide per-PC counts.
Positive counts plus an unambiguous static site are sufficient to consider the
mechanism; no arbitrary frequency percentage predicts elapsed benefit. If the
site is absent, does not specialize, cannot be attributed, or an exact existing
image cannot be bound, stop at **insufficient evidence**. Do not silently build
a new image or replace the consumer. Broader census work needs a new scope.

## Exact transformation, if the census justifies it

Use two original words, not a newly packed comparison/target/feedback format:

```text
h: COMPARE_OP             A=temporary result, B=left, C=right, D=compare feedback
b: POP_JUMP_IF_FALSE      A=h.result, B=branch feedback, CD=forward target
```

The following branch retains its existing target and feedback entry. This
avoids pretending two inputs, a result, a24-bit target and feedback fit in the
comparison's four fields. Word offsets, line/exception tables and direct
branch entry remain intact.

After existing register rewrites, before compaction, a narrowly scoped region
nomination in `regcfg_passes.c` admits only the following pair:

- Consecutive operations in one block, same source line and exception context;
  no intervening event, NOP, predecessor boundary or instruction.
- `COMPARE_OP` requests a boolean result (oparg bit16); the next operation is
  `POP_JUMP_IF_FALSE` with a forward target. No backwards branch, OR_POP form,
  None branch, window, call, yield, return or other opcode is admitted.
- The boolean is a fresh temporary, not a local or canonical live-out. It has
  exactly one read, consumed by that next branch; no implicit exception use,
  additional reader or folded STORE destination. Preserve both branch edges.
- After compaction, independently verify the branch's input field equals the
  comparison's destination and that it is a temporary. Keep all input alias
  and borrowed-local rules already enforced by the verifier.

Nomination needs a generic comparison-region head and one compact-int region
specialization, both with the comparison's existing field format and semantic
base opcode. The following word stays an ordinary independently executable
branch. Assembly chooses the head only for a verified pair; no field or
sidecar target is invented. Generic execution reuses the existing comparison
semantics, materializes the result normally and dispatches to the branch.
Specialization/deoptimization and instrumentation restoration must preserve
the head's form. The generated metadata remains the authority for disassembly,
validation, base-op identity, feedback and wrapper restoration; all affected
readers must be updated. A hand-built malformed pair must be rejected by code
validation, and runtime lookahead must be bounded even for crafted input.
One concrete narrowing pitfall is `_Py_Specialize_CompareOp`'s current
`uint8_t specialized_op` (`specialize.c:2620`): an IR-only handler above 255
cannot pass through it. Use the existing full handler type at that selector and
audit the generated metadata/installer consumers; do not truncate the new ID
or reserve an unrelated opcode by hand.

This is a deliberate two-word region mechanism. It is not an encoding-neutral
peephole, and its generator cost must be reviewed before source authorization.
If it needs a general new optimizer or duplicate operator semantics, reject
this bounded design instead of expanding it.

## Facts, effects and conservative exits

The compiler proves adjacency and value use, **not operand types**. At runtime
reuse the existing compact-int guards and `_COMPARE_OP_INT` comparison-bit
calculation (`bytecodes.c:3056–3090`). Reject bool/int subclasses, noncompact
integers, floats and arbitrary objects from this fast path. Input fields are
read before any possible Python execution, as existing borrowed-local rules
require. This exact path performs no allocation, conversion, Python call,
frame switch, collection or potentially failing operation.

Fast execution also requires the executing second word to be exactly
`POP_JUMP_IF_FALSE_BOOL`. This preserves ordinary branch warmup: while the
second word is generic/adaptive or has another specialization, execute the
ordinary two-word path. The fast region records the existing branch history
at the second word's feedback entry, including the existing NOT_TAKEN skip;
it does not reproduce a new history or adaptive-counter algorithm. A change
to the second word invalidates fast admission immediately.

Reuse the current instrumentation/version authority and deny fast execution
with pending eval-breaker work, active tracing/monitoring, or a transparent
instrumentation wrapper at either word. A conservative initial policy may
deny fusion whenever the code has monitoring storage. Such a guard must be
explicit and tested; "same source line" alone is insufficient. Enabling then
disabling monitoring may therefore leave the ordinary path active without
affecting correctness. There is no new user flag or permanent nursery/VM mode.

Any type, pair, epoch, instrumentation or metadata guard failure enters the
original comparison at `h` with inputs and homes untouched, then reaches the
ordinary branch. Generic rich comparison and truth conversion retain their
callbacks, reflected behavior, exceptions and allocation/OOM paths. The new
fast path does not cover or hoist them. Do not consume an input before all
admission checks finish; slow-path recovery must not reconstruct a value from
already-cleared storage.

The generator should compose the two existing logical operations and their
`IRStack`/`SlotLayout` home effects. The temporary boolean can live in a C
value between them; input consumption and final NULL homes use existing
operations. It must not declare a false one-word stack effect or hide a
second hand-written unwind implementation. This requires a small explicit
region emitter and tests for its two logical instruction states; it does not
require SSA, dominators or new GC root maps.
Counter-enabled builds count the one physical region entry honestly; they
must not synthesize a second handler-entry counter for the skipped dispatch.
Branch history and comparison-hit accounting still occur. This distinction
needs source and raw-counter tests before using stats as a work-removal witness.

| State / entry | Required behavior |
| --- | --- |
| Before `h` | Original operand homes and exception state; publish IP=`h` as today. |
| Between operations on fast path | Computed immortal boolean is a C value; comparison operands retain homes until consumed. No call, collection, observer callback or service point is permitted here. |
| Logical `b` fast step | Preserve IP=`b` publication; consume inputs with existing local/temporary distinction, record branch history and take the original target/fallthrough. Result temporary ends NULL. |
| Direct entry at `b` | Execute the ordinary branch using the actual current boolean/value home; never assume the comparison ran. This covers debugger redirection and other legal entry. |
| Guard failure or generic exception | Original materialized execution and original error word/state. No fused error path exists. |
| Monitoring/line/instruction/branch event | Original two words and their original observable states execute; no event is synthesized from a stale C value. |

Keep both original IP stores in this first hypothesis. Removing either needs
a separate publication/async-observer proof and is not part of the proposed
benefit. Existing asynchronous stack readers already observe ordinary
handler update sequences; this work grants no stronger atomic-frame guarantee
and may not weaken their current contract.

The maximum region is exactly two forward words. It crosses no backedge or
existing breaker service. Pending work denies fast admission; work arriving
during these nonreentrant steps is serviced at the same next existing service
point. Do not remove, postpone or combine RESUME/backedge checks. A bounded
sequence length alone would not justify moving a check which actually exists.

## Required controls and acceptance evidence

Semantic negatives must include: both branch outcomes and NOT_TAKEN layout;
every supported comparison relation; inline and boxed compact ints and their
boundary values; equal/aliased inputs; borrowed locals and temporary operands;
noncompact integers, bools, subclasses, floats/NaNs and custom rich comparison
returning an object whose truth conversion mutates or raises. Their fallback
must retain original call/error order. Reject nomination for a second use,
stored-local result, exception-live result, changed line/context, window or
backward target. Include malformed/truncated pairs and altered branch fields.

Exercise actual instruction and line callbacks which change a local, activate
monitoring between calls, redirect `f_lineno` to the branch, and raise during
comparison/truth handling. Direct branch entry must work without any prior
comparison. Test restored specialization after observation, both branch
feedback histories, and branch counter warmup. Run the existing root/stale,
exception/OOM, frame and generator suites; no blanket checker/stress skips.
Generated-source and linked audits must remain strict, with no new opaque
classification exemption.

A temporary **source-only ablation branch**, not a runtime flag, must use the
same fused region admission, comparison, branch and dispatch but force the
boolean home write/read/clear through a volatile-qualified view of that
one temporary. This prevents C from silently optimizing away the intervention.
It performs no callback or collection and adds no compiler-wide memory fence.
Its source and actual emitted path must prove those memory operations remain;
otherwise it earns no materialization-control credit. Volatile access can also
alter surrounding native code, so its elapsed delta is an intervention result,
not a cycle-perfect decomposition. Preserve that limit.

Require native complete-path evidence that the candidate removes the named
boolean round trip and second dispatch while preserving both IP publications,
guards, slow exits and branch recording. Compare parent / materializing-region
ablation / region candidate under one exact source/config/toolchain bridge.
Do not substitute static source-store counts for emitted work or speed.

If the census supports `fibonacci`, coroutines remains the predeclared primary.
The eventual elapsed campaign should include **all 29 registered kernels at
their existing loop counts**, not just a synthetic compare loop. Freeze the
registry, literal order and exact setup/origins before scores. For three arms,
two rotating/reversed rounds and five serial calls per fresh cell, this is
174 scored processes /870 durations, with one registered warmup per scored
process. Add 87 separate one-loop zero-sample origin processes, for 261 total.
Do not confuse those origin warmups with registered-loop coverage. All raw
vectors, including outliers, remain; no appended samples or automatic retry.

Use the accepted direct elapsed driver and one 45-minute exclusive quiet window:
CPU 0 with sibling 8 quiet, both machine slots and all three image locks, load
ceiling 1.5, OFF/N0, seed 0, unchanged GC/setup policy and no new counter probe.
Record peak RSS, actual frame/code/feedback bytes and matched source/product
origins outside the clock; diagnostics cannot veto or rescue elapsed results.
Primary candidate/parent medians must improve at least 1% in both rounds, exceed
summed within-cell MADs and retain all ten leave-one-out ratios below1. These
are descriptive adequacy checks, not significance claims. Report every other
kernel; repeated >2% regressions require a bounded resolution before acceptance.
Report candidate/ablation contrasts without inventing a separate percentage
gate for the storage contribution. A primary failure rejects the increment.

Measure compiler cost separately with the existing frozen Lib corpus procedure:
three fresh compile processes per arm in rotating order, no pyc reuse, report
elapsed, peak RSS and code/frame/feedback totals for every run. This is a
compile workload, never mixed with the kernel clock or an already-run cached
import. Reject unsupported complexity or regressions on their actual costs,
not an arbitrary instruction/text-size cap.

All these are proposed future scopes. Source work, independent review, fresh
correctness feedback, the three-arm quiet campaign, compile-cost comparison
and the mandatory qualification cohort cannot be assumed to fit the remaining
04:18:11 UTC integration boundary. A coordinator schedule must demonstrate
that fit or explicitly defer this region experiment. Do not reset existing
deadlines, resume paused helpers or qualify two architecture alternatives.

An eventual source grant should start with a 45-minute host prototype ceiling,
with 15/30-minute checkpoints and no native work: prove the region emitter,
metadata restoration and negative admission tests first. If those do not fit,
report the concrete missing work rather than extending the grant. Independent
source review, build/behavior feedback and timing need their own later grants.

**Current disposition:** useful provisional mechanism design; insufficient
registered execution evidence to select it for implementation. The next
decision is the small separately authorized census, not a runtime patch.
