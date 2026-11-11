# C50: checked adjacent method entry into ordinary activation

Status: component proposal for independent review; no runtime implementation yet.
Implements C in proposal50 and Astra's approved two-live-word contract. A owns
canonical method capture/publication; coordinate its actual identifiers before
editing shared sources. Source anchors refer to selected runtime48 `5559f60093e`.

## Bounded first component

Refine `LOAD_ATTR_METHOD_WITH_VALUES` when its immediate next real word is
`CALL_PY_EXACT_ARGS` with zero explicit arguments and the same split window.
Thread-owned caller, GIL build, exact Python function only. Keep both original
words, operands, feedback units and source positions; independent CALL entry
continues to use its existing handler. No compiler/source-map rewrite, NOP,
callee grammar, optional CALL gate, per-hit candidate search or runtime flags
API. Other method layouts retain A's ordinary canonical handler initially.

Nomination happens when either member specializes: method nomination may see an
already-exact neighbor, while CALL nomination may see an already-specialized
method. This second hook is necessary: first-run method specialization happens
before CALL specializes, so nomination from method alone can miss every hot
site indefinitely. Add a small specialization-time pair selector invoked after
successful method and exact-CALL installation; install a distinct method-family
refinement. Never copy CALL's mutable function version, destination or feedback
into attribute feedback. Existing method canonical payload remains authoritative.

The selector checks immutable adjacency, method form, CALL arity zero,
`IR_A(method) == IR_B(call)`, two valid temporary output homes and thread frame.
Source coordinate equality, including same-line equality, is not an admission
condition. Both ordinary words retain their existing line and column maps;
observer admission prevents skipping required intermediate events. Initially
decline code with any exception table rather than introducing another decoder.
At nomination scan immutable words for branch destinations to the CALL; reject
such pairs. Use existing `_PyCode_InstrTarget`, which handles absolute and fused
signed jumps; unknown transfer formats decline. This scan is specialization work, never hit
work. Existing incoming CALL remains executable; rejecting interior static
entry makes the first proof smaller. Pending runtime redirects decline hit.

No feedback geometry change: one new specialized handler shares LOAD_ATTR's
format and canonical two-output ownership contract. Add its mapping everywhere
that enumerates method handlers: deopt/base/format/opcode generation,
feedback row copy/replay and diagnostic snapshots. It must remain a LOAD_ATTR
semantic word for observer/error/reset purposes; do not mark it as the existing
removed-word compare fusion. Existing execution variants canonicalize
`INSTRUCTION_STATS` to their base handler, so a distinct variant opcode alone
does not prove separate hit counts. For this terminal-transfer variant pass its
actual handler to existing `INSTRUCTION_STATS`; arrays already span
`IR_NUM_HANDLERS`. This counts refined handler entries, including misses, not
successful combined activations. Keep existing specialization-hit accounting,
document skipped direct CALL dispatch and use structural assertions/native
evidence beside these bounded counts. No new public hook, counter schema or
always-on counter. Collect stats separately from timing.

## Hit phases and precise ownership

1. **Before capture, original LOAD_ATTR PC.** Check thread owner, GIL build,
   no pending redirect, trace/profile/debug observers excluded, matching current
   instrumentation epoch, current next handler exactly plain
   CALL_PY_EXACT_ARGS, current geometry/arity and no instrumented wrapper.
   Perform existing receiver pointer/type-version and inline-validity guards.
   Under that proof read A's guarded canonical descriptor and require exact
   PyFunction kind. METHOD_DESCRIPTOR alone is insufficient. All misses still
   own the original receiver home and jump to ordinary LOAD_ATTR.
2. **Capture, no callback.** Duplicate guarded function once. For a LOCAL
   receiver duplicate its canonical owner and preserve the local; for a TEMP
   receiver move its owner and NULL the input cell. Use A's checked canonical
   output facts. Capture function before any possible receiver retirement;
   this path retains self, so no receiver release occurs. Keep both owners in
   C locals. Neither pair output has been published yet.
