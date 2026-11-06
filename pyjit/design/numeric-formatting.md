# Numeric formatting

Value-native numeric formatting removes core-owned boxes when Python formats
inline floats. The first increment covers `float.__format__`, exact-float
fields in `str.format` and the shared float advanced writer. Integer and
complex formatting remain separate work.

`_PyFloat_FormatAdvancedWriterValue` accepts a borrowed `PyValue` and preserves
the existing format parser, conversion rules, locale handling and writer
behavior. The object writer delegates through a borrowed object value for
object callers. The float method and exact-float field path call the value
writer directly. The field path checks inline floats before attempting an
object projection; its field owner remains live across formatting callbacks.
The float formatter uses `_PyValue_AsDouble` and `_PyValue_Str` for its value
input, without materializing an object for numeric output or an empty format
specification.

Error messages must read `tp_name` at the point the parser or formatter emits
the error. A heap type can change its name during callbacks, so callers pass a
live `PyTypeObject *`, not a cached name pointer. The borrowed value or object
owner keeps that type alive. Keep the old object-only writers for object
callers; they are adapters at the actual object API boundary.

The first increment does not close integer formatting, float conversion in
the long formatter, complex formatting or Decimal formatting. Merged release
and debug validation must cover `test_format`, `test_float` and `test_str`,
including subclass overrides, invalid-spec error order, signed zero, special
values and format fields. Performance admission requires a focused stats
probe showing no new core float boxes at the method and field entry points.

Implementation and review evidence: [initial patch](../issues/checkpoint-closure/w5-float-value-format.patch), [review fixes](../issues/checkpoint-closure/w5-float-value-format-review-fixes.patch), [validation receipt](../issues/checkpoint-closure/w5-float-value-format-receipt.md).
