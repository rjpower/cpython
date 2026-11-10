# Lookup proposal progress

2026-11-09. Scope: read-only source survey and bounded proposal, initial 30-minute
budget. Runtime inspected: frozen `4d666e1b482cb4983c7683f91c5922af14d0ab40` in
`execution-boundaries-20261108`; canonical documentation lives here.

Completed: read applicable guidance, CPython AI policy, current goal/process/
status/handoff, ranked next steps and source-backed micro addendum. Audited
classification, specialization, borrowed feedback/snapshots, emitted handlers,
shared-key insertion invalidation, dictionary replacement and first-probe API.

Findings: scalar class values explicitly decline the guarded instance path;
existing object handlers retire the receiver before acquiring the cached value;
shared-key addition invalidates the owning type before insertion; the current
first-probe dict helper only supports inline integers, not the measured string
key. Proposal reuses the existing non-descriptor family with explicit
`_PyValueGuarded` metadata and an acquire-before-retire transaction. Astra’s
first review rejected persistent `PyValueBorrow` heap storage; the revised
proposal shares the existing guarded-word type and creates a transient borrow
only after guards. Snapshots copy inert guarded metadata. It keeps current layout
admission and defers direct class loads, bucket hints and layout changes.

Output: [lookup proposal](lookup-proposal.md), ready for independent review.
No runtime edits, native runs, commits or reproducer were made. Root owns
builds, native verification, integration and timing. Root reports checkpoint
publication completed at `cb906f9bc52` (runtime39 tags).
Next: independent review settles payload typing, absence proof, mutation and
publication cases; implementation waits for Astra’s final peer approval and
the coordinator’s function-scoped file ownership assignment. Small-first ladder is generation/source inspection → release
and debug compile → focused semantic checks → post-review → integration → paired
performance. Scope cut remains scalar class-value admission plus the two
existing non-descriptor result transactions.

## Approved implementation

Astra approved the guarded-word revision in [proposal peer review](proposal-peer-review.md).
The coordinator released runtime edits after checkpoint publication. Producer
changes now exist in the runtime worktree; no generated files were rewritten by
this lane. `_PyValueGuarded` and its helpers moved to `pycore_value.h`; attr
feedback and handler-selected polymorphic snapshots store inert guarded words.
Unshadowed inline scalars share the existing non-descriptor admission and both
handlers acquire/publish the canonical result before receiver retirement.
Attribute feedback remains 40 bytes and poly rows' aggregate remains 88 bytes,
checked with compile-time assertions. The generator rejects the guarded operand
with the wrong width or a non-attribute feedback layout.

New runtime coverage is in `Lib/test/test_ir_attr_cache.py`: canonical/object
loads, inheritance/MRO/class mutations, instance shadows, shared-key insertion
on another instance, materialized/replaced dictionaries, descriptors/hooks and
exceptions, mutable descriptor-type exclusion, polymorphic observations,
reentrant feedback rewrite and eligible list-valued retirement witnesses. The
latter assert warmed NONDESCRIPTOR handlers and cover both receiver layouts,
return/local/discard/aliased destinations and class deletion/replacement.

Host-only codegen checks pass all four tests in the separate
`Lib/test/test_ir_attr_cache_codegen.py`, including in-memory generation of the
actual shared producer source. Both new modules parse; `git diff --check` is
clean. Host invocation uses `UV_CACHE_DIR=/home/power/.cache/uv uv run
--no-project python -B`, loads `Lib/test` as the `test` package via
`importlib.util.spec_from_file_location`, and runs the codegen unittest module.
Putting this checkout's whole `Lib` on host `sys.path` instead produced an SRE
version mismatch and was corrected; no runtime failure is inferred from it.
No native execution, build, checked-in regeneration or commit was done by this
lane. Root owns regeneration, release/debug compilation, focused runtime checks,
Astra actual-diff review, integration and measurement. Runtime edits are ready
for those checks; passing host codegen checks do not establish runtime admission.

## Confirmed pre-change lifetime witness

Root's retained `E/object-call-20261109/lookup-lifetime-before.json` records
the eligible list-valued class-attribute/finalizer witness against both frozen39
release and upstream. **Both processes exit with SIGSEGV (`returncode: -11`).**
Frozen39's flushed IR identifies `LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES` before
the crash. Upstream does not pass this witness. The script begins with the
non-slotted receiver, so these interrupted runs do not establish a result for
the later no-dictionary case.

The class owns an eligible list containing a weak-referenceable payload; the
receiver finalizer deletes that class attribute. This is a native lifetime-safety
failure on an actual warmed cache route. Acquiring the selected result before
receiver retirement protects it when finalization occurs; the requirement does
not depend on Python guaranteeing immediate finalization or matching CPython's
exact destruction schedule. Root is regenerating/building the approved change;
post-change native results and integration remain pending. This lane makes no
further runtime edits while those checks run.

Root's first release compile found that the new non-descriptor retirement path
reloads `this_instr` while its declaration was still const. The generator now
uses the checked result-publication contract to select mutable instruction
state. The actual-handler codegen test requires that declaration and its
post-retirement reload for both handlers. All four host codegen tests pass;
root must regenerate and retry native compilation. No generated output was
edited directly.

Astra noted that the first declaration fix covered only non-target handlers.
With root's approval after the release build stopped, the family-head branch
now uses the same checked publication fact. A synthetic family outside the
handler-name whitelist verifies mutable `next_instr - 1` instruction state and
post-retirement reload. All five lookup host codegen tests pass. Root retains
regeneration and native validation ownership.
