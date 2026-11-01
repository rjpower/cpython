# RC compatibility feedback: author outcome

The corrected v4 batch completed successfully on unchanged RC source `d7df12b1d81722927e71d9734c70eeafdccfee73`. This is the helper author's evidence report; independent acceptance belongs to the coordinator and peer reviewer. Root confirmed supervisor 41374 exit 0 at 06:54:15 UTC. Completion records 06:53:26.451745, before the declared 07:22:25.398985 deadline.

| Actual exposure | Debug | Release |
|---|---:|---:|
| Selected methods passed |50|48|
| Selected skips |0|0|
| Driver + ordinary workers |10|10|
| Platform auxiliary |1|1|
| Embedding programs, exit 0 |6|6|
| Expected aborts, SIGABRT/returncode−6 |2|0|
| Natural environment probe, exit 0 |1|0|
| Distinct target PIDs |20|17|

The two original debug negative-reference-count cases retained their unmodified child code and each produced the required `_Py_NegativeRefcount` assertion in its retained stderr. Their expected failure is separate from ordinary method success. The natural `python -E -c 'import sys; sys.exit(0)'` probe ran once with empty streams; its cache and isolation semantics were preserved. All 12 embedding programs returned 0. There were 98 selected method invocations across 50 distinct methods and 37 distinct target PIDs, including both platform auxiliaries.

The host reader rehashed all 7,216 completion artifacts and replayed the frozen ordinary-result and child-ledger validators. It additionally checked each case against its actual suite-result worker PID and checked PID disjointness across ordinary, platform and inner processes in both rows. Source/configuration/tool/image/module bindings matched; each build retained all 584 original products, with no added bytecode. Both Python and `_testembed` were idle in retained postgate cleanup under the build locks, and remained idle during this audit. Git source stayed clean, including the restored tracked gate script.

The ledger observes child argv, requested environment, PID/start identity and frozen executable origin. It does not provide a tracing-mode getter or root-statistics proof for this historical RC runtime. Recorded count/holes remain diagnostic observations; this report grants no diagnostic qualification, performance, integration or collector-selection credit.

The failed v3 launch remains preserved, including its accidental uninstrumented startup and tracked-gate truncation/restoration receipt. It earned zero selected-test credit. The v4 correction validates the complete timeout/shell/gate/lock/Python/command sequence before launch; the earlier review miss is not erased by this successful batch.

Evidence: `build/rc-compatibility-author-outcome-20261101-v4/outcome.json` SHA256 `3853dea21c55382e76bd913c2f0b38aee49eefd295e8e8ea4371b860a3c91724`; reader receipt `cde23295470f36135186035bcee61a58c035e45b91ee92f1b48a1909f8db8a96`. Native evidence remains in `build/worktrees/rc-successor/build/rc-compatibility-feedback-v4`. No new target, compiler or measurement was invoked by this audit.
