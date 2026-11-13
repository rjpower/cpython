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
