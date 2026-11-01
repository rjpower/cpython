# Qualify and integrate the selected RC runtime

- State: selected; qualification pending; not integrated
- Owner: coordinator
- Source: `dce654018d5ae16ffe5e772976d9ccc2c9d1e01a`, `boil/rc-successor`
- Worktree: `build/worktrees/rc-successor`
- Decision: [early GC rejection and RC selection](../gc-parity-deadline/decision-20261101.md)
- Profile: Linux/GIL, GCC computed interpreter, boxed-slot shapes

## Contract

Restore one coherent RC/cyclic-collection model while retaining the register
compiler/interpreter and eight-byte NaN-boxed frame representation. Do not
introduce a collector selector or claim that RC has reached performance parity.
Later core-wide inline heap values and their ABI are deferred.

## Accepted evidence

- [Exact source/ABI review](../fundamental-runtime-design/rc-source-review-20261101-v1.md).
- [Fresh initial behavior](../fundamental-runtime-design/rc-native-outcome-review-20261101-v1.md):39 methods in each image, zero skips.
- [Canonical reproduction](../fundamental-runtime-design/rc-canonical-review-20261101.md): all7,059 source paths identical; no source correction or rebuild required.
- [Additional compatibility](../fundamental-runtime-design/rc-compatibility-native-outcome-review-20261101-v4.md):50 debug/48 release invocations, zero skips;37 actual programs including embedded executables and required aborts.
- [Final focused composite](../fundamental-runtime-design/rc-final-targeted-output-root-review-20261101-v1.md):35 methods per image, zero skips; standard eight-case `-R4:5` succeeds with72 natural body invocations and24 actual programs. Original host-validator failure remains failed; additive reader independently accepted without target rerun.
- [Private-namespace correction and native reuse](namespace-source-review-20261101-v1.md): successor dce changes only the traceback diagnostic and two Python tests. Both original584 native-product sets remain exact; release adds six enumerated regular caches. Actual fresh native checks remain pending.

Earlier launch failures remain preserved and receive no passing credit.
Existing `Py_DEBUG` images do not imply `Py_STACKREF_DEBUG` qualification.

## Active work and next decisions

| Track | Owner | Next artifact / gate |
| --- | --- | --- |
| Last targeted ownership/refleak tranche | coordinator | Closed through reviewed additive result reuse; no further bespoke cohort |
| Release full suite | restart_audit; qualification_review_resumed | First full FAILED; corrected short-path/source/origin helpers in final preparation and review, then a separately authorized3810-second window |
| Remaining broad profiles | coordinator; coroutine_design_review | [Broad plan](broad-qualification-plan-20261101.md) independently accepted; O1/debug full and fresh ASan/stackref-debug build helpers nearing freeze; CPU extension/regen helpers held |
| First native JIT design | coordinator; Fable5.1 | Fable completed: amend. [Coordinator disposition](../jit-first-native-tier/coordinator-disposition-20261101.md) requires actual word capture before choosing the source attempt; no implementation grant |

## First full-suite failure and bounded correction

The [independent first full outcome](release-full-outcome-review-20261101-v1.md)
is FAILED:565 selected/scheduled/returned modules,50,827 tests,12 failures and
3,430 skips. Ten assertions depend on a short temporary root; our preactivation
source survey missed those test preconditions. The other two expose private
magic/ABI integration assumptions. The actual traceback consumer needed a fix,
not just a changed golden assertion. A separate observer defect left70 passed
nested modules without origin proof. No passing full-suite credit is carried.

Source dce follows an independent design review and exact source/product review.
Its behavioral filesystem fixtures reject the old diagnostic for upstream,
abi3 and untagged native files without broadening loader admission. Corrected
helpers keep TMPDIR short, normalize module origins, bind the changed traceback
origin and explicitly reuse historic d7 compiled images with current dce
library/test hashes. The old batch and all12 failures remain preserved.
The [helper follow-up](release-full-helper-review-followup-20261101-v1.md)
also corrects preflight telemetry: four GCC metadata queries occurred, with
no compilation or target execution during preparation.

The [final targeted plan](../fundamental-runtime-design/rc-integration-next-plan-20261101.md)
and [independent plan review](../fundamental-runtime-design/rc-final-targeted-plan-review-20261101-v1.md)
define the last bespoke cohort. After a pass, proceed directly to broad
qualification on one source: release and O1/debug stdlib, `test_cext -u cpu`,
applicable ASan/stress and native stackref-debug checks, and required extension
rebuilds against the RC ABI. Reuse exact canonical-generation evidence.
Do not invent another intermediate cohort without a concrete failure.

Full-suite and native shared-resource jobs serialize. Independent source/design
review may overlap. Preserve source/product identities, actual failures, known
exclusions and unrelated user work. Independent landing review and an exact
integration bridge precede the fresh release/debug integration smoke checks.
Only then update the main runtime description to RC.
