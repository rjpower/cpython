# Return recipients after36: ranked experiments

Read-only proposal, based on retained clean36 and golden executables. No source,
native, build or regeneration change is admitted here. Root's accepted generator
counts are cycles/golden 1.04524, instructions 1.02042 and CPI 1.02432. The
recording locates work in SEND_GEN, YIELD_VALUE, the inlined return-slot publisher,
backedges and refcounts. These locations do not apportion the excess counts.

## Actual comparison

Both executables are retained under E11 `null36-generator-perf-record`:

| Executable | Source | SHA256 | Build ID |
| --- | --- | --- | --- |
| candidate-python | 4c9c113b4e5ab0d8d061eb1b8a75c24f5c271d3c | 42b0422be04ecefa9e6957dba686e9dd07279d5ad16d48bda3a8164c532df206 | 34e484e9e6ab0ad7965955f6cbd057141572ecb1 |
| golden-python | 0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae | c9c3370017e5754670a27acb40a65e61ab103f4020a33ebad55b1a3029509ece | 61fbf1bb863cc2c72050228cc7704979a7779589 |

`sha256sum` agrees with `identity-before.json`. Read-only `nm -S --size-sort`
and `objdump -d --no-show-raw-insn --disassemble=<symbol>` give these sizes:

| Handler/helper | Golden bytes | Current36 bytes |
| --- | ---: | ---: |
| SEND_GEN | 474 | 743 |
| FOR_ITER_GEN | 369 | 538 |
| YIELD_VALUE | 500 | 453 |
| RETURN_VALUE | 624 | 546 |
| RETURN_GENERATOR | 915 | 1002 |
| CALL_PY_EXACT_ARGS | 1281 | 1426 |
| CompleteGeneratorReturn | absent | 1795 |
| CompleteStep | absent | 1597 |

Absence of a named helper is not absence of equivalent golden inline work.
Current YIELD no longer publishes/canonicalizes its proven home value. Golden
YIELD still contains object/type checks and a Canonicalize call. The current
publisher is only destination replacement and old-owner retirement.

In current YIELD, +0x8a loads parent flags; +0xa0 branches on pending redirect;
+0xa2 loads return_slot; +0xaa branches on NoDst; +0xb0 loads the old owner;
+0xb4 installs the new owner; +0xb8/+0xbb tests the old owner before closing.
Golden also has the NoDst test and load/install/test-old sequence. Thus the old
owner work is a possible shared optimization, not newly introduced overhead.

Current SEND_GEN loads gi_frame_state once at +0x81. CREATED checks the actual
sent value against None before EXECUTING is stored at +0xa3. It then detaches
the sent owner, publishes parent return_slot/return_pc, child/current/handled
exception links, checks recursion, and installs or discards the incoming owner.
Golden did not carry the same CREATED/non-None and recursion checks or safe
publication order. Its shorter text cannot justify deleting these obligations.

The current recipient contract is in `pycore_interpframe.h`:
SetReturnValue completes a pending redirect first, handles NoDst, then calls
PublishReturnSlot. The latter captures old, installs new, checks the home in
debug and closes old. Its old-owner load is necessary for a local. The return
path must honor finalizer redirects, including a newer redirect during a close.

## 1. Encode a proven empty-temporary recipient once

This is the first bounded experiment worth specifying. Replace the *meaning*
of the existing uint16 return_slot with one recipient descriptor: GENERAL slot,
proven EMPTY_TEMP slot, or DISCARD. It is a derived ownership fact, installed at
the existing callback-free call/resume/yield commit; it is not a benchmark mode
or a new independent pending flag. GENERAL retains the current replacement
semantics. EMPTY_TEMP requires the physical cell to be NULL when delivery begins.

The current 12-bit slot domain leaves bits in uint16, so an encoding could avoid
growing the current frame header. This is an internal ABI change despite keeping
offsets and size. Do not expose encoded bits as a slot index. One reader API must
decode it everywhere, including gen_yf, remote asyncio, debugger pending-child
checks, initial/interpreter/trampoline frames, frame copies and all constructors.
The native remote layout version/validation and any size assertions must be
audited. No live pointer or extra GC root is needed.

The smallest positive producer is VM SEND: the parent recipient is B+2, and
IR_ENTER_GENERATOR has just detached that cell into the child's incoming owner.
Do not infer FOR's A is temporary: it may be a local or NoDst. Other call and
yield producers may use the same fact only from their existing physical output
and cleared-home contract; otherwise install GENERAL. A suspended generator's
incoming destination is a separate recipient from its parent's yielded-result
destination. Both must be initialized and refreshed at their actual commits.

The normal EMPTY_TEMP sink can store without loading or closing an old owner.
A descriptor test/mask is added; it may save little or nothing versus the
current load/test/branch. Classification must not load code/nlocals repeatedly
to save one cheap return-side test. SEND has a static proof; local/temp choices
elsewhere need measured emitted code rather than an assumed free classification.

