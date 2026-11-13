# M2 nested retirement source review

2026-11-12: GC/frame lanes independently reviewed the first actual retirement
bridge against integrated typed-pin base `49c081b0358`. Source review only;
coordinator compile, generated output, native review and candidate tests remain
pending. NULL homes and conservative scans are retained.

GC/header disposition: frames peer GO. The nested chain describes an initialized
remaining prefix, explicit typed header bits and one independent result. The
visitor performs no code/geometry lookup after code detach, uses the value API,
and is called for external marking only, including optional FT stack marking.
It is compiled without the tracing-verifier allocator requirement so focused
tests can inspect the precise bridge directly. No container subtraction edge
is added and no scanner is weakened.

Producer disposition: GO after the direct heap close and exception-anchor
corrections below. Final source diff and precise test fixtures reviewed.
Callback-free partial clearing stops before the first potentially reentrant
close. A noinline continuation owns the record through association/locals/
mapping/function release, final thread code close and evaluator heap exception
cleanup. Cursor/bit removal precedes close. Takeover removes transferred prefix
and header authority, retaining only the departing source code pin. Debug
snapshots check nested growth restores the reservation. Result publication
remains after retirement callbacks, and no-result unwind is a distinct caller.

Resolved during source review: a new clear-thread call initially omitted its
result argument on initialization failure; it now passes NULL. Heap saved_top
was not initialized by frame copy, so debug snapshots now read it only for
thread-owned frames. Code close may finalize mortal constants: result
registration correctly survives that close, with no subsequent map lookup.

Resolved source coverage finding: `Objects/genobject.c:gen_clear_frame` initially
cleared exception state after its direct frame-clear record popped. It now calls
the common result-aware `_PyFrame_ClearGenerator` boundary with NULL, spanning
frame and exception cleanup for direct and evaluator paths. Source review then
found the still-owned exception anchor needs explicit marking while the generator
is untracked. Root approved actual-used EXCEPTION=16; header/visitor now enforce
generator containment without code lookup. Final producer installs the bit for
generator storage, preserves it across takeover, and removes it before exception
close. The retained code bit is disarmed after cleanup.

The authored tests inspect precise IDs independently of conservative
success: remaining/result/nested-outer roots during collection and arena growth,
late caller result publication after mutation, final code-constant destruction,
and unwind cleanup. Association trace-destructor, direct created-generator
deallocation and generator-completion tests were added. The eighth test exercises
direct untracked-generator deallocation while a held GeneratorExit anchor remains
after higher-numbered local/scratch cleanup. Its unraisable hook retains only the
message, avoiding resurrection that would bypass the intended path. Runtime
evidence is still pending. No new quarantine or disabled test was introduced.

First coordinator release execution exposed a fixture phase error: the held
GeneratorExit test observed False. Source diagnosis traces injected exception
to `PyTraceBack_Here`, whose traceback strongly owns the frame. Takeover moves
Trigger into that escaped frame. Exception cleanup then removes its root bit
before decref, and traceback/frame destruction can trigger Trigger afterwards;
that already-finalizing exception must not be marked as a remaining owner.
Proposed fixture correction clears the caught error's traceback before yielding,
so Trigger executes during the intended direct generator-prefix retirement.
Root must verify this inference on the candidate. Keeping EXCEPTION set during
its own close would violate the root contract and is not a proposed fix.
