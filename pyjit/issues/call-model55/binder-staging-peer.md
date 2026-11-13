# Generic binder staging: GC interface peer

2026-11-12 independent review of
[binder-staging-proposal.md](binder-staging-proposal.md). GO for the bounded
fallback/exported-wrapper implementation. No complete M3 or M4 claim follows.

The private constructor takes mandatory caller-owned record storage. The
initialized NULL/value range and typed header mask are linked before the first
binding callback. Successful construction returns a still-linked record to its
outer owner; fallback EX vector/borrow/callargs/kwargs cleanup remains inside
that lifetime. Publication makes the child current before callback-free record
disarm/pop. Failure transfers to the existing M2 retirement route without a
callback between staged pop and its first reached slow-root publication.
Installed child owners retain unique frame/heap edge authority; external root
marking must never subtract these as extra container edges.

The full initialized child range includes constant binding homes and any
partially installed arguments/defaults/kwargs. Geometry belongs to the selected
independent code pin. This is NULL-control coverage rather than a static phase
proof, and it requires no per-argument root stores or duplicated owner counts.
Keep parent call-window ownership and revalidation until their existing commit;
do not cache an arena home pointer across a callback that can redirect its owner.

Outlined fallback and noinline exported wrappers keep the large record out of
exact-call handlers. Wrapper-local scope includes its own cleanup but cannot
prove an unknown caller's post-return activation gap. The proposal explicitly
retains PEP523/custom provider/outer eval-entry coverage as an open gate before
arena scanner removal. Numeric-box callback-free cleanup claims at the init
handoff need their concrete producer proof, not a general assumption that native
cleanup cannot reenter.

Root owns retirement-reason hook lines; frames owns binder regions and tests.
The proposed equality/nested binding, partial failure, local/code mutation and
EX cleanup fixtures must inspect precise transition IDs, not merely survival
under conservative scanning. Root owns generation, execution and the price of
record stores, wrapper call boundaries, native stack growth and ABI traffic.

Source postreview GO: reviewed `Python/ceval.c` and five focused methods in the
frames lane, two files218+/9-. Every successful constructor initializes stage
storage before access; failed preconstruction paths leave it unused. Fallback
sets its linked flag only for a returned child, keeps it through refresh and
all cleanup, and pops after child-current publication or immediately before
M2 error retirement. Wrappers retain it through independent code decrement.
The tests inspect actual precise IDs during keyword equality, previous keyword
installation, nested binding, partial failure and selected-code replacement;
caller-local mutation and chain exhaustion are asserted. Explicit EX cleanup
source lifetime is reviewed, but its destructor-specific fixture remains an
open test gate rather than an invented success claim. No native execution was
performed by this peer. Root still owns retirement-reason integration.
