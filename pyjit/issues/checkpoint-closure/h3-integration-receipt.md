# H3 integration receipt

The complete generated handler-domain producer and strict reader are integrated
after `f81ebbd646d`. Both stats counter axes use the generated 319-handler
bound. The producer declares 298 named entries and records actual entered
targets, pairs and separate specialization events. The reader validates each
dump's complete contract before aggregation; it does not add metadata across
files or infer hits by subtracting misses.

Merged qualification: debug/release/stats builds, regeneration, lint and 4,640
tests per debug/release build pass. The exact `malloc_debug` plus enabled-stats
import that previously crashed now exits zero. Its raw dump includes handler
306 with 150 entries and six misses, exercising the previously out-of-bounds
write. The strict reader renders that unchanged dump and passes 13 synthetic
contract tests.

The stats-build follow-up ran 2,000 tests across the eight previously failing
files plus the new reader tests. Eight files pass; only six assertions in
test_pystats initially fail. Four are stale stimuli/expectations for newly
value-native operations; two inherit enabled startup stats. The reviewed
fixture correction then passes all ten test_pystats tests with the parent
still collecting. It explicitly disables inherited startup collection unless
requested, asserts zero for ported core operations and retains deliberate
test-only adapter/thunk/owning-bridge controls. The adapter/thunk source probe
records exactly two call-adapter and three thunk boxes. No counter or gate
threshold is relaxed.

All 29 kernels now complete counting with real handler coverage, retaining 87
raw dumps under `checkpoint-closure/kernels-f81-h3v2`. Every differenced
per-loop box count is zero, but startup-inclusive counts remain nonzero
(warmups range from two to 1,175 boxes). The strict R27 gate fails as intended.
This establishes measurement coverage and removes the diagnosed crash; it
does not qualify the PyValue checkpoint. Cold compiler/marshal/keyword ports,
the full suite tail and process/inventory accounting remain open.
