# Interpreter shared-cost roadmap

- State: closed
- Outcome: superseded
- Decision: select one bounded hypothesis at a time
- Updated: 2026-10-31
- Owner: coordinator

Current work starts with [residual cost attribution](../interpreter-residual-costs/status.md).
Its result selects one implementation experiment. Native interfaces, producer
facts, shape admission, frame transitions and alias-safe string growth remain
candidate directions, selected by observed cost rather than one large cleanup.
Roughly 2× is soft guidance. No new interpreter runtime change is landed here.
