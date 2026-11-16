# Grouped review71: source and native basis

**Recommendation: CHANGE in two serial, independently selectable units.** First
transport audited canonical-bit facts through existing publication (A); then
partition eligible generated handlers at one pre-effect cut into hot execution
and a body-only terminal cold continuation (B). B has the larger potential.
A is not a mandatory winner for B. The [design71](../../design/proposals/interpreter-parity71.md)
defines contracts, five alternatives, gates and two flows. This document is a
review, not source GO, implementation or performance evidence.

## Authority and method

Read applicable AGENTS.md, the [official CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
canonical README/GOAL/STATUS/HANDOFF/PROCESS, active performance56 plan, report,
round70 closure, actual runtime source and retained source/native evidence.
Documentation branch starts at `704e93379df607e083d7bc902a3b5d063f9656fe`.
Canonical documentation checkout runtime is stale and was not used as authority.
No agents, runtime edits, generation, build, target execution, tests, timing,
profiles, controls or pushes were used for this review. Only these two docs change.

Runtime source roots, immutable for this review:

| Root | Revision / role |
| --- | --- |
| `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated` (**I**) | `0dcba75bb3df40c32137db453b001f7e8f101de0`, selected57 |
| `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-3a440b6` (**U**) | `3a440b65695615e9fd9912342b79cdd54fced0d2`, upstream |
| `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc` (**G**) | `52806aacedf5be0a89546344162bb53a8e40ec34`, frozen rejected69 |

Below, source anchors are relative to those actual absolute roots, never C.
Evidence root **E59** is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59`.
Selected captured ELF is sibling `call-model55/comp57-dictionary-native/python`,
SHA `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
U release ELF SHA `8ed926d80536c6f768bf0bcdcb225c631bf4ff29a23cecdc88c2120345ca5867`.
G69 release SHA `505b199a47f9b6c7148511492a4fd57267b3b635b825533739420decbd1b9b36`.

## Actual source anchors and deductions

| Actual source | Fact and implication |
| --- | --- |
| I `Python/ceval_macros.h:43–150,249,527,788`; `Python/ceval.c:2402` | Separate TARGET functions, five NCSR arguments and musttail dispatch; packed word decoding and cached feedback base+index already exist. A new separate handler does not enlarge every ordinary handler's allocator CFG. |
| U `Python/ceval.c:1268,1381`; actual upstream release `pyconfig.h` | Measured build uses computed gotos, not optional tail-call interpreter source. Native activation reserves0x168 once; do not count it per word. |
| I `Include/internal/pycore_value.h:101,141–152,568–601,789–843,882+` | Canonical homes, canonicalizing generic Publish, bit-preserving ownership operations and non-reentrant close criterion. Canonicality does not erase owning obligations. Duplicate closing owners need joint alias consideration. |
| I `Python/bytecodes.c:2125–2140,2228,2685–2818`; `Include/internal/pycore_cell.h:64`; `Objects/structmember.c:64–90` | Guarded dictionary/field/cell reads can supply canonical bits; existing SLOT/HINT/GLOBAL/CELL publication does not consistently retain that fact. Existing shape temporary is already canonical. Writers and reader preconditions must be audited together. |
| I `Tools/cases_generator/ir_generator.py:2974–3124,3709–3800,6566–6800,7045–7285,8480+` | Existing ownership/publication and canonical facts; many current source recognizers match whole token bodies. Entry, body and final outputs are distinguishable but no ready split API exists. Extend semantic value flow; do not add arbitrary helper-name trust. |
| I `Tools/cases_generator/ir_generator.py:4758`; `generators_common.py:804` | Parsed branch Storage clone/merge and IRStack ownership state support checking one explicit cut. They do not prove arbitrary C purity or identify invisible heap/statistic effects by themselves. |
| I `Python/regcfg_passes.c:155–415,680–802,1126+`; `Python/regcfg.c:453–475` | Local borrow/copy propagation, truth/compare folding and join renaming already selected. There is no general join-MOVE sequence to remove. |
| I `Include/internal/pycore_ir.h:15–105` | Current64-bit handler/A/B/C/D packing. Wider direct operands are a genuine alternative requiring all physical readers/writers to change, not a new annotation on the old format. |

Source inference: normal-C continuations can require state preservation on an
otherwise noncalling successful path. It is **not established** which cold edge
causes each native register choice, nor whether the proposed split eliminates it.
Only a replacement ELF can establish the latter. Canonical Publish removal may
already remove some continuation pressure; B cannot count that saving again.

## Retained native evidence, not new measurements

| Retained evidence under E59 | Established fact / limit |
| --- | --- |
| `parity60-engine1-static-review/selected57-BINARY_OP_ADD_INT.txt` | Inline scratch success enters a40B own frame; overflow boxes and preserves live values. Canonical arithmetic publication already selected. |
| `retained-delegation68-native-controls/static-active13b/selected-release-_TAIL_CALL_LOAD_ATTR_SLOT.txt` | SLOT1314B,56B own frame; ordinary C canonicalization/retirement paths and state preservation exist. Successful-path saving remains prospective. |
| `parity60-engine1-static-review/selected57-LOAD_ATTR_SHAPE_LOCAL_TEMP.txt` | Already canonical; no ordinary C call on the inspected body. Do not credit new canonicality here or assume every leaf has a large removable frame. |
| `compact64-baseline-static/_TAIL_CALL_LIST_APPEND.txt` | Provider call and state transport remain despite simple source operation. Moving this work to another returning helper does not remove it. |
| `round71-execution-architecture-input.md` | Pins actual U58,419B EvalFrameDefault envelope, selected369B wrapper and separate handlers; these are not comparable total interpreter text sizes. BOOL branch has only8B savedRBP. |
| `callout67-source-host/unit-a-native-peer.md` | Whole modeled boundary228→370 with ordinary helper preservation; selected provider/consumer calls excluded symmetrically. Not a new opportunity to repeat Invoke or CALL→APPEND fusion. |
| `retained-delegation68-native-controls/static-readback.md`, `static-final15.md`, `ordinary-path-source-note.md` | Changed native entry really grows24→72B own stack; inspected ordinary bodies often normalize equal, not all. Shared THREAD clear/result emission also changes. No logging cause established. |
| `guarded-summary69-native-peer.md`, `guarded-summary69-native-controls`, `guarded-summary69-natural` | Full actual395→437 and537→612; natural Delta successful653-instruction witness, comp0 demonstrated sites. Child removal is real, complete work grows, coverage is not frequency. |

Round69 remains incompletely behaviorally qualified and rejected by its early
screen (GM1.012667, logging1.176093, Delta1.034596). Fallback-only tools tests do
not close its committed-summary matrix. No closure is imported into71.
Round70's source stop is preserved in `round70-unit-proposal-peer.md`
(SHA `1e8995b8b15364d34ec6a5d1e2dff2ebfd53f12027ff03a2cd6f363ad10d0eb8`):
implicit property/getitem/shim recipients lack a CALL discriminator; typed routing
adds work without credible whole-consumer removal. Neither71 unit changes return
recipients or normal Python call semantics.

Other antecedents remain decisive: round60's monolith had4472B stack and lost all8;
compact64 saved47 internal instructions but added93 at entry/exit; role63 removed
branches while retaining owners and lost;67B added ordinary owner-mode checks;
68 qualified real retained delegation yet failed protection (gen0.708007,
Delta1.101089, logging1.239007). Stop those executables, not the legitimate
mechanism findings. New units use selected57, not their accumulated runtime.

## Independent source inputs incorporated

These are source proposals/reviews, not implementation or qualification:

- `round71-execution-architecture-input.md`, SHA
  `d11452436eb0d4f11c58afdf999832e2060be493bb41ce95a5df692a90b2aa92`.
- `round71-value-architecture-input.md`, SHA
  `12270e5ba34ec1cf88ce95f7e14fa58754119c471a034c6656a02e5a73ac5389`.
- `round71-split-source-proposal.md`, SHA
  `73ff5f149166f334fc883dcaa8db8ad915f880d2f205d42806965ff901fc9733`.
- `round71-canonical-source-proposal.md`, SHA
  `3c93c425f5ec90fd59919a05c36e77552ef5ae684163a2a75e88060b81d3f516`.

The adopted distinctions matter: no credible general home-elision unit is yet
closed; canonical bits are not ownership; existing canonical families earn no
new credit; a terminal cold body must receive advanced PC and skip semantic entry;
the arithmetic cut precedes body statistics; aliasing mortal close candidates
decline. The source cut is restricted and audited, not an arbitrary purity solver.
Exact worker proposals remain subject to root's independent cross-peer adoption.
The canonical proposal's owned nullable reader may move acquisition across only
the NULL branch under its explicit contract; native nullable checks remain a
cost, not an assumed zero. A result fact must not strengthen unrelated UNKNOWN
error/owner/effect authority.

## Work budget and disposition

A removes only redundant representation validation; retains all acquire/close,
home/NULL, dynamic guards and dispatch. B removes only proved native preservation
on complete successful paths; retains those same semantic operations and adds
eligibility, slow transfer/rechecks and cold text. A's frame saving cannot count
again for B. Native entry, miss, last-owner and local/discard consumers are in
scope, not just selected successful interiors. Ordinary nonparticipants must be
priced; no automatic logging recovery claim follows from unchanged source.

Old matched selected/upstream PMU remains comp instructions1.436416/cycles1.333179,
Delta1.361424/1.118423 and coro1.307831/1.267788. Generator1.372523 is old elapsed,
without fresh matched generator PMU. No new upstream ratios exist. The66 finite
logging diagnostic (instructions0.992588, cycles1.085426) rules out only an
equal-CPI proportional retired-count explanation of the entire gap; changed mix
or unobserved expensive work remains possible. No further diagnostic is ordered.

Prototype B is warranted by actual prologues and a concrete terminal ABI, not a
static speed proof. Source peer must close every cut and owner consumer; native
peer must show whole-path removal in arithmetic and reads, price slow routes and
exclude A's contribution. No broadened roster or counters to rescue a loser.
Root then uses unchanged protected hot8 and finite confirmation/all29/full/
upstream checkpoints from design71. A one-pair confirmation requires an explicitly
reviewed harness capability because current even-round validation does not allow
it. No silent timing-policy change is authorized.

**GO for independent source adoption of the two bounded designs; no source GO.**
Stop if contracts require a new runtime owner mode, an extra execution interpreter,
or no actual whole-consumer removal survives. Successful adoption permits a
bounded prototype, not selection. Parity and logging benefit remain unproved.
