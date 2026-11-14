# Comprehension hot-path fixes: results and selection

The four proposed performance changes were implemented, reviewed and tested.
Their composition removes real work, but it is **not selected**: comprehensions
improve while logging and coroutines regress. Two bounded revisions also lose.
The **dictionary-only subset is selected**, retaining its lifetime repair and
shared lookup layers while excluding method and list changes. It reduces the
observed all-29 geometric mean 0.44% versus golden56 and is 12.71% slower than
upstream in the two-pair aggregate. This is a correctness and abstraction
checkpoint, not comprehension parity or a substantial performance breakthrough.

The [plan](plan.md), [candidate design](../../design/proposals/comprehension-hot-paths57.md)
and [complete measurements](benchmark-tables.md) separate implementation from
selection. Golden56 remains immutable at
`883524c180e4486b47df77ee2276c9276c0ec32c`.

## Performance bundle

Measured candidate: `9ec291998f7395fedf21f456e5eb58c2187d2e23`, preserved on
`pyjit/comp57-qualified-initial`. Direct all-29 comparisons give
**1.001784/golden56** (15 wins, 14 losses) and **1.144639/upstream**
(seven wins). Ratios are elapsed candidate/control; below one is faster.

| Benchmark | Six-pair confirmation /golden56 | Direct all-29 /upstream |
| --- | ---: | ---: |
| Comprehensions | .948526 | 1.294292 |
| DeltaBlue | .966836 | 1.204904 |
| Generators | .993400 | 1.368729 |
| Coroutines | 1.046046 | 1.312683 |
| Richards | .997461 | .892258 |
| Silent logging | 1.131569 | .788098 |

Comprehensions and DeltaBlue improve in every confirmation pair. Logging,
coroutines and raytrace lose in every pair. Spectral_norm improves 11.29%.
The complete mean is essentially flat, with substantial repeatable losses in
previously improved workloads. This does not justify replacing golden56.
Separate campaigns are not pooled, and their ratios must not be multiplied to
infer changes against upstream. Ranges in the tables are observations, not
confidence intervals.

## What changed and what disappeared

Sol workers proposed the bounded changes, cross-reviewed ownership and effects,
implemented in separate branches and reviewed the actual diffs. Root generated
products, compiled, tested and measured the composition. Astra was not used in
the implementation loop.

| Change | Native evidence |
| --- | --- |
| Checked native method entry | The specialized descriptor instruction uses its existing admission guards to enter a narrow helper. Generic vectorcall redispatch and repeated descriptor checks disappear. Two call handlers shrink 1697→1254 and 1446→1199 bytes, with a new 306-byte helper. Recursion, object adapters, argument validation and ordinary completion remain. |
| Shared dictionary reader | Typed `dict.get` enters below general API validation. Lookup and result acquisition share an implementation, avoiding nested reader/pin layers. The found result is acquired before input cleanup; callback-capable lookup still pins its inputs. |
| Checked list iteration | The private checked storage read removes the trivial getter call, spills and generic numeric publication checks. FOR_ITER_LIST shrinks 1153→694 bytes. Representation facts remain separate from ownership, readability and scalar proof identity. Three kind/tag guard branches remain after partial CSE. |
| Spare-capacity append | LIST_APPEND and specialized append store directly when capacity permits, calling the consuming resize helper only when full. General header inlining also changes other C callers; handler size alone does not predict workload performance. |

The implementation found and fixed two integration compile errors, an opaque
preprocessor-directive hole in the producer validator, and an assertion using an
implicit argument-window read. These are closed findings, not suppressed checks.
The [candidate design](../../design/proposals/comprehension-hot-paths57.md)
defines the exact contracts; it is not the selected interpreter specification.

## Why the bundle is rejected

Matched counters confirm comprehension work removal, but do not explain away
the regressions:

| Workload | Instructions /golden56 | Cycles /golden56 |
| --- | ---: | ---: |
| Comprehensions | .952246 | .961340 |
| Silent logging | 1.000000 | 1.129093 |
| Coroutines | 1.000000 | 1.043247 |

Logging and coroutine regressions occur without additional dynamic instructions.
Earlier matching-head measurements also find essentially unchanged branch and
branch-miss counts. Logging profiles retain the same hot functions. Its steady
path does not execute the new list append, list iteration or checked native
method helper. Source and assembly establish removed work in the target paths;
they do not establish an exclusive cause for the unrelated cycle losses.

Code placement and dependency effects remain hypotheses. A six-event frontend
diagnostic failed on its first invocation with every counter not counted; it
provides no cache-specific evidence. Ordinary cycles/instructions captures and
six upstream/candidate profiles succeeded. Do not attribute the slowdown to an
instruction-cache mechanism without valid measurements.

