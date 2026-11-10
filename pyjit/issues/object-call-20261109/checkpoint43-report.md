# Checkpoint 43: measured, API rejected

Candidate 43 is a historical checkpoint, **not a promotion candidate**. Its API gate rejected two guarded-word unions. The full 43 suite was not run. Successor 44 is in progress and needs its own qualification; these measurements describe 43 only.

Candidate source is `47dc23f48cf1470d4a98137beb5d313b5093033d`; frozen 39 is `4d666e1b482cb4983c7683f91c5922af14d0ab40`, and upstream is `3a440b65695615e9fd9912342b79cdd54fced0d2`. Evidence root `E` is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`. The [tables](checkpoint43-tables.md) retain every all-29 ratio, follow-up ranges, micro results and instruction estimates. The [cold review](final-cold-review43.md) audits the raw measurements and their limits.

## What 43 contains

The packet adds bounded fixed-keyword Python-call plans, guarded class/non-descriptor lookup metadata, and the same-code positional-default shape guard. Keyword capacity is 1–3 names, independent of function arity; the binder preserves one frame-transfer ABI. Relative to 41, 43 changes only the inline policy of the shared overlapping-frame push and frame initializer. These annotations remove unconditional outlined constructor boundaries without duplicating the binder.

The [compiled review](post-review43.md) found a remaining conditional 112-byte constant-fill leaf, larger hot stack reservations and collateral custom-getattribute code growth. Release text is 7,527,899 bytes, 744 bytes above 41. These are tradeoffs, not evidence that a further forced-inline cascade would help. The 43/39 timing comparison includes the whole packet; it does not isolate the annotations.

## Measured result

The all-29 elapsed geometric mean is **1.004676 versus frozen 39** and **1.191545 versus upstream**, with two pairs per case. Long six-pair micros improve sharply versus 39: class load **0.156627**, keyword call **0.604334**. They remain above upstream at **1.047186** and **1.070554** respectively. The class micro exercises one repeated integer class attribute, not all lookup paths.

Six-pair application follow-ups confirm **coroutines 1.067700**, **spectral norm 1.081747** and **comprehensions 1.055996** versus 39, with every pair slower in each case. Logging silent improves to **0.959975**, with every pair faster. DeltaBlue, generators and nbody remain near parity. Logging simple is mixed: **1.027095**, range **0.953031–1.102142**. The aggregate and micro wins do not waive the repeated application regressions.

Whole-process differencing estimates coroutine instructions at **0.999405 versus 39** and **1.383604 versus upstream**. Near-equal instructions versus 39 do not prove a CPI, code-layout, cache or branch-prediction cause for slower elapsed time: the packet lacks matched 39 cycles. Retained 41 profiles identify exact Python calls and generator creation/completion as relevant coroutine work; an assumption that general-call fallback dominates is unsupported. This report makes no 43 profile-share claim.

The separate hot-interval PMU protocol shows 43/upstream instructions of **1.378196 for coroutines** and **1.870776 for DeltaBlue**. Micro net instruction ratios versus upstream are **1.204971 for class load** and **1.547851 for keyword call**. Thus meaningful executed-work gaps remain despite elapsed micro gains. Skeleton-subtracted micro estimates, whole-process differences and hot-interval counts have different boundaries and must remain separate.

All cited elapsed campaigns report stable identities and no campaign errors. The independently audited secondary campaign includes 36 successful invocations. PMU stat retained 18/18 valid invocations and record retained 6/6 stable profiles. Recorded compiler-flag dictionaries match across roles, but contain null/unexpanded entries; they do not independently establish every resolved compiler/linker input.

## Qualification boundary

Source-43 release and debug builds succeeded, with 117 checked modules each. The six focused modules passed **273 tests per build**: frame calls, attribute cache, calls, native call transactions, semantic completion and operand modes. The stats build succeeded and its two-module smoke passed **150 tests**. Sources are `E/object43-{build,focus,stats}-checks.json` and their retained logs.

**The API gate failed** (`E/object43-lint-checks.json`, `E/object43-release-lint.log`): 306 value-native units, zero parse errors, and two failing raw-bits findings at `Include/internal/pycore_feedback.h:131` and `:189`, both “puns a word through a union.” Handler discrimination does not satisfy the representation rule. The [guarded-cache contract review](guarded-cache-contract-review.md) approves independent guarded fields, with separate capacity/coverage accounting. That repair changes the source identity. Focused passes do not replace the failed gate or the unrun full suite.

## Next work, kept separate from this snapshot

The immediate priority is the approved storage repair and successor qualification. Then locate the repeated application regressions using matched measurements and actual hot paths. The existing design packets remain proposals, not permission to combine new features with the gate repair:

1. [Owning generator/coroutine entry](owning-generator-entry-proposal.md), with the concrete [entry/call contract](entry-call-contract-proposal.md): finish the publication, precommit redirect, pinned-code and cleanup proof; census eligible exact calls before implementation.
2. [Lookup work](lookup-proposal.md): census weighted DeltaBlue sites and repeated proof work before expanding the class-micro shape.
3. [Keyword plans](keyword-proposal.md): distinguish immutable builder proofs from mutable guards, auditing every publication/invalidation route before reducing checks.
4. [Value-native cell/getset boundary](value-native-cell-proposal.md): defer until application exposure justifies the bridge work.
5. [Generator-cycle work](generator-cycle-proposal.md): retain the full ownership/update/delivery ledger; it is not a substitute for the creation contract.

The [cold review’s ranked priorities](final-cold-review43.md#five-next-priorities) put the gate repair and confirmed regressions ahead of further optimization. Candidate 43 stays sealed and rejected. A later report must name successor 44’s actual identity, gate results, capacity evidence and measurements.
