# Maintained object-map inventory: independent source review

2026-11-01 06:09 UTC. **ACCEPT the exact source for separately reviewed focused
native feedback.** No blocking source defect found. This is not execution,
performance, qualification or landing acceptance.

Reviewed clean `dc2ec2c059152933e8804f5f997e81370883460f` in
`build/worktrees/gc-object-map-maintained`, based on
`417a9443a599e879c76c34c13ae76943bd6f6b3e`. Frozen capsule
`build/object-map-maintained-source/capsule.json` SHA-256:
`e47e0567c72b2908d68950a56ae65636854a0a6d112f0ebf1ca9b129b9f7182a`;
patch SHA-256:
`1fd8116693b92a9f600ec96fd5e58bc3b7b29fe87aec55067fc0dc20335ac3c6`.
All five changed files, three design/review inputs and nineteen artifacts
were independently rehashed; the patch matches the committed diff. The
classifier is byte-identical to the maintained base. Release iterator
projections match the previously reviewed c43 implementation; maintained
mark-failure policy and five allocator/lifecycle files remain unchanged.
This verifies the port rather than inheriting c43 acceptance by ancestry.

The production boundary is coherent. OBJECT/GC pages use existing pinned
domain, attached-thread and abandoned-page routes. MEM-page discovery keeps
its existing callback. Map enumeration validates geometry, committed bounds,
debug offset, canonical starts and bitmap endpoints before classification;
huge singleton pages inspect their sole start. Mimalloc's actual area fields
match those assumptions. Publication, realloc/free/quarantine updates, object
ownership, nursery policy and the classifier remain outside the delta. No
new root, registry or allocator policy appears.

The revised failed-prefix witness closes the previous source-stage gap.
Independent true-block counts run before production visitation, after its
visitor has unwound, after cold clearing and after outer rollback. None is
called recursively from a page or abandoned-page callback. The wrapper
counts actual MARKED/SHALLOW transitions through the unchanged classifier.
It aborts only with a nonempty real death prefix and strictly positive,
non-underflowing remaining counts; it consumes the trigger before cleanup.
The independent pre-clear count must reconcile both exact remainders and
the actual DEAD prefix before `before_valid` receives credit. HEAD-lock
scope and domain pins cover this interval; there is no intervening Python
allocation, callback or death dispatch.

Cold clearing runs even if reconciliation fails. Its failed walk still
visits allocator blocks and clears marked/shallow bits without adding dead
records. The unchanged outer failure path clears prefix DEAD bits, frees
and rejects the array, and returns failure before the collector's lifecycle
dispatch. Unavailable counts remain -1. The fixed following collection must
reclaim garbage and preserve the rooted payload. These assertions require
actual native evidence; structural guard mutations alone do not prove them.

The cold API uses exported manual `_PyGC_Collect`, so automatic GC being
disabled does not turn the fixture into a no-op or require a hidden extension
symbol. It rejects active context/collection and enabled young admission;
its context is removed before error propagation or packaging. The result
dictionary and every boxed integer have explicit precise roots. The module
also roots the returned result while restoring tracking on every initialized
held list, including partial allocation failure. The volatile untracked
lists in an ordinary opaque extension frame can supply genuine conservative
SHALLOW roots; interleaved unrooted lists can supply deaths. This remains a
native classification obligation, not a source assertion or manufactured
header flag. Missing positive witnesses must fail without retries.

The address oracle retains its independent allocated-block reference,
checked same-page keeper, reserved scratch arrays and allocation-free
free-to-bit-restoration interval. It tests omission, committed-boundary and
stale-start disagreement before reading object headers. Actual singleton,
chunk-crossing and abandoned-page observations remain mandatory. None of
these local oracle faults substitutes for the real failed production prefix.

Independent host replay passed 22 root methods (21 ordinary passes and one
existing expected failure), three C/header root audits, seven structural
guard mutations and the missing-result-root mutation. Five methods assemble
seven children, each compiled on the host without execution. Release has
exactly one named debug-hook skip. Reviewer outputs are isolated under
`build/maintained-object-map-source-review-20261101-v1`; receipt SHA-256:
`c0f60bd96f6bc0a9ec4e4ba674eeedc47366acc00fe25d6b1d659235284d7181`.
The first replay comparison rejected the explicit reviewer Python 3.14 path
versus the author's `/usr/bin/python3` label after the controls passed;
its retained log and correction compare every other field unchanged. Initial
shared-cache sandbox refusal is also retained. No author artifact changed.

Native builds, linked/root audits, real failed-prefix cleanup and fixed
recovery, publication/ownership controls and emitted-path evidence are still
pending. Elapsed benefit remains a separate decision; historical inventory
percentages are not a speedup estimate. No compiler, target, workload,
counter probe or timing ran in this review. The review paused during the
coordinator's quiet window and resumed after actual terminal/idle release.
