# Encoded completion proof: independent source review

Read-only review of completion_handler_safe and validate_completion_words in
Objects/codeobject.c after diagnostic6. No native execution or source edits.

The core transfer is coherent. A four-bit unavailable-home set is sufficient
for each completion window. REACHED is separate from an empty mask, and QUEUED
prevents duplicate ring entries; monotone unions terminate even with cycles.
Explicit semantic reads reject unavailable homes before definitions clear them.
The initial set does not treat SEND's publication A as reconstruction of an old
receiver. Exception and lasti stores define their own homes; ordinary exception
edges clear scratch at the actual floor. Per-outcome kills distinguish retired
NULL homes from produced values. RERAISE's implicit anchors are not encoded
semantic reads, preserving the actual PEP479 wrapper without a POP exemption.
Saved-local restoration remains a semantic read and must retain its owner.

Three fail-closed boundary issues were reported to B and the coordinator:

1. The read census ignores LOCAL_IN/MOVE/CLEAR/CELL roles. The general public
   constructor does not validate every local-role operand against nlocalsplus.
   A forged LOAD_FAST pointing into retired scratch can therefore bypass the
   read check. Validate these operand domains or count their actual semantics.
2. The proof selects the first covering exception-table entry. Runtime uses
   marked-entry binary search for long tables. Enforce canonical sorted,
   nonoverlapping ranges and entry-marker structure before proof, so malformed
   public tables cannot make proof and execution choose different handlers.
3. The worklist adds ordinary EH edges but not source-only LINE event error
   edges reached later inside an accepted handler. Source-map validation seeds
   this proof only when the event's from word is itself a completion. A forged
   ordinary-from record can route a later callback error into a handler that
   consumes the retired homes. Include reached source error edges with their
   actual outcome mask, or reject them conservatively under this proof.

These are constructor/proof boundary findings, not requests for a general new
IR verifier. Their repair/disposition remains with B. Native validation and
performance admission remain with the coordinator; approval is pending these
proof-alignment gaps.

## Repair disposition against frozen native9 source

All three findings are closed at source level:

- Completion-containing programs validate every LOCAL_IN through CELL field
  below nlocalsplus before any handler proof. Such fields cannot address a
  retired scratch home, including through public code.replace.
- The complete exception table is validated before any proof: entry-start
  markers appear only at the first byte, ranges are positive/sorted/disjoint,
  and targets plus exception/lasti homes are bounded. The proof's linear
  selection and runtime binary/linear lookup therefore agree.
- Every reached normal CFG outcome now considers its source callback error
  records and enqueues their handlers with the post-operation availability
  mask, unwind clearing and exception/lasti definitions. Matching by from and
  edge without requiring the recorded destination is a safe overapproximation.

The worklist still distinguishes reached-empty facts, preserves monotone unions
and checks reads before definitions. Initial SEND A publication does not supply
an old receiver owner. PEP479's semantically unread iterator/cursor holes remain
accepted; semantic reads and saved-local restoration still require their homes.
The separate NO_FALLTHROUGH classification preserves semantic CLEANUP_THROW
while allowing pure-jump optimizers to handle only actual jump instructions.

**Source admission:** no remaining concrete must-fix from this bounded peer
review. This does not claim a general malformed-bytecode type verifier, native
behavioral passage or performance qualification. Coordinator native gates remain
pending; no source/native/build edits were made during this review.

## Native11 optional-self refinement: proposal review

The FileFinder._find_children failure reaches next(scan_iterator) after an
exception handler: its CALL has a deliberately NULL self home. Treating every
unavailable original owner as a forbidden semantic read is too coarse for this
one operand role. The bounded two-set lattice is sound in proposal:

- U records unavailable old-owner obligations; N records may-be-nonNULL among
  U, so only U without N is a guaranteed NULL placeholder across admitted
  handler-entry phases. Seed all tracked homes U/N, then apply the actual
  unwind floor to establish NULL and clear both facts for exception/lasti defs.
- A retired preserved prefix below that floor remains U/N initially. Physical
  POST clearing alone does not prove that the PRE handler expected NULL. This
  prevents a forged required receiver from becoming an optional CALL self.
- Join ORs U and N, with N restricted to U. Fresh real definitions clear both;
  proven semantic retirement sets U and clears N. Generic reads do not by
  themselves prove retirement: retained/borrowed/pass-through inputs remain
  owners unless an exact contract proves their kill.
- The nullable exception applies only to CALL/CALL_KW B+1. Callable, other
  arguments, and CALL_FUNCTION_EX receive no blanket exemption. SEND A does
  not reconstruct an old owner. Other read checks retain the existing rule.

This closes the identified abstraction mismatch without hot state. Actual
source transfer review and native tests remain required, including the
preserved receiver-to-self and published-A negatives. Queue facts require
space for U4/N4 plus reached/queued flags; the existing zero-mask distinction
and monotone scheduling must be retained.

### Actual-source disposition after native11

