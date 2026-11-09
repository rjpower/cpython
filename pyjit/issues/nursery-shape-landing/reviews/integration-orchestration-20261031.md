# Integration activation and smoke orchestration review, 2026-10-31

Decision: activation design is clear. The runner needs the two corrections
below before execution. Actual qualification acceptance, merge/prose review,
activation and smoke remain unexecuted gates in this static review.

## Reviewed identities

Primary `build/nursery-shape-integration/activate.py`:

- Initial: `f15f93c9e09dd6fe4a2484e1f5d937c4999b464e455ca1823786f2cb9ef222cb`.
- Reviewed successor, which resolves the prose review path before checking its
  location: `448321d19469cdaef2b2c3c2e5aafba6b82aa50899a6223e236010e496ee4e96`.

Primary `build/nursery-shape-integration/run_smokes.py`:

- Initial: `9e3d4c4dca6cf8c3a1e52baf4bd7f12e46451480132de976d1c5d5b13603c82b`.
- Reviewed successor, adding boot/executable cleanup context:
  `ce8478fcee94c13465ac552258081df9db2dc31430161851f68df3ebdbb23f62`.

Read these scripts, their held preparation/helper contract and existing gate
body. No native program, loader, build, suite, script activation or bulk hash
audit was run. The reviewer edited only this review; the author owns script
corrections. Existing preparation and sealer reviews remain in force.

## Activation findings

The once-only activation lock and exclusive output creation preserve held
metadata before replacing the plan. Count expectations are recomputed from
terminal accepted current-union full results; actual product/source authority
is loaded and qualified artifacts are checked. The final independent ACCEPT
record binds the exact qualification hash, source, review prose hash and a
timezone-aware review time within the declared remaining-batch window. A
future-dated review is rejected. The review path's resolved location is now
checked inside the designated issue review directory.

The source bridge starts from actual primary HEAD, requires a two-parent merge
with the qualification union as an ancestor, and compares every tree entry's
mode/type/blob. Every changed entry must be regular nonexecutable UTF-8 prose
under `boil/` with an `.md` suffix; unchanged tracked entries must match
exactly. It records the actual commit/tree, parents and entry differences.
The final reviewer must still inspect the actual prose patch: UTF-8 and a file
extension alone cannot prove prose content.

The activated plan binds the actual bridge, final qualification/review,
activation and runner bodies, and reuses the sealed union images and runtime
library/test source. This does not claim a rebuild from the merge commit.
No activation blocker was found at the reviewed successor identity.

## Runner corrections required

1. **Recheck primary HEAD after each smoke and before completion.** The runner
   currently checks it only before the first row. The accepted source bridge
   requires the bound merge identity to remain actual primary HEAD throughout
   the smoke handoff. A changed primary HEAD must stop acceptance rather than
   leave a successful completion associated with a stale integration binding.

2. **Close the terminal evidence bindings.** The final completion map includes
   native artifacts and receipt containers but omits gate and verification
   logs, despite their hashes appearing in receipts. Include those actual log
   files so a flat completion consumer can revalidate them. Include the batch
   start, activation, ready plan/launch manifest and actual merge proof, rather
   than leave the execution-to-activation chain implicit. During aggregation,
   check success result/cleanup digest links, plan/source/integration identities,
   literal launch/verification commands and hashes, and successful current
   idle observations. Keep every failed receipt and reject after-deadline
   cleanup as qualification credit.

These requests require no frozen helper change or repeated qualification.

## Runner behavior otherwise clear

The two rows are release then debug, with exact existing literal gates and
build locks. The helper takes its suite serialization lock; each gate acquires
the existing global slot before its build lock and sweeps owned workers before
return. The runner records native terminal status before attempting cleanup,
so cleanup failure does not erase an already returned command's status.

The fifteen-minute batch clamps outer timeouts to remaining time while
reserving 210 seconds for kill grace and bounded verification. Verification
holds the build lock, requires the target executable idle before and after,
and checks successful completion before the deadline. Safety cleanup still
records its outcome on failure and carries no qualification credit. Exclusive
outputs and the existing helper's no-overwrite rule prevent an automatic retry.

Final acceptance must inspect actual run/worker/root/skip/product and cleanup
evidence. Neither this static review nor an activated manifest earns smoke
credit. Any successor script must receive a distinct reviewed identity before
execution.
