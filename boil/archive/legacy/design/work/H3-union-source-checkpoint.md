# Isolated H3 union: source checkpoint

2026-10-29. Managed worktree .worktrees/boil-h3-union, branch boil/h3-union,
starts at frozen primary f2a5b3ca33d1ed5a526983f9094137c03120f37b and composes
stats parent5d108561e7af08189e29fe237c7e15c48305e54b (including original H3
81417aff097 and reviewed native statsd462e8338ce). Root explicitly authorized
this isolated source-preparation exception while primary GC gates run.
213GB was free before creation. No original tree, product, environment or
historical artifact was modified. No union native build/test/probe ran.

The merge had no textual conflicts. Six overlapping paths were reviewed:

- bytecodes.c keeps ROOTS _Py_Check_ArgsIterable(tstate,callargs_o), plus H3
  monitoring/domain accessors, no-monitoring guard and DSL properties.
- ceval.h keeps ROOTS precise annotations/exception spans, plus full-domain
  name lookup and H3 monitoring eligibility.
- ceval_macros.h keeps ROOTS precise helpers/conversion cleanup, and stats's
  full-domain entry/pair updates before predicted labels. Tailcall forwarding
  and activation-local lastopcode lifetime are unchanged.
- _testinternalcapi.c retains ROOTS walk/state/root fixtures and bytes owner-root
  helper/table entry, adding the H3 scalar/storage/domain controls. No shared
  fixture was replaced.
- test_generated_cases.py retains ROOTS C-checker fixtures, H3 domain/generator
  tests and stats adaptive/entry checks.
- generated_ir_cases.c.h was regenerated from the combined handwritten source;
  it was not hand-edited or selected from one branch.

Primary PROCESS/README/WORKLOG and global gate_run bytes are unchanged from
f2a5. All ROOTS/bytes native files outside the H3 delta remain unchanged,
including collector, bytes writer ownership and raw-mutex precision markers.
H3 retains its monitoring/storage/domain behavior and stats producer/reader/
benchmark rename; no H1/queue/generation/allocator policy entered this patch.

## Host verification

Using shared-cache uv and /usr/bin/python3 pinned to CPUs2,3, regenerated all
seven products via existing opcode_id, uop_id, py_metadata, opcode_metadata,
ir_generator (cases+targets), and ir_metadata commands, in serial order.
Second generation was byte-identical; the first also matched the automatic
merge byte-for-byte. Six products equal the isolated stats parent. Generated
IR cases additionally contain ROOTS's argument/helper and resulting temporary
root/boxing changes; copying the stats parent's generated file would lose
those changes. IDs/dispatch target table remain unchanged; no renumbering.

Full host test_generated_cases plus the15 reader tests initially ran90 tests:
87 ordinary passes, two failures and the existing expected failure
TestRootCheckC.test_block_local_beside_frame_result; zero errors/skips. Both
failures were existing literal expected-output strings in test_feedback_mapping
and test_oparg_in_entry: OP_NOFB still expected INSTRUCTION_STATS(OP). Updated
only those two strings to OP_NOFB, as the reviewed actual-target contract
requires. Both cases then passed once. Other passing tests were not repeated;
the first failed run remains retained, not relabeled as a full green run.
These are host generator/checker/reader checks, not native union qualification.

Evidence under this tree's ignored build/H3-union:
regen_union.py and regen-result.json (exact commands/seven hashes/idempotence),
product-comparison.json (parent comparison), host-tests.log/result.json
(initial failures), host-snapshot-fix.log/result.json (two corrected passes),
and runtime-paths.txt. Native/test/tool diff whitespace checks pass.

## Review boundary

This checkpoint needs independent source review before any native build.
The inherited private counter driver remains bound to its isolated d462
native products; it is not an actual-union executor and must not be rerun here
as if those products existed. A fresh union Py_STATS build/raw producer-reader
closure and actual call/frame/mode/root controls remain prospective. No
standalone prior receipt is a new-head pass. H3 later consumer/sampling/
renumbering stages, H1 passed-frame experiments and finalizer/generation
proposals remain separate. Root owns primary integration, native execution
plans and the later qualification/landing decision.
