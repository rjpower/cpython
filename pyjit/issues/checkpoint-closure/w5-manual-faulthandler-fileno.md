# faulthandler `fileno()` value-call follow-up

This incremental patch applies after [`w5-manual-time-random-faulthandler.patch`](w5-manual-time-random-faulthandler.patch). It keeps the `fileno()` result owned as a `PyValue` through the integer check and conversion, then closes it on the common success path. The receiver remains an object because it may come from `sys.stderr` or an intentional float-to-object fallback.

Validation: the S faulthandler unit compiles. A W5 shadow build imported 117 modules with no failures; `test_faulthandler` passed (51 tests run, 6 skipped). The original W5 file was restored and the release tree rebuilt afterward.