3. **CALL admission, captured pair.** Set caller `instr_ptr` to the original
   CALL word before any call error or generic fallback. Read CALL's own feedback
   using its live D operand; preserve PEP523, current function-version,
   `co_argcount == 1`, stack-space and recursion checks. Current exact function
   proof replaces only CALL's representation/kind rediscovery. No callback can
   change the pair/epoch/neighbor between phase 1 and this check in the GIL-held
   interval. On failure publish the captured canonical owners at the ordinary
   split window, keep the original CALL word and dispatch CALL there. This is
   CALL's original state, not a replayed lookup. The ordinary handler may deopt,
   grow stack or invoke hooks; its window owns both references exactly once.
4. **Commit.** Write self to `window[1]`, which is the physical overlapping
   argument home; that store cannot be eliminated by retaining a real frame.
   Keep function C-owned until direct transfer to `f_funcobj`; do not publish
   it to window[0] just to reload/clear it. Preserve NULL window[0] contract.
   Save CALL destination and CALL+1 continuation, perform ordinary exact push,
   set child current, recursion decrement and usual RESUME admission/entry.
   Independent executable owner and constants initialize normally.
5. **After commit.** Ordinary callee body, return, exception and retirement own
   completion. No C-specific return, ledger or delayed owner state.

Temporary receiver may alias an output cell only if ordinary method def-use
permits it: consume first into a C owner, NULL its old cell, then write self.
A local receiver never becomes an output because method destinations are
compiler-owned temporary windows. Debug assertions check output/window homes
are NULL after input consumption and before writes. Never add a second reference
for the same counted owner on fallback. If nomination cannot establish legal
window ownership from immutable operands and frame-local geometry, decline.

## Shared implementation boundary

Do not copy the exact activation implementation into a second handler.

Extract the existing `_INIT_CALL_PY_EXACT_ARGS` storage/ownership code
(`bytecodes.c:4322`) to one `static inline` internal frame helper, tentatively
`_PyFrame_PushExact(tstate, callable, self_or_null, args, nargs, previous)`.
Its signature contains actual owners/argument homes, no mode Boolean or flags.
It consumes callable and supplied argument owners; retains existing thread
`PushOverlapping` and generator `PushUnchecked`/copy alternative. Ordinary CALL
continues to load its same inputs and call this helper. C supplies the captured
function/self and real argument base. Its thread guard allows the compiler to
remove the generator alternative. The helper can retain existing window[0]
NULL store (it is already NULL for C); the important eliminated traffic is
function publication/reload and repeated canonical/kind checks. If a retained
redundant NULL store proves material, simplify from an explicit shared input
contract, not an extra runtime flag.

Extract the current `_SAVE_RETURN_SLOT` two stores and `_PUSH_FRAME` entry
sequence into narrow ceval macros reused by their DSL uops and C's terminal
entry. Tentative interfaces `IR_SAVE_CALL_CONTINUATION(dst, pc)` and
`IR_ENTER_PYTHON_FRAME(child)` preserve the exact current stores, current-frame
publication, recursion decrement, slots/IP reload, RESUME skip and DTrace/
LLTrace entry. These are compile-time source sharing, not another out-of-line
call. No macro may evaluate an owner argument twice. Keep all existing call
producers using the same entry sequence.

Keep A's terminal canonical producer unchanged. The refinement attaches a
bounded terminal-consumer descriptor to the execution variant, rather than
appending a transforming uop or weakening A's terminal-producer rule. The
descriptor names the complete canonical output vector and a checked emitter
for its next consumer. It carries no runtime flag or cached mutable CALL facts.

Before generation, `canonical_output_sources` still proves the unchanged
guard/capture chain and every owning producer source. A separate positive
terminal-transfer validator proves: scalar nonwindow base format with exactly
two new canonical outputs; output names/order match the producer and consumer;
producer is the final uop; declaration is its final statement; no flush,
intervening transform, owner reuse, array or extra external output; the variant
retains the base LOAD_ATTR operand/feedback/source identities. Reject a consumer
that changes the argument/window relation or asks for a borrowed owner. The
consumer's contract accepts canonical owned homes, not merely values with a
canonical representation.

