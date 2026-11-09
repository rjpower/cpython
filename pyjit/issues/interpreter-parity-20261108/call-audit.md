# Logging call-path audit and proposal

2026-11-08. Runtime source `0953757f4e7`; analysis only, pending independent
review and route measurements. Coordinator owns all builds and timing. The
CPython AI policy requires detailed human review before any upstream proposal.

## Target and evidence

Choose `logging_silent`: it executes ten `logger.debug(m)` calls per loop with
WARNING level and verifies an empty stream (`pyjit/bench/internal/kernels/logging.py`).
`Logger.debug(self, msg, *args, **kwargs)` calls `isEnabledFor(self, level)`;
the latter normally returns a warmed `_cache[level]` entry. This makes a
real, short call-heavy workload, though attribute lookup and frame teardown are
also material. The final whole-campaign candidate/upstream elapsed ratio is
1.74303; selected perf cycles per loop ratio is 1.62960 and instructions per
loop ratio 1.720317. Do not confuse the latter two with elapsed time. Retained
candidate perf self shares: overlap helper 13.17%, frame clear 9.02%, method
attribute load 8.00%, exact Python call handler 4.93%, general Python call
handler 2.83%. Samples do not provide route counts, exclusive costs of source
phases, or a bound on potential elapsed savings. The sample contains no Richards
capture; Richards is useful as a broader secondary workload but less isolated
for a first call-path hypothesis. Reaching parity may need several mechanisms.

Workload-specific emitted IR has not been found in the retained evidence; the
register instruction and specialization source establish possible layouts,
not which instructions/routes actually execute in this run. Ask coordinator
to capture warmed IR for `bench_silent`, `Logger.debug`, and `isEnabledFor`,
with source/ELF identity and a separate counter run. No inference of exact route
frequency from source, bytecode, or sample stacks.

## What the sources and GCC show

* `Python/regcfg.c:792` and `:916` require consecutive temporary window homes,
  no locals in windows, and a live temporary below the window. `Python/bytecodes.c:4308`
  uses callable, self-or-NULL, and contiguous positional arguments; a thread
  frame overlaps child argument homes. `Include/internal/pycore_interpframe.h:526`
  initializes only the previously unknown NULL tail. This is already a no-copy
  transfer for those argument owners, not a reason to add a new call IR.
  Preparing owning temporary homes and enforcing canonical observable locals
  are current design choices with costs, not a proof that register IR requires
  copies in every possible design. The overlap, NULL and ownership contracts
  must hold before considering a producer change.
* `CALL_PY_EXACT_ARGS` uses guarded direct push in `Python/bytecodes.c:4374`.
  `CALL_PY_GENERAL` uses the transaction (`:4215`); generic calls can do so
  too. `Python/ceval.c:3287` expands methods and monitors when necessary, then
  checks function type, vectorcall and PEP 523 before overlap. The overlap
  helper (`:3049`) checks optimized code, no keyword-only parameters, positional
  fit/defaults, executable and stack space. It pins code, allocates an empty
  keyword dict only for `CO_VARKEYWORDS`, refreshes after allocation, and
  rechecks callable/code, vectorcall, hook, executable, space, defaults and
  self offset. It initializes defaults, empty varargs, kwargs and publishes the
  child in one no-failure commit. Fallback (`:3139`) copies owned arguments to
  the consuming binder, refreshes and publishes before releasing retired
  values; generator parents, keywords, EX, monitoring and declines need it.
* `r5-gcc/call_transaction_overlap_python.txt` shows a separate helper with
  callee-saved-register pushes and 0x68 stack bytes after initial gates, plus
  loads/spills, initial and subsequent guard blocks, frame initialization and
  variable loops. The sample at this symbol establishes that code executes;
  it cannot assign frequency to the individual branches. Header fields,
  executable owner, parameter defaults, NULL initialization and frame state
  have semantic purposes; the specific helper boundary, repeated guards,
  reloads and GCC spills are implementation choices. Monitoring and PEP 523
  checks preserve observable callbacks and must remain effective.

## Proposed bounded implementation: non-allocating overlap

In `Python/ceval.c:call_transaction_overlap_python`, retain the initial gates
and all initial argument/space checks. If `!(flags & CO_VARKEYWORDS)`, take a
no-escape path directly to the existing shared infallible commit, carrying
`homes`, `func`, `code`, `defaults`, `ndefaults`, `offset`, and `nargs` from
those checks. Do not pin/unpin code or repeat final checks on this path. The
caller already established exact function type, standard vectorcall and no
PEP 523 hook, and there is no allocation, callback, release, thread switch or
other escaping operation between those checks and the commit under the GIL.
Confirm every intervening macro/instrumentation expansion in the actual build,
including Py_STATS, before relying on that proof. `Py_GIL_DISABLED` continues
to decline. Thread owner and executable/stack gates remain unchanged.

