# Finalizer-order eligibility: source checkpoint

Base f2a5b3ca33d1, branch boil/gc-finalizer-order, isolated managed worktree
boil-gc-finalizer-order. Source only: unqualified; no configure, build, target
execution, test or measurement has been run. Disk check before creation:
213GiB available. Frozen parent products and other worktrees are untouched.

The coordinator approved proposal a4f5c0fe. The patch adds one local
finalizer_eligible predicate shared by DFS root admission, postorder emission
and enqueue. It tests DEAD, FINALIZED, tp_finalize and the unchanged
_PyGen_FinalizerIsNoop helper. order_visit/general traversal, dead metadata,
flags, queues, resurrection, deallocation and allocation-error paths are
unchanged. Ineligible objects remain transit nodes for active finalizers.

The single regression uses ordinary Python objects:
outer -> cleared generator -> code -> co_consts tuple -> inner. Both F
instances have finalizers. Public code.replace/FunctionType builds the path;
the generator is explicitly closed and its remaining code edge is checked.
Only weakrefs escape the construction helper. The test requires outer before
inner, intact access to inner's name in outer, and cleared weakrefs afterward.

Copied negative recipe after source review: create a separate managed source
copy from the frozen positive commit and remove only finalizer_order's
reverse-postorder loop. Keep helper, traversal and the test identical. On the
acyclic path, postorder must emit inner before outer; the expected result is
the named event-order assertion failing, without crash/import/build failure.
No negative source/build has been created, and no negative has run. A control
that filters closed-generator transit edges can accidentally pass with a
favorable root order; it is not a substitute for the deterministic negative.

Next is coordinator review of the exact three-file diff and an affected
launch plan. First feedback should include the new graph test and existing
finalizer/resurrection/generator/weakref/deallocation/queue-OOM semantics on
owned release and actual debug-kill builds. Required broader qualification
is not waived. Matched parent/candidate release measurements then reuse the
existing driver/loops/warmup/paired sample policy on the three slow rows and
Richards. No gain or complete OOM-injection coverage is claimed.
