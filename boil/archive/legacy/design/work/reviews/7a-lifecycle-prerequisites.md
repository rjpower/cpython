# Queue completion: close and fork prerequisites

Source-only recommendation, 2026-10-30; no implementation/native grant.
Reviewed `boil-queue2a-sweep@7f4459bf259` (runtime base5a45), note13
sections3.5/5.1–5.5, and the owner's completion-readiness note
`1012cbb4c4e7e31ff4e9851f724856fae5e9fdddee5e4fe14555b8876e9ea8b2`.
V2 OOM validation is separate; it establishes none of these contracts.

## Recommended decisions

1. **Normal shutdown waits for actual dispatch completion, with ordinary
   admission open throughout the wait.** Commit the admission gate only
   at the successful final world-stopped readiness check.
2. **An opened shared-heap owner adopts its remaining READY/WAITING work
   into main after retiring owner-local destruction metadata.** Do not
   synchronously drain a foreign dependency. This explicitly changes
   note13§5.1; it is a recommendation requiring coordinator acceptance.
3. **For the initial fork prerequisite, quiesce other threads' dispatches
   before committing fork.** Preserve the surviving main-thread dispatch
   chain. Accept/document that fork may wait for a callback. Do not
   implement child cancellation by dropping graph nodes.

These transitions cover every call kind, including callbacks and future
blocking native buffer cleanup in CALL_DESTROY. The graph's predecessor
eligibility is a separate decision: today's FINALIZE/LEGACY-only rule must
not be built into lifetime/quiescence interfaces. No owner-count shortcut
or single global finalizer drainer is needed.

## A. Quiescence without closing admission during the wait

`make_pre_finalization_calls()` already performs thread shutdown, pending
calls, atexit and subinterpreter cleanup with the world started. Leave
dispatch admitted during this work AND while waiting for RUNNING calls.
An active callback can join a worker that needs its own ready dispatch;
closing admission before the wait would introduce a dependency deadlock.

Extend the final readiness transaction at `pylifecycle.c:2327–2356`:

- Acquire the existing pending mutex, stop the world, then take HEAD as
  today. Require RUNNING==0 alongside the existing thread/atexit/pending/
  subinterpreter checks and successful finalization-guard CAS.
- Only on that successful branch latch owner dispatch admission closed.
  The function returns with the world stopped and pending mutex held.
  Both `_Py_Finalize()` and `Py_EndInterpreter()` call SetFinalizing before
  starting the world; source contains no intervening Python callback.
- On any failed predicate, do not latch. Release HEAD, restart the world,
  release pending mutex, and wait/retry with dispatch still admitted.
  A RUNNING notification uses publish-before-detach or compare-and-park
  semantics, with the condition rechecked after attachment. No wait while
  holding HEAD, pending mutex or a stopped world; no polling completion.

Under the supported shared-domain GIL, READY→RUNNING and completion occur
attached while holding that GIL. StopTheWorldAll excludes a new start at
the final zero check. A detached callback remains RUNNING until it resumes
and finishes dispatch/error reporting; it cannot disappear from the count
merely because its thread detached. Finalizing-thread drains remain allowed
after commit; ordinary threads cannot start new calls. Initialization and
failure reset the gate consistently.

An application callback that never returns can prevent shutdown. That is
an explicit semantic consequence, not a reason to forge DONE or abandon
its resource dependencies. Reentrant close must be rejected before state
mutation: detect an active call for the target owner on the CALLING OS
THREAD, including across a C-level tstate switch. Current-tstate
`running_queue` alone misses that self-wait. Runtime finalization must
reject any active dispatch on that OS thread. Recommend preserving the
existing fatal invalid-use route for void Py_EndInterpreter and returning
failure from Py_FinalizeEx without changing runtime state; the exact public
error contract needs explicit review before code.

## B. Settle foreign WAITING by adoption, not strict drain

Strict§5.1 drain is unsafe as a progress promise: a predecessor may belong
to an idle foreign owner with no running thread to service it. Releasing
the GIL does not create that execution. Cooperative pumping by switching
into arbitrary foreign interpreters is a larger behavioral change.

For an opened shared owner: drain local READY work on its closing thread,
retire watcher/code-extra metadata using its own tables, drain resulting
READY work, then atomically transfer remaining READY/WAITING nodes and
publish CLOSED under the existing domain-GIL/HEAD ordering. Never transfer
a RUNNING call. Preserve graph identities, pending counts and edges;
change each node's current owner before releasing old GCState storage.
Completion must target that current owner. No transferred waiter runs
before its actual predecessors finish.

