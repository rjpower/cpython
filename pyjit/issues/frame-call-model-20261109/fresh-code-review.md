# Cold source review: frame and call model

2026-11-09. Independent read-only review of the uncommitted runtime diff against
`4b3d739d867b`, branch `pyjit/frame-call-model-20261109`, in
`build/worktrees/execution-boundaries-20261108`. Source changed during review;
the coordinator and B worker were told of findings immediately. No runtime
files were changed, native builds started, or native tests run by this reviewer.

## Assessment

The frame/binder portion is suitable for merged native validation after the
generator guard below is tightened. I found no present incorrect ownership
transfer in the guarded binder, lazy kwargs publication, canonical return, or
window/local transfers. This is not admission of the merged implementation:
B's compiler/observer work and generated products were still incomplete during
review. No throughput claim follows from this review.

The model removes meaningful work from the general Python-call hit: it avoids
the generic transaction record, repeated shape/default discovery, and eager
empty kwargs allocation for eligible functions. It keeps one owning overlap
ABI and an independently owned executing code object. The return recipient is
an ordinary slot or discard, and the continuation is a real executable pointer.
These are coherent changes. They do not remove argument-home duplication before
the call or the eventual child-local cleanup, and should not be credited with
those savings. New frame and feedback fields and their initialization are real
costs to include in compiled-path and matched-counter analysis.

## Findings

### P2: canonical-return guard does not fail closed on changed producers

`Tools/cases_generator/ir_generator.py:3868` (`canonical_return_input`, line
numbers can move during integration) checks substrings and counts of `temp`
and `res`, but does not constrain all uses or definitions of `retval`. Adding
an assignment to `retval` before `PyValue temp = retval` still satisfies the
guard. The instrumented prefix similarly allows extra operations on `held`
apart from a few literal token patterns. Its docstring and the canonical proof
claim a stronger contract than the check supplies.

The current DSL bodies in `Python/bytecodes.c:1477` and `:1510` preserve the
canonical input, so this is a generator proof weakness, not an observed corrupt
return. Tighten the checked value uses/definitions, or use an explicit
provenance contract. Add generator mutation cases that introduce a new write
to `retval` and a transforming use of `held`; they must select ordinary
publication or reject generation. Checking the current two handler names alone
would not fix the proof.

### P1 found and repaired during review: structural replacement retained lazy eligibility

At first inspection `code_replace_impl` passed `CO_VIRTUAL_KWARGS` unchanged
after changing the argument/local layout. The new
`test_structural_code_replace_cancels_eligibility` expected cancellation, but
the implementation could retain the flag or fail constructor validation
because a retained materialization barrier named the old slot.

B added structural-change cancellation at `Objects/codeobject.c:3481-3505`.
Source review confirms the fix clears eligibility before reconstruction.
Retained barriers are harmless with eager binding: `MATERIALIZE_KWARGS` permits
the sentinel pending state, and `_PyFrame_MaterializeKwargs` returns immediately
in that state. Native execution of the replacement test remains required.

## Ownership and guard checks

- The specialized prefix checks PEP 523, valid function version and recursion
  before the direct helper. Version invalidation covers supported code,
  defaults, closure and vectorcall setters. Scalar plans retain no weak object
  pointers, and defaults are read through the currently guarded function.
  Large legal default tuples decline the bounded plan rather than truncate.
- The hit changes no owners before its final commit. Function and argument
  owners move into the overlapping child; defaults acquire counted edges;
  code receives an independent home. Empty-dict allocation uses the existing
  no-Python-callback allocation assumption. Error before commit leaves caller
  window owners intact for ordinary error cleanup.
- Pending kwargs is canonical NULL and owns no GC edge. A semantic first-read
  barrier or frame-publication barrier installs a fresh dict before exposure.
  The compiler declines writes, deletion, movement/capture and suspended code.
  Existing-frame access, locals proxies and traceback exposure reach the frame
  object barrier; a saved exception is preserved on successful publication.
  Current remote frame consumers inspect positions/owners and generator slots,
  not ordinary kwargs locals; lazy binding excludes generators.
- Frame initialization, copy, entry trampoline, trampoline frames and the base
  thread frame initialize/copy the new state. Code-owned executable storage
  anchors `return_pc` as it already anchors `instr_ptr`.
- Return takes a separate C owner before child retirement. Canonical frame
  input can transfer unchanged; native/SEND bridges still publish normally.
  Discard releases that C owner directly. Tuple/list unpack provenance is
  distinct from generic iterator output, which retains publication.
- The old bool/discard aliases and POP decoder are removed from producers;
  stale generated IDs/cases remained awaiting coordinator regeneration. Root
  removed the unused `bool_boundary_edge` source helper during this review.
  Neither generated staleness nor B's unfinished observer integration is
  evidence of a newly discovered A ownership defect.

Python's lack of guaranteed prompt finalization was respected: no finding asks
to restore a historical decref location or virtual POP event.

## Validation required

Start with regeneration and a real release compile, then the new
`test_ir_frame_calls` and generator tests in release and debug. Add the
canonical-guard mutation cases above. Native focused coverage must include:

- Lazy kwargs unused/read/folded window paths, distinct identity, frame and
  ancestor exposure, profile/trace exposure, traceback locals, eager declines,
  defaults/code/provider mutation, and overlong defaults.
- Allocation failure at first materialization and frame publication, preserving
  one error and intact caller/child owners through unwind. Include cyclic GC
  and escaped frames while bindings are pending or materialized.
- Python/native discarded results and overwrites, callback re-entry during
  child cleanup/result disposal, and monitoring enabled from a callee or
  finalizer. Check the next real event without inventing a POP event.
- Generator/coroutine yield, send, return, throw and close through normal and
  interpreter-entry continuations; property/getitem/init frame shims; exceptions
  crossing those frame boundaries.
- B's compact-PC/source-edge exceptions, traceback lines, late observation and
  admitted/rejected debugger redirects, including code.replace/marshal paths.

Host-only checks: the full IR generator completed in memory and emitted exactly
two canonical-return stores. A subsequent optional substring search for a
handler heading failed; generation itself had completed. `git diff --check`
was clean at inspection. These checks establish no native result. After merged
focused validation, inspect release code and measure logging/spectral/unpack
before broader qualification and the prescribed parity campaigns.
