# Grouped architecture review67

Decision: **GO source Unit A, complete native callout/result consumer; GO source
Unit B, one caller-anchored exact-method parameter, after independent adoption.
STOP standalone continuation/union rescue.** [Design67](../../design/proposals/interpreter-parity67.md)
specifies interfaces, ownership closure, tests, ordering and exact screen rules.
Neither unit is implemented or native-qualified by this review.

This review starts from canonical `40add6e56c36d53363ec6b4b54cdfb6ace4fdcf9`.
README, GOAL, STATUS, HANDOFF, PROCESS, the active
[plan](../interpreter-performance56/plan.md), [report](report.md), prior design66,
applicable AGENTS instructions and the official
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/)
were consulted. Canonical runtime source is stale and was not used as runtime
authority. No target execution, build, generation, benchmark, new counter,
control mutation or subagent work occurred. Only these two review documents
belong to this branch. Root owns navigation, peer adoption and integration.

## Authority and closure

| Role | Immutable authority |
| --- | --- |
| I, selected57 | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`, `0dcba75bb3df40c32137db453b001f7e8f101de0` |
| U, upstream | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6`, `3a440b65695615e9fd9912342b79cdd54fced0d2` |
| G, rejected but qualified66 | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc`, `64348dd5c182af27c058e60f1e3f849f00d7ef81` |
| E59 | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59` |
| Selected captured ELF | `../call-model55/comp57-dictionary-native/python`, SHA256 `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2` relative to E59 |
| G final release ELF | `E59/parity65-continuation4-native/python`, SHA256 `79bd05e06af44d730c343d8e34a41ae1538041e4f1e27655bd853e587c7d9fc4` |

Round65's native NO-GO remains historical fact. Round66 separately qualified
unchanged G: both117/79 builds and48 focused methods/role, plus current natural1,
terminal2, armed-error1 and armed-normal2 actual transactions with independent
final GO. This is a qualified mechanism followed by a rejected executable, not
an implementation failure. No all29, confirmation, full or upstream screen of G
was authorized after the losing screen. Auxiliary union remains unimplemented.

| Stable round66 candidate/selected57 mean | Ratio |
| --- | ---: |
| comprehensions | .9747244492 |
| DeltaBlue | .9791755373 |
| coroutines | .6260262279 |
| generators | 1.0144569295 |
| logging_silent | 1.0764670969 |
| raytrace | .9553110279 |
| spectral_norm | .9697604717 |
| nqueens | .9818861826 |
| hot8 geometric mean | .9368713684 |

Source: [stable screen][screen],32 rows. Generator mean and logging protection
reject selection despite the GM. Do not reinterpret these as fresh upstream
measurements. Last selected/upstream matched instruction/cycle ratios remain
comp1.436416/1.333179, Delta1.361424/1.118423, coro1.307831/1.267788.
Generator1.372523 is old elapsed evidence, not matched new PMU.

## Completed diagnostic: what it does and does not establish

[Diagnostic receipts][receipts] and [offline leaf attribution][attribution]
are the retained evidence. Same work:2,383,648 stat iterations,5,959,120 record
iterations and60,000 warmup; three stat pairs and one cycle record per role.
Input closure5,563, current ELF/build IDs and FIFO ACKs are pinned.

Cycles GM1.085426304460, pairs1.084509/1.084837/1.086935; instructions
GM.992588295967, essentially identical across pairs, all100% running.
The records have2,421/2,510 samples and no lost/throttled events.

| Disjoint, period-weighted physical self category | Selected57 | G |
| --- | ---: | ---: |
| Changed terminals/accessors/GC scope | 8.307% | 8.453% |
| Proved-equal ordinary cleanup | 13.416% | 13.518% |
| Other ordinary/core/native helpers, not proved equal | 78.276% | 78.029% |
| Separate DSO providers / unknown | negligible | negligible |

