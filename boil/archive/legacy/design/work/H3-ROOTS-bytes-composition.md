# Source composition inventory: H3 statistics, ROOTS and bytes writer

Read-only inventory, 2026-10-29. No merge, checkout, generation, build or target
execution was performed for this inventory. Integration belongs to the
coordinator. Branch-local results do not qualify the union.

Pinned inputs:

| Tree | Inspected identity | Role |
|---|---|---|
| primary | e504931803deaba8f16fc3825443bfc3baa05744 | integration base; preserve its current global gate admission fix |
| ROOTS | c67954f336a90402bf320d2cd02fa30e4ca7d223 | ce2ac native checkpoint plus exact threshold-test eligibility/docs successor |
| bytes writer | 08953c89274bf5541e49ebaf41147912734cbf15 | descends ce2ac; four native/test paths plus its record |
| original H3 | 81417aff0971fe621fec296a324c7a48315a6082 | isolated domain/monitoring-storage source; no mutation |
| stats | e8b3070067c7d7c882a41370a572d6dc5b27680e | H3 plus reviewed stats implementation and bounded evidence |

These were clean when each endpoint was inspected. ROOTS advanced from ce2ac
while this inventory was being read; c67954f is the explicit final endpoint,
not a relabeling of ce2ac products. Its test-only successor must keep the
coordinator's source/native bridge. Bytes does not contain that successor.
H3/stats fork before the queue052 prerequisite already present in primary
and ROOTS; preserve that primary/ROOTS state rather than replacing files.

## Concrete overlaps and order

H3/ROOTS share five paths. Stats adds a sixth; bytes adds its own CAPI hunk.
These are overlap domains, not a claim that all hunks conflict textually.

| Shared path | Composition obligation |
|---|---|
| Python/bytecodes.c | Keep ROOTS's two-argument _Py_Check_ArgsIterable call and H3's monitoring accessor/domain checks, no-monitoring guard, pristine/static-exec annotations. Regenerate from their combined DSL. |
| Python/ceval.h | Keep ROOTS precise helper annotations and exception/cause spans; keep H3 full-domain name lookup and CO_NO_MONITORING_EVENTS guard in monitor_handled. |
| Modules/_testinternalcapi.c | Retain ROOTS state/root-walk integration and H3 IR/domain/storage controls; add bytes gc_byteswriter_roots and its method entry. Check additive method tables/includes, not whole-file replacement. |
| Lib/test/test_generated_cases.py | Preserve ROOTS C-checker fixtures and H3/HandlerDomain/generator tests, including the stats target-name/adaptive checks. Shared-file ownership does not permit dropping either class. |
| Python/generated_ir_cases.c.h | Generated overlap: retain neither side as the final product; regenerate combined bytecodes/generator sources. |
| Python/ceval_macros.h | Sixth overlap from stats: keep ROOTS precise helper annotations and obsolete conversion-macro removal, plus full-domain counted-entry/pair macro. Preserve actual tailcall lastopcode forwarding and predicted routes. |

Bytes relative to ce2ac changes Include/internal/pycore_bytesobject.h,
Objects/bytesobject.c, its CAPI fixture, test_capi/test_bytes.py and the work
record. It adds no handler/domain/generator changes. Retain its registered
owner-slot lifetime through resize/finish/discard; do not register derived raw
buffer pointers or remove the existing opaque allocation protection.

Compose primary/ROOTS successor and bytes first, then the H3/stats source
changes, preserving coordinator-owned docs/tools. This describes review order,
not merge authorization. Resolve handwritten inputs before generated outputs.
The primary gate_run global slot correction must survive: the older stats
branch used outer TMPDIR unset, not a private-admission replacement.

Regenerate all seven existing products from that exact union, using its own
Tools/cases_generator modules and Python/bytecodes.c. A serial order matching
the current Makefile targets is opcode IDs, uop IDs, Python opcode metadata,
C opcode metadata, IR cases/targets, IR metadata. Products are:
Include/opcode_ids.h, Include/internal/pycore_uop_ids.h,
Lib/_opcode_metadata.py, Include/internal/pycore_opcode_metadata.h,
Python/generated_ir_cases.c.h, Python/ir_targets.h and
Include/internal/pycore_ir_metadata.h. No numbering change is intended; compare
actual full IDs/names/flags and regeneration idempotence. H3 metadata and the
stats ADAPTIVE bit come from the same combined HandlerDomain. Copying old
products would lose ROOTS's argument fix or stats's actual target counters.

## Smallest proposed actual-union gates

The coordinator chooses exact execution plans and existing admission. No old
receipt becomes a new-head pass, and this inventory grants no execution.

1. Review the six handwritten/generated overlap resolutions; run the existing
   complete generated-cases checker/domain tests and seven-product regeneration
   comparison. Retain ROOTS's negative fixtures and bytes omission oracle;
   no new validation framework or arbitrary repetition count is needed.
2. Build the actual union in its ordinary nonstats release and selected debug
   configurations, retaining the coordinator's O1/Og distinction. Build's
   existing C checker/inline audit must run on actual headers/handlers. Rebind
   actual source/config/compiler/products before qualification.
3. Run affected modules once in each relevant mode: code/IR/dis/opcodes/compile,
   monitoring and generated cases for H3; bytes CAPI for the writer. Reuse the
   reviewed ROOTS strict stale/walk/Park caller and affected thread/eval controls
   on the union, including actual debug kill where ownership matters. Exercise
   call/return, yield/resume and exception/collecting monitoring paths; private
   stats C-local models do not replace real frame coverage.
4. Use one matching union Py_STATS build, not an all-config stats crossproduct:
   stats lifecycle/high rows, reader tests and rebased private numeric controls.
   Also retain a real emitted raw file and validate its complete producer facts
   with the union reader; synthetic reader fixtures alone do not establish that
   live producer/reader boundary. Capture actual dispatch mode and sizes.
5. Run broad normal release/debug coverage early on the actual milestone union
   using the established full-suite profile. Further stress/support modes follow
   the coordinator's scoped ROOTS disposition and changed-interface risks,
   without automatic 120-transaction reuse/redo. A tailcall claim requires an
   actual union tailcall build and its relevant frame/root/monitoring checks;
   GCC non-tail stats controls supply no such claim.

Unresolved historical stress100k/HTTP findings and retained exclusions keep
their actual dispositions; composition does not erase them or add filters.

## H1 handoff

Stats changes no frame/call ABI. H1's passed-frame experiment still shares
ceval_macros.h, ceval.c, ir_generator.py, generated cases/targets and generator
hunks. It must start from the composed source/metadata and preserve lastopcode
in both stats and nonstats signatures, predicted routes and activation entry.
Actual ROOTS checker/root-attribution controls and compiler/configuration must
be handed off; zero fallback alone is not an ownership proof. The existing
value-ingress audit must be revalidated on this union before canonical guard
simplification; semantic slots differ from RAWROOT anchoring spans. H2 clearing
store/root-map changes, H3 sampling/renumbering and string storage remain separate.
