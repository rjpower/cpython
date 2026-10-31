# H3-1 stage 2b.2b: displaced monitoring storage

Status: design-only handoff against frozen
`dc02f76c0ccf69cd7195431dc0335a349bdec48f`. The coordinator authorized eight
bounded AST probe children, recorded below; no native/test source, build or
configuration was changed. Fixed-line storage is the single recommendation,
pending independent storage review and exact shared-file grants. H1 remains
in its separate H1-1.md. Later H3 stages remain separate.
The coordinator recorded the private API/layout policy in primary
`db96fc12f15` (PROCESS section 0.2); that direction does not authorize this
implementation or replace the corrected shared-hunk/ROOTS review.

This refines the storage part of H3-1-stage2b2.md, whose accepted correction
has SHA256 `33cef3ae191078a1f578df57f9ac9dae3fee7143f4d214b6510ca9add310bda7`.
It supersedes that document's relative/variable-width line codec and its
stricter wording about suppressed DISABLE callbacks, and makes the local
OOM disposition explicit.

## 1. Prerequisite outcome and boundary

Stage 2b.2a now reads only immutable co_linetable bytes in SafeAddr2Line,
with the negative-offset firstlineno rule and a preceding nonnegative
bounds check. Its actual real-word controls separate ordinary instruction
heads from synthetic relaxed jumps. Both builds' copied old-cache control
fails at valid FOR_ITER word 5 and SEND word 8; direct lookup now returns
the immutable line there. Its frozen record preserves final smoke/affected/
strict-kill evidence and the private export needed for the real CAPI hook.
It does not repair the emergency reader's existing dead-frame heuristics.

Consequently this storage change adds no asynchronous monitoring-array
reader. The owner/GIL, public Addr2Line acquire/release protocol and
code-owned array lifetime described in the earlier design still apply.
Ordinary mutations are aligned uint16 stores under the owning interpreter's
GIL; empty FT/StopTheWorld wrappers are not evidence of additional exclusion.
No new atomic word, global lock, fallback representation or runtime option
is proposed. SafeAddr2Line and its frozen tests stay unchanged.

Keep every handler ID, format and A-D field unchanged. LINE stores a
uint16 original-token array separately from immutable fixed int line entries;
INSTRUCTION becomes a uint16 array. Tool/event masks remain uint8. The
header/int-line allocation plus aligned original-token allocation avoids
unaligned casts into the old packed byte layout. Free the nested LINE token
allocation only with code-owned monitoring teardown.

## 2. Fixed line entries, canonical relations and ABI

Recommend one format, with no runtime width choice:

```c
typedef struct {
    uint16_t *original_opcodes;
    int lines[1];
} _PyCoLineInstrumentationData;
```

Each line entry is -1 for None, or a nonnegative C int source line, including
0 and INT_MAX. Real instruction heads keep the immutable lookup's absolute
line value; trailing synthetic words retain the cache-specific None rule
below. There is no firstlineno subtraction/addition, reserved -2 delta,
byte packing or
per-lookup decoding. Line zero must never be mistaken for None. The token
array keeps its separate zero eligibility sentinel. A legitimate decoded
negative value other than None would require explicit review disposition;
do not silently narrow/coerce it. No constructor/AST ordering restriction is
invented to justify the old delta range.

Initialize every slot before publication: fill line slots with -1 and
original tokens with 0, then visit real instruction heads using the existing
instruction lengths. Store each head's immutable absolute line, even when
the head is not eligible for LINE. Preserve existing first-traceable,
line-change, branch-target, exception-handler and excluded-op eligibility
rules separately in the original-token array. Every relaxed trailing
IR_JUMP word stays cached -1/token0, even if co_lines covers it with the
loop's positive line. This preserves the intentional cache/SafeAddr2Line
distinction proved by 2a. Do not blanket-copy co_lines into every cache slot,
replace absent-line fill with zero, or infer eligibility from line!=None
alone. Branch/exception eligibility scans change tokens, not line values.

Allocate `sizeof(header) + nwords * sizeof(int)` for header/lines, preserving
the project's trailing-[1] convention and explicitly accounting for its
spare element/padding, plus `nwords * sizeof(uint16_t)` for originals. Check
PyMem limits before multiplication/addition. INSTRUCTION allocates by its
own uint16 element size, not sizeof(LINE header). The latter is currently
2 bytes but becomes 16 on this ABI; retaining the old mistaken expression
would overallocate INSTRUCTION eightfold.

