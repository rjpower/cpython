# H3-1 stage 2b: remaining fixed-ID consumers

Status: design/inventory only, 2026-10-29, against frozen `624c75f6455`.
Stage 2a and all its source/evidence remain unchanged during independent
review. The coordinator reports LAND_STAGE2A624; native integration remains
behind ROOTS 2b. This document proposes the next checkpoint and the order of
remaining stage 2 work. It claims no new build, test or runtime evidence.
References below are snapshot paths and approximate source lines.

## 1. Scope and recommended order

Keep every opcode, pseudo and dispatch-handler ID unchanged through stage 2.
The count remains the actual generated span, currently 302, rather than the
65536 representational capacity. No new operation or runtime option is
needed. Migrate semantic decisions to explicit domain properties; retain
existing threshold exports until their coordinated stage 4 removal.

Use four bounded checkpoints:

1. **2b.1: names and inspection.** One full-domain C name table, generated
   Python base/kind/operation metadata, explicit adaptive display and NOFB
   restoration. This is the first implementation scope proposed here.
2. **2b.2: monitoring storage and publication.** Separately aligned original
   LINE IDs, uint16 INSTRUCTION IDs, canonical-ID relations and bounded
   allocation/publication/ownership. Preserve event masks and GIL contracts.
3. **2b.3: statistics.** Count actual dispatch targets and pairs, including
   per-NB, NOFB and IR_JUMP. Extend printing/aggregation against the domain.
4. **2b.4: remaining producer/table/compiler widths.** Cache/feedback and
   expansion tables, specialization locals/truth rows, compiler admission,
   pseudo indexing and wide uop references. Close stage 2's full inventory
   before sampler work or final renumbering.

Each checkpoint must compile and run coherently with current IDs. The
native landing order remains Q -> ROOTS 2b -> H3. Stage 3 target-schema/codec
work and stage 4 renumbering, uop namespace reassignment, threshold removal
and pyc magic stay separate. No raw handler is translated through a second
byte representation.

## 2. Exact first checkpoint: 2b.1

### Changes

`opcode_metadata_generator.generate_name_table()` currently emits only the
ordinary/pseudo span (`256 + len(pseudos)`). Emit names for the entire domain,
using numeric indices so an IR-only name does not require its macro to be
defined before opcode metadata. Metadata holes remain NULL. Remove
`_PyIR_OnlyNames` and the name split from ir_metadata_generator and its two
C consumers. The names are generated from the same domain as IDs/base/kind
metadata; there is no second name allocator.

Add immutable `HandlerDomain.operations: Mapping[str, int]`, keyed exactly
by per-NB handler names, with values from the existing `binary_ops()` order.
Validate key/kind agreement, bounds and uniqueness during domain construction.
Do not reparse operation numbers in an output generator or consumer.

Generate private `_handler_flags`, `_handler_bases` and `_handler_operations`
tuples indexed by ID, each of length domain.count, plus named kind-bit
constants from HandlerKind. Flags for metadata holes are 0, base entries
match the C base map, and operations are None except for per-NB targets
(NB_ADD = 0 remains an operation). Disassembly consumes these directly.
Its adaptive display rule is: preserve an actual specialization's identity;
otherwise show its semantic base while retaining the raw executed handler
in the existing instruction information. A family-only BINARY_OP token is
metadata, not a dispatch body. The per-NB operation value is explicit data,
not inferred from an arbitrary category range. Preserve opcode.opmap,
opcode.opname, EXTENDED_ARG and the existing name APIs.

`_PyCode_HandlerFor()` must select a NOFB twin by the generated NOFB/layout
properties, preserving its pristine handler and all field bits. Replace
its current `handler >= IR_FIRST_ONLY_HANDLER` semantic condition. This
reuses the stage 2a base map; it does not introduce another normalization.
Existing numeric exports may remain defined until stage 4, but these moved
consumers no longer use them as semantic predicates.

### Owned files/functions proposed for implementation

