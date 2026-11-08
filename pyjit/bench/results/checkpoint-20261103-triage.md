# Performance triage of checkpoint `dbce7b1260d`

2026-11-03. Candidate C: `build/checkpoint-release-dbce7b1260d`. Upstream U:
`build/upstream-release-3a440b6`. Interim I: `build/rc-integrated-release-20261101`
(`eba351fee7d`). Fix F: `build/triage-perf-fix`, built from the worktree
`build/worktrees/triage-perf` (`dbce7b1260d` plus `perf-triage-fix.diff`) with the
same configure line as C (`CC=gcc --with-mimalloc --with-computed-gotos --with-shapes`).

Other agents loaded the machine (1-minute load 0.7 to 15 during this work).
Compare ratios, not absolute nanoseconds. Every timing comparison below
interleaves the builds, so they share the same conditions. The final loop and
kernel tables were measured at load 0.7 to 1.3.

## Summary

**Root cause.** The checkpoint is not a JIT-off build. Commit `3bcb891956a`
("Add natural hot-call and backedge method admission"), which landed after the
interim, admits every eligible function (anything but a generator or
coroutine) to the fork's method JIT after 256 backward jumps or 64 calls
from one call site. The admitted code is the M0
*baseline* (`Python/jit_baseline.c`). It compiles each word to a call to the
generated helper for the word's **generic, unspecialised** handler
(`_PyCode_GenericHandler`), followed by a call to `_PyJit_AfterHelper`. Once a
function is admitted, it loses every interpreter specialisation. `FOR_ITER_RANGE`
runs as `FOR_ITER`, which boxes each index as a `PyLong` and then frees it.
`BINARY_OP_MULTIPLY_FLOAT` runs as `BINARY_OP_MULTIPLY`, which boxes both floats
and calls `PyNumber_Multiply`. `CALL_LEN` runs as a generic `CALL` transaction.
Every word also pays `PyErr_Occurred`, a revalidation of the unit, and
`_PyJit_ObserversAllow`, which loops over all monitoring events. Quickening
still happens, which is why `dis(adaptive=True)` looks right, but the native
code never reads the quickened words.

**Why while loops differ.** The observed-loop optimiser (`_PyJit_BuildGraph`,
`Python/jit_ir.c`) lowers only int32 `BINARY_OP_ADD/SUBTRACT/MULTIPLY/AND`,
`COMPARE_OP` followed by a conditional jump, small-int loads and the backedge
poll. `while i < n: i = i + 1` consists entirely of those words, so it becomes
native code that runs 7 times faster than upstream. Every other word, including
`FOR_ITER`, every call and every float operation, remains a generic helper call.

**Why the checkpoint report missed it.** `_PyJIT_BASELINE_SUPPORTED` is 0 when
`Py_STATS` is defined (`Include/internal/pycore_jit.h`). The pystats build that
supplied the dispatch counts therefore ran the interpreter. The release build
spent its time in JIT helpers. The report's "JIT-off" check used `sys._jit`,
which covers only the upstream copy-and-patch JIT.

**Fix.** Natural admission is now off by default. A test hook,
`_testinternalcapi.jit_method_natural(flag)`, turns it on. The JIT test modules
turn it on for their whole module. Explicit `jit_method_compile` is unchanged.

**Share.** On the 29 kernels with interleaved runs at load 0.7 to 1.3,
C/U falls from 4.64 to 2.60 (geometric means). I/U is 1.21 and C/F is 1.79.
The fix removes 38% of the log slowdown against upstream and 43% of the
regression against the interim (log 1.79 / log(4.64/1.21)). F remains 2.15
times slower than I. On the microbenchmarks it removes 5- to 27-fold
pathologies. The rest of the slowdown has a second, distributed cause in the
PyValue interpreter itself (section 6).

## 1. Profiler

`perf-triage-sampler.c` is an LD_PRELOAD library. It arms
`setitimer(ITIMER_PROF)` (default 200 µs), records `REG_RIP` from the
`ucontext`, and optionally walks up to 7 frame pointers (all builds use
`-fno-omit-frame-pointer`). It dumps raw PCs and `/proc/self/maps` at exit.
`perf-triage-symbolize.py` symbolises the PCs with `nm`.
`perf-triage-handlers.py` assigns interpreter-loop PCs, and the out-of-line
callees beneath them, to IR handlers through `addr2line -i` and the `TARGET()`
lines of `generated_ir_cases.c.h`.

