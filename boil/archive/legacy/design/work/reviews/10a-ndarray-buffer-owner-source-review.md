# ndarray buffer owner: independent source review

Verdict: **SOURCE_CLEAR**, scoped to frozen diff
`88ec3da201c789b08e3a89d80cf27a41f3ae0460ed5b6ca6b7e2b1330db9032b`,
checkpoint `3cb6c5ff`, on base
`db0256990c26209d771d3ab4543a98807acb586d` in `boil-buffer-ndarray`.
All five checkpoint file identities were rechecked. No owner-file edits or
native execution. The reused managed-buffer API was separately approved by
root; this reviewer authored that earlier API/Unpickler change and does not
independently clear it here.

`Modules/_testbuffer.c:278` acquires directly into the stable owner master,
then publishes a borrowed cached view and static head without a collecting
operation between those stores. Owner traversal retains the original exporter;
deallocation frees only cached arrays marked ND_OWN_ARRAYS and drops the owner,
never releases the transformed cached copy. Legacy obj==NULL/static-storage
contracts remain distinct from owned callback-bearing acquisitions.

Dimension reduction advances cached metadata pointers only. Slicing and
add_suboffsets use copy_structure before taking ownership of cached arrays.
Its allocation failure path frees only newly allocated arrays and leaves the
old borrowed pointers intact. The original owner's metadata is never passed
through those transformations or freed by the consumer. Child owners acquire
their parent, retaining parent-owned metadata until their own release completes.

The one-shot constructor gate precedes conversion/acquisition, rejects recursive
init, and removes replacement/ABA without another lifecycle state. A failed
first init has no published head and can retry. This is an intentional private
API restriction, not a claim to support arbitrary producer mutation during
conversion callbacks. Existing producer push/pop conversion-reentry hazards
remain explicitly outside the port's proof.

All fourteen getsets, raw data/metadata operations, hash, item/slice/assignment,
getbuf and direct module contiguity access have the active gate. Across Python
conversion/collection, a live consumer traces a sealed owner that cannot be
explicitly released through Python. On automatic owner release, RELEASED is set
before callbacks; resurrected ND readers therefore reject before dereferencing
the cached exporter or arrays. Releasebuf, traversal and deallocation correctly
remain usable for bookkeeping after closure.

Ordinary getbuf increments head.exports and owner.exports only after successful
validation, with overflow checks. Consumer releasebuf decrements both and runs
no upstream release or callback. Producer internal-token behavior stays separate.
ND_REDIRECT still acquires independently upstream and does not increment either
ND counter. This distinction is necessary for readiness; the owner's exports
count is not a reference count. Closure retention and queued ready releases
remain the base collector's policy, not a new ND scheduler or universal graph
progress proof.

The three ordinary ND tests cover callback/resurrection/closed access, original
versus transformed metadata and multiple outward leases, and failed/recursive
or repeated init. Their external event/error assertions catch callback failures.
The omitted tobytes-gate negative uses a contiguous cache over globally live
backing, so the intended named readability failure does not require stale
storage. The inherited scanner .group()/temporary-acquisition fixture problem
remains acknowledged; its successor must be composed explicitly.

The migrated retention/OOM fixture restores the synthetic count in finally and
retains zero-sweep/fired-hook/retained-closure/eventual-release assertions. Its
new exact-MV blocker is private test machinery, not a real acquisition/root.
Important qualifier: unblock checks exports==1; it cannot distinguish a real
sole external export from the test's synthetic one. Approved tests establish
no genuine export and use a matched pair. Describe this caller discipline,
rather than promising general unmatched-unblock rejection. No extra registry
or production field is needed for the approved isolated seam.

An inherited functional limit also remains: ND_REDIRECT forwards to cached
base.obj. A Python __buffer__ acquisition can put `_buffer_wrapper` there,
whose table has no bf_getbuffer. That forwarding path can reject a live request,
like the separately recorded PickleBuffer re-export issue. This is unchanged
by the port and is not proof that live Python-wrapper redirection works.

Host-only checks: complete test_gc AST and five affected child programs compile;
all five source identities match the checkpoint; diff whitespace check passes.
Native callback ordering, export balance, actual kill/ASan behavior and final
safeguard removal still require the actual composed source and root's gate
grant. No production/native PASS follows from SOURCE_CLEAR.
