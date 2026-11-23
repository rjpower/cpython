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

## Outcome (qualified 2026-11-23 at `ea4d023c2b4`)

Later units, added as B1 landed:

| Unit | Work | Head |
| --- | --- | --- |
| JT | B2: inline templates for 74 handlers, generated from the leaf IR (simplification, dominator CSE, register allocation, calls to tested primitive lowerings) | `e029b80005c` |
| JP | the object, type and frame primitive lowerings, each tested against its C definition | `e3d40f3beb6` |
| JU | an exact `.eh_frame` per unit, registered with libgcc and gdb's JIT interface; one DWARF writer shared with `-X perf_jit` | `adaf96f75db` |
| JI | `ir_evaluate` is the cold tier: `RESUME_JIT` and `JUMP_BACKWARD_JIT`, chosen at lowering only when the JIT is on, hand frames to units; returns, resumes and handled exceptions re-enter them; `jit_run()` is the one driver | `182e5c8af64` |

`pyjit/jit88` merges: JB (B1) `7e0adc32948`, JT `aa08adfc097`, JU
`b6829720a09`, a gdb test fix `8b6e6db836f`, JI `79ed738ab6b`, and
`ea4d023c2b4` (`PYTHON_JIT_WARMUP` sets the warm-up from startup). Evidence
`E59/jit88/`; reports `{jb,jt,jp,ju,ji}/report.md`.

Timed all-29 on CPU 15 against golden87 with its JIT off (geomeans, cold /
warm; `bench/*-summary.txt`):

| Build | `on` | `always` | Instructions (`on`) |
| --- | ---: | ---: | ---: |
| B1 (`7e0adc32948`) | 1.226 / 1.224 | 1.225 / 1.220 | 1.203 |
| B2 (`8b6e6db836f`) | 1.062 / 1.056 | 1.062 / 1.053 | 1.039 |
| JI (`ea4d023c2b4`), warm-up 32 | 1.065 / 1.060 | 1.075 / 1.064 | 1.042 (B2 × 1.0029) |

- tiers87 expected B2 at 0.77-0.85 of golden87's instructions. Kernels with
  few calls reach it (nbody 0.768); call-heavy kernels do not (richards and
  deltablue 1.16): calls, returns, polymorphic attribute loads and the
  driver are about 45% of their instructions.
- With the JIT off, JI's build executes 0.9995 of golden87's instructions.
- The warm-up threshold does not move throughput: warm 1.056-1.060 at 32,
  256, 1024 and 4096. It sets startup: an import-heavy start costs +30% at
  32, +10% at 256 and +2.3% at 1024. A compiled word costs about 3.2k
  instructions; one call of the assembler per machine instruction. The
  default stays 32 until regions89 changes compile cost and staleness.
- Recompiles: any rewrite of a handler in a code marks its unit stale, and
  the next event recompiles the whole code (cap 8). On an import-heavy start
  24 codes at 5-8 compiles hold about 40% of compiles.
- Time without instructions: unpack_sequence runs 1.41 at 1.02 of the
  instructions. Its 400-site loop is about 52 KB of native code (95-byte
  form-call sites) against a 32 KiB L1i: L2 i-cache fills 33x, 53% of ops
  from the legacy decoder. logging_silent (1.155 to about 1.25) and
  spectral_norm (1.09 to about 1.14) moved with JI at unchanged
  instructions. Not chased: no placement tuning.

Qualification at `ea4d023c2b4`: full suites with `PYTHON_JIT=always` on
release and on the assert build (`-O3 -g`, assertions on), and with the JIT
off on release: known failures only, plus the intermittent test_profiling
failure also seen in contract87's and handlers81's sweeps. Sweeps now run on
release and the assert build; Py_DEBUG is kept for diagnosis and refleak
runs.

Astra's review of the handlers (`E59/jit88/astra/handlers-proposal.md`)
followed this wave. Its units continue the baseline JIT in
[regions89](regions89.md).
