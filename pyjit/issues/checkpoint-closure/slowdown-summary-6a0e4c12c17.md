# Slowdown diagnosis and narrowed plan

Current direction, 2026-11-06: retain GCC, prioritize steady-state throughput,
and use boxing as a diagnostic rather than requiring an absolute zero count.
This supersedes the earlier compiler comparison and requirement to finish all
R27 zero-box work before attempting performance changes. Historical failed
gate receipts remain valid records of their original criteria.

## What the evidence establishes

The same Python workloads execute substantially more native instructions in
the candidate. The independently accepted
[hardware review](hardware-perf-review-6a0e4c12c17.md) records:

| Workload | Candidate/upstream instructions per workload loop | Candidate/upstream cycles per instruction |
| --- | ---: | ---: |
| deltablue | 2.047 | 0.943 |
| logging_silent | 1.903 | 1.019 |
| comprehensions | 1.628 | 0.916 |

The common measured problem is extra executed work, with comparable or better
CPI. The measurements do not assign a percentage of the upstream gap to each
source mechanism. Source and native stacks identify concrete contributors:

* **Result publication and reentry machinery.** Attribute handlers publish
  owning results, retire inputs and refresh frame/executable state after
  possible Python callbacks. The generator emits repeated publication across
  consecutive flushes. Generic type/descriptor lookup is also significant:
  deltablue's generic attribute ancestry is 26.48% of sampled cycle period,
  and its shape handler is 15.27% self. These shares overlap other helpers;
  they are not all redundant work.
* **Python-call argument traffic.** The generic Python-call path duplicates
  arguments while parent homes stay rooted, binds the child, then detaches
  and retires parent inputs. Logging's general-call ancestry is 30.30% of
  sampled cycle period. Exact-argument calls already have a separate
  overlapping-frame fast path; a general binder rewrite is too broad.
* **Frame-slot traffic.** The candidate initializes register homes and
  constant slots. Ordinary thread-frame retirement already clears locals
  only after asserting temporaries are empty; generators, detached frames and
  exception unwind have separate clearing paths. The earlier full-slot
  description was too broad; the [holistic review](../interpreter-throughput/holistic-design-review.md)
  corrects it. Upstream initializes locals and clears through its live stack
  pointer. Logging has 7.22% of
  sampled cycle period in an unresolved memset-shaped libc routine directly
  under frame initialization, on both exact and generic call paths. Clearing
  frames costs additional time in both runtimes. Slots cannot be left unsafe
  for GC, callbacks, errors or suspended/exposed frames.
* **Unnecessary small C helper boundaries.** List iteration calls an
  out-of-line two-load value getter and spills/reloads interpreter state
  around it. Upstream performs its corresponding item read inline. This is
  concrete but small: the getter is 0.82% self in the comprehension capture.

Dispatch already uses real tail jumps. A register-IR slot is a memory home,
not a machine register; fewer VM words do not guarantee less native work.
The [source review](steady-state-source-review.md) preserves exact source,
assembly, ownership constraints and the historical compiler hypothesis. The
two-compiler experiment is no longer selected.

These are instructions **per workload**, not per bytecode. The two runtimes
execute different instruction sets and specialization mixes. A fair next
diagnostic counts dispatches and matched semantic operations separately from
release timing, then normalizes native instructions by those counts. We have
not yet measured an exact native-instructions-per-dispatched-op ratio on this
source. Do not present the table above as that ratio.

## What the unfinished PyValue work means

Core storage and interpreter values use the universal format, and release
and debug suites each pass 54,351 tests. Remaining object API callers,
constructor parsers and extension adapters still materialize inline values
as objects. The rollout reached a functional baseline, not complete API
migration. Ownership-sensitive ctypes/LRU work remains held by the earlier
automatic approval decisions; other remaining ports were still queued.

The census's 11,028,818 debt boxes are cumulative conversion events across
6,512 test-suite dumps, not live objects or a per-second application rate.
The overall census also counts cached-small-int results; a box event need not
allocate a new heap object. That cached count covers all classes, not just the
11 million debt subset. Accounting gaps limit completeness claims.
Largest attributed units include ctypes (2.83 million), call.c (2.28 million),
builtins (0.74 million) and functools (0.72 million). Module totals do not
identify one specific API or establish elapsed-time priorities.

Remaining bridges can hurt applications that exercise them repeatedly.
However, all 29 measured kernels have zero incremental boxes in their repeated
loops; their few nonzero boxes occur outside measured repeated-loop increments.
Finishing those cold
parsers will not directly remove the measured hot-loop instruction excess.
The pyperformance subset needs its own hot-region attribution before making
the same claim there. Value ownership/publication can cost time even when no
value is boxed.

## Execution order

1. **Measure the finite excess on GCC.** Count dispatches, common call shapes,
   argument copies, initialized/cleared slots, and publication/refresh work
   in the three regressions. Keep instrumentation separate from release
   timing. Retain accurate workload accounting; do not wait for an exhaustive
   zero-box census to start throughput work. Add only counters needed to
   resolve a specific hot-path question; existing source/profile evidence
   can already support a bounded patch without a new instrumentation project.
2. **Reduce generated overhead.** Review repeated publication/refresh and
   unnecessary helper boundaries, one mechanism per patch. Verify emitted
   assembly and compare instructions/workload and elapsed time against its
   parent. Start with high-share attribute/call paths; the list getter alone
   cannot explain broad regressions.
3. **Reduce call/frame traffic.** Design a common no-keyword varargs path
   with fallible work while parent inputs remain rooted and a non-reentrant
   ownership-transfer commit. Use measured slot/live-owner counts to select
   frame initialization or cleanup changes with explicit visibility proofs.
4. **Continue PyValue ports by workload impact.** Trace remaining conversions
   to exact callers, distinguish cached conversions from allocations, and
   rank by rate and measured time on representative workloads. Correct
   reviewed object bridges may remain; raw zero is not the success metric.
5. **Integrate and retest.** Terra medium owns bounded runtime/porting packets;
   Luna medium owns mechanical callers and documentation; Sol reviews
   ownership/design and hard debugging. Root owns shared interfaces,
   integration and measurements. Use minimal affected tests per packet,
   broader release/debug checks at coherent integration points, then repeat
   the three regressions and full matched 29-kernel/12-result comparison.
   Success is correct Python behavior and measured throughput gains without
   hidden coverage loss. No second compiler or IR encoding experiment is
   part of this plan.
