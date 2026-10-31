# Interpreter source inventory

The register compiler, value representation, call paths and generated families
provide the following shared implementation facts. This inventory is not a task
queue or a performance attribution. The [active attribution issue](../interpreter-residual-costs/status.md)
selects work; [measurements](../gc-cleanup-scanning/performance.md) own numeric results.

## Implemented paths


| Layer | Actual implementation | Important remaining boundary |
| --- | --- | --- |
| Compiler | `compile.c` obtains the optimized stack CFG; `regcfg.c:1404` runs swap renaming, copy propagation, store elimination, pop-top and truth folding, then physical slot compaction. `assemble.c` emits words, feedback initializers, source locations and exception tables. | No runtime lowering from ordinary bytecode. The compiler still starts with stack-shaped semantics; the final physical IR is not yet the authority for complete H2 liveness. |
| IR | `pycore_ir.h:12`: one64-bit word,16-bit handler and four12-bit fields with generated slot/immediate/feedback roles; CD can hold wider data. Code words are immutable; `ir.c:245` lazily creates mutable execution words and feedback. | Four fields are not four interchangeable operands. Folded inputs, call windows, relaxed jumps and exception successors have distinct real semantics. |
| Handlers | `bytecodes.c` declares operations, guards, macro compositions and specialization families. `ir_generator.py` generates executable cases; IR metadata describes formats, properties and instrumentation relationships. | There is one source of generated truth, but some complex effects still need explicit semantics before maps or ownership proofs can consume it. |
| Values and objects | Canonical64-bit `PyValue`: inline exact int32/non-NaN doubles, otherwise object pointers; bool, large integers, subclasses and NaN stay objects. Core frames/containers/cells/instance slots use values. Shapes describe positional attributes; materialized dictionaries enter terminal dictionary mode. | Object-pointer APIs still box; raw native fields are not automatically value storage. This is an intentional object/ABI migration, requiring rebuilt ports. |
| Calls and slots | Value vectorcall, value arguments/results for METH_O/FASTCALL/METHOD, value `tp_call` and attribute/getter results. Generic and specialized call routes coexist. | Native self remains an object; NOARGS retains object result; many numeric/sequence/mapping/wrapper slots still take objects. Bound-method object storage and VARARGS tuples are separate allocation sources. |
| Dispatch | Generated computed-goto/switch and preserve_none/musttail backends exist. Current tail handlers pass slots/thread/IP/word, plus stats sequence state, and reload frame from the thread state. | A backend's existence is not a measured win. H1's passed-frame convention remains isolated; changing diagnostic IP publication is outside that experiment. |
| Roots and memory | Full-heap nonmoving tracing, no reference counts, explicit precise value/raw spans and checked precise C frames, conservative opaque stack/PyMem-carrier roots. Frame clearing remains active. | No active H2 maps, generational barriers/minors or moving collector. Current frame pointers, initialized slots and hybrid coverage cannot be removed by implication. |

Source anchors: [register pass driver](../../../Python/regcfg.c),
[IR/feedback interface](../../../Include/internal/pycore_ir.h),
[operation DSL](../../../Python/bytecodes.c),
[value representation](../../../Include/pyvalue.h),
[native method conventions](../../../Include/methodobject.h),
[dispatch macros](../../../Python/ceval_macros.h), and
[root contract](../../design/gc-roots-and-lifetimes.md). These common interpreter sources
are shared with the integrated candidate; the GC composition is
separately reviewed in the union source review.

## Existing common and typed families

The source already shares meaningful machinery. BINARY_OP groups int arithmetic,
division/remainder, bitwise/shift operations, float arithmetic, Unicode addition,
subscription and generic extension. Shared guards admit inline or compact exact
integers. Int/float/string comparisons, list/tuple/dict/string subscription,
sequence unpacking and list/tuple/range/generator iteration have specializations.
Generic value APIs retain overflow, subclass and user-code fallback semantics.

LOAD/STORE_ATTR shares shape/version/transition and polymorphic plans, alongside
module, slot, property and dictionary-mode routes. Method loading can forward a
descriptor and value receiver without constructing a bound method first. CALL
has Python exact/general, bound-method, builtin and descriptor conventions.
SEND/YIELD inline generator/coroutine transitions while retaining exception,
return-slot, frame and IP state. These are common families, not an absence of
specialization or a need to add a separate arithmetic interpreter.

Several dimensions remain entangled in target names and adapters: semantic
operation, receiver admission, stored representation, result representation,
call convention, monitoring and available feedback. A shape proof that a slot
is filled does not automatically prove its value is int32. A non-NaN float
representation does not admit numeric subclasses. A predicted entry is not an
ordinary counted dispatch entry. A clean parameterized design must preserve
those distinctions rather than assign one ambiguous "type" parameter.

## Examples and proof boundaries


Three source examples make the distinction between cleaner operation families
and faster execution concrete:

- **`i += 1`.** Existing register passes fold eligible local loads and the
  final store. Warm `BINARY_OP_ADD_INT` checks both compact integer operands,
  decodes them and checks overflow. An ordinary int32 result allocates nothing;
  overflow takes the boxing path. Constant `1` still uses the general operand
  guard because its kind is not propagated into this consumer. Generated C
  temporaries alone do not establish machine-level moves or their cost.
- **`o.x + o.y`.** The existing attribute-sum test specifies four words: two
  loads, add and return. A shape load guards owner/version and copies one
  `PyValue`; it does not box the result. Shape slot kinds already promise I32,
  F64 or any value, and generalization invalidates the version; these are
  value promises rather than different layouts. There is no string slot kind.
  A typed LOAD_ATTR name alone saves no payload work. A consumer would need
  to use the promised result kind, with a valid path when the producer falls
  back to generic behavior.
- **`if i < n`.** The compiler already folds away the separate TO_BOOL for
  comparisons used as conditions. An integer comparison still writes a
  True/False singleton value and dispatches to a BOOL branch that checks and
  consumes it. This creates no boolean allocation. Shared result contracts
  may expose redundant work, but removing a guard requires proof for every
  entry, including instrumentation and debugger control transfers.

See [existing IR expectations](../../../Lib/test/test_ir.py),
[shape kind promises](../../../Include/internal/pycore_shape.h),
[specialization feedback](../../../Python/specialize.c),
[truth folding](../../../Python/flowgraph.c) and the operation DSL above.
An initial design experiment can ask whether existing producer facts can be
reused by consumers without adding another type system or changing instruction
boundaries. This is a feasibility question; matched attribution must establish
whether the saved work is worth an implementation.