If `CO_VARKEYWORDS`, retain code pin, `PyDict_New`, refresh, every existing
revalidation and cleanup exactly as today. All fallible/reentrant work ends
before any owner is moved; after either path reaches the shared commit, move
the callable once, initialize defaults/varargs/kwargs, set return slot, and
publish child with no failure or reentry. The frame owns executable and
callable; parent window retains all owners up to commit. Never cache values
across the allocating branch. No new general ABI, permanent switch, lazy dict,
IR or window-layout change. The change would target `isEnabledFor` only if it
actually uses this overlap route; `debug` itself needs `**kwargs` and retains
its allocation/revalidation. Route counters decide whether to implement this
candidate; this is an incremental optimization, not a parity claim.

Independent review must verify no hidden Python execution or observable state
mutation in the non-allocating span; that `func_code`, defaults and hook cannot
change under the held GIL; frame initialization/constant-slot ownership; all
failure, deopt and monitored/free-threaded paths; and generated release/debug
code. Tests in existing call/argument and monitoring suites must cover exact
and defaulted arguments, positional-only, varargs and kwargs, wrong arities,
mutated `__code__` and `__defaults__`, reentrant allocation/GC and finalizers,
exposed locals, instrumented calls, custom eval hook, generator parents, EX
and keyword fallback, and cleanup/error ordering. Coordinator runs the focused
release/debug cases, compares generated GCC branch/spill work, then paired
logging and representative call/non-call controls; reject if no repeatable
benefit or material regressions. Full suites at coherent integration.

## Separate diagnostic-only route counters

Implement only in a disposable diagnostic diff, never the release timing build.
Existing mechanism is verified: `Include/internal/pycore_stats.h` defines
`CALL_STAT_INC` as `_Py_STATS_EXPR` under `Py_STATS`, no-op otherwise;
`Include/cpython/pystats.h:109` holds `CallStats`; `Python/pystats.c:221,396`
prints and merges fields; `Python/sysmodule.c:2399` exposes `_stats_on`,
`_stats_off`, `_stats_clear`, `_stats_dump` in a stats build. Add named uint64
fields there, print and merge each with stable `Call route: <id>` labels.
Use these insertion points and mutually exclusive counts:

1. In `call_transaction_overlap_python`: `overlap_enter`; one outcome each
   `overlap_decline_kind_or_monitor_or_owner`, `overlap_decline_signature`,
   `overlap_decline_exec_or_stack`, `overlap_decline_revalidate`,
   `overlap_error_prepare`, or `overlap_success`. Split existing compound
   guard only within `#ifdef Py_STATS` accounting without changing guard order
   or effects. On success additionally count independent flags
   `overlap_success_kwargs`, `overlap_success_varargs`, and
   `overlap_success_defaults` (nargs < argcount). The last three overlap and
   must not be summed. Record initial versus post-allocation declines separately.
2. `call_transaction_push_python_fallback`: `fallback_enter`, and one of
   `fallback_published`, `fallback_declined`, `fallback_error` at its final
   outcome. Count fallback kind vector/keywords/EX on entry; generator owner
   and instrumented are independent flags. In `call_transaction_invoke`, count
   `native_enter` immediately before native call, and monitored transaction
   entries once at entry. Do not count native's returned status as a call route.
3. Exact specialization bypasses this transaction: in the source definition
   `_INIT_CALL_PY_EXACT_ARGS` (`Python/bytecodes.c:4308`), count direct exact
   success, split thread-overlap versus generator-owned, using the existing
   `CALL_STAT_INC` macro. Regenerate products through the repository generator;
   do not edit generated files alone. The bound-method exact macro also uses
   this op, so label it as both exact handler families, not just CALL_PY_EXACT.

Check `overlap_enter = success + all declines + errors` and
`fallback_enter = published + declined + error`; entry-kind subtotals sum to
fallback entry. On a quiet, isolated stats-only run, prepare and warm logging,
clear stats, turn on immediately before a fixed number of kernel loops, dump immediately afterwards to the configured pystats directory while stats
are still on, then turn stats off. `sys._stats_dump()` returns false if stats
are off and clears the interval on success. Keep
the small wrapper/on-off accounting visible, report iterations and validate
no output. The diagnostic has instrumentation overhead and must not be timed
against release. If needed, add per-code identity only in a separate diagnostic
probe; global counts alone do not establish which function took each route.
