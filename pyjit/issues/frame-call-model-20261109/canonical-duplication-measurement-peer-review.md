# Canonical duplication measurement driver: source review

Disposition: **accept for the bounded three-kernel component comparison**.
I found no source blocker against
`measure-canonical-duplication-plan.md`. This is a read-only review of
`measure-canonical-duplication.py`, not a timing result.

The driver ties the retained before executable to the sealed baseline's
full revision, SHA256, ELF notes and build ID. It requires a distinct
after inode in the same build directory, clean after source HEAD, the
same configured Makefile hash and exact 213 shared-extension path/hash
set. Both release probes must match the sealed configuration, GCC
version, 117-module import gate and JIT-off state. A complete preflight
snapshot precedes the first benchmark child; the same snapshot is
repeated after all children. Source, benchmark, build inputs, imported
files and the driver itself are hashed or checked for drift.

Each child is pinned to CPU 15 and checks native revision, affinity,
hash seed and JIT state before and after its unchanged one-warmup,
seven-timed-call driver loop. Four rounds reverse before/after order;
the cases and fixed registry loop counts are `nbody`,
`logging_silent` and `pickle`. Raw stdout/stderr, commands, seven
durations and round medians are retained. A child, load, timeout or
postflight identity failure leaves an incomplete status and cannot
publish a successful summary.

The result can support only this component comparison. Four paired
rounds and three kernels do not establish a general performance win;
the static load-handler instruction reduction is separate evidence.
Root should report per-case paired ratios with the retained identities,
and treat any failed preflight or postflight as no measurement.
