# Independent span native v4 outcome

2026-11-01. **FAILED: release focused case; stop preserved.** Exact source
`7d10d5bfd33cb7302df8008065c4dd637e453478` is not cleared for elapsed work by
this run. Root confirmed session 76892 terminal exit 1. The batch stopped at
04:27:22.536316 UTC, before its fixed 05:08:23.673437 deadline. There is no
completion receipt, release test verification or full release cohort credit.

Both fresh images built and imported their modules successfully: 116 checked,
10 named optional modules missing, zero failed imports. This resolves v3's
actual extension import failure for these images. The debug cohort ran all
20 span methods plus six existing controls: **26 passes, no skips**, across
four modules. I replayed the unchanged validator and exact roster checks on
actual observer/root files. Five paired ordinary driver/worker PIDs map to
their own main-interpreter root records; one platform auxiliary is separate.
The whole debug root file has 13 physical PIDs, 494 collections, zero fallbacks
and observed zero unpinned. Seven additional root PIDs are inner test activity,
not fabricated ordinary-worker or per-case attribution. Expected abort children
earn no normal end/root-counter credit. The quarantine KILL child is a focused
source-assertion witness, not a full KILL cohort.

Release ran five methods before failfast: four passed and one failed, with no
observed skips. The failure is
`CoroutineSpanTests.test_custom_wrapper_declines_and_forwards_old_free`, at
`Lib/test/test_gc_spans.py:212`: the callback's
`self.assertIsNone(reference())` saw the old closed coroutine still alive after
`support.gc_collect()`. The traceback records address `0x54816b20010`. Its
caller is `gc_span_with_allocator(callback)` at line 218. Actual returned worker
stats are five run, one failure, zero errors/skips; target return code is 2 and
outer gate return code is 1. The scheduled PID/config/seed and raw traceback
agree. The failing runner emits no outer suite-result token; none was synthesized.
The remaining 15 span methods and three other modules were omitted. In particular,
the four declared release-only skips were never reached and receive no credit.

Release's actual driver and one worker both have paired OFF/extras-zero/N0 state
and main root records. The separate platform auxiliary is also paired. Their
three root records sum to 75 collections, zero fallbacks and observed zero
unpinned. These counters establish actual collection activity, not that the old
coroutine was unreachable or that its free dispatch ran.

Source diagnosis remains bounded. The Python callback removes and deletes its
explicit old local before collection, while the C wrapper saves allocator
callbacks/context, invokes Python and restores the previous allocator. It does
not explicitly retain the old coroutine. O3 conservative register/stack retention
is plausible; so are defects that need further evidence. This run does not prove
a stale C root, a collector leak, or a forwarding failure. The immediate weakref
absence assertion combines reachability/reclamation with allocator forwarding;
its failure alone cannot distinguish them. No assertion was weakened or retried.

A useful next design would preserve new-object decline and allocator restoration,
then prove actual forwarding independently: obtain an unpublished reservation
through the real typed public allocation path before wrapper installation, free
it through `PyObject_Free` while the wrapper is active, and require a forwarding
witness plus the ownership-lookup transition before reuse. Debug-prefix geometry
and unpublished-byte map invalidation must be reviewed first. Directly calling
`_PyGC_SpanFree` or `_PyObject_MiFree` would bypass the property under test. Keep a
separate bounded real-coroutine lifecycle check after the opaque C wrapper
returns. This is a proposal for source review, not an implementation or native grant.

I rehashed all 4,673 plan inputs, actual raw state/root/log files and result
bindings; checked literal launches, downward timeout bounds, configuration,
tool hashes, both images and all 589 frozen products per image (zero additions);
and verified all four post-gate cleanup records have no live target PIDs and
finish before deadline. The source remains clean. The three successful rows
have successful verifiers; the failed release row has no successful verifier.
Earlier v1 invalid-frame, v2 disabled-GC witness and v3 hidden-import failures
remain failures. No qualification, parity or speed is established.

Reviewer receipt: `build/span-native-outcome-review-20261101-v4/receipt.json`,
SHA256 `5d2c955e9dd439e1f083024a3ca8602bda23fb2c08f74bcfeffa581585966a77`.
The reviewer invoked no compiler, target, workload or new probe.
