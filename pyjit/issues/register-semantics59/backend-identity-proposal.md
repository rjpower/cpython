# Round60 backend identity and preflight closure

Source implemented against immutable selected57 `0dcba75bb3df`; root's
configuration/native gates are pending. Selected controls remain unchanged.
The engine proposal and independent review are
[interpreter-parity60.md](../../design/proposals/interpreter-parity60.md) and
[backend-peer.md](backend-peer.md). Root owns configuration, generation, native
qualification and measurement; IR owns the engine conversion. The contract
below records the reviewed proposal, followed by its current source status.

## Configuration contract

On the experimental branch, replace the unconditional musttail/convention
checks in actual selected `configure.ac:7928–7999` with one explicit
`PY_INTERPRETER_BACKEND=computed_goto` substitution. Remove that branch's
`PY_TAIL_CALL_CC` substitution and Makefile field; do not encode computed goto
as a tail-call convention or claim `none` passed the old qualification.
`Makefile.pre.in` exports the new variable to generated sysconfig data.
No runtime public API, configuration switch or permanent dual-backend layer
is needed for this fully converted experiment.

Retain the existing computed-goto capability check at `configure.ac:7906`
and require its positive result for this engine. IR will also reject missing
`HAVE_COMPUTED_GOTOS` at compile time. Preserve its existing SRE use. This adds
no `Py_*` backend macro or new `pyconfig.h.in` definition; the generated header
already has `HAVE_COMPUTED_GOTOS`. Actual native evidence must corroborate the
configuration identity with the engine function and label table.

Important source distinction: selected57 has **no active**
`--with-computed-gotos` option and no GCC computed-goto flag tuning, despite
that historical argument appearing in its retained CONFIG_ARGS. Canonical C's
stale runtime configure is different. Do not copy its evaluator tuning or
`-fno-gcse`/cross-jumping policy into this experiment. Both selected Makefiles
have empty `CFLAGS_CEVAL`. Release OPT is `-DNDEBUG -g -O3 -Wall`; debug OPT is
`-g -Og -Wall` with `CONFIGURE_CFLAGS=-O1 -g`. Preserve the actual compiler,
hardening, dependency, allocator, shape and role-specific flags, not an
approximate reconstruction from the old design.

## Consumer boundaries

| File | Proposed change |
| --- | --- |
| W `configure.ac`, `Makefile.pre.in` | New explicit backend substitution; remove only candidate tail ABI requirements; require existing goto capability |
| W generated `configure` | Root regenerates from the reviewed configure source; do not hand-edit it |
| C `pyjit/bench/run.py:648` | Record raw `interpreter_backend` and `tail_call_cc`; qualify the combinations below in its existing build probe |
| C `pyjit/issues/checkpoint-closure/measure.py:78` | Include backend, tail convention, `Py_IR_INTERP`, goto capability and `CFLAGS_CEVAL` in the existing native configuration snapshot; retain all before/after identity gates |
| C `pyjit/issues/call-model55/run-checks.py` | Add raw backend/convention/capability to the existing version/magic/Lib preflight record; preserve its current build/module gates and paired explicit paths |
| C `pyjit/issues/call-model55/capture-native.py` | Extend its existing native-head probe/manifest with the same raw identity; preserve clean/head checks, exclusive destination, copied ELF hash and actual symbol validation |
| C `pyjit/bench/tests/test_backend_identity.py` | Small host-only mocked build-probe cases for valid and contradictory identities; no benchmark/perf subprocess |

The benchmark probe should read `PY_INTERPRETER_BACKEND`, `PY_TAIL_CALL_CC`,
`Py_IR_INTERP` and `HAVE_COMPUTED_GOTOS` in its existing single child invocation.
Allowed interpretations are deliberately finite:

| Raw backend | Raw tail convention | Disposition |
| --- | --- | --- |
| `computed_goto` | absent/None | Candidate, require register IR and positive goto capability |
| absent/None | `preserve_none` or `no_callee_saved_registers` | Frozen tail-call control; record the original convention |
| absent/None | absent/None | Older non-IR/upstream build, preserving current support; report backend unspecified rather than inventing a native fact |
| absent/None | `none` | Keep rejecting the unqualified plain-musttail build |
| `computed_goto` | any tail convention | Reject contradictory/stale configuration |
| unknown explicit backend, or IR with neither identity | any | Reject missing/unsupported candidate identity |

