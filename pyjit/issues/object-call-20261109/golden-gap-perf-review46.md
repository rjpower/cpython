# Golden/candidate PMU wrapper review

**Approve the requested diagnostic with explicit repeat directory, case list and full candidate revision.** This closes a meaningful evidence gap: matched frozen39/candidate46 counters for the confirmed application regressions. Source review only; no native work or helper edits were performed.

`capture-golden-gap-perf.py` changes the controller roles to golden/candidate, supplies exact expected heads and replaces its snapshot with the canonical checkpoint snapshot plus retained-executable, extension and input hashing. The original FIFO interval, grouped events, parser, invocation validation, child conditions, acknowledgement checks and record checks remain unchanged. `make_invocation` treats the role as a label; the unused original protocol main's upstream-specific argument parsing is not invoked.

The replacement snapshot requires error-free canonical build snapshots and exact equality with the matched timing campaign's build records. It verifies each clean full source head, retained ELF build ID and SHA-256 equality, then retains hashes of the executable and enumerated build extension DSOs. The inherited input census includes controller/protocol/checkpoint helpers, timing identities/status/commands, driver/benchmark sources, source Lib files and bytecode, build configuration and module logs; the wrapper adds itself. Pre/post snapshot equality therefore detects changed input content and changed DSO sets during capture. The module checks require 117 for both roles, with the existing canonical origin/release/JIT validation.

Configuration equality checks are the same selected fields as the earlier controller, not a new claim that every possible compiler input was proven identical. Extension hashes cover the build's `build/lib*/*.so` set, not every system DSO the process might load. The original child's native conditions still reject JIT/natural-method API, wrong CPU affinity or hash randomization, and record checks use the adapted role identity.

Two invocation details matter:

- Pass `--repeat` pointing at the completed, valid `measure46-all29-golden`. The wrapper inherits the older controller's upstream repeat default; leaving it unchanged cannot select the intended evidence. The retained repeat gate requires exactly two rounds of all29 kernels, unchanged input hashes and successful equal identities.
- Pass `--cases coroutines,spectral_norm,comprehensions`. The wrapper admits five cases and defaults to all five, which would schedule 30 stat invocations per event group. The explicit three-case list schedules the intended 18. Supply candidate head `b8e7e1bee6611eeda320ce163cd3ce89323da877` in full.

Neither default is a silent identity bypass: the wrong repeat should reject, and an expanded case list is recorded. Making `--repeat` mandatory or narrowing the default cases would improve ergonomics, but is not required for the explicit reviewed command and must not disturb a frozen measurement helper.

Run cycles/instructions and branches as separate preserved campaigns after current timing jobs finish. Three rounds alternate roles but yield a 2/1 order split; do not describe them as perfectly balanced elapsed trials. Loop counts are calibrated independently per role, so compare counts per hot loop, not raw event totals. Derive CPI only from cycles and instructions measured together; branch ratios do not by themselves establish misprediction as the cause of a timing gap. Preserve all statuses and reject incomplete or changed-identity campaigns.
