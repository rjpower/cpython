# Replace the bitmap source bridge with emitted home capabilities

The nine whole-body footprint matches unblock writer validation. They are a
temporary bridge, not the final writer interface. Removing this duplication
is an explicit M5/M7 gate. The static and bitmap generators must consume the
same checked transport operations; only their physical store implementation
differs. This proposal adds no runtime frame fields or addressable handler
records.

## Actual writer operations

Introduce checked DSL operations for an output-home publication, an input
home take, and a NULL home retirement. The emitter supplies the active frame
capability; source bodies never pass an arbitrary frame pointer. An operand
must identify a declared scalar home or declared array plus a proved index.
No arbitrary pointer expression, raw `frame`, `slots`, or `window` is accepted.

For array loops, prove the loop/index relationship rather than the whole
opcode body: the iterator is bounded by the declared extent, its address is
not exposed, and its index changes only by the checked loop step. Output
transport may reverse the input item index while keeping the physical output
index in its own declared range. This proof must not assume contiguous CALL
roles or give virtual holes a physical address.

Each actual emitted write records the same physical owning-home transport
used by normal admission. The bitmap implementation emits WriteHome/TakeHome;
the static implementation emits its ordinary publication/retirement primitive.
A take removes the home owner before a consuming helper or destructor runs.
A publication installs the canonical home owner before cleanup can reenter.
Local replacement retains the existing publish-then-close/reload contract.
Known nonmortal output writes may omit a bit change only with the independently
proved vacant-bit invariant.

## Narrow helper adapters

Add emitter adapters for the three real helper contracts rather than wrapping
arbitrary calls in an unchecked macro:

- String/map builders receive one declared input array and its derived count
  (map count is half the array extent). They drain exactly that range, clear
  each home before its close, and return a native owned result.
- Special lookup receives exactly the declared method/receiver pair. Its
  descriptor callbacks retain the receiver; successful replacement publishes
  the method before detaching/closing a bound receiver. Error exits retain
  their existing pair authority.
- Global lookup returns a native owned value, then publishes the sole declared
  output home before the operation's bridge cleanup.

The emitter owns the frame argument and argument-count derivation for these
adapters. Their finite storage/callback contracts stay explicit at the helper
boundary. A changed helper signature, unproved frame escape, or widened range
fails generation instead of falling back to a name-based grant.

## Bounded implementation order

First port direct window writes/takes and their index proof, then the two
builder adapters, then the special pair and global publication. Keep the
whole-body bridge only for unported bodies; delete each match with its actual
consumer port. Normal-final-commit and family closure checks remain necessary:
a home-write operation alone does not certify the rest of a handler.

Validation must compare emitted ownership events and metadata on both static
and bitmap sources, including source perturbations of indexes, extents,
helper storage, branch arms and frame aliases. Root owns regeneration/native
checks and must confirm no new handler stack record, argument or retained
register is introduced. This is a proposed follow-on, not a new source freeze.


## Concrete first source interface

IR recommends splitting fixed-index/adapters from induction loops. The first
packet ports fixed-index publications and the actual builder/pair adapters.
The array-loop packet follows immediately; its three whole-body matches remain
explicitly temporary until then. IR_TAKE_INPUT is specified below for that
consumer, but is not implemented until the loop packet actually uses it.

| DSL form | Checked source capability | Physical lowering |
| --- | --- | --- |
| `IR_WRITE_OUTPUT(array, index, value);` | A declared output array; constant index in its extent; existing canonical owner/publication contract | Store to the Storage/IRStack-selected home, with bitmap WriteHome on the rival |
| `IR_TAKE_INPUT(array, index)` | A declared owning input array; readable current home; index proven in its extent | Return the owner after vacating its physical home and bit, before any close |
| `IR_BUILD_STRING(pieces)` | Sole declared input array, extent `oparg` | Call the checked consuming string helper with emitter-derived frame and count |
| `IR_BUILD_MAP(values)` | Sole declared input array, extent `2*oparg` | Call the checked consuming map helper with frame and derived half-count |
| `IR_LOOKUP_SPECIAL(name, pair)` | Exact declared two-home through pair, method then receiver | Call the existing pair helper with exactly those two addresses and active frame |

The write is a statement marker; takes/builders are expression replacements
inside the existing C statement parser. Use existing IREmitter replacers,
Storage variables and IRStack PointerOffset/address mapping. Add no parallel
C parser or second slot decoder. `frame` is never a source argument to a
marker. Reject a native entry or native C buffer instead of adding a universal
NULL-frame fallback.

