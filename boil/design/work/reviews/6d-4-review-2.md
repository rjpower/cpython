## Summary

The in-place signatures and whole-family commits fit the architecture.  
Clinic’s deletion handling and several boxing paths remain underspecified.  
The inventory and classifier counts reproduce; no implementation gates were run.

## Blocking issues

1. **Clinic’s deletion-capable setters need a separate conversion rule.**  
   **Where:** §D; [parse_args.py:520](/home/power/code/cpython/.worktrees/boil-6d/Tools/clinic/libclinic/parse_args.py:520). Updating `SETTER_PREAMBLE` covers setters that reject deletion. `SETTER_AND_DELETER` instead emits `if (arg != NULL)` inside `render_setter_value`; the listed changes leave that comparison invalid for `PyValue`. Removing the guard would make object conversion treat deletion as a failed box. This affects `frame.f_trace`, `function.__annotations__`, and ctypes accessors.  
   **Fix:** specify `!PyValue_IsNull(arg)` around conversion, retain the impl parameter’s `NULL` initialization, and return before the impl on boxing failure. Also specify boxing for the `PyArg_Parse` fallback when `parse_value_arg` returns `None`. Add generator cases for deletion, `None`, ordinary assignment, and conversion failure.

2. **The conversion rules leave boxing behind shared helpers without naming its retirement.**  
   **Where:** §§D, E.0–1; [bufferedio.c:692](/home/power/code/cpython/.worktrees/boil-6d/Modules/_io/bufferedio.c:692), [textio.c:765](/home/power/code/cpython/.worktrees/boil-6d/Modules/_io/textio.c:765), [socketmodule.c:3347](/home/power/code/cpython/.worktrees/boil-6d/Modules/socketmodule.c:3347). Keeping Clinic impls unchanged preserves object-protocol round trips: `_Buffered.name` calls `PyObject_GetAttr`, and `TextIOWrapper.name` delegates through `buffer_getattr`. An inline result is boxed there and canonicalised again by the new parser. Separately, E.1 leaves `socket.timeout` calling `sock_gettimeout_impl`, which constructs a float object. Neither case belongs to D’s named constructor deferrals.  
   **Fix:** convert forwarding impls and their shared helpers to values, using Clinic’s existing `-> value` converter where appropriate. Make the socket helper return a value and adapt its object consumer explicitly. Alternatively, name each deferred accessor, its allocation budget, and its retirement step. Add discriminating audit cases for `BufferedReader.name` backed by an uncached integer and for a non-`None` socket timeout.

## Non-blocking issues

- **The deferred numeric-getter list is incomplete.** §D omits `frame.f_lineno`, `frame.f_lasti`, `_ThreadHandle.ident`, and `_SSLContext.minimum_version`, `maximum_version`, and `protocol`. Include these in the promised per-module 6e manifest; note 06’s current 6e row contains no such manifest.

- **The statx dispatch is misidentified.** §C describes tuple construction, but [posixmodule.c:3568](/home/power/code/cpython/.worktrees/boil-6d/Modules/posixmodule.c:3568) is `pystatx_result_repr`. Specify preservation of its `None` omission and checks for both getter failure and boxing failure. Exercise `repr(os.statx(...))`.

- **The collection witness depends on the chosen entry point.** Tests 4 proposes a `gc.callbacks` counter. [gc_live.c:1433](/home/power/code/cpython/.worktrees/boil-6d/Python/gc_live.c:1433) sets `no_python` for inline collections, suppressing those callbacks. Specify explicit `PyGC_Collect()` for that witness, or use an injection counter. Assert collection occurred between boxing and publication.

- **Complete the validation ladder.** Explicitly schedule the release full-suite gate 2 required by `PROCESS.md`; gate 1 on release and gate 3 on debug do not replace it. Add allocation budgets for numeric setters, such as `characters_written` and `_CHUNK_SIZE`, since the proposed setter audit currently proves only cell storage.

## Questions

- Are unsigned Clinic setters intentionally allowed to box? `Cursor.arraysize`’s `uint32` converter and `_ssl`’s unsigned converters inherit object-based conversion paths. §D’s claim that numeric converters read values directly needs either narrower wording or additional converter work and a named owner.

- Which Wasm trampoline signatures and arities will be reviewed? Note 12 §5 requires each rung to identify its unbuilt trampolines; the module-coverage section currently omits Wasm.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 06:49 PDT
