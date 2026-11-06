# Decimal value-core migration

The first clang-21 census used the current preserved `_decimal.c` with
`-DPy_VALUE_CORE=1`, the isolated bulk tree's 48 API marks and numeric marks,
and the configured libmpdec sysroot. It found 47 diagnostics: 45 in the C
source and two in generated Clinic output. The complete log is
`build/checkpoint-bulk-build-20261105/decimal-census.log`. This census checks
marked API calls, not the object-form method and slot boundaries, and therefore
is not a completion estimate. No source was changed for the census.

## Ordered increments

1. **Signals and context fields.** Port `flags_as_list`, `signals_as_list`,
   `list_as_flags`, `flags_as_dict`, `dict_as_flags`, `signaldict_setitem` and
   the five context numeric setters to existing value container/read APIs.
   Keep `Decimal`, context, signal classes and containers as held object
   references. A temporary returned value remains owned until used and is
   closed on every exit. Numeric values are never projected to objects merely
   for the extractor. Compile and test Context flags/traps and setter errors.
2. **Conversion core.** Make `PyDecType_FromFloatExact`, `dectuple_as_str`,
   `PyDecType_FromLongExact`, `PyDecType_FromObjectExact`, `PyDec_FromObject`,
   `convert_op` and comparison conversion accept `PyValueBorrow` at the
   polymorphic input boundary. A Decimal instance is a known heap object;
   inline int/float is read as a scalar and converted with libmpdec primitives,
   with big integer/NaN heap inputs following existing ownership. Tuple element
   values are owners until conversion completes. Preserve subclass, bool,
   unsupported operand, `NotImplemented`, context traps and float-operation
   signaling. The exact libmpdec int51 route must be checked against the
   current decimal implementation before coding; no int box is an allowed
   substitute.
3. **Numeric slots.** Convert the binary/ternary arithmetic families generated
   by `CONVERT_*`, `nm_mpd_*`, `dec_richcompare`, and int/float result paths to
   borrowed value inputs and owned value outputs. Heap `PyType_Spec` entries use
   paired object thunks and `_Py_value_nb_*`/`_Py_value_richcompare` spec IDs,
   as `Modules/_testinternalcapi/value_slots.c` demonstrates. Retain object
   slot thunks only for actual object callers. Type-state discovery uses
   `PyValue_TYPE` and guards absent ternary operands; it must not project an
   inline operand. Unary Decimal-only leaves may remain object methods where
   they never receive or return inline values.
4. **Clinic and methods.** The file has many concrete-object leaves. Do not
   blanket-enable `value_emission`: the previous trial created hundreds of
   failures and object-only Decimal leaves need not change. The new
   `value_core` directive can preserve object signatures while emitting marked
   scalar converters; it does not solve polymorphic input boxing at the call
   adapter. Add a separately reviewed per-function Clinic value opt-in, or
   convert the small set of polymorphic methods to explicit `METH_VALUE`
   wrappers, covering Decimal constructors, `Context.create_decimal`,
   `Dec_BinaryFuncVA`/ternary and context arithmetic families. Choose after
   inventorying the actual Clinic declarations and generated signatures;
   do not hand-edit generated output. Owned context and argument windows must
   outlive calls that can re-enter Python.
5. **Module setup and residual calls.** Port module init, tuple/structseq
   creation and singleton lookups (the remaining census sites), then mark
   the file `Py_VALUE_CORE` and rerun the same compiler census with all marks.
   A clean compiler result is necessary, not proof of a value-native Decimal:
   run box census over constructors, mixed int/float arithmetic, comparisons,
   context methods and subtype/error cases, then classify remaining adapters.

## Boundaries and validation

No mutable staging representation or duplicate object API implementation is
introduced. Actual external object callers use existing thunk/adapter paths;
Decimal heap allocation and non-inline big-int/NaN objects are allocations,
not inline boxes. Enforce `NotImplemented`, exception/trap status, nested
context, signal, callback and type lifetime rules in each increment. Use small
focused Decimal/Context constructor, arithmetic, comparison, context setter
and exception tests plus a real release compile per increment; run complete
`test_decimal` and the R27 suite only at integration. Independently review the
actual value-slot and selective Clinic interfaces before broad implementation.

Census follow-up: `dec_from_long` already calls `mpd_qset_i64` for a compact
export and `mpd_qimport_u16/u32` for digit arrays. An inline int51 can take the
existing `mpd_qset_i64` branch directly, using the same context and status
handling; heap integers continue through `PyLong_Export`. Heap-spec paired slot
IDs and thunks are demonstrated by `_testinternalcapi/value_slots.c`.
