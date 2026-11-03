# Canonical repr-guard source cut

Adopt D's critical review of
build/pyvalue-rc-restoration-design-20261101-v1/repr-guard-cut-v1.md,
with the mandatory conditional insertion refinement below. Root implements;
D reviews actual source and witnesses. This separate cut does not extend any
prior window.

Conservative start 2026-11-02T01:46:26.000Z, checkpoint
2026-11-02T01:51:26.000Z, ceiling 2026-11-02T02:01:26.000Z.
Record actual first edit.

Only Py_ReprEnter/Py_ReprLeave in Objects/object.c, at most one main internal
C fixture/entry, and a focused new Python test file. C's four accepted external
hunks and A's future whole GetMethodValue body remain untouched.

Pin the canonical argument before allocation, own current thread dict/list
through all callbacks, scan via canonical identity with no box/user equality.
After a miss and new list allocation, DO NOT unconditionally replace the guard:
allocation may reenter and install one. Use accepted SetDefaultValue to return
the actual current/inserted owned guard, retaining the independent candidate
owner through that call; validate actual pointer-kind/list. Append a duplicated
canonical value through consuming AppendTakeValue. Preserve normal recursion,
equal-distinct object behavior, scalar type/zero/NaN identity and checked errors.
Leave detaches/removes before cleanup and restores exact entry exception;
never use a cached array after release. No metadata format, scalar cache or
legacy API removal.

Cheap source/lifetime/exception review, AST and at most2 meaningful actual
syntax-only TUs30seconds each. Add an actual C enter/leave/primary-preservation
fixture and real recursive/custom repr witnesses. No object/link/candidate/
runtime/timing. A passing Object TU does not qualify the whole runtime.

Source is frozen within this ceiling. Actual first edit01:48:26.495439 UTC;
concrete v1 checkpoint01:49:31.938883 meets the early checkpoint. V2 fixes the
fixture's failure-path duplicate-registration cleanup before the ceiling.
Final independent review522be114 binds author612dc1da and actual corrected
patcha81845e8. The v2 delta-correction record supersedes only its inherited v1
patch-hash field. Both actual Object/fixture syntax checks pass; four methods
remain AST-only. This grant is closed; no candidate/runtime acceptance.
