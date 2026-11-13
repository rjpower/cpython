# Bounded bitmap authority assessment

Source checkpoint: B `bf7` passed coordinator builds and its bounded writer
focus; the staged merge of I `3b4547380d0` adds general returns, tuple/list
handoffs and known-home commits. No stale-scratch authority claim follows.
Preserve counted B until the replacement contract below is selected.

## Earliest coherent counted switch

Complete writers allow a set counted bit to authorize loading that initialized
canonical object edge. GIL counted-root tracing and private retirement can
then select those bits without complete callback maps. Retirement must reload
after callbacks, preserve the descending cursor, clear before close, keep
named locals NULL and keep code/reservation/result/exception lifetime through
the last callback. No zero-bit word is loaded. Blind stores that implement
required named-local clearing remain.

The existing once-per-chunk tracing path covers physical arena owners,
including staged/unlinked reservations. It needs all moves and callable-to-
header transfers to clear old physical bits, plus explicit typed header and
detached result roots. Arena addresses must not be counted again through
transition prefixes. Heap traversal/subtraction is a different policy.

These counted-root and private-drain consumers are already implemented in B;
they are not a future switch. Their implementation cannot justify removing
all scratch NULL maintenance yet. These remaining readers would still be
unsafe on arbitrary stale zero-bit homes:

* copy/escape and generic generator creation must preserve live inline values,
  tagged indices and no-count object values as well as counted objects;
* generic `tp_traverse` / `gc.get_referents` must report no-count and immortal
  referents; verifier-wide immortal roots do not replace that API behavior;
* observer redirects, exceptional continuation cleanup, delegated/resume
  protocols and diagnostics need selected valid values or finite role proof;
* FT deferred marking/count adjustment cannot use counted bits as all-object
  presence. Keep its explicit initialized/tag policy.

For counted B, retain its existing counted consumers and explicit NULL control until
copy/introspection/error readers have their real selected authority, then
remove only the proven obsolete scratch stores. This does not supply the
complete rival needed for a final representation comparison.

## Would one occupancy bit finish the rival sooner?

One replacement bit could instead denote an initialized logical value. A
positive bit would then authorize copy and tag classification for inline,
control and no-count values too. A copy could enumerate positive homes,
retaining dynamic full named bindings, while GC classifies selected tags.
It would avoid waiting for general static callback maps solely to discover
non-counted scratch values. Definite assignment, NO_HOME call roles and finite
protocol admission still govern execution reads; an arbitrary zero-bit read
never becomes legal.

Two precise contracts must not be conflated:

| Contract | Extra work over counted bits |
| --- | --- |
| Every valid home, including NULL | Split logical publication from vacancy. Existing WriteHome(NULL) serves both purposes, so its callers need semantic Publish/Vacate distinction. Publish valid NULL sets the bit; take/vacate clears it. |
| Every valid non-NULL value | Zero means semantic absence/NULL only to consumers allowed to synthesize NULL, such as copy destination fill. It still does not authorize loading the source word. All inline/index/no-count publications set bits, all actual vacancy clears them. |

Replacing the counted plane costs no additional tail or header bytes. Under
the current maximum-first-bit heap allocation, N=24 uses 16 tail bytes and
N=224 uses 40, plus the existing extra 8-byte header pointer. A second plane
would add another 16/40 tail bytes in those examples and either another cached
pointer (+8 header bytes) or address arithmetic on updates. It is not the
first proposal.

Occupancy increases write traffic: inline arithmetic and index/control
publications that currently keep counted bits zero would set/clear them.
Retirement and tracing would load and classify each occupied value instead
of treating each selected bit as a counted owner. Tag classification on
publication may become a NULL test, but no native cost improvement is assumed.
Overlap still shares physical bits, endpoints still need masks, copied moves
still transfer authority, and native raw owners remain outside the plane.

Root selects a single non-NULL occupancy plane as the next bounded rival,
subject to GC/IR actual interface freeze. It closes the actual
non-counted copy and generic traversal blocker without waiting for every
static callback recipe. Preserve the counted source checkpoint; this is a
replacement of that plane, not a second bitmap or a reinterpretation of old
counted bits. The static comparator still requires full static closure.

## Actual remaining work and removal

