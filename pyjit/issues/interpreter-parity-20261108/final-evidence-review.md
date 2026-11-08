# Independent final evidence review

2026-11-08. This is a read-only audit of the frozen elapsed captures. Native
execution, builds, and perf collection belong to the coordinator. Ratios below
are **candidate time / control time**: a value above one means the candidate is
slower. Hardware counters and sampled stacks remain under collection; this
document does not yet accept the final performance explanation.

## Elapsed captures

I independently parsed every raw stdout file, compared its JSON metadata and
seven nanosecond values with the corresponding commands.json row, recomputed
each row mean, every paired ratio, every case geometric mean, and the 29-case
geometric means. All match the retained summaries to floating-point precision.
Every command exited zero, every stderr file is empty, and all four status files
report no failure. The target driver records CPU affinity [15], JIT disabled,
and hash randomization zero for every child. The commands use taskset -c 15,
-S -B, and the same kernel and loop count for each pair. The target and broad
logging runs each use 60,000 loops. Recorded one-minute load averages remained
below 2.44 in the upstream comparisons.

| Capture | Raw rows; cases × rounds | First role by round | Result |
| --- | ---: | --- | ---: |
| final-target-1 | 72; 6 × 6 | upstream/candidate alternating; 3 each per case | logging_silent 1.3871766264 |
| final-target-2 | 12; 1 × 6 | upstream/candidate alternating; 3 each | logging_silent 1.3786187136 |
| final-broad | 116; 29 × 2 | upstream then candidate; 1 each per case | 29-case GM 1.2411582928 |
| final-vs-r5 | 232; 29 × 4 | R5/candidate alternating; 2 each per case | 29-case GM 0.9809811509 |

The two independent target campaigns reproduce a roughly 38% candidate
elapsed-time excess over pinned upstream. All six target ratios in each
campaign exceed the plan's 1.02 ceiling; both geometric means exceed its 1.00
criterion. Thus the elapsed parity criterion fails on the retained results.
The broad capture independently shows logging_silent at 1.4004144930 and
spectral_norm at 0.8407681740 versus upstream. It is a control, not a
substitute for the two six-round target campaigns.

Direct comparison with the retained R5 ELF gives logging_silent
0.8401633085, spectral_norm 1.0860365072, and 29-case GM 0.9809811509.
These are diagnostic deltas against R5, not a clean historical source
comparison: the R5 ELF shares the candidate's current rebuilt extension
libraries and stdlib. The capture itself records this limitation. Its 323
hashed files, including both ELFs, driver, and kernel files, have identical
before/after hashes; the rows identify the candidate as 0267d428808 and
the retained R5 ELF as 0953757f4e7.

## Identity and scope

All three upstream captures have byte-identical identity snapshots before and
after. Each snapshot has no preflight errors and 109 driver-source hashes.
The pinned upstream source is clean 3a440b65695615e9fd9912342b79cdd54fced0d2
with ELF build ID ff51c387c04ccd5a2664e5a572bc68e491ec0db1; the
candidate is clean 0267d428808aa25131bf228929bab6e53027df6a with
ELF build ID d70c9143464a2fb1efeb488c5609c502cc4e5caf.
Their module logs report respectively 116 and 117 built modules, with no
missing modules or failed imports. Both probes report the standard JIT and
natural-method admission API absent. Child sys._git values match these
sources. Each role and case was measured in both orders; the two-round broad
capture has only one observation per order, so its point estimates are less
stable than the six-round target captures.

## Gated counters and sampled stacks

