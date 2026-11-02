# Porting batches and remaining work

Stage: v4 family analysis complete; AST sequence and constructor batches
reviewed/committed as e645682dab5 and b1dce616174. The full AST TU/object now pass
without diagnostics. The unified Clinic48 batch is reviewed and committed as
180b4b6d9af; list assertions are committed as 4bf934bb4ec. Three pure predicates
are committed as 69a2750540e; corrected core14 is committed as e789685426a.
Template is committed as e0dfcd4d7ff and corrected TypeVar as bef49ec8fc9.
Descriptor calls are committed as ca594319d0b. The shared bytes/bytearray join
rule is committed as checkpoint32; ABC pair readers are checkpoint33.
The Clinic helper rule for bytes, bytearray and enumerate plus four existing
fixtures is checkpoint34 after independent review. Luna ports three iterator
constructors; Sol designs type wrappers and reviews the isolated counter patch.
The coordinator integrates and validates the merged result.
MRO providers and the public wrapper
passed code/native attribution review; their delta remains held. The next
publication transaction and immediate bases/readiness callers are in design.

## Required baseline versus later tuning

Universal PyValue describes stored language values. An owning temporary
PyObject view at a C boundary is valid: it may box an inline scalar, remains
alive through the call and is released afterward. We do not need to convert
every PyObject local or C function into direct-value code before measuring.

| Required for a safe baseline | Can follow the first measurement |
| --- | --- |
| Remove reads/casts that treat value storage as object pointers | Avoid boxing in otherwise correct owning C bridges |
| Port removed borrowed accessors to explicit owners or scoped leases | Make generic iteration return canonical values directly |
| Keep parser outputs and native argument arrays alive through their users | More value-native builtin/extension implementations |
| Preserve errors and coherent state through allocation, callbacks and cleanup | IR optimizations, dispatch tuning and register allocation |
| Complete type/cache invalidation, guards, mutation and publication contracts | Method/tracing JIT implementation after this checkpoint |
| Port startup and the selected core/bundled build; qualify exercised routes | Other build profiles/platforms, with explicit scope |

All extensions will be rebuilt and ported. They need not all become value-native
before a first core interpreter measurement. Extensions outside the chosen
baseline profile remain queued ports; record exclusions rather than quietly
claiming universal extension qualification.

## Reusable rules

| Pattern | Rule and automation boundary |
| --- | --- |
| Already-owned list item | Replace Py_NewRef of a borrowed item read with the owning fallible getter, check failure, and retain the existing lifetime. Add first-error cleanup where required. |
| Expected type/name metadata | Prove the canonical kind before pointer access. Keep the source owner through pure reads; own selected metadata across callbacks. |
| Tuple/native argument window | Use the reviewed owner-scope object view and end it after the implementation and converter cleanup. Never cast PyValue storage to PyObject pointers. |
| Dictionary lookup | Use owning value/object APIs and preserve missing/error distinction. A missing value must not conceal a pending error. |
| Mutable-container iteration | Use an owned snapshot or a reviewed restart/revalidation contract if the body allocates or calls out. Renaming an iterator API alone is insufficient. |
| Value-field write | Prepare the new owner, publish coherent state, then retire the displaced owner. Preserve first error and revalidate when callbacks can change the selected target. |
| Generated products | Change the generator, regenerate, check unrelated output and regenerate again for idempotence. Do not hand-edit each expansion. |

These are rule families, not blanket source-edit grants. Borrowed variables that
escape, nested/keyword parser outputs, descriptor lookup, C3/type mutation and
publication need explicit contracts. Once reviewed, repeated applications can
share a batch design and checks rather than repeat the entire design exercise.

## Batch execution

Each packet names its rule, exact files/producer, input hashes, dependencies,
worker and acceptance checks. Use disjoint file ownership. A rewriter should
match only the proven pattern, report unmatched/ambiguous sites, and produce a
reviewable patch. Cheap workers can perform the edits and flag exceptions; root
owns the index, commit and merged build.

Luna identifies build-error patterns and implements approved solutions; Sol 6.1
designs and reviews the solutions. Keep root-cause counts distinct from cascades.
The v4 analysis starts with 519 error observations at 406 source locations in
89 paths. The keyword Clinic family includes 40 call mismatches and 40 repeated
shared-header conditional mismatches; getter failures also cause many later
pointer-from-integer errors. Exact classification and packets live in
build/pyvalue-v4-error-families-20261102-v1. These are early grouped observations,
not independent defect counts or a claim that any of these fixes has passed.

