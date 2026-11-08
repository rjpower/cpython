# P3: preserve guarded Python entry through transaction preparation

2026-11-08. Bounded implementation proposal for coordinator peer review.
Runtime base is the P1/P2 checkpoint
`0539e0c1ff96ccf2a1d95e96f99c9e499b20552a`. This proposal adds no runtime edits
or measurements.

## Work removed and work retained

The reviewed route diagnostic records ten accepted outer overlap entries and
ten inner direct exact-thread entries per logging loop. Every overlap entry
fills kwargs and empty varargs; no consuming-binder fallback, native route,
decline or instrumented call appears in its loop delta. P3 therefore targets
the outer entry's repeated **pre-escape** callable/provider discovery, not
argument-copying in a binder that the hot path never uses.

`CALL_PY_GENERAL` already checks PEP 523, exact function/version and recursion,
then invokes generic vector transaction entry. `call_transaction_invoke` still
considers method expansion and rediscovers exact function, provider and hook
facts before `call_transaction_push_python`. The guarded unbound-function
route has no intervening escape. A method's explicit self argument in the
window is allowed; a PyMethod wrapper requiring expansion is excluded.

This cannot remove the fresh observable kwargs dictionary, its eventual
destruction, frame/code/function owners, defaults, empty varargs, recursion,
frame publication or post-allocation revalidation. The inner exact call is
already direct and does not benefit. M1 and M2b address different work inside
that inner path. Their source coverage is not an additive gain estimate.

## Shared transaction initialization and commit

Factor only the existing `_PyCallTransaction` designated initializer into one
private static-inline initializer returning that record by value. It takes
the current scalar inputs (thread/frame, base/count/destination, argument
count, kind and flags), captures `frame->instr_ptr`, and initializes all fields
exactly as the present `call_transaction_run` does. It allocates nothing,
publishes nothing, consumes no owner and cannot fail or run Python. Both
ordinary run and the new private guarded vector run use this initializer.
Do not duplicate its field assignments in two transaction bodies.

The existing generic `call_transaction_run` continues through
`call_transaction_invoke`, unchanged in routing and preparation. The guarded
out-of-line vector entry uses the shared initializer with VECTOR kind,
`count = argument_count + 2`, and the established prepared fact. It then calls
the **same** `call_transaction_push_python` as generic entry, which retains
both `call_transaction_overlap_python` and the consuming fallback. If that
returns positive decline, invoke the existing `call_transaction_native` on
the same live record. Do not replay method or monitoring preparation after a
decline. Preserve the existing `{status, child}` return convention and null
child on errors. A two-line shared result packer is optional; another general
call ABI or callback-based transaction framework is unnecessary.

Suggested private entry name/signature:

```c
static Py_NO_INLINE _PyCallResult
call_transaction_run_guarded_python_vector(
    PyThreadState *tstate, _PyInterpreterFrame *frame,
    int base, int destination, int argument_count, int flags);
```

No new runtime flag selects generic versus guarded entry. The generated
successful invoke names this entry directly, while ordinary invokes name the
ordinary entry. Flags retain their existing meaning; prepared is set in the
guarded record because its callable cannot be a method wrapper. Debug asserts
may reject instrumented/monitored/effectful incoming state; do not add those
checks to every unrelated call. Under `Py_GIL_DISABLED`, the private entry
delegates to ordinary vector entry at compile time until a concurrent proof
is separately reviewed.

## Entry proof and invalidation

The proof is local to the ordinary `CALL_PY_GENERAL` successful guard prefix:

1. `_CHECK_PEP_523` succeeds.
2. `_CHECK_FUNCTION_VERSION` proves exact PyFunction and the cached valid
   version. `specialize_py_call` only installs this form for
   `_PyFunction_Vectorcall`. `PyFunction_SetVectorcall` clears the version
   before replacing that provider, so a matching valid version carries the
   provider proof under the held GIL.
