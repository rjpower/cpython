# Reuse small fixed-keyword binding plans

Proposal only, against frozen runtime `4d666e1b482`. No runtime source edits,
builds, tests or commits are authorized by this document. Golden publication
and the coordinator's implementation approval precede implementation.

## Problem and useful increment

`specialize_py_call_kw()` (`Python/specialize.c:2230`) records only a function
version. `_PY_FRAME_KW` invokes the ordinary transaction; the overlap admission
at `Python/ceval.c:3259` accepts VECTOR only. Every fixed keyword call therefore
reaches the fallback at 3344: duplicate argument owners, initialize a separate
child, search names in `initialize_locals()`, then detach and close parent
inputs. The fixed-keyword microcase's 1.985× raw upstream instruction ratio
justifies a bounded experiment; it is not a promised speedup budget.

Extend the existing scalar guarded Python binding plan and consuming frame
transfer. Admit one to three explicit keywords, any positional prefix fitting
the signature, permutation, omitted positional defaults, and keyword-only
parameters when all are explicitly supplied. The limit comes from the existing
six-byte positional-plan payload: three `uint16_t` local destinations fit
without growing every CALL feedback entry. It does not depend on function arity
or benchmark parameter names. Larger keyword sets use the existing provider.

Initial exclusions: `*args`, `**kwargs`, omitted keyword-only defaults, bound
method wrappers, generator/coroutine functions, non-thread-owned callers,
free-threading, custom vectorcall, PEP 523, and instrumented calls. Each keeps
its existing path. An ordinary function with an explicit self operand may be
admitted by treating self as the first positional argument. Defaults need no
cached owner: the hit reads the guarded function's current defaults.

## Scalar plan and guards

Use a named union in `_PyFbCall` for the current three positional fields and
`keyword_dest[3]`; update existing positional users to the named member. Keep
`binding_flags` outside it and encode keyword count and self presence there.
The instruction fixes the remaining argument count; the self bit prevents a
changed implicit prefix from exposing a previously covered required slot.
Preserve the entry's size and scalar-only lifetime. The specializer clears flags before
considering either plan. There is no cached tuple, borrowed string pointer,
heap plan, per-frame flag or new value format.

Pass current kwnames through `_Py_Specialize_CallKw` and its declaration and DSL
caller. The plan builder performs only pointer comparisons against the guarded
code's `co_localsplusnames`; it accepts an exact tuple of exact Unicode names
whose entries are identical to parameter-name objects. Equal but nonidentical
names fall back. It checks:

- optimized ordinary function with the standard provider and a valid version;
- keyword count 1–3 and every destination fits `uint16_t`;
- positional count (including self) does not exceed `co_argcount`;
- each keyword maps at or beyond `co_posonlyargcount`, outside the positional
  prefix, to a distinct parameter slot;
- all required positional and all keyword-only parameters are supplied;
  uncovered positional slots belong to the trailing defaults range;
- no variadics; default count is within the same bounded scalar arithmetic
  accepted by the existing positional plan (overlong legal tuples decline).

No equality/hash/descriptor callback is needed in plan construction. Never
reject a Python call merely because plan construction declines it.

On a hit, retain the checked PEP 523/function-version/recursion prefix. The shared
binder also checks plan kind/count, thread-owned frame, standard vectorcall,
available executable, stack space, cached self presence and tuple shape. The
fixed instruction argument count and cached keyword count then recover the
same positional count used during construction. For each keyword, compare the
current name to the guarded code's local name at its
cached destination. This proves the current permutation without caching or
retaining kwnames. Changed kwnames or changed callable therefore misses before
effects. Function mutation (`__code__`, `__defaults__`, `__kwdefaults__`, closure)
invalidates the existing version guard (`Objects/funcobject.c:276–300`); no new
invalidation system is needed. In-place kwdefaults mutation cannot invalidate
this plan's result because every keyword-only parameter is explicitly supplied.

## One frame-transfer convention

Extend `call_bind_known_python()` with the call kind, rather than adding a
second public binder. Preserve its existing positional branch. The keyword
branch uses the same initialized call window, `_PyFrame_PushOverlapping()`,
return slot/PC publication and child-current handoff:

1. Complete every guard before detaching any owner. A miss returns status 1
   with the original window untouched; the existing keyword transaction runs.
2. Move at most three keyword owners into a fixed local array and NULL their
   source homes. Detach the kwnames owner into one local and NULL its home.
   Keep the positional prefix in place. No allocation, release, callback or
   failure is permitted from this point until the child is current.
3. Move the callable out of its home. Push the overlapping child at the first
   positional cell with `nargs = positional_count`. This respects the existing
   helper's assertion that every later caller cell is NULL. It also prevents
   frame constant-slot initialization from overwriting an unremoved kwnames
   owner or keyword owner.
