# Queue completion graph: representation and ordering decision

Source-only review, 2026-10-30. **Preferred direction for independent
review: SCC-topological canonical batch ranks, with component completion
tokens and a local qualifying-call chain.** This explicitly changes the
global call-kind delivery order. It is not equivalent to note13 section
3.5 under its existing batch ranks. Coordinator accepts this as a proposed
design direction; there is no source or activation approval.

Read note13 section3.5, `7a-2a-safety-design.md`,
`7a-completion-lifecycle-readiness.md`, the current lifecycle review and
`10a-buffer-release-readiness.md`. Source anchors below refer to frozen
`boil-queue2a-workstack@168f30a96ffdb5516fe94e9ed13b0e60dcb9fae6`.
No code, build, target, probe or historical S1 rerun was performed.

## Exact existing rule and its cost

Let G contain the initial dead-object membership D and its strong edges E,
as reported by `_PyGC_TraceTraverse`, restricted to that membership. A
zero-length path counts for two distinct calls on the same object. Let K
be potential call slots; only actual successfully batched calls receive
ranks. Q is the actual qualifying-predecessor subset: currently FINALIZE
and LEGACY. FINALIZE, LEGACY and DESTROY can wait; CALLBACK has no graph
slot. The existing rule is:

`P precedes R iff P is in Q, P != R, rank(P) < rank(R), and G reaches P→R.`

Inactive slots remain absent; their objects remain reachability transit.
Closed/no-op and already-finalized generators must not gain FINALIZE
slots. Reuse actual eligibility, legacy-cycle/SAVEALL and hook decisions.
Never replace them with a second type/kind inventory.

The proposed explicit reachable-call adjacency is correct if fully built,
deduplicated and filtered, but a K-call chain has Θ(K²) pairs despite only
K−1 object edges. Two count/fill walks from each eligible predecessor can
also cost Θ(K(D+E)) before membership lookup costs. Sparse arrays remove
the independent-call matrix; they do not fix this chain cost.

Condensing G into its strongly connected components does not alone remove
rank filtering. Example: object A reaches B, but B's FINALIZE has rank1
and A's LEGACY has rank2. Under the current rule B must not wait for A.
An unconditional A-component→B-component completion token adds that wait.
Inside an SCC, waiting for the component's own all-done token prevents
its first call from starting. Both are incorrect representations.

An exact bounded-storage alternative for the OLD ranks is a monotone
minimum frontier. For component C, maintain the minimum rank of every
unfinished qualifying call in C or any ancestor component, or infinity.
R is ready exactly when `frontier(component(R)) >= rank(R)`. Equality
correctly excludes R itself. Several predecessors, diamonds and same-SCC
calls follow directly from the minimum predicate.

This is exact, but not a linear-work solution. Each component minimum may
advance through Q ranks; a chain can require Θ(Q·E) propagation. A simple
topological recomputation per completion has O(Q(C+E)) time. An event
implementation must also account for finding changed incoming minima:
scanning all inputs on every input notification can be worse. Fixed indexed
input heaps give O(Q·E·log(max_indegree+1)+K) propagation and local tracking
bounds, with O(D+E+K) storage. These are implementation alternatives, not
approved extra machinery. Do not advertise just the storage bound as a
linear-time algorithm or assume an arbitrary cutoff makes it acceptable.

## Proposed clean policy revision

Keep **selection** unchanged. `handle_finalizers` at `gc_live.c:890`
continues legacy-cycle handling, all-finalizer selection, SAVEALL, legacy
selection and resurrection in that order. `queue_destroy_hooks` at `:998`
still selects hooks only after that resurrection and queues them all
before resurrecting from them. Do not topologically interleave these
construction phases: that can suppress legitimate finalizers or clean an
object kept alive by its user finalizer.

After batching stops, define the canonical ranks of its ACTUAL non-callback
calls by:

1. A deterministic topological order of initial-G SCCs, referrer first.
2. Within an SCC, retain original actual batch order, including FINALIZE
   before LEGACY for one object. No reordering by owner.

Callbacks retain their original batching/publication contract and neither
gain waits nor become predecessors. No new global callback-completion
barrier is introduced. Existing owner queues still permit unrelated calls
to run concurrently; static rank is not a global wall-clock invocation
order.

This revises the global all-FINALIZE→all-LEGACY→all-DESTROY delivery order
across DIFFERENT SCCs and the relative delivery of unrelated non-callback
calls. Note13's existing rank contract must be
explicitly amended and reviewed before implementation. The source comment
“tp_del after tp_finalize” is retained for each object and within an SCC;
its current global phase order is not silently claimed preserved.

## Tokens and proof under the NEW ranks

Each component C has an upstream gate and one completion token T(C).
The gate opens when all incoming component tokens complete. A component
with no qualifying local calls completes T immediately when its gate
opens. Otherwise its qualifying local calls form a chain in local rank
order. First qualifying call starts only after the gate opens; each next
one waits for its predecessor's whole dispatch completion. T completes
after the last qualifying call completes.

A nonqualifying waiter is released after the upstream gate and its most
recent earlier LOCAL qualifying call, if any. It does not delay the next
qualifying call or T. Thus pure DESTROY calls never become predecessors
through token accounting. This is a proven local-chain reduction, not the
invalid “last call of an owner” shortcut: all qualifying calls within one
SCC mutually reach each other and their earlier local ranks form a chain.

