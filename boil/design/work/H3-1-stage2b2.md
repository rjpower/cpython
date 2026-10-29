# H3-1 stage 2b.2: monitoring storage and lifetime

Status: design only, against frozen `c5cf6ba6d2888ebbad085e990cf47e156a079780`.
No implementation, new tests, builds or probes have run for this document.
The stage 2b.1 sources, work record, evidence and earlier stage 2b design
remain unchanged. This checkpoint keeps every handler ID and word layout.

## Decision and bounded order

First make `_PyCode_SafeAddr2Line` read the immutable code line table only.
Its current mutable monitoring-cache branch creates a dependency that an
asynchronous reader does not need. Prove the ordinary frame-position
equivalence and record the synthetic-word distinction below before this
small prerequisite lands. Do not introduce a signal-reader reclamation
scheme or claim that its existing freed-frame heuristics protect lifetime.

Then widen displaced monitoring tokens to uint16, generate explicit twin
relations, and stage per-code allocations before publication and handler
installation. Existing ordinary monitoring readers and writers run under
the owning interpreter's GIL. The arrays survive callbacks, disable and
restart; only code destruction retires them. There is no new word atomic,
FT wrapper, lock, fallback representation or runtime option in this plan.

Two reviewable implementation checkpoints are proposed:

1. **2b.2a, immutable asynchronous line lookup:** codeobject.c's safe lookup
   and its tests, with a separately approved small CAPI lookup hook.
2. **2b.2b, monitoring representation and relations:** header, generator,
   instrumentation, bytecode accessors and coherent allocation/cleanup
   changes together. This must not land with a half-widened reader/writer.

If the first step cannot preserve the stated supported lookup behavior,
stop that prerequisite for coordinator disposition. Do not hide an unresolved
reader-lifetime problem inside the storage patch. Stages 2b.3/4, sampler
schema/codec work and final renumbering remain separate.

## Concrete readers and owners

| Path at c5cf | What it reads and what protects it |
|---|---|
| instrumentation.c:704-853, 1353-1508; bytecodes.c:5507-5548 | Mutable displaced LINE/INSTRUCTION tokens, tool masks and executing words. Attached monitoring/dispatch paths use the owning GIL; callbacks can reenter only after a complete state has been installed. |
| codeobject.c:1028-1044, `PyCode_Addr2Line` | Acquire-loads monitoring and LINE pointers and reads immutable line deltas. Ordinary C API calls require a live code object and the attached interpreter contract. Keep its current publication protocol. |
| frame.c:197-200; traceback.c:157; lazyimportobject.c:310; _testcapimodule.c:2352-2376 | In-tree public Addr2Line callers: live interpreter frame, traceback, lazy import or checked C API argument. None is a displaced-token reader. |
| traceback.c:1031-1061 | Signal-safe dump_frame obtains a *borrowed* code pointer using SafeGetCode, obtains lasti, then calls SafeAddr2Line. The helper is explicitly heuristic, not a lifetime pin. |
| traceback.c:1107-1156, 1260-1335; faulthandler.c:188-217, 674-706 | Same-thread fatal/signal dumps and other-thread timeout/all-thread dumps. A target thread can pop/free frames while another thread dumps it. The source explicitly acknowledges this. |
| tracemalloc.c:316-327, 390-441, 602-650 | Allocation traceback capture uses its own current or most recently bound thread's frame chain, including while detached, and calls SafeAddr2Line. TABLES_LOCK protects tracemalloc tables, not monitoring storage or code lifetime. |

The apparent locking names are not proof. instrumentation.c:27-33 defines
LOCK_CODE and ASSERT_WORLD_STOPPED* as empty; pystate.c:2216-2233 also has
empty StopTheWorld/StartTheWorld bodies. FT loads/stores are plain accesses
in pycore_pyatomic_ft_wrappers.h:22-38. The concrete owner here is the GIL,
not those names. ceval_gil.c:450-486 supports shared and interpreter-owned
GILs: do not describe all interpreters as one global serialization domain.
Normal cross-interpreter code transfer marshals and reconstructs a new code
object (crossinterp_data_lookup.h:693-711, crossinterp.c:852-890); this plan
does not support concurrent mutation of one code object from independent
own-GIL interpreters through an extension's shared raw pointer.

