# Canonical sys.flags construction and copies

- Stage: independent review accepted; committed as `015eb226fb39fbe4fb3940a1d9037053a9b41d6c`
- Owner: state_audit implemented; coordinator reviewed design/committed; wiki_audit reviewed code/results
- Scope: two Python/sysmodule.c helpers and Lib/test/test_pyvalue_sysflags.py

Fresh structseq slots receive canonical owners; unchanged visible and hidden
fields copy without boxing. The old snapshot stays owned through publication;
cleanup preserves primary errors. The ABI block and other functions are exact.

Full-TU syntax passed and the new build produced Python/sysmodule.o. Applicable
host value/error/reentry controls passed. Four host snapshot/identity assertions
failed because host3.14 mutates flags in place while preexisting W6 replaces
them; those failures remain preserved. All five candidate cases remain
unexecuted; source inspection does not prove dynamic snapshot/OOM behavior.

[Design](../../../../../build/pyvalue-sysflags-consumers-20261102-v1/design.md),
[results](../../../../../build/pyvalue-sysflags-consumers-20261102-v1/results.json),
[review](../../../../../build/pyvalue-sysflags-consumers-20261102-v1/independent-review.md).
Next: execute candidate snapshot/reentry tests after linking.
