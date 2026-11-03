# H3-1 stage 2b.1: unified names and generated inspection

Status: implemented and gated; frozen for independent review, from `d387aca48a9`, native
prerequisite `624c75f6455`. Coordinator accepted IMPLEMENT_2B1 for the
frozen stage 2b design section 2. ROOTS/coordinator separately approved only
the exact get_ir name-selector hunk, applied with the full-domain table.
The rest of that file, including GC, compiler and scalar fixtures, is
byte-identical to the parent revision.

Ownership follows that design: three metadata generators and HandlerDomain
operation data; their opcode/IR/Python products; ceval.h names, ir.c NOFB
selection, opcode/dis private generated metadata; HandlerDomain/IR metadata
classes in test_generated_cases and relevant IR/dis/opcode tests. ROOTS
checker/import hunks remain untouched. No remaining stage 2b.2-4, sampler,
renumbering, threshold-export removal or new operation is implemented.

Before edits, the minimal NOP+IR_BACON domain put IR_BACON at 256 but the
ordinary/pseudo name emitter omitted it. The probe/output is retained in
build/H3-1-stage2b1/baseline-name.log. Existing get_ir recovered names from
its second table; this is a generation-contract defect, not a lost-name
runtime claim. IDs remain frozen.

Per-NB operation metadata is immutable and explicit; flags/base/operation
Python tuples come from the same domain. C names use numeric full-domain
indices, and the second IR name table retires. Nonadaptive display still
normalizes pristine inline constants. Adaptive executing-copy display uses
specialization kind, not an ID cutoff. HandlerFor retains its pristine-base
match before selecting the NOFB twin by explicit kind/layout.

The NOFB oracle constructs a legal shared-feedback LOAD_SUPER_ATTR_NOFB
word, installs real CALL monitoring, observes execution of its instrumented
NOFB twin, and demands exact pristine ID plus unchanged A-D bits on removal.
It does not settle for a generic-base restore. A copied control must remove
the NOFB selection and fail this actual twin/restoration assertion.

Deferred inventory now explicitly includes ir.c:87-90 and :166-167 per-NB
classification/subtraction. Future checkpoints still own that C decoder.

## Bounded correction found by the live oracle

The first real NOFB probe installed and executed
INSTRUMENTED_LOAD_SUPER_ATTR_NOFB, but its CALL callback never ran. The
generated body assigned that raw ID to `opcode`; its shared
_LOAD_SUPER_ATTR body tests `opcode == INSTRUMENTED_LOAD_SUPER_ATTR`.
The failed assertion is preserved in
build/H3-1-stage2b1/baseline-nofb-callback.log, with pre-rebuild binary,
configuration and generated-case hashes beside it. This is an inherited
body-identity bug, distinct from HandlerFor's twin selection.

The coordinator authorized a generic correction within this checkpoint:
where an instruction body uses `opcode`, generate its semantic `inst.name`
rather than the derived handler's `name`. TARGET dispatch and the tail-call
entry's raw-ID declaration stay as before; the body assignment is shared
by normal and tail-call emission. In production this changes exactly two
assignments, the generic and instrumented LOAD_SUPER_ATTR NOFB forms.
CALL, CALL_KW and CALL_FUNCTION_EX have no derived NOFB handler here;
their argument layouts retain their existing body identities.

The live regression now requires CALL, C_RETURN and C_RAISE, runs a full
collection in each callback, and checks both successful execution and the
expected error after callbacks. Monitoring removal must restore the exact
pristine NOFB ID and all four operand fields. The copied HandlerFor control
compares the same live-oracle word without executing the intentionally
wrong-layout selection. Omitting only NOFB selection must fail the
installed-twin assertion. Its scope is branch selection; the separate live
test establishes monitoring integration.

## Final evidence

The gated source is `d387aca48a9` plus the exact source hashes in
[final-provenance.json](controls/H3-1-stage2b1/final-provenance.json).
Gate headers name that parent; these are dirty-candidate runs, not tests
of the later evidence commit. The release build remains GCC O3 and debug
remains qualified GCC Og. Sources stayed frozen during each gate. All
controls use copied source or headers; no live variant was installed.

| Check | Result |
|---|---|
| Rebuild and smoke, release | 1,924 tests, 50 skipped, passed |
| Rebuild and smoke, Og debug | 1,924 tests, 20 skipped, passed |
| Affected modules, release | 889 tests, 11 skipped, passed |
| Affected modules, Og debug | 889 tests, 11 skipped, passed |
| Strict actual-mode debug kill | 889 tests, 11 skipped, passed with `--fail-env-changed` |
| Worker mode and collector totals | All nine workers reported `(2, 0)` through the actual getter and entered `worker.main`; 37 processes, 764 collections, zero fallbacks, zero unpinned records |
| Regeneration | regen-cases and clinic leave status and tracked diff unchanged |
| Optional extensions | 19 syntax checks pass; _tkinter, _dbm, _gdbm and readline remain unchecked for missing headers |
| Root checker | Existing checker fixtures pass in both affected suites; all 14 marked source inputs pass the unchanged C checker |
| Shared ownership | Exact approved CAPI selector only; ROOTS checker class, marked sources, compiler/specializer sources and scalar/phase fixtures unchanged |

The affected modules are test_generated_cases, test_dis, test_ir,
test__opcode, test_peepholer, test_compile, test_monitoring, test_code and
test_marshal. They include the compiler's direct-C no-Python phase oracle.
Normal and strict kill runs both exercise the new collecting monitoring
regression. No historical full suite has been relabeled as this candidate.

The domain comparison proves all 264 opcode/pseudo IDs, all 287 dispatch
IDs, count 302 and first IR-only ID 267 unchanged. All 299 metadata names
and Python flags/base/operation entries match the domain. Three of seven
generated products are byte-identical to the parent: opcode IDs, uop IDs
and dispatch targets. The intended changes are the unified C names, removal
of the second name table, Python metadata, and exactly two shared-body
opcode assignments. Handler formats and A-D layouts are unchanged.

Retained controls under [controls/H3-1-stage2b1](controls/H3-1-stage2b1)
include exact patches, file/dependency hashes and outputs:

- Both builds compile the exact extracted HandlerFor body (renamed only)
  against their matching configuration and current generated headers.
  Positive selection matches the actual live monitored word and restores
  its exact pristine word; omitting only NOFB selection fails the installed
  twin assertion. Wrong-layout control words are compared, never executed.
- Copied generators omit IR_JUMP's high-ID name or restore the old semantic
  body assignment. Each fails its named assertion. Copied Python metadata
  changes NB_ADD's operation value or marks its generic handler specialized;
  actual disassembly then fails the operand or display assertion.
- The original missing-CALL failure and pre-rebuild interpreter/configuration
  hashes remain separate evidence. The copied old-body generator control
  reproduces the cause without a live source swap. Initial test encoding
  through public opmap was corrected to use the full handler map before the
  final affected runs.

These are targeted checkpoint gates. No performance claim is made. The
full stage-2 and merged integration ladders remain due, after ROOTS lands.
Tail-call code generation is inspected and asserted; a tail-call runtime
build was not qualified here. Monitoring storage/atomics, monitoring
relations, stats/remaining consumers, target sampler schema and final
renumbering remain subsequent checkpoints. The inherited thresholds are
still present only as the accepted migration state; none was added.
