# Steady-state source review at `6a0e4c12c17`

The object-heavy regressions warrant interpreter work. The current evidence
supports several small experiments, led by compiler/call-boundary calibration,
thin internal accessors, and generic Python-call preparation. It does not
support blaming boxing, native scopes, or a failed tail-call transformation.

This review used the six retained profiles, their executable copies, source,
`nm -S`, `objdump`, and batch `addr2line -afi`. It changed only this document;
no compiler, runtime, test or benchmark ran. Source and builds stay frozen.
Artifacts are in the external build root's
`pyjit-link-wave1/checkpoint-closure/profiles-6a0e4c12c17-diagnostic/`.
All candidate addresses below are ELF virtual addresses in build ID
`df7d4401cc45f00e9dabead7cb8fc8f3b06bd00c`, not ASLR addresses.

## What the gap requires

| Workload | Candidate/upstream elapsed | Candidate time removal needed for parity |
| --- | ---: | ---: |
| deltablue | 1.8984 | 47.3% |
| logging_silent | 1.8067 | 44.6% |
| comprehensions | 1.4992 | 33.3% |
| 29-kernel geomean | 1.2868 | 22.3% uniform reduction |

The 12-result subset needs about 8.8% uniform reduction for upstream parity.
One redundant store cannot plausibly close these gaps. Startup RSS is not a
priority for this steady-state investigation.

The bounds below use disjoint sampled leaf shares and the formula
`speedup <= 1 / (1 - removed_fraction)`, assuming samples approximate CPU time
and other work stays fixed. They are optimistic ceilings, not forecasts or
confidence intervals. Timer sampling, about two thousand samples per case,
and code-layout changes limit precision. A PC at a branch does not establish
a misprediction, and a load PC does not establish a cache miss.

## Dispatch and compiler convention

`_TAIL_CALL_LOAD_ATTR_SHAPE` is a real tail-call handler. At
`0x352914..0x352935` it loads the next word, selects the table entry, restores
arguments, executes `leave`, and **jumps** through `%rax`. There is no growing
chain of handler calls/returns. The exact-call handler likewise tail-jumps.
Do not replace this on the premise that dispatch became an ordinary C call.

There is a different, established concern: GCC's
`no_callee_saved_registers` passes interpreter state in caller-clobbered
argument registers. Normal C helper calls force moves/spills. For example,
`FOR_ITER_LIST` saves the word, index and next PC at `0x340740..0x340748`,
calls `PyList_GET_VALUE` at `0x34074c`, then reloads them at
`0x340751..0x340759`. Clang's `preserve_none` uses registers that survive
ordinary C calls. This is the existing [A3 R1/Q2 experiment](../../design/proposals/register-ir-v2-a3.md),
not a new ABI design or a reason to reopen G2.

The retained A3 `probes/handler.sh` and `probes/tcobj.sh` use
`/usr/bin/clang-21`; that symlink and `/usr/lib/llvm-21/bin/clang` are present.
The scripts also document a required control: pass the same
`-fstack-protector-strong` and `-fcf-protection=full` to both compilers,
because their distribution defaults differ. Keep frame pointers. Preserve
the normal-ABI `enter_handlers()` trampoline and its object check: A3 R13
records a GCC 15.2 non-tail-entry miscompile when values live across the call.

**Experiment:** first check the installed compiler and the existing small
handler probes, then compare representative handler assembly. If feasible,
build candidate and upstream with matched Clang flags and dev libraries,
and compare both with the existing matched GCC pair. Keep source, ownership,
encoding and workloads fixed. This measures a compiler/convention package,
not the convention alone. Falsify the hypothesis if state traffic does not
shrink on actual hot helpers, or the matched candidate/upstream ratio does
not improve beyond run spread. No numerical benefit is established yet;
the historical toy instruction counts are not a current speed forecast.

## Thin list access is an avoidable call boundary

`Objects/listobject.c:254` defines `PyList_GET_VALUE` out of line. The retained
body at `0x18de70..0x18de7c` is `endbr64`, two loads, and `ret`. The declaration
in `pycore_list.h` does not inline it. Upstream's `_ITER_NEXT_LIST` uses the
inline `PyList_GET_ITEM` on its GIL path. The candidate therefore pays a C
boundary and spills for work that needs two loads.

