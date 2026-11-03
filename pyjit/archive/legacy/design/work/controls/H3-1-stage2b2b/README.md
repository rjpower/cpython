# Monitoring storage evidence

The as-built ledger is [H3-1-stage2b2b-work.md](../../H3-1-stage2b2b-work.md).
[final-provenance.json](final-provenance.json) binds the tested parent plus
13 native/test source hashes, both binaries and CAPI extensions, seven generated
products, worker receipts and the retained artifact hashes. The accepted design,
original valid-AST probes and rejected first CAPI proposal remain unchanged.

Final combined qualification uses `combined-sources.json` (a6c07b4c...) and
`combined-production.patch` (b2098362...). Its runtime logs start with
`combined-`; final short controls use `allocation-final-*`, `synthetic-final-*`,
`no-monitor-final-*`, `global-oom-final-*`, `storage-controls-final2-*`,
`eligibility-combined-*` and `final-ast-*`. The copied controls' complete
source/header variants and patches are in `final2-storage-*`.

Historical filenames are preserved. In particular, `frozen-*` identifies the
earlier 603 candidate, and `refrozen-*` identifies 54b, whose affected run
failed. The older `final-smoke-*` and `final-targeted-*` files are 603 results,
not combined qualification. `old-*` holds membership/OOM failures, and the
original `default/` and `debug/` AST logs hold the eight old-codec results.
Early allocation outputs and final outputs have separate directories and
native identities.

`eligibility-old-default.*` is the bounded old RESUME watchdog failure;
the identical probe completes in `eligibility-combined-*`. The three-guard
prerequisite is qualified only as part of the combined storage candidate.
`final-storage-default` preserves an initial copied-helper compilation error;
it earns no semantic-negative credit. Passing controls use the v2 driver,
C11 and the production CAPI assertion context. The first codegen-closure
launcher failed to supply the checker script's import directory; passing
`*-v2` closure is separate. Neither correction changed native source.

The scripts are exact qualification inputs, not a replacement for current
gate orchestration. To repeat them, copy them into a new task-owned directory
under `build/`, use the recorded source/configuration, obtain the normal build
lock and machine slot, and retain a fresh output cohort. Do not run them in
this evidence directory: several scripts intentionally write outputs beside
themselves. The outer probe's recorded dirty-diff hashes describe the actual
qualification tree; the committed manifest supplies its complete identity.
Extension binaries are not committed; their hashes, build commands and
consumed-header identities are retained. No source variant was written into
the live tree by a negative control.
