# M0 actual debug feedback review

ACCEPT focused M0 behavior at source `f0a49b6d30a38db705329bcfd5e5f2d6d2413710`. Root observed supervisor 80454 exit 0 at 17:14:45 UTC. Both fixed rows and their verifiers returned zero: the fresh GCC/O1/Py_DEBUG build and all eight exact `TestMethodHelpers` methods, with zero skips. The independent replay checked 7,477 completion artifacts, all 585 frozen build products, tool/config/source/image bindings, scheduled and returned method IDs, actual worker/library/CAPI origins and ordinary RC configuration.

Actual assertions cover cold and later loop/branch/Python-call execution with one target return, C-to-dunder effects once, caught helper error without replay, nested/error TLS restoration, bounded before-effect fallback, profile activation and fresh-code recovery, owned-local publication before destructor reentry, and a foreign generator call. These exercise generated C helpers and the existing evaluator Action consumer. They do not establish native machine-code entry, compiled caller continuation, ENTER_STARTED/YIELD producers, elapsed benefit or broader qualification.

The build reports 116 modules: 37 built-in, 67 shared, one not applicable, one disabled and ten missing; none failed import. Missing optional-module scope remains explicit. Root additive cleanup at 17:14:52 UTC verified all 21 locks, native idle, 30 unchanged parent files and all 137 preserved failed-attempt files, then removed only the owned empty m0d2 cache. The original 49cc build failure remains failed and immutable.

The focused M0 prerequisite is satisfied. M1 source work and every later execution remain separately authorized.
