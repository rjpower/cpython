# Cadence cost campaign: stopped at the retention guard

2026-11-01. The first actual campaign is incomplete. It supplies no complete
elapsed comparison, cadence acceptance, qualification or landing credit.

The coordinator accepted the exact
[native-binding review](reviews/cadence-cost-native-binding-20261101-v1.md),
reserved both machine slots and all three image locks, verified CPU policy and
idle targets, then materialized only the reviewed proposed launch bytes.
Activation began at 00:55:00.325366 UTC with deadline 01:55:00.325366 UTC;
it also reserved 110 minutes for downstream qualification/integration before
the unchanged 04:18:11 UTC integration boundary. All research agents held bulk
work during actual measurement.

The master is the isolated nursery-adoption worktree's
`build/nursery-cadence-cost-native-v1`. Exact identities:

| Input | SHA-256 |
| --- | --- |
| Immutable binding capsule v2 | `c8ac2a2b116bb42ca56fc198e248a82da0dd13b433f0e198e49e11a200eebe4f` |
| Materialized plan | `89c15bf3fda2c0635cdb8d78921493ac97526d66155be5d91f99fa3acf3b086f` |
| Materialized launch | `f90dd8a15e64bf5fbe458e02ca0394388380ac0d14bfcfe55428e77c17bea526` |
| Activation | `9241a53787c854cfdc774ca26ae3c49c2ba11ba2e6cdeb61534920267949e229` |

Root observed session 29523 terminal exit 1 at 00:57:23 UTC. The first failed
normalization is `score-r0-richards-0-historical`: the target itself returned
zero and produced its raw result, but the object-retention guard failed.
Fifty-seven rows had normalized successfully: twelve origin rows and forty-five
scored rows. The failed row adds one actual process and timed call; it receives
no normalized-pass credit. No second round or complete score exists.

| Failed historical Richards observation | Point 1 | Point 2 | Point 3 |
| --- | ---: | ---: | ---: |
| Post-major tracked objects | 12,112 | 12,168 | 12,224 |
| Post-major live bytes | 3,181,888 | 3,185,088 | 3,188,288 |

The final-minus-first increase is 112 objects, above the predeclared 32-object
bound, and 6,400 bytes, within 16 KiB. Preliminary independent source inspection
finds that the unchanged Richards kernel retains each new task through the
global `taskWorkArea.taskList` chain and never resets that chain in `run`.
This challenges the steady-retention oracle; it is not evidence of a cadence
collector leak or permission to widen the bound. A source-grounded diagnosis
and independent outcome review own the next decision. No partial timing is used
to select a policy or change an acceptance threshold.

Terminal cleanup recorded all three target lists empty, all three build locks
held and no deadline overrun. Root separately checked current host processes
and found no target/compiler jobs before releasing the quiet hold. The supervisor
trace, raw process output, failure envelopes, incomplete row list and all source
and product evidence remain preserved. No automatic retry or appended samples
are authorized by this failure.