| Source | Present B work | Replacement / required proof |
| --- | --- | --- |
| `gc_trace.c` precise roots and `frame.c` private retirement | Already select counted bits, reload cursors after callbacks, and keep typed headers/results explicit | Presence selection must classify each positive value. Skip inline/index values for GC; preserve actual counted-versus-no-count policy. No new claim that these consumers were previously unimplemented. |
| `gc.c:_PyGC_VisitFrameStack` | Generic full-N tag scan except created selected view | Presence iteration reports no-count and immortal referents too. A smaller counted-only packet could select bits only for the existing exact `visit == visit_decref` policy, but would leave generic traversal unresolved. |
| `gc_trace.c:scan_thread_struct` | Explicit conservative arena ranges remain alongside precise arena roots | Remove these arena ranges only after writer, typed header and unlinked-reservation proof; retain native stack/register/PyMem policy. Record the STRUCTS/FRAMES flag behavior explicitly. |
| `interpframe.h:_PyFrame_Copy`, `frame.c:take_ownership` | Selected ranges or explicit NULL control, then destination NULL fill | Dynamic named bindings remain fully initialized. For scratch, move only positive homes clipped to the actual owner/cursor extent; synthesize NULL without reading zero-bit source cells only where required by the destination API. |
| ERROR, redirects, resume, delegated receiver and debug readers | Some full NULL-control ranges and physical NULL checks remain | Ownership drains may iterate presence. Execution and finite-role reads still require their own protocol proof. Diagnostics check positive scratch plus initialized named bindings; no zero-bit word loads. |

The source packet crosses `pycore_home_bitmap.h`, the three known-home
commits, frame initialization/constants and constructors, `frame.c`,
`frameobject.c`, `genobject.c`, `gc.c`, `gc_trace.c`, transition prefixes,
observer local/redirect helpers, CAPI probes and generated writer products.
The shared emitter's physical stores/takes remain the same interface. Every
blind NULL initialization/vacate and every physical move must update or clear
the presence plane before a callback. Code/header cells sharing physical
words must remain zero-bit; endpoint masks preserve adjacent owners.

A bitmap view needs an explicit representation rather than falsely labelling
all scratch readable. One candidate retains the existing 16-byte HomeView:
keep the two 16-bit bounds, divide its 32-bit metadata into an explicit kind,
physical first-bit offset and range count, and use a pointer union for static
ranges versus bitmap words. A common iterator can then return positive
scratch runs while retaining full dynamic bindings. Static views keep their
existing borrowed code lifetime; bitmap views borrow the actual frame/chunk
storage under the owner's cursor/lifetime. Metadata-only transition nodes
have neither form. IR must freeze the encoding before source; no magic pointer
shape or stale PC reconstructs authority.

The representation adds no storage bytes or handler argument. It adds set/clear
updates for inline, tagged-index and no-count values and tag tests to positive
GC/drain cells. ReplaceKnown can skip bitmap access when old/new presence is
equal; TakeKnown only clears for a known non-NULL value. These replace the
counted tests in the current three helpers. The expected removed work is
full scratch NULL initialization, skipped-home NULL retirement sweeps and
full physical source copy/traverse scans, after their actual readers are
ported. Native measurements must price those removals against the added
scalar bit traffic; the current bits-plus-NULL timing is not that comparison.

Named-local initialization and Python boundness do not change. Zero presence
only permits authorized NULL synthesis, not execution, observer-role or
receiver reads. A present bit can authorize tag classification, but cannot
repair a missing CALL-role or delegated protocol certificate. FT still needs
its existing mutation/read claims and separate deferred policy; initial
qualification remains GIL. The exact ownership/lookup interface peer remains a source gate.

GC's bounded policy peer gives GO to this assessment: positive counted bits
may select GIL counted roots/private drains after writer closure; occupancy is
a separate selection, with physical handoff and Publish/Vacate semantics still
required. No second plane or FT shortcut is implied.

## Selected source interface and sequence

Root selects the single non-NULL plane; counted checkpoint `0ee3a61aba3`
remains preserved. Runtime source ports began after GC/IR interface peer and the B native
window closed; full-source peer remains the freeze gate. The bit means a canonical non-NULL home currently owned by
that frame, not every initialized pending value at the same physical address.
An external prefix owns its initialized values under its explicit range while
the corresponding frame bits remain clear. Success publishes bits before
prefix disarm, callback-free; failure advances the external cursor before
NULL/close. This avoids a second validity plane and prefix-chain subtraction
from ordinary heap traversal. It adds an output commit pass where the actual
emitted final publication cannot absorb it. The current tracer's
arena-resident-prefix skip must change: address residency alone no longer
means the arena bitmap covers an external owner.

