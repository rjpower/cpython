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
| G0b | The seed: every uop and callee declared from today's records with byte-identical output; findings and decisions for what the vocabulary cannot say; the review manifest (Opus) | running: 134 of 351 uops declared |
| G0a3 | The vocabulary the seed needs: fact kinds, GUARD results, CONSTANT rows, the `closed` input mode, `moved(k)` results, SLOT rows, `assumed(fact) by`, error-label exits, preprocessor arms, HOME for frame homes (Opus) | running |

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

DV, measured in instructions against jit88: all-29 0.9988; spectral_norm
0.971; a `//` word 51 to 22 instructions, `%` 47 to 13. Each division site
is 290-360 bytes larger than the form call it replaces; G1c tightens it.

TA, measured in instructions against `ff4ac9500e5` with the JIT on: all-29
0.9878; nbody 0.905, spectral_norm 0.947, chaos 0.961. Hot template bytes
fall 8.6% over 62 templates (7523 hot, 2073 cold); ADD_INT's longest path
34 to 28 instructions, LOAD_ATTR_INSTANCE_VALUE 41. A forced allocation
failure in a compiled box raises in the word's own frame with its inputs in
their homes.
