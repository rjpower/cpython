# Classmethod forwarding: second integration

Runtime `5559f60093e8216d9399bea74d0f10c8cdfb9a6b` is committed and pushed.
Production change `bfc9fe1aeff` forwards eligible classmethod lookups through the
existing owned function/class pair. The follow-up updates three pinned import
fixture lines for the intentional opcode-format revision. Handler 111 uses
magic 64418; cache geometry and ordinary call/frame ABI are unchanged.

The descriptor stays in guarded feedback. Each hit acquires its live exact
Python function before explicit argument evaluation, then transfers the actual
accessed class. Descriptor reinitialization, inheritance and later function
code/default mutation keep their normal Python meaning. Escaping attribute
retrieval and unsupported descriptors/metaclasses remain ordinary lookups.
The increment also corrects two raw readers' nonescaping effect declarations;
it does not add a handler exemption to the generator.

The [component review](A-post-review.md) and [cold review](cold-review48.md)
find no correctness/ownership blocker or required abstraction cleanup. Release
and debug build 117 modules. Initial release qualification passes 13 modules,
including all 12 classmethod tests, monitoring, profiling and disassembly; it
also reproduces the known 9 debugger-jump errors and finds two stale import
fixtures. After correcting those fixtures, release import/classmethod testing
passes 147 tests (13 skipped). Debug passes 14 modules, 867 tests (13 skipped).
The API check covers 306 units with zero failing findings or parse errors.
The failed first records remain retained; no full B+A suite is claimed.

Two balanced pairs provide a basic screen. Lower elapsed ratio is better:

| Case | /rebuilt 47 | /upstream, current build |
| --- | ---: | ---: |
| DeltaBlue |0.92971|1.19574|
| Logging silent |0.82760|0.78633|
| nbody |1.00870|not run in this upstream screen|

A separate fresh-build full29 screen gives geometric mean 1.18014/upstream,
DeltaBlue 1.20915 and logging 0.75865. Keep these build/campaign results separate.
The broad upstream gap remains about 18%; the selected target's gap is about 20%.
These are two-pair screens, not a parity claim or causal attribution of every
timing change to wrapper removal. All use GCC 15.2, CPU 15, hash 0, JIT off and
unchanged inputs; identities remain stable throughout each campaign.

A warm snapshot selects the new handler at 9 scoped sites; all 328 ATTR sites
remain private with unchanged entry allocation. This is route coverage, not
dynamic call frequency. The next structural experiment is conditional leaf
activation elision, including shared raw-read semantics and callback-free
aggregate retirement. It is isolated and unintegrated until post-review.

Evidence under `BASE/pyjit-link-wave1/deltablue-call-model-20261110`:
`delta48*` check journals/logs, `measure48-class-values47`, `measure48-upstream`,
`measure48-fresh-all29-upstream` and `coverage48-deltablue.json`. BASE is defined
in [HANDOFF](../../HANDOFF.md). Historical golden 39/upstream controls remain
unchanged. Task-owned frozen 46/47/48 builds provide intermediate comparisons.
