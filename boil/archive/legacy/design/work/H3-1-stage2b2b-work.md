# H3-1 stage 2b.2b: fixed monitoring storage and prepared updates

Status: implementation and bounded qualification complete; independent landing
review and integration remain pending. The accepted design is
[H3-1-stage2b2b.md](H3-1-stage2b2b.md), SHA256
`84e518cb872e0522247ffced7f131e943add7891c348746e9ca626c62b2f2116`.
Native baseline is `dc02f76c0ccf69cd7195431dc0335a349bdec48f`;
the tested tree is evidence/design parent `878647973ba` plus the exact
13 source hashes in [combined-sources.json](controls/H3-1-stage2b2b/combined-sources.json)
(SHA256 `a6c07b4ce7b2932a82c2bc8bcc187f8940359fe1674951f075ee1b98773e8557`).
The native patch is [combined-production.patch](controls/H3-1-stage2b2b/combined-production.patch),
SHA256 `b209836201b0fbdc9356dae76ec1fbe4a2e8fc259acc702be025ccfe6503abc6`.
The evidence commit follows those dirty-candidate runs; it does not relabel
historical runs as current-head tests.
Native implementation commit `76274ed06ec` contains exactly that tested patch.

## Behavior and ownership

LINE storage has one fixed `int` per word and separate `uint16_t` displaced
tokens; INSTRUCTION displaced tokens are also `uint16_t`. Lines admit -1 and
0 through INT_MAX, without relative arithmetic, width descriptors or multiple
runtime forms. Every slot starts at -1/token 0. Real heads receive immutable
absolute lines; relaxed trailing IR_JUMP slots retain cached -1/token 0 even
when immutable co_lines covers them with a positive line. LINE eligibility is
separate from stored line value. Tool and event masks remain 8-bit.

HandlerDomain owns immutable install/deinstrument relations. Generators emit
16-bit maps sized by the actual handler count. Runtime aliases normalize
through the generated base map before lookup. The private int-to-int
deinstrument endpoint validates the original scalar before narrowing or
indexing: invalid values return -1, and valid entries without an ordinary
counterpart return 0. Existing rewrite helpers preserve the exact pristine
handler and A–D fields. This stage adds no operation or renumbering.

Local updates prepare missing arrays privately, using old installed wrappers
and active masks to seed membership. Requests, versions, arrays and installed
changes publish only after all allocations succeed. Failure frees unpublished
blocks and preserves existing requests, active masks, code version, array
presence and executing words; an identical request retries. An independent
empty header/executing copy may remain. Published nonnull arrays retain their
addresses until code destruction; teardown frees the nested LINE token array.
The owning GIL excludes mutable-array readers and writers. No new retirement
protocol or nominal FT wrapper is claimed as concurrency protection.
SafeAddr2Line retains stage 2a's immutable implementation. Global requested
masks and versions retain partial-progress semantics: a failed per-code refresh
stays stale and retries on entry; there is no whole-interpreter rollback.

All five production GetLine callers move together, including both legacy
tracing endpoints. Bounds precede public Addr2Line cache access and both
legacy endpoint reads. GetLine drops the unused code argument and gains real
private-extension visibility. The coordinator recorded the private layout and
signature break, and external private/JIT rebuild requirement, in PROCESS.
There is no byte adapter or stable-ABI claim.

Shared CAPI scope is the reviewed pycore_instruments include and three VARARGS
functions/entries from proposal `71ec68cb...`. The state hook copies all
primitive fields before its first result allocation. Removing exactly those
additions reproduces the baseline CAPI file byte for byte. The existing
compiler, GC, scalar and safe-lookup fixtures are untouched. TestRootCheckC,
checker/call-effect sources and the SafeAddr2Line body are also unchanged.
Other owned files cover generators/products, instrumentation, code teardown
and lookup, legacy tracing, bytecodes/ceval.h and separate monitoring/domain
test classes. [final-provenance.json](controls/H3-1-stage2b2b/final-provenance.json)
records those boundaries and both native/CAPI binary hashes.

## Reproduced defects and bounded eligibility prerequisite