Keep `_PyCode_GetOriginalOpcode` and the pure setters
`_PyCode_SetOriginalOpcode` / `_PyCode_SetMonitoringLine` together in the
internal header, used by the actual production paths and primitive fixture.
Metadata/canonical admission is separate from primitive token storage.
Replace delta helpers/NO_LINE=-2/width selection with direct line access;
line tests become boundary/adjacency tests, not codec-width tests.

| Symbol | At dc02 | Proposed | Current in-tree use |
|---|---|---|---|
| _PyCode_Deinstrument | `PyAPI_FUNC(uint8_t) _PyCode_Deinstrument(uint8_t opcode)` (pycore_instruments.h:131; instrumentation.c:249) | `PyAPI_FUNC(int) _PyCode_Deinstrument(int opcode)` | Source search finds only declaration and definition, no caller. It is exported for external JIT support. |
| _Py_Instrumentation_GetLine | `extern int (PyCodeObject*, _PyCoLineInstrumentationData*, int)` (header:122; implementation:1346) | `PyAPI_FUNC(int) _Py_Instrumentation_GetLine(_PyCoLineInstrumentationData*, int)` | Five calls: codeobject.c:1037, instrumentation.c:1366/1370, legacy_tracing.c:418/419. The new private CAPI fixture also calls the real production function. |

GetLine becomes a direct int-array load; it retains nonnull/nonnegative
assertions and its precondition that the caller supplies a valid index.
Remove the unused code argument entirely. Move the existing public
PyCode_Addr2Line bounds assertion **before** its monitoring-pointer/cache
branch, after the negative-offset return. This explicitly preserves
bounds-before-access in debug; the public C API's existing valid-offset
precondition remains unchanged in release. That one assertion covers both
cache and fallback branches; no duplicate post-cache assertion is needed.
At both instrumentation callers assert 0 <= i/prev_index < Py_SIZE(code)
before their respective decode. At both legacy jump decodes preserve the
existing nonnegative from/to assertions and add upper bounds against the
owning code before either decode. Code remains their frame/API lifetime
anchor. The primitive fixture validates its selected index against
Py_ARRAY_LENGTH of its actual allocated originals and uses only the three
corresponding line slots. No fake code object or redundant code parameter
is added merely to satisfy the decoder signature.

The exact five call-site edits proposed are:

```c
/* PyCode_Addr2Line: after its existing negative-offset return, before
   loading _co_monitoring; remove its old later copy of this assertion. */
assert(addrq >= 0 && addrq < Py_SIZE(co));
/* ... in the existing lines != NULL branch ... */
return _Py_Instrumentation_GetLine(lines, addrq);

/* _Py_call_instrumentation_line: before decoding this instruction. */
assert(i >= 0 && i < Py_SIZE(code));
int line = _Py_Instrumentation_GetLine(line_data, i);
/* ... after computing prev_index and before decoding it ... */
assert(prev_index >= 0 && prev_index < Py_SIZE(code));
int prev_line = _Py_Instrumentation_GetLine(line_data, prev_index);

/* sys_trace_jump_func: after existing nonnegative from/to checks,
   its unchanged forward-jump return, and PyCode_Check(code). */
assert(to < Py_SIZE(code));
assert(from < Py_SIZE(code));
int to_line = _Py_Instrumentation_GetLine(code->_co_monitoring->lines, to);
int from_line = _Py_Instrumentation_GetLine(code->_co_monitoring->lines, from);
```

The two legacy calls are active production paths: legacy_tracing.o is in
Makefile.pre.in:489 and setup_trace_callbacks registers sys_trace_jump_func
for JUMP at legacy_tracing.c:664-666. The previous three-caller inventory
was incomplete and is superseded by the full tracked-source audit.
Do not move/remove the to>from early DISABLE, the differing-line DISABLE
that leaves the target LINE wrapper responsible, the f_trace_lines check,
or same-line backward-edge trace delivery. trace_line (344-364) already
suppresses negative lines; preserve that behavior for absent source/target
locations rather than adding new endpoint line>=0 assertions.

