# Qualification repair log

These increments repair the third engineering integration's full-diagnostic
failures. They do not supersede its measured performance or qualify the whole
candidate. Golden and upstream controls remain unchanged.

Current integrated repair source is `192cfbe8aa0`. All three build identifiers
match this clean source; stats smoke passes 146 tests. Native26's wider diagnostic
still has unsupported transfer cases. The native27 LINE repair passes its focused
gates. The fourth engineering measurements below are screens, not qualification.

## Compact source and consumer repair: 37a67b9d155

Fused compare/contains/identity branches produce source-edge metadata using the
actual conditional control operation, matching encoding. The emitted word keeps
its executable offset. Both source-only successors and callback exception
handlers are preserved without adding executable words. Cache magic 64416
invalidates old compiler-produced metadata; opcode format/digest is unchanged.

An exact real-LINE identity request keeps all frame owners and handled exception
state intact. Admission requires the first selected real target to equal the
actual current LINE wrapper, the actual observed frame and no source-only,
error, pending-child or suspended phase. It commits redirect generation even
when the PC is unchanged. This does not admit general protected jumps.

Static disassembly/location/layout fixtures describe compact semantic
FOR/SEND/THROW execution and current frame fields. The frontend CFG fixture
retains a source carrier; final register code emits real operations only.
Frozen-main embedding includes an absolute build-root generated header and
depends on the configured build Python, preventing VPATH or source-neighbor
selection of stale marshalled code. Existing behavior assertions are retained.

Independent proposal/actual-source/result reviews are recorded in
[source24 peer review](source24-peer-review.md). Root reviewed the final diff.

Executed evidence under the packet's build evidence directory:

- `ownership24-{release,debug}-build.log`: both builds check 117 modules,
  no import failures; both regenerate the build-root frozen-main header.
- `ownership24-{release,debug}-focus.log`: 1,027 tests each across test_code,
  test_ir_source_reentry, test_patma, test_dis, test_sys, test_exceptions,
  test_embed and test_import. All pass; release skips 33, debug skips 31.
  The original pattern-matching expectation and frozen embedding test pass.
- `ownership24-construction-*`: 2,152 valid library files compile without
  rejection; 11 intentionally invalid parser fixtures raise SyntaxError.
- The earlier static-fixture check used the old 87 binary and is retained as
  `ownership24-fixtures-release-focus.*`; it is not the merged native gate.

The merged binaries were built before commit from 87 plus the full reviewed
37 diff. Their identifier still names 87; strict measurements require the next
identity rebuild. No full suite or performance campaign is claimed for 37.

## Next repair contracts

The [private debugger-entry interface](debugger-entry-interface-proposal.md)
is peer/root admitted for implementation. It enters synchronously with an
explicit observer phase, strict active ancestry and restored observer/tracing
state. Bdb uses a dedicated entry hook; existing pending-call owner proof
separately governs jump commands. No instruction event is fabricated.

The [derived context schema](debugger-context-schema-draft.md) is the reviewed
direction for protected jumps, independent of physical owner availability.
The exact implementation still needs admission. Same caught context preserves
its dynamic owner generation. Leaving a caught context must resolve the target's
actual exception-restoration link, including callback-owned exceptions, before
any mutation. General suspension and comprehension restoration remain open.

Completion-event policy, remote/GDB active source display, broad performance
repair and final qualification remain required by the [plan](plan.md).

## Explicit entry engineering25: consumer repair in progress

The private entry implementation and its dedicated Bdb dispatch pass source
peer review. Root's per-file Clinic regeneration succeeds, and both builds
check 117 modules. Seven new entry tests pass on each profile, covering both
backends, commands, ancestry, deferred ignored-call redirects and used-result
refusal, exception context and callback failure/quit.

The wider eight-module gate runs 517 tests on each profile. Six modules pass:
bdb, remote_pdb, source_reentry, code, frame_calls and semantic_completion.
Pdb reports twelve failures and four errors; doctest reports four failures.
The logs are `ownership25-observer-release-focus.log` and
`ownership25-observer-debug-debug-focus.log`. These failures remain evidence,
not a qualified landing. The core entry change is still uncommitted.

Concrete consumer gaps are doctest redirection that intercepted only ordinary
trace dispatch; async Pdb initialization that assumes deferred entry and later
fabricates an instruction callback; and quit cleanup after observer suppression
has ended, which can trace the cleanup itself. Shared setup and explicit entry
must be separate, with error cleanup inside the suppressed transaction.

`ownership25-*-original-class-init.*` faithfully reuses the original fixture
and tracer. Both candidate builds fail the opcode variant; matched upstream
passes both variants. Every-event opcode enablement changes instrumentation
during a source-only LINE and invalidates the landing epoch, delivering another
LINE. Narrowing invalidation requires proof about actual LINE configuration,
including callback replacement and event restart, rather than equal masks alone.

