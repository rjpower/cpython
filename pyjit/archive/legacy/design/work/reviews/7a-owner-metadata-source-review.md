# Queue2b owner metadata: independent source review

Disposition: **SOURCE_CLEAR for bounded native validation**, 2026-10-30.
This is the registration/access and destruction-discovery slice only.
It does not establish complete owner safety, native correctness or landing
readiness. No author source was edited and no compiler, target interpreter,
test or probe was executed in this review.

## Frozen inputs

Reviewed `boil-queue2b-owner`, base
`168f30a96ffdb5516fe94e9ed13b0e60dcb9fae6`, and its complete fourteen-file
uncommitted delta. Evidence under `build/Queue2b-owner/source-v1/`:

| Input | SHA-256 |
| --- | --- |
| source-map.json | `bc0eb105f9acbb3f1a0533ccd4a8a82b042519d44d838be689ca3a6ac7b2eb19` |
| source.diff | `69cafe55640431bfb93fe9deeb632515fe157685a6fc3adcf4e8f1ddaad98cc3` |
| checkpoint.md | `8c2ff8df2d5a7ca92621fa0d723c7595a25a96894bcc9098e1bb8cd99beea6a3` |
| negative-collecting-owner.diff | `c53502500c0e92cadb849561a091731cf11b1c9a175c2fd31a210bf858ad2967` |

Independently checked the base, every listed source hash, exact changed-path
set and equality of the saved diff with `git diff --binary`. `git diff
--check` and read-only negative patch applicability pass. Read the applicable
agent instructions, CPython AI policy, PROCESS compatibility ruling, note
13 section 3.4 and the Queue2b work record before assessing the patch.

## Source findings

No blocking finding in the bounded slice.

- `Python/gc_trace.c:1725`: `_PyGC_LiveOwnedBy` compares the segment's
  subprocess with live interpreter membership while holding HEAD. HEAD
  uses `_Py_LOCK_DONT_DETACH`; the segment lookup is an atomic map read,
  and Head/Next are direct list accesses. There is no allocation, callback
  or Python execution inside the comparison. It returns a boolean, so no
  unpinned foreign interpreter pointer escapes. A valid caller-owned
  object argument supplies the object lifetime; this helper does not make
  arbitrary stale pointers valid. Static, unknown and deleted owners fail
  closed, while a heap immortal keeps its allocator owner.
- `Objects/dictobject.c:6230` and `:6253`,
  `Objects/typeobject.c:918` and `:949`: Watch and Unwatch both preserve
  existing object-kind and current-table ID validation before ownership
  checks. Foreign rejection precedes all watched-bit/version mutation.
  Reserved dictionary IDs bypass only the owner guard. Static types are
  explicitly rejected on both paths; there is no immortality shortcut.
- `Objects/codeobject.c:1561` and `:1590`: GetExtra initializes the result
  to NULL and leaves foreign slots undisclosed. SetExtra preserves the
  existing type/index error precedence and rejects foreign writes before
  mutation or free callbacks. Existing publication-before-old-free,
  growth failure and destroy-time array detachment remain unchanged.
  `_PyCode_HasExtraFree` now consumes the explicit discovery owner; its
  declaration and sole caller agree. The fixed free-function array and
  existing index allocation bound are unchanged.
- `Python/gc_trace.c:2215` and `:2235`: destruction classification uses
  the current heap-domain member, rebuilding its dictionary/type callback
  masks before both active-thread and abandoned-segment walks. Function,
  code-watcher and code-extra discovery consequently consult that owner's
  tables. Existing HEAD-held traversal and domain-pin routing contracts
  remain in force. This introduces no callback under HEAD and makes no
  new cross-own-GIL object-sharing promise.

## Controls and evidence limits

The seven new queue tests cover foreign Watch/Unwatch collisions, isolated
local registration, foreign extra access and four owner-only destruction
table discoveries. Static type rejection covers both operations. The
existing bad-ID/non-type tests remain applicable. Child-mode assertions
come from the existing `_queue_child` protocol; actual execution still
needs to establish modes, skips and callback outcomes.

