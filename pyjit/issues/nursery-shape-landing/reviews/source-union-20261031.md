# Nursery/shape union source review

Reviewed union `e9e10d5b503eaf219797a5db44766eeab6fad4dc` against base
`71ee990b802c3888ff7cf885f0c349d3c8006563`: thirteen changed files, composing
nursery `3a473612157` and shape `c46f1bef67f`. Inspected the real diff, adjacent
allocation/tracing/lifecycle code, component contracts/results and the retained
negative. No native command or repeated broad hash audit ran during this review.

No supported-path source defect found. Source is clear for conditional landing
acceptance after the planned broad qualification, closure and final evidence
review. This review supplies no missing execution or cleanup gate.

## Nursery

The age bit is separate from TRACKED and scratch/lifecycle flags. Existing
`new_reference()` clears collector flags on block reuse; exact cell and nonempty
base-tuple allocation alone opt in. Shape-header writes preserve flags.
Promotion changes no address or ownership and retains TRACKED. `young_pending`
survives disabling, so a later successful main-interpreter major retires age.
The never-enabled path avoids the added promotion census.

The minor seeds all allocated old objects before ordinary root closure, then
uses the existing trace/dead inventory and lifecycle engine. This deliberately
retains unreachable old owners and their young referents until a major.
Unpublished old tuples take the SHALLOW path until TRACKED, so the all-old
census does not interpret partial tuple fields as initialized values. Artificial
minor retention does not clear DEL_CALLED or non-GC FINALIZED state.

Mark-stack/dead-inventory failures retain objects through the existing failure
walk and DEAD rollback. Promotion runs only after successful tracing and uses
a nonallocating census; dead candidates retain age if later lifecycle
preparation fails. The pending hint may conservatively remain set until another
successful census. No new fallible reclamation or bulk-free path is introduced.
This is source reasoning, not a claim of exhaustive minor OOM fault injection.

The minor request is passed explicitly through collection and any second pass.
Automatic/full/shutdown paths remain major. Minor callbacks and statistics use
generation zero; minors preserve major allocation debt and threshold.
Experimental object listing avoids an implicit major and filters GC-capable,
tracked objects before age selection. The private setter rejects collection,
sweep and queue-delivery reentrancy; admission and collection reject unsupported
verifier/stress profiles. Default mode remains disabled.

The retained old-owner negative changes the request into a major and fails at
the nominated retention assertion before the following field access, while its
`finally` completes. The five controls cover payloads/promotion/mutation, young
garbage, old-boundary retention/eventual major, debt/statistics and callbacks.
Their scope does not establish dominant allocation coverage or reduced old work.

## Shape admission

Both polymorphic bodies reject inline receivers before borrowing, then find one
row by the unchanged live shape/version matcher. A miss precedes payload
access, hit accounting, writes and input consumption; STORE kind rejection also
precedes mutation. LOAD preserves data versus descriptor/self layout, and STORE
preserves slot-write-before-shape-publication order. No row/table pointer spans
a call or safepoint. Moving STORE's value read and LOAD's output-pointer formation
earlier has no side effect on deoptimization.

The generated handlers match these semantics. Actual IR formats, feedback and
instruction boundaries are unchanged; the deleted private guard reduces the
uop enum by one. Consumer search found no remaining `_GUARD_SHAPE_POLY` use in
Python, Include or Tools. Diff whitespace checking passed. Component evidence
supports removed emitted comparisons, not an elapsed speedup.

## Support boundary checked

Initial inspection raised a possible age-hint mismatch if a legacy subinterpreter
initiated a major over the shared main domain. Rechecking the actual lifecycle
rejected that scenario: both public constructors enter `new_interpreter()`,
which rejects creation while the main interpreter exists, before legacy/config
setup. Nursery admission also rejects nonmain interpreters. The scenario is
unreachable under the documented one-main/GIL boundary and is withdrawn as a
landing finding. Foreign lifecycle expansion would need a fresh domain/age
review; this candidate does not expand that boundary.