Old release and Og each lose A's enabled INSTRUCTION sites when B adds the
same event or causes multi-tool storage through LINE: `[1,2,3,4,5]` becomes
`[]`. Their allocation-failure control changes the requested mask from 0 to
96 despite MemoryError. Old logs/manifests are retained without installing a
source variant. The unchanged valid-AST probes also show corrupt LINE 257 or
missing LINE in release and assertions in Og for legitimate backward lines;
the old INT_MAX decoder asserts in Og. All eight corresponding final AST
cases pass. Primitive storage covers adjacent -1/0/INT_MAX lines and
255/511/65535 tokens, separately from canonical handler admission.

Development review found that the first prepared-update draft lost configured
requests on accepted CO_NO_MONITORING_EVENTS replacement code. The 54b fix
restores the local request/tool-version bookkeeping while leaving installed
state empty. Its execution regression exposed a separate inherited RESUME
loop after a global version advance: refresh was a no-op, so RESUME repeatedly
redispatched. The failed affected run retains its 5m38 duration, 1,262 completed
tests, SIGUSR1 stack at flagged func/test lines 297/316 and test_monitoring
failure. The executable was verified before signaling only that H3 worker;
normal gate cleanup ran. No ROOTS worker was interrupted.

The separately reviewed eligibility proposal v3 is retained as
[patch d5d88a7b...](controls/H3-1-stage2b2b/eligibility-proposal-v3.patch) and
[design b6541798...](controls/H3-1-stage2b2b/eligibility-proposal-v3.md).
Exactly three property guards close the paths: inside RESUME's existing
version mismatch, before direct/global/throw refresh sanity, and in the cold
monitor_handled callback before global fallback. The latter prevents the sole
exception route bypassing do_monitor_exc from using absent local metadata.
There is no version acknowledgment, static/shared write, weakened debug check
or test filter. This prerequisite is qualified with storage as one candidate
and must not be advertised or landed independently.

The bounded old 7c probe prints its actual normal mode and enter_suppressed
marker, then exits through a 2-second watchdog stack. The same exact
`4dee9c0f...` script now completes on release and Og: suppressed callbacks stay
empty, ordinary code delivers 17 instruction sites, and flagged generator
throw/on-stack refresh complete. A separate no-local-data handled-exception
case retains NULL metadata; paired ordinary code delivers the callback.
The unit regression also uses a collecting ordinary callback, independently
of the original RESUME timeout and without hiding the route behind a setter.

## Allocation evidence and limits

The isolated C recorder `4d8f7a6...` uses 128 fixed records, unchanged allocator
delegates and no Python calls or extra allocation inside callbacks. It restores
the original allocator before exception/result construction. Overflow,
calloc/realloc and untracked frees reject the measurement. Its read-only
inspector copies at most 128 initialized line/token slots before allocating
results; it does no preparation and exposes no pointers. Outer and inner
manifests bind the actual compiler executable, consumed headers, binary,
configuration and sources before and after each final run.

Both final builds measured these successful MEM-domain requests:

| Prepared case | Words | Requests | Peak/live new requested bytes |
|---|---:|---|---:|
| Fresh ordinary LINE+INSTRUCTION | 6 | 40, 12, 12 | 64 |
| Add second tool to installed LINE+INSTRUCTION | 6 | 6, 6, 6 | 18 |
| Valid backward absolute line | 3 | 28, 6, 6 | 40 |
| Valid INT_MAX absolute line | 3 | 28, 6, 6 | 40 |

Successful allocation prefixes before denial have peaks 0/40/52 bytes for
fresh six-word code, 0/6/12 for the transition, and 0/28/34 for three-word
code. Every failure finishes with zero live unpublished bytes, unchanged
snapshots/words and a passing identical retry. Sweeps stop at the first
successful nonnegative ordinal and require success. Existing failmalloc sits
above the recorder: denied requests never reach it. Their size/site is
unmeasured and is not attributed from ordinal numbers.

