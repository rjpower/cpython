# W5 direct slot-call implementation packet

The apply-ready final implementation patch is `w5-slot-call.patch`. It was
generated from the W5 release tree at
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/w5`
against root's clean runtime baseline at
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`
(HEAD `58db4324fec`).
It replaces the 17 direct slot calls reviewed in
`w5-slot-call-mapping.md`; direct object-slot calls in the checker now fail
for the nine dispatched families. The checker deliberately reports slot reads
as non-failing findings. This does not close copied/indirect slot calls; Sol's
separate finite inventory review remains required.

The changed direct-call source files are:

- `Modules/_collectionsmodule.c`
- `Modules/_ctypes/_ctypes.c`
- `Modules/_json.c`
- `Modules/_lsprof.c`
- `Modules/clinic/itertoolsmodule.c.h`
- `Modules/itertoolsmodule.c`
- `Modules/mathintegermodule.c`
- `Modules/posixmodule.c`
- `Objects/dictobject.c`
- `Objects/floatobject.c`
- `Objects/odictobject.c`

The typed-base helpers live in `Objects/object.c` and `Objects/abstract.c`,
with declarations in `Include/internal/pycore_value_ops.h`. The same files also
contain concurrent PyValue integration changes from other W5 packets. Likewise
`Modules/_json.c`, `Objects/abstract.c`, `Objects/object.c`,
`Include/internal/pycore_value_ops.h`, `Tools/build/check_value_api.py`, and
`Lib/test/test_tools/test_check_value_api.py` contain unrelated concurrent
work. Integrate those hunks from their own frozen packets rather than applying
whole-file diffs from W5.

Validation on `w5-release`:

- `make -j6 all` completed and the build imported 117 modules with no import
  failures.
- `./python -m test -v test_itertools test_collections test_json test_int
  test_float test_complex test_ctypes test_os.test_posix` passed all 8 files:
  1,480 tests run, 92 skipped.
- `./python -m test -v test_tools.test_check_value_api` passed all 14 tests.
- After the chain thunk correction, `./python -m test -v test_itertools` passed
  140 tests with 5 skipped.
- After the float ratio ownership correction, `./python -m test -v test_float`
  passed 54 tests with 1 skipped.
- `git diff --check` passed.

The patch applies against exact `58db4324fec` commit blobs and reconstructs all
16 selected final W5 files in a short-lived scratch tree. No commit was
created.
