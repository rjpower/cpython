# C50 actual-source review

Reviewing the C working delta on A50 `e014965013b` in the execution-boundaries
runtime checkout. This report owns review findings; the coordinator owns native
builds, committed identities, merged validation and measurement. No runtime
edits, builds or commits were made by this reviewer.

## First coherent boundary

Independent in-memory real-source generation confirms the intended control
flow: preflight precedes lookup; existing pointer/type/inline-validity guards
precede guarded target access; function acquisition precedes receiver TAKE;
the marker sees two C owners with the original input consumed. No ordinary
pair flush follows the terminal consumer. Admission failure publishes both
owners at the original CALL PC and dispatches CALL once. Success stores self
in the ordinary overlap, transfers function through shared PushExact, saves
CALL+1 and uses the shared ordinary frame-entry sequence. No custom return or
retirement machinery appears.

PushExact mirrors the previous thread-overlap and generator-copy branches,
including NULL clears. The shared generated entry calls Storage.full_reload
for ordinary CALL and supplies the same runtime entry for the terminal path.
Nomination considers both method-first and CALL-first specialization, installs
the int handler without uint8 truncation, checks temporary/window geometry,
rejects exception tables/interior static targets, and uses InstrTarget. Live
raw-handler and epoch checks defeat observation/re-specialization changes.
Canonical feedback/history mappings and conservative frame-entry effects are
present. Entry counters include misses, as documented.

## Findings and resolutions

- A's canonical representation proof alone did not establish guarded payload
  lifetime for the consumer. The worker added positive validation of the
  actual type-version and inline-validity guard source/stack shape.
- Source-body equality alone omitted annotations/properties affecting emitted
  pointer checks. An independent modified-source generation accepted an
  inline_ok type guard. The worker now rejects changed guard annotations,
  unexpected caches and escape/frame/sync effects; negative coverage was added.
- The initial c_tracefunc/c_profilefunc concern was withdrawn after checking
  swap_trace_func_arg and swap_profile_func_arg: each maintains nonnull equality
  with its value callback, so the existing checks cover both interfaces.
- Shared continuation use and complete runtime discovery/scope checks were
  requested. Focused observer, attribution, fallback and descriptor negatives
  remain under preparation.

No current emitted owner/source sequencing bug was found. Approval remains
pending final source, generated/native release review and executed focused
evidence. Exact-kind preservation at nomination is being assessed with the
coordinator; this first-pass report does not claim a final implementation.

## Exact-kind nomination review

Moving exact Python-function kind to nomination is sound under the live
descriptor identity/type-version proof. Method nomination reads its still-owned
input; CALL nomination reads captured self. Each must validate current receiver
type/version and inline validity before reading the guarded payload. Per-hit
existing guards then preserve the immutable function-kind fact without another
kind load/check. No new payload is needed.

The first implementation had a concrete NULL-dereference hazard: the receiver
test used IsPointerSpace, which accepts NULL. CALL's captured self may be NULL
after a shadowed/nonmethod lookup while the preceding method specialization is
still installed. Reject NULL (or require IsObject) before reading Py_TYPE.
The worker and coordinator were notified immediately; a warmed shadow/CALL
re-specialization negative is required. Source approval remains pending this
fix and final executed evidence.

The receiver test now uses IsObject before dereference. The strengthened
regression starts with an actual builtin descriptor/base method handler, then
shadows it with a standalone Python lambda and allows CALL re-specialization.
This exercises NULL self rather than relying on the already-refined handler's
earlier decline. The NULL finding is resolved in source.

The final source tests also cover real refined-path traceback attribution using
a closure flag (without invalidating type/layout), monitoring both original
instruction offsets, temporary receiver ownership, dictionary replacement,
code-arity fallback and bounded recursion. Runtime tests explicitly require
GIL and decline DTrace builds; direct-file discovery guards are at file ends.
The worker reports 113 final host generator tests pass. Executed native runtime
results remain pending.

