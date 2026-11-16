# Execution72 compiler/layout source proposal

Reviewed first-unit source contract, from selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`. Independent source review and root implementation GO received on 2026-11-15. The complete cold-map source is integrated in candidate G. Both builds pass 117 modules/79 shared extensions; 53 focused methods pass per role at `48084587d0b`. The narrowed foundation at `ba98006e236` retains the existing compactor; qualification continues before selection. **Yes: cold EH survivor maps can release physical allocation independently of the accumulator ABI, with initialized scratch retained.** This is the first implementable unit; it does not claim the full execution72 architecture or an accumulator speedup.

## Ownership and first implementation boundary

Compiler worker owns `Python/regcfg.c`, `regcfg_passes.c`, `assemble.c`, `Objects/codeobject.c`, `Include/internal/pycore_regcfg.h`, and new compiler/public-image tests. No new SSA or parallel owner analysis. Reuse logical canonical/fresh register IDs, current liveness, owner-held occupancy, generated `_PyIR_NormalSemanticTransfer`/Storage actions, and codeobject semantic worklist.

Root assigns this worker the COMPLETE cold layout unit: also `Python/ceval.c`, `frame.c`, `Objects/frameobject.c`, `instrumentation.c`, `Modules/_opcode.c`, `Lib/dis.py` and test-support map decoders, plus required cold generator/frame-header consumers. This worker owns shared `pycore_ir_exception.h`, `pycore_ir_source.h`, `pycore_ceval.h` and any HomeView/error-scope changes for this unit. The backend worker develops the accumulator ABI proposal read-only, then rebases on the integrated cold-layout checkpoint before any overlapping runtime edits. Generator/effect-ABI changes belong to that later backend unit. Root owns magic/integration/products. No concurrent shared-header edits.

The foundation retains existing compaction. It removes the unwinder's numeric
suffix contract: the compiler serializes physical homes of logical handler
survivors, and all cold consumers use explicit membership. Canonical join IDs
and contiguous call windows keep their established placement. There is no
frame-size, slot-count or initialization-elision claim.

The proposed scalar interval/relocation pass was implemented and tested, then
removed at `ba98006e236` after the bounded native comparison found identical
words and extents for all three ordinary source probes. Its additional compiler
bookkeeping had no demonstrated useful placement. Future coloring must first
show a concrete benefit; it is not a prerequisite for accumulator transport.
The arbitrary physical-layout contract has its own positive runtime fixture:
keep home 7 while dropping home 4, with the exception payload in home 9 and
GC/reentrant observation during cleanup.

## Concrete cold format

Use version **IRSC2**, no silent legacy translator. Empty semantic bytes remain canonical no-EH/no-source-recovery. Nonempty IRSC1 is rejected after the root-owned bytecode magic bump. Proposed 24B little-endian header: magic4, version32=2, exception-byte-count32, recovery-count32, keep-set-count32, keep-home-count32. Sections: exception varints, recovery records, 8B `{first32,count32}` keep-set descriptors, then packed uint16 physical home indices. All products and allocations check size additions/multiplication before pointer arithmetic.

An EH varint record gains a fifth field `keep_set` after existing start,size,target,slot_lasti. The exception payload may retain an adjacent lasti/exception pair as one explicit output bundle; its chosen physical slot is not an EH stack-depth frontier. Recovery records gain `keep_set` as their seventh int32 field (28B). Set0 is empty; intern identical sorted, unique scratch-only home lists. Locations remain optional IRSM2 with the same recovery-row IDs; source recovery semantics never come from line/column positions.

Assembler records each protected operation's logical handler-survivor IDs before allocation (the existing canonical values below `handler_base`), then serializes their allocated physical homes. Ordinary exception ranges coalesce only when target, payload, lasti and keep-set all agree. Source-only recovery edges capture the same logical survivor set at their actual semantic edge. A source recovery event must not borrow the destination instruction's unrelated EH map. No hot instruction looks up or interprets a keep set.

## Allocation and exact cold behavior

Keep every physical scratch cell initialized. Every owner take clears its old home and every publication maintains existing canonical-home invariants. Unread owners remain held until consumed, even if ordinary read-liveness ends. Do not free by last read alone. Pre-effect output aliasing rules remain those of the actual generated owner transport; a free hole that aliases a still-read input is not admissible merely because the operation will eventually consume it.

At an actual exception checkpoint, retain the reached HomeView/Storage authority already produced by the handler, including partial takes, published results and error restoration. Filter **that actual state**, never successful normal outputs, through the EH keep-set: all reached owned scratch except survivors is detached/NULLed and closed; bindings remain dynamic. Unknown checkpoints retain initialized-NULL control over readable scratch only. This is exact runtime ownership under initialized storage, not a fabricated static bitmap or a new error/normal certificate.

GC/copy/reentrant observation during cleanup must still see survivors at any physical index. The current numeric `remaining` clipping is insufficient after dropping below a retained higher slot. Use the reached readable selection for the whole cleanup and mark its ranges nullable: each dropped cell is detached/NULLed before a callback, so it remains safe to inspect while kept cells stay selected. Do not clip this view by the descending drop cursor. Preserve actual child-overlap exclusions and existing takeover/copy pinning; an absent/unreadable cell cannot become readable because it appears in a keep list. If a reached shape cannot support this nullable view, retain its complete initialized readable control rather than fabricate authority. No kept owner may disappear from roots because a lower physical hole was processed. Original exception is preserved around closes; existing redirect/error handling must choose fresh actual state after reentry.

After drops finish, install optional lasti/exception in their explicit vacant payload bundle, then enter the actual handler. If the handled monitor fails, restart ordinary exception processing from the new reached state/map; do not restore the old snapshot. On no handler, close every reached scratch owner, then perform existing binding/frame teardown. Physical descending drop order is permitted by the lifetime policy; it no longer asserts Python stack-order destruction.

## Calls, windows and stale storage

Contiguous call windows and overlapping argument binding remain. A live caller owner cannot occupy memory that the child header/locals/scratch may overwrite. Initially retain the existing invariant that live-across-call scratch lies below the window and place such windows above the actual owner-held set. This is an activation-overlap constraint, not an EH constraint. No new input copy or window move is silently charged as free.

Initialized arena frontier does not make an active child header a PyValue array. Parent GC/copy/observer/error views exclude the actual borrowed/overlapped tail while child active; child retirement restores NULL before parent tail becomes readable again. Retain current phase/owner recipes for this. SEND/FOR_ITER completion payload aliases are checked against explicit survivor/held sets, replacing `dst < handler_base` tests only when that set test is implemented.

## Common validation and reader ports

`Objects/codeobject.c` decodes and validates IRSC2 for compiler, CodeType/replace, marshal and exec-plan creation through the same path. Reject unknown version/flags, truncated or trailing bytes, out-of-range/overflowing set descriptors, duplicate/unsorted homes, binding homes, payload/keep overlap, invalid handler/edge targets or inconsistent optional recovery position counts. Word changes still require matching semantic metadata. Keep lists grant no nonNULL, canonical, owner or input-read fact; their nullable cells come from initialized storage and actual reached state. Existing semantic worklist and owner transfer rules validate handler entries, reachable read requirements and actual partial-error checkpoints.

Replace codeobject `semantic_handler_entry`, `semantic_exception_entry`, abrupt-floor/suffix propagation and completion exception-edge readers with explicit physical keep membership. A keep set says what is preserved; all other scratch is absent after cleanup regardless of address. Preserve owner/must-initialized/null facts independently. Unknown remains unknown; no opcode-name exemption or new whole-C-body string proof.

Targeted consumers found in actual source: `pycore_ir_exception.h` codec/iterator; `pycore_ir_source.h` observation join; `assemble.c` EH/recovery emission; `codeobject.c` semantic/owner/completion validation; `ceval.c` lookup/error scope/source-error recipe; `frameobject.c` resume/liveness/EH transforms/debugger redirects; `instrumentation.c` source recovery; `_opcode.c` public exception export; public disassembly/test-support decoders. GC, generator copy, suspended/captured frame views and remote layout readers must retain their reached HomeView authority. Accumulator changes are not part of these ports.

## Accumulator interface requirements for the backend unit

No sentinel is added by this first compiler unit. A later accumulator location must be a typed logical placement distinct from local/scratch/discard, with one canonical owned value; it cannot masquerade as a nullable scratch slot. Producer output and sole consuming use must be encoded/certified together. Calls, RETURN and SEND/yield deliver that owner to a typed continuation; common store/discard/branch consumers are generated into that continuation, not an added mandatory dispatch. Joins, multiple uses, observers, allocation and unknown providers materialize the owner before losing its anchor. The backend proposal must specify exact field/handler representation and effects before the compiler selects accumulator placements; no universal `IR_SLOT` sentinel branch or per-hit map interpreter is assumed.

## Small decisive checks and release gate

Host checks: codec roundtrip/overflow/shape negatives; explicit survivor/payload alias rejection; protected scalar overwrite and partial-error owner transfer; fixed windows/call live-across-owner preservation; source-only recovery using its own survivor row. Require a validated physical-image fixture with a drop below a retained higher EH home, without pretending pinned canonical IDs naturally emit it. New discoverable runtime tests (root executes) cover finalizer/weakref GC reentry during non-prefix drops, exception chaining/handled monitor failure, nested calls with overlap, generator throw/close and copied/captured frames, CodeType/replace/marshal malformed maps and source/debugger recovery. Reuse existing owner/error/phase tests rather than expand into an API census.

Root must integrate all cold readers before enabling free-hole placement, regenerate/review metadata/products, build both roles and run focused behavior. No runtime GO follows from compiler-only host syntax. Charge allocation metadata/compile cost and any changed call-window moves separately. Native acceptance of the later accumulator slice remains arithmetic + ordinary call + SEND/yield through full consumers, without borrowing a speed claim from this layout unit.

## Reviewed implementation clarifications

The reached HomeView gets a nullable override without widening its readable
ranges. Its per-view range count becomes uint16 with an explicit checked bound;
a uint16 flags field uses the remaining space, retaining the target 16-byte
view. Global plan offsets remain uint32. Shared readers honor the nullable
override. Retirement-only cursor clipping remains separate from ERROR scopes.

Keep membership does not grant initialized, non-NULL or owner facts. Kept homes
retain the prior unknown state unless actual incoming evidence proves more.
Existing abrupt-prefix and operand capabilities remain conservative; intersect
them with keep membership rather than broadening their source-effect claims.
Handcrafted test images must encode their intended survivors explicitly.