Reviewed the frozen `completion_handler_safe` implementation and its two
constructor negatives. `uint16_t` stores U4/N4 and the separate reached/queued
bits. Entry applies the actual unwind to the all-U/all-N seed. Exception and
lasti definitions end the old-owner obligation; preserved homes below the
unwind floor retain it. Normal semantic retirement establishes NULL, ordinary
definitions end the obligation, and both exception and source-error edges carry
the two facts. The exact CALL/CALL_KW self exception uses U without N. OR joins
and reached-empty queue handling preserve the stated proof.

**Source admission:** no concrete must-fix in this lattice implementation.
The preserved-receiver negative checks both discarded and receiver-overlapping
SEND destinations. Native passage remains the coordinator's separate gate.

Native12 then exposed another nullable operand role, rather than a lattice
failure: CALL_FUNCTION_EX's fourth input is optional kwargs. Its transaction
uses `Py_XNewRef` for that home and accepts NULL kwargs in both Python and native
routes. The fixed EX self home is also NULL. An exact role exception for these
two homes, conditioned on U without N, is justified; callable and tuple remain
required. A preserved old receiver with N set must still be rejected. This is
not permission to exempt a whole call window or infer retirement from a read.

For the follow-up, use one cold nullable-input role helper shared by constructor
and debugger analysis: relative window bit 1 for CALL/CALL_KW; bits 1 and 3 for
CALL_FUNCTION_EX; zero otherwise. Normalize execution handlers to their semantic
opcode. Each consumer must still prove definite NULL using its own dataflow;
debugger application must also check the current cell is NULL. Keep this
contract separate from outcome definitions and retirement. It requires no new
hot checks or generated-format field.

## Read-only follow-up inventory

The debugger proposal's NULL requirement and active iterator pair are different
proofs. NULL is a forward must fact, intersected across incoming paths. An
iterator pair additionally needs dynamic-generation preservation from target
to the first arrival at the current site; a matching producer PC or non-NULL
cells alone is insufficient. Re-entry, consuming moves, replacement and unknown
ingress must reject the initial bounded recipe. Preserve verified owners,
detach surplus owners before callbacks, publish target first, and retain any
subsequent callback redirect.

Remote frame readers currently derive the line solely from instr_ptr/ir_words;
GDB's current_line_num similarly calls addr2line(f_lasti). Neither consults the
source-only observer cursor. The descriptor does not yet export the frame-object
cursor fields. A later bounded observer-display batch should share the public
frame line override semantics and keep compact f_lasti unchanged, with an
inactive/invalid cursor fallback to the real instruction position. This is a
known observation gap, not a reason to add a hot-frame cursor or virtual public
instruction domain. No implementation or native qualification is claimed here.

### Native13 broader cold operand census

SEND and FOR_ITER explicitly accept a NULL cursor at B+1: normal iteration
uses NULL, virtual iteration uses a tagged index. WITH_EXCEPT_START likewise
accepts NULL exit_self. CLEANUP_THROW's last-sent cell can be NULL after the VM
child-entry protocol detached the sent owner; its exception input remains
required. These are ABI role facts, distinct from a cleanup implementation
merely tolerating NULL.

The existing through extent proves only an unchanged prefix, not an absence
of semantic reads. LIST_APPEND still reads its through list owner. Generic
window writes must exclude that prefix from fresh definitions; COPY duplicates
its required bottom and leaves the unused middle untouched, while SWAP
transports its endpoints. Neither can manufacture a fresh valid owner from an
unavailable input. Conservative rejection of unavailable transport endpoints
is safe until exact fact transport is implemented.

Generated-source output mapping has a further distinction: PUSH_EXC_INFO and
LOAD_SUPER_ATTR_METHOD expose an A field but use STORE_NEVER with no_fold, and
actually publish B-relative window outputs. Their A is not a fresh definition.
CALL transactions and ordinary STORE_RESULT adapters publish their actual A;
CALL frame-return forms use A as recipient. FOR_ITER uses its explicit outcome
contract. A full in-memory census found only the two no_fold STORE_NEVER forms
above outside STORE_RESULT/RETURN and the semantic FOR_ITER override. A shared
cold mapping must represent this distinction or conservatively reject ambiguous
encodings; explicit DST presence alone is insufficient. No constructor/runtime
changes were made by this review.

### Frozen native15 source disposition

Reviewed the shared _PyIR_DefinesHome implementation against actual generated
publication. It handles semantic outcomes and SWAP before ordinary definitions;
UNPACKS retains its separate local-run destination. WINDOW definitions exclude
the unchanged prefix. WINDOW_DST_FIRST excludes exactly B+fmt.dst from those
window definitions and defines only the explicit A recipient instead. The
register mapping remains separate. Unknown output kinds define nothing.

The shared nullable-input contract now names window-relative or semantic
register input positions, with the same independent U/N check. Required
receivers, callables, tuple arguments and exceptions remain required.
STORE_GLOBAL's explicit NULL deletion path is included; STORE_ATTR is omitted
because its specializations do not all implement NULL deletion correctly.
No generic unused-array or pass-through read exemption was introduced.

**Source admission:** no remaining concrete must-fix in this bounded mapping
and nullable-role review. Whole-window reads can still conservatively reject
unread holes; they cannot create fresh ownership. The focused PUSH_EXC_INFO
negative covers its ignored A field. Native constructor corpus and behavioral
qualification remain the coordinator's gates. No runtime edits or native jobs
were made during this peer review.
