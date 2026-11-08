# Independent review of the revised runtime work breakdown

2026-11-01. **ACCEPT the revised work allocation and bounded host preparation.**
This is not native execution, runtime-source, performance, qualification or
integration authority. No target, compiler, workload or probe ran in this review.

Reviewed final document: `boil/design/runtime-work-breakdown.md`, SHA256
`47d0810b841fc7e07431832621f71033d3c2770b181e2e1fc55867119c0155fe`.
The initial reviewed draft was
`0725ccda0cd32179294f9d9c0bfb86cb12441e49ee19e18ae4977a02ccf822c9`.
Two requested prose corrections were applied before this disposition: the
counter-retention change is now correctly described as accepted and integrated,
and emitted-work verification no longer requires subjectively “simpler” native
paths as an additional performance veto. No blocking finding remains.

## Why the allocation is useful

Compiler/IR, interpreter/backend, native interfaces and collector work now
have named owners, concrete questions and different deciding evidence. This
corrects the GC-heavy ranking without pretending the tracks need equal effort
or can all execute native work concurrently. The shared baseline is a finite
attribution input, not a mandatory week-long all-runtime profiling project.
Independent source designs may proceed while that image is prepared.

Static operand provenance is a useful first compiler consumer with a complete
encoding/reader/rewriter obligation. It is not an accepted terminal IR design.
The plan retains the broader architectural question and permits a larger
coherent transformation when justified, rather than substituting a succession
of safe peepholes for the requested compiler/interpreter work. General SSA,
block materialization and a new generator still require concrete consumers.
The provisional comparison region remains held for actual selection evidence.

Backend controls are separate from operand cleanup. GCC-computed versus
Clang-computed identifies compiler-policy effects; Clang-computed versus
Clang-tail identifies the existing backend contrast. Neither proves tail is
mandatory, and a repeatable loss remains an elapsed result. Future execution
must receive a fresh exact scope; the old four-arm VM0 helper preparation is
not activated by this document.

The native track has an actual source-bound lead rather than a blanket boxing
explanation. The GC track asks for per-object lifecycle-state eligibility
before changing storage or omitting death records. The old-work track includes
complete mutation coverage and opaque-owner rescans, avoiding the inference
that a dummy card write costs the same as a functioning remembered set.

## What the shared stats image can establish

The six selected workloads and warmup/low/high protocol imply eighteen
registered counter processes. The retained driver `run` mode performs setup
and one `module.run(loops)` per such process; it is not the timed mode's
within-process warmup and score vector. Normalized differences retain startup,
setup, specialization and loop-subtraction limitations. They are not hot-only
observations, independent timing samples or release-speed measurements.

Full-domain entries and pairs can identify executed handler families and
candidate adjacent operations. They cannot identify a particular word site
without a closed attribution argument; same-word adaptive redispatch must be
separated. Native return-PC tables need the exact image/symbol mapping,
unresolved-PC disposition and the existing 64-site overflow limit. Conversion
attempts do not distinguish fresh allocations from cached-int reuse or measure
their elapsed share by themselves.

These counters do **not** provide actual per-type death counts, completed-coro
eligibility, survival, allocator slack, or cheap-deallocator time. The separate
observational constructor/classifier/sweep census remains necessary. No result
from the shared image can silently replace it. Missing or contradictory
attribution holds the affected hypothesis rather than authorizing a shortcut.

## Source, tooling and clocks

I independently compared source `417a9443a599e879c76c34c13ae76943bd6f6b3e`
with qualified e9 `e9e10d5b503eaf219797a5db44766eeab6fad4dc`: the Git path
diff outside `boil/` is empty. The existing managed VM worktree is clean at
417a9443. This is a content bridge for runtime/build/test source, not evidence
that an unbuilt stats image already has a valid configuration, roots, selected
tools, libraries or executing origins. Those remain admission requirements.
The changed host serializer must be pinned independently of that runtime bridge.

The accepted reporting change is integrated at `129131574a8`. Current
`boil/bench/run.py` and README hashes match the independent
[counter-retention review](counter-retention-review-20261101.md): respectively
`0a7771918e609486940440a1a339fb4ee644e7b6ef80a9ae134ae58c0aa94468`
and `d1c6cd7be3c974e7bb3af2861704be329ffcc0a4428e2fa9c44df176b452310a`.
That review accepts host retention/serialization and records seven mocked host
methods. It grants no native census or target-runtime adoption. I did not rerun
those tests or turn the plan's additional reported host checks into native
coverage.

The new host-preparation window is explicit, as is the proposed inclusive
45-minute build/focused/census exposure and subsequent independent outcome
review. They require actual manifests and a coordinator activation. Failed
source/tool/audit/child work stops without automatic rebuild or replacement.
The old 04:18:11 nursery-adoption deadline is not extended: its timing and
qualification are explicitly deferred for reassessment, without a fabricated
performance rejection. Its unmerged candidates acquire no foundation or landing
credit. Retiring the maintained private nursery option remains explicit debt
for the next selected collector adoption.

Before a mechanism's scores, freeze its own primary, controls, exposure, noise
handling and resource tradeoffs. Full adoption still requires all 29 elapsed
rows and absolute/upstream-relative RSS as well as parent-relative changes,
semantic/root/lifecycle checks and qualification. Neither static text growth
nor instruction count overrides elapsed by itself. A complete experiment that
cannot fit its granted time must defer or obtain a new explicit scope; this
design review supplies no extension.

Verification used the final document, the three Fable source audits, accepted
counter-retention review, actual serializer/driver source, exact Git content
comparison and current source status. Only this review was written; the root
document and all runtime sources were left untouched.
