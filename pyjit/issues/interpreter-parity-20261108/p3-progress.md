# P3 implementation progress

2026-11-08 12:23 UTC. The isolated worktree preserves the M1 diff and contains
P3 source: one shared call-record initializer, a private guarded vector entry
that uses the existing Python push/overlap/fallback and native paths, and a
checked invocation marker on the ordinary general Python-call prefix. The
GIL-disabled branch delegates to the ordinary vector entry. The guard's
cached version is asserted valid only in debug builds; the common version
comparison is unchanged.

In-memory generation shows the guarded success invoking the private helper
and the generic fallback invoking the generic vector entry. Six altered
prefix/marker/instrumentation examples are rejected. Added focused call,
monitoring and vectorcall-setter tests; native tests and regeneration remain
with the coordinator. Source review and final test review are in progress.

12:25 UTC. Source ready for coordinator and independent actual-diff review.
The generator checks the named prefix, scalar/window shapes, focused guard
conditions and nonescaping behavior. The local invoke override does not alter
the generated fallback. The setter helper exercises public
`PyFunction_SetVectorcall`; native execution has not yet occurred.

Marker metadata was renamed to IR_MARKER_EFFECTS and ir_marker_properties
to cover both readers and calls. In-memory generated output was identical
before and after; all 11 existing M1/P3 mutation checks still rejected.