4. Move saved keyword owners into their cached child locals. Fill NULL
   positional holes from current `func_defaults` with `PyValue_XDUP`; eligibility
   has already proved each hole has a default. All keyword-only locals are
   already supplied. Argument/default homes are distinct from constant slots.
5. Publish the parent's existing return destination and continuation, then
   make the fully initialized child current. Close the detached names owner.
   An exact tuple containing exact Unicode strings has no Python destructor
   callback; no arbitrary argument or callable is retired here.

The temporary array contains moved ownership, not extra references. It is
drained by stores before publication. The child then owns callable, positional,
keyword and duplicated default homes; the parent owns none of those aliases.
No child exists on an eligibility miss. No partial-child error cleanup is
introduced: the transfer phase is infallible, just like the current positional
hit. Stack exhaustion, code preparation, callback-capable binding, errors and
partial child cleanup stay with the established fallback transaction.

## Source boundaries

| File | Intended change |
| --- | --- |
| `Include/internal/pycore_feedback.h` | Same-size union, keyword count/kind flags |
| `Include/internal/pycore_code.h` | Add borrowed kwnames input to keyword specializer |
| `Python/specialize.c` | Build bounded mapping and coverage proof; update positional field names |
| `Python/ceval.c` | Extend shared consuming binder; preserve positional behavior |
| `Python/bytecodes.c` | Pass kwnames into specialization; guarded invoke only for `CALL_KW_PY` |
| `Tools/cases_generator/ir_generator.py` | Share guarded invocation emission, select correct keyword fallback, extend exact prefix/window proof |
| `Lib/test/test_generated_cases.py` | Generator contract positives and changed-transfer/guard negatives |
| `Lib/test/test_ir_frame_calls.py` | Warm binding, mutation, ownership and fallback regressions |
| Generated interpreter products | Regenerate from producers; do not hand edit |

`CALL_KW_BOUND_METHOD` currently shares `_PY_FRAME_KW`; give `CALL_KW_PY` its own
guarded invoke primitive so method expansion cannot inherit an unchecked plan.
The existing generator proof currently accepts only `CALL_PY_GENERAL` and a
VECTOR window. Extend it explicitly to the keyword form and its trailing
kwnames input. Keep rejecting changed guards, missing recursion checks, altered
consumption and extra callbacks. Its fallback must use `self.call_run`, not the
current hardcoded `call_transaction_run_vector`.

Coordinator approval also settles ownership of any stats touch. If existing
profiling cannot distinguish binder hits from generic `CALL_KW_PY`, add one
stats-only keyword-plan-hit counter through `Include/cpython/pystats.h` and
`Python/pystats.c` (dump and aggregation). Handler names alone do not prove
plan execution. Release builds incur no counter instruction. This is optional
measurement support, not a second implementation lane.

## Decision checks

Small-first ladder, executed by the coordinator after approval:

1. Generator contract tests and regeneration. Confirm unchanged feedback size,
   correct kind-specific fallback, and absence of accidental invocation-marker
   or window-consumption changes. Compile real release and debug builds.
2. Warm 1/2/3-keyword cases with distinct values, all permutations, positional
   prefixes and explicit self; functions with more than three parameters;
   missing positional defaults including holes and object defaults;
   explicitly supplied keyword-only parameters. Confirm a real plan hit.
3. Warm then change code/signature, defaults, kwdefaults and callable; exercise
   default tuple deletion/replacement/overlong tuples, retained frames and
   traceback locals. Verify returned object identities and collection after
   success/error, rather than relying only on scalar results.
4. Compare warm/cold error behavior for positional-only keyword use, duplicate
   positional/keyword supply, duplicate names through an existing vectorcall
   test fixture, unknown/non-string names, missing required arguments, excess
   positionals and missing kwonly defaults. Arbitrary `**mapping` with equality
   hooks, exceptions or mutation remains on CALL_FUNCTION_EX; retain its tests.
   Exercise method, generator-owned caller, tracing/monitoring and custom
   provider fallbacks; verify cleanup callbacks see coherent frames.
5. Run focused `test_ir_frame_calls`, `test_generated_cases`, `test_call`,
   `test_extcall`, `test_pyvalue_call_bridges`, and applicable call-transaction
   tests. Independent post-review examines source and executed evidence.
6. After integration, coordinator measures unchanged keyword micro and
   call-heavy applications, with identity/affinity controls. Report hot hits,
   removed owner duplications/name searches and instruction counts. Repeat any
   material application regression before retaining the change.

Keep only if Python behavior and owner checks pass, the admitted hit bypasses
generic binding, keyword instructions fall, and application evidence does not
show a material regression. Do not broaden into variadics or kwdefault-cache
invalidation to rescue a weak result. If time runs short, finish the same
1–3-keyword plan without omitted defaults; do not add a benchmark-only arity.