All changed-scope samples are RETURN_VALUE_EMPTY. Equal scope is
TryClearExceptCode and clear_thread_frame, not every ordinary handler.
METHOD_WITH_VALUES15.52/12.87%, SHAPE_LOCAL_TEMP11.53/10.96%, PY_GENERAL
11.25/12.22%, BOOL branch9.31/8.86% and exact CALL8.48/8.38% remain other.
Independent self-share samples are neither inclusive costs nor paired deltas.
Multiplying shares by the cycle ratio or subtracting them cannot attribute loss.

The diagnostic rejects only equal-CPI proportional instruction volume as the
whole explanation. It leaves mix, dependence, expensive instructions, phase
interactions and unobserved work unresolved. There is no demonstrated cache,
frontend, branch-miss, allocator-class or layout cause. In particular the old
generator request240→248 B does not prove an allocator-class transition.
No new profile/counter campaign, compiler/flags or padding/alignment search is
authorized by this review.

## Actual source contrasts

Paths below resolve to I/U/G, not the documentation checkout. Line anchors are
inspection locations; functions and immutable revisions identify the authority.

| Source read | Established source fact; limit |
| --- | --- |
| [I handler ABI][abi], [I exact entry][exact] | Five NCSR VM arguments; exact THREAD homes overlap. Callable ownership and independent code ownership remain. `_PUSH_FRAME` already skips RESUME. |
| [I general CALL][general], [I binder][binder] | Direct/keyword plans already exist. Keyword hit revalidates cached destinations/name identities/duplicates/default coverage. Direct hit validates default shape when needed, then initializes overlapping child. `start_frame` also already uses IR_SKIP_RESUME. |
| [I producer][specialize], [I function versions][versions], [I feedback][feedback] | Function version can be shared by same-code closures. Critical mutations invalidate it. Cached binding data are scalar layout facts, not a function-identity ownership receipt. Replacing version semantics would affect other consumers. |
| [I descriptor CALL][native-call], [I provider][provider], [I value dispatch][value-call] | Handler guards receiver/descriptor, then value-vectorcall selects provider. B0 roots the result through retirement. Canonicalization cannot be removed merely because a helper returns PyValue. METH_VALUE is not a boxing adapter. |
| [U general/exact CALL][upstream-call], [U checked helper][upstream-helper] | General path uses a consuming binder; exact entry moves StackRefs. Checked descriptor helper converts StackRefs. Argument lifetime and callback obligations do not disappear upstream. |
| [I frame layout][layout], [I kwargs materialization][kwargs], [I clear][clear], [I locals writer][locals] | Header112; slot indices are12-bit, pending_kwargs is16-bit. Clear assumes owning homes; kwargs/copy/locals readers need explicit changes for Unit B, not an annotation. |
| [G return helper][g-return], [G terminal][g-terminal] | THREAD-first ordinary retirement is separate from resumable completion; resumable path obtains live continuation after callbacks. This architecture is preserved as evidence, not patched in round67. |

Existing frontier clearing, PyValue canonical homes, local borrowing/copy
propagation, PICs, exact overlap and inline completion remain selected. IR_FB
already uses frame-cached base plus index on ordinary hits. These are not new
savings. Independent code ownership remains necessary when function code can
change. Public IR must share proofs with compiler output; source positions and
benchmark identity never establish ownership or admission.

## Native boundaries

Retained selected listings were read, not regenerated:

- [FAST baseline listing][fast-listing]: 64 B local stack; VM/operand preservation,
  provider selection, result canonicalization/publication and input cleanup are
  visible. Whole Unit A cost must include its consumer and return to dispatch.
- [General CALL baseline listing][general-listing]:112 B local stack and numerous
  binding intermediates. This is evidence of physical work, not a proof that a
  new C ABI will remove those spills or that all samples hit the same path.
- [Round65 corrected boundary review][b-review]: common completion93 instructions;
  ordinary SEND_GEN/yield/exit157 versus selected160/await9176; generic SEND child
  yield185 versus187/201. Those are static paths, not round66's causal explanation.

Invoke59 already removed duplicate window clearing and lost. Provider59 lost.
Compact64's whole paths173→227,234→280,38→128 lost native net work despite
112 B/1,665 B caps; internal47 saved versus93 entry/exit. Method62's adjacent
method/exact-CALL optimization lost. Round60's4,472 B stack/+55 KiB monolith lost
all8. Unit A must remove an additional complete consumer boundary; Unit B must
remove a receiver owner across the activation. A wrapper, another role head or
the same work moved into a helper fails the respective mechanism claim.

