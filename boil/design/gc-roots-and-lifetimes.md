# GC roots and lifetimes

The collector is a nonmoving, full-domain tracing mark/sweep collector in the
supported Linux/main-interpreter/GIL configuration. Native addresses remain
stable while their allocations remain live. Ordinary threads are supported;
foreign interpreter creation and adoption are not implied.

## Root and edge coverage

- Managed frame arrays are roots with explicit initialized bounds. Shared cell,
  tuple and frame range definitions serve direct marking and generic traversal.
- Temporary C values/objects use the existing rooted spans and LIFO lifetime
  rules. Durable registrations describe caller-owned storage for its declared
  lifetime. Neither is an independent registry of every object.
- `Py_GC_PRECISE` and generated precise-handler metadata identify declared frame
  contracts. C root checking, linked inline/effect auditing and compiled-stack
  attribution are complementary checks, not interchangeable proofs.
- Supported opaque C stacks, PyMem storage and static/native roots retain
  conservative coverage. Detached stacks must preserve the state and extent
  needed by the actual scan protocol.
- Precise field origin does not by itself prove an active allocation, current
  domain or readable header. Object-map, allocator and domain validation remain
  in the maintained path; static immortals require their own supported handling.
- A strong-field visitor must cover type/runtime attributes and initialized
  managed fields without turning weak or borrowed aliases into strong edges.

## Publication and failure

Initialize every traversed field before tracking/publication or an allocating
call that can expose the object. Frame bounds and executable sentinels must
remain valid throughout construction, suspension and teardown. SHALLOW state
does not authorize scanning uninitialized storage.

Mark/work-stack publication is checked: journal the mutation before making
MARKED visible. OOM must undo temporary state and retain safely; a later pass
must not skip referents because stale mark bits survived. Prepare fallible order
and batch storage before irreversible cleanup. Partial delivery keeps its
declared semantics and does not become transactional merely because sweep stops.

## Death processing

The heap walk classifies actual weakref, finalizer, hook and buffer work.
Eligibility is revalidated where resurrection or callback effects can change it.
All nodes required to prove ordering remain present even when a node has no
callback of its own. Missing optional work must not drop a required connector.

Callback-bearing weakrefs clear at the prescribed boundary. Callback-free refs
can remain usable through finalization/resurrection and clear at actual death.
Do not replace this distinction with a blanket early clear.

One interpreter-wide claim serializes queue draining. Competitors and nested
collectors enqueue and return instead of waiting. The active operation and saved
exception stay rooted through callback execution and unraisable reporting.
Every exit releases the claim; readiness must not be lost.

Resource release obeys acquisition ownership and readiness. A borrowed
`Py_buffer` remains usable for its declared lifetime; release runs once for the
right acquisition and cannot invalidate another live consumer. Guarded native
generator cleanup uses the existing close-only proof, excludes observing or
Python-capable work, revalidates admission and commits only after fallible
preparation. Escaped frames and other ineligible cases retain ordinary handling.

## Teardown and observation

Supported finalize/fork callers must reject a busy drainer before destructive
commit, unwind locks/world state and preserve the declared attachment/error
contract. Void embedding APIs and unsupported entry modes have explicit stronger
preconditions. A Python-visible caller must use an available recoverable path;
it must not destroy a still-initialized runtime after rejection.

Debug/kill verification, root counters, reftracing and monitoring preserve their
actual semantics. A diagnostic unpinned snapshot visit is not an unwind fallback.
Do not weaken the checker or reinterpret a failure to make a counter zero.

## Extensions

Minor collection, movement, writable-slot/reload interfaces and pinning are
additional capabilities. A read-only precise visitor is not a relocation proof.
Age metadata and native-owner indexes must not become accidental major roots.
New spaces must preserve lookup, publication, lifetime, OOM and observer contracts.

See [current direction](gc-and-interpreter-performance.md), the
[root/checker issue](../issues/root-checker-audit/status.md) and
[validation](validation.md).