"Once at entry" is not necessarily amortized. Every CALL commit usually has one
return; every SEND resume usually has one yield or completion. Before selecting
this experiment, count recipient updates and deliveries in the exact tree and
yield-from kernels, partitioned by static producer and destination class. Compare
the added classification/mask/stores, including any code/nlocals loads, with the
removed old-owner load/tag/test branches across the whole pair of paths. A
saved descriptor may merely move work. Root's later cold37 two-pair generator
screen was about1.0065/golden, so the earlier36 loss also needs a repeat before
it motivates this ABI change. Neither observation identifies a source cause.

Pending redirect remains authoritative and is checked before consuming the
descriptor. A redirect may change the destination and must replace/invalidate
the descriptor at its real commit. The existing pending-child admission requires
NoDst; it cannot be widened by this experiment. Local replacement still installs
the value and continuation before retiring a displaced owner. DISCARD still
closes exactly once after frame/link commitment. Escape/copy retains a coherent
descriptor; external/native/PEP523 routes start with their legitimate general
recipient and do not inherit an unproved temp fact.

**Falsifiable gate:** first prepare a complete read/write census and descriptor
proof, then isolate SEND's derived empty-temp delivery. Compare generated and
compiled entry plus yield routes: an old-owner load/test should disappear on
the intended route without a larger net dynamic instruction ledger. Measure
balanced generator/coroutine elapsed and PMU counts, including branches. Reject
if added entry work or layout cost offsets the saving. Require local displacement,
NoDst mortal values, native send, created non-None, recursion, PEP523, tracing,
remote await inspection, frame escape and newer redirect/finalizer tests. No
prompt finalizer timing guarantee is required.

## 2. Bind newly created generators directly into their owning frame

This has a larger potential removal, with a larger proof. Exact calls currently
reserve/init a thread frame; RETURN_GENERATOR allocates the coroutine/generator,
copies the frame, marks its heap ownership, pops the reservation and returns the
new object. The compiled creation handler is 1002 bytes, versus 915 golden;
this path exists in both. Creation-heavy recursive coroutines could benefit,
but a long-lived repeatedly yielding generator pays creation once. The current
generator recording alone cannot establish enough creation frequency.

A coherent experiment binds arguments/defaults directly into an initialized,
GC-safe owning heap frame and runs the required creation prolog there. It must
not skip code merely because CO_GENERATOR/CO_COROUTINE is set. Genexpr codegen
can insert GET_ITER/GET_AITER before RETURN_GENERATOR; closure/cell setup and
PEP523/monitoring entry semantics also matter. An arbitrary function can mutate
its code/defaults during allocating hooks, so reuse the existing selected-code
pin/revalidation and failure transaction. Partial frames need initialized roots,
exact cleanup of all bound arguments/defaults, no double ownership of overlap,
correct unstarted-generator GC/finalization and coherent frame escape.

**Falsifiable gate:** obtain creation/resume/complete counts for the unchanged
workloads before implementation. First restrict to a positively certified entry
sequence and exact-call binding contract, retaining normal entry for other
sequences. Verify removal of reservation/copy/pop in assembly and measure whether
allocation and initialization dominate instead. Gate all argument binding/error,
cell/freevar, genexpr prolog, PEP523, monitoring and GC paths. This is a separate
larger packet, not an extension of recipient encoding.

## 3. Improve completion layout only after a compiled path ledger

SharedStep34 already removed VM canonicalization and generic kind branches using
one shared owner contract. Both normal and observed paths still have the same
PRE-before-detach, callback-free commit, POST-after-publication and held-owner
cleanup semantics. The current 1795-byte VM wrapper includes cold observer,
error, alias and finalizer paths. More forced inlining is not a proposal by
itself. ObserveStep is already out of line and its calls are guarded by monitoring
data; an inactive-but-allocated monitoring structure is a distinct case.

An admissible next experiment would outline only measured cold observer/error
preparation around the same commit implementation, leaving unobserved fixed-kind
commit and retirement compact. It must not introduce an unconditional held-owner
struct/spill bank or duplicate cleanup. First list normal-route instructions and
actual branch frequencies; if no unnecessary status/spill work remains, defer.
This cannot accelerate every YIELD, and completion-only cost is amortized across
many resumes in a delegating generator.

**Falsifiable gate:** preserve source owner/order identity, verify smaller hot
blocks without extra calls on the normal route, count total text/clones/spills,
and re-run exact PRE/POST callback error/redirect/alias cases. Measure separately
from recipient or creation changes. Keep the current implementation if the
compiler already separates the cold work or the counts do not improve.

The duplicated gi_state load is not a fourth optimization: retained assembly
already coalesces it. None of these options permits removal of created-send,
running/exhausted, recursion, exception-link, pending redirect or semantic event
checks merely because the benchmark does not exercise them.
