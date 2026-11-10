# Cold architecture and evidence review: candidate 43

**Candidate 43 is API-gate rejected and must not be promoted.** Its two guarded-word unions violate the existing representation rule; the approved separate-storage repair needs a new source identity and qualification. Provisionally retain the two inline annotations in that successor, without claiming a general performance improvement. Logging and focused micros improve, but coroutine, spectral norm and comprehension regressions repeat. Reverting the annotations alone is not yet a demonstrated repair.

This review uses an optimizing-runtime perspective: specialize from durable facts, keep the ownership/observation boundary explicit, and judge complete hot paths rather than isolated dispatch or helper counts. It does not claim a V8 endorsement or import JavaScript object semantics into Python.

## Evidence audited

Evidence root `E`: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.

Audited completed `measure43-all29-{golden,upstream}`, `measure43-basic-golden39`, and all four long class/keyword micro campaigns. For each, pre/post snapshots match, errors are empty, and status records success/stable identity. Every timed raw stdout matches its journal metadata and seven samples; means, paired ratios and geometric means recompute. Roles alternate control-first/candidate-first, use identical loop counts within pairs, CPU 15, JIT off, and hash randomization off. Candidate source is clean `47dc23f48cf1470d4a98137beb5d313b5093033d`; controls are frozen 39 `4d666e1b482…` and upstream `3a440b65695…`.

| Completed elapsed comparison | Candidate/control | Qualification |
| --- | ---: | --- |
| All-29 / frozen 39 | 1.004676 | Two pairs per case; screening aggregate |
| All-29 / upstream | 1.191545 | Two pairs per case; substantial remaining gap |
| Logging silent / frozen 39 | 0.959975 | Six pairs; range 0.949314–0.969576 |
| Coroutines / frozen 39 | 1.067700 | Six pairs; range 1.032593–1.089639 |
| DeltaBlue / frozen 39 | 0.993731 | Six pairs; small difference |
| Generators / frozen 39 | 1.000976 | Six pairs; small difference |
| nbody / frozen 39 | 1.005488 | Six pairs; small difference |
| Long class load / frozen 39, upstream | 0.156627, 1.047186 | Six pairs each |
| Long keyword call / frozen 39, upstream | 0.604334, 1.070554 | Six pairs each |

The two micros remain markedly better than 39, but neither reaches measured upstream parity. The all-29 geometric mean must not erase individual regressions. The initial spectral norm, comprehension and logging-simple screening signals now have the independent six-pair results below. The class micro covers one repeated class integer attribute; it is not a broad attribute-lookup census.

### Secondary regression and micro-instruction follow-up

Audited `measure43-secondary-regressions-golden39`: 36 successful timed invocations, equal valid snapshots at the same 43/39 revisions, raw stdout matching all samples/metadata, three pairs in each order, and exactly recomputable paired summaries.

| Six-pair case / frozen39 | Geometric mean | Pair range | Reading |
| --- | ---: | ---: | --- |
| Spectral norm | 1.081747 | 1.056895–1.103310 | All six pairs slower |
| Comprehensions | 1.055996 | 1.045373–1.063960 | All six pairs slower |
| Logging simple | 1.027095 | 0.953031–1.102142 | Noisy; five slower, one faster |

Spectral norm and comprehensions are confirmed application regressions, not merely two-round warnings. Logging-simple evidence is less precise and should retain its range.

Audited `instructions43-micro`: equal valid snapshots, complete status, 28 successful perf finishes per role, 100% running coverage on every event, exact three-entry roster, four 200/600-loop pairs per entry, raw-record hashes, pair identifiers, medians and skeleton subtraction. These are whole-process differenced estimates per inner iteration. “Raw” means before empty-loop subtraction, not the whole-process count; “net” subtracts the same role's empty-loop median.

| Case | Upstream raw / net | Frozen39 raw / net | Candidate43 raw / net |
| --- | ---: | ---: | ---: |
| Class load | 144.752 / 63.904 | 657.188 / 565.191 | 168.999 / 77.002 |
| Keyword call | 719.800 / 638.952 | 1429.001 / 1337.004 | 1081.000 / 989.003 |

