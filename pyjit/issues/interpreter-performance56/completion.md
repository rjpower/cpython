# Direct native completion entries

Status: source implementation committed at `40b2858cf50` with GC actual
four-file source review GO; no native or performance result.
Root owns full generation, builds, diagnostics and measurements. This is a
bounded follow-up to [exact-tuple preparation](call-ex-tuple.md), using the
[combined design](../../design/proposals/interpreter-performance56.md).

## Baseline evidence and applicability

The isolated P branch `pyjit/perf56-direct-completion` starts from frozen I
`196284139a1`. The prior tuple and retained-runtime branches are preserved.

The retained baseline assembly is
`E55/scalar-results-baseline-native/_PyEval_CompleteStep.txt`. It shows actual
incoming stack loads at `rbp+0x10` (value) and `rbp+0x18` (kind), several
kind-dependent tests/branches, a `0x48` local stack reservation, five saved
general-purpose registers and a frame pointer. Which branches execute depends
on the kind, monitoring and outcome. This evidence is from the retained
baseline ELF, not a native measurement of the new source.

The sole generated caller is `IREmitter.complete_step`, reached at an actual
`IR_COMPLETE_STEP` terminal. Its source already supplies a literal kind:

| Entry | Reached operations | Frequency unit |
| --- | --- | --- |
| FOR | Generic/instrumented and list/tuple/range/virtual FOR exhaustion | One terminal exhaustion, not every item |
| SEND | Generic native/virtual SEND completion and async-generator-asend completion | A native completed step, not every yield/resume |
| SEND_THROW | CLEANUP_THROW handling StopIteration | A completed delegated throw, not every throw |

Normal generator RETURN is different: `_PyEval_CompleteGeneratorReturn`
already calls the same implementation with a canonical input and literal
SEND/FOR kind after decoding the immutable caller word. Its native-entry
branch, decoder and canonical paths stay unchanged. Ordinary Python RETURN
already publishes inline and is also unchanged. Thus this packet is not a
generator whole-call redesign, and a generator benchmark's total resume count
cannot be used as its applicability count.

## Interface and invariant

Replace `_PyEval_CompleteStep` and its sole generated use with:

- `_PyEval_CompleteForIter(tstate, frame, site, base, exit, value)`;
- `_PyEval_CompleteSend(tstate, frame, site, base, destination, exit, value)`;
- `_PyEval_CompleteSendThrow(tstate, frame, site, base, destination, exit, value)`.

Each entry calls the existing `complete_step_impl` with its literal kind and
`canonical_input=false`. FOR passes `IR_NO_DST`; the common body never reads
the destination on any FOR route. On x86-64 SysV, the incoming argument count
changes from eight to six/seven, potentially eliminating two/one stack
arguments. No new return record is introduced: the existing 16-byte result
still returns through registers. Other platforms must price their actual ABI.

The common implementation body is unchanged. Keep native-result Publish,
PRE/POST observation, source/error origins, redirects, window detachment,
recipient displacement, publication before cleanup, exception preservation,
one-pass retirement and continuation reload. No early local steal, descriptor
lookup, frame field, tail-call parameter or handler-body copy is added.

The emitter selects a direct entry only for the three supported literal kinds;
unsupported marker kinds fail source generation. This does not add a semantic
grant: the existing reached source, normal-completion, abrupt and error
receipts remain mandatory. Changed target geometry still loses the existing
proof. Helper spelling establishes no new canonicality or ownership fact.

## Review and checks

GC and IR give design agreement with the unchanged-body/receipt boundary.
GC additionally reviewed the retained disassembly evidence and gives actual
source post-review GO. Static comparisons confirm that the common completion
body and GeneratorReturn remain byte-identical. Generated products remain
unchanged until root regenerates the combined source; this commit alone is
not a buildable generated checkpoint.

