# M2b progress

2026-11-08: Implemented explicit terminal ordinary-NOP marker only in the
ordinary false-bool branch in W `0539e0c1ff9`. The emitter checks the trusted
uop sequence, top-level bool guard, scalar stack contract and effects, then
emits the marker after final flush. The GIL-only macro uses a fresh atomic
NOP check, retains PC publication, declines debug lltrace and wrappers, and
counts actual handler entries. Added generator acceptance/rejection and IR
observer/branch tests. Host in-memory generation emits one terminal marker;
changed guard, escaping prefix and reordered marker each reject; AST parsing
and `git diff --check` pass. No generated products, native builds/tests or
commits; independent review requested. Coordinator owns generation, builds,
validation and measurement.

Coordinator release focus exposed a test API error: `starts_line` is boolean,
so the trace-jump test now asserts it and uses `nop.line_number` for the
source line. Sol reviewed tier-1 scope; the marker is now declared `tier1` to
keep it out of optimizing uop IDs. This also makes the entire BOOL false
macro ineligible for legacy optimizing expansion and must be recorded as a
tradeoff. Host in-memory IR generation still emits one marker; host AST and
`git diff --check` passed. Coordinator owns R5 control and focused retries.
