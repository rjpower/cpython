# Keyword-plan invariant proposal review

**Approve the bounded experiment after candidate 46's frozen work closes.** This is a better small implementation candidate than beginning the larger owning-entry provider before its census and ownership proof are complete. It is not promotion approval and does not authorize removal of mutable hit-time guards.

Reviewed the 57-line proposal against the actual destination/flag writers, version assignment/invalidation, feedback reset/history, generated binder call sites and guarded-prefix validator. No runtime edits or native work were performed.

## Proof checked

`specialize_keyword_binding` is the only publisher of keyword destinations. It computes and validates a local array, including bounds, positional-only exclusion, positional-prefix exclusion and distinctness, checks coverage, copies destinations, and only then writes keyword/count/self flags. `specialize_py_call_kw` first clears flags and only calls that builder for an ordinary Python function. Positional specialization clears flags before writing the shared union. `fb_begin_ex` resets the complete call payload by `sizeof`, and new execution feedback starts zeroed.

Call poly/history rows retain versions and handler identity, not destination plans. The inspected snapshot/reset routes cannot revive a stale keyword plan. The only generated calls to `call_bind_known_python` are the guarded keyword and positional sites. The generator validator checks their exact nonescaping PEP 523/function-version/recursion prefix and terminal invoke shape.

Code versions are assigned from the interpreter's monotonic counter; overflow leaves it zero rather than reusing valid versions. MAKE_FUNCTION assigns the function version from its code, and function code replacement clears the version before installing new code. Thus valid equal versions pin the same code **within the relevant interpreter**, not merely a similar signature. Version-cache collisions/eviction do not recycle the actual version. Same-code functions can have different defaults, which is why the current-default proof remains essential.

The retained code's site argument count, cached keyword count and checked self-presence determine the same positional prefix used by construction. Names may change, but the remaining per-position exact-string/identity checks ensure they match the names belonging to the cached destinations. Consequently the proposed destination well-formedness checks are construction invariants, not mutable facts needing repeated release guards.

This proof is for the existing GIL-only route. It does not authorize restoration of plans from history, a new unchecked binder caller, an escaping prefix, or shared execution across interpreters without the corresponding version/ownership protocol.

## Exact implementation scope

Remove only the release destination bounds/positional-only/prefix tests and the repeated earlier-destination comparison loop. Keep their debug assertions **before** the destination-indexed `co_localsplusnames` read. Make debug-only bounds temporaries and duplicate-check loops explicitly debug-only where needed, avoiding unused-variable warnings or residual release work.

Retain the keyword discriminator/count bound, thread-owner admission, self check, positional sanity, exact tuple/count/name checks, current function's standard vectorcall, executing-code/stack-space checks, and current-default size/required-argument coverage. Retain the generator-validated PEP 523/function/version/recursion prefix. Exact-name checks also protect callback-free names retirement and must not be converted into arbitrary equality calls.

Keep every transfer/publication statement, feedback layout, builder, default proof, fallback and completion ABI unchanged. Do not add runtime code/name pointers or a second binding plan. A short comment should connect trusted destinations to construction plus code/site/self identity, without saying that version equality proves defaults.

No blocking design revision is needed. The actual diff still requires review, including proof that the removed checks have not taken mutable guards with them.

## Tests and measurement gates

Reuse existing binding/ownership tests rather than duplicating their entire matrix. Add targeted warmed-site transitions where coverage is absent:

- Same-code replacement with changed default shape must retain current-default semantics; different-code replacement with changed arity, name order or positional-only boundary must miss the version proof before indexed access. `__code__` replacement must invalidate.
- Valid plan → non-Python/unsupported callable → valid plan, and declined builder → valid builder, must rebuild safely; repeated observation history must never restore destinations.
- Permutations, self-presence changes, prefix overlap and duplicate/unexpected names must retain results/errors through the existing fallback. Explicit keyword-only values remain independent of changed `__kwdefaults__`.
- Keep generator-prefix negative tests rejecting an escaping or altered version guard. Do not add a production feedback-mutation API to simulate impossible corrupt states.

Run release/debug focused call/ownership checks and the API gate; inspect actual compiled keyword and positional bodies for instruction/stack/footprint changes. Then compare whole-path instructions and paired elapsed across 1–3-name shapes, prefixes, holes and fallback controls, retaining raw versus skeleton-subtracted accounting. Use the qualified current checkpoint as control, not an earlier micro result whose layout differs.

The observed candidate 43 keyword cost was about 1081 raw/989 net instructions versus upstream 720/639. Destination validation is only part of that sequence: at capacity 3 there are at most three pairwise comparisons, plus scalar bounds checks. This proposal may save useful repeated work, but neither the whole gap nor an application-wide improvement follows from the source proof. Retain it only on measured benefit without new regressions. Continue the separate owning-entry census/proof work without widening this patch.
