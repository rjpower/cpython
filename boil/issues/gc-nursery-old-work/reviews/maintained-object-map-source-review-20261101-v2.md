# Maintained object-map audit correction: source review

2026-11-01 06:28 UTC. **ACCEPT exact
`3a9f70e065bf4112ef3a070746011df7b08892f3` for separately reviewed fresh focused
feedback.** No blocking source finding. This supersedes the repair preference
in the failed-v1 outcome; it does not relabel that failed build as passed.

The clean successor has only `Python/gc_trace.c` and its results document
changed from `dc2ec2c059152933e8804f5f997e81370883460f`. Capsule
`build/worktrees/gc-object-map-maintained/build/object-map-maintained-source-v2/capsule.json`
SHA-256 is `e913d5f923c2ee144370bcf943534801728f13d0911916765d5c5d0069bb6a0c`;
patch SHA-256 is `b09a88cabb468b9926ef124c6d70eae033cb08bd756900cfbb7b87a0f837d29d`.
The committed diff, two changed files, seventeen artifacts and original source
capsule binding were independently checked.

The correction removes the new precise adapter entirely. Debug scalar
observation now follows the existing MARKED/SHALLOW clear store inside the
original opaque allocator callback. That branch already establishes a
published object, increments the result object count and tests the old MARKED
bit. Its exact active-owner/armed guard, actual post-store checks and bounded
counters therefore retain the adapter's transition meaning. Overflow fails
the walk; unconditional cold clearing still runs. The independent initial,
reconciled pre-clear, post-clear and rollback observations, real nonempty
death prefix, consumed trigger and fixed recovery assertions are unchanged.
The cleanup walk has a different owner and cannot count or retrigger.

Both normal and fault page paths now use the ordinary `live_walk_block`
callback. Release projection of its entire body matches the maintained base,
including the original type snapshot, weakref calculation, kinds expression
and all header stores. No per-death root, type reload, precision conversion,
effect-table entry, audit exemption or vendor-header change remains. This
avoids expanding a small debug witness into the legacy collector closure.
Existing collector entry excludes its own deeper frames and the active
collection guard prevents recursive marking; the observer adds no allocation,
callback, object reference or new boundary to that convention. Fresh linked
audits must still accept the actual compiled image; future devirtualization
is not exempted.

For GNU compilers the nonzero 64-bit bitmap uses `__builtin_ctzll` directly.
It has the same bit index as the former size-dependent `mi_ctz` sequence;
the original non-GNU fallback remains. No zero input reaches the intrinsic.
This addresses missing inline-body visibility rather than declaring arbitrary
callbacks pure. In particular, `_PyObject_IS_GC` retains its opaque `tp_is_gc`
effect and original operational placement.

Independent host replay passed 22 root methods (21 ordinary passes, one
existing expected failure), three root audits, seven structural negatives,
the missing-result-root negative and seven assembled child compilations.
All seven child hashes and the single named release skip match v1 exactly.
Only reviewer output paths were redirected; source and author artifacts stay
unchanged. The reviewer uses explicit host Python3.14, with every other
reported check compared unchanged. Evidence and receipt are under
`build/maintained-object-map-source-review-20261101-v2`.

No compiler, target, workload or diagnostic ran in this review. The failed-v1
native audit, its unused annotation/root-span attempts and all prior evidence
remain preserved. New linked/root audits and the actual strict native witness
are still required before elapsed readiness; no performance, qualification
or landing credit is granted here.
