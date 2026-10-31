# Shared qualification: evidence and restart recovery

Qualification source is `e9e10d5b503eaf219797a5db44766eeab6fad4dc` in
`../boil-nursery-shape-qualification`. The original batch started
2026-10-31 16:06:20 UTC with a 17:36:20 UTC deadline. That attempt is incomplete;
the deadline is not extended retroactively.

## Completed before restart

This section reconstructs the previously untracked result summary from its
captured read and retained evidence. It does not claim recovery of that file's
original bytes.

All four fresh builds and cleanup passed: GCC O3, GCC O1/Py_DEBUG,
ASan Og/Py_DEBUG and Clang21 O1/Py_DEBUG tail. Each module check reported 116
modules and zero failed imports; optional missing modules retain their limits.
Linked audits, tail membership/CFI checks and early traversal passed.

O1 nursery five OFF/N0 passed unskipped with 108 collections; O1 shape/IR
thirty-four KILL/N0 passed unskipped with 127 collections. Previously reviewed
`build/nursery-shape-early/early-completion.json` is `94ab2b24` (39 references).

The three registered stale-root modes passed six cases by 100 iterations.
ASan OFF29 passed with 148 collections; tail OFF29 with 153 collections;
tail ownership KILL1 with 60 collections. All selections had zero skips,
fallbacks and report-only detached-snapshot visits. Previously reviewed
`build/nursery-shape-common/common-completion.json` is `692fac53` (60 references).
OFF29 includes actual experimental minor execution; KILL ownership does not
establish nursery verifier support. Component evidence and its negative remain
separately scoped under the [original plan](plan.md).

Release full also completed: 564 selected modules, 51,261 tests, 3,795 skips,
58,701 collections, zero fallbacks and one report-only detached-snapshot visit.
Module states were 462 PASSED, 94 SKIPPED and eight RESOURCE_DENIED. The terminal
result is `build/nursery-shape-broad/broad-v2/release-full.result.json`
(`51,261` is the actual count, not a count inferred from the component suite).
The launcher returned zero, but the post-cleanup receipt was not written before
interruption. Current input and cleanup verification are required to carry it.

## Restart recovery

At 19:07 UTC the primary worktree's `.git` backlink pointed to an absent metadata
directory, and several source directories were absent. The branch ref survived
at `a01a9b953e2b57d0a5ab17929f7b22b54b7448bf`; qualification source and products
survived separately. Host process inspection found no qualification workers.

`build/restart-recovery/recover.py` recreated only missing worktree metadata,
rebuilt its index from the preserved branch and restored 4,921 missing tracked
files. It preserved all 2,001 surviving tracked files byte-for-byte and mode-for-
mode. Its `before.json` and `after.json` retain the inventories. Git reports no
tracked differences after normalization; 135 raw blob differences in surviving
files were preserved rather than rewritten.

The independent [recovery audit](reviews/restart-recovery-audit.md) recovered the
exact modified user source review from loose blob `bdf87cffe841f3f342f88967b9c460ca099a968b`.
Its SHA256 matches the preserved migration manifest. The first filename,
loose-object, snapshot and editor-history search found no copies of four
untracked legacy notes. A later bounded, task-specific session-patch search
reconstructed all four with exact original hashes, then restored only absent
destinations. All five protected user files now match the preserved migration
manifest and retain their original modified/untracked state. The
[follow-up](reviews/restart-user-note-recovery.md) binds patch-replay receipts;
the original audit remains unchanged. Recovered user work stays outside project
commits.

The [independent carry review](reviews/restart-qualification-20261031.md) replayed
the release validator and checked retained source/product/tool/evidence hashes.
`build/restart-recovery/qualification-preflight.json` additionally validates the
borrowed headers and records an idle candidate process state on boot
`7bbe1ecf-490c-40f3-8db6-d46a8f6b6ba3`. Timestamped cleanup/launch receipts supply
the observation times; the original preflight has no UTC field.

Post-restart input verification and remaining execution use the
[restart plan](resume-plan.md). The release cleanup observation at 19:14:30 UTC
records no live release executable under its build lock; it does not backdate
cleanup. The unchanged release verifier then wrote the missing receipt.
`build/restart-recovery/release-full.cleanup-observation.json` retains the boot,
time and exact verification command.

The remaining-only batch started 19:14:30 UTC and has a 20:14:30 UTC deadline.
`build/restart-recovery/remaining-batch-start.json` binds the reviewed launcher
(`9eb24b16`), frozen launches, preflight, source and exact remaining selections.
Debug full OFF/N0 returned at 19:24:28 UTC with a terminal pass: 51,261 tests,
3,744 skips and 60,750 collections across 564 selected modules. It returned
565 actual driver/worker processes, zero fallbacks and four report-only
detached-snapshot visits. Module states match release: 462 PASSED, 94 SKIPPED
and eight RESOURCE_DENIED. Current post-gate idle observation passed; the
original success verifier completed cleanup at 19:24:50 UTC. Debug KILL full
then launched at 19:24:50 UTC and returned at 19:36:00 UTC: 51,261 tests,
3,749 skips, 60,708 collections, zero fallbacks and one report-only visit.
Its five additional skips are exactly the unsupported nursery controls with
the approved reason; the validator accepted their identities. Module states
again match release/OFF. Post-gate idle observation passed; its cleanup
verifier completed at 19:36:20 UTC. Release stress1000 then launched and passed
at 19:38:30 UTC: 3,121 tests, 153 skips, eleven passing modules, 35,917
collections, zero fallbacks and 41 report-only snapshot visits. Its cleanup
passed at 19:38:51 UTC; debug stress1000 launched immediately. These stress
profiles validate the major path with the exact unsupported nursery skips.
Debug stress1000 passed at 19:41:43 UTC: 3,121 tests, 118 skips, eleven passing
modules, 38,381 collections, zero fallbacks and 38 report-only snapshot visits.
Current cleanup passed. ASan16 KILL then passed at 19:47:02 UTC: 4,043 tests,
146 skips, sixteen passing modules, 13,128 collections, zero fallbacks and
four report-only snapshot visits. Current cleanup passed. This keeps the
approved ASan exclusions, fake-stack/leak-check limits and lack of
multiprocessing-leaf credit. Closure launched at 19:47:35 UTC and passed:
`test_cext` returned 16 tests/seven exact skips, 557 collections and zero
fallbacks/report-only snapshot visits. Regeneration left tracked outputs
unchanged; traversal passed; nineteen syntax receipts passed with no warnings.
The existing unsupported `_tkinter`, `_dbm`, `_gdbm` and `readline` header limits
remain explicit. Closure returned at 19:52:09 UTC; final input/idle checks
completed at 19:52:27 UTC, before the 20:14:30 UTC deadline. The reviewed sealer
then verified and flattened 334 artifacts into
`build/restart-recovery/qualification-completion.json` (SHA256
`ca931e655365f1995a37ef3b429d333bb8d4dd73d6ed82935b69c1b7832994da`).
Independent final audit rehashed all 334 references, replayed all six broad
validators and checked current source/products/headers and closure. The
[final review](reviews/final-qualification-20261031.md) accepted the exact union
at 19:55:44 UTC, before the deadline. The separate
`build/restart-recovery/final-landing-review.json` binds the review and capsule;
its decision is acceptance for integration, with actual merge and post-merge
smokes still required.

The original broad preparation also retains
`build/nursery-shape-broad/host-preflight-original-failure.json`: a host assertion
selected `uv run` rather than the helper's `run` argument. No native job launched
in that failed preparation. The original activation preflight corrected that
anchor and confirmed unchanged literal selections; the failure record remains.
