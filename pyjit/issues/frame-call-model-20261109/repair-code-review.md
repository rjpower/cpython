# Bounded repair post-review

2026-11-09. Read-only source review of the repairs after the first frame and
observer reviews, against the same `4b3d739d867b` baseline in the runtime
integration checkout. A/B were frozen and the coordinator's third release build
was running. No native jobs, runtime changes or commits by this reviewer.

## Open finding: GDB still receives the wrong container type

**P1**, `Tools/gdb/libpython.py:708-722`: the new envelope comparison uses
`locations[:4] == b"IRSM"`, but its actual caller obtains the table from
`PyBytesObjectPtr.proxyval` at line 1423, which returns `str(self)`. The comparison
therefore fails and the existing character decoder parses the header as location
data. Changing the comparison alone is insufficient: `int.from_bytes` cannot
read the resulting string slices either.

Normalize the proxy's Latin-1 string to bytes for the envelope header, then
retain/adapt the existing character payload reader, or decode header integers
with the character representation explicitly. Cover the actual proxy input
shape, not only a bytes argument.

A host-only AST extraction of the four pure parser functions confirmed the
failure: raw payload `b'\x80\x00'` represented as Latin-1 str yields the expected
`(0,1,10)` range; the same payload in a valid IRSM envelope represented as str
raises `RuntimeError: generator raised StopIteration`. This was not an inferior
process or native runtime test. The finding was sent to the coordinator promptly.

## Repairs reviewed

- MATERIALIZE_KWARGS is semantic opcode 127; the prior IR-only 325 allocation
  is retired. Its generated frontend stack effect and ordinary event identity
  now agree with compiler use.
- `value_reader_complete` treats IR_NO_DST as a C-owned result, skips physical
  result-home validation/publication, retains the owner across input retirement,
  and hands failure cleanup the same C owner used for a folded local. This
  removes the identified scalar-sink indexing error. The separate two-output
  method-home failure remains under coordinator investigation; this repair is
  not evidence that failure is fixed.
- The pending redirect bit is initialized for normal, trampoline, activation
  and base frames and copied with frame state. The setter admits the bounded
  case only from an active observed descendant, through an immediate VM child,
  with CALL/CALL_KW and a discard recipient. It validates the successful
  continuation's incoming register requirements, retains CALL as the exception
  position and defers scratch clearing until child retirement.
- The completion helper clears the pending bit and publishes the target PC
  before clearing scratch. The held result remains a separate C owner. Return
  macros reload return_pc after redirected completion, including callbacks
  during result disposal. Exceptional completion resets the pending bit both
  before and after child cleanup. No direct double release or overlap clearing
  before retirement was found in these bounded paths.
- The observer-only `observed_frame` field is saved/restored around monitoring
  callbacks and both manual legacy line callbacks. This avoids treating the
  Python callback implementation frame as the observed VM transition.
- Resume liveness now kills all scalar DST output homes, including method
  lookup's second output. Protected and suspended jumps remain conservatively
  rejected; this is still an incomplete debugger, not final admission.
- Frontend NOP removal no longer transfers a source location across POP_BLOCK
  or a different exception context; POP_BLOCK retains the exiting handler
  before becoming NOP. This addresses the demonstrated last-pass EH loss at
  its source rather than guessing from the landing word. Native evidence is
  still needed for both observer modes and traceback attribution.
- Fixed location payload bytes now reject high bits, with malformed short and
  one-line cases added. Unsupported ENTRY/from=-1 records are rejected rather
  than silently accepted. The remote C decoder unwraps the validated location
  section before its bounded payload parser. The independent test_code decoder
  correctly unwraps its bytes input. The GDB port has the type bug above.

## Focused missing tests

Before broadening the redirect protocol, exercise a scheduled parent redirect
followed by child failure, with and without a child-local finalizer that invokes
traced Python. Verify the exception stays at CALL, scratch owners retire once,
and a later normal call cannot inherit the canceled redirect. Add multiple
ancestor redirects before one child returns and an attempted redirect through
an external child; rejection must leave owners and continuation untouched.

For scalar sinks, include an attribute/item provider whose result owns its
receiver, operand-finalizer re-entry and observer enablement, plus a provider
error. Check result reachability after normal and exceptional disposal without
requiring exact historical finalizer timing. Keep the two-output method path
as a separate regression using the diagnostic's actual reproducer.

Rerun both source-only callback error modes and inspect the repaired final-pass
record and traceback line. Add nested protected regions so location transfer
cannot silently substitute a neighboring handler. Remote and GDB parser tests
must cover truncated headers, inconsistent lengths and real caller input types.

`git diff --check` passed during this review. Native frame tests passed on the
previous build according to the coordinator; the repaired source was not yet
qualified. Full protected/suspended debugger recipes and semantic loop/send/
shared-owner cleanup remain known unfinished work. No performance claim is made.
