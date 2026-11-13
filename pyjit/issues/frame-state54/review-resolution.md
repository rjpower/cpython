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
  escaped frame. Its scope and recurring updates reject it as this round's first
  implementation; the user's idea is assessed, not silently substituted.
- The interpreter-exit trampoline has an execution prefix with NULL entries;
  the thread base sentinel has no words. Feedback derivation keeps those
  distinct contracts without a new guard on every normal access.
- Function readonly environment attributes are insufficient proof by themselves.
  GC teardown, frame.clear specials and reentrant inspection must preserve the
  owner graph; safe-dump heuristics cannot decode an unvalidated function.
- Locals storage may not allocate inside pure frame initialization. Mapping
  preparation stays fallible before commit; teardown holds the mapping across
  unique frame-object destruction and slot retirement.
- Static native inspection found the same-home ADD input NULL store survives;
  integer input close machinery is already optimized away. Count only work
  actually removed. Make alias equality visible in emitted C; a release-elided
  assertion is not compiler authority. Weighted reached sites remain a gate.

## Integration decision

Use unchanged hot workloads, then all29 for promising lanes. Compose only
qualified winners or a bounded near-neutral field-layout combination whose
owner proofs are complete. Expected F+N+L header geometry is 80 bytes, but added
loads/allocation do not disappear at that size. Measure the composition directly.
Retain semantic, native and performance rejection evidence. No automatic golden
promotion follows a source cleanup, smaller header or individual microbenchmark.