Two additional experiments are closed:

| Experiment | Evidence and decision |
| --- | --- |
| Uniform 64-byte alignment of all interpreter entries, `cd0c48ea527` | Nine-case screen retains a flat mean and worsens logging to 1.192799/golden56. Reverted; branch `pyjit/comp57-alignment-trial` preserves it. No opcode-specific padding or seed search. |
| Restrict append inlining to the two interpreter bodies, `0748a6b8b36c609d94bd0e2f95ce6d134b00378c` | Restores the exported general C entry and returns 47 common function sizes to golden sizes. Builds and focused tests pass, but the screen gives comprehensions .927003, logging 1.211611, coroutines 1.032974 and Richards 1.036581. Rejected without another full-suite run. |

The second experiment changes only three generated append call targets. Its
interpreter fast paths remain intact; restoring a cleaner general C boundary
does not cure the performance problem. Smaller text and fewer instructions are
necessary evidence for these hypotheses, not sufficient selection criteria.

## Independent correctness issue

A cached exact-string hash does not imply callback-free dictionary lookup.
In a mixed GENERAL table, a stored custom key's equality can execute Python.
The original cached-string shortcut skipped the dictionary lifetime scope on
the false assumption that equality could not run Python. The general route
already provides that protection, and the fork already tests input-owner
retirement through its internal reader provider.

The isolated repair admits the shortcut only for a cached exact string and a
Unicode-only table under the GIL. Mixed tables retain the original scoped
route. Free-threaded code takes that route without a new unlocked table-kind
read. GetValue and ContainsValue share the admission rule. This closes the
existing stronger internal provider contract; the borrowed-holder regression
does not demonstrate a public-extension vulnerability. Ordinary callers still
owe their borrowed inputs a valid lifetime.

The minimal correctness-only candidate `72ee8afece7ce962172dc84ec32db995658e6ea0`
builds 117 modules/79 extensions and passes 18 focused methods per role. All29
is essentially flat at .999030/golden56 (19 wins), with 1.141682/upstream.
Nevertheless, six-pair confirmation loses DeltaBlue 6.94% and comprehensions
2.30%. A separate four-pair diagnostic reproduces 6.89% and 3.07% losses.
These are unresolved regressions; the flat broad mean does not erase them.

The DeltaBlue discrepancy comes mainly from golden running faster in the
focused campaign: roughly 136ms in all29 versus 125ms in confirmation, while
the candidate remains near 134ms. Workload, warmup, order and native identities
check out; both role orders lose in confirmation. Matched counters give
DeltaBlue instructions 1.000110 and cycles 1.078691/golden56; comprehensions
1.000213 and 1.027255. Nearly unchanged work does not establish a cache cause.
No further repeat or layout revision is used to choose a favorable result.

## Final dictionary-only subset

The last bounded candidate is `0dcba75bb3df40c32137db453b001f7e8f101de0`,
compared with golden56. Its full range changes only `Objects/dictobject.c` and
dictionary tests. It includes shared lookup/result acquisition and typed
`dict.get` entry as well as the lifetime repair. Removing nested reader and
query-pin layers is a distinct source-backed optimization; this subset had
previously been measured only alongside method changes. It is not a padding
or alignment variant. Method, list and generator changes remain excluded.

Both builds complete 117 modules/79 extensions. All 28 focused reader methods
and all 203 methods across three dictionary modules pass per role; these
counts overlap. No generated interpreter or representation protocol changes.
The full 55,045-test runs below belong to the rejected composition, not this
subset. Targeted qualification of the dictionary-only successor reuses the
established selected56 checkpoint and the already-reviewed dictionary source;
it makes no new full-suite or free-threaded claim.

Root selects this subset on `pyjit/round57-golden`, tag
`pyjit-golden-20261114-dictionary-readers`. Golden56 remains an immutable
comparison and rollback point. The direct all-29 results are
**.995586/golden56** (16 wins, 13 losses) and **1.127137/upstream** (six wins).
The separate six-pair hot-nine mean is .995750/golden56. No observations are
pooled and these small means are not claims of statistical significance.
Upstream XML iterparse has large opposing paired ratios (.685998–1.905215),
so the upstream aggregate is not a stable precise estimate of the remaining
gap. Retain both observations; no outlier is deleted or rerun into a win.

| Benchmark | Six-pair /golden56 | Observed range |
| --- | ---: | ---: |
| Comprehensions | .997484 | .974490–1.011757 |
| DeltaBlue | 1.000239 | .994835–1.010206 |
| Generators | 1.024797 | .992243–1.119177 |
| Coroutines | 1.019635 | 1.015298–1.028476 |
| Richards | .991167 | .985159–.999443 |
| Silent logging | .992049 | .946656–1.028672 |
| Raytrace | 1.020117 | 1.012356–1.027468 |
| Spectral_norm | .931732 | .929744–.934007 |
| Nqueens | .987729 | .982278–.995268 |

