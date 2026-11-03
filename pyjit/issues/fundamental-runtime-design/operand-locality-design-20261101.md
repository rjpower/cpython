# I1: execution-copy operand locality for ADD cleanup

2026-11-01. **Design for independent review; implementation and execution are
held.** Scope: source and retained native evidence only. The
[work breakdown](../../design/runtime-work-breakdown.md) assigns a real
compiler-fact consumer without endorsing the current stack-constrained IR as
the final representation. The [comparison-region design](ir-region-hypothesis.md)
remains unchanged and held.

Exact source and retained-native input hashes, static-block facts and layout
arithmetic are recorded in
`build/fundamental-runtime-review-20261101/operand-locality-design-evidence.json`.
The host arithmetic checks validate this proposal, not an implementation.

## Hypothesis and deciding evidence

The final physical B/C operand indices and the code's `co_nlocalsplus` are
immutable. Derive their locality once while creating the execution copy;
consume those facts in exactly generic `BINARY_OP_ADD` and specialized
`BINARY_OP_ADD_INT` success cleanup. This may remove the shared execution-header
lookup while preserving every NULL store and local borrow.

The saved qualified e9 evaluator has one shared two-load chain, not two
independent pointer chases: ordinary binary success `45935e–459395` and an
ADD_INT success path `4652a5–4652dc` load frame->ir_words, load its header's
nlocalsplus, then compare both inputs and conditionally clear them. The
[independent audit](fable-ir-vm-audit-20261101.md) binds that native evidence.
Two bit tests can replace the limit comparisons but still need metadata/index
loads, decisions and stores. Decoding, initialization and changed machine
allocation may cost more than the lookup saves. Counts do not prove cycles.

No new handler ID, word, side table, frame field, root representation, native
ABI, validity bit or runtime option is introduced. This is a bounded lowering
fact, not a type fact or a general SSA prerequisite.

## Exact representation

The 64-bit word remains eight bytes; A/B/C/D retain their four 12-bit fields.
Current IDs remain numerically unchanged; the domain is 302 entries.

| Bits | Meaning | Pristine `co_code` | Executing copy |
|---|---|---|---|
| 0–8 | Handler ID, mask `0x01ff` | Existing valid pristine ID | Existing valid dispatch ID |
| 9 | B is below co_nlocalsplus, mask `0x0200` | Zero | Derived boolean |
| 10 | C is below co_nlocalsplus, mask `0x0400` | Zero | Derived boolean |
| 11–15 | Reserved, mask `0xf800` | Zero | Zero |
| 16–63 | Existing A/B/C/D | Unchanged | Unchanged |

Pristine non-handler mask: `0xfe00`. Execution-locality mask: `0x0600`.
Handler count must be in 1…512; each accepted ID must additionally satisfy the
existing domain/property checks. A count of 512 does not make every value
0…511 a valid dispatch ID. Slots still stop at 4094; the 4095 sentinel,
feedback capacity, jump limits, line/exception offsets and feedback stride do
not change.

This narrows the audit's provisional four-field suggestion to **two facts with
actual consumers**. A/D locality is not computed or stored. For B/C, the bit
means only `field_value < co_nlocalsplus`, independent of the current field
role. An immediate or feedback field may therefore have that numerical bit;
only an admitted slot-input consumer may use it. This avoids invalidating facts
when a handler is instrumented, specialized or restored. It does not classify
derived window positions, ownership, operand types or GC liveness.

One generator-side layout definition supplies the C and Python constants:
export the handler width/mask and B/C masks through `opcode_id_generator.py`
and `py_metadata_generator.py`. `pycore_ir.h` and `dis.py` consume those
products. The handler-domain generator and C static assertion enforce the
same nine-bit capacity. Do not maintain a second handwritten Python mask or
renumber the existing IDs to make this fit.

## Production, validity and publication

`Objects/codeobject.c:check_words` is the common pristine-word boundary called
by `_PyCode_New`, including public constructors, replacement and marshal.
Reject **any** low bits 9–15 before masked handler extraction or metadata
indexing. A crafted old 16-bit value such as `NOP | 0x0200` must fail rather
than become NOP plus trusted locality. Preserve all existing field, constant,
feedback and pristine-handler checks. `_PyIR_MakeWord` continues to validate
the original scalar before narrowing; `IR_WORD` produces flag-free trusted
pristine words. Valid serialized code retains the same bytes and handler IDs;
there is no new persisted execution metadata or automatic bytecode epoch bump.

