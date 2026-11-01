# Independent audit of Fable's compiler and VM recommendations

2026-11-01. Read-only source and retained native evidence. **Accept the need
for better attribution; narrow the proposed mechanisms and reject the claimed
architectural prerequisites.** This review grants no implementation, build,
target invocation, census or timing. The frozen
[region design](ir-region-hypothesis.md) remains unchanged and held.

Input: `build/claude-fable-peer-review-20261101/review-medium-v1.md`, the
completed direct medium-effort Claude Fable 5.1 review. Exact input and local
evidence hashes are retained in
`build/fundamental-runtime-review-20261101/fable-ir-vm-audit-evidence.json`.
The coordinator owns reconciliation with the separate GC review.

## Findings and dispositions

| Fable claim | Disposition | Evidence and consequence |
|---|---|---|
| Current workload attribution is insufficient to rank opcode mechanisms. | **Accept.** | There is no bound current-runtime per-workload opcode/pair census. The existing synthetic pair probe and zero-call disassembly do not establish workload hotness. |
| Richards winning without timed GC proves the interpreter is not a deficit. | **Reject that inference.** | The broad table compares an integrated value/IR/object/GC implementation with upstream. A win can coexist with substantial avoidable interpreter cost. The table does not isolate dispatch, memory homes or boxing. |
| Source `IR_IS_LOCAL` expansions establish the native per-input cost. | **Narrow to actual paths.** | The saved e9 binary shares the locality limit across inputs. Exact loads and comparisons are described below. The proposed static fact remains useful, but its gain is unmeasured. |
| Seven unused handler bits make a consume mask trivial. | **Narrow.** | The current 302-entry domain fits nine bits, but decoding, replacement, validation, instrumentation and Python readers currently treat all sixteen bits as the handler. Reserving bits changes the executing-word contract. |
| Boil can publish IP only at escaping/raising handlers “as upstream does.” | **Reject the precedent; separate the hypothesis.** | Pinned upstream CPython 3.14.0 publishes IP in ordinary LOAD_FAST_BORROW and NOP handlers. Boil's observing, throwing and resuming paths need an independent publication proof. |
| An optimizing compile-time IR has no consumer before a runtime tier exists. | **Reject.** | Existing copy/store rewrites already consume static value/effect facts before specialization. Additional value-use, placement and guarded-region transformations need no native JIT tier. Full SSA is still not justified merely by naming it. |
| Tail calls isolate layout noise, so select tail even at an elapsed loss. | **Reject as an acceptance rule.** | Handler functions change compiler boundaries, not all code-generation interactions. No retained matched experiment proves lower sensitivity. Repeated measured regressions remain product performance effects. |
| Fewer IR words than upstream decides whether the compiler is the lever. | **Reject the dichotomy.** | Word counts omit payload, guard, operand, root and feedback costs. They help diagnose lowering but neither establish nor rule out a useful compiler transformation. |
| Counter access is required and would halve necessary samples. | **Reject as a prerequisite or quantitative claim.** | Access is concretely denied; no security-setting change is authorized. Counters help explain work but do not determine elapsed variance or a universal sample count. Elapsed and memory comparisons remain possible. |

The historical table remains useful context, with its recorded 0b15 source and
sample limitations. It does not describe the current nursery candidate or
attribute seven native-heavy losses to boxing. Boxing deserves its own measured
workstream; that causal attribution is not supplied by workload names.

## What the locality check actually costs

`Python/ceval_macros.h:516–538` obtains `nlocalsplus` through the execution
header before `frame->ir_words`. The two `IR_CONSUME_INPUT` expansions in
`Python/generated_ir_cases.c.h:134–200` do not imply two independent native
pointer-chase chains.

In the retained qualified e9 whole evaluator:

- Ordinary binary success at `45935e–459395` loads `frame->ir_words` once,
  then its `nlocalsplus` once. The resulting register is reused for both input
  comparisons. Each temporary input can cause its own NULL store.
- ADD_INT's retained boxed-left/inline-right success path reaches
  `4652a5–4652dc`: again one frame/header pointer load, one limit load and
  two comparisons, followed by conditional clears. Its earlier admission
  branches and possible overflow allocation remain separate costs.

Thus Fable's literal “two loads” can describe the *total shared two-load
chain* on these paths; it must not be read as evidence that each source macro
independently reloads that chain. This is static emitted-path evidence, not
an executed-path frequency or cycle measurement. Removing the shared chain
still leaves operand-index extraction, two decisions and the required temporary
stores. A bit test can replace a comparison without removing a branch. It also
needs a mask/word load and may change native register allocation.

The saved baseline ELF is
`289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`.
The reviewed whole-evaluator text is
`build/worktrees/dispatch-slot-base/build/dispatch-slot-base-feedback/emission/baseline-whole-evaluator.asm`.
No new disassembly or target execution was needed.

## Seven bits are an encoding opportunity, not a free change

