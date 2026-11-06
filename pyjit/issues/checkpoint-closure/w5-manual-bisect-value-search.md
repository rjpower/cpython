# Manual bisect value-search port

Patch: `w5-manual-bisect-value-search.patch`

This packet ports the shared left and right binary-search helpers and their callers to `PyValue` operands/results. It preserves sequence-item access boundaries, same-type rich-compare dispatch, truth coercion, key-callback order, and recursion/error behavior. Search results and callback values remain owned values through their consumers; the existing insort argument compatibility path remains unchanged.

Baseline: main runtime commit `d6eae4b1c1c200444b325240a00791b44288b1e3`. The patch applies cleanly to that commit.

Validation: D compiled `Modules/_bisectmodule.o`. Focused precomputed-key, key-callback, non-boolean comparison, and `NotImplemented` comparison tests passed in the W5 release build. `git diff --check` reported no whitespace errors.
