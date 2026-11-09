# Fifth engineering checkpoint: frame-state repairs

Measured runtime `4a1f70fcef49bccf210180fa69cfc4fb409a80cd` is not fully qualified
or promoted. Logging is faster than upstream in this short screen. Golden
`0765e9be47f` remains immutable and selected while correctness work continues.

## Performance

GCC 15.2, JIT explicitly disabled, CPU 15, fixed hash seed, unchanged kernels,
balanced process order, seven timed durations per process. Source, executable,
modules and drivers have stable pre/post identities. Ratios are candidate/control
elapsed time; lower is better.

| Workload | Control | Geometric mean | Two paired ratios |
| --- | --- | --- | --- |
| logging_silent | upstream | **0.838496** | 0.812193, 0.865650 |
| logging_silent | golden | **0.585975** | 0.579503, 0.592520 |
| nbody | golden | **0.999512** | 1.005882, 0.993182 |
| pickle | golden | **1.014001** | 1.021217, 1.006837 |

Evidence: `build/pyjit-link-wave1/frame-call-model-20261109/` in the
python-boil-the-ocean checkout, `fifth-engineering-{upstream,golden}/`.
Do not combine campaigns or attribute a speedup to cold debugger repairs.
The earlier nbody loss does not repeat. Pickle remains a small, variable loss
to check in the unchanged broad screen. Final logging parity requires two
independent balanced six-round campaigns, geometric mean at most 1.0 and every
pair at most 1.02, after coherent qualification.

The prior stable nbody census shows equal material routes apart from removed
release handlers, not native-cost attribution. Previous PMU captures belong to
the older 87 image. Profile the current coherent image before another hot-path
change; startup RSS and a zero-box census are not admission gates.

## Integrated frame-state change

Saved local bindings are obligations indexed by the actual named local.
Different locals restore independently; nested saves of one local remain
ordered. Losing a saved origin does not make its obligation disappear. Bounded
real-LINE PRE cancellation can abandon disposable expression inputs only with
positively empty saved-local columns and a compatible handled-exception context.

Assigning the current real LINE preserves its actual PC and phase. The previous
first-word selection could jump from with-exit back into with-entry on the same
source line and loop. The bounded cold/warm probe and original fixture now pass.
The obsolete arbitrary-capacity test now proves successful cancellation before
a 101-argument call; it no longer tests the removed operand-stack projection.

Release/debug each check 117 modules and pass 125 focused tests with one skip.
The separate identity/large-window gate passes two tests each. The last wide
gate, before that test-only port, runs 999 tests per profile and still fails
monitoring, pdb, peepholer and sys_settrace. Trace errors fall from 65 to 38;
21 diagnostic assertions and the now-ported capacity expectation also fail.
Pdb retains two failures/two errors. These are not full-suite passes.
Stats still identifies `192cfbe8aa0`; its earlier 146-test smoke does not qualify
the current source. See the [repair log](qualification-repairs.md).

## Next integrated model

The [cold review](cold-checkpoint28-review.md) ranks eight recommendations.
Their implementation is split across three coordinated streams:

1. **Terminal outputs and observer scopes:** factor the existing generator final
   output convention, publish complete homes and continuation before callbacks,
   detach old owners and preserve callback errors through reentrant closes.
   Inventory every composed/direct observer provider; advertise committed POST
   only after proving that provider's full path. An activation-owned borrowed
   scope carries actual site, physical next, edge, phase and original generation.
2. **Saved-local actions:** extend the existing request-owned Plan with proved
   source-home transfers. Detach all sources and displaced owners, install all
   final bindings and PC, then close. Genuine unbound bindings stay unbound.
   Lost, cyclic or cell-backed origins require stronger witnesses, not relaxed
   physical checks.
3. **Shared cold validation:** one generated per-edge outcome declaration and
   one existing constructor worklist. Physical new-value validity and old-owner
   provenance remain separate. Ordinary compute errors and committed callback
   errors have distinct input facts. The bounded FOR destination geometry repair
   is independently reviewed before admission.

These streams address review actions 1–6 and derive the effect-boundary facts in
action 8. Action 7, proving scratch ownership at ordinary frame exit, remains
an explicit integration audit. No generic runtime transaction object or second
unwind engine is planned. The actual FOR callback-error probe retains three
unpublished results after collection on both candidate and upstream; fixing
that leak is required even though upstream has the same bug.

Root owns regeneration, native builds, focused checks, commits and measurements.
After source/result review and basic screens, complete full release/debug, API
lint, current stats, the unchanged 29-workload screen with repeated material
losses, current PMU diagnostics, both final logging campaigns and a cold review.
The [plan](plan.md) and canonical [frame ABI](../../design/call-and-frame-lifetimes.md),
[IR](../../design/register-ir.md), [interpreter](../../design/interpreter.md) and
[debugger transfers](../../design/debugger-frame-transfers.md) remain the design
chain. Do not promote this engineering screen into the finished checkpoint.
