# R2: isolate the consuming Python-call fallback

Integrated runtime `25ce1654801`, parent `22f49d676db`, 2026-11-07.
The common overlap transfer remains first; only its decline invokes the
private no-inline fallback. The consuming fallback body and its ownership,
error and monitoring order are unchanged. The proposal, source reviews
(`calls-fallback-source-review.md`, `calls-boundary-review.md`) and fresh
`cold-r2-review.md` accept this boundary.

Release and debug each check 117 modules and pass the same five focused files
as R1: 916 tests and one skip per profile. Build identities match the commit.
No new distinct release warning lines appear. Full suites and stats rebuilding
remain deferred to the coherent batch boundary.

The retained GCC ELF separates the common overlap symbol (0x881 bytes) from
the fallback (0x917 bytes). The former reserves 0x58 stack bytes without the
fallback's stack canary; the latter reserves 0xb8 and retains its array/canary.
The preceding qualified C executable combined them in a 0x1275-byte symbol
with a 0xf8-byte reservation and canary before the guards. This comparison is
against retained C, not an exact retained R1 ELF: R1's ELF was not saved.

The seven-kernel basic screen versus R1 has raw candidate elapsed geomean
1.036849 and upstream-control change 1.009107. Individual results are mixed,
including regressions; this is not a speed admission. The change is retained
for its clean provider boundary. Final coherent evidence must assess throughput.

Evidence: `build/pyjit-link-wave1/interpreter-architecture-20261107/` under
`python-boil-the-ocean`: `r2-checks.json`, release/debug logs,
`r2-identities.json`, `r2-gcc/` (retained ELF and disassembly), `r2-basic/`.
