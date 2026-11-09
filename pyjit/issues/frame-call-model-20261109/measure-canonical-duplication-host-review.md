# Component driver host review

The driver implements the approved four-round, three-case retained-executable
comparison. Candidate execution remains coordinator-owned. The sealed baseline
identity supplies the original configuration; current shared sysconfig data
alone is not evidence of the retained executable's build configuration.

Host checks passed without launching either candidate:

- Parse the controller and compile its generated child.
- Check that the generated child retains the original warmup and seven-call
  timing block. Identity assertions run outside each timed interval.
- Check the baseline manifest contains the expected 213 extension paths in the
  shared build directory.
- Invoke the controller's invalid-revision preflight with every subprocess
  entry point forbidden. It rejects before subprocess creation and records an
  incomplete status and empty command journal.
- Review the final source and run `git diff --check`.

An initial host assertion counted an indented substring rather than complete
lines and failed. Correcting that assertion verified the unchanged timing
block; it required no driver change. These checks do not establish native
compatibility, campaign stability or a performance result. The coordinator must
run the driver against the clean committed after image and review both identity
snapshots, command journal and status before using its summary.

The first coordinator preflight rejected before timing because the baseline
records lexical build-directory extension paths, while the driver resolved
their symlinks into `Modules`. The driver now compares lexical absolute paths
and still hashes their contents. A host check confirms the complete 213-path
set matches the baseline; native preflight must still establish hash equality.
The failed campaign is retained. Build sysconfig bytecode caches are also
included in the shared input snapshots.
