# Held causal-control feedback

Prepared 2026-10-31. First native feedback completed and received independent
acceptance. Its original deadline remains historical. This separate proposal
grants no execution; root activation follows exact helper review and an idle
shared native window.

One new window: at most 45 minutes from actual activation, capped by the
overall 2026-11-01 04:18:11 UTC dispatch deadline. No broad qualification or
timing is included. Stop at the first unexpected result; expected negative
failures earn diagnostic evidence only.

| Source | Role | Runtime change from ef0 |
| --- | --- | --- |
| `ef0af95904b87d888eee4a98b6a745835ff95085` | Adopted candidate with strengthened finite-major test | None from accepted856 runtime |
| `d0dd0774107406256bd78a0d7bd45e2fbda15583` | Temporary force-major negative | Guarded minor entry passes literal0 |
| `b13efb1792dc0816b10ae1d75a49f0388e317593` | Temporary debt-reset negative | Successful minor resets allocated-byte debt |

The [source review](reviews/finite-major-oracle-20261031-v1.md) accepts the
test correction and narrow negative patches for preparation. No global nursery
switch is added; negative commits never enter the adopted branch.

Run six rows serially:

1. Baseline OFF: old boundary, exact debt/statistics and finite-major tests.
2. Baseline KILL with both extras: finite-major test.
3. Fresh debug force-major build, ceiling900 seconds.
4. Force-major OFF: old-boundary test must fail its original old-owner assertion.
5. Fresh debug debt-reset build, ceiling900 seconds.
6. Debt-reset OFF: exact debt and finite-major tests must fail their respective
   original debt-reset and major-starvation assertions.

All IDs have prefix `test.test_gc.NurseryTests.`. Exact suffixes are
`test_old_boundary_and_eventual_major`,
`test_minor_preserves_major_debt_and_statistics`, and
`test_repeated_minors_do_not_starve_automatic_major`.
Baseline positives precede either negative build. Every row binds actual
mode/extras/stress, seed, worker/child PID, source/configuration/origins, positive
main-interpreter collection records, zero fallback and idle cleanup. Negative
children must return1 with the nominated final assertion; outer regrtest must
return2 with exactly the selected failures and no errors/skips. Do not infer a
failure from a message merely printed inside source text.

Failed suites have no outer result token: preserve that fact and validate
actual returned failures/stats, unique verbose case IDs and typed child
return/stderr evidence. Keep the existing observer and original success
assertions. No extra unittest/regrtest hooks synthesize a result.

Baseline uses sealed compiled856 products through an exact test-only ef0
bridge. Each negative has its own managed worktree and fresh O1 debug products.
Preserve source isolation and all failed artifacts; qualification is a separate
later decision. After evidence closure, retire only task-created negative
worktrees through ordinary cleanup, retaining required evidence.

Frozen preparation lives at the baseline worktree's
`build/nursery-adoption-negative-controls/`. Plan SHA-256:
`a182fdb515f96e131a94f5c552d7c7dce1b48b8719f6e57adbfcd983e59a6409`;
manifest: `16316123c9e0539e6e6a6b5cf8aa53a9eb0129556c305afe0bf27e3e28833d21`;
ready: `69319f23aa91a570505ba639c03ea643b265d8126188580fc0334c9da9973e09`.
Its detailed design SHA-256 is
`aa4e55b30c18efce3714c9f60a23e30d2ef8c1f0c235232624251788841c16d1`.
Independent [preparation review](reviews/causal-controls-preparation-20261031-v1.md)
accepts these exact artifacts. Its receipt SHA-256 is
`37a09c0f9b6654e20ad8b075745714fd1a960213ede9e75084b9b2c7c7a99a3c`.
Root activation remains held behind the IR release-completion window.

Automatic halfway cadence remains a separate source increment and measured
accept/reject decision under the [accepted design](design.md). This small
causal-control batch cannot accept its scheduling cost or broader coverage.
