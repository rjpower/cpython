# Source boundaries and immediate debugger entry

Source proposal, not runtime admission. The full engineering23 diagnostic has
missing pattern LINE coverage and debugger first-stop failures. Keep their
mechanisms separate before choosing repairs.

## Public requirement and actual dependency

Pdb's documented Python3.13 change requires set_trace to enter immediately;
it cannot be ported by shifting eighty command transcripts to the next line.
Primary documentation: [pdb.set_trace](https://docs.python.org/3/library/pdb.html#pdb.set_trace)
and [What's New3.13](https://docs.python.org/3/whatsnew/3.13.html#pdb).

Current bdb.Bdb.set_trace installs frame traces, calls set_stepinstr and waits
for the next real opcode callback. An ignored set_trace call formerly had a
result-retirement instruction on its own line. The compact NoDst call has no
such successor: its next real instruction can be on the next line. The doctest
failure's initial stop moves from the set_trace line to self.f1, consuming the
wrong command and cascading through later output. This source mechanism is
strong, but the bounded Bdb probe receipt remains needed before implementation.

Restoring an instruction event for a removed release would contradict the actual
instruction monitoring contract. Ordinary sys.settrace activation and immediate
debugger entry are also different operations; generic activation must not invent
an extra LINE at every call return.

## Proposed entry contract, unresolved API choice

Debugger entry needs its own explicit observer transaction at the selected
caller's current call/activation boundary. It displays the actual current source
line, preserves caller owners and the active child overlap, and accepts commands
before the caller advances. It is not a claim that a new instruction is about
to execute on that line.

Two possible implementations require peer selection:

1. A private debugger-entry invocation used by Bdb, establishing an explicit
   observer phase around its initial interaction while the caller is suspended
   in the real call. It must resolve observed target/child links, restore prior
   observer state on all exits, and make pending-caller jump restrictions
   explicit. Directly calling user_opcode without such a phase would leave
   f_lineno commands outside the setter contract and is insufficient.
2. A one-shot debugger stop delivered at the caller's real call-completion
   boundary, after child retirement and result ownership commit. It must preserve
   call-line display separately from next executable PC, honor generation-aware
   commands, and have an explicit observer/owner state. It is not a synthetic
   INSTRUCTION or generic LINE event, and ordinary calls must not acquire an
   unconditional hook/metadata walk just to serve debugger entry.

The first avoids adding normal return work but executes while the debugger's
child is active; the second has cleaner completed caller ownership but needs a
safe one-shot registration and completion path. Neither is admitted yet. Test
initial command step/next/continue, locals, same-line multi-statement calls,
ignored/used results, nested debugging, both backends, and permitted/rejected
jump commands. Existing deferred-child recipient behavior cannot be assumed to
support arbitrary live-result targets.

## Pattern source coverage is a producer/map question

The actual failing source is match command, literal case1 with an ifFalse body,
then default case_ with another ifFalse body. The unmatched path expects LINEs
1,2,5,6; it currently omits5. Codegen explicitly emits the default pattern's
coverage boundary. Source origins must survive frontend optimization, register
conversion, target relaxation and zero-word source-map emission on the actual
incoming edge. A missing record is repaired at its producer/transfer; an existing
record not dispatched is repaired in the observed edge path. Do not add an
executable boundary or globally infer lines between source offsets.

The coordinator-run probe-source-boundary-stops.py emits this exact ordinary
source's trace, real words, positions, co_lines and IRSM records, plus a
noninteractive Bdb initial-stop witness for both backends. It has host AST
validation only. Actual receipt should identify whether wildcard line5 is absent
from the map or skipped at runtime before any patch.

## Completion-event follow-up

Probe-observer-completion-events.py now separates modern-only, legacy-only and
combined tools for generator returns None/7, cold/warm and opcode tracing. The
first combined receipt shows candidate artificial FOR/None STOP plus legacy
exception, upstream neither; return7 reports an exception on both but modern
STOP only on candidate. This interaction is not enough to define modern-only
policy. Keep SEND and real iterator exceptions distinct, and retain exact-once
non-None completion observation. The None difference is lower priority than
source/debugger correctness and has no runtime fix admitted.

## Actual source-boundary receipt and peer direction

Coordinator release23/upstream probe confirms both issues independently:

- Wildcard unmatched candidate trace is1,2,6; upstream1,2,5,6. Candidate has
  zero IRSM records and no line5 co_lines/position entry. Real
  COMPARE_OP_JUMP goes directly to line6's result producer. This is a lost
  producer/optimizer boundary, not an existing source record skipped by runtime.
  probe-wildcard-cfg.py isolates codegen vs optimized locations next.
- Both Bdb backends stop candidate on next-line LINE offset3, upstream on
  call-line opcode offset2. The caller local x is still1 in either case;
  entering before x=2 does not fulfill the documented immediate call-line stop.

B independent design peer prefers option1: synchronous explicit debugger entry
avoids ordinary call-return work. Require a dedicated private phase/dispatch,
exact caller ancestry, recursion suppression and restoration on every error;
keep child overlap owned and use only proved deferred-call jump recipes. Replace
initial stepinstr state so step/next/continue cannot leave a second automatic
stop armed. This remains proposal admission pending the precise interface and
source ownership; no runtime change yet.

A source peer admitted B's separate real-LINE identity implementation only
after it required the first actual selected marklines target to equal current
PC. Source line equality alone was insufficient. This narrow recipe does not
solve active debugger-entry, protected nonidentity or suspension contexts.

## Localized wildcard correction

CFG-v2 receipt retains the default pattern's NOP line5 after frontend
optimization. The final branch is COMPARE_OP_JUMP. Existing word encoding selects
its actual control operation from cfg->ops[op->fuse], but assemble_source_map
uses the emitted compare's frontend target/format and skips the now-zero-word
conditional jump. Consequently it never records the taken edge through the
surviving default source boundary.

Bounded correction: for each emitted word, select its fused conditional jump as
the control operation when _PyReg_FUSED is set; otherwise select itself. Use
that operation's target, no-fallthrough classification and post-control-op
fallthrough index. Keep the actual emitted word index as event.from. This
matches encoding's existing target selection, introduces no runtime state and
leaves unfused transfers unchanged. The source record still derives its own
EH tuple from the actual eliminated source boundary.

Peer before edit. Preserve original patma assertion and add ordinary source
coverage for compare/contains/is fused conditions with multiple empty statements
on both edges, plus callback error in a protected source-only branch. Assert the
actual fused handler exists so fixtures cannot silently exercise only unfused
control. No executable boundary or output expectation weakening.

Actual fused source-map patch is source frozen for engineering24: select
control_index from the actual fused flag, assert same-block/zero-word control,
and derive target/fallthrough from that operation while retaining emitted
from-PC. Two new ordinary source tests cover compare/contains/is fused handlers,
both source-only statement edges, and protected callback exception routing.
Original patma expectations remain unchanged. Host AST/diff checks pass; B
actual source peer and coordinator native24 are pending. No event suppression,
executable boundary, generator or observer-state change.
