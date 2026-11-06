# Hardware performance evidence review: `6a0e4c12c17`

Independent review on 2026-11-06 accepts the successful hardware counter and
user-space DWARF recordings as diagnostic evidence. No collection or arithmetic
blocker remains. This does not qualify R27, close census debt, or establish that
one proposed optimization will make the interpreter competitive with upstream.
The reviewer read source, JSON, and coordinator-generated offline reports; it
ran no interpreter, benchmark, test, build, or perf workload.

All evidence directories below are under
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/checkpoint-closure/`.
The frozen executable identities remain candidate `6a0e4c12c17`, ELF
`df7d4401cc45f00e9dabead7cb8fc8f3b06bd00c`, and upstream `3a440b65695`, ELF
`ff51c387c04ccd5a2664e5a572bc68e491ec0db1`. Live configured build interpreters
were executed after matching retained symbolization ELF identities.

## Protocol corrections and empty-region calibration

`perf-empty-6a0e4c12c17/` is a preserved failed implementation check. The local
perf writes the five bytes `ack\n\0`; the initial collector required the
four-byte `ack\n` form. It therefore rejected all 18 invocations before their
hot loops. The saved correction accepts exactly those two byte sequences in
the child and parent validators, retaining both decoded response and hex bytes.
It does not strip arbitrary suffixes or accept a prefix.

`perf-empty-6a0e4c12c17-v2/` contains 18 successful invocations: three rounds,
two roles and three grouped event pairs, using logging setup/warmup followed by
`run(0)`. All are unique and valid, all hot-loop counts are zero, all requested
events ran 100% of their enabled runtime, and before/after identities are
byte-identical. The maximum recorded load is 2.5821.

Enable acknowledgment latency is 59.2/68.2/92.9 microseconds and disable latency
22.9/26.8/35.6 microseconds, as minimum/median/maximum across the 18 invocations.
The timed empty call takes 10.69–21.30 microseconds wall time; event runtime is
50.47–94.44 microseconds. The enabled counter envelope includes timing/control
work beyond the timed benchmark call.

| Median empty invocation | Upstream | Candidate |
| --- | ---: | ---: |
| User cycles | 132,754 | 168,627 |
| Retired user instructions | 33,685 | 68,311 |
| User branches | 6,321 | 13,604 |
| User branch misses | 790 | 998 |
| User cache references | 9,846 | 15,644 |
| User cache misses | 4,249 | 7,401 |

These are `run(0)` plus protocol overhead observations, not performance scores.
No failed or empty-region result is mixed into the hot measurements, and no
calibration count is subtracted. Empty instruction counts are below 0.00036%
of the corresponding successful hot invocation instruction counts. This is
an observed scale comparison, not an assumption that all overhead is constant.

## Hardware stat acceptance

`perf-stat-6a0e4c12c17/` contains all 54 unique invocations: three rounds,
three kernels, two roles, and three grouped pairs. Groups are
`{cycles:u,instructions:u}`, `{branches:u,branch-misses:u}`, and
`{cache-references:u,cache-misses:u}`. Setup and warmup precede enable/ack;
the hot call precedes disable/ack. Role/group order rotates deterministically.
Loops are fixed from the fresh per-role timing median, targeting two seconds.
No selected retry or failed-attempt sample enters the aggregate.

Every invocation exits zero, has both valid acknowledgments and no recorded
error, and has 100% running for both events. All 108 original perf event rows
match journal counts, runtime and running percentages exactly. Independent
division by the saved loop count reproduces every normalized counter. Loops
are upstream/candidate 549/290 for deltablue, 1,652,262/914,534 for logging,
and 112,509/75,045 for comprehensions. They match `ceil(2e9 / median_ns)`.

Hot wall time is 1.852–2.144 seconds and process CPU/wall is at least 0.99398.
Recorded pre/post load reaches 3.1099, below the unchanged rejection threshold
four. Successful wait observations use the declared resume threshold below
three. Enable latency is 57–133 microseconds and disable latency 52–112
microseconds. Status is complete, all 54 invocations are valid, identities are
byte-identical, and R27 eligibility remains false.

The following ratios divide the candidate median count per loop by the
upstream median count per loop, separately for each event. CPI ratio is the
cycles ratio divided by the instruction ratio; it is derived arithmetic,
not an additional hardware event.

| Kernel | Instructions C/U | Cycles C/U | CPI C/U | Branches C/U | Branch misses C/U |
| --- | ---: | ---: | ---: | ---: | ---: |
| deltablue | 2.046606 | 1.929480 | 0.942771 | 2.298633 | 1.046532 |
| logging_silent | 1.903192 | 1.939236 | 1.018938 | 2.146603 | 0.511446 |
| comprehensions | 1.628162 | 1.490982 | 0.915746 | 1.723108 | 1.714583 |

Instruction counts vary by less than 0.047% max/min spread in every role/case;
cycle spreads stay below 1.973%. The three selected regressions execute
substantially more user instructions, while cycles per instruction remain
similar or lower. This supports reducing executed work as the main direction
for these targets. It does not assign that work to boxing or one subsystem.

Cache counters are substantially less stable. Upstream logging cache-reference
counts are 68,709,624 / 2,733,880 / 5,400,251 at identical loop counts, a
25.13-fold max/min range. Its median candidate/upstream reference ratio 7.810
must not be used as a cache-causality claim. Cache-miss max/min spreads also
reach 78.7% for upstream comprehensions. These generic event names alone do
not establish a cache level, instruction site or memory access responsible
for a difference.

## Recording scope, identities and quality

`perf-record-6a0e4c12c17/` preserves six immediate permission failures before
the benchmark children. The successful isolated probe showed that user-only
callchains were required at the available permission level. The sole record
argv correction adds `--user-callchains`; it keeps `cycles:u`, frequency 499,
DWARF stack size 8192, delayed enable and FIFO control unchanged. It does not
request kernel callchains or broader permissions.

`perf-record-6a0e4c12c17-v2/` has all six successful role/kernel combinations,
with fixed loops targeting five seconds, no errors and unchanged executable
identities. Commands, raw data, child timing/acknowledgments, build-ID hits,
and offline analysis commands/stdout/stderr are retained. Recorded pre/post
load reaches 3.1363. Hot wall time is 4.842–5.189 seconds. All offline report
and script commands exit zero, with empty analysis stderr. Stats and script
sample counts reconcile; all six callgraph reports say zero lost samples,
and `--show-lost-events` scripts contain no lost-event records.

| Role / kernel | Samples | Unknown leaf symbol, weighted cycle period | Samples whose stack reaches `_start` |
| --- | ---: | ---: | ---: |
| Upstream deltablue | 2,591 | 1.079% | 2,572 |
| Candidate deltablue | 2,542 | 1.204% | 2,513 |
| Upstream logging | 2,412 | 0% | 2,396 |
| Candidate logging | 2,524 | 7.222% | 2,466 |
| Upstream comprehensions | 2,484 | 1.751% | 2,473 |
| Candidate comprehensions | 2,478 | 1.855% | 2,439 |

Unknown leaf symbols have known mapped modules; they are not lost samples.
The libc hit identity in all six recordings is
`066527e430a32768d82741e00b81eebb1a872294`. Select extension identities are
candidate `9251519adfa7fa73526d5f2c69610479c1167c1f` and upstream
`08964a4dcc555b09a9778f5f2ea6fa982b68426c`. Main executable build-ID hits
match the retained identities above. The five-entry `retained-modules.json`
manifest retains matching libc/select copies and reuses the main ELF copies.
Final `perf-preflight-6a0e4c12c17/postflight-after.json` has no errors and
`postflight-differences.json` is empty. Earlier SIGPROF captures did not retain
the libc identity; their opaque external bucket remains opaque.

The stack recovery is useful for C helper-to-current-handler ancestry. Reaching
`_start` is a quality check, not proof of complete recovery. Tail dispatch
removes preceding handlers, and compile-time inline entries are not additional
physical frames or Python caller stacks. Two upstream samples have fewer than
two distinct frame PCs, one in select and one in GC untracking; candidate
samples all have at least two. There is no measured percentage of logically
complete Python stacks.

Shares below sum sampled event **periods**, rather than counting samples.
Adaptive frequency recording gives samples unequal periods, especially during
initial control work. Inclusive ancestry shares overlap; inline and parent
percentages must not be added. Sampling skid and a single recording per case
limit site precision and stability. Record-run wall times are not replacement
benchmark scores. The initial `--stats` flat reports contain statistics only
on this perf; the separate `*-self.txt` reports omit that option and contain
symbol rows. Self-symbol reports group physical symbol bodies, including their
inlined work; the script's first inline name can therefore have a smaller share.

## Recommendations supported by the new callers

Candidate logging has 183 samples with unresolved libc leaf symbols, carrying 7.2215% of
sampled cycle period. Every one immediately unwinds into `_PyFrame_Initialize`.
Their ancestry divides into 3.3993% through `_PyFrame_PushOverlapping` and the
exact-argument call handler, and 3.8223% through
`eval_frame_push_and_init_code` and the general call path. This proves the
observed C callers for these samples. The retained libc ID matches its ELF
notes. Disassembly at `0x198400` broadcasts the byte in `esi`, branches on the
length in `rdx`, writes vector stores through `rdi`, and returns at `0x19843d`.
This is a memset-shaped routine with no resolved function symbol; objdump's
nearest `_dl_mcount_wrapper` label does not identify its function. The caller
evidence supports a bounded frame-initialization/slot-zeroing experiment on
both paths. It must preserve initialized-slot invariants, constant-slot ownership, error
cleanup, exposed/generator frames and reference-release ordering. Removing the
entire 7.22% leaf bucket would cap the idealized speedup at about 1.078 times;
one store cleanup cannot close the logging gap.

Deltablue inclusive ancestry attributes 26.4787% to generic `LOAD_ATTR`,
15.2659% to shape `LOAD_ATTR`, and 9.9914% to exact-argument calls. The generic
attribute branch reaches type lookup/cache admission; its completion also
reaches reader refresh. A bounded lookup/admission experiment deserves attention
alongside the existing shape publication/flush experiment. Preserve metaclass,
descriptor, type-version invalidation and callback behavior. Removing the whole
shape family would cap idealized speedup near 1.180 times; the proposed flush
deduplication concerns only a fraction of that family.

Comprehensions has 6.7586% inclusive `FOR_ITER_LIST` ancestry, including 0.8175%
in `PyList_GET_VALUE`. This supports the existing minimal internal borrowed
accessor experiment and matched compiler/handler-ABI comparison in A3 R1/Q2.
The getter bucket alone has a full-elimination ceiling near 1.008 times.
Preserve immediate ownership duplication, GIL requirements, exported interfaces
and the GCC non-tail-entry trampoline. These measurements do not authorize an
ABI redesign or removal of the trampoline.

Logging general-call ancestry is 30.2996%, exact calls 7.0507%, and reader
completion 3.5963%. These support separate generic binding/root-traffic and
publication/refresh experiments, rather than treating all calls as generic.
Keep allocation-failure ownership and roots valid before any callback, preserve
fresh kwargs behavior and exception state, and preserve reentrant frame/home
refresh. Each experiment should change one cause, retain correctness gates,
and compare normalized instructions plus elapsed time against its parent.
The detailed static constraints and falsification plans remain in
[steady-state-source-review.md](steady-state-source-review.md).

The recorded instruction excess supports concrete interpreter work before a
method-tracing redesign. None of these samples establishes boxing causality,
predicts parity, reopens held LRU/ctypes work, or changes the existing gate.

## Checkpoint report review

The added hardware sections of
[checkpoint-6a0e4c12c17.md](../../bench/results/checkpoint-6a0e4c12c17.md)
agree with the reviewed evidence: ratio direction, CPI, branch-miss rates,
sample counts, physical self versus inclusive ancestry, libc caller split,
cache variability, and optimistic speedup ceiling. The new self report also
supports comprehensions dictionary lookup at 5.92% physical-symbol self share.
Its ownership recommendations retain callback/root validity and distinguish
generic binding from exact overlapping frames. Its compiler proposal resumes
A3 R1/Q2, preserves the trampoline, and separates compiler and runtime changes.
No unsupported boxing or measured-speedup claim was found in those sections.