I then audited the completed gated perf captures against their raw result,
child, and perf JSON files. The upstream comparison has 54 valid stat
invocations across three cases, two roles, three rounds and three event groups;
the retained-R5 spectral comparison has 18. All 144 individual event readings
have 100% running time. Every invocation has zero child and perf exit status,
an acknowledged enable/disable FIFO sequence, a warmed child, a nonzero hot
loop count, and native before/after checks for CPU 15, JIT off, hash zero,
natural-method API absent, and the expected source identity. One-minute load
at invocation boundaries remained below the collector's threshold. I
recomputed every event count per loop from raw count divided by hot loops,
then every three-round median and candidate/reference ratio; no mismatches
were found. All four stat/record identity snapshots are equal before and
after and match their retained symbolization ELF build IDs. In the spectral
capture, the collector's role label "upstream" denotes the retained R5 ELF,
with build ID 9d98a70694382c4650742915f0560f12346c85bf.

| Case and reference | Candidate/reference instructions per loop | Candidate/reference cycles per loop |
| --- | ---: | ---: |
| logging_silent / pinned upstream | 1.4348879165 | 1.3838607411 |
| deltablue / pinned upstream | 1.9421964563 | 1.7393283902 |
| comprehensions / pinned upstream | 1.5091187069 | 1.3909551708 |
| spectral_norm / retained R5 ELF | 0.9949142738 | 1.0809793938 |

These are medians of separate, roughly two-second gated hot regions. They
support that the logging elapsed gap accompanies more retired instructions
and cycles in the candidate. They do not isolate a particular owner operation
or prove a mechanism-specific speed effect. The spectral R5 result combines
near-equal instruction counts with higher candidate cycles; cache-event
readings are variable and do not establish a cause.

All six final and two spectral DWARF record invocations completed, and all
eight retained reports show about 2,000 decoded cycles:u samples with zero
lost samples and named Python ELF symbols. The input-stability check reports
401 files unchanged. Candidate logging self-cycle samples include
call_transaction_overlap_python (14.59%),
_PyFrame_ClearExceptCode (11.41%), _PyDict_TryGetFirstProbeValue (3.48%),
and PyDict_New (2.97%). These are sampled locations, not an additive
decomposition of the elapsed difference. Every perf report also prints
"Invalid HEADER_EVENT_DESC: nre=2 sz=144 (min 64)" and says its event
description header is unavailable. The saved perf evlist -v output identifies
cycles:u and dummy:u event attributes for every profile; reports still decode
samples and ELF symbols. The header warning is a metadata limitation and its
cause has not been established.

Four subsequent warmed-shape probes exited zero and retain both source
identities, CPU 15 and JIT off. They establish that logging MESSAGE and the
logger are mortal in both interpreters and that both warmed logging routes
contain LOAD_FAST_BORROW; the candidate retains an argument-load producer.
The reported MESSAGE refcount of two was measured outside the hot interval.
These probes support a next-packet borrowed-frame-view investigation, but do
not attribute a per-call owner count or change this batch's parity decision.

The elapsed and gated-counter evidence is internally consistent and
traceable to the frozen 0267d428808 source. It establishes failure of the
stated elapsed parity criterion and a real logging instruction/cycle gap.
The sampled profiles identify candidate locations worth investigating, with
the event-header metadata warning and retained-R5 shared-module limitation
above.

## Comment-only successor and final report

After all native captures, the coordinator committed 0765e9be47f. I reviewed
its actual diff: the same seven-line Run-a-call comment moved from before
call_transaction_init to before call_transaction_run, which it describes.
No executable source changed. Release, debug and stats each rebuilt with 117
modules, and stats passed its 142-test smoke. The saved text comparison finds
debug .text byte-identical. Release .text differs in exactly four bytes of
two existing references, in memoryview_cast and append_joinedstr, to the same
single-character literal "f"; the retained equivalence file identifies
linker suffix merging with the successor's build string. Other release .text
bytes are identical. This is a bounded code-equivalence check, not a claim
that the complete rebuilt ELFs are byte-identical. The qualification and
performance receipts remain attributable to 0267d428808 and its retained
release ELF.

I also checked the final results report against the retained summaries and
raw receipts. Its target ranges, four R5 logging ratios, 29-case table,
counter ratios, provenance caveats, spectral tradeoff and parity-failure
decision agree with this audit. No additional must-fix evidence finding
remains.
