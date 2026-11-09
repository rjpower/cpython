# Initialized frame arena: independent code review

2026-11-09. Reviewed the initialized-arena source delta in runtime checkout
`build/worktrees/execution-boundaries-20261108`, based on `6ef45e391aa` plus
uncommitted changes. Read the applicable agent guidance, CPython AI policy,
arena proposal, design review and retirement census. Runtime source was not
modified, compiled or executed by this reviewer.

**No must-fix finding in the arena delta. Source review supports integration
testing; it does not establish native correctness or a performance result.**
The required release compile and merged native post-review remain with the
coordinator. Compiler/constructor exception-owner changes were outside this
review.

The scope covers `Include/cpython/pystate.h`, the reservation/initialization/pop
changes in `Include/internal/pycore_interpframe.h`, allocation changes in
`Python/pystate.c`, heap `init_frame` in `Objects/frameobject.c`, the dirty-arena
helper in `Modules/_testinternalcapi.c` and `Lib/test/test_frame_arena.py`.
Direct callers and retirement/copy implementations were inspected to check
their contracts; unrelated semantic and remote-reader changes were not reviewed.

## Checks and conclusions

- All three physical reservation paths establish storage before their first
  header write: unchecked push, overlapping push and new/cached chunk push.
  The helper truncates the frontier before filling an uncertain suffix. Its
  capacity precondition separates values strictly from the new header.
- Overlap uses the maximum of the child end and existing physical top. This
  preserves an ancestor's initialized tail when the child is smaller. Live
  transferred arguments remain below the prior frontier and are not cleared.
  The existing debug ownership assertion for the shared caller tail remains.
- Fresh chunks start the frontier at data. Cached reuse retains the existing
  frontier, and neither root pop nor chunk pop restores a stale frontier.
  Former header storage therefore becomes uncertain until explicitly filled.
- Allocation sizing includes the changed data offset, requested values,
  header, spare `owned_slots` cell and strict separation cell. The computation
  uses `size_t`; the existing checked-push size contract bounds the addition
  and multiplication. Doubling rejects the next step before signed overflow.
- The complete initializer and push caller census has matching signatures,
  including generic entry, getitem/property entry and generated call cases.
  Trampolines receive the same reservation guarantee. Direct heap construction
  fills its own storage once; complete escape/generator copies need no prefill.
- Both production direct pop paths preserve NULL source storage: ordinary
  retirement clears locals after scratch retirement, while RETURN_GENERATOR
  moves every home through `_PyFrame_Copy`. Escape also moves every home.
  The physical stack pointers continue to reserve a departing frame while
  decrefs can re-enter Python. The new debug pop check runs before release and
  does not consult the already-cleared code owner.
- The dirty allocator hook is restored immediately after each checked push,
  including allocation failure. Explicit cleanup uses the original allocator
  and recorded allocation sizes. The subprocess owns its fake thread state;
  no allocator hook or allocated chunk survives the helper's normal return.
- The former-header probe writes real header `base`/`saved_top` fields before
  reusing their addresses as values. It checks NULL contents and frontier
  coverage. Assertions remain enabled in release test builds because
  `_testinternalcapi.c` undefines `NDEBUG` before including internal headers.
- Python tests cover changing frame sizes, provider entry, binding/recursion
  failure, heap construction, escape, generator suspension and finalizer
  re-entry with eventual exactly-once retirement. This is source coverage
  assessment, not a claim that those tests have passed on this increment.

No executable cleanup instruction or alternate runtime representation is
introduced by this delta. Warmed calls can reuse a proven NULL tail without
the removed unconditional frame-tail initialization. Whether the extra
frontier guards improve logging or broad workloads remains a measurement
question after merged correctness checks.
