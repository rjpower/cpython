# Independent registered-site and linked-path outcome

2026-11-01. **ACCEPT the registered-site nomination and linked mechanism for
early net candidate/parent elapsed screening.** Root confirmed the actual
session terminal with exit 0. Completion was 04:44:06.989815 UTC, before the
04:53:53.538782 deadline. All 66 completion artifact bindings match; child and
row verification passed, post-gate PID set was empty, and both image locks and
actual idle state were independently checked. No new target, compiler or ELF
tool command ran during this review.

Evidence: `build/region-receiving-witness-20261101-v1`. Completion SHA256
`cb9c2aa2be0e0d52900fb4a6e67de0f3d366b7867da0c3bace0ac0ed022ae91b`.
Candidate remains clean `40498d1e0d3773ac2a1180eb8cac7fd04831028d`, release image
`e20e982ec814b8842929b600690cd615c7d1d49f5b582a570d545ab65c53e70b`.
The matched qualified parent is e9 image `289cf16e…`, with identical pyconfig,
toolchain and whole Makefile after source/build-path normalization; the scoped
runtime source bridge to candidate parent 5c is empty.

The sole child, PID 1604941, called the unchanged registered `run(4)` exactly
once. Its three actual code objects retained identity, source, constants, local
kinds, line and exception tables. Fibonacci's comparison at word 4 changed
from pristine region 302 to executing region 303; word 5 retained the BOOL
branch, shared temporary 1 and forward target 8. Both origin records bind actual
image/CAPI/module files and maps, OFF mode, stress/seed 0, absent trace/profile,
callbacks and monitoring. Actual-PID roots show 16 collections, fallback 0 and
observed unpinned 0. This is a warmed encoding nomination, not an instruction
frequency measurement.

The 14 captured ELF/disassembly commands preserved both complete evaluators,
their symbol/relocation evidence and supplementary entry prefixes. Each image
has one named nonzero evaluator range in the saved symbol inventory; no named
cold partition is missing. The following linked paths were inspected in the
complete captures, including outlined continuations:

| Path | Actual addresses and observed work |
| --- | --- |
| Candidate admitted region | Entry 454092 → admission 468593 → arithmetic 46861f (inline continuation 475974) → branch/cleanup 46871b–468817. False branch 472a6c rejoins cleanup. |
| Candidate rejected admission | Every admission rejection rejoins 4540f3; ordinary integer result materializes at 454278 and dispatches at 45428a. Inline continuation 465084 rejoins this body. |
| Candidate ordinary BOOL receiver | 460a32–460b2e; false-target continuation 466fdb returns to cleanup. The retained independent word still loads, checks and clears its real home. |
| Parent ordinary pair | 451c73–451e05, including inline continuations 46100f/46e939, followed by BOOL receiver 458220–4582e1; false-target continuation 462c32 rejoins cleanup. |

On the admitted candidate path the bool remains a register value through branch
selection and history update. There is no intermediate bool-home store, reload
or clear, and no second handler dispatch. The two original frame/IP publications
remain at **4540ab** and **468758**. Input-home cleanup remains on both target
and fallthrough paths; NOT_TAKEN handling and feedback history remain. The
parent stores the bool at **451df1**, dispatches at **451e05**, reloads its home
at **458248**, clears it at **4582c8** and dispatches again at **4582e1**. These
are managed-home operations, distinct from native stack spills.

Compact-int guard misses reach the ordinary generic comparison at 453b30.
Its specialization branch and backoff/rich-compare branch were followed through
453b9f and 4616e0–461781. The other ordinary generic body at 453bc5–453ce8 uses
the same rich-compare helper and materializes before receiving dispatch.
Optional truth conversion at 461e5c/466ef2 rejoins those ordinary exits;
comparison/truth errors reach 46f650/46fafe, clear appropriate inputs and enter
the existing common error path. A BOOL guard miss reaches the generic branch
body at 45aff2, preserving its truth/helper, feedback, target, home-clear and
dispatch behavior. These known helper/error boundaries and the already accepted
34-case debug/release semantic profile supply fallback evidence; this witness
does not claim to execute every linked edge.

There is a concrete package cost to measure. The candidate adds nine release
admission conditions before the existing integer guards: bounds, branch form,
shared home, temporary status, trace, profile, monitoring storage, frame object
and breaker/version. Its linked paths repeatedly reload frame, IP and slot bases
from native stack locations **-630/-628/-640(%rbp)**. In the matched parent
comparison/receiver, frame and IP instead remain in **r14/rbx**, with the slot
base loaded from -628. Thus removal of the bool home and dispatch does not
establish lower total interpreter work or elapsed time. The net comparison must
charge the guards, register-allocation/layout changes and receiving traffic.

Independent receipt: `build/region-receiving-outcome-review-20261101-v1/receipt.json`,
SHA256 `1da58b15f211fdd45472ab7a12e3c0c45aedd0b9682843e7e6dc4686fb725c7b`.
It binds the original full captures, exact path ranges, actual contract replay,
site records and idle images. **Fast/fallback frequency remains unmeasured.**
The snapshot does not sample active frame-object or breaker state at each
comparison. No isolated-home cost, speedup or broad qualification is claimed;
the held materialization ablation remains unbuilt. This verdict clears mechanism
interpretation for the separately reviewed elapsed pilot, whose activation and
accept/reject decision remain root-owned.
