## Summary

The revision resolves several first-round objections, especially canonical integers and position publication.  
Rule 2 still lacks a complete contract for root states, helper coverage, and map reconstruction.  
This is a source review; no files were changed or runtime tests run.

## Blocking issues

1. **The published instruction position is not always a valid root-map key.**

   **Where:** §2 Rules 1–2; [Python/bytecodes.c:1746](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:1746), exception unwinding in the same file, and [Python/frame.c:48](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/frame.c:48).

   **Problem:** `RERAISE` restores the original exception’s `instr_ptr`, then calls `monitor_reraise()`, which can run a collecting callback. Slots now contain the exception handler’s state, not the original instruction’s live-in state. With stale dead slots, selecting the original map can dereference an already freed object.

   Other transitions also need distinct rules. `exception_unwind` installs the exception and lasti slots before calling `monitor_handled()`, while `instr_ptr` still describes the previous position. A completed frame copied by `take_ownership()` retains its last position; applying RETURN’s live-in map indefinitely would retain its returned temporary through an escaped frame object.

   **Fix:** Define the collector’s state key independently of diagnostic position. Specify its publication at exception transitions, suspension/resume, and completion. A completed frame must retain its exposed locals without inheriting the last instruction’s temporary roots. Explicitly decide how the suspended, not-yet-written send slot differs from the executing resume state.

   **Required evidence:** Collect in a RERAISE callback after an earlier collection has reclaimed an excluded temporary; record the actual root key and visited slots. Selecting the diagnostic position must fail as a negative control. This does **not** establish completed-frame retention: separately retain `sys._getframe()`, return a fresh temporary, discard the result, and assert at collection that the saved frame does not mark it. That probe does **not** prove active-frame safety.

2. **Frame-storage provenance still substitutes for proof that the collector visits the value.**

   **Where:** §2 Rule 2, especially lines 89–97; [Tools/cases_generator/root_check_c.py:2163](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Tools/cases_generator/root_check_c.py:2163), `root_check.py`, and [Python/ceval.c:2193](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/ceval.c:2193).

   **Problem:** The document changes the generator checker’s anchoring rule but leaves the C checker’s corresponding guarantee unstated. The C checker currently accepts a slot-derived value when its pointer has frame-slot provenance. That no longer proves coverage: `_PyEval_UnpackIterableValues()` writes output slots and calls the iterator again, although those outputs need not belong to the word’s live-in map.

   Also, “rooted … by the handler’s own precise C frame” is false. Note 11 explicitly excludes that frame from conservative scanning. C locals provide no protection without an enumerated anchor; reference counting provides none either.

   **Fix:** Separate storage provenance from root coverage in **both** checkers. Require a covering span whenever map membership cannot be proved, including helper outputs and early return publication. Name the helper conversions H owns; the existence of 10a’s span mechanism does not establish that these frame-slot outputs already use it.

   **Required evidence:** Add a negative C-checker fixture that stores a fresh object into a map-excluded frame slot and collects before using it. It must be rejected without a span. This cannot detect an incorrectly generated runtime map. The dynamic unpack probe must additionally collect on the second `__next__`, record coverage and alternate roots, and fail with the output span omitted; it does not cover return publication.

3. **The table’s construction cannot depend on whether code is currently instrumented.**

   **Where:** §2 Rule 2’s entry coverage and lifetime paragraphs; [Python/bytecodes.c:5538](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:5538), `Python/instrumentation.c`, and [Python/assemble.c:837](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/assemble.c:837).

   **Problem:** Assembly produces the table before runtime monitoring can make any word collecting. “Every word in an instrumented code object” does not say how entries appear when monitoring is enabled later. The suspended resume word also needs coverage independently of whether it ordinarily collects.

   The emission rule must cover final machine-independent words, including the synthetic `IR_JUMP` words introduced by relaxed FOR_ITER/SEND. `_PyRegCfg_Liveness()` still returns virtual-register sets after compaction; simply rerunning it does not turn its bit indices into physical slots.

   **Fix:** Commit to one complete table covering every final word at construction, with explicit projection through `cfg->slot` and entries for synthetic words. This avoids a second map-construction path during instrumentation.

   **Required evidence:** Enable INSTRUCTION monitoring after compilation and warm-up, then collect before a normally non-collecting word with a live temporary. Record the entry actually selected; removing it must fail. This does not test relaxed-loop words, which need a separately exercised long-loop case and its own event record.

