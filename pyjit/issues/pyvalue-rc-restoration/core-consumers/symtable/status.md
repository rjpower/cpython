# Canonical symtable metadata consumers

- Stage: independent review accepted; committed as `23cec84d2823b73d65aec8a7cc6344adbec27ecd`
- Owner: state_audit; coordinator accepted design; wiki_audit reviews implementation
- Scope: Python/symtable.c and new Lib/test/test_pyvalue_symtable.py

All six dictionary loops use owning canonical entries. Bounded flags are read
and written inline; error and continue paths close selected owners. Directive
coordinates are read without clearing a pending SyntaxError.

One actual full translation-unit syntax check passed. Seven focused semantic
cases passed on host Python 3.13 and 3.14 as reference controls; they have not
executed on the restored runtime. Earlier host fixture failures remain in the
evidence with their corrections explained. Review caught a declared-only dictionary
getter; the corrected port computes a hash and uses the implemented private
owning KnownHash provider. Renewed actual syntax passed.

[Design and evidence](../../../../../build/pyvalue-symtable-consumers-20261102-v1/review-notes.md).
[Independent review](../../../../../build/pyvalue-symtable-consumers-20261102-v1/independent-review.md).
Next: merged-candidate execution; the source checkpoint remains non-runnable.