| File | Bounded change |
|---|---|
| `Tools/cases_generator/opcode_metadata_generator.py` | `generate_name_table`, its explicit domain argument/call |
| `Tools/cases_generator/ir_metadata_generator.py` | Remove only `_PyIR_OnlyNames` generation/declaration |
| `Tools/cases_generator/py_metadata_generator.py` | Emit private base/kind/per-NB operation data from the domain |
| `Tools/cases_generator/ir_generator.py` | Immutable operations mapping and its invariants on HandlerDomain; no allocator change |
| `Include/internal/pycore_opcode_metadata.h`, `pycore_ir_metadata.h`; `Lib/_opcode_metadata.py` | Regenerated products only |
| `Python/ceval.h` | `lltrace_instruction` uses the unified names |
| `Python/ir.c` | `_PyCode_HandlerFor` uses NOFB/layout properties; update metadata comment |
| `Lib/dis.py`, `Lib/opcode.py` | Consume generated private metadata; preserve existing public name/argument APIs |
| `Lib/test/test_generated_cases.py` | HandlerDomain/generated-metadata/IR-case classes only |
| `Lib/test/test_dis.py`, `test_ir.py`, `test__opcode.py` | Names/base/kind/operation consistency, adaptive and NOFB behavior |

**Shared-file exception required before implementation:** change only the
name selector in `Modules/_testinternalcapi.c:get_ir()` (currently 1611-1613)
to the full-domain table. No new method entry, GC hook or compiler phase
fixture change is needed for this checkpoint. Send the exact hunk to ROOTS
and the coordinator before editing. The stage 2a scalar-hook ownership does
not grant this separate hunk automatically. The exact proposed diff is:

```diff
-        const char *name = handler < IR_FIRST_ONLY_HANDLER
-            ? _PyOpcode_OpName[handler]
-            : _PyIR_OnlyNames[handler - IR_FIRST_ONLY_HANDLER];
+        const char *name = _PyOpcode_OpName[handler];
```

The three numeric CLIs already import ir_generator in this direction; the
new data adds no reverse import into analyzer or symbolic root checking.
Python/codegen.c remains the NEED_OPCODE_METADATA definition site; Python/ir.c
remains the NEED_IR_METADATA definition site. No new output file or build
rule is needed. The current span includes legacy compiler-pseudo positions;
stage 4 must retain names for its generated compiler suffix when those IDs
move beyond runtime count. This checkpoint does not store pseudos in words.

No changes to gc_trace, gc_live, test_gc, C checker, call_effects, PROCESS,
README, WORKLOG or shared gate tools. ROOTS checker classes/import contracts
in test_generated_cases remain untouched. No new CAPI fixture is needed
for 2b.1. `_opcode.get_ir_formats()` already covers IR_NUM_HANDLERS; change
Modules/_opcode.c only if consistency checking identifies an actual gap,
and report that expansion before editing it.

### First controls and gates

These are planned controls, not new run results:

- First pre-fix oracle: analyze a minimal NOP + IR_BACON DSL whose IR-only
  ID is 256, then require its generated C name table to contain that exact
  IR_BACON entry. Today's ordinary/pseudo-only emitter omits it. This is a
  generator-output contract failure, not a claim that get_ir currently loses
  names (its second table currently repairs that lookup). Also check every
  production metadata ID, including IR_JUMP 267, BINARY_OP_ADD 268 and NOFB,
  for exact C get_ir/Python/dis name/base/kind agreement. A source closure
  scan rejects either surviving split lookup as supplementary evidence.
- An isolated copied-generator negative control omits an IR-derived name
  from the unified table; the affected actual high-ID name must fail, not
  merely an import. Another changes a per-NB operation value and must fail
  operand/disassembly agreement.
- Keep the existing specialization-kind/base-distinction tests. A copied
  Python-metadata variant marks a generic per-NB handler specialized and
  must fail adaptive display expectations. A NOFB-property removal must
  fail exact restoration rather than only a source-text assertion.
- All 264 opcode/pseudo IDs, 287 dispatch IDs, count 302 and first 267 must
  match the frozen baseline. Unchanged products are compared bytewise;
  intentionally changed name/metadata products are checked structurally.
- Rebuild/smoke release and qualified Og debug; affected generated-cases,
  dis, IR, opcode, compiler and monitoring modules on both; actual-mode
  asserted strict debug-kill via corrected worker.main launcher. Preserve
  the compiler's direct C phase oracle. Regen+clinic and ownership checks.

Expected implementation boundary: about 60-90 minutes including targeted
gates and copied controls, then freeze for review. Full stage 2 end gates
and merged integration gates remain separate.

## 3. Monitoring inventory and state contract for 2b.2

The concrete width sinks are:

- `pycore_instruments.h:89-96,116,125-131`: packed LINE original byte,
  uint8 INSTRUCTION array, original-opcode getter and byte de-instrument API.
