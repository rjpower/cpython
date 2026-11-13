# Executable polymorphic managed-attribute feedback

Status: **source draft in G; not natively qualified**. This is the
specialization increment requested after [Astra6036](../../design/proposals/interpreter-performance56.md).
Source reviewed: I `ccb96ccff6c` and B `0e918ddc25b`. Golden54 remains selected.
Root owns generation, native diagnosis, counters and measurements. The pending
bitmap monitoring-role diagnosis is separate and is not repaired by this design.

## Existing mechanism and intended removal

`Python/specialize.c:fb_begin_ex`, `fb_save_mono` and `fb_record` already retain
up to four historical observations in the code-owned `_PyFbPoly`. A row records
type version, auxiliary version/shape, offset/index, handler and an optional
guarded payload. Execution still uses the newest monomorphic entry. The only
reader of historical rows outside their writer/diagnostic path is
`fb_saw_other_shape`, which changes specialization policy. These rows do not
currently implement a polymorphic interpreter hit path.

Consequently, a stable mixed-type attribute site can miss its newest case and
execute generic lookup/backoff or specialization again. The proposed increment
uses an already learned, complete indexed-read case without generic descriptor
lookup, opcode repatching, cache-body rewriting or re-specialization on that hit.
It is distinct from scalar canonicality propagation: it changes which guarded
operation executes, rather than only proving properties of its result.

Keep monomorphic execution unchanged. Publish one bounded polymorphic member
only for a learned set whose cases have the same checked read operation. Select
one row by current receiver type version, execute its remaining guards, then use
the existing indexed-load/publication/receiver-retirement action body. A miss
before any effect takes the existing generic path. This is neither a universal
cache interpreter nor an arbitrary dispatch through historical handler numbers.

## Selected first family and observed coverage

The simplest first case is `LOAD_ATTR_SLOT`: rows need only a type version and
byte offset, with a NULL check before reading an absent attribute. However, a
source-only census of the unchanged six kernel files under
`pyjit/bench/internal/kernels/` found **no `__slots__` declarations**. DeltaBlue,
Richards, Tree and Widget primarily use ordinary instances. Imported runtime
types can still produce slot reads, so this does not establish zero dynamic
hits; it does rule out assuming that user slot objects dominate these kernels.

Root's native feedback census at I `6c192eb9fde` confirms that SLOT is the wrong
initial target. The existing APIs report these pre-warmed row mixtures:

| Kernel | POLY ATTR sites | Homogeneous SHAPE-load sites | Other histories |
| --- | ---: | ---: | --- |
| DeltaBlue | 68 | 30, currently LOCAL_TEMP | method loads and stores |
| Richards | 32 | 14, including 12 LOCAL_TEMP | method loads and stores |
| Generators, coroutines, comprehensions, silent logging | 0 | 0 | no POLY ATTR sites in this capture |

The SHAPE histories contain two distinct receiver type versions in DeltaBlue
and three/four in Richards. `BinaryConstraint.add_to_graph` word1 has two SHAPE
rows; `Task.addPacket` word1 has three. The other four kernels have no pre-warmed
POLY ATTR sites in this capture. This is learned history, not dynamic matches,
misses, generic lookup counts or throughput savings.

Evidence: `E56/feedback-census-6c192/{deltablue,richards,...}.json`, where
`E56=BASE/pyjit-link-wave1/interpreter-performance56`. This is diagnostic native
evidence supplied by root, not qualification of this unimplemented candidate.
The first `E56/site-counts-6c192` probe incorrectly divided word offsets by8.
Its visit totals are invalid and excluded; root retained it as failed diagnostic
evidence. Monitoring/dis offsets are word indexes. The corrected separate run
at I `196284`, `E56/site-counts-corrected-196284`, uses them directly and reports
1,709,700 visits at pre-warm POLY SHAPE-load sites in DeltaBlue and 1,253,922 in
Richards (14,679,757 and 11,231,069 total monitored instructions respectively).
The other four kernels have zero POLY ATTR sites. These are site-frequency
counts under instrumentation, still not actual eligible row-hit/miss ratios,
native instruction counts or timing. Root reports a fixture-regex-only source
change for that diagnostic identity; its manifest owns exact source/native state.

Select **homogeneous LOAD_ATTR_SHAPE rows**, with the existing LOCAL_TEMP
ownership form preserved. Require current receiver type-version match and
current shape equality before using the offset. The type match proves the
managed layout; the exact current shape proves the slot filled, as in today's
SHAPE handler. Retain its NULL debug assertion and result publication contract.
A remembered shape number without the current checks grants no read permission.

