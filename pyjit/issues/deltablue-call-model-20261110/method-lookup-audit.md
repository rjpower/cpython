# Method lookup and complete ordinary calls

Read-only proposal against runtime `b8e7e1bee6611eeda320ce163cd3ce89323da877`
(candidate46). Preferred first increment: **forward exact Python classmethods
accessed through a class directly into the existing owned function/class call
pair**. Keep ordinary binding, frame entry and return. No benchmark names,
method names, new hash layout or coroutine entry changes are needed.

## Actual application paths

The source is `pyjit/bench/internal/kernels/deltablue.py`. Evidence `E` is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.
The retained `coverage46-candidate-deltablue.json` has matching pre/post runtime
identity and source hashes. It inspects 67 module-defined code objects after
setup and one 30-loop warmup. Its scope includes dormant functions and excludes
module top-level code; handler counts are static sites, not execution counts.

All 328 ATTR sites have private feedback; none uses the shared admission
fallback. Planned ATTR bytes are 15,744 and all planned entries total 28,192;
26,664 entry bytes are allocated after warmup. The largest individual plan is
3,000 bytes in `BinaryConstraint.choose_method`, far below the 32,768-byte
per-code capacity. Increasing that capacity cannot explain or fix these generic
sites. The warm site distribution is:

| Handler | Sites |
| --- | ---: |
| `LOAD_ATTR_METHOD_WITH_VALUES` | 78 |
| `LOAD_ATTR_SHAPE_LOCAL_TEMP` | 78 |
| `LOAD_ATTR_SHAPE` | 31 |
| `STORE_ATTR_SHAPE` | 34 |
| `STORE_ATTR_SHAPE_ADD` | 19 |
| `LOAD_ATTR_CLASS` | 17 |
| `LOAD_SUPER_ATTR_METHOD` | 12 |
| Generic `LOAD_ATTR` / `STORE_ATTR` | 35 / 4 |

The remaining sites are in code without allocated feedback. Examples connect
the actual word offsets to source, without claiming dynamic per-site counts:

| Source family | Retained warm IR evidence | Current route |
| --- | --- | --- |
| `self.input()`, `self.output()`, `c.execute()`, planner methods | `BinaryConstraint.recalculate` words 1/3 and `Plan.execute` word 4 use `METHOD_WITH_VALUES` | Guarded descriptor → owned function/self → exact or general CALL → ordinary frame → ordinary return |
| `Strength.weakest_of(...)` | Both `BinaryConstraint.recalculate` and `ScaleConstraint.recalculate`, word 6, remain `LOAD_ATTR` | Generic class lookup → classmethod descriptor → allocated bound method → bound-method CALL unwrap |
| `cls.weaker(s1, s2)` | `Strength.weakest_of`, word 1, remains `LOAD_ATTR` | Same classmethod route, with the actual class received as `cls` |
| `s1.strength`, `s2.strength` | `Strength.stronger`/`weaker`, words 1/2, use `SHAPE_LOCAL_TEMP` | Canonical field read; body comparison and return remain ordinary |
| `Strength.REQUIRED`, etc. | `Constraint.satisfy` word 9 and six `next_weaker` constant loads use `LOAD_ATTR_CLASS` | Existing class-version guarded object payload |
| `self.__class__` | Six loads in `Strength.next_weaker` remain generic | Special descriptor case, separate from classmethod forwarding |
| `Direction.NONE/FORWARD/BACKWARD` | Generic sites include `BinaryConstraint` direction operations | Canonical scalar through a class is a separate current specialization gap; generic sites are not all method misses |
| Explicit `super(C, self).__init__` and super methods | 12 warm `LOAD_SUPER_ATTR_METHOD` sites | Existing super lookup/guard and function/receiver route |
| Property | No `@property` in this application | General runtime family to preserve and later unify, not an observed DeltaBlue cause |

The source has four classmethod definitions and ten syntactic classmethod call
expressions: six `stronger`, two `weaker`, two `weakest_of`. One expression is
inside the unexecuted `Strength.strongest`; nine have an executing source caller
in this warm snapshot. This is static scope, not nine calls or a heat ranking.

The independently decoded retained46 profile has 2,500 candidate DeltaBlue
samples, zero lost samples, and about 0.76% unresolved self share (rounded-row
sum). Its leading locations include exact CALL, method/shape loads and generic
type lookup. Of 214 callchains containing `_PyTypeCache_Lookup`, the nearest
handler is generic `LOAD_ATTR` in 211 and `STORE_ATTR` in three. This supports
investigating generic lookup but does not identify the descriptor at each sample.
Upstream puts much of its work inside one evaluator symbol; comparing its symbol
shares with candidate handler shares would not compare equivalent operations.
See `../object-call-20261109/current-profile46-analysis.md` for identities and
all top-ten tables. Inline DWARF names are attributed code locations, not proof
of physical helper calls or nonzero slot initialization.

