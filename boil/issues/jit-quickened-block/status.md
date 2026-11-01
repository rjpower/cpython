# Selected IR block entry

**Superseded as a production-tier proposal; source retained as component evidence.**
Main retains the accepted RC images. All three frame-entry native experiments
remain rejected; none was integrated. Tracing-GC work is closed.

The hypothesis is that entering a native arithmetic block through its own
quickened instruction can retain arithmetic gains without charging unrelated
frame entries. The [design](design.md), [design review](design-review-20261101-v1.md)
and [source scope](source-manifest-20261101-v1.md) describe one selected
BINARY_OP-family head per code object. Native loops remain deferred.

The isolated source is frozen clean at `a4b0bf358b4cae18f6abefb7af2ec50a7c3ac612`.
Its 18 changed paths remove both canonical/all 14 generated frame-entry hooks,
reuse the execution-header layout and native arithmetic compiler, and implement
interior-PC, live-in, fallback, ownership and observer contracts. The known block
starts at word 2, resumes at 12, and commits one integer output while preserving
the already materialized float.

The [early checkpoint](source-checkpoint-review-20261101-v1.md) and
[exact source review](source-review-20261101-v1.md) passed. Existing opcode IDs,
formats and dispatch indices remain stable except for filling unused slot 127.
Review caught and closed a marshal-validation bypass and feedback kind/argument
gaps. All six generators/seven products and 26 host generator tests pass. The
12 JIT method IDs and 54-case focused roster are unchanged.

The [45-minute source grant](source-grant-20261101-v1.json) closed complete ahead
of its cap; the [disposition](source-disposition-20261101-v1.json) binds the frozen
source and independent receipt. No source edits or integration are authorized.

Next gates, in order:

1. Separately admit fresh release/debug builds and the existing 54-case cohort.
2. Review actual selected-head entry, fallback, ABI, output mapping and RX owner.
3. Run one fixed all-29 comparison with unchanged scored commands and rules.
4. Admit publication and broad qualification only after useful aggregate elapsed.

The build controller and unscored timing witness are prepared and held. The
witness uses the interior single-output contract and must observe natural
activation before introspection. No runtime or performance result exists for
this candidate. There is no repeated four-workload pilot, arbitrary text/RSS
veto, collector selector or reopening of rejected frame filters.

Owners: restart_audit implemented and prepares emission evidence;
coroutine_design_review reviews independently; qualification_review_resumed
prepares held feedback; the coordinator admits gates and records decisions.

At user feedback 36, the coordinator redirected work to a cohesive
[method and tracing JIT system](../jit-e2e-system/status.md). The interrupted
activation did not take effect: activation/start records are absent and both
build directories contain only their locks. The empty owned cache was removed.
No target ran and no performance rejection is claimed. The source acceptance,
reviews and prepared helpers remain historical component evidence. All further
build/timing gates above are held; [scope disposition](scope-disposition-20261101-v1.json)
owns the change. No competing runtime tier or optional switch was integrated.
