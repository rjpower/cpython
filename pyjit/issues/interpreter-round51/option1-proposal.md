# Option 1: retain the inline-values layout witness

Status: proposed; source unchanged. Runtime subtree is
`BASE/worktrees/round51-option1`, based on `bf810ec1a33`.

The existing exact, nonzero receiver type-version guard establishes the
inline-values displacement, yet validity and shape guards reload
`Py_TYPE(receiver)->tp_basicsize`. Cache this scalar layout witness once during
specialization and address inline values through receiver plus displacement.
Keep validity, shape, presence, descriptor and type-version guards unchanged.

Use the existing four-byte `_PyFbAttr._pad` as `values_offset`. A single checked
nomination helper accepts positive `tp_basicsize` representable in uint32 and
records the displacement. A single inline address helper takes a receiver and
that displacement; its contract requires a preceding exact type-version guard.
It performs no release-mode type metadata loads. Attribute slot offset remains
separate; check its full displacement before narrowing as well.

Apply this witness to `LOAD_ATTR_INSTANCE_VALUE`, shape loads (including the
generated local/temporary variant), shape stores/adds, instance-value stores,
and method/nondescriptor WITH_VALUES loads. The same helper serves validity
reads, shape reads and insertion-order/shape updates. Other attribute layouts
retain their present addressing. Cache operands use zero inline units, as the
existing shape operands do; this does not change instruction geometry.

Preserve ATTR/poly sizes of 48/120 bytes. In `_PyFbPayload`, reuse the native
pointer union as a scalar pair `{shape_to, values_offset}` for these handlers;
methods retain their distinct guarded `class_value`. The recorded handler is
the discriminator. `fb_row_of` copies the offset for precisely these handlers,
alongside shape_to only for STORE_ATTR_SHAPE_ADD. Whole-row copies, feedback
clones and resets retain their current behavior. Audit all snapshot consumers
and reconstructors, including debug/test readers; no pointer is treated as an
offset. Keep atomic validity reads and the existing shape-read semantics.

Layout proof: ordinary type mutation invalidates type versions. `__class__`
assignment changes the actual receiver type, so the guard fails; its supported
layout check also requires compatible layouts and detaches/materializes inline
dict storage before changing type (`Objects/typeobject.c`). `__bases__` changes
must retain compatible layouts and invalidate descendants. Public `__basicsize__`
is read-only. Direct unsupported extension mutation of a ready type's layout
is outside the existing instance-offset specialization contract. Free-threaded
guards keep their current owner lock and stop-the-world class reassignment
rules; no additional callback is introduced between guard and dereference.

Files: `Include/internal/pycore_feedback.h`, an inline helper in
`Include/internal/pycore_object.h`, `Python/specialize.c`, `Python/bytecodes.c`,
the generator's positive canonical-producer helper allowlist (and guard-local
type recognition only if necessary), focused `Lib/test/test_opcache.py` tests.
Coordinator regenerates all affected products and owns actual builds/checks.
No calls, arithmetic, iteration, frame or generator-construction changes.

Evidence required: focused tests warm multiple slot-bearing receiver layouts at
one site, then cover method/class value replacement, instance shadowing and
dict replacement, class reassignment, compatible base reassignment, shape
load/store/add and feedback transitions. Assert specialization before mutation.
Exercise existing snapshot/poly/reset coverage and geometry assertions. Native
release disassembly must show that the cached guard no longer loads
tp_basicsize; debug source assertions may still validate the witness. Root
measures the unchanged suite against the immutable baseline with the planned
attribute-heavy targets and logging/spectral-norm guards. No performance claim
precedes native evidence and matched timing.

Timebox: proposal review before source edits; implementation by 23:20 UTC.
If broad shape/store coverage needs a larger abstraction or cannot retain the
geometry cleanly, cut to the existing WITH_VALUES/INSTANCE_VALUE validity
guard users for this isolated candidate. Do not add a second candidate.
