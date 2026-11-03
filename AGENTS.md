# AI agent guidance

CPython has a [policy on the use of AI tools](https://devguide.python.org/getting-started/ai-tools/).
All use of AI tools and agents when working on or interacting with CPython
must follow it.

> [!important]
> **Primary directive**: Read the policy before making or proposing any changes.

When acting on this repository, apply the policy's core principles:

- Consider whether the change is necessary.
- Make minimal, focused changes.
- Follow existing coding style and patterns.
- Write tests that exercise the change.
- Keep backwards compatibility with prior releases in mind.

## PyJIT rebuild coordination

Start with [pyjit/README.md](pyjit/README.md), [STATUS](pyjit/STATUS.md),
[GOAL](pyjit/GOAL.md) and [HANDOFF](pyjit/HANDOFF.md). Follow the stage →
workstream → experiment hierarchy and [process](pyjit/PROCESS.md). The canonical
documentation is in this checkout, not historical copies in runtime worktrees.

This project rebuilds and ports extensions; hard backward source/binary
compatibility is not an admission gate and needs no further approval. Preserve
Python behavior and explicit ownership. Keep unrelated work and sealed evidence
unchanged. The coordinator owns integration, merged validation and status.
