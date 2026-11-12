# A: independent source post-review

Point-in-time review. The [final result](final-homes-result.md) records executed
qualification, performance rejection and the retained evidence.

Status: **current source/native mechanism and focused qualification accepted
for isolated timing**. Initial native regression is corrected, and actual
constructor routes are demonstrated. No performance approval is granted.
Reviewed the handwritten diff in `BASE/worktrees/activation53-final-homes`
against golden51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`, including the
untracked `Lib/test/test_ir_generator_construction.py`. Read A's proposal and
the corrected Astra contract. This review ran no generator, build, candidate
test, native job or benchmark. Root owns those serialized windows.

## Resolved producer correction

The added constructor call in `Python/ceval.c:3857` consumes a validated
`_Py_CALL_BIND_DIRECT` positional plan, but its actual producer at
`Python/specialize.c:2229` still explicitly excludes generator/coroutine code.
Therefore planned defaults/empty-variadic/kwargs construction was unreachable;
the semantic test for `planned()` can pass entirely through fallback. This is
a blocking coverage/interface finding in the first diff, not evidence of a semantic failure on
an executed direct plan. A and root accepted the finding.

Constrain the producer correction to the admitted immutable physical word-zero
RETURN_GENERATOR shape, IR_NO_DST recipient and a real successor; retain the
old nongenerative plan eligibility and async-generator exclusion. Do not
silently enable the old direct binder for arbitrary creation prefixes on a
constructor miss. The producer's static entry restriction bounds scope; it is
not authority to skip observers. Current executable/provider/hook/origin and
storage checks remain in the constructor. Root has authorized A to make this
correction. Its actual followup diff is now reviewed and accepted: the shared
`_PyCode_HasDirectGeneratorEntry` in pycore_ir.h requires exactly generator or
coroutine flags, physical immutable RETURN_GENERATOR, IR_NO_DST and length >1.
specialize.c admits those shapes while preserving old nongenerative eligibility
and async exclusion. The constructor reuses the shape fact and retains live
executable/provider/observer admission. Arbitrary prefix creation therefore
does not gain the old direct-binder route merely because the constructor misses.

The exact direct-success arm also bypasses `_INIT_CALL_PY_EXACT_ARGS`' stats-only
CALL hit increment. The followup restores it once in the new success arm.
This adds no release counter or benchmark claim. These two findings are closed
at handwritten source level; regenerated effects and compiled behavior remain
unqualified.

## Source findings checked

- Exact admission still precedes allocation and uses the checked function,
  argument-count, capacity and recursion prefix. The emitter's validator binds
  that prefix and window geometry. MISS reaches the existing single initializer;
  ERROR reaches the normal error label with intact PRE owners, rather than
  manually retiring an arbitrary-arity window before traceback/RAISE handling.
- The shared constructor pins the explicit executing code before allocation;
  shell type/size is selected from that code. The function's input home remains
  owning until transfer. GC allocation schedules collection in this GIL path;
  no poll/callback occurs between admission and seal.
- The shell has FRAME_CLEARED, names, weakrefs, exception links and executable
  initialized; coroutine origin is NULL before the constructor can return.
  Traverse skips physical homes while CLEARED. Allocation failures occur before
  moves. Prepared empty kwargs and the extra code pin cannot invoke user
  finalizers on these failure paths. No newly CREATED shell is decrefed there.
- Every physical final heap home is NULL before the one shared initializer;
  constant homes fill once. Supplied/default/empty-vararg/kwargs homes follow
  the validated positional layout and do not overwrite hidden constant homes.
  Moves NULL each source, including self when present; a no-self NULL spacer
  stays empty. The frame owns function and code independently before seal.
- Final state is previous=NULL, generator-owned/CREATED, successor of physical
  word zero and its proven initial recipient. No transient current frame,
  exception link, thread reservation or recursion entry is published.
- Exact success publishes actual recipient and continuation before
  displacement/discard can run Python, retains the executed CALL site and then
  reloads the actual return_pc. The
  known-plan finished result `{0, NULL}` has a matching generated completion
  branch that reloads PC and homes, rather than reusing stale next_instr.
- Both native adapters hold one explicit function owner and private argument
  owners. MISS passes the same owners to the frame API; ERROR retires them once
  with the allocation exception preserved and frees only scratch; SUCCESS frees
  scratch after ownership moved and returns the canonical result through the
  existing API. Explicit locals, keywords, unsupported counts and recursion/
  capacity limits decline. No tagged pointer or mixed frame/object return ABI
  was introduced.
- Origin tracking, active instrumentation version/code monitoring, trace/profile,
  PEP523, DTrace and debug LLTrace decline. Prefix execution is not bypassed.
  The debug globals lookup first requires Unicode-only dict keys, avoiding
  arbitrary key equality during admission.

No additional blocking ownership or error-path defect was found in this pass.
That statement does not approve uncompiled C or establish runtime behavior.

## Coverage and evidence limits

The global `instrumentation_version != 0` guard is intentionally conservative:
after monitoring/tracing has changed the epoch it can continue declining even
with hooks disabled. The followup splits exact and planned finalizer-redirection
tests, but a single module run can still execute direct-construction cases after
an earlier tracing test. Those later cases may test fallback only. Run the meaningful
direct cases in fresh subprocesses or individually in fresh interpreters, and
obtain diagnostic constructor hits with producer identity (including binding
non-NULL) before claiming direct coverage. The native-vector test now establishes
the executable before map invokes the constructor boundary. A new flags-only
code.replace negative verifies that generative flags do not authorize skipping
a physical word zero that is not RETURN_GENERATOR.

The new tests exercise language results, escaped frames, origin fallback,
warnings and redirecting finalizers; they do not themselves prove constructor
admission, allocation-failure cleanup or all malformed-entry negatives. Root's
candidate release/debug checks and negative producer checks remain necessary.
Host checks reported by A are not credited as native candidate passes here.

The initial source review required native evidence for one ordinary-call branch, unchanged miss
initialization, elimination of transient reservation/copy/pop and creation-only
native evaluator entry on actual hits, and record added common-path instructions,
spills and code size. Timing must qualify these costs independently of B.

## Initial native failure and observer correction

Root's first release build checks 117 modules. Its initial focused candidate
run executes eight construction tests and fails the exact displaced-owner
redirect case: it observes `['skipped', 'selected']` instead of `['selected']`.
Evidence is `E/final-homes-initial-release-focus.log`, where
E=BASE/pyjit-link-wave1/activation53. The same exact case passes golden51 in a
fresh process (`E/golden51-construction-redirect-reference.log`, reported by
root). The planned case passing later in the first run does not establish its
fast-path correctness, because tracing has already disabled constructor entry.

The defect is introduced by eager `frame->instr_ptr = next` in
call_finish_generator, before the displaced owner's finalizer installs tracing.
On the next INSTRUMENTED_LINE, prev_instr equals this_instr; the existing line
adapter concludes prev_line==line and suppresses the landing callback
(`Python/instrumentation.c:1888–1958`). This is a real observation regression,
not a flaky test or permission to weaken its expected redirect.

The actual correction is independently reviewed and accepted: remove that eager
instr_ptr write entirely. Ordinary CALL completion already leaves the executed
CALL site in instr_ptr during recipient publication and owner close; return_pc
holds the future continuation. Selected ordinary RETURN_VALUE and
call_transaction_publish_pc use this model. This differs from SEND/FOR's
committed semantic-step cursor, which must not be imported into ordinary CALL.
No post-callback reset to CALL is permitted: actual callback-selected instr_ptr,
same-PC redirect generation and winning return_pc remain authoritative.
The terminal continues to reload return_pc afterward.

The existing LINE adapter now receives the real predecessor and handles both
source-edge events and landing observation itself. No new line handoff helper,
source-coordinate execution dependency or source-map load on ordinary dispatch
is required. This review initially considered a generic committed-edge handoff;
that would overgeneralize SEND/FOR semantics and is unnecessary for this CALL
correction. The strengthened test also checks that the finalizer sees the CALL's
f_lasti, preserving selected ordinary-call behavior. It is written but not yet
credited as a passing corrected candidate test here.

Root owns the rebuild and fresh-process exact/planned regression runs. Until
they pass, direct-route qualification remains incomplete despite source
acceptance of this correction.

Root's corrected committed release at `dd5913961ec64ad0e814d62e172563570b9dbf2b`
now passes all eight construction tests, including exact redirect, in
`E/final-homes-fixed-release-focus.log` (reviewed). Fresh-process route-specific
tests and constructor-hit identities are still pending. The initial failing
log remains evidence of a caught regression, not a passing run.

The initial debug compile then rejected the compatibility PyDict_Contains in
the LLTrace admission branch. Root's followup
`4b339980d020ae4f6026474e8942b7174ce57d16` ports that one lookup to
`PyDict_ContainsValue(globals, _PyValue_FromObjectKnown(static_name))`. The actual
source diff is reviewed and accepted: it explicitly borrows the canonical
static string and retains the Unicode-only table guard and original status
handling. This is debug-only and does not change release admission semantics.
At that point the new release/debug rebuilds were pending. The following
section records the subsequent completed native inspection at that head.

## Current-head native review

Root subsequently declared both 4b339 builds ready, each checking 117 modules.
This review then read their ELF metadata and release disassembly without
executing either interpreter. Source remained clean at
`4b339980d020ae4f6026474e8942b7174ce57d16`. SHA256 was unchanged before/after
inspection:

| Image | SHA256 | ELF build ID |
| --- | --- | --- |
| activation53-final-homes-release/python | 1af23c6f86dc25f0ed7dcd59c945ea00bef39754ddd5f1e751387bfdd040b56c | 1ec0ef56803b03a97e33a01d9d43cf2cfea24255 |
| activation53-final-homes-debug/python | 67c6fd0b2d7121a517b88fbef81fb336e33c6c91e185ce1998bb9f0d39e4ae69 | 5492db15348e744680d35e63bd66e71a8d89725f |
| immutable round51-golden-release/python | 9f9c5548026783fa1802369d277243c561850fd472ce1d45275773b3143f3a3e | Not needed for the candidate identity claim |

Paths are relative to BASE. Native reads used `nm -S`, `readelf -n`,
`sha256sum` and `objdump -d --no-show-raw-insn --disassemble=SYMBOL`.

### Actual direct and miss paths

Exact CALL starts at `0x381d60`. The common generative-flags test at `0x381e84`
branches to existing reservation at `0x381eec` for ordinary functions. Generator
admission calls `_PyEval_TryMakeGenerator` at `0x381eb9`, before reservation or
header writes. MISS reloads stack/code geometry at `0x381ed0–0x381ee9` and joins
that same initializer. ERROR at `0x38221f` tail-jumps to the normal error label
with original window homes; there is no new input retirement loop. SUCCESS at
`0x3821ae` publishes recipient/return_pc and uses the existing result-owner
protocol, bypassing transient thread initialization and frame dispatch.
The corrected source behavior is visible: it does not store next into parent's
instr_ptr during completion. Actual return_pc is reloaded at `0x382200`, after
displaced/discard cleanup, and controls the next dispatch. Pending redirect and
deallocation remain cold arms.

The positional-plan handler genuinely contains its constructor call at
`0x382cb5`, with the plan in the sixth argument, and branches on MISS/ERROR/
SUCCESS afterward. Combined with the corrected static plan producer, this
establishes a reachable native implementation; it does not establish that a
particular workload exercised it. Root's binding-non-NULL hits remain required.

Constructor `_PyEval_TryMakeGenerator` starts at `0x381790`. It checks immutable
word-zero shape, current exec word and current provider/observer/recursion/
capacity admission before owner effects. Explicit-code shell allocation is at
`0x381947`; final physical homes are NULL-filled by memset at `0x38199d`.
One final header is initialized at `0x3819b5–0x381a55`; conditional constant
filling calls `_PyFrame_FillConstSlots` only at `0x381d3d`. Argument moves write
final homes and NULL original homes in `0x381a66–0x381a7f`; defaults/empty
variadics/kwargs follow in the plan arm. Success sets successor, initial
recipient, generator owner and CREATED state at `0x381b1a–0x381b42`, releases
the extra code pin and returns one canonical object owner. There is no thread
reservation, whole-frame copy or transient frame pop in this helper.

`_Py_MakeCoroFromCode` at `0x170940` obtains type flags and slot count from its
explicit code argument, calls GC allocation at `0x170975`, initializes CLEARED,
names, weakrefs, executable and exception fields, tracks the shell, and sets
coroutine origin NULL at `0x170a23`. It does not reread function->__code__ for
type or layout. Tracking and allocation remain necessary work.

The value-vector adapter calls the constructor at `0x127318`; success returns
at `0x12735d` through its normal epilogue, bypassing both frame push at
`0x127139` and `_PyEval_EvalFrameValue` at `0x12715f`. The object-vector adapter
calls at `0x38339b`; success reaches `0x3833d4` and the object return epilogue,
bypassing frame push `0x383154` and evaluator/hook entry. MISS retains those
original routes. Thus the creation-only evaluator entry really disappears on
native direct hits; this is not a source-wrapper inference.

### Common-path and size costs

| Symbol | Golden51 bytes | A bytes |
| --- | ---: | ---: |
| Exact CALL | 973 | 1316 |
| Value-vector adapter | 563 | 1114 |
| Object-vector adapter | 1437 | 2025 |
| New constructor | — | 1484 |
| New explicit-code shell allocator | — | 240 |

The listed symbols alone grow 3206 bytes; this does not include all other
inlined known-plan callers or unrelated linker layout. Exact CALL grows 343
bytes (35%). Its C-stack reservation stays 0x50 bytes.

A static path count for ordinary thread-owned calls, initialized storage below
the header, callee end no higher than saved top, mortal code, no constant homes
and a direct non-RESUME first-op dispatch is **151 instructions and 16
rbp-addressed memory operations**, versus **149 and 18** in golden51. Candidate
ranges are inclusive `381d60–381e88`, `381eec–381f36`, `381f3f–381f5a`,
`381f63–38204f`; golden ranges are `37a660–37a7ce`, `37a7d7–37a7f2`,
`37a7fb–37a8e7`. Counts include taken conditional branches but exclude their
skipped arms and contain no helper call on either path. They establish modest
actual common-path work and register-allocation changes, not a dynamic
instruction ratio or cycle prediction.

The direct constructor also adds its own normal-ABI prologue/saved registers,
guards and an extra code pin pair. It retains heap NULL initialization,
allocation, argument/default/constant owners and recipient retirement. Those
costs cannot be omitted from a work-removal claim. Native evidence establishes
the intended mechanism with no new source/native blocker; measured breadth
still decides admission. Root's fresh-process focused runs and GDB hit
diagnostics are running separately and are not pre-credited here.

## Final focused qualification addendum

Read the completed root logs at the same clean 4b339 head; no runtime, debugger,
build or timing job was started by this reviewer. These results close the
previous fresh-process and actual-route evidence gaps for the bounded screen:

- All eight construction methods pass individually in fresh release processes
  and again in fresh debug processes. This includes both exact and planned
  finalizer-enabled tracing/redirect regressions, so the planned pass no longer
  relies on a preceding test disabling direct admission. Evidence:
  `E/final-homes-fresh-{0..7}-{release,debug}-focus.log`, the corresponding
  focus-check JSON files, and `E/final-homes-fresh-cases-runner.log`.
- The nine call/generator/coroutine/yield-from/monitoring/source-reentry/completion
  modules pass 569 filtered tests in each role. Evidence:
  `E/final-homes-protocols-{release,debug}-focus.log` and its qualification JSON.
  This is focused qualification, not a full diagnostic suite.
- Both bounded constructor producer/effect checks pass in release and debug:
  two tests per role, recorded in
  `E/final-homes-producers-{release,debug}-focus.log`. The matching JSON records
  clean source identity, native paths and magic 64418.
- Four forced-allocation-failure probes cover generator and coroutine creation
  in release and debug. Each reports MemoryError, zero warnings and released
  input ownership at 4b339 in
  `E/final-homes-allocfail-{release,debug}-{generator,coroutine}.log`.
  This establishes the probed failure boundary; it is not exhaustive leak or
  allocator-failure coverage of every binding shape.

Root's GDB logs inspect one full unchanged workload loop after one warmup loop;
each inferior exits normally and records hash randomization zero and 4b339:

| Recording | Actual constructor route | Code identity | Entries |
| --- | --- | --- | ---: |
| final-homes-hits-generators.log | native value-vector | Tree.__iter__, flags 32, firstlineno 24 | 100000 |
| final-homes-hits-coroutines.log | exact IR | fibonacci, flags 128, firstlineno 14 | 242784 |
| final-homes-hits-positional-plan.log | known positional plan | separate probe, flags 32, firstlineno 4 | 64 |

These logs directly demonstrate the native producer and exact-child routes in
the actual kernels and separately close the formerly unreachable positional
plan finding. The coroutine census records 242785 starts/completions; the one
outer generic construction is not included in the 242784 exact-child entries.
Do not claim that every coroutine constructor used the new route or mix the
64-entry diagnostic probe into unchanged-kernel counts. GDB entry counts prove
route execution and code identity, not throughput.

The earlier failed native test and debug API compile remain retained alongside
their corrections. Source/native review and this focused evidence support
running the isolated A screen; the running balanced timing results, broader
regression gates and any later integration still decide performance admission.
