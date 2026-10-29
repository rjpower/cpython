# 6e: inline numeric receiver audit

Source-only, 2026-10-29. No code, build, test, probe or benchmark changed or
ran. This narrows the receiver handoff in [note 12 §2.4/open question 1](../12-object-slots-on-values.md)
and [06 §21.6](../06-architecture-v2.md), using the existing
[ingress audit](pre-I-value-ingress-audit.md). It approves no calling convention,
type flag, opcode or port rewrite. Timing binding and H3 handoff take priority.

Inspected primary `57b5a61a6f2` and ROOTS `ce2ac8991ee`. Numeric bodies,
Clinic converter/generator and method typedefs agree between these snapshots;
ROOTS changes descriptor/call lifetime scaffolding. Its source is still a
qualifying candidate, not an integrated 6e implementation.

## Actual receiver conversions

| Route | Current boundary and consequence |
|---|---|
| Immediate `x.bit_length()`; unbound `int.bit_length(x)` | Generic `_LOAD_ATTR`/`get_method_value` and specialized METHOD_SHAPE can forward descriptor plus **PyValue receiver** without a bound-method allocation. Generic descriptor vectorcall then calls `method_check_args`; it boxes self. ROOTS checks type/keywords first and stores the box into the caller's raw object-root slot. `method_vectorcall_NOARGS` then calls the object-result trampoline and canonicalizes with `PyValue_FromResult`. |
| Specialized NOARGS | `specialize_method_descriptor` selects existing CALL_METHOD_DESCRIPTOR_NOARGS. Its guard checks declared convention/arity/exact receiver type. The body calls `_PyValue_ObjectRooted(..., _PY_BOX_CALL)`, invokes the object-self/object-result trampoline, unroots, and canonicalizes the result. The emitted handler contains that one explicit receiver conversion; it does not reveal a second eager receiver box. |
| Other specialized methods | METHOD_DESCRIPTOR_O boxes self with the same rooted helper; FAST/FAST_WITH_KEYWORDS helpers in ceval.c call `PyValue_Object(arguments[0])`. Arguments/results already use values for O/FAST conventions; receiver remains object. ROOTS precise ownership and current call-window alias/anchoring contracts must be retained. |
| `m = getattr(x, 'bit_length'); m()` | `generic_getattr_value` calls `descr_get_value`, which boxes owner before method_get constructs a PyCFunction. Its `m_self` is a **PyObject pointer**, visited and used by repr/reduce/equality/hash as well as calls. A new value-self callback does not by itself remove this storage box. `cfunction_vectorcall_NOARGS` must nevertheless honor any changed callback ABI for the boxed receiver. |
| `(5).real`, `(2.5).imag` and other numeric getsets | The generic executor boxes inline owner in `box_owner_for_descr`, raw-anchors it and calls `getset_get`. Getter result is already PyValue, but `getter` still takes `PyObject *self`. Even constant getters such as float.imag pay this receiver conversion. Six own numeric entries use this path. |
| Explicit descriptor/wrapper call | `int.real.__get__(x, int)` and `int.__add__(x, y)` also traverse object-argument descriptor/wrapper interfaces. Their boxes are distinct from ordinary immediate method calls; changing numeric method callbacks alone does not eliminate them. Inherited `object` descriptors and explicit `super` binding retain their own contracts. |

Source anchors: Objects/object.c `box_owner_for_descr`, `descr_get_value`,
`get_method_value`, `generic_getattr_value`; Objects/descrobject.c
`method_get`, `getset_get`, `method_check_args`, `method_vectorcall_*`;
Objects/methodobject.c `PyCMethod_New`, `cfunction_vectorcall_NOARGS`;
Python/bytecodes.c descriptor-call family; Python/ceval.c descriptor helpers;
Python/specialize.c `specialize_method_descriptor`. Existing generated
handlers, properties and call-effect/root checkers remain the authorities.

## The historical chaos number

Current chaos runs 5,000 image iterations per workload loop. Every
transform_point calls get_random_trafo, which makes exactly two randrange
calls. random._randbelow_with_getrandbits computes `n.bit_length()` once,
**outside** its getrandbits rejection loop. Thus source predicts 10,000
bit_length calls per workload loop; registry loops=2 gives 20,000 per
registered invocation. This is a source count, not a new runtime measurement
or a proof of every box's attribution.

