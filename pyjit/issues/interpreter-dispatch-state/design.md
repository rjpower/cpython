# Derived slot-base lifetime in GCC dispatch

Research scope: 2026-10-31 23:14:20–23:44:20 UTC; interim due 23:29:20.
This is a new host-only design investigation. It does not reopen the rejected
thirteen-handler operand increment or authorize implementation, compilation,
target execution, counters, workloads or timing. The coordinator owns status
and any next bounded decision.

## Hypothesis

The maintained computed-goto evaluator carries three mutable C state values
between handlers: current frame, instruction pointer and cached slot base.
The third is derived: `_PyFrame_Slots(frame)` returns `frame->base`. The accepted
baseline already keeps frame and IP in registers at the sampled common entries.
Its return/yield paths nevertheless load the resumed frame's base and store it
in a C stack cache just before dispatch. Removing that cross-handler cache,
while retaining a local alias within each handler, could remove that transport
load/store pair. It may also reduce allocation pressure when the handler
repertoire changes, but that is a separate stability hypothesis. It can lose:
a fresh frame-base load may add a dependency, and GCC may recreate the old cache or
keep spilling frame/IP. This is an unmeasured hypothesis, not an explanation
already proved by the rejected candidate.

The smallest credible source experiment is therefore **handler-local slot-base
derivation**, applied to the maintained source without the operand prototype.
Keep `frame` and `next_instr` as the existing dispatcher state. Do not pin
machine registers, change instruction words, add a state struct/ABI, move IP
publication, or create an optional runtime mode.

## Minimal design

For non-tail execution, the generator would give each handler a mutable local
`PyValue *slots = _PyFrame_Slots(frame)` alias. Existing IR_SLOT, GETLOCAL and
LOCALS_ARRAY readers keep their names and one underlying frame-base authority.
An unused alias has no semantic work; native inspection must verify that it
does not introduce a load in handlers that never use it. The function-wide
non-tail `slots` declaration goes away. Keep the existing reload assignments in
source: when no slot use follows them in their handler, the compiler can remove
the now-dead local assignment. The actual slots, their numbering, clears,
stores, physical compaction and roots do not change.

Entry placement is part of correctness, not formatting. Family heads have a
PREDICTED label inside their handler after its ordinary prologue. Derive the
alias after that label, alongside the existing this_instr binding, so direct
specialized misses do not skip initialization. Ordinary table entries derive
it before their body. Transparent instruction/line wrappers must reach a fresh
alias when they redispatch. Common error, exception_unwind, exit and start_frame
labels have their own lexical scopes and need the same explicit derivation
where they use slots. Initial throw handling enters those labels through the
existing IP/frame protocol; it must not depend on an outgoing cache variable.

Keep every existing `RELOAD_STACK`/`emit_fullreload` after a frame switch. It
updates the current handler's local alias after frame is changed, before that
handler reads or writes the new frame. The alias cannot be const because some
handlers execute in more than one frame. Window arguments still point into the
same frame slots, and return/yield stores still publish outputs before unlink
or clear. The tail backend retains its slots parameter and existing convention;
this design does not incorporate deferred H1 frame passing. The evaluator's
initial throw path needs its slots declaration/assignment only for the existing
tail call; non-tail throw enters the common error label's own scope.

The shared generator and its existing frame-switch/reload analysis remain the
authority. There is no handwritten handler allowlist, arithmetic copy, second
effect table or schema extension. A prototype should touch the non-tail local
declaration, generated handler/label entry binding, and only the necessary
reload plumbing. Needing a new continuation ABI, manual per-handler exceptions,
or independent frame ownership analysis would reject this small cut.

## Existing entry and reload coverage

The frozen generated file has 287 actual TARGET bodies and 19 predicted joins:
CALL, CALL_FUNCTION_EX, CALL_KW, COMPARE_OP, CONTAINS_OP, FOR_ITER, GET_ITER,
LOAD_ATTR, LOAD_CONST, LOAD_GLOBAL, LOAD_SMALL_INT, LOAD_SUPER_ATTR,
POP_JUMP_IF_FALSE, POP_JUMP_IF_TRUE, SEND, STORE_ATTR, STORE_SUBSCR, TO_BOOL and
UNPACK_SEQUENCE. The new alias must follow the predicted label in each such
body, before any feedback/window/body slot access. The preceding ordinary
prologue publishes IP and records statistics; it does not consume slots.
Both direct table dispatch and specialized predicted misses then initialize it.

The five generated common labels are error, exception_unwind, exit_unwind,
exit_unwind_notrace and start_frame. Each needs a non-tail local alias after
TAIL_CALL_LOAD_FRAME, inside its existing scope. The unknown-opcode path has
no slot use before entering error. Initial throw also enters error directly;
inlined frame entry goes through start_frame. Same-oparg redispatch and
transparent instruction/line wrappers enter a fresh handler. Exception-unwind
self-jumps enter the label again. These routes must not inherit an outgoing
handler's alias. The switch backend must obey the same entry discipline;
tail handlers keep their existing parameter and frame-loading convention.

All 26 existing reloads remain. Their inventory is 13 frame-entry sites,
three plain return/yield sites, six instrumented sites and four assignments in
common labels. In particular, exception_unwind reloads before writing lasti
and the exception into frame slots, then reloads again after the observer.
Those writes require a current alias. Instrumented yield has three separate
reloads covering failure, f_lineno redirection and normal frame resumption.
The machine-readable inventory in results.md names every site. This is entry
coverage for review, not proof that all 26 assignments can disappear.

## Root and observer obligations

