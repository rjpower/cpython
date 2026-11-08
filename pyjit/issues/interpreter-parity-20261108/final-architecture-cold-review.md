# Final cold architecture review: interpreter parity batch

2026-11-08. Reviewed the four committed increments on qualified R5
`0953757f4e7` through `10627c339fa`, their generated handlers and metadata,
the currently edited canonical design pages, and the existing focused reviews.
The runtime's three modified stats files are a temporary diagnostic overlay,
outside this review. This is a source and design review; combined qualification
and performance admission remain with the coordinator.

## Verdict and required correction

**No must-fix runtime correctness or architecture defect found.** The batch
keeps one register-word layout, one ownership model, and one generic fallback
for unproved cases. The narrow generated edges are justified by local proofs;
they do not assert purity for their whole handlers. The code changes are
coherent with the intended interpreter architecture.

**Correct one canonical design statement before closing documentation.**
`pyjit/design/call-and-frame-lifetimes.md:53-55` says M1 retires inputs
"before publishing the result." The generated
`BINARY_OP_SUBSCR_DICT` hit path calls `IR_STORE_CANONICAL_TEMP_DST` **before**
ordered key/dictionary retirement. A temporary destination owns the result
before a possible destructor callback; a folded local keeps its old home and
receives the result only after input retirement. State this two-phase rule
explicitly. The current source and M1 contract follow it, so this is a
documentation correction, not a code blocker.

## Architecture assessment

- P1/P2's `_PyFrame_CodeHome` uses the pointer representation only for a typed
  code object, duplicates its owner, and leaves generic numeric canonicalization
  in `PyValue_FromObjectBorrow`. Successful refresh returns before redundant
  failure revalidation; the error path retains that check. The pointer helper's
  canonicality premise should remain explicit at every future caller.
- M2b executes one live ordinary NOP after the checked exact-bool branch. It
  retains the word and PC publication, declines wrapped targets and debug
  lltrace, and keeps free-threaded execution on the old route. Its generator
  checks terminal placement, shape, guard and nonescaping prefix. Ten fewer
  entered handlers per observed loop are an accounting result, not a timing
  claim.
- M1's provider accepts an exact GIL dict, inline-int key, combined general
  table and first-probe identity hit. The hit's owned result is published to a
  temporary before ordered owner retirement; a local is written after it.
  Decline preserves the original lookup and its callbacks. The emitter reuses
  shared retirement and reload logic; the handler's conservative whole-path
  effect metadata remains appropriate.
- P3 confines its preallocation proof to guarded `CALL_PY_GENERAL`. The private
  runner shares the transaction initializer and overlap/consuming/native
  routes. Allocation can invalidate callable facts, so the existing final
  callable, code, provider, hook, default, capacity and offset checks still
  run. The generated fallback remains the generic runner. No second call
  ownership protocol was introduced.

The generator's marker validators depend partly on trusted named uops and
source-text shapes. That is a reasonable bounded contract for these three
producers, with negative generator tests; it is not a general semantic proof.
When changing any trusted guard, branch or provider body, review its consumers
and update the associated validator/tests. Do not generalize the markers to
unrelated handlers merely because their stack shape matches.

## Legacy expansion and next decisions

`_PyOpcode_macro_expansion` has no runtime consumer in this checkout: repository
source references are its generator, declaration and definition. R5 contained
entries for `POP_JUMP_IF_FALSE_BOOL` and `BINARY_OP_SUBSCR_DICT`; the new
interpreter-only NOP suffix and dictionary marker make their expansions
ineligible. `CALL_PY_GENERAL` already had no entry at R5. The live register-IR
format and handler metadata retain all three opcodes. This is no current JIT
or interpreter defect, but a future frontend must treat a missing expansion
as unsupported and provide explicit semantics for these operations before it
uses that table. The optimizing method/tracing design should lower them into
its own effect and recovery model; copying these interpreter terminal edges
into SSA would conflate distinct contracts.

Next, finish the combined release/debug qualification and clean-source target
and control measurements. Use the route and first-probe diagnostics to explain
coverage, and judge keep decisions against observed elapsed results and
regressions. The current source proves removal of named work, not its time
value. Investigate a remaining hot route only after those results identify it.
An encoding, dispatch-backend or frame-layout rewrite has no demonstrated
target benefit in this packet and would expand the verification surface far
beyond the four bounded changes.
