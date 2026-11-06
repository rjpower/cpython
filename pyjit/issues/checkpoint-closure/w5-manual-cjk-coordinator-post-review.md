# CJK manual calls: coordinator post-review

Reviewed the frozen `w5-manual-cjk.patch` against the proposed value-call and
callback continuation contracts. No must-fix finding; approved for integration.

The shared `getcodec` entry becomes an actual `METH_VALUE|METH_O` function.
It checks the encoding value before projecting Unicode, retains the codec
capsule and callable across the callback, and returns its arbitrary owned
value. Capsule allocation failure releases both the allocation and callable.

Encoding and decoding error callbacks return owned values. Consumers check
for a two-item tuple before projecting the tuple, keep independent item owners,
check replacement text before projection and extract the position from its
integer value. Success and error paths close tuple/items exactly once. The
existing bounds and error messages remain at the same points; this patch does
not invent new error precedence. The concrete exception and callable object
APIs are retained where their results are proven objects.

Luna reports compilation of the seven affected codec objects, a W5 shadow
release build with 117 modules and no import failures, and 486 tests with four
skips across multibyte codecs, callbacks and the six CJK encoding suites.
Original W5 sources were restored and rebuilt afterward. Coordinator merged
debug/release qualification remains outstanding.
