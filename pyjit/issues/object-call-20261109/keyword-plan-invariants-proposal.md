# Keyword-plan construction invariants

**Future proposal, pending peer review; no implementation authorized.** Retain the existing binder and frame ABI. The bounded change is to remove repeated destination bounds, positional-only, prefix-overlap and pairwise-distinctness checks from the release keyword hit path. Keep mutable guards and exact-name validation. No runtime edits or native jobs were performed for this audit.

## Identity and arity proof

`Objects/codeobject.c` assigns each code object an immutable `co_version` from the interpreter counter. The counter stops at zero on overflow; versions are not recycled. `MAKE_FUNCTION` is the sole caller of `_PyFunction_SetVersion`, setting `func_version` to that code's version. `_PyFunction_GetVersionForCurrentState` returns this value, and builders require a valid nonzero version. Functions sharing a version share the exact code object, not merely a compatible signature. Code allocation/cache eviction does not authorize version reuse.

`Objects/funcobject.c:func_set_code` clears the function version before replacing `func_code`, under the existing mutation protocol. Code replacement creates a separate code object/version; exposed signature fields and `co_localsplusnames` are immutable. Same-code factory functions may have different defaults, so version equality does **not** prove current default coverage. Existing default-shape guards remain necessary.

`CALL_KW_PY` executes the checked PEP-523, function-type/version and recursion prefix. `validate_guarded_python_call` requires that exact nonescaping prefix, immediate guarded invoke and terminal completion. No callback occurs between the version proof and the binder's owner transfer. The call site's encoded argument count is immutable for the retained code/instruction; specialization/instrumentation change its handler, not its call arity. The existing self-presence guard pins the builder's effective positional prefix, `argument_count + has_self - kwcount`.

These proofs apply to the current GIL-only direct binder. Free-threaded execution already declines it. Expanding entry points, restoring historical plans, mutating call windows, or adding an escaping prefix would require a fresh proof, not reuse of this conclusion.

## Writer, reset and copy audit

| Route | Effect on plan | Consequence |
| --- | --- | --- |
| `specialize_keyword_binding` | Sole destination publisher; checks a local array first, copies destinations, then publishes keyword/count/self flags | Published destinations are complete and valid |
| `specialize_py_call_kw` | Sets version, clears flags, calls builder only for an ordinary Python function, then publishes `CALL_KW_PY` | Declined/partial builds cannot expose a keyword plan |
| `specialize_py_call` | Clears flags, may write the shared positional union and DIRECT flags | Keyword discriminator cannot survive positional reuse |
| `_Py_Specialize_Call`, `CallKw`, `CallFunctionEx` | Enter `fb_begin_call[_ex]` before replacing observations | All call-family respecializations use the reset below |
| `fb_begin_ex`, `FB_CALL` | Zeros every payload byte after the header, including destinations/flags/version | Reset removes the plan before replacement handler publication |
| `fb_row_of`, `FB_CALL`; `fb_put_row`, `fb_save_mono`, `fb_record` | Call history records versions/handler; it does not save destination plans | No historical-plan reconstruction or unchecked plan copy exists |
| `Python/ir.c` execution-copy creation | Copies original instruction words; zeroes new feedback and initializes headers/static operands | A fresh execution copy starts without a plan |
| Instrumentation/deoptimization | Changes dispatch; cannot construct keyword destinations | Reentry requires the checked handler and valid plan flags/version |

The reset/publication sequence is serialized by the GIL. A failed builder leaves flags zero; it writes destinations only after every admission check succeeds. Snapshot allocation may happen before reset, but does not restore plans or call user code inside the checked hit prefix. Searches found no other destination/flag writes, executable call-plan history reader, or raw feedback-copy route that publishes a keyword plan.

## Invariants established by the builder

For published keyword plans, keyword count is 1–3; the code is optimized and excludes generator/coroutine/async-generator and variadic shapes. `total = co_argcount + co_kwonlyargcount` fits `uint16_t`. Each destination is found by exact-name identity starting at `co_posonlyargcount`, ends before `total`, is at or above the builder's positional prefix, and differs from all earlier destinations. Every keyword-only argument is explicitly supplied; omitted positional arguments were covered by defaults at construction time.

Version equality pins the first three destination facts to the same signature. Site arity, cached keyword count and the retained self-presence guard pin the positional prefix. Thus destination bounds, positional-only exclusion, prefix exclusion and pairwise distinctness remain invariant on a hit. Keyword-only explicit coverage is also invariant; omitted positional default coverage is not.

## Exact bounded change and retained hit guards

In `Python/ceval.c:call_bind_known_python`, remove only these release checks from the destination/name loop:

```c
slot < code->co_posonlyargcount || slot < positional || slot >= total
/* plus the nested j<i duplicate-destination loop */
```

Remove the now-unused local `total` if appropriate. Express these invariants as debug assertions immediately before destination-indexed reads; keep the builder's full validation. No new feedback fields, cached pointers, names tuple ownership, signature hashing, alternate binder or generated ABI is needed. Comments should identify the builder + version/site/self proof, not imply the version proves defaults.

Keep all current hit-time checks: keyword discriminator/count bounds; thread-owned caller; matching self presence; positional count sanity; exact names tuple and count; every name an exact Unicode object equal by identity to the current code name at its cached slot; standard vectorcall provider; initialized current code execution state; stack capacity; current default count bound and required-positional coverage. Keep the checked PEP-523/function/version/recursion prefix and generated completion contract. Keep names exactness because their release after child publication relies on callback-free tuple/string retirement. Keep the complete scratch/detach/overlap/publish sequence unchanged.

Default values and shape remain read from the current function. `__kwdefaults__` mutations cannot affect admitted explicit keyword-only values, but tests must retain that distinction. An equal-but-different names object safely misses the identity check. A changed names order must miss unless it matches every cached destination; no plan rebuild on the hit path.

## Validation before any performance claim

Retain existing permutation/prefix, 1–3-name, larger-arity, defaults-hole, explicit keyword-only, positional-only error, duplicate positional/keyword, unexpected name, method, generator-owner, observation and owner-retirement cases. Add targeted transitions at one warmed call site: same-code function replacement with equal/larger/shorter/empty/overlong defaults; different-code replacement with reordered names, narrower arity and changed positional-only boundary; `__code__` replacement; standard/custom vectorcall transition where the internal test API supports it. Confirm generic error messages and complete ownership on misses.

Exercise cache lifecycle explicitly: successful plan → non-Python callable → successful Python plan, successful plan → unsupported shape → successful plan, failed builder → valid builder, and repeated MONO/POLY/MEGA observations. Check that history never revives stale destinations. A host source/generator contract test should reject an escaping or changed version prefix; do not add writable production feedback APIs merely to inject impossible corrupt plans. Debug assertions plus builder/lifecycle tests cover the construction invariant.

Gate the experiment on release/debug focused ownership/call/generator checks and the API gate, then review actual compiled code. Measure whole-path keyword instructions and paired elapsed across 1–3-name permutations, positional prefixes, default holes, larger signatures and fallback controls. Retain the empty-loop raw/net accounting. Candidate 43's net keyword estimate was 989 instructions versus upstream 638.952; removing at most three pair comparisons cannot explain that whole gap. Require a repeatable measured reduction without new application regressions, code/stack growth or changed fallback behavior. Current candidate 46 must finish its frozen qualification before this proposal enters implementation review.
