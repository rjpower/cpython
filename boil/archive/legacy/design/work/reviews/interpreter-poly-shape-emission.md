# Polymorphic shape matching in the frozen O3 image

2026-10-30. Read-only inspection of candidate source
`33944d24e551829a06a3c84c3c5e2c38d6acb463`, ordinary O3 native capsule
`17efeeac9f860afe7928b42453150144f353e545`. The combined qualification ladder
was still running. No target interpreter, benchmark, profiler or compiler ran for this
review; host tools inspected existing objects. This document's topic
HEAD is not the candidate's source or native identity.

The duplicate search **partly survives GCC O3**. GCC shares the live shape,
version and poly-table address, and eliminates the second search on a first-row
hit. Later-row hits still execute redundant comparisons in both LOAD and STORE.
This establishes emitted redundancy, not its frequency or performance cost.

## Source and emitted paths

`Python/bytecodes.c:2672–2712,2933–2964` invokes `_PyFb_ShapeRow` in the
polymorphic guard and again in each payload operation. The generated handlers
are `Python/generated_ir_cases.c.h:13193–13251,17328–17392`.
`Include/internal/pycore_interpframe.h:60–70` searches four rows by shape pointer
and the live shape's version. In this GIL build the FT atomic version read is an
ordinary load (`pycore_pyatomic_ft_wrappers.h:36`).

The inspected function is `_PyEval_EvalFrameDefault`: object address `0x6090`,
linked ELF address `0x44f010`, linked size `0x22d5c`. Its current O3 product and
object hashes match the original 588-product build map before and after the
inspection. There was no relink or new image.

| Path | Linked ELF anchors | What survives |
| --- | --- | --- |
| LOAD admission | `0x453e6a–0x453e8f` | Shape-row pointer comparisons; successful branches check cached row versions. |
| LOAD row0 success | `0x462440–0x46245f` | Checks version0, sets row0 directly, then joins payload at `0x4622bc`; no second search. |
| LOAD row1 success | `0x462409`, then `0x46242a` | After version1 succeeds, compares row0's shape again; a conditional version0 retest also remains emitted. |
| LOAD row2/3 success | `0x462366` / `0x46226f`, then `0x462290–0x4622b6` | Re-enters a shared row-search sequence; version branches continue at `0x464b9c–0x464be2`. |
| STORE admission | `0x45bc06–0x45bc2b` | Same four-row pointer search and version branches. |
| STORE row0 success | `0x462938–0x462956` | Uses row0 directly, then checks kind/payload. |
| STORE later successes | `0x46291e`, `0x462812–0x462838` | Repeated earlier-row comparison or shared second search before kind/payload. |

The first-row fast path is already optimized. It would be wrong to describe
this as two complete searches on every hit, or to charge duplicated table/header
loads that GCC has removed. The surviving later-row comparisons are visible in
the linked image, not merely inferred from two inline C calls.

Evidence is retained in PRIMARY
`build/gc-shape-row-emission/review-evidence.json`, SHA256
`6647e8fc5308e0861472d3d8e6ea6e2ca61ded7ecbc21753ed1cabd323078bad`.
It binds source files, actual build receipt/product map, object and ELF hashes,
the 2.99 MB object listing and six small linked-image excerpts. ELF SHA256 is
`3493f33bfa04102d5c3143abcc635952739e9bc2c6497aaee6de2a68908078cc`;
ceval object SHA256 is
`f25b37a3e41c9929f22587788c74c28352111e8b9c970bc5eccd9d73faca9c2b`.

## Smallest prospective shared contract

If refreshed profiles select this cost, match once and consume that integer row
within the same LOAD/STORE instruction: receiver admission and live-version
matching precede payload access; the existing matcher yields row0–3 or miss;
the load, method or store uses that result without searching again. Keep the
existing cache layout and opcode family. No new opcode, registry, cross-word
row cache or micro-specific specialization is needed.

A narrow implementation should keep the row local to the payload operation,
with a checked miss before any payload side effect, rather than adding an
interpreter stack value for it. Preserve the pointer-receiver guard: simply
removing `_GUARD_SHAPE_POLY` is insufficient, since
`Tools/cases_generator/ir_generator.py:1707–1730` uses a guard's object fact to
avoid boxing an unsupported receiver in the payload. A shared admission/match
preamble must preserve that property, generic deoptimization and hit/miss
statistics. Keep store-kind checks, transition ordering, method/self layout,
input consumption and result rooting unchanged.

Cached shape pointers stay address comparisons only. The match must still use
the live owner's version so address reuse and invalidation cannot apply stale
payloads. The row and borrowed feedback pointer must not survive a callback,
safepoint or instruction boundary. Existing `test_shapes` polymorphism,
address-reuse, class-mutation, method/data, transition and slot-kind cases are
behavior controls for any later patch.

No hit distribution, branch frequency or current CPU share was measured here.
Sites dominated by row0 may benefit little; code-layout effects can also matter.
Use the already planned post-LAND profiles and existing workloads to decide
whether this small family-wide simplification deserves an implementation.
