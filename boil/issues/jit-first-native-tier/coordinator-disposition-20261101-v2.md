# Disposition of the word-selected JIT peer review

2026-11-01 09:29 UTC. **AMEND; design only.** The direct, medium-effort
[Fable review](claude-fable-5.1-word-selected-review-20261101.md) and its
[independent source audit](fable-amendment-source-audit-20261101-v2.md) inform
the [single seven-site candidate](word-selected-candidate-20261101-v2.md).
This grants no runtime implementation or measurement. RC qualification remains
open. Earlier designs, reviews and failed observations remain unchanged.

Reject the proposed reduced observer predicate. Local LINE events can instrument
later words while global/code versions remain zero and RESUME remains plain.
Keep actual global/local monitoring, trace/profile, tracing-depth, DTrace,
lltrace, PEP 523 and breaker checks, including checks after cold work. A
conservative decline on retained code-monitoring state is permitted; report
admission and decline counts rather than calling zero native entries a success.

Reject replacing all four temporary NULL checks with assertions at both hooks.
The shared start_frame entry also accepts publicly supplied existing frames;
the initial instruction pointer alone does not prove fresh initialization.
Keep the checked decline and both non-NULL argument guards. A future proposal
may prove narrower entry provenance, but that is not part of this candidate.

Accept a sentinel-first lookup and cached structural ineligibility. Establish
structural eligibility on the first candidate visit; count only eligible
entries toward the existing 64-entry compilation threshold. Transient observer,
argument or breaker declines must not install a structural-negative sentinel.
Measure the universal lookup cost on the fixed controls. There is no runtime
choice between entry policies.

Resolve tagged-value policy at descriptor construction only within the owning
interpreter/code-execution lifetime. Preserve compile-time inline-value support
and reject unsupported interpreter state. Keep the selected nonnegative
dividend/literal-two logical shift; the suggested broader arithmetic shift is
not adopted. The observed layout has a 104-byte fixed prefix and sizeof(frame)
112 including owned_slots[1]; use the existing slot-count helper throughout.

Keep scalar-only descriptors and leaf scratch. Appending exec fields requires
coherent allocation, initialization, alignment, static-exec, accounting and
failure cleanup changes. Teardown runs after names/constants decrefs, so it
must not consult borrowed metadata. Installed and partially built native owners
must be released exactly once; sentinel/static execs own no native mapping.

Before native execution, implement and check RET, 32-bit stores, immediate
arithmetic and shifts without accidental REX.W, bounded fixups and W-to-RX
publication. Preserve the C-contract checkpoint, then replace its leaf with
native code. Existing publication code is compiled away in this profile and
does not establish unwind/profiler support. The bounded experiment retains its
explicit coverage limit and later landing obligations.

The selected graph, seven source sites, six native arithmetic operations,
transactional two-store commit and finite tests remain unchanged. Actual
varying-input entry witnesses precede timing. Keep the frozen two-round elapsed
decision and controls; no additional samples, alternate region or smaller rescue
candidate follows from a loss. No estimate of implementation duration or native
frequency is accepted as evidence.
