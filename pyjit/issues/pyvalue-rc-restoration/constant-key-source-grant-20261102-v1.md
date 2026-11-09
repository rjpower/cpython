# Canonical constant-key producer source grant

Adopt independently accepted producer design
build/pyvalue-rc-restoration-design-20261101-v1/constant-key-producer-cut-v1.md,
SHA256ce91a74b51a260bdab760023233587d4f5ddb53986738fc582e5adeff0125c3d,
reviewc02823f62fa2ead0ec80b185d04aa01b5eca7da7b68082aac05f4a788d0dc5a3.
Root implements; D reviews actual ownership, exact key semantics and tests.

Separate45-minute source cut: conservative start 2026-11-02T01:02:39.000Z,
checkpoint 2026-11-02T01:17:39.000Z, ceiling 2026-11-02T01:47:39.000Z.
Record actual first edit. Prior compiler-consumer window is closed/accepted;
this is not an extension. No source grant to the later cache/interner design.

Exact source: new _PyCode_ConstantValueKey and one bounded tuple-key construction
helper if necessary, old _PyCode_ConstantKey owned-object adapter body in
Objects/codeobject.c; internal pycore_code.h declaration/necessary includes;
one _testinternalcapi adapter function/entry and focused methods in existing
test_pyvalue_compiler_constants.py. Other codeobject readers, compiler cache,
tuple mutation, VM/generators and native families remain outside scope.

Borrowed canonical input is pinned before allocation. Return independent owned
canonical key, NULL1+error on failure. Preserve actual existing typed-key policy:
exact scalar/tag differences, negativezero/complexfourzero tags, heapNaN identity,
recursive tuple/frozenset keys, slice representation and unsupported object-id
key. Bounded pair/triple C arrays remain independently rooted across allocation;
no mutable container array passed as stable external storage. Frozenset iterator
returns owned values with checked status; partial tuples and recursive owners
close exactly once preserving primary. New tuples fill only once.

Old object entry remains a fallible owned adapter; retain canonical key through
AsObject result ownership then close it. No scalar box cache, borrowed getter,
public API removal or hidden storage mode. Legacy object identity is governed
by the selected canonical scalar policy, with NaN objects retained.

Cheap gates: source decisiontree/owner/error review, AST and exact declaration/
call consistency, then at most3 meaningful bounded syntax-only TUs30seconds
each, reporting wider codeobject/Clinic failures accurately. No object output,
link, candidate/runtime execution or timing. Tests inspect real value keys and
identity/ownership/failure recovery; host parsing alone is not runtime evidence.

Actual first header write2026-11-02 01:04:45.956375774 UTC. Concrete v1
checkpoint8b927116 at01:07:43; corrected v2 receipt6f64aa7c at01:09:29,
both before01:17:39. Review corrected frozenset construction to fresh empty
plus AddValue, avoiding an ordinary iterator's scalar boxing. Real fixture TU
and exact unchanged producer/adapter projection under real headers pass syntax;
whole codeobject retains old Clinic/intern/read failures. No API shims or error
suppression are used in the projection; it is not whole-TU qualification.
Thirteen methods are AST-only, six new. A32-slot tuple exceeds actual freelist
limit20, so the one-shot OBJ allocator fixture reaches real key allocation,
restores the allocator before error/cleanup and checks recovery. Independent
review97607e27 accepts v2 as dependent source; close this window early.
