# Documentation ownership

Keep a clear path from the [goal](GOAL.md) to the active
[plan](issues/call-model55/plan.md),
[proposed call model](design/proposals/register-call-model55.md),
[status](STATUS.md), and durable
[designs](design/runtime-architecture.md). Readers should not need an experiment
chronology to understand a project decision.

| Level | Authority | Contents |
| --- | --- | --- |
| Project | GOAL, README, STATUS, ROADMAP | Direction, entrypoint, current outcome and stage dependencies |
| Stage | stages/*/README.md | Deliverable, dependencies, gates and workstreams |
| Workstream | Workstream plan | Current problem, options, next steps and gates |
| Experiment | issues/* | Design, review, implementation, commands, results and decision |
| Durable contracts | design/*, PROCESS | Selected architecture and operating rules |
| Handoff | HANDOFF | Paths, builds, hazards and execution state |

STATUS is the aggregate authority for current qualification and performance.
Avoid copying its counts into navigation and design pages. Update the relevant
parent when an outcome changes. Distinguish proposed, reviewed, implemented,
tested, integrated, qualified and measured; identify the runtime on which
evidence was obtained. Keep exact operational details in the handoff and
experiment records. Label planned JIT tiers explicitly.

Round54 is closed at a correctness checkpoint with recorded qualification
limits and no selected performance experiment. Round55 has a reviewed design
and complete migration plan; implementation is active in isolated worktrees. Its proposal
stays separate from as-built runtime contracts until implementation and
qualification establish a replacement.
