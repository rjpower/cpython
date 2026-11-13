# M1 GC and observer source post-review

2026-11-12: coordinator and frames peer source review GO for the seven-file patch in
`BASE/worktrees/call55-gc`, based on selected54
`61015a9452d9069646fb5fede9ff963896833b89`.

Typed frame deallocation clears strong pins; free-var initialization checks
nullable typed function. Tracing diagnostics and tracemalloc use direct typed
code presence. FT deferred adjustment excludes strong pins, while optional
marking enumerates both code and function. Remote frame and asyncio readers
read direct code pointers; frame sentinel ownership is checked before NULL.
Frames peer found that sentinel-first remote validation bypassed the existing
high-address rejection for torn/reused samples. Resolved before integration:
range rejection precedes sentinel acceptance, which precedes NULL rejection.
GDB skips interpreter shims before reading code metadata. Header names/offsets
are unchanged, requiring no debug-offset wire change.

Diff reviewed and `git diff --check` passes. No scanner, full-home interpretation,
locals boundness, overlap, map authority, or frame allocation changes occur here.
The patch is frozen for coordinator integration. Merged release/debug compile,
focused lifetime/frame/construction, external inspection, tracemalloc, GC verifier
and permitted GDB checks remain pending. Native inspection remains pending.
FT source consistency is not FT runtime qualification.