A unit assertion that an emitted LINE value belongs to non-None co_lines
values applies to valid eligible LINE sites. It is not a universal storage
invariant: the generic getter/primitive fixture must round-trip -1, and
previous locations or legacy jump endpoints may be None. Preserve the
existing current-line>=0 assertion in _Py_call_instrumentation_line because
the installed wrapper requires an eligible real line; do not extend it to
prev_line. Controls must cover real heads with token0, real None entries,
line0, both relaxed FOR_ITER/SEND trailing words (-1/token0), and the separate
2a safe lookup's positive immutable result at those same synthetic words.

Affected legacy tests include test_sys_settrace's test_11_tightloop,
test_12_tighterloop, test_15_loops and
test_async_for_backwards_jump_has_no_line, plus inherited no-line/opcode
tracing variants, frame.f_lineno jump tests, and test_monitoring's combined
LINE/INSTRUCTION/backward-assignment controls. Add a focused forced-relaxation
legacy tracing regression if existing cases do not witness both endpoints
and cached None; preserve actual event sequences and repeated same-line
backedge events. No legacy source/test edit is authorized by this document.

Changing GetLine's declaration to PyAPI_FUNC supplies the visibility needed
for that real shared-extension test call, as with 2a's safe helper. Record
both private signature changes and the new test-symbol visibility in the
coordinator-owned PROCESS ledger; external private/JIT users rebuild. No
public stable ABI promise or byte-prototype adapter is added.

Deinstrument checks the original int for generated metadata validity
before indexing, base lookup or narrowing. Return canonical base for an
ordinary instrumented counterpart, 0 for a valid metadata token without a
removable counterpart (including transparent wrappers), and -1 for an
invalid scalar/hole. This does not establish pristine/dispatch admissibility.
Generate canonical install/deinstrument relations from explicit ordinary
INSTRUMENTED_ pairs, validating formats/kinds. Minimal DSLs do not acquire
dummy unrelated instructions. Use explicit self-install/no-deinstrument
entries for LINE/INSTRUCTION wrappers, generated base normalization for
aliases, and generated kind instead of this consumer's numeric cutoff.
Canonical BINARY_OP can have metadata without dispatch; only the installed
handler requires dispatch/state validity. Consume every checked result.
Final exported threshold removal remains stage 4.

### Bounded evidence and memory decision

The authorized script is retained with SHA256
`7feccf21ac66fb3f2da3a205a5d8e194d7446b0d70c72fa625f06dae8af0b612`;
unexecuted v1 is preserved at its original 4ab3de7... hash. Both builds use
the dc02 final binaries/configs and unchanged source hashes in the per-build
manifests. All eight children recorded optimize=0, __debug__=True,
verifier=(0,0), stress=0 and RLIMIT_CORE=(0,0). Inherited PYTHON_GC* and
PYTHONOPTIMIZE were removed. Fresh caches, normal machine slots/build locks,
180-second outer ceilings and 120-second child ceilings were used. No full
qualification, rebuild or new gate profile is implied.

| Valid compiled input | Release outcome | Og outcome |
|---|---|---|
| Function first line 10, return line 1 (delta -9) | Emits invalid LINE 257; semantic assertion fails | Aborts enabling LINE: compute_line_delta assertion |
| Function first line 10, return line 8 (delta -2) | No body LINE event; semantic assertion fails | Same delta assertion |
| Normal module, first line 1 with RESUME line 0 | Emits body line 1; passes | Passes |
| Function first line 1, return line INT_MAX | Emits INT_MAX; passes | Aborts in get_line_delta encoded-value assertion |

Every AST compiled and its immutable co_lines/safe lookup agreed before
monitoring. The source assumption therefore fails on actual accepted ASTs,
not malformed words/pointers. The old codec only permits real delta >=-1;
its decoder's encoded<INT_MAX assertion additionally limits delta to
INT_MAX-3. Merely widening that upper limit would still reject the observed
backward locations. This is why the relative codec proposal is withdrawn.

For n words on the qualified x86-64 ABI (int=4, pointer=8), the explicit
LINE requested-byte formulas are:

- Old packed relative cache: `1 + n * (1 + d)`, d=1..4.
- Considered variable absolute cache: `16 + n * (2 + w)`, w=1..4.
- Recommended fixed-int cache: `16 + 6*n`.

The two proposed headers each have layout-derived sizeof 16 on this ABI.
Fixed costs `n*(4-w)` bytes more than variable absolute: zero to 3n bytes.
Both proposed formats require two LINE allocations instead of the old one.
INSTRUCTION's actual requested size remains 2n: its old accidental header
size was already 2. Common monitoring header, masks, executing copy and
allocator overhead are excluded here and must remain explicit in memory
evidence; code_sizeof does not count this storage.