## Coverage used to rank the options

[Resident coverage](resident-coverage.md) and the retained
`E59/resident-selected57-*.json` are monitored semantic visits assigned to
pre-monitor warmed labels. They do not count specialized hits, guard success,
mortal receiver eligibility or unpromoted loans. A bounded host `jq` aggregation
of `before[].words[]` CALL labels confirmed:

| Case | Warmed-label visits relevant to this choice |
| --- | --- |
| Delta | CALL_PY_EXACT_ARGS1,523,940; CALL_NO_SELF_PY_EXACT_ARGS210; descriptor FAST107,040; no PY_GENERAL label visits |
| logging | CALL_PY_EXACT_ARGS600,000; CALL_PY_GENERAL600,000; no hot keyword-CALL visits |
| comp, prior retained census | Descriptor FAST648,000; LIST_APPEND972,000; all CALL1,140,002 |

Prior structural zero-explicit-argument method pairs are Delta1,280,190 and
ray1,026,471; comp/logging have none in that shape. Unit B's stronger admission
may remove many of these. They are a reason to attempt one complete owner
convention, not an additive instruction budget. Unit A's reached dict.get→append
edge exists, but no result-move credit is available when A=B=17. Neither unit
has demonstrated throughput or a complete parity budget.

## Adoption and limits

Design67 deliberately chooses two different consumers of call facts. A retains
owning snapshots through a native callback and consumes a rooted result. B
avoids creating one snapshot only while its caller-local owner is protected;
promotion converts it to the ordinary contract before exposure. The effect
boundary is shared; per-dispatch loan checks and another ownership interpreter
are excluded. B's extra zero-counter test and retirement classification are
priced against ordinary logging, not assumed free because they are short.

Hard gates remain behavior, single ownership, GC roots, supported aliases,
callbacks/errors/observers and exact source/product/ELF identity. Native counts,
stack/text and header bytes inform a bounded experiment; no static+1 cliff
substitutes for throughput. Mechanism falsifiers are concrete: A cannot retain
both complete dispatch boundaries, and B cannot retain the receiver snapshot
or have no natural unpromoted mortal hit. Each safe qualified unit gets one
balanced broad screen and stops on the explicit losing predicate. Bulk suite,
all29, fresh confirmation and direct upstream measurements belong only at a
promising coherent integration. No speculative profiles are requested.

The coroutine success is retained as a design asset. Composing it after a broad
winner is a new integration question with full logging/generator protection;
neither source reasoning nor ordinary-call savings guarantee cancellation of
its loss. Generator delegation remains a separate ranked option. No source
unit here claims to solve generator or coroutine parity by itself.

[screen]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity66-continuation-eight-selected57/summary.json
[receipts]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/continuation66-source-host/logging-diagnostic-receipts-peer.md
[attribution]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/continuation66-source-host/logging-leaf-attribution.md
[abi]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval_macros.h#L43
[exact]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c#L4353
[general]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h#L12117
[binder]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval.c#L4243
[specialize]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/specialize.c#L2260
[versions]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/funcobject.c#L280
[feedback]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_feedback.h#L155
[native-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h#L8479
[provider]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/descrobject.c#L572
[value-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_value_call.h#L108
[upstream-call]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c#L4418
[upstream-helper]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/ceval.c#L871
[layout]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_interpframe_structs.h#L29
[kwargs]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/frame.c#L280
[clear]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/frame.c#L420
[locals]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/frameobject.c#L306
[g-return]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c#L3796
[g-terminal]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/generated_ir_cases.c.h#L23097
[fast-listing]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/compact64-baseline-static/_TAIL_CALL_CALL_METHOD_DESCRIPTOR_FAST.txt
[general-listing]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/parity64-await9-native/selected57-static/_TAIL_CALL_CALL_PY_GENERAL.txt
[b-review]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/boundary65-source-host/unit-b-corrected-native-cold-review.md
