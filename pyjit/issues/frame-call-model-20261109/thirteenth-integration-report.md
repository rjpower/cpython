# Thirteenth integration: pathwise frame requirements

Committed runtime `4c9c113b4e5ab0d8d061eb1b8a75c24f5c271d3c` distinguishes
inputs requiring an owned value from inputs requiring physical NULL. This is
a cold extension of the existing frame-transfer graph and Plan, not another
interpreter or unwind engine. No hot frame ABI, instruction encoding, generated
handler or benchmark changes.

## Frame-model contract

A shared physical-transfer query certifies simultaneous COPY/SWAP transports,
STORE_FAST clearing and declared completed-FOR clearing. Unsupported outcomes,
child-overlap tails and errors forget NULL facts. Forward ingress joins
intersect facts; backward VALUE and NULL_ONLY demands remain independent.
The strict ALL projection includes both, preserving earlier adapters.

The new Plan is ordinary real-LINE PRE, inherited context, empty stopped and
target saved bindings, no incoming VALUE demand, and all other requirements
contained in NULL_ONLY. Every required home must be proved NULL at the stopped
PC and actually NULL in the frame. Warning callbacks require complete reproof;
a newer successful setter wins. Application still detaches surplus owners,
commits valid bindings and continuation, then retires detached owners.

Request allocations are bounded. The aggregate four-row budget makes eligible
PRE analysis stricter for large functions than the prior single-row analysis;
that resource limit is a qualification caveat, not a new semantic guarantee.
See [source and independent post-review](null-pre-source-checkpoint.md).

## Native validation and remaining failures

Release and debug build 117 modules each. The three-module minimal gate passes
300 tests with two skips each. It includes the three unchanged peepholer jump
witnesses, all repaired, plus new cold/warm, join-conflict, method-owner and
warning-reentry controls. The broader frame/observer gate passes **691 tests in
13 modules, one skip each**, including monitoring, generators and async cases.
Committed identity rebuilds also check 117 modules each.

The first minimal attempt failed only a new warning fixture in both profiles.
At its stopped line, deleted x and not-yet-bound checkpoint both require the
outer warning. The nested setter bound only x and correctly warned again.
The corrected fixture binds both before nesting and asserts the exact outer
two-local warning; one-warning, getter, newer-redirect and result assertions
remain. Original failed logs are retained. No runtime repair was needed.

The release census runs 726 tests across sys_settrace and Pdb: **23 trace-jump
errors and four Pdb outcomes** (two failures, two errors across backends), with
13 skips. The unchanged backward-listcomp jump now passes; the other ordinary
capabilities remain obligations. Full qualification and the historical GC
negative-control failure remain open.

A bounded read-only diagnostic executes all 24 original trace methods with
their assertions active. Its second run has one request per method and no caps,
matching the census. The first run hit a diagnostic event cap in five regex
setup paths before target execution; it is not behavior evidence for those
requests. A separate GDB read-only matrix probe confirms that two ordinary
failures classify actually NULL optional-call inputs as VALUE: across-with
current4→target21/home2, and completed comprehensions current33→target4/home5.
The latter has empty current/target save columns. This supports investigating
successful CALL input drainage in the shared physical query; it does not
by itself prove a new CALL owner contract. The first GDB script could not read
an optimized-out phase field; the successful second script reports matrix rows
and explicitly makes no phase-authority claim.

Evidence: `E11/null36-{release,debug}-build.log`,
`null36-minimal[-debug]-*`, `null36-minimal-v2-*`,
`null36-frame-observer-*`, `null36-committed-*-identity.log`,
`null36-debugger-census-release-focus.log`,
`null36-trace-capabilities[-v2]/`, `null36-call-gdb-masks[-v2]/`.

## Basic performance and counters

Unchanged kernels, GCC 15.2, JIT disabled, CPU15, hash seed0, stable identities.
The golden comparison has four balanced pairs; upstream has two. These are
basic integration checks, not the final two-campaign logging gate.

| Workload | / Golden | / Upstream |
| --- | ---: | ---: |
| logging_silent | — | 0.913965 |
| generators | 1.037690 | 1.433626 |
| coroutines | 0.948954 | 1.279977 |
| nbody | 0.982620 | 0.846876 |
| float | — | 0.818632 |
| pickle | — | 0.919044 |

The current generator regression remains; coroutine and nbody means beat golden.
Nbody has one slower pair, so do not claim every current36 pair wins. The latest
full 29-kernel screen belongs to35: 0.954489/golden and1.163732/upstream.
Cross-campaign differences do not isolate cold-analysis effects.

Six generator PMU stat captures pass at 100% event running, with complete
identity equality. Means of raw count/hot-loop values give **1.045237 cycles**,
**1.020424 instructions** and **1.024316 CPI** versus golden. Added work and
execution efficiency both contribute; no single source cause is established.
Two current generator recordings also pass capture and ELF identity gates.
They are available for location analysis, not a causal speedup budget. The
first stat invocation rejected parent affinity before captures; the corrected
CPU15 campaign is separate and retained.

Evidence: `E11/null36-repeat-golden/`, `null36-basic-upstream/`,
`null36-generator-perf-stat[-v2]/`, `null36-generator-perf-summary.json`,
`null36-generator-perf-record/`. The collector retarget changes only expected
HEAD and calibration path; independent review and host success/failure controls
pass. Workload, event and identity protocols are unchanged.

## Next work

The forward same-caught PRE follow-up is committed as
`d7abf803c607b0de3cec58d7e91ba91ff1699792`. Both profiles build117 modules and
pass136 focused tests (one skip) plus the unchanged original jump, backward
exception-owner control and two new controls. The negative fixture derives its
scratch iterator/cursor window, target body interval and backedge from actual
IR; no fixed PCs or weaker state assertions. The implementation preserves the
old backward proof and admits a forward fresh-PRE/empty-save transfer through
a normal-edge-only survival slice. Exception links remain untouched. Finally
payload roots and sibling except arms still need proof. See
[proposal](forward-same-caught-pre-proposal.md),
[peer](forward-same-caught-pre-peer.md) and
[source/post-review](forward-same-caught-source-peer.md).

The two-pair basic37 checks pass identities: golden ratios logging0.645187,
generators1.006499, coroutines0.958126 and nbody0.979884; logging/upstream is
0.907800. The generator difference is smaller in this screen but has not been
repeated at37. These are combined-image results, not an isolated effect of a
cold debugger helper. Evidence: `E11/forward37-basic-{golden,upstream}/`.

The separate CALL success-window packet has completed its finite22-route
source census and independent source review, and is applied for native checks.
Its shared query certifies only the audited vector-call geometry: local or
discard recipients, or a temporary recipient at BASE. Consumed inputs become
NULL; the new result and outside-window/child-tail homes remain UNKNOWN. Native,
VM, known-call and overlapping-child success paths support that same contract. Coroutine entry remains
a separate ingress proof. The coordinator integrates and validates merged
source; workers use an isolated source without another native build.
Full release/debug qualification, current stats, final logging campaigns,
independent cold review and the remaining generator loss keep the goal active.
Golden remains immutable and selected.