Run scope/diff checks, producer checks and affected translation-unit checks
early. Independent review checks the actual patch and results. Group compatible
packets for a serialized merged diagnostic build, then startup/focused runtime
tests. Reserve broad qualification and quiet timing for a linked candidate.

First pilot: one Obj2ModVisitor list-item emission in Parser/asdl_c.py expands
74 times in Python/Python-ast.c. Use PyList_GetItemRef with balanced recursion,
one conversion, first-error retirement and the existing mutation check. This
does not grant the generator's separate dictionary/constructor ports.

The pilot is committed: root's independent normalization proves all generated
text outside the 74 reviewed blocks exact. Regeneration is idempotent and both
AST headers unchanged. Three host 3.13.15 controls pass. A stable same-header
full-TU syntax comparison reports 81 diagnostics before and seven afterward,
with no new diagnostic messages/contexts. All seven remain in unchanged
ast_type_init at that historical endpoint; both compilations failed.

The following constructor packet is now committed as b1dce616174. Stable native
checks go from seven diagnostics to zero and the actual object builds. Output
outside ast_type_init, including all 74 sequence sites, remains exact. Independent
Sol 6.1 code/result review accepts it. Seven host tests ran on 3.14: four pass
and three skip older AST error semantics. All seven candidate tests remain
unexecuted. Evidence: build/pyvalue-ast-constructor-design-20261102-v1/commit.json.

The Clinic fixed-keyword rule now has one path for 48 production constructors
and three Owner fixtures. Independent review proves 769 unselected function
bodies, 27 handwritten inputs and all twenty earlier positional wrappers exact.
Two generation passes are idempotent; 285 host checks pass with explicit
unavailable runtime/fixture omissions. Root checked 27 configured translation
units: six pass completely, and no selected-wrapper error/warning contexts
remain. No candidate C was executed. Evidence:
build/pyvalue-clinic-keyword-bulk-20261102-v2/commit.json.

Two canonical non-NULL list-sort assertions replace C pointer-truth checks;
all other source bytes match and the actual list object builds. Evidence:
build/pyvalue-list-sort-validity-20261102-v1/commit.json.

Reader grants are finite: six files/fourteen functions for Luna in
build/pyvalue-sol-port-rules-20261102-v1/coordinator-core-readers-grant.md;
three pure functions in funcobject/typevarobject, now committed. Owning generic
getters retain operands through callbacks and preserve first errors during
cleanup; pure kind checks do not box. TypeVar's tuple-vararg parser resource
is a separate queued producer amendment. Native checks serialize at freeze.

The three pure functions passed independent actual code/native review and were
committed as 69a2750540e. All other bytes remain exact except explicit value
includes. Full-TU error counts go 6→3 and 11→10; both TUs still fail, with zero
selected-function diagnostics after. Evidence:
build/pyvalue-pure-predicates-20261102-v1/commit.json.

Core14 v1 is rejected before integration: an outside-grant brace edit breaks
codec syntax, and grouped/unprotected owner closes violate the per-owner
first-error rule. Six host tests do not execute that C. The original snapshot,
false scope claim and compiler failure are retained in
build/pyvalue-core-readers-bulk-20261102-v1/coordinator-early-review.json.
The subsequent correction passed after an API-type correction (Copy returns int,
Append returns PyStatus) and deterministic host witness fixes. It is committed
as e789685426a, six sources/fourteen bodies plus tests. Configured native TU
errors go 53→39, with zero selected-function diagnostics; the AST unparser TU
and object pass. Eight host 3.14 controls pass. Candidate startup/config/path-hook
and fault tests remain pending. Evidence:
build/pyvalue-core-readers-bulk-20261102-v2/commit.json.

The frozen v6 inventory is complete: 430 observations/361 locations/67 paths,
including 253 likely cascades, not independent edits. TypeVar is the last
fixed-keyword mismatch family residue; its first implementation removes the
two wrapper/macro errors, but review requires collision-safe cleanup temporaries
and complete retained controls before acceptance. Template's next three-body
rule corrects the earlier blanket strings-metadata assumption: compiler-built
TemplateStr ASTs can contain scalar strings slots. Boundaries need owning views;
interior copies can stay canonical. Evidence and finite grants live in
build/pyvalue-v6-error-families-20261102-v1 and
build/pyvalue-sol-port-rules-20261102-v1.

Template passed independent review and is checkpoint29 e0dfcd4d7ff. Three
functions now compile as a full TU/object without diagnostics (previously seven
errors/two warnings). Three retained host 3.15 controls pass; candidate execution
and the type-pin reentrancy witness remain pending. Evidence:
build/pyvalue-template-readers-bulk-20261102-v1/commit.json.

