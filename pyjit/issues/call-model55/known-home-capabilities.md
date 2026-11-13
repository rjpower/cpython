# Known home commits

Implemented at static `3b4547380d0` and counted B `0ee3a61aba3`, following
source peer review. Both variants build117/79 in both roles. Static four-module
focus passes 36 cases per role; B eight-module focus passes 176, eight skips.
The [report](report.md) owns measurements and remaining authority limits.
This packet preserves storage/physical call ABI and NULL maintenance; it does
not establish the complete rival or choose its final representation.

## Actual source cost

Current WriteHome stores the value, derives the physical word and mask, loads
the bitmap word, compares its old bit with the new count tag, then conditionally
updates it. TakeHome rereads the value and invokes that same NULL replacement.
IR_CONSUME_INPUT already has the canonical value in C; vacant publications and
clears already have stronger Storage facts. These facts are currently lost.

Root's retained UNPACK_SEQUENCE_TUPLE dump in
E55/bitmap-home-writer-native illustrates both branches: even the no-count
value path computes word/mask and reads the old bit before deciding whether
to clear it. The loop keeps the bitmap pointer, base quotient and mask seed
live. This is bounded native evidence of redundant work, not a predicted
timing gain or permission to remove a correctness check.

## Proposed common interfaces

| Capability | Positive precondition | Static lowering | Counted B lowering |
| --- | --- | --- | --- |
| PublishVacant(frame, home, value) | Actual recipient is vacant, including its physical count bit; value is canonical | Store value | Store; only a counted value computes the bitmap address and sets its bit, without loading it to rediscover vacancy |
| Replace(frame, home, old, value) | `old` is the current home value captured with no intervening callback; value canonical | Store value | Compare old/new count tags in C; equal tags do not touch bitmap, differing tags set/clear directly |
| TakeKnown(frame, home, value) | C value is the actual canonical owner being removed from this home, not a stale local borrow | Store NULL, adopt value | Store NULL; only a counted value computes the bit address and clears it; do not reload the home/bitmap to rediscover its tag |
| VacateKnownEmpty(frame, home) | Actual earlier transport already removed its physical owner, and no later store reused it | Required blind NULL store | Same blind store, no bitmap access |

The proposed first signatures are `_PyFrame_PublishVacantHome(frame, home,
PyValue value)`, `_PyFrame_ReplaceKnownHome(frame, home, PyValueBorrow old,
PyValue value)` and `_PyFrame_TakeKnownHome(frame, home, PyValueBorrow value)`.
Only the last returns an adopted PyValue; the others return void. Use explicit
PyValue_Borrow at owner-valued callsites. Implement the same abstraction in
static and bitmap headers/macros, selected by actual Storage transport and
value provenance. Do not add a nullable-frame or runtime capability enum to
every call. A general WriteHome remains for genuinely unknown old occupancy;
this is not a compatibility fallback for an unported writer.

The old value is not closed by Replace. Its caller retains the existing
displaced-owner lifetime and closes only after commit/publication. Local
occupancy facts expire across callbacks; reload the current binding before
choosing Replace. TAKE of a borrowed local remains a DUP and leaves that home
alone. NO_HOME call roles never form an address or load a nonexistent self.
PublishVacant cannot follow an opaque helper or aliasing store merely because
the logical stack once said empty. Endpoint-neighbor bits remain untouched.
Logical dead/spent and NO_HOME alone are not physical vacancy receipts.
String/map helper drains really commit empty homes; tuple/list steal-success
inputs still have pending physical bits until final vacate. Callback paths
without a positive footprint or redirect handoff revoke scratch facts too.

There is no added frame field, tail-call argument or record. Replace has two
value arguments to an always-inline helper and TakeKnown has one extra known
value operand; these are existing C values, but extra register pressure and
spills must still be measured. In GIL code, direct set/clear eliminates the
explicit pretest, not the memory access inherent in a read-modify-write. FT
needs its existing unrelated-bit-preserving operations and remains outside the
counted authority qualification.

## Callback-free move batching

Same physical source/destination stays a no-op. For distinct cells, use the
known source value and actual destination vacancy/current-old capability.
When the computed bitmap-word pointers match, combine source clear and
destination set masks in one unrelated-bit-preserving update. Otherwise use
the two direct updates. Dynamic addresses require a word-pointer comparison;
do not assume adjacent logical homes share a word at every first-bit offset.
Bulk moves may accumulate per-word masks only across a proven callback/poll-
free interval and must commit before the first observer, close or publication
that can escape. The masks add live registers; charge them rather than
assuming batching wins for every scalar move.

## Bounded first source packet and gates

Start with canonical vacant IR_STORE_TEMP, IR_CONSUME_INPUT's existing C value,
and local replacement where the macro already loads `ir_old` immediately
before its store. Add source perturbations that revoke vacancy after a write,
callback, alias or unmatched branch, and actual bitmap/tag oracle fixtures for
inline, no-count, counted, NULL, replacement and physical-alias cases. Then
port one actual callback-free move consumer and test same-word/cross-word
endpoint offsets and displaced local callbacks. No benchmark-specific path.

Keep the current NULL-control stores in this packet. The authority gate is
separate: complete writers, counted consumer switch, then real copy/traverse/
error and observer validity closure before stale scratch. These capabilities
must help that representation work, not postpone it behind indefinite control
micro-optimization. GC and IR peer the exact Storage facts/API before source
implementation; root owns generation, native inspection and timing.

## Implemented first packet

P `6a80182f670` implements the three static commits, macro callsites and the
separate checked native-exit transport. Root, GC and IR give actual source GO;
coordinator host checks pass eight selected methods plus full cases/metadata.
The staged B merge of I `3b4547380d0` implements the matching counted bodies:
vacant publish sets without an old-bit test, equal count tags skip replacement
bitmap access, and a known non-counted take does not address the bitmap.
The existing 64-alignment probe now covers counted/count-preserving and
counted/inline transitions, known takes and physical aliases. GC gives actual
source GO; five parser checks pass. Coordinator owns merge commit, generation,
native checks and costs. No move batching or stale-scratch switch is included.
