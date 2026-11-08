# Interpreter-parity checkpoint, 2026-11-08

The selected V8-review batch is integrated and passes combined correctness
qualification. Keep it for the measured logging improvement and small aggregate
improvement in the retained-ELF comparison, with the spectral regression
accepted explicitly. **Upstream parity is not established.** Logging remains
about 1.38× upstream; the broader 29-kernel screen is 1.2412× upstream.

The functional checkpoint is `0267d428808`; all suites and measurements below
identify that commit. Its successor `0765e9be47f` only moves an existing C
comment to the function it describes. Post-cleanup build/code-equivalence
verification is recorded separately below. Source, raw measurements and prior
failures remain retained. The previous [R1–R5 report](interpreter-architecture-20261107.md)
is historical evidence, not a directly interchangeable measurement protocol.

## Changes and design decisions

| Increment | Commit | Result |
| --- | --- | --- |
| P1/P2 | `0539e0c1ff9` | Typed code-home construction avoids generic numeric checks; successful refresh returns before duplicate error-path validation |
| M2b | `73c20472a9f` | Checked ordinary boolean continuation executes a live NOP without entering a separate handler; original word, PC and instrumentation remain |
| M1 | `d93b83800a5` | Exact-dict/inline-int first-probe identity hit returns an owner through shared result publication and ordered retirement; declines keep the generic lookup |
| P3 | `10627c339fa` | Guarded unbound Python calls retain pre-escape facts through a private entry sharing initialization, frame transfer and final post-allocation guards; generic fallback stays generic |
| Test integration | `1c8774797f3`, `0267d428808` | Update the independent terminal-marker scanner and keep an argument-taking helper out of automatic zero-argument discovery |

Terra workers implemented the bounded source changes; Luna synchronized four
design pages; independent Sol reviews checked contracts, actual diffs,
generated C, GCC output, composition and evidence. The coordinator built,
tested and committed integration. See the [eight-item decision ledger](../../issues/interpreter-parity-20261108/recommendations.md),
[cold architecture review](../../issues/interpreter-parity-20261108/final-architecture-cold-review.md)
and [evidence review](../../issues/interpreter-parity-20261108/final-evidence-review.md).
Not all eight ideas became code. Encoding/compiler rewrites lacked a concrete
case; native JIT recovery remains a separate stage. The borrowed-frame source
comparison changed the original ownership-deferral premise, as discussed below.

There is one current word layout and ownership model. M1 reuses the existing
publication and retirement machinery; P3 shares the existing call transaction
and frame stack. M1 publishes a temporary before input retirement, while a
folded local receives the result after successful retirement/refresh. The
ordinary NOP shortcut declines wrapped targets, debug lltrace and free-threaded
execution. Regeneration removes two operations from an unused legacy
optimizing-expansion table; a future optimizing frontend needs explicit
lowering before using that table. This does not change current baseline IR
metadata or add a native method/tracing backend.

## Correctness and identity

At clean `0267d428808`, release and debug each ran **54,394 tests with no
failures**: 2,531 release skips and 2,469 debug skips. Each build reports 117
modules; clean stats also reports 117 and passes its 142-test smoke. The API
lint covers 306 units with zero failing findings and zero parse errors;
advisory notes remain inventoried. Full suites ran at the coherent integration
point; individual increments used focused tests.

Retained failures matter: the initial stats smoke found seven independent-
scanner assertions caused by its missing guarded-call marker; the initial full
release suite found one argument-taking internal helper accidentally included
in zero-argument test discovery. Both were corrected, independently reviewed,
and followed by successful focused checks and the repeat combined suites.
Earlier fixture failures for line metadata and local-finalizer ordering were
checked against R5, corrected, and retained with their reviews.

Temporary route/hit counters are not production changes. M1's overlay was
retired byte-for-byte, source mtimes restored, and the stats header dependency
forcibly rebuilt so the diagnostic layout could not survive in stale objects.
All elapsed/profile captures used production release code with GCC 15.2.0,
-O3, frame pointers, mimalloc, no PGO/LTO, JIT disabled and native CPU15
checks. Upstream is `3a440b65695`; the retained pre-batch executable is R5
`0953757f4e7`. Startup RSS and a zero-box census are not admission gates.

## Elapsed results

Two fresh balanced six-round logging campaigns give candidate/upstream
geometric means **1.387177** and **1.378619**. Their paired ranges are
1.3652–1.4087 and 1.3446–1.4235. Every ratio exceeds the plan's 1.02 repeatability
limit, and both geometric means exceed 1.00. The parity criterion fails;
roughly another 28% reduction in candidate time is needed. The pinned R5
baseline was 1.657289 under the same target protocol, but captures from
different times alone cannot establish the batch's effect.

