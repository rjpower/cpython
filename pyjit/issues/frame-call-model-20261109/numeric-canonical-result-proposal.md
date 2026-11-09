# Checked canonical numeric results

Proposal only, following source20 and the separately admitted local-first store
experiment. No source changes, native jobs or measured benefit in this proposal.

## Positive contract

A final scalar store can omit `_PyValue_Publish` when its owned result is proved
to have canonical bits on every successful path and that same owner reaches the
store unchanged. Establish the fact from parsed constructor assignments and
checked uop composition, never from the opcode's name, `pure`, or the presence of
a constructor token somewhere in its body.

Use a small `CanonicalResultContract` alongside the existing checked return and
result-publication contracts: producer Uop identity, actual scalar output
identity and checked constructor source. This is distinct from the existing
"temporary is published before input retirement" phase. Canonical construction
does not grant early publication, eliminate input retirement or establish that a
previously published cell survived arbitrary callbacks.

The first useful implementation should cover direct unconditional assignment
from `PyValue_FromDouble(expression)` to the first scalar result. This covers
multiply/add/subtract/divide float implementations by their source structure,
including mixed inline numeric operands admitted by their existing guards.
A synthetic differently named producer with the same valid structure must also
qualify. A familiar float handler with an invalid structure must not.

## Trusted constructor boundary

`pycore_value.h` PyValue_FromDouble returns inline non-NaN bits or delegates NaN
to `_PyValue_BoxDouble`; the latter asserts NaN and creates an exact boxed float.
NULL allocation failure follows the existing error branch. Signed zero and
infinities remain inline; NaN remains a pointer. Thus the public wrapper's
successful result is canonical without another exact-type/number inspection.

PyValue_FromInt64 also has an unconditional canonical-result API: inline if it
fits, otherwise `_PyValue_BoxInt64`, whose precondition is outside the inline
range. Its direct use can qualify under the same structural contract once
covered by tests. Do **not** grant the fact to every direct `_PyValue_BoxInt64`
call: calling it with a fitting value violates its canonicality precondition in
release builds. Similarly, `_PyValue_FromIntScaled` requires aligned scaled bits.
Current add/subtract-int producers have conditional scaled-overflow paths and
use these narrower APIs. Their arithmetic/guard preconditions need a separately
checked bounded source pattern before their two assignment branches qualify.
They remain on ordinary publication in the initial direct-constructor increment.
Direct `_PyValue_BoxDouble` likewise does not inherit its wrapper's unconditional
contract. This keeps the leaf trust list about constructor APIs, not instructions.

Constructor implementations are the trusted C boundary, like existing canonical
home and return primitives. Record their source rationale and test NaN/boxing
boundaries; a Python generator cannot prove arbitrary C helper implementations.

## Constructor binding is part of the proof

The parser preserves C preprocessor statements (CMACRO_IF/ELSE/ENDIF/OTHER);
it does not provide C name resolution. Therefore a constructor token is not
proof that the audited global helper was called. For the initial bounded rule,
reject every preprocessor statement in the candidate's full uop chain. Require
the constructor identifier's only occurrence in that chain to be the callee of
the exact accepted assignment. Check stack inputs, outputs, caches and parsed
local declarations for the same identifier too. This rejects local function
pointers, constructor-named parameters, declarations, rebindings and local macro
replacement without needing a C symbol resolver. No aliases or indirect calls
qualify. Apply corresponding binding exclusion to helper names relied on by the
inline-input/nonreentrant-retirement proof.

Global helper definitions and header/build preprocessing remain an explicit
trusted source boundary, just as for canonical return/home helpers. The current
included definitions must be audited and must not redefine an approved helper
at an external macro site. The generator does not claim to prove arbitrary C
linkage or malicious toolchain overrides. This boundary is narrower than
assuming that every same-spelled callee in a parsed body resolves globally.

## Fail-closed source recognizer

Reuse parsed BlockStmt/IfStmt/SimpleStmt traversal and the existing external_io,
first_new_output and alias checks. Initial accepted grammar is deliberately
narrow:

1. Exactly one scalar ROLE_DST output, non-window, non-array, not aliased with
   any input. Identify the actual external result and its producer through stack
   composition, not an identifier spelling shared by multiple uops.
2. Exactly one unconditional top-level assignment of that output in its
   producer, with RHS exactly one approved constructor call. No cast, comma,
   conditional operator, assignment expression or result reference in arguments.
   Reject nested constructor calls or argument expressions with address escape,
   mutation or opaque calls in the first implementation; arithmetic on existing
   scalar C locals is sufficient for the four float producers.
3. Before assignment, retain the existing guards/deopt/error behavior. After
   assignment, permit only the source forms actually needed: a flat
   `if (PyValue_IsNull(result)) { ERROR_NO_POP(); }`, input-to-suffix output
   assignments, INPUTS_DEAD and checked result-preserving operations. Result
   references are allowed only in the exact NULL error test; any reassignment,
   address-taking, consumption, unknown use or unsupported control flow rejects
   the fact. A NULL result never reaches the canonical final store.
4. Follow the external result's same stack identity through every remaining
   uop. Reject overwrite, changed output ordering, result/input aliasing,
   frame-switch, callback, side exit after construction, unknown transport or
   direct access to the destination home. The actual numeric input-retirement
   suffix must match its checked source contract, not just a `_POP_TOP_*` name.
   Existing float number retirement asserts inline float/int and closes those
   owners; no user code can run there. Check the complete body and input/output
   shape, with fail-closed mutation coverage.
5. A failed recognition emits ordinary publication. Do not partially record a
   fact and silently use it after a later unsupported suffix. An explicit future
   declaration, if introduced, must instead produce an analysis error when its
   asserted proof fails.

The full-uop identity check is necessary: proving `res` canonical in one uop
cannot justify a later different output that happens to use the same C name.
The existing canonical_return_input exact-chain checker provides a useful
fail-closed precedent; its proof stays independent and unchanged.

## One final destination store

Factor the current local-first IR_STORE_DST bodies once into an internal macro
with a value-expression parameter. Ordinary use supplies
`_PyValue_Publish(&(V))`; checked canonical use supplies `(V)`. The value
expression is evaluated exactly once only in a storing branch. The NoDst branch
closes V without evaluating either expression or forming a slot address.

Both front ends share local publication-before-old-owner retirement, temporary
NULL-or-same-owner assertions and debug canonical-home checks. A local old owner
may run a finalizer through the existing displacement/retirement protocol;
changing how result bits are obtained must not move that callback. Preserve
input/destination alias checks and the established `ir_displaced` handling.
The C-owned constructor result remains owned until that same existing transfer.

IRStack.store selects the canonical front end only for the matched final result
identity. Do not make the macro public as a general unchecked promise from any
handler. No new handler, value representation or per-dispatch branch is needed.
Temporary-specialized stores may use their existing canonical direct-transfer
primitive only under this same fact. Initial scope excludes leaf/fused stores
unless their actual scalar result flow is independently checked.

This proposal does not fix every double publication. A result published before
an input callback needs the existing post-retirement identity/refresh contract;
canonical construction alone cannot prove the destination still contains it.
Do not remove the final publication of UNARY_NEGATIVE on that basis.

## Expected work and limits

For an admitted final store, remove the redundant exact-long/float
classification and possible conversion work inside _PyValue_Publish. Retain
local/NoDst/temp classification, canonical debug assertions and all ownership
work. Benefit depends on the actual generated route and dynamic frequency;
whole-handler size or warmed static destination counts are not savings counts.
The coordinator will inspect compiled multiply/add/subtract paths and measure
nbody plus the broad set after correctness. Initial four-float source coverage
is useful without pretending that conditional integer producers are proved.

## Test and review ladder

Host generator positives: current four float chains, renamed synthetic producer,
direct FromInt64, local/temp/NoDst destinations and variable result names. Assert
the final store changes while input retirement/displacement/error text remains.