Proof: a strict ancestor SCC's every call has an earlier canonical rank
than R, and its qualifying calls all reach R. Incoming tokens therefore
require exactly the strict-ancestor qualifying set, by induction over
the condensation DAG. Within R's SCC, every qualifying call reaches R;
exactly those with earlier local rank are required. The local chain
represents that prefix, excluding R and all later calls. Components that
cannot reach R do not contribute to either condition. This is precisely
the earlier-rank rule using the NEW ranks, with no added dependency on
nonqualifiers or disconnected work.

Diamonds need no transitive deduplication: wait for each incoming token
once. A shared ancestor contributes through both branches, but each branch
token completes once; there is no duplicate decrement of a call count.
Duplicate condensation edges may either be deduplicated or counted and
consumed individually, consistently. Token processing must use an
iterative preallocated worklist, not recursive C calls along a long chain.

After input construction, SCC/condensation/topological processing,
publication and all normal completion transitions take O(D+E+K) total
work and storage. Each component token, graph edge and local-chain link
is consumed once. Deterministic address-sorted membership adds
O(D log D + E log D) input lookup work; two checked count/fill traversals
do not change that bound. Do not describe that lookup as free. Independent
qualifiers have no mutual wait, a chain requires linear graph storage and
transitions, and a giant SCC uses one local qualifier chain. These are
algorithmic bounds, not measured performance claims.

## Storage, partial batches and lifetime

Capture G in the reviewed no-Python/non-detaching snapshot interval,
before the FIRST call node, including weakref callback nodes. Allocate
checked membership, edge counts/offsets, edge storage, SCC scratch,
component inputs/outputs, token worklist and K-bounded activation/local
lists before batching. Count/fill must match; visitor error, arithmetic
overflow or allocation failure aborts through empty-batch rollback.
No incomplete graph can route calls or permit sweep. Logical cases with
no possible predecessor need no dependency graph; that is not a size
threshold or fallback.

Existing per-node allocation or later collector work can still fail after
some calls were batched. BOTH success and that common failure path must
finalize the actual partial batch from preallocated graph storage, before
`route_calls` at `:1609` or domain unpin. Inactive slots contribute no local
qualifier, waiter or lifetime reference, but their graph vertices remain
transit. Original selection/notification state and rollback accounting
remain authoritative; no unordered partial batch is routed. This requires
an explicit common-path source proof at implementation review.

Before Python resumes, publish each actual node on READY, WAITING or
RUNNING root-visible lists. Graph metadata retains integer identities;
retire object-bearing snapshot/index scratch before its storage can become
unrooted. Completion occurs after callback-specific cleanup and unraisable
reporting return. Stable node identities survive running callbacks; do not
copy/free a running node as the current drain does. Metadata can retain
terminal integer tombstones until the last actual graph node retires.
Do not free the graph when all TOKENS complete: pure DESTROY waiters may
still be waiting for execution or running. Last-node retirement releases
graph storage exactly once, under the domain-GIL protocol.

## Native cleanup and lifecycle decisions still required

The buffer readiness note establishes that native consumer cleanup may use
an exporter's resources while yielding or collecting. If both cleanups
use CALL_DESTROY, waiter-only DESTROY classification is insufficient.
Review a native-cleanup predecessor capability independently of user
`tp_finalize` and ordinary watcher notification. A qualifying cleanup call
then participates in Q and the same local chain/token protocol; a pure
notification remains a nonqualifying waiter. No new slot, flag, manual
type table or acquisition-rearming protocol is approved by this note.
Selection must identify the actual acquisition and preserve inert state,
view storage and consume-before-callback lifetime. Making every destroy
hook a predecessor is a separate semantic expansion, not a representation
fix.

Activation remains blocked on reviewed close/fork lifecycle source.
Use the current `reviews/7a-lifecycle-prerequisites.md` recommendation,
not the superseded early-admission-gate/cancel-in-child alternatives in
the historical readiness draft. RUNNING is not complete when a thread is
removed or disappears after fork. Owner transfer must preserve all roots
and node identities. Neither token success nor an empty ready list can
stand in for the quiescence, retirement/adoption and fork admission proofs.

## Focused review and future controls

Require source/test plans for the following before a source grant; no
native matrix is proposed:

- Explicit note13 rank-policy ruling, including the cross-kind A-LEGACY→
  B-FINALIZE counterexample, while same-object order stays unchanged.
- Two unfinished ancestors of one waiter, reverse completion, a diamond,
  an inactive middle slot, a cyclic object SCC and a disconnected call
  that finishes while those predecessors remain blocked.
- Pure DESTROY blocked on application code must not hold a downstream
  token; qualifying native release must hold it. Same-SCC waiters before
  and after a qualifier distinguish prefix waits from an all-SCC barrier.
- Node-allocation failure at a partial batch, graph-preparation OOM before
  any node, and last pure-waiter retirement after all tokens are complete.
- Copied omission of one incoming token must fail a named early-release
  oracle; treating every DESTROY as qualifying must fail the unrelated
  progress oracle. These use ordinary callback coordination, not raw
  addresses or the historically held C-memory probes.

Recommend the policy revision for independent design review rather than
implementing the closure or frontier algorithm now. If the existing global
kind order is retained, the exact frontier remains a valid bounded-storage
option with its stated repeated-propagation cost; it has not been approved
as an acceptable default.
