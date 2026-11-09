# Same-PC observer redirect intent

Read-only source review during native21 freeze. No runtime fix or worker native
execution. The new frameobject generation counts successful debugger commits,
including same-PC commits; monitoring line_epoch instead counts configuration
changes. These are independent facts.

## Concrete plain-source probe

`probe-same-pc-source-redirect.py` is a bounded coordinator-run probe (host AST
PASS), with cold/warm instances and no-redirect controls:

```
def target():
    x = 1
    pass
    return x
```

At the source-only pass LINE callback, change f_locals['x'] to99 and request the
assignment line. The LINE landing wrapper publishes the predecessor real word
while delivering removed-source events. If that predecessor is the folded
assignment, this is a successful same-PC setter, not missing-owner
reconstruction. It should select that real assignment for execution again.
Expected control99 and redirected1 distinguish intent from unchanged execution;
recorded before/after f_lasti establishes whether the commit is actually same-PC.
Actual coordinator receipt is pending; do not claim reproduction from source
inspection alone.

## Actual source path

`source_line_event` saves before=frame->instr_ptr, old_cursor and old_line. The
setter clears source cursor and commits return_pc/instr_ptr, incrementing the
private generation. However the observer checks only instr_ptr!=before after
each callback. With a same-PC commit it restores old_cursor/old_line and reports
no redirect. `source_edge_events` then continues the old edge. The LINE wrapper
can overwrite frame->instr_ptr with its cached landing instruction, losing the
selected predecessor replay. The owner recipe can admit this case: the folded
assignment writes a local and has no incoming temporary requirement. Current
recipes therefore do not provide a general proof against the failure.

Error handling has the analogous ambiguity: a setter followed by a callback
exception leaves the same PC, so source_line_event may record the old edge EH
recipe despite the setter having explicitly selected a real-word state and
cleared its cursor. A source event's handler can differ from its surrounding
real word. Do not silently assume their recipes coincide. A dedicated ordinary
source try/pass probe should follow a confirmed replay counterexample.

Ordinary jump wrappers similarly detect redirect only by PC and otherwise
return their cached destination. A successful same-site selection can be a
request to execute that real transfer again. The generation should define
whether the old callback transaction is still valid, separately from the
meaning of the selected PC.

## Scope and rejected hypotheses

- A public f_lineno getter can mutate its lazy cache; it does not increment
  redirect generation. Public-cache changes must not cancel a setter or edge.
- line_epoch invalidates LINE suppression after reconfiguration, but it does
  not encode redirect intent. A setter need not reconfigure monitoring.
- PRE StopIteration completion callbacks cannot normally set f_lineno: that
  event is excluded by the setter gate. They are not an immediate positive.
- FOR_ITER/SEND semantic sites require live input windows. The current empty
  incoming-owner recipes generally reject a same-site restart there; do not
  claim an existing completion counterexample until its actual setter succeeds.
  The upcoming loop recipe must explicitly preserve phase and cannot rely on
  that accidental rejection forever.
- A same-PC ordinary LINE callback often already proceeds to that same real
  instruction, so not every missed intent check changes the result. The source
  predecessor-to-landing and jump-site-to-destination paths are the relevant
  places where cached continuation differs from the selected PC.
- Current activation-owned source-error storage protects nested evaluator
  activations; it does not decide whether an old source callback recipe should
  survive a successful same-PC redirect in its own activation.

## Minimal corrective direction, not implementation admission

At callback-transaction boundaries, capture frameobject redirect generation
before dispatch and treat either a PC change or a successful generation change
as redirect. Creating a frameobject lazily in a callback is not itself intent:
no object before the callback corresponds to initial generation0. Snapshot and
check must handle that case without adding eager allocation to ordinary paths.
Source-only callbacks already have a frameobject.