The existing source test
`test_generated_cases.TestIRCases.test_completion_result_local_does_not_capture_held_value`
now emits only the 16 selected handlers, in both record modes. It checks exact
direct signatures, held-result identity, FOR's absent recipient parameter,
absolute CLEANUP_THROW target, unchanged generator-entry/RETURN routes, and an
unsupported-kind mutation. The focused host method passes. Its first attempt
used the ordinary-instruction dictionary for an IR-only EMPTY return; the
fixture now uses the shared `all_instructions` lookup. No full generation,
native execution or build was run in this lane.

Root should retain current SEND/FOR normal/error/observer/source-recovery
checks, alias and discarded-recipient cases, finalizer/GC and frame-escape
coverage, and generator send/throw/close behavior. In particular, PRE redirect
must preserve untouched inputs, POST failure must retain committed state and
its actual error origin, and source recovery must continue selecting the same
transfer. Existing helper/action receipts must not erode in regenerated
metadata. The separate native-result normalization path must remain fallible.

## Root counts and falsifiers

Count these boundaries separately in a diagnostic process:

- Old baseline `_PyEval_CompleteStep`, split by `_PY_IR_FOR_ITER=0`,
  `_PY_IR_SEND=1`, `_PY_IR_SEND_THROW=2`.
- Candidate `_PyEval_CompleteForIter`, `_PyEval_CompleteSend`, and
  `_PyEval_CompleteSendThrow` by symbol, plus the unchanged
  `_PyEval_CompleteGeneratorReturn` as a separate denominator.
- PRE/POST monitor paths and terminal failures separately from successful
  unmonitored terminal completion. Ordinary returns and yielded items are not
  direct-entry hits.

At the exact entry address of the retained x86-64 baseline symbol, argument
seven is at `rsp+8` and argument eight at `rsp+16`; the prologue's `rbp` loads
confirm that layout. A debugger breakpoint placed after the prologue cannot
reuse entry `rsp` offsets. Root may instead use a retained diagnostic counter
patch; ordinary timed binaries carry no counters.

The predicted removal is incoming kind/destination traffic and dead kind
branches, not one less helper call or less ownership cleanup. Inspect all
three optimized entries and their callers, total text size, stack reservations,
spills and calls to the observer/retirement machinery. Three specialized bodies
can cost more code space than the generic body. If optimized native already
eliminates this work, the mechanism has no cost result. If terminal hit counts
are negligible, make no broad claim. Parent/hot6 confirmation and all29 gates
still decide selection; nominal argument count alone cannot admit it.

## Frequent generator handoff

The next independent slice is based on I `6a00c6a08d4`, compared with pinned
upstream `3a440b65695615e9fd9912342b79cdd54fced0d2`. Both already enter
specialized SEND/FOR generator frames directly, link the exception stack and
return yielded values without a new Python call. The fork also skips an inert
RESUME. The upstream stack interpreter pushes to a known stack position;
the fork publishes to an explicit recipient, supporting pending redirects,
discard and displaced named-local owners. Those semantics must survive.

There is a concrete admission gap: `names_return_slot()` does not recognize
the actual terminal `IR_ENTER_GENERATOR`. Consequently `FOR_ITER_GEN` selects
`STORE_LEAVE`, and its generated prologue deoptimizes whenever A names a local
or `IR_NO_DST`. Store elimination can give an ordinary `for item in iterator`
that local A. The intrinsic already writes the correct parent return slot
before linking the child, and a child yield uses `SetReturnValue`; the missing
compiler capability prevents that implemented path from being used.

Root's retained generator profile has the matching generic outer path:
`FOR_ITER` → `VirtualIteratorNext` → `gen_iternext_value` → `EvalFrameValue`.
`SEND_GEN` accounts for 27.91% physical-symbol self samples and
`YIELD_DELEGATED` 13.94%; these identify frequent boundaries, not removable
percentages. Evidence is under `E56/call-ex-perf-record`. Root's complete
generator comparison reports about 49.3% more native instructions and 47.2%
more cycles than upstream. The admission gap is a mechanism hypothesis for
part of that difference, not an attribution of the whole gap.

