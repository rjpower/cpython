# Independent review: mechanisms and existing stats

2026-11-08. Source reviewed against runtime `0953757f4e7` and the architectural
alternatives review. This is a design/evidence review, not approval to implement
or a selection of a batch. No build or native workload was run.

## Blocking proofs before implementation

**M1, sequential owner retirement.** The shared multi-input contract must check
whether each owner may deallocate immediately before its own close, after the
preceding closes. Two aliased mortal owners at refcount two both appear safe
if checked together, but the second close deallocates. The existing generator
`retire_published` (`ir_generator.py:2398–2403`) already does the sequential
check; reuse that behavior and preserve key-before-container order from
`value_reader_complete` (`ceval.c:1827`). The proposed inline-int key and
dict cannot alias as objects, but a general multi-input abstraction cannot
assume that. Publication of a temporary result before a possible callback,
local destination ordering, and restoration on generic failure also need a
concrete generated-edge proof. These are design obligations, not a finding
that the existing implementation is wrong.

**M2, observer and destination proof.** `bytecodes.c:_RETURN_VALUE` unlinks and
clears the child before deriving the caller continuation; destructors can
change instrumentation. `IR_STORE_RETURN` publishes through
`_PyFrame_SetReturnValue`, which closes a displaced destination and handles
`IR_NO_DST`. A bypass must prove a matching ordinary POP, a temporary return
slot that is actually NULL, a safe result close, and the complete current
instrumentation/observer state *after* child clearing; otherwise use the
existing continuation. The proposal identifies these requirements, but does
not yet specify an exhaustive epoch/wrapper, pending-event and trace/monitor
predicate. `tstate->tracing` alone is insufficient. Do not implement until
the predicate and its failed-instrumentation and changed-PC behavior are
reviewed. A sole no-deallocation test does not prove observer absence.

**M3, complete mutation and comparison proof.** `dk_version` is invalidated at
several sites, and `_PyDictKeys_GetVersionForCurrentState` assigns a nonzero
interpreter version (`dictobject.c:7680`); that alone does not establish the
required invariant for every structural mutation, copy, resize, reuse, version
exhaustion and table shape. Audit all relevant writes and lifetime/version
scope before using it as a certificate. The scan must preserve probe sequence,
never invoke or suppress an equality callback, and distinguish changed values
from changed key layout. In particular, a cached final-key identity with no
proof of the preceding collision prefix is unsound. The proposal explicitly
recognizes this; proof remains open. Diagnostic shadow scans must avoid
callbacks and perturbation and be compared with real lookup behavior.

## Coverage and abstractions

M1 is a real competitor to call-entry simplification: the warmed inner
`BINARY_OP_SUBSCR_DICT` reaches an out-of-line provider and completion even
though the shape and method handlers already have more specialized retirement.
Its narrow first-probe inline-int proof and conditional retirement could remove
some of that work on each cache hit. Count actual hits and inspect release GCC
before claiming coverage or cost. A dictionary-owned hit primitive plus the
existing checked first-output/ordered-retirement generator contract is the
clean shared boundary; no handler-specific phase exceptions or duplicate probe
implementation. The proposed fast path must preserve mortal result acquisition
and avoid copying value or entry pointers beyond their guaranteed lifetime.

M2 is also a real competitor: it targets the caller continuation after either
call-entry strategy. The compiler preserves POP for line-observation reasons;
source shows ordinary POP publishes its PC, consumes its home and closes the
owner. A shared return continuation is plausible if it leaves the real POP
and fallback available. It cannot claim ten eliminated dispatches per loop
until the static/dynamic discrepancy below is resolved. Its guard itself,
result publication, callback boundaries and code footprint may erase the gain.
Treat call-return and native-call continuations separately until their different
monitoring/retirement protocols are proven equivalent.

M3 is a structurally distinct but overlapping lookup alternative, not an
independent additive benefit over M1: both can remove portions of the same
dict operation. It is a credible competitor only if safe longer probe prefixes
are common and outweigh the keys-version, feedback, footprint and maintenance
costs. If first probe already hits, a feedback cache may be redundant. Reuse
dictionary-owned probing and the same canonical completion contract, with
explicitly owned metadata and coherent invalidation, rather than a second
interpreter probe implementation or cached unowned entry pointers.

Cheapest discrimination: first reconcile per-site warmed IR with dynamic
counts. Then gather narrowly scoped stats-only first-probe identity versus
safe longer prefix versus callback-risk/miss, plus retirement outcomes; use
existing handler counts as denominators and inspect the *release* GCC path.
For M2 count return continuation site/destination, observer state after child
clear and result deallocation risk, with virtual boundaries separated from
actual dispatches. For call entry use reconciled exact, overlap, fallback and
native routes from the companion diagnostics proposal. Compare each with
call/non-call controls; these counts describe opportunity, not speedup.

## Independent audit of retained stats

Read raw `.txt` for 0, 1000 and 2000 loops and independently parsed the union
of execution-count keys. The sums are **57, 161057, 322057** respectively;
every handler count scales exactly 2x after the empty interval is subtracted.
Frames pushed are **3, 20003, 40003**; inlined Python calls have the same
counts. POP_TOP is **4, 9004, 18004**, and NOP is **0, 10000, 20000**. Thus
`scaling-summary.json` correctly reports 161 dispatches, 20 frames, 9 POP_TOP
and 10 NOP per loop for this global stats interval. Other reported rows match
the raw data. These are process-wide handler counts, not site counts or a
release-build measurement; exact scaling is not proof of route identity or
zero harness contamination.

The capture script warms 20,000 loops with stats off, clears, enables, executes
`module.run(loops)`, dumps while on and disables. All three commands exit zero;
stderr is empty, the stream assertion passes, and before/after receipts record
matching source HEAD and hashes. Stdout records CPU 15, JIT disabled and absence
of the method-admission API. These are useful diagnostic controls; assert/API
absence and matching recorded hashes are not independently measured route
frequencies or timing results. Per-pair counters can cross frame transitions.

The release warmed IR shows ten static retained POP sites while stats record
nine dynamic POPs per loop and ten NOPs. Static sites need not all execute,
and the stats binary may have different warmed handler selection; the present
records do not establish which explanation applies. Capture/compare each
actual site and handler in the *stats* process after the same warmup, including
call destination and NOP transitions, or obtain a narrowly filtered site
count. Preserve both source/build identities and reconcile totals before using
ten as a dynamic opportunity. Do not infer NOP is the missing POP without
that evidence.