In `Python/ir.c:make_exec`, retain the existing one allocation and unpublished
execution header. Replace the word memcpy with a bounded copy/derive loop:

```text
for each validated pristine word w:
    flags = (IR_B(w) < nlocalsplus ? 0x0200 : 0)
          | (IR_C(w) < nlocalsplus ? 0x0400 : 0)
    words[i] = w | flags
```

Derive for **every** word, including non-ADD words, because its handler can
change while its fields do not. Do this before the `fb_size == 0` early return
and before publishing `co->_co_exec`. Header and feedback initialization keep
their current ordering. The loop allocates nothing, runs no Python and adds no
new rollback stage. Existing allocation failure still leaves no published
execution copy. Debug checks enforce the pristine-zero precondition and derived
truth. Clearing/rebuilding an execution copy derives new facts; `code.replace`
must not copy metadata from the old execution copy when locals or words change.

The two static execution blocks need explicit compile-time derivation:

- `Python/ir.c:_PyIR_InitCleanup` has nlocalsplus=0: all B/C flags are zero.
- `Python/ceval.c:entry_trampoline` has nlocalsplus=1 and B=C=0 in all three
  words: **both flags are set**, even though its handlers do not use the new
  cleanup. Do not confuse slot1 being temporary with B/C's actual zero values.

Use one trusted constant-expression execution-word construction rule for those
blocks, distinct from pristine `IR_WORD`. Do not add runtime initialization of
the constant trampoline, a second block registry or an initialization flag.
`PyCode_NewEmpty` and assembler-generated jump words remain pristine and pass
through the ordinary copy/derive path. Frozen/import tools preserve pristine
code; audit their paths without rewriting inactive upstream stack tooling.

## Complete reader and writer obligations

| Boundary / actual source | Required disposition |
|---|---|
| `pycore_ir.h:IR_HANDLER`, `_PyCode_InstrHandler`, `_PyIR_FieldOparg`, format/target/folded-input readers | Extract nine handler bits; keep A/B/C/D/CD and property tables unchanged. Never treat metadata as an opcode. |
| `ceval_macros.h:NEXTOPARG`, `DISPATCH_SAME_OPARG`, `IR_DEOPT_BINARY_OP`, direct/predicted dispatch, frame-entry RESUME tests and LOAD_SPECIAL check | Handler fetch uses the shared mask. Same-word dispatch retains the word's B/C facts. Opcode variables remain pure handler/base IDs. |
| Generator unknown-handler paths, computed table, switch and tail table | Decode through IR_HANDLER. Keep the current domain size, handler order and predicted joins. Add no handlers, dispatches or statistics events. |
| `_PyIR_SetHandler` | Validate the unmasked scalar as today, then replace only `IR_HANDLER_MASK`: `(old & ~(_PyIRWord)IR_HANDLER_MASK) | handler`. Preserve locality and upper fields. A rejected scalar changes nothing. It cannot validate locality truth without a code/header; do not claim it does. |
| `specialize.c:instr_opcode`, `set_opcode`, `specialize`, fb-record/restore and poly row handler tokens | Decode masked IDs; every install routes through the preservation boundary. Poly tokens remain pure IDs in their existing uint16_t storage, with no locality mixed into them. |
| `instrumentation.c:word_opcode`, `set_word_opcode`, `store_opcode`, line/instruction displacement, `_PyIR_RealHandler` / `_PyIR_OriginalHandler` | Transparent wrappers and monitoring twins retain flags in the word. Displaced/canonical tokens contain only IDs; restoration changes only the ID. Preserve fields and existing format selection. |
| `Objects/codeobject.c:check_words` and inline-constant fallback | Reject pristine metadata first. Existing fallback may call SetHandler on a pristine local word; its zero flags remain zero. `_co_code_adaptive` remains a raw copy of execution bytes and can now contain flags. |
| `Lib/dis.py:_unpack_words` and generated `_opcode_metadata.py` | Use the generated nine-bit mask for pristine and adaptive bytes. Fields, source labels, base-op identity, feedback and jump display remain unchanged. Do not hide execution bytes by returning a sanitized fake `_co_code_adaptive`. |
| `_testinternalcapi.get_ir`, feedback inspection and scalar-boundary fixtures | Existing handler/field tuple shape stays unchanged and uses masked IDs. Add a narrow test-only readout of execution locality that independently recomputes expected B/C truth; it must not make live words writable or create a public API. |
| `Python/pystats.c`, INSTRUCTION_STATS, profiler/ELF table readers | Pure-ID accounting and table cardinality remain unchanged. Handler count remains 302. Saved raw addresses and stats do not acquire metadata bits or synthetic entries. Existing broader historical stats parsers need no unrelated domain rewrite. |
| `marshal.c`, assembler/compiler, code getters, frozen producers | Serialize/build pristine zero-flag words; no execution flags leak into caches. Verify raw-word fixtures and code replacement. Existing valid pristine bytes need not change. |
| GDB helpers and `_remote_debugging` frames/code_objects | Current line lookup uses word offsets/IP and code metadata, not low-sixteen-bit opcode decoding. Verify this remains true; no line/offset or layout change is required. Any actual newly discovered independent handler decoder must use the shared constant before acceptance. |

