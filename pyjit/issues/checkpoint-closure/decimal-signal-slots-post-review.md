# Decimal SignalDict value slots: source post-review

Reviewed `decimal-signal-mapping.diff`, `decimal-signal-compare.diff`, and the separate comparison regression. No must-fix finding.

The mapping pair passes borrowed keys directly when they are object values and otherwise passes null to `exception_as_flag`, which only compares pointer identity and raises the existing KeyError for invalid keys. Assignment distinguishes the null deletion sentinel from an inline value, then evaluates truth while the caller owns the borrowed value. The object slot thunks and value slots have corresponding signatures and return conventions.

Comparison checks the left SignalDict's attached flags before use; it checks an attached right SignalDict before reading its flags. For a normal dict, `dict_as_flags` may call Python code, so the left attachment is checked again before reading it. Invalid signal dictionaries retain ValueError. Invalid or unrelated operands return NotImplemented as appropriate. The empty-dict result intentionally changes from a preexisting invalid `SdFlagAddr(w)` access to a false comparison, as approved by the coordinator; the new regression covers that behavior and detached SignalDicts.

Producer reports the extension compiled and 69 focused C Context tests plus direct probes passed. This source review did not independently rerun those tests.
