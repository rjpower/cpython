# Jit88: the baseline JIT's B0 and B1

Approved 2026-11-23. The user promoted contract87's integration as golden87
(`684a98eb30e`, tag `pyjit-golden-20261123-contract87`, branch
`pyjit/round87-golden`) and approved Phase 2 of
[tiers87](../../design/proposals/tiers87.md): a Sparkplug-style baseline
method JIT. The user asked for an update once a baseline version of the JIT
has performance numbers.

Base: golden87. Integration branch `pyjit/jit88`. Evidence `E59/jit88/`:
`coordinator.md` is the log, `common.md` and `brief-*.md` are the briefs,
and `<unit>/` holds each unit's design, progress log and report. The gates
are correctness under `PYTHON_JIT=always` and with the JIT off, and root's
review of each diff. Performance is recorded, not gated.

## Units

| Unit | Step | Work | Done when |
| --- | --- | --- | --- |
| JA | B0 | An x86-64 assembler with labels and fixups (general, SSE2 scalar double, label addresses as data) and W^X code memory reserved within ±2 GiB of the interpreter's text, so that compiled code calls C with `call rel32` | every instruction form matches GNU `as` byte for byte; placed code runs |
| JB | B0, B1 | Code units on the exec with a word-to-native entry table; `-X jit=off\|on\|always` and `PYTHON_JIT`; warm-up; the perf map; every word compiled as a call of its function form, with native branches; calls, returns, generators and exceptions through the activation loop; monitoring retires units | the common ladder passes with `PYTHON_JIT=always`, release and debug |

JA delivers the subset that JB needs first, so that JB designs and builds
the runtime side meanwhile. JB's design note answers how compiled code
follows the interpreter's specialization (the handler column is read at
compile time, so a respecialized word must reach compiled code) and how
uncompiled frames run while the JIT is on.

## Integration

On `pyjit/jit88`:
- full suites with `PYTHON_JIT=always`, release and debug, and with the JIT
  off;
- instructions per loop for all 29 with the JIT on, against golden87 with
  it off;
- compile cost per code object and per word, and code bytes per word;
- timed all-29 on CPU 15, cold (compilation included) and warm.

B1 is expected to run near the interpreter's speed: it compiles the whole
semantics with no hand-written lowering. B2, inline templates generated
from LS's leaf IR, follows and is where tiers87's estimate (0.77–0.85 of
golden87's instructions) is tested.
