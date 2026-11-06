# Operator, factorial and process accounting integration

Base runtime: `812cba08f4b`. Coordinator owns the shared builds. Component
reviews approve source; the landing receipt determines whether the composition
is admitted.

Apply these packets in order:

1. `w5-operator-value-call-integration-v2.patch`: current-main diff to the two
   reviewed worker files, including the repr owner release and failure-lifetime
   regression. The original additive test patch does not apply cleanly to its
   frozen base; neither old artifact was overwritten. The integration packet
   also registers `_operator.c` as a legitimate paired-vectorcall creator in
   the existing lint inventory. No lint rule is weakened.
2. `r27-factorial-values.diff`: value-owned partial products and final shift,
   retaining the balanced product algorithm and public argument behavior.
3. `r27-accounting-exact-shapes.patch`, then normalization v2 and v3: preserve
   marker argument boundaries and raw filesystem bytes; admit only exact,
   bounded process identities. Executable normalization applies only to
   `argv[0]`; code and shell payloads stay exact. The new accounting rules file
   starts empty. Hardening the checker does not qualify existing subprocesses.

Sol approved operator and factorial source against the predecessor main; the
coordinator inspected the actual current-main composition and corrected the
operator artifact context before application. Accounting v2 had a remaining
payload-normalization bug and a byte-decoding mismatch; v3 fixes both and its
isolated checker suite passed 40 tests with one skip.

Merged validation is `landing.sh operator-factorial-accounting test_operator
test_math_integer test_functools test_pickle test_gc test_finalization`.
Generation, lint, all three 117-module builds, limited ABI and frozenmain have
passed. Release and debug integration tests each passed 5,768 tests. Sol's
post-review approved the actual composition. The runtime commit is
`af82c03c4e5`.

After this coherent batch, run complete release/debug suites and a fresh full
stats census. Retain functional failures, invalid owners, dropped records,
unknown process shapes and all remaining core-owned boxes. Raw kernel and
full-suite admission remain open. Ctypes and LRU ownership implementations
remain pending explicit user approval following automatic-review rejection.
