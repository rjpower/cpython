# Stage-one process-accounting source review

Reviewed `process-accounting-stage1-current-main-357.patch` against the current-main `357baeea402` reader and worker interfaces. The patch preserves raw structured argv boundaries and replaces the old payload search with explicit executable identity, but is held for an additive correction:

1. The reader drops `pid` from uncounted spawn rows. `suite_violations()` requires an integer `pid`, so every uncounted launch is permanently marked incomplete even after an exact rule is approved.
2. Counted spawn rows are accepted without first validating the producer's tag, pid, event, executable and argv field types. They must be syntactically complete even though stage three, rather than audit events, will establish child coverage.
3. `_write_record()` does not reject a short `os.write`. A partial ledger can be reported as a successful hook call.
4. The reader decodes spawn ledgers with replacement characters and accepts an existing empty file. Both conditions must leave accounting errors.
5. `realpath(relative_executable)` uses the recorder's cwd, not `Popen(cwd=...)` or the child's PATH. Trust an exact absolute build product or record an ambiguous relative launch as opaque; do not silently treat it as a nonproduct.

The existing exact-rule manifest remains empty. These findings require focused negative fixtures and an additive patch; the original candidate stays frozen. Native post-success pid receipts, once implemented, will be the primary child-coverage evidence, while the audit ledger remains diagnostic.
