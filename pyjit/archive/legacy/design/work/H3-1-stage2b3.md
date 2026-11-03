# H3-1 stage 2b.3: handler-entry statistics

Status: revised design only against frozen storage candidate
`ffc548603b823484fdf106b1a9de06785017c682`, native `76274ed06ec`.
No source, tests, build or configuration changed for this inventory; no
native probe, benchmark or test was run. The storage landing branch stays
immutable. Implementation waits for the coordinator's integration/source
handoff after the ROOTS outcome. Primary has queued-runtime changes absent
from this branch; no joint Q/ROOTS/H3 qualification is claimed.

This refines section 4 of H3-1-stage2b.md. Sampler capture identity/transport
remains stage 3, and final numbering/threshold removal remains stage 4.
The change adds no operation or opcode-count cutoff.
The first draft, SHA256 `141e85f1f4a407cc3183380c9c90c7080dab1b49f860c9c25cf4af46d3318f14`,
is preserved verbatim at
[design-superseded-v1.md](controls/H3-1-stage2b3/design-superseded-v1.md).
The response to independent REVISE review `26a2ed1360e4...` is below.

## Counted events and their identities

Count each existing generated `INSTRUCTION_STATS` entry under the actual
target's generated handler name. Do not turn it into a counter of every
logical operation, instruction word or body execution. Today generate_handler
emits the counter before IR_PREDICTED. In the normal interpreter a goto to
that label bypasses the counter. Tailcall JUMP_TO_PREDICTED instead calls
the target function at its entry (ceval_macros.h:131–137), so that entry
counts again. Preserve both modes' existing placement and behavior. Record
the mode in producer metadata and reject cross-mode entry comparisons.
Specializer and monitoring redispatch that enter a target count separately.
A change to this lifecycle needs its own proof and measurement.

Pairs connect consecutive *counted entries* in the existing eval-loop
lastopcode sequence. The int starts at 0 in ceval.c:1230 and passes through
tailcall signatures as int. Preserve the initial CACHE-0 source sentinel,
frame/call lifetime, and updates while statistics are disabled. An inlined
frame switch within the same eval activation keeps the sequence; a distinct
recursive eval activation starts its own sequence at 0. Stats clear zeroes
the aggregate, not the activation's lastopcode. Do not claim
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
Remove print_spec_stats' manually listed BINARY_SLICE/STORE_SLICE/GET_ITER
declarations and cache-width/jump-name heuristic. Placeholder slice uops do
not qualify without feedback use; GET_ITER now has a real feedback-driven
specializer and qualifies through that fact. Truth branches likewise have
actual specializing uops, unlike branch-history storage alone. There is no
compatibility approximation for this classification.

The summarizer exposes raw entry/pair tables, family entry totals and
separate semantic event tables. Their arithmetic and limits are specified
in the next section. No semantic counter is reassigned through the base map.

Count coverage has changed. Emit `Handler entry coverage version : 2` in raw
dumps and `_handler_entry_coverage_version: 2` in exported summary JSON.
Also emit `Handler entry dispatch mode : 0` for normal dispatch or `: 1`
for tailcall dispatch, and retain `_handler_entry_dispatch_mode` in JSON.
The summarizer requires that version before reading or comparing new cohorts;
it does not reinterpret older retained results as expanded raw entry totals.

The producer also emits `Handler domain count : 302` and
`Handler metadata entries : 299` at this baseline, retaining `_handler_count`
and `_handler_metadata_entries` in JSON. These values come from the compiled
domain and named-entry count, not new fixed numeric limits.
It emits actual generated metadata facts for every named
metadata entry: `opcode[NAME].handler_id`, `.base_id` and `.handler_flags`,
using integer values from its compiled tables. Store these in exported JSON
as `_handler_metadata`, separate from execution/specialization counters.
Validate each input file before any counter summation: exactly one supported
coverage/mode/count declaration, exactly one complete ID/base/flags triple
per name, unique IDs/names, declared metadata-entry count, IDs within the
declared domain, base idempotence and referenced bases. A pair destination
must be dispatchable; its source may also be the metadata-valid CACHE-0
initial sentinel. Execution rows require dispatchability; semantic counters
and boxes may name metadata-only families. Missing, duplicate or conflicting
facts are errors, including identical repeated declarations. Apply the same
validation to exported JSON, rejecting duplicate JSON keys before a dict
can erase them.

