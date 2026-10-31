# Native feedback evidence

## First attempt: stopped before configuration

The coordinator accepted the [sealed helpers](reviews/feedback-helpers-20261031-v1.md)
and activated candidate `b34931a881b33b47a2b2f4b1b2a644c50fed963b` at
2026-10-31 21:07:43 UTC. Batch start was 21:07:46.664152, with the absolute
deadline **22:07:46.664152 UTC**. Root-owned exec session 94799 terminated with
status 1. The original evidence remains unchanged under the isolated
checkout's `build/nursery-adoption-feedback/`.

The build row failed after 0.704 seconds at the initial toolchain comparison,
before configure, compilation or a target interpreter. Readiness recorded
`/usr/bin/as` and `/usr/bin/ld`; the accepted live tool probe selected GCC's
`/usr/bin/x86_64-linux-gnu-as` and `/usr/bin/x86_64-linux-gnu-ld`. Their
resolved paths and hashes agree, but the full origin dictionaries correctly
rejected the different raw spellings. Readiness and build used different
identity procedures. This is a harness failure and earns no build/test credit.

The batch stopped at 21:07:47.646257 UTC. Post-gate cleanup acquired the build
lock and verified an empty target PID set. `build/debug/Makefile` and `python`
are absent; source remains clean at b34931a881b. The initial comparison was
outside the build receipt's `try/finally`, so this failure has no build-result
receipt. The gate traceback, launch result, batch failure and cleanup remain.

| Receipt | SHA-256 |
| --- | --- |
| `batch-start.json` | `986484b900b59b239f90929018473dca8c6364a97e7126341a98f36093922440` |
| `batch-failure.json` | `e71bf5726a24a8bc4ef8f19a589088e8f272e378fb84d32cd311b459fda86c6d` |
| `debug-build.launch-result.json` | `56b05f14552f5cde5656363262a32dbb9b62377e2c1cddd253948727b09a9c99` |
| `debug-build.post-gate-cleanup.json` | `1a95e9246e7b941b9f42e3b348219e03ff7936c74f312a8cb1db2c1e802825e8` |
| `debug-build.gate.log` | `f41d6cce29a2b3645e47833b582e3e87e95be7bcd22d5c6b1da801424d03fc77` |

At 21:08 UTC the coordinator authorized a fifteen-minute helper correction,
with no runtime edits or retry: use the same compiler-selected tool-origin
procedure for readiness and build, keep exact origin equality, and serialize
the initial comparison through the existing failure receipt. Prepare a new
held evidence directory and retain the original absolute deadline. Independent
correction review and fresh root activation precede any remaining-only retry.