Audit actual current callers, not only this table. An unexplained direct
low-sixteen-bit load/index, whole-word opcode replacement or persistent
execution-word producer blocks source acceptance. Search results, exact caller
classification and resulting changes belong in the source capsule.

## Two consumers, success cleanup only

`IRStack.consume` currently emits IR_CONSUME_INPUT for borrowable register
inputs (`ir_generator.py:1272–1278`). Give the current handler's final success
flush an immutable, generator-only selection for B/C when its **emitted name**
is exactly `BINARY_OP_ADD` or `BINARY_OP_ADD_INT`. Carry that selection through
the generator's stack-state copies. Check the existing format/borrowability
instead of assuming arbitrary input k equals physical field B or C.

Only those two final success clears use named B/C locality consumers. Preserve
the current order C then B, then destination store. Earlier writes/restores,
error flushes, destination-local tests, implicit/window inputs and generic
IR_CONSUME_INPUT remain dynamic. Regenerating the file must demonstrate every
other handler body and label unchanged after source-line normalization. Do not
hand-edit or postprocess generated C, or attach false nonallocating effects to
an allocating ADD.

For the GIL little-endian computed backend, read the fixed flag from the word
in memory (the existing narrow-field access policy) rather than keeping a new
full ir_word lifetime across the handler. Tail and atomic-word paths derive it
from their already-carried/loaded word. A debug assertion compares each used
bit with existing IR_IS_LOCAL immediately before consumption. The release
consumer tests the bit and clears that exact input slot iff it is temporary.
Do not replace the required stores with a root bitmap or leave stale values.

This source access policy is an intent, not a native guarantee. The compiler
may combine a flag read with a full-word load, lengthen ir_word's lifetime or
spill other state. Acceptance needs actual branch-following native proof for
the selected backend; macro spelling alone proves none of those properties.

Important source fact: specialization maps **both NB_ADD and NB_INPLACE_ADD**
to BINARY_OP_ADD_INT (`specialize.c:2395–2400`). Therefore specialized integer
`+=` is in scope. Generic BINARY_OP_INPLACE_ADD stays dynamic. ADD_FLOAT,
ADD_UNICODE, in-place unicode, instrumented generic BINARY_OP and every other
specialization retain their existing cleanup. A site may transition among
these forms without recomputing immutable flags; which consumer runs is
determined by its actual handler, not its original source spelling.

Compact-int admission, overflow boxing, generic callbacks, error jumps,
specialization counters, feedback rows, ordinary and predicted entry, both IP
publications and all frame/slot stores stay in the same semantic order. Inputs
remain rooted in their current homes throughout allocation/reentry. A callback
may change a local's value; its index remains local and must never be cleared.
No root span, safepoint, breaker, observer, call window or unwind rule changes.

## Controls that must fail for the right reason

The implementation must supply source-bound controls, not a second validator
that merely agrees with its own producer:

1. Test independent expected flags for B/C below, equal to and above the
   boundary, zero locals, maximum legal boundaries and distinct B/C classes.
   Include no-feedback code, code.replace with a changed locals layout,
   execution-copy recreation, and both static execution blocks.
