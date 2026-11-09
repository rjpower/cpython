# Final micro evidence changes the optimization order

Read-only addendum to `final-engine-architecture-review38.md`, against frozen 39.
No execution, source change or new optimization admission. Root's final-v2
net-instruction results versus upstream are class_attr_load 8.830x (565 vs 64),
generator_resume 2.801x (465 vs 166), dict_subscript 2.392x (500 vs 209),
cell_contents 2.250x, call_py_kw 2.092x and str_join 2.031x. The broad elapsed
geomean is 1.175x, DeltaBlue 1.61965x and generators about 1.415x. Micro ratios
are not workload speedup budgets; root's broad profiles are still being read.

## Concrete first finding: inline class values cannot specialize

The actual `pyjit/bench/internal/micro/class_attr_load.py` defines `K.k = 1`,
creates `o = K()` once, and repeats `z = o.k`. It is an **instance load of a
class value**, not K.k. LOAD_ATTR_CLASS is not the measured route.

The frozen source gives a precise missing admission:

1. `Python/specialize.c:764–770` classifies a non-object canonical class value
   as INLINE_VALUE. K.k's exact int 1 is canonical inline, not a descriptor.
2. `specialize_instance_load_attr` at1312–1325 proves type/descriptor and
   instance shadow state, then calls do_specialize_instance_load_attr.
3. That switch's INLINE_VALUE case at1273–1277 tries the instance dictionary
   only when shadowed; otherwise it returns failure. The adjacent NON_DESCRIPTOR
   case calls specialize_attr_loadclassattr for the unshadowed class value.
4. The existing cache helper at1640–1731 accepts PyObject*descr and asserts an
   object non-descriptor/method classification. `_PyFbAttr` in
   `Include/internal/pycore_feedback.h:115–136` stores borrowed object pointers,
   not a logical cached scalar value. The NONDESCRIPTOR handlers at
   `Python/bytecodes.c:4055–4083` reconstruct their value from that object pointer.

Thus this ordinary immutable numeric class value is structurally excluded from
the guarded path, even though it needs no descriptor invocation or boxing.
The generic LOAD_ATTR route at2594–2637 remains responsible for the operation.
`Objects/object.c:1917–2065` shows its generic work: ready/type/name checks,
a rooted class lookup, descriptor classification, instance lookup/possible
dictionary handling, and ownership/error cleanup before returning the class
value. Adaptive retries additionally perform no-allocation lookup preflights
and classification; their frequency needs the actual warm-site census.

Direct class-object loads have a related INLINE_VALUE omission in
specialize_class_load_attr at1590–1631, but that is a separate coverage case,
not evidence for this micro's ratio. Metaclass checks do not explain an instance
load merely because the benchmark is named class_attr_load.

The first proposal should cache a guarded canonical class **value** under the
existing type/MRO and instance-absence proof, with explicit non-owning cache
lifetime. It should not manufacture a PyObject pointer for an inline scalar.
Invalidation must cover K.k mutation/deletion, base/MRO changes, an instance
shadow assignment/deletion and custom getattribute/descriptor behavior. Audit
the actual absence/shared-key guard rather than assuming that an inline-values
validity check alone proves absence. Produce the result before any callback-
capable receiver retirement. This is an ordinary object-protocol optimization,
not a new representation or a handler-name-only bypass.

## Reweighted next five investigations

| Priority | Concrete measured/source basis | Small falsifiable next step |
| --- | --- | --- |
| 1. Guarded class-value load coverage | 8.830x micro; INLINE_VALUE is explicitly refused on the measured unshadowed instance route. DeltaBlue's large loss makes attributes a plausible broad lead, not a confirmed cause. | Root records the warmed actual handler and specialization failures, then profiles DeltaBlue. A bounded value-cache proposal must show cache lifetime/absence invalidation and a reduced instruction ledger before broad retention. |
| 2. The full resume/yield/loop cycle | generator_resume creates **one generator per 1000 yields**. Its net result includes both caller FOR, callee range/FOR and the yield/resume transitions, not creation alone. | Count/locate instructions across the complete cycle in the retained ELF and root profile. Preserve created-send/recursion/context checks. Compare guarded common-path work and recipient publication, including any cost shifted into every resume. Direct heap creation is amortized here and drops below resume work in priority. |
| 3. Exact string-key dictionary hits | dict_subscript is a four-entry dict, fixed key 'c', inline value 3. It is 2.392x despite an existing checked first-probe route (`ir_generator.py:2896`, `bytecodes.c:_BINARY_OP_SUBSCR_DICT`). | Check actual warm specialization and first-probe hit/miss geometry before designing another cache. Decompose hashing/probing from shared publication/retirement/dispatch; if the first probe misses, determine whether a safe bounded hint helps. Keep collision, mutation and equality-hook fallbacks. |
| 4. Value-native getset completion | cell_contents reads a cell containing 3000. `Objects/cellobject.c:181–196` obtains a value then boxes it through AsObjectAt and closes the value. Its PyGetSetDef uses that object getter. `Objects/descrobject.c:214–238` already has a direct d_value record route, but this cell getter does not register one. | Propose a direct `_PyValueGetSet` record through the existing mechanism, retaining the owning object bridge and exact empty-cell ValueError/deletion/write behavior. Measure removed box/canonicalization work and attribute lookup separately; the entire 2.250x is not the boxing budget. |
| 5. Fixed keyword and small native-call transactions | call_py_kw repeatedly calls f3(a=i,b=i,c=i), and f3 returns a. The fast overlapping admission at `ceval.c:3259` is VECTOR-only; KEYWORDS still reaches consuming binding/fallback work. str_join appends 1000 strings before **one join**, so its 2.031x cannot be attributed to join alone or inline-number boxing. | Profile fixed keyword mapping/binding/owner moves and list.append separately. Cache a versioned keyword-to-local binding plan only after its function/code/default/kwnames proof, retaining errors and reentry. For str_join, inspect the actual append specialization and loop dispatch before touching unicode join. |

This replaces the earlier frame-first ranking for the next performance round.
The largest new concrete omission is attribute specialization for canonical
class scalars. Recipient descriptors, direct heap creation, semantic retirement
and compiler proof consolidation remain useful conditional architectural work;
they are not more urgent than an observed missed fast path solely because the
last round centered on frames. None is an instruction to reopen the closed
round or to trade Python's descriptor/exception/ownership semantics for a micro
ratio. Root's unchanged workloads, exact identities and broad profiles decide
which proposal receives an implementation experiment.
