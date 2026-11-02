# Dictionary construction, assignment and VM publication

The coordinator adopts D's independent critical review of the concrete proposal
in build/pyvalue-vm-dict-consumer-design-20261102-v1/proposal.md,
SHA2562d5a89fa169b81afd086f693db8a3389a8cb08e3b4afd6795a5f1ff0285179f2,
and its window proof75f1dc8d048d62d551635035cb95dd8da702869167f7b2e0c11cc6d5afa193af.
A implements; D reviews actual emission and callback/error/owner paths.

Distinct75-minute source cut from conservative2026-11-02 00:32:08 UTC,
checkpoint00:57:08, ceiling01:47:08. Record actual first edit. Accepted kernel
and common-constant windows are closed, not extended.

## Source ownership

A owns only BUILD_MAP, _STORE_SUBSCR, _STORE_SUBSCR_DICT and its macro,
_STORE_ATTR_WITH_HINT and its macro in bytecodes.c; _Py_BuildMap_ValueSteal
and the bounded pre-provider PC capture/post-provider refresh helpers in ceval.c;
selected generator publication/refresh/capacity/error-metadata support and
corresponding semantic-helper Action support when required. A remains sole
generator writer and owns affected emitted-source tests and actual products.
Add one dictionary storage inspection fixture/registration in _testinternalcapi.c
and new test_pyvalue_dict_vm.py. Necessary declarations/includes and exact
analyzer intrinsic metadata belong to this cut; record them for review.

C owns the provider addition _PyDict_FromValueItems and its internal declaration
under C's existing mutation/growth deadline01:34:24. This exact additive handoff
is authorized: canonical strided borrowed arrays, caller-guaranteed stable owning
evaluated inputs; presize and insert through the accepted canonical provider,
never cast to object arrays. Legacy _PyDict_FromItems remains unchanged here.
C must bind the helper's real presizing/allocation/cleanup dependencies and
publish it before A's final consumer freeze. No parallel insertion engine.

## Required semantics

Ordinary compiler-produced BUILD_MAP windows are consecutive private temporaries
and cannot name locals (regcfg compaction/verifier proof). OwnArray before the
first allocation makes these evaluated roots stable. Raw constructor BASE bounds
alone do not prove a full structural window; preserve the existing unsupported
malformed-IR limitation explicitly instead of silently tightening public checks
or claiming that proof covers arbitrary injected IR.

Capture scalar current/next PCs BEFORE provider/boxing callbacks; promote inputs
and publish/detach real homes before invoking the provider exactly once. Never
subtract a possibly stale post-callback pointer or decode cached fields before
forced validation/reload. Refresh after provider and cleanup even with Temps0.
New folded-local assignment happens at its semantic position; failure retains
the old local and uses actual consumed-input/temporary-result liveness.

Preserve each site's actual ownership order. Generic _STORE_SUBSCR borrows into
the operation and retires originals subscript→container→value; its dictionary
branch uses borrowed SetValue, not an early Take2 transfer. The already-consuming
specialized dictionary site retains Take2 then dictionary retirement. Unwatched
hinted store publishes the field then closes oldfield→receiver. Watched store
uses checked pre-notifying/re-probing provider and settles value→watched_dict→
receiver. Prove the finite3-owner capacity rather than introduce a general
delayed-owner queue. Continue cleanup after a checked failure and preserve the
primary error, never replay a committed operation or deopt BEFORE its effect.
Deallocators preserve entry exceptions; their failures are unraisable.

Keep existing guards, IDs and RegIR formats. Managed instance helpers, generic
attribute protocols, dictionary reads and Clinic/parser migration remain explicit
required later cuts. No implicit shared-file or API-removal scope.

## Gates

At the checkpoint inspect real BUILD_MAP and cold/warm assignment emissions,
saved-PC timing, pre-call NULL writes, exact close order and error destinations.
Add legal finalizer/profile/unraisable and alias tests, asserting actual handler
selection. Serialize required generation; affected host tests and at most three
actual changed syntax-only TUs30seconds each. Coordinate C's provider diagnostics
to avoid duplicate checks. No object/link/candidate execution or performance
measurement. Final source acceptance remains separate from runtime qualification.
