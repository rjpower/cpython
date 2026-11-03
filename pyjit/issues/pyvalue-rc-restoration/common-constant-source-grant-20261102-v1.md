# Canonical common constants

D independently reviewed the concrete common-constant-cut-v1.md design and all
actual references:14 table entries and14 owned/check-NULL flowgraph callers.
The coordinator adopts that design. Root implements; D independently reviews.

Distinct20-minute source cut, conservative2026-11-02 00:20:03 UTC start,
00:30:03 checkpoint,00:40:03 ceiling. Record actual first edit. The slot window
closed early; no earlier budget is extended.

Exact scope: common_objs initialization loop in pylifecycle.c (disjoint from
C's watcher); common_const_is_initialized/common_constants_clear in pystate.c;
LOAD_COMMON_CONSTANT in bytecodes.c; get_const_value in flowgraph.c. Add one
common_constant_storage fixture and method registration in _testinternalcapi.c
and a new test_pyvalue_constants.py. Necessary existing internal includes only.
No public header/layout change. Canonicalize -1 at initialization; DUP supports
inline; detach before pointer-only shutdown; return an owned fallible object
from the compiler's object boundary. Retain heap-immortal shutdown policy.

After A freezes the local/slot kernel, root takes this exact uop hunk; A remains
the sole generator writer. Do not race bytecode edits/regeneration. Inspect
actual generated interpreter/helper loads and retain unchanged IDs/formats.
Tests inspect actual table kind and require a real LOAD_COMMON_CONSTANT minus-one
route, not merely a literal that might choose LOAD_SMALL_INT. Exercise owning
compile/fold and ordinary None/bool/empty constant behavior; runtime remains
unexecuted until coherent source exists.

Cheap gates: exact source/reference review, AST and affected generator host
tests, required regeneration, at most two actual changed syntax-only TUs30s
each. No objects/link/candidate/timing. Wider flowgraph, init and native consumers
remain separate required closure; this cut does not claim their completion.
