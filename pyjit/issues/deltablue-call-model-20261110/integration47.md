# Canonical direct class values: first integration

Runtime47 `986c65ee5ab6433ea80ab3d385abfe5ca8dd7675` is committed and pushed
to `pyjit/frame-call-model-20261109`. It unifies existing class-load handlers
around guarded canonical values, covering inline scalars and existing objects.
There is no new opcode, cache geometry or call ABI. The [proposal](lookup-implementation-proposal.md),
[peer review](B-proposal-review.md) and [post-review](B-post-review.md) describe
the guards, ownership and generated publication contract.

Release and debug each build 117 modules and pass 10 focused modules: 571 tests
run, 13 skipped. The value API check covers 306 units with zero failing findings
and zero parse errors. Full suites are reserved for the coherent batch checkpoint.
Magic remains 64417 and ATTR48/POLY120 geometry is unchanged.

Two balanced pairs per case provide a basic integration screen, not a parity
claim. Lower elapsed ratio is better:

| Case | /rebuilt frozen46 | /upstream |
| --- | ---: | ---: |
| DeltaBlue | 0.72080 | 1.26303 |
| Logging silent | 1.05526 | 0.91755 |
| nbody | 0.99574 | not run in this upstream screen |

A separate fresh-build confirmation (two pairs) gives DeltaBlue 0.75910,
logging 1.06341 and nbody 1.00430 against frozen46. That is a 24% DeltaBlue
gain in the fresh-build comparison, versus 28% in the incremental comparison
above. Keep the campaigns separate; the direct-upstream ratios above remain
1.26303 for DeltaBlue and 0.91755 for logging.

The direct upstream comparison leaves about 26% excess elapsed time on DeltaBlue;
logging remains faster than upstream. The logarithmic mean across unrelated
kernels is not this increment's decision criterion. Keep the useful DeltaBlue
gain with the measured logging regression under the user's incremental policy.
Repeat the target and broaden the suite at the batch boundary.

The rebuilt frozen46 is a separate matched-configuration build, not the old46
ELF used by earlier screens. Do not multiply ratios across independent campaigns
or infer an exact percentage of the upstream gap removed. Both use GCC 15.2, the
existing release configuration, fixed CPU 15, hash seed 0, JIT off and unchanged
benchmark inputs. Source/build checks pass before and after timing.

A bounded warm snapshot retains all328 scoped ATTR sites as private and selects
the class-value handler at29 sites. It establishes route coverage, not dynamic
hit counts or a time budget. Classmethod forwarding is the next separate landing;
conditional leaf-call elimination is under implementation in an isolated worktree.

Evidence under `BASE/pyjit-link-wave1/deltablue-call-model-20261110`:
`delta47-{regen,build,focus,lint}-checks.json`, `measure47-baseline46`,
`measure47-upstream`, `coverage47-deltablue.json`. BASE is defined in
[HANDOFF](../../HANDOFF.md). The initial fresh46 control build failed on the
development-library UUID include path; the retained sysroot retry passes117
modules. Its module flags were matched to the existing release build. No control
source or measured workload was altered.
