# Advanced integer formatting: implementation receipt

The proposed value writer is frozen in [the implementation patch](r27-long-format-values-draft.diff), with [separate regression tests](r27-long-format-values-tests.diff). It carries exact inline integers through the existing decimal and binary formatting kernels without a transient object projection; object and subclass paths retain their existing behavior. The advanced object writer remains as the object API entry, with a narrow value alias. The broader integer writer is not marked as a compatibility entry.

Independent source review and coordinator review found no must-fix issue. The changed `longobject.c`, `unicode_formatter.c`, and `unicodeobject.c` translation units each passed release-configuration syntax checks with the scratch headers. The exact new regression test ran against upstream: one test passed. The patch passed an apply check against the then-current candidate source.

This packet is queued, not committed or admitted. It has not been tested on a patched runtime. Integration must run the focused formatting tests and obtain fresh merged statistics before making a performance or boxing claim. See the [design proposal](long-advanced-format-values-proposal.md) for the contract and remaining checks.
