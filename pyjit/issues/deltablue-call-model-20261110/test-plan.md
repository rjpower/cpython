# Focused lookup qualification plan

Base: runtime46 `b8e7e1bee6611eeda320ce163cd3ce89323da877`. Proposed checks,
not completed evidence. B's canonical existing class-load handlers land first;
A's classmethod pair is a separately reviewed/measured second commit. Root
coordinates native execution, builds and integration. Sol reviews inner-loop
proposals and actual diffs after the single grouped architecture review.

## B: canonical direct class values

Extend `Lib/test/test_ir_attr_cache.py` with class-receiver tests using existing
`reset_code`, specialization-threshold warmup and adaptive IR helpers. Cover
both `LOAD_ATTR_CLASS` and `LOAD_ATTR_CLASS_WITH_METACLASS_CHECK`, and require
the intended warmed handler in tests that claim cache coverage.

| Behavior | Required witness |
| --- | --- |
| Canonical scalar coverage | Class attribute reads of eligible int/float values, including boundary/negative values; compare behavior and canonical identity rules, not newly invented boxing identities |
| Existing object coverage | Lists, dicts, tuples, plain functions and admitted enum-like values preserve identity and existing eligibility; no scalar-only alternate cache |
| Method-bit behavior | `C.plain_function(x)` continues to pass only explicit args; an object/scalar/non-callable result retains ordinary NULL-self/error behavior |
| Inheritance/invalidation | Warm subclass read, change/delete base value, change bases, then replace scalar with data/non-data descriptor; correct result/error without stale payload |
| Metaclass guards | Plain mutable metaclass reaches190; add/change metaclass data descriptor or override `__getattribute__` after warmup; proper precedence/deopt, no bypass |
| Reentry and lifetime | Temporary class receiver becomes last class owner; acquire an eligible object payload containing a weakrefable finalizer-bearing object before class retirement. Finalizer mutates/deletes attributes and reenters. Assert result stays alive and cleanup occurs once; exact upstream destruction timing is not the oracle |
| Destination cases | Temporary result destination and folded local destination; owner/result aliasing and displaced local owner with reentrant finalizer |
| Historical payload selection | One read site alternates classes with scalar/object payloads and builtin/custom metaclasses; inspect existing poly rows and verify recorded handler IDs plus fresh results after mutations |
| Adjacent exclusions | Property, classmethod, descriptor subclasses and arbitrary mutable descriptor behavior remain ordinary; existing instance shadow/class-value tests still pass |

Do not fabricate coverage by inspecting cold code with allocating
`_testinternalcapi.get_feedback`. Warm first before using it for row assertions;
use `_co_feedback`/adaptive `get_ir` for nonallocating snapshots. A row containing
a handler does not prove that a stale historical value is safe to use.

Extend `Lib/test/test_ir_attr_cache_codegen.py` to include both real class
handlers in acquisition/publication tests. Required generated ordering:
class/metaclass guard → `_PyValueGuarded_Load` → canonical owned acquisition →
temporary result publication → receiver retirement → any refreshed folded-local
write. Require the checked canonical-before-retire result phase, guarded cache
type, correct selector offset, effect-free optional NULL suffix, reloadable
instruction pointer, reentry refresh and error handling. Keep negative synthetic
tests for wrong cache width/type and invalid publication suffixes. Do not use
a test that merely accepts a name without checking emitted behavior.

Metadata assertions should prove both IDs189/190 select `class_value`, their
proper version operands remain unchanged, and family/layout remain ATTR48/
POLY120. Existing feedback-capacity tests continue to establish admission;
no new 700-site duplication is needed when storage has not changed. Host
generator tests precede native builds, then run the actual API gate using the
current compile database. Root regenerates and compiles release/debug: host
text inspection alone does not prove generated C compiles.

After builds, run focused attr-cache/codegen/IR-contract tests and relevant
`test_descr`, `test_class`, `test_super`, `test_enum`, `test_sys_settrace`,
`test_monitoring`, and frame/call smoke tests selected by the coordinator.
Run the project's required integration gate/full suites at the integration
point. Record actual failures and unsupported configurations rather than
claiming this plan was executed. In particular, run an appropriate free-threaded
build only if admitted in the project's current gate; otherwise label it untested.

## A: owned classmethod pair

Add a dedicated class-receiver call test class in the same attr-cache module
or a narrowly named `test_ir_classmethod_cache.py` if size warrants it. After
warming, require `LOAD_ATTR_CLASSMETHOD` plus ordinary `CALL_PY_EXACT_ARGS` or
the expected current general Python CALL. Require absence of a bound-method
CALL at that tested site. Attribute-form escape must still return a bound
method with correct `__self__`/`__func__`.

Required semantic witnesses:

* Inherited descriptor binds accessed subclass, not defining base. Different
  classes with the same wrapped function bind independently.
* Invoke `classmethod.__init__` on the same descriptor before lookup, then
  from an argument expression after lookup. The former selects the new target;
  the latter invokes the previously captured target exactly once.
* During argument evaluation replace/delete the descriptor, mutate bases and
  remove global class/function bindings; the captured pair remains owning.
  Separately mutate captured function `__code__`/defaults and require ordinary
  current-function binding and errors.
* Replace wrapped function with a non-function callable or mutate class
  metaclass where allowed: decline/deopt before output acquisition. Custom
  metaclasses, classmethod subclasses and instance shadowing retain behavior.
* Observer activation, exceptions during argument evaluation and function
  binding errors release each captured owner once. Check actual handler state
  after reentry rather than assuming warmed specialization persisted.
* Monomorphic→polymorphic observations retain descriptor-backed guarded words,
  not snapshots of mutable `cm_callable`; subsequent same-object reinitialization
  must be seen at the next lookup.

Codegen tests must prove all guards/live target type checks precede acquisition,
owned function and class fill the ordinary two-result window, no receiver close
occurs in the no-callback split path, and one existing family hit is recorded
after checks. Check numbering, deopt/instrumentation maps and generated dispatch
as part of A's separately resolved format policy. A is not implementable until
that numbering decision and broad/inner-loop reviews are closed.

## Evidence and landing comparison

Retain pre/post runtime/build identity and benchmark input identities for each
landing. Refresh the bounded 67-code/328-ATTR DeltaBlue census using the same
scope. B should change eligible scalar class sites to189/190 without altering
capacity; A should change its classmethod sites to the split-pair handler and
ordinary CALL. Snapshot counts establish coverage only. Use stats-build existing
family hits and per-handler dispatch counts for dynamic route evidence;
semantic profile counts remain a separate observer-instrumented quantity.

Measure complete DeltaBlue, representative conditional-accessor calls,
classmethod calls, property and ordinary method cases, and application controls
with the coordinator's paired protocol. Compare B versus46, then A versusB,
rather than combining unpaired sample percentages. Document elapsed/instruction
regressions allowed by the revised incremental gate. Do not infer a performance
budget from49.4% semantic selector calls or5.3% classmethod calls. Defer coroutine
entry, keyword-only micros and any selector frame-elision claims.
