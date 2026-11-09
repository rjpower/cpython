# Independent region-site import-repair review

2026-11-01. **ACCEPT exact v2 host readiness.** No blocker remains. The
replacement remains held for a new root activation; this review runs no target,
compiler, probe or workload and grants no retry itself.

The failed v1 was a real host-preparation failure which my earlier review
missed. I verified pinned source and fixtures without actually importing the
supervisor under the composed search path and module cache. Its feedback_inputs
resolved baseline build_once, whose argv variant check rejected `launch`.
Session 73805 ended 1 before run_row/gate/target. The activation, start, failure
and root disposition are preserved; no launch receipt, target streams, child
result or completion exists. **V1 has zero target/workload calls and receives
no region observation credit.**

Reviewed capsule: `build/region-site-attribution-20261101-v2/`.

```text
plan.json 7936781c70f37846879efe8c02348e12ff2c2d895f82521a1c4c244de5a7a348
launch-held.json 094b5e2e16f6a15ca24cd329f35e37b23601b85d4f91af6bd6db9f177969dedd
ready.json fd6d37fd75e4642770567e830e730a32032b6923d3af18d884e0e31b23fa5c66
host-receipt.json d54cc717812253de352ec6b0133cb1e94175c367a858cd0b576d8b620bcba67f
helper.diff bd3c6d8636e9f47757d330aa90f37cb88703726701f0097d2c4b2d7e4bb9cb9f
```

The repair is narrow. `bindings.load_supervisor` loads the actual pinned
sealed build_once, feedback_inputs and run_batch by filename under temporary
canonical module names. It verifies every file hash, loaded origin and the
supervisor→inputs→build-module object identities. A finally block restores
preexisting entries, including the absent-entry case. It neither relies on
search-path precedence nor clears unrelated module state. The existing
run_row body is unchanged; the launcher still rebinds only its output, root
and build inputs. Ordinary source/image/authority guards remain intact, with
an added check that v1 target paths remain absent.

The new host fixture executes the **actual launcher/import graph** through
the row boundary with literal `['launch.py','launch']` argv and baseline path
ordering. It covers cold imports and the real baseline build_once plus its
feedback_inputs/supervisor already cached. The corrected dependency graph is
checked before the existing row body is replaced with a deliberate stop;
subprocess.Popen is forbidden. Original cached modules are restored afterward.
An altered-hash fixture rejects the load and verifies cache preservation.
These tests close the missed import gap without invoking a target or trusting
a synthetic import-name stand-in.

Independent replay passed all ten host controls. Rechecked 89 reference pins,
20 readiness artifacts, eight helper hashes/modes, current clean 417a source,
actual image/CAPI and selected tool hashes. Target, contract, run and authority
are byte-identical to accepted v1; all original plan fields except added prior
failure bindings are unchanged. The job differs only in its owned v2 helper
path. The accepted image remains
`c26805f272103c51e2ebf23065aba3d781975f636b2a0d94fbae7274bf28aa01`.

Thus exposure remains one child, one warmup run(4), one counted run(4), no
additional observer inside counting, no image build or timing. Original dump,
partial streams, actual modes/origins/root checks, three code snapshots,
specialized-site bridge, feedback and locals-kind handling are unchanged.
Typed nonnegative unpinned remains explicit/report-only; zero fallbacks,
actual PID and positive collections remain mandatory. No per-PC counter,
compiler-admission proof or successful-region count is invented.

Root must independently activate a fresh inclusive scope of at most 600 seconds
with the unchanged 360-second row cap and 210-second cleanup reserve. Neither
v1's failed activation nor the earlier baseline grant is extended implicitly.
The shared serialization/machine/image locks, stop-on-first-failure behavior,
exclusive outputs and original supervisor cleanup contract remain in force.

Reviewer evidence: `build/region-site-attribution-review-20261101-v2/`.

```text
receipt.json ff488ec60b3035b16f1f86d2adbb56017851ac4e6e1d9c2af63689aeacf463ee
binding-check.py 2ba36e5934b19160f37d450b4ccd122f21aa4b7d0c7782efcddb233a58a994c3
host-controls.log 364ca0b93f5162b017c2499855565f2b0038c3dfc20bcf089068ec93470f42bb
v1 failure.json bdc954a00e1bd8d79b75b111eb6f112047047ee4dac87628f9416be72a234cad
v1 root-failure-disposition.json 318e669004baa5d7b68a7cffb6043763db32fce443323639d004827c861b5ac9
```

No helper was edited by this review. Actual site-attribution acceptance still
requires the future target's complete result and independent outcome review.
