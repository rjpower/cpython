# Retained roots and handled-context transfers

Read-only amendment to `debugger-context-schema-draft.md` and the existing Plan
contract. Current source31 has no general retained-with or caught-exit recipe.
This does not admit runtime changes. Exact stopped-view work takes priority.

## Documented API and chosen support policy

The [data model's f_lineno contract](https://docs.python.org/3/reference/datamodel.html#frame.f_lineno)
documents changing the next source line from a trace function for the bottom
frame. The [Pdb jump command](https://docs.python.org/3/library/pdb.html#pdbcommand-jump)
explicitly limits allowable jumps, including examples involving for/finally.
Neither document specifies CPython's full accepted jump matrix or promises
reuse of one manager while landing in another manager's lexical body.

Inference and proposed policy: preserve useful transfers that retain the same
established protocol context, and refuse transfers requiring new protocol
owners, a substituted lexical manager or an unproved handled-context action.
Normal execution must retain Python's complete with/exception protocol. Debugger
scope abandonment is a separately documented choice, not ordinary unwinding.
Any intentional divergence from legacy tests requires explicit admission,
documented behavior and reviewed test ports; do not silently skip or weaken them.

## Actual remaining cases and the first capability

The source30 wide receipt contains eight with-related errors: across sync/async
with, between sync/async with, and leaving inner sync/async with while retaining
an outer for or with. These are not necessarily one stopped phase; collect the
actual callback scope before assigning them all a retained-root diagnosis.
The eight caught/finally cases similarly include same-context moves, ancestor
exits and exceptional-finally propagation; an EH-table tuple is not that context.

Start with **PRE transfer retaining one established with pair**, inherited
handled context, and no active saved-local obligation. The concrete witness is
a trace jump between two ordinary statements inside that same with body: one
enter, the selected body result, then one matching exit. This is an extension of
the existing Plan's preserve set. It does not require scope abandonment or
invented cleanup calls. Async suspension, caught links, nested-scope exits and
arbitrary root relocation stay out of the first implementation.

`test_jump_out_of_with_block_within_with_block` is a useful later policy witness:
its expected `[1,2,3,5,-2,6]` drops the inner scope without calling its exit and
retains the outer manager. That behavior is CPython's debugger choice, not a
documented normal with-protocol requirement. Supporting it needs explicit
scope-abandonment admission; the test's existence alone does not require it.

The ordinary `with_owner` census already identifies exit callable/self in
homes3/4, established by LOAD_SPECIAL and transported by SWAP; the enter call
uses a different window. Its exceptional handler preserves these same homes
for WITH_EXCEPT_START. Use actual encoded home relationships, not those example
indices or an EH floor, in the proof.

## One request-owned root capability

Derive a cold role/provenance view alongside existing context/save origins:

- A successful special-method extraction and enter sequence establishes a
  paired exit capability. Both actual owner homes and the optional-self shape
  belong to the pair; a callable alone does not establish it.
- COPY/SWAP preserve value lineage and actual ownership edges. Unknown writes,
  partial-error publication, exit consumption or reentry through its establishing
  sequence invalidate the corresponding availability proof.
- Require an unambiguous established pair at the stopped phase, a bounded
  no-consume/no-rebind/no-reentry slice to the target continuation, and compatible
  target requirements. Initially require matching physical homes, avoiding a new
  scratch-remapping action. Keep the existing outer FOR proof as an independent
  capability; do not infer iterator provenance from a surviving object alone.
- The target's callable and self requirements must be covered together. An
  optional NULL self needs both the actual NULL cell and an independently proved
  compatible target expectation. ABI NULL acceptance is insufficient.

CPython's `test_jump_between_with_blocks` expects `[1,2,5,-2]`, using the first
active manager after landing in the second body's source. That role substitution
is not promised by the documented API and is outside the proposed support
contract. Refuse it coherently unless separately admitted, rather than building
owner substitution merely to satisfy its old assertion. A final-IR derivation
for the supported same-context transfer must positively identify the
paired establishment/consumers. If it cannot distinguish a supported shape,
collect that concrete ambiguity before proposing validated compiler intent.

The Plan unions independently proved retained roots with existing context roots,
then detaches only surplus owners. It must not reuse a pre-site survival slice
for a committed POST row. No per-opcode setter branches, generic scratch
retention or new hot dispatch metadata is needed.

## Caught contexts require a link action, not just retained slots

Existing PUSH_EXC_INFO moves the previous handled exception into an owned home,
then installs a new owned exception in the active `_PyErr_StackItem`.
POP_EXCEPT consumes its exact input and restores that link. Same-context
transfers preserve both actual saved-previous roots and the handled link;
bare RAISE_VARARGS0 depends on that link despite having no explicit value input.

First caught exit should be **one ordinary active caught node to its inherited
ancestor**, with its saved-previous token available and target needs otherwise
covered. Do not initially enter another caught node, skip an exceptional-finally
propagation owner, or reconstruct an exception-as binding. The latter cleanup
needs a separately specified action/policy. For nested exits, only later compose
proven ancestor actions; equal homes or equal exception objects do not prove a
context generation.

The active `tstate->exc_info` may belong to the debugger callback's own except.
Writing it directly would change callback `sys.exception()` and its later
POP_EXCEPT could resurrect the target's abandoned context. Resolve instead the
**physical link representing the target context**:

1. Anchor the target's handled-context identity at the exact stopped phase.
2. Follow the active callback/frame/activation and exception-stack-item chain.
   If intervening Python caught contexts displaced it, identify the oldest
   corresponding saved-previous ownership home using their actual context
   provenance. The resolver must stop on an unsupported C context manipulation,
   ambiguous origin or generator/suspension link; an object-identity match alone
   is not authority.
3. Prove the selected link/home is live, outside active child-owned overlap and
   uniquely corresponds to the target context. This is a plan-time check, not a
   scan that writes through arbitrary frame homes.
4. Transfer the target's saved previous owner into that resolved link. Preserve
   the callback's current handled exception; its eventual POP_EXCEPT must restore
   the target's **new** inherited context. Keep previous-value NULL/None encoding
   and ownership conversion explicit for a PyValue home versus PyObject link.

Until this resolver has a concrete proof and callback-owned-exception test, keep
caught exits unavailable. Existing `probe-debugger-caught-callback.py` supplies
the target ZeroDivisionError / callback LookupError witness. Test inherited
non-None exceptions too; inherited does not mean no exception.

## Shared application order and admission gates

All fallible allocations and revalidation precede mutation. Capture displaced
owners, detach surplus/source owners, install restored named bindings and any
resolved handled link, then publish PC/continuation/generation and invalidate
old source-error recipes in one callback-free commit. Only afterward close
detached owners. Do not restore cached state after a close-time nested redirect;
its newer committed generation wins. Failed proof leaves all frame/link state
unchanged. A setter that then raises uses the committed target's error context.

First gate: the retained same-with-body witness, an outside-to-body missing-pair
negative, pair overwrite/reentry/NULL-alias negatives, and deferred-finalizer
reentry. Assert one matching enter/exit and exact selected body effects. Next
separately gate retained outer FOR, async executing phases and any admitted
scope-abandonment policy. Lexical-manager substitution remains unsupported.
For caught exits gate ordinary target/callback handled identities before/inside/
after the callback, callback-owned exception preservation, bare-reraise behavior,
missing-token refusal and exactly-once owner retirement. Yield/actual suspended
frames remain independent capabilities.

No implementation admission follows from this amendment. B owns Plan/context
consumers, H shared cold physical facts, A provider/exception-link ownership
review, and root concrete admission and native qualification.
