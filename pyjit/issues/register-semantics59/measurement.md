# IR59 execution logistics

Execution guide, 2026-11-14. The [trial report](report.md) owns completed build,
qualification and timing evidence. Root owns those jobs and selection. Read the
[handoff](../../HANDOFF.md) before execution; canonical C source is stale.

## Fixed control and candidate

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`C=/home/power/code/cpython/.worktrees/python-jit`;
`E59=$BASE/pyjit-link-wave1/register-semantics59`.

| Role | Source | Release build | Exact head / module log |
| --- | --- | --- | --- |
| Selected57, timing role `golden` | `BASE/worktrees/call55-integrated` | `BASE/call55-integrated-release` | `0dcba75bb3df40c32137db453b001f7e8f101de0`; `BASE/pyjit-link-wave1/call-model55/comp57-dictionary-release-build.log` |
| IR59 candidate | `BASE/worktrees/call55-gc`, branch recorded in the current HANDOFF | `BASE/reg59-results-release` | Use the actual committed, clean candidate head and its new build log |
| Golden56 | `BASE/worktrees/round56-golden` | `BASE/round56-golden-release` | `883524c180e4486b47df77ee2276c9276c0ec32c`; `BASE/pyjit-link-wave1/comprehensions57/control-retry-build.log` |
| Upstream | `BASE/worktrees/upstream-3a440b6` | `BASE/upstream-release-devlibs-3a440b6` | `3a440b65695615e9fd9912342b79cdd54fced0d2`; `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log` |

**Leave I and both call55-integrated builds at selected57.** This is the simplest
no-rebuild immutable control: its compiled source path, Lib, extensions,
Makefile and native revision stay coherent. Do not advance, regenerate or build
I. The retained `E55/comp57-dictionary-native/python` and live release executable
currently both hash
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
The capture has its manifest, symbols and dictionary disassembly. Preserve it;
use the original configured build for execution, not the isolated captured ELF.

A copied ELF alone would still locate I's Lib through its compiled absolute
VPATH. The chosen arrangement needs no ELF copying, rebuilt control, Pythonhome
override or import-path workaround. Source cleanliness and actual Lib resolution
remain admission requirements. Recheck the live control hash before each new
campaign rather than assuming the preserved path still contains those bytes.

## Configure, generate and qualify

`reg59-results-{release,debug}` are configured from the exact respective
selected57 configure arguments. Retain GCC, mimalloc, computed-goto,
shapes, dependency flags and frame-pointer options. The old debug configuration
also includes `--with-pydebug` and `CFLAGS=-O1 -g`; do not substitute release
flags. Existing configurations are recorded in each old Makefile's CONFIG_ARGS.
The relocated module-specific dependency flags are also required: config.args
alone loses them because the devlibs pkg-config files retain `/usr` prefixes.
The first failed build and durable config.status/Makefile.pre repair are recorded
in `E59/configure-module-relocation.json`; all module flag variables and all
core/compiler flags now match selected57. Do not copy its source/build paths.

Root's `C/pyjit/issues/call-model55/run-checks.py` extension accepts paired
`--source`/`--build-prefix` and `--evidence-root`; its defaults still target I.
**Use the explicit arguments for IR59.** They resolve this candidate to
`call55-gc`, `BASE/reg59-results` and E59 while preserving the existing exclusive
command ledger/logs, native revision/magic/Lib preflight and 117/79 module check.
Use absolute paths. Before generation/build, verify both new Makefiles' srcdir
names call55-gc: the helper's native path preflight runs at test stages, not
before make. Do not use a wrongly configured build directory.

```sh
CHECK="$C/pyjit/issues/call-model55/run-checks.py"
SOURCE="$BASE/worktrees/call55-gc"
PREFIX="$BASE/reg59-results"
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B "$CHECK" \
  reg59-results1 regen --source "$SOURCE" --build-prefix "$PREFIX" --evidence-root "$E59"
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B "$CHECK" \
  reg59-results1 build --source "$SOURCE" --build-prefix "$PREFIX" --evidence-root "$E59"
```

Generation uses the configured release build and host `PYTHON_FOR_REGEN=uv run
--no-project python -B`; root reviews and commits products before qualified
timing. Build uses `make -s -j10 -W SOURCE/Modules/getbuildinfo.c`, so the native
revision is refreshed even when the last source change is metadata. Serialize
roles and preserve each return code and complete module summary. Both fork roles
require `[117,37,79,1,0,0,0]`; upstream remains 116 modules/78 extensions.

