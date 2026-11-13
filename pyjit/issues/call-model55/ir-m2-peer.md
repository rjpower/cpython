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

2026-11-12 integration fixture follow-up: independently reviewed the integrated
`test_code.py`-only delta. Source GO. Old consumed-home/NULL-copy/discarded-save
witnesses now assert early construction rejection; valid-image runtime controls
retain actual line jumps and check reconstructed call state or restored outer
binding. The distinct-local witness changes only STORE_FAST destination B,
preserving the second saved source A, so normal transfer remains valid while
the observer must still reject missing restoration. No native execution was
performed for this peer; root owns the focused rerun.

Profile-led fast retirement contract peer GO, before implementation. Existing
`PyValue_RefcountOnObject` exactly matches XCLOSE's count classification,
including NULL/index/scalar/no-count values. Under the GIL, a count greater than
one followed by existing `Py_DECREF_MORTAL` cannot reach deallocation, weakref
or reftracer-destroy callbacks. Reusing that primitive preserves statistics,
ref-debug totals/static-immortal checks and dynamically immortalized untagged
owners; do not duplicate raw refcount writes. Counts at most one remain
unconsumed for outlined retirement and its existing diagnostics. FT counted
values always take that latter route.

No callback/poll occurs between a true result's non-last decrement and the
caller's home-NULL/cursor store, so that store may follow consumption. False
must leave the home and count intact before slow roots link. Retain existing
debug home validation and avoid admitting wrapped/borrowed mortal homes. Limit
the first source consumer to the actual TryClear predicate/XCLOSE pair. Native
mask/branch/stack work and matched elapsed/PMU evidence must establish any gain;
typed-header tests and other retirement costs are not savings from this helper.
This peer authorizes no source implementation without coordinator GO.

Fast-close four-file source postreview GO after coordinator authorization:
helper uses the existing classifier and decref, TryClear retains a false home
and NULLs only after true, with no intervening callback. Probes balance normal,
last, no-count and both wrapped-pointer encodings (a real two-reference object
is used in release), verify pending-exception identity, and use the existing
unregistered stack-object precedent for a prior owner dynamically immortalized.
No allocated immortal leaks or extra ref-debug total mutation is introduced.
Five focused methods exercise count delta, last-owner retention and exactly-once
finalization. Source only; role runs and native/performance admission are root
gates.