The separate balanced four-round retained-ELF diagnostic gives new/R5 logging
**0.840163** (about 16.0% less elapsed time), with paired ratios
0.841267/0.835551/0.845509/0.838359. Across 29 kernels its geometric mean is
**0.980981**, about 1.9% lower time. Both executables share the current rebuilt
extension modules and stdlib: this is a main-executable comparison in that
shared environment, not a pristine old-source rebuild or a claim of statistical
significance. The independent audit reconciled 432 final native children and
3,024 raw timings, balanced order, metadata, quiet-load limits and stable
source/build identities.

The broad upstream screen has two balanced rounds, seven values per child;
the table gives geometric means of paired process-mean ratios. Lower is better.
The direct R5 column has four balanced rounds. Its largest repeatable loss,
`spectral_norm`, is included rather than hidden in the aggregate.

| Workload | New / upstream | New / retained R5 |
| --- | ---: | ---: |
| richards | 1.3087 | 1.0171 |
| go | 1.2930 | 0.9895 |
| nbody | 0.8615 | 0.9664 |
| float | 0.7995 | 0.9534 |
| spectral_norm | 0.8408 | 1.0860 |
| raytrace | 1.0417 | 0.9798 |
| deltablue | 1.7409 | 1.0189 |
| chaos | 0.9868 | 0.9875 |
| nqueens | 1.2152 | 0.9974 |
| fannkuch | 1.2739 | 0.9970 |
| generators | 1.4082 | 0.9536 |
| comprehensions | 1.4140 | 0.9359 |
| coroutines | 1.3426 | 0.8972 |
| unpack_sequence | 1.4104 | 0.9828 |
| deepcopy | 1.4059 | 0.9985 |
| deepcopy_reduce | 1.3953 | 1.0000 |
| deepcopy_memo | 1.3309 | 0.9554 |
| json_dumps | 1.3651 | 1.0083 |
| json_loads | 1.2195 | 0.9887 |
| pickle | 1.1076 | 0.9950 |
| unpickle | 1.2249 | 0.9896 |
| regex_compile | 1.2138 | 0.9869 |
| logging_format | 1.3112 | 0.9706 |
| logging_silent | 1.4004 | 0.8402 |
| logging_simple | 1.3404 | 0.9803 |
| xml_etree_generate | 1.3195 | 0.9960 |
| xml_etree_iterparse | 1.3238 | 1.0087 |
| xml_etree_parse | 1.1579 | 1.0067 |
| xml_etree_process | 1.4421 | 0.9875 |

## Why the remaining gap is execution work

Warmed interval diagnostics reconcile **151 entered handlers and 20 frames per
logging loop**, versus R5's 161 handlers and the same 20 frames. The ten NOP
words still exist; ordinary continuations execute them without separate entry.
Nine result POPs remain. All ten dictionary reads hit the new first-probe path
in the 0/1,000/2,000-loop scaling probe. The earlier route diagnostic found ten
exact inner calls and ten outer overlap calls per loop, with no consuming-
binder or native-fallback delta. This target already uses the frame stack and
argument-window overlap; copying through the fallback binder is not its hot
cost. Both hot callees have zero constant homes, and normal cleanup is already
locals-only.

The recorded logging interval has zero inline numeric boxes; its interpreter
mortal counters are 90 increments and 100 decrements per loop. Ten aggregate
freelist allocations/frees are consistent with the ten fresh kwargs creations,
but are not dict-specific allocation provenance or an avoidable-time budget.
The remaining gap cannot be explained by counting startup boxes.

Gated perf stat uses three runs per event group and reports ratios of median
counts per unchanged workload loop. All 54 upstream/new runs pass, with 100%
event running. These are workload-operation comparisons, **not equivalent
bytecode counts** across the two interpreters.

| Workload | Instructions new / upstream | Cycles new / upstream |
| --- | ---: | ---: |
| comprehensions | 1.5091 | 1.3910 |
| deltablue | 1.9422 | 1.7393 |
| logging_silent | 1.4349 | 1.3839 |

For logging, upstream/new retire about **1,394/2,001 instructions per disabled
call** and use about **484/670 cycles per call** (ten calls per loop). New code
executes 43.5% more instructions and 52.7% more branches, while branch misses
are lower. Its cycles per instruction are slightly lower, so added retired
work is the main observed distinction; branch misprediction is not the leading
evidence. More instructions do not identify which source mechanism caused
each cycle. The stronger owning-home contract, generic value/tag handling,
call preparation and frame retirement remain concrete mechanisms to separate.

