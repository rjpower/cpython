# Actual merge and smoke activation review, 2026-10-31

**Clear the two post-merge smoke launches.** This reviews the concrete activated
metadata and actual primary merge, not just preparation. It supplies no smoke
execution or landing-closure credit.

Primary HEAD was independently observed as
`d98291018c9665ae3db6952da0b0a40a4e04992b`, with parents
`6f4584939a07145fad9218ebfb33098f20e4b594` and qualification union
`e9e10d5b503eaf219797a5db44766eeab6fad4dc`. The bound merge tree is
`31b8431cd0f076190b427ef563116ec599b85eb2`.

## Actual source bridge and prose

An independent host check compared every tree entry's mode/type/blob between
the frozen union and actual merge. All **6,903 entries outside the enumerated
prose changes match exactly**. The ready plan, activation, launch manifest and
merge proof agree on the exact path list and changed entries. Each changed
entry is a regular `100644` UTF-8 Markdown blob under `boil/`.

The exact no-renames bridge enumerates **34 paths**. Git's ordinary diffstat
reports 33 files because it recognizes the exact archive rename of the old
benchmark comparison; these are consistent views, not a missing path. The old
benchmark guide and comparison retain their exact original blobs in the
archive. Reviewed current prose covers benchmark-guide consolidation,
call-setup attribution, bounded coroutine design/review, recovery and
qualification records, source/runner reviews, issue status and runtime contract
updates. It distinguishes component feedback, qualification and pending smoke,
retains failures and recovery provenance, and makes no new throughput claim.

The runtime prose accurately describes the source being integrated: opt-in
cell/tuple admission, stable-address promotion, conservative old retention,
automatic majors and verifier/stress limitations. The user's subsequent
direction to adopt the nursery as the architecture is a separate next design;
it does not alter this sealed integration or imply that implementation has
already happened. No runtime, test, tool or generated-code difference is
hidden in the prose bridge.

## Actual qualification and activation bindings

The ready plan is SHA-256
`6061f3d26a2b18003a8314c2b03fad803a9429db686c9d37a5f9b6eb07a8c08d`;
ready launches are
`c147e270cfc8b0fa0a5e16a1cac9e055af15777b496c54367696d58db1a3903d`;
activation is
`c2f1b322b56dfc3d9764af838732e6ddc90d50d9096a82627dd7437fcec87a76`.

All nineteen ready-plan inputs were independently hashed and matched. The
source/native identity remains the frozen union. The qualification capsule
`ca931e655365f1995a37ef3b429d333bb8d4dd73d6ed82935b69c1b7832994da`
matches the independent ACCEPT record
`787b403dfb5e164b0989dd9b730e175fc3a342cb8193528b36ed52da497a99a7`.
The actual final review prose and its audit hashes match that record.
Acceptance at 19:55:44.021317 UTC falls inside the retained remaining-batch
window ending 20:14:30.135818 UTC. The review correctly grants integration
acceptance while retaining the pending merge/smoke gates.

The held-before-activation plan and launch snapshots exist, remain held and
retain their plan/helper binding. Current held launches equal the saved held
copy. The ready literal run/verify commands equal those held launches; the
activation changes bindings/readiness rather than the selected commands. Other
held plan fields and every original input binding are preserved.

The accepted script identities remain unchanged:

| Script | SHA-256 |
| --- | --- |
| `activate.py` | `448321d19469cdaef2b2c3c2e5aafba6b82aa50899a6223e236010e496ee4e96` |
| `run_smokes.py` | `e27f866b86053ca459a4a7513962abcd2acb571c2034c1eef994e77ad48fd290` |
| `run_post_merge_smoke.py` | `8e1b341d0aa03374d5c2327f4f7fb427a3c5a1229f01526062bf2f03848d4f53` |

## Actual count expectations and evidence limits

A second host-only check replayed the frozen preparation count derivation from
the current terminal full OFF/N0 returned/suite records. Exact origin records,
counts and skip identities match both ready jobs:

| Fresh smoke to execute | Tests expected | Exact skips expected |
| --- | ---: | ---: |
| Release, nine modules, OFF/N0 | 1,937 | 80 |
| Debug, nine modules, OFF/N0 | 1,937 | 50 |

These are expectations, not executed smoke results. Commands use the existing
two-slot/build-lock gates, sealed union executables and frozen runtime
test/library source. The fifteen-minute runner and current cleanup protocol
remain as independently reviewed. Actual merge identity is recorded separately
from the compilation source.

Primary host artifacts retain this review's exact checks:

- `build/nursery-shape-integration/activation-independent-check.py` and
  `activation-independent-audit.json` (audit SHA-256
  `58187a3b8043c5b5e55c27373e01984166883e513b26e826c885f840925a31e0`),
  completed at 20:00:42.576244 UTC.
- `activation-count-independent-check.py` and
  `activation-count-independent-audit.json` (audit SHA-256
  `609c9e5941898271b4d25193e685b8ec90ee4cd03610f671f95556d63cb170e6`).

This reviewer ran only host metadata/count/tree checks, with uv and the system
Python. No target interpreter, native suite, build, loader, product hash replay,
334-artifact qualification replay or commit was run. The accepted final
qualification review supplies that carried scope; activation also performed
its original product/source guards. The actual merge diff passes whitespace
checking. Unrelated recovered user files remain modified/untracked outside
the merge and were not touched.

No blocker remains for the concrete release then debug smoke pair. Retain and
independently review actual worker/root/skip/product, terminal and current
cleanup evidence before closing the landing. Preserve primary HEAD and frozen
source/helper/plan identities throughout that execution.
