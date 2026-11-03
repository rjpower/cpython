# Stage 5: extension ecosystem

State: baseline core/bundled ports are active; wider ecosystem migration and
the eventual opaque backend remain pending. This track can overlap other stages.

Rebuild extensions. Hard upstream source/binary compatibility is not a gate and
does not require further user approval. Make source ports practical and
incremental: owning, fallible object bridges first, then adoption of a deliberately
opaque API inspired by handle-based systems such as HPy or V8.

Workstreams are public handles/scopes and error contracts; generators and
mechanical source-port rules; native/resource/escaping-output exceptions;
private build/packaging identity; and declared extension qualification.
The current parser public-wrapper proposal is still a design packet, not a
released SDK or implemented HPy backend.

The baseline profile must qualify its required modules before measurement;
unrelated external/platform profiles remain explicit queued ports. Exit for a
declared profile requires reproducible rebuilds plus behavior, ownership,
exception and resource tests. A successful compile alone does not admit it.

References: [extension API design](../../design/extension-api.md),
[native-boundary workstream](../01-runtime/workstreams/native-boundaries.md),
[process](../../PROCESS.md), [current status](../../STATUS.md).
