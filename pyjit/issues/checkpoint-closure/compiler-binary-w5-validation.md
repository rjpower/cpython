# Compiler binary-fold overlay validation

Under the shared W5 release lock, the current main `Python/flowgraph.c`, `Python/compile.c`, and `Include/internal/pycore_compile.h` were overlaid with `w5-cold-compiler-binary.patch`. The release `make all` completed with 117 modules checked and zero failed imports. `test_compile` and `test_ast` passed: 426 tests run, 3 skipped.

The first supplemental arithmetic probe stopped at its own inaccurate exact expectation that `(-1)**0.5 == 1j`; CPython's complex result has a tiny real component. The corrected standalone [probe](compiler-binary-arithmetic-probe.py) uses a tolerance and passes against the restored W5 interpreter. It should run once on the merged candidate to establish patch-specific evidence. No source failure was observed in the failed probe.

The three W5 files were restored from exact backups, `make all` was rerun (117 modules, zero failed imports), and the lock was released. Logs: `compiler-binary-w5-overlay-build.log`, `compiler-binary-w5-focused-tests.log`, `compiler-binary-w5-arithmetic-probe.log`, and `compiler-binary-w5-restore-build.log`.
