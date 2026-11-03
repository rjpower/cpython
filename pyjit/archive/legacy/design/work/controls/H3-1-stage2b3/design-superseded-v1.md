# H3-1 stage 2b.3: handler-entry statistics

Status: design only against frozen storage candidate
`ffc548603b823484fdf106b1a9de06785017c682`, native `76274ed06ec`.
No source, tests, build or configuration changed for this inventory; no
native probe, benchmark or test was run. The storage landing branch stays
immutable. Implementation waits for the coordinator's integration/source
handoff after the ROOTS outcome. Primary has queued-runtime changes absent
from this branch; no joint Q/ROOTS/H3 qualification is claimed.

This refines section 4 of H3-1-stage2b.md. Sampler capture identity/transport
remains stage 3, and final numbering/threshold removal remains stage 4.
The change adds no operation or opcode-count cutoff.

## Counted events and their identities

Count each existing generated `INSTRUCTION_STATS` entry under the actual
target's generated handler name. Do not turn it into a counter of every
logical operation, instruction word or body execution. Today generate_handler
emits the counter before IR_PREDICTED, and jumps to that predicted label
bypass it. Keep this placement for both ordinary and tailcall emission.
Specializer redispatch and monitoring redispatch that enter a target still
count separately. A future change to that lifecycle needs a separate proof
and measurement, rather than silently changing this widening checkpoint.

Pairs connect consecutive *counted entries* in the existing eval-loop
lastopcode sequence. The int starts at 0 in ceval.c:1230 and passes through
tailcall signatures as int. Preserve the initial CACHE-0 source sentinel,
frame/call lifetime, and updates while statistics are disabled. Do not claim
that every pair is an edge between adjacent executed words. Covering the
previously omitted IR_JUMP entry changes which counted entries become
neighbors; record that coverage change explicitly.

Three identities remain separate:

| Counter/data | Identity |
|---|---|
| execution_count and pair_count | Actual generated TARGET name, including per-NB, NOFB and IR_JUMP |
| Existing specialization attempts/hits/misses/failures | Existing semantic counter sites and family association |
| Existing boxes and object totals | Existing semantic instruction/box-purpose sites |

In particular, changing a shared body's semantic `opcode` assignment would
break the reviewed CALL monitoring behavior. Change only the target-entry
counter argument from inst.name to name. Keep semantic opcode tests,
BOX_STAT_INC emission and specialization counter arguments unchanged.
Family reports may aggregate raw entry counts through the generated base
map; never overwrite the raw rows or treat base inequality as specialization.

## Current sinks, allocation and supported ownership

| Source at ffc5486 | Existing constraint / disposition |
|---|---|
| Include/cpython/pystats.h:64,146 | pair_count[256] and opcode_stats[256]; size both by generated IR_NUM_HANDLERS |
| Python/ceval_macros.h:70–82 | Skips op >=256; replace with domain admission/assertions and unconditional supported-handler counting |
| Tools/cases_generator/ir_generator.py:1805 | Uses inst.name; emit actual target name before existing predicted label |
| Python/pystats.c:141–186 | Both print axes stop at 256; full-domain names and generated properties replace bounds/cache heuristics |
| Python/pystats.c:103–134 | Existing Python specialization dictionary lists semantic family counters; preserve its API/counter sites |
| Include/internal/pycore_stats.h:27–43; Python/specialize.c:39 | Named counter indexing; audit all arguments against the domain without changing semantic ownership |
| Python/pystats.c:315–321,489–499 | Lazy and startup PyMem_RawCalloc(1,sizeof(PyStats)); one allocation per interpreter |
| Python/pystats.c:341–380,505–531 | Clear uses sizeof(PyStats); thread init/fini and merge are no-ops here; attach points threads at the interpreter aggregate |
| Python/pystate.c:464–466 | Frees that interpreter-owned allocation; no per-thread matrix to retire |
| Modules/_testinternalcapi/typecache.c:232 | Reads unrelated object stats through the same aggregate; unchanged offsets require rebuild, no semantic edit |
| Tools/scripts/summarize_stats.py:58–79,177–347 | Loads source metadata, sums entry rows and classifies specialization; raw/family views must remain distinct |
| boil/bench/run.py:37,392,398; boil/bench/README.md:82 | Current dispatches label overstates the counted-entry contract; coordinator-owned compatibility change below |

