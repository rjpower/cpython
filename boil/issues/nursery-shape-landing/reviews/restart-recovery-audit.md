# Restart recovery audit

Date: 2026-10-31. Independent recovery audit; no runtime changes or native
qualification commands were run by this reviewer.

## Findings

The primary checkout's `.git` file survived but pointed to an absent linked
worktree metadata directory. Its branch ref and Git objects survived at
`a01a9b953e2b57d0a5ab17929f7b22b54b7448bf`. The source loss included all of
`boil/`, `Include/` and `Python/`, plus much of `Lib/` and `Modules/`. This was
filesystem loss, beyond the loss of running processes. Directory timestamps
place a change to the common worktree registry at 12:06:53 PDT, but do not
identify the actor or establish whether the restart caused the loss.

The coordinator's authoritative pre-recovery inventory found 4,921 missing
tracked entries and 2,001 surviving entries. The latter included 135 raw
content/mode differences from Git blobs. An earlier shell inventory counted
four additional apparent missing paths because it followed broken symlinks
under `Misc/mypy/`; the recovery correctly used `lexists` and retained them.

## Method and verification

Reviewed `build/restart-recovery/recover.py`. It checks the exact branch tip
and `.git` backlink, refuses to replace existing metadata, reconstructs the
linked HEAD and index without moving the branch, and restores only absent
tracked entries. Exclusive file creation prevents replacing files that appear
after inventory. It handles executable modes and symlinks, checks parent
containment and verifies all surviving tracked bytes/modes after recovery.

`build/restart-recovery/before.json` and `after.json` record the inventory and
restoration: all 4,921 absent entries restored; all 2,001 surviving entries
retained. Git connectivity verification of the surviving tip passed. The
reconstructed index represents the known committed tip; it cannot recover
unknown pre-loss staging state or missing uncommitted content.

The surviving migration manifest,
`build/documentation-migration-20261031/protected-user-files.json`, supplied
SHA-256 fingerprints for five pre-existing legacy user notes. A read-only scan
of all 3,342 loose Git objects found an exact copy of the modified source
review in blob `bdf87cffe841f3f342f88967b9c460ca099a968b`. After verifying that
its destination held the restored committed baseline, this reviewer restored
the user copy. Its SHA-256 is
`47fbf2e11262345e4af0cd771626d65b7e40a8dec45f4eedaab6b601dcd55991`,
matching the protected manifest. Git again shows the pre-existing 48-line
addition. Receipts:
`build/restart-recovery/protected-loose-object-scan.json` and
`protected-source-review-restored.sha256`.

## Remaining losses and limits

No exact copies were found for these four untracked legacy notes:

- `boil/archive/legacy/design/work/reviews/10a-native-cleanup-protocol-review.md`
- `boil/archive/legacy/design/work/reviews/10a-native-cleanup-protocol.md`
- `boil/archive/legacy/design/work/reviews/7a-completion-graph-independent-review.md`
- `boil/archive/legacy/design/work/reviews/7a-retirement-completion-policy.md`

Filename searches across retained worktrees and primary build artifacts found
no copies. The loose-object fingerprint scan found none. An escalated,
read-only ZFS snapshot listing reported no datasets available. The standard
code-server, VS Code Server and desktop VS Code history directories were
absent. Packed upstream history was not scanned. These searches establish
that the inspected recovery sources lack the notes; they do not prove that
no external backup exists.

The separate pre-restart untracked
`boil/issues/nursery-shape-landing/results.md` can be reconstructed from a
captured read by the coordinator. Such reconstruction must be described as
reconstruction, rather than original-file recovery.

The source recovery method is accepted for its stated scope. The four missing
user notes remain explicitly unresolved. Frozen qualification inputs and
retained build evidence require their separate qualification audit; this
recovery audit does not establish a runtime qualification pass.
