# Internal benchmarks

`run.py` runs the registered workloads against an explicit interpreter and writes
raw samples with loop counts, units, build identity and measurement conditions.
Follow the [measurement contract](../design/measurement.md); each experiment's
issue owns its selection, time budget and decision. The
[GC performance report](../issues/gc-cleanup-scanning/performance.md) records the
completed matched comparison and its limits.

## Workloads

- **Kernels:** 29 pure-Python workloads in `internal/kernels/`, derived from
  pyperformance with fixed loop counts. Variants share some modules; `run.py`
  defines the authoritative names and counts. Optional `setup(variant)` prepares
  data outside the timed `run(loops)` call.
- **Micro:** object access, arithmetic, calls, control flow and resume diagnostics
  in `internal/micro/`. Instruction results subtract the corresponding empty-loop
  skeleton and retain both raw values. Read each workload's body: invariant
  unused `x + y` arithmetic is not an `i += 1` recurrence, and emitted work must
  be checked before drawing conclusions about folding or dispatch.
- **Memory:** fixed live-set, allocation and collector workloads in
  `internal/memory/`. Collector units use fixed object counts across builds;
  seconds per allocated GB do not mean allocator-observed byte totals.

## Metrics

| Metric | Meaning |
| --- | --- |
| `instr` | User instructions from `perf stat`, differenced between `loops` and `3 * loops`; median of four pairs after warm-up. Micro values are per inner iteration, less the skeleton. |
| `time` | Median of five timed calls after one warm-up, normally ns per loop; collector rows report seconds per fixed allocated GB. |
| `rss` | Peak process RSS in KiB from GNU time; median of three processes. |
| `boxes` | Pystats-only differenced boxing counts; includes class/instruction breakdown and counted handler entries. |

`counted_handler_entries` measures generated target entries, not every logical
operation: predicted-label jumps and tail calls have different entry behavior.
Compare only matching counter coverage and dispatch policies. Micro diagnostics
are separate from the kernel aggregate.

Children use fresh writable bytecode caches, remove inherited `PYTHON*` settings
and set hash seed 0. Workload counts are fixed, not calibrated. Missing metric
tools or invalid samples fail the run. Timing requires a quiet reservation;
`--max-load` is an additional guard, not a reservation mechanism.

## Usage

Run the host driver through the shared uv environment; `--build` accepts the
target executable or its build directory. This example records one selected
diagnostic, not a qualification or matched timing campaign:

```sh
uv run --no-project --python /usr/bin/python3 -- python -B boil/bench/run.py \
  --build build/release --label candidate --set micro --only int_add \
  --metric instr -o build/int-add-instructions.json
```

`--set` accepts `kernels,micro,memory` or `all`; `--only` narrows names.
`--loops` overrides counts for smoke checks. `--update` adds metrics to an
existing file for the same build. `compare.py REF.json OTHER.json...` prints
ratios to the reference: below 1 means less elapsed time, instructions or memory.
State the direction explicitly; matched throughput reports may use the inverse.
Retain raw vectors, spreads and source/native/origin bindings with every report.