The new immutable safe lookup removes every asynchronous read of the
monitoring header/arrays. A code object can still be invalid in an emergency
dump: pycore_interpframe.h:107-165 and pycore_code.h:185-189 explicitly retain
that best-effort limitation. The change does not make an already dead code
pointer safe. Nor does an atomic monitoring pointer provide reclamation.
The timeout/signal path therefore needs no *new monitoring-array* lifetime
scheme for this layout change; broader dump safety is an independent issue.

For detached tracemalloc capture, distinguish three facts. Its own thread
does not push/pop its Python frames while the allocator walks them
(tracemalloc.c:413-416). The frame holds `f_executable` and its code holds
`co_linetable`: frame initialization sets the executable (interpframe.h:276),
frame traversal visits it (frame.c:17), and the collector visits active/data
stack frames of thread states (gc_trace.c:1400-1429, 2249-2285), including
detached states. These are the root-lifetime paths for ROOTS to confirm
against its pending landing, not a new claim about arbitrary C readers.
The InterpreterGuard at tracemalloc.c:421-441 prevents finalization only:
pystate.c:3030-3101 increments `finalization_guards`; it does not pin an
individual code object or suspend GC. No guard-name shortcut is permitted.

## Immutable safe lookup and its exact equivalence boundary

SafeAddr2Line already has an allocation-free fallback through
`_PyCode_InitAddressRange` and `_PyCode_CheckLineNumber`
(codeobject.c:1055-1060, 1075-1100). It reads code-owned immutable bytes and
local stack state, without locks, Python calls or new allocations. The
proposed step removes only the earlier monitoring-cache branch; negative
offsets still return firstlineno, and out-of-range offsets still return -1.

For a real instruction head, initialize_lines obtains the same line from
CheckLineNumber, encodes `line - firstlineno` or NO_LINE, and GetLine decodes
it (instrumentation.c:360-430, 1571-1615). Later branch/exception scans change
only original tokens, not deltas. Consequently enabling/disabling monitors
does not change the authoritative source location at those heads.

**Do not claim equality at every word.** assemble_location_info counts all
`op->nwords` under the op's location (assemble.c:327-341), while
initialize_lines deliberately writes NO_LINE for the extra relaxed IR_JUMP
words (instrumentation.c:1616-1620). The immutable table can therefore give
those words the loop's line while the monitoring cache gives -1. IR_JUMP is
`no_save_ip` by design (bytecodes.c:5672-5678; generated_ir_cases.c.h:12250),
so ordinary executing-frame lasti remains at the loop head. Safe callers
obtain lasti from SafeGetLasti. No broad CFG validation is being added to
force this property on hand-constructed code objects.

The proposed explicit behavior is that direct safe lookup of any valid
offset uses the immutable table, including a synthetic jump offset. The
cache remains an event-eligibility optimization for ordinary monitoring;
the existing public Addr2Line cache path is otherwise unchanged. Tests must
separate ordinary frame-position equivalence from this synthetic-offset
choice. That choice requires review; it is not silently labeled equivalent.

First controls for 2b.2a: compare lookup against `co_lines()` at every real
instruction head before/during/after LINE+INSTRUCTION, including exception
handlers, no-line entries, backward locations and forced relaxed FOR_ITER
and SEND. Use the existing `ir_max_forward` fixture. A copied old safe
function, against the same real monitored relaxed code and a synthetic
jump offset, must fail the authoritative-line assertion; no fake pointer
or malformed code execution is needed. Source closure separately asserts
that the safe function has no monitoring-field dependency. Existing
faulthandler/tracemalloc regressions exercise their callers; they do not
prove all emergency cross-thread frame reclamation safe.

## Exact uint16 representation

Use one LINE header/delta allocation and an independently aligned uint16
token allocation, keeping the current trailing-array coding convention:

```c
typedef struct {
    uint16_t *original_opcodes;  /* nwords entries, mutable under owner GIL */
    uint8_t delta_bytes;        /* 1..4, immutable after initialization */
    uint8_t deltas[1];          /* delta-only bytes, immutable after initialization */
} _PyCoLineInstrumentationData;
```

