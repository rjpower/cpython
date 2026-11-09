# M2b independent source review

2026-11-08. Reviewed the isolated runtime's five-file source diff against
`0539e0c1ff96ccf2a1d95e96f99c9e499b20552a`, including the final no-box
classification check. Verdict: accept for coordinator regeneration, release/
debug compilation and focused execution. No must-fix finding. This is a source
pass, not a generated-product, compile, executed-test or performance pass.

The producer is explicit: only ordinary POP_JUMP_IF_FALSE_BOOL appends the
zero-I/O terminal uop. The emitter records that intrinsic and emits it after
the common final stack flush, before dispatch. Its per-handler state resets
on both generation passes. The validator checks the trusted guard/branch/uop
sequence, final placement, scalar stack effects, actual exact-bool guard,
branch property, no result/window/local destination, no releases or escaping/
poll/frame/error effects, and no required private boxing. It no longer copies
entire guard and branch C bodies into a second string; their semantic
implementations remain small trusted primitives checked by this review.

The primitive proof holds in current source: the guard admits only bool,
the branch uses that same condition, consumes its temporary or leaves its
local owning, records branch history and resolves its ordinary edge. There
is no callback between that point and the intrinsic. No result or displaced
local exists, and flushing makes the consumed temporary NULL first.

The runtime macro is GIL-only and declines under debug lltrace>=5. Its fresh
atomic target-word read accepts exactly NOP, publishes that retained boundary's
PC and advances once. A wrapper or any other target falls through unchanged;
instrumented branch handlers are untouched. Ordinary NOP itself has no epoch,
poll, DTrace or observer callback, so this does not require the stronger RESUME
skip predicate. No Python thread can install instrumentation inside this
nonescaping GIL-held span. P+1 still uses the ordinary dispatch path and debug
checks. The NOP word, source offsets, exception tables and f_lineno target
remain present. Actual NOP handler-entry stats fall on accepted edges; the
semantic boundary and PC publication remain.

The new generator test checks successful placement and rejects changed guard,
escaping-prefix insertion and reordered terminal use. The IR test warms the
specialized producer, retains a concrete NOP, enables opcode tracing from an
inner call, checks the NOP offset, exercises a line jump, both monitored
branches and an observer error at that offset. Source review does not establish
that those tests were discovered or passed; the coordinator's execution will.

The diff has no whitespace errors. Review the regenerated handler and GCC
output after compilation: accepted edges should avoid the NOP function
dispatch, while fallback wrappers retain ordinary observation. The fail-edge
extra word read and instruction layout still need target/control evidence.
No speedup is inferred from the ten candidate NOPs per logging loop.
