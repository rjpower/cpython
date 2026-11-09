# Full R27 census for `6a0e4c12c17`

The full release suite passed with 54,351 tests, zero failures, and 2,531
skips. The full debug suite also passed with 54,351 tests, zero failures, and
2,469 skips. The matched-build preflight passed its identity checks, but it is
diagnostic-only and ineligible as a performance result.

The strict R27 census failed. The retained artifacts are under
`build/pyjit-link-wave1/checkpoint-closure/suite-6a0e4c12c17-census/` in the
runtime build tree: `summary.json`, `debt-totals.json`, `run.log`, and `dumps/`.
The summary reports:

| Measure | Result |
| --- | ---: |
| Stats dumps | 6,512 |
| Core/core-pending/unresolved debt boxes | 11,028,818 |
| Core-attributed debt | 11,028,795 |
| Core-pending debt | 23 |
| Invalid owners | 0 |
| Dropped boxes | 0 |
| Unfinished `.run` markers | 539 |
| Identity-matched dumps | 6,510 / 6,512 |
| Identity errors | 1,233 |
| Accounting errors | 1,774 |
| Predicted-uncounted audit events | 437 |
| Expected `test_sys` resets | 2 |

The 437 entries are predicted-uncounted events, not a count of unique child
processes. The two missing dump identities and marker errors remain
unresolved. The `.done` inventory includes 251 orphan markers: 247 tagged
`test_pyvalue_box_sites` and 4 tagged `test_value_slots`. They may be connected
to fixtures deleting their own `.txt` files, but that is unproven; they are not
allowlisted.

The stats regrtest exited 2 with one `test_subprocess` error. Preserve that
failure. A fresh standalone stats run of `test_subprocess` passed 371 tests,
skipped 48, and completed in 22.8 seconds; that pass does not erase the full
census run's failure.

The static scan also failed with 11 findings in
`build/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/static-r27.log`:
one ownership-inventory completion finding and ten missing `Py_VALUE_COMPAT`
declaration marks. These marks cover optional attribute/special lookup,
mapping optional item, iterator next/send, and object call/vectorcall entry
points. Audit production callers before changing marks or source; do not make
the scanner green by assertion alone.

The kernel result is recorded separately in
[the capture receipt](kernels-6a0e4c12c17.md): strict gate failed despite
18/29 startup-inclusive raw-zero kernels and 29/29 zero loop deltas. No elapsed
performance result has been collected. Any next benchmark run is diagnostic;
performance qualification remains blocked on passing R27 evidence.
