# Merged interpreter integration review

Reviewed candidate: `cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8`, clean at inspection.
Runtime source root I: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`.
Canonical documentation root C: `/home/power/code/cpython/.worktrees/python-jit`.
Golden54: `61015a9452d9069646fb5fede9ff963896833b89`.

Disposition: **no new must-fix source finding in the reviewed composition**.
This is a source-only cold review, independent of the component implementation.
It authorizes neither selection nor a claim of complete qualification. No source
was edited; no generation, compilation, tests, profiling or timing ran here.
The coordinator owns all native evidence and integration decisions.

Read applicable AGENTS, CPython's AI policy, C's README/GOAL/STATUS/HANDOFF/
PROCESS, performance56 proposal/audit/report, and actual merged source and
relevant fixture diffs. Canonical documents still name ac4 in places; this
review deliberately uses root's supplied cb3 head. Their design describes intent,
not selection facts. Runtime sources in C were not used.

## Actual ownership and proof boundaries

- **Logical vacancy repair:** [Objects/codeobject.c:2143](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Objects/codeobject.c#L2143) introduces an
  absence predicate without modifying facts. Its consumers at 2292, 2738,
  2847, 2979 and 3337 reject required reads or omit ownerless slots. The
  normal/error phase distinction remains intact: omission does not imply
  physical NULL, initialized state or readability. MAY_OWNER and completeness
  still block certificates. `semantic_empty_scratch` also retains its INIT
  exclusion. `semantic_exact_return` does not authorize scratch reads; its
  cold shadow check can still report a dirty physical slot. The fix repairs
  inconsistent consumers of VACATE rather than making VACATE assert more.
- **Canonical list/tuple producers:**
  [Tools/cases_generator/ir_generator.py:3900](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Tools/cases_generator/ir_generator.py#L3900) checks current value identities,
  exact container and integer guards, bounds, lexical index scope and anchor.
  Misses precede ownership acquisition. Callback invalidation at 2825 clears
  container capabilities; joins at 3289 intersect them. `canonical_native`
  at 2714 follows checked representation provenance separately from ownership.
  [Python/bytecodes.c:1053](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/bytecodes.c#L1053) and 1151 use those primitives. List access retains
  the GIL restriction, and tuple's negative-index fallback remains generic.
  Checked canonical stores remove normalization, while retirement still owns
  input disposal and recipient replacement. Generated products reflect this.
- **Coroutine terminal owning result:**
  [Python/ceval_macros.h:772](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval_macros.h#L772) admits only exact coroutine objects outside the
  already-awaited suspended states, matching the identity arm of
  `_PyEval_GetAwaitable` at [Python/ceval.c:6316](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval.c#L6316). Other values and error
  states keep that helper. [Tools/cases_generator/ir_error_effects.py:759](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Tools/cases_generator/ir_error_effects.py#L759)
  uses the existing typed walk for a narrowly constrained final output; it
  does not provide normal/error home authority. At
  [Tools/cases_generator/ir_generator.py:3066](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Tools/cases_generator/ir_generator.py#L3066), publication requires its
  owning receipt; 5093 uses the ownership-only receipt separately from the
  scalar graph. Conditional/loop/opaque invalidations and join intersection
  retain conservative behavior. [Python/generated_ir_cases.c.h:15210](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/generated_ir_cases.c.h#L15210)
  confirms TAKE on the exact arm, ordinary helper/close/error on fallback,
  then existing destination publication/displacement. Executing and exhausted
  states retain later SEND errors; the ValueError fixture correction does not
  change runtime semantics.
- **Generator recipient:** `generator_entry_recipe` at
  [Tools/cases_generator/ir_generator.py:2171](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Tools/cases_generator/ir_generator.py#L2171) requires checked guard prefixes,
  one terminal intrinsic, unchanged transport roles and the legacy unreachable
  frame-switch tail. It grants recipient transport, not surrounding recovery
  authority. `enter_generator` at 5621 commits parent continuation, child link,
  exception state and active child before disposing the resume input. FOR_ITER
  uses explicit A, including local/discard, whereas SEND yields through B+2.
  This preserves the existing owner/recipient protocol rather than adding a
  second call convention.
- **Managed SHAPE PIC:** [Python/specialize.c:605](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/specialize.c#L605) requires two or more
  nominated SHAPE rows with distinct valid type versions and scalar read
  capability. Publication follows feedback recording. `shape_read_offset`
  at [Python/ceval_macros.h:793](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval_macros.h#L793) tests the current type version and shape;
  `shape_read_dup` at 821 acquires from the guarded slot. No descriptor pointer
  or feedback pointer survives a callback. The emitter at 3814/3874 binds one
  checked offset to the current receiver and clears that capability before
  intervening arbitrary statements. Learned-only handlers cannot enter via
  public images. This review covers the GIL configuration; it does not grant
  free-threaded publication admission.
- **Terminal completion:** [Python/ceval.c:3372](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval.c#L3372) retains one inlined shared
  implementation with constant-kind entry points at 3464 onward. It holds
  the result across PRE observation, detaches the full input window before
  publishing, then closes owners after POST observation while preserving an
  existing error. Generator return at 3492 decodes immutable call-site geometry
  and has a separately checked canonical result. These entries concern
  exhaustion/return, not every yield. No new per-call descriptor is built.
- **Exact-tuple CALL_EX:** [Python/ceval.c:1510](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval.c#L1510) performs only a callback-free
  borrowed exact-object-tuple test before refresh. Nonexact input still takes
  owning snapshots across iteration/callbacks. Exactness avoids preparation
  acquisitions without bypassing the refresh/reentry contract.
- **Qualification repairs:** f953's optional observer metadata check at
  [Tools/cases_generator/ir_metadata_generator.py:235](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Tools/cases_generator/ir_metadata_generator.py#L235) makes missing wrappers
  deny the permission. Generated-case mutations still test revoked permission
  or earlier mandatory rejection, and lowered object readers are discovered
  from emitted calls rather than exempted by a fixed whitelist. ac4/cb3 retain
  semantic assertions while correcting current magic, genuinely unknown unary
  controls, frame-size accounting and exact call geometry. Module attribute
  sites keep self-or-NULL routes; direct callable sites use no-self routes.

## Selection and evidence limits

The coordinator reports both builds at 117/79, repaired admission/retirement
regressions and 53 core methods passing in both roles, and current 70-method
fixture checks in progress. Those are reported results, not rerun here. The
previous full release had 116 failures across 13 modules; debug was interrupted. A new
coherent release/debug gate must establish the remaining failure classification.
Six sys_settrace jump errors are reported as an exact subset of golden54's nine;
that comparison does not fix observation and must not become a new waiver.
Only the preexisting GDB pretty-print class is quarantined.

All current performance numbers belong to **82e857384f8**, not cb3. At that
identity, all-29 geometric means are 1.151069/upstream and .989410/golden54; the
six-pair raytrace ratio is 1.035032/golden54. That regression requires explicit
root judgment/diagnosis before selection. Richards' large gain does not erase
it. Final qualification fixes need their own native identity and measurements.

## Clean architecture and coding opportunities

1. Centralize compiler fact invalidation and meet operations around the existing
   Local/token/action graph. Today `owns_value`, `ir_value`, nonnullness,
   element guards and shape capability have related handling in copy, merge,
   condition, loops and opaque statements. The cfb repair demonstrates the
   maintenance risk. Keep the facts independent, with one conservative kill
   interface per effect; do not introduce runtime descriptors or another parser.
2. Continue migrating individual whole-body publication recognizers to checked
   action receipts when the next real producer needs that contract. Existing
   narrow source recipes remain conservative but are expensive to audit as
   contracts multiply. Do not replace all recipes as unrelated cleanup.
3. `STORE_RESULT` documentation still describes outputs as nonaliases at
   [Tools/cases_generator/ir_generator.py:2245](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Tools/cases_generator/ir_generator.py#L2245); the checked owning-alias
   exception immediately below deserves a short documentation update next time
   that code changes. It is not a correctness blocker.
4. Keep direct completion entry points thin and constant-parameterized. Their
   shared implementation is preferable to independent copies of ownership,
   monitor and unwind logic. Do not widen the ABI solely to remove source-level
   switches that the compiler already removes.

## Ranked next performance investigations

These rankings use the retained 82e physical-symbol profiles and counters as
search evidence. Sample shares are neither exclusive costs nor removable
budgets. Each experiment first needs a dynamic operation delta, actual native
removal and balanced same-head elapsed confirmation; retain broad regression
checks and logging's advantage.

1. **Ordinary resume/delegated-yield transport.** Inspect surviving guard,
   recipient, resume-value and exception-state work in
   [Python/generated_ir_cases.c.h:23209](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/generated_ir_cases.c.h#L23209) (SEND_GEN) and 25497
   (YIELD_DELEGATED), grounded in generator profile 29.89%/14.80% self and
   instruction ratio 1.411737/upstream. Count normal yields, resume inputs and
   observation/slow exits before choosing one callback-free repeated operation
   to eliminate. Terminal completion is only 1.66% in that profile and cannot
   explain the whole gap.
2. **Exact Python call hit work.** Audit repeated admission/binding/refresh
   and frame-home work in generated exact-call handlers at 11388/11958 and
   [Python/ceval.c:4231](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval.c#L4231). Exact-call self samples are 11.91% in DeltaBlue and
   11.28% in coroutines; both have excess instruction counts. Confirm any
   supposedly redundant refresh survives optimized native code before changing
   it. Preserve independent code/function pins, observation and overlap.
3. **Coroutine creation and retirement.** Separate actual creation copies,
   clear scans, finalizer-capable releases and return adapters around
   [Python/frame.c:191](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/frame.c#L191),484,614 and [Python/ceval.c:3492](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval.c#L3492). Coroutine profiles
   name MakeCoro 4.63%, CopyCreatedGenerator 2.68%, ClearGenerator 4.78%, gen
   deallocation 5.44% and completion 6.29%. Count extents, survivors and slow
   entries to select a bounded hypothesis; do not presume zeroing is the cause
   or repeat a broad final-home construction rewrite.
4. **Current mono/PIC guarded read hits.** Examine repeated tag/type/shape
   checks and acquire/retire work around [Python/ceval_macros.h:793](https://github.com/rjpower/cpython/blob/cb3f0b00ef268a75acdc1f9609ad9bbb13a756d8/Python/ceval_macros.h#L793) and the
   generated SHAPE handlers. Mono SHAPE is 9.90% in generators; DeltaBlue has
   PIC-local 8.28% and mono 6.11%. Prior diagnostic counts prove selector use in
   DeltaBlue/Richards, with nearly all hits. Recount at the final identity if
   changing policy; specialize the frequent guarded form, preserving invalidation
   and pre-effect miss behavior. Logging has zero selector attempts, so selector
   loop work is not an explanation of its earlier regression.
5. **Next proved transport producer, chosen by executed frequency.** Count
   surviving Publish/object bridges and post-pass temporary owners, then extend
   shared result facts for one actual hot numeric/native producer. Current
   list/tuple/GET_ANEXT native removal proves the mechanism but not a general
   speedup. The exact-tuple CALL_EX shortcut has zero executed hot-six sites;
   that census does not establish list/tuple indexing frequency. The multi-use
   COPY morphology had
   zero candidates across 426 CFGs and should remain closed absent new evidence.
   The inline-list experiment removed a call/spills yet regressed coroutines
   10.7%; keep it rejected rather than treating fewer instructions as sufficient.

The final artifact was read back and reviewed. Candidate HEAD and clean worktree
were checked after source inspection. Only this review artifact was created.

## Subsequent memory-fill attribution

A separate analysis of the retained **82e857** coroutine recording resolves
its 4.330604% libc bucket at offset0x19846d. The recorded and current libc
build IDs match, and the host IFUNC resolver selects memset at0x198440.
All109 samples at that offset unwind immediately to
`_PyFrame_CopyCreatedGenerator`, at its single memset call. The retained
Python ELF matches the recorded build ID; its callsite explicitly passes zero
and fills destination scratch `[L,N)` under initialized-NULL control.

This identifies one construction cost, rather than an unknown Python symbol.
It does not classify every memset bucket, establish that scratch initialization
can be removed safely, or predict a4.33% speedup. Sampling skid and unwind/period
weighting still apply. The analysis is historical; current-head profiles must
be checked independently. No target Python execution or source mutation was
needed. Commands, ELF identities, raw stacks, disassembly and readbacks remain
under E56/qualification56/libc-profile-analysis. The [handoff](../../HANDOFF.md)
defines E56. Resume/delegated yield and exact-call work retain higher priority.

## Final selection review

After final883 qualification and measurement, root requested an independent
selection judgment over the six-pair confirmation, complete 29/control results,
regression PMU and qualification limits. The cold reviewer supports selecting56
under the user's explicit allowance for mostly-better incremental progress:
17 broad wins, a 1.53% mean reduction, and all six focus cases improving in every
confirmation pair. This is an explicit override of the broad audit's suggested
unexplained>2% regression tripwire, not evidence that those regressions are fixed.

Raytrace's 4.50% confirmation loss, spectral_norm/nqueens~3.5% broad losses and
chaos/XML losses remain accepted unresolved debt. PMU separates more executed
instructions from higher cycle cost but establishes no exclusive cause. Keep
immutable golden54 for comparison/rollback. The [report](report.md) and
[complete tables](benchmark-tables.md) own final numbers and qualification;
this review makes no universal-improvement or parity claim.