The agreed IR view shape is exactly 16 bytes on the current 64-bit ABI:

```c
typedef struct {
    uint16_t nbindings, scratch_limit;
    uint32_t nranges:24, kind:2, first_bit:6;
    union {
        const _PyIRPhaseRange *ranges;
        const uint64_t *presence;
    };
} _PyIRHomeView;
```

Kinds are `_PyIR_HOME_VIEW_RANGES=0` and
`_PyIR_HOME_VIEW_PRESENCE=1`; 2/3 reject. Range views require first_bit=0;
presence views require nranges=0. Existing designated static initializers
remain range views. Presence addresses use `(first_bit + home_index) / 64`
and the corresponding bit, relative to the supplied frame base. Dynamic
views borrow the frame/chunk allocation, not merely the executing-code pin.
B record authority `_PyIR_HOME_FRAME_PRESENCE=3` requests a deferred presence
view from the frame and explicit remaining extent, with homes=NULL. It is
not an already selected descriptor or a full-N readability assertion. NONE
stays empty metadata. `_PyIR_HOME_MUST_READABLE=2` means an already selected
readable view of either kind; it does not imply a static proof. Materializing
tag 3 for copy normalizes it to tag 2, and captured views retain only tag 2.
A candidate row supplies neither authority. Reserved kinds and mismatched metadata reject before union access,
not as a producer assertion that the shape is empty.
The iterator dispatches before reading the pointer union and returns full
dynamic named bindings followed by positive scratch runs clipped to the
actual ownership extent. N is admitted within IR_MAX_SLOTS, so a 24-bit range
count exceeds the maximum number of disjoint home ranges. IR gives interface
GO to this schema, not source/authority qualification.

Presence does not assign a shared cell to a parent. A real overlapping
activated THREAD child removes the parent suffix starting at the actual child
base; an independently reserved child does not remove still-owned parent PRE
inputs. Retirement and takeover use the real remaining cursor, not a PC or
full-N reconstruction. Captured heap storage has its own bitmap and retained
extent. Common view lookup must find the actual active child for a waiting
THREAD query, or receive an explicit child/extent capability from the actual
walker. It may not infer this partition from bits or the CALL B operand.
Root selects actual own-thread chain lookup and explicit producer extents.
Foreign THREAD generic queries return NONE and have no full-N fallback; the
new inspection helper raises an explicit error. Supported foreign frame names
and bindings remain available. Existing container traversal visits only owned
heap frame storage, and precise THREAD roots visit physical arenas once, so
these actual consumers do not need a new global thread lookup or WAIT marker.
Any later foreign protected scratch reader must freeze a real HEAD/quiescence
capability; GIL alone is not thread-list lock ownership. GC gives contract GO.

Source ownership and first complete port sequence:

1. Frames owns the tiny view type/iterator hunk, presence storage/address
   helpers and the three known-home bodies in B. Rename counted helper names
   and update all consumers together; do not leave an old counted name with a
   new presence meaning. Write/Publish/Replace/Take/Move use canonical NULL
   presence, preserve unrelated physical bits and commit before callbacks.
2. Frames ports constructor and reservation frontiers, local/constants/cell
   initialization, source/destination copies, resume/publication, exact-call
   handoff and native-entry exemptions. Named locals still initialize NULL.
   Frame/chunk header cells must have no home bits, including reused overlap.
   Fresh scratch storage may remain stale only after the reader packet below.
3. Frames ports actual external unpack ownership as above, preserving the
   input/iterator/native owners and cleanup order. Native observer retirement
   prefixes retain their explicit initialized range and native policy.
4. Frames ports reverse retirement, temporary/error drains, selected copy,
   generic traversal and precise tracing to positive presence plus the
   consumer's existing tag/count policy. All actual owner partitions remain
   clipped. Code, result, exception and arena reservation lifetimes remain
   fixed through the last callback. Generic traversal keeps no-count and
   immortal referents; FT count adjustment retains its deferred policy.
