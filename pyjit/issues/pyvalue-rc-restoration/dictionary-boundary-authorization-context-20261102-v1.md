# Direct user authorization and the retained dictionary proposal

The first execution of the new boundary-provider script was rejected before
source edits. Its justification cited the coordinator's source grant, not
the user's latest direct instruction. Preserve that rejection and the original
superseded removal-script rejection; neither is an executed source change.

The latest direct user instruction is:

> ideally we should design the extension API explicitly, like hpy or v8, with
> an opaque design moving forward. something where old extensions can be
> compiled with minor modifications and have a slow but workable option,
> and can then adopt the new apis gradually would be great. but in general
> hard compat is not required

The concrete proposal keeps public names/signatures but changes unsupported
legacy scalar borrowing to explicit failure. GetItem returns NULL/TypeError;
Boolean Next returns0/TypeError with NULL outputs and no cursor advance.
Unchecked old callers can mistake those results for absence/exhaustion and
must be ported. Borrowed SetDefault rejects before mutation. Existing owning
GetItemRef/SetDefaultRef and new owned NextEntryRef supply the slower workable
source migration. All affected core and bundled consumers must migrate before
qualification; there is no promise that unchanged extensions work universally.

Root interprets the quoted instruction as authorization for these explicit
source/API compatibility changes, rather than relying on the broader PyValue
restoration request. Independent design review1790b048 accepts51e56173, with
actual borrow lifetime proof and final exact source review still required.

One retry may present this new authorization evidence to the SAME approval
review path. It is not permission to run through another tool, evade review,
execute the old removal script, or alter the scope after rejection. If review
still rejects it, hold the compatibility-changing action and continue the
separate additive owning API/core migration work. Record the outcome exactly.

## Resolved by direct user instruction,2026-11-02 03:49 UTC

After disclosure of the exact GetItem/Next/SetDefault behavior and downstream
risk, the user instructed: "the whole goal of the project is a python rebuild
and we rebuild everything". The follow-up explicitly directs agent judgment
to make extension porting possible and incremental, and rejects compatibility
as a hard gate. This resolves the specific approval question. Implement the
reviewed boundary and port affected callers; no further compatibility approval
is required within this rebuild scope. Correctness, ownership and actual source
review remain required. Preserve the prior rejected actions as history.

The additive bridge is already implemented separately. Apply only the remaining
legacy delta against its frozen baseline; the earlier combined script is stale
and must not duplicate the new providers. The superseded API-removal script
remains unused because it does not implement the selected migration design.
