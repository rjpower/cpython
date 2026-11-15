# Independent native Invoke result

Bounded source implementation, reviewed by root; strengthened fixtures await
root's review and native qualification. Baseline: selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0`.
Source checkout: `call55-pins`, branch `pyjit/reg59-native-invoke`. Three focused
host source methods pass; `E59/native-invoke-host1.log` retains the initial
gate and `E59/native-invoke-host2.log` the strengthened NULL-error ordering
gate (three methods, 9.263 seconds).
No full generation, native build, runtime execution or measurement by this
worker. Root owns those gates. The first result-binding trial remains separate.

## Actual overlap

The selected57 `Python/bytecodes.c:5010` declares `_CALL_METHOD_DESCRIPTOR_FAST` with unchanged
callable/self/argument outputs. Its body obtains an owned `result` from
`_PyValue_VectorcallTstate`, assigns it to `callable`, then closes the old
callable. The macro appends `_POP_TOP_OPARG`, `POP_TOP` and
`_CHECK_PERIODIC_AT_END`. RegCFG already gives the complete call a fresh output
ID. `result_offset()` nevertheless recovers its DSL output by recognizing an
assignment to an input name.

The selected generated handler at `Python/generated_ir_cases.c.h:8479` performs:

1. The existing descriptor/type/flag/receiver guards and the same vectorcall.
2. Publication of the result to `window[0]`, then close of the old callable.
3. `_PyFrame_CloseHomeRange(frame, args, oparg)` for arguments at B+2 onward.
   The helper takes each home before closing its owner, descending.
4. `call_clear_owned_slots(window, 1, 1 + oparg, ..., call_state)` before
   closing the saved self. This clears B+1 when self is present and rewrites
   **all `oparg` argument cells already taken by step 3**. It skips absent self.
5. Periodic check, then move/discard of `window[0]` to recipient A.

`CALL_NO_SELF_METHOD_DESCRIPTOR_FAST` at generated line 10678 has the same
overlap; its virtual self home remains untouched. These are microphases in one
handler, not dispatches. The concrete duplicate is `oparg` NULL stores plus a
second traversal of those cells. The callable has one old-owner close and the
result has one necessary normalization; neither is a removable duplicate.

Common admission (`Objects/codeobject.c:1955`) currently restricts scratch A
to B for this WINDOW_DST_FIRST shape. Keep that restriction in this trial.
Arbitrary scratch destinations and extra overlap workspace are unnecessary.

## Small contract and consumer

Migrate FAST first, including its NO_SELF execution variant. Leave
FAST_WITH_KEYWORDS and other native families unchanged until the first native
comparison. Keep the original guard uop, provider, call-state roles, argument
layout and fallback to the existing generic transaction.

Change the producer DSL output to one distinct owning `result`; remove the
assignment `callable = result` and the trailing POP uops. The result is the
complete operation's existing `out[0]`, not a new RegCFG ID. Keep its physical
result geometry unchanged initially. An explicit producer declaration takes
precedence over assignment-based `result_offset()` inference; it must not
coexist with a second inferred result for this producer.

Use one immutable generator contract with only consumed fields:

```text
NativeInvokePlan(
    provider="_PyValue_VectorcallTstate",
    output_index=0,
    input_roles=(0, 1, 2),
    retirement=(0, 2, 1))
```

These roles resolve callable, optional self and the argument array. The
lowerer checks the supported signature, existing CALL_SELF_PRESENT role and
actual oparg. It stages B0 for every recipient and retains the existing late
local/discard commit. No extra declaration fields encode those fixed policies.
The declaration resolves external IO and the existing call-role geometry.
It grants no result owner, canonicality, non-NULL value, safe callback, or
completed drain by itself. Two used markers consume it:

```text
IR_INVOKE_NATIVE_VALUE(result, callable, self_or_null, args, oparg)
IR_RETIRE_NATIVE_INPUTS(result, callable, self_or_null, args, oparg)
```

The first emits the existing provider and its actual NULL/error branch, then
binds the owning current output Local. The second consumes that receipt and
the declared input identities. The producer consists of these two ordered
statements; it cannot append a release, jump or second publication afterward.
The existing periodic-check uop remains the following instruction component.

The lowerer emits the same `_PyValue_VectorcallTstate` call, using the actual
checked input window and existing optional-self role. The declarative provider
and role/signature checks precede emission; actual acquire/retire receipts must
both occur. There is no new C-body matcher or registration of ordinary native
calls as DSL macros. Unsupported aliases, shadows, address escapes or result
reassignment reject generation for the migrated producer rather than silently
restoring the removed POP spelling.

The terminal lowers the following ownership sequence, without a per-call
allocated retired-input array or a general transaction-helper call:

* NULL result: transfer to the existing error path while original input homes
  still own their inputs. No result owner exists and no drain is claimed.
* Success: take the callable home into its own retiring token. Normalize and
  publish the distinct result to B0 for **every** recipient mode before any
  close, preserving the selected intermediate root/phase/error state. This
  transfers the result owner, not the old callable token.
* Close callable, take/close each argument descending, then take/close actual
  self. Never read, clear or close the absent-self gap. Detach each owner before
  its callback-capable close, retaining the existing close order.
* Run the existing periodic/continuation check with the result still in B0.
  Failure enters ProcessError with exactly the existing B0 result root. On
  success retain the existing final move to A: a local is committed only now,
  with displaced-local retirement afterward; discard only now. This first
  slice introduces no C-owned uncommitted result across cleanup callbacks.

