# NULL36 generator PMU and CALL-window peer review

The six retained generator stat rows are valid, with equal before/after
identities and 100% event running. Recomputing each event's raw counter divided
by its own hot-loop count, then averaging the three normalized rows per role,
gives candidate/golden ratios of 1.045236699 cycles, 1.020423820
instructions, and 1.024316249 CPI. These match the retained summary. The
golden rows use 46 loops and the candidate rows use 44, so dividing the
unnormalized totals would be wrong. The initial wrong-affinity attempt is
failed preflight evidence, not a seventh stat row.

Both five-second record captures completed with stable identities and no lost
samples. The retained candidate ELF maps samples to `SEND_GEN`,
`JUMP_BACKWARD_NO_INTERRUPT`, `YIELD_VALUE`, `_PyFrame_PublishReturnSlot`, and
reference-count work. These are code locations, not an attribution of the
cycle or CPI difference. In particular, the candidate's live-binary flat
report did not resolve symbols after the live build changed; address mapping
used the retained matching ELF. These measurements describe committed NULL36,
not the subsequent forward-caught source packet.

For reproducible read-only location inspection, use the retained
`E11/null36-generator-perf-record/record-r1-{candidate,golden}-generators-cycles-record-perf.data`
files. The flat command is `perf report --stdio --no-children --sort
dso,symbol --percent-limit 1 -i PATH`; `perf buildid-list -i PATH` gives the
recorded executable ID. The retained candidate
`E11/null36-generator-perf-record/candidate-python` has build ID
`34e484e9e6ab0ad7965955f6cbd057141572ecb1`, matching the capture.
Resolve the unresolved candidate report's leading addresses with
`addr2line -f -C -e E11/null36-generator-perf-record/candidate-python
0x3774c0 0x33cace 0x37763b 0x37752c 0x343941 0x3438f4 0x367533`.
They map, in order, to `SEND_GEN`, `JUMP_BACKWARD_NO_INTERRUPT`, two more
`SEND_GEN` sites, `YIELD_VALUE`, `_PyFrame_PublishReturnSlot`, and
`_Py_INCREF_MORTAL`. The flat candidate report has zero lost samples; the
golden report resolves `_TAIL_CALL_SEND_GEN` and `_TAIL_CALL_YIELD_VALUE`
directly. Sampling shares are location clues, not per-operation costs.

The initial [CALL proposal](call-success-null-facts-next-proposal.md) needed a
complete provider census before implementation. The GDB capture establishes
that `across_with` home 2 and
`back_over_comprehensions` home 5 are physically NULL while the current cold
analysis still requires VALUE. It does not show that every normal CALL route
clears those homes. `call_transaction_detach` does clear captured native inputs,
whereas direct specialized handlers publish at `window[0]` and rely on
`IR_MOVE_TO_DST_OR_DISCARD`; that macro leaves a temporary result in place.
Thus a temporary destination other than BASE must remain UNKNOWN for the
entire window for such a route. Python overlapping-child return also needs
the explicit child-frame retirement proof; a `PUSHES_FRAME` bit alone is
insufficient.

Before a shared pristine CALL fact is admitted, enumerate every reachable
specialized and instrumented CALL path, including its exact captured count,
success-only retirements, and result home. Validate temporary A == BASE for
the direct-window family; local and NoDst use the actual final move. Unknown
routes, malformed geometry, errors, and homes outside the proven window stay
UNKNOWN. Keep downstream VALUE and saved-local checks unchanged. This is a
finite source proof for one cold query, not a new setter policy or an opcode
name whitelist. The v2 GDB probe is appropriate evidence for physical bits
and analysis rows; it deliberately does not establish stopped-phase authority.

The proposal later gained a finite census of the twenty `family(CALL, ...)`
specializations plus generic and instrumented CALL. I checked the family
declaration, direct generated publication, exact Python entry and overlapping
frame retirement; no concrete successful route remained unaccounted for in
this bounded pass. The resulting N header/test source is accepted for root's
native gate: it validates exact roles and extents, handles the `writes=0`
specialized Python/init rows as well as `writes=1`, requires temporary A ==
BASE, and leaves result, outside-window, invalid and error cases UNKNOWN.
The contract test pins all twenty-two reachable rows and alias/count/edge
negatives. This source acceptance does not imply native qualification.
