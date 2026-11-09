# Full31 GC negative control and value API lint

Investigation of the current31 full-run failures. No candidate was executed by
this worker. The GC investigation was read-only; the separately admitted
value-API repair is recorded below.

## Missing expected hole remains unresolved

`canonical31-full-release-release-full.log:623–632` reports a real failure in
`test_gc_verify.VerifierTests.test_hole_list_is_kept`: all five attempts in
`verify_pymem_array()` returned an empty hole list. The other negative-control
test emitted its expected hidden-array hole. This differs from native87's
passing module and must not be dismissed as intentional diagnostic output.

The helper calls the factory, hides its owning PyMem array address, clears
16KiB of dead C stack, and verifies before recovering the hidden pointer. The
verifier conservatively scans C stacks/registers and thread data stacks. A stale
pointer can make this negative control appear reachable; the Python test already
retries distinct object addresses for that reason. It keeps objects, not their
freed arrays, so repeated PyMem block reuse is another possible false-root
route. Neither route is demonstrated by this failure receipt.

Read-only disassembly of the current rc3 `_testinternalcapi` confirms recovery
of the hidden array occurs after `_PyGC_TraceVerify`; it was not hoisted ahead
of verification. `_PyValue_Publish` also does not clear a moved C local, so
removing that call from canonical local duplication cannot be explained as
removing a stale-pointer clearing operation. The two canonical load handlers
may affect compiled stack/register contents, but that is not causal evidence.

Root subsequently ran the exact failing method and whole module with retained
417 release, current31 release and current31 debug. All six commands passed;
receipts are `canonical31-gc-focus`. This narrows reproduction but does not
qualify the original full failure as fixed. Keep it unresolved. If it recurs,
retain test order, seed, worker environment and allocator identity, then compare
fresh-process repetitions before adding failure-only root diagnostics. Do not
relax the negative assertion, increase retries without evidence, or change
normal frame clearing to silence the verifier.

## Six window-read findings have one focused repair

Host `check_value_api.check_call_text(runtime_source)` enumerates exactly six
errors in `Python/legacy_tracing.c`, all in `sys_trace_stop_iteration_func`:

- Line391: direct `args[0]` and `args[1]` predicates.
- Line392: direct `args[2]` predicate.
- Lines394–396: direct code, exception and offset conversions.

Proposed repair: create three C-local `PyValueBorrow` values with
`PyValue_Borrow(args[0/1/2])`, then use those locals for the predicates and
conversions. The window continues to own all values. No duplication, close,
event filtering or callback ordering changes are needed. Neighboring legacy
callbacks already use this borrowing convention.

Applying this transformation first to a host string produced no findings. Root
then admitted the exact three-local/six-read runtime change. The unchanged full
`check_call_text` gate now passes with zero findings; the runtime diff check also
passes. The file is frozen for independent H source peer and coordinator native
legacy STOP/modern completion gates. No lint rule was changed.
