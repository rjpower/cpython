# Validation contract

Use finite, source-frozen checks that answer the change's question. Cheap
experimental feedback and qualification of a coherent landing are different
scopes; record the selection and why it covers the change before execution.
Documentation-only changes use document/link/diff review, not native suites.

## Feedback and landing gates

| Scope | Required evidence |
| --- | --- |
| Build feedback | Effective compiler/configuration, compile/link, imports/module checks and applicable root/effect/inline audits |
| Focused behavior | Actual executing test IDs, useful data/liveness/resource/error checks, modes and cleanup |
| Runtime milestone | Release full suite and root-counter validation |
| Landing closure | O1/Py_DEBUG full suite, `test_cext` with CPU resource, unchanged regeneration, traversal audit and applicable unbuilt-module syntax checks |
| GC/root/layout/evaluator change | Debug-kill full, applicable ASan selection, release/debug stress1000, compiled-root/stale/ownership and affected backend controls |
| Performance decision | Matched build/origin policy, unchanged workloads, all raw vectors, per-row ratios/spreads, relevant instruction/memory/phase evidence |

Use the existing gate/helper bodies and registered selections. The issue plan
names concrete commands, counts/protected cases, exclusions and finite timeouts.
O1/Py_DEBUG retains assertions, symbols, frame pointers and the current root
policy. Release measurements use O3 under a declared policy; ASan and Clang-tail
checks remain applicable to changes that affect those paths. Do not add a second
optimization-policy comparison merely to repeat qualification.

Run cheap traversal/effect checks before broad suites when relevant source,
fields or allow metadata changes. Reuse an exact-input early receipt rather
than waiting for closure or rerunning it without cause. Closure regeneration
must leave generated outputs unchanged. State unsupported optional modules
and borrowed-header coverage explicitly.

## Trust the execution, not its label

- Bind source, runtime library, native image, configuration and tools separately.
  Verify actual executable/import/extension origins and product aliases.
- Assert actual verifier mode/extras/stress in executing workers and relevant
  children. Configuration and inherited environment alone do not prove the mode.
- Require nonzero real selection, terminal worker/root records and protected
  cases. Preserve module states, expected skips and environment-change failures.
- Omit stress variables for ordinary N0 runs where tests inspect their presence.
  Allow bytecode writes and use fresh writable target caches on a compatible
  filesystem. Record only narrowly allowed regular cache additions.
- Treat informational auxiliary processes separately from test workers; they
  never earn worker or coverage credit.
- Preserve exact known exclusions with their reasons and scope. Do not broaden
  filters, suppress diagnostics or drop a failure to obtain a passing aggregate.
- Zero unwind fallback is a root gate. Report-only unpinned snapshot diagnostics
  keep their actual meaning; do not silently turn them into a new zero gate.

## Admission and cleanup

The machine has two existing global slots at `/tmp/boilgate-slot.N`. Use the
actual configured build lock, finite timeout/process-group protocol and cleanup
verification; never rebuild products while another command uses them. Full
suites, StackWalk and perf-map-sensitive jobs serialize because tests can remove
shared `/tmp/perf-*.map` files.

Quiet measurements use their declared CPU/sibling, slots, locks and load limit.
No competing native or bulk-analysis work runs inside the measurement window.
Retain raw rejection/failure envelopes and stop on the stated rule. Repetition
needs a named hypothesis and exposure budget, not an automatic run count.

## Changes and reuse

An evidence line names the source and image actually executed. A changed input
requires the affected checks; unchanged work may stand through an explicit
source/product bridge. Test-only or metadata-only corrections do not imply a
rebuild, and old failures remain failures. A composite must enumerate carried
and refreshed scopes instead of relabeling the original run.

Independent landing review checks the real diff, implementation/result contract,
critical negative controls and scope limits. For an integration differing only
in an exact enumerated set of project prose, prove all other tracked bytes
identical, bind the actual merge/tree and reuse sealed products for one fresh
release and one debug smoke. A runtime, test or tool change needs its own scope.

The [verification issue](../issues/verification-orchestration/status.md) tracks
shared orchestration; a valid manifest or plan display is not an executed pass.
See [measurement](measurement.md) for performance interpretation.
