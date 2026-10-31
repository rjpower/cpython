# Independent source review: retirement oracle correction

2026-10-31, completed at 21:44 UTC. **ACCEPT this exact test-only source for
separately activated fresh-debug feedback.** No blocker found. This does not
accept behavior, authorize a build or supersede the original failed evidence.

Candidate `856222c62458bf3648e51d0bb0d278b1fbf54bd7`, tree
`e0773b9e3e553bc27470fe573eb6426c6f1897e0`, is clean in the isolated
`build/worktrees/nursery-adoption` checkout. I reviewed its complete five-file
diff against `b34931a881b33b47a2b2f4b1b2a644c50fed963b` and the relevant
unchanged retirement function and fixture paths. The accepted design is
`retirement-oracle-design-20261031.md`, SHA-256
`6799cdd3f471ecc3fb0b97e2c0b4210cd46670ce23a5f8e1bcb4af6ac1862a36`.

The live `_PyGC_TraceForgetObject()` is byte-identical to the parent, SHA-256
`02d9341c448b03b847c124b23e83fc4d662488092d62c3d760f74d237386d658`.
No runtime retirement hook, age, ledger/root policy or allocator protocol
changes. The new internal helper and CAPI exposure follow existing private
trace-fixture patterns; the non-mimalloc stub reports unsupported use.

`_PyGC_TestForgetLastRecord()` uses zeroed raw heap storage for an unregistered
scratch interpreter layout, plus a local trace state and one local diagnostic
record. The only connected fields are those the unchanged function reads.
The fixture calls that same function for an absent `Py_True` address, the
matching `Py_None` address, and a subsequent empty request. Neither object's
header is dereferenced by retirement. The absent query checks full original
record bytes, including its two origins and overflow count; the returned
count/hint pairs are captured after each actual call. The Python oracle
requires `((1, 1, 1), (0, 0), (0, 0))`.

The raw allocation has one failure return and one release, before result
allocation. There is no interpreter lifecycle call or publication into real
thread/interpreter state, no allocation within the retirement calls, and no
real root, ledger or age mutation. The one-past record pointer used by the
existing zero-length `memmove` remains within its valid array boundary.
This is explicitly metadata-unit coverage, not a synthetic claim of actual
B reachability or object-free behavior.

The free case still requires an actual initial target B record. It now proves
that record disappears and the post-free ledger equals the previous ledger
minus that address, while keeping the collection sequence unchanged and hint
equal to actual nonemptiness. It then runs the isolated last-record control.
The resize case keeps same-address and actual allocator-NULL provenance
checks, actual movement/payload and reclamation checks, and every unrelated
record across the move. It rejects the original address/origin pair rather
than an independently born allocation at a reused address. Its final major
checks the known original and known rooted-unrelated origins retire without
requiring the entire ambient ledger to be empty. The dedicated actual-reuse
case is unchanged.

Independent receipt:
[`build/nursery-adoption-review/retirement-correction-source-20261031-v1.json`](../../../../build/nursery-adoption-review/retirement-correction-source-20261031-v1.json),
SHA-256 `1f1341be15100d4902459bb9ee7af49ae3a8630028d1c055f4cd879d0fc36684`.
It binds all five source files, exact commit/tree, unchanged hook and the
author's host receipt
`3abe802a9d056845b3176c3077aa030faccb2ca48813b0f7a67dae3bf6a2a7fe`.
I independently parsed all twenty child templates and preserved twenty method
IDs. Only the two nominated methods changed; every other method is AST
identical. The parent 37/12/2 child schedule remains applicable. Both changed
C files report zero diagnostics in an independent precise-root check, and
`git diff --check` passes. The author's source-rewriting preparation script
was read but not executed.

No native command or build ran in this review. The new C helper requires a
fresh debug image; the earlier sealed image cannot test it. C compilation,
actual oracle execution, subsequent applicable feedback and qualification
remain pending separate helper review and coordinator activation. Preserve
the original failures and the diagnostic prefix's changed failure location.