| Probe-measured words/location inputs | Old source allocation formula | Variable absolute forecast | Fixed forecast |
|---|---:|---:|---:|
| Each backward case, n=3, lines 10 and 1/8 | 7 bytes | 25 bytes | 34 bytes |
| Module, n=5, lines 0 and 1 | 11 bytes | 31 bytes | 46 bytes |
| Extreme case, n=3, lines 1 and INT_MAX | 16 bytes | 34 bytes | 34 bytes |

[allocation-model.json](controls/H3-1-stage2b2b/allocation-model.json)
reproduces these source/layout forecasts from measured probe inputs.
They are **not** measurements of unimplemented new allocator requests,
RSS or peak memory; broken old-cache cases are not correctness baselines.
No measured memory evidence here outweighs deleting the codec, so fixed
int is the single recommendation. After implementation, record actual
requested bytes/allocation counts and preparation/failure peak storage for
representative low/high line ranges. No policy cutoff selects a second form.

## 3. Failure and publication disposition

The concrete local path is monitoring_set_local_events_impl (instrumentation.c:
2439-2478), its pre-mutation LiveRunQueue drain, _PyMonitoring_SetLocalEvents
(2076-2105), force_instrument_lock_held (1827-1956), and
update_instrumentation_data (1723-1824). Currently the C setter writes
tool_versions and local masks before preparation; a subsequent same-mask
call returns early. If an allocation failed after the mask write, that retry
can skip the refresh. This is a source-derived risk, not a reproduced result.

Proposed local disposition for review: include the requested local mask,
stale-tool cleanup and tool-version change in the pending per-code plan.
On failure preserve those published values as well as executing words,
backing-array presence, active masks and code version. A valid empty
monitoring header/executing copy may remain as an independent allocation.
An identical retry then still sees the old local request and prepares the
requested update. The equality fast return is allowed only after stale
tool-version/request state is reconciled; it cannot discard a failed update.
This explicitly strengthens local failure atomicity; local docs do not
themselves specify rollback after MemoryError.

Prepare absent arrays privately and fully initialize them from old installed
state. Allocate every required block before rewriting any existing state.
On failure free all unpublished blocks. On success publish the complete
header/backing arrays, candidate local/active masks and tool versions,
rewrite wrappers and finally publish the code version. Calculate removed/
new masks from the old-active snapshot; the single-tool removal helpers
must observe the intended snapshot rather than accidentally the new masks.
No recoverable allocation or Python call is allowed in this commit interval.
Arrays remain code-owned across callbacks, DISABLE and restart.

Global behavior stays separate: _PyMonitoring_SetEvents (2050-2073) changes
interp monitors/global version before instrument_all_executing_code_objects
(1987-2007) walks frames. It can fail after earlier codes refreshed. Do not
promise whole-interpreter rollback. get_events may expose the requested new
mask; a stale code must remain structurally valid and retry on its next
instrumentation/version check. The same-mask global API early return is not
a per-code refresh proof. Record requested masks, per-code active state and
retry behavior separately in failure evidence.

## 4. DISABLE contract and membership controls

Local sys.monitoring docs specify independent tools (lines 35-38), local
DISABLE at a location without changing configured masks/other locations
(330-344), and restart of disabled events for all tools (349-352).
instrumentation.c:3012-3014 explicitly allows spurious post-DISABLE events
while implementing the legacy BRANCH adapter. This source allowance must
not be converted into an undocumented zero-extra-callback guarantee for
LINE/INSTRUCTION representation changes.

The hard oracle is preservation of an enabled tool that never returned
DISABLE: it must continue receiving its previously observed deterministic
sites after another tool/event causes multi-tool storage to appear.
Allowed extra disabled-site callbacks cannot excuse loss of that membership.
Seed new masks from old installed wrapper layers and old active masks,
preserving disabled-site suppression where observable as an implementation
choice, but do not make zero spurious callbacks a public-contract assertion.
Do not infer installed LINE state merely from a nonzero eligibility token.

First controls, each on fresh deterministic code with code-filtered callbacks:

1. Enable A's INSTRUCTION, establish A's site sequence without DISABLE, then
   add B's INSTRUCTION. Both receive their expected sites; remove B and A
   in both orders. Current per_instruction_tools initialization to zero
   (1813-1819), followed by adding only the new B mask, predicts loss of A.
