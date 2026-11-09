# Full31 qualification triage map

Prepared from native87's `third-integration-release-full.log` and the verbose
`third-failure-details-release-focus.log`, against current source
`3cde040a1af`. No candidate was executed for this review. Later focused receipts
below establish only their recorded selections, not a full31 result.

## Correctness and ownership first

The native87 full receipt reports no fatal interpreter error, allocator abort or
segmentation fault. `test_gc_verify` passed: its two printed holes name the
intentional string held only by a PyMem array. The test explicitly expects that
external-root diagnostic; it is not evidence of an unrooted frame owner.
Neither this observation nor a successful ordinary full run proves absence of
callback-only leaks.

If full31 exposes a crash, non-NULL scratch assertion, double release, lost
exception or unexplained retained mortal owner, prioritize its exact reproducer
over the representation and message assertions below. Run the minimal case in
debug as well as release, retain the actual handler/edge/recipient, and use
eventual collection rather than a prompt-finalizer timing assertion. Existing
terminal-observer and frame-arena tests are the first focused companion gates;
do not compensate by clearing the entire scratch array at frame exit.

## Prior failures outside monitoring, trace, Pdb and peepholer

| Original group / signature | Current source disposition | Minimal gate if it persists |
| --- | --- | --- |
| `test_dis`: 10 failures | Expected streams still contained removed loop/send cleanup words, old labels and handler names. Current fixtures were ported with independent expected streams. Native24's eight-module selection passed in both builds. | `DisTests` and `DisWithFileTests`: `test_bug_708901`, `test_disassemble_asyncwith`, `test_disassemble_recursive`, `test_loop_quicken`; `InstructionTests.test_jumpy`; `TestFinderMethods.test_findlabels`. Diff actual words, targets, feedback fields and positions before changing a literal. A missing semantic operation is a compiler fault, not a fixture port. |
| `test_exceptions.PEP626Tests.test_missing_lineno_shows_as_none`: constructor `ValueError: code: source map locations do not fit code` | The fixture now emits one no-location entry per executable 8-byte word, retaining the expected traceback line `None`. Native24 selection passed. | Run that method and adjacent with-exit traceback-line tests. A constructor rejection is geometry/format; an accepted code object with the wrong traceback line is a location consumer fault. |
| `test_embed.MiscTests.test_frozenmain`: `ImportError: Frozen object named '__main__' is invalid` | `_testembed.c` now includes the build-generated frozen header. Its absolute build target and object prerequisite regenerate it with the matching interpreter. Native24 selection passed. | Run `test_frozenmain` with the committed build's `_testembed`. If rejection returns, compare the generated header, compiling interpreter, marshal/format identity and object dependencies before touching exception or frame logic. |
| `test_sys.SizeofTest.test_objecttypes`: frame size 264 vs expected216 | Current size expression accounts for the private frame-object observer fields and current interpreter-frame layout. Native24 selection passed, but later layout edits still require checking the current expression. | Run `SizeofTest.test_objecttypes` in both builds. Reconcile exact C fields/alignment and generator size as well as frame size; never bless a size mismatch without that accounting. |
| `test_patma.TestTracing.test_unreachable_code`: LINE sequence `[1,2,6]` instead of `[1,2,5,6]` | Fused source-map assembly now selects control edges from the actual fused branch while retaining the emitted compare as `from`. Original patma test and new compare/contains/is source/error cases passed native24 selections. | Run original `test_unreachable_code` and `test_ir_source_reentry`. Retain the actual fused handler, source records and callback-error route if LINE5 disappears. Do not add a dispatched boundary to restore a source span. |
| `test_ir.AssemblerTest.test_bool_branch_source_boundary_observation`: setter refuses a protected owner recipe | This is a real debugger phase/capability issue, not an IR-format literal. Current neutral-JUMP support does not by itself authorize non-neutral committed branches or every protected/source callback. The next non-neutral stopped-view proposal remains unimplemented. | Run the exact original method. Record scope phase/site/selected edge/physical next, target scratch and caught/save context. A safe refusal remains an open Python API gap; do not weaken owner requirements to make it pass. |
| `test_value_slots.ForLoopMonitoringTests.test_return_none` and `test_return_value`: modern `stop` vs old expected empty/`raise` | Current tests explicitly expect modern STOP_ITERATION for both returned None and5. The shared completion path constructs payloads with `_PyGen_SetStopIterationValue`; the legacy adapter separately maps delivered valueless FOR completion. Native26's new payload/event gates passed. | Run both methods, then `test_ir_semantic_completion` and `test_ir_terminal_observer`. On an event mismatch distinguish modern STOP from real RAISE and legacy EXCEPTION. Check count, type, `.value`, tuple/returned-exception identity, callback mutation/error and eventual collection; do not globally suppress None or filter real iterator raises. |

The exact original class paths should be read from the current test module when
constructing coordinator selectors; the receipt identifies the method and
failure even where the full runner omitted its class name.

## Adjacent debugger consumers

Native87 also failed `test_doctest.test_pdb_set_trace_nested` and
`test_remote_pdb.test_connect`: they stopped after the requested call line and
then consumed the scripted commands at the wrong location. These belong with
the ongoing Pdb qualification, not independent arithmetic/frame-storage faults.
Current Bdb uses an explicit private entry transaction; doctest's entry hook
redirects output in the same scope as trace dispatch, and async Pdb uses shared
preparation plus explicit entry rather than a fake opcode event. Native26
consumer gates are narrower evidence; rerun the original transcript before
claiming these modules repaired. Preserve strict-ancestor/active-child ownership
when diagnosing a jump from immediate entry.

## Suggested diagnostic order

1. Full31 fatal/ownership failures, if any, with a bounded debug reproducer.
2. Full31 semantic result, exception/payload identity, collection or source-event
   failures; retain original tests and actual stopped phase.
3. The exact old mechanical selectors above, only where full31 still reports
   them. Native24 already supplies a useful prior focused control.
4. Diagnostic wording and disassembly literals after their underlying behavior,
   ownership, source coverage and geometry agree.

No current full31 outcome is inferred from source inspection. The component
timing campaign and later full diagnostic remain coordinator-owned.
