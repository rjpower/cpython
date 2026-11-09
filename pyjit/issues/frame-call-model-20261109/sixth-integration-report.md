# Sixth engineering integration: committed observer outcomes

Runtime `c1424be11db46b5d9e2aca84bcc96ddad6a7ea62` integrates terminal
observer publication and saved-local restoration. It is an engineering
checkpoint, not a qualified replacement for immutable golden `0765e9be47f`.

## Measured throughput

GCC 15.2, JIT disabled, CPU 15, fixed hash seed, unchanged workloads, balanced
process order and seven timed durations per process. All campaigns have stable
pre/post source, executable, module and driver identities. Ratios are
candidate/control elapsed time; lower is better. Each row is a separate campaign.

| Workload | Control | Paired rounds | Geometric mean | Pair range |
| --- | --- | --- | --- | --- |
| logging_silent | upstream | 2 | **0.936868** | 0.913258–0.961087 |
| logging_silent | golden | 2 | **0.653035** | 0.639137–0.667235 |
| nbody | golden | 2 | 1.041296 | 1.037355–1.045252 |
| pickle | golden | 2 | 1.026258 | 1.025427–1.027090 |
| nbody repeat | golden | 4 | 1.019904 | 0.983848–1.048559 |
| pickle repeat | golden | 4 | 1.021510 | 1.008689–1.038471 |

Logging is faster than upstream in this short screen. Final parity still
requires two independent six-round campaigns after coherent qualification.
Nbody and pickle remain follow-up costs. The prior 4a screen was faster and its
nbody ratio was flat; that different campaign does not establish a causal cost
for this packet or disprove current added native work.

Evidence under `build/pyjit-link-wave1/frame-call-model-20261109/` in the
python-boil-the-ocean checkout: `sixth-engineering-{upstream,golden}/` and
`sixth-repeat-golden/`. Complete pair lists and raw durations are retained.

## Current hardware work and profiles

The retained FIFO protocol enables counters only for the warmed workload region.
Every event runs 100% of its requested interval. Logging has 18 valid stat
invocations; nbody has 12. Ratios below use the mean per-workload-iteration
counts across three balanced captures, rather than bytecode counts.

| Workload/control | Cycles | Instructions | Branches | Branch misses |
| --- | --- | --- | --- | --- |
| logging/upstream | **0.918210** | 1.044970 | 1.103195 | 0.996963 |
| nbody/golden | **1.040876** | 1.033309 | 1.058609 | 1.040206 |

Logging now retires about 4.5% more instructions while spending about 8% fewer
cycles than upstream. The old approximately 40% instruction excess is historical
evidence, not the current result. Nbody's cycle excess mostly accompanies added
instructions; cycles per instruction rise about 0.7%. The earlier handler census
showed equal material routes but belongs to an older image and is not a current
native-cost attribution.

Logging's flat cycle samples concentrate in method lookup (two specialized
handlers, about 23%), exact/general Python call handlers (about 22%), frame
clearing/retirement (about 16%) and return (about 8%). These identify locations;
they do not quantify the gain from optimizing each location. Both logging
profiles report zero lost samples.
Use the retained native profiles and actual source/disassembly before changing
hot paths. Generic cache-reference ratios have very small upstream absolute
counts and do not identify an instruction-cache bottleneck.

Evidence: `sixth-{logging,nbody}-perf-{stat,record}/`, their summary JSON files,
and `sixth-perf-identities/`. All 401 logging protocol/workload/module/executable
input hashes remain unchanged after capture. The nbody driver independently
checks clean candidate/control source and live/retained executable identities.

## Integrated model and validation

Terminal generated providers publish final homes and their real continuation
before observation. Deferred values remain explicit C owners; callback errors
survive their retirement, and later redirects win. Activation-owned observer
scopes identify actual frame/site/phase/edge/generation without adding ordinary
dispatch work. One generated outcome declaration feeds the existing constructor
ownership worklist; physical fresh values and old-owner provenance remain
separate. Saved-local restoration uses explicit source-home actions, preserves
genuine unbound bindings, and refuses unchecked target reads of those bindings.

Release/debug builds each check 117 modules. Debug passes 363 tests across six
core modules, with one skip; each profile passes 64 further observer/frame
tests. Twenty-two selected release constructor/diagnostic checks pass. Eventual
ownership probes leave zero survivors from 660 values and fix the stranded FOR
result on callback error. The constructor accepts 2,156 valid library files;
11 intentionally invalid files retain their SyntaxErrors. Makefile dependencies
now include the four previously omitted IR headers; a stale-object retry remains
recorded as failed evidence.

The 999-test release integration diagnostic still fails four modules:
monitoring (one error), peepholer (three), sys_settrace (38), and pdb (two failures
and two errors), with 14 skips overall. The 21 obsolete diagnostic mismatches
are gone. Full release/debug, current stats and API lint remain open. See the
[repair log](qualification-repairs.md), [source review](native29-final-source-review.md)
and [result review](native29-cold-source-review.md).

## Next work follows actual failures

The [provider/stopped-state review](next-provider-stopped-state-peer-review.md)
separates reported event, actual owner edge, physical continuation and normalized
source landing. The producer increment admits explicit edge facts, terminal
jumps/iterator retirement and positive source-publication authority. Legacy
uncommitted hooks retain callbacks without authorizing a transfer. Yield
cancellation needs a separate typed pre-suspension barrier.

The proposed exact-edge stopped-state view remains under investigation. The
new native compile inventory finds **zero source-only records in all six
unchanged each-LINE comprehension fixtures**. Those failures cannot be assigned
to SOURCE_POST from their ordinal alone. Inspect their actual callback PCs,
ownership and target bindings before admitting a consumer change. The shared
Analysis/Plan remains the intended mechanism; do not create opcode-specific
setter branches or widen authority to pass a fixture.

A subsequent 84-row bounded cold/warm probe confirms the refused list/set/dict
ordinals 2–4 stop at the actual backward-jump instruction. Legacy tracing emits
a Python LINE callback while the active monitoring event is still JUMP.
The amended first consumer uses the producer's positively declared, owner-neutral
committed edge and the existing restoration Plan. It is independently reviewed
and admitted for implementation; source-only and yield authority stay separate.
See the [actual-JUMP amendment](stopped-state-actual-jump-amendment.md).

After the next coherent integration: focused source/observer/constructor gates,
basic performance, then full qualification, the unchanged 29-workload screen,
repeats of material losses, final logging campaigns and independent cold review.
The [active plan](plan.md) and canonical [IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md), [frame ABI](../../design/call-and-frame-lifetimes.md)
and [debugger](../../design/debugger-frame-transfers.md) specifications own the
current model. Golden remains selected.