Reuse existing publication normalization. A VALUE result may be private/TEMP;
its type and ownership do not permit a raw canonical store. The error edge
must neither lose the B0 owner nor publish/close it twice. A result equal to a
callable or argument is still a distinct acquired count.

## The NULL receipt is the gate

The terminal's actual take/close operations provide the detached-home receipts.
Only their completed, callback-safe plan can set the existing
`window_taken`/`CALL_DRAIN` state. That state suppresses the later generic
vacate, which is the current second clear pass. A source declaration or helper
name alone must never set it.

The selected storage reservation supports the proposed omission for this
native path; this is stronger than logical absence:

* `_PyThreadState_PushFrameUnchecked` (`pycore_interpframe.h:705`) reserves all
  `nslots` below `datastack_top`, placing new headers above that value frontier.
  `_PyFrame_PushUnchecked` supplies the code's full slot count, not the current
  logical live extent. `_PyFrame_ReserveForCall` (line 780) can overlap exact
  Python arguments, but keeps `datastack_top=max(saved_top, child_end)`.
* `_PyFrame_CloseHomeRange` (line 222) changes individual homes via TakeHome;
  it never reduces that reservation. Neither the selected descriptor handler
  nor its POP/periodic microphases lower `datastack_top` or unlink the parent.
* A Python call entered from native code uses
  `eval_frame_push_and_init_code` (`ceval.c:3631`) and
  `_PyThreadState_PushFrame`, starting its value storage at the existing top
  (`pycore_interpframe.h:712`). Further exact calls can overlap **that child's**
  argument window, not the native parent's reserved homes. Header/value
  separation is checked before header writes by EnsureFrameStorage (line 688).
* New-chunk allocation saves the previous top (`pystate.c:3160`); PopFrame and
  PopChunk restore it only when the child has departed. Parent generator homes
  are independently heap-owned; nested thread-stack allocation cannot reuse
  them. Normal child retirement leaves its own value suffix NULL.

Consequently ordinary nested allocation cannot replace a detached parent's
argument cell with an untyped child header. The first native implementation
must retain this reservation through the last retirement callback. It must not
adopt a compacted live frontier or a returned-child tail argument from another
path as proof. Verify the actual owning-thread/current-frame and generator
paths in the integrated native witnesses; no extra reservation is proposed.

Specifically audit reentry during each close. Existing source setters can
clear scratch or restore protocol values to L; named-local writes affect L,
and executing-frame clear/resume is denied. Those facts need an actual checked
consumer before previously detached argument cells can remain certified NULL
across later closes. If redirect/replacement can repopulate or change the
relevant storage interpretation, refresh/invalidate the plan and retain the
required clear. Canonical-or-NULL preservation alone is not an absence proof.

Keep intermediate observation separate from normal completion. The final
normal CALL poststate is unchanged, so the existing call outcome, physical
transfer and absence readers can remain applicable only after the terminal
actually completes. During native invocation and retirement retain honest
phase-0/control coverage. Audit GC/transition roots, error ingress, source
redirect recipes and monitoring enabled by the callee against the result's
actual B0 location. Do not publish a normal-completion receipt before cleanup
finishes. Local/discard delay retains the existing B0 root until final commit.

## Files and evidence gate

The current source packet is exactly `Python/bytecodes.c`,
`Tools/cases_generator/analyzer.py`, `Tools/cases_generator/ir_generator.py`,
`Lib/test/test_ir_native_invoke_source.py` and
`Lib/test/test_ir_native_invoke.py`. Root reviewed the production delta;
frames' independent actual review remains pending. Root owns generated
cases/metadata and native qualification. Scalar/error replay remains uncovered for this
manual window operation. Inspect
`pycore_ir_outcomes.h`, `Objects/codeobject.c`, ProcessError, frame/GC and
instrumentation readers for the unchanged final geometry and changed
intermediate result location. Update any affected reader in the same packet.
No ABI field or new format enum is proposed without such a reader.

Source checks must reject missing invoke ownership, a borrowed/copy result,
alias/address/shadow mutations, missing NULL arm, a second publication, wrong
input roles/count/order, a missing retirement receipt and a post-terminal tail.
They must preserve ordinary nested native expressions. Emitted-source checks
count actual argument takes and prove the redundant clear interval disappeared;
they do not replace ownership tests.

The new runtime fixtures cover both self roles, local/temp/discard, a result
aliasing an input, provider failure, input finalization inspecting/rebinding
caller L with GC, and monitoring enabled during the provider. The retirement
witness removes the mapping's last payload reference during the key's
finalizer, so only the staged result can keep that payload alive before local
commit. Existing call-role/window-result/retirement suites supply the wider
admission and periodic-error gates; the new fixtures have not run natively.
Root runs release/debug builds and the focused native set before broader
qualification.

Native hypothesis: retain one provider call, one required normalization and
the existing B0 result materialization; remove `oparg` repeated NULL stores
and clear-loop work. The independent result token removes assignment-based
inference, not this first slice's materialization. Inspect both self variants against selected57 for
loads/stores, branches, spills, calls and text size. There is no predicted
whole-benchmark percentage. Reject or narrow the trial if ownership/observation
requires a general transaction helper, duplicate cleanup survives native
optimization, or broad sentinels regress. Trial 1 and builder allocation need
not be selected for this trial to proceed.
