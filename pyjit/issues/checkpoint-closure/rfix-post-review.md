# Required W4b review fixes: post-review

Independent review, 2026-11-05, of the six required files in the preserved
`rfix` worktree at base `e1229448be3`. Result: **no must-fix findings** for
this bounded patch. The separate `object.__sizeof__` experiment in
`pycore_long_value.h`, `longobject.c` and `typeobject.c` is outside this review
and should not be included in this landing.

`_thread.allocate` and `_thread.exit_thread` now declare `METH_NOARGS` with
`METH_VALUE`; the trampoline rejects a supplied argument consistently with
their existing aliases. The test checks each exposed name. `mmap` applies the
value `__index__` operation, converts its owned integer result with the int
extractor that reports upstream `Py_ssize_t` overflow, and closes that result
on success and failure. The CJK reader already restricts `size` to int or a
subtype; its extractor change corrects the overflow text without widening
accepted arguments. Focused tests include both positive and negative
overflow cases.

The three changed C units compiled in the release build. Its saved focused
run passed all three test modules (137 tests, 8 skipped). The six-file diff is
whitespace-clean. The `_posixshmem` wording difference requires no code change:
the new value parser uses current Argument Clinic diagnostics, as detailed in
`rfix-progress.md` and the W4b review. This is an intentional parser outcome.

This review covers the component patch. The coordinator must validate its
composition after integration.