Canonical ownership is necessary but not sufficient for function/self roles:
the consumer validator also binds output position zero to the guarded-payload
duplicate and position one to the unchanged receiver TAKE. Reuse A's checked
sole-assignment/source-expression helpers, or expose their validated source
kinds; do not infer roles from C variable spelling. Reject reordered canonical
sources or two unrelated canonical outputs even if their representation is
valid. Exact Python function kind is established by the pre-capture hook on
that same guarded payload.

The consumer descriptor has three compile-time hooks, with fixed validated
placement. Its entry preflight runs after ordinary handler declarations but
before the lookup chain and checks current geometry/neighbor/observers without
touching the receiver. Its target-kind hook runs after the existing lookup
guards and immediately before the canonical producer's first statement; it
reads the guarded descriptor and checks exact Python function kind without
acquiring or retiring anything. Both failures go to ordinary LOAD_ATTR with
the original receiver intact. The marker hook runs only after the existing
function duplicate and receiver TAKE and therefore must never replay lookup.
The validator binds the two earlier hook locations to the actual proven
producer and prefix; it rejects any capture, escaping operation or owner change
before those hooks. No post-capture guard may use the pre-capture miss target.

At `IR_CANONICAL_OUTPUTS(attr, self)`, the normal emitter remains a no-op for A.
For a validated terminal consumer, its emitter accesses exactly those produced
C locals and emits the complete phase-3 admission with two terminal outcomes:
canonical pair publication plus dispatch at CALL, or self-home placement plus
shared ordinary frame commit/entry and dispatch. Each outcome consumes both
owners exactly once, with no returning path. The replacement reports the
current path unreachable to the existing generator control-flow machinery;
therefore no ordinary output flush follows. This is terminal owner transfer,
not a Boolean exemption suppressing final stores. A failure to prove either
terminal path, an added statement after the declaration, changed owner list,
extra suffix or a reachable fallthrough is a generation error. Negative tests
cover these cases. No general dataflow analysis or opcode-name waiver is needed.

A's implementation owner confirms the concrete storage boundary: marker-time
outputs are defined C-owned `Local` values and TAKE has already emitted input
consumption; no output home has been published. Returning False from the
replacement skips `storage.push_outputs`, final output emission and default
dispatch. It also bypasses normal end-of-uop definition/liveness checks, so
the terminal validator must explicitly prove the exact live output vector and
order, each `Local.in_local`, absence of publication/spills/extra owners, and
consumption of the original input. Never call `storage.flush`, `prepare_flush`
or `_push_defined_outputs` at this boundary: they publish the pair and lose
the intended saving. The kind hook runs after cache binding (so class_value is
available) and existing receiver guards, before the first capture assignment.

A read-only in-memory generation of committed A confirmed marker storage for
all three ordinary method producers: `spilled == 0`, `peeks == []`; the consumed
input has `in_local == False`, `memory_offset is None`, `ir_inplace == 'B'`,
`ir_input == -1`; attr/self each have `in_local == True`, no memory offset,
`ir_inplace is None` and `ir_input is None`. C's bounded validator will require
these proved ownership conditions (expressed through format input fields rather
than hardcoded B where possible), plus no additional live input. This is
actual generator-storage evidence, not permission to relax producer validation.

Execution metadata must describe the refinement's added frame entry: extend
the variant metadata producer to retain the base operand/window/feedback and
semantic source/event identity while conservatively adding
`_PyIR_PUSHES_FRAME`, `_PyIR_RUNS_PYTHON` and `_PyIR_ALLOCATES` execution effects.
Keep LOAD_ATTR's family `RUNS_CODE` guarantee and ordinary semantic events;
instrumentation must restore ordinary method execution and retain the separate
CALL word/event rather than invent a CALL event at the method PC. Test the
effects and row/snapshot/deopt mappings explicitly. The existing shape-only
variant's metadata remains unchanged.