3. `_CHECK_RECURSION_REMAINING` succeeds. No allocation, release, Python call,
   poll, frame switch, instrumentation callback or input mutation follows
   these checks before the guarded invocation.

Validate the actual prefix properties and shapes in the generator, rather
than trusting the specialized opcode name alone. The version guard's
deoptimization exit remains before effects/input consumption. Require a valid
nonzero version, unchanged evaluated owning window, ordinary VECTOR kind,
and uninstrumented invocation. Unknown new prefix calls invalidate the proof.
Use an explicit checked invoke marker in a dedicated general-Python invoke
uop selected only by this macro; the bound-method and instrumented macros keep
their ordinary invocation markers.

The allocating kwargs path **ends** this proof. Do not remove any final guard
in `call_transaction_overlap_python`: callable identity/type, function/code,
vectorcall, PEP 523, executable, capacity, defaults/arity and self-offset
mapping must be checked after allocation. Defaults/code can change during
allocation or cleanup. The shared consuming fallback also retains its own
post-allocation provider choice. The final transfer remains once-only and
nonescaping until the clearable child is current. Both fast and fallback
paths preserve existing result publication, monitoring, error and owner order.

## Per-invocation override: generic fallback must stay generic

Let `IREmitter.call_completion` take an optional **local invocation** run-helper
override; its default remains `self.call_run`, the generic helper selected
from call kind. The checked guarded invoke marker passes the private helper
for its successful invocation only. It must not mutate `self.call_run`.

The emitter's later `call_transaction_fallback_*` label always calls
`call_completion()` without an override. A failed guard following effects
may reach that label without completing the successful proof; inheriting the
private helper there would be invalid even if today's unbound prefix has no
such escape. Generic deoptimization, bound-method expansion and monitoring
retain their current entries and state ledger.

Register the new invoke marker with the same existing transaction effects,
frame/call shape and ownership rules as IR_INVOKE_CALL. Replace the generator's
several literal marker-name detections with one invocation-marker set/predicate
so format construction, uses-call-transaction, monitoring and emitter behavior
all see the new marker. This is an update of existing consumers, not a fallback
ABI. Generated code must visibly name the private entry on the successful
guarded edge and ordinary vector entry at its generated fallback label.

## Validation and rejection

Generator tests must reject a missing/changed function-version or PEP guard,
an escaping prefix insertion, instrumented use and bound-wrapper use. Test
that a guarded invocation's override never leaks to the fallback label or a
later handler generated by the same emitter. These distinguish the proof
boundary from a handler-name optimization.

Runtime coverage includes kwargs/varargs/defaults, exact/general and bound
method routes, code/default replacement during allocating preparation,
PyFunction_SetVectorcall through its public setter, custom evaluators,
monitoring activation/errors, generators, keyword/EX fallbacks, recursion and
allocation failure. Caller window owners stay intact on every fallible prepare
edge. Post-allocation guards and final commit are shared, not copied into a
second implementation for these tests to diverge.

Fixed-GCC review must identify the finite pre-escape work removed: method-wrapper
test/expansion entry, repeated function/provider/hook discovery, and associated
transaction routing or spills. Retain the post-kwargs checks. Verify the common
initializer inlines and introduces no record copies or indirect invocation.
The source should not claim those checks disappeared merely because a helper
was renamed. Compare target/control effects against the same retained source/
ELF control; elapsed ratios from different time/control campaigns cannot admit
or reject layout changes on their own.

If route diagnostics are repeated, count the private entry separately and
reconcile it with ordinary invoke entries; moving accepted calls out of
call_transaction_invoke must not appear to remove semantic calls. Shared
overlap/child counts, fresh kwargs and frame totals remain the same expected
workload denominator. Counter instrumentation stays outside release timing.

Reject P3 if the private entry merely moves checks/spills, duplicates commit
logic, leaks its proof into generic fallback, weakens final revalidation, or
cannot show repeatable useful progress on its actual accepted route. Keep
M1/M2b as independent structural candidates; do not enlarge P3 into a permanent
call-plan representation to avoid a negative result.