Regeneration exposed one remaining source correction: PushExact must join
PushUnchecked/PushOverlapping in NON_ESCAPING_FUNCTIONS. The audited helper has
no callout, but the missing declaration adds a spurious ESCAPES effect to
ordinary exact CALL. Coordinator/worker were notified before native admission.
The IR_ENTER frame marker also makes its callers unavailable to tier2 macro
expansion; this needs an explicit scope decision because native JIT tiers are
unfinished and this experiment measures JIT-disabled execution.

## Final source and regeneration pass

PushExact is now declared nonescaping. The frame-entry marker retains sync/frame
effects and drops the inapplicable caller-PC dependency: it only loads the
new callee IP and never needs this_instr. Regenerated ordinary exact CALL
metadata returns to its A50 contract, and opcode macro-expansion metadata has
no remaining diff. The spurious ordinary escape effect and macro-expansion
loss are resolved. The duplicate PEP523 debug assertion was also removed.

The final generated refinement retains two live words/source identities,
preserves exact kind through nomination and uses separate entry/hit counters:
the refinement's hit increments only after successful admission, immediately
before its infallible frame commit. Neither counter is a quiet timing input.
The new handler maps to LOAD_ATTR and its canonical method payload/history;
only its conservative execution frame-entry effects differ from the base.

**Actual source and generated output pass this review for native admission.**
There is no remaining identified source blocker. This approval is conditional
on the coordinator's real release/debug compilation, native ordinary-CALL and
refinement inspection, and executed focused tests. No performance or landing
claim follows from source approval.

Coordinator committed the reviewed candidate as `b576289a8eb`. Source/generated
approval applies to that candidate; release/debug compilation and native/runtime
qualification are in progress. No review finding remains open in source.

## Release compile/native pass

Coordinator release/debug builds of `b576289a8eb` succeed with 117 checked
modules each. Reviewed `native50c` disassembly against A50. Ordinary exact CALL
remains 1,410 bytes and 339 instruction lines; after normalizing addresses, the
only differences swap two spill-slot register assignments around an existing
memset. No added ordinary hot guard or out-of-line helper boundary appears.

The refined handler is 1,396 bytes; ordinary canonical method remains 249.
The success path omits function-home publication/reload and the Python-function
kind comparison. It retains self's argument-home store, the function increment
or receiver transfer/duplicate, independent code ownership and ordinary frame
initialization/continuation. The compiler retains a cold generator alternative
despite the entry thread guard; this is a possible later static-helper
simplification, not a correctness defect. Existing value-API fatal object checks
also remain; this review does not claim every representation check disappears.

Initial release focus passed 14 of 15 files, including the new runtime cases,
and failed one contract assertion that compared an execution consumer's effects
only with its semantic base. The worker added a validated handler_runs_code
query and explicit independent assertions for refined entry versus plain method.
The fix retains the contract check rather than exempting a handler name. Its
source passes review; final committed identity and release/debug rerun evidence
remain pending. No performance-admission approval is made yet.

The contract-query correction is committed as `3e96bd9311e`, on top of C source
`b576289a8eb`. Its generator/test source passes review and leaves native C and
generated runtime behavior unchanged. Coordinator final identity builds and
focused reruns are in progress. Source/native approval applies to this combined
candidate; final runtime admission remains pending the rerun.

## Closed experiment

The coordinator's final release/debug reruns passed 1,939 tests each. The
direct-overlap follow-up passed 607 focused tests in each role, including the
corrected source-free fixture, and reduced native size to 1,052 bytes. These
component checks did not establish performance admission: the final C/A
comparison loses on DeltaBlue and logging. C is rejected and retained only as
experimental evidence. [Performance review](C50-performance-debug-review.md)
and [selected-source review](final-review50.md) own the final dispositions.

## Final focused runtime evidence

Reviewed coordinator `delta50c3` focused logs for `3e96bd9311e`: release and
debug both pass all 15 files and 1,939 tests, with 19/17 skips respectively.
The contract-query correction passes alongside the actual runtime ownership,
fallback, observation and attribution cases. **Component source/native/focused
runtime review is complete; C50 is approved for performance admission.** This
does not claim a measured C/A improvement or full-suite/batch qualification.

The initial `measure50c-call-pair48` run overlapped debug generator testing.
Exclude it from admission and causal comparisons. Clean matched measurement,
including frozen A as the direct control for C's contribution, remains required.