Allocate `sizeof(header) + nwords * delta_bytes` bytes for header/deltas
(the spare trailing byte/padding is deliberate), and
`nwords * sizeof(uint16_t)` for originals. INSTRUCTION becomes
`uint16_t *per_instruction_opcodes`, allocated by its own element size.
This removes the old wrong allocation by sizeof(LINE header)
(instrumentation.c:1800). Check both multiplication and header addition
before allocation, also respecting PyMem's PY_SSIZE_T_MAX limit. Use widened
arithmetic for adjusted delta/width calculations; do not preserve the
current `firstlineno + 1` overflow opportunity as a sizing trick.

The codec still stores `delta - NO_LINE`, with NO_LINE=-2 encoded as zero;
delta bytes start at index `i * delta_bytes`, with no original-ID byte
mixed into them. Read/write the originals through their naturally aligned
uint16 array. Never cast the old packed byte address to uint16*. Exercise
all four delta widths, odd/even token indices, adjacent records and extreme
valid line deltas. The original-opcode getter returns uint16; callback and
API scalars stay int until validated. Zero remains the ineligible-LINE
sentinel; CACHE constructor padding and its execution trap are unchanged.

Keep all tool arrays, tool memberships, delta-width descriptors and event
table values at their current narrow widths. Neither a tool bitset nor a
line delta becomes a handler ID merely because it is nearby in a struct.
code_sizeof currently omits monitoring data entirely (codeobject.c:2691);
do not claim it measures these arrays. No sizeof-policy expansion is part
of this checkpoint. Memory evidence must count the allocations themselves.

## Canonical tokens and explicit monitoring relations

Displaced storage holds a canonical opcode/event token, which may be an
instrumented canonical token or LINE/INSTRUCTION wrapper. It is not always
the raw dispatch ID. word_opcode already normalizes through the generated
base map (instrumentation.c:62-64). BINARY_OP has metadata but no dispatch
body; requiring DISPATCH on every displaced token would reject valid state.
Restoring a word still passes that token through HandlerFor's pristine-base
match and NOFB/layout selection to recover the exact raw handler/A-D word.

Add immutable domain relations for ordinary instrumented/base pairs using
the existing explicit INSTRUMENTED_ shared-format association
(ir_generator.py:231-233, 459-467). Validate present paired declarations,
kind, metadata and matching formats. A minimal DSL without an unrelated
counterpart remains valid; do not add dummy declarations. The two transparent
wrappers have explicit self-install/no-deinstrument entries and state
requirements; they do not acquire an invented ordinary base. Any actual
exception to the shared-format association must be named and validated.

Emit domain-sized uint16 canonical install/deinstrument tables in the IR
metadata product. Ordinary install maps base to twin, a twin to itself;
deinstrument maps twin to base and otherwise returns zero. Normalize raw
NOFB/specialized aliases to their canonical token before these lookups.
Retain the explicit C event association table, sized by IR_NUM_HANDLERS,
with int8 values and the existing RESUME-specific event selection. Replace
is_instrumented's numeric cutoff with generated kind; no semantic cutoff
or numeric fallback remains in this monitoring consumer. Stage 4 still owns
the inherited exported threshold removals.

Widen exported-internal `_PyCode_Deinstrument` to `int (int)`: validate the
original scalar, canonicalize, then return the canonical base, zero for a
valid token without a counterpart, or -1 for an invalid scalar. Its only
in-tree occurrences are declaration/definition; external JIT users rebuild
under the project's internal-ABI policy. Propose that signature break in
PROCESS's compatibility ledger through the coordinator, who owns that file.
No narrowing conversion may precede the validity check. Likewise validate
each displaced-token store as metadata/canonical state before indexing or
casting. Only the final installed handler requires DISPATCH; consume every
checked-helper result. Do not execute a wrong-layout negative-control word.

## Initialization, publication, reentry and retirement

1. Keep a pending per-code plan on the C stack: proposed local/version
   cleanup, required new pointers and active-state calculations. Allocate
   and fully initialize absent arrays privately; initializers receive the
   explicit pending pointers instead of reaching through shared fields.
