# Holistic interpreter and frame design review

2026-11-06; source baseline `6a0e4c12c17`; GCC only. This is a source/design
review, not a new performance result. Runtime and builds were read-only for
the reviewer. The implementation sequence below is bounded and does not
depend on a second compiler, IR re-encoding, or an exhaustive zero-box census.

## Assessment

The interpreter has a real frame stack, contiguous argument windows, exact-call
overlap, explicit exception homes and a single owned `PyValue` representation.
Those are sound foundations. The gap is not that someone forgot stack frames
or needs another value representation. Generic calls use a consuming provider
that must preserve the actual caller across fallible/reentrant binding; its
safe copies and retirement are visible costs. Transfer needs a cleaner provider
boundary for common shapes, rather than weakening root ownership.

The baseline IR is a frame-home interpreter, not optimizing SSA. That is an
appropriate separation. Its abstractions still leak cost: future-read liveness
is compiler-only; runtime observers require every slot valid; helper effects
and partial instruction phases are not a complete reusable exported contract;
some generator publication knowledge is special-cased rather than uniformly
tracked. These are concrete gaps. Neither a wide instruction nor an explicit
slot operand guarantees efficient generated machine code.

Correctness requires roots, ownership and release order, current observation
state, exception recovery and invalidation. It does not require duplicate
publication of an unchanged value, copying arguments through an entirely
non-escaping commit, or zeroing homes already proven NULL. Optimize those
redundancies while keeping one selected representation.

The reusable contract and exact initialization recipe are in
[call-and-frame-lifetimes.md](../../design/call-and-frame-lifetimes.md).

## Durable correction to earlier slowdown notes

At this same clean source HEAD, ordinary thread frame retirement already
asserts all temporaries NULL and clears only locals (`Python/frame.c:185`).
Generators/detached frames, frame copying and explicit exception unwind retain
their separate work. Earlier blanket descriptions of full-slot retirement in
the historical source/slowdown reviews were too broad. The measured frame-init
and cleanup shares remain valid; they do not prove redundant ordinary-return
temporary scanning. This correction is additive; retained evidence is unchanged.

## Call proposal review

[call-frame-proposal.md](call-frame-proposal.md) is acceptable in design after
including fresh empty `**kwargs` preparation, with the commit owner table and
requirements in the canonical lifetime contract. Logging's `debug` has both
`*args` and `**kwargs`; excluding the latter would miss the intended route.
Empty `*args` can use the immortal empty tuple; each `**kwargs` dict is fresh.
Do not promise logging coverage without checking actual path selection.

The route should be one guarded common binding protocol: fallible preparation,
final refreshed code/default/entry checks, then non-escaping transfer. Existing
generic binding handles keywords, nonempty varargs, unsupported hooks and stack
growth. It remains a provider with an explicit consumption contract. Do not
add parallel piles of shape flags, benchmark-specific opcodes or a speculative
all-purpose binder rewrite. Fallback is allowed only before commit.

The first implementation must show every owner transfer and preserve monitor
ordering, recursion/RESUME, failures and result destination handling. Mutable
defaults/code and introspection during preparation are useful adversarial
tests. Instrumented cases can conservatively retain the existing provider
until CALL provenance and event-once behavior are proved.

Source inspection also finds `call->effects` written but never read in this
baseline. It is not a dynamic proof that an effect happened or that a refresh
is unnecessary. Removing unused bookkeeping can be reviewed separately; it is
not a prerequisite or additional scope for this call packet.

## Frame initialization and cleanup decision

The [liveness proposal](frame-liveness-proposal.md) correctly rejects using
future-read sets as root/ownership maps. A dead value may still own a reference;
compiler compaction explicitly retains it. Its proposed aggregate slot counters
are safe diagnostic preparation if needed, but are not a prerequisite for the
smaller ownership-preserving overlap initialization change.

Prefer the existing overlap contract: after the supplied argument prefix, the
portion of child storage inside the caller's known frame extent is already
NULL. Initialize only the child's newly exposed tail and fill constants as
usual. The canonical recipe gives pointer bounds, required held-owner proof,
all current overlap producers, initialization order and generator exclusions.
This changes no GC/frame representation and adds no per-instruction checks.
It requires peer review of the actual helper diff and generated-code inspection;
a small saved memset may not beat its new bounds calculation.

Keep ordinary thread cleanup as it is. Do not release locals at last program
use; debugger/frame locals and reference-count finalization order matter.
Do not change suspended-generator or detached-frame scanning using a future-read
map. A general map belongs to a separate ownership-validity design only if
measurements justify its observer and instruction-phase complexity.

## Generator publication recipe

**Implementation refinement:** the subsequent [publication source review](publication-review.md)
rejects a distributed publication-state cache for this case. The selected
implementation makes retirement own synchronization/publication and removes
the five redundant DSL markers. The fact-tracking discussion below records
the earlier proposal, not the implementation selected for integration.

First prove a duplicate in actual generated output. Generic attribute completion
already sets `published_temporary_result` and suppresses the final temporary
store; changing an unexecuted branch is not a throughput improvement. The
source-proven specialized shape sequence instead flushes at `sync_sp`, flushes
again in `retire_published`, then finishes the destination store.

Track the single fact the generator needs: this unchanged result has already
been published canonically to this temporary destination. Reuse existing stack
publication state where it proves that fact. Copy the state along branches;
keep it at a join only if both paths agree on value and destination. Reset it
when the result is reassigned, the destination/home/frame changes, or an opaque
operation can invalidate that relationship. A helper may explicitly restore
the fact through a reviewed postcondition after refreshing.

Use the fact only to omit a redundant temporary publication, including a final
temporary store. Preserve first publication before receiver release; preserve
the final folded-local store after retirement and its displaced-owner ordering.
Do not skip frame/PC refresh after an actual callback. Unhandled error/deopt
paths retain their existing publication/cleanup. The first packet should cover
one proven attribute flow with generator tests and focused semantic tests,
not build a new general effect engine before implementation can proceed.

## Implementation order and exit checks

1. Implement the reviewed common positional bind/commit protocol, including
   fresh empty kwargs where guarded, with focused ownership and call tests.
2. Independently implement/review overlap initialization of only new tail
   storage; preserve non-overlap/generator initialization and constant owners.
3. Remove one proven generated duplicate publication using explicit state;
   check both temporary and folded-local destinations and reentrant retirement.
4. Root measures GCC instructions/workload and elapsed time for each parent/diff,
   then composes the accepted changes and checks the coherent integration.

Changes remain separate enough to identify wins or regressions. Targeted tests
belong to each packet; full suites belong to key integrations. Counters and
semantic-operation/dispatch census can refine priorities without blocking
these already bounded changes. Box counts remain diagnostic. No ctypes/LRU
held implementation or JIT redesign is reopened by this review.
