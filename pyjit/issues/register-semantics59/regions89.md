# Regions89: declared contracts and shared stubs for the baseline JIT

Approved 2026-11-23. After [jit88](jit88.md) qualified, Astra reviewed the
baseline JIT's handlers (`E59/jit88/astra/handlers-proposal.md`). It found
that a word without a template calls its handler's whole function form and
then tests outcomes its fast path cannot produce. It also found that the
operation contract's effects and facts are inferred, not declared
(`ir_records.py`: "no uop declares any"), although tiers87's C6 requires
declarations.

The user approved the direction and Astra's three decisions:
1. Keep the generated C interpreter.
2. Calls and returns become tail transfers within one native activation.
3. Frame homes own their values at every baseline word boundary; removing
   ownership traffic across words is the tracing tier's.

And set the style: declarations and explicit metadata on every handler and
component, in the manner of V8's bytecode and builtin lists, call
descriptors and Torque's checked `transitioning` and `labels`. The
generator verifies declarations and never authorizes by inference.

Base `pyjit/jit88` at `ea4d023c2b4`. Integration branch `pyjit/regions89`.
Evidence `E59/regions89/`: `coordinator.md` is the log, `common.md` and
`brief-*.md` are the briefs, `<unit>/` holds each unit's design, progress
and report, `astra/` the reviews. Correctness gates each step; performance
is recorded.

## Units