Include/cpython/pystats.h can include the generated public opcode_ids.h for
IR_NUM_HANDLERS: that header includes no Python or internal generator header,
so there is no import/include cycle or duplicate count authority. Python.h
already includes pystats.h at line 74. The generated count stays at 302.
No 65,536-row capacity reservation or 65,536² fixture is proposed.

At the current ABI's source-level widths, SpecializationStats has 66 uint64
fields (528 bytes), and the other non-pair OpcodeStats fields add 16 bytes.
The pair matrix grows from 256² × 8 to 302² × 8: +205,344 bytes. The 46
additional rows each have 544 non-pair bytes: +25,024. Expected PyStats request
growth is therefore 230,368 bytes **per interpreter that enables stats**, not per
thread. These are source arithmetic, not measured sizeof, allocator overhead
or RSS. A matching-build fixture must report actual sizeof(OpcodeStats),
sizeof(PyStats), count and successful request sizes before any memory claim.
Keep the single dense matrix for the actual count; this stage needs no new
allocator, sparse representation or runtime policy.

All stats mutations remain under this fork's supported owning-GIL contract.
Do not invent a free-threaded merge or locking protocol from the existing
empty wrappers. Existing aggregate enable/disable/attach/clear semantics
must keep passing, including threads and interpreter teardown.

## Generated adaptive property and report contract

Add one orthogonal generated ADAPTIVE property: a dispatch target has its
own feedback and executes an actual specializing uop from the analyzed DSL.
Derive it from the real specializing annotation and resolved target format;
exclude NOFB targets, specialized members and instrumentation wrappers.
Generic per-NB targets can qualify. IR_JUMP, ordinary branch counters,
metadata-only family tokens and feedback-free forms do not qualify merely
because they have a cache, base alias or numeric position.

The existing uint16 property table has room for the proposed next bit, 512.
The value is a flag, not an opcode category boundary. Validate flags against
the generated domain and export the same classification to Python metadata.
The semantic families that can specialize are the generated base images of
ADAPTIVE targets, including BINARY_OP despite its metadata-only family token.
Remove print_spec_stats' fake future-specializable BINARY_SLICE/STORE_SLICE/
GET_ITER declarations and its cache-width/jump-name heuristic. There is no
compatibility approximation for that classification.

The summarizer exposes two explicit views. Raw entry/pair tables keep
actual handler names. Family specialization sections aggregate only entry
counts through the matching generated base map and then combine them with
the existing semantic family counters. Raw specialized-handler classification
uses the generated SPECIALIZED property; a generic per-NB alias or NOFB form
is not specialized just because its base differs. Do not sum a family view
back into raw totals, duplicate rows, or change box attribution.

Count coverage has changed. Emit `Handler entry coverage version : 2` in raw
dumps and `_handler_entry_coverage_version: 2` in exported summary JSON.
The summarizer requires that version before reading or comparing new cohorts;
it does not reinterpret older retained results as expanded raw entry totals.

The producer also emits actual generated metadata facts for every named
metadata entry: `opcode[NAME].handler_id`, `.base_id` and `.handler_flags`,
using integer values from its compiled tables. Store these in exported JSON
as `_handler_metadata`, separate from execution/specialization counters.
Validate unique IDs/names, bounded base IDs, base idempotence and referenced
metadata before aggregating. Both compared cohorts must carry consistent
facts for the rows being compared. Use these producer facts for handler
classification and family grouping, rather than importing the reporting
process's opcode table and assuming it matches the sampled executable.
Exclude metadata keys from specialization-count tables. Existing failure-kind
text decoding remains source-dependent and outside this handler-width slice;
do not claim this repairs that older attribution limitation.

This is the stats report's coverage/classification contract. It does not
implement the remote sampler binary schema or replace stage 3 target identity
validation. Old artifacts keep their old source/tool/coverage identities and
remain readable with their corresponding historical tool.

The coordinator owns boil/bench/run.py and README. Proposed coordinated delta:
replace the result key dispatches with counted_handler_entries and document
its target-entry/predicted-label contract; require the new coverage marker
and carry it into result metadata. Source search finds no other Python reader
of dispatches in boil/bench. Preserve boxes, by_instruction, allocations and
their definitions; do not extend the benchmark set or add memory/pause claims.
Update affected callers/docs together, with no old-key fallback. Existing
results retain their old source/coverage identity and are not rewritten.

