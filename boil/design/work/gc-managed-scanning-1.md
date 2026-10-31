# Managed scanning: first source checkpoint

2026-10-30. Source only, from integration `d0f4671006c3e90879b5cb854cd0a055822eb44e`
in `boil/gc-managed-scanning`. No build, target or timing has run. The approved
implementation packet is held in primary as
`boil/design/work/gc-managed-scanning-implementation.md`.

## Implemented boundary

`_PyGCValueRange` is a borrowed address/length view. The cell, tuple and frame
accessors provide the single definitions of their value fields and physical
bounds. Generic cell/tuple traversals use those accessors; the frame generic
loop's three-line refactor remains an **unapplied fragment** while the
native-cleanup owner holds `Python/gc_live.c`. That file is byte-identical to
the base. Merge landed primary and apply/review the fragment after its release;
there is no unlanded cross-branch merge or native-ready claim at this point.

`live_precise_child` contains the old `live_visit` body, including exact
`live_lookup`, current-domain validation and debug invalid/dangling-pointer
checks. `live_visit` remains the native `visitproc` adapter. Managed loops use
the concrete helper directly after checking value tags; they neither box nor
canonicalize values. Tuple scanning retains the native traversal's reverse
element order. A `reverse` argument is constant at each call site and is
intended to specialize under ordinary inlining; actual code generation and
size remain unverified.

Exact cells and nonempty exact tuples have direct range dispatch in
`live_drain`. Their type edge is still visited first, and assertions record
that these exact layouts have neither runtime attribute words nor inline
attribute storage. Every other object, including tuple subclasses and empty
tuples, retains `_PyGC_TraceTraverse` with its type/runtime-attribute/static-type
rules. The non-live diagnostic traversal remains generic. No arbitrary type
cutoff or allocator change is involved.

One private frame enumerator supplies both generic and live adapters. It
preserves chunk order, pushed-but-unlinked frames, the uninitialized executable
sentinel, avoidance of duplicate thread-owned frames and active generator
frames. Interpreter-owned entry/base frames still visit only their return
slot. `Python/frame.c` shares the fixed header visitor; the live adapter uses
direct array marking after those header fields. Its callback is per frame,
not per register. The fixed small header set retains its existing callback
protocol. The ordinary generic entry still propagates visitor errors.

These views are not registered roots or writable-alias proofs. They do not
permit moving a raw/interior pointer, treating a start-map bit as domain
membership, using a partial liveness map, adding weak/borrowed fields to the
strong graph, or skipping construction/SHALLOW rules. Mark-stack OOM retains
the existing rollback and per-object traversal behavior; no new failure state
or allocation is introduced by a range or frame adapter.

`Include/object.h` receives only the requested opening-comment correction:
the actual one-word shape/flags header, tracing ownership and explicit resize
API rules replace stale RC/two-field/always-fixed-address descriptions. The
header and object layout are unchanged. On the current 64-bit layout the
source fields make a cell 8-byte header plus 8-byte value, not24 bytes; no
runtime sizeof measurement is claimed here.

## Focused observable controls

Four `ManagedScanningTests` methods in existing `test_gc` use the existing
child helper, preserving actual mode/extras/stress assertions:

- A closure cell owns a mixed tuple whose mutable payload survives repeated
  collections; generic referent queries still identify the same child.
- A tuple subclass retains both its element and a separately owned attribute,
  checking the complete generic fallback.
- A function creates its mutable local internally and retains it across
  ordinary and raising callbacks, including another collection in exception
  handling. The caller has no argument-window copy of that payload.
- A generator retains its mutable local across suspension, collection while
  running and escaped-frame lifetime after generator death.

The generator fixture is a public data/lifetime regression, not exclusive
proof of direct active-frame scanning: the rooted generator's generic
traversal can also reach its frame values.

Automatic GC is disabled within these child fixtures, with explicit collections
at the tested boundaries. They exercise external data/lifetime behavior, not
private dispatch counters. Existing call/return, overlap, exception, stale-root
and native traversal tests remain relevant. These new controls have only been
AST-parsed and their four child bodies compiled by the host; none has executed.

## Review and next transition

The original ignored `build/gc-managed-scanning-source/` packet is preserved.
The successor `build/gc-managed-scanning-source-v2/` freezes the source diff,
per-file hashes, four child IDs, host results and unapplied fragment. Existing
`root_check_c.py` passes the four changed C translation units; whitespace and
fragment applicability checks pass. The initial hand-written fragment had an
incorrect hunk count and was corrected before freezing; no tracked collector
source was modified by it.

Independent source review follows this checkpoint. After native cleanup lands,
merge primary without rewriting history, apply the small released-file hunk,
and review the actual composed source before any native build. Native evidence
must check the direct loop's assembly, code-size effect, actual mode and affected
lifetime/root behavior. No claim about dispatch savings, movement, omitted
lookups, region coverage or full qualification follows from this source pass.
