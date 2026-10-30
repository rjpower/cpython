# GC correctness before interpreter reflection

Coordinator checkpoint,2026-10-30. The user asks to finish the outstanding
GC fixes, then receive a detailed interpreter report and commission design
reflection. Roughly2× is soft guidance, particularly on simple arithmetic.
The prepared Clang dispatch/frame-passing timing plan is parked.

Scope revision,2026-10-30 09:35 PDT: support the main interpreter and ordinary
threads first. Defer subinterpreters; reject their creation explicitly in a
focused follow-up rather than emulate their APIs with processes. This is
not enforced by the landed baseline yet. Foreign ownership/adoption and
interpreter-deletion support are no longer prerequisites for this milestone.
The holistic reviews linked from README supersede the broader lifecycle and
completion-scheduler requirements below where noted.

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
| MARKED state left by failing cycle/finalizer worklist allocation | Reviewed repair composed at168 passes bounded checks and causal negative. Combined union has six profiles passed; debug stress fails only a diagnostic snapshot test allowing intervening automatic collection | Exact test-only9e isolates snapshots with assertions unchanged; one whole-GC debug/off/N1000 successor carries ten original passed modules. Original FAIL stays failed. Closure and independent landing review remain |
| Dependent finalizer/resource completion | Invocation order alone is insufficient. Generic completion DAG held after holistic review | Evaluate one drainer retaining the active callback through error reporting; competing/nested collectors enqueue without waiting. State loss of concurrent finalizer progress. Buffer release readiness still needs its own proof |
| Interpreter close, owner metadata, retirement/adoption and fork | Owner63b bounded validation and causal negative complete; source pushed. Foreign mutation/lifecycle is not closed | Defer integration and broader lifecycle machinery under main-interpreter-only scope. Audit creation APIs for explicit rejection. Separate process exit from host-returning finalization; decide fork restrictions explicitly, without fabricating callback completion |
| Python buffer-release callbacks | Five consumer families recorded in PROCESS §0.3; generic type capability held | Prefer existing per-acquisition managed owners. Prove exactly-once release, stable metadata and consumer raw-pointer/reentry checks for the concrete holder/view/exporter cycle and returned-view forwarding. Every blocked view must remain rooted with its dependencies; no forced zero export counts or skipping DEAD objects into sweep |
| Remaining root/lifetime contracts and safe conservative coverage | Writers landed; compiler/PyMem carriers, reader operands, saved exception, detached/swap roots and raw inputs retain named debt | Resolve actual missing-lifetime cases with explicit owners/spans and causal controls. Keep conservative coverage until its replacement is proved; a clean run alone does not authorize removing it |
| End-to-end lifecycle regressions and support boundary | Old HTTP/resource error and trashcan timeout remain historical failures; platform/SDK limits explicit | Run the relevant unchanged regressions once on the repaired union, inspect failures rather than retrying green; integration gates on the actual union. Publish supported configurations and unsupported ones honestly |

The first three safety repairs can land independently. Preserve their
qualification rather than restart it for the narrower support policy.
The old sparse-reachability/SCC scheduler proposals remain review artifacts,
not implementation requirements. Serialization is now an explicit candidate
policy with a stated progress tradeoff, rather than an implicit promise to
preserve every concurrency guarantee. Actual process termination can reclaim
retained late state after exit handlers and stream flushes, provided no active
callback resumes into freed state. Host-returning finalization and fork need
separate support decisions; process exit does not excuse ordinary leaks.

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
