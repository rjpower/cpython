# Checkpoint-I upstream: separate make-only plan

Status: the separately reviewed exact make-only grant completed once,
2026-10-29. Products and final cleanup are frozen; independent
`MAKE_CLOSURE_CLEAR` accepts the closure. **Standalone qualification, origin witnesses and timing
have not run.** The original source-only draft is preserved below and in
its hash history; configure success alone did not authorize make.

## Accepted input and exact proposed command

Independent actual-config reviewde24b7f8 clears the once-only configure
closure, not a runtime environment. Preserve original V2 runnerd853,
plan419a, launch/inner receipts and all configured products unchanged.
The separate executor is under primary build storage:
`build/checkpoint-i-baseline-binding/make-v3/`.

| Input / proposed artifact | SHA256 |
|---|---|
| Accepted `../configure-closure.json` | `8908a1cc959d3828d5d50cda311e827400eef2b3bf79d92803d464cd12e49b02` |
| Accepted final configure launch success | `2dd59476670c92fec03cf1332be5b7ae7143304f6361ce91206aa21579be2124` |
| Accepted inner configure result | `1107b06e463851b6b5ea67af590331e4d03b2ed88fa1a71b285ab7c958a28b27` |
| Accepted14-product config manifest | `379f76037cb827ba5ba5abd1ddb7cb4592566beca8900cfd2f490c4f34da703a` |
| Proposed V3 `phase_runner.py` | `a3f271b50940a091f03245d7b3ef4179f1ca0e7b7eec82499ce5bc0b7567d9b4` |
| Proposed V3 `plan.json` | `d08ddfa21196e0d8e64c018530c9029419516c0aca543ba9b0b327bdaafa5096` |

The native command remains `/usr/bin/make -j4 all` in the owned exact321925
checkout's `build/default`, ceiling3600seconds. `make-launch.json` freezes
the exact host-uv outer argv, cwd and minimal environment, including the
reviewed plan hash and accepted configure-result/product hashes. The
proposed wrapper admits **build only**; it cannot rerun configure.

Reuse the same one-slot/task-lock/build-lock supervisor protocol and frozen
gate_run as V2. The new guards additionally require the accepted final
configure launch's return0/post-cleanup success, its joint closure and
all recorded raw artifact hashes. They reject a failed final receipt even
if the inner configure result succeeded. Before first make, the actual
build tree must equal the accepted14-product snapshot exactly; no old
executable/extension may be silently reused.

During locked make post-checks and again after gate cleanup, all14 accepted
configuration files must retain their exact bytes. Additional compiled
outputs are expected, and receive full before/after product inventories,
resolved alias bytes, raw logs, source/tool rechecks and a final launch
receipt before any later qualification. No input in the original configure
closure is modified to attach this new phase.

## Scope and source-specific policy

Normal upstream `all` necessarily executes build-internal bootstrap Python,
freezing, generated sysconfig/platform/build-details and checksharedmods
extension-import checks. An eventual make grant includes those disclosed
build actions only. No separately invoked interpreter smoke, origin
witness, suite, probe or timing is included. Stop after product capture
and final cleanup/closure for independent review.

The accepted upstream Makefile has **empty CFLAGS_CEVAL**; current fork
release has **-fno-gcse**, deliberately added by fork configure. Keep the
accepted upstream source policy unchanged for this build. Compiler/O3,
frame pointers and zero-init are matched, but every eval-unit flag is
not. This cohort cannot claim object-model-only attribution. Before any
checkpoint-I parity claim, require an upstream -fno-gcse sensitivity or
a predeclared fully matched eval cohort chosen from evidence. No extra
build or sensitivity experiment is authorized now. H1 benefit can use a
matched nonstats parent/changed calling-convention cohort; Py_STATS stays
mechanism evidence rather than elapsed performance.

## Copied-host falsifying controls

Eight host-only copied-plan controls pass at runnera3f271b5/pland08ddfa2;
`host-controls-v2/results.json` SHA256
`469dc1d3e15adeb9844d3c583be156e78df50d242d056a61c61c80e0184e6176`.
They execute a harmless existing host-Python fixture, never make or a new
upstream interpreter:

