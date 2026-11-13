# Round54 qualification snapshot

Draft of completed checks; no selection or admission decision. Golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` remains selected. Focused
qualification does not imply a green full suite, free-threaded qualification or
a performance win. [Benchmark tables](benchmark-tables.md) retain the distinct
campaign sizes and candidate identities.

E=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/frame-state54`.
Build/source roots follow [plan](plan.md). The checked JSON manifests record
clean source, matching native `sys._git`, expected magic 64418 and return code 0.
All six component build pairs and the compact build pair checked 117 modules
with 0 failed imports. The counts below reproduce filtered-test logs; separate groups are
not claimed as deduplicated totals.

## Completed component focus

| Candidate | Reviewed/current head | Actual GIL header | Release and debug focus |
| --- | --- | ---: | --- |
| R: recipient | `612fb4e9d170532947c007f0ccaaa04aff623329` |112 bytes |567 tests each role |
| F: feedback | `7ea40d5d67ccb5a9250b8313646efb537c7ff934` |104 bytes |578 tests each role |
| N: namespaces | `c2cc6777e4d72e6688854c0423459971fc81b355` |96 bytes |772 tests, 6 skips; separate 27 environment tests, each role |
| L: mapping | `b48fe2d39896fef83d795bba3ec9c95af68cc235` |104 bytes |84 frame/C-API tests, 8 skips; 640 core; 2 generated-effect tests, each role |
| S: initial overwrite | `4fbd16a14621f2b5327a0bfde6c2198190e71428` |112 bytes |245 runtime tests; 2 generated-proof tests, each role |
| S2: validated overwrite | `e95328ba1d6f3ea74dd92d0870213367287d64ef` |112 bytes |453 runtime tests, 9 skips; 2 generated-proof tests, each role |

Independent source/native details, ELF identities and mechanism limits live in
[R](option1-post-review.md), [F](option2-post-review.md),
[N](option3-post-review.md), [L](option4-post-review.md),
[S](option5-post-review.md) and [S2](option5-validated-post-review.md).
Header sizes are fixed bytes before owning homes,
not sizeof including the declared trailing home or total heap allocation.

| Check manifest under E | Matching focus logs |
| --- | --- |
| `recipient-corrected-focus-checks.json` | `recipient-corrected-{release,debug}-focus.log` |
| `feedback-focus-focus-checks.json` | `feedback-focus-{release,debug}-focus.log` |
| `globals-{core,env}-focus-checks.json` | `globals-{core,env}-{release,debug}-focus.log` |
| `locals-corrected-{frame,core}-focus-checks.json`; `locals-effect-proof-focus-checks.json` | `locals-corrected-{frame,core}-{release,debug}-focus.log`; `locals-effect-proof-{release,debug}-focus.log` |
| `clears-overwrite-{runtime,proof}-focus-checks.json` | `clears-overwrite-{runtime,proof}-{release,debug}-focus.log` |
| `clears-validated-{focus,proof}-focus-checks.json` | `clears-validated-{focus,proof}-{release,debug}-focus.log` |

R's one-test remote check passed both roles at the earlier corrected ancestor
`daab07a69851edeb9632caadc5b2651c48a22c2c`, recorded in
`recipient-remote-focus-checks.json`. It is not added to the current head's 567
tests. Matching build evidence is in
`feedback-initial-{release,debug}-build.log`,
`recipient-assert-fix-{release,debug}-build.log`,
`globals-trace-fix-{release,debug}-build.log`,
`locals-effects-fix-{release,debug}-build.log`,
`clears-overwrite-fix-{release,debug}-build.log` and
`clears-validated-initial-{release,debug}-build.log`.

## Compact composition

F+N+L source head `82afe7aef2d86c0c01f60c681b082712171a92b4` has
[cold source GO](compact-peer.md). Root's non-running GDB check confirms the
actual 80-byte GIL header; this replaces the source review's predicted geometry
only, without a performance inference.

Both roles pass all 10 focused modules: 946 filtered tests per role, 16 skipped
in release and 15 in debug. Evidence:
`E/compact-focus-focus-checks.json`,
`E/compact-focus-{release,debug}-focus.log` and
`E/compact-initial-{release,debug}-build.log`.
Those 10 modules are frame, frame C-API, call, opcache, generators, coroutines,
monitoring, GC, IR contract and warnings. This is merged focused evidence;
full diagnostics and complete native post-review remain separate coordinator
gates. The coordinator rejects compact after the completed two-pair hot screen
in `E/compact-screen-hot-golden/summary.json`; an 80-byte header does not by
itself justify the measured cost. This does not select another candidate.

## Preserved failures and next candidate

Earlier failed diagnostics remain evidence: R's helper-purity/generated-owner
finding, N's eager LLTrace sentinel decode, L's premature record visibility and
generator effect/first-input findings, and S's ordinary temporary assertion on
the intentionally uncleared inline home. Their corrections and matching
current identities are described in the component post-reviews. The successful
native S census uses the four `clears-hits-snapshot-*.log` files; the earlier
failed diagnostic attempt is not a runtime failure or an admission result.

Constructor-validated S2 at `e95328ba1d6` now has matching
[source](option5-validated-post-source.md) and
[generated/native](option5-validated-post-review.md) GO, completed builds and
the focused results above. Its own hot screen, six-pair hot confirmation,
two-pair all29 screen and matching native census appear in the benchmark tables.
Original S evidence remains separate; no overall admission decision is made.

R2-NONE at `fcc82220ddd` has [proposal](recipient-default-proposal.md),
[peer review](recipient-default-peer.md) and
[handwritten-source GO](recipient-default-post-source.md). Its native build,
executed qualification and performance remain pending in this snapshot. No
result from R at `612fb4e9d17` is relabeled as R2.

These focused runs retain the narrow
`test.test_gdb.test_pretty_print.PrettyPrintTests.*` quarantine where the
orchestrator specifies it; they do not broaden it. No round54 full-suite pass
is claimed. Selected51's documented debugger-jump/LLTrace diagnostic limits
remain unchanged until coherent full diagnostics establish otherwise.
