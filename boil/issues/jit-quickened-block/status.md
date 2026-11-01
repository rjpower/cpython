# Selected IR block entry

**Design accepted; exact source cut under review. No implementation or execution grant.**
All three global frame-entry native experiments are rejected. Main runs the
accepted RC images; source11aefbdd and its measured loss stay frozen.

One bounded proposal removes the frame-entry hooks and routes a selected
executing IR arithmetic head through a BINARY_OP-family handler, installed by
existing cold specialization. It retains a known multi-operation body while
avoiding explicit entry checks in unrelated frame paths. Region length and loop
compilation remain separate future questions; no benefit is promised.

The [design](design.md) and [peer review](design-review-20261101-v1.md) are frozen.
The accepted contract requires deterministic one-site-per-code ownership/counting,
original-head PC/SP publication before cold work, exact live-in/materialization
and NULL-temp requirements, original-ADD fallback once, pristine opcode mapping,
observer invalidation and owner retirement. Interruptible backedge periodic
checks and await/yield behavior must remain intact. Existing TryEntry's
word-one/RESUME contract does not cover an arbitrary interior block.

restart_audit designs; coroutine_design_review reviews. Deliver one concrete
source cut and early rejection conditions before any separate finite source
grant. The later gate order is focused behavior and actual emitted-path checks,
then one fixed all29 comparison; publication/fullLib only after useful elapsed.
Do not reopen old filters or tracing GC, add runtime selectors, infer dynamic
counts from source, or introduce text/RSS-only performance vetoes.

Peer review closed one deterministic lowest eligible head per code, site-keyed
ownership/counting, relative region bounds, original-head PC/SP publication and
integer-only output mapping. Loop compilation is deferred. Exact file/metadata
and handler-ID/cache closure must precede the separate bounded source grant.
The old frame filter and its obsolete fixture interface must be removed when
replacing the entry path. The unscored witness needs the new block contract;
scored commands, workload exposure and numerical rules remain unchanged.