- Positive: actual inherited-lock protocol, accepted configuration and
  final cleanup remain valid while a new host text product is captured.
- A false accepted-final-success flag, changed joint closure, changed raw
  configure log, changed config product and wrong configure CLI identity
  each reject at their named guard before entering the host command.
- Deliberate config mutation in the successful host command rejects the
  locked post-check; the same mutation during actual gate sweep rejects
  the supervisor's final guard even with inner and gate return0.

The original V2 thirteen admission/cleanup controls remain source-scoped
evidence for unchanged protocol bodies; they are not relabeled V3 runs.
The first make-control attempt is retained under `host-controls/` with
`run_host_controls-v1.py`: its scratch products collided with the phase
evidence directory, and the outer guard refused before execution. This
is a harness failure with no semantic-negative credit. Corrected controls
use a separate products directory; no native runner/plan edit hid it.

`provenance.json` indexes source, plans, copied fixtures and all retained
logs/receipts. Original configure inputs and actual14 products were checked
again after controls and remain unchanged; the real upstream build has
no target python or compiled extension. This is make-plan evidence, not
make success, import binding, correctness qualification or parity.

## Exact granted execution and frozen products

The source-only first draft had SHA256
`af2eee87c055bc1d3da61ae2fb3f5d70dce026eb92a545319e43b3395b4dbcf5`.
Independent round5 `MAKE_ONLY_PLAN_CLEAR` (`a6c3ce2e`) and the coordinator's
exact command grant preceded the once-only execution of launch74e12464,
runnera3f271b5 and pland08ddfa2. Outer, gate and inner returned0; both
locked post-check and final post-cleanup verification succeeded. No
standalone target command followed the build.

Evidence is in primary `build/checkpoint-i-baseline-binding/`:

| Captured artifact | SHA256 |
|---|---|
| Joint `make-closure.json` | `dddcf24dbea8b0eead4b4bec8eb09f6801837a62b1b696bdd4de8a25b5e54a42` |
| V3 final `build-launch-result.json` | `241d86940daa38d0466281dcccb2dec90155c0277fe0b75986eb655240246940` |
| V3 inner `build/result.json` | `8539798e5b2d51476c7593cb2c877d0f2d0545d31ec5672e7f233c5dbfb3baa0` |
| V3 `build/products-after.json` and identical post-cleanup inventory | `ab41a2dfedb83d7976c4560209d3cb925770cd150d75bafafc0acc81e48f87db` |
| V3 `build/command.log` | `29b712caac5aaf8dc09d8208f7d28ad72ac5122112b980d74cf10ac39644e321` |
| Pre-make external candidate supplement | `b960cb2e5dee062d456cd54eeb12b1c84db633c2be2364a122f390041f67a13f` |

The closure rechecks5806 source entries,14 tools, all14 accepted config
products and original configure artifacts unchanged. It inventories584
build products,570 new since configure, and518 V3 evidence products,
including bootstrap-generated bytecode outside the upstream build root.
The executable is38,581,192bytes, SHA256
`4565c7f92f126a1ad6dab0ae756ec57fec612103682062c25bf9cfdc7da576e7`.
The134 `.so` paths include build-library aliases to67 module products;
they are not134 distinct extension implementations.

Upstream's build-internal check reports116 modules:37 built-in,67 shared,
1 not applicable,1 disabled,10 missing and0 failed imports. SSL and the
other optional backends absent at configure remain absent; this is build
evidence, not the standalone correctness/origin qualification. Make took
61.092seconds, launch through cleanup66.120seconds; these are setup
durations, not interpreter performance measurements.

The pre-make supplement binds341 configured SDK candidate paths and
metadata without copying the SDK or executing a compiler query. Config
detection is not actual consumed-header or mapped-library proof. Its
unbound remainder and the eval-policy difference above remain explicit
before timing/parity; a later final cohort rebuild may be needed. Preserve
the original configure8908 closure unchanged. Independent actual-build
review round6 accepts this closure only (`MAKE_CLOSURE_CLEAR`, review SHA256
`53446dc867096477045f83959241f4cd8d3e5ac964d4f1aef5a6f90df09bfa27`).
Separately reviewed and approved qualification/origin commands are next.
