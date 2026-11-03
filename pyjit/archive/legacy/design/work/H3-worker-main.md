# Retained worker launcher correction

Status: tooling-only fix, 2026-10-29. No native source or generated product
belongs to this commit.

The retained compiler/H3 launchers called `worker_process()` directly.
That bypassed `worker.main()`'s per-worker temporary cwd and exit timeout.
A strict H3-1 stage2a kill replay exposed the consequence: test_opcache's
file snapshot observed another worker's `@test_PID_tmp` file appear or
vanish in the repository cwd. The earlier quiet replay reported file
changes in test_code and test_opcache. No test assertion or native crash
was implicated. Verbose mode also made an independent `-m platform` probe
that the old actual-mode launcher rejected.

The three retained helpers now set the normal worker argv and call
`worker.main()`. Copied-generator imports and their path/hash assertions
remain before that entry; actual collector-mode assertions remain before
all test workers. The actual-mode helpers separately support regrtest's
host-platform probe. The copied-generator helper already supported it.
Historical gate logs, manifests, variant patches and provenance remain
unchanged; this correction does not relabel those runs.

## Fresh evidence

The fresh checks used current H3-1 stage2a draft native/generator sources
at base `6c724c29408` plus the exact source hashes in
`controls/H3-worker-main/provenance.json`. They are not checks of a clean
stage1 or the later coordinator integration. Release/Og debug interpreters,
build configs, extensions and launcher hashes are recorded there.

- Each actual-mode launcher passed strict verbose test_code/test_opcache:
  135 tests, one skip, no environment change. Both workers per invocation
  witnessed the collector's actual `(2, 0)` kill mode.
- The copied-generator positive ran all ten current HandlerDomain tests;
  all seven existing negative variants failed their selected invariants.
  Driver and worker asserted copied module paths and the variant hash.
  The pin variant reproduces the two inherited invalid layouts; the
  identity variant's metadata emitter also reaches its unrelated empty-max
  error after its identity guard is removed, while the other emitters
  fail the missing-identity expectation. Complete logs preserve that scope.
- Live generator/input/product/test hashes match before and after all
  copied variants, and the task-owned generator scratch was removed.

Only task-owned copies changed the runner's output directory and the two
mode launchers' journal paths, to avoid altering historical outputs.
Exact relocation patches and original/relocated hashes are retained.
Those changes do not affect imports, mode assertions or worker entry.
Both native interpreters and all native/test source files stayed unchanged
through these runs.

The coordinator owns validation on its stable integration and any reruns
of larger matrices. This commit corrects the retained launch protocol;
it does not claim completion of H3 runtime stage2.