2. For each locality/reserved bit, forged pristine bytes are rejected before
   handler indexing. Negative, 512-plus-valid-ID and oversized scalar handlers
   are rejected before narrowing by both construction and replacement. Existing
   instrumented/runtime-only versus pristine admission tests remain intact.
3. Snapshot actual flags across warm specialization, plain/in-place ADD_INT,
   misses, MONO/POLY/MEGA transitions where reachable, instrumentation enable,
   instruction/line wrappers and restoration. Keep actual source/input fields
   and expected handler transitions. If a transition is unreachable, document
   it rather than manufacture a coverage claim.
4. Use one source fault that reinstates the old `~0xffff` replacement: the
   actual preservation test must fail: seed its copy-only setter fixture from
   an actual execution word with nonzero B/C flags, not pristine IR_WORD/NOP
   whose zero flags would make the old setter look correct. Use another that
   omits a local flag and another that swaps B/C derivation: mixed local/temp fixtures must fail the
   independent metadata assertion or debug consumer assertion before an
   unsafe local clear. Preserve fault outputs and exact nominated failures.
   Copy-only test validation may inspect a malformed word; no release target
   is authorized to run deliberately corrupted live metadata.
5. Execute local/local, local/temp, temp/local and temp/temp ADD, equal input
   fields, a destination aliasing each local, normal return and handled
   exception, int subclass/reflected addition, callbacks rebinding f_locals,
   specialization loss, compact overflow boxing and injected allocation failure.
   Check result, old/new local visibility, original exception location and
   temporary release with weakref payloads. Result equality alone is inadequate.

The flags describe numerical field provenance, so missing a clear can retain
a temporary and a wrong temporary flag can destroy a live local. Both failure
directions need witnesses. Existing stale-root controls, debug checker KILL
and precise-root audits remain required; they are not replaced by flag tests.

## Frequency question and minimum observational prerequisite

Nominate registered **coroutines** provisionally, with generators and
unpack_sequence controls and spectral_norm secondary. Before deciding to
implement for performance, use the accepted shared baseline to ask: do generic
ADD and ADD_INT entries occur materially in these workloads, and which actual
current code words have B/C cleanup with each locality combination? Reject the
selection as insufficient if the nominated consumer is absent or cannot be
attributed; do not silently replace the primary after looking at scores.

The shared debug/stats image and retained original raw counters can answer
entry activity, not successful cleanup frequency. ADD_INT guard misses and
generic ADD's specialization redispatch increment entries without taking
success cleanup; family hit counters are not an ADD-only PC census. Actual
pristine/executing get_ir plus code/kernel/origin identity can identify sites
and their static flags, but cannot distribute global handler counts over
multiple PCs. Report upper bounds and ambiguity explicitly. A code-word capture
not already in the shared manifest needs separate bounded approval/review; this
design does not append processes or observers to that campaign.

There is no 5% pair-frequency gate, claimed cycle saving or need to fix counter
permissions. Stats source/config/product/origin and real coverage2/mode0 must
be bound first. Designing the representation now does not wait for that census
and does not confer implementation or timing authority.

## Implementation, native proof and qualification

One managed candidate implements the whole contract, not an ADD-only patch
that leaves flag-bearing words unsafe elsewhere. Expected source families:
IR header/runtime initialization; code validation; two generator consumers and
their C/Python metadata products; interpreter macros/static trampoline;
disassembly; narrow internal test inspection and focused tests. Audit-only
consumers need not be edited. Keep the seven standard generated products
reproducible; unchanged products, handler IDs, targets and metadata tables must
be proved unchanged. Extend relevant generator/domain tests to the new limit;
do not weaken full-domain coverage. No collector or native ABI changes belong
in this candidate.

Review three source arms:

| Arm | Purpose |
|---|---|
| P | Exact unchanged parent |
| M | Complete flag production/decoding/preservation, but the generator's two-consumer selection is empty; ADD cleanup remains dynamic |
| C | Same metadata mechanism plus the two admitted ADD consumers |

