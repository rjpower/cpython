# Frames/calls lane progress

2026-11-12: read applicable instructions, official CPython AI policy, canonical
README/STATUS/GOAL/HANDOFF/PROCESS, target design, migration plan and resolution.
Inspected selected54 frame layout, allocation, copy/clear, entry/trampoline,
generator construction and typed-header consumers. No native execution/builds.

M0 comparison and M1 contract are in [pins-proposal](pins-proposal.md), accepted
by root and [GC peer review](pins-peer.md). Shared placement/NULL boundness remains the
reference; separate regions, clean ranges and explicit local validity have
distinct entry/exit/packing/observation costs. Mortal bits do not encode boundness.

Important findings: current code/function PyValues already own counted mortal
references under FT; independent code lifetime must survive function mutation;
cleared generators retain code; partial generator construction has no code yet;
escaped copies require a temporary departing-source code pin. Interpreter entry
and base frames use None today; typed NULL sentinels require remote owner-first
validation. GC lane has the proposed field types and reader obligations.

Implemented the approved M1 scope in `BASE/worktrees/call55-pins`: typed pins,
direct accessors, strong acquisition/clear/traversal, NULL entry/base and partial
generator sentinels, code-backed trampolines, all lane-owned readers and internal
hook helpers. Kept source code duplication on escaped-frame takeover. Removed
the value-returning code-home helper and migrated its test to typed pin counts.
Added independent-pin escaped-frame and completed-generator lifetime regressions.
Own diff review and `git diff --check` pass. No candidate tests/builds run here;
root owns generation, focused execution, native evidence, integration and commits.

Reviewed the GC proposal in [gc-peer](gc-peer.md). GC incorporated fixed-last
code lifetime and reserved-storage assertions before M2; M1 reader source review
requested preservation of remote high-pointer rejection before sentinel bypass.
Awaiting merged release/debug checks and independent post-review.

## M2 producer and first precise bridge

Implemented in `BASE/worktrees/call55-pins` over integrated M1
`49c081b035821feb472faf59d905ad73bdcc6d4b`; GC source peer GO in
[m2-peer](m2-peer.md). Root's subsequent M1 cold API and escaped-frame test
corrections are separate integration changes and must be retained when applying
this patch.

`_PyFrame_TryClearExceptCode` clears only callback-free owners and returns a
stage/remaining prefix before the first possible reentry. Outlined noinline
continuations then link the nested GC record and finish all retirement. Thread
retirement keeps the record through final code release, disarms the result, pops
the record and releases the reservation without further callbacks. Generator
completion and direct close/deallocation share `_PyFrame_ClearGenerator`, keeping
the record through exception-state release while preserving the original lock
scope. Code remains pinned for completed generators. The obsolete general clear
entry has no remaining callers.

The real generator exception owner required mask bit16; takeover moves homes and
function/mapping/association authority while preserving independent code plus
the original generator exception. Every cursor/header removal precedes close.
Collectors enumerate explicit fields after code detach and never decode freed
geometry. Debug assertions check nested calls restore chunk/top/limit and the
thread frame's saved frontier. FT routes any counted object close to the slow
path; the GIL last-reference test alone is not a concurrent proof.

The precise test helper visits transition roots directly, bypassing conservative
fallback. Eight fixtures cover nested result/remaining/outer roots with arena
growth and collection, late publication after caller-local mutation, final code
constant destruction, earliest frame-association/trace destruction, unstarted
generator deallocation, generator completion, direct deallocation retaining
GeneratorExit state, and unwind cleanup. No tests or native builds run in this
lane. Own diff check passes; root quiet timing is respected.

Source cost ledger: no home clears or conservative scans are removed in M2.
Callback-free retirement adds the last-reference proof before a close, the
result-aware boundary passes/returns one value, and the thread gains one nullable
head pointer. The agreed record is 32 bytes on the initial 64-bit GIL layout and
exists only in a reached noinline continuation; nested reentry adds one record
per active retirement. These are source/layout obligations, not measured native
instruction or elapsed claims. Root must regenerate the RETURN template and
escaping-call classification, compile both roles, execute the fixtures and
inspect actual fast/slow native stack frames before integration.
