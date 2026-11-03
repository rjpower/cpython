# H3-1 stage 2a: fixed-ID metadata and pristine scalar boundaries

Status: reviewable bounded checkpoint, not landed, 2026-10-29. Branch `boil/h3`, from
integration `6c724c29408` (clean fast-forward at quiescence). This is a
coherent checkpoint within stage 2, not completion of H3-1. Stage 1 and
its accepted review are preserved; the coordinator owns the separate
round-2 verdict record. No stages 3/4 or new operation are included.

## Ownership before edits

This checkpoint owns:

- `Tools/cases_generator/{ir_generator,ir_metadata_generator,
  opcode_id_generator,opcode_metadata_generator,lexer}.py`, their generated
  `Include/{opcode_ids.h,internal/pycore_ir_metadata.h,
  internal/pycore_opcode_metadata.h}` products, and generated-metadata/
  HandlerDomain hunks in `Lib/test/test_generated_cases.py`.
- Pure `pristine`/`static_exec` declaration annotations in `Python/bytecodes.c`.
- `Include/internal/pycore_ir.h`; `Python/{ir,specialize,assemble}.c`;
  `Objects/codeobject.c`; the minimal checked-rewrite caller in
  `Python/instrumentation.c` (not its storage redesign yet).
- Code/IR constructor and scalar-boundary tests in `test_code.py` and
  `test_ir.py`, as needed; marshal/freeze paths through the shared code
  constructor are included in their bounded tests.
- Only the exact additive include, `ir_scalar_boundary` function and one
  method entry proposed in `build/H3-1-stage2a/scalar-fixture-proposal.patch`
  in `Modules/_testinternalcapi.c`. ROOTS reviewed it and the coordinator
  granted this bounded exception. GC hooks and compiler phase fixture stay
  unchanged. No ROOTS-tree edit or cross-step merge.

ROOTS owns the C-checker fixtures/import contracts in
`test_generated_cases.py`; those are untouched. H3 does not edit gc_trace,
gc_live, test_gc, C checker or call_effects. Monitor storage/atomic paths,
statistics and remaining inspection/compiler bounds are later stage-2
checkpoints, with their own complete consumer/field evidence.

## First failing control

Before implementation, the qualified default interpreter accepted a new
code object whose first pristine handler was INSTRUMENTED_LINE=255:
`build/H3-1-stage2a/baseline-pristine.py` exits 1 and records the acceptance
in `baseline-pristine.log`. It does not execute that code. Newly allocated
code has no monitoring data, while that handler requires displaced line
state. This is the first bounded defect, distinct from CFG/root validation.
CACHE's existing zero/padding constructor contract and explicit execution
trap remain.

## Intended change and control boundary

Generate metadata presence, dispatch-body presence, pristine admissibility
and required installation-state separately, under the same numeric IDs.
Compiler-selected inline constant forms are admissible despite belonging
to specialization families; audit the exact `choose_handler()` producers.
Ordinary, per-NB and uninstrumented NOFB forms remain constructible.
Monitoring wrappers, runtime-selected specializations and interpreter-only
shim forms require their installation context and are not pristine.

The sole generated count moves to opcode_ids.h, and the single deopt/base
map covers the full fixed-ID domain. The duplicate IR shadow relation
retires with its consumers. Numeric threshold exports remain until stage4;
new runtime decisions use explicit properties.

Allocation-free checked `_PyIR_MakeWord()` and `_PyIR_SetHandler()` take
an int handler before any uint16 cast and return boolean success, leaving
the output unchanged on rejection. `encode()` validates before its first
format lookup. All setter results are consumed: the attribute retry uses its existing decline result;
impossible specialization/monitor rewrites fail the internal invariant
explicitly before indexing metadata with a rejected scalar. Field bits and the 64-bit word layout do not change.
The C fixture returns success/before/after for each actual producer;
tests include 65536, 65536+NOP and NOP, so unchanged-word alone cannot hide
a successful truncation to NOP.