4. **Map invalidation omits inputs that change liveness.**

   **Where:** §2 Rule 2’s lifetime paragraph; [Objects/codeobject.c:2745](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/codeobject.c:2745), [Python/regcfg.c:644](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/regcfg.c:644), `Python/marshal.c`, and code constructors.

   **Problem:** Regenerating when words or slot count change is insufficient. `code.replace(co_exceptiontable=...)` can change exceptional successors while preserving both. `_PyRegCfg_Liveness()` explicitly incorporates those edges. Changing the locals/temporaries boundary also changes which slots are unconditional roots.

   Python and C constructors do not possess the compiler’s register CFG. The document does not define their reconstruction algorithm or what “rejects a mismatched map” validates.

   **Fix:** Define all semantic inputs to map generation, including exception edges and local layout. Give compiler output, `code.replace()`, Python/C constructors, and unmarshalling one reconstruction/validation contract. Prefer derived metadata over trusting a caller-supplied bitmap.

   **Required evidence:** Replace only the exception table so an exceptional path needs a temporary, collect inside the protected region, and assert that temporary’s coverage at the event. Reusing the old map must fail. This does not establish malformed-map bounds checking; rejection tests must separately exercise invalid entries.

5. **The poisoning probe would overwrite legitimate roots.**

   **Where:** §5 Rule 2; §2’s in-flight-output rule; overlapping frames in `Include/internal/pycore_interpframe.h` and root spans visited by `Python/gc_trace.c`.

   **Problem:** “Every map-excluded slot” includes newly written outputs protected by spans—the document’s own required roots. A caller’s excluded physical slots can also overlap a callee’s live storage. Poisoning either would corrupt correct execution and make the test reject the intended design.

   Poisoning before B captures the original excluded words would also destroy the provenance needed to diagnose a missed root.

   **Fix:** Compute poison eligibility by physical address against the union of all applicable frame maps, spans, and transition roots. Capture B’s original words and provenance before mutation. Assert that ordinary precise readers avoid genuinely dead storage; B is expressly allowed to inspect its saved words conservatively.

   **Required evidence:** Exercise an overlapping call and a partially filled unpack output span. At the poisoning event, assert that their covered addresses remain intact and only globally excluded addresses change. Deliberately ignoring one span or alias must fail. This verifies poison selection, not the completeness of the compiler’s live sets.

## Non-blocking issues

- **Rule 5’s assembly criterion is too broad.** Omitting frame pointers does not eliminate stack frames needed for calls, spans, or spills. Replace “handlers have no prologue” with a criterion for the measured leaf handlers and attribution for remaining stack work. Use the compiled-stack tests to assert opaque-root ownership at collection; zero fallbacks alone cannot detect misattribution. Those tests still cover only exercised compiler/ABI layouts.

- **Name the build matrix.** PROCESS §3 gates GCC release/debug builds; Rule 1 targets tail-call handlers. Moreover, [Python/gc_trace.c:4005](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_trace.c:4005) rejects kill mode outside debug builds. “Kill on both builds” needs explicit configurations, including a debug tail-call build if that is intended.

- **Rule 3 needs handler evidence beyond canonical-form assertions.** Warm the changed integer handlers with C-returned values around ±2³⁰ and the int32 boundaries, then exercise overflow, large-int fallback, bools, and subclasses. Record the operand encoding and selected handler when executed. This cannot prove every ingress canonicalises correctly. Preserve boxed NaNs as a legitimate float representation.

- **§3’s measurements need elapsed execution time and a direct predecessor baseline.** Keep the 6d comparison, but also measure each rung against its actual starting head after 10a. Record map-lookup cost for long blocks and large live sets; suite-average table bytes can hide expensive delta reconstruction.

- **Close §6 question 2.** A whole-frame check that dereferences every purported object contradicts the permitted stale pointers. Map-aware checking is required; a separate bit-format check may inspect excluded words without dereferencing them.

- **Reconcile the records.** PROCESS §0 still describes safepoint-only position publication, and the contract’s opening links a different review filename from `14-handler-contract-review-1.md`.

## Questions

- What bounds map lookup work during collection, and can lookup allocate or fail?
- Which metric and workload choose between passing `frame` and reloading it?
- Is the map format versioned independently for tier 1, and which interpreter-state guarantees must that extension preserve?

## Verdict

revise
---
Reviewer: codex, 2026-10-28 17:32 PDT
