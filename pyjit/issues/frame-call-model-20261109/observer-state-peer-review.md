# Observer state: bounded source review

## Pending source errors

Approve moving the pending frame/record pair into _PyEvalActivation. The
evaluation loop checks recursive C entry before linking its activation, then
restores the previous pointer on both normal and early_exit returns. Initialize
the pair before publishing the activation. The pre-activation recursion failure
clears its child while leaving the parent activation intact. Python finalizers,
trace/monitor callbacks and call-tracing reentry enter through a fresh evaluator
activation; VM child transfers alone do not run another source callback during
outer error retirement. Consuming only a matching current-activation recipe
keeps nested failures from replacing the outer selected handler.

Frame/code owners remain reserved until exception lookup and retirement;
non-owning frame storage therefore needs no extra reference. The cold setter
must not rely on frame-object allocation succeeding. The frame-object cursor
continues to serve source display/traceback, separate from handler selection.
The coordinator's plain-Python nested-error probe supplies a concrete failing
case; this review has run no native tests.

## LINE edge handoff

Approve dedicated cold frame-object fields for expected compact landing PC,
previous source line and monitoring epoch. f_lineno must retain only its public
display/cache role. Detach a record before callbacks, require exact landing and
epoch, clear before debugger redirection, and initialize every frame-object
allocation. Frame ownership transfer to a dead copied frame should clear the
handoff rather than preserve a continuation that cannot execute.

Monitoring data is allocated once and freed only with the code object. A frame
owns that code, preventing an allocation-reuse epoch collision. Increment the
epoch before old source_forced wrappers are removed, not after a possibly
failing reinstrumentation. Local event changes and restart must also invalidate
the record. A stale record at the expected landing requires fresh landing LINE
semantics with an unknown previous line; physical prev==next must not suppress
the newly enabled observer. Do not replay previously completed source edges.

Far relaxation requires an explicit observed-only rule. The assembler emits
semantic operation, jump-to-body, jump-to-exit; completion can first continue at
the exit jump while the source map names the ultimate exit. Existing dest_offset
already follows one IR_JUMP, so no missing source-record lookup was demonstrated
in this review. Tighten that unchecked normalization and instr_length's greedy
grouping to the exact immutable relaxation shape. Use its normalized landing in
the handoff; helper words have NO_LINE and still execute their own INSTRUCTION
events. The independent cold record then survives them without the old f_lineno
save/restore. Do not skip arbitrary source-bearing jumps or introduce a chain
walk on ordinary dispatch. With this explicit rule, the three-field handoff is
sufficient and needs no extra source-position domain.

## Actual pending-error patch disposition

Reviewed the coordinator's concrete patch: the cold setter requires a current
activation, initialization precedes publication, and exception lookup clears
only the matching activation recipe and any existing display cursors. Normal,
early-exit and pre-activation recursion paths retain their established link
restoration. The frame-allocation failure branch records an activation recipe
without needing a frame object. The new ordinary compiled control/nested-error
test exercises both cold and warmed functions. Source admission approved;
native17 and allocation-failure injection evidence remain separate gates.

## Source18 iterator and callback-epoch disposition

Reviewed the concrete co_lines candidate cache: rows sort by offset/line,
deduplicate in place without overwriting unread input, and filter equal-real
rows with one forward location cursor. Cached merged real bounds avoid a
backward seek after a source row splits a range. The merge retains distinct
lines at one offset and real-range coverage. Initialization and deallocation
cover allocation failure. Source admission approved; native timing remains a
separate coordinator gate.

The explicit epoch output now pairs the final source line with the epoch
captured immediately before its selected callback. A last callback that changes
configuration leaves a stale pair; a later event under the new configuration
can legitimately replace it. The direct landing LINE route applies that same
test before suppressing a duplicate. Reviewed source matches the admitted
contract, closing the producer-after-callback epoch finding. A separate debug
entry/version assertion is under diagnosis and is not qualified by this review.

## Confirmed entry provenance finding

Native19 diagnostic evidence isolates a stale code version at PC1 with correct
monitor masks. Its printed pristine map has RETURN_GENERATOR at PC0 and RESUME
at PC1, plus a source record from 0 to 1 at the function definition line. The
code generator gives its synthetic initial POP input drop that same location;
eliminating it incorrectly promotes compiler bookkeeping to a source LINE span.
The resulting LINE wrapper executes before RESUME's existing instrumentation
synchronization and before PY_START/PY_RESUME ordering is established.

Approve a producer correction: the initial generated input drop has no source
location/origin. Keep ordinary source spans after RESUME, so outgoing source
records from RESUME remain valid and run after its entry event. A record that
targets RESUME is not an admissible source-only landing; reject it in the
constructor and assert that invariant in wrapper construction. This fixes the
false boundary at its producer instead of adding version checks to dispatch or
extraordinary synchronization to every frame entry. Native validation and the
actual implementation remain coordinator-owned.

## First debugger NULL chunk: actual-source review pending two repairs

A read the new register_resume_sites and setter application plus shared
NullableHome. Root's two concrete findings are confirmed. The initial helper
accepts the first nullable alias; a required read alias of the same home must
instead defeat NULL admission. Count all actual explicit and audited window
read roles, excluding untouched window holes. Generic STORE_ATTR's nullable
value cannot excuse its required aliased receiver. Scalar and mixed-role
constructor negatives are needed.

Warning-hook reentry needs a cold frameobject redirect generation, not equality
of PC/continuation/source cursor/public line cache. An identical-PC successful
nested setter is still new intent; a getter changing f_lineno is not. Initialize
new frameobjects, preserve the generation on escape, check overflow before any
binding/detachment effect and increment at each successful actual or deferred
commit after validation/allocation succeed. Failed setters do not increment.

The complete-boundary NULL invariant, actual-handler role check and
all-surplus-detach before continuation commit/close are otherwise consistent
with the reviewed bounded plan. Active child overlap retains its separate gate.
Source admission remains conditional on the two repairs and focused tests;
no worker native execution.

Actual repaired NULL chunk source admitted: ReadsWindowHome factors only the
previous window audit; ReadsHome remains explicit/window union. NullableHome
now requires every actual aliased input role to accept NULL and returns false
for no read. Constructor and debugger pass the argument extent. The aliased
STORE_ATTR receiver negative is present. f_lineno_generation initializes in the
common New_NoTrack allocator used by both frame construction routes, stays on
the frameobject through escape, checks overflow before warning/effects, and
increments only after validation/allocation at actual or deferred commit. The
same-PC warning setter and getter-only controls cover intent vs public cache.
No hot interpreter-frame field added. No remaining concrete source blocker;
coordinator native validation pending.
