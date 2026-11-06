# Manual time, codec, and random value continuations

The next three residuals must use value-native continuations rather than an object projection inside a core helper.

`time_strptime` should become an actual `METH_VALUE` entry. With two arguments, call `_PyTime_Strptime` only when both values are proven objects; that helper accepts exact ASCII Unicode and returns 0 for other objects. Build the parsed `struct_time` with inline value integers in its slots. For fallback, retain the imported `_strptime._strptime_time` callable and invoke `_PyValue_Vectorcall` with the incoming value argument array, close the callable, and return the owned result. Keep the old no-keyword behavior.

`multibytecodec.call_error_callback` is private and should return an owned `PyValue`. Its two consumers first verify that the returned value holds a two-item tuple, then take owned item values. The replacement item must be a proven Unicode/bytes object for encoding or Unicode object for decoding; the position needs only `PyValue_LongCheck` and `_PyValue_IntAsSsize_t`. Keep the tuple and item owners through buffer writes and position checks, close them on every exit, and preserve the existing TypeError and bounds messages. A failed tuple-item read retains its original exception.

`Random.seed` should retain the result of the base `int.__abs__` call as an owned value, derive `_PyLongSpan` while that owner lives, then use `_PyLong_SpanNumBits` and `_PyLong_SpanAsNativeBytes` with little-endian unsigned-buffer flags to fill the existing 32-bit key array. Keep the existing big-endian word reversal. For a non-int input, hash it and pass `(uint64_t)(size_t)hash` directly to `init_by_uint64`; creating an intermediate integer changes the representation path and adds a box.
