# Round51 qualification policy

Each option uses a separate source checkout and release/debug build directories.
Workers leave unrelated subtrees and frozen controls unchanged. The coordinator
owns source commits, builds, generated products, tests and quiet measurements.
Use the existing GCC15.2 configuration and shared uv/cache locations. No builds
or measurements were launched while preparing these entrypoints.

## Frozen comparison

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
The frozen source is `BASE/worktrees/round51-baseline`, build
`BASE/round51-baseline-release`, commit
`bf810ec1a33cacb94102c73779d015c18f85aa8d`. Root is preparing this control;
its named path is not evidence that the build has completed qualification.
Upstream remains `BASE/upstream-release-devlibs-3a440b6` at
`3a440b65695`. Never rebuild or modify either during measurements.

## Diagnostic quarantine

The default runner passes exactly:

```text
--ignore test.test_gdb.test_pretty_print.PrettyPrintTests.*
```

The native regrtest parser implements `--ignore` as a negative glob filter;
`Lib/test/libregrtest/filter.py` matches a dotted wildcard against the complete
test ID. This excludes only methods of this class. No runtime test is modified.
Use `--include-gdb-pretty-print` to restore it for diagnostic work.

Final50 `delta50final-rfull-release-full.log` and
`delta50final-dfull-debug-full.log` under
`BASE/pyjit-link-wave1/deltablue-call-model-20261110` report 16/21 failures in
`test.test_gdb.test_pretty_print`. That module contains one `PrettyPrintTests`
class. Quiet logs do not identify the failing methods; class quarantine also
excludes passing methods in that class. It is a deliberate diagnostic coverage
gap, not a fix. Backtrace, C-function, full C-function and miscellaneous GDB
groups passed and remain active. `test_sys_settrace` and `test_lltrace` remain
active; their established jump/trace-format failures remain visible.

Every journal records the ignored pattern and qualification label. Report a
passing filtered run as "passed with GDB display diagnostics quarantined".
Do not call it an unfiltered full pass or describe these failures as fixed.
Unexpected failures still fail the runner. To narrow the quarantine further,
first collect case-level evidence outside a quiet timing window.

## Native checks for each subtree

From the canonical checkout, set `ROUND51_SOURCE`, `ROUND51_RELEASE`,
`ROUND51_DEBUG` and `ROUND51_EVIDENCE` to that option's absolute directories.
Use a fresh tag for each invocation. Commands use host Python to launch the
explicit native executables; they do not execute the tests with host Python.

```sh
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B pyjit/issues/interpreter-round51/run-checks.py option-focus focus test_ir test_ir_contract --source "$ROUND51_SOURCE" --build "release=$ROUND51_RELEASE" --build "debug=$ROUND51_DEBUG" --evidence "$ROUND51_EVIDENCE"
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B pyjit/issues/interpreter-round51/run-checks.py option-full full --source "$ROUND51_SOURCE" --build "release=$ROUND51_RELEASE" --build "debug=$ROUND51_DEBUG" --evidence "$ROUND51_EVIDENCE"
```

Add the option's own focused modules. The runner retains revision, bytecode magic
and source-Lib identity preflight, sanitized environment, hash0/JIT-off settings,
durable commands/logs and native failure exit status. Native qualification may
run before commit; the journal records its source diff. Benchmark qualification
requires a clean committed candidate with an executable identifying that commit.
The full runner stops on a failing role, matching the established runner; invoke
roles separately with unique tags when both diagnostic outcomes are needed.

## Matched measurements

`measure.py` delegates to the established DeltaBlue runner without changing its
CLI or algorithms. `CANDIDATE_MODULE_LOG`, `FROZEN_MODULE_LOG` and
`UPSTREAM_MODULE_LOG` must name the corresponding immutable native module-check
logs. The runner checks 117 candidate/frozen modules and 116 upstream modules,
clean source/build identities, extension origins, ELF IDs and disabled JIT before
and after timing. The unchanged driver runs the kernels' existing checks during
warmup/timing. Logging raises on incorrect output; DeltaBlue prints failures.
The measurement reader now requires exactly the two expected output lines
(metadata and samples), rejecting printed failure reports and naming their
retained stdout artifact. CPU15, hash0, balanced order and positive samples
remain required.
Reserve quiet host access with the coordinator before running either command.

```sh
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B pyjit/issues/interpreter-round51/measure.py --control /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/round51-baseline-release --control-role golden --control-head bf810ec1a33cacb94102c73779d015c18f85aa8d --candidate "$ROUND51_RELEASE" --module-log "golden=$FROZEN_MODULE_LOG" --module-log "candidate=$CANDIDATE_MODULE_LOG" --output "$ROUND51_EVIDENCE/target-vs-bf810" --cases deltablue,logging_silent --rounds 6 --cpu 15
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B pyjit/issues/interpreter-round51/measure.py --control /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/upstream-release-devlibs-3a440b6 --candidate "$ROUND51_RELEASE" --module-log "upstream=$UPSTREAM_MODULE_LOG" --module-log "candidate=$CANDIDATE_MODULE_LOG" --output "$ROUND51_EVIDENCE/target-vs-upstream" --cases deltablue,logging_silent --rounds 6 --cpu 15
```

`golden` is the existing runner's generic frozen-runtime role, explicitly pinned
to bf810 here. Ratios are candidate/control elapsed time; lower is faster.
Use two balanced pairs for the unchanged 29-kernel screen and six for target
confirmation. Preserve raw identities, status, commands and per-invocation output;
exclude overlapping campaigns explicitly. Host syntax inspection is not native
qualification, and neither identity checks nor a timing result establish an
unfiltered full-suite pass.