`_PyEval_EvalFrameDefault` stays Py_GC_PRECISE. Its C stack is not a replacement
root; the existing frame-slot publication and precise-root checks remain
mandatory. A raw pointer to slots does not anchor an otherwise unrooted value.
No new leaf annotation or root exemption is justified. Local aliases use the
existing `_PyFrame_Slots` accessor and `IR_SLOT`/window rules so root analysis
does not gain a parallel notion of storage.

Publish `frame->instr_ptr` at exactly the same places, retain next_instr's
current/next-word conventions, and preserve error/RERAISE locations, f_lineno
redirection, monitoring wrappers, specialization/deopt, lltrace, recursion,
PEP523, call/return/yield/resume and unwinding. The proposal does not remove
publication as a way to save the observed loads. Tail, switch, debug and
free-threaded compilation must remain coherent under their existing backend
conditions; no new user feature flag selects the experiment.

## Falsifiable next decision

The source evidence supports a bounded prototype after independent design
review; no compiler-pass attribution is required first. If a concrete allocation
uncertainty needs resolution, a separately authorized compiler-only diagnostic can
replay the exact saved GCC compile command on the unchanged parent and rejected
sources, recording IRA/reload diagnostics and disassembly. That would ask
whether the spilled frame/IP values actually share the long-lived slot-cache
allocation region and whether a different transformation explains the change.
It is not a new optimization flag, benchmark or retuning of the rejected
source. It must write separate task-owned objects/dumps, preserve the original
cwd, compiler and optimization flags, and prove the resulting object/native
code matches the saved artifact before using its allocation story. A mismatch
is a reported diagnostic limit, not permission to replace the saved image.
This optional diagnostic has not been prepared or performed in this scope.

If independent design review selects a prototype, use the current maintained
baseline, with the rejected operand source preserved unchanged. First require
host generator/root checks and generated entry-scope tests for ordinary,
predicted, transparent wrapper, frame-switch and common-label entries. Reject
any path that can read an uninitialized or stale alias. Then one separately
reviewed native feedback batch must exercise calls, generator/coroutine
return/yield, exceptions and unwinding, tracing/monitoring and precise GC/OOM.
No runtime handler domain or arithmetic test expectation should change.

Useful existing target controls include test_ir.AssemblerTest's
test_exception_from_a_pushed_frame, test_frames_when_instrumented and
test_yield_to_each_kind_of_resumer, plus RegCfgTest's pushed-frame result and
generator-local controls. Generator throw/close/frame-lifetime controls and
test_monitoring's f_lineno callback, instruction and exception events cover
the other entry routes. A frozen feedback plan must choose exact case IDs,
root-injection modes, origins and timeout before execution. New host entry
tests must actually check all 19 predicted joins and five labels rather than
asserting only a total occurrence count. These checks would be prepared by a
prototype; none were executed in this research scope.

Native evidence must compare the accepted baseline's YIELD_VALUE, RETURN_VALUE
and RETURN_GENERATOR normal exits, plus the existing six common control entries
on matched GCC O3 images. Follow actual fast/fallback paths rather than stopping
at a conditional branch. Test the specific claim: does the frame-base load plus
outgoing cache store disappear on those exits, and what replaces the next
handler's cached-base load? Record dependent frame-base loads, other spills,
frame/IP residency, dispatch and complete function text as evidence. If GCC
reconstructs the same cache, report that the work-removal mechanism failed.
If frame/IP regress, report it explicitly. Neither an instruction count nor
text size is an automatic performance veto or acceptance rule.

Only a separate premeasurement decision could authorize elapsed feedback. A
reasonable held selection uses the same four registered workloads and counts,
two rotating-order rounds and five samples per cell. Coroutines is the nominated
call/return consumer; generators provides yield/resume evidence. Preserve the
existing registry loops: coroutines 4, generators 2, unpack_sequence 4000 and
spectral_norm 1. Unpack_sequence and spectral_norm are controls for common
dispatch. The primary claim is work
already present in the accepted baseline, not removal of spills seen only in
the rejected image. Predeclare a useful elapsed benefit and control-regression
rule before new scores; no instruction counter
or arbitrary size limit substitutes for performance. This research neither
launches that campaign nor fixes its acceptance threshold for the coordinator.

The rejected operand source is a retained negative showing sensitivity to
handler changes, not a second candidate in this prototype. The first experiment
changes only the maintained baseline's derived-state lifetime. It makes no
promise that another enlarged repertoire will retain the same registers.

## Alternatives excluded from this cut

- Enabling `-fno-gcse`, disabling SLP, or switching back to field loads is not a
  new fix: all three policies are already present in the measured GCC images.
- Deleting the full-word C variable or merely delaying IP increment is not
  established work removal: release dispatch already loads a 16-bit handler,
  field readers use narrow memory loads, and the candidate already rematerializes
  next IP at the tail of simple handlers.
- Register keywords and lexical braces do not establish a machine-level
  residency contract. Fixed-register or inline-assembly constraints introduce
  compiler/ABI/root-tooling obligations without evidence that they solve this
  case; they are not the initial design.
- Caching frame state in a struct adds aliasable mutable storage instead of
  reducing derived state. Reloading the entire frame from tstate at every
  handler makes H1's tail-only convention into a new GCC cost.
- Outlining every handler or adding a new entry/continuation ABI is much wider
  than removing one derived cache. H1 remains deferred for an actual matched
  Clang-tail consumer; it has no maintained-GCC win to borrow here.
- Removing frame IP publication, frame clears or precise roots changes runtime
  semantics and is outside this hypothesis. Code-size growth alone does not
  identify cache causation or authorize any of those changes.
