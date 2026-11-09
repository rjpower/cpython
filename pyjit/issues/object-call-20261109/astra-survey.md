# Object lookup and call conventions: independent source review

Reviewed 2026-11-09 against runtime `4d666e1b482cb4983c7683f91c5922af14d0ab40`.
Canonical planning documents are in `python-jit`; implementation references below
are to the measured `execution-boundaries-20261108` worktree. This is a read-only
review of runtime source and existing measurements. No native tests or new
performance measurements were run. The coordinator owns checkpoint publication,
reproduction, implementation admission and merged validation.

The current proposal is directionally sound. Keep canonical owning homes,
overlapping argument windows, explicit recipients and real continuations. Choose
**canonical class-value lookup** and **fixed-keyword binding plans** as the two
independent implementation lanes. First triage the borrowed-cache retirement
hazard below. Do not infer a need to replace the object layout from the class
scalar microbenchmark: that case currently misses specialization altogether.

## A source-backed ownership finding

The existing object-form non-descriptor handlers are unsafe templates for the new
canonical cache. `Python/bytecodes.c:4055` and `:4071` close `owner` before acquiring
the result from borrowed `descr`. This ordering survives generation:
`Python/generated_ir_cases.c.h:13280` and `:13346` consume the receiver, call
`_PyValue_XCLOSE_POINTER`, and only then duplicate `descr`. The helper in
`Include/internal/pycore_value.h:936` performs a normal mortal decref. There is no
result pin or retirement/refresh transaction between these statements.

A last-owner receiver can run `__del__`, whose replacement/deletion of the class
attribute can release the cached object's last owner. Type-version invalidation
does not protect a borrow already held across that callback. Reading it afterward
can therefore use a stale object pointer. This is a concrete source-level hazard,
not an observed crash; reproduction remains required. A useful witness warms
`read(o): return o.k` with a retained receiver and a sole-class-owned object value,
then calls it with an ephemeral receiver whose finalizer replaces `type(self).k`.
Cover both ordinary managed instances and instances with no dictionary. Preserve
an identity witness without accidentally keeping the old value alive.

Adjacent `LOAD_ATTR_SHAPE` and `LOAD_ATTR_WITH_HINT` acquire the result first and
use `IR_RETIRE_PUBLISHED_RESULT`. The new lane should use that established
publication/retirement contract, including folded-local mutation and error
refresh, and review the existing two paths. Merely moving the duplicate earlier
addresses the borrow lifetime but does not by itself establish the full observer
and local-publication contract. This finding contradicts any blanket claim that
all specialized attribute readers already share the safe transaction.

Treat a confirmed stale-pointer failure as a correctness repair required before
promoting a new runtime as qualified, not as a discretionary performance lane.
Publishing the frozen historical image as a comparison control is a separate
decision; it must carry this limitation if confirmed rather than imply it was
repaired. Do not mutate that image or its evidence in place.

## What the runtime already implements

| Mechanism | Evidence at the frozen runtime | Consequence |
| --- | --- | --- |
| Canonical values in attributes and dictionaries | `Include/internal/pycore_dict.h:166`, `Python/bytecodes.c:2691` | Object pointers are metadata/bridges, not a reason to create another value representation. |
| Shared-key inline instance values and per-type shapes | `Include/internal/pycore_dict.h:332`; release `pyconfig.h:1907` enables `Py_SHAPES` | This is already a hybrid of compact indexed storage, shape guards and dictionary semantics. |
| Bounded shape transitions | Same header: 64 shapes/type, presence bitmap, insertion-order paths, stable shared-key slot per name | More transition machinery requires evidence that these limits cause important misses. |
| Shape-polymorphism fallback | `Python/specialize.c:889–977` | Different shapes of the same type deliberately fall back to presence-checking instance-value handlers. A larger PIC is not automatically better. |
| Dictionary entry hints | `Python/specialize.c:984`; `Python/bytecodes.c:2736` | Materialized combined Unicode dictionaries already get a cached entry index, bounds/key-identity/non-NULL checks and canonical result duplication. |
| Narrow dictionary first-probe path | `Objects/dictobject.c:506`; `Python/bytecodes.c:1201` | It only accepts exact dictionaries, inline-int queries, general combined tables and an identity hit. It is not a string-name cache. |
| Positional binding plans | `Include/internal/pycore_feedback.h:150`; `Python/ceval.c:3576` | Exact Python positional calls already transfer an owning window into a child and fill defaults without a generic transaction record. |
| Generic keyword binding | `Python/ceval.c:2741`; `Python/bytecodes.c:5005` | Keyword Python specialization still invokes the provider; binding scans parameter names, first by pointer identity, then by equality. |
| Name interning | `Objects/codeobject.c:173,1624,1630` | Code names and local names are already interned. Adding interning to the same path cannot remove its linear keyword scan. |

