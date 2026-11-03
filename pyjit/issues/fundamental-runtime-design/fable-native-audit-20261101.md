# Value/native boundary audit after the Fable review

2026-11-01, bounded source/saved-evidence audit, 02:03–02:23 UTC. No target,
compiler, patch, probe or timing. Prefix-witness implementation and all earlier
helper drafts remain held. This report gives the native track a concrete first
census; it authorizes no runtime or ABI change.

**Conclusion:** the native/value boundary deserves an owned track, but the
claim that seven native-heavy losing workloads spend most of their time boxing
is not established. Existing historical counts give a much sharper lead:
`regex_compile` has very high inline-int conversion counts, while `json_loads`
and `unpickle` have zero recorded inline boxes. Select the regex code-list
numeric extraction family for a source-bound census before choosing a callback
ABI or float cache.

## Existing counts challenge the blanket attribution

The saved `6d-rung4-kernels-pystats.json` identifies source `b3b30c90830`,
GCC 15.2 and seed 0. It has 29 kernel rows. These are historical software
counters, not current-head attribution or timings. Its low/high loop subtraction
produces the following per registered loop:

| Kernel | Inline box conversions | Float subset | Object allocations |
|---|---:|---:|---:|
| regex_compile | 991,905 | 0 | 1,193,839 |
| pickle | 400 | 0 | 800.0025 |
| xml_etree_iterparse | 321 | 0 | 728,399 |
| deepcopy | 4 | 0 | 3,098 |
| json_loads | 0 | 0 | 7,240 |
| unpickle | 0 | 0 | 4,660 |
| nbody | 1 | 0 | 4 |
| float | 2 | 0 | 100,011 |
| coroutines | 2 | 0 | 242,790.5 |
| generators | 0 | 0 | 100,000 |

The regex row classifies 348,916 conversions as container store and 642,989 as
other, with none as call-into-C. “Other” does not identify one source family.
The earlier `6086cccc9c2` census also has zero json_loads/unpickle inline boxes
and 986,325 regex boxes. No old count is a CPU share or a current allocation
budget. The seven losing benchmark names do not establish a common bottleneck.

Zero inline boxes is narrower than zero representation cost. These counters
count conversion of an existing inline PyValue into an object through the
boxing helpers. Native producers may allocate numeric objects directly and
then canonicalize them, without incrementing this counter. For example,
`Modules/_json.c:1075–1097` parses floats directly into PyValue but constructs an
integer through `PyLong_FromString` followed by `PyValue_FromResult`. The bytes
buffer, parsing, dict/list construction and object-form native work still cost
time. This audit does not declare json/unpickle optimized or cost-free.

Saved-input SHA256:

- `boil/bench/results/6d-rung4-kernels-pystats.json`:
  `f01a0ad49c82494df2f341764d5efe192811247df2956537d339df983e370320`.
- `boil/bench/results/6d-before-kernels-pystats.json`:
  `22fa6ea3d6583cd71322d6d20aaf0c45eba31822092c3dca88b2d7e65db09a7e`.
- The 65-micro-row companion `6d-rung4-pystats.json`:
  `04b608c4325834d08d0bd61ad66d69156a78ade7aa15aefcf8c119a79706720c`.

Host JSON extraction initially failed because the sandbox could not create a
lock in the required shared uv cache. The same read-only command succeeded in
the escalated host context with that cache; no alternative cache or target was
used. All 29 and 65 historical `by_instruction` maps were empty.

## Available counters and their actual units

| Hook | What it measures | Important limitation |
|---|---|---|
| `Python/pyvalue.c:37–46`, `box()` | Inline int/float conversion attempts, before `PyLong_FromLong` / `PyFloat_FromDouble` | Attempts can fail/retry; cached ints can avoid allocation. Does not count every direct numeric producer |
| `pycore_stats.h:13–35`, `VALUE_BOX_STAT_INC` | Selected purpose classes | Not an exhaustive site or native-module attribution |
| `BOX_STAT_INC` and generated call/storage paths | Selected handler box events plus purpose | Rooted helper and Clinic conversions need not increment a handler event; one handler is not one native function |
| `pyvalue.c:14–34,90–109` | Return-PC sites for `_PyValue_BoxInline` and `_PyValue_BoxOrAbort` | Only 64 slots; on exhaustion the final slot accumulates overflow under its existing PC, so it is not reliably a single site |
| `Python/pystats.c:153–185,243–272` | Full handler-domain boxes/entries/pairs and raw box-site image offsets | Source/image identity and unresolved/overflow disposition must be retained |
| `boil/bench/run.py:443–473` | Low/high normalized total, type, class and selected handler counts | Drops box-site rows; `pystats_counts:394–415` deletes the raw file after parsing |