Empty-loop medians are 80.848 upstream, 91.996 frozen39 and 91.997 candidate43. Candidate/golden instruction ratios are class 0.257155 raw / 0.136240 net and keyword 0.756473 raw / 0.739715 net. Candidate/upstream ratios remain class **1.167510 raw / 1.204971 net**, keyword **1.501806 raw / 1.547851 net**. This confirms substantial removed work versus 39 and a larger remaining keyword instruction gap than its elapsed ratio suggests. It does not identify all that excess as repeated plan guards: binding, frame setup, retirement and loop work still contribute. Do not treat net instructions as direct opcode counts or pool them with elapsed samples.

### Coroutine instruction evidence

Audited `instructions43-coroutines`: equal clean snapshots, successful status, three roles, ten successful perf finishes per role, and all events reporting 100% running coverage. Recomputed all four low/high differences from retained whole-process counts and checked their invocation references and raw-record SHA-256 against each result. Low/high loop counts are 4/12; no skeleton is subtracted.

| Role | Median instructions per loop |
| --- | ---: |
| Upstream | 294,894,391.625 |
| Frozen 39 | 408,259,942.250 |
| Candidate 43 | 408,017,082.688 |

Thus 43/39 is **0.999405**, and 43/upstream is **1.383604**. Candidate and 39 differenced estimates are tightly grouped; upstream ranges about 293.68–296.11 million per loop, so do not overinterpret its final decimals. The host report's documentation-checkout commit/dirty metadata is distinct from the clean native source identity; the frozen input manifest is the relevant workload check.

Near-equal instructions plus slower elapsed against 39 narrows the investigation toward cost per executed work or environmental effects. It does **not** establish increased hardware CPI, instruction-cache misses, branch prediction trouble, or code-layout causality. This packet has no matched golden 39 coroutine cycle capture. Identical instruction counts can also hide different instruction mixes, branch paths, dependencies, or memory accesses.

### Candidate 43 PMU

Audited `gap43-perf-stat`: 18/18 valid, equal manifests, raw perf counts and runtime/coverage fields matching parsed records, valid child enable/disable acknowledgements and native metadata, and approximately 1.966–2.022-second hot intervals. All events report 100% running coverage. Three rounds alternate roles, giving a 2/1 order split; these are diagnostic counters, not a fully balanced elapsed campaign.

Recomputed ratios of per-role median counts per hot loop:

| Case, 43/upstream | Instructions | Cycles | CPI ratio |
| --- | ---: | ---: | ---: |
| Logging silent | 1.036363 | 0.872322 | 0.841715 |
| Coroutines | 1.378196 | 1.321449 | 0.958825 |
| DeltaBlue | 1.870776 | 1.661810 | 0.888300 |

These reinforce an executed-work problem relative to upstream in coroutines and DeltaBlue. They do not supply a golden 39 CPI baseline. The FIFO coroutine estimate differs modestly from whole-process differencing; the protocols have different interval and warmup boundaries and should not be pooled.

At review time `gap43-perf-record/status.json` reports 6/6 complete with stable identity. I have not decoded or independently audited those new profiles, so the architectural attribution below uses the separately reviewed 41 profiles and source. No 43 sample shares are asserted here.

`compiler-flags-census43.json` has equal recorded dictionaries across the three roles. Several entries are null or unexpanded Make variables. This supports equality of that census, not a claim that every resolved compiler/linker input was independently verified. Existing configuration/build identities remain useful controls; a second compiler or global flag experiment is not the next justified fix.

## Architecture judgment

The implementation's strongest feature is its intended owning frame/home contract. However, the guarded metadata unions fail the API policy; handler discrimination does not override that gate. See `guarded-cache-contract-review.md` for the approved independent-field repair and required capacity accounting. Lookup feedback stores canonical guarded metadata; handlers acquire the result before receiver retirement can invalidate the cache. Keyword binding uses one compact destination plan, validates mutable observations before effects, detaches overlapping owners once, and publishes the initialized child before retirement. The same-code defaults repair is necessary correctness work and must survive any performance revision. These are appropriate durable contracts, unlike retaining borrowed payloads in caches or creating another binder.