With 302 handler-domain entries, a nine-bit handler leaves seven bits in the
existing sixteen-bit field. That arithmetic is correct. A complete design
must settle the following affected consumers before changing any handler:

| Contract | Concrete current consumers and required change |
|---|---|
| Decode and dispatch | `IR_HANDLER`, NEXTOPARG, same-word redispatch, generated tail dispatch and predicted paths must mask the handler independently of static flags. The generator's count limit and compile-time domain assertions must match the smaller domain. |
| Construction and validity | `IR_WORD`, `_PyIR_MakeWord`, assembler jumps, `Objects/codeobject.c` validation and trusted static execution words must specify which flags are legal and who derives them. A crafted word must not falsely mark a local temporary and clear a live local. |
| Execution initialization | `Python/ir.c:make_exec` copies pristine words. Decide whether flags belong in pristine code or are derived for this execution copy. The `_PyIR_InitCleanup` static execution block also needs an explicit rule. |
| Rewrite and restoration | `_PyIR_SetHandler` currently replaces all low sixteen bits. Specialization, deoptimization, polymorphic feedback, monitoring twins and displaced sixteen-bit opcode tokens must preserve flags as word facts while keeping tokens pure handler IDs. Transparent wrappers must restore the same physical-field interpretation. |
| Python and external inspection | `Lib/dis.py:414` independently masks sixteen bits. Generated metadata, `_testinternalcapi.get_ir`, profiler/debugger readers, scalar-boundary tests and raw-word fixtures must agree about pristine versus executing flags. Marshal/deepfreeze/code construction need either consistent encoded flags or an explicit pristine-zero rule; their ability to copy bytes is not a semantic validation. |
| Storage semantics | Locality is a physical-home property, not liveness. Register-style field consumption, conditional store/restore, derived window positions, unwind and frame-switch code are different consumers. An input mask cannot silently replace all IR_IS_LOCAL uses. |

This is the affected-consumer checklist for a future exact diff, not a claim
that every listed file must change. Existing consumers already routed through
`IR_HANDLER` may need only verification. A fixed count of spare bits must not
become a second handwritten metadata authority. No two-word event protocol is
needed for this idea.

## IP publication needs its own correctness argument

