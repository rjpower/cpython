# W5 startup box reduction packet

The apply-ready source delta is [`w5-startup-boxes.patch`](w5-startup-boxes.patch),
generated against runtime commit `58db4324fec`. It changes only
`Objects/dictobject.c`, `Modules/_io/_iomodule.c`, and
`Modules/_io/winconsoleio.c`.

`dict_value_keyerror` now calls `_PyErr_SetKeyErrorValue` directly, preserving
the existing `KeyError` construction without boxing an inline key. `_io.open`
keeps `path_or_fd` as an owned `PyValue` through the raw-I/O constructor call,
uses `_PyValue_Vectorcall` with an owned four-value argument window, and clears
the path owner before later fallible work. The Windows console selector has a
value entry for inline integer descriptors; object-valued paths and numeric
objects continue through the existing selector.

The first independent review found no remaining Linux must-fix issue after the
owner-clear correction. A later review caught that clearing `path_or_fd` on
the success path leaves a NULL sentinel which the shared error label must
close safely. The patch uses `PyValue_XCLOSE` there; the error path still
closes a live owner when present. The review notes that Windows'
`_WindowsConsoleIO` initializer remains object-convention Clinic code, so a Windows call adapter
may still box inline descriptor arguments. The source patch removes the
`_io_open_impl` CORE box; it does not establish zero total Windows boxes. This
platform case remains an admission check if Windows is included in the gate.

The W5 release build compiled the changed dict and I/O units and linked all
117 available modules. Focused `test_dict` passed 154 tests; `test_io.test_file`
passed 36 tests with two skips. Targeted `io.open` checks passed for a pipe file
descriptor with `closefd=False`, an `int` subclass descriptor, string-returning
and invalid-returning `PathLike` objects, and a custom opener. After the nullable
cleanup fix, release and debug builds both passed a failure-after-RawIO probe:
`io.open(fd, "r", buffering=0, closefd=False)` raises the expected `ValueError`
and leaves the descriptor usable (`os.fstat(fd)` succeeds). Both builds passed
`make -j6 all` and checked 117 modules with zero import failures. A fresh W5
stats process run as
`-I -S -X pystats -c pass` reports zero startup CORE boxes: dict 0, parser 0,
down from the measured baseline of dict 1 and parser 3. The frozen patch was
dry-run and applied in a scratch copy of the exact `58db4324fec` source blobs.