`ownership25-*-completion-payload.*` records fifty ordinary-source cases on
release, debug and upstream. Candidate completion observation unpacks tuple
payloads and reuses returned StopIteration instances. Matched upstream shares
the bug on modern/SEND routes, while its legacy FOR route preserves payloads.
The actual SEND result retains identity on both. The admitted repair uses the
existing value-native generator exception constructor; the legacy event mapping
remains separately under review. Python value semantics drive this repair.

## Context, entry consumers and completion: 9f677103e00

The complete source/result-reviewed batch integrates explicit entry and all
affected stdlib consumers; inherited and bounded same-caught transfer plans;
one canonical EH decoder; corrected completion payload construction; and a
legacy STOP adapter that omits only implicit valueless FOR completion. Modern
STOP(None), SEND(None), real iterator RAISE and non-None payloads remain visible.
Two tools, DISABLE/restart, callback mutation, errors and nested reentry are
covered. Existing debugger/trace transcripts remain unchanged.

The request-owned cold analysis indexes source records and sparse forward/reverse
normal/error edges once. Previous exception origins are separate from physical
incoming requirements. A preserve bitmap feeds one detach/commit/close path;
no exception-link writes are introduced. The next cancellation/restoration model
must distinguish disposable expression owners from comprehension/with roots.

Executed native26 evidence:

- Both builds check 117 modules; six focused modules pass 131 tests each,
  with one skip. This includes the new protected-context, payload and entry cases.
- Fifty ordinary-source completion probe rows on each build have zero payload
  identity mismatches, compared with 24 rows before the repair.
- The wide gate runs 1,572 tests per profile. Four actual modules still fail:
  monitoring, pdb, peepholer and sys_settrace. Trace has 65 errors, 21 diagnostic
  mismatches and one pending LINE-epoch duplicate; Pdb has six failures and two
  errors across three repeated backend cases. Full qualification is still open.
- Root also selected obsolete `test_cprofile`; retain its import failure as a
  harness error. Correct `test_profiling.test_tracing_profiler` passes all
  28 tests in both builds. The wrong selector is not a runtime regression.
- The compile-only corpus accepts 2,154 valid files without rejection; eleven
  intentionally invalid parser fixtures remain SyntaxError. The two new modules
  account for the increase from 2,152.

Logs use `ownership26-{small,wide,profiling-corrected,...}` prefixes. The wide
driver continued through both profiles despite expected failed modules. Native26
ELFs were built from 37 plus the complete reviewed 26 diff and identify 37;
native27 refreshes the build identifier. No full qualification or performance
promotion is claimed for this cold repair.

Independent source/result review is in
[context post-review](context-source26-post-review.md), the
[entry source review](debugger-entry-source-review.md), and the proposal peers.
The [canonical debugger design](../../design/debugger-frame-transfers.md) records
the implemented model and its limits.

## LINE configuration epochs: 192cfbe8aa0

LINE handoffs are invalidated by actual LINE mask changes, reused LINE tool
identity or restart. Enabling INSTRUCTION alone does not replay a pending LINE.
Stale tool versions are excluded from effective local masks; a reused tool resets
its real-word and source-record DISABLE state. Callback replacement retains the
existing API policy: it changes subsequent delivery without implicitly restarting
disabled events. No ordinary-dispatch field or work is added.

`ownership27-*` checks 117 modules and passes 21 related plus 51 existing tests
per profile. The unchanged class-initialization fixture passes both its base and
opcode variants in release/debug. Independent review is recorded in
[LINE post-review](line-epoch-source27-post-review.md). The subsequent
`fourth-engineering-{release,debug}-identity.log` and stats build/smoke establish
the committed identity on all three builds; stats smoke passes 146 tests.

## Fourth engineering performance screen

Clean `192cfbe8aa0`, GCC 15.2, JIT disabled, CPU 15; unchanged workloads and
immutable controls. Ratios are candidate/control elapsed time, lower is better.

| Campaign | Comparison | Result | Limit |
| --- | --- | --- | --- |
| `fourth-engineering-upstream` | logging / upstream | 0.817460 | Two balanced pairs; range 0.798429–0.836945 |
| `fourth-engineering-golden` | logging / golden | 0.591506 | Two balanced pairs |
| same | nbody / golden | 1.012647 | Two balanced pairs |
| same | pickle / golden | 1.044816 | Two balanced pairs |
| `fourth-engineering-golden-repeat` | logging / golden | 0.592805 | Four balanced pairs; range 0.590244–0.595484 |
| same | nbody / golden | 1.000054 | Range 0.991650–1.021431 |
| same | pickle / golden | 1.017928 | Range 0.997762–1.039830 |

Nbody's earlier 5.6% loss does not repeat on this image. Pickle's first 4.5% loss
also shrinks and varies across pairs; retain it for the full screen rather than
claiming a resolved component cost. Cold correctness changes do not establish a
causal performance improvement. Do not combine ratios from different controls
or campaigns. Final full suites, broad screen and two independent six-round
logging/upstream campaigns remain open.