Separate that validated metadata object from additive counters. For a
directory cohort, require every file to carry exactly the same metadata,
mode and coverage; keep one copy and sum counters only. Never send IDs,
flags, count, version or mode through the current Counter integer summation.
Both cohorts in a comparison must carry the same classification/coverage
contract; otherwise reject the comparison. Use these producer facts for handler
classification and family grouping, rather than importing the reporting
process's opcode table and assuming it matches the sampled executable.
Exclude metadata keys from specialization-count tables. Existing failure-kind
text decoding remains source-dependent and outside this handler-width slice;
do not claim this repairs that older attribution limitation.

This is the stats report's coverage/classification contract. It does not
implement the remote sampler binary schema or replace stage 3 target identity
validation. Old artifacts keep their old source/tool/coverage identities and
remain readable with their corresponding historical tool.

## Reporting arithmetic and attribution limits

Let E[h] be the recorded execution_count for raw handler h, and b[h] its
producer-declared canonical base. Define total counted entries R = sum E[h],
and family counted entries F[g] = sum(E[h] for b[h] = g). These totals contain
only entry counters. Never add hit, miss or deferred counters to either one.
The former get_specialization_total formula, execution_count + hit + miss,
is removed rather than reused as the denominator of a widened family view.

Partition raw entries into A (ADAPTIVE target), S (SPECIALIZED target), and
O (every other dispatch target). Require ADAPTIVE and SPECIALIZED to be
disjoint. Then A + S + O = R. A/R, S/R and O/R are *counted-entry shares*,
shown only for R > 0. They are not hit rates or shares of logical operations.
NOFB targets are in O even when their base has adaptive targets. A guard
failure still entered a specialized target, so its entry stays in S.
Do not subtract any specialized miss counter or infer successful specialized
entries; this slice establishes no event-to-entry bijection.

Semantic counters remain M[name,event] under exactly their recorded names.
The separate "Recorded specialization events" section selects only
specialization.hit, .miss, .deferred and .deopt for counts, with no execution
ratio. Never iterate arbitrary remaining numeric fields: boxes, execution,
pair and metadata fields are excluded explicitly. Success/failure has its
own table and denominator T[name] = M[name,success] + M[name,failure]. Display
success/T and failure/T only for T > 0, labeled shares of recorded attempt
outcomes. Failure-kind diagnostics keep their recorded key and recorded
failure-event denominator, not an execution denominator. Event rows with
counts but no counted entries remain visible.

Do not remap semantic events through b[h], sum miss rows into a presumed
global miss total, or calculate hit/deferred execution shares. UPDATE_MISS_STATS
writes both a member and its family key for the same guard failure. Reused
truth uops write TO_BOOL hits even when executed by a branch specialization;
the branch specializer records success under its branch family. Its
OPCODE_DEFERRED_INC(TO_BOOL) is conditional on opcode == TO_BOOL, so it does
not provide a branch-deferred count. Neither the raw base map nor existing
aggregate data can reconstruct the missing attribution. No counter-site,
truth-uop or specializer rewrite is authorized to manufacture it here.

The report replaces "Specialization effectiveness" with "Counted entries
by target kind" using A/S/O. Deferred/miss top lists, if retained, show
counts by recorded key without global execution-share columns. Family entry
tables and recorded-event tables are separate; no merged row suggests that
their quantities partition family executions.
Update every affected reader use: remove TOTAL/get_specialization_total's
overlapping denominator and the raw execution table's Miss ratio column
(get_execution_counts and calc_execution_count_table). Its existing claim
that a deopt never counts the base is false for tailcall predicted entry.
The raw table shows counted entries and their self/cumulative shares only.
Pair table shares describe recorded entry pairs, including the initial
sentinel, rather than universal dynamic-word edges. Update section titles,
column labels and explanatory text together; retain no old-ratio fallback.

Required reader fixtures have these exact numeric expectations:

