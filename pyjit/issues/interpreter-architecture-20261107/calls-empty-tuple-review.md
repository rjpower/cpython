# Empty-varargs initializer: source review

2026-11-07. Reviewed the Stage 3 uncommitted diff after runtime commit
`25ce1654801`. **Source review: accept, subject to coordinator build,
generated-code and measurement checks.** I made no runtime edits or tests.

The sole runtime change is in the guarded `CO_VARARGS` branch of
`call_transaction_overlap_python`: construct the exact static-immortal
`tuple_empty` as a `PyValue` with existing `_PyValue_STATIC_IMMORTAL` and
place it in the same child slot. The prior branch converted that same
singleton with `PyValue_FromObjectBorrow` and duplicated it. No guard,
frame transfer, owner retirement, callback or error path changed. The test
adds empty-tuple identity checks across repeated calls and a
generator-owned caller.

This is a local ownership proof for that singleton, not permission to use
the initializer for mortal objects. Source scope matches the reviewed
proposal. Whether GCC removes meaningful common-path work and whether
elapsed workloads improve remain measurement questions.