2. Enable A's LINE+INSTRUCTION, then add B's LINE. That makes multiple_tools
   true and allocates an INSTRUCTION tool array even though no INSTRUCTION
   tool was added. A's existing instruction membership must survive.
3. Mix one DISABLE-returning A site with still-enabled A sites, then add B
   for the same event. Assert delivery for A's never-disabled sites and B's
   required sites; record any extra A callback at disabled sites without
   failing solely for that reason. Repeat for LINE and INSTRUCTION and with
   their wrappers nested. Explicit restart must make A's disabled sites
   observable again. The old line_tools initializer (1686-1695) copying all
   requested masks may give extra disabled callbacks; that alone is not a
   hard correctness failure.

Retain exact old behavior before changing initialization, but do not call
these predictions reproduced until a gated probe actually runs. Keep the
real CALL/C_RETURN/C_RAISE collecting oracle, exact pristine ID+A-D restore,
exception propagation and callback reentry tests. Existing raw NB/NOFB IDs
can exceed 255; their displaced canonical tokens currently do not. Primitive
65535 tests therefore do not claim live execution of a high canonical token.

## 5. Exact shared-fixture request and failure witnesses

[storage-fixture-proposal.patch](controls/H3-1-stage2b2b/storage-fixture-proposal.patch)
is an unapplied additive proposal against dc02's _testinternalcapi.c:
one pycore_instruments.h include, three functions after code_safe_addr2line,
and their three method entries. It does not alter any existing helper,
method entry, fixture or source file. It is coherent only with the proposed
header layout/signature changes; do not cherry-pick it as a standalone hook.
Each additive block needs coordinator/ROOTS review and an exact-hunk grant.
All three hooks use the existing legacy METH_VARARGS ABI, with explicit
iii / i / O! tuple parsers respectively. This fork's METH_O uses the PyValue
ABI; no PyObject-returning METH_O entry or unreviewed cast is proposed.
The distinction is explicit in methodobject.h:19/26 and
methodobject.c:510-536/572: METH_VARARGS returns PyObject*, while the METH_O
vectorcall entry accepts/returns PyValue. Check scalar hook calls using
ordinary inline integers and state calls using real code objects when the
coherent implementation is authorized. The rejected ABI proposal was
never applied or executed.

- monitoring_storage_roundtrip(index,opcode,line) constructs aligned
  stack-only token/int-line storage, uses the actual shared setters and the
  real exported GetLine, and returns three adjacent (token,line) pairs.
  Index is validated against the actual three-slot allocation; input tokens
  remain int until 0..65535 admission. Test indices 0/1/2, tokens
  0/255/256/511/65535, and line values -1/0/1/8/10/INT_MAX, preserving adjacent
  values. Reject negative indices, index==nwords, token -1/65536 and line
  below -1; Python-to-C int overflow rejects INT_MAX+1 before access.
  No token is dispatched and no width descriptor remains.
- monitoring_deinstrument(int) calls the real new boundary. Test -1, holes,
  65536 and 65536+INSTRUMENTED_CALL before any cast, plus every actual domain
  token's specified outcome. A copied cast-before-check variant must fail
  the high-alias rejection assertion without indexing outside the table.
- monitoring_state(code) is the separately requested read-only minimal
  snapshot: copied local/active mask bytes, copied tool-version bytes, code
  version, and five backing-array presence booleans. It returns no pointer,
  borrowed array, raw address or array payload. Copy ALL requested bytes,
  version and flags into stack-local primitive snapshots before the first
  Python result allocation; Py_BuildValue consumes only those copies.
  The owning GIL covers that allocation-free capture. Result construction
  can reenter monitoring through GC/queued Python, so neither the GIL nor a
  live code argument is used as a coherence proof across that construction.
  Header bytes are initialized; no uninitialized event-array holes are
  sampled. Failure hooks are removed before capture/result construction.
  It proves presence/header state, not byte equality
  of every backing-array element; runtime membership/restoration oracles
  and source review cover reachable contents and cleanup.

Copied truncation controls use exact new getter/setter/fixture bodies and
one explicit uint8 patch: token 256 must mismatch, as must an int line above
255. Require semantic assertion failure rather than compile/crash failure.
The live fixture calls real GetLine; copies provide negative isolation,
not the only positive proof. Primitive storage deliberately accepts tokens
outside metadata; installed-token admission remains separate. The retained
2a copied old-cache controls describe their frozen old layout and must not
be rerun against the new layout or relabelled as current evidence.

