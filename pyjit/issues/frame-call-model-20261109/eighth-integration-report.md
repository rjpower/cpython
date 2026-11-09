# Eighth integration: canonical local owners

Runtime `3cde040a1afdc433db274ff3771dabff08d7b777` is integrated and
independently reviewed. It is an engineering candidate, not a qualified or
promoted checkpoint. Golden remains unchanged. The preceding
[seventh report](seventh-integration-report.md) records the committed-jump model
and its broader focused checks.

## Implementation and focused evidence

Pure local loads already read canonical PyValues. Their owned duplicate now
uses the shared canonical final store rather than repeating numeric publication
checks. The source recognizer proves the exact local read/duplicate body,
immediate/local-index relationship, single fresh output and absence of callbacks,
named caches, helper shadowing or result mutation. All affected generator uses
share the renamed canonical-result fact; there is no alternate emitter.

Regeneration changes only LOAD_FAST and LOAD_FAST_BORROW final-store selection.
The other 285 generated bodies, four numeric-constructor bodies and all metadata
are unchanged. Local, temporary and discarded destinations keep the existing
ownership and displacement protocol. No borrowed frame edge is introduced.

Host checks pass four selected methods. Release/debug each pass 25 focused
tests, including local replacement across callbacks, mortal payload identity,
eventual collection, numeric/subclass behavior, operand modes and discarded
results. Both committed builds check 117 modules. The
[independent source/result review](canonical-home-post-review.md) approves this
bounded increment. Static GCC disassembly shrinks each load handler from 134 to
85 decoded instructions; that is code evidence, not a dynamic speedup claim.

## Isolated component result

Four balanced pairs compare the retained clean `417a8bd6e2e3` release executable
with clean `3cde040a1afd`, in the same build directory. GCC 15.2, JIT disabled,
CPU 15, hash seed 0, unchanged kernels/loop counts, one warmup and seven timed
calls per child. Both probes check 117 modules. All 213 physical extension paths
and hashes, configuration, source, imported inputs and pre/post identities are
stable. Historical ABI files in that set are not additional checked modules.

| Workload | After/before ratio of round medians | Paired range |
| --- | --- | --- |
| nbody | **0.970773** | 0.949685–0.996600 |
| logging_silent | **1.034605** | 1.030460–1.042455 |
| pickle | 1.011179 | 0.987392–1.031217 |

Nbody improves in all four pairs; logging regresses in all four. Pickle is mixed.
This is a measured tradeoff, not a general win. These ratios cannot be combined
with earlier upstream campaigns to claim current parity. No causal explanation
for the logging regression is established by static assembly or old profiles.

Evidence under `build/pyjit-link-wave1/frame-call-model-20261109/`:
`canonical31-component-v2/{summary,status,identity-before,identity-after}.json`,
raw child streams and command journal. The first `canonical31-component`
preflight rejected mismatched lexical/resolved extension symlink keys before
timing. The independently reviewed correction retains exact path/hash equality;
the failed attempt remains preserved.

## Current qualification and next work

A full release diagnostic is running at the clean integrated source. It will
replace the old native87 inventory and determine which remaining failures need
runtime repair versus source-map/display consumer updates. It is not yet a pass.
Debug full qualification, current stats/API checks, the unchanged broad screen,
material-loss repeats, current PMU and both six-round logging/upstream campaigns
remain open.

The next reviewed model work shares exact committed-edge frame views and typed
incoming NULL requirements. It separates physical availability from old-owner
provenance, transports COPY/SWAP facts simultaneously and keeps unsupported
states conservative. Real monitoring/peepholer witnesses drive the initial
scope. Retained context roots, caught exits and pre-suspension yield require
their own semantic contracts. No per-event setter exceptions or weaker positive
test assertions are authorized. Logging parity remains the active goal.