| Input facts | Required report |
|---|---|
| E[generic]=10, E[specialized]=90, same base; recorded family hit=90 | F=100 and R=100; A=10, S=90; no 190 denominator |
| E[POP_JUMP_IF_FALSE]=10, E[POP_JUMP_IF_FALSE_INT]=90; TO_BOOL.hit=90; no TO_BOOL entries | Branch F=100; TO_BOOL F=0; recorded TO_BOOL hit count 90 stays there, with no inferred branch hit/deferred share |
| Adaptive LOAD_ATTR entries 6, specialized entries 3, NOFB entries 1, all base LOAD_ATTR | F=10; A=6, S=3, O=1; NOFB contributes no adaptive entry |
| One specialized guard entry; member miss 1 and family miss 1; normal predicted fallback bypasses entry | R=1, S=1; two recorded keys each retain miss 1; no claimed 2 execution denominator or successful-hit count |
| Same guard route in tailcall mode, whose predicted call re-enters the generic target | R=2, S=1, A=1; recorded miss keys remain unchanged; cross-mode comparison rejects |
| A semantic event row also contains boxes 7 and ID/base/flags metadata | Event table renders only the four whitelisted event kinds; boxes 7 remains in its own metric |

A copied old denominator must fail `family_entries_100_not_190`. A copied
attribution-by-base assumption must fail `truth_hits_stay_at_recorded_key`.
An attempted miss subtraction must fail `guard_entry_remains_specialized`.
These are reader-only numeric controls, independent of native counter probes.

The coordinator owns boil/bench/run.py and README. Proposed coordinated delta:
replace the result key dispatches with counted_handler_entries and document
its target-entry/predicted-label contract; require the new coverage marker
and carry it into result metadata. Source search finds no other Python reader
of dispatches in boil/bench. Preserve boxes, by_instruction, allocations and
their definitions; do not extend the benchmark set or add memory/pause claims.
Update affected callers/docs together, with no old-key fallback. Existing
results retain their old source/coverage identity and are not rewritten.

## Exact proposed scope and first falsifiable controls

Proposed source files are Include/cpython/pystats.h,
Include/internal/pycore_stats.h, Python/ceval_macros.h and Python/pystats.c;
Tools/cases_generator/{ir_generator,ir_metadata_generator,py_metadata_generator}.py;
their intended products Include/internal/pycore_ir_metadata.h,
Lib/_opcode_metadata.py and Python/generated_ir_cases.c.h;
Tools/scripts/summarize_stats.py; Lib/test/test_pystats.py; separate
HandlerDomain tests in Lib/test/test_generated_cases.py; and new reader tests
at Lib/test/test_tools/test_summarize_stats.py using test_tools.import_tool.
No ceval frame-passing, GC, root-checker/call_effects, monitoring storage or
specializer source edit is proposed. Existing phase/root/NOFB oracles remain.
The shared test_generated_cases hunks and coordinator-owned benchmark reader
require explicit ownership handoff before implementation. No new shared CAPI
block is needed: use a task-owned isolated extension for primitive counters.
Its exact proposed artifact paths are
boil/design/work/controls/H3-1-stage2b3/stats_counter_fixture.c,
run_counter_controls.py, counter_probe.py, entry_lifecycle_probe.py and
run_reader_controls.py. Builds and copied variants go under a distinct
build/H3-1-stage2b3 cohort. Review the exact fixture/driver diffs and ownership
at source handoff before execution; do not accommodate them by editing CAPI,
ROOTS checker imports/fixtures or H1 frame-passing interfaces.

The first implementation checkpoint is a bounded, matching Py_STATS-build
control. A task-owned extension consumes the actual header/macro, allocates a
private PyStats before interception, saves the current thread's stats pointer,
and executes a fixed entry sequence such as low -> IR_JUMP -> high-NOFB ->
low. No Python call/allocation occurs while the private pointer is installed;
restore it on every path before producing Python output or freeing storage.
Copy only primitive selected counts and actual sizes for output. Exercise both
pair axes and the initial 0 source. Never execute a synthetic high handler or
construct malformed code to test counter storage.

