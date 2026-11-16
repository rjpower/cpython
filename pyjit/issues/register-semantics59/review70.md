# Grouped architecture review70

**Recommend one canonical CALL-result unit from selected57 for independent
source adoption.** Move local assignment/discard to explicit caller IR and
remove their state from both Python and native call producers. The complete
five-option ranking, interfaces, owner/control contract, two diagrams and finite
selection gates are in [design70](../../design/proposals/interpreter-parity70.md).
This review is not implementation, source GO or native qualification.

Round69 is stopped and unselected, with incomplete behavioral qualification.
Earlier losers remain closed. Selected57 stays unchanged. Root owns adoption,
Sol implementation, generated products, qualification, measurement and selection.
No second source lane or new diagnostic is proposed.

## Authority and work performed

Based on canonical documentation commit
`b959c9af9b5964937c703c24be026f491cebd515`. Read applicable AGENTS and the
[official CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
README, GOAL, STATUS, HANDOFF, PROCESS, active performance56 plan, current report
and previous grouped designs/reviews. Only these two round70 documents change.
No agents, runtime edits, target execution, generation, builds, tests, timings,
profiles, counters, control mutations or push were performed for this review.
Verification is static source/evidence reading and document/diff checks.

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`R=BASE/worktrees`; `E59=BASE/pyjit-link-wave1/register-semantics59`.

| Authority | Actual pinned source / binary |
| --- | --- |
| I, selected57 | `R/call55-integrated`, `0dcba75bb3df40c32137db453b001f7e8f101de0`; captured `BASE/pyjit-link-wave1/call-model55/comp57-dictionary-native/python`, SHA256 `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2` |
| U | `R/upstream-3a440b6`, `3a440b65695615e9fd9912342b79cdd54fced0d2`; release `BASE/upstream-release-devlibs-3a440b6` |
| G69, frozen loser | `R/call55-gc`, `52806aacedf5be0a89546344162bb53a8e40ec34`; current release `BASE/reg59-results-release/python`, SHA256 `505b199a47f9b6c7148511492a4fd57267b3b635b825533739420decbd1b9b36` |

C and this documentation worktree are **not runtime-source authorities**. Paths
below use actual I/U/G, with line anchors referring to those pinned trees.
Source inference, static native cost, actual dynamic reach and elapsed results
are distinct evidence classes.

## What round69 establishes

Source/products4 is `f19b6629588`; fixture-only fixes continue through G52806.
Both tests6 roles built117 modules/79 extensions; focused29/role with1skip,
related53/role, and code/marshal/function/monitoring350/role with9skips passed.
All331 existing format rows stayed unchanged. Those passes do not close the
literal positive committed-summary pending/immediate/same-PC/service-error
contract. Existing trace controls activate tools before CALL and force a miss.

The original finalizer redirect fixture asked for behavior that I release and
debug also reject: external child, parent IR_NO_DST and CALL do not make that
ancestor recipe supported. The corrected fixture records an actual attempt and
supported/rejected outcome; it does not broaden the setter. Python finalizer,
signal and GC reentry cross an external evaluator activation, and same-PC
identity requires fresh observed-parent PRE/LINE state. Neither fact proves all
C/remote ingress impossible. Keep the open matrix open.

The source bugs matter: legal A≠B0 result publication needed a real repair;
shared output, guard and disjoint proofs also needed ports after initial source
GO. New source review must prove the complete producer/consumer relation, not
infer it from specialized names or compiler-origin annotations.

Read `E59/guarded-summary69-native-peer.md` and raw original/current native JSON
receipts in `E59/guarded-summary69-native-controls`:

| Complete reached path | I instructions / stack | G instructions / stack | Actual G decomposition |
| --- | ---: | ---: | --- |
| projection | 395 / 80B | 437 / 152B | summary handler321 + path reader116 |
| select false / true | 537 / 80B | 612 / 264B | summary handler337 + predicate43 + two path reads232 |

The envelope is actual CALL entry through the next parent handler, including
child/helpers/guards/acquisitions/retirement/poll/recipient, excluding that final
parent handler's body. All G successful expressions remove the child frame and
dispatch, return the correct object, consume inputs to NULL and balance native
stack. Debug reached success is also proved. Stack is the reached synchronous
high-water, not a universal bound. Absence of a helper entry alone is not proof
that its work was absent or inlined.

Selected projection's exact decomposition is CALL140, slot read96, RETURN_EMPTY67,
clear_thread_frame42 and TryClear50. This shows the real child work that was
removed; it does not imply all159 return/clear instructions are optional under a
real-frame design. G's read totals include necessary mutable guards alongside
plan decoding. Moving static proof cold cannot delete those dynamic checks.

Main release .text grows5,079,417→5,125,113 (+45,696B). Peer grouping reports
11 summary handlers19,775B, named reader helpers7,352B and cold completion1,900B.
Do not add incompatible producer-symbol scopes or call these sums the whole
text delta. Different tests5/tests6 stamps and normalized symbol comparisons
are preserved; no old-ELF execution is relabeled as a current run.

Actual unchanged setup/driver/run(1), two ordinary warmups and one post-ready run
are recorded in `E59/guarded-summary69-natural/*-tests6-release-v2.native.json`.
Delta has23 selected static sites and a first successful
`BinaryConstraint.choose_method` word47 witness:653 instructions, no child.
Comp has0 selected sites/attempts/demonstrated hits. These are not frequencies or
proof of a nomination/equality cause. The failed initial fixture preflight is
retained separately from the corrected run.

`E59/guarded-summary69-tests6-screen-admission.json` explicitly admits an early
**rejection diagnostic**, not selectable qualification. The stable32-row
`E59/summary69-hot8-selected57-screen/summary.json` records:

| Case | G / I mean |
| --- | ---: |
| comp | .979364115 |
| Delta | 1.034596287 |
| coro | .957014137 |
| gen | 1.018078910 |
| logging | 1.176093456 |
| ray | 1.034176961 |
| spectral | .945331801 |
| nq | .974314386 |

GM1.012667498; only improvement passes. GM/all-means/logging/all-pairs fail.
Comp pairs .954286 and1.005101 oppose. No confirmation, all29, PMU, full suite,
repair, new qualification or timing rescue is ordered. No fresh upstream result.

## Source contrasts that determine the recommendation

| Actual source anchor | Finding / consequence |
| --- | --- |
| [I exact generated CALL](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:11388) | CALL carries base and arbitrary destination through guards/entry/fallback; exact overlap and direct binding are already selected. C source repetition alone does not prove duplicate native loads. |
| [I native descriptor CALL](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:8479) | Provider publishes B0, drains inputs, polls, then handles local/discard and displaced-owner completion. Rank1 removes that last producer responsibility, retaining the earlier sequence. METH_VALUE does not imply a boxing-array adapter. |
| [I transaction](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval.c:789), [recipient macros](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval_macros.h:708) | Destination is real state in the general transaction; specialized producers have local/discard handling too. Remove the state and interfaces, not just a branch before the unchanged engine. |
| [I RETURN_EMPTY](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/generated_ir_cases.c.h:22942), [rooted cleanup](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval.c:3275) | Selected already unlinks, clears, reloads parent and returns inline. Reentrant clear roots and real live bindings remain necessary. A terminal-return alias alone earns no new saving. |
| [I return publication](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Include/internal/pycore_interpframe.h:237), [pending ancestor mutation](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/frameobject.c:3544) | Scratch is vacant; local/discard have extra effects. Pending mutation currently requires return_slot IR_NO_DST. Canonicalization must replace that cold logical-discard test and preserve supported control, not silently make it unreachable. |
| [I semantic constructor](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Objects/codeobject.c:3939), [CALL window validation](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/ceval.c:1417) | Admission already freezes immutable validated words. Windows start at scratch homes. Require the new A=B format here; lower compiler uses before layout and encode maps normally. No legacy word/map translator is needed. |
| [I store elimination](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/regcfg_passes.c:374), [discard folding](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/regcfg_passes.c:665) | Prevent CALL-specific folding and preserve actual caller consumers. Do not disable useful folds on unrelated producers. |
| [U exact call](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c:4568), [U return](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6/Python/bytecodes.c:1574) | U still initializes/clears a frame and guards stack/recursion/provider. Its simpler stack recipient defers STORE/POP. Charge those consumers in every comparison. |
| [G summary evaluation](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:1342), [G summary admission](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc/Python/ceval.c:1473) | Root/form/path interpretation and repeated live reader guards replace real child execution. This is not a model for a new intraframe descriptor walker. |
| [I derived reads](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c:2685), [I slot read](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/bytecodes.c:2799), [existing FuseJump](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated/Python/regcfg_passes.c:1130) | Derived values still get owning homes; that differs from selected local forwarding. But compare/jump and truth folding already exist, and source/EH restrictions are real observation boundaries. Rank2 needs a distinct physical lowering before release. |

The source-only independent inputs are retained as
`E59/round70-call-cost-source.md`, SHA256
`101bb427bfb58f6a7b0da2de84b62b34181cac2b9a47ee86f4ebfc47048e7207`, and
`E59/round70-dataflow-source.md`, SHA256
`749f4328a8421cccd6c1bd73d0480f3d02fa333e739ee2fad9387df0b7240617`.
They supply source findings, not GO or measurements. This review checked the
actual sources and raw native receipts rather than treating those notes as proof.

## Coverage, rejected alternatives and limits

Read actual words in `E59/resident-selected57-{deltablue,comprehensions}.json`.
`Strength.stronger/weaker` have two attribute reads, compare and return;
`BinaryConstraint.input` contains a global/class comparison and two projection
return arms. Comp `_is_big_spinny` has generic equality, short-circuit and an
attribute on the final return arm. Its RETURN also receives the false edge:
adjacency alone cannot certify a unique consuming edge. These are source/IR
opportunities, not successful borrowed-region hits. No line/name-based admission
or per-benchmark generated handler is acceptable.

The provisional derived-read choice was changed, not silently enlarged. A few
field-return/test/compare heads would remove real homes but remain a small motif
roster. A general forest needs a physical representation: a path walker repeats
69's problem; an additional carried-value ABI changes all execution boundaries.
Neither earns broad coverage from those examples. Keep rank2 deferred rather
than claim that a few motifs solve the comp/Delta deficits.

Root's draft review correctly removes another unnecessary complication: rebuilt
IR/API/source/binary compatibility is not an admission gate. The final design
defines A=B as the new CALL format, updates public producers/fixtures and magic,
and rejects noncanonical new-format images. It does not translate arbitrary old
CALL words or rebase already-encoded maps. Compiler changes precede layout.
Round69's A≠B repair remains necessary evidence under its unchanged format;
round70 deliberately changes that format while preserving Python behavior.

Rank1 is distinct from Invoke59 (already removed a duplicate CALL-window sweep),
provider59, compact64 (47 internal instructions saved versus93 entry/exit added),
and native67's appended consumer wrapper. It changes the shared result contract
and **deletes** old producer responsibilities. It is also unlike role63 or
anchored-self67: it adds no owner mode, promotion check or role-specific head.
All actual input owners and real frame retirement remain. Its unfavorable
possibility is equally concrete: explicit consumers may cost more than the
branches and live state they replace. The full-consumer native gate must expose
that before a broad campaign.

No claim is made that ordinary native bodies are all unchanged, that .text
explains logging, or that equal retired counts imply equal work. The earlier
finite logging diagnostic excludes only a proportional-count/equal-CPI account
of the whole gap. Different mixes and unobserved phases remain possible. No new
causal diagnostic is requested.

The ordered handoff is one source proposal/independent peer, one coherent
implementation from I, root focused/source/product/native qualification, then
one protected screen if justified. At most a single expressly labeled early
rejection screen can precede unfinished hard qualification. Only a fully
qualified protected winner reaches confirmation/all29/full/direct upstream.
No instruction-count target alone selects an executable or waives owners,
observers, errors or the existing hot8 protections.
