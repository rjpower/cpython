# Option 5 untimed reached-route census

Proposal for root execution only; no runtime or probe has run as part of this
document. Keep the measured release/debug candidate at `4fbd16a1462` unchanged.

## Preferred existing-ELF GDB probe

Root selected [option5-hits.gdb](option5-hits.gdb) with the companion
[signal driver](option5-hit-driver.py). This requires no stats build or runtime
patch. Root launches GDB with auto Python loading disabled before loading the
ELF, the unchanged candidate as inferior, and `-S -B option5-hit-driver.py
KERNEL_PATH [VARIANT]`. Hash0/JIT-off are required. The companion follows the
existing constructor-hit driver's load/setup/warmup/run(1) pattern; its second
SIGUSR1 stops counting before JSON reporting and process shutdown. The optional
variant is necessary for shared modules, for example logging_silent.

The script asserts clean source HEAD4fbd, the exact reviewed ELF SHA256, success
offset0xd7 and native store bytes49 89 3f. Root retains its invocation, source/
ELF identity and full GDB output in the round54 journal. It counts only three
native points between signals: exact refined-handler entry, the successful B
store, and generic ADD_INT entry. It evaluates no inferior variables or helper
functions. All hit callbacks increment debugger-side counters and auto-continue.
The final OPTION5_HITS JSON appears only after a normal zero exit and the
expected signal sequence. A failed identity/boundary check invalidates the run.

`successful_overwrites` is the dynamic number of removed B NULL stores in that
single diagnostic run. Keep `refined_entries` and `generic_entries` separate:
they are handler attempts, and a malformed-geometry fallback would enter both.
They are not distinct static sites or a count of all generic BINARY_OP kinds.
No kernel monkeypatching, monitoring or candidate counter patch is used. GDB
trap cost makes these results unsuitable for timings. Run3–6 actual hot kernels
as root planned; do not rescale loop1 into an elapsed claim.

## Alternative stats probe

Use a separate diagnostic stats build, not counters, tracing, monkeypatches or
an altered kernel inside any timed pair. Counters are diagnostic evidence of
route frequency; their executable supplies no elapsed-time comparison.

## Exact probe

The existing Py_STATS opcode array is sized by IR_NUM_HANDLERS, and the
OVERWRITE_TEMP row326 is available. The production generated handler currently
charges INSTRUCTION_STATS to canonical ADD_INT, so that existing total cannot
distinguish the refinement. In the diagnostic generator only:

1. Emit `OPCODE_EXE_INC(BINARY_OP_ADD_INT_OVERWRITE_TEMP)` on refined-handler
   entry before geometry fallback. Keep normal canonical INSTRUCTION_STATS
   unchanged. This is an additional diagnostic row, not another instruction in
   the denominator of the existing canonical opcode total.
2. Emit `STAT_INC(BINARY_OP_ADD_INT_OVERWRITE_TEMP, hit)` immediately before
   IR_OVERWRITE_INLINE_INT_TEMP. This counts successful actual publications:
   geometry passed, both integer guards passed, overflow allocation succeeded.
   It is the exact number of removed B NULL stores, including boxed results.
3. Optionally count the shared geometry-fallback branch as the variant row's
   `deferred`, without changing the existing canonical miss/deopt counters.
   Entry minus success minus geometry fallback includes inline guard misses
   and rare arithmetic errors; do not call all entries saved stores.

These native stats increments neither allocate nor call Python; they do not
alter operands, owners, handler selection or the commit sequence. Keep the
canonical token certificate intact: counters are emitted by the diagnostic
generator, not inserted as unknown calls into the certified DSL body. Root
must label this probe patch/build separately, preserve its exact diff and
inspect that increments cannot leak into the ordinary candidate.

## Unchanged work and boundaries

Use canonical `pyjit/bench/run.py` registry paths, variants and loop counts:
generators2, coroutines4, deltablue30, richards2, comprehensions6000 and
logging_silent60000. Use the same module loader and optional setup as
`pyjit/bench/internal/driver.py`. Load/setup, then call `module.run(loops)` once
for ordinary warmup. Clear the stats and enable gathering only for one further
unchanged `module.run(loops)`. Dump while gathering is enabled, then turn it off.
The existing actual APIs are sys._stats_clear/_stats_on/_stats_dump/_stats_off.
Give each process a task-owned directory under round54 evidence using
`-X pystats_dir=...` or PYTHON_PYSTATS_DIR; do not use the default /tmp output.
Keep hash0/JIT-off and record source/ELF identities. A small copied untimed
driver may control these boundaries; never replace or wrap kernel functions.

Report each workload's canonical ADD_INT executions, refined entries,
successful overwrites, geometry fallbacks, successful overwrites per outer
loop and successful-overwrite fraction of canonical ADD_INT entries. The
numerator is weighted dynamic work actually executed, not static opcode count.
Keep per-workload figures; do not sum unlike outer loop units into a purported
whole-corpus throughput percentage. Include all29 only after the six-kernel
census establishes relevant hits.

Separately snapshot `_testinternalcapi.get_ir(code, True)` after the measured
diagnostic phase, with stats off. List selected OVERWRITE_TEMP sites by filename,
qualname, word index and A/B/C/local boundary, using live functions plus nested
code constants from the loaded corpus/dependency modules. Also list canonical
ADD_INT sites with eligible geometry. This shows where the refinement was
selected, but a post-phase selected site is **not** proof it executed during
that phase: imports/warmup can nominate sites too. Do not assign the aggregate
dynamic count evenly to those sites or claim per-site counts without another
probe. The successful-publication counter establishes actual weighted hits
without that false inference.

## Decision evidence

For each successful publication the corrected release handler removes one
B-home NULL store. It retains four recurring geometry checks: A/B equality,
B/C inequality, nondiscard B, and B/local classification, plus extraction and
branches. It saves generic B classification and destination replacement/
discard machinery; input closes were already absent natively. Report actual
successful hits against all refined entries, and compare the unchanged
candidate's native instruction/cycle/elapsed evidence separately. Zero or scarce
hits cannot justify broadening the family to manufacture a result. A constructor
proof can remove checks only through the separate reviewed follow-up; it cannot
be substituted into this initial candidate's measurement record.

An instruction-monitoring census alone is insufficient: instrumentation can
restore canonical handlers and callbacks can change dispatch behavior, so it
would measure geometry-eligible opcode events rather than actual successful
refined publications. Static site counts alone are likewise insufficient.
