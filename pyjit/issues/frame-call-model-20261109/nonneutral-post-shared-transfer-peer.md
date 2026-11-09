# Shared physical transfer for the next stopped-state packet

Disposition: the proposal has a sound bounded direction if its physical
facts remain separate from owner provenance and its first gate proves only
the four pinned witnesses. This is a source and design review, not runtime
admission.

Use one cold, per-scratch-home query for a pristine word, actual argument,
normal edge, frame bounds and home. Its result is `PRESERVE`,
`SET_NULL`, `UNKNOWN`, or `TRANSPORT(source_home)`. A caller applies
all `TRANSPORT` results from the **old** row, then the other writes.
`UNKNOWN` means only that must-NULL knowledge is lost; it is not a
fresh-owner definition. Keep old-origin cuts and definite output
definitions in existing `_PyIR_Outcome`, `_PyIR_DefinesHome` and
`_PyIR_ObservedHomes.old_cut`. An unsupported normal effect may safely
forget all scratch facts; it must never default to preservation.

The minimum exact cases have direct source witnesses:

* Generated `COPY` reads `window[0]` and publishes its duplicate at
  `window[oparg]`: transport from B to B+oparg. Generated `SWAP`
  exchanges B and B+oparg-1. Require valid bounds/arguments; untouched
  middle holes preserve. Neither is a fresh old-origin definition.
* Generated `STORE_FAST` moves input A to the named local and sets
  `IR_SLOT(A)` to NULL before closing the displaced local. Its scratch
  A has definite `SET_NULL`. Other generic input reads/outputs can
  conservatively become `UNKNOWN` until the emitter proves their exact
  success clearing.
* `_PyEval_CompleteStep` clears FOR exhaustion's B and B+1 to NULL
  (`Python/ceval.c:3015`). FOR body B+1 is a nullable cursor and B+2
  is a produced item; neither is definitely NULL unless the actual
  destination move/discard clears B+2. Use the semantic edge, not
  `fmt.through`, for this distinction.
* A `WINDOW_DST_FIRST` output moved to a named local or discarded
  leaves its *temporary* B+`fmt.dst` NULL. With a temporary destination
  A=B+`fmt.dst`, it is an unknown produced value instead. The
  existing `_PyIR_DefinesHome` deliberately excludes the moved temp,
  so a must-NULL transfer must account for that physical clear itself.
  Other output kinds may mark their exact output homes `UNKNOWN`;
  opaque child overlap forgets its scratch tail.

For an ordinary PRE error, apply no normal success transfer. Forget
must-NULL on every home the failing handler might have touched before
unwind: explicit input/output fields, its bounded window and semantic
outputs; an unbounded activation effect forgets the whole affected tail
or all scratch. Exception and lasti payload homes then become
`UNKNOWN`. An unproved source hook likewise loses knowledge over its
affected homes. A certified source POST error starts from the selected
normal committed row, with callback-side changes handled under its
separate phase/generation proof. This error rule cannot claim a
successful output definition or erase an ordinary PRE owner need.

For NULL-only target reads, require both forward must-NULL at the
original encoded read on every reaching path and
`_PyIR_NullableHome` on the actual executable handler after
de-instrumentation. Merge aliases with VALUE taking precedence.
At application, also require the stopped-view must-NULL fact and the
actual current cell to be NULL. Named-local binding proof and active
child overlap stay separate. The proposed first rung must reject
`PUSH_EXC_INFO` and actual `LOCAL_MOVE` token creation before its
request-owned context-row transfer; otherwise the existing transfer
would mutate shared `Analysis.parents` or save parents.

This interface supports a forward must-NULL worklist and the existing
backward owner walk without a new opcode admission table or second
unwind engine. It does not by itself authorize SOURCE_POST, suspended
frames, a caught-link exit, or a NULL optional receiver where the
original path expected a real method owner.
