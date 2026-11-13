# Semantic transfer worklist peer review

2026-11-12: GC lane source review of the IR lane's generated normal-transfer
query, encoded admission worklist and focused tests. Disposition: GO after the
owner-join correction below, with exact-source integration updates still needed.
No compile, generation or candidate tests were run by this reviewer.

The queue's per-block queued flag bounds pending entries by block count.
Scratch vectors cover actual admitted geometry; branch/domain checks precede
block indexing, allocation arithmetic is guarded, and semantic read inspection
runs only after convergence. Parallel COPY/SWAP sources are read before any
destination facts change. Exception/observer entries seed unknown facts, and
unsupported rows drop logical/owner authority rather than inventing it from
physical preservation. This is bounded hardening, not M4 map admission.

Must-fix resolved: the initial queue meet intersected owner bits, losing an
unretired owner present on only one covered predecessor. The reviewed plan
requires owner obligations to merge independently of initialized availability.
The corrected `SEM_MAY_OWNER` vector unions predecessor obligations, while NULL
and initialization intersect. Its bit proves neither readability nor a mortal
count. The mixed finite lattice remains monotone. New all-covered diamond tests
exercise either owner-producing arm, both condition senses, final exit versus
overwrite, and both queue arrival orders.

Exact source-token checks plus IO shape constrain the supported row subset.
Changed/synthetic bodies become UNKNOWN, and tests mutate these bodies to check
that behavior. Public construction/replacement/marshal tests cover consumption,
transport, joins, loops, unknown barriers and full scratch extent. Existing
completion/provenance/nullability proofs remain separate.

Integration requirement: `canonical_frame_transfer_input` still expects the
old return clear call, while the reviewed M2 producer now returns the same
owner through `_PyEval_FrameClearAndPopWithResult`. Root must migrate that exact
template and its tests to the new consuming/returning contract. The yield proof's
old PyValue sentinel guard likewise needs the typed-NULL source spelling.
Keep exact proof checks; do not bypass them. Generated metadata, source coverage
tests and both-role candidate execution are coordinator gates.
