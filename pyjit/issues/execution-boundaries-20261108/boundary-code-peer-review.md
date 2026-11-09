# Independent boolean continuation code review

2026-11-08, working diff in `boundary-ir-20261108` against golden `0765e9be47f`.
This is source review; release compile and runtime evidence remain pending.

The amended implementation closes the initial design blockers: eligibility
checks a generated SAVES_IP capability derived from the same `no_save_ip`
property used by handler generation, on both pristine and live successors.
All debug builds decline the optimization, avoiding frame-local lltrace on a
shared handler. The three generated forms retain guard-before-consumption,
branch history and canonical stats/deopt indexing. Taken/fallthrough advances
are correct for the absolute JUMPTO encoding; a fallthrough preadvance is
replaced by JUMPTO on a taken edge.

Receiver/value ownership does not change: a successful guard proves an inline
bool before consumption, so no finalizer can execute in the skipped interval.
Instrumentation resets physical variants before patching any target. Canonical
coordinates and observer anchors survive. These claims preserve the selected
implementation behavior; exact finalizer timing is not a Python language law.
The separate lifetime-policy page correctly allows other reviewed policies.

Findings sent to implementer:

- Regenerate Python opcode metadata: the initial diff omitted new `_ir_opmap`
  names, despite generated C metadata and dispatch entries being present.
- Add an explicit no-save successor rejection case (IR_JUMP is stack-neutral
  and easy to construct), and negative boundary descriptor generation cases.
  These cover the proof that initially blocked admission.

Disposition: no further C ownership blocker found in this source pass. Resolve
those producer/test gaps and obtain the coordinator's release/debug builds,
focused observer tests and generated-product review before final approval.
A source review cannot establish performance or merged integration.

Follow-up source pass: Python opcode metadata now contains the physical names.
The test file now checks generated absence of dynamic NOP traversal, emitted
single PC save, canonical opcode accounting, invalid boundary masks/bases,
SAVES_IP exclusions, and an explicit post-NOP IR_JUMP rejection case. These
close the source/producer gaps above. No remaining source blocker; native
compile/runtime and integrated generator review remain pending.
