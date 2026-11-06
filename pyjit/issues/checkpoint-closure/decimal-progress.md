# Decimal progress

The first signals/context increment is frozen in `decimal-signals-context.diff`;
Sol approved it in `decimal-signals-context-post-review.md`. Its release
extension build and 70 targeted context tests passed.

The separate `decimal-integer-conversion.diff` layers on that increment.
It accepts borrowed values through long conversion and uses `mpd_qset_i64`
directly for inline ints. Heap integers and bool/int subclasses retain the
existing PyLong export path, context and status handling. Its release
extension compiled and linked, 32 focused Decimal tests passed, and a direct
probe covered integer boundaries, bigints, bool/subclasses, arithmetic and
an Inexact trap. Sol approved it in `decimal-integer-post-review.md`. A first probe compared a rounded
large arithmetic result to an unrounded one; it was corrected to set sufficient
precision and passed without changing the implementation.

The module is not yet value-native. `decimal-method-census.md` records the
method families and a proposed selective wrapper approach. Float conversion,
slot migration, constructors, Clinic wrappers and final module-wide marked
compile and box gates remain.

The separate `decimal-float-conversion.diff` layers on the integer increment.
The release extension compiled and linked; 84 focused constructor/context
tests and a direct special-value, float-subclass and FloatOperation probe
passed. Sol approved the null-safe error cleanup after review. It keeps the float scalar and ratio
components in PyValue form; Luna separately owns the upstream float ratio
implementation. The first compile exposed a missing explicit borrow, which
was corrected before the passing compile.

`clinic-per-method-value-proposal.md` records the concrete selective Clinic
decorator contract for independent review. The initial method count missed
decorated Clinic blocks; the corrected census has 65 macro-family methods.

Selective Clinic generation is implemented in the separate
`clinic-per-method-value.diff`: decorated methods and clones get independent
flags and converter state; unsupported shapes are rejected; generated value
fragments carry core-build guards. The final W5 Clinic suite ran 478 tests,
all passing. A generated fixture compiled with `Py_BUILD_CORE_MODULE` and
failed without core macros at the expected guard. Sol post-review is pending.
The patch is based on W5 and overlaps the separately integrated `value_core`
Clinic producer change; integration must preserve both mechanisms.

The first method-family layer, `decimal-context-unary.diff`, ports all 15
`DecCtx_UnaryFunc` methods after the three conversion layers and the selective
Clinic generator. The extension compiled and 70 Context tests passed; a direct
comparison of every method with `_pydecimal` covered inline and large ints,
bool, int subclasses, invalid operands and an Inexact trap. Source post-review
found no issue; final test-aware approval is pending. The next proposed family
is the 21 binary Context methods, subject to peer review.

The next layer, `decimal-context-binary.diff`, ports all 21
`DecCtx_BinaryFunc` methods after the unary layer. Its extension built and 70
Context tests passed. Direct comparisons of every method with `_pydecimal`
covered inline and large integers, bool, int subclasses, Decimal objects,
invalid operands on both sides and an Inexact trap. Source post-review is
pending. A separate 12-method predicate, context-free comparison and fma
family has been proposed for independent review.

`decimal-context-rest.diff` ports the other 12 macro-based Context methods
(nine predicates, two context-free comparisons and fma) after the binary
layer. The extension compiled, 70 Context tests passed, and all 12 methods
matched `_pydecimal` in a direct test covering invalid operands in each
position and an Inexact trap. Sol found no source issue; final approval is
pending. The 17 Decimal-instance macro methods remain and their optional
context ownership recipe is under review.

`decimal-instance-macros.diff` ports the final 17 inventoried macro-family
methods after the three Context family layers. The extension compiled and 90
focused Context/usability tests passed. Direct probes covered operands,
omitted/None/explicit contexts, invalid-context C error text and an Inexact
trap. Python Decimal differs from C Decimal for at least one invalid-context
case, so that case was checked against the existing C contract. Sol source
review found no issue; final approval is pending. The four method-family layers
now cover all 65 inventoried macro methods; manual methods and type slots are
still outstanding.

