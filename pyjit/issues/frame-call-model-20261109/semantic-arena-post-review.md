# D11 semantic completion and arena post-review

2026-11-09. Read-only source review of runtime branch
`pyjit/frame-call-model-20261109`, HEAD `6ef45e391aa` plus the uncommitted
semantic16 increment. Canonical documentation lives in this checkout; runtime
source is in `build/worktrees/execution-boundaries-20261108` under the separate
`python-boil-the-ocean` checkout. This report does not claim native passage,
release compilation, regeneration, performance qualification or integration.
The coordinator owns those gates. No runtime source was changed.

Read the applicable AGENTS instructions, CPython AI policy, canonical entry
pages and the semantic completion, initialized arena, nullable input, prior
proof review and remote-layout proposals. Reviewed actual completion handlers,
generator activation/return, frame reservation/retirement/copy, constructor
proof and shared cold operand/output contracts. Prior closed findings are not
reopened without a counterexample.

## Must fix: pending source-error recipes are lost across retirement reentry

**Source-only finding; native reproduction is pending.**

`Python/instrumentation.c:1511` records a failed source-only LINE callback in
one thread-global pair, `source_error_frame/source_error_record`, and also
records its index on the affected frame object's `f_source_event_error`.
`Python/ceval.h:530` consumes the recipe only through that thread-global pair,
then clears the frame object's cursor. A complete source census finds no
activation save/restore of the pair.

This does not compose with semantic completion retirement. After a POST
source callback fails, `Python/ceval.c:2968` closes exception, sent value,
cursor, receiver, displaced local and discarded completion value before the
caller reaches exception lookup. Any last owner can run a Python finalizer.
If that finalizer runs another instrumented function whose source-only LINE
callback fails and whose error is caught locally, the nested callback replaces
the global pair, and nested exception lookup clears it. The outer frame's
pending recipe is then absent. Outer exception lookup falls back to the real
semantic instruction's handler instead of the failed source record's handler.
That can choose a different protected region or miss its handler, and leaves
the outer frame object's source-error cursor uncleared. The same singleton
also makes any other intervening reentry capable of losing the pending recipe.

The singleton exists in the committed baseline; this increment adds the direct
completion callback-error to owner-retirement path. `call_one_instrument`
restores `tstate->tracing` before returning the callback failure, so finalizer
execution can receive instrumentation normally. `Objects/typeobject.c:13718`
preserves the raised exception across `__del__`, but does not preserve the
source-error pair. The outer exception therefore survives while its handler
recipe is lost.

A bounded fix should make the pending error recipe frame scoped. The existing
frame-object field already carries the same record index and survives nested
frames; use it with appropriate validation and clearing, or save/restore the
thread state at a real activation boundary. Avoid adding a hot frame field or
a second observer-state mechanism solely to stack an already frame-local fact.

Coordinator reproduction ladder:

1. Construct a source-only LINE edge whose source-record handler differs from
   the real predecessor's handler; make its callback raise an outer marker.
2. Have semantic completion own a last mortal iterator, discarded returned
   object or displaced local. Its finalizer runs a nested function with its
   own source-only LINE callback error caught inside that function.
3. Verify both nested and outer errors reach their own handlers and that the
   outer traceback uses the source-only line. A public `code.replace` fixture
   can pin the edge/handler distinction if an ordinary compiler fixture is
   awkward. Exercise a real compiled case as well if practical.
4. Repeat without nested failure as the control. Inspect that consumed and
   redirected frame-object cursors are cleared.

This is an exception attribution and reentry contract defect, not a demand for
CPython's exact finalizer timing. Once this implementation elects to run the
callback and retire those owners, the pending error must survive that reentry.

## Bounded source admissions

No concrete additional must-fix was found in the reviewed initialized arena
paths. `_PyStackChunk_EnsureFrameStorage` truncates the frontier before header
writes and fills only uncertain value storage. Overlap preserves the maximum
of the requested end and the existing physical top. Pop retains the current
frontier rather than restoring a stale saved copy. The full departing physical
suffix is checked for NULL in debug builds. Direct heap initialization fills
owned slots once; escape/generator copies move and clear every source slot.
These checks support the intended arena invariant but do not replace native
debug coverage with a dirty allocator, former header reuse, recursion failure
and retirement reentry.

No new counterexample was found in `_PyIR_ReadsHome`, `_PyIR_DefinesHome` or
`_PyIR_NullableInputs` against the reviewed emitted store modes. In particular,
through prefixes do not become definitions, WINDOW_DST_FIRST moves only its
actual first result, and nullable roles do not create NULL facts. The separate
U/N proof and encoded handler checks remain necessary. This is a bounded cold
ownership review, not a general type verifier for arbitrary hand-built IR.

The distinct PRE/POST statuses intentionally share the generated unwind entry;
constructor proofs must make both physical states safe for that handler.
CompleteStep detaches all window homes before publishing an overlapping SEND
result and before any retirement callback. Generator resume commits links,
exception state, current frame and recursion entry before discarding an unused
mortal input. No per-item generic completion interpreter or new hot frame field
was found in these paths.

## Remaining integration gates

The pending source-error reentry finding needs coordinator reproduction and a
reviewed repair. Existing known f_lineno recipe, final LINE, co_lines and
relaxation-fixture work remains outside this finding. Broad native/debug
qualification and logging parity/broad-regression measurement remain open;
this source review does not change their status.
