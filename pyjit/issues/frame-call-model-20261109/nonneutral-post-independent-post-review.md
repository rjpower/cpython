# Independent post-review: committed POST and tracing borrows

The final source diff contains only `Objects/frameobject.c`, five focused
`Lib/test/test_code.py` methods, and the six borrowed reads in
`Python/legacy_tracing.c`. The stopped-state proof still uses the generated
positive observer outcome, actual edge/successor and exact scope generation.
It transfers one request-owned site row, cuts old origins independently of
physical replacement, excludes shared-parent token creators, and admits only
inherited/empty-save/owner-free targets. Source-only fallback binds a separate
conservative PRE row. Warning callbacks recapture and rebuild before commit.
The tracing change passes borrowed views of the three live arguments to the
six declared borrow APIs without changing ownership or STOP filtering.

Root's `ownership32-observer-core-focus-checks.json` records release and debug
return code 0 on the three-file packet before the allocation warning cleanup.
Each corresponding log reports 14/14 selected modules, 455 tests run, one
skip, and no failures. The seven-method
minimal rerun also passes in both profiles, including `test_call_text` for the
legacy tracing borrow contract. Both build receipts return 0 and report 117
modules checked. The first minimal run's two negative tests never entered
their callbacks because they registered BRANCH_LEFT; the retained actual-IR
probe shows their true conditionals are BRANCH_RIGHT. The final test-only
correction registers RIGHT and retains refusal/state assertions. This is a
bounded engineering gate, not full-suite or performance qualification.

Both initial build logs exposed a new signedness warning from
`Py_MAX(count, 1)` with `size_t count` in the new row allocation. B changed the
two uses to a shared `size_t alloc_count = count ? count : 1;`; I reviewed the
exact fix and found it preserves the overflow check and logical copy extents.
Root's `ownership32-warning-fix-{release,debug}-build.log` both report 117
modules checked and omit the two new allocation warning sites. Its
`ownership32-minimal-v3/{release,debug}.log` each pass all seven selected
methods again. The wider 455-test gate preceded this one-line source cleanup;
it has not been rerun after it. Older warnings at other `Py_MAX` sites are
outside this patch. No source blocker remains for a bounded engineering
commit, with wider qualification still explicit future work.
