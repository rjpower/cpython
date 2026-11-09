# Explicit observed LINE landing handoff

The initial observer17 patch used f_lineno to carry a predecessor line through
completion. That field is a public lazy cache: -1 requests computation and a
getter can replace it. The pending patch must be replaced in full, including
ordinary source-edge handoff and instruction-wrapper save/restore of f_lineno.
There is no fallback based on f_lineno being positive or nonzero.

Append a cold record to PyFrameObject: int next_offset (-1 inactive), int
previous_line (including -1 for unknown), uint64_t epoch. next_offset is a compact
real instruction offset in this frame's retained code. Add uint64_t line_epoch
to the code's monitoring data, initialized on allocation and advanced before
each force_instrument_lock_held mutation. Global instrumentation_version alone
is insufficient: local event changes force instrumentation at the same global
version. Monitoring data is freed only with its code; the active frame retains
that code, so a pending record cannot outlive and accidentally match a newly
allocated monitoring object.

A common cold producer ensures the frame object, records the expected landing,
final edge source line and current epoch. Completion POST produces even when no
source-only span advanced the line; ordinary jump/source-edge observation
produces when source events advanced it. Allocation failure follows the existing
POST or jump error route. The source callback f_lineno override and
f_source_event cursor remain separate and retain their public/display purpose.

The LINE wrapper detaches the pending record before any callbacks. A matching
landing and epoch supplies previous_line and proves that this source edge was
already observed; LINE then emits only when its actual landing needs an event.
A mismatching landing or stale epoch cannot supply an old predecessor. Successful
f_lineno redirects clear the record before committing a PC or closing owners,
including deferred ancestor redirects. _PyFrame_New_NoTrack initializes it once;
frame copy/escape preserves the same frame object's record. A cleared frame never
resumes. There is no interpreter-frame field, ordinary dispatch lookup or cursor
maintenance.

The reviewed bridge rule is structural, not an arbitrary jump-chain walk.
Assembler relaxation emits exactly [FOR_ITER/SEND, IR_JUMP(site+3),
IR_JUMP(realexit)]. A producer normalizes an immediate bridge only after
checking this shape in pristine code, including the semantic operation's
second-helper target. Compiler helpers have NO_LINE in line instrumentation;
their actual INSTRUCTION callbacks still execute. The record expects the true
landing and survives those callbacks unless they redirect or change the epoch.
The existing dest_offset already follows one helper for source-edge lookup;
this proposal tightens that one-hop assumption rather than claiming a newly
observed missing source record. Logical branch grouping remains separate from
real instruction events. Ordinary source-bearing IR_JUMP words are not skipped.

For a matching expected landing but stale epoch, detach the record and treat
the landing as fresh with an unknown predecessor line. Do not replay old source
records or use the old previous_line to suppress the new epoch's LINE. The
record supplies no old callback context. A wrong landing simply discards it and
uses the normal current transition. Reconfiguration is cold; no per-dispatch
epoch maintenance is introduced.

Existing exact trace sequences remain tests. Add finalizer reconfiguration,
public f_lineno reads before landing, source-edge plus real helper INSTRUCTION
observation, local/global event changes and restart cases. Root owns native
validation. This is an observed-path correctness repair, not a performance claim.


Implementation prepared for native17: exact triplet recognition is shared by
instr_length, jump_dest and dest_offset. The code epoch fails on overflow before
mutating wrappers. Completion records a landing whenever LINE remains active,
even when no source-only records exist. Constructor fixtures passed root's
native16b retry and the compile corpus admitted all 2151 valid files (11
intentional SyntaxErrors). The new finalizer matrix targets this VM's chosen
retirement path, not a Python prompt-destruction requirement. Host test AST and
diff checks pass; actual source peer and native17 remain pending.

## Source18 epoch correction

A found that capturing the epoch at handoff publication stamps a callback's new
configuration onto a line observed using the old callback snapshot. The producer
now accepts an explicit observation epoch. Callers initialize it before scanning
an edge, and the scan refreshes it immediately before each selected source event.
Thus the last event's callback can invalidate its epoch; a later event actually
observed under the new configuration can legitimately establish a new final
line/epoch pair. Multiple tools within one event retain the pre-event epoch
because the snapshot can omit newly enabled tools. A direct landing wrapper also
checks the returned epoch before suppressing its own LINE. Tests cover both a
last-event reconfiguration requiring a fresh same-line landing and an earlier
reconfiguration followed by an event in the new epoch.

## Source18 co_lines complexity repair

Root retained the observer17 failure: NameLimitTest consumed minutes in
lineiter_next; 209 perf samples were all repeated location-table seeking, with
zero lost samples. The bounded 40/80/160/320 scaling probe grew about 7–8 times
per doubling. The first future-row filter fixed duplicate output but made the
existing full source-record rescan substantially worse. This is a real iterator
regression, not a test timeout allowance.

The replacement iterator caches static (to,line) candidates once, sorts and
deduplicates them, and removes equal-real-line candidates using one monotonic
location cursor. Output merges that stream with a second monotonic real-range
cursor. A cached merged real-range end prevents a source split from seeking
backward on the next output row. Construction costs O(r log r + location payload)
and traversal is linear in emitted rows and payload; storage is O(r). Checked
PyMem_Calloc handles allocation size/failure, iterator deallocation frees the
cache on every path, and the source map remains immutable. Runtime source-event
order is unchanged. Coverage and equal-offset row tests contain no timing limit;
root owns the bounded post-build scaling check and exact stopped-method retry.
