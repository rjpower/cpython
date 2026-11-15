# Native Invoke post-review

Reviewed frozen composition `582e61c2b28445cadbf219df23f708d6e5988e40`
against selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`.
Disposition: source and retained native mechanism GO; selection remains open.
No must-fix finding. This review ran no interpreter, generation, build or test.
Root reports both builds at 117/79 and 66 focused methods per role passing.

## Source and ownership

The checked `NativeInvokePlan` ties the actual producer, sole scalar output,
input roles and ordered retirement together. Generation rejects a missing,
duplicated, reordered or shadowed acquisition/retirement. The reached owning
provider result and explicit NULL error exit supply the result receipt;
the plan's spelling alone does not. Missing emission receipts fail generation.

The provider remains `_PyValue_VectorcallTstate`. On its NULL result, the
generated handler enters the original error protocol before any input is
detached or result published. On success, it snapshots the **current** B0,
publishes the result into B0, closes the displaced callable, takes/closes
arguments in descending order, and takes/closes an actually present self.
There is no preliminary B0 take/NULL store. Exact-number normalization is
callback-free: the original home anchors the callable until overwrite, which
transfers its ownership to the snapshot before the close. This reasoning does
not rely on a native register being a GC root.

B0 remains the result root for local, scratch and discard recipients throughout
input finalizers and the unchanged periodic check. The existing final A move
and delayed local displacement remain. The parent reservation is not shortened
while inputs retire. The generator kills the input Locals, records B0 as the
result home, and marks the checked window drained; scalar home identity stays
unknown.

Root's retirement `RELEASES` registration preserves callback effects. The
generated IR format rows for both FAST handlers are unchanged from selected57:
raising, escape, Python execution, deopt, polling and allocation remain;
normal transfer stays UNKNOWN and error routes stay zero. The opcode metadata
adds synchronization, and the explicit tier-1 producer removes its unsupported
tier-2 expansion and uop ID. No new read, error or normal-home proof is granted.
The Invoke generated delta contains only the two positional FAST forms;
keyword forms keep their existing path.

## Retained native comparison

Evidence is `E59/combined3-native/` (manifest source `582e61c2b28`, ELF SHA256
`523243d8436d559acba1f5553c27e9a46e8adc84d2d421095bbfb0065e52219e`)
and `E59/invoke-selected57-{fast,noself-fast}.txt`, checked against the retained
selected57 ELF. Addresses below belong to those artifacts.

| Handler | Selected57 bytes | Composition bytes | Stack allocation |
| --- | ---: | ---: | --- |
| `CALL_METHOD_DESCRIPTOR_FAST` | 1,697 | 1,553 | 64 → 48 bytes |
| `CALL_NO_SELF_METHOD_DESCRIPTOR_FAST` | 1,446 | 1,473 | 48 → 48 bytes |

The FAST selected57 take/close loop at `390c89..390cb5` is followed by a
second NULL-only traversal at `390cf0..390d0a`. The composition has the single
descending take/close loop at `3914a9..3914cb`; the second traversal is gone.
The NO_SELF selected57 loop at `391949..39196c` is followed by `memset` at
`39198d`. The composition loop at `391b29..391b4c` has no following `memset`.
The required first NULL write for each argument remains.

The new B0 snapshots are at `3913d8` and `391a6d`; replacement stores are at
`391430` and `391ad0`. No B0 NULL store intervenes. Later B0 NULL stores belong
to the unchanged final recipient move/discard and are still required. The
FAST indirect provider call now saves/restores the interpreter-frame pointer once
(`3915f8`, `391607`), versus frame and self-state saves/restores around the
selected57 call at `390ea1`. NO_SELF retains two such save/restore pairs around
its provider call. There is no additional helper hop or larger stack frame.
These observations do not imply that every spill or branch disappears:
normalization, generic provider dispatch, result checks, possible deallocation,
periodic handling and final recipient classification remain. NO_SELF grows by
27 bytes despite removing the repeated clear.

## Fixtures and limits

The source fixtures check both FAST forms and both record modes, provider NULL
before any take/publication, exactly one acquisition and ordered retirement,
no B0 take, preserved periodic/final move, and rejection of altered plans,
shadowed operations or missing actual receipts. The runtime fixtures retain
all public image routes, exact warmed handler assertions, result/input aliases,
provider failure, finalizer mutation/GC before local commit, and monitoring
enabled during the provider.

The corrected NO_SELF fixture binds `dict.get` as module-global `_dict_get`
and calls it with an explicit mapping. Its exact NO_SELF assertion remains;
the earlier `dict.get(...)` attribute expression legitimately selected the
CLASS optional-self form. Actual fixture SHA256 is
`4a807193cf2580a7e8d864126b9db351986092d75978d0733e1ba477ada5db29`.

This is an ordered-retirement improvement, not a checked direct native method
ABI or removal of the generic vectorcall bridge. Native mechanism evidence
does not select the composition: it also includes the independently reviewed
builder trial, and root's serialized measurements decide its outcome.
