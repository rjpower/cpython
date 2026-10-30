# Selective death processing

This increment starts at `070ce55ea16`. Implementation `788695e30e9`
classifies potential cleanup work during the existing heap walk:

- Weakref work uses the registry flag, retaining the static builtin type's
  separate list case.
- Finalizer roots exclude already finalized objects and finished generators
  whose finalizer is a no-op. All objects remain available as ordering
  connectors; delivery still checks current eligibility.
- An independent exact memoryview/managed-buffer bit gates retention work and
  avoids repeated type tests. Current death and release-readiness checks remain.

There are no new allocations or worklists, callback cadence changes, ownership
transfers, or collector budget changes. Existing OOM rollback remains intact.
This is category selection, not compact stage lists or a measured speedup.

The fresh GCC O1/Py_DEBUG build passed its audits and 116 module checks, with
zero import failures and ten missing optional modules. Tests used actual
kill/extras0/N0. The finite controls passed 13/0 skips; the four whole modules
(`test_gc`, `test_weakref`, `test_memoryview`, `test_buffer`) passed 599/46 skips.
These scopes overlap. They recorded 219 and 10,784 collections respectively,
with zero unwind fallbacks. The whole-module run reported one unpinned detach
record, consistent with the passing `StackWalkTests.test_unpinned_counter`
control; its conservative scan is distinct from an unwind fallback. The module
aggregate is not a separate per-case trace. Original products, configuration,
tools and post-cleanup guards passed.

The first new test incorrectly expected a callback-free weakref to clear
before resurrection. Its failure and twelve passing controls remain preserved.
Pinned upstream `321925c92c9`, `Python/gc.c:762–800`, deliberately delays that
clearing through finalization (GH-135552). Test-only successor `69c34fb1f66`
asserts the surviving plain ref's identity and usable data, callback-bearing
ref clearing, eventual clearing of both refs, and single finalization. Native
products remain from `788695e30e9`; no runtime repair or rebuild was needed.

The immutable bounded index is
`build/selective-death-resurrection-validation/completion.json`, SHA256
`f463bf790acbd55efd20bd254763cfb168bbb6d7e9ff1907793f85ea7a95e2ea`.
O3 measurement, broad qualification and landing remain separate decisions.
