# CJK value-entry and callback post-review

Reviewed frozen `w5-manual-cjk.patch` against its stated S base. No must-fix
source finding. The module's `getcodec` retains the capsule and constructor
callable through the value call, then closes both owners on every exit.
`METH_VALUE | METH_O` uses the established checked one-argument trampoline,
so arity and keyword errors remain at the method boundary.

The error callback returns an owned PyValue. Both encode/decode consumers
first prove it is a two-item tuple, then hold owned values for both positions
through conversion, buffer writing and callbacks. Object projection is only
performed after proving the replacement is Unicode/bytes; inline integer
positions remain values. Null and malformed returns reach the existing error
paths, and all acquired owners close there. This removes the tuple-item box
without changing the callback's object ABI or its error text.

The seven affected CJK translation units compile. Runtime behavior has not
been validated because the worker's private build stopped earlier in
`Parser/pegen.c` on an unrelated `PyObject_Vectorcall` mismatch. Before
integration, run the focused multibyte codec tests, including custom handlers
returning inline positions, invalid tuple/replacement/position values and a
handler that re-enters Python while result owners remain live.
