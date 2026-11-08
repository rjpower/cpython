# Diagnostic counters for architectural comparison

2026-11-08. Design only; runtime base `0953757f4e7`. No route is selected by
this document. Counters belong in a disposable Py_STATS-only diff based on
the frozen R5 source, separate from the held P1 diff. The coordinator owns
instrumentation, review, build, capture and retirement.

## Start with what exists

`Python/ceval_macros.h:25–38` already counts handler entries and pairs under
Py_STATS; `Python/pystats.c:171–215` emits named handlers, counts and pairs.
Use these for dispatch, `LOAD_FAST_BORROW`, `POP_TOP`, shape/dict and call
handler frequencies. Pair counts are previous-handler transitions, including
frame switches, not a per-code path trace. Existing `CallStats.frames_pushed`
and object/incref/decref totals (`pystats.c:221–250`) provide broad cross-checks,
not per-frame attribution. Reuse warmed IR to classify surviving moves and
line barriers; inspect actual release GCC for class tests, PC/home stores,
spills and reloads. No new global dispatch or generic refcount counters.

All proposed counts are global for the interval, including harness and library
work. Handler names and warmup establish possible sites, not code attribution.
For target-specific conclusions either prove and subtract a separately measured
empty-harness contribution and check linear scaling, or add a separately
reviewed code-identity-filtered diagnostic pass. Filtering itself changes cost;
never silently label a global count as a logging-function count.

## First diagnostic: call routes

Reuse the route definitions and field/print/merge scheme in
[call-audit.md](call-audit.md), but implement just the following first. Add
`uint64_t` fields to `CallStats` in `Include/cpython/pystats.h`, increment via
`CALL_STAT_INC` (`Include/internal/pycore_stats.h`), and print and merge in
`Python/pystats.c:print_call_stats` and the existing CallStats aggregation.
Use stable `Call route:` labels. Do not change guard evaluation order or run
extra Python; use conditional compilation for split-outcome accounting.

* `Python/bytecodes.c:_INIT_CALL_PY_EXACT_ARGS`: count successful exact calls
  separately in the existing thread-overlap and other-owner branches; these
  cover both handler families using the op. Regenerate the IR cases from the
  producer. Their sum is exact accepted calls, not all exact handler entries:
  earlier guard exits and instrumented variants differ.
* `Python/ceval.c:call_transaction_overlap_python`: increment `enter` at
  function entry, then exactly one final outcome at each actual return:
  `success`, `decline_kind_monitor_owner` (include free-threaded compile-time
  decline), `decline_initial_signature_or_capacity`, `decline_revalidate`,
  or `error`. In cleanup, count the final return after its refresh: cleanup
  refresh failure overrides a provisional decline. For success only, count
  independent `kwargs`, `varargs` and `defaults` flags. Do not derive error
  from an intermediate status before cleanup.
* `call_transaction_push_python_fallback`: count entry and exactly one final
  `published`, `declined` or `error` immediately before each final return,
  after cleanup and final refresh. Separately count mutually exclusive input
  kinds vector/keywords/EX at entry. This helper runs only after an overlap
  decline, not after an overlap error.
* `call_transaction_invoke`: count entry, preparation/monitor error returns,
  Python-push success or error at the existing `status <= 0` return, and
  native entry at the final `call_transaction_native` call. These are terminal
  selections, not classifications of native outcome. Count `instrumented`
  as an independent entry flag; optionally count expanded-method success at
  `call_transaction_expand_method`'s actual success return, noting that it
  may also be called by generated preparation outside `invoke`.

Require `overlap_enter = success + all declines + error`,
`fallback_enter = published + declined + error`, and kinds sum to fallback
entry. For the same interval `fallback_enter = overlap declines` if all
counters cover both GIL configurations and all exits; `invoke_enter =
preparation_or_monitor_error + python_push_success + python_push_error +
native_entry`. Check each equality rather than normalizing away mismatches.
The success flags overlap and are not additive. Entrances from exact direct
calls bypass the transaction; no equality equates them to transaction calls.
Instrumented and generator flags are orthogonal. Separate allocation attempt
and refresh counters are only needed if outcome/kwargs counts cannot answer a
specific question; allocation failures and refresh failures must then remain
distinct, including short-circuit `kwdict == NULL`.

## Second diagnostic: focused phases and frame work

First map `LOAD_ATTR_SHAPE`, `LOAD_ATTR_METHOD_WITH_VALUES`, the actual warmed
dict subscription and call/return generated cases to their producer expansions.
Use existing handler counts as denominators. If release GCC still contains
candidate state traffic, instrument only those generated paths, with a stable
counter identity per phase site rather than a handler-name special-case table:

