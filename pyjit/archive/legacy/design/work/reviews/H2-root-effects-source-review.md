# H2 inert scalar effects: independent source review

Verdict: **SOURCE_CHECKPOINT_CLEAR**, limited to the inert first slice. This
does not clear compilation, native semantics, complete effects/CFG/liveness,
map construction, runtime root reading or removal of clearing stores.

Reviewed the eight-file packet in `boil-h2-root-effects`, based on
`5a45f9bb6a73e7351c861faf05624ae4810f6d87`; frozen diff SHA256
`79bc9dbf90bf4a18b876e2a56f26870d04a058d4c7e2c172a24cffc9fc599ae4`.
Read source and existing contracts only; no generation, tests, target
execution, builds or owner-file changes by this reviewer.

## Source conclusions

- No falsely supported current handler was found among the 27 listed in the
  work record. These are declared **temporary-slot** inputs and new outputs,
  not a complete semantic read/write or purity model. LOAD_FAST and its
  variants read locals; STORE_FAST/COPY_FREE_VARS alter them; POP_EXCEPT
  changes exception state; STORE_DEREF/SET_FUNCTION_ATTRIBUTE mutate objects.
  Those bodies do not create an omitted temporary input or control edge.
  Locals/frame-owned roots remain independently covered under note14 rule2.
  Oparg validity and those implicit domains are not validated by this API.
  See `Python/bytecodes.c:265`, `:299`, `:2190`, `:5380` and
  `Tools/cases_generator/ir_generator.py:310`.
- The table has one DSL/analyzer authority, with no opcode whitelist.
  Exception, jump/trap, frame-transition, window and unsupported stack-shape
  classes return no usable sets. Family alignment uses the existing physical
  format: END_FOR selects the top of its family's three input fields.
  Repeated physical inputs are permitted; later liveness must union them.
  See `Tools/cases_generator/ir_metadata_generator.py:56` and `:188`.
- The decoder zeros the result, checks the executing handler's PRISTINE
  property and format, then physical slot/output-span bounds before success.
  Holes and state-dependent instrumented/specialized handlers fail closed;
  PRISTINE is the existing generated property, not a blanket rejection of
  NOFB or adaptive-family names. Flags and table defaults are coherent with
  that contract. No allocation, retained ownership, constructor admission,
  runtime root-reader call or executable-word mutation is introduced.
  See `Python/ir.c:60`, `Include/internal/pycore_ir.h:137` and
  `Tools/cases_generator/ir_generator.py:715`.
- Existing test additions exercise family operand placement, aliases,
  multi-output spans, slot bounds and named unresolved transfers/traps.
  The CAPI adapter owns only ordinary result objects and propagates allocation
  failure. Native compilation and these target tests remain outstanding.
  See `Lib/test/test_ir.py:64`, `Lib/test/test_generated_cases.py:392` and
  `Modules/_testinternalcapi.c:5016`.

## Nonblocking follow-up

Before the full producer relies on successful status, retain the explicit
temporary-only scope at its call site; OK cannot justify purity, local
operand validity, complete effects or initialized exceptional outputs.
The generator is not a proof for arbitrary future DSL bodies: new hidden
control macros, conditional initialization or reentrant/mutating helpers
require fresh semantic classification, rather than assuming infallible
means pure. Current supported bodies were inspected against that distinction.

Useful small target negatives are an actual in-domain hole, an instrumented
handler, a state-dependent specialization, zero-slot NOP versus an input
handler, and exact upper slot/output boundaries. They would exercise the
explicit property/default branches beyond the present out-of-domain65535
case. These are decoder inputs, not malformed executable code. They do not
require a new framework or expanding this slice into CFG/map activation.

## Subsequent native-test handoff

The owner later reported the first eight-module release run stopped with
748 tests/10 skips and a failure in the existing
`test_generated_cases.test_runs_code`: its metadata parser keys every
initializer by opcode, so the new second table replaces the property row.
The source review above missed that existing test-consumer interaction.
It needs a focused table-scoped parser repair before qualification can
close. This does not establish a runtime decoder failure. The original
failed run remains failed; debug did not run. This reviewer has executed
no native tests or retry.

## Coordinator disposition, 2026-10-29 19:49 PDT

The focused test-only correction b3e6847348d scopes the existing parser to
the actual formats table and asserts that a colliding effect row remains
separate. Coordinator read the exact one-file diff. The owner's host
TestIRCases controls pass27; the copied old parser fails the intended
separation assertion. No decoder/C/generated change or native rebuild occurs.

The corrected whole release generator module reports79 cases, zero skips
and one expected failure; debug actual-kill runs all eight modules,
748 cases/10 skips/one expected failure. All seven new decoder/generator
tests execute successfully. Coordinator independently verifies both result
artifact hashes, cleanup links, cache removal and exact one-file source
bridge. Original releaseFAIL remains; its seven successful modules are
carried forward explicitly rather than relabeled as current runs. Both new
results report zero fallback and zero report-only unpinned hits.

This closes bounded native validation for the inert scalar decoder. It does
not establish complete effects/CFG/liveness, root-reader activation,
performance or primary landing. The independent reviewer's source conclusions
above retain their original scope and execution limits.
