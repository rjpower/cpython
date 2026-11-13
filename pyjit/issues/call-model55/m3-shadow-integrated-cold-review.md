# Integrated frames and provider cold review

2026-11-12. Source review covers the retirement/shadow composition at
`a95bcbbf836`, the subsequent fast-close component `d5c1411fa0c`, and full
provider composition `de95475accb`. GO for these bounded ownership contracts;
this does not admit M4 map authority or a placement/performance winner. No
candidate execution, generation or compilation was performed in this review.

## Entry, pins and placement

Executing code and function are independently counted typed pins. Changing
`function.__code__` during a binding callback cannot change the selected
geometry or executing descriptor. Generic construction selects and pins code
before binding, initializes the whole reserved NULL domain, and stages it
before defaults/keyword/cell work can reenter. A mandatory linked/success flag
prevents failed prelink preparation from reading an uninitialized C record.
Constant homes still install explicitly; `_PyFrame_Initialize` adds no full
local/scratch fill loop.

Exact thread calls retain their overlapping argument base and transfer counts
without argument copies. The physical end is still `max(P,B+L+S)`, and the
uncertain initialized tail is extended only when needed. Heap callers retain
their separate move-and-source-NULL path. Neither typed pins nor the shadow
pointer changes this geometry. Pending empty kwargs remain a virtual binding;
physical NULL is not a general test for Python local boundness.

At `a95`, exported construction wrappers ended their stage before external
evaluation: that was an explicit open gate. `de95475` closes it with an outer
provider scope. Its live STAGED record covers all initialized homes plus current
nullable locals/frame-object headers through provider cleanup. The default
evaluator first publishes the actual child, then disarms the matching token.
An empty node stays linked through hook postprocessing but holds no frame
pointer or authority. Nested delegation cannot later inspect freed frame
storage. Custom activation/retirement rejects a missing token before a frame
dereference. A hook that abandons its input has one outer-owner disposition.

Recursion-entry failure disarms to M2 retirement without a callback or poll in
the transfer gap. Constructor failure has the same gapless staged-to-retiring
handoff. Results and saved errors after detachment are counted native owners
under the explicitly retained native conservative policy. No global C-value
head was added. The internal object-provider export and the helper's balanced
argument DUP/CLOSE correct test-extension boundaries without changing these
production ownership rules.

The specialized init-shim remains a separate open gate at this reviewed head.
Its revised source proposal is reviewed in `init-shim-peer.md`; no claim here
includes that later implementation. In particular, an active thread trampoline
needs unconditional NULL borrowed environment headers, and consuming init
binding needs both child/shim lifetime and parent-redirect protection.

## Retirement and shadow proof

The first possible frame-object close, local close, final code close and
generator exception close all remain within the outlined retirement record.
The code pin is fixed last; its constants may run finalizers. The arena
reservation is retained until the last callback completes. A copied escaped
frame becomes the authority for transferred headers/homes, while the source's
independent code and actual generator exception remain independently covered.
Caller result publication occurs after child retirement, with callback-current
local state reloaded. Heap `frame.clear()` retains its one-pass callback-write
semantics through the existing locals clearer.

The normal retirement certificate is deliberately narrower than a general
phase plan. Its worklist intersects definite initialized/NULL facts, unions
possible owners, and separately meets predecessor completeness. Error and
observer-recovery entries poison completeness; opaque transfers cannot regain
it. EXACT requires every scratch word to be canonical NULL, including inline
and immortal leftovers, not merely absence of mortal edges. Dynamic local
boundness remains outside that certificate.

The cold query accepts only literal NORMAL_RETURN, checks image identity,
aligned/in-range canonical site and actual EXIT before default/sparse lookup,
and runs after record push but before cursor mutation. No code geometry is
decoded after code detach: later test queries read stored reason/status only.
UNCOVERED remains explicit for error, heap and staged reasons. Map build failure
does not publish a half-built exec; sparse storage is freed once and included
in code size. These choices preserve exception behavior and descriptor lifetime.

Code-exec grows from 32 to 40 bytes by one plan pointer. A sparse certificate
costs `4+4R` bytes for R sites; no-plan and shared-all-return cases have no
persistent side allocation. The existing words-to-local-count displacement
remains -12. Cold work is still real: for n words, B blocks and
W=max(1,ceil(S/64)), its principal arrays cost approximately
`4n+16B+24(B+1)W` bytes, excluding allocator overhead/parser storage. Admission
and first exec construction must be priced separately from hot lookup.

## Recorded native costs and validation limits

Evidence is under `build/pyjit-link-wave1/call-model55`. The provider ELF and
five `m3-provider-native-*.txt` dumps correspond to the coordinator's full
provider build. Exact CALL reserves 0x40 (64) stack bytes after its frame-pointer
push; RETURN reserves 0x10 (16). Neither contains `handoff_staged_frame`, a
transition-head load, a map lookup or an automatic 40-byte root record. CALL
still shows the outlined initialized-tail call and its six saved/reloaded
values; heap-caller argument transfer remains a separate call. RETURN keeps the
detached result across clear and publishes afterward, with literal normal
reason traffic on the thread-clear path.

`_PyEval_Vector` reserves 0xc8 (200) bytes and saves rbp plus five registers;
its fixed frame therefore consumes 248 bytes below entry rsp, excluding the
return address. `_PyEval_VectorValue` reserves 0xa8 (168), with the same saves,
for 216 bytes. Their constructor call temporarily adds 32 bytes of stack
arguments/alignment. Each fixed frame contains a 64-byte small argument array,
40-byte stage, canary and other state/spills; these numbers are complete frame
costs, not the incremental cost of the record. No matched pre-provider wrapper
dump is available here to establish that delta. Large argument arrays use
PyMem and retain the selected native discovery policy.

The matcher is an outlined leaf without stack reservation. It walks the chain
by kind and explicit frame identity and disarms via remaining/header/frame
stores; cost is O(linked scopes). It is reached only by external evaluator
publication/failure and explicit custom handoffs, not inline child calls or
ordinary returns. Later shim metadata requires the separately reviewed
caller_staged exclusion.

The corrected `m3-fast-close-native-geometry.txt` records owned_slots at 0x70
(112), struct size 120 including the declared first home, transition size 40,
and exec size 40. The earlier shadow geometry command failed on obsolete field
names and is not geometry evidence. The fast-close dump shows one tag
classification, a count test and direct nonlast decrement, then home NULLing;
there is no second XCLOSE classification, call or automatic record. Closing
before NULL is confined to that callback-free proof; last-count and FT mortal
owners go to the rooted slow path.

Coordinator evidence at de95475 reports, separately per release/debug role:
eight precise provider/reader cases, 1,487 core cases, and 28 borrow-scope/
EvalCodeEx cases passing. These groups overlap and are not summed. The release
core command also named one nonexistent module; the corrected borrow-scope
group ran separately. Earlier a95's sole remote malformed-source-map fixture
failure is superseded by the corrected reader case, without relaxing admission.
The bounded tests establish reached records and nested cleanup behavior, not
complete precise arena coverage. Hot6 timing and full M4 coverage remain root
gates. Native stack/register/PyMem discovery is intentionally retained.
