# Cold R4 repair review

2026-11-07. **Accept commit
`b58036e9718b87b95ad83b72e10d4578942a61d6` for the bounded publication-contract
increment. No remaining must-fix finding.** The original cold review and its
two rejected source shapes remain in [cold-r4-review.md](cold-r4-review.md).
This review covers the repair and its integration, not a native JIT backend or
a performance improvement.

The committed runtime worktree is clean. Its eight edited source files match
the reviewed isolated worker files byte for byte; the ninth file is generated
metadata. Independent normalization, removing only `result_phase` initializers,
reproduces the parent's metadata exactly. Generated interpreter bodies and
targets have no change from parent `225480a4250`.

The repair rejects known direct consumption, transfer, address escape and
mutation of generic completion's three owning inputs on continuing paths.
It recognizes existing release/take operations and `value_operation_close3`,
uses explicit parser node types, and rejects protected uses in loops
conservatively. The current flat cleanup branch ending in
`IR_ABORT_VALUE_READER()` remains valid. Variable names come from the checked
completion arguments rather than a fixed list of local names.

The explicit producer now requires a direct, unconditional assignment and
exactly one occurrence of its result name in that statement. This closes
same-statement consumption and a conditional expression containing an
assignment. Both original findings are resolved. Independent probes exercised
these failures through metadata construction and emission, plus receiver,
pending-result and pending-self closes, lowercase cleanup, stealing,
nonterminal conditional cleanup, loop-header consumption, member mutation
and prefix mutation. Safe control and the actual terminal cleanup were
accepted. The committed regression matrix covers the representative cases.

The ownership premise remains bounded: the checker recognizes the declared
intrinsics and source shape; reviewers must still establish arbitrary C helper
ownership and pointer/alias provenance. The shared phase describes the first
output on success. It does not authorize release motion, suppress refresh,
describe every output, or supply a failure root map. The clarified header also
correctly permits callbacks to change a folded local before this handler's
final store.

I inspected the completed `r4-repair-checks.json` and actual log footers in
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`:
all six commands returned zero; the isolated generator passed 73 tests; both
release and debug builds checked 117 modules with no missing modules or import
failures; both profiles passed 97 generator/contract tests. The coordinator's
repair driver also checked that interpreter, targets and metadata products
were byte-identical to the initially validated R4 products. The earlier eight
focused files passed 1,100 tests with one skip per profile, and value-API lint
covered 306 units with zero failing findings and zero parse errors. Those
earlier checks remain separate evidence for the unchanged runtime code.

Build warnings remain in unchanged generated cases, macros and unrelated
sources. Comparing their counts with older incremental builds would be
unsound. Full suites and stats qualification remain for the coherent batch
boundary.

Identity and basic-screen completion, same acceptance round: `r4-identities.json`
binds both executables' native Git identity to `b58036e9718`. Release/debug ELF
build IDs are `a410f4a9c634117fdb4921079b2bcdcdfd190b92` and
`cc0a5cdc68227964971994955eb13b0c25f9cc0a`. The R4 basic screen's before/after
identity snapshots are identical and record clean source worktrees. Independent
recomputation from the R3/R4 summary vectors gives candidate raw elapsed
geomean factor `0.99936338` and upstream-control drift `1.03883914`. Results
are mixed; this sequential screen establishes no R4 speedup. The retained R4
ELF and disassembly remain available under `r4-gcc` for the next comparison.
