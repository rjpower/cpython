# H3 stage 2b.3 implementation source review

Disposition: **SOURCE_CHECKPOINT_CLEAR** for the branch-local source checkpoint
on `81417aff0971fe621fec296a324c7a48315a6082`, reviewed 2026-10-29. This is
independent of the stats author and grants no native execution, landing or
performance claim. No required source changes were found in the reviewed slice.
The coordinator owns subsequent build/control authorization and composition.

The review read the approved stage2b3 design, all 12 tracked changed files,
the new reader tests and copied reader controls, and the implementation record.
The two array axes use the generated handler count. The entry macro admits the
actual dispatch domain, advances activation-local lastopcode even with stats
off, and preserves CACHE-0 initialization and clear/activation behavior. The
generator changes only entry identity, before the existing predicted label;
semantic opcode, box and specialization sites retain their ownership.

The ADAPTIVE property derives from specializing DSL work, actual feedback use
and the target format, excluding specialized/instrumented/NOFB targets. Its C
and Python exports agree. Reader metadata is validated per raw file or JSON
before addition; duplicate keys are rejected, facts remain outside Counter,
and mismatched contracts/modes reject aggregation/comparison. Raw/family
entry sums and adaptive/specialized/other partitions do not mix semantic events
or subtract misses. Event fields are explicit; truth counters retain their
recorded key. The benchmark key/coverage change is coordinated and leaves old
artifacts intact. The five copied reader defects target the numeric and JSON
assertions rather than import failures; author-reported host results were read,
not rerun by this reviewer.

A separate host-only static inspection parsed the generated Python flag tuple
and both generated C ID headers, then checked all 287 emitted target counters:
unique names, dispatchable IDs within the 302-handler domain (maximum 301),
counter before the corresponding predicted label, and 44 ADAPTIVE entries
with no specialized/instrumented/NOFB overlap. Initial inspection attempts
omitted one of the two ID maps and stopped with lookup/assertion errors; the
complete-map inspection passed. No target interpreter was involved.

Still unproven: matching native Py_STATS size/request data, actual private
counter lifecycle/disabled/clear/pair-axis controls, generated-target and
normal-predicted copied negatives, live frame/call/thread/exit behavior,
affected native/regen/ID checks, and a real tailcall Py_STATS runtime for any
tailcall claim. The 230,368-byte growth is source arithmetic. The branch lacks
the later ROOTS/queue union; this source review carries no qualification to it.

Reviewed file identities (SHA-256):

- `Include/cpython/pystats.h`: `355f84e30939b8c37d6cb10cd846ed41235875f2e5a5ea44cba5524a40645fdf`
- `Include/internal/pycore_ir_metadata.h`: `529b21645fdbfc35631ec091caa23832d75923784ad24047621ae698f1443787`
- `Lib/_opcode_metadata.py`: `0e01a344958006b509d013161f7958b9329d078b2656ead36df43ad87278bda1`
- `Lib/test/test_generated_cases.py`: `a8c18e568b52686024c42238e240d1688e419312af94713ed8a75147b4822c46`
- `Lib/test/test_pystats.py`: `79954339f3dbbfeee59e85c00692705cdf584629dc3f3c269c3cb62a1383f6c4`
- `Python/ceval_macros.h`: `e5c3b7da12cbe662eb60d8c6a14705121ecb52b824f8a9cfeb6d8bb3ea45dcac`
- `Python/generated_ir_cases.c.h`: `122f99fb251a9d2cac795ccf091de363ea82eb1b3d69190547a335a3190802ad`
- `Python/pystats.c`: `f7895201f581d115fc4afae6ee2785ceab483c83b72a0623b7fc351750706ed4`
- `Tools/cases_generator/ir_generator.py`: `0a24d1cd83b7dbeac95abdfe018ed93b9bfd6198f19be136c0b890fddeb93cdd`
- `Tools/scripts/summarize_stats.py`: `bd7f022973ae730d472b98c4bd964088b852f3270a87aeb4530dfd3a0dcd4b0c`
- `boil/bench/README.md`: `c0f43402f39dcef9de24e40a2743b0bdaa562aecb07cb87640fcc03d2f5b2493`
- `boil/bench/run.py`: `7ec1cdf621835b84b273219bbde1966b711032c81effbb15268b377af614eaf8`
- `Lib/test/test_tools/test_summarize_stats.py`: `43436d5e87d577f6cbec1fb8d06099f312a24b0f63b6c9dcc4eca02665b3f964`
- `boil/design/work/H3-1-stage2b3-implementation.md`: `a726e75d3b4c17ef8a507f52e788bb9f06d5e138b8ed6c90a124dbfb6ced31b3`
- `boil/design/work/controls/H3-1-stage2b3/run_reader_controls.py`: `4254c80bc459be956f800501369eb519a38ea002c1e0a26492e774f326fd9506`
