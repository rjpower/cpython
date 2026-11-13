# Created-generator allocation and copy authority

2026-11-12. Next bounded packet after the first copy/control component.
Root owns generation and native validation. The allocation correction is
integrated and tested at `8f6567840e7`. The certificate consumer is integrated
at `9f28c0fa16d`: both builds and 580 lifecycle tests per role pass; raw admission
fixtures need correction before that component is qualified. The next
[created lifecycle selector](created-selector-proposal.md) is source-reviewed
and awaits integration.

## Executing code determines allocation

RETURN_GENERATOR calls `_Py_MakeCoro(func)`, which reads mutable
`func->func_code` both to select generator/coroutine/async-generator kind and
to size the allocation in `make_gen`. The subsequent copy uses the frame's
independent executing-code pin. A PEP 523 hook can replace function code after
the frame is initialized and before delegating to the default evaluator;
generator-expression prefix callbacks provide another mutation interval.
Different flags can select the wrong container kind, and a smaller replacement
can underallocate the destination. This is a source finding, not a reproduced
failure claim.

Pass the executing `PyCodeObject *code` explicitly through
`_Py_MakeCoro(func, code)` and private `make_gen(type, func, code)`. The frame
pins code throughout allocation, origin construction and the callback-free
copy commit. Kind flags and `_PyFrame_NumSlotsForCodeObject` use that code;
name/qualname continue using function metadata. No additional code reference
or frame-header field is needed. Census finds one external source caller,
RETURN_GENERATOR, and three private calls in genobject.c (generator, async
generator and coroutine creation). Update the internal declaration and all
calls together; root regenerates the handler.

Use existing `run_staged_provider` with mode default for the regression. A
closure-free generator has many named locals so its executing allocation is
larger; the before observer replaces its function code with a closure-free,
small ordinary function. Assert the returned value remains a generator whose
gi_code is the original code, then resume it and check the original payload.
Run both object and value providers. This exercises the real predefault
mutation interval, kind selection, geometry and subsequent owner release
without a new hook API. Keep an additional generator-to-small-generator case
if needed to isolate geometry from kind. No allocator-fault fixture is needed
for this ownership mismatch.

## First actual created copy certificate

Do not infer empty scratch from co_flags or from an apparent compiler prefix.
The compiler emits RESUME/LOAD_FAST/GET_ITER before RETURN_GENERATOR for
generator expressions; the retained iterator is a real scratch owner.
Arbitrary admitted images can place RETURN_GENERATOR at other sites.

Add one emitter-produced CREATED_COPY ownership event at the actual copy
marker, before PC advance. The common semantic builder can set an
ALL_CREATED_EMPTY plan flag only when every physical copy site has a reached,
complete entry proof with no logical scratch values. Count unsupported,
unreached and observer-entry sites as uncovered; inline values and initialized
control values are obligations too. The flag applies to the whole immutable
executing image, so the created helper consumes one ready plan flag rather
than searching a phase table or guessing a PC. Bindings stay dynamic [0,L).
Price flag-only plan storage explicitly. Shared defaults must cover the
ALL_CREATED_EMPTY and ALL_RETURNS combinations, or the free rule must identify
allocated flag-only plans; today's zero-site/zero-phase test must not leak
such allocations.

The initial certificate covers normal function-created empty scratch. Audited
MAKE_CELL/COPY_FREE_VARS emission contracts must positively preserve scratch;
their names alone do not supply that fact. Generator expressions with a
GET_ITER survivor remain explicit NULL-control until their actual transport
events and created shape are certified. A later matched SUSPENDED view must
preserve the resume recipient and delegated-receiver roles; it is not implied
by this empty-created flag.

The real consumer must separate the certified empty common copy from the
outlined query/view control continuation; branching inside the existing
canary-bearing function would still charge its prologue. Root native evidence
prices that current continuation at 88 local plus 48 saved bytes, 136 below
entry. Do not add a speculative fast API without the actual flag consumer.
The certified copy supplies MUST_READABLE empty scratch only under the flag.
Otherwise the existing named NULL-control adapter remains an explicit
development limit. This removes source scratch loads and transfers for the
certified path; destination inactive NULL filling stays until its readers are
ported. It does not yet remove that destination cost or claim M4 authority.

## Ownership and sequencing

Frames owns the typed allocation contract, RETURN_GENERATOR consumer and
provider regression. IR owns the shared plan flag and semantic builder; GC's
current CALL work also edits ownership-event generation, so that event hunk
must be sequenced after its source freeze. No concurrent whole-file edits to
the builder or generator. The first copy/control packet must commit before
this one. GC/IR design peer, root freeze, source peer, then root generation and
focused both-role execution remain the gates.

## Concrete certificate packet

Root merged the allocator fix separately. The six-file frames packet now
counts every physical CREATED_COPY event and compares that count with stable
fixed-point empty proofs supplied by IR's semantic-step event. Unsupported,
unreachable and observer-entry sites cannot disappear from the denominator.
IR owns the exact emitter contract and checked local-only prefix capability;
frames owns certificate aggregation and its actual consumer.

Three shared flag-only plans cover ALL_RETURNS, ALL_CREATED_EMPTY and their
combination. This adds 32 static bytes over the former return-only default,
no per-code row/header growth or flag-only allocation. Existing zero-row
free/size accounting remains valid. Mixed plans keep the same row allocation.

The reached created helper now tests the immutable flag and moves dynamic
bindings through the common header-move primitive. It neither constructs a
view nor reads source scratch. Its uncovered branch calls a separate outlined
query/view continuation. Destination scratch still gets NULL stores. Public
frame-to-generator continues using its actual captured view, regardless of
the code's created flag. There is no new hot header, handler argument, record
or generator field.

The bounded cold probe calls the actual certified copy primitive with poisoned
inactive scratch, balanced counted/inline payloads and a real prepared code
plan; it executes no malformed image. Three fixture methods cover generator,
coroutine and async-generator certificates, cell/free-variable preservation
through real resume, and the genexpr iterator case remaining uncovered. IR
owns additional raw branch/unreachable/multi-site negatives. Host AST and
whitespace checks pass; no native job ran here. IR and GC source peers are GO.
The six-file packet is frozen over `81a76326039` (186+/29-); coordinator
generation/native/cost checks remain pending.