```
gcc -O2 -shared -fPIC -o sampler.so perf-triage-sampler.c
LD_PRELOAD=./sampler.so SAMPLER_OUT=/tmp/x.out SAMPLER_DEPTH=4 python script.py
python3 perf-triage-symbolize.py /tmp/x.out.<pid> 15 --callers
```

## 2. Candidate profiles (top 15 self symbols)

Probes: `for i in range(20_000_000): pass`; a 200-iteration `s = s + x * 2.0`
for loop, called 30,000 times; and `len("abc")` in a 3M for loop.

| empty for loop | | float multiply-add | | `len("abc")` | |
|---|---|---|---|---|---|
| `jit_helper_FOR_ITER` | 20.4% | `PyErr_GetRaisedException` | 9.2% | `PyErr_GetRaisedException` | 10.8% |
| `_PyJit_AfterHelper` | 13.6% | `_PyJit_AfterHelper` | 8.6% | `value_operation_refresh` | 8.6% |
| `value_operation_refresh` | 8.7% | `PyErr_Occurred` | 8.2% | `PyErr_Occurred` | 6.7% |
| `_PyJit_ObserversAllow` | 8.0% | `value_reader_complete` | 7.0% | `_PyJit_AfterHelper` | 5.5% |
| `PyErr_Occurred` | 7.8% | `_PyJit_ObserversAllow` | 6.2% | `call_transaction_invoke` | 5.0% |
| `PyErr_GetRaisedException` | 6.6% | `_PyValueTemps_CloseNonempty` | 5.3% | `call_transaction_begin` | 4.6% |
| `jit_helper_JUMP_BACKWARD` | 6.4% | `jit_helper_BINARY_OP_MULTIPLY` | 4.8% | `_PyJit_ObserversAllow` | 4.4% |
| `_PyValueTemps_PublishObject` | 6.0% | `PyErr_SetRaisedException` | 4.2% | `call_transaction_refresh` | 4.1% |
| `PyErr_SetRaisedException` | 4.3% | `jit_helper_BINARY_OP_ADD` | 3.8% | `_PyNativeScope_End` | 3.9% |
| `_PyValueTemps_CloseNonempty` | 3.9% | `_PyValue_RestoreException` | 3.7% | `_PyNativeScope_BeginVector` | 3.6% |
| `long_dealloc` | 2.7% | `value_operation_refresh` | 3.6% | `_PyCFunction_CallScoped` | 3.1% |
| `rangeiter_next` | 2.1% | `jit_helper_FOR_ITER` | 3.6% | `jit_helper_FOR_ITER` | 3.1% |
| `_PyLong_FromMedium` | 1.9% | `_PyValueTemps_Box` | 3.5% | `PyErr_SetRaisedException` | 3.1% |
| `_Py_Dealloc` | 1.5% | `_PyValueTemps_PublishObject` | 2.4% | `_PyValue_RestoreException` | 2.4% |
| `_PyValue_RestoreException` | 0.9% | `value_binary_operation` | 2.3% | `jit_helper_LOAD_GLOBAL` | 2.2% |

The callers show that the generic helpers come from anonymous JIT code pages
(`?0x7086…`) under `PyEval_EvalCode`. The float loop boxes through
`value_binary_operation → _PyValueTemps_Box → _PyValue_BoxInlineValue →
PyFloat_FromDouble` and frees through `float_dealloc`. The empty loop creates a
`PyLong` per index (`_PyLong_FromMedium`) and frees it (`long_dealloc`).
`_testinternalcapi.jit_method_info` confirms the admissions. For example,
`empty` reports `active: True, compile_attempts: 2, trace.optimized: False`,
while `whileloop` reports `trace.optimized: True` with a five-word native path.

After the fix, the same probes take 9 to 15 times fewer samples (188, 260 and
139 against 2074, 2830 and 2074), and `_PyEval_EvalFrameLoop` accounts for 75%
to 91% of them.

## 3. Mechanism, with code locations

1. Seeding. `make_exec` (`Python/ir.c:275`) and the end of `_Py_Instrument`
   (`Python/instrumentation.c:1986`) call `_PyJit_SeedBackedges`, which rewrites
   every real backward `JUMP_BACKWARD` to `JUMP_BACKWARD_JIT_WARM`. In
   `Python/specialize.c:2230`, after a call site specialises successfully, the
   specialiser overwrites the specialised handler with `CALL_JIT_WARM` whenever
   `_PyJit_AutoEligible(callable)` is true.