The two inline annotations also target one concrete frame-initialization boundary, with the project's debug policy preserved. Candidate 43's conditional 112-byte constant-fill leaf is a real remaining cost, and the +744 text bytes versus 41 and larger hot stack reservations are real tradeoffs. The compiled review additionally found collateral custom-getattribute code growth. “One helper” in source does not guarantee stable hot code as callers are added; compiled-path review was necessary.

The source diff from 41 remains just two declarations. Logging's repeated improvement versus 39 and the removal of the outlined unconditional constructor make keeping this policy for qualification reasonable. However, 43-versus 39 elapsed captures do not isolate the annotations from the entire object/call packet; 41 and 43 separate sessions are not a paired annotation ablation. There is no basis for claiming that all recovered time was native call overhead, or that the annotations caused or cured the coroutine gap.

No evidence supports a hidden-class rewrite, a universal call dispatcher, generalized tag changes, or another forced-inline cascade. Python already has useful shape/shared-key machinery; the next gains depend on actual site coverage, valid proofs and total transfer costs. The API gate has failed with two guarded-word union findings; full diagnostics are paused while the coordinator prepares the approved repair. Focused 273 release/debug passes are valuable but do not replace it.

## Five next priorities

1. **Repair storage and requalify.** Implement only the approved independent guarded fields, keeping the checker strict. Verify attr 40→48 and poly 88→120, offsets, the 4096-unit feedback limit, admitted-site counts and metadata bytes. Then run the API gate, builds and focused ownership/cache checks before retiming. This closes an actual policy failure; no new specialization or checker exception belongs in that patch.

2. **Locate the confirmed application regressions.** Use the corrected successor and retained39 control to obtain matched cycles/instructions for coroutines, spectral norm and comprehensions. Inspect the actual hot paths; do not substitute the earlier DIRECT-clear general-call ledger. Equal coroutine instruction counts do not establish CPI or layout causality. Gate: remove or explain the material regressions under the admission policy; keep logging-simple uncertainty explicit. Do not let the aggregate waive this work.

3. **Finish the bounded owning-generator entry proof and census.** The coroutine instruction gap versus upstream is about 38%, and its dynamic profile identifies exact calls and creation/completion. Count eligible calls and complete the existing pinned-code/shell/observation/owner proof. No further entry-shape expansion is recommended. Implementation remains subject to independent contract review. Gate: actual hot-provider hits, lower complete creation/call/completion work, paired application benefit, and unchanged behavior for unsupported prefixes and observations.

4. **Census redundant lookup work in DeltaBlue.** Its 43 PMU instruction ratio is 1.870776 versus upstream, while the class micro is only 1.167510 raw. Find the actual generic/type-object/method/shape sites, their misses and repeated proof work before choosing one bounded specialization. Gate: lower weighted application instructions with mutation, descriptors and lifetime behavior preserved. Do not project the class micro's gain onto all attributes or rewrite storage on profile shares alone.

5. **Audit trusted keyword-plan proofs before reducing guards.** The keyword micro still costs 1.501806 raw instructions versus upstream. Separate builder-proved immutable destination validity from mutable names/defaults/provider/observation checks, auditing every publisher and invalidation route. Then measure the complete binding/frame/retirement sequence to establish how much cost those checks actually represent. Gate: fewer instructions across 1–3-keyword shapes and fallback controls, preserved same-code/default mutation semantics, and paired benefit. The small plan capacity bounds any expected gain; it cannot explain the entire remaining gap by assumption.

The value-native cell bridge remains a lower-risk deferred option when counters show application exposure; current measurements do not rank it ahead of these gaps. SEND-recipient tags remain deferred until a full update-plus-delivery ledger and applicable workload establish net benefit. Neither is a substitute for the direct creation contract or the current gate repair.

## Decision boundary

Preserve 43 as a measured but API-rejected checkpoint. The approved split-storage successor must carry its own identity, gate results, capacity/coverage evidence and timings; it cannot inherit a green qualification from 43. Keep new optimization work separate from that repair. If unexplained material regressions survive, hold or reject promotion and isolate the performance change; do not let the aggregate or micro wins waive the requirement. If the inline policy is retained later, document both its removed unconditional boundaries and its remaining conditional constant-fill call and footprint cost.

This cold review performed source/document inspection and lightweight host-side parsing of retained evidence only. No runtime edits, builds, native benchmark workloads, or perf captures were executed.
