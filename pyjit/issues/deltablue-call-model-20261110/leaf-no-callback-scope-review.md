# Leaf scope reduction: prove retirement cannot deallocate

**Yes: require nonfinal retirement for the first leaf experiment.** On this
runtime's GIL build, a bounded alias-aware refcount preflight can exclude all
deallocation, Python callbacks and native destructor callbacks through result
publication. That removes the unresolved callback-time completed-CALL debugger
phase from the first implementation. It is a useful narrower contract, subject
to measured hit rate and guard cost; it is not a replacement for general ordinary
completion. Keep the bounded whole-body certificate and shared read/compare
probes from [the first review](leaf-contract-review.md).

This follow-up reviews runtime46 `b8e7e1bee6611eeda320ce163cd3ce89323da877` in
the execution-boundaries worktree. No runtime/code edits, builds, tests or
commits. The previously read repository instructions and
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/) apply.
Remote sampling may omit the optimized callee, as for normal frame elision;
exact asynchronous logical-stack equality is not an admission condition here.

## Why the source supports the proof

`Include/internal/pycore_value.h:PyValue_XCLOSE` tests
`PyValue_RefcountOnObject` and calls `Py_DECREF_MORTAL` only for a word that owns
a count. Its GIL implementation in `pycore_object.h` decrements the object and
calls `_Py_Dealloc` only at zero. Debug reference-total and stats updates do not
invoke Python/native destructor callbacks. Nonfinal closes therefore cannot
clear weakrefs, release contained owners, run a finalizer or trigger a destructor
cascade. A C function call used for bookkeeping is not itself an observer.

`PyValue_XDUP` increments the existing counted object's count through
`_Py_INCREF_MORTAL`; it does not allocate, canonicalize, box or invoke an object
hook. Use canonical-home duplication only. A fallible object bridge or scalar
normalization with a hidden box release does not satisfy this contract.

Deferred reference counting is explicitly absent here:
`_PyObject_HasDeferredRefcount` returns zero in the GIL build. Do not generalize
the proof to `Py_GIL_DISABLED`: local/shared counts, merging, deferred roots and
concurrent mutation need a different proof. Tagged no-count mortal values are
not ordinary canonical GIL homes. Reject execution while
`_PyValue_ClearingImmortals()` is true; finalization can otherwise invalidate
the tagged-implies-immortal home invariant. For counted words, including an
object made immortal after that word was created, use the actual release
semantics and strict-count check rather than assuming the no-count fast path.

## Exact release multiset and alias proof

Start with the existing exact-call guards, a supported complete leaf certificate,
an unchanged original call window of at most eight homes, and the original
callback-free entry/observer predicate. Probe the selected body and acquire one
canonical result owner while all original roots remain intact. Now construct
the complete multiset of releases the selected completion will perform:

* Every consumed callable/self/argument window home, including duplicate owners.
  NULL and inline values cause no counted release.
* The old LOCAL destination owner, if this is a LOCAL result. The current LOCAL
  is read now, under the same GIL-held interval; it cannot change before commit.
* The acquired result owner itself, if the destination is discard.

LOCAL and discard are mutually exclusive, so there are at most nine release
words. A temporary destination overlapping the call window has its old owner
already in the window multiset. Do not add it again: after detachment its old
contents are NULL. A nonoverlapping temporary destination must already be NULL,
as required by ordinary return publication; otherwise decline. LOCAL homes are
outside the consumed temporary window by the call ABI.

For each distinct object `o` among **count-owning release words**, count their
multiplicity `R(o)`. Compare object addresses, not equality, type, hash or numeric
value. Require, after result acquisition:

```
Py_REFCNT(o) > R(o)
```

Use a fixed table or pairwise identity scan over at most nine records; no hash
table, allocation or auxiliary object owners. Actual count ownership comes from
`PyValue_RefcountOnObject`, not merely `PyValue_IsObject`. No-count immortal and
inline words do not contribute a decrement. Validate NULL/language-value/home
requirements through existing producer contracts; do not treat arbitrary tagged
bits as a valid no-count value.

The strict inequality proves that every prefix of the planned releases leaves
each counted object positive. There are no other releases, no GIL release, no
Python execution, no GC/poll, no mutation of those counts by another Python
thread, and no object can deallocate. Thus no transitive releases can occur
outside the counted multiset. The release order cannot invalidate this proof.
Keep canonical CLOSE/CLEAR operations; do not implement a new bulk refcount API.

Result acquisition is reversible preparation, not ownership commit. If any
count/admission guard fails, close only that acquired result and return MISS.
Its original source is still protected by the unchanged window/code/global/
field owner chain, so closing this one duplicate cannot be final. No original
input/destination is written. Avoid unchecked arithmetic that predicts an
incremented count: acquire first, then inspect the actual count. Reuse existing
dup semantics and assumptions about valid counts rather than inventing a new
overflow/immortalization mechanism.

Three distinguishing alias cases:

