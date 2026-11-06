# Type descriptor-get value lookup

`r27-type-descr-get-values.diff` is the one-function runtime patch. It replaces the object-form `__get__` lookup with an owned value and passes that value directly to the existing value vectorcall; absence, errors, the guarded lazy slot reset and the None arguments retain their previous order.

The changed translation unit compiled successfully with GCC using release configuration and current runtime headers, with output confined to the task scratch directory. The first compile attempt lacked the source Objects include path and was corrected. `r27-type-descr-get-tests.diff` adds a permanent test for inline noncallable descriptor getters and mutation during lookup; that test passed against upstream. It has not been run against a patched interpreter. The coordinator owns merged runtime tests and any fresh performance or boxing measurement.