5. Frames ports debug/LLTRACE/observer reads: initialized named bindings or
   positive scratch only, synthesizing NULL only for authorized absence
   consumers. Frame proxy boundness, execution operands and finite delegated
   receiver checks retain their own contracts. CAPI fake frames receive real
   bitmap tails and deliberately poisoned zero-bit scratch controls.
6. After all source readers/writers compose, remove old scratch NULL fills,
   skipped-cell retirement NULL sweeps and explicit conservative arena scans.
   Retain native stack/register/PyMem conservative discovery. Root regenerates,
   builds, runs faults/nested callbacks and captures costs before comparison.

GC and IR remain source peers; neither is assigned concurrent B runtime
edits while their static packets are active. Shared IR source-container hunks
remain IR-owned. The view hunk is explicitly assigned to Frames; the existing ERROR
lane retains ownership of any later shared HEAD-held traversal capability.
No speculative second plane, per-handler addressable guard or extra tail-call
argument is part of this packet.

### First actual source packet and execution-read limit

The first B source packet implements the renamed presence plane and typed
view, actual-child clipping, explicit cursor capture/rebinding, external unpack
commit, positive generic GIL traversal and logical/precise transition reads.
It removes skipped-zero retirement NULL sweeps and the explicit conservative
arena ranges. Native stack/register/PyMem scanning remains. The external
prefix no longer needs a bitmap pointer (24-byte prefix, 40-byte root record).
IR gives actual schema/iterator-only GO. GC gives final source GO on all 17
paths, including the allocation-free inspection snapshot and explicit public
copy rejection. Root qualifies the presence-plus-drain checkpoint `0c47a4616ef`:
both builds produce 117/79 modules; 27 ownership/provider/writer cases pass per
role, then 510 GC/generator/coroutine/frame/weakref/finalization/profile cases
pass per role (release 14 skips, debug 13). Evidence is
`E/bitmap-presence-gc-integration-*`. This reaches native-tested ownership
authority, with the initialized-NULL execution policy and full reader packet
still open; it does not qualify stale execution or a complete rival comparison.

This packet deliberately retains the fresh arena initialized frontier,
semantic NULL stores and destination gap fills. Current admission permits
some public POP_TOP/COPY/SWAP reads of initially NULL cells; a zero presence
bit is not permission for those raw execution reads of stale storage. Every
positive home still becomes physical NULL on Take/vacate, so omitting an
already-zero retirement store preserves this initial read contract. The
remaining full initialization and copy-fill work is an explicit representation
cost, not a claim that the stale-scratch execution gate is closed. Removing
it requires positive input readability admission or checked NULL-synthesizing
reads for the actual scalar/array/protocol cases, priced before source.

The packet adds four runtime fixtures: actual waiting-parent/child ownership,
pending unpack external authority versus frame presence, immortal generic
referents, and foreign THREAD explicit query rejection with named-local
observation preserved. The existing all-64-offset probe now includes inline,
index, immortal and counted homes and iterates the real dynamic view. Existing
poisoned copy and collector probes use presence selection without executing
fake frames. Root owns all generation/native/fault/performance validation.

The inspection helper snapshots object identities without Python allocation
before constructing its Python result, so later allocations cannot invalidate
an associated generator frame pointer still in use. Public frame-stealing
generator constructors reject non-owned storage before copying; they also
check view availability explicitly instead of relying on a debug assertion.
The reached created-copy marker supplies its known current-frame extent
directly, so it cannot turn a failed generic lookup into a full-N fallback.

Unlinked-header census for the arena-scan removal: image preparation and
reservation precede header ownership; `_PyFrame_Initialize` and constant DUPs
are callback-free. `stage_binding_frame` publishes CODE/FUNCTION/nullable
LOCALS/FRAME_OBJECT before `initialize_locals`, including keyword comparison,
defaults/error formatting and later EX/provider cleanup. The stage hands off
only at actual evaluator publication or callback-free M2 retirement. The slow
init shim publishes CODE plus its actual home extent before binder/workspace
cleanup; exact entry remains callback-free until current-child publication.
Public heap frame/generator creation retains native/source code pins until the
owned header is complete. The new
`ProviderHandoffTests.test_unlinked_header_pins_survive_collection_and_hook_failure`
forces collection and a hook fault before evaluator publication in both object
and value providers, requiring precise function/code/mapping header IDs.