Instance shadow invalidation deserves an explicit test, not an invented second
version scheme: `Objects/dictobject.c:1537` invalidates the owning type when adding
a shared key, and rechecks after watcher callbacks. The scalar cache must retain
that chain as well as type/MRO and descriptor guards. A name present in shared
keys can also be absent on this particular instance; the first bounded admission
may decline that case, but must record the coverage limitation.

The unchanged `pyjit/bench/internal/micro/dict_subscript.py` queries the string
`'c'` in a four-entry string-key dictionary. Its first-probe helper admission is
therefore impossible regardless of collision geometry. The next-steps advice
to inspect first-probe hits is useful broadly but insufficient for this witness:
first measure the actual Unicode-key provider and canonical publication path.
An exact-Unicode identity-first probe or stable-entry hint is a justified bounded
experiment; choosing between them requires checking their guard and miss costs.

## LuaJIT, V8 and the table-backed hypothesis

LuaJIT really does specialize constant hash keys to observed slots. Its recorder
guards the hash mask, loads the current node base and emits guarded `HREFK` with
the observed slot; unsuitable cases use `HREF`. This is a trace-compiler
mechanism, not a universal fixed field layout. [LuaJIT recorder, `rec_idx_key`](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_record.c)

On x86, `asm_hrefk` compares the key at that offset and exits on mismatch. The
mask guard alone is insufficient: same-sized tables can have different contents.
The lesson for this interpreter is to validate the current entry, never to keep
a naked hash-node pointer through resize or callbacks. [LuaJIT x86 assembler](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_asm_x86.h)

LuaJIT tables have array and hash parts; string lookup starts at a hash position
and follows collision links. Its string lookup compares string-object identity.
These are actual implementation facts, not evidence of hidden-class transitions
for ordinary Lua tables. [LuaJIT tables](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_tab.c)
`lj_str_new` interns strings through a hash table and reuses an equal existing
string. Importing this policy wholesale would add creation-time lookup and
retention/collector costs to Python workloads that do not repeatedly use strings
as names. [LuaJIT strings](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_str.c)

