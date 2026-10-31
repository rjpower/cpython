# First native feedback evidence

Root activated the [reviewed helpers](reviews/feedback-helpers-20261031-v1.md)
for exact source `7f0b1819f84ee11b354d60394a4975e1fcb0a188` at
2026-10-31 **21:20:45.868428 UTC**. The sole sixty-minute batch ends at
**22:20:45.868428 UTC**, with first-failure stop and no automatic retry.
The GC invocation had terminated and verified idle cleanup before this launch;
GC source/fixture diagnosis continues without a second GC native invocation.

The root-owned exec session is **71460**. Evidence is under the isolated
checkout's `build/constant-operands-feedback/`:

| Artifact | SHA-256 |
| --- | --- |
| `plan.json` | `32c27a4025cfc9f0c10bee19a3111ecf114dd9458945d32d8d3a23d0e97ff70e` |
| `launch-held.json` | `a70b94aec5d6ee0f1b857e09230d9bc30552815a00505ed45e0b52dfe8d38517` |
| `activation.json` | `f6024cfc993788931a9cb3538fe57568b02462e01300e50797490a03ee88803e` |
| `feedback-batch-start.json` | `ffd89d889dbd50cc13dc65412ab4c66ddbd4c6372a40397ecbe6d108d419503f` |

At 21:22 UTC, tool admission and configuration had completed and debug
compilation was running. No target-test, emitted-work, qualification or
performance result is recorded yet. The owner monitors this invocation;
changes or a retry require a separate bounded decision and review.

## First selection failed; source unchanged

The debug build and post-cleanup verification passed. The exact eleven-case
selection then ran in 0.140 seconds: ten passes, one failure, zero skips.
`test_shared_or_canonical_producer_stays` failed at its assertion that none
of the three source examples has an immediate RHS. Its loop did not identify
which function failed. The target returned 2; the enclosing row returned 1.
The batch stopped at **21:22:30.262695 UTC**. Session 71460 is terminal, and
post-gate cleanup verified an empty PID set under the exact build lock.
Full IR, the five compatibility modules, O3 and emission rows did not run.

Independent source inspection identifies a concrete hypothesis: an existing
flowgraph pass inlines a small exit block into an unconditional predecessor.
The conditional expression's arithmetic/return join can therefore become a
same-block, fresh, single-use constant consumer before the new register pass.
That would make the source-spelling negative oracle incorrect while preserving
the actual raw-CFG canonical-register rejection, whose test passed. This
interpretation still needs the exact emitted words; the failing assertion is
preserved and neither the compiler nor test has been changed.

Root accepted preparation of one held, compile-only diagnostic extracting the
three exact nested CodeTypes from the frozen test method. It must report their
original words without calling the nested functions or tests, changing source
spellings or altering conditions. Independent probe review and root activation
precede execution; its five-minute exposure remains within the original
22:20:45.868428 UTC deadline. No performance or behavior acceptance follows
from that observation alone.

| Artifact | SHA-256 |
| --- | --- |
| `debug/build-result.json` | `946f4917fbf722a6b656dbe72824d78d23e2c773799c69368cbe42aebed0ec12` |
| `debug/post-cleanup.json` | `17da99d273c356edfd861ff382436bf97beaa889c0960465e9f1fade26f393a7` |
| `feedback-batch-failure.json` | `78a4512e810a6f93f933624c1be4d9dc159b6dfe1ce2e4e15aaa2b8ac1c5c7aa` |
| `debug-0-new-cases.post-gate-cleanup.json` | `faaaa262017e40b40fa5b551fd598f32a717e112fb62d96464154d8e2c97279f` |
| `affected/debug-0-new-cases.result.json` | `1db0a2ad18c5ba81efba9494da2ee6bc2c4902e0daea2d838df3911a90480104` |
| `affected/debug-0-new-cases.log` | `1115089156461e410d31cc8c1b4bba9db87e4f3a8422c82b6b3420f55f3b47ab` |

## Observed words and reviewed test-only continuation

The exact-code observation ran once in session 36051 at 21:33 UTC, returned
zero and passed verification/idle cleanup. It recorded 27 collections and
zero fallbacks. It called neither the three nested functions nor their test.
Shared/dynamic inputs retain ordinary forms; only the inlined true branch of
`canonical` has ADD_SMALL_RHS literal 1. Its false branch retains literal 0's
load and ordinary ADD. The [test correction review](reviews/test-oracle-correction-20261031-v1.md)
binds all inner and external outer receipts through audit
`d7bf9af6357bc56c9f7b778c250715c72fc028f98204cabc9883832c75a53efd`.

Commit `a475acbf46d117137f7201adcb4557ce92fb9ec5` changes only
`Lib/test/test_ir.py`: the renamed control keeps shared/dynamic rejection,
asserts the witnessed branch forms, and retains both branch outputs. Raw-CFG
rejection cases and all runtime/compiler/generator sources are unchanged.
The original failed assertion is preserved; the correction has its own scope.

Root accepted the [remaining helpers](reviews/remaining-helpers-20261031-v1.md)
and activated them at **21:40:36.097965 UTC**, session **7327**. Debug execution
uses the explicitly bridged 7f0 native image and a475 test source; a later fresh
O3 build will name a475. The original deadline remains **22:20:45.868428 UTC**.
No debug product or old receipt is rewritten. New plan SHA-256 is
`630f7d4408d7709c93c885ae1685ed1391d72ff934be9e49940e1cb662036b39`;
manifest is `2a436739b8846b3825466a913741d397c6c1e2e09141d9c1606b9e378e96a2d4`.

At 21:40:46 UTC the corrected eleven-case selection passed with zero skips,
including terminal verification and idle cleanup. Full `test_ir` then started.
Broader behavior, emitted-work, qualification and performance decisions remain
open; this checkpoint does not accept the complete representation.
