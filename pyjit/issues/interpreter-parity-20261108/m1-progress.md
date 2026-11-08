# M1 implementation progress

2026-11-08 12:15 UTC. Provider, checked first-statement marker, shared ordered
retirement emitter and focused source tests are in the isolated runtime
worktree. The provider accepts GIL-held exact combined general dictionaries
and inline integer keys only, then returns an owned canonical value only for
first-probe identity. Decline leaves inputs and exception state untouched.

In-memory IR generation succeeds. Five mutations (effect before marker,
duplicate/nested marker, changed guard and swapped input shape) are rejected.
The generated hit consumes B/C, publishes the temporary, checks and retires
key then dict, reloads only after a possible escape, stores a local and
dispatches. The original slow body follows the decline. Native builds, tests,
generated products and performance evidence belong to the coordinator.

Fresh review corrected collision-test setup: mutation is armed only after the
target dictionary is built. Source is ready for coordinator materialization.
