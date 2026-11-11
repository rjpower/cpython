# Selected48 ordinary-call path audit

Read-only audit of selected source `5559f60093e8216d9399bea74d0f10c8cdfb9a6b`
and its existing release ELF. Upstream source is `3a440b65695615e9fd9912342b79cdd54fced0d2`.
No runtime changes, native benchmark executions, builds or commits. A follow-up
read-only native compile/disassembly of definitions is recorded below. This is a
static explanation and experiment menu, not a speedup claim or implementation
approval.

Source abbreviations below: `S` is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108`;
`U` is its sibling `upstream-3a440b6`. Selected ELF is
`build/execution-boundaries-release/python`. Existing profile evidence is
`build/pyjit-link-wave1/deltablue-call-model-20261110/final48-perf-record`.

## One complete common call

`BinaryConstraint.output` and `input` are ordinary unary instance methods;
benchmark lines 265–276 read `self.direction`, compare with `Direction.FORWARD`
and return `self.v1` or `self.v2`. The earlier semantic census observes 25,424
such calls in 51,422 total calls; it changes execution and supplies no selected48
optimized hit count. Its immutable body has RESUME, two predicate loads plus a
class load, conditional comparison and two field/return arms. It has one local
(`self`). Nothing about this shape warrants a benchmark-name rule.

| Phase | Selected48 warmed route | Upstream route / relevant difference |
| --- | --- | --- |
| Method resolution | `S/Python/generated_ir_cases.c.h:13211`: pointer-space, type-version and inline-values-valid checks; duplicate cached descriptor and acquire/transfer self into split call window. No bound-method object. | Same broad method-specialization design. This is already instruction-local feedback, not a global hash-table lookup. |
| CALL admission | `S/.../generated_ir_cases.c.h:7195`, DSL `bytecodes.c:4388`: PEP523, canonical callable/type, function-version, effective arity, stack-space, recursion. | `U/Python/bytecodes.c:4619` has the same essential function/arity/stack/recursion checks. Native upstream is one evaluator symbol, so flat symbol percentages are not equivalent-operation comparisons. |
| Storage | `S/Include/internal/pycore_interpframe.h:584`: point child homes at current argument cells; update downward frame limit and saved upward slot top. Initialized watermark may zero newly exposed physical storage. | `U/.../pycore_interpframe.h:404`: bump contiguous frame stack, copy arguments (`U/Python/bytecodes.c:4573`), NULL-fill remaining locals. Selected already removes ordinary argument copying and routine local zero-fill. |
| Header/owners | `S/.../pycore_interpframe.h:300`: preserve function owner, acquire independent code owner; previous/base, globals/builtins, observer locals/frame object, IR words/feedback/PC, return recipient and pending state. Constants filled when present. | `U/.../pycore_interpframe.h:189` also owns function/code and initializes derived globals/builtins/observer fields, but has a stackpointer and return offset rather than register bases/feedback and explicit recipient. |
| Enter/body | Publish child current, decrement recursion, load its IR, skip plain RESUME if instrumentation epoch matches (`ceval_macros.h:779`). Body performs ordinary guarded loads/compare/branch/return. | Upstream dispatches body bytecodes using stack operands and inline cache layout. Selected does not always pay a RESUME dispatch. |
| Return/result | `S/.../generated_ir_cases.c.h:16639`: transfer temporary result or acquire local result, unlink child, clear/pop, load parent slots and continuation, handle redirect/discard/destination replacement. | `U/Python/bytecodes.c:1574`: unlink, same clear/pop API, restore return offset and place result atop stack. Selected explicit destination requires replacing any previous owner and callback-aware continuation handling. |
| Retirement | `S/Python/frame.c:204`: escaped frame may take ownership; ordinary thread frame clears `co_nlocalsplus` homes, then function; `ceval.c:3103` clears code and restores stack geometry. | `U/Python/frame.c:114` similarly handles frame escape and locals/function. Successful selected thread return does not scan the full temporary bank; debug-only temporary assertions disappear. |

Independent code ownership is necessary for a real activation: the executing
function's `__code__` may change during callbacks. Locals must be NULL before
release, child must be unlinked before callbacks, and a surviving result must
stay owned through retirement. The header and exact destruction timing are
implementation choices, but deleting owners or state needed by readers is not.

## What the selected release compiler actually emits

Inspection used `nm -S` and `objdump -d --no-show-raw-insn --disassemble=SYMBOL`
on the existing ELF. These addresses identify this artifact only.

* Exact CALL is `0x37ad60`, size `0x582`. Its one `func_code` load at
  `0x37adf6` feeds arity, frame-size and initialization. Repeated source getters
  are commoned. `_PyValue_RecheckWindow` and the constant-NULL `ir_displaced`
  tail leave no dynamic instructions. Thread overlap takes the branch at
  `0x37aea3` into `0x37aea9`; the generator argument copy is cold alternative
  code. The `memset` at `0x37af14` is behind initialized-watermark comparisons,
  not unconditional per-call zeroing. Header initialization survives at
  `0x37af39` onward, with vector stores combining several fields. Code-owner
  acquisition survives at `0x37af58`–`0x37af84`. RESUME skip survives at
  `0x37b019` and `0x37b1c9`.
* RETURN is `0x381e70`, size `0x222`. It distinguishes local from temporary
  source, NULLs a consumed temporary, increments recursion and unlinks the
  child. There is a real call at `0x381ec9` to `_PyEval_FrameClearAndPop`, then
  parent destination/redirect/discard handling. `_PyEval_FrameClearAndPop`
  (`0x378c90`, size `0x13a`) really calls `_PyFrame_ClearExceptCode`
  (`0x3d0300`, size `0x438`). Those are two native boundaries, not merely DWARF
  inline attribution. They retain frame-escape/generator/profile/chunk paths.
* `LOAD_ATTR_SHAPE_LOCAL_TEMP` is `0x367120`. Pointer discrimination, current
  type-version equality, shape-byte equality and offset load survive. Its
  generated NULL-receiver retirement machinery disappears completely; there
  is no helper call on the hit. The current receiver type locates inline
  values; shape is a separate byte load, not a hash of type/shape/offset.

Thus source cleanup which merely passes a resolved code variable or removes a
dead generic retirement block has no demonstrated dynamic saving. Native
boundaries, guards, owners, slot accesses and dispatches are the useful units.

Existing selected48 DeltaBlue samples put 13.53% at exact CALL, 9.61% at method
lookup, 15.23% at the two shape reads, and 10.30% at RETURN plus clear/pop. These
are locations from roughly 2,000 samples, with zero lost samples; summing them
does not establish a causal fraction or an attainable speedup.

## Three general hypotheses worth discriminating

### 1. Make ordinary retirement direct; keep exceptional observation cold

Factor a shared common retirement path for a thread frame with no escaped
frame object, preserving NULL-before-close, parent publication, independent code
owner and profile-cache updates. Keep generator, escape and chunk transitions
in existing cold helpers. This targets all short Python calls, including
logging, without a new leaf gate or body language. The credible saving is two
native boundaries, repeated state loads and common-path branch structure;
there is no full temporary sweep to remove. One-local selectors are the
smallest representative test.

Proof required: native before/after shows fewer executed common-path
instructions; all frame-object ownership transfer and profile-cache updates
remain once-only; each callback sees parent current and valid homes; code
and frame geometry remain live through the final close; generator/error
completion uses the same ownership contract. Do not remove a branch merely
because the benchmark never takes it.

Cheap falsification: obtain representative exact call/return instruction
counts and local-count/escape census before implementation; if common helper
boundaries are a small fraction after inlining, stop. A minimal reviewed
ordinary-path refactor must beat selected48 on DeltaBlue and preserve logging
in matched measurements before any broader frame-layout change. This may be
a modest improvement: upstream also has out-of-line retirement.

### 2. Share checked facts across a callback-free region

The selector reads the same receiver twice, with scalar/class/global operations
between. Every ordinary shape read rechecks receiver type/version/shape and
decodes its own feedback. A general register-IR region can carry facts through
operations certified not to call, mutate or expose observers, rather than add
another lookaside table. This applies to repeated reads/comparisons in many
bodies whether or not their call activation survives. A compiler certificate
plus handler guard contract can identify a maximal valid fact interval; a
branch closes it unless both successors have a proved continuation.

Proof required: current type/shape/provider guards remain at region entry;
every intervening opcode is proven callback-free for the values actually
accepted; instrumentation/epoch changes and alternate handlers invalidate the
region; immutable metadata only stores scalar feedback units and code offsets;
normal failure preserves owned inputs and error attribution. Type-version
equality alone does not make instance shape immutable across arbitrary code.

Cheap falsification: enumerate repeated same-receiver guard sequences in the
warmed DeltaBlue IR and count the guards a valid interval could remove. If
the first useful interval cannot exclude callback/mutation without repeating
the same guards, or retained profiles locate little work there, stop. Do not
implement a general optimizer based on the selector's source size alone.

### 3. Compile an admitted region once; avoid a per-hit miniature evaluator

If activation and retirement dominate, a certified straight-line/conditional
region may remove them. Construct executable region facts during code
construction/specialization, resolve immutable feedback geometry once and use
ordinary mutable guard providers. Dispatch the admitted CALL directly to its
region handler, leaving ordinary exact calls untouched. A shared committed
caller/result-retirement contract must precede activation omission. This is
the general direction of C2, not proof that C2 wins or a request to reuse C49.

Proof required: complete body coverage, current function/code/observer/recursion
admission; normal misses before effects; one owned result; coherent caller
before any final close; bounded ownership implementation without replay;
metadata/handler invalidation; all retained ordinary calls have unchanged hot
work. Reusing a checked exact prefix must remove checks from the actual native
region entry, not just rename them.

Cheap falsification: compare the full proposed hit ledger with the existing
ordinary selector ledger above before coding. C49 has already falsified the
generic runtime transaction: despite 25,504 elisions it regressed DeltaBlue
15.2%. If a new design still performs per-read generic metadata decoding,
repeated admission and a dynamic alias ledger, reject it on that evidence.
Use one bounded general region and unchanged workloads for any next experiment;
do not expand grammar to seek coverage before the first region is cheaper.

## Suggested decision order

First attribute the surviving exact CALL and retirement instructions using the
selected48 capture, not C49. Compare one full selector path to upstream with
equivalent boundaries; upstream's one evaluator symbol cannot answer this from
flat samples. Choose the smallest hypothesis that removes a demonstrated
cost. A fresh instruction-local lookaside cache is not the default: selected
method/shape/CALL sites already have local feedback and compare scalar version
or shape fields. Cross-instruction fact reuse or activation omission requires
stronger semantic proofs than replacing multiple fields with a hash integer.

## Follow-up: adjacent method lookup and ordinary CALL

This fourth direction retains an ordinary child, body, return and frame ABI.
It removes one dispatch, split callable/self stores/reloads, and admission
work whose proof lookup supplies. It removes no code owner, header, body or
retirement. Function-version, arity, recursion, stack and observer checks remain.

Compiled unchanged DeltaBlue with selected48 and recursively disassembled
immutable code objects without executing the module/workload. This small
read-only command used CPU14, separate from timing CPU15. Of **180 static CALL
words, 43 immediately follow method-form LOAD_ATTR with CALL arity zero**:
11 output, 5 input, 6 execute, 6 is_satisfied; 2 each add_constraint, is_input,
mark_unsatisfied, new_mark, recalculate, remove_from_graph; 1 each add_to_graph,
destroy_constraint, next_weaker. There are 58 total adjacent LOAD_ATTR/CALL pairs;
the remaining 15 are argument-expression attributes and do not qualify.

Actual immutable offsets: `EqualityConstraint.execute` has input at LOAD_ATTR
1 / CALL 2, output at 4/5. `BinaryConstraint.recalculate` has input at 1/2,
output at 3/4, execute at 15/16. `Planner.make_plan` has output at 13/14,
27/28 and 31/32. The dominant selector family therefore has real adjacency;
the earlier 25,424 semantic calls are an opportunity bound, not native admitted
coverage. Polymorphism, observations and window/control-flow constraints remain.

Reproduce with selected `python -S -B`: compile
`pyjit/bench/internal/kernels/deltablue.py` without exec; recursively traverse
`CodeType` constants and `dis.get_instructions(..., adaptive=False)`; count
consecutive LOAD_ATTR with `NULL|self` in `argrepr`, then CALL with `arg == 0`.

**Compiler fusion** creates one real combined operation before final assembly,
updating liveness/window roles, source/exception maps, feedback and generated
consumers. No NOP filler belongs in the stream. Compare/jump fusion at
`Python/regcfg_passes.c:1125` supplies precedent for same-block, precise def-use,
same-line and same-exception-range constraints. Method/call needs both source
origins or conservative matching because lookup and call can fail separately.
It also needs a normal observed/deopt representation. This is the cleaner
persistent representation but reaches more compiler/observer consumers.

**Two-word quickening** keeps both original executable words and identities.
The installed first handler consumes the real next CALL and advances by two;
the second is not replaced by NOP. Check its current opcode and payload.
Prohibit alternate entry into its interior from branches, exception handlers,
redirects and instrumentation, or prove a valid captured window for such entry.
Disable the pair when either word is instrumented or changed. Adjacency alone
is insufficient installation proof.

Initial payload must be an exact Python function from guarded ordinary instance
lookup with valid managed values and no shadow. Present METHOD_WITH_VALUES
checks `Py_TPFLAGS_METHOD_DESCRIPTOR`, broader than exact Python function.
A fused specialization must establish exact function positively before deleting
CALL's type/tag checks. Live function-version and arity checks remain: zero
explicit arguments means fixed self and `co_argcount == 1`, not immutable
function code or unrestricted defaults/vectorcall behavior.

Choose one once-only fallback protocol:

1. Check lookup and current CALL/function/code/stack/recursion facts before any
   publication or callback. Failure uses ordinary LOAD_ATTR with receiver
   intact. Success acquires function/self owners and directly commits the
   overlapping ordinary child. No release, observer callback or fallible
   preparation may intervene between proof and commit. This removes pair
   traffic as well as dispatch.
2. Capture the owned split pair with normal lookup, then execute CALL without
   dispatch. Later CALL guard failure resumes ordinary CALL at its own PC
   using the captured pair. It must never restart lookup. This smaller first
   increment saves dispatch but retains pair traffic unless forwarding is
   explicit.

Do not mix these protocols. After capture/effects, replayed lookup could select
a different target or repeat descriptor behavior. Properties, custom access,
arbitrary descriptors and unexpected callables take ordinary lookup. LOCAL
and temporary receiver ownership, including exact overlap geometry, remains
explicit. The callback-free specialized method hit enables preflight fusion;
generic attribute lookup does not.

Caller must name original CALL PC before child entry and continuation after
CALL. Lookup errors belong to LOAD_ATTR; call errors to CALL. Monitoring
CALL/instruction/line and profiling need ordinary execution or preserved
origins. `INSTRUMENTED_CALL` at `bytecodes.c:4212` has distinct monitoring
semantics: checking only the first opcode is insufficient. PEP523 and callee
RESUME epoch/interrupt checks remain. Caller frame escape, callback locals and
redirects must see complete ordinary frame/window state.

Cheap gate: census the 43 sites for actual METHOD_WITH_VALUES plus exact
Python CALL hits, compatible windows/ranges/locations, no interior targets and
plain observation. If most selector calls qualify this has a sound coverage
case without C49's body evaluator/owner ledger. Before coding, review the
combined native ledger and exact point where fallback switches from LOAD_ATTR
to CALL. Test monkey-patching/bases/shadowing, descriptor/code/default mutation,
monitoring/tracing/PEP523, recursion/stack limits, redirects, temporary receivers
and local/discard results. Measure whole DeltaBlue and logging. Benefit is
bounded by lookup/entry work, not all frame costs.
