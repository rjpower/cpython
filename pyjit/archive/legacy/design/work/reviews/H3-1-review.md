# H3-1 design review, round 1

Reviewer: `/root/compiler_review`, gpt-6.1-sol, xhigh reasoning.
Date: 2026-10-29. Read-only source/artifact review; no tests, builds or edits.
Inventory source is unchanged between `99435658509` and `e02e177903f`.
Coordinator preserved the review below; source paths refer to that snapshot.

**REVISE.** The domain architecture is sound, but three contracts need
decisions before the complete rung is approved.

## 1. The schema identity must come from the sampled process

H3-1 requires a capture fingerprint but leaves its transport and provenance
unspecified. Current attachment checks Python version, threading mode and
structural layout (`Modules/_remote_debugging/module.c:212`). The target
exports no handler fingerprint in `_Py_DebugOffsets`
(`Include/internal/pycore_debug_offsets.h:63`). Names come from the sampling
process's opcode module, and the binary writer stamps its own build
metadata (`Modules/_remote_debugging/binary_io_writer.c:1241`).

Counterexample: builds A and B have identical PY_VERSION_HEX and layouts
but different handler numbering. A attaches to B successfully, labels B's
IDs using A's table, and could write A's fingerprint. Subsequent file
validation would accept the incorrect interpretation.

Specify immutable target-side publication, transport through the remote
interface, and comparison with the actual target before opcode
interpretation. Define the remote layout/size compatibility change: the
reader copies sizeof(_Py_DebugOffsets) (`Python/remote_debug.h:1671`), and
validation pins its size
(`Modules/_remote_debugging/debug_offsets_validation.h:33`). The writer
must receive the verified target identity or rely on an enforced
target/local equality check. Add a same-Python-version, different-schema
attach control, alongside file-schema mismatch tests.

## 2. Dispatchability is insufficient for pristine code construction

The draft distinguishes metadata from executable handlers, but construction
needs a further state contract.

Counterexample: INSTRUMENTED_LINE has valid, transparent metadata and a
dispatch body. It therefore passes an executable-handler predicate. Newly
constructed code initializes _co_monitoring=NULL
(`Objects/codeobject.c:459`), while its instrumentation helper
(`Python/instrumentation.c:1361`) requires monitoring->lines. Such a wrapper
is meaningful only with runtime displaced state.

Conversely, test_code's CACHE padding contract
(`Lib/test/test_code.py:520`) intentionally permits construction used by
Cython; rejecting every non-executable word at construction could remove
that behavior.

Define pristine-word admissibility and required runtime state separately
from metadata validity and dispatch-body existence. Specify treatment of
wrappers, stateful specializations and CACHE padding. Route the checks
through constructors, replacement, marshal and freeze; marshal currently
bypasses check_words (`Python/marshal.c:1944`). This is a bounded ID/state
validation change, independent of H2's CFG and root reconstruction.

Make the pre-cast oracle concrete. IR_WORD
(`Include/internal/pycore_ir.h:78`) and _PyIR_SetHandler
(`Include/internal/pycore_ir.h:263`) already cast to uint16_t; a later
decoder cannot recover the original value. Exercise the actual scalar
boundary with 65536 and 65536 + NOP, whose truncated value is otherwise
valid. Python byte-encoding rejection alone proves nothing about these C
boundaries.

## 3. The threshold-export contract remains undecided

The draft defers the externally visible decision. This blocks
renumbering/API cleanup, though it need not block the initial
numbering-preserving refactor.

HAVE_ARGUMENT is publicly re-exported. Its current value is 41, which is
also WITH_EXCEPT_START's ID, although that instruction's metadata lacks
HAS_ARG_FLAG (`Include/internal/pycore_opcode_metadata.h:1434`). Retaining
the existing meaning where it still describes numbering therefore needs an
explicit value-generation rule and applicability contract.

Inventory each affected C/Python export and state whether it remains,
changes meaning or is removed. Obtain the coordinator's disposition for
visible removals; the approved sampler break does not authorize them.
Internal classification should still use generated metadata.

## Decisions that hold up and bounded implementation split

The other central decisions hold up: one-way imports into ir_generator,
explicit domain arguments across the six numeric CLIs and direct callers,
separate base/kind/twin relations, exact pristine variant restoration,
aligned displaced arrays with actual atomic primitives where required,
and dense statistics sized by the actual count. The accounting warning is
correct: current high handlers are omitted, so old/new dispatch totals
require matched coverage. No sparse statistics redesign is needed.

1. Settle these contracts, then move allocation into the domain under
   existing IDs and verify generated ID equality.
2. Close runtime, constructor, monitoring, inspection and statistics
   consumers under fixed numbering.
3. Close target-schema transport and the approved 32-bit sampler
   codec/version/schema change coherently.
4. Renumber only after consumer closure; bump magic, regenerate frozen
   code, and collect fresh final-head evidence.

H3-1 needs ownership coordination, but no new H1/H2 root semantics. H3-2
and new operations remain outside this rung. Preserve the compiler phase
oracle and its controls.

Read-only review: no tests, builds or edits; the untracked draft remained
untouched.