2. Admission. `_PyJit_WarmBackedge` counts `loop_heat` to 256, and
   `_PyJit_PrepareCall` counts `fb->h.hits` to 64. Each then calls
   `automatic_compile`, which runs `_PyJit_CompileCode`.
3. Baseline. `build_method` (`Python/jit_baseline.c:60`) uses
   `_PyCode_GenericHandler(code, i)` for every word. `generated_jit_helpers.c.h`
   provides helpers only for 84 generic handlers and has none for specialised
   forms. `Python/jit_backend_x86.c:379` emits a call to `_PyJit_AfterHelper`
   after each helper.
4. Per-word tax. `_PyJit_AfterHelper` (`Python/jit_runtime.c:641`) calls
   `PyErr_Occurred`, rechecks exec, unit, generation, epoch and current frame,
   and calls `_PyJit_ObserversAllow` (`Python/ceval.c:1526`). That function
   tests PEP 523, tracing, profiling and code monitoring, and loops over all
   `_PY_MONITORING_UNGROUPED_EVENTS` tool masks. The generic helpers then run
   the object bridge (`value_operation_refresh`, `value_reader_complete`), whose
   exception save and restore costs about three out-of-line calls.
5. Native loops. `_PyJit_BuildGraph` (`Python/jit_ir.c:102`) lowers only the
   scalar int32 subset. A for loop always retains `FOR_ITER` as a helper.
   `intadd` records an optimised path, but `s` soon leaves int32, so its guards
   fail and it runs the helpers (263 ns against 46 ns upstream).

The coordinator's hypotheses: (a) is partly right. The method JIT compiles
both kinds of loop, but only the scalar while loop becomes native. Admission is
not repeated, the eval breaker is not stuck, and `_Py_HandlePending` does not
appear in any profile. (b) is the second cause (section 6).

## 4. The fix

`perf-triage-fix.diff` (6 files, +57 −19):

- `Python/jit_runtime.c` adds a process-wide `natural_admission` flag, default 0,
  next to the existing process-wide build lock and bootstrap cooldown. When the
  flag is off, `_PyJit_SeedBackedges` arms no backedges and `_PyJit_AutoEligible`
  returns 0, so the specialiser keeps its specialised call. Sites armed while the
  flag was on fall back on their next execution: `_PyJit_WarmBackedge` restores
  `JUMP_BACKWARD` without allocating JIT state, and `_PyJit_PrepareCall` restores
  the generic `CALL`, which then re-specialises. `_PyJit_SetNaturalAdmission(int)`
  returns the previous setting.
- `Modules/_testinternalcapi.c` adds `jit_method_natural(flag)`, and
  `Include/internal/pycore_jit.h` declares the setter.
- `Lib/test/test_jit_method.py` and `Lib/test/test_pyvalue_native_scope.py` turn
  admission on in `setUpModule` and restore it with `addModuleCleanup`, so they
  test what they tested before.
- `Lib/test/test_dis.py` removes `JIT_WARMS_CALLS` and its `platform` and
  `sysconfig` imports. Without admission, the quickened call is
  `CALL_PY_GENERAL` on every build, as it is upstream.

Trade-off: the native scalar while loop goes too. `whileloop` moves from 0.14×
to 1.6× upstream. Admission should return only when the baseline runs at least
as fast as the interpreter, for example once it compiles the quickened handler
and deoptimises to the generic one, or once it leaves words it cannot improve
to the interpreter.

Tests on F:
- `test_grammar test_int test_float test_list test_dict test_generators -j8`:
  all 6 pass (468 tests), as on C.
- `test_monitoring test_sys_settrace test_dis test_opcache test_generated_cases`
  pass. `test_dis` passed only after the expectation change above.
- `test_jit_method test_pyvalue_native_scope`: the same 6 failures as C, all in
  `test_pyvalue_native_scope` (`NativeGenericVMTests` and
  `NativeScopeDispatchTests`), identical by name. They predate this change.
- Full suite: see section 7.

## 5. Before and after

### Loops, `spec_probe.py`, `len` and call (ns per iteration, median of 3 interleaved runs)

