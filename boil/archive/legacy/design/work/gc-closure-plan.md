# Supported GC closure and the next interpreter checkpoint

Coordinator checkpoint,2026-10-30 after integration75e8. The declared Linux
main-interpreter GC milestone is qualified, integrated and checked after
merge. Roughly2× interpreter speed remains soft design guidance, especially
on simple arithmetic; it is not a numerical gate. The [matched29 elapsed
diagnostic](pre-I-post-GC-timing.md) completed at0.824299 upstream/fork;
independent review passed. No new interpreter ABI is implemented.

## Integrated identity and evidence

Qualified source **4da292b1294280da838eae8b886aca7103f2d014** is integrated as
**75e8a1e0c01a69b0e34f667c4dac0ad837da4c68**, tree8a309fd40dd. The exact
integration delta is15 declared project Markdown paths. All other tracked
bytes match4da. Runtime library/tests remain **dac6**; ordinary GCC O3 and
O1/Py_DEBUG native products were built at **3c6**; ASan/Py_DEBUG Og was built
at **f945**. The one-file4da traversal metadata correction changes no C,
Python runtime/test or checker algorithm. These are explicit source bridges,
not a claim of new-head builds or relocated runnable payloads.

- Final qualification index **12f1ea31** binds276 absolute artifact hashes.
  The [independent LAND review](reviews/gc-main-only-final-landing.md) checks
  raw outcomes, source/native associations and the bounded support limits.
- Both fresh normal/kill O1-debug fulls pass **51,240 tests/3,739 skips**,
  all564 module outcomes accounted for, with zero fallback.
- Release/debug stress11 pass **3,104/148** and **3,104/113** at actualN1000,
  zero fallback. Report-only unpinned counts are not recast as zero.
- Release coverage is the declared **557 retained +7 fresh** assembly,
  51,240/3,794,462 PASSED/94 SKIPPED/8 RESOURCE_DENIED. The fresh memoryview
  row enables cpu and protects its manager race. Original uniform release
  remains nativeSUCCESS0/qualificationFAIL.
- ASan16 is **15 retained +fresh memoryview**,4,026/141, plus separate local4
  changed-path checks. Parent multiprocessing sanitizer skips do not prove
  processes-leaf execution. Unicode7/owner15 retain their actual f945 scope.
- Cext16/exact7 skips and unchanged regeneration pass at dac6. The original
  traversal batch stays failed. Corrected traversal and the previously unrun
  **19 borrowed-header receipts** pass at4da, with _tkinter/_dbm/_gdbm/readline
  explicitly unsupported. All three original588-product maps, config/tool/
  alias bindings and2,915 borrowed headers match after cleanup.
- Post-merge index **f56e0b6b** binds20 references. Release/debug off/N0 smokes
  each pass **1,935 tests**, exact80/50 skips, zero fallback/unpinned and
  verified cleanup. Their qualified code and own Lib/extensions remain in
  the serving tree; there is no primary runtime overlay.

Historical stress100k, scanner oracle, environment/cache/helper and unknown
ambient-build failures remain failures. Current normal/kill HTTP passes103/8
with SSL-only skips; GC and stress select trashcan_threads without filtering
or skipping it. This is loader/selection/module-result evidence, not a separate
success-ID journal or a passed rerun of the old100k profile. No current
ENV_CHANGED or resource unraisable was waived.

## Completed repair boundaries

| Repair | Closure |
| --- | --- |
| Sweep-order OOM | No unsafe heap-order fallback; no-sweep causal assertion and ordinary qualification preserved from safetyff6 |
| Snapshot/debug/registry safety | Six-site non-detaching lock protocol, noncreating GC lookup, libc private metadata, deferred byte-owned output and rooted raw garbage; source/control and actual union evidence retained |
| MARKED/work-stack OOM | Fallible preflight and rollback before publication, including journal growth; named copied failure and corrected diagnostic test bridge preserved |
| Main-only admission | Three public C creation entries reject nonfatally before allocation; existing capability flag and finite success-only test classification preserve main inspection, rejection and ordinary threads |
| Callback delivery | One interpreter-wide drainer; active payload/callback/raised-exception roots span reporting; nested/competing collectors enqueue and return without waiting |
| Persistent acquisitions | Readiness and full unready-closure retention; sealed struct/PickleBuffer/scanner, managed-owner Unpickler/ND, ordinary same-owner XID views and real release/count invariants |
| Ordinary SharedMemory | Automatic disposal drops references and consumes its owned fd; explicit close remains strict. Local4 in ordinary/debug/ASan and real multiprocessing/memoryview integration cover data, reclamation and resource reuse |

The [owner ledger](10a-buffer-safeguard-removal.md),
[note13 policy](../13-interpreter-queues.md) and independent source reviews
state the exact contracts. Four disjoint copied C omissions reach their named
assertions in one negative build; an old-policy SharedMemory worker reaches
its captured BufferError assertion. Those native failures remain failures,
not positive qualification outcomes. No generic cleanup slot, completion DAG,
forced export-count zero or silent DEAD/SWEPT callback suppression is needed.

## Support and separate obligations

This milestone supports Linux x86-64, one main interpreter, ordinary threads
and the shared GIL. Subinterpreters reject explicitly, not through a process
shim. Foreign/adoption, repeated interpreter deletion, parallel callback
progress and Windows persistent holders are deferred. A blocked callback
can delay other callbacks; ordinary threads may progress. Busy shutdown/fork
contracts stay explicit. Actual process exit may leave safe late state for OS
reclamation; it does not authorize normal-runtime leaks or imply safe repeated
host-returning Py_FinalizeEx.

There is no remaining supported callback/buffer-completion landing blocker.
Precision/checker/readers, PyMem carriers, detached roots and additional
platform coverage remain separately named obligations. Preserve hybrid root
coverage until its replacement has a proof and oracle. Live PickleBuffer
Python-wrapper re-export remains separate functional debt. The finite ports
do not qualify arbitrary external acquisition graphs or future NumPy ports.
Historical ASan SDK/source-path and borrowed-header limits remain explicit;
this is not exhaustive system-header or leak-sanitizer proof.

## Current work and acceptance policy

The separate owned upstream321925 build/eval-nogcse has passed configure/make
and independent build review (23c9/c518), with the accepted GCC/O3/frame-pointer,
zero-init, nondebug/nonstats/non-tail/JIT-off policy and CFLAGS_CEVAL=-fno-gcse.
The stock upstream image is preserved. Bounded correctness and both29 origin
checks pass; the completed matched29 diagnostic reuses the qualified ordinary
fork products with their true source bindings. Its uncertainty and raw vectors
are recorded separately from GC correctness qualification.

The [interpreter report](pre-I-interpreter-status-and-opportunities.md) and
[first-slice comparison](pre-I-first-slice-decision.md) provide source evidence
for clean shared improvements. H3 is landed; H1 and inert H2 remain isolated.
No numeric NOARGS ABI or other interpreter performance change is implemented
by these documents. Compare operation families, value/call/frame costs and
measured attribution before choosing the next coherent slice. Roughly2× is
soft guidance, not permission for benchmark-specific tuning.

Acceptance emphasizes live data, eventual reclamation, usable resources,
callback effects and memory growth over a declared interval. Critical rollback,
root handoff and ownership tests still need unit invariants and causal controls.
Unexpected failures stop a campaign; do not retry green, relabel old receipts,
add broad exclusions or restart unchanged ladders. Existing cheap stale loops
remain hazard controls. Full/perf-map work serializes; normal admission remains
two slots, and timing needs its own quiet reservation.
