# Combined R2 + validated S2: final source peer review

Source GO at `frame54-combined@f3c464a54c8` for coordinator regeneration,
build and qualification. No new must-fix source defect found. This review read
the actual final handwritten and checked-in generated code against golden51,
R2 `fcc82220ddd` and validated S2 `e95328ba1d6`. No runtime, generator,
compiler, inferior, test, profiler or timing ran. No runtime source changed.

The candidate combines recipient proofs with the revised integer overwrite;
it does not remove feedback, namespace or locals fields. Its unchanged source
layout has the 112-byte GIL header; matching combined ELF verification remains
the coordinator's responsibility. Compact F+N+L conclusions do not apply to
this candidate's storage contract.

## Recipient state and generated transfer seams

NONE is exactly UINT16_MAX. Ordinary initialization and the thread base store
it as a constant. Ready validation still excludes NONE, tagged discard and
reserved bits; index and replacement accessors remain ready-only. IsDiscard
permits NONE in its debug precondition but returns false with an unchanged
release equality. SetReturnValue completes pending redirect, then asserts a
ready descriptor before discard/publication
([pycore_interpframe.h:164](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Include/internal/pycore_interpframe.h#L164),
[pycore_interpframe.h:246](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Include/internal/pycore_interpframe.h#L246)).
There is no release NONE guard or silent discard fallback.

CALL, native transaction, SEND/FOR, YIELD, ordinary generator creation and
trampoline producers still install actual descriptors before their transfers.
The evaluator entry still has real empty slot1. Copies preserve NONE or an
installed descriptor without interpreting it. The pending debugger recipe's
discard gate therefore remains false for a fresh activation
([frameobject.c:3516](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Objects/frameobject.c#L3516));
remote asyncio still rejects NONE before numeric decoding. NONE does not repair
the provenance of a stale real descriptor, and this candidate claims no such
redesign.

The analyzer's nonescaping certification names only the pure recording helper
([analyzer.py:570](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Tools/cases_generator/analyzer.py#L570)).
That helper reads the existing execution prefix and writes the encoded field;
it does not allocate, close an owner or switch frames. Result publication,
which can close displaced owners, is not made pure. The generator's canonical
frame-transfer body recognizes the new setter, and generated call/yield code
contains it without adding a wrapped-frame result publication. The checked-in
generated changes are the expected recipient setter substitutions plus S's
new handler. Regeneration must establish reproducibility.

## Same-home certificate and interaction with NONE

The shared geometry predicate still proves A==B, C!=B, B scratch, and both
inputs within the actual home extent and below the read/discard sentinel.
Common constructor validation and specialization use that same predicate
([codeobject.c:1390](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Objects/codeobject.c#L1390),
[specialize.c:640](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Python/specialize.c#L640)).
Neither uses return_slot. A fresh NONE descriptor is valid activation state
while this arithmetic executes; its return goes to the caller's real descriptor.
Monitoring callbacks may observe that activation without authorizing pending
child publication into it.

The generated refined handler checks both inline types before fallible result
production; failure retains PRE homes. On success it adopts inline B without
clearing it, consumes C by the normal rule, performs only count-free inline
closes and immediately overwrites the same B expression with the canonical
result ([generated_ir_cases.c.h:233](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Python/generated_ir_cases.c.h#L233)).
Its geometry check remains debug-only. The generator's exact-body certificate
and negative cases preserve the nonescaping commit boundary. Polymorphic
feedback canonicalizes the variant; immutable directly supplied variants are
constructor-validated before instrumentation can restore them.

The integrated constructor tests retain malformed geometry, changed frame
metadata, marshal ingress, valid boxed/deoptimized execution and monitoring
enable/disable/restoration. These tests exercise real constructor paths; their
presence is source evidence, not a combined runtime pass.

## Cold public construction

The shared public constructor allocates and computes fallible coroutine origin
before moving frame ownership. Failure frees only the untracked allocation and
consumes the stolen input frame. Under the frame lock it installs actual discard,
copies the owning frame, acquires the frame-object owner, establishes generator
ownership and initializes exception, weakref, origin/finalizer, async flags and
name owners. Only then does it associate f with the destination and GC-track the
generator; it unlocks before releasing the stolen input reference
([genobject.c:1227](https://github.com/rjpower/cpython/blob/f3c464a54c89783e50af1ce9f3cffec6f5e7279d/Objects/genobject.c#L1227)).
The generator is untracked and unavailable through f during its earlier CREATED
assignment. This differs from ordinary make_gen's tracked cleared object;
do not transfer that earlier state-store ordering to the ordinary path.

The direct f_locals owner moves with the frame. There is no optional mapping
record or L copy helper. PyCoro_New and PyAsyncGen_New return directly from the
common constructor, preserving one initialization authority and avoiding the
earlier duplicate-origin leak. First send uses producer-established discard;
NONE is never treated as the first-input contract. The retained tests cover
frame identity, mapping lifetime through escape, first input, completion/close,
coroutine origins and async-generator hooks/resume.

## Qualification and selection

The supplied R2 release/debug, remote-reader and failpoint passes are component
evidence. Root must regenerate/build this exact combined tree and run combined
recipient/redirect/C-API/ownership, generator, constructor/marshal, monitoring
and generator-proof checks, followed by native inspection and direct balanced
performance comparisons. No combined runtime or free-threaded qualification
is established by this source review.

Assess measured breadth under the user's updated allowance for an increment
that is mostly better. The earlier R coroutine regression remains material
evidence to report; it is neither a prediction for R2+S2 nor an automatic
single-case veto. Keep per-workload ratios and variability visible alongside
aggregate results. Do not multiply component ratios or infer hardware causes
from text size. Source GO is not performance admission or golden promotion.