TypeVar passed after collision-safe temporary names, durable controls and raw
reproducible receipts. It is checkpoint30 bef49ec8fc9: one additional scoped
tuple-vararg wrapper under the same predicate, prior51 keyword/twenty positional
bodies exact. Producer suite: 427 run, 311 pass, 116 unavailable fixture skips.
Combined producer/keyword suite: 437 run, 321 pass, 116 skips; these overlap.
Configured TU errors go 10→8, with zero selected-wrapper diagnostics. The whole
TU still fails. Evidence: build/pyvalue-clinic-typevar-bulk-20261102-v2/commit.json.

The completed descriptor grant covers two bodies in descrobject.c plus focused
test_descr controls. It reuses the immutable tuple OwnerScope after classmethod
binding, and ordinary owned receiver/slice on the wrapper path; no new adapter
API or raw value-array cast. Exact grant:
build/pyvalue-sol-port-rules-20261102-v1/coordinator-descriptor-grant.md.
It is checkpoint31 ca594319d0b: full TU/object pass without diagnostics (four
errors before), three retained host controls, candidate execution pending.
Evidence: build/pyvalue-descriptor-calls-bulk-20261102-v1/commit.json.

The completed shared join grant covers two stringlib bodies instantiated for bytes
and bytearray. It uses checked owning current items, existing buffer leases,
ascending once-only release at original buffer addresses and protected output
retirement. The existing normalized TypeError retains type/name backing owners
through formatting. No separator policy or buffer API changes. Exact grant:
build/pyvalue-sol-port-rules-20261102-v1/coordinator-bytes-join-grant.md.

The held typeobject v7 inventory attributes all 38 observations to unadmitted
functions. Its initial report accidentally used the future transaction input
list as accepted MRO15; that receipt is preserved and corrected. The accepted
fifteen-body provider scope still has zero changed-function diagnostics. Source
d0721997 is unchanged. Corrected evidence:
build/pyvalue-typeobject-error-patterns-20261102-v1.

## Census and evidence

The historical v3 build predates the separately checked graph and
call bridges: 540 error diagnostics at 425 distinct source locations in 90
source/header paths. These are observations
of that failed build, not independent defects or a current edit count. It also
reported 171 warnings, counted separately. Repeated headers, cascades, generated
expansions and already fixed calls inflate these figures.

The latest v11 merged probe reports 350 errors/164 objects present and no
executable; all 7,345 source paths stayed unchanged at checkpoint39 plus held
typeobject. Counts remain repeated/cascading compiler
observations, not independent defects or a percentage complete.

The production scan finds 363 PyTuple_GET_ITEM, 84 PyList_GET_ITEM and
40 PyDict_Next occurrences, excluding recognized generated products and test sources.
These are search candidates, not verified defects. Generated products and tests
are classified separately in the evidence; generated recognition is explicit
and not exhaustive. A grep match can be a comment, valid
guarded use or an unbuilt configuration. Conversely, a scalar-unsafe pointer
decode can compile without appearing in these lists. No defensible percentage
or completion date follows from either count.

Evidence: build/pyvalue-porting-census-20261102-v2 contains the reproducible
error-only census and classified occurrence scan. The v1 directory retains the
initial warning-inclusive scan and AST pilot design/review. The
[board](../../README.md) owns live assignments and source checkpoints. There is
still no linked restored candidate or restored-runtime timing.

Shared join passed the coordinator's independent actual code/results review
and is committed as `87ec5704d6c`, three paths/two bodies instantiated for bytes
and bytearray. Configured native errors fell 8→4 and 5→2; selected functions
are clean, both full TUs still fail. Eight extracted host controls pass on
Python 3.15.1, including existing large joins and callback shrink/replacement.
Candidate execution and C-level cleanup fault injection remain pending.
Evidence: build/pyvalue-bytes-join-bulk-20261102-v1/{coordinator-review.md,commit.json}.
Merged v8 has 392 errors/164 objects and no executable on 7,342 unchanged inputs;
The completed v8 shortlist records 392 observations, 332 unique sites and
61 paths in build/pyvalue-v8-error-families-20261102-v1. Its automatic lexical
census attempt failed and is disclosed; examples were checked in source.
The next finite source grant covers compute_abstract_methods in Modules/_abc.c
plus a value include and focused new tests. Root independently gated Sol's
existing selected-pair draft; it was not an already accepted blanket rule.
Luna applies it while Sol designs the MRO caller transaction. Exact grant:
build/pyvalue-sol-port-rules-20261102-v1/coordinator-abc-grant.md.