| probe | U | I | C | F | I/U | C/U | F/U |
|---|---|---|---|---|---|---|---|
| empty | 10.7 | 5.0 | 74.1 | 7.7 | 0.47x | 6.93x | 0.72x |
| intadd | 45.9 | 49.5 | 263.1 | 173.1 | 1.08x | 5.73x | 3.77x |
| whileloop | 18.0 | 18.8 | 2.5 | 28.8 | 1.04x | 0.14x | 1.60x |
| straight | 27.3 | 45.2 | 229.5 | 54.4 | 1.66x | 8.41x | 1.99x |
| spec_probe | 31.1 | 20.9 | 394.7 | 36.1 | 0.67x | 12.71x | 1.16x |
| len | 17.3 | 21.6 | 466.0 | 36.5 | 1.25x | 26.94x | 2.11x |
| call | 27.4 | 48.2 | 125.0 | 53.0 | 1.76x | 4.56x | 1.93x |

Measured at load 0.7. `spec_probe` is `/tmp/pyjit-coord/spec_probe.py` without
its `dis.dis` call: I's `Lib/` has been deleted, so I falls back to
`/home/power/code/cpython/Lib`, whose `dis` does not import. An earlier run at
load 2.7 gave the same picture: C/U 6.7, 5.7, 0.13, 8.3, 14.3, 25.0 and 4.1;
F/U 0.71, 3.71, 1.49, 1.96, 1.37, 1.96 and 1.85.

`intadd` stays at 3.8× upstream after the fix. Its sum leaves the int32 range,
and the generic `value_binary_operation` path then costs about 170 ns against
upstream's 46 ns. That cost belongs to cause 2.

`len` and call use `timeit -n 2000000 -r 3` (best of 3): `len("abc")`, and
`f()` with `def f(): pass`.

### 29 kernels (`pyjit/bench/run.py --set kernels --metric time`, interleaved U, I, C, F per kernel; load 0.7 to 1.3)

| kernel | U ms | I ms | C ms | F ms | I/U | C/U | F/U | C/F |
|---|---|---|---|---|---|---|---|---|
| richards | 61.49 | 69.11 | 505.87 | 186.68 | 1.12 | 8.23 | 3.04 | 2.71 |
| go | 119.89 | 153.50 | 1,609.90 | 365.54 | 1.28 | 13.43 | 3.05 | 4.40 |
| nbody | 103.49 | 128.49 | 1,494.43 | 190.82 | 1.24 | 14.44 | 1.84 | 7.83 |
| float | 81.66 | 100.89 | 925.73 | 167.17 | 1.24 | 11.34 | 2.05 | 5.54 |
| spectral_norm | 105.92 | 105.68 | 1,566.19 | 202.46 | 1.00 | 14.79 | 1.91 | 7.74 |
| raytrace | 330.81 | 358.48 | 3,279.58 | 810.56 | 1.08 | 9.91 | 2.45 | 4.05 |
| deltablue | 3.82 | 4.85 | 65.18 | 17.17 | 1.27 | 17.06 | 4.49 | 3.80 |
| chaos | 65.49 | 78.36 | 632.48 | 174.29 | 1.20 | 9.66 | 2.66 | 3.63 |
| nqueens | 90.26 | 118.55 | 203.89 | 197.49 | 1.31 | 2.26 | 2.19 | 1.03 |
| fannkuch | 413.66 | 483.40 | 1,026.09 | 1,062.54 | 1.17 | 2.48 | 2.57 | 0.97 |
| generators | 32.64 | 45.44 | 87.62 | 84.99 | 1.39 | 2.68 | 2.60 | 1.03 |
| comprehensions | 0.02 | 0.02 | 0.06 | 0.06 | 1.37 | 3.27 | 3.03 | 1.08 |
| coroutines | 25.15 | 38.61 | 49.76 | 55.55 | 1.53 | 1.98 | 2.21 | 0.90 |
| unpack_sequence | 0.01 | 0.03 | 0.04 | 0.04 | 2.12 | 2.71 | 2.87 | 0.95 |
| deepcopy | 0.32 | 0.40 | 1.70 | 1.18 | 1.25 | 5.38 | 3.73 | 1.44 |
| deepcopy_reduce | 0.00 | 0.00 | 0.02 | 0.01 | 1.21 | 4.46 | 3.45 | 1.29 |
| deepcopy_memo | 0.03 | 0.04 | 0.28 | 0.15 | 1.33 | 9.69 | 5.33 | 1.82 |
| json_dumps | 11.89 | 13.13 | 43.02 | 29.47 | 1.10 | 3.62 | 2.48 | 1.46 |
| json_loads | 0.56 | 0.61 | 1.59 | 1.42 | 1.09 | 2.82 | 2.51 | 1.12 |
| pickle | 0.28 | 0.28 | 0.52 | 0.45 | 1.01 | 1.86 | 1.59 | 1.17 |
| unpickle | 0.34 | 0.35 | 0.63 | 0.60 | 1.03 | 1.83 | 1.75 | 1.05 |
| regex_compile | 180.72 | 212.54 | 679.38 | 446.18 | 1.18 | 3.76 | 2.47 | 1.52 |
| logging_format | 0.08 | 0.09 | 0.45 | 0.23 | 1.07 | 5.61 | 2.83 | 1.98 |
| logging_silent | 0.00 | 0.00 | 0.01 | 0.00 | 1.23 | 6.61 | 3.61 | 1.83 |
| logging_simple | 0.07 | 0.09 | 0.43 | 0.21 | 1.19 | 5.89 | 2.84 | 2.07 |
| xml_etree_generate | 135.24 | 147.92 | 357.19 | 333.88 | 1.09 | 2.64 | 2.47 | 1.07 |
| xml_etree_iterparse | 107.27 | 122.99 | 238.04 | 219.95 | 1.15 | 2.22 | 2.05 | 1.08 |
| xml_etree_parse | 164.21 | 170.96 | 256.99 | 256.74 | 1.04 | 1.56 | 1.56 | 1.00 |
| xml_etree_process | 88.54 | 103.72 | 262.74 | 246.90 | 1.17 | 2.97 | 2.79 | 1.06 |
| **geomean (29)** | | | | | **1.21** | **4.64** | **2.60** | **1.79** |

