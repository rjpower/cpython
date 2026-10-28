# Cleanup after the architecture work

Shortlist from the 2026-10-28 architecture review at `4bb672e0bf4`.
Recheck each item against the completed implementation before changing it.
These are deferred recommendations, not additions to the current rung gates.
[PROCESS.md](PROCESS.md) remains the plan of record.

## 1. Make C identity agree with value identity

- [ ] Make both the `Py_Is` macro and exported function agree with Python
  `is` and `PyValue_Is` for boxed numerics. The macro in
  [object.h](../Include/object.h) and function in
  [object.c](../Objects/object.c) currently compare pointers.
- [ ] Audit core identity comparisons and update those that mean Python
  identity; preserve pointer comparisons that mean storage identity.
- [ ] Update the identity contract in [pyvalue.h](../Include/pyvalue.h),
  which currently explicitly permits the discrepancy. Cover the macro,
  function and Python paths with the same cases: separate boxes of equal
  int32s and floats, signed zero, NaNs, large integers, bools and subclasses.
  Preserve existing NULL behavior and evaluate macro arguments once.

Canonical numeric identity is intentional: separately constructed equal
inline numerics may satisfy `a is b`. Keep that behavior. The cleanup is
consistency across APIs, using canonical identity rather than numeric `==`
(which would conflate signed zero and distinct numeric types).

## 2. Close the native ABI identification gap

- [ ] Reconcile the `boil2` SOABI marker with upstream-shaped wheel tags
  ([configure.ac](../configure.ac)) and untagged `.so` acceptance
  ([dynload_shlib.c](../Python/dynload_shlib.c)). Give packages a distinct
  compatibility identity and enforce an ABI-generation check when loading
  native modules. Verify that foreign and older binaries fail cleanly,
  while extensions built for the settled fork load normally.

## 3. Enforce representation and platform assumptions

- [ ] Enforce the below-2^48 pointer requirement for values and header shape
  pointers. Alignment assertions and a 64-bit build check do not establish
  that bound ([pyvalue.h](../Include/pyvalue.h),
  [object.h](../Include/object.h)). Include static and externally allocated
  objects in the contract; reject unsupported addresses before truncation
  or tag interpretation.
- [ ] Publish the supported platform/compiler/unwind matrix. Distinguish
  precise stack walking from conservative fallback; the current walker is
  restricted to x86-64 ELF with suitable glibc
  ([pycore_gc_walk.h](../Include/internal/pycore_gc_walk.h)).

## 4. Make verification summaries match the required checks

- [ ] Have one manifest or driver enumerate required ordinary, kill, ASan,
  stress and repeated-probe runs, with exclusions, reasons and owners.
  [gate.sh](tools/gate.sh) level 3 does not itself execute the full
  verification set required by [PROCESS.md](PROCESS.md).
- [ ] Generate summaries tied to source commit, build configuration and
  exclusions. Distinguish ordinary-suite success from completion of the
  collector verification set. Restore temporary skips as their owners'
  fixes land; retain separate records of intentional behavior changes.

## 5. Settle the allocation behavior of compatibility accessors

- [ ] Audit object-form accessors that now allocate when boxing and abort
  if boxing fails, through `_PyValue_ObjectOrAbort`
  ([pyvalue.h](../Include/pyvalue.h)). Migrate remaining core callers to
  fallible value APIs where practical. Document the residual C API contract
  and exercise allocation failure explicitly; do not imply that these are
  allocation-free borrowed reads.

## 6. Reconcile documentation with the finished implementation

- [ ] Mark [PR.md](PR.md) historical or rewrite it: it still describes
  first-call IR lowering, optional shapes/tier 0 and a per-change ports gate.
- [ ] Reconcile note 06's opening promises with its as-built record,
  including residual data-movement instructions. Document the accepted
  4,095-slot restriction and its generated-code consequences, or revisit
  the encoding if that limit is no longer acceptable. A test of rejection
  does not establish preservation of the old large-function behavior.
- [ ] Publish final wall-time, RSS and collector-pause measurements against
  upstream, with cold and allocation-heavy workloads. Keep historical
  waypoint measurements clearly dated.

## Existing work to check at closeout

These already have owners; keep them in their current steps rather than
postponing correctness fixes into cleanup:

- **7a:** interpreter ownership of weakref callbacks and finalizers; restore
  the skipped shared-heap subinterpreter reproducer.
- **10a:** remaining precise roots, PyMem/bytes-writer lifetimes,
  conservative-retention skips and `PYTHONMALLOC` behavior.
- **7:** collection/retry on allocation failure and collector policy.
- **9:** linear string append behavior after removal of refcount-based
  uniqueness; verify the common cold path as well as optimized traces.
- **10b/P:** explicit handle discipline before movement, followed by ports
  against the settled API.