The copied negative restores collecting-interpreter discovery and masks,
leaving the API guards and tests unchanged. With owner-only callback/free
tables, its intended failures are the named event/token assertions, not a
startup error or crash. It is a meaningful proposed causal control, not
executed evidence. Separate guard omissions can distinguish each of the
four Watch/Unwatch rejections. No raw-address fixture or held historical
finalizer probe is used.

The tests compare callback membership/counts with the observed dead-weakref
subset, allowing conservative retention. Error-callback cases observe the
existing unraisable path. They do not independently prove the absence of
every unexpected unraisable error. The ctypes free-callback case records
Python callback interpreter identity; GIL-state entry can obscure the C
free-function entry identity, as the checkpoint already states.

Two nonblocking documentation corrections are appropriate at handoff:

1. `boil/design/work/7a-2b.md:6` and `:20` retain the old unimplemented
   status and `_PyGC_LiveOwnerOf` name. The appended source status and
   actual boolean `_PyGC_LiveOwnedBy` interface should be the current
   description.
2. `source-v1/checkpoint.md` calls dead weakrefs "actual reclamation".
   Destroy-hook staging can clear weakrefs before physical sweep. These
   assertions prove logical death plus the observed delivery subset;
   physical reclamation is a separate claim. Keep the frozen record and
   clarify this wording in its successor rather than relabeling evidence.

Ordinary foreign dictionary/type/function mutation events still consult
mutator tables. Registration rejection does not resolve existing foreign
mutations. Closed-owner admission, table freeze, retirement/adoption and
fork lifecycle remain Queue2c work. The current-interpreter assertion and
HEAD membership comparison are not a new post-close API guarantee.
Those limits are explicit in the checkpoint and are required in any later
qualification or landing claim.

## Addendum: precise argument root after first build rejection

Disposition: **SOURCE_CLEAR for the narrow proposed root-span successor**.
The initial source review's no-callback/no-detach analysis did not catch
the formal argument-root obligation of this precise function. Preserve
the original release make failure: native make2 at `gc_precise.stamp`,
four findings for unspanned `op` across HEAD/segment/Head/Next calls. This
was not a successful build or target validation.

Original committed source is clean `2d613fc8784bd3a7d89687f8dbf6e4a34e8231e2`.
Original `build/queue2b-validation/release/build-result.json` SHA
`3c1d0c32fccb9a6266bbe5233d83f24ce2473bbfffddb10725bdbc03d3e128b8`,
and `build.log` SHA
`eb49018d83f93198b9186312b8d9d1dfba716f79d185b8f1d158ba07cc4f4939`
remain unchanged. No native successor was run for this review.

Reviewed task-copy proposal under `build/Queue2b-owner/root-span-proposal/`:

| Input | SHA-256 |
| --- | --- |
| root-span.diff | `cfae4dea74a3a264a300d3ab96630b97e8311b4f2a84cf707ba7c82b20c45dd0` |
| gc_trace-proposed.c | `6d590757e4378a068d595e560d8b88dbafdc51475204c987c55c67967b512c5c` |
| source-check.log | `2c8476b674c5df5da1eb31e6318fb8bb8c0aa113b08d63e4d7570547095ce9c7` |

Independently verified that the saved diff is exactly the difference
between clean original source and proposed copy, changes only this helper,
and applies without modifying original source. The author's host parser
log records the original four findings and zero for the proposed body;
that log is not a successful native build receipt.

The existing one-slot value span is linked to the CURRENT thread-state
root chain and filled with `_PyValueSpan_AppendObject` before HEAD is
acquired. Its raw object pointer anchors the actual object without boxing
or inline conversion. Push/append are allocation-free stores; no collecting
call intervenes before coverage. The single exit unlocks HEAD before the
LIFO pop. Neither the stack-local slot nor its span address moves or escapes.
`_PyGC_TraceVisitThreadState` visits the appended slot through the existing
chain; no new root framework or effect annotation is introduced.

Rooting does not make collection inside this critical section permissible.
The independent no-Python/no-allocation/non-detach proof still holds:
HEAD uses DONT_DETACH, segment lookup uses atomic metadata loads, Head/Next
read scalar links, and the body invokes no callback or dependent lock.
The attached current interpreter and HEAD-contained membership comparison
retain the prior lifetime proof; the return remains boolean-only. Source
clearance applies to this exact hunk, with original native failure retained
and a newly bound native build still required.
