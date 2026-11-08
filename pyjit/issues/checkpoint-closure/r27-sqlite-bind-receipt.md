# SQLite value-native binding

The review draft `r27-sqlite-bind-values.diff` applies to runtime base `af82c03c4e5`. Sequence and named parameters stay owned values through adaptation and SQLite binding. Adaptation keeps registered adapters, protocol and conform callables alive across calls, and distinguishes omitted alternate from explicit None. The object-form int64 helper remains for the two `connection.c` callers.

A locked W5 release shadow build and restoration each checked 117 modules with zero import failures. `test_sqlite3` passed 527 tests with four skips. `r27-sqlite-bind-probe.py` produced identical output to upstream for int64 boundaries and overflow, scalar/subclass and buffer binding, mapping lookup, registered and conform adapters, protocol callbacks, raised exceptions, and omitted/None/numeric alternate values. The initial compile found a missing `pycore_value_call.h` include, which the review draft corrects.

The narrow-long-long fallback is source-reviewed only on this 64-bit host. Merged testing, raw box measurements, and the two connection object-form helper call sites remain for integration.
