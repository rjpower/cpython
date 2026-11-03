# Canonical compiler cache and interning consumers

Adopt D's critical review of compiler-cache-consumer-cut-v2.md in
build/pyvalue-rc-restoration-design-20261101-v1. Root implements; D reviews.
New45-minute source cut: conservative start2026-11-02T02:05:00Z,
concrete checkpoint02:20:00Z, ceiling02:50:00Z. Record actual first edit.

Only named Python/compile.c cache/index/recursive merge/ordered constants/
interning functions, private canonical helpers and focused compiler constant
tests. Retain all old object entry points as explicit owned/fallible adapters.
No legacy dictionary removal, type/cache/tuple implementation or VM edits.

Pin canonical input/key/container owners before recursion/allocation. Use
accepted typed-key production and conditional insertion (0/1 both successful
owning output). Recurse only newly inserted constants. Frozensets use owned
value enumeration and fresh New(NULL)/AddValue. Scalar constants/indices stay
canonical. Object bridge conversion succeeds before old pointer publication/
retirement. Slice fields cross an explicit owned object boundary.

Shared tuples may already be AST inputs or hashed dictionary keys. Private
replacement consumes only typed-equivalent merge/intern representatives, with
unchanged equality, hash and cached hash, including signed-zero/type/NaN identity.
Publish before displaced close. No public setter weakening, uniqueness claim,
arbitrary comparison or cached raw array across callbacks. Final list replacement
uses checked consuming setter.

Checkpoint actual canonical insertion/index source and actual syntax diagnostics;
then recursive interning/fixture AST/exact independent review. Wider compile.c
diagnostics remain explicit. At most2 meaningful syntax checks per changed
revision; no object/link/candidate/runtime/timing in this source grant.

Window closed early. First source edit observed no later than02:05:45 UTC;
concrete checkpointfa7b288e at02:08:21 meets the15-minute checkpoint. Final
v2 author709c89c9 and independent review94d257be accept the exact named source.
Review corrected explicit negative iterator statuses and added actual slice/
same-NaN hash witnesses. Eighteen methods are AST-only (five new). The exact
unchanged selected-body projection passes actual syntax with real headers;
full compiler metadata was a separate seven-read dependency, subsequently
closed under its own source grant. No runtime qualification.