Do not weaken the shape predicate into an unchecked offset read. A later
experiment could normalize eligible rows to the existing INSTANCE_VALUE
contract, but that requires current inline-values validity and actual non-NULL
field checks. It is outside this first implementation. Method, property, class,
module, SLOT and store rows remain excluded even when they are more frequent.

## Shared producer and execution contract

Use the existing DSL/action graph and feedback layouts. One checked guard/read
description must govern both row nomination and generated execution. Do not
add another whole-body recognizer or infer a capability from an opcode name.

| Part | Required contract |
| --- | --- |
| Receiver | Current initialized required value; object representation established before pointer projection. A borrowed local remains anchored until taken, exactly as in the existing family. |
| Type version | Nonzero current receiver type version matches the row before using its offset. Follow existing version exhaustion/invalidation rules; a stale or zero version misses. No cached type pointer is needed. |
| Offset | Produced by the existing descriptor or managed-values layout lookup for this exact guarded type/name. The checked read kind, type guard and layout proof authorize address formation. A serialized public image cannot supply an unchecked cache offset. |
| Extra read guard | Current shape equals the selected row's shape after the matching type-version guard establishes managed layout. Current shape must still certify the slot filled. Missing/detached/different storage goes generic before a value or owner moves. |
| Result | Borrow current canonical field only while receiver anchors it, duplicate the result owner, then run existing canonical destination publication and receiver retirement. No borrowed bits become a persistent home. |
| Miss | No close, owner move, callback, source-edge delivery or destination publication has occurred. Generic fallback executes once with original inputs and semantic site. |
| Commit | After result acquisition or input detachment, use existing reached completion/error rules. A retirement callback cannot restart lookup. |
| Row publication | Build the complete scalar row before publishing the polymorphic member under the GIL. Allocation failure retains correct generic/monomorphic execution. Record a semantic read-kind tag, not the new polymorphic opcode recursively as another historical case. |
| Lifetime | Rows contain scalar versions/offsets only. The current receiver pins its type and field through the read; code owns the feedback bank. No new owning GC edge, frame field, root record or image refcount. Never retain a row pointer across a callback or feedback reallocation. |

The active entry's scalar payload can identify the existing poly table. Generated
code selects the current offset into a C metadata local and executes the shared action
body. Do not copy a historical row into the monomorphic entry on every hit or
patch the instruction for alternating receivers. Do not add a row-selection
branch to unchanged monomorphic handlers.

Use the existing folded-local versus owning-input lowering for LOCAL_TEMP.
The guarded row selector supplies metadata to the same load action; it does
not consume a PyValue home or become a scalar-result owner. Preserve the
current local/scratch/discard behavior rather than routing these frequent
LOCAL_TEMP sites through an extra receiver snapshot. At most the already
selected ordinary and LOCAL_TEMP forms need new handler entries; price their
native code size. A typed row nomination and emitted selector receipt must
agree on type version, shape, offset, input role and read action. Historical
handler IDs are an inspection key, not the admission certificate.

The bounded source interface is a `SHAPE_READ` guard and a compiler-only cache
binding. The ordinary guard emits the existing type-version and shape tests;
the polymorphic guard selects a scalar offset after those same tests. Both
feed the same `_LOAD_ATTR_SHAPE` publication and LOCAL_TEMP path.
The binding identifies the current receiver input, read kind and offset source;
callbacks or mutations invalidate it before a later cache consumer. It is not
a PyValue, frame field or owner. Actual shape nomination records the read kind
only after `_PyShape_HasSlot` succeeds. The active entry can reuse its existing
padding, and the historical SHAPE row can reuse its otherwise unused scalar
payload, preserving both feedback structure sizes.

The read primitive duplicates the guarded field atomically into an owned
canonical result. It does not expose a native borrowed temporary that a later
callback could invalidate before duplication. The existing receiver retirement
and destination publication follow this primitive. Four narrow source methods
cover both record modes, missing/changed guards, intervening effects and a
missing actual read. Runtime fixtures are drafted; root owns their execution.
The two new execution forms require learned feedback and are rejected by common
public constructor/replace/marshal admission. Existing LOCAL_TEMP admission is
unchanged. This adds one effect bit, with no feedback size or hot frame growth.

Cached descriptor payloads require separate owner/version contracts and are
excluded here. In particular, a live receiver type version does not by itself
make an unrelated borrowed function pointer or mutable classmethod payload safe.

## Invalidation, observation and saturation

Type/MRO/descriptor mutation must invalidate the row through the same version
guard as today's specialization. A data descriptor added later wins; changing
`__getattribute__` or reassigning `__class__` must miss. Managed dictionary
materialization and attribute deletion must take the existing invalid/NULL
paths. A receiver type that dies cannot leave a usable pointer in this scalar
table; a new type cannot match an expired version by an unreviewed ABA rule.

