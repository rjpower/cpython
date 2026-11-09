# Final39: priorities after the supplemental measurements

This updates the priorities in the earlier
[cold architectural review](closing38-cold-architecture-review.md), rather than
holding its frame-focused ranking fixed. It is a read-only assessment of final39
and unchanged workloads. No new source packet or implementation is proposed for
this closing round. Root's upcoming profiles must decide the remaining causal
questions. The V8/LuaJIT references in the earlier review are analogies only.

## What the current evidence establishes

I independently recomputed these ratios from
`E11/closing39-supplemental-v2/*-micro-instr.json`. The status says six sweeps
completed with stable identities. These are differenced instructions per
iteration, not elapsed ratios or percentages of whole-program cost.

| Micro workload | Net instructions / upstream | Raw instructions / upstream | Net / golden |
| --- | ---: | ---: | ---: |
| class_attr_load | 8.829597 | 4.539366 | 1.012527 |
| generator_resume | 2.801176 | 2.256941 | 0.949404 |
| dict_subscript | 2.392067 | 2.042866 | 1.004010 |
| cell_contents | 2.250400 | 2.028186 | 1.009728 |
| call_py_kw | 2.092427 | 1.985451 | 0.997764 |
| str_join | 2.030916 | 1.791363 | 1.008922 |

The empty-loop skeleton changes denominators substantially. In particular,
8.83x net instructions is not an 8.83x runtime slowdown. Most gaps above were
already present in golden; generator resume improves against golden. Neither
the new call/frame packet nor its debugger proofs explain them all.

Root's current upstream elapsed screen also separates workloads: DeltaBlue
and generators regress, while nbody is faster. That makes another nbody-only
layout experiment a poor first priority. Final conclusions should use the
retained current stat/record campaigns, not old helper sample percentages.

## Revised five structural priorities

1. **Restore an efficient guarded path for stable class/object lookup.**
   The first diagnostic is class_attr_load: an unchanged class and attribute,
   not a mutating descriptor stress test. Audit its actual warm handler and
   fallback path before changing code. The intended fix is a shared
   version/descriptor/provider contract that permits a direct value read when
   Python lookup semantics allow it, with a complete fallback for metaclass,
   descriptor and custom-hook changes. Dict subscripting belongs to the same
   investigation into guards and generic-reader detours, not the same assumed
   implementation. Its current specialized body already has
   `IR_TRY_DICT_FIRST_PROBE`; establish whether that path hits before adding one.
   Benefit: remove a large common lookup tax and potentially address DeltaBlue.
   Risk: invalidation, MRO/metaclass dependencies and cached-owner lifetime.
   Gate: current hot-path/guard-hit evidence, mutation/descriptor/weakref controls,
   then instruction and elapsed improvements on the microcases and DeltaBlue.

2. **Remove avoidable object-form bridges at genuinely value-native boundaries.**
   The cell witness stores 3000, so its exact scalar is inline. The actual
   `cell_get_contents` object getter obtains a PyValue, boxes via
   `_PyValue_AsObjectAt`, then closes that value; a later owning value boundary
   canonicalizes the exact numeric box again. This is a concrete source cost
   candidate, subject to confirming the warm lookup route. A value-native getset
   path could preserve an owned canonical value while keeping the public
   object getter's owned-box contract. Do not cast arbitrary getset callbacks
   or weaken their error/lifetime rules. The str_join microcase additionally
   performs 1000 list.append calls per amortized join: its name does not locate
   the cost in Unicode joining. Audit append/method dispatch before blaming the
   final join's iterable bridge. Gate: actual boxing/call-path counts, empty
   cell and descriptor errors, mutable numeric/subclass inputs, owner reentry,
   then the unchanged full workloads. No borrowed box cache is needed.

3. **Simplify generator delivery and recursive delegation as one transaction.**
   Resume still has a clear upstream instruction gap, but has improved against
   golden. Keep one recipient/publication/retirement implementation with derived
   producer facts, and account for entry plus return, not just the sampled
   publisher. The tree-generators workload includes recursive delegation and
   attribute reads; the simple resume microcase has an inner range step too.
   Risk: multiplying recipient flags, code size and per-resume classification
   can offset an old-owner-load saving. Gate: current profile split among
   lookup, SEND/FOR, YIELD, completion and frame creation; total text/spills;
   then matched generator/coroutine elapsed and counters with all owner/error
   and pending-redirect controls. Direct heap construction stays a separate,
   larger experiment until creations are demonstrated to matter.

4. **Extend known binding without duplicating keyword-call semantics.**
   call_py_kw repeatedly calls one function with one fixed three-name tuple.
   Actual CALL_KW_PY still enters the generic `IR_INVOKE_CALL` provider; the
   existing fast known binding work targets positional shapes. A versioned
   keyword-shape/permutation plan could reuse the same consuming binder and
   frame ABI instead of repeatedly rediscovering argument placement. Benefit:
   improve a measured call class rather than assume ordinary CALL dominates.
   Risk: defaults, rebinding, duplicate keywords, arbitrary mapping providers
   and partially bound child cleanup. Gate: shape-hit/argument-move evidence,
   valid/error/default/observer cases, then unchanged keyword-call and broader
   call-heavy elapsed/counter comparisons. No per-frame keyword flag is implied.

5. **Make semantic consumption and producer facts structural in lowering.**
   Shared source-derived output facts have already avoided redundant numeric
   publication. Remaining cleanup words and general result handling should be
   evaluated by dynamic counts, not removed by assuming frame clear owns all
   scratch. Attach retirement to real semantic transitions only with explicit
   alias/error/observer transfers. Derive the corresponding ownership contract
   from the same emitter objects, so constructor and cold analyses do not grow
   independent opcode lists. Benefit: less dispatch/publication work and less
   duplicated semantic knowledge. Risk: emitter complexity, retained owners,
   reentry ordering and text growth. Gate: one bounded operation class with
   exact generated/compiled comparison, malformed/alias mutation tests,
   eventual collection and finalizer controls, then measured workload impact.

These priorities do not authorize new runtime work now. Caching cold debugger
proofs, broadening safe jumps and exact tooling ports are implementation-quality
or capability follow-ups, not explanations of these normal execution gaps.
GDB formatting and CPython-internal transcript/disassembly identity remain
separate from Python semantics and throughput. The nine reported ordinary
safe-jump gaps stay visible; H40 and the archived owner draft remain deferred.
