# Documentation

Keep one authoritative current record per issue. Documents have distinct jobs;
link them instead of copying status, plans or logs between entry points.

## Where things belong

| Path | Purpose |
| --- | --- |
| `README.md` | Project orientation and navigation |
| `issues/README.md` | Current work index |
| `issues/<id>/status.md` | State, owner, dependencies, next action and completion |
| `design/<descriptive-name>.md` | Current cross-cutting architecture and contracts |
| Issue `design.md` / `plan.md` | Proposed change and implementation sequence |
| Issue `reviews/` | Findings and dispositions for a named revision |
| Issue `results.md` or `results/` | Actual behavior, measurements and evidence links |
| `PROCESS.md` | Short development workflow |
| `WORKLOG.md` | Append-only, concise high-level outcomes |

Only `status.md` is required. A small task can keep its scope and result there.
Use descriptive, stable IDs; do not create an issue per agent, run or review
message. Split a child only when it has independent ownership and a useful
completion condition. Keep paths stable when work closes.

## Bounded experiments

Before execution, record:

- **Hypothesis:** the specific uncertainty or bottleneck being tested.
- **Owner and budget:** one accountable owner and a concrete elapsed-time budget.
- **Minimum change:** the smallest coherent implementation or observation.
- **Checks:** externally useful behavior and measurable success/failure criteria.
- **Stop rule:** when to stop on failure, ambiguity or budget exhaustion.
- **Decision:** integrate or reject the attempt; name the deciding evidence.
  An investigation can report a finding without landing runtime code.

Choose a useful first increment rather than closing every future contract before
feedback. Follow-ups are optional new work, not automatic extensions. Broader
qualification belongs to the coherent landing, under the
[validation contract](design/validation.md).

## Keep records legible

- Status describes what is true now. Rewrite it at meaningful transitions;
  chronology and logs belong in results, Git and WORKLOG.
- Use `queued`, `active`, `blocked`, `review`, `deferred` or `closed`. A closed
  issue records `landed`, `reported`, `abandoned` or `superseded` as its outcome.
- Distinguish proposed, reviewed, built, measured and landed work. Name source,
  runtime library, native image and integration identities separately when needed.
- Preserve raw results, failures and source/product bindings. Put large outputs
  in project storage; commit a readable result with evidence locators and hashes.
- Review outputs identify their input revision and remain under unique names.
  Active navigation points to current contracts and relevant issue records.
- The owner updates its issue; the coordinator updates integration and the board.
  Published views identify their repository source and revision.
