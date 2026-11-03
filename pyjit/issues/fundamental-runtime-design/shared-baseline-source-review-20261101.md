# Shared attribution baseline: source and scope review

2026-11-01, coordinator. **Accept the source bridge for one observational
baseline; helper review and explicit activation remain required.** This accepts
neither a runtime change nor a performance or qualification claim.

The clean managed source is `417a9443a599e879c76c34c13ae76943bd6f6b3e`.
The enumerated 4,174 runtime/build/test files match the actual qualified e9
source. Against the maintained primary source, six Windows batch/PowerShell
files have explicit CRLF/LF-only exceptions; those files are outside the Linux
build and selected tests. The remaining 4,168 files match. This is an enumerated
source bridge, not whole-commit equivalence. Current primary documentation does
not define the native image. Host driver inputs are bound separately.

Root's actual preflight at 02:37:10 UTC rehashed all 4,193 mixed source/helper
inputs, 97 driver inputs and 50 capsule artifacts; checked the tool paths and
digests, exact clean worktree HEAD, absent activation and empty build directory;
and acquired then released both machine slots and the shared serialization
lock. A separate host process observation found no CPython/compiler/profile
job. These observations grant no lasting reservation; the accepted executor
must acquire its locks and verify identity at execution.

| Input | SHA256 |
| --- | --- |
| `preparation-capsule.json` | `32afb06637d12b86fa15cc50df812b22fb118fd72e58ada885b485e64d6fa134` |
| `plan.json` | `974097c852c66d9e92a8d08da49864bc80ed82790b70339fb78239bf6c034a87` |
| `launch-held.json` | `346e4a0d590c362ded86141f5919f5e486780edaa1f6c9a1656b419cf5dc8298` |
| `source-bridge.json` | `d1e670ce01c256cf56aef52e18c285af79f5f7d2d416dc034c27da1ec5eaa8b1` |

These inputs live in
`build/worktrees/vm-compiler-backend/build/shared-attribution-gcc-stats-v1`.
Root's receipt is
`build/shared-attribution-preparation-v1/root-preactivation-preflight.json`.
The independently accepted counter driver remains separately pinned by the plan.

The scope is one fresh GCC computed-dispatch O1/debug/stats build, twenty
frame/root methods, six stats methods with ten original child sources, the
canonical six-by-100 stale-root child, and six registered counter consumers.
The consumers create eighteen workload processes and twelve auxiliary probes.
The build includes its normal bootstrap/build-time processes; eighteen is not
a count of every process in the batch. Original dumps, actual origins/maps,
raw dictionaries, signed differences and failure streams must survive.

The six consumers are a shared observation set. Regex implementation is paused
under the user's priority correction; this baseline does not restart it.
Global entries/pairs cannot establish per-PC success frequency. Native return
PCs require per-process image/load-map interpretation before comparing sites;
raw ASLR-dependent key subtraction is not site attribution. Existing GC totals
do not supply object-state eligibility or allocation-family distributions.

An activation may authorize one inclusive 45-minute batch with the existing
210-second termination/verification reserve. The first unexpected failure
stops it; no automatic rebuild, retry, substituted workload or new probe.
Keep source-only architecture work separate from native execution. The old
nursery dispatch stays deferred, with its original deadline unchanged. This
baseline has its own explicitly activated clock and earns no elapsed speedup.