The isolated proposal/implementation uses a small shared `GeneratorEntryRecipe`
from the actual top-level terminal intrinsic, literal kind, scalar IO and
unchanged two-input prefix. Its legacy frame output has only the unreachable
stack-model consumer. Preceding guard parts use the shared typed effect walk:
no arena write, callback, origin adoption or unknown helper. No opcode-name
permission or second full-body table is introduced. The same recipe is
consumed by store selection and actual intrinsic lowering. FOR yields to A;
SEND yields to B+2 independently of completion A. Existing normal/error/WAIT
receipts remain separate and unchanged.

For FOR, select the existing `STORE_RETURN` protocol and remove the local/
discard fallback. Do not steal an old local early or cache it across child
execution. The eventual yield publishes first, then retires the actual current
displaced owner. A finalizer or observer can mutate that binding. Discard closes
the yielded owner through the same established protocol. Keep child, parent
continuation and exception-state commit before any failing entry or owner
cleanup. A type/state/PEP-523 miss still falls back before that handoff.

The source expectation is removal of the unconditional local/discard deopt and,
for affected sites, the nested native iterator/evaluator call chain per item.
The source is unchanged for scratch recipients. Measure actual entered
`FOR_ITER_GEN` sites and failed local-recipient guards, rather than just seeing
a specialized handler in feedback: the old handler can remain installed while
deoptimizing every time. Falsify the hypothesis if sites use scratch recipients,
the direct entry count does not rise, or optimized native/workload costs do not
improve. Extra source checking adds no hot runtime descriptor or frame field.

Focused source fixtures emit actual FOR/SEND handlers in both record modes and
reject changed literal/IO, conditional/duplicate/suffixed terminals and poisoned
prefix guards. Runtime fixtures cover L replacement with a finalizer that
observes the published value, mutates locals and collects; trace callbacks;
scratch tuple-unpack and constructed discard; and a different iterator type at
the same warmed site. Native qualification remains root-owned.

## Publication caller audit

At I `6a00c6a08d4`, `SetReturnValue` already receives a canonical owning value;
it does not call Publish. `PublishReturnSlot` is used only by that helper.
Its home check is diagnostic, and its live work is recipient selection,
publication and displaced-owner retirement. Removing normalization from these
two helpers would therefore remove no existing normalization.

| Actual producer | Value boundary and publication |
| --- | --- |
| `IR_ENTER_GENERATOR` SEND | Moves the already canonical sent home; FOR supplies canonical None; both use `SetReturnValue` |
| Delegated suspension | TAKE of the canonical outgoing home, then `SetReturnValue` after suspension commit |
| Ordinary YIELD | Checked canonical frame transport selects `IR_STORE_CANONICAL_RETURN`; no Publish |
| Ordinary/EMPTY RETURN | Checked canonical frame transport, or explicit Publish for an uncovered native C result, then the same final publication |
| `CompleteGeneratorReturn` | Canonical input; native interpreter entry uses `SetReturnValue`, ordinary SEND/FOR uses literal canonical completion |
| `gen_send_value_ex` | Canonical None or an owning duplicate of its borrowed API argument, followed by `SetReturnValue`; the duplicate cannot be dropped without argument-lifetime proof |
| Remaining `IR_STORE_RETURN` expansions | RETURN_GENERATOR and the terminal cleanup result retain their explicit native-to-home publication boundary |
| Native CALL result transaction | Has its own actual `_PyValue_Publish(&result)` boundary; this is separate from `SetReturnValue` |

Rare redirect/displacement paths may still force register liveness, branches or
spills in every SEND/yield handler. That needs optimized native comparison;
flat inline attribution alone cannot establish which instructions disappear.
Any later outlining or once-admitted recipient geometry is a separate measured
increment, preserving all public constructors, native entry, callbacks and
storage authority. Do not add a universal per-yield descriptor switch.

Root also observes recurring `FillConstSlots` attribution in coroutine exact
calls. This is follow-up inventory only: count existing constant-slot folding
and entry stores before proposing pool/rematerialization changes, and charge
any added per-read decoding. No constant or frame-encoding change is included.
