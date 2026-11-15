# Unit A69: concrete source proposal for independent adoption

Proposal only. No runtime edit, generation, build, target, profiling or agent
work is authorized by this document. Source is I selected57 at
`0dcba75bb3df40c32137db453b001f7e8f101de0`,
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`.
Read the live A design69/review69 and actual I sources. This proposal follows
root's FINAL atomic-summary completion decision: generated B0/drain/refresh/
end-periodic/recipient completion, explicitly different from Python child
RETURN. The superseded Python-style/EXTERNAL-root alternative is not proposed.
Final documents and source peer must agree on this choice before edits.

## One feature and exact source ownership

Implement producer, checked readers, adaptive arm, generated consumer and all
owner/error/observer exits together. No proof-only runtime nomination. Feature
is GIL-only, exact Python function/standard vectorcall, fully supplied exact
positional arity of zero through three, including already-present self. Exclude
method expansion, keywords/default binding, varargs, cells/freevars, stores,
calls, loops, generators/coroutines and exception-handler bodies.

Producer files: `Objects/codeobject.c`, `Include/internal/pycore_ir.h`,
`Python/ir.c`. Consumer files: `Python/bytecodes.c`, `Python/specialize.c`,
`Include/internal/pycore_feedback.h`, generated-source contracts in
`Tools/cases_generator/{analyzer.py,ir_generator.py,ir_read_effects.py}`.
Pure-reader declarations may live in a small internal header included at the
normal ceval dependency point; place no frame/runtime includes in the feedback
layout header. Any shared ordinary-reader extraction must preserve its actual
existing guards. No frame, generator, interpreter or executable header grows.
Root alone regenerates affected IDs/contracts/cases/metadata and runs builds.

## Immutable representation: optional 160-byte plan tail

Retain the selected 16-byte `_PyIRCodePlan` header. Allocate a summary only when
the complete proof succeeds; reserve `_PyIR_PLAN_CALL_SUMMARY` flag value4 (the next free bit,
subject to collision assertion) in its existing flags. Tail starts at
`align_up(existing sites/phases/ranges size, 4)`. Existing plans without the bit
retain their exact size. Proposed scalar-only record:

| Component | Exact proposed geometry |
| --- | --- |
| Header |16B: uint8 form, arity, nreads, comparison-op; uint8 comparison-kind, test-path, lhs-path, rhs-path; uint8 result-path, true-path, false-path, flags; uint32 reserved0 |
| Four fixed paths |4×12B: uint32 root-index; uint8 root-kind, depth, step[4]; uint16 reserved0 |
| Four projection sites |4×16B: uint32 word-index/name-index; uint16 feedback-unit; uint8 semantic-kind/flags; uint32 reserved0 |
| One global→class chain |2×16B site records of the same geometry |
| Total |160B, alignment4, no pointer or owned object |

Root kinds are parameter, constant, exact False/True, or the sole global/class
chain. A path has zero through four projection steps, referencing the four
site records. Across the entire body there are at most four actual attribute
projection sites, one global load, one class-value load, one comparison and one
conditional. All indices, signatures, kinds, constant-slot maps and tuple/name
bounds are validated by admission. Constants must be canonical values retained
by the actual code, not addresses stored in the tail.

Add `_PyIR_PlanSummary(plan)` and extend `_PyIR_PlanSize` using the flag and
checked alignment/overflow. A summary-only formerly shared singleton becomes
an owned allocation of at least16+160B; existing owned plans pay160B plus0–3B
padding. No-summary singletons remain immutable/shared. Update BOTH free and
sizeof predicates in `Python/ir.c:561,576`: nsites/nphases alone no longer
identify an owned plan. A summary flag necessarily means owned storage. Static
assert sizes and test empty/summary-only/ordinary-plus-summary cases. Execution
allocation remains in `make_exec`; allocation failure propagates its existing
preparation error, never a hot retry or suppressed exception. Code deallocation
owns the tail through its executable; replacement creates a fresh code/proof.

## Producer and finite execution grammar

`_PyIR_BuildCallSummary(PyCodeObject *co, const semantic_image *image,
SummaryCandidate *out)` is a private admission routine, not a public annotation
decoder. Call it for the actual code in `_PyCode_BuildSemanticPlan`; `co` supplies
the signature, flags, name/constant tuples and constant-slot map missing from
`semantic_image`. The tail is derived during code preparation, never serialized
or accepted from public metadata. Do not grow the common image or code header. Reuse validated
immutable words, CFG edges and home/value transfers from common admission,
including all normal/abrupt exits and UNKNOWN/manual boundaries. Accept only
the following complete reductions after accounting for source aliases/moves:

* Return(path0).
* Predicate(path0, path1, checked comparison), or exact-bool test(path0).
* Select(the same one predicate, path2, path3). Short-circuit and/or reduce to
  this form only when their tested value is proved bool; selected RHS remains
  its actual value. The two result paths may be parameters/constants/fields.

Every reachable return must agree with that reduction and every reached read
must be represented. Unsupported valid code gets no tail. Constructor/replace/
marshal/preparation have the same authority; public images cannot inject a
learned handler or an unchecked tail. The builder follows actual dataflow,
not function names, source positions, apparent bytecode lengths or compiler
provenance. Do not reuse an EMPTY-return certificate as expression authority.

No runtime node program or evaluator loop is permitted. Emit three fixed C
form bodies. A path is a fixed primitive with four explicit guarded steps:
read0, if depth>1 read1, if depth>2 read2, if depth>3 read3. No loop, recursion,
operand stack, scratch value array or variable opcode dispatch is hidden in
that primitive. Predicate evaluates two paths; select evaluates the predicate
and only its chosen result path. Thus a select reaches at most the statically
bounded sites, with branch-local guards. A finite root-kind switch and a
finite read-kind switch remain real native costs, not zero-cost generation.
Borrowed C locals are ordinary transient values, never owning frame homes;
charge their live ranges and stack. Runtime does not generate per-code C.

## Existing feedback as conditional read authority

Structural sites store immutable word/name coordinates and feedback units.
Learned facts remain in the actual callee executable's feedback. Proposed pure
interfaces return `{hit, PyValueBorrow}` by value (no address of handler locals):

* `_PyIR_SummaryProject(callee_exec, site, owner)` shares checked shape/slot
  guards and the canonical raw read; no DUP/OWNER/retirement/stat update.
* `_PyIR_SummaryGlobal(func, callee_exec, site)` uses THAT function's actual
  globals/builtins, exact dictionaries, matching key versions, valid index and
  nonNULL current value. It must not use caller GLOBALS().
* `_PyIR_SummaryClass(callee_exec, site, owner)` shares current class-version,
  required metaclass-version and guarded-value checks. Descriptor/custom
  access cases not proved by existing class-value specialization decline.

Use `callee_exec->entries/feedback`, never caller IR_FB or `_PyFrame_Exec(frame)`.
For a MONO shape/slot site require its current accepted handler/capability and
actual type/version/layout/nonNULL guards. For shape POLY reuse the existing
`shape_read_offset` nomination semantics with the callee block; its bounded
four-row search is a read PIC cost, not a summary instruction stream. No
historical non-shape row is treated as an executable shape fact. COLD/MEGA,
unsupported handlers, shared entry0 or absent executable/read facts MISS.
Summary evaluation never learns/allocates feedback or calls `_PyCode_GetExec`.
Ordinary execution warms missing read facts.

Int comparison uses actual canonical inline-int operands and the immutable
comparison mask, matching I `_COMPARE_OP_INT` without creating owned operand
homes. Exact bool test accepts only actual bool. Same-exact-type object EQ/NE
may reduce to identity only if BOTH current `tp_richcompare` and value-richcompare
fields are the base-object implementations (`_Py_BaseObject_RichCompare`,
`_PyObject_ValueRichCompare`). No generic comparison call is made. For unequal
objects the ordinary two NotImplemented results lead to identity fallback;
same exact type excludes reflected-subclass precedence. The declarations are
`pycore_typeobject.h:153` and `pycore_value_ops.h:177`; use those normal internal
dependencies, not an invented comparison API. Other operators/types
MISS. Enum names confer no authority. Comp coverage remains conditional on
those actual guards; generic equality feedback alone is insufficient.

## CALL arm: same 16-byte payload, complete adaptive lifecycle

Keep `_PyFbCall`32B total (16B header+16B payload). Preserve func_version and
callable_type_version. Add a six-byte union member `summary` with three uint16:
schema1, packed form/arity, reserved0. Allocate unused binding_flags bit0x80 for
SUMMARY after a collision assertion. Its only compatible other bit is SELF;
DIRECT/KEYWORDS/VARARGS/default plans are exclusive. No pointer, owner or plan
offset is cached: obtain the tail from the current function/code/executable.

Nomination in the existing Python specializer requires its normal provider/
version/arity checks, existing executable with complete tail, and learned
supported read facts. Select the summary family only then; ordinary exact CALL
and unrelated general/native/logging paths gain no summary-probe branch. A
same-code function may share a version but use other globals; runtime reads
the current function environment. Fully supplied calls never use defaults.
Closure/keyword/default-binding forms are excluded structurally.

Initialization/reset: make_exec's zeroing and fb_begin_ex clear the whole
payload, including the arm. Publish fully populated arm before specialize()
publishes its handler. Runtime requires schema/form/arity/flag congruence and
current function version/code/tail; every failure is a no-effect MISS.
Deopt/instrumentation do not preserve authority through a bare handler name.
CALL history in `fb_row_of` saves version/type, not this binding arm, and
`fb_record` saves the handler. Actual I has NO historical CALL-row-to-live-handler
restoration reader. `_PyFbPoly`'s execution readers are attribute/shape paths
(`fb_shape_read_set`/`fb_saw_other_shape` and `shape_read_offset`), with a
separate read-only test export. Leave CALL history inert: neither its handler
nor version/type reconstructs summary authority. On revisiting a callable the
current specializer must construct a fresh current arm. Reset/re-specialization
and summary→other callable→summary tests prove that lifecycle; do not invent a
history-restoration feature. Do not enlarge `_PyFbPayload` or reinterpret a
positional arm as a summary.

## Generated consumer: complete atomic CALL, not native ownership engine

Use new CALL family member(s) and a checked source marker
`IR_TRY_CALL_SUMMARY()` before consuming any input. Source validation pins the
normal CALL window/role/format, exact Python/provider/arity prefix and the
entire fixed expression/read contract. Root assigns actual handler IDs only
through generation. Define exactly seven source-declared bounded handler shapes:
no-self with0,1,2,3 explicit arguments and present-self with0,1,2 explicit
arguments. Each shape declares its actual scalar input owners and parsed POPs;
there is no runtime owner array or arity drain loop. The existing CALL role
mechanism supplies only the self-hole ABI, not these arity expansions. It may
reuse those role definitions, but all seven arity/source bodies and their
contracts must exist explicitly. Input holes are never read or vacated as
inputs; legitimate output publication may initialize a hole if actual A names
it. Generation refuses a shape whose source owners, guards or exits cannot be
accounted by the parser. An unparsed emitter string acquires no capability by
naming this marker.

Before body reads, require GIL, standard function value/vectorcall provider,
valid arm and exact supplied arity, current proof association, ordinary stack/
recursion admission, and clear PEP523/trace/profile/caller+callee monitoring/
DTrace/debug LLTRACE conditions. The first actual executable word must be the
ordinary RESUME that I IR_SKIP_RESUME would skip; compare the FULL eval_breaker
with callee instrumentation version and decline on mismatch. Exclude Emscripten
before touching its signal clock and FT entirely. No completion poll substitutes
for that pre-body gate. If the debug/tool gate itself can call Python, perform
it outside speculation and reload everything, or conservatively decline.

MISS changes no input owner/home/PC/exception or prepared-call state; use the
existing generated family fallback. No method expansion is attempted here.
The original normal CALL executes at most once. At the successful expression's
end acquire ONE canonical selected result using the ordinary home DUP; then
stop all function/code/plan/feedback/borrowed-path reads. This is the commit:
no fallback or callee replay remains after it.

The committed source body uses the existing generated CALL result publication,
parsed input POP/drain, periodic and destination/discard contracts, as seen in
I CALL_STR_1/CALL_BUILTIN families. It does not call `call_transaction_native`
or invoke an outlined transaction on an already-produced result. Before any
close, capture every original function/parameter owner in the shape's explicit
owning C locals, vacate exactly those original input homes, and publish the
canonical result at B0. Set the parent's logical `return_slot=A` and
`return_pc=next_instr` for supported active-CALL setters. No stale input address
survives. Close displaced callable first, then explicit arguments in reverse
order, then present self, matching the chosen generated atomic CALL schedule.
Each POP consumes its captured owner once; no later callback can make the drain
reread a replacement home. Source ownership accounts for these C locals across
every exit and any required spills count as native cost.

### Exact summary-only callback and redirect continuation

The existing `emit_ordered_retirement` reaches `retire_published_values` →
`finish_value_bridges_ex` → `value_operation_refresh`, whose final PC reset is
unsafe after an applied redirect. Do not call that PC-restoring chain and then
classify. Add a summary-only, single-owner cold close/validation continuation
in `Python/ceval.c`, emitted only by the parsed summary retirement sites. It
receives one owner by value and closes it once. Reuse existing close/validation
operations; it is not an argument-drain loop, owner array, new transition root,
native ownership engine or universal operation hook. Fast non-reentering closes
retain the existing `_PyValue_MayDealloc`/close shape and do not load frame
aliases or redirect generations. Snapshot/classification occurs only around
an actual may-reenter close, pending service, or recipient callback.

Immediately before such a callback, snapshot the actual parent PC, its
`pending_redirect` and `frame_obj->f_lineno_generation` (NULL alias means0).
Admission/commit requires pending0. After the callback validate the actual
current frame/executable/word bounds using the existing checks, without writing
`instr_ptr`. Alias publication with generation0 alone is not a redirect. With
entry pending0, classify the summary's reached control as follows:

| Reached state | Required continuation |
| --- | --- |
| NORMAL: PC still summary CALL, no redirect-generation change, pending0 | Continue the captured-owner drain; no descriptor or borrowed value is read. |
| PENDING: pending1 at the actual CALL | Preserve actual `return_pc`; B0 still owns the summary result. Finish only the captured owners. |
| APPLIED: PC differs from CALL, or generation changed and pending0, or pre-callback pending1 became pending0 (including same-PC redirect) | Sticky supersession of this CALL. Never inspect, clear or move its old B0 or any old input index again. Finish captured owners, then dispatch actual landing; skip the superseded CALL's poll and recipient. |

For each later callback compare both PC and generation, including NULL→published
alias. A pre-callback pending1→post-callback pending0 transition is APPLIED even
without another generation increment. PENDING can become APPLIED, and APPLIED
never reverts to authority over old homes. A same-PC redirect with unchanged raw value bits still owns a new
landing; value equality/identity is never an ABA test. All captured owners are
retired even when the landing already consumed/replaced B0. If an exception is
already pending during remaining owner closes, preserve it using the existing
error-preserving close convention. Revalidation failure takes the existing
bridge SystemError/error path after those C owners drain; there is no retry.

After the drain, NORMAL/PENDING perform `check_periodics_at_end` at the actual
CALL with its LOAD_SPECIAL suppression. Keep the existing event test and
no-callback fast case; generation snapshots belong only to the may-service
cold branch. Classify again immediately after service, before any generic
refresh. If a callback applied the redirect, do not run the old recipient.
APPLIED skips this superseded poll: it is a different semantic landing, and a
subsequent summary must pass the full pre-body eval-breaker guard again.

If end service returns<0, the error wins over successful completion:

* NORMAL: retain B0 as the error root and enter
  `PROCESS_ERROR_AT(RAISE,0,summary_CALL)`.
* PENDING: clear `pending_redirect` before that same error ingress, with no
  intervening callback; retain CALL PC and B0. Abrupt child unwinding similarly
  clears the parent's pending flag (`ceval.c:2386–2388`). Do not complete a
  successful debugger return on failure.
* APPLIED: do not read old B0. Enter `PROCESS_ERROR_AT(RAISE,0,frame->instr_ptr)`
  using the current landing's initialized homes. Checkpoint0 already supports
  that error cleanup; do not stamp the old CALL's home certificate onto it.

For successful NORMAL, reload actual parent word/homes and use existing
B0→destination/discard plus displaced-local operations. A=B is already rooted;
no fictitious move saving. For successful PENDING, TAKE B0 to one C owner and
NULL its old home, then use existing `_PyFrame_SetReturnValue` solely as the
pending-redirect adapter: its `_CompleteRedirect` clears current scratch and
uses actual `return_pc`; a supported pending CALL has IR_NO_DST, so no captured
A write occurs. This cold adapter does not change the normal atomic policy to
Python-child completion. Classify any callbacks inside the adapter/discard/
displaced-local close before a PC-restoring refresh. Recipient publication is
one-way: once the result moved or was consumed, never restore B0 or replay it.
After a recipient has consumed/moved the result, any newly PENDING redirect
uses `_PyFrame_CompleteRedirect` directly: there is no remaining result to TAKE
or publish. Classify its closing callbacks again. NORMAL then dispatches the
original next instruction; APPLIED dispatches actual `frame->instr_ptr`. Error
processing uses the actual reached landing if validation/service failed. No result,
original owner, or callback-selected replacement owner is consumed twice.

This is an explicitly ATOMIC SUMMARY completion policy. Python child RETURN
normally retires a real child then uses return_to_caller/_PyFrame_SetReturnValue;
it has no native-call end-periodic stage. This unit instead deliberately uses
the generated atomic-CALL B0/periodic/folded-recipient recipe under the accepted
bounded-lifetime contract. Do not describe it as exact Python child release/
poll timing, and do not mix in the superseded EXTERNAL-root Python recipe.

## Required implementation and source-proof gates

1. **Callback-selected control is part of this unit.** Implement the exact
   summary-only cold continuation above before source GO. Mutation controls
   must cover pending, immediate and same-PC redirects, redirect during service,
   poll failure with each reached state, and callbacks after recipient commit.
   Raw B0 macros or a refresh after PC-reset do not supply this contract.
2. **Pure primitive extraction is a semantic change, not an alias.** Ordinary
   owning attribute handlers DUP/retire; they cannot be called as borrowed
   readers. Share actual guard/read definitions, callee feedback selection and
   nullable/descriptor/metaclass/canonical bits for each accepted handler.
   Mutation/UNKNOWN controls refuse before effects. The two actual EQ
   declarations above provide dependencies, not semantic admission by name.
3. **The family proof follows emitted source actions.** Pin all seven explicit
   arity/role shapes, whole read/owner/effect contracts, normal B0 result and
   post-drain error roots. Mutating source guard/primitive/capture/POP/
   completion must refuse certification. No per-name exemption or blanket
   CALL_DRAIN stamp suffices.

## Acceptance and handoff

Independent source peer covers all three concerns above plus plan allocation/
free/sizeof, fresh-arm/reset lifecycle and the atomic remote display contract:
noncalling body exposes the real parent at CALL with valid original homes;
there is no fabricated child or new remote layout. Focused real-object tests
cover all forms/arms and non-bool RHS, mutations/invalidation, missing/custom
read/compare exactly once, globals/code swaps, denied signatures, result/input
alias and mortality, all recipients/redirect/error/pending/recursion/tools and
FT denial. Reuse existing tests where they prove the same boundary.

Root alone qualifies actual projection/select/both-arm hits, unchanged comp/
Delta hits if admitted, zero child reservation/body dispatch/clear on hits,
complete native costs including misses/nonparticipating logging, both builds
and the unchanged protected screen. Target budgets remain alarms from design69,
not a correctness substitute or a +1 instruction veto. If the bounded grammar
or required consumer closure fails, stop this unit and return evidence to
grouped review; no larger summary VM or second runtime lane is proposed.