2. On failure, free every unpublished pending block and preserve the prior
   valid executing-word/backing-array/active-version state. Do not set
   active_monitors or the code instrumentation version before preparation
   succeeds. A completed empty monitoring header or executing-copy cache
   may remain if it is
   already a valid independent allocation. Existing caller-requested/global
   masks may precede per-code refresh; do not promise an unimplemented
   transaction across all code objects in set_events. Its failure/retry
   semantics must be recorded and tested explicitly.
3. Once all required allocations succeed, publish a fully initialized
   monitoring header if absent, then all backing arrays and candidate active
   masks, then the wrapper words, and finally the code version. Compute the
   removed/new masks from the old-active snapshot before that commit;
   existing remove_tools/remove_line_tools single-tool branches consult
   active_monitors (instrumentation.c:871-875, 906-910), so simply moving
   that assignment after the rewrite loop would change their assumptions.
   No recoverable allocation/Python call may occur inside this commit.
   Keep the existing real release stores for `_co_monitoring` and `lines`
   and ordinary Addr2Line's matching
   acquire reads. This does not authorize a new lock-free multiwriter path.
4. Displaced-token mutations are ordinary aligned uint16 stores under the
   owning GIL. Replace the old nominal FT uint8 store at instrumentation.c:729
   with the validated uint16 store; do not add atomics for naming symmetry.
   Word rewrites still use the checked full word helper and preserve the
   upper 48 bits. Other dispatch/specialization writers share the owner;
   the out-of-process sampler's snapshot/schema contract remains stage 3.
5. Do not allocate or call Python in the commit interval. The configured
   mem allocator does not collect (obmalloc.c:1220-1236, 1303-1316);
   default/debug, tracemalloc and failmalloc hooks do not execute Python.
   Tracemalloc may reenter line lookup, which now reads immutable data.
   `_PyGC_LiveRunQueue` happens before the API mutation (instrumentation.c:
   2379, 2468). Monitoring callbacks happen after complete state and may
   collect, DISABLE, restart, change local events or raise.
6. Published arrays are never freed/replaced by those actions. Local pointers
   held across callbacks remain owned by the live code/frame. Teardown adds
   `PyMem_Free(lines->original_opcodes)` before freeing the LINE block in
   free_monitoring_data (codeobject.c:2312-2334); INSTRUCTION and the other
   buffers retain their existing code-owned teardown. No hidden leak or
   retire-later list substitutes for this lifetime argument.

The primitive builder uses only scalar/byte arrays and does not add Python
object fields or new collector traversal obligations. Existing collecting
callback roots, exception paths and active-frame anchors remain mandatory;
no function becomes Py_GC_PRECISE in this patch. If implementation discovers
a supported reentrant allocator or GIL-free token reader, report a bounded
new prerequisite with its actual call path before changing this protocol.

### Preserve existing site membership when adding arrays

New multi-tool mask arrays must describe the *old installed state* before
removed/new masks are applied. They must not resurrect a site's DISABLE or
erase the existing tool simply because representation changed. Decode the
old executing word and existing LINE/INSTRUCTION layers, using the snapshot
of old active masks and old published backing arrays. Do not infer enabled
LINE state merely from its nonzero eligibility token.

For a new line_tools array, a site with an installed LINE wrapper gets the
old active LINE mask; an eligible site whose LINE was disabled gets zero.
For a new per_instruction_tools array, a site with INSTRUCTION installed
directly or displaced beneath LINE gets the old INSTRUCTION mask; a disabled
site gets zero. Apply removed/new masks once during commit. Ordinary event
tools initialization likewise uses actual old instrumented state, as the
current initialize_tools already attempts (instrumentation.c:1515-1570).

This matters beyond pointer publication: current initialize_line_tools
copies all_events at every site, and current per_instruction_tools starts
at zero (instrumentation.c:1690-1695, 1814-1821). With only new_events added
later, these are source-level risks of re-enabling an old disabled LINE tool
or losing an existing INSTRUCTION tool when the second tool appears. No
runtime reproduction is claimed here. Add exact old-behavior oracles for
both transitions and retain any confirmed failure before implementing their
shared initialization correction. This stays within the representation
transition; it does not change DISABLE's allowed spurious-event contract.

## Ownership, controls and gates

