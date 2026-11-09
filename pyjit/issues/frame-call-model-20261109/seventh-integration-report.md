# Seventh integration: committed jump frame states

Runtime `417a8bd6e2e3b32dce5ea2f052a4791a6bdb8e79` is independently reviewed
and integrated. It remains an engineering candidate; golden is unchanged.

## Model repair

An observer now names its reported source, immediate physical continuation,
public event and actual owner edge separately. Generated facts and execution
use the same explicit edge. Iterator retirement can report a FOR right-branch
event while its own instruction continues by fallthrough. Its detached iterator
stays C-owned through observation and retires with the callback error preserved.

Legacy tracing can emit a Python LINE callback from a monitoring JUMP callback.
The setter now uses the positively certified committed jump state and existing
restoration Plan. An exact scope/site/generation/continuation check and zero-I/O
contract prove the site's rows survive the transition. New cancellation stays
bounded to inherited exception context and targets without incoming scratch;
existing stricter owner recipes retain their previous scope. Warning callbacks
require fresh authority and plan checks. Source-only and yield setter authority
remain separate work.

The unchanged comprehension each-LINE test now passes. A bounded 84-row probe
on each profile confirms all 18 previously refused list/set/dict cold/warm stops
commit. No event-list or positive-test expectation was weakened.

## Executed checks

Release/debug each check 117 modules, pass 343 focused tests with one skip,
and pass five selected generator methods. The targeted twelve-test gate passes;
its first failure was a fixture that used a generic `any` call and emitted no
POP_ITER. The corrected generator-expression fixture retains the instruction,
callback-error and eventual collection assertions. Actual generated comparison
changes only three new instrumented providers; 262 ordinary and the prior ten
instrumented terminal bodies remain byte-identical. Ownership probes leave no
survivors after collection from 660 values per profile.

Both wider runs execute 999 tests with 14 skips. Four modules still fail:
monitoring (one error), peepholer (three), sys_settrace (**24 errors, down from
38**), and pdb (two failures/two errors). Full suites, current stats, API lint
and broad performance qualification remain open.

Evidence: `build/pyjit-link-wave1/frame-call-model-20261109/ownership30-*`
in the python-boil-the-ocean checkout. See the
[source post-review](native30-provider-consumer-post-review.md),
[result review](native30-result-review.md) and
[frame-exit audit](frame-exit-scratch-owner-source-audit.md).

## Performance

GCC 15.2, JIT disabled, CPU 15, unchanged workloads, balanced process order,
two pairs per campaign and stable pre/post identities. Ratios are candidate /
control elapsed time; lower is better. These short screens are not final parity.

| Workload | Control | Geometric mean | Pair range |
| --- | --- | --- | --- |
| logging_silent | upstream | **0.864255** | 0.859844–0.868689 |
| logging_silent | golden | **0.618457** | 0.616770–0.620149 |
| nbody | golden | 1.048030 | 1.025393–1.071165 |
| pickle | golden | 0.978917 | 0.978660–0.979173 |

Evidence: `seventh-engineering-{upstream,golden}/`. Do not combine campaigns
or assign throughput changes to cold observer repairs. Current retained PMU
and profiles still belong to c142, as specified in the
[sixth report](sixth-integration-report.md): nbody had 3.33% more instructions
and 4.09% more cycles than golden; logging had fewer cycles than upstream.

## Next bounded changes

The canonical-home duplication optimization is now being implemented under a
fresh runtime assignment. It reuses the existing canonical-result emitter fact
for the two exact local-duplication producers; it must preserve numeric
constructor and observer bodies. Host and native checks, assembly inspection
and matched before/after measurement precede integration. The 417 executable
is retained for that comparison with explicit shared-module identity checks.

Actual phase probes identify the next correctness cases: monitoring stops after
a nonneutral conditional result is committed; the peepholer cases stop at real
PRE LINE wrappers and target a path with a NULL receiver requirement. The
proposed exact-edge view and typed target requirements must use existing
transfer facts and all-alias checks, rather than broaden event-name authority.
Retained scope roots, caught exits and pre-suspension yield need their reviewed
semantic contracts. Complete coherent full qualification, the unchanged broad
screen and both final logging campaigns before declaring the goal complete.
