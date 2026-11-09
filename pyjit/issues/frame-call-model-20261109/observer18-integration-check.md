# Semantic, arena and observer integration checks

The active runtime remains `6ef45e391aa` plus the uncommitted next increment.
Release and debug `observer18` each build and import-check 117 modules. This
is an engineering diagnostic, not qualification or a performance promotion.

The release semantic/arena/source-reentry group passes 37 tests. Across the
six-file observation diagnostic, 219 tests run with two known debugger owner
errors (`test_monitoring` and `test_code`); source callback reconfiguration,
completion LINE, static line ranges and malformed completion contracts pass.
The separate nine-test discard module passes. The wider release run covers
919 tests: 13 modules pass, `test_ir` retains its two debugger jump errors, and
one coordinator selector was wrong (`test_ir_discard`); the correctly named
`test_ir_discard_completion` is the separate passing retry. Preserve both runs.

Debug covers 632 tests, with 13 modules passing, including dirty arena reuse,
source-error reentry, discard, generators, coroutines, frame escape and GC.
Two modules abort at the instrumentation version/mask sanity assertion:
`test_ir_boundaries` and `test_ir_completion`. That defect must be diagnosed
and repaired before the next engineering timing. Existing assertions remain.
No full suite or new timing has run for this increment.

## Source-error reentry

The independent [post-review](semantic-arena-post-review.md) found that a
thread-global pending source recipe could be overwritten by a nested callback
error during completion retirement. The coordinator reproduced an uncaught
outer exception using ordinary compiled Python, without malformed metadata.
`observer17` passes all four cold/warm control/nested cases after moving the
recipe into the evaluator activation; `observer18` passes its permanent test.
See [proposal and reproduction](source-error-reentry-proposal.md).

## Static line iteration

`observer17`'s future-row filter repeatedly sought through the location table.
The large `NameLimitTest.test_method_call_store_delete_augassign` worker was
stopped after several minutes. All 209 perf samples were in `lineiter_next`,
with annotated locations in the repeated location-table walk and zero lost
samples. The failure and worker output remain retained.

The bounded diagnostic alternates attribute assignment and an eliminated
literal, then times only `list(code.co_lines())`:

| Pairs | Executable words / rows | Before (seconds) | After (seconds) |
| --- | --- | --- | --- |
| 40 | 82 | 0.000146 | 0.000023 |
| 80 | 162 | 0.001010 | 0.000022 |
| 160 | 322 | 0.006899 | 0.000042 |
| 320 | 642 | 0.055491 | 0.000092 |
| 640 | 1282 | not run | 0.000158 |
| 1280 | 2562 | not run | 0.000487 |

The replacement sorts and filters cold candidates once, using a forward
location cursor, then merges two monotonic streams. Runtime source-event order
is unchanged. The previously slow NameLimit method and exact relaxation-event
test pass together in 1.25 seconds. These small, unpinned timings demonstrate
the inspection regression and its repair; they are not workload throughput
claims.

## Documentation

The five canonical IR, interpreter, frame, observer and architecture pages
now describe the active model and identify its remaining work. Six Mermaid
diagrams render successfully and were inspected. The first render exposed a
reserved node identifier in the interpreter flow; it is corrected. Historical
qualification and golden identities remain separate.

## Retained evidence

All native logs, commands and diagnostics are under build root
`pyjit-link-wave1/frame-call-model-20261109/`: `observer17-*`, `observer18-*`,
`observer18-debug-*`, `observer18-integration-*`, `observer18-discard-*`, and
`design-diagrams-18-v2/`. The earlier `semantic16b-construction` scan accepts
2,151 valid library files with zero constructor rejections; 11 intentionally
invalid syntax fixtures are recorded separately. The remote inspection module
passes 72 tests. No failed receipt has been relabelled as a pass.