Comprehensions, DeltaBlue and logging are near flat with mixed pairs. Richards,
spectral_norm and nqueens improve in all six pairs. Root explicitly accepts the
coroutine and raytrace losses, which repeat in every pair, and the generator
mean loss with its wide range. They remain performance debt. Selection rests on
the lifetime repair, simpler shared provider boundary, absence of the other
candidates' large DeltaBlue/logging regressions and the small broad improvement.
It does not waive or conceal the losses, and it does not attribute every result
to fewer lookup layers. An independent cold reviewer checks the actual source,
executed qualification and selection in the [final review](review.md).

## Qualification of the rejected composition

Coherent full release at `3235a266f14` and debug at final `9ec` each execute
**55,045 tests**, with no crash. Release finds the six known debugger-jump errors
and one assertion-reader lint failure. The explicit-borrow assertion correction
passes its final release focused check; debug's only errors are the same six
baseline debugger cases. Verbose final checks reproduce their exact names in
both roles. Final release `.text` is byte-identical to the measured `70a`
composition; this is not a whole-ELF identity claim.

Both builds complete 117 modules/79 shared extensions. Regeneration, freezer
build, five bootstrap images and 27 compile probes pass. Only the existing GDB
`PrettyPrintTests` class is quarantined. There is no green full-suite or
free-threaded qualification claim.

The local-append trial separately passes 172 focused list/iterator/C-API methods
per role, its generator-contract method and assertion-reader lint. A failed
initial selector selected no tests; the corrected selector passes. Its losing
screen gives no reason to repeat the full composition gate.

## Remaining gap and next hypotheses

The selected dictionary-only checkpoint's matched three-pair counters show:

| Workload | Instructions /upstream | Cycles /upstream |
| --- | ---: | ---: |
| DeltaBlue | 1.361424 | 1.118423 |
| Comprehensions | 1.436416 | 1.333179 |
| Coroutines | 1.307831 | 1.267788 |

All 18 invocations are valid with stable source/native identities and
unmultiplexed counters. The selected runtime still executes **43.64% more
native instructions in comprehensions**. Native call validation, ownership,
generic completion and out-of-line list access/append remain. The rejected
bundle reduced the instruction ratio to 1.394208, but its broad elapsed losses
prevent selection. Register opcode count alone is not a useful cost model;
removing lookup layers does not remove the rest of each handler's work.

1. Combine list type/bounds/exhaustion/read/acquire in one checked native-pointer
   interval. Carry its proof through the existing generator facts to remove
   repeated object-kind tests; do not introduce a second ownership system.
2. Establish a shared iterator-pair protocol fact across actual producers,
   joins, observation and constructed images before removing cursor-tag checks.
   Assertions alone supply no release-mode proof.
3. Use the selected56 profile to evaluate one common resume/yield or exact-call
   operation at a time. Require actual native work removal and a broad sentinel
   screen; terminal completion alone cannot close the generator gap.

These are proposals, not implemented improvements. Close a losing hypothesis
after bounded revisions; do not hunt favorable padding or assume that an
unmeasured frame/ABI rewrite will solve the problem.

## Evidence

[HANDOFF](../../HANDOFF.md) defines BASE/E55/E57, exact worktrees and retained
ELFs. E57/final-measurement owns validated numbers, input hashes and tables;
final-all29-{golden56,upstream} each retain 116 timing commands,
final-confirm9-golden56 108, final-{perf,golden}-stat 18 each, and
final-perf-record six valid profiles. E57/qualification-summary.json records
the full-gate details and final repairs. E57/local-screen-golden and
local-append-native-review.md retain the rejected revision's data.

The selected subset's E57/dictionary-measurement/{numbers.json,table.md,
perf-numbers.json,perf-table.md} validates its separate 116/116/108 timing
commands and 18 upstream PMU invocations. E55/comp57-dictionary-* retains its
build/focused logs and native ELF. E57/correctness-measurement retains the
minimal repair's original campaigns and the separate 16-command/30-PMU
diagnostic. Neither source nor observations are substituted between candidates.

Comparisons use unchanged workloads, CPU15/hash0/JIT disabled and matched
GCC15.2/O3/devlibs. The new frozen golden56 build has matching configuration,
hot-handler sizes and addresses versus its retained original. Whole `.text`
differs in relative references and `.rodata` metadata; no whole-binary identity
or exclusive compiler/layout cause is claimed. Controls and sealed evidence
remain immutable.