For a changed result destination, start with native `test_ir_transfer_admission`,
`test_ir_operand_modes`, `test_ir_folded_input_transfer`, `test_ir_completion`,
`test_ir_discard_completion` and `test_ir_empty_return`, in both roles. These
cover unavailable/overwritten homes, local/scratch/discard recipients, owner
transfer, callbacks and semantic completion. Include the new packet's targeted
fixture. Broaden to full `test_ir` and `test_code`, then bootstrap/freeze and
coherent full diagnostics as scope requires. Keep all semantic negatives; the
six existing debugger-jump errors are baseline limits, not new exclusions.
Only the existing GDB PrettyPrintTests quarantine is admitted.

Selected57's last incremental build took 7.53s release and 7.32s debug, from
`comp57-dictionary-build-commands.json`; logs are 1,497 and 1,822 bytes. These were
small incremental dictionary changes, **not** estimates for fresh IR59 builds.
No fresh-build time is known. The configured selected57 release tree is about
194 MiB and its captured executable about 52 MiB; retaining the existing paths
avoids a second copy.

The shell examples below reproduce the rejected first trial. For a new trial,
choose a new evidence tag/output and the actual new build log; never overwrite
those retained outputs. The HANDOFF records the current candidate branch.

## Timing commands

Use the existing `C/pyjit/issues/deltablue-call-model-20261110/measure.py` unchanged.
It already accepts explicit build paths, `--control-role golden`, and a full
`--control-head`. No script with embedded 9ec, 72ee or other old candidate head
may launch a new campaign. Establish the candidate's clean committed source
and native identity first; use a new output directory for every campaign.

```sh
MEASURE="$C/pyjit/issues/deltablue-call-model-20261110/measure.py"
CASES=comprehensions,deltablue,coroutines,generators,logging_silent,raytrace,spectral_norm,nqueens
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B "$MEASURE" \
  --candidate "$BASE/reg59-results-release" \
  --control "$BASE/call55-integrated-release" --control-role golden \
  --control-head 0dcba75bb3df40c32137db453b001f7e8f101de0 \
  --module-log "candidate=$E59/reg59-results1-retry-release-build.log" \
  --module-log "golden=$BASE/pyjit-link-wave1/call-model55/comp57-dictionary-release-build.log" \
  --output "$E59/results1-eight-selected57" --cases "$CASES" --rounds 2 --cpu 15
```

Those eight cases are four hot workloads plus four sentinels, not an all-29
result. Two pairs produce 32 workload rows; six confirmation pairs produce 96.
For all29, pass all kernel names explicitly in `--cases`; omitting that option
selects only `logging_silent`. Require 29 summary entries and 116 rows before
calling a two-pair campaign all29.
Do not alter fixed kernel loop counts or warmup/timing bodies. For upstream,
replace control path/log with the table's upstream entries, use role `upstream`
and omit `--control-head`. Golden56 is a separate golden-role campaign with its
own exact head/log. Report each control separately; observed ranges are not CIs.

The harness strips inherited PYTHON variables/LD_PRELOAD, sets hash seed 0 and
LD_BIND_NOW, pins each child to CPU 15, checks JIT off and native method admission
off, and takes seven samples per invocation. It alternates role order per pair,
requires load below 3 before starting and below 4 after each invocation, and
retains exact commands, driver hashes, source/native/config/module identity and
pre/post equality. Sources and builds must remain untouched throughout timing.

## PMU after a complete matched campaign

`C/pyjit/issues/call-model55/capture-checkpoint-perf.py` accepts an explicit
`--candidate-head` and `--repeat`; it preserves the sealed FIFO hot interval.
Its upstream/candidate wrapper requires a **complete matched all-29 two-pair
campaign: 116 rows**, not the eight-case screen above. It verifies heads, ELF
and imported-extension build IDs, 79 candidate/78 upstream extensions, config,
hashes, native before/after and no lost records. Run only after that campaign:

```sh
HEAD59=$(git -C "$SOURCE" rev-parse HEAD)
taskset -c 15 env UV_CACHE_DIR=/home/power/.cache/uv \
  uv run --no-project python -B "$C/pyjit/issues/call-model55/capture-checkpoint-perf.py" \
  --candidate-head "$HEAD59" --repeat "$E59/results1-all29-upstream" \
  --mode stat --stat-group cycles-instructions \
  --cases comprehensions,deltablue,coroutines --output "$E59/results1-upstream-stat"
```

Three cases × three pairs × two roles gives 18 stat rows. Requested counters
must be valid and fully running; failed/not-counted events yield no result.
`--mode record` uses the same admitted interval, one pair/case and a separate
output. Never reinterpret startup-inclusive counts or exclusive profile samples
as additive causal savings. A selected57 PMU comparison needs a new task-owned
copy of the existing golden PMU wrapper with exact selected57 head/build/count
metadata; the sealed E57 script must not be edited or reused with stale HEADS.
