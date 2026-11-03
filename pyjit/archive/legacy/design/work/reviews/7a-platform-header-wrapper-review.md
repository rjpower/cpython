# Platform header auxiliary: source and retained-evidence review

2026-10-30. Independent source/evidence review only; no target execution,
helper edits, native rebuild or test rerun. Scope: Queue2a auxiliary-aware
successor and the same rejection in H1/baseline/broad/postmerge records.

**SOURCE_CLEAR** for Queue2a successor diff
`boil-queue2a-sweep/build/queue2a-validation/auxiliary-v2.diff`, SHA256
`ec1ac0e10e8007eb5b93530d11aeac0b674f083e19f41002d2d52e15a88c1a73`.
Native execution remains the coordinator's separate decision. The original
Queue2a six-case receipt/log and their subsequent INVALID disposition stay
unchanged. This review does not turn a new source plan into actual coverage.

## Cause and narrow successor

`Lib/test/libregrtest/utils.py:667–681` constructs
`[*python_cmd, '-m', 'platform']` inside `display_header`. It captures stdout,
prints a failed-host-platform line for a nonzero exit, and continues. The
old wrapper accepts only `test`/`test.libregrtest.worker`, so the auxiliary
fails before its state witness, while actual workers can still pass.
`boil/tools/verify_launch.py:368` already recognizes the exact two-element
`['-m', 'platform']` auxiliary form separately from workers. H2's accepted
own-binary platform handling addresses this same distinction.

Reviewed successor files under `boil-queue2a-sweep/build/queue2a-validation/`:

| File | SHA256 |
| --- | --- |
| `full_assert_state_v2.py` | `994ee3da55eceac58151bd3a678e9d3cce19cec27c791e519349109e765431d1` |
| `run_affected_v2.py` | `2cd2e88c3dc6e5ebe2cd5e17301349a3c281ead9bbf717b08c9d992136e72204` |

The new allowlist accepts platform only with no extra arguments. It retains
assertions for the actual executable, extension origin, build/debug config,
source/config metadata, Python assertions, collector mode and stress. This
is not a bypass of actual-mode checks. Only the `test` driver installs
scheduling/result interception; worker observer and full-V3 validation stay
unchanged. Shell forwarding changes only the wrapper filename.

Platform gets a separate auxiliary journal. Adapter requires exactly one
start/end pair, same PID, literal platform module/empty test arguments,
matching plan/source/config/provenance/mode/stress, and a main-interpreter
root-counter witness for that PID. A module exception/nonzero SystemExit
adds a failure row, so the required two-row journal rejects it even if
regrtest swallows its exit. Early validation failure leaves an absent or
incomplete journal and also rejects. Auxiliary records add no worker/test
coverage. The adapter/plan/output successors preserve original files.

## Actual affected evidence

| Retained record | Actual tests/native outcome | Header limitation |
| --- | --- | --- |
| Queue2a `affected/release-0-focused` | Six selected tests pass, zero skips, native0; original result reports success=true/qualification=false | Log begins wrapper `AssertionError: platform`, then failed-host-platform line. Subsequent explicit harness INVALID disposition is correct. No current corrected-run credit. |
| H1 `build/h1-validation/results/clang-tail-0-combined` and `clang-tail-debug-2-combined` | Three modules, 103 reported tests each, zero skips, native0; receipt qualification=false | Both logs contain the same rejected auxiliary and failed host-header line. Test-worker evidence remains real; successful auxiliary/header validation was not established. |
| 5a45 Clang baseline `build/clang-validation/results/clang-goto-0-combined` / `clang-tail-0-combined` | Three modules, 103 reported tests each, native0; goto has one skip, tail zero; receipt qualification=false | Both logs contain the same auxiliary rejection. These are not uniformly error-free wrapper transactions. |

These are wrapper coverage/accounting defects, not evidence that the
native interpreter failed those test cases. Conversely, native0 does not
prove a subprocess ignored by regrtest succeeded. Historical source/evidence
reviews missed the header failure; retain the logs/receipts and attach this
limitation rather than rewriting them or fabricating blanket passes.
The two baseline stale runs do not use this regrtest header mechanism;
this review does not withdraw or newly qualify their separate controls.

## 5a45 broad and postmerge qualification impact

The actual frozen `run_broad_v2.py:63` and
`run_post_merge_smoke.py:81` commands include:

```
<own python> <full_assert_state.py> -m test -q --randseed 0
  --no-randomize --timeout <declared> -j8 --python <full_worker.sh> ...
```

Neither adds `--header`. `Lib/test/libregrtest/main.py:550–556` calls
`display_header` only for explicit want_header or when its default display
condition holds; quiet mode makes that default condition false. Thus these
commands do not launch the platform auxiliary. This is a source fact
corroborated by the actual log beginnings (`Using random seed: 0`) and the
absence of both `AssertionError: platform` and a failed-host-platform line.

Checked retained result/log pairs in `boil-pre-i-union/build/`:

| Record | Result SHA256 prefix | Tests / skips; native outcome |
| --- | --- | --- |
| `pre-i-qualification/broad-v2/release-full` | `28cf749dce90` | 51181 /3469; native0, existing PASS |
| `.../debug-full` | `6d822e0bf5c0` | 51181 /3412; native0, existing PASS |
| `.../debug-kill-full` | `2a2f5b858cc6` | 51181 /3412; native0, existing PASS |
| `.../stress11-release` | `c8bdf0f1a094` | 3065 /100; native0, existing PASS |
| `.../stress11-debug` | `8d8c824cd915` | 3065 /64; native0, existing PASS |
| `pre-i-qualification/post-merge-smoke/post-merge-release-smoke` | `b5f08128a6a4` | 1930 /50; native0, existing PASS |
| `.../post-merge-debug-smoke` | `d815254531e0` | 1930 /20; native0, existing PASS |

Their reported driver/worker/root coverage remains legitimate relative to
this defect: the failed auxiliary never ran in these profiles. They do not
need another full run or a newly invented auxiliary witness. This review
rechecks the specific header impact, not every prior gate predicate; exact
skip/filter, report-only unpinned and incremental-landing limits remain.

The ORIGINAL `pre-i-qualification/broad/release-full` is still FAILURE,
native2, 51181 reported tests/two failures/3533 skips from its recorded
bytecode/cache launcher-environment defects. It also used quiet mode and
has no platform-header rejection. Do not reinterpret that actual failure
as the harmless informational-header issue or relabel it PASS.

For nonquiet H1/baseline rows, original worker test coverage can be retained
with an explicit informational-header limitation and coordinator ruling;
it cannot be described as a completely clean auxiliary-aware transaction.
For Queue2a, the chosen fresh focused successor must demonstrate its own
auxiliary-aware receipt before granting that scope. No historical full
rerun or native rebuild is indicated by this source-only wrapper change.