The lifecycle fixture uses the actual emitted counter/IR_PREDICTED span from
a real family target and records its extraction/source hashes. In normal
mode a direct target entry followed by a jump to its predicted label counts
one entry; moving the emitted counter after that label counts two and must
fail `predicted_normal_entry_count_1`. Check the original relative ordering
in both emission modes as well as this numeric normal-mode control. In
tailcall mode the corresponding predicted route calls the target entry;
the positive expectation is two counted entries, not normal mode's one.
Bind any runtime tailcall result to an actual matching tailcall Py_STATS
build; a normal-mode fixture is not tailcall-runtime qualification.

The primitive pair fixture captures separate snapshots before/after clear:
on: L -> H records 0->L and L->H; off: K changes lastopcode to K without
incrementing entries/pairs; on: L records K->L. Clear zeroes the matrix but
keeps lastopcode=L, so the next H records L->H. A same-activation frame
transition to K records H->K; a new eval activation records 0->L and leaves
the outer activation's lastopcode=K. Returning to outer H records K->H.
Here L is a real low dispatch ID, H=IR_JUMP and K a valid high NOFB ID.
Permit the initial 0 source without treating CACHE as an executable target.
The C sequence isolates this state contract; source closure and live
call/frame workloads must also check the unchanged actual eval/tailcall
lastopcode paths. Do not claim that the private sequence alone executes
Python frame transitions or establishes their integration.

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
  event separation using the numeric cases above. Two input files with
  identical producer facts and entry counts 10/90 sum to 100 while version,
  IDs, flags, mode and domain count retain one unchanged copy. One-file
  metadata/mode mismatch, missing/duplicate facts and corrupted exported
  JSON reject before counter aggregation. Box/object counters stay unchanged.
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

## Response to independent design review

The source/artifact-only review is preserved by the coordinator at
boil/design/work/reviews/H3-1-stage2b3-design-review.md, SHA256
`26a2ed1360e41150345036f6de1f8c9e55ce62637fce95fc818e888499c6282b`.
It ran no native tests or probes. All findings are accepted; this response
is a revised proposal for review, not an implementation clearance.

| Finding | Design disposition / required evidence |
|---|---|
| Family denominator double-counts specialized entries | F sums raw entry rows only; no execution+hit+miss formula. The 10/90 fixture requires 100, and a copied old denominator fails its named assertion. |
| Truth-branch hit/deferred attribution is not recoverable from base | Keep TO_BOOL and branch semantic keys exactly as recorded; show event counts separately, with no inferred hit/deferred execution shares. Numeric fixture leaves TO_BOOL hit 90 at TO_BOOL with zero TO_BOOL entries. No new counter-site mapping. |
| Specialized misses are not a proven raw-entry partition | Raw shares use disjoint ADAPTIVE/SPECIALIZED/other entry totals. A guard failure remains a specialized entry; no miss subtraction or successful-hit claim. |
| boxes/metadata can enter event rendering | Whitelist the four semantic event fields, keep success/failure separate, and render the explicit mixed boxes/metadata fixture. |
| Per-file metadata must not be summed | Validate every raw file and exported JSON before addition; retain one identical metadata/coverage/mode object per cohort. Two-file count positive and one-file conflict/duplicate/missing-fact negatives are required. |
| Predicted-label ordering needs its own control | Preserve counter-before-label in both emitted forms; copied movement fails a normal-mode numeric fixture. Source inspection also establishes tailcall predicted entry differs, so capture mode and require its separate expectation. |
| Initial sentinel/off/clear/frame sequence and fixture ownership | Give explicit pair expectations, keep CACHE-0 source legal without execution, distinguish a primitive sequence from actual frame qualification, and name fixture/test paths. Shared hunks still require grants; no ROOTS/CAPI/H1 edits. |
| GET_ITER and memory precision | Derive GET_ITER from its real feedback-driven specializer, not the old manually printed list. Keep +230,368 bytes as source arithmetic until actual matching-build size/request evidence. |

No implementation, benchmark, build, test or native probe was performed for
this revision. The storage candidate, its evidence and landing reference
remain unchanged. The next action is independent design recheck and the
coordinator's later integrated-source/ownership handoff.
