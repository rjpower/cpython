# Backend elapsed decision

2026-11-01 06:11 UTC. **Reject Clang computed and Clang tail; retain GCC
computed for the remaining GC timebox.** This follows the selector declared
before scores, without changing the hard cutoff or parity test.

| Contrast | Coroutine round 0 / 1 | Four-workload GM round 0 / 1 |
| --- | ---: | ---: |
| Clang computed / GCC computed | 1.061794 / 1.056042 | 1.005749 / 0.971299 |
| Clang tail / Clang computed | 1.019815 / 1.016109 | 1.004842 / 1.031877 |
| Clang tail / GCC computed | 1.082834 / 1.073053 | 1.010618 / 1.002261 |

Every contrast fails the coroutine primary in both rounds. All primary
benefits are negative and fail the declared MAD/leave-one-out adequacy check.
Both tail contrasts also regress comprehensions by more than 2% in both
rounds. The second-round computed GM gain does not override the primary.
No extra samples, Clang upstream build or broad backend qualification follow.
These observations concern the complete compiler/backend packages; they do
not isolate a compiler mechanism or establish GC parity.

The coordinator independently recomputed all 120 positive raw values,
normalization, medians, spreads, RSS, all three contrasts and every primary
MAD/leave-one-out vector. All 36 exact command/environment records are serial
and within the declared window. All twelve raw origin traces were reparsed;
own standard library and extension evidence matches. All 72 quiet checks pass,
maximum load 0.682617, affinity CPU0. Current source/products/tools/ELF and 265
terminal bindings verify. Products match before/after/cleanup; all three images
are idle. The prior native correctness acceptance remains separate.

The first root audit stopped on comparing the parser's generic scope label
against the deliberately replaced witness label. Its script is preserved.
The corrected audit checks both labels explicitly and compares all underlying
identity fields. No target, helper or campaign artifact changed or reran.

Independent receipt: `build/vm0-elapsed-root-review-20261101-v1/receipt.json`,
SHA256 `ab4b2a347786790819e39b0b50a6c8e0bf99fb70d8b72360e6c1b2c5e44c77fe`.
Campaign: `build/vm0-nonstats-elapsed-v1`. The separate
[author report](../elapsed-author-outcome-20261101-v1.md) agrees with this
decision. Serial MAD/leave-one-out checks are descriptive, not confidence bounds.

The [fixed selection](../../gc-parity-deadline/compiler-selection-20261101.json)
retains the original matched GCC/O3 upstream protocol. No profile shopping or
integration authority is granted. Runtime source hypotheses can continue in
isolation under that fixed profile and the original deadline.