Raw values are in `perf-triage-kernels-interleaved.json`. An earlier
interleaved run of U, C and F at load 6 to 14
(`perf-triage-kernels-interleaved-loaded.json`) gave C/U 4.62, F/U 2.55 and
C/F 1.81. The checkpoint measured C/U 4.7, and the coordinator measured I/U
1.23. As the checkpoint report notes, I's own `Lib/` has been deleted, so it
runs with `/home/power/code/cpython/Lib`.

The fix matters most where hot code is admitted: numeric and object loops
(nbody 7.8×, spectral_norm 7.7×, float 5.5×, go 4.4×, raytrace 4.1×), plus
richards, deltablue and chaos (2.7 to 3.8×) and the logging kernels (about
2×). Generators, coroutines and comprehensions are never admitted, and the
C-heavy kernels (xml_etree, pickle) run little hot Python code, so they do not
change. In those kernels C/U is still 1.6 to 3.3, against
I/U of 1.0 to 1.5. That gap belongs to the second cause.

## 6. Ranked causes

| Rank | Cause | Measured share |
|---|---|---|
| 1 | Natural method-JIT admission into the generic-helper baseline | C/F 1.79× geomean. 38% of the log slowdown against U (log 1.79 / log 4.64); 43% of the regression against I. 5-27× on the probes. |
| 2 | Per-word cost of the PyValue interpreter itself | the remaining F/U 2.60× (F/I 2.15; 62% of the log slowdown). Split below. |

Split of cause 2. Each kernel was profiled on F and on U for about 1.2 s, and
the F−U time gap was divided by self-symbol category: (F·fraction_F −
U·fraction_U) / (F − U), with F and U per-loop times from the loaded
interleaved run. Means over the 29 kernels; per-kernel values are in
`perf-triage-gap-decomposition.json`.

| Category | Share of the F−U gap | Share of F time |
|---|---|---|
| Interpreter-loop self time (`_PyEval_EvalFrameLoop` against `_PyEval_EvalFrameDefault`) | 39% | 39% |
| Exception save/restore (`PyErr_GetRaisedException`, `PyErr_SetRaisedException`, `_PyValue_RestoreException`, `PyErr_Occurred`) | 25% | 14% |
| PyValue bridge and boxing (`value_*`, `_PyValueTemps_*`, `_PyValue_*`, `call_transaction_*`, `_PyNativeScope_*`, `_PyBorrow_*`) | 17% | 10% |
| Other (object work, allocation, dicts) | 14% | 31% |
| Attribute lookup (generic getattr, type cache) | 3% (richards 11%, deltablue 15%) | 3% |
| Frames | 2% | 3% |

