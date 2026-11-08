# Float value-format packet receipt

This packet adds a borrowed-value float advanced writer and routes
`float.__format__` and exact-float `str.format` fields through it. The source
base is the saved runtime `84ed` preimage plus the phase-1 `floatobject.c`
state; the source snapshot is retained under `build/r27-float-format-84/`.

The base patch is `w5-float-value-format.patch`. Its additive review correction
is `w5-float-value-format-review-fixes.patch`. The correction removes the
unreachable exact-object float formatter branch and passes live `PyTypeObject *`
values to format-spec parsing. The float writer reads `tp_name` when it emits
an error instead of retaining a cached name pointer. The object, long and
complex parser callers now pass their live type objects as well.

`git apply --check` passed for the base patch against its saved preimage and
for the additive patch after the base. The three affected translation units
(`floatobject.c`, `unicode_formatter.c`, and `unicodeobject.c`) passed
release-flags syntax-only compilation against current-main headers, with the
packet's updated float and string-format headers first in the include path.
The earlier private W5 object compile built `unicode_formatter.c` and
`unicodeobject.c`; `floatobject.c` stopped at the pre-existing phase-1
`.new_` initializer because that W5 header snapshot predates the slot. No
linked runtime tests or performance probe were run. Merged release/debug
compilation, `test_format`, `test_float`, `test_str`, and the focused box-count
probe remain required.
