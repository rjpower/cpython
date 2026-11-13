# Option 4 progress

Proposal independently approved in option4-peer.md and coordinator authorized
implementation. Handwritten source is ready in BASE/worktrees/frame54-locals;
independent actual-diff review by frame54_fields gives final source GO. No commits,
generation, compilation, tests or other native execution by this lane.

The new owner is PyFrameObject.f_locals_mapping. Generic mapping entries prepare
an empty untracked object before thread reservation, commit the mapping and
function owners nonfallibly, and keep that private thread record untracked until
complete observer publication or complete heap move. This supersedes the prior
early-tracking prescription, which exposed semantically incomplete frame objects
through GC enumeration. Unique object retirement holds the mapping in C until
homes retire.
FRAME_OBJECT mapping access derives its containing object rather than creating
a self edge. Copies take an explicit destination owner and serialize association
publication. Generator copy installs complete homes, owner, first-send recipient
and object association before a release store makes its destination resumable.
Direct frame-to-generator conversion prepares coroutine origin before ownership
commit and holds the frame lock through shared metadata and GC publication.

Ten new tests are in test.test_frame.TestFrameLocalsMapping, and two C API
tests in test.test_capi.test_frame.FrameTest. The new frame_new_generator helper
exercises PyFrame_New-to-generator conversion. Allocation-failure coverage uses
existing call_with_nomemory. The two publication regressions check exec/class
GC enumeration invisibility and an unobserved mapping→generator→record cycle.
These tests are written, not executed by this lane.

Root handoff: regenerate generated_ir_cases.c.h after the _PyFrame_Copy signature
change in RETURN_GENERATOR, rebuild release/debug and deepfreeze through normal
build dependencies, then run focused tests and actual native-layout gates.
Source git diff --check passes; searched handwritten consumers contain no old
f_locals member references. Debug offsets use offsetof and deepfreeze uses
FRAME_SPECIALS_SIZE, so no manual numeric geometry changes were needed. GDB and
remote tooling have no direct mapping-field reader to port.

Native 112-to-104-byte contraction, no ordinary optimized allocation, new class/
exec allocation and tail cost, clear-path code, free-threaded compilation,
semantic passes and throughput remain coordinator-owned gates. No measured
claim is made by this lane.

## Qualification corrections

The coordinator's generated-contract inspection found that moving CREATED into
the copy helper hid RETURN_GENERATOR's suspension effect from the old direct-
write inference. The focused semantic helper-call inference and tests have
independent source GO in option4-effect-review.md; root owns executed evidence.

The new C-API conversion test exposed another preexisting register-ABI omission:
PyFrame_New sets the default incoming recipient to slot zero and advances beyond
the initial RESUME, while gen_new_with_qualname used to copy that recipient
unchanged. Its earlier impossible frame_obj assertion masked this path on the
selected debug runtime (confirmed by the coordinator's public-API probe).
After repairing ownership conversion, initial send(None) filled scratch zero
and the first constant store failed the NULL-home debug assertion. Under the
existing frame lock, conversion now selects IR_NO_DST before Copy publishes
CREATED. Objects/codeobject.c:1217 rejects any RETURN_GENERATOR destination other
than IR_NO_DST, so the cold C-API conversion now matches that actual initial-
send contract. The existing test_frame_new_generator_mapping remains the
behavior regression; no test was removed or weakened. Fields independently
approved this source correction. Root reports corrected b48 release/debug focus
passing in each role: frame/C-API 84 tests with eight skips, seven core modules
640 tests, and the two generated-effect checks. Root retains executed evidence;
these are focused checks, not full-suite qualification.

## Mapping-path diagnostic

mapping-path-probe.py supplies separate exec/class steady-work diagnostics:
precompiled timeit statement/setup, a separately precompiled answer=17 exec
payload, reused namespace/environment per sample, 20000 iterations by default,
one warmup and seven raw-duration samples. Its JSON records interpreter/git,
JIT/hash and the default timeit GC disable/restore policy.
Class setup explicitly collects type cycles before each run outside its measured
interval, as requested by independent source review; automatic collection
remains disabled during timing in both runtime roles. This diagnostic does
not measure startup or replace unchanged workloads. It was written without
execution; fields approved the final source, including class-only setup
collection. Coordinator quiet balanced execution owns the next step.