Apply the same decision before restoring saved source cursor/line, before
installing a source error recipe, before producing a LINE handoff, and before
returning a cached jump/completion continuation. Return the committed current
PC on success. On callback error, use the committed real-PC context when intent
changed; do not resurrect the cancelled old edge. Source errors without a
redirect retain the existing source-record recipe, including allocation failure.
No per-opcode generation load is needed: checks belong only to observed
callbacks that can call the setter.

A generation check is necessary but not a substitute for owner-phase validation.
The setter still must prove and commit an admissible target, and completion
helpers must preserve reentrant redirects during detached-owner closes. Root
and B should review the demonstrated route before broadening shared helpers.

## Confirmed receipt and bounded implementation contract

Coordinator native21 reproduced the plain-source failure cold0/warm20: redirected
result99, PC1→PC1, where replay requires1. Evidence:
`ownership21-same-pc-source-probe.{json,log}`, release ELF SHA
255ca2f9c86f3373b533e8236ac532b990986f6fd160a80981dcf35d089e9aeb.
This is a supported setter success, not an inferred invalid owner recipe.

Proposed source scope is Python/instrumentation.c and ordinary compiled
regressions in test_ir_source_reentry.py only. No DSL/header/frame change.

Add a file-private callback snapshot containing physical instr_ptr and
f_lineno_generation (0 when frame_obj is absent), and a predicate comparing
both against current state. Frameobject creation with generation0 is not intent.
Use it at the three existing PC-sensitive observer transactions:

1. source_line_event: capture before publishing the source cursor. After each
   callback, return redirected on either change; restore cursor/cache only when
   unchanged. On error install the old edge recipe only when unchanged. The
   caller's existing positive redirect result prevents cached landing commit.
2. _PyIR_ObserveStep: capture at semantic-site observation entry. Test after
   branch callbacks and before returning the cached continuation, including PRE
   completion. Successful change returns committed PC with REDIRECT. Error still
   uses published PC+1 and existing PRE/POST status; source_line_event decides
   whether a source-record override remains valid. No new phase/error schema.
3. _Py_call_instrumentation_jump: replace its existing callback PC comparison
   with the shared predicate, so a same-site selection returns that committed
   PC instead of its cached destination. Source-only edge callbacks already
   propagate the positive redirect result through source_edge_events.

Do not add generation checks to generic call_instrumentation_vector or ordinary
opcode dispatch. INSTRUCTION cannot call the setter under the existing event
gate. PY_START/PY_RETURN/RAISE/STOP likewise do not admit it. Real landing LINE
callbacks already execute their selected same real instruction; the confirmed
loss is source-predecessor/cached-continuation state, so avoid unrelated changes
to tool notification ordering. PY_RESUME/YIELD paths require separate evidence
before broadening this bounded patch.

Regression: reproduce the compiled assignment/pass/return case cold/warm, verify
control99, same-PC setter selection, replay result1 and single intentional
application. Keep public getter-only control independent of redirect intent.
The existing source-reentry/error, epoch reconfiguration and relaxed-edge tests
are the coordinator regression ladder. Add a source callback setter-then-raise
case only if its ordinary source target passes current protected-owner admission;
do not synthesize malformed EH maps to manufacture a positive.

Actual bounded implementation is source frozen for peer: file-private snapshot
and predicate replace only the three transactions' existing PC comparisons.
Snapshot precedes optional frameobject allocation, preserving old-source error
attribution when allocation fails without intent. No generic callback ordering
changed. Added ordinary compiled cold/warm replay with no-action/getter controls
and an explicit same-PC commit assertion. Host test AST and diff checks pass;
no worker native jobs.

Protected setter-then-raise is not a valid positive under the current setter:
it rejects every nonempty co_exceptiontable before committing. Therefore the
old-source-vs-real-EH distinction is source-reviewed but has no supported
protected regression yet. Existing nested source-error tests still cover
unredirected recipe preservation and must pass the coordinator gate. This is an
explicit test gap for future protected recipes, not grounds to synthesize an
invalid target or weaken their admission.
