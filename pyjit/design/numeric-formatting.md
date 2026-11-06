# Numeric formatting

Value-native numeric formatting removes core-owned boxes from float and integer
formatting paths. The float and long value writers are integrated at runtime
HEAD `6a0e4c12c17`; complex and Decimal formatting remain separate work.

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

The float writer's source-level review and validation receipts are historical
evidence for that increment. The integrated integer writer passed a 412-case
upstream differential; 14 focused startup/hot probes reported zero boxes, and
the release and debug full suites each passed 54,351 tests. These results do
not qualify the overall runtime: the full R27 census still reports
11,028,818 core/core-pending/unresolved boxes, and the kernel gate fails. The
current [full census](../issues/checkpoint-closure/full-census-6a0e4c12c17.md)
and [kernel receipt](../issues/checkpoint-closure/kernels-6a0e4c12c17.md)
control current status. The completed [diagnostic measurements](../bench/results/checkpoint-6a0e4c12c17.md)
do not qualify performance while R27 fails.

Implementation and review evidence: [initial patch](../issues/checkpoint-closure/w5-float-value-format.patch), [review fixes](../issues/checkpoint-closure/w5-float-value-format-review-fixes.patch), [validation receipt](../issues/checkpoint-closure/w5-float-value-format-receipt.md).

## Integer continuation in the current candidate

The reviewed implementation is integrated at `6a0e4c12c17`. Its
borrowed-value advanced integer writer feeds inline
integers to the existing decimal and binary digit kernels using a local
`_PyLongSpan`. The span and its digit storage stay live for each synchronous
writer call. Heap integers retain the existing paths, including large-number
limits and the `_pylong` threshold. Exact integer format fields and
`int.__format__` use the value writer; subclasses keep their existing dispatch.
The object advanced writer remains the object entry and has a narrow value
alias. The broader `_PyLong_FormatWriter` remains unmarked while its other
callers exist.

The parser, sign, grouping, locale and `c` validation order remain shared with
the existing implementation. Float-style integer formatting holds the owned
converted float until formatting completes. The [implementation receipt](../issues/checkpoint-closure/r27-long-format-values-receipt.md)
and [design proposal](../issues/checkpoint-closure/long-advanced-format-values-proposal.md)
record the implementation history and design. The merged 412-case differential
is in [the AST/type-call validation receipt](../issues/checkpoint-closure/ast-original-kwargs-v3-merged-validation.md).
Complex formatting remains separate work.

The earlier repair cohort's 12 constructor and float-format probes are
historical, pre-integer-writer evidence; see the [merged-repairs
receipt](../issues/checkpoint-closure/typecall-format-merged-repairs-receipt.md).
