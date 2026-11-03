# 7a rung1 landing review, round3

2026-10-29. Independent read-only review of
6f416bc4582952b8d7c8da590193ed94707cc8dd against b7d467969f6.
No tests, builds or edits by the reviewer. Verdict: LAND.

All four round2 findings are closed:

- The child prelude forwards verifier/stress settings, preserves explicit
  overrides and asserts actual state. Old-helper and omitted-stress
  controls fail at mode(0,0) and interval0. Raw records contain564 distinct
  debug-kill workers and24 ASan workers at(2,0), plus11 stress workers with
  paired start/end records at1000 and matching source/configuration hashes.
- Delivery/ownership tests identify individual calls and inspect payloads
  after collection. Compensating duplicate/drop controls fail despite
  unchanged totals. The native readiness fixture proves one receiving GIL
  acquisition/safepoint while the collecting thread remains blocked;
  suppressing readiness fails the scheduled-bit assertion. Omitting queue
  roots causes rediscovery/requeue in the two-inline-collection fixture.
- Normal/delayed domain traces explain the tenth/eleventh remap. The
  shared-deadline feeder wait has retained old/current/blocked controls
  proving premature warning, bounded completion and a preserved warning
  for a genuinely blocked feeder. The historical warning's exact cause
  remains unknown.
- Unbuilt checks cover seven supported module families, both decimal
  configurations. Normal suites retain d43c8b4dd32 provenance; later kill,
  ASan, stress and subprocess runs retain5cc31bacee1 provenance. Final
  native and affected runtime-test sources match that tested revision;
  seven product hashes match and generator tests62 pass per build.

The retained totals and zero-fallback claims check out. The normal-debug
root file also has a later cext suffix; its first5,111 records reproduce
the stated65,346 collections. Original unfiltered ASan smoke remains
visibly failed; the final exact overflow exclusion and five recovered
same-named cases are distinguished.

Limits: stress witnesses cover entry/end, not continuous state; two
fixtures intentionally disable automatic stress after startup. The
omitted-root control does not prove every payload edge. Capsule/buffer
coverage substitutes for a direct m_free oracle. Final-code measurements
are deferred to the2a boundary before runtime edits.

Clearance covers owner routing and the accepted rung1 boundary. FIFO
invocation does not prove dependent-call completion. S1/S3, ordering
allocation safety, owner metadata, retirement/adoption and fork disposal
remain2a–2c obligations.

Coordinator merged as052d8342e56. Detached integration passed smoke1,924
per build, queue tests22 per build and actual-worker-kill22. An initial
coordinator kill launch ran zero tests because BOIL_MODE_EVIDENCE was
omitted; that harness failure is retained separately. Corrected launch
records worker mode(2,0). Regeneration leaves detached source unchanged.
