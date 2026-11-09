# GenericAlias tuple readers and substitution

Reviewed dependent checkpoint49 is `7a6cba14ed0`: eight functions and one
public test module. Owning fallible views replace generic borrowed item reads;
the two tuple-extension callers share an object-owned tuple interface. Canonical
value storage is never cast to an object-pointer array.

The full translation unit and actual object compile with zero diagnostics,
down from15 errors/two warnings. Independent review binds exact function and
outside-source scope, owner/error flow, helper/caller composition and native
results. The coordinator integrated only the reviewed two paths.

Actual unittest discovery on reference3.14.4 finds11 tests:10 pass and one
stale-parameter diagnostic skips on Python<3.16. The restored3.16 candidate must
execute it. The earlier reference probe crashed; that raw evidence and failed
prototypes are retained. Candidate behavior, allocation/error fault coverage
and the first runtime link remain pending.

The worker's parameter/local name collision was corrected by the exact reviewed
two-name change. One helper error check also required correction. Original
packets and reviews remain; repaired overlapping review receipts use distinct
names and retain all original bytes.

Evidence: [design and literal recipes](../../../../../build/pyvalue-genericalias-readers-design-20261102-v1/README.md),
[full C/native review](../../../../../build/pyvalue-genericalias-readers-composed-20261103-v2/independent-solshared-composed-code-review.md),
[final two-path admission](../../../../../build/pyvalue-genericalias-readers-composed-20261103-v2/independent-solshared-composed-review.md)
and [commit receipt](../../../../../build/pyvalue-genericalias-readers-composed-20261103-v2/commit.json).