- `instrumentation.c:124,167,191,249,255`: opcode-indexed fixed 256 event and
  twin tables, byte-returning de-instrument API, numeric instrumented test.
- `instrumentation.c:391-422`: original opcode occupies byte 0 of a variable
  width LINE record; delta bytes start at byte 1. Casting this field's address
  to uint16 is unaligned and can overwrite adjacent delta bytes.
- `instrumentation.c:707-850,1448,1469`: displaced pointers, stores and
  callback local truncate IDs; event/tool variables elsewhere remain masks.
- `instrumentation.c:1703-1823`: `_co_monitoring` and LINE pointers have
  initialize-before-release-publication paths; INSTRUCTION currently assigns
  its shared pointer before initializing elements and allocates with
  `sizeof(_PyCoLineInstrumentationData)` rather than the element type.
- `bytecodes.c:5505-5548`: tracing fast path directly reads the packed byte;
  INSTRUCTION redispatch asserts next_opcode<256. Generated cases must follow
  the same supported accessor/state contract.
- `Objects/codeobject.c:1033-1054,2310-2338`: public and safe line readers,
  nested allocation cleanup. Count all nested bytes in any existing sizing
  path; do not count a pointer as owned storage.

Use a LINE header with an independently allocated, naturally aligned
uint16 original-ID array and compressed **delta-only** bytes. Keep the
line width descriptor and tool/event masks byte-sized. INSTRUCTION storage
becomes an aligned uint16 array. Check element-count multiplications and
header addition against SIZE_MAX before allocation. A scalar entering a
metadata-indexing or ID-store boundary stays int until validated; never
validate an already truncated uint16 value. Store 0 only as the documented
ineligible-LINE sentinel; CACHE execution semantics do not change.

Displaced IDs retain their canonical/event-token meaning. word_opcode()
normalizes the raw target through the base map; restoration uses the
pristine word through HandlerFor to recover exact per-NB/NOFB forms.
A canonical family token can lack a dispatch body (BINARY_OP); requiring
DISPATCH on every stored displaced token would be wrong. Validate metadata
and canonical/twin meaning before indexing, then require DISPATCH only for
the final installed word. LINE/INSTRUCTION nesting and DISABLE callbacks
must preserve this distinction in both removal orders.

Generate instrumented/twin relations from explicit domain properties and
DSL associations. Existing exceptional twins and transparent wrappers have
explicit entries; never derive the whole relation just from a base-map
inequality or a numerical cutoff. Event association can remain an explicit
semantic table, sized by the domain; its **values** stay int8 event IDs.
Use uint16 twin values with zero meaning no twin, preserving the existing
CACHE 0 contract. Keep all eight-bit tool membership operations unchanged.

### Writers, readers, publication and retirement

The supported release/Og builds have a GIL. Monitoring mutation, Python
callbacks, legacy tracing and executing-word rewrites run attached under
that owner domain. LOCK_CODE, ASSERT_WORLD_STOPPED* and all FT wrappers
in this fork are empty/plain assignments; their names establish no lock or
atomic proof. Do not add atomics solely to change those spellings, or claim
free-threaded ownership from them. Do not invent a lock-free multiwriter
protocol for H3.

There is a narrower real concurrent-reader contract to preserve:
PyCode_Addr2Line acquire-loads `_co_monitoring` and `lines`; the signal-safe
traceback dump_frame (`traceback.c:1026-1061`) and non-locking tracemalloc
path (`tracemalloc.c:317-327`) use SafeAddr2Line and read line deltas.
`_Py_Instrumentation_GetLine()` reads immutable line-position data, not the
mutable displaced-opcode array. Build the complete LINE header, immutable
deltas and original-ID array before release-publishing lines. They are
installed once, never replaced during enable/disable, and remain owned by
the code object until free_monitoring_data at code destruction. A valid
reader must keep the code/frame lifetime protected by its existing caller
protocol; release/acquire alone does not make a borrowed dead code object
safe. The proposed pointer access for SafeAddr2Line matches the existing public
Addr2Line acquire loads, because it is an actual asynchronous reader; it
adds neither allocation nor a mutex in that signal-safe path. Before its
later implementation, ROOTS/coordinator must confirm the existing frame/code
lifetime proof (`_PyFrame_SafeGetCode` in dump_frame, current frame in
tracemalloc). Do not replace that proof with atomics, add a retirement
scheme or widen ROOTS's reader-lifetime claim here. This is a prerequisite
for 2b.2, outside the bounded 2b.1 decision.

