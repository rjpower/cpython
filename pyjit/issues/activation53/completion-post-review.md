# Completion component post-review

Point-in-time review. The [final result](completion-result.md) records executed
qualification, performance rejection and the retained evidence.

Source and release-native review find no blocking defect. The intended native
boundary is removed. Broader qualification and timing remain pending; this is
not performance or integration approval.

Reviewed source is `BASE/worktrees/activation53-completion`, branch
`pyjit/activation53-completion`, against golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. The candidate is an uncommitted
five-file diff: four handwritten files and `Python/generated_ir_cases.c.h`.
BASE follows [HANDOFF](../../HANDOFF.md). Review read the applicable agent
guidance, [official AI policy](https://devguide.python.org/getting-started/ai-tools/),
[proposal](completion-proposal.md), [Astra review](astra-review.md),
[resolution](review-resolution.md) and [design](../../design/activation-transfers.md).
It made no runtime source edits, builds, test executions or measurements.

## Source findings

- Both return variants retain the canonical producer proof and its existing
  Publish fallback. Only their generator arm replaces the instruction word
  with result bits and musttail-transfers to the shared terminal. The terminal
  adopts one owner; neither caller uses it afterward. Ordinary return retains
  its direct recipient publication and no new admission check.
- The parent and its homes are reloaded after child cleanup. Eligibility reads
  current code monitoring metadata and pending redirect after that cleanup,
  allowing a child finalizer to install observation. The new regression test
  covers both successful and raising STOP_ITERATION callbacks installed there.
- The unobserved path uses the existing immutable-word A/B/signed-D decode,
  including saved continuations for far adapters. It shares the actual
  `complete_step_impl` detach, publish, continuation and retirement body with
  the cold helper. SEND input overlap remains safe; FOR does not publish a
  returned value over its last item. The exception/sent/cursor/receiver/
  displaced/result retirement order is unchanged.
- Admission-to-commit has no callback. Disabling the shared PRE/POST blocks is
  therefore justified by the current absent monitoring pointer. Owner closes
  happen after the semantic continuation is committed. The body reloads
  `frame->instr_ptr` after closes, preserving callback-selected continuation
  without a pointer-equality test.
- Cold fallback receives the untouched owner before detachment. A real parent
  word replaces the value bits before that call; cold failure refreshes the
  word from the actual frame site before the error musttail. Existing PRE/POST
  failure and redirect recipes remain shared with the selected implementation.
- The interpreter-owned parent uses its existing recipient transaction and
  reads return_pc after publication/retirement. The terminal adds no opcode or
  INSTRUCTION_STATS event, passes lastopcode unchanged, declares the NULL value
  temporary record expected by dispatch/error macros, and runs the existing
  LLTrace resume followed by dispatch. NEXTOPARG initializes its local opcode.

`git diff --check` passes. Generated changes are confined to the two return
arms and match the emitter changes. This is a bounded shared terminal with an
explicit internal transport contract, not a new continuation representation.
Its emitter and handwritten terminal remain trusted code; string checks on
selected generated handlers do not prove arbitrary emitted-body mutations safe.

## Release-native evidence

Inspected `BASE/activation53-completion-release/python` with nm, readelf and
objdump after coordinator regeneration/build. Its identity is the dirty
golden51 HEAD above plus the reviewed five-file diff, not a new source commit.
ELF SHA256 is
`c2c4df6ad8cca41b1639fc9e7826f11f2c40a5115579ea603b1bf46940a4e059`;
GNU build ID is `bb98cecce683625029ea777b5b59fa1bb7e31a0c`.

Both return variants tear down their stack frame and directly jump to the
single `_TAIL_CALL_complete_generator_return` at `0x378590` (return jumps at
`0x381577` and `0x38180e`). Result bits occupy r8, the ir_word argument. The
unobserved path has no call to `_PyEval_CompleteGeneratorReturn`, no struct-status
test and no return to the RETURN handler. Its remaining calls are owner
deallocation. Monitoring and pending-redirect guards retain the cold helper
call at `0x3785d2`; that branch retains status/error handling. Cold failure loads
the actual parent-site word into r8 before tail-jumping to error.

The terminal still has a frame-pointer prologue and 16 bytes of scratch space;
this is not a prologue-free path. It avoids the ordinary helper's five saved
register pushes and 40-byte local allocation in addition to rbp. Up to two held
owners spill into the terminal's scratch around callback-capable deallocation.
After retirement it reloads instr_ptr and tail-dispatches. The hot body contains
the expected window clears, destination store, real continuation stores and
SEND/FOR split; PRE/POST observation calls are absent there.

| Native symbol | Golden51 bytes | Candidate bytes |
| --- | ---: | ---: |
| RETURN_VALUE | 546 | 491 |
| INSTRUMENTED_RETURN_VALUE | 696 | 699 |
| Shared terminal | absent | 813 |
| Cold CompleteGeneratorReturn | 1795 | 1795 |
| CALL_PY_EXACT_ARGS | 973 | 973 |

These selected symbols grow by 761 bytes in total. This is not a whole-image
code-size claim. Ordinary RETURN still goes directly through recipient
publication, its established redirect/displaced-owner handling and dispatch;
the monitoring/pending admission checks are confined to generator completion.
Exact CALL has no new completion check and retains its prior size. Register
allocation and code layout changed around RETURN, so unchanged ordinary source
does not establish identical ordinary-call throughput.

## Executed evidence and limits

Coordinator logs under `BASE/pyjit-link-wave1/activation53` establish the release
build checks 117 modules, including 79 shared, with no import failures
(`completion-initial-release-build.log`).
`completion-initial-release-focus.log` reports SUCCESS for 10 filtered tests
across semantic completion and generated cases, including the new child-cleanup
monitoring test and selected producer checks. Reviewer read these logs and did
not execute the candidate.

The coordinator owns further release/debug qualification. Existing completion,
discard, source-reentry and far-target tests cover relevant behavior, but their
presence alone does not establish candidate passes. Debug LLTrace and stats
contracts are source-reviewed here, without a corresponding native execution.
No measured benefit or merged qualification is established by this review.

## Committed identity addendum

The coordinator committed the reviewed candidate as
`4d40da15b6e2bb9299fa56ef5d08d884a927e7d1` on
`pyjit/activation53-completion`. Static inspection confirms that source HEAD
and a clean worktree. The rebuilt release ELF has SHA256
`1f15b15b5e5b911a4742997b3ad618bebf84a236569d5fbb5ee93b485c2c97b5`
and GNU build ID `6b77c1cf1044b9ebd69a257abae4fb91371e1c03`.
`completion-committed-native.json` records matching source/native metadata;
`completion-committed-release-build.log` checks 117 modules, including 79 shared,
with no import failures.

Reinspection finds the same terminal disassembly, selected-symbol addresses
and sizes, cold helper/status branch, spills and two return tail edges recorded
above. The committed rebuild therefore preserves the inspected mechanism.
This comparison is static and does not claim whole-image binary equivalence.

Keep the `completion-protocols` attempt excluded: its preflight rejected the
old dirty-base native identity against the new source HEAD before test execution.
It provides no candidate test result. The subsequent
`completion-committed-protocols-release-focus.log` reaches test execution and
reports overall FAILURE: generators, coroutines, yield_from and monitoring pass
(309 tests run), while the fifth requested module fails import because
`test.test_ir_source_observation` does not exist. The available module is
`test.test_ir_source_reentry`; its coverage is still pending in this run.
This selection error is not evidence of a runtime semantic failure or a fully
passing qualification run. Debug qualification remains pending at this addendum.
The reviewer did not execute either native image.