V8's architectural account describes shared HiddenClasses and indexed property
storage, alongside dictionary storage for frequently changing properties.
It illustrates a workload tradeoff, not a requirement that one representation
win for every object. The article is historical and is not used here as a claim
about every current V8 optimization. [V8 fast properties](https://v8.dev/blog/fast-properties)

The current runtime already implements the most immediately transferable idea:
`LOAD_ATTR_WITH_HINT` checks a current dictionary entry. Extending measured gaps
in that path is a smaller experiment than replacing ordinary attributes with a
new table. The constant class scalar failure is instead an object-form feedback
coverage defect (`INLINE_VALUE` returns failure at `Python/specialize.c:1273`).
Fixing it cannot establish whether tables beat indexed instance values.

Treat explicit `__slots__` carefully. Python slots define descriptors, storage,
dictionary/weakref availability and inheritance constraints. Storage hints may
guide an implementation while preserving those semantics; silently making slots
ordinary dictionary keys is a language change. For ordinary attributes, infer
capacity/layout from existing shared keys or observed construction patterns
before proposing a new annotation or globally allocating per-instance tables.

## Ranked bounded ideas

Benefits below describe scope and confidence, not promised speedups. The measured
micro ratios include loop cost and are not removable whole-program fractions.

| Rank | Experiment | Bounded benefit and cost | Stop condition |
| --- | --- | --- | --- |
| 1 | Canonical class-scalar cache, with safe result acquisition/retirement | High confidence that it removes a real generic route; application benefit unknown. Small-to-medium change across feedback, specialization, handlers and generated products. | Warm handler never hits, boxing persists, or class-heavy application improvement does not repeat. |
| 2 | Fixed-keyword name-to-local plan consumed by the existing binding transaction | Medium-to-high confidence of fewer repeated name scans; benefits fixed call shapes only. Medium cost: plan storage, invalidation and alias-safe transfer. | Saved lookup work is outweighed by guards/moves, or gains require a second binder semantics. |
| 3 | Value-native cell/getset result route | High confidence of eliminating a witnessed box/republication pair; narrow workload benefit. Low-to-medium cost using typed value callbacks. | Boxing counts do not fall or descriptor/error behavior changes. |
| 4 | Complete generator-cycle work ledger, then remove one redundant action | Broad measured generator gap; attribution less complete. Medium/high ownership and control-flow cost. | Only creation improves while steady resume/yield cost remains; classification/retirement work merely moves elsewhere. |
| 5 | Extend entry-hint coverage only where hit/miss census finds a gap | Potential moderate benefit for stable dictionary keys, low benefit when existing first probe already hits. Medium cost; start exact Unicode keys or a proven split-table miss, not arbitrary equality. | Dominant sites already hit current paths, added guard cost exceeds removed probing, or mutation churn defeats reuse. |
| 6 | Share structured ownership facts for one emitter producer | Mainly reduces proof duplication and enables later bounded retirement changes; no immediate speedup assumed. Medium tooling cost. | It expands into a general effect system without identical generated output or a concrete saved dynamic action. |
| 7 | Measure shape transitions and name interning before changing layout policy | Potential memory/construction benefit in shape-heavy or dynamic-name workloads; lowest evidence. Census is cheap; global layout/interning change is expensive. | Existing name interning/shared keys cover the hot sites, or construction/retention costs exceed lookup savings. |

Ranking keyword binding above generators here reflects the requested object/call
focus and independent bounded implementation, not a claim that keywords account
for more total time. The original generator priority remains plausible. No
available profile establishes a positive whole-program gain for either selected
lane, so an honest benefit interval still includes zero.

## Lane A: canonical class-value lookup

First scope: non-object canonical class values on the already-supported
unshadowed ordinary-instance shapes; keep existing descriptor/custom-hook
fallbacks. Do not box a scalar to satisfy `_PyFbAttr.descr`. Cache a typed logical
value or a guarded current-home index with an explicit lifetime rule. A scalar
cache owns no mortal object; broadening it to object values requires a valid
borrow anchor or owned cache with clear/traverse discipline. Audit `_PyFbPoly`
and feedback readers as well as the main entry; its current descriptor field is
also an object-oriented pointer (`pycore_feedback.h:178`).

Acquire the result before any receiver retirement. Use the established temporary
publication and folded-local retirement behavior. Cover mutations of the class,
base class and MRO; instance shadow insertion/deletion; data/non-data descriptor
replacement; custom access hooks; temporary receivers and reentrant finalizers;
cache clearing and owner collection. A mutation must miss or produce the current
language result, and no stale borrowed cache object may survive a callback.

Suggested falsifiable performance gate: show the intended handler and over 95%
warm hits in a stable witness, zero boxing attributable to its class scalar,
and at least 20% fewer raw instructions for the unchanged class-attribute
microcase. Require a repeatable elapsed gain in a representative class-heavy
workload; report DeltaBlue separately even if it does not improve. These numerical
thresholds are proposed experiment criteria, not predictions. Retain only if
balanced controls show no repeatable material regression in other attribute,
call and allocation cases.

## Lane B: fixed-keyword binding plans

First scope: exact Python functions, a stable exact-string keyword tuple, a fixed
positional count, and an already-supported provider/frame shape. Cache the
validated permutation, not argument values. Guard the function/code version and
keyword shape under a documented owner. Do not assume tuple identity without a
live anchor, and do not use pointer equality as the only correctness route for
non-interned equal strings. Unsupported mappings and string subclasses should
continue through the existing provider.

The coordinator's revised bounded design is sensible: store up to three
`uint16_t` destination indices in the existing six-byte plan payload; support
arbitrary positional prefixes, one to three keywords, omitted positional defaults
and explicitly supplied keyword-only arguments. Decline variadics and missing
keyword-only defaults initially. This limits cache capacity rather than hardcoding
a particular three-argument signature. Checking each current keyword pointer
against the guarded code's name at its cached destination avoids retaining the
keyword tuple altogether. Validate keyword count and destination bounds/collisions
before effects; equal-but-nonidentical strings take the ordinary provider. Cover
one/two/three keywords, reordered signatures, and over-capacity misses, then
measure coverage before expanding the payload.

The plan must feed the consuming binder rather than create another binder.
Overlapping windows make a permutation a parallel move problem: preflight
duplicate/positional collisions before effects, then use a proved cycle-safe
move order or bounded initialized scratch with explicit owners. A simple scatter
can overwrite an unread argument. Read current defaults, including mutable
`__kwdefaults__` contents; function-version guards alone must not be presumed to
notice every mutation of a dictionary. Complete fallible preparation before
commit; after commit use normal child cleanup, never restart the generic call.

Behavior gates: positional-only and keyword-only arguments, defaults changed
between calls, duplicate/unexpected/missing keywords, functions/code replaced,
bound methods, `*args`/`**kwargs` fallback, recursion errors, monitoring, partial
child failure, finalizer reentry and visible locals. Verify exact one-time
consumption of each evaluated argument, initialized unused homes, and one
recipient/continuation path on success and error.

Suggested performance gate: at least 15% fewer raw instructions and 5% lower
paired elapsed time for the unchanged fixed-keyword witness, plus a repeatable
gain in a representative keyword-heavy application and no material positional
call regression. Count name comparisons, moves, fallback frequency and feedback
bytes; a fast witness purchased with widespread extra frame work is not a win.

## Measurement and abstraction assessment

The closing report correctly distinguishes 1.175× broad elapsed time from raw
and skeleton-subtracted instruction ratios. DeltaBlue's 1.869× instructions and
better relative CPI justify investigating executed work, but do not identify its
class-scalar share. The reported instance-allocation sweep is approximately
0.970× upstream, an unpaired diagnostic with no evidence for a global instance
layout replacement. A huge isolated lookup ratio is compatible with a tiny
application fraction.

Keep one frame contract. Ownership is independent of future reads; function and
code owners are distinct; arguments transfer only at a non-reentrant commit;
output ownership survives child retirement; locals remain observable; continuations
are executable positions pinned by code. Neither lane needs borrowed frame banks,
new per-frame flags, alternate return adapters or a second value ABI.

The proposal needs a call/lookup shape census before further ranking: instance
value versus class value versus descriptor, shape versus presence versus hint,
first-probe hits by key kind, positional/keyword counts, generic-provider causes,
and whole generator-cycle action counts. Instrumented runs locate mechanisms;
uninstrumented balanced runs determine elapsed benefit. Preserve the published
checkpoint, fixed inputs, compiler settings and existing failed-test disclosures.
The ownership finding above warrants focused reproduction before declaring the
checkpoint ready; this review does not certify the entire interpreter.

Sources: canonical [next steps](../frame-call-model-20261109/next-steps.md),
[closing report](../frame-call-model-20261109/closing-report.md),
[micro addendum](../frame-call-model-20261109/final-micro65-priority-addendum39.md),
and [frame contract](../../design/call-and-frame-lifetimes.md). Runtime file/line
references are relative to
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108`.
External LuaJIT links were inspected on 2026-11-09 and follow its moving `v2.1`
branch; they establish mechanisms, not a pinned performance comparison.