**Experiment:** supply an internal inline borrowed-value accessor and use it
in the already-guarded exact-list iteration/subscript paths. Preserve the
external entry, exact-list/index guards, GIL assumptions, immediate owning
`PyValue_XDUP`, and the separate free-threaded path. Do not expose an element
pointer across callbacks or weaken ownership checks. Inspect the assembly
for disappearance of the call and surrounding spills before timing.

Comprehensions has 126/1,946 leaves in `FOR_ITER_LIST` (6.47%) and 23 in
`PyList_GET_VALUE` (1.18%). Eliminating the entire pair would cap speedup at
1.083x; eliminating just the accessor's sampled body caps it at 1.012x.
The real opportunity includes caller spills, but is smaller than removing
the entire loop handler. Reject this as an important explanation if the call
vanishes yet repeated steady-state timing barely changes.

## Generic Python-call preparation is a logging target

The specialized `_TAIL_CALL_CALL_PY_EXACT_ARGS` already uses
`_PyFrame_PushOverlapping`, moves argument ownership, and initializes the child
inline. It is not routed through `call_transaction_push_python`. In deltablue,
16 samples at `0x3632d8` (`+0x208`) resolve to `_PyFrame_SetExec` inside that
initialization; this is frame setup, not generic argument binding.

Silent logging follows a different path. `Logger.debug(self, msg, *args,
**kwargs)` calls `isEnabledFor` even when disabled. The benchmark issues ten
debug calls per loop. Its normal cached path therefore has twenty Python
calls per loop, a source-derived count that a diagnostic counter should
confirm. The observed elapsed gap is about 97.6 ns per debug/isEnabledFor
pair, not a measured cost assigned to one helper.

`call_transaction_push_python` duplicates the function and arguments while
the parent window remains owning, calls the consuming binder, detaches the
parent window into separate storage, and retires it. The copies protect
reentry and error semantics. In logging, this helper owns 168/2,006 leaves
(8.37%); `eval_frame_push_and_init_code`, including inlined `initialize_locals`,
owns 170 (8.47%). At `0x361a1c` (`+0x37c`, 13 samples), the code clears the
retirement-buffer slot; `0x361a31` (`+0x391`, 11) is its DECREF path.
`0x35b25f` (`+0xe0f`, 11) resolves to positional binding at `ceval.c:2636`.

**Experiment:** design one no-keyword, optimized Python-call fast lane for
the common `*args/**kwargs` shape. Keep the fresh `**kwargs` dictionary and
empty tuple semantics. Complete allocations and possible callbacks while
parent inputs remain rooted; only a proved non-reentrant commit may transfer
ownership without duplicate/retire traffic. Preserve the pinned selected
code, vectorcall/eval-frame hooks, argument-window semantics, recursion,
monitoring, OOM and cleanup order. Do not simply steal parent arguments into
the current fallible binder. First count this shape's calls and retired
entries; test the exact reentry/failure cases before a timing A/B.

Removing both sampled helpers entirely caps logging speedup at 1.203x;
halving their combined cost would yield about 1.092x under the model.
They contain necessary work, so neither is a promised gain. Falsify the
proposal if this shape is uncommon, the copies cannot be removed safely, or
the emitted fast lane retains the same duplicate/retire loops.

## Attribute publication and reader completion

The shape handler is 0x4b4 bytes at `0x352630`. Generated code currently emits
two consecutive `IR_STORE_TEMP_DST` calls and a final `IR_STORE_DST` for its
result. `sync_sp()` flushes before retirement; `retire_published()` flushes
again. The generator tracks an already-published temporary for a different
reader-completion path, but not this path. The machine code retains repeated
local/temp and numeric-object publication branches. This is a concrete
generator-cleanup candidate, not proof of a large elapsed cost.

