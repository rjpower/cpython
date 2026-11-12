# Native-only construction: independent post-review

Point-in-time review. The [final result](native-construction-result.md) records
executed qualification, performance rejection and the retained evidence.

**Source/native mechanism accepted for isolated screening, subject to root's
focused semantic and actual-hit qualification.** No throughput approval is
granted. Reviewed `BASE/worktrees/activation53-native-construction`
against immutable golden51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`, including
the untracked seven-test native construction module. No interpreter, debugger,
build, test or benchmark was executed by this reviewer.

## Source and ownership

The actual runtime diff is confined to Objects/genobject.c, Objects/call.c and
Include/internal/pycore_genobject.h. Python/ceval.c, bytecodes, specialization,
IR headers, generators and generated products remain golden51. Full A's
IR marker, plan producer/consumer, caller finisher and ordinary-call guards
are absent. The extra header explicitly includes pycore_value.h for its status
type, and debug admission uses the canonical PyDict_ContainsValue API.

The shared `_PyGen_TryMakeFromFunction` is a storage/ownership transaction,
not a frame-returning provider or caller-control adapter. It checks exact
generator/coroutine flags, optimized code, immutable physical word zero and
IR_NO_DST, a real successor, existing executable/current RETURN_GENERATOR,
exact positional count, no variadic/keyword-only layout and standard provider.
It rejects all prefixes/wrappers and does not interpret any creation prefix.
Current PEP523/trace/profile/monitoring/origin, recursion/capacity and enabled
DTrace/LLTrace checks precede effects. Free-threaded builds decline.

The caller supplies an explicit current code selected from its anchored function;
the helper pins that code before allocation. `make_gen` now accepts it directly
for type-independent sizing; existing `_Py_MakeCoro` callers pass their selected
code. No mutable function code reread chooses the new object's layout.

The GIL allocation/commit proof remains valid: allocation schedules GC but
does not synchronously execute Python. The tracked shell is CLEARED with valid
weakrefs, names, exception links and executable; coroutine origin is initialized
before the helper continues. No poll/callback occurs before seal. Allocation
failure predates argument/function moves, releases only the temporary code pin,
and returns ERROR with all original input homes intact. No CREATED shell can
be decrefed on this error path.

Every physical heap home is NULL before the existing shared initializer fills
the header and constants. Function and exact arguments then move once, with
each original home NULLed. Arguments occupy their validated binding prefix;
hidden constant homes retain the shared initializer's ownership. The sealed
frame has previous=NULL, the actual successor/input recipient, generator owner
and CREATED state. Its independent code owner makes release of the temporary
pin non-finalizing. There is no fallible step or callback-capable owner release
between the first move and this seal.

Objects/call.c keeps one explicit function owner and its original private
argument-owner array. MISS passes exactly those unchanged owners to the old
frame API. ERROR closes intact arguments/function once, preserving the original
exception across finalizers, frees only scratch and returns NULL. SUCCESS frees
scratch and returns its canonical result through the existing value API. The
object thunk retains its ordinary owned-object conversion. No IR recipient,
instruction cursor or continuation is changed by the constructor, so the
earlier full-A eager-CALL-PC defect is not present here.

No blocking source ownership, GC or semantic-admission defect was found.
Native guard/code-size costs and actual code execution remain to be verified.

## Omitted internal adapter and tests

Omitting `_PyEval_Vector` is correct. `_PyFunction_Vectorcall` is the existing
object-to-value thunk in Objects/call.c; public object vector calls already
reach `_PyFunction_ValueVectorcall`. The direct in-tree `_PyEval_Vector` users
are legacy EvalCode/Ex and build_class with explicit locals, outside this
constructor's admission. Keeping a new eligible arm there would add unexercised
code; no test-only internal API was added to manufacture a caller.

The new test module covers generator and coroutine calls through map and the
existing `_testcapi.pyobject_vectorcall`, executable warmup, mortal argument and
escaped-frame retention, initial send/throw/close, prefix and flags-only
fallback, origin/keyword behavior, unawaited warnings and a fresh subprocess
allocation-failure/input-retirement probe. These are meaningful semantics tests,
not proof of direct admission. Both API access routes share the same native
value-provider body. The allocation probe's weak reference checks the input
edge it creates, not every possible owner or allocation boundary.

Root must execute these tests in release/debug, retain fresh-process coverage
before persistent monitoring-version changes and demonstrate actual generator/
coroutine constructor success on both API access routes. Hook/LLTrace and
near-limit recursion/capacity declines still require appropriate focused
regressions; broad existing protocol/observer tests must retain their limits.
Full A's test and GDB results do not qualify this new candidate.

## Native review

Root supplied the release ELF after an initial 117-module build and seven-test
construction pass. During this review it committed the identical runtime body
and rebuilt getbuildinfo. The final source is clean at
`779a1568a41652736dbd4c93b968d3381a9693b5`. After that metadata rebuild, release
ELF SHA256 is
`9025ed28c04979feacc6a77c2a44961c454cde26cc235c8a9f261294a8893288`,
GNU build ID `f7ff2005ec5a0b66fd7cf10f4e35e42568e18996`.
The earlier initial ELF hash `ab260e8f...` is superseded; it is not the final
committed native identity. Native reads used nm, readelf and objdump only.

The actual `_PyFunction_ValueVectorcall` admitted branch calls
`_PyGen_TryMakeFromFunction` at `0x1272fa`. SUCCESS reaches `0x127348`, returns
its canonical owner and uses the normal epilogue. It bypasses temporary frame
push at `0x127140` and `_PyEval_EvalFrameValue` at `0x127166`. MISS returns to
the original frame API; ERROR enters the intact-array cleanup/exception-preserve
arm. The shared object thunk reaches this same provider rather than another
object-side constructor.

The constructor at `0x170950` performs immutable/current entry, exact-count,
provider/observer and stack/recursion checks before effects. It pins explicit
code, calls the private shell allocator, NULL-fills final physical homes at
`0x170b85`, initializes one final header, conditionally fills constants, moves
each supplied owner while NULLing its source at `0x170c4b–0x170c64`, then sets
successor/recipient/generator-owner/CREATED at `0x170c66–0x170c90`.
There is no temporary thread reservation, whole-frame copy, pop or evaluator
entry in that successful helper. It retains allocation, final NULL fill,
function/code/constant owners, a normal C prologue and the extra code pin pair.
Private `make_gen.isra.0` receives explicit code's already selected slot count
at its optimized boundary and subtracts the 14-word header before allocation;
its type argument and layout do not come from a mutable function code reread.

Address-independent disassembly comparisons against golden51 pass for both
exact CALL and RETURN_VALUE after normalizing only instruction addresses,
symbolic target addresses and RIP-relative symbol displacements. Opcodes,
registers, immediates and internal branch offsets remain compared. Their symbol
sizes remain 973 and 546 bytes respectively. `_PyEval_Vector` remains 1437
bytes. Combined with the source diff, this confirms omission of full A's common
IR admission/result changes; linker placement still changes absolute addresses.

Native costs are visible rather than assumed free. The value provider grows
from 563 to 1085 bytes, and its C-stack scratch reservation grows from 0x78 to
0xa8 bytes. Its owning-array pointer is now spilled and reloaded per argument
at `0x12708c`, where golden51 keeps that pointer in a register. The helper adds
1143 bytes and its own 0x48 scratch reservation plus saved registers. Ordinary
native calls also execute new shape/flags branching. These are reasons to
measure breadth; unchanged IR code does not imply zero runtime or layout cost.

No source/native mechanism blocker was found. Root's fresh release/debug tests,
allocation probes and actual-hit diagnostics are separate pending gates; the
initial seven-test pass is not substituted for them. Isolated balanced timing,
confirmation and breadth remain necessary before any promotion.
