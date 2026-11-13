# Cold construction: independent actual-source review

Source GO for the frozen three-file handwritten diff in
`BASE/worktrees/frame54-cold-construction`, based on golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. The actual diff is138 additions
and35 deletions across Objects/genobject.c, Modules/_testcapi/frame.c and
Lib/test/test_capi/test_frame.py. This reviewer read the full diff and checked
whitespace; no generator, compiler, runtime, inferior or test was run.

Only the approved cold gen_new_with_qualname and PyCoro_New/PyAsyncGen_New
hunks are extracted. There is no YieldFromRef descriptor port, NONE/tag state,
header/default/pystate change, debugger regression or generated product.
Baseline SetReturnValue's raw IR_NO_DST branch supplies first-input discard;
the helper explicitly sets that raw source recipient before two-argument
Copy. The comment correctly references the unchanged slot-zero initializer.
No baseline publication or temporary-home protocol needs modification.

The actual owner/failure sequence matches the independently approved proposal.
Subtype allocation failure consumes f. Enabled coroutine origin is computed
before moving any source owner; failure directly frees the untracked empty
generator allocation and consumes f. Under f's critical section, the helper
installs discard, copies homes/header, establishes CREATED/GENERATOR and
acquires a real owning frame_obj reference. Shared weakref/exception/origin/
async fields and names are initialized before f_frame association and GC
tracking. The stolen f reference is released after unlocking. Direct f_locals
moves through baseline Copy; escaped-frame takeover remains baseline behavior.
Both subtype wrappers directly return the common helper, so no duplicate
origin allocation or postpublication flag mutation remains.

The testcapi helper consumes the new frame through the matching public
constructor selected by code flags. Exactly the three direct constructor
regressions are extracted, with support and weakref imports: generator wrong
first input, permitted first None, yield/completion/close and escaped namespace
ownership; coroutine origin depth0/2 and completion; async-generator first hook,
first yield, asend and close. Neither fresh-frame-default nor native-debugger
NONE coverage is imported. The retained frame/weakref test meaningfully checks
mapping lifetime through the physical move and eventual frame.clear.

No source blocker remains. Root must build and execute these focused tests and
bounded allocation/origin failure probes on the extraction's new identity;
R2's logs are precedent, not qualification of this tree. Preserve the baseline
assertion failure evidence. The full-suite, free-threaded/native and elapsed
limits from the proposal remain; unchanged hot source alone does not prove
literal native placement or throughput equality. This is a correctness source
GO, not selection or admission.
