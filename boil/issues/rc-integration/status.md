# Qualify and integrate the selected RC runtime

- State: selected; qualification pending; not integrated
- Owner: coordinator
- Source: `8e132becdad485f9d281d63c5ec1c9c60ef3e56f`, `boil/rc-successor`; packaging-only successor of dce654
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
- [Corrected full-suite outcomes](full-suite-outcomes-review-20261101-v2.md): release and O1/debug each50,827 tests, zero failures;3,430/3,377 standard skips. All565 selected modules returned,73 passed nested origins and seven protected modules verified. Actual under-lock cleanup accepted.
- [Fresh safety builds](profile-build-outcomes-review-20261101-v1.md): ASan and native stackref-debug each584 products, own extensions/configuration and actual profile witnesses accepted. Build/probe evidence only; no safety-suite credit.
- [Source syntax closure](source-syntax-outcome-review-20261101-v1.md):19 existing borrowed-header checks passed, all9,984 input records unchanged; four unsupported modules explicit. No link/import/runtime claim.
- [ASan composite adjudication](asan-selection-adjudication-review-20261101-v1.md): retained target3,827/0/77,26 actual modules/27 ordinary processes,14 passes and12 standard skips accepted with the source-derived reader. Original outer batch stays failed; no target rerun or fabricated completion.

Earlier launch failures remain preserved and receive no passing credit.
Existing `Py_DEBUG` images do not imply `Py_STACKREF_DEBUG` qualification.

## Active work and next decisions

| Track | Owner | Next artifact / gate |
| --- | --- | --- |
| Last targeted ownership/refleak tranche | coordinator | Closed through reviewed additive result reuse; no further bespoke cohort |
| Ordinary full suites | qualification_review_resumed | Independently accepted both corrected full suites; no further ordinary full rerun without affected source changes |
| Safety profiles | coordinator; qualification_review_resumed | ASan composite accepted. Stackref partial run failed. Both upstream builds failed before comparison methods; no further repairs. Entire diagnostic-profile scope amendment proposed, independent review pending |
| Extension and generation closure | restart_audit; qualification_review_resumed | Five-path packaging8e132 and four-image reuse bridge independently accepted. Concrete cheap-first five-row helpers in host preparation/review; generation still pending |
| First native JIT design | coordinator | Corrected metadata capture accepted; actual seven-site candidate and second direct Fable review have a source-audited disposition. No implementation grant |

At09:47 UTC, all native jobs are terminal. Corrected upstream control64610
returned1, observed09:43:39, within its fixed09:42:16-10:12:16 window. Its exact
[source](stackref-corrected-upstream-source-root-review-20261101-v1.md) and
[helpers](stackref-corrected-upstream-helper-root-review-20261101-v2.md)
are independently accepted. Only the take_ownership assignment's left-hand side
differs from upstream321925. This is a labelled compatibility control, not
unmodified upstream. Bootstrap again aborted while freezing four modules,
now on a closed-with-one-borrowed-reference check. No comparison method ran;
cleanup09:43:28 confirms all four native executable paths idle. Stop this
upstream repair path. A [whole-profile scope amendment](stackref-gate-amendment-proposal-20261101-v1.md)
is proposed for independent review; no comparison equivalence, exclusions,
passing checker result or approved amendment is claimed.
The original upstream build78795 returned1;
its cleanup receipt reports idle at09:22:59. It aborted in unmodified upstream
`_bootstrap_python` while freezing four standard modules: Invalid StackRef at
ceval.c2076. The three comparison tests did not run. Source diagnosis and a
bounded alternative are pending; this supplies no causal clearance for our
failed full checker run. Corrected release
session87194 returned0 with3,430 skips; debug60637 returned0 with3,377 skips.
Both selected and returned565 module results:463 passed,94 skipped and eight
resource-denied. The standard file-run summary is557/565. Keep skips visible. Successful
builds supply configuration/import witnesses, not safety-suite coverage.
Extension tests, cases/Clinic generation and stackref failure disposition
remain. Applicable unbuilt-module syntax and scoped ASan evidence are closed. Main source is still
the tracing runtime.

The first CPU extension batch29576 returned1. All15 runnable cases compiled
and linked their private-RC-suffixed modules, then setuptools rejected the
wheel ABI tag as absent from its vendored packaging support list. No child
runtime/refleak pass is claimed. The sole skipped case is the existing Linux
MSVC-only C++14 test. The serial batch stopped before regeneration and retained
raw child output and cleanup receipts. Keep loader isolation; review a coherent
packaging ABI correction rather than retagging wheels as upstream or disabling
the compatibility assertion. No source or retry grant follows from this failure.

The [packaging v2 design](private-abi-packaging-design-20261101-v2.md) is
[independently accepted](private-abi-packaging-design-review-20261101-v2.md).
It covers the three vendored tag functions and pip's separate installer path;
the first proposal missed that path and remains incomplete. The
[source implementation](private-abi-packaging-source-review-20261101-v1.md)
and [public layout](private-abi-packaging-public-layout-review-20261101-v1.md)
passed independent review and33 host checks. Root applied exactly five paths
at09:14:42 after idle/lock checks. The independently accepted formatting-only
stage preserves ASTs, all271 policy/test strings and wheel bytes. Root applied
its two text files at09:31:26 and froze exactly five paths in8e132; source is
clean. The [four-profile compiled-input bridge](private-abi-packaging-bridge-review-20261101-v3.md)
is independently accepted: all584 original products per image, configurations
and tool pins remain exact; only six/six/zero/six enumerated regular caches
were added. Source entry kinds and symlink targets are exact. This is explicit
reuse of historical builds, not four fresh successor builds or new test credit.
Public loader, runtime identity and test_cext bodies stay unchanged;
cross-target pip overrides are unsupported in this initial integration.
A held [focused feedback design](packaging-feedback-design-review-20261101-v1.md)
puts regeneration and affected ensurepip/venv checks before the expensive CPU
extension gate. Concrete helpers remain in host preparation; no native retry
is granted yet.

ASan31887 returned1 in the host result reader although the standard target
harness returned SUCCESS:3,827 tests, zero failures and77 skips. Standard
discovery expands the three multiprocessing packages, giving26 scheduled
modules from16 requested names. The frozen validator wrongly compared those
sets directly. Its [additive correction](asan-selection-output-correction-20261101-v1.md)
retains every other predicate and all raw evidence; independent adjudication
accepts the composite. The original batch remains failed without a native rerun.

Root stopped remaining stackref work at08:47:32 after563/565 module returns:
449 passed,94 skipped,12 failed and eight resource-denied. Target shutdown,
threading, asynchronous dump/profiler and embedding cases failed. The timeout
returned124 after root's intentional SIGTERM, not expiration of the original
5,400-second allowance. Supervisor15564 returned1; exact-executable cleanup
verified idle and removed its owned cache. This is a failed partial full run.
Source audit distinguishes live-reference shutdown reporting, async readers
without TLS/GIL and remote readers assuming pointer bits. Some source paths
also occur in upstream321925; no fork-specific cause, repair, exclusion or gate
relaxation is established yet. Landing remains blocked pending causal evidence.

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