The historical emitter at `b3b30c90830:Python/pystats.c:150,179` iterated only
256 opcode IDs/pairs. The maintained emitter now uses the full IR domain and
the maintained serializer already parses `.boxes`. Consequently, an empty old
`by_instruction` is neither proof of no handler boxing nor a demonstrated
current serializer bug. Do not blindly extend the already-corrected emitter.
Preserve and inspect actual raw rows first.

The int cache is -5 through 1024 inclusive
(`pycore_runtime_structs.h:97–109`, `longobject.c:29–56`). A conversion in that
range can return an existing singleton. Dividing the historical 991,905 regex
conversions by allocations would therefore not yield an allocation fraction.
`box_after_failure` may collect and retry; a software count is not a successful
fresh allocation count, much less time spent in the allocation subtree.

## Clinic and remaining ABI boundaries

Fable's Clinic statement needs qualification. An object-typed Clinic converter
requests `PyValue_Object`, but an already object-valued input passes through.
Integer/predicate and double converters explicitly use `PyLong_AsIntValue`,
`PyObject_IsTrueValue` and `PyFloat_AsDoubleValue`. `value_converter` passes a
PyValue directly. See `converter.py:580–603` and
`converters.py:140–158,803–851,868–927`. Clinic does not box every argument or
every numeric parameter.

The remaining object-form `binaryfunc`, `richcmpfunc`, `hashfunc` and `reprfunc`
are real (`Include/object.h:292–318`). METH_O/FASTCALL/METHOD arguments/results
already use PyValue, but self remains an object; NOARGS and VARARGS retain their
own object-form conventions (`Include/methodobject.h:19–33`). Generic,
specialized and escaped method routes must agree. The existing receiver audit
already describes an 18-entry numeric NOARGS slice, separate getsets and bound
method `m_self` storage. It is an interface option, not evidence those receivers
dominate the registered native-heavy kernels.

An ABI decision must distinguish receiver, arguments, result, stored owner and
error channel. Equal 64-bit width does not make a PyValue struct callback and an
object-pointer callback interchangeable. Before a public slot conversion, record
the declared convention, generic/specialized/wrapper/Clinic closure, canonical
return checks, rooted raw-box lifetimes, SDK/ABI epoch and affected port rebuilds.
Do not choose all slot typedefs from benchmark names or preserve parallel
fallback ABIs to avoid updating callers. The selected internal census below
does not require a public typedef change or a port rebuild.

## Selected family: numeric code-list extraction in `_sre.compile`

`Modules/_sre/sre.c:1764–1802` constructs a PatternObject, then for every code
word calls `PyList_GET_ITEM(code, i)` and `PyLong_AsUnsignedLong`. The list stores
PyValue, and its object accessor calls `_PyValue_ObjectOrAbort`; an inline int
therefore requests a box simply to convert it back to an unsigned scalar
(`Include/cpython/listobject.h:39–57`). This is a concrete removable boundary
shape in the workload with the strongest saved count lead.

It is **not yet proved** that this loop accounts for most regex boxes or CPU.
Regex compilation also executes Python parsing/compiler code and uses subclassed
integer opcode constants, which retain objects. The benchmark purges regex
caches and compiles its fixed captured corpus each loop
(`boil/bench/internal/kernels/regex_compile.py:57–79`). This is a meaningful
consumer, not a fabricated numeric receiver microbenchmark.

A later change could read `PyList_GET_VALUE` and convert canonical inline ints
to unsigned scalar directly, keeping existing object behavior for bool, int
subclasses/NamedIntConstant and larger ints. No corresponding value-form
unsigned-long converter was found in the maintained long API. Its exact
semantics need design: negative/overflow/type errors, SRE_CODE narrowing,
allocation failure, partially initialized PatternObject rooting and validation.
`PyLong_AsUnsignedLong` requires an integer; it does not arbitrarily invoke
`__index__` for unrelated objects. Do not replace that contract with a more
permissive conversion.

This family has a useful existing failure distinction: GET_ITEM's boxing failure
aborts because its API lacks an error channel; `_sre.compile` itself can return
an error. Eliminating an unnecessary valid-input box removes that particular
failure opportunity. It does not authorize changing object accessors globally
or silently weakening malformed-input/error ordering.

