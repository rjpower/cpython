# Provider record kind: frames source peer

2026-11-12 source GO for GC's four-file kind/visitor/query increment, now in the
provider base. EXTERNAL=0, RETIRING=1 and STAGED=2 occupy the existing result byte;
result_present is one bit. Static assertions retain 40-byte size and reason/
status offsets 22/23. InitClearRoots supplies RETIRING explicitly.

A NULL-frame visitor branch asserts STAGED, zero remaining/header/result
presence and NULL prefix before skipping. Pop permits NULL only for STAGED and
still requires all authority exhausted. This is explicit empty-scope handling,
not a generic fallback for malformed records. The query reports every node as
(kind, reason, status, has_frame), including disarmed provider scopes, and never
reads code or frame geometry. Existing retirement fixtures select their kind
explicitly. Root owns native qualification; no candidate execution ran here.
