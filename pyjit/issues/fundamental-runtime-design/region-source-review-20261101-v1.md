# Independent region source review

2026-11-01. **ACCEPT for bounded fresh-image correctness feedback.** Reviewed
clean commit `40498d1e0d3773ac2a1180eb8cac7fd04831028d` against parent
`5c24163405d0d70f568720a43931a1859599a260` in
`build/worktrees/interpreter-regions`. All 18 changed files and the relevant
unchanged compaction, dispatch and instrumentation bridges were read. Candidate
capsule `build/interpreter-region-source/candidate.json` has SHA256
`543a8e9c9d100835131cbcd84e7b90858f0fdbf41d55702932cac29ca21a4efa`.
No concrete source blocker remains. Execution, landing and performance require
separate coordinator decisions.

The planner runs after rewrites and before compaction. Admission requires a
fresh result with exactly one read, including implicit operands, no local or
canonical destination, no live-out/handler obligation, matching complete source
position and exception context, and a same-block forward false branch. Plans
are compiler-owned and freed on compilation exits; allocation failure propagates.
Post-compaction release checks validate actual homes. Assembly additionally
requires adjacent single words, the intended ordinary forms and private feedback
at both words. Shared feedback and relaxed/multiword branches remain ordinary.
No instruction, frame home or feedback record is removed.

The generator composes the existing integer comparison and boolean branch
through IRStack. The reusable step rejects collecting, reentrant, error,
service, frame-switch and stack-synchronizing effects. All type rejection occurs
before input consumption. The bool remains in C until the logical branch
consumes it; final flushing clears consumed input homes, including admitted
aliases, without the intermediate bool-home store/reload. The branch retains
history, TO_BOOL hit bookkeeping and NOT_TAKEN handling. Both IP stores remain.
The guarded region is two forward logical operations with no new loop or
service point; pending breaker/version state disables admission.

Trace/profile hooks, frame observation, monitoring storage and debug lltrace
disable fusion. Retained monitoring storage conservatively keeps fusion disabled
even after events are removed; that accepted limitation is explicit. The
ordinary integer handler body remains byte-identical after the fast block.
Generic comparison, truth callbacks and errors use ordinary materializing code.
Guard misses enter the actual ordinary generic body, not a same-head pristine
deopt/redispatch loop. The second word remains independently executable with a
materialized value. This supplies bounded safepoint behavior, not an atomic
asynchronous snapshot guarantee.

Handler IDs 302/303 append to the full domain. C/Python metadata, formats,
pristine/specialization state, names, dispatch tables and uint16 specialization
selection agree. Both forms map to semantic COMPARE_OP. Existing instrumentation
uses `word_opcode` to recover that base and `_PyCode_HandlerFor` to restore the
pristine region form; no new instrumentation switch is needed. Code construction
bounds lookahead before reading the second word and validates the temporary,
feedback ranges/kinds, forward target and bool comparison before accepting a
pristine region. Executing specialized form 303 is rejected as pristine input.
The new magic separates compiled caches.

Independent host replay reproduced seven generated products, all 287 existing
handler IDs and all 287 old generated handler bodies. All 79 existing/new host
source tests passed (78 ordinary, one existing expected failure); six affected
C/root checks returned zero. The in-memory materialization fault caused exactly
the intended mechanism assertion failure and no error. Six changed Python
modules parse. All 18 source hashes, 23 artifact bindings and 12 frozen references
match; the worktree remains clean. No compiler or target ran. Reviewer receipt:
`build/region-source-review-20261101-v1/receipt.json`, SHA256
`7b4fd2a828c6d665cae20d5194087a75ef64eb3e5cc2b0318f54344b4521bc15`.

The 12 new native methods are appropriate first feedback: production planner
admission/rejection, integer and generic semantics, callback/error/local changes,
observer restoration, operand aliases, shared feedback, history, malformed words
and a real direct branch entry with its own materialized bool. Planner negatives
overlap guards and do not prove every predicate independently. None of their
native behavior is credited yet; existing IR, exception, generator/frame,
monitoring/debug and native-root controls remain necessary.

Before accepting a performance claim, require actual registered-site compiler
admission and receiving/fallback behavior, linked native inspection showing the
intended path and its spills/traffic, and matched candidate-versus-parent net
elapsed. Aggregate captured pair counts establish a family opportunity, not
per-PC fast-path execution or recoverable time. The held, unbuilt volatile-home
control also clears inputs early and again at final flush; it is not an isolated
measurement of only home-roundtrip traffic. This is an attribution limitation,
not a source blocker or additional prerequisite for the initial correctness
batch. The all-29 and resource decisions remain separate; source expansion and
global compiler/layout effects must be charged to the candidate.