| Unit | Work | State |
| --- | --- | --- |
| RD | Design of G0 and G1 and a pilot of the notation and verifier on BINARY_OP, UNPACK_SEQUENCE and LOAD_ATTR_INSTANCE_VALUE (Opus) | design revised after two Astra reviews; pilot merged with G0a |
| DV (G5) | `cqo` and `idiv`; signed division and remainder lowered from leaf IR; FLOOR_DIVIDE_INT and REMAINDER_INT as templates (Sonnet) | merged, `ac598a3df3f` |
| G0a | The notation and verifier for every construct: sound ownership transitions, per-path nullness, exported facts from the live state, `holding(x)` raise checkpoints, skeleton steps and arrays across uops, feedback access, ROUTINE and transaction rows, V11, the counter functions, and the CONJOINS, NONNULL_FORM and SLOW columns (Opus) | merged, `ff4ac9500e5` |
| G0a2 | The verifier fails closed: Astra's review of G0a found seven acceptance holes, each closed with a negative test; the columns' structural checks (Opus) | merged, `d38559113ef` |
| TA | Template tightening from declared facts: guard fusion, dead NULL tests, box exits, results in destination registers, CSE by declared effects; rows for every primitive the templates use; no sampling of models (Opus) | merged, `0a4e7bd4bae` |
| G0b | The seed: every uop and callee declared from today's records with byte-identical output; findings and decisions for what the vocabulary cannot say; the review manifest (Opus) | merged, `f91d4f80af3`: 195 of 351 uops declared with unchanged generated output; 132 uops routed to G0c, 21 to G0a5 |
| G0a3 | The vocabulary the seed needs: fact kinds, GUARD results, CONSTANT rows, the `closed` input mode, `moved(k)` results, SLOT rows, `assumed(fact) by`, error-label exits, preprocessor arms, HOME for frame homes (Opus) | merged, `5af180074c5` |
| G1a | Whole-word completion stubs generated from declared uops for UNPACK_SEQUENCE and BINARY_OP's generic arms; a 23-byte shared site with one cold resolver (MISS, ESCAPED, RAISED, BRANCH, AGAIN); staleness found by execution (Opus) | merged, `a5502da133f` |
| G0a4 | The last vocabulary before G0c: correlated status-and-value results, frame-path and call-state facts, a leaf `assume` op, moves out of operand homes, object constants (Opus) | merged, `6966e459c7e` |
| G0c | Review of the seeded contracts in eight batches by topic: evidence per row, corrections in both directions, the routed uops declared, native tests; vocabulary frozen, gaps routed to G0a5 (8 Sonnet) | merged, `3cf8fc95568`: 278 of 351 uops declared; each generated difference listed with its evidence; uops blocked by a gap left with their intended lines for G0c9 |
| G0a5 | The 21 uops the batches cannot declare (facts of an input's field, a declared calling-convention enum, walk gaps, yields the walk misses) and the vocabulary desk during G0c (Opus) | merged, `3fd555b6e3f`: field paths, the `_PyCallConvention` enum, walk gaps, committed exits, one encoding lattice, closed types, status facts; 297 of 351 uops declared |
| G3D | Design of compiled frame transfers: a per-unit switch routine, the next frame's entry from its `instr_ptr`, one code pin per evaluation loop placed by a declared `retires_frame` effect, a SWITCH bit in G1a's ABI (Opus) | accepted (revision 3) after two Astra reviews |
| G0c9 | The pending declarations: each line the batches could not commit before G0a5's rules, applied or recorded for G0d (Sonnet) | merged, `6c79bf26c5c`: 320 of 351 uops declared |
| G3a | Compiled frame transfers through the existing forms: pins, the switch routine, RESUME_CHECK, the forms' transfer path; runtime first, declarations after G0d (Opus) | merged, `e571a8717e6`: execution pins, the switch routine, `RESUME_CHECK` as a template, and calls and returns between compiled frames in one native activation (fib(18) under `always`: 16,687 driver turns before, 0 after, with 16,828 switches) |
| G0d1 | The cutover's records, ownership and transfers: modes, frame switches and facts from declarations; the `redirect` and `abort` exits; the verifier's nested-call check (Opus) | merged, `a4e194a018e`, `d344a3dcd5a` and `e0cefcf2df6`: records from declarations only (373 of 373 uops), the `redirect` and `abort` exits, a verifier that checks every argument and owner, two crash fixes and a settrace fix (31 `f_lineno` jumps under `always`) |
| G0d2 | The cutover's effects: reads, escapes and error effects from rows; the observer and the SEND boundary by declaration (Sonnet) | merged `ece998c458d` and `887c6a4bb1a`: escapes and error effects from rows, the SEND family and INSTRUMENTED_FOR_ITER declared, `raises` on 81 rows |
| G0d3 | The cutover's leaf translator: IDENTITIES, ERASED and the view tables become rows; V11 strict (Sonnet) | merged, `a464a3b966c` |
| G0d4 | The cutover's slot stores and the F16 cursor uops, then the uops other units refused (Sonnet) | merged `8191094c781`: six slot stores through declared helpers, the F16 cursor by value; its follow-up was denied edits outside its brief and went to G0d2 and G3a |

The design (`E59/regions89/rd/design.md`) sets the following units, in
order:
- **G0a core:** the declaration notation (contract clauses on uops,
  `region(...)` on arms, one X-macro component list with call
  descriptors), the verifier rules and the restartability invariant, with
  a negative test per rule.
- **G0b seed:** declarations seeded from today's records, with
  byte-identical generated output, so that they grant no new authority.
  There is no joined record and no review marker in code; review status
  lives in the manifest.
- **G0c review:** parallel Sonnet batches under a frozen symbol-to-owner
  manifest; every row carries its evidence.
- **G0d cutover:** records from declarations only; the inference tables
  are deleted.
- **G1a core:** whole-word completion stubs, generated as C and compiled by
  GCC; a call site of about 25 bytes; staleness found by execution.
- **G1b inventory:** a plan for every handler.
- **G1c tight inline arms:** guard fusion, in-word facts and CSE. The
  targets are an int add at about 15-21 instructions (32 today) and an
  instance-value load at about 20 (38).
- **G2:** polymorphic loads. **G3:** compiled frame transfers. **G4:** exact
  call and return regions, and `split` arms with typed continuations.
- **G6:** write-through register caching across words.

On 2026-11-24 the user moved semantics from the stack uops to register
instruction families ([rir90](../../design/proposals/rir90.md)). G1c, G2,
G4 and G6 are held so that they are not built on the stack uops. Rir90
gives each a new home:
- G1c becomes measured work on `Add.Int` after the pilot;
- G2 becomes the pilot's poly arms for `GetAttr` and `GetMethod`;
- G4 follows the pilot's `Call` and `Return`;
- G6 follows the independent-entry tests.

G3a's runtime work carries over: the execution pins, the switch routine and
`RESUME_CHECK`.

DV, measured in instructions against jit88: all-29 0.9988; spectral_norm
0.971; a `//` word 51 to 22 instructions, `%` 47 to 13. Each division site
is 290-360 bytes larger than the form call it replaces; G1c tightens it.

TA, measured in instructions against `ff4ac9500e5` with the JIT on: all-29
0.9878; nbody 0.905, spectral_norm 0.947, chaos 0.961. Hot template bytes
fall 8.6% over 62 templates (7523 hot, 2073 cold); ADD_INT's longest path
34 to 28 instructions, LOAD_ATTR_INSTANCE_VALUE 41. A forced allocation
failure in a compiled box raises in the word's own frame with its inputs in
their homes.

G1a: a shared word's site is 23 hot bytes (86 before); unpack_sequence's
compiled loop 90.4 KB to 78.5 KB; all-29 instructions with the JIT on
0.9974. Recompiles on an import-heavy start 380 to 377: staleness found by
execution does not shorten the tail of codes compiled 5-8 times.

Timed at `a5502da133f` on CPU 15 against golden87 with its JIT off (cold /
warm geomean): `always` 1.038 / 1.030, `on` at warm-up 32 1.024 / 1.022,
at warm-up 1024 1.021 / 1.019; jit88 at warm-up 32 was 1.065 / 1.060. At
warm-up 32, warm: nbody 0.964 to 0.748, spectral_norm 1.133 to 0.792,
chaos 1.014 to 0.940, fannkuch 0.955 to 0.915, go 1.004 to 0.979.
unpack_sequence moves only from 1.424 to 1.380, and richards (1.157),
deltablue (1.155) and raytrace (1.104) wait for compiled calls and returns
(G3, G4).

G3D measured one compiled call and return at 491 instructions, 137 of them
driver turns and unit entries and exits; those are 14.9% of richards' and
15.5% of deltablue's instructions. The design's estimate is about 57 after
G3, leaving richards and deltablue near 1.05 of golden87; the call and
return bodies (270 instructions) are G4's.

## Closing, 2026-11-24

Regions89 closed at `e571a8717e6` (pushed), with G3a's frame transfers on
top of G0d1's cutover. The full suites at that commit:
- release and the assert build with `PYTHON_JIT=always`: known failures,
  plus `test_profiling`'s `test_cprofile` in both, with identical counts;
- release with the JIT off: known failures only.

The `test_cprofile` failure (backlog 793) is an upstream `_lsprof` bug, not
the JIT's. An enabled `_lsprof.Profiler` left in a reference cycle by an
earlier test clears the running thread's `sys.setprofile` function when the
cyclic GC finalizes it. It reproduces on upstream `3a440b6`. Rir90's PF unit
fixes it.

The user set timing as approximate on 2026-11-24, so the closing
measurement did not gate anything (`E59/regions89/bench/g3a-summary.txt`).
Against golden87 with its JIT off, cold / warm geomean:
- `always`: 1.070 / 1.048, 2 rounds with the core gate recorded but not
  enforced;
- `on` at warm-up 32: 1.051 / 1.025, from one gated round.

At G1a the same figures were 1.038 / 1.030 and 1.024 / 1.022. The noise is
larger than the difference. The cases that move most under `always`, warm,
are spectral_norm (0.778 to 0.923), raytrace (1.124 to 1.281), regex_compile
(1.070 to 1.319) and xml_etree_iterparse (1.067 to 1.295). They need a gated
run before any reading. Rir90 rebuilds the paths these numbers cover.

