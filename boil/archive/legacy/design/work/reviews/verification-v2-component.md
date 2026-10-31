# Verification v2 identity/result component review

Coordinator review, 2026-10-29 09:48 PDT. Verdict: LAND_COMPONENT.
This grants only the identity/result prerequisite, not a named/full gate,
production orchestration or numeric gate hook.

Scope is explicit source/runtime/tool roots, complete snapshots, qualified
build receipts, actual imported origins and targeted result/skip witnesses.
The run command and numeric gate hooks remain separate. Runtime identity has
one strict path: component-only coverage still requires a qualified receipt.
Documentation-only commits may change contextual HEAD while executable inputs
stay equal; executable drift must fail. Fresh cache creation remains the
caller's obligation and actual cache-prefix identity is checked at runtime.

Review and controls found four material gaps before component clearance:

- Exact-only protected skips missed parameter/message subtests. Matching now
  covers the protected base and those suffixes, including allowlisted skips.
- Importing the worker target through run_workers before runpy breaks -Werror.
  Worker/platform checks avoid pre-importing their target; driver checks keep
  the infrastructure imports and module-origin proof.
- The initial build inventory recorded extension aliases without their target
  bytes. Alias and resolved-content records now bind both, reject outside
  targets and detect target mutation with an unchanged symlink. The initial
  receipt is superseded by the independently reviewed172-entry supplement.
- Cext's instrumented command added --python without -j. It now explicitly
  uses one isolated module worker, preserving16 cases/cpu/no filters and
  matching short scheduled names to canonical imported names. This is an
  isolation difference from ordinary sequential gate3, whose argv is unchanged.

Harness preflight also caught its own pycache writes, stale tool-root paths
and an assumption that every discovered ID executes without a source skip.
Failed/unexecuted attempts remain retained. Final qualification uses one
frozen tool root and exactly five existing no-cpu skips in context/weakref,
plus seven limited-API/MSVC source skips in cext. Required successful IDs
exclude only those explicit IDs; no new CLI filter or source skip was added.

Frozen proposal af48c18732b9911b49162bc95232e4a589f36b5af0c3bc0f47c93078524b50dc,
identity84f115e1f256cad92abdf0724ce07f05e2886ef79648824716f5ba1d0f6fe500 and
qualified supplement9f6f6a6d97c158c198f32503e819af4b5ce95b0b00163269598dea3c93a19c3f
bind native052, final tools and actual controls. The owner reports36 host
controls, including old-behavior failures. Actual off/kill, auxiliary,
module-order, repetition, cext and isolated-failure evidence now passes.
Ten unavailable optional extensions and report-only unpinned counters remain
explicit limits; no current-ROOTS or all-extension result is inferred.

Final independent checks: all105 indexed artifact hashes match, along with
the five changed source/test/document hashes and the complete current identity.
The coordinator reconciled all six driver cohorts from raw records, concrete
discovery, exact approved skip policy, XML outcomes and counters: four exact
case cohorts,203-case module order/5skips and cext16/7skips. Actual process
roles total6 drivers,7workers and1 auxiliary. Seven actual isolated failures
have their intended status/marker, and11 actual-evidence corruptions fail.
The batch completed0 at16:45:01 UTC; its code/helpers remained frozen.

Final evidence index SHA256
`bdb4997d8334bb6691187fa45e7d2697b7061a381e41d5cf4c6535d499c482c5`
is retained under identity-stage2b/component-final-index.json. Primary native
sources, production registry, gate.sh, gate_run.sh and the worker shell adapter
are unchanged. Component checks cover the affected tooling; no additional
native integration test run is required for this tool-only landing.
