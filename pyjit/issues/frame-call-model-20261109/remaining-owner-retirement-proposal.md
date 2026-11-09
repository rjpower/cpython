# Remaining owner retirement: source census and bounded next steps

This is a source proposal after native15, not an admission or an instruction
count receipt. Semantic FOR_ITER/SEND/CLEANUP_THROW completion is implemented;
standalone retirement words still exist on other paths. No performance benefit
is attributed to the following changes before measurement.

## Producers and current limits

| Source evidence in runtime worktree | Ownership case | Why normal completion fusion does not cover it |
| --- | --- | --- |
| Python/codegen.c:616–625 | break/return exits a synchronous or asynchronous loop; preserve_tos may move a return value past anchors | This is an explicit non-exhaustion exit. FOR_ITER's exhausted edge never ran. |
| Python/codegen.c:4212 | optimized builtin guard succeeds and discards the original callable | The callable is shared with the fallback path; it is not a unique dead producer. |
| Python/codegen.c:4239 | any/all consumes a decisive item and leaves the iterator early | The predicate branch falls out before the exhausted edge. |
| Python/codegen.c:655,705 | a finally return overrides a pending exception or pending return owner | The owner must survive the finally body until a semantic override or exit. |
| Python/codegen.c:2713,2910–2938 | exception matching and except-star paths discard branch-specific owners | Exception/lasti values and active exception state are distinct owners. POP_EXCEPT remains a semantic state change. |
| Python/codegen.c:5147 | failed inlined comprehension discards partial result before restoring enclosing locals | Restoration and exception routing must survive; omitting cleanup cannot skip or reorder the restoration. |
| Python/codegen.c:196–199 | pattern-match failure chains discard copied/shared subjects | Different failure paths own different subject prefixes. |
| Python/regcfg_passes.c:635–670 | producer sink lowering | Only a fresh, unique single output, no other explicit or implicit read, no preassigned destination, and a DISCARDS-capable producer qualifies. Canonical/shared owners are deliberately excluded. |
| Python/assemble.c:1137–1230 | allocated completion fusion | Only positively matched FOR_ITER/END_FOR/POP_ITER and SEND/CLEANUP_THROW/END_SEND paths are removed. Unrelated release words cannot inherit those outcomes. |

Other source POP_TOP sites are not proof that a word survives allocation. Pure
expressions, unused call results and unused resume inputs already have lowering
paths. A post-lowering corpus inventory of real handlers is needed before
claiming an exhaustive surviving-word count. Source comments describing CLEAR
or retained NOP events are historical and should not define the new contract.

## Proposed order

First handle real loop transfers. A break or decisive any/all branch must
retire its active iterator/cursor as part of that semantic edge. Continue
preserves the active pair; ordinary exhaustion retains its existing completion
outcome. Nested exits identify the exact anchor ranges being left. The edge
proof must cover item/result destinations, outer loop anchors, callback errors,
source events, and relaxation. There is no separate cleanup dispatch and no
ordinary metadata walk. Operand space is a real design constraint: condition,
feedback and target already occupy a branch word. A wider transfer encoding or
a bounded fixed exit descriptor needs review before choosing an implementation;
merely naming a release operation EXIT would not meet the requirement.

Next remove cleanup immediately subsumed by RETURN or another real state
transition. Frame retirement already owns all surviving frame homes. Python does
not require historical CPython decref locations, but retaining an owner must not
make the next producer overwrite a non-NULL scratch cell. A finally body can
still access locals and invoke observers; its retained pending return/exception
must be modelled separately from active exception state. For exception matching
and comprehension restoration, attach retirement to the actual consuming match,
restore or exception-state operation, with the correct exceptional outcome.

Shared producers that cannot be consumed at a real edge need a separately
reviewed retained-owner model. A bounded frame-owned range could retain owners
until a real overwrite or frame exit; it cannot be implemented by leaving values
in ordinary scratch. Current scratch publication assumes NULL, and arena reuse
also relies on cleared inactive cells. Such a model must define overwrite,
traversal, frame escape/copy, unwind, debugger visibility and loop reuse, without
fake Python locals, unbounded per-iteration accumulation or a new dispatch tax.
This is a fallback design question, not authorization to add that range now.

## Required evidence for the next implementation proposal

Obtain real post-lowering surviving words across ordinary loops, nested
break/continue, return through finally/with, any/all fast and fallback routes,
pattern failure, except-star and comprehension failures. For each proposed
fusion, show the exact normal and exceptional owner sets before and after the
real operation. Test observed and unobserved paths, source-only callback errors,
last-owner reentry, suspended generators and far targets. Keep the current
constructor proof and independently verify the new physical outcomes. No
blanket POP_TOP/POP_ITER ban is justified until every valid frontend producer
has a real ownership transition.