The first existing-counter nbody census is retained as `fourth-nbody-census`.
Its setup helper still specializes during capture: `run(20)` warms advance but
calls setup once. The corrected driver warms twenty separate `run(1)` calls;
its fresh capture is `fourth-nbody-census-v2`. Census counts are not elapsed or
native-instruction measurements. Adaptive POWER retry counts must be reported
separately when 0/N/2N subtraction is nonlinear.

The corrected census completes with stable routes before/after every sample and
stable source/ELF/driver/kernel identities. Both roles retain nonlinear POWER
retry counts; all other material handler counts match except removed POP_ITER
40,005 and POP_TOP 2 per outer loop. Totals are 17,600,702 vs 17,560,695. This
does not explain native cost by itself. The actual thirteen-case compile-only
cancellation inventory also succeeds under 192, with no fixture function invoked:
`ownership28-cancellation-inventory/` records its ELF, driver, command and output.

## Per-local PRE cancellation and LINE identity: 4a1f70fcef4

Integrated after independent source/result review: per-distinct-local save
obligations, bounded real-LINE PRE cancellation and actual-current-operation
identity. Independent locals restore in different normal/EH orders; only nested
saves of one local are ordered. Lost saved origins remain obligations.

Retained native28 attempts include the rejected global-order proof, two source-map
fixture mistakes and the first wide check's same-line with-entry loop. The latter
has a captured faulthandler diagnostic; root stopped only its own worker. The
fixed bounded probe has16 rows per profile with no caps/errors and unchanged
current PC on identity. See `ownership28-running-line-after/`.

`ownership28-identity-small-*` passes125 tests per profile with one skip.
`ownership28-model-port-and-geometry` separately passes the two identity and
101-argument call tests. Its first geometry driver failed a mistaken near-distance
assertion; the corrected v3 driver verifies actual near/far helper shapes and
shows all20 variants accepted on each unmodified native profile, including the
invalid B+3 destination. That is missing-gate evidence, not a repaired gate pass.

The last `ownership28-wide-v2` precedes the capacity test port:999 tests each,
four failed modules. Trace errors fall65→38, with21 diagnostic mismatches plus the
now-ported expectation; Pdb retains two failures/two errors. No full qualification.
The `fifth-engineering-{release,debug}-identity.log` files identify clean4a and
117 modules each. Stats remains192.

`fifth-engineering-upstream/` gives logging0.838496 over two balanced pairs.
The separate `fifth-engineering-golden/` has logging0.585975, nbody0.999512,
pickle1.014001. See the [fifth report](fifth-integration-report.md) for pair values,
limits and the next three-stream packet. Engineering timing is not final parity.

## Terminal observers and saved-local restoration: c1424be11db

Independent source and result review admitted this bounded engineering commit.
Terminal providers publish outputs and continuation before observation, retain
detached owners in C, and preserve callback errors through retirement. Explicit
saved-local actions preserve genuine NULL bindings; an all-alias target proof
rejects unchecked reads. One generated outcome declaration feeds the existing
constructor ownership worklist, with separate compute-PRE and callback-POST
facts. Generated changes affect ten instrumented providers; 262 ordinary
handler bodies remain unchanged.

Evidence is under `build/pyjit-link-wave1/frame-call-model-20261109/`:

- `ownership29-dependencies-*`: corrected release build, three formerly failed
  methods passing, 2,156 valid compiler-corpus files and 11 intentionally invalid
  files. The earlier v2 retry used stale `codeobject.o`; missing IR header
  dependencies are now repaired in Makefile.pre.in. Retain that failed receipt.
- `ownership29-final-*`: debug build checks 117 modules; six core modules pass
  363 tests with one skip. The selected release gate passes 22 tests. Debug
  ownership probes leave zero survivors from 660 created values; the FOR
  callback-error probe creates three values and leaves zero, with all three
  errors caught. Its non-error control also leaves zero survivors.
- `ownership29-observers-*`: eight modules pass 64 tests on each profile.
- `ownership29-wide-*`: release runs 999 tests, 14 skips; monitoring has one
  error, peepholer three, sys_settrace 38, pdb two failures/two errors. The 21
  diagnostic mismatches are gone. This is not a wide-suite pass.
- `ownership29-committed-*`: release/debug identities rebuilt after commit;
  each checks 117 modules. Stats still identifies the older 192 image.

The [final source review](native29-final-source-review.md) and
[cold source/result review](native29-cold-source-review.md) retain the exact
scope. SOURCE_POST setter authority, yield cancellation, retained roots and
caught exits remain open. The next packet is independently reviewed in
[provider/stopped-state peer review](next-provider-stopped-state-peer-review.md).
No full qualification or new performance result is claimed by this repair gate.