This requires note13's finite Queue2b/2c work BEFORE graph activation:
owner-local discovery/registration checks, registration freeze, watcher
retirement, detaching code extras before their free functions, and adoption
of heap segments, registered roots, immortals and kept-thread state. These
are not present at the reviewed source: there is no LiveAdopt,
subproc_absorb, registration_frozen or Code_ClearExtras implementation to
reuse. Ready-list splicing alone is insufficient. Python/free functions
run on the owner outside HEAD; only allocation-free structural publication
and ownership transfer occur under HEAD.

An isolated domain has no foreign predecessor; after RUNNING quiescence,
its finite rank-oriented graph can drain locally. Never-opened isolated
domains retain the separately specified whole-domain discard/leak rule.
At runtime shutdown, subinterpreters close before main commits finalization;
all shared work then belongs to main, whose close drains the remaining
graph rather than discarding its waiters.

Compatibility change: closing a shared subinterpreter can return with
blocked calls retained on main. Their delivery owner changes at close,
consistent with the heap-owner remap, rather than falsely promising every
call completed before return. Record this change to§5.1/5.4 explicitly.

## C. Prefer a fork quiescence prerequisite to incomplete cancellation

Run before-fork Python hooks once, then cooperatively wait with admission
open for every OTHER thread's RUNNING calls, across the runtime. At the
existing import-lock/world-stop/HEAD fork boundary recheck; if a call raced
in, restore locks/world and retry the wait without replaying Python hooks.
Place that recheck before the warnings-before-fork lock step. Only a
successful stopped-world check commits fork admission. Parent/error paths
reopen only admission this fork closed, preserving any existing shutdown
gate, before after-fork Python hooks. Child reconciliation occurs
before any vanished tstate clear, interpreter clear or child user hook.

Calls on the surviving main thread remain RUNNING, rooted and identifiable;
their recursion guards remain set and they complete normally after fork
returns. Do not infer a single surviving call from one tstate boolean:
nested C-level tstate switches require an exact dispatch chain. A surviving
foreign-owner dispatch cannot coexist with deleting that interpreter;
reject that unsupported fork context before OS fork rather than freeing
its active owner. Specify the void PyOS_BeforeFork embedding-error contract
explicitly; the Python fork paths already refuse a current subinterpreter.

Queue2c's dedicated allocation-free/no-Python child disposal is also
required. Current DeleteList and DeleteExceptMain clear Python state and
are not that protocol. Adopt shared pending nodes/roots/heap state; discard
an isolated domain only as a whole, which cannot later be collected or
rediscover its calls. No vanished RUNNING node exists after successful
quiescence. Preserve the surviving main call; never mark it DONE in child.

This policy may block fork on a nonreturning daemon callback or an
application callback waiting for its caller. Accept that compatibility
tradeoff explicitly. If nonblocking fork is required instead, cancellation
needs a separate design: dropping CANCELLED WAITING nodes permits later GC
to rediscover never-started FINALIZE/LEGACY work, and graph tombstones alone
do not prevent it. Redelivery suppression, descendant native cleanup and
retained payload lifetime must all be specified; no fake completion flag
or unrooted descriptor is an acceptable substitute.

## Finite interfaces, split and decisive controls

First implement the owner-metadata/retirement/adoption prerequisite, then
stable rooted RUNNING identity plus the shutdown/fork handshake, then graph
activation. The latter activates READY/WAITING/RUNNING together; do not
publish waiting nodes before lifecycle handling exists. Code ownership:
gc_live and GCState for lists/counters/admission/notifications; tstate or
thread-local dispatch identity for self-wait/fork; pylifecycle for the final
readiness check; posixmodule/pystate for fork and no-Python disposal;
watcher/code-extra/mimalloc files for the already specified adoption work.
Keep root visitation and terminal accounting independent of call kind.

Future bounded oracles should establish:

- A blocked dispatch can resume during shutdown; its joined worker can
  execute a ready call while admission stays open. Copied early-gating or
  omitted-RUNNING checks must fail a named order/completion assertion.
- Closing an owner with a waiter on an idle foreign predecessor returns
  by adoption; the waiter remains blocked/rooted, then runs once on main
  only after the predecessor completes. Exercise metadata free callbacks
  that allocate, collect and release the GIL before transfer.
- Two predecessors finishing in reverse order never release a waiter
  early; unrelated work still runs. A kind-independent stand-in must cover
  blocking native cleanup, not only Python finalizers.
- Fork waits for a foreign-thread dispatch but supports fork inside the
  surviving main dispatch; parent and child each complete that survivor
  once in their own process. Shared waiters survive disposal, no foreign
  destructor runs under HEAD, and copied synthetic completion is detected.
- Reentrant close across tstate switching rejects before teardown rather
  than self-waiting. No source-only recommendation authorizes a probe.

Timing is parked. These are implementation-review boundaries and semantic
controls, not a new repeat matrix or an execution request.