| Owners | Correct guard consequence |
| --- | --- |
| Two window homes own `x`, refcount two; no retained result | `R(x)=2`, reject. Checking each home against one would incorrectly admit the final decrement. |
| Window and old LOCAL both own `x`, refcount two; result differs | `R(x)=2`, reject. Counting only window owners would miss destruction during replacement. |
| Window owns `x`, and result is `x` | Acquisition raises its count. A stored result retains that new count, so exclude it from releases; discard includes it. A sole window owner can pass for stored result and fail for discard. |

## Function, code and environment roots

The captured function is itself in the release multiset. If its window owner
would be its final count, reject even if the function type looks innocuous:
function deallocation can run weakref callbacks and release code/globals.
Common named functions have a dictionary/class owner and pass; an ephemeral
lambda/function may miss. A function returned as the result can also survive
through that acquired result owner.

Before commit the function anchors its selected code, globals and builtins;
arguments and those dictionaries anchor intermediate attribute paths. No callbacks
or writes can replace `__code__`, globals values, type providers or field values
within this interval. After the guard, every counted object that will be
released survives. In particular, function destruction cannot remove the code/
environment roots transitively. Global/constant/attribute borrowed values remain
valid until result acquisition; all such borrowed metadata should still become
dead before retirement to keep the boundary simple. No independent activation
code owner is needed because there is no activation or callback. Never count
a hypothetical independent code owner as an existing root.

This guard proves survival from the actual count, not the identity of an outside
owner. Cycles are acceptable: GC cannot run in the interval. Do not inspect the
heap to find the extra root or assume a named function/receiver necessarily has
one. Do not add temporary pins merely to force the guard to pass: dropping those
pins would reintroduce exactly the potentially final release being excluded.

## Minimal commit and terminal completion

After result acquisition, build/check the release multiset and recheck the
pending-work/instrumentation admission immediately before mutation. No guard may
fail after the first original-home mutation. The successful terminal helper can:

1. Save numeric window/destination facts and the caller's real next instruction;
   set `return_slot`/`return_pc` as the ordinary CALL would.
2. Detach and NULL consumed homes into bounded C storage. Alternatively clear
   each home before its nonfinal close, but full detachment is the simpler shared
   owner recipe and costs only eight C words.
3. Retire those owners. Keep the canonical result held until retirement finishes.
4. Use `_PyFrame_SetReturnValue` for the actual LOCAL/temporary/discard operation,
   with `pending_redirect == 0`. Its possible old-LOCAL/result close was included
   in the preflight; temporary overlap is NULL. The result is transferred once.
5. Return DONE and the actual next pointer in the terminal two-word result.
   The generated hit edge reloads slots and dispatches that continuation without
   flushing cached window owners, moving `window[0]`, saving a child return slot
   again, decrementing recursion or entering `_PUSH_FRAME`.

There is no post-commit fallible operation or callback in this scope and therefore
no committed ERROR/redirect recovery branch. Errors from an unsupported read,
comparison or pending entry work are handled by the ordinary activation after
MISS. An internal invariant violation is a runtime bug, not permission to fall
back with consumed inputs. Keep MISS/DONE explicit; do not invent a child.

This scope can reuse the existing ordinary publication primitive under a proved
noncallback precondition; it need not first generalize ordinary return's cleanup
and debugger protocol. The generated exact-prefix/window/terminal validation
from the earlier review remains required. The CALL input owner phase is unchanged
on every observable MISS; no synchronous observer runs during any transient hit
phase. By the next dispatch, normal caller homes and continuation are valid.
Retained caller frame objects do not invalidate that proof. Existing supported
debugger redirects are neither broadened nor reconstructed inside this interval.

Keep all entry exclusions: monitoring/trace/profile/PEP523, LLTrace, supported
DTrace build policy, current instrumentation, recursion/stack admission and
pending eval work. Disabling retirement callbacks does not make an instrumented
callee entry observationally inert. Pending work arriving after the final check
waits for the next normal poll, just as after a skipped inert RESUME. No extra
periodic-at-end check belongs in this completed call.

## Value and stopping rule

This is cleaner than introducing a general committed-call debugger owner phase
for the first experiment. Common long-lived self/function/argument owners should
pass, but the earlier semantic-call census does not prove that. At worst nine
release words require 36 pairwise alias comparisons when building unique groups,
plus count loads and the existing probes. This could consume the activation
saving. Count eligibility before implementation expansion; measure complete
guard/probe/retirement cost and application time afterward.

Required witnesses: every alias row above; result/old-LOCAL/window all the same
object; function identical to an argument; mortal constant/global/field result;
overlapping temporary and discard; sole-owner receiver/function with weakref or
finalizer callbacks must MISS before effects and execute the normal body once;
external-owner cases must actually hit. Add observer/pending-work negatives and
retained caller inspection after completion. Verify canonical-home behavior and
GIL/debug/reference-stat builds, with free-threaded explicitly declining.

Do not relax the strict inequality based on destructor whitelists, build a
transitive release graph, admit deferred RC or add pins to game the test. Those
are different scopes. This restriction is an optimization boundary, not a claim
that Python requires the existing finalizer schedule. If its measured cost or
miss rate is poor, retain the lookup increments and revisit general completion
with its own review.
