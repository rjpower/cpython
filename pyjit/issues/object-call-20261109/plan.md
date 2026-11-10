# Object lookup and call binding

This round starts from runtime `4d666e1b482` and the
[closed frame/call report](../frame-call-model-20261109/closing-report.md).
The user requested an Astra medium review before publishing that checkpoint
to the existing golden branch and beginning the next improvements.

## Sequence

1. Review the measured priorities and survey LuaJIT/V8 lookup and call designs.
   Separate actual mechanisms from speculative layout changes.
2. Publish the frozen runtime plus canonical documentation to
   `pyjit/golden-20261108`. Preserve the old immutable tags and controls.
   Record the known tooling and frame-jump qualification limits.
3. Select two bounded lanes: canonical attribute lookup and reusable keyword
   binding, subject to the review. Prepare independent proposals and review
   their ownership, mutation, reentry and fallback contracts before editing.
4. Implement in disjoint files or isolated patches. Root owns regeneration,
   release/debug builds, focused tests, commits and serialized native runs.
5. Post-review each actual diff. Integrate and compare unchanged relevant micro
   workloads, DeltaBlue, logging and the complete 29-kernel roster against the
   frozen checkpoint and upstream. Run full diagnostics at the combined
   checkpoint, with previously documented failures reported separately.

## Design constraints

One canonical value model, one owning frame/call ABI and one selected object
storage model. Keep fast paths as guarded plans over semantic operations.
Do not introduce a second compiler, a table/shape mode switch, arbitrary
benchmark-specific cases or exact CPython finalizer timing requirements.

Investigate guarded hash-slot lookup and name interning as bounded mechanisms.
A wholesale attribute-layout rewrite requires evidence unavailable today.
Dictionary string and integer lookup paths must be distinguished in source.

## Reporting

The original packet began around 23:55 UTC on 2026-11-09, with a reporting
target of 01:55 UTC. Work resumed for the current recap/integration around
08:40 UTC on 2026-11-10. That earlier target is historical, not a scheduled job.
Report the current integration result and actual available measurements; do not
claim completion or new speedups while tests/review remain pending.
Canonical pages own state; Loom mirrors the current goal, plan, status and
designs. Preserve unrelated work and sealed historical evidence.

## Current execution and calibrated priorities

Publication is complete: runtime tag `pyjit-golden-20261109-runtime` points to
`4d666e1b482`; combined golden `cb906f9bc52` includes canonical documentation.
The new frozen release build checks 117 modules. Historical timings remain
0.934192 times the previous golden and 1.174985 times upstream across 29 kernels;
logging meets parity. The new increments have no measured performance result yet.

| Owner | Current assignment | Exit evidence |
| --- | --- | --- |
| Sol lookup lane | Canonical guarded class-value metadata; acquire/publish before receiver retirement; cache and value design docs | Producer/codegen checks complete; native mutation, shadowing and lifetime tests pending |
| Sol call lane | Cached 1–3-keyword destination plans over the existing overlap ABI; frame and IR design docs | Host generator suite 96 passed; native binder-hit and semantic evidence pending |
| Astra medium | Independent actual-diff review across both lanes | Findings, corrections and post-review disposition |
| Coordinator | Regeneration, serialized builds/tests, integration, counters and balanced measurements | Release/debug focused checks, stats hit witness, independent review, basic timing then combined qualification |

Keep initialized reusable frames, overlapping arguments and explicit result
recipients/continuations. Performance work should remove repeated semantic
preparation: cached canonical lookup, reusable binding, generator-cycle work and
value-native boundaries. Structured ownership/effect facts should become the
single source for lowering, publication proofs and generated handlers; migrate
one proof with equivalent output before broadening it.

After this integration, use measured application benefit to order generator-cycle
work and native descriptor bridges. A guarded current hash-entry hint is a
bounded lookup experiment; changing all object storage is not justified by the
current evidence. Already-interned attribute/local names do not remove repeated
binding searches. Preserve complete Python errors, mutation, observation and
owner safety; exact CPython finalizer timing is not a design requirement.

A new lifetime witness fails in both frozen runtime39 and upstream. Its list
class value exercises the actual guarded handler, unlike the initial generic
control. The new lookup ordering must pass this witness as well as mutation
coverage before integration.