Historical `6d-rung4-kernels-pystats.json` reports 10,000 “call into C”
boxes per chaos loop, 135,290 “other”, 145,290 total int boxes and 251,037
allocations, normalized from loops [2,6]. The earlier rung-2/control rows
agree on the call category. Those broad counters support the distinction;
they do not independently identify each call site. The old “20,000 per loop”
handoff must not become a current total-box or allocation budget. New
source-bound box-site/allocation evidence is needed after the ABI is chosen.
No historical JSON/log is changed, and no elapsed benefit is predicted.

## Smallest coherent interface option

The own tables contain **27 methods**, not an undifferentiated set of
roughly 50: 14 int and 13 float. Four are classmethods receiving a type
(int.from_bytes; float.from_number/fromhex/__getformat__). Five other
instance methods use O/FAST conventions. **18 NOARGS instance entries** form
one reviewable first receiver-convention slice:

- int: conjugate, bit_length, bit_count, as_integer_ratio, __trunc__,
  __floor__, __ceil__, __getnewargs__, __sizeof__, is_integer.
- float: conjugate, __trunc__, __floor__, __ceil__, as_integer_ratio,
  hex, is_integer, __getnewargs__.

This is a readiness option, not an implementation decision or numeric scope
threshold. A broad `PyCFunction`/getter self-typedef change would also rewrite
unrelated native callbacks and ports. A blanket receiver-type flag alone
does not distinguish inherited descriptors, classmethods, wrappers and
different callback/result ABIs. The owner review must settle how the
**declared descriptor/callback convention** identifies value-self and its
result, using existing method metadata/construction and one consistent
dispatch path. No separate opcode/effect tables or bit_length-only intrinsic
are needed to state this interface. Do not infer ABI from equal word width.

The coherent caller closure includes MethodDescr construction/vectorcall,
the specialization selector and existing NOARGS guard/body, PyCFunction
construction/vectorcall for escaped methods, Clinic self/parser/result
generation, manual long_long_meth entries and actual numeric implementations.
`self: value` does not exist as a completed Clinic feature: self_converter
still derives an object parser type; parse_args.returns_value excludes
NOARGS; gc_precise generation currently raw-appends self as an object.
These must change together for any value-self/value-result convention.
Methods returning strings/tuples still allocate their legitimate results.

The six getsets (int real/imag/numerator/denominator; float real/imag) are a
separate next interface: PyGetSetDef has object-self getter pointers and no
per-entry convention field. Their direct executor path and explicit __get__
must agree before claiming receiver-box removal. Bound-method `m_self`
storage is another explicit scope choice; retaining its actual object box
is compatible with a first immediate-call improvement, but not a claim of
zero receiver boxes for all attribute forms. Remaining slot families and
Clinic residue stay with the existing 6e handoff, not this first slice.

## Roots, ports and falsifying evidence before implementation

ROOTS's initialized spans/raw object-slot receiver handoff are the starting
contract. A value-self path keeps actual heap receivers, descriptor and any
arguments/results live across collecting calls; inline values need no box.
Do not canonicalize a retained raw box root. Unknown native callbacks remain
collecting under the existing checker; NOARGS is not a purity claim. Preserve
error ordering, recursion guards, monitoring delivery and exceptional cleanup.
Ingress/canonical-result checks apply to semantic values, separately from
raw roots. Bool, large ints, numeric subclasses and boxed NaN must retain
their present acceptance, result and identity rules; returning self blindly
would break several inherited numeric methods.

Review oracles should pair direct syntax, unbound generic calls, specialized
calls and stored bound methods; exercise arity/keyword/wrong-receiver errors,
subclasses, int32 edges including INT_MIN, large ints, signed zero/infinities/
NaN, collecting callbacks and MemoryError. Measure receiver conversions
separately from real output allocations. Prove generic/specialized agreement
and reject a copied wrong-convention callback at the declared boundary.
Affected modules include test_long, test_float, test_descr, test_call,
test_clinic, test_monitoring and the existing collector/call lifetime cases.
Exact runtime plans and native ownership remain to be reviewed.

Ports remain frozen at value-v1; SDK records value-v2. A settled exported
callback/header/layout change requires the existing ABI/suffix/SDK decision
and rebuild process. Audit Cython/cffi generated calling-convention emitters,
native method/getset definitions and explicit function-pointer casts; numpy
subclasses cannot be assumed to adopt exact builtin receiver semantics.
Pure-Python ports need no C signature rewrite. No port build or old wheel
is qualification for the proposed convention. First choose priority from
the early elapsed diagnostic and actual box budgets; reject a broad ABI
rewrite whose useful receiver savings do not justify its complexity.
