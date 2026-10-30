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

Safety candidate9e223040 is now merged as ff6ed9d6bfb over the baseline
below. Six original profile passes plus the explicit debug stress composite
and all closure checks clear independent integration review. Final receipt
index9bacd190 binds43 fresh references and the retained86-reference packet;
both were independently checked. Post-merge release/debug smoke each passes
1,930 reported tests, exact50/20 skips and zero fallback, using qualified
native168 products and the explicit test-only source9e bridge. Smoke index
68b4bba0 and its15 references/raw/cleanup were checked independently. Primary
differs only in38 project Markdown paths. This safety increment is complete.

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
| MARKED state left by failing cycle/finalizer worklist allocation | Reviewed repair composed at168 passes bounded checks and causal negative. Combined union has six profiles passed; debug stress fails only a diagnostic snapshot test allowing intervening automatic collection | Exact test-only9e isolates snapshots with assertions unchanged; one whole-GC debug/off/N1000 successor carries ten original passed modules. Original FAIL stays failed. Closure and independent landing review complete; integrated as ff6 |
| Dependent finalizer/resource completion | Invocation order alone is insufficient. Generic completion DAG held after holistic review | Evaluate one drainer retaining the active callback through error reporting; competing/nested collectors enqueue without waiting. State loss of concurrent finalizer progress. Buffer release readiness still needs its own proof |
| Interpreter close, owner metadata, retirement/adoption and fork | Owner63b bounded validation and causal negative complete; source pushed. Foreign mutation/lifecycle is not closed | Defer integration and broader lifecycle machinery under main-interpreter-only scope. Audit creation APIs for explicit rejection. Separate process exit from host-returning finalization; decide fork restrictions explicitly, without fabricating callback completion |
| Python buffer-release callbacks | Five ordinary consumer families plus same-owner XID are recorded in the closure ledger; generic type capability held | Prefer existing per-acquisition managed owners. Prove exactly-once release, stable metadata and consumer raw-pointer/reentry checks for the concrete holder/view/exporter cycle and returned-view forwarding. Every blocked view must remain rooted with its dependencies; no forced zero export counts or skipping DEAD objects into sweep |
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

## Active landing order after review

1. **Safety union, complete.** Closure and integration smoke pass;9e lands
   as ff6ed9d6bfb, using native168
   with test-only9e: the corrected whole-GC debug stress module now passes
   152 tests/4 skips, zero fallback. Ten original passed stress modules carry
   forward explicitly; the original profile stays failed. The actual
   six-file source delta and all final receipts were reviewed; primary
   documentation is preserved. The code-identical qualified builds were reused
   explicitly for post-merge smoke. This landing fixes the three safety defects; it does
   not complete ordinary buffer release or callback-completion safety.
2. **Main-only admission, bounded complete.** Source dbb5 includes early
   creation rejection, the existing capability declaration and finite test
   classification. O1-debug new5 and whole4 pass; whole4 reports811/76skips,
   including59 unsupported creation cases. Root reviewed source and all38
   final receipt references. Candidate925d includes it and passes bounded validation, not primary yet.
3. **One drainer, bounded complete.** Reviewed44a5 is composed with main-only
   and buffer readiness at925d. O1 focus17/0 and affected9 1518/146skips pass,
   zero fallback; af128e61/all36refs checked. Callback payload/error roots,
   nonwaiting competing collectors and explicit busy shutdown/fork handling
   pass their focused controls. This is not release/ASan/full qualification.
4. **Persistent buffer closure, source complete.** Reviewed P/SRE627590,
   Unpickler48b021, ND baf8 and same-owner XID9d compose with main-only,
   readiness and drainer work at3c6b8daa506. Safeguard424 removes export-count
   coercion and silent DEAD/SWEPT release suppression, retaining checked
   invariants. ND original metadata stays separate from transformed views;
   its private constructor rejects repeated successful initialization. XID
   reuses the original managed memoryview acquisition and existing disposal.
   The [owner ledger](10a-buffer-safeguard-removal.md) and independent source
   reviews state the finite support boundary and inherited limitations.
   Corrected consumer evidence is a composite, never a relabeled whole pass:
   eleven old focused cases plus fresh scanner; six old whole-module passes
   plus fresh test_re172/3skips. Original failed profiles remain failed.
5. **Coherent supported GC union, qualifying.** At11:20 PDT the initial final
   make stopped before compilation at source-cleanliness check, caused by an
   unknown-origin ambient in-source build. All588 owned debug products were
   unchanged; preserve that build and failure. Clean existing worktree
   boil-buffer-xid-main-only, new branch boil/gc-final-validation at3c6, owns
   a fresh O1 out-of-source build. Next: new13, conditional whole9, four
   reviewed causal omissions in one negative build, then actual-union release,
   debug and ASan profiles plus closure. Reuse existing recipes/modes/locks;
   stop unexpected failures, diagnose and keep original results. Independent
   source/evidence review precedes primary merge and post-merge smoke. Process
   exit does not excuse supported-run lifetime leaks.
6. **Interpreter report and reflection.** Once the supported GC boundary is
   sound, refresh matched release timing with JITs off and publish the detailed
   IR/dispatch/value-flow/call/root cost report. Compare the preserved H1
   candidate, then commission independent reflection on clean parameterized
   operation families and shared costs. Roughly2× on simple arithmetic guides
   design attention; it is neither a hard gate nor a reason for special cases.

Safety qualification and component feedback are complete. The remaining
ports are composed; final supported-union qualification is active. Keep builds within the two
existing slots; serialize full suites, StackWalk and perf-map-sensitive work. Interpreter timing waits
for quiet native execution and the agreed GC correctness boundary.

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