Upstream's pinned generated code publishes current IP before LOAD_FAST_BORROW,
NOP, ADD_INT and ordinary BINARY_OP. Therefore the proposed C2 change cannot
borrow an upstream guarantee that these stores are unnecessary.
[CPython 3.14.0 generated handlers](https://github.com/python/cpython/blob/v3.14.0/Python/generated_cases.c.h#L8974-L8988),
[NOP](https://github.com/python/cpython/blob/v3.14.0/Python/generated_cases.c.h#L9949-L9958).

Boil's generator already respects `no_save_ip`; it is a property, not the
inability to emit a selective prologue. The current generated file has 287
TARGET bodies and 284 standard IP-publication prologues; Fable's denominator
300 is not that body count. Neither number is execution weighted. Broadening
the property requires proving current
and previous IP at every traceback/error exit, synchronous callback, line and
instruction observer, f_lineno jump, deoptimization retry, breaker, external
frame inspection, yield/return, throw and frame entry. Previous/current IP is
also needed by instructions marked `needs_prev`. The frame helpers explicitly
allow racy IP inspection for sys._current_frames-like APIs
(`pycore_interpframe.h:325–336`); a stale IP policy must state that observable
contract rather than reason only about C helpers that can raise.

Do not invent an IP-dependent root-map obligation that this implementation
does not have: `gc_trace.c:live_visit_frame` uses `_PyFrame_GCValues`, which
scans the full frame slot range. Those roots must remain current at collection
and reentry boundaries regardless of IP. A future liveness-map collector would
create a different obligation. Lazy IP and lazy root materialization are
separate changes; neither is part of the first hypothesis below.

## Compile-time optimization has existing consumers

The current compiler already reasons about fresh values before physical slot
placement. `regcfg_passes.c:153–308` folds a borrowed local into its single
popped consumer. It stops across possible Python execution, writes to the
local/destination, unsuitable windows and source-line boundaries. It does
**not** require the consumer itself to be nonreentrant: the admissible input
field is read before that consumer runs Python. Store elimination separately
preserves a destination's old value across raising operations. These are useful
effect-aware transformations on generic operations today.

Static constants, exact value-use counts, redundant transport, branch shape,
home interference, call-window placement and exception/observer materialization
are also compile-time facts. A guarded interpreter region can use static
adjacency/use facts and check operand types at runtime without compiling native
code. This is why the provisional comparison-region design is meaningful even
though its workload selection is not yet justified.

None of this licenses full SSA, GVN or LICM without a specific consumer. Keep
logical values before physical-home assignment and add the smallest effect
proof a transformation needs. Canonical memory homes remain defensible at
observable boundaries: native frames are not automatically GC roots, locals
can change through callbacks, exception exits expose state, and calls require
contiguous windows. Removing those homes throughout the VM is a larger design.

## One first source hypothesis: precomputed ADD operand locality

**Provisional, held:** deriving ADD's two input-locality facts once when an
execution block is initialized can remove the shared header lookup from its
success cleanup without changing root lifetimes, IP publication or dispatch
count. This is a different mechanism from the rejected constant-operand and
slot-base changes. It must earn its own elapsed result.

The smallest useful design is execution-copy-only field-locality flags in bits
9–12, one per A/B/C/D field, with a nine-bit handler. Derive each flag from
that immutable field value and the code's immutable `co_nlocalsplus` during
`make_exec`; the numerical predicate is valid independently of handler role.
Keep pristine words flag-free and reject nonzero reserved bits there. Preserve
execution flags through every handler rewrite. Initialize static execution
blocks explicitly. Initially consume the B/C facts only in generic ADD and
ADD_INT cleanup; retain existing rules elsewhere, including windows and error
paths. Emit debug equivalence checks against IR_IS_LOCAL. No mask is a root
bitmap, and local slots never become consumable because of a type guard.

This choice pays a global decode/initialization contract for a deliberately
small consumer. That cost may outweigh the saved lookup and is part of the
hypothesis. If preserving all affected consumers requires a broad format
rewrite, stop and redesign rather than call it a one-week peephole.

Before implementation, require a separately reviewed exact encoding/reader
diff design and a bound current-runtime Py_STATS image plus a small registered
workload census. The
`pystats-readiness.md` receipt found **no bindable existing current image**;
the minimum missing prerequisite is a separately authorized fresh computed
Py_STATS build with exact source/config/tool/product/origin bindings and real
coverage-version2/mode0 validation. No ancestry-only source bridge is valid.
Keep raw per-opcode counts and pairs; distinguish same-word specialization
redispatch from adjacent-word pairs. Select one registered primary from actual
relevant execution, not the synthetic probe. The absence of counters presently
holds mechanism selection; it does not establish that this candidate wins.

If selected, compare parent, static-flags-plus-ADD-use, and a source-only
materialization control that derives/preserves the same flags but retains ADD's
dynamic locality cleanup. Include compile/execution-block initialization cost
and code/frame/feedback/RSS changes, not only warmed loops. An ablation is not
machine-layout-identical causal proof. Native review must show the lookup
actually removed on admitted paths and all cold/error/observer exits preserved.
Test local/local, local/temp, temp/local and temp/temp inputs, equal input
fields, local destination aliases, boxing overflow/OOM, callback local writes,
specialization miss/restore, monitoring toggles and full-domain malformed-word
rejection. Existing precise-root and stale-slot tests remain required.

Use actual elapsed as the acceptance criterion under a frozen workload/order,
finite exposure and noise policy; include whole-suite control and RSS costs
before landing. No instruction-count, text-size or arbitrary pair-frequency
percentage rescues a losing elapsed result. If the minimum evidence/build and
qualification work cannot fit the integration deadline, defer this source
increment explicitly.

## Better compiler/interpreter work breakdown

1. **Bind attribution once.** Preserve per-handler and pair vectors, actual
   code-word sites, representation bytes and current source/image identities.
   A small attribution question need not wait for six week-long universal
   censuses. Stats are a separate diagnostic image; release elapsed is the
   product result. Hardware access remains optional.
2. **Review the static-fact consumer above.** One lowering fact, one cleanup
   consumer, complete encoding/observer/root proof and a materialization
   control. Leave the region candidate held until its own evidence suffices.
3. **Keep backend controls separate.** Matched GCC-computed, Clang-computed
   and Clang-tail can identify compiler/backend effects and validate roots.
   Tail is a candidate with migration costs, not infrastructure that must be
   adopted before any other measurement. Its benefits must include the actual
   product workload costs; an intentional slower infrastructure choice would
   require a new explicit user decision.
4. **Grow the logical-value/effect layer only with consumers.** Keep calls,
   windows, exception edges and observer states explicit before assigning
   physical homes. A winning static-fact or bounded-region experiment can
   justify the next representation change. Do not defer every compiler
   transformation until a JIT, or introduce full SSA merely as architecture.

The two rejected campaigns remain elapsed rejections. Slot-base's primary was
1.119789/1.109957 and unpack 1.477540/1.527735; constant operands' primary was
1.044653/1.116384. Their small correlated samples limit effect estimates, but
the repeated slowdowns cannot be relabeled measurement noise because native
code changed outside the intended handler. Whole-evaluator register allocation
and layout are possible *mechanisms of real product cost*. Existing branch
captures show extra frame/IP transport; no retained evidence apportions the
elapsed difference to it or proves an instruction-cache cause.

Verification: read source and retained disassembly, reviewed both sealed
elapsed outcomes, checked the current format/readers and consulted pinned
official upstream source. No target, compiler, profile or timing ran. This
report is a design audit, not a source acceptance or a backend selection.