M is a source-only ablation, not a configure/environment/runtime flag. C−M
asks about consumption; C−P includes eager initialization and global decode
cost. Confirm the only C/M semantic-generation difference is the selected
cleanup. Separate binaries are not machine-layout-identical causal controls.
This is substantial global decode/validation work for two initial consumers;
the shared census is only an upper-bound screen, and cannot pay that cost by
itself. A loss in non-ADD workloads is a real package cost even when their
generated bodies stay identical.

First feedback requires fresh GCC debug/O1 and O3 computed images, strict C
root and linked inline audits, focused flag tests and whole test_ir,
test_generated_cases, test_dis, test_code, test_trace, test_monitoring,
test_generators, test_coroutines and test_gc in declared OFF/KILL modes.
Also validate the unchanged tail word-decoding path with a supported Clang
debug/tail cohort under its reviewed backend contract; this is compatibility
coverage, not backend adoption. Bind exact rosters, skips, origins, raw streams,
source/products, timeouts and idle cleanup before activation. No broad skip
or C-backend/full-qualification claim follows from these focused selections.

Native review must follow complete successful local/temp combinations and
cold paths through ADD_INT boxed/inline admission, overflow and failure,
generic callbacks and exceptions, deoptimization, predicted/ordinary entries,
monitoring restoration, flag production and non-ADD dispatch. Verify the
intended header-limit lookup is removed in C versus M, not merely hidden
beyond a short prefix. Retain any additional mask/index/state loads and common
handler changes. Compiler outlining must pass the existing precise audit;
there is no opaque helper exemption. Actual elapsed decides despite static
instruction/text counts.

If native correctness and attribution justify a trial, propose one fixed
45-minute quiet comparison on P/M/C and the four nominated registered
workloads: two rotating-order rounds, five serial durations per cell,
24 fresh score processes/120 raw durations, plus 12 one-loop zero-sample origin
processes. Keep registered setup/warmups, OFF/N0, 32 MiB policy, CPU/sibling,
locks and load admission unchanged. These are correlated serial samples, not
120 independent processes. Before scores freeze exact order, descriptive
noise rules, RSS capture and resource tradeoffs. Require C/P coroutine median
at most 0.99 in each round, and a C/M ratio below 1 in each round to attribute
a consumer gain. For each primary contrast and round, require the absolute
median benefit to exceed the sum of both MADs and all ten leave-one-out
median ratios (drop each sample once from either arm) to remain below 1.
These are descriptive adequacy checks, not confidence intervals or independent
process replications. A repeated control loss above 2% needs bounded
resolution; no extra samples or static proxy rescues
a failed primary. If M alone explains a package improvement, do not credit ADD
consumption without its incremental evidence.

Eager derivation replaces memcpy for every executed code block. Separately
capture compilation and first-execution cost using a fixed existing compiler
corpus and fresh processes: warmed score loops exclude this cost. Freeze that
finite corpus/count and helper in the later performance plan, or defer the
trial if no suitable existing helper can be bound. Record full-process peak
RSS, code/execution/feedback bytes, executable footprint and build wall cost;
the proposed bit placement adds no allocated word or frame bytes but does not
prove zero process-memory effect.

A successful focused trial is not adoption. Before landing, report all 29
registered elapsed ratios and absolute/upstream-relative RSS and complete
the existing [validation contract](../../design/validation.md) on one coherent
source: release/debug OFF/debug KILL full scopes, root/stress/ASan, supported
backend and C-extension closure, generated products and cleanup. Reconcile
actual missing modules/skips explicitly. Retire M/fault source controls with
the decision and retain their evidence. No candidate is exempt because its
language semantics were intended to stay constant.

## Stop conditions

Reject or redesign this increment if it needs a whole-IR rewrite, handler
renumbering, per-PC registry, mutable locals metadata, new runtime option,
extra root/publication policy, callback protocol, broad audit exemption or
additional physical-home changes. Any unexpected independent decoder must be
closed within this contract before native work. Do not weaken pristine-zero
validation, ignore an unsupported transition, or accept a mask-bit native path
merely because it is shorter in C.

The coordinator must grant exact implementation/build/performance scopes and
feasible qualification time. This document extends no old clock or deadline;
if the necessary work does not fit, defer the increment explicitly. The
proper logical-value/effect compiler and controlled VM lowering remain broader
architectural investigations rather than prerequisites silently implemented
inside I1.
