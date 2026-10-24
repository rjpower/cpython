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
- Ports tree landed (`ports/`, about 1000 lines of driver). Verbs: `build`,
  `check`, `bench`. Six ports pass `check` against the baseline interpreter:
  six, attrs, pycparser, markupsafe (C speedups), pyyaml (pure loader only,
  no libyaml headers on the host), cffi (ABI mode). One patch so far: cffi
  needs `PyConfig_Get("filesystem_encoding")` because 3.16 removed
  `Py_FileSystemDefaultEncoding` (10 patch lines). Everything is keyed by
  `abi_id` in `ports/sdk.json`, currently `cpython-3.16-refcnt-v0`.
  Host has no OpenSSL headers, so the target interpreter has no `ssl`; the
  host fetches `build_requires` wheels and the build venv installs offline.
- Design note `boil/design/02-tagged-values.md` landed. Decisions: JSC-style
  NaN-boxing of `_PyStackRef` now (raw pointers, doubles offset by 2^49,
  int32 at `0xFFFE_...`), inline floats in Phase 1, NaN stays boxed, value
  identity for inline ints and floats, internal loop indices move to their
  own region. Measured today's costs with `perf stat`: an int add is 164
  instructions per iteration, a float add 130. Seven-step sequence with an
  overhead gate at step 2. Coordinator decisions recorded in section 12.
- Design note `boil/design/01-internal-ir.md` landed. Decisions: 64-bit IR
  words (16-bit handler index plus three 16-bit fields, optional second
  word), 15-bit slots with a borrow bit, one typed feedback block per code
  object with seven kinds absorbing all 15 inline-cache structs, lazy
  lowering at first execution, in-place quickening of the handler field.
  Measured maxima on the stdlib: 803 slots per frame, 32 percent of
  instructions specialisable. Nine-step sequence; step 1 deletes tier 2 and
  the copy-and-patch JIT. Coordinator decisions in section 10.
