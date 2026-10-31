# Slot-base prototype: first native feedback

Held, 2026-10-31. Source review and a concrete helper manifest precede execution.
This plan grants no compiler or target invocation. Preserve the rejected
constant-operand source and images; the comparison is the maintained baseline.

## Question and finite scope

Does handler-local slot derivation preserve every entry/root contract, and does
the actual GCC O3 evaluator remove the baseline return/yield cache transport?
Propose one 45-minute feedback batch from activation, including builds, focused
execution, emitted-code inspection and cleanup. Stop at the first unexpected
failure; preserve it before any separately scoped correction. No timing or
broad qualification belongs to this batch.

Use the existing reviewed isolated build/gate/worker/observer protocol, both
machine slots and each actual build lock. Rebind exact source, configuration,
compiler, standard library, extensions, native products and case selections.
Protect existing images. Freeze all helper changes and actual discovered IDs
before activation; a descriptive module name is not a coverage receipt.

## Ordered checks

1. Review the frozen source, generator tests and precise-root checks. Verify
   every ordinary handler, nineteen predicted joins and five common labels;
   preserve all original frame/IP/window operations and twenty-six reloads.
   Confirm tail handlers retain their existing parameter and no new opcode,
   metadata, format or feature flag appears.
2. Build fresh GCC O1/Py_DEBUG with the established frame-pointer, zero-init,
   GIL/mimalloc, JIT-off/nonstats/no-PGO-LTO policy. Preserve effective flags,
   module/import results and applicable linked root/inline/effect audits.
3. Execute the complete frozen `test_ir` and `test_generated_cases` selections
   OFF/N0. Include actual pushed-frame exception/result, instrumented frame,
   generator-local and each-resumer IDs from the source. Run `test_generators`,
   `test_coroutines`, `test_asyncgen`, `test_sys_settrace` and `test_monitoring`
   OFF/N0 for return, throw/close, unwinding, f_lineno and monitoring entries.
   Include `test_dis` and `test_code` to check the unchanged encoding contract.
4. Run the six existing registered stale-root controls, one hundred iterations
   each, on debug OFF and debug KILL, using their actual accepted selector and
   checker protocol. Rebind exact IDs, useful assertions, mode/extras/stress
   observations and iteration count; retain expected injected child failures
   without crediting them as normal parent passes. Preserve the existing
   focused GC/OOM entry controls selected by exact IDs in the ready manifest.
5. Build fresh matched GCC O3. Run the exact IR pushed-frame/resumer controls
   and registered stale-root controls OFF; source review and debug execution
   do not establish release compiler behavior.
6. Compare actual O3 baseline and candidate handler ranges: YIELD_VALUE,
   RETURN_VALUE and RETURN_GENERATOR normal exits, plus the six existing
   common control entries in the research inventory. YIELD_VALUE overlaps:
   bind eight unique handlers, with nine comparison roles. Follow fast and
   fallback edges. Record outgoing cache stores, receiving frame-base loads, dependent
   loads, other spills, frame/IP residency, dispatch and evaluator text. A
   smaller function alone neither establishes work removal nor predicts speed.

N0 omits the stress environment variable. Require actual executing worker and
child identities, precise-root observations, zero unwind fallback and terminal
idle cleanup. Report-only unpinned visits remain diagnostic. Command ceilings
are downward-clamped to the batch deadline with a cleanup reserve; a polling
timeout never authorizes a restart. The concrete manifest must show feasibility.

The initial regular GCC feedback does not claim tail, switch, free-threaded or
whole-suite execution. Entry-guard host checks cover generated backend intent;
applicable final backend qualification remains mandatory for an accepted source.

## Subsequent performance decision

After actual correctness and native review, prepare a separate quiet elapsed
comparison using the existing four kernels and fixed counts: coroutines 4,
generators 2, unpack_sequence 4000, spectral_norm 1. Use two rotating-order
rounds and five samples per source/workload/round, exact matched images and
origins. Freeze commands and the fresh/serial sampling policy before execution.
The ready quiet plan must also predeclare a spread/noise-adequacy rule and
retain every raw vector; two favorable point medians alone cannot accept a
comparison whose spread makes the claimed benefit inconclusive.

Coroutines is the primary call/return consumer: require at least 1% lower
median elapsed time in both rounds. Generators is the yield/resume consumer;
unpack and spectral_norm are common-dispatch controls. A greater-than-2%
regression repeated in both rounds on any of those three rows requires a
bounded causal explanation/correction before acceptance. Do not add samples
after seeing the scores. Text and instruction counts are diagnostic, with no
hard size veto. Pin the existing unavailable hardware-counter receipt; do not
run another permission probe or substitute sampled counters.

Native behavior, emitted work and elapsed results receive separate independent
reviews. A useful source may join the one coherent GC qualification candidate
only before that source freezes. A failed source is rejected or corrected in a
new explicit scope; no runtime switch preserves it as an alternative.
