# A bounded collector callback boundary hypothesis

2026-11-01 06:42 UTC. **One small new design hypothesis is justified; the
failed v2 implementation remains rejected.** This is design advice, not
source or execution approval. Inspected source is frozen
`3a9f70e065bf4112ef3a070746011df7b08892f3` against maintained
`417a9443a599e879c76c34c13ae76943bd6f6b3e`.

Proposed scope: remove only `Py_GC_PRECISE` from the new `live_walk_page`
allocator callback, document its ordinary opaque callback contract, and
leave its body and all other source semantics unchanged. Retain the existing
checker, effect tables, other helper annotations and compiler flags. Add no
opaque exemption, pointer wrapper, noinline/alwaysinline directive or
compiler-specific way of concealing the classifier edge.

This follows an existing boundary. Maintained417's `live_walk_block` and
`live_walk_heaps` are ordinary unmarked functions. The allocator visitor API
invokes its object callback in that convention; classification itself was
never made precise. The new page callback replaces that callback route with
page/map enumeration before invoking the same classifier. Its automatic
precision marker introduced the new requirement that the legacy classifier
and its entire outlined closure become precise. The actual release failure
shows that GCC inlined the map helper and emitted this edge from the page
callback. It does not establish that the callback must have a precise frame.

Production classification runs inside `collect()`: it rejects an already
collecting/sweeping invocation, sets `collecting=1` before marking, pins the
domain and holds the HEAD lock while visiting heaps. The recorded
`collect_from_entry` context starts stack scanning above the collector's
own deeper frames. Changing the page callback's frame annotation therefore
does not add collector temporaries to mutator roots or remove live mutator
roots in that path. The callback's objects still have the same initialized
map membership, pinned heap storage and unchanged classifier/lifecycle
ownership. No new publication, allocation or safepoint rule is proposed.

The collecting guard is not a universal property of every inventory caller.
`_PyGC_TraceLiveHeapObjects` lists objects using `res==NULL`; that classifier
branch only records addresses with native scratch allocation. Cold inventory
tests use address-only callbacks. `_PyGC_TraceLiveRootsOfHidden` also uses a
failed clear walk outside the ordinary collecting entry; its classifier can
reach the inherited `tp_is_gc` path. That is a reason to preserve the normal
opaque callback convention, not to label all helpers noncollecting or to
invent a new reentry guarantee. These preexisting diagnostic call paths are
not repaired or given broader support by this hypothesis.

All actual operations must remain identical: original type and weakref
snapshots, kinds expression, header transitions, page traversal, fault
trigger, independent counts, unconditional clear, rollback and recovery.
The strict debug witness already passed under v2; its source and assertions
must not change. No exact scalar/PID telemetry is inferred from that pass.
Source review should check the one-marker diff, complete caller uses and
unchanged bodies; this does not need a legacy collector precision rewrite.

Acceptance requires a separately granted finite source/feedback attempt and
fresh exact linked audits, particularly O3, followed by the same focused
case/origin/root/product/idle contract. If another optimized direct edge
crosses a precise boundary, stop this hypothesis rather than expand closure
or add an exemption. No forcing inlining or indirection is allowed. Existing
failed images and debug partial evidence remain frozen. Even a linked-audit
pass establishes no speedup: matched elapsed is the next decision, before
expensive qualification. Poor or absent measured promise should retire the
candidate; the hard 15:14:44 cutoff is a ceiling, not a reason to consume all
remaining tracing-GC time. This narrow boundary evidence supports one bounded
attempt rather than immediate abandonment solely because of these annotation
failures. RC remains the ready exit if the attempt cannot close or measure.

No source/helper patch, compiler, target, probe or timing ran in this design
audit. Source anchors: `Python/gc_live.c` `collect`/`collect_from_entry`;
`Python/gc_trace.c` `live_mark_roots`, `live_walk_heaps`, `live_walk_page`,
`live_map_area`, `live_walk_block`, the listing/root diagnostic callers;
`Python/gc_walk.c` `_PyGC_WalkStack`; and the unchanged direct-edge rule in
`Tools/build/gc_inline_audit.py`.
