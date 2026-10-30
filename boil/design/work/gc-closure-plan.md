# GC correctness before interpreter reflection

Coordinator checkpoint,2026-10-30. The user asks to finish the outstanding
GC fixes, then receive a detailed interpreter report and commission design
reflection. Roughly2× is soft guidance, particularly on simple arithmetic.
The prepared Clang dispatch/frame-passing timing plan is parked.

## Landed baseline

Runtime merge `d6a23b9fe0c`, published through documentation commit
`b238276108a`, integrates qualified `5a45f9bb6a7`: ROOTS2b, bytes/Unicode
writer ownership, reader lifetime/history repair, H3 handler/storage/stats
and finalizer eligibility. All seven qualification profiles, closure and
both post-merge smoke rows pass. The landing is incremental under the
explicit exception; historical failed profiles remain failed. It does
not establish finalizer completion order or whole-GC safety.

## Repair sequence and concrete boundaries

| Work | Current state | Next evidence / completion condition |
| --- | --- | --- |
| Unsafe heap-order fallback on sweep-order OOM | Revised late-boundary patch `7f4459bf259` passes release/debug-kill focus and four affected modules; restored fallback fails the named no-sweep assertion. Original header-wrapper failure preserved | Bounded controls complete. Included in actual168 combined qualification; no standalone primary landing |
| Debug output and registry detachment over collector snapshots | Source-reviewed S3 native009fa passes rebuilt release/debug checks; test-only07af corrects six actual child verifier modes. Corrected focus, actual-child-kill ASan and copied immediate-write negative complete; earlier audit/mode limitations preserved | Coherent six-site non-detaching lock protocol with leaf holders; weakref metadata outside replaceable RAW hooks; deferred byte output; raw garbage rooted through publication. Phase oracle fails old immediate output without reproducing a native crash; included in actual168 combined qualification |
| MARKED state left by failing cycle/finalizer worklist allocation | Reviewed repair composed at168 passes three new focused cases, prior nine safety cases and whole GC in release/debug-kill; copied mark-before-journal negative fails exact next-collection recovery assertion | Checked sticky push and journal-before-MARKED rollback reviewed; bounded controls complete. Actual168 ASan build active, followed by seven qualification profiles and closure |
| Dependent finalizer/resource completion | Readiness design in progress; invocation order alone is insufficient | Explicit pending/ready/running/completed lifetimes and roots; every unfinished predecessor counted; unrelated work progresses; callbacks/errors finish before release of dependents. Close and fork contracts reviewed before scheduler code |
| Interpreter close, owner metadata, retirement/adoption and fork | Rung1 routes owners. Queue2b owner-guard/destruction-discovery draft is frozen for independent source review; no native execution or commit. Ordinary foreign mutation-event ownership and Queue2c lifecycle remain separate obligations | Freeze registration before retirement; preserve queued/running roots and heap ownership; no callback indexes another interpreter's watcher/extra tables. Quiesce while threads can still attach; never fabricate completion for a vanished fork thread |
| Python buffer-release callbacks | Five consumer families recorded in PROCESS §0.3; queue prerequisite | Exactly-once release outside sweep, exporter retained through an allocating/collecting callback, owner routing and no Python under allocation-only operations; inspect PickleBuffer, Unpickler, SRE scanner, struct iterator and testbuffer ndarray together |
| Remaining root/lifetime contracts and safe conservative coverage | Writers landed; compiler/PyMem carriers, reader operands, saved exception, detached/swap roots and raw inputs retain named debt | Resolve actual missing-lifetime cases with explicit owners/spans and causal controls. Keep conservative coverage until its replacement is proved; a clean run alone does not authorize removing it |
| End-to-end lifecycle regressions and support boundary | Old HTTP/resource error and trashcan timeout remain historical failures; platform/SDK limits explicit | Run the relevant unchanged regressions once on the repaired union, inspect failures rather than retrying green; integration gates on the actual union. Publish supported configurations and unsupported ones honestly |

The first three source boundaries can advance independently of the full
completion scheduler. Snapshot ownership is a prerequisite for building
dependency metadata over the marked dead graph. Close/fork is a scheduler
design prerequisite, not a cleanup to append after normal dispatch works.

The current sparse reachable-call proposal can still have quadratic edges.
Before committing that representation, assess whether standard object-graph
SCC condensation and completion tokens can preserve the required semantics
without materializing transitive reachability. Do not introduce size cutoffs,
owner-count shortcuts or whole-queue serialization to hide the problem.

## Validation and integration

Use the existing owned release/O1-debug builds, workers, mode checks and
validators. First exercise the new invariant and its meaningful copied
negative; then affected modules once per relevant mode. Use ASan for the
actual native lifetime boundary. Stop at unexpected failure. Broad suites
and closure gates qualify the resulting integration candidate, rather than
repeating them after every small source checkpoint. Existing cheap stale
loops remain hazard controls; there is no default20-pass campaign.

Each owner freezes source and evidence before review; no source edits in a
worktree serving an active gate. The coordinator checks composition and
receipts, integrates reviewed changes, runs required integration checks and
updates README/PROCESS/WORKLOG. Hourly status reports distinguish source,
executed validation and landing. No source-only proposal gets native credit.

## Boundary with subsequent interpreter work

Nonmoving generations, active IR liveness, removal of conservative scans,
zero-initialization retirement and moving collection are distinct changes.
They are not automatic consequences of fixing a lifetime bug. Preserve safe
coverage while resolving the correctness list above; later retire each
mechanism only with its own proof and oracle.

After these repairs, the interpreter report should state what executes
today, what remains isolated, and which costs are actually measured. Then
commission design reflection on operation families, representation guards,
value flow, calls and dispatch. The user's parameterized LOAD_ATTR example
belongs in that reflection, including generic fallback, monitoring and
root semantics. Neither parity nor2× is a mandatory numerical gate.
