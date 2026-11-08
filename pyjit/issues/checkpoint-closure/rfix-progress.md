# Checkpoint review fixes: rfix

## Scope

Base: `e1229448be3`. This packet closes the three required W4b post-review
findings preserved in `python-boil-the-ocean/build/worktrees/rfix`:

1. `_thread.allocate` and `_thread.exit_thread` must advertise `METH_NOARGS`
   along with `METH_VALUE`, so the vectorcall trampoline rejects positional
   arguments before entering their implementations.
2. `mmap` index conversion and CJK stream-reader size conversion must retain
   `PyLong_AsSsize_t` overflow behavior and wording. The mmap path first applies
   `__index__`; the CJK path already restricts input to `int` subclasses.
3. `_posixshmem` now uses Argument Clinic's value parser after dropping the
   limited-API parser. Its changed short-argument, wrong-type, and C-int/C-long
   diagnostics match current upstream Clinic behavior. Accept and document this
   wording as an intentional parser modernization; do not restore the old parser
   solely to preserve its wording.

The optional W2b `object.__sizeof__` compatibility proposal is outside this
packet and must remain separate from the required review-fix patch.

## Proposed implementation and tests

Use the existing draft changes in `_threadmodule.c`, `mmapmodule.c`, and
`cjkcodecs/multibytecodec.c`; retain or adjust the focused tests in
`test_thread.py`, `test_mmap.py`, and `test_multibytecodec.py`. Validate each
focused test module with the existing rfix release build, and compile the
modified C translation units in release mode. Do not run a full test suite here;
integration owns bulk validation.

## Validation record

Release compilation passed for the three changed C units:

- `Modules/_threadmodule.c`: `build/rfix-release/build2.log:3156`
- `Modules/cjkcodecs/multibytecodec.c`: `build/rfix-release/build2.log:3244`
- `Modules/mmapmodule.c`: `build/rfix-release/build2.log:3248`

The release objects postdate their corresponding source files. The focused
release run passed all three requested modules: 137 tests run, 8 skipped,
with no failures. Captured output:
`build/pyjit-link-wave1/review-rfix-closure/tests-release.log`.

The diff is whitespace-clean. Changes remain uncommitted for coordinator
integration. The optional `object.__sizeof__` experiment remains in three
separate files and is excluded from the required six-file patch.

`_posixshmem` acceptance details from `review-w4b.md`: current Clinic wording
for short and wrong-type calls to `shm_open`, C-int overflow wording for its
flags, `None` rather than `NoneType` in `shm_unlink` type errors, and an
unqualified subclass name all match upstream Clinic output. The old reference
messages came from the limited-API legacy parser. Accept and document these
diagnostics; retain no compatibility parser solely for the old wording.

## Review closure

Independent post-review of the required six-file patch found no must-fix
issues. Final reviewer report pending.
