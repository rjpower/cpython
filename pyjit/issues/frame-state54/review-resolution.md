# Round54 review resolution

One broad [Astra review](astra-review.md) and independent
[field](field-inventory.md) / [validity](validity-inventory.md) inventories are
complete. Root accepts five isolated experiments, not five predicted wins.
Each starts from immutable golden51. Sol peer review resolves the concrete
interfaces before handwritten implementation; root owns generation and native
validation. Astra is not in the implementation loop.

| Experiment | Contract | Common-path tradeoff | Owner / source suffix |
| --- | --- | --- | --- |
| 1: recipient | Encode replace-required versus proved-empty temporary in existing uint16 recipient; redirects commit before descriptor reload | Producer classification and publication branch versus removed old-owner load/retirement | validity worker / frame54-recipient |
| 2: feedback | Derive entries through the execution prefix before retained ir_words; remove cached pointer | One dependent feedback load versus one less header word and setup/copy work | field worker / frame54-feedback |
| 3: namespaces | Derive globals/builtins through independently retained function; retain executing-code owner | Extra function/member dependency versus two less header words | field worker / frame54-globals |
| 4: locals | Store strong mapping on optional frame object; prepare non-NULL mapping entries before nonfallible commit | Eager class/exec frame object and mapping indirection versus one less ubiquitous header word | locals worker / frame54-locals |
| 5: same-home commit | Execution-only integer ADD variant proves temporary A==B and C!=B, skips intermediate B clear only after successful result production | Variant selection/text versus actual surviving NULL store and generic destination work | validity worker / frame54-clears |

## Corrections incorporated

- Warm arena initialization already uses an initialized frontier. Ordinary
  thread return clears locals, not every scratch slot. Bitmap benefits cannot
  include removing work that selected51 does not perform.
- A GC root map, future-read liveness and RC occupancy are different contracts.
  A dynamic bitmap must cover every read/write, callback, overlap, unwind and
  escaped frame. It was not selected for this round's first implementation;
  this is not a measured rejection of bitmap architecture. Round55 reopens
  the complete architecture and recurring costs before implementation.
- The interpreter-exit trampoline has an execution prefix with NULL entries;
  the thread base sentinel has no words. Feedback derivation keeps those
  distinct contracts without a new guard on every normal access.
- Function readonly environment attributes are insufficient proof by themselves.
  GC teardown, frame.clear specials and reentrant inspection must preserve the
  owner graph; safe-dump heuristics cannot decode an unvalidated function.
- Locals storage may not allocate inside pure frame initialization. Mapping
  preparation stays fallible before commit; teardown holds the mapping across
  unique frame-object destruction and slot retirement.
- Source review corrects Astra's early-tracking prescription: a private
  mapping-bearing thread record must not become GC-enumerable before semantic
  frame completion. Existing root paths retain its mapping while it stays
  untracked; track on actual complete observer publication or complete heap
  transition. This correction is subject to actual source and runtime gates.
- Generator suspension metadata must follow the explicit owner-specific copy
  contract after state publication moves into that helper. Unknown owner
  expressions fail generation; actual generator/IR contract tests cover it.
- The public frame-to-generator conversion exposed a preexisting association
  assertion and invalid first-send recipient. The corrected conversion uses
  the same validated discard recipient as ordinary RETURN_GENERATOR before
  complete publication. Both original failures and corrected checks are retained.
- Static native inspection found the same-home ADD input NULL store survives;
  integer input close machinery is already optimized away. Count only work
  actually removed. Make alias equality visible in emitted C; a release-elided
  assertion is not compiler authority. Weighted reached sites remain a gate.
- Pure recipient recording must be certified nonescaping in the analyzer.
  The first generated attempt published a wrapped frame pointer as a value;
  bootstrap rejected it. The corrected classification retains pure setup.
- Debug tracing must reject entry sentinels before deriving their namespaces.
  Its helper now accepts a frame and checks its class before function access,
  preserving release handlers without a new guard.

## Integration decision

The original integration gate was to use unchanged hot workloads, then all29
for promising lanes, and compose only
qualified winners or a bounded near-neutral field-layout combination whose
owner proofs are complete. Expected F+N+L header geometry is 80 bytes, but added
loads/allocation do not disappear at that size. Measure the composition directly.
Retain semantic, native and performance rejection evidence. No automatic golden
promotion follows a source cleanup, smaller header or individual microbenchmark.


## Final resolution and selection

All nine performance candidates are rejected after their own isolated and
composition measurements. Compact F+N+L's actual header is 80 bytes, but its
screen loses; R2+S2's modest all29 aggregate does not compensate its material
coroutine regression. None is in the selected checkpoint. This is a measured
tradeoff decision, not a rule that every workload must improve.

The separate three-file cold constructor correction is independently reviewed,
qualified and selected at `61015a9452d9069646fb5fede9ff963896833b89`.
Published `pyjit/round54-golden` and annotated tag
`pyjit-golden-20261112-frame-state` resolve there. The full diagnostic suites
failed with recorded limits, including the unresolved profiling finding;
selection does not establish a green suite or performance lift. Final all29
versus golden51 is 1.006381, separate hot6 is 0.996669, and upstream all29 is
1.156543. See [qualification](qualification.md),
[admission review](checkpoint-admission-review.md) and [tables](benchmark-tables.md).

All rejected branches and verified retirement evidence are retained. Round54
is closed; [round55](../call-model55/plan.md) now researches the whole
call/frame/register architecture and bitmap alternatives before implementation.
As-built designs remain current until a reviewed proposal is implemented.
