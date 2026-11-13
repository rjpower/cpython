# Super lookup to CALL_EX normal transport

The matrix failure is the old class input at `base+1`, not a missing runtime
NULL write. With an even LOAD_SUPER_ATTR argument the optional self output is
absent. The actual normal flush consumes the class input; the existing
NORMAL_CLEAR_INPUTS consumer can therefore establish NO_HOME where the input
is consumed and DefinesHome is false. An odd argument defines the optional
self output and cannot inherit that absence proof.

The generic producer lacked the event because its instrumentation helpers
use `frame`, which the normal-vacate gate rejected wholesale. The bounded
fix accepts an instrumentation frame-use footprint, without recognizing an
opcode name or duplicating the whole super body:

- Every frame occurrence is the sole third argument of a global audited
  instrumentation helper. The first four arguments are exactly tstate,
  an allowed literal event, frame and this_instr; total argument count is six.
- The two-argument helper permits CALL/C_RETURN; the exception helper permits
  C_RAISE. The frame setter rejects scratch redirects from these events
  (`can't jump during a call` or line-event eligibility), while f_locals can
  still mutate named bindings.
- Extra frame arguments, aliases, raw fields, helper rebinding, other event
  kinds, unknown preprocessing and changed parsed branch structure reject.
  Existing slots/window/raw-dispatch and frame-switch exclusions remain.
- The actual emitted final flush plus canonical-family closure still create
  the ownership event. This grants only consumed-input absence, not output
  readability, phase coverage, or a general observer transport recipe.

Source scope is ir_generator.py, source parser perturbations in
 test_generated_cases.py, and focused runtime fixtures in
 test_ir_super_call_transport.py. No runtime C, opcode format, semantic event,
row or storage changes are needed. The pins lane's delegated frame changes
are a separate unstaged packet.

IR and GC source peers are GO. The host-only source parser check passes all four actual
super family members plus a renamed positive body and invalid helper prefixes,
events, positions, aliases and preprocessor/AST variants. Runtime fixtures
cover ordinary super EX behavior across constructor/replace/marshal routes,
an odd optional-output negative, observer named-local mutation with rejected
redirect, and the existing attribute-error route. The isolated three-file commit is de1e3de49ef. Root's regeneration/native
validation remains required; no candidate ran in this lane.
