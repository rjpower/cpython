# Interpreter throughput: calls, frames and generated handlers

Coordinator plan, 2026-11-06. Runtime starts at `6a0e4c12c17`; GCC and the
current value representation remain fixed. Correct behavior and measured
throughput decide admission. Absolute zero boxing is a diagnostic, not a gate.

## Parallel ownership

| Stream | Owner | First result | Source boundary |
| --- | --- | --- | --- |
| Calls | Terra medium, `call_frames` | Reviewed prepare/commit ownership map for positional/defaults/empty varargs and fresh kwargs | `Python/ceval.c` and assigned call tests |
| Generated publication | Luna medium, `publication` | Reviewed output-publication invariant that removes redundant flush stores | `Tools/cases_generator/ir_generator.py` and assigned generator tests |
| IR and frame architecture | Sol, `measurement_review` | Holistic review, canonical call/frame contract and bounded frame-entry recipe | Documentation and read-only source review |
| Slot validity | Independent Sol, completed | Distinguish future-read liveness from owned-reference validity; audit current cleanup | [Frame-liveness proposal](frame-liveness-proposal.md) |
| Integration | Coordinator | Source review, shared headers/generated products, serialized builds/tests/measurements | Shared runtime and builds |

Work follows proposal → independent review → implementation → focused tests →
post-review → integration. Shared source interfaces are agreed before editing.
Workers never run full suites or shared builds, and stop source edits during
integration/measurement freezes. Keep unrelated worker artifacts and earlier
results unchanged. The old empty performance-report worktree edit is unrelated.

Integration A is accepted after release/debug/stats builds and the focused
939-test release/debug profiles. Its small timing sample is mixed; see the
[A receipt](integration-a.md), and make no broad speedup claim. Integration B
is committed and accepted after three builds, focused release/debug profiles,
lint and cold review; its basic screen shows no overall gain, so a performance
benefit remains unproven. See the
[B receipt](integration-b.md). Integration C's reviewed final-publication
change is committed as `7780e32e0e2`; lint and scoped tests pass, and three
builds, cold review, the mixed basic screen and both full test suites are
complete. The functional suite gate passes. The final 29-kernel and
pyperformance comparisons are running serially; no performance conclusion is
available yet. See the [C receipt](integration-c.md).

## Design questions and decisions

Calls already use a frame stack. Exact calls transfer owners to overlapping
callee homes; the general binder retains parent owners while fallible work
runs. Improve that protocol rather than introducing a competing frame system.
Before detaching, complete possible allocation/reentry and revalidate selected
code/callable/defaults. After detaching, commit cannot fail or fall back.

Ordinary thread-frame retirement already asserts empty temporaries and clears
locals only. Do not implement that optimization twice. Entry still initializes
the slot range and fills constants. Compiler future-read liveness is not an
ownership-validity map; GC, errors, exposed frames and generators need valid
homes. Investigate reuse of already-NULL overlapping caller slots before any
new bitmap or exported per-PC ownership map.

Generated result publication must remain visible before a release can run a
finalizer. The reviewed Integration A design makes receiver retirement own the
input clear and flush, eliminating exactly five duplicate pre-retirement
stores without tracking publication state across effects. A remaining final
temporary store was separately inspected in generated code. Integration C
uses the existing `published_temporary_result` postcondition for the five
specialized attribute handlers, suppressing only the redundant final temporary
write while preserving the folded-local write. Its validation and performance
receipt is still in progress.

## Integration ladder

1. Capture a fresh frozen baseline on the selected kernels before edits/builds.
2. Integrate one coherent reviewed increment, using release compile and its
   minimal meaningful tests. Build debug where ownership assertions matter.
   Regenerate only affected products from their producer.
3. At each integrated runtime commit, stop builds and run a small matched
   throughput comparison: deltablue, logging_silent, comprehensions plus
   representative numeric/call/generator controls. Retain raw vectors, source
   and executable identity, load and exact commands. Use hardware counters
   where they answer the increment's question.
4. Send a fresh Sol reviewer the integrated source and evidence without the
   implementation rationale. Ask for correctness, clearer abstractions and
   remaining IR/call-flow opportunities. Resolve findings before the next
   dependent integration; record accepted and deferred feedback explicitly.
5. After the coherent calls/frame/generator batch and cold review, run broad
   release/debug suites once. Then run the matched 29 kernels and declared
   pyperformance subset for the final report. Repeat broad tests only when a
   later semantic correction requires them.

No second compiler, collector/IR encoding restart, or ctypes/LRU source change
is part of this work. If a proposed optimization lacks a sound ownership proof
or repeatable benefit, retain the finding and choose a smaller change.