Displaced original IDs are mutable only in GIL-held monitoring/dispatch
paths; current asynchronous line readers do not consume them. A plain,
aligned uint16 accessor is sufficient under that concrete owner contract.
If inventory finds an actual supported GIL-free ID reader, stop that scope
and specify its lifetime plus matching real atomic primitive before
implementation. The existing API has seq_cst uint16 load/store and relaxed
variants; it does not expose a uint16 release/acquire pair. No atomic backend
extension or global FT-wrapper rewrite is justified by this inventory.

Likewise, handler-word writes preserve the upper 48 operand bits. Their
writers are serialized by the supported GIL owner. Full-width atomic
publication is required only if a concrete supported simultaneous reader
needs it; first enumerate executing-word, remote-memory and signal readers,
including generated dispatch loads, rather than mixing a new atomic store
with unaudited plain accesses. The remote sampler does not read displaced
LINE/INSTRUCTION arrays. Its target-schema/codec work remains stage 3.
Atomicity would not solve monitor-state lifetime, publication ordering or
multiwriter lost updates by itself.

Allocate pending buffers locally and initialize all required new arrays
before assigning their shared pointers or publishing wrapper handlers.
Compute monitor-state changes locally so allocation failure leaves the
previous valid instrumentation state, not merely a non-NULL pointer.
Commit pending state before executing-word installation. Free only pending
unpublished buffers on failure; published storage survives disable/re-enable
and callbacks until code teardown. Audit allocator call effects and reentry
before asserting a builder cannot run Python. Callback DISABLE may rewrite
handler IDs but must retain backing arrays and restore exact operands.

### Ownership and controls for the later monitoring checkpoint

Proposed files: pycore_instruments.h; instrumentation.c; bytecodes.c and
regenerated IR cases; codeobject.c cleanup/line-reader/sizing functions;
ir.h/ceval_macros/ir_generator dispatch accessors only if the concrete-reader
audit warrants word atomics; IR/runtime monitoring tests. Preserve ROOTS
collector sources and phase fixture. A C representation test hook would need
an additional exact additive CAPI include/function/method exception, proposed
separately after this design review; 2b.1 does not need it.

The first primitive width witness is 256 read back as 0 through the old byte
LINE getter; also cover 255, 511, 65535 at odd and even indices and every delta
width. This is a representation-only fixture, never dispatch of an arbitrary
ID. Separately test actual existing high raw per-NB/NOFB targets through
nested LINE+INSTRUCTION, DISABLE, restart and both removal orders. Their
canonical stored token may currently be below 256: those tests must not be
misreported as executing a high canonical token. Stage 4 must supply actual
renumbered high canonical/twin execution witnesses.

Controls: narrow one production getter/store in a copied header and demand
a semantic round-trip failure; retain exact patch and matching config.
Use failmalloc at each new allocation to prove old valid state/exception
and cleanup; establish the available allocator fixture before proposing a
new global hook. Retain debug-kill, monitor callback/reentry, code line
lookup, tracemalloc/traceback, and constructor/CACHE regressions. Estimate
2-3 hours implementation/targeted evidence; any actual cross-owner lifetime
gap is a coordinator prerequisite, not a guessed atomic fix.

## 4. Raw dispatch statistics for 2b.3

`ceval_macros.h:70-83` skips stats operands >= 256. In addition,
ir_generator.generate_handler:1763 emits INSTRUCTION_STATS(inst.name),
so per-NB and NOFB targets are currently folded to low family counters;
IR_JUMP is skipped altogether. Widening arrays alone does not fix raw-ID
coverage. Emit the actual target name at target entry and retain semantic
family grouping as an explicit generated-base aggregation in reporting.
Do not change operand boxing accounting or specialization attempts to raw
IDs just because dispatch execution counters change identity.

Use the actual generated count for `OpcodeStats.pair_count` and
`PyStats.opcode_stats` in Include/cpython/pystats.h, with only public
opcode_ids.h as the count dependency. Update pystats print loops, name
lookup, clear/merge/allocation paths, lastopcode bounds and every counter
macro indexing contract. No manual 256 or byte lastopcode. Keep the first
pair's existing sentinel semantics unless a separately reviewed stats
change is necessary. Add an explicit generated adaptive/specializable
property from actual specializing operations; cache-width nonzero and
base!=raw are insufficient (NOFB and operation variants show why).