For the optional NULL output, index zero is legal only inside the matched
`if (oparg & 1)` arm for that declared extent. Validate the actual branch
before forming an address, including the existing NULL array pointer when
its extent is zero. Fixed two-home INSERT_NULL writes indexes 1 and 0;
LOAD_GLOBAL writes index zero of its sole output. Returned/helper values keep
the existing canonical publication rules: a marker must not silently turn a
borrowed or noncanonical C box into an owner or introduce an addressable
publication temporary.

A consuming builder call emits a compile-time drain receipt for exactly the
declared array; existing DEAD bookkeeping follows it. Error returns are also
drained, while its result remains a native counted owner until publication.
Lookup can replace only its pair and preserves its receiver through descriptor
callbacks; it is not a whole-frame mutation capability. These receipts permit
the existing actual final-vacate/event path, not a new grant from marker
presence. Normal-final-commit, whole-source effect checks and canonical family
closure remain required. Unknown array pointer escapes or raw frame access
fail closed. Do not broaden semantic READ/FRESH array programs or claim full
home-map coverage merely to remove these source matches.

## Immediate bounded loop packet

Port tuple/list output filling and BUILD_SET draining through the same
primitive. Prefer a forward physical index `i` while the object item expression
can use `oparg-1-i`. A generic ForStmt check proves initialization at zero,
comparison against the exact declared extent, a single increment, no other
assignment/shadow/address exposure of the index, and no mutable escape of the
array pointer. A constant bound or an outer checked conditional is a separate
positive case. Nested unknown loops, widened bounds, pointer cursors and
mutated indices do not inherit this capability. Reads and takes retain their
own initialized-home proof; an ownership bit does not supply it.

## Source split and cost accounting

Frames owns bytecode marker ports, existing helper signatures/contracts and
focused runtime fixtures. IR owns or explicitly delegates IREmitter replacers,
parser/analyzer marker effects, Storage/PointerOffset integration and event
receipt tests. GC peers clear-before-close, helper callbacks and native/arena
storage separation. Root freezes exact hunks before either lane edits shared
generator source, then owns regeneration, combined native review and tests.

There is no new frame field, tail-call argument, handler-local root record or
runtime index checker. Direct writes/takes keep the existing frame argument;
index proof runs at generation time. Source loop induction changes can alter
register allocation and must be inspected rather than assumed free.

Aligning the static C helper interface with the rival does have a cost:
BuildString/Map gain one frame argument (two to three); mixed special lookup
gains one (three to four). The bitmap lane already pays these arguments.
Static native lookup callers pass the explicit native capability, while its
actual bytecode adapter passes the active frame. This must be priced in native
dumps for the common static/bitmap contract; no claim that an outlined extra
argument is optimized away. Native conservative C-owner policy is unchanged.

LOAD_GLOBAL also changes from the direct out-pointer call to the existing
NO_INLINE native-result wrapper, followed by home publication. Charge that
wrapper hop in native inspection rather than assuming it disappears. The
wrapper owns its addressable result outside the handler.

## Escaping helper classification

The existing analyzer's `IR_MARKER_EFFECTS` also identifies protocols whose
emitter owns publication. `IREmitter.emit_SimpleStmt` suppresses ordinary
escape spilling/reloading for those markers. The builder and lookup adapters
are ordinary escaping helper calls: adding their names to that table without
separating this distinction would silently remove the existing callback
publication contract.

Before source freeze, give checked helper effects their own classification
for `find_escaping_api_calls`, while keeping the manual-protocol suppression
explicit. Equivalently, a DSL helper spelling recognized as an ordinary call
can preserve the same analysis, provided its checked replacer still supplies
the real frame/range capability. Do not implement a second spill path inside
the adapters. Fixed output writes have no callback of their own; their source
statement retains any independently detected escaping value expression.

The first shared hunks are therefore analyzer helper-effect classification,
the existing emitter's write/helper replacers and declared-array address
resolution, then the actual bytecode callsites. The static helper signatures
and their bitmap equivalents must land together before deleting their source
bridge entries. IR and frames freeze those exact hunks after delegated source
is committed; no B or integrated source changes are authorized by this note.

## First source packet