Initial host checks pass value text-rule fixtures, changed-source raw-value
and slot-write text checks, test AST and preprocessor balance. These do not
replace root's generated products, native compilation, whole writer oracle,
GC fault runs or reader/remote coverage. The existing ancestor-unpack redirect
rejection fixture remains applicable to the pending external prefix interval.

### Execution-reader packet after presence qualification

Root selects absence-aware execution reads for the dynamic rival, subject to
the following source closure. B `4a1a39fa9e3` implements presence ownership;
root merged current static work as `20576b58f4b`. This next packet is a source
proposal, with B held unchanged during root's qualification. It uses the same
single plane, logical transfer ABI, initialized named bindings and native-entry
exception. It adds no frame field, tail-call parameter or addressable record.

The pure interface is `_PyFrame_ReadHome(frame, const PyValue *home) ->
PyValue`: test the physical presence bit, load only when positive, otherwise
return canonical NULL. Its caller must own the actual frame/home partition.
It does not grant a required operand permission to be absent. Actual emitted
Storage reads must distinguish nullable controls/NULL-safe operations from
required values. A required value needs a positive producer/read capability
or a missing-input error before dereference through the existing error-flush
protocol. The latter is an actual new error edge with a receipt/checkpoint,
not an untracked jump around temporary cleanup. Do not infer either role
from logical NO_HOME, an opcode name or a successful compiler census.

| Actual read class | Required port before removing fills |
| --- | --- |
| `IRStack.load/pop`, `IR_VIEW`, folded input fields and scalar reloads | Read the actual Storage home through ReadHome. Reused C values need no second home read. Borrowed locals retain dynamic boundness and TAKE-as-DUP. Required projections must reject absence before object/type dereference; nullable reads synthesize NULL. |
| POP_TOP, COPY/SWAP and other accepted initially-NULL transport | Preserve the existing harmless NULL behavior through declared NULL-safe transport. Moves of absence publish logical NULL without reading or copying stale words. A required consumer after that transport still needs its own check. |
| CALL/EX windows, exact argument moves, binder/provider and init adapters | Keep explicit self/kwargs roles: an omitted hole is never read or materialized. Actual argument values require readable inputs. Child overlap belongs to the child only after the real handoff; native/external initialized ranges retain their independent range authority. |
| FOR/SEND/CLEANUP_THROW and completion helpers | Replace raw nullable cursor/sent/exception and displaced-recipient loads with the matching typed read. Retained receiver, tagged-index and resume roles stay distinct. `complete_step_impl` must snapshot all authorized inputs before detaching the window, including A/input aliases. |
| Checked array TAKE and builder/borrowed-array helper inputs | TAKE uses absence-aware load plus detach. Only a callee that truly needs raw nullable cells gets a bounded declared-range normalization immediately before its ordinary escape: write physical NULL only in absent nullable cells, with no bit publication. Required elements get checks, not blanket normalization. Native initialized workspaces need neither. |
| Result vacancy checks and known-home debug assertions | Test the presence bit for scratch vacancy. Do not read the old scratch word to verify it is NULL or equal. Named-local replacement may still load its initialized old value. A known NULL C value does not justify a raw old-home identity assertion. |
| Local-proxy, debugger, redirect plans and line/monitor recovery | Named bindings stay direct. Every scratch NULL test, saved operand, restore action and diagnostic value must use a checked selected read or ReadHome with its real observer context/extent. Rebuild no storage shape from a stale PC. |
| Delegated receiver and remote values | Preserve the finite A/D protocol and actual state/read claim. Read the receiver only with positive presence; an absent required D is an invalid protocol, not a synthesized object. Remote layout must carry enough bitmap addressing information for the same decision before reading target bytes. |
| Copy/traverse/retirement and error scopes | Existing positive iteration remains. Remove destination gap fills only once zero-bit cells are never loaded by these consumers or subsequent execution. Dynamic L remains initialized. Current/error/captured views retain real clipped extents and allocation pins. |
| Native interpreter exit and external prefixes | Keep their exact initialized-storage contracts. Do not pass a NULL-code/native frame to a universal bitmap fallback. Pending external unpack values remain raw readable under their explicit prefix until callback-free frame-bit commit. |

