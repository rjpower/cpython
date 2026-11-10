# Object lookup and call binding: active plan

Reduce repeated runtime work while preserving one canonical value model and one
frame/call ABI. Baseline runtime39 (`4d666e1b482`) meets logging parity but uses
1.175 times upstream elapsed time across 29 application kernels. Native JIT tiers
remain unfinished. Startup RSS and zero-box totals are not admission gates.

## Current checkpoint

Candidate `0a35a382909` implements canonical class-value caches, reusable fixed
keyword binding and a current-default-shape repair for positional calls.
Release/debug/stats builds pass, each checking 117 modules. Focused release/debug
checks pass 464 tests per build; stats smoke passes 150. Standalone lifetime tests
pass both receiver layouts. Five admitted keyword shapes each record 1,000 binder
hits; four unsupported shapes each record zero. Astra's
[post-review](final-post-review41.md) has no blocking finding.

The frozen comparison source is published as `pyjit-golden-20261109-runtime`;
combined golden `cb906f9bc52` includes canonical docs. Earlier immutable golden
controls remain unchanged. See [publication](../golden-20261109/README.md) and
[handoff](../../HANDOFF.md) for exact paths and qualification limits.

## Workstreams and ownership

| Owner | Delivered/current work | Next boundary |
| --- | --- | --- |
| Sol lookup lane | Canonical guarded cache; acquire/publish before retirement; value/interpreter docs | Read-only generator-cycle census proposal; no further runtime edits |
| Sol call lane | 1–3-keyword plans over overlap ABI; defaults guard; frame/IR docs | Reviewed typed cell-getset proposal; implementation waits for current measurements |
| Astra medium | Proposal reviews, actual-diff review, measurement audit | Critique future lanes and audit measured evidence independently |
| Coordinator | Regeneration, builds, native tests, commits and measurement | Finish balanced timings, combined diagnostics and performance report |

The contract is propose → independent review → implement → focused tests →
post-review → integrate → basic performance. Root serializes native work and
owns source/build identities. Bulk testing belongs at coherent checkpoints.

## Remaining admission work

The first full29 screen is 1.01024/frozen39 and 1.18105/upstream. Micro gains
are real but do not justify promotion. Repeat logging/coroutines/DeltaBlue losses,
retain current41 PMU evidence, and review the shared frame-inline experiment
before further features. See [current report](checkpoint41-report.md) and
[triage](regression-triage41.md).

1. Complete: six balanced long-interval micro rounds against frozen39 and upstream:
   class reads at 20,000 loops and keyword calls at 2,000. Keep the unchanged bodies,
   identical work per role, GCC, CPU 15, hash 0 and JIT disabled. Brief initial
   class samples show order sensitivity against upstream; do not call parity.
2. Complete: run the unchanged 29-kernel roster in two balanced rounds against both controls.
   Inspect individual material regressions and repeat them before deciding.
3. Collect current instruction/cycle counters and retained-ELF profiles after
   valid timing calibration. Attribute application improvements from evidence,
   not microbenchmark names or sample shares alone.
4. Run combined full release/debug diagnostics and the value API check. Separate
   known GDB display/frame-jump/trace-format limitations from new runtime failures.
   Preserve all failed preflights and rejected captures; exclude them from claims.
5. Publish a concise checkpoint report, update status/design qualification and
   Loom mirrors, then choose the next implementation from current evidence.

Source/builds and measurement helpers remain frozen during campaigns. No new
feature is admitted solely from host generator tests, an opcode name or a
source-level prediction. Actual fast-path hits, mutation/error/owner coverage
and balanced elapsed measurements are distinct requirements.

## Next design changes, in evidence order

Keep initialized reusable homes, overlapping call windows and explicit result
recipients/real continuations. Canonical lookup and keyword plans remove repeated
preparation under those contracts; they do not create another object or call ABI.

- **Generator resume convention:** obtain a whole-cycle census and one contract
  map for SEND, FOR and native entry. Share commit/completion semantics while
  choosing producer policies at generation time. The reviewed
  [generator proposal](generator-cycle-proposal.md) does not yet justify its
  EMPTY_TEMP experiment: count added updates/decoding as well as saved delivery
  work. A SEND-only improvement cannot fix the FOR-only resume micro.
- **Value-native boundaries:** the reviewed [cell proposal](value-native-cell-proposal.md)
  converts getter and setter together using existing typed callbacks, keeping
  public object getters' owned-box contract. Verify actual removed bridge work
  before broadening to other descriptors.
- **Structured IR effects:** derive ownership, publication, reload and recovery
  facts once for lowering, generation and validation. Migrate one proof with
  equivalent generated behavior first. The current declaration fix already
  derives mutability from the checked publication fact rather than opcode names.
- **Lookup coverage:** measure guarded-path/miss frequencies in real workloads.
  Guarded current hash-entry hints are bounded experiments within the existing
  layout. A wholesale table/hidden-class rewrite is not justified. Names are
  already interned; that alone does not remove repeated binding searches.

The [Astra survey](astra-survey.md), [future-lane review](future-lanes-review.md)
and [initial performance audit](performance-audit41.md) explain the ranking and
limits. Follow Python's data model and the selected ownership contract; exact
CPython destruction timing and developer-tool transcripts are not design goals.