At count 302, the pair matrix is 302² * 8 = 729,632 bytes, versus 524,288 at 256:
205,344 bytes extra before other struct fields. Measure actual sizeof and
per-interpreter/thread allocation, not just this formula. Allocate for the
actual count, never a dense 65,536² capacity table or huge boundary fixture.

Proposed files: Include/cpython/pystats.h, internal/pycore_stats.h and
pycore_pystats.h as needed; Python/pystats.c, ceval_macros.h; ir_generator
stats emission; domain/Python metadata for the adaptive property;
Lib/test/test_pystats.py and generator tests; Tools/scripts/summarize_stats.py
only affected name/base aggregation. The benchmark reader sums execution
counts at boil/bench/run.py:392; coordinator owns that shared runner, so
supply an exact compatibility/coverage proposal before editing it.

First witnesses: IR_JUMP has no execution row today, per-NB/NOFB dispatches
have family rows instead of their own, and a high->low pair cannot survive
the old high-ID skip. Use a bounded C counter fixture or deterministic
actual workload and stage 2's existing high raw handlers, checking source
and destination axes. Removed raw-target emission and one reintroduced
256 bound must each fail semantic count/pair assertions. Update/reset/merge
checks must run on a matching existing Py_STATS build, not merely skip on
release/debug. Record source/binary/config provenance, corrected root mode,
family/raw totals and counter coverage changes. Historical dispatch totals
are not directly comparable without that coverage reconciliation. Existing
kernel+micro box/alloc benchmarks remain scoped as recorded; memory/pause
attribution still belongs to coordinator checkpoint I. Estimate 1-2 hours plus
the matching stats rebuild and benchmark time.

## 5. Producer/compiler closure for 2b.4 and stage 4 handoff

Inventory to close before final renumbering:

- opcode_metadata_generator: cache[256], feedback-fields[256], ordinary
  metadata/name extents, expansion[256], int16 uop references, uint8 pseudo
  target values and hard-coded pseudo-256 indexing. Widths/field counts stay
  narrow when they are not IDs. Use explicit generated pseudo indexing and
  integer uop references that cannot lose aliases above INT16_MAX.
- specialize.c: ID locals around 1913/2620/2966 and truth rows/signature
  around 2951/3017. Widen IDs; oparg/slot bitsets stay their semantic types.
- assemble.c: fb_kind/has_fb_entry[256] at 587/595. Index domain-sized
  arrays, retain byte feedback-kind values, validate before indexing.
- pycore_opcode_utils.h MAX_REAL_OPCODE 254, instruction_sequence.c
  MAX_OPCODE 511 and flowgraph.c:790 deopt-inequality admission. Compiler
  inputs, pristine assembler forms, metadata and runtime dispatch are
  distinct permissions. Generate compiler-input eligibility from actual
  producers/pseudos; do not admit wrappers or inline assembler forms just
  because they are metadata/pristine. Public/private threshold exports are
  removed together at stage 4, after consumers use these properties.
- uop_id_generator's distinct default namespace begins 300. Widen expansion
  reference consumers in stage 2; move its numeric start after the final
  generated compiler domain in stage 4 together with namespace/ID changes.
  Do not silently change uop numbers inside a checkpoint claiming identical
  numbering. No tier 2 execution feature is implied.

Proposed ownership adds the corresponding generator functions/generated
headers, Python/specialize.c and assemble.c, flowgraph.c,
instruction_sequence.c and pycore_opcode_utils.h, plus their targeted tests.
No root checker/call_effects change. A new compile-input property requires
an explicit domain/import design before implementation; do not scatter
locally reconstructed numeric ranges. Compiler files can include the
existing internal IR property interface directly without making symbolic
analyzer import numeric generators. Existing phase/root obligations remain.

Controls cover invalid int scalars before casts/indexing (65536 and
65536 + NOP at actual boundaries), a metadata-only BINARY_OP token versus a
legal assembler per-NB target, stateful wrappers versus compiler inputs,
high-ID pseudo targets/uop aliases in bounded generated fixtures, and
original numeric IDs/products. No 65,536² stats fixture is needed. A complete
consumer scan and final stage 2 ladder are required before stage 3. The new
sampler identity/codec and exported API removals remain on the accepted
H3-1 design's stage 3/4 compatibility rows, without an early fallback path.
