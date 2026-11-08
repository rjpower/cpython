# Coordinator handoff

The 2026-11-08 golden runtime is published. Read [STATUS](STATUS.md), the
[manifest](issues/golden-20261108/README.md), [current IR](design/register-ir.md),
[interpreter](design/interpreter.md) and [next plan](issues/golden-20261108/next-steps.md).
The previous 435-line board is [archived](archive/20261108/HANDOFF.md).
Do not recover active assignments from its historical instructions.

## Source and evidence

- Canonical documentation: `/home/power/code/cpython/.worktrees/python-jit`,
  branch `weaver/python-jit`. This checkout's CPython implementation is not the
  measured runtime. Use the published combined golden snapshot for a coherent
  source/documentation checkout; see the manifest.
- Runtime: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`,
  branch `boil/pyvalue-rc-universal`, clean HEAD `0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae`.
  Origin has this branch and immutable tag `pyjit-golden-20261108-runtime`.
  Runtime implementation is frozen. Its old `boil/` documentation is historical;
  the combined publication replaces it with canonical `pyjit/` documentation.
- Functional qualification and all final timing captures: `0267d428808`.
  Runtime successor `0765e9be47f` moves one existing comment. Debug `.text`
  matches exactly. Release differs only in four address-relocation bytes for
  equivalent `f` literals caused by build-identifier suffix merging. Do not
  claim whole ELF identity. The exact measured ELF is retained.
- Build root: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
  Builds: `pyvalue-rc-repr-{release,debug,stats}`. Current binaries identify
  `0765e9be47f`, GCC 15.2.0, JIT off, 117 modules each; rebuilt stats smoke
  passed 142 tests. Matched upstream: `upstream-release-devlibs-3a440b6`
  at `3a440b65695`; historical interim: `eba351fee7d`.
- Evidence under build root: `pyjit-link-wave1/interpreter-parity-20261108/`.
  Its `run-checks.py`, `run-performance.py` and receipts contain exact commands.
  Final captures include `final-qualified-*`, `comment-cleanup-*`,
  `final-target-{1,2}`, `final-broad`, `final-vs-r5`, `final-perf-*`,
  `spectral-perf-*`, `profile-analysis` and `warmed-shape-comparison`.
  Retain failures and do not reuse output names. The
  [report](bench/results/interpreter-parity-20261108.md) records evidence limits.
- Temporary route/hit counters were retired byte-for-byte; stats was rebuilt
  after restoring the header layout. Current runtime has no diagnostic overlay.
  R5's ELF remains at `baseline-identities/candidate-python` under the evidence
  root. Its old build-local control copy and the prior task worktree were
  removed; recreate that copy before reusing the direct-comparison launcher.

## Next execution

The golden review is advisory. Follow the staged [next plan](issues/golden-20261108/next-steps.md)
before more runtime edits. Calls already use frame stacking/overlap; logging has
no consuming-binder fallback and its hot callees have zero constant homes.
Its retained message is mortal. Do not repeat obsolete argument-copy or
constant-home hypotheses. Keep spectral in integration screens. Its CPI
regression has no established cause.

Use Terra medium for implementation, Luna medium for documentation/mechanical
work, Sol for substantial design/debug/review. Workers do not commit. The
coordinator owns integration, native builds/tests and quiet measurements. Use
`uv run` for host Python drivers, sanitize `PYTHON*`/`LD_PRELOAD` before native
children, and use the existing shared caches. One compiler: GCC. JIT off must
be checked in native children, including the fork's own method-JIT admission.

## Preserved local work

These canonical changes predate this checkpoint and remain outside its commits:
emptied `bench/results/checkpoint-6a0e4c12c17.md`, appended
`issues/checkpoint-closure/ctypes-rawpin-build.log`, and untracked
`issues/checkpoint-closure/w5-release.lock`. The published tree uses their
committed content. Do not stage, reset or delete them incidentally.

The [cleanup audit](issues/golden-20261108/cleanup-audit.md) found dirty legacy
worktrees, cited ignored evidence and an active bulk-build source dependency.
Keep those paths; clean Git status alone does not make a worktree disposable.
Historical branches, ABI and build names retain their identities. Canonical
docs own current state; Loom artifacts mirror them and retain prior revisions.
