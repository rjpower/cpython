# 6c rung 5: measurements and the write-up

Status: design, 2026-10-27. Specification: note 09 section 9 ("Recorded,
not gated") and item 5; boil/PROCESS.md level 4. Before: rung 4
(371c9c93078).

This rung adds no mechanism. It adds the counters that the record needs,
records gate 4 for the step, attributes the movers, and writes the step's
decisions into the notes.

## 1. Counters and the stats directory (one commit)

**Dictionary-mode entries by reason (Py_STATS).** These replace
`dict_materialized_on_request`, `_new_key`, `_too_big` and
`_str_subclass`.

- **One accounting point.** Every conversion from shape mode
  publishes through `publish_dict()`. The reason is passed down to it,
  and its counter is incremented there, once, after the header changes.
  A failed conversion counts nothing. A store to an object already in
  dictionary mode counts nothing.
- **The reasons**, in the order a caller passes them:

| reason | caller |
|---|---|
| `str_name` | a store, or a DICT-plan load, with a name that is not an exact str |
| `capacity` | an ADD refused because `nslots` is at capacity. This is checked before the shape limit, so an ADD that hits both is `capacity` |
| `shape_limit` | an ADD, or a REMOVE, refused because the type has 64 shapes. A REMOVE needs no capacity |
| `class_assigned` | `__class__` assignment, which enters dictionary mode |
| `dict_assigned` | `__dict__ = d` on an object in shape mode |
| `dict_deleted` | `del o.__dict__` on an object in shape mode |
| `on_request` | every other `_PyObject_EnterDictMode()` caller: `__dict__`, `vars()`, `PyObject_GenericGetDict`, `_PyObject_GetDictPtr()` |

- **Plumbing.** `_PyObject_EnterDictMode()` and
  `_PyObject_SetSlotDict()` take the reason, so the `__class__`
  assignment path and the str-name load pass their own reasons instead
  of counting as `on_request`. `Python/pystats.c` and
  `Tools/scripts/summarize_stats.py` print the new names.
- **Reading the counters.**
  `_testinternalcapi.dict_mode_counts()` returns them as a dict on a
  Py_STATS build and None otherwise.
- **Tests, on dev-pystats.** They skip elsewhere.
  - The full counter vector after each reason, entered once. The second
    operation on an object already in dictionary mode counts nothing.
  - An injected allocation failure during conversion counts nothing,
    including through `_PyObject_GetDictPtr()`'s suppressed error.

**The stats directory.** Py_STATS writes each process's file to
`/tmp/py_stats/`, which every user of the machine shares. User
namespaces are not permitted here (`unshare` fails on `uid_map`), so
isolation must come from the build:

- On a Py_STATS build, `PYTHON_STATS_DIR`, if set, names the directory
  instead. It is read with `getenv`, so `-E` children inherit it too.
- regrtest workers and test subprocesses inherit the environment.
- A process that clears its environment writes to the default
  directory. It is not counted, and it does not contaminate the run.
- The suite run uses its own empty directory, sums only that
  directory, and removes it.
- `boil/bench/run.py` keeps its single-child rule.

**Fixed-capacity waste is measured by sampling, not counters.** A count
of ADD commits does not measure occupancy, because one object can add
and delete without bound.

- `boil/tools/measure/occupancy.py` runs a kernel's `setup(variant)`
  and `run(loops)` under a `sys.setprofile` hook.
- Every 200,000 call events, the hook walks `gc.get_objects()` and
  reads the shape metadata (`_testinternalcapi.shape()`, which does not
  materialise `__dict__`) of each object of a type with inline values.
- **Per sample it records:** objects, slots allocated (the type's
  capacity), named slots, holes, and objects in dictionary mode. A
  dictionary-mode object counts its capacity as allocated and its
  slots as unused.
- After a `__class__` change the object is already in dictionary mode,
  and only slot 0 is meaningful, so it counts as dictionary mode.
- **The population** is defined as "live at each sample". The result is
  the sums over the samples.
- `instances_1m` is measured the same way.

**Generalisation fan-out** comes from rung 4's always-on counters,
reported as deltas over `run(loops)` alone. `setup()` and cold-start
generalisations are reported separately.
`boil/tools/measure/kernel_shapes.py` follows `driver.py`'s
`setup(variant)` contract.

## 2. Gate 4 and the recorded list

The builds are dev-default at the rung's last code commit, and
dev-pystats rebuilt at the same commit for boxes and counters. The
measurements:

- **Gate 4** (`GATE_LABEL=6c`): `--set all --metric instr,rss`, written
  to `results/6c.json`.
  - **Attribution for the step:** against `6b.json`.
  - **Attribution for rung 4:** against a new
    `results/6c-rung3-end.json`, measured on a build of a9e02dccada in a
    detached worktree (`.worktrees/boil-6c-r3base`, `CC=gcc`, removed
    at the rung's end). Rung 3's own file, `6c-rung3.json`, is kept
    unchanged; it predates the storage-marking fix. The doc records the
    new file's source revision, bench revision and configuration.
- **Boxes per loop:** `--metric boxes` on dev-pystats.
- **Wall time:**
  - `--metric time` with `--max-load`, if the machine is idle enough;
  - otherwise recorded as not measured, as in rung 3, with the load
    averages given.
- **Kernels per loop** for richards, raytrace, deltablue, chaos, go,
  nbody and json_loads. json_loads also gives the per-loop cost of the
  extra load on a container-heavy benchmark.
- **Code shape** from `objdump` of `_PyEval_EvalFrameDefault`, against 6b:
  - the `Py_TYPE` site count;
  - the masks (`movabs` of the shape mask) in the eval loop.
- **Memory:**
  - RSS for one million four-attribute instances (`instances_1m`);
  - `sys.getsizeof` of a small instance;
  - shape and plan-cache bytes per type, over the kernels' classes;
  - transient classes, as current RSS after `gc.collect()`, in three
    batches of 10,000, with a weak reference to each class checked dead.
    Growth across batches separates retention from the allocator's
    one-time reservation.

  These use rung 3's scripts, moved under `boil/tools/measure/` so that
  they are kept.
- **Micro benchmarks:**
  - `attr_load_after_dict`, `attr_store_after_dict`, `attr_load_poly`,
    `attr_store`, `attr_store_poly` and `new_instance`;
  - `str_concat_loop` against `str_concat` (the `str +=` register row).
- **Two new micro benchmarks**, with the existing rows kept for history:
  - `attr_load_poly5`: one site, five shapes, which is past the
    four-row table, so megamorphic;
  - `attr_store_boxed`: a monomorphic store of a str into a BOXED
    field, the vacuous guard of rung 4.

  Each is checked to warm into the intended handler (generic
  `LOAD_ATTR`; `STORE_ATTR_SHAPE` with a BOXED field) before it is
  measured. The baseline is measured with the same benchmark sources.
- **Shape-cap exhaustion:** instructions per store for an object whose
  type has used its 64 shapes, against one in shape mode, from a
  `perf stat` pair on a small script.
- **Generalisation fan-out:** generalisations and shapes re-versioned
  per kernel, from a wrapper that runs each kernel's `run()` and prints
  the counters.
- **Dictionary-mode entries by reason:**
  - over the benchmark set;
  - over the suite (`PYTHONSTATS=1` with its own `PYTHON_STATS_DIR`,
    the pystats build, `-j` as gate 2), summed with
    `summarize_stats.py`.
- **Fixed-capacity waste:** occupancy by sampling (section 1), over the
  kernels with instances and `instances_1m`.

**Attribution.** A move of 3% or more in instructions, or 5% or more in
RSS, gets a demonstrated cause: a profile (`perf record`) against the
previous build, or a counter.

- An explanation that is not demonstrated is labelled a hypothesis and
  goes in the hand-off list with its numbers.
- Without wall time, instructions are not used to claim anything about
  the latency of dependent loads.
- Nothing is tuned (coordinator rule).

## 3. The write-up (one commit, boil/ only)

- **Note 06 section 20, "Step 6c decisions"**, in section 19's form:
  - the rulings with dates;
  - a commit table per rung;
  - the measurements with their attribution;
  - the hazards found (test_buffer's traverse, the dict word, the
    `_testcapi` heap types);
  - the hand-off list, each item with its numbers and its owner step,
    so that the next design agent reads them from the plan (coordinator,
    2026-10-27). The items include go's +24% (polymorphic stores), the
    `FOR_ITER_RANGE` register spill, logging's conversion cost, and
    `str +=`.

  It gathers the work docs' decision sections. Rung 1's doc already
  marks its decisions for section 20.
- **Note 09:**
  - Section 4's plan table names `LOAD_ATTR_NONDESCRIPTOR_SHAPE` for
    CLASS_VALUE on an inline-values type (rung 3, decision 2).
  - Section 5 gains the version-pairing rule (rung 4, ruling 1).
  - Section 5 gains the never-dereference invariant for borrowed
    shapes.
- **Note 06, sections 3.3 and 6:** brought up to date. In section 3.3,
  kinds are promises generalised in place, not shape transitions, and
  members are not shapes. Section 6's deletion list matches the
  handlers note 09 keeps.
- **Note 08 section 3:**
  - The managed-dict pre-header row is retired by 6c, with its proof
    measured: `sys.getsizeof` and the bin of a minimum-capacity plain
    instance.
  - The `str +=` row stays (ee167b42a18).
  - "object slots box" stays for 6d.

## 4. Order and gates

1. The counters commit:
   - gate 1 on the ladder on dev-default and dev-debug;
   - gate 1 on dev-pystats with test_shapes, which runs the counter
     tests, and a `-X pystats` dump through `summarize_stats.py`;
   - then gate 2.
2. The measurements, then their results files and the "Results" section
   of this doc. Measurement commits change no code.
3. The write-up commit.
4. Gate 3 at the last code commit, the counters; the later commits touch
   boil/ only.

## Risks

- **Load and wall time.** Other agents load the machine (10a runs
  beside this rung), so wall time may be unmeasurable, as in rung 3.
  Instructions and RSS are insensitive to load.
- **Suite volume under Py_STATS.** There is one file per process, in
  the run's own `PYTHON_STATS_DIR` (section 1). It is removed after it
  is summed.
- **The rebuilt rung 3 baseline** needs a temporary worktree
  (`boil/tools/worktree.sh`), removed when the rung ends.

## Review

The review (codex, 2026-10-27, `reviews/6c-rung5-review.md`) gave the
verdict "revise". The coordinator had already accepted the plan,
adding the hand-off list in note 06 section 20. Each blocking issue:

1. **The counters had no single accounting point.** Accepted. The
   reason is passed to `publish_dict()`, which counts it once, after a
   successful publication. There is an order of precedence, and dict
   deletion has its own reason. The tests check the full vector, a
   repeated operation, and allocation failure.
2. **"Slots used" was not occupancy.** Accepted. The ADD counter is
   dropped. Occupancy is sampled over a defined live population, with
   holes, dictionary mode and `__class__` changes stated (section 1).
3. **Stats isolation by listing a shared directory.** Accepted.
   `PYTHON_STATS_DIR` replaces the before-and-after listing, because
   namespaces are not permitted on this machine.
4. **The five-shape and BOXED-store cases were missing.** Accepted:
   `attr_load_poly5` and `attr_store_boxed` are added, and each is
   checked for its warmed handler before it is measured.

Non-blocking:

- **Gating the Py_STATS tests.** Gate 1 now runs on dev-pystats.
- **Note 06 sections 3.3 and 6.** Added to the write-up.
- **Attribution wording.** Tightened.

Questions:

- **Transient classes.** Measured in batches with weak-reference checks.
- **Fan-out.** Reported as deltas over `run()`, with `setup()` reported
  apart.
- **Rung 3's results.** `6c-rung3.json` is kept; the new baseline is a
  separate file with its revision recorded.

## As built

Commits:

- f70da2cad2a: the counters, `PYTHON_STATS_DIR`, the two micro
  benchmarks and the measurement scripts.
- The write-up: note 06 section 20, notes 08 and 09, the results files
  `6c-rung3-end.json`, `6c.json` and `6c-pystats.json`, and this
  section.

The measurements and their attribution are in note 06 section 20.8, and
the hand-off with owners is in 20.9. Where the work differs from the plan
above:

- **Gates at f70da2cad2a.** Gate 1 passed on the ladder (dev-default
  and dev-debug, 10,304 tests each) and on dev-pystats (test_shapes and
  test_sys). Gate 2 passed (50,884 tests). Gate 3 failed on
  test_io.test_signals, which timed out: it is 10a's known hazard, whose
  fix lands with the 10a merge. gate.sh exits at a failed command, so
  test_cext with `-u cpu`, the regen check and the traverse audit were
  run by hand, and passed. Gate 4 passed (`results/6c.json`).
- **The new micro benchmarks' handlers** were checked on the rung 3 end
  build after warming: `attr_load_poly5`'s site is generic `LOAD_ATTR`,
  and `attr_store_boxed`'s is `STORE_ATTR_SHAPE`. On the current build,
  `attr_store_boxed` costs what `attr_store` costs (52 instructions per
  store), which the generic store (283) would not.
- **Attribution used exact counts where sampling could not.** perf
  throttles at 20,000 samples a second, so the store profiles sample
  every 2M instructions. Specialised-store counts come from Py_STATS
  execution counts, multiplied by the guard's eight instructions as
  disassembled.
- **The rung 3 RSS items.** Rerunning showed tuples_10m and
  collector_live512 to be bimodal within one build, so they are retired
  rather than handed on.
- **The occupancy tool** counts the import system's objects too, so it
  reports the kernel's own classes separately.
- **Wall time** was not measured, because gates ran on the machine
  throughout.
- **The coordinator's addition**, the capacity floor, is in note 09's
  recorded list and in note 06 section 20.9, item 4.
