# Native prefix source review

**ACCEPT** `67f5d22bb386e362571e9555c08073d1bf79f6ea`, parent `30a0f5ab1bd02128495c5a0fe03128a417375460`, for separately reviewed focused native feedback. All six changed files were read; no remaining critical source blocker was found.

The production C arithmetic loop is replaced by the compiled leaf. Six caller-saved registers, operand pinning, duplicate operands, noncommutative SUB handling, last-use extension and spill/output slot separation are coherent. MOV/ADD/SUB/IMUL directions and REX bits preserve 32-bit signed arithmetic; immediate JO and TEST/JS before SHR reach the common decline. Fixed buffer/label/fixup bounds fail closed. ENDBR64, RET and the selected register set preserve the declared indirect-call ABI without changing RSP or callee-saved state.

Plan indices, initialization and live-out metadata are checked before emission. The first-member owner preserves scalar Plan views. Allocation, emission, mapping and protection failures free/unmap partial ownership exactly once; address-zero mappings are explicitly rejected and unmapped. Publication occurs only after RX protection, with existing BUILDING/state and full post-cold guards retained. Clear detaches before free; SizeOf includes the owner and rounded mapping.

The direct fixture compiles/runs/frees an actual temporary owner before boxing and never reads failed outputs. Code arguments retain attached owner metadata while it is copied. Pressure bytes are copied before free and their address is explicitly historical. The same twelve methods now cover real register pressure/spills, scratch/input bounds, malformed Plans, four cold failure stages, automatic success/mapping bytes and mapping lifetime. Other test classes and the selected roster are unchanged.

Independent exact-commit/hash/AST checks passed; retained author checks are lexical/source-only. No C compiler, target, emitted code or performance test was invoked by the reviewer. Profiler publication and leaf CFI remain explicitly deferred under the accepted no-call/RSP-preserving experiment. Actual byte/control-flow/ABI/W→RX/lifetime feedback and the fixed elapsed decision remain required; this is no native-success or speedup claim.

Receipt: `build/jit-prefix-native-source-review-20261101-v1/receipt.json`.
