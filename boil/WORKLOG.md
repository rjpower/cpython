# Boil-the-ocean work log

Branch `weaver/python-boil-the-ocean`. Plan artifact: Loom `plan` rev 2.
Entries are appended in order; each milestone is also a commit.

## 2026-10-23

- Reviewed shellsim's ports tree and wrote the plan artifact (rev 1), then
  rev 2 after review: added the internal IR (idea 0), dropped the
  copy-and-patch JIT in favour of our own tier 0 and tier 1, V8 embedder API
  as the handle-API bootstrap.
- Phase 0 started. Layout decisions:
  - `boil/` holds this log and design notes (`boil/design/`).
  - `ports/` is the package ports tree, modelled on shellsim's.
  - Out-of-tree builds live under `build/` (gitignored):
    `build/baseline-default`, `build/baseline-jit`, `build/baseline-ft`.
  - Benchmark results go to `boil/bench/` as JSON plus a short summary.
- Machine: 16 cores, 62 GiB, gcc 15.2. LLVM 21 runtime libs present but no
  `clang-21` binary; installing `clang-21 llvm-21` for the JIT baseline.
