# Completion and held integration preparation review, 2026-10-31

Decision: the held smoke preparation is sound. The completion sealer needs the
blocking corrections below before execution or landing acceptance. No native
program, loader, build, suite, bulk hash verification or smoke was run for this
static review. The reviewer changed only review prose.

## Exact reviewed inputs

All paths below are in the primary checkout unless stated otherwise.

| Input | SHA-256 |
| --- | --- |
| `build/restart-recovery/seal_completion.py` | `9af247128da465c4d9683f14151a62804be41308797e6c9c5e5f3d97fada248e` |
| `build/nursery-shape-integration/prepare.py` | `3677e8301fb9cb122012f2dd6b671254f9f1fa3557d12e1bb8e96239cf2eda3f` |
| `build/nursery-shape-integration/run_post_merge_smoke.py` | `8e1b341d0aa03374d5c2327f4f7fb427a3c5a1229f01526062bf2f03848d4f53` |
| `build/nursery-shape-integration/helper.diff` | `a2ecd90fd32d8fdb616972f191d99a1cb71b09adeae590f1bf318d31f7f72822` |
| `build/nursery-shape-integration/REVIEW.md` | `65eb927c31ac7b3363523c83e1bf53d7e09529d121f3451d7b0f778ff64fe908` |
| Held `plan.json` | `e961b3466885ccd62b49c1b671f82dc301b1a1a6a11cd8b6e4b3085dabbafaab` |
| Held `launch-held.json` | `c77d1c1c9256777b546edc6c6ace2c6d1ed8a62b22fc6ba9f24915bce4995fd4` |

Inspected the original/resume issue plans and validation contract; the existing
broad, ASan and closure helper bodies; the current-input/product authority;
the restart launcher/start/preflight; carried completion/component maps; and
the independent restart/source-union reviews. Frozen qualification identity is
`e9e10d5b503eaf219797a5db44766eeab6fad4dc`. Reading the maps establishes their
schema and reference coverage, not current validity of every referenced file.

## Blocking sealer findings

1. **Carried evidence is not transitively verified or retained.** Lines 41–43
   hash the early/common completion files and common component index but do
   not verify their `artifacts` maps. The existing loader freezes the manifest
   containers and current products; it does not check every early/backend
   raw/result/cleanup artifact. The smoke helper later checks only the flat
   final qualification artifact map. Verify and flatten the explicitly carried
   maps, including their nested completion/component maps, into the seal.

   The claimed component negative is also absent from that path. Its known
   follow-up capsule is
   `../boil-gc-nursery-minimum/build/gc-nursery-minimum-validation-v2/followup-completion.json`,
   expected SHA-256
   `59c531941370d8f74b64578d22b8e7d991a4403f513435e713d9b13d6b49f009`.
   The restart reviewer checked its 29 references; the final sealer must
   retain and verify them if it claims to carry that negative. Preserve its
   component source identity and source-equivalence proof; do not relabel it
   as union execution. Likewise bind any other component capsule claimed in
   the final carried scope, rather than depending on a prose locator alone.

2. **Restart records are retained without checking their links or outcomes.**
   Require completion's `start_sha256`, source and deadline to match the
   retained start, and require `closure_debug_idle`. Check start's preflight,
   frozen-launch and launcher hashes against the actual retained files. Retain
   the sealer and restart launcher bodies. For each remaining launch, verify
   its recorded gate-log hash, literal command/deadline linkage and successful
   post-gate idle observation. The current code merely hashes
   `post-gate-cleanup.json`, so an unsuccessful observation is not rejected by
   the sealer. Check `post_gate_idle_verified`, empty live PIDs and the stated
   qualification deadline. Preserve after-deadline safety cleanup without
   granting it acceptance credit.

   Retain each cleanup observation's actual verify log and check
   `verify_log_sha256` and verification argv. That output is currently omitted
   from the final artifact set. Apply the same linkage to the carried release
   cleanup, preserving its separately timestamped post-restart meaning.

3. **Closure results need explicit plan/command bindings and cleanup checks.**
   Each closure result is required to pass but its `plan_sha256` and `command`
   are not compared with the retained closure plan. Verify those links and
   verify the closure launch/gate-log/post-gate-idle records rather than only
   hashing their containers. The existing closure loader proves current
   source/products/headers; the completion receipt must also prove that the
   accepted result belongs to that exact command and plan.

These are evidence-sealing defects, not evidence of a runtime failure. They
do not authorize modifying frozen helpers or repeating native qualification.

## Held preparation findings

`prepare.py` cannot launch a target interpreter. Both output records remain
held, and an activated plan cannot be overwritten. Debug count evidence is
pending instead of filled from a historical expectation. Release counts derive
from accepted current-union full OFF/N0 state: each of the nine returned module
records must match the outer suite PID and stats, report PASSED/zero failures,
and reconcile its exact skip list. Their test totals and skip identities become
expectations for fresh smoke, without claiming smoke execution. Full validator
and terminal cleanup acceptance still belong to the final qualification review.

The copied helper differs from the prior accepted integration helper in exactly
the serving authority and native-head constants; an independent `diff -u`
matches `helper.diff`. The worker, root/origin/mode/product/skip validation and
cleanup bodies are otherwise unchanged. Current source aliases are observed,
not constructed. Cohort/configuration checks keep native products and runtime
test/library source bound to the frozen union. Running those images does not
claim they were compiled from the future merge commit.

## Required activation and landing review

The held preparation is approved subject to the sealer corrections and a
separate final review. Before readiness changes:

- Bind reviewed terminal debug count/skip evidence, the corrected completed
  qualification seal and a separate independent final landing-acceptance record.
  `QUALIFIED_PENDING_INDEPENDENT_LANDING_REVIEW` is an appropriate seal state;
  hashing that state alone does not supply independent acceptance.
- Bind the actual primary integration commit/tree and inspect its complete
  patch against the frozen union. The exact changed-path list must contain
  only reviewed project prose; check regular-file modes/content as well as
  names. Git's tree diff covers tracked byte/mode changes outside the list.
- Verify primary HEAD is the bound integration commit at activation and after
  smoke. The unchanged helper verifies the referenced commit/tree/path list,
  but does not itself assert current primary HEAD. Preserve the user's existing
  unrelated edit; a claim that every primary working-tree byte is clean would
  need a separate, accurate explanation of that exception.
- Freeze the final plan/helper/literal launches and bind this preparation
  review plus the final qualification review. Retain the held snapshots before
  refreshing counts or activating metadata.
- Run release smoke and current idle/cleanup verification, then debug smoke
  and current idle/cleanup verification, under the existing global slots,
  build locks, timeouts and serialized suite policy. Retain actual launch,
  root/worker/skip, return-code and cleanup evidence. No smoke acceptance exists
  at this preparation stage.

Optional refinement: explicitly reject duplicate selected returned/suite module
records while deriving counts. The accepted full validator already constrains
their execution; the extra check would make the small derivation self-contained.
