# Option 1 handwritten-source post-review

The handwritten patch in `BASE/worktrees/frame54-recipient` matches the
approved descriptor contract. No blocking source finding. This is not release
compile, runtime or performance evidence; root owns those remaining gates and
the generated-product review.

Field-name search covers the complete port: initialization/copy/sentinels,
trampoline, call and resume producers, result publication, debugger discard
comparison, generator yield-from index subtraction, debug offsets and remote
asyncio. The only remaining raw field transfers are intentional descriptor
copy, explicit sentinel construction or remote byte loading before validation.
Remote readers reject reserved bits and tagged discard before numeric indexing.

Publication rereads the descriptor after `_PyFrame_CompleteRedirect`; replace
commits the actual result before closing the displaced owner; empty has a debug
NULL assertion and no old-home read. Locals/cells/free variables are always
encoded replace-required through the current execution-prefix local boundary.
IR_NO_DST stays untagged4095 and cannot be mistaken for replace home4095.
Independent code/function owners and NULL-on-consume contracts remain intact.

The new behavioral tests cover callback mutation of a replaced ordinary result,
send result local mutation, and nested yield-from/native resume inspection.
Remote tests change descriptors only while suspended and restore them before
resuming, checking numeric decode without using a fabricated proof to publish.
Existing generator-string check was updated for the new producer spelling.
Root must still execute actual index-boundary, pending redirect, ordinary/
generator exception, observer and remote test gates on both builds, and inspect
producer-added prefix/classification and publication branches in native code.
No throughput claim follows from this source review.
