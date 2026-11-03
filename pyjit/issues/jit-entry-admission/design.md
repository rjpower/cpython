# Successor hypothesis: inline cached admission before the C entry call

Read-only design; no source/execution authority. Source599897 remains REJECTED: valid full29 GM losses2.26%/1.94%, including logging_silent14.98%/14.42%, generators12.20%/11.87%, deepcopy_memo8.07%/7.09% and deltablue7.19%/6.89%. No fraction is attributed to entry calls by those measurements.

## Concrete mechanism

The two canonical `Python/bytecodes.c:4401,5958` hooks unconditionally call TryEntry, producing14 generated call sites. The fresh599897 emission review confirms these remain after by-value PC restored the inspected ordinary-handler register IP paths. Qualified RC has no prefix call.

`jit_prefix_entry.c:126` runs initial_frame BEFORE Probe. Cached REJECTED/RESOURCE_DISABLED/OBSERVER_DISABLED/BUILDING rejection in `jit_prefix.c:695` therefore still pays the call boundary and initial checks. Generator/resume positions can fail initial_frame before Probe and never cache a rejection; simply moving the cached-state check inline does not cover them.

The high-loss workloads fit this hypothesis, without proving it. logging_silent's60000 loop explicitly calls debug10 times per iteration; configured WARNING logging normally enters Python isEnabledFor/cache handling then returns. Generators traverses recursive Tree.__iter__/yield-from frames, whose flags are always entry-ineligible. deepcopy_memo shares a list through a graph and repeatedly enters Python atomic/memo/copy-helper paths. DeltaBlue delegates through small constraint/plan methods. These are source shapes, not observed prefix-state distributions or dynamic call counts.

## One bounded implementation

Share existing state constants/mask through `Include/internal/pycore_ir.h`, keeping numeric values and exec layout unchanged. Add one pure inline necessary-admission predicate taking exec and PC BY VALUE. Under the existing Linux/x86-64/GIL/INLINE/!Py_STATS profile, allow only normalized states UNSEEN0, ELIGIBLE1 or ACTIVE3, masking SUCCESS_SEEN0x80 exactly as Probe. BUILDING2, terminal4/5/6 and invalid states fail closed. Require nonnull exec, explicit `exec != &_PyIR_InitCleanupExec`, and `next_instr == exec->words+1`. Unsupported compile configurations inline-return false. Never pass `&next_instr`.

In `Python/ir.c:make_exec`, initialize REJECTED for immutable entry exclusions: generator/coroutine/async-generator/iterable-coroutine flags, nonempty exception table, or fewer than5 words; otherwise UNSEEN. Generator flags are existing observers_allow_entry exclusions, NOT a claim of identical decode_prefix output for hand-crafted flag-only code. The other two conditions are literal decoder exclusions. Read pristine code facts, before both make_exec return paths/publication, not rewritten/boxed executing words. Explicitly initialize static InitCleanup REJECTED. No new allocation, callback or code scan.

Never cache dynamic input types, pending errors/breakers, tagged-value policy or transient tracing state. Probe remains authoritative for tagged_values_enabled. Structurally eligible code can become usable after earlier wrong-type inputs; preserve that opportunity.

Wrap only the two existing canonical hook bodies with the predicate and regenerate actual products. Obtain exec through the existing valid-frame helper after reload. Keep original RESUME and DTrace/lltrace notifications BEFORE admission. True calls unchanged by-value TryEntry and conditionally applies its nonnull continuation. False continues ordinary dispatch without touching frame/IP/homes/owners/errors. Direct CAPI TryEntry bypasses this hook filter and retains every full guard, promotion/post-cold recheck, native overflow decline and transactional commit.

No release counter/store, new exec field, descriptor cache, runtime option, altered eligible64 threshold, scalar C executor, new arithmetic or eager compilation. Preserve existing supported-build debug automatic-attempt accounting through the existing debug-only NoteDecline on a filtered valid nonstatic exec; never increment for unsupported builds, null or static cleanup. This does not add a release branch/store/call or a new counter. Direct TryEntry retains its current accounting.

Exact cut: `pycore_ir.h` (state/predicate), `Python/ir.c` (cold seeds), `Python/jit_prefix.c` (shared constants only), `Python/bytecodes.c` and actual regenerated products, plus small existing CAPI/test_ir fixture extensions. Keep `jit_x86.h`, decoder, CompilePlan/Run/FreePlan and leaf encoding unchanged. Expected complexity: tens of handwritten lines plus mechanical state references/generated hooks.

## Deciding checks

After separate reviewed source authority, reuse release/debug54 focused behavior through the accepted four-row build executor. Add cold predicate truth-table coverage using a local exec/word array, not live-state mutation:0/1/3 and3|seen allow word1;2/4/5/6/invalid including seen reject; wrong PC/null/static cleanup reject. Check actual initialized generator/exception-table/short-code state BEFORE Probe. Existing direct frame fixture remains defensive and bypasses the filter. Preserve exact64 promotion, later valid inputs, automatic success-seen, observer disable, faults and rollback. An ACTIVE-filtering source-negative must fail the existing automatic positive witness; no new runtime path/counter is needed.

Inspect complete fresh emitted call windows: cached-negative/excluded-generator routes must reach ordinary dispatch without TryEntry, and previously inspected ordinary IP paths must stay register-resident. All14 call instructions may remain for admissible code; do not claim binary-wide call removal. Existing54 raw witnesses must prove unchanged automatic94-byte and pressure234-byte/six-spill leaves. This is mechanism/behavior evidence, not recovered-time proof or a text-size veto.

If those checks pass, proceed DIRECTLY to one separately predeclared fresh full29 comparison against the SAME qualified RC image: existing116 scored/580values+58origins/174targets controller, unchanged GM29/290single-value/spectral10 gates and explicit material-regression disposition. Omit a redundant four-workload pilot. Retain every old score/rejection; no adaptive samples, shorter region or performance exception. Publication/fullLib remain held unless the broad mean survives.

## Limits and recommendation

Recommend this one admission change for peer review because it can remove a concrete call on permanently ineligible/rejected entries while leaving native semantics intact. It promises no recovery. Eligible-but-dynamically-unsuitable inputs still pay the slow call. Exec-header growth versus RC, inline tests, text/layout changes, caller spills, active-entry guards and owner costs remain; the branch may itself lose. A successful emitted mechanism with a failed fixed full29 score rejects the successor. No attribution narrative overrides the result.
