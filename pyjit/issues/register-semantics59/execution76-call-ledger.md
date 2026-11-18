# Execution76: call/return cost ledger

Analysis of round 2 (`f78c3902c1b`) against upstream `3a440b65695`,
2026-11-18. It changes no source. Evidence: `E59/execution76-call-ledger`.

## Method

Per-call instruction counts are perf-stat differences between two loop
counts, minus a loop-only control. gdb single-stepped whole loop iterations
and mapped each instruction to its handler, micro-op region and source line.
The traces agree with perf stat to within one instruction, so the ledgers are
exact counts. Kernel figures come from AMD IBS op samples, which are not
subject to the skid that misattributes ordinary samples. They are estimates
(±10–25%), because the kernel caps the sample rate and IBS counts macro-ops.

## Per call

| Case | Ours | Upstream | Ratio |
| --- | ---: | ---: | ---: |
| `f(x, y)` | 455 | 343 | 1.33 |
| `o.m(x)` | 423 | 334 | 1.27 |
| Default argument (`CALL_PY_GENERAL`) | 523 | 479 | 1.09 |
| Keyword argument (`CALL_KW_PY`) | 760 | 558 | 1.36 |
| `o.store(x)`, returns None | 540 | 403 | 1.34 |
| Class with a Python `__init__` | 1,633 | 1,204 | 1.36 |
| Callee whose local dies at return | 1,004 | 699 | 1.44 |
| Polymorphic call site | 860 | 541 | 1.59 |

On the exact shapes, ours takes **fewer cycles per call** than upstream (89
against 95 for `f(x, y)`, 80 against 104 for `o.m(x)`). Trimming the exact
sequence alone therefore should not speed those loops. The losses in the
kernels come from the slow and generic paths and from front-end pressure.

For `f(x, y)` the 112 extra instructions are:

- 52 for argument copies into the call window (36 per `LOAD_FAST_BORROW`);
- 30 for frame retirement (108 against 78);
- 17 for loading the global;
- 13 for the call itself, from handler entry, operand decoding and
  `call_transaction_begin`. Frame setup is 5 cheaper than upstream's.

## In the kernels

DeltaBlue's call/return/frame work is about 20.2M instructions per loop
against upstream's 11.8M. The excess is 21.5% of upstream's total:

| Path | Excess per loop |
| --- | ---: |
| Return and retirement | 3.4M |
| Exact-call handler | 1.8M |
| Generic Python call | 1.6M |
| Native call | 1.1M |

Call code occupies 81 of our 372 hot lines against 48 of upstream's 256. 48 of
our 81 belong to the generic and native helpers, which carry only about 19%
of call instructions.

Raytrace's excess is about 331M instructions per loop (11.3% of upstream), of
which 269M is return and retirement. Dying `Vector` temporaries send returns
to the slow path, which costs 218 more instructions than upstream's.
Python operator methods reached through C slots (`slot_nb_subtract`, ...) add
about 91M that no microbenchmark covers.

## Consequences

Round 3 (`pyjit/calls76`) takes the local items:

1. One thread-only retirement function.
2. Shadow retirement computed only when observed.
3. No second admission pass on the generic path when the callee takes no
   `**kwargs`.
4. Only the native-call refreshes that follow a real callback or allocation.
5. A cheaper exact-call entry. Self/no-self stays a word flag rather than a
   handler variant.
6. Canonical stores for immortal constants.

Three items are deferred. A leaner normal-return root record changes the
boundary73 retirement protocol. Coalescing arguments into the call window is a
compiler or frame-ABI change. Certifying the `__init__` shim is moderate risk.

The child frame holds its own count on each argument while the caller keeps
its own, about 6 instructions per heap argument more than upstream. The values76
rule "callers own, callees borrow" covers that case, but applying it to frame
bindings changes the frame ABI: bindings can be rebound, observed through
`f_locals` and copied into generators. That decision belongs to values76.