`decimal-context-manual.diff` ports seven manual Context entry points after
the macro-family layers, including the conditional EXTRA_FUNCTIONALITY clone.
The extension compiled and 70 Context tests passed; manual copy/sign/quantum
parity, Decimal identity, and an `_apply` trap probe passed. A separate
`-DEXTRA_FUNCTIONALITY` syntax-only compile passed. Final post-review is
pending. This is not complete Decimal admission: other manual methods,
constructors, numeric slots, the module mark and boxing gates remain.

`decimal-context-strings.diff` ports Context `number_class`, `to_sci_string`
and `to_eng_string` after the manual Context layer. The extension built, 69
focused Context tests passed, and direct comparisons matched `_pydecimal`
across both capitals settings, integers, subclasses and invalid operands.
Post-review is pending. `Context.divmod` and `Context.power` have a separate
proposal awaiting review.

`decimal-context-power.diff` ports Context divmod/power after the string layer.
The extension compiled and 69 Context tests passed; direct probes covered
modulo, operand errors, boundary integers and an Inexact trap. C Decimal's
existing divmod(1, 0) InvalidOperation differs from `_pydecimal`'s
DivisionByZero, so that case was checked separately. Sol found no source issue;
final approval is pending.

`decimal-context-canonical.diff` ports the two Context methods that require a
Decimal argument. The extension compiled; direct probes passed for Decimal
subclass and identity, and exact TypeError on inline primitives and None.
Final post-review is pending. A separate Decimal-instance copy/sign packet has
been proposed for review.

The paired numeric slot layers are `decimal-binary-slots.diff` (six binary
operators), `decimal-unary-slots.diff` (+/-/abs/bool) and
`decimal-power-slots.diff` (divmod/power), each layered on the preceding one.
Each compiled and passed 37 focused tests, plus direct operand/reflection,
subclass, NotImplemented and trap probes. Source post-review belongs to the
coordinator.

`decimal-float-slot.diff` adds the nb_float pair and value-native __complex__.
The extension compiled and 40 focused tests passed. Direct results matched
upstream C bit-for-bit for finite boundaries, subnormal, overflow/underflow,
signed zero, infinities and NaNs; the fork already used a lowercase sNaN
error message where the tested upstream uses uppercase, and this patch keeps
the fork's existing message. Source post-review is pending.

The selective constructor producer/Decimal registration layer and the
round/int/ratio layer were tested and handed to the coordinator. Context
initialization and value getsets require the separately documented
`decimal-context-init-proposal.md`; they are not implemented yet.

### Follow-up: comparison and Context descriptors

- `decimal-richcompare.diff` is a W5 worker-tested layer, pending independent post-review and integration. W5 builds and 37 focused C Decimal tests pass; probes cover float and complex comparisons, FloatOperation traps, reflected integers, and Rational callbacks including errors and mutation. Pre-port W5 invokes the Rational numerator before its denominator. The initial benchmark comparison used an unverified Decimal implementation and is not evidence of an upstream C behavior difference; upstream C source also invokes numerator first.
- `decimal-context-getsets.diff` adds the six ordinary numeric and rounding value getsets. W5 builds and 64 focused Context tests pass, plus assignment, deletion and range probes. This is not full Context closure: custom attribute dispatch, EXTRA_FUNCTIONALITY masks, shared `context_setattrs`, initializer, replace and localcontext still need coherent value paths and merged checks.
- `clinic-selective-initializer.diff` is a draft producer layer with 484 Clinic tests passing. It needs a compiled positive fixture, source post-review, exact-Context registration and constructor/subclass runtime probes before admission.
- `value-call-export.diff` in W5 includes pre-existing unrelated header changes; the coordinator extracts only the three approved dispatcher-leaf exports. The separately frozen `decimal-state-cleanup.diff` removes unused cached long operations and their empty comments.
- `decimal-context-attribute-slots.diff` adds the custom Context value get/set attribute slots after the six-getset layer. The W5 build and 70 focused Context API/input/flags tests pass; post-review and merged validation remain pending. Shared `context_setattrs`, EXTRA_FUNCTIONALITY and initializer work remain open.
