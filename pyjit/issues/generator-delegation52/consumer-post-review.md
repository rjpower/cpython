# Independent consumer post-review

2026-11-12. Read-only review of candidate R52 against selected runtime
`0e5e61f4c6b`. Reviewed actual `Objects/codeobject.c`, `Objects/frameobject.c`,
`Python/ir.c`, `instrumentation.c`, `specialize.c`, relevant handwritten/generated
metadata interfaces, disassembly and active storage readers. The reviewer owns
the compiler slice, not these consumer changes. No source edits, native
execution, builds or tests were performed for this review.

## Finding closed in source

**Source fix verified; native mutation negative pending:**
At review time, `Objects/codeobject.c:1275` admitted normalized completion
`done == site` after the near-D2 bootstrap repair. A forged far adapter whose
CD targets its own DELEGATE, paired with matching CLEANUP_THROW.CD, passed the
local triple/target/handler checks. Completion retires the scratch receiver
window, then re-enters DELEGATE with its receiver NULL. Completion's existing
handler safety proof checks exceptional ownership; it does not establish a
fresh receiver on this normal edge.

The fix restores explicit rejection of `done == site` and both middle words.
This does not interfere with valid near `done == site+3`.
The actual source now rejects `(done >= i && done <= i+2)`, restoring self-target
rejection independently of the ordinary near-D2 admission. The consumer owner
added a paired adapter/cleanup mutation negative. Root compiled this source in
release-build-4 and debug-build-2; both logs check 117 modules with zero import
failures. I inspected the source and those build log tails. Execution of the
specific mutation negative remains pending.
No malformed candidate was executed. Broader arbitrary-jump mutation safety
is outside this finding: this is the bounded new far-delegation contract.

## Source checks and nonblocking observations

| Area | Actual assessment |
| --- | --- |
| Triple and capability | Constructor validates actual A/B/C/D geometry, resume flags/location/target, receiver scratch window, middle-word normal/EH/source ingress and generator entry filtering. Runtime actual-generator-owner guards remain necessary and separate. Constructor flags alone do not grant capability. |
| Two exception contexts | Delegated SUSPEND selects a CLEANUP_THROW with the same window, final recipient and normalized done; DELEGATE does not share that synthetic handler. Far adapter is outside that suspension handler. Completion and source-handler validation use canonical HandlerBase identities, including new IR-only operations. |
| Near-D2 repair | Reviewed the actual correction and coordinator/owner freeze logs: an ordinary near target at s+3 no longer has to resemble a far adapter. An actual IR_JUMP still needs zero unused A/B and a bounded CD. Both initial failing freeze targets subsequently pass. The self-target omission above is a separate, narrower regression in this edit. |
| Far physical versus semantic PC | Dedicated exact-shape `_PyIR_DelegateDoneTarget` is used by LINE landing normalization; branch/source destinations use that normalized landing. Physical dispatch retains the adapter and its INSTRUCTION event. `instr_length` continues grouping only legacy SEND/FOR adapters, never the DELEGATE/SUSPEND/RESUME triple. |
| Event identity | DELEGATE retains semantic STOP_ITERATION/BRANCH_LEFT completion with no instrumentation twin. SUSPEND and RESUME_DELEGATE have explicit PY_YIELD/PY_RESUME mappings. BaseOpcode preserves instrumented IDs for wrapper storage while HandlerBase supplies canonical semantic identities for construction/outcomes. These roles are deliberately different and consistent in the reviewed paths. |
| High handler storage | Event/deinstrument/instrument lookup tables span IR_NUM_HANDLERS. Instrumentation stores original and per-instruction handlers as uint16_t. Line records start at an aligned uint16_t data member and use even 4/6-byte strides; the packed delta begins at byte +2 and each read/write loop covers exactly the remaining 2/4 bytes. Allocation uses offsetof(data)+count*stride. No high-handler truncation or stride/alignment defect was found. |
| Active storage readers | Raw generated LINE access now uses `_PyCode_GetOriginalOpcode`; instrumentation pointers, atomic stores, return locals, per-instruction allocation and the exported deinstrument signature are widened together. Active Include/Objects/Python/Modules/Tools searches found no retained byte-based reader of the changed handler storage. Archived fixtures were excluded as historical evidence. |
| Specialization | The new exact delegate specialization uses the same unary feedback machinery; the selected handler argument is widened to int and stats lookup goes through BaseOpcode. Generic/PEP523/virtual/asend behavior remains runtime provider work, without retrying a completed provider. Free-threaded instrumentation-race rejection includes the new instrumented twins. |
| Frame and public readers | Existing frame successor/owner analyses use IR flags and CD for unconditional jumps, so no frameobject-specific opcode case was needed. Suspension/frame-push effects retain stopped-state exclusions. IR target decoding explicitly separates RESUME_DELEGATE's A flags from CD. Disassembly/findlabels have the same special distinction. Remote await receiver geometry is unchanged. |
| Cost/coverage | Wider line records can increase observed-code memory use; this is a real consequence of representing high handlers, not a hot unobserved frame allocation. No observation performance or full diagnostic claim follows from source inspection. Dynamic install/remove/restart, callback failure/redirect, far completion and malformed entry tests still need candidate execution. |

No second concrete consumer blocker was found in this pass. The unobserved
bootstrap segfault reported during this review was diagnosed independently in
the runtime terminal's access to an interpreter trampoline's uninitialized
frame_obj; it is not evidence of a widened-storage defect. Runtime owns that
fix and generated/native confirmation. Consumer qualification remains
conditional on executing the agreed gates, including the paired self-target
mutation negative. No consumer source blocker remains open in this review.

## Debug dispatch bound follow-up

Root reports contracts2-release passes 152 tests, while contracts2-debug exposed
the retained `INSTRUMENTED_INSTRUCTION` assertion restricting next_opcode to
below 256. Independently approved the bounded correction to
`0 < next_opcode < IR_NUM_HANDLERS`: this is an actual-handler dispatch, its
lookup/storage/table are already in that domain, and the instruction helper
returns int from widened uint16_t storage. The corrected bound is present at
`Python/bytecodes.c:5533`. Regenerated/debug rerun evidence remains the
coordinator's responsibility.

A multiline assertion/check scan over active Python, Objects, internal headers
and cases-generator sources found no other legacy 255/256/0x100 handler bound.
Remaining numeric matches concern dictionary prefixes, byte values, Unicode
characters and allocator tags. No source edit or test execution was performed
by this reviewer for the follow-up.