* In `Tools/cases_generator/ir_generator.py`, at the emission of
  `frame->instr_ptr = next_instr` (around line 3694), count executed PC
  publications for the selected handlers. At `reader_reload` and
  `call_reload` (2522–2533), count emitted reload-group executions separately;
  call_reload includes reader_reload, so report nesting rather than add them.
  At the actual result/destination store emitted by `IRStack.store` and at
  `release_displaced`, count home-store and non-NULL displaced-release
  executions. Inspect the regenerated code to confirm each increment is at
  the executed store/release and that conditional paths are covered. Count
  source-level events, not machine instructions; GCC may eliminate or move
  the corresponding work in the uninstrumented build.
* In `Python/ceval.c:value_operation_refresh` and
  `call_transaction_refresh`, count entries and success/failure at their
  actual returns; the latter is nested inside the former, so do not add their
  totals. Count the separate `call_transaction_publish_pc` assignment at
  its actual store. A release or provider call is a *possible* escape; these
  counts cannot say whether it invoked Python. Count an actual callback only
  at an independently identified callback boundary, if the selection question
  requires it; do not infer it from a mortal refcount or release alone.
* For frame placement, in `_PyFrame_Initialize` count initialized frames,
  sum `nslots - null_from` for NULL-tail stores and sum `co_nconstslots` at
  `_PyFrame_FillConstSlots`; in `_PyFrame_ClearExceptCode` count thread,
  non-thread and frame-object-take-ownership outcomes, and at the actual
  `clear_slots` call add the number of slots scanned. These are slots visited,
  not owners released or callbacks. Include trampoline separately if needed;
  it initializes its own slots outside `_PyFrame_Initialize`. A frame may
  outlive the measurement interval, so pushes and clears need not balance.

This is a conditional second pass, not permission to instrument every
handler or build a permanent profiling framework. Track each phase event's
entry/branch partition where relevant, compare counts with handler entries,
and explicitly state nested, asynchronous or cross-interval exceptions to
reconciliation. Counters cannot recover actual machine-store count, per-slot
ownership, object lifetime or observer correctness. Inspect releases and
reentry semantics before claiming a candidate is safe.

## Comparison and capture

| Alternative | Discriminator | Limit |
| --- | --- | --- |
| A: call entry | Exact vs overlap/fallback/native outcomes; successful kwargs/default flags | Post-allocation work and GCC duplicate checks still require source/disassembly |
| B: publication phases | Executed publication/reload/store and potential-release counts on hot prefix | A possible escape is not an actual callback; stats alters generated code |
| C: equivalence/window | Existing move handler counts and warmed IR reason for each transfer | A count cannot establish a transfer is semantically removable |
| D: operand classes | Existing hot handler counts, inspect release GCC; optional selected-site class split only if branch survives | Static branch does not establish dynamic taken mix; no new word layout inferred |
| E: dispatch/fusion | Existing entries/pairs, POP and call/return frequencies plus GCC and footprint | Pairs across frames are not automatically a fusible sequence |
| F: frame/constant work | Existing frames pushed plus selected slot fill/clear counts | Frame lifetime crosses interval; a visited slot need not be mortal or occupied |

Capture the unchanged target and representative call/non-call controls with
identical fixed loops. Record source commit, complete diagnostic diff/hash,
executable/build identity, compiler/configuration, imports and actual target
function identities. Start with the approved R5 configuration that disables
natural method/tracing JIT admission as well as checking
`not sys._jit.is_enabled()`; the latter alone is insufficient (see
`bench/results/checkpoint-20261103-triage.md`). Warm specialization before
clearing, validate silent output, clear stats, enable immediately before the
fixed loop, dump while enabled, then disable. `sys._stats_dump()` returns false
when off and clears on successful dump. Account for on/off/dump wrapper work,
record errors and raw output, repeat for stability and input scaling. Avoid
profiling/monitoring unless it is the specific control, since it changes routes.

Py_STATS adds increments, an extra tail-call state argument and cache traffic;
additional counters change register pressure, layout, specialization timing and
possibly scheduling. Counts are diagnostic, not timings or a savings estimate.
Compare predicted removable work to *uninstrumented release* GCC and later
balanced target/control timings. After capture preserve raw stats and diff,
remove the complete diagnostic diff and generated artifacts from the timing
source, rebuild release from the selected source, and verify source and ELF
identity plus no counter instrumentation before performance admission. Do not
merge these temporary counters or use the stats binary for parity claims.