These are requested bytes, excluding common headers/executing copies, old
resident arrays, allocator overhead and RSS. On this ABI fixed LINE requests
`sizeof(header) + n*sizeof(int)` plus `n*sizeof(uint16_t)`, or `16 + 6n`, in
two allocations; INSTRUCTION requests `2n`. The trailing-[1] spare/padding is
included. The old packed-cache figures in the design are source formulas,
not a measured old heap baseline. The two maps occupy 604 bytes each and
EVENT_FOR_OPCODE 302 bytes, confirmed by final release symbols: 742 extra
static bytes over the old three tables. Whole-program memory, pauses and
checkpoint I attribution remain coordinator work; this checkpoint establishes
no speedup or heap-parity claim.

## Qualification ledger

All final native runs use the a6c/b209 candidate, release O3 and debug Og,
normal admission/build locks, fresh probe caches and bounded timeouts.

| Check | Release | Og debug |
|---|---|---|
| Smoke, 9 modules | 1,924 pass; 50 skipped | 1,924 pass; 20 skipped |
| Affected, 11 modules | 1,370 pass; 12 skipped | 1,370 pass; 12 skipped |
| Actual kill, same 11 modules, fail-env-changed | — | 1,370 pass; 12 skipped |
| Allocation, synthetic heads, no-monitor flag, global OOM | pass | pass |
| Valid AST backward -9/-2, module 0, INT_MAX | all 4 pass | all 4 pass |
| Copied-header positive and three semantic negatives | 0 / 1,1,1 | 0 / 1,1,1 |

The affected modules are test_generated_cases, test_dis, test_ir, test__opcode,
test_peepholer, test_compile, test_monitoring, test_code, test_marshal,
test_sys_settrace and test_sys_setprofile. They retain compiler no-Python phase
and collecting CALL/C_RETURN/C_RAISE NOFB restoration oracles. All 11 kill
workers enter through worker.main and assert actual verifier `(2,0)`, stress 0,
optimize 0, __debug__ true, and the exact local CAPI origin/hash. The checked
root stream has 40 processes, 839 collections, zero fallback and zero unpinned.

Regeneration/Clinic leaves tracked diff, status, all frozen sources and binary
unchanged. IDs remain 264 opcode/pseudo entries, 287 dispatch handlers and
302 slots, first IR-only 267; all C/Python domain metadata agrees. Five of
seven generated products remain byte-identical to the parent. The intended
changes are the IR metadata relations and generated IR handler bodies.
The C checker passes all 13 currently marked files; traversal audit passes.
Unbuilt syntax checks pass 19 compilations; _tkinter, _dbm, _gdbm and readline
remain unchecked for unavailable headers. No final full suite, ASan, stress,
benchmark or integration result is claimed by this bounded slice.

Copied controls use task-owned sources only. The positive retains the exact
CAPI roundtrip and deinstrument bodies, the real exported GetLine and verified
copied-header resolution. Narrowed token/line stores and omitted alias
normalization fail their named semantic assertions. No malformed or
wrong-layout instruction executes. Live canonical displaced IDs remain below
256; primitive 65535 coverage does not claim live high-handler execution.

Earlier 603 smoke/affected and 54b smoke successes stay source-bound; the 54b
affected failure remains failed. Early allocation/AST outputs and final outputs
use different directories. An initial copied-helper compile error (GNU23
instead of the core C11 context) remains a harness failure, with no negative
credit. Corrected v2 uses C11 and the production CAPI assertion context.
The first audit launcher omitted the script directory from runpy's import
path; it failed before the checker and is retained separately from passing v2.
No native edit followed either harness correction.
Raw logs and patch context retain their original whitespace; source, scripts
and documentation pass the whitespace diff check.

Independent read-only development review `/root/compiler_review` cleared all
13 source hashes and the exact combined diff, including direct callback
closure. Its primary review record has SHA256
`96e493f68d598e7920621bb830da1097e834b6d069edbcfcce48ace79fbf2524`.
That source/artifact review ran no native tests and is not landing approval.
Final landing review and merged integration validation remain due.

A queued same-build transaction briefly reserved a second machine slot while
waiting for its own active build lock. It was canceled before acquiring that
lock, without sweeping or signaling the active build. Subsequent H3 batches
run sequentially, leaving a lane to ROOTS. Controls never mutate live source or
headers. Stats, remaining inspection, transport and final renumbering are
outside this implementation; H1 remains a separate design/handoff.
