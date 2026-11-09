# Corrected PRE cancellation result review

Reviewed the frozen two-file source diff and coordinator receipts. The correction
uses one active save top per actual encoded local, with parents only within that
column. Joins retain ambiguity; losing an origin never empties an obligation.
Independent restores commute. The nested fixture explicitly verifies both saves
name the same encoded local before substituting the older token into the younger
restore. The coordinator's compile-only inventories establish that the chosen
stores are normal-path restores before exceptional cleanup.

Read `ownership28-final-small-{release,debug}-focus.log` and
`ownership28-final-small-focus-checks.json`: both profiles report SUCCESS for
test_code, test_ir_debugger_entry, test_ir_source_reentry and test_ir_contract,
124 tests with1 skip each. Receipts identify source parent192cfbe8aa0, the same
two-file dirty diff, magic64416 and the intended runtime Lib/extension paths.
The corrected builds passed117 modules each. The isolated nested fixture retry
also passed both profiles.

The failed first global-order gate and both subsequent source-map fixture errors
remain evidence. The final fixture supplies a distinct equal map because the
constructor rejects structural code changes retaining the old container by
identity. Neither constructor behavior nor compiler restoration order changed.

The subsequent original-module run found a new nontermination regression, so
integration is withheld despite the focused passes. The unchanged running-line
test repeatedly assigns f_lineno to its current line. At a later real word on
that same line, the identity fast path's first-word prerequisite fails; PRE
cancellation can select an earlier word and restart the expression. The root
retained the timed-out worker and signal diagnostic. A capped original-fixture
probe and independently reviewed identity-rule correction are pending.

The positive focused evidence remains valid, but is insufficient for admission.
No named-local restoration, POST, suspended capability or performance benefit is
claimed. Original failing-module inventory and quiet measurements remain
coordinator-owned.

## Identity correction and wider result

Root's post-identity builds passed117 modules per profile. Independently read
`ownership28-identity-small-{release,debug}-focus.log`: both four-module gates
pass125 tests with1 skip. Parsed both `ownership28-running-line-after/*-probe.log`
receipts using the host interpreter: each has16 rows, zero capped rows, zero
errors and every expected result identity true. The original running-line method
and new capped manager-count regression pass. The C correction removes only the
first-word prerequisite; all other real-LINE phase gates remain.

The original four-module inventory now completes in both profiles:
`ownership28-wide-v2/{release,debug}.log` each reports999 tests, four failed
modules,24 reported failures and14 skips. This is not a successful broad gate.
The coordinator's detailed classification retains38 sys_settrace errors,
21 diagnostic mismatches and the obsolete projection-capacity refusal test;
Pdb retains two failures/two errors, monitoring one error and peepholer three
errors. None is waived by the focused passes.

The obsolete capacity test is being ported separately to a positive101-argument
call-window cancellation test, with one actual invocation, argument identity,
sentinel return, bounded single redirect and captured-frame validity assertions.
The class/live import rename to JumpFrameModelTestCase removes the obsolete
stack-projection description. A reviewed the actual test-only change; its native
gate remains coordinator-owned and pending. The corrected PRE/per-local/identity
runtime has no remaining concrete blocker found by this bounded review, but
integration waits for that focused test port. Full debugger and performance
qualification remain open.

The renamed-class positive large-window gate subsequently passed, together with
the unchanged original running-line method. Root integrated the narrow runtime
and focused tests as4a1f70fcef49bccf210180fa69cfc4fb409a80cd. Identity rebuild and
quiet timing remain separate gates; remaining debugger capability failures are
unchanged admission work, not waived by this commit.
