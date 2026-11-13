# M3 bounded integrated cold review

Reviewed `b75e9cc3201..01586ea4d0b` source and root's retained
`E55/m3-native-{_TAIL_CALL_CALL_PY_EXACT_ARGS,_TAIL_CALL_RETURN_VALUE,_PyEval_UnpackIterableValue}.txt`.
**Source/native-placement GO for this bounded change; no MUSTFIX found.**
No candidate/native jobs were run by this review. Root reports six targeted
and 961 merged focused tests passing in both roles; full M3/static authority
and performance admission remain open.

`_PyValue_FunctionOwnerSteal` consumes an already canonical owning function,
asserting the mortal owning-tag contract in debug and extracting its typed
pointer without the generic borrowed/deferred acquisition. The initializer's
callers already transfer/duplicate that owner: exact guarded CALL, generic/EX
initialization, unchecked/overlapping pushes and the explicit DUP in
`PyFrame_New`. The independent code pin remains acquired separately. The
count fixture checks this transfer rather than accepting an unowned pointer.

The exact CALL native dump retains the necessary pointer/type/version,
argument-count, reservation and recursion guards. It stores the owned function
pointer directly at 3777c1. Compared with the retained M1 initializer sequence,
the generic owning-tag mask/test/branch and borrowed acquisition arm are gone.
The independent code increment and immortal/refcount checks remain. CALL still
reserves 64 stack bytes, with argument/self/extent/continuation spills visible;
this is a classification removal, not elimination of guards, spills or code
pin traffic. RETURN still reserves 16 bytes and publishes its result after
retirement. No transition record was added to either handler.

Generic `_PyEval_UnpackIterableValue` now consumes the sequence on every exit;
both actual generated callers drop their later DECREF. Its linked initialized
suffix starts before iteration and survives surplus-value, iterator and
sequence cleanup. Each source write installs a value before extending the
suffix; failure advances the cursor and NULLs the home before close. Starred
tail transfers remove the list's old edges before the next callback. Success
disarms only after outer cleanup, with callback-free handoff to the current
NULL-authoritative frame. Nested unpack gets a separate descriptor. The
specialized exact tuple/list paths remain independent fixed-output paths.

Native unpack reserves 120 stack bytes after five saved registers and carries
a stack canary. The record/head and suffix bounds are set before the first
iteration callback; the cleanup path advances its cursor and NULLs before
decref, closes iterator then sequence while linked, and restores the head
afterwards. Scalar spills and numeric conversion branches remain visible.
The compiler reorders some adjacent write/cursor initialization stores, but
there is no intervening callback on these GIL commit segments. This does not
provide separate FT synchronization qualification.

The fixtures inspect precise IDs during production and outer cleanup, nested
unpack and partial failure, plus mutation/redirect refusal. Root ran them;
ordinary object survival through native conservative discovery alone would
not establish prefix coverage. Removing the unproduced FT `c_stack_refs` loop
does not remove a real producer: existing CValue workspaces hold counted
owners and retain the selected native policy. No blanket C-root chain was
added to pay for verifier-only discovery.

The live suffix is one concrete M3 arena consumer. Before M4, success needs
verified output POST publication before disarm, folded local outputs and
observer refresh need exact contracts, and staged generic/keyword/EX binding
homes need an outer lifetime descriptor spanning provider/caller cleanup.
Complete callback/error/suspend maps and lifecycle readers remain required;
this change removes neither NULL homes nor arena scanning. Actual whole-model
publication, storage and spill costs still determine admission.

## GC composition peer at 9136c202e8e

This is a continuing cross-lane review, not a fresh-agent review. The GC lane
independently reread frames' retirement/binding/provider and IR's admission/
certificate source in the integrated checkpoint, plus root's retained native
evidence. It authored the constructor and collector observation components;
those components are **not** claimed as independently reviewed here. Frames'
separate constructor source GO remains in init-shim-peer.md. Root independently
reviews the complete composition. Later heap-deallocator source `0851fd51801`
has its own peer file and is outside the native checkpoint below.

No new MUSTFIX was found in the reviewed cross-lane composition under current
NULL authority. Provider handoff matches explicit kind and a live frame before
dereference, excludes constructor-only flagged scopes, and disarms authority
before default/custom retirement. Empty LIFO provider nodes remain through
post-default hooks, so recycled frame storage is not probed afterward. Typed
nullable LOCALS/FRAME_OBJECT authority is dynamic while staged. Object and value
providers preserve their different result conventions; the extra selected-code
pin closes before evaluator entry while the child's execution pin remains.

Unlinked thread/gen retirement retains the result, remaining homes, headers and
generator exception through their actual closes. Code remains available through
the final cursor/map read; records use only stored fields after code detach.
Heap takeover removes source edge authority before the destination is observed.
Fast retirement consumes only callback-free releases and leaves a dangerous
owner in its home until the outlined bridge is linked. Unpack's installed suffix
and binder's not-current child remain rooted through actual outer cleanup.
No external marking visitor is reused for cyclic subtraction/deferred adjustment.

The IR worklist keeps must-NULL/readability intersections separate from
MAY_OWNER unions and inspects only after fixpoint. Unsupported/error/observer
seeds cannot acquire complete certificate coverage. The persistent return plan
is built from the normalized image, accounts for normalized physical scratch,
uses shared-empty or sorted sparse storage and balances size/free paths. The
cold query validates reason, independent code/image identity, PC bounds and
alignment and immutable exit-site role before scanning canonical NULL control;
inline/index leftovers also mismatch. This certifies only the stated empty
normal-return shape, not callback/error/suspend authority.

Root's retained provider native dumps show the outlined object/value provider
stack reservations of 200/168 bytes respectively, plus saved registers. The
handoff remains an outlined chain walk; retained exact CALL/RETURN dumps contain
no provider lookup and reserve 64/16 bytes respectively. Those dumps predate the
constructor flag exclusion, whose source and focused fixtures are checked
separately; they are not native proof of that later instruction sequence.

Evidence inspected, without running candidate jobs in this review:

- `m3-provider-precise-{release,debug}-focus.log`: seven provider methods plus
  malformed remote-location degradation, eight passes per role at clean de954.
- `m3-provider-core-debug-debug-focus.log`: 1,487 tests across 27 modules passed.
  The release campaign's one failure was a wrong `test_borrow_scope` module
  name, not an execution failure; the corrected CAPI/borrow-scope campaign ran
  28 tests successfully in each role. Do not label the original release log
  wholly green.
- `m3-shadow-precise-{release,debug}-focus.log`: 13 focused certificate/retirement
  tests per role at a95. Command manifests pin clean source and native paths.
- At clean 9136, `m3-init-precise-*` records three constructor fixtures per role;
  `m3-init-core-*` records 942 tests across 13 modules per role, release skips12
  and debug skips9. This is execution evidence, not independent authorship.

Performance admission, FT execution, the complete suite and the atomic map
switch remain separate gates. [lifecycle-readers.md](lifecycle-readers.md) now
names the concrete copy, suffix-close, resume old-load, elided-self, suspended
receiver, observer and full-range GC consumers that still assume initialized
NULL scratch. Passing current NULL-control fixtures does not remove those ports.
