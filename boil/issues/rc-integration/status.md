# Qualify and integrate the selected RC runtime

- State: candidate qualified and merged locally; fresh main integration builds pending
- Owner: coordinator
- Source: `c4acfdde9d794345cd1f7001bcc7198d7d810f2d`, `boil/rc-successor`; canonical Clinic successor of8e132, clean
- Worktree: `build/worktrees/rc-successor`
- Decision: [early GC rejection and RC selection](../gc-parity-deadline/decision-20261101.md)
- Profile: Linux/GIL, GCC computed interpreter, boxed-slot shapes

## Contract

At10:44 UTC, main merge `fb5dbc0627a1` contains the qualified c4ac runtime.
Its complete6,113-entry committed projection and working bytes now match the
candidate. Eight declared DOS-text files needed LF-to-CRLF checkout correction;
Git blobs and executable modes are unchanged. Protected user files, main prose
and historical build products remain exact. The independent
[source-bridge review](source-merge-review-20261101-v1.md) accepts fresh main
validation. Two fresh main builds and42 integration methods per image remain. No main
native pass or JIT implementation is claimed.

The evidence below records qualification and earlier failures. Statements about
pending gates or the old tracing checkout describe those historical stages;
this paragraph and the active-work table govern current work.

Restore one coherent RC/cyclic-collection model while retaining the register
compiler/interpreter and eight-byte NaN-boxed frame representation. Do not
introduce a collector selector or claim that RC has reached performance parity.
Later core-wide inline heap values and their ABI are deferred.

At10:33 UTC, [independent actual outcome review](packaging-outcome-review-20261101-v4.md)
accepts packaging v4 and closes the selected candidate's remaining declared
qualification gates. Main merge, exact source/protected-file bridge and fresh
main images/smokes remain. Packaging v4 session44236 is terminal0, observed10:27:47, under its fixed
10:21:41-11:41:41 inclusive window. Its source/bridge and
[minimal helper delta](packaging-helper-review-20261101-v4.md) are independently
accepted. Regeneration returned0 with no tracked changes and completed
verification. Debug/release ensurepip+venv each returned SUCCESS86 tests/nine
standard skips, and the foreign-wheel row returned0; their row verifications
completed. The original CPU extension/refcount suite returned SUCCESS16 tests,
one standard Linux MSVC skip. All five rows and verifications returned0;
completion exists and cleanup10:27:08 confirms idle/cache removal. All evidence
retains its actual source/profile scope. No native job is active.

The [whole-profile scope decision](stackref-gate-decision-20261101-v1.md)
is adopted after independent review: global `Py_STACKREF_DEBUG` is unsupported
and unvalidated for this ordinary RC landing. Its failed partial run and all
twelve failure families remain unresolved; no upstream comparison method ran.
No checker/runtime/test change or exclusions accompany the scope decision.

Packaging feedback48281 returned1, observed10:05:50. Its first exact
`make -s regen-cases clinic` returned0 but changed one tracked generated header,
`Python/clinic/sysmodule.c.h`. The batch stopped before both packaging suites,
foreign-wheel calls and CPU extension coverage. Cleanup confirms idle and its
cache removed. Automatic reconfiguration left compiled products, Makefile and
pyconfig bytes unchanged but changed debug config.log and deleted pybuilddir.txt.
The prior strict reuse bridge therefore cannot simply be reused. The
[canonical correction](clinic-canonical-correction-review-20261101-v1.md)
is independently accepted: its fallback is unreachable, wrappers and method
bodies unchanged. Root restored the exact27-byte/mode0664 marker at10:13:22
under all seven locks and committed only the generated header as c4acfdde9d79.
The new config.log is retained. The new actual
[semantic bridge](clinic-semantic-bridge-review-20261101-v1.md) is independently
accepted; minimally rebound v4 helpers are under independent review. No fresh-object bit identity
or successful regeneration/packaging retry is claimed.

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
| Safety profiles | coordinator | ASan composite accepted; whole global StackRef profile withdrawn after independent review, failures retained. No further upstream repairs |
| Extension and generation closure | coordinator | Closed through independently accepted actual v4 outcome; no further cohort |
| Main integration | coordinator; coroutine_design_review; restart_audit; qualification_review_resumed | Source merged; independently review exact actual bridge, freeze prose checkpoint/helper inputs, then fresh main release/debug builds and42 smokes each |
| First native JIT design | coordinator | Corrected metadata capture accepted; actual seven-site candidate and second direct Fable review have a source-audited disposition. No implementation grant |

The earlier upstream control64610
returned1, observed09:43:39, within its fixed09:42:16-10:12:16 window. Its exact
[source](stackref-corrected-upstream-source-root-review-20261101-v1.md) and
[helpers](stackref-corrected-upstream-helper-root-review-20261101-v2.md)
are independently accepted. Only the take_ownership assignment's left-hand side
differs from upstream321925. This is a labelled compatibility control, not
unmodified upstream. Bootstrap again aborted while freezing four modules,
now on a closed-with-one-borrowed-reference check. No comparison method ran;
cleanup09:43:28 confirms all four native executable paths idle. Stop this
upstream repair path. A [whole-profile scope amendment](stackref-gate-amendment-proposal-20261101-v1.md)
was independently accepted and adopted through the decision above. No
comparison equivalence, exclusions or passing checker result is claimed.
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
