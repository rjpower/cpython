# Selected56: interpreter performance report

Selected runtime: **883524c180e4486b47df77ee2276c9276c0ec32c**, branch
`pyjit/round56-golden`, tag `pyjit-golden-20261113-interpreter-performance`.
Golden54 remains immutable at `61015a9452d9069646fb5fede9ff963896833b89`.
Upstream is `3a440b65695615e9fd9912342b79cdd54fced0d2`.

The candidate reduces geometric-mean elapsed time **1.53% versus golden54**,
with 17 of 29 cases improving. It remains **14.11% slower than upstream** on
that mean, with seven cases faster. All six focus workloads improve in every
six-pair confirmation. This is useful incremental progress, not parity.
[All comparisons and pair ranges](benchmark-tables.md) are direct measurements.

## Selection and accepted regressions

Root explicitly overrides the broad audit's suggested “no unexplained repeatable
>2% regression” promotion tripwire for this checkpoint. An independent cold
selection peer supports the judgment under the user's allowance for mostly-better
incremental progress. The six focus wins, broad mean improvement and stronger
qualification justify landing; the losses remain unresolved performance debt.
Golden54 remains the comparison and rollback point.

Raytrace regresses **4.50%** in six confirmation pairs, range 3.88–4.95%.
The broad run also regresses spectral_norm 3.53%, nqueens 3.52%,
xml_etree_iterparse 2.70% and chaos 2.51%. These are visible measured losses,
not noise or resolved findings. No exclusive causal explanation is established.

| Focus benchmark | Six-pair /golden54 | Direct all-29 /upstream |
| --- | ---: | ---: |
| DeltaBlue | .981856 | 1.162111 |
| Generators | .951182 | 1.371818 |
| Coroutines | .990377 | 1.255482 |
| Richards | .712389 | .875120 |
| Comprehensions | .983860 | 1.337402 |
| Silent logging | .904529 | .690918 |

The campaigns differ: do not infer upstream changes by multiplying these columns.
Silent logging is already faster than upstream; formatted/simple logging remain
slower. The logging parity goal is not a claim about every logging mode.

## What landed

The [combined architecture](../../design/proposals/interpreter-performance56.md)
and [critical audit](review.md) guided four workstreams under shared value,
ownership, effect and observer contracts. Sol peers reviewed proposals and actual
changes; root generated, built, qualified and measured the composition.

- Scalar replay tracks actual saves, vacates, publications, native results,
  guards, callbacks and joins. Required-read/retirement consumers now recognize
  logical vacancy without granting physical NULL, initialization or readability.
  Current arena/producers still clear physical cells; full M4 is unfinished.
- Exact-tuple CALL_EX preparation probes before taking owning snapshots. It
  skips up to three mortal reference pairs on eligible calls. The focus six
  execute no CALL_EX sites, so this has no assigned broad speedup.
- Literal FOR/SEND/THROW native completion entries share one implementation.
  Generator recipient admission derives from the actual checked entry recipe;
  SEND's yielded recipient remains distinct from its completion destination.
- Managed SHAPE specialization executes two to four homogeneous cached receiver
  rows through the same guarded read/acquire body. No cache footprint increase
  or method/store/SLOT expansion. A separate warmed diagnostic confirms actual
  PIC use in DeltaBlue/Richards and zero selector attempts in logging.
- Checked list/tuple producers carry canonical element facts into publication;
  exact-coroutine GET_AWAITABLE moves a scratch owner instead of acquiring and
  closing another reference. Folded locals duplicate and generic fallback remains.
  Shared generator facts replace the old list-body matcher.

PyValue integration was already complete. Private numeric C results and remaining
bridges are transport/ownership work, not a second format rollout. The existing
borrow pass and copy propagation already remove many local reads. A reproducible
census of 21 files/426 CFGs found **zero eligible surviving callback-free COPY webs**
in the proposed first morphology; no speculative extra pass was added. Broader
CFG/materialization borrowing remains a separate hypothesis.

The as-built [IR](../../design/register-ir.md), [interpreter](../../design/interpreter.md),
[frames](../../design/call-and-frame-lifetimes.md),
[activation transfers](../../design/activation-transfers.md) and
[PyValue](../../design/pyvalue.md) include the selected contracts. Bitmap readers,
IRSC2 exceptional transfers and full static scratch authority remain experimental.

## Why upstream is still faster

Matched counters show **36.14% more native instructions in DeltaBlue**,
**41.11% in generators** and **30.78% in coroutines**. Their cycle ratios are
1.152042,1.340458 and1.238294 respectively. The runtime still executes more
common-path work despite direct overlapped exact calls and canonical values.
Register IR alone does not make its generated C cheaper than upstream.

Current generator flat samples concentrate in SEND_GEN 31.82% and
YIELD_DELEGATED 13.07%; terminal generator completion is 1.90%. Reworking only
terminal completion cannot plausibly close the whole generator gap. Coroutine
samples spread across exact calls 10.69%, completion 7.39%, deallocation 7.02%,
SEND_GEN 6.01%, clear 5.80%, creation 5.76% and GET_AWAITABLE 4.02%. DeltaBlue
retains exact-call 11.36%, PIC-read 9.31% and method-read 6.41% work. These are
physical-symbol flat samples, not additive removable budgets or causal speedups.