**2a, interpreter-loop self time.** The time is spread thinly over the
handlers. In `straight` (ten `b = a` per iteration, each one `LOAD_FAST` word),
a copy costs about 4.7 ns against 1.7 ns upstream. Its machine code (at
`_PyEval_EvalFrameLoop+0x14a5f` in F) keeps `next_instr`, `frame` and `slots` on
the stack (`-0x1530`, `-0x1568`, `-0x1578(%rbp)`) and does not hold them in
registers. Their addresses escape: for example, `value_operation_refresh(...,
_PyInstr **next, ...)` receives `&next_instr`. Each word also initialises the
four-field `_PyValueTemps` record in memory, reads `co_nlocalsplus` for the
`IR_IS_LOCAL` test in `IR_STORE_DST`, tests `PyLong` and `PyFloat` in
`_PyValueTemps_Publish`, and checks `pending_local` and `count` before
dispatch: about 50 instructions per word on the inline-float path.

**2b, exception bracketing.** The universal-value commits wrap retirements in
`saved = PyErr_GetRaisedException(); Py_XDECREF(x); _PyValue_RestoreException(saved);`.
This happens even when the decref cannot deallocate and no exception is
pending. `_PyValue_RestoreException(NULL)` itself calls
`PyErr_GetRaisedException` and then `PyErr_SetRaisedException`. In six files
(`dictobject.c`, `object.c`, `enumobject.c`, `typeobject.c`, `ceval.c`,
`pyvalue.c`), the sites grew from 15 at `eba351fee7d` to 371 at `dbce7b1260d`.
The heaviest callers vary by kernel: `dict_setdefault_object_owned`,
`_PyValue_Hash` and `insertdict` in json_loads; `enum_next` and
`_PyForIter_VirtualIteratorNext` in spectral_norm; `native_call_finish` in
float and deepcopy; generic getattr and `value_reader_complete` in richards.

**2c, bridge and boxing.** Examples: generic object-bridge transactions
(`value_operation_refresh`, `call_transaction_*`, `_PyNativeScope_*`);
inline ints boxed for slices in fannkuch (`STORE_SLICE`,
`BINARY_OP_SUBSCR_LIST_SLICE` and `BUILD_SLICE` account for 49% of its time);
polymorphic attribute loads deoptimising into the generic `LOAD_ATTR` (50% of
richards' samples, 37% of deltablue's). On a four-class polymorphic `o.x`, F
costs 133 ns per load against 38 ns upstream, and exception save/restore
accounts for 29% of that loop's samples.

Proposed next steps (not done here, since each needs a design decision):
- 2b: bracket only when the retirement can deallocate (`_PyValue_MayDealloc`
  already exists), and return early from `_PyValue_RestoreException(NULL)`.
- 2a: keep `next_instr` and `frame` in registers. Out-of-line helpers should
  return the new PC, not write through `_PyInstr **`. Create the
  `_PyValueTemps` record only in handlers that box.

## 7. Full test suite on F

`./python -m test -j14` (2 min 10 s): 513 test files OK, 4 failed: `test_capi`,
`test_pyvalue`, `test_pyvalue_dict_vm` and `test_pyvalue_native_scope`. All four
fail on C as well. `test_capi` and `test_pyvalue` abort on the same assertion in
the test module on both builds (`_testinternalcapi.c:3217`
`pyvalue_internal_contract: Assertion !PyValue_IsLanguageValue(invalid[i])`).
The other two have the same 10 named failures on C and F (compared with
`diff`): 4 in `test_pyvalue_dict_vm` and 6 in `test_pyvalue_native_scope`.

## Files

- `perf-triage-fix.diff`: the fix against `dbce7b1260d`.
- `perf-triage-sampler.c`, `perf-triage-symbolize.py`, `perf-triage-handlers.py`:
  the profiler and its analysis scripts.
- `perf-triage-kernels-interleaved.json` (load 0.7 to 1.3, with I) and
  `perf-triage-kernels-interleaved-loaded.json` (load 6 to 14): raw kernel
  timings in ns per loop.
- `perf-triage-gap-decomposition.json`: the per-kernel split of cause 2.
- `progress-perf-triage.txt`: the progress log.

The build directory `build/triage-perf-fix` remains. Its source worktree has
been removed, so to run or rebuild it, recreate the worktree at the same path:
`git -C /home/power/code/cpython worktree add --detach build/worktrees/triage-perf dbce7b1260d`,
then apply `perf-triage-fix.diff` with `git apply`.
