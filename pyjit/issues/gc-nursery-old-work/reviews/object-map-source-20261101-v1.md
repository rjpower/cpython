# Object-map inventory: independent source review

Decision: **ACCEPT `c43c8374dbd56ca7862be49a31820ded6ac5a9d9` for bounded
fresh-debug feedback**. Reviewed on 2026-11-01. This permits the coordinator to
review concrete build/feedback helpers; it grants no execution, performance,
qualification or landing credit. Actual failed-prefix mark cleanup remains an
unfulfilled witness, distinct from the local address-oracle negatives.

The clean source is `build/worktrees/gc-object-map-inventory`, based on
`47ccf35e21a94fde9351d1c2e4571bf207257a4c`. Its frozen
`build/object-map-source/capsule.json` SHA-256 is
`e1f3354da23d38d6cdff3146b1c40417f61a43537f2bc2967dfcb67154ad0cad`.
I reviewed all five changed files and independently checked 6,947 source
entries, modes and symlink texts, all capsule artifacts and six exact child
strings. The child strings were compiled on the host without execution. The
C root checker reports zero errors. Evidence:
`build/gc-object-map-source-review-20261101-v1/receipt.json`, SHA-256
`a70f7f1e87cb727211d5a49ebdafa1483d3a59b7016cd0e9831fb412fd3232f6`.
The first host audit assumed all manifest entries had hashes; its correction
for the four explicit link-text entries is preserved separately. No author
artifact was rewritten by this review.

The production delta follows the accepted design. `live_visit_heaps` keeps
object-route selection separate from its false block-visit argument. Domain
iteration, head-lock scope, thread heap selection, subprocess/tag filtering,
abandoned-page visits and MEM discovery retain their prior structure. The
existing allocator APIs still collect pending frees and supply page metadata.
The object classifier is byte-identical, SHA-256
`0422fb27a0b807b739f5032b6dd25d865f1abb92e3b57d9cbf10b82b1d6b0fa6`.
Old seeding, classification, promotion and object listing reuse it; no new
lifecycle policy or allocation registry appears.

`live_map_area` checks alignment, nonzero stride, committed divisibility,
used/capacity, usable size, debug offset/header fit and endpoint overflow.
Singleton pages test their one canonical start, so a huge object's payload
does not become a bitmap scan. The general walk masks committed endpoints,
advances across absent chunks without allocating, and validates canonical
stride after subtracting the debug offset. Eight-byte aligned endpoints keep
the partial-word shift strictly below 64. A validated canonical block cannot
extend past the committed last block. Production map reads retain the
existing supported allocator/address-space assumptions; this is not an API
for arbitrary externally supplied page descriptors.

The publication/free boundary is unchanged. New-reference publication,
successful and failing realloc, ordinary free, dead-block free and quarantine
retirement retain their existing map updates and diagnostic hooks. I compared
the relevant object, allocator, collector-lifecycle and mimalloc files with
the parent byte for byte. Direct backing object free/realloc calls remain in
the guarded public object allocator paths; raw MEM/NOSCAN paths do not gain
initialized-object enumeration. Constructor SHALLOW/readiness rules, old-root
closure, weakrefs, resurrection and individual storage reclamation remain in
the unchanged consumer/lifecycle code. Their inherited contracts are not a
substitute for the nominated native regressions.

A new rejected geometry can stop a classification prefix, so the wrapper
adds a cold allocator-based keep-everything walk. With `.failed = 1` and a
result, the unchanged classifier clears marked/shallow bits without adding
death records or calling lifecycle code. The traversal does not short-circuit
on `t.oom` or that preexisting failure flag; those affect its final return.
The outer failed mark still clears the prefix death flags and rejects its
death array. This is a credible static recovery path, with no successful-path
policy switch. However faults 1–3 in the new private oracle do not execute
that recovery. A temporary separately reviewed source ablation or explicit
test point must force a real failed production prefix and prove mark clearing
and later recovery before broader acceptance. No such execution is credited.

The private oracle compares address/usable-size/tag records before header
classification. Both capture arrays are reserved before freeing the nominated
tuple. A same-page tuple keeper is checked, and the interval from free through
comparison/bit restoration contains no Python allocation, callback or
collection. In-place heapsort avoids a fallible libc sorting allocation.
The stale bit is cleared after both capture calls even on their ordinary
failure returns, before unlocking or freeing fixture storage. Publication,
raw-clear and actual free-clear checks supplement the shared map gate; set
equality alone cannot prove publication completeness. The known object must
appear once and the freed address must be absent from the allocator reference.

The four methods assemble six children: ordinary comparison, three local
oracle faults, huge singleton and an exited-thread object. The last two
require actual singleton/chunk and abandoned-page observations. Two consecutive
tuple allocations may fail to supply a same-page keeper, and fixture setup
could reclaim an abandoned segment before observation. Those are explicit
missing-witness failures, not silent passes, retries or collector defects.
Preserve their first actual results.

No concrete source defect blocks a first reviewed debug batch. Actual debug
offsets, linked precise-root audits, zero ordinary true-block visits, failed
prefix cleanup, publication/realloc/free/reuse/quarantine, native owners,
minor/major lifecycle and OOM recovery remain required feedback. The source
may still compile to an indirect per-object consumer call. The historical
12.1377% inventory umbrella is not recoverable elapsed time; removal of
free-list reconstruction is the supported source claim. Matched elapsed
benefit and qualification remain separate decisions.

The review paused at 01:22:01 UTC for the coordinator's additive retention
helper correction, resumed at approximately 01:23 UTC, and finishes within
the original 01:35 UTC ceiling. No source/compiler/target/workload/timing
command ran here; the source and its capsule stay frozen.
