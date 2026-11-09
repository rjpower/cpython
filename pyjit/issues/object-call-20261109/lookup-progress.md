# Lookup proposal progress

2026-11-09. Scope: read-only source survey and bounded proposal, initial 30-minute
budget. Runtime inspected: frozen `4d666e1b482cb4983c7683f91c5922af14d0ab40` in
`execution-boundaries-20261108`; canonical documentation lives here.

Completed: read applicable guidance, CPython AI policy, current goal/process/
status/handoff, ranked next steps and source-backed micro addendum. Audited
classification, specialization, borrowed feedback/snapshots, emitted handlers,
shared-key insertion invalidation, dictionary replacement and first-probe API.

Findings: scalar class values explicitly decline the guarded instance path;
existing object handlers retire the receiver before acquiring the cached value;
shared-key addition invalidates the owning type before insertion; the current
first-probe dict helper only supports inline integers, not the measured string
key. Proposal reuses the existing non-descriptor family with an explicit
canonical borrow and acquire-before-retire transaction. It keeps current layout
admission and defers direct class loads, bucket hints and layout changes.

Output: [lookup proposal](lookup-proposal.md), ready for independent review.
No runtime edits, native runs, commits or reproducer were made. Root owns
checkpoint publication, builds, native verification, integration and timing.
Next: independent review settles payload typing, absence proof, mutation and
publication cases; implementation waits for the coordinator's release of the
checkpoint gate. Small-first ladder is generation/source inspection → release
and debug compile → focused semantic checks → post-review → integration → paired
performance. Scope cut remains scalar class-value admission plus the two
existing non-descriptor result transactions.