ABC selected pairs are checkpoint33 de422d7fbb3. V1 changed inherited-value
finalizer/hash order and was revised. V2 restores that order, passes nine host
controls and has native selected errors6→0/full TU7→1; the remaining registry
traversal error is unchanged. Both rejected v1 source files were recovered
byte-exact against original hashes. Evidence: build/pyvalue-abc-pairs-bulk-20261102-v2.

The first Luna helper assignment froze inputs but did not implement the
generator. Sol implemented the two-file kernel; root reviewed its actual diff
and 697-function in-memory audit (seven selected, 690 excluded exact). Producer
checks: 314 pass/116 unavailable-fixture skips. Luna now performs product
regeneration, scope/idempotence proofs and durable host controls. No helper
source checkpoint or native/runtime pass is claimed. Evidence:
build/pyvalue-clinic-helper-kernel-20261102-v1/coordinator-review.md.

Clinic helpers are checkpoint34 fa398d21618 after kernel and final actual
code/results review. The seven-path batch changes seven constructors/21
generated bodies; 690 other templates and the previous51 keyword/twenty
positional wrappers remain exact. Producer:432run/316pass/116missingfixture
skips; two expanded host controls pass. Native errors4→3,2→1,7→6,35→31;
all selected contexts clean, all four full TUs still fail. Weak/incorrect
negative/arity/state assertions were corrected with preserved before-images.
Evidence: build/pyvalue-clinic-helper-bulk-20261102-v2/commit.json.

The iterator constructor packet is checkpoint35 aa46fc4fd01: product_new,
zip_longest_new and itertools_teedataobject_impl only. Owned tuple selection
and bounded canonical tee capture replace seven selected errors. V1 failed
early product cleanup review; v2 fixes that owner/exception path. Native full
TU errors are 34→27, with zero warnings; seven host controls pass and one
large-address-space control skips. Candidate execution remains pending.
The earlier report of 29 errors/five warnings was wrong: five shared-header
errors were misclassified. Raw receipts and the correction are preserved in
build/pyvalue-itertools-readers-bulk-20261102-v2/native-attribution.json.
Design: build/pyvalue-itertools-readers-design-20261102-v1/coordinator-accept.md.
Admission: build/pyvalue-itertools-readers-bulk-20261102-v2/commit.json.

Current parallel work: Sol reviews the next fifteen-function operator/LRU
consumer rule and shared explicit caller-owned format parser. Luna applies the
next bulk packet after its design gate.
Root owns code/result review, composition and merged validation. The v9 family
census records 376 errors/320 sites/59 paths and 57 separate warnings; it does not
count independent ports. Evidence: build/pyvalue-v9-error-families-20261102-v1.

The recording-only MRO counter foundation is checkpoint 36, `5a9d4841a67`. Actual
old/new public-layout native probes add no diagnostics; typeobject remains at
38 errors/23 warnings and the test helper at 1 error/2 warnings. Counter observations,
near-limit execution and full publication/rollback authority remain pending.
Evidence: build/pyvalue-mro-counter-bulk-20261102-v1/commit.json.

Thirteen slot wrappers are checkpoint 37, `ef48e73fb24`. The isolated Luna batch
passed independent Sol and coordinator actual-code review and composes exactly
with the counter foundation. Native selected 15→0/full TU 38→23 errors, 23 warnings
unchanged; six host controls pass, candidate/fault paths unexecuted. The copy
command receipt was corrected with its original retained. Evidence:
build/pyvalue-type-slot-wrappers-bulk-20261102-v1/commit.json.

The previously reviewed type_new argument boundary and exact test are now
checkpoint38, `d2f39118c93`. Root projected only that function into the index;
all live source bytes stayed unchanged. Its twelve host controls and three
unexecuted candidate methods retain the original independent review. Evidence:
build/pyvalue-startup-type-new-checkpoint-20261102-v1/commit.json.

Construction readers are checkpoint39, `913c15ce15b`: ten bodies plus the
explicit caller base/metaclass pins and a new test. Independent Sol/root actual
code review and composed native checks pass: selected 11→0/full TU 23→12 errors,
23 warnings unchanged. Six host controls pass, candidate/fault cases unexecuted.
The isolated compiler receipt initially counted only seven C-file warnings;
sixteen header warnings bring both TU totals to 23. Original receipts and the
correction are retained. Evidence:
build/pyvalue-type-construction-readers-bulk-20261102-v1/commit.json.
