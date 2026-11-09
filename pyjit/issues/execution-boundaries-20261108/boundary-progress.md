# Boundary progress

2026-11-08: design independently reviewed; both blockers resolved by generated
SAVES_IP eligibility and all-Py_DEBUG decline. Coordinator admitted implementation.
Runtime source in boundary-ir-20261108 now emits three constant bool edge
continuations, invalidates dependent handlers before instrumentation target
updates, and preserves canonical feedback/opcode identities. Host regeneration,
numbers checker, new-test py_compile and diffcheck pass. Native validation not
run by worker; coordinator owns it. Peer actual-diff review requested.

New runtime tests cover three edge descriptors, guard decline, global/local
monitoring enabled in a callee, and suspended generator monitoring. Existing
bool/NOP observer test accepts the new physical aliases. Canonical anchor words
remain allocated metadata; physical compaction is deferred. Next: independent
code review and coordinator native results; separate ignored-result completion
proposal, with language requirements distinguished from CPython timing policy.

Separate call-result proposal now covers exact and guarded general Python child
entry with one tagged-recipient/completion protocol; final retained logging IR
has nine general CALL/POP pairs and no ignored exact-call result on the hot path.
Peer review in progress. Proposed CALL completion PC avoids dormant POP PC store;
source line remains equal, raw untraced finalizer f_lasti change is explicit.
Successful f_lineno setter invalidates pending optimization to preserve baseline
changed-PC continuation. Generator/coroutine callers and callees excluded.
No call-result runtime edits before admission.

Coordinator admitted the exact+general completion protocol and ordinary CALL-PC
policy. Source now implements aliases 323/324, original-slot recipient tagging,
a single ordinary-return range branch into shared slot publication, late live-POP
fallback after child cleanup, and successful f_lineno tag cancellation. General
Python commits use the existing runner and a nonowning transaction flag; native
fallback keeps its original destination. Generator/coroutine callers/callees
remain excluded. Host regeneration, numbers, diff checks and explicit generated
protocol inspection pass; native builds/runtime evidence remain coordinator-only.
Actual diff sent for independent peer review. New discard tests cover both routes,
child-cleanup tracing, direct CALL-PC policy, result lifetime, decline/errors,
generator exclusions and cross-frame debugger continuation. Bool milestone patch
and test snapshot are retained separately before these edits.

00:17 UTC: source frozen for peer/native handoff. Additional host contract/domain
inheritance and negative descriptor checks pass. Inline None/bool/int/float
results now have stable warmed exact/general selection cases. Local monitoring
activation/callback error cleanup and accepted cross-frame setter cancellation
are included. The latter is a compatibility safety test, not a new promise of
cross-frame jumps; no post-close PC reload/check was added. Common return
publication factors into one helper and normal recipient classification uses
one range branch. Native validation and performance remain pending coordinator.

2026-11-09 UTC handoff: both source increments and independently reviewed test
repairs are integrated at `5bfd1e7e457`. Fresh fixture code objects prevent stale
specialization feedback from contaminating route assertions; synthetic encoders
use the complete handler domain and contract tests inherit canonical shadows.
Result-finalizer activation after the completion guard has an explicit next-real-
instruction observation test. Coordinator focus-v2 release/debug runs each pass
40 tests; CDB lint reports 306 units and zero failing findings. Full suites are
still running and are not claimed as passed. Census-v2 reports 142 entries per
outer loop versus golden 151, zero POP entries and 20 frames/inlined calls, with
linear N/2N deltas. These are canonical counter totals, not physical-alias site
hit counts or elapsed performance. Three architecture pages were independently
source-reviewed; see `design-sync-review.md`.

Full-release follow-up: the import handler-table test exposed an omitted magic
policy update. Coordinator admitted a two-file worker fix: magic 64411, history
entry identifying execution-only table entries, and the table pin
`c96c524dc507b07af93fd7f7a97e36b5e07752c0b85b96790e6f4981d741ff40`.
Host source/header/pin consistency and diff checks pass; native rebuild and
revalidation remain coordinator-owned. The separate GC hole-fixture failure
occurred before the hole-list retention assertion: all five attempts reported
no hole. Its source explicitly allows conservative false reachability despite
hidden-array masking and dead-stack scrubbing. No GC source change was made;
coordinator matched fresh-process golden/candidate/debug module controls pass.
