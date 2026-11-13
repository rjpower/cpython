# Model55 implementation ledger

Status: active. The user authorized implementation of the reviewed M0–M7 plan,
Sol high-effort workers and coordinator integration. The
[plan](plan.md), [target design](../../design/proposals/register-call-model55.md)
and [review resolution](review-resolution.md) own the contracts. Selected54
remains unchanged until a candidate earns admission.

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Evidence E=`BASE/pyjit-link-wave1/call-model55`.

| Source under BASE/worktrees | Branch | Owner |
| --- | --- | --- |
| call55-integrated | pyjit/call55-integrated | Coordinator: composition and generated products |
| call55-pins | pyjit/call55-pins | Sol frames/calls: frame header, binding, lifecycle |
| call55-ir | pyjit/call55-ir | Sol IR/image: transfer metadata and common validation |
| call55-gc | pyjit/call55-gc | Sol GC/observation: collectors and external readers |

All start from immutable selected54
`61015a9452d9069646fb5fede9ff963896833b89`. Controls, selected source and
selected release/debug builds remain untouched. Initial integration builds are
`BASE/call55-integrated-{release,debug}`, using the selected54 configurations
and module flags. Root records generation/build/test commands under E and
commits reviewed source before measurements.

| Stage | State | Required next evidence |
| --- | --- | --- |
| M0 allocation/interfaces | Source census and [placement cost model](placement-cost-model.md) independently reviewed. Shared arena plus initialized NULL named locals selected as M4 reference; split bases retain dirty-local fills and add addressing/reservation work | Measure actual dirty-local fills, chunk work and copy traffic at the static checkpoint; local validity remains a separate unimplemented alternative |
| M1 typed pins/admission | M1a typed pins dd3c9fc8d9e passes 932 focused tests per role and native/source review; hot confirmation mixed, no admission. M1b ce71fcfc240 passes 413 focused tests per role; two-pair screen still loses logging | Carry typed owned-function proof across call boundary; qualify and price composition |
| M2 shadow maps/roots | Shared-emitter/GC phase composition through ed08ab64c7e passes 675 core, nine phase and nine selected generated-contract cases per role; bounded source/native review GO. Hot6/all29/PMU show preparatory cost; no admission | Complete callback/exception/observer phases and general owner maps; partial coverage does not authorize stale scratch. Typed home authority and copy/escape packet are next |
| M3 all lifecycle readers | Provider/classify-once de95475accb has core/native/hot6/all29/PMU evidence. Constructor 9136c202e8e passes 3 precise and 942 core cases per role; heap teardown 0851fd51801 passes its precise fixture and 420 core cases per role. Full diagnostics expose a new OOM crash; 7c42db43224 fixes the specialization readiness proof, original OOM passes both roles | Retain NULL authority. Finish fix qualification and [reader ports](lifecycle-readers.md), including copy/escape, suspension and virtual CALL roles |
| M4 static switch | Not started | Atomic reader/collector/allocator switch and representation qualification |
| M5 bitmap comparison | [Concrete implementation design](bitmap-implementation.md) has both peers' GO; no source implementation yet | Same placement/boundness/transfer ABI and complete measured lifecycle |
| M6 optional lowering | Not started | Independently attributable useful increments |
| M7 final selection | Not started | Integrated source/native review, qualification, retention and unchanged-workload comparisons |

The six-case hot set is DeltaBlue, generators, coroutines, Richards,
comprehensions and silent logging. Selected54 upstream confirmation ratios
are respectively 1.154663, 1.450207, 1.254281, 1.280930, 1.390876 and 0.712216.
These are historical direct comparisons with their published ranges; do not
multiply them by new ratios. Use direct selected54 and upstream controls for
new campaigns. See the [active report](report.md) for M1a results.

Full suites belong at coherent representation integrations. Preserve existing
debugger/LLTrace failures, GC-verifier instability and the unexplained full-run
profiling failure as explicit limits; do not add quarantines. The existing GDB
pretty-print quarantine remains unchanged. Source-only review does not count
as a runtime pass, and a source-level store reduction does not establish a win.

## Collector policy decision during M3 review

Keep ordinary native stack/register/PyMem conservative discovery as an explicit
policy in this checkpoint. GIL cyclic RC already preserves real counted C
owners without a root head; extra per-owner chains would mainly serve the
optional verifier and add stores on ordinary native calls. Do not conflate
removing arena enumeration with a fully precise native GC migration. M3 must
cover every active/staged/retiring arena and heap owner partition, including
native/provider/binder windows and dynamic prefixes. Scalar boxes remain live
leaf roots under native discovery. The [collector policy audit](collector-policy.md)
records the distinction and source costs. The [helper proposal](helper-roots-proposal.md)
prioritizes actual staged arena gaps rather than blanket C-local registration.
