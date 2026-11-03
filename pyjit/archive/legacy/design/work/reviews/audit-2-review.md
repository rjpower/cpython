## Summary

The scanner finding is real, and the proposed audit is useful.  
The design leaves buffer ownership and finalization incomplete, while its suppression rules can hide future omissions.  
This is a source review; execution was blocked by uv’s cache lock on the read-only filesystem.

## Blocking issues

1. **The scanner fix must cover the buffer owner independently of the input.**  
   **Where:** “First reading,” row 11; “Tests and gates”; [sre.c](/home/power/code/cpython/.worktrees/boil-audit2/Modules/_sre/sre.c), `getstring`, `state_init`, and `scanner_traverse`.  
   **Problem:** Row 11 identifies `state.string` as real but leaves `state.buffer.obj` without an explicit disposition. `PyObject_GetBuffer()` can return an owner different from the input object. Keeping `state.string` alive therefore does not establish the buffer owner’s lifetime. The Unicode-only regression cannot detect this omission.  
   **Fix:** Explicitly visit both fields. Add a bytes-pattern test using a Python buffer exporter: its buffer wrapper is a separate object. Collect while only the scanner holds the acquired buffer, then match and inspect the result.

2. **Scanner destruction can execute Python during sweep.**  
   **Where:** “Method,” step 3; “File ownership”; `scanner_dealloc` → `state_fini` → `PyBuffer_Release`; [gc_trace.c](/home/power/code/cpython/.worktrees/boil-audit2/Python/gc_trace.c), `live_destroy_hook_of`; [gc_live.c](/home/power/code/cpython/.worktrees/boil-audit2/Python/gc_live.c), `run_destroy_hook`.  
   **Problem:** Releasing a Python exporter’s buffer can call `__release_buffer__`, which may allocate or run arbitrary Python. The existing collector hooks cover memoryviews and managed buffers, but not scanners. Adding visits repairs marking without repairing this destruction path.  
   **Fix:** Route scanner buffer release through the existing pre-sweep destroy-hook mechanism, with release performed exactly once. Name the collector owner and dependency in the hand-off and ownership sections. Test dropping the scanner while retaining the exporter, with a release callback that allocates and records its invocation.

3. **B5 credits storage layout as though it proved traversal.**  
   **Where:** “The tool fixes,” B5; [traverse_audit.py](/home/power/code/cpython/.worktrees/boil-audit2/Tools/boil/traverse_audit.py), `all_refs`, `last_member`, and visited-member comparison.  
   **Problem:** Delegating to a type’s traverse does not prove that it visits every member of that type’s struct. Furthermore, reducing paths to their final member name lets a visited base `value` hide an omitted `side->value` in the derived object. Struct compatibility also does not establish that the delegated call receives the object being audited.  
   **Fix:** Resolve the actual slot implementation and credit its identified members within the delegated base subobject. Leave unresolved cases reported. Add negative fixtures for a partial base traverse, repeated member names, and delegation on another receiver. Checking only entries removed from today’s report cannot establish this behavior for future changes.

4. **The allowlist disables the new gate for every listed type.**  
   **Where:** “Joining --check (D)”; `read_allowlist` and `check`.  
   **Problem:** The document explicitly accepts that adding a new omitted strong member to an allowlisted type will pass. A prose instruction to inspect later struct changes provides no enforcement, and the stale-entry rule detects only disappearance of the entire finding.  
   **Fix:** Store the exact allowed member paths and subtract only those paths from findings. Update the existing entries to the same format. Test that an additional omitted member fails, and that removing an allowed omission makes its allowance stale.

## Non-blocking issues

- **State the checker’s remaining coverage limits and assign owners.** `member_refs` does not recognize reference-bearing `PyValue` fields. For example, [cellobject.h](/home/power/code/cpython/.worktrees/boil-audit2/Include/cpython/cellobject.h) stores `ob_ref` as a scalar `PyValue`; its pointer encoding can hold a strong edge. Missing traverse bodies and presumed inheritance also escape checking. This audit can remain focused, but an empty report must not imply complete heap-edge coverage.

- **Constrain header lookup and test false suppression.** B3 searches directory headers and all of `Include/`, without specifying include visibility or duplicate-name handling. An unrelated helper can supply member names that suppress a finding. Specify resolution rules and add a fixture with conflicting helper definitions. B4 likewise needs a negative case where naming a function does not invoke it as a traversal callback.

- **Strengthen the regression oracle and verification record.** Conservative stack pointers and stale register slots can retain the scanner target; plain kill mode does not eliminate every conservative root. Add a direct `gc.get_referents(scanner)` assertion alongside the before/after survival test, and construct expected contents without retaining the original target. Explicitly inherit the applicable gates from `PROCESS.md` §3, and record report changes and audit runtime after the expanded helper search.

- **Correct two lifetime explanations.** In row 5, `children == _children` is the alias invariant; shrinking below `STATIC_CHILDREN` does not restore inline storage after a spill. In row 11, the pattern does not permanently retain the scanner’s copied `fail_after_exc`: `_fail_after()` can replace the pattern’s field. The proposed scanner visit addresses that case; add a debug regression for replacement.

## Questions

- When parsing cannot resolve a traverse, or reaches the depth limit, will `--check` report incomplete analysis or silently accept it?
- Will the 6d stream-writer fix land before this audit, or will integration temporarily retain the known premature-free defect behind an allowance?
- Who owns proving the symtable’s `PyMem` roots when 10a rung 3 removes their conservative protection? Row 16 names the concern, but the hand-off does not assign it.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 10:45 PDT
