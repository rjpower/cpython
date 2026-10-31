# Experiment title

- State: queued / active / blocked / review / deferred / closed
- Owner: accountable owner and active agent when assigned
- Updated: YYYY-MM-DD
- Budget: concrete elapsed/work budget; checkpoint and decision deadline
- Depends on: linked experiment IDs, or none
- Branch/worktree/head: actual identity when assigned
- Outcome: landed / reported / abandoned / superseded (closed only)
- Decision: integrate / reject (when concluded)

## Hypothesis

One falsifiable claim about useful behavior, performance or maintainability.
State the baseline and the smallest change that can answer it.

## Experiment

List the bounded change, owned files, relevant behavior checks and measurements.
Use the smallest functioning consumer that answers the question. Record the
control, comparison conditions and evidence links; prefer observed data,
liveness, resource behavior and memory over incidental implementation counters.

## Decision rule

Integrate when the behavior is correct and the stated benefit/structural result
justifies the change. Reject when evidence refutes the hypothesis or complexity
outweighs the benefit. Define both conditions before running the experiment.
At the budget limit, stop and record the result. Reject an inconclusive attempt
without claiming the hypothesis disproved; narrow or split a follow-up instead
of silently extending scope. Partial foundations can integrate when
they are useful and correct; they need not deliver every later optimization.

## Now / next

Current source-bound result, immediate action and blocker/resume condition.
Keep this record current; put long designs, run history and reviews in linked
optional files. Close with the decision, actual landing identity if applicable,
and separate follow-ups. A reported finding is not a runtime landing.
