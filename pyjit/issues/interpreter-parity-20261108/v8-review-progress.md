# V8 interpreter review progress

2026-11-08 09:07 UTC. Scope: independent source/design review only; runtime,
builds, benchmarks and other experiment owners' files remain read-only.

- Read repository guidance, CPython AI policy, project process and current
  interpreter/call contracts and report. Runtime inspected at `0953757f4e7`.
- Inspected register CFG conversion, compaction and copy propagation; actual
  call handlers, transaction helpers and overlapping frame initialization;
  retained R5 GCC overlap disassembly and final workload/profile tables.
- Pinned primary V8 source to `63481c19453c72221531058be6d2e94adf3ebc66`
  using `git ls-remote`; inspected Ignition handlers, assembler, register
  optimizer, bytecodes and x64 interpreter entry. Historical V8 design posts
  are dated separately. Ignition baseline words are distinct from optimizing
  SSA and native machine-register allocation.
- Initial target: retain `logging_silent`, whose known instruction gap and
  call/frame profiles give a stronger starting point than switching targets.
  Its hot `Logger.debug` signature includes `**kwargs`, so the required fresh
  kwargs dictionary makes a no-allocation-only call shortcut insufficient.
- Concrete implementation lead: exact code homes still pass through generic
  scalar canonicalization in frame initialization. Broader call design will
  retain post-allocation guards and once-only commit/publication.

2026-11-08 09:13 UTC. First useful packet complete in `v8-review.md`: eight
ranked recommendations and a concrete specialized common-call prepare/guard/
shared-commit proposal. Reviewed root's six-round/two-campaign parity plan;
sent estimator/order/native-affinity clarifications to root. Read root's new
warmed-IR capture: ten outer general Python call sites and the exact inner
call confirm that the nonallocating overlap shortcut misses this target's
hot calls. An offline retained-ELF disassembly also confirms duplicate
successful refresh validation survives GCC. Root received that bounded lead.

2026-11-08 09:14 UTC. Completed source/citation and artifact review; explicit
no-index whitespace checks pass for both new files. Verified public
`PyFunction_SetVectorcall` clears function version, supporting the limited
pre-escape provider fact. The report preserves all post-allocation guards,
once-only ownership transfer, canonical homes and observation/error behavior.
Handed first packet to root. No native code, build, test or benchmark was run
by this reviewer; no performance outcome is claimed.