Ordinary CALL admission still uses existing DSL guard uops unchanged. C repeats
the required mutable guard comparisons against its captured function but omits
representation/kind conversions already proved. This is not duplicate stack
allocation or a second activation implementation. A later shared admission
emitter is unnecessary unless native code demonstrates extra work.

## Observation and live neighbor state

Both real PCs survive. Lookup failures stay at LOAD_ATTR; captured admission,
binding errors and child execution attribute to CALL; return resumes CALL+1.
Initial eligibility requires no Python trace/profile, PEP523, supported debug
tracing or pending observer work, plus exact plain current neighbor and an
instrumentation epoch match. Derive epoch handling from existing RESUME/runtime
instrumentation contracts; do not cache an observer bit forever. Free-threaded
builds decline nomination. Ordinary method fallback still supports all observers.

Monitoring mutates live handlers and reinstruments executing frames. A current
neighbor handler check defeats a CALL replacement. A refined method itself
must map correctly through instrumentation/deopt; its original method word
cannot skip an instruction/line event while retaining the refinement. No user
callback, decref, allocation or GIL release occurs after phase-1 checks until
commit or captured-pair fallback, so observer installation cannot interleave in
that interval. DTrace/LLTrace builds decline initial nomination unless existing
entry events are explicitly proven equivalent. Pending redirects decline
before capture and continue through ordinary lookup/CALL.

## Verification and stop conditions

First generate and inspect the handlers. Structural tests must prove refinement
is installed, both words remain, independent CALL entry works, live neighbor
replacement defeats refinement, and incorrect geometry/targets/observers/kinds
are rejected. Use existing Py_STATS handler counters separately from timings
for actual hit coverage; no permanent public census API.

Behavior tests: local/temporary receivers and aliasing, method/type/MRO
replacement, shadowing and dict materialization, code/default mutation, recursion
and stack fallback, exception locations, tracing/profiling/monitoring enable and
disable, captured-target fallback with reentry, frame inspection and ordinary
return callbacks. Keep standalone exact CALL's focused regression set.

Coordinator performs real release compilation and native comparison: ordinary
CALL should retain essentially its current hot path; refined method must omit
one dispatch and actual function pair conversion/publication/reload, with no
new out-of-line activation call. If the helper introduces such a call, repair
source sharing before measurement. Then matched DeltaBlue/logging/nbody against
frozen48, with independent component review. Stop/simplify if admission/code
size outweighs the removed boundary work; do not add arities, callee recipes or
new method layouts to rescue the first experiment.

## Concrete focused test placement

Extend existing `test_ir_attr_cache_codegen` for the validated consumer descriptor
and generated control flow; reuse its real-source generation and synthetic
renamed producer cases. Require both owned canonical outputs at handoff, terminal
unreachable return, and absence of automatic pair stores. Mutations reject wrong
output order/count, reassignment/extra suffix, added owner spill, borrowed output,
extra live input, moved preflight/kind hook and false effect metadata.

Extend `test_ir_attr_cache` beside `TestCanonicalMethodCapture` using existing
`get_ir(code, True)` and warmup helpers. Assert refined installation before each
mutation/observer test. Add a multiline parenthesized method call whose LOAD_ATTR
and CALL have different line and column spans and assert it qualifies; this
proves coordinates are attribution metadata, not a hidden gate. Test standalone
CALL after refinement decline/replacement, temporary receiver weakref lifetime,
`__code__` arity mutation that forces captured CALL fallback, instance shadow,
materialized dict, method replacement, and normal traceback pointing at CALL
when the callee raises. Observe both original source positions when tracing or
instruction monitoring disables refinement. Keep no-dict/lazy-dict and explicit
argument methods as ordinary A paths in the first component.
