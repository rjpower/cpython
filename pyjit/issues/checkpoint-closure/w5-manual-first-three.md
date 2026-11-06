# Manual PyValue call and mapping packet: first three files

The apply-ready delta is [`w5-manual-first-three.patch`](w5-manual-first-three.patch).
It is based on exact S source commit `f65d64a398778ecb78c43730d0f8d5cb22e3302a`
and changes only `Modules/_lzmamodule.c`, `Modules/_posixsubprocess.c`, and
`Modules/atexitmodule.c`.

In `_lzmamodule.c`, the `preset` and `id` optional mapping lookups now use
`_PyValue_MappingGetOptionalItemString` and close the owned result on all paths.
The preset conversion preserves `_PyLong_UInt32_Converter`'s native-endian,
`__index__`, negative-rejection, and unsigned-buffer behavior, including its
`OverflowError` wording. `lzma_vli_converter` now converts its owned or borrowed
value directly with those same flags and preserves the existing
`lzma_vli` overflow message, avoiding a temporary box. Object converters for
the other fields remain where the argument parser still needs them.

`_posixsubprocess.c` now calls `preexec_fn` through `_PyValue_Call`. On
successful return it retains the result until `exec`, matching the existing
comment and behavior. `atexitmodule.c` uses `_PyValue_Call` for registered
callbacks and closes the owned return value after successful calls; the
callback, positional tuple, and optional keyword dict remain typed heap
objects.

The three changed translation units compile in the S build. For runtime
validation, only these three files were temporarily shadowed into W5; the
original W5 sources were restored and the release build was rebuilt afterward.
The shadow build linked all modules and passed `test_lzma`, `test_subprocess`,
and `test_atexit`: 505 tests passed, 50 skipped, no failures. W5 release is
available at
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/w5-release`;
the assigned S source and build are
`/home/power/code/cpython/.worktrees/python-jit/build/checkpoint-bulk-20261105`
and `../checkpoint-bulk-build-20261105`.

The broader object-convention converter coverage in `_lzmamodule.c` remains
separate follow-up work; this packet only ports the reviewed manual sites and
the existing VLI converter.