Mutation negatives: constructor merely mentioned in a guard/comment/dead branch;
one conditional arm unknown; result rewritten or addressed; result consumed;
output alias/array/window; NULL error test removed; suffix result changed or
addressed; suffix helper body changed under an unchanged name; output permutation;
unknown callback; side exit after construction; a synthetic local function
pointer or input named PyValue_FromDouble, reassignment of that callee, and a
preprocessor override/conditional inside the chain. Direct narrow BoxInt64/IntScaled
calls must not acquire the unconditional constructor fact.

Coordinator native gates: finite/mixed float arithmetic, signed zero, infinities,
NaN construction and boxed NaN input fallback; int inline boundaries and boxed
out-of-range fallback; all three destinations; aliasing and displaced local
last-owner reentry; allocation failures where existing test support applies.
Use the existing numeric/IR tests plus focused new cases, then debug/release
broader checks and paired measurement. B independent review precedes runtime
implementation. No performance admission from source inspection alone.

## Implementation sketch awaiting peer admission

1. In ir_generator.py, add a small immutable CanonicalResultContract and a
   recognizer returning the checked producer/result identity or None. First
   validate global-binding exclusions, exact scalar format/aliases and the
   constructor assignment/error grammar; then validate the entire suffix using
   actual parsed bodies and composed stack identities. Keep the constructor API
   list and the permitted AST grammar explicit and small. Unsupported source
   falls back to the ordinary path, including any failed suffix check.
2. Thread the optional fact into IRStack's final store decision, independently
   of ResultPublicationContract and canonical_return_input. It must not change
   the timing of stack.flush or input retirement. Do not infer eligibility from
   execution aliases or output names alone.
3. In ceval_macros.h, factor only the existing final-store body into an internal
   IR_STORE_DST_IMPL(F,V,VALUE) and two front ends. Canonical supplies (V), ordinary
   supplies _PyValue_Publish(&(V)). Branch-local evaluation preserves discard
   and local overwrite behavior. Retain existing assertions and retirement.
4. Add host generator positives and all mutation negatives before broadening
   the source rule. Generate to memory to inspect the four current float bodies
   and confirm that destination/retirement/error order is unchanged. Root owns
   generated files, native checks, disassembly and measurement.

No bytecodes.c change is expected for the direct-constructor phase. Conditional
integer proof and callback-crossing published-home reuse remain separate work.

## Implemented bounded source, pending native review

The generator now records CanonicalNumericResult only after exact parsed
producer/body and retirement-body checks. Actual float operands, result and
suffix aliases must have distinct scalar identities; each of the two suffixes
consumes the top non-result position, leaving precisely the result at final
ROLE_DST. The matcher uses no instruction-name whitelist. It accepts the current
four float chains and a renamed version, plus a zero-input decimal-constant
FromInt64 source form. Broader FromInt64 expressions are not enabled.

Only Uop and inert unnamed Skip parts qualify; Flush and future part kinds fail
closed. Named uop caches are excluded entirely, along with trusted-helper-named
stack bindings. Exact body checks exclude declarations/rebindings and macros;
CMACRO tokens also receive an explicit rejection. Box/Scaled helpers, conditional
integer producers, unknown result/suffix effects and unsupported shapes retain
ordinary publication.

Final scalar stores share IR_STORE_DST_IMPL; the canonical front end substitutes
only (V) for the ordinary _PyValue_Publish expression. This does not change
pre-retirement flush timing or input/displaced-local ownership. No bytecodes,
metadata, opcode number or value-format changes.

Four focused host generator methods PASS in17.032s: numeric constructor proof,
mutation negatives, direct-int64 boundary and existing canonical-return
provenance mutations. Test loading uses uv host Python with Lib appended after
startup, not PYTHONPATH before codec initialization. Source AST/diff checks pass.
B final source peer and coordinator regeneration/native checks/measurement are
pending. No benefit is claimed from this source result.

B actual source peer approved the final bounded proof and shared macro.
Coordinator regeneration23 changes exactly four final float-store calls;
A generated-source review confirms unchanged guard/deopt/zero-divisor/error,
input retirement and displaced-local suffixes. Full host test_generated_cases
module: 87 PASS in37.871s. Native23 build/focus and paired measurements remain
coordinator-owned and pending; no worker native result claimed.