Candidate logging's sampled self shares are overlap call preparation 14.59%,
frame clear 11.41%, method lookup 8.71%, exact call 7.78%, return 5.59%, shape
lookup 4.71%, and frame clear/pop 4.13%. These locate work; they are not
independent removable savings. Some libc samples remain unresolved. All eight
DWARF captures report zero lost samples. `perf report` warns
`Invalid HEADER_EVENT_DESC`; retained event attributes still identify
`cycles:u` and the control `dummy:u`, and named executable symbols decode.
This descriptive-header warning is a profile metadata limitation, not a claim
that capture was warning-free; stat validation is independent of it.

## Spectral tradeoff

The direct spectral elapsed factor is **1.086037**, with all four rounds
above 1.08. Eighteen additional gated counter runs pass at 100% running:
new/R5 instructions **0.994914**, cycles **1.080979**. Thus increased cycles
per retired instruction, rather than added retired work, accompany the loss.
Warm IR identifies exact Python calls on the dominant `eval_A` path, bypassing
P3's guarded transaction. The paired profiles show no new dominant helper;
branch/cache data do not establish a cause. Code layout remains a hypothesis,
not a finding. Spectral is still 0.8408× upstream in the broad screen.

Keep the batch for this checkpoint with that explicit tradeoff. If spectral
becomes a gate, compare matched P1/P2 `0539e0c1ff9` versus retained R5 first,
then full `0267d428808` versus R5, using elapsed plus instructions/cycles to
locate the first worsening increment. The earlier per-increment screens did
not include spectral. See the [bounded regression review](../../issues/interpreter-parity-20261108/spectral-regression-review.md).

## Follow-up

The next high-value design investigation is borrowed arguments across frame
boundaries. Pinned upstream can preserve a nonowning argument in child locals,
protect overwritten caller locals, and promote on heap escape. The candidate's
current homes instead require their own mortal reference; retained message
loads take that owner. A new native probe confirms the real message and logger
are mortal and captures warmed upstream/candidate shapes. This corrects the
claim that an owning snapshot is an architectural necessity. It does not yet
provide an interval owner-cost budget or approve a representation change.

The [proposal](../../issues/interpreter-parity-20261108/borrowed-frame-views-proposal.md)
and [two-round review](../../issues/interpreter-parity-20261108/borrowed-frame-views-peer-review-round2.md)
approve diagnostics and a representation decision, **not implementation**.
Before implementation, resolve legal guard declines after view creation,
complete the consumer census, and specify exact private-tag promotion if that
variant is chosen. GC counted edges, anchors, frame exposure/copy, suspension,
native fallback and destructor order are mandatory design boundaries. Universal
eight-byte canonical heap values remain selected; the encoding winner is open.

Separately, count fresh-kwargs dict-specific freelist/tracer outcomes before
proposing a proof-preserving creation path. A freelist hit can invoke a reference
tracer, so it does not by itself preserve callable facts. Keep ordinary creation
and all final guards on decline. Preserve observer semantics before considering
return/POP continuation; defer a word-layout rewrite without a measured
operand/decode bottleneck. The [updated plan](../../issues/interpreter-parity-20261108/plan.md)
assigns gates and scope cuts.

## Evidence and successor cleanup

Raw commands, logs, identities, timings, counters, exact ELFs and perf data:
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-parity-20261108/`.
Main packets are `final-qualified-*`, `final-target-{1,2}`, `final-broad`,
`final-vs-r5`, `final-perf-*`, `spectral-perf-*`, `profile-analysis`,
`warmed-shape-comparison`, and the earlier increment/diagnostic receipts.

The comment-only successor `0765e9be47f` rebuilds release/debug/stats with
117 modules each; stats smoke again passes 142 tests. No suites or timings were
repeated for a comment move. Debug `.text` is byte-identical. Release differs
in four relocation bytes in two existing references to the same literal `f`:
the new build identifier ends in `f`, allowing linker suffix merging. Every
other release text byte is identical, including interpreter/call handlers.
The initially attempted whole-text equality assertion failed and is retained;
`comment-cleanup-release-text-diff.json` and `comment-cleanup-equivalence.json`
resolve it precisely. Do not describe all three rebuilt ELFs as byte-identical.
Full qualification and performance provenance remain at `0267d428808`, with
that exact release ELF retained for symbolization. The current source is the
clean comment-only successor, and current binaries identify that successor.