Move bounded `check_words()` admission and existing inline-constant
replacement into `_PyCode_New()` with explicit replacement ownership and
cleanup. Constructor/replace/compiler/marshal/freeze paths reach it before
publishing code. Preserve slot/feedback checks; do not add CFG/H2/root-map
validation. Runtime state installation remains an obligation of each
installer, not something the bare scalar setter can infer from a word.

## Evidence plan

Targeted generation/code/IR/compiler/monitoring/phase-oracle/marshal tests
on release and qualified Og debug; actual-mode asserted kill regressions.
Regenerate every affected product and compare every numeric ID against the
stage1 baseline; changed metadata bytes are expected, renumbering is not.
Run positive scalar fixture and removed-precast-guard controls, constructor
wrapper/stateful rejection and a marshal bypass control. Do not execute
malformed pristine wrappers. Use task-owned isolation or a coordinator
source freeze covering both builds for C control builds; never reuse the
unsafe stage1 live-mutation reproducer.

Record exact source/binary/patch hashes and tested revisions. Keep source
quiescent during gates and use standard machine slots/build locks. Whole
stage2 and final integrated ladders remain separate from this checkpoint.

## Implementation disposition

The fixed domain has 302 index slots and 287 dispatch bodies. Of those,
153 are admissible in pristine code and 134 require installation state.
All 264 opcode/pseudo IDs, all 287 dispatch IDs, first IR-only ID 267 and
count 302 match the stage 1 baseline. The generated count now lives only in
`opcode_ids.h`. `_PyOpcode_Deopt` is the sole uint16 base map over that count;
formats no longer carry a second shadow relation. Base normalization and
specialization kind remain distinct.

`pristine` marks the compiler's three inline-constant forms;
`static_exec` marks the two interpreter shim forms. These declarations
are independent of family membership. Generated metadata presence,
dispatchability, pristine admissibility and required-state bits are
separate properties. Monitoring and specialization state are not inferred
from a numeric category cutoff. Event/tool masks remain unchanged.

Both actual dynamic word producers validate the original int before a
uint16 cast. Their boolean rejection changes no word. The assembler also
checks admission before its first format lookup. Every setter result is
consumed. The attribute-absence retry propagates failure through its
existing decline result; impossible specialization/monitoring rewrites
fail the internal invariant without indexing metadata with the rejected
scalar. These helpers allocate nothing and run no Python.

`_PyCode_New()` now owns one shared bounded check for constructor, replace,
compiler and marshal callers. Its local constructor copy owns replacement
code through interning, optional location compaction and initialization;
all exits release replacement references. Existing inline-constant
replacement and CACHE zero-padding remain. Frozen imports in this build
use marshal (`Python/import.c:unmarshal_frozen_code`), so reach this check too.
The unused legacy deepfreeze generator is not represented as a tested
static-code constructor. This adds no CFG, branch-target, root-map or H2
validation.

## Controls and provenance

The pre-change local replace probe accepted INSTRUMENTED_LINE. With
coordinator authorization, a second read-only probe used the frozen
stage 1 integration interpreter at `6c724c29408`, under its existing build
lock, timeout and fresh task-owned pycache. Both replace and marshal
accepted LINE and the two static shims; marshal also accepted
BINARY_OP_ADD_INT. Legacy replace already rejected that specialization for
missing feedback entry 0. The seven acceptances and that separate rejection
are recorded exactly. None of the malformed code objects was executed.
The baseline log records actual binary/config/source hashes and the
opcode module loaded from the frozen source.

`run_scalar_controls.py` copies the actual current header and extracts the
exact authorized CAPI fixture body into tiny task-owned extension builds.
Dependency files prove resolution of that copied header and the current
generated metadata and matching build config. Each variant runs in a new
process. No live source, interpreter or existing extension is mutated.
Positive and variant source/fixture/header/config/interpreter/extension
hashes and compile commands are retained for release and Og debug.