The pins implementation ports six actual uops: LOAD_GLOBAL's result,
conditional NULL, INSERT_NULL, string/map builders and special lookup. The
shared argument reader serves source validation and lowering. Checked array
names resolve to Storage's generated physical array bindings. Output reads
require a preceding write in a dominating lexical branch: equal condition
text in separate `if` statements does not establish domination, and else
arms remain distinct. Conditional extents also forbid changing or exposing
`oparg`.

`IR_HELPER_EFFECTS` records ordinary escaping adapters separately from manual
protocol markers. Static WriteHome is an inline store with debug frame/range
checks; bitmap WriteHome retains its physical bit commit. Builders carry the
strict frame argument and remove the home before each close. The mixed
special helper takes frame or explicit native NULL capability at all three
callsites. No new root scope or per-home nullable-frame fallback is introduced.

Two parser-only tests cover actual helper escape classification and source
perturbations. Emitted-storage checks and four callback/language fixtures
are provided for coordinator validation. Source review found and fixed
branch-identity confusion and parenthesized address/member-lvalue escapes.
Raw array reads now require complete by-value argument shapes of the actual
typed readers; unary addresses, member lvalues, unknown pointer contexts and
extent shadowing/mutation reject. Root's first host smoke found an invalid
synthetic output-only scalar format; the positive test now declares actual
window storage. Root's revised host check passes all three source tests and
full cases/metadata generation. Final formatting uses the existing token
spacing and CWriter machinery. C compilation and runtime remain pending.
The fixed/helper increment is committed `99560a5115e`; root's final host
recheck passes the three selected source methods and full cases/metadata
generation. This does not complete bridge removal or switch bitmap authority.

The immediate loop packet is committed `948577105fb` after root and GC actual
source GO. Tuple/list output filling uses forward physical indices and the
same reverse object-item order. SET uses TakeHome before hashing/closing.
The generic AST check requires typed zero/exclusive/unit induction, exactly
one index-matched operation on every normal iteration, immutable unexposed
index/extent and no skip/duplicate/unknown-control path. Fixed-index repeated
publications also reject. Counter names cannot shadow handler locals or
checked helper names. This is a declared-array bounds/unique-operation
capability, not a whole-body initialization or ownership certificate.

Three parser-only source checks pass. The emitted-storage method and three
runtime methods await coordinator validation. See writer-closure-ledger.md for
the remaining aliases, helper transfers, scalar/local writers and reader
gates; no closure claim follows from removing direct array lvalue stores.

## Steal-on-success helper handoff

The next bounded packet uses IR_BUILD_TUPLE/LIST to lower directly to the
existing native const-array helpers. A shared callee map supplies both the C
spelling and ordinary escape classification: tuple retains its existing
NON_ESCAPING policy; list retains its ordinary escaping policy. Neither marker
enters manual-protocol suppression. Tuple allocation schedules GC rather than
calling Python; its element copy and tracking are callback-free.

A parameterized AST ownership grammar requires a sole owning `values[oparg]`
input, native object result initialization, the sole NULL failure arm taking
ERROR_NO_POP, then INPUTS_DEAD and object-steal output adoption. It rejects
extra/reordered statements, different failure control, changed array/count,
aliases and additional uops. This sole-array grammar cannot request scalar
boxing; classification's provisional `record=True` is not a runtime value-box
requirement. Actual boxing discovery and final-vacate emission remain checked.

Failure leaves input homes intact. On success, ordinary emitter reload adds
no callback, and the actual final flush vacates the transferred input homes
before result displacement cleanup. This reuses the generated vacancy/bitmap
commit: there is no extra helper hop, frame argument, input sweep or temporary
record. Missing the success suffix or reached final commit cannot inherit a
normal drain receipt.

The source packet includes perturbation controls and a coordinator-only
boxing/emission-order check, a displaced-local finalizer observing the fully
built container, and one subprocess allocation-failure/retry fixture. The
fault arms after all 24 element owners exist, fails one allocation, and checks
the innermost traceback opcode before claiming a builder failure. Runtime and
generated-product validation remain coordinator work. Source review caught
and fixed an incorrect gate on the provisional discovery record.

The five-file packet is frozen as `2c27c4bd2e9`, after root and GC actual source
GO (IR direction GO only). Four parser-only checks and AST checks pass;
coordinator boxing/emission/native and allocation-failure validation remain.
The B merge source resolution also removes all nine copied-body bridge entries
while retaining strict physical bit writers and rejecting unapproved raw helper
uses. Root owns that merge's generated products and qualification.
