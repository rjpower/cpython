# Frame and call model progress

2026-11-09: proposal-only pass against integrated 4b3d739d867 complete. Read
canonical coordination/report/lifetime pages and exact/general binding,
overlapped frame initialization, return and frame-retirement source. Read-only
objdump confirms generic overlap's empty dict allocation, refresh and repeated
guards. No native jobs, runtime edits or qualification claims.

Proposed first coherent increment: stable direct known-Python binding plan plus
lazy empty kwargs under one owning overlapping-window ABI. Semantic result
recipient/continuation replaces tagged POP decoding, coordinated with the
logical-position worker. Borrowed anchors and owner maps are compared explicitly;
existing overlap already removes copies, so neither receives fictitious savings.
Pending review gates include folded operand reads, materialization reentrancy,
frame/remote observer consumers and mutation/version invalidation. Independent
peer review requested before implementation.

Proposal refined after root/peer review: compact executable offsets remain the
public instruction domain; removed POP/NOP instructions have no virtual
INSTRUCTION event, recovery slot or ghost result owner. Source-edge LINE
metadata carries only required source observation. Existing function setters
invalidate func_version; mutable kwdefaults contents need their own validation.
Core empty-dict allocation schedules GC rather than running callbacks, matching
the existing frame-object allocation contract.

Coordinator and independent peer admitted implementation. Runtime source now
has scalar-only guarded binding plans, direct general overlap entry, compiler-
qualified virtual kwargs with MATERIALIZE_KWARGS and frame-publication barrier,
plain IR_NO_DST recipients and explicit return_pc continuations. The first lazy
form excludes kwargs rebind/delete/clear/capture, keyword-only binding and
generator/coroutine code; these use eager binding. Defaults retain Py_ssize_t
until checked for plan-field bounds, including legal overlong tuples.

The five bool/discard aliases, their selectors and checked-NOP generator hook
are removed. Canonical return provenance selects two checked slot-origin
handlers; 33 existing window-to-local sites transfer already-published bits.
Tuple/list unpack uses its proven canonical input fact; generic iterator
unpack remains conservative. See `canonical-transfer-proof.md`.

Host full IR generation in memory, Python syntax and diff checks pass. New
frame-call tests use a read-only internal raw-frame probe to prove execution
of virtual bindings without exposing/materializing a frame. No native builds,
runtime tests or performance jobs by this worker. IR/observer integration and
independent actual-diff review remain in progress.

Frame-worker review handoff: own source chunk is ready for independent review,
not native admission. The known binder is now a separate static inline hit/
miss/error primitive; only a miss constructs the existing out-of-line generic
transaction. Return continuation is a native pointer with the same executable
lifetime as instr_ptr; standard, copied, trampoline, activation-entry and base
frames initialize it and pending kwargs explicitly. The interpreter-entry
trampoline is one real exit instruction. Frame publication saves an existing
exception before materialization and restores it on success.

Tests: `test_ir_frame_calls` covers actual raw pending-state execution, folded
window reads, distinct dict identity, frame/ancestor/profile exposure, eager
declines, nonempty variadics, default mutation/70,000 defaults, structural code
replacement and traceback locals. `test_generated_cases` checks direct-hit/
generic-miss separation under the existing checked prefix. Host number checks,
syntax, full in-memory IR generation and diff checks pass. Shared generated
outputs were not rewritten by this worker. B still owns source-edge observer
integration, static iterator shims, code verification and migration of obsolete
alias/NOP tests; coordinator owns regeneration, native gates and performance.

First merged native attempt: host regeneration passed. Release C compilation
reached bootstrap freezing, which failed with compiler stack-underflow errors
while compiling importlib. The new kwargs materialization operation had been
declared IR-only, outside frontend stack-effect and base-opcode metadata.
The correction under review makes it an ordinary semantic opcode, rather than
adding separate bridge exceptions or giving it an unrelated event identity.
The failed build is retained as `merge1-release-build.log` and
`merge1-build-checks.json` in this packet's evidence directory. No native test
or new performance result has passed at this point.

Independent actual-source review found structural code replacement retained
compiler-derived lazy eligibility; B corrected it. The reviewer also required
a stricter canonical-return provenance guard. A now checks the complete token
contracts and added five mutation cases that reject or retain ordinary
publication. The revised guard passed host checks; native generator tests await
the bootstrap repair. Remaining source-map coverage, shared-owner/loop
completion and debugger resume recipes stay explicit integration work.

Second merged release build: passed, with 117 modules checked. The focused
four-file run executed 27 tests; `test_ir_frame_calls` passed all 11 tests.
The other three files failed: a method lookup encountered occupied output
homes during generator-tool imports, source-only callback exceptions escaped
their source handler, debugger redirects were rejected, and generator cleanup
words still violated the final stream invariant. No suite or benchmark was run
after these failures. Native probes preserve the exact IR and source records.
The last protected pass line has handler -1 while its preceding pass correctly
names handler 3; this is lost frontend exception context, not a reason to infer
the handler from the next executable word. Workers are repairing these causes.

Coordinator merge2 receipt: release build completed 117 bootstrap modules and
all 11 frame-call tests passed. Other focused modules failed; no merged native
admission or performance claim. Annotationlib method lookup found a non-NULL
output-home invariant violation, under investigation. A fixed a distinct scalar
reader bridge gap: IR_NO_DST now keeps the result C-owned across operand
retirement rather than indexing a physical slot. Added custom attribute/item
provider sink coverage and a failure-only method-home diagnostic; no native
run by this worker. Source is frozen for coordinator rebuild. Loop/shared-owner
completion remains proposal-only in the proposal's final section.
