# Qualify and integrate the selected RC runtime

- State: selected; qualification pending; not integrated
- Owner: coordinator
- Source: `d7df12b1d81722927e71d9734c70eeafdccfee73`, `boil/rc-successor`
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

Earlier launch failures remain preserved and receive no passing credit.
Existing `Py_DEBUG` images do not imply `Py_STACKREF_DEBUG` qualification.

## Active work and next decisions

| Track | Owner | Next artifact / gate |
| --- | --- | --- |
| Last targeted ownership/refleak tranche | coordinator | Session68982 terminal1:35/35 semantic rows verified; standard `-R4:5` returned success but natural-output reader rejected progress prefixes; failed batch preserved, no rerun |
| Independent targeted review | coroutine_design_review | Review actual raw results and narrowly corrected additive reader; no custom observers in measured leak callbacks |
| Broad qualification plan | restart_audit; coordinator | Concrete existing-gate binding by07:50; remove inapplicable tracing-counter assumptions explicitly |
| First native JIT design | qualification_review_resumed | Source-grounded finite candidate by07:45; no implementation grant or working-tier claim |

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