This supports the immutable controls without modifying or rebuilding them.
It does not provide a production compatibility path. The checkpoint controller
already calls the benchmark runner; its additional raw snapshot fields make
postflight changes visible without duplicating the qualification policy.
No throughput library, roster, loop, load, affinity, hash, JIT, extension,
ELF/source identity or retry gate changes.

## Root configuration and native procedure

Preserve W's recipient branch, then create the experiment branch from exact
selected57. Use new root-owned release/debug build directories and retained
selected role-specific configure argv/environment. Do not use `config.status
--recheck` from a rejected candidate or copy its caches/objects. Regenerate
configure using the repository's Autoconf 2.72 procedure
(`Tools/build/regen-configure.sh`, or its matching installed
`autoreconf -ivf -Werror` toolchain); inspect its diff. `pyconfig.h.in` should
stay unchanged. Root handles this generated step, then configures both roles.

Before native execution, retain configure commands/logs, Makefiles,
`pyconfig.h`, compiler version/path, and actual compile/link command flags.
Compare the relevant flags against the selected role, permitting only source
and build paths plus the explicit backend/convention change. Assert the new
Makefile/sysconfig backend, absence of `PY_TAIL_CALL_CC`, positive goto support,
and unchanged `CFLAGS_CEVAL`; do not alter options to rescue a compile or timing
result. Native version/source/Lib/extension checks remain mandatory.

IR's agreed interface is normal-ABI `run_handlers(frame,tstate,throwflag)`
with function-local `instruction_targets[]`, preserving the outer evaluator
and its activation cleanup. Capture both symbols and the complete engine.
The label-address table is the truthful replacement for per-handler symbols:
retain a handler-ID → target-address map from that named table and the same
ELF's section/symbol data, matched to generated target order. Resolve an actual
compiler suffix such as `instruction_targets.N` through its symbol, bounds and
relocations; do not assume the unsuffixed C name exists in the ELF. Missing or
unresolved table entries fail that diagnostic; do not report old
`_TAIL_CALL_*` symbol checks as passing. Legacy NCSR/musttail object scripts
remain sealed control evidence and are not run as candidate admission gates.

Native review must confirm ordinary ABI entry/return, actual computed dispatch,
fresh handler scopes, CET/branch protection and the whole engine's stack frame,
spills and code size. The new backend identity is not evidence of fewer dynamic
instructions. Source/module checks, observer/error/bridge/remote gates and root's
eight-case screen still precede any broad qualification or selection.

## First checks after implementation

Host mocked probes should accept the new candidate and both existing tail
conventions, retain stock/upstream support, and reject plain musttail, an unknown
backend, a computed-goto/tail-convention contradiction, missing goto capability
and an unidentified IR build. Assert that snapshot/manifests retain the raw
identity instead of manufacturing NCSR success. Root then owns configure
regeneration, both 117/79 builds, native source/head/config equivalence and
static engine/table inspection. No additional compiler or layout variants are
part of this proposal.

## Implemented source packet

Configuration source is frozen in W commit
`47653c3bb4c378eef5ea5690de5cfbef80f16661`, directly atop selected57;
the recipient branch is preserved. Root and IR reviewed the two-file change.
Root owns generated configure and fresh role builds using the retained official
Autoconf image, so no generated configuration file is in that source commit.

The four approved C tooling consumers and one host test file are implemented
and left uncommitted for root's combined review. Six mocked/synthetic host
methods pass in `E59/parity60-identity-source-host/retry.log` (0.006 seconds).
The initial failing synthetic section-offset assertion is retained separately;
it was a fixture offset correction. No target interpreter ran for these checks.
The map reader is deliberately bounded to the pinned ELF64/little-endian
x86-64 link configuration. It resolves RELATIVE addends and compiler table
suffixes; unsupported relocations/RELR, ambiguous symbols, truncated sections,
source/table disagreement and labels outside the engine fail the diagnostic.
Native mapping and backend qualification remain root gates.
IR's actual five-file tooling source review is GO. The frozen patch, new test
and full hashes are retained in `E59/parity60-identity-source-host/`; root's
combined review/commit and actual native ELF mapping are still pending.