## Exact proposed scope and first falsifiable controls

Proposed owned source files are Include/cpython/pystats.h,
Include/internal/pycore_stats.h and Python/ceval_macros.h if their indexing
contract needs assertions; Python/pystats.c; the existing HandlerDomain,
IR-case and C/Python metadata generators and their regenerated products;
Tools/scripts/summarize_stats.py; Lib/test/test_pystats.py; and separate
HandlerDomain tests in Lib/test/test_generated_cases.py. Reader tests can
use test_tools.import_tool in a new test_summarize_stats module.
No ceval frame-passing, GC, root-checker/call_effects, monitoring storage or
specializer source edit is proposed. Existing phase/root/NOFB oracles remain.
The shared test_generated_cases hunks and coordinator-owned benchmark reader
require explicit ownership handoff before implementation. No new shared CAPI
block is needed: use a task-owned isolated extension for primitive counters.

The first implementation checkpoint is a bounded, matching Py_STATS-build
control. A task-owned extension consumes the actual header/macro, allocates a
private PyStats before interception, saves the current thread's stats pointer,
and executes a fixed entry sequence such as low -> IR_JUMP -> high-NOFB ->
low. No Python call/allocation occurs while the private pointer is installed;
restore it on every path before producing Python output or freeing storage.
Copy only primitive selected counts and actual sizes for output. Exercise both
pair axes and the initial 0 source. Never execute a synthetic high handler or
construct malformed code to test counter storage.

Required independent controls and positives:

- Restoring the old 256 skip in a copied macro must fail named high execution
  and high-source/destination pair assertions, not compilation or allocation.
- Returning the emitter to inst.name must fail generated output for actual
  per-NB/NOFB entries. A live matching-build workload must also produce the
  exact high raw rows after witnessing its executing IDs through get_ir.
  Reuse legitimate forced-relaxation FOR_ITER/SEND and valid NOFB paths.
- Production names/IDs are unchanged: 264 opcode/pseudo entries, 287 dispatch
  handlers, 302 slots, first IR-only 267. Domain/name/flag tests distinguish
  adaptive generic forms, specialized members, NOFB and IR_JUMP. An omitted
  adaptive flag must fail a classification assertion, not just a source scan.
- Reader fixtures check raw totals, family aggregation and specialization
  partitions without double counting; mismatched coverage/producer metadata
  comparisons fail explicitly. Box/object allocation counters stay unchanged.
- Actual sys._stats_on/off/clear/dump and thread/exit tests run on a matching
  stats build, including high rows after clear and preserved disabled-phase
  pair lifecycle. Do not claim the existing merge-named test proves a
  per-thread merge implementation that this fork does not contain.

The private-counter fixture and live workload prove different things: the
former isolates full-domain storage/pair behavior; the latter establishes
actual generator/runtime entry coverage. Preserve both controls and precise
limits. Capture source/header/config/compiler/binary identities and actual
verifier/stress/optimization state; copy variants only, with no live mutation.

## Execution boundary

There is an existing build/pystats with Py_STATS enabled and GCC configuration;
it is historical and has not been rebuilt or executed for this design. Reuse
it only after the settled integration handoff and rebuild against the exact
candidate. Its previous artifacts remain bound to their original sources.

After design review and ownership grants, estimate a 45-minute first counter
and emitter checkpoint, then 2–3 hours for coherent writer/reader/tests and
recording, plus the matching stats rebuild and qualification time. Gate scope:
release and Og smoke/affected generator/IR/opcode/compiler/monitoring modules,
matching Py_STATS functional/counter/reader checks, actual-worker kill checks,
regen and seven-product/ID comparison. Use the coordinator's normal admission,
locks, fresh caches and timeouts; source remains frozen for each transaction.
Any performance measurement is separate, serial and explicitly authorized.

Stop after a reviewable statistics checkpoint. Inspection/compiler consumer
closure remains 2b.4; sampler transport remains stage 3; final renumbering/magic
and threshold API removal remain stage 4. H1 still needs its separate integrated
generator/build ownership handoff, and H2 still needs the precise-root work.
