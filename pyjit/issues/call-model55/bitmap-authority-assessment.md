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

Recommendation: select a single non-NULL occupancy plane as the next bounded
rival, subject to root and GC/IR interface freeze. It closes the actual
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
qualification remains GIL. Root selection and GC/IR peer are pending for this
replacement source scope.

GC's bounded policy peer gives GO to this assessment: positive counted bits
may select GIL counted roots/private drains after writer closure; occupancy is
a separate selection, with physical handoff and Publish/Vacate semantics still
required. No second plane or FT shortcut is implied.