## Existing contract and the missing link

`Python/specialize.c:analyze_descriptor_load` respects overridden access,
descriptor classification and type versions. Ordinary unshadowed instance
methods use `specialize_attr_loadclassattr`, a version-guarded non-owning
descriptor cache, instance-layout checks, and `LOAD_ATTR_METHOD_WITH_VALUES` or
its no-dict/lazy-dict variants. `Python/bytecodes.c:_LOAD_ATTR_METHOD_WITH_VALUES`
acquires the function and transfers the receiver into the ordinary call window.
There is already one canonical representation: owned `PyValue` callable,
owned `PyValue` receiver-or-NULL, followed by owned argument slots.

Classmethod forwarding is missing. Both instance and class specializers decline
the unshadowed `PYTHON_CLASSMETHOD` case. `Objects/object.c:_PyValue_GetMethod`
explicitly documents why its status/value result cannot forward a receiver
different from the original object. Its generic classmethod path consequently
returns a bound-method object. `Objects/funcobject.c:cm_descr_get` constructs it
with `_PyMethod_NewValue(cm_callable, bound_class)`. Class access binds the
actual accessed class, including inherited descriptors; instance access binds
the instance's type. `_INIT_CALL_BOUND_METHOD_EXACT_ARGS` later duplicates that
function and receiver and closes the temporary bound method.

CALL then checks function version, positional count, stack and recursion,
transfers the canonical window to `_PyFrame_PushOverlapping` for thread callers
or copies it into an independently allocated frame for generator callers,
sets the return destination and enters. RETURN keeps an owned canonical result,
unlinks the child before clearing locals, and uses the existing caller result
publication path. Forwarding improves the whole route by removing an avoidable
intermediate object and unwrap; it does not remove frame initialization or
frame retirement. Those costs must remain visible in the combined design.

Property paths differ from method forwarding. For a supported plain instance
property read, `LOAD_ATTR_PROPERTY` guards the receiver and getter function,
pushes a getter frame with `_PyFrame_PushUnchecked`, transfers the receiver as
argument zero, and uses `_SAVE_RETURN_SLOT`/`_PUSH_FRAME` and ordinary return.
It does not already use the overlapping method window. Method-form property
loads are explicitly declined: `o.p()` must first run the getter, then call its
returned value; that value need not be a Python function. Access through a
class returns the property descriptor rather than invoking its getter, unless
metaclass lookup changes the route. Custom descriptors and `__getattribute__`
must preserve those execution points. A later entry-recipe change must audit
getter acquisition too: `property.__init__` can replace `prop_get` on the same
property (`Objects/descrobject.c:property_init_impl`), so class-version validity
alone cannot establish a permanent getter pointer. This audit does not claim
the existing property cache satisfies a new proof or authorize extending it.

## Three structural options

### A. Forward class-receiver Python classmethods first — preferred

Specialize only method-form `LOAD_ATTR`, an exact class receiver whose metaclass
is exactly builtin `type`, an exact builtin `classmethod` descriptor resolved
through normal class/MRO lookup, and a current exact Python function target.
First scope excludes custom metaclasses, descriptor subclasses, arbitrary
wrapped callables, instance receivers and free-threaded hits until a synchronized
owned-read contract exists. Misses use existing ordinary lookup semantics;
this adds no second call/binding ABI.

Use the existing ATTR metadata fields: class version and non-owning descriptor
pointer. Do not cache `cm_callable` as a stable payload. On each successful hit:

1. Check class identity/version and exact metaclass/descriptor kind before
   dereferencing cached metadata. A nonzero class version tracks MRO changes;
   replacing/deleting a descriptor or changing bases invalidates that proof.
2. Read `_PyClassMethod_GetFunc` live, check the exact function type, and acquire
   an owned canonical function value without callbacks. Transfer the owned
   class receiver to the call window as the implicit argument. No bound method
   is allocated. The transferred class also keeps inherited binding correct.
3. Publish both outputs with the ordinary method-form IR ownership contract.
   Then evaluate explicit arguments in their original order. CALL uses its
   existing current-function/version/binding guards and existing entry/return.

Crucial mutation: `classmethod.__init__` calls `cm_set_callable`, which can
replace `cm_callable` **on the same descriptor** without a class-version change,
even though `__func__` is read-only. Live-read at lookup handles this. Once the
owned function/class pair is captured, argument evaluation may reinitialize or
replace the descriptor, mutate bases or delete the class name; CALL must use the
captured target, exactly as the already-created bound method would. Never reload
the descriptor or target at CALL. Function `__code__`/defaults mutations are a
different event: CALL must validate/bind the captured function's current state,
not freeze code at attribute lookup.

