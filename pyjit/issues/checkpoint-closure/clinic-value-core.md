# Clinic value-core object wrappers

`Objects/stringlib/transmogrify.h` is included in value-native bytes and
bytearray units, but its generated wrappers keep their object signatures. The
existing `value_emission` directive would also change those signatures and is
not suitable for this shared header. Regenerating the wrappers without another
declaration emits marked object-number conversions and breaks the static
port.

`value_core` declares that a Clinic file emits object-signature wrappers only
for a `Py_VALUE_CORE` unit. It does not select `METH_VALUE`, change argument
or result signatures, or expose the output to a non-core unit. It causes object
arguments of scalar converters to use their existing value readers on borrowed
canonical values. The generated header carries the core-only guard. The
directive is rejected in limited-API mode, after a function, when repeated,
or alongside `value_emission`. Existing external object Clinic files retain
their old code.

The initial producer supports object-wrapper int, bool-as-int, long and
unsigned conversions using their existing value twins. `_curses`, `_ssl`,
`gc`, `syslog` and the shared stringlib outputs are regenerated from the
producer. Do not hand-edit generated Clinic products. Peer review approved the
interface; the candidate needs post-review of the exact source and output.
Focused checks cover generator mode/signatures and bytes/bytearray numeric
conversion, type errors and overflow. The remaining signal handler's
object-storage bridge is separate explicit R27 debt.
