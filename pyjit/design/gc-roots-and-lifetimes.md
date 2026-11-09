# Memory ownership and lifetimes

Reference counting with cyclic collection is selected. Tracing mark/sweep and
nursery experiments are closed; no collector selector is planned. Their
[earlier contract](../archive/legacy/design/gc-roots-before-wiki-reset-20261102.md)
is preserved as history, not a current root or testing requirement.

## Strong edges and publication

Every initialized generic heap field owns its object edge or an immortal value.
Inline scalars have no object edge. Traversal visits the actual initialized
strong fields, masks ownership tags before object access and excludes weak or
borrowed aliases. The cyclic collector must see container, cell, instance and
extension payload edges under the same contract.

Initialize storage before tracking or publication. Publish replacement state
and detach the old owner before releasing it. Capture independent owners before
allocation or callbacks if a borrowed table/frame view could change. No stale
entry pointer may survive resizing, callback-driven mutation or retirement.

## Reentry, failure and finalization

Closing an object can run Python, weakref callbacks or finalizers. Preserve
observable release order, primary exceptions and initialized live homes.
Deallocators preserve the exact entry exception and report their own failures
as unraisable. Do not weaken that contract to make a test inject an error.

Release contract (D2, invariant I9). Freeing an exact `int` or `float` box
never runs Python and never changes the current exception. Allocator hooks,
for allocation as well as free, and reftracers (`PyRefTracer_SetTracer`)
never run Python, never raise and never change the current exception, so
canonicalising a value frees its box at once, with no exception save around
it. Debug builds check the hook and tracer entry points in `obmalloc.c` and
`_PyReftracerTrack`, and the box free in `_PyValue_Canonicalize`: the current
exception and frame must be unchanged (`_PyReleaseCheck`). Releasing any other
object can still run Python, so revalidation after such a release stays.

Fallible preparation precedes publication. Distinguish failure before commit
from cleanup after commit; do not roll back callbacks or overwrite their later
progress. Keep old/result owners until their actual caller-state publication
point, then refresh interpreter/JIT state after callback-capable cleanup.

Weakref clearing, resurrection and cyclic finalization retain their explicit RC
ordering. Type caches and feedback need a lifetime/invalidation proof; a pointer
kind check or a late version comparison cannot rescue a dangling borrow.

Opaque extension Fields/Globals/buffers require owned edges and clear/traverse/
release protocols. A call-local handle cannot silently become a persistent root.
[Extension design](extension-api.md) owns those boundaries.

## Diagnostics

Use refcount, traversal, cycle, reentry and applicable sanitizer tests for this
runtime. `Python/gc_trace.c` can provide RC verification, but it does not make
tracing the freeing collector. Retired tracing counters, conservative stack
scans and zero-unwind gates are not universal RC admission requirements.