Keep immutable A/B/C/D geometry, semantic site, borrow/read roles and recovery
identity unchanged. Transparent LINE/INSTRUCTION wrappers must recover and
restore the selected family member correctly. Monitoring enable/disable,
legacy tracing and callbacks that mutate the receiver/type cannot leave an old
row or object view in use. PRE callback changes must be followed by current
guard evaluation; post-publication mutation follows existing completion.

Preserve `FB_POLY_SIZE == 4` and the existing `fb_record` saturation/backoff
policy. Promote only two to four complete SHAPE cases with distinct nonzero
type versions. Do not replace stale rows eagerly on a hit: they fail their
guard, and existing miss/re-specialization policy decides the next observation.
A fifth distinct observation follows existing FB_MEGA behavior; it must not
leave an executable polymorphic member attached to an incomplete bank. Keep
mixed-kind and repeated-type/different-shape histories on existing execution.
No unbounded growth, new Python references, or changed version-exhaustion policy.

Initial scope is the configured GIL interpreter. FT must retain its existing
path until a separate publication/read/reallocation protocol is reviewed. GIL
publication is not an FT synchronization proof.

## Minimal root diagnostic procedure

Use the actual native identity and unchanged kernel path/loops/variant from the
existing hot6 manifest. Run a separate diagnostic process, not a timed process:

1. Load the kernel with the existing driver loader and call its setup normally.
   Keep its module functions/classes and nested code objects for inspection;
   include the imported logging code for the silent-logging kernel.
2. Run the unchanged workload once with its existing warmup count. For each
   code object, enumerate `_testinternalcapi.get_ir(code, True)` and inspect
   LOAD_ATTR-family word indices with
   `_testinternalcapi.get_feedback(code, word_index)`.
3. Retain `(filename, qualname, word, current_handler, state, rows)` for ATTR
   sites whose rows contain at least two distinct nonzero type versions and
   whose row handlers are all SHAPE with distinct receiver type versions.
   Retain other kinds separately; do not pool their counts or call
   method/property/SLOT/INSTANCE_VALUE rows eligible.
4. Run the same workload again, snapshot the same sites, and compare membership
   and state. `get_feedback` returns `(kind, state, field0, field1, rows)`;
   rows are `(type_version, auxiliary, handler_id)`. Resolve IDs through the
   current contract table. `get_ir` and these rows show learned cases, not
   per-case execution counts or saved cycles.
5. If candidates exist, root uses a separately retained diagnostic counter
   build or an existing matched instruction probe to count candidate-site
   executions, pre-effect guard misses, generic lookup/specializer entries and
   hypothetical cached-row matches. No production enable flag or new hot
   ownership bookkeeping is part of that diagnostic. Remove diagnostic work
   before timing.

The captured census rejects SLOT as the first hot6 target. Historical rows
alone do not establish current repeated hits: MEGA/stale observations and rare
initialization sites must not inflate expected savings. Record full source/ELF
identity and counter method beside the results.

## Tests and admission

Use unchanged workloads plus focused same-site alternating receiver tests:

- Two/four eligible classes with different field offsets; both input binding
  and scratch forms, local/scratch/discard destinations, and all public code
  construction routes. One/four cases stay bounded; the fifth follows the
  declared generic policy. Monomorphic native output must remain unchanged.
- Replace a slot by a property/custom descriptor, delete the field, change
  `__getattribute__`/`__class__`, materialize an instance dict and vary managed
  shapes. Check descriptor precedence, exact exceptions and fallback recovery.
- Generic fallback descriptor/callable counters prove that a miss invokes user
  code once. Result and receiver finalizers mutate locals/types, run GC and
  capture the frame; verify result rooting and no post-effect lookup replay.
- Enable/disable LINE/INSTRUCTION and legacy tracing before and during repeated
  reads. Exercise accepted redirects and callback errors without duplicating
  attribute access or losing semantic recovery.
- Free receiver types and code, exhaust/disable specialization, inject row-bank
  allocation failure and inspect bounded retained objects. No stale cache
  payload dereference, extra GC edge or use-after-free is allowed.
- Generator source mutations that change guard order, offset provenance,
  required/nullable roles or publication must revoke the new row capability.

Before implementation, record the expected dynamic delta: cached polymorphic
matches replace generic lookup/specializer work with a bounded version selection
and the same guarded read/retirement body. After implementation, require actual
generated and optimized native removal; price row scanning, branches, code size,
register pressure and misses. Use the combined plan's parent/hot6 confirmation
and all29 gates. Warmed histories and corrected monitored site visits establish
a coverage lead only; no eligible row-hit/miss ratio, PMU or performance result
exists for this proposal.
