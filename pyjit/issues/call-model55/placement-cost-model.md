# Placement cost model for M4 selection

2026-11-12. This prices concrete layouts against the current GIL ABI, not a
timing result. Source obligations and readers are enumerated in
[allocation-alternatives.md](allocation-alternatives.md). All variants below
use the same owner recipes, Python binding semantics and retained native
conservative policy. A placement comparison must not silently change them.

## Units and common work

One PyValue or pointer is 8 bytes. The header extent is 112 bytes (14 cells);
the C struct is 120 because it declares its first owned home. Heap raw extent
for N homes is therefore `112+8N`, before its enclosing object/GC header and
allocator rounding. Let L be local homes, S normalized physical scratch homes,
N=L+S, A transferred arguments including actual self, and K constant-home
installs. B is the child's argument/local base and P the old physical value top,
in cell units. These formulas assume a same-chunk thread call; chunk misses,
heap callers and generic binding are separate paths.

Let Q be unbound local homes not directly installed by binding/constants and
not proved clean. Q is a count of actual dirty/unproved destinations, not
necessarily L-A-K (some constants/arguments overlap or locals get defaults).
U is the newly exposed uncertain value tail in the current frontier scheme.
The chosen design cannot price all warm entries as U=N: selected54 already
skips fills on previously initialized/cleared reuse.

Common ordinary work includes header construction, independent code pin,
constant installs, count changes for actual owners, parent continuation, and
map publication. K home stores and mortal constant increments remain in every
layout. A heap caller moves A homes: A reads, A destination writes and A source
NULL stores (24A bytes of explicit cell traffic), without a count roundtrip.
Generic kwargs/default/cell binding keeps its allocations and callback stages.
Thread exact overlap has zero such argument traffic. These are source memory
operations; compiler batching, cache effects and emitted instructions require
native evidence.

## Four concrete candidates

| Candidate | Header and entry bookkeeping | Same-chunk end and extra reservation | Local state and exit |
| --- | --- | --- | --- |
| Shared base, NULL locals, mapped scratch | 112 bytes; no new base/bitmap store. Fill Q local homes: Q writes, 8Q bytes. | `max(P,B+L+S)`, reference end. | Valid local words stay readable. Current one-pass retirement reads L and writes L NULLs (16L bytes) plus closes. Maps avoid dead scratch reads; any retained cleanliness stores are charged separately. |
| Same arena, explicit second scratch base | 120 bytes; one extra pointer store per frame. Fill the same Q local homes. | Scratch starts `max(P,B+L)`; end `max(P,B+L)+S`. Extra D=`min(S,max(0,P-(B+L)))` cells, or 8D bytes, plus 8 header bytes. | Same local work as first candidate. Two spans for copy/walk/cleanup; mixed logical addresses require selection or distinct generated operand domains. |
| Shared base, 64-bit inline/sidelong local validity | 120 bytes; new 8-byte union is inline bits for L<=64, pointer otherwise. Set W=ceil(L/64), W>=1. Small case: one initial word store. Large case: W zero-word stores plus one pointer store and 8W side bytes. | Reference value end, plus bitmap storage for L>64 and an explicitly restored side-reservation extent. | No Q value NULL stores solely for unboundness. Batch supplied-prefix bits where possible; each later bind/delete still updates validity. Retire/copy only valid homes, invalidate before callback close. Heap/observer readers port to bits first. |
| Two independent arenas, NULL locals, copied arguments | A concrete frame adds scratch_base and saved secondary top: 128 bytes. Locals retain the existing arena; scratch uses a second thread arena with top/limit/chunk/cache state. | Local and scratch reservations commit together; both remain reserved through last callback. Capacity/growth tests cover both; two saved frontiers restore. | Common thread path now pays 24A bytes of argument move traffic unless producer staging is redesigned. Q fills remain unless the local arena separately maintains clean proof. |

The fourth row fixes an implementation rather than claiming a universal
minimum: other two-arena designs may need another saved limit or pointer. A
second arena cannot derive pop authority from code after the final code close;
the reservation state must be explicit. A combined precheck can share a branch,
but cannot make two independent capacity conditions or failure rollback vanish.

The second row preserves zero-copy argument transfer by giving callee locals
the caller's argument prefix. It does not make the remaining local tail clean:
it still aliases former caller scratch. Its extra reservation follows exactly
from subtracting the two end equations. Putting scratch at B+L removes D, but
then restores the shared contiguous placement rather than this split.

The validity row's side storage must be coallocated/reserved with a stored
extent or have a cold allocation. A raw side allocation per large-L frame is
an additional allocator cost, not free bitmap bytes. The inline union requires
access to pinned code while deciding inline versus side; pop uses saved
reservation state after code closure. A design that instead keeps the bitmap
at an address derivable from base may avoid the pointer, but must specify
header-crossing, overlap, heap-copy and code-independent pop geometry before
claiming the saved eight bytes. This table does not grant that optimization.