For failmalloc, precreate the target executing copy, tool IDs, callbacks and
all arguments, and drain pending finalizers before arming the existing
_testcapi.call_with_nomemory. It prebuilds its call tuple before arming and
disarms before returning (mem.c:214-245). Use the real public local setter;
its pre-mutation queue drain still exists and must not be mislabelled an
allocation-free guarantee. Run isolated deterministic transitions without
new finalizer-bearing objects in that interval. Do not add a global hook.

Sweep persistent failure start ordinal k from zero until the first successful
call, using fresh transition state each time; later allocations cannot hide
behind a recovered one-shot failure. Keep the normal gate deadline, record
every ordinal/outcome and do not infer exact allocation-site attribution
without evidence. If source/counter evidence cannot attribute a new block's
failure, report that coverage gap at the checkpoint rather than claiming it
covered from the number of failures. Cover fresh LINE+INSTRUCTION, adding
either layer to the other, and second-tool transitions for all three mask
arrays. Both the new LINE header/int-array block and its original-token
block must be witnessed.
After each MemoryError require preserved local request/tool version, active
masks, code version, existing adaptive words and backing presence, allowing
only a valid independent empty header. Retry the identical requested mask
without failure and execute/restore successfully. The first predicted old
failure is changed local masks despite MemoryError; a later repeated-mask
call can then return success without the requested active instrumentation.
Global-mask failure tests record the separate partial-progress semantics.

## 6. Ownership, review and bounded implementation

Owned after approval: pycore_instruments.h; instrumentation.c line accessors,
canonical relations and prepare/commit paths; codeobject.c monitoring
teardown and the exact public Addr2Line assertion/call update only;
legacy_tracing.c's two sys_trace_jump_func bounds/call updates only;
bytecodes.c LINE/INSTRUCTION accesses; ir_generator.py domain
relations and ir_metadata_generator.py; their generated metadata/cases;
test_monitoring.py/test_ir.py and affected test_sys_settrace regressions;
HandlerDomain/generated-metadata classes in
test_generated_cases.py only. The three exact CAPI blocks above are pending
shared grants. Existing scalar, compiler, GC/provenance hooks and ROOTS
checker/import contracts are outside this ownership. No SafeAddr2Line,
H1/frame-passing, collector, stats, sampler or final allocator edits.

Independent review of the fixed format precedes implementation/grants.
After independent IMPLEMENT and grants, estimate 2-3 hours for coherent
storage/relations/publication, with the first failing-control checkpoint
within 45 minutes. Stop at a reviewable boundary if initialization/failure
semantics need a separate prerequisite. Rebuild/smoke release and qualified
Og, run affected modules and strict actual-kill with worker.main/origin/root
receipts, preserve phase/checker tests, prove frozen IDs/A-D/formats, and run
regen+clinic/optional-extension closure. Use copied isolated controls only;
keep source frozen during gates. Full stage-2/merged ladders are coordinator
work. No storage implementation gate has run; only the eight prerequisite
probe children recorded above executed.

## 7. Preflight response

| Finding | Revised disposition |
|---|---|
| Missed legacy GetLine callers | Five-call inventory from the full tracked-source search is retained in get-line-callers.txt; both legacy endpoint bounds and active tracing behavior are in ownership/controls. |
| METH_O ABI mismatch in two proposed hooks | All three hooks now use PyObject-returning METH_VARARGS with explicit tuple parsers; no mismatched cast. |
| Live header pointers passed into allocating result construction | Copy every mask/version byte, code version and presence flag onto the C stack before constructing any Python result; the builder sees only copies. |
| Blanket immutable-line/nonnull assumption | All slots are initialized; real heads store their immutable line, but relaxed trailing words preserve cached -1/token0. Only current eligible LINE callbacks require nonnegative lines. |

These are documentation/unapplied-patch revisions. The frozen dc02 source,
2a evidence, old 33cef design and separate H1 design remain unchanged.
The original unsafe/unexecuted fixture proposal is preserved verbatim as
storage-fixture-proposal.rejected-v1.patch (SHA256 1bf0f9cd66e62b54...);
it is review history, not the active proposal or an implementation input.
