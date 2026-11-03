# Integration orchestration follow-up review, 2026-10-31

Decision: clear the activation and runner designs at the identities below,
subject to actual final qualification acceptance and exact merge/prose review.
This supersedes the two runner blockers in
[the initial orchestration review](integration-orchestration-20261031.md),
which remains preserved. No native execution or activation is accepted by this
static disposition.

Reviewed the actual primary files:

| File under `build/nursery-shape-integration/` | SHA-256 |
| --- | --- |
| `activate.py` | `448321d19469cdaef2b2c3c2e5aafba6b82aa50899a6223e236010e496ee4e96` |
| `run_smokes.py` | `e27f866b86053ca459a4a7513962abcd2acb571c2034c1eef994e77ad48fd290` |

The reviewer read the full changed bodies and hashed only these small script
files. No loader, native program, build, suite, activation, runner or bulk hash
audit was executed. The author made the script corrections; the reviewer
changed only review prose.

The runner now rechecks current primary HEAD after each smoke and immediately
before completion. Final source/product loading must return the same activated
plan, and the activation record and ready launch hash must remain unchanged.

`collect_row()` binds each successful native result to the exact plan,
compiled/runtime-test source and integration identity. It verifies its raw
artifact hashes and the cleanup receipt's result digest. Launch records match
the literal ready manifest, with only a positive downward outer-timeout clamp.
Terminal gate and verification return codes and log hashes are checked; actual
gate/verification logs are included in the completion artifact map. The idle
receipt must prove no live target processes and no after-deadline observation.
Successful verification and final aggregation must finish before the smoke
deadline.

The completion map includes the batch start, activation, ready launches, plan,
merge proof and all plan input hashes, including the final independent
qualification ACCEPT record, its review prose and qualification capsule. The
final loader also verifies that capsule's separately enumerated carried and
refreshed artifacts. Final evidence review must preserve that transitive
qualification map, rather than treat the capsule container hash alone as
verification of its contents.

The previously reviewed activation guards, fifteen-minute smoke budget,
210-second kill/verification reserve, literal global-slot/build-lock gates,
native-status-before-cleanup recording and no-overwrite behavior remain intact.
No remaining static blocker was found at these exact identities.

Before execution, the coordinator and final reviewer still bind actual terminal
qualification accepted within its 20:14:30 UTC deadline, actual two-parent
merge/tree and inspected prose-only diff, current primary HEAD and refreshed
debug count/skip expectations. Then execute release and debug smoke once and
review their actual worker/root/skip/product/cleanup evidence. These scripts
continue to use products compiled from the frozen union; no rebuild from the
merge commit is claimed.