Class-receiver transfer avoids retiring an unrelated instance whose finalizer
could run between descriptor guards and publication. No callbacks, decrefs or
fallible allocation may intervene between checking the cached descriptor and
acquiring the function. Under free threading, disable this specialization
rather than treating that interval as synchronized. Do not store typed borrows
in feedback. Include each new handler in existing row-copy, polymorphic
replay and snapshot handler/payload mapping; publish metadata before the handler
and invalidate/reset through existing feedback transitions. No layout growth
should be needed, but compile-time size assertions and the API gate must prove it.

For each eligible hit this removes generic class lookup/descriptor dispatch,
one bound-method allocation and later destruction, its wrapped references, and
the CALL bound-method extraction/duplication/retirement sequence. It replaces
them with version/kind checks, one live target read and ownership acquisition.
It saves zero callee activations and zero callee-local clears. The profile cannot
convert that operation ledger into an elapsed estimate.

Instance classmethod access is the next extension of the same family: guard
ordinary instance lookup, prove absence of shadowing with the existing shape/
shared-key/dict machinery, acquire function and bound type, publish both before
retiring the original instance, then refresh frame/handler identity after any
reentry. Do not forward an instance shadow value as a classmethod. Arbitrary
descriptor forwarding remains generic, since `__get__` may execute Python and
return any callable. Ordinary attribute-form access must still return a real
bound method whose `__self__`, `__func__`, identity and escape behavior are visible.

### B. Fuse a certified adjacent method lookup and ordinary exact call

For a zero-explicit-argument instance method such as `self.output()`, a fused
handler could use existing receiver type/layout and function-version guards,
capture the target, then build the same ordinary frame directly. The potential
saving is one dispatch and some intermediate call-window ownership traffic;
all frame initialization, body execution, result publication and teardown remain.
Existing methods already avoid bound-method allocation, so do not count that
as another saving. Keep the same call-window storage/binder ABI.

Do not fuse across argument expressions, monitoring/tracing events, control-flow
targets, exception-region boundaries, or a point that can observe or replace
the target. Deoptimizing CALL after successful lookup must use its captured
target; restarting lookup could call a different function or repeat descriptor
effects. A fused opcode needs both source sites, precise error attribution,
observation deoptimization and complete frame reconstruction. Prefer option A
before paying for that proof; merely seeing neighboring source expressions is
insufficient. No user-code body inlining is proposed here.

### C. Compiler-certified tiny ordinary entry recipes

Constant/argument returns and exact simple getters could avoid one entire frame
activation and retirement, using a compiler-issued immutable code recipe and
the same owned result publication machinery. DeltaBlue has ordinary methods
such as `is_input`, `output` and `input`; only a syntactically/proven supported
subset qualifies (`input` branches, so is not a trivial getter). This is a
general code-shape optimization, never a method-name table. It offers a larger
structural saving per hit than A/B, with a materially larger observation and
ownership proof. Root's entry/binding lane must specify that proof; this audit
does not approve it.

The contract must pin the selected function/code, preserve arguments and
binding errors, guard observers/monitoring/PEP523/recursion before eliding a
frame, reconstruct or decline observed execution, retain normal descriptor
semantics for field reads, and own the result before releasing input owners.
Calling a getter can execute Python; a simple return recipe must not silently
turn an arbitrary descriptor into a field read. Completion and finalizers may
reenter; exception/redirect paths must consume each owner once. A recipe proof
is insufficient if CALL can select new code after the proof was checked.

## First implementation and evidence boundary

Choose A's class-receiver method-form family first and combine it with the
ordinary entry/return lane at review. Required implementation areas are the
existing attr specializer, DSL handler/IR metadata, feedback row/snapshot mapping
and dedicated lookup/call tests. `_PyClassMethod_GetFunc` already exists; no
generic `_PyValue_GetMethod` ABI expansion is required for this first family.
No frame format, CALL result convention or return handler changes are needed.

Prove the complete route with warmed handler and call-target evidence, not just
a classmethod microbenchmark: inherited descriptors bind subclasses; same-object
`classmethod.__init__` before lookup and during argument evaluation; descriptor/
base replacement; function code/default changes; attribute escape; custom
metaclass and wrapped-callable decline; observer and exception paths. Force
class-name removal during argument evaluation and let the captured class/function
be sole owners. Verify safe lifetime and once-only cleanup without requiring
exact CPython finalizer timing. Normal instance method, property and instance
shadow behavior must remain covered as neighboring contracts.

