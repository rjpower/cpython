# `_struct` value-pack post-review

Reviewed frozen `long-value-bytearray-provider.diff`, `struct-value-pack.diff`, and `struct-value-pack-tests.diff` against the W5 source. **Verdict: approved for merged qualification; no must-fix source finding.**

The `formatdef.pack` callback type and all 39 `np_*`, `bp_*`, and `lp_*` implementations accept `PyValueBorrow`. The three format tables and native-endian callback substitution use that same type. `s_pack_internal` borrows from the live Clinic value argument window, preserves sequential conversion and destination writes, and keeps the existing writable `Py_buffer` cleanup. The `c`, `s`, and `p` paths guard object projection; numeric readers hold owned `__index__` results through extraction and close them on success and error. Float, complex, truth, pointer, and overflow remapping retain their original callback order and error paths.

The four endian 64-bit packers call `_PyLong_ValueAsByteArray` while their owned integer result remains live. Its stack digit span is consumed synchronously by the existing `span_as_byte_array`, preserving the existing partial destination writes and `with_exceptions=0` behavior. The new test checks those writes on overflow and a complex callback with both `__complex__` and `__float__`.

W5 reported a linked build, 48 `test_struct` tests, and a 16-case differential against an upstream C extension. Merged debug, stats, opposite-endian and 32-bit validation remain integration/platform work; this review does not claim them. The patch is source-review approved for the current Linux build.