## Minimum informative census and decision

This is a proposed separately granted census, not a run performed here.

1. Bind one actual current Py_STATS image, full-domain handler metadata, source,
   config, modules and seed/mode. Use the existing box-metric warmup plus low/high
   protocol on **regex_compile, json_loads and unpickle only**: nine fresh
   processes, with existing registered loop pairs 1/3, 200/600 and 300/900.
   Retain each actual raw stats file before its existing deletion; copy bytes,
   never synthesize zero rows. Treat setup subtraction and negative/noisy deltas
   honestly. This census has no elapsed acceptance or hardware-counter dependency.
2. Keep totals, type/class events, full handler entries/boxes and image-relative
   box PCs. Resolve existing return-PC sites against the actual image/source.
   Missing sites, a full 64-slot table, unresolved shared-library origins or
   failed metadata mean attribution is incomplete. In particular, do not report
   the table's final bucket as one known hottest caller when it may contain overflow.
3. If that cannot isolate the selected loop, the smallest follow-up instrumentation
   is Py_STATS-only scalar counts at **that loop**, before conversion: inline-int
   reads inside the cache range, inline-int reads outside it, inline-float reads
   and object reads. Export through the existing stats serializer, not a new
   registry. Add an explicit box-site-overflow count if PC attribution is used.
   This needs a separately reviewed source/build grant. No normal release probe,
   per-element allocation, callback, timing call or public ABI extension is needed.

The census answers whether this family actually requests many fresh-capable
int boxes and what part is cache reuse; it cannot prove time dominance. Select
implementation only if current site counts show a material repeated boundary.
Then a matched source ablation restoring the object accessor/converter provides
the causal control. Require useful output/error/OOM/root tests and compare the
unchanged regex consumer with json/unpickle and pure-Python controls. Whole
elapsed plus GC/allocation and RSS effects decide whether the change earns its
scope; counts alone cannot adopt it. If site counts are small, reject this
candidate and select another family from the retained sites without a blanket
C API rewrite.

Freeze a finite census/helper window before activation; a proposed ceiling is
30 minutes including all nine processes and cleanup once an exact qualified
stats image is available. A build is a separate bounded prerequisite, not
included by implication. Stop at the first unexpected failure and preserve it;
no broad 29-workload profiling campaign is required to ask this first question.

## Float caching is not semantically free

`PyFloat_FromDouble` currently calls `PyObject_Malloc` and initializes a fresh
object; `_PyFloat_ExactDealloc` calls `PyObject_Free`
(`Objects/floatobject.c:125–135,227–241`). There is no maintained float freelist
in those paths. This verifies the allocation mechanism, not its importance on
any selected workload. The historical nbody/float rows above provide no float
boxing lead at all.

**Caching live boxes by numeric value** changes native pointer sharing and
requires a declared retention/root policy and bounded storage. Python's
canonical PyValue identity is bit-based for inline int/non-NaN doubles, while
`Py_Is` remains pointer equality for C boxes. Signed zero has distinct bits;
NaNs remain boxed and preserve object identity, and subclasses retain their
own identity/behavior. A cache must not conflate these or canonicalize a raw
box root into an inline value while C still holds the pointer. The current
header explicitly says separate conversions can produce separate boxes.

**Reusing dead float storage** is a different mechanism and need not share
simultaneously live objects. It can occur only after the collector proves
death and completes lifecycle/quarantine obligations. Py_DECREF is a keepalive
barrier here, not a prompt destruction event (`Include/refcount.h:75–136`).
Moreover sweep defers actual block return: `PyObject_Free` is suppressed for
`gc.sweep_block`, and `_PyGC_FreeDeadBlock` later clears map membership and frees
storage (`gc_live.c:1350–1459`, `obmalloc.c:1674–1709`). Copying an upstream
deallocator freelist into this path would not establish correct reuse ownership.
Reused storage needs map/header/publication reset, domain/thread ownership,
bounded retained bytes, KILL poisoning/quarantine behavior and finalization of
the pool. Mimalloc already reuses small blocks, so another pool needs evidence
of incremental benefit. Fable E2's “none beyond canonical form” obligation is
therefore insufficient for either a cache or a freelist.

The next owned native action is the source-bound regex extraction census above.
Float caching, numeric receiver conversion and exported slot changes remain
distinct hypotheses with their own measured consumers and complete contracts.
