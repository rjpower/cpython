# Porting batches and remaining work

Stage: v4 family analysis complete; AST sequence and constructor batches
reviewed/committed as e645682dab5 and b1dce616174. The full AST TU/object now pass
without diagnostics. The unified Clinic48 batch is reviewed and committed as
180b4b6d9af; list assertions are committed as 4bf934bb4ec. Three pure predicates
are committed as 69a2750540e. The first core14 draft requires correction after
early review; Sol is specifying finite examples for Luna's next bulk pass.
The coordinator integrates and validates the merged result.
MRO providers and the public wrapper
passed code/native attribution review; their delta remains held. The next
publication transaction has a read-only handoff, queued for design.

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
Sol is preparing explicit correction templates and witness fixes before another
Luna pass; no runtime acceptance is claimed. A separate Luna analysis is grouping
the frozen v6 errors for subsequent bounded designs, excluding this active packet.

## Census and evidence

The historical v3 build predates the separately checked graph and
call bridges: 540 error diagnostics at 425 distinct source locations in 90
source/header paths. These are observations
of that failed build, not independent defects or a current edit count. It also
reported 171 warnings, counted separately. Repeated headers, cascades, generated
expansions and already fixed calls inflate these figures.

The latest v6 merged probe reports 430 errors/161 objects present and no
executable; all 7,341 source paths stayed unchanged. Its Clinic v2 bytes were
subsequently reviewed and committed. Counts remain repeated/cascading compiler
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