The two spare GIL bytes at 110 are already proposed for phase publication.
They cannot contain a general 64-bit validity word. Bound inline and immortal
values need validity too; the mortal-owner bitmap is not a substitute. A valid
cell home and a bound cell payload are separate states. Pending empty kwargs
and observer-created bindings also need their actual binding protocol.

## Worked geometry and entry counts

These are arithmetic examples, not measured workload frequencies. In each,
all local homes outside the disjoint A arguments and K constant homes are
initially unbound and unproved clean, so Q=L-A-K. No defaults/cells are installed.

| Input | Shared NULL-local entry | Second scratch base | Explicit validity |
| --- | --- | --- | --- |
| L=8, S=16, A=3, K=1, P-B=24 | Q=4: four value writes (32 bytes). Raw heap extent 304 bytes. | D=16: 128 extra arena bytes plus 8 header bytes; same four local writes. Raw two-span heap extent 312 bytes. | One validity zero store (8 bytes), plus prefix/constant validity updates. Header 120; raw extent 312 bytes, no side bitmap. |
| L=96, S=128, A=4, K=4, P-B=160 | Q=88: 88 writes (704 bytes). Raw heap extent 1,904 bytes. | D=64: 512 extra arena bytes plus 8 header bytes; same 88 writes. Raw heap extent 1,912 bytes. | Two zero-word stores (16 bytes), one side-pointer store, and binding-bit updates. Side bitmap 16 bytes; raw extent including it 1,928 bytes. |

Bitmap zero stores can be fewer than Q value stores, but that comparison alone
omits validity update/read instructions and all reader ports. Conversely,
charging one bit operation per supplied argument is too pessimistic when a
contiguous prefix can be installed in whole words. Native tests must report
actual batching and whether an address/bitmap base occupies another register.

## Clean reuse and staging alternatives

A single clean frontier needs at least one pointer (8 bytes per chunk) if it
cannot share the initialized frontier. With stale scratch those facts diverge:
initialized does not imply NULL. Arbitrary holes force a conservative frontier
to discard useful clean suffixes and pay Q later. A concrete interval alternative
uses sorted half-open uintptr pairs: 16 bytes per retained interval plus count/
capacity/storage metadata. For r intervals its payload is 16r bytes, and a
simple vector has O(r) query/update/move work. This is a costable alternative,
not a claim that one free range check handles fragmented reuse. Publishing a
clean interval must follow the last possible callback touching that storage.
Retaining exit NULL stores to rebuild clean proof moves work; it does not
remove it.

Producing call arguments directly in a future clean-local reservation can
avoid the fourth row's A moves. It changes the argument evaluator, not just
frame placement: partial reservations survive nested calls, yield, exceptions,
dynamic EX expansion and allocation failure, with a staged initialized-prefix
root and restored frontier. Callable/self, surviving caller scratch and the
new bank must have disjoint authority. Until that producer/consumer protocol
exists, the copied-argument row is the honest independent-arena baseline.

Heap escape currently moves N cells with N source reads, N destination writes
and N source NULL stores: 24N bytes. With static scratch authority and dynamic
local validity, let T be the number of authorized initialized homes copied.
The NULL-local candidate can reduce value traffic toward 24T, plus range metadata.
A validity design may instead copy T reads and T destination writes (16T bytes)
and invalidate source bits, if its lifecycle proves no source NULL stores are
needed; charge those bitmap updates separately. Each candidate must establish
destination state before publication and preserve source
authority through callbacks. A split layout uses two extents rather than
pretending independent arrays are contiguous. Direct heap construction remains
a separate initialization path.

## Selection recommendation and remaining measurements

Use shared placement with NULL local authority as the first M4 control. It
preserves the measured header, direct addressing, zero-copy argument overlap
and current local readers while the event graph proves scratch authority. It
still pays actual Q writes when stale scratch becomes unbound locals. Do not
claim that store saving before measuring Q and the retained entry/exit work.

The useful competing placement is shared storage with explicit dynamic local
validity, since it attacks those Q writes directly. Compare it once the full
locals/proxy/cell/kwargs/copy/GC/debugger ports exist. The simple second-base
layout pays eight header bytes and 8D extra reservation while retaining Q; it
has no source-level local-initialization advantage to justify selecting it
first. Clean-local argument staging is a larger separate producer experiment.

Root should measure L/S/A/K and dirty Q distributions, warm U versus chunk/cache
miss fills, D at actual call sites, callback-visible staged depth, large-L
bitmap frequency, heap-copy T/N, validity/address instructions, spills, native
stack and code/data size. Keep exact thread, heap caller, generic binder,
unwind, escape and cold/cache paths separate. Compare complete implementations
on the same admitted images/workloads; M5 must retain the selected M4 placement.
This cost model neither predicts timing from byte counts nor selects an
unimplemented validity representation for production.

GC's bounded source/arithmetic peer reports GO. Root selects shared storage
with initialized NULL named locals as the M4 reference. Its costs are the
112-byte header, actual Q admission writes, current overlap/reservation and
NULL local readers described above. Dynamic local validity remains an optional
bounded alternative; splitting bases does not eliminate its semantic work.