TakeKnown must stop asserting a raw word matches a known NULL value when its
bit is zero. Take/Move/Vacate must clear authority before callbacks; they need
not store physical NULL into scratch once all readers use this policy. A NULL
write to a named binding still writes initialized NULL and preserves language
boundness. Positive writes still store the canonical value before publishing
presence. The debug full-N NULL/tag oracle must become initialized-L plus
positive-home checks; poison-zero tests replace its obsolete zero-word read.
The retained `_PyFrame_Copy` comment about a future M4 fill removal must be
changed with this packet to describe this actual reader policy.

The concrete work removed is fresh-chunk/frontier scratch filling, destination
scratch gap filling, scratch NULL stores on detach/vacate and physical-NULL
pop assertions. This does not remove bitmap clearing or named-local NULL
initialization. Each scalar reload adds one bitmap-word load, physical-bit
address/mask work and a conditional value load; a required unchecked input
also needs an absence error path. Folded local inputs using the same primitive
pay that bit test too unless actual local Storage provenance selects a direct
initialized read. Nullable pointer ranges add a bounded word walk and stores
only for absent cells, possibly repeated at separate helper boundaries. There
is no wrapper hop, native buffer copy or per-item reference increment by
default. Native dumps must measure live-register/spill costs after lowering;
these costs cannot be inferred from unchanged frame bytes alone.

The parsed source census has 27 used array-input uops. Outside CALL/init and
their guards/monitors, they are BUILD_INTERPOLATION, LIST/MAP/SET/SLICE/STRING/
TUPLE, RAISE_VARARGS, RERAISE, the special-method pair and `_POP_TOP_OPARG`.
This is a source inventory, not a generated-reader count. Container elements
are required language values: tuple/list helpers explicitly assert that
contract, so filling absent elements with NULL would be wrong. Validate a
required contiguous range using bounded bitmap-word masks before passing its
raw pointer; its cost is a word walk, not a per-element NULL store. RERAISE
reads a required lasti index at the actual endpoint, not its whole declared
passthrough window. `_POP_TOP_OPARG` instead needs an actual ownership drain.
The census found it still uses raw CloseStack, which NULLs words without
clearing B bits before callbacks; the immediate shared checked-drain packet
repairs that correctness gap independently of stale-storage removal.

Implement scalar and actual finite/helper readers, observer/remote/debug ports,
and poison controls before removing any fill. Required tests include accepted
initially-NULL raw transport, nullable controls, required-absence rejection,
array zero/partial-failure, local output aliases, SEND recipient aliases,
CALL virtual holes, overlap/waiting copies, resumed and delegated frames,
redirect/traceback/monitor faults and supported remote observation. Root owns
generation, both-role execution, retained native evidence and comparison.

The execution-reader source is now **work in progress in P**, based on the
qualified B `0c47a4616ef` presence/drain checkpoint. Root reports 27 focused
ownership/provider/writer cases and 510 GC/lifecycle integration cases passing
in each native role there. Those results do not validate the new stale packet.

The draft adds input-only `:nullable` to the shared StackItem schema and a
positive typed-use validator; default inputs are required. Actual Storage
loads use ReadHome. Required scalars branch on that single loaded value;
required contiguous helper ranges use bitmap-word checks before receiving a
raw pointer. Actual read failures use the emitter's existing error flush with
per-uop-occurrence identities. CALL also checks its actual mandatory ranges
before any input transfer, preserving omitted self/kwargs holes. These checks
can repeat across uops and at a CALL prologue; that traffic is charged.

GIL source removes the chunk scratch frontier, copy gaps and known scratch
detach/vacate word stores. Named bindings instead get explicit physical NULL
initialization at their real constructors, preserving overlap argument homes.
Generic cold TakeHome still writes NULL when it lacks a binding/scratch
capability; no claim eliminates every such store. Debug/shadow checks select
positive scratch and initialized bindings, with poison controls for absent
words. Remote delegated D retains its finite-role proof and checks target
presence before reading its word.

Free-threaded builds explicitly retain the old physical-NULL frontier, copy
fills and scratch commits at compile time. Unstopped FT traversal lacks the
claim needed for concurrent stale words; that redesign is a separate gate.
This adds no runtime branch to the selected GIL rival. Source peer review,
full reader closure, generation and native qualification remain pending.
