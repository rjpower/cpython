# Language requirements and runtime lifetime policy

Python does not require immediate destruction of an unreachable object or
CPython's usual last-reference finalizer timing. The
[language data model](https://docs.python.org/3/reference/datamodel.html#objects-values-and-types)
permits delayed or omitted garbage collection, forbids collecting reachable
objects, and warns against depending on immediate finalization. It explicitly
labels prompt reference-counted collection as a CPython implementation detail.

This distinction matters when designing register ownership and call completion.
Our selected runtime uses reference counting with cyclic collection, but that
selection does not turn each existing decref location into a language rule.
The earlier golden review treated exact destruction order too broadly as an
admission constraint. Its source description remains accurate; its implication
that Python requires that timing is corrected here.

## Separate the constraints

| Constraint | What a design must explain |
| --- | --- |
| Python language and supported APIs | Preserve live values, required execution order, explicit cleanup such as `with`/`finally`, exceptions and supported observation behavior |
| Chosen CPython compatibility | Identify any intentionally retained implementation-specific finalizer, weakref, refcount or debugger detail and explain its benefit and cost |
| Ownership and GC safety | Every counted edge and borrowed view has a valid lifetime; no use-after-free, double release, missing owner or incorrect cyclic-GC accounting |
| Memory/resource quality | Bound retention through loops, recursion, suspension, frame escape and exceptional exits; measure relevant costs rather than presenting them as language mandates |
| Current implementation assumptions | Inventory consumers before changing valid/NULL homes, transfer rules or cleanup; these assumptions may be replaced by a reviewed design |

An observable difference in finalizer timing is not automatically a Python
language violation. It can still break an explicitly selected compatibility
policy or reveal an ownership bug. Tests and review findings must say which
constraint they exercise. Do not weaken tests merely to hide failures; document
intended implementation-specific changes and independently review their scope.

## Frame ownership and value retirement

Caller/callee frame ownership answers where references are stored and who
eventually releases them. It need not require a separately dispatched release
instruction for each dead value. Candidate mechanisms include:

- Give call completion a discard destination, so an unused result never needs
  a caller register home.
- Keep values owned by a frame or region and release them during overwrite,
  region cleanup or exit, with an explicit map of occupied owners.
- Fuse required releases with an existing operation or transition when that
  removes slot traffic and dispatch without adding equivalent bookkeeping.

Delayed cleanup is language-permitted. It is not automatically faster or
memory-efficient. A reused slot must account for its previous owner; accumulating
a new owner on every loop iteration needs bounded storage or reclamation.
Even fixed-slot retention can keep a large object alive for a long-running
frame. Conversely, direct disposal of an unused returned value may be cheaper
than transferring it into a frame merely to release it later.

The current IR's `POP_TOP` names an explicit register slot and releases its
owner. Its name comes from the stack frontend; it is not an operand-stack pop
and is not required by register IR. Removing it requires a coherent owner
destination/cleanup protocol, not preservation of the inherited instruction.

## Observation and re-entry are separate questions

Line/instruction monitoring and debugger positioning currently share physical
words with value operations. Eliminating a release word must still explain
the supported observation contract. That requirement is distinct from requiring
a finalizer at the old word. A design may separate the logical boundary from
the value's chosen release point.

When the runtime does release an owner, its finalizer can execute Python.
State and ownership must be valid at that chosen point. Changing release timing
does not justify exposing half-committed frames or invalid homes to callbacks.
The golden return path holds a result across child retirement and then publishes
or discards it. That is an as-built protocol, not an immutable language rule.
An alternative must define its own safe transition and its intended differences.

This page governs new proposals. The golden tag and sealed historical reviews
remain unchanged. See the [current experiment](../issues/execution-boundaries-20261108/plan.md),
[interpreter](interpreter.md) and [frame contracts](call-and-frame-lifetimes.md).
