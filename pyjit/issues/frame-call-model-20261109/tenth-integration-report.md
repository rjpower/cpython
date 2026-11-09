# Tenth integration: direct completion decoding

Runtime `3f0ca44b16ac0ed2e4b8fb1d00af5e0c62b44369` replaces two general
metadata-decoder calls on a Python-generator return with direct reads of its
proved immutable SEND/FOR word. The frame ABI, physical continuation,
publication, observation and retirement contract are unchanged. The shared
completion body has the same 1,858-byte machine-code body in the reviewed
precommit builds. The wrapper shrinks from 343 to 340 bytes.

The [three-option review](generator-completion-and-creation-alternatives.md)
compared shared completion specialization, saved continuations and direct heap
frame binding. This first increment isolates decoding. Saved continuations
would add per-resume/header costs; direct heap binding needs a partial-frame GC
and call-result contract. Neither is implemented.

## Validation

Release and debug build 117 modules each and pass **575 tests across 11 modules**:
IR/contract, semantic completion, completion/payload/discard, frame calls,
monitoring, generators, coroutines and async generators. New checks cover the
full declared SEND/FOR geometry and cold/warm monitored generator completion,
including its actual instrumented handler and returned payload identity.

The first release run failed the new contract selector: the exported base-opcode
interface retains an instrumented opcode's identity. The corrected selector
follows declared instrumentation twins and retains all geometry/membership
assertions. Independent review approved the correction and final native gate.
The failed receipt remains separate. Committed identity builds check 117 modules
each. No schema, metadata, magic or generated-provider change.

Evidence under E11: `completion33-{release,debug}-build.log`,
`completion33-release-focus.log`, `completion33-v2-{release,debug}-focus.log`,
`completion33-committed-{release,debug}-identity.log` and
`completion33-assembly-review/`. See the
[independent source/result peer](completion-decoder-and-null-pre-independent-peer.md).
Full qualification remains open; these checks do not replace it.

## Basic performance

Unchanged kernels, GCC 15.2, JIT disabled, CPU 15, fixed hash seed, four balanced
fresh process pairs per workload and seven timed calls per process. Native,
configuration/module and before/after identity gates pass in both campaigns.
Elapsed ratios are current/control, lower is better.

| Workload | / Golden | / Upstream |
| --- | ---: | ---: |
| logging_silent | 0.649643 | **0.895730** |
| generators | **1.057876** | 1.473238 |
| coroutines | **1.050444** | 1.400457 |
| nbody | **1.070179** | 0.923726 |
| float | 1.006045 | — |
| unpack_sequence | 0.822628 | — |
| pickle | 0.999034 | — |
| deepcopy_memo | 0.982994 | — |

All four logging/upstream pairs lie between 0.889081 and 0.903801. This basic
check preserves logging parity; the two independent six-round timing gate was
established at32, not repeated in full here. Generators, coroutines and nbody
still lose to golden in every pair. Different campaigns are not a direct
before/after component comparison, so their differences do not establish an
attributable decoder speedup or nbody regression.

Evidence: `completion33-basic-{golden,upstream}/`, including individual
durations, commands and stable identities. The [ninth report](ninth-integration-report.md)
retains the full 29-workload screen, current32 counters and cycle profiles;
those counts have not been relabeled as33 measurements.

## Next admitted experiment

Both independent reviewers accept the
[shared completion specialization proposal](shared-completion-specialization-shape.md).
Use one C implementation, with literal SEND/FOR kind and canonical input on
proved VM returns; native/THROW providers retain general publication. Keep
the existing observer adapter, detach/publication order, held exceptions and
latest-PC reload. This adds no frame fields or per-resume continuation work.
Inspect summed compiled text/clones and spills before paired performance:
forced inlining can duplicate cold paths and is not a guaranteed gain.

The NULL-only PRE capability remains a separately reviewed correctness packet,
with a shared physical transfer query and pathwise forward facts/backward
requirements. Runtime changes there wait for this performance isolation.
Canonical list-child duplication is a further source-proof proposal, not an
implemented bypass. Broad repair, full release/debug qualification, current
stats and final review remain open. Golden stays immutable and selected.