The hottest deltablue PCs tell a broader story: shape guard `0x3526a2`
(`+0x72`) has 34 samples; feedback addressing `0x35267e` (`+0x4e`) has 22;
type-version branch `0x352692` (`+0x62`) has 18; dispatch argument move
`0x352931` (`+0x301`) has 23. The full handler has 301/2,026 leaves (14.86%).
Deleting the entire handler caps speedup at 1.174x, well below the 1.898x gap.
Do not infer guard failures, memory latency, or redundant-publication cost
from the function total.

**Experiment:** track the exact published output/value across flushes and
emit one temporary publication before receiver retirement, while preserving
the final local assignment after retirement. Invalidate that knowledge when
the output changes or callbacks can replace its home. Preserve canonical
homes, numeric identity, owning results, folded-local error ordering, and
publish-before-last-reference-release behavior. Validate generated output and
actual machine paths; then measure. Reject the performance hypothesis if
duplicate work disappears without a repeatable timing change. Broad guard/
feedback changes need separate measured evidence and are not authorized here.

`value_reader_complete` is another small boundary: logging has 66 leaves
(3.29%), deltablue 45 (2.22%), comprehensions 23 (1.18%). It checks frame and
output-home identity, publishes, retires operands, and refreshes after possible
Python reentry. At `0x33a079` it compares the current frame; `0x33a145` checks
executing words; `0x33a256` calls the post-retirement refresh. None is inherently
boxing. A safe fast path must prove which releases cannot deallocate and must
retain validation after actual callbacks.

A narrower cleanup is visible in `call_transaction_refresh`: after successful
frame/word/range validation at `0x3339ee..0x333a38`, it reloads and checks the
same exec/words identity at `0x333a3e..0x333a55`, without an intervening call.
An explicit success return could remove that duplicate while retaining the
existing failure path. Only six deltablue leaves and one logging leaf lie in
that duplicate block. Treat it as a small cleanup, not the main speedup.

## Frame traffic and low-priority hypotheses

Candidate initialization NULLs all register slots and fills constant slots;
upstream initializes only `co_nlocalsplus`. Candidate `clear_slots` scans all
slots and NULLs each before closing; upstream clears up to its live stack
pointer. Logging has 153/2,006 leaves in frame clearing (7.63%), with many at
`0x3b8d94..0x3b8dcc` in that scan. Upstream also spends substantial leaf time
clearing frames, so this is not all excess candidate cost.

**Experiment:** first count initialized/cleared slots, live owners, NULL
temporaries and constant slots per hot code object. Consider a normal-return
cleanup specialized to proved live owners only if the data warrants it.
Keep the conservative error, suspended-generator and exposed-frame paths;
preserve the NULL invariant, overlapping windows, finalizer visibility and
reverse release order. Do not skip scans solely because a test passed, or
introduce general liveness machinery before measuring this finite cost.

There are zero sampled leaves inside `_PyNativeScope_Begin`, `_PyNativeScope_End`
and `call_transaction_monitor` in these three candidate captures. Their guards
and callers can still cost time, and zero samples is not zero cost, but the
evidence does not rank those bodies ahead of call/frame/attribute work.
Keep callbacks and observer protocols intact. Pure-handler PC-store elision
also needs a generator effect/observation proof and a separate measurement;
it is not justified by simply noticing one store per handler.

## Next bounded sequence

1. Finish calibrated, size-bounded PC/inline-source accounting and count hot
   call shapes/slot traffic separately from elapsed timing. Keep external
   samples opaque and conserve the original denominators.
2. Resume A3 R1/Q2 with matched compiler/hardening controls; independently
   remove the demonstrated thin internal list call boundary.
3. Propose the logging generic-call fast lane and the attribute publication
   cleanup as separate ownership-reviewed packets. Integrate and remeasure
   one mechanism at a time; do not combine speculative gains.
4. Use the resulting profiles to decide whether frame cleanup or other guard/
   feedback work merits a packet. Repeat the three regressions first, then
   the full matched kernels and declared subset before claiming broad parity.

These experiments preserve the interpreter/optimizing-SSA separation and do
not restart G2, collector work, or the held ctypes/LRU implementation. Actual
runtime edits and integration require the coordinator's reviewed packet;
this document supplies source-backed hypotheses, not an implementation diff.
