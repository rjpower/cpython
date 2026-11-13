# Provider handoff: GC protocol peer

2026-11-12 independent review of
[provider-handoff-proposal.md](provider-handoff-proposal.md), including its
independent code-pin and hook-return refinements. Design GO for the bounded
production consumers and tests, subject to root's final shared-schema approval.
No source implementation or runtime result is claimed here.

Explicit kinds distinguish external/prefix authority from retiring and staged
frames; no accidental cursor/header pattern identifies a stage. Agreed names
are `_Py_TRANSITION_KIND_EXTERNAL=0`, `_Py_TRANSITION_KIND_RETIRING=1` and
`_Py_TRANSITION_KIND_STAGED=2`. Packing result-present:1/kind:2 retains the
40-byte record and reason/status22/23. A live stage owns initialized child
arena homes until actual publication or retirement transfer.

The outer node survives a delegated default evaluator and later hook cleanup,
but disarms to frame=NULL and zero remaining/header/result/prefix authority at
the actual handoff. This prevents lookup/visits through a recycled arena
address. Keep this empty node in LIFO order until its owning C scope pops it;
an interior unlink protocol would add complexity without supplying authority.
The collector must explicitly validate and skip a NULL-frame STAGED node, and
continue visiting outer live nodes. The observation query returns every
`(kind,reason,status,has_frame)` tuple; all tests deliberately adopt that
contract. Logical owner exhaustion is not an empty physical chain.

Strict custom activate/retire helpers return int/error on a missing live token
before any frame dereference. Optional default entry handoff separately accepts
genuinely unstaged public/resumed frames. Activation publishes current-frame
authority before disarm and any callback. Retirement disarms then reaches M2
without a callback gap; the outer scope remains linked until retirement ends.
Default recursion failure uses its owner-specific close path, not a fabricated
thread-only ownership assumption.

Both real provider adapters own storage from binding through buffer cleanup,
hook execution and final disposition. A hook that returns without consuming
leaves a live stage, which the outer provider retires exactly once while
holding its native result/error under the selected policy. A hook that consumes
through the explicit helper leaves an empty node; the provider never probes
the former frame. Hook current-frame/reservation/scope restoration is an
explicit engine contract, checked against snapshots rather than guessed from
stale frame memory. Raw PopFrame/Copy cannot silently consume a stage.

Release the provider's independent C code pin while the live stage and child's
execution pin still protect geometry, before evaluator entry. Otherwise that
extra pin would postpone the actual final code close and its result-root
callback evidence until outside retirement. Preserve direct object versus value
result conventions instead of canonicalizing merely to share an adapter.

The native provider/default entry lookup is an actual new cold consumer behind
a nullable-head test; ordinary exact CALL/SEND and hot returns gain no lookup
or token signature. Price external entry frequency, nested lookup, provider
stack growth, record stores and ABI work. No global native owner chain is
introduced. Init shim failure, custom active-map authority and complete
callback/alias/phase coverage remain explicit open gates before scanner removal.

GC owns the shared kind/header/visitor/query and the single InitClearRoots kind
port; frames confirmed no competing frame.c edit. Frames owns provider/call/
ceval/helper/tests, root owns retirement reasons and integration, and IR owns
plan descriptors/tests. Actual before/inside/after-default, substitute/error,
independent retirement, recursion failure and nested arena tests must exercise
the real adapters and all-node observation contract before qualification.

Root approved the shared protocol and GC's bounded four-file source delta is
implemented over `6ef8868ac53`, frames source postreview GO. It adds explicit
kind bits/enum validation with existing 40-byte/offset assertions, the guarded
empty-STAGED visitor path, the retiring InitClearRoots kind and the all-node
four-tuple query. Prefix defaults remain external0. No provider behavior is
claimed until frames' actual stage producers/handoff consumers and test ports
are integrated. Diff checks pass; no native jobs were run in this lane.

Frames' five-file production/helper/fixture delta has independent source GO.
The shared lookup disarms exactly once, optional default entry accepts genuinely
unstaged frames, strict helpers reject missing/NULL tokens before dereferencing
old storage, and provider scopes retain empty nodes through post-default hooks.
Both object and value providers retain their existing result conventions.

The final review found and closed two helper/header issues. The explicit
`staged_running` guard now stays set through hook/configuration restoration and
all three final owner closes; its fixture reenters from a selected-code constant
finalizer and then proves a later fresh call succeeds. STAGED masks include
nullable LOCALS and FRAME_OBJECT unconditionally, so sanctioned hook-created
headers acquire current-field authority without another record or count. The
new installed-mapping fixture checks precise root IDs before publication and
their absence from the transition chain inside/after default execution.
RETIRING snapshot masks are unchanged. These are source dispositions; merged
generation, native tests and cost checks remain root's gate.
