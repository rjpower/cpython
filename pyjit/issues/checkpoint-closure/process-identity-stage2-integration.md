# Process identity integration

Runtime `84edb605d31` integrates the frozen stage-one launch capture, reviewed
stage-two reader corrections and C `.done`/`.zero` producer. The canonical
benchmark consumer landed in documentation/tooling commit `5aef6eef1e2`.

All three profiles build the expected 117 modules, with no missing modules or
failed imports. Lint, regeneration, limited ABI and frozen-main checks pass.
The merged landing runs 4,276 tests per release/debug profile; both pass.
After the final test-only argv-summary assertion, the 57 checker/worker tests
pass again on both profiles, with one expected skip each.

Real processes pass normal-exit identity pairing, explicit zero-stats rejection,
two distinct dump intervals, fork child/parent pid correlation, and exact argv
round-trip including invalid filesystem bytes, newlines, spaces and an empty
argument. The private `richards` benchmark capture produces three journal rows,
each with exactly one same-stem `.txt`/`.done` pair. The initial host probe
failed before starting a child because its reader import path omitted the
reader's helper directory; it was corrected, and the failed attempt is retained.

Evidence is under the external build root's
`pyjit-link-wave1/checkpoint-closure/process-identity-stage2/` and
`process-identity-stage2-probes-v2/`. The latter retains raw artifacts,
`receipt.json`, the benchmark JSON/journal and exact stats ELF, build ID
`474a6d46a4e37920c82d1bd907cbfb5f991c77ed`. Its source matches committed
`84edb605d31`; the embedded revision is the preceding dirty build identity.

This is a coherent producer/reader landing, not full-suite R27 admission.
Native post-success child receipts, external/zero/unfinished process
dispositions, remaining core boxing and complete functional qualification
remain open. No elapsed-time performance result is claimed.