| Owned file/functions proposed for implementation | Boundary |
|---|---|
| Objects/codeobject.c: SafeAddr2Line; free_monitoring_data | Immutable lookup prerequisite and nested token-array cleanup only; no collector/type traversal or sizeof change |
| Include/internal/pycore_instruments.h | LINE layout/getter, INSTRUCTION type, Deinstrument signature |
| Python/instrumentation.c | Delta codec; displaced pointers/stores; initialization/allocation; canonical relations/event indexing; LINE callback result; debug sanity paths |
| Tools/cases_generator/ir_generator.py; ir_metadata_generator.py | Domain relations and generated uint16 tables, preserving all IDs/formats |
| Include/internal/pycore_ir_metadata.h; Python/generated_ir_cases.c.h | Generated products only |
| Python/bytecodes.c: LINE/INSTRUCTION handlers | Replace direct packed-byte read with getter; replace `<256` with validated canonical/final-dispatch checks |
| Lib/test/test_generated_cases.py | Domain/generated-metadata classes only; ROOTS checker class/imports unchanged |
| Lib/test/test_monitoring.py; test_ir.py; test_code.py | Nested/reentrant monitoring, precise restore, primitive width and safe-line tests |

**Shared grant still required:** an additive `pycore_instruments.h` include,
a primitive-only monitoring-storage roundtrip helper, a SafeAddr2Line lookup
helper and their method entries in _testinternalcapi.c. Send exact proposed
hunks to ROOTS/coordinator before editing. The old selector/scalar grants do
not authorize them. No GC/provenance/compiler fixtures are touched. Use the
existing `_testcapi.call_with_nomemory` (mem.c:214-245), which arms failure
immediately around the C call and removes hooks before returning to Python;
do not introduce a global allocation hook.

First representation witness: token 256 round-trips as zero in the old
byte representation. Also test 255, 511 and 65535 at odd/even indices and
every delta width, preserving adjacent IDs/deltas. These are primitive
representation values, not instructions to execute. A copied narrowed
production getter/store must fail the exact round-trip assertion. Test
original int boundaries -1, 65536 and 65536+INSTRUMENTED_CALL at the actual
Deinstrument C boundary; a copied cast-before-check model must accept a
bad alias and fail the rejection assertion without indexing out of range.

Use real existing high raw per-NB/NOFB words for LINE+INSTRUCTION nesting,
both removal orders, DISABLE, restart, legacy tracing recursion and collecting
callbacks. Require exact pristine IDs and A-D restoration, and preserve the
2b.1 collecting CALL/C_RETURN/C_RAISE oracle. Those displaced canonical
tokens are currently low: do not misreport this as runtime execution of a
high canonical token. Final renumbering must supply that later witness.
Copied relation controls choose another valid canonical twin and compare
the resulting IDs; never dispatch their wrong layouts.

Fail each newly introduced allocation and each path enabling multiple tools,
with the target executing copy pre-created. Include fresh instrumentation,
adding INSTRUCTION to LINE, adding LINE to INSTRUCTION, and a second tool
sharing an event so tools/line_tools/per_instruction_tools are allocated.
Include a disabled site and a still-enabled site for the first tool before
adding the second same-event tool, checking both retained and suppressed
memberships independently.
After MemoryError, inspect the prior active words/backing state, remove the
failure hook, retry and execute successfully. Record requested-mask behavior
separately from active-state validity. Source assertions about initialization
order supplement this; the old early pointer assignment is not by itself
claimed to be an observed concurrent-reader race under the GIL.

Each implementation checkpoint rebuilds/smokes release and qualified Og
debug, runs affected modules in both and strict actual-mode debug kill with
worker.main and root counters. Add test_sys_settrace, test_sys_setprofile,
test_frame, test_traceback, test_faulthandler and test_tracemalloc to the
monitoring/code/IR/compiler/generated suite. Preserve compiler phase and
ROOTS checker fixtures. Require frozen ID comparison, all-domain relation
checks, regen+clinic closure and relevant optional-extension syntax checks.
Full stage-2 and merged-integration ladders remain coordinator gates.

Estimate after reviewed design and shared-hunk grants: 45-75 minutes for
2b.2a, then 2-3 hours for coherent storage/relations with focused evidence.
Checkpoint after 45 minutes of each implementation; no new native work
begins while c5cf's independent review is running. No performance claim or
unrun validation result is implied by this design.