The two negative controls move the handler's uint16 cast ahead of the
respective pack/rewrite property check. These are precise narrowing-order
controls, not a claim that one redundant guard omission alone proves the
boundary. Each wrongly accepts 65536 and changes NOP to CACHE, and wrongly
accepts 65536 + NOP even though the word stays unchanged. The positive helper
rejects both original scalars and leaves the destination unchanged. A
valid NOP succeeds. Thus status, as well as word contents, is essential.

## Remaining stage 2 work and limits

This checkpoint does not complete H3-1 stage 2. Displaced-opcode storage,
compressed LINE data, full-width atomic publication, opcode-indexed
monitoring tables, remaining uint8 specialization consumers, statistics,
inspection and remaining compiler/tooling bounds still need closure.
Required-state metadata states the installer's obligation; the bare
rewrite primitive cannot infer whether a caller installed that state.
The scalar rewrite-only wrapper test never dispatches that word.

Sampler target-schema verification and codec changes are stage 3. Numeric
renumbering, threshold export removal and the pyc magic change are stage 4.
No new operation, temporary runtime mode, fallback representation or JIT
change is introduced here. Full stage 2 gates, performance measurements,
merged integration gates and independent review remain separate from
this bounded checkpoint. No performance gain is claimed from these
correctness changes.


## Final checkpoint gates

These runs used `6c724c29408` plus exactly the native/generated/test bytes
in `controls/H3-1-stage2a/final-provenance.json`. The intervening tooling
commit `3dff5879df8` changes no native/generated/test file. The eventual
checkpoint commit preserves those recorded tested bytes; it does not turn
older full-suite runs into current-head evidence.

| Check | Release | Og debug |
|---|---|---|
| Rebuilt gate0 smoke | 1,924 tests; 50 skips | 1,924 tests; 20 skips |
| Ten affected modules | 974 tests; 11 skips | 974 tests; 11 skips |
| Copied-header scalar controls | positive and both narrowing controls | positive and both narrowing controls |

The ten modules are test_generated_cases, test_code, test_ir,
test_peepholer, test_compile, test_marshal, test_monitoring, test_dis,
test_opcache and test__opcode. They include the existing direct C
specialization phase oracle. ROOTS C-checker classes and that existing CAPI
phase fixture are byte-identical to the integration base.

The final strict verbose kill run used the actual-mode asserting launcher
through normal `worker.main()`: 974 tests, 11 skips, all ten modules OK,
no environment changes. Ten distinct test workers witnessed actual
`(2, 0)` mode. Retained collector counters cover 38 processes and 808
collections, with zero fallbacks and zero unpinned roots.

The earlier direct-worker launcher bypassed the worker cwd/exit setup:
one quiet replay reported two file-environment warnings; a strict verbose
replay exposed a peer worker's temporary file in the repository cwd and
exited 3. The interim two-module strict rerun was clean but did not explain
that concurrency. Those logs and the old launcher are retained, superseded
by the corrected full strict run. The three historical retained launchers
were separately corrected and gated in [H3-worker-main.md](H3-worker-main.md).
No native source changed while resolving this harness defect.

Regeneration (`regen-cases` and clinic) leaves the tracked diff and status
unchanged. Four of seven stage1 products are byte-identical; the opcode
count declaration, full base map and IR property/format metadata change
in the other three as intended. All numeric IDs remain identical. The
marked-source C checker scan is clean after the annotation-token addition;
all its fourteen marked inputs and checker sources retain the same bytes
through final native fixups. Nineteen optional-module syntax checks pass;
_tkinter, _dbm, _gdbm and readline remain unchecked for missing headers.

Two preliminary test-only mistakes (a count-definition spacing expectation
and looking up a private specialization in public opmap) were corrected
before the final production candidate and final passing affected runs.
The final source audit also consumed the inherited ignored setter result
and removed invalid-index access from its impossible install-failure path;
all final smoke/target runs above followed those native changes.
