# Porting batches and remaining work

Stage: census complete; first generated-AST batch reviewed and committed as
e645682dab5. Luna analyzes the v4 compiler log and applies approved bulk rules;
Sol 6.1 reviews the AST constructor and designs high-yield common fixes, starting
with repeated keyword Clinic callers. The coordinator assigns, integrates and
validates the merged result. The MRO providers and immediate public wrapper
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
ast_type_init; both compilations fail. There is no AST object pass or candidate
execution. The next constructor packet is design-only.

## Census and evidence

The latest completed merged build predates the separately checked graph and
call bridges: 540 error diagnostics at 425 distinct source locations in 90
source/header paths. These are observations
of that failed build, not independent defects or a current edit count. It also
reported 171 warnings, counted separately. Repeated headers, cascades, generated
expansions and already fixed calls inflate these figures.

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
