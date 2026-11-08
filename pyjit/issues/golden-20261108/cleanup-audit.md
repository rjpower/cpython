# Golden checkpoint cleanup audit, 2026-11-08

Scope: read-only inventory for the checkpoint. No worktrees or artifacts were
removed. The filesystem had 159 GiB available of 882 GiB (82% used). Worktree
status counts below include tracked changes and ordinary untracked files, but
not ignored build products; they do not establish that a tree is disposable.

## Retain

- The canonical checkout and `python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`
  are the documentation and runtime sources. Preserve the three preexisting
  canonical changes: `pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
  `pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`, and untracked
  `pyjit/issues/checkpoint-closure/w5-release.lock`.
- `build/checkpoint-bulk-20261105` has 52 status entries and is referenced by
  an active `make`/configure using `build/checkpoint-bulk-build-20261105`.
  Its configuration also uses `python-boil-the-ocean/build/devlibs`. Keep all
  three paths. `build/checkpoint-h3-emitter` has 20 status entries and is cited
  in the H3 producer receipt; keep it too.
- `/tmp/a5b4/trial` is a registered worktree with 59 status entries, including
  staged source changes. Despite its location, it is not disposable scratch.
- Under `python-boil-the-ocean/build/worktrees`, `a3s2`, `a5b4`, `b1`,
  `b10iii`, `b2`, `bpin`, `g2-a4`, `g2-b3`, `g2-base`, `m1b`, `m1b-e`,
  `m1b-thin`, `m2`, `m3a`, `m3b`, `m3b-dryrun`, `m3b-trial`, `rfix`,
  `v2a1a`, `v2a1a-w2`, `v2a1a-w3`, `v2a1a-w4`, `v2a1a-w5`, `v2a1a-w7`,
  `w1`, `w4`, and `w5` have changes or untracked files. Preserve them and
  their worktree registrations; do not force-remove or reset them.
- The clean `constant-operands` and `dispatch-slot-base` worktrees contain
  ignored build/evidence paths cited by historical reviews. Other clean trees
  may likewise contain evidence or be source paths for builds. Preserve sealed
  evidence, baseline binaries, shared dependencies, and historical branches.
  A clean status is not sufficient proof that removal is safe.
- Tracked filename searches for watch, temporary, polling and cleanup scripts
  found only historical `archive/legacy/.../no_monitoring_flag.py`; retain it
  with the archive. No tracked obsolete watch/temp tool was established by
  this audit. The names `comment-cleanup-*` identify checkpoint evidence, not
  disposable cleanup scripts.

## Proposed operations

No specific existing worktree or tracked tool is established as safe to remove
now. Retain the inventory pending explicit evidence ownership and reproduction
checks. If a future pass identifies a clean, unused tree with no required
ignored artifacts or build references, record its HEAD and branch, check for
active process use and status again, then use `git worktree remove <path>`;
retain its branch and do not use `--force`. Detached commits need an explicit
retention ref before removal if their continued reachability is required.
Check build configuration and receipt references as well as current processes;
a running-process snapshot cannot prove a path is unused. Recheck status just
before any operation. This audit did not scan large build trees or measure
individual tree sizes, and did not run native tests.