The code still pays admission/binding/recipient and owner maintenance at calls;
resume/yield use separate state, frame/exception links and recipient boundaries;
creation copies bindings and initializes destination scratch; retirement protects
result/code/storage and handles reentry. Those operations are evidence-backed
optimization targets. Their individual avoidable fractions are not yet measured.
Upstream's combined evaluator symbol prevents a direct flat-bucket subtraction.

A historical82e coroutine profile is resolved more precisely: all 109 samples at
libc offset0x19846d (4.3306% sampled period) unwind directly to
`_PyFrame_CopyCreatedGenerator`. Matching libc/build IDs and retained native
assembly identify its zero-filled destination scratch. This is real work, but
not enough evidence to blame all slowdown on clearing or predict a 4.3% gain.
Current883 also samples that libc address at 4.89%; its callchain has not been
independently attributed. Keep historical and current identities separate.

## Regression diagnostic

A bounded three-pair PMU comparison against golden54 gives:

| Benchmark | Instructions /golden54 | Cycles /golden54 |
| --- | ---: | ---: |
| Raytrace | 1.020394 | 1.039664 |
| spectral_norm | .972709 | 1.024014 |
| nqueens | 1.015206 | 1.039182 |

All 18 commands pass with stable identities. Six matching profiles also pass.
Raytrace/nqueens execute more work; spectral_norm executes fewer instructions
at higher average cycle cost. These observations reject one universal “extra
clearing” or “extra dispatches” explanation, but do not isolate layout, spills,
branch cost or instruction dependencies. spectral_norm's main loop has no indexed
reads, so the stronger list/tuple guard cannot directly explain every loss.

Static/native review finds the new element projection keeps its known-object
fact, yet its stronger preguard adds roughly four successful-path instructions
and one conditional branch compared with golden's pointer-space guard. List
getter spills remain. That is a plausible cost for indexing-heavy workloads,
not a proven whole-program cause or permission to weaken semantic guards.
Profile source and counters are retained under E56/regression-diagnostic.

## Qualification

Coherent full release and debug at cb3f0b00ef2 each execute **55,023 tests**.
Release reports588 modules OK; debug 587. Neither crashes. Release has only six
`test_sys_settrace` debugger-jump errors; debug has those same six and one stale
LLTrace assertion. Both sets of six are exact subsets of golden54's nine errors
per role. They remain observation limitations, not newly quarantined tests.

The final 883 commit changes only the LLTrace fixture: it checks actual discarded
CALL_INTRINSIC_1 execution instead of stale POP_TOP text. Its runtime source tree
is byte-identical to cb3. All three debug LLTrace methods now pass; release skips
those debug-only methods. Final verbose484-method sys_settrace checks per role
reproduce exactly the six baseline errors. No green full-suite claim is made;
the entire suite was not rerun for the final fixture-only commit.

Both final builds complete 117 modules/79 shared. The unchanged three admission/
retirement regressions,53 core methods and70 final fixture/source methods pass
per role at their recorded integration heads. Full regeneration leaves runtime
products byte-identical. Five bootstrap images, 27 compile probes and the freezer
build pass at 883. Only the existing GDB PrettyPrintTests class is quarantined.
Qualification is GIL-only; free-threaded performance is not established.

The full gate found and fixed a real NO_HOME/SEM_NULL mismatch in five scalar
consumers. It also closed stale magic/table, exact-call-role, frame-size, unknown-
unary and generated-source fixtures; optional observer metadata now conservatively
denies authority when absent. Negative controls remain. Failed attempts and their
repairs are retained, rather than reported as passes.

Retained final native ELF: E55/perf56-final56-native/python, SHA256
`2f46bcc126b8857095bd8b25a9256ad0f76fe65667e023e96edd4d7acd21f4cc`.
E56/qualification56 owns source equivalence, debugger-baseline comparison and
commands; E55/perf56-closure56-* holds the full logs. See [handoff](../../HANDOFF.md)
for absolute roots and frozen worktrees.

## Rejected experiment and next work

Inlining the list getter removes a call/six spills and shrinks its handler
831→771 bytes, stack48→16, but repeated coroutines regress 10.73%. It is reverted;
its branch/ELFs/evidence remain. Smaller generated/native code is not sufficient
for selection. Native terminal entry splitting also grows combined text despite
removing stack arguments; no isolated throughput gain is assigned to it.

The [cold review](review-integration.md) ranks resume/delegated yield, exact calls,
coroutine construction/retirement, guarded attribute hits and the next executed
canonical producer. The [plan](plan.md) assigns evidence gates and keeps a parallel
regression diagnosis lane. Count repeated operations before another broad ABI
rewrite. No further implementation is part of this checkpoint closure.

## Evidence and publication

BASE/E55/E56 and exact controls are defined in HANDOFF. Fixed CPU15/hash0,
JIT disabled, matched GCC15.2/O3/devlibs; fork/golden use five-argument tailcall
handlers and upstream computed goto. This measures the complete runtimes, not an
isolated dispatch-convention experiment. All29 campaigns use116 commands each,
confirmation 84, matched stat 24, profiles 8; every source/native identity remains
stable. Diagnostics add 18 stat and 6 record commands. Results include observed
pair ranges; they are not confidence intervals.

Canonical docs and the runtime/golden refs are pushed before guarded Loom
publication. The goal → plan → architecture/status chain owns ongoing work;
old staging pages and ten obsolete mirrors were retired with exports and git
history preserved. Sealed native evidence is unchanged.