After focused qualification, retain DeltaBlue's private-site/handler census and
the actual CALL route: selected classmethod sites should leave generic lookup
and bound-method CALL while ordinary body entry/return remains. Measure complete
DeltaBlue and representative method/classmethod/property workloads with the
same source/build identities. Attribute-site snapshots do not measure binding,
frame counts or destruction frequency; add narrowly scoped instrumentation only
if those quantities decide between A and C. Root owns all native execution.
The revised user gate permits mostly-better incremental progress with documented
regressions; report the affected cases and magnitudes rather than silently
requiring universal improvement or declaring application coverage from micros.

## Calibration from the semantic call census

The later `Enew/baseline46-semantic-calls.json`, with `Enew` at
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/deltablue-call-model-20261110`,
records one loop at embedded runtime `b8e7e1bee66`: 51,422 semantic Python calls.
Its declared limit is observed semantic counts, not timings or fast-path
coverage. These counts refine scope selection; they are not costs.

| Family | Observed calls | Share of these semantic calls |
| --- | ---: | ---: |
| `BinaryConstraint.output` | 13,717 | 26.7% |
| `BinaryConstraint.input` | 11,707 | 22.8% |
| Both conditional selectors | 25,424 | 49.4% |
| `Strength.stronger` / `weaker` / `weakest_of` | 708 / 1,205 / 804 | 5.3% combined |

Both selectors read a receiver field, compare it with a class scalar, then
return one of two receiver fields. Their recorded pristine IR has nine words:
RESUME, field load, global load, class load, conditional compare, and two
field-return arms. They are not the single unconditional getter covered by a
trivial getter recipe. Classmethod forwarding remains a small, clear ownership
increment, but this census does not justify treating it as the main ordinary
call-volume opportunity. A compiler-certified conditional-selector recipe could
remove full entry/return for a larger observed family, provided it proves the
comparison, branch and both field paths with normal observation/error behavior.
That larger proof belongs in the combined design review; call frequency alone
does not establish eligibility or payoff.

### Adjacent lane: canonical scalar loads through classes

The generic scalar-class path is independently removable work inside those
selectors. The existing instance class-value specialization classifies
non-object canonical words as `INLINE_VALUE`, stores `_PyValueGuarded` in the
independent `class_value` field and creates a transient borrow only after
guards. In contrast, `specialize_class_load_attr` does not accept `INLINE_VALUE`:
its current successful cases store object `descr`, and `_LOAD_ATTR_CLASS`
duplicates that native object payload. The warm selector snapshots confirm
their class-scalar word 3 remains generic. A direct class-scalar specialization
is therefore a coverage extension of the existing canonical cache architecture,
not a new object layout or a special rule for a benchmark constant.

Smallest coherent proposal: attribute-form loads from an exact class receiver
with builtin `type` as metaclass, where normal MRO resolution returns an eligible
non-null canonical inline scalar. Cache the guarded word in existing
`class_value`, retain the existing class-version guard and acquire with
`_PyValueGuarded_Load` plus canonical home duplication only after that guard.
Publish the owned result before retiring the class receiver through the checked
publication/retirement path. Class/MRO/base mutation invalidates the receiver
version before releasing/replacing the source value. No typed borrow or owning
value is stored in feedback, no boxing is needed, and native descriptor and
guarded-word storage remain independent. Include the new handler in row-copy,
replay and active-handler snapshot contracts. Existing ATTR48/POLY120 sizes and
capacity admission should stay unchanged and must be checked.

Initially exclude custom metaclasses, descriptor execution, arbitrary mutable
objects and method-form loads; these retain their ordinary semantics. Inherited
class constants must resolve for the accessed subclass and invalidate on base
mutation. A later extension to custom metaclasses must apply the existing
metaclass/version/data-descriptor precedence proof, not simply read a class dict.
Do not fold a global class binding into a permanent literal: the preceding
global load and class-value guard must observe rebinding and attribute changes.
If a future selector recipe incorporates the load, it must use the same guarded
facts and fail before any user-visible effect or retain a valid continuation.

Each eligible execution removes generic class-attribute lookup/descriptor
classification and replaces it with guards and a canonical word acquisition.
It removes no ordinary callee entry, receiver-field read, comparison, return or
frame clear. Each of the 25,424 observed selector invocations contains one such
class-scalar read, which identifies repeated semantic work; it is not a measured
native hit count, instruction saving or fraction of total time. Scalar lookup
and frame-elision savings must not be double-counted when combined.

Required tests/evidence: unchanged/inherited int/float/other eligible scalar
loads; scalar replacement/deletion and base mutation after warmup; scalar-to-
descriptor transitions; custom metaclass decline/precedence; global class-name
rebinding; safe result ownership across receiver retirement/reentry; codegen
evidence that no pointer conversion or heap borrow field was introduced. Then
confirm actual selector word coverage and measure complete ordinary calls and
DeltaBlue. This adjacent lane is useful even if conditional-selector frame
elision is deferred, but it does not replace the main call setup/teardown task.
